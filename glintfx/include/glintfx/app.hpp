// SPDX-License-Identifier: MPL-2.0
// glintfx — drop-in UI engine + GL3 effects facade.
// Fachada drop-in: motor de UI + efeitos GL3.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <glintfx/config.hpp>
#if !GLINTFX_BACKEND_GLFW
#error "glintfx::App requires GLINTFX_BACKEND_GLFW=ON (embed-only build: use glintfx::UiLayer)"
#endif
#include <memory>
#include <cstddef>
#include <functional>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).
// EN: version() moved to its own header (L1.9-VERSEMBED) so embed-only consumers that
//     never include app.hpp still get glintfx::version(). Re-included here (transitively)
//     so existing code that only includes app.hpp keeps compiling unchanged.
// PT: version() foi movida para header próprio (L1.9-VERSEMBED) para que consumidores
//     embed-only, que nunca incluem app.hpp, ainda tenham glintfx::version(). Reincluída
//     aqui (transitivamente) para que código existente que só inclui app.hpp continue
//     compilando sem mudanças.
#include <glintfx/version.hpp>
#include <glintfx/element_box.hpp>
#include <glintfx/click_info.hpp>
namespace glintfx {

// EN: Configuration for App construction. Zero-initialize safe; defaults are sane.
// PT: Configuração para construção do App. Zero-init seguro; defaults razoáveis.
struct AppConfig {
  const char* title = "glintfx";
  int width  = 1280;
  int height = 720;
  // EN: Initial density-independent pixel ratio. 1 dp = dp_ratio physical pixels.
  //     Matches UiLayerConfig::dp_ratio; see set_dp_ratio() for runtime update.
  // PT: Density-independent pixel ratio inicial. 1 dp = dp_ratio pixels físicos.
  //     Corresponde a UiLayerConfig::dp_ratio; ver set_dp_ratio() para update em runtime.
  float dp_ratio = 1.0f;
};

// EN: RAII application facade. Owns the window, renderer, and UI bootstrap.
//     Move-only. No third-party engine or graphics types appear in this header.
//
//     GLOBAL STATE (N3): glintfx initialises process-global state (window backend + UI library).
//     Only ONE App instance per process is supported. Creating a second instance while
//     the first is alive, or after it has been destroyed, results in undefined behaviour.
//
//     MOVED-FROM STATE (L1.10-APIDOC): after `App b = std::move(a);` (or `b = std::move(a);`),
//     `a` is left in a moved-from state. Calling ANY method on `a` other than ok() or the
//     destructor is undefined behaviour — same convention as std::unique_ptr. ok() on a
//     moved-from App returns false (impl_ is null), which is the only state query that is
//     safe to make; destruction of a moved-from App is always safe (no-op, impl_ is null).
//
// PT: Fachada RAII da aplicação. Possui janela, renderer e bootstrap de UI.
//     Move-only. Nenhum tipo de engine ou gráficos de terceiros aparece neste header.
//
//     ESTADO GLOBAL (N3): glintfx inicializa estado global de processo (backend de janela + lib de UI).
//     Apenas UMA instância de App por processo é suportada. Criar uma segunda instância
//     enquanto a primeira está viva, ou após ser destruída, resulta em comportamento indefinido.
//
//     ESTADO MOVIDO-DE (L1.10-APIDOC): após `App b = std::move(a);` (ou `b = std::move(a);`),
//     `a` fica em estado movido-de. Chamar QUALQUER método em `a` além de ok() ou o destrutor
//     é comportamento indefinido — mesma convenção do std::unique_ptr. ok() num App movido-de
//     retorna false (impl_ é nulo), sendo a única consulta de estado segura; destruir um App
//     movido-de é sempre seguro (no-op, impl_ é nulo).
class App {
public:
  // EN: Constructs the App: opens a window and initialises the GL context and UI engine.
  //     Construction can fail silently (e.g. no display, no GL context available).
  //     On failure (N2): ok() returns false; running() also returns false; load() is a no-op;
  //     render() and run() do nothing. Check ok() once after construction to distinguish an
  //     initialization failure from a window that was closed after a successful start.
  //     run() is safe to call regardless — it returns immediately when !running().
  // PT: Constrói o App: abre a janela e inicializa o contexto GL e o motor de UI.
  //     A construção pode falhar silenciosamente (ex.: sem display, sem contexto GL disponível).
  //     Em falha (N2): ok() retorna false; running() também retorna false; load() é no-op;
  //     render() e run() não fazem nada. Verifique ok() uma vez após a construção para distinguir
  //     falha de inicialização de uma janela fechada após início bem-sucedido.
  //     run() é seguro de chamar independentemente — retorna imediatamente quando !running().
  explicit App(AppConfig cfg = {});
  ~App();
  // EN: Move leaves the source in a moved-from state — see the class-level comment above
  //     for the exact contract (only ok() and ~App() remain valid on the source afterwards).
  // PT: O move deixa a origem em estado movido-de — ver o comentário de nível de classe
  //     acima para o contrato exato (só ok() e ~App() permanecem válidos na origem depois).
  App(App&&) noexcept;
  App& operator=(App&&) noexcept;
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // EN: Returns true if construction succeeded: window, GL context, and UI engine are all live.
  //     Use ok() to distinguish "initialization failed" from "window closed":
  //     running() returns false in both cases, conflating them. Check ok() once right after
  //     construction; it never changes after that point.
  // PT: Retorna true se a construção teve sucesso: janela, contexto GL e motor de UI estão todos vivos.
  //     Use ok() para distinguir "falha de inicialização" de "janela fechada":
  //     running() retorna false nos dois casos, misturando-os. Verifique ok() uma vez logo após
  //     a construção; ele nunca muda depois desse ponto.
  bool ok() const noexcept;

