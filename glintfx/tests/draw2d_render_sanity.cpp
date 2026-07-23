// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_render_sanity (D2D-1B) -- pixel-readback proof of `glintfx::Draw2d` under Xvfb/
//     llvmpipe, STATISTICAL/tolerant (never pixel-exact -- the house's own golden_test precedent,
//     see AGENTS.md's "golden_test is flaky on llvmpipe" gotcha). Embed fixture: a hidden
//     WindowGlfw provides the current GL context (same idiom as gl_state_guard.cpp/
//     ui_layer_compose.cpp), no UiLayer/RmlUi involved at all -- this proves Draw2D standalone.
//
//     Every fixture texture is a HAND-CRAFTED, uncompressed 24bpp TGA (stb_image-decodable, D7)
//     with EXACTLY known solid colours -- deliberately NOT circle_rgba.png (whose own pixel
//     values are an opaque implementation detail of a PNG file this test does not own): every
//     expected pixel below is a HAND-COMPUTED delta against the D8 tint formula, and that
//     computation needs exact source colours, not "reddish, roughly".
//
//     Four checks (plan section 5, `draw2d_render_sanity`):
//       1. solid-colour sprite readback (region mean within tolerance).
//       2. alpha blend of a semi-transparent sprite over a known clear colour matches the
//          premultiplied expectation (a HAND-DERIVED delta, not "looks blended").
//       3. src_px quadrant of a 2x2 checker texture shows the right dominant colour.
//       4. the D8 tint formula verified with a computed expected value (a SECOND, independent
//          delta test at a non-trivial tint, per the house's "formula without a delta test is
//          the silent-bug class" rule).
// PT: draw2d_render_sanity (D2D-1B) -- prova de pixel-readback do `glintfx::Draw2d` sob
//     Xvfb/llvmpipe, ESTATÍSTICA/tolerante (nunca pixel-exata -- o próprio precedente do
//     golden_test da casa, ver o gotcha "golden_test é flaky no llvmpipe" do AGENTS.md). Fixture
//     embed: um WindowGlfw oculto fornece o contexto GL corrente (mesmo idioma de
//     gl_state_guard.cpp/ui_layer_compose.cpp), nenhum UiLayer/RmlUi envolvido -- isto prova o
//     Draw2D sozinho.
//
//     Toda textura fixture é um TGA 24bpp não-comprimido FEITO À MÃO (decodificável por
//     stb_image, D7) com cores sólidas EXATAMENTE conhecidas -- deliberadamente NÃO
//     circle_rgba.png (cujos próprios valores de pixel são um detalhe de implementação opaco de
//     um arquivo PNG que este teste não possui): todo pixel esperado abaixo é um delta
//     CALCULADO À MÃO contra a fórmula de tint D8, e esse cálculo precisa de cores de origem
//     exatas, não "avermelhado, mais ou menos".
//
//     Quatro checks (plano seção 5, `draw2d_render_sanity`):
//       1. readback de sprite de cor sólida (média de região dentro de tolerância).
//       2. blend alpha de um sprite semi-transparente sobre uma cor de clear conhecida bate com
//          a expectativa premultiplicada (um delta DERIVADO À MÃO, não "parece misturado").
//       3. quadrante src_px de uma textura checker 2x2 mostra a cor dominante certa.
//       4. a fórmula de tint D8 verificada com um valor esperado calculado (um SEGUNDO teste de
//          delta independente, num tint não-trivial, conforme a regra da casa "fórmula sem teste
//          de delta é a classe de bug silencioso").
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
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

// EN: Uncompressed 24bpp TGA, image_descriptor 0x20 (top-down: the FIRST row written is the
//     TOP row) -- matches D5's "src_px ... matching image memory row order" so the checker
//     quadrant test below can reason about which texel is which without a second flip to track.
// PT: TGA 24bpp não-comprimido, image_descriptor 0x20 (top-down: a PRIMEIRA linha escrita é a
//     linha de CIMA) -- bate com o "src_px ... batendo com a ordem de linha da memória de
//     imagem" do D5, então o teste de quadrante checker abaixo pode raciocinar sobre qual texel é
//     qual sem uma segunda inversão pra rastrear.
std::vector<unsigned char> tga_header(int w, int h) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0);  // id length
  f.push_back(0);  // colour map type
  f.push_back(2);  // image type: uncompressed true-colour
  push16(0); push16(0); f.push_back(0); // colour map spec, unused
  push16(0); push16(0); // x/y origin
  push16(w); push16(h);
  f.push_back(24);   // bits per pixel
  f.push_back(0x20); // top-down, no alpha bits
  return f;
}

