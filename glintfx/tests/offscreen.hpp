// SPDX-License-Identifier: MPL-2.0
// EN: Minimal offscreen FBO helpers for GL tests: create a plain RGBA8 FBO backed by a
//     texture, and read its contents back as tightly-packed RGB bytes.
//     The caller is responsible for having a current GL context (e.g. via WindowGlfw::create)
//     and for calling glx_gl_load() before using these helpers.
// PT: Helpers mínimos de FBO offscreen para testes GL: cria FBO RGBA8 com backing de textura
//     e lê o conteúdo de volta como bytes RGB sem padding.
//     O chamador é responsável por ter um contexto GL corrente (ex.: via WindowGlfw::create)
//     e por chamar glx_gl_load() antes de usar esses helpers.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include "gl_loader.h"
#include <vector>

namespace gtest_off {

// EN: Offscreen framebuffer descriptor. All fields are zeroed on construction.
// PT: Descritor de framebuffer offscreen. Todos os campos são zero na construção.
struct Fbo {
  GLuint fbo = 0;
  GLuint tex = 0;
  int    w   = 0;
  int    h   = 0;
};

// EN: Allocate a colour-only RGBA8 FBO of the requested dimensions.
//     The FBO is left bound (GL_FRAMEBUFFER) on return.
// PT: Aloca FBO RGBA8 somente-cor das dimensões pedidas.
//     O FBO fica bound (GL_FRAMEBUFFER) ao retornar.
inline Fbo make_fbo(int w, int h) {
  Fbo f;
  f.w = w;
  f.h = h;

  glGenTextures(1, &f.tex);
  glBindTexture(GL_TEXTURE_2D, f.tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenFramebuffers(1, &f.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, f.tex, 0);

  return f;
}

// EN: Read back the FBO colour attachment as tightly-packed RGB (3 bytes per pixel, row-major).
//     The FBO is bound as GL_FRAMEBUFFER (read + draw) before the readback.
// PT: Lê o colour attachment do FBO como RGB sem padding (3 bytes por pixel, row-major).
//     O FBO é bound como GL_FRAMEBUFFER (leitura + escrita) antes do readback.
inline void read_rgb(const Fbo& f, std::vector<unsigned char>& px) {
  px.resize(static_cast<size_t>(f.w) * f.h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, f.w, f.h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
}

} // namespace gtest_off
