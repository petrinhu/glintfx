// SPDX-License-Identifier: MPL-2.0
// EN: transform2d_sanity (D2D-2A) -- headless unit test for the PURE projection/transform math
//     in src/transform2d.hpp (plan docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md,
//     decisions D12/D14/D16/D17/D18/D19/D20). No GL, no Xvfb, no window -- runs in BOTH build
//     configs, same class of test as sprite_batch_sanity.cpp/image_decode_sanity.cpp. This is
//     the delta-value test this house's own postmortem demands for a conversion formula (plan
//     section 0, risk 2 -- "a conversion formula without a computed-value delta test" already
//     cost a Critical once): every D12/D18 expected value below is DERIVED ON PAPER from the
//     literal formulas in transform2d.hpp's own header comment, not just "does it not crash".
//     Covers: D12 hand-computed screen points (offset-only, zoom-only 2x/0.5x, rotation-only
//     pi/2 exact, combined), the world_to_screen<->screen_to_world round-trip over a grid of
//     points x cameras, rotation wrap (t vs t+2*pi), D16 totality (zoom 0/negative, NaN/Inf
//     camera members, NaN/Inf point, non-positive viewport -- input returned UNCHANGED for
//     both functions), D17 camera_from_world_rect (valid fit + hostile corpus), D18 corner
//     math (pi/2 rotation permutes corners exactly, pivot (0,0) vs (1,1), negative scale
//     mirrors, and -- PROG-1, closing an Onda 4 adversarial-review gap -- anisotropic scale
//     (scale_x != scale_y) COMBINED with a nonzero rotation, the only case where "scale then
//     rotate" is observably different from "rotate then scale"; every other D18 case above has
//     either uniform scale or zero rotation, under which the two orders commute and a swapped
//     apply() lambda passes unnoticed), D19 exact equivalence (identity SpriteTransform
//     reproduces dst's own 4 corners bit-for-bit -- exact float equality is legitimate here,
//     see transform2d.hpp's own rationale), and D20 overflow (a finite scale times a finite dst
//     that multiplies past float's range produces a non-finite corner, caught by
//     is_finite(SpriteCorners)).
// PT: transform2d_sanity (D2D-2A) -- teste unitário headless para a matemática PURA de
//     projeção/transform em src/transform2d.hpp (plano docs/superpowers/plans/2026-07-23-
//     onda4-draw2d-camera.md, decisões D12/D14/D16/D17/D18/D19/D20). Sem GL, sem Xvfb, sem
//     janela -- roda em AMBAS as configs de build, mesma classe de teste de
//     sprite_batch_sanity.cpp/image_decode_sanity.cpp. Este é o teste de valor delta que o
//     próprio postmortem desta casa exige pra fórmula de conversão (plano seção 0, risco 2 --
//     "fórmula de conversão sem teste de valor computado" já custou um Critical uma vez): todo
//     valor esperado D12/D18 abaixo é DERIVADO NO PAPEL a partir das fórmulas literais do
//     próprio comentário de cabeçalho de transform2d.hpp, não só "não crashou". Cobre: pontos
//     de tela computados à mão do D12 (offset só, zoom só 2x/0.5x, rotação só pi/2 exato,
//     combinado), o round-trip world_to_screen<->screen_to_world numa grade de pontos x
//     câmeras, wrap de rotação (t vs t+2*pi), totalidade D16 (zoom 0/negativo, membro de
//     câmera NaN/Inf, ponto NaN/Inf, viewport não-positivo -- input devolvido INALTERADO nas
//     duas funções), D17 camera_from_world_rect (fit válido + corpus hostil), matemática de
//     canto D18 (rotação pi/2 permuta cantos exatamente, pivô (0,0) vs (1,1), escala negativa
//     espelha, e -- PROG-1, fechando uma lacuna do review adversarial da Onda 4 -- escala
//     anisotrópica (scale_x != scale_y) COMBINADA com rotação não-zero, o único caso em que
//     "escala depois rotação" é observavelmente diferente de "rotação depois escala"; todo
//     outro caso do D18 acima tem escala uniforme ou rotação zero, condição sob a qual as duas
//     ordens comutam e uma lambda apply() com as linhas trocadas passa despercebida),
//     equivalência exata D19 (SpriteTransform identidade reproduz os 4 cantos do
//     próprio dst bit-a-bit -- igualdade de float exata é legítima aqui, ver a racional do
//     próprio transform2d.hpp), e overflow D20 (uma escala finita vezes um dst finito que
//     multiplica além da faixa do float produz um canto não-finito, capturado por
//     is_finite(SpriteCorners)).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/transform2d.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

