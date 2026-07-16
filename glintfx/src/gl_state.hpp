// SPDX-License-Identifier: MPL-2.0
// EN: RAII snapshot/restore of the GL state the RmlUi GL3 backend touches, so the host's
//     renderer (e.g. Render2dGl3 in GusWorld) sees the context unchanged after UiLayer::render().
//     Captured: viewport, scissor box+enable, blend func+equation+enable, cull/depth/stencil
//     enables, active program, vertex array binding, active texture unit, 2D texture binding,
//     draw FBO binding, READ FBO binding + read buffer (L1.22-CAPTURE), and colour write mask.
//
//     L1.22-CAPTURE ADDITION (GL_READ_FRAMEBUFFER_BINDING / GL_READ_BUFFER): added for
//     Gl3RenderInterface::EnsureBackdropCaptured (render_gl3.cpp), which rebinds
//     GL_READ_FRAMEBUFFER to 0 and calls glReadBuffer(GL_BACK) to read the host's already-drawn
//     backbuffer content, but deliberately does NOT restore either itself. THIS is the one
//     correct place to do it: this guard's ctor runs in Engine::render_compose (engine.cpp)
//     BEFORE RenderGl3::begin_frame_compose, and this guard's dtor runs AFTER
//     RenderGl3::end_frame_compose (RenderInterface_GL3::EndFrame()) returns --
//     EnsureBackdropCaptured itself runs SOMEWHERE inside that window (on demand, from within
//     Rml::Context::Render()'s tree walk, the first time -- if ever -- a "glintfx-ripple" shader
//     actually draws that frame; see EnsureBackdropCaptured's own doc comment, render_gl3.cpp,
//     for the L1.22-CAPTURE cold-start-fix derivation of exactly WHEN within the window it runs)
//     -- the EXACT moment within the window does not matter for this guard's own correctness,
//     only that the window fully encloses it, which it always does. EndFrame() itself
//     unconditionally rebinds BOTH read+draw targets to FBO 0 partway through
//     (glBindFramebuffer(GL_FRAMEBUFFER, 0), Backends/RmlUi_Renderer_GL3.cpp -- confirmed by
//     reading the pinned source) -- so a save/restore scoped to JUST
//     EnsureBackdropCaptured's own method body would be silently clobbered by EndFrame() moments
//     later regardless, and could never recover a host read-fb binding that was NOT already FBO
//     0 to begin with. Restoring here, at the very end of the whole render_compose() call, is
//     therefore the only place that actually returns the host's TRUE original read-framebuffer
//     state.
// PT: Snapshot/restore RAII do estado GL que o backend GL3 do RmlUi mexe, para o renderer do
//     host (ex.: Render2dGl3 no GusWorld) ver o contexto inalterado após UiLayer::render().
//     Capturado: viewport, scissor box+enable, blend func+equation+enable, enables
//     cull/depth/stencil, programa ativo, vertex array binding, unidade de textura ativa,
//     binding de textura 2D, draw FBO binding, binding de READ FBO + read buffer
//     (L1.22-CAPTURE), e colour write mask.
//
//     ADIÇÃO DO L1.22-CAPTURE (GL_READ_FRAMEBUFFER_BINDING / GL_READ_BUFFER): adicionado para
//     Gl3RenderInterface::EnsureBackdropCaptured (render_gl3.cpp), que revincula
//     GL_READ_FRAMEBUFFER a 0 e chama glReadBuffer(GL_BACK) pra ler o conteúdo do backbuffer já
//     desenhado pelo host, mas deliberadamente NÃO restaura nenhum dos dois por conta própria.
//     ESTE é o único lugar correto pra fazer isso: o ctor deste guard roda em
//     Engine::render_compose (engine.cpp) ANTES de RenderGl3::begin_frame_compose, e o dtor
//     deste guard roda DEPOIS de RenderGl3::end_frame_compose (RenderInterface_GL3::EndFrame())
//     retornar -- o próprio EnsureBackdropCaptured roda EM ALGUM PONTO dentro dessa janela (sob
//     demanda, de dentro da caminhada de árvore de Rml::Context::Render(), na primeira vez -- se
//     alguma -- que um shader "glintfx-ripple" de fato desenha naquele frame; ver o próprio
//     doc-comment de EnsureBackdropCaptured, render_gl3.cpp, pra derivação do fix de cold-start
//     do L1.22-CAPTURE de exatamente QUANDO dentro da janela ele roda) -- o momento EXATO dentro
//     da janela não importa pra correção deste próprio guard, só que a janela o envolva por
//     inteiro, o que sempre acontece. O próprio EndFrame() revincula AMBOS os alvos read+draw
//     pro FBO 0 incondicionalmente no meio do caminho (glBindFramebuffer(GL_FRAMEBUFFER, 0),
//     Backends/RmlUi_Renderer_GL3.cpp -- confirmado lendo o source pinado) -- então um
//     save/restore restrito só ao corpo do próprio método EnsureBackdropCaptured seria
//     silenciosamente sobrescrito pelo EndFrame() momentos depois de qualquer forma, e nunca
//     poderia recuperar um binding de read-fb de host que NÃO era já o FBO 0 pra começo de
//     conversa. Restaurar aqui, bem no fim da chamada inteira de render_compose(), é portanto o
//     único lugar que de fato devolve o estado de read-framebuffer ORIGINAL VERDADEIRO do host.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include "gl_loader.h"

