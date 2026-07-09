// SPDX-License-Identifier: MPL-2.0
// EN: polygon_sanity -- proves the "polygon(<sides>, <color>[, <rotation>])" decorator (v0.2.6,
//     decorator_polygon.{hpp,cpp}) renders a REGULAR N-GON (not a filled rectangle), that
//     changing `sides` changes the SHAPE, that a (negative) `rotation` is applied, AND that the
//     input-hardening surface (v0.2.6 review-fix) rejects invalid / hostile `sides` values
//     fail-high instead of silently rendering something.
//
//     Scene (polygon_scene.rml/.rcss):
//       Row 1 -- geometry proofs (three 200x200 squares, centre=(left+100, top+100), radius=100):
//         #hex left=50   polygon(6, #fff)         default rotation, vertex up
//         #oct left=280  polygon(8, #fff)
//         #rot left=510  polygon(6, #fff, -30deg) hexagon rotated so a VERTEX lands due-east
//       Row 2 -- input-validation matrix (seven 80x80 squares, centre=(left+40, 330)):
//         #empty left=50   polygon()              -> default sides=0 < 3  -> REJECTED (bg #303030)
//         #neg   left=150  polygon(-1, #fff)      -> sides=-1 < 3         -> REJECTED
//         #small left=250  polygon(2, #fff)       -> sides=2 < 3          -> REJECTED
//         #tri   left=350  polygon(3, #fff)       -> sides=3 (floor incl) -> ACCEPTED
//         #max   left=450  polygon(1024, #fff)    -> sides=1024 (ceil incl)-> ACCEPTED
//         #huge  left=550  polygon(100000, #fff)  -> sides>1024           -> REJECTED
//         #huge2 left=650  polygon(1e30, #fff)    -> sides>1024, non-int  -> REJECTED, NO UB
//
//     Proof 1 -- alpha shape (a polygon, not a filled rectangle): for #hex AND #oct the box
//       CENTRE is opaque white and a point 5px inside a box CORNER (~134px from centre, outside
//       any inscribed-r=100 polygon) is background. A rectangle decorator would fill the corner.
//     Proof 2 -- `sides` changes the shape: a point 95px due-east of centre is background for the
//       hexagon (edge midpoint / apothem ~86.6px < 95) but opaque for the octagon (a vertex sits
//       at east, radius 100 > 95). Only possible if sides=6 and sides=8 tessellate differently.
//     Proof 3 -- (negative) rotation is applied AND legitimate: #rot is polygon(6, .., -30deg);
//       -30deg rotates a hexagon vertex onto due-east, so #rot's east sample is opaque where
//       #hex's identical east sample is background. Proves the angle is used (not ignored) and
//       that a negative angle renders (the hardening did not kill legitimate rotation).
//     Proof 4 -- input hardening (fail-high, with a "tooth" -- reverting a guard flips a pixel):
//       every REJECTED box centre is NOT opaque white (an accepted decorator would have painted
//       it); #empty additionally shows its dark-grey background (rejected != nothing rendered);
//       every ACCEPTED box centre IS opaque white. The #huge2 (1e30) case running to completion
//       (test exits 0, and clean under UBSan when GLINTFX_SANITIZE=ON) is itself the proof that
//       the float->int conversion is guarded (no float-cast-overflow UB).
//
// PT: polygon_sanity -- prova que o decorator "polygon(<lados>, <cor>[, <rotacao>])" (v0.2.6,
//     decorator_polygon.{hpp,cpp}) renderiza um N-agono REGULAR (nao um retangulo preenchido),
//     que mudar `lados` muda a FORMA, que uma `rotacao` (negativa) e aplicada, E que a superficie
//     de hardening de entrada (review-fix v0.2.6) rejeita valores de `lados` invalidos / hostis
//     fail-high em vez de renderizar algo silenciosamente.
//
//     Cena (polygon_scene.rml/.rcss):
//       Fileira 1 -- provas de geometria (tres quadrados 200x200, centro=(left+100, top+100),
//       raio=100):
//         #hex left=50   polygon(6, #fff)         rotacao padrao, vertice pra cima
//         #oct left=280  polygon(8, #fff)
//         #rot left=510  polygon(6, #fff, -30deg) hexagono rotacionado ate um VERTICE cair no leste
//       Fileira 2 -- matriz de validacao de entrada (sete quadrados 80x80, centro=(left+40, 330)):
//         #empty left=50   polygon()              -> default lados=0 < 3  -> REJEITADO (bg #303030)
//         #neg   left=150  polygon(-1, #fff)      -> lados=-1 < 3         -> REJEITADO
//         #small left=250  polygon(2, #fff)       -> lados=2 < 3          -> REJEITADO
//         #tri   left=350  polygon(3, #fff)       -> lados=3 (piso incl)  -> ACEITO
//         #max   left=450  polygon(1024, #fff)    -> lados=1024 (teto incl)-> ACEITO
//         #huge  left=550  polygon(100000, #fff)  -> lados>1024           -> REJEITADO
//         #huge2 left=650  polygon(1e30, #fff)    -> lados>1024, nao-int  -> REJEITADO, SEM UB
//
//     Prova 1 -- alpha shape (um poligono, nao um retangulo preenchido): para #hex E #oct o
//       CENTRO da caixa e branco opaco e um ponto 5px dentro de um CANTO da caixa (~134px do
//       centro, fora de qualquer poligono de raio-inscrito=100) e background. Um decorator
//       retangulo preencheria o canto.
//     Prova 2 -- `lados` muda a forma: um ponto 95px a leste do centro e background para o
//       hexagono (ponto-medio de aresta / apotema ~86.6px < 95) mas opaco para o octogono (um
//       vertice fica no leste, raio 100 > 95). So possivel se lados=6 e lados=8 tesselam diferente.
//     Prova 3 -- rotacao (negativa) e aplicada E legitima: #rot e polygon(6, .., -30deg); -30deg
//       rotaciona um vertice do hexagono pro leste, entao a amostra leste de #rot e opaca onde a
//       amostra leste identica de #hex e background. Prova que o angulo e usado (nao ignorado) e
//       que um angulo negativo renderiza (o hardening nao matou a rotacao legitima).
//     Prova 4 -- hardening de entrada (fail-high, com "dente" -- reverter uma guarda inverte um
//       pixel): todo centro de caixa REJEITADA NAO e branco opaco (um decorator aceito o teria
//       pintado); #empty adicionalmente mostra o proprio background cinza-escuro (rejeitado !=
//       nada renderizado); todo centro de caixa ACEITA E branco opaco. O caso #huge2 (1e30)
//       rodar ate o fim (teste sai 0, e limpo sob UBSan com GLINTFX_SANITIZE=ON) e em si a prova
//       de que a conversao float->int e guardada (sem UB de float-cast-overflow).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                           // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
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
// EN: Sample the pixel at RCSS content-space coordinates (top-left origin, y-down). glReadPixels'
//     row 0 is the window BOTTOM (see viewport_origin_sanity.cpp), so the row is flipped; the
//     column (x) needs no flip.
// PT: Amostra o pixel em coordenadas de espaço-conteúdo do RCSS (origem topo-esquerda, y pra
//     baixo). A linha 0 do glReadPixels é a BASE da janela (ver viewport_origin_sanity.cpp),
//     então a linha é invertida; a coluna (x) não precisa de flip.
// ---------------------------------------------------------------------------
static const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int h,
                                      int content_x, int content_y) {
  const int row = h - 1 - content_y;
  const int col = content_x;
  return &px[(static_cast<size_t>(row) * w + col) * 3];
}

