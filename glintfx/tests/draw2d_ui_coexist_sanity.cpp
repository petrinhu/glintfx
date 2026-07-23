// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_ui_coexist_sanity (D2D-1B) -- the test that guards this wave's CENTRAL RISK (plan
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md section 0/D9): Draw2D's own GL path
//     COHABITS the exact GL context the RmlUi GL3 backend renders into. D9 (the
//     anti-`GlStateGuard`-bug contract) says Draw2D sets EVERY piece of state it needs at each
//     flush and never restores afterwards -- this test is the proof, in BOTH possible orders
//     within one frame: sprite pass THEN ui_layer.render(), and ui_layer.render() THEN sprite
//     pass. If either renderer silently depended on state the OTHER one happened to leave behind
//     (the `GlStateGuard`-class bug this whole decision exists to prevent), one of the two orders
//     would render wrong while the other renders right -- both readbacks below must show BOTH
//     layers correct, independent of order.
//
//     Non-overlapping regions by design (UI box top-left, sprite bottom-right, a neutral strip
//     between the two): this isolates "did state corruption break the OTHER renderer" from "what
//     does premultiplied-alpha compositing of two OVERLAPPING draws look like" (a different,
//     already-covered question -- ui_layer_compose.cpp/draw2d_render_sanity.cpp).
// PT: draw2d_ui_coexist_sanity (D2D-1B) -- o teste que guarda o RISCO CENTRAL desta onda (plano
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md seção 0/D9): o caminho GL próprio do
//     Draw2D COABITA o mesmo contexto GL exato em que o backend GL3 do RmlUi renderiza. A D9 (o
//     contrato anti-bug-`GlStateGuard`) diz que o Draw2D seta TODO pedaço de estado de que
//     precisa a cada flush e nunca restaura depois -- este teste é a prova, nas DUAS ordens
//     possíveis dentro de um frame: passe de sprite DEPOIS ui_layer.render(), e
//     ui_layer.render() DEPOIS passe de sprite. Se algum dos dois renderers dependesse em
//     silêncio de estado que o OUTRO por acaso deixou pra trás (a classe de bug `GlStateGuard`
//     que esta decisão inteira existe pra prevenir), uma das duas ordens renderizaria errado
//     enquanto a outra renderizaria certo -- os dois readbacks abaixo precisam mostrar as DUAS
//     camadas corretas, independente da ordem.
//
//     Regiões sem sobreposição por design (caixa de UI no topo-esquerda, sprite no
//     fundo-direita, uma faixa neutra entre as duas): isso isola "a corrupção de estado quebrou o
//     OUTRO renderer" de "como fica a composição alpha-premultiplicada de dois desenhos QUE SE
//     SOBREPÕEM" (uma pergunta diferente, já coberta -- ui_layer_compose.cpp/
//     draw2d_render_sanity.cpp).
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
#include <string>
#include <vector>

namespace fs = std::filesystem;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::Texture2d;
using glintfx::UiLayer;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Same hand-crafted-TGA idiom as draw2d_render_sanity.cpp -- an exact, known solid colour,
//     independent of any PNG fixture's own opaque byte values.
// PT: Mesmo idioma de TGA feito-à-mão de draw2d_render_sanity.cpp -- uma cor sólida exata e
//     conhecida, independente dos próprios valores de byte opacos de qualquer fixture PNG.
std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                            unsigned char b) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0); f.push_back(0); f.push_back(2);
  push16(0); push16(0); f.push_back(0);
  push16(0); push16(0);
  push16(w); push16(h);
  f.push_back(24);
  f.push_back(0x20); // top-down.
  for (int i = 0; i < w * h; ++i) { f.push_back(b); f.push_back(g); f.push_back(r); }
  return f;
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

bool write_ui_box_rml(const fs::path& path, int box_size) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "<rml>\n<head>\n<style>\n"
       "body { margin: 0; padding: 0; }\n"
       "#box { position: absolute; left: 0px; top: 0px; width: "
    << box_size << "px; height: " << box_size
    << "px; background-color: white; }\n"
       "</style>\n<title>coexist probe</title>\n</head>\n<body>\n<div id=\"box\"></div>\n</body>\n</rml>\n";
  return f.good();
}

