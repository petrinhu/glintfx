// SPDX-License-Identifier: MPL-2.0
// EN: Engine implementation — owns RenderGl3 + Bootstrap; provides standalone + compose frame paths.
// PT: Implementação do Engine — possui RenderGl3 + Bootstrap; provê caminhos standalone e compose.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_state.hpp includes gl_loader.h — must precede any other GL/RmlUi includes.
// PT: gl_state.hpp inclui gl_loader.h — deve preceder quaisquer outros includes GL/RmlUi.
#include "gl_state.hpp"
#include "engine.hpp"
#include "render_gl3.hpp"
#include "bootstrap.hpp"
#include "data_binder.hpp"
#include <RmlUi/Core.h>
#include <cmath>  // EN: std::isfinite (dp-ratio input hardening). PT: std::isfinite (hardening do dp-ratio).

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

bool Engine::attach(Rml::SystemInterface* system, int w, int h, FontEngine font_engine) {
  if (impl_->ok) return false; // EN: guard: already attached. PT: guard: já anexado.
  if (!impl_->render.init()) return false;
  if (!impl_->boot.init(system, impl_->render, w, h, font_engine)) return false;
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
  // EN: Defence-in-depth w/h guard (input-hardening audit, v0.3.0). UiLayer::set_viewport already
  //     guards before reaching here, but validating at this common Engine point too means no
  //     current or future caller can push a zero/negative dimension into
  //     Rml::Context::SetDimensions (degenerate layout). Same rationale as set_dp_ratio's guard.
  // PT: Guard de w/h em defesa-em-profundidade (auditoria de hardening, v0.3.0). UiLayer::
  //     set_viewport já valida antes de chegar aqui, mas validar também neste ponto comum do
  //     Engine garante que nenhum caller atual ou futuro empurre uma dimensão zero/negativa para
  //     Rml::Context::SetDimensions (layout degenerado). Mesma racional do guard do set_dp_ratio.
  if (w <= 0 || h <= 0) return;
  if (auto* c = impl_->boot.context()) {
    c->SetDimensions(Rml::Vector2i(w, h));
  }
}

void Engine::set_dp_ratio(float ratio) {
  if (!impl_->ok) return;
  // EN: Input-hardening (audit, v0.3.0). The dp ratio scales the RCSS `dp` unit and feeds
  //     Rml::Context::SetDensityIndependentPixelRatio, which propagates it through 40+ layout
  //     calculations. A non-positive ratio (0 / negative) collapses every dp-sized element to
  //     zero; a non-finite ratio (NaN/inf, e.g. from a `real_w / logical_w` where logical_w
  //     ended up 0) silently poisons the entire layout with NaN, producing an invisible or
  //     garbage UI with no error. Reject anything that is not a finite positive number: keep the
  //     previous (or default 1.0) ratio and log. Applied here at the single common Engine point
  //     so App and UiLayer are both covered. Mirrors the fail-high spirit of the polygon() sides
  //     hardening: invalid input is ignored, never propagated.
  // PT: Hardening de entrada (auditoria, v0.3.0). O dp ratio escala a unidade RCSS `dp` e
  //     alimenta Rml::Context::SetDensityIndependentPixelRatio, que o propaga por 40+ cálculos de
  //     layout. Um ratio não-positivo (0 / negativo) colapsa todo elemento dimensionado em dp
  //     para zero; um ratio não-finito (NaN/inf, ex.: de um `real_w / logical_w` onde logical_w
  //     acabou 0) envenena silenciosamente todo o layout com NaN, produzindo uma UI invisível ou
  //     lixo sem erro. Rejeita qualquer coisa que não seja um número finito positivo: mantém o
  //     ratio anterior (ou o default 1.0) e loga. Aplicado aqui no ponto comum único do Engine
  //     para que App e UiLayer sejam ambos cobertos. Espelha o espírito fail-high do hardening do
  //     sides de polygon(): entrada inválida é ignorada, nunca propagada.
  if (!std::isfinite(ratio) || ratio <= 0.0f) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "set_dp_ratio(%g) ignored -- dp ratio must be a finite positive number "
        "(previous ratio kept).", ratio);
    return;
  }
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

