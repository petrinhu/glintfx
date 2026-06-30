// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap implementation — wires caller-supplied SystemInterface + RenderInterface
//     into RmlUi. The caller is the OWNER of the SystemInterface lifetime.
// PT: Implementação do Bootstrap — conecta SystemInterface (fornecida pelo chamador) +
//     RenderInterface ao RmlUi. O chamador é DONO do lifetime do SystemInterface.
// Copyright (c) 2026 Petrus Silva Costa
#include "bootstrap.hpp"
#include "render_gl3.hpp"
#include "base_url_file_interface.hpp"
#include <RmlUi/Core.h>

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
  bool initialised  = false;
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
  doc->Show();
  return true;
}

Rml::Context* Bootstrap::context() { return impl_ ? impl_->ctx : nullptr; }

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
