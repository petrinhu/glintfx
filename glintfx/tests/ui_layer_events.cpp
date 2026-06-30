// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test for UiLayer event injection (T4 — embed mode).
//     The HOST owns the GL context (hidden GLFW window standing in for SDL3 in GusWorld).
//     Verifies: injecting a representative sequence of UiEvents (MouseMove, MouseButton,
//     Key navigation via gamepad-style Key::Down, Resize) does not crash and leaves
//     ok() true. Focus-movement assertions are deferred to F2, when focusable components exist.
// PT: Teste de fumaça para injeção de eventos no UiLayer (T4 — embed mode).
//     O HOST é dono do contexto GL (janela GLFW oculta representando o SDL3 no GusWorld).
//     Verifica: injetar sequência representativa de UiEvents (MouseMove, MouseButton,
//     navegação por Key via gamepad Key::Down, Resize) não crasha e mantém ok() true.
//     Asserções de movimento de foco ficam para F2, quando existirem componentes focáveis.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>

int main() {
  // EN: Host creates a hidden GL context — stands in for SDL3 in GusWorld.
  // PT: Host cria contexto GL oculto — representa o SDL3 no GusWorld.
  glintfx::WindowGlfw host;
  if (!host.create("host-events", 800, 600)) {
    std::puts("FAIL: host window create");
    return 1;
  }

  // EN: UiLayer attaches to the current GL context (already made current by WindowGlfw::create).
  // PT: UiLayer anexa ao contexto GL corrente (tornado corrente por WindowGlfw::create).
  glintfx::UiLayer ui({ .logical_width = 800, .logical_height = 600, .load_gl = true });
  if (!ui.ok()) {
    std::puts("FAIL: ui attach");
    return 2;
  }

  // EN: Load a minimal document so the RmlUi context has a document tree to process events against.
  // PT: Carrega documento mínimo para que o contexto RmlUi tenha uma árvore de documento para eventos.
  ui.load("tests/min.rml");

  // ---------------------------------------------------------------------------
  // EN: Inject representative event sequence — anti-flaky: asserts no crash + ok() stays true.
  // PT: Injeta sequência representativa de eventos — anti-flaky: sem crash + ok() permanece true.
  // ---------------------------------------------------------------------------

  // EN: Mouse move — pointer at (100, 80).
  // PT: Movimento do mouse — ponteiro em (100, 80).
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 100.f, .y = 80.f });

  // EN: Left button press.
  // PT: Pressão do botão esquerdo.
  ui.process_event({ .type    = glintfx::UiEvent::Type::MouseButton,
                     .button  = 0,
                     .pressed = true });

  // EN: Left button release.
  // PT: Liberação do botão esquerdo.
  ui.process_event({ .type    = glintfx::UiEvent::Type::MouseButton,
                     .button  = 0,
                     .pressed = false });

  // EN: Gamepad navigation — Key::Down (d-pad down / arrow down) → RmlUi KI_DOWN.
  // PT: Navegação por gamepad — Key::Down (d-pad baixo / seta abaixo) → RmlUi KI_DOWN.
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = true,
                     .key     = glintfx::Key::Down });
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = false,
                     .key     = glintfx::Key::Down });

  // EN: Key::Enter (d-pad confirm / Return) pressed and released.
  // PT: Key::Enter (d-pad confirmar / Return) pressionado e liberado.
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = true,
                     .key     = glintfx::Key::Enter });
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = false,
                     .key     = glintfx::Key::Enter });

  // EN: Key::Escape (back / cancel).
  // PT: Key::Escape (voltar / cancelar).
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = true,
                     .key     = glintfx::Key::Escape });
  ui.process_event({ .type    = glintfx::UiEvent::Type::Key,
                     .pressed = false,
                     .key     = glintfx::Key::Escape });

  // EN: Viewport resize — common when host window resizes (e.g. SDL2_WINDOWEVENT_RESIZED).
  // PT: Redimensionamento de viewport — comum quando janela do host redimensiona.
  ui.process_event({ .type   = glintfx::UiEvent::Type::Resize,
                     .width  = 800,
                     .height = 600 });

  // EN: Drive one full frame — update + compose-render on top of the host backbuffer.
  // PT: Executa um frame completo — update + compose-render sobre o backbuffer do host.
  ui.update();
  ui.render();

  // EN: Core assertion: the layer must still be healthy after the event sequence.
  // PT: Asserção central: a camada deve continuar saudável após a sequência de eventos.
  if (!ui.ok()) {
    std::puts("FAIL: ok() false after event sequence");
    return 3;
  }

  std::puts("ui_layer_events OK");
  return 0;
}
