// SPDX-License-Identifier: MPL-2.0
// EN: UiLayer implementation — embed/guest facade that attaches the RmlUi+GL3 engine to
//     a GL context owned by the host. Owns a SystemClock (no GLFW) and an Engine.
//     render() and process_event() are fully implemented in T3/T4; see stubs below.
// PT: Implementação do UiLayer — fachada embed/guest que anexa o motor RmlUi+GL3 a um
//     contexto GL do host. Possui SystemClock (sem GLFW) e Engine.
//     render() e process_event() são implementados completamente na T3/T4; ver stubs abaixo.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl3w must be included before any other OpenGL header (defines GL function pointers).
// PT: gl3w deve ser incluído antes de qualquer outro header OpenGL (define ponteiros de função GL).
#include <GL/gl3w.h>

#include <glintfx/ui_layer.hpp>
#include "engine.hpp"
#include "system_clock.hpp"

namespace glintfx {

struct UiLayer::Impl {
  SystemClock clock;  // EN: minimal RmlUi SystemInterface (clock only, no GLFW).
                      // PT: SystemInterface mínimo do RmlUi (só relógio, sem GLFW).
  Engine      engine;
  int  w   = 0;
  int  h   = 0;
  bool ok  = false;
};

UiLayer::UiLayer(Config cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.logical_width;
  impl_->h = cfg.logical_height;

  // EN: Load GL function pointers against the host's CURRENT context.
  //     gl3wInit() is idempotent within one process — subsequent calls are fast no-ops.
  //     Skip when the host has already initialised gl3w (cfg.load_gl = false).
  // PT: Carrega ponteiros de função GL contra o contexto CORRENTE do host.
  //     gl3wInit() é idempotente dentro de um processo — chamadas subsequentes são no-ops rápidos.
  //     Pular quando o host já inicializou o gl3w (cfg.load_gl = false).
  if (cfg.load_gl) {
    if (gl3wInit() != 0) return;
  }

  impl_->ok = impl_->engine.attach(&impl_->clock, impl_->w, impl_->h);
}

UiLayer::~UiLayer() = default;

UiLayer::UiLayer(UiLayer&&) noexcept = default;
UiLayer& UiLayer::operator=(UiLayer&&) noexcept = default;

bool UiLayer::ok() const noexcept {
  return impl_ && impl_->ok;
}

void UiLayer::load(const char* rml_path) {
  if (!impl_->ok) return;
  impl_->engine.load(rml_path);
}

void UiLayer::set_viewport(int w, int h) {
  if (!impl_->ok) return;
  impl_->w = w;
  impl_->h = h;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::update() {
  if (!impl_->ok) return;
  impl_->engine.update();
}

void UiLayer::render() {
  // EN: Compose-only path — fully implemented in T3 (GlStateGuard + begin/end_frame_compose).
  //     In T2 this is an intentional no-op stub so no test accidentally relies on it.
  // PT: Caminho compose-only — implementado completamente na T3 (GlStateGuard + begin/end_frame_compose).
  //     Na T2 este é stub no-op intencional para que nenhum teste dependa acidentalmente dele.
  (void)impl_;
}

void UiLayer::process_event(const UiEvent& /*ev*/) {
  // EN: Event translation (UiEvent → RmlUi calls) implemented in T4.
  // PT: Tradução de eventos (UiEvent → chamadas RmlUi) implementada na T4.
}

} // namespace glintfx
