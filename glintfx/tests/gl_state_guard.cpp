// SPDX-License-Identifier: MPL-2.0
// EN: Verify that UiLayer::render() leaves every captured GL state variable unchanged.
//     Sets a non-default GL state (viewport, scissor, blend func+equation, VAO, program,
//     color mask, FBO), snapshots it, calls render(), then asserts each post-render value
//     equals pre-render. A single diverging value is printed and the test fails.
//     Blend equation is specifically probed because RmlUi's BeginFrame() unconditionally
//     calls glBlendEquation(GL_FUNC_ADD); the GlStateGuard must restore the host's original.
// PT: Verifica que UiLayer::render() não altera nenhuma variável de estado GL capturada.
//     Define estado GL não-default (viewport, scissor, blend func+equation, VAO, programa,
//     color mask, FBO), faz snapshot, chama render(), e afirma que cada valor pós-render iguala
//     o pré-render. Um único valor divergente é impresso e o teste falha.
//     A blend equation é especificamente sondada porque BeginFrame() do RmlUi chama
//     incondicionalmente glBlendEquation(GL_FUNC_ADD); o GlStateGuard deve restaurar a original.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h. PT: inclui GL/gl3w.h.
#include <cstdio>

// EN: Compile a trivial pass-through shader program for use as a bound dummy.
//     Returns a valid program id (> 0); caller owns deletion.
// PT: Compila shader trivial pass-through para uso como dummy bound.
//     Retorna id de programa válido (> 0); o chamador é dono da deleção.
static GLuint make_dummy_program() {
  const char* vs_src =
    "#version 330 core\nvoid main(){gl_Position=vec4(0,0,0,1);}\n";
  const char* fs_src =
    "#version 330 core\nout vec4 c;void main(){c=vec4(1,0,0,1);}\n";

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vs_src, nullptr);
  glCompileShader(vs);

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fs_src, nullptr);
  glCompileShader(fs);

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

int main() {
  // EN: Host creates a hidden window + current GL context.
  // PT: Host cria janela oculta + contexto GL corrente.
  glintfx::WindowGlfw host;
  if (!host.create("guard_host", 400, 300)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  // EN: UiLayer attaches (loads gl3w, initialises RmlUi).
  // PT: UiLayer anexa (carrega gl3w, inicializa RmlUi).
  glintfx::UiLayer ui({ .logical_width = 400, .logical_height = 300, .load_gl = true });
  if (!ui.ok()) {
    std::puts("FAIL: ui attach failed");
    return 2;
  }
  ui.load("tests/min.rml");

  // EN: Create a host FBO (render target) and bind it as both read + draw framebuffer.
  //     This also ensures GL_DRAW_FRAMEBUFFER_BINDING is non-zero so the restoration
  //     of the FBO binding is observable.
  // PT: Cria FBO do host (render target) e faz bind como framebuffer de leitura + escrita.
  //     Também garante que GL_DRAW_FRAMEBUFFER_BINDING seja não-zero, tornando a restauração
  //     do binding de FBO observável.
  gtest_off::Fbo fbo = gtest_off::make_fbo(400, 300);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // EN: Build dummy GL objects to occupy program + VAO bindings.
  // PT: Cria objetos GL dummy para ocupar os bindings de programa + VAO.
  GLuint prog = make_dummy_program();
  GLuint vao  = 0;
  glGenVertexArrays(1, &vao);

  // EN: Set a well-defined, non-default GL state that GlStateGuard must fully restore.
  //     Blend equation is set to GL_FUNC_SUBTRACT (non-default) to prove the guard
  //     catches the change made by RmlUi's BeginFrame() (which sets GL_FUNC_ADD).
  // PT: Define estado GL bem-definido e não-default que GlStateGuard deve restaurar por completo.
  //     Equação de blend definida como GL_FUNC_SUBTRACT (não-default) para provar que o guard
  //     captura a mudança feita pelo BeginFrame() do RmlUi (que define GL_FUNC_ADD).
  glUseProgram(prog);
  glBindVertexArray(vao);
  glViewport(1, 2, 300, 200);
  glEnable(GL_SCISSOR_TEST);
  glScissor(5, 6, 7, 8);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
  // EN: Non-default blend equations (default is GL_FUNC_ADD for both).
  // PT: Equações de blend não-default (padrão é GL_FUNC_ADD para ambas).
  glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT);

  // EN: Snapshot the state the GlStateGuard must restore.
  // PT: Snapshot do estado que o GlStateGuard deve restaurar.
  GLint     pre_vp[4], pre_sc[4];
  GLint     pre_blend, pre_scissor, pre_depth;
  GLint     pre_prog, pre_vao, pre_fbo;
  GLint     pre_beq_rgb, pre_beq_a;
  GLboolean pre_cmask[4];
  glGetIntegerv(GL_VIEWPORT,                  pre_vp);
  glGetIntegerv(GL_SCISSOR_BOX,               pre_sc);
  pre_blend   = (GLint)glIsEnabled(GL_BLEND);
  pre_scissor = (GLint)glIsEnabled(GL_SCISSOR_TEST);
  pre_depth   = (GLint)glIsEnabled(GL_DEPTH_TEST);
  glGetIntegerv(GL_CURRENT_PROGRAM,           &pre_prog);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING,      &pre_vao);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,  &pre_fbo);
  glGetIntegerv(GL_BLEND_EQUATION_RGB,        &pre_beq_rgb);
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA,      &pre_beq_a);
  glGetBooleanv(GL_COLOR_WRITEMASK,           pre_cmask);

  // EN: Render — GlStateGuard inside Engine::render_compose saves + restores state.
  //     RmlUi's BeginFrame() will call glBlendEquation(GL_FUNC_ADD), overriding our
  //     GL_FUNC_SUBTRACT; the guard must undo that and restore GL_FUNC_SUBTRACT.
  // PT: Render — GlStateGuard dentro de Engine::render_compose salva + restaura estado.
  //     BeginFrame() do RmlUi chama glBlendEquation(GL_FUNC_ADD), sobrescrevendo nosso
  //     GL_FUNC_SUBTRACT; o guard deve desfazer isso e restaurar GL_FUNC_SUBTRACT.
  ui.update();
  ui.render();

  // EN: Re-snapshot the state and compare each field to the pre-render snapshot.
  // PT: Re-snapshot do estado e compara cada campo com o snapshot pré-render.
  GLint     post_vp[4], post_sc[4];
  GLint     post_blend, post_scissor, post_depth;
  GLint     post_prog, post_vao, post_fbo;
  GLint     post_beq_rgb, post_beq_a;
  GLboolean post_cmask[4];
  glGetIntegerv(GL_VIEWPORT,                  post_vp);
  glGetIntegerv(GL_SCISSOR_BOX,               post_sc);
  post_blend   = (GLint)glIsEnabled(GL_BLEND);
  post_scissor = (GLint)glIsEnabled(GL_SCISSOR_TEST);
  post_depth   = (GLint)glIsEnabled(GL_DEPTH_TEST);
  glGetIntegerv(GL_CURRENT_PROGRAM,           &post_prog);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING,      &post_vao);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,  &post_fbo);
  glGetIntegerv(GL_BLEND_EQUATION_RGB,        &post_beq_rgb);
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA,      &post_beq_a);
  glGetBooleanv(GL_COLOR_WRITEMASK,           post_cmask);

  bool pass = true;

