// SPDX-License-Identifier: MPL-2.0
// EN: App facade implementation — RAII wrapper over WindowGlfw + RenderGl3 + Bootstrap.
// PT: Implementação da fachada App — wrapper RAII sobre WindowGlfw + RenderGl3 + Bootstrap.
// Copyright (c) 2026 Petrus Silva Costa
// EN: gl3w for GL function pointers used in snapshot() (glReadPixels, glBindFramebuffer, etc.).
// PT: gl3w para ponteiros de função GL usados em snapshot() (glReadPixels, glBindFramebuffer, etc.).
#include <GL/gl3w.h>
#include <cstdio>

#include <glintfx/glintfx.hpp>
#include "window_glfw.hpp"
#include "render_gl3.hpp"
#include "bootstrap.hpp"
#include <RmlUi/Core.h>
#include <vector>

namespace glintfx {

const char* version() { return "0.1.0"; }

// EN: Impl owns all subsystems. Destruction order (reverse declaration) is intentional:
//     boot → render → window ensures RmlUi shuts down before GL context is destroyed.
// PT: Impl possui todos os subsistemas. Ordem de destruição (declaração reversa) é intencional:
//     boot → render → window garante que RmlUi encerra antes do contexto GL ser destruído.
struct App::Impl {
  WindowGlfw window;
  RenderGl3  render;
  Bootstrap  boot;
  int  w   = 0;
  int  h   = 0;
  bool ok  = false;
};

App::App(AppConfig cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.width;
  impl_->h = cfg.height;
  if (!impl_->window.create(cfg.title, cfg.width, cfg.height)) return;
  if (!impl_->render.init()) return;
  impl_->ok = impl_->boot.init(impl_->window, impl_->render, cfg.width, cfg.height);
}

App::~App() = default;
App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

void App::load(const char* rml_path) {
  if (impl_->ok) impl_->boot.load(rml_path);
}

bool App::running() const {
  return impl_->ok && !impl_->window.should_close();
}

void App::poll_events() {
  impl_->window.poll();
}

void App::update() {
  if (auto* c = impl_->boot.context()) c->Update();
}

void App::render() {
  int w = 0, h = 0;
  impl_->window.size(w, h);
  impl_->render.begin_frame(w, h);
  if (auto* c = impl_->boot.context()) {
    c->Render();
  }
  impl_->render.end_frame();
  impl_->window.swap();
}

bool App::snapshot(const char* ppm_path) {
  // EN: Render one frame, read FBO 0 BEFORE swap (back-buffer has defined content here),
  //     save as PPM, then swap. On many GL implementations (including Mesa/llvmpipe under
  //     Xvfb) the back-buffer content is undefined AFTER glfwSwapBuffers, so capture must
  //     happen here, between EndFrame and the buffer swap.
  // PT: Renderiza um frame, lê o FBO 0 ANTES do swap (back-buffer tem conteúdo definido aqui),
  //     salva como PPM, depois troca. Em muitas implementações GL (incluindo Mesa/llvmpipe sob
  //     Xvfb) o conteúdo do back-buffer é indefinido APÓS glfwSwapBuffers, então a captura
  //     deve acontecer aqui, entre EndFrame e a troca de buffer.
  if (!impl_->ok) return false;
  int w = 0, h = 0;
  impl_->window.size(w, h);
  impl_->render.begin_frame(w, h);
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame();  // composites to FBO 0
  // EN: FBO 0 now contains the complete composited frame. Read before swap.
  // PT: FBO 0 agora contém o frame composto completo. Lê antes do swap.
  glBindFramebuffer(0x8D40, 0); // GL_FRAMEBUFFER, 0 = window
  glReadBuffer(0x0402);         // GL_BACK
  // EN: PACK_ALIGNMENT=1: pack rows with no padding so w*3 bytes/row matches vector size exactly.
  // PT: PACK_ALIGNMENT=1: empacota linhas sem padding para que w*3 bytes/linha bata exatamente.
  glPixelStorei(0x0D05, 1);     // GL_PACK_ALIGNMENT = 1
  std::vector<unsigned char> px((size_t)w * h * 3);
  glReadPixels(0, 0, w, h, 0x1907, 0x1401, px.data()); // GL_RGB, GL_UNSIGNED_BYTE
  glPixelStorei(0x0D05, 4);     // restore GL_PACK_ALIGNMENT = 4
  impl_->window.swap();
  FILE* f = fopen(ppm_path, "wb");
  if (!f) return false;
  fprintf(f, "P6\n%d %d\n255\n", w, h);
  fwrite(px.data(), 1, px.size(), f);
  fclose(f);
  return true;
}

void App::run() {
  while (running()) {
    poll_events();
    update();
    render();
  }
}

} // namespace glintfx
