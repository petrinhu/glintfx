// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_primitives_render_sanity (D2D-3B, onda 5 -- docs/superpowers/plans/2026-07-23-
//     onda5-draw2d-primitives.md, decisions D23-D30, section 5 item 3) -- pixel-readback proof
//     for the untextured primitives, layers, and scissor this wave adds, under Xvfb/llvmpipe,
//     STATISTICAL/tolerant, NEVER pixel-exact (same house discipline as draw2d_render_sanity.cpp/
//     draw2d_camera_render_sanity.cpp -- see this repo's own `golden_test` gotcha in AGENTS.md).
//     Embed fixture: a hidden `WindowGlfw` provides the current GL context, no `UiLayer`/RmlUi
//     involved. ONE window, created ONCE at 200x200 and reused for every check below.
//
//     Checks, each hand-derived from the LITERAL D8/D24/D25/D26/D27/D28 formulas (never "looks
//     right"):
//       1. draw_filled_rect: region mean equals the D8 premultiplied-tint formula on the white
//          texel (an opaque tint paints EXACTLY that colour); outside the rect stays background.
//       2. draw_rect_outline, alpha 0.5: the geometric CORNER region's blended mean equals a
//          straight EDGE region's mean within tolerance -- the D26 double-blend killer (a strip
//          formula bug that made two strips overlap would blend the corner TWICE, a measurable
//          jump this test would catch, even though the delivered non-overlapping strips make both
//          regions single-blended and therefore equal by construction).
//       3. draw_line under camera zoom 1 vs zoom 3, SAME world thickness: the screen band width
//          (painted-pixel count along a vertical scanline through a horizontal line) at zoom 3 is
//          ~3x the zoom-1 width -- the D24 "thickness scales by projection alone" delta test,
//          risk 3 of the plan's own section 0.
//       4. draw_filled_rect(transform overload) under camera rotation pi/2 vs no rotation: the
//          SAME world dst lands in a DIFFERENT screen quadrant, hand-derived from D12's
//          rot(v,-cam.rotation) formula -- same style as draw2d_camera_render_sanity.cpp's own
//          camera-rotation check, exercising the primitive's transform overload instead of a
//          textured sprite.
//       5. set_scissor: an off-center rect proven on BOTH y-halves of the viewport (the D28
//          y-flip killer -- an inverted mapping would move the painted region to the WRONG half,
//          failing loudly), plus an empty (`w == 0`) scissor clipping the ENTIRE draw.
//       6. set_layer: two overlapping translucent quads submitted in REVERSE layer order render
//          back-to-front (hand-derived premultiplied-blend colour proves WHICH one painted last,
//          i.e. is "on top"); the SAME pair submitted WITHOUT set_layer() proves plain call order
//          instead (a DIFFERENT, hand-derived colour) -- the D27 arming/stable-sort pin.
//       7. set_layer with sprites AND primitives interleaved, scrambled submission order across 3
//          layers, 2 DIFFERENT texture ids (a loaded sprite texture + the internal white texture)
//          -- proves texture-change flushes survive being replayed in a DIFFERENT order than they
//          were submitted (each shape still lands at its own position with its own colour, no
//          vertex/texture cross-contamination).
//     v0.21.0/onda-3/onda-4 regression coverage is NOT re-proven here (draw2d_render_sanity.cpp/
//     draw2d_camera_render_sanity.cpp/draw2d_ui_coexist_sanity.cpp/draw2d_hostile_sanity.cpp/
//     sprite_batch_sanity.cpp/transform2d_sanity.cpp run UNCHANGED and green, the published-
//     contract guard, D31).
// PT: draw2d_primitives_render_sanity (D2D-3B, onda 5 -- docs/superpowers/plans/2026-07-23-
//     onda5-draw2d-primitives.md, decisões D23-D30, seção 5 item 3) -- prova de pixel-readback
//     pras primitivas não-texturizadas, camadas e scissor que esta onda adiciona, sob
//     Xvfb/llvmpipe, ESTATÍSTICA/tolerante, NUNCA pixel-exata (mesma disciplina da casa de
//     draw2d_render_sanity.cpp/draw2d_camera_render_sanity.cpp -- ver o próprio gotcha do
//     `golden_test` no AGENTS.md). Fixture embed: um `WindowGlfw` oculto fornece o contexto GL
//     corrente, nenhum `UiLayer`/RmlUi envolvido. UMA janela, criada UMA vez em 200x200 e reusada
//     em todo check abaixo.
//
//     Checks, cada um derivado à mão das fórmulas literais D8/D24/D25/D26/D27/D28 (nunca "parece
//     certo"): ver a lista EN acima -- os mesmos sete itens, espelhados em pt-br nos comentários
//     de cada função de teste abaixo.
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
using glintfx::Camera2d;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::SpriteTransform;
using glintfx::Texture2d;
using glintfx::Vec2F;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Same hand-crafted, top-down, uncompressed 24bpp TGA idiom every sibling draw2d test file
//     uses (draw2d_render_sanity.cpp/draw2d_camera_render_sanity.cpp/draw2d_hostile_sanity.cpp)
//     -- exact, known colours.
// PT: Mesmo idioma de TGA 24bpp não-comprimido, top-down, feito à mão que todo arquivo de teste
//     irmão do draw2d usa -- cores exatas e conhecidas.
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

