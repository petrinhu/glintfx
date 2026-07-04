// SPDX-License-Identifier: MPL-2.0
// EN: polygon_gradient_sanity -- proves the "polygon(<sides>, radial-gradient(...) /
//     linear-gradient(...))" fill extension (v0.3.1, decorator_polygon.{hpp,cpp}, consumer-driven
//     by GusWorld's brass hexagon corner nodes: "muito plano", needed centre-clear ->
//     edge-dark screw-head volume) actually ramps colour per-fragment, respects explicit stop
//     percentages, respects the linear angle, leaves the pre-existing SOLID fill and `sides`
//     hardening completely unaffected, and rejects malformed fill syntax fail-high (no crash, no
//     partial/garbage render). Companion to polygon_sanity.cpp (which covers geometry + `sides`
//     hardening only, pre-v0.3.1, still passing unmodified).
//
//     Scene (polygon_gradient_scene.rml/.rcss):
//       Row 1 -- gradient-fill proofs (four 200x200 squares, centre=(left+100,150), radius=100):
//         #radial      left=50   radial-gradient(#fff, #000)              centre bright, edge dark
//         #radial_stop left=280  radial-gradient(#fff, #f00 80%, #000 100%) explicit-% stop proof
//         #linear      left=510  linear-gradient(90deg, #fff, #000)       west bright, east dark
//         #solid       left=740  polygon(6, #C9A24B)                      UNCHANGED flat brass
//       Row 2 -- fill-hardening matrix (four 80x80 squares, centre=(left+40,340)):
//         #bad_radial  left=50   radial-gradient()            empty stop list      -> REJECTED
//         #bad_linear  left=150  linear-gradient(#fff,#000)   missing angle        -> REJECTED
//         #bad_fill    left=250  not-a-color                  neither colour/grad  -> REJECTED
//         #bad_sides   left=350  polygon(2, radial-gradient(#fff,#000))  sides<3   -> REJECTED
//
//     Proof 1 (radial ramp) -- #radial centre is near-opaque-white (t=0) and a point 80px due EAST
//       of centre (still inside the hexagon: apothem ~86.6px > 80px, see polygon_sanity.cpp's own
//       east-sample geometry proof) is near-black (t=0.8). A "dente": swapping the two stop colours
//       in decorator_polygon.cpp (or feeding the shader p/v backwards) flips which sample is bright.
//     Proof 2 (explicit stop percentage) -- #radial_stop's middle stop sits at 80%; the fragment
//       shader's smoothstep blending makes the colour AT EXACTLY a stop's own position equal to
//       that stop's own colour (see shader_frag_gradient's mix_stop_colors -- smoothstep(prev,cur,
//       cur)=1). So the r=80 east sample must read strongly RED (high R, low G/B), while r=40 east
//       (well short of the stop) must still read white-ish. A "dente": if ResolveStopPositions
//       ignored the "80%" text and treated the sole interior stop as "auto" (evenly spaced at
//       50%), the r=80 sample would already be deep into the red->black blend, not pure red --
//       this assertion would fail.
//     Proof 3 (linear angle) -- #linear is linear-gradient(90deg, #fff, #000); CSS angle convention
//       (0deg=up, clockwise) makes 90deg point due EAST, so the gradient runs left(white)->
//       right(black). A point 40px WEST of centre must read brighter than a point 40px EAST. A
//       "dente": swapping p0/p1 (or mis-deriving the angle-to-direction formula) flips this.
//     Proof 4 (solid fill untouched) -- #solid (polygon(6, #C9A24B), the exact GusWorld brass
//       colour) must show the IDENTICAL colour at both its centre and an east-r=40 sample --
//       proof this feature did not accidentally turn a plain colour into a two-point gradient
//       (e.g. by misclassifying "#C9A24B" or by always taking the gradient code path).
//     Proof 5 (fill hardening, fail-high) -- all four malformed-fill/sides boxes must show their
//       OWN dark-grey background at centre (rejected != a filled hexagon, rejected != a crash);
//       #bad_sides in particular proves the pre-existing `sides<3` guard still runs and still wins
//       even when the fill argument is itself a well-formed gradient.
//
// PT: polygon_gradient_sanity -- prova que a extensão de preenchimento
//     "polygon(<lados>, radial-gradient(...) / linear-gradient(...))" (v0.3.1,
//     decorator_polygon.{hpp,cpp}, consumer-driven pelos nós hexagonais de latão dos cantos do
//     GusWorld: "muito plano", precisava do volume de cabeça-de-parafuso centro-claro ->
//     borda-escura) de fato gradua a cor por fragmento, respeita porcentagens de stop explícitas,
//     respeita o ângulo do linear, deixa o preenchimento SOLID pré-existente e o hardening de
//     `sides` completamente intocados, e rejeita sintaxe de preenchimento malformada fail-high
//     (sem crash, sem render parcial/lixo). Companheiro de polygon_sanity.cpp (que cobre só
//     geometria + hardening de `sides`, pré-v0.3.1, continua passando sem modificação).
//
//     Cena: ver polygon_gradient_scene.rml/.rcss (comentário espelhado acima em EN).
//
//     Prova 1 (rampa radial), Prova 2 (porcentagem de stop explícita), Prova 3 (ângulo linear),
//     Prova 4 (preenchimento sólido intocado), Prova 5 (hardening de preenchimento, fail-high) --
//     ver os parágrafos EN acima para o detalhe completo de cada uma.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h (gl3w already loaded by UiLayer ctor).
                           // PT: inclui GL/gl3w.h (gl3w já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) -- same methodology as polygon_sanity.cpp.
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK) -- mesma metodologia de
//     polygon_sanity.cpp.
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

