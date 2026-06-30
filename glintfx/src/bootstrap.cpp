// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap implementation — wires caller-supplied SystemInterface + RenderInterface
//     into RmlUi. The caller is the OWNER of the SystemInterface lifetime.
// PT: Implementação do Bootstrap — conecta SystemInterface (fornecida pelo chamador) +
//     RenderInterface ao RmlUi. O chamador é DONO do lifetime do SystemInterface.
// Copyright (c) 2026 Petrus Silva Costa
#include "bootstrap.hpp"
#include "render_gl3.hpp"
#include <RmlUi/Core.h>

namespace glintfx {

// EN: Impl only needs to track the context + initialisation state.
//     The SystemInterface pointer is NOT stored here — the caller owns it.
// PT: Impl só precisa rastrear o contexto + estado de inicialização.
//     O ponteiro do SystemInterface NÃO é armazenado aqui — o chamador é dono.
struct Bootstrap::Impl {
  Rml::Context* ctx = nullptr;
  bool initialised  = false;
};

Bootstrap::~Bootstrap() { shutdown(); }

bool Bootstrap::init(Rml::SystemInterface* system, RenderGl3& render, int w, int h) {
  if (impl_) return false; // EN: guard: already initialised. PT: guard: já inicializado.

  impl_ = new Impl();
  Rml::SetSystemInterface(system);
  Rml::SetRenderInterface(render.iface());

  if (!Rml::Initialise()) {
    // EN: Initialise() failed — clean up and leave object safe for shutdown()/dtor.
    // PT: Initialise() falhou — limpa e deixa objeto seguro para shutdown()/dtor.
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
