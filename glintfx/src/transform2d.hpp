// SPDX-License-Identifier: MPL-2.0
// EN: D2D-2A -- the PURE projection/transform math behind Draw2D's camera and per-sprite
//     transforms (plan docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md, decisions
//     D12/D14/D16/D17/D18/D20; ADR-0017's Addendum records the leader's Q1=(c)/Q2=(2a) decision
//     this header implements). ZERO GL dependency, same discipline as src/sprite_batch.hpp: this
//     header takes camera/transform values and returns points/corners, headless-testable
//     (tests/transform2d_sanity.cpp), with draw2d.cpp (D2D-2B) owning ONLY the one-line
//     delegation from the PUBLIC glintfx::world_to_screen/screen_to_world/
//     camera_from_world_rect free functions (declared in <glintfx/draw2d.hpp>, defined
//     out-of-line per D21) to the functions in this header -- SINGLE SOURCE for both the draw
//     path and the public helpers, so the two can never diverge (plan section 0, risk 2: "a
//     conversion formula without a computed-value delta test" is this house's named silent-bug
//     class -- it already cost a Critical once). Includes <glintfx/draw2d.hpp> for the public
//     Vec2F/Camera2d/SpriteTransform/RectF types (reusing them here avoids a duplicate POD
//     family, same rationale as sprite_batch.hpp reusing RectF/ColorF).
//
//     TOTALITY CONTRACT (D16, world_to_screen/screen_to_world only): invalid input (any
//     non-finite member of `cam` or the point, `cam.zoom <= 0`, or a non-positive viewport)
//     returns the INPUT POINT UNCHANGED -- deterministic, documented, never NaN out. These two
//     functions cannot log (they are pure); the caller (draw2d.cpp) owns any dedup'd-log
//     decision built on top, same split as this header's is_finite() predicates below, which
//     only ANSWER "is this valid", never decide what to do about it.
//
//     CORNER MATH (D18): compute_sprite_corners() assumes `transform`'s fields are ALREADY
//     validated finite (D20's first guard, "before any arithmetic" -- the caller's job, see
//     is_finite(const SpriteTransform&) below); it does not re-validate, and is not itself total
//     the way world_to_screen/screen_to_world are -- overflow (e.g. an astronomic scale on an
//     astronomic dst) can still produce a non-finite corner from finite inputs, which is exactly
//     D20's SECOND guard (a post-math check with no v0.20.0 twin, by design -- the old
//     axis-aligned path has no multiplication that can overflow). Use
//     is_finite(const SpriteCorners&) after calling this to implement that guard.
// PT: D2D-2A -- a matemática PURA de projeção/transform por trás da câmera e dos transforms
//     por-sprite do Draw2D (plano docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md,
//     decisões D12/D14/D16/D17/D18/D20; o Adendo da ADR-0017 grava a decisão do líder
//     Q1=(c)/Q2=(2a) que este header implementa). ZERO dependência de GL, mesma disciplina de
//     src/sprite_batch.hpp: este header recebe valores de câmera/transform e devolve
//     pontos/cantos, testável headless (tests/transform2d_sanity.cpp), com o draw2d.cpp
//     (D2D-2B) sendo dono SÓ da delegação de uma linha das free functions PÚBLICAS
//     glintfx::world_to_screen/screen_to_world/camera_from_world_rect (declaradas em
//     <glintfx/draw2d.hpp>, definidas fora de linha conforme D21) pras funções deste header --
//     FONTE ÚNICA pro caminho de desenho e pros helpers públicos, então os dois nunca podem
//     divergir (plano seção 0, risco 2: "fórmula de conversão sem teste de valor computado" é a
//     classe de bug silencioso nomeada desta casa -- já custou um Critical uma vez). Inclui
//     <glintfx/draw2d.hpp> pros tipos públicos Vec2F/Camera2d/SpriteTransform/RectF (reusá-los
//     aqui evita uma família de POD duplicada, mesma racional de sprite_batch.hpp reusando
//     RectF/ColorF).
//
//     CONTRATO DE TOTALIDADE (D16, só world_to_screen/screen_to_world): input inválido
//     (qualquer membro não-finito de `cam` ou do ponto, `cam.zoom <= 0`, ou viewport
//     não-positivo) devolve o PONTO DE ENTRADA INALTERADO -- determinístico, documentado, nunca
//     NaN pra fora. Estas duas funções não podem logar (são puras); o chamador (draw2d.cpp) é
//     dono de qualquer decisão de log dedup'd em cima, mesmo split dos predicados is_finite()
//     abaixo, que só RESPONDEM "isto é válido", nunca decidem o que fazer a respeito.
//
//     MATEMÁTICA DE CANTO (D18): compute_sprite_corners() assume que os campos de `transform`
//     JÁ foram validados como finitos (a primeira guarda do D20, "antes de qualquer conta" --
//     trabalho do chamador, ver is_finite(const SpriteTransform&) abaixo); não revalida, e não é
//     total do mesmo jeito que world_to_screen/screen_to_world são -- overflow (ex.: escala
//     astronômica sobre um dst astronômico) ainda pode produzir um canto não-finito a partir de
//     entrada finita, exatamente a SEGUNDA guarda do D20 (um cheque pós-conta sem gêmeo na
//     v0.20.0, por design -- o caminho antigo axis-aligned não tem multiplicação que possa
//     estourar). Use is_finite(const SpriteCorners&) depois de chamar isto pra implementar essa
//     guarda.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <glintfx/draw2d.hpp>

