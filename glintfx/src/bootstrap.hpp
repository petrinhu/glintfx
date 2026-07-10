// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap — wire SystemInterface + RenderInterface into RmlUi, create Context.
//     No RmlUi or GL types leak through this header (pImpl pattern).
//     The caller is the OWNER of the SystemInterface lifetime (Bootstrap does not delete it).
// PT: Bootstrap — conecta SystemInterface + RenderInterface ao RmlUi, cria Context.
//     Nenhum tipo RmlUi ou GL vaza por este header (padrão pImpl).
//     O chamador é DONO do lifetime do SystemInterface (Bootstrap não o deleta).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Forward-declare only — callers must not depend on RmlUi headers.
// PT: Só forward-declaration — callers não devem depender dos headers do RmlUi.
namespace Rml { class Context; class SystemInterface; }

#include <functional>
#include <glintfx/click_info.hpp>

namespace glintfx {

class RenderGl3;

// EN: RAII bootstrap for RmlUi. One instance per application lifetime; not copyable.
//     The caller supplies a Rml::SystemInterface* (does not have to be GLFW-based).
// PT: Bootstrap RAII para RmlUi. Uma instância por lifetime da aplicação; não copiável.
//     O chamador fornece um Rml::SystemInterface* (não precisa ser baseado em GLFW).
class Bootstrap {
public:
  Bootstrap() = default;
  ~Bootstrap();
  Bootstrap(const Bootstrap&) = delete;
  Bootstrap& operator=(const Bootstrap&) = delete;

  // EN: Set up RmlUi system+render interfaces, call Rml::Initialise(), and create a
  //     context named "main" with dimensions w×h. The caller OWNS the lifetime of
  //     `system` and must keep it alive until shutdown() or destruction.
  //     Returns true on success. On failure the object is left in a safe state for
  //     shutdown() or destruction.
  // PT: Configura interfaces de sistema+render do RmlUi, chama Rml::Initialise() e
  //     cria contexto "main" com dimensões w×h. O chamador é DONO do lifetime de
  //     `system` e deve mantê-lo vivo até shutdown() ou destruição.
  //     Retorna true em caso de sucesso. Em falha o objeto fica em estado seguro para
  //     shutdown() ou destruição.
  bool init(Rml::SystemInterface* system, RenderGl3& render, int w, int h);

  // EN: Load a document from rml_path and call Show(). Returns true on success.
  // PT: Carrega documento de rml_path e chama Show(). Retorna true em caso de sucesso.
  bool load(const char* rml_path);

  // EN: Override the base URL for asset resolution. When set, relative paths (fonts, images,
  //     RCSS) are resolved as base_url + '/' + path instead of relative to CWD.
  //     Pass nullptr or "" to clear.
  //     Safe to call after init(). Calling before init() is a no-op — the configuration
  //     is silently lost because impl_ does not yet exist.
  // PT: Sobrepõe o base URL para resolução de assets. Quando definido, caminhos relativos
  //     (fontes, imagens, RCSS) são resolvidos como base_url + '/' + path em vez de relativo ao CWD.
  //     Passe nullptr ou "" para limpar.
  //     Seguro chamar após init(). Chamar antes de init() é no-op — a configuração é
  //     silenciosamente perdida pois impl_ ainda não existe.
  void set_asset_base_url(const char* url);

  // EN: Returns the active Rml::Context (valid after successful init()), or nullptr.
  // PT: Retorna o Rml::Context ativo (válido após init() com sucesso), ou nullptr.
  Rml::Context* context();

  // EN: Register a click callback -- reports the id of the ancestor-or-self nearest to the
  //     clicked element that has a non-empty id ("" if none). No ordering constraint versus
  //     load(): the listener reads the CURRENT callback at click time, not at attach time.
  //     Safe to call before or after load(); a no-op copy is stored even before init().
  // PT: Registra um callback de clique -- reporta o id do ancestral-ou-o-próprio mais próximo
  //     do elemento clicado que tenha id não-vazio ("" se nenhum). Sem restrição de ordem vs.
  //     load(): o listener lê o callback CORRENTE no momento do clique, não no attach.
  //     Seguro chamar antes ou depois de load(); sem efeito antes de init() (impl_ ainda nulo).
  void set_click_callback(std::function<void(const char*)> cb);

