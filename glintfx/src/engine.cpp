// SPDX-License-Identifier: Apache-2.0
// EN: Engine implementation — owns RenderGl3 + Bootstrap; provides standalone + compose frame paths.
// PT: Implementação do Engine — possui RenderGl3 + Bootstrap; provê caminhos standalone e compose.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_state.hpp includes gl_loader.h — must precede any other GL/RmlUi includes.
// PT: gl_state.hpp inclui gl_loader.h — deve preceder quaisquer outros includes GL/RmlUi.
#include "gl_state.hpp"
#include "engine.hpp"
#include "render_gl3.hpp"
#include "rml/bootstrap.hpp"
#include "data_binder.hpp"
#include <glintfx/log.hpp> // EN: glintfx::log_warn (RMLX-0/F2 -- replaces the two Rml::Log::Message calls below). PT: glintfx::log_warn (RMLX-0/F2 -- substitui as duas chamadas Rml::Log::Message abaixo).
#include <cmath>     // EN: std::isfinite (dp-ratio input hardening / process_event guards). PT: std::isfinite (hardening do dp-ratio / guards do process_event).
#include <exception> // EN: std::exception (CAPTURE-NOTHROW, capture_frame's own try/catch). PT: std::exception (CAPTURE-NOTHROW, try/catch do próprio capture_frame).
#include <vector>    // EN: intermediate RGB scratch buffer (capture_frame, FRAMEGRAB-EMBED). PT: buffer RGB intermediário (capture_frame, FRAMEGRAB-EMBED).

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
  //     QW-GUARDLOG (v0.18.1): this is the common Engine-level gate App relies on directly (see
  //     app.cpp's sync_viewport, which calls engine.set_viewport(w, h) with no local pre-guard of
  //     its own) -- unlike set_dp_ratio, this guard used to reject silently. Logging here fixes
  //     that for App and for any future direct Engine caller; UiLayer's own pre-guard (ui_layer.cpp)
  //     is logged separately, since it short-circuits before ever reaching this point.
  // PT: Guard de w/h em defesa-em-profundidade (auditoria de hardening, v0.3.0). UiLayer::
  //     set_viewport já valida antes de chegar aqui, mas validar também neste ponto comum do
  //     Engine garante que nenhum caller atual ou futuro empurre uma dimensão zero/negativa para
  //     Rml::Context::SetDimensions (layout degenerado). Mesma racional do guard do set_dp_ratio.
  //     QW-GUARDLOG (v0.18.1): este é o gate comum de nível Engine do qual o App depende
  //     diretamente (ver o sync_viewport de app.cpp, que chama engine.set_viewport(w, h) sem
  //     pre-guard próprio) -- diferente do set_dp_ratio, este guard rejeitava em silêncio. Logar
  //     aqui corrige isso para o App e para qualquer caller futuro direto do Engine; o pre-guard
  //     próprio do UiLayer (ui_layer.cpp) é logado separadamente, já que ele intercepta antes de
  //     sequer chegar até aqui.
  if (w <= 0 || h <= 0) {
    // EN: RMLX-0/F2 -- glintfx::log_warn (glintfx/log.hpp) replaces the direct
    //     Rml::Log::Message(LT_WARNING, ...) call this guard used before this fatia: same final
    //     destination (a host's own installed sink, or the fprintf(stderr, ...) default), but
    //     routed through this library's OWN public log surface instead of RmlUi's, so engine.cpp
    //     no longer needs to touch Rml::Log at all. See src/rml/bootstrap.hpp's own doc-comment
    //     on the "driving" surface below (set_dimensions) for why the w/h > 0 guard itself
    //     stays HERE (pure int comparison, needs nothing RmlUi-shaped) rather than moving into
    //     Bootstrap.
    // PT: RMLX-0/F2 -- glintfx::log_warn (glintfx/log.hpp) substitui a chamada direta
    //     Rml::Log::Message(LT_WARNING, ...) que esta guarda usava antes desta fatia: mesmo
    //     destino final (o sink próprio instalado por um host, ou o default
    //     fprintf(stderr, ...)), mas roteado pela superfície de log PRÓPRIA desta biblioteca em
    //     vez da do RmlUi, então engine.cpp deixa de precisar tocar Rml::Log de qualquer forma.
    //     Ver o próprio doc-comment da superfície de "condução" em src/rml/bootstrap.hpp
    //     (set_dimensions) pro motivo da guarda w/h > 0 em si permanecer AQUI (comparação de
    //     int pura, não precisa de nada em formato RmlUi) em vez de mudar para o Bootstrap.
    glintfx::log_warn(
        "set_viewport(%d, %d) ignored -- viewport dimensions must both be positive "
        "(previous viewport kept).",
        w, h);
    return;
  }
  impl_->boot.set_dimensions(w, h);
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
    // EN: RMLX-0/F2 -- glintfx::log_warn replaces Rml::Log::Message here, same reasoning as
    //     set_viewport's own guard right above.
    // PT: RMLX-0/F2 -- glintfx::log_warn substitui o Rml::Log::Message aqui, mesma racional da
    //     própria guarda de set_viewport logo acima.
    glintfx::log_warn(
        "set_dp_ratio(%g) ignored -- dp ratio must be a finite positive number "
        "(previous ratio kept).",
        ratio);
    return;
  }
  impl_->boot.set_density_independent_pixel_ratio(ratio);
}

