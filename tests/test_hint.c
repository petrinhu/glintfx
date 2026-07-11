// SPDX-License-Identifier: MPL-2.0
// EN: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-phase 1.1) -- functional/oracle test suite (C1 harness,
//     freestanding). See include/core/hint.h and src/hint.c for the full contract/algorithm. Every
//     expected value below is derived BY HAND from the closed-form scale/snap arithmetic (documented
//     inline), never a binary golden fixture. `test_real_font_O` additionally exercises a REAL Open
//     Sans glyph (same `ld -r -b binary` fixture as tests/test_sfnt.c / tests/test_raster.c) for the
//     robustness/idempotency proof on genuine outline data.
// PT: SOV-HINT (L1.20-FONTFLIP / FT-F4, sub-fase 1.1) -- suíte de teste funcional/oráculo (harness
//     C1, freestanding). Ver include/core/hint.h e src/hint.c pro contrato/algoritmo completo. Todo
//     valor esperado abaixo é derivado À MÃO da aritmética de escala/snap de forma fechada
//     (documentado inline), nunca uma fixture golden binária. `test_real_font_O` ainda exercita um
//     glyph Open Sans REAL (mesma fixture `ld -r -b binary` do tests/test_sfnt.c / tests/test_raster.c)
//     pra prova de robustez/idempotência em dado de outline genuíno.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "core/hint.h"
#include "core/sfnt.h"

// EN: Open Sans fixture (real TrueType blob), same embedding as tests/test_sfnt.c/test_raster.c.
// PT: Fixture Open Sans (blob TrueType real), mesmo embutimento do tests/test_sfnt.c/test_raster.c.
extern const unsigned char _binary_opensans_regular_ttf_start[];
extern const unsigned char _binary_opensans_regular_ttf_end[];

// EN: Re-derives, in the test, the EXACT device-space scale src/hint.c uses (`font * num / den`,
//     round-to-nearest ties-away, int64) -- lets each assertion below independently compute where a
//     hinted point lands in 26.6 device space and check it against the pixel grid. Intentionally a
//     hand copy of the algorithm's formula (an independent oracle), not a call into src/hint.c
//     (whose scale helper is `static`).
// PT: Re-deriva, no teste, a escala de espaço-device EXATA que o src/hint.c usa (`font * num / den`,
//     arredonda-pro-mais-próximo empate-longe, int64) -- deixa cada asserção abaixo computar
//     independentemente onde um ponto hintado pousa no espaço device 26.6 e checar contra a grade de
//     pixel. Deliberadamente uma cópia à mão da fórmula do algoritmo (um oráculo independente), não
//     uma chamada ao src/hint.c (cujo helper de escala é `static`).
static int32_t t_scale(int32_t font_unit, int32_t num, int32_t den) {
    int64_t n = (int64_t)font_unit * (int64_t)num;
    int64_t half = (int64_t)den / 2;
    int64_t r = (n >= 0) ? (n + half) / den : -(((-n) + half) / den);
    return (int32_t)r;
}

// EN: Distance of a 26.6 value from its nearest whole pixel (nearest multiple of 64). `0` means
//     exactly on the pixel grid. PT: Distância de um valor 26.6 do pixel inteiro mais próximo
//     (múltiplo de 64 mais próximo). `0` significa exatamente na grade de pixel.
static int32_t t_off_grid(int32_t v_26_6) {
    int32_t snapped = (int32_t)(((int64_t)v_26_6 + 32) & ~(int64_t)63);
    int32_t d = v_26_6 - snapped;
    return (d < 0) ? -d : d;
}

