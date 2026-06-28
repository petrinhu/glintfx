// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — initialize RmlUi via Bootstrap, load a minimal document, shutdown cleanly.
// PT: Teste de fumaça — inicializa RmlUi via Bootstrap, carrega documento mínimo, encerra limpo.
#include "../src/window_glfw.hpp"
#include "../src/render_gl3.hpp"
#include "../src/bootstrap.hpp"
#include <cstdio>
int main() {
  glintfx::WindowGlfw w;
  if (!w.create("b", 320, 240)) { std::puts("window create failed"); return 1; }
  glintfx::RenderGl3 r;
  if (!r.init()) { std::puts("render init failed"); return 2; }
  glintfx::Bootstrap b;
  if (!b.init(w, r, 320, 240)) { std::puts("bootstrap init failed"); return 3; }
  if (!b.load("tests/min.rml")) { std::puts("load failed"); return 4; }
  b.shutdown();
  std::puts("bootstrap smoke OK");
  return 0;
}