using glintfx::Camera2d;
using glintfx::RectF;
using glintfx::SpriteTransform;
using glintfx::Vec2F;
using glintfx::draw2d_detail::compute_sprite_corners;
using glintfx::draw2d_detail::is_finite;
using glintfx::draw2d_detail::SpriteCorners;

namespace {

// EN: world_to_screen/screen_to_world/camera_from_world_rect are declared with the SAME names
//     in the PUBLIC glintfx:: namespace (draw2d.hpp, D21 -- defined out-of-line in draw2d.cpp,
//     not yet linked into this test binary). A bare `using draw2d_detail::world_to_screen;`
//     plus an unqualified call is genuinely ambiguous: argument-dependent lookup on
//     Camera2d/Vec2F (both `glintfx::`) ALSO finds the public declarations, and both overloads
//     match identically -- not a test-only quirk, a real consequence of D21's single-source
//     design (the real caller, draw2d.cpp, sidesteps it the same way: fully qualifying
//     `draw2d_detail::world_to_screen(...)` inside its own out-of-line definition of
//     `glintfx::world_to_screen`, which is qualified lookup, no ADL). These thin wrappers do
//     the same -- fully qualified calls -- so every test body below can stay unqualified.
// PT: world_to_screen/screen_to_world/camera_from_world_rect são declaradas com os MESMOS
//     nomes no namespace PÚBLICO glintfx:: (draw2d.hpp, D21 -- definidas fora de linha em
//     draw2d.cpp, ainda não linkadas neste binário de teste). Um `using
//     draw2d_detail::world_to_screen;` puro mais uma chamada não-qualificada é genuinamente
//     ambíguo: a busca dependente de argumento (ADL) em Camera2d/Vec2F (ambos `glintfx::`)
//     TAMBÉM acha as declarações públicas, e os dois overloads batem identicamente -- não é
//     capricho só de teste, é consequência real do desenho de fonte-única do D21 (o chamador
//     de verdade, draw2d.cpp, contorna do mesmo jeito: qualificando totalmente
//     `draw2d_detail::world_to_screen(...)` dentro da própria definição fora de linha de
//     `glintfx::world_to_screen`, que é busca qualificada, sem ADL). Estes wrappers finos
//     fazem o mesmo -- chamada totalmente qualificada -- pra que todo corpo de teste abaixo
//     possa ficar não-qualificado.
Vec2F w2s(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F world) {
  return glintfx::draw2d_detail::world_to_screen(cam, viewport_w, viewport_h, world);
}
Vec2F s2w(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F screen) {
  return glintfx::draw2d_detail::screen_to_world(cam, viewport_w, viewport_h, screen);
}
Camera2d fit_cam(const RectF& world_rect, int viewport_w, int viewport_h) {
  return glintfx::draw2d_detail::camera_from_world_rect(world_rect, viewport_w, viewport_h);
}

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

constexpr float kPi = 3.14159265358979323846f;
constexpr float kHalfPi = kPi * 0.5f;

bool near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }
bool near(Vec2F a, Vec2F b, float eps) { return near(a.x, b.x, eps) && near(a.y, b.y, eps); }

// EN: D16's totality contract returns the INPUT value UNCHANGED, including NaN members --
//     plain `==` cannot verify that (NaN != NaN by IEEE 754), so this helper treats NaN==NaN
//     as a match (the exact "did nothing to it" property D16 promises), while every other
//     value (including Inf, which IS `==`-comparable) still needs bit-for-bit equality.
// PT: O contrato de totalidade do D16 devolve o valor de ENTRADA INALTERADO, inclusive
//     membros NaN -- `==` puro não consegue verificar isso (NaN != NaN pelo IEEE 754), então
//     este helper trata NaN==NaN como igual (a propriedade exata "não fez nada com isso" que
//     o D16 promete), enquanto todo outro valor (inclusive Inf, que É comparável por `==`)
//     ainda precisa de igualdade bit-a-bit.
bool same_or_nan(float a, float b) {
  if (std::isnan(a) && std::isnan(b)) return true;
  return a == b;
}
bool same_point(Vec2F a, Vec2F b) { return same_or_nan(a.x, b.x) && same_or_nan(a.y, b.y); }

