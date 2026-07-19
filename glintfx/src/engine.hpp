// SPDX-License-Identifier: MPL-2.0
// EN: Engine — reusable RmlUi+GL3 core decoupled from any window/input backend.
//     Owns a RenderGl3 + Bootstrap pair. Two frame paths:
//       render_standalone — clear + UI + alpha-fix on FBO0 (used by App).
//       render_compose    — compose-only, no clear/no swap, with GL state guard (T3, UiLayer).
//     Callers supply a Rml::SystemInterface* (owned by the caller, not by Engine).
// PT: Engine — núcleo RmlUi+GL3 reusável desacoplado de qualquer backend de janela/input.
//     Possui um par RenderGl3 + Bootstrap. Dois caminhos de frame:
//       render_standalone — clear + UI + alpha-fix no FBO0 (usado pelo App).
//       render_compose    — compose-only, sem clear/sem swap, com guard de estado GL (T3, UiLayer).
//     Chamadores fornecem um Rml::SystemInterface* (dono do chamador, não do Engine).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Forward-declare only — no RmlUi/GL types visible to callers.
// PT: Só forward-declaration — nenhum tipo RmlUi/GL visível aos chamadores.
namespace Rml { class Context; class SystemInterface; }
#include <cstddef>
#include <functional>
#include <string>  // EN: std::string out-param (get_string, L1.16-DOMRW). PT: out-param std::string (get_string, L1.16-DOMRW).
#include <glintfx/click_info.hpp>
#include <glintfx/font_engine.hpp>
#include <glintfx/ui_event.hpp>  // EN: process_event (A1, framework-2D). PT: process_event (A1, framework-2D).

namespace glintfx {

class Engine {
public:
  Engine();
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  // EN: Initialise the RmlUi+GL3 subsystem against the CURRENT GL context.
  //     `system` lifetime is owned by the caller; must stay alive until ~Engine().
  //     `font_engine` (L1.20-FONTFLIP Phase 2, default FontEngine::Own) is forwarded verbatim
  //     to Bootstrap::init(), which reads it exactly once, before Rml::Initialise() -- see
  //     that call's doc-comment (src/bootstrap.hpp) and glintfx/include/glintfx/font_engine.hpp
  //     for the full selection/fallback contract. The default parameter exists so pre-existing
  //     internal callers that never pass one (test harnesses under glintfx/tests/) keep
  //     compiling unchanged, behaving exactly as they did before this parameter was added.
  //     Returns true on success. Safe to call only once per instance.
  // PT: Inicializa o subsistema RmlUi+GL3 contra o contexto GL CORRENTE.
  //     O lifetime de `system` pertence ao chamador; deve sobreviver até ~Engine().
  //     `font_engine` (L1.20-FONTFLIP Fase 2, default FontEngine::Own) é encaminhado tal-e-qual
  //     ao Bootstrap::init(), que o lê exatamente uma vez, antes do Rml::Initialise() -- ver o
  //     doc-comment dessa chamada (src/bootstrap.hpp) e glintfx/include/glintfx/font_engine.hpp
  //     para o contrato completo de seleção/fallback. O parâmetro default existe para que
  //     chamadores internos pré-existentes que nunca passam um (harnesses de teste sob
  //     glintfx/tests/) continuem compilando sem mudança, se comportando exatamente como antes
  //     deste parâmetro existir.
  //     Retorna true em caso de sucesso. Seguro chamar apenas uma vez por instância.
  bool attach(Rml::SystemInterface* system, int w, int h,
              FontEngine font_engine = FontEngine::Own);

  // EN: True after a successful attach().
  // PT: True após um attach() com sucesso.
  bool ok() const noexcept;

  // EN: Load a document from rml_path and call Show(). Requires ok().
  // PT: Carrega documento de rml_path e chama Show(). Requer ok().
  bool load(const char* rml_path);

  // EN: Notify RmlUi of a viewport resize (logical dimensions).
  // PT: Notifica o RmlUi de um redimensionamento de viewport (dimensões lógicas).
  void set_viewport(int w, int h);

  // EN: Set the density-independent pixel ratio on the active context.
  //     1 dp = ratio physical pixels. Call after a successful attach().
  //     No-op when not ok(). Triggers a full layout re-pass if ratio changes.
  // PT: Define o density-independent pixel ratio no contexto ativo.
  //     1 dp = ratio pixels físicos. Chamar após attach() com sucesso.
  //     No-op quando não ok(). Dispara re-layout completo se ratio mudar.
  void set_dp_ratio(float ratio);

