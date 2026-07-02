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
#include <RmlUi/Core.h>
#include <RmlUi/Core/StreamMemory.h>
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
    (*cb_)((el && !el->GetId().empty()) ? el->GetId().c_str() : "");
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
  Rml::ElementDocument* doc = impl_->ctx->LoadDocument(rml_path);
  if (!doc) return false;
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
  if (!impl_ || !impl_->doc || !id) return false;
  Rml::Element* el = impl_->doc->GetElementById(id);
  if (!el) return false;
  const Rml::Vector2f offset = el->GetAbsoluteOffset(Rml::BoxArea::Border);
  const Rml::Vector2f size   = el->GetBox().GetSize(Rml::BoxArea::Border);
  x = offset.x; y = offset.y; w = size.x; h = size.y;
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
