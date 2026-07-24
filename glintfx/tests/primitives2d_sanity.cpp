// SPDX-License-Identifier: MPL-2.0
// EN: primitives2d_sanity (D2D-3A) -- headless unit test for the PURE geometry in
//     src/primitives2d.hpp (line-quad corners D25, rect-outline decomposition D26, scissor GL
//     mapping D28) AND the pure D29 bbox helper in src/image_decode.hpp. No GL, no Xvfb, no
//     window -- runs in BOTH build configs, same class of test as transform2d_sanity.cpp/
//     sprite_batch_sanity.cpp/image_decode_sanity.cpp. This is the delta-value test the plan's
//     own section 0 (risk 3) demands for a conversion formula ("a conversion formula without a
//     computed-value delta test" already cost a Critical once, and the onda-4 lesson applies
//     equally: a test that cannot distinguish the mutation does not exist) -- every expected
//     value below is DERIVED ON PAPER from the literal formulas in primitives2d.hpp's/
//     image_decode.hpp's own header comments, not just "does it not crash".
//     Covers: D25 hand-computed line corners (horizontal AND a 3-4-5 diagonal, asymmetric on
//     purpose -- a swapped normal or dropped minus sign fails a horizontal case too, but only the
//     diagonal case catches a swapped-AXIS normal, since a horizontal line's normal is (0,1)
//     either way), the thickness/2-vs-thickness mutation killer, is_degenerate_line() BEFORE any
//     division (a==b exact, thickness<=0/0, non-degenerate negative), a guard-order demo that
//     mirrors what draw2d.cpp (D2D-3B) will do (finite check first, degeneracy second, math
//     last -- proving no divide-by-zero path is EVER reached), and the D20-class post-math
//     overflow guard on an astronomically long+thick line. D26 hand-picked 4-strip values,
//     NON-OVERLAP (pairwise AABB check) + EXACT COVERAGE (union area == 2t(w+h)-4t^2, the
//     algebraic identity derived from the literal formula) asserts, the `2t == min(w,h)` boundary
//     tiling `dst` exactly, and `2t > min(w,h)` producing the identical (already-clamped) strips
//     -- proving the "one filled rect" degenerate case falls out of the SAME formula with no
//     special-case branch, per the header's own claim. D29 crafted RGBA arrays: single-pixel
//     corners at (0,0) and (w-1,h-1) (the off-by-one mutation targets), a two-pixel diagonal
//     spanning bbox, all-transparent (found==false), all-opaque (full rect), an alpha==1 pixel
//     counting (the alpha>0 definition pin, not alpha==255), and the total-function invalid-input
//     set (null data, w<=0, h<=0). D28 hand-computed glScissor mapping for TWO off-center rects
//     on a known target height (the y-flip killer -- an off-center rect makes a wrong sign move
//     the clipped band to a visibly different position, not just a slightly-off one) plus the
//     clamp of negative w/h extents to 0.
// PT: primitives2d_sanity (D2D-3A) -- teste unitário headless pra geometria PURA em
//     src/primitives2d.hpp (cantos de linha D25, decomposição de contorno D26, mapeamento GL de
//     scissor D28) E o helper puro D29 de bbox em src/image_decode.hpp. Sem GL, sem Xvfb, sem
//     janela -- roda nas DUAS configs de build, mesma classe de teste de transform2d_sanity.cpp/
//     sprite_batch_sanity.cpp/image_decode_sanity.cpp. Este é o teste de valor delta que a
//     própria seção 0 do plano (risco 3) exige pra fórmula de conversão ("fórmula de conversão
//     sem teste de valor computado" já custou um Critical uma vez, e a lição da onda-4 vale
//     igual: um teste que não distingue a mutação é um teste que não existe) -- todo valor
//     esperado abaixo é DERIVADO NO PAPEL a partir das fórmulas literais dos próprios comentários
//     de cabeçalho de primitives2d.hpp/image_decode.hpp, não só "não crashou".
//     Cobre: cantos de linha D25 computados à mão (horizontal E uma diagonal 3-4-5, assimétrica
//     de propósito -- uma normal trocada ou um sinal de menos derrubado falha até um caso
//     horizontal, mas só o caso diagonal pega uma normal com EIXO trocado, já que a normal de
//     uma linha horizontal é (0,1) dos dois jeitos), o mata-mutação thickness/2-vs-thickness,
//     is_degenerate_line() ANTES de qualquer divisão (a==b exato, thickness<=0/0, negativo
//     não-degenerado), uma demonstração de ordem-de-guarda que espelha o que draw2d.cpp (D2D-3B)
//     vai fazer (cheque de finitude primeiro, degenerescência segundo, conta por último --
//     provando que nenhum caminho de divisão-por-zero é JAMAIS alcançado), e a guarda de
//     overflow pós-conta classe-D20 numa linha astronomicamente longa+grossa. Valores das 4
//     faixas do D26 escolhidos à mão, cheques de NÃO-SOBREPOSIÇÃO (AABB par-a-par) + COBERTURA
//     EXATA (área de união == 2t(w+h)-4t^2, a identidade algébrica derivada da fórmula literal),
//     a fronteira `2t == min(w,h)` ladrilhando `dst` exatamente, e `2t > min(w,h)` produzindo as
//     MESMAS faixas (já clampadas) -- provando que o caso degenerado "um retângulo preenchido só"
//     sai da MESMA fórmula sem ramo de caso especial, conforme a própria alegação do header.
//     Arrays RGBA forjados do D29: cantos de pixel único em (0,0) e (w-1,h-1) (os alvos de
//     mutação off-by-one), bbox de diagonal de dois pixels, totalmente-transparente
//     (found==false), totalmente-opaco (retângulo cheio), um pixel alpha==1 contando (a fixação
//     da definição alpha>0, não alpha==255), e o conjunto de input inválido da função total (data
//     nulo, w<=0, h<=0). Mapeamento glScissor computado à mão do D28 pra DOIS retângulos
//     fora-do-centro numa altura-alvo conhecida (o mata-mutação de y-flip -- um retângulo
//     fora-do-centro faz um sinal errado mover a banda clipada pra uma posição visivelmente
//     diferente, não só levemente errada) mais o clamp das extensões w/h negativas pra 0.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/image_decode.hpp"
#include "../src/primitives2d.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <vector>

