// SPDX-License-Identifier: MPL-2.0
// EN: text_raster.hpp's implementation. See that header's own file comment for the full
//     clean-room/scope writeup -- this file only adds the per-function derivations not already
//     covered there. The bake math (kPad border, scale_num/scale_den, origin_x/y_26_6, the
//     vsnap-only-not-pen-snap split) mirrors the proven, reviewed pattern
//     src/font_engine_own.cpp's BakeFaceInstance()/DeriveHintZones() already established in
//     production -- re-derived here, not copy-pasted (this seam's own vsnap-only contract is
//     narrower: no pen-snap X, no per-glyph A/B hint-bypass hooks, no COLR/A4-EMOJI path, see
//     text_raster.hpp's top comment for why).
// PT: Implementação do text_raster.hpp. Ver o comentário de arquivo próprio daquele header pro
//     relato completo de clean-room/escopo -- este arquivo só acrescenta as derivações
//     por-função ainda não cobertas lá. A matemática de bake (borda kPad, scale_num/scale_den,
//     origin_x/y_26_6, a divisão só-vsnap-sem-pen-snap) espelha o padrão provado e revisado que
//     o BakeFaceInstance()/DeriveHintZones() do src/font_engine_own.cpp já estabeleceu em
//     produção -- re-derivada aqui, não copiada-e-colada (o contrato só-vsnap desta costura é
//     mais estreito: sem pen-snap X, sem os hooks de A/B hint-bypass por-glifo, sem caminho
//     COLR/A4-EMOJI, ver o comentário de topo do text_raster.hpp pro porquê).
// Copyright (c) 2026 Petrus Silva Costa
#include "text_raster.hpp"

#include <cmath>

