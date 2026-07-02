// SPDX-License-Identifier: MPL-2.0
// EN: Embed/guest facade — attaches the UI+effects engine to a GL context the HOST owns.
//     Does not create a window; render() is compose-only (no clear, no swap).
// PT: Fachada embed/guest — anexa o motor de UI+efeitos a um contexto GL do HOST.
//     Não cria janela; render() é compose-only (sem clear, sem swap).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <memory>
#include <cstddef>
#include <functional>
#include <glintfx/ui_event.hpp>
#include <glintfx/element_box.hpp>

namespace glintfx {

// EN: Configuration for UiLayer — specifies logical viewport dimensions and GL loader policy.
//     Declared at namespace scope so it is a complete type when used as a default argument
//     in the UiLayer constructor (inner-class default-args hit a C++ language restriction).
// PT: Configuração para UiLayer — especifica dimensões lógicas do viewport e política do GL loader.
//     Declarada em namespace scope para ser tipo completo ao ser usada como argumento padrão
//     no construtor do UiLayer (default-args de inner-class atingem uma restrição da linguagem C++).
struct UiLayerConfig {
  int logical_width  = 1280;
  int logical_height = 720;
  bool load_gl       = true;
  // EN: Initial density-independent pixel ratio (dp_ratio).
  //     1 dp = dp_ratio physical pixels. Set > 1.0 when the host renders at a higher
  //     physical resolution than the logical RCSS canvas (e.g. dp_ratio=2.0 means 1dp=2px).
  //     Can be updated at runtime via set_dp_ratio(). Default 1.0 = no scaling.
  // PT: Density-independent pixel ratio inicial (dp_ratio).
  //     1 dp = dp_ratio pixels físicos. Defina > 1.0 quando o host renderiza numa resolução
  //     física maior que o canvas lógico RCSS (ex.: dp_ratio=2.0 significa 1dp=2px).
  //     Pode ser atualizado em runtime via set_dp_ratio(). Padrão 1.0 = sem escala.
  float dp_ratio = 1.0f;
};

// EN: MOVED-FROM STATE (L1.10-APIDOC): after `UiLayer b = std::move(a);` (or
//     `b = std::move(a);`), `a` is left in a moved-from state. Calling ANY method on `a`
//     other than ok() or the destructor is undefined behaviour — same convention as
//     std::unique_ptr. ok() on a moved-from UiLayer returns false (impl_ is null), which is
//     the only state query that is safe to make; destruction of a moved-from UiLayer is
//     always safe (no-op, impl_ is null).
// PT: ESTADO MOVIDO-DE (L1.10-APIDOC): após `UiLayer b = std::move(a);` (ou
//     `b = std::move(a);`), `a` fica em estado movido-de. Chamar QUALQUER método em `a`
//     além de ok() ou o destrutor é comportamento indefinido — mesma convenção do
//     std::unique_ptr. ok() num UiLayer movido-de retorna false (impl_ é nulo), sendo a
//     única consulta de estado segura; destruir um UiLayer movido-de é sempre seguro
//     (no-op, impl_ é nulo).
class UiLayer {
public:
  // EN: Inner alias so callers can write UiLayer::Config{...} — matches the spec interface.
  // PT: Alias interno para que chamadores possam escrever UiLayer::Config{...} — segue a spec.
  using Config = UiLayerConfig;

  // EN: Requires a CURRENT GL context owned by the host.
  //     GL function pointers are loaded (idempotent per-process) when cfg.load_gl is true.
  // PT: Exige contexto GL CORRENTE do host.
  //     Ponteiros de função GL são carregados (idempotente por processo) quando cfg.load_gl for true.
  explicit UiLayer(Config cfg = Config{});
  ~UiLayer();

  // EN: Move leaves the source in a moved-from state — see the class-level comment above
  //     for the exact contract (only ok() and ~UiLayer() remain valid on the source afterwards).
  // PT: O move deixa a origem em estado movido-de — ver o comentário de nível de classe
  //     acima para o contrato exato (só ok() e ~UiLayer() permanecem válidos na origem depois).
  UiLayer(UiLayer&&) noexcept;
  UiLayer& operator=(UiLayer&&) noexcept;
  UiLayer(const UiLayer&)            = delete;
  UiLayer& operator=(const UiLayer&) = delete;

