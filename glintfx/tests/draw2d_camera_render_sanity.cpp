// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_camera_render_sanity (D2D-2B) -- pixel-readback proof of `Draw2d::set_camera`/
//     `reset_camera`/the `SpriteTransform` overload under Xvfb/llvmpipe, STATISTICAL/tolerant
//     (never pixel-exact, same house discipline as draw2d_render_sanity.cpp -- see this repo's
//     own `golden_test` gotcha in AGENTS.md). Embed fixture: a hidden `WindowGlfw` provides the
//     current GL context, no `UiLayer`/RmlUi involved -- this proves Draw2D's camera/transform
//     path standalone. ONE window, created ONCE at 160x160 and reused for every check below
//     (same idiom draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp already use --
//     `WindowGlfw::create()` is not meant to be called twice on one instance to "resize"; every
//     check's own hand-derived math is expressed against this fixed 160x160 viewport, centre
//     (80,80)). Every expected screen position/colour below is HAND-DERIVED from the LITERAL
//     D12/D18 formulas (plan docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md), not
//     "looks right" -- this is the readback half of D2D-2's own risk-2 antidote (a conversion
//     formula without a computed-value delta test is this house's named silent-bug class; the
//     CPU-side delta tests live in transform2d_sanity.cpp, D2D-2A -- this file proves the SAME
//     formulas end-to-end through the real GL path).
//
//     Six checks (plan section 5, `draw2d_camera_render_sanity`, plus two flip checks this
//     file's own gap-sweep added -- D18's flip_h/flip_v had zero coverage anywhere else, since
//     the UV-swap is implemented in draw2d.cpp, D2D-2B, not in transform2d.hpp's pure corner
//     math, D2D-2A, which does not touch flip_h/flip_v at all):
//       1. camera offset: a world sprite lands at screen centre when the camera is centred on
//          it, disappears from that point when the camera pans away, and reappears there when
//          the camera pans back.
//       2. zoom 2x: a world sprite that would occupy a QUARTER of the viewport at zoom 1x fills
//          the WHOLE viewport at zoom 2x -- proven by a point outside the zoom-1x footprint but
//          inside the zoom-2x one.
//       3. camera rotation pi/2 with an asymmetric 4-colour checker texture: the four screen
//          quadrants' dominant colours permute exactly as D12's rot(v,-cam.rotation) predicts.
//       4. `SpriteTransform.rotation` = pi/2 (NO camera): the same checker, the same four-
//          quadrant permutation check, but D18's rot(v,+rotation) (opposite sign convention
//          from the camera, D12 vs D18 -- a DIFFERENT permutation than check 3, hand-derived
//          independently) proves this is the sprite's OWN rotation, not a mislabelled repeat of
//          check 3.
//       5/6. `SpriteTransform.flip_h`/`flip_v` (NO camera): D18's own contract says flip swaps
//          UV on the RESOLVED src_px sub-rect -- these two checks are the gap-sweep this file
//          adds (see the file's own top-comment above), each a full-viewport checker readback
//          with the hand-derived quadrant permutation flip_h/flip_v produce.
//       7. mid-bracket `set_camera`/`reset_camera` switch, ONE texture, ONE bracket: a world
//          sprite (camera set) then a screen sprite (camera reset) both land where computed --
//          same texture across the switch is the D13 "camera switch never forces a flush"
//          story's readback-visible half; the OTHER half (SpriteBatch itself never sees the
//          camera at all, so it cannot possibly flush on a camera change) is a structural
//          property provable by inspection (set_camera()/reset_camera() touch ONLY
//          `Impl::has_camera`/`Impl::camera`, never `Impl::batch`) plus
//          sprite_batch_sanity.cpp's own `test_same_texture_accumulates` (D4) -- not
//          re-provable from a pixel readback, which cannot count GL draw calls without
//          instrumentation this suite does not have.
// PT: draw2d_camera_render_sanity (D2D-2B) -- prova de pixel-readback de `Draw2d::set_camera`/
//     `reset_camera`/do overload `SpriteTransform` sob Xvfb/llvmpipe, ESTATÍSTICA/tolerante
//     (nunca pixel-exata, mesma disciplina da casa de draw2d_render_sanity.cpp -- ver o próprio
//     gotcha do `golden_test` no AGENTS.md). Fixture embed: um `WindowGlfw` oculto fornece o
//     contexto GL corrente, nenhum `UiLayer`/RmlUi envolvido -- isto prova o caminho de
//     câmera/transform do Draw2D sozinho. UMA janela, criada UMA vez em 160x160 e reusada em
//     todo check abaixo (mesmo idioma que draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp
//     já usam -- `WindowGlfw::create()` não é feito pra ser chamado duas vezes na mesma
//     instância pra "redimensionar"; a matemática derivada à mão de cada check é expressa
//     contra este viewport fixo 160x160, centro (80,80)). Toda posição/cor de tela esperada
//     abaixo é DERIVADA À MÃO das fórmulas literais D12/D18 (plano
//     docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md), não "parece certo" -- este é o
//     antídoto de readback do próprio risco 2 do D2D-2 (fórmula de conversão sem teste de valor
//     computado é a classe de bug silencioso nomeada desta casa; os testes de delta CPU-side
//     moram em transform2d_sanity.cpp, D2D-2A -- este arquivo prova as MESMAS fórmulas
//     ponta-a-ponta pelo caminho GL real).
//
//     Seis checks (plano seção 5, `draw2d_camera_render_sanity`, mais dois checks de flip que o
//     próprio sweep de lacuna deste arquivo somou -- flip_h/flip_v do D18 tinham zero cobertura
//     em qualquer lugar, já que a troca de UV é implementada em draw2d.cpp, D2D-2B, não na
//     matemática pura de canto do transform2d.hpp, D2D-2A, que não toca flip_h/flip_v de jeito
//     nenhum):
//       1. offset de câmera: um sprite de mundo pousa no centro da tela quando a câmera está
//          centrada nele, some desse ponto quando a câmera se afasta, e reaparece lá quando a
//          câmera volta.
//       2. zoom 2x: um sprite de mundo que ocuparia um QUARTO do viewport em zoom 1x preenche o
//          viewport INTEIRO em zoom 2x -- provado por um ponto fora da pegada de zoom-1x mas
//          dentro da de zoom-2x.
//       3. rotação de câmera pi/2 com uma textura checker 4-cores assimétrica: as cores
//          dominantes dos quatro quadrantes de tela permutam exatamente como o
//          rot(v,-cam.rotation) do D12 prediz.
//       4. `SpriteTransform.rotation` = pi/2 (SEM câmera): o mesmo checker, o mesmo cheque de
//          permutação de quatro quadrantes, mas o rot(v,+rotation) do D18 (convenção de sinal
//          OPOSTA à da câmera, D12 vs D18 -- uma permutação DIFERENTE do check 3, derivada
//          independentemente) prova que é a rotação PRÓPRIA do sprite, não uma repetição
//          mal-rotulada do check 3.
//       5/6. `SpriteTransform.flip_h`/`flip_v` (SEM câmera): o próprio contrato do D18 diz que o
//          flip troca UV no sub-retângulo `src_px` RESOLVIDO -- estes dois checks são o
//          sweep de lacuna que este arquivo soma (ver o próprio comentário de topo do arquivo
//          acima), cada um um readback checker full-viewport com a permutação de quadrante
//          derivada à mão que flip_h/flip_v produzem.
//       7. troca de `set_camera`/`reset_camera` no MEIO do bracket, UMA textura, UM bracket: um
//          sprite de mundo (câmera setada) e depois um sprite de tela (câmera resetada) pousam
//          onde computado -- a mesma textura através da troca é a metade visível-por-readback
//          da história "trocar câmera nunca força flush" do D13; a OUTRA metade (o próprio
//          SpriteBatch nunca vê a câmera, então não pode fisicamente dar flush numa troca de
//          câmera) é uma propriedade estrutural provável por inspeção (set_camera()/
//          reset_camera() tocam SÓ `Impl::has_camera`/`Impl::camera`, nunca `Impl::batch`) mais
//          o próprio `test_same_texture_accumulates` (D4) de sprite_batch_sanity.cpp -- não
//          re-provável por um readback de pixel, que não consegue contar chamadas de draw GL
//          sem instrumentação que esta suíte não tem.
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