namespace glintfx {

class GlStateGuard {
public:
  GlStateGuard() {
    glGetIntegerv(GL_VIEWPORT,                 vp_);
    blend_   = glIsEnabled(GL_BLEND);
    scissor_ = glIsEnabled(GL_SCISSOR_TEST);
    cull_    = glIsEnabled(GL_CULL_FACE);
    depth_   = glIsEnabled(GL_DEPTH_TEST);
    stencil_ = glIsEnabled(GL_STENCIL_TEST);
    glGetIntegerv(GL_SCISSOR_BOX,             scbox_);
    glGetIntegerv(GL_BLEND_SRC_RGB,           &bsrc_rgb_);
    glGetIntegerv(GL_BLEND_DST_RGB,           &bdst_rgb_);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,         &bsrc_a_);
    glGetIntegerv(GL_BLEND_DST_ALPHA,         &bdst_a_);
    // EN: Blend equations — RmlUi's BeginFrame() calls glBlendEquation(GL_FUNC_ADD);
    //     must be restored so the host sees its original equation on return.
    // PT: Equações de blend — BeginFrame() do RmlUi chama glBlendEquation(GL_FUNC_ADD);
    //     devem ser restauradas para o host ver a equação original ao retornar.
    glGetIntegerv(GL_BLEND_EQUATION_RGB,      &beq_rgb_);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA,    &beq_a_);
    glGetIntegerv(GL_CURRENT_PROGRAM,         &prog_);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,    &vao_);
    glGetIntegerv(GL_ACTIVE_TEXTURE,          &active_tex_);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,      &tex2d_);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo_);
    // EN: L1.22-CAPTURE — see the class-level doc comment above for why THIS ctor (not a local
    //     save/restore inside EnsureBackdropCaptured itself) is the correct place to snapshot these.
    //     GL_READ_BUFFER always queries whichever framebuffer object is CURRENTLY bound to
    //     GL_READ_FRAMEBUFFER at this point (the host's own, untouched-by-us binding) -- if that
    //     binding is later something EnsureBackdropCaptured never rebinds to 0 itself (i.e. the host's
    //     original read-fb was NOT the default framebuffer), this captured value pertains to
    //     THAT fbo's own read-buffer state (never mutated by EnsureBackdropCaptured's
    //     glReadBuffer(GL_BACK) call, which only ever touches FBO 0's own stored setting) and
    //     restoring it in the destructor below is a harmless, correct no-op; if the host's
    //     original read-fb WAS 0, this captures FBO 0's pre-capture read-buffer setting, which
    //     EnsureBackdropCaptured's glReadBuffer(GL_BACK) DOES mutate -- restoring it below is then the
    //     step that actually matters.
    // PT: L1.22-CAPTURE — ver o doc-comment de nível de classe acima pro motivo deste ctor (e
    //     não um save/restore local dentro do próprio EnsureBackdropCaptured) ser o lugar correto pra
    //     tirar snapshot destes. GL_READ_BUFFER sempre consulta qualquer que seja o objeto de
    //     framebuffer CORRENTEMENTE vinculado a GL_READ_FRAMEBUFFER neste ponto (o binding
    //     próprio do host, intocado por nós) -- se esse binding for depois algo que
    //     EnsureBackdropCaptured nunca revincula a 0 ele mesmo (isto é, o read-fb original do host NÃO
    //     era o framebuffer default), este valor capturado pertence ao próprio estado de
    //     read-buffer DAQUELE fbo (nunca mutado pela chamada glReadBuffer(GL_BACK) de
    //     EnsureBackdropCaptured, que só toca o próprio ajuste guardado do FBO 0) e restaurá-lo no
    //     destrutor abaixo é um no-op inofensivo e correto; se o read-fb original do host ERA 0,
    //     isto captura o ajuste de read-buffer pré-captura do FBO 0, que o
    //     glReadBuffer(GL_BACK) de EnsureBackdropCaptured DE FATO muta -- restaurá-lo abaixo é então o
    //     passo que de fato importa.
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo_);
    glGetIntegerv(GL_READ_BUFFER,              &read_buffer_);
    glGetBooleanv(GL_COLOR_WRITEMASK,         cmask_);
  }

  ~GlStateGuard() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw_fbo_);
    // EN: L1.22-CAPTURE — bind the read-fb target FIRST, then set its read-buffer, since
    //     GL_READ_BUFFER is state that belongs to whichever fbo is currently bound to
    //     GL_READ_FRAMEBUFFER (see the ctor's doc comment above).
    // PT: L1.22-CAPTURE — vincula o alvo de read-fb PRIMEIRO, depois seta o read-buffer dele, já
    //     que GL_READ_BUFFER é estado que pertence a qualquer fbo que esteja atualmente
    //     vinculado a GL_READ_FRAMEBUFFER (ver o doc-comment do ctor acima).
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)read_fbo_);
    glReadBuffer((GLenum)read_buffer_);
    glViewport(vp_[0], vp_[1], vp_[2], vp_[3]);
    glScissor(scbox_[0], scbox_[1], scbox_[2], scbox_[3]);
    set(GL_BLEND,         blend_);
    set(GL_SCISSOR_TEST,  scissor_);
    set(GL_CULL_FACE,     cull_);
    set(GL_DEPTH_TEST,    depth_);
    set(GL_STENCIL_TEST,  stencil_);
    glBlendFuncSeparate((GLenum)bsrc_rgb_, (GLenum)bdst_rgb_,
                        (GLenum)bsrc_a_,   (GLenum)bdst_a_);
    glBlendEquationSeparate((GLenum)beq_rgb_, (GLenum)beq_a_);
    glUseProgram((GLuint)prog_);
    glBindVertexArray((GLuint)vao_);
    glActiveTexture((GLenum)active_tex_);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex2d_);
    glColorMask(cmask_[0], cmask_[1], cmask_[2], cmask_[3]);
  }

  GlStateGuard(const GlStateGuard&)            = delete;
  GlStateGuard& operator=(const GlStateGuard&) = delete;
  GlStateGuard(GlStateGuard&&)                 = delete;
  GlStateGuard& operator=(GlStateGuard&&)      = delete;

private:
  static void set(GLenum cap, GLboolean on) { on ? glEnable(cap) : glDisable(cap); }

  GLint     vp_[4], scbox_[4];
  GLint     bsrc_rgb_, bdst_rgb_, bsrc_a_, bdst_a_, beq_rgb_, beq_a_;
  GLint     prog_, vao_, active_tex_, tex2d_, draw_fbo_;
  GLint     read_fbo_, read_buffer_;  // L1.22-CAPTURE — see ctor/dtor doc comments above.
  GLboolean blend_, scissor_, cull_, depth_, stencil_, cmask_[4];
};

} // namespace glintfx