  // EN: Override the base URL used to resolve relative asset paths (fonts, images, RCSS).
  //     Delegates to Bootstrap's FileInterface; safe to call at any time after attach().
  //     Pass nullptr or "" to clear. No-op when not ok().
  // PT: Sobrepõe o base URL usado para resolver caminhos relativos de assets (fontes, imagens, RCSS).
  //     Delega ao FileInterface do Bootstrap; seguro chamar a qualquer momento após attach().
  //     Passe nullptr ou "" para limpar. No-op quando não ok().
  void set_asset_base_url(const char* url);

  // EN: Advance the RmlUi context by one step (call once per frame before render_*).
  // PT: Avança o contexto RmlUi um passo (chamar uma vez por frame antes de render_*).
  void update();

  // EN: Standalone frame path — clear + render UI + alpha-fix on FBO0.
  //     Used by App (owns the window). Does NOT swap buffers.
  // PT: Caminho de frame standalone — clear + render UI + alpha-fix no FBO0.
  //     Usado pelo App (dono da janela). NÃO faz swap de buffers.
  void render_standalone(int w, int h);

  // EN: Compose-only frame path. (offset_x, offset_y) are OpenGL-native (bottom-left-origin)
  //     viewport offset -- ALREADY CONVERTED by the caller (UiLayer). Engine stays offset-space
  //     agnostic: it never interprets these values, only forwards them.
  // PT: Caminho compose-only. (offset_x, offset_y) são offset de viewport NATIVO do OpenGL
  //     (origem inferior-esquerda) -- JÁ CONVERTIDO pelo chamador (UiLayer). O Engine
  //     permanece agnóstico ao espaço do offset: nunca interpreta esses valores, só repassa.
  void render_compose(int offset_x, int offset_y, int w, int h);

  // EN: Returns the active Rml::Context, or nullptr if not ok().
  // PT: Retorna o Rml::Context ativo, ou nullptr se não ok().
  Rml::Context* context();

  // EN: Translate a neutral UiEvent into the corresponding Rml::Context::Process* call (A1,
  //     framework-2D). This is the SHARED route UiLayer::process_event has used since embed
  //     mode shipped -- moved up from ui_layer.cpp so App can reuse it verbatim instead of
  //     duplicating the translation (docs/superpowers/plans/2026-07-19-framework2d-A1-input.md
  //     section 2.1). Handles None/MouseMove/MouseButton/Key/Text/MouseWheel; does NOT handle
  //     Resize -- that case mutates facade-owned state (UiLayer's letterbox offset, App's
  //     window-size cache) and stays with each caller (UiLayer::process_event keeps its
  //     Resize branch; App::process_event treats it as a documented no-op, since App is
  //     already the window's own size authority).
  //     `offset_x`/`offset_y` are subtracted from MouseMove's position before forwarding
  //     (window-space -> content-space, same F3 sub-viewport translation UiLayer::process_event
  //     already did): UiLayer passes its letterbox origin (impl_->x, impl_->y); App passes
  //     (0, 0) -- it owns the whole window, same reasoning already used by
  //     App::get_element_box. MouseWheel ignores the offset (it carries a DELTA, not a
  //     position -- see UiEvent::Type::MouseWheel's doc-comment). No-op when not ok().
  // PT: Traduz um UiEvent neutro na chamada Rml::Context::Process* correspondente (A1,
  //     framework-2D). É a rota COMPARTILHADA que UiLayer::process_event usa desde que o embed
  //     mode foi lançado -- movida para cá de ui_layer.cpp para que o App a reuse tal-e-qual em
  //     vez de duplicar a tradução (docs/superpowers/plans/2026-07-19-framework2d-A1-input.md
  //     seção 2.1). Trata None/MouseMove/MouseButton/Key/Text/MouseWheel; NÃO trata Resize --
  //     esse caso muta estado de posse da fachada (offset de letterbox do UiLayer, cache de
  //     tamanho de janela do App) e permanece em cada chamador (UiLayer::process_event mantém
  //     o próprio branch de Resize; App::process_event o trata como no-op documentado, já que o
  //     App já é a própria autoridade sobre o tamanho da janela).
  //     `offset_x`/`offset_y` são subtraídos da posição do MouseMove antes de repassar (espaço-
  //     janela -> espaço-conteúdo, mesma tradução de sub-viewport F3 que UiLayer::process_event
  //     já fazia): o UiLayer passa a própria origem de letterbox (impl_->x, impl_->y); o App
  //     passa (0, 0) -- é dono da janela inteira, mesma racional já usada em
  //     App::get_element_box. O MouseWheel ignora o offset (carrega um DELTA, não uma posição
  //     -- ver o doc-comment de UiEvent::Type::MouseWheel). No-op quando não ok().
  void process_event(const UiEvent& ev, int offset_x = 0, int offset_y = 0);

