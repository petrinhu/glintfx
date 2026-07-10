// SPDX-License-Identifier: MPL-2.0
// EN: UiLayer implementation — embed/guest facade that attaches the RmlUi+GL3 engine to
//     a GL context owned by the host. Owns a SystemClock (no GLFW) and an Engine.
// PT: Implementação do UiLayer — fachada embed/guest que anexa o motor RmlUi+GL3 a um
//     contexto GL do host. Possui SystemClock (sem GLFW) e Engine.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_loader.h must be included before any other OpenGL header (defines GL function pointers).
// PT: gl_loader.h deve ser incluído antes de qualquer outro header OpenGL (define ponteiros de função GL).
#include "gl_loader.h"

#include <glintfx/ui_layer.hpp>
#include "engine.hpp"
#include "system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <cmath>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).

namespace glintfx {

// EN: Sane ceiling for viewport dimensions/offsets (AUD-TEC-4). No real display or letterbox
//     layout needs more than this; the point is to keep `target_h - y - h` (set_viewport's
//     gl_offset_y computation, below) comfortably inside int range so it can never
//     signed-overflow (UB) even with adversarial input close to INT_MAX.
// PT: Teto são para dimensões/offsets de viewport (AUD-TEC-4). Nenhum display ou layout de
//     letterbox real precisa de mais que isto; o objetivo é manter `target_h - y - h` (o
//     cálculo de gl_offset_y do set_viewport, abaixo) folgado dentro do range de int, para que
//     nunca dê overflow com sinal (UB) mesmo com entrada adversarial perto de INT_MAX.
constexpr int kMaxViewportDim = 32768;

// ---------------------------------------------------------------------------
// EN: Internal translation helpers — never exposed in public headers.
// PT: Helpers internos de tradução — nunca expostos nos headers públicos.
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

struct UiLayer::Impl {
  SystemClock clock;  // EN: minimal RmlUi SystemInterface (clock only, no GLFW).
                      // PT: SystemInterface mínimo do RmlUi (só relógio, sem GLFW).
  Engine      engine;
  int  w   = 0;
  int  h   = 0;
  // EN: F3 (v0.2.5) -- see set_viewport() doc comments in ui_layer.hpp for the full contract.
  //     x, y: public, top-down window-space offset (mirrors UiEvent's convention).
  //     target_h: last target_h given to the 5-arg overload; re-used by the Resize event
  //           handler below to keep gl_offset_y consistent when content h changes via
  //           process_event() instead of via an explicit set_viewport() call.
  //     gl_offset_x/y: OpenGL-native (bottom-left) offset, PRE-COMPUTED here and passed
  //           through unchanged every frame in render() -- render() itself does zero math.
  //     letterbox_mode: false = legacy 2-arg path (offset always (0,0), Resize never touches
  //           it). true = 5-arg path -- Resize recomputes gl_offset_y (not gl_offset_x, x
  //           does not depend on h).
  // PT: F3 (v0.2.5) -- ver doc-comments de set_viewport() em ui_layer.hpp pro contrato
  //     completo. x, y: offset público, espaço-janela top-down (espelha a convenção do
  //     UiEvent). target_h: último target_h passado à sobrecarga de 5 args; reusado pelo
  //     handler de evento Resize abaixo pra manter gl_offset_y consistente quando o h de
  //     conteúdo muda via process_event() em vez de via set_viewport() explícito.
  //     gl_offset_x/y: offset nativo do OpenGL (inferior-esquerda), PRÉ-CALCULADO aqui e
  //     repassado sem mudança a cada frame em render() -- o render() em si não faz conta
  //     nenhuma. letterbox_mode: false = caminho legado de 2 args (offset sempre (0,0),
  //     Resize nunca o toca). true = caminho de 5 args -- Resize recalcula gl_offset_y (não
  //     gl_offset_x, x não depende de h).
  int  x = 0, y = 0, target_h = 0;
  int  gl_offset_x = 0, gl_offset_y = 0;
  bool letterbox_mode = false;
  bool ok  = false;
};

UiLayer::UiLayer(Config cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.logical_width;
  impl_->h = cfg.logical_height;

  // EN: Load GL function pointers against the host's CURRENT context.
  //     glx_gl_load() is idempotent within one process — a repeat call just re-resolves
  //     and overwrites the same ~344 pointers (cheap, no allocation kept around).
  //     Skip when the host has already initialised the loader (cfg.load_gl = false).
  // PT: Carrega ponteiros de função GL contra o contexto CORRENTE do host.
  //     glx_gl_load() é idempotente dentro de um processo — uma chamada repetida apenas
  //     re-resolve e sobrescreve os mesmos ~344 ponteiros (barato, sem alocação retida).
  //     Pular quando o host já inicializou o loader (cfg.load_gl = false).
  if (cfg.load_gl) {
    if (glx_gl_load() != 0) return;
  }

  impl_->ok = impl_->engine.attach(&impl_->clock, impl_->w, impl_->h);
  // EN: Apply the initial dp_ratio to the newly created context.
  //     SetDensityIndependentPixelRatio is idempotent when value equals the context default
  //     (1.0f), so always calling it here is safe and makes the intent explicit.
  // PT: Aplica o dp_ratio inicial ao contexto recém-criado.
  //     SetDensityIndependentPixelRatio é idempotente quando o valor iguala o padrão do
  //     contexto (1.0f), então chamá-lo sempre aqui é seguro e deixa a intenção explícita.
  if (impl_->ok) impl_->engine.set_dp_ratio(cfg.dp_ratio);
}

UiLayer::~UiLayer() = default;

UiLayer::UiLayer(UiLayer&&) noexcept = default;
UiLayer& UiLayer::operator=(UiLayer&&) noexcept = default;

bool UiLayer::ok() const noexcept {
  return impl_ && impl_->ok;
}

bool UiLayer::load(const char* rml_path) {
  if (!impl_->ok) return false;
  return impl_->engine.load(rml_path);
}

void UiLayer::set_viewport(int w, int h) {
  if (!impl_->ok) return;
  // EN: Input-hardening (audit, v0.3.0). w/h are viewport dimensions fed to
  //     Rml::Context::SetDimensions; a zero/negative dimension degenerates the layout engine.
  //     Skip silently and keep the previous viewport -- this replicates the exact guard that
  //     already lives in process_event(Resize) (`if (ev.width <= 0 || ev.height <= 0) break;`);
  //     the two set_viewport overloads were the ones that lacked it. x/y/target_h are NOT
  //     validated: a legitimate letterbox offset can be negative.
  // PT: Hardening de entrada (auditoria, v0.3.0). w/h são dimensões de viewport passadas a
  //     Rml::Context::SetDimensions; uma dimensão zero/negativa degenera o motor de layout.
  //     Ignora silenciosamente e mantém o viewport anterior -- replica o guard exato que já
  //     existe em process_event(Resize) (`if (ev.width <= 0 || ev.height <= 0) break;`); as duas
  //     sobrecargas de set_viewport eram as que não o tinham. x/y/target_h NÃO são validados: um
  //     offset de letterbox legítimo pode ser negativo.
  // EN: Guard (AUD-TEC-4): also cap w/h at a sane ceiling -- kMaxViewportDim is shared with the
  //     5-arg overload below so both reject the same range.
  // PT: Guard (AUD-TEC-4): também limita w/h a um teto são -- kMaxViewportDim é compartilhado
  //     com a sobrecarga de 5 args abaixo para que ambas rejeitem o mesmo range.
  if (w <= 0 || h <= 0 || w > kMaxViewportDim || h > kMaxViewportDim) return;
  impl_->w = w;
  impl_->h = h;
  impl_->x = impl_->y = impl_->gl_offset_x = impl_->gl_offset_y = 0;
  impl_->target_h = h;
  impl_->letterbox_mode = false;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::set_viewport(int x, int y, int w, int h, int target_h) {
  if (!impl_->ok) return;
  // EN: Same w/h guard as the 2-arg overload above (input-hardening audit, v0.3.0). Only w/h are
  //     checked here -- x/y (letterbox origin) can legitimately be any value including negative
  //     offsets. target_h (total window height) must stay positive, see AUD-TEC-4 below. Keeps
  //     the previous viewport on invalid dimensions.
  // PT: Mesmo guard de w/h da sobrecarga de 2 args acima (auditoria de hardening, v0.3.0). Aqui
  //     só w/h são checados -- x/y (origem do letterbox) podem legitimamente ser qualquer valor,
  //     incluindo offsets negativos. target_h (altura total da janela) precisa continuar
  //     positivo, ver AUD-TEC-4 abaixo. Mantém o viewport anterior em dimensões inválidas.
  // EN: Guard (AUD-TEC-4): x/y/target_h/w/h all capped at a sane ceiling so
  //     `target_h - y - h` below cannot signed-overflow (UB) -- e.g. target_h=INT_MAX would
  //     transiently underflow int range as the subtraction proceeds. w/h and target_h must stay
  //     positive (> 0, target_h is the total window height, same rule as the 2-arg overload's
  //     w/h); only x/y may legitimately be negative (letterbox origin) but not adversarially
  //     huge in either direction.
  // PT: Guard (AUD-TEC-4): x/y/target_h/w/h todos limitados a um teto são para que
  //     `target_h - y - h` abaixo não dê overflow com sinal (UB) -- ex.: target_h=INT_MAX
  //     faria a subtração transbordar o range de int. w/h e target_h precisam continuar
  //     positivos (> 0, target_h é a altura total da janela, mesma regra do w/h da sobrecarga
  //     de 2 args); só x/y podem legitimamente ser negativos (origem de letterbox) mas não
  //     adversarialmente enormes em nenhuma direção.
  if (w <= 0 || h <= 0 || w > kMaxViewportDim || h > kMaxViewportDim ||
      x < -kMaxViewportDim || x > kMaxViewportDim ||
      y < -kMaxViewportDim || y > kMaxViewportDim ||
      target_h <= 0 || target_h > kMaxViewportDim) {
    return;
  }
  impl_->x = x;
  impl_->y = y;
  impl_->w = w;
  impl_->h = h;
  impl_->target_h = target_h;
  impl_->letterbox_mode = true;
  impl_->gl_offset_x = x;
  impl_->gl_offset_y = target_h - y - h;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::set_dp_ratio(float ratio) {
  if (!impl_->ok) return;
  impl_->engine.set_dp_ratio(ratio);
}

void UiLayer::set_asset_base_url(const char* url) {
  if (!impl_->ok) return;
  impl_->engine.set_asset_base_url(url);
}

void UiLayer::update() {
  if (!impl_->ok) return;
  impl_->engine.update();
}

void UiLayer::render() {
  // EN: Delegate to Engine::render_compose — compose UI over the host's current FBO
  //     without clearing and without swapping buffers. GL state is saved/restored internally.
  // PT: Delega a Engine::render_compose — compõe a UI sobre o FBO corrente do host
  //     sem limpar e sem trocar buffers. Estado GL é salvo/restaurado internamente.
  if (impl_->ok)
    impl_->engine.render_compose(impl_->gl_offset_x, impl_->gl_offset_y, impl_->w, impl_->h);
}

// ---------------------------------------------------------------------------
// EN: Data-model API — forward to Engine; Engine guards the ordering constraint.
// PT: API de data-model — encaminha ao Engine; Engine enforça a restrição de ordem.
// ---------------------------------------------------------------------------

bool UiLayer::create_data_model(const char* name) {
  if (!impl_->ok) return false;
  return impl_->engine.create_data_model(name);
}

bool UiLayer::bind_number(const char* key, double initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_number(key, initial);
}

bool UiLayer::bind_string(const char* key, const char* initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_string(key, initial);
}

bool UiLayer::bind_bool(const char* key, bool initial) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_bool(key, initial);
}

bool UiLayer::bind_list(const char* key) {
  if (!impl_->ok) return false;
  return impl_->engine.bind_list(key);
}

void UiLayer::set_number(const char* key, double value) {
  if (!impl_->ok) return;
  impl_->engine.set_number(key, value);
}

void UiLayer::set_string(const char* key, const char* value) {
  if (!impl_->ok) return;
  impl_->engine.set_string(key, value);
}

void UiLayer::set_bool(const char* key, bool value) {
  if (!impl_->ok) return;
  impl_->engine.set_bool(key, value);
}

void UiLayer::set_list(const char* key, const char* const* items, std::size_t count) {
  if (!impl_->ok) return;
  impl_->engine.set_list(key, items, count);
}

void UiLayer::set_click_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_click_callback(std::move(cb));
}

void UiLayer::set_click_info_callback(std::function<void(const ClickInfo&)> cb) {
  if (!impl_->ok) return;
  // EN: AUD-PUB-4 (v0.5.0): unlike set_click_callback (id-only, no coordinate translation
  //     needed), the ClickInfo the Engine/Bootstrap hand back carries x/y in CONTENT-LOCAL
  //     space (offset-free -- see Engine::set_click_info_callback's doc-comment). Wrap the
  //     host's callback in a translating lambda that adds the current sub-viewport offset
  //     (impl_->x/y) before forwarding -- the SAME translation get_element_box() applies
  //     (box.x = x + impl_->x). Captures a raw Impl* (not `this`/shared_ptr): safe because the
  //     wrapped lambda is stored inside Bootstrap::Impl::click_info_cb, which lives inside
  //     impl_->engine's Bootstrap, which is a member of THIS UiLayer::Impl -- the callback
  //     cannot outlive the Impl it points into. Reads impl_->x/y at INVOCATION time (not
  //     capture time) so a later set_viewport() call is honoured for every subsequent click.
  // PT: AUD-PUB-4 (v0.5.0): diferente de set_click_callback (só-id, sem tradução de
  //     coordenada necessária), o ClickInfo que o Engine/Bootstrap devolvem carrega x/y no
  //     espaço LOCAL DE CONTEÚDO (offset-free -- ver o doc-comment de
  //     Engine::set_click_info_callback). Envolve o callback do host numa lambda tradutora que
  //     soma o offset de sub-viewport corrente (impl_->x/y) antes de repassar -- a MESMA
  //     tradução que get_element_box() aplica (box.x = x + impl_->x). Captura um Impl* cru (não
  //     `this`/shared_ptr): seguro porque a lambda envolvida fica armazenada dentro de
  //     Bootstrap::Impl::click_info_cb, que vive dentro do Bootstrap de impl_->engine, que é
  //     membro DESTE UiLayer::Impl -- o callback não pode sobreviver ao Impl para o qual
  //     aponta. Lê impl_->x/y no momento da INVOCAÇÃO (não no momento da captura), então uma
  //     chamada posterior a set_viewport() é respeitada em todo clique subsequente.
  Impl* self = impl_.get();
  impl_->engine.set_click_info_callback(
      [self, cb = std::move(cb)](const ClickInfo& info) {
        if (!cb) return;
        ClickInfo translated = info;
        translated.x += static_cast<float>(self->x);
        translated.y += static_cast<float>(self->y);
        cb(translated);
      });
}

void UiLayer::set_scroll_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  // EN: Straight passthrough (GLINTFX-SCROLL-1 follow-up, v0.6.0) -- unlike
  //     set_click_info_callback, there is no coordinate payload here to translate by the
  //     sub-viewport offset (impl_->x/y): the callback carries only an id string.
  // PT: Repasse direto (desdobramento do GLINTFX-SCROLL-1, v0.6.0) -- diferente do
  //     set_click_info_callback, não há payload de coordenada aqui a traduzir pelo offset de
  //     sub-viewport (impl_->x/y): o callback carrega só uma string de id.
  impl_->engine.set_scroll_callback(std::move(cb));
}

