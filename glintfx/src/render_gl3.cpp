// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of RenderGl3 — wraps RmlUi's RenderInterface_GL3.
//     gl3w must be included before any other GL header.
// PT: Implementação de RenderGl3 — encapsula o RenderInterface_GL3 do RmlUi.
//     gl3w deve ser incluído antes de qualquer outro header GL.

// EN: gl3w FIRST — defines all GL 3.x function pointers loaded at runtime.
// PT: gl3w PRIMEIRO — define todos os ponteiros de função GL 3.x carregados em tempo de execução.
#include <GL/gl3w.h>

#include "render_gl3.hpp"
// EN: Backend header from rmlui_SOURCE_DIR/Backends (added via CMake include path).
// PT: Header do backend do RmlUi a partir de rmlui_SOURCE_DIR/Backends (via CMake).
#include "RmlUi_Renderer_GL3.h"

namespace glintfx {

// EN: Impl holds the concrete RenderInterface_GL3 instance.
//     It is constructed lazily inside init() to guarantee a live GL context.
// PT: Impl contém a instância concreta do RenderInterface_GL3.
//     É construído de forma lazy dentro de init() para garantir contexto GL ativo.
struct RenderGl3::Impl {
  RenderInterface_GL3 renderer;
};

RenderGl3::RenderGl3() : impl_(nullptr) {}

RenderGl3::~RenderGl3() {
  delete impl_;   // EN: safe to call on nullptr. PT: seguro com nullptr.
}

bool RenderGl3::init() {
  if (impl_) {
    // EN: already initialized — report existing status.
    // PT: já inicializado — reporta status existente.
    return static_cast<bool>(impl_->renderer);
  }

  // EN: With RMLUI_GL3_CUSTOM_LOADER the namespace init is a no-op, but call
  //     it for forward compatibility and to signal intent.
  // PT: Com RMLUI_GL3_CUSTOM_LOADER a init do namespace é no-op, mas chamamos
  //     para compatibilidade futura e clareza de intenção.
  Rml::String msg;
  if (!RmlGL3::Initialize(&msg)) {
    return false;
  }

  // EN: Construct Impl; the RenderInterface_GL3 constructor compiles GL shaders.
  // PT: Constrói Impl; o construtor do RenderInterface_GL3 compila os shaders GL.
  impl_ = new Impl;
  bool renderer_ok = static_cast<bool>(impl_->renderer);
  if (!renderer_ok) {
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  return true;
}

Rml::RenderInterface* RenderGl3::iface() {
  return impl_ ? &impl_->renderer : nullptr;
}

void RenderGl3::begin_frame(int w, int h) {
  // EN: Guard: no-op if init() was not called or failed (mirrors iface() behaviour).
  // PT: Guard: sem efeito se init() não foi chamado ou falhou (espelha iface()).
  if (!impl_) return;

  // EN: Set viewport and clear the colour buffer before handing control to RmlUi.
  // PT: Define viewport e limpa o buffer de cor antes de entregar controle ao RmlUi.
  glViewport(0, 0, w, h);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  impl_->renderer.SetViewport(w, h);
  impl_->renderer.BeginFrame();
}

void RenderGl3::end_frame() {
  // EN: Guard: no-op if init() was not called or failed (mirrors iface() behaviour).
  // PT: Guard: sem efeito se init() não foi chamado ou falhou (espelha iface()).
  if (!impl_) return;
  impl_->renderer.EndFrame();
}

} // namespace glintfx
