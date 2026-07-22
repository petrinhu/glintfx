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
#include <RmlUi/Core/Input.h>
#include <cmath>  // EN: std::isfinite (dp-ratio input hardening / process_event guards). PT: std::isfinite (hardening do dp-ratio / guards do process_event).

namespace glintfx {

// ---------------------------------------------------------------------------
// EN: Internal translation helpers — never exposed in public headers. Moved here from
//     ui_layer.cpp (A1, framework-2D, docs/superpowers/plans/2026-07-19-framework2d-A1-input.md
//     section 2.1) so process_event() below is the ONE place both App and UiLayer route
//     through -- see that method's doc-comment in engine.hpp.
// PT: Helpers internos de tradução — nunca expostos nos headers públicos. Movidos para cá de
//     ui_layer.cpp (A1, framework-2D, docs/superpowers/plans/2026-07-19-framework2d-A1-input.md
//     seção 2.1) para que process_event() abaixo seja o ÚNICO lugar por onde App e UiLayer
//     passam -- ver o doc-comment desse método em engine.hpp.
// ---------------------------------------------------------------------------

// EN: Map glintfx::Key to Rml::Input::KeyIdentifier.
//     Gamepad nav: Up/Down/Left/Right → arrow keys; Enter → Return; Escape stays Escape.
//     Tab drives RmlUi's built-in focus cycle; Space/Backspace for text widget editing.
//     AUD-PUB-3 (v0.5.0): Delete/Home/End/PageUp/PageDown → KI_DELETE/KI_HOME/KI_END/KI_PRIOR/
//     KI_NEXT — names confirmed by grepping the pinned RmlUi source
//     (build/_deps/rmlui-src/Include/RmlUi/Core/Input.h:115-128; KI_PRIOR is Page Up,
//     KI_NEXT is Page Down — RmlUi's own naming, not a typo here).
// PT: Mapeia glintfx::Key para Rml::Input::KeyIdentifier.
//     Nav por gamepad: Up/Down/Left/Right → setas; Enter → Return; Escape permanece Escape.
//     Tab aciona o ciclo de foco interno do RmlUi; Space/Backspace para edição em widget de texto.
//     AUD-PUB-3 (v0.5.0): Delete/Home/End/PageUp/PageDown → KI_DELETE/KI_HOME/KI_END/KI_PRIOR/
//     KI_NEXT — nomes confirmados grepando o source pinado do RmlUi
//     (build/_deps/rmlui-src/Include/RmlUi/Core/Input.h:115-128; KI_PRIOR é Page Up,
//     KI_NEXT é Page Down — nomenclatura do próprio RmlUi, não é erro de digitação aqui).
static Rml::Input::KeyIdentifier to_rml_key(Key k) noexcept {
  switch (k) {
    case Key::Up:        return Rml::Input::KI_UP;
    case Key::Down:      return Rml::Input::KI_DOWN;
    case Key::Left:      return Rml::Input::KI_LEFT;
    case Key::Right:     return Rml::Input::KI_RIGHT;
    case Key::Enter:     return Rml::Input::KI_RETURN;
    case Key::Escape:    return Rml::Input::KI_ESCAPE;
    case Key::Tab:       return Rml::Input::KI_TAB;
    case Key::Space:     return Rml::Input::KI_SPACE;
    case Key::Backspace: return Rml::Input::KI_BACK;
    case Key::Delete:    return Rml::Input::KI_DELETE;
    case Key::Home:      return Rml::Input::KI_HOME;
    case Key::End:       return Rml::Input::KI_END;
    case Key::PageUp:    return Rml::Input::KI_PRIOR;
    case Key::PageDown:  return Rml::Input::KI_NEXT;
    default:             return Rml::Input::KI_UNKNOWN;
  }
}

// EN: Map glintfx::Mod bitmask to Rml::Input::KeyModifier bitmask.
//     glintfx::Mod_Shift=1, Mod_Ctrl=2, Mod_Alt=4
//     Rml::Input::KM_CTRL=1, KM_SHIFT=2, KM_ALT=4
// PT: Mapeia bitmask de glintfx::Mod para bitmask de Rml::Input::KeyModifier.
static int to_rml_mods(int mods) noexcept {
  int result = 0;
  if (mods & Mod_Shift) result |= Rml::Input::KM_SHIFT;  // EN: Shift. PT: Shift.
  if (mods & Mod_Ctrl)  result |= Rml::Input::KM_CTRL;   // EN: Ctrl.  PT: Ctrl.
  if (mods & Mod_Alt)   result |= Rml::Input::KM_ALT;    // EN: Alt.   PT: Alt.
  return result;
}

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
    Rml::Log::Message(Rml::Log::LT_WARNING,
                      "set_viewport(%d, %d) ignored -- viewport dimensions must both be positive "
                      "(previous viewport kept).",
                      w, h);
    return;
  }
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

