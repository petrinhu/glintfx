// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_own_embed_sanity -- L1.19-FONTENG (FT-F3) embed-mode (UiLayer) visual oracle.
//
//     [Review IMPORTANT #1, 2026-07-10] fonteng_own_sanity.cpp only ever ran inside
//     tests/CMakeLists.txt's if(GLINTFX_BACKEND_GLFW) block, via glintfx::App -- the App-
//     standalone path. GusWorld, the ONLY real consumer of glintfx, attaches via
//     glintfx::UiLayer in an embed-only build (GLINTFX_BACKEND_GLFW=OFF, no glintfx::App
//     compiled at all); before this file NOTHING in the tracked test suite exercised the font
//     engine's glyph-rasterisation pipeline (LoadFontFace -> GetFontFaceHandle ->
//     GenerateString) on that specific consumer path. The reviewer confirmed the pipeline
//     renders correctly via an ad hoc probe during the L1.19-FONTENG review, but an ad hoc
//     probe is not regression protection -- this file is that protection, tracked in CI.
//
//     Renders tests/fonteng_embed_scene.rml (the same 3-line text mix as fonteng_scene.rml:
//     ASCII ascender/descender-heavy, lowercase ASCII, pt-br-accented Latin-1 Supplement)
//     through glintfx::UiLayer attached to a hidden GLFW host window -- the exact GL-context
//     fixture pattern ui_layer_sanity.cpp/data_model_embed_sanity.cpp already use in this
//     suite (see their own doc-comments for the full "why a hidden window" rationale) --
//     compose-only to FBO 0, read back via glReadPixels.
//
//     Oracle: the same 3 checks fonteng_own_sanity.cpp uses (ink exists / ink is shaped, not a
//     filled block / each line's own band has ink), PLUS review IMPORTANT #2's fix applied HERE
//     from the start (not retrofitted): each line's band is UiLayer::get_element_box(id)'s REAL
//     bounding box (not an h/3 approximation), so the check measures ink density inside each
//     line's true glyph region -- including line 3's accented body, not just whatever ink falls
//     in a blind geometric third of the frame.
//
//     Compiled ONLY when GLINTFX_OWN_FONT_ENGINE=ON (tests/CMakeLists.txt) -- unlike
//     fonteng_own_sanity.cpp (deliberately engine-agnostic, always compiled), this file's whole
//     purpose is proving glintfx's own SOV-SFNT/SOV-RAST engine specifically THROUGH THE UiLayer
//     PATH, so gating it behind the A/B option makes sense: a FreeType-through-UiLayer variant
//     is out of this ticket's scope (the FreeType-through-App path already has indirect coverage
//     via data_model_scalar.cpp's OpenSans rendering).
//
// PT: fonteng_own_embed_sanity -- oráculo visual do modo embed (UiLayer) do L1.19-FONTENG
//     (FT-F3).
//
//     [IMPORTANTE #1 do review, 2026-07-10] fonteng_own_sanity.cpp só rodava dentro do bloco
//     if(GLINTFX_BACKEND_GLFW) do tests/CMakeLists.txt, via glintfx::App -- o caminho App
//     standalone. O GusWorld, ÚNICO consumidor real do glintfx, anexa via glintfx::UiLayer num
//     build embed-only (GLINTFX_BACKEND_GLFW=OFF, glintfx::App nem sequer compilado); antes
//     deste arquivo NADA na suíte de testes rastreada exercitava o pipeline de rasterização de
//     glyph do motor de fonte (LoadFontFace -> GetFontFaceHandle -> GenerateString) nesse
//     caminho específico do consumidor. O reviewer confirmou que o pipeline renderiza
//     corretamente via uma sonda ad hoc durante o review do L1.19-FONTENG, mas uma sonda ad hoc
//     não é proteção de regressão -- este arquivo é essa proteção, rastreada em CI.
//
//     Renderiza tests/fonteng_embed_scene.rml (a mesma mistura de 3 linhas de fonteng_scene.rml:
//     ASCII carregado de ascendente/descendente, ASCII minúsculo, Latin-1 Supplement acentuado
//     pt-br) através de glintfx::UiLayer anexado a uma janela host GLFW oculta -- o mesmo padrão
//     de fixture de contexto GL que ui_layer_sanity.cpp/data_model_embed_sanity.cpp já usam
//     nesta suíte (ver os doc-comments deles pro racional completo de "por que janela oculta")
//     -- compose-only pro FBO 0, lido via glReadPixels.
//
//     Oráculo: os mesmos 3 checks que fonteng_own_sanity.cpp usa (tinta existe / tinta tem
//     forma, não é bloco preenchido / cada linha tem tinta na própria banda), MAIS o fix do
//     IMPORTANTE #2 do review aplicado AQUI desde o início (não retrofitado): a banda de cada
//     linha é a bounding box REAL de UiLayer::get_element_box(id) (não uma aproximação h/3),
//     então o check mede densidade de tinta dentro da região real de glyph de cada linha --
//     incluindo o corpo acentuado da linha 3, não só a tinta que cai numa fração geométrica cega
//     do frame.
//
//     Compilado SÓ quando GLINTFX_OWN_FONT_ENGINE=ON (tests/CMakeLists.txt) -- diferente de
//     fonteng_own_sanity.cpp (deliberadamente agnóstico de motor, sempre compilado), o propósito
//     inteiro deste arquivo é provar especificamente o motor próprio SOV-SFNT/SOV-RAST do
//     glintfx ATRAVÉS DO CAMINHO UiLayer, então bloqueá-lo atrás da option A/B faz sentido: uma
//     variante FreeType-via-UiLayer é fora do escopo desta tarefa (o caminho FreeType-via-App já
//     tem cobertura indireta via o rendering de OpenSans do data_model_scalar.cpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                           // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

