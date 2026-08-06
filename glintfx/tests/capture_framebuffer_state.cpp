// SPDX-License-Identifier: Apache-2.0
// EN: capture_framebuffer_state (CAPTURE-FREE, W22 S8, 2026-07-30) -- verifies that
//     glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp) leaves every
//     GL state variable it touches unchanged on return. Same discipline, same 5 sentinelled
//     slots, as ui_layer_capture_frame_state.cpp (FRAMEGRAB-EMBED) -- GL_READ_FRAMEBUFFER
//     binding, GL_READ_BUFFER, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PIXEL_PACK_BUFFER
//     binding -- but this file needs NO UiLayer/RmlUi attach step at all: capture_framebuffer()
//     needs only a GL context already current, so a plain WindowGlfw host is the entire
//     fixture, proving the state-hygiene contract holds for the instance-free entry point too,
//     not just its two instance-method siblings.
//
//     SENTINELS: identical technique and identical non-default values to
//     ui_layer_capture_frame_state.cpp, for the same reason (a broken restore must be
//     observable): a second host FBO bound as GL_READ_FRAMEBUFFER with GL_READ_BUFFER=GL_NONE
//     (a user FBO's own default is GL_COLOR_ATTACHMENT0); a bound, empty GL_PIXEL_PACK_BUFFER
//     (GL default is 0/unbound); GL_PACK_ALIGNMENT=2, GL_PACK_ROW_LENGTH=37 (GL defaults are 4
//     and 0). capture_framebuffer() is called AFTER all five sentinels are set -- it MUST
//     still succeed (reading FBO 0/GL_BACK internally regardless of the caller's pre-existing
//     read-target mess, frame_capture.cpp's own top comment), and every one of the five values
//     must read back IDENTICAL afterward.
// PT: capture_framebuffer_state (CAPTURE-FREE, W22 S8, 2026-07-30) -- verifica que
//     glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp) não altera
//     nenhuma variável de estado GL que toca, ao retornar. Mesma disciplina, mesmos 5 slots
//     sentinelados, de ui_layer_capture_frame_state.cpp (FRAMEGRAB-EMBED) -- binding de
//     GL_READ_FRAMEBUFFER, GL_READ_BUFFER, GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, binding de
//     GL_PIXEL_PACK_BUFFER -- mas este arquivo não precisa de passo de attach de
//     UiLayer/RmlUi nenhum: capture_framebuffer() só precisa de um contexto GL já corrente,
//     então um host WindowGlfw simples é a fixture inteira, provando que o contrato de higiene
//     de estado vale pro ponto de entrada sem instância também, não só pras duas irmãs de
//     método de instância dela.
//
//     SENTINELAS: mesma técnica e mesmos valores não-default de
//     ui_layer_capture_frame_state.cpp, pelo mesmo motivo (uma restauração quebrada precisa
//     ser observável): um segundo FBO do host vinculado como GL_READ_FRAMEBUFFER com
//     GL_READ_BUFFER=GL_NONE (o próprio default de um FBO de usuário é
//     GL_COLOR_ATTACHMENT0); um GL_PIXEL_PACK_BUFFER vinculado, vazio (o default do GL é
//     0/desvinculado); GL_PACK_ALIGNMENT=2, GL_PACK_ROW_LENGTH=37 (os defaults do GL são 4 e
//     0). capture_framebuffer() é chamado DEPOIS de todas as cinco sentinelas definidas --
//     DEVE ainda ter sucesso (lendo o FBO 0/GL_BACK internamente independente da bagunça de
//     alvo-de-leitura pré-existente do chamador, o próprio comentário de topo de
//     frame_capture.cpp), e cada um dos cinco valores deve ler de volta IDÊNTICO depois.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h, provides gtest_off::make_fbo. PT: inclui gl_loader.h, fornece gtest_off::make_fbo.
#include <cstdio>

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("capture_framebuffer_state_host", 200, 150)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT); // EN: warm-up paint. PT: pintura de aquecimento.

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
  // EN: CAPTURE-PACKSKIP (W28, 2026-08-06) -- two MORE non-default sentinels (GL defaults for
  //     both are 0), the two GL_PACK_* parameters this fix added to the save-neutralize-restore
  //     block (frame_capture.cpp's own CAPTURE-PACKSKIP comment). This is the state-HYGIENE half
  //     of the proof (does a value survive the round trip); it does NOT by itself prove the
  //     heap-corruption bug these two parameters caused before this fix was ever present --
  //     capture_framebuffer_packskip_regression.cpp is the file that proves THAT (a state-restore
  //     check cannot: the pre-fix code never touched these two variables AT ALL, so "post == pre"
  //     would have trivially held even with the guard completely absent).
  // PT: CAPTURE-PACKSKIP (W28, 2026-08-06) -- duas sentinelas A MAIS não-default (os defaults do
  //     GL pros dois são 0), os dois parâmetros GL_PACK_* que este conserto somou ao bloco de
  //     salvar-neutralizar-restaurar (o próprio comentário CAPTURE-PACKSKIP de frame_capture.cpp).
  //     Esta é a metade de HIGIENE DE ESTADO da prova (um valor sobrevive à ida-e-volta); ela NÃO
  //     prova sozinha o bug de corrupção de heap que estes dois parâmetros causavam antes deste
  //     conserto sequer existir -- capture_framebuffer_packskip_regression.cpp é o arquivo que
  //     prova AQUILO (uma checagem de restauração de estado não consegue: o código pré-conserto
  //     nunca tocava essas duas variáveis DE JEITO NENHUM, então "post == pre" teria valido
  //     trivialmente mesmo com a guarda completamente ausente).
  glPixelStorei(GL_PACK_SKIP_PIXELS, 11);
  glPixelStorei(GL_PACK_SKIP_ROWS, 13);

  GLint pre_read_fbo = 0, pre_read_buffer = 0;
  GLint pre_pack_alignment = 0, pre_pack_row_length = 0, pre_pack_pbo = 0;
  GLint pre_pack_skip_pixels = 0, pre_pack_skip_rows = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &pre_read_fbo);
  glGetIntegerv(GL_READ_BUFFER, &pre_read_buffer);
  glGetIntegerv(GL_PACK_ALIGNMENT, &pre_pack_alignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &pre_pack_row_length);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pre_pack_pbo);
  glGetIntegerv(GL_PACK_SKIP_PIXELS, &pre_pack_skip_pixels);
  glGetIntegerv(GL_PACK_SKIP_ROWS, &pre_pack_skip_rows);

  // EN: The call under test -- must succeed despite the caller's pre-existing read-target
  //     mess (it forces GL_READ_FRAMEBUFFER=0/GL_BACK internally for the duration of the
  //     read, then restores all five sentinels).
  // PT: A chamada sob teste -- deve ter sucesso apesar da bagunça pré-existente de
  //     alvo-de-leitura do chamador (ela força GL_READ_FRAMEBUFFER=0/GL_BACK internamente
  //     pela duração da leitura, depois restaura as cinco sentinelas).
  const glintfx::CapturedFramebuffer frame = glintfx::capture_framebuffer(0, 0, 200, 150);

  GLint post_read_fbo = 0, post_read_buffer = 0;
  GLint post_pack_alignment = 0, post_pack_row_length = 0, post_pack_pbo = 0;
  GLint post_pack_skip_pixels = 0, post_pack_skip_rows = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &post_read_fbo);
  glGetIntegerv(GL_READ_BUFFER, &post_read_buffer);
  glGetIntegerv(GL_PACK_ALIGNMENT, &post_pack_alignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &post_pack_row_length);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &post_pack_pbo);
  glGetIntegerv(GL_PACK_SKIP_PIXELS, &post_pack_skip_pixels);
  glGetIntegerv(GL_PACK_SKIP_ROWS, &post_pack_skip_rows);

  bool pass = true;

  if (!frame.ok) {
    std::puts("FAIL: capture_framebuffer() returned ok == false despite a valid 200x150 read");
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
  CHK_I("GL_PACK_SKIP_PIXELS", pre_pack_skip_pixels, post_pack_skip_pixels);
  CHK_I("GL_PACK_SKIP_ROWS", pre_pack_skip_rows, post_pack_skip_rows);

#undef CHK_I

  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glDeleteBuffers(1, &pbo);

  if (!pass) return 3;
  std::puts("capture_framebuffer_state: PASS");
  return 0;
}
