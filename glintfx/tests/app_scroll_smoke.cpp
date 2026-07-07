// SPDX-License-Identifier: MPL-2.0
// EN: App parity smoke for the five scroll methods (GLINTFX-SCROLL-1, v0.3.2):
//     scroll_element_into_view()/get_element_scroll_top()/set_element_scroll_top()/
//     get_element_scroll_height()/get_element_client_height() -- proves the surface is wired
//     on App and returns the expected true/values for a known id. Full behavioural proof (wheel
//     forwarding, clamping, hardening) lives in scroll_sanity.cpp via UiLayer (same Engine
//     path App delegates to).
// PT: Smoke de paridade do App para os cinco métodos de rolagem (GLINTFX-SCROLL-1, v0.3.2):
//     scroll_element_into_view()/get_element_scroll_top()/set_element_scroll_top()/
//     get_element_scroll_height()/get_element_client_height() -- prova que a superfície está
//     conectada no App e retorna os true/valores esperados para um id conhecido. A prova
//     comportamental completa (encaminhamento de wheel, saturação, hardening) está em
//     scroll_sanity.cpp via UiLayer (mesmo caminho de Engine para o qual o App delega).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cmath>
#include <cstdio>

static bool approx(float a, float b, float tol = 0.5f) { return std::fabs(a - b) <= tol; }

int main() {
  glintfx::App app({ .title = "app_scroll_smoke", .width = 200, .height = 200 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); scroll_scene.rml
  //     is copied once to CMAKE_CURRENT_BINARY_DIR (build/tests/) by the scroll_sanity block in
  //     tests/CMakeLists.txt, addressed here with the "tests/" prefix (same pattern as
  //     app_element_box_smoke.cpp).
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo);
  //     scroll_scene.rml é copiado uma vez para CMAKE_CURRENT_BINARY_DIR (build/tests/) pelo
  //     bloco scroll_sanity em tests/CMakeLists.txt, endereçado aqui com o prefixo "tests/"
  //     (mesmo padrão de app_element_box_smoke.cpp).
  if (!app.load("tests/scroll_scene.rml")) { std::puts("FAIL: load"); return 2; }
  app.update();
  app.render();

  if (!app.set_element_scroll_top("scroller", 50.f)) { std::puts("FAIL: set_element_scroll_top"); return 3; }
  float top = -1.f;
  if (!app.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_element_scroll_top"); return 4; }
  if (!approx(top, 50.f)) { std::fprintf(stderr, "FAIL: scroll_top=%.2f expected ~50\n", top); return 5; }

  float scroll_h = -1.f, client_h = -1.f;
  if (!app.get_element_scroll_height("scroller", scroll_h)) { std::puts("FAIL: get_element_scroll_height"); return 9; }
  if (!approx(scroll_h, 900.f, 1.0f)) { std::fprintf(stderr, "FAIL: scroll_height=%.2f expected ~900\n", scroll_h); return 10; }
  if (!app.get_element_client_height("scroller", client_h)) { std::puts("FAIL: get_element_client_height"); return 11; }
  if (!approx(client_h, 100.f, 1.0f)) { std::fprintf(stderr, "FAIL: client_height=%.2f expected ~100\n", client_h); return 12; }

  if (!app.scroll_element_into_view("item-29")) { std::puts("FAIL: scroll_element_into_view"); return 6; }
  if (!app.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_element_scroll_top after scroll_into_view"); return 7; }
  if (!approx(top, 800.f, 1.5f)) { std::fprintf(stderr, "FAIL: scroll_top=%.2f expected ~800\n", top); return 8; }

  std::puts("app_scroll_smoke OK");
  return 0;
}