// ---------------------------------------------------------------------------
// D12 -- hand-computed screen points / pontos de tela computados à mão
// ---------------------------------------------------------------------------

void test_d12_offset_only() {
  // cam={10,20,zoom=1,rot=0}, viewport 200x100 (center=(100,50)), p=(15,25).
  // centered=(15-10,25-20)*1=(5,5); rotate(., -0)=(5,5); screen=center+that=(105,55).
  const Camera2d cam{10.f, 20.f, 1.f, 0.f};
  const Vec2F got = w2s(cam, 200, 100, Vec2F{15.f, 25.f});
  check(near(got, Vec2F{105.f, 55.f}, 1e-3f), "d12_offset_only: hand-computed screen point");
}

void test_d12_zoom_only() {
  // cam={0,0,zoom=2,rot=0}, viewport 100x100 (center=(50,50)), p=(10,0).
  // centered=(10*2,0*2)=(20,0); screen=(50+20,50+0)=(70,50).
  const Camera2d cam2x{0.f, 0.f, 2.f, 0.f};
  const Vec2F got2x = w2s(cam2x, 100, 100, Vec2F{10.f, 0.f});
  check(near(got2x, Vec2F{70.f, 50.f}, 1e-3f), "d12_zoom_only: 2x zoom hand-computed point");

  // Same p, zoom=0.5: centered=(10*0.5,0)=(5,0); screen=(55,50).
  const Camera2d cam05{0.f, 0.f, 0.5f, 0.f};
  const Vec2F got05 = w2s(cam05, 100, 100, Vec2F{10.f, 0.f});
  check(near(got05, Vec2F{55.f, 50.f}, 1e-3f), "d12_zoom_only: 0.5x zoom hand-computed point");
}

void test_d12_rotation_only() {
  // cam={0,0,zoom=1,rot=pi/2}, viewport 100x100 (center=(50,50)), p=(10,0).
  // centered=(10,0); rot(v,-pi/2): cos=0,sin=-1 -> x'=10*0-0*(-1)=0, y'=10*(-1)+0*0=-10.
  // screen=(50+0,50-10)=(50,40).
  const Camera2d cam{0.f, 0.f, 1.f, kHalfPi};
  const Vec2F got = w2s(cam, 100, 100, Vec2F{10.f, 0.f});
  check(near(got, Vec2F{50.f, 40.f}, 1e-2f), "d12_rotation_only: pi/2 hand-computed point");

  // Inverse leg (pins the +cam.rotation sign in screen_to_world independently of the
  // round-trip test -- if both signs flipped together the round-trip alone would not catch
  // it, this hand-computed exact-inverse leg does): screen=(50,40) -> world=(10,0).
  const Vec2F back = s2w(cam, 100, 100, Vec2F{50.f, 40.f});
  check(near(back, Vec2F{10.f, 0.f}, 1e-2f), "d12_rotation_only: screen_to_world exact inverse");
}

void test_d12_combined() {
  // cam={5,5,zoom=2,rot=pi/2}, viewport 100x100 (center=(50,50)), p=(15,5).
  // centered=((15-5)*2,(5-5)*2)=(20,0); rot(.,-pi/2)=(0,-20); screen=(50,30).
  const Camera2d cam{5.f, 5.f, 2.f, kHalfPi};
  const Vec2F got = w2s(cam, 100, 100, Vec2F{15.f, 5.f});
  check(near(got, Vec2F{50.f, 30.f}, 1e-2f), "d12_combined: offset+zoom+rotation hand-computed");
}

// ---------------------------------------------------------------------------
// Round-trip property + rotation wrap / propriedade de round-trip + wrap de rotação
// ---------------------------------------------------------------------------

