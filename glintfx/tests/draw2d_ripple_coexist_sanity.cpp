// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_ripple_coexist_sanity (D2D-1B follow-up, review adversarial brief 8.4 item 3) --
//     the risk the WAVE'S PLAN itself named and no test of the wave covered:
//     `EnsureBackdropCaptured` (render_gl3.cpp, L1.22-CAPTURE) reads FBO 0 mid-`Context::Render()`
//     to feed the `ripple()` decorator, and a Draw2D sprite pass now changes what FBO 0 contains
//     when the UI renders after it -- CORRECT behaviour by design (the ripple refracting the world
//     under it is arguably the whole point), but nothing in `draw2d_ui_coexist_sanity.cpp`
//     exercises ripple+sprite together (that file's own oracle is D9's GL-state contract, not
//     backdrop-capture content). Adapted from the review's own scratch probe
//     (`qa_ripple_draw2d_coexist.cpp`, adversarial finding, brief 8.4 item 3), restyled to house
//     convention and promoted into the permanent suite -- a risk the plan names and only a review
//     probe covers goes back to being uncovered the moment the review ends.
//
//     Two orders, same oracle shape as draw2d_ui_coexist_sanity.cpp:
//       - sprite-then-ui: the sprite pass runs BEFORE ui.render() (D6, the GusWorld production
//         pattern) with an active ripple centred on the sprite's own top-left corner -- the ring
//         must visibly refract the SPRITE's own colour into the background (proves
//         EnsureBackdropCaptured saw the sprite pass, not stale/garbage FBO 0 content), while a
//         control point far from the ring (d=60 > ripple-width=50, `smoothstep` clamps to exactly
//         1 there per the GLSL spec) stays sprite-exact, zero ripple contribution.
//       - ui-then-sprite: ui.render() (ripple captures FBO 0 BEFORE the sprite exists this frame)
//         runs first, then the sprite pass -- correct-by-construction expectation (D9): the
//         capture must NOT show a sprite that was not there yet (no crash, no garbage), and the
//         LATER sprite draw must be unaffected by whatever GL state the ripple/UI pass left (D9's
//         own claim, exercised here with a real fx decorator instead of a plain white div).
//     Zero crash, zero ASan finding in either order is the actual gate; the hand-derived colour
//     oracle (same `result = sampled_rgb*ring + original_dst*(1-ring)` formula
//     ripple_sanity.cpp/draw2d_ui_coexist_sanity.cpp already use) is the proof it is not just
//     "did not crash" but pixel-correct.
//
//     Needs BOTH GLINTFX_MODULE_FX (ripple() itself) and GLINTFX_MODULE_DRAW2D (the sprite pass)
//     -- gated on both in tests/CMakeLists.txt, same "lean-ui build must not try to link a
//     decorator that was never compiled in" reasoning ripple_sanity.cpp's own CMake comment
//     gives. Runs in BOTH build configs (embed-mode fixture, links ${_embed_dep} glloader, same
//     pair as ripple_sanity.cpp) -- the decorator + Draw2D are both backend-agnostic.
// PT: draw2d_ripple_coexist_sanity (follow-up do D2D-1B, brief de review adversarial 8.4 item 3)
//     -- o risco que o PRÓPRIO PLANO da onda nomeou e nenhum teste da onda cobria:
//     `EnsureBackdropCaptured` (render_gl3.cpp, L1.22-CAPTURE) lê o FBO 0 no meio do
//     `Context::Render()` pra alimentar o decorator `ripple()`, e um passe de sprite do Draw2D
//     agora muda o que o FBO 0 contém quando a UI renderiza depois dele -- comportamento CORRETO
//     por design (o ripple refratar o mundo por baixo dele é arguavelmente o ponto inteiro), mas
//     nada em `draw2d_ui_coexist_sanity.cpp` exercita ripple+sprite juntos (o próprio oráculo
//     daquele arquivo é o contrato de estado GL da D9, não o conteúdo da captura de backdrop).
//     Adaptado da própria sonda de scratch do review (`qa_ripple_draw2d_coexist.cpp`, achado
//     adversarial, brief 8.4 item 3), restilizado pra convenção da casa e promovido pra suíte
//     permanente -- um risco que o plano nomeia e que só uma sonda de review cobre volta a ficar
//     descoberto assim que o review acaba.
//
//     Duas ordens, mesma forma de oráculo de draw2d_ui_coexist_sanity.cpp:
//       - sprite-then-ui: o passe de sprite roda ANTES de ui.render() (D6, o padrão de produção
//         do GusWorld) com um ripple ativo centrado no próprio canto superior-esquerdo do sprite
//         -- o anel precisa refratar visivelmente a própria cor do SPRITE pro background (prova
//         que EnsureBackdropCaptured viu o passe de sprite, não conteúdo obsoleto/lixo do FBO 0),
//         enquanto um ponto de controle longe do anel (d=60 > ripple-width=50, `smoothstep`
//         clampa em exatamente 1 ali pela própria spec GLSL) fica sprite-exato, zero contribuição
//         de ripple.
//       - ui-then-sprite: ui.render() (o ripple captura o FBO 0 ANTES do sprite existir neste
//         frame) roda primeiro, depois o passe de sprite -- expectativa correta-por-construção
//         (D9): a captura NÃO pode mostrar um sprite que ainda não existia (sem crash, sem lixo),
//         e o desenho de sprite POSTERIOR precisa ficar imune a qualquer estado GL que o passe de
//         ripple/UI tenha deixado (o próprio claim da D9, exercitado aqui com um decorator de fx
//         de verdade em vez de um div branco plano).
//     Zero crash, zero achado do ASan em qualquer uma das duas ordens é o gate de fato; o oráculo
//     de cor derivado à mão (mesma fórmula `result = sampled_rgb*ring + original_dst*(1-ring)`
//     que ripple_sanity.cpp/draw2d_ui_coexist_sanity.cpp já usam) é a prova de que não é só "não
//     crashou", é correto por pixel.
//
//     Precisa TANTO de GLINTFX_MODULE_FX (o próprio ripple()) QUANTO de GLINTFX_MODULE_DRAW2D (o
//     passe de sprite) -- gateado nos dois em tests/CMakeLists.txt, mesmo racional "build lean-ui
//     não pode tentar linkar um decorator nunca compilado" que o próprio comentário CMake de
//     ripple_sanity.cpp dá. Roda em AMBAS as configs de build (fixture embed-mode, linka
//     ${_embed_dep} glloader, mesmo par de ripple_sanity.cpp) -- o decorator + o Draw2D são os
//     dois backend-agnósticos.
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