#include <cmath>

namespace glintfx::draw2d_detail {

// EN: The 4 corners of a transformed sprite quad, in the SAME winding order sprite_batch.hpp's
//     emit_quad() already uses (TL,TR,BR,BL, D5) -- draw2d.cpp (D2D-2B) feeds these straight
//     into SpriteBatch::draw_quad() (also D2D-2B), or through world_to_screen() per-corner
//     first when a camera is set (D18's "one composition story, no special cases"). Not part of
//     the public API (not listed in the plan's section 4 literal contract) -- internal to this
//     header and draw2d.cpp.
// PT: Os 4 cantos de um quad de sprite transformado, na MESMA ordem de giro que o emit_quad()
//     de sprite_batch.hpp já usa (TL,TR,BR,BL, D5) -- o draw2d.cpp (D2D-2B) alimenta isto direto
//     no SpriteBatch::draw_quad() (também D2D-2B), ou por world_to_screen() canto-a-canto
//     primeiro quando uma câmera está setada ("uma história de composição só, sem caso
//     especial" do D18). Não é parte da API pública (não listado no contrato literal da seção 4
//     do plano) -- interno a este header e ao draw2d.cpp.
struct SpriteCorners {
  Vec2F tl;
  Vec2F tr;
  Vec2F br;
  Vec2F bl;
};

// EN: Finiteness predicates (D20's "before any arithmetic" guard, and D16's own input check) --
//     pure, cannot log; the caller decides what a `false` means (skip + dedup'd log for
//     draw2d.cpp, a check() failure for the test).
// PT: Predicados de finitude (a guarda "antes de qualquer conta" do D20, e o próprio cheque de
//     entrada do D16) -- puros, não podem logar; o chamador decide o que um `false` significa
//     (pular + log dedup'd no draw2d.cpp, uma falha de check() no teste).
inline bool is_finite(Vec2F v) { return std::isfinite(v.x) && std::isfinite(v.y); }

inline bool is_finite(const RectF& r) {
  return std::isfinite(r.x) && std::isfinite(r.y) && std::isfinite(r.w) && std::isfinite(r.h);
}

inline bool is_finite(const Camera2d& cam) {
  return std::isfinite(cam.x) && std::isfinite(cam.y) && std::isfinite(cam.zoom) &&
         std::isfinite(cam.rotation);
}

inline bool is_finite(const SpriteTransform& t) {
  return std::isfinite(t.origin_x) && std::isfinite(t.origin_y) && std::isfinite(t.rotation) &&
         std::isfinite(t.scale_x) && std::isfinite(t.scale_y);
}

inline bool is_finite(const SpriteCorners& c) {
  return is_finite(c.tl) && is_finite(c.tr) && is_finite(c.br) && is_finite(c.bl);
}

// EN: D12's rot(v, t) = (v.x*cos(t) - v.y*sin(t), v.x*sin(t) + v.y*cos(t)), literal -- shared by
//     world_to_screen/screen_to_world (camera rotation) AND compute_sprite_corners (per-sprite
//     rotation), the "implemented ONCE" the plan demands.
// PT: O rot(v, t) = (v.x*cos(t) - v.y*sin(t), v.x*sin(t) + v.y*cos(t)) do D12, literal --
//     compartilhado por world_to_screen/screen_to_world (rotação de câmera) E
//     compute_sprite_corners (rotação por-sprite), o "implementado UMA vez" que o plano exige.
inline Vec2F rotate(Vec2F v, float t) {
  const float c = std::cos(t);
  const float s = std::sin(t);
  return Vec2F{v.x * c - v.y * s, v.x * s + v.y * c};
}

// EN: D12, literal: world_to_screen(cam, W, H, p) = rot((p - (cam.x,cam.y)) * cam.zoom,
//     -cam.rotation) + (W/2, H/2). D16 totality: on invalid input (non-finite `cam`/`world`,
//     `cam.zoom <= 0`, non-positive viewport) returns `world` UNCHANGED, never NaN out.
// PT: D12, literal: world_to_screen(cam, W, H, p) = rot((p - (cam.x,cam.y)) * cam.zoom,
//     -cam.rotation) + (W/2, H/2). Totalidade D16: em input inválido (`cam`/`world` não-finito,
//     `cam.zoom <= 0`, viewport não-positivo) devolve `world` INALTERADO, nunca NaN pra fora.
inline Vec2F world_to_screen(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F world) {
  if (!is_finite(cam) || !is_finite(world) || cam.zoom <= 0.f || viewport_w <= 0 ||
      viewport_h <= 0)
    return world;
  const Vec2F centered{(world.x - cam.x) * cam.zoom, (world.y - cam.y) * cam.zoom};
  const Vec2F rotated = rotate(centered, -cam.rotation);
  return Vec2F{rotated.x + static_cast<float>(viewport_w) * 0.5f,
               rotated.y + static_cast<float>(viewport_h) * 0.5f};
}

// EN: D12, literal: screen_to_world(cam, W, H, s) = rot(s - (W/2,H/2), +cam.rotation) /
//     cam.zoom + (cam.x,cam.y) -- the exact inverse of world_to_screen (rot(rot(v,-t),t) == v,
//     pinned by the round-trip property test). D16 totality: same invalid-input set as
//     world_to_screen, returns `screen` UNCHANGED.
// PT: D12, literal: screen_to_world(cam, W, H, s) = rot(s - (W/2,H/2), +cam.rotation) /
//     cam.zoom + (cam.x,cam.y) -- o inverso exato de world_to_screen (rot(rot(v,-t),t) == v,
//     fixado pelo teste de propriedade de round-trip). Totalidade D16: mesmo conjunto de input
//     inválido de world_to_screen, devolve `screen` INALTERADO.
inline Vec2F screen_to_world(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F screen) {
  if (!is_finite(cam) || !is_finite(screen) || cam.zoom <= 0.f || viewport_w <= 0 ||
      viewport_h <= 0)
    return screen;
  const Vec2F centered{screen.x - static_cast<float>(viewport_w) * 0.5f,
                       screen.y - static_cast<float>(viewport_h) * 0.5f};
  const Vec2F rotated = rotate(centered, cam.rotation);
  return Vec2F{rotated.x / cam.zoom + cam.x, rotated.y / cam.zoom + cam.y};
}

// EN: D17, literal: fits `world_rect` by WIDTH (`zoom = viewport_w / world_rect.w`, center =
//     rect center); vertical coverage follows the viewport's own aspect (letterbox-exact fit is
//     the caller's job, composing with UiLayer::set_viewport -- documented, not this function's
//     problem). Fail-high: non-finite `world_rect`, `world_rect.w <= 0 || world_rect.h <= 0`, or
//     a non-positive viewport return `Camera2d{}` (documented).
// PT: D17, literal: ajusta `world_rect` por LARGURA (`zoom = viewport_w / world_rect.w`, centro
//     = centro do retângulo); a cobertura vertical segue o próprio aspecto do viewport (fit
//     letterbox-exato é trabalho do chamador, compondo com UiLayer::set_viewport --
//     documentado, não é problema desta função). Fail-high: `world_rect` não-finito,
//     `world_rect.w <= 0 || world_rect.h <= 0`, ou viewport não-positivo devolvem `Camera2d{}`
//     (documentado).
inline Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h) {
  if (!is_finite(world_rect) || world_rect.w <= 0.f || world_rect.h <= 0.f || viewport_w <= 0 ||
      viewport_h <= 0)
    return Camera2d{};
  Camera2d cam{};
  cam.zoom = static_cast<float>(viewport_w) / world_rect.w;
  cam.x = world_rect.x + world_rect.w * 0.5f;
  cam.y = world_rect.y + world_rect.h * 0.5f;
  cam.rotation = 0.f;
  return cam;
}

