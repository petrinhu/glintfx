// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap implementation — wires caller-supplied SystemInterface + RenderInterface
//     into RmlUi. The caller is the OWNER of the SystemInterface lifetime.
// PT: Implementação do Bootstrap — conecta SystemInterface (fornecida pelo chamador) +
//     RenderInterface ao RmlUi. O chamador é DONO do lifetime do SystemInterface.
// Copyright (c) 2026 Petrus Silva Costa
#include "bootstrap.hpp"
#include "render_gl3.hpp"
#include "base_url_file_interface.hpp"
#include "decorator_polygon.hpp"
#include "decorator_image_tint.hpp"
#include "ua_stylesheet.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Core/StreamMemory.h>
#include <cmath>   // EN: std::isfinite (set_element_scroll_top input hardening).
                   // PT: std::isfinite (hardening de entrada de set_element_scroll_top).
#include <cstring>
#include <string>  // EN: std::string -- pending right/middle-click down-target storage below.
                   // PT: std::string -- armazenamento do alvo pendente de down direito/meio abaixo.

namespace glintfx {

// EN: Pending-down record for right/middle-click synthesis (AUD-PUB-4 gap closure, v0.5.0).
//     Declared at NAMESPACE scope, deliberately NOT nested inside Bootstrap::Impl -- Impl
//     itself is a PRIVATE member type of Bootstrap (`private: struct Impl;` in bootstrap.hpp),
//     so only Bootstrap's own member functions may name `Bootstrap::Impl`. The
//     ClickInfoButtonDownListener/UpListener classes below are ordinary Rml::EventListener
//     subclasses -- neither members nor friends of Bootstrap -- so they cannot hold a
//     `Bootstrap::Impl*` (would be an access-control error at the point of use, same reason the
//     existing ClickEventListener/ClickInfoEventListener above hold raw
//     `std::function<...>*` pointers into Impl's fields instead of an `Impl*`). This type is
//     free-standing for the identical reason; Impl (below) holds an array of it as a plain
//     member, and the listener pair is handed a pointer straight to that array.
// PT: Registro de down pendente para a síntese de clique direito/meio (fechamento de gap
//     AUD-PUB-4, v0.5.0). Declarado em escopo de NAMESPACE, deliberadamente NÃO aninhado dentro
//     de Bootstrap::Impl -- o próprio Impl é um tipo membro PRIVADO de Bootstrap
//     (`private: struct Impl;` em bootstrap.hpp), então só as funções-membro do próprio
//     Bootstrap podem nomear `Bootstrap::Impl`. As classes ClickInfoButtonDownListener/
//     UpListener abaixo são subclasses comuns de Rml::EventListener -- nem membros nem friends
//     de Bootstrap -- então não podem guardar um `Bootstrap::Impl*` (seria erro de controle de
//     acesso no ponto de uso, mesmo motivo pelo qual os ClickEventListener/
//     ClickInfoEventListener já existentes acima guardam ponteiros crus
//     `std::function<...>*` para campos do Impl em vez de um `Impl*`). Este tipo é
//     independente pelo motivo idêntico; o Impl (abaixo) guarda um array dele como membro
//     comum, e o par de listeners recebe um ponteiro direto para esse array.
struct PendingButtonDown {
  bool valid = false;
  std::string id;
};

// EN: Impl tracks context, initialisation state, and owns the process-global FileInterface.
//     The FileInterface must outlive Rml::Shutdown() — keeping it in Impl satisfies that.
//     The SystemInterface pointer is NOT stored here — the caller owns it.
// PT: Impl rastreia contexto, estado de inicialização e é dono do FileInterface global de processo.
//     O FileInterface deve sobreviver ao Rml::Shutdown() — mantê-lo no Impl satisfaz isso.
//     O ponteiro do SystemInterface NÃO é armazenado aqui — o chamador é dono.
struct Bootstrap::Impl {
  BaseUrlFileInterface file_iface;  // EN: installed before Rml::Initialise(); mutable base_url.
                                    // PT: instalado antes de Rml::Initialise(); base_url mutável.
  // EN: "polygon(<sides>, <color>[, <rotation>])" decorator instancer (v0.2.6). Held as an
  //     UniquePtr, DEFAULT-CONSTRUCTED TO NULL HERE and only allocated AFTER Rml::Initialise()
  //     returns true (see init() below) -- this is NOT the same lifetime pattern as file_iface
  //     above (which IS a by-value member constructed eagerly, before Initialise()).
  //     PolygonDecoratorInstancer's constructor calls EffectSpecification::RegisterProperty()
  //     /AddParser(), which look up globally-registered parsers ("number", "color", "angle") by
  //     name in StyleSheetSpecification's registry. That registry (and the PropertyId counter
  //     backing RegisterProperty) is populated by StyleSheetSpecification::Initialise(), which
  //     Rml::Initialise() calls internally BEFORE Factory::Initialise() constructs RmlUi's own
  //     built-in instancers (see Source/Core/Core.cpp and Source/Core/Factory.cpp in the pinned
  //     RmlUi source) -- so constructing this instancer any earlier reads an empty/not-yet-
  //     initialised registry. This was found the hard way: making it a by-value member (like
  //     file_iface, constructed at `new Impl()` time, i.e. BEFORE Rml::Initialise()) segfaulted
  //     every test that called Bootstrap::init() (SIGSEGV, rc=139 under Xvfb) -- window_smoke/
  //     render_smoke (which never call init()) still passed, isolating the cause to this
  //     ordering. Once allocated (after Initialise() succeeds), it must still outlive
  //     Rml::Shutdown() -- RmlUi's Factory keeps a raw, non-owning pointer to every registered
  //     instancer -- which UniquePtr's automatic destruction (impl_ is only ever deleted AFTER
  //     Rml::Shutdown() completes, in both init()'s CreateContext-failure path and shutdown()
  //     below) still satisfies, same end result as file_iface's by-value approach.
  // PT: Instancer do decorator "polygon(<lados>, <cor>[, <rotacao>])" (v0.2.6). Guardado como
  //     UniquePtr, DEFAULT-CONSTRUÍDO NULO AQUI e só alocado APÓS Rml::Initialise() retornar
  //     true (ver init() abaixo) -- NÃO é o mesmo padrão de lifetime do file_iface acima (que É
  //     um membro por valor construído avidamente, antes do Initialise()).
  //     O construtor de PolygonDecoratorInstancer chama EffectSpecification::RegisterProperty()
  //     /AddParser(), que buscam parsers registrados globalmente ("number", "color", "angle")
  //     por nome no registro do StyleSheetSpecification. Esse registro (e o contador de
  //     PropertyId por trás de RegisterProperty) é populado por
  //     StyleSheetSpecification::Initialise(), que o Rml::Initialise() chama internamente ANTES
  //     de Factory::Initialise() construir os próprios instancers embutidos do RmlUi (ver
  //     Source/Core/Core.cpp e Source/Core/Factory.cpp no source pinado do RmlUi) -- então
  //     construir este instancer mais cedo lê um registro vazio/ainda-não-inicializado. Isso foi
  //     descoberto na marra: torná-lo um membro por valor (como file_iface, construído no
  //     momento do `new Impl()`, ou seja, ANTES do Rml::Initialise()) fazia segfault em todo
  //     teste que chamava Bootstrap::init() (SIGSEGV, rc=139 sob Xvfb) -- window_smoke/
  //     render_smoke (que nunca chamam init()) continuavam passando, isolando a causa nessa
  //     ordem. Uma vez alocado (após Initialise() ter sucesso), ainda precisa sobreviver ao
  //     Rml::Shutdown() -- a Factory do RmlUi guarda um ponteiro cru, não-dono, de cada
  //     instancer registrado -- o que a destruição automática do UniquePtr (impl_ só é deletado
  //     APÓS Rml::Shutdown() terminar, tanto no caminho de falha de CreateContext em init()
  //     quanto em shutdown() abaixo) ainda satisfaz, mesmo resultado final da abordagem por
  //     valor do file_iface.
  Rml::UniquePtr<glintfx::PolygonDecoratorInstancer> polygon_instancer;
  // EN: GLINTFX-TINT-3 -- "image-tint(<url>)" decorator instancer (ADR-0010). Same lifetime
  //     discipline as polygon_instancer immediately above (see that field's long doc comment for
  //     the full ordering rationale, which applies identically here: the SAME
  //     StyleSheetSpecification/Factory initialisation-order constraint governs both) --
  //     default-null here, allocated in init() only AFTER Rml::Initialise() succeeds (this
  //     instancer's ctor ALSO calls Rml::StyleSheetSpecification::RegisterProperty for the three
  //     global "image-tint-*" properties, not just EffectSpecification::RegisterProperty for its
  //     own `src` shorthand argument -- see decorator_image_tint.cpp's ctor), and outlives
  //     Rml::Shutdown() for the identical reason (Rml::Factory::RegisterDecoratorInstancer stores
  //     a raw, non-owning pointer).
  // PT: GLINTFX-TINT-3 -- instancer do decorator "image-tint(<url>)" (ADR-0010). Mesma
  //     disciplina de lifetime de polygon_instancer logo acima (ver o doc-comment longo daquele
  //     campo pra racional completa de ordenação, que se aplica identicamente aqui: a MESMA
  //     restrição de ordem-de-inicialização de StyleSheetSpecification/Factory governa os dois)
  //     -- default-nulo aqui, alocado em init() só APÓS Rml::Initialise() ter sucesso (o ctor
  //     deste instancer TAMBÉM chama Rml::StyleSheetSpecification::RegisterProperty pras três
  //     propriedades globais "image-tint-*", não só EffectSpecification::RegisterProperty pro
  //     próprio argumento de shorthand `src` -- ver o ctor de decorator_image_tint.cpp), e
  //     sobrevive ao Rml::Shutdown() pelo motivo idêntico (Rml::Factory::
  //     RegisterDecoratorInstancer guarda um ponteiro cru, não-dono).
  Rml::UniquePtr<glintfx::ImageTintDecoratorInstancer> tint_instancer;
  Rml::Context* ctx = nullptr;
  Rml::ElementDocument* doc = nullptr;  // NEW (F1/F2, v0.2.5): last-loaded document.
  bool initialised  = false;
  // EN: Parsed once in init(), reused as a merge base by every load() -- see there.
  // PT: Parseada uma vez em init(), reutilizada como base de merge por todo load() -- ver lá.
  Rml::SharedPtr<Rml::StyleSheetContainer> ua_style_sheet;
  std::function<void(const char*)> click_cb;  // NEW (F1): empty by default.
  // EN: AUD-PUB-4 (v0.5.0) -- second/parallel channel, see Bootstrap::set_click_info_callback.
  //     Empty by default; independent of click_cb (both can be set, both fire on the same click).
  // PT: AUD-PUB-4 (v0.5.0) -- segundo canal/paralelo, ver Bootstrap::set_click_info_callback.
  //     Vazio por padrão; independente de click_cb (ambos podem ser setados, ambos disparam no mesmo clique).
  std::function<void(const ClickInfo&)> click_info_cb;
  // EN: GLINTFX-SCROLL-1 follow-up (v0.6.0) -- see Bootstrap::set_scroll_callback for the full
  //     contract (fires on every source of scrolling: wheel, native scrollbar drag, and the
  //     programmatic scroll_element_into_view/set_element_scroll_top). Empty by default.
  // PT: Desdobramento do GLINTFX-SCROLL-1 (v0.6.0) -- ver Bootstrap::set_scroll_callback para o
  //     contrato completo (dispara em toda fonte de rolagem: wheel, arraste de scrollbar nativa,
  //     e o scroll_element_into_view/set_element_scroll_top programático). Vazio por padrão.
  std::function<void(const char*)> scroll_cb;

