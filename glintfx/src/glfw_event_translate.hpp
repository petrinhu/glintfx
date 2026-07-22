// SPDX-License-Identifier: MPL-2.0
// EN: Pure GLFW-raw-value -> glintfx::UiEvent translation functions (A1, framework-2D,
//     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md section 2.2). Every function
//     here takes only ints/doubles/GLFW #define constants and returns a filled-in piece of a
//     UiEvent -- ZERO GLFWwindow*, ZERO callback registration, ZERO side effect. That is what
//     makes them testable without ever creating a window (see
//     tests/glfw_event_translate_sanity.cpp, the "GLFW disponível sem display" unit lane).
//     window_glfw.cpp's 5 GLFW callback trampolines are one-line wrappers over these -- the
//     GLFW-specific plumbing (glfwSetWindowUserPointer, the sink std::function) lives there,
//     never here.
//     Private header (glintfx/src/, never installed) -- the neutral UiEvent/Key types these
//     functions produce are the only thing that crosses back into the public API; no GLFW
//     type ever does (tools/check_encapsulation.sh enforces this at glintfx/include/glintfx/).
// PT: Funções puras de tradução valor-cru-GLFW -> glintfx::UiEvent (A1, framework-2D,
//     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md seção 2.2). Toda função aqui
//     recebe só ints/doubles/constantes #define do GLFW e devolve um pedaço preenchido de
//     UiEvent -- ZERO GLFWwindow*, ZERO registro de callback, ZERO efeito colateral. É isso
//     que as torna testáveis sem nunca criar uma janela (ver
//     tests/glfw_event_translate_sanity.cpp, a pista unit "GLFW disponível sem display").
//     Os 5 trampolins de callback GLFW em window_glfw.cpp são wrappers de uma linha sobre
//     estas funções -- a plomberia específica do GLFW (glfwSetWindowUserPointer, o
//     std::function do sink) mora lá, nunca aqui.
//     Header privado (glintfx/src/, nunca instalado) -- os tipos neutros UiEvent/Key que estas
//     funções produzem são a única coisa que cruza de volta para a API pública; nenhum tipo
//     GLFW cruza (tools/check_encapsulation.sh enforça isto em glintfx/include/glintfx/).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <GLFW/glfw3.h>
#include <glintfx/ui_event.hpp>

