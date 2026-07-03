// SPDX-License-Identifier: MPL-2.0
// EN: Verifies UiLayer::set_click_callback(): (A) direct id on the clicked element; (B) click
//     on a child WITHOUT an id bubbles up to report the nearest ancestor id ("panel"); (C) click
//     on a subtree with NO id anywhere reports "" (never a crash, never silently dropped).
//     A click is simulated as MouseMove (to hover/set mouse_position) + MouseButtonDown + Up at
//     the same point -- RmlUi's own click detection (down+up on the same element == click).
// PT: Verifica UiLayer::set_click_callback(): (A) id direto no elemento clicado; (B) clique num
//     filho SEM id sobe até reportar o ancestral mais próximo com id ("panel"); (C) clique numa
//     subárvore SEM id em lugar nenhum reporta "" (nunca crash, nunca descartado silenciosamente).
//     Um clique é simulado como MouseMove (hover/mouse_position) + MouseButtonDown + Up no mesmo
//     ponto -- a própria detecção de clique do RmlUi (down+up no mesmo elemento == click).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <string>
#include <vector>

static void click_at(glintfx::UiLayer& ui, float x, float y) {
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = x, .y = y });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = true });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = false });
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("click_cb_host", 300, 200)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 300, .logical_height = 200, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  std::vector<std::string> hits;
  ui.set_click_callback([&hits](const char* id) { hits.emplace_back(id); });

  if (!ui.load("click_scene.rml")) { std::puts("FAIL: load"); return 3; }
  ui.update(); ui.render();

  // (A) direct id
  click_at(ui, 50.f, 30.f);
  // (B) child without id -> bubbles to "panel"
  click_at(ui, 50.f, 80.f);
  // (C) subtree without any id -> ""
  click_at(ui, 50.f, 130.f);

  if (hits.size() != 3) {
    std::fprintf(stderr, "FAIL: expected 3 hits, got %zu\n", hits.size());
    return 4;
  }
  if (hits[0] != "btn_a") { std::fprintf(stderr, "FAIL: hits[0]='%s' expected 'btn_a'\n", hits[0].c_str()); return 5; }
  if (hits[1] != "panel") { std::fprintf(stderr, "FAIL: hits[1]='%s' expected 'panel'\n", hits[1].c_str()); return 6; }
  if (hits[2] != "")      { std::fprintf(stderr, "FAIL: hits[2]='%s' expected ''\n", hits[2].c_str()); return 7; }

  // EN: A click outside any element must NOT invoke the callback (no 4th hit).
  // PT: Um clique fora de qualquer elemento NÃO deve invocar o callback (sem 4º hit).
  click_at(ui, 250.f, 190.f);
  if (hits.size() != 3) { std::fprintf(stderr, "FAIL: click outside body added a hit\n"); return 8; }

  std::puts("click_callback_sanity: PASS");
  return 0;
}
