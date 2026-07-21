// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window + GL3.3 core context. PT: janela GLFW + contexto GL3.3 core.
#pragma once
#include <functional>
#include <glintfx/ui_event.hpp>
#include <glintfx/window_mode.hpp>  // EN: A4-WINMODES. PT: A4-WINMODES.
struct GLFWwindow;
namespace glintfx {
class WindowGlfw {
public:
  WindowGlfw() = default;
  ~WindowGlfw();
  WindowGlfw(const WindowGlfw&) = delete;
  WindowGlfw& operator=(const WindowGlfw&) = delete;

  // EN: A4-WINMODES -- `req_mode` defaults to Windowed so the ~30 pre-existing embed-mode test
  //     fixtures that only use WindowGlfw as a bare GL-context host (create(title, w, h), no
  //     window-mode concern) keep compiling unchanged; glintfx::App always passes an explicit
  //     mode (app.cpp). See window_mode.hpp for the per-mode semantics and app.hpp's
  //     App::set_window_mode() doc-comment for the full contract this create() call backs.
  // PT: A4-WINMODES -- `mode` tem default Windowed para que os ~30 fixtures de teste
  //     pré-existentes do modo embed que só usam WindowGlfw como host nu de contexto GL
  //     (create(title, w, h), sem preocupação de modo de janela) continuem compilando sem
  //     mudança; o glintfx::App sempre passa um modo explícito (app.cpp). Ver window_mode.hpp
  //     para a semântica por modo e o doc-comment de App::set_window_mode() (app.hpp) para o
  //     contrato completo que este create() sustenta.
  bool create(const char* title, int w, int h, WindowMode req_mode = WindowMode::Windowed);
  void make_current();
  void swap();
  void poll();
  bool should_close() const;
  void size(int& w, int& h) const;
  GLFWwindow* handle() const { return win_; }

  // EN: A4-WINMODES -- runtime mode switch. Returns false (no-op, window untouched) when
  //     win_ is null (create() not called or failed) or `req_mode` is not a valid enumerator (an
  //     out-of-range static_cast) -- same fail-high discipline as every other guard in this
  //     file. Re-requesting the CURRENT live mode() is a safe no-op returning true. See
  //     window_mode.hpp for per-mode semantics and app.hpp's App::set_window_mode() for the
  //     full public contract (this is its direct implementation).
  //     Transition mechanics (D7/D8): leaving Windowed saves the current position+size
  //     (glfwGetWindowPos/Size) so a later return to Windowed restores it; a window that was
  //     NEVER windowed (born Maximized/Fullscreen) restores to AppConfig's construction
  //     width×height at position (64,64) -- documented default, avoids a window materialising
  //     under the panel. Fullscreen(Desktop or Exclusive) -> Maximized restores windowed FIRST
  //     (glfwSetWindowMonitor(..., nullptr, ...)), THEN glfwMaximizeWindow() -- mandatory
  //     order, GLFW does not support maximizing a fullscreen window directly. Wayland: window
  //     position is not readable/settable (best-effort restore, documented limitation).
  // PT: A4-WINMODES -- troca de modo em runtime. Retorna false (no-op, janela intocada) quando
  //     win_ é nulo (create() não chamado ou falhou) ou `req_mode` não é um enumerador válido (um
  //     static_cast fora de faixa) -- mesma disciplina fail-high de toda outra guarda deste
  //     arquivo. Repedir o modo VIVO corrente (mode()) é um no-op seguro que retorna true. Ver
  //     window_mode.hpp para a semântica por modo e App::set_window_mode() (app.hpp) para o
  //     contrato público completo (este é a implementação direta dele).
  //     Mecânica de transição (D7/D8): sair de Windowed salva a posição+tamanho corrente
  //     (glfwGetWindowPos/Size) para que um retorno posterior a Windowed os restaure; uma
  //     janela que NUNCA foi windowed (nasceu Maximized/Fullscreen) restaura para o
  //     width×height de construção do AppConfig na posição (64,64) -- default documentado,
  //     evita a janela materializar sob o painel. Fullscreen(Desktop ou Exclusive) ->
  //     Maximized restaura windowed PRIMEIRO (glfwSetWindowMonitor(..., nullptr, ...)), DEPOIS
  //     glfwMaximizeWindow() -- ordem obrigatória, o GLFW não suporta maximizar uma janela
  //     fullscreen diretamente. Wayland: posição de janela não é legível/gravável (restauração
  //     best-effort, limitação documentada).
  bool set_mode(WindowMode req_mode);

