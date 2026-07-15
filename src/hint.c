// SPDX-License-Identifier: MPL-2.0
// EN: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-phase 1.1) -- implementation. See include/core/hint.h's
//     file header for the full clean-room rationale, scope (Y-axis zone grid-fitting ONLY), and the
//     public contract; this file's comments focus on the LOCAL "how".
//
//     BUILD-TWICE NOTE (mirrors src/sfnt.c / src/raster.c): this file is compiled TWICE by the
//     Makefile -- once freestanding into build/obj/ (tests/test_hint.c and every other gate
//     program), once under $(CORE_RENAME_FLAGS) into build/objcore/ for build/libcore.a. Its only
//     externally-visible symbol is `glx_hint_outline` (already namespaced by construction, no
//     $(CORE_RENAME_FLAGS) entry needed -- see tools/check_libcore_symbols.sh's `required` list);
//     every other identifier below is `static` (internal linkage). This file `#include`s only
//     <stdint.h>/<stddef.h>/"core/hint.h" (which pulls "core/sfnt.h" for the `glx_sfnt_point` POD),
//     so, exactly like src/raster.c, it also compiles UNCHANGED as a plain hosted translation unit.
//
//     THE ALGORITHM (re-derived from the PUBLIC TrueType/OpenType spec + first-principles grid-
//     fitting -- NO FreeType source read, see hint.h's CLEAN-ROOM note):
//
//     (1) ZONE SETUP: gather the 5 caller zones (font units), sort ascending, de-dup. For each
//         unique zone `zf`: device position `zdev = round(zf * num / den)` (26.6, the SAME rounding
//         src/raster.c's `scale_to_fx` uses, so a snapped point lands where the rasterizer expects),
//         snapped-to-pixel `zsnap = round64(zdev)` (add-half + mask, ZERO libm), and the font-unit
//         representative of that pixel line `gridf = zf + round((zsnap - zdev) * den / num)` (the
//         device correction converted BACK to font units -- this is what keeps the rasterizer
//         untouched). `zsnap` is monotonic in `zdev` (round is monotonic), hence `gridf` is
//         monotonic in `zf` -- the interpolation below can never invert point order.
//
//     (2) CLASSIFY each point by its device Y `pdev = round(pt.y * num / den)`: it is TOUCHED by the
//         nearest zone within HALF A PIXEL (`|pdev - zdev| <= 32` in 26.6) -- "its Y falls in that
//         zone" -- otherwise UNTOUCHED.
//
//     (3) APPLY (Y only, in place): a TOUCHED point's Y is set to its zone's `gridf` (the whole
//         feature edge collapses onto one crisp pixel line). An UNTOUCHED point between zones `lo`
//         and `hi` is INTERPOLATED: `gridf[lo] + (gridf[hi]-gridf[lo]) * (pt.y - zf[lo]) / (zf[hi]-
//         zf[lo])` (IUP -- Interpolate Untouched Points, clamped between the two `gridf` for
//         monotonicity); below the lowest / above the highest zone it is rigidly translated by that
//         end zone's own `gridf - zf` shift. After interpolation a point that happens to land back
//         inside a zone band is PROMOTED to that zone's `gridf` (keeps the post-fit invariant
//         "every in-band point sits exactly on a gridf" -- what makes idempotency exact, see (4)).
//
//     (4) IDEMPOTENCY is exact and unconditional: before applying, a detection pass returns
//         `GLX_HINT_NOOP` iff every currently in-band point already sits EXACTLY on its zone's
//         `gridf` (an integer font-unit equality, no tolerance -- possible precisely because step 3
//         writes the CONSTANT `gridf`, not a per-point rounded delta, so the fitted state is a fixed
//         point of the map). First call: features are off-grid -> detection false -> apply. Second
//         call: features sit on `gridf` -> detection true -> NOOP. The promotion in step 3
//         guarantees no untouched point can drift into a band while off its `gridf`, so the
//         detection is never fooled.
// PT: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-fase 1.1) -- implementação. Ver o cabeçalho de arquivo
//     do include/core/hint.h pro racional clean-room completo, escopo (grid-fitting de zona no eixo
//     Y SÓ), e o contrato público; os comentários deste arquivo focam no "como" LOCAL.
//
//     NOTA DE COMPILAÇÃO-DUPLA (espelha src/sfnt.c / src/raster.c): este arquivo é compilado DUAS
//     VEZES pelo Makefile -- uma freestanding pra build/obj/ (tests/test_hint.c e todo outro
//     programa-gate), outra sob $(CORE_RENAME_FLAGS) pra build/objcore/ pra build/libcore.a. Seu
//     único símbolo externamente visível é `glx_hint_outline` (já namespaced por construção, nenhuma
//     entrada em $(CORE_RENAME_FLAGS) necessária -- ver a lista `required` do
//     tools/check_libcore_symbols.sh); todo outro identificador abaixo é `static` (linkage interno).
//     Este arquivo só `#include`ia <stdint.h>/<stddef.h>/"core/hint.h" (que puxa "core/sfnt.h" pro
//     POD `glx_sfnt_point`), então, exatamente como src/raster.c, compila INALTERADO como uma
//     unidade de tradução hosted comum também.
//
//     O ALGORITMO (re-derivado da spec PÚBLICA TrueType/OpenType + grid-fitting do zero -- NENHUM
//     fonte do FreeType lido, ver a nota CLEAN-ROOM do hint.h):
//
//     (1) SETUP DE ZONA: junta as 5 zonas de quem chama (unidades de fonte), ordena ascendente,
//         de-duplica. Pra cada zona única `zf`: posição device `zdev = round(zf * num / den)` (26.6,
//         o MESMO arredondamento que o `scale_to_fx` do src/raster.c usa, pra um ponto snapado
//         pousar onde o rasterizador espera), snapada-pro-pixel `zsnap = round64(zdev)` (add-half +
//         máscara, ZERO libm), e o representante em unidade-de-fonte dessa linha de pixel
//         `gridf = zf + round((zsnap - zdev) * den / num)` (a correção device convertida DE VOLTA
//         pra unidade de fonte -- é isto que mantém o rasterizador intocado). `zsnap` é monotônico
//         em `zdev` (round é monotônico), logo `gridf` é monotônico em `zf` -- a interpolação abaixo
//         nunca consegue inverter a ordem dos pontos.
//
//     (2) CLASSIFICA cada ponto pelo seu Y device `pdev = round(pt.y * num / den)`: ele é TOCADO
//         pela zona mais próxima dentro de MEIO PIXEL (`|pdev - zdev| <= 32` em 26.6) -- "seu Y cai
//         naquela zona" -- senão NÃO-TOCADO.
//
//     (3) APLICA (Y só, in place): o Y de um ponto TOCADO vira o `gridf` da sua zona (a aresta
//         inteira da feature colapsa numa linha de pixel nítida). Um ponto NÃO-TOCADO entre zonas
//         `lo` e `hi` é INTERPOLADO: `gridf[lo] + (gridf[hi]-gridf[lo]) * (pt.y - zf[lo]) / (zf[hi]-
//         zf[lo])` (IUP -- Interpolate Untouched Points, clampado entre os dois `gridf` pra
//         monotonicidade); abaixo da mais baixa / acima da mais alta zona é transladado rigidamente
//         pelo deslocamento `gridf - zf` daquela zona-ponta. Depois da interpolação um ponto que por
//         acaso pouse de volta dentro de uma banda de zona é PROMOVIDO ao `gridf` daquela zona
//         (mantém o invariante pós-fit "todo ponto em-banda pousa exatamente num gridf" -- o que
//         torna a idempotência exata, ver (4)).
//
//     (4) IDEMPOTÊNCIA é exata e incondicional: antes de aplicar, uma passada de detecção retorna
//         `GLX_HINT_NOOP` sse todo ponto em-banda atual já pousa EXATAMENTE no `gridf` da sua zona
//         (uma igualdade inteira em unidade-de-fonte, sem tolerância -- possível precisamente porque
//         o passo 3 escreve o `gridf` CONSTANTE, não um delta arredondado por-ponto, então o estado
//         fitado é um ponto fixo do mapa). Primeira chamada: features fora-da-grade -> detecção
//         falsa -> aplica. Segunda chamada: features pousam no `gridf` -> detecção verdadeira ->
//         NOOP. A promoção no passo 3 garante que nenhum ponto não-tocado pode derivar pra dentro de
//         uma banda enquanto fora do seu `gridf`, então a detecção nunca é enganada.
// Copyright (c) 2026 Petrus Silva Costa
#include "core/hint.h"

