// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_lazy_sanity -- L1.20-FONTFLIP (FT-F4) sub-phase 2B ADVERSARIAL GATE for the lazy
//     on-demand bake (FontEngineOwn::EnsureGlyphs()/BakeGlyph()/GrowAtlas()/RasterizeGlyph(),
//     src/font_engine_own.cpp). Written by the INDEPENDENT QA reviewer -- NOT the implementer of
//     sub-phase 2B (commit 7f119f1) -- as its own gate, not a rubber stamp of the implementer's own
//     claims. Executes the real GL/RmlUi pipeline under Xvfb (statistical ink oracle + numeric
//     width oracle, the two techniques this suite already established in fonteng_extended_sanity.cpp
//     / fonteng_own_kern_sanity.cpp), never merely reads source and asserts by inspection.
//
//     WHAT SUB-PHASE 2B CLAIMS (per its own doc-comments): the eager warm set (BuildBakeSet(),
//     ~199 codepoints: printable ASCII + Latin-1 Supplement U+00A0..U+00FF + typographic
//     punctuation up to U+2026) is now ADDITIVELY topped up -- any codepoint missing from
//     inst.glyphs when a string is measured/rendered gets baked on the spot (EnsureGlyphs(), called
//     from both GetStringWidth() and GenerateString() before either touches the glyph map), the
//     atlas grows via amortized doubling (GrowAtlas()) without moving any already-baked glyph, and
//     a codepoint the face genuinely lacks (gid==0) is negative-cached so it is never re-attempted.
//
//     THE SIX CRITERIA THIS FILE GATES (see main() for the per-check mapping to review criteria
//     1-5; criterion 6, the sanitizer run, is NOT in this .cpp -- it is this SAME binary run a
//     second time from a `-DGLINTFX_SANITIZE=ON` build tree, see this ticket's review report):
//
//     [1] NON-TOFU OUT-OF-LATIN-1. Four codepoints named explicitly by the review brief -- Greek
//         U+0391 (Α, GREEK CAPITAL LETTER ALPHA) and U+03B1 (α, GREEK SMALL LETTER ALPHA), Cyrillic
//         U+0410 (А, CYRILLIC CAPITAL LETTER A) and U+0434 (д, CYRILLIC SMALL LETTER DE) -- all sit
//         well above the warm set's own ceiling (U+2026) and are all present in Open Sans (common
//         Latin+Greek+Cyrillic coverage). Each renders in its own `display:inline-block` span with
//         NO ASCII filler (same "isolate the target glyph" shape fonteng_extended_sanity.cpp uses),
//         so a blank span pinpoints exactly which codepoint failed to lazy-bake. Deliberately NOT
//         CJK here -- Open Sans genuinely lacks CJK, so a CJK span rendering blank would prove
//         nothing about the lazy path (see [4] below, which uses CJK for the CORRECT reason: the
//         negative-cache path).
//     [2] GROWTH WITHOUT CORRUPTION. `greek_alpha_up` (Α) and `cyr_a_up` (А) double as CANARIES:
//         their get_element_box().w and ink count are captured after frame 1 (their own first-time
//         lazy bake, BEFORE any GrowAtlas() call can possibly have happened -- nothing has grown
//         the atlas yet at that point), then `grower` (empty in the static .rml) is populated at
//         runtime via UiLayer::set_text() with ~114 further distinct Greek+Cyrillic codepoints
//         engineered (see kGrowerCodepoints's own comment below) to GUARANTEE at least one
//         GrowAtlas() call on the next render. Frame 2 re-measures the SAME two canaries: if
//         GrowAtlas()'s row-by-row copy (or the texture-recreation MakeCallbackTexture() path in
//         GenerateString()) ever mis-copied/mis-addressed an already-baked glyph, the canary's
//         width or ink count would visibly change (or crash) between frame 1 and frame 2. Byte-
//         identical width + ink is the pass bar.
//     [3] WIDTH == MESH INVARIANCE for lazy strings. `greek_seq` ("ΑΒΓ", three lazily-baked glyphs
//         in ONE string) is checked two ways: (a) its get_element_box().w (the LAYOUT width, from
//         GetStringWidth()->IterateGlyphs()) is compared against the independently-measured sum of
//         the three single-letter spans' own widths, within the same +-1px rounding-carry tolerance
//         fonteng_own_kern_sanity.cpp derives for its own un-kerned control pair (NOT exact equality
//         -- see that file's header for why); and (b) no ink pixel is found OUTSIDE
//         [0,box.w)x[0,box.h) of `greek_seq`'s own box -- if GenerateString()'s mesh (the actual
//         quads IterateGlyphs() emits at render time) ever advanced by DIFFERENT amounts than what
//         GetStringWidth() measured at layout time, ink would spill past the box edge the layout
//         engine sized around. Both GetStringWidth() and GenerateString() call the SAME
//         EnsureGlyphs()+IterateGlyphs() pair (per BakeGlyph()'s "already tried" cache-hit guard,
//         font_engine_own.cpp:1106), so this checks that invariant behaviourally, not by reading it.
//     [4] NEGATIVE-CACHE. `cjk_missing` (U+4E00, 一) -- Open Sans' cmap does not map this codepoint
//         (glx_sfnt_glyph_id() returns gid 0) -- must render with ZERO ink and must not crash the
//         process (a crash fails the whole test binary, caught structurally). The "does it get
//         re-attempted every render instead of cached" half of this criterion is NOT independently
//         measurable from this file's black-box vantage (no timing/counter hook is exposed across
//         the public API, and this reviewer does not have license to add one -- gate scope is
//         tests/ only): verified INSTEAD by reading EnsureGlyphs() (font_engine_own.cpp:1102-1121),
//         whose very first line inside the loop is
//         `if (inst.glyphs.find(cp) != inst.glyphs.end()) continue;` -- a codepoint negative-cached
//         by BakeGlyph() (`inst.glyphs.emplace(cp, GlyphInfo{})`, gid==0 branch) IS present in
//         inst.glyphs (with .baked==false), so this exact line short-circuits BakeGlyph() on every
//         subsequent EnsureGlyphs() call for that codepoint -- confirmed by code reading, not
//         execution; documented here per the review brief's own allowance for that fallback. This
//         test DOES exercise the mechanical half: `cjk_missing` renders across the SAME 2 frames as
//         every other span (no crash across repeated EnsureGlyphs() calls on an already-negative-
//         cached codepoint).
//     [5] REGRESSION. Not this file's job directly -- gated by re-running the pre-existing 5
//         fonteng_* ctest targets (fonteng_own_sanity, fonteng_own_embed_sanity,
//         fonteng_own_kern_sanity, fonteng_ab_compare, fonteng_extended_sanity) unmodified,
//         alongside this new one, in the same `ctest -R fonteng` invocation -- see the review
//         report for the captured pass/fail of that run.
//
//     METHOD (shared with fonteng_extended_sanity.cpp/fonteng_own_kern_sanity.cpp): one hidden GLFW
//     host window provides the GL context; UiLayer attaches embed-mode (proves the lazy path on
//     GusWorld's own consumption shape, no glintfx::App involved); two warm-up update+render cycles
//     before EVERY measurement (suite convention: cycle 1 settles layout, cycle 2 is stable);
//     glReadPixels + flip_to_top_down to agree with get_element_box()'s top-left/y-down convention;
//     count_ink_rect() counts near-white pixels in a box. Deterministic under llvmpipe (same basis
//     every other fonteng_* ink/width oracle in this suite already relies on) -- no sleeps, no
//     retries, no tolerance beyond the two explicitly-derived +-1px budgets above.
// PT: fonteng_lazy_sanity -- GATE ADVERSARIAL da sub-fase 2B (L1.20-FONTFLIP/FT-F4) para o
//     empacotamento preguiçoso sob demanda (FontEngineOwn::EnsureGlyphs()/BakeGlyph()/GrowAtlas()/
//     RasterizeGlyph(), src/font_engine_own.cpp). Escrito pelo revisor de QA INDEPENDENTE -- NÃO o
//     implementador da sub-fase 2B (commit 7f119f1) -- como gate próprio, não carimbo das alegações
//     do implementador. Executa o pipeline real GL/RmlUi sob Xvfb (oráculo estatístico de tinta +
//     oráculo numérico de largura, as duas técnicas que esta suíte já estabeleceu em
//     fonteng_extended_sanity.cpp / fonteng_own_kern_sanity.cpp), nunca apenas lê o fonte e afirma
//     por inspeção.
//
//     O QUE A SUB-FASE 2B ALEGA (conforme os próprios doc-comments dela): o conjunto ávido
//     (BuildBakeSet(), ~199 codepoints: ASCII imprimível + Latin-1 Supplement U+00A0..U+00FF +
//     pontuação tipográfica até U+2026) agora é completado ADITIVAMENTE -- todo codepoint faltando
//     em inst.glyphs quando uma string é medida/renderizada é empacotado na hora (EnsureGlyphs(),
//     chamado tanto por GetStringWidth() quanto por GenerateString() antes de qualquer um tocar o
//     mapa de glyph), o atlas cresce via dobra amortizada (GrowAtlas()) sem mover nenhum glyph já
//     empacotado, e um codepoint que a face de fato não tem (gid==0) é cache-negativado pra nunca
//     ser retentado.
//
//     OS SEIS CRITÉRIOS QUE ESTE ARQUIVO GATEIA (ver main() pro mapeamento cheque-a-cheque aos
//     critérios 1-5 do review; o critério 6, a rodada de sanitizer, NÃO está neste .cpp -- é ESTE
//     MESMO binário rodado uma segunda vez a partir de uma árvore de build `-DGLINTFX_SANITIZE=ON`,
//     ver o relatório de review desta tarefa):
//
//     [1] NÃO-TOFU FORA DO LATIN-1. Quatro codepoints nomeados explicitamente pela tarefa de review
//         -- Grego U+0391 (Α, LETRA MAIÚSCULA GREGA ALFA) e U+03B1 (α, LETRA MINÚSCULA GREGA ALFA),
//         Cirílico U+0410 (А, LETRA MAIÚSCULA CIRÍLICA A) e U+0434 (д, LETRA MINÚSCULA CIRÍLICA DE)
//         -- todos ficam bem acima do teto do conjunto ávido (U+2026) e todos estão presentes na
//         Open Sans (cobertura comum de Latin+Grego+Cirílico). Cada um renderiza no próprio span
//         `display:inline-block` SEM enchimento ASCII (mesmo formato "isole o glyph alvo" de
//         fonteng_extended_sanity.cpp), então um span em branco aponta exatamente qual codepoint
//         falhou o empacotamento preguiçoso. De propósito NÃO CJK aqui -- a Open Sans de fato não
//         tem CJK, então um span CJK renderizando em branco não provaria nada sobre o caminho
//         preguiçoso (ver [4] abaixo, que usa CJK pelo motivo CORRETO: o caminho de cache negativo).
//     [2] CRESCIMENTO SEM CORRUPÇÃO. `greek_alpha_up` (Α) e `cyr_a_up` (А) fazem dupla função de
//         CANÁRIOS: seu get_element_box().w e contagem de tinta são capturados após o frame 1 (o
//         próprio empacotamento preguiçoso de primeira vez deles, ANTES de qualquer GrowAtlas()
//         poder ter acontecido -- nada cresceu o atlas ainda nesse ponto), então `grower` (vazio no
//         .rml estático) é populado em runtime via UiLayer::set_text() com mais ~114 codepoints
//         distintos de Grego+Cirílico projetados (ver o comentário próprio de kGrowerCodepoints
//         abaixo) pra GARANTIR pelo menos uma chamada de GrowAtlas() no próximo render. O frame 2
//         remede os DOIS mesmos canários: se a cópia linha-a-linha do GrowAtlas() (ou o caminho de
//         recriação de textura MakeCallbackTexture() em GenerateString()) alguma vez copiou/
//         endereçou errado um glyph já empacotado, a largura ou contagem de tinta do canário mudaria
//         visivelmente (ou crasharia) entre o frame 1 e o frame 2. Largura + tinta byte-idênticas é
//         a barra de aprovação.
//     [3] INVARIÂNCIA LARGURA == MESH pra strings preguiçosas. `greek_seq` ("ΑΒΓ", três glyphs
//         empacotados preguiçosamente numa ÚNICA string) é checado de duas formas: (a) seu
//         get_element_box().w (a largura de LAYOUT, de GetStringWidth()->IterateGlyphs()) é
//         comparado contra a soma medida independentemente dos três spans de letra única, dentro da
//         mesma tolerância de +-1px de carregamento de arredondamento que fonteng_own_kern_sanity.cpp
//         deriva pro próprio par de controle sem kern (NÃO igualdade exata -- ver o cabeçalho
//         daquele arquivo pro motivo); e (b) nenhum pixel de tinta é encontrado FORA de
//         [0,box.w)x[0,box.h) da própria box de `greek_seq` -- se o mesh do GenerateString() (os
//         quads reais que IterateGlyphs() emite em render-time) algum dia avançasse por quantidades
//         DIFERENTES do que GetStringWidth() mediu em layout-time, a tinta vazaria pra fora da borda
//         da box que o motor de layout dimensionou. Tanto GetStringWidth() quanto GenerateString()
//         chamam o MESMO par EnsureGlyphs()+IterateGlyphs() (pelo guard de acerto-de-cache "já
//         tentado" do BakeGlyph(), font_engine_own.cpp:1106), então isto checa aquele invariante
//         comportamentalmente, não lendo-o.
//     [4] CACHE NEGATIVO. `cjk_missing` (U+4E00, 一) -- o cmap da Open Sans não mapeia este
//         codepoint (glx_sfnt_glyph_id() retorna gid 0) -- precisa renderizar com ZERO tinta e não
//         pode crashar o processo (um crash reprova o binário de teste inteiro, capturado
//         estruturalmente). A metade "é retentado a cada render em vez de cacheado" deste critério
//         NÃO é independentemente mensurável do ponto-de-vista caixa-preta deste arquivo (nenhum
//         hook de timing/contador é exposto pela API pública, e este revisor não tem licença pra
//         adicionar um -- escopo do gate é só tests/): verificado EM VEZ DISSO lendo EnsureGlyphs()
//         (font_engine_own.cpp:1102-1121), cuja primeiríssima linha dentro do laço é
//         `if (inst.glyphs.find(cp) != inst.glyphs.end()) continue;` -- um codepoint
//         cache-negativado pelo BakeGlyph() (`inst.glyphs.emplace(cp, GlyphInfo{})`, ramo gid==0)
//         ESTÁ presente em inst.glyphs (com .baked==false), então esta linha exata corta o caminho
//         do BakeGlyph() em toda chamada seguinte de EnsureGlyphs() pra aquele codepoint --
//         confirmado por leitura de código, não execução; documentado aqui conforme a própria
//         permissão da tarefa de review pra este fallback. Este teste EXERCITA a metade mecânica:
//         `cjk_missing` renderiza pelos MESMOS 2 frames que todo outro span (sem crash através de
//         chamadas repetidas de EnsureGlyphs() num codepoint já cache-negativado).
//     [5] REGRESSÃO. Não é trabalho direto deste arquivo -- gateado re-rodando os 5 alvos ctest
//         fonteng_* pré-existentes (fonteng_own_sanity, fonteng_own_embed_sanity,
//         fonteng_own_kern_sanity, fonteng_ab_compare, fonteng_extended_sanity) sem modificação,
//         junto com este novo, na mesma invocação `ctest -R fonteng` -- ver o relatório de review
//         pro pass/fail capturado daquela rodada.
//
//     MÉTODO (compartilhado com fonteng_extended_sanity.cpp/fonteng_own_kern_sanity.cpp): uma única
//     janela host GLFW oculta provê o contexto GL; UiLayer anexa em modo embed (prova o caminho
//     preguiçoso na própria forma de consumo do GusWorld, sem glintfx::App envolvido); dois ciclos
//     update+render de aquecimento antes de TODA medição (convenção da suíte: ciclo 1 estabiliza
//     layout, ciclo 2 é estável); glReadPixels + flip_to_top_down pra concordar com a convenção
//     top-left/y-para-baixo do get_element_box(); count_ink_rect() conta pixels quase-brancos numa
//     caixa. Determinístico sob llvmpipe (mesma base que todo outro oráculo de tinta/largura
//     fonteng_* desta suíte já usa) -- sem sleeps, sem retries, sem tolerância além dos dois
//     orçamentos de +-1px explicitamente derivados acima.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// EN: A/B engine-selection hook, forward-declared locally (see fonteng_ab_compare.cpp for the full
//     rationale of why the header cannot be #include-d from a test target). Forced false below so
//     this test unambiguously measures the OWN engine's lazy-bake path.
// PT: Hook de seleção de motor A/B, forward-declarado localmente (ver fonteng_ab_compare.cpp pro
//     racional completo de por que o header não pode ser #include-ado de um alvo de teste). Forçado
//     false abaixo para este teste medir inequivocamente o caminho de empacotamento preguiçoso do
//     motor PRÓPRIO.
namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

