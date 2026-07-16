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

  // EN: Compose-only begin: notify RmlUi of the sub-viewport size AND placement -- NO glClear,
  //     NO alpha-fix. (offset_x, offset_y) are OpenGL's NATIVE bottom-left-origin viewport
  //     offset (NOT the UiLayer public top-down contract -- see UiLayer::set_viewport's doc
  //     comment and the ADR-0008 "F3 implementation update" section for the conversion
  //     formula). Forwarded verbatim to RenderInterface_GL3::SetViewport(w, h, offset_x,
  //     offset_y); RmlUi applies it ONLY at the final blit inside EndFrame()
  //     (glViewport(offset_x, offset_y, w, h) right before the fullscreen-quad draw onto
  //     FBO 0) -- confirmed by reading the pinned backend source
  //     (RmlUi_Renderer_GL3.cpp:909-910). Internal render-layer passes (the UI's own content
  //     rasterisation into its private FBOs) are UNAFFECTED by the offset -- they always start
  //     at (0,0), exactly as before F3 (RmlUi_Renderer_GL3.cpp:857).
  // PT: Begin compose-only: notifica o RmlUi do tamanho E posicionamento da sub-viewport --
  //     SEM glClear, SEM alpha-fix. (offset_x, offset_y) são o offset de viewport NATIVO do
  //     OpenGL, origem inferior-esquerda (NÃO o contrato público top-down do UiLayer -- ver o
  //     doc-comment de UiLayer::set_viewport e a seção "F3 implementation update" do ADR-0008
  //     pra fórmula de conversão). Repassado literalmente a
  //     RenderInterface_GL3::SetViewport(w, h, offset_x, offset_y); o RmlUi o aplica SÓ no
  //     blit final dentro de EndFrame() (glViewport(offset_x, offset_y, w, h) logo antes do
  //     draw do quad fullscreen no FBO 0) -- confirmado lendo o source do backend pinado
  //     (RmlUi_Renderer_GL3.cpp:909-910). Os passes internos do render-layer (rasterização do
  //     próprio conteúdo da UI nos FBOs privados dela) NÃO são afetados pelo offset -- sempre
  //     começam em (0,0), exatamente como antes da F3 (RmlUi_Renderer_GL3.cpp:857).
  void begin_frame_compose(int offset_x, int offset_y, int w, int h);

  // EN: Compose-only end: call RmlUi EndFrame — NO FBO0 alpha-fix.
  // PT: End compose-only: chama EndFrame do RmlUi — SEM alpha-fix do FBO0.
  void end_frame_compose();

  // EN: L1.22-CAPTURE — wires the "ripple()" decorator's shared active-instance counter into
  //     the renderer, so begin_frame_compose can skip the FBO-0 backdrop capture entirely (zero
  //     GL calls, zero allocation) whenever no "ripple()" decorator is currently alive anywhere
  //     in the document tree -- see Gl3RenderInterface::CaptureBackdrop's doc comment
  //     (render_gl3.cpp) and decorator_ripple.hpp's class-level doc comment for the full
  //     contract. `counter` is a plain, non-owning `const int*` -- no RmlUi/GL type leaks
  //     through this signature (same "no RmlUi or GL types leak" discipline this whole header
  //     already follows). `counter` must outlive this RenderGl3 -- bootstrap.cpp satisfies that
  //     (owned by Bootstrap::Impl, same lifetime discipline as the decorator instancers
  //     themselves). A no-op if init() was not called or failed (mirrors every other method on
  //     this class).
  // PT: L1.22-CAPTURE — conecta o contador de instância ativa compartilhado do decorator
  //     "ripple()" ao renderer, para que begin_frame_compose possa pular a captura de backdrop
  //     do FBO-0 inteiramente (zero chamadas GL, zero alocação) sempre que nenhum decorator
  //     "ripple()" estiver vivo em nenhum lugar da árvore do documento -- ver o doc-comment de
  //     Gl3RenderInterface::CaptureBackdrop (render_gl3.cpp) e o doc-comment de nível de classe
  //     de decorator_ripple.hpp pro contrato completo. `counter` é um `const int*` comum,
  //     não-dono -- nenhum tipo RmlUi/GL vaza nesta assinatura (mesma disciplina "nenhum tipo
  //     RmlUi ou GL vaza" que este header inteiro já segue). `counter` precisa sobreviver a este
  //     RenderGl3 -- bootstrap.cpp satisfaz isso (dono é Bootstrap::Impl, mesma disciplina de
  //     lifetime dos próprios decorator instancers). Sem efeito se init() não foi chamado ou
  //     falhou (espelha todo outro método desta classe).
  void set_ripple_active_counter(const int* counter);

private:
  struct Impl;   // EN: defined in render_gl3.cpp; holds RenderInterface_GL3.
                 // PT: definido em render_gl3.cpp; contém RenderInterface_GL3.
  Impl* impl_;
};

} // namespace glintfx
