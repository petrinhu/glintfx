// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test for UiLayer embed attach:
//     The HOST owns the GL context (hidden GLFW window here stands in for SDL3 in GusWorld).
//     Verifies: attach succeeds, load() does not crash, set_viewport() does not crash.
// PT: Teste de fumaça para o attach do embed UiLayer:
//     O HOST é dono do contexto GL (janela GLFW oculta aqui faz o papel do SDL3 no GusWorld).
//     Verifica: attach bem-sucedido, load() não crasha, set_viewport() não crasha.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  // EN: Host creates a hidden window — this establishes the GL context the UiLayer will use.
  // PT: Host cria janela oculta — estabelece o contexto GL que o UiLayer vai usar.
  glintfx::WindowGlfw host;
  if (!host.create("host", 640, 480)) { std::puts("host window create failed"); return 1; }

  // EN: UiLayer attaches to the CURRENT GL context (already made current by WindowGlfw::create).
  // PT: UiLayer anexa ao contexto GL CORRENTE (já tornado corrente por WindowGlfw::create).
  glintfx::UiLayer ui({ .logical_width = 640, .logical_height = 480, .load_gl = true });
  if (!ui.ok()) { std::puts("ui attach failed"); return 2; }

  // EN: load() must not crash (min.rml is a minimal document with no font reference).
  // PT: load() não deve crashar (min.rml é documento mínimo sem referência de fonte).
  ui.load("tests/min.rml");

  // EN: Resize must not crash even if no render has happened yet.
  // PT: Resize não deve crashar mesmo que nenhum render tenha ocorrido ainda.
  ui.set_viewport(800, 600);

  std::puts("ui_layer_attach OK");
  return 0;
}