namespace glintfx {

// EN: GLFW key code -> glintfx::Key. Navigation-oriented subset (mirrors UiLayer's own
//     to_rml_key/Key enum -- see ui_event.hpp). Returns false for any glfw_key outside the
//     supported subset (out_key left untouched) -- the CALLER must not emit a Key event in
//     that case (never synthesise Key::None; an inert-but-emitted event is a different
//     contract than "nothing happened", see UiEvent::Type::None's doc-comment). Both
//     GLFW_KEY_ENTER and GLFW_KEY_KP_ENTER (numpad Enter) map to Key::Enter.
// PT: Código de tecla GLFW -> glintfx::Key. Subconjunto orientado a navegação (espelha o
//     próprio to_rml_key/enum Key do UiLayer -- ver ui_event.hpp). Retorna false para
//     qualquer glfw_key fora do subconjunto suportado (out_key intocado) -- o CHAMADOR não
//     deve emitir um evento Key nesse caso (nunca sintetizar Key::None; um evento inerte-mas-
//     emitido é um contrato diferente de "nada aconteceu", ver o doc-comment de
//     UiEvent::Type::None). Tanto GLFW_KEY_ENTER quanto GLFW_KEY_KP_ENTER (Enter do numpad)
//     mapeiam para Key::Enter.
inline bool glfw_translate_key(int glfw_key, Key& out_key) noexcept {
  switch (glfw_key) {
    case GLFW_KEY_UP:        out_key = Key::Up;        return true;
    case GLFW_KEY_DOWN:      out_key = Key::Down;      return true;
    case GLFW_KEY_LEFT:      out_key = Key::Left;      return true;
    case GLFW_KEY_RIGHT:     out_key = Key::Right;     return true;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:  out_key = Key::Enter;     return true;
    case GLFW_KEY_ESCAPE:    out_key = Key::Escape;    return true;
    case GLFW_KEY_TAB:       out_key = Key::Tab;       return true;
    case GLFW_KEY_SPACE:     out_key = Key::Space;     return true;
    case GLFW_KEY_BACKSPACE: out_key = Key::Backspace; return true;
    case GLFW_KEY_DELETE:    out_key = Key::Delete;    return true;
    case GLFW_KEY_HOME:      out_key = Key::Home;      return true;
    case GLFW_KEY_END:       out_key = Key::End;       return true;
    case GLFW_KEY_PAGE_UP:   out_key = Key::PageUp;    return true;
    case GLFW_KEY_PAGE_DOWN: out_key = Key::PageDown;  return true;

    // EN: HOSTIN-1 (Onda 2, v0.19.0, D1) -- append-only extension mirroring GLFW's own nameable
    //     key set, for App's PHYSICAL input state channel (is_key_down/set_key_callback). These
    //     are POSITION tokens (named after the US-layout physical key), not label tokens -- see
    //     Key::Count's own doc-comment (ui_event.hpp) for the ABNT2 consequences (ç arrives as
    //     Semicolon; the ABNT2 second "/" -- scancode 97 -- has no GLFW token, a documented
    //     upstream gap, seeded as SEED-SCANCODE/is_scancode_down). Unmapped by design: anything
    //     GLFW itself reports as GLFW_KEY_UNKNOWN (no case matches, falls to default below).
    // PT: HOSTIN-1 (Onda 2, v0.19.0, D1) -- extensão append-only espelhando o próprio conjunto
    //     nomeável de teclas do GLFW, para o canal de estado de input FÍSICO do App
    //     (is_key_down/set_key_callback). Estes são tokens de POSIÇÃO (nomeados pela tecla
    //     física do layout US), não de rótulo -- ver o próprio doc-comment de Key::Count
    //     (ui_event.hpp) para as consequências no ABNT2 (ç chega como Semicolon; a segunda "/"
    //     do ABNT2 -- scancode 97 -- não tem token GLFW, uma lacuna upstream documentada,
    //     semeada como SEED-SCANCODE/is_scancode_down). Não mapeado por desenho: o que o
    //     próprio GLFW reporta como GLFW_KEY_UNKNOWN (nenhum case casa, cai no default abaixo).
    case GLFW_KEY_A:
      out_key = Key::A;
      return true;
    case GLFW_KEY_B:
      out_key = Key::B;
      return true;
    case GLFW_KEY_C:
      out_key = Key::C;
      return true;
    case GLFW_KEY_D:
      out_key = Key::D;
      return true;
    case GLFW_KEY_E:
      out_key = Key::E;
      return true;
    case GLFW_KEY_F:
      out_key = Key::F;
      return true;
    case GLFW_KEY_G:
      out_key = Key::G;
      return true;
    case GLFW_KEY_H:
      out_key = Key::H;
      return true;
    case GLFW_KEY_I:
      out_key = Key::I;
      return true;
    case GLFW_KEY_J:
      out_key = Key::J;
      return true;
    case GLFW_KEY_K:
      out_key = Key::K;
      return true;
    case GLFW_KEY_L:
      out_key = Key::L;
      return true;
    case GLFW_KEY_M:
      out_key = Key::M;
      return true;
    case GLFW_KEY_N:
      out_key = Key::N;
      return true;
    case GLFW_KEY_O:
      out_key = Key::O;
      return true;
    case GLFW_KEY_P:
      out_key = Key::P;
      return true;
    case GLFW_KEY_Q:
      out_key = Key::Q;
      return true;
    case GLFW_KEY_R:
      out_key = Key::R;
      return true;
    case GLFW_KEY_S:
      out_key = Key::S;
      return true;
    case GLFW_KEY_T:
      out_key = Key::T;
      return true;
    case GLFW_KEY_U:
      out_key = Key::U;
      return true;
    case GLFW_KEY_V:
      out_key = Key::V;
      return true;
    case GLFW_KEY_W:
      out_key = Key::W;
      return true;
    case GLFW_KEY_X:
      out_key = Key::X;
      return true;
    case GLFW_KEY_Y:
      out_key = Key::Y;
      return true;
    case GLFW_KEY_Z:
      out_key = Key::Z;
      return true;

    case GLFW_KEY_0:
      out_key = Key::Digit0;
      return true;
    case GLFW_KEY_1:
      out_key = Key::Digit1;
      return true;
    case GLFW_KEY_2:
      out_key = Key::Digit2;
      return true;
    case GLFW_KEY_3:
      out_key = Key::Digit3;
      return true;
    case GLFW_KEY_4:
      out_key = Key::Digit4;
      return true;
    case GLFW_KEY_5:
      out_key = Key::Digit5;
      return true;
    case GLFW_KEY_6:
      out_key = Key::Digit6;
      return true;
    case GLFW_KEY_7:
      out_key = Key::Digit7;
      return true;
    case GLFW_KEY_8:
      out_key = Key::Digit8;
      return true;
    case GLFW_KEY_9:
      out_key = Key::Digit9;
      return true;

    case GLFW_KEY_F1:
      out_key = Key::F1;
      return true;
    case GLFW_KEY_F2:
      out_key = Key::F2;
      return true;
    case GLFW_KEY_F3:
      out_key = Key::F3;
      return true;
    case GLFW_KEY_F4:
      out_key = Key::F4;
      return true;
    case GLFW_KEY_F5:
      out_key = Key::F5;
      return true;
    case GLFW_KEY_F6:
      out_key = Key::F6;
      return true;
    case GLFW_KEY_F7:
      out_key = Key::F7;
      return true;
    case GLFW_KEY_F8:
      out_key = Key::F8;
      return true;
    case GLFW_KEY_F9:
      out_key = Key::F9;
      return true;
    case GLFW_KEY_F10:
      out_key = Key::F10;
      return true;
    case GLFW_KEY_F11:
      out_key = Key::F11;
      return true;
    case GLFW_KEY_F12:
      out_key = Key::F12;
      return true;

    case GLFW_KEY_LEFT_SHIFT:
      out_key = Key::LeftShift;
      return true;
    case GLFW_KEY_RIGHT_SHIFT:
      out_key = Key::RightShift;
      return true;
    case GLFW_KEY_LEFT_CONTROL:
      out_key = Key::LeftControl;
      return true;
    case GLFW_KEY_RIGHT_CONTROL:
      out_key = Key::RightControl;
      return true;
    case GLFW_KEY_LEFT_ALT:
      out_key = Key::LeftAlt;
      return true;
    case GLFW_KEY_RIGHT_ALT:
      out_key = Key::RightAlt;
      return true;
    case GLFW_KEY_LEFT_SUPER:
      out_key = Key::LeftSuper;
      return true;
    case GLFW_KEY_RIGHT_SUPER:
      out_key = Key::RightSuper;
      return true;

    case GLFW_KEY_INSERT:
      out_key = Key::Insert;
      return true;
    case GLFW_KEY_CAPS_LOCK:
      out_key = Key::CapsLock;
      return true;
    case GLFW_KEY_SCROLL_LOCK:
      out_key = Key::ScrollLock;
      return true;
    case GLFW_KEY_NUM_LOCK:
      out_key = Key::NumLock;
      return true;
    case GLFW_KEY_PRINT_SCREEN:
      out_key = Key::PrintScreen;
      return true;
    case GLFW_KEY_PAUSE:
      out_key = Key::Pause;
      return true;
    case GLFW_KEY_MENU:
      out_key = Key::Menu;
      return true;

    case GLFW_KEY_APOSTROPHE:
      out_key = Key::Apostrophe;
      return true;
    case GLFW_KEY_COMMA:
      out_key = Key::Comma;
      return true;
    case GLFW_KEY_MINUS:
      out_key = Key::Minus;
      return true;
    case GLFW_KEY_PERIOD:
      out_key = Key::Period;
      return true;
    case GLFW_KEY_SLASH:
      out_key = Key::Slash;
      return true;
    case GLFW_KEY_SEMICOLON:
      out_key = Key::Semicolon;
      return true;
    case GLFW_KEY_EQUAL:
      out_key = Key::Equal;
      return true;
    case GLFW_KEY_LEFT_BRACKET:
      out_key = Key::LeftBracket;
      return true;
    case GLFW_KEY_BACKSLASH:
      out_key = Key::Backslash;
      return true;
    case GLFW_KEY_RIGHT_BRACKET:
      out_key = Key::RightBracket;
      return true;
    case GLFW_KEY_GRAVE_ACCENT:
      out_key = Key::GraveAccent;
      return true;

    case GLFW_KEY_WORLD_1:
      out_key = Key::World1;
      return true;
    case GLFW_KEY_WORLD_2:
      out_key = Key::World2;
      return true;

    case GLFW_KEY_KP_0:
      out_key = Key::Kp0;
      return true;
    case GLFW_KEY_KP_1:
      out_key = Key::Kp1;
      return true;
    case GLFW_KEY_KP_2:
      out_key = Key::Kp2;
      return true;
    case GLFW_KEY_KP_3:
      out_key = Key::Kp3;
      return true;
    case GLFW_KEY_KP_4:
      out_key = Key::Kp4;
      return true;
    case GLFW_KEY_KP_5:
      out_key = Key::Kp5;
      return true;
    case GLFW_KEY_KP_6:
      out_key = Key::Kp6;
      return true;
    case GLFW_KEY_KP_7:
      out_key = Key::Kp7;
      return true;
    case GLFW_KEY_KP_8:
      out_key = Key::Kp8;
      return true;
    case GLFW_KEY_KP_9:
      out_key = Key::Kp9;
      return true;
    case GLFW_KEY_KP_DECIMAL:
      out_key = Key::KpDecimal;
      return true;
    case GLFW_KEY_KP_DIVIDE:
      out_key = Key::KpDivide;
      return true;
    case GLFW_KEY_KP_MULTIPLY:
      out_key = Key::KpMultiply;
      return true;
    case GLFW_KEY_KP_SUBTRACT:
      out_key = Key::KpSubtract;
      return true;
    case GLFW_KEY_KP_ADD:
      out_key = Key::KpAdd;
      return true;
    case GLFW_KEY_KP_EQUAL:
      out_key = Key::KpEqual;
      return true;

    default:                 return false;
  }
}

// EN: D10 -- the subset of Key that App's physical-key callback ALSO forwards into the neutral
//     UiEvent/RmlUi route, UNCHANGED by HOSTIN-1's enum growth: exactly the pre-existing
//     nav-oriented 14 keys (Up..PageDown), the same subset Engine::to_rml_key (engine.cpp) has
//     understood since before this wave. None of the ~92 keys added above (letters, digits,
//     F-keys, modifiers, numpad, punctuation) are forwarded to the UI engine by this wave --
//     forwarding them for UI shortcuts is a separate, deliberate, future change (SEED-UIKEYFWD),
//     not smuggled in here. Pinned by tests/glfw_event_translate_sanity.cpp so a future edit
//     cannot silently widen (or narrow) what reaches RmlUi.
// PT: D10 -- o subconjunto de Key que o callback físico de tecla do App TAMBÉM encaminha para a
//     rota neutra UiEvent/RmlUi, INALTERADO pelo crescimento do enum do HOSTIN-1: exatamente as
//     14 teclas pré-existentes orientadas a navegação (Up..PageDown), o mesmo subconjunto que
//     Engine::to_rml_key (engine.cpp) já entendia antes desta onda. Nenhuma das ~92 teclas
//     adicionadas acima (letras, dígitos, F-teclas, modificadores, numpad, pontuação) é
//     encaminhada ao motor de UI por esta onda -- encaminhá-las para atalhos de UI é uma
//     mudança futura separada e deliberada (SEED-UIKEYFWD), não contrabandeada aqui. Fixado por
//     tests/glfw_event_translate_sanity.cpp para que uma edição futura não alargue (nem
//     estreite) silenciosamente o que chega ao RmlUi.
inline bool glfw_key_is_ui_forwardable(Key k) noexcept {
  switch (k) {
    case Key::Up:
    case Key::Down:
    case Key::Left:
    case Key::Right:
    case Key::Enter:
    case Key::Escape:
    case Key::Tab:
    case Key::Space:
    case Key::Backspace:
    case Key::Delete:
    case Key::Home:
    case Key::End:
    case Key::PageUp:
    case Key::PageDown:
      return true;
    default:
      return false;
  }
}

// EN: GLFW key/button action -> UiEvent::pressed. GLFW_PRESS and GLFW_RELEASE map directly;
//     GLFW_REPEAT (held-key auto-repeat) maps to `true` (pressed) so a held navigation key
//     (e.g. Down while scrolling a focus list) produces repeated KeyDown, which is what
//     RmlUi's own focus-cycling machinery expects for held-key navigation.
// PT: Ação de tecla/botão GLFW -> UiEvent::pressed. GLFW_PRESS e GLFW_RELEASE mapeiam direto;
//     GLFW_REPEAT (auto-repeat de tecla segurada) mapeia para `true` (pressionado) para que
//     uma tecla de navegação segurada (ex.: Down rolando uma lista com foco) produza KeyDown
//     repetido, que é o que a própria maquinaria de ciclo de foco do RmlUi espera para
//     navegação com tecla segurada.
inline bool glfw_translate_action_pressed(int glfw_action) noexcept {
  return glfw_action == GLFW_PRESS || glfw_action == GLFW_REPEAT;
}

// EN: GLFW modifier bitmask -> glintfx::Mod bitmask. Explicit per-bit mapping (never rely on
//     the numeric values coinciding, same discipline the pre-existing to_rml_mods in
//     engine.cpp already follows for the Rml::Input::KeyModifier side) -- GLFW_MOD_SHIFT=0x1,
//     GLFW_MOD_CONTROL=0x2, GLFW_MOD_ALT=0x4 (glfw3.h) happen to line up with
//     glintfx::Mod_Shift/Mod_Ctrl/Mod_Alt today, but this function does not depend on that.
// PT: Bitmask de modificador GLFW -> bitmask glintfx::Mod. Mapeamento explícito bit-a-bit
//     (nunca depender dos valores numéricos coincidirem, mesma disciplina que o to_rml_mods
//     pré-existente em engine.cpp já segue para o lado Rml::Input::KeyModifier) --
//     GLFW_MOD_SHIFT=0x1, GLFW_MOD_CONTROL=0x2, GLFW_MOD_ALT=0x4 (glfw3.h) acontecem de bater
//     com glintfx::Mod_Shift/Mod_Ctrl/Mod_Alt hoje, mas esta função não depende disso.
inline int glfw_translate_mods(int glfw_mods) noexcept {
  int result = Mod_None;
  if (glfw_mods & GLFW_MOD_SHIFT)   result |= Mod_Shift;
  if (glfw_mods & GLFW_MOD_CONTROL) result |= Mod_Ctrl;
  if (glfw_mods & GLFW_MOD_ALT)     result |= Mod_Alt;
  return result;
}

// EN: GLFW mouse button code -> UiEvent::button (0=left, 1=right, 2=middle). GLFW's own
//     GLFW_MOUSE_BUTTON_LEFT/RIGHT/MIDDLE are aliases for buttons 1/2/3 (1-based,
//     GLFW_MOUSE_BUTTON_1..3), so they already equal 0/1/2 -- this function makes that
//     explicit rather than passing glfw_button through uninspected. Buttons beyond MIDDLE
//     (side buttons, GLFW_MOUSE_BUTTON_4..8) are outside glintfx's 3-button contract and are
//     discarded (returns false, out_button untouched) -- same "do not synthesise an event for
//     an unmapped input" discipline as glfw_translate_key.
// PT: Código de botão do mouse GLFW -> UiEvent::button (0=esquerdo, 1=direito, 2=meio). Os
//     próprios GLFW_MOUSE_BUTTON_LEFT/RIGHT/MIDDLE do GLFW são aliases dos botões 1/2/3
//     (base 1, GLFW_MOUSE_BUTTON_1..3), então já valem 0/1/2 -- esta função torna isso
//     explícito em vez de repassar glfw_button sem inspecionar. Botões além do MIDDLE (botões
//     laterais, GLFW_MOUSE_BUTTON_4..8) estão fora do contrato de 3 botões da glintfx e são
//     descartados (retorna false, out_button intocado) -- mesma disciplina "não sintetizar
//     evento para input não mapeado" de glfw_translate_key.
inline bool glfw_translate_mouse_button(int glfw_button, int& out_button) noexcept {
  switch (glfw_button) {
    case GLFW_MOUSE_BUTTON_LEFT:   out_button = 0; return true;
    case GLFW_MOUSE_BUTTON_RIGHT:  out_button = 1; return true;
    case GLFW_MOUSE_BUTTON_MIDDLE: out_button = 2; return true;
    default:                       return false;
  }
}

// EN: GLFW cursor position (screen/content-area coordinates, from glfwSetCursorPosCallback)
//     -> UiEvent::x/y in FRAMEBUFFER pixels, the space glintfx's Rml::Context is dimensioned
//     in (glfwGetFramebufferSize, see App::render()/AUD-PUB-1). Under HiDPI the window's
//     content-area size and its framebuffer size differ (e.g. a 2x-scaled display reports a
//     1280x720 window backed by a 2560x1440 framebuffer); scaling by fb/win before emitting
//     MouseMove is REQUIRED so clicks land on the correct element. Confirmed against the
//     pinned RmlUi 6.3 GLFW backend, which applies the identical scale (framebuffer_size /
//     window_size, then rounds) before calling ProcessMouseMove --
//     build/_deps/rmlui-src/Backends/RmlUi_Platform_GLFW.cpp:134-142
//     (RmlGLFW::ProcessCursorPosCallback). win_w/win_h <= 0 (degenerate/minimized window,
//     transiently possible between a resize and the next framebuffer-size callback) falls
//     back to scale 1:1 rather than dividing by zero.
//     UNTESTED UNDER XVFB (Xvfb/X11 reports fb == win, scale is always 1:1 there) -- the
//     ratio != 1 branch is exercised ONLY by tests/glfw_event_translate_sanity.cpp (see this
//     plan's section 5, "the HiDPI case Xvfb does not exercise").
// PT: Posição de cursor GLFW (coordenadas de tela/área-de-conteúdo, de
//     glfwSetCursorPosCallback) -> UiEvent::x/y em pixels de FRAMEBUFFER, o espaço em que o
//     Rml::Context da glintfx é dimensionado (glfwGetFramebufferSize, ver App::render()/
//     AUD-PUB-1). Sob HiDPI o tamanho da área-de-conteúdo da janela e o tamanho do framebuffer
//     divergem (ex.: um display com escala 2x reporta uma janela 1280x720 sustentada por um
//     framebuffer 2560x1440); escalar por fb/win antes de emitir o MouseMove é OBRIGATÓRIO
//     para que cliques caiam no elemento correto. Confirmado contra o backend GLFW pinado do
//     RmlUi 6.3, que aplica a MESMA escala (framebuffer_size / window_size, depois arredonda)
//     antes de chamar ProcessMouseMove --
//     build/_deps/rmlui-src/Backends/RmlUi_Platform_GLFW.cpp:134-142
//     (RmlGLFW::ProcessCursorPosCallback). win_w/win_h <= 0 (janela degenerada/minimizada,
//     transitoriamente possível entre um resize e o próximo callback de framebuffer-size) cai
//     de volta para escala 1:1 em vez de dividir por zero.
//     NÃO TESTADO SOB XVFB (Xvfb/X11 reporta fb == win, a escala é sempre 1:1 lá) -- o branch
//     de razão != 1 é exercitado SÓ por tests/glfw_event_translate_sanity.cpp (ver a seção 5
//     deste plano, "o caso HiDPI que o Xvfb não exercita").
inline void glfw_translate_cursor_pos(double xpos, double ypos, int win_w, int win_h,
                                       int fb_w, int fb_h, float& out_x, float& out_y) noexcept {
  const double scale_x = (win_w > 0) ? static_cast<double>(fb_w) / static_cast<double>(win_w) : 1.0;
  const double scale_y = (win_h > 0) ? static_cast<double>(fb_h) / static_cast<double>(win_h) : 1.0;
  out_x = static_cast<float>(xpos * scale_x);
  out_y = static_cast<float>(ypos * scale_y);
}

// EN: GLFW scroll offsets (from glfwSetScrollCallback) -> UiEvent::MouseWheel delta
//     convention. SIGN INVERSION on BOTH axes.
//     Confirmed against the pinned RmlUi 6.3 GLFW backend: it negates yoffset before calling
//     ProcessMouseWheel -- `context->ProcessMouseWheel(-float(yoffset), mods)`,
//     build/_deps/rmlui-src/Backends/RmlUi_Platform_GLFW.cpp:161-168
//     (RmlGLFW::ProcessScrollCallback), invoked from
//     build/_deps/rmlui-src/Backends/RmlUi_Backend_GLFW_GL3.cpp:223-224. HONEST CAVEAT: that
//     backend's own scroll callback lambda discards xoffset entirely (parameter name commented
//     out, `/*xoffset*/`) and calls the DEPRECATED single-float ProcessMouseWheel overload --
//     it never negates or forwards x at all, because it was written before RmlUi's Vector2f
//     overload existed. glintfx already uses the Vector2f overload end-to-end (UiEvent::x/y
//     both carry delta, forwarded verbatim by Engine::process_event's MouseWheel case to
//     `ProcessMouseWheel(Rml::Vector2f(ev.x, ev.y), mods)`), so there is no upstream x
//     precedent to copy. This function negates x BY SYMMETRY with the y negation above: both
//     deltas originate from the same physical wheel/trackpad gesture and GLFW's own directional
//     sign convention (positive = away from the user / rightward) is consistent across axes,
//     so treating them identically is the defensible, testable choice -- see
//     tests/glfw_event_translate_sanity.cpp for the assertion pinning this exact behaviour.
// PT: Offsets de scroll GLFW (de glfwSetScrollCallback) -> convenção de delta do
//     UiEvent::MouseWheel. INVERSÃO DE SINAL nos DOIS eixos.
//     Confirmado contra o backend GLFW pinado do RmlUi 6.3: ele nega o yoffset antes de chamar
//     ProcessMouseWheel -- `context->ProcessMouseWheel(-float(yoffset), mods)`,
//     build/_deps/rmlui-src/Backends/RmlUi_Platform_GLFW.cpp:161-168
//     (RmlGLFW::ProcessScrollCallback), invocado a partir de
//     build/_deps/rmlui-src/Backends/RmlUi_Backend_GLFW_GL3.cpp:223-224. RESSALVA HONESTA: a
//     própria lambda de callback de scroll desse backend descarta o xoffset inteiramente (nome
//     do parâmetro comentado, `/*xoffset*/`) e chama a sobrecarga DEPRECADA de float único do
//     ProcessMouseWheel -- nunca nega nem repassa x, porque foi escrita antes da sobrecarga
//     Vector2f do RmlUi existir. A glintfx já usa a sobrecarga Vector2f ponta-a-ponta
//     (UiEvent::x/y carregam ambos delta, repassados tal-e-qual pelo caso MouseWheel de
//     Engine::process_event para `ProcessMouseWheel(Rml::Vector2f(ev.x, ev.y), mods)`), então
//     não há precedente upstream de x para copiar. Esta função nega x POR SIMETRIA com a
//     negação de y acima: ambos os deltas se originam do mesmo gesto físico de roda/trackpad e
//     a própria convenção de sinal direcional do GLFW (positivo = para longe do usuário /
//     para a direita) é consistente entre os eixos, então tratá-los identicamente é a escolha
//     defensável e testável -- ver tests/glfw_event_translate_sanity.cpp para a asserção que
//     fixa este comportamento exato.
inline void glfw_translate_scroll(double xoffset, double yoffset, float& out_x, float& out_y) noexcept {
  out_x = static_cast<float>(-xoffset);
  out_y = static_cast<float>(-yoffset);
}

// EN: Unicode codepoint (from glfwSetCharCallback) -> UTF-8, written into a caller-owned
//     buffer of at least 5 bytes (up to 4 data bytes + a terminating NUL, so the result can be
//     handed to UiEvent::text as-is). Returns the encoded length in bytes (1-4), or 0 for an
//     invalid codepoint (UTF-16 surrogate range 0xD800-0xDFFF, or > 0x10FFFF, the Unicode
//     ceiling) -- the CALLER must not emit a Text event in that case (out_buf is left
//     untouched on a 0 return, same "do not synthesise an event" discipline as
//     glfw_translate_key/glfw_translate_mouse_button). GLFW itself only ever calls the char
//     callback with a valid Unicode code point (its own input layer already filters control
//     characters), so this guard is defence-in-depth, not a documented GLFW failure mode.
// PT: Codepoint Unicode (de glfwSetCharCallback) -> UTF-8, escrito num buffer de posse do
//     chamador com no mínimo 5 bytes (até 4 bytes de dado + um NUL terminador, para que o
//     resultado possa ser entregue a UiEvent::text tal-e-qual). Retorna o comprimento
//     codificado em bytes (1-4), ou 0 para um codepoint inválido (faixa de surrogate UTF-16
//     0xD800-0xDFFF, ou > 0x10FFFF, o teto do Unicode) -- o CHAMADOR não deve emitir um evento
//     Text nesse caso (out_buf fica intocado num retorno 0, mesma disciplina "não sintetizar
//     evento" de glfw_translate_key/glfw_translate_mouse_button). O próprio GLFW só chama o
//     callback de char com um code point Unicode válido (a própria camada de input dele já
//     filtra caracteres de controle), então este guard é defesa-em-profundidade, não um modo
//     de falha documentado do GLFW.
inline int glfw_encode_utf8(unsigned int codepoint, char out_buf[5]) noexcept {
  if (codepoint > 0x10FFFFu || (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) return 0;
  if (codepoint <= 0x7Fu) {
    out_buf[0] = static_cast<char>(codepoint);
    out_buf[1] = '\0';
    return 1;
  }
  if (codepoint <= 0x7FFu) {
    out_buf[0] = static_cast<char>(0xC0u | (codepoint >> 6));
    out_buf[1] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    out_buf[2] = '\0';
    return 2;
  }
  if (codepoint <= 0xFFFFu) {
    out_buf[0] = static_cast<char>(0xE0u | (codepoint >> 12));
    out_buf[1] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
    out_buf[2] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
    out_buf[3] = '\0';
    return 3;
  }
  out_buf[0] = static_cast<char>(0xF0u | (codepoint >> 18));
  out_buf[1] = static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu));
  out_buf[2] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
  out_buf[3] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
  out_buf[4] = '\0';
  return 4;
}

