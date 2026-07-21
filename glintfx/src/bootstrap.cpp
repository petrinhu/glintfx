// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap implementation — wires caller-supplied SystemInterface + RenderInterface
//     into RmlUi. The caller is the OWNER of the SystemInterface lifetime.
// PT: Implementação do Bootstrap — conecta SystemInterface (fornecida pelo chamador) +
//     RenderInterface ao RmlUi. O chamador é DONO do lifetime do SystemInterface.
// Copyright (c) 2026 Petrus Silva Costa
#include "bootstrap.hpp"
#include "render_gl3.hpp"
#include "base_url_file_interface.hpp"
#include "ua_stylesheet.hpp"
#include <glintfx/config.hpp>
// EN: FX-CARVE-1 -- the 3 decorator instancer headers (polygon/image-tint/ripple) are only
//     pulled in when the fx module is compiled; with GLINTFX_MODULE_FX=OFF, none of
//     PolygonDecoratorInstancer/ImageTintDecoratorInstancer/RippleDecoratorInstancer are named
//     anywhere in this file (the Impl members + init()'s construct+register block below share
//     this same gate) -- an unknown "decorator: polygon(...)"/"image-tint(...)"/"ripple(...)"
//     in RCSS is then just ignored by RmlUi (fail-high, same idiom every other unrecognised
//     RCSS construct already gets).
// PT: FX-CARVE-1 -- os 3 headers de instancer de decorator (polygon/image-tint/ripple) só são
//     puxados quando o módulo fx é compilado; com GLINTFX_MODULE_FX=OFF, nenhum de
//     PolygonDecoratorInstancer/ImageTintDecoratorInstancer/RippleDecoratorInstancer é nomeado
//     em lugar nenhum deste arquivo (os membros do Impl + o bloco de construção+registro do
//     init() abaixo compartilham este mesmo gate) -- um "decorator: polygon(...)"/
//     "image-tint(...)"/"ripple(...)" desconhecido no RCSS aí é só ignorado pelo RmlUi
//     (fail-high, o mesmo idioma que toda outra construção RCSS não-reconhecida já recebe).
#if GLINTFX_MODULE_FX
#include "decorator_polygon.hpp"
#include "decorator_image_tint.hpp"
#include "decorator_ripple.hpp"
#endif
#if GLINTFX_OWN_FONT_ENGINE
// EN: L1.19-FONTENG (FT-F3) A/B gate -- only pulled in (and only pulls in Layer 0's
//     include/core/sfnt.h + raster.h, transitively) when GLINTFX_OWN_FONT_ENGINE=ON. See
//     font_engine_own.hpp's own file header for the full design/clean-room writeup.
// PT: Gate A/B do L1.19-FONTENG (FT-F3) -- só puxado (e só puxa transitivamente o
//     include/core/sfnt.h + raster.h da Camada 0) quando GLINTFX_OWN_FONT_ENGINE=ON. Ver o
//     próprio cabeçalho de arquivo de font_engine_own.hpp pro relato completo de
//     design/clean-room.
#include "font_engine_own.hpp"
#endif
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
  // EN: FX-CARVE-1 -- the 3 decorator instancer members below (polygon/image-tint/ripple)
  //     only exist when the fx module is compiled; each instancer's own doc comment further
  //     down explains its individual lifetime rule (allocate-after-Rml::Initialise(),
  //     outlive-Rml::Shutdown()) -- this #if only decides whether the 3 members exist at
  //     all, not their lifetime discipline once they do.
  // PT: FX-CARVE-1 -- os 3 membros de instancer de decorator abaixo (polygon/image-tint/
  //     ripple) só existem quando o módulo fx é compilado; o próprio doc-comment de cada
  //     instancer mais abaixo explica a regra de lifetime individual dele (alocar-após-
  //     Rml::Initialise(), sobreviver-ao-Rml::Shutdown()) -- este #if só decide se os 3
  //     membros existem, não a disciplina de lifetime deles quando existem.
#if GLINTFX_MODULE_FX
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
  // EN: L1.22-WAVE -- "ripple([<max-radius>])" decorator instancer. Same allocate-after-
  //     Rml::Initialise()/outlives-Rml::Shutdown() lifetime discipline as polygon_instancer/
  //     tint_instancer immediately above (see polygon_instancer's long doc comment for the full
  //     ordering rationale, which applies identically here). Does NOT carry any L1.22-CAPTURE
  //     gate counter (a prior revision, commit 647350f, did -- removed as part of the cold-start
  //     fix; see src/fx/effects_gl3.cpp's Gl3FxHook::arm_backdrop_capture/EnsureBackdropCaptured
  //     doc comment (FX-CARVE-1) and decorator_ripple.hpp's own doc comment for why the gate no
  //     longer needs one).
  // PT: L1.22-WAVE -- instancer do decorator "ripple([<raio-max>])". Mesma disciplina de
  //     lifetime alocar-após-Rml::Initialise()/sobrevive-ao-Rml::Shutdown() de
  //     polygon_instancer/tint_instancer logo acima (ver o doc-comment longo de
  //     polygon_instancer pra racional completa de ordenação, que se aplica identicamente
  //     aqui). NÃO carrega nenhum contador de gate do L1.22-CAPTURE (uma revisão anterior,
  //     commit 647350f, carregava -- removido como parte do fix de cold-start; ver o
  //     doc-comment de Gl3FxHook::arm_backdrop_capture/EnsureBackdropCaptured de
  //     src/fx/effects_gl3.cpp (FX-CARVE-1) e o próprio doc-comment de decorator_ripple.hpp pro
  //     motivo do gate não precisar mais de um).
  Rml::UniquePtr<glintfx::RippleDecoratorInstancer> ripple_instancer;
#endif
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

  // EN: L1.15-FORMEV -- form/DOM event callbacks (design doc
  //     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Shape B). See the
  //     doc-comments on Bootstrap::set_change_callback/set_submit_callback/set_focus_callback/
  //     set_blur_callback/set_hover_callback in bootstrap.hpp for the full per-event contract.
  //     All empty by default -- a null/empty std::function is a safe listener no-op.
  // PT: L1.15-FORMEV -- callbacks de evento de formulário/DOM (doc de design
  //     docs/superpowers/specs/2026-07-09-glintfx-form-events-design.md, Formato B). Ver os
  //     doc-comments de Bootstrap::set_change_callback/set_submit_callback/set_focus_callback/
  //     set_blur_callback/set_hover_callback em bootstrap.hpp para o contrato completo por
  //     evento. Todos vazios por padrão -- um std::function nulo/vazio é no-op seguro no listener.
  std::function<void(const char*, const char*)> change_cb;
  std::function<void(const char*)> submit_cb;
  std::function<void(const char*)> focus_cb;
  std::function<void(const char*)> blur_cb;
  std::function<void(const char*, bool)> hover_cb;

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