  // EN: Register a RICHER click callback -- reports id + button + coordinates + double-click,
  //     a second/parallel channel alongside set_click_callback (AUD-PUB-4, v0.5.0). Does NOT
  //     replace or interact with set_click_callback -- both can be registered simultaneously
  //     and both fire independently on the same click.
  //     LEFT (button 0): sourced from the native Rml::EventId::Click AND Rml::EventId::Dblclick
  //     events (a double-click fires THIS callback a total of 3 times: Click on
  //     mouse-down-1-up, Dblclick on mouse-down-2 with double_click=true, Click again on
  //     mouse-down-2-up -- mirrors RmlUi's own event model exactly).
  //     RIGHT/MIDDLE (button 1/2, v0.5.0 gap closure): RmlUi never dispatches Click/Dblclick for
  //     non-primary buttons, so we synthesize it from a Mousedown+Mouseup pair -- fires once,
  //     double_click always false, ONLY when the paired down and up resolved to the SAME
  //     element id (drag-off cancels, same rule as RmlUi's own left-click). See ClickInfo's
  //     doc-comment for the full contract.
  //     x/y are in Bootstrap's own content-local space (offset-free); UiLayer/App translate to
  //     window-space at the public boundary, same as get_element_box.
  //     No ordering constraint versus load(); safe to call before or after it.
  // PT: Registra um callback de clique MAIS RICO -- reporta id + botão + coordenadas +
  //     duplo-clique, um segundo canal/paralelo ao set_click_callback (AUD-PUB-4, v0.5.0). NÃO
  //     substitui nem interage com set_click_callback -- ambos podem ser registrados
  //     simultaneamente e ambos disparam independentemente no mesmo clique.
  //     ESQUERDO (botão 0): originado dos eventos nativos Rml::EventId::Click E
  //     Rml::EventId::Dblclick (um duplo-clique dispara ESTE callback um total de 3 vezes: Click
  //     no mouse-down-1-up, Dblclick no mouse-down-2 com double_click=true, Click de novo no
  //     mouse-down-2-up -- espelha exatamente o próprio modelo de evento do RmlUi).
  //     DIREITO/MEIO (botão 1/2, fechamento de gap v0.5.0): o RmlUi nunca dispara Click/Dblclick
  //     para botões não-primários, então sintetizamos a partir de um par Mousedown+Mouseup --
  //     dispara uma vez, double_click sempre false, SÓ quando o down e o up pareados resolveram
  //     para o MESMO id de elemento (arrastar-para-fora cancela, mesma regra do clique esquerdo
  //     do próprio RmlUi). Ver o doc-comment de ClickInfo para o contrato completo.
  //     x/y estão no espaço local de conteúdo próprio do Bootstrap (offset-free); UiLayer/App
  //     traduzem para espaço-janela na fronteira pública, igual a get_element_box. Sem restrição
  //     de ordem vs. load(); seguro chamar antes ou depois.
  void set_click_info_callback(std::function<void(const ClickInfo&)> cb);

  // EN: Register a scroll callback (GLINTFX-SCROLL-1 follow-up, v0.6.0) -- reports the id of the
  //     ancestor-or-self nearest to the element whose OWN scroll offset just changed, same
  //     ancestor-walk-to-nearest-id rule as set_click_callback ("" if none). Fires for EVERY
  //     source of scrolling glintfx does not itself distinguish: wheel (via
  //     Rml::Context::ProcessMouseWheel's smoothscroll animation, one Scroll event per animated
  //     step, NOT just at rest), native scrollbar thumb drag/track click/arrow click
  //     (Source/Core/WidgetScroll.cpp -- all funnel through Element::SetScrollTop/SetScrollLeft),
  //     and this library's own scroll_element_into_view()/set_element_scroll_top(). Confirmed by
  //     reading the pinned RmlUi source (Source/Core/Element.cpp: SetScrollTop/SetScrollLeft each
  //     do `if (new_offset != scroll_offset.*) { ...; DispatchEvent(EventId::Scroll,
  //     Dictionary()); }` -- deduped against the CURRENT offset, so setting the same value twice
  //     fires only once; and Source/Core/EventSpecification.cpp:39 registers EventId::Scroll with
  //     bubbles=true, target = the scrolled element itself, dispatched WITH AN EMPTY Dictionary
  //     (no wheel-delta/position parameters carried on this event -- unlike Mousescroll). A host
  //     that needs the numeric offset reads it back via the already-existing
  //     get_element_scroll_top(id) inside the callback -- kept as a SEPARATE call (not bundled
  //     into this callback's signature) because the event itself carries no such payload, mirrors
  //     set_click_callback's minimal (id-only) shape rather than set_click_info_callback's richer
  //     one (there is no button/coordinate/double-click analogue for a scroll). No ordering
  //     constraint versus load(): the listener reads the CURRENT callback at scroll time, not at
  //     attach time. A null/empty std::function is a safe no-op (ProcessEvent checks `!cb_ ||
  //     !*cb_` before touching the target element, same guard as ClickEventListener). Safe to
  //     call before or after load(); no-op before init() (impl_ still null).
  // PT: Registra um callback de rolagem (desdobramento do GLINTFX-SCROLL-1, v0.6.0) -- reporta o
  //     id do ancestral-ou-o-próprio mais próximo do elemento cujo PRÓPRIO offset de rolagem
  //     acabou de mudar, mesma regra de subida de ancestral até o id mais próximo do
  //     set_click_callback ("" se nenhum). Dispara para TODA fonte de rolagem que a glintfx não
  //     distingue internamente: wheel (via a animação smoothscroll de
  //     Rml::Context::ProcessMouseWheel, um evento Scroll por passo animado, NÃO só ao
  //     assentar), arrastar/clicar na scrollbar nativa (thumb/track/setas --
  //     Source/Core/WidgetScroll.cpp -- todos convergem em Element::SetScrollTop/SetScrollLeft),
  //     e o scroll_element_into_view()/set_element_scroll_top() da própria biblioteca.
  //     Confirmado lendo o source pinado do RmlUi (Source/Core/Element.cpp: SetScrollTop/
  //     SetScrollLeft cada um faz `if (new_offset != scroll_offset.*) { ...;
  //     DispatchEvent(EventId::Scroll, Dictionary()); }` -- deduplicado contra o offset CORRENTE,
  //     então definir o mesmo valor duas vezes dispara só uma vez; e
  //     Source/Core/EventSpecification.cpp:39 registra EventId::Scroll com bubbles=true, alvo = o
  //     próprio elemento rolado, despachado com um Dictionary VAZIO (nenhum parâmetro de
  //     delta-de-wheel/posição carregado neste evento -- diferente do Mousescroll). Um host que
  //     precisa do offset numérico o lê de volta via o já existente get_element_scroll_top(id)
  //     dentro do callback -- mantido como chamada SEPARADA (não empacotado na assinatura deste
  //     callback) porque o próprio evento não carrega esse payload, espelha a forma mínima
  //     (só-id) do set_click_callback em vez da mais rica do set_click_info_callback (não há
  //     análogo de botão/coordenada/duplo-clique para uma rolagem). Sem restrição de ordem vs.
  //     load(): o listener lê o callback CORRENTE no momento da rolagem, não no attach. Um
  //     std::function nulo/vazio é um no-op seguro (ProcessEvent checa `!cb_ || !*cb_` antes de
  //     tocar o elemento-alvo, mesmo guard do ClickEventListener). Seguro chamar antes ou depois
  //     de load(); no-op antes de init() (impl_ ainda nulo).
  //
  //     WARNING (recursion, review v0.6.0 Minor #2): set_element_scroll_top/
  //     scroll_element_into_view called SYNCHRONOUSLY dispatch EventId::Scroll on the SAME call
  //     stack (Element::SetScrollTop -> DispatchEvent before returning), which re-enters this
  //     callback. If the handler itself calls set_element_scroll_top/scroll_element_into_view
  //     with a value that does NOT converge to the element's current offset (e.g. a "clamp" that
  //     always writes something different from what it just read), it recurses without bound ->
  //     stack overflow. It IS safe to call those methods from inside this callback as long as the
  //     written value CONVERGES to the current offset -- RmlUi's own dedup inside
  //     Element::SetScrollTop (`if (new_offset != scroll_offset.*) { ...; DispatchEvent(...); }`)
  //     stops the cycle the moment the offset stops changing (typically 1-2 iterations for a
  //     constant target, immediately for a no-op write).
  //     AVISO (recursão, review v0.6.0 Minor #2): set_element_scroll_top/
  //     scroll_element_into_view chamados SINCRONAMENTE despacham EventId::Scroll na MESMA pilha
  //     de chamada (Element::SetScrollTop -> DispatchEvent antes de retornar), o que reentra
  //     neste callback. Se o próprio handler chamar set_element_scroll_top/
  //     scroll_element_into_view com um valor que NÃO converge para o offset atual do elemento
  //     (ex.: um "clamp" que sempre escreve algo diferente do que acabou de ler), ele recursa sem
  //     limite -> stack overflow. É SEGURO chamar esses métodos de dentro deste callback desde
  //     que o valor escrito CONVIRJA para o offset atual -- o dedup do próprio RmlUi dentro de
  //     Element::SetScrollTop (`if (new_offset != scroll_offset.*) { ...; DispatchEvent(...); }`)
  //     para o ciclo no instante em que o offset para de mudar (tipicamente 1-2 iterações para um
  //     alvo constante, imediato para uma escrita no-op).
  void set_scroll_callback(std::function<void(const char* id)> cb);