// EN: Half a pixel in 26.6 device units -- the band half-width for "this point's Y falls in that
//     zone". `32 == 0.5 * 64`.
// PT: Meio pixel em unidades device 26.6 -- a meia-largura de banda pra "o Y deste ponto cai naquela
//     zona". `32 == 0.5 * 64`.
#define HINT_BAND_26_6 32

// EN: Device-space saturation bound (26.6), same value/rationale as src/raster.c's GLX_RASTER_FX_MAX
//     -- generous (262144 px), and small enough that no intermediate sum here overflows int32/int64.
// PT: Limite de saturação de espaço device (26.6), mesmo valor/racional do GLX_RASTER_FX_MAX do
//     src/raster.c -- generoso (262144 px), e pequeno o bastante que nenhuma soma intermediária aqui
//     estoura int32/int64.
#define HINT_FX_MAX ((int32_t)16777216)

// EN: `a / b` rounded to nearest, ties away from zero, `b > 0` a PRECONDITION every caller here
//     already guarantees (num/den validated `> 0`, inter-zone spans proven `> 0` by the sort+dedup).
//     `int64_t` throughout: the largest product this file forms is `int16 * int32` (a font coord
//     times `scale_num`), ~7e13, comfortably inside int64's ~9.2e18 -- same overflow reasoning as
//     src/raster.c's `scale_to_fx`.
// PT: `a / b` arredondado pro mais próximo, empate longe de zero, `b > 0` uma PRECONDIÇÃO que todo
//     chamador aqui já garante (num/den validados `> 0`, vãos inter-zona provados `> 0` pelo
//     sort+dedup). `int64_t` por toda parte: o maior produto que este arquivo forma é `int16 *
//     int32` (uma coord de fonte vezes `scale_num`), ~7e13, confortavelmente dentro dos ~9.2e18 do
//     int64 -- mesmo racional de overflow que o `scale_to_fx` do src/raster.c.
static int64_t rdiv_round(int64_t a, int64_t b) {
    int64_t half = b / 2;
    return (a >= 0) ? (a + half) / b : -(((-a) + half) / b);
}

