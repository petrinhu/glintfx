// SPDX-License-Identifier: MPL-2.0
// EN: RAII snapshot/restore of the GL state the RmlUi GL3 backend touches, so the host's
//     renderer (e.g. Render2dGl3 in GusWorld) sees the context unchanged after UiLayer::render().
//     Captured: viewport, scissor box+enable, blend func+enable, cull/depth/stencil enables,
//     active program, vertex array binding, active texture unit, 2D texture binding,
//     draw FBO binding, and colour write mask.
// PT: Snapshot/restore RAII do estado GL que o backend GL3 do RmlUi mexe, para o renderer do
//     host (ex.: Render2dGl3 no GusWorld) ver o contexto inalterado após UiLayer::render().
//     Capturado: viewport, scissor box+enable, blend func+enable, enables cull/depth/stencil,
//     programa ativo, vertex array binding, unidade de textura ativa, binding de textura 2D,
//     draw FBO binding e colour write mask.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <GL/gl3w.h>

namespace glintfx {

class GlStateGuard {
public:
  GlStateGuard() {
    glGetIntegerv(GL_VIEWPORT,            vp_);
    blend_   = glIsEnabled(GL_BLEND);
    scissor_ = glIsEnabled(GL_SCISSOR_TEST);
    cull_    = glIsEnabled(GL_CULL_FACE);
    depth_   = glIsEnabled(GL_DEPTH_TEST);
    stencil_ = glIsEnabled(GL_STENCIL_TEST);
    glGetIntegerv(GL_SCISSOR_BOX,        scbox_);
    glGetIntegerv(GL_BLEND_SRC_RGB,      &bsrc_rgb_);
    glGetIntegerv(GL_BLEND_DST_RGB,      &bdst_rgb_);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,    &bsrc_a_);
    glGetIntegerv(GL_BLEND_DST_ALPHA,    &bdst_a_);
    glGetIntegerv(GL_CURRENT_PROGRAM,    &prog_);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao_);
    glGetIntegerv(GL_ACTIVE_TEXTURE,     &active_tex_);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d_);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo_);
    glGetBooleanv(GL_COLOR_WRITEMASK,    cmask_);
  }

  ~GlStateGuard() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw_fbo_);
    glViewport(vp_[0], vp_[1], vp_[2], vp_[3]);
    glScissor(scbox_[0], scbox_[1], scbox_[2], scbox_[3]);
    set(GL_BLEND,         blend_);
    set(GL_SCISSOR_TEST,  scissor_);
    set(GL_CULL_FACE,     cull_);
    set(GL_DEPTH_TEST,    depth_);
    set(GL_STENCIL_TEST,  stencil_);
    glBlendFuncSeparate((GLenum)bsrc_rgb_, (GLenum)bdst_rgb_,
                        (GLenum)bsrc_a_,   (GLenum)bdst_a_);
    glUseProgram((GLuint)prog_);
    glBindVertexArray((GLuint)vao_);
    glActiveTexture((GLenum)active_tex_);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex2d_);
    glColorMask(cmask_[0], cmask_[1], cmask_[2], cmask_[3]);
  }

  GlStateGuard(const GlStateGuard&)            = delete;
  GlStateGuard& operator=(const GlStateGuard&) = delete;

private:
  static void set(GLenum cap, GLboolean on) { on ? glEnable(cap) : glDisable(cap); }

  GLint     vp_[4], scbox_[4];
  GLint     bsrc_rgb_, bdst_rgb_, bsrc_a_, bdst_a_;
  GLint     prog_, vao_, active_tex_, tex2d_, draw_fbo_;
  GLboolean blend_, scissor_, cull_, depth_, stencil_, cmask_[4];
};

} // namespace glintfx
