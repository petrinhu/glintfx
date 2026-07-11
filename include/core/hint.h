// SPDX-License-Identifier: MPL-2.0
// EN: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-phase 1.1) -- a clean-room, INTEGER-ONLY VERTICAL
//     (Y-axis) GRID-FITTER for TrueType/SFNT outlines. Given a point stream in FONT UNITS (the
//     exact `glx_sfnt_point` shape SOV-SFNT produces), a device scale (the same rational
//     `(scale_num, scale_den)` 26.6 mapping `glx_rasterize_outline` consumes), and a small POD of
//     KEY HORIZONTAL ZONES in font units (baseline / x-height / cap-height / ascender / descender),
//     it nudges each point's Y so that the zone features land on WHOLE PIXEL boundaries in device
//     space -- the step that makes small text's baseline and x-height "snap" to the pixel grid,
//     turning fuzzy horizontal edges crisp.
//
//     WHAT THIS IS / IS NOT: this is sub-phase 1.1 ONLY -- Y-axis zone grid-fitting (highest ROI,
//     lowest risk). It does NOT do vertical STEM detection/snapping (sub-phase 1.2, a future
//     decision), NOR any X-axis work, NOR the C++ shell wiring (sub-phases 1.3/1.4). It is a PURE,
//     points-in / points-out-IN-PLACE transform on the caller's own `glx_sfnt_point` array -- no
//     dynamic allocation, no `glx_sfnt_face` dependency (only the two POD types
//     `glx_sfnt_point`/`glx_hint_zones`), and CRITICALLY it leaves `glx_rasterize_outline`
//     UNTOUCHED: the correction is computed in device space (26.6) but converted BACK to font units
//     and written into `points[i].y`, so the rasterizer keeps re-scaling font units exactly as
//     before (the grid alignment simply survives that re-scale by construction). Only the Y
//     coordinate is ever modified; X is never touched this sub-phase.
//
//     CLEAN-ROOM: **no FreeType autohinter/pshinter source was read.** The IUP ("Interpolate
//     Untouched Points") idea is a PUBLIC concept from the TrueType/OpenType spec itself; the
//     grid-fitting math here (scale to device, round-to-nearest-pixel via add-half + arithmetic
//     shift, piecewise-linear interpolation of the untouched points between snapped zones) is
//     re-derived from first principles and the public spec -- see src/hint.c's own comments.
//
//     ZERO libm / ZERO float: unlike SOV-RAST (which justified `float` for analytic coverage), this
//     file is 100% integer (int32/int64) -- snapping is `((v + 32) & ~63)` (round-to-64 in 26.6),
//     interpolation is an `int64` round-divide. Proven the same way SOV-FCONV/SOV-RAST prove it:
//     `nm -u` shows no undefined libm symbol.
// PT: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-fase 1.1) -- um GRID-FITTER VERTICAL (eixo Y) clean-
//     room, SÓ-INTEIRO, pra outlines TrueType/SFNT. Dado um fluxo de ponto em UNIDADES DE FONTE (o
//     formato exato `glx_sfnt_point` que o SOV-SFNT produz), uma escala device (o mesmo par
//     racional `(scale_num, scale_den)` 26.6 que o `glx_rasterize_outline` consome), e um POD
//     pequeno de ZONAS HORIZONTAIS-CHAVE em unidades de fonte (baseline / x-height / cap-height /
//     ascender / descender), ele empurra o Y de cada ponto pra que as features das zonas caiam em
//     fronteiras de PIXEL INTEIRO no espaço device -- o passo que faz a baseline e a x-height do
//     texto pequeno "grudarem" na grade de pixel, deixando arestas horizontais borradas nítidas.
//
//     O QUE É / NÃO É: esta é a sub-fase 1.1 SÓ -- grid-fitting de zona no eixo Y (maior ROI, menor
//     risco). NÃO faz detecção/snap de HASTE vertical (sub-fase 1.2, decisão futura), NEM trabalho
//     no eixo X, NEM o wiring na casca C++ (sub-fases 1.3/1.4). É um transform PURO, ponto-entra /
//     ponto-sai-IN-PLACE no próprio array `glx_sfnt_point` de quem chama -- sem alocação dinâmica,
//     sem dependência de `glx_sfnt_face` (só os dois PODs `glx_sfnt_point`/`glx_hint_zones`), e
//     CRITICAMENTE deixa o `glx_rasterize_outline` INTOCADO: a correção é calculada em espaço device
//     (26.6) mas convertida DE VOLTA pra unidade de fonte e escrita em `points[i].y`, então o
//     rasterizador segue re-escalando unidade de fonte exatamente como antes (o alinhamento de
//     grade simplesmente sobrevive esse re-escale por construção). Só a coordenada Y é modificada; X
//     nunca é tocado nesta sub-fase.
//
//     CLEAN-ROOM: **nenhum fonte do autohinter/pshinter do FreeType foi lido.** A ideia do IUP
//     ("Interpolate Untouched Points") é um conceito PÚBLICO da própria spec TrueType/OpenType; a
//     matemática de grid-fitting aqui (escala pra device, arredonda-pro-pixel-mais-próximo via add-
//     half + shift aritmético, interpolação linear-por-partes dos pontos não-tocados entre zonas
//     snapadas) é re-derivada do zero e da spec pública -- ver os comentários próprios do src/hint.c.
//
//     ZERO libm / ZERO float: diferente do SOV-RAST (que justificou `float` pra cobertura
//     analítica), este arquivo é 100% inteiro (int32/int64) -- o snap é `((v + 32) & ~63)`
//     (arredonda-pra-64 em 26.6), a interpolação é uma divisão-arredondada `int64`. Provado do mesmo
//     jeito que SOV-FCONV/SOV-RAST provam: `nm -u` não mostra símbolo libm indefinido.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/sfnt.h"