#if GLINTFX_OWN_FONT_ENGINE
  // EN: L1.19-FONTENG (FT-F3) -- glintfx's own SOV-SFNT/SOV-RAST font engine (A/B gate,
  //     GLINTFX_OWN_FONT_ENGINE=ON). Same by-value, constructed-eagerly-before-Initialise()
  //     lifetime discipline as file_iface above (NOT the allocate-after-Initialise() pattern
  //     polygon_instancer/tint_instancer use -- FontEngineOwn's constructor touches no
  //     StyleSheetSpecification/Factory registry, it is completely inert until RmlUi actually
  //     calls LoadFontFace/GetFontFaceHandle on it, so there is no ordering hazard here).
  //     Registered via Rml::SetFontEngineInterface(&font_engine_own) in init() BEFORE
  //     Rml::Initialise() (see below) -- must outlive Rml::Shutdown() for the identical reason
  //     file_iface must (RmlUi keeps the raw pointer for the whole session); satisfied the same
  //     way (impl_ is only ever deleted AFTER Rml::Shutdown() completes).
  // PT: L1.19-FONTENG (FT-F3) -- motor de fonte próprio do glintfx, SOV-SFNT/SOV-RAST (gate
  //     A/B, GLINTFX_OWN_FONT_ENGINE=ON). Mesma disciplina de lifetime por-valor,
  //     construído-avidamente-antes-do-Initialise() do file_iface acima (NÃO o padrão
  //     alocar-após-Initialise() que polygon_instancer/tint_instancer usam -- o construtor do
  //     FontEngineOwn não toca nenhum registro de StyleSheetSpecification/Factory, é
  //     completamente inerte até o RmlUi de fato chamar LoadFontFace/GetFontFaceHandle nele,
  //     então não há risco de ordenação aqui). Registrado via
  //     Rml::SetFontEngineInterface(&font_engine_own) em init() ANTES do Rml::Initialise() (ver
  //     abaixo) -- precisa sobreviver ao Rml::Shutdown() pelo motivo idêntico do file_iface
  //     (o RmlUi guarda o ponteiro cru pela sessão inteira); satisfeito do mesmo jeito (impl_
  //     só é deletado APÓS o Rml::Shutdown() terminar).
  glintfx::FontEngineOwn font_engine_own;
#endif
};

namespace {

// EN: AUD-L1-QUALITY Item A -- shared ancestor-id walk. Starting at `el`, climbs
//     GetParentNode() until it finds the first element with a non-empty id, and returns that
//     id (`""` if none found). This same text was previously duplicated verbatim 10x across
//     the listener classes below; the loop and its stop condition are now defined ONCE here.
//
//     THE INVARIANT (do not touch without re-reading this comment in full): the walk stops
//     AT `doc` and never asks GetParentNode() past it. Bug found in Step 3 (v0.2.5): a plain
//     "walk to the first non-empty id" loop with no stop condition overshoots the document --
//     Context.cpp:65 does `root->SetId(name)` on the CONTEXT's internal root element (the one
//     above every ElementDocument, named after the context -- "main" in this codebase), so an
//     author-authored subtree with NO id anywhere would silently report that internal "main"
//     id instead of "". Stopping the walk AT `doc` ensures only ids the RML AUTHOR could have
//     written are ever reported; the document's own id (usually unset) is still checked once,
//     inclusively (`el != doc` is the loop's continuation condition, not its stop-and-exclude
//     condition -- when `el == doc` the loop body does not run again, but `doc`'s own
//     (typically empty) id is still read by the ternary below, same as every other element).
//     Removing or weakening `el != doc` reintroduces the overshoot bug in EVERY call site at
//     once -- this is exactly the "did the guard's sibling get the fix too?" dominó class the
//     AUD-L1-QUALITY finding calls out.
// PT: AUD-L1-QUALITY Item A -- subida de ancestral por id, compartilhada. Partindo de `el`,
//     sobe por GetParentNode() até achar o primeiro elemento com id não-vazio, e retorna esse
//     id (`""` se nenhum for achado). Este mesmo texto estava duplicado ao pé da letra 10x
//     entre as classes de listener abaixo; o laço e sua condição de parada agora são
//     definidos UMA VEZ aqui.
//
//     O INVARIANTE (não mexer sem reler este comentário por inteiro): a subida para EM `doc`
//     e nunca chama GetParentNode() além dele. Bug encontrado no Step 3 (v0.2.5): um loop
//     simples "sobe até o 1º id não-vazio" sem condição de parada ultrapassa o documento --
//     Context.cpp:65 faz `root->SetId(name)` no elemento root INTERNO do contexto (o que fica
//     acima de todo ElementDocument, nomeado com o nome do contexto -- "main" neste código),
//     então uma subárvore autorada SEM id em lugar nenhum reportaria silenciosamente esse id
//     interno "main" em vez de "". Parar a subida EM `doc` garante que só ids que o AUTOR do
//     RML poderia ter escrito sejam reportados; o id do próprio documento (normalmente não
//     definido) ainda é checado uma vez, de forma inclusiva (`el != doc` é a condição de
//     CONTINUAÇÃO do laço, não uma condição de parar-e-excluir -- quando `el == doc` o corpo
//     do laço não roda de novo, mas o id (tipicamente vazio) do próprio `doc` ainda é lido
//     pelo ternário abaixo, igual a qualquer outro elemento). Remover ou enfraquecer
//     `el != doc` reintroduz o bug de overshoot em TODO sítio de chamada de uma vez -- é
//     exatamente a classe dominó "o irmão da guarda levou o fix também?" que o achado
//     AUD-L1-QUALITY aponta.
const char* AncestorIdOf(Rml::Element* el, Rml::ElementDocument* doc) {
  while (el && el->GetId().empty() && el != doc) el = el->GetParentNode();
  return (el && !el->GetId().empty()) ? el->GetId().c_str() : "";
}

} // namespace

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
    // EN: AncestorIdOf() (defined above) walks to the nearest ancestor id, stopping AT doc_ --
    //     see its doc-comment for the overshoot-bug invariant (Step 3, v0.2.5) it preserves.
    // PT: AncestorIdOf() (definida acima) sobe até o id ancestral mais próximo, parando EM
    //     doc_ -- ver o doc-comment dela pro invariante do bug de overshoot (Step 3, v0.2.5)
    //     que ela preserva.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
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
    // EN: Same AncestorIdOf() as ClickEventListener::ProcessEvent above (id="" if no ancestor
    //     up to doc_ has one) -- see its doc-comment for why the walk stops AT doc_ instead of
    //     overshooting into the context's internal root element.
    // PT: Mesma AncestorIdOf() do ClickEventListener::ProcessEvent acima (id="" se nenhum
    //     ancestral até doc_ tiver um) -- ver o doc-comment dela do motivo de a subida parar EM
    //     doc_ em vez de ultrapassar pro elemento root interno do contexto.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);

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

    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);

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

    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* up_id = AncestorIdOf(event.GetTargetElement(), doc_);

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
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above -- see its
    //     doc-comment for why the walk must not overshoot into the context's internal root
    //     element.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima -- ver o
    //     doc-comment dela do motivo de a subida não poder ultrapassar pro elemento root
    //     interno do contexto.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
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