constexpr int W = 640, H = 900;

// EN: Encodes a single Unicode codepoint (BMP range, up to 3-byte UTF-8 -- everything this file
//     uses, U+0391..U+04FF Greek/Cyrillic and U+4E00 CJK, fits) and appends it to `out`. No need for
//     4-byte (surrogate-pair / astral plane) support -- this test never uses one.
// PT: Codifica um único codepoint Unicode (faixa BMP, até UTF-8 de 3 bytes -- tudo que este arquivo
//     usa, Grego/Cirílico U+0391..U+04FF e CJK U+4E00, cabe) e anexa a `out`. Sem necessidade de
//     suporte a 4 bytes (par substituto / plano astral) -- este teste nunca usa um.
void append_utf8(std::string& out, uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

// EN: (Criterion [2] growth trigger) ~114 distinct codepoints -- the FULL Greek alphabet
//     (U+0391..U+03A9 upper, U+03B1..U+03C9 lower -- a few slots in that range are unassigned in
//     Unicode, e.g. U+03A2; harmless here, RasterizeGlyph()/glx_sfnt_glyph_id() just gid==0/negative
//     -caches those like `cjk_missing`, contributing nothing to the atlas but nothing broken either)
//     plus the full base Cyrillic alphabet (U+0410..U+042F upper, U+0430..U+044F lower). NONE of
//     `greek_alpha_up`/`cyr_a_up` (the two canaries) are excluded from this range -- they may repeat
//     here, which is harmless: EnsureGlyphs()'s "already tried" guard (font_engine_own.cpp:1106)
//     skips a codepoint already in inst.glyphs, so re-mentioning Α/А does not re-bake them or move
//     them (would defeat the canary check if it did).
//
//     WHY THIS COUNT IS A GUARANTEE, NOT A HOPE (read from BakeGlyph()/GrowAtlas(),
//     font_engine_own.cpp:989-1083, not merely asserted): BakeFaceInstance()'s own warm-set packing
//     leaves inst.atlas_h EXACTLY equal to inst.pen_y+inst.shelf_h (zero slack -- font_engine_own.cpp
//     "const int atlas_h = std::max(1, pen_y + shelf_h);"), and BakeGlyph() continues packing from
//     that SAME persistent (pen_x, pen_y, shelf_h) cursor. The first lazily-baked glyph whose width
//     doesn't fit the current shelf (`pen_x + gw > atlas_w`, atlas_w fixed at the warm-set's 512px)
//     wraps to a new row: `pen_y_new = pen_y_old + shelf_h_old`, which by the zero-slack identity
//     above is EXACTLY `inst.atlas_h`. The very next check, `pen_y_new + gh > inst.atlas_h`, is then
//     true for ANY gh > 0 -- so the FIRST shelf-row wrap among the new glyphs deterministically
//     triggers GrowAtlas(). At 48px (this scene's font-size) Open Sans' Greek/Cyrillic capitals run
//     roughly 25-40px wide; ~114 of them cannot possibly all fit on the single left-over shelf
//     row-width budget (512px, minus whatever `grower`'s bake order leaves as leftover pen_x) without
//     at least one wrap. This is a geometric certainty for this codepoint count at this font-size,
//     not a probabilistic assumption -- see the file header's [2] for the corruption check this sets
//     up.
// PT: (gatilho de crescimento do critério [2]) ~114 codepoints distintos -- o alfabeto grego
//     COMPLETO (U+0391..U+03A9 maiúsculo, U+03B1..U+03C9 minúsculo -- alguns slots dessa faixa não
//     são atribuídos no Unicode, ex. U+03A2; inofensivo aqui, RasterizeGlyph()/glx_sfnt_glyph_id()
//     simplesmente cache-negativam esses como `cjk_missing`, sem contribuir nada ao atlas mas também
//     sem quebrar nada) mais o alfabeto cirílico base completo (U+0410..U+042F maiúsculo,
//     U+0430..U+044F minúsculo). NENHUM de `greek_alpha_up`/`cyr_a_up` (os dois canários) é excluído
//     desta faixa -- podem repetir aqui, o que é inofensivo: o guard "já tentado" do EnsureGlyphs()
//     (font_engine_own.cpp:1106) pula um codepoint já em inst.glyphs, então re-mencionar Α/А não os
//     reempacota nem os move (isso derrubaria o cheque de canário se acontecesse).
//
//     POR QUE ESTA CONTAGEM É UMA GARANTIA, NÃO UMA ESPERANÇA (lido de BakeGlyph()/GrowAtlas(),
//     font_engine_own.cpp:989-1083, não meramente afirmado): o próprio empacotamento do warm set do
//     BakeFaceInstance() deixa inst.atlas_h EXATAMENTE igual a inst.pen_y+inst.shelf_h (folga zero --
//     font_engine_own.cpp "const int atlas_h = std::max(1, pen_y + shelf_h);"), e o BakeGlyph()
//     continua empacotando a partir DESSE MESMO cursor persistente (pen_x, pen_y, shelf_h). O
//     primeiro glyph empacotado preguiçosamente cuja largura não cabe na prateleira atual
//     (`pen_x + gw > atlas_w`, atlas_w fixo nos 512px do warm set) vira linha nova:
//     `pen_y_new = pen_y_old + shelf_h_old`, que pela identidade de folga-zero acima é EXATAMENTE
//     `inst.atlas_h`. O próximo cheque, `pen_y_new + gh > inst.atlas_h`, é então verdadeiro pra
//     QUALQUER gh > 0 -- então o PRIMEIRO wrap de linha-de-prateleira entre os glyphs novos dispara
//     deterministicamente o GrowAtlas(). A 48px (font-size desta cena) as maiúsculas grego/cirílico
//     da Open Sans rodam aproximadamente 25-40px de largura; ~114 delas não podem de jeito nenhum
//     caber todas no orçamento de largura da única prateleira remanescente (512px, menos o que quer
//     que a ordem de empacotamento do `grower` deixe como pen_x sobrando) sem pelo menos um wrap.
//     Isto é uma certeza geométrica pra esta contagem de codepoint neste font-size, não uma suposição
//     probabilística -- ver o [2] do cabeçalho do arquivo pro cheque de corrupção que isto monta.
std::string build_grower_text() {
  std::string s;
  for (uint32_t cp = 0x0391; cp <= 0x03A9; ++cp) append_utf8(s, cp); // Greek upper.
  for (uint32_t cp = 0x03B1; cp <= 0x03C9; ++cp) append_utf8(s, cp); // Greek lower.
  for (uint32_t cp = 0x0410; cp <= 0x042F; ++cp) append_utf8(s, cp); // Cyrillic upper.
  for (uint32_t cp = 0x0430; cp <= 0x044F; ++cp) append_utf8(s, cp); // Cyrillic lower.
  return s;
}

// EN: glReadPixels' origin is bottom-left; get_element_box() is top-left, y-down -- flip in place
//     (identical to fonteng_extended_sanity.cpp/fonteng_ab_compare.cpp).
// PT: A origem do glReadPixels é bottom-left; get_element_box() é top-left, y-para-baixo -- inverte
//     in-place (idêntico a fonteng_extended_sanity.cpp/fonteng_ab_compare.cpp).
void flip_to_top_down(std::vector<unsigned char>& px, int w, int h) {
  const size_t row_bytes = static_cast<size_t>(w) * 3;
  std::vector<unsigned char> tmp(row_bytes);
  for (int top = 0, bottom = h - 1; top < bottom; ++top, --bottom) {
    unsigned char* rt = px.data() + static_cast<size_t>(top) * row_bytes;
    unsigned char* rb = px.data() + static_cast<size_t>(bottom) * row_bytes;
    std::copy(rt, rt + row_bytes, tmp.data());
    std::copy(rb, rb + row_bytes, rt);
    std::copy(tmp.data(), tmp.data() + row_bytes, rb);
  }
}

// EN: Counts "ink" pixels (any channel > threshold) inside [x,x+w) x [y,y+h), clamped to the frame
//     -- px is TOP-DOWN RGB (flip already applied).
// PT: Conta pixels "tinta" (algum canal > threshold) em [x,x+w) x [y,y+h), recortado ao frame -- px
//     é RGB TOP-DOWN (flip já aplicado).
long count_ink_rect(const std::vector<unsigned char>& px, int fw, int fh,
                    float rx, float ry, float rw, float rh, int threshold = 100) {
  int x0 = std::max(0, static_cast<int>(std::floor(rx)));
  int y0 = std::max(0, static_cast<int>(std::floor(ry)));
  int x1 = std::min(fw, static_cast<int>(std::ceil(rx + rw)));
  int y1 = std::min(fh, static_cast<int>(std::ceil(ry + rh)));
  long count = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      if (px[i] > threshold || px[i + 1] > threshold || px[i + 2] > threshold) ++count;
    }
  }
  return count;
}