  // EN: Load an .rml document and show it. Returns true on success.
  // PT: Carrega um documento .rml e exibe. Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Update the density-independent pixel ratio at runtime (parity with UiLayer).
  //     Triggers a layout re-pass when changed.
  // PT: Atualiza o density-independent pixel ratio em runtime (paridade com UiLayer).
  //     Dispara re-layout quando alterado.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL for asset resolution (parity with UiLayer).
  //     Relative asset paths are prefixed with base_url/. Call before load().
  // PT: Sobrepõe o base URL para resolução de assets (paridade com UiLayer).
  //     Caminhos relativos de assets são prefixados com base_url/. Chame antes de load().
  void set_asset_base_url(const char* url);

  // EN: Returns false if the window was closed or initialization failed.
  //     To distinguish the two cases check ok() once after construction.
  // PT: Retorna false se a janela foi fechada ou se a inicialização falhou.
  //     Para distinguir os dois casos, verifique ok() uma vez após a construção.
  bool running() const;

  // EN: Process pending window/input events (non-blocking).
  // PT: Processa eventos de janela/entrada pendentes (sem bloqueio).
  void poll_events();

  // EN: Advance the UI context by one tick.
  // PT: Avança o contexto de UI por um tick.
  void update();

  // EN: Render one frame (begin_frame → UI render pass → end_frame → swap).
  // PT: Renderiza um frame (begin_frame → passe de render de UI → end_frame → swap).
  void render();

  // EN: Convenience loop: poll + update + render until !running() (blocking; returns only
  //     after the window closes or ok() turns false). This is exactly the composition of
  //     poll_events() -> update() -> render() already exercised frame-by-frame by every other
  //     App test in this suite (app_smoke, data_model_*, app_resize_smoke, etc.) -- run()
  //     itself has no test of its own because a blocking loop cannot be asserted on without
  //     either an external quit signal or a timeout race, and the 3 calls it composes are
  //     already covered individually. AUD-TEC-7 (2026-07-08).
  // PT: Laço de conveniência: poll + update + render até !running() (bloqueante; só retorna
  //     depois que a janela fecha ou ok() vira false). É exatamente a composição de
  //     poll_events() -> update() -> render() já exercitada frame-a-frame por todo outro
  //     teste de App desta suíte (app_smoke, data_model_*, app_resize_smoke, etc.) -- run()
  //     em si não tem teste próprio porque um laço bloqueante não dá pra verificar sem um
  //     sinal externo de saída ou uma corrida de timeout, e as 3 chamadas que ele compõe já
  //     estão cobertas individualmente. AUD-TEC-7 (2026-07-08).
  void run();

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none reached before the document root).
  //     No ordering constraint versus load(): safe to call before or after it. Parity with
  //     UiLayer::set_click_callback (same signature).
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum for encontrado até a raiz do
  //     documento). Sem restrição de ordem vs. load(): seguro chamar antes ou depois. Paridade
  //     com UiLayer::set_click_callback (mesma assinatura).
  void set_click_callback(std::function<void(const char* element_id)> cb);