// EN: Bubble-phase "change" listener attached to each loaded document's root (L1.15-FORMEV).
//     Same id-resolution/reentrancy shape as ClickEventListener/ScrollEventListener above, plus
//     RmlUi's own "value" event parameter forwarded verbatim -- see
//     Bootstrap::set_change_callback's doc-comment for the full per-control-kind payload
//     contract (already normalised by RmlUi itself, confirmed against the pinned source).
// PT: Listener de "change" em fase bubble anexado à raiz de cada documento carregado
//     (L1.15-FORMEV). Mesma forma de resolução de id/reentrância de ClickEventListener/
//     ScrollEventListener acima, mais o próprio parâmetro de evento "value" do RmlUi
//     encaminhado verbatim -- ver o doc-comment de Bootstrap::set_change_callback para o
//     contrato completo de payload por tipo de controle (já normalizado pelo próprio RmlUi,
//     confirmado contra o source pinado).
class ChangeEventListener : public Rml::EventListener {
public:
  ChangeEventListener(std::function<void(const char*, const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
    // EN: Local Rml::String keeps the "value" parameter's storage alive for the duration of
    //     this call (event.GetParameter<Rml::String> returns BY VALUE) -- c_str() on a
    //     temporary would dangle the instant the full-expression ends; binding to a named
    //     local extends its lifetime to the end of this scope, past both cb_ invocations below.
    // PT: A Rml::String local mantém o storage do parâmetro "value" vivo pela duração desta
    //     chamada (event.GetParameter<Rml::String> retorna POR VALOR) -- c_str() num temporário
    //     penduraria no instante em que a expressão completa termina; vincular a uma local
    //     nomeada estende o lifetime até o fim deste escopo, além das invocações de cb_ abaixo.
    const Rml::String value = event.GetParameter<Rml::String>("value", "");
    // EN: Copy the functor before invoking (AUD-TEC-3), same reentrancy guard as every other
    //     listener in this file.
    // PT: Copia o functor antes de invocar (AUD-TEC-3), mesma guarda de reentrância de todo
    //     outro listener deste arquivo.
    auto cb = *cb_;
    if (cb) cb(id_str, value.c_str());
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*, const char*)>* cb_;
  Rml::ElementDocument* doc_;
};

// EN: Bubble-phase "submit" listener attached to each loaded document's root (L1.15-FORMEV).
//     The dispatch TARGET of Rml::EventId::Submit IS the <form> element itself
//     (Rml::ElementForm::Submit() dispatches on `this`), so the ancestor walk below typically
//     resolves the form's OWN id immediately -- id-only payload, see
//     Bootstrap::set_submit_callback's doc-comment for the v1 scope decision (no name/value
//     Dictionary forwarded).
// PT: Listener de "submit" em fase bubble anexado à raiz de cada documento carregado
//     (L1.15-FORMEV). O ALVO de despacho de Rml::EventId::Submit É o próprio elemento <form>
//     (Rml::ElementForm::Submit() despacha em `this`), então a subida de ancestral abaixo
//     tipicamente resolve o PRÓPRIO id do form de imediato -- payload só-id, ver o doc-comment
//     de Bootstrap::set_submit_callback para a decisão de escopo da v1 (nenhum Dictionary
//     nome/valor encaminhado).
class SubmitEventListener : public Rml::EventListener {
public:
  SubmitEventListener(std::function<void(const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
    auto cb = *cb_;
    if (cb) cb(id_str);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*)>* cb_;
  Rml::ElementDocument* doc_;
};

// EN: CAPTURE-phase "focus" listener attached to each loaded document's root (L1.15-FORMEV).
//     Registered with in_capture_phase=TRUE, unlike every bubble-phase listener above --
//     Rml::EventId::Focus does NOT bubble (EventSpecification.cpp: bubbles=false), so a
//     bubble-phase document-root listener would never see it. RmlUi's capture phase always
//     descends root->target regardless of the `bubbles` flag (confirmed reading
//     EventDispatcher::DispatchEvent: `phases_to_execute = Capture | Target | (bubbles ?
//     Bubble : 0)`), so this listener still reaches every focus target despite the
//     document-root attachment point. See Bootstrap::set_focus_callback's doc-comment for the
//     full empirically-verified recursion/dedup analysis.
// PT: Listener de "focus" em fase de CAPTURA anexado à raiz de cada documento carregado
//     (L1.15-FORMEV). Registrado com in_capture_phase=TRUE, diferente de todo listener de fase
//     bubble acima -- Rml::EventId::Focus NÃO borbulha (EventSpecification.cpp: bubbles=false),
//     então um listener de fase bubble na raiz do documento nunca o veria. A fase de captura do
//     RmlUi sempre desce raiz->alvo independente da flag `bubbles` (confirmado lendo
//     EventDispatcher::DispatchEvent: `phases_to_execute = Capture | Target | (bubbles ?
//     Bubble : 0)`), então este listener ainda alcança todo alvo de foco apesar do ponto de
//     anexação na raiz do documento. Ver o doc-comment de Bootstrap::set_focus_callback para a
//     análise completa de recursão/dedup verificada empiricamente.
class FocusEventListener : public Rml::EventListener {
public:
  FocusEventListener(std::function<void(const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
    auto cb = *cb_;
    if (cb) cb(id_str);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*)>* cb_;
  Rml::ElementDocument* doc_;
};

// EN: CAPTURE-phase "blur" listener attached to each loaded document's root (L1.15-FORMEV).
//     Same capture-phase rationale as FocusEventListener above (Rml::EventId::Blur also does
//     NOT bubble) -- a separate class (not a shared instance with a bool flag) because it
//     targets a DIFFERENT Impl field (blur_cb vs. focus_cb), mirroring
//     ClickEventListener/ScrollEventListener's "one class, one cb pointer" idiom rather than
//     ClickInfoEventListener's "one class, two instances differing by a bool" idiom (that
//     shape exists there because both instances share ONE cb; here focus and blur are
//     genuinely separate callbacks).
// PT: Listener de "blur" em fase de CAPTURA anexado à raiz de cada documento carregado
//     (L1.15-FORMEV). Mesma racional de fase-captura do FocusEventListener acima
//     (Rml::EventId::Blur também NÃO borbulha) -- uma classe separada (não uma instância
//     compartilhada com um flag bool) porque mira um campo DIFERENTE do Impl (blur_cb vs.
//     focus_cb), espelhando o idioma "uma classe, um ponteiro de cb" de
//     ClickEventListener/ScrollEventListener em vez do idioma "uma classe, duas instâncias
//     diferindo por um bool" do ClickInfoEventListener (aquela forma existe lá porque as duas
//     instâncias compartilham UM cb; aqui focus e blur são callbacks genuinamente separados).
class BlurEventListener : public Rml::EventListener {
public:
  BlurEventListener(std::function<void(const char*)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above.
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima.
    const char* id_str = AncestorIdOf(event.GetTargetElement(), doc_);
    auto cb = *cb_;
    if (cb) cb(id_str);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*)>* cb_;
  Rml::ElementDocument* doc_;
};

// EN: Bubble-phase "hover" listener attached to each loaded document's root (L1.15-FORMEV),
//     listening ONLY to Rml::EventId::Mouseover -- Mouseout is deliberately NOT separately
//     listened to (see Bootstrap::set_hover_callback's doc-comment for the full dedup-machine
//     rationale). Owns `current_hover_id_` as an INSTANCE member (not a Bootstrap::Impl field):
//     a fresh listener instance is created per load() (same idiom as every listener above), so
//     the dedup state is automatically reset to "" on every document reload with no extra
//     bookkeeping.
//
//     CRITICAL correctness detail (found empirically, form_events_sanity.cpp F5): RmlUi's own
//     Mouseover, like Focus/Blur, is NOT a single dispatch to the deepest hovered element that
//     then bubbles -- Context::UpdateHoverChain calls the SAME Context::SendEvents helper
//     Context::OnFocusChange uses (`std::set_difference` over the old/new hover ANCESTOR
//     CHAINS, `element->DispatchEvent(Mouseover, ...)` called ONCE PER newly-entered ancestor).
//     A single mouse move that enters several new ancestors at once (e.g. the very first hover
//     of the whole session, or a jump across a previously-unhovered subtree) therefore fires
//     THIS listener multiple times in a row, each with a DIFFERENT `event.GetTargetElement()`
//     (one specific ancestor), in an UNSPECIFIED order (ElementSet iteration order, not
//     leaf-to-root) -- resolving the id from `event.GetTargetElement()` directly would make the
//     dedup outcome order-dependent and non-deterministic across runs. The fix: resolve the
//     ancestor walk from `Context::GetHoverElement()` (via the target element's own
//     `GetContext()`) INSTEAD OF `event.GetTargetElement()`. Confirmed by reading
//     Context::UpdateHoverChain: `hover = mouse_active ? GetElementAtPoint(position) :
//     nullptr;` is assigned BEFORE the new_hover_chain is built and BEFORE SendEvents dispatches
//     ANY Mouseover/Mouseout -- so GetHoverElement() already reflects the FINAL, deepest hover
//     target throughout the entire fan-out dispatch sequence, regardless of which particular
//     ancestor a given dispatch's `event.GetTargetElement()` happens to be. Every fan-out
//     dispatch within one logical mouse move therefore resolves the IDENTICAL `rid`, so the
//     dedup below collapses them to AT MOST one real transition, deterministically -- this is
//     what actually makes the "hover-a via its span vs. via its own background" test case
//     ("cruzar filhos com id do mesmo container") stable rather than order-dependent. (This
//     trick is NOT available for Focus/Blur: Context::OnFocusChange assigns `focus = new_focus`
//     only AFTER both its Blur and Focus SendEvents passes complete, so GetFocusElement() is
//     stale -- still the OLD value -- for the entire duration of a Focus/Blur fan-out dispatch;
//     see set_focus_callback's doc-comment for how that asymmetry is handled instead.)
// PT: Listener de "hover" em fase bubble anexado à raiz de cada documento carregado
//     (L1.15-FORMEV), escutando SÓ Rml::EventId::Mouseover -- Mouseout deliberadamente NÃO é
//     escutado separadamente (ver o doc-comment de Bootstrap::set_hover_callback para a
//     racional completa da máquina de dedup). Possui `current_hover_id_` como membro de
//     INSTÂNCIA (não um campo de Bootstrap::Impl): uma instância de listener nova é criada por
//     load() (mesmo idioma de todo listener acima), então o estado de dedup é automaticamente
//     resetado para "" a cada reload de documento, sem contabilidade extra.
//
//     Detalhe CRÍTICO de correção (achado empiricamente, form_events_sanity.cpp F5): o próprio
//     Mouseover do RmlUi, igual a Focus/Blur, NÃO é um único despacho pro elemento mais fundo em
//     hover que depois borbulha -- Context::UpdateHoverChain chama o MESMO helper
//     Context::SendEvents que Context::OnFocusChange usa (`std::set_difference` sobre as CADEIAS
//     DE ANCESTRAIS de hover antiga/nova, `element->DispatchEvent(Mouseover, ...)` chamado UMA
//     VEZ POR ancestral recém-entrado). Um único movimento de mouse que entra em vários
//     ancestrais novos de uma vez (ex.: o primeiríssimo hover da sessão inteira, ou um salto
//     através de uma subárvore nunca antes em hover) portanto dispara ESTE listener várias vezes
//     seguidas, cada vez com um `event.GetTargetElement()` DIFERENTE (um ancestral específico),
//     numa ordem NÃO ESPECIFICADA (ordem de iteração do ElementSet, não folha-pra-raiz) --
//     resolver o id a partir de `event.GetTargetElement()` diretamente tornaria o resultado do
//     dedup dependente de ordem e não-determinístico entre execuções. O conserto: resolver a
//     subida de ancestral a partir de `Context::GetHoverElement()` (via o próprio
//     `GetContext()` do elemento-alvo) EM VEZ DE `event.GetTargetElement()`. Confirmado lendo
//     Context::UpdateHoverChain: `hover = mouse_active ? GetElementAtPoint(position) :
//     nullptr;` é atribuído ANTES do new_hover_chain ser construído e ANTES do SendEvents
//     despachar QUALQUER Mouseover/Mouseout -- então GetHoverElement() já reflete o alvo de
//     hover FINAL, mais fundo, durante toda a sequência de despacho em fan-out, independente de
//     qual ancestral específico o `event.GetTargetElement()` de um dado despacho seja. Todo
//     despacho de fan-out dentro de um único movimento lógico de mouse portanto resolve o MESMO
//     `rid`, então o dedup abaixo colapsa eles pra NO MÁXIMO uma transição real,
//     deterministicamente -- é isso que de fato torna o caso de teste "hover-a via seu span vs.
//     via seu próprio background" ("cruzar filhos com id do mesmo container") estável em vez de
//     dependente de ordem. (Esse truque NÃO está disponível pra Focus/Blur: Context::
//     OnFocusChange atribui `focus = new_focus` só APÓS as duas passadas de SendEvents de Blur e
//     Focus terminarem, então GetFocusElement() fica obsoleto -- ainda o valor ANTIGO -- durante
//     toda a duração de um despacho em fan-out de Focus/Blur; ver o doc-comment de
//     set_focus_callback pra como essa assimetria é tratada em vez disso.)
class HoverEventListener : public Rml::EventListener {
public:
  HoverEventListener(std::function<void(const char*, bool)>* cb, Rml::ElementDocument* doc)
      : cb_(cb), doc_(doc) {}

