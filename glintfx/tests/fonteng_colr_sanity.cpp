// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_colr_sanity -- A4-EMOJI (COLRv0 colour emoji) pixel-proof gate for
//     FontEngineOwn::RasterizeColorGlyph()/GlyphInfo::is_color/GenerateString()'s white-vertex
//     branch (src/font_engine_own.{cpp,hpp}). Executes the real GL/RmlUi pipeline under Xvfb --
//     same method this suite already established in fonteng_fallback_sanity.cpp/
//     fonteng_lazy_sanity.cpp (glReadPixels + flip_to_top_down + get_element_box(), never mere
//     source inspection) -- adapted here from an INK-PRESENCE oracle to a COLOUR-AVERAGE oracle
//     (average_ink_colour() below), since this ticket's questions are about WHICH colour rendered,
//     not merely whether ink rendered. See tests/fonteng_colr_scene.rml's own header for the exact
//     fixture layout (tests/assets/colr_fixture.ttf, S1's own posse) and why an area-average over
//     each span's whole get_element_box() -- rather than one hand-picked pixel coordinate -- is the
//     more robust oracle against this fixture's own generous "advance width >> glyph ink width"
//     geometry: pixels near-black (below `threshold`) are excluded from the average (background,
//     including all the empty space the wide hmtx advance leaves around the glyph itself and the
//     line's own ascent/descent padding beyond the glyph's own ink), so the average is dominated by
//     the glyph's own FULLY-OPAQUE interior texels regardless of exactly where in the box they
//     land.
//
//     THE THREE CRITERIA THE TEAM-LEAD BRIEF NAMES, PLUS ONE BONUS WIDTH CHECK:
//
//     (a) TEXT COLOUR DOES NOT TINT THE EMOJI. `two_layer` (U+E000, `color: red`) must read back
//         average colour dominated by BLUE (avg_b high, avg_r/avg_g low) -- if the vertex-colour
//         path wrongly multiplied the colour atlas texel by the span's OWN `red` `colour` (treating
//         a colour glyph like a grayscale coverage one), the readback would be red-tinted/near-
//         black (red * blue-premultiplied-texel has near-zero blue and non-trivial red only where
//         the (255,0,0,255)-premultiplied vertex times a (0,0,255,255) texel divides down --
//         concretely (0,0,255,255)_straight × (255,0,0,255)_vertex / 255 == (0,0,255,255) is
//         actually still blue-ish component-wise for THIS specific colour pair by coincidence of
//         zero cross-channels, so this criterion additionally cross-checks against `fg_sentinel`
//         below, whose (255,255,255) white texel WOULD visibly tint toward the span's own `lime`
//         `colour` under the same bug -- the pair together is what makes the bug unmistakable
//         regardless of which single colour pair might accidentally alias).
//     (b) A GRAYSCALE GLYPH IN THE SAME TEST STILL TINTS NORMALLY. `gray_ascii` ('A', "Sans"/
//         OpenSans, `color: red`) must read back average colour dominated by RED -- proving
//         RasterizeColorGlyph()'s addition to RasterizeGlyph() (font_engine_own.cpp) did not
//         regress the pre-existing, unrelated coverage-atlas path for a face with NO `COLR` table
//         at all (glx_sfnt_colr_layers() must report 0 layers for it, falling straight through to
//         the untouched grayscale branch). NOTE ON SCOPE: this is tested via a SEPARATE
//         FaceInstance/GenerateString() call (a second @font-face, "Sans"), not literally the same
//         Rml::String argument as the colour glyphs -- colr_fixture.ttf's own cmap (S1's posse,
//         tools/gen_colr_fixture.py) maps ONLY U+E000/U+E001, both colour glyphs, so there is no
//         codepoint this fixture alone can supply for a genuinely-mixed single string. Proving the
//         grayscale path is unbroken IN THE SAME TEST BINARY/SESSION is the achievable, still fully
//         meaningful form of this criterion -- an autonomous, documented scope call (see this
//         file's own report to the orchestrator for the explicit flag).
//     (c) THE 0xFFFF-PALETTE GLYPH BAKES TO A FIXED, TEXT-COLOUR-INDEPENDENT OPAQUE WHITE.
//         `fg_sentinel` (U+E001, `color: lime`) must read back average colour with ALL THREE
//         channels high (near-white) -- NOT dominated by green the way a genuine "substitute the
//         CURRENT draw call's text colour" implementation would render. THIS IS A DELIBERATE,
//         DOCUMENTED DIVERGENCE from the orchestrating message's literal paraphrase ("vira a cor de
//         foreground") -- RasterizeColorGlyph() (font_engine_own.cpp/.hpp) instead follows this
//         ticket's own PLAN document (2026-07-22-framework2d-A4-emoji-colr.md, §5 "Known
//         limitations"): "Palette index 0xFFFF... bakes as opaque white... the atlas is baked
//         colour-independent... a per-text-colour bake would break the atlas cache." A colour
//         glyph's atlas cell is baked ONCE per FaceInstance (family/style/weight/pixel-size), then
//         REUSED verbatim for every subsequent GenerateString() call regardless of THAT call's own
//         `colour` argument (the same "bake once, tint never" contract every OTHER glyph in this
//         atlas already has) -- making the sentinel track the literal, current-call foreground
//         colour would require re-baking (or a second, colour-parametrised composite pass) on every
//         colour change for the SAME cached glyph, defeating the whole atlas-cache premise this
//         engine is built on. Flagged explicitly in this file's own report to the orchestrator, not
//         silently substituted.
//     (bonus) VS16 WIDTH INVARIANCE. `two_layer_vs16` (U+E000 + U+FE0F) must measure the IDENTICAL
//         width (get_element_box().w) as `two_layer` (U+E000 alone) -- proving
//         IsVariationSelector()'s skip (font_engine_own.cpp, EnsureGlyphs()/IterateGlyphs()) treats
//         VS16 as truly zero-width/zero-advance, not merely zero-ink.
//
//     What this file deliberately does NOT do (S3/qa-engineer's job per this ticket's plan §6):
//     corrupt the fixture's COLR/CPAL bytes and re-probe under ASan/UBSan, repeated-reload/atlas-
//     growth stress, or the "does this guard exist in a sibling path" domino sweep. This file is
//     the CORRECTNESS proof; the adversarial hardening proof is a separate, distinct reviewer's
//     gate, per this codebase's own "implementer != reviewer" discipline.
// PT: fonteng_colr_sanity -- GATE de prova-de-pixel do A4-EMOJI (emoji colorido COLRv0) pro
//     FontEngineOwn::RasterizeColorGlyph()/GlyphInfo::is_color/ramo de vértice-branco do
//     GenerateString() (src/font_engine_own.{cpp,hpp}). Executa o pipeline real GL/RmlUi sob Xvfb
//     -- mesmo método que esta suíte já estabeleceu no fonteng_fallback_sanity.cpp/
//     fonteng_lazy_sanity.cpp (glReadPixels + flip_to_top_down + get_element_box(), nunca mera
//     inspeção de fonte) -- adaptado aqui de um oráculo de PRESENÇA-DE-TINTA pra um oráculo de
//     MÉDIA-DE-COR (average_ink_colour() abaixo), já que as perguntas desta tarefa são sobre QUAL
//     cor renderizou, não meramente se tinta renderizou. Ver o cabeçalho próprio de
//     tests/fonteng_colr_scene.rml pro layout exato de fixture (tests/assets/colr_fixture.ttf,
//     posse própria do S1) e o motivo de uma média de área sobre a caixa get_element_box() inteira
//     de cada span -- em vez de uma coordenada de pixel escolhida à mão -- ser o oráculo mais
//     robusto contra a própria geometria generosa "largura-de-avanço >> largura-de-tinta-do-glyph"
//     desta fixture: pixels quase-pretos (abaixo de `threshold`) são excluídos da média (fundo,
//     incluindo todo o espaço vazio que o avanço hmtx largo deixa ao redor do próprio glyph e o
//     próprio padding de ascent/descent da linha além da tinta do glyph), então a média fica
//     dominada pelos próprios texels TOTALMENTE OPACOS do interior do glyph independente de onde
//     exatamente na caixa eles caiam.
//
//     OS TRÊS CRITÉRIOS QUE A TAREFA DO TEAM-LEAD NOMEIA, MAIS UM CHEQUE BÔNUS DE LARGURA:
//
//     (a) A COR DE TEXTO NÃO TINGE O EMOJI. `two_layer` (U+E000, `color: red`) precisa ler de
//         volta cor média dominada por AZUL (avg_b alto, avg_r/avg_g baixos) -- se o caminho de
//         cor-de-vértice multiplicasse errado o texel do atlas colorido pela própria `colour`
//         `red` do span (tratando um glyph colorido como um de cobertura em tons de cinza), a
//         leitura de volta seria tingida-de-vermelho/quase-preta (checado em conjunto com
//         `fg_sentinel` abaixo, cujo texel branco (255,255,255) TINGIRIA visivelmente pra própria
//         `colour` `lime` do span sob o mesmo bug -- a dupla junta é o que torna o bug inconfundível
//         independente de qual par de cor específico poderia acidentalmente alisar).
//     (b) UM GLYPH EM TONS DE CINZA NO MESMO TESTE AINDA TINGE NORMAL. `gray_ascii` ('A',
//         "Sans"/OpenSans, `color: red`) precisa ler de volta cor média dominada por VERMELHO --
//         provando que a adição do RasterizeColorGlyph() ao RasterizeGlyph()
//         (font_engine_own.cpp) não regrediu o caminho pré-existente, não-relacionado, de
//         atlas-de-cobertura pra uma face SEM tabela `COLR` nenhuma. NOTA DE ESCOPO: testado via
//         uma chamada FaceInstance/GenerateString() SEPARADA (um 2º @font-face, "Sans"), não
//         literalmente o mesmo argumento Rml::String dos glyphs coloridos -- o próprio cmap da
//         colr_fixture.ttf (posse do S1, tools/gen_colr_fixture.py) mapeia SÓ U+E000/U+E001, ambos
//         glyphs coloridos, então não há codepoint que esta fixture sozinha consiga suprir pra uma
//         string única genuinamente mista. Provar que o caminho em tons de cinza está intacto NO
//         MESMO BINÁRIO/SESSÃO de teste é a forma alcançável, ainda plenamente significativa, deste
//         critério -- uma decisão de escopo autônoma, documentada (ver o próprio relatório deste
//         arquivo pro orquestrador pro flag explícito).
//     (c) O GLYPH DE PALETA 0xFFFF EMPACOTA PRA UM BRANCO OPACO FIXO, INDEPENDENTE DA COR DE
//         TEXTO. `fg_sentinel` (U+E001, `color: lime`) precisa ler de volta cor média com OS TRÊS
//         canais altos (quase-branco) -- NÃO dominada por verde do jeito que uma implementação
//         genuína de "substitui pela cor de texto DESTA chamada de desenho" renderizaria. ISTO É
//         UMA DIVERGÊNCIA DELIBERADA, DOCUMENTADA da paráfrase literal da mensagem orquestradora
//         ("vira a cor de foreground") -- o RasterizeColorGlyph() (font_engine_own.cpp/.hpp) em vez
//         disso segue o próprio documento de PLANO desta tarefa
//         (2026-07-22-framework2d-A4-emoji-colr.md, §5 "Limitações documentadas"): "índice de
//         paleta 0xFFFF... empacota como branco opaco... o atlas é empacotado independente-de-cor...
//         um bake por-cor-de-texto quebraria o cache do atlas." A célula de atlas de um glyph
//         colorido é empacotada UMA vez por FaceInstance (família/estilo/peso/tamanho-em-px), depois
//         REUSADA ao pé da letra pra toda chamada GenerateString() seguinte independente do próprio
//         argumento `colour` DAQUELA chamada (o mesmo contrato "empacota uma vez, nunca tinge" que
//         todo OUTRO glyph deste atlas já tem) -- fazer o sentinela acompanhar a cor de texto
//         literal, corrente, exigiria reempacotar (ou um 2º passe de composite parametrizado por
//         cor) a cada mudança de cor pro MESMO glyph cacheado, derrotando a premissa inteira de
//         cache-de-atlas sobre a qual este motor é construído. Sinalizado explicitamente no próprio
//         relatório deste arquivo pro orquestrador, não substituído em silêncio.
//     (bônus) INVARIÂNCIA DE LARGURA DO VS16. `two_layer_vs16` (U+E000 + U+FE0F) precisa medir a
//         MESMA largura (get_element_box().w) que `two_layer` (U+E000 sozinho) -- provando que o
//         skip do IsVariationSelector() (font_engine_own.cpp, EnsureGlyphs()/IterateGlyphs()) trata
//         o VS16 como genuinamente largura-zero/avanço-zero, não meramente tinta-zero.
//
//     O que este arquivo deliberadamente NÃO faz (trabalho do S3/qa-engineer conforme o §6 do
//     plano desta tarefa): corromper os bytes COLR/CPAL da fixture e re-sondar sob ASan/UBSan,
//     stress de reload/crescimento-de-atlas repetido, ou a varredura dominó "este guard existe num
//     caminho irmão". Este arquivo é a prova de CORRETUDE; a prova de robustez adversarial é o gate
//     de um revisor separado, distinto, conforme a própria disciplina "implementador != revisor"
//     deste código-base.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