using glintfx::ColorF;
using glintfx::compute_content_bbox;
using glintfx::ContentBbox;
using glintfx::RectF;
using glintfx::Vec2F;
using glintfx::draw2d_detail::compute_line_corners;
using glintfx::draw2d_detail::compute_outline_strips;
using glintfx::draw2d_detail::is_degenerate_line;
using glintfx::draw2d_detail::is_degenerate_outline;
using glintfx::draw2d_detail::is_finite;
using glintfx::draw2d_detail::map_scissor_to_gl;
using glintfx::draw2d_detail::OutlineStrips;
using glintfx::draw2d_detail::ScissorGl;
using glintfx::draw2d_detail::SpriteCorners;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

bool near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }
bool near(Vec2F a, Vec2F b, float eps) { return near(a.x, b.x, eps) && near(a.y, b.y, eps); }
bool near(const RectF& a, const RectF& b, float eps) {
  return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.w, b.w, eps) && near(a.h, b.h, eps);
}

// EN: AABB overlap predicate (closed-open intervals -- two strips that only TOUCH at an edge, as
//     compute_outline_strips()'s own tiling produces, are NOT considered overlapping). Test-only,
//     mirrors the standard rect-intersection test; not part of production code.
// PT: Predicado de sobreposição AABB (intervalos fechado-aberto -- duas faixas que só SE TOCAM
//     numa borda, como a própria decomposição de compute_outline_strips() produz, NÃO contam
//     como sobrepostas). Só do teste, espelha o teste padrão de interseção de retângulos; não é
//     parte do código de produção.
bool rects_overlap(const RectF& a, const RectF& b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

// ---------------------------------------------------------------------------
// D25 -- line geometry / geometria de linha
// ---------------------------------------------------------------------------

void test_d25_horizontal_line() {
  // a=(0,0), b=(10,0), thickness=4. d=(10,0), len=10, u=(1,0), n=(0,1), h=2.
  // tl=a+n*h=(0,2), tr=b+n*h=(10,2), br=b-n*h=(10,-2), bl=a-n*h=(0,-2).
  const SpriteCorners c = compute_line_corners(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, 4.f);
  check(near(c.tl, Vec2F{0.f, 2.f}, 1e-4f), "d25_horizontal: TL hand-computed");
  check(near(c.tr, Vec2F{10.f, 2.f}, 1e-4f), "d25_horizontal: TR hand-computed");
  check(near(c.br, Vec2F{10.f, -2.f}, 1e-4f), "d25_horizontal: BR hand-computed");
  check(near(c.bl, Vec2F{0.f, -2.f}, 1e-4f), "d25_horizontal: BL hand-computed");
}

void test_d25_diagonal_asymmetric() {
  // a=(0,0), b=(3,4) -- a 3-4-5 triangle, deliberately NOT axis-aligned (a horizontal/vertical
  // line's normal is (0,1)/(1,0) either way, which cannot catch a swapped-axis normal --
  // n=(-u.y,u.x) vs the mutation n=(u.y,-u.x) or n=(u.x,u.y)). len=5, u=(0.6,0.8),
  // n=(-0.8,0.6), thickness=10 -> h=5, n*h=(-4,3).
  // tl=a+n*h=(-4,3); tr=b+n*h=(3-4,4+3)=(-1,7); br=b-n*h=(3+4,4-3)=(7,1); bl=a-n*h=(4,-3).
  const SpriteCorners c = compute_line_corners(Vec2F{0.f, 0.f}, Vec2F{3.f, 4.f}, 10.f);
  check(near(c.tl, Vec2F{-4.f, 3.f}, 1e-3f), "d25_diagonal: TL hand-computed (3-4-5)");
  check(near(c.tr, Vec2F{-1.f, 7.f}, 1e-3f), "d25_diagonal: TR hand-computed (3-4-5)");
  check(near(c.br, Vec2F{7.f, 1.f}, 1e-3f), "d25_diagonal: BR hand-computed (3-4-5)");
  check(near(c.bl, Vec2F{4.f, -3.f}, 1e-3f), "d25_diagonal: BL hand-computed (3-4-5)");
}

void test_d25_thickness_half_mutation_killer() {
  // The tl-to-bl (and tr-to-br) span must equal `thickness` EXACTLY -- a mutation that uses
  // `thickness` instead of `thickness/2` for `h` doubles this span to 2x, and one that inverts
  // the halving (uses thickness*2) quadruples it; either fails loudly against this exact value.
  const float thickness = 6.f;
  const SpriteCorners c = compute_line_corners(Vec2F{0.f, 0.f}, Vec2F{20.f, 0.f}, thickness);
  check(near(c.tl.y - c.bl.y, thickness, 1e-4f),
        "d25_thickness_half: TL-BL span equals thickness exactly (h=thickness/2 mutation killer)");
  check(near(c.tr.y - c.br.y, thickness, 1e-4f),
        "d25_thickness_half: TR-BR span equals thickness exactly");
}

void test_d25_is_degenerate_line() {
  check(is_degenerate_line(Vec2F{5.f, 5.f}, Vec2F{5.f, 5.f}, 4.f),
        "d25_degenerate: a==b exactly -> degenerate");
  check(is_degenerate_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, 0.f),
        "d25_degenerate: thickness==0 -> degenerate");
  check(is_degenerate_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, -1.f),
        "d25_degenerate: thickness<0 -> degenerate");
  check(!is_degenerate_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, 4.f),
        "d25_degenerate: normal a!=b, thickness>0 -> NOT degenerate");
}

