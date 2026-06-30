// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — initialise Engine (RmlUi+GL3) via SystemClock (no GLFW interface),
//     load a minimal document, verify ok(). Replaces bootstrap_smoke which tested Bootstrap
//     directly with a WindowGlfw argument (removed from Bootstrap::init in T1).
// PT: Teste de fumaça — inicializa Engine (RmlUi+GL3) via SystemClock (sem interface GLFW),
//     carrega documento mínimo, verifica ok(). Substitui bootstrap_smoke que testava Bootstrap
//     diretamente com argumento WindowGlfw (removido de Bootstrap::init na T1).
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <cstdio>

int main() {
  glintfx::WindowGlfw w;
  // EN: Window::create() establishes the GL context required by Engine::attach().
  // PT: Window::create() estabelece o contexto GL exigido por Engine::attach().
  if (!w.create("engine", 320, 240)) { std::puts("window create failed"); return 1; }

  glintfx::SystemClock clock;
  glintfx::Engine e;
  if (!e.attach(&clock, 320, 240)) { std::puts("engine attach failed"); return 2; }
  if (!e.load("tests/min.rml"))    { std::puts("load failed"); return 3; }

  std::puts("engine smoke OK");
  return 0;
}