  // EN: Register a RICHER click callback -- id + button + coordinates + double-click, a
  //     second/parallel channel alongside set_click_callback (AUD-PUB-4, v0.5.0). Does NOT
  //     replace set_click_callback -- both can be registered simultaneously and both fire
  //     independently on the same click (see ClickInfo's doc-comment for the full contract,
  //     including a double-click firing this callback 3 times total, and the `button` field's
  //     known limitation: effectively always 0, since RmlUi never dispatches Click/Dblclick for
  //     non-primary mouse buttons). Coordinate space: window physical pixels, top-left origin,
  //     y-down -- App owns the whole window, so there is no sub-viewport offset to translate
  //     (unlike UiLayer). Parity with UiLayer::set_click_info_callback (same signature).
  // PT: Registra um callback de clique MAIS RICO -- id + botão + coordenadas + duplo-clique,
  //     um segundo canal/paralelo ao set_click_callback (AUD-PUB-4, v0.5.0). NÃO substitui
  //     set_click_callback -- ambos podem ser registrados simultaneamente e ambos disparam
  //     independentemente no mesmo clique (ver o doc-comment de ClickInfo para o contrato
  //     completo, incluindo um duplo-clique disparando este callback 3 vezes no total, e a
  //     limitação conhecida do campo `button`: na prática sempre 0, já que o RmlUi nunca
  //     despacha Click/Dblclick para botões não-primários do mouse). Espaço de coordenadas:
  //     pixels físicos da janela, origem superior-esquerda, y pra baixo -- o App é dono da
  //     janela inteira, então não há offset de sub-viewport a traduzir (diferente do UiLayer).
  //     Paridade com UiLayer::set_click_info_callback (mesma assinatura).
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