  // EN: Query the border-box geometry of an element by id, in the LATEST loaded document's
  //     own content-local space (top-left origin, y-down, offset-free -- UiLayer/App translate
  //     to window-space at the public boundary). Returns false if no document is loaded or the
  //     id is not found; x/y/w/h are left untouched in that case.
  // PT: Consulta a geometria border-box de um elemento por id, no espaço LOCAL de conteúdo do
  //     último documento carregado (origem topo-esquerda, y pra baixo, offset-free -- UiLayer/
  //     App traduzem pra espaço-janela na fronteira pública). Retorna false se nenhum documento
  //     estiver carregado ou o id não for encontrado; x/y/w/h ficam intocados nesse caso.
  bool get_element_box(const char* id, float& x, float& y, float& w, float& h) const;

  // EN: Scroll an element's nearest scrolling ancestor(s) so the element becomes visible
  //     (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven: GusWorld could not follow keyboard/gamepad
  //     selection when it moved outside an overflow-y:auto menu list). Thin wrapper over
  //     Rml::Element::ScrollIntoView(bool) -- NOT the ScrollIntoViewOptions overload, which
  //     exposes more knobs (horizontal alignment, ScrollBehavior::Smooth) than this API needs.
  //     align_with_top true aligns the element to the top of the scrollable view, false to the
  //     bottom -- same semantics as the wrapped RmlUi method. Instant (not animated): the
  //     ScrollIntoViewOptions default behavior is ScrollBehavior::Instant (confirmed in
  //     ScrollTypes.h), so the scroll offset is applied synchronously, before this call returns
  //     -- no need to pump update() afterwards for the new position to read back correctly via
  //     get_element_scroll_top(). Returns false (no-op) when no document is loaded or id is not
  //     found -- same guard shape as get_element_box.
  // PT: Rola o(s) ancestral(is) rolável(eis) mais próximo(s) de um elemento para que ele fique
  //     visível (GLINTFX-SCROLL-1, v0.4.0 -- consumer-driven: o GusWorld não conseguia acompanhar
  //     a seleção por teclado/gamepad quando ela saía de uma lista de menu overflow-y:auto).
  //     Wrapper fino sobre Rml::Element::ScrollIntoView(bool) -- NÃO a sobrecarga
  //     ScrollIntoViewOptions, que expõe mais opções (alinhamento horizontal, ScrollBehavior::
  //     Smooth) do que esta API precisa. align_with_top true alinha o elemento ao topo da área
  //     rolável, false ao fundo -- mesma semântica do método RmlUi encapsulado. Instantâneo (não
  //     animado): o behavior default de ScrollIntoViewOptions é ScrollBehavior::Instant
  //     (confirmado em ScrollTypes.h), então o offset de rolagem é aplicado sincronamente, antes
  //     desta chamada retornar -- não precisa bombear update() depois para a nova posição ser lida
  //     corretamente via get_element_scroll_top(). Retorna false (no-op) quando nenhum documento
  //     está carregado ou o id não é encontrado -- mesmo formato de guard de get_element_box.
  bool scroll_element_into_view(const char* id, bool align_with_top = true) const;