  // EN: AUD-PUB-4 gap closure (v0.5.0) -- pending "button went down here" record for the
  //     right/middle click synthesis (see ClickInfoButtonDownListener/UpListener below, and the
  //     PendingButtonDown type comment above for why this is a namespace-scope type, not a
  //     nested one). Indexed DIRECTLY by RmlUi button_index (0=left, 1=right, 2=middle); index
  //     0 is never written/read (left goes through the native Click/Dblclick path above,
  //     unchanged) -- wasting one slot buys a trivial, mistake-proof `pending[button]` at both
  //     call sites instead of an off-by-one `pending[button - 1]`. Reset to `valid=false` at
  //     the top of every load() (see below) so a down that never got its matching up before a
  //     document reload cannot spuriously match an unrelated id in the NEW document.
  // PT: Fechamento de gap AUD-PUB-4 (v0.5.0) -- registro pendente "o botão desceu aqui" para a
  //     síntese de clique direito/meio (ver ClickInfoButtonDownListener/UpListener abaixo, e o
  //     comentário do tipo PendingButtonDown acima pro motivo de ser um tipo de escopo de
  //     namespace, não aninhado). Indexado DIRETAMENTE pelo button_index do RmlUi (0=esquerdo,
  //     1=direito, 2=meio); o índice 0 nunca é escrito/lido (esquerdo passa pelo caminho nativo
  //     Click/Dblclick acima, inalterado) -- desperdiçar um slot compra um `pending[button]`
  //     trivial e à prova de erro nos dois call sites, em vez de um `pending[button - 1]`
  //     off-by-one. Resetado para `valid=false` no topo de todo load() (ver abaixo) para que um
  //     down que nunca recebeu seu up pareado antes de um reload de documento não bata
  //     acidentalmente com um id não relacionado no documento NOVO.
  PendingButtonDown pending_button_down[3];
};

// EN: Bubble-phase "click" listener attached to each loaded document's root (F1, v0.2.5).
//     Self-deletes on OnDetach -- RmlUi's own idiom, confirmed in the pinned samples
//     (Samples/basic/{benchmark,animation,harfbuzz,demo}/src/*.cpp).
// PT: Listener de "click" em fase bubble anexado à raiz de cada documento carregado (F1,
//     v0.2.5). Autodeleta em OnDetach -- idioma do próprio RmlUi, confirmado nos samples
//     pinados.
class ClickEventListener : public Rml::EventListener {
public:
  ClickEventListener(std::function<void(const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    Rml::Element* el = event.GetTargetElement();
    // EN: Bug found in Step 3 (v0.2.5): a plain "walk to the first non-empty id" loop with no
    //     stop condition overshoots the document -- Context.cpp:65 does `root->SetId(name)` on
    //     the CONTEXT's internal root element (the one above every ElementDocument, named after
    //     the context -- "main" here), so an author-authored subtree with NO id anywhere would
    //     silently report that internal "main" id instead of "". Stop the walk AT doc_ (never
    //     ask GetParentNode() past it) so only ids the RML AUTHOR could have written are ever
    //     reported; the document's own id (usually unset) is still checked once, inclusively.
    // PT: Bug encontrado no Step 3 (v0.2.5): um loop simples "sobe até o 1º id não-vazio" sem
    //     condição de parada ultrapassa o documento -- Context.cpp:65 faz `root->SetId(name)`
    //     no elemento root INTERNO do contexto (o que fica acima de todo ElementDocument,
    //     nomeado com o nome do contexto -- "main" aqui), então uma subárvore autorada SEM id
    //     em lugar nenhum reportaria silenciosamente esse id interno "main" em vez de "". Para
    //     a subida EM doc_ (nunca chama GetParentNode() além dele) para que só ids que o AUTOR
    //     do RML poderia ter escrito sejam reportados; o id do próprio documento (normalmente
    //     não definido) ainda é checado uma vez, de forma inclusiva.
    while (el && el->GetId().empty() && el != doc_) el = el->GetParentNode();
    const char* id_str = (el && !el->GetId().empty()) ? el->GetId().c_str() : "";
    // EN: Copy the functor before invoking (AUD-TEC-3): cb_ points into
    //     Bootstrap::Impl::click_cb. If the callback itself calls set_click_callback(...) (a
    //     plausible pattern -- "clicked 'Play' -> swap in the next screen's handler"), that
    //     std::move-assigns a NEW value into *cb_ while THIS invocation is still executing,
    //     which would destroy the std::function's current target out from under itself ->
    //     use-after-free. A local copy survives reentrant reassignment of *cb_.
    // PT: Copia o functor antes de invocar (AUD-TEC-3): cb_ aponta para
    //     Bootstrap::Impl::click_cb. Se o próprio callback chamar set_click_callback(...)
    //     (padrão plausível -- "clicou em 'Play' -> troca o handler da tela seguinte"), isso
    //     faz std::move-assign de um valor NOVO em *cb_ enquanto ESTA invocação ainda está
    //     executando, destruindo o alvo atual do std::function debaixo de si mesmo ->
    //     use-after-free. Uma cópia local sobrevive à reatribuição reentrante de *cb_.
    auto cb = *cb_;
    if (cb) cb(id_str);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  // EN: Points into Bootstrap::Impl::click_cb -- outlives this listener (Impl is destroyed
  //     only AFTER Rml::Shutdown(), which triggers OnDetach on every live listener first).
  // PT: Aponta pra dentro de Bootstrap::Impl::click_cb -- sobrevive a este listener (Impl só é
  //     destruído APÓS Rml::Shutdown(), que dispara OnDetach em todo listener vivo antes).
  std::function<void(const char*)>* cb_;
  // EN: Document boundary for the ancestor walk above -- never a dangling pointer while this
  //     listener is alive: OnDetach() (which self-deletes) fires when the SAME document is
  //     unloaded/destroyed, so doc_ and `this` share an equal-or-shorter lifetime by construction.
  // PT: Fronteira de documento para a subida de ancestral acima -- nunca é ponteiro pendente
  //     enquanto este listener está vivo: OnDetach() (que autodeleta) dispara quando o MESMO
  //     documento é descarregado/destruído, então doc_ e `this` compartilham lifetime
  //     igual-ou-menor por construção.
  Rml::ElementDocument* doc_;
};

// EN: Bubble-phase "click"/"dblclick" listener attached to each loaded document's root
//     (AUD-PUB-4, v0.5.0) -- the ClickInfo-reporting sibling of ClickEventListener above. TWO
//     instances are attached per load() (see load() below), one on Rml::EventId::Click
//     (is_double_=false) and one on Rml::EventId::Dblclick (is_double_=true) -- a SINGLE
//     instance is deliberately NOT shared across both AddEventListener registrations: each
//     instance self-deletes independently in its own OnDetach (RmlUi's own idiom, same as
//     ClickEventListener above), and attaching one instance to two different EventIds would
//     make OnDetach fire twice against the same `this`, a double-delete. Same ancestor-walk
//     and reentrancy-safe local-copy-before-invoke discipline as ClickEventListener (AUD-TEC-3).
// PT: Listener de "click"/"dblclick" em fase bubble anexado à raiz de cada documento carregado
//     (AUD-PUB-4, v0.5.0) -- o irmão que reporta ClickInfo do ClickEventListener acima. DUAS
//     instâncias são anexadas por load() (ver load() abaixo), uma em Rml::EventId::Click
//     (is_double_=false) e uma em Rml::EventId::Dblclick (is_double_=true) -- uma ÚNICA
//     instância deliberadamente NÃO é compartilhada entre os dois registros de
//     AddEventListener: cada instância autodeleta independentemente no próprio OnDetach (idioma
//     do próprio RmlUi, igual ao ClickEventListener acima), e anexar uma instância a dois
//     EventId diferentes faria OnDetach disparar duas vezes contra o mesmo `this`, um
//     double-delete. Mesma subida de ancestral e disciplina de cópia local antes de invocar,
//     segura contra reentrância, do ClickEventListener (AUD-TEC-3).
class ClickInfoEventListener : public Rml::EventListener {
public:
  ClickInfoEventListener(std::function<void(const ClickInfo&)>* cb, Rml::ElementDocument* doc,
                          bool is_double)
      : cb_(cb), doc_(doc), is_double_(is_double) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Same ancestor-walk-to-nearest-id as ClickEventListener::ProcessEvent above (id="" if
    //     no ancestor up to doc_ has one) -- see the detailed comment there for why the walk
    //     stops AT doc_ instead of overshooting into the context's internal root element.
    // PT: Mesma subida de ancestral até o id mais próximo do ClickEventListener::ProcessEvent
    //     acima (id="" se nenhum ancestral até doc_ tiver um) -- ver o comentário detalhado lá
    //     do motivo de a subida parar EM doc_ em vez de ultrapassar pro elemento root interno
    //     do contexto.
    Rml::Element* el = event.GetTargetElement();
    while (el && el->GetId().empty() && el != doc_) el = el->GetParentNode();
    const char* id_str = (el && !el->GetId().empty()) ? el->GetId().c_str() : "";

    // EN: Parameter names confirmed by grepping the pinned RmlUi source
    //     (Source/Core/Context.cpp:1538-1541, GenerateMouseEventParameters): "button", "mouse_x",
    //     "mouse_y". button defaults to 0 (left) when absent -- matches Click/Dblclick's own
    //     implicit left-button-only dispatch (see ClickInfo's doc-comment "KNOWN LIMITATION").
    // PT: Nomes de parâmetro confirmados grepando o source pinado do RmlUi
    //     (Source/Core/Context.cpp:1538-1541, GenerateMouseEventParameters): "button", "mouse_x",
    //     "mouse_y". button tem default 0 (esquerdo) quando ausente -- bate com o despacho
    //     implícito só-botão-esquerdo do próprio Click/Dblclick (ver "LIMITAÇÃO CONHECIDA" no
    //     doc-comment de ClickInfo).
    ClickInfo info;
    info.id           = id_str;
    info.button       = event.GetParameter<int>("button", 0);
    info.x            = event.GetParameter<float>("mouse_x", 0.f);
    info.y            = event.GetParameter<float>("mouse_y", 0.f);
    info.double_click = is_double_;

    // EN: Copy the functor before invoking (AUD-TEC-3, same reentrancy guard as
    //     ClickEventListener::ProcessEvent above -- see the detailed comment there).
    // PT: Copia o functor antes de invocar (AUD-TEC-3, mesma guarda de reentrância do
    //     ClickEventListener::ProcessEvent acima -- ver o comentário detalhado lá).
    auto cb = *cb_;
    if (cb) cb(info);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const ClickInfo&)>* cb_;
  Rml::ElementDocument* doc_;
  bool is_double_;
};

// EN: Mousedown half of the right/middle-click synthesis pair (AUD-PUB-4 gap closure, v0.5.0).
//     Bubble-phase listener attached to Rml::EventId::Mousedown on the document root -- fires
//     for EVERY button (RmlUi dispatches Mousedown for all of them, Context.cpp:649/703), but
//     only ACTS when button != 0: button 0 (left) is deliberately left untouched here, it is
//     already fully covered by the native Click/Dblclick path above (ClickInfoEventListener) --
//     acting on it too would double-fire click_info_cb for every left click.
//     Records the ancestor-walked id (SAME walk as ClickEventListener/ClickInfoEventListener --
//     stop at doc_, never overshoot into the context's internal root) into
//     Impl::pending_button_down[button], to be consumed by ClickInfoButtonUpListener below.
//     Does NOT invoke click_info_cb itself -- a click is a down+up PAIR, only the up (if it
//     matches) fires the callback.
// PT: Metade Mousedown do par de síntese de clique direito/meio (fechamento de gap AUD-PUB-4,
//     v0.5.0). Listener em fase bubble anexado a Rml::EventId::Mousedown na raiz do documento --
//     dispara para TODO botão (o RmlUi despacha Mousedown para todos eles, Context.cpp:649/703),
//     mas só AGE quando button != 0: o botão 0 (esquerdo) é deliberadamente deixado intocado
//     aqui, já é totalmente coberto pelo caminho nativo Click/Dblclick acima
//     (ClickInfoEventListener) -- agir nele também dispararia click_info_cb em dobro para todo
//     clique esquerdo.
//     Registra o id obtido pela subida de ancestral (MESMA subida de
//     ClickEventListener/ClickInfoEventListener -- para em doc_, nunca ultrapassa pro root
//     interno do contexto) em Impl::pending_button_down[button], a ser consumido pelo
//     ClickInfoButtonUpListener abaixo. NÃO invoca click_info_cb por si só -- um clique é um PAR
//     down+up, só o up (se bater) dispara o callback.
class ClickInfoButtonDownListener : public Rml::EventListener {
public:
  // EN: `pending` points into Impl::pending_button_down (a 3-entry array) -- see the
  //     PendingButtonDown type comment above for why this is a raw pointer to a namespace-scope
  //     type rather than a `Bootstrap::Impl*`.
  // PT: `pending` aponta para dentro de Impl::pending_button_down (array de 3 entradas) -- ver
  //     o comentário do tipo PendingButtonDown acima pro motivo de ser um ponteiro cru pra um
  //     tipo de escopo de namespace em vez de um `Bootstrap::Impl*`.
  ClickInfoButtonDownListener(PendingButtonDown* pending, Rml::ElementDocument* doc)
      : pending_(pending), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    // EN: Parameter name/semantics confirmed the same way as ClickInfoEventListener above
    //     (Source/Core/Context.cpp:1538-1541 GenerateMouseEventParameters) -- Mousedown carries
    //     the same "button" parameter Click/Dblclick do.
    // PT: Nome/semântica do parâmetro confirmados do mesmo jeito que ClickInfoEventListener
    //     acima (Source/Core/Context.cpp:1538-1541 GenerateMouseEventParameters) -- Mousedown
    //     carrega o mesmo parâmetro "button" que Click/Dblclick.
    const int button = event.GetParameter<int>("button", 0);
    // EN: button==0 (left): native Click/Dblclick path already covers it, ignore here.
    //     button>2: unknown/exotic hardware button (RmlUi itself only names 0/1/2) -- no slot
    //     to record it into (pending_ has 3 entries), ignore rather than index OOB.
    // PT: button==0 (esquerdo): já coberto pelo caminho nativo Click/Dblclick, ignora aqui.
    //     button>2: botão de hardware exótico/desconhecido (o próprio RmlUi só nomeia 0/1/2) --
    //     sem slot para registrar (pending_ tem 3 entradas), ignora em vez de indexar fora dos
    //     limites.
    if (button <= 0 || button > 2) return;

    Rml::Element* el = event.GetTargetElement();
    while (el && el->GetId().empty() && el != doc_) el = el->GetParentNode();
    const char* id_str = (el && !el->GetId().empty()) ? el->GetId().c_str() : "";

    pending_[button].valid = true;
    pending_[button].id    = id_str;
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  PendingButtonDown* pending_;
  Rml::ElementDocument* doc_;
};

// EN: Mouseup half of the right/middle-click synthesis pair (AUD-PUB-4 gap closure, v0.5.0) --
//     the counterpart of ClickInfoButtonDownListener above. On a non-primary Mouseup, checks
//     whether Impl::pending_button_down[button] is armed (a matching Mousedown was seen since
//     the last consumed up/document load) and, if the up's own ancestor-walked id matches the
//     down's recorded id, fires click_info_cb ONCE with THIS event's own button/mouse_x/mouse_y
//     (mirrors RmlUi's own left-click semantics: Click's coordinates are the release point, not
//     the press point -- see ClickInfoEventListener's parameter-name comment above) and
//     double_click=false (RmlUi has no Dblclick for non-primary buttons -- see ClickInfo's
//     doc-comment "KNOWN LIMITATION"). The pending slot is consumed (valid=false) on EVERY
//     non-primary up regardless of whether the id matched, so a single Mousedown can arm at
//     most one Mouseup outcome (match-and-fire, or mismatch-and-drop) -- it never leaks into a
//     LATER, unrelated up.
// PT: Metade Mouseup do par de síntese de clique direito/meio (fechamento de gap AUD-PUB-4,
//     v0.5.0) -- a contraparte do ClickInfoButtonDownListener acima. Num Mouseup não-primário,
//     checa se Impl::pending_button_down[button] está armado (um Mousedown pareado foi visto
//     desde o último up consumido/load de documento) e, se o id obtido pela subida de ancestral
//     do próprio up bate com o id registrado do down, dispara click_info_cb UMA VEZ com o
//     próprio botão/mouse_x/mouse_y DESTE evento (espelha a semântica de clique esquerdo do
//     próprio RmlUi: as coordenadas do Click são o ponto de soltura, não o de pressão -- ver o
//     comentário de nome de parâmetro do ClickInfoEventListener acima) e double_click=false (o
//     RmlUi não tem Dblclick para botões não-primários -- ver "LIMITAÇÃO CONHECIDA" no
//     doc-comment de ClickInfo). O slot pendente é consumido (valid=false) em TODO up
//     não-primário independente do id ter batido, então um único Mousedown pode armar no máximo
//     um resultado de Mouseup (bate-e-dispara, ou não-bate-e-descarta) -- nunca vaza para um up
//     POSTERIOR e não relacionado.
class ClickInfoButtonUpListener : public Rml::EventListener {
public:
  // EN: `pending` -- same array pointer as ClickInfoButtonDownListener (both listener instances
  //     for one load() point at the SAME Impl::pending_button_down array, so a down recorded by
  //     one is visible to the other). `cb` points into Impl::click_info_cb, same raw-pointer-
  //     into-Impl-field pattern as ClickEventListener/ClickInfoEventListener above (see the
  //     PendingButtonDown type comment for why an `Impl*` itself cannot be named here).
  // PT: `pending` -- mesmo ponteiro de array do ClickInfoButtonDownListener (ambas as instâncias
  //     de listener de um load() apontam para o MESMO array Impl::pending_button_down, então um
  //     down registrado por uma é visível pela outra). `cb` aponta para dentro de
  //     Impl::click_info_cb, mesmo padrão de ponteiro-cru-para-campo-do-Impl de
  //     ClickEventListener/ClickInfoEventListener acima (ver o comentário do tipo
  //     PendingButtonDown pro motivo de um `Impl*` em si não poder ser nomeado aqui).
  ClickInfoButtonUpListener(PendingButtonDown* pending, std::function<void(const ClickInfo&)>* cb,
                             Rml::ElementDocument* doc)
      : pending_(pending), cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    const int button = event.GetParameter<int>("button", 0);
    if (button <= 0 || button > 2) return; // EN: same left/OOB exclusion as the down half.
                                            // PT: mesma exclusão esquerdo/OOB da metade down.

    PendingButtonDown& pending = pending_[button];
    if (!pending.valid) return; // EN: up with no matching down we tracked -- ignore.
                                 // PT: up sem down pareado rastreado -- ignora.
    // EN: Consume unconditionally BEFORE the id comparison below -- a down that does not lead
    //     to a matching up (drag-off) must not remain armed for some LATER, unrelated up on the
    //     same button.
    // PT: Consome incondicionalmente ANTES da comparação de id abaixo -- um down que não leva a
    //     um up pareado (arrastar-para-fora) não pode continuar armado para algum up POSTERIOR,
    //     não relacionado, no mesmo botão.
    pending.valid = false;

    if (!cb_ || !*cb_) return;

    Rml::Element* el = event.GetTargetElement();
    while (el && el->GetId().empty() && el != doc_) el = el->GetParentNode();
    const char* up_id = (el && !el->GetId().empty()) ? el->GetId().c_str() : "";

    if (pending.id != up_id) return; // EN: down and up landed on different elements -- no click.
                                      // PT: down e up caíram em elementos diferentes -- sem clique.

    ClickInfo info;
    info.id           = up_id;
    info.button        = button;
    info.x             = event.GetParameter<float>("mouse_x", 0.f);
    info.y             = event.GetParameter<float>("mouse_y", 0.f);
    info.double_click  = false; // EN: no native Dblclick for non-primary buttons.
                                 // PT: sem Dblclick nativo para botões não-primários.

    // EN: Copy the functor before invoking (AUD-TEC-3, same reentrancy guard as the other
    //     listeners in this file -- see ClickEventListener::ProcessEvent for the detailed
    //     comment).
    // PT: Copia o functor antes de invocar (AUD-TEC-3, mesma guarda de reentrância dos outros
    //     listeners deste arquivo -- ver ClickEventListener::ProcessEvent pro comentário
    //     detalhado).
    auto cb = *cb_;
    if (cb) cb(info);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  PendingButtonDown* pending_;
  std::function<void(const ClickInfo&)>* cb_;
  Rml::ElementDocument* doc_;
};

// EN: Bubble-phase "scroll" listener attached to each loaded document's root
//     (GLINTFX-SCROLL-1 follow-up, v0.6.0) -- same shape as ClickEventListener above (id-only,
//     ancestor-walk-to-nearest-id, reentrancy-safe local copy before invoke), but for
//     Rml::EventId::Scroll instead of Click. EventId::Scroll's target IS the element whose own
//     scroll offset changed (Source/Core/Element.cpp SetScrollTop/SetScrollLeft dispatch on
//     `this`, confirmed in the pinned RmlUi source) -- typically an overflow-y:auto container
//     that already carries an author-given id, but the ancestor walk is kept identical to
//     ClickEventListener's for consistency/robustness (e.g. a scrollable element without its own
//     id still resolves to its nearest ancestor id, "" if none up to doc_).
// PT: Listener de "scroll" em fase bubble anexado à raiz de cada documento carregado
//     (desdobramento do GLINTFX-SCROLL-1, v0.6.0) -- mesma forma do ClickEventListener acima
//     (só-id, subida de ancestral até o id mais próximo, cópia local segura contra reentrância
//     antes de invocar), mas para Rml::EventId::Scroll em vez de Click. O alvo do
//     EventId::Scroll É o elemento cujo próprio offset de rolagem mudou
//     (Source/Core/Element.cpp SetScrollTop/SetScrollLeft despacham em `this`, confirmado no
//     source pinado do RmlUi) -- tipicamente um container overflow-y:auto que já carrega um id
//     autorado, mas a subida de ancestral é mantida idêntica à do ClickEventListener por
//     consistência/robustez (ex.: um elemento rolável sem id próprio ainda resolve para o id do
//     ancestral mais próximo, "" se nenhum até doc_).
class ScrollEventListener : public Rml::EventListener {
public:
  ScrollEventListener(std::function<void(const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    Rml::Element* el = event.GetTargetElement();
    // EN: Same ancestor-walk-stops-at-doc_ rule as ClickEventListener::ProcessEvent above -- see
    //     the detailed comment there for why the walk must not overshoot into the context's
    //     internal root element.
    // PT: Mesma regra de subida-para-em-doc_ do ClickEventListener::ProcessEvent acima -- ver o
    //     comentário detalhado lá do motivo de a subida não poder ultrapassar pro elemento root
    //     interno do contexto.
    while (el && el->GetId().empty() && el != doc_) el = el->GetParentNode();
    const char* id_str = (el && !el->GetId().empty()) ? el->GetId().c_str() : "";
    // EN: Copy the functor before invoking (AUD-TEC-3 reentrancy guard, same reasoning as
    //     ClickEventListener::ProcessEvent above).
    // PT: Copia o functor antes de invocar (guarda de reentrância AUD-TEC-3, mesmo raciocínio de
    //     ClickEventListener::ProcessEvent acima).
    auto cb = *cb_;
    if (cb) cb(id_str);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*)>* cb_;
  Rml::ElementDocument* doc_;
};

Bootstrap::~Bootstrap() { shutdown(); }

bool Bootstrap::init(Rml::SystemInterface* system, RenderGl3& render, int w, int h) {
  if (impl_) return false; // EN: guard: already initialised. PT: guard: já inicializado.

  impl_ = new Impl();
  Rml::SetSystemInterface(system);
  Rml::SetRenderInterface(render.iface());
  // EN: Install our FileInterface BEFORE Rml::Initialise(). It is owned by Impl and lives
  //     until shutdown(). When base_url is empty (the default) its behaviour is identical to
  //     RmlUi's built-in default — no functional change for existing callers.
  // PT: Instala nosso FileInterface ANTES do Rml::Initialise(). É possuído pelo Impl e vive
  //     até shutdown(). Quando base_url está vazio (padrão), o comportamento é idêntico ao
  //     padrão embutido do RmlUi — sem mudança funcional para chamadores existentes.
  Rml::SetFileInterface(&impl_->file_iface);

  if (!Rml::Initialise()) {
    // EN: Initialise() failed — clear the global FileInterface pointer BEFORE deleting
    //     impl_, which owns file_iface. Without this, the RmlUi-global `file_interface`
    //     pointer would dangle after the delete (use-after-free on the next Open() call,
    //     which can happen in error-log paths inside RmlUi itself).
    //     SetFileInterface(nullptr) is safe here: the implementation is a single global
    //     assignment (Rml::Core.cpp), and RmlUi is not initialised, so no code path
    //     will dereference the pointer between this call and the delete.
    //     Note: CreateContext() failure is handled differently — that path calls
    //     Rml::Shutdown(), which internally sets file_interface=nullptr before returning,
    //     so no extra reset is needed there.
    // PT: Initialise() falhou — limpa o ponteiro global do FileInterface ANTES de deletar
    //     impl_, que é dono do file_iface. Sem isso, o ponteiro `file_interface` global do
    //     RmlUi ficaria pendente após o delete (use-after-free na próxima chamada a Open(),
    //     que pode ocorrer em caminhos de log de erro dentro do próprio RmlUi).
    //     SetFileInterface(nullptr) é seguro aqui: a implementação é uma única atribuição
    //     global (Rml::Core.cpp), e o RmlUi não está inicializado, então nenhum caminho de
    //     código desreferenciará o ponteiro entre esta chamada e o delete.
    //     Nota: a falha de CreateContext() é tratada de forma diferente — aquele caminho
    //     chama Rml::Shutdown(), que internamente define file_interface=nullptr antes de
    //     retornar, então nenhum reset extra é necessário lá.
    Rml::SetFileInterface(nullptr);
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  impl_->initialised = true;

  // EN: Construct + register the "polygon" decorator instancer -- MUST happen AFTER
  //     Rml::Initialise() returns true (see the long Impl comment above for why: its
  //     constructor needs the "number"/"color"/"angle" parser registry that Initialise() just
  //     populated) and BEFORE any load(), since decorators are resolved while parsing RCSS
  //     (v0.2.6, consumer-driven by GusWorld's hex slider node). Registered as "polygon" so
  //     "decorator: polygon(6, #5fd0ff);" resolves. Rml::Factory::RegisterDecoratorInstancer
  //     stores a raw, non-owning pointer -- see the Impl comment above for the lifetime rule
  //     that keeps the pointee valid until Rml::Shutdown() completes.
  // PT: Constrói + registra o instancer do decorator "polygon" -- DEVE acontecer APÓS
  //     Rml::Initialise() retornar true (ver o comentário longo do Impl acima pro motivo: o
  //     construtor dele precisa do registro de parsers "number"/"color"/"angle" que o
  //     Initialise() acabou de popular) e ANTES de qualquer load(), já que decorators são
  //     resolvidos ao parsear RCSS (v0.2.6, consumer-driven pelo nó hex slider do GusWorld).
  //     Registrado como "polygon" para que "decorator: polygon(6, #5fd0ff);" resolva.
  //     Rml::Factory::RegisterDecoratorInstancer guarda um ponteiro cru, não-dono -- ver o
  //     comentário do Impl acima pra regra de lifetime que mantém o apontado válido até o
  //     Rml::Shutdown() terminar.
  impl_->polygon_instancer = Rml::MakeUnique<glintfx::PolygonDecoratorInstancer>();
  Rml::Factory::RegisterDecoratorInstancer("polygon", impl_->polygon_instancer.get());

  // EN: GLINTFX-TINT-3 -- construct + register the "image-tint" decorator instancer, same
  //     ordering constraint as "polygon" immediately above (see impl_->tint_instancer's own doc
  //     comment). Registered as "image-tint" so "decorator: image-tint(base.png);" resolves --
  //     see ADR-0010 Decision (b) for why the function name is glintfx's own, not "image".
  // PT: GLINTFX-TINT-3 -- constrói + registra o instancer do decorator "image-tint", mesma
  //     restrição de ordenação de "polygon" logo acima (ver o próprio doc-comment de
  //     impl_->tint_instancer). Registrado como "image-tint" para que
  //     "decorator: image-tint(base.png);" resolva -- ver a Decisão (b) do ADR-0010 pro motivo
  //     do nome de função ser próprio da glintfx, não "image".
  impl_->tint_instancer = Rml::MakeUnique<glintfx::ImageTintDecoratorInstancer>();
  Rml::Factory::RegisterDecoratorInstancer("image-tint", impl_->tint_instancer.get());

  // EN: Parse the UA stylesheet once. It is never compiled directly (never attached
  //     alone to a document) so it stays reusable as a merge base for every load().
  // PT: Parseia a UA stylesheet uma vez. Nunca é compilada diretamente (nunca anexada
  //     sozinha a um documento), então permanece reutilizável como base de merge a cada load().
  {
    auto ua = Rml::MakeShared<Rml::StyleSheetContainer>();
    auto stream = Rml::MakeUnique<Rml::StreamMemory>(
        reinterpret_cast<const Rml::byte*>(kUaStylesheetRcss), std::strlen(kUaStylesheetRcss));
    stream->SetSourceURL("glintfx://ua.rcss");
    if (ua->LoadStyleSheetContainer(stream.get()))
      impl_->ua_style_sheet = std::move(ua);
    // EN: On parse failure ua_style_sheet stays null; load() then no-ops the UA merge
    //     (fail open -- a malformed embedded sheet must never block document loading).
    // PT: Em falha de parse ua_style_sheet fica nulo; load() então no-opa o merge da UA
    //     (fail open -- uma sheet embutida malformada nunca deve bloquear o carregamento).
  }

  impl_->ctx = Rml::CreateContext("main", Rml::Vector2i(w, h));
  if (!impl_->ctx) {
    Rml::Shutdown();
    impl_->initialised = false;
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  return true;
}

bool Bootstrap::load(const char* rml_path) {
  if (!impl_ || !impl_->ctx) return false;
  // EN: CRITICAL null-guard (input-hardening audit, v0.3.0). Rml::Context::LoadDocument takes a
  //     `const Rml::String&` (= std::string); a null `rml_path` would construct std::string from
  //     a null pointer -- undefined behaviour that in practice throws std::logic_error, and since
  //     glintfx is built -fno-exceptions-agnostic and the host (e.g. GusWorld) does not wrap
  //     load() in a try/catch, the uncaught exception calls std::terminate() -> SIGABRT, taking
  //     the ENTIRE host process down. A bad path from the caller must fail soft (return false),
  //     never abort the host. This mirrors the null-guards already present across the API
  //     (DataBinder::create/bind_*, Bootstrap::get_element_box, BaseUrlFileInterface::
  //     set_base_url) -- load() was the one entry point that lacked it.
  // PT: Null-guard CRÍTICO (auditoria de hardening de entrada, v0.3.0). Rml::Context::LoadDocument
  //     recebe `const Rml::String&` (= std::string); um `rml_path` nulo construiria std::string a
  //     partir de ponteiro nulo -- comportamento indefinido que na prática lança std::logic_error,
  //     e como o host (ex.: GusWorld) não envolve load() num try/catch, a exceção não capturada
  //     chama std::terminate() -> SIGABRT, derrubando o PROCESSO HOST INTEIRO. Um caminho ruim do
  //     chamador deve falhar suave (retornar false), nunca abortar o host. Espelha os null-guards
  //     já presentes na API (DataBinder::create/bind_*, Bootstrap::get_element_box,
  //     BaseUrlFileInterface::set_base_url) -- load() era o único ponto de entrada sem ele.
  if (!rml_path) return false;
  Rml::ElementDocument* doc = impl_->ctx->LoadDocument(rml_path);
  if (!doc) return false;

  // EN: BUG FIX (reported by GusWorld, 2026-07-04): load() used to overwrite impl_->doc
  //     with the NEW document without ever closing the PREVIOUS one. Since impl_->doc is
  //     the only reference glintfx held, every reload on the SAME Bootstrap/UiLayer/App
  //     (e.g. menu-screen navigation calling load(next_path) repeatedly) stacked one more
  //     live document into the Rml::Context forever: (1) a silent memory leak, one document
  //     per reload; (2) the previous document stayed Show()n and kept rendering UNDERNEATH
  //     the new one -- a "ghost document" bug, most visible with transparency/glow.
  //     Close the OLD document only AFTER the NEW one has loaded successfully (this line),
  //     not before LoadDocument() above -- decision: if the new load fails (returns above),
  //     the caller keeps whatever was on screen instead of being left with a blank
  //     Context (0 documents). A previous fail-early attempt (guard right before
  //     LoadDocument(), unconditional) was rejected for exactly that regression: it would
  //     leave the UiLayer/App with NO visible document until the NEXT successful load(),
  //     which is a worse UX than "briefly stale but visible".
  //     Rml::ElementDocument::Close() is equivalent to Rml::Context::UnloadDocument(doc)
  //     here (both exist; Close() needs no Context* and reads slightly more directly at
  //     the call site) -- destruction is DEFERRED to the next Rml::Context::Update() (see
  //     ElementDocument.h/Context.h in the pinned RmlUi source), which Engine::update()
  //     already calls once per frame in production, so no change to the frame loop is
  //     needed for the deferred teardown to actually happen.
  // PT: CORREÇÃO DE BUG (relatado pelo GusWorld, 2026-07-04): load() sobrescrevia
  //     impl_->doc com o documento NOVO sem nunca fechar o ANTERIOR. Como impl_->doc era a
  //     única referência que a glintfx mantinha, todo reload no MESMO Bootstrap/UiLayer/App
  //     (ex.: navegação entre telas de menu chamando load(proximo_path) repetidamente)
  //     empilhava mais um documento vivo no Rml::Context para sempre: (1) vazamento de
  //     memória silencioso, um documento por reload; (2) o documento anterior continuava
  //     Show()n e seguia renderizando POR BAIXO do novo -- bug de "documento fantasma",
  //     mais visível com transparência/glow.
  //     Fecha o documento ANTIGO só APÓS o NOVO ter carregado com sucesso (esta linha), não
  //     antes do LoadDocument() acima -- decisão: se o novo load falhar (retorna acima), o
  //     chamador mantém o que já estava na tela em vez de ficar com um Context em branco (0
  //     documentos). Uma tentativa anterior de fechar cedo (guard logo antes do
  //     LoadDocument(), incondicional) foi rejeitada por exatamente essa regressão: deixaria
  //     a UiLayer/App SEM nenhum documento visível até o PRÓXIMO load() bem-sucedido, o que
  //     é pior UX que "levemente desatualizado mas visível".
  //     Rml::ElementDocument::Close() equivale a Rml::Context::UnloadDocument(doc) aqui
  //     (ambos existem; Close() dispensa o Context* e lê um pouco mais direto no call site)
  //     -- a destruição é DIFERIDA até o próximo Rml::Context::Update() (ver
  //     ElementDocument.h/Context.h no source pinado do RmlUi), que o Engine::update() já
  //     chama uma vez por frame em produção, então nenhuma mudança no loop de frame é
  //     necessária para a destruição diferida realmente acontecer.
  if (impl_->doc && impl_->doc != doc) {
    impl_->doc->Close();
  }
  if (impl_->ua_style_sheet) {
    // EN: Merge UA base (low specificity) under the document's own sheet (merged ON TOP
    //     -- StyleSheet::MergeStyleSheet's specificity_offset accumulation resolves ties
    //     in favour of whoever is merged LAST). Works with or without the document's own
    //     <link>/<style>.
    //     NOTE: split into if/else (instead of a ternary) because StyleSheetContainer is
    //     NonCopyMoveable -- a ternary's common-type rule would require materialising a
    //     copy of the temporary default-constructed container, which does not compile.
    // PT: Mescla a base UA (baixa especificidade) sob a sheet própria do documento
    //     (mesclada POR CIMA -- o acúmulo de specificity_offset em
    //     StyleSheet::MergeStyleSheet resolve empates a favor de quem é mesclado POR
    //     ÚLTIMO). Funciona com ou sem <link>/<style> próprio do documento.
    //     NOTA: dividido em if/else (em vez de ternário) porque StyleSheetContainer é
    //     NonCopyMoveable -- a regra de tipo-comum do ternário exigiria materializar uma
    //     cópia do container default-construído temporário, o que não compila.
    const Rml::StyleSheetContainer* existing = doc->GetStyleSheetContainer();
    Rml::SharedPtr<Rml::StyleSheetContainer> combined;
    if (existing) {
      combined = impl_->ua_style_sheet->CombineStyleSheetContainer(*existing);
    } else {
      const Rml::StyleSheetContainer empty;
      combined = impl_->ua_style_sheet->CombineStyleSheetContainer(empty);
    }
    doc->SetStyleSheetContainer(std::move(combined));
  }
  doc->Show();
  doc->AddEventListener(Rml::EventId::Click,
                         new ClickEventListener(&impl_->click_cb, doc), false);
  // EN: AUD-PUB-4 (v0.5.0) -- two separate ClickInfoEventListener instances, one per EventId
  //     (see the class-level comment on ClickInfoEventListener above for why they must NOT be
  //     the same instance).
  // PT: AUD-PUB-4 (v0.5.0) -- duas instâncias separadas de ClickInfoEventListener, uma por
  //     EventId (ver o comentário de nível de classe em ClickInfoEventListener acima do motivo
  //     de NÃO poderem ser a mesma instância).
  doc->AddEventListener(Rml::EventId::Click,
                         new ClickInfoEventListener(&impl_->click_info_cb, doc, false), false);
  doc->AddEventListener(Rml::EventId::Dblclick,
                         new ClickInfoEventListener(&impl_->click_info_cb, doc, true), false);
  // EN: AUD-PUB-4 gap closure (v0.5.0) -- right/middle click synthesis pair, registered on the
  //     NEW document like every other listener above. Reset pending_button_down FIRST (before
  //     registering), not after -- a Mousedown on the OLD document with no matching Mouseup
  //     before this reload must not survive into the new document's lifetime and spuriously
  //     match an unrelated element sharing the same id (see the field comment on
  //     Impl::pending_button_down for the full rationale).
  // PT: Fechamento de gap AUD-PUB-4 (v0.5.0) -- par de síntese de clique direito/meio,
  //     registrado no documento NOVO como todo outro listener acima. Reseta
  //     pending_button_down PRIMEIRO (antes de registrar), não depois -- um Mousedown no
  //     documento ANTIGO sem Mouseup pareado antes deste reload não pode sobreviver ao lifetime
  //     do documento novo e bater acidentalmente com um elemento não relacionado que compartilhe
  //     o mesmo id (ver o comentário de campo de Impl::pending_button_down pra racional
  //     completa).
  for (auto& p : impl_->pending_button_down) { p.valid = false; p.id.clear(); }
  doc->AddEventListener(Rml::EventId::Mousedown,
                         new ClickInfoButtonDownListener(impl_->pending_button_down, doc), false);
  doc->AddEventListener(Rml::EventId::Mouseup,
                         new ClickInfoButtonUpListener(impl_->pending_button_down,
                                                        &impl_->click_info_cb, doc), false);
  // EN: GLINTFX-SCROLL-1 follow-up (v0.6.0) -- same bubble-phase-listener-on-doc-root pattern as
  //     every listener above; see Bootstrap::set_scroll_callback for the full contract.
  // PT: Desdobramento do GLINTFX-SCROLL-1 (v0.6.0) -- mesmo padrão de listener-em-fase-bubble-na-
  //     raiz-do-documento de todo listener acima; ver Bootstrap::set_scroll_callback para o
  //     contrato completo.
  doc->AddEventListener(Rml::EventId::Scroll,
                         new ScrollEventListener(&impl_->scroll_cb, doc), false);
  impl_->doc = doc;
  return true;
}

Rml::Context* Bootstrap::context() { return impl_ ? impl_->ctx : nullptr; }

void Bootstrap::set_click_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->click_cb = std::move(cb);
}

void Bootstrap::set_click_info_callback(std::function<void(const ClickInfo&)> cb) {
  if (impl_) impl_->click_info_cb = std::move(cb);
}

void Bootstrap::set_scroll_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->scroll_cb = std::move(cb);
}

bool Bootstrap::get_element_box(const char* id, float& x, float& y, float& w, float& h) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  const Rml::Vector2f offset = el->GetAbsoluteOffset(Rml::BoxArea::Border);
  const Rml::Vector2f size   = el->GetBox().GetSize(Rml::BoxArea::Border);
  x = offset.x; y = offset.y; w = size.x; h = size.y;
  return true;
}

bool Bootstrap::scroll_element_into_view(const char* id, bool align_with_top) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  el->ScrollIntoView(align_with_top);
  return true;
}

bool Bootstrap::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  out_scroll_top = el->GetScrollTop();
  return true;
}

bool Bootstrap::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  out_scroll_height = el->GetScrollHeight();
  return true;
}

