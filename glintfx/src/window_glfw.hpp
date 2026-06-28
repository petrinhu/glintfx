// SPDX-License-Identifier: MPL-2.0
// EN: GLFW window + GL3.3 core context. PT: janela GLFW + contexto GL3.3 core.
#pragma once
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
private:
  GLFWwindow* win_ = nullptr;
};
}
