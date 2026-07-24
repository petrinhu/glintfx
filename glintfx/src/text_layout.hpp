// SPDX-License-Identifier: MPL-2.0
// EN: D2D-TEXT (Onda 6, T1b -- docs/superpowers/plans/2026-07-24-onda6-draw2d-text.md, ADR-0018)
//     -- the LAYOUT seam of Draw2D's text machinery (TX2 (b)). PURE positioning math: ZERO GL,
//     ZERO raster, ZERO SFNT -- it never rasterizes a pixel, never opens a face, never calls a
//     glx_* function. Same header-only, hand-computable discipline as src/transform2d.hpp: it
//     takes an advance-measuring interface plus a decoded codepoint run and returns positioned
//     glyphs (PlacedGlyph{gid, pen_x, pen_y}) + block metrics, headless-testable
//     (tests/text_layout_sanity.cpp) with NO font, NO core link -- a monospace MOCK of
//     GlyphAdvanceSource is all its tests need (see that file's own header).
//
//     WHAT LIVES HERE (TX2 (b), TX15, TX16): explicit '\n' breaking, greedy word-wrap against a
//     caller-supplied max_width (TX15), and per-line alignment incl. justify space distribution
//     (TX16). SINGLE SOURCE: measure_text and draw_text (T2, in draw2d.cpp) BOTH call layout_text()
//     -- a mutation in the advance/wrap/align math must break both, exactly the transform2d.hpp/
//     primitives2d.hpp single-source contract this house's own postmortem demands ("a conversion
//     formula without a computed-value delta test" is the named silent-bug class).
//
//     WHAT DOES NOT LIVE HERE (the frozen split, ADR-0018 (a)/(f)): UTF-8 decode is text_raster's
//     (TX2 (a), decode_utf8_codepoint) -- this seam consumes ALREADY-decoded codepoints so it
//     carries ZERO decode duplication and ZERO link dependency on text_raster.cpp/the sovereign
//     core. It #includes text_raster.hpp for exactly ONE type -- the frozen GlyphAdvanceSource
//     (text_raster.hpp:225) -- and uses NOTHING else from it (no TextFace, no glx_* call): the
//     transitive core/*.h headers text_raster.hpp pulls are compile-time-only here, never linked.
//     (A truly compile-time-isolated GlyphAdvanceSource would live in its own tiny header both
//     seams include; T1a froze it inside text_raster.hpp and this slice must not edit that file, so
//     the isolation is behavioural, not physical -- see this slice's hand-off note.)
//
//     ADVANCE/SNAP SPLIT (why layout never sees a "snap" flag, mirroring text_raster.hpp's own
//     rule): wrap and align are geometry over the RAW, unsnapped advance/kern -- their results are
//     independent of whether a camera is active (TX8's pen-snap/vsnap is strictly a render/bake
//     concern the caller layers on top of this float layout, never fed back in). pen_x/pen_y here
//     are therefore always the exact float positions relative to the block's TOP-LEFT origin (0,0):
//     pen_x is the glyph's horizontal pen origin (align/justify already folded in), pen_y is the
//     BASELINE of the glyph's line (line i baseline = ascent + i*line_height). The caller (Draw2d,
//     T2) adds the public `pos`, projects per-corner through the active camera (TX7), and -- only in
//     the no-camera integer-size path -- rounds (pen-snap X) before placing the baked quad at
//     (pen_x + bake.bearing_x, pen_y + bake.bearing_y).
//
//     TOTALITY / FAIL-HIGH (TX6): a not-ok() source, a null codepoint pointer, or count == 0 ->
//     an all-zero LayoutResult (line_count 0, no glyphs), never a crash, never a dereference of a
//     null function pointer. A non-finite max_width degrades to "no wrap" (every `> max_width`
//     comparison against NaN is false -- the loop still terminates, no NaN escapes into a
//     position); the caller (T2) is expected to reject a non-finite max_width BEFORE calling per
//     TX6, this is the belt-and-suspenders floor under that. The per-call 1 MiB BYTE cap (TX6) is
//     the decoder's (T2) job -- this seam has no bytes, only the already-decoded, already-bounded
//     codepoint run.
// PT: D2D-TEXT (Onda 6, T1b -- docs/superpowers/plans/2026-07-24-onda6-draw2d-text.md, ADR-0018)
//     -- a costura de LAYOUT da maquinaria de texto do Draw2D (TX2 (b)). Matemática PURA de
//     posicionamento: ZERO GL, ZERO raster, ZERO SFNT -- nunca rasteriza um pixel, nunca abre uma
//     face, nunca chama uma função glx_*. Mesma disciplina header-only, computável-à-mão do
//     src/transform2d.hpp: recebe uma interface de medição de avanço mais um run de codepoints
//     decodificados e devolve glifos posicionados (PlacedGlyph{gid, pen_x, pen_y}) + métricas do
//     bloco, testável headless (tests/text_layout_sanity.cpp) SEM fonte, SEM link do core -- um
//     MOCK monoespaçado de GlyphAdvanceSource é tudo que os testes dele precisam.
//
//     O QUE MORA AQUI (TX2 (b), TX15, TX16): quebra explícita em '\n', word-wrap guloso contra um
//     max_width fornecido por quem chama (TX15), e alinhamento por linha inclusive distribuição de
//     espaço do justify (TX16). FONTE ÚNICA: measure_text e draw_text (T2, em draw2d.cpp) AMBOS
//     chamam layout_text() -- uma mutação na matemática de avanço/wrap/align tem que quebrar os
//     dois, exatamente o contrato de fonte única do transform2d.hpp/primitives2d.hpp que o
//     postmortem desta casa exige ("fórmula de conversão sem teste de valor computado" é a classe
//     de bug silencioso nomeada).
//
//     O QUE NÃO MORA AQUI (a divisão congelada, ADR-0018 (a)/(f)): decode UTF-8 é do text_raster
//     (TX2 (a), decode_utf8_codepoint) -- esta costura consome codepoints JÁ decodificados, então
//     carrega ZERO duplicação de decode e ZERO dependência de link do text_raster.cpp/do núcleo
//     soberano. Inclui o text_raster.hpp por exatamente UM tipo -- a GlyphAdvanceSource congelada
//     (text_raster.hpp:225) -- e usa NADA além dele (nenhum TextFace, nenhuma chamada glx_*): os
//     headers core/*.h transitivos que o text_raster.hpp puxa são só-compile-time aqui, nunca
//     linkados. (Uma GlyphAdvanceSource isolada de verdade em compile-time moraria num header
//     minúsculo próprio que as duas costuras incluiriam; o T1a a congelou dentro do text_raster.hpp
//     e esta fatia não pode editar aquele arquivo, então o isolamento é comportamental, não
//     físico -- ver a nota de hand-off desta fatia.)
//
//     DIVISÃO AVANÇO/SNAP (por que o layout nunca vê uma flag "snap", espelhando a regra própria do
//     text_raster.hpp): wrap e align são geometria sobre o avanço/kern CRU, não-snapado -- seus
//     resultados independem de haver câmera ativa (o pen-snap/vsnap do TX8 é estritamente
//     preocupação de render/bake que quem chama sobrepõe a este layout float, nunca realimentada).
//     pen_x/pen_y aqui são portanto sempre as posições float exatas relativas à origem TOPO-ESQUERDO
//     do bloco (0,0): pen_x é a origem horizontal da pena do glifo (align/justify já embutidos),
//     pen_y é a BASELINE da linha do glifo (baseline da linha i = ascent + i*line_height). Quem
//     chama (Draw2d, T2) soma o `pos` público, projeta canto-a-canto pela câmera ativa (TX7), e --
//     só no caminho sem-câmera em tamanho-inteiro -- arredonda (pen-snap X) antes de pôr o quad
//     assado em (pen_x + bake.bearing_x, pen_y + bake.bearing_y).
//
//     TOTALIDADE / FAIL-HIGH (TX6): uma source não-ok(), um ponteiro de codepoint nulo, ou
//     count == 0 -> um LayoutResult todo-zero (line_count 0, sem glifos), nunca crash, nunca
//     desreferência de ponteiro de função nulo. Um max_width não-finito degrada pra "sem wrap"
//     (toda comparação `> max_width` contra NaN é falsa -- o laço ainda termina, nenhum NaN escapa
//     pra uma posição); quem chama (T2) é esperado a rejeitar um max_width não-finito ANTES de
//     chamar conforme o TX6, isto é o piso cinto-e-suspensório sob aquilo. O teto de 1 MiB de BYTE
//     por chamada (TX6) é trabalho do decodificador (T2) -- esta costura não tem bytes, só o run de
//     codepoints já decodificado, já limitado.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "text_raster.hpp" // EN: for the frozen GlyphAdvanceSource ONLY / PT: só pela GlyphAdvanceSource congelada

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// EN: Own detail namespace (mirrors transform2d.hpp's glintfx::draw2d_detail) -- deliberately NOT
//     the public glintfx:: namespace, so `Align` here never collides with the PUBLIC
//     `glintfx::TextAlign` T2 adds to <glintfx/draw2d.hpp> (TX1). The two enums share the SAME
//     value order (Left=0, Center=1, Right=2, Justify=3) on purpose: T2 maps public -> internal
//     with a single static_cast, and is expected to pin the correspondence with a static_assert on
//     the underlying values (the transform2d.hpp public-thin-wrapper-delegates-to-detail pattern).
// PT: Namespace de detalhe próprio (espelha o glintfx::draw2d_detail do transform2d.hpp) --
//     deliberadamente NÃO o namespace público glintfx::, pra que `Align` aqui nunca colida com o
//     `glintfx::TextAlign` PÚBLICO que o T2 adiciona ao <glintfx/draw2d.hpp> (TX1). Os dois enums
//     compartilham a MESMA ordem de valor (Left=0, Center=1, Right=2, Justify=3) de propósito: o T2
//     mapeia público -> interno com um static_cast só, e é esperado a fixar a correspondência com um
//     static_assert nos valores subjacentes (o padrão wrapper-público-fino-delega-pro-detalhe do
//     transform2d.hpp).
namespace glintfx::text_detail {

// EN: Per-line horizontal alignment (TX16). Word-spacing-only Justify (letter-spacing distribution
//     and hyphenation are SEED-D2D-TEXT-TYPESETTING, out of this wave).
// PT: Alinhamento horizontal por linha (TX16). Justify só-word-spacing (distribuição por
//     letter-spacing e hifenização são SEED-D2D-TEXT-TYPESETTING, fora desta onda).
enum class Align : int { Left = 0,
                         Center = 1,
                         Right = 2,
                         Justify = 3 };

// EN: One positioned glyph. `gid` is the face glyph id (0 == .notdef/tofu, TrueType's own "no
//     glyph"); pen_x/pen_y are float positions relative to the block's TOP-LEFT origin (0,0) --
//     pen_x the horizontal pen origin (align/justify folded in), pen_y the BASELINE of this glyph's
//     line. Space glyphs (U+0020) are EMITTED with their advance-derived position (they bake to an
//     empty bitmap downstream) so a caller can rely on positional continuity; '\n' is never a glyph
//     (it is a break, not ink).
// PT: Um glifo posicionado. `gid` é o id de glifo da face (0 == .notdef/tofu, o "sem glifo" próprio
//     do TrueType); pen_x/pen_y são posições float relativas à origem TOPO-ESQUERDO do bloco (0,0)
//     -- pen_x a origem horizontal da pena (align/justify embutidos), pen_y a BASELINE da linha
//     deste glifo. Glifos de espaço (U+0020) são EMITIDOS com sua posição derivada do avanço (eles
//     assam num bitmap vazio adiante) pra quem chama poder confiar na continuidade posicional; '\n'
//     nunca é glifo (é quebra, não tinta).
struct PlacedGlyph {
  uint32_t gid = 0;
  float pen_x = 0.f;
  float pen_y = 0.f;
};

// EN: The layout of a whole text block: the positioned glyphs (reading order) plus the block
//     metrics measure_text (T2) reports directly. `width` is the widest LINE's natural advance
//     width (the tight content width -- NOT the reference/max_width the block was aligned within);
//     `height` = line_count * line_height; `ascent`/`line_height` are the face's own metrics
//     (echoed so measure_text needs no second query and a baseline-precise caller converts with one
//     add, ADR-0018 (c)); `line_count` is the number of lines AFTER wrap + explicit '\n'.
// PT: O layout de um bloco de texto inteiro: os glifos posicionados (ordem de leitura) mais as
//     métricas do bloco que o measure_text (T2) reporta direto. `width` é a largura de avanço
//     natural da LINHA mais larga (a largura de conteúdo justa -- NÃO o max_width/largura de
//     referência dentro da qual o bloco foi alinhado); `height` = line_count * line_height;
//     `ascent`/`line_height` são as métricas próprias da face (ecoadas pro measure_text não
//     precisar de uma segunda consulta e quem precisa de baseline converter com uma soma, ADR-0018
//     (c)); `line_count` é o número de linhas DEPOIS do wrap + '\n' explícito.
struct LayoutResult {
  std::vector<PlacedGlyph> glyphs;
  float width = 0.f;
  float height = 0.f;
  float ascent = 0.f;
  float line_height = 0.f;
  int line_count = 0;
};

// EN: Lays out `count` already-decoded Unicode codepoints through the frozen `src` metrics
//     interface. `max_width <= 0` (or non-finite) is the documented "no wrap" sentinel (only '\n'
//     breaks then); `max_width > 0` enables greedy word-wrap in the SAME units as `src`'s advances
//     (active-space units: px without a camera, world units with -- this seam is unit-agnostic).
//     `align` positions each line within the reference width (max_width when wrapping, else the
//     block's widest line -- so centered/right multi-line '\n' text works with no wrap). See this
//     file's header for the full totality contract; the normative wrap rules are TX15 and the
//     normative align/justify rules are TX16, both reproduced inline below.
// PT: Faz o layout de `count` codepoints Unicode já decodificados através da interface de métricas
//     congelada `src`. `max_width <= 0` (ou não-finito) é a sentinela documentada de "sem wrap" (só
//     '\n' quebra então); `max_width > 0` liga word-wrap guloso nas MESMAS unidades dos avanços de
//     `src` (unidades do espaço ativo: px sem câmera, mundo com -- esta costura é agnóstica de
//     unidade). `align` posiciona cada linha dentro da largura de referência (max_width com wrap,
//     senão a linha mais larga do bloco -- então texto multi-linha '\n' centralizado/à-direita
//     funciona sem wrap). Ver o cabeçalho deste arquivo pro contrato de totalidade completo; as
//     regras normativas de wrap são o TX15 e as de align/justify o TX16, ambas reproduzidas inline
//     abaixo.
inline LayoutResult layout_text(const GlyphAdvanceSource& src, const uint32_t* codepoints,
                                std::size_t count, float max_width, Align align) {
  LayoutResult out;
  // EN: TX6 fail-high floor -- a not-ok() source or an empty/null run is a legal, inert no-op.
  // PT: Piso fail-high do TX6 -- uma source não-ok() ou um run vazio/nulo é um no-op legal, inerte.
  if (!src.ok() || codepoints == nullptr || count == 0) return out;

  const void* ctx = src.ctx;
  const auto advance = [&](uint32_t gid) -> float { return src.advance_px(ctx, gid); };
  const auto kern = [&](uint32_t a, uint32_t b) -> float { return src.kern_px(ctx, a, b); };

  out.ascent = src.ascent_px(ctx);
  out.line_height = src.line_height_px(ctx);
  const float ascent = out.ascent;
  const float line_height = out.line_height;

  // EN: Non-finite max_width degrades to "no wrap" (belt-and-suspenders under T2's own TX6 reject).
  // PT: max_width não-finito degrada pra "sem wrap" (cinto-e-suspensório sob o reject do TX6 do T2).
  const bool do_wrap = std::isfinite(max_width) && max_width > 0.f;

  // EN: A glyph on a line, pre-alignment. `is_space` marks U+0020 (a break opportunity AND a
  //     justify gap); U+00A0 (nbsp) is a normal, NON-breaking rendered glyph (is_space == false).
  // PT: Um glifo numa linha, pré-alinhamento. `is_space` marca U+0020 (oportunidade de quebra E gap
  //     de justify); U+00A0 (nbsp) é glifo normal, NÃO-quebrável, renderizado (is_space == false).
  struct AG {
    uint32_t gid;
    bool is_space;
  };
  // EN: A finished line. `soft_wrapped` == this line ended because of a WRAP (there are more lines
  //     in the same paragraph after it) -- the ONLY lines Justify touches. The last line of every
  //     paragraph (whether the paragraph ended by explicit '\n' or by end-of-run) has
  //     soft_wrapped == false, which is exactly TX16's "block's last line AND any line ended by an
  //     explicit '\n' are NOT justified" in one flag. `width` is the natural advance width.
  // PT: Uma linha pronta. `soft_wrapped` == esta linha terminou por causa de um WRAP (há mais linhas
  //     no mesmo parágrafo depois dela) -- as ÚNICAS linhas que o Justify toca. A última linha de
  //     todo parágrafo (terminado por '\n' explícito OU por fim-de-run) tem soft_wrapped == false,
  //     que é exatamente o "última linha do bloco E qualquer linha terminada por '\n' explícito NÃO
  //     justificam" do TX16 numa flag só. `width` é a largura de avanço natural.
  struct Line {
    std::vector<AG> glyphs;
    float width = 0.f;
    bool soft_wrapped = false;
  };
  std::vector<Line> lines;

  // --- current-line assembly state (reused across paragraphs) ---
  std::vector<AG> curr;
  float curr_w = 0.f;
  uint32_t curr_last = 0;
  bool curr_has = false;

  const auto reset_curr = [&]() {
    curr.clear();
    curr_w = 0.f;
    curr_last = 0;
    curr_has = false;
  };
  const auto push_curr = [&](bool soft_wrapped) {
    lines.push_back(Line{std::move(curr), curr_w, soft_wrapped});
    reset_curr();
  };
  // EN: Append one glyph to `curr`, force-breaking when it would exceed max_width -- the TX15
  //     "single word wider than max_width force-breaks at the last glyph that fits, MINIMUM one
  //     glyph per line" rule. The break is only ever taken when `curr` already holds >= 1 glyph
  //     (the curr_has branch), so a line is never emitted empty by a force-break and a single glyph
  //     wider than max_width is kept (it overflows by exactly that one mandatory glyph -- never an
  //     infinite loop, never vanishing text). Force-break lines are soft_wrapped (mid-word), but if
  //     they have no U+0020 they carry 0 justify gaps and fall back to Left anyway (TX16).
  // PT: Adiciona um glifo a `curr`, quebrando à força quando estouraria max_width -- a regra do
  //     TX15 "palavra maior que max_width quebra forçada no último glifo que cabe, MÍNIMO um glifo
  //     por linha". A quebra só é tomada quando `curr` já tem >= 1 glifo (o ramo curr_has), então
  //     uma linha nunca é emitida vazia por quebra forçada e um único glifo maior que max_width é
  //     mantido (estoura por exatamente esse um glifo obrigatório -- nunca loop infinito, nunca
  //     texto sumindo). Linhas de quebra forçada são soft_wrapped (meio-de-palavra), mas se não têm
  //     U+0020 carregam 0 gaps de justify e caem pra Left de qualquer jeito (TX16).
  const auto add_glyph_fb = [&](AG ag) {
    const float adv = advance(ag.gid);
    if (curr_has) {
      const float k = kern(curr_last, ag.gid);
      if (do_wrap && (curr_w + k + adv) > max_width) {
        push_curr(true);
        curr.push_back(ag);
        curr_w = adv;
        curr_has = true;
        curr_last = ag.gid;
      } else {
        curr.push_back(ag);
        curr_w += k + adv;
        curr_last = ag.gid;
      }
    } else {
      curr.push_back(ag);
      curr_w = adv;
      curr_has = true;
      curr_last = ag.gid;
    }
  };
  // EN: Append a whole run (already known to fit) to a non-empty `curr` with exact boundary kern --
  //     used for the pending-separator + word that the greedy check just proved fits, so no
  //     force-break path is needed here.
  // PT: Adiciona um run inteiro (já sabido que cabe) a um `curr` não-vazio com kern de fronteira
  //     exato -- usado pro separador-pendente + palavra que o cheque guloso acabou de provar que
  //     cabe, então nenhum caminho de quebra forçada é preciso aqui.
  const auto append_fitting = [&](const std::vector<AG>& run) {
    for (const AG& ag : run) {
      const float adv = advance(ag.gid);
      curr_w += kern(curr_last, ag.gid) + adv;
      curr.push_back(ag);
      curr_last = ag.gid;
    }
  };

  // EN: Lay out one paragraph (a maximal run with no '\n') into `lines`. Greedy word-wrap with the
  //     classic pending-separator model: spaces (U+0020) are break opportunities held as `pending`
  //     until the next word decides whether they render (word fits -> they render as an interior
  //     gap) or are consumed (word wraps -> the break space vanishes at the line edge, TX15).
  // PT: Faz o layout de um parágrafo (run maximal sem '\n') em `lines`. Word-wrap guloso com o
  //     modelo clássico de separador-pendente: espaços (U+0020) são oportunidades de quebra
  //     seguradas como `pending` até a próxima palavra decidir se renderizam (palavra cabe ->
  //     renderizam como gap interior) ou são consumidos (palavra quebra -> o espaço de quebra some
  //     na borda da linha, TX15).
  const auto layout_paragraph = [&](const std::vector<AG>& para) {
    reset_curr();
    std::vector<AG> pending; // the space-run separator awaiting its following word
    float pending_w = 0.f;
    bool pending_has = false;
    uint32_t pending_last = 0;

    const auto clear_pending = [&]() {
      pending.clear();
      pending_w = 0.f;
      pending_has = false;
      pending_last = 0;
    };

    std::size_t i = 0;
    const std::size_t n = para.size();
    while (i < n) {
      if (para[i].is_space) {
        // gather a maximal space run into `pending` (with internal kern)
        clear_pending();
        bool prevset = false;
        uint32_t prev = 0;
        while (i < n && para[i].is_space) {
          const uint32_t g = para[i].gid;
          if (prevset) pending_w += kern(prev, g);
          pending_w += advance(g);
          pending.push_back(para[i]);
          prev = g;
          prevset = true;
          ++i;
        }
        pending_last = prev;
        pending_has = !pending.empty();
        continue;
      }
      // gather a maximal word run (non-space, incl. nbsp) with its standalone width + endpoints
      std::vector<AG> word;
      float word_w = 0.f;
      uint32_t word_first = 0;
      uint32_t word_last = 0;
      bool wprevset = false;
      uint32_t wprev = 0;
      while (i < n && !para[i].is_space) {
        const uint32_t g = para[i].gid;
        if (wprevset) word_w += kern(wprev, g);
        word_w += advance(g);
        if (!wprevset) word_first = g;
        word.push_back(para[i]);
        wprev = g;
        wprevset = true;
        ++i;
      }
      word_last = wprev;
      (void)word_last;

      if (curr_has) {
        // candidate width of curr + pending + word (exact boundary kern, additive)
        float cand = curr_w;
        uint32_t prev = curr_last;
        if (pending_has) {
          cand += kern(prev, pending.front().gid) + pending_w;
          prev = pending_last;
        }
        cand += kern(prev, word_first) + word_w;
        if (do_wrap && cand > max_width) {
          // wrap: finish curr as a soft-wrapped line, CONSUME pending, word starts a fresh line
          push_curr(true);
          clear_pending();
          for (const AG& ag : word) add_glyph_fb(ag);
        } else {
          // fits: render the pending separator then the word on the current line
          if (pending_has) append_fitting(pending);
          append_fitting(word);
          clear_pending();
        }
      } else {
        // fresh/empty line: leading pending spaces render as indentation, then the word
        if (pending_has)
          for (const AG& ag : pending) add_glyph_fb(ag);
        clear_pending();
        for (const AG& ag : word) add_glyph_fb(ag);
      }
    }
    // trailing spaces with no following word render on the current (last) line
    if (pending_has) {
      for (const AG& ag : pending) {
        const float adv = advance(ag.gid);
        if (curr_has) {
          curr_w += kern(curr_last, ag.gid) + adv;
          curr.push_back(ag);
          curr_last = ag.gid;
        } else {
          curr.push_back(ag);
          curr_w = adv;
          curr_has = true;
          curr_last = ag.gid;
        }
      }
    }
    // the paragraph's final line is NEVER justified (soft_wrapped == false), even when empty
    push_curr(false);
  };

  // --- split the codepoint run into paragraphs on '\n' (U+000A) and lay each out ---
  std::vector<AG> para;
  for (std::size_t k = 0; k < count; ++k) {
    const uint32_t cp = codepoints[k];
    if (cp == 0x000Au) { // '\n' hard break
      layout_paragraph(para);
      para.clear();
      continue;
    }
    const uint32_t gid = src.glyph_id(ctx, cp);
    const bool is_space = (cp == 0x0020u);
    para.push_back(AG{gid, is_space});
  }
  layout_paragraph(para); // final paragraph (possibly empty -> one empty last line)

  // --- pen pass: exact base pen_x per glyph + authoritative line width + widest line ---
  std::vector<std::vector<float>> base_x(lines.size());
  float widest = 0.f;
  for (std::size_t li = 0; li < lines.size(); ++li) {
    Line& L = lines[li];
    base_x[li].resize(L.glyphs.size());
    float pen = 0.f;
    bool prevset = false;
    uint32_t prev = 0;
    for (std::size_t gi = 0; gi < L.glyphs.size(); ++gi) {
      const uint32_t g = L.glyphs[gi].gid;
      if (prevset) pen += kern(prev, g);
      base_x[li][gi] = pen;
      pen += advance(g);
      prev = g;
      prevset = true;
    }
    L.width = pen;
    if (pen > widest) widest = pen;
  }

  // EN: Reference width for alignment (TX16): max_width when wrapping, else the block's widest line
  //     (so Center/Right/Justify of multi-line '\n' text is meaningful with no wrap).
  // PT: Largura de referência pro alinhamento (TX16): max_width com wrap, senão a linha mais larga
  //     do bloco (pra Center/Right/Justify de texto multi-linha '\n' fazer sentido sem wrap).
  const float ref_w = do_wrap ? max_width : widest;

  out.width = widest;
  out.line_count = static_cast<int>(lines.size());
  out.height = static_cast<float>(out.line_count) * line_height;

  // --- emit: per-line alignment offset (+ justify gap distribution) folded into each pen_x ---
  for (std::size_t li = 0; li < lines.size(); ++li) {
    const Line& L = lines[li];
    const float baseline_y = ascent + static_cast<float>(li) * line_height;

    float offset = 0.f;   // Left/Center/Right rigid shift of the whole line
    bool justify = false; // Justify: offset stays 0, per-gap `shift` widens interior spaces
    float extra_per_gap = 0.f;
    switch (align) {
      case Align::Left:
        offset = 0.f;
        break;
      case Align::Center:
        offset = (ref_w - L.width) * 0.5f;
        break;
      case Align::Right:
        offset = ref_w - L.width;
        break;
      case Align::Justify: {
        // TX16: Justify degrades to Left when there is no target width (no wrap) and NEVER touches
        // the last line of a paragraph (soft_wrapped == false covers block-last + explicit-'\n').
        if (!do_wrap || !L.soft_wrapped) {
          offset = 0.f;
          break;
        }
        const int gaps = static_cast<int>(
            std::count_if(L.glyphs.begin(), L.glyphs.end(), [](const AG& ag) { return ag.is_space; }));
        if (gaps == 0) { // no interior space to stretch -> Left
          offset = 0.f;
          break;
        }
        justify = true;
        offset = 0.f;
        extra_per_gap = (ref_w - L.width) / static_cast<float>(gaps);
        break;
      }
    }

    float shift = 0.f; // Justify running offset, incremented past each interior space
    for (std::size_t gi = 0; gi < L.glyphs.size(); ++gi) {
      const AG& ag = L.glyphs[gi];
      const float x = base_x[li][gi] + offset + shift;
      out.glyphs.push_back(PlacedGlyph{ag.gid, x, baseline_y});
      if (justify && ag.is_space) shift += extra_per_gap;
    }
  }

  return out;
}

} // namespace glintfx::text_detail
