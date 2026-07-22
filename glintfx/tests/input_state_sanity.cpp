// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for src/input_state.hpp (HOSTIN-1, Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, sections D2/D3/4). No window, no
//     GL context, no glfwInit() anywhere in this file -- InputState is a plain struct with no
//     GLFW dependency at all. Exercises press/release/edge-vs-level semantics, the [0,7] mouse
//     button range (D3, honest GLFW ceiling), cursor position round-trip, and hostile inputs
//     (out-of-range Key cast, negative/huge button index) -- all must degrade to false/(0,0),
//     never UB (fail-high, same discipline as gamepad.hpp/audio.hpp).
// PT: Teste unit puro para src/input_state.hpp (HOSTIN-1, Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, seções D2/D3/D4). Sem janela, sem
//     contexto GL, sem glfwInit() nenhum neste arquivo -- InputState é uma struct simples sem
//     dependência de GLFW alguma. Exercita semântica de press/release/nível-vs-borda, a faixa
//     [0,7] de botão de mouse (D3, teto honesto do GLFW), round-trip de posição de cursor, e
//     inputs hostis (cast de Key fora de faixa, índice de botão negativo/enorme) -- todos
//     precisam degradar para false/(0,0), nunca UB (fail-high, mesma disciplina de
//     gamepad.hpp/audio.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/input_state.hpp"
#include <cstdio>
#include <cmath>
#include <limits>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: Fresh InputState -- everything false/(0,0) before any set_* call ever happens.
  // PT: InputState recém-criado -- tudo false/(0,0) antes de qualquer chamada set_* acontecer.
  // ---------------------------------------------------------------------------
  {
    InputState st;
    check(!st.is_key_down(Key::W), "fresh InputState: Key::W starts up");
    check(!st.is_key_down(Key::None), "fresh InputState: Key::None starts up");
    check(!st.is_mouse_button_down(0), "fresh InputState: button 0 starts up");
    float x = -1.f, y = -1.f;
    st.get_cursor_pos(x, y);
    check(x == 0.f && y == 0.f, "fresh InputState: cursor starts at (0,0)");
  }

  // ---------------------------------------------------------------------------
  // EN: Level state -- press sets it, release clears it. Spot-checked across the vocabulary:
  //     a pre-existing nav key AND a HOSTIN-1 addition (letters, digits, modifiers).
  // PT: Estado de nível -- press seta, release limpa. Checado em pontos representativos do
  //     vocabulário: uma tecla de nav pré-existente E uma adição do HOSTIN-1 (letras, dígitos,
  //     modificadores).
  // ---------------------------------------------------------------------------
  {
    InputState st;
    st.set_key_down(Key::Up, true);
    check(st.is_key_down(Key::Up), "Key::Up: press -> down");
    st.set_key_down(Key::Up, false);
    check(!st.is_key_down(Key::Up), "Key::Up: release -> up");

    st.set_key_down(Key::W, true);
    check(st.is_key_down(Key::W), "Key::W (HOSTIN-1): press -> down");
    check(!st.is_key_down(Key::A), "Key::W held does not leak into Key::A (independent slots)");
    st.set_key_down(Key::W, false);
    check(!st.is_key_down(Key::W), "Key::W: release -> up");

    st.set_key_down(Key::LeftShift, true);
    check(st.is_key_down(Key::LeftShift), "Key::LeftShift: press -> down");
    check(!st.is_key_down(Key::RightShift), "LeftShift does not alias RightShift (D1 distinct slots)");

    st.set_key_down(Key::Kp0, true);
    check(st.is_key_down(Key::Kp0), "Key::Kp0: press -> down");
    check(!st.is_key_down(Key::Digit0), "Kp0 does not alias Digit0 (distinct slots)");
  }

  // ---------------------------------------------------------------------------
  // EN: D2 -- level, not edge: setting the same key down twice in a row is idempotent (mirrors
  //     GLFW_REPEAT not re-setting an already-down key at the WindowGlfw call site -- this test
  //     proves the STATE itself tolerates that, independent of window_glfw.cpp's own repeat
  //     guard).
  // PT: D2 -- nível, não borda: setar a mesma tecla como down duas vezes seguidas é idempotente
  //     (espelha o GLFW_REPEAT não re-setar uma tecla já segurada no call site de WindowGlfw --
  //     este teste prova que o próprio ESTADO tolera isso, independente do guard de repeat do
  //     próprio window_glfw.cpp).
  // ---------------------------------------------------------------------------
  {
    InputState st;
    st.set_key_down(Key::Space, true);
    st.set_key_down(Key::Space, true); // EN: repeat-shaped re-set. PT: re-set com formato de repeat.
    check(st.is_key_down(Key::Space), "Key::Space: redundant press stays down (level state)");
  }

  // ---------------------------------------------------------------------------
  // EN: HOSTILE -- an out-of-range Key (an invalid static_cast, and the Count sentinel itself)
  //     must be ignored on write and answer false on read, never touching the array out of
  //     bounds (ASan would catch a real OOB write/read here).
  // PT: HOSTIL -- uma Key fora de faixa (um static_cast inválido, e o próprio sentinela Count)
  //     precisa ser ignorada na escrita e responder false na leitura, nunca tocando o array fora
  //     de faixa (o ASan pegaria uma escrita/leitura OOB real aqui).
  // ---------------------------------------------------------------------------
  {
    InputState st;
    const Key huge = static_cast<Key>(1'000'000);
    const Key neg = static_cast<Key>(-1);
    st.set_key_down(huge, true);
    st.set_key_down(neg, true);
    st.set_key_down(Key::Count, true);
    check(!st.is_key_down(huge), "hostile Key (huge cast) -> is_key_down false, no crash");
    check(!st.is_key_down(neg), "hostile Key (negative cast) -> is_key_down false, no crash");
    check(!st.is_key_down(Key::Count), "Key::Count sentinel -> is_key_down false (never a real key)");
  }

  // ---------------------------------------------------------------------------
  // EN: D3 -- mouse buttons [0, 7], NOT restricted to the UI route's 3-button contract. All 8
  //     GLFW-reportable buttons are independently tracked; out-of-range (8, negative, INT_MAX)
  //     is ignored on write and false on read.
  // PT: D3 -- botões de mouse [0, 7], NÃO restritos ao contrato de 3 botões da rota de UI. Os 8
  //     botões reportáveis pelo GLFW são rastreados independentemente; fora de faixa (8,
  //     negativo, INT_MAX) é ignorado na escrita e false na leitura.
  // ---------------------------------------------------------------------------
  {
    InputState st;
    for (int b = 0; b < InputState::kMouseButtonCount; ++b) {
      check(!st.is_mouse_button_down(b), "button starts up (pre-loop sanity)");
    }
    st.set_mouse_button_down(0, true);
    st.set_mouse_button_down(7, true); // EN: GLFW_MOUSE_BUTTON_LAST. PT: GLFW_MOUSE_BUTTON_LAST.
    check(st.is_mouse_button_down(0), "button 0 (left): press -> down");
    check(st.is_mouse_button_down(7), "button 7 (GLFW's own ceiling, D3): press -> down");
    check(!st.is_mouse_button_down(1), "button 1 untouched by button 0/7 (independent slots)");
    st.set_mouse_button_down(0, false);
    check(!st.is_mouse_button_down(0), "button 0: release -> up");

    st.set_mouse_button_down(8, true); // EN: one past GLFW's ceiling. PT: um além do teto do GLFW.
    st.set_mouse_button_down(-1, true);
    st.set_mouse_button_down(std::numeric_limits<int>::max(), true);
    check(!st.is_mouse_button_down(8), "hostile button 8 (one past GLFW ceiling) -> false, no crash");
    check(!st.is_mouse_button_down(-1), "hostile button -1 -> false, no crash");
    check(!st.is_mouse_button_down(std::numeric_limits<int>::max()), "hostile button INT_MAX -> false, no crash");
  }

  // ---------------------------------------------------------------------------
  // EN: Cursor position -- plain round-trip, including negative coordinates (a cursor can sit
  //     outside the window's own bounds transiently, e.g. mid-drag past the border).
  // PT: Posição de cursor -- round-trip simples, incluindo coordenadas negativas (um cursor
  //     pode estar fora dos limites da própria janela transitoriamente, ex.: no meio de um
  //     arrasto além da borda).
  // ---------------------------------------------------------------------------
  {
    InputState st;
    st.set_cursor_pos(123.5f, 456.25f);
    float x = 0.f, y = 0.f;
    st.get_cursor_pos(x, y);
    check(std::fabs(x - 123.5f) < 0.001f && std::fabs(y - 456.25f) < 0.001f, "cursor round-trips exactly");

    st.set_cursor_pos(-10.f, -20.f);
    st.get_cursor_pos(x, y);
    check(std::fabs(x - (-10.f)) < 0.001f && std::fabs(y - (-20.f)) < 0.001f, "cursor round-trips negative coordinates");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "input_state_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("input_state_sanity: PASS");
  return 0;
}