  void ProcessEvent(Rml::Event& event) override {
    if (!cb_ || !*cb_) return;
    // EN: Resolve from the Context's LIVE hover element, not event.GetTargetElement() -- see
    //     the class-level comment above for why (fan-out dispatch order-independence).
    // PT: Resolve a partir do elemento de hover VIVO do Context, não event.GetTargetElement()
    //     -- ver o comentário de nível de classe acima pro motivo (independência de ordem do
    //     despacho em fan-out).
    Rml::Element* target = event.GetTargetElement();
    Rml::Context* target_ctx = target ? target->GetContext() : nullptr;
    Rml::Element* el = target_ctx ? target_ctx->GetHoverElement() : target;
    // EN: Same AncestorIdOf() walk as ClickEventListener::ProcessEvent above -- starting point
    //     `el` is the live hover element resolved above, not event.GetTargetElement() directly
    //     (see the class-level comment above for why).
    // PT: Mesma subida AncestorIdOf() do ClickEventListener::ProcessEvent acima -- o ponto de
    //     partida `el` é o elemento de hover vivo resolvido acima, não event.GetTargetElement()
    //     diretamente (ver o comentário de nível de classe acima do motivo).
    const char* id_str = AncestorIdOf(el, doc_);
    // EN: Dedup: no state change, no-op. This is what kills re-firing while the cursor crosses
    //     unlabeled children of the SAME labeled container (every raw Mouseover on those
    //     children resolves, via the ancestor walk above, to the SAME container id).
    // PT: Dedup: sem mudança de estado, no-op. É isso que mata o redisparo enquanto o cursor
    //     cruza filhos sem id do MESMO container com id (todo Mouseover cru nesses filhos
    //     resolve, pela subida de ancestral acima, para o MESMO id de container).
    if (current_hover_id_ == id_str) return;
    // EN: Capture the OLD id into our own std::string (survives the mutation below) BEFORE
    //     updating current_hover_id_ -- current_hover_id_ itself must already hold the NEW
    //     value BEFORE either functor is invoked (both the AUD-TEC-3 reentrancy discipline and
    //     the exact ordering set_hover_callback's doc-comment specifies).
    // PT: Captura o id ANTIGO na nossa própria std::string (sobrevive à mutação abaixo) ANTES
    //     de atualizar current_hover_id_ -- o próprio current_hover_id_ já deve conter o valor
    //     NOVO ANTES de qualquer functor ser invocado (tanto a disciplina de reentrância
    //     AUD-TEC-3 quanto a ordem exata que o doc-comment de set_hover_callback especifica).
    const std::string previous = current_hover_id_;
    current_hover_id_ = id_str;
    // EN: Copy the functor before invoking (AUD-TEC-3), same reentrancy guard as every other
    //     listener in this file.
    // PT: Copia o functor antes de invocar (AUD-TEC-3), mesma guarda de reentrância de todo
    //     outro listener deste arquivo.
    auto cb = *cb_;
    if (!cb) return;
    if (!previous.empty()) cb(previous.c_str(), false);
    if (!current_hover_id_.empty()) cb(current_hover_id_.c_str(), true);
  }