// ================================================================================================
// EN: (1) ZONE LANDS ON A WHOLE PIXEL -- the core promise. upem=1000, scale (num=672, den=1000) =
//     a 10.5px em. A point placed EXACTLY at x_height (font y=1000) scales to device
//     `round(1000*672/1000) = 672` = 10.5px (a HALF pixel, off-grid). After hinting, that point's
//     Y must scale to a WHOLE pixel. Worked arithmetic: x_height snaps 672 -> 704 (11px); the
//     font-Y representing 11px is `gridf = 1000 + round((704-672)*1000/672) = 1000 + 48 = 1048`;
//     `round(1048*672/1000) = 704`, exactly 11px on the grid.
// PT: (1) ZONA POUSA NUM PIXEL INTEIRO -- a promessa central. upem=1000, escala (num=672, den=1000)
//     = um em de 10.5px. Um ponto posto EXATAMENTE na x_height (y-fonte 1000) escala pra device
//     `round(1000*672/1000) = 672` = 10.5px (MEIO pixel, fora-da-grade). Após hinting, o Y desse
//     ponto precisa escalar pra um pixel INTEIRO. Aritmética por extenso: x_height snapa 672 -> 704
//     (11px); o Y-fonte representando 11px é `gridf = 1000 + round((704-672)*1000/672) = 1000 + 48 =
//     1048`; `round(1048*672/1000) = 704`, exatamente 11px na grade.
// ================================================================================================
static void test_zone_lands_on_pixel(void) {
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1000;
    z.cap_height = 1400;
    z.ascender = 1500;
    z.descender = -300;
    z.axis_flags = GLX_HINT_AXIS_Y;

    // EN: A tiny mutable outline: one point AT x_height (touched), one below it (untouched). X is
    //     arbitrary (never modified). PT: Um outline mutável minúsculo: um ponto NA x_height
    //     (tocado), um abaixo dele (não-tocado). X é arbitrário (nunca modificado).
    glx_sfnt_point pts[2];
    pts[0].x = 5; pts[0].y = 1000; pts[0].on_curve = 1; // at x_height
    pts[1].x = 5; pts[1].y = 500;  pts[1].on_curve = 1; // between baseline and x_height

    // EN: pre-hint: x_height point is off the pixel grid (10.5px). PT: pré-hint: ponto da x_height
    //     está fora da grade (10.5px).
    TEST_ASSERT(t_off_grid(t_scale(1000, 672, 1000)) != 0);

    glx_hint_result r = glx_hint_outline(pts, 2, 1000, 672, 1000, &z);
    TEST_ASSERT_EQ(r, GLX_HINT_OK);

    // EN: post-hint: the x_height point now scales to a WHOLE pixel, and to the expected 11px line.
    // PT: pós-hint: o ponto da x_height agora escala pra um pixel INTEIRO, e pra a linha esperada de
    //     11px.
    TEST_ASSERT_EQ(pts[0].y, 1048);
    TEST_ASSERT_EQ(t_off_grid(t_scale(pts[0].y, 672, 1000)), 0);
    TEST_ASSERT_EQ(t_scale(pts[0].y, 672, 1000), 704);

    // EN: X untouched. PT: X intocado.
    TEST_ASSERT_EQ(pts[0].x, 5);
    TEST_ASSERT_EQ(pts[1].x, 5);
}

// ================================================================================================
// EN: (2) MONOTONIC interpolation -- points in increasing Y stay in non-decreasing Y after hinting,
//     and a point between two zones never overshoots either snapped zone. Same 10.5px scale as (1).
// PT: (2) Interpolação MONOTÔNICA -- pontos em Y crescente ficam em Y não-decrescente após hinting,
//     e um ponto entre duas zonas nunca ultrapassa nenhuma das zonas snapadas. Mesma escala 10.5px
//     de (1).
// ================================================================================================
static void test_monotonic(void) {
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1000;
    z.cap_height = 1400;
    z.ascender = 1500;
    z.descender = -300;
    z.axis_flags = GLX_HINT_AXIS_Y;

    // EN: strictly increasing Y: baseline, mid-low, x_height, mid-high, cap_height, ascender.
    // PT: Y estritamente crescente: baseline, meio-baixo, x_height, meio-alto, cap_height, ascender.
    glx_sfnt_point pts[6];
    int32_t orig[6] = {0, 500, 1000, 1200, 1400, 1500};
    for (int i = 0; i < 6; i++) { pts[i].x = 0; pts[i].y = (int16_t)orig[i]; pts[i].on_curve = 1; }

    glx_hint_result r = glx_hint_outline(pts, 6, 1000, 672, 1000, &z);
    TEST_ASSERT_EQ(r, GLX_HINT_OK);

    // EN: order preserved (non-decreasing). PT: ordem preservada (não-decrescente).
    for (int i = 1; i < 6; i++) TEST_ASSERT(pts[i].y >= pts[i - 1].y);

    // EN: the untouched point originally between baseline(0) and x_height stays strictly between
    //     their hinted positions (baseline gridf=0, x_height gridf=1048). PT: o ponto não-tocado
    //     originalmente entre baseline(0) e x_height fica estritamente entre suas posições hintadas
    //     (baseline gridf=0, x_height gridf=1048).
    TEST_ASSERT(pts[1].y > pts[0].y && pts[1].y < pts[2].y);
    TEST_ASSERT_EQ(pts[0].y, 0);    // baseline snapped to itself (already on grid at 0px)
    TEST_ASSERT_EQ(pts[2].y, 1048); // x_height snapped (matches test 1)
}