// EN: D18, literal: pivot P = (dst.x + origin_x*dst.w, dst.y + origin_y*dst.h); for each of
//     TL,TR,BR,BL: corner' = P + rot((corner - P) * (scale_x, scale_y), rotation) -- scale
//     about the pivot, THEN rotate about the pivot, in that literal order (the mutation the
//     adversarial reviewer is briefed to try). An identity `transform` (default-constructed
//     SpriteTransform{}: origin 0.5/0.5, rotation 0, scale 1/1) reproduces `dst`'s own 4 corners
//     bit-for-bit -- `*1.0f`, `+0.0f`, `cos(0)=1`, `sin(0)=0` are IEEE-exact, the half of D19's
//     equivalence pin that belongs to this slice (the rest -- the batcher's v0.20.0 path
//     staying byte-identical -- is D2D-2B's). ASSUMES `transform` already passed is_finite()
//     (D20's first guard, the caller's job) -- does not itself guard against overflow producing
//     a non-finite corner from finite input (D20's SECOND guard; call
//     is_finite(const SpriteCorners&) on the result to implement it).
// PT: D18, literal: pivô P = (dst.x + origin_x*dst.w, dst.y + origin_y*dst.h); para cada um de
//     TL,TR,BR,BL: canto' = P + rot((canto - P) * (scale_x, scale_y), rotation) -- escala sobre
//     o pivô, DEPOIS rotação sobre o pivô, nessa ordem literal (a mutação que o reviewer
//     adversarial está orientado a tentar). Um `transform` identidade (SpriteTransform{}
//     default-construído: origem 0.5/0.5, rotação 0, escala 1/1) reproduz os 4 cantos do
//     próprio `dst` bit-a-bit -- `*1.0f`, `+0.0f`, `cos(0)=1`, `sin(0)=0` são IEEE-exatos, a
//     metade da fixação de equivalência do D19 que pertence a esta fatia (o resto -- o caminho
//     v0.20.0 do batcher continuar byte-idêntico -- é do D2D-2B). ASSUME que `transform` já
//     passou por is_finite() (a primeira guarda do D20, trabalho do chamador) -- não guarda
//     sozinha contra overflow produzindo um canto não-finito a partir de entrada finita (a
//     SEGUNDA guarda do D20; chame is_finite(const SpriteCorners&) no resultado pra
//     implementá-la).
inline SpriteCorners compute_sprite_corners(const RectF& dst, const SpriteTransform& transform) {
  const Vec2F pivot{dst.x + transform.origin_x * dst.w, dst.y + transform.origin_y * dst.h};
  const Vec2F tl{dst.x, dst.y};
  const Vec2F tr{dst.x + dst.w, dst.y};
  const Vec2F br{dst.x + dst.w, dst.y + dst.h};
  const Vec2F bl{dst.x, dst.y + dst.h};

  const auto apply = [&](Vec2F corner) -> Vec2F {
    const Vec2F local{(corner.x - pivot.x) * transform.scale_x,
                      (corner.y - pivot.y) * transform.scale_y};
    const Vec2F rotated = rotate(local, transform.rotation);
    return Vec2F{rotated.x + pivot.x, rotated.y + pivot.y};
  };

  return SpriteCorners{apply(tl), apply(tr), apply(br), apply(bl)};
}

} // namespace glintfx::draw2d_detail