// EN: Sample the pixel at RCSS content-space coordinates (top-left origin, y-down) -- glReadPixels'
//     row 0 is the window BOTTOM, so the row is flipped; the column (x) needs no flip. Identical to
//     polygon_sanity.cpp's helper.
// PT: Amostra o pixel em coordenadas de espaço-conteúdo do RCSS (origem topo-esquerda, y pra
//     baixo) -- a linha 0 do glReadPixels é a BASE da janela, então a linha é invertida; a coluna
//     (x) não precisa de flip. Idêntico ao helper de polygon_sanity.cpp.
static const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int h,
                                      int content_x, int content_y) {
  const int row = h - 1 - content_y;
  const int col = content_x;
  return &px[(static_cast<size_t>(row) * w + col) * 3];
}

static bool is_dark_grey(const unsigned char* p) {
  return p[0] > 25 && p[0] < 80 && p[1] > 25 && p[1] < 80 && p[2] > 25 && p[2] < 80;
}

int main() {
  constexpr int W = 1000, H = 420;

  glintfx::WindowGlfw host;
  if (!host.create("polygon_gradient_sanity_host", W, H)) {
    std::puts("polygon_gradient_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("polygon_gradient_sanity FAIL: ui attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  // EN: Every #bad_* box logs a "decorator ignored" warning by design (fail-high). Expected, not a
  //     test failure.
  // PT: Toda caixa #bad_* loga um aviso de "decorator ignorado" por design (fail-high). Esperado,
  //     não é falha de teste.
  ui.load("polygon_gradient_scene.rml");

  const auto px = capture(ui, W, H);

  // ---------------------------------------------------------------------------
  // Proof 1 -- radial ramp. #radial centre=(150,150), radius=100.
  // ---------------------------------------------------------------------------
  const auto* radial_center = pixel_at(px, W, H, 150, 150);
  const auto* radial_east80 = pixel_at(px, W, H, 230, 150); // centre.x + 80 (t=0.8, inside apothem).

  std::printf("polygon_gradient_sanity: radial centre=(%d,%d,%d) east80=(%d,%d,%d)\n",
              radial_center[0], radial_center[1], radial_center[2],
              radial_east80[0], radial_east80[1], radial_east80[2]);

  if (!(radial_center[0] > 200 && radial_center[1] > 200 && radial_center[2] > 200)) {
    std::puts("polygon_gradient_sanity FAIL: #radial centre not near-white (t=0 should be ~opaque white)");
    return 3;
  }
  if (!(radial_east80[0] < 80 && radial_east80[1] < 80 && radial_east80[2] < 80)) {
    std::puts("polygon_gradient_sanity FAIL: #radial east80 not near-black (t=0.8 should be dark) -- "
               "stops reversed or shader centre/radius wrong?");
    return 4;
  }

  // ---------------------------------------------------------------------------
  // Proof 2 -- explicit stop percentage. #radial_stop centre=(380,150).
  // ---------------------------------------------------------------------------
  const auto* stop_east40 = pixel_at(px, W, H, 420, 150); // centre.x + 40 (t=0.4, well short of the 80% stop).
  const auto* stop_east80 = pixel_at(px, W, H, 460, 150); // centre.x + 80 (t=0.8, exactly the stop).

  std::printf("polygon_gradient_sanity: radial_stop east40=(%d,%d,%d) east80=(%d,%d,%d)\n",
              stop_east40[0], stop_east40[1], stop_east40[2],
              stop_east80[0], stop_east80[1], stop_east80[2]);

  if (!(stop_east40[0] > 150 && stop_east40[1] > 100 && stop_east40[2] > 100)) {
    std::puts("polygon_gradient_sanity FAIL: #radial_stop east40 (t=0.4) should still read white-ish, not red/black yet");
    return 5;
  }
  if (!(stop_east80[0] > 150 && stop_east80[1] < 100 && stop_east80[2] < 100)) {
    std::puts("polygon_gradient_sanity FAIL: #radial_stop east80 (t=0.8, exactly the 80% stop) should read "
               "strongly red -- the '80%' stop position was likely ignored/misresolved");
    return 6;
  }

  // ---------------------------------------------------------------------------
  // Proof 3 -- linear angle. #linear centre=(610,150).
  // ---------------------------------------------------------------------------
  const auto* linear_west40 = pixel_at(px, W, H, 570, 150); // centre.x - 40.
  const auto* linear_east40 = pixel_at(px, W, H, 650, 150); // centre.x + 40.

  std::printf("polygon_gradient_sanity: linear west40=(%d,%d,%d) east40=(%d,%d,%d)\n",
              linear_west40[0], linear_west40[1], linear_west40[2],
              linear_east40[0], linear_east40[1], linear_east40[2]);

  if (!(linear_west40[0] > linear_east40[0] + 50)) {
    std::puts("polygon_gradient_sanity FAIL: #linear west40 should read clearly brighter than east40 "
               "(90deg must point EAST, white->black) -- angle-to-direction mapping wrong?");
    return 7;
  }

  // ---------------------------------------------------------------------------
  // Proof 4 -- solid fill untouched. #solid centre=(840,150).
  // ---------------------------------------------------------------------------
  const auto* solid_center = pixel_at(px, W, H, 840, 150);
  const auto* solid_east40 = pixel_at(px, W, H, 880, 150);

  std::printf("polygon_gradient_sanity: solid centre=(%d,%d,%d) east40=(%d,%d,%d)\n",
              solid_center[0], solid_center[1], solid_center[2],
              solid_east40[0], solid_east40[1], solid_east40[2]);

  // #C9A24B = (201, 162, 75).
  if (!(solid_center[0] > 180 && solid_center[0] < 220 && solid_center[1] > 140 && solid_center[1] < 180 &&
        solid_center[2] > 55 && solid_center[2] < 95)) {
    std::puts("polygon_gradient_sanity FAIL: #solid centre is not the expected brass colour #C9A24B");
    return 8;
  }
  const int dr = solid_center[0] - solid_east40[0], dg = solid_center[1] - solid_east40[1], db = solid_center[2] - solid_east40[2];
  if (!((dr > -6 && dr < 6) && (dg > -6 && dg < 6) && (db > -6 && db < 6))) {
    std::puts("polygon_gradient_sanity FAIL: #solid centre and east40 differ -- solid fill accidentally "
               "became a gradient");
    return 9;
  }

  // ---------------------------------------------------------------------------
  // Proof 5 -- fill hardening (fail-high). Row-2 box centres all at content-y 340.
  // ---------------------------------------------------------------------------
  const auto* bad_radial_c = pixel_at(px, W, H, 90,  340); // radial-gradient() -- empty.
  const auto* bad_linear_c = pixel_at(px, W, H, 190, 340); // linear-gradient(#fff,#000) -- no angle.
  const auto* bad_fill_c   = pixel_at(px, W, H, 290, 340); // not-a-color.
  const auto* bad_sides_c  = pixel_at(px, W, H, 390, 340); // sides=2 with a valid gradient fill.

  std::printf("polygon_gradient_sanity: bad_radial=(%d,%d,%d) bad_linear=(%d,%d,%d) bad_fill=(%d,%d,%d) "
              "bad_sides=(%d,%d,%d)\n",
              bad_radial_c[0], bad_radial_c[1], bad_radial_c[2], bad_linear_c[0], bad_linear_c[1], bad_linear_c[2],
              bad_fill_c[0], bad_fill_c[1], bad_fill_c[2], bad_sides_c[0], bad_sides_c[1], bad_sides_c[2]);

  if (!is_dark_grey(bad_radial_c)) { std::puts("polygon_gradient_sanity FAIL: radial-gradient() (empty) not rejected"); return 10; }
  if (!is_dark_grey(bad_linear_c)) { std::puts("polygon_gradient_sanity FAIL: linear-gradient() without angle not rejected"); return 11; }
  if (!is_dark_grey(bad_fill_c))   { std::puts("polygon_gradient_sanity FAIL: 'not-a-color' fill not rejected"); return 12; }
  if (!is_dark_grey(bad_sides_c))  { std::puts("polygon_gradient_sanity FAIL: sides=2 with a valid gradient fill not rejected"); return 13; }

  std::puts("polygon_gradient_sanity: PASS");
  return 0;
}
