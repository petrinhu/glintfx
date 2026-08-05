// SPDX-License-Identifier: Apache-2.0
// EN: App parity smoke for set_cursor_callback() (AUD-PUB-6g) -- proves the surface is wired on
//     App (the "fires only on change" behaviour itself is fully covered by
//     cursor_callback_sanity via UiLayer, same RmlUi Context::UpdateHoverChain path). On App the
//     callback is an OBSERVER, not a replacement: SystemInterfaceGlfwDedup::SetMouseCursor calls
//     the inherited SystemInterface_GLFW::SetMouseCursor() FIRST (the real glfwSetCursor the
//     window already relied on), THEN forwards the same name to this callback -- App::ok() must
//     stay true after render() with the callback installed, same as every other App parity
//     smoke in this family (app_click_callback_smoke, app_scroll_smoke, ...).
// PT: Smoke de paridade do App para set_cursor_callback() (AUD-PUB-6g) -- prova que a
//     superfície está conectada no App (o comportamento "só dispara na mudança" em si é coberto
//     por cursor_callback_sanity via UiLayer, mesmo caminho Context::UpdateHoverChain do RmlUi).
//     No App o callback OBSERVA, não substitui: SystemInterfaceGlfwDedup::SetMouseCursor chama
//     PRIMEIRO o SystemInterface_GLFW::SetMouseCursor() herdado (o glfwSetCursor real do qual a
//     janela já dependia), DEPOIS repassa o mesmo nome a este callback -- App::ok() precisa
//     continuar true após render() com o callback instalado, igual a todo outro smoke de
//     paridade do App desta família (app_click_callback_smoke, app_scroll_smoke, ...).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>

int main() {
  glintfx::App app({.title = "app_cursor_cb_smoke", .width = 300, .height = 200});
  if (!app.ok()) {
    std::puts("FAIL: app ok() false");
    return 1;
  }

  std::string last_cursor = "<none>";
  app.set_cursor_callback([&last_cursor](const char* name) { last_cursor = name; });

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); cursor_scene.rml
  //     is copied once to CMAKE_CURRENT_BINARY_DIR by tests/CMakeLists.txt, addressed here with
  //     the "tests/" prefix -- same pattern as app_click_callback_smoke.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo);
  //     cursor_scene.rml é copiado uma vez para CMAKE_CURRENT_BINARY_DIR por
  //     tests/CMakeLists.txt, endereçado aqui com o prefixo "tests/" -- mesmo padrão de
  //     app_click_callback_smoke.
  if (!app.load("tests/cursor_scene.rml")) {
    std::puts("FAIL: load");
    return 2;
  }
  app.update();
  app.render();
  if (!app.ok()) {
    std::puts("FAIL: ok() false after render");
    return 3;
  }

  std::puts("app_cursor_callback_smoke OK");
  return 0;
}
