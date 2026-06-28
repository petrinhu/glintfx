// SPDX-License-Identifier: MPL-2.0
// EN: Bootstrap implementation — wires SystemInterface_GLFW + RenderInterface into RmlUi.
//     RmlUi Platform_GLFW 6.3: SystemInterface_GLFW(GLFWwindow*) — constructor takes the
//     window pointer directly; there is no default constructor or SetWindow() method.
// PT: Implementação do Bootstrap — conecta SystemInterface_GLFW + RenderInterface ao RmlUi.
//     Platform_GLFW 6.3: SystemInterface_GLFW(GLFWwindow*) — construtor recebe o ponteiro
//     da janela diretamente; sem construtor default nem método SetWindow().
#include "bootstrap.hpp"
#include "render_gl3.hpp"
#include "window_glfw.hpp"
// EN: Backend header from rmlui_SOURCE_DIR/Backends (added via CMake private include path).
// PT: Header do backend do RmlUi via rmlui_SOURCE_DIR/Backends (via include path privado CMake).
#include "RmlUi_Platform_GLFW.h"
#include <RmlUi/Core.h>

namespace glintfx {

// EN: Impl owns the SystemInterface_GLFW instance on the heap because it has no default
//     constructor — it requires the GLFWwindow* at construction time (available only in init()).
// PT: Impl possui a instância de SystemInterface_GLFW no heap pois não tem construtor default
//     — exige o GLFWwindow* na construção (disponível apenas em init()).
struct Bootstrap::Impl {
  SystemInterface_GLFW* system = nullptr; // EN: heap; owns lifetime. PT: heap; dono do lifetime.
  Rml::Context* ctx = nullptr;
  bool initialised = false;
};

Bootstrap::~Bootstrap() { shutdown(); }

bool Bootstrap::init(WindowGlfw& win, RenderGl3& render, int w, int h) {
  if (impl_) return false; // EN: guard: already initialised. PT: guard: já inicializado.

  impl_ = new Impl();
  impl_->system = new SystemInterface_GLFW(win.handle());

  Rml::SetSystemInterface(impl_->system);
  Rml::SetRenderInterface(render.iface());

  if (!Rml::Initialise()) {
    // EN: Initialise() failed — clean up and leave object safe for shutdown()/dtor.
    // PT: Initialise() falhou — limpa e deixa objeto seguro para shutdown()/dtor.
    delete impl_->system;
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  impl_->initialised = true;

  impl_->ctx = Rml::CreateContext("main", Rml::Vector2i(w, h));
  if (!impl_->ctx) {
    Rml::Shutdown();
    impl_->initialised = false;
    delete impl_->system;
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
    // PT: Rml::Shutdown() libera todos os contextos, documentos e recursos de fonte.
    Rml::Shutdown();
    impl_->initialised = false;
  }
  delete impl_->system;
  impl_->system = nullptr;
  delete impl_;
  impl_ = nullptr;
}

} // namespace glintfx