// EN: Same hand-crafted-TGA idiom as draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp -- an
//     exact, known solid colour, independent of any PNG fixture's own opaque byte values.
// PT: Mesmo idioma de TGA feito-à-mão de draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp --
//     uma cor sólida exata e conhecida, independente dos próprios valores de byte opacos de
//     qualquer fixture PNG.
std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                           unsigned char b) {
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
  f.push_back(0x20); // top-down.
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

bool write_rml(const fs::path& path) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "<rml>\n<head>\n<link type=\"text/rcss\" href=\"scene.rcss\"/>\n</head>\n"
       "<body>\n<div id=\"overlay\"></div>\n</body>\n</rml>\n";
  return f.good();
}

// EN: Same hand-derived-math shape as ripple_sanity.cpp's own zone1 (origin ON a colour
//     boundary, sample points 30px either side along a single axis, so `d` is exactly 30 and the
//     SAME worked numbers apply: width=50/strength=150/phase=0 -> ring=0.352). `origin` is the
//     MIDPOINT of the sprite's own left edge (sprite box is [200,264)x[200,264)).
// PT: Mesma forma de matemática derivada à mão do próprio zone1 de ripple_sanity.cpp (origem
//     NUMA fronteira de cor, pontos de amostra 30px de cada lado ao longo de um único eixo, então
//     `d` é exatamente 30 e os MESMOS números derivados se aplicam: width=50/strength=150/
//     phase=0 -> ring=0,352). `origin` é o PONTO MÉDIO da própria borda esquerda do sprite (caixa
//     do sprite é [200,264)x[200,264)).
bool write_rcss(const fs::path& path) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "body { margin: 0px; padding: 0px; }\n"
       "#overlay { position: absolute; left: 0px; top: 0px; width: 400px; height: 400px;\n"
       "  ripple-origin-x: 200; ripple-origin-y: 232; ripple-phase: 0; ripple-strength: 150; "
       "ripple-width: 50; }\n"
       "#overlay.active { decorator: ripple(300); }\n";
  return f.good();
}

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

const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int x, int y) {
  return &px[(static_cast<std::size_t>(y) * w + x) * 3];
}

bool near(int v, int expect, int tol) { return v > expect - tol && v < expect + tol; }

} // namespace