// ================================================================================================
// EN: (3) IDEMPOTENCY -- applying twice equals applying once. First call returns OK and moves
//     points; the second call detects the already-fitted state and returns GLX_HINT_NOOP, leaving
//     the array byte-for-byte identical.
// PT: (3) IDEMPOTÊNCIA -- aplicar duas vezes igual aplicar uma. A primeira chamada retorna OK e move
//     pontos; a segunda detecta o estado já-fitado e retorna GLX_HINT_NOOP, deixando o array
//     byte-a-byte idêntico.
// ================================================================================================
static void test_idempotent(void) {
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1000;
    z.cap_height = 1400;
    z.ascender = 1500;
    z.descender = -300;
    z.axis_flags = GLX_HINT_AXIS_Y;

    glx_sfnt_point pts[6];
    int32_t orig[6] = {-200, 0, 500, 1000, 1400, 1500};
    for (int i = 0; i < 6; i++) { pts[i].x = 7; pts[i].y = (int16_t)orig[i]; pts[i].on_curve = 1; }

    glx_hint_result r1 = glx_hint_outline(pts, 6, 1000, 672, 1000, &z);
    TEST_ASSERT_EQ(r1, GLX_HINT_OK);

    int16_t after1[6];
    for (int i = 0; i < 6; i++) after1[i] = pts[i].y;

    glx_hint_result r2 = glx_hint_outline(pts, 6, 1000, 672, 1000, &z);
    TEST_ASSERT_EQ(r2, GLX_HINT_NOOP);
    for (int i = 0; i < 6; i++) TEST_ASSERT_EQ(pts[i].y, after1[i]);
}

// ================================================================================================
// EN: (4) ALREADY ON GRID -> NOOP. scale (num=640, den=1000) = a 10px em, at which upem=1000 maps
//     every integer-hundred zone to a whole pixel already (x_height 1000 -> 640 = 10px exactly, cap
//     1400 -> 896 = 14px, etc.). Nothing to snap -> GLX_HINT_NOOP, points untouched.
// PT: (4) JÁ NA GRADE -> NOOP. escala (num=640, den=1000) = um em de 10px, no qual upem=1000 mapeia
//     toda zona de centena-inteira pra um pixel inteiro já (x_height 1000 -> 640 = 10px exato, cap
//     1400 -> 896 = 14px, etc.). Nada pra snapar -> GLX_HINT_NOOP, pontos intocados.
// ================================================================================================
static void test_already_on_grid_noop(void) {
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1000;
    z.cap_height = 1400;
    z.ascender = 1500;
    z.descender = -300;
    z.axis_flags = GLX_HINT_AXIS_Y;

    glx_sfnt_point pts[3];
    pts[0].x = 1; pts[0].y = 1000; pts[0].on_curve = 1;
    pts[1].x = 2; pts[1].y = 1400; pts[1].on_curve = 1;
    pts[2].x = 3; pts[2].y = 700;  pts[2].on_curve = 1;
    int16_t snap[3] = {1000, 1400, 700};

    // EN: sanity: x_height already on grid at this scale. PT: sanidade: x_height já na grade nesta
    //     escala.
    TEST_ASSERT_EQ(t_off_grid(t_scale(1000, 640, 1000)), 0);

    glx_hint_result r = glx_hint_outline(pts, 3, 1000, 640, 1000, &z);
    TEST_ASSERT_EQ(r, GLX_HINT_NOOP);
    for (int i = 0; i < 3; i++) TEST_ASSERT_EQ(pts[i].y, snap[i]);
}