// EN: `font_unit * num / den` rounded to nearest (ties away), saturated into
//     `[-HINT_FX_MAX, HINT_FX_MAX]` -- the exact device-space mapping src/raster.c's `scale_to_fx`
//     performs (replicated, not shared, because that one is `static` to its own TU). `den > 0` is a
//     precondition (validated in `glx_hint_outline`).
// PT: `font_unit * num / den` arredondado pro mais próximo (empate longe), saturado em
//     `[-HINT_FX_MAX, HINT_FX_MAX]` -- o mapeamento device exato que o `scale_to_fx` do
//     src/raster.c faz (replicado, não compartilhado, porque aquele é `static` na própria TU).
//     `den > 0` é precondição (validada no `glx_hint_outline`).
static int32_t hint_scale_fx(int32_t font_unit, int32_t num, int32_t den) {
    int64_t r = rdiv_round((int64_t)font_unit * (int64_t)num, (int64_t)den);
    if (r > (int64_t)HINT_FX_MAX) return HINT_FX_MAX;
    if (r < -(int64_t)HINT_FX_MAX) return -HINT_FX_MAX;
    return (int32_t)r;
}

// EN: Round a 26.6 device value to the nearest WHOLE PIXEL (nearest multiple of 64), ties toward
//     +infinity. `(v + 32) & ~63` in `int64_t`: add half a pixel then clear the low 6 fractional
//     bits. Works for negatives via two's-complement AND (which floors toward -inf, so the +32 makes
//     it round-half-up) -- e.g. `-672 -> (-640) & ~63 = -640` (`-10.5px -> -10px`). `v` is already
//     `HINT_FX_MAX`-bounded so `v + 32` cannot overflow. PURE INTEGER, zero libm (this ticket's
//     brief: "round = add-half + shift").
// PT: Arredonda um valor device 26.6 pro PIXEL INTEIRO mais próximo (múltiplo de 64 mais próximo),
//     empate pra +infinito. `(v + 32) & ~63` em `int64_t`: soma meio pixel e limpa os 6 bits
//     fracionários baixos. Funciona pra negativos via AND dois-complemento (que faz floor pra -inf,
//     então o +32 o torna round-half-up) -- ex.: `-672 -> (-640) & ~63 = -640` (`-10.5px -> -10px`).
//     `v` já é limitado por `HINT_FX_MAX` então `v + 32` não pode estourar. INTEIRO PURO, zero libm
//     (brief desta tarefa: "round = add-half + shift").
static int32_t hint_snap_pixel(int32_t v) {
    return (int32_t)(((int64_t)v + 32) & ~(int64_t)63);
}