// EN: (Criterion [3]b, mesh-overflow check) Counts ink in a MARGIN ring around [x,x+w)x[y,y+h] but
//     OUTSIDE it -- specifically the `margin`-px band directly to the right of the box (where a
//     too-wide mesh advance would spill) and directly below (where a too-tall glyph offset would
//     spill). Left/above are not checked: `greek_seq`'s own left edge is also its line's left edge
//     (no preceding inline content on that line in this scene), so there is nothing upstream that
//     could spill leftward, and glyphs never render above their own y0 in this engine (offset_y is
//     always >= 0 from the pen baseline per RasterizeGlyph()'s own bbox derivation).
// PT: (cheque de transbordo-de-mesh do critério [3]b) Conta tinta num anel de MARGEM ao redor de
//     [x,x+w)x[y,y+h] mas FORA dele -- especificamente a faixa de `margin`-px diretamente à direita
//     da box (onde um avanço de mesh largo demais vazaria) e diretamente abaixo (onde um offset de
//     glyph alto demais vazaria). Esquerda/acima não são checados: a própria borda esquerda de
//     `greek_seq` também é a borda esquerda da própria linha (sem conteúdo inline precedente nessa
//     linha nesta cena), então não há nada a montante que pudesse vazar pra esquerda, e glyphs nunca
//     renderizam acima do próprio y0 neste motor (offset_y é sempre >= 0 a partir da baseline da
//     pena, pela própria derivação de bbox do RasterizeGlyph()).
long count_ink_right_margin(const std::vector<unsigned char>& px, int fw, int fh,
                             float box_x, float box_y, float box_w, float box_h, float margin) {
  return count_ink_rect(px, fw, fh, box_x + box_w, box_y, margin, box_h);
}
long count_ink_bottom_margin(const std::vector<unsigned char>& px, int fw, int fh,
                              float box_x, float box_y, float box_w, float box_h, float margin) {
  return count_ink_rect(px, fw, fh, box_x, box_y + box_h, box_w, margin);
}

} // namespace