void test_round_trip_grid() {
  const Vec2F points[] = {{0.f, 0.f}, {50.f, -30.f}, {-75.f, 120.f}, {300.f, -200.f}};
  const Camera2d cameras[] = {
      {0.f, 0.f, 1.f, 0.f},
      {10.f, -5.f, 2.f, 0.5f},
      {-40.f, 60.f, 0.25f, kPi / 3.f},
      {0.f, 0.f, 10.f, kPi},
      {100.f, 100.f, 0.1f, 7.f}, // rotation beyond 2*pi (7 rad).
  };
  for (const Camera2d& cam : cameras) {
    for (const Vec2F& p : points) {
      const Vec2F s = w2s(cam, 640, 480, p);
      const Vec2F back = s2w(cam, 640, 480, s);
      check(near(back, p, 5e-2f), "round_trip_grid: s2w(w2s(p)) == p");
    }
  }
}

void test_rotation_wrap() {
  const Camera2d cam_t{20.f, -10.f, 1.5f, 0.7f};
  const Camera2d cam_t2pi{20.f, -10.f, 1.5f, 0.7f + 2.f * kPi};
  const Vec2F p{33.f, 17.f};
  const Vec2F s1 = w2s(cam_t, 300, 200, p);
  const Vec2F s2 = w2s(cam_t2pi, 300, 200, p);
  check(near(s1, s2, 1e-2f), "rotation_wrap: rotation t and t+2*pi produce the same mapping");

  const Vec2F w1 = s2w(cam_t, 300, 200, Vec2F{120.f, 80.f});
  const Vec2F w2 = s2w(cam_t2pi, 300, 200, Vec2F{120.f, 80.f});
  check(near(w1, w2, 1e-2f), "rotation_wrap: screen_to_world agrees across the 2*pi wrap too");
}

// ---------------------------------------------------------------------------
// D16 -- pure helper totality / totalidade dos helpers puros
// ---------------------------------------------------------------------------

void test_d16_totality_wts() {
  const Camera2d good{1.f, 2.f, 1.f, 0.f};
  const Vec2F p{3.f, 4.f};
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();

  check(same_point(w2s(Camera2d{0.f, 0.f, 0.f, 0.f}, 100, 100, p), p),
        "d16_w2s: zoom==0 returns input point unchanged");
  check(same_point(w2s(Camera2d{0.f, 0.f, -1.f, 0.f}, 100, 100, p), p),
        "d16_w2s: negative zoom returns input point unchanged");
  check(same_point(w2s(Camera2d{nan, 0.f, 1.f, 0.f}, 100, 100, p), p),
        "d16_w2s: NaN cam.x returns input point unchanged");
  check(same_point(w2s(Camera2d{0.f, 0.f, 1.f, inf}, 100, 100, p), p),
        "d16_w2s: Inf cam.rotation returns input point unchanged");
  check(same_point(w2s(good, 100, 100, Vec2F{nan, 4.f}), Vec2F{nan, 4.f}),
        "d16_w2s: NaN point.x returns input point unchanged (NaN preserved, not laundered)");
  check(same_point(w2s(good, 100, 100, Vec2F{3.f, inf}), Vec2F{3.f, inf}),
        "d16_w2s: Inf point.y returns input point unchanged");
  check(same_point(w2s(good, 0, 100, p), p),
        "d16_w2s: viewport_w<=0 returns input point unchanged");
  check(same_point(w2s(good, 100, -5, p), p),
        "d16_w2s: viewport_h<=0 returns input point unchanged");
}

void test_d16_totality_stw() {
  const Camera2d good{1.f, 2.f, 1.f, 0.f};
  const Vec2F s{3.f, 4.f};
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();

  check(same_point(s2w(Camera2d{0.f, 0.f, 0.f, 0.f}, 100, 100, s), s),
        "d16_s2w: zoom==0 returns input point unchanged");
  check(same_point(s2w(Camera2d{0.f, 0.f, -2.f, 0.f}, 100, 100, s), s),
        "d16_s2w: negative zoom returns input point unchanged");
  check(same_point(s2w(Camera2d{0.f, 0.f, 1.f, nan}, 100, 100, s), s),
        "d16_s2w: NaN cam.rotation returns input point unchanged");
  check(same_point(s2w(Camera2d{inf, 0.f, 1.f, 0.f}, 100, 100, s), s),
        "d16_s2w: Inf cam.x returns input point unchanged");
  check(same_point(s2w(good, 100, 100, Vec2F{nan, 4.f}), Vec2F{nan, 4.f}),
        "d16_s2w: NaN point.x returns input point unchanged");
  check(same_point(s2w(good, -1, 100, s), s),
        "d16_s2w: viewport_w<=0 returns input point unchanged");
  check(same_point(s2w(good, 100, 0, s), s),
        "d16_s2w: viewport_h<=0 returns input point unchanged");
}

