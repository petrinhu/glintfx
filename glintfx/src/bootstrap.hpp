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