int main() {
  constexpr int W = 400, H = 400;

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_ripple_coexist_host", W, H)) {
    std::puts("draw2d_ripple_coexist_sanity FAIL: host window create failed");
    return 1;
  }

  UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("draw2d_ripple_coexist_sanity FAIL: ui attach failed");
    return 2;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_ripple_coexist_sanity FAIL: Draw2d::init() failed");
    return 3;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_ripple_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_ripple_coexist_sanity FAIL: mkdtemp failed");
    return 4;
  }
  const fs::path dir(dir_c);

  const fs::path p_sprite = dir / "sprite.tga";
  const fs::path p_rml = dir / "scene.rml";
  const fs::path p_rcss = dir / "scene.rcss";
  write_file(p_sprite, build_tga_solid(64, 64, 200, 30, 30)); // a distinct "sprite red".
  write_rml(p_rml);
  write_rcss(p_rcss);

  Texture2d tex = d2d.load_texture(p_sprite.c_str());
  check(tex.ok(), "fixture: sprite.tga decoded");
  check(ui.load(p_rml.c_str()), "fixture: scene.rml loaded");
  check(ui.add_class("overlay", "active"), "fixture: ripple activated");

  glViewport(0, 0, W, H);

  // -------------------------------------------------------------------------------------------
  // Order A: sprite pass (bottom-right quadrant, [200,264)x[200,264)) THEN ui.render() with an
  // active ripple centred exactly on the sprite's own top-left corner -- the ring must visibly
  // refract the sprite's own colour into the background quadrant (a leak), same "leak across
  // boundary" oracle draw2d_ui_coexist_sanity.cpp/ripple_sanity.cpp already use, just with the
  // boundary being a SPRITE this time, not a UI div.
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  d2d.begin(W, H);
  d2d.draw_sprite(tex, RectF{200.f, 200.f, 64.f, 64.f});
  d2d.end();
  ui.update();
  ui.render();
  {
    const auto px = read_backbuffer_rgb(W, H);
    // Far control, INSIDE the sprite box but at d=60 from origin(200,232) -- smoothstep(0,50,x)
    // clamps to EXACTLY 1 for x>=50 (GLSL spec), so ring==0 exactly here: must be sprite-exact,
    // no ripple contribution whatsoever.
    const auto* sprite_ctrl = pixel_at(px, W, 260, 232);
    std::printf(
        "draw2d_ripple_coexist_sanity [sprite-then-ui]: sprite_ctrl=(%d,%d,%d), "
        "expect (200,30,30)\n",
        sprite_ctrl[0], sprite_ctrl[1], sprite_ctrl[2]);
    check(sprite_ctrl[0] == 200 && sprite_ctrl[1] == 30 && sprite_ctrl[2] == 30,
          "[sprite-then-ui] sprite interior untouched by ripple far from the ring (d=60>width=50)");
    // (170,232): background side, exactly 30px LEFT of origin(200,232) along a single axis --
    // hand-derived expected colour (same formula ripple_sanity.cpp/draw2d_ui_coexist_sanity.cpp
    // use, `result = sampled_rgb*ring + original_dst*(1-ring)`, ring=0.352):
    //   R: 10*0.648 + 200*0.352 = 76.88   G: 20*0.648 + 30*0.352 = 23.52   B: 40*0.648 + 30*0.352 = 36.48
    const auto* leak = pixel_at(px, W, 170, 232);
    std::printf(
        "draw2d_ripple_coexist_sanity [sprite-then-ui]: leak-point=(%d,%d,%d), "
        "expect ~(77,24,36) (background=10,20,40 sprite=200,30,30, ring=0.352)\n",
        leak[0], leak[1], leak[2]);
    check(near(leak[0], 77, 20) && near(leak[1], 24, 15) && near(leak[2], 36, 15),
          "[sprite-then-ui] the sprite's own colour leaked into the background at the "
          "hand-derived ripple sample point (proves EnsureBackdropCaptured saw the sprite pass, "
          "not garbage/stale FBO 0 content)");
    check(ui.ok() && d2d.ok(), "[sprite-then-ui] both layers survived the frame");
  }

  // -------------------------------------------------------------------------------------------
  // Order B: ui.render() (ripple active, captures whatever is in FBO 0 at that moment -- the
  // clear colour only, since the sprite has not been drawn yet this frame) THEN the sprite pass.
  // Correct-by-construction expectation (D6/D9): the ripple must NOT show the sprite (it did not
  // exist at capture time) -- no crash, no garbage -- and the LATER sprite draw is unaffected by
  // whatever state the UI pass left (D9's own claim under direct test).
  // -------------------------------------------------------------------------------------------
  glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.update();
  ui.render();
  d2d.begin(W, H);
  d2d.draw_sprite(tex, RectF{200.f, 200.f, 64.f, 64.f});
  d2d.end();
  {
    const auto px = read_backbuffer_rgb(W, H);
    const auto* sprite_ctrl = pixel_at(px, W, 232, 232);
    std::printf(
        "draw2d_ripple_coexist_sanity [ui-then-sprite]: sprite_ctrl=(%d,%d,%d), "
        "expect (200,30,30) -- sprite drawn AFTER ripple captured, must still show the "
        "sprite itself untouched\n",
        sprite_ctrl[0], sprite_ctrl[1], sprite_ctrl[2]);
    check(sprite_ctrl[0] == 200 && sprite_ctrl[1] == 30 && sprite_ctrl[2] == 30,
          "[ui-then-sprite] sprite pass drawn cleanly on top, unaffected by the ripple pass's "
          "own GL state");
    check(ui.ok() && d2d.ok(), "[ui-then-sprite] both layers survived the frame");
  }

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_ripple_coexist_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_ripple_coexist_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