  // EN: Register a scroll callback -- reports the id of the ancestor-or-self nearest to the
  //     element whose OWN scroll offset just changed ("" if none), one call PER SOURCE OF
  //     SCROLLING glintfx does not distinguish: wheel (one call per animated smoothscroll step,
  //     not just at rest), native scrollbar thumb/track/arrow interaction, and the programmatic
  //     scroll_element_into_view()/set_element_scroll_top() calls below. glintfx does NOT play
  //     audio itself -- this is the hook a host uses to, e.g., trigger its own scroll sound.
  //     Deduped against the CURRENT offset by RmlUi itself: setting the same scroll position
  //     twice in a row does not re-fire. No numeric offset is carried by this callback (the
  //     underlying Rml::EventId::Scroll event itself carries none) -- call
  //     get_element_scroll_top(id) from inside the callback if the value is needed. Parity with
  //     UiLayer::set_scroll_callback (same signature). Null/empty callback is a safe no-op. No
  //     ordering constraint versus load().
  //
  //     WARNING (recursion, review v0.6.0 Minor #2): scroll_element_into_view()/
  //     set_element_scroll_top() below are SYNCHRONOUS and dispatch the underlying Scroll event
  //     on the SAME call stack before returning, which re-enters this callback. Calling either of
  //     them from inside the callback with a value that does NOT converge to the element's
  //     current offset (e.g. a badly-written dynamic clamp/snap that always writes something
  //     different from what it just read) recurses without bound -> stack overflow. Safe as long
  //     as the written value CONVERGES (RmlUi's own dedup -- same offset twice does not re-fire --
  //     stops the cycle, typically in 1-2 iterations for a constant target).
  // PT: Registra um callback de rolagem -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento cujo PRÓPRIO offset de rolagem acabou de mudar ("" se nenhum), uma chamada
  //     POR FONTE DE ROLAGEM que a glintfx não distingue: wheel (uma chamada por passo animado
  //     do smoothscroll, não só ao assentar), interação com a scrollbar nativa (thumb/track/
  //     setas), e as chamadas programáticas scroll_element_into_view()/set_element_scroll_top()
  //     abaixo. A glintfx NÃO toca áudio -- este é o gancho que um host usa para, ex., disparar
  //     seu próprio som de rolagem. Deduplicado contra o offset CORRENTE pelo próprio RmlUi:
  //     definir a mesma posição de rolagem duas vezes seguidas não redispara. Nenhum offset
  //     numérico é carregado por este callback (o próprio evento Rml::EventId::Scroll
  //     subjacente não carrega nenhum) -- chame get_element_scroll_top(id) de dentro do callback
  //     se o valor for necessário. Paridade com UiLayer::set_scroll_callback (mesma assinatura).
  //     Callback nulo/vazio é no-op seguro. Sem restrição de ordem vs. load().
  //
  //     AVISO (recursão, review v0.6.0 Minor #2): scroll_element_into_view()/
  //     set_element_scroll_top() abaixo são SÍNCRONOS e despacham o evento Scroll subjacente na
  //     MESMA pilha de chamada antes de retornar, o que reentra neste callback. Chamar qualquer
  //     um deles de dentro do callback com um valor que NÃO converge para o offset atual do
  //     elemento (ex.: um clamp/snap dinâmico mal escrito que sempre escreve algo diferente do
  //     que acabou de ler) recursa sem limite -> stack overflow. Seguro desde que o valor escrito
  //     CONVIRJA (o dedup do próprio RmlUi -- mesmo offset duas vezes não redispara -- para o
  //     ciclo, tipicamente em 1-2 iterações para um alvo constante).
  void set_scroll_callback(std::function<void(const char* element_id)> cb);

  // EN: Query the border-box geometry of an element by id. Coordinate space: window physical
  //     pixels, top-left origin, y-down -- App owns the whole window, so there is no sub-
  //     viewport offset to translate (unlike UiLayer, whose set_viewport(x,y,w,h,target_h)
  //     shifts this space -- see UiLayer::get_element_box). Returns ElementBox{found=false}
  //     (all fields zeroed) when the id is not found in the currently loaded document, or no
  //     document is loaded yet. Parity with UiLayer::get_element_box (same signature).
  // PT: Consulta a geometria border-box de um elemento por id. Espaço de coordenadas: pixels
  //     físicos da janela, origem superior-esquerda, y pra baixo -- o App é dono da janela
  //     inteira, então não há offset de sub-viewport a traduzir (diferente do UiLayer, cujo
  //     set_viewport(x,y,w,h,target_h) desloca esse espaço -- ver UiLayer::get_element_box).
  //     Retorna ElementBox{found=false} (todos os campos zerados) quando o id não é encontrado
  //     no documento carregado atualmente, ou nenhum documento carregado ainda. Paridade com
  //     UiLayer::get_element_box (mesma assinatura).
  ElementBox get_element_box(const char* id) const;