// EN: Saturating narrow to `int16_t` (the wire type of `glx_sfnt_point::y`). Same posture as
//     src/sfnt.c's saturating casts -- an out-of-range hinted coordinate is a degenerate OUTPUT
//     (a point pushed absurdly far), clamped rather than wrapped, never UB.
// PT: Estreitamento saturante pra `int16_t` (o tipo de fio de `glx_sfnt_point::y`). Mesma postura
//     dos casts saturantes do src/sfnt.c -- uma coordenada hintada fora de faixa é uma SAÍDA
//     degenerada (um ponto empurrado absurdamente longe), clampada em vez de dar a volta, nunca UB.
static int16_t hint_sat_i16(int64_t v) {
    if (v > (int64_t)INT16_MAX) return INT16_MAX;
    if (v < (int64_t)INT16_MIN) return INT16_MIN;
    return (int16_t)v;
}

// EN: The de-duplicated, ascending-sorted active zones and their precomputed device/pixel/font-grid
//     values. `n` is at most 5 (the 5 caller zones). Fixed-size arrays -- no dynamic allocation.
// PT: As zonas ativas de-duplicadas, ordenadas ascendente, e seus valores device/pixel/grade-de-
//     fonte pré-computados. `n` é no máximo 5 (as 5 zonas de quem chama). Arrays de tamanho fixo --
//     sem alocação dinâmica.
typedef struct {
    int n;
    int32_t zf[5];    // EN: zone font-Y (sorted asc, unique)   PT: Y-fonte da zona (asc, único)
    int32_t zdev[5];  // EN: zone device-Y (26.6)               PT: Y-device da zona (26.6)
    int32_t gridf[5]; // EN: font-Y representing the pixel line  PT: Y-fonte da linha de pixel
} hint_zone_set;

// EN: Nearest zone whose device-Y is within HINT_BAND_26_6 of `pdev`; returns its index or -1 (the
//     point is UNTOUCHED). Linear scan over <=5 zones (correctness-first, same trade-off as
//     src/sfnt.c's table scans). Ties (equal distance) resolve to the LOWER index -- deterministic.
// PT: Zona mais próxima cujo Y-device está dentro de HINT_BAND_26_6 de `pdev`; retorna seu índice ou
//     -1 (o ponto é NÃO-TOCADO). Varredura linear sobre <=5 zonas (correção-primeiro, mesma troca
//     das varreduras de tabela do src/sfnt.c). Empates (distância igual) resolvem pro índice MENOR
//     -- determinístico.
static int hint_zone_in_band(const hint_zone_set* zs, int32_t pdev) {
    int best = -1;
    int32_t best_dist = 0;
    for (int i = 0; i < zs->n; i++) {
        int32_t d = pdev - zs->zdev[i];
        if (d < 0) d = -d;
        if (d <= HINT_BAND_26_6 && (best < 0 || d < best_dist)) {
            best = i;
            best_dist = d;
        }
    }
    return best;
}