  // EN: Register a click callback -- forwards to Bootstrap::set_click_callback (F1, v0.2.5).
  //     See Bootstrap::set_click_callback for the full ordering/lifetime contract.
  // PT: Registra um callback de clique -- encaminha a Bootstrap::set_click_callback (F1,
  //     v0.2.5). Ver Bootstrap::set_click_callback para o contrato completo de ordem/lifetime.
  void set_click_callback(std::function<void(const char*)> cb);

  // EN: Register a richer click callback -- forwards to Bootstrap::set_click_info_callback
  //     (AUD-PUB-4, v0.5.0). Straight passthrough, no coordinate translation here -- x/y stay
  //     in Bootstrap's content-local space; UiLayer/App translate to window-space at the public
  //     boundary (see Bootstrap::set_click_info_callback / ClickInfo doc-comment).
  // PT: Registra um callback de clique mais rico -- encaminha a
  //     Bootstrap::set_click_info_callback (AUD-PUB-4, v0.5.0). Repasse direto, sem tradução de
  //     coordenada aqui -- x/y permanecem no espaço local de conteúdo do Bootstrap; UiLayer/App
  //     traduzem para espaço-janela na fronteira pública (ver Bootstrap::set_click_info_callback
  //     / doc-comment de ClickInfo).
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

  // EN: Register a scroll callback -- forwards to Bootstrap::set_scroll_callback
  //     (GLINTFX-SCROLL-1 follow-up, v0.6.0). Straight passthrough, id-only (no coordinate
  //     translation applies -- there is none for scroll, unlike ClickInfo's x/y). See
  //     Bootstrap::set_scroll_callback for the full contract (fires on wheel, native scrollbar
  //     drag, and the programmatic scroll methods; empty/null callback is a safe no-op).
  //     WARNING (recursion, review v0.6.0 Minor #2): calling set_element_scroll_top/
  //     scroll_element_into_view from inside this callback with a non-convergent value (never
  //     settles to the element's current offset) recurses without bound -> stack overflow, since
  //     both dispatch the Scroll event synchronously on the same call stack. See
  //     Bootstrap::set_scroll_callback's doc-comment for the full explanation and the safe
  //     (convergent-value) pattern.
  // PT: Registra um callback de rolagem -- encaminha a Bootstrap::set_scroll_callback
  //     (desdobramento do GLINTFX-SCROLL-1, v0.6.0). Repasse direto, só-id (não há tradução de
  //     coordenada aplicável -- não existe uma pro scroll, diferente do x/y do ClickInfo). Ver
  //     Bootstrap::set_scroll_callback para o contrato completo (dispara em wheel, arraste de
  //     scrollbar nativa, e os métodos programáticos de rolagem; callback nulo/vazio é no-op
  //     seguro).
  //     AVISO (recursão, review v0.6.0 Minor #2): chamar set_element_scroll_top/
  //     scroll_element_into_view de dentro deste callback com um valor não-convergente (nunca
  //     assenta no offset atual do elemento) recursa sem limite -> stack overflow, já que ambos
  //     despacham o evento Scroll sincronamente na mesma pilha de chamada. Ver o doc-comment de
  //     Bootstrap::set_scroll_callback para a explicação completa e o padrão seguro (valor
  //     convergente).
  void set_scroll_callback(std::function<void(const char*)> cb);