// EN: Mirrors the guard order draw2d.cpp (D2D-3B) will implement for draw_line(): finite check
//     FIRST, degeneracy check SECOND, compute_line_corners() (the only place a division happens)
//     LAST -- and only when both guards pass. This is the mechanical proof "no divide-by-zero
//     path exists" (D25's own claim): the degenerate/non-finite branches below never reach the
//     line 2 (`std::sqrt`/division inside compute_line_corners), by construction.
// PT: Espelha a ordem de guarda que draw2d.cpp (D2D-3B) vai implementar pro draw_line(): cheque
//     de finitude PRIMEIRO, cheque de degenerescência SEGUNDO, compute_line_corners() (o único
//     lugar onde uma divisão acontece) POR ÚLTIMO -- e só quando as duas guardas passam. Esta é
//     a prova mecânica de "nenhum caminho de divisão-por-zero existe" (a própria alegação do
//     D25): os ramos degenerado/não-finito abaixo nunca alcançam a linha 2 (`std::sqrt`/divisão
//     dentro de compute_line_corners()), por construção.
bool would_skip_line(Vec2F a, Vec2F b, float thickness, const ColorF& color) {
  if (!is_finite(a) || !is_finite(b) || !std::isfinite(thickness) || !is_finite(color))
    return true; // NonFinite -- rejected BEFORE is_degenerate_line()/compute_line_corners().
  if (is_degenerate_line(a, b, thickness))
    return true; // Degenerate -- rejected BEFORE compute_line_corners()'s division.
  return false;
}

