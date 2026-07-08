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
#include <glintfx/click_info.hpp>

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

  // EN: Notify a viewport resize (2-arg, unchanged): real target pixels, no sub-region
  //     offset -- equivalent to set_viewport(0, 0, w, h, /*target_h=*/h).
  // PT: Notifica redimensionamento de viewport (2 args, inalterado): pixels reais do alvo,
  //     sem offset de sub-região -- equivalente a set_viewport(0, 0, w, h, /*target_h=*/h).
  void set_viewport(int w, int h);

  // EN: Notify a viewport resize AND placement within a larger host render target (letterbox,
  //     F3, v0.2.5). (x, y): top-down, window-space pixel offset of the sub-viewport's
  //     top-left corner -- the SAME convention as UiEvent's mouse coordinates (see
  //     process_event below) and as get_element_box()'s return value. (w, h): the
  //     sub-viewport's own logical/content dimensions (RmlUi context size), unaffected by
  //     the offset. target_h: the FULL height of the host's render target (its window), in
  //     physical pixels -- used ONLY to convert (x, y) into the OpenGL-native bottom-up
  //     offset the final composite needs (glViewport is bottom-left-origin; see the
  //     ADR-0008 "F3 implementation update" section for the exact formula and rationale).
  //     No target_w parameter: the X axis needs no conversion between the two conventions.
  //     Precondition: target_h >= y + h (not enforced -- an inconsistent value composites at
  //     an unexpected but harmless on-screen position, never a crash).
  // PT: Notifica redimensionamento de viewport E posicionamento dentro de um render target
  //     maior do host (letterbox, F3, v0.2.5). (x, y): offset em pixels, espaço-janela
  //     topo-esquerda, do canto superior esquerdo da sub-viewport -- a MESMA convenção das
  //     coordenadas de mouse do UiEvent (ver process_event abaixo) e do retorno de
  //     get_element_box(). (w, h): as dimensões lógicas/de conteúdo da própria sub-viewport
  //     (tamanho do contexto RmlUi), não afetadas pelo offset. target_h: a altura TOTAL do
  //     render target do host (a janela dele), em pixels físicos -- usada SÓ pra converter
  //     (x, y) no offset nativo bottom-up que o OpenGL exige na composição final (glViewport
  //     tem origem inferior-esquerda; ver a seção "F3 implementation update" do ADR-0008 pra
  //     fórmula exata e justificativa). Sem parâmetro target_w: o eixo X não precisa de
  //     conversão entre as duas convenções. Pré-condição: target_h >= y + h (não enforçada --
  //     um valor inconsistente compõe numa posição de tela inesperada mas inofensiva, nunca
  //     crash).
  void set_viewport(int x, int y, int w, int h, int target_h);

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
  //     The viewport ORIGIN within FBO 0 is configurable since v0.2.5 via the 5-arg
  //     set_viewport(x, y, w, h, target_h) overload (letterbox) -- see its doc comment and
  //     the ADR-0008 "F3 implementation update" section. When letterboxing, the host must
  //     clear/redraw its scene across the FULL window (not just the sub-region) before
  //     calling render() -- render() itself never clears.
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
  //     A ORIGEM do viewport dentro do FBO 0 é configurável desde a v0.2.5 via a sobrecarga de
  //     5 args set_viewport(x, y, w, h, target_h) (letterbox) -- ver o doc-comment dela e a
  //     seção "F3 implementation update" do ADR-0008. Ao letterboxar, o host deve limpar/
  //     redesenhar sua cena na janela INTEIRA (não só a sub-região) antes de chamar render() --
  //     o render() em si nunca limpa.
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

  // EN: Register a RICHER click callback -- id + button + coordinates + double-click, a
  //     second/parallel channel alongside set_click_callback (AUD-PUB-4, v0.5.0). Does NOT
  //     replace set_click_callback -- both can be registered simultaneously and both fire
  //     independently on the same click (see ClickInfo's doc-comment for the full contract,
  //     including a double-click firing this callback 3 times total, and the `button` field's
  //     known limitation: effectively always 0, since RmlUi never dispatches Click/Dblclick for
  //     non-primary mouse buttons). Coordinate space: window/render-target physical pixels,
  //     top-left origin, y-down -- SAME convention as get_element_box (the sub-viewport offset
  //     from set_viewport(x,y,w,h,target_h) is added here, mirroring get_element_box's
  //     translation). No ordering constraint versus load(). Parity with
  //     App::set_click_info_callback (same signature).
  // PT: Registra um callback de clique MAIS RICO -- id + botão + coordenadas + duplo-clique,
  //     um segundo canal/paralelo ao set_click_callback (AUD-PUB-4, v0.5.0). NÃO substitui
  //     set_click_callback -- ambos podem ser registrados simultaneamente e ambos disparam
  //     independentemente no mesmo clique (ver o doc-comment de ClickInfo para o contrato
  //     completo, incluindo um duplo-clique disparando este callback 3 vezes no total, e a
  //     limitação conhecida do campo `button`: na prática sempre 0, já que o RmlUi nunca
  //     despacha Click/Dblclick para botões não-primários do mouse). Espaço de coordenadas:
  //     pixels físicos do render-target/janela, origem superior-esquerda, y pra baixo -- MESMA
  //     convenção de get_element_box (o offset de sub-viewport de
  //     set_viewport(x,y,w,h,target_h) é somado aqui, espelhando a tradução de
  //     get_element_box). Sem restrição de ordem vs. load(). Paridade com
  //     App::set_click_info_callback (mesma assinatura).
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

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

  // EN: Scroll an element's nearest scrolling ancestor(s) (e.g. an overflow-y:auto container)
  //     so the element becomes visible (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven by
  //     GusWorld: keyboard/gamepad selection moving inside a 30-item menu list had no way to
  //     "follow" the selection out of the visible area). Wraps Rml::Element::ScrollIntoView(bool):
  //     align_with_top true aligns the element to the top of the scrollable view, false to the
  //     bottom. Instant (synchronous) -- the resulting scroll offset is readable via
  //     get_element_scroll_top() immediately after this call returns, no update() needed in
  //     between. Returns false (no-op) when the id is not found in the currently loaded
  //     document, or no document is loaded yet -- same guard shape as get_element_box.
  //     Typical use, gamepad/keyboard menu navigation (the GusWorld scenario):
  //       // On Key::Down moving selection to the next item:
  //       ui.set_string("selected_id", next_item_id);        // app-side selection state
  //       ui.scroll_element_into_view(next_item_id);          // follow it into view
  // PT: Rola o(s) ancestral(is) rolável(eis) mais próximo(s) de um elemento (ex.: um container
  //     overflow-y:auto) para que ele fique visível (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven
  //     pelo GusWorld: a seleção por teclado/gamepad se movendo dentro de uma lista de menu de
  //     30 itens não tinha como "seguir" a seleção para fora da área visível). Encapsula
  //     Rml::Element::ScrollIntoView(bool): align_with_top true alinha o elemento ao topo da
  //     área rolável, false ao fundo. Instantâneo (síncrono) -- o offset de rolagem resultante
  //     é lido via get_element_scroll_top() imediatamente após esta chamada retornar, sem
  //     precisar de update() no meio. Retorna false (no-op) quando o id não é encontrado no
  //     documento carregado atualmente, ou nenhum documento carregado ainda -- mesmo formato de
  //     guard de get_element_box.
  //     Uso típico, navegação de menu por gamepad/teclado (o cenário do GusWorld):
  //       // No Key::Down movendo a seleção para o próximo item:
  //       ui.set_string("selected_id", next_item_id);        // estado de seleção do app
  //       ui.scroll_element_into_view(next_item_id);          // segue ele até a área visível
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset, in RCSS pixels (e.g. how far an
  //     overflow-y:auto container has been scrolled down). Returns false (out_scroll_top left
  //     untouched) when the id is not found or no document is loaded yet -- same guard shape as
  //     get_element_box. Chosen as bool + out-param (not an ElementBox-style aggregate struct):
  //     a single scalar does not benefit from a named 4-field struct the way border-box
  //     geometry does; this mirrors the internal Bootstrap/Engine signature 1:1 with no
  //     translation at this boundary.
  // PT: Consulta o offset de rolagem vertical próprio de um elemento, em pixels RCSS (ex.:
  //     quanto um container overflow-y:auto foi rolado para baixo). Retorna false
  //     (out_scroll_top intocado) quando o id não é encontrado ou nenhum documento carregado
  //     ainda -- mesmo formato de guard de get_element_box. Escolhido como bool + out-param (não
  //     um struct agregado no estilo ElementBox): um único escalar não se beneficia de um struct
  //     nomeado de 4 campos como a geometria border-box se beneficia; isso espelha 1:1 a
  //     assinatura interna de Bootstrap/Engine, sem tradução nesta fronteira.
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query the total scrollable content height of an element, in RCSS pixels (its own
  //     padding included, margin excluded -- same box-model slice RmlUi documents for
  //     GetScrollHeight()). Completes the scroll-metrics trio with get_element_scroll_top() and
  //     get_element_client_height() below -- exactly the three values RmlUi itself groups
  //     together (see Element.h) and everything a host needs to build a CUSTOM scrollbar (the
  //     builtin one already renders when the RCSS declares overflow-y: scroll/auto; this trio
  //     is for hosts that want to draw their own indicator instead, e.g. matching a game's HUD
  //     style):
  //       float scroll_top = 0.f, scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_top("list", scroll_top);
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       float scroll_fraction = (scroll_h > client_h)
  //           ? scroll_top / (scroll_h - client_h) : 0.f;      // 0..1, guard divide-by-zero
  //       float thumb_size_fraction = client_h / scroll_h;      // 0..1
  //     Returns false (out_scroll_height left untouched) when the id is not found or no
  //     document is loaded yet -- same guard shape as get_element_box.
  // PT: Consulta a altura total do conteúdo rolável de um elemento, em pixels RCSS (padding
  //     próprio incluso, margem excluída -- a mesma fatia do box-model que o RmlUi documenta
  //     para GetScrollHeight()). Completa o trio de métricas de rolagem com
  //     get_element_scroll_top() e get_element_client_height() abaixo -- exatamente os três
  //     valores que o próprio RmlUi agrupa (ver Element.h) e tudo que um host precisa para
  //     construir uma scrollbar CUSTOMIZADA (a embutida já renderiza quando o RCSS declara
  //     overflow-y: scroll/auto; este trio é para hosts que querem desenhar seu próprio
  //     indicador, ex.: casando com o estilo de HUD de um jogo):
  //       float scroll_top = 0.f, scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_top("list", scroll_top);
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       float fracao_rolada = (scroll_h > client_h)
  //           ? scroll_top / (scroll_h - client_h) : 0.f;      // 0..1, guardar divisão por zero
  //       float fracao_tamanho_thumb = client_h / scroll_h;     // 0..1
  //     Retorna false (out_scroll_height intocado) quando o id não é encontrado ou nenhum
  //     documento carregado ainda -- mesmo formato de guard de get_element_box.
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query the client (visible) height of an element, in RCSS pixels -- the third member of
  //     the scroll-metrics trio (see get_element_scroll_height()'s doc-comment above for the
  //     full usage formula). Returns false (out_client_height left untouched) when the id is
  //     not found or no document is loaded yet -- same guard shape as get_element_box.
  // PT: Consulta a altura de cliente (visível) de um elemento, em pixels RCSS -- o terceiro
  //     membro do trio de métricas de rolagem (ver o doc-comment de get_element_scroll_height()
  //     acima para a fórmula de uso completa). Retorna false (out_client_height intocado)
  //     quando o id não é encontrado ou nenhum documento carregado ainda -- mesmo formato de
  //     guard de get_element_box.
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset directly, in RCSS pixels. RmlUi clamps the
  //     value internally to [0, scroll_height - client_height] -- an out-of-range value is
  //     saturated, not rejected. Applied synchronously (not animated): a subsequent
  //     get_element_scroll_top() call reads the new (clamped) value back immediately.
  //     Input hardening: rejects non-finite scroll_top (NaN/inf) with false and no effect --
  //     same fail-high spirit as set_dp_ratio()'s NaN/inf guard (v0.3.0 hardening audit).
  //     Returns false when the id is not found, no document is loaded yet, or scroll_top is
  //     not finite.
  //     Typical use, a custom scrollbar thumb the host draws and drags itself:
  //       float scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       ui.set_element_scroll_top("list", drag_fraction * (scroll_h - client_h));
  // PT: Define o offset de rolagem vertical próprio de um elemento diretamente, em pixels
  //     RCSS. O RmlUi satura o valor internamente em [0, scroll_height - client_height] -- um
  //     valor fora do intervalo é saturado, não rejeitado. Aplicado sincronamente (não
  //     animado): uma chamada subsequente a get_element_scroll_top() lê o novo valor (saturado)
  //     de volta imediatamente.
  //     Hardening de entrada: rejeita scroll_top não-finito (NaN/inf) com false e sem efeito --
  //     mesmo espírito fail-high do guard de NaN/inf de set_dp_ratio() (auditoria de hardening
  //     v0.3.0). Retorna false quando o id não é encontrado, nenhum documento carregado ainda,
  //     ou scroll_top não é finito.
  //     Uso típico, um thumb de scrollbar customizado que o host desenha e arrasta:
  //       float scroll_h = 0.f, client_h = 0.f;
  //       ui.get_element_scroll_height("list", scroll_h);
  //       ui.get_element_client_height("list", client_h);
  //       ui.set_element_scroll_top("list", drag_fraction * (scroll_h - client_h));
  bool set_element_scroll_top(const char* id, float scroll_top) const;

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

  // EN: Create a data model. Call before bind_* and before load(). LIMIT: at most ONE data
  //     model per UiLayer instance -- a second create_data_model() call (any name, including a
  //     different one) returns false and is a silent no-op; the first model stays the only one
  //     bound (DataBinder's `created` flag, checked-before-mutate). Not currently surfaced to
  //     the caller other than the false return -- if multiple independent models are needed,
  //     bind all variables under the ONE model created here. Exercised end-to-end (embed path)
  //     by data_model_embed_sanity.cpp. AUD-TEC-7 (2026-07-08).
  // PT: Cria um data model. Chamar antes de bind_* e antes de load(). LIMITE: no máximo UM
  //     data model por instância de UiLayer -- uma segunda chamada a create_data_model()
  //     (qualquer nome, mesmo diferente) retorna false e é um no-op silencioso; o primeiro
  //     modelo permanece o único ligado (flag `created` do DataBinder, check-before-mutate).
  //     Não é sinalizado ao chamador além do retorno false -- se múltiplos modelos
  //     independentes forem necessários, ligue todas as variáveis sob o ÚNICO modelo criado
  //     aqui. Exercitado ponta-a-ponta (caminho embed) por data_model_embed_sanity.cpp.
  //     AUD-TEC-7 (2026-07-08).
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