  // EN: Query the current vertical scroll offset of an element (its own scroll container, e.g.
  //     an overflow-y:auto div), in RCSS pixels. Thin wrapper over Rml::Element::GetScrollTop().
  //     Returns false (out_scroll_top left untouched) when no document is loaded or id is not
  //     found -- same guard shape as get_element_box. 0 for an element that is not scrolled or
  //     is not itself scrollable (GetScrollTop() returns the element's own scroll_offset.y,
  //     which stays 0.0 if never scrolled/not a scroll container).
  // PT: Consulta o offset de rolagem vertical atual de um elemento (seu próprio container de
  //     rolagem, ex.: uma div overflow-y:auto), em pixels RCSS. Wrapper fino sobre
  //     Rml::Element::GetScrollTop(). Retorna false (out_scroll_top intocado) quando nenhum
  //     documento está carregado ou o id não é encontrado -- mesmo formato de guard de
  //     get_element_box. 0 para um elemento que não foi rolado ou não é ele mesmo rolável
  //     (GetScrollTop() retorna o scroll_offset.y próprio do elemento, que permanece 0.0 se
  //     nunca rolado/não for um container de rolagem).
  bool get_element_scroll_top(const char* id, float& out_scroll_top) const;

  // EN: Query the total scrollable content height of an element, in RCSS pixels. Thin wrapper
  //     over Rml::Element::GetScrollHeight() (includes the element's own padding, excludes
  //     margin -- same box-model slice RmlUi itself documents). Part of the scroll-metrics trio
  //     (GetScrollTop/GetScrollHeight/GetClientHeight, grouped together in Element.h) a caller
  //     needs to build a CUSTOM scrollbar/scroll-percentage indicator without depending on
  //     RmlUi's own built-in one:
  //       scroll_fraction = scroll_top / (scroll_height - client_height)   // 0..1, guard /0
  //       thumb_size_fraction = client_height / scroll_height              // 0..1
  //     Returns false (out_scroll_height left untouched) when no document is loaded or id is
  //     not found -- same guard shape as get_element_box/get_element_scroll_top.
  // PT: Consulta a altura total do conteúdo rolável de um elemento, em pixels RCSS. Wrapper
  //     fino sobre Rml::Element::GetScrollHeight() (inclui o padding próprio do elemento,
  //     exclui margem -- a mesma fatia do box-model que o próprio RmlUi documenta). Parte do
  //     trio de métricas de rolagem (GetScrollTop/GetScrollHeight/GetClientHeight, agrupados
  //     juntos em Element.h) que um chamador precisa para construir um indicador de scrollbar/
  //     porcentagem de rolagem CUSTOMIZADO sem depender do scrollbar embutido do RmlUi:
  //       fracao_rolada = scroll_top / (scroll_height - client_height)   // 0..1, guardar /0
  //       fracao_tamanho_thumb = client_height / scroll_height            // 0..1
  //     Retorna false (out_scroll_height intocado) quando nenhum documento está carregado ou o
  //     id não é encontrado -- mesmo formato de guard de get_element_box/get_element_scroll_top.
  bool get_element_scroll_height(const char* id, float& out_scroll_height) const;

  // EN: Query the client (visible, content-minus-scrollbar) height of an element, in RCSS
  //     pixels. Thin wrapper over Rml::Element::GetClientHeight(). Completes the scroll-metrics
  //     trio alongside get_element_scroll_top/get_element_scroll_height -- see the usage formula
  //     in get_element_scroll_height's doc-comment above. Returns false (out_client_height left
  //     untouched) when no document is loaded or id is not found -- same guard shape as
  //     get_element_box.
  // PT: Consulta a altura de cliente (visível, conteúdo-menos-scrollbar) de um elemento, em
  //     pixels RCSS. Wrapper fino sobre Rml::Element::GetClientHeight(). Completa o trio de
  //     métricas de rolagem junto de get_element_scroll_top/get_element_scroll_height -- ver a
  //     fórmula de uso no doc-comment de get_element_scroll_height acima. Retorna false
  //     (out_client_height intocado) quando nenhum documento está carregado ou o id não é
  //     encontrado -- mesmo formato de guard de get_element_box.
  bool get_element_client_height(const char* id, float& out_client_height) const;

