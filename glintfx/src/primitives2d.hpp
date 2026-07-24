// SPDX-License-Identifier: MPL-2.0
// EN: D2D-3A -- the PURE geometry behind Draw2D's untextured primitives (plan
//     docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md, decisions D25/D26/D28; ADR-
//     0017's Addendum's Q1=(c) camera model is what "ACTIVE space" below refers to). ZERO GL
//     dependency, same discipline as src/sprite_batch.hpp/src/transform2d.hpp: this header takes
//     endpoints/rects/thickness and returns corners/rects/mapped-ints, headless-testable
//     (tests/primitives2d_sanity.cpp), with draw2d.cpp (D2D-3B) owning ONLY the GL execution and
//     the camera-projection/fail-high orchestration of what this header computed. Includes
//     transform2d.hpp for `SpriteCorners` (the SAME TL,TR,BR,BL 4-corner type sprite/camera math
//     already uses, D5/D19 -- reusing it here avoids a duplicate POD family, same rationale as
//     transform2d.hpp reusing draw2d.hpp's own Vec2F/RectF) and its `is_finite(Vec2F)`/
//     `is_finite(const RectF&)` predicates (single source, no redefinition -- see the guard-order
//     note below for why this header does NOT re-declare `is_finite(Vec2F)`/`is_finite(RectF)`).
//
//     GUARD SPLIT (same discipline as transform2d.hpp's compute_sprite_corners/is_finite split):
//     `compute_line_corners()`/`compute_outline_strips()` ASSUME their inputs already passed the
//     relevant `is_degenerate_*()` check below (false) and the relevant finiteness predicates
//     (`is_finite(Vec2F)`/`is_finite(const RectF&)` from transform2d.hpp, plus `std::isfinite()`
//     on the raw `thickness`/`color` scalars/struct at the call site) -- neither function
//     re-validates, and `compute_line_corners()` is not itself total the way
//     `world_to_screen`/`screen_to_world` are (an astronomic `thickness` on an astronomic `a`/`b`
//     can still overflow a finite input into a non-finite corner, D24's SECOND guard, no v0.20.0
//     twin by design; use `is_finite(const SpriteCorners&)`/`is_finite(const OutlineStrips&)`
//     after calling either to implement that guard, exactly the pattern
//     compute_sprite_corners()'s own header comment already establishes).
//
//     D25 LINE GEOMETRY, literal: with `d = b - a`, `len = |d|`, unit `u = d/len`,
//     `n = (-u.y, u.x)`, `h = thickness/2`: `tl = a+n*h, tr = b+n*h, br = b-n*h, bl = a-n*h`
//     (TL,TR,BR,BL, matching emit_quad_corners()'s slot-UV pairing). Butt caps (the quad ends
//     exactly at `a`/`b`); round/miter caps and polylines are seed `SEED-D2D-LINECAPS`, out of
//     this wave. `is_degenerate_line()` below is the caller's cue to skip the division entirely
//     (`a == b` exactly, or `thickness <= 0`) -- NO divide-by-zero path exists in
//     `compute_line_corners()` because the caller never invokes it when degenerate.
//
//     D26 RECT OUTLINE, literal: with `t = min(thickness, min(dst.w, dst.h)/2)` (the collapse
//     clamp), the four strips are `top={x,y,w,t}`, `bottom={x,y+h-t,w,t}`, `left={x,y+t,t,h-2t}`,
//     `right={x+w-t,y+t,t,h-2t}`. Non-overlap is a CORRECTNESS property (a translucent colour
//     double-blends at the corners of overlapping strips, visibly darker) -- pinned by this
//     header's own tests via a union-area/pairwise-intersection-empty computed check. The SAME
//     clamped formula, with no special-case branch, tiles `dst` EXACTLY once
//     `2*thickness >= min(dst.w, dst.h)` (the collapse-to-one-filled-rect case, proved by
//     algebra in this file's own test, not asserted by inspection alone).
//
//     D28 SCISSOR GL MAPPING, literal (the y-flip conversion, risk 3 of the plan's own section 0
//     -- "a conversion formula without a computed-value delta test" is this house's named
//     silent-bug class): `glScissor(x, target_h - (y + h), w, h)`, with `w`/`h` (the GL
//     "extents") clamped to non-negative BEFORE the int cast (`x`/`y`, the ORIGIN, are legal
//     negative per D28's own "clamping is the GPU's job" rule and are left untouched by this
//     mapping). `map_scissor_to_gl()` lives here rather than in draw2d.cpp so this header's own
//     test suite (D2D-3A) can pin it with a hand-computed off-center value BEFORE draw2d.cpp
//     (D2D-3B) ever calls it from `set_gl_state_for_draw()` -- single source, the same
//     "implemented ONCE" discipline transform2d.hpp's own `rotate()` already established.
// PT: D2D-3A -- a geometria PURA por trás das primitivas não-texturizadas do Draw2D (plano
//     docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md, decisões D25/D26/D28; o
//     modelo de câmera do Adendo da ADR-0017, Q1=(c), é o que "espaço ATIVO" abaixo referencia).
//     ZERO dependência de GL, mesma disciplina de src/sprite_batch.hpp/src/transform2d.hpp: este
//     header recebe pontas/retângulos/espessura e devolve cantos/retângulos/inteiros-mapeados,
//     testável headless (tests/primitives2d_sanity.cpp), com o draw2d.cpp (D2D-3B) sendo dono SÓ
//     da execução GL e da orquestração de projeção-de-câmera/fail-high do que este header
//     computou. Inclui transform2d.hpp pro `SpriteCorners` (o MESMO tipo de 4 cantos
//     TL,TR,BR,BL que a matemática de sprite/câmera já usa, D5/D19 -- reusá-lo aqui evita uma
//     família de POD duplicada, mesma racional de transform2d.hpp reusando o próprio
//     Vec2F/RectF de draw2d.hpp) e seus predicados `is_finite(Vec2F)`/`is_finite(const
//     RectF&)` (fonte única, sem redefinição -- ver a nota de ordem-de-guarda abaixo pro
//     porquê deste header NÃO redeclarar `is_finite(Vec2F)`/`is_finite(RectF)`).
//
//     SPLIT DE GUARDA (mesma disciplina do split compute_sprite_corners/is_finite de
//     transform2d.hpp): `compute_line_corners()`/`compute_outline_strips()` ASSUMEM que os
//     próprios inputs já passaram pelo `is_degenerate_*()` relevante abaixo (false) e pelos
//     predicados de finitude relevantes (`is_finite(Vec2F)`/`is_finite(const RectF&)` de
//     transform2d.hpp, mais `std::isfinite()` sobre os escalares/struct crus `thickness`/`color`
//     no sítio de chamada) -- nenhuma das duas funções revalida, e `compute_line_corners()` não é
//     total do jeito que `world_to_screen`/`screen_to_world` são (um `thickness` astronômico
//     sobre um `a`/`b` astronômico ainda pode estourar entrada finita num canto não-finito,
//     SEGUNDA guarda do D24, sem gêmeo na v0.20.0 por design; use `is_finite(const
//     SpriteCorners&)`/`is_finite(const OutlineStrips&)` depois de chamar qualquer uma das duas
//     pra implementar essa guarda, exatamente o padrão que o próprio comentário de cabeçalho de
//     compute_sprite_corners() já estabelece).
//
//     GEOMETRIA DE LINHA D25, literal: com `d = b - a`, `len = |d|`, unitário `u = d/len`,
//     `n = (-u.y, u.x)`, `h = thickness/2`: `tl = a+n*h, tr = b+n*h, br = b-n*h, bl = a-n*h`
//     (TL,TR,BR,BL, batendo com o pareamento de slot-UV do emit_quad_corners()). Tampas butt (o
//     quad termina exatamente em `a`/`b`); tampas round/miter e polylines são a semente
//     `SEED-D2D-LINECAPS`, fora desta onda. O `is_degenerate_line()` abaixo é a deixa do chamador
//     pra pular a divisão inteiramente (`a == b` exatamente, ou `thickness <= 0`) -- NENHUM
//     caminho de divisão-por-zero existe em `compute_line_corners()` porque o chamador nunca a
//     invoca quando degenerado.
//
//     CONTORNO DE RETÂNGULO D26, literal: com `t = min(thickness, min(dst.w, dst.h)/2)` (o clamp
//     de colapso), as quatro faixas são `top={x,y,w,t}`, `bottom={x,y+h-t,w,t}`,
//     `left={x,y+t,t,h-2t}`, `right={x+w-t,y+t,t,h-2t}`. Não-sobreposição é uma propriedade de
//     CORREÇÃO (uma cor translúcida double-blenda nos cantos de faixas sobrepostas, visivelmente
//     mais escura) -- fixada pelos próprios testes deste header via um cheque computado de
//     área-de-união/interseção-par-a-par-vazia. A MESMA fórmula clampada, sem ramo de caso
//     especial, ladrilha `dst` EXATAMENTE quando `2*thickness >= min(dst.w, dst.h)` (o caso de
//     colapso pra um retângulo preenchido só, provado por álgebra no próprio teste deste
//     arquivo, não afirmado só por inspeção).
//
//     MAPEAMENTO GL DO SCISSOR D28, literal (a conversão de y-flip, risco 3 da própria seção 0
//     do plano -- "fórmula de conversão sem teste de valor computado" é a classe de bug
//     silencioso nomeada desta casa): `glScissor(x, target_h - (y + h), w, h)`, com `w`/`h` (a
//     "extensão" GL) clampados a não-negativo ANTES do cast pra int (`x`/`y`, a ORIGEM, são
//     legalmente negativos pela própria regra "clampar é trabalho da GPU" do D28 e ficam
//     intocados por este mapeamento). `map_scissor_to_gl()` mora aqui em vez de em draw2d.cpp
//     pra que a própria suíte de teste deste header (D2D-3A) possa fixá-la com um valor
//     fora-do-centro computado à mão ANTES do draw2d.cpp (D2D-3B) sequer chamá-la de dentro do
//     `set_gl_state_for_draw()` -- fonte única, a mesma disciplina "implementado UMA vez" que o
//     próprio `rotate()` de transform2d.hpp já estabeleceu.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "transform2d.hpp"

