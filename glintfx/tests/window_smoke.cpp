// SPDX-License-Identifier: MPL-2.0
#include "../src/window_glfw.hpp"
#include <cstdio>
int main() {
  glintfx::WindowGlfw w;
  if (!w.create("smoke", 320, 240)) { std::puts("create failed"); return 1; }
  int ww=0, hh=0; w.size(ww, hh);
  if (ww <= 0 || hh <= 0) { std::puts("bad size"); return 2; }
  w.make_current();
  w.swap();           // não deve crashar
  std::puts("window smoke OK");
  return 0;
}
