// SPDX-License-Identifier: MPL-2.0
// EN: UiLayer implementation — embed/guest facade that attaches the RmlUi+GL3 engine to
//     a GL context owned by the host. Owns a SystemClock (no GLFW) and an Engine.
// PT: Implementação do UiLayer — fachada embed/guest que anexa o motor RmlUi+GL3 a um
//     contexto GL do host. Possui SystemClock (sem GLFW) e Engine.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl3w must be included before any other OpenGL header (defines GL function pointers).
// PT: gl3w deve ser incluído antes de qualquer outro header OpenGL (define ponteiros de função GL).
#include <GL/gl3w.h>

#include <glintfx/ui_layer.hpp>
#include "engine.hpp"
#include "system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

namespace glintfx {

// ---------------------------------------------------------------------------
// EN: Internal translation helpers — never exposed in public headers.
// PT: Helpers internos de tradução — nunca expostos nos headers públicos.
// ---------------------------------------------------------------------------

// EN: Map glintfx::Key to Rml::Input::KeyIdentifier.
//     Gamepad nav: Up/Down/Left/Right → arrow keys; Enter → Return; Escape stays Escape.
//     Tab drives RmlUi's built-in focus cycle; Space/Backspace for text widget editing.
// PT: Mapeia glintfx::Key para Rml::Input::KeyIdentifier.
//     Nav por gamepad: Up/Down/Left/Right → setas; Enter → Return; Escape permanece Escape.
//     Tab aciona o ciclo de foco interno do RmlUi; Space/Backspace para edição em widget de texto.
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
  //     gl3wInit() is idempotent within one process — subsequent calls are fast no-ops.
  //     Skip when the host has already initialised gl3w (cfg.load_gl = false).
  // PT: Carrega ponteiros de função GL contra o contexto CORRENTE do host.
  //     gl3wInit() é idempotente dentro de um processo — chamadas subsequentes são no-ops rápidos.
  //     Pular quando o host já inicializou o gl3w (cfg.load_gl = false).
  if (cfg.load_gl) {
    if (gl3wInit() != 0) return;
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
  impl_->w = w;
  impl_->h = h;
  impl_->x = impl_->y = impl_->gl_offset_x = impl_->gl_offset_y = 0;
  impl_->target_h = h;
  impl_->letterbox_mode = false;
  impl_->engine.set_viewport(w, h);
}

void UiLayer::set_viewport(int x, int y, int w, int h, int target_h) {
  if (!impl_->ok) return;
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
