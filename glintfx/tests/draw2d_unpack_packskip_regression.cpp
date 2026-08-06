// SPDX-License-Identifier: Apache-2.0
// EN: draw2d_unpack_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06, auditoria-dominó
//     twin of the PACK-side readback fix) -- `Draw2d::create_texture()` (draw2d.hpp), the
//     GENERAL caller-supplied-pixels upload path (`upload_gl_texture_raw()`, draw2d.cpp,
//     D2D-TEXPIXELS), reads its own `pixels` argument through `glTexImage2D`, which obeys
//     GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_PIXELS/GL_UNPACK_SKIP_ROWS -- the UNPACK-side mirror of
//     the GL_PACK_ROW_LENGTH/GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS that made
//     `capture_framebuffer()`'s own `glReadPixels` WRITE past its destination buffer
//     (frame_capture.cpp's own CAPTURE-PACKSKIP comment). Left non-zero by a cohabiting renderer
//     (RmlUi's own GL3 backend shares this context, D9's own class-doc comment in draw2d.hpp),
//     these three parameters make `glTexImage2D` READ past the end of the caller's `pixels`
//     buffer instead -- the mirror-image OUT-OF-BOUNDS READ, not a WRITE, but the SAME class of
//     bug (this function's own promised contract, "reads exactly this many bytes from the
//     caller's buffer", silently broken by state the caller never sees).
//
//     PROOF METHOD (content correctness, not just "no crash" -- an OOB read on a small heap
//     allocation may not always fault/crash, unlike the PACK-side WRITE this ticket's sibling
//     test proves by process survival; this test instead proves the UPLOADED PIXELS THEMSELVES
//     are correct despite the dirty state, which an OOB read that silently succeeds would still
//     get WRONG): create a small, solid, KNOWN-colour RGBA8 buffer; dirty
//     GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_PIXELS/GL_UNPACK_SKIP_ROWS; call
//     `Draw2d::create_texture(..., PixelFormat::Rgba8)`; draw the resulting texture full-screen;
//     read the backbuffer back and assert it is STILL the exact known colour. If the dirty state
//     leaked through unguarded, the driver would read the wrong byte range (out of bounds, or
//     simply the wrong in-bounds offset for a buffer this small) and the readback would show a
//     WRONG colour (garbage or all-zero), not necessarily a crash -- this is why a content check,
//     not a survival check, is the right oracle for THIS half of the domino pair. A second block
//     repeats the same proof for `PixelFormat::R8` (the atlas/glyph-coverage path,
//     `upload_gl_texture_raw()`'s OTHER branch), tinted through the SAME D8 swizzle-to-RRRR
//     formula `draw2d_pixels_flush_render_sanity.cpp`'s own checks 3/4 already use.
// PT: draw2d_unpack_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06, gêmeo de
//     auditoria-dominó do conserto de readback do lado PACK) -- `Draw2d::create_texture()`
//     (draw2d.hpp), o caminho de upload GERAL de pixels fornecidos pelo chamador
//     (`upload_gl_texture_raw()`, draw2d.cpp, D2D-TEXPIXELS), lê o próprio argumento `pixels`
//     através do `glTexImage2D`, que obedece GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_PIXELS/
//     GL_UNPACK_SKIP_ROWS -- o espelho do lado UNPACK do GL_PACK_ROW_LENGTH/GL_PACK_SKIP_PIXELS/
//     GL_PACK_SKIP_ROWS que fazia o próprio glReadPixels de `capture_framebuffer()` ESCREVER além
//     do próprio buffer de destino (o próprio comentário CAPTURE-PACKSKIP de frame_capture.cpp).
//     Deixados não-zero por um renderer coabitante (o próprio backend GL3 do RmlUi compartilha
//     este contexto, o próprio comentário de classe do D9 em draw2d.hpp), estes três parâmetros
//     fazem o `glTexImage2D` LER além do fim do buffer `pixels` do chamador em vez disso -- a
//     LEITURA fora dos limites com imagem espelhada, não uma ESCRITA, mas a MESMA classe de bug
//     (o próprio contrato prometido desta função, "lê exatamente esta quantidade de bytes do
//     buffer do chamador", quebrado em silêncio por estado que o chamador nunca vê).
//
//     MÉTODO DE PROVA (correção de conteúdo, não só "não crasha" -- uma leitura fora dos limites
//     numa alocação de heap pequena pode nem sempre falhar/crashar, diferente da ESCRITA do lado
//     PACK que o teste irmão desta ticket prova por sobrevivência de processo; este teste em vez
//     disso prova que os PRÓPRIOS PIXELS ENVIADOS estão corretos apesar do estado sujo, o que uma
//     leitura fora dos limites que sucede em silêncio ainda erraria): cria um buffer RGBA8
//     pequeno, sólido, de cor CONHECIDA; suja GL_UNPACK_ROW_LENGTH/GL_UNPACK_SKIP_PIXELS/
//     GL_UNPACK_SKIP_ROWS; chama `Draw2d::create_texture(..., PixelFormat::Rgba8)`; desenha a
//     textura resultante em tela cheia; lê o backbuffer de volta e afirma que AINDA é a cor
//     conhecida exata. Se o estado sujo vazasse sem guarda, o driver leria a faixa de byte errada
//     (fora dos limites, ou simplesmente o offset errado dentro dos limites pra um buffer tão
//     pequeno) e o readback mostraria uma cor ERRADA (lixo ou tudo-zero), não necessariamente um
//     crash -- por isso uma checagem de conteúdo, não de sobrevivência, é o oráculo certo pra
//     ESTA metade do par dominó. Um segundo bloco repete a mesma prova pro `PixelFormat::R8` (o
//     caminho de atlas/cobertura-de-glifo, a OUTRA ramificação de `upload_gl_texture_raw()`),
//     tingido pela MESMA fórmula de swizzle-pra-RRRR do D8 que os próprios checks 3/4 de
//     draw2d_pixels_flush_render_sanity.cpp já usam.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <cmath>
#include <cstdio>
#include <vector>

