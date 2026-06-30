// SPDX-License-Identifier: MPL-2.0
// EN: Thin RAII wrapper around RmlUi's RenderInterface_GL3.
//     No RmlUi or GL types leak into the public interface.
// PT: Wrapper RAII fino em volta do RenderInterface_GL3 do RmlUi.
//     Nenhum tipo RmlUi ou GL vaza na interface pública.
#pragma once

// EN: Forward-declare only — callers must not depend on RmlUi headers.
// PT: Só forward-declaration — callers não devem depender dos headers do RmlUi.
namespace Rml { class RenderInterface; }

namespace glintfx {

class RenderGl3 {
public:
  RenderGl3();
  ~RenderGl3();
  RenderGl3(const RenderGl3&) = delete;
  RenderGl3& operator=(const RenderGl3&) = delete;

  // EN: Initialize GL3 renderer state. Must be called with a current GL context.
  //     Idempotent: subsequent calls return the existing status without re-initializing.
  //     Returns true on success (first call) or if already successfully initialized.
  // PT: Inicia o estado do renderer GL3. Requer contexto GL ativo.
  //     Idempotente: chamadas subsequentes retornam o status existente sem re-inicializar.
  //     Retorna true em caso de sucesso (primeira chamada) ou se já inicializado com sucesso.
  bool init();

  // EN: Returns the RmlUi render interface (valid after successful init()).
  // PT: Retorna a interface de render do RmlUi (válida após init() com sucesso).
  Rml::RenderInterface* iface();

  // EN: Prepare GL state for a RmlUi render pass. Call before any Rml rendering.
  // PT: Prepara estado GL para um passe de render do RmlUi. Chamar antes de renderizar.
  void begin_frame(int w, int h);

  // EN: Finalize frame and restore saved GL state.
  // PT: Finaliza o frame e restaura o estado GL salvo.
  void end_frame();

  // EN: Compose-only begin: set viewport + notify RmlUi — NO glClear, NO alpha-fix.
  //     The host owns the framebuffer; do not touch its contents before or after.
  // PT: Begin compose-only: define viewport + notifica RmlUi — SEM glClear, SEM alpha-fix.
  //     O host é dono do framebuffer; não tocar no conteúdo antes ou depois.
  void begin_frame_compose(int w, int h);

  // EN: Compose-only end: call RmlUi EndFrame — NO FBO0 alpha-fix.
  // PT: End compose-only: chama EndFrame do RmlUi — SEM alpha-fix do FBO0.
  void end_frame_compose();

private:
  struct Impl;   // EN: defined in render_gl3.cpp; holds RenderInterface_GL3.
                 // PT: definido em render_gl3.cpp; contém RenderInterface_GL3.
  Impl* impl_;
};

} // namespace glintfx