// EN: Counts "ink" pixels (max channel > threshold) within row range [row_begin, row_end) of a
//     TOP-DOWN (window-space, y=0 at top) RGB buffer -- see flip_to_top_down() below for why
//     the raw glReadPixels buffer must be flipped before this is meaningful.
// PT: Conta pixels de "tinta" (canal máximo > threshold) na faixa de linha [row_begin, row_end)
//     de um buffer RGB TOP-DOWN (espaço-janela, y=0 no topo) -- ver flip_to_top_down() abaixo
//     pra saber por que o buffer cru do glReadPixels precisa ser invertido antes disso fazer
//     sentido.
size_t count_bright_rows(const std::vector<unsigned char>& px, int w, int row_begin, int row_end,
                          int threshold = 100) {
  size_t count = 0;
  for (int y = row_begin; y < row_end; ++y) {
    for (int x = 0; x < w; ++x) {
      const size_t i = (size_t)(y * w + x) * 3;
      const int r = px[i + 0], g = px[i + 1], b = px[i + 2];
      if (r > threshold || g > threshold || b > threshold) ++count;
    }
  }
  return count;
}

// EN: glReadPixels' origin is bottom-left (row 0 = bottom of window); UiLayer::get_element_box()
//     (and every other geometry query in this codebase, per element_box.hpp's own doc-comment)
//     uses top-left, y-down window-space -- the SAME convention App::snapshot() flips into when
//     it writes a PPM (see src/app.cpp's own "glReadPixels origin is bottom-left; PPM origin is
//     top-left" comment, and fonteng_own_sanity.cpp's App-path sibling test, which gets this
//     flip for free via snapshot()'s PPM). This test has no PPM writer, so it flips the readback
//     buffer in-place into that same top-down convention up front, once, so every row index used
//     below -- including the get_element_box()-derived per-line bands -- means the same thing
//     consistently.
// PT: A origem do glReadPixels é bottom-left (linha 0 = base da janela); UiLayer::get_element_box()
//     (e toda outra query de geometria deste codebase, pelo próprio doc-comment de
//     element_box.hpp) usa espaço-janela top-left, y pra baixo -- a MESMA convenção pra qual
//     App::snapshot() inverte ao escrever um PPM (ver o comentário "glReadPixels origin is
//     bottom-left; PPM origin is top-left" do próprio src/app.cpp, e o teste irmão do caminho
//     App, fonteng_own_sanity.cpp, que ganha esse flip de graça via o PPM do snapshot()). Este
//     teste não tem writer de PPM, então inverte o buffer de readback in-place pra essa mesma
//     convenção top-down uma vez, no início, para que todo índice de linha usado abaixo --
//     inclusive as bandas por-linha derivadas de get_element_box() -- signifique a mesma coisa
//     consistentemente.
void flip_to_top_down(std::vector<unsigned char>& px, int w, int h) {
  const size_t row_bytes = (size_t)w * 3;
  std::vector<unsigned char> tmp(row_bytes);
  for (int top = 0, bottom = h - 1; top < bottom; ++top, --bottom) {
    unsigned char* row_top    = px.data() + (size_t)top * row_bytes;
    unsigned char* row_bottom = px.data() + (size_t)bottom * row_bytes;
    std::copy(row_top, row_top + row_bytes, tmp.data());
    std::copy(row_bottom, row_bottom + row_bytes, row_top);
    std::copy(tmp.data(), tmp.data() + row_bytes, row_bottom);
  }
}

