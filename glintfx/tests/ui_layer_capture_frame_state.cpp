// SPDX-License-Identifier: Apache-2.0
// EN: `FRAMEGRAB-EMBED` -- verifies that UiLayer::capture_frame() leaves every GL state
//     variable it touches unchanged on return. Same discipline as gl_state_guard.cpp (which
//     covers render()'s own GlStateGuard) but scoped to the DIFFERENT 5 pieces of state a
//     plain pixel readback perturbs (engine.hpp's own Engine::capture_frame doc-comment):
//     GL_READ_FRAMEBUFFER binding, GL_READ_BUFFER, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, and
//     the GL_PIXEL_PACK_BUFFER binding -- NONE of which render()'s own GlStateGuard captures
//     (it saves the render-PASS state: viewport/blend/program/VAO/etc), so this is a genuinely
//     separate proof, not a duplicate of gl_state_guard.cpp.
//
//     SENTINELS (deliberately non-default, so a broken restore is observable): a SECOND host
//     FBO (gtest_off::make_fbo) bound as GL_READ_FRAMEBUFFER (the DRAW framebuffer stays the
//     window, FBO 0, unaffected -- capture_frame() only touches the READ target) with
//     GL_READ_BUFFER=GL_NONE (a user FBO's own default read-buffer is
//     GL_COLOR_ATTACHMENT0, so GL_NONE is a genuine non-default value, same sentinel
//     technique gl_state_guard.cpp already uses for the SAME reason); a bound
//     GL_PIXEL_PACK_BUFFER (a fresh, empty PBO -- the GL default is 0/unbound);
//     GL_PACK_ALIGNMENT=2 and GL_PACK_ROW_LENGTH=37 (GL defaults are 4 and 0).
//     capture_frame() is called AFTER all five sentinels are set -- it MUST still succeed
//     (ok()==true, reading FBO 0/GL_BACK internally regardless of the caller's pre-existing
//     read-target mess), and every one of the five values must read back IDENTICAL
//     afterward.
//
//     MUTATION PROOF: during FRAMEGRAB-EMBED implementation, the GL_PACK_ROW_LENGTH
//     save/restore pair in Engine::capture_frame was temporarily commented out and this file
//     was rebuilt and re-run -- it went RED (post != pre for GL_PACK_ROW_LENGTH, all four
//     other slots stayed green, proving the checks are independent and each one actually
//     exercises its own slot). Restoring the pair and rebuilding turned it back GREEN.
// PT: `FRAMEGRAB-EMBED` -- verifica que UiLayer::capture_frame() não altera nenhuma variável
//     de estado GL que toca, ao retornar. Mesma disciplina de gl_state_guard.cpp (que cobre o
//     próprio GlStateGuard do render()) mas restrita às 5 peças de estado DIFERENTES que um
//     readback de pixel puro perturba (o próprio doc-comment de Engine::capture_frame,
//     engine.hpp): binding de GL_READ_FRAMEBUFFER, GL_READ_BUFFER, GL_PACK_ALIGNMENT,
//     GL_PACK_ROW_LENGTH, e o binding de GL_PIXEL_PACK_BUFFER -- NENHUM dos quais o próprio
//     GlStateGuard do render() captura (ele salva o estado do PASSE de render:
//     viewport/blend/programa/VAO/etc), então esta é uma prova genuinamente separada, não uma
//     duplicata de gl_state_guard.cpp.
//
//     SENTINELAS (deliberadamente não-default, para que uma restauração quebrada seja
//     observável): um SEGUNDO FBO do host (gtest_off::make_fbo) vinculado como
//     GL_READ_FRAMEBUFFER (o framebuffer de DESENHO continua sendo a janela, FBO 0, intocado
//     -- capture_frame() só toca o alvo de LEITURA) com GL_READ_BUFFER=GL_NONE (o próprio
//     default de read-buffer de um FBO de usuário é GL_COLOR_ATTACHMENT0, então GL_NONE é um
//     valor genuinamente não-default, mesma técnica de sentinela que gl_state_guard.cpp já
//     usa pelo mesmo motivo); um GL_PIXEL_PACK_BUFFER vinculado (um PBO novo, vazio -- o
//     default do GL é 0/desvinculado); GL_PACK_ALIGNMENT=2 e GL_PACK_ROW_LENGTH=37 (os
//     defaults do GL são 4 e 0). capture_frame() é chamado DEPOIS de todas as cinco
//     sentinelas definidas -- DEVE ainda ter sucesso (ok()==true, lendo o FBO 0/GL_BACK
//     internamente independente da bagunça de alvo-de-leitura pré-existente do chamador), e
//     cada um dos cinco valores deve ler de volta IDÊNTICO depois.
//
//     PROVA DE MUTAÇÃO: durante a implementação do FRAMEGRAB-EMBED, o par salvar/restaurar de
//     GL_PACK_ROW_LENGTH em Engine::capture_frame foi temporariamente comentado e este
//     arquivo foi rebuildado e re-rodado -- ficou VERMELHO (post != pre para
//     GL_PACK_ROW_LENGTH, as outras quatro sentinelas continuaram verdes, provando que as
//     checagens são independentes e cada uma de fato exercita a própria sentinela).
//     Restaurar o par e rebuildar a trouxe de volta a VERDE.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h. PT: inclui gl_loader.h.
#include <cstdio>

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_state_host", 200, 150)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = 200, .logical_height = 150, .load_gl = true});
  if (!ui.ok()) {
    std::puts("FAIL: ui attach failed");
    return 2;
  }
  ui.load("tests/min.rml");
  ui.update();
  ui.render(); // EN: warm-up pass, same as gl_state_guard.cpp. PT: passada de aquecimento.

  // EN: Second host FBO -- bound as READ (not draw) target, sentinel read-buffer GL_NONE.
  // PT: Segundo FBO do host -- vinculado como alvo de LEITURA (não desenho), read-buffer
  //     sentinela GL_NONE.
  gtest_off::Fbo read_fbo = gtest_off::make_fbo(64, 64);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo.fbo);
  glReadBuffer(GL_NONE);

  // EN: Fresh, empty pixel-pack buffer object, bound.
  // PT: Objeto de pixel-pack buffer novo, vazio, vinculado.
  GLuint pbo = 0;
  glGenBuffers(1, &pbo);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);

  // EN: Non-default pack alignment/row-length sentinels.
  // PT: Sentinelas não-default de alinhamento/comprimento-de-linha de pack.
  glPixelStorei(GL_PACK_ALIGNMENT, 2);
  glPixelStorei(GL_PACK_ROW_LENGTH, 37);

  GLint pre_read_fbo = 0, pre_read_buffer = 0;
  GLint pre_pack_alignment = 0, pre_pack_row_length = 0, pre_pack_pbo = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &pre_read_fbo);
  glGetIntegerv(GL_READ_BUFFER, &pre_read_buffer);
  glGetIntegerv(GL_PACK_ALIGNMENT, &pre_pack_alignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &pre_pack_row_length);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pre_pack_pbo);

  // EN: The call under test -- must succeed despite the caller's pre-existing read-target
  //     mess (it forces GL_READ_FRAMEBUFFER=0/GL_BACK internally for the duration of the
  //     read, then restores all five sentinels).
  // PT: A chamada sob teste -- deve ter sucesso apesar da bagunça pré-existente de
  //     alvo-de-leitura do chamador (ela força GL_READ_FRAMEBUFFER=0/GL_BACK internamente
  //     pela duração da leitura, depois restaura as cinco sentinelas).
  const glintfx::UiLayer::CapturedFrame frame = ui.capture_frame();

  GLint post_read_fbo = 0, post_read_buffer = 0;
  GLint post_pack_alignment = 0, post_pack_row_length = 0, post_pack_pbo = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &post_read_fbo);
  glGetIntegerv(GL_READ_BUFFER, &post_read_buffer);
  glGetIntegerv(GL_PACK_ALIGNMENT, &post_pack_alignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &post_pack_row_length);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &post_pack_pbo);

  bool pass = true;

  if (!frame.ok) {
    std::puts("FAIL: capture_frame() returned ok == false despite a valid 200x150 viewport");
    pass = false;
  } else if (frame.width != 200 || frame.height != 150) {
    std::fprintf(stderr, "FAIL: size %dx%d, expected 200x150\n", frame.width, frame.height);
    pass = false;
  }

#define CHK_I(name, pre, post)                                                     \
  if ((pre) != (post)) {                                                           \
    std::printf("FAIL: %s before=%d after=%d\n", (name), (int)(pre), (int)(post)); \
    pass = false;                                                                  \
  }

  CHK_I("GL_READ_FRAMEBUFFER_BINDING", pre_read_fbo, post_read_fbo);
  CHK_I("GL_READ_BUFFER", pre_read_buffer, post_read_buffer);
  CHK_I("GL_PACK_ALIGNMENT", pre_pack_alignment, post_pack_alignment);
  CHK_I("GL_PACK_ROW_LENGTH", pre_pack_row_length, post_pack_row_length);
  CHK_I("GL_PIXEL_PACK_BUFFER_BINDING", pre_pack_pbo, post_pack_pbo);

#undef CHK_I

  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glDeleteBuffers(1, &pbo);

  if (!pass) return 3;
  std::puts("ui_layer_capture_frame_state: PASS");
  return 0;
}