// EN: glReadPixels itself returns rows BOTTOM-up (row 0 = the window's bottom edge, OpenGL's own
//     convention) -- flipped here so the RETURNED buffer is TOP-down, matching the y-down
//     convention every `RectF`/region coordinate in this file already uses (D5, and RmlUi's own
//     document space). Same gotcha decorator_ripple.cpp's own top comment documents ("row = h -
//     1 - content_y") for the identical backbuffer-vs-document-space mismatch.
// PT: O próprio glReadPixels retorna linhas de BAIXO pra cima (linha 0 = borda de baixo da
//     janela, convenção própria do OpenGL) -- invertido aqui pra que o buffer RETORNADO fique de
//     CIMA pra baixo, batendo com a convenção y-para-baixo que toda coordenada `RectF`/região
//     deste arquivo já usa (D5, e o próprio espaço de documento do RmlUi). Mesma armadilha que o
//     próprio comentário de topo de decorator_ripple.cpp documenta ("row = h - 1 - content_y")
//     pro descasamento idêntico entre backbuffer e espaço de documento.
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

struct Rgb { double r = 0, g = 0, b = 0; };

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
  constexpr int W = 160, H = 160;
  constexpr double kTol = 8.0;
  // UI box occupies [0,64)x[0,64) (top-left); sprite occupies [96,160)x[96,160) (bottom-right);
  // [64,96) is the neutral strip that must stay at the anchor colour in every case.
  constexpr int kUiCx = 32, kUiCy = 32;
  constexpr int kSpriteCx = 128, kSpriteCy = 128;
  constexpr int kNeutralCx = 80, kNeutralCy = 80;

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_ui_coexist_host", W, H)) {
    std::puts("draw2d_ui_coexist_sanity FAIL: host window create failed");
    return 1;
  }

  UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("draw2d_ui_coexist_sanity FAIL: ui attach failed");
    return 2;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_ui_coexist_sanity FAIL: Draw2d::init() failed");
    return 3;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_coexist_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_ui_coexist_sanity FAIL: mkdtemp failed");
    return 4;
  }
  const fs::path dir(dir_c);

  const fs::path p_sprite = dir / "sprite.tga";
  const fs::path p_rml = dir / "box.rml";
  write_file(p_sprite, build_tga_solid(8, 8, 200, 100, 50));
  write_ui_box_rml(p_rml, 64);

  Texture2d tex = d2d.load_texture(p_sprite.c_str());
  check(tex.ok(), "fixture: sprite.tga decoded");
  check(ui.load(p_rml.c_str()), "fixture: box.rml loaded");

  glViewport(0, 0, W, H);

  auto assert_both_layers = [&](const char* order_label) {
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb ui_mean = region_mean(px, W, H, kUiCx, kUiCy, 8);
    const Rgb sprite_mean = region_mean(px, W, H, kSpriteCx, kSpriteCy, 8);
    const Rgb neutral_mean = region_mean(px, W, H, kNeutralCx, kNeutralCy, 4);
    std::printf("draw2d_ui_coexist_sanity [%s]: ui=(%.1f,%.1f,%.1f) sprite=(%.1f,%.1f,%.1f) "
                "neutral=(%.1f,%.1f,%.1f)\n",
                order_label, ui_mean.r, ui_mean.g, ui_mean.b, sprite_mean.r, sprite_mean.g,
                sprite_mean.b, neutral_mean.r, neutral_mean.g, neutral_mean.b);
    check(near_rgb(ui_mean, 255, 255, 255, kTol),
          (std::string(order_label) + ": UI box still opaque white").c_str());
    check(near_rgb(sprite_mean, 200, 100, 50, kTol),
          (std::string(order_label) + ": sprite still its own solid colour").c_str());
    check(near_rgb(neutral_mean, 10, 20, 40, kTol),
          (std::string(order_label) + ": neutral strip untouched (anchor colour)").c_str());
    check(ui.ok(), (std::string(order_label) + ": ui.ok() survived the frame").c_str());
    check(d2d.ok(), (std::string(order_label) + ": d2d.ok() survived the frame").c_str());
  };

  // -------------------------------------------------------------------------------------------
  // Order A: sprite pass THEN ui_layer.render() -- the GusWorld production pattern (D6: world
  // under the UI).
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex, RectF{96, 96, 64, 64});
  d2d.end();
  ui.update();
  ui.render();
  assert_both_layers("sprite-then-ui");

  // -------------------------------------------------------------------------------------------
  // Order B: ui_layer.render() THEN sprite pass -- the reverse, D9's own explicit "both orders"
  // requirement (an overlay recipe a host is free to choose, D6).
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.update();
  ui.render();
  d2d.begin(W, H);
  d2d.draw_sprite(tex, RectF{96, 96, 64, 64});
  d2d.end();
  assert_both_layers("ui-then-sprite");

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_ui_coexist_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_ui_coexist_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