// EN: [Review IMPORTANT #2] Row band derived from a REAL element bounding box
//     (UiLayer::get_element_box(id)), clamped to the captured frame's [0, frame_h) bounds --
//     same fix, same rationale as fonteng_own_sanity.cpp's clamp_band() (see that file's
//     doc-comment for the full "h/3 approximation vs. real bounding box" writeup), applied here
//     from the start rather than retrofitted.
// PT: [IMPORTANTE #2 do review] Banda de linha derivada de uma bounding box REAL de elemento
//     (UiLayer::get_element_box(id)), recortada aos limites [0, frame_h) do frame capturado --
//     mesmo fix, mesmo racional do clamp_band() de fonteng_own_sanity.cpp (ver o doc-comment
//     daquele arquivo pro relato completo "aproximação h/3 vs. bounding box real"), aplicado
//     aqui desde o início em vez de retrofitado.
struct Band { int begin; int end; };
Band clamp_band(float y, float box_h, int frame_h) {
  int begin = (int)std::floor(y);
  int end   = (int)std::ceil(y + box_h);
  if (begin < 0) begin = 0;
  if (end > frame_h) end = frame_h;
  if (end < begin) end = begin;
  return {begin, end};
}

} // namespace

int main() {
  // EN: 640x260 -- same dimensions as fonteng_own_sanity.cpp's App window, for an apples-to-
  //     apples comparison between the two paths (App vs. UiLayer) rendering the same text mix.
  // PT: 640x260 -- mesmas dimensões da janela App do fonteng_own_sanity.cpp, pra comparação
  //     maçã-com-maçã entre os dois caminhos (App vs. UiLayer) renderizando a mesma mistura de
  //     texto.
  constexpr int W = 640, H = 260;

  // EN: Host window (GL context provider) -- same fixture shape as every other embed test in
  //     this suite (ui_layer_sanity.cpp/element_box_sanity.cpp/data_model_embed_sanity.cpp):
  //     a failed host.create() is a FAIL, not a SKIP (Xvfb provides a real, if software, GL
  //     context in CI -- unlike fonteng_own_sanity.cpp's App path, which SKIPs on no display
  //     for local-dev convenience outside CI).
  // PT: Janela host (provê o contexto GL) -- mesmo formato de fixture de todo outro teste embed
  //     desta suíte (ui_layer_sanity.cpp/element_box_sanity.cpp/data_model_embed_sanity.cpp):
  //     um host.create() falho é FAIL, não SKIP (Xvfb provê um contexto GL real, ainda que por
  //     software, em CI -- diferente do caminho App de fonteng_own_sanity.cpp, que dá SKIP sem
  //     display por conveniência de dev local fora do CI).
  glintfx::WindowGlfw host;
  if (!host.create("fonteng_own_embed_sanity_host", W, H)) {
    std::puts("fonteng_own_embed_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_own_embed_sanity FAIL: ui attach failed");
    return 2;
  }

  // EN: Deterministic black baseline before load()/render() -- same rationale as
  //     ui_layer_sanity.cpp step 3 (reproducible pixel statistics across runs).
  // PT: Baseline preto determinístico antes de load()/render() -- mesmo racional do passo 3 de
  //     ui_layer_sanity.cpp (estatísticas de pixel reproduzíveis entre execuções).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_embed_scene.rml")) {
    fputs("fonteng_own_embed_sanity FAIL: load() returned false\n", stderr);
    return 3;
  }

  // EN: Two warm-up frames -- same convention as ui_layer_sanity.cpp/fonteng_own_sanity.cpp
  //     (frame 1 settles RmlUi's internal layout/render-layer allocation, frame 2 is the stable
  //     state inspected below).
  // PT: Dois frames de aquecimento -- mesma convenção de ui_layer_sanity.cpp/
  //     fonteng_own_sanity.cpp (frame 1 estabiliza a alocação interna de layout/render-layer do
  //     RmlUi, frame 2 é o estado estável inspecionado abaixo).
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px((size_t)W * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  const size_t total = (size_t)W * H;

  const size_t bright_total = count_bright_rows(px, W, 0, H);
  const double bright_pct = 100.0 * (double)bright_total / (double)total;

  // EN: [Review IMPORTANT #2] Real per-line bands via UiLayer::get_element_box(id) -- see
  //     clamp_band()'s doc-comment above.
  // PT: [IMPORTANTE #2 do review] Bandas reais por-linha via UiLayer::get_element_box(id) --
  //     ver o doc-comment de clamp_band() acima.
  const glintfx::ElementBox box1 = ui.get_element_box("line1");
  const glintfx::ElementBox box2 = ui.get_element_box("line2");
  const glintfx::ElementBox box3 = ui.get_element_box("line3");
  if (!box1.found || !box2.found || !box3.found) {
    std::fprintf(stderr,
        "fonteng_own_embed_sanity FAIL [0] get_element_box found=(%d,%d,%d) -- expected all true "
        "(line1/line2/line3 ids missing from fonteng_embed_scene.rml?)\n",
        box1.found, box2.found, box3.found);
    return 4;
  }

  const Band band1 = clamp_band(box1.y, box1.h, H);
  const Band band2 = clamp_band(box2.y, box2.h, H);
  const Band band3 = clamp_band(box3.y, box3.h, H);

  const size_t bright_line1 = count_bright_rows(px, W, band1.begin, band1.end);
  const size_t bright_line2 = count_bright_rows(px, W, band2.begin, band2.end);
  const size_t bright_line3 = count_bright_rows(px, W, band3.begin, band3.end);

  std::printf("fonteng_own_embed_sanity: %dx%d  bright_total=%zu (%.2f%%)  "
              "line1=[%d,%d)=%zu line2=[%d,%d)=%zu line3=[%d,%d)=%zu\n",
              W, H, bright_total, bright_pct,
              band1.begin, band1.end, bright_line1, band2.begin, band2.end, bright_line2,
              band3.begin, band3.end, bright_line3);

  int failures = 0;

  // EN: Check 1 -- some ink exists (same 0.3% floor as fonteng_own_sanity.cpp).
  // PT: Cheque 1 -- existe alguma tinta (mesmo piso de 0.3% do fonteng_own_sanity.cpp).
  if (bright_pct < 0.3) {
    std::fprintf(stderr,
        "fonteng_own_embed_sanity FAIL [1] bright_pct=%.2f%% < 0.3%% -- no text rendered\n",
        bright_pct);
    ++failures;
  }

  // EN: Check 2 -- ink stays well under a filled-block threshold (same 40% ceiling as
  //     fonteng_own_sanity.cpp).
  // PT: Cheque 2 -- a tinta fica bem abaixo de um limiar de bloco preenchido (mesmo teto de
  //     40% do fonteng_own_sanity.cpp).
  if (bright_pct > 40.0) {
    std::fprintf(stderr,
        "fonteng_own_embed_sanity FAIL [2] bright_pct=%.2f%% > 40%% -- "
        "looks like a filled block, not glyphs\n",
        bright_pct);
    ++failures;
  }

  // EN: Check 3 -- each of the 3 lines' own real bounding-box band has some ink (line 3 =
  //     pt-br accented set, this ticket's own BuildBakeSet() scope).
  // PT: Cheque 3 -- cada uma das 3 linhas tem alguma tinta na própria banda de bounding-box
  //     real (linha 3 = conjunto acentuado pt-br, escopo do próprio BuildBakeSet() desta
  //     tarefa).
  if (bright_line1 == 0) {
    std::fputs(
        "fonteng_own_embed_sanity FAIL [3a] line1 (ASCII ascender/descender) band is fully blank\n",
        stderr);
    ++failures;
  }
  if (bright_line2 == 0) {
    std::fputs("fonteng_own_embed_sanity FAIL [3b] line2 (lowercase ASCII) band is fully blank\n",
               stderr);
    ++failures;
  }
  if (bright_line3 == 0) {
    std::fputs("fonteng_own_embed_sanity FAIL [3c] line3 (pt-br accented) band is fully blank\n",
               stderr);
    ++failures;
  }

  // EN: Check 4 -- ok() stays true after the full render sequence (embed-mode-specific hygiene
  //     check every other UiLayer embed test in this suite also performs).
  // PT: Cheque 4 -- ok() permanece true após a sequência completa de render (checagem de
  //     higiene específica do embed mode que todo outro teste embed de UiLayer desta suíte
  //     também faz).
  if (!ui.ok()) {
    std::fputs("fonteng_own_embed_sanity FAIL [4] ok() false after render sequence\n", stderr);
    ++failures;
  }

  if (failures == 0) {
    std::fputs("fonteng_own_embed_sanity: PASS\n", stdout);
    return 0;
  }
  std::fprintf(stderr, "fonteng_own_embed_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