  // EN: Form/DOM event callbacks -- forward to Bootstrap::set_change_callback/
  //     set_submit_callback/set_focus_callback/set_blur_callback/set_hover_callback
  //     (L1.15-FORMEV). Straight passthrough, no translation at this layer -- see there for the
  //     full per-event contract (id resolution, lifetime, capture-vs-bubble wiring, the
  //     empirically-verified focus/blur recursion analysis, the hover dedup machine).
  // PT: Callbacks de evento de formulário/DOM -- encaminham a Bootstrap::set_change_callback/
  //     set_submit_callback/set_focus_callback/set_blur_callback/set_hover_callback
  //     (L1.15-FORMEV). Repasse direto, sem tradução nesta camada -- ver lá o contrato completo
  //     por evento (resolução de id, lifetime, fiação captura-vs-bubble, a análise de recursão
  //     de focus/blur verificada empiricamente, a máquina de dedup do hover).
  void set_change_callback(std::function<void(const char* id, const char* value)> cb);
  void set_submit_callback(std::function<void(const char* id)> cb);
  void set_focus_callback(std::function<void(const char* id)> cb);
  void set_blur_callback(std::function<void(const char* id)> cb);
  void set_hover_callback(std::function<void(const char* id, bool entered)> cb);

  // EN: Query the border-box geometry of an element by id -- forwards to
  //     Bootstrap::get_element_box (F2, v0.2.5). Content-local space (offset-free); UiLayer/
  //     App translate to window-space at the public boundary. Returns false when not ok(),
  //     no document is loaded, or the id is not found; x/y/w/h are left untouched in that case.
  // PT: Consulta a geometria border-box de um elemento por id -- encaminha a
  //     Bootstrap::get_element_box (F2, v0.2.5). Espaço local de conteúdo (offset-free);
  //     UiLayer/App traduzem para espaço-janela na fronteira pública. Retorna false quando
  //     não ok(), nenhum documento estiver carregado, ou o id não for encontrado; x/y/w/h
  //     ficam intocados nesse caso.
  bool get_element_box(const char* id, float& x, float& y, float& w, float& h) const;

  // EN: Scroll an element's scrolling ancestor(s) into view -- forwards to
  //     Bootstrap::scroll_element_into_view (GLINTFX-SCROLL-1, v0.4.0). Instant, synchronous.
  //     Returns false when not ok(), no document is loaded, or the id is not found.
  // PT: Rola o(s) ancestral(is) rolável(eis) de um elemento até ele ficar visível -- encaminha a
  //     Bootstrap::scroll_element_into_view (GLINTFX-SCROLL-1, v0.4.0). Instantâneo, síncrono.
  //     Retorna false quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query an element's own vertical scroll offset -- forwards to
  //     Bootstrap::get_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). Returns false (out param
  //     untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta o offset de rolagem vertical próprio de um elemento -- encaminha a
  //     Bootstrap::get_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out param
  //     intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query an element's total scrollable content height -- forwards to
  //     Bootstrap::get_element_scroll_height (GLINTFX-SCROLL-1, v0.4.0). Returns false (out
  //     param untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta a altura total do conteúdo rolável de um elemento -- encaminha a
  //     Bootstrap::get_element_scroll_height (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out
  //     param intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query an element's client (visible) height -- forwards to
  //     Bootstrap::get_element_client_height (GLINTFX-SCROLL-1, v0.4.0). Returns false (out
  //     param untouched) when not ok(), no document is loaded, or the id is not found.
  // PT: Consulta a altura de cliente (visível) de um elemento -- encaminha a
  //     Bootstrap::get_element_client_height (GLINTFX-SCROLL-1, v0.4.0). Retorna false (out
  //     param intocado) quando não ok(), nenhum documento estiver carregado, ou o id não for
  //     encontrado.
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set an element's own vertical scroll offset -- forwards to
  //     Bootstrap::set_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). RmlUi clamps internally;
  //     this layer rejects non-finite input. Returns false when not ok(), no document is
  //     loaded, the id is not found, or scroll_top is not finite.
  // PT: Define o offset de rolagem vertical próprio de um elemento -- encaminha a
  //     Bootstrap::set_element_scroll_top (GLINTFX-SCROLL-1, v0.4.0). O RmlUi satura
  //     internamente; esta camada rejeita entrada não-finita. Retorna false quando não ok(),
  //     nenhum documento estiver carregado, o id não for encontrado, ou scroll_top não for
  //     finito.
  bool set_element_scroll_top(const char* id, float scroll_top) const;

