// SPDX-License-Identifier: MPL-2.0
// EN: text_raster_sanity (D2D-TEXT, T1a, Onda 6 -- docs/superpowers/plans/2026-07-24-onda6-
//     draw2d-text.md, TX2 (a), TX17's hand-computed-value gate, raster half) -- PURE unit test
//     for src/text_raster.hpp: no window, no GL context, no RmlUi/GLFW dependency whatsoever
//     (same "nothing display-related to isolate here" reasoning as every other draw2d pure-seam
//     test in this suite). Compiles the vendored sovereign C core (sfnt.c/raster.c/hint.c)
//     DIRECTLY into THIS executable (tests/CMakeLists.txt, via ADR-0018 (e)/TX9's own widened
//     `_glx_core_src_dir`/`_glx_core_include_dir` gate) rather than linking `glintfx` -- proves
//     TX9's decoupling holds standalone: this target builds and passes even with
//     GLINTFX_OWN_FONT_ENGINE=OFF, since it is gated on GLINTFX_MODULE_DRAW2D alone (see
//     tests/CMakeLists.txt's own comment on this target).
//
//     ORACLE METHOD (same discipline tests/test_sfnt.c's own file header documents for the
//     Layer-0 suite -- "só p/ gerar os esperados, nunca código"): every advance/kern/metrics
//     value asserted below was derived INDEPENDENTLY with `fontTools`
//     (https://github.com/fonttools/fonttools, MIT, a Python OpenType library -- NOT this
//     codebase's own parser, unrelated code) reading OpenSans-Regular.ttf/PixelOperatorMono.ttf's
//     head/hhea/hmtx/kern tables directly, THEN hand-multiplied by size_px/units_per_em on paper
//     -- never by calling text_raster.hpp's own code and comparing it to itself. `size_px` was
//     picked in BOTH fixtures so units_per_em divides it into an EXACT power-of-two fraction
//     (16/2048 = 1/128 for OpenSans; 100/1600 = 1/16 for PixelOperatorMono) -- every expected
//     pixel value below is therefore EXACTLY representable in IEEE754 float, asserted with `==`,
//     no epsilon (the TX17 "no tolerance" discipline: a sign-flipped or off-by-one-bitshift
//     formula cannot slip through a fuzzy comparison here). Commands used (not committed, this
//     comment IS the record): `TTFont("demos/showcase/assets/OpenSans-Regular.ttf")`, reading
//     `head.unitsPerEm`, `hhea.ascender/descender/lineGap`, `hmtx[glyphname]`,
//     `kern.kernTables[0].kernTable[(left, right)]`, and `getGlyphOrder().index(glyphname)` for
//     gids; same for PixelOperatorMono.ttf (which has NO `kern`/`GPOS` table at all -- confirmed
//     via `'kern' in f` / `'GPOS' in f`, both False, the fixture's own oracle for the "no kern
//     table" degrade path below).
//
//     OpenSans-Regular.ttf oracle (unitsPerEm=2048, ascender=2189, descender=-600, lineGap=0):
//       gid(A)=36 gid(V)=57 gid(T)=55 gid(a)=68 gid(period)=17
//       hmtx: A=(1296,0) V=(1219,0) T=(1133,18) a=(1139,94) period=(545,152)
//       kern pairs (format 0): (A,V)=-82 (T,period)=-123 (V,a)=-41 (A,T)=-143 (T,A)=-143
//     PixelOperatorMono.ttf oracle (unitsPerEm=1600, ascender=1300, descender=-300, lineGap=72,
//       every glyph's hmtx advance = 800 -- genuinely monospace, no `kern`/`GPOS` table).
// PT: text_raster_sanity (D2D-TEXT, T1a, Onda 6 -- docs/superpowers/plans/2026-07-24-onda6-
//     draw2d-text.md, TX2 (a), gate de valor-à-mão do TX17, metade raster) -- teste unit PURO
//     para src/text_raster.hpp: sem janela, sem contexto GL, nenhuma dependência de RmlUi/GLFW
//     (mesmo racional "nada relacionado a display pra isolar aqui" de todo outro teste de costura
//     pura do draw2d nesta suíte). Compila o núcleo C soberano vendorizado (sfnt.c/raster.c/
//     hint.c) DIRETO neste executável (tests/CMakeLists.txt, via o gate alargado
//     `_glx_core_src_dir`/`_glx_core_include_dir` do ADR-0018 (e)/TX9) em vez de linkar
//     `glintfx` -- prova que o desacoplamento do TX9 aguenta sozinho: este alvo builda e passa
//     mesmo com GLINTFX_OWN_FONT_ENGINE=OFF, já que é gateado só por GLINTFX_MODULE_DRAW2D (ver
//     o comentário próprio deste alvo em tests/CMakeLists.txt).
//
//     MÉTODO DO ORÁCULO (mesma disciplina que o cabeçalho próprio do tests/test_sfnt.c documenta
//     pra suíte da Camada 0): todo valor de avanço/kern/métrica testado abaixo foi derivado
//     INDEPENDENTEMENTE com `fontTools` lendo as tabelas head/hhea/hmtx/kern de
//     OpenSans-Regular.ttf/PixelOperatorMono.ttf direto, DEPOIS multiplicado à mão no papel por
//     size_px/units_per_em -- nunca chamando o próprio código do text_raster.hpp e comparando com
//     ele mesmo. `size_px` foi escolhido nas DUAS fixtures pra units_per_em dividir numa fração
//     EXATA de potência-de-dois -- todo valor de pixel esperado abaixo é portanto EXATAMENTE
//     representável em float IEEE754, testado com `==`, sem epsilon.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/text_raster.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void check_eq_f(float got, float want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got %.9g, want %.9g)\n", what, (double)got, (double)want);
    ++g_failures;
  }
}

