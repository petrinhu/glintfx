// SPDX-License-Identifier: MPL-2.0
// EN: HOSTIN-1 (Onda 2, framework-2D, docs/superpowers/plans/2026-07-22-onda2-input-host.md,
//     decision D2) -- pure, headless-testable physical input state table. Zero GLFW, zero I/O,
//     zero side effect: WindowGlfw owns exactly one instance and feeds it from its own GLFW
//     callback tap (handle_key/handle_mouse_button/handle_cursor_pos, window_glfw.cpp) BEFORE
//     dispatching to the UiEvent sink, so a host callback that polls is_key_down() mid-frame
//     sees state that is already fresh. The SYNTHETIC channel (App::process_event()) never
//     writes here (D5) -- physical and synthetic input stay two separate, non-lying channels.
//     Testable directly, no window/display required: see tests/input_state_sanity.cpp.
// PT: HOSTIN-1 (Onda 2, framework-2D, docs/superpowers/plans/2026-07-22-onda2-input-host.md,
//     decisão D2) -- tabela de estado de input físico pura, testável headless. Zero GLFW, zero
//     I/O, zero efeito colateral: WindowGlfw possui exatamente uma instância e a alimenta a
//     partir do próprio tap de callback GLFW (handle_key/handle_mouse_button/handle_cursor_pos,
//     window_glfw.cpp) ANTES de despachar ao sink de UiEvent, então um callback do host que
//     consulta is_key_down() no meio do frame vê estado já fresco. O canal SINTÉTICO
//     (App::process_event()) nunca escreve aqui (D5) -- input físico e sintético permanecem
//     dois canais separados, que não mentem um pro outro. Testável direto, sem janela/display
//     nenhum: ver tests/input_state_sanity.cpp.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <glintfx/ui_event.hpp>
#include <cstddef>

namespace glintfx {

// EN: Fixed-size, fail-high input state table -- same discipline as every other module in this
//     library (Gamepads, Audio; see gamepad.hpp's own header comment): every accessor below
//     treats an out-of-range index (an invalid Key cast, a mouse button outside [0,7]) as "not
//     held" (false), NEVER undefined behaviour.
// PT: Tabela de estado de input de tamanho fixo, fail-high -- mesma disciplina de todo outro
//     módulo desta biblioteca (Gamepads, Audio; ver o próprio comentário de cabeçalho de
//     gamepad.hpp): todo acessor abaixo trata um índice fora de faixa (um cast de Key inválido,
//     um botão de mouse fora de [0,7]) como "não segurado" (false), NUNCA comportamento
//     indefinido.
class InputState {
public:
  // EN: One slot per Key enumerator, including Key::None (index 0), up to but EXCLUDING
  //     Key::Count -- the sentinel itself is never a valid index (see Key::Count's own
  //     doc-comment, ui_event.hpp: "Count nunca emitido").
  // PT: Um slot por enumerador de Key, incluindo Key::None (índice 0), até mas EXCLUINDO
  //     Key::Count -- o próprio sentinela nunca é um índice válido (ver o próprio doc-comment de
  //     Key::Count, ui_event.hpp: "Count nunca emitido").
  static constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::Count);
  // EN: GLFW's own honest ceiling (D3, memory feedback_input_multibutton_teclados) --
  //     GLFW_MOUSE_BUTTON_LAST == GLFW_MOUSE_BUTTON_8 == 7, so [0, 8) covers everything GLFW
  //     itself can ever report. A 12-button mouse's extra buttons never reach this table (see
  //     App::is_mouse_button_down's doc-comment, app.hpp, for the seeded evdev follow-up).
  // PT: O próprio teto honesto do GLFW (D3, memória feedback_input_multibutton_teclados) --
  //     GLFW_MOUSE_BUTTON_LAST == GLFW_MOUSE_BUTTON_8 == 7, então [0, 8) cobre tudo que o
  //     próprio GLFW pode reportar. Os botões extras de um mouse de 12 botões nunca alcançam
  //     esta tabela (ver o próprio doc-comment de App::is_mouse_button_down, app.hpp, pro
  //     desdobramento evdev semeado).
  static constexpr int kMouseButtonCount = 8;

  // EN: Sets/clears the level state of `k`. Silently ignored (no-op) for an out-of-range
  //     enumerator (e.g. an invalid static_cast, or Key::Count itself) -- never indexes past
  //     the backing array.
  // PT: Define/limpa o estado de nível de `k`. Ignorado silenciosamente (no-op) para um
  //     enumerador fora de faixa (ex.: um static_cast inválido, ou o próprio Key::Count) --
  //     nunca indexa além do array de apoio.
  void set_key_down(Key k, bool down) noexcept {
    const auto idx = static_cast<std::size_t>(k);
    if (idx >= kKeyCount) return;
    keys_[idx] = down;
  }

  // EN: false for an out-of-range enumerator -- fail-high, see the class-level comment above.
  // PT: false para um enumerador fora de faixa -- fail-high, ver o comentário de nível de
  //     classe acima.
  bool is_key_down(Key k) const noexcept {
    const auto idx = static_cast<std::size_t>(k);
    if (idx >= kKeyCount) return false;
    return keys_[idx];
  }

  // EN: `button` outside [0, kMouseButtonCount) is silently ignored (D3's honest GLFW ceiling).
  // PT: `button` fora de [0, kMouseButtonCount) é ignorado silenciosamente (teto honesto do
  //     GLFW do D3).
  void set_mouse_button_down(int button, bool down) noexcept {
    if (button < 0 || button >= kMouseButtonCount) return;
    mouse_buttons_[button] = down;
  }

  // EN: false for `button` outside [0, kMouseButtonCount) -- fail-high.
  // PT: false para `button` fora de [0, kMouseButtonCount) -- fail-high.
  bool is_mouse_button_down(int button) const noexcept {
    if (button < 0 || button >= kMouseButtonCount) return false;
    return mouse_buttons_[button];
  }

  // EN: Overwrites the last-known cursor position. Coordinate space is whatever the caller
  //     feeds it (WindowGlfw feeds framebuffer pixels -- see get_cursor_pos's doc-comment on
  //     App/app.hpp for the full contract); this struct itself has no opinion on units.
  // PT: Sobrescreve a última posição conhecida do cursor. Espaço de coordenadas é o que o
  //     chamador alimentar (WindowGlfw alimenta pixels de framebuffer -- ver o doc-comment de
  //     get_cursor_pos em App/app.hpp pro contrato completo); esta struct em si não tem opinião
  //     sobre unidades.
  void set_cursor_pos(float x, float y) noexcept {
    cursor_x_ = x;
    cursor_y_ = y;
  }

  // EN: (0, 0) before the first cursor-position update this instance has ever seen.
  // PT: (0, 0) antes da primeira atualização de posição de cursor que esta instância já viu.
  void get_cursor_pos(float& out_x, float& out_y) const noexcept {
    out_x = cursor_x_;
    out_y = cursor_y_;
  }

private:
  bool keys_[kKeyCount] = {};
  bool mouse_buttons_[kMouseButtonCount] = {};
  float cursor_x_ = 0.f;
  float cursor_y_ = 0.f;
};

} // namespace glintfx
