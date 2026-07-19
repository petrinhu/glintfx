// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window implementation — guards glfwTerminate with glfw_inited_ flag and
//     null-checks win_ in every method that dereferences it. Since A1 (framework-2D), also
//     owns the 5 GLFW input callbacks that feed glintfx::App's physical input route -- see
//     glfw_event_translate.hpp for the pure translation functions the trampolines below call.
// PT: Implementação da janela GLFW — protege glfwTerminate com flag glfw_inited_ e
//     verifica null de win_ em todo método que o desreferencia. Desde o A1 (framework-2D),
//     também possui os 5 callbacks de input do GLFW que alimentam a rota de input físico da
//     glintfx::App -- ver glfw_event_translate.hpp para as funções puras de tradução que os
//     trampolins abaixo chamam.
#include "window_glfw.hpp"
#include "gl_loader.h"
#include "glfw_event_translate.hpp"
#include <GLFW/glfw3.h>
namespace glintfx {

WindowGlfw::~WindowGlfw() {
  if (win_) glfwDestroyWindow(win_);
  // EN: Only terminate GLFW if we successfully initialised it; avoids calling
  //     glfwTerminate() on a GLFW state that was never set up.
  // PT: Só termina o GLFW se o inicializamos com sucesso; evita chamar
  //     glfwTerminate() sobre um estado nunca configurado.
  if (glfw_inited_) glfwTerminate();
}

bool WindowGlfw::create(const char* title, int w, int h) {
  if (!glfwInit()) return false;
  glfw_inited_ = true;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  win_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
  if (!win_) { glfwTerminate(); glfw_inited_ = false; return false; }
  glfwMakeContextCurrent(win_);
  if (glx_gl_load() != 0) {
    glfwDestroyWindow(win_); win_ = nullptr;
    glfwTerminate(); glfw_inited_ = false;
    return false;
  }
  // EN: A1 (framework-2D) -- register the 5 physical-input callbacks unconditionally, before
  //     any sink is set. Each trampoline is a no-op until set_event_sink() installs one (see
  //     dispatch() below); registering early means App never needs a second "arm the input"
  //     call, and a host that never calls set_event_sink() at all (synthetic-only via
  //     process_event()) pays zero extra cost beyond these 5 pointer-sized registrations.
  // PT: A1 (framework-2D) -- registra os 5 callbacks de input físico incondicionalmente,
  //     antes de qualquer sink ser definido. Cada trampolim é no-op até set_event_sink()
  //     instalar um (ver dispatch() abaixo); registrar cedo significa que o App nunca precisa
  //     de uma segunda chamada "arma o input", e um host que nunca chama set_event_sink()
  //     (só síntese via process_event()) paga custo extra zero além destes 5 registros do
  //     tamanho de um ponteiro.
  glfwSetWindowUserPointer(win_, this);
  glfwSetKeyCallback(win_, &WindowGlfw::on_key);
  glfwSetCharCallback(win_, &WindowGlfw::on_char);
  glfwSetMouseButtonCallback(win_, &WindowGlfw::on_mouse_button);
  glfwSetCursorPosCallback(win_, &WindowGlfw::on_cursor_pos);
  glfwSetScrollCallback(win_, &WindowGlfw::on_scroll);
  return true;
}

// EN: make_current, swap, poll, size: no-op when win_ is null (partially-constructed object).
// PT: make_current, swap, poll, size: no-op quando win_ é nulo (objeto parcialmente construído).
void WindowGlfw::make_current() { if (win_) glfwMakeContextCurrent(win_); }
void WindowGlfw::swap()         { if (win_) glfwSwapBuffers(win_); }
void WindowGlfw::poll()         { if (win_) glfwPollEvents(); }
bool WindowGlfw::should_close() const { return win_ && glfwWindowShouldClose(win_); }
void WindowGlfw::size(int& w, int& h) const {
  if (win_) { glfwGetFramebufferSize(win_, &w, &h); }
  else      { w = 0; h = 0; }
}

void WindowGlfw::set_event_sink(std::function<void(const UiEvent&)> sink) {
  sink_ = std::move(sink);
}

void WindowGlfw::dispatch(const UiEvent& ev) {
  if (sink_) sink_(ev);
}

// ---------------------------------------------------------------------------
// EN: Static trampolines -- one line each, per the plan's testability contract (section 2.2):
//     resolve `this` from the GLFW user pointer and forward to the instance method that does
//     the actual translation + dispatch.
// PT: Trampolins static -- uma linha cada, conforme o contrato de testabilidade do plano
//     (seção 2.2): resolvem `this` a partir do user pointer do GLFW e repassam ao método de
//     instância que faz a tradução + despacho de fato.
// ---------------------------------------------------------------------------

void WindowGlfw::on_key(GLFWwindow* w, int key, int scancode, int action, int mods) {
  static_cast<WindowGlfw*>(glfwGetWindowUserPointer(w))->handle_key(key, scancode, action, mods);
}
void WindowGlfw::on_char(GLFWwindow* w, unsigned int codepoint) {
  static_cast<WindowGlfw*>(glfwGetWindowUserPointer(w))->handle_char(codepoint);
}
void WindowGlfw::on_mouse_button(GLFWwindow* w, int button, int action, int mods) {
  static_cast<WindowGlfw*>(glfwGetWindowUserPointer(w))->handle_mouse_button(button, action, mods);
}
void WindowGlfw::on_cursor_pos(GLFWwindow* w, double xpos, double ypos) {
  static_cast<WindowGlfw*>(glfwGetWindowUserPointer(w))->handle_cursor_pos(xpos, ypos);
}
void WindowGlfw::on_scroll(GLFWwindow* w, double xoffset, double yoffset) {
  static_cast<WindowGlfw*>(glfwGetWindowUserPointer(w))->handle_scroll(xoffset, yoffset);
}

// ---------------------------------------------------------------------------
// EN: Instance handlers -- translate (glfw_event_translate.hpp) then dispatch(). Unmapped
//     input (key outside the nav subset, button beyond middle, invalid codepoint) is silently
//     dropped: no UiEvent is built or dispatched, per glfw_translate_key/
//     glfw_translate_mouse_button/glfw_encode_utf8's own "do not synthesise an event"
//     contract.
// PT: Handlers de instância -- traduzem (glfw_event_translate.hpp) e então dispatch(). Input
//     não mapeado (tecla fora do subconjunto de nav, botão além do meio, codepoint inválido)
//     é descartado silenciosamente: nenhum UiEvent é construído ou despachado, conforme o
//     próprio contrato "não sintetizar evento" de glfw_translate_key/
//     glfw_translate_mouse_button/glfw_encode_utf8.
// ---------------------------------------------------------------------------

void WindowGlfw::handle_key(int key, int /*scancode*/, int action, int mods) {
  active_mods_ = glfw_translate_mods(mods);
  Key k;
  if (!glfw_translate_key(key, k)) return;
  UiEvent ev{};
  ev.type      = UiEvent::Type::Key;
  ev.key       = k;
  ev.pressed   = glfw_translate_action_pressed(action);
  ev.modifiers = active_mods_;
  dispatch(ev);
}

void WindowGlfw::handle_char(unsigned int codepoint) {
  char buf[5];
  if (glfw_encode_utf8(codepoint, buf) == 0) return;
  UiEvent ev{};
  ev.type = UiEvent::Type::Text;
  ev.text = buf;  // EN: buf outlives this synchronous dispatch call. PT: buf sobrevive a esta chamada síncrona de dispatch.
  dispatch(ev);
}

void WindowGlfw::handle_mouse_button(int button, int action, int mods) {
  active_mods_ = glfw_translate_mods(mods);
  int b;
  if (!glfw_translate_mouse_button(button, b)) return;
  UiEvent ev{};
  ev.type      = UiEvent::Type::MouseButton;
  ev.button    = b;
  ev.pressed   = (action == GLFW_PRESS);  // EN: buttons have no REPEAT action. PT: botões não têm ação REPEAT.
  ev.modifiers = active_mods_;
  dispatch(ev);
}

void WindowGlfw::handle_cursor_pos(double xpos, double ypos) {
  int win_w = 0, win_h = 0, fb_w = 0, fb_h = 0;
  glfwGetWindowSize(win_, &win_w, &win_h);
  glfwGetFramebufferSize(win_, &fb_w, &fb_h);
  UiEvent ev{};
  ev.type      = UiEvent::Type::MouseMove;
  glfw_translate_cursor_pos(xpos, ypos, win_w, win_h, fb_w, fb_h, ev.x, ev.y);
  ev.modifiers = active_mods_;
  dispatch(ev);
}

void WindowGlfw::handle_scroll(double xoffset, double yoffset) {
  UiEvent ev{};
  ev.type      = UiEvent::Type::MouseWheel;
  glfw_translate_scroll(xoffset, yoffset, ev.x, ev.y);
  ev.modifiers = active_mods_;
  dispatch(ev);
}

} // namespace glintfx
