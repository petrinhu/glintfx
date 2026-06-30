// SPDX-License-Identifier: MPL-2.0
#include "window_glfw.hpp"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
namespace glintfx {
WindowGlfw::~WindowGlfw() {
  if (win_) glfwDestroyWindow(win_);
  glfwTerminate();
}
bool WindowGlfw::create(const char* title, int w, int h) {
  if (!glfwInit()) return false;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  win_ = glfwCreateWindow(w, h, title, nullptr, nullptr);
  if (!win_) { glfwTerminate(); return false; }
  glfwMakeContextCurrent(win_);
  if (gl3wInit() != 0) { glfwDestroyWindow(win_); win_ = nullptr; glfwTerminate(); return false; }
  return true;
}
void WindowGlfw::make_current() { glfwMakeContextCurrent(win_); }
void WindowGlfw::swap() { glfwSwapBuffers(win_); }
void WindowGlfw::poll() { glfwPollEvents(); }
bool WindowGlfw::should_close() const { return win_ && glfwWindowShouldClose(win_); }
void WindowGlfw::size(int& w, int& h) const { glfwGetFramebufferSize(win_, &w, &h); }
}
