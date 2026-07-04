// SPDX-License-Identifier: MPL-2.0
// EN: polygon_sanity -- proves the "polygon(<sides>, <color>[, <rotation>])" decorator (v0.2.6,
//     decorator_polygon.{hpp,cpp}) actually renders a REGULAR N-GON and not a filled rectangle,
//     and that changing `sides` changes the rendered SHAPE (not just a cosmetic no-op).
//
//     Scene (polygon_scene.rml/.rcss): two 200x200 squares at known absolute positions.
//       #hex  left=50,  top=50 -> decorator: polygon(6, #ffffff)  (default rotation, vertex up)
//       #oct  left=300, top=50 -> decorator: polygon(8, #ffffff)
//     Both boxes are exactly square, so for each one centre=(left+100, top+100) and the
//     inscribed-circle radius = 100 (half the shorter box dimension -- see the design-decision
//     comment in decorator_polygon.cpp's GenerateElementData).
//
//     Proof 1 -- "it's a polygon, not a filled rectangle" (the spec's own alpha-shape test):
//       for BOTH #hex and #oct, the box CENTRE must be opaque (white) and a point 5px inside
//       each box CORNER must be background (black). A corner-of-the-box point sits at distance
//       ~134.4px from the centre (Pythagoras on a 95,95 offset), which is always OUTSIDE any
//       inscribed-circle-radius-100 polygon regardless of `sides` -- a plain
//       "background-color: white" rectangle decorator would light up that corner; a polygon
//       decorator must not.
//
//     Proof 2 -- "changing `sides` changes the shape" (not just re-parsed and ignored):
//       sample a point 95px due EAST of each box's centre (same row, +95 on the x axis only --
//       no glReadPixels row-flip concern since only the column changes).
//         #hex (sides=6, vertex-up): east is the MIDPOINT of an edge (vertices sit at
//           -90+/-30 i.e. the two neighbours straddle east) -> boundary there is the apothem,
//           radius*cos(30 deg) = 86.6px < 95px -> the sample point is OUTSIDE -> background.
//         #oct (sides=8, vertex-up): east IS exactly a vertex (vertices at -90 + n*45 deg
//           include 0 deg = east) -> boundary there is the full radius, 100px > 95px -> the
//           sample point is INSIDE -> opaque (white).
//       This single pair of assertions (hex-east dark, oct-east bright) is only possible if the
//       renderer actually tessellates a DIFFERENT polygon for sides=6 vs sides=8 -- it cannot
//       pass by accident (e.g. from a bug that ignores `sides` and always draws a hexagon, or
//       always draws a circle-ish blob).
//
// PT: polygon_sanity -- prova que o decorator "polygon(<lados>, <cor>[, <rotacao>])" (v0.2.6,
//     decorator_polygon.{hpp,cpp}) de fato renderiza um N-ágono REGULAR e não um retângulo
//     preenchido, e que mudar `sides` muda a FORMA renderizada (não é um no-op cosmético).
//
//     Cena (polygon_scene.rml/.rcss): dois quadrados 200x200 em posições absolutas conhecidas.
//       #hex  left=50,  top=50 -> decorator: polygon(6, #ffffff)  (rotação padrão, vértice p/ cima)
//       #oct  left=300, top=50 -> decorator: polygon(8, #ffffff)
//     Ambas as caixas são exatamente quadradas, então para cada uma centro=(left+100, top+100)
//     e o raio do círculo inscrito = 100 (metade da dimensão menor da caixa -- ver o comentário
//     de decisão de design em GenerateElementData de decorator_polygon.cpp).
//
//     Prova 1 -- "é um polígono, não um retângulo preenchido" (o próprio teste de alpha-shape
//     pedido na spec): para AMBOS #hex e #oct, o CENTRO da caixa deve ser opaco (branco) e um
//     ponto 5px pra dentro de cada CANTO da caixa deve ser background (preto). Um ponto de
//     canto-da-caixa fica a ~134.4px do centro (Pitágoras num offset 95,95), o que está sempre
//     FORA de qualquer polígono de raio-de-círculo-inscrito-100 independente de `sides` -- um
//     decorator raso "background-color: white" acenderia esse canto; um decorator de polígono
//     não pode.
//
//     Prova 2 -- "mudar `sides` muda a forma" (não é só reparseado e ignorado): amostra um
//     ponto 95px a LESTE do centro de cada caixa (mesma linha, +95 só no eixo x -- sem
//     preocupação de flip de linha do glReadPixels já que só a coluna muda).
//       #hex (sides=6, vértice p/ cima): leste é o PONTO-MÉDIO de uma aresta (vértices ficam em
//         -90+/-30, os dois vizinhos ladeiam o leste) -> a borda ali é o apótema,
//         raio*cos(30 graus) = 86.6px < 95px -> o ponto amostrado fica FORA -> background.
//       #oct (sides=8, vértice p/ cima): leste É exatamente um vértice (vértices em -90 + n*45
//         graus incluem 0 grau = leste) -> a borda ali é o raio cheio, 100px > 95px -> o ponto
//         amostrado fica DENTRO -> opaco (branco).
//     Esse único par de asserções (hex-leste escuro, oct-leste claro) só é possível se o
//     renderer de fato tesselar um polígono DIFERENTE para sides=6 vs sides=8 -- não pode
//     passar por acidente (ex.: um bug que ignora `sides` e sempre desenha um hexágono, ou
//     sempre desenha um blob circular).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h (gl3w already loaded by UiLayer ctor).
                           // PT: inclui GL/gl3w.h (gl3w já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) -- same methodology as