// ---------------------------------------------------------------------------
// D17 -- camera_from_world_rect
// ---------------------------------------------------------------------------

void test_d17_valid_fit() {
  // world_rect={0,0,100,50}, viewport 200x100 -> zoom=200/100=2, center=(50,25).
  const Camera2d cam = fit_cam(RectF{0.f, 0.f, 100.f, 50.f}, 200, 100);
  check(near(cam.zoom, 2.f, 1e-4f), "d17_valid_fit: zoom = viewport_w / world_rect.w");
  check(near(cam.x, 50.f, 1e-4f) && near(cam.y, 25.f, 1e-4f), "d17_valid_fit: center = rect center");
  check(near(cam.rotation, 0.f, 1e-4f), "d17_valid_fit: rotation defaults to 0");
}

void test_d17_hostile() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const RectF ok{0.f, 0.f, 10.f, 10.f};
  const auto is_default = [](const Camera2d& c) {
    return c.x == 0.f && c.y == 0.f && c.zoom == 1.f && c.rotation == 0.f;
  };

  check(is_default(fit_cam(RectF{0.f, 0.f, 0.f, 10.f}, 100, 100)),
        "d17_hostile: world_rect.w<=0 returns Camera2d{}");
  check(is_default(fit_cam(RectF{0.f, 0.f, 10.f, -5.f}, 100, 100)),
        "d17_hostile: world_rect.h<=0 returns Camera2d{}");
  check(is_default(fit_cam(RectF{nan, 0.f, 10.f, 10.f}, 100, 100)),
        "d17_hostile: non-finite world_rect member returns Camera2d{}");
  check(is_default(fit_cam(ok, 0, 100)),
        "d17_hostile: viewport_w<=0 returns Camera2d{}");
  check(is_default(fit_cam(ok, 100, -1)),
        "d17_hostile: viewport_h<=0 returns Camera2d{}");
}

// ---------------------------------------------------------------------------
// D18 -- sprite corner math / matemática de canto do sprite
// ---------------------------------------------------------------------------

void test_d18_rotation_permutes_corners() {
  // dst={0,0,10,10} (TL=(0,0),TR=(10,0),BR=(10,10),BL=(0,10)), pivot=center=(5,5), rot=pi/2.
  // Hand-derived: new_tl=old_tr, new_tr=old_br, new_br=old_bl, new_bl=old_tl (cyclic shift).
  const RectF dst{0.f, 0.f, 10.f, 10.f};
  SpriteTransform t{};
  t.rotation = kHalfPi;
  const SpriteCorners c = compute_sprite_corners(dst, t);
  check(near(c.tl, Vec2F{10.f, 0.f}, 1e-2f), "d18_rotation: new TL lands at old TR");
  check(near(c.tr, Vec2F{10.f, 10.f}, 1e-2f), "d18_rotation: new TR lands at old BR");
  check(near(c.br, Vec2F{0.f, 10.f}, 1e-2f), "d18_rotation: new BR lands at old BL");
  check(near(c.bl, Vec2F{0.f, 0.f}, 1e-2f), "d18_rotation: new BL lands at old TL");
}

void test_d18_pivot_variants() {
  const RectF dst{0.f, 0.f, 10.f, 10.f};

  // Pivot at (0,0) (origin 0/0 == TL corner itself): TL stays put, others rotate about it.
  SpriteTransform t_tl{};
  t_tl.origin_x = 0.f;
  t_tl.origin_y = 0.f;
  t_tl.rotation = kHalfPi;
  const SpriteCorners c_tl = compute_sprite_corners(dst, t_tl);
  check(near(c_tl.tl, Vec2F{0.f, 0.f}, 1e-2f), "d18_pivot_00: pivot corner (TL) stays put");
  check(near(c_tl.tr, Vec2F{0.f, 10.f}, 1e-2f), "d18_pivot_00: TR hand-computed about pivot (0,0)");
  check(near(c_tl.br, Vec2F{-10.f, 10.f}, 1e-2f), "d18_pivot_00: BR hand-computed about pivot (0,0)");
  check(near(c_tl.bl, Vec2F{-10.f, 0.f}, 1e-2f), "d18_pivot_00: BL hand-computed about pivot (0,0)");

  // Pivot at (1,1) (origin 1/1 == BR corner itself): BR stays put.
  SpriteTransform t_br{};
  t_br.origin_x = 1.f;
  t_br.origin_y = 1.f;
  t_br.rotation = kHalfPi;
  const SpriteCorners c_br = compute_sprite_corners(dst, t_br);
  check(near(c_br.br, Vec2F{10.f, 10.f}, 1e-2f), "d18_pivot_11: pivot corner (BR) stays put");
  check(near(c_br.tl, Vec2F{20.f, 0.f}, 1e-2f), "d18_pivot_11: TL hand-computed about pivot (1,1)");
}