  void OnDetach(Rml::Element* /*element*/) override { delete this; }

private:
  std::function<void(const char*, bool)>* cb_;
  Rml::ElementDocument* doc_;
  // EN: OUR OWN copy of the currently-resolved hover id -- "" means nothing (with an id) is
  //     currently considered hovered. See the class-level comment above for why this lives
  //     here (per-listener-instance) rather than in Bootstrap::Impl.
  // PT: Cópia NOSSA do id de hover resolvido atualmente -- "" significa que nada (com id) é
  //     considerado em hover no momento. Ver o comentário de nível de classe acima pro motivo
  //     de isto morar aqui (por instância de listener) em vez de em Bootstrap::Impl.
  std::string current_hover_id_;
};

Bootstrap::~Bootstrap() { shutdown(); }

bool Bootstrap::init(Rml::SystemInterface* system, RenderGl3& render, int w, int h,
                      FontEngine font_engine) {
  if (impl_) return false; // EN: guard: already initialised. PT: guard: já inicializado.

  impl_ = new Impl();
  Rml::SetSystemInterface(system);
  Rml::SetRenderInterface(render.iface());
#if GLINTFX_OWN_FONT_ENGINE
  // EN: L1.19-FONTENG A/B gate -- install BEFORE Rml::Initialise() so our engine is the ACTIVE
  //     one from the very first font load inside this session (RmlUi resolves the registered
  //     FontEngineInterface lazily, on first use, but installing early removes any doubt).
  //     RMLUI_FONT_ENGINE=freetype remains the CMake-time default for the fetched RmlUi build
  //     (glintfx/CMakeLists.txt) either way -- Rml::SetFontEngineInterface() overrides it at
  //     RUNTIME, so switching this A/B needs no RmlUi rebuild, only reconfiguring/relinking
  //     glintfx itself with a different CMake option.
  // PT: Gate A/B do L1.19-FONTENG -- instala ANTES do Rml::Initialise() para que nosso motor
  //     seja o ATIVO desde o 1º carregamento de fonte dentro desta sessão (o RmlUi resolve a
  //     FontEngineInterface registrada preguiçosamente, no 1º uso, mas instalar cedo remove
  //     qualquer dúvida). RMLUI_FONT_ENGINE=freetype continua o padrão em tempo de CMake pro
  //     build fetchado do RmlUi (glintfx/CMakeLists.txt) de qualquer forma --
  //     Rml::SetFontEngineInterface() o sobrepõe em RUNTIME, então trocar este A/B não exige
  //     rebuild do RmlUi, só reconfigurar/relinkar o próprio glintfx com uma option de CMake
  //     diferente.
  //
  //     PRECEDENCE (L1.20-FONTFLIP Phase 2): own_font_engine_ab_bypass() (test-only, see below)
  //     WINS over `font_engine` -- checked first, short-circuits the `font_engine` check via
  //     `&&`, so a test that forces the bypass stays authoritative regardless of what a public
  //     FontEngine config asked for (keeps tests/fonteng_ab_compare.cpp's pre-existing contract
  //     intact). Below that: `font_engine == FontEngine::Own` (the public, per-instance choice
  //     from AppConfig::font_engine/UiLayerConfig::font_engine) gates the actual install. When
  //     `font_engine == FontEngine::FreeType`, the own engine is never installed here -- no
  //     `#else` fallback warning needed in THIS branch, since the own engine module IS compiled
  //     in (GLINTFX_OWN_FONT_ENGINE=ON) but simply not the caller's pick; RmlUi's FreeType
  //     default takes over inside Rml::Initialise() exactly as if this whole #if block were
  //     absent.
  //     PRECEDÊNCIA (L1.20-FONTFLIP Fase 2): own_font_engine_ab_bypass() (só-de-teste, ver
  //     abaixo) VENCE sobre `font_engine` -- checado primeiro, faz curto-circuito no check de
  //     `font_engine` via `&&`, então um teste que força o bypass permanece autoritativo
  //     independente do que um FontEngine config público pediu (mantém o contrato pré-existente
  //     do tests/fonteng_ab_compare.cpp intacto). Abaixo disso: `font_engine ==
  //     FontEngine::Own` (a escolha pública, por-instância, de AppConfig::font_engine/
  //     UiLayerConfig::font_engine) controla a instalação de fato. Quando `font_engine ==
  //     FontEngine::FreeType`, o motor próprio nunca é instalado aqui -- sem aviso de fallback
  //     `#else` necessário NESTE ramo, já que o módulo do motor próprio ESTÁ compilado
  //     (GLINTFX_OWN_FONT_ENGINE=ON) mas simplesmente não é a escolha do chamador; o default
  //     FreeType do RmlUi assume dentro do Rml::Initialise() exatamente como se este bloco #if
  //     inteiro estivesse ausente.
  //
  //     A/B TEST BYPASS (L1.20-FONTFLIP, FT-F4): own_font_engine_ab_bypass() is a src-internal,
  //     test-only toggle (default false -- see its doc-comment in font_engine_own.hpp). When a
  //     test sets it true, this install is SKIPPED, so RmlUi's global font_interface stays null
  //     and Rml::Initialise() (called just below) falls back to its built-in FreeType default
  //     engine -- the ONE runtime switch that lets tests/fonteng_ab_compare.cpp measure the same
  //     scene through BOTH engines in a single ON build. A normal ON build never flips it, so the
  //     own engine is installed exactly as before (when `font_engine == FontEngine::Own`, the
  //     default).
  //     BYPASS DE TESTE A/B (L1.20-FONTFLIP, FT-F4): own_font_engine_ab_bypass() é um toggle
  //     src-interno, só-de-teste (default false -- ver seu doc-comment em font_engine_own.hpp).
  //     Quando um teste o seta true, esta instalação é PULADA, então o font_interface global do
  //     RmlUi fica nulo e o Rml::Initialise() (chamado logo abaixo) cai no motor FreeType default
  //     embutido -- o ÚNICO switch de runtime que deixa o tests/fonteng_ab_compare.cpp medir a
  //     mesma cena através dos DOIS motores num único build ON. Um build ON normal nunca o vira,
  //     então o motor próprio é instalado exatamente como antes (quando `font_engine ==
  //     FontEngine::Own`, o default).
  if (!glintfx::own_font_engine_ab_bypass() && font_engine == FontEngine::Own) {
    Rml::SetFontEngineInterface(&impl_->font_engine_own);
  }
#else
  // EN: FALLBACK (L1.20-FONTFLIP Phase 2): this build was compiled with
  //     GLINTFX_OWN_FONT_ENGINE=OFF -- the own engine module does not exist in this binary at
  //     all (font_engine_own.hpp/.cpp are excluded from the build, glintfx/CMakeLists.txt), so
  //     there is no interface to install regardless of `font_engine`. A caller that requested
  //     FontEngine::Own gets FreeType instead (RmlUi's own built-in default, taking over inside
  //     Rml::Initialise() below exactly as if this call had asked for FreeType) -- NEVER a crash
  //     or a hard failure, per FontEngine's own doc-comment contract. Logged once per init() call
  //     so a consumer who opted OUT of the own engine at build time, but forgot to also flip
  //     their config to FontEngine::FreeType, is not silently surprised.
  // PT: FALLBACK (L1.20-FONTFLIP Fase 2): este build foi compilado com
  //     GLINTFX_OWN_FONT_ENGINE=OFF -- o módulo do motor próprio simplesmente não existe neste
  //     binário (font_engine_own.hpp/.cpp são excluídos do build, glintfx/CMakeLists.txt), então
  //     não há interface para instalar independente de `font_engine`. Um chamador que pediu
  //     FontEngine::Own recebe FreeType no lugar (o default embutido do próprio RmlUi, assumindo
  //     dentro do Rml::Initialise() abaixo exatamente como se esta chamada tivesse pedido
  //     FreeType) -- NUNCA um crash ou falha dura, conforme o contrato do doc-comment do próprio
  //     FontEngine. Logado uma vez por chamada a init() para que um consumidor que optou por SAIR
  //     do motor próprio em tempo de build, mas esqueceu de também virar seu config para
  //     FontEngine::FreeType, não seja surpreendido silenciosamente.
  if (font_engine == FontEngine::Own) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "FontEngine::Own was requested, but this glintfx build was compiled with "
        "GLINTFX_OWN_FONT_ENGINE=OFF (the own font engine is not available in this binary) -- "
        "falling back to RmlUi's built-in FreeType engine.");
  }
#endif
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

