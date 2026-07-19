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
    default:                 return false;
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

} // namespace glintfx