// EN: L1.15-FORMEV -- straight passthrough, no coordinate translation (none of these five
//     payloads carry geometry -- id [+ value for change] only, same reasoning as
//     set_scroll_callback's lack of offset math above). See the doc-comments in ui_layer.hpp/
//     bootstrap.hpp for the full contract.
// PT: L1.15-FORMEV -- repasse direto, sem tradução de coordenada (nenhum dos cinco payloads
//     carrega geometria -- só id [+ value pro change], mesma racional da ausência de matemática
//     de offset do set_scroll_callback acima). Ver os doc-comments em ui_layer.hpp/
//     bootstrap.hpp para o contrato completo.
void UiLayer::set_change_callback(std::function<void(const char*, const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_change_callback(std::move(cb));
}

void UiLayer::set_submit_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_submit_callback(std::move(cb));
}

void UiLayer::set_focus_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_focus_callback(std::move(cb));
}

void UiLayer::set_blur_callback(std::function<void(const char*)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_blur_callback(std::move(cb));
}

void UiLayer::set_hover_callback(std::function<void(const char*, bool)> cb) {
  if (!impl_->ok) return;
  impl_->engine.set_hover_callback(std::move(cb));
}

ElementBox UiLayer::get_element_box(const char* id) const {
  ElementBox box;
  if (!impl_->ok) return box;
  float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
  if (!impl_->engine.get_element_box(id, x, y, w, h)) return box;
  box.found = true;
  // EN: content-space -> window-space: add the sub-viewport offset (F3, v0.2.5 --
  //     impl_->x/y are (0,0) in legacy mode, so this is a no-op unless
  //     set_viewport(x,y,w,h,target_h) placed the sub-viewport elsewhere).
  // PT: espaço-conteúdo -> espaço-janela: soma o offset da sub-viewport (F3, v0.2.5 --
  //     impl_->x/y são (0,0) em modo legado, então é no-op a menos que
  //     set_viewport(x,y,w,h,target_h) tenha posicionado a sub-viewport em outro lugar).
  box.x = x + static_cast<float>(impl_->x);
  box.y = y + static_cast<float>(impl_->y);
  box.w = w; box.h = h;
  return box;
}

