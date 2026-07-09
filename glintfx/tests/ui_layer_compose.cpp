// SPDX-License-Identifier: MPL-2.0
// EN: Verify compose-only render preserves the host's framebuffer contents.
//
//     Architecture note: the RmlUi GL3 backend's EndFrame() always blends the UI result
//     directly to FBO 0 (the window backbuffer). This matches the production embed use-case
//     where the host renders its scene to the window (FBO 0) and UiLayer overlays the UI on
//     top — without the host giving up ownership of FBO 0 and without any buffer swap from
//     within UiLayer. For the test we use a hidden GLFW window; the window's back buffer plays
//     the role of "host FBO 0".
//
//     Proof objectives:
//       (a) Background preserved — the host clears FBO 0 to an anchor colour; after
//           UiLayer::render() the anchor colour must survive in the areas the UI did not cover,
//           proving begin_frame_compose() did NOT call glClear on FBO 0.
//       (b) UI composed something — at least one pixel in FBO 0 must differ from the anchor
//           colour after render(), proving the UI layer actually drew into FBO 0.
//
// PT: Verifica que o render compose-only preserva o conteúdo do framebuffer do host.
//
//     Nota de arquitetura: o EndFrame() do backend GL3 do RmlUi sempre mistura o resultado
//     da UI diretamente ao FBO 0 (o backbuffer da janela). Isso corresponde ao caso embed de
//     produção onde o host renderiza a cena na janela (FBO 0) e a UiLayer sobrepõe a UI por
//     cima — sem que o host abandone a posse do FBO 0 e sem swap de buffer pela UiLayer.
//     No teste usamos janela GLFW oculta; o back buffer da janela faz o papel do "FBO 0 do host".
//
//     Objetivos de prova:
//       (a) Fundo preservado — o host limpa FBO 0 com cor âncora; após UiLayer::render()
//           a cor âncora deve sobreviver nas áreas que a UI não cobriu, provando que
//           begin_frame_compose() NÃO chamou glClear no FBO 0.
//       (b) UI compôs algo — ao menos 1 pixel do FBO 0 deve diferir da cor âncora após
//           render(), provando que a camada UI de fato desenhou no FBO 0.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: for gl_loader.h and glx_gl_load() side-effect.
                           // PT: para gl_loader.h e efeito colateral de glx_gl_load().
#include <cstdio>
#include <cstdlib>   // std::abs (int)
#include <vector>

int main() {
  // EN: Host creates a hidden 400×300 window — FBO 0 is the render target.
  // PT: Host cria janela oculta 400×300 — FBO 0 é o render target.
  glintfx::WindowGlfw host;
  if (!host.create("compose_host", 400, 300)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  // EN: UiLayer attaches to the host's CURRENT GL context (loads GL function pointers here).
  // PT: UiLayer anexa ao contexto GL CORRENTE do host (carrega os ponteiros de função GL aqui).
  glintfx::UiLayer ui({ .logical_width = 400, .logical_height = 300, .load_gl = true });
  if (!ui.ok()) {
    std::puts("FAIL: ui attach failed");
    return 2;
  }

  // EN: Load a document with a small opaque white element at a fixed corner.
  //     No font dependency — the element renders from background-color alone.
  //     Most of the viewport is transparent so the anchor colour survives there.
  // PT: Carrega documento com elemento branco opaco pequeno num canto fixo.
  //     Sem dep de fonte — o elemento renderiza só a partir de background-color.
  //     A maior parte do viewport é transparente, então a cor âncora sobrevive lá.
  ui.load("tests/min_partial.rml");

  // EN: Clear FBO 0 (the window back buffer) to a distinctive non-black anchor colour.
  //     Using R=10,G=20,B=40 (dark blue) makes the "not cleared" proof unambiguous:
  //     any accidental glClear() would erase this distinctive hue.
  //     render_compose's begin_frame_compose() must NOT touch FBO 0 at all — it only
  //     clears the RmlUi internal FBOs (render layers) inside BeginFrame().
  // PT: Limpa FBO 0 (back buffer da janela) com cor âncora distinta e não-preta.
  //     R=10,G=20,B=40 (azul escuro) torna a prova "não foi limpo" sem ambiguidade:
  //     qualquer glClear() acidental apagaria esse tom distinto.
  //     begin_frame_compose() do render_compose NÃO deve tocar no FBO 0 — limpa apenas
  //     os FBOs internos do RmlUi (render layers) dentro de BeginFrame().
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // EN: Warm up + compose: update layout, then render UI over FBO 0.
  //     EndFrame() blends the postprocessed UI onto FBO 0 using premultiplied-alpha
  //     blend (GL_ONE, GL_ONE_MINUS_SRC_ALPHA): transparent UI pixels preserve the
  //     anchor colour; opaque UI pixels (the white #box) overwrite it with white.
  // PT: Aquecimento + compose: atualiza layout, renderiza UI sobre FBO 0.
  //     EndFrame() mistura a UI pós-processada no FBO 0 via blend premultiplied-alpha
  //     (GL_ONE, GL_ONE_MINUS_SRC_ALPHA): pixels UI transparentes preservam a cor âncora;
  //     pixels UI opacos (o #box branco) sobrescrevem com branco.
  ui.update();
  ui.render();

  // EN: Read back from FBO 0 (window back buffer, not yet swapped).
  // PT: Lê de volta do FBO 0 (back buffer da janela, ainda não trocado).
  const int W = 400, H = 300;
  std::vector<unsigned char> pixels(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  // EN: Tally anchor-colour vs changed pixels.
  //     Tolerance ±5 per channel absorbs premultiplied-alpha blend boundary artefacts.
  // PT: Conta pixels de cor âncora vs alterados.
  //     Tolerância ±5 por canal absorve artefatos de borda do blend premultiplied-alpha.
  int anchor_count  = 0;
  int changed_count = 0;
  const int total   = W * H;
  for (int i = 0; i < total; ++i) {
    const int r = pixels[i * 3 + 0];
    const int g = pixels[i * 3 + 1];
    const int b = pixels[i * 3 + 2];
    const bool at_anchor = (std::abs(r - 10) <= 5)
                        && (std::abs(g - 20) <= 5)
                        && (std::abs(b - 40) <= 5);
    if (at_anchor) ++anchor_count;
    else           ++changed_count;
  }

  // EN: (a) Background preserved — begin_frame_compose must NOT clear FBO 0.
  //     At least 50 % of pixels must remain at anchor colour (the #box covers ~2.7 %).
  // PT: (a) Fundo preservado — begin_frame_compose NÃO deve limpar o FBO 0.
  //     Ao menos 50 % dos pixels devem permanecer na cor âncora (o #box cobre ~2,7 %).
  if (anchor_count < total / 2) {
    std::printf("FAIL: fundo apagado — %d/%d pixels em cor-ancora (esperado >= %d)\n",
                anchor_count, total, total / 2);
    return 3;
  }

  // EN: (b) UI composed something — at least one pixel must differ from anchor.
  // PT: (b) UI compôs algo — ao menos um pixel deve diferir da âncora.
  if (changed_count < 1) {
    std::puts("FAIL: UI nao desenhou nada — nenhum pixel mudou da cor-ancora no FBO 0");
    return 4;
  }

  std::printf("ui_layer_compose: PASS (anchor=%d changed=%d total=%d)\n",
              anchor_count, changed_count, total);
  return 0;
}