  // EN: Scroll an element's nearest scrolling ancestor(s) so the element becomes visible
  //     (GLINTFX-SCROLL-1, v0.4.0). Parity with UiLayer::scroll_element_into_view (same
  //     signature/semantics -- see there for the full contract: instant/synchronous, wraps
  //     Rml::Element::ScrollIntoView(bool)).
  // PT: Rola o(s) ancestral(is) rolável(eis) mais próximo(s) de um elemento para que ele fique
  //     visível (GLINTFX-SCROLL-1, v0.4.0). Paridade com UiLayer::scroll_element_into_view
  //     (mesma assinatura/semântica -- ver lá o contrato completo: instantâneo/síncrono,
  //     encapsula Rml::Element::ScrollIntoView(bool)).
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset, in RCSS pixels. Parity with
  //     UiLayer::get_element_scroll_top (same signature/semantics).
  // PT: Consulta o offset de rolagem vertical próprio de um elemento, em pixels RCSS. Paridade
  //     com UiLayer::get_element_scroll_top (mesma assinatura/semântica).
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query an element's total scrollable content height, in RCSS pixels. Parity with
  //     UiLayer::get_element_scroll_height (same signature/semantics; see there for the full
  //     scroll-metrics-trio usage formula shared with get_element_scroll_top/
  //     get_element_client_height).
  // PT: Consulta a altura total do conteúdo rolável de um elemento, em pixels RCSS. Paridade
  //     com UiLayer::get_element_scroll_height (mesma assinatura/semântica; ver lá a fórmula
  //     completa de uso do trio de métricas de rolagem, compartilhada com
  //     get_element_scroll_top/get_element_client_height).
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query an element's client (visible) height, in RCSS pixels. Parity with
  //     UiLayer::get_element_client_height (same signature/semantics).
  // PT: Consulta a altura de cliente (visível) de um elemento, em pixels RCSS. Paridade com
  //     UiLayer::get_element_client_height (mesma assinatura/semântica).
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset directly, in RCSS pixels. Parity with
  //     UiLayer::set_element_scroll_top (same signature/semantics, including the non-finite
  //     input rejection).
  // PT: Define o offset de rolagem vertical próprio de um elemento diretamente, em pixels
  //     RCSS. Paridade com UiLayer::set_element_scroll_top (mesma assinatura/semântica,
  //     incluindo a rejeição de entrada não-finita).
  bool set_element_scroll_top(const char* id, float scroll_top) const;

  // EN: Give input focus to an element, programmatically. Parity with
  //     UiLayer::set_focus (same signature/semantics, including the `focus`-property-not-
  //     `tab-index` gating contract -- see UiLayer::set_focus's doc-comment for the full
  //     writeup confirmed against the pinned RmlUi 6.3 source). (L1.17-FOCUS)
  // PT: Dá foco de entrada a um elemento, programaticamente. Paridade com
  //     UiLayer::set_focus (mesma assinatura/semântica, incluindo o contrato de controle
  //     pela propriedade `focus` -- não `tab-index` -- ver o doc-comment de UiLayer::set_focus
  //     para o relato completo confirmado contra o source pinado do RmlUi 6.3). (L1.17-FOCUS)
  bool set_focus(const char* id) const;

  // EN: Remove input focus. Parity with UiLayer::clear_focus (same signature/semantics,
  //     including the idempotent-true-when-nothing-focused contract). (L1.17-FOCUS)
  // PT: Remove o foco de entrada. Paridade com UiLayer::clear_focus (mesma
  //     assinatura/semântica, incluindo o contrato de true-idempotente-quando-nada-focado).
  //     (L1.17-FOCUS)
  bool clear_focus() const;

  // -------------------------------------------------------------------------
  // EN: DOM read/write by id (L1.16-DOMRW, consumes AUD-PUB-6(d) -- "imperative setters by
  //     id"). Parity with UiLayer (same signatures/semantics). See UiLayer's doc-comments for
  //     the full contract, including the RmlUi-signature-level detail (SetClass/SetProperty/
  //     SetInnerRML) confirmed against the pinned RmlUi 6.3 source.
  // PT: Leitura/escrita de DOM por id (L1.16-DOMRW, consome AUD-PUB-6(d) -- "setters
  //     imperativos por id"). Paridade com UiLayer (mesmas assinaturas/semântica). Ver os
  //     doc-comments do UiLayer para o contrato completo, incluindo o detalhe no nível de
  //     assinatura RmlUi (SetClass/SetProperty/SetInnerRML) confirmado contra o source pinado
  //     do RmlUi 6.3.
  // -------------------------------------------------------------------------

