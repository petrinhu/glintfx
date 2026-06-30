// SPDX-License-Identifier: MPL-2.0
// EN: Engine implementation — owns RenderGl3 + Bootstrap; provides standalone + compose frame paths.
// PT: Implementação do Engine — possui RenderGl3 + Bootstrap; provê caminhos standalone e compose.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_state.hpp includes GL/gl3w.h — must precede any other GL/RmlUi includes.
// PT: gl_state.hpp inclui GL/gl3w.h — deve preceder quaisquer outros includes GL/RmlUi.
#include "gl_state.hpp"
#include "engine.hpp"
#include "render_gl3.hpp"
#include "bootstrap.hpp"
#include "data_binder.hpp"
#include <RmlUi/Core.h>

namespace glintfx {

struct Engine::Impl {
  RenderGl3  render;
  Bootstrap  boot;
  DataBinder data_binder;  // EN: declared after boot; destroyed before boot (reverse order).
                           // PT: declarado após boot; destruído antes de boot (ordem reversa).
  bool ok     = false;
  bool loaded = false;
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
  // EN: Set loaded flag unconditionally so that bind_* calls after load() return false.
  //     The constraint is enforced regardless of whether load succeeds or not.
  // PT: Seta o flag loaded incondicionalmente para que bind_* após load() retorne false.
  //     A restrição é enforçada independente de o load ter sucesso ou não.
  impl_->loaded = true;
  return impl_->boot.load(rml_path);
}

void Engine::set_viewport(int w, int h) {
  if (!impl_->ok) return;
  if (auto* c = impl_->boot.context()) {
    c->SetDimensions(Rml::Vector2i(w, h));
  }
}

void Engine::set_dp_ratio(float ratio) {
  if (!impl_->ok) return;
  if (auto* c = impl_->boot.context()) {
    // EN: SetDensityIndependentPixelRatio is idempotent when ratio hasn't changed;
    //     it re-triggers layout when it has, so no need to guard here.
    // PT: SetDensityIndependentPixelRatio é idempotente quando ratio não mudou;
    //     re-dispara layout quando mudou, portanto sem guard necessário aqui.
    c->SetDensityIndependentPixelRatio(ratio);
  }
}

void Engine::set_asset_base_url(const char* url) {
  if (!impl_->ok) return;
  impl_->boot.set_asset_base_url(url);
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
  // EN: Compose-only frame path. The host already has its scene on the bound FBO; we layer
  //     the UI on top without clearing and without touching FBO0. A GlStateGuard snapshot is
  //     taken before begin_frame_compose and restored after end_frame_compose, so the host's
  //     renderer sees an unchanged GL context when we return.
  // PT: Caminho de frame compose-only. O host já tem a cena no FBO corrente; sobrepomos
  //     a UI sem limpar e sem tocar no FBO0. Um snapshot do GlStateGuard é feito antes de
  //     begin_frame_compose e restaurado após end_frame_compose, para que o renderer do host
  //     veja o contexto GL inalterado ao retornar.
  if (!impl_->ok) return;
  GlStateGuard guard;                                         // EN: saves host GL state.
                                                              // PT: salva estado GL do host.
  impl_->render.begin_frame_compose(w, h);
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame_compose();
}                                                             // EN: ~guard restores here.
                                                              // PT: ~guard restaura aqui.

Rml::Context* Engine::context() {
  return impl_->ok ? impl_->boot.context() : nullptr;
}

// ---------------------------------------------------------------------------
// EN: Data-model API — delegates to DataBinder with lifecycle guards.
//     create_data_model / bind_*: blocked when !ok or after load().
//     set_*: blocked when !ok; DataBinder is a no-op when the key is unknown.
// PT: API de data-model — delega ao DataBinder com guards de ciclo de vida.
//     create_data_model / bind_*: bloqueados quando !ok ou após load().
//     set_*: bloqueados quando !ok; DataBinder é no-op para chave desconhecida.
// ---------------------------------------------------------------------------

bool Engine::create_data_model(const char* name) {
  if (!impl_->ok || impl_->loaded) return false;
  return impl_->data_binder.create(impl_->boot.context(), name);
}

bool Engine::bind_number(const char* key, double initial) {
  if (!impl_->ok || impl_->loaded) return false;
  return impl_->data_binder.bind_number(key, initial);
}

bool Engine::bind_string(const char* key, const char* initial) {
  if (!impl_->ok || impl_->loaded) return false;
  return impl_->data_binder.bind_string(key, initial);
}

bool Engine::bind_bool(const char* key, bool initial) {
  if (!impl_->ok || impl_->loaded) return false;
  return impl_->data_binder.bind_bool(key, initial);
}

bool Engine::bind_list(const char* key) {
  if (!impl_->ok || impl_->loaded) return false;
  return impl_->data_binder.bind_list(key);
}

void Engine::set_number(const char* key, double v) {
  if (!impl_->ok) return;
  impl_->data_binder.set_number(key, v);
}

void Engine::set_string(const char* key, const char* v) {
  if (!impl_->ok) return;
  impl_->data_binder.set_string(key, v);
}

void Engine::set_bool(const char* key, bool v) {
  if (!impl_->ok) return;
  impl_->data_binder.set_bool(key, v);
}

void Engine::set_list(const char* key, const char* const* items, std::size_t n) {
  if (!impl_->ok) return;
  impl_->data_binder.set_list(key, items, n);
}

} // namespace glintfx