// EN: HOSTIN-4 (Onda 2, v0.19.0, D6) -- pure decision seam for the user-initiated close veto,
//     extracted so the branch is directly unit-testable without ever creating a window (see
//     tests/glfw_event_translate_sanity.cpp). `has_callback=false` means no veto was ever registered --
//     the window's default GLFW behaviour applies and the close always proceeds (returns true),
//     matching the pre-HOSTIN-4 behaviour exactly. `has_callback=true` defers entirely to
//     `callback_allows_close` (the registered `std::function<bool()>`'s return value): `true` ->
//     close proceeds, `false` -> WindowGlfw::handle_window_close() resets
//     glfwWindowShouldClose back to GLFW_FALSE (the veto). request_close() (HOSTIN-3) never
//     calls into this seam at all -- it BYPASSES the veto entirely by design (D6).
// PT: HOSTIN-4 (Onda 2, v0.19.0, D6) -- seam de decisão pura para o veto de close iniciado pelo
//     usuário, extraído para que o ramo seja diretamente testável por unidade sem nunca criar
//     uma janela (ver tests/glfw_event_translate_sanity.cpp). `has_callback=false` significa que nenhum
//     veto foi registrado -- o comportamento default do GLFW se aplica e o close sempre
//     prossegue (retorna true), batendo exatamente com o comportamento pré-HOSTIN-4.
//     `has_callback=true` defere inteiramente a `callback_allows_close` (o valor de retorno do
//     `std::function<bool()>` registrado): `true` -> o close prossegue, `false` ->
//     WindowGlfw::handle_window_close() reseta glfwWindowShouldClose de volta a GLFW_FALSE (o
//     veto). request_close() (HOSTIN-3) nunca chama este seam -- ele BYPASSA o veto inteiramente
//     por desenho (D6).
inline bool glfw_decide_window_close(bool has_callback, bool callback_allows_close) noexcept {
  return !has_callback || callback_allows_close;
}

} // namespace glintfx