void Engine::process_event(const UiEvent& ev, int offset_x, int offset_y) {
  // EN: Guard: drop events when the engine is not ready or context is gone (same shape as
  //     every other Engine method — "!ok() -> no-op"). Moved verbatim from
  //     UiLayer::process_event (A1, framework-2D).
  // PT: Guard: descarta eventos quando o engine não está pronto ou o contexto sumiu (mesmo
  //     formato de todo outro método do Engine — "!ok() -> no-op"). Movido tal-e-qual de
  //     UiLayer::process_event (A1, framework-2D).
  if (!impl_->ok) return;
  Rml::Context* c = impl_->boot.context();
  if (!c) return;

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
      c->ProcessMouseMove(static_cast<int>(ev.x) - offset_x, static_cast<int>(ev.y) - offset_y,
                          to_rml_mods(ev.modifiers));
      break;

    case T::MouseButton:
      // EN: button: 0=left, 1=right, 2=middle — matches Rml convention directly.
      // PT: button: 0=esquerdo, 1=direito, 2=meio — bate diretamente com a convenção Rml.
      if (ev.pressed)
        c->ProcessMouseButtonDown(ev.button, to_rml_mods(ev.modifiers));
      else
        c->ProcessMouseButtonUp(ev.button, to_rml_mods(ev.modifiers));
      break;

    case T::Key:
      // EN: Gamepad nav: Key::Up/Down/Left/Right/Enter/Escape → Rml arrow/Return/Escape.
      //     RmlUi uses ProcessKeyDown + a subsequent ProcessTextInput for printable chars;
      //     for nav-only keys (arrows, Return, Escape, Tab) text input is not needed.
      // PT: Nav por gamepad: Key::Up/Down/Left/Right/Enter/Escape → Rml seta/Return/Escape.
      //     RmlUi usa ProcessKeyDown + ProcessTextInput para chars imprimíveis;
      //     para teclas só de navegação (setas, Return, Escape, Tab) text input não é necessário.
      if (ev.pressed)
        c->ProcessKeyDown(to_rml_key(ev.key), to_rml_mods(ev.modifiers));
      else
        c->ProcessKeyUp(to_rml_key(ev.key), to_rml_mods(ev.modifiers));
      break;

    case T::Text:
      // EN: UTF-8 text input — forwarded as-is to RmlUi (handles multi-byte sequences).
      //     text must remain valid for the duration of this call; caller owns the lifetime.
      // PT: Entrada de texto UTF-8 — repassado ao RmlUi como-está (lida com sequências multi-byte).
      //     text deve permanecer válido durante esta chamada; lifetime é do chamador.
      if (ev.text) c->ProcessTextInput(Rml::String(ev.text));
      break;

    case T::MouseWheel:
      // EN: Wheel/trackpad delta forwarding (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven by
      //     GusWorld's 30-item overflow-y:auto menu list, which could not scroll in embed mode).
      //     Unlike MouseMove/MouseButton, NO offset translation here: ev.x/ev.y are a DELTA
      //     (scroll amount), not a window-space position, so there is nothing to subtract
      //     offset_x/offset_y from -- RmlUi's Context::ProcessMouseWheel(Vector2f, mods) takes
      //     the delta as-is and scrolls whatever element is currently under `hover` (set by the
      //     most recent ProcessMouseMove/ProcessMouseButtonDown -- confirmed by reading
      //     Context.cpp: it reads the `hover` member, resolves
      //     Element::GetClosestScrollableContainer() from there, and is a no-op when hover is
      //     null). The Vector2f overload is used, not the deprecated single-float one (see
      //     Context.h: "@deprecated Please use the Vector2f version").
      // PT: Encaminhamento de delta de roda/trackpad (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven
      //     pela lista de menu de 30 itens overflow-y:auto do GusWorld, que não conseguia rolar em
      //     embed mode). Diferente de MouseMove/MouseButton, SEM tradução de offset aqui: ev.x/
      //     ev.y são um DELTA (quantidade de rolagem), não uma posição espaço-janela, então não
      //     há nada para subtrair de offset_x/offset_y -- o
      //     Context::ProcessMouseWheel(Vector2f, mods) do RmlUi recebe o delta como está e rola o
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
      c->ProcessMouseWheel(Rml::Vector2f(ev.x, ev.y), to_rml_mods(ev.modifiers));
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
