// SPDX-License-Identifier: MPL-2.0
// EN: App facade implementation — RAII wrapper over WindowGlfw + RenderGl3 + Bootstrap.
// PT: Implementação da fachada App — wrapper RAII sobre WindowGlfw + RenderGl3 + Bootstrap.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include "window_glfw.hpp"
#include "render_gl3.hpp"
#include "bootstrap.hpp"
#include <RmlUi/Core.h>

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
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame();
  impl_->window.swap();
}

void App::run() {
  while (running()) {
    poll_events();
    update();
    render();
  }
}

} // namespace glintfx
