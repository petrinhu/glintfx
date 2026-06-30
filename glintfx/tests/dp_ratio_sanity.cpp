// SPDX-License-Identifier: MPL-2.0
// EN: dp_ratio sanity test — verifies that UiLayerConfig::dp_ratio and set_dp_ratio() work.
//
//     Scene: black background + one white 100dp×100dp box (dp_ratio_scene.rml/.rcss).
//     At dp_ratio=1.0: the box occupies 100×100 physical pixels = 10 000 bright pixels.
//     At dp_ratio=2.0: the box occupies 200×200 physical pixels = 40 000 bright pixels (4×).
//     We assert that the bright pixel count at ratio=2.0 is at least 3× the count at ratio=1.0,
//     giving a comfortable margin against llvmpipe threaded-tile non-determinism.
//
//     Methodology:
//       1. Host creates a hidden 900×600 GLFW window (GL context provider).
//       2. UiLayer constructed with dp_ratio=1.0 (default) — tests the Config path.
//       3. Load dp_ratio_scene.rml, warm up (2 frames), capture → bright_count1.
//       4. Call set_dp_ratio(2.0f) — tests the runtime setter path.
//       5. Warm up 2 more frames (let layout re-calculate), capture → bright_count2.
//       6. Assert bright_count2 >= 3 * bright_count1.
//
//     "Bright" definition: any pixel with ALL channels > 200 (pure white is 255,255,255).
//     This threshold is unambiguous for a pure-white box on a pure-black background.
//
// PT: Teste de sanidade do dp_ratio — verifica que UiLayerConfig::dp_ratio e set_dp_ratio()
//     funcionam.
//
//     Cena: fundo preto + um box branco 100dp×100dp (dp_ratio_scene.rml/.rcss).
//     Em dp_ratio=1.0: o box ocupa 100×100 pixels físicos = 10 000 pixels brilhantes.
//     Em dp_ratio=2.0: o box ocupa 200×200 pixels físicos = 40 000 pixels brilhantes (4×).
//     Verificamos que a contagem de pixels brilhantes em ratio=2.0 é pelo menos 3× a de ratio=1.0,
//     dando margem confortável contra o não-determinismo de tiles em threads do llvmpipe.
//
//     Metodologia:
//       1. Host cria janela GLFW oculta 900×600 (provedor de contexto GL).
//       2. UiLayer construída com dp_ratio=1.0 (padrão) — testa o caminho Config.
//       3. Carrega dp_ratio_scene.rml, aquece (2 frames), captura → bright_count1.
//       4. Chama set_dp_ratio(2.0f) — testa o caminho setter runtime.
//       5. Aquece mais 2 frames (deixa o layout recalcular), captura → bright_count2.
//       6. Verifica bright_count2 >= 3 * bright_count1.
//
//     Definição de "brilhante": qualquer pixel com TODOS os canais > 200 (branco puro = 255,255,255).
//     Este limiar é inequívoco para box branco puro em fundo preto puro.
//
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h (gl3w already loaded by UiLayer ctor).
                           // PT: inclui GL/gl3w.h (gl3w já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Count pixels where all channels (R, G, B) > threshold.
// PT: Conta pixels onde todos os canais (R, G, B) > limiar.
// ---------------------------------------------------------------------------
static size_t count_bright(const std::vector<unsigned char>& px,
                           int w, int h, int threshold = 200) {
  size_t count = 0;
  const size_t total = static_cast<size_t>(w) * h;
  for (size_t i = 0; i < total; ++i) {
    if (px[i * 3 + 0] > threshold &&
        px[i * 3 + 1] > threshold &&
        px[i * 3 + 2] > threshold)
      ++count;
  }
  return count;
}

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK).
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK).
// ---------------------------------------------------------------------------
static std::vector<unsigned char> capture(glintfx::UiLayer& ui, int w, int h) {
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 — host creates a hidden 900×600 window (GL context provider).
  // PT: Passo 1 — host cria janela oculta 900×600 (provê contexto GL).
  // ---------------------------------------------------------------------------
  constexpr int W = 900, H = 600;
  glintfx::WindowGlfw host;
  if (!host.create("dp_ratio_sanity_host", W, H)) {
    std::puts("dp_ratio_sanity FAIL: host window create failed");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 — UiLayer attaches with dp_ratio=1.0 (default via Config path).
  // PT: Passo 2 — UiLayer anexa com dp_ratio=1.0 (padrão via caminho Config).
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H,
                        .load_gl = true, .dp_ratio = 1.0f });
  if (!ui.ok()) {
    std::puts("dp_ratio_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3a — clear to black (known baseline), then load the scene.
  // PT: Passo 3a — limpa para preto (baseline conhecido), depois carrega a cena.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.load("dp_ratio_scene.rml");

  // ---------------------------------------------------------------------------
  // EN: Step 3b — capture at dp_ratio=1.0.
  // PT: Passo 3b — captura em dp_ratio=1.0.
  // ---------------------------------------------------------------------------
  const auto px1 = capture(ui, W, H);
  const size_t bright1 = count_bright(px1, W, H);
  std::printf("dp_ratio_sanity: ratio=1.0 bright=%zu\n", bright1);

  // EN: Sanity guard: at dp_ratio=1.0 the box should cover at least 50×50 px
  //     (conservative lower bound; actual is 100×100 = 10 000 px under normal layout).
  // PT: Guard de sanidade: em dp_ratio=1.0 o box deve cobrir ao menos 50×50 px
  //     (limite inferior conservador; real é 100×100 = 10 000 px no layout normal).
  if (bright1 < 2500u) {
    std::fprintf(stderr,
        "dp_ratio_sanity FAIL: ratio=1.0 bright=%zu < 2 500 — scene may not have rendered\n",
        bright1);
    return 3;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 4 — switch to dp_ratio=2.0 (runtime setter path).
  // PT: Passo 4 — muda para dp_ratio=2.0 (caminho setter runtime).
  // ---------------------------------------------------------------------------
  ui.set_dp_ratio(2.0f);

  // ---------------------------------------------------------------------------
  // EN: Step 5 — capture at dp_ratio=2.0.
  // PT: Passo 5 — captura em dp_ratio=2.0.
  // ---------------------------------------------------------------------------
  const auto px2 = capture(ui, W, H);
  const size_t bright2 = count_bright(px2, W, H);
  std::printf("dp_ratio_sanity: ratio=2.0 bright=%zu\n", bright2);

  // ---------------------------------------------------------------------------
  // EN: Step 6 — assert scaling: bright2 must be ≥ 3× bright1.
  //     Expected ratio is ~4× (100dp→100px vs 100dp→200px: area 4×).
  //     Threshold of 3× gives tolerance for sub-pixel edge AA and llvmpipe tiling.
  // PT: Passo 6 — verifica escala: bright2 deve ser ≥ 3× bright1.
  //     Razão esperada é ~4× (100dp→100px vs 100dp→200px: área 4×).
  //     Limiar de 3× dá tolerância para AA de borda sub-pixel e tiling do llvmpipe.
  // ---------------------------------------------------------------------------
  if (bright2 < bright1 * 3u) {
    std::fprintf(stderr,
        "dp_ratio_sanity FAIL: bright at ratio=2.0 (%zu) < 3 * bright at ratio=1.0 (%zu)\n"
        "  Expected ~4× scaling (100dp×100dp: 100px→200px in each axis)\n",
        bright2, bright1 * 3u);
    return 4;
  }

  std::printf("dp_ratio_sanity: PASS  (scale factor bright2/bright1 = %.2f, expected ~4.0)\n",
              (double)bright2 / (double)bright1);
  return 0;
}