bool Bootstrap::get_element_client_height(const char* id, float& out_client_height) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  out_client_height = el->GetClientHeight();
  return true;
}

bool Bootstrap::set_element_scroll_top(const char* id, float scroll_top) const {
  // EN: Guard (AUD-TEC-5): reject empty id "" too, not just null -- an author/host that
  //     passes "" (e.g. a computed id that resolved empty) should fail-high the same way
  //     null does, not silently fall through to GetElementById("") lookups.
  // PT: Guard (AUD-TEC-5): rejeita id vazio "" também, não só nulo -- um autor/host que
  //     passe "" (ex.: id computado que resolveu vazio) deve falhar fail-high da mesma
  //     forma que nulo, não cair silenciosamente em buscas GetElementById("").
  if (!impl_ || !impl_->doc || !id || !*id) return false;
  // EN: Reject non-finite input BEFORE touching RmlUi -- see the doc-comment in bootstrap.hpp
  //     for why Element::SetScrollTop offers no guard of its own.
  // PT: Rejeita entrada não-finita ANTES de tocar o RmlUi -- ver o doc-comment em bootstrap.hpp
  //     pro motivo de Element::SetScrollTop não oferecer guard próprio.
  if (!std::isfinite(scroll_top)) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  el->SetScrollTop(scroll_top);
  return true;
}