  // EN: Set the vertical scroll offset of an element directly, in RCSS pixels. Thin wrapper
  //     over Rml::Element::SetScrollTop(), which clamps internally to
  //     [0, GetScrollHeight() - GetClientHeight()] -- an out-of-range value (including negative)
  //     is silently clamped by RmlUi, not rejected by this method. Applied synchronously
  //     (SetScrollTop is not animated), so a subsequent get_element_scroll_top() call reads the
  //     new (clamped) value back immediately, without needing update() in between.
  //     Input hardening: rejects non-finite scroll_top (NaN/inf) with false and NO call into
  //     RmlUi -- Element::SetScrollTop does `Math::Round(Math::Clamp(Math::Round(scroll_top), ...))`
  //     with no NaN/inf guard of its own (Math::Clamp's comparisons are all false against NaN,
  //     leaving the clamp result implementation-defined), so an unguarded NaN/inf here would
  //     silently corrupt the element's scroll_offset.y with an unpredictable value -- same
  //     fail-high spirit as Engine::set_dp_ratio's std::isfinite guard (v0.3.0 hardening audit).
  //     Returns false (no-op) when no document is loaded, id is not found, or scroll_top is not
  //     finite.
  // PT: Define o offset de rolagem vertical de um elemento diretamente, em pixels RCSS. Wrapper
  //     fino sobre Rml::Element::SetScrollTop(), que internamente satura em
  //     [0, GetScrollHeight() - GetClientHeight()] -- um valor fora do intervalo (incluindo
  //     negativo) é saturado silenciosamente pelo RmlUi, não rejeitado por este método. Aplicado
  //     sincronamente (SetScrollTop não é animado), então uma chamada subsequente a
  //     get_element_scroll_top() lê o novo valor (saturado) de volta imediatamente, sem precisar
  //     de update() no meio.
  //     Hardening de entrada: rejeita scroll_top não-finito (NaN/inf) com false e SEM chamar o
  //     RmlUi -- Element::SetScrollTop faz `Math::Round(Math::Clamp(Math::Round(scroll_top), ...))`
  //     sem guard próprio de NaN/inf (as comparações de Math::Clamp são todas falsas contra NaN,
  //     deixando o resultado do clamp definido-pela-implementação), então um NaN/inf sem guard
  //     aqui corromperia silenciosamente o scroll_offset.y do elemento com um valor imprevisível
  //     -- mesmo espírito fail-high do guard std::isfinite de Engine::set_dp_ratio (auditoria de
  //     hardening v0.3.0). Retorna false (no-op) quando nenhum documento está carregado, o id não
  //     é encontrado, ou scroll_top não é finito.
  bool set_element_scroll_top(const char* id, float scroll_top) const;

  // EN: Give input focus to an element, programmatically (L1.17-FOCUS -- consumes the GAP-4
  //     seed from docs/embed-integration.md section 5: hosts whose MODEL owns selection, e.g. a
  //     game menu driven by data-binding rather than RmlUi's own Tab/arrow navigation). Thin
  //     wrapper over Rml::Element::Focus(false) -- focus_visible is always false; glintfx does
  //     not surface the RmlUi ':focus-visible' pseudo-class distinction through this API (a host
  //     that needs the visual affordance still gets it "for free" via real Tab-key navigation,
  //     which DOES set focus_visible=true on its own path).
  //     IMPORTANT -- verified by reading the pinned RmlUi 6.3 source (Source/Core/Element.cpp,
  //     Source/Core/StyleSheetSpecification.cpp), NOT assumed from HTML tabindex semantics:
  //     Element::Focus() gates ONLY on the element's RCSS `focus` property (Style::Focus,
  //     keyword `auto`|`none`), whose REGISTERED DEFAULT IS `auto` -- i.e. by default EVERY
  //     element accepts a programmatic Focus() call, including a plain unstyled `<div>`, UNLESS
  //     the author (or a UA stylesheet) explicitly authors `focus: none;`. This is DIFFERENT
  //     from the separate `tab-index` RCSS property (default `none`), which controls Tab-key
  //     TRAVERSAL ORDER only (see docs/embed-integration.md section 5) and has NO effect on
  //     this method whatsoever -- an element with the default `tab-index: none` still returns
  //     true from set_focus() as long as its `focus` property stays at its own default `auto`.
  //     There is no RmlUi/glintfx concept equivalent to an implicit "only tabindex-bearing
  //     elements are focusable" rule; a host that wants a real "this element must never be
  //     focused programmatically" gate has to author `focus: none;` explicitly on it.
  //     Returns Element::Focus()'s own bool. Guards (same fail-high shape/AUD-TEC-5 convention
  //     as get_element_box/scroll_element_into_view above): false when no document is loaded,
  //     id is null or empty (""), or the id is not found. Also false when the resolved element
  //     itself is authored `focus: none;` (RmlUi's own internal check inside Focus()).
  // PT: Dá foco de entrada a um elemento, programaticamente (L1.17-FOCUS -- consome a semente
  //     GAP-4 de docs/embed-integration.md seção 5: hosts cujo MODELO é dono da seleção, ex.: um
  //     menu de jogo dirigido por data-binding em vez da navegação Tab/setas própria do RmlUi).
  //     Wrapper fino sobre Rml::Element::Focus(false) -- focus_visible é sempre false; o glintfx
  //     não expõe a distinção da pseudo-classe ':focus-visible' do RmlUi por esta API (um host
  //     que precisa da affordance visual ainda a ganha "de graça" via navegação real por Tab,
  //     que DEFINE focus_visible=true no próprio caminho).
  //     IMPORTANTE -- verificado lendo o source pinado do RmlUi 6.3 (Source/Core/Element.cpp,
  //     Source/Core/StyleSheetSpecification.cpp), NÃO assumido a partir da semântica de tabindex
  //     do HTML: Element::Focus() se protege SÓ pela propriedade RCSS `focus` do elemento
  //     (Style::Focus, keyword `auto`|`none`), cujo DEFAULT REGISTRADO É `auto` -- ou seja, por
  //     padrão TODO elemento aceita uma chamada Focus() programática, incluindo uma `<div>` lisa
  //     sem estilo, A MENOS QUE o autor (ou uma stylesheet UA) autore explicitamente `focus:
  //     none;`. Isto é DIFERENTE da propriedade RCSS `tab-index` separada (default `none`), que
  //     controla só a ORDEM DE TRAVESSIA por tecla Tab (ver docs/embed-integration.md seção 5) e
  //     não tem NENHUM efeito neste método -- um elemento com o `tab-index: none` default ainda
  //     retorna true de set_focus() desde que sua propriedade `focus` fique no próprio default
  //     `auto`. Não há conceito no RmlUi/glintfx equivalente a uma regra implícita de "só
  //     elementos com tabindex são focáveis"; um host que quer uma guarda real de "este elemento
  //     nunca deve ser focado programaticamente" precisa autorar `focus: none;` explicitamente
  //     nele.
  //     Retorna o próprio bool de Element::Focus(). Guards (mesmo formato fail-high/convenção
  //     AUD-TEC-5 de get_element_box/scroll_element_into_view acima): false quando nenhum
  //     documento estiver carregado, id for nulo ou vazio (""), ou o id não for encontrado.
  //     Também false quando o próprio elemento resolvido está autorado com `focus: none;`
  //     (checagem interna do próprio RmlUi dentro de Focus()).
  bool set_focus(const char* id) const;