// EN: glReadPixels itself returns rows BOTTOM-up -- flipped here so the RETURNED buffer is
//     TOP-down, matching this file's own y-down convention (D5), same fix every sibling draw2d
//     test file already applies.
// PT: O próprio glReadPixels retorna linhas de BAIXO pra cima -- invertido aqui pra que o buffer
//     RETORNADO fique de CIMA pra baixo, batendo com a própria convenção y-para-baixo deste
//     arquivo (D5), o mesmo fix que todo arquivo de teste irmão do draw2d já aplica.
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

// EN: Counts pixels along the vertical scanline at column `x` whose colour is close to
//     `r,g,b` -- used by the D25/D24 line-thickness-scales-with-zoom check (item 3) to measure
//     the screen band width of a horizontal line without depending on exact edge-pixel rounding.
// PT: Conta pixels ao longo da linha de varredura vertical na coluna `x` cuja cor está perto de
//     `r,g,b` -- usado pelo cheque D25/D24 de espessura-de-linha-escala-com-zoom (item 3) pra
//     medir a largura de banda em tela de uma linha horizontal sem depender do arredondamento
//     exato de pixel de borda.
int count_painted_column(const std::vector<unsigned char>& px, int w, int h, int x, double r,
                         double g, double b, double tol) {
  int n = 0;
  for (int y = 0; y < h; ++y) {
    const std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 3;
    if (std::fabs(px[idx + 0] - r) <= tol && std::fabs(px[idx + 1] - g) <= tol &&
        std::fabs(px[idx + 2] - b) <= tol)
      ++n;
  }
  return n;
}

} // namespace