void test_d25_guard_order_before_math() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  const ColorF ok_color{};

  check(would_skip_line(Vec2F{nan, 0.f}, Vec2F{10.f, 0.f}, 4.f, ok_color),
        "d25_guard_order: NaN a.x skips BEFORE degeneracy/math is ever consulted");
  check(would_skip_line(Vec2F{0.f, 0.f}, Vec2F{inf, 0.f}, 4.f, ok_color),
        "d25_guard_order: Inf b.x skips BEFORE degeneracy/math");
  check(would_skip_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, nan, ok_color),
        "d25_guard_order: NaN thickness skips BEFORE degeneracy/math");
  check(would_skip_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, 4.f, ColorF{nan, 1.f, 1.f, 1.f}),
        "d25_guard_order: NaN color.r skips (colour is part of the D25 literal guard order)");
  check(would_skip_line(Vec2F{5.f, 5.f}, Vec2F{5.f, 5.f}, 4.f, ok_color),
        "d25_guard_order: finite-but-degenerate (a==b) still skips, after the finite check passes");
  check(!would_skip_line(Vec2F{0.f, 0.f}, Vec2F{10.f, 0.f}, 4.f, ok_color),
        "d25_guard_order: finite AND non-degenerate reaches compute_line_corners()");
}

void test_d25_post_math_overflow_guard() {
  // Every field individually finite (3e38f/1e38f are legal, finite floats, both under FLT_MAX
  // ~3.402823e38) -- but the endpoint is placed NEAR FLT_MAX so the perpendicular OFFSET
  // (thickness/2 added on top of it) pushes the result past FLT_MAX into Inf. `d = b - a`
  // cancels the shared 3e38 term EXACTLY (b.y == a.y, so d.y == 0 with no precision loss), so
  // `len`/`u`/`n` themselves stay small and finite -- the overflow is isolated to the FINAL
  // addition (`a.y + n.y*h` = 3e38 + 5e37 = 3.5e38 > FLT_MAX), exactly the D20-class SECOND
  // guard (no v0.20.0 twin: the old axis-aligned sprite path has no such addition against an
  // already-near-max coordinate) that draw2d.cpp checks on the PROJECTED corners.
  const Vec2F a{0.f, 3e38f};
  const Vec2F b{10.f, 3e38f};
  const float thickness = 1e38f;
  check(std::isfinite(a.y) && std::isfinite(b.y) && std::isfinite(thickness),
        "d25_overflow: every individual field is finite going in (all under FLT_MAX)");
  check(!is_degenerate_line(a, b, thickness), "d25_overflow: not degenerate (a!=b, thickness>0)");
  const SpriteCorners c = compute_line_corners(a, b, thickness);
  check(!is_finite(c), "d25_overflow: overflowed corner caught by is_finite(SpriteCorners)");
}