  // EN: Remove input focus (L1.17-FOCUS). Thin wrapper over Rml::Context::GetFocusElement() +
  //     Rml::Element::Blur() (Blur() itself returns void -- per the pinned RmlUi source it walks
  //     up to the parent, transferring focus there as part of RmlUi's own chain-repair logic;
  //     glintfx does not alter or observe that repair, it only triggers it).
  //     Guards: false when no document is loaded (same "must have a document" gate as every
  //     other Bootstrap query method above). Returns TRUE (deliberate idempotent no-op) when a
  //     document IS loaded but NOTHING is currently focused (Context::GetFocusElement() ==
  //     nullptr) -- there is nothing to undo, so "cleared" already holds; mirrors the general
  //     no-op-is-success convention used elsewhere in this API (e.g. a redundant
  //     set_scroll_callback(nullptr) is likewise not an error).
  //     NOTE: focus is a property of the Rml::Context, not of a specific document -- if a host
  //     ever loads more than one document into the SAME context, this clears whatever is
  //     focused context-wide, not scoped to the latest-loaded (impl_->doc) document.
  // PT: Remove o foco de entrada (L1.17-FOCUS). Wrapper fino sobre Rml::Context::GetFocusElement()
  //     + Rml::Element::Blur() (o próprio Blur() retorna void -- pelo source pinado do RmlUi, ele
  //     sobe até o pai, transferindo o foco para lá como parte da própria lógica de reparo de
  //     cadeia do RmlUi; o glintfx não altera nem observa esse reparo, só o dispara).
  //     Guards: false quando nenhum documento estiver carregado (mesma guarda de "precisa ter
  //     documento" de todo outro método de consulta do Bootstrap acima). Retorna TRUE (no-op
  //     idempotente deliberado) quando um documento ESTÁ carregado mas NADA está focado no
  //     momento (Context::GetFocusElement() == nullptr) -- não há nada para desfazer, então
  //     "limpo" já vale; espelha a convenção geral de no-op-é-sucesso usada em outro lugar desta
  //     API (ex.: um set_scroll_callback(nullptr) redundante também não é erro).
  //     NOTA: foco é propriedade do Rml::Context, não de um documento específico -- se um host
  //     algum dia carregar mais de um documento no MESMO contexto, isto limpa o que estiver
  //     focado no contexto inteiro, não restrito ao documento carregado por último (impl_->doc).
  //
  //     CORRECTION (L1.16-DOMRW review finding, 2026-07-09; ex-INBOX "clear_focus/
  //     GetFocusElement doc-precision vs. OnElementDetach"): the doc-comment above (and
  //     docs/embed-integration.md section 5) previously claimed "Rml::Context::focus is never
  //     reset to null". That is true for Element::Blur() (re-parents focus UP the tree) but NOT
  //     universally true: Rml::Context::OnElementDetach (Source/Core/Context.cpp, confirmed by
  //     reading the pinned RmlUi 6.3 source) does `if (element == focus) focus = nullptr;`
  //     whenever the CURRENTLY FOCUSED element is DETACHED from the tree -- e.g. via
  //     set_text(id, ...) replacing an element's children through SetInnerRML, or a data-for
  //     list shrinking and dropping the row that held focus. No compensating re-focus happens
  //     in that path. The code here was already defensive regardless (`if (!focused) return
  //     true;` above treats GetFocusElement()==nullptr as a legitimate, idempotent-success
  //     state), so this correction changes NO behaviour -- only the prose was stronger than
  //     reality. GetFocusElement() CAN legitimately return nullptr after a DOM mutation that
  //     detaches the focused element, in addition to the "nothing has ever been focused since
  //     Context construction" case Blur()'s reparenting never reaches on its own.
  //     CORREÇÃO (achado de review do L1.16-DOMRW, 2026-07-09; ex-INBOX "precisão de doc de
  //     clear_focus/GetFocusElement vs. OnElementDetach"): o doc-comment acima (e
  //     docs/embed-integration.md seção 5) antes afirmava "Rml::Context::focus nunca é resetado
  //     para nulo". Isso é verdade para Element::Blur() (reparenta o foco PRA CIMA na árvore)
  //     mas NÃO é universalmente verdade: Rml::Context::OnElementDetach (Source/Core/
  //     Context.cpp, confirmado lendo o source pinado do RmlUi 6.3) faz `if (element == focus)
  //     focus = nullptr;` sempre que o elemento ATUALMENTE FOCADO é DESANEXADO da árvore -- ex.:
  //     via set_text(id, ...) substituindo os filhos de um elemento por SetInnerRML, ou uma
  //     lista data-for encolhendo e descartando a linha que segurava o foco. Nenhum re-foco
  //     compensatório acontece nesse caminho. O código aqui já era defensivo de qualquer forma
  //     (`if (!focused) return true;` acima trata GetFocusElement()==nullptr como um estado
  //     legítimo, sucesso idempotente), então esta correção não muda NENHUM comportamento -- só
  //     a prosa era mais forte que a realidade. GetFocusElement() PODE legitimamente retornar
  //     nullptr após uma mutação de DOM que desanexa o elemento focado, além do caso "nada foi
  //     focado desde a construção do Context" que o reparenting do Blur() sozinho nunca alcança.
  bool clear_focus() const;