// EN: Builds `zs` from the caller's 5 zones: gather, insertion-sort ascending, de-dup equal font-Y,
//     then precompute `zdev`/`gridf` for each unique zone. Field-by-field gather (NOT an aggregate
//     initializer) -- avoids the clang backend synthesizing a `memcpy`/`memset` the
//     $(CORE_RENAME_FLAGS) rename cannot reach (exact same footgun src/sfnt.c's `kern_t` hit; see
//     tools/check_libcore_symbols.sh PASS 2). Always yields `n >= 1` (at least the baseline).
// PT: Constrói `zs` das 5 zonas de quem chama: junta, insertion-sort ascendente, de-duplica Y-fonte
//     igual, depois pré-computa `zdev`/`gridf` pra cada zona única. Junção campo-a-campo (NÃO um
//     inicializador de agregado) -- evita o backend do clang sintetizar um `memcpy`/`memset` que o
//     rename $(CORE_RENAME_FLAGS) não alcança (exatamente o mesmo footgun que o `kern_t` do
//     src/sfnt.c bateu; ver PASSO 2 do tools/check_libcore_symbols.sh). Sempre resulta `n >= 1` (ao
//     menos a baseline).
static void hint_build_zones(hint_zone_set* zs, const glx_hint_zones* zones, int32_t num,
                             int32_t den) {
    int32_t raw[5];
    raw[0] = zones->baseline;
    raw[1] = zones->x_height;
    raw[2] = zones->cap_height;
    raw[3] = zones->ascender;
    raw[4] = zones->descender;

    // EN: Insertion sort ascending (5 elements -- same tiny-sort idiom as src/raster.c's 4-value
    //     scanline sort). PT: Insertion sort ascendente (5 elementos -- mesmo idioma de sort-pequeno
    //     do sort de 4 valores de scanline do src/raster.c).
    for (int i = 1; i < 5; i++) {
        int32_t key = raw[i];
        int j = i - 1;
        while (j >= 0 && raw[j] > key) {
            raw[j + 1] = raw[j];
            j--;
        }
        raw[j + 1] = key;
    }

    zs->n = 0;
    for (int i = 0; i < 5; i++) {
        if (zs->n > 0 && raw[i] == zs->zf[zs->n - 1]) continue; // EN: skip duplicate PT: pula dup
        int idx = zs->n;
        int32_t zf = raw[i];
        int32_t zdev = hint_scale_fx(zf, num, den);
        int32_t zsnap = hint_snap_pixel(zdev);
        // EN: device correction (zsnap - zdev) converted BACK to font units and added to the zone's
        //     own font position -> the font-Y whose re-scale lands on the pixel line. PT: correção
        //     device (zsnap - zdev) convertida DE VOLTA pra unidade de fonte e somada à posição de
        //     fonte da própria zona -> o Y-fonte cujo re-escale pousa na linha de pixel.
        int32_t gridf = zf + (int32_t)rdiv_round((int64_t)(zsnap - zdev) * (int64_t)den, (int64_t)num);
        zs->zf[idx] = zf;
        zs->zdev[idx] = zdev;
        zs->gridf[idx] = gridf;
        zs->n = idx + 1;
    }
}

// EN: The hinted font-Y for a point at original font-Y `y` classified as UNTOUCHED (not in any
//     band). Interpolates `gridf` between the two zones bracketing `y`; below the lowest / above the
//     highest zone, rigidly translates by that end zone's `gridf - zf` shift. Result is clamped
//     between the two bracketing `gridf` (monotonic -- a point between two zones can never overshoot
//     either snapped zone). PROMOTION: if the interpolated position lands back inside some zone's
//     band, it is snapped to that zone's `gridf` (keeps the post-fit invariant that makes
//     idempotency exact -- see this file's header (4)).
// PT: O Y-fonte hintado pra um ponto no Y-fonte original `y` classificado como NÃO-TOCADO (fora de
//     toda banda). Interpola `gridf` entre as duas zonas que cercam `y`; abaixo da mais baixa /
//     acima da mais alta zona, translada rigidamente pelo deslocamento `gridf - zf` daquela zona-
//     ponta. Resultado é clampado entre os dois `gridf` que cercam (monotônico -- um ponto entre
//     duas zonas nunca ultrapassa nenhuma das zonas snapadas). PROMOÇÃO: se a posição interpolada
//     pousa de volta dentro da banda de alguma zona, é snapada pro `gridf` daquela zona (mantém o
//     invariante pós-fit que torna a idempotência exata -- ver cabeçalho deste arquivo (4)).
static int32_t hint_untouched_y(const hint_zone_set* zs, int32_t y, int32_t num, int32_t den) {
    int32_t ny;
    if (y <= zs->zf[0]) {
        // EN: below the lowest zone -- rigid translate. PT: abaixo da mais baixa -- translação rígida.
        ny = y + (zs->gridf[0] - zs->zf[0]);
    } else if (y >= zs->zf[zs->n - 1]) {
        ny = y + (zs->gridf[zs->n - 1] - zs->zf[zs->n - 1]);
    } else {
        // EN: find the bracket zf[lo] < y < zf[hi] (zf strictly increasing after dedup, so the span
        //     is > 0 -- no division by zero). PT: acha o intervalo zf[lo] < y < zf[hi] (zf
        //     estritamente crescente após dedup, então o vão é > 0 -- sem divisão por zero).
        int lo = 0;
        while (lo + 1 < zs->n && zs->zf[lo + 1] <= y) lo++;
        int hi = lo + 1;
        int32_t g_lo = zs->gridf[lo], g_hi = zs->gridf[hi];
        ny = g_lo + (int32_t)rdiv_round((int64_t)(g_hi - g_lo) * (int64_t)(y - zs->zf[lo]),
                                        (int64_t)(zs->zf[hi] - zs->zf[lo]));
        int32_t bnd_lo = (g_lo < g_hi) ? g_lo : g_hi;
        int32_t bnd_hi = (g_lo < g_hi) ? g_hi : g_lo;
        if (ny < bnd_lo) ny = bnd_lo;
        if (ny > bnd_hi) ny = bnd_hi;
    }
    // EN: promotion -- if the interpolated point landed in a band, snap it onto that zone's gridf.
    // PT: promoção -- se o ponto interpolado caiu numa banda, snapa ele no gridf daquela zona.
    int j = hint_zone_in_band(zs, hint_scale_fx(ny, num, den));
    if (j >= 0) ny = zs->gridf[j];
    return ny;
}