  // EN: Give input focus to an element -- forwards to Bootstrap::set_focus (L1.17-FOCUS).
  //     Returns false when not ok(), no document is loaded, the id is not found, or the
  //     resolved element's RCSS `focus` property is `none`. See Bootstrap::set_focus's
  //     doc-comment for the full contract, including why this is gated by the `focus`
  //     property (default `auto`) and NOT by `tab-index`/HTML-style tabindex.
  // PT: Dá foco de entrada a um elemento -- encaminha a Bootstrap::set_focus (L1.17-FOCUS).
  //     Retorna false quando não ok(), nenhum documento estiver carregado, o id não for
  //     encontrado, ou a propriedade RCSS `focus` do elemento resolvido for `none`. Ver o
  //     doc-comment de Bootstrap::set_focus para o contrato completo, incluindo por que
  //     isto é controlado pela propriedade `focus` (default `auto`) e NÃO por `tab-index`/
  //     tabindex estilo HTML.
  bool set_focus(const char* id) const;

  // EN: Remove input focus -- forwards to Bootstrap::clear_focus (L1.17-FOCUS). Returns
  //     false when not ok() or no document is loaded; returns true (idempotent) when a
  //     document is loaded but nothing is currently focused.
  // PT: Remove o foco de entrada -- encaminha a Bootstrap::clear_focus (L1.17-FOCUS).
  //     Retorna false quando não ok() ou nenhum documento estiver carregado; retorna true
  //     (idempotente) quando um documento está carregado mas nada está focado no momento.
  bool clear_focus() const;

  // -------------------------------------------------------------------------
  // EN: DOM read/write by id (L1.16-DOMRW, consumes AUD-PUB-6(d) -- imperative setters).
  //     Mirrors the guard shape of the 6 pre-existing per-id methods above (get_element_box,
  //     scroll_element_into_view, etc.): false when not ok(), no document is loaded, or id is
  //     null/empty ("") -- AUD-TEC-5 fail-high. Forwards to Bootstrap; see there for the
  //     RmlUi-signature-level contract (Element::SetClass/SetProperty/SetInnerRML) and the
  //     set_text markup-escaping hardening.
  // PT: Leitura/escrita de DOM por id (L1.16-DOMRW, consome AUD-PUB-6(d) -- setters
  //     imperativos). Espelha o formato de guard dos 6 métodos por-id pré-existentes acima
  //     (get_element_box, scroll_element_into_view, etc.): false quando não ok(), nenhum
  //     documento estiver carregado, ou id for nulo/vazio ("") -- fail-high AUD-TEC-5. Encaminha
  //     ao Bootstrap; ver lá o contrato no nível de assinatura RmlUi (Element::SetClass/
  //     SetProperty/SetInnerRML) e o hardening de escape de markup do set_text.
  // -------------------------------------------------------------------------

  // EN: Replace an element's text content. `text` is treated as LITERAL TEXT, never markup --
  //     `& < > "` are escaped (Rml::StringUtilities::EncodeRml) before reaching
  //     Element::SetInnerRML, so a host CANNOT inject RML/tags through this call (the string
  //     "<script>"/"<img onerror=...>" ends up as the literal visible text "<script>", not a
  //     parsed element). A null `text` is treated as "" (same normalisation as
  //     DataBinder::set_string), never rejected.
  // PT: Substitui o conteúdo de texto de um elemento. `text` é tratado como TEXTO LITERAL,
  //     nunca markup -- `& < > "` são escapados (Rml::StringUtilities::EncodeRml) antes de
  //     chegar em Element::SetInnerRML, então um host NÃO CONSEGUE injetar RML/tags por esta
  //     chamada (a string "<script>"/"<img onerror=...>" vira o texto visível literal
  //     "<script>", não um elemento parseado). `text` nulo é tratado como "" (mesma
  //     normalização de DataBinder::set_string), nunca rejeitado.
  bool set_text(const char* id, const char* text) const;

  // EN: Add/remove a CSS class on an element -- forwards to Rml::Element::SetClass(cls, true/
  //     false), which returns void (unlike set_property below), so success here means only
  //     "the guarded id/cls were valid and the element was found", not a parse outcome (there
  //     is none to parse). `cls` null/empty is rejected (false), same AUD-TEC-5 discipline as
  //     `id`.
  // PT: Adiciona/remove uma classe CSS de um elemento -- encaminha a
  //     Rml::Element::SetClass(cls, true/false), que retorna void (diferente de set_property
  //     abaixo), então sucesso aqui significa só "o id/cls guardados eram válidos e o elemento
  //     foi encontrado", não um resultado de parse (não há um para parsear). `cls` nulo/vazio é
  //     rejeitado (false), mesma disciplina AUD-TEC-5 de `id`.
  bool add_class(const char* id, const char* cls) const;
  bool remove_class(const char* id, const char* cls) const;