// EN: Helper macros — print a diff line and set pass=false if values diverge.
// PT: Macros auxiliares — imprimem linha de diff e definem pass=false se valores divergem.
#define CHK_I(name, pre, post)   \
  if ((pre) != (post)) {         \
    std::printf("FAIL: %s before=%d after=%d\n", (name), (int)(pre), (int)(post)); \
    pass = false;                \
  }
#define CHK_B(name, pre, post)   \
  if ((int)(pre) != (int)(post)) { \
    std::printf("FAIL: %s before=%d after=%d\n", (name), (int)(pre), (int)(post)); \
    pass = false;                \
  }

  CHK_I("GL_VIEWPORT[0]",              pre_vp[0],      post_vp[0]);
  CHK_I("GL_VIEWPORT[1]",              pre_vp[1],      post_vp[1]);
  CHK_I("GL_VIEWPORT[2]",              pre_vp[2],      post_vp[2]);
  CHK_I("GL_VIEWPORT[3]",              pre_vp[3],      post_vp[3]);
  CHK_I("GL_SCISSOR_BOX[0]",           pre_sc[0],      post_sc[0]);
  CHK_I("GL_SCISSOR_BOX[1]",           pre_sc[1],      post_sc[1]);
  CHK_I("GL_SCISSOR_BOX[2]",           pre_sc[2],      post_sc[2]);
  CHK_I("GL_SCISSOR_BOX[3]",           pre_sc[3],      post_sc[3]);
  CHK_I("GL_BLEND",                    pre_blend,      post_blend);
  CHK_I("GL_SCISSOR_TEST",             pre_scissor,    post_scissor);
  CHK_I("GL_DEPTH_TEST",               pre_depth,      post_depth);
  CHK_I("GL_CURRENT_PROGRAM",          pre_prog,       post_prog);
  CHK_I("GL_VERTEX_ARRAY_BINDING",     pre_vao,        post_vao);
  CHK_I("GL_DRAW_FRAMEBUFFER_BINDING", pre_fbo,        post_fbo);
  // EN: Blend equation — key assertion: guard must restore GL_FUNC_SUBTRACT that
  //     RmlUi's BeginFrame() overwrote with GL_FUNC_ADD.
  // PT: Equação de blend — asserção-chave: guard deve restaurar GL_FUNC_SUBTRACT que
  //     BeginFrame() do RmlUi sobrescreveu com GL_FUNC_ADD.
  CHK_I("GL_BLEND_EQUATION_RGB",       pre_beq_rgb,    post_beq_rgb);
  CHK_I("GL_BLEND_EQUATION_ALPHA",     pre_beq_a,      post_beq_a);
  CHK_B("GL_COLOR_WRITEMASK[0]",       pre_cmask[0],   post_cmask[0]);
  CHK_B("GL_COLOR_WRITEMASK[1]",       pre_cmask[1],   post_cmask[1]);
  CHK_B("GL_COLOR_WRITEMASK[2]",       pre_cmask[2],   post_cmask[2]);
  CHK_B("GL_COLOR_WRITEMASK[3]",       pre_cmask[3],   post_cmask[3]);

#undef CHK_I
#undef CHK_B

  // EN: Release GL objects before context teardown.
  // PT: Libera objetos GL antes do teardown do contexto.
  glDeleteProgram(prog);
  glDeleteVertexArrays(1, &vao);

  if (!pass) return 5;
  std::puts("gl_state_guard: PASS");
  return 0;
}
