// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — construct RenderGl3 under a live GL context, run one frame.
// PT: Teste de fumaça — constrói RenderGl3 sob contexto GL ativo, executa um frame.
#include "../src/window_glfw.hpp"
#include "../src/render_gl3.hpp"
#include <cstdio>
int main() {
  glintfx::WindowGlfw w;
  if (!w.create("r", 320, 240)) { std::puts("window create failed"); return 1; }
  glintfx::RenderGl3 r;
  if (!r.init()) { std::puts("render init failed"); return 2; }
  int ww, hh; w.size(ww, hh);
  r.begin_frame(ww, hh);
  r.end_frame();
  std::puts("render smoke OK");
  return 0;
}