void push_bgr(std::vector<unsigned char>& f, unsigned char r, unsigned char g, unsigned char b) {
  f.push_back(b);
  f.push_back(g);
  f.push_back(r);
}

std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                            unsigned char b) {
  std::vector<unsigned char> f = tga_header(w, h);
  for (int i = 0; i < w * h; ++i) push_bgr(f, r, g, b);
  return f;
}

// EN: 2x2 checker: row0 (TOP, per the header above) = [tl, tr]; row1 (bottom) = [bl, br].
// PT: Checker 2x2: linha0 (CIMA, conforme o header acima) = [tl, tr]; linha1 (baixo) = [bl, br].
std::vector<unsigned char> build_tga_checker2x2(unsigned char tl_r, unsigned char tl_g,
                                                 unsigned char tl_b, unsigned char tr_r,
                                                 unsigned char tr_g, unsigned char tr_b,
                                                 unsigned char bl_r, unsigned char bl_g,
                                                 unsigned char bl_b, unsigned char br_r,
                                                 unsigned char br_g, unsigned char br_b) {
  std::vector<unsigned char> f = tga_header(2, 2);
  push_bgr(f, tl_r, tl_g, tl_b);
  push_bgr(f, tr_r, tr_g, tr_b);
  push_bgr(f, bl_r, bl_g, bl_b);
  push_bgr(f, br_r, br_g, br_b);
  return f;
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

std::vector<unsigned char> read_backbuffer_rgb(int w, int h) {
  std::vector<unsigned char> px(static_cast<std::size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

struct Rgb { double r = 0, g = 0, b = 0; };

// EN: Mean colour of a (2*half+1)^2 box centred at (cx,cy) -- absorbs single-pixel llvmpipe
//     rasterization/rounding noise, same "statistical, not exact" discipline as render_sanity.
// PT: Cor média de uma caixa (2*half+1)^2 centrada em (cx,cy) -- absorve ruído de
//     rasterização/arredondamento de pixel único do llvmpipe, mesma disciplina "estatístico, não
//     exato" do render_sanity.
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
  if (n > 0) { sum.r /= n; sum.g /= n; sum.b /= n; }
  return sum;
}

bool near_rgb(const Rgb& got, double r, double g, double b, double tol) {
  return std::fabs(got.r - r) <= tol && std::fabs(got.g - g) <= tol && std::fabs(got.b - b) <= tol;
}

} // namespace

int main() {
  constexpr int W = 128, H = 128;
  constexpr double kTol = 8.0; // llvmpipe/blend-rounding tolerance, same order as render_sanity.

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_render_host", W, H)) {
    std::puts("draw2d_render_sanity FAIL: host window create failed");
    return 1;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_render_sanity FAIL: Draw2d::init() failed");
    return 2;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_render_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_render_sanity FAIL: mkdtemp failed");
    return 3;
  }
  const fs::path dir(dir_c);

  const fs::path p_solid_a = dir / "solid_a.tga"; // R=200,G=100,B=50, used by checks 1+2.
  const fs::path p_solid_b = dir / "solid_b.tga"; // R=250,G=150,B=50, used by check 4.
  const fs::path p_checker = dir / "checker.tga";

  write_file(p_solid_a, build_tga_solid(8, 8, 200, 100, 50));
  write_file(p_solid_b, build_tga_solid(8, 8, 250, 150, 50));
  // tl=(255,0,0) tr=(0,255,0) bl=(0,0,255) br=(255,255,0).
  write_file(p_checker, build_tga_checker2x2(255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0));

  Texture2d tex_a = d2d.load_texture(p_solid_a.c_str());
  Texture2d tex_b = d2d.load_texture(p_solid_b.c_str());
  Texture2d tex_checker = d2d.load_texture(p_checker.c_str());
  check(tex_a.ok() && tex_a.width() == 8 && tex_a.height() == 8, "fixture: solid_a.tga decoded");
  check(tex_b.ok() && tex_b.width() == 8 && tex_b.height() == 8, "fixture: solid_b.tga decoded");
  check(tex_checker.ok() && tex_checker.width() == 2 && tex_checker.height() == 2,
        "fixture: checker.tga decoded");

  glViewport(0, 0, W, H); // D9: viewport is the host's own responsibility, not Draw2D's.

  // -------------------------------------------------------------------------------------------
  // Check 1: solid-colour sprite readback (default tint, opaque texel -- an exact colour, not
  // just "not fallback").
  // -------------------------------------------------------------------------------------------
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex_a, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)});
  d2d.end();
  {
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb mean = region_mean(px, W, H, W / 2, H / 2, 8);
    std::printf("draw2d_render_sanity: solid readback mean = (%.1f,%.1f,%.1f), expect ~(200,100,50)\n",
                mean.r, mean.g, mean.b);
    check(near_rgb(mean, 200, 100, 50, kTol), "solid_colour_sprite_readback: mean within tolerance");
  }

  // -------------------------------------------------------------------------------------------
  // Check 2: alpha blend, D8 formula hand-derived. Texture solid_a (200,100,50), fully opaque
  // (A=255 forced by the 24bpp decode, so premultiply on upload is a NO-OP: R*255/255==R).
  // tint = {1,1,1,0.5}. finalColor = texel*(tint.rgb*tint.a, tint.a)
  //   = (200*0.5/255, 100*0.5/255, 50*0.5/255, 0.5) = (100/255, 50/255, 25/255, 0.5).
  // Background cleared to the anchor colour (10,20,40) (same anchor ui_layer_compose.cpp uses).
  // GL_ONE/GL_ONE_MINUS_SRC_ALPHA "over": result = src + dst*(1-src.a)
  //   = (100/255,50/255,25/255) + (10/255,20/255,40/255)*0.5 = (105/255,60/255,45/255)
  //   -> (105, 60, 45) in 8-bit -- every term above is an EXACT integer division, no rounding.
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex_a, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)}, RectF{},
                   ColorF{1.f, 1.f, 1.f, 0.5f});
  d2d.end();
  {
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb mean = region_mean(px, W, H, W / 2, H / 2, 8);
    std::printf("draw2d_render_sanity: alpha-blend mean = (%.1f,%.1f,%.1f), expect ~(105,60,45)\n",
                mean.r, mean.g, mean.b);
    check(near_rgb(mean, 105, 60, 45, kTol),
          "alpha_blend_premultiplied: matches the hand-derived D8 delta");
  }

  // -------------------------------------------------------------------------------------------
  // Check 3: src_px quadrant of the 2x2 checker -- top-left texel only (src_px = {0,0,1,1}).
  // The viewport CENTRE maps to that texel's OWN sample centre (UV (0.25,0.25) of a 2-wide
  // texture), so GL_LINEAR returns the texel value exactly, no neighbour bleed.
  // -------------------------------------------------------------------------------------------
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex_checker, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)},
                   RectF{0.f, 0.f, 1.f, 1.f});
  d2d.end();
  {
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb mean = region_mean(px, W, H, W / 2, H / 2, 4);
    std::printf("draw2d_render_sanity: quadrant mean = (%.1f,%.1f,%.1f), expect ~(255,0,0) (top-left texel)\n",
                mean.r, mean.g, mean.b);
    check(near_rgb(mean, 255, 0, 0, kTol),
          "src_px_quadrant_dominant_colour: top-left texel dominates as expected");
  }

  // -------------------------------------------------------------------------------------------
  // Check 4: a SECOND, independent D8 delta at a non-trivial tint (every channel != 1, D8's own
  // house rule against a formula proven only at the identity/half-alpha special cases above).
  // Texture solid_b (250,150,50), opaque. tint = {0.2,0.4,0.6,0.8}.
  // finalColor = (250*0.2*0.8/255, 150*0.4*0.8/255, 50*0.6*0.8/255, 0.8)
  //            = (40/255, 48/255, 24/255, 0.8) -- every term an exact integer product/division.
  // Anchor background (10,20,40): result = (40,48,24)/255 + (10,20,40)/255*0.2
  //            = (42,52,32)/255 -> (42,52,32) in 8-bit, exact.
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex_b, RectF{0, 0, static_cast<float>(W), static_cast<float>(H)}, RectF{},
                   ColorF{0.2f, 0.4f, 0.6f, 0.8f});
  d2d.end();
  {
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb mean = region_mean(px, W, H, W / 2, H / 2, 8);
    std::printf("draw2d_render_sanity: tint-formula mean = (%.1f,%.1f,%.1f), expect ~(42,52,32)\n",
                mean.r, mean.g, mean.b);
    check(near_rgb(mean, 42, 52, 32, kTol),
          "tint_formula_computed_value: matches the D8 formula at a non-trivial tint");
  }

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_render_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_render_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