void test_d18_negative_scale_mirrors() {
  // dst={0,0,10,10}, pivot=center=(5,5), scale_x=-1 (horizontal mirror), rotation=0.
  // TL(0,0): local=(-5,-5)*(-1,1)=(5,-5); result=(10,0) == original TR position.
  const RectF dst{0.f, 0.f, 10.f, 10.f};
  SpriteTransform t{};
  t.scale_x = -1.f;
  const SpriteCorners c = compute_sprite_corners(dst, t);
  check(near(c.tl, Vec2F{10.f, 0.f}, 1e-3f), "d18_negative_scale: TL mirrors to old TR position");
  check(near(c.tr, Vec2F{0.f, 0.f}, 1e-3f), "d18_negative_scale: TR mirrors to old TL position");
  check(near(c.br, Vec2F{0.f, 10.f}, 1e-3f), "d18_negative_scale: BR mirrors to old BL position");
  check(near(c.bl, Vec2F{10.f, 10.f}, 1e-3f), "d18_negative_scale: BL mirrors to old BR position");
}

void test_d18_anisotropic_scale_then_rotate_order() {
  // Adversarial-review gap (PROG-1): every other D18 case above keeps scale UNIFORM
  // (scale_x==scale_y, including the default 1/1) whenever rotation != 0, or keeps rotation==0
  // whenever scale is anisotropic (test_d18_negative_scale_mirrors' scale_x=-1 has rot=0). A
  // uniform scale COMMUTES with rotation (R*(s*v) == s*(R*v) when sx==sy), so "scale about the
  // pivot, THEN rotate" and "rotate, THEN scale" produce the SAME corners in every case above --
  // the qa-engineer swapped the two lines in compute_sprite_corners()'s apply() lambda and all
  // 93 tests still passed. This case sets BOTH scale_x != scale_y AND rotation != 0 at once, the
  // only combination where order is observable, closing the gap.
  //
  // dst={0,0,10,10} (TL=(0,0),TR=(10,0),BR=(10,10),BL=(0,10)), origin default 0.5/0.5 ->
  // pivot=(5,5). scale_x=2, scale_y=1 (anisotropic), rotation=pi/2 (cos=0,sin=1 exactly at this
  // angle in float). D18 literal: corner' = pivot + rot((corner - pivot) * (scale_x, scale_y),
  // rotation) -- scale FIRST, rotate SECOND, about the pivot, in that order.
  //
  // Hand-derivation for TL=(0,0):
  //   local  = (corner - pivot) * scale = ((0-5)*2, (0-5)*1) = (-10, -5)
  //   rotated = rot(local, pi/2) = (local.x*cos - local.y*sin, local.x*sin + local.y*cos)
  //           = (-10*0 - (-5)*1, -10*1 + (-5)*0) = (5, -10)
  //   result  = pivot + rotated = (5+5, 5-10) = (10, -5)
  // Same derivation for the other 3 corners (TR=(10,0), BR=(10,10), BL=(0,10)):
  //   TR: local=(10,-5) -> rotated=(5,10)  -> result=(10, 15)
  //   BR: local=(10, 5) -> rotated=(-5,10) -> result=( 0, 15)
  //   BL: local=(-10,5) -> rotated=(-5,-10)-> result=( 0, -5)
  //
  // Contrast with the WRONG order (rotate FIRST, then scale -- the exact mutation this test
  // guards against) for TL: rot((0,0)-(5,5), pi/2) = rot(-5,-5, pi/2) = (5,-5); then *scale(2,1)
  // = (10,-5); +pivot = (15, 0) -- a full (5,5) away from the correct (10,-5). Every corner
  // below differs by at least that much between the two orders, so the mutation fails LOUD, not
  // subtly.
  const RectF dst{0.f, 0.f, 10.f, 10.f};
  SpriteTransform t{};
  t.scale_x = 2.f;
  t.scale_y = 1.f;
  t.rotation = kHalfPi;
  const SpriteCorners c = compute_sprite_corners(dst, t);
  check(near(c.tl, Vec2F{10.f, -5.f}, 1e-2f),
        "d18_aniso_scale_then_rotate: TL hand-computed (scale-then-rotate order)");
  check(near(c.tr, Vec2F{10.f, 15.f}, 1e-2f),
        "d18_aniso_scale_then_rotate: TR hand-computed (scale-then-rotate order)");
  check(near(c.br, Vec2F{0.f, 15.f}, 1e-2f),
        "d18_aniso_scale_then_rotate: BR hand-computed (scale-then-rotate order)");
  check(near(c.bl, Vec2F{0.f, -5.f}, 1e-2f),
        "d18_aniso_scale_then_rotate: BL hand-computed (scale-then-rotate order)");
}