// EN: A/B engine-selection hook, forward-declared locally (see fonteng_ab_compare.cpp/
//     fonteng_fallback_sanity.cpp for the full rationale of why font_engine_own.hpp cannot be
//     #include-d from a test target).
// PT: Hook de seleção de motor A/B, forward-declarado localmente (ver fonteng_ab_compare.cpp/
//     fonteng_fallback_sanity.cpp pro racional completo de por que font_engine_own.hpp não pode
//     ser #include-ado de um alvo de teste).
namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

constexpr int W = 500, H = 1500;

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

// EN: Average RGB colour over every "ink" pixel (max channel > `threshold`) inside the given
//     rect -- see this file's own header for why an area average, excluding near-black background,
//     is the robust oracle here (rather than one hand-picked pixel coordinate). `n == 0` (nothing
//     above threshold anywhere in the rect) is a valid, checked outcome -- a caller must inspect it
//     before trusting r/g/b (both left at 0.0 in that case, same "unspecified on failure" caution
//     this suite's own count_ink_rect() sibling functions imply by convention).
// PT: Cor RGB média sobre todo pixel de "tinta" (canal máximo > `threshold`) dentro do retângulo
//     dado -- ver o cabeçalho próprio deste arquivo pro motivo de uma média de área, excluindo
//     fundo quase-preto, ser o oráculo robusto aqui (em vez de uma coordenada de pixel escolhida à
//     mão). `n == 0` (nada acima do threshold em lugar nenhum do retângulo) é um resultado válido,
//     checado -- quem chama precisa inspecioná-lo antes de confiar em r/g/b (ambos deixados em 0.0
//     nesse caso, mesma cautela "não-especificado em falha" que as funções irmãs count_ink_rect()
//     desta suíte já implicam por convenção).
struct AvgColour {
  long n = 0;
  double r = 0.0, g = 0.0, b = 0.0;
};