void Engine::set_asset_base_url(const char* url) {
  if (!impl_->ok) return;
  impl_->boot.set_asset_base_url(url);
}

void Engine::update() {
  if (!impl_->ok) return;
  impl_->boot.update();
}

void Engine::render_standalone(int w, int h, const std::function<void()>& scene_hook) {
  // EN: Standalone frame path — clear + [scene_hook] + render UI + alpha-fix on FBO0.
  //     Identical to the old App loop (minus the buffer swap, which the caller does) when
  //     scene_hook is empty -- see this method's doc-comment in engine.hpp for the GL state
  //     contract the GlStateGuard below enforces around the hook (A1, framework-2D).
  // PT: Caminho de frame standalone — clear + [scene_hook] + render UI + alpha-fix no FBO0.
  //     Idêntico ao loop antigo do App (menos o swap de buffer, que o chamador faz) quando
  //     scene_hook está vazio -- ver o doc-comment deste método em engine.hpp para o contrato
  //     de estado GL que o GlStateGuard abaixo enforça ao redor do hook (A1, framework-2D).
  if (!impl_->ok) return;
  impl_->render.begin_frame(w, h);
  if (scene_hook) {
    GlStateGuard guard;  // EN: snapshot the state BeginFrame() just set. PT: snapshota o estado que BeginFrame() acabou de definir.
    scene_hook();
  }                      // EN: ~guard restores it here, before Context::Render() below.
                         // PT: ~guard o restaura aqui, antes do Context::Render() abaixo.
  impl_->boot.render();
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
  impl_->boot.render();
  impl_->render.end_frame_compose();
}                                                             // EN: ~guard restores here.
                                                              // PT: ~guard restaura aqui.

Rml::Context* Engine::context() {
  return impl_->ok ? impl_->boot.context() : nullptr;
}