// ---------------------------------------------------------------------------
// D26 -- rect outline decomposition / decomposicao de contorno
// ---------------------------------------------------------------------------

void test_d26_hand_picked_strips() {
  // dst={10,20,100,50}, thickness=8. min(w,h)=50, half_min=25, t=min(8,25)=8 (unclamped).
  const RectF dst{10.f, 20.f, 100.f, 50.f};
  const float thickness = 8.f;
  const OutlineStrips s = compute_outline_strips(dst, thickness);
  check(near(s.top, RectF{10.f, 20.f, 100.f, 8.f}, 1e-4f), "d26_strips: top hand-computed");
  check(near(s.bottom, RectF{10.f, 62.f, 100.f, 8.f}, 1e-4f), "d26_strips: bottom hand-computed");
  check(near(s.left, RectF{10.f, 28.f, 8.f, 34.f}, 1e-4f), "d26_strips: left hand-computed");
  check(near(s.right, RectF{102.f, 28.f, 8.f, 34.f}, 1e-4f), "d26_strips: right hand-computed");
}

void test_d26_non_overlap_and_exact_coverage() {
  const RectF dst{10.f, 20.f, 100.f, 50.f};
  const float thickness = 8.f;
  const OutlineStrips s = compute_outline_strips(dst, thickness);

  check(!rects_overlap(s.top, s.bottom), "d26_coverage: top/bottom do not overlap");
  check(!rects_overlap(s.top, s.left), "d26_coverage: top/left do not overlap");
  check(!rects_overlap(s.top, s.right), "d26_coverage: top/right do not overlap");
  check(!rects_overlap(s.bottom, s.left), "d26_coverage: bottom/left do not overlap");
  check(!rects_overlap(s.bottom, s.right), "d26_coverage: bottom/right do not overlap");
  check(!rects_overlap(s.left, s.right), "d26_coverage: left/right do not overlap");

  // EN: because the 4 strips are pairwise non-overlapping (just proved above), the union area is
  //     the plain SUM of the 4 rect areas -- compared against the algebraic identity
  //     2t(w+h)-4t^2 derived from the literal per-strip formula (top+bottom = 2*w*t, left+right =
  //     2*t*(h-2t) = 2th-4t^2; sum = 2wt+2th-4t^2 = 2t(w+h)-4t^2).
  // PT: como as 4 faixas são par-a-par sem sobreposição (provado acima), a área de união é a
  //     SOMA simples das 4 áreas -- comparada com a identidade algébrica 2t(w+h)-4t^2 derivada
  //     da fórmula literal por-faixa (top+bottom = 2*w*t, left+right = 2*t*(h-2t) = 2th-4t^2;
  //     soma = 2wt+2th-4t^2 = 2t(w+h)-4t^2).
  const float union_area = s.top.w * s.top.h + s.bottom.w * s.bottom.h + s.left.w * s.left.h +
                           s.right.w * s.right.h;
  const float t = thickness; // unclamped in this hand-picked case (8 < half_min 25).
  const float expected = 2.f * t * (dst.w + dst.h) - 4.f * t * t;
  check(near(union_area, expected, 1e-2f),
        "d26_coverage: union area == 2t(w+h)-4t^2 (the exact-coverage algebraic identity)");
}