namespace glintfx {

namespace {

// EN: Fixed caps for glx_sfnt_glyph_outline()'s caller-owned buffers -- same magnitude as
//     src/font_engine_own.cpp's own kMaxOutlinePoints/kMaxOutlineContours (Open Sans' letter
//     glyphs run well under 100 points/10 contours; pt-br accented composites, one level deep,
//     fit comfortably). A glyph that legitimately needs more (GLX_SFNT_ERR_BUFFER_TOO_SMALL)
//     degrades to "no bitmap baked" (see compute_bake_geometry() below) rather than growing the
//     buffer dynamically.
// PT: Tetos fixos pros buffers possuídos-por-quem-chama do glx_sfnt_glyph_outline() -- mesma
//     magnitude do kMaxOutlinePoints/kMaxOutlineContours do próprio src/font_engine_own.cpp (os
//     glyphs de letra da Open Sans ficam bem abaixo de 100 pontos/10 contornos; compostos
//     acentuados pt-br, um nível de profundidade, cabem confortavelmente). Um glifo que
//     legitimamente precisa de mais (GLX_SFNT_ERR_BUFFER_TOO_SMALL) degrada pra "nenhum bitmap
//     assado" (ver compute_bake_geometry() abaixo) em vez do buffer crescer dinamicamente.
constexpr uint16_t kMaxOutlinePoints = 768;
constexpr uint16_t kMaxOutlineContours = 48;

// EN: 1px transparent border baked into every non-empty glyph bitmap -- avoids AA bleed between
//     neighbouring cells once the caller (Draw2d's atlas, T2) packs this bitmap into a shared
//     texture. Same constant/reasoning src/font_engine_own.cpp's own BakeFaceInstance() already
//     uses (`constexpr int kPad = 1`).
// PT: Borda transparente de 1px assada em todo bitmap de glifo não-vazio -- evita sangramento de
//     AA entre células vizinhas quando quem chama (o atlas do Draw2d, T2) empacota este bitmap
//     numa textura compartilhada. Mesma constante/racional que o próprio BakeFaceInstance() do
//     src/font_engine_own.cpp já usa (`constexpr int kPad = 1`).
constexpr int kPad = 1;

// EN: `y_max` (font units, straight from the glyf record header) of the glyph `cp` maps to via
//     `sf`'s cmap, used ONLY to MEASURE this face's x-height/cap-height zones for vsnap (see
//     derive_hint_zones() below). `*ok = false` (return value 0) for an unmapped codepoint, a
//     glx_sfnt_glyph_outline() failure, or an empty outline -- the caller falls back to a
//     fraction-of-ascender estimate, never crashes on a face missing these glyphs. Verbatim
//     re-derivation of src/font_engine_own.cpp's own GlyphYMax() (same core calls, same fixed
//     caps, same degrade contract).
// PT: `y_max` (unidade de fonte, direto do cabeçalho do registro glyf) do glifo pro qual `cp`
//     mapeia via o cmap de `sf`, usado SÓ pra MEDIR as zonas de altura-x/altura-de-maiúscula
//     desta face pro vsnap (ver derive_hint_zones() abaixo). `*ok = false` (retorno 0) pra um
//     codepoint não-mapeado, uma falha do glx_sfnt_glyph_outline(), ou um outline vazio -- quem
//     chama cai pra uma estimativa de fração-do-ascender, nunca crasha numa face sem esses
//     glifos. Re-derivação literal do próprio GlyphYMax() do src/font_engine_own.cpp (mesmas
//     chamadas de core, mesmos tetos fixos, mesmo contrato de degradação).
int16_t glyph_y_max(const glx_sfnt_face& sf, uint32_t cp, bool& ok) {
  ok = false;
  const uint32_t gid = glx_sfnt_glyph_id(&sf, cp);
  if (gid == 0) return 0;
  glx_sfnt_point points[kMaxOutlinePoints];
  uint16_t contour_ends[kMaxOutlineContours];
  glx_sfnt_outline o{};
  if (glx_sfnt_glyph_outline(&sf, gid, points, kMaxOutlinePoints, contour_ends,
                             kMaxOutlineContours, &o) != GLX_SFNT_OK)
    return 0;
  if (o.num_points == 0 || o.num_contours == 0) return 0;
  ok = true;
  return o.y_max;
}

// EN: Derives this face's vertical grid-fitting zones ONCE (called from TextFace::open()).
//     Verbatim re-derivation of src/font_engine_own.cpp's own DeriveHintZones() -- see that
//     function's doc-comment for the full rationale (baseline=0, ascender/descender from hhea,
//     x_height/cap_height MEASURED from 'x'/'H' with an 'o'/'O' then fraction-of-ascender
//     fallback chain, never crashes on a face missing every one of those glyphs).
// PT: Deriva UMA vez as zonas de grid-fitting vertical desta face (chamada do TextFace::open()).
//     Re-derivação literal do próprio DeriveHintZones() do src/font_engine_own.cpp -- ver o
//     doc-comment daquela função pro racional completo (baseline=0, ascender/descender do hhea,
//     x_height/cap_height MEDIDOS de 'x'/'H' com cadeia de fallback 'o'/'O' depois
//     fração-do-ascender, nunca crasha numa face sem nenhum desses glifos).
glx_hint_zones derive_hint_zones(const glx_sfnt_face& sf) {
  glx_hint_zones z{};
  z.baseline = 0;
  z.ascender = sf.ascender;
  z.descender = sf.descender;

  bool ok = false;
  int16_t v = glyph_y_max(sf, 0x78, ok);  // 'x'
  if (!ok) v = glyph_y_max(sf, 0x6F, ok); // 'o'
  z.x_height = ok ? v : (int16_t)((int)sf.ascender / 2);

  ok = false;
  v = glyph_y_max(sf, 0x48, ok);          // 'H'
  if (!ok) v = glyph_y_max(sf, 0x4F, ok); // 'O'
  z.cap_height = ok ? v : (int16_t)((int)sf.ascender * 72 / 100);

  z.axis_flags = GLX_HINT_AXIS_Y;
  return z;
}

// EN: Recomputed geometry shared by plan_glyph_bake()/bake_glyph_into() (below) so the two can
//     never silently disagree (single source, the same TX17-class discipline the plan's own
//     hand-computed-value gate applies to text_layout.hpp's wrap/align math). `has_outline ==
//     false` (w==h==0) is the CORRECT, non-error answer for a glyph with no ink (space, an empty
//     `.notdef`, or an outline this seam's fixed point/contour buffers are too small for) -- see
//     text_raster.hpp's own GlyphBakePlan doc-comment.
// PT: Geometria recomputada compartilhada pelo plan_glyph_bake()/bake_glyph_into() (abaixo) pra
//     que os dois nunca possam discordar em silêncio (fonte única, a mesma disciplina classe-
//     TX17 que o próprio gate de valor-à-mão do plano aplica à matemática de wrap/align do
//     text_layout.hpp). `has_outline == false` (w==h==0) é a resposta CORRETA, sem erro, pra um
//     glifo sem tinta (espaço, um `.notdef` vazio, ou um outline pro qual os buffers fixos de
//     ponto/contorno desta costura são pequenos demais) -- ver o doc-comment próprio de
//     GlyphBakePlan em text_raster.hpp.
struct BakeGeometry {
  bool ok = false; // false only for a hard precondition failure (see callers).
  bool has_outline = false;
  int w = 0;
  int h = 0;
  float bearing_x = 0.f;
  float bearing_y = 0.f;
  int32_t scale_num = 0;
  int32_t scale_den = 0;
  int32_t origin_x_26_6 = 0;
  int32_t origin_y_26_6 = 0;
  uint16_t num_points = 0;
  uint16_t num_contours = 0;
};

// EN: `points_out`/`contour_ends_out` are the CALLER's own kMaxOutlinePoints/kMaxOutlineContours-
//     sized scratch arrays (bake_glyph_into() needs the actual points AFTER vsnap to rasterize;
//     plan_glyph_bake() discards them) -- passed in rather than allocated here so this function
//     itself never allocates.
// PT: `points_out`/`contour_ends_out` são os próprios arrays de scratch de tamanho
//     kMaxOutlinePoints/kMaxOutlineContours de quem chama (o bake_glyph_into() precisa dos
//     pontos DEPOIS do vsnap pra rasterizar; o plan_glyph_bake() os descarta) -- passados em vez
//     de alocados aqui, então esta função em si nunca aloca.
bool compute_bake_geometry(const TextFace& face, uint32_t gid, float size_px, bool vsnap,
                           glx_sfnt_point* points_out, uint16_t* contour_ends_out,
                           BakeGeometry& out) {
  out = BakeGeometry{};
  const glx_sfnt_face* sf = face.raw();
  if (sf == nullptr) return false;                             // not-ok() face (TX6).
  if (gid >= sf->num_glyphs) return false;                     // out-of-range gid (TX6).
  if (!std::isfinite(size_px) || size_px <= 0.f) return false; // TX6, before any arithmetic.
  if (sf->units_per_em == 0) return false;                     // defense in depth, mirrors glyph_advance_px().

  out.ok = true;

  glx_sfnt_outline outline{};
  const glx_sfnt_result r = glx_sfnt_glyph_outline(sf, gid, points_out, kMaxOutlinePoints,
                                                   contour_ends_out, kMaxOutlineContours,
                                                   &outline);
  // EN: r != GLX_SFNT_OK (e.g. GLX_SFNT_ERR_BUFFER_TOO_SMALL for an outline too complex for this
  //     seam's fixed caps) or an empty outline (space, num_points==0) both leave `has_outline ==
  //     false` -- a valid "no bitmap baked" glyph, not a hard failure (see this struct's own
  //     doc-comment above).
  // PT: r != GLX_SFNT_OK (ex.: GLX_SFNT_ERR_BUFFER_TOO_SMALL pra um outline complexo demais pros
  //     tetos fixos desta costura) ou um outline vazio (espaço, num_points==0) ambos deixam
  //     `has_outline == false` -- um glifo "nenhum bitmap assado" válido, não uma falha dura (ver
  //     o doc-comment próprio desta struct acima).
  if (r != GLX_SFNT_OK || outline.num_points == 0 || outline.num_contours == 0) return true;

  const int32_t scale_num = (int32_t)std::lround((double)size_px * 64.0);
  const int32_t scale_den = (int32_t)sf->units_per_em;
  const float scale = size_px / (float)sf->units_per_em;

  if (vsnap) {
    const int32_t hint_scale_num = scale_num;
    const glx_hint_zones zones = derive_hint_zones(*sf);
    // EN: GLX_HINT_NOOP (nothing to fit / already fitted) leaves `points_out` byte-for-byte
    //     unchanged -- a clean, non-error outcome neither checked nor needed here, same posture
    //     src/font_engine_own.cpp's own BakeFaceInstance() already takes.
    // PT: GLX_HINT_NOOP (nada a fitar / já fitado) deixa `points_out` byte-a-byte intacto -- um
    //     resultado limpo, sem erro, nem checado nem necessário aqui, mesma postura que o
    //     próprio BakeFaceInstance() do src/font_engine_own.cpp já assume.
    glx_hint_outline(points_out, outline.num_points, sf->units_per_em, hint_scale_num,
                     (int32_t)sf->units_per_em, &zones);
  }

  const float fx_min = (float)outline.x_min * scale;
  const float fx_max = (float)outline.x_max * scale;
  const float fy_min = (float)outline.y_min * scale;
  const float fy_max = (float)outline.y_max * scale;

  const int gw = (int)std::ceil(fx_max - fx_min) + kPad * 2;
  const int gh = (int)std::ceil(fy_max - fy_min) + kPad * 2;

  // EN: TX6's own per-glyph bitmap cap (kMaxGlyphBitmapDim, text_raster.hpp) -- a
  //     hostile/degenerate bbox does NOT get a bitmap, it degrades to "no bitmap baked" exactly
  //     like a too-complex outline above (never a hard failure the caller has to special-case
  //     differently).
  // PT: O próprio teto por-glifo de bitmap do TX6 (kMaxGlyphBitmapDim, text_raster.hpp) -- uma
  //     bbox hostil/degenerada NÃO ganha bitmap, degrada pra "nenhum bitmap assado" exatamente
  //     como um outline complexo demais acima (nunca uma falha dura que quem chama precise
  //     tratar diferente).
  if (gw <= 0 || gh <= 0 || gw > kMaxGlyphBitmapDim || gh > kMaxGlyphBitmapDim) return true;

  out.has_outline = true;
  out.w = gw;
  out.h = gh;
  out.num_points = outline.num_points;
  out.num_contours = outline.num_contours;
  out.scale_num = scale_num;
  out.scale_den = scale_den;
  out.origin_x_26_6 = (int32_t)std::lround(((double)-fx_min + kPad) * 64.0);
  // EN: vsnap: place the glyph's y_max on a WHOLE bitmap row -- rounding fy_max FIRST, then
  //     scaling by 64 (multiply, not lround-of-a-product), forces origin_y_26_6 to an exact
  //     multiple of 64, same reasoning src/font_engine_own.cpp's own BakeFaceInstance() already
  //     documents for its own origin_y_26_6.
  // PT: vsnap: põe o y_max do glifo numa row INTEIRA do bitmap -- arredondar fy_max PRIMEIRO,
  //     depois escalar por 64 (multiplicar, não lround-de-um-produto), força o origin_y_26_6 a
  //     um múltiplo exato de 64, mesmo racional que o próprio BakeFaceInstance() do
  //     src/font_engine_own.cpp já documenta pro próprio origin_y_26_6.
  out.origin_y_26_6 = vsnap ? ((int32_t)std::lround((double)fy_max) + kPad) * 64
                            : (int32_t)std::lround(((double)fy_max + kPad) * 64.0);
  out.bearing_x = fx_min - (float)kPad;
  out.bearing_y = -(fy_max + (float)kPad);
  return true;
}

} // namespace

Utf8Decoded decode_utf8_codepoint(const char* utf8, std::size_t len) {
  Utf8Decoded r;
  if (utf8 == nullptr || len == 0) return r; // {0xFFFD, 0} -- pure defensive floor (TX6).

  const auto byte_at = [&](std::size_t i) -> unsigned char { return (unsigned char)utf8[i]; };
  const unsigned char b0 = byte_at(0);

  // EN: 1-byte ASCII -- the overwhelmingly common case, checked first.
  // PT: 1 byte ASCII -- o caso de longe mais comum, checado primeiro.
  if (b0 < 0x80u) {
    r.codepoint = b0;
    r.consumed = 1;
    return r;
  }

  // EN: A stray continuation byte (0x80..0xBF) with no lead byte before it, or an invalid lead
  //     byte (0xF8..0xFF, 5/6-byte sequences and the two never-valid bytes 0xC0/0xC1 fold into
  //     this same overlong-2-byte-rejection path below, not here) -- always U+FFFD, consumed=1.
  // PT: Um byte de continuação (0x80..0xBF) solto sem lead byte antes, ou um lead byte inválido
  //     (0xF8..0xFF, sequências de 5/6 byte e os dois bytes nunca-válidos 0xC0/0xC1 dobram no
  //     mesmo caminho de rejeição-overlong-de-2-byte abaixo, não aqui) -- sempre U+FFFD,
  //     consumed=1.
  if (b0 < 0xC2u || b0 >= 0xF8u) {
    r.codepoint = 0xFFFDu;
    r.consumed = 1;
    return r;
  }

  // EN: `n` = total sequence length (2/3/4), `min_cp` = the smallest codepoint that sequence
  //     length is allowed to encode (rejects overlong encodings), `cp` accumulated from `b0`'s
  //     own payload bits.
  // PT: `n` = comprimento total da sequência (2/3/4), `min_cp` = o menor codepoint que aquele
  //     comprimento de sequência tem permissão de codificar (rejeita codificações overlong),
  //     `cp` acumulado a partir dos próprios bits de payload de `b0`.
  int n;
  uint32_t cp;
  uint32_t min_cp;
  if (b0 < 0xE0u) {
    n = 2;
    cp = b0 & 0x1Fu;
    min_cp = 0x80u;
  } else if (b0 < 0xF0u) {
    n = 3;
    cp = b0 & 0x0Fu;
    min_cp = 0x800u;
  } else {
    n = 4;
    cp = b0 & 0x07u;
    min_cp = 0x10000u;
  }

  // EN: Consume continuation bytes one at a time, stopping (as a "maximal subpart" U+FFFD) at
  //     the first of: running out of input (`i >= len`), or a byte that is not a continuation
  //     byte (`(b & 0xC0) != 0x80`) -- the byte that broke the sequence is NOT consumed here, so
  //     the next decode_utf8_codepoint() call re-examines it fresh (never silently swallowed).
  // PT: Consome bytes de continuação um de cada vez, parando (como U+FFFD "subparte máxima") no
  //     primeiro de: ficar sem input (`i >= len`), ou um byte que não é byte de continuação
  //     (`(b & 0xC0) != 0x80`) -- o byte que quebrou a sequência NÃO é consumido aqui, então a
  //     próxima chamada ao decode_utf8_codepoint() o reexamina do zero (nunca engolido em
  //     silêncio).
  std::size_t i = 1;
  for (; i < (std::size_t)n; ++i) {
    if (i >= len) {
      r.codepoint = 0xFFFDu;
      r.consumed = i; // >= 1: only the lead byte + whatever valid continuations preceded EOF.
      return r;
    }
    const unsigned char b = byte_at(i);
    if ((b & 0xC0u) != 0x80u) {
      r.codepoint = 0xFFFDu;
      r.consumed = i; // the non-continuation byte itself is left unconsumed.
      return r;
    }
    cp = (cp << 6) | (b & 0x3Fu);
  }

  // EN: Final validity gates, ALL checked before accepting `cp`: overlong (cp < min_cp), a UTF-16
  //     surrogate (U+D800..U+DFFF -- illegal in UTF-8 by definition, the specific hostile shape a
  //     naive decoder mishandles), and above the Unicode maximum (U+10FFFF).
  // PT: Guardas finais de validade, TODAS checadas antes de aceitar `cp`: overlong (cp <
  //     min_cp), um surrogate UTF-16 (U+D800..U+DFFF -- ilegal em UTF-8 por definição, o formato
  //     hostil específico que um decoder ingênuo maltrata), e acima do máximo Unicode
  //     (U+10FFFF).
  if (cp < min_cp || (cp >= 0xD800u && cp <= 0xDFFFu) || cp > 0x10FFFFu) {
    r.codepoint = 0xFFFDu;
    r.consumed = 1; // reject the WHOLE sequence's minimal-forward-progress unit: just the lead byte.
    return r;
  }

  r.codepoint = cp;
  r.consumed = (std::size_t)n;
  return r;
}

TextFace TextFace::open(const uint8_t* blob, std::size_t len) {
  TextFace face;
  if (blob == nullptr || len == 0 || len > kMaxFontBlobBytes) return face; // ok_ stays false.

  face.blob_.assign(blob, blob + len);
  if (glx_sfnt_open(face.blob_.data(), face.blob_.size(), &face.face_) != GLX_SFNT_OK) {
    face.blob_.clear();
    face.blob_.shrink_to_fit();
    return TextFace{}; // ok_ stays false -- discard the partially-built state entirely.
  }

  face.hint_zones_ = derive_hint_zones(face.face_);
  face.ok_ = true;
  return face;
}

uint32_t TextFace::glyph_id(uint32_t codepoint) const {
  if (!ok_) return 0;
  return glx_sfnt_glyph_id(&face_, codepoint);
}

float TextFace::glyph_advance_px(uint32_t gid, float size_px) const {
  if (!ok_ || !std::isfinite(size_px) || size_px <= 0.f) return 0.f;
  if (face_.units_per_em == 0) return 0.f;
  uint16_t advance_units = 0;
  int16_t lsb = 0;
  if (glx_sfnt_hmetrics(&face_, gid, &advance_units, &lsb) != GLX_SFNT_OK) return 0.f;
  return (float)advance_units * (size_px / (float)face_.units_per_em);
}

float TextFace::kern_px(uint32_t left_gid, uint32_t right_gid, float size_px) const {
  if (!ok_ || !std::isfinite(size_px) || size_px <= 0.f) return 0.f;
  if (face_.units_per_em == 0) return 0.f;
  const int16_t k = glx_sfnt_kern(&face_, left_gid, right_gid);
  return (float)k * (size_px / (float)face_.units_per_em);
}

TextFace::Metrics TextFace::metrics_px(float size_px) const {
  Metrics m;
  if (!ok_ || !std::isfinite(size_px) || size_px <= 0.f) return m;
  if (face_.units_per_em == 0) return m;
  const float scale = size_px / (float)face_.units_per_em;
  m.ascent = (float)face_.ascender * scale;
  // EN: hhea's own `descender` is typically NEGATIVE (below the baseline, include/core/sfnt.h's
  //     own doc-comment) -- sign-flipped to a positive magnitude here, see this method's own
  //     declaration doc-comment in the header for why.
  // PT: o próprio `descender` do hhea é tipicamente NEGATIVO (abaixo da baseline, doc-comment
  //     próprio do include/core/sfnt.h) -- sinal invertido pra uma magnitude positiva aqui, ver
  //     o doc-comment próprio de declaração deste método no header pro porquê.
  m.descent = -(float)face_.descender * scale;
  m.line_gap = (float)face_.line_gap * scale;
  m.line_height = m.ascent + m.descent + m.line_gap;
  return m;
}

namespace {

// EN: The 4 GlyphAdvanceSource trampolines -- each casts `ctx` back to `const AdvanceContext*`
//     and delegates to the matching TextFace method at that context's own `size_px`. Never
//     dereferences a null `ctx` (make_advance_source() below only ever binds these to a
//     validated, non-null AdvanceContext -- see that function's own doc-comment).
// PT: Os 4 trampolins do GlyphAdvanceSource -- cada um faz cast de `ctx` de volta pra `const
//     AdvanceContext*` e delega pro método TextFace correspondente no `size_px` próprio daquele
//     contexto. Nunca desreferencia um `ctx` nulo (o make_advance_source() abaixo só amarra
//     estes a um AdvanceContext validado, não-nulo -- ver o doc-comment próprio daquela função).
uint32_t advance_source_glyph_id(const void* ctx, uint32_t codepoint) {
  const auto* c = static_cast<const AdvanceContext*>(ctx);
  return c->face->glyph_id(codepoint);
}

float advance_source_advance_px(const void* ctx, uint32_t gid) {
  const auto* c = static_cast<const AdvanceContext*>(ctx);
  return c->face->glyph_advance_px(gid, c->size_px);
}

float advance_source_kern_px(const void* ctx, uint32_t left_gid, uint32_t right_gid) {
  const auto* c = static_cast<const AdvanceContext*>(ctx);
  return c->face->kern_px(left_gid, right_gid, c->size_px);
}

float advance_source_ascent_px(const void* ctx) {
  const auto* c = static_cast<const AdvanceContext*>(ctx);
  return c->face->metrics_px(c->size_px).ascent;
}

float advance_source_line_height_px(const void* ctx) {
  const auto* c = static_cast<const AdvanceContext*>(ctx);
  return c->face->metrics_px(c->size_px).line_height;
}

} // namespace

GlyphAdvanceSource make_advance_source(const AdvanceContext& ctx) {
  GlyphAdvanceSource src;
  if (ctx.face == nullptr || !ctx.face->ok() || !std::isfinite(ctx.size_px) || ctx.size_px <= 0.f)
    return src; // ok() stays false -- every function pointer left null (TX6).

  src.ctx = &ctx;
  src.glyph_id = &advance_source_glyph_id;
  src.advance_px = &advance_source_advance_px;
  src.kern_px = &advance_source_kern_px;
  src.ascent_px = &advance_source_ascent_px;
  src.line_height_px = &advance_source_line_height_px;
  return src;
}

GlyphBakePlan plan_glyph_bake(const TextFace& face, uint32_t gid, float size_px, bool vsnap) {
  GlyphBakePlan plan;
  glx_sfnt_point points[kMaxOutlinePoints];
  uint16_t contour_ends[kMaxOutlineContours];
  BakeGeometry geo;
  if (!compute_bake_geometry(face, gid, size_px, vsnap, points, contour_ends, geo)) return plan;

  plan.ok = true;
  if (!geo.has_outline) return plan; // valid "no ink" glyph -- bitmap_w/h stay 0.

  plan.bitmap_w = geo.w;
  plan.bitmap_h = geo.h;
  plan.bearing_x = geo.bearing_x;
  plan.bearing_y = geo.bearing_y;
  return plan;
}

std::size_t glyph_bake_scratch_floats(int w, int h) { return glx_raster_scratch_floats(w, h); }

bool bake_glyph_into(const TextFace& face, uint32_t gid, float size_px, bool vsnap, int expect_w,
                     int expect_h, uint8_t* bitmap_out, int stride, float* scratch,
                     std::size_t scratch_capacity_floats) {
  glx_sfnt_point points[kMaxOutlinePoints];
  uint16_t contour_ends[kMaxOutlineContours];
  BakeGeometry geo;
  if (!compute_bake_geometry(face, gid, size_px, vsnap, points, contour_ends, geo)) return false;

  if (!geo.has_outline) {
    // EN: A valid "no ink" glyph -- true only if the caller's own plan agrees there is nothing
    //     to bake (stale-plan guard, see this function's own doc-comment in the header).
    // PT: Um glifo "sem tinta" válido -- true só se o próprio plano de quem chama concorda que
    //     não há nada a assar (guarda de plano obsoleto, ver o doc-comment próprio desta função
    //     no header).
    return expect_w == 0 && expect_h == 0;
  }
  if (geo.w != expect_w || geo.h != expect_h) return false; // stale/mismatched plan.
  if (bitmap_out == nullptr || stride < geo.w) return false;
  if (scratch == nullptr || scratch_capacity_floats < glx_raster_scratch_floats(geo.w, geo.h))
    return false;

  const glx_raster_result rr =
      glx_rasterize_outline(points, geo.num_points, contour_ends, geo.num_contours, geo.scale_num,
                            geo.scale_den, geo.origin_x_26_6, geo.origin_y_26_6, bitmap_out,
                            geo.w, geo.h, stride, scratch, scratch_capacity_floats);
  return rr == GLX_RASTER_OK;
}

} // namespace glintfx