#ifdef __cplusplus
extern "C" {
#endif

// EN: Axis-enable bitmask for `glx_hint_zones::axis_flags`. Only Y grid-fitting exists this
//     sub-phase -- with the Y bit UNSET, `glx_hint_outline` is a clean no-op (`GLX_HINT_NOOP`,
//     points untouched). The X bit is reserved for a future sub-phase and is currently ignored.
// PT: Bitmask de habilitação de eixo pro `glx_hint_zones::axis_flags`. Só grid-fitting Y existe
//     nesta sub-fase -- com o bit Y DESMARCADO, o `glx_hint_outline` é um no-op limpo
//     (`GLX_HINT_NOOP`, pontos intocados). O bit X é reservado pra uma sub-fase futura e é ignorado
//     por ora.
#define GLX_HINT_AXIS_Y 0x01u

// EN: The key horizontal reference lines, IN FONT UNITS (same space as `glx_sfnt_point::y` and
//     `head::units_per_em`). `baseline` is typically `0`; `descender` is typically NEGATIVE (below
//     the baseline). Zones need NOT be sorted or distinct -- `glx_hint_outline` sorts and de-dups
//     them internally, and tolerates any subset being equal (e.g. a caller that only knows the
//     baseline can leave the rest at `0`). `axis_flags` selects which axes to grid-fit (only
//     `GLX_HINT_AXIS_Y` is honoured this sub-phase). This sub-phase does NOT auto-derive these from
//     the font's `OS/2`/`hhea` metrics -- the caller passes them (a later sub-phase may derive
//     them; that is why `units_per_em` is part of the API surface even though this sub-phase only
//     validates it).
// PT: As linhas de referência horizontais-chave, EM UNIDADES DE FONTE (mesmo espaço de
//     `glx_sfnt_point::y` e `head::units_per_em`). `baseline` é tipicamente `0`; `descender` é
//     tipicamente NEGATIVO (abaixo da baseline). As zonas NÃO precisam estar ordenadas nem
//     distintas -- o `glx_hint_outline` as ordena e de-duplica internamente, e tolera qualquer
//     subconjunto igual (ex.: um chamador que só conhece a baseline pode deixar o resto em `0`).
//     `axis_flags` seleciona quais eixos grid-fitar (só `GLX_HINT_AXIS_Y` é honrado nesta sub-fase).
//     Esta sub-fase NÃO auto-deriva isto das métricas `OS/2`/`hhea` da fonte -- quem chama as passa
//     (uma sub-fase posterior pode derivá-las; é por isso que `units_per_em` faz parte da superfície
//     de API mesmo que esta sub-fase só o valide).
typedef struct {
    int16_t baseline;
    int16_t x_height;
    int16_t cap_height;
    int16_t ascender;
    int16_t descender;
    uint32_t axis_flags;
} glx_hint_zones;

// EN: Result of `glx_hint_outline`. `GLX_HINT_OK == 0` (points were modified toward the grid);
//     `GLX_HINT_NOOP` is a NON-error "nothing to do, points left byte-for-byte unchanged" (no axis
//     enabled, a degenerate scale/`units_per_em`, an empty outline, an outline with no feature
//     touching any zone, an outline ALREADY grid-fitted -- see `glx_hint_outline`'s own doc);
//     `GLX_HINT_ERR_INVALID_ARG` is a hard argument error (`NULL zones`, or `NULL points` with
//     `num_points > 0`). BOTH `OK` and `NOOP` are success from the caller's view (safe to rasterize
//     the resulting points); a caller can treat `>= GLX_HINT_ERR_INVALID_ARG` as failure.
// PT: Resultado do `glx_hint_outline`. `GLX_HINT_OK == 0` (pontos foram modificados em direção à
//     grade); `GLX_HINT_NOOP` é um "nada a fazer, pontos deixados byte-a-byte intactos" SEM erro
//     (nenhum eixo habilitado, uma escala/`units_per_em` degenerada, um outline vazio, um outline
//     sem feature tocando zona nenhuma, um outline JÁ grid-fitado -- ver o doc próprio do
//     `glx_hint_outline`); `GLX_HINT_ERR_INVALID_ARG` é um erro forte de argumento (`zones` NULL, ou
//     `points` NULL com `num_points > 0`). TANTO `OK` QUANTO `NOOP` são sucesso do ponto de vista de
//     quem chama (seguro rasterizar os pontos resultantes); quem chama pode tratar
//     `>= GLX_HINT_ERR_INVALID_ARG` como falha.
typedef enum {
    GLX_HINT_OK = 0,
    GLX_HINT_NOOP,
    GLX_HINT_ERR_INVALID_ARG,
} glx_hint_result;

// EN: Grid-fits `points[0..num_points)` IN PLACE along the Y axis (X untouched), so that outline
//     features sitting on the key zones land on whole device pixels.
//
//     ALGORITHM (see src/hint.c for the full derivation): each active zone `z` (font units) maps to
//     device `z_dev = round(z * scale_num / scale_den)` (26.6, same rounding as
//     `glx_rasterize_outline`), snapped to the nearest whole pixel `z_snap = round64(z_dev)`; the
//     font-unit position that best represents that pixel line, `grid_font(z)`, is precomputed. A
//     point whose device Y lands within HALF A PIXEL of a zone (it "falls in" that zone) is snapped:
//     its Y is set to that zone's `grid_font` -- collapsing the whole feature edge onto one crisp
//     pixel line. Points BETWEEN zones are INTERPOLATED (IUP-like): a monotonic piecewise-linear
//     map carries them proportionally between the snapped positions of the surrounding zones (points
//     below the lowest / above the highest zone are rigidly translated by that end zone's shift), so
//     the contour stays continuous with no tears.
//
//     COORDINATE SPACES: the snap/interpolation is reasoned in DEVICE space (26.6) but every write
//     is converted BACK to font units and clamped to `int16_t` -- so `glx_rasterize_outline` stays
//     bit-for-bit unchanged (it re-scales font units itself). `(scale_num, scale_den)` is the SAME
//     rational device scale that function takes (`scale_den > 0`; an SFNT caller passes
//     `(px_size_26_6, face->units_per_em)`). `units_per_em` is validated (`> 0`) and reserved for a
//     future sub-phase that auto-derives zones from font metrics; this sub-phase's math uses only
//     `(scale_num, scale_den)`. For an SFNT caller `scale_den == units_per_em`.
//
//     IDEMPOTENT: applying twice equals applying once -- the second call detects the already-fitted
//     state (every in-band feature point already sits exactly on its zone's `grid_font`, an EXACT
//     integer font-unit test, no tolerance) and returns `GLX_HINT_NOOP` without moving anything.
//     An outline whose zones already land on whole pixels is likewise a `GLX_HINT_NOOP`.
//
//     HOSTILE INPUT (points/zones come from an UNTRUSTED font): every degenerate case is a CLEAN
//     `GLX_HINT_NOOP` or `GLX_HINT_ERR_INVALID_ARG`, NEVER undefined behaviour -- `scale_den <= 0`,
//     `scale_num <= 0`, `units_per_em == 0`, `num_points == 0`, zones out of order or all-equal or
//     negative are all handled (out-of-order zones are sorted internally; a zero-height inter-zone
//     span never divides). Every division guards its denominator; every device coordinate is
//     saturated before arithmetic; every font write is `int16_t`-clamped.
//
//     Returns `GLX_HINT_ERR_INVALID_ARG` for `zones == NULL`, or `points == NULL` with
//     `num_points > 0`. Returns `GLX_HINT_NOOP` (points unchanged) when there is nothing to do (see
//     above). Returns `GLX_HINT_OK` when at least one point's Y was moved toward the grid.
// PT: Grid-fita `points[0..num_points)` IN PLACE ao longo do eixo Y (X intocado), pra que features
//     do outline pousadas nas zonas-chave caiam em pixels device inteiros.
//
//     ALGORITMO (ver src/hint.c pra derivação completa): cada zona ativa `z` (unidades de fonte)
//     mapeia pra device `z_dev = round(z * scale_num / scale_den)` (26.6, mesmo arredondamento do
//     `glx_rasterize_outline`), snapada pro pixel inteiro mais próximo `z_snap = round64(z_dev)`; a
//     posição em unidade-de-fonte que melhor representa essa linha de pixel, `grid_font(z)`, é
//     pré-computada. Um ponto cujo Y device cai a MEIO PIXEL de uma zona (ele "cai naquela zona") é
//     snapado: seu Y vira o `grid_font` daquela zona -- colapsando a aresta inteira da feature numa
//     linha de pixel nítida. Pontos ENTRE zonas são INTERPOLADOS (IUP-like): um mapa linear-por-
//     partes monotônico os carrega proporcionalmente entre as posições snapadas das zonas ao redor
//     (pontos abaixo da mais baixa / acima da mais alta são transladados rigidamente pelo
//     deslocamento daquela zona-ponta), pro contorno seguir contínuo sem rasgo.
//
//     ESPAÇOS DE COORDENADA: o snap/interpolação é raciocinado em espaço DEVICE (26.6) mas toda
//     escrita é convertida DE VOLTA pra unidade de fonte e clampada em `int16_t` -- então o
//     `glx_rasterize_outline` segue bit-a-bit intocado (ele re-escala unidade de fonte sozinho).
//     `(scale_num, scale_den)` é a MESMA escala device racional que aquela função recebe
//     (`scale_den > 0`; um chamador SFNT passa `(px_size_26_6, face->units_per_em)`). `units_per_em`
//     é validado (`> 0`) e reservado pra uma sub-fase futura que auto-deriva zonas de métricas da
//     fonte; a matemática desta sub-fase usa só `(scale_num, scale_den)`. Pra um chamador SFNT
//     `scale_den == units_per_em`.
//
//     IDEMPOTENTE: aplicar duas vezes igual aplicar uma -- a segunda chamada detecta o estado já-
//     fitado (todo ponto-feature em-banda já pousa exatamente no `grid_font` da sua zona, um teste
//     EXATO em unidade-de-fonte inteira, sem tolerância) e retorna `GLX_HINT_NOOP` sem mover nada.
//     Um outline cujas zonas já caem em pixels inteiros é igualmente um `GLX_HINT_NOOP`.
//
//     INPUT HOSTIL (pontos/zonas vêm de uma fonte NÃO-CONFIÁVEL): todo caso degenerado é um
//     `GLX_HINT_NOOP` ou `GLX_HINT_ERR_INVALID_ARG` LIMPO, NUNCA comportamento indefinido --
//     `scale_den <= 0`, `scale_num <= 0`, `units_per_em == 0`, `num_points == 0`, zonas fora de
//     ordem ou todas-iguais ou negativas são todas tratadas (zonas fora de ordem são ordenadas
//     internamente; um vão inter-zona de altura zero nunca divide). Toda divisão guarda seu
//     denominador; toda coordenada device é saturada antes da aritmética; toda escrita de fonte é
//     clampada em `int16_t`.
//
//     Retorna `GLX_HINT_ERR_INVALID_ARG` pra `zones == NULL`, ou `points == NULL` com
//     `num_points > 0`. Retorna `GLX_HINT_NOOP` (pontos intocados) quando não há nada a fazer (ver
//     acima). Retorna `GLX_HINT_OK` quando ao menos um Y de ponto foi movido em direção à grade.
glx_hint_result glx_hint_outline(glx_sfnt_point* points, uint16_t num_points,
                                 uint16_t units_per_em, int32_t scale_num, int32_t scale_den,
                                 const glx_hint_zones* zones);

#ifdef __cplusplus
}
#endif