  // EN: FX-CARVE-1 -- the whole construct+register block below (all 3 decorator instancers)
  //     only compiles when the fx module is on; with GLINTFX_MODULE_FX=OFF none of
  //     impl_->polygon_instancer/tint_instancer/ripple_instancer exist (see the Impl struct's
  //     own #if above), so nothing here would even compile without this matching gate.
  // PT: FX-CARVE-1 -- o bloco de construção+registro inteiro abaixo (os 3 instancers de
  //     decorator) só compila quando o módulo fx está ligado; com GLINTFX_MODULE_FX=OFF nenhum
  //     de impl_->polygon_instancer/tint_instancer/ripple_instancer existe (ver o próprio #if
  //     da struct Impl acima), então nada aqui sequer compilaria sem este gate correspondente.
#if GLINTFX_MODULE_FX
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

  // EN: L1.22-WAVE -- construct + register the "ripple" decorator instancer, same ordering
  //     constraint as "polygon"/"image-tint" immediately above (see impl_->ripple_instancer's
  //     own doc comment). Registered as "ripple" so "decorator: ripple(200);" /
  //     "decorator: ripple;" both resolve. NO extra gate-counter wiring needed here (unlike a
  //     prior revision, commit 647350f) -- the L1.22-CAPTURE cost-zero-when-inactive gate is now
  //     structural, entirely inside src/fx/effects_gl3.cpp's Gl3FxHook::arm_backdrop_capture/
  //     EnsureBackdropCaptured pair (FX-CARVE-1); see decorator_ripple.hpp's own doc comment for
  //     the fix and why the earlier counter-based gate had a cold-start bug.
  // PT: L1.22-WAVE -- constrói + registra o instancer do decorator "ripple", mesma restrição de
  //     ordenação de "polygon"/"image-tint" logo acima (ver o próprio doc-comment de
  //     impl_->ripple_instancer). Registrado como "ripple" para que "decorator: ripple(200);" /
  //     "decorator: ripple;" ambos resolvam. NENHUMA conexão extra de contador de gate
  //     necessária aqui (diferente de uma revisão anterior, commit 647350f) -- o gate de
  //     custo-zero-quando-inativo do L1.22-CAPTURE agora é estrutural, inteiramente dentro do
  //     par Gl3FxHook::arm_backdrop_capture/EnsureBackdropCaptured de src/fx/effects_gl3.cpp
  //     (FX-CARVE-1); ver o próprio doc-comment de decorator_ripple.hpp pro fix e por que o
  //     gate baseado em contador anterior tinha um bug de cold-start.
  impl_->ripple_instancer = Rml::MakeUnique<glintfx::RippleDecoratorInstancer>();
  Rml::Factory::RegisterDecoratorInstancer("ripple", impl_->ripple_instancer.get());
#endif  // GLINTFX_MODULE_FX

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
  // EN: L1.15-FORMEV -- change/submit/hover are bubble-phase (in_capture_phase=false), same
  //     pattern as every listener above. focus/blur are CAPTURE-phase (in_capture_phase=true)
  //     -- see FocusEventListener/BlurEventListener's class comments and
  //     Bootstrap::set_focus_callback's doc-comment for why (Focus/Blur do not bubble).
  // PT: L1.15-FORMEV -- change/submit/hover são fase bubble (in_capture_phase=false), mesmo
  //     padrão de todo listener acima. focus/blur são fase de CAPTURA (in_capture_phase=true)
  //     -- ver os comentários de classe de FocusEventListener/BlurEventListener e o doc-comment
  //     de Bootstrap::set_focus_callback pro motivo (Focus/Blur não borbulham).
  doc->AddEventListener(Rml::EventId::Change,
                         new ChangeEventListener(&impl_->change_cb, doc), false);
  doc->AddEventListener(Rml::EventId::Submit,
                         new SubmitEventListener(&impl_->submit_cb, doc), false);
  doc->AddEventListener(Rml::EventId::Focus,
                         new FocusEventListener(&impl_->focus_cb, doc), true);
  doc->AddEventListener(Rml::EventId::Blur,
                         new BlurEventListener(&impl_->blur_cb, doc), true);
  doc->AddEventListener(Rml::EventId::Mouseover,
                         new HoverEventListener(&impl_->hover_cb, doc), false);
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

void Bootstrap::set_change_callback(std::function<void(const char*, const char*)> cb) {
  if (impl_) impl_->change_cb = std::move(cb);
}

void Bootstrap::set_submit_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->submit_cb = std::move(cb);
}

void Bootstrap::set_focus_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->focus_cb = std::move(cb);
}