//     dp_ratio_sanity.cpp / ua_stylesheet_sanity.cpp's capture().
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK) -- mesma metodologia
//     de dp_ratio_sanity.cpp / ua_stylesheet_sanity.cpp.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> capture(glintfx::UiLayer& ui, int w, int h) {
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

// ---------------------------------------------------------------------------
// EN: Sample the pixel at RCSS content-space coordinates (top-left origin, y-down -- the same
//     space RCSS "left"/"top"/"width"/"height" are authored in). glReadPixels' row 0 is the
//     window BOTTOM (see viewport_origin_sanity.cpp's header comment for the same convention),
//     so the row is flipped here; the column (x) needs no flip.
// PT: Amostra o pixel em coordenadas de espaço-conteúdo do RCSS (origem topo-esquerda, y pra
//     baixo -- o mesmo espaço em que "left"/"top"/"width"/"height" do RCSS são autorados). A
//     linha 0 do glReadPixels é a BASE da janela (ver o comentário de cabeçalho de
//     viewport_origin_sanity.cpp para a mesma convenção), então a linha é invertida aqui; a
//     coluna (x) não precisa de flip.
// ---------------------------------------------------------------------------
static const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int h,
                                      int content_x, int content_y) {
  const int row = h - 1 - content_y;
  const int col = content_x;
  return &px[(static_cast<size_t>(row) * w + col) * 3];
}

static bool is_opaque_white(const unsigned char* p) { return p[0] > 200 && p[1] > 200 && p[2] > 200; }
static bool is_background(const unsigned char* p)   { return p[0] < 50  && p[1] < 50  && p[2] < 50;  }

int main() {
  constexpr int W = 560, H = 300;

  glintfx::WindowGlfw host;
  if (!host.create("polygon_sanity_host", W, H)) {
    std::puts("polygon_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("polygon_sanity FAIL: ui attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.load("polygon_scene.rml");

  const auto px = capture(ui, W, H);

  // EN: #hex box centre=(150,150), radius=100.  #oct box centre=(400,150), radius=100.
  // PT: Caixa #hex centro=(150,150), raio=100.  Caixa #oct centro=(400,150), raio=100.
  const auto* hex_center = pixel_at(px, W, H, 150, 150);
  const auto* hex_corner = pixel_at(px, W, H, 55, 55);   // 5px inside the (50,50) corner.
  const auto* hex_east   = pixel_at(px, W, H, 245, 150); // centre.x + 95.

  const auto* oct_center = pixel_at(px, W, H, 400, 150);
  const auto* oct_corner = pixel_at(px, W, H, 305, 55);  // 5px inside the (300,50) corner.
  const auto* oct_east   = pixel_at(px, W, H, 495, 150); // centre.x + 95.

  std::printf("polygon_sanity: hex center=(%d,%d,%d) corner=(%d,%d,%d) east=(%d,%d,%d)\n",
              hex_center[0], hex_center[1], hex_center[2],
              hex_corner[0], hex_corner[1], hex_corner[2],
              hex_east[0], hex_east[1], hex_east[2]);
  std::printf("polygon_sanity: oct center=(%d,%d,%d) corner=(%d,%d,%d) east=(%d,%d,%d)\n",
              oct_center[0], oct_center[1], oct_center[2],
              oct_corner[0], oct_corner[1], oct_corner[2],
              oct_east[0], oct_east[1], oct_east[2]);

  // ---------------------------------------------------------------------------
  // EN: Proof 1 -- alpha shape: centre opaque, corner transparent (both boxes).
  // PT: Prova 1 -- alpha shape: centro opaco, canto transparente (ambas as caixas).
  // ---------------------------------------------------------------------------
  if (!is_opaque_white(hex_center)) {
    std::fprintf(stderr, "polygon_sanity FAIL: #hex centre is not opaque white -- polygon did not render\n");
    return 3;
  }
  if (!is_background(hex_corner)) {
    std::fprintf(stderr, "polygon_sanity FAIL: #hex box corner is filled -- decorator drew a rectangle, not a hexagon\n");
    return 4;
  }
  if (!is_opaque_white(oct_center)) {
    std::fprintf(stderr, "polygon_sanity FAIL: #oct centre is not opaque white -- polygon did not render\n");
    return 5;
  }
  if (!is_background(oct_corner)) {
    std::fprintf(stderr, "polygon_sanity FAIL: #oct box corner is filled -- decorator drew a rectangle, not an octagon\n");
    return 6;
  }

  // ---------------------------------------------------------------------------
  // EN: Proof 2 -- sides changes the shape: hex-east is an edge midpoint (outside at r=95),
  //     oct-east is a vertex (inside at r=95).
  // PT: Prova 2 -- sides muda a forma: hex-leste é ponto-médio de aresta (fora em r=95),
  //     oct-leste é vértice (dentro em r=95).
  // ---------------------------------------------------------------------------
  if (!is_background(hex_east)) {
    std::fprintf(stderr,
        "polygon_sanity FAIL: #hex east sample (r=95) is filled -- expected background "
        "(hexagon apothem at east ~= 86.6px < 95px)\n");
    return 7;
  }
  if (!is_opaque_white(oct_east)) {
    std::fprintf(stderr,
        "polygon_sanity FAIL: #oct east sample (r=95) is background -- expected opaque "
        "(octagon has a vertex at east, radius=100px > 95px)\n");
    return 8;
  }

  std::puts("polygon_sanity: PASS");
  return 0;
}