int main() {
  constexpr int W = 200, H = 200;
  constexpr double kBgR = 10.0, kBgG = 20.0, kBgB = 30.0; // a distinctive, known clear colour.

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_primitives_render_host", W, H)) {
    std::puts("draw2d_primitives_render_sanity FAIL: host window create failed");
    return 1;
  }
  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_primitives_render_sanity FAIL: Draw2d::init() failed");
    return 2;
  }
  glViewport(0, 0, W, H);

  auto clear_bg = [&]() {
    glClearColor(static_cast<float>(kBgR / 255.0), static_cast<float>(kBgG / 255.0),
                 static_cast<float>(kBgB / 255.0), 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
  };

  // -------------------------------------------------------------------------------------------
  // 1. draw_filled_rect -- the D8 premultiplied-tint formula on the white texel, item 3.1.
  // EN: An OPAQUE tint (alpha=1) on the white premultiplied texel yields `out = (1,1,1,1) *
  //     vec4(tint.rgb*1, 1) = (tint.r, tint.g, tint.b, 1)` -- the region must show EXACTLY that
  //     colour (fully opaque, no blend with the background), and outside the rect must stay the
  //     clear colour (D10's "silent legal no-op" does not apply here -- this is a normal draw).
  // PT: 1. draw_filled_rect -- a fórmula de tint premultiplicado D8 sobre o texel branco, item
  //     3.1. Um tint OPACO (alpha=1) sobre o texel branco premultiplicado produz `out = (1,1,1,1)
  //     * vec4(tint.rgb*1, 1) = (tint.r, tint.g, tint.b, 1)` -- a região precisa mostrar
  //     EXATAMENTE essa cor (totalmente opaca, sem blend com o fundo), e fora do retângulo
  //     precisa continuar na cor de clear.
  // -------------------------------------------------------------------------------------------
  {
    clear_bg();
    d2d.begin(W, H);
    d2d.draw_filled_rect(RectF{40.f, 40.f, 60.f, 60.f}, ColorF{0.2f, 0.6f, 0.9f, 1.f});
    d2d.end();
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb inside = region_mean(px, W, H, 70, 70, 10);
    const Rgb outside = region_mean(px, W, H, 10, 10, 5);
    check(near_rgb(inside, 51.0, 153.0, 229.5, 6.0),
          "filled_rect: region mean equals the D8 premultiplied opaque-tint colour exactly");
    check(near_rgb(outside, kBgR, kBgG, kBgB, 6.0), "filled_rect: outside the rect stays background");
  }

  // -------------------------------------------------------------------------------------------
  // 2. draw_rect_outline, alpha 0.5 -- the D26 double-blend killer, item 3.2.
  // EN: dst={40,40,80,80}, thickness=20 (t=min(20,40)=20, unclamped -- top/bottom/left/right tile
  //     dst exactly, D26). Single blend of tint{1,1,1,0.5} over the black-cleared background:
  //     premult_src=(0.5,0.5,0.5), a=0.5 -> out = 0.5 + bg*0.5. Both the geometric CORNER (near
  //     the top-left strip boundary, (45,45)) and a straight EDGE (mid of the top strip, (80,50))
  //     must read the SAME single-blend value -- a strip formula that overlapped them would
  //     double-blend the corner (out2 = 0.5 + out*0.5, visibly different).
  // PT: 2. draw_rect_outline, alpha 0.5 -- o mata-double-blend do D26, item 3.2. dst={40,40,80,80},
  //     thickness=20 (t=min(20,40)=20, sem clamp -- topo/base/esquerda/direita ladrilham dst
  //     exatamente, D26). Um blend único do tint{1,1,1,0.5} sobre o fundo clareado em preto:
  //     premult_src=(0.5,0.5,0.5), a=0.5 -> out = 0.5 + bg*0.5. Tanto o CANTO geométrico (perto da
  //     fronteira da faixa topo-esquerda, (45,45)) quanto uma BORDA reta (meio da faixa de topo,
  //     (80,50)) precisam ler o MESMO valor de blend único -- uma fórmula de faixa que os
  //     sobrepusesse daria double-blend no canto (out2 = 0.5 + out*0.5, visivelmente diferente).
  // -------------------------------------------------------------------------------------------
  {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_rect_outline(RectF{40.f, 40.f, 80.f, 80.f}, 20.f, ColorF{1.f, 1.f, 1.f, 0.5f});
    d2d.end();
    const auto px = read_backbuffer_rgb(W, H);
    const Rgb corner = region_mean(px, W, H, 45, 45, 2);
    const Rgb edge = region_mean(px, W, H, 80, 50, 4);
    const Rgb hole = region_mean(px, W, H, 80, 80, 4); // inside the outline's own hole -- untouched.
    check(near_rgb(corner, 127.5, 127.5, 127.5, 12.0), "outline: corner region is a SINGLE blend");
    check(near_rgb(edge, 127.5, 127.5, 127.5, 12.0), "outline: edge region is a SINGLE blend");
    check(std::fabs(corner.r - edge.r) <= 12.0, "outline: corner mean == edge mean (no double-blend)");
    check(near_rgb(hole, 0.0, 0.0, 0.0, 6.0), "outline: the hole region stays untouched (black)");
  }

  // -------------------------------------------------------------------------------------------
  // 3. draw_line, camera zoom 1 vs zoom 3, SAME world thickness -- the D24 delta test, risk 3,
  //    item 3.3.
  // EN: World a=(-15,0), b=(15,0), thickness=10, camera centred at origin. zoom 1: screen
  //     a=(85,100), b=(115,100), band width = 10px. zoom 3: screen a=(55,100), b=(145,100), band
  //     width = 30px (thickness scales BY PROJECTION alone, no separate formula, D24). The ratio
  //     assertion (not an absolute pixel count) is the delta test: a mutation that used
  //     `thickness` instead of `thickness/2` for `h` would make BOTH bands 2x too wide but the
  //     RATIO would still read ~3x (that specific mutation is primitives2d_sanity.cpp's own job,
  //     D25); a mutation that dropped the projection scale entirely (screen thickness independent
  //     of zoom) would make the ratio read ~1x here, failing loudly.
  // PT: 3. draw_line, câmera zoom 1 vs zoom 3, MESMA espessura de mundo -- o teste delta do D24,
  //     risco 3, item 3.3. Mundo a=(-15,0), b=(15,0), thickness=10, câmera centrada na origem.
  //     zoom 1: tela a=(85,100), b=(115,100), largura de banda = 10px. zoom 3: tela a=(55,100),
  //     b=(145,100), largura de banda = 30px (espessura escala SÓ POR PROJEÇÃO, sem fórmula
  //     separada, D24). A afirmação de RAZÃO (não uma contagem absoluta de pixel) é o teste delta:
  //     uma mutação que usasse `thickness` em vez de `thickness/2` pro `h` faria AS DUAS bandas
  //     2x largas demais mas a RAZÃO ainda leria ~3x (essa mutação específica é trabalho do
  //     próprio primitives2d_sanity.cpp, D25); uma mutação que descartasse a escala de projeção
  //     inteiramente (espessura de tela independente do zoom) faria a razão ler ~1x aqui, falhando
  //     alto.
  // -------------------------------------------------------------------------------------------
  {
    const ColorF line_color{1.f, 0.31f, 0.08f, 1.f}; // (255,80,20) opaque -- distinct from bg.
    auto band_width_at_zoom = [&](float zoom) -> int {
      clear_bg();
      d2d.set_camera(Camera2d{0.f, 0.f, zoom, 0.f});
      d2d.begin(W, H);
      d2d.draw_line(Vec2F{-15.f, 0.f}, Vec2F{15.f, 0.f}, 10.f, line_color);
      d2d.end();
      d2d.reset_camera();
      const auto px = read_backbuffer_rgb(W, H);
      return count_painted_column(px, W, H, 100, 255.0, 79.0, 20.0, 20.0);
    };
    const int width_zoom1 = band_width_at_zoom(1.f);
    const int width_zoom3 = band_width_at_zoom(3.f);
    std::printf("draw2d_primitives_render_sanity: line band width zoom1=%d zoom3=%d\n", width_zoom1,
                width_zoom3);
    check(width_zoom1 >= 6 && width_zoom1 <= 14, "line_zoom: zoom-1 band width is close to 10px");
    check(width_zoom3 >= width_zoom1 * 5 / 2 && width_zoom3 <= width_zoom1 * 7 / 2,
          "line_zoom: zoom-3 band width is ~3x the zoom-1 width (D24 projection-scales-thickness)");
  }

  // -------------------------------------------------------------------------------------------
  // 4. draw_filled_rect(transform overload) under camera rotation pi/2 -- quadrant permutation,
  //    item 3.4, same style as draw2d_camera_render_sanity.cpp's own camera-rotation check.
  // EN: World dst={20,-5,10,10} (centre (25,0), to the RIGHT of the origin). D12:
  //     world_to_screen = rot(local, -cam.rotation) + (W/2,H/2). rotation=0: screen centre =
  //     (125,100) (right of screen centre). rotation=pi/2: rot((25,0),-pi/2) = (0,-25) -> screen
  //     centre = (100,75) (ABOVE screen centre) -- hand-derived from rot(v,t) =
  //     (v.x*cos(t)-v.y*sin(t), v.x*sin(t)+v.y*cos(t)) with t=-pi/2 (cos=0,sin=-1):
  //     (25*0-0*(-1), 25*(-1)+0*0) = (0,-25).
  // PT: 4. draw_filled_rect(overload de transform) sob rotação de câmera pi/2 -- permutação de
  //     quadrante, item 3.4, mesmo estilo do próprio cheque de rotação de câmera de
  //     draw2d_camera_render_sanity.cpp. Mundo dst={20,-5,10,10} (centro (25,0), à DIREITA da
  //     origem). D12: world_to_screen = rot(local, -cam.rotation) + (W/2,H/2). rotation=0: centro
  //     de tela = (125,100) (direita do centro de tela). rotation=pi/2: rot((25,0),-pi/2) =
  //     (0,-25) -> centro de tela = (100,75) (ACIMA do centro de tela) -- derivado à mão de
  //     rot(v,t) = (v.x*cos(t)-v.y*sin(t), v.x*sin(t)+v.y*cos(t)) com t=-pi/2 (cos=0,sin=-1):
  //     (25*0-0*(-1), 25*(-1)+0*0) = (0,-25).
  // -------------------------------------------------------------------------------------------
  {
    const ColorF rect_color{0.9f, 0.1f, 0.7f, 1.f};
    auto probe_at_rotation = [&](float rotation) {
      clear_bg();
      d2d.set_camera(Camera2d{0.f, 0.f, 1.f, rotation});
      d2d.begin(W, H);
      d2d.draw_filled_rect(RectF{20.f, -5.f, 10.f, 10.f}, rect_color, SpriteTransform{});
      d2d.end();
      d2d.reset_camera();
      return read_backbuffer_rgb(W, H);
    };
    const auto px_no_rot = probe_at_rotation(0.f);
    const Rgb right_no_rot = region_mean(px_no_rot, W, H, 125, 100, 3);
    const Rgb above_no_rot = region_mean(px_no_rot, W, H, 100, 75, 3);
    check(near_rgb(right_no_rot, 229.5, 25.5, 178.5, 10.0),
          "filled_rect_transform_camera: rotation=0 -- rect lands to the RIGHT of centre");
    check(near_rgb(above_no_rot, kBgR, kBgG, kBgB, 6.0),
          "filled_rect_transform_camera: rotation=0 -- ABOVE-centre stays background");

    const auto px_rot = probe_at_rotation(static_cast<float>(M_PI / 2.0));
    const Rgb right_rot = region_mean(px_rot, W, H, 125, 100, 3);
    const Rgb above_rot = region_mean(px_rot, W, H, 100, 75, 3);
    check(near_rgb(above_rot, 229.5, 25.5, 178.5, 10.0),
          "filled_rect_transform_camera: rotation=pi/2 -- rect lands ABOVE centre (D12 permutation)");
    check(near_rgb(right_rot, kBgR, kBgG, kBgB, 6.0),
          "filled_rect_transform_camera: rotation=pi/2 -- RIGHT-of-centre stays background");
  }

  // -------------------------------------------------------------------------------------------
  // 5. set_scissor -- off-center rect on BOTH y-halves (the D28 y-flip killer), plus an empty
  //    scissor clipping everything, item 3.5.
  // EN: A full-viewport filled_rect under set_scissor({20,20,60,60}) (near the TOP -- small y,
  //     y-down convention) must paint ONLY inside that rect: a probe at (50,50) (inside) shows
  //     colour, a probe at (50,150) (the OPPOSITE y-half, near the BOTTOM) stays background. The
  //     y-flip killer: an inverted glScissor mapping would clip the WRONG half, flipping which of
  //     the two probes reads colour vs background -- so a second scissor near the BOTTOM
  //     ({20,120,60,60}) is asserted with the SAME two probes, roles swapped, proving the mapping
  //     is not an accidental match at one position. `reset_scissor()` between checks (D28's own
  //     sticky-until-reset contract) and at the very end (leaving no scissor behind for whatever
  //     runs after main() -- this file's own last line, defensive).
  // PT: 5. set_scissor -- retângulo fora do centro nas DUAS metades y (o mata-y-flip do D28), mais
  //     um scissor vazio clipando tudo, item 3.5. Um filled_rect de viewport-inteiro sob
  //     set_scissor({20,20,60,60}) (perto do TOPO -- y pequeno, convenção y-para-baixo) precisa
  //     pintar SÓ dentro daquele retângulo: uma sonda em (50,50) (dentro) mostra cor, uma sonda
  //     em (50,150) (a metade y OPOSTA, perto da BASE) continua fundo. O mata-y-flip: um
  //     mapeamento glScissor invertido cliparia a metade ERRADA, trocando qual das duas sondas lê
  //     cor vs fundo -- então um segundo scissor perto da BASE ({20,120,60,60}) é afirmado com as
  //     MESMAS duas sondas, papéis trocados, provando que o mapeamento não é uma coincidência
  //     numa só posição. `reset_scissor()` entre checks (o próprio contrato sticky-até-reset do
  //     D28) e no fim de tudo (não deixando scissor pra trás pro que rodar depois do main() --
  //     última linha deste próprio arquivo, defensiva).
  // -------------------------------------------------------------------------------------------
  {
    const ColorF fill_color{0.85f, 0.55f, 0.1f, 1.f};
    auto probe_scissor = [&](const RectF& scissor_rect) {
      clear_bg();
      d2d.set_scissor(scissor_rect);
      d2d.begin(W, H);
      d2d.draw_filled_rect(RectF{0.f, 0.f, static_cast<float>(W), static_cast<float>(H)}, fill_color);
      d2d.end();
      d2d.reset_scissor();
      return read_backbuffer_rgb(W, H);
    };

    const auto px_top = probe_scissor(RectF{20.f, 20.f, 60.f, 60.f});
    check(near_rgb(region_mean(px_top, W, H, 50, 50, 5), 216.75, 140.25, 25.5, 10.0),
          "scissor_top: inside the near-top scissor rect is painted");
    check(near_rgb(region_mean(px_top, W, H, 50, 150, 5), kBgR, kBgG, kBgB, 6.0),
          "scissor_top: the opposite (near-bottom) y-half stays background (the y-flip killer)");

    const auto px_bottom = probe_scissor(RectF{20.f, 120.f, 60.f, 60.f});
    check(near_rgb(region_mean(px_bottom, W, H, 50, 150, 5), 216.75, 140.25, 25.5, 10.0),
          "scissor_bottom: inside the near-bottom scissor rect is painted");
    check(near_rgb(region_mean(px_bottom, W, H, 50, 50, 5), kBgR, kBgG, kBgB, 6.0),
          "scissor_bottom: the opposite (near-top) y-half stays background -- roles swapped, "
          "proving the mapping is not a one-position coincidence");

    // Empty scissor (w==0) -- D28's own "clip everything" legal semantics.
    const auto px_empty = probe_scissor(RectF{20.f, 20.f, 0.f, 60.f});
    check(near_rgb(region_mean(px_empty, W, H, 100, 100, 20), kBgR, kBgG, kBgB, 6.0),
          "scissor_empty: w==0 clips the ENTIRE draw, nothing painted anywhere");
  }

  // -------------------------------------------------------------------------------------------
  // 6. set_layer -- reverse-order back-to-front, and plain call order without it, item 3.6.
  // EN: red={60,60,60,60} alpha 0.5, green={80,80,60,60} alpha 0.5, overlap region [80,120)^2,
  //     background BLACK (cleared here, not kBg -- keeps the premultiplied arithmetic exact).
  //     LAYERED: red submitted FIRST but tagged layer 5, green submitted SECOND but tagged layer
  //     1 -- ascending replay paints green(1) THEN red(5), red ends up ON TOP: overlap =
  //     red-over-(green-over-black) = (0.5,0.25,0) -> (127.5,63.75,0). CALL ORDER (no
  //     set_layer()): red drawn first, green drawn second, green ends up ON TOP: overlap =
  //     green-over-(red-over-black) = (0.25,0.5,0) -> (63.75,127.5,0) -- R/G channels SWAPPED
  //     relative to the layered case, the D27 arming/stable-sort pin.
  // PT: 6. set_layer -- trás-pra-frente em ordem invertida, e ordem de chamada simples sem ele,
  //     item 3.6. red={60,60,60,60} alpha 0.5, green={80,80,60,60} alpha 0.5, região de
  //     sobreposição [80,120)^2, fundo PRETO (clareado aqui, não kBg -- mantém a aritmética
  //     premultiplicada exata). BUFFERIZADO (COM CAMADA): red submetido PRIMEIRO mas marcado
  //     camada 5, green submetido SEGUNDO mas marcado camada 1 -- o replay ascendente pinta
  //     green(1) DEPOIS red(5), red termina POR CIMA: sobreposição =
  //     red-sobre-(green-sobre-preto) = (0.5,0.25,0) -> (127.5,63.75,0). ORDEM DE CHAMADA (sem
  //     set_layer()): red desenhado primeiro, green desenhado segundo, green termina POR CIMA:
  //     sobreposição = green-sobre-(red-sobre-preto) = (0.25,0.5,0) -> (63.75,127.5,0) -- canais
  //     R/G TROCADOS em relação ao caso com camada, a fixação de armar/ordenar-estável do D27.
  // -------------------------------------------------------------------------------------------
  {
    const RectF red_dst{60.f, 60.f, 60.f, 60.f};
    const RectF green_dst{80.f, 80.f, 60.f, 60.f};
    const ColorF red{1.f, 0.f, 0.f, 0.5f};
    const ColorF green{0.f, 1.f, 0.f, 0.5f};

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.set_layer(5);
    d2d.draw_filled_rect(red_dst, red); // submitted first, tagged the HIGHER layer.
    d2d.set_layer(1);
    d2d.draw_filled_rect(green_dst, green); // submitted second, tagged the LOWER layer.
    d2d.end();
    const auto px_layered = read_backbuffer_rgb(W, H);
    const Rgb overlap_layered = region_mean(px_layered, W, H, 100, 100, 5);
    check(near_rgb(overlap_layered, 127.5, 63.75, 0.0, 15.0),
          "layers_reverse: red (higher layer, submitted first) paints ON TOP -- red-dominant blend");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.draw_filled_rect(red_dst, red);     // no set_layer() -- plain call order, red first.
    d2d.draw_filled_rect(green_dst, green); // green second -- ends up on top.
    d2d.end();
    const auto px_call_order = read_backbuffer_rgb(W, H);
    const Rgb overlap_call_order = region_mean(px_call_order, W, H, 100, 100, 5);
    check(near_rgb(overlap_call_order, 63.75, 127.5, 0.0, 15.0),
          "layers_call_order: without set_layer(), green (drawn second) paints ON TOP instead");
    check(std::fabs(overlap_layered.r - overlap_call_order.r) > 20.0,
          "layers: the layered and call-order overlaps are MEASURABLY different (R channel)");
  }

  // -------------------------------------------------------------------------------------------
  // 7. set_layer with sprites AND primitives interleaved, scrambled submission order, 2 different
  //    texture ids -- proves texture-change flushes survive the layer-sort replay, item 3.6's
  //    own "sprites and primitives interleaved" clause.
  // EN: sprite (loaded texture, solid orange) at A={10,10,20,20} tagged layer 2 (submitted 1st);
  //     filled_rect (white texture, cyan tint) at B={40,10,20,20} tagged layer 0 (submitted 2nd);
  //     sprite again at C={70,10,20,20} tagged layer 1 (submitted 3rd). Final REPLAY order is
  //     B(0), C(1), A(2) -- neither submission order NOR texture-id order -- each region must
  //     still show its OWN colour, proving no vertex/texture cross-contamination when the
  //     texture id alternates white/orange/orange across the sorted sequence.
  // PT: 7. set_layer com sprites E primitivas intercalados, ordem de submissão embaralhada, 2 ids
  //     de textura diferentes -- prova que flushes de troca-de-textura sobrevivem ao replay
  //     ordenado-por-camada, a própria cláusula "sprites e primitivas intercalados" do item 3.6.
  //     sprite (textura carregada, laranja sólido) em A={10,10,20,20} marcado camada 2 (submetido
  //     1º); filled_rect (textura branca, tint ciano) em B={40,10,20,20} marcado camada 0
  //     (submetido 2º); sprite de novo em C={70,10,20,20} marcado camada 1 (submetido 3º). A
  //     ordem final de REPLAY é B(0), C(1), A(2) -- nem ordem de submissão NEM ordem de id de
  //     textura -- cada região precisa continuar mostrando a PRÓPRIA cor, provando nenhuma
  //     contaminação cruzada de vértice/textura quando o id de textura alterna branco/laranja/
  //     laranja pela sequência ordenada.
  // -------------------------------------------------------------------------------------------
  {
    char tmpl[] = "/var/tmp/glintfx_draw2d_primitives_XXXXXX";
    char* dir_c = mkdtemp(tmpl);
    if (!dir_c) {
      std::puts("draw2d_primitives_render_sanity FAIL: mkdtemp failed");
      return 3;
    }
    const fs::path dir(dir_c);
    const fs::path p_sprite = dir / "sprite.tga";
    write_file(p_sprite, build_tga_solid(4, 4, 255, 140, 0)); // solid orange.
    Texture2d tex = d2d.load_texture(p_sprite.c_str());
    check(tex.ok(), "layers_interleaved: fixture sprite loaded");

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    d2d.begin(W, H);
    d2d.set_layer(2);
    d2d.draw_sprite(tex, RectF{10.f, 10.f, 20.f, 20.f}); // A: submitted 1st, layer 2.
    d2d.set_layer(0);
    d2d.draw_filled_rect(RectF{40.f, 10.f, 20.f, 20.f}, ColorF{0.f, 1.f, 1.f, 1.f}); // B: 2nd, layer 0.
    d2d.set_layer(1);
    d2d.draw_sprite(tex, RectF{70.f, 10.f, 20.f, 20.f}); // C: submitted 3rd, layer 1.
    d2d.end();

    const auto px = read_backbuffer_rgb(W, H);
    check(near_rgb(region_mean(px, W, H, 20, 20, 5), 255.0, 140.0, 0.0, 6.0),
          "layers_interleaved: A (sprite, layer 2) still shows solid orange");
    check(near_rgb(region_mean(px, W, H, 50, 20, 5), 0.0, 255.0, 255.0, 6.0),
          "layers_interleaved: B (primitive, layer 0) still shows opaque cyan");
    check(near_rgb(region_mean(px, W, H, 80, 20, 5), 255.0, 140.0, 0.0, 6.0),
          "layers_interleaved: C (sprite, layer 1) still shows solid orange, no cross-contamination");

    std::error_code ec;
    fs::remove_all(dir, ec);
  }

  check(d2d.ok(), "draw2d_primitives_render_sanity: d2d survives the whole suite");
  d2d.reset_scissor(); // defensive: leave no scissor behind for whatever runs after this process.
  d2d.shutdown();

  if (g_failures == 0) {
    std::puts("draw2d_primitives_render_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_primitives_render_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
