// SPDX-License-Identifier: MPL-2.0
// EN: base_url sanity test — verifies set_asset_base_url() on UiLayer.
//
//     The test proves two things:
//
//     [Negative] Without a base URL, load("bu_scene.rml") silently fails because
//     bu_scene.rml does NOT exist in the process CWD (= build/tests/).  CMakeLists
//     copies bu_scene.rml/.rcss ONLY to base_url_assets/, never to the CWD root.
//     A render after the failed load produces all-black (no document shown).
//
//     [Positive] After set_asset_base_url("base_url_assets"), the same relative path
//     resolves to base_url_assets/bu_scene.rml.  The scene renders and passes the
//     same three aggregate pixel statistics used in ui_layer_sanity.
//
//     Test flow:
//       1. Host creates a hidden 900x600 GLFW window.
//       2. UiLayer attaches (no base URL set).
//       3. [Negative] Clear FBO 0 to black, load("bu_scene.rml"), render 2 frames,
//          capture FBO 0, assert mean_brightness < 5 (still black == document not found).
//       4. set_asset_base_url("base_url_assets").
//       5. [Positive] Clear FBO 0 to black, load("bu_scene.rml") again, render 2 frames,
//          capture FBO 0, assert three pixel statistics (same as ui_layer_sanity).
//
// PT: Teste de sanidade do base_url -- verifica set_asset_base_url() no UiLayer.
//
//     O teste prova duas coisas:
//
//     [Negativo] Sem base URL, load("bu_scene.rml") falha silenciosamente porque
//     bu_scene.rml NAO existe no CWD do processo (= build/tests/). O CMakeLists
//     copia bu_scene.rml/.rcss SOMENTE para base_url_assets/, nunca para a raiz do CWD.
//     Um render apos a carga falha produz tudo-preto (nenhum documento exibido).
//
//     [Positivo] Apos set_asset_base_url("base_url_assets"), o mesmo path relativo
//     resolve para base_url_assets/bu_scene.rml. A cena renderiza e passa as mesmas
//     tres estatisticas agregadas de pixels usadas em ui_layer_sanity.
//
//     Fluxo do teste:
//       1. Host cria janela GLFW oculta 900x600.
//       2. UiLayer anexa (sem base URL definido).
//       3. [Negativo] Limpa FBO 0 para preto, load("bu_scene.rml"), renderiza 2 frames,
//          captura FBO 0, verifica mean_brightness < 5 (ainda preto = documento nao encontrado).
//       4. set_asset_base_url("base_url_assets").
//       5. [Positivo] Limpa FBO 0 para preto, load("bu_scene.rml") novamente, renderiza
//          2 frames, captura FBO 0, verifica tres estatisticas de pixel.
//
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes gl_loader.h. PT: inclui gl_loader.h.
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) into a flat RGB buffer.
// PT: Renderiza dois frames e le o FBO 0 (GL_BACK) num buffer RGB plano.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> capture_fbo(glintfx::UiLayer& ui, int w, int h) {
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