  // EN: Replace an element's text content. `text` is LITERAL TEXT, never markup -- `& < > "`
  //     are escaped before reaching RmlUi's SetInnerRML, so this call can NEVER be used to
  //     inject RML/tags into the document, even with untrusted `text` (chat, a player-chosen
  //     display name, etc.). Null `text` is normalised to "" (never rejected). Returns false
  //     when no document is loaded or `id` is null/empty ("")/unknown -- same AUD-TEC-5
  //     fail-high convention as get_element_box/set_focus.
  // PT: Substitui o conteúdo de texto de um elemento. `text` é TEXTO LITERAL, nunca markup --
  //     `& < > "` são escapados antes de chegar no SetInnerRML do RmlUi, então esta chamada
  //     NUNCA pode ser usada para injetar RML/tags no documento, mesmo com `text` não confiável
  //     (chat, nome de exibição escolhido pelo jogador, etc.). `text` nulo é normalizado para ""
  //     (nunca rejeitado). Retorna false quando nenhum documento estiver carregado ou `id` for
  //     nulo/vazio ("")/desconhecido -- mesma convenção fail-high AUD-TEC-5 de
  //     get_element_box/set_focus.
  bool set_text(const char* id, const char* text) const;

  // EN: Add a CSS class on an element -- wraps Rml::Element::SetClass(cls, true), which returns
  //     void (no RmlUi-level parse outcome, unlike set_property below). Returns false when no
  //     document is loaded, or `id`/`cls` is null/empty ("") -- AUD-TEC-5 fail-high, applied to
  //     BOTH parameters here (an empty class name is a structurally-invalid caller input, same
  //     as an empty id).
  // PT: Adiciona uma classe CSS num elemento -- encapsula Rml::Element::SetClass(cls, true),
  //     que retorna void (nenhum resultado de parse em nível RmlUi, diferente do set_property
  //     abaixo). Retorna false quando nenhum documento estiver carregado, ou `id`/`cls` for
  //     nulo/vazio ("") -- fail-high AUD-TEC-5, aplicado aos DOIS parâmetros aqui (um nome de
  //     classe vazio é entrada estruturalmente inválida do chamador, igual a um id vazio).
  bool add_class(const char* id, const char* cls) const;

  // EN: Remove a CSS class from an element -- wraps Rml::Element::SetClass(cls, false). Same
  //     `id`/`cls` null/empty guard as add_class. Removing a class the element never had is a
  //     safe no-op inside RmlUi itself, so this still returns true in that case.
  // PT: Remove uma classe CSS de um elemento -- encapsula Rml::Element::SetClass(cls, false).
  //     Mesmo guard de `id`/`cls` nulo/vazio do add_class. Remover uma classe que o elemento
  //     nunca teve é um no-op seguro dentro do próprio RmlUi, então ainda retorna true nesse
  //     caso.
  bool remove_class(const char* id, const char* cls) const;

