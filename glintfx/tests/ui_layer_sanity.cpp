// SPDX-License-Identifier: MPL-2.0
// EN: Embed-mode render sanity gate — the F1 analogue of render_sanity for the UiLayer path.
//
//     Architecture: RmlUi's EndFrame() always blends the final postprocessed UI result directly
//     to FBO 0 (the window back buffer), regardless of whatever FBO the host had bound.
//     This matches the production embed contract: the host renders its scene to the window's
//     FBO 0, then calls UiLayer::render() (compose-only: no glClear, no swap), and finally
//     does the swap itself. For the test we use a hidden 900×600 GLFW window whose FBO 0 is
//     the "offscreen" render target (invisible to the user; no swap issued by the test).
//
//     Test procedure:
//       1. Host creates hidden 900×600 window (provides the current GL context).
//       2. UiLayer attaches (loads gl3w, initialises RmlUi Engine in compose mode).
//       3. Host clears FBO 0 to black — UiLayer must NOT clear it during render().
//       4. Load embed_scene.rml (dark body + linear-gradient section + glow card).
//       5. Two warm-up frames (update + render each).
//       6. Read back FBO 0 (GL_BACK, before any swap) via glReadPixels.
//       7. Compute three aggregate statistics tolerant of llvmpipe's threaded tile rendering:
//            mean_brightness > 5    (rendered something; body background alone yields ~20)
//            dark_count    >= 10 %  (body background #0d1020, R+G+B=61 < 120, covers ≥ 50 %)
//            bright_count  >= 1 %   (gradient stop #6a0ddc B=220, stop #e02828 R=224, glow B=255)
//
//     Gate: all three checks pass → prints "ui_layer_sanity: PASS", exits 0.
//
// PT: Gate de sanidade do embed mode — análogo da F1 ao render_sanity para o caminho UiLayer.
//
//     Arquitetura: EndFrame() do RmlUi sempre mistura o resultado final da UI ao FBO 0
//     (o back buffer da janela), independente do FBO que o host tinha bound.
//     Isso corresponde ao contrato embed de produção: o host renderiza a cena no FBO 0 da
//     janela, chama UiLayer::render() (compose-only: sem glClear, sem swap) e depois faz o
//     swap ele mesmo. No teste usamos janela GLFW oculta 900×600 cujo FBO 0 é o render target
//     "offscreen" (invisível ao usuário; o teste não faz swap).
//
//     Procedimento:
//       1. Host cria janela oculta 900×600 (provê o contexto GL corrente).
//       2. UiLayer anexa (carrega gl3w, inicializa Engine do RmlUi em modo compose).
//       3. Host limpa FBO 0 a preto — UiLayer NÃO deve limpar durante render().
//       4. Carrega embed_scene.rml (body escuro + seção linear-gradient + card glow).
//       5. Dois frames de aquecimento (update + render em cada).
//       6. Lê FBO 0 (GL_BACK, antes de qualquer swap) via glReadPixels.
//       7. Calcula três estatísticas agregadas tolerantes ao rendering de tiles do llvmpipe:
//            mean_brightness > 5    (renderizou algo; body background já gera ~20)
//            dark_count    >= 10 %  (body #0d1020, R+G+B=61 < 120, cobre ≥ 50 %)
//            bright_count  >= 1 %   (gradiente #6a0ddc B=220, #e02828 R=224, glow B=255)
//
//     Gate: os três checks passam → imprime "ui_layer_sanity: PASS", sai com 0.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h (gl3w already loaded by UiLayer ctor).
                           // PT: inclui GL/gl3w.h (gl3w já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 — host creates a hidden 900×600 window (GL context provider).
  //     Window dimensions match the embed_scene RCSS layout expectations.
  // PT: Passo 1 — host cria janela oculta 900×600 (provê o contexto GL).
  //     Dimensões da janela coincidem com as expectativas de layout do embed_scene RCSS.
  // ---------------------------------------------------------------------------
  glintfx::WindowGlfw host;
  if (!host.create("sanity_host", 900, 600)) {
    std::puts("ui_layer_sanity FAIL: host window create failed");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 — UiLayer attaches to the current GL context.
  //     load_gl=true: UiLayer calls gl3wInit() (idempotent in this process).
  // PT: Passo 2 — UiLayer anexa ao contexto GL corrente.
  //     load_gl=true: UiLayer chama gl3wInit() (idempotente neste processo).
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({ .logical_width = 900, .logical_height = 600, .load_gl = true });
  if (!ui.ok()) {
    std::puts("ui_layer_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 — host clears FBO 0 (window back buffer) to black.
  //     UiLayer::render() is compose-only: it must NOT call glClear on FBO 0.
  //     After the compose, body background (#0d1020) overwrites the clear, but
  //     starting from black proves the compose path did not clear itself.
  // PT: Passo 3 — host limpa FBO 0 (back buffer da janela) a preto.
  //     UiLayer::render() é compose-only: NÃO deve chamar glClear no FBO 0.
  //     Após o compose, o background do body (#0d1020) sobrescreve o clear, mas
  //     partir de preto prova que o caminho compose não fez o clear por conta própria.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // ---------------------------------------------------------------------------
  // EN: Step 4 — load the embed sanity scene.
  //     WORKING_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR} (= build/tests/) so
  //     embed_scene.rml resolves there (copied via configure_file in CMakeLists).
  //     embed_scene.rcss and the RML itself live in the same build/tests/ dir.
  // PT: Passo 4 — carrega a cena de sanidade do embed.
  //     WORKING_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR} (= build/tests/) portanto
  //     embed_scene.rml resolve lá (copiado via configure_file no CMakeLists).
  //     embed_scene.rcss e o próprio RML ficam no mesmo dir build/tests/.
  // ---------------------------------------------------------------------------
  ui.load("embed_scene.rml");

  // ---------------------------------------------------------------------------
  // EN: Steps 5 — two warm-up frames: update layout and render (compose-only).
  //     Frame 1 settles RmlUi's internal layout/render-layer allocation.
  //     Frame 2 produces the stable state that we will inspect.
  //     Since body background-color is opaque, frame 2 fully overwrites frame 1.
  // PT: Passo 5 — dois frames de aquecimento: atualiza layout e renderiza (compose-only).
  //     Frame 1 estabiliza a alocação interna de layout/render-layer do RmlUi.
  //     Frame 2 produz o estado estável que inspecionaremos.
  //     Como body background-color é opaco, o frame 2 sobrescreve completamente o frame 1.
  // ---------------------------------------------------------------------------
  ui.update(); ui.render();
  ui.update(); ui.render();

  // ---------------------------------------------------------------------------
  // EN: Step 6 — read back FBO 0 (window back buffer, before any swap).
  //     EndFrame() always writes to FBO 0, so the result is in GL_BACK.
  // PT: Passo 6 — lê de volta do FBO 0 (back buffer da janela, antes de qualquer swap).
  //     EndFrame() sempre escreve no FBO 0, portanto o resultado está em GL_BACK.
  // ---------------------------------------------------------------------------
  constexpr int W = 900, H = 600;
  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());

  // ---------------------------------------------------------------------------
  // EN: Step 7 — aggregate pixel statistics (same methodology as render_sanity).
  //     All thresholds chosen to be robust against llvmpipe threaded-tile non-determinism.
  // PT: Passo 7 — estatísticas agregadas de pixel (mesma metodologia do render_sanity).
  //     Todos os limiares escolhidos para robustez contra o não-determinismo de tiles do llvmpipe.
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

  std::printf("ui_layer_sanity: %dx%d  mean=%.1f  dark=%.1f%%  bright=%.1f%%\n",
              W, H, mean_brightness, dark_pct, bright_pct);

  int failures = 0;

  // EN: Check 1 — not all black: the UiLayer rendered something into FBO 0.
  //     Body background #0d1020 (R=13, G=16, B=32) yields mean_channel ≈ 20 >> 5.
  //     Failure here means render() produced an all-black result (RmlUi init bug,
  //     viewport mismatch, or the compose path accidentally cleared FBO 0).
  // PT: Cheque 1 — não tudo preto: UiLayer renderizou algo no FBO 0.
  //     Body background #0d1020 (R=13, G=16, B=32) produz mean_channel ≈ 20 >> 5.
  //     Falha aqui significa que render() gerou resultado todo-preto (bug de init do RmlUi,
  //     viewport incompatível, ou o caminho compose limpou o FBO 0 acidentalmente).
  if (mean_brightness < 5.0) {
    std::fprintf(stderr,
        "ui_layer_sanity FAIL [1] mean_brightness=%.1f < 5 — appears all-black\n",
        mean_brightness);
    ++failures;
  }

  // EN: Check 2 — dark background pixels present: body background-color rendered.
  //     Body #0d1020 covers the upper 300 px of the 900×600 frame (≈50% of total).
  //     Requiring at least 10% (54 000 px) to qualify as dark (R+G+B < 120).
  // PT: Cheque 2 — pixels de fundo escuro presentes: background-color do body renderizado.
  //     Body #0d1020 cobre os 300 px superiores do frame 900×600 (≈50% do total).
  //     Requerendo ao menos 10% (54 000 px) classificados como escuros (R+G+B < 120).
  if (dark_count < total / 10) {
    std::fprintf(stderr,
        "ui_layer_sanity FAIL [2] dark_count=%zu < %zu (10%%) — dark background missing\n",
        dark_count, total / 10);
    ++failures;
  }

  // EN: Check 3 — bright/colourful pixels present: gradient + glow rendered.
  //     embed_scene gradient stops: #6a0ddc (B=220), #e02828 (R=224), glow #5fd0ff (B=255).
  //     Gradient section covers 900×300 = 270 000 px (50% of frame); most have max > 150.
  //     Requiring at least 1% (5 400 px) with any channel > 150.
  // PT: Cheque 3 — pixels brilhantes/coloridos presentes: gradiente + glow renderizados.
  //     Stops do gradiente embed_scene: #6a0ddc (B=220), #e02828 (R=224), glow #5fd0ff (B=255).
  //     Seção gradiente cobre 900×300 = 270 000 px (50% do frame); maioria tem max > 150.
  //     Requerendo ao menos 1% (5 400 px) com algum canal > 150.
  if (bright_count < total / 100) {
    std::fprintf(stderr,
        "ui_layer_sanity FAIL [3] bright_count=%zu < %zu (1%%) — gradient/glow missing\n",
        bright_count, total / 100);
    ++failures;
  }

  if (failures == 0) {
    std::puts("ui_layer_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "ui_layer_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