  // EN: Set an inline RCSS property by name -- forwards to Rml::Element::SetProperty(name,
  //     value), which DOES return bool (a real parse outcome, unlike SetClass/SetInnerRML above)
  //     -- an unparseable `value` for the named property (e.g. `set_property(id, "color",
  //     "not-a-color")`) propagates RmlUi's own `false`, not a glintfx-invented one. `prop`
  //     null/empty is rejected (false) before ever reaching RmlUi, same AUD-TEC-5 discipline as
  //     `id`.
  // PT: Define uma propriedade RCSS inline por nome -- encaminha a
  //     Rml::Element::SetProperty(name, value), que DE FATO retorna bool (um resultado real de
  //     parse, diferente de SetClass/SetInnerRML acima) -- um `value` que não parseia para a
  //     propriedade nomeada (ex.: `set_property(id, "color", "not-a-color")`) propaga o próprio
  //     `false` do RmlUi, não um inventado pela glintfx. `prop` nulo/vazio é rejeitado (false)
  //     antes de sequer chegar no RmlUi, mesma disciplina AUD-TEC-5 de `id`.
  bool set_property(const char* id, const char* prop, const char* value) const;

  // -------------------------------------------------------------------------
  // EN: Data-model API (T1). Call order: create_data_model -> bind_* -> load -> set_*.
  //     Binds are registered against the RmlUi context BEFORE LoadDocument so that
  //     data-views are compiled with the correct variable map.
  //     Guards: create_data_model / bind_* return false after load() is called (too late).
  //             set_* are no-ops when the key was never bound (safe any time after load).
  // PT: API de data-model (T1). Ordem: create_data_model -> bind_* -> load -> set_*.
  //     Binds são registrados no contexto RmlUi ANTES do LoadDocument para que
  //     as data-views sejam compiladas com o mapa de variáveis correto.
  //     Guards: create_data_model / bind_* retornam false após load() (tarde demais).
  //             set_* são no-op quando a chave nunca foi ligada (seguro após load).
  // -------------------------------------------------------------------------

  // EN: Create a data model; must be called before bind_* and before load().
  // PT: Cria um data model; deve ser chamado antes de bind_* e antes de load().
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
  // EN: Bind a string-list cell (for data-for iteration).
  // PT: Liga uma célula de lista de strings (para iteração data-for).
  bool bind_list  (const char* key);

  // EN: Mutate a bound cell and dirty the variable (reflected on next update()).
  // PT: Muta uma célula ligada e marca a variável suja (refletido no próximo update()).
  void set_number(const char* key, double v);
  void set_string(const char* key, const char* v);
  void set_bool  (const char* key, bool v);
  void set_list  (const char* key, const char* const* items, std::size_t n);

  // EN: Read-back a bound cell's CURRENT value (L1.16-DOMRW, consumes AUD-PUB-6(e) -- the
  //     data-model was write-only before this). Reflects any RmlUi-driven bidirectional write
  //     (e.g. a user typing into a data-bound <input>), not just what the host itself last
  //     set_* -- see DataBinder::get_number's doc-comment for the pointer-identity reasoning
  //     that makes this true without any extra RmlUi call. Returns false (out untouched) when
  //     not ok() or the key was never bound -- no document/load-order guard beyond that (a
  //     bound key is readable before OR after load()).
  // PT: Lê de volta o valor CORRENTE de uma célula ligada (L1.16-DOMRW, consome AUD-PUB-6(e) --
  //     o data-model era write-only antes disto). Reflete qualquer escrita bidirecional dirigida
  //     pelo RmlUi (ex.: um usuário digitando num <input> ligado ao data-model), não só o que o
  //     próprio host definiu por último via set_* -- ver o doc-comment de DataBinder::get_number
  //     para o raciocínio de identidade-de-ponteiro que torna isto verdade sem chamada extra ao
  //     RmlUi. Retorna false (out intocado) quando não ok() ou a chave nunca foi ligada -- sem
  //     guard de documento/ordem-de-load além disso (uma chave ligada é legível antes OU depois
  //     de load()).
  bool get_number(const char* key, double& out) const;
  bool get_string(const char* key, std::string& out) const;
  bool get_bool  (const char* key, bool& out) const;

private:
  struct Impl;
  Impl* impl_;
};

} // namespace glintfx