glx_hint_result glx_hint_outline(glx_sfnt_point* points, uint16_t num_points,
                                 uint16_t units_per_em, int32_t scale_num, int32_t scale_den,
                                 const glx_hint_zones* zones) {
    // EN: Hard argument errors first. PT: Erros fortes de argumento primeiro.
    if (zones == NULL) return GLX_HINT_ERR_INVALID_ARG;
    if (points == NULL && num_points > 0) return GLX_HINT_ERR_INVALID_ARG;

    // EN: Nothing-to-do cases -> clean NOOP, points left byte-for-byte unchanged. `units_per_em` is
    //     validated here (its only use this sub-phase -- see hint.h). PT: Casos nada-a-fazer -> NOOP
    //     limpo, pontos deixados byte-a-byte intactos. `units_per_em` é validado aqui (seu único uso
    //     nesta sub-fase -- ver hint.h).
    if (!(zones->axis_flags & GLX_HINT_AXIS_Y)) return GLX_HINT_NOOP;
    if (scale_den <= 0 || scale_num <= 0 || units_per_em == 0 || num_points == 0) return GLX_HINT_NOOP;

    hint_zone_set zs;
    hint_build_zones(&zs, zones, scale_num, scale_den);

    // EN: DETECTION pass (idempotency + "nothing at any zone"): scan every point; if a point is
    //     in-band but NOT already exactly on its zone's gridf, there is real work to do. If NO
    //     in-band point is off its gridf (covers both "already fitted" and "no feature touches any
    //     zone"), return NOOP without moving anything. PT: Passada de DETECÇÃO (idempotência +
    //     "nada em zona nenhuma"): varre todo ponto; se um ponto está em-banda mas NÃO já
    //     exatamente no gridf da sua zona, há trabalho real. Se NENHUM ponto em-banda está fora do
    //     seu gridf (cobre tanto "já fitado" quanto "nenhuma feature toca zona"), retorna NOOP sem
    //     mover nada.
    int work = 0;
    for (uint16_t i = 0; i < num_points; i++) {
        int32_t pdev = hint_scale_fx(points[i].y, scale_num, scale_den);
        int j = hint_zone_in_band(&zs, pdev);
        if (j >= 0 && points[i].y != zs.gridf[j]) {
            work = 1;
            break;
        }
    }
    if (!work) return GLX_HINT_NOOP;

    // EN: APPLY pass (Y only, in place). PT: Passada de APLICAÇÃO (Y só, in place).
    for (uint16_t i = 0; i < num_points; i++) {
        int32_t pdev = hint_scale_fx(points[i].y, scale_num, scale_den);
        int j = hint_zone_in_band(&zs, pdev);
        int32_t ny;
        if (j >= 0) {
            ny = zs.gridf[j]; // EN: touched -> snap feature to grid PT: tocado -> snapa feature na grade
        } else {
            ny = hint_untouched_y(&zs, points[i].y, scale_num, scale_den);
        }
        points[i].y = hint_sat_i16((int64_t)ny);
    }
    return GLX_HINT_OK;
}