int main() {
  // EN: Force the OWN engine (see file header). A create() failure is a FAIL, not a SKIP -- Xvfb
  //     provides a real (software) GL context in CI, the repo-wide embed-test convention.
  // PT: Força o motor PRÓPRIO (ver cabeçalho). Um create() falho é FAIL, não SKIP -- o Xvfb provê
  //     um contexto GL real (por software) em CI, convenção repo-wide dos testes embed.
  glintfx::own_font_engine_ab_bypass() = false;

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_lazy_sanity_host", W, H)) {
    std::puts("fonteng_lazy_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_lazy_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_lazy_scene.rml")) {
    std::puts("fonteng_lazy_sanity FAIL: load() returned false");
    return 3;
  }

  int failures = 0;
  auto capture_frame = [&](std::vector<unsigned char>& px) {
    ui.update(); ui.render();
    ui.update(); ui.render();
    px.assign(static_cast<size_t>(W) * H * 3, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    flip_to_top_down(px, W, H);
  };

  // ===========================================================================================
  // FRAME 1 -- everything in the static .rml lazy-bakes here (`grower` is still empty). This is
  // the "before any GrowAtlas() call" snapshot for the [2] canaries.
  // ===========================================================================================
  std::vector<unsigned char> px1;
  capture_frame(px1);

  struct Box { glintfx::ElementBox box; long ink; };
  auto measure = [&](const std::vector<unsigned char>& px, const char* id) -> Box {
    const glintfx::ElementBox b = ui.get_element_box(id);
    const long ink = b.found ? count_ink_rect(px, W, H, b.x, b.y, b.w, b.h) : 0;
    return { b, ink };
  };

  const Box ctrl1        = measure(px1, "ctrl_ascii");
  const Box greek_a_up1  = measure(px1, "greek_alpha_up");
  const Box greek_b_up1  = measure(px1, "greek_beta_up");
  const Box greek_g_up1  = measure(px1, "greek_gamma_up");
  const Box greek_seq1   = measure(px1, "greek_seq");
  const Box greek_a_lo1  = measure(px1, "greek_alpha_lo");
  const Box cyr_a_up1    = measure(px1, "cyr_a_up");
  const Box cyr_de_lo1   = measure(px1, "cyr_de_lo");
  const Box cjk1         = measure(px1, "cjk_missing");

  std::printf("fonteng_lazy_sanity: frame 1 (pre-growth) -- %dx%d\n", W, H);
  std::printf("  %-16s %6s %8s %6s   %s\n", "id", "found", "w", "ink", "what");
  auto print_box = [&](const char* id, const Box& b, const char* what) {
    std::printf("  %-16s %6s %8.2f %6ld   %s\n", id, b.box.found ? "yes" : "no", b.box.w, b.ink, what);
  };
  print_box("ctrl_ascii",     ctrl1,       "ASCII control 'Axg'");
  print_box("greek_alpha_up", greek_a_up1, "Greek U+0391 (Α) -- canary");
  print_box("greek_beta_up",  greek_b_up1, "Greek U+0392 (Β)");
  print_box("greek_gamma_up", greek_g_up1, "Greek U+0393 (Γ)");
  print_box("greek_seq",      greek_seq1,  "Greek U+0391 U+0392 U+0393 (ΑΒΓ) sequence");
  print_box("greek_alpha_lo", greek_a_lo1, "Greek U+03B1 (α)");
  print_box("cyr_a_up",       cyr_a_up1,   "Cyrillic U+0410 (А) -- canary");
  print_box("cyr_de_lo",      cyr_de_lo1,  "Cyrillic U+0434 (д)");
  print_box("cjk_missing",    cjk1,        "CJK U+4E00 (一) -- Open Sans lacks this glyph");

  constexpr long kMinInk = 15; // same floor fonteng_extended_sanity.cpp derives (thinnest realistic glyph clears it easily).

  // --- Criterion [1]: non-tofu out-of-Latin-1 (+ the ASCII control, pipeline sanity) -----------
  auto check_ink_present = [&](const Box& b, const char* id, const char* what, bool is_control) {
    if (!b.box.found) {
      std::fprintf(stderr, "fonteng_lazy_sanity FAIL [1] span '%s' (%s) not found in layout\n", id, what);
      ++failures;
      return;
    }
    if (b.ink < kMinInk) {
      if (is_control) {
        std::fprintf(stderr,
            "fonteng_lazy_sanity FAIL [1] ASCII CONTROL '%s' rendered no ink (ink=%ld < %ld) -- the "
            "whole render pipeline is broken, NOT merely the lazy-bake path\n", id, b.ink, kMinInk);
      } else {
        std::fprintf(stderr,
            "fonteng_lazy_sanity FAIL [1] '%s' (%s) rendered no ink (ink=%ld < %ld) -- this "
            "out-of-warm-set codepoint did not lazy-bake (EnsureGlyphs()/BakeGlyph() regression)\n",
            id, what, b.ink, kMinInk);
      }
      ++failures;
    }
  };
  check_ink_present(ctrl1,       "ctrl_ascii",     "ASCII control",     true);
  check_ink_present(greek_a_up1, "greek_alpha_up", "Greek U+0391 (Α)",  false);
  check_ink_present(greek_b_up1, "greek_beta_up",  "Greek U+0392 (Β)",  false);
  check_ink_present(greek_g_up1, "greek_gamma_up", "Greek U+0393 (Γ)",  false);
  check_ink_present(greek_a_lo1, "greek_alpha_lo", "Greek U+03B1 (α)",  false);
  check_ink_present(cyr_a_up1,   "cyr_a_up",       "Cyrillic U+0410 (А)", false);
  check_ink_present(cyr_de_lo1,  "cyr_de_lo",      "Cyrillic U+0434 (д)", false);

  // --- Criterion [3]: width == mesh invariance for the lazy multi-glyph string `greek_seq` -----
  if (!greek_seq1.box.found || !greek_a_up1.box.found || !greek_b_up1.box.found || !greek_g_up1.box.found) {
    std::fprintf(stderr, "fonteng_lazy_sanity FAIL [3] greek_seq or a component span not found\n");
    ++failures;
  } else {
    const float sum_w = greek_a_up1.box.w + greek_b_up1.box.w + greek_g_up1.box.w;
    const float gap = std::fabs(sum_w - greek_seq1.box.w);
    std::printf("fonteng_lazy_sanity: [3] width(greek_seq)=%.3f  sum(A+B+G)=%.3f  gap=%.3f (budget <= 1.0)\n",
                greek_seq1.box.w, sum_w, gap);
    if (gap > 1.0f) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [3a] width(greek_seq)=%.3f vs sum(component widths)=%.3f -- "
          "gap=%.3f exceeds the +-1px rounding-carry budget; layout-time width (GetStringWidth) and "
          "single-glyph advances disagree for lazily-baked glyphs\n", greek_seq1.box.w, sum_w, gap);
      ++failures;
    }
    constexpr float kMargin = 6.f; // generous band -- any real mesh/width divergence spills far more than this.
    const long right_spill = count_ink_right_margin(px1, W, H, greek_seq1.box.x, greek_seq1.box.y,
                                                      greek_seq1.box.w, greek_seq1.box.h, kMargin);
    const long bottom_spill = count_ink_bottom_margin(px1, W, H, greek_seq1.box.x, greek_seq1.box.y,
                                                        greek_seq1.box.w, greek_seq1.box.h, kMargin);
    std::printf("fonteng_lazy_sanity: [3b] greek_seq mesh-overflow ink: right=%ld bottom=%ld (budget ==0)\n",
                right_spill, bottom_spill);
    if (right_spill != 0 || bottom_spill != 0) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [3b] greek_seq mesh spills past its own layout box (right=%ld, "
          "bottom=%ld ink px outside [0,w)x[0,h)) -- GenerateString()'s render-time mesh disagrees "
          "with GetStringWidth()'s layout-time measurement for lazily-baked glyphs\n",
          right_spill, bottom_spill);
      ++failures;
    }
  }

  // --- Criterion [4]: negative-cache (CJK the face lacks) renders blank, no crash --------------
  if (!cjk1.box.found) {
    std::fprintf(stderr, "fonteng_lazy_sanity FAIL [4] span 'cjk_missing' not found in layout\n");
    ++failures;
  } else if (cjk1.ink != 0) {
    std::fprintf(stderr,
        "fonteng_lazy_sanity FAIL [4] 'cjk_missing' (U+4E00) rendered ink=%ld -- Open Sans is not "
        "expected to contain this glyph; a non-zero ink count here means either a tofu box is being "
        "drawn for gid==0, or a stale/wrong glyph is aliasing this codepoint's quad\n", cjk1.ink);
    ++failures;
  }
  // (reaching here at all -- no crash across the process's 2 frames of EnsureGlyphs() re-visiting
  //  this already negative-cached codepoint -- is itself part of this criterion; see file header.)

  // ===========================================================================================
  // Populate `grower` -- DOM-write trigger for ~114 further distinct codepoints, engineered (see
  // build_grower_text()'s own comment) to GUARANTEE at least one GrowAtlas() call on frame 2.
  // ===========================================================================================
  const std::string grower_text = build_grower_text();
  if (!ui.set_text("grower", grower_text.c_str())) {
    std::fputs("fonteng_lazy_sanity FAIL: set_text(\"grower\", ...) returned false\n", stderr);
    ++failures;
  }

  // ===========================================================================================
  // FRAME 2 -- `grower`'s ~114 new codepoints lazy-bake here, forcing >=1 GrowAtlas() call (see
  // build_grower_text()'s comment for why this is a geometric certainty, not a hope). Re-measure
  // the SAME two canaries (Α, А) plus `grower` itself.
  // ===========================================================================================
  std::vector<unsigned char> px2;
  capture_frame(px2);

  const Box greek_a_up2 = measure(px2, "greek_alpha_up");
  const Box cyr_a_up2   = measure(px2, "cyr_a_up");
  const Box grower2     = measure(px2, "grower");
  const Box ctrl2        = measure(px2, "ctrl_ascii");

  std::printf("fonteng_lazy_sanity: frame 2 (post-growth) -- %dx%d\n", W, H);
  print_box("greek_alpha_up", greek_a_up2, "Greek U+0391 (Α) -- canary, post-growth");
  print_box("cyr_a_up",       cyr_a_up2,   "Cyrillic U+0410 (А) -- canary, post-growth");
  print_box("grower",         grower2,     "114-codepoint grower string (post-growth)");
  print_box("ctrl_ascii",     ctrl2,       "ASCII control 'Axg' -- post-growth sanity");

  // --- Criterion [2]: growth without corruption -------------------------------------------------
  if (!greek_a_up2.box.found || !cyr_a_up2.box.found) {
    std::fprintf(stderr, "fonteng_lazy_sanity FAIL [2] a canary span not found after growth\n");
    ++failures;
  } else {
    if (greek_a_up2.box.w != greek_a_up1.box.w) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [2] greek_alpha_up (canary) width CHANGED across GrowAtlas(): "
          "before=%.3f after=%.3f -- a glyph baked before growth got its geometry corrupted by the "
          "atlas reallocation\n", greek_a_up1.box.w, greek_a_up2.box.w);
      ++failures;
    }
    if (greek_a_up2.ink != greek_a_up1.ink) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [2] greek_alpha_up (canary) ink CHANGED across GrowAtlas(): "
          "before=%ld after=%ld -- the glyph's own bitmap/UV mapping was corrupted by the atlas "
          "reallocation (GrowAtlas()'s row copy, or the texture recreate in GenerateString())\n",
          greek_a_up1.ink, greek_a_up2.ink);
      ++failures;
    }
    if (cyr_a_up2.box.w != cyr_a_up1.box.w) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [2] cyr_a_up (canary) width CHANGED across GrowAtlas(): "
          "before=%.3f after=%.3f\n", cyr_a_up1.box.w, cyr_a_up2.box.w);
      ++failures;
    }
    if (cyr_a_up2.ink != cyr_a_up1.ink) {
      std::fprintf(stderr,
          "fonteng_lazy_sanity FAIL [2] cyr_a_up (canary) ink CHANGED across GrowAtlas(): "
          "before=%ld after=%ld\n", cyr_a_up1.ink, cyr_a_up2.ink);
      ++failures;
    }
  }
  // Sanity: the grower string itself must actually have rendered SOMETHING (else the whole
  // growth-trigger premise -- "114 new glyphs got baked" -- never happened, and criterion [2]
  // above would be vacuously passing rather than meaningfully passing).
  if (!grower2.box.found) {
    std::fprintf(stderr, "fonteng_lazy_sanity FAIL [2] 'grower' span not found after set_text()\n");
    ++failures;
  } else if (grower2.ink < kMinInk * 10) { // 114 glyphs must clear a much higher floor than a single glyph.
    std::fprintf(stderr,
        "fonteng_lazy_sanity FAIL [2] 'grower' rendered implausibly little ink (%ld) for a "
        "114-codepoint string -- the growth-trigger premise itself did not fire (set_text() content "
        "not actually lazy-baked/rendered), so the canary comparison above is not meaningful\n",
        grower2.ink);
    ++failures;
  }
  if (!ctrl2.box.found || ctrl2.ink < kMinInk) {
    std::fprintf(stderr,
        "fonteng_lazy_sanity FAIL [2] ASCII control went blank/missing AFTER growth (ink=%ld) -- "
        "the render pipeline broke across the atlas reallocation, not merely a lazy glyph\n",
        ctrl2.ink);
    ++failures;
  }

  // --- Embed-mode hygiene (repo-wide convention: ok() stays true after the full sequence) ------
  if (!ui.ok()) {
    std::fputs("fonteng_lazy_sanity FAIL: ok() false after render sequence\n", stderr);
    ++failures;
  }

  // Restore the default (hygiene -- no shared process today, but consistent with fonteng_ab_compare
  // / fonteng_extended_sanity).
  glintfx::own_font_engine_ab_bypass() = false;

  if (failures == 0) {
    std::puts("fonteng_lazy_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_lazy_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