bool UiLayer::scroll_element_into_view(const char* id, bool align_with_top) const {
  if (!impl_->ok) return false;
  return impl_->engine.scroll_element_into_view(id, align_with_top);
}

bool UiLayer::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_element_scroll_top(id, out_scroll_top);
}

bool UiLayer::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_element_scroll_height(id, out_scroll_height);
}

bool UiLayer::get_element_client_height(const char* id, float& out_client_height) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_element_client_height(id, out_client_height);
}

bool UiLayer::set_element_scroll_top(const char* id, float scroll_top) const {
  if (!impl_->ok) return false;
  return impl_->engine.set_element_scroll_top(id, scroll_top);
}

bool UiLayer::set_focus(const char* id) const {
  if (!impl_->ok) return false;
  // EN: Straight passthrough (L1.17-FOCUS) -- no coordinate translation applicable, same
  //     reasoning as the scroll trio above (id-only, no x/y in either direction).
  // PT: Repasse direto (L1.17-FOCUS) -- sem tradução de coordenada aplicável, mesma
  //     racional do trio de rolagem acima (só-id, sem x/y em nenhuma direção).
  return impl_->engine.set_focus(id);
}

bool UiLayer::clear_focus() const {
  if (!impl_->ok) return false;
  return impl_->engine.clear_focus();
}

