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
#include <cstdio>  // EN: A4-WINMODES -- stderr fallback note (D5). PT: A4-WINMODES -- aviso de fallback em stderr (D5).
namespace glintfx {

WindowGlfw::~WindowGlfw() {
  if (win_) glfwDestroyWindow(win_);
  // EN: Only terminate GLFW if we successfully initialised it; avoids calling
  //     glfwTerminate() on a GLFW state that was never set up.
  // PT: Só termina o GLFW se o inicializamos com sucesso; evita chamar
  //     glfwTerminate() sobre um estado nunca configurado.
  if (glfw_inited_) glfwTerminate();
}

// EN: A4-WINMODES -- mode-aware create(). Switches BEFORE glfwCreateWindow: Windowed is the
//     unchanged bare-create path; Maximized sets the GLFW_MAXIMIZED hint (the WM applies it
//     after the window maps, respecting the work area for free -- D1/the GusWorld fix);
//     FullscreenDesktop/FullscreenExclusive resolve the primary monitor first (D5: null ->
//     stderr note + graceful windowed fallback, never a crash) then apply the D8 recipe:
//     Desktop pins the RED/GREEN/BLUE_BITS + REFRESH_RATE hints to the monitor's CURRENT video
//     mode and creates at that mode's exact width×height (GLFW's documented "windowed full
//     screen" recipe -- no mode switch); Exclusive creates directly at the requested w×h
//     against the monitor, letting GLFW pick the closest matching video mode (a REAL switch).
//     An out-of-range `mode` (invalid static_cast) falls through the switch's default case to
//     the same graceful-windowed path as a null monitor -- domino guard, twin of set_mode()'s
//     own invalid-enum check below (D5 fail-high discipline applied at BOTH call sites, not
//     just one).
// PT: A4-WINMODES -- create() ciente de modo. Decide ANTES do glfwCreateWindow: Windowed é o
//     caminho nu inalterado; Maximized define o hint GLFW_MAXIMIZED (o WM o aplica depois da
//     janela mapear, respeitando a work area de graça -- D1/o fix do GusWorld);
//     FullscreenDesktop/FullscreenExclusive resolvem o monitor primário primeiro (D5: nulo ->
//     aviso em stderr + fallback gracioso pra windowed, nunca um crash) e então aplicam a
//     receita D8: Desktop fixa os hints RED/GREEN/BLUE_BITS + REFRESH_RATE no video mode ATUAL
//     do monitor e cria exatamente naquela largura×altura (a receita documentada "windowed
//     full screen" do GLFW -- sem troca de modo); Exclusive cria direto na w×h pedida contra o
//     monitor, deixando o GLFW escolher o video mode mais próximo (uma troca REAL). Um `mode`
//     fora de faixa (static_cast inválido) cai pelo default do switch no mesmo caminho gracioso
//     pra windowed de um monitor nulo -- guarda de dominó, gêmea da própria checagem de enum
//     inválido do set_mode() abaixo (disciplina fail-high D5 aplicada nos DOIS call sites, não
//     só um).
bool WindowGlfw::create(const char* title, int w, int h, WindowMode req_mode) {
  if (!glfwInit()) return false;
  glfw_inited_ = true;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  cfg_w_ = w;
  cfg_h_ = h;
  saved_w_ = w;
  saved_h_ = h;
  has_saved_geometry_ = false;
  fullscreen_desktop_ = false;

  GLFWmonitor* monitor  = nullptr;
  int          create_w = w;
  int          create_h = h;

  // EN: `req_mode` (not `mode`) -- cppcheck shadowFunction: a parameter literally named `mode`
  //     shadows the member function WindowGlfw::mode() (same lint class the A2-GAMEPAD slice
  //     hit with a `locale` parameter -- see TODO.md).
  // PT: `req_mode` (não `mode`) -- cppcheck shadowFunction: um parâmetro literalmente chamado
  //     `mode` sombreia o método membro WindowGlfw::mode() (mesma classe de lint que a fatia
  //     A2-GAMEPAD pegou com um parâmetro `locale` -- ver TODO.md).
  switch (req_mode) {
    case WindowMode::Windowed:
      break;
    case WindowMode::Maximized:
      glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
      break;
    case WindowMode::FullscreenDesktop:
    case WindowMode::FullscreenExclusive: {
      monitor = glfwGetPrimaryMonitor();
      if (!monitor) {
        std::fprintf(stderr,
          "glintfx: WindowGlfw::create: no primary monitor available, "
          "falling back to windowed mode\n");
        break;  // EN: monitor stays null -> bare windowed create below. PT: monitor fica nulo -> create windowed nu abaixo.
      }
      if (req_mode == WindowMode::FullscreenDesktop) {
        // EN: D5 hardening -- glfwGetVideoMode can return null for a monitor with no reported
        //     mode (never observed on real hardware, but not documented as impossible either).
        //     Same graceful-windowed fallback as a null monitor above, not a null-deref crash.
        // PT: Reforço D5 -- glfwGetVideoMode pode retornar nulo pra um monitor sem modo
        //     reportado (nunca observado em hardware real, mas também não documentado como
        //     impossível). Mesmo fallback gracioso pra windowed do monitor nulo acima, não um
        //     crash de desreferência nula.
        const GLFWvidmode* vm = glfwGetVideoMode(monitor);
        if (!vm) {
          std::fprintf(stderr,
            "glintfx: WindowGlfw::create: primary monitor reports no video mode, "
            "falling back to windowed mode\n");
          monitor = nullptr;
          break;
        }
        glfwWindowHint(GLFW_RED_BITS,     vm->redBits);
        glfwWindowHint(GLFW_GREEN_BITS,   vm->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS,    vm->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, vm->refreshRate);
        create_w = vm->width;
        create_h = vm->height;
        fullscreen_desktop_ = true;
      }
      // EN: Exclusive -- create_w/create_h stay cfg w/h; GLFW itself picks the closest
      //     matching video mode against the monitor (a real switch, D8).
      // PT: Exclusive -- create_w/create_h permanecem w/h do cfg; o próprio GLFW escolhe o
      //     video mode mais próximo contra o monitor (uma troca real, D8).
      break;
    }
    default:
      // EN: Invalid enum (out-of-range cast) -- fail-high fallback to Windowed (D5).
      // PT: Enum inválido (cast fora de faixa) -- fallback fail-high para Windowed (D5).
      break;
  }

  win_ = glfwCreateWindow(create_w, create_h, title, monitor, nullptr);
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

// EN: A4-WINMODES -- captures the CURRENT position+size while the window is still windowed,
//     the instant before set_mode() dispatches to a different target (D7). Only meaningful
//     (and only ever called) when mode() has just reported Windowed -- see set_mode() below.
// PT: A4-WINMODES -- captura a posição+tamanho CORRENTE enquanto a janela ainda está windowed,
//     no instante antes de set_mode() despachar pra um alvo diferente (D7). Só faz sentido (e
//     só é chamada) quando mode() acabou de reportar Windowed -- ver set_mode() abaixo.
void WindowGlfw::save_windowed_geometry() {
  glfwGetWindowPos(win_, &saved_x_, &saved_y_);
  glfwGetWindowSize(win_, &saved_w_, &saved_h_);
  has_saved_geometry_ = true;
}

// EN: A4-WINMODES (D4) -- see the doc-comment on the declaration (window_glfw.hpp) for the
//     full derivation contract. glfwGetWindowMonitor() is the GLFW-documented way to ask "is
//     this window currently fullscreen" -- it returns the monitor the window occupies, or
//     NULL for windowed/maximized (GLFW does not have a separate "maximized" monitor state).
// PT: A4-WINMODES (D4) -- ver o doc-comment na declaração (window_glfw.hpp) para o contrato
//     completo de derivação. glfwGetWindowMonitor() é a forma documentada do GLFW de perguntar
//     "esta janela está fullscreen agora" -- retorna o monitor que a janela ocupa, ou NULL para
//     windowed/maximized (o GLFW não tem um estado de monitor separado para "maximizada").
WindowMode WindowGlfw::mode() const {
  if (!win_) return WindowMode::Windowed;
  if (glfwGetWindowMonitor(win_) != nullptr) {
    return fullscreen_desktop_ ? WindowMode::FullscreenDesktop : WindowMode::FullscreenExclusive;
  }
  if (glfwGetWindowAttrib(win_, GLFW_MAXIMIZED)) return WindowMode::Maximized;
  return WindowMode::Windowed;
}

// EN: A4-WINMODES -- runtime transition machine (D7/D8). See the doc-comment on the
//     declaration (window_glfw.hpp) for the full contract; this is the implementation.
//     Re-requesting the CURRENT live mode() is a no-op (matches app.hpp's documented
//     contract); leaving Windowed always snapshots the geometry FIRST (D7), regardless of
//     which target follows, so it is available for whichever later request returns to
//     Windowed. The fullscreen->Maximized branch below is the one D8 mandates a strict order
//     for: restore windowed FIRST via glfwSetWindowMonitor(..., nullptr, ...), THEN
//     glfwMaximizeWindow() -- GLFW has no direct fullscreen->maximized transition.
// PT: A4-WINMODES -- máquina de transição em runtime (D7/D8). Ver o doc-comment na declaração
//     (window_glfw.hpp) pro contrato completo; esta é a implementação. Repedir o modo VIVO
//     corrente (mode()) é um no-op (bate com o contrato documentado em app.hpp); sair de
//     Windowed sempre tira o snapshot da geometria PRIMEIRO (D7), independente de qual alvo
//     vem a seguir, para que fique disponível para qualquer pedido posterior de volta a
//     Windowed. O ramo fullscreen->Maximized abaixo é o único para o qual o D8 exige ordem
//     estrita: restaura windowed PRIMEIRO via glfwSetWindowMonitor(..., nullptr, ...), DEPOIS
//     glfwMaximizeWindow() -- o GLFW não tem transição direta fullscreen->maximizada.
bool WindowGlfw::set_mode(WindowMode req_mode) {
  // EN: `req_mode` (not `mode`) -- same cppcheck shadowFunction avoidance as create() above
  //     (this parameter would otherwise shadow the member function WindowGlfw::mode()).
  // PT: `req_mode` (não `mode`) -- mesma prevenção de shadowFunction do cppcheck que o create()
  //     acima (este parâmetro sombrearia o método membro WindowGlfw::mode() caso contrário).
  if (!win_) return false;
  const WindowMode current = this->mode();
  if (current == req_mode) return true;  // EN: no-op re-request. PT: repedido é no-op.

  if (current == WindowMode::Windowed) save_windowed_geometry();

  const int restore_x = has_saved_geometry_ ? saved_x_ : 64;
  const int restore_y = has_saved_geometry_ ? saved_y_ : 64;
  const int restore_w = has_saved_geometry_ ? saved_w_ : cfg_w_;
  const int restore_h = has_saved_geometry_ ? saved_h_ : cfg_h_;
  const bool from_fullscreen =
      current == WindowMode::FullscreenDesktop || current == WindowMode::FullscreenExclusive;

  switch (req_mode) {
    case WindowMode::Windowed:
      if (current == WindowMode::Maximized) {
        glfwRestoreWindow(win_);
      } else if (from_fullscreen) {
        glfwSetWindowMonitor(win_, nullptr, restore_x, restore_y, restore_w, restore_h, 0);
      }
      fullscreen_desktop_ = false;
      return true;

    case WindowMode::Maximized:
      if (from_fullscreen) {
        // EN: D8 mandatory order -- restore windowed BEFORE maximizing.
        // PT: Ordem obrigatória D8 -- restaura windowed ANTES de maximizar.
        glfwSetWindowMonitor(win_, nullptr, restore_x, restore_y, restore_w, restore_h, 0);
        fullscreen_desktop_ = false;
      }
      glfwMaximizeWindow(win_);
      return true;

    case WindowMode::FullscreenDesktop:
    case WindowMode::FullscreenExclusive: {
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      if (!monitor) {
        std::fprintf(stderr,
          "glintfx: WindowGlfw::set_mode: no primary monitor available, request ignored\n");
        return false;  // EN: D5 -- window left untouched. PT: D5 -- janela intocada.
      }
      if (req_mode == WindowMode::FullscreenDesktop) {
        // EN: D5 hardening, twin of create()'s own null-vm guard above -- see its comment.
        // PT: Reforço D5, gêmeo da própria guarda de vm nulo do create() acima -- ver comentário lá.
        const GLFWvidmode* vm = glfwGetVideoMode(monitor);
        if (!vm) {
          std::fprintf(stderr,
            "glintfx: WindowGlfw::set_mode: primary monitor reports no video mode, "
            "request ignored\n");
          return false;
        }
        glfwSetWindowMonitor(win_, monitor, 0, 0, vm->width, vm->height, vm->refreshRate);
        fullscreen_desktop_ = true;
      } else {
        glfwSetWindowMonitor(win_, monitor, 0, 0, cfg_w_, cfg_h_, GLFW_DONT_CARE);
        fullscreen_desktop_ = false;
      }
      return true;
    }

    default:
      // EN: Invalid enum (out-of-range cast) -- fail-high no-op (D5), twin of create()'s guard.
      // PT: Enum inválido (cast fora de faixa) -- no-op fail-high (D5), gêmeo da guarda do create().
      return false;
  }
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
