// SPDX-License-Identifier: MPL-2.0
// EN: App facade implementation — RAII wrapper over WindowGlfw + SystemInterface_GLFW + Engine.
//     Engine owns RenderGl3 + Bootstrap. SystemInterface_GLFW lives here because it requires a
//     GLFWwindow* at construction time (available only after WindowGlfw::create()).
// PT: Implementação da fachada App — wrapper RAII sobre WindowGlfw + SystemInterface_GLFW + Engine.
//     Engine possui RenderGl3 + Bootstrap. SystemInterface_GLFW vive aqui pois exige
//     GLFWwindow* na construção (disponível apenas após WindowGlfw::create()).
// Copyright (c) 2026 Petrus Silva Costa
// EN: gl3w for GL function pointers used in snapshot() (glReadPixels, glBindFramebuffer, etc.).
// PT: gl3w para ponteiros de função GL usados em snapshot() (glReadPixels, glBindFramebuffer, etc.).
#include <GL/gl3w.h>
#include <cstdio>
#include <memory>
#include <vector>

#include <glintfx/glintfx.hpp>
#include "window_glfw.hpp"
#include "engine.hpp"
// EN: RmlUi_Platform_GLFW.h migrated from bootstrap.cpp — SystemInterface_GLFW now lives here.
// PT: RmlUi_Platform_GLFW.h migrou de bootstrap.cpp — SystemInterface_GLFW agora vive aqui.
#include "RmlUi_Platform_GLFW.h"

namespace glintfx {

// EN: version() lives in src/version.cpp now (L1.9-VERSEMBED) — that TU is always
//     compiled, unlike this one (app.cpp is GLFW-backend-only).
// PT: version() agora vive em src/version.cpp (L1.9-VERSEMBED) — essa TU é sempre
//     compilada, diferente desta (app.cpp é exclusiva do backend GLFW).

// EN: Impl owns all subsystems. Declaration order is intentional: C++ destructs members in
//     REVERSE order, so engine destructs first (calls Rml::Shutdown via Bootstrap::~Bootstrap),
//     then system is deleted (no longer needed by RmlUi), then window dtor destroys GL context.
//     window → system → engine = construction order; engine → system → window = destruction order.
// PT: Impl possui todos os subsistemas. Ordem de declaração é intencional: C++ destrói membros
//     na ORDEM REVERSA, então engine destrói primeiro (chama Rml::Shutdown via Bootstrap::~Bootstrap),
//     depois system é deletado (não mais necessário ao RmlUi), depois dtor de window destrói contexto GL.
//     window → system → engine = ordem de construção; engine → system → window = ordem de destruição.
struct App::Impl {
  WindowGlfw                            window;
  std::unique_ptr<SystemInterface_GLFW> system;  // EN: heap; deleted after engine (RmlUi shutdown).
                                                  // PT: heap; deletado após engine (shutdown RmlUi).
  Engine                                engine;
  int  w  = 0;
  int  h  = 0;
  bool ok = false;
};

App::App(AppConfig cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.width;
  impl_->h = cfg.height;
  if (!impl_->window.create(cfg.title, cfg.width, cfg.height)) return;
  // EN: Window::create() makes the GL context current — required by Engine::attach().
  // PT: Window::create() torna o contexto GL corrente — exigido por Engine::attach().
  impl_->system = std::make_unique<SystemInterface_GLFW>(impl_->window.handle());
  impl_->ok = impl_->engine.attach(impl_->system.get(), cfg.width, cfg.height);
  // EN: Apply initial dp_ratio; idempotent for the default 1.0f.
  // PT: Aplica dp_ratio inicial; idempotente para o padrão 1.0f.
  if (impl_->ok) impl_->engine.set_dp_ratio(cfg.dp_ratio);
}

App::~App() = default;
App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

bool App::load(const char* rml_path) {
  if (!impl_->ok) return false;
  return impl_->engine.load(rml_path);
}

// ---------------------------------------------------------------------------
// EN: Data-model API — forward to Engine; Engine guards the ordering constraint.
// PT: API de data-model — encaminha ao Engine; Engine enforça a restrição de ordem.
// ---------------------------------------------------------------------------

bool App::create_data_model(const char* name) {
  if (!impl_->ok) return false;
  return impl_->engine.create_data_model(name);
}

bool App::bind_number(const char* key, double initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_number(key, initial);
}

bool App::bind_string(const char* key, const char* initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_string(key, initial);
}

bool App::bind_bool(const char* key, bool initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_bool(key, initial);
}

bool App::bind_list(const char* key) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_list(key);
}

void App::set_number(const char* key, double value) {
  if (!impl_->ok) return;
  impl_->engine.set_number(key, value);
}

void App::set_string(const char* key, const char* value) {
  if (!impl_->ok) return;
  impl_->engine.set_string(key, value);
}

void App::set_bool(const char* key, bool value) {
  if (!impl_->ok) return;
  impl_->engine.set_bool(key, value);
}

void App::set_list(const char* key, const char* const* items, std::size_t count) {
  if (!impl_->ok) return;
  impl_->engine.set_list(key, items, count);
}

void App::set_dp_ratio(float ratio) {
  if (!impl_->ok) return;
  impl_->engine.set_dp_ratio(ratio);
}

void App::set_asset_base_url(const char* url) {
  if (!impl_->ok) return;
  impl_->engine.set_asset_base_url(url);
}

bool App::ok() const noexcept {
  return impl_ && impl_->ok;
}

bool App::running() const {
  return impl_->ok && !impl_->window.should_close();
}

void App::poll_events() {
  // EN: Guard: consistent with load()/running() — no-op if subsystem init failed.
  // PT: Guard: consistente com load()/running() — no-op se a inicialização falhou.
  if (!impl_->ok) return;
  impl_->window.poll();
}

void App::update() {
  if (impl_->ok) impl_->engine.update();
}

void App::render() {
  // EN: Guard: no-op if subsystem init failed (consistent with load()/running()).
  // PT: Guard: no-op se a inicialização falhou (consistente com load()/running()).
  if (!impl_->ok) return;
  int w = 0, h = 0;
  impl_->window.size(w, h);
  impl_->engine.render_standalone(w, h);
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
  impl_->engine.render_standalone(w, h);
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
  // EN: glReadPixels origin is bottom-left; PPM origin is top-left.
  //     Write rows in reverse order (last row first) so the image is upright.
  // PT: A origem do glReadPixels é bottom-left; a do PPM é top-left.
  //     Grava as linhas em ordem inversa (última linha primeiro) para a imagem ficar na vertical correta.
  const size_t row_bytes = (size_t)w * 3;
  for (int row = h - 1; row >= 0; --row) {
    fwrite(px.data() + (size_t)row * row_bytes, 1, row_bytes, f);
  }
  fclose(f);
  return true;
}

void App::set_click_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_click_callback(std::move(cb));
}

ElementBox App::get_element_box(const char* id) const {
  ElementBox box;
  if (!impl_->ok) return box;
  float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
  if (impl_->engine.get_element_box(id, x, y, w, h)) {
    box.found = true; box.x = x; box.y = y; box.w = w; box.h = h;
  }
  return box;
}

void App::run() {
  while (running()) {
    poll_events();
    update();
    render();
  }
}

} // namespace glintfx