// ---------------------------------------------------------------------------
// EN: DOM read/write by id (L1.16-DOMRW) -- straight forwards to Engine, no coordinate
//     translation applicable (same reasoning as set_focus/scroll trio above: these operate on
//     text/classes/properties, not on geometry).
// PT: Leitura/escrita de DOM por id (L1.16-DOMRW) -- repasse direto ao Engine, sem tradução de
//     coordenada aplicável (mesma racional do set_focus/trio de rolagem acima: operam sobre
//     texto/classes/propriedades, não geometria).
// ---------------------------------------------------------------------------

bool UiLayer::set_text(const char* id, const char* text) const {
  if (!impl_->ok) return false;
  return impl_->engine.set_text(id, text);
}

bool UiLayer::add_class(const char* id, const char* cls) const {
  if (!impl_->ok) return false;
  return impl_->engine.add_class(id, cls);
}

bool UiLayer::remove_class(const char* id, const char* cls) const {
  if (!impl_->ok) return false;
  return impl_->engine.remove_class(id, cls);
}

bool UiLayer::set_property(const char* id, const char* prop, const char* value) const {
  if (!impl_->ok) return false;
  return impl_->engine.set_property(id, prop, value);
}

bool UiLayer::get_number(const char* key, double& out) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_number(key, out);
}

