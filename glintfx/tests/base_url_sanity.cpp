// SPDX-License-Identifier: MPL-2.0
// EN: base_url sanity test — verifies set_asset_base_url() on UiLayer.
//
//     The test proves that a document whose file only exists inside a subdirectory
//     (not in the process CWD) is correctly loaded when set_asset_base_url() points
//     at that subdirectory.
//
//     Setup (performed by CMakeLists.txt):
//       - embed_scene.rml and embed_scene.rcss are copied to
//         ${CMAKE_CURRENT_BINARY_DIR}/base_url_assets/ (NOT to the CWD directly).
//       - CWD for this test = ${CMAKE_CURRENT_BINARY_DIR} (= build/tests/).
//
//     Test flow:
//       1. Host creates a hidden 900×600 GLFW window.
//       2. UiLayer attaches.
//       3. set_asset_base_url("base_url_assets") — relative base URL, resolved from CWD.
//       4. load("embed_scene.rml") — resolves to base_url_assets/embed_scene.rml.
//          Without the base URL this would fail (file not in CWD).
//       5. Two warm-up frames, capture FBO 0.
//       6. Verify the same three aggregate pixel statistics as ui_layer_sanity:
//            mean_brightness > 5   (rendered something)
//            dark_count    >= 10 % (body background #0d1020 present)
//            bright_count  >= 1 %  (gradient + glow present)
//
// PT: Teste de sanidade do base_url — verifica set_asset_base_url() no UiLayer.
//
//     O teste prova que um documento cujo arquivo existe apenas dentro de um subdiretório
//     (não no CWD do processo) é carregado corretamente quando set_asset_base_url() aponta
//     para esse subdiretório.
//
//     Setup (realizado pelo CMakeLists.txt):
//       - embed_scene.rml e embed_scene.rcss são copiados para
//         ${CMAKE_CURRENT_BINARY_DIR}/base_url_assets/ (NÃO para o CWD diretamente).
//       - CWD deste teste = ${CMAKE_CURRENT_BINARY_DIR} (= build/tests/).
//
//     Fluxo do teste:
//       1. Host cria janela GLFW oculta 900×600.
//       2. UiLayer anexa.
//       3. set_asset_base_url("base_url_assets") — base URL relativo, resolvido a partir do CWD.
//       4. load("embed_scene.rml") — resolve para base_url_assets/embed_scene.rml.
//          Sem o base URL isso falharia (arquivo não está no CWD).
//       5. Dois frames de aquecimento, captura do FBO 0.
//       6. Verifica as mesmas três estatísticas de pixel do ui_layer_sanity:
//            mean_brightness > 5   (renderizou algo)
//            dark_count    >= 10 % (body background #0d1020 presente)
//            bright_count  >= 1 %  (gradiente + glow presentes)
//
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h. PT: inclui GL/gl3w.h.
#include <cstdio>
#include <vector>

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 — host creates a hidden 900×600 window (GL context provider).
  // PT: Passo 1 — host cria janela oculta 900×600 (provê o contexto GL).
  // ---------------------------------------------------------------------------
  constexpr int W = 900, H = 600;
  glintfx::WindowGlfw host;
  if (!host.create("base_url_sanity_host", W, H)) {
    std::puts("base_url_sanity FAIL: host window create failed");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 — UiLayer attaches to the current GL context.
  // PT: Passo 2 — UiLayer anexa ao contexto GL corrente.
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("base_url_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 — set base URL to the subdirectory that holds the scene files.
  //     Without this, load("embed_scene.rml") would fail because the file lives
  //     in base_url_assets/ not in the process CWD (= build/tests/).
  // PT: Passo 3 — define o base URL para o subdiretório que contém os arquivos de cena.
  //     Sem isso, load("embed_scene.rml") falharia pois o arquivo está em base_url_assets/
  //     e não no CWD do processo (= build/tests/).
  // ---------------------------------------------------------------------------
  ui.set_asset_base_url("base_url_assets");

  // ---------------------------------------------------------------------------
  // EN: Step 4 — clear FBO 0 to black, then load the scene via base URL.
  // PT: Passo 4 — limpa FBO 0 para preto, depois carrega a cena via base URL.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.load("embed_scene.rml");

  // ---------------------------------------------------------------------------
  // EN: Step 5 — two warm-up frames, then read back FBO 0 (GL_BACK).
  // PT: Passo 5 — dois frames de aquecimento, depois lê o FBO 0 (GL_BACK).
  // ---------------------------------------------------------------------------
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());

  // ---------------------------------------------------------------------------
  // EN: Step 6 — aggregate pixel statistics (same methodology as ui_layer_sanity).
  // PT: Passo 6 — estatísticas de pixel agregadas (mesma metodologia do ui_layer_sanity).
  // ---------------------------------------------------------------------------
  constexpr size_t total = static_cast<size_t>(W) * H;  // 540 000
  long long brightness_sum = 0;
  size_t dark_count   = 0;  // EN: R+G+B < 120   PT: R+G+B < 120
  size_t bright_count = 0;  // EN: max(R,G,B)>150 PT: max(R,G,B)>150

  for (size_t i = 0; i < total; ++i) {
    const int r = px[i * 3 + 0];
    const int g = px[i * 3 + 1];
    const int b = px[i * 3 + 2];
    brightness_sum += r + g + b;
    if (r + g + b < 120)               ++dark_count;
    if (r > 150 || g > 150 || b > 150) ++bright_count;
  }

  const double mean_brightness = (double)brightness_sum / ((double)total * 3.0);
  const double dark_pct        = 100.0 * (double)dark_count   / (double)total;
  const double bright_pct      = 100.0 * (double)bright_count / (double)total;

  std::printf("base_url_sanity: %dx%d  mean=%.1f  dark=%.1f%%  bright=%.1f%%\n",
              W, H, mean_brightness, dark_pct, bright_pct);

  int failures = 0;

  // EN: Check 1 — not all black: scene loaded and rendered via base URL.
  //     If base URL resolution failed, the document and CSS are absent → all black (mean≈0).
  // PT: Cheque 1 — não tudo preto: cena carregada e renderizada via base URL.
  //     Se a resolução do base URL falhou, documento e CSS estão ausentes → tudo preto (mean≈0).
  if (mean_brightness < 5.0) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [1] mean_brightness=%.1f < 5 — scene did not load via base URL\n",
        mean_brightness);
    ++failures;
  }

  // EN: Check 2 — dark background present (body #0d1020, R+G+B=61 < 120, ≥ 10 % of frame).
  // PT: Cheque 2 — fundo escuro presente (body #0d1020, R+G+B=61 < 120, ≥ 10 % do frame).
  if (dark_count < total / 10) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [2] dark_count=%zu < %zu (10%%) — body background missing\n",
        dark_count, total / 10);
    ++failures;
  }

  // EN: Check 3 — bright/colourful pixels present (gradient + glow from embed_scene.rcss).
  // PT: Cheque 3 — pixels brilhantes/coloridos presentes (gradiente + glow do embed_scene.rcss).
  if (bright_count < total / 100) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [3] bright_count=%zu < %zu (1%%) — gradient/glow missing\n",
        bright_count, total / 100);
    ++failures;
  }

  if (failures == 0) {
    std::puts("base_url_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "base_url_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