// EN: Reads a font fixture (copied to WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR by
//     tests/CMakeLists.txt at configure time, same convention as image_decode_sanity.cpp) into
//     an owned byte buffer.
// PT: Lê uma fixture de fonte (copiada pra WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR pelo
//     tests/CMakeLists.txt em tempo de configure, mesma convenção do image_decode_sanity.cpp)
//     num buffer de byte possuído.
std::vector<uint8_t> read_file(const char* path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

using namespace glintfx;

int main() {
  // ===========================================================================================
  // Group 1 -- UTF-8 decode (TX6's hostile matrix, hand-computed byte sequences -- see this
  // file's own top comment for the derivation of each expected {codepoint, consumed} pair).
  // ===========================================================================================
  {
    const char a[] = {(char)0x41};
    const Utf8Decoded d = decode_utf8_codepoint(a, 1);
    check(d.codepoint == 0x41u && d.consumed == 1, "utf8: 'A' (1-byte ASCII)");
  }
  {
    // 'a' with acute (U+00E1) -- 0xC3 0xA1.
    const char a[] = {(char)0xC3, (char)0xA1};
    const Utf8Decoded d = decode_utf8_codepoint(a, 2);
    check(d.codepoint == 0x00E1u && d.consumed == 2, "utf8: U+00E1 (valid 2-byte)");
  }
  {
    // EURO SIGN (U+20AC) -- 0xE2 0x82 0xAC.
    const char a[] = {(char)0xE2, (char)0x82, (char)0xAC};
    const Utf8Decoded d = decode_utf8_codepoint(a, 3);
    check(d.codepoint == 0x20ACu && d.consumed == 3, "utf8: U+20AC (valid 3-byte)");
  }
  {
    // GRINNING FACE (U+1F600) -- 0xF0 0x9F 0x98 0x80.
    const char a[] = {(char)0xF0, (char)0x9F, (char)0x98, (char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 4);
    check(d.codepoint == 0x1F600u && d.consumed == 4, "utf8: U+1F600 (valid 4-byte)");
  }
  {
    // Overlong 2-byte NUL -- 0xC0 0x80 (0xC0/0xC1 are ALWAYS overlong leads, rejected on the
    // lead byte alone before any continuation is even examined -- consumed == 1).
    const char a[] = {(char)0xC0, (char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 2);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: overlong 2-byte lead 0xC0 rejected");
  }
  {
    // Overlong 3-byte encoding of U+0000 -- 0xE0 0x80 0x80 (cp==0 < min_cp==0x800).
    const char a[] = {(char)0xE0, (char)0x80, (char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 3);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: overlong 3-byte encoding rejected");
  }
  {
    // Truncated 3-byte sequence -- lead 0xE2 + ONE valid continuation 0x82, then EOF (len==2).
    // consumed == 2 (lead + the one valid continuation byte consumed before running out).
    const char a[] = {(char)0xE2, (char)0x82};
    const Utf8Decoded d = decode_utf8_codepoint(a, 2);
    check(d.codepoint == 0xFFFDu && d.consumed == 2, "utf8: truncated 3-byte sequence at EOF");
  }
  {
    // Lone continuation byte, no lead before it.
    const char a[] = {(char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 1);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: lone continuation byte 0x80");
  }
  {
    // Invalid lead byte (5/6-byte sequences do not exist in UTF-8, 0xF8..0xFF is never valid).
    const char a[] = {(char)0xFF};
    const Utf8Decoded d = decode_utf8_codepoint(a, 1);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: invalid lead byte 0xFF");
  }
  {
    // UTF-16 surrogate U+D800 encoded as UTF-8 -- 0xED 0xA0 0x80 (illegal in UTF-8 by
    // definition; the whole 3-byte sequence resolves to cp==0xD800, then rejected -- consumed
    // == 1, the lead byte only, per this seam's own "reject the whole sequence's minimal unit"
    // policy documented in text_raster.hpp).
    const char a[] = {(char)0xED, (char)0xA0, (char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 3);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: UTF-16 surrogate U+D800 rejected");
  }
  {
    // U+110000, one past the Unicode maximum U+10FFFF -- 0xF4 0x90 0x80 0x80.
    const char a[] = {(char)0xF4, (char)0x90, (char)0x80, (char)0x80};
    const Utf8Decoded d = decode_utf8_codepoint(a, 4);
    check(d.codepoint == 0xFFFDu && d.consumed == 1, "utf8: codepoint above U+10FFFF rejected");
  }
  {
    // Hostile floor: null/zero-length input never dereferences utf8.
    check(decode_utf8_codepoint(nullptr, 5).consumed == 0, "utf8: null utf8 pointer (consumed==0)");
    const char a[] = {(char)0x41};
    check(decode_utf8_codepoint(a, 0).consumed == 0, "utf8: zero len (consumed==0)");
  }

  // ===========================================================================================
  // Group 2 -- TextFace::open() hostile floor (no font blob touched yet).
  // ===========================================================================================
  {
    check(!TextFace::open(nullptr, 0).ok(), "open: null blob rejected");
    const uint8_t garbage[4] = {0, 0, 0, 0};
    check(!TextFace::open(garbage, 0).ok(), "open: zero len rejected");
    check(!TextFace::open(garbage, 4).ok(), "open: 4 bytes of garbage rejected by the core parser");
  }

  // ===========================================================================================
  // Group 3 -- OpenSans-Regular.ttf, size_px=16 (scale = 16/2048 = 1/128, EXACT). Every expected
  // value below is the fontTools-derived font-unit value divided by 128 BY HAND (see this file's
  // own top comment for the full oracle table).
  // ===========================================================================================
  {
    const std::vector<uint8_t> blob = read_file("OpenSans-Regular.ttf");
    const TextFace face = TextFace::open(blob.data(), blob.size());
    check(face.ok(), "OpenSans: open() succeeds on a real, well-formed font");
    if (face.ok()) {
      check(face.raw() != nullptr && face.raw()->units_per_em == 2048,
            "OpenSans: units_per_em == 2048 (fontTools oracle)");

      const uint32_t gA = face.glyph_id('A');
      const uint32_t gV = face.glyph_id('V');
      const uint32_t gT = face.glyph_id('T');
      const uint32_t ga = face.glyph_id('a');
      const uint32_t gPeriod = face.glyph_id('.');
      check(gA == 36 && gV == 57 && gT == 55 && ga == 68 && gPeriod == 17,
            "OpenSans: cmap gid lookups match fontTools oracle (A=36 V=57 T=55 a=68 period=17)");

      const float size_px = 16.0f;
      check_eq_f(face.glyph_advance_px(gA, size_px), 10.125f, "OpenSans: advance(A) @16px");
      check_eq_f(face.glyph_advance_px(gV, size_px), 9.5234375f, "OpenSans: advance(V) @16px");
      check_eq_f(face.glyph_advance_px(gT, size_px), 8.8515625f, "OpenSans: advance(T) @16px");
      check_eq_f(face.glyph_advance_px(ga, size_px), 8.8984375f, "OpenSans: advance(a) @16px");
      check_eq_f(face.glyph_advance_px(gPeriod, size_px), 4.2578125f, "OpenSans: advance(period) @16px");

      check_eq_f(face.kern_px(gA, gV, size_px), -0.640625f, "OpenSans: kern(A,V) @16px");
      check_eq_f(face.kern_px(gT, gPeriod, size_px), -0.9609375f, "OpenSans: kern(T,period) @16px");
      check_eq_f(face.kern_px(gV, ga, size_px), -0.3203125f, "OpenSans: kern(V,a) @16px");
      check_eq_f(face.kern_px(gA, gT, size_px), -1.1171875f, "OpenSans: kern(A,T) @16px");
      check_eq_f(face.kern_px(gT, gA, size_px), -1.1171875f, "OpenSans: kern(T,A) @16px (asymmetric pair)");
      // EN: A pair with no kern entry at all (e.g. 'a' followed by 'a') is the neutral 0.f, NOT
      //     a failure -- distinguishes "no adjustment" from "no kern table" (both 0.f, but this
      //     exercises the "found table, no entry for THIS pair" branch specifically).
      // PT: Um par sem entrada de kern nenhuma (ex.: 'a' seguido de 'a') é o 0.f neutro, NÃO uma
      //     falha -- distingue "sem ajuste" de "sem tabela kern" (ambos 0.f, mas isto exercita o
      //     ramo "tabela achada, sem entrada pra ESTE par" especificamente).
      check_eq_f(face.kern_px(ga, ga, size_px), 0.0f, "OpenSans: kern(a,a) has no entry -> 0.f");

      const TextFace::Metrics m = face.metrics_px(size_px);
      check_eq_f(m.ascent, 17.1015625f, "OpenSans: ascent @16px");
      check_eq_f(m.descent, 4.6875f, "OpenSans: descent magnitude @16px (sign-flipped from hhea's -600)");
      check_eq_f(m.line_gap, 0.0f, "OpenSans: line_gap @16px");
      check_eq_f(m.line_height, 21.7890625f, "OpenSans: line_height = ascent+descent+line_gap @16px");

      // EN: An unmapped codepoint (U+E000, private-use area, confirmed absent from Open Sans'
      //     cmap via fontTools) resolves to gid 0 (.notdef), and .notdef's own advance is a
      //     legal, non-crashing query (never asserted against a specific value here -- .notdef's
      //     own metrics are font-defined and not part of this oracle, only that the call is
      //     safe).
      // PT: Um codepoint não-mapeado (U+E000, área de uso privado, confirmado ausente do cmap da
      //     Open Sans via fontTools) resolve pro gid 0 (.notdef), e o próprio avanço do .notdef
      //     é uma consulta legal, sem crash (nunca testado contra um valor específico aqui -- as
      //     próprias métricas do .notdef são definidas-pela-fonte e não fazem parte deste
      //     oráculo, só que a chamada é segura).
      const uint32_t gUnmapped = face.glyph_id(0xE000u);
      check(gUnmapped == 0, "OpenSans: unmapped codepoint U+E000 resolves to .notdef (gid 0)");
      const float notdef_advance = face.glyph_advance_px(gUnmapped, size_px);
      check(notdef_advance >= 0.0f, "OpenSans: .notdef advance query never crashes/negative");

      // EN: TX6 hostile floor on the advance/kern/metrics surface -- guarded BEFORE any
      //     arithmetic.
      // PT: Piso hostil TX6 na superfície de avanço/kern/métricas -- guardado ANTES de qualquer
      //     conta.
      check_eq_f(face.glyph_advance_px(gA, 0.0f), 0.0f, "OpenSans: advance() size_px==0 -> 0.f");
      check_eq_f(face.glyph_advance_px(gA, -1.0f), 0.0f, "OpenSans: advance() negative size_px -> 0.f");
      check_eq_f(face.glyph_advance_px(gA, 1.0f / 0.0f), 0.0f, "OpenSans: advance() +inf size_px -> 0.f");
      check_eq_f(face.glyph_advance_px(999999u, size_px), 0.0f, "OpenSans: advance() out-of-range gid -> 0.f");
      check_eq_f(face.kern_px(999999u, gA, size_px), 0.0f, "OpenSans: kern() out-of-range left gid -> 0.f");

      // EN: TX2 (a) glyph bake -- 'A' at 16px on the no-camera path (vsnap=true) yields a
      //     non-empty bitmap, and plan_glyph_bake()/bake_glyph_into() agree on its dimensions
      //     (single-source geometry, see this seam's own doc-comment on why the two can never
      //     disagree).
      // PT: Bake de glifo do TX2 (a) -- 'A' em 16px no caminho sem-câmera (vsnap=true) produz um
      //     bitmap não-vazio, e plan_glyph_bake()/bake_glyph_into() concordam nas dimensões
      //     (geometria de fonte única, ver o doc-comment próprio desta costura pro porquê dos
      //     dois nunca poderem discordar).
      const GlyphBakePlan plan = plan_glyph_bake(face, gA, size_px, /*vsnap=*/true);
      check(plan.ok && plan.bitmap_w > 0 && plan.bitmap_h > 0, "OpenSans: plan_glyph_bake('A') yields a non-empty bitmap");
      if (plan.ok && plan.bitmap_w > 0 && plan.bitmap_h > 0) {
        std::vector<uint8_t> bitmap((size_t)plan.bitmap_w * (size_t)plan.bitmap_h, 0xAA);
        const std::size_t scratch_n = glyph_bake_scratch_floats(plan.bitmap_w, plan.bitmap_h);
        std::vector<float> scratch(scratch_n);
        const bool baked = bake_glyph_into(face, gA, size_px, /*vsnap=*/true, plan.bitmap_w, plan.bitmap_h,
                                           bitmap.data(), plan.bitmap_w, scratch.data(), scratch.size());
        check(baked, "OpenSans: bake_glyph_into('A') succeeds against its own plan");

        // EN: At least one coverage byte in the baked bitmap is non-zero -- proves the
        //     rasterizer actually WROTE ink (not merely returned true over an all-zero buffer);
        //     a full-bitmap sweep for a specific coverage VALUE would be a golden-image test
        //     (out of scope here, this is TX17's hand-value gate for advance/kern/geometry, not
        //     pixel-exact AA -- the adversarial T3 pass owns pixel-readback).
        // PT: Ao menos um byte de cobertura no bitmap assado é não-zero -- prova que o
        //     rasterizador de fato ESCREVEU tinta (não só retornou true sobre um buffer
        //     tudo-zero); uma varredura de bitmap inteiro por um VALOR específico de cobertura
        //     seria um teste golden-image (fora de escopo aqui, este é o gate de valor-à-mão do
        //     TX17 pra avanço/kern/geometria, não AA pixel-exato -- a fatia adversarial T3 é
        //     dona do pixel-readback).
        bool any_ink = false;
        for (unsigned char c : bitmap)
          if (c != 0) any_ink = true;
        check(any_ink, "OpenSans: bake_glyph_into('A') writes at least one non-zero coverage byte");

        // EN: A mismatched plan (stale size_px) is rejected WITHOUT writing -- the stale-plan
        //     guard bake_glyph_into()'s own doc-comment names.
        // PT: Um plano descasado (size_px obsoleto) é rejeitado SEM escrever -- a guarda de
        //     plano obsoleto que o doc-comment próprio do bake_glyph_into() nomeia.
        check(!bake_glyph_into(face, gA, size_px, /*vsnap=*/true, plan.bitmap_w + 1, plan.bitmap_h,
                               bitmap.data(), plan.bitmap_w + 1, scratch.data(), scratch.size()),
              "OpenSans: bake_glyph_into() rejects a mismatched expect_w");
      }

      // EN: 'space' (U+0020) is the canonical zero-ink glyph -- plan_glyph_bake() reports
      //     ok==true with bitmap_w==bitmap_h==0 (a valid "no bitmap baked" answer, never a
      //     failure), and bake_glyph_into() with expect_w==expect_h==0 is a true, no-op success.
      // PT: 'espaço' (U+0020) é o glifo canônico sem-tinta -- plan_glyph_bake() reporta ok==true
      //     com bitmap_w==bitmap_h==0 (uma resposta válida de "nenhum bitmap assado", nunca uma
      //     falha), e bake_glyph_into() com expect_w==expect_h==0 é um sucesso no-op verdadeiro.
      const uint32_t gSpace = face.glyph_id(' ');
      const GlyphBakePlan space_plan = plan_glyph_bake(face, gSpace, size_px, /*vsnap=*/true);
      check(space_plan.ok && space_plan.bitmap_w == 0 && space_plan.bitmap_h == 0,
            "OpenSans: plan_glyph_bake(' ') is a valid zero-ink glyph (ok, w==h==0)");
      check(bake_glyph_into(face, gSpace, size_px, /*vsnap=*/true, 0, 0, nullptr, 0, nullptr, 0),
            "OpenSans: bake_glyph_into(' ') with expect_w==expect_h==0 is a true no-op");
    }
  }

  // ===========================================================================================
  // Group 4 -- PixelOperatorMono.ttf, size_px=100 (scale = 100/1600 = 1/16, EXACT). Genuinely
  // monospace (every glyph's own advance == 800 funits) and has NO `kern`/`GPOS` table at all --
  // the fixture's own oracle for the "kern table absent" degrade path (distinct from Group 3's
  // "table present, no entry for this pair" case).
  // ===========================================================================================
  {
    const std::vector<uint8_t> blob = read_file("PixelOperatorMono.ttf");
    const TextFace face = TextFace::open(blob.data(), blob.size());
    check(face.ok(), "PixelOperatorMono: open() succeeds");
    if (face.ok()) {
      check(face.raw() != nullptr && face.raw()->units_per_em == 1600,
            "PixelOperatorMono: units_per_em == 1600 (fontTools oracle)");

      const float size_px = 100.0f;
      const uint32_t gA = face.glyph_id('A');
      const uint32_t gW = face.glyph_id('W');
      const uint32_t gi = face.glyph_id('i');
      check(gA != 0 && gW != 0 && gi != 0, "PixelOperatorMono: 'A'/'W'/'i' all map to a real glyph");

      // EN: Monospace: EVERY glyph's advance is EXACTLY 50.0px @100px (800 funits / 16) --
      //     'A' (wide-looking) and 'i' (narrow-looking) and 'W' (widest-looking) all agree, the
      //     cheapest mutation-killer there is for the advance formula (a sign flip, a swapped
      //     numerator/denominator, or an off-by-one bitshift would break this uniformity
      //     immediately).
      // PT: Monoespaçada: TODO glifo tem avanço EXATAMENTE 50.0px @100px (800 funits / 16) --
      //     'A' (parece largo) e 'i' (parece estreito) e 'W' (parece o mais largo) todos
      //     concordam, o mata-mutante mais barato que existe pra fórmula de avanço (um sinal
      //     trocado, um numerador/denominador trocado, ou um off-by-one de bitshift quebraria
      //     esta uniformidade imediatamente).
      check_eq_f(face.glyph_advance_px(gA, size_px), 50.0f, "PixelOperatorMono: advance(A) @100px == 50.0 (monospace)");
      check_eq_f(face.glyph_advance_px(gW, size_px), 50.0f, "PixelOperatorMono: advance(W) @100px == 50.0 (monospace)");
      check_eq_f(face.glyph_advance_px(gi, size_px), 50.0f, "PixelOperatorMono: advance(i) @100px == 50.0 (monospace)");

      // EN: No `kern` table at all -- every pair is the neutral 0.f (distinct code path from
      //     Group 3's "table present, pair absent" case: face_.kern_len == 0 here).
      // PT: Nenhuma tabela `kern` -- todo par é o 0.f neutro (caminho de código distinto do caso
      //     "tabela presente, par ausente" do Group 3: face_.kern_len == 0 aqui).
      check_eq_f(face.kern_px(gA, gW, size_px), 0.0f, "PixelOperatorMono: kern() with no kern table -> 0.f");

      const TextFace::Metrics m = face.metrics_px(size_px);
      check_eq_f(m.ascent, 81.25f, "PixelOperatorMono: ascent @100px");
      check_eq_f(m.descent, 18.75f, "PixelOperatorMono: descent magnitude @100px");
      check_eq_f(m.line_gap, 4.5f, "PixelOperatorMono: line_gap @100px");
      check_eq_f(m.line_height, 104.5f, "PixelOperatorMono: line_height @100px");
    }
  }

  // ===========================================================================================
  // Group 5 -- the frozen GlyphAdvanceSource contract (make_advance_source()) -- T1b's own
  // consumption surface, exercised end-to-end here so a mistake in the trampolines is caught at
  // T1a's own gate, not discovered later inside text_layout.hpp.
  // ===========================================================================================
  {
    const std::vector<uint8_t> blob = read_file("OpenSans-Regular.ttf");
    const TextFace face = TextFace::open(blob.data(), blob.size());
    check(face.ok(), "GlyphAdvanceSource: OpenSans opens for the source-adapter group");
    if (face.ok()) {
      const AdvanceContext ctx{&face, 16.0f};
      const GlyphAdvanceSource src = make_advance_source(ctx);
      check(src.ok(), "GlyphAdvanceSource: make_advance_source() is ok() for a valid face+size_px");
      if (src.ok()) {
        const uint32_t gA = src.glyph_id(src.ctx, 'A');
        const uint32_t gV = src.glyph_id(src.ctx, 'V');
        check(gA == 36 && gV == 57, "GlyphAdvanceSource: glyph_id() trampoline matches TextFace directly");
        check_eq_f(src.advance_px(src.ctx, gA), 10.125f, "GlyphAdvanceSource: advance_px() trampoline matches TextFace directly");
        check_eq_f(src.kern_px(src.ctx, gA, gV), -0.640625f, "GlyphAdvanceSource: kern_px() trampoline matches TextFace directly");
        check_eq_f(src.ascent_px(src.ctx), 17.1015625f, "GlyphAdvanceSource: ascent_px() trampoline matches TextFace directly");
        check_eq_f(src.line_height_px(src.ctx), 21.7890625f, "GlyphAdvanceSource: line_height_px() trampoline matches TextFace directly");
      }

      // EN: A not-ok() face / non-finite size_px yields a not-ok() source with EVERY function
      //     pointer null -- text_layout.hpp's own entry point checks ok() first and never
      //     dereferences one of these.
      // PT: Uma face não-ok() / size_px não-finito produz uma source não-ok() com TODO ponteiro
      //     de função nulo -- o próprio ponto de entrada do text_layout.hpp checa ok() primeiro
      //     e nunca desreferencia um destes.
      const TextFace bad_face;
      const AdvanceContext bad_ctx1{&bad_face, 16.0f};
      check(!make_advance_source(bad_ctx1).ok(), "GlyphAdvanceSource: not-ok() face yields not-ok() source");
      const AdvanceContext bad_ctx2{&face, 0.0f};
      check(!make_advance_source(bad_ctx2).ok(), "GlyphAdvanceSource: size_px==0 yields not-ok() source");
      const AdvanceContext bad_ctx3{nullptr, 16.0f};
      check(!make_advance_source(bad_ctx3).ok(), "GlyphAdvanceSource: null face pointer yields not-ok() source");
    }
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "text_raster_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("text_raster_sanity: PASS");
  return 0;
}