  // -------------------------------------------------------------------------
  // EN: DOM read/write by id (L1.16-DOMRW, consumes AUD-PUB-6(d) -- "imperative setters by id",
  //     the second half of the read/write pair alongside the data-model get_* on Engine/
  //     UiLayer/App). Mirrors the guard shape of get_element_box/scroll_element_into_view/
  //     set_focus above: `!impl_ || !impl_->doc || !id || !*id` -> false (AUD-TEC-5 fail-high).
  //     Verified against the pinned RmlUi 6.3 source (Include/RmlUi/Core/Element.h):
  //       void SetClass(const String& class_name, bool activate);      // add/remove, no bool
  //       bool SetProperty(const String& name, const String& value);   // real parse outcome
  //       void SetInnerRML(const String& rml);                          // no bool, parses RML
  //     SetClass/SetInnerRML return void -- unlike SetProperty, there is no RmlUi-level
  //     success/failure to propagate from them; this class's own bool return communicates only
  //     "the guarded id (and, for set_property, prop) were valid and the element was found".
  // PT: Leitura/escrita de DOM por id (L1.16-DOMRW, consome AUD-PUB-6(d) -- "setters
  //     imperativos por id", a segunda metade do par leitura/escrita junto do get_* do
  //     data-model em Engine/UiLayer/App). Espelha o formato de guard de get_element_box/
  //     scroll_element_into_view/set_focus acima: `!impl_ || !impl_->doc || !id || !*id` ->
  //     false (fail-high AUD-TEC-5). Verificado contra o source pinado do RmlUi 6.3
  //     (Include/RmlUi/Core/Element.h):
  //       void SetClass(const String& class_name, bool activate);      // add/remove, sem bool
  //       bool SetProperty(const String& name, const String& value);   // resultado real de parse
  //       void SetInnerRML(const String& rml);                          // sem bool, parseia RML
  //     SetClass/SetInnerRML retornam void -- diferente de SetProperty, não há sucesso/falha em
  //     nível RmlUi para propagar deles; o próprio retorno bool desta classe comunica só "o id
  //     guardado (e, para set_property, prop) eram válidos e o elemento foi encontrado".
  // -------------------------------------------------------------------------

  // EN: Replace an element's text content, treating `text` as LITERAL TEXT -- NOT markup.
  //     HARDENING (mandatory, not optional): `&`, `<`, `>`, `"` are escaped via
  //     Rml::StringUtilities::EncodeRml(text) (confirmed in the pinned RmlUi source,
  //     Source/Core/StringUtilities.cpp -- the SAME encoder RmlUi's own ElementText.cpp uses
  //     internally to serialise text nodes back to RML) BEFORE the result reaches
  //     Element::SetInnerRML(). Without this, a host that forwards untrusted text (chat, a
  //     player-chosen display name, any user-generated string) straight into SetInnerRML would
  //     let that string inject arbitrary RML/tags into the document -- the public promise of
  //     this method is "set the TEXT", not "set arbitrary markup", so skipping the escape would
  //     be an RML-injection vulnerability, structurally the same class of bug as building SQL
  //     via string concatenation. A null `text` is normalised to "" (same convention as
  //     DataBinder::set_string's `v ? v : ""`), never rejected -- an empty text content is a
  //     legitimate value, not an error.
  // PT: Substitui o conteúdo de texto de um elemento, tratando `text` como TEXTO LITERAL -- NÃO
  //     markup. HARDENING (obrigatório, não opcional): `&`, `<`, `>`, `"` são escapados via
  //     Rml::StringUtilities::EncodeRml(text) (confirmado no source pinado do RmlUi,
  //     Source/Core/StringUtilities.cpp -- o MESMO encoder que o próprio ElementText.cpp do
  //     RmlUi usa internamente para serializar nós de texto de volta pra RML) ANTES do
  //     resultado chegar em Element::SetInnerRML(). Sem isto, um host que encaminha texto não
  //     confiável (chat, nome de exibição escolhido pelo jogador, qualquer string gerada por
  //     usuário) direto para SetInnerRML deixaria essa string injetar RML/tags arbitrários no
  //     documento -- a promessa pública deste método é "definir o TEXTO", não "definir markup
  //     arbitrário", então pular o escape seria uma vulnerabilidade de injeção de RML,
  //     estruturalmente a mesma classe de bug que construir SQL por concatenação de string. Um
  //     `text` nulo é normalizado para "" (mesma convenção do `v ? v : ""` de
  //     DataBinder::set_string), nunca rejeitado -- conteúdo de texto vazio é um valor
  //     legítimo, não um erro.
  bool set_text(const char* id, const char* text) const;