// ---------------------------------------------------------------------------
// EN: Compute mean brightness per channel (sum / (total * 3)).
// PT: Calcula brilho medio por canal (soma / (total * 3)).
// ---------------------------------------------------------------------------
static double mean_brightness(const std::vector<unsigned char>& px, int w, int h) {
  long long sum = 0;
  const size_t total = static_cast<size_t>(w) * h;
  for (size_t i = 0; i < total; ++i)
    sum += px[i * 3 + 0] + px[i * 3 + 1] + px[i * 3 + 2];
  return (double)sum / ((double)total * 3.0);
}

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 -- host creates a hidden 900x600 window (GL context provider).
  // PT: Passo 1 -- host cria janela oculta 900x600 (prove o contexto GL).
  // ---------------------------------------------------------------------------
  constexpr int W = 900, H = 600;
  glintfx::WindowGlfw host;
  if (!host.create("base_url_sanity_host", W, H)) {
    std::puts("base_url_sanity FAIL: host window create failed");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 -- UiLayer attaches. No base URL set yet.
  // PT: Passo 2 -- UiLayer anexa. Sem base URL definido ainda.
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("base_url_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 -- [NEGATIVE ASSERT] Load without base URL.
  //     bu_scene.rml is ONLY in base_url_assets/, not in the CWD (= build/tests/).
  //     CMakeLists enforces this by copying ONLY to the subdirectory.
  //     The load silently fails (LoadDocument returns nullptr, UiLayer::load is void);
  //     rendering two frames leaves FBO 0 all-black because no document is displayed.
  // PT: Passo 3 -- [ASSERT NEGATIVO] Carrega sem base URL.
  //     bu_scene.rml esta SOMENTE em base_url_assets/, nao no CWD (= build/tests/).
  //     O CMakeLists impoe isso copiando SOMENTE para o subdiretorio.
  //     A carga falha silenciosamente (LoadDocument retorna nullptr, UiLayer::load e void);
  //     renderizar dois frames deixa FBO 0 todo-preto pois nenhum documento e exibido.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  ui.load("bu_scene.rml");  // EN: expected to fail -- file NOT in CWD.
                             // PT: esperado falhar -- arquivo NAO esta no CWD.

  const auto px_neg  = capture_fbo(ui, W, H);
  const double m_neg = mean_brightness(px_neg, W, H);
  std::printf("base_url_sanity [negative]: mean_brightness=%.2f (expect < 5 -- all black)\n",
              m_neg);

  if (m_neg >= 5.0) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [negative]: mean=%.2f >= 5 -- bu_scene.rml was found without "
        "base URL. Verify bu_scene.rml is NOT present in build/tests/ (CWD root).\n", m_neg);
    return 3;
  }
  std::puts("base_url_sanity [negative]: PASS -- document correctly not found without base URL");

  // ---------------------------------------------------------------------------
  // EN: Step 4 -- set base URL to the subdirectory that holds bu_scene.rml/.rcss.
  //     After this call, relative paths in Open() are prefixed with "base_url_assets/".
  // PT: Passo 4 -- define base URL para o subdiretorio que contem bu_scene.rml/.rcss.
  //     Apos esta chamada, paths relativos em Open() sao prefixados com "base_url_assets/".
  // ---------------------------------------------------------------------------
  ui.set_asset_base_url("base_url_assets");

  // ---------------------------------------------------------------------------
  // EN: Step 5 -- [POSITIVE] Load the same relative path again. Now the FileInterface
  //     opens base_url_assets/bu_scene.rml (then base_url_assets/bu_scene.rcss for the
  //     <link> inside the RML). Clear FBO 0 first for a deterministic baseline.
  // PT: Passo 5 -- [POSITIVO] Carrega o mesmo path relativo novamente. Agora o FileInterface
  //     abre base_url_assets/bu_scene.rml (depois base_url_assets/bu_scene.rcss para o
  //     <link> dentro do RML). Limpa FBO 0 primeiro para baseline deterministico.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  ui.load("bu_scene.rml");  // EN: now resolves via base URL. PT: agora resolve via base URL.

  const auto px_pos = capture_fbo(ui, W, H);
  const size_t total = static_cast<size_t>(W) * H;
  long long brightness_sum = 0;
  size_t dark_count   = 0;
  size_t bright_count = 0;
  for (size_t i = 0; i < total; ++i) {
    const int r = px_pos[i * 3 + 0];
    const int g = px_pos[i * 3 + 1];
    const int b = px_pos[i * 3 + 2];
    brightness_sum += r + g + b;
    if (r + g + b < 120)               ++dark_count;
    if (r > 150 || g > 150 || b > 150) ++bright_count;
  }

  const double mean_pos   = (double)brightness_sum / ((double)total * 3.0);
  const double dark_pct   = 100.0 * (double)dark_count   / (double)total;
  const double bright_pct = 100.0 * (double)bright_count / (double)total;

  std::printf("base_url_sanity [positive]: %dx%d  mean=%.1f  dark=%.1f%%  bright=%.1f%%\n",
              W, H, mean_pos, dark_pct, bright_pct);

  int failures = 0;

  // EN: Check 1 -- not all black: scene loaded and rendered via base URL.
  // PT: Cheque 1 -- nao tudo preto: cena carregada e renderizada via base URL.
  if (mean_pos < 5.0) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [positive/1] mean=%.1f < 5 -- scene not loaded via base URL\n",
        mean_pos);
    ++failures;
  }

  // EN: Check 2 -- dark background present (body #0d1020, R+G+B=61 < 120, >= 10% of frame).
  // PT: Cheque 2 -- fundo escuro presente (body #0d1020, R+G+B=61 < 120, >= 10% do frame).
  if (dark_count < total / 10) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [positive/2] dark_count=%zu < %zu (10%%) -- background missing\n",
        dark_count, total / 10);
    ++failures;
  }

  // EN: Check 3 -- bright/colourful pixels present (gradient + glow from bu_scene.rcss).
  // PT: Cheque 3 -- pixels brilhantes/coloridos presentes (gradiente + glow do bu_scene.rcss).
  if (bright_count < total / 100) {
    std::fprintf(stderr,
        "base_url_sanity FAIL [positive/3] bright_count=%zu < %zu (1%%) -- gradient/glow missing\n",
        bright_count, total / 100);
    ++failures;
  }

  if (failures == 0) {
    std::puts("base_url_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "base_url_sanity: %d positive check(s) FAILED\n", failures);
  return failures;
}