// ---------------------------------------------------------------------------
// D19 -- exact equivalence (identity transform, bit-for-bit) / equivalência exata
// ---------------------------------------------------------------------------

void test_d19_identity_exact_equivalence() {
  // dst chosen with values exactly representable in float (integers + .5 halves) so every
  // intermediate op (subtraction, *1.0f, *0.0f via cos(0)/sin(0), addition) is IEEE-exact --
  // this is the D2D-2A half of D19's pin: identity SpriteTransform{} reproduces dst's own 4
  // corners BIT-FOR-BIT (exact ==, not near()), matching the plan's own claim that identity
  // ops are IEEE-exact. The other half (the batcher's v0.20.0 path staying byte-identical
  // through the quad path) is D2D-2B's.
  const RectF dst{3.f, 7.f, 20.f, 15.f};
  const SpriteCorners c = compute_sprite_corners(dst, SpriteTransform{});
  check(c.tl.x == 3.f && c.tl.y == 7.f, "d19_identity: TL bit-exact");
  check(c.tr.x == 23.f && c.tr.y == 7.f, "d19_identity: TR bit-exact");
  check(c.br.x == 23.f && c.br.y == 22.f, "d19_identity: BR bit-exact");
  check(c.bl.x == 3.f && c.bl.y == 22.f, "d19_identity: BL bit-exact");
}

// ---------------------------------------------------------------------------
// D20 -- post-math overflow guard / guarda pós-conta de overflow
// ---------------------------------------------------------------------------

void test_d20_overflow_guard() {
  // Every field of `t` is individually finite (1e30f is a legal, finite float) -- the FIRST
  // D20 guard (is_finite(transform)) passes. But dst.w * scale_x = 1e30f * 1e30f = 1e60,
  // beyond float's ~3.4e38 range: the SECOND D20 guard (is_finite on the computed corners)
  // must catch the resulting Inf -- this is the guard with no v0.20.0 twin (the old
  // axis-aligned path has no multiplication that can overflow), named explicitly in D20.
  const RectF dst{0.f, 0.f, 1e30f, 1e30f};
  SpriteTransform t{};
  t.origin_x = 0.f;
  t.origin_y = 0.f;
  t.scale_x = 1e30f;
  check(is_finite(t), "d20_overflow: SpriteTransform's own fields are individually finite");
  const SpriteCorners c = compute_sprite_corners(dst, t);
  check(!is_finite(c), "d20_overflow: overflowed corner caught by is_finite(SpriteCorners)");
}

} // namespace

int main() {
  test_d12_offset_only();
  test_d12_zoom_only();
  test_d12_rotation_only();
  test_d12_combined();
  test_round_trip_grid();
  test_rotation_wrap();
  test_d16_totality_wts();
  test_d16_totality_stw();
  test_d17_valid_fit();
  test_d17_hostile();
  test_d18_rotation_permutes_corners();
  test_d18_pivot_variants();
  test_d18_negative_scale_mirrors();
  test_d18_anisotropic_scale_then_rotate_order();
  test_d19_identity_exact_equivalence();
  test_d20_overflow_guard();

  if (g_failures == 0) {
    std::puts("transform2d_sanity: PASS");
    return 0;
  }
  std::printf("transform2d_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
