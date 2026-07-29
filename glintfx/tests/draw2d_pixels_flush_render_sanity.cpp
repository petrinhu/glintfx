// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_pixels_flush_render_sanity (D2D-TEXPIXELS / D2D-FLUSH) -- pixel-readback proof of
//     `Draw2d::create_texture`/`Draw2d::flush` under Xvfb/llvmpipe, STATISTICAL/tolerant (never
//     pixel-exact -- the house's own `golden_test` precedent, AGENTS.md's "golden_test is flaky
//     on llvmpipe" gotcha, same discipline draw2d_render_sanity.cpp/
//     draw2d_camera_render_sanity.cpp already follow). Embed fixture: a hidden `WindowGlfw`
//     provides the current GL context, no `UiLayer`/RmlUi involved. ONE window, 128x128, created
//     once and reused for every check below (same idiom as the sibling files above).
//
//     Eight checks:
//       1. create_texture(Rgba8) solid-colour readback EQUALS the SAME colour loaded via
//          load_texture() from an equivalent TGA -- the D2D-TEXPIXELS equivalence claim
//          ("a texture created by pixels must render identically to the same image loaded by
//          path"), proven by comparing BOTH readbacks against the same hand-known colour AND
//          against each other.
//       2. create_texture(Rgba8) premultiply -- a semi-transparent STRAIGHT-alpha pixel buffer
//          blended over a known background matches the SAME D8 hand-derived delta
//          draw2d_render_sanity.cpp's own check 2 uses (proving create_texture() premultiplies
//          on ingest exactly like load_texture() does, not "looks blended").
//       3/4. create_texture(R8) -- a solid-coverage buffer tinted a pure colour matches the D8
//          formula applied to a coverage texel (proves the GL_R8 + swizzle-to-RRRR path is
//          wired exactly like the font-glyph atlas already relies on); a ZERO-coverage buffer is
//          fully invisible (the background shows through unchanged), proving R8's own alpha IS
//          the coverage value, not a constant.
//       5. flush() forces GL work WITHOUT closing the bracket: a readback taken BEFORE end()
//          (right after flush()) already shows the first draw, and a SECOND draw issued in the
//          SAME still-open bracket after flush() also lands correctly once end() runs -- proving
//          both halves of the D2D-FLUSH contract (pushed to GL now; bracket still usable after).
//       6. flush() leaves the CURRENT camera intact: two world-space draws at the SAME world
//          rect, straddling a flush(), both project to the SAME screen position -- if the camera
//          had been reset by flush() (screen-space fallback), the second draw's dst would be
//          read as raw (tiny, near-origin) screen coordinates instead, landing far from centre.
//       7. flush() leaves the CURRENT scissor intact in STREAMING mode: a scissor clips a filled
//          rect to the LEFT half of the viewport; a second, differently-coloured filled rect
//          issued AFTER a flush() is STILL clipped to the left half when end() finally runs -- if
//          the scissor had been reset by flush(), the second rect would cover the WHOLE viewport
//          instead.
//       8. flush() leaves the CURRENT scissor intact in BUFFERED mode (set_layer() armed) too --
//          its OWN code path (Impl::replay_layer_queue()'s per-group loop), not exercised by
//          check 7: scissor changes to a DIFFERENT rect between the buffered draw and the
//          flush() that replays it; a fill pushed AFTER flush() must capture the scissor the
//          caller actually has CURRENT, not a stale snapshot the replay's own loop left behind.
// PT: draw2d_pixels_flush_render_sanity (D2D-TEXPIXELS / D2D-FLUSH) -- prova de pixel-readback
//     de `Draw2d::create_texture`/`Draw2d::flush` sob Xvfb/llvmpipe, ESTATÍSTICA/tolerante
//     (nunca pixel-exata -- o próprio precedente do `golden_test` da casa, o gotcha
//     "golden_test é flaky no llvmpipe" do AGENTS.md, mesma disciplina que
//     draw2d_render_sanity.cpp/draw2d_camera_render_sanity.cpp já seguem). Fixture embed: um
//     `WindowGlfw` oculto fornece o contexto GL corrente, nenhum `UiLayer`/RmlUi envolvido. UMA
//     janela, 128x128, criada uma vez e reusada em todo check abaixo (mesmo idioma dos arquivos
//     irmãos acima).
//
//     Sete checks:
//       1. readback de cor sólida do create_texture(Rgba8) É IGUAL à MESMA cor carregada via
//          load_texture() a partir de um TGA equivalente -- a alegação de equivalência do
//          D2D-TEXPIXELS ("uma textura criada por pixels precisa renderizar identicamente à
//          mesma imagem carregada por caminho"), provada comparando OS DOIS readbacks contra a
//          mesma cor conhecida à mão E um contra o outro.
//       2. premultiply do create_texture(Rgba8) -- um buffer de pixel alpha STRAIGHT
//          semi-transparente misturado sobre um fundo conhecido bate com o MESMO delta
//          derivado-à-mão D8 que o próprio check 2 de draw2d_render_sanity.cpp usa (provando que
//          o create_texture() premultiplica na entrada exatamente como o load_texture() faz, não
//          "parece misturado").
//       3/4. create_texture(R8) -- um buffer de cobertura sólida tingido de uma cor pura bate com
//          a fórmula D8 aplicada a um texel de cobertura (prova que o caminho GL_R8 +
//          swizzle-pra-RRRR está fiado exatamente como o atlas de glifo de fonte já depende);
//          um buffer de cobertura ZERO é totalmente invisível (o fundo aparece sem alteração),
//          provando que o próprio alpha do R8 É o valor de cobertura, não uma constante.
//       5. flush() força trabalho GL SEM fechar o bracket: um readback tirado ANTES do end()
//          (logo depois do flush()) já mostra o primeiro desenho, e um SEGUNDO desenho emitido no
//          MESMO bracket ainda aberto depois do flush() também pousa corretamente quando o end()
//          finalmente roda -- provando as duas metades do contrato D2D-FLUSH (empurrado pra GL
//          agora; bracket ainda usável depois).
//       6. flush() deixa a câmera CORRENTE intacta: dois desenhos em espaço de mundo no MESMO
//          retângulo de mundo, um de cada lado de um flush(), os dois projetam pra MESMA posição
//          de tela -- se a câmera tivesse sido resetada pelo flush() (fallback pra espaço de
//          tela), o dst do segundo desenho seria lido como coordenadas de tela cruas (minúsculas,
//          perto da origem) em vez disso, pousando longe do centro.
//       7. flush() deixa o scissor CORRENTE intacto em modo STREAMING: um scissor recorta um
//          retângulo preenchido pra METADE ESQUERDA do viewport; um segundo retângulo
//          preenchido, de cor diferente, emitido DEPOIS de um flush() AINDA está recortado pra
//          metade esquerda quando o end() finalmente roda -- se o scissor tivesse sido
//          resetado pelo flush(), o segundo retângulo cobriria o viewport INTEIRO em vez disso.
//       8. flush() deixa o scissor CORRENTE intacto em modo BUFFERIZADO (set_layer() armado)
//          também -- o PRÓPRIO caminho de código dele (o loop por-grupo do
//          Impl::replay_layer_queue()), não exercitado pelo check 7: o scissor muda pra um
//          retângulo DIFERENTE entre o desenho bufferizado e o flush() que o reproduz; um
//          preenchimento empurrado DEPOIS do flush() precisa capturar o scissor que o chamador
//          de fato tem CORRENTE, não um snapshot obsoleto que o próprio loop do replay deixou
//          pra trás.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using glintfx::Camera2d;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::SpriteTransform;
using glintfx::Texture2d;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Same builder as draw2d_render_sanity.cpp -- uncompressed 24bpp TGA, top-down.
// PT: Mesmo builder de draw2d_render_sanity.cpp -- TGA 24bpp não-comprimido, top-down.
std::vector<unsigned char> tga_header(int w, int h) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0);
  f.push_back(0);
  f.push_back(2);
  push16(0);
  push16(0);
  f.push_back(0);
  push16(0);
  push16(0);
  push16(w);
  push16(h);
  f.push_back(24);
  f.push_back(0x20);
  return f;
}