void test_d26_collapse_boundary_and_beyond() {
  // dst={0,0,30,50}: min(w,h)=30, half_min=15. At thickness==15, 2t==min(w,h) EXACTLY (the
  // boundary); at thickness==100, 2t>min(w,h) (well past it). Both clamp to the SAME t=15 --
  // the header's own claim that no special-case branch is needed, pinned by asserting the two
  // produce IDENTICAL strips and that their union covers dst EXACTLY (the single-filled-rect
  // degenerate case).
  const RectF dst{0.f, 0.f, 30.f, 50.f};
  const OutlineStrips at_boundary = compute_outline_strips(dst, 15.f);
  const OutlineStrips past_boundary = compute_outline_strips(dst, 100.f);

  check(near(at_boundary.top, past_boundary.top, 1e-4f) &&
            near(at_boundary.bottom, past_boundary.bottom, 1e-4f) &&
            near(at_boundary.left, past_boundary.left, 1e-4f) &&
            near(at_boundary.right, past_boundary.right, 1e-4f),
        "d26_collapse: 2t==min(w,h) and 2t>min(w,h) produce IDENTICAL clamped strips");

  const float union_area = at_boundary.top.w * at_boundary.top.h +
                           at_boundary.bottom.w * at_boundary.bottom.h +
                           at_boundary.left.w * at_boundary.left.h +
                           at_boundary.right.w * at_boundary.right.h;
  check(near(union_area, dst.w * dst.h, 1e-2f),
        "d26_collapse: the collapsed strips' union area equals dst's own full area exactly");
  check(!rects_overlap(at_boundary.left, at_boundary.right),
        "d26_collapse: left/right still do not overlap at the exact collapse boundary");
}

void test_d26_is_degenerate_outline() {
  check(is_degenerate_outline(RectF{0.f, 0.f, 10.f, 10.f}, 0.f),
        "d26_degenerate: thickness==0 -> degenerate");
  check(is_degenerate_outline(RectF{0.f, 0.f, 10.f, 10.f}, -1.f),
        "d26_degenerate: thickness<0 -> degenerate");
  check(is_degenerate_outline(RectF{0.f, 0.f, 0.f, 10.f}, 2.f),
        "d26_degenerate: dst.w<=0 -> degenerate");
  check(is_degenerate_outline(RectF{0.f, 0.f, 10.f, -5.f}, 2.f),
        "d26_degenerate: dst.h<=0 -> degenerate");
  check(!is_degenerate_outline(RectF{0.f, 0.f, 10.f, 10.f}, 2.f),
        "d26_degenerate: normal positive dst/thickness -> NOT degenerate");
}

// ---------------------------------------------------------------------------
// D29 -- texture content bbox / bbox de conteudo de textura
// ---------------------------------------------------------------------------

// EN: Builds a w*h RGBA8 buffer, alpha 0 everywhere except the listed (col,row,alpha) texels --
//     RGB channels are irrelevant to D29 (alpha>0 is the only predicate), left at 0. Test-only.
// PT: Monta um buffer RGBA8 w*h, alpha 0 em toda parte exceto os texels (col,row,alpha)
//     listados -- os canais RGB são irrelevantes pro D29 (alpha>0 é o único predicado), ficam em
//     0. Só do teste.
std::vector<unsigned char> make_rgba(int w, int h,
                                     std::initializer_list<std::array<int, 3>> opaque_texels) {
  std::vector<unsigned char> buf(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u,
                                 0);
  for (const auto& t : opaque_texels) {
    const int col = t[0], row = t[1], alpha = t[2];
    const std::size_t idx = (static_cast<std::size_t>(row) * static_cast<std::size_t>(w) +
                             static_cast<std::size_t>(col)) *
                                4u +
                            3u;
    buf[idx] = static_cast<unsigned char>(alpha);
  }
  return buf;
}

