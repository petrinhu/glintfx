// SPDX-License-Identifier: MPL-2.0
// EN: App-level smoke test for the HOSTIN-1..4 physical input surface (Onda 2, v0.19.0 --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, section 4.2). Runs under Xvfb
//     (GLFW backend, real window/GL context, no real keyboard/mouse events -- that leg is
//     manual, section 4.3 of the plan). Covers:
//
//       1. A fresh App answers is_key_down/is_mouse_button_down/get_cursor_pos as
//          false/false/(0,0) -- nothing has ever touched the physical state table.
//       2. Out-of-range Key/button queries stay false (fail-high), same as a fresh App.
//       3. Callbacks (set_key_callback, set_close_request_callback, set_window_focus_callback,
//          set_window_iconify_callback) are registerable both BEFORE and AFTER load(), and a
//          null/empty callback is a safe no-op (clearing a previously-set one).
//       4. process_event() (the SYNTHETIC channel) does NOT write into the physical state table
//          (D5) -- a synthetic Key/MouseButton event must leave is_key_down/
//          is_mouse_button_down exactly as they were.
//       5. request_close() makes running() false immediately, and run() (poll+update+render in
//          a loop) returns without hanging -- this is the bounded, no-timeout-race proof AUD-
//          TEC-7 asked for (run() itself was previously untestable; request_close() finally
//          gives it one).
//
//     E2E DOWNGRADE (recorded honestly per the plan's own time-box escape hatch, section 4.2):
//     the close-VETO round-trip via a real WM_DELETE_WINDOW (XSendEvent/GLFW_EXPOSE_NATIVE_X11)
//     is NOT exercised here. The pure decision seam it would prove (glfw_decide_window_close)
//     is already pinned in tests/glfw_event_translate_sanity.cpp, and the real end-to-end
//     (a genuine X-button click reaching the veto callback) is covered by the nested-compositor
//     manual QA leg (docs/superpowers/plans/2026-07-22-onda2-input-host.md section 4.3) instead
//     -- Xvfb has no window manager to generate a real close request through, so a from-scratch
//     XSendEvent harness would be testing X11 client-message plumbing, not glintfx's own logic.
// PT: Teste de fumaça em nível de App para a superfície de input físico HOSTIN-1..4 (Onda 2,
//     v0.19.0 -- docs/superpowers/plans/2026-07-22-onda2-input-host.md, seção 4.2). Roda sob
//     Xvfb (backend GLFW, janela/contexto GL reais, sem eventos reais de teclado/mouse -- essa
//     perna é manual, seção 4.3 do plano). Cobre:
//
//       1. Um App recém-criado responde is_key_down/is_mouse_button_down/get_cursor_pos como
//          false/false/(0,0) -- nada nunca tocou a tabela de estado físico.
//       2. Consultas de Key/botão fora de faixa continuam false (fail-high), igual a um App
//          recém-criado.
//       3. Callbacks (set_key_callback, set_close_request_callback,
//          set_window_focus_callback, set_window_iconify_callback) são registráveis tanto
//          ANTES quanto DEPOIS do load(), e um callback nulo/vazio é um no-op seguro (limpando
//          um previamente setado).
//       4. process_event() (o canal SINTÉTICO) NÃO escreve na tabela de estado físico (D5) --
//          um evento sintético Key/MouseButton precisa deixar is_key_down/is_mouse_button_down
//          exatamente como estavam.
//       5. request_close() faz running() virar false imediatamente, e run() (poll+update+render
//          em laço) retorna sem travar -- esta é a prova limitada, sem corrida de timeout, que
//          o AUD-TEC-7 pedia (o próprio run() era antes intestável; request_close() finalmente
//          dá um teste real a ele).
//
//     DOWNGRADE E2E (registrado honestamente conforme a própria válvula de escape de time-box
//     do plano, seção 4.2): o ida-e-volta do VETO de close via um WM_DELETE_WINDOW real
//     (XSendEvent/GLFW_EXPOSE_NATIVE_X11) NÃO é exercitado aqui. O seam de decisão puro que ele
//     provaria (glfw_decide_window_close) já está fixado em
//     tests/glfw_event_translate_sanity.cpp, e o ponta-a-ponta de verdade (um clique real no
//     botão X alcançando o callback de veto) é coberto pela perna manual de QA em compositor
//     aninhado (docs/superpowers/plans/2026-07-22-onda2-input-host.md seção 4.3) em vez disso
//     -- o Xvfb não tem window manager pra gerar um pedido de close de verdade através, então
//     um harness XSendEvent do zero estaria testando a plomberia de client-message do X11, não
//     a própria lógica da glintfx.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <limits>

using glintfx::App;
using glintfx::AppConfig;
using glintfx::Key;
using glintfx::KeyAction;
using glintfx::UiEvent;