void Bootstrap::set_blur_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->blur_cb = std::move(cb);
}

void Bootstrap::set_hover_callback(std::function<void(const char*, bool)> cb) {
  if (impl_) impl_->hover_cb = std::move(cb);
}

// EN: AUD-L1-QUALITY Item A -- shared lookup prelude for the id-keyed DOM accessors below.
//     See the doc-comment on the declaration (bootstrap.hpp) for the full contract; this text
//     was previously duplicated verbatim across get_element_box .. set_property.
// PT: AUD-L1-QUALITY Item A -- prelúdio de busca compartilhado pelos acessores DOM indexados
//     por id abaixo. Ver o doc-comment da declaração (bootstrap.hpp) pro contrato completo;
//     este texto estava duplicado ao pé da letra entre get_element_box .. set_property.
Rml::Element* Bootstrap::find_element(const char* id) const {
  if (!impl_ || !impl_->doc || !id || !*id) return nullptr;
  return impl_->doc->GetElementById(id);
}

bool Bootstrap::get_element_box(const char* id, float& x, float& y, float& w, float& h) const {
  // EN: find_element() folds in the AUD-TEC-5 guard (reject empty id "" too, not just null)
  //     and the impl_/doc null-checks -- see its doc-comment (bootstrap.hpp) for the contract.
  // PT: find_element() incorpora a guarda AUD-TEC-5 (rejeita id vazio "" também, não só nulo)
  //     e os checks de nulo de impl_/doc -- ver o doc-comment dela (bootstrap.hpp) pro contrato.
  Rml::Element* el = find_element(id);
  if (!el) return false;
  const Rml::Vector2f offset = el->GetAbsoluteOffset(Rml::BoxArea::Border);
  const Rml::Vector2f size   = el->GetBox().GetSize(Rml::BoxArea::Border);
  x = offset.x; y = offset.y; w = size.x; h = size.y;
  return true;
}

bool Bootstrap::scroll_element_into_view(const char* id, bool align_with_top) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  el->ScrollIntoView(align_with_top);
  return true;
}

bool Bootstrap::get_element_scroll_top(const char* id, float& out_scroll_top) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  out_scroll_top = el->GetScrollTop();
  return true;
}

bool Bootstrap::get_element_scroll_height(const char* id, float& out_scroll_height) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  out_scroll_height = el->GetScrollHeight();
  return true;
}

bool Bootstrap::get_element_client_height(const char* id, float& out_client_height) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  out_client_height = el->GetClientHeight();
  return true;
}

// EN: NOT refactored onto find_element() (AUD-L1-QUALITY Item A, deliberate exclusion): the
//     std::isfinite(scroll_top) check is INTERLEAVED between the id guard and the element
//     lookup below, not a byte-identical repeat of the shared prelude -- forcing it into
//     find_element() would mean reordering the isfinite check around the helper call. Left as
//     its own inline sequence per this wave's conservative-refactor instruction (see
//     AUD-L1-QUALITY.md and find_element()'s own doc-comment in bootstrap.hpp).
// PT: NÃO refatorado sobre find_element() (AUD-L1-QUALITY Item A, exclusão deliberada): o
//     check std::isfinite(scroll_top) fica INTERCALADO entre a guarda de id e a busca do
//     elemento abaixo, não é uma repetição byte-idêntica do prelúdio compartilhado --
//     forçá-lo em find_element() significaria reordenar o check de isfinite ao redor da
//     chamada do helper. Deixado como sua própria sequência inline conforme a instrução
//     conservadora desta onda (ver AUD-L1-QUALITY.md e o doc-comment de find_element() em
//     bootstrap.hpp).
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

bool Bootstrap::set_focus(const char* id) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  // EN: Element::Focus()'s own bool -- false when the element itself is authored
  //     `focus: none;` (see the doc-comment in bootstrap.hpp for the full `focus` vs.
  //     `tab-index` distinction confirmed against the pinned RmlUi source).
  // PT: O próprio bool de Element::Focus() -- false quando o próprio elemento está
  //     autorado com `focus: none;` (ver o doc-comment em bootstrap.hpp para a distinção
  //     completa `focus` vs. `tab-index` confirmada contra o source pinado do RmlUi).
  return el->Focus();
}

bool Bootstrap::clear_focus() const {
  // EN: Guard: must have a document loaded, same "must have a document" gate as every
  //     other Bootstrap query method above -- but NOT an id guard (this method has no id
  //     parameter; it operates on whatever the Context currently reports as focused).
  // PT: Guard: precisa ter um documento carregado, mesma guarda de "precisa ter
  //     documento" de todo outro método de consulta do Bootstrap acima -- mas SEM guarda
  //     de id (este método não tem parâmetro de id; opera sobre o que o Context reporta
  //     como focado no momento).
  if (!impl_ || !impl_->doc) return false;
  Rml::Element* focused = impl_->ctx ? impl_->ctx->GetFocusElement() : nullptr;
  // EN: Idempotent no-op: nothing focused means "cleared" already holds. See the
  //     doc-comment in bootstrap.hpp for the full rationale.
  // PT: No-op idempotente: nada focado significa que "limpo" já vale. Ver o
  //     doc-comment em bootstrap.hpp para a racional completa.
  if (!focused) return true;
  focused->Blur();
  return true;
}