  // EN: True when attach + engine init succeeded.
  // PT: True quando attach + init do motor foram bem-sucedidos.
  bool ok() const noexcept;

  // EN: Load a document from rml_path and call Show(). Requires ok().
  //     Returns true on success.
  // PT: Carrega documento de rml_path e chama Show(). Requer ok().
  //     Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Notify of a viewport resize (real target pixels).
  // PT: Notifica redimensionamento de viewport (pixels reais do alvo).
  void set_viewport(int w, int h);

  // EN: Update the density-independent pixel ratio at runtime.
  //     1 dp (in RCSS) = ratio physical pixels. Triggers a layout re-pass when changed.
  //     Example: host renders 1920×1080 with RCSS authored at 960×540 → set_dp_ratio(2.0f).
  //     Pairs with set_viewport(real_w, real_h) which stays in physical pixels.
  // PT: Atualiza o density-independent pixel ratio em runtime.
  //     1 dp (no RCSS) = ratio pixels físicos. Dispara re-layout quando alterado.
  //     Exemplo: host renderiza 1920×1080 com RCSS no canvas lógico 960×540 → set_dp_ratio(2.0f).
  //     Usa com set_viewport(real_w, real_h) que permanece em pixels físicos.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL for asset resolution (fonts, images, RCSS).
  //     When set, relative paths passed to Open() are resolved as base_url/path.
  //     Absolute paths are unaffected. Call before load() for the URL to take effect.
  //     Pass nullptr or "" to clear. Safe to call after ok() is true.
  // PT: Sobrepõe o base URL para resolução de assets (fontes, imagens, RCSS).
  //     Quando definido, caminhos relativos passados a Open() são resolvidos como base_url/path.
  //     Caminhos absolutos não são afetados. Chame antes de load() para o URL ter efeito.
  //     Passe nullptr ou "" para limpar. Seguro chamar após ok() ser true.
  void set_asset_base_url(const char* url);

  // EN: Advance the UI context by one step (call once per frame before render()).
  // PT: Avança o contexto de UI um passo (chamar uma vez por frame antes de render()).
  void update();

  // EN: Compose-only render -- overlays the UI onto the window backbuffer (FBO 0) via
  //     premultiplied-alpha blend. No glClear. No buffer swap. GL state is automatically
  //     saved and restored on return (viewport, blend func+equation, scissor, program, VAO,
  //     texture, draw-FBO binding, colour mask).
  //
  //     F1 contract limitation: the internal graphics backend unconditionally targets the
  //     window backbuffer (FBO 0) during EndFrame; a custom host FBO bound before this call
  //     is NOT used as the composition target (see ADR-0008). The host must render its scene
  //     into the window backbuffer and call this method after drawing its scene, before swap.
  //     The viewport origin is hardcoded at (0,0); sub-region compositing is not supported.
  //
  // PT: Render compose-only -- sobrepõe a UI ao backbuffer da janela (FBO 0) via blend
  //     premultiplied-alpha. Sem glClear. Sem swap. O estado GL é salvo e restaurado
  //     automaticamente ao retornar (viewport, blend func+equation, scissor, programa, VAO,
  //     textura, draw-FBO binding, colour mask).
  //
  //     Limitação de contrato F1: o backend gráfico interno mira incondicionalmente o backbuffer
  //     da janela (FBO 0) durante EndFrame; um FBO customizado do host ligado antes desta chamada
  //     NÃO é usado como alvo de composição (ver ADR-0008). O host deve renderizar sua cena no
  //     backbuffer da janela e chamar este método após desenhar a cena, antes do swap.
  //     A origem do viewport é hardcoded em (0,0); composição em sub-região não é suportada.
  void render();