int main() {
  AppConfig cfg;
  cfg.title = "app_input_query_smoke";
  cfg.width = 320;
  cfg.height = 240;

  App app(cfg);
  if (!app.ok()) {
    std::puts("app_input_query_smoke FAIL: App init failed");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: 1/2 -- fresh App: nothing physical has happened yet, so every query answers the
  //     "never touched" default, INCLUDING out-of-range queries (fail-high, D3).
  // PT: 1/2 -- App recém-criado: nada físico aconteceu ainda, então toda consulta responde o
  //     default "nunca tocado", INCLUSIVE consultas fora de faixa (fail-high, D3).
  // ---------------------------------------------------------------------------
  if (app.is_key_down(Key::W)) {
    std::puts("app_input_query_smoke FAIL: fresh App reports Key::W down");
    return 2;
  }
  if (app.is_mouse_button_down(0)) {
    std::puts("app_input_query_smoke FAIL: fresh App reports mouse button 0 down");
    return 3;
  }
  if (app.is_mouse_button_down(8) || app.is_mouse_button_down(-1) ||
      app.is_mouse_button_down(std::numeric_limits<int>::max())) {
    std::puts("app_input_query_smoke FAIL: out-of-range mouse button must be false (fail-high)");
    return 4;
  }
  {
    float x = -1.f, y = -1.f;
    app.get_cursor_pos(x, y);
    if (x != 0.f || y != 0.f) {
      std::puts("app_input_query_smoke FAIL: fresh App cursor pos must be (0,0)");
      return 5;
    }
  }

  // ---------------------------------------------------------------------------
  // EN: 3 -- callbacks registerable BEFORE load(), null callback is a safe no-op afterwards.
  // PT: 3 -- callbacks registráveis ANTES do load(), callback nulo é um no-op seguro depois.
  // ---------------------------------------------------------------------------
  app.set_key_callback([](Key, KeyAction, int) {});
  app.set_close_request_callback([]() { return true; });
  app.set_window_focus_callback([](bool) {});
  app.set_window_iconify_callback([](bool) {});
  if (!app.ok()) {
    std::puts("app_input_query_smoke FAIL: ok() false after registering callbacks pre-load()");
    return 6;
  }

  app.load("tests/min.rml");

  // EN: 3 (cont.) -- re-registerable AFTER load() too; null clears a previous registration.
  // PT: 3 (cont.) -- re-registrável DEPOIS do load() também; nulo limpa um registro anterior.
  app.set_key_callback(nullptr);
  app.set_close_request_callback(nullptr);
  app.set_window_focus_callback(nullptr);
  app.set_window_iconify_callback(nullptr);
  if (!app.ok()) {
    std::puts("app_input_query_smoke FAIL: ok() false after clearing callbacks post-load()");
    return 7;
  }

  // ---------------------------------------------------------------------------
  // EN: 4 -- D5: process_event() (synthetic channel) must NOT write the physical state table.
  //     A synthetic Key press + MouseButton press must leave is_key_down/
  //     is_mouse_button_down exactly as before (false, since no PHYSICAL event ever fired).
  // PT: 4 -- D5: process_event() (canal sintético) NÃO PODE escrever na tabela de estado
  //     físico. Um press sintético de Key + MouseButton precisa deixar is_key_down/
  //     is_mouse_button_down exatamente como antes (false, já que nenhum evento FÍSICO disparou
  //     alguma vez).
  // ---------------------------------------------------------------------------
  {
    UiEvent key_ev{};
    key_ev.type = UiEvent::Type::Key;
    key_ev.key = Key::W;
    key_ev.pressed = true;
    app.process_event(key_ev);
    if (app.is_key_down(Key::W)) {
      std::puts("app_input_query_smoke FAIL: synthetic Key event leaked into physical is_key_down (D5 violated)");
      return 8;
    }

    UiEvent btn_ev{};
    btn_ev.type = UiEvent::Type::MouseButton;
    btn_ev.button = 0;
    btn_ev.pressed = true;
    app.process_event(btn_ev);
    if (app.is_mouse_button_down(0)) {
      std::puts("app_input_query_smoke FAIL: synthetic MouseButton event leaked into physical state (D5 violated)");
      return 9;
    }
  }

  app.poll_events();
  app.update();
  app.render();

  // ---------------------------------------------------------------------------
  // EN: 5 -- request_close(): running() must become false immediately, and run() (which loops
  //     poll_events()->update()->render() while running()) must return without hanging --
  //     bounded by construction (no external quit signal or timeout race needed here, since
  //     request_close() is exactly the "quit signal" AUD-TEC-7's own doc-comment (app.hpp)
  //     names as the missing piece).
  // PT: 5 -- request_close(): running() precisa virar false imediatamente, e run() (que laça
  //     poll_events()->update()->render() enquanto running()) precisa retornar sem travar --
  //     limitado por construção (nenhum sinal externo de saída ou corrida de timeout necessário
  //     aqui, já que request_close() é exatamente o "sinal de saída" que o próprio doc-comment
  //     do AUD-TEC-7 (app.hpp) nomeia como a peça faltante).
  // ---------------------------------------------------------------------------
  app.request_close();
  if (app.running()) {
    std::puts("app_input_query_smoke FAIL: running() must be false right after request_close()");
    return 10;
  }
  app.run(); // EN: must return immediately -- no hang. PT: precisa retornar imediatamente -- sem travar.

  std::puts("app_input_query_smoke: PASS");
  return 0;
}