void test_d29_single_pixel_corners() {
  // 3x3 image, one opaque pixel at (0,0) (the TL corner, an off-by-one mutation target).
  const std::vector<unsigned char> img_tl = make_rgba(3, 3, {{0, 0, 255}});
  const ContentBbox c_tl = compute_content_bbox(img_tl.data(), 3, 3);
  check(c_tl.found, "d29_corners: (0,0) pixel found");
  check(c_tl.x == 0 && c_tl.y == 0 && c_tl.w == 1 && c_tl.h == 1,
        "d29_corners: (0,0) -> {true,0,0,1,1} exactly");

  // 3x3 image, one opaque pixel at (w-1,h-1)=(2,2) (the BR corner, the OTHER off-by-one target).
  const std::vector<unsigned char> img_br = make_rgba(3, 3, {{2, 2, 255}});
  const ContentBbox c_br = compute_content_bbox(img_br.data(), 3, 3);
  check(c_br.found, "d29_corners: (w-1,h-1) pixel found");
  check(c_br.x == 2 && c_br.y == 2 && c_br.w == 1 && c_br.h == 1,
        "d29_corners: (2,2) -> {true,2,2,1,1} exactly (the w-1/h-1 off-by-one pin)");
}

void test_d29_diagonal_spanning_bbox() {
  // 3x3 image, opaque pixels at (0,0) and (2,2) -- spans the full image.
  const std::vector<unsigned char> img = make_rgba(3, 3, {{0, 0, 255}, {2, 2, 255}});
  const ContentBbox c = compute_content_bbox(img.data(), 3, 3);
  check(c.found, "d29_diagonal: found");
  check(c.x == 0 && c.y == 0 && c.w == 3 && c.h == 3,
        "d29_diagonal: two-pixel diagonal spans the full 3x3 bbox");
}

void test_d29_all_transparent_and_all_opaque() {
  const std::vector<unsigned char> transparent = make_rgba(2, 2, {}); // no opaque texels at all.
  const ContentBbox c_empty = compute_content_bbox(transparent.data(), 2, 2);
  check(!c_empty.found, "d29_transparent: fully-transparent image -> found == false");
  check(c_empty.x == 0 && c_empty.y == 0 && c_empty.w == 0 && c_empty.h == 0,
        "d29_transparent: every field left at the default-constructed 0");

  const std::vector<unsigned char> opaque =
      make_rgba(2, 2, {{0, 0, 255}, {1, 0, 255}, {0, 1, 255}, {1, 1, 255}});
  const ContentBbox c_full = compute_content_bbox(opaque.data(), 2, 2);
  check(c_full.found, "d29_opaque: fully-opaque image -> found == true");
  check(c_full.x == 0 && c_full.y == 0 && c_full.w == 2 && c_full.h == 2,
        "d29_opaque: fully-opaque 2x2 -> full {0,0,2,2}");
}

void test_d29_alpha_one_counts() {
  // The `alpha > 0` definition pin: alpha==1 (the smallest nonzero byte) COUNTS, not just
  // alpha==255. If a mutation changed the predicate to `alpha == 255` or `alpha >= 128`, this
  // single-texel-at-alpha-1 case would flip to found==false.
  const std::vector<unsigned char> img = make_rgba(2, 2, {{1, 1, 1}});
  const ContentBbox c = compute_content_bbox(img.data(), 2, 2);
  check(c.found, "d29_alpha_one: a texel with alpha==1 counts as content");
  check(c.x == 1 && c.y == 1 && c.w == 1 && c.h == 1, "d29_alpha_one: bbox pinpoints that texel");
}

void test_d29_total_invalid_input() {
  const std::vector<unsigned char> img = make_rgba(2, 2, {{0, 0, 255}});
  check(!compute_content_bbox(nullptr, 2, 2).found, "d29_invalid: null rgba -> found==false");
  check(!compute_content_bbox(img.data(), 0, 2).found, "d29_invalid: w<=0 -> found==false");
  check(!compute_content_bbox(img.data(), 2, 0).found, "d29_invalid: h<=0 -> found==false");
  check(!compute_content_bbox(img.data(), -1, 2).found, "d29_invalid: negative w -> found==false");
}