  // EN: Inject a host input event — translated to engine input calls internally (T4).
  // PT: Injeta evento de entrada do host — traduzido para chamadas de input do motor internamente (T4).
  void process_event(const UiEvent& ev);

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none reached before the document root).
  //     No ordering constraint versus load(): safe to call before or after it -- the listener
  //     always reads the CURRENT callback at click time, not the value at attach time. Parity
  //     with App::set_click_callback (same signature).
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum for encontrado até a raiz do
  //     documento). Sem restrição de ordem vs. load(): seguro chamar antes ou depois -- o
  //     listener sempre lê o callback CORRENTE no momento do clique, não o valor no attach.
  //     Paridade com App::set_click_callback (mesma assinatura).
  void set_click_callback(std::function<void(const char* element_id)> cb);

  // EN: Query the border-box geometry of an element by id. Coordinate space: window/render-
  //     target physical pixels, top-left origin, y-down -- the SAME space as UiEvent's mouse
  //     coordinates (see ElementBox doc-comment and docs/embed-integration.md section 10).
  //     Border-box (includes border, excludes margin). Returns ElementBox{found=false} (all
  //     fields zeroed) when the id is not found in the currently loaded document, or no
  //     document is loaded yet. Parity with App::get_element_box (same signature).
  // PT: Consulta a geometria border-box de um elemento por id. Espaço de coordenadas: pixels
  //     físicos do render-target/janela, origem superior-esquerda, y pra baixo -- o MESMO
  //     espaço das coordenadas de mouse do UiEvent (ver doc-comment de ElementBox e
  //     docs/embed-integration.md seção 10). Border-box (inclui borda, exclui margem). Retorna
  //     ElementBox{found=false} (todos os campos zerados) quando o id não é encontrado no
  //     documento carregado atualmente, ou nenhum documento carregado ainda. Paridade com
  //     App::get_element_box (mesma assinatura).
  ElementBox get_element_box(const char* id) const;

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1) — parity with App. Call order: create_data_model
  //     -> bind_* -> load() -> set_*. All methods forward to Engine; the Engine
  //     enforces the ordering constraint (bind_* after load() returns false).
  //     API uses only plain C types (const char*, double, bool, size_t).
  // PT: API de data-model (T1) — paridade com App. Ordem de chamada:
  //     create_data_model -> bind_* -> load() -> set_*(). Todos os métodos
  //     encaminham ao Engine; o Engine enforça a restrição de ordem (bind_*
  //     após load() retorna false). API usa apenas tipos C simples
  //     (const char*, double, bool, size_t).
  // -------------------------------------------------------------------------

  // EN: Create a data model. Call before bind_* and before load().
  // PT: Cria um data model. Chamar antes de bind_* e antes de load().
  bool create_data_model(const char* name);

  // EN: Bind a numeric (double) cell with an optional initial value.
  // PT: Liga uma célula numérica (double) com valor inicial opcional.
  bool bind_number(const char* key, double initial = 0.0);

  // EN: Bind a string cell.
  // PT: Liga uma célula de string.
  bool bind_string(const char* key, const char* initial = "");

  // EN: Bind a boolean cell.
  // PT: Liga uma célula booleana.
  bool bind_bool  (const char* key, bool initial = false);

  // EN: Bind a string-list cell (for data-for iteration in RML).
  // PT: Liga uma célula de lista de strings (para iteração data-for no RML).
  bool bind_list  (const char* key);

  // EN: Update a numeric cell after load. No-op when key is unknown.
  //     The change is reflected on the next update() call.
  // PT: Atualiza uma célula numérica após load. No-op quando a chave é desconhecida.
  //     A mudança é refletida na próxima chamada a update().
  void set_number(const char* key, double value);

  // EN: Update a string cell after load.
  // PT: Atualiza uma célula de string após load.
  void set_string(const char* key, const char* value);

  // EN: Update a boolean cell after load.
  // PT: Atualiza uma célula booleana após load.
  void set_bool  (const char* key, bool value);

  // EN: Replace the entire string list and dirty the variable.
  //     items[0..count-1] are copied; caller retains ownership.
  // PT: Substitui a lista de strings inteira e marca a variável suja.
  //     items[0..count-1] são copiados; o chamador retém a propriedade.
  void set_list  (const char* key, const char* const* items, std::size_t count);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