void Bootstrap::set_asset_base_url(const char* url) {
  // EN: Safe before init() — file_iface is a value member of Impl, but Impl is allocated
  //     in init(); calling this before init() is a no-op. After init() the FileInterface
  //     is already installed: updating base_url_ here affects all subsequent Open() calls.
  // PT: Seguro antes de init() — file_iface é membro por valor do Impl, mas o Impl é alocado
  //     em init(); chamar antes de init() é no-op. Após init() o FileInterface já está instalado:
  //     atualizar base_url_ aqui afeta todas as chamadas Open() subsequentes.
  if (impl_) impl_->file_iface.set_base_url(url);
}

void Bootstrap::shutdown() {
  if (!impl_) return;
  if (impl_->initialised) {
    // EN: Rml::Shutdown() releases all contexts, documents and font resources.
    //     Caller is responsible for deleting the SystemInterface after this point.
    // PT: Rml::Shutdown() libera todos os contextos, documentos e recursos de fonte.
    //     O chamador é responsável por deletar o SystemInterface após este ponto.
    Rml::Shutdown();
    impl_->initialised = false;
  }
  // EN: We do NOT delete the SystemInterface here — caller owns it.
  // PT: NÃO deletamos o SystemInterface aqui — o chamador é dono.
  delete impl_;
  impl_ = nullptr;
}

} // namespace glintfx
