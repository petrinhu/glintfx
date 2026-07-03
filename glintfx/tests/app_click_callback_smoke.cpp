// SPDX-License-Identifier: MPL-2.0
// EN: App parity smoke for set_click_callback() -- proves the surface is wired on App (F1
//     behaviour itself is fully covered by click_callback_sanity via UiLayer, same Engine path).
// PT: Smoke de paridade do App pra set_click_callback() -- prova que a superfície está
//     conectada no App (o comportamento da F1 em si é coberto por click_callback_sanity via
//     UiLayer, mesmo caminho de Engine).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>

int main() {
  glintfx::App app({ .title = "app_click_cb_smoke", .width = 300, .height = 200 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  std::string last_id = "<none>";
  app.set_click_callback([&last_id](const char* id) { last_id = id; });

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention -- see
  //     app_dp_ratio_smoke.cpp/app_smoke.cpp/data_model_smoke.cpp); click_scene.rml is copied
  //     once to CMAKE_CURRENT_BINARY_DIR (build/tests/) by tests/CMakeLists.txt, so it must be
  //     addressed with the "tests/" prefix here (deviation from the plan's literal snippet,
  //     which omitted the prefix -- see implementer's final report).
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo -- ver
  //     app_dp_ratio_smoke.cpp/app_smoke.cpp/data_model_smoke.cpp); click_scene.rml é copiado
  //     uma vez para CMAKE_CURRENT_BINARY_DIR (build/tests/) pelo tests/CMakeLists.txt, então
  //     precisa do prefixo "tests/" aqui (desvio do trecho literal do plano, que omitiu o
  //     prefixo -- ver relatório final do implementador).
  if (!app.load("tests/click_scene.rml")) { std::puts("FAIL: load"); return 2; }
  app.update();
  app.render();
  if (!app.ok()) { std::puts("FAIL: ok() false after render"); return 3; }

  std::puts("app_click_callback_smoke OK");
  return 0;
}