bool UiLayer::get_string(const char* key, std::string& out) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_string(key, out);
}

bool UiLayer::get_bool(const char* key, bool& out) const {
  if (!impl_->ok) return false;
  return impl_->engine.get_bool(key, out);
}

void UiLayer::process_event(const UiEvent& ev) {
  // EN: Guard: drop events when the layer is not ready or context is gone.
  // PT: Guard: descarta eventos quando a camada não está pronta ou contexto sumiu.
  if (!impl_->ok) return;
  Rml::Context* c = impl_->engine.context();
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
      //     window-space -> content-space (F3, v0.2.5): subtract the sub-viewport offset
      //     (0,0 in legacy mode) before forwarding to RmlUi, which only knows its own
      //     offset-free content space -- see set_viewport()'s doc comment.
      // PT: Posição do ponteiro em pixels inteiros; modificadores raramente ativados no move
      //     mas repassados. espaço-janela -> espaço-conteúdo (F3, v0.2.5): subtrai o offset da
      //     sub-viewport (0,0 em modo legado) antes de repassar ao RmlUi, que só conhece o
      //     próprio espaço de conteúdo offset-free -- ver o doc-comment de set_viewport().
      // EN: Guard (AUD-TEC-2): static_cast<int> of a non-finite float is UB ([conv.fpint]).
      //     A host may forward NaN/inf coords (driver glitch, network replay) -- reject before
      //     the cast, same rule already enforced by set_element_scroll_top.
      // PT: Guard (AUD-TEC-2): static_cast<int> de float não-finito é UB ([conv.fpint]). Um
      //     host pode repassar coords NaN/inf (glitch de driver, replay de rede) -- rejeita
      //     antes do cast, mesma regra já aplicada em set_element_scroll_top.
      if (!std::isfinite(ev.x) || !std::isfinite(ev.y)) break;
      c->ProcessMouseMove(static_cast<int>(ev.x) - impl_->x, static_cast<int>(ev.y) - impl_->y,
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
      //     Unlike MouseMove/MouseButton, NO sub-viewport offset translation here: ev.x/ev.y are
      //     a DELTA (scroll amount), not a window-space position, so there is nothing to subtract
      //     impl_->x/y from -- RmlUi's Context::ProcessMouseWheel(Vector2f, mods) takes the delta
      //     as-is and scrolls whatever element is currently under `hover` (set by the most recent
      //     ProcessMouseMove/ProcessMouseButtonDown -- confirmed by reading Context.cpp: it reads
      //     the `hover` member, resolves Element::GetClosestScrollableContainer() from there, and
      //     is a no-op when hover is null). The Vector2f overload is used, not the deprecated
      //     single-float one (see Context.h: "@deprecated Please use the Vector2f version").
      // PT: Encaminhamento de delta de roda/trackpad (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven
      //     pela lista de menu de 30 itens overflow-y:auto do GusWorld, que não conseguia rolar em
      //     embed mode). Diferente de MouseMove/MouseButton, SEM tradução de offset de sub-
      //     viewport aqui: ev.x/ev.y são um DELTA (quantidade de rolagem), não uma posição
      //     espaço-janela, então não há nada para subtrair de impl_->x/y -- o
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
      // EN: Logical viewport resize — update cached dimensions and notify RmlUi.
      //     The next render_compose() uses the updated impl_->w/h via set_viewport().
      // PT: Redimensionamento lógico do viewport — atualiza dimensões cacheadas e notifica RmlUi.
      //     O próximo render_compose() usa o impl_->w/h atualizado via set_viewport().

      // EN: Guard: zero/negative dimensions are invalid for the layout engine — skip silently.
      // PT: Guard: dimensões zero/negativas são inválidas para o motor de layout — ignorar silenciosamente.
      if (ev.width <= 0 || ev.height <= 0) break;

      impl_->w = ev.width;
      impl_->h = ev.height;
      c->SetDimensions(Rml::Vector2i(ev.width, ev.height));
      // EN: F3 (v0.2.5) -- keep gl_offset_y consistent with the new h when in letterbox mode
      //     (x/target_h stay whatever the last explicit set_viewport(x,y,w,h,target_h) set).
      //     Legacy mode stays at offset 0 -- unchanged behaviour, gl_offset_x untouched (does
      //     not depend on h).
      // PT: F3 (v0.2.5) -- mantém gl_offset_y consistente com o novo h em modo letterbox
      //     (x/target_h continuam o que o último set_viewport(x,y,w,h,target_h) explícito
      //     setou). Modo legado permanece em offset 0 -- comportamento inalterado,
      //     gl_offset_x intocado (não depende de h).
      if (impl_->letterbox_mode)
        impl_->gl_offset_y = impl_->target_h - impl_->y - impl_->h;
      break;

    default:
      // EN: Unknown/future event types are intentionally ignored — forward compatibility.
      // PT: Tipos de evento desconhecidos/futuros são ignorados intencionalmente — compatibilidade futura.
      break;
  }
}

} // namespace glintfx