bool Bootstrap::set_text(const char* id, const char* text) const {
  // EN: find_element() folds in the AUD-TEC-5 guard -- see its doc-comment (bootstrap.hpp).
  // PT: find_element() incorpora a guarda AUD-TEC-5 -- ver o doc-comment dela (bootstrap.hpp).
  Rml::Element* el = find_element(id);
  if (!el) return false;
  // EN: CRITICAL hardening (see the doc-comment in bootstrap.hpp for the full rationale):
  //     escape RML-significant characters BEFORE SetInnerRML, so `text` can never inject
  //     markup -- only ever ends up as the literal visible text. Null text is normalised to ""
  //     (DataBinder::set_string precedent), never rejected.
  // PT: Hardening CRÍTICO (ver o doc-comment em bootstrap.hpp para a racional completa):
  //     escapa caracteres significativos pra RML ANTES do SetInnerRML, então `text` nunca pode
  //     injetar markup -- só termina como o texto visível literal. Texto nulo é normalizado
  //     para "" (precedente de DataBinder::set_string), nunca rejeitado.
  el->SetInnerRML(Rml::StringUtilities::EncodeRml(Rml::String(text ? text : "")));
  return true;
}

bool Bootstrap::add_class(const char* id, const char* cls) const {
  // EN: Guard (AUD-TEC-5): cls must be non-null/non-empty -- an empty class name is a
  //     structurally-invalid caller input (e.g. a computed class string that resolved empty),
  //     rejected the same way an empty id is. The id/impl_/doc part of the original combined
  //     guard now lives in find_element() below -- splitting a single `&&`-conjunction guard
  //     into two sequential early-returns changes neither input has no observable behaviour
  //     (both checks are pure, side-effect-free, and their overall truth table -- reject iff
  //     id invalid OR cls invalid -- is unchanged by evaluation order).
  // PT: Guard (AUD-TEC-5): cls deve ser não-nulo/não-vazio -- um nome de classe vazio é
  //     entrada estruturalmente inválida do chamador (ex.: string de classe computada que
  //     resolveu vazia), rejeitada do mesmo jeito que um id vazio. A parte id/impl_/doc da
  //     guarda combinada original agora mora em find_element() abaixo -- dividir uma guarda de
  //     conjunção `&&` única em dois early-returns sequenciais não muda comportamento
  //     observável (os dois checks são puros, sem efeito colateral, e a tabela-verdade geral
  //     -- rejeita se id inválido OU cls inválido -- é inalterada pela ordem de avaliação).
  if (!cls || !*cls) return false;
  Rml::Element* el = find_element(id);
  if (!el) return false;
  el->SetClass(cls, /*activate=*/true);
  return true;
}

bool Bootstrap::remove_class(const char* id, const char* cls) const {
  // EN: Guard (AUD-TEC-5): same cls null-or-empty rejection as add_class above, same
  //     split-guard equivalence rationale (see add_class's comment above).
  // PT: Guard (AUD-TEC-5): mesma rejeição de cls nulo-ou-vazio do add_class acima, mesma
  //     racional de equivalência de guarda dividida (ver o comentário do add_class acima).
  if (!cls || !*cls) return false;
  Rml::Element* el = find_element(id);
  if (!el) return false;
  el->SetClass(cls, /*activate=*/false);
  return true;
}

bool Bootstrap::set_property(const char* id, const char* prop, const char* value) const {
  // EN: Guard (AUD-TEC-5): prop must be non-null/non-empty -- an empty property NAME cannot
  //     resolve to any real RCSS property, structurally invalid caller input. `value` is NOT
  //     guarded the same way here -- a null value is normalised to "" and forwarded to RmlUi's
  //     own parser, which is the sole authority on whether an empty value is acceptable for
  //     the named property (see the doc-comment in bootstrap.hpp). The id/impl_/doc part of
  //     the original combined guard now lives in find_element() below -- same split-guard
  //     equivalence rationale as add_class's comment above.
  // PT: Guard (AUD-TEC-5): prop deve ser não-nulo/não-vazio -- um NOME de propriedade vazio
  //     não resolve para nenhuma propriedade RCSS real, entrada estruturalmente inválida do
  //     chamador. `value` NÃO é guardado do mesmo jeito aqui -- um value nulo é normalizado
  //     para "" e encaminhado ao próprio parser do RmlUi, que é a única autoridade sobre se um
  //     valor vazio é aceitável para a propriedade nomeada (ver o doc-comment em bootstrap.hpp).
  //     A parte id/impl_/doc da guarda combinada original agora mora em find_element() abaixo
  //     -- mesma racional de equivalência de guarda dividida do comentário de add_class acima.
  if (!prop || !*prop) return false;
  Rml::Element* el = find_element(id);
  if (!el) return false;
  // EN: SetProperty's own bool -- a real parse outcome, propagated unchanged (no glintfx-level
  //     re-validation on top).
  // PT: O próprio bool de SetProperty -- um resultado real de parse, propagado sem mudança
  //     (nenhuma revalidação em nível glintfx por cima).
  return el->SetProperty(prop, value ? value : "");
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
#if GLINTFX_OWN_FONT_ENGINE
    // EN: L1.19-FONTENG -- release our own cached CallbackTexture GPU resources EXPLICITLY,
    //     BEFORE Rml::Shutdown() runs. See FontEngineOwn::Shutdown()'s own doc-comment
    //     (font_engine_own.hpp/.cpp) for why this cannot be left to RmlUi's own internal
    //     `font_interface->Shutdown()` hook alone -- Rml::Shutdown() destroys every Context's
    //     RenderManager (`core_data->contexts.clear()`) BEFORE that hook fires, which is too
    //     late (found the hard way: "Leaking CallbackTexture detected..." RMLUI_ERROR +
    //     shutdown crash without this explicit early call).
    // PT: L1.19-FONTENG -- libera os próprios recursos GPU CallbackTexture cacheados
    //     EXPLICITAMENTE, ANTES do Rml::Shutdown() rodar. Ver o doc-comment próprio do
    //     FontEngineOwn::Shutdown() (font_engine_own.hpp/.cpp) pro motivo disto não poder ser
    //     deixado só pro hook interno `font_interface->Shutdown()` do próprio RmlUi -- o
    //     Rml::Shutdown() destrói o RenderManager de todo Context
    //     (`core_data->contexts.clear()`) ANTES daquele hook disparar, o que é tarde demais
    //     (achado na marra: RMLUI_ERROR "Leaking CallbackTexture detected..." + crash no
    //     shutdown sem esta chamada explícita antecipada).
    impl_->font_engine_own.Shutdown();
#endif
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