  // EN: A4-WINMODES -- LIVE window mode, derived from the window system, NOT an echo of the
  //     last set_mode()/create() request: glfwGetWindowMonitor(win_)!=nullptr means fullscreen
  //     (an internal flag distinguishes Desktop from Exclusive -- GLFW itself does not); else
  //     glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) means Maximized; else Windowed. Returns
  //     Windowed when win_ is null. Headless consequence (documented, not a bug): under Xvfb
  //     (no WM) a Maximized request may not be honoured and this can report Windowed --
  //     tolerated by the headless test suite, proven for real by the manual smoke on a live
  //     KDE/GNOME session (see docs/window-modes.md).
  // PT: A4-WINMODES -- modo de janela VIVO, derivado do sistema de janelas, NÃO um eco do
  //     último pedido set_mode()/create(): glfwGetWindowMonitor(win_)!=nullptr significa
  //     fullscreen (uma flag interna distingue Desktop de Exclusive -- o próprio GLFW não
  //     distingue); senão glfwGetWindowAttrib(win_, GLFW_MAXIMIZED) significa Maximized; senão
  //     Windowed. Retorna Windowed quando win_ é nulo. Consequência headless (documentada, não
  //     é bug): sob Xvfb (sem WM) um pedido Maximized pode não ser honrado e isto pode reportar
  //     Windowed -- tolerado pela suíte headless, provado de verdade pelo smoke manual numa
  //     sessão KDE/GNOME real (ver docs/window-modes.md).
  WindowMode mode() const;

  // EN: Register the sink that receives physical input translated to the neutral UiEvent
  //     format (A1, framework-2D). The 5 GLFW callbacks (key/char/mouse-button/cursor-pos/
  //     scroll) are always registered inside create() -- independent of whether a sink has
  //     been set yet -- and read the CURRENT sink_ via the stored WindowGlfw* user pointer
  //     every time they fire (glfwSetWindowUserPointer), not a snapshot taken at registration
  //     time; so calling set_event_sink() at any point after create() (including from inside
  //     App's constructor, after engine.attach() succeeds -- see app.cpp) is enough to start
  //     receiving events on the next poll(). A null/empty sink is a safe no-op: events are
  //     silently discarded, same "nothing happens" contract poll() already had before A1.
  // PT: Registra o sink que recebe input físico traduzido para o formato neutro UiEvent (A1,
  //     framework-2D). Os 5 callbacks GLFW (key/char/botão-do-mouse/cursor/scroll) são sempre
  //     registrados dentro de create() -- independente de um sink já ter sido definido -- e
  //     leem o sink_ CORRENTE via o ponteiro de usuário WindowGlfw* armazenado
  //     (glfwSetWindowUserPointer) toda vez que disparam, não um snapshot tirado no momento do
  //     registro; então chamar set_event_sink() a qualquer momento após create() (inclusive de
  //     dentro do construtor do App, após engine.attach() ter sucesso -- ver app.cpp) já basta
  //     para começar a receber eventos no próximo poll(). Um sink nulo/vazio é um no-op
  //     seguro: eventos são descartados silenciosamente, mesmo contrato "nada acontece" que
  //     poll() já tinha antes do A1.
  void set_event_sink(std::function<void(const UiEvent&)> sink);

private:
  GLFWwindow* win_        = nullptr;
  bool        glfw_inited_ = false; // EN: true only after glfwInit() succeeds. PT: true somente após glfwInit() com sucesso.
  std::function<void(const UiEvent&)> sink_;