// ================================================================================================
// EN: (5) HOSTILE / DEGENERATE inputs -> clean NOOP or INVALID_ARG, NEVER UB, points untouched. A
//     firmware-grade edge sweep: NULL args, zero/negative scale, zero upem, empty outline, disabled
//     axis, and out-of-order zones (internally sorted, must not crash / must stay monotonic).
// PT: (5) Inputs HOSTIS / DEGENERADOS -> NOOP ou INVALID_ARG limpo, NUNCA UB, pontos intocados. Uma
//     varredura de borda grau-firmware: args NULL, escala zero/negativa, upem zero, outline vazio,
//     eixo desabilitado, e zonas fora de ordem (ordenadas internamente, não pode crashar / precisa
//     seguir monotônico).
// ================================================================================================
static void test_hostile(void) {
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1000;
    z.cap_height = 1400;
    z.ascender = 1500;
    z.descender = -300;
    z.axis_flags = GLX_HINT_AXIS_Y;

    glx_sfnt_point pts[2];
    pts[0].x = 5; pts[0].y = 1000; pts[0].on_curve = 1;
    pts[1].x = 5; pts[1].y = 500;  pts[1].on_curve = 1;

    // EN: hard argument errors. PT: erros fortes de argumento.
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 1000, 672, 1000, NULL), GLX_HINT_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(glx_hint_outline(NULL, 5, 1000, 672, 1000, &z), GLX_HINT_ERR_INVALID_ARG);

    // EN: NULL points with count 0 is NOT an error (nothing to point at) -> NOOP. PT: points NULL
    //     com contagem 0 NÃO é erro (nada pra apontar) -> NOOP.
    TEST_ASSERT_EQ(glx_hint_outline(NULL, 0, 1000, 672, 1000, &z), GLX_HINT_NOOP);

    // EN: degenerate scale / upem / count / axis -> NOOP, points unchanged. PT: escala / upem /
    //     contagem / eixo degenerados -> NOOP, pontos intocados.
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 1000, 672, 0, &z), GLX_HINT_NOOP);   // scale_den == 0
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 1000, 672, -1, &z), GLX_HINT_NOOP);  // scale_den < 0
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 1000, 0, 1000, &z), GLX_HINT_NOOP);  // scale_num == 0
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 0, 672, 1000, &z), GLX_HINT_NOOP);   // units_per_em == 0
    TEST_ASSERT_EQ(glx_hint_outline(pts, 0, 1000, 672, 1000, &z), GLX_HINT_NOOP); // num_points == 0
    TEST_ASSERT_EQ(pts[0].y, 1000);
    TEST_ASSERT_EQ(pts[1].y, 500);

    // EN: axis disabled -> NOOP even with otherwise-valid args. PT: eixo desabilitado -> NOOP mesmo
    //     com args válidos no resto.
    glx_hint_zones z_noaxis = z;
    z_noaxis.axis_flags = 0u;
    TEST_ASSERT_EQ(glx_hint_outline(pts, 2, 1000, 672, 1000, &z_noaxis), GLX_HINT_NOOP);
    TEST_ASSERT_EQ(pts[0].y, 1000);

    // EN: out-of-order / negative-heavy zones -> internally sorted, must produce a monotonic result
    //     without UB. PT: zonas fora de ordem / carregadas de negativo -> ordenadas internamente,
    //     precisa produzir resultado monotônico sem UB.
    glx_hint_zones z_scrambled;
    z_scrambled.baseline = 1500;   // deliberately not the smallest
    z_scrambled.x_height = -300;
    z_scrambled.cap_height = 0;
    z_scrambled.ascender = 1000;
    z_scrambled.descender = 1400;
    z_scrambled.axis_flags = GLX_HINT_AXIS_Y;
    glx_sfnt_point pts2[4];
    int32_t o2[4] = {-300, 0, 1000, 1500};
    for (int i = 0; i < 4; i++) { pts2[i].x = 0; pts2[i].y = (int16_t)o2[i]; pts2[i].on_curve = 1; }
    glx_hint_result rs = glx_hint_outline(pts2, 4, 1000, 672, 1000, &z_scrambled);
    TEST_ASSERT(rs == GLX_HINT_OK || rs == GLX_HINT_NOOP); // never INVALID_ARG, never a crash
    for (int i = 1; i < 4; i++) TEST_ASSERT(pts2[i].y >= pts2[i - 1].y); // still monotonic
}

