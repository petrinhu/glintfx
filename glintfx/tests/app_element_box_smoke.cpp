// SPDX-License-Identifier: MPL-2.0
// EN: App parity smoke for get_element_box() -- proves the surface is wired on App.
// PT: Smoke de paridade do App pra get_element_box() -- prova que a superfície está conectada.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  glintfx::App app({ .title = "app_elbox_smoke", .width = 200, .height = 150 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention -- see
  //     app_click_callback_smoke.cpp/app_dp_ratio_smoke.cpp); element_box_scene.rml is
  //     copied once to CMAKE_CURRENT_BINARY_DIR (build/tests/) by tests/CMakeLists.txt, so
  //     it must be addressed with the "tests/" prefix here (same deviation as F1's
  //     app_click_callback_smoke.cpp).
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo -- ver
  //     app_click_callback_smoke.cpp/app_dp_ratio_smoke.cpp); element_box_scene.rml é copiado
  //     uma vez para CMAKE_CURRENT_BINARY_DIR (build/tests/) pelo tests/CMakeLists.txt, então
  //     precisa do prefixo "tests/" aqui (mesmo desvio de app_click_callback_smoke.cpp da F1).
  if (!app.load("tests/element_box_scene.rml")) { std::puts("FAIL: load"); return 2; }
  app.update();
  app.render();
  auto box = app.get_element_box("probe");
  if (!box.found) { std::puts("FAIL: found=false"); return 3; }
  std::puts("app_element_box_smoke OK");
  return 0;
}