  // EN: A4-WINMODES -- mode-transition state. cfg_w_/cfg_h_ are the AppConfig construction
  //     dimensions (create()'s w/h args), used both for a FullscreenExclusive create()/set_mode()
  //     target (GLFW picks the closest matching video mode) and as the D7 restore-to-windowed
  //     fallback size for a window that was NEVER windowed (born Maximized/Fullscreen).
  //     fullscreen_desktop_ disambiguates Desktop from Exclusive while glfwGetWindowMonitor()
  //     reports "some fullscreen" for both (D4 -- GLFW itself does not track the distinction).
  //     saved_x_/saved_y_/saved_w_/saved_h_ + has_saved_geometry_ are the D7 windowed-geometry
  //     snapshot, captured by save_windowed_geometry() the moment mode() reads Windowed just
  //     before set_mode() dispatches to a different target; saved_x_/saved_y_ default to
  //     (64,64) (D7's documented "never was windowed" position) so a fullscreen/maximized-born
  //     window that later asks for Windowed never materialises at (0,0), under a panel.
  // PT: A4-WINMODES -- estado de transição de modo. cfg_w_/cfg_h_ são as dimensões de
  //     construção do AppConfig (args w/h do create()), usadas tanto como alvo de
  //     create()/set_mode() FullscreenExclusive (o GLFW escolhe o video mode mais próximo)
  //     quanto como tamanho de fallback D7 pra restaurar-a-windowed de uma janela que NUNCA
  //     foi windowed (nasceu Maximized/Fullscreen). fullscreen_desktop_ desambigua Desktop de
  //     Exclusive enquanto glfwGetWindowMonitor() reporta "algum fullscreen" pros dois (D4 -- o
  //     próprio GLFW não rastreia a distinção). saved_x_/saved_y_/saved_w_/saved_h_ +
  //     has_saved_geometry_ são o snapshot de geometria windowed do D7, capturado por
  //     save_windowed_geometry() no instante em que mode() lê Windowed logo antes de
  //     set_mode() despachar pra um alvo diferente; saved_x_/saved_y_ têm default (64,64) (a
  //     posição documentada "nunca foi windowed" do D7) para que uma janela nascida
  //     fullscreen/maximized que depois peça Windowed nunca materialize em (0,0), sob um painel.
  int  cfg_w_ = 0;
  int  cfg_h_ = 0;
  bool fullscreen_desktop_   = false;
  bool has_saved_geometry_   = false;
  int  saved_x_ = 64;
  int  saved_y_ = 64;
  int  saved_w_ = 0;
  int  saved_h_ = 0;
  void save_windowed_geometry();
  // EN: Last known modifier bitmask (glintfx::Mod), updated by the key and mouse-button
  //     trampolins and reused for the cursor-pos/scroll callbacks, which GLFW does NOT hand a
  //     mods parameter to -- same pattern the pinned RmlUi GLFW backend uses
  //     (glfw_active_modifiers, RmlUi_Backend_GLFW_GL3.cpp).
  // PT: Último bitmask de modificador conhecido (glintfx::Mod), atualizado pelos trampolins de
  //     tecla e botão-do-mouse e reusado pelos callbacks de cursor/scroll, aos quais o GLFW
  //     NÃO entrega um parâmetro mods -- mesmo padrão que o backend GLFW pinado do RmlUi usa
  //     (glfw_active_modifiers, RmlUi_Backend_GLFW_GL3.cpp).
  int active_mods_ = 0;

  // EN: One-line static trampolines registered with glfwSet*Callback -- resolve the
  //     WindowGlfw* from glfwGetWindowUserPointer and call into the corresponding instance
  //     method below, which does the actual translation (via glfw_event_translate.hpp) and
  //     sink dispatch. Kept to a single line each per the plan's testability contract: all
  //     real logic lives in the pure functions the unit test exercises directly.
  // PT: Trampolins static de uma linha registrados com glfwSet*Callback -- resolvem o
  //     WindowGlfw* via glfwGetWindowUserPointer e chamam o método de instância
  //     correspondente abaixo, que faz a tradução de fato (via glfw_event_translate.hpp) e o
  //     despacho ao sink. Mantidos a uma linha cada, conforme o contrato de testabilidade do
  //     plano: toda a lógica real mora nas funções puras que o teste unit exercita direto.
  static void on_key(GLFWwindow* w, int key, int scancode, int action, int mods);
  static void on_char(GLFWwindow* w, unsigned int codepoint);
  static void on_mouse_button(GLFWwindow* w, int button, int action, int mods);
  static void on_cursor_pos(GLFWwindow* w, double xpos, double ypos);
  static void on_scroll(GLFWwindow* w, double xoffset, double yoffset);

  void dispatch(const UiEvent& ev);
  void handle_key(int key, int scancode, int action, int mods);
  void handle_char(unsigned int codepoint);
  void handle_mouse_button(int button, int action, int mods);
  void handle_cursor_pos(double xpos, double ypos);
  void handle_scroll(double xoffset, double yoffset);
};
}