using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::Texture2d;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

std::vector<unsigned char> read_backbuffer_rgb(int w, int h) {
  std::vector<unsigned char> raw(static_cast<std::size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, raw.data());
  return raw;
}

struct Rgb {
  double r = 0, g = 0, b = 0;
};

Rgb region_mean(const std::vector<unsigned char>& px, int w, int cx, int cy, int half) {
  Rgb sum;
  int n = 0;
  for (int y = cy - half; y <= cy + half; ++y) {
    for (int x = cx - half; x <= cx + half; ++x) {
      const std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 3;
      sum.r += px[idx + 0];
      sum.g += px[idx + 1];
      sum.b += px[idx + 2];
      ++n;
    }
  }
  if (n > 0) {
    sum.r /= n;
    sum.g /= n;
    sum.b /= n;
  }
  return sum;
}

bool near_rgb(const Rgb& got, double r, double g, double b, double tol) {
  return std::fabs(got.r - r) <= tol && std::fabs(got.g - g) <= tol && std::fabs(got.b - b) <= tol;
}

std::vector<unsigned char> build_rgba8_solid(int w, int h, unsigned char r, unsigned char g,
                                             unsigned char b) {
  std::vector<unsigned char> px(static_cast<std::size_t>(w) * h * 4);
  for (int i = 0; i < w * h; ++i) {
    px[static_cast<std::size_t>(i) * 4 + 0] = r;
    px[static_cast<std::size_t>(i) * 4 + 1] = g;
    px[static_cast<std::size_t>(i) * 4 + 2] = b;
    px[static_cast<std::size_t>(i) * 4 + 3] = 255;
  }
  return px;
}

std::vector<unsigned char> build_r8_solid(int w, int h, unsigned char coverage) {
  return std::vector<unsigned char>(static_cast<std::size_t>(w) * h, coverage);
}

} // namespace

int main() {
  constexpr int W = 64, H = 64;
  constexpr double kTol = 8.0; // llvmpipe/blend-rounding tolerance, same order as the sibling file.

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_unpack_packskip_host", W, H)) {
    std::puts("draw2d_unpack_packskip_regression FAIL: host window create failed");
    return 1;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_unpack_packskip_regression FAIL: Draw2d::init() failed");
    return 2;
  }
  glViewport(0, 0, W, H);

  // -------------------------------------------------------------------------------------------
  // Case 1: PixelFormat::Rgba8, GL_UNPACK_ROW_LENGTH/SKIP_PIXELS/SKIP_ROWS dirtied BEFORE
  // create_texture() -- must still upload the exact known colour (200,100,50).
  // -------------------------------------------------------------------------------------------
  {
    const std::vector<unsigned char> raw = build_rgba8_solid(8, 8, 200, 100, 50);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 37);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 5);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 3);

    Texture2d tex = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::Rgba8);
    check(tex.ok() && tex.width() == 8 && tex.height() == 8,
          "rgba8_fixture: 8x8 buffer decoded via create_texture(Rgba8) despite dirty UNPACK state");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)});
    d2d.end();
    const Rgb mean = region_mean(read_backbuffer_rgb(W, H), W, W / 2, H / 2, 8);
    std::printf(
        "draw2d_unpack_packskip_regression: Rgba8 mean=(%.1f,%.1f,%.1f), expect ~(200,100,50)\n",
        mean.r, mean.g, mean.b);
    check(near_rgb(mean, 200, 100, 50, kTol),
          "unpack_packskip_rgba8: create_texture(Rgba8) uploaded the CORRECT bytes despite dirty "
          "GL_UNPACK_ROW_LENGTH/SKIP_PIXELS/SKIP_ROWS left by a cohabiting renderer");

    d2d.destroy_texture(tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  }

  // -------------------------------------------------------------------------------------------
  // Case 2: PixelFormat::R8 (atlas/glyph-coverage path), same dirty UNPACK trio -- must still
  // tint to the exact known colour via the D8 swizzle-to-RRRR formula (coverage=200, red tint,
  // black background -> ~(200,0,0), same derivation draw2d_pixels_flush_render_sanity.cpp's own
  // check 3 uses).
  // -------------------------------------------------------------------------------------------
  {
    const std::vector<unsigned char> raw = build_r8_solid(8, 8, 200);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 37);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 5);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 3);

    Texture2d tex = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::R8);
    check(tex.ok() && tex.width() == 8 && tex.height() == 8,
          "r8_fixture: 8x8 coverage buffer decoded via create_texture(R8) despite dirty UNPACK "
          "state");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)}, RectF{},
                    ColorF{1.f, 0.f, 0.f, 1.f});
    d2d.end();
    const Rgb mean = region_mean(read_backbuffer_rgb(W, H), W, W / 2, H / 2, 8);
    std::printf("draw2d_unpack_packskip_regression: R8 mean=(%.1f,%.1f,%.1f), expect ~(200,0,0)\n",
                mean.r, mean.g, mean.b);
    check(near_rgb(mean, 200, 0, 0, kTol),
          "unpack_packskip_r8: create_texture(R8) uploaded the CORRECT coverage bytes despite "
          "dirty GL_UNPACK_ROW_LENGTH/SKIP_PIXELS/SKIP_ROWS left by a cohabiting renderer");

    d2d.destroy_texture(tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  }

  d2d.shutdown();

  if (g_failures == 0) {
    std::puts("draw2d_unpack_packskip_regression: PASS");
    return 0;
  }
  std::printf("draw2d_unpack_packskip_regression: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