  // EN: Add ("activate") a CSS class on an element -- thin wrapper over
  //     Rml::Element::SetClass(cls, true). `cls` null or empty ("") is rejected (false) BEFORE
  //     touching RmlUi, same AUD-TEC-5 fail-high discipline as `id` -- an empty class name is
  //     not a meaningful operation to forward (SetClass with an empty string is well-defined in
  //     RmlUi but semantically pointless, and treating it as an error catches an obvious caller
  //     bug -- e.g. a computed class string that resolved empty -- the same way the existing
  //     id-empty guards do).
  // PT: Adiciona ("ativa") uma classe CSS num elemento -- wrapper fino sobre
  //     Rml::Element::SetClass(cls, true). `cls` nulo ou vazio ("") é rejeitado (false) ANTES
  //     de tocar o RmlUi, mesma disciplina fail-high AUD-TEC-5 de `id` -- um nome de classe
  //     vazio não é uma operação significativa a encaminhar (SetClass com string vazia é
  //     bem-definido no RmlUi mas semanticamente inútil, e tratá-lo como erro captura um bug
  //     óbvio do chamador -- ex.: uma string de classe computada que resolveu vazia -- do mesmo
  //     jeito que os guards de id-vazio existentes fazem).
  bool add_class(const char* id, const char* cls) const;

  // EN: Remove ("deactivate") a CSS class from an element -- thin wrapper over
  //     Rml::Element::SetClass(cls, false). Same `cls` null/empty guard as add_class above.
  //     Removing a class the element never had is a safe no-op inside RmlUi itself (SetClass's
  //     false branch is a set-difference operation), so this still returns true in that case --
  //     the guard here only rejects a structurally-invalid `cls`, not a semantically-redundant
  //     one.
  // PT: Remove ("desativa") uma classe CSS de um elemento -- wrapper fino sobre
  //     Rml::Element::SetClass(cls, false). Mesmo guard de `cls` nulo/vazio do add_class acima.
  //     Remover uma classe que o elemento nunca teve é um no-op seguro dentro do próprio RmlUi
  //     (o ramo false de SetClass é uma operação de diferença de conjunto), então ainda retorna
  //     true nesse caso -- o guard aqui só rejeita um `cls` estruturalmente inválido, não um
  //     semanticamente redundante.
  bool remove_class(const char* id, const char* cls) const;

  // EN: Set a single inline RCSS property by name -- thin wrapper over
  //     Rml::Element::SetProperty(name, value), which (unlike SetClass/SetInnerRML above) DOES
  //     return a real bool: RmlUi's own property parser rejects an unparseable `value` for the
  //     named property (e.g. `set_property(id, "color", "not-a-color")`) and SetProperty
  //     propagates that rejection as false -- this method forwards RmlUi's own bool unchanged,
  //     it does not invent a separate glintfx-level validation pass ("propagate the false from
  //     invalid parse", per the task brief). `prop` null or empty ("") is rejected (false)
  //     BEFORE touching RmlUi, same AUD-TEC-5 discipline as `id`/`cls` above -- an empty
  //     property NAME cannot resolve to any real RCSS property, so it is a structurally-invalid
  //     caller input, distinct from an unparseable `value` (which IS forwarded to RmlUi's own
  //     parser rather than pre-rejected here, since only RmlUi itself knows which values are
  //     valid for which properties). `value` null is normalised to "" (same convention as
  //     set_text/DataBinder::set_string) and forwarded as-is -- RmlUi's own parser is the
  //     authority on whether an empty value is acceptable for the named property.
  // PT: Define uma única propriedade RCSS inline por nome -- wrapper fino sobre
  //     Rml::Element::SetProperty(name, value), que (diferente de SetClass/SetInnerRML acima)
  //     DE FATO retorna um bool real: o próprio parser de propriedade do RmlUi rejeita um
  //     `value` que não parseia para a propriedade nomeada (ex.: `set_property(id, "color",
  //     "not-a-color")`) e SetProperty propaga essa rejeição como false -- este método repassa
  //     o próprio bool do RmlUi sem mudança, não inventa uma passada de validação separada em
  //     nível glintfx ("propague o false de parse inválido", conforme o brief da tarefa). `prop`
  //     nulo ou vazio ("") é rejeitado (false) ANTES de tocar o RmlUi, mesma disciplina
  //     AUD-TEC-5 de `id`/`cls` acima -- um NOME de propriedade vazio não resolve para nenhuma
  //     propriedade RCSS real, então é entrada estruturalmente inválida do chamador, distinta
  //     de um `value` que não parseia (que É encaminhado ao próprio parser do RmlUi em vez de
  //     pré-rejeitado aqui, já que só o próprio RmlUi sabe quais valores são válidos para quais
  //     propriedades). `value` nulo é normalizado para "" (mesma convenção de
  //     set_text/DataBinder::set_string) e encaminhado como está -- o próprio parser do RmlUi é
  //     a autoridade sobre se um valor vazio é aceitável para a propriedade nomeada.
  bool set_property(const char* id, const char* prop, const char* value) const;

  // EN: Shutdown RmlUi and release all resources. Safe to call multiple times.
  //     Does NOT delete the SystemInterface — the caller owns it.
  // PT: Encerra o RmlUi e libera todos os recursos. Seguro chamar múltiplas vezes.
  //     NÃO deleta o SystemInterface — o chamador é o dono.
  void shutdown();

private:
  struct Impl;        // EN: defined in bootstrap.cpp; hides RmlUi types.
                      // PT: definido em bootstrap.cpp; oculta tipos RmlUi.
  Impl* impl_ = nullptr;
};

} // namespace glintfx
