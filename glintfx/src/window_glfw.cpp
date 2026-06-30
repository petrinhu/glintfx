// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window implementation — guards glfwTerminate with glfw_inited_ flag and
//     null-checks win_ in every method that dereferences it.
// PT: Implementação da janela GLFW — protege glfwTerminate com flag glfw_inited_ e
//     verifica null de win_ em todo método que o desreferencia.
#include "window_glfw.hpp"
#include <GL/gl3w.h>
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
  if (gl3wInit() != 0) {
    glfwDestroyWindow(win_); win_ = nullptr;
    glfwTerminate(); glfw_inited_ = false;
    return false;
  }
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

} // namespace glintfx
