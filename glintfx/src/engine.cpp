// SPDX-License-Identifier: MPL-2.0
// EN: Engine implementation — owns RenderGl3 + Bootstrap; provides standalone frame path.
//     render_compose() is a documented no-op stub in T1; real implementation arrives in T3.
// PT: Implementação do Engine — possui RenderGl3 + Bootstrap; provê caminho de frame standalone.
//     render_compose() é stub no-op documentado na T1; implementação real chega na T3.
// Copyright (c) 2026 Petrus Silva Costa
#include "engine.hpp"
#include "render_gl3.hpp"
#include "bootstrap.hpp"
#include <RmlUi/Core.h>

namespace glintfx {

struct Engine::Impl {
  RenderGl3 render;
  Bootstrap boot;
  bool ok = false;
};

Engine::Engine()  : impl_(new Impl()) {}
Engine::~Engine() { delete impl_; }

bool Engine::attach(Rml::SystemInterface* system, int w, int h) {
  if (impl_->ok) return false; // EN: guard: already attached. PT: guard: já anexado.
  if (!impl_->render.init()) return false;
  if (!impl_->boot.init(system, impl_->render, w, h)) return false;
  // EN: Belt-and-suspenders: init() above already guards these, but we cache ok_ here
  //     so that all callers use a single flag rather than re-querying iface()/context().
  // PT: Dupla segurança: o init() acima já guarda esses casos, mas cacheamos ok_ aqui
  //     para que todos os callers usem um único flag em vez de re-consultar iface()/context().
  impl_->ok = (impl_->render.iface() != nullptr) && (impl_->boot.context() != nullptr);
  return impl_->ok;
}

bool Engine::ok() const noexcept { return impl_ && impl_->ok; }

bool Engine::load(const char* rml_path) {
  if (!impl_->ok) return false;
  return impl_->boot.load(rml_path);
}

void Engine::set_viewport(int w, int h) {
  if (!impl_->ok) return;
  if (auto* c = impl_->boot.context()) {
    c->SetDimensions(Rml::Vector2i(w, h));
  }
}

void Engine::update() {
  if (!impl_->ok) return;
  if (auto* c = impl_->boot.context()) c->Update();
}

void Engine::render_standalone(int w, int h) {
  // EN: Standalone frame path — clear + render UI + alpha-fix on FBO0.
  //     Identical to the old App loop (minus the buffer swap, which the caller does).
  // PT: Caminho de frame standalone — clear + render UI + alpha-fix no FBO0.
  //     Idêntico ao loop antigo do App (menos o swap de buffer, que o chamador faz).
  if (!impl_->ok) return;
  impl_->render.begin_frame(w, h);
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame();
}

void Engine::render_compose(int w, int h) {
  // EN: Compose-only path — implemented in T3 (GlStateGuard + begin/end_frame_compose).
  //     In T1 this is intentionally a no-op so no test accidentally relies on it.
  // PT: Caminho compose-only — implementado na T3 (GlStateGuard + begin/end_frame_compose).
  //     Na T1 este é intencionalmente um no-op para que nenhum teste dependa dele acidentalmente.
  (void)w; (void)h;
}

Rml::Context* Engine::context() {
  return impl_->ok ? impl_->boot.context() : nullptr;
}

} // namespace glintfx