static bool is_opaque_white(const unsigned char* p) { return p[0] > 200 && p[1] > 200 && p[2] > 200; }
static bool is_background(const unsigned char* p)   { return p[0] < 50  && p[1] < 50  && p[2] < 50;  }
// EN: The dark-grey own-background of #empty (#303030 = 48). Distinct from both opaque white
//     (rejected decorator did NOT paint a hexagon) and pure black (proves the box IS there, its
//     background showing -- "rejected" is not "nothing at all").
// PT: O background próprio cinza-escuro de #empty (#303030 = 48). Distinto tanto de branco opaco
//     (decorator rejeitado NÃO pintou hexágono) quanto de preto puro (prova que a caixa ESTÁ lá,
//     seu background aparecendo -- "rejeitado" não é "nada").
static bool is_dark_grey(const unsigned char* p) {
  return p[0] > 25 && p[0] < 80 && p[1] > 25 && p[1] < 80 && p[2] > 25 && p[2] < 80;
}

int main() {
  constexpr int W = 760, H = 400;

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
  // EN: A rejected 'decorator: polygon(...)' logs a warning by design (fail-high). These are
  //     EXPECTED for #empty/#neg/#small/#huge/#huge2 -- they are not test failures.
  // PT: Um 'decorator: polygon(...)' rejeitado loga um aviso por design (fail-high). Estes são
  //     ESPERADOS para #empty/#neg/#small/#huge/#huge2 -- não são falhas de teste.
  ui.load("polygon_scene.rml");

  const auto px = capture(ui, W, H);

  // ---------------------------------------------------------------------------
  // EN: Row 1 -- geometry. #hex centre=(150,150), #oct centre=(380,150), #rot centre=(610,150).
  // PT: Fileira 1 -- geometria. #hex centro=(150,150), #oct centro=(380,150), #rot centro=(610,150).
  // ---------------------------------------------------------------------------
  const auto* hex_center = pixel_at(px, W, H, 150, 150);
  const auto* hex_corner = pixel_at(px, W, H, 55, 55);    // 5px inside the (50,50) corner.
  const auto* hex_east   = pixel_at(px, W, H, 245, 150);  // centre.x + 95.
  const auto* oct_center = pixel_at(px, W, H, 380, 150);
  const auto* oct_corner = pixel_at(px, W, H, 285, 55);   // 5px inside the (280,50) corner.
  const auto* oct_east   = pixel_at(px, W, H, 475, 150);  // centre.x + 95.
  const auto* rot_center = pixel_at(px, W, H, 610, 150);
  const auto* rot_east   = pixel_at(px, W, H, 705, 150);  // centre.x + 95.

  std::printf("polygon_sanity: hex c=(%d,%d,%d) corner=(%d,%d,%d) east=(%d,%d,%d)\n",
              hex_center[0],hex_center[1],hex_center[2], hex_corner[0],hex_corner[1],hex_corner[2],
              hex_east[0],hex_east[1],hex_east[2]);
  std::printf("polygon_sanity: oct c=(%d,%d,%d) corner=(%d,%d,%d) east=(%d,%d,%d)\n",
              oct_center[0],oct_center[1],oct_center[2], oct_corner[0],oct_corner[1],oct_corner[2],
              oct_east[0],oct_east[1],oct_east[2]);
  std::printf("polygon_sanity: rot c=(%d,%d,%d) east=(%d,%d,%d)\n",
              rot_center[0],rot_center[1],rot_center[2], rot_east[0],rot_east[1],rot_east[2]);

  // Proof 1 -- alpha shape.
  if (!is_opaque_white(hex_center)) { std::puts("polygon_sanity FAIL: #hex centre not opaque -- polygon did not render"); return 3; }
  if (!is_background(hex_corner))   { std::puts("polygon_sanity FAIL: #hex corner filled -- drew a rectangle, not a hexagon"); return 4; }
  if (!is_opaque_white(oct_center)) { std::puts("polygon_sanity FAIL: #oct centre not opaque -- polygon did not render"); return 5; }
  if (!is_background(oct_corner))   { std::puts("polygon_sanity FAIL: #oct corner filled -- drew a rectangle, not an octagon"); return 6; }

  // Proof 2 -- sides changes the shape.
  if (!is_background(hex_east))     { std::puts("polygon_sanity FAIL: #hex east (r=95) filled -- expected bg (hexagon apothem ~86.6px)"); return 7; }
  if (!is_opaque_white(oct_east))   { std::puts("polygon_sanity FAIL: #oct east (r=95) bg -- expected opaque (octagon vertex at east)"); return 8; }

  // Proof 3 -- (negative) rotation applied and legitimate.
  if (!is_opaque_white(rot_center)) { std::puts("polygon_sanity FAIL: #rot centre not opaque -- rotated polygon did not render"); return 9; }
  if (!is_opaque_white(rot_east))   { std::puts("polygon_sanity FAIL: #rot east (r=95) bg -- -30deg rotation not applied (vertex should be at east)"); return 10; }

  // ---------------------------------------------------------------------------
  // EN: Row 2 -- input-validation matrix. All box centres at content-y 330.
  // PT: Fileira 2 -- matriz de validação de entrada. Todos os centros em content-y 330.
  // ---------------------------------------------------------------------------
  const auto* empty_c = pixel_at(px, W, H, 90,  330);  // polygon()        -> rejected
  const auto* neg_c   = pixel_at(px, W, H, 190, 330);  // polygon(-1)      -> rejected
  const auto* small_c = pixel_at(px, W, H, 290, 330);  // polygon(2)       -> rejected
  const auto* tri_c   = pixel_at(px, W, H, 390, 330);  // polygon(3)       -> accepted
  const auto* max_c   = pixel_at(px, W, H, 490, 330);  // polygon(1024)    -> accepted
  const auto* huge_c  = pixel_at(px, W, H, 590, 330);  // polygon(100000)  -> rejected
  const auto* huge2_c = pixel_at(px, W, H, 690, 330);  // polygon(1e30)    -> rejected

  std::printf("polygon_sanity: empty=(%d,%d,%d) neg=(%d,%d,%d) small=(%d,%d,%d) tri=(%d,%d,%d) "
              "max=(%d,%d,%d) huge=(%d,%d,%d) huge2=(%d,%d,%d)\n",
              empty_c[0],empty_c[1],empty_c[2], neg_c[0],neg_c[1],neg_c[2], small_c[0],small_c[1],small_c[2],
              tri_c[0],tri_c[1],tri_c[2], max_c[0],max_c[1],max_c[2], huge_c[0],huge_c[1],huge_c[2],
              huge2_c[0],huge2_c[1],huge2_c[2]);

  // Proof 4a -- empty polygon() REJECTED: centre must NOT be opaque white (tooth: reverting the
  // sides default "0"->"6" makes this an opaque hexagon and this assert fires), and its own
  // dark-grey background must show through (rejected, not painted over).
  if (is_opaque_white(empty_c)) { std::puts("polygon_sanity FAIL: polygon() empty rendered a white polygon -- must be REJECTED (sides default < 3)"); return 11; }
  if (!is_dark_grey(empty_c))   { std::puts("polygon_sanity FAIL: polygon() empty -- expected the box's dark-grey background to show"); return 12; }

  // Proof 4b -- negative / below-floor sides REJECTED (centre stays black body background).
  if (!is_background(neg_c))   { std::puts("polygon_sanity FAIL: polygon(-1) not rejected -- centre should be background"); return 13; }
  if (!is_background(small_c)) { std::puts("polygon_sanity FAIL: polygon(2) not rejected -- centre should be background"); return 14; }

  // Proof 4c -- floor (3) and ceiling (1024) inclusive: ACCEPTED (centre opaque white).
  if (!is_opaque_white(tri_c)) { std::puts("polygon_sanity FAIL: polygon(3) not accepted -- triangle centre should be opaque (floor is inclusive at 3)"); return 15; }
  if (!is_opaque_white(max_c)) { std::puts("polygon_sanity FAIL: polygon(1024) not accepted -- centre should be opaque (ceiling is inclusive at 1024)"); return 16; }

  // Proof 4d -- above-ceiling / astronomically-large sides REJECTED, no UB (huge2=1e30 exercises
  // the float->int overflow guard; reaching here at all means no float-cast-overflow occurred).
  if (!is_background(huge_c))  { std::puts("polygon_sanity FAIL: polygon(100000) not rejected -- centre should be background (ceiling 1024)"); return 17; }
  if (!is_background(huge2_c)) { std::puts("polygon_sanity FAIL: polygon(1e30) not rejected -- centre should be background (and must not UB)"); return 18; }

  std::puts("polygon_sanity: PASS");
  return 0;
}