void Engine::render_compose(int offset_x, int offset_y, int w, int h) {
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
  impl_->render.begin_frame_compose(offset_x, offset_y, w, h);
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame_compose();
}                                                             // EN: ~guard restores here.
                                                              // PT: ~guard restaura aqui.

Rml::Context* Engine::context() {
  return impl_->ok ? impl_->boot.context() : nullptr;
}

void Engine::set_click_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_click_callback(std::move(cb));
}

void Engine::set_click_info_callback(std::function<void(const ClickInfo&)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_click_info_callback(std::move(cb));
}

void Engine::set_scroll_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_scroll_callback(std::move(cb));
}

void Engine::set_change_callback(std::function<void(const char*, const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_change_callback(std::move(cb));
}

void Engine::set_submit_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_submit_callback(std::move(cb));
}

void Engine::set_focus_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_focus_callback(std::move(cb));
}

void Engine::set_blur_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_blur_callback(std::move(cb));
}

void Engine::set_hover_callback(std::function<void(const char*, bool)> cb) {
  if (!impl_->ok) return;
  impl_->boot.set_hover_callback(std::move(cb));
}

bool Engine::get_element_box(const char* id, float& x, float& y, float& w, float& h) const {
  if (!impl_->ok) return false;
  return impl_->boot.get_element_box(id, x, y, w, h);
}

bool Engine::scroll_element_into_view(const char* id, bool align_with_top) const {
  if (!impl_->ok) return false;
  return impl_->boot.scroll_element_into_view(id, align_with_top);
}

bool Engine::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  if (!impl_->ok) return false;
  return impl_->boot.get_element_scroll_top(id, out_scroll_top);
}

bool Engine::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  if (!impl_->ok) return false;
  return impl_->boot.get_element_scroll_height(id, out_scroll_height);
}

bool Engine::get_element_client_height(const char* id, float& out_client_height) const {
  if (!impl_->ok) return false;
  return impl_->boot.get_element_client_height(id, out_client_height);
}

bool Engine::set_element_scroll_top(const char* id, float scroll_top) const {
  if (!impl_->ok) return false;
  return impl_->boot.set_element_scroll_top(id, scroll_top);
}

bool Engine::set_focus(const char* id) const {
  if (!impl_->ok) return false;
  return impl_->boot.set_focus(id);
}

bool Engine::clear_focus() const {
  if (!impl_->ok) return false;
  return impl_->boot.clear_focus();
}

// ---------------------------------------------------------------------------
// EN: DOM read/write by id (L1.16-DOMRW) — straight forwards to Bootstrap; the guard/escaping
//     logic lives there (see bootstrap.cpp), same "!ok() -> false" pattern as every other
//     per-id method above.
// PT: Leitura/escrita de DOM por id (L1.16-DOMRW) — repasse direto ao Bootstrap; a lógica de
//     guard/escape mora lá (ver bootstrap.cpp), mesmo padrão "!ok() -> false" de todo outro
//     método por-id acima.
// ---------------------------------------------------------------------------

bool Engine::set_text(const char* id, const char* text) const {
  if (!impl_->ok) return false;
  return impl_->boot.set_text(id, text);
}

bool Engine::add_class(const char* id, const char* cls) const {
  if (!impl_->ok) return false;
  return impl_->boot.add_class(id, cls);
}

bool Engine::remove_class(const char* id, const char* cls) const {
  if (!impl_->ok) return false;
  return impl_->boot.remove_class(id, cls);
}

bool Engine::set_property(const char* id, const char* prop, const char* value) const {
  if (!impl_->ok) return false;
  return impl_->boot.set_property(id, prop, value);
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

bool Engine::get_number(const char* key, double& out) const {
  if (!impl_->ok) return false;
  return impl_->data_binder.get_number(key, out);
}

bool Engine::get_string(const char* key, std::string& out) const {
  if (!impl_->ok) return false;
  return impl_->data_binder.get_string(key, out);
}

bool Engine::get_bool(const char* key, bool& out) const {
  if (!impl_->ok) return false;
  return impl_->data_binder.get_bool(key, out);
}

} // namespace glintfx