// ================================================================================================
// EN: (6) REAL FONT ('O' from Open Sans) -- robustness + idempotency on genuine outline data.
//     Hint at a 16px em (upem=2048 for Open Sans; num = 16*64 = 1024, den = 2048) with hand-passed
//     plausible zones (zone auto-derivation is a future sub-phase, see hint.h). Asserts: the call
//     does not error; hinting is idempotent (a second pass is a NOOP leaving the array identical);
//     every point that ends up in a zone band scales to within 1/64 px of the whole pixel grid; and
//     the outline's X coordinates are untouched.
// PT: (6) FONTE REAL ('O' da Open Sans) -- robustez + idempotência em dado de outline genuíno.
//     Hinta num em de 16px (upem=2048 pra Open Sans; num = 16*64 = 1024, den = 2048) com zonas
//     plausíveis passadas à mão (auto-derivação de zona é uma sub-fase futura, ver hint.h). Asserta:
//     a chamada não dá erro; o hinting é idempotente (uma segunda passada é NOOP deixando o array
//     idêntico); todo ponto que termina numa banda de zona escala pra dentro de 1/64 px da grade de
//     pixel inteiro; e as coordenadas X do outline são intocadas.
// ================================================================================================
static void test_real_font_O(void) {
    const unsigned char* blob = _binary_opensans_regular_ttf_start;
    size_t len = (size_t)(_binary_opensans_regular_ttf_end - _binary_opensans_regular_ttf_start);

    glx_sfnt_face face;
    TEST_ASSERT_EQ(glx_sfnt_open(blob, len, &face), GLX_SFNT_OK);

    uint32_t gid = glx_sfnt_glyph_id(&face, 'O');
    TEST_ASSERT(gid != 0);

    glx_sfnt_point points[256];
    uint16_t contour_ends[8];
    glx_sfnt_outline out;
    TEST_ASSERT_EQ(glx_sfnt_glyph_outline(&face, gid, points, 256, contour_ends, 8, &out),
                   GLX_SFNT_OK);
    TEST_ASSERT(out.num_points > 0 && out.num_points <= 256);

    // EN: snapshot original X to prove it is never modified. PT: snapshot do X original pra provar
    //     que nunca é modificado.
    int16_t orig_x[256];
    for (uint16_t i = 0; i < out.num_points; i++) orig_x[i] = points[i].x;

    // EN: plausible Open Sans zones (font units, upem=2048). PT: zonas Open Sans plausíveis
    //     (unidades de fonte, upem=2048).
    glx_hint_zones z;
    z.baseline = 0;
    z.x_height = 1082;
    z.cap_height = 1462;
    z.ascender = 2189;
    z.descender = -600;
    z.axis_flags = GLX_HINT_AXIS_Y;

    const int32_t num = 1024, den = 2048; // 16px em

    glx_hint_result r1 = glx_hint_outline(points, out.num_points, 2048, num, den, &z);
    TEST_ASSERT(r1 == GLX_HINT_OK || r1 == GLX_HINT_NOOP); // real data, either is legitimate

    int16_t after1[256];
    for (uint16_t i = 0; i < out.num_points; i++) after1[i] = points[i].y;

    // EN: idempotency on real data -- second pass changes nothing. PT: idempotência em dado real --
    //     segunda passada não muda nada.
    glx_hint_result r2 = glx_hint_outline(points, out.num_points, 2048, num, den, &z);
    TEST_ASSERT_EQ(r2, GLX_HINT_NOOP);
    for (uint16_t i = 0; i < out.num_points; i++) TEST_ASSERT_EQ(points[i].y, after1[i]);

    // EN: every hinted point that sits in a zone band is on (within 1/64 px of) the pixel grid --
    //     the whole point of grid-fitting. `zdev` recomputed here for the 5 zones. PT: todo ponto
    //     hintado que fica numa banda de zona está na (a até 1/64 px da) grade de pixel -- o ponto
    //     inteiro do grid-fitting. `zdev` recomputado aqui pras 5 zonas.
    int32_t zf[5] = {z.descender, z.baseline, z.x_height, z.cap_height, z.ascender};
    for (uint16_t i = 0; i < out.num_points; i++) {
        int32_t pdev = t_scale(points[i].y, num, den);
        for (int k = 0; k < 5; k++) {
            int32_t zdev = t_scale(zf[k], num, den);
            int32_t d = pdev - zdev;
            if (d < 0) d = -d;
            if (d <= 32) { // in this zone's half-pixel band
                TEST_ASSERT(t_off_grid(pdev) <= 1); // on grid within one 26.6 unit (double-round)
                break;
            }
        }
    }

    // EN: X untouched. PT: X intocado.
    for (uint16_t i = 0; i < out.num_points; i++) TEST_ASSERT_EQ(points[i].x, orig_x[i]);
}

int main(void) {
    test_zone_lands_on_pixel();
    test_monotonic();
    test_idempotent();
    test_already_on_grid_noop();
    test_hostile();
    test_real_font_O();
    TEST_PASS();
}