// ---------------------------------------------------------------------------
// D28 -- scissor GL mapping / mapeamento GL do scissor
// ---------------------------------------------------------------------------

void test_d28_off_center_mapping() {
  // rect_px={20,30,40,50}, target_h=200 (deliberately OFF-CENTER: y=30 is far from
  // target_h/2=100, so a y-flip sign error moves gl_y to a value nowhere near the correct one,
  // not just slightly off). gl_x=20 (unchanged), gl_y=200-(30+50)=120, gl_w=40, gl_h=50.
  const ScissorGl s1 = map_scissor_to_gl(RectF{20.f, 30.f, 40.f, 50.f}, 200);
  check(s1.x == 20, "d28_mapping_1: gl_x passes through unchanged");
  check(s1.y == 120, "d28_mapping_1: gl_y = target_h-(y+h) = 200-80 = 120 (the y-flip pin)");
  check(s1.w == 40 && s1.h == 50, "d28_mapping_1: gl_w/gl_h pass through unchanged (positive)");

  // A SECOND off-center rect near the BOTTOM of the window (large y), on the same target_h --
  // an inverted/mirrored formula (e.g. using `y` directly instead of `target_h-(y+h)`) would
  // move this rect's clipped band to the WRONG half of the screen, failing loudly, not subtly.
  const ScissorGl s2 = map_scissor_to_gl(RectF{5.f, 150.f, 10.f, 40.f}, 200);
  check(s2.x == 5, "d28_mapping_2: gl_x passes through unchanged");
  check(s2.y == 10, "d28_mapping_2: gl_y = 200-(150+40) = 10 (near-bottom rect maps near gl_y=0)");
  check(s2.w == 10 && s2.h == 40, "d28_mapping_2: gl_w/gl_h pass through unchanged");
}

void test_d28_negative_extent_clamp() {
  // rect_px.w negative -> gl_w clamps to 0 (the "extents" clamp); rect_px.h stays positive and
  // passes through, and the y-flip formula uses the RAW (pre-clamp) h, per the literal formula.
  const ScissorGl s_neg_w = map_scissor_to_gl(RectF{0.f, 0.f, -5.f, 10.f}, 100);
  check(s_neg_w.w == 0, "d28_clamp: negative rect_px.w clamps to gl_w==0");
  check(s_neg_w.h == 10, "d28_clamp: positive rect_px.h passes through unclamped");
  check(s_neg_w.y == 90, "d28_clamp: gl_y = 100-(0+10) = 90, unaffected by the w clamp");

  // rect_px.h negative -> gl_h clamps to 0 (a "clip everything" outcome downstream, consistent
  // with set_scissor()'s own documented `h<=0` legal-empty-scissor semantics).
  const ScissorGl s_neg_h = map_scissor_to_gl(RectF{0.f, 0.f, 10.f, -5.f}, 100);
  check(s_neg_h.h == 0, "d28_clamp: negative rect_px.h clamps to gl_h==0");
  check(s_neg_h.w == 10, "d28_clamp: positive rect_px.w passes through unclamped");
}

} // namespace

int main() {
  test_d25_horizontal_line();
  test_d25_diagonal_asymmetric();
  test_d25_thickness_half_mutation_killer();
  test_d25_is_degenerate_line();
  test_d25_guard_order_before_math();
  test_d25_post_math_overflow_guard();

  test_d26_hand_picked_strips();
  test_d26_non_overlap_and_exact_coverage();
  test_d26_collapse_boundary_and_beyond();
  test_d26_is_degenerate_outline();

  test_d29_single_pixel_corners();
  test_d29_diagonal_spanning_bbox();
  test_d29_all_transparent_and_all_opaque();
  test_d29_alpha_one_counts();
  test_d29_total_invalid_input();

  test_d28_off_center_mapping();
  test_d28_negative_extent_clamp();

  if (g_failures == 0) {
    std::puts("primitives2d_sanity: PASS");
    return 0;
  }
  std::printf("primitives2d_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
