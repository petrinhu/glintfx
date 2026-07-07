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
#include "ua_stylesheet.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Core/StreamMemory.h>
#include <cmath>   // EN: std::isfinite (set_element_scroll_top input hardening).
                   // PT: std::isfinite (hardening de entrada de set_element_scroll_top).
#include <cstring>

namespace glintfx {

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
  Rml::Context* ctx = nullptr;
  Rml::ElementDocument* doc = nullptr;  // NEW (F1/F2, v0.2.5): last-loaded document.
  bool initialised  = false;
  // EN: Parsed once in init(), reused as a merge base by every load() -- see there.
  // PT: Parseada uma vez em init(), reutilizada como base de merge por todo load() -- ver lá.
  Rml::SharedPtr<Rml::StyleSheetContainer> ua_style_sheet;
  std::function<void(const char*)> click_cb;  // NEW (F1): empty by default.
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
  impl_->doc = doc;
  return true;
}

Rml::Context* Bootstrap::context() { return impl_ ? impl_->ctx : nullptr; }

void Bootstrap::set_click_callback(std::function<void(const char*)> cb) {
  if (impl_) impl_->click_cb = std::move(cb);
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