// EN: Same hand-crafted, top-down (image_descriptor 0x20), uncompressed 24bpp TGA idiom as
//     draw2d_render_sanity.cpp -- exact, known colours, not an opaque PNG fixture's own bytes.
// PT: Mesmo idioma de TGA 24bpp não-comprimido, top-down (image_descriptor 0x20), feito à mão de
//     draw2d_render_sanity.cpp -- cores exatas e conhecidas, não os próprios bytes opacos de uma
//     fixture PNG.
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
  f.push_back(0x20); // top-down.
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

// EN: 2x2 checker: row0 (TOP) = [tl, tr]; row1 (bottom) = [bl, br] -- same layout as
//     draw2d_render_sanity.cpp's own build_tga_checker2x2, re-derived here (different test
//     binary, same "not a fuzz campaign" reasoning those files already give for their own
//     fixture duplication).
// PT: Checker 2x2: linha0 (CIMA) = [tl, tr]; linha1 (baixo) = [bl, br] -- mesmo layout do próprio
//     build_tga_checker2x2 de draw2d_render_sanity.cpp, re-derivado aqui (binário de teste
//     diferente, mesmo racional de "não é campanha de fuzz" que aqueles arquivos já dão pra
//     própria duplicação de fixture).
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

// EN: glReadPixels returns rows BOTTOM-up -- flipped here so the RETURNED buffer is TOP-down,
//     matching this file's y-down `RectF`/region convention (D5), same fix
//     draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp already apply.
// PT: glReadPixels devolve linhas de BAIXO pra cima -- invertido aqui pra que o buffer RETORNADO
//     fique de CIMA pra baixo, batendo com a convenção y-para-baixo de `RectF`/região deste
//     arquivo (D5), o mesmo fix que draw2d_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp já
//     aplicam.
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
  constexpr int W = 160, H = 160; // ONE viewport for every check below (centre=(80,80)).
  constexpr double kTol = 8.0;    // axis-aligned checks (1/2/7) -- same tolerance draw2d_render_sanity uses.
  // EN: Rotated/flipped quads (checks 3-6) sample a small (2x2) GL_LINEAR-magnified texture
  //     across a NON-axis-aligned rasterized quad -- empirically confirmed (measured against
  //     hand-computed expectations while writing this file) to carry more llvmpipe
  //     interpolation noise than the axis-aligned case even at an interior sample point far
  //     from any texel boundary (observed deltas up to ~15/255 on a single channel, direction
  //     always matching the hand-derived expectation -- never a different quadrant's colour).
  //     A wider, still-tight tolerance, not a pixel-exact claim (this file's own top comment).
  // PT: Quads rotacionados/espelhados (checks 3-6) amostram uma textura pequena (2x2)
  //     magnificada por GL_LINEAR através de um quad rasterizado NÃO-alinhado aos eixos --
  //     confirmado empiricamente (medido contra as expectativas calculadas à mão ao escrever
  //     este arquivo) carregar mais ruído de interpolação do llvmpipe que o caso alinhado aos
  //     eixos mesmo num ponto de amostra interior, longe de qualquer fronteira de texel
  //     (deltas observados de até ~15/255 num único canal, direção sempre batendo com a
  //     expectativa derivada à mão -- nunca a cor de um quadrante diferente). Uma tolerância
  //     mais larga, ainda apertada, não um claim pixel-exato (o próprio comentário de topo
  //     deste arquivo).
  constexpr double kTolRotated = 24.0;

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_camera_render_host", W, H)) {
    std::puts("draw2d_camera_render_sanity FAIL: host window create failed");
    return 1;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_camera_render_sanity FAIL: Draw2d::init() failed");
    return 2;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_camera_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_camera_render_sanity FAIL: mkdtemp failed");
    return 3;
  }
  const fs::path dir(dir_c);

  const fs::path p_solid = dir / "solid.tga";     // R=200,G=100,B=50.
  const fs::path p_checker = dir / "checker.tga"; // tl=RED tr=GREEN bl=BLUE br=YELLOW.
  write_file(p_solid, build_tga_solid(8, 8, 200, 100, 50));
  write_file(p_checker, build_tga_checker2x2(255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0));

  Texture2d tex_solid = d2d.load_texture(p_solid.c_str());
  Texture2d tex_checker = d2d.load_texture(p_checker.c_str());
  check(tex_solid.ok(), "fixture: solid.tga decoded");
  check(tex_checker.ok() && tex_checker.width() == 2 && tex_checker.height() == 2,
        "fixture: checker.tga decoded");

  glViewport(0, 0, W, H); // D9: viewport is the host's own responsibility, not Draw2D's.

  // -----------------------------------------------------------------------------------------
  // Check 1: camera offset -- panned onto the sprite, panned away, panned back.
  // World sprite dst={40,40,20,20} (world rect [40,60]x[40,60], centre (50,50)). D12:
  // w2s(cam,W,H,p) = rot((p-cam)*zoom,-rot) + centre.
  //   Camera A {50,50,1,0}: centred = (50-50,50-50)=(0,0) -> screen (80,80) (viewport centre).
  //   Camera B {200,200,1,0} (panned away): centred=(50-200,50-200)=(-150,-150) ->
  //     screen=(80-150,80-150)=(-70,-70) -- the whole sprite is off-screen, viewport centre
  //     (80,80) shows the clear colour, not the sprite.
  // -----------------------------------------------------------------------------------------
  {
    const RectF world_dst{40.f, 40.f, 20.f, 20.f};

    glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{50.f, 50.f, 1.f, 0.f});
    d2d.begin(W, H);
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb centre = region_mean(px, W, H, 80, 80, 6);
      check(near_rgb(centre, 200, 100, 50, kTol),
            "camera_offset: camera centred on the sprite -> sprite lands at viewport centre");
    }

    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{200.f, 200.f, 1.f, 0.f});
    d2d.begin(W, H);
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb centre = region_mean(px, W, H, 80, 80, 6);
      check(near_rgb(centre, 10, 20, 40, kTol),
            "camera_offset: camera panned away -> viewport centre shows the clear colour, sprite invisible there");
    }

    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{50.f, 50.f, 1.f, 0.f}); // panned back.
    d2d.begin(W, H);
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb centre = region_mean(px, W, H, 80, 80, 6);
      check(near_rgb(centre, 200, 100, 50, kTol),
            "camera_offset: camera panned back -> sprite reappears at viewport centre");
    }
    d2d.reset_camera();
  }

  // -----------------------------------------------------------------------------------------
  // Check 2: zoom 2x doubles the world-to-screen footprint.
  // World sprite dst={-40,-40,80,80} (world rect centred at the origin, 80x80). D12 with
  // cam={0,0,zoom,0}: screen = (p*zoom) + centre (80,80).
  //   zoom=1: corners map to screen [40,120]x[40,120] -- a QUARTER of the viewport by area,
  //     centred. Point (10,10) is OUTSIDE that footprint -> stays the clear colour.
  //   zoom=2: corners map to screen [0,160]x[0,160] -- the WHOLE viewport. Point (10,10) is
  //     INSIDE that footprint now -> shows the sprite's own colour.
  // -----------------------------------------------------------------------------------------
  {
    const RectF world_dst{-40.f, -40.f, 80.f, 80.f};

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{0.f, 0.f, 1.f, 0.f});
    d2d.begin(W, H);
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb corner = region_mean(px, W, H, 10, 10, 4);
      check(near_rgb(corner, 0, 0, 0, kTol),
            "zoom_2x: at zoom 1x, (10,10) is outside the sprite's quarter-viewport footprint");
    }

    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{0.f, 0.f, 2.f, 0.f});
    d2d.begin(W, H);
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb corner = region_mean(px, W, H, 10, 10, 4);
      check(near_rgb(corner, 200, 100, 50, kTol),
            "zoom_2x: at zoom 2x, the SAME point (10,10) is now inside the doubled footprint");
    }
    d2d.reset_camera();
  }

  // -----------------------------------------------------------------------------------------
  // Check 3: camera rotation pi/2 permutes the checker's four screen quadrants.
  // World sprite dst={-80,-80,160,160} (world rect [-80,80]^2, exactly covers the viewport at
  // zoom 1/rotation 0). cam={0,0,1,pi/2}. D12: rot(v,-pi/2) = (v.y, -v.x)
  // (cos(-pi/2)=0, sin(-pi/2)=-1).
  //   world TL(-80,-80) -> rot=(-80, 80) -> screen=(0,160)  (bottom-left)
  //   world TR( 80,-80) -> rot=(-80,-80) -> screen=(0,0)    (top-left)
  //   world BR( 80, 80) -> rot=( 80,-80) -> screen=(160,0)  (top-right)
  //   world BL(-80, 80) -> rot=( 80, 80) -> screen=(160,160)(bottom-right)
  // texel TL=RED sits at screen bottom-left, TR=GREEN at screen top-left, BR=YELLOW at screen
  // top-right, BL=BLUE at screen bottom-right.
  // -----------------------------------------------------------------------------------------
  {
    const RectF world_dst{-80.f, -80.f, 160.f, 160.f};
    constexpr float kHalfPi = 1.57079632679489661923f;

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.set_camera(Camera2d{0.f, 0.f, 1.f, kHalfPi});
    d2d.begin(W, H);
    d2d.draw_sprite(tex_checker, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb top_left = region_mean(px, W, H, 40, 40, 8);
      const Rgb top_right = region_mean(px, W, H, 120, 40, 8);
      const Rgb bottom_right = region_mean(px, W, H, 120, 120, 8);
      const Rgb bottom_left = region_mean(px, W, H, 40, 120, 8);
      std::printf(
          "draw2d_camera_render_sanity: camera_rotation_pi2 TL=(%.1f,%.1f,%.1f) TR=(%.1f,%.1f,%.1f) "
          "BR=(%.1f,%.1f,%.1f) BL=(%.1f,%.1f,%.1f), expect GREEN/YELLOW/BLUE/RED\n",
          top_left.r, top_left.g, top_left.b, top_right.r, top_right.g, top_right.b, bottom_right.r,
          bottom_right.g, bottom_right.b, bottom_left.r, bottom_left.g, bottom_left.b);
      check(near_rgb(top_left, 0, 255, 0, kTolRotated), "camera_rotation_pi2: screen TL shows GREEN (world TR)");
      check(near_rgb(top_right, 255, 255, 0, kTolRotated), "camera_rotation_pi2: screen TR shows YELLOW (world BR)");
      check(near_rgb(bottom_right, 0, 0, 255, kTolRotated), "camera_rotation_pi2: screen BR shows BLUE (world BL)");
      check(near_rgb(bottom_left, 255, 0, 0, kTolRotated), "camera_rotation_pi2: screen BL shows RED (world TL)");
    }
    d2d.reset_camera();
  }

  // -----------------------------------------------------------------------------------------
  // Check 4: SpriteTransform.rotation = pi/2, NO camera -- opposite sign convention from D12,
  // a DIFFERENT permutation than check 3 (proves this is the sprite's OWN rotation).
  // dst={0,0,160,160} (full viewport, screen space), pivot=centre (default). D18:
  // corner' = pivot + rot(corner-pivot, +rotation) (positive sign, camera negates, D18 does
  // not). Same permutation transform2d_sanity.cpp's own test_d18_rotation_permutes_corners
  // already pins at the pure-math level (new_tl=old_tr, new_tr=old_br, new_br=old_bl,
  // new_bl=old_tl):
  //   c.tl lands at old TR position (160,0)   -> gets UV u0v0 = RED   -> screen (160,0) = RED
  //   c.tr lands at old BR position (160,160) -> gets UV u1v0 = GREEN -> screen (160,160)=GREEN
  //   c.br lands at old BL position (0,160)   -> gets UV u1v1 = YELLOW-> screen (0,160)  =YELLOW
  //   c.bl lands at old TL position (0,0)     -> gets UV u0v1 = BLUE  -> screen (0,0)    =BLUE
  // So: screen TL=BLUE, TR=RED, BR=GREEN, BL=YELLOW.
  // -----------------------------------------------------------------------------------------
  {
    const RectF dst{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)};
    constexpr float kHalfPi = 1.57079632679489661923f;
    SpriteTransform t{};
    t.rotation = kHalfPi;

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_checker, dst, RectF{}, ColorF{}, t);
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb top_left = region_mean(px, W, H, 40, 40, 8);
      const Rgb top_right = region_mean(px, W, H, 120, 40, 8);
      const Rgb bottom_right = region_mean(px, W, H, 120, 120, 8);
      const Rgb bottom_left = region_mean(px, W, H, 40, 120, 8);
      std::printf(
          "draw2d_camera_render_sanity: sprite_transform_rotation_pi2 TL=(%.1f,%.1f,%.1f) "
          "TR=(%.1f,%.1f,%.1f) BR=(%.1f,%.1f,%.1f) BL=(%.1f,%.1f,%.1f), expect BLUE/RED/GREEN/YELLOW\n",
          top_left.r, top_left.g, top_left.b, top_right.r, top_right.g, top_right.b, bottom_right.r,
          bottom_right.g, bottom_right.b, bottom_left.r, bottom_left.g, bottom_left.b);
      check(near_rgb(top_left, 0, 0, 255, kTolRotated), "sprite_transform_rotation_pi2: screen TL shows BLUE");
      check(near_rgb(top_right, 255, 0, 0, kTolRotated), "sprite_transform_rotation_pi2: screen TR shows RED");
      check(near_rgb(bottom_right, 0, 255, 0, kTolRotated), "sprite_transform_rotation_pi2: screen BR shows GREEN");
      check(near_rgb(bottom_left, 255, 255, 0, kTolRotated), "sprite_transform_rotation_pi2: screen BL shows YELLOW");
    }
  }

  // -----------------------------------------------------------------------------------------
  // Check 5: SpriteTransform.flip_h, NO camera, NO rotation -- D18: flip swaps UV on the
  // RESOLVED src_px sub-rect, implemented (draw2d.cpp, D2D-2B) as a slot re-pairing that
  // leaves the quad's screen footprint unchanged: left/right columns swap, top/bottom rows
  // stay put. dst={0,0,160,160}, identity rotation/scale (corners == dst's own corners).
  // Unflipped mapping is TL=RED,TR=GREEN,BL=BLUE,BR=YELLOW (the sentinel default) -- flip_h
  // swaps left<->right: screen TL=GREEN(was TR), TR=RED(was TL), BL=YELLOW(was BR),
  // BR=BLUE(was BL).
  // -----------------------------------------------------------------------------------------
  {
    const RectF dst{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)};
    SpriteTransform t{};
    t.flip_h = true;

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_checker, dst, RectF{}, ColorF{}, t);
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb top_left = region_mean(px, W, H, 40, 40, 8);
      const Rgb top_right = region_mean(px, W, H, 120, 40, 8);
      const Rgb bottom_left = region_mean(px, W, H, 40, 120, 8);
      const Rgb bottom_right = region_mean(px, W, H, 120, 120, 8);
      std::printf(
          "draw2d_camera_render_sanity: sprite_transform_flip_h TL=(%.1f,%.1f,%.1f) TR=(%.1f,%.1f,%.1f) "
          "BL=(%.1f,%.1f,%.1f) BR=(%.1f,%.1f,%.1f), expect GREEN/RED/YELLOW/BLUE\n",
          top_left.r, top_left.g, top_left.b, top_right.r, top_right.g, top_right.b, bottom_left.r,
          bottom_left.g, bottom_left.b, bottom_right.r, bottom_right.g, bottom_right.b);
      check(near_rgb(top_left, 0, 255, 0, kTolRotated), "sprite_transform_flip_h: screen TL shows GREEN (was TR)");
      check(near_rgb(top_right, 255, 0, 0, kTolRotated), "sprite_transform_flip_h: screen TR shows RED (was TL)");
      check(near_rgb(bottom_left, 255, 255, 0, kTolRotated), "sprite_transform_flip_h: screen BL shows YELLOW (was BR)");
      check(near_rgb(bottom_right, 0, 0, 255, kTolRotated), "sprite_transform_flip_h: screen BR shows BLUE (was BL)");
    }
  }

  // -----------------------------------------------------------------------------------------
  // Check 6: SpriteTransform.flip_v, NO camera, NO rotation -- top/bottom rows swap, left/right
  // columns stay put. Unflipped TL=RED,TR=GREEN,BL=BLUE,BR=YELLOW -- flip_v swaps top<->bottom:
  // screen TL=BLUE(was BL), TR=YELLOW(was BR), BL=RED(was TL), BR=GREEN(was TR).
  // -----------------------------------------------------------------------------------------
  {
    const RectF dst{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)};
    SpriteTransform t{};
    t.flip_v = true;

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_sprite(tex_checker, dst, RectF{}, ColorF{}, t);
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb top_left = region_mean(px, W, H, 40, 40, 8);
      const Rgb top_right = region_mean(px, W, H, 120, 40, 8);
      const Rgb bottom_left = region_mean(px, W, H, 40, 120, 8);
      const Rgb bottom_right = region_mean(px, W, H, 120, 120, 8);
      std::printf(
          "draw2d_camera_render_sanity: sprite_transform_flip_v TL=(%.1f,%.1f,%.1f) TR=(%.1f,%.1f,%.1f) "
          "BL=(%.1f,%.1f,%.1f) BR=(%.1f,%.1f,%.1f), expect BLUE/YELLOW/RED/GREEN\n",
          top_left.r, top_left.g, top_left.b, top_right.r, top_right.g, top_right.b, bottom_left.r,
          bottom_left.g, bottom_left.b, bottom_right.r, bottom_right.g, bottom_right.b);
      check(near_rgb(top_left, 0, 0, 255, kTolRotated), "sprite_transform_flip_v: screen TL shows BLUE (was BL)");
      check(near_rgb(top_right, 255, 255, 0, kTolRotated), "sprite_transform_flip_v: screen TR shows YELLOW (was BR)");
      check(near_rgb(bottom_left, 255, 0, 0, kTolRotated), "sprite_transform_flip_v: screen BL shows RED (was TL)");
      check(near_rgb(bottom_right, 0, 255, 0, kTolRotated), "sprite_transform_flip_v: screen BR shows GREEN (was TR)");
    }
  }

  // -----------------------------------------------------------------------------------------
  // Check 7: mid-bracket set_camera/reset_camera switch, ONE texture, ONE bracket. World
  // sprite via camera (screen centre (80,80), same math as check 1's camera A), then
  // reset_camera() and a screen-space HUD sprite (dst={10,10,20,20}, centre (20,20)) --
  // non-overlapping regions, same texture (tex_solid) throughout the bracket.
  // -----------------------------------------------------------------------------------------
  {
    const RectF world_dst{40.f, 40.f, 20.f, 20.f};
    const RectF hud_dst{10.f, 10.f, 20.f, 20.f};

    glClearColor(10.f / 255.f, 20.f / 255.f, 40.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.set_camera(Camera2d{50.f, 50.f, 1.f, 0.f});
    d2d.draw_sprite(tex_solid, world_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.reset_camera();
    d2d.draw_sprite(tex_solid, hud_dst, RectF{}, ColorF{}, SpriteTransform{});
    d2d.end();
    {
      const auto px = read_backbuffer_rgb(W, H);
      const Rgb world_mean = region_mean(px, W, H, 80, 80, 6);
      const Rgb hud_mean = region_mean(px, W, H, 20, 20, 6);
      const Rgb neutral_mean = region_mean(px, W, H, 140, 140, 4);
      check(near_rgb(world_mean, 200, 100, 50, kTol),
            "mid_bracket_camera_switch: world sprite (camera set) landed at its computed position");
      check(near_rgb(hud_mean, 200, 100, 50, kTol),
            "mid_bracket_camera_switch: HUD sprite (camera reset mid-bracket) landed at its computed position");
      check(near_rgb(neutral_mean, 10, 20, 40, kTol),
            "mid_bracket_camera_switch: untouched region stays the clear colour");
    }
    d2d.reset_camera();
  }

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_camera_render_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_camera_render_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