  // EN: Set a single inline RCSS property by name -- wraps Rml::Element::SetProperty(name,
  //     value), which DOES return a real bool (RmlUi's own parser rejects an unparseable
  //     `value` for the named property and this call propagates that rejection unchanged --
  //     no separate glintfx-level validation on top). `id`/`prop` null or empty ("") is
  //     rejected (false) before touching RmlUi -- AUD-TEC-5 fail-high. `value` null is
  //     normalised to "" and forwarded to RmlUi's own parser (which is the sole authority on
  //     whether an empty value is acceptable for the named property).
  // PT: Define uma única propriedade RCSS inline por nome -- encapsula
  //     Rml::Element::SetProperty(name, value), que DE FATO retorna um bool real (o próprio
  //     parser do RmlUi rejeita um `value` que não parseia para a propriedade nomeada e esta
  //     chamada propaga essa rejeição sem mudança -- nenhuma validação separada em nível
  //     glintfx por cima). `id`/`prop` nulo ou vazio ("") é rejeitado (false) antes de tocar o
  //     RmlUi -- fail-high AUD-TEC-5. `value` nulo é normalizado para "" e encaminhado ao
  //     próprio parser do RmlUi (que é a única autoridade sobre se um valor vazio é aceitável
  //     para a propriedade nomeada).
  bool set_property(const char* id, const char* prop, const char* value) const;

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1) — parity with UiLayer. Call order: create_data_model
  //     -> bind_* -> load() -> set_*(). Engine enforces the ordering constraint
  //     (bind_* after load() returns false). No engine-specific types in this header.
  // PT: API de data-model (T1) — paridade com UiLayer. Ordem de chamada:
  //     create_data_model -> bind_* -> load() -> set_*(). Engine enforça a
  //     restrição de ordem (bind_* após load() retorna false). API usa tipos C simples.
  // -------------------------------------------------------------------------

  // EN: Create a data model. Call before bind_* and before load(). LIMIT: at most ONE data
  //     model per App instance -- a second create_data_model() call (any name, including a
  //     different one) returns false and is a silent no-op; the first model stays the only one
  //     bound (DataBinder's `created` flag, checked-before-mutate). Not currently surfaced to
  //     the caller other than the false return -- if multiple independent models are needed,
  //     bind all variables under the ONE model created here. AUD-TEC-7 (2026-07-08).
  // PT: Cria um data model. Chamar antes de bind_* e antes de load(). LIMITE: no máximo UM
  //     data model por instância de App -- uma segunda chamada a create_data_model() (qualquer
  //     nome, mesmo diferente) retorna false e é um no-op silencioso; o primeiro modelo
  //     permanece o único ligado (flag `created` do DataBinder, check-before-mutate). Não é
  //     sinalizado ao chamador além do retorno false -- se múltiplos modelos independentes
  //     forem necessários, ligue todas as variáveis sob o ÚNICO modelo criado aqui.
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

  // EN: Read-back the CURRENT value of a bound cell (L1.16-DOMRW, consumes AUD-PUB-6(e) -- the
  //     data-model was write-only before this: a host could push values in via set_*, but had
  //     no way to read back a value the UI itself, or the user through a bound control (e.g. an
  //     `<input data-value>`), wrote into the model. Reflects any such RmlUi-driven
  //     bidirectional write, not just the host's own last set_* call -- see
  //     glintfx/src/data_binder.hpp's get_number doc-comment for the pointer-identity reasoning
  //     confirmed against the pinned RmlUi source. Returns false (out left untouched) when the
  //     key was never bound; no ordering constraint versus load() beyond that (a bound key is
  //     readable before or after load()). Parity with UiLayer::get_number/get_string/get_bool
  //     (same signatures).
  // PT: Lê de volta o valor CORRENTE de uma célula ligada (L1.16-DOMRW, consome AUD-PUB-6(e) --
  //     o data-model era write-only antes disto: um host podia empurrar valores via set_*, mas
  //     não tinha como ler de volta um valor que a própria UI, ou o usuário através de um
  //     controle ligado (ex.: um `<input data-value>`), escreveu no modelo. Reflete qualquer
  //     escrita bidirecional dirigida pelo RmlUi assim, não só a última chamada set_* do
  //     próprio host -- ver o doc-comment de get_number em glintfx/src/data_binder.hpp para o
  //     raciocínio de identidade-de-ponteiro confirmado contra o source pinado do RmlUi. Retorna
  //     false (out intocado) quando a chave nunca foi ligada; sem restrição de ordem vs. load()
  //     além disso (uma chave ligada é legível antes ou depois de load()). Paridade com
  //     UiLayer::get_number/get_string/get_bool (mesmas assinaturas).
  bool get_number(const char* key, double& out) const;
  bool get_string(const char* key, std::string& out) const;
  bool get_bool  (const char* key, bool& out) const;

  // EN: Render one frame and save the result to a PPM file before swapping buffers.
  //     This captures the composited image from the window framebuffer (FBO 0) immediately
  //     after EndFrame composites the render layer, ensuring correct pixel data even on
  //     platforms where the back buffer content is undefined after glfwSwapBuffers.
  //     Returns true on success.
  // PT: Renderiza um frame e salva o resultado em arquivo PPM antes de trocar buffers.
  //     Captura a imagem composta do framebuffer da janela (FBO 0) imediatamente após
  //     o EndFrame compositar o render layer, garantindo dados corretos mesmo em plataformas
  //     onde o back buffer fica indefinido após glfwSwapBuffers.
  //     Retorna true em caso de sucesso.
  bool snapshot(const char* ppm_path);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
