// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window + GL3.3 core context. PT: janela GLFW + contexto GL3.3 core.
#pragma once
#include <functional>
#include <glintfx/ui_event.hpp>
struct GLFWwindow;
namespace glintfx {
class WindowGlfw {
public:
  WindowGlfw() = default;
  ~WindowGlfw();
  WindowGlfw(const WindowGlfw&) = delete;
  WindowGlfw& operator=(const WindowGlfw&) = delete;
  bool create(const char* title, int w, int h);
  void make_current();
  void swap();
  void poll();
  bool should_close() const;
  void size(int& w, int& h) const;
  GLFWwindow* handle() const { return win_; }

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