std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                           unsigned char b) {
  std::vector<unsigned char> f = tga_header(w, h);
  for (int i = 0; i < w * h; ++i) {
    f.push_back(b);
    f.push_back(g);
    f.push_back(r);
  }
  return f;
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

// EN: A flat RGBA8 (straight alpha) buffer, w*h pixels, one constant colour -- the raw-pixel
//     twin of build_tga_solid() above, fed to create_texture() instead of load_texture().
// PT: Um buffer RGBA8 (alpha straight) chapado, w*h pixels, uma cor constante -- o gêmeo
//     cru-em-pixel do build_tga_solid() acima, entregue ao create_texture() em vez do
//     load_texture().
std::vector<unsigned char> build_rgba8_solid(int w, int h, unsigned char r, unsigned char g,
                                             unsigned char b, unsigned char a) {
  std::vector<unsigned char> px(static_cast<std::size_t>(w) * h * 4);
  for (int i = 0; i < w * h; ++i) {
    px[static_cast<std::size_t>(i) * 4 + 0] = r;
    px[static_cast<std::size_t>(i) * 4 + 1] = g;
    px[static_cast<std::size_t>(i) * 4 + 2] = b;
    px[static_cast<std::size_t>(i) * 4 + 3] = a;
  }
  return px;
}

// EN: A flat R8 buffer, w*h pixels, one constant coverage value.
// PT: Um buffer R8 chapado, w*h pixels, um valor de cobertura constante.
std::vector<unsigned char> build_r8_solid(int w, int h, unsigned char coverage) {
  return std::vector<unsigned char>(static_cast<std::size_t>(w) * h, coverage);
}

// EN: glReadPixels row order fix -- same idiom as draw2d_render_sanity.cpp's own
//     read_backbuffer_rgb (OpenGL returns bottom-up; this file's region math wants top-down,
//     y-down, matching every RectF this test uses).
// PT: Fix de ordem de linha do glReadPixels -- mesmo idioma do próprio read_backbuffer_rgb de
//     draw2d_render_sanity.cpp (o OpenGL devolve de baixo pra cima; a matemática de região deste
//     arquivo quer de cima pra baixo, y pra baixo, batendo com todo RectF que este teste usa).
std::vector<unsigned char> read_backbuffer_rgb(int w, int h) {
  std::vector<unsigned char> raw(static_cast<std::size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, raw.data());
  std::vector<unsigned char> px(raw.size());
  for (int y = 0; y < h; ++y) {
    const unsigned char* src_row = raw.data() + static_cast<std::size_t>(h - 1 - y) * w * 3;
    unsigned char* dst_row = px.data() + static_cast<std::size_t>(y) * w * 3;
    std::copy(src_row, src_row + static_cast<std::size_t>(w) * 3, dst_row);
  }
  return px;
}

struct Rgb {
  double r = 0, g = 0, b = 0;
};

Rgb region_mean(const std::vector<unsigned char>& px, int w, int h, int cx, int cy, int half) {
  Rgb sum;
  int n = 0;
  for (int y = cy - half; y <= cy + half; ++y) {
    for (int x = cx - half; x <= cx + half; ++x) {
      if (x < 0 || x >= w || y < 0 || y >= h) continue;
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

} // namespace

int main() {
  constexpr int W = 128, H = 128;
  constexpr double kTol = 8.0; // llvmpipe/blend-rounding tolerance, same order as the sibling files.

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_pixels_flush_host", W, H)) {
    std::puts("draw2d_pixels_flush_render_sanity FAIL: host window create failed");
    return 1;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_pixels_flush_render_sanity FAIL: Draw2d::init() failed");
    return 2;
  }
  glViewport(0, 0, W, H);

  char tmpl[] = "/var/tmp/glintfx_draw2d_pixels_flush_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_pixels_flush_render_sanity FAIL: mkdtemp failed");
    return 3;
  }
  const fs::path dir(dir_c);

  // -------------------------------------------------------------------------------------------
  // Check 1: create_texture(Rgba8) EQUALS load_texture() for the same solid colour.
  // -------------------------------------------------------------------------------------------
  {
    const fs::path p = dir / "solid.tga";
    write_file(p, build_tga_solid(8, 8, 200, 100, 50));
    Texture2d tex_file = d2d.load_texture(p.c_str());
    check(tex_file.ok() && tex_file.width() == 8 && tex_file.height() == 8,
          "fixture: solid.tga decoded via load_texture()");

    const std::vector<unsigned char> raw = build_rgba8_solid(8, 8, 200, 100, 50, 255);
    Texture2d tex_mem = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::Rgba8);
    check(tex_mem.ok() && tex_mem.width() == 8 && tex_mem.height() == 8,
          "fixture: equivalent 8x8 buffer decoded via create_texture(Rgba8)");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_file, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)});
    d2d.end();
    const Rgb mean_file = region_mean(read_backbuffer_rgb(W, H), W, H, W / 2, H / 2, 8);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_mem, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)});
    d2d.end();
    const Rgb mean_mem = region_mean(read_backbuffer_rgb(W, H), W, H, W / 2, H / 2, 8);

    std::printf(
        "draw2d_pixels_flush_render_sanity: load_texture mean=(%.1f,%.1f,%.1f) "
        "create_texture mean=(%.1f,%.1f,%.1f), expect ~(200,100,50) both\n",
        mean_file.r, mean_file.g, mean_file.b, mean_mem.r, mean_mem.g, mean_mem.b);
    check(near_rgb(mean_file, 200, 100, 50, kTol),
          "texpixels_equivalence: load_texture() readback matches the known colour");
    check(near_rgb(mean_mem, 200, 100, 50, kTol),
          "texpixels_equivalence: create_texture(Rgba8) readback matches the SAME known colour");
    check(near_rgb(mean_mem, mean_file.r, mean_file.g, mean_file.b, kTol),
          "texpixels_equivalence: create_texture() and load_texture() render identically");

    d2d.destroy_texture(tex_file);
    d2d.destroy_texture(tex_mem);
  }

  // -------------------------------------------------------------------------------------------
  // Check 2: create_texture(Rgba8) premultiply -- straight-alpha (200,100,50,128) blended over
  // background (10,20,40), tint identity. Premultiply on upload (D7/D8, same formula
  // decode_premultiplied_rgba() uses): premult = (200*128/255, 100*128/255, 50*128/255, 128) =
  // (100, 50, 25, 128), integer truncation. GL_ONE/GL_ONE_MINUS_SRC_ALPHA "over":
  // result = premult/255 + bg/255*(1-128/255) ~= (105,60,45) in 8-bit -- the SAME hand-derived
  // triple draw2d_render_sanity.cpp's own check 2 uses (200,100,50 opaque + tint alpha 0.5),
  // confirming create_texture()'s own premultiply-on-ingest lands in the identical space.
  // -------------------------------------------------------------------------------------------
  {
    const std::vector<unsigned char> raw = build_rgba8_solid(8, 8, 200, 100, 50, 128);
    Texture2d tex = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::Rgba8);
    check(tex.ok(), "premultiply_fixture: straight-alpha buffer decoded via create_texture(Rgba8)");

    glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)});
    d2d.end();
    const Rgb mean = region_mean(read_backbuffer_rgb(W, H), W, H, W / 2, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: create_texture premultiply mean=(%.1f,%.1f,%.1f), "
        "expect ~(105,60,45)\n",
        mean.r, mean.g, mean.b);
    check(near_rgb(mean, 105, 60, 45, kTol),
          "texpixels_premultiply: create_texture(Rgba8) premultiplied straight alpha on ingest, "
          "matches the D8 hand-derived delta");
    d2d.destroy_texture(tex);
  }

  // -------------------------------------------------------------------------------------------
  // Check 3: create_texture(R8), full coverage=200, tint pure red opaque. D8: texel swizzled to
  // (200,200,200,200)/255; finalColor = texel*(tint.rgb*tint.a,tint.a) = (200/255,0,0,200/255).
  // Background black, so GL_ONE/GL_ONE_MINUS_SRC_ALPHA "over" leaves the result exactly the src
  // -- expect ~(200,0,0).
  // -------------------------------------------------------------------------------------------
  {
    const std::vector<unsigned char> raw = build_r8_solid(8, 8, 200);
    Texture2d tex = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::R8);
    check(tex.ok(), "r8_fixture: coverage buffer decoded via create_texture(R8)");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)}, RectF{},
                    ColorF{1.f, 0.f, 0.f, 1.f});
    d2d.end();
    const Rgb mean = region_mean(read_backbuffer_rgb(W, H), W, H, W / 2, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: R8 coverage=200 red-tint mean=(%.1f,%.1f,%.1f), "
        "expect ~(200,0,0)\n",
        mean.r, mean.g, mean.b);
    check(near_rgb(mean, 200, 0, 0, kTol),
          "texpixels_r8_coverage: R8 texel swizzles to (c,c,c,c) and tints exactly like a "
          "font-glyph coverage atlas would (D8)");
    d2d.destroy_texture(tex);
  }

  // -------------------------------------------------------------------------------------------
  // Check 4: create_texture(R8), full coverage=0 -- fully invisible, background shows through
  // unchanged. Proves R8's own byte IS the coverage/alpha (not a constant-alpha shape).
  // -------------------------------------------------------------------------------------------
  {
    const std::vector<unsigned char> raw = build_r8_solid(8, 8, 0);
    Texture2d tex = d2d.create_texture(raw.data(), 8, 8, Draw2d::PixelFormat::R8);
    check(tex.ok(), "r8_zero_fixture: zero-coverage buffer decoded via create_texture(R8)");

    glClearColor(30.f / 255.f, 40.f / 255.f, 50.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)}, RectF{},
                    ColorF{1.f, 1.f, 1.f, 1.f});
    d2d.end();
    const Rgb mean = region_mean(read_backbuffer_rgb(W, H), W, H, W / 2, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: R8 coverage=0 mean=(%.1f,%.1f,%.1f), "
        "expect ~(30,40,50) (background untouched)\n",
        mean.r, mean.g, mean.b);
    check(near_rgb(mean, 30, 40, 50, kTol),
          "texpixels_r8_zero_coverage: zero coverage is fully transparent, background shows "
          "through unchanged");
    d2d.destroy_texture(tex);
  }

  // -------------------------------------------------------------------------------------------
  // Check 5: flush() forces GL work without closing the bracket. Left half drawn, flush()'d,
  // READ BACK BEFORE end() (proves the GL draw call already ran); right half drawn in the SAME
  // still-open bracket AFTER flush(); end(); read back again (proves the bracket stayed usable
  // and both halves survive to the final frame).
  // -------------------------------------------------------------------------------------------
  {
    const fs::path p_a = dir / "flush_a.tga";
    const fs::path p_b = dir / "flush_b.tga";
    write_file(p_a, build_tga_solid(4, 4, 200, 100, 50));
    write_file(p_b, build_tga_solid(4, 4, 250, 150, 50));
    Texture2d tex_a = d2d.load_texture(p_a.c_str());
    Texture2d tex_b = d2d.load_texture(p_b.c_str());
    check(tex_a.ok() && tex_b.ok(), "flush_fixture: both halves' textures loaded");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_a, RectF{0.f, 0.f, static_cast<float>(W) / 2.f, static_cast<float>(H)});
    d2d.flush(); // must push the left half to GL NOW -- bracket stays open.

    const Rgb mid_left = region_mean(read_backbuffer_rgb(W, H), W, H, W / 4, H / 2, 8);
    const Rgb mid_right_before =
        region_mean(read_backbuffer_rgb(W, H), W, H, 3 * W / 4, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: mid-bracket (post-flush, pre-end) left=(%.1f,%.1f,%.1f) "
        "expect ~(200,100,50), right=(%.1f,%.1f,%.1f) expect ~(0,0,0) (not drawn yet)\n",
        mid_left.r, mid_left.g, mid_left.b, mid_right_before.r, mid_right_before.g,
        mid_right_before.b);
    check(near_rgb(mid_left, 200, 100, 50, kTol),
          "flush_bracket_stays_open: flush() pushed the pending draw to GL BEFORE end() ran");
    check(near_rgb(mid_right_before, 0, 0, 0, kTol),
          "flush_bracket_stays_open: the not-yet-issued right half is still background at the "
          "post-flush readback");

    // Same still-open bracket, AFTER flush() -- proves the bracket remained usable.
    d2d.draw_sprite(tex_b, RectF{static_cast<float>(W) / 2.f, 0.f, static_cast<float>(W) / 2.f,
                                 static_cast<float>(H)});
    d2d.end();

    const auto px_final = read_backbuffer_rgb(W, H);
    const Rgb final_left = region_mean(px_final, W, H, W / 4, H / 2, 8);
    const Rgb final_right = region_mean(px_final, W, H, 3 * W / 4, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: final left=(%.1f,%.1f,%.1f) expect "
        "~(200,100,50), right=(%.1f,%.1f,%.1f) expect ~(250,150,50)\n",
        final_left.r, final_left.g, final_left.b, final_right.r, final_right.g,
        final_right.b);
    check(near_rgb(final_left, 200, 100, 50, kTol),
          "flush_bracket_stays_open: the pre-flush draw survives to the final frame unchanged");
    check(near_rgb(final_right, 250, 150, 50, kTol),
          "flush_bracket_stays_open: the post-flush draw in the SAME bracket rendered correctly");

    d2d.destroy_texture(tex_a);
    d2d.destroy_texture(tex_b);
  }

  // -------------------------------------------------------------------------------------------
  // Check 6: flush() leaves the CURRENT camera intact. Camera centred on the world origin; a
  // 4x4-world-unit sprite at world rect {-2,-2,4,4} projects to screen centre. Drawn once BEFORE
  // flush() (tex_a) and once AFTER flush() (tex_b, SAME world rect, SAME camera never reset in
  // between) -- if flush() had reset the camera (screen-space fallback), the second draw would
  // read {-2,-2,4,4} as raw SCREEN pixels instead, landing near the top-left corner, not centre.
  // Uses the SpriteTransform (D18) overload with an identity transform -- draw2d.hpp's own
  // contract: only THIS overload (and draw_filled_rect/quad/line/outline below) ever consults
  // the camera; the plain 4-arg draw_sprite() keeps the v0.20.0 no-camera path verbatim
  // regardless of camera state (Q1 (c)), so it would be the wrong probe for "did the camera
  // survive" -- it never looks at the camera at all, flush() notwithstanding.
  // -------------------------------------------------------------------------------------------
  {
    const fs::path p_a = dir / "cam_a.tga";
    const fs::path p_b = dir / "cam_b.tga";
    write_file(p_a, build_tga_solid(4, 4, 10, 200, 10));
    write_file(p_b, build_tga_solid(4, 4, 10, 10, 200));
    Texture2d tex_a = d2d.load_texture(p_a.c_str());
    Texture2d tex_b = d2d.load_texture(p_b.c_str());
    check(tex_a.ok() && tex_b.ok(), "flush_camera_fixture: both textures loaded");

    const RectF world_dst{-2.f, -2.f, 4.f, 4.f};
    d2d.set_camera(Camera2d{0.f, 0.f, 1.f, 0.f}); // centres the world origin on screen centre.

    glClearColor(100.f / 255.f, 100.f / 255.f, 100.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_a, world_dst, RectF{}, ColorF{}, SpriteTransform{}); // BEFORE flush().
    d2d.flush();
    // AFTER flush(), SAME world rect, SAME camera, SAME identity transform.
    d2d.draw_sprite(tex_b, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();

    const auto px = read_backbuffer_rgb(W, H);
    const Rgb centre = region_mean(px, W, H, W / 2, H / 2, 1);
    // A near-origin screen probe: only lit up if the SECOND draw were misread as raw screen
    // coordinates (camera lost) instead of world coordinates (camera intact).
    const Rgb near_origin = region_mean(px, W, H, 1, 1, 0);
    std::printf(
        "draw2d_pixels_flush_render_sanity: camera-intact centre=(%.1f,%.1f,%.1f) expect "
        "~(10,10,200) (tex_b, last-draw-wins), near-origin=(%.1f,%.1f,%.1f) expect "
        "~(100,100,100) (background)\n",
        centre.r, centre.g, centre.b, near_origin.r, near_origin.g, near_origin.b);
    check(near_rgb(centre, 10, 10, 200, kTol),
          "flush_camera_intact: the post-flush world-space draw still projects to screen centre "
          "-- the camera survived flush()");
    check(near_rgb(near_origin, 100, 100, 100, kTol),
          "flush_camera_intact: nothing bled into the raw-screen-coordinate position a lost "
          "camera would have used");

    d2d.reset_camera();
    d2d.destroy_texture(tex_a);
    d2d.destroy_texture(tex_b);
  }

  // -------------------------------------------------------------------------------------------
  // Check 7: flush() leaves the CURRENT scissor intact. Scissor clips to the LEFT half; a
  // filled rect (colour 1) drawn, flush()'d, then a SECOND filled rect (colour 2, full viewport)
  // drawn in the SAME bracket AFTER flush() -- if the scissor survived, colour 2 only shows on
  // the LEFT half (right half stays whatever colour 1 left there, i.e. still clipped-away
  // background); if flush() had reset the scissor, colour 2 would cover the WHOLE viewport.
  // -------------------------------------------------------------------------------------------
  {
    d2d.set_scissor(RectF{0.f, 0.f, static_cast<float>(W) / 2.f, static_cast<float>(H)});

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_filled_rect(RectF{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)},
                         ColorF{200.f / 255.f, 100.f / 255.f, 50.f / 255.f, 1.f});
    d2d.flush();
    d2d.draw_filled_rect(RectF{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)},
                         ColorF{50.f / 255.f, 100.f / 255.f, 200.f / 255.f, 1.f});
    d2d.end();

    const auto px = read_backbuffer_rgb(W, H);
    const Rgb left = region_mean(px, W, H, W / 4, H / 2, 8);
    const Rgb right = region_mean(px, W, H, 3 * W / 4, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: scissor-intact left=(%.1f,%.1f,%.1f) expect "
        "~(50,100,200), right=(%.1f,%.1f,%.1f) expect ~(0,0,0) (still clipped)\n",
        left.r, left.g, left.b, right.r, right.g, right.b);
    check(near_rgb(left, 50, 100, 200, kTol),
          "flush_scissor_intact: the post-flush fill still lands inside the left-half scissor");
    check(near_rgb(right, 0, 0, 0, kTol),
          "flush_scissor_intact: the right half stayed clipped-away -- the scissor survived "
          "flush(), the post-flush fill did NOT cover the whole viewport");

    d2d.reset_scissor();
  }

  // -------------------------------------------------------------------------------------------
  // Check 8: flush() restores the CURRENT scissor in BUFFERED mode too (set_layer() armed).
  // Check 7 above only exercises STREAMING mode; buffered mode has its OWN code path
  // (Impl::replay_layer_queue()'s per-group loop leaves `scissor_active`/`scissor_rect` at
  // whichever GROUP was replayed last, not necessarily the caller's actual current scissor --
  // flush() must save/restore around that call). Sequence: scissor=LEFT, one buffered fill under
  // it; THEN scissor changes to RIGHT (no draw under RIGHT yet); flush() drains+replays the
  // LEFT-scissor group and must leave the CURRENT scissor read back as RIGHT afterwards; a
  // second buffered fill is pushed (captures whatever scissor flush() left behind); end()
  // replays it. If flush() failed to restore (left the state at the just-replayed LEFT group
  // instead), the second fill would ALSO be clipped to the left half instead of the right --
  // directly distinguishable from the correct outcome below.
  // -------------------------------------------------------------------------------------------
  {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.set_layer(0);                                                                     // arms buffered mode for this bracket.
    d2d.set_scissor(RectF{0.f, 0.f, static_cast<float>(W) / 2.f, static_cast<float>(H)}); // LEFT.
    d2d.draw_filled_rect(RectF{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)},
                         ColorF{200.f / 255.f, 100.f / 255.f, 50.f / 255.f, 1.f}); // buffered under LEFT.
    d2d.set_scissor(
        RectF{static_cast<float>(W) / 2.f, 0.f, static_cast<float>(W) / 2.f, static_cast<float>(H)}); // RIGHT, no draw yet.
    d2d.flush();                                                                                      // replays the LEFT-scissor group; must leave the CURRENT scissor as RIGHT.
    d2d.draw_filled_rect(RectF{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)},
                         ColorF{50.f / 255.f, 100.f / 255.f, 200.f / 255.f, 1.f}); // buffered under whatever flush() left as current.
    d2d.end();                                                                     // replays the second (RIGHT-scissor, if correct) group.

    const auto px = read_backbuffer_rgb(W, H);
    const Rgb left = region_mean(px, W, H, W / 4, H / 2, 8);
    const Rgb right = region_mean(px, W, H, 3 * W / 4, H / 2, 8);
    std::printf(
        "draw2d_pixels_flush_render_sanity: buffered-mode scissor-intact left=(%.1f,%.1f,%.1f) "
        "expect ~(200,100,50), right=(%.1f,%.1f,%.1f) expect ~(50,100,200)\n",
        left.r, left.g, left.b, right.r, right.g, right.b);
    check(near_rgb(left, 200, 100, 50, kTol),
          "flush_scissor_intact_buffered: the pre-flush LEFT-scissor fill landed on the left "
          "half, untouched by the post-flush fill");
    check(near_rgb(right, 50, 100, 200, kTol),
          "flush_scissor_intact_buffered: the post-flush fill captured the CURRENT (RIGHT) "
          "scissor, not a stale snapshot left behind by the buffered replay");

    d2d.reset_scissor();
  }

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_pixels_flush_render_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_pixels_flush_render_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