#include <cmath>

namespace glintfx::draw2d_detail {

// EN: D26 -- the 4 non-overlapping strips of a rect outline, in top/bottom/left/right order (not
//     a winding order -- each field is an independent RectF, D5's coordinate space). Not part of
//     the public API -- internal to this header and draw2d.cpp.
// PT: D26 -- as 4 faixas sem sobreposição de um contorno de retângulo, na ordem
//     topo/base/esquerda/direita (não é ordem de giro -- cada campo é um RectF independente,
//     espaço de coordenadas do D5). Não é parte da API pública -- interno a este header e ao
//     draw2d.cpp.
struct OutlineStrips {
  RectF top;
  RectF bottom;
  RectF left;
  RectF right;
};

// EN: D28 -- the 4 integer arguments `glScissor()` takes, already y-flipped and extent-clamped
//     (see map_scissor_to_gl() below). Not part of the public API.
// PT: D28 -- os 4 argumentos inteiros que `glScissor()` recebe, já com y-flip e clamp de
//     extensão aplicados (ver map_scissor_to_gl() abaixo). Não é parte da API pública.
struct ScissorGl {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

// EN: is_finite(ColorF) -- D25/D26's own literal guard order names `color` alongside the
//     geometry inputs ("all of a/b/thickness/color finite BEFORE any arithmetic"); this predicate
//     is the single source for that check, reused by this file's own tests AND by draw2d.cpp
//     (D2D-3B) instead of each re-deriving it (sprite_batch.hpp's SpriteBatch class has its own
//     PRIVATE all_finite(ColorF), a different scope -- no redefinition conflict, that one is a
//     class member, this one is a free function in draw2d_detail). Pure, cannot log.
// PT: is_finite(ColorF) -- a própria ordem de guarda literal do D25/D26 nomeia `color` junto dos
//     inputs de geometria ("a/b/thickness/color todos finitos ANTES de qualquer aritmética");
//     este predicado é a fonte única pra esse cheque, reusado pelos próprios testes deste
//     arquivo E pelo draw2d.cpp (D2D-3B) em vez de cada um re-derivar -- a classe SpriteBatch de
//     sprite_batch.hpp tem o próprio all_finite(ColorF) PRIVADO, escopo diferente (sem conflito
//     de redefinição, aquele é membro de classe, este é função livre em draw2d_detail). Pura,
//     não pode logar.
inline bool is_finite(const ColorF& c) {
  return std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b) && std::isfinite(c.a);
}

// EN: is_finite(OutlineStrips) -- the D24 post-math guard's counterpart for the outline's 4
//     strips (each strip is itself checked via transform2d.hpp's own is_finite(const RectF&),
//     reused, not redefined). Pure, cannot log.
// PT: is_finite(OutlineStrips) -- a contraparte da guarda pós-conta do D24 pras 4 faixas do
//     contorno (cada faixa é checada pelo próprio is_finite(const RectF&) de transform2d.hpp,
//     reusado, não redefinido). Pura, não pode logar.
inline bool is_finite(const OutlineStrips& s) {
  return is_finite(s.top) && is_finite(s.bottom) && is_finite(s.left) && is_finite(s.right);
}

// EN: D25 guard -- true when the line is degenerate: `a == b` EXACTLY (matching `len == 0`
//     BEFORE any sqrt/division is attempted -- no epsilon, the plan's own literal wording) or
//     `thickness <= 0`. The caller's cue to skip compute_line_corners() entirely (a SILENT legal
//     no-op, D10's idiom) instead of letting it divide by zero. Does NOT itself check finiteness
//     (that is is_finite(Vec2F)/std::isfinite(thickness), checked BEFORE this call, per D25's
//     literal guard order -- finite check first, degeneracy check second, arithmetic last).
// PT: Guarda D25 -- true quando a linha é degenerada: `a == b` EXATAMENTE (batendo com
//     `len == 0` ANTES de qualquer sqrt/divisão ser tentada -- sem epsilon, a própria letra do
//     plano) ou `thickness <= 0`. A deixa do chamador pra pular compute_line_corners()
//     inteiramente (um no-op legal SILENCIOSO, idioma D10) em vez de deixá-la dividir por zero.
//     NÃO checa finitude sozinha (isso é is_finite(Vec2F)/std::isfinite(thickness), checado
//     ANTES desta chamada, pela própria ordem de guarda literal do D25 -- cheque de finitude
//     primeiro, cheque de degenerescência segundo, aritmética por último).
inline bool is_degenerate_line(Vec2F a, Vec2F b, float thickness) {
  return (a.x == b.x && a.y == b.y) || thickness <= 0.f;
}

// EN: D25, literal. ASSUMES !is_degenerate_line(a, b, thickness) and that a/b/thickness already
//     passed finiteness (the caller's job, see this file's header comment and
//     is_degenerate_line() above) -- does not itself guard against either, and is not total the
//     way world_to_screen/screen_to_world are (see the SECOND-guard note in this file's header
//     comment). Returns TL,TR,BR,BL (SpriteCorners, transform2d.hpp) -- the SAME 4-corner type
//     and winding draw2d.cpp already feeds into SpriteBatch::draw_quad() for the camera/transform
//     path (D18/D19), so a line's corners compose into the exact same downstream call as a
//     sprite's.
// PT: D25, literal. ASSUME !is_degenerate_line(a, b, thickness) e que a/b/thickness já passaram
//     por finitude (trabalho do chamador, ver o comentário de cabeçalho deste arquivo e
//     is_degenerate_line() acima) -- não guarda sozinha contra nenhum dos dois, e não é total do
//     jeito que world_to_screen/screen_to_world são (ver a nota de SEGUNDA guarda no comentário
//     de cabeçalho deste arquivo). Devolve TL,TR,BR,BL (SpriteCorners, transform2d.hpp) -- o
//     MESMO tipo e giro de 4 cantos que o draw2d.cpp já alimenta em SpriteBatch::draw_quad() pro
//     caminho de câmera/transform (D18/D19), então os cantos de uma linha compõem pro EXATO
//     mesmo call site posterior de um sprite.
inline SpriteCorners compute_line_corners(Vec2F a, Vec2F b, float thickness) {
  const Vec2F d{b.x - a.x, b.y - a.y};
  const float len = std::sqrt(d.x * d.x + d.y * d.y);
  const Vec2F u{d.x / len, d.y / len};
  const Vec2F n{-u.y, u.x};
  const float h = thickness * 0.5f;
  return SpriteCorners{
      Vec2F{a.x + n.x * h, a.y + n.y * h}, // tl
      Vec2F{b.x + n.x * h, b.y + n.y * h}, // tr
      Vec2F{b.x - n.x * h, b.y - n.y * h}, // br
      Vec2F{a.x - n.x * h, a.y - n.y * h}, // bl
  };
}

// EN: D26 guard -- true when the outline is degenerate: `thickness <= 0` or
//     `dst.w <= 0 || dst.h <= 0`. The caller's cue to skip compute_outline_strips() entirely (a
//     SILENT legal no-op, D10's idiom). Does NOT itself check finiteness (is_finite(const
//     RectF&)/std::isfinite(thickness), checked BEFORE this call, D26's literal guard order).
// PT: Guarda D26 -- true quando o contorno é degenerado: `thickness <= 0` ou
//     `dst.w <= 0 || dst.h <= 0`. A deixa do chamador pra pular compute_outline_strips()
//     inteiramente (um no-op legal SILENCIOSO, idioma D10). NÃO checa finitude sozinha
//     (is_finite(const RectF&)/std::isfinite(thickness), checado ANTES desta chamada, ordem de
//     guarda literal do D26).
inline bool is_degenerate_outline(const RectF& dst, float thickness) {
  return thickness <= 0.f || dst.w <= 0.f || dst.h <= 0.f;
}

// EN: D26, literal: `t = min(thickness, min(dst.w, dst.h)/2)` (the collapse clamp), then the 4
//     non-overlapping strips top/bottom/left/right as this file's header comment states
//     verbatim. ASSUMES !is_degenerate_outline(dst, thickness) and that dst/thickness already
//     passed finiteness (the caller's job, same split as compute_line_corners/is_degenerate_line
//     above) -- does not itself guard against either. No special-case branch for the
//     `2*thickness >= min(dst.w, dst.h)` collapse -- the SAME formula with the clamped `t`
//     produces the single-filled-rect degenerate case for free (pinned by this file's own
//     boundary-value test, not asserted by inspection).
// PT: D26, literal: `t = min(thickness, min(dst.w, dst.h)/2)` (o clamp de colapso), depois as 4
//     faixas sem sobreposição topo/base/esquerda/direita como o comentário de cabeçalho deste
//     arquivo declara verbatim. ASSUME !is_degenerate_outline(dst, thickness) e que
//     dst/thickness já passaram por finitude (trabalho do chamador, mesmo split de
//     compute_line_corners/is_degenerate_line acima) -- não guarda sozinha contra nenhum dos
//     dois. Sem ramo de caso especial pro colapso `2*thickness >= min(dst.w, dst.h)` -- a MESMA
//     fórmula com o `t` clampado produz o caso degenerado de retângulo-preenchido-só de graça
//     (fixado pelo próprio teste de valor-de-fronteira deste arquivo, não afirmado por
//     inspeção).
inline OutlineStrips compute_outline_strips(const RectF& dst, float thickness) {
  const float half_min = (dst.w < dst.h ? dst.w : dst.h) * 0.5f;
  const float t = thickness < half_min ? thickness : half_min;
  return OutlineStrips{
      RectF{dst.x, dst.y, dst.w, t},                           // top
      RectF{dst.x, dst.y + dst.h - t, dst.w, t},               // bottom
      RectF{dst.x, dst.y + t, t, dst.h - 2.f * t},             // left
      RectF{dst.x + dst.w - t, dst.y + t, t, dst.h - 2.f * t}, // right
  };
}

// EN: D28, literal: `glScissor(x, target_h - (y + h), w, h)`, with `w`/`h` (the GL "extents")
//     clamped to non-negative BEFORE the int cast (a negative `rect_px.w`/`rect_px.h` -- an
//     already-invalid state by set_scissor()'s own D28 contract, but this function stays total
//     and safe regardless of what the caller hands it). `x`/`y` (the ORIGIN) are cast straight
//     through, un-clamped -- D28's own "clamping is the GPU's job" rule for negative origin.
//     `target_h` is the SAME target_height begin() was opened with; not validated here (the
//     caller's job, this function is a pure mapping, not a guard).
// PT: D28, literal: `glScissor(x, target_h - (y + h), w, h)`, com `w`/`h` (a "extensão" GL)
//     clampados a não-negativo ANTES do cast pra int (um `rect_px.w`/`rect_px.h` negativo -- já
//     um estado inválido pelo próprio contrato D28 do set_scissor(), mas esta função continua
//     total e segura independente do que o chamador entregar). `x`/`y` (a ORIGEM) são
//     convertidos direto, sem clamp -- a própria regra "clampar é trabalho da GPU" do D28 pra
//     origem negativa. `target_h` é o MESMO target_height com que o begin() foi aberto; não
//     validado aqui (trabalho do chamador, esta função é um mapeamento puro, não uma guarda).
inline ScissorGl map_scissor_to_gl(const RectF& rect_px, int target_h) {
  const int gl_x = static_cast<int>(rect_px.x);
  const int gl_y = static_cast<int>(static_cast<float>(target_h) - (rect_px.y + rect_px.h));
  const int gl_w = rect_px.w > 0.f ? static_cast<int>(rect_px.w) : 0;
  const int gl_h = rect_px.h > 0.f ? static_cast<int>(rect_px.h) : 0;
  return ScissorGl{gl_x, gl_y, gl_w, gl_h};
}

} // namespace glintfx::draw2d_detail