void Engine::process_event(const UiEvent& ev, int offset_x, int offset_y) {
  // EN: Guard: drop events when the engine is not ready (same shape as every other Engine
  //     method — "!ok() -> no-op"). Moved verbatim from UiLayer::process_event (A1,
  //     framework-2D). RMLX-0/F2: the extra "context is gone" half of this guard that used to
  //     live here (`Rml::Context* c = impl_->boot.context(); if (!c) return;`) is now folded
  //     into EACH of the Bootstrap "driving" calls below (process_mouse_move/
  //     process_mouse_button/process_key/process_text/process_mouse_wheel all internally guard
  //     `if (!impl_ || !impl_->ctx) return;`, bootstrap.hpp/.cpp) -- this switch no longer needs
  //     a local `Rml::Context*` at all.
  // PT: Guard: descarta eventos quando o engine não está pronto (mesmo formato de todo outro
  //     método do Engine — "!ok() -> no-op"). Movido tal-e-qual de UiLayer::process_event (A1,
  //     framework-2D). RMLX-0/F2: a metade extra "contexto sumiu" desta guarda que morava aqui
  //     (`Rml::Context* c = impl_->boot.context(); if (!c) return;`) agora fica dobrada dentro
  //     de CADA chamada de "condução" do Bootstrap abaixo (process_mouse_move/
  //     process_mouse_button/process_key/process_text/process_mouse_wheel todas guardam
  //     internamente `if (!impl_ || !impl_->ctx) return;`, bootstrap.hpp/.cpp) -- este switch
  //     não precisa mais de um `Rml::Context*` local nenhum.
  if (!impl_->ok) return;

  using T = UiEvent::Type;
  switch (ev.type) {
    case T::None:
      // EN: Inert event (L1.10-APIDOC) — the default-constructed UiEvent{} value.
      //     Intentional no-op: a host that forgets to set .type gets silently dropped
      //     instead of being misinterpreted as a mouse-move to (0, 0).
      // PT: Evento inerte (L1.10-APIDOC) — valor default-construído UiEvent{}.
      //     No-op intencional: um host que esquece de definir .type é descartado
      //     silenciosamente em vez de ser interpretado como mouse-move para (0, 0).
      break;

    case T::MouseMove:
      // EN: Pointer position in integer pixels; modifiers rarely set on move but forwarded.
      //     window-space -> content-space (F3, v0.2.5): subtract the caller-supplied offset
      //     (0,0 for App; UiLayer's letterbox origin otherwise) before forwarding to RmlUi,
      //     which only knows its own offset-free content space -- see this method's
      //     doc-comment in engine.hpp.
      // PT: Posição do ponteiro em pixels inteiros; modificadores raramente ativados no move
      //     mas repassados. espaço-janela -> espaço-conteúdo (F3, v0.2.5): subtrai o offset
      //     fornecido pelo chamador (0,0 para o App; origem de letterbox do UiLayer nos demais
      //     casos) antes de repassar ao RmlUi, que só conhece o próprio espaço de conteúdo
      //     offset-free -- ver o doc-comment deste método em engine.hpp.
      // EN: Guard (AUD-TEC-2): static_cast<int> of a non-finite float is UB ([conv.fpint]).
      //     A host may forward NaN/inf coords (driver glitch, network replay) -- reject before
      //     the cast, same rule already enforced by set_element_scroll_top.
      // PT: Guard (AUD-TEC-2): static_cast<int> de float não-finito é UB ([conv.fpint]). Um
      //     host pode repassar coords NaN/inf (glitch de driver, replay de rede) -- rejeita
      //     antes do cast, mesma regra já aplicada em set_element_scroll_top.
      if (!std::isfinite(ev.x) || !std::isfinite(ev.y)) break;
      impl_->boot.process_mouse_move(static_cast<int>(ev.x) - offset_x,
                                     static_cast<int>(ev.y) - offset_y, ev.modifiers);
      break;

    case T::MouseButton:
      // EN: button: 0=left, 1=right, 2=middle — matches Rml convention directly (translation
      //     itself now lives in Bootstrap::process_mouse_button, RMLX-0/F2).
      // PT: button: 0=esquerdo, 1=direito, 2=meio — bate diretamente com a convenção Rml
      //     (tradução em si agora mora em Bootstrap::process_mouse_button, RMLX-0/F2).
      impl_->boot.process_mouse_button(ev.button, ev.pressed, ev.modifiers);
      break;

    case T::Key:
      // EN: Gamepad nav: Key::Up/Down/Left/Right/Enter/Escape → Rml arrow/Return/Escape (the
      //     Key->Rml::Input::KeyIdentifier mapping itself now lives in input_map.hpp/.cpp,
      //     RMLX-0/F2, driven from Bootstrap::process_key). RmlUi uses ProcessKeyDown + a
      //     subsequent ProcessTextInput for printable chars; for nav-only keys (arrows, Return,
      //     Escape, Tab) text input is not needed.
      // PT: Nav por gamepad: Key::Up/Down/Left/Right/Enter/Escape → Rml seta/Return/Escape (o
      //     mapeamento Key->Rml::Input::KeyIdentifier em si agora mora em input_map.hpp/.cpp,
      //     RMLX-0/F2, conduzido a partir de Bootstrap::process_key). RmlUi usa ProcessKeyDown +
      //     ProcessTextInput para chars imprimíveis; para teclas só de navegação (setas, Return,
      //     Escape, Tab) text input não é necessário.
      impl_->boot.process_key(ev.key, ev.pressed, ev.modifiers);
      break;

    case T::Text:
      // EN: UTF-8 text input — forwarded as-is to RmlUi (handles multi-byte sequences).
      //     text must remain valid for the duration of this call; caller owns the lifetime.
      // PT: Entrada de texto UTF-8 — repassado ao RmlUi como-está (lida com sequências multi-byte).
      //     text deve permanecer válido durante esta chamada; lifetime é do chamador.
      if (ev.text) impl_->boot.process_text(ev.text);
      break;

    case T::MouseWheel:
      // EN: Wheel/trackpad delta forwarding (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven by
      //     GusWorld's 30-item overflow-y:auto menu list, which could not scroll in embed mode).
      //     Unlike MouseMove/MouseButton, NO offset translation here: ev.x/ev.y are a DELTA
      //     (scroll amount), not a window-space position, so there is nothing to subtract
      //     offset_x/offset_y from -- RmlUi's Context::ProcessMouseWheel(Vector2f, mods) (called
      //     from Bootstrap::process_mouse_wheel, RMLX-0/F2) takes the delta as-is and scrolls
      //     whatever element is currently under `hover` (set by the most recent
      //     ProcessMouseMove/ProcessMouseButtonDown -- confirmed by reading Context.cpp: it
      //     reads the `hover` member, resolves Element::GetClosestScrollableContainer() from
      //     there, and is a no-op when hover is null). The Vector2f overload is used, not the
      //     deprecated single-float one (see Context.h: "@deprecated Please use the Vector2f
      //     version").
      // PT: Encaminhamento de delta de roda/trackpad (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven
      //     pela lista de menu de 30 itens overflow-y:auto do GusWorld, que não conseguia rolar em
      //     embed mode). Diferente de MouseMove/MouseButton, SEM tradução de offset aqui: ev.x/
      //     ev.y são um DELTA (quantidade de rolagem), não uma posição espaço-janela, então não
      //     há nada para subtrair de offset_x/offset_y -- o
      //     Context::ProcessMouseWheel(Vector2f, mods) do RmlUi (chamado a partir de
      //     Bootstrap::process_mouse_wheel, RMLX-0/F2) recebe o delta como está e rola o
      //     elemento que estiver sob `hover` no momento (definido pelo ProcessMouseMove/
      //     ProcessMouseButtonDown mais recente -- confirmado lendo Context.cpp: lê o membro
      //     `hover`, resolve Element::GetClosestScrollableContainer() a partir dele, e é no-op
      //     quando hover é nulo). Usa a sobrecarga de Vector2f, não a de float único deprecada
      //     (ver Context.h: "@deprecated Please use the Vector2f version").
      // EN: Guard (AUD-TEC-2): a non-finite delta would poison the scroll offset with NaN
      //     inside RmlUi. Reject before forwarding -- same rule as MouseMove above.
      // PT: Guard (AUD-TEC-2): um delta não-finito envenenaria o offset de scroll com NaN
      //     dentro do RmlUi. Rejeita antes de repassar -- mesma regra do MouseMove acima.
      if (!std::isfinite(ev.x) || !std::isfinite(ev.y)) break;
      impl_->boot.process_mouse_wheel(ev.x, ev.y, ev.modifiers);
      break;

    case T::Resize:
      // EN: Deliberately NOT handled here -- Resize mutates facade-owned state (UiLayer's
      //     letterbox offset, App's window-size cache), not just an RmlUi call. Each caller
      //     handles it locally; see this method's doc-comment in engine.hpp.
      // PT: Deliberadamente NÃO tratado aqui -- Resize muta estado de posse da fachada (offset
      //     de letterbox do UiLayer, cache de tamanho de janela do App), não é só uma chamada
      //     ao RmlUi. Cada chamador trata localmente; ver o doc-comment deste método em
      //     engine.hpp.
      break;

    default:
      // EN: Unknown/future event types are intentionally ignored — forward compatibility.
      // PT: Tipos de evento desconhecidos/futuros são ignorados intencionalmente — compatibilidade futura.
      break;
  }
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

bool Engine::scroll_element_into_view(const char* id, ScrollAlign align) const {
  if (!impl_->ok) return false;
  return impl_->boot.scroll_element_into_view(id, align);
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

bool Engine::load_font_face(const FontFaceDesc& desc) {
  if (!impl_->ok) return false;
  return impl_->boot.load_font_face(desc);
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

// EN: `FRAMEGRAB-EMBED` -- see this method's own doc-comment (engine.hpp) for the full
//     contract. Pure readback: renders nothing, only reads whatever FBO 0 already holds.
//     GL STATE HYGIENE (this is what keeps the compose-only promise for UiLayer's own
//     caller): unlike render_compose above, this method does NOT use GlStateGuard (that
//     class only covers the render-pass state -- viewport/blend/program/VAO/etc -- none of
//     which a plain glReadPixels touches). It saves and restores, by hand, the exact 5 pieces
//     of GL state a pixel readback CAN perturb: GL_READ_FRAMEBUFFER binding, GL_READ_BUFFER,
//     GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, and the GL_PIXEL_PACK_BUFFER binding (a bound
//     PBO would silently redirect glReadPixels's 4th argument from a pointer to a buffer
//     OFFSET -- unbinding it for the read, then restoring the host's own binding afterward,
//     is what makes this call safe to use even when a host renders through a pixel-pack PBO
//     of its own). GL_PACK_ALIGNMENT is forced to 1 (tightly-packed rows) for the read
//     itself -- this is the fix for the "width not a multiple of 4 corrupts rows" class of
//     bug the default alignment of 4 causes (NOT "odd width" -- the review adversarial's own
//     reclassification, `w21_framegrab_embed` thread, 2026-07-30: gcd(3,4)=1, so an EVEN
//     width that is not a multiple of 4, e.g. 2/6/198, corrupts the exact same way; this
//     unconditional fix already covers the whole class regardless): a default-4-byte-aligned
//     read of a width*3-byte-per-pixel RGB row whose byte count is not itself a multiple of
//     4 silently pads each row to the next 4-byte boundary, which the row_bytes_src stride
//     below does NOT account for, corrupting every row after the first -- restored to
//     whatever the host had afterward, never left at 1.
// PT: `FRAMEGRAB-EMBED` -- ver o próprio doc-comment deste método (engine.hpp) pro contrato
//     completo. Readback puro: não renderiza nada, só lê o que o FBO 0 já guarda.
//     HIGIENE DE ESTADO GL (é isto que sustenta a promessa compose-only pro próprio chamador
//     do UiLayer): diferente do render_compose acima, este método NÃO usa GlStateGuard
//     (aquela classe só cobre o estado do passe de render -- viewport/blend/programa/VAO/etc
//     -- nada disso um glReadPixels puro toca). Salva e restaura, à mão, as exatas 5 peças de
//     estado GL que um readback de pixel PODE perturbar: binding de GL_READ_FRAMEBUFFER,
//     GL_READ_BUFFER, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, e o binding de
//     GL_PIXEL_PACK_BUFFER (um PBO vinculado redirecionaria silenciosamente o 4º argumento do
//     glReadPixels de um ponteiro pra um OFFSET de buffer -- desvinculá-lo pra leitura, depois
//     restaurar o binding próprio do host depois, é o que torna esta chamada segura de usar
//     mesmo quando um host renderiza através de um PBO de pixel-pack próprio).
//     GL_PACK_ALIGNMENT é forçado a 1 (linhas compactadas sem padding) pra própria leitura --
//     este é o conserto pra classe de bug "largura não múltipla de 4 corrompe linhas" que o
//     alinhamento default de 4 causa (NÃO "largura ímpar" -- a própria reclassificação do
//     review adversarial, thread do `w21_framegrab_embed`, 2026-07-30: mdc(3,4)=1, então uma
//     largura PAR que não é múltipla de 4, ex. 2/6/198, corrompe exatamente do mesmo jeito;
//     este conserto incondicional já cobre a classe inteira de qualquer forma): uma leitura
//     com alinhamento default-4-bytes de uma linha RGB de 3-bytes-por-pixel cuja contagem de
//     bytes não é ela mesma múltipla de 4 preenche silenciosamente cada linha até o próximo
//     limite de 4 bytes, o que o passo row_bytes_src abaixo NÃO leva em conta, corrompendo
//     toda linha após a primeira -- restaurado ao que o host tinha depois, nunca deixado em 1.
// EN: CAPTURE-NOTHROW (W22, 2026-07-30) -- this method allocates TWO buffers below (the
//     intermediate `rgb` scratch, `w*h*3` bytes, and the final `out.pixels`, `w*h*4` bytes,
//     coexisting simultaneously during the flip/expand loop) with NO guard against
//     `std::bad_alloc` before this fix, despite `engine.hpp`'s own doc-comment already promising
//     a clean, default-constructed `CapturedFramePixels{}` on every OTHER documented failure
//     (`!impl_->ok`, `w<=0||h<=0`) -- the SAME shape of gap the `never a crash` family
//     (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`) already closed elsewhere in this
//     codebase, found here by the promise-vs-code MATRIX audit (W22), not by grepping for the
//     literal phrase (this method's own doc-comment never used those words). `App::capture_frame()`
//     (`app.cpp`) and `UiLayer::capture_frame()` (`ui_layer.cpp`) both delegate to this ONE shared
//     method (FRAMEGRAB-EMBED) -- fixing it here closes the gap for BOTH public entry points at
//     once, same as every other Engine-level fix in this class. The THIRD, independent duplicate
//     of this exact algorithm, `capture_framebuffer()` (`frame_capture.cpp`, CAPTURE-FREE), is
//     fixed separately in that file -- see this method's own top comment there for why it is a
//     deliberate code duplication, not a shared call, and for the ADDITIONAL byte-count cap that
//     ONLY that instance-free entry point needs (a real window/viewport already bounds `w`/`h`
//     here; `capture_framebuffer()` has no instance to bound the plausible range of its own
//     caller-supplied `w`/`h`).
//     `noexcept` ADDED to this method's own declaration (`engine.hpp`) -- only after auditing
//     this method's OWN body statement-by-statement, not by convention: every call inside the
//     `try` below is either a plain C GL entry point (`glGetIntegerv`/`glBindFramebuffer`/
//     `glReadBuffer`/`glBindBuffer`/`glPixelStorei`/`glReadPixels` -- a C ABI, structurally
//     incapable of throwing a C++ exception) or one of the two allocations this fix now guards
//     (`std::vector`'s own ctor, `std::make_unique<unsigned char[]>`) -- nothing else. The two
//     guard clauses ABOVE the `try` (`!impl_->ok`, `w<=0||h<=0`) are plain member reads/int
//     comparisons, no call that could throw either. With every statement accounted for, marking
//     THIS method noexcept is safe by construction, not a guess -- UNLIKE its two PUBLIC callers,
//     `App::capture_frame()` (`app.cpp`) and `UiLayer::capture_frame()` (`ui_layer.cpp`), which
//     this fatia does NOT mark noexcept: `UiLayer::capture_frame()`'s own body, re-audited the
//     same way, turns out to ALSO qualify (see that method's own doc-comment, `ui_layer.hpp`),
//     but `App::capture_frame()` additionally calls `sync_viewport()`/`render_frame()`/the buffer
//     swap -- the FULL RmlUi render pipeline, layout included -- which this fatia has NOT audited
//     statement-by-statement; the team lead's own explicit instruction was "if in doubt, do not
//     mark noexcept, and say why", so `App::capture_frame()` stays as it was on this specific
//     point, its own doc-comment (`app.hpp`) stating the reason.
// PT: CAPTURE-NOTHROW (W22, 2026-07-30) -- este método aloca DOIS buffers abaixo (o scratch
//     intermediário `rgb`, `w*h*3` bytes, e o `out.pixels` final, `w*h*4` bytes, coexistindo
//     simultaneamente durante o laço de flip/expansão) SEM guarda nenhuma contra `std::bad_alloc`
//     antes deste conserto, apesar do próprio doc-comment de `engine.hpp` já prometer um
//     `CapturedFramePixels{}` limpo, default-construído, em toda OUTRA falha documentada
//     (`!impl_->ok`, `w<=0||h<=0`) -- a MESMA forma de lacuna que a família `never a crash`
//     (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`) já fechou em outro lugar deste
//     código-base, achada aqui pela auditoria de MATRIZ promessa-vs-código (W22), não por grep da
//     frase literal (o próprio doc-comment deste método nunca usou essas palavras).
//     `App::capture_frame()` (`app.cpp`) e `UiLayer::capture_frame()` (`ui_layer.cpp`) os dois
//     delegam pra este ÚNICO método compartilhado (FRAMEGRAB-EMBED) -- consertar aqui fecha a
//     lacuna pros DOIS pontos de entrada públicos de uma vez, mesmo padrão de todo outro conserto
//     em nível de Engine desta classe. A TERCEIRA duplicata independente deste exato algoritmo, o
//     `capture_framebuffer()` (`frame_capture.cpp`, CAPTURE-FREE), é consertada separadamente
//     naquele arquivo -- ver o próprio comentário de topo deste método lá pro porquê de ser uma
//     duplicação de código deliberada, não uma chamada compartilhada, e pro teto de contagem de
//     bytes ADICIONAL que SÓ aquele ponto de entrada sem-instância precisa (uma janela/viewport
//     real já limita `w`/`h` aqui; o `capture_framebuffer()` não tem instância nenhuma pra limitar
//     a faixa plausível do próprio `w`/`h` fornecido pelo chamador).
//     `noexcept` SOMADO à própria declaração deste método (`engine.hpp`) -- só depois de auditar
//     o PRÓPRIO corpo deste método instrução-por-instrução, não por convenção: toda chamada
//     dentro do `try` abaixo ou é um entry point C puro do GL (`glGetIntegerv`/
//     `glBindFramebuffer`/`glReadBuffer`/`glBindBuffer`/`glPixelStorei`/`glReadPixels` -- ABI C,
//     estruturalmente incapaz de lançar uma exceção C++) ou é uma das duas alocações que este
//     conserto agora guarda (o próprio construtor do `std::vector`, `std::make_unique<unsigned
//     char[]>`) -- nada além disso. As duas guardas ACIMA do `try` (`!impl_->ok`,
//     `w<=0||h<=0`) são leitura de membro/comparação de int puras, nenhuma chamada que pudesse
//     lançar tampouco. Com toda instrução contabilizada, marcar ESTE método noexcept é seguro por
//     construção, não um chute -- DIFERENTE dos dois chamadores PÚBLICOS dele, `App::capture_frame()`
//     (`app.cpp`) e `UiLayer::capture_frame()` (`ui_layer.cpp`), que esta fatia NÃO marca
//     noexcept: o próprio corpo do `UiLayer::capture_frame()`, reauditado do mesmo jeito, acaba
//     TAMBÉM se qualificando (ver o próprio doc-comment daquele método, `ui_layer.hpp`), mas o
//     `App::capture_frame()` adicionalmente chama `sync_viewport()`/`render_frame()`/o swap de
//     buffer -- o pipeline de render do RmlUi INTEIRO, layout incluso -- que esta fatia NÃO
//     auditou instrução-por-instrução; a própria instrução explícita do team lead foi "na dúvida,
//     não marque noexcept, e diga por quê", então o `App::capture_frame()` fica como estava neste
//     ponto específico, com o próprio doc-comment (`app.hpp`) dizendo o motivo.
CapturedFramePixels Engine::capture_frame(int gl_x, int gl_y, int w, int h) const noexcept {
  if (!impl_->ok) return CapturedFramePixels{};
  if (w <= 0 || h <= 0) return CapturedFramePixels{};

  // EN: CAPTURE-PACKSKIP (W28, 2026-08-06, CTO-directed) -- GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS
  //     ADDED to the save-neutralize-restore block below, alongside the pre-existing 5 slots (now
  //     7). Found by a consumer under Mesa/llvmpipe against the byte-for-byte-identical block in
  //     `capture_framebuffer()` (`frame_capture.cpp`, CAPTURE-FREE) -- this method has the SAME
  //     gap (it is that function's own documented algorithmic twin) and is the MORE-reached of the
  //     two: both `App::capture_frame()` and `UiLayer::capture_frame()` delegate here
  //     (FRAMEGRAB-EMBED), so this is the shared implementation behind the library's two primary
  //     capture entry points, not a duplicate found and fixed independently. See
  //     `frame_capture.cpp`'s own top-of-try comment for the full derivation (heap-buffer-overflow
  //     WRITE mechanism, why only these two of the eight PACK pixel-store parameters were
  //     unguarded, and the Mesa-only-measured caveat) -- not re-derived here to avoid drifting out
  //     of sync with that account of it.
  // PT: CAPTURE-PACKSKIP (W28, 2026-08-06, dirigido pelo CTO) -- GL_PACK_SKIP_PIXELS/
  //     GL_PACK_SKIP_ROWS SOMADOS ao bloco de salvar-neutralizar-restaurar abaixo, ao lado dos 5
  //     slots pré-existentes (agora 7). Achado por um consumidor sob Mesa/llvmpipe contra o bloco
  //     idêntico byte-a-byte em `capture_framebuffer()` (`frame_capture.cpp`, CAPTURE-FREE) --
  //     este método tem a MESMA lacuna (é o próprio gêmeo algorítmico documentado daquela função)
  //     e é o MAIS ALCANÇADO dos dois: tanto `App::capture_frame()` quanto
  //     `UiLayer::capture_frame()` delegam pra cá (FRAMEGRAB-EMBED), então esta é a implementação
  //     compartilhada por trás dos dois pontos de entrada de captura primários da biblioteca, não
  //     uma duplicata achada e consertada independentemente. Ver o próprio comentário de topo do
  //     `try` de `frame_capture.cpp` pra derivação completa (mecanismo de escrita
  //     heap-buffer-overflow, por que só estes dois dos oito parâmetros de pixel-store PACK
  //     ficaram sem guarda, e a ressalva de só-medido-sob-Mesa) -- não re-derivado aqui pra não
  //     desviar da própria explicação lá.
  try {
    GLint prev_read_fbo = 0, prev_read_buffer = 0;
    GLint prev_pack_alignment = 4, prev_pack_row_length = 0, prev_pack_pbo = 0;
    GLint prev_pack_skip_pixels = 0, prev_pack_skip_rows = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &prev_pack_row_length);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_pack_pbo);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &prev_pack_skip_pixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &prev_pack_skip_rows);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    std::vector<unsigned char> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3);
    glReadPixels(gl_x, gl_y, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

    glPixelStorei(GL_PACK_ALIGNMENT, prev_pack_alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, prev_pack_row_length);
    glPixelStorei(GL_PACK_SKIP_PIXELS, prev_pack_skip_pixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, prev_pack_skip_rows);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(prev_pack_pbo));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prev_read_fbo));
    glReadBuffer(static_cast<GLenum>(prev_read_buffer));

    // EN: Row-flip (glReadPixels origin is bottom-left; CapturedFramePixels row 0 is top -- same
    //     convention App::CapturedFrame/UiLayer::CapturedFrame document) + RGB -> RGBA8 expansion
    //     with synthetic alpha=255 -- IDENTICAL to what App::capture_frame's own body used to do
    //     inline before FRAMEGRAB-EMBED (see this method's own doc-comment, engine.hpp, and
    //     App::CapturedFrame's doc-comment, app.hpp, for the full "why 255" rationale).
    // PT: Inversão de linha (a origem do glReadPixels é bottom-left; a linha 0 de
    //     CapturedFramePixels é o topo -- mesma convenção que App::CapturedFrame/
    //     UiLayer::CapturedFrame documentam) + expansão RGB -> RGBA8 com alpha sintético=255 --
    //     IDÊNTICO ao que o próprio corpo de App::capture_frame fazia inline antes do
    //     FRAMEGRAB-EMBED (ver o próprio doc-comment deste método, engine.hpp, e o doc-comment
    //     de App::CapturedFrame, app.hpp, pro racional completo do "por que 255").
    CapturedFramePixels out;
    out.width = w;
    out.height = h;
    out.byte_count = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    out.pixels = std::make_unique<unsigned char[]>(out.byte_count);
    const size_t row_bytes_src = static_cast<size_t>(w) * 3;
    // EN: Hoisted out of the loop (same pattern applied to the byte-for-byte-identical loop in
    //     frame_capture.cpp, CI-LINT-RED): besides avoiding a repeated `.get()` call per row,
    //     cppcheck 2.13.0 (CI's ubuntu-latest version, TST-L1-STATIC) can misinfer a `.get()`
    //     call chained straight into pointer arithmetic as `void*` (arithOperationsOnVoidPointer)
    //     even though `out.pixels` is `unique_ptr<unsigned char[]>` and `.get()` is
    //     unambiguously `unsigned char*` by the standard. This exact spot was not flagged in the
    //     CI run that found the sibling case in frame_capture.cpp, but the identical pattern
    //     here is equally exposed to the same tool limitation -- fixed proactively rather than
    //     waiting for a future CI run to go red on it (auditoria-dominó).
    // PT: Extraído do laço (mesmo padrão aplicado ao laço byte-a-byte idêntico em
    //     frame_capture.cpp, CI-LINT-RED): além de evitar uma chamada `.get()` repetida por
    //     linha, o cppcheck 2.13.0 (versão do ubuntu-latest do CI, TST-L1-STATIC) pode inferir
    //     errado uma chamada `.get()` encadeada direto em aritmética de ponteiro como `void*`
    //     (arithOperationsOnVoidPointer), mesmo `out.pixels` sendo `unique_ptr<unsigned char[]>`
    //     e `.get()` sendo inequivocamente `unsigned char*` pela norma. Este ponto exato não foi
    //     sinalizado na rodada de CI que achou o caso irmão em frame_capture.cpp, mas o padrão
    //     idêntico aqui está igualmente exposto à mesma limitação da ferramenta -- corrigido
    //     proativamente em vez de esperar uma rodada futura de CI ficar vermelha por causa dele
    //     (auditoria-dominó).
    unsigned char* const pixels_base = out.pixels.get();
    for (int dst_row = 0; dst_row < h; ++dst_row) {
      const int src_row = h - 1 - dst_row;
      const unsigned char* src = rgb.data() + static_cast<size_t>(src_row) * row_bytes_src;
      unsigned char* dst = pixels_base + static_cast<size_t>(dst_row) * static_cast<size_t>(w) * 4;
      for (int x = 0; x < w; ++x) {
        dst[x * 4 + 0] = src[x * 3 + 0];
        dst[x * 4 + 1] = src[x * 3 + 1];
        dst[x * 4 + 2] = src[x * 3 + 2];
        dst[x * 4 + 3] = 255;
      }
    }
    out.ok = true;
    return out;
  } catch (const std::exception&) {
    // EN: std::bad_alloc (the expected case -- see this method's own top comment) or any other
    //     std::exception -- degrade to a clean, default-constructed CapturedFramePixels{}
    //     (ok == false), never a partially-filled result.
    // PT: std::bad_alloc (o caso esperado -- ver o próprio comentário de topo deste método) ou
    //     qualquer outro std::exception -- degrada pra um CapturedFramePixels{} limpo,
    //     default-construído (ok == false), nunca um resultado parcialmente preenchido.
    return CapturedFramePixels{};
  } catch (...) {
    // EN/PT: belt-and-suspenders, same "never a crash" discipline as this codebase's own
    // load_texture()/load_font()/decode_*()/encode_*() catch(...) blocks -- an exception type
    // that is not even a std::exception must not escape this boundary either.
    return CapturedFramePixels{};
  }
}

} // namespace glintfx