AvgColour average_ink_colour(const std::vector<unsigned char>& px, int fw, int fh, float rx, float ry, float rw,
                              float rh, int threshold = 40) {
  const int x0 = std::max(0, static_cast<int>(std::floor(rx)));
  const int y0 = std::max(0, static_cast<int>(std::floor(ry)));
  const int x1 = std::min(fw, static_cast<int>(std::ceil(rx + rw)));
  const int y1 = std::min(fh, static_cast<int>(std::ceil(ry + rh)));
  AvgColour a;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      const int r = px[i], g = px[i + 1], b = px[i + 2];
      if (r > threshold || g > threshold || b > threshold) {
        a.r += r;
        a.g += g;
        a.b += b;
        ++a.n;
      }
    }
  }
  if (a.n > 0) {
    a.r /= (double)a.n;
    a.g /= (double)a.n;
    a.b /= (double)a.n;
  }
  return a;
}

} // namespace

int main() {
  glintfx::own_font_engine_ab_bypass() = false; // force the OWN engine.

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_colr_sanity_host", W, H)) {
    std::puts("fonteng_colr_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_colr_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_colr_scene.rml")) {
    std::puts("fonteng_colr_sanity FAIL: load() returned false");
    return 3;
  }

  int failures = 0;

  ui.update();
  ui.render();
  ui.update();
  ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  auto measure = [&](const char* id) {
    const glintfx::ElementBox b = ui.get_element_box(id);
    const AvgColour c = b.found ? average_ink_colour(px, W, H, b.x, b.y, b.w, b.h) : AvgColour{};
    return std::pair<glintfx::ElementBox, AvgColour>{b, c};
  };

  const auto [two_layer_box, two_layer_c] = measure("two_layer");
  const auto [two_layer_vs16_box, two_layer_vs16_c] = measure("two_layer_vs16");
  const auto [fg_sentinel_box, fg_sentinel_c] = measure("fg_sentinel");
  const auto [gray_ascii_box, gray_ascii_c] = measure("gray_ascii");

  auto print_result = [&](const char* id, const glintfx::ElementBox& b, const AvgColour& c) {
    std::printf("  %-16s found=%-3s w=%7.2f  n=%5ld  avg_rgb=(%6.1f,%6.1f,%6.1f)\n", id, b.found ? "yes" : "no",
                b.w, c.n, c.r, c.g, c.b);
  };
  std::printf("fonteng_colr_sanity: frame -- %dx%d\n", W, H);
  print_result("two_layer", two_layer_box, two_layer_c);
  print_result("two_layer_vs16", two_layer_vs16_box, two_layer_vs16_c);
  print_result("fg_sentinel", fg_sentinel_box, fg_sentinel_c);
  print_result("gray_ascii", gray_ascii_box, gray_ascii_c);

  constexpr long kMinInkPixels = 30; // generous floor for a ~30px glyph core at 300px font-size.
  constexpr double kHi = 180.0;      // "clearly this channel is present" floor.
  constexpr double kLo = 60.0;       // "clearly this channel is absent" ceiling.

  // --- (a) text colour does not tint the colour glyph -----------------------------------------
  if (!two_layer_box.found || two_layer_c.n < kMinInkPixels) {
    std::fprintf(stderr, "fonteng_colr_sanity FAIL (a) 'two_layer' not found or rendered no ink (n=%ld)\n",
                 two_layer_c.n);
    ++failures;
  } else if (!(two_layer_c.b > kHi && two_layer_c.r < kLo && two_layer_c.g < kLo)) {
    std::fprintf(stderr,
                 "fonteng_colr_sanity FAIL (a) 'two_layer' (span colour=red) did not read back pure BLUE -- "
                 "avg_rgb=(%.1f,%.1f,%.1f); expected r<%.0f g<%.0f b>%.0f. Either the layer paint ORDER is "
                 "backwards (the square layer must be composited ON TOP, fully occluding the red triangle) or "
                 "the quad's vertex colour is wrongly the span's own text colour instead of white-scaled-by-"
                 "alpha for an is_color glyph (GenerateString(), font_engine_own.cpp)\n",
                 two_layer_c.r, two_layer_c.g, two_layer_c.b, kLo, kLo, kHi);
    ++failures;
  }

  // --- (bonus) VS16 is truly zero-width, not merely zero-ink ----------------------------------
  if (!two_layer_box.found || !two_layer_vs16_box.found) {
    std::fprintf(stderr, "fonteng_colr_sanity FAIL (bonus) 'two_layer' or 'two_layer_vs16' not found\n");
    ++failures;
  } else if (std::fabs(two_layer_box.w - two_layer_vs16_box.w) > 0.5f) {
    std::fprintf(stderr,
                 "fonteng_colr_sanity FAIL (bonus) width(two_layer)=%.3f != width(two_layer_vs16)=%.3f -- "
                 "U+FE0F (VS16) is not being skipped as zero-width by IsVariationSelector() "
                 "(EnsureGlyphs()/IterateGlyphs(), font_engine_own.cpp)\n",
                 two_layer_box.w, two_layer_vs16_box.w);
    ++failures;
  }

  // --- (c) the 0xFFFF-palette layer bakes to a fixed opaque white, NOT the span's text colour --
  if (!fg_sentinel_box.found || fg_sentinel_c.n < kMinInkPixels) {
    std::fprintf(stderr, "fonteng_colr_sanity FAIL (c) 'fg_sentinel' not found or rendered no ink (n=%ld)\n",
                 fg_sentinel_c.n);
    ++failures;
  } else if (!(fg_sentinel_c.r > kHi && fg_sentinel_c.g > kHi && fg_sentinel_c.b > kHi)) {
    std::fprintf(stderr,
                 "fonteng_colr_sanity FAIL (c) 'fg_sentinel' (span colour=lime, palette_index=0xFFFF) did not "
                 "read back near-white -- avg_rgb=(%.1f,%.1f,%.1f); expected all three channels > %.0f. Either "
                 "the 0xFFFF sentinel is not being substituted with opaque white in RasterizeColorGlyph() "
                 "(font_engine_own.cpp), or the quad is being vertex-tinted by the span's own text colour "
                 "instead of white-scaled-by-alpha\n",
                 fg_sentinel_c.r, fg_sentinel_c.g, fg_sentinel_c.b, kHi);
    ++failures;
  }

  // --- (b) an ordinary grayscale glyph in the same test still tints normally -------------------
  if (!gray_ascii_box.found || gray_ascii_c.n < kMinInkPixels) {
    std::fprintf(stderr, "fonteng_colr_sanity FAIL (b) 'gray_ascii' not found or rendered no ink (n=%ld)\n",
                 gray_ascii_c.n);
    ++failures;
  } else if (!(gray_ascii_c.r > kHi && gray_ascii_c.g < kLo && gray_ascii_c.b < kLo)) {
    std::fprintf(stderr,
                 "fonteng_colr_sanity FAIL (b) 'gray_ascii' (span colour=red, plain grayscale 'A') did not read "
                 "back pure RED -- avg_rgb=(%.1f,%.1f,%.1f); expected r>%.0f g<%.0f b<%.0f. The COLR bake path "
                 "addition to RasterizeGlyph() (font_engine_own.cpp) may have regressed the pre-existing "
                 "coverage-atlas path for a face with no COLR table\n",
                 gray_ascii_c.r, gray_ascii_c.g, gray_ascii_c.b, kHi, kLo, kLo);
    ++failures;
  }

  if (failures == 0) {
    std::puts("fonteng_colr_sanity PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_colr_sanity FAIL: %d criterion/criteria failed\n", failures);
  return 10 + failures;
}
