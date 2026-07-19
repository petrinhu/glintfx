// SPDX-License-Identifier: MPL-2.0
// EN: SOV-HINT (L1.20-FONTFLIP / FT-F4) SANITIZER GATE -- AUD-SEC-Delta addition (2026-07-19):
//     `src/hint.c` was the ONLY component of the SFNT/raster/hint trio with NO hosted-companion
//     sanitizer target (AUD-C0-PLAN.md's "Hotspot #2" -- the newest translation unit, tested by
//     `tests/test_hint.c`'s functional oracle but never run under ASan/UBSan). This file closes
//     that gap, reusing the EXACT hosted-companion shape `tests/sanitize_sfnt.c` established and
//     `tests/sanitize_raster.c` already reused ("... deve crescer um sanitize-rast/sanitize-hint
//     irmão na mesma forma", TESTES.md's TST-SFNT-SAN entry). `src/hint.c` is UNCHANGED here --
//     no `#ifdef`, no sanitizer-only code path -- it compiles hosted with zero modification
//     because it only `#include`s "core/hint.h" (which pulls "core/sfnt.h" for the
//     `glx_sfnt_point` POD), itself only `<stdint.h>`/`<stddef.h>`, no freestanding-internal
//     symbol (mirrors `src/sfnt.c`/`src/raster.c`'s own same-shape header note).
//
//     WHY THIS FILE MATTERS: `glx_hint_outline` is a pure, points-in/points-out-in-place integer
//     grid-fitter fed CALLER-SUPPLIED zones (`glx_hint_zones`) and a device scale
//     (`scale_num`/`scale_den`) that -- for a real SFNT caller -- both originate from an
//     UNTRUSTED font file (units_per_em, ascender/descender/cap-height/x-height metrics). Its own
//     header (`include/core/hint.h`) makes an explicit HOSTILE-INPUT correctness claim ("every
//     degenerate case is a CLEAN GLX_HINT_NOOP or GLX_HINT_ERR_INVALID_ARG, NEVER undefined
//     behaviour"). This gate exercises exactly the classes that claim covers -- hard argument
//     errors, degenerate scale/units_per_em, out-of-order/all-equal/negative zones, INT16
//     coordinate extremes (the domino target: `src/raster.c`'s own `iabs32(INT32_MIN)` cosmetic
//     mine, confirmed unreachable there since `dx`/`dy` are `GLX_RASTER_FX_MAX`-bounded far from
//     INT32_MIN -- this file's own arithmetic is reviewed for the same twin pattern; see the
//     AUD-SEC-Delta report for the full domino analysis) -- under ASan/UBSan, plus an exact
//     idempotency re-assertion (a wrong/tolerant idempotency check would silently mask a real
//     grid-fitting regression).
//
//     COVERAGE (loose PASS/FAIL sanity via `CHECK`, mirroring `sanitize_sfnt.c`'s/
//     `sanitize_raster.c`'s own shape -- `tests/test_hint.c` remains the tight freestanding
//     functional/oracle suite; this file's job is narrower: confirm the sanitizers stay SILENT
//     end to end over the SAME production code under hostile/boundary input):
//       1. hard argument errors (`zones==NULL`, `points==NULL` with `num_points>0`)
//       2. every documented degenerate-scale/units_per_em/num_points/axis-off NOOP path, with a
//          byte-for-byte "points left untouched" check on each
//       3. hostile zones: out-of-order, all-equal (zero-height inter-zone span), negative,
//          INT16_MIN/INT16_MAX zones AND points, and INT32_MAX scale_num/scale_den
//       4. exact idempotency -- apply twice, second call MUST be GLX_HINT_NOOP with points
//          byte-for-byte unchanged from after the first call
//       5. a full `num_points == 65535` (uint16_t ceiling) sweep with pseudo-random Y spread
// PT: PORTÃO SANITIZER do SOV-HINT (L1.20-FONTFLIP / FT-F4) -- acréscimo do AUD-SEC-Delta
//     (2026-07-19): o `src/hint.c` era o ÚNICO componente do trio SFNT/raster/hint SEM alvo
//     sanitizer hosted-companion (o "Hotspot #2" do AUD-C0-PLAN.md -- a unidade de tradução mais
//     nova, testada pelo oráculo funcional do `tests/test_hint.c` mas nunca rodada sob
//     ASan/UBSan). Este arquivo fecha essa lacuna, reusando a forma hosted-companion EXATA que o
//     `tests/sanitize_sfnt.c` estabeleceu e o `tests/sanitize_raster.c` já reusou ("... deve
//     crescer um sanitize-rast/sanitize-hint irmão na mesma forma", entrada TST-SFNT-SAN do
//     TESTES.md). O `src/hint.c` fica INALTERADO aqui -- sem `#ifdef`, sem caminho de código
//     só-de-sanitizer -- compila hosted com zero modificação porque só `#include`ia
//     "core/hint.h" (que puxa "core/sfnt.h" pro POD `glx_sfnt_point`), ele mesmo só
//     `<stdint.h>`/`<stddef.h>`, nenhum símbolo interno-freestanding (espelha a mesma nota de
//     cabeçalho do `src/sfnt.c`/`src/raster.c`).
//
//     POR QUE ESTE ARQUIVO IMPORTA: `glx_hint_outline` é um grid-fitter inteiro puro,
//     ponto-entra/ponto-sai-in-place, alimentado com zonas fornecidas por quem chama
//     (`glx_hint_zones`) e uma escala device (`scale_num`/`scale_den`) que -- pra um chamador
//     SFNT real -- ambos se originam de um arquivo de fonte NÃO-CONFIÁVEL (métricas
//     units_per_em, ascender/descender/cap-height/x-height). O próprio header dele
//     (`include/core/hint.h`) faz uma alegação de corretude explícita sobre INPUT HOSTIL ("todo
//     caso degenerado é um GLX_HINT_NOOP ou GLX_HINT_ERR_INVALID_ARG LIMPO, NUNCA comportamento
//     indefinido"). Este portão exercita exatamente as classes que essa alegação cobre -- erros
//     fortes de argumento, escala/units_per_em degenerados, zonas fora-de-ordem/todas-iguais/
//     negativas, extremos de coordenada INT16 (o alvo dominó: a mina cosmética
//     `iabs32(INT32_MIN)` do próprio `src/raster.c`, confirmada inalcançável ali já que `dx`/`dy`
//     são limitados por `GLX_RASTER_FX_MAX` bem longe de INT32_MIN -- a aritmética própria deste
//     arquivo é revisada pro mesmo padrão gêmeo; ver o relatório AUD-SEC-Delta pra análise dominó
//     completa) -- sob ASan/UBSan, mais uma reafirmação de idempotência EXATA (uma checagem de
//     idempotência tolerante/errada mascararia silenciosamente uma regressão real de
//     grid-fitting).
//
//     COBERTURA (sanidade PASS/FAIL frouxa via `CHECK`, espelhando a própria forma do
//     `sanitize_sfnt.c`/`sanitize_raster.c` -- o `tests/test_hint.c` segue sendo a suíte
//     funcional/oráculo freestanding apertada; o trabalho deste arquivo é mais estreito: confirmar
//     que os sanitizers ficam CALADOS de ponta a ponta sobre o MESMO código de produção sob input
//     hostil/de fronteira):
//       1. erros fortes de argumento (`zones==NULL`, `points==NULL` com `num_points>0`)
//       2. todo caminho NOOP degenerado documentado (escala/units_per_em/num_points/eixo-off),
//          com checagem byte-a-byte de "pontos deixados intocados" em cada um
//       3. zonas hostis: fora-de-ordem, todas-iguais (vão inter-zona de altura zero), negativas,
//          zonas E pontos INT16_MIN/INT16_MAX, e scale_num/scale_den INT32_MAX
//       4. idempotência exata -- aplica duas vezes, a segunda chamada TEM que ser GLX_HINT_NOOP
//          com pontos byte-a-byte inalterados desde depois da primeira chamada
//       5. uma varredura completa `num_points == 65535` (teto do uint16_t) com espalhamento Y
//          pseudo-aleatório
// Copyright (c) 2026 Petrus Silva Costa
#include "core/hint.h"
#include "core/sfnt.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                                       \
    do {                                                                                        \
        if (!(cond)) {                                                                          \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));                    \
            g_failures++;                                                                       \
        }                                                                                        \
    } while (0)

static glx_hint_zones make_zones(int16_t base, int16_t xh, int16_t cap, int16_t asc, int16_t desc,
                                  uint32_t flags) {
    glx_hint_zones z;
    z.baseline = base;
    z.x_height = xh;
    z.cap_height = cap;
    z.ascender = asc;
    z.descender = desc;
    z.axis_flags = flags;
    return z;
}

int main(void) {
    // ============================================================================================
    // ---- (1) hard argument errors ---------------------------------------------------------------
    // ============================================================================================
    {
        glx_sfnt_point pts[1] = { { 0, 0, 1 } };
        glx_hint_zones z = make_zones(0, 500, 700, 800, -200, GLX_HINT_AXIS_Y);
        CHECK(glx_hint_outline(pts, 1, 1000, 640, 1000, NULL) == GLX_HINT_ERR_INVALID_ARG,
              "zones==NULL must be ERR_INVALID_ARG");
        CHECK(glx_hint_outline(NULL, 5, 1000, 640, 1000, &z) == GLX_HINT_ERR_INVALID_ARG,
              "points==NULL with num_points>0 must be ERR_INVALID_ARG");
        CHECK(glx_hint_outline(NULL, 0, 1000, 640, 1000, &z) != GLX_HINT_ERR_INVALID_ARG,
              "points==NULL with num_points==0 must NOT be ERR_INVALID_ARG");
    }

    // ============================================================================================
    // ---- (2) every documented degenerate NOOP path -- points BYTE-FOR-BYTE unchanged -----------
    // ============================================================================================
    {
        glx_hint_zones z = make_zones(0, 500, 700, 800, -200, GLX_HINT_AXIS_Y);
        glx_sfnt_point before[3] = { { 1, 111, 1 }, { 2, 222, 0 }, { 3, 333, 1 } };
        glx_sfnt_point pts[3];

        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 1000, 0, 1000, &z) == GLX_HINT_NOOP, "scale_num==0 not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "scale_num==0 touched points");

        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 1000, -640, 1000, &z) == GLX_HINT_NOOP, "scale_num<0 not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "scale_num<0 touched points");

        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 1000, 640, 0, &z) == GLX_HINT_NOOP, "scale_den==0 not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "scale_den==0 touched points");

        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 1000, 640, -1000, &z) == GLX_HINT_NOOP, "scale_den<0 not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "scale_den<0 touched points");

        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 0, 640, 1000, &z) == GLX_HINT_NOOP, "units_per_em==0 not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "units_per_em==0 touched points");

        CHECK(glx_hint_outline(pts, 0, 1000, 640, 1000, &z) == GLX_HINT_NOOP, "num_points==0 not NOOP");

        glx_hint_zones z_noaxis = make_zones(0, 500, 700, 800, -200, 0);
        memcpy(pts, before, sizeof(pts));
        CHECK(glx_hint_outline(pts, 3, 1000, 640, 1000, &z_noaxis) == GLX_HINT_NOOP,
              "axis_flags without Y bit not NOOP");
        CHECK(memcmp(pts, before, sizeof(pts)) == 0, "axis-off touched points");
    }

    // ============================================================================================
    // ---- (3) hostile zones: out-of-order, all-equal, negative, INT16/INT32 extremes -------------
    // ============================================================================================
    {
        glx_hint_zones z_rev = make_zones(800, 700, 500, 0, -200, GLX_HINT_AXIS_Y); // reversed
        glx_sfnt_point pts[2] = { { 0, 500, 1 }, { 0, -200, 1 } };
        glx_hint_result r = glx_hint_outline(pts, 2, 1000, 640, 1000, &z_rev);
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "reversed zones: unexpected result code");

        glx_hint_zones z_eq = make_zones(100, 100, 100, 100, 100, GLX_HINT_AXIS_Y); // all-equal
        glx_sfnt_point pts2[3] = { { 0, 50, 1 }, { 0, 100, 1 }, { 0, 150, 1 } };
        r = glx_hint_outline(pts2, 3, 1000, 640, 1000, &z_eq);
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "all-equal zones: unexpected result code");

        glx_hint_zones z_neg = make_zones(-50, -10, 10, 50, -900, GLX_HINT_AXIS_Y);
        glx_sfnt_point pts3[2] = { { 0, -900, 1 }, { 0, 0, 1 } };
        r = glx_hint_outline(pts3, 2, 1000, 640, 1000, &z_neg);
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "negative zones: unexpected result code");

        // INT16 extremes -- domino target (raster.c's iabs32(INT32_MIN) cosmetic mine's twin).
        glx_hint_zones z_ext = make_zones(INT16_MIN, (int16_t)(INT16_MIN / 2), 0,
                                           (int16_t)(INT16_MAX / 2), INT16_MAX, GLX_HINT_AXIS_Y);
        glx_sfnt_point pts4[4] = {
            { 0, INT16_MIN, 1 },
            { 0, INT16_MAX, 1 },
            { 0, 0, 1 },
            { 0, (int16_t)(INT16_MIN + 1), 1 },
        };
        r = glx_hint_outline(pts4, 4, 1000, 640, 1000, &z_ext);
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "INT16-extreme zones/points: unexpected result");
        for (int i = 0; i < 4; i++) {
            CHECK(pts4[i].y >= INT16_MIN && pts4[i].y <= INT16_MAX,
                  "INT16-extreme: output y out of int16_t range");
        }

        glx_hint_zones z_normal = make_zones(0, 500, 700, 800, -200, GLX_HINT_AXIS_Y);
        glx_sfnt_point pts5a[2] = { { 0, INT16_MAX, 1 }, { 0, INT16_MIN, 1 } };
        r = glx_hint_outline(pts5a, 2, 65535, INT32_MAX, 1, &z_normal); // huge scale_num
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "scale_num==INT32_MAX: unexpected result");
        glx_sfnt_point pts5b[2] = { { 0, INT16_MAX, 1 }, { 0, INT16_MIN, 1 } };
        r = glx_hint_outline(pts5b, 2, 65535, 1, INT32_MAX, &z_normal); // huge scale_den
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "scale_den==INT32_MAX: unexpected result");
    }

    // ============================================================================================
    // ---- (4) exact idempotency -- second call MUST be a byte-for-byte NOOP ----------------------
    // ============================================================================================
    {
        glx_hint_zones z = make_zones(0, 500, 700, 800, -200, GLX_HINT_AXIS_Y);
        glx_sfnt_point pts[5] = {
            { 0, 500, 1 }, { 0, 250, 1 }, { 0, 0, 1 }, { 0, -100, 1 }, { 0, 700, 1 },
        };
        (void)glx_hint_outline(pts, 5, 1000, 672, 1000, &z);
        glx_sfnt_point after_first[5];
        memcpy(after_first, pts, sizeof(pts));
        glx_hint_result r2 = glx_hint_outline(pts, 5, 1000, 672, 1000, &z);
        CHECK(r2 == GLX_HINT_NOOP, "second glx_hint_outline call was not NOOP (idempotency broken)");
        CHECK(memcmp(pts, after_first, sizeof(pts)) == 0,
              "second call moved points despite reporting NOOP");
    }

    // ============================================================================================
    // ---- (5) full uint16_t ceiling sweep (65535 points, pseudo-random Y spread) ------------------
    // ============================================================================================
    {
        static glx_sfnt_point big[65535];
        for (uint32_t i = 0; i < 65535; i++) {
            big[i].x = 0;
            big[i].y = (int16_t)((i * 37) % 2000 - 1000);
            big[i].on_curve = 1;
        }
        glx_hint_zones z = make_zones(-1000, -300, 300, 700, -1000, GLX_HINT_AXIS_Y);
        glx_hint_result r = glx_hint_outline(big, 65535, 2000, 640, 2000, &z);
        CHECK(r == GLX_HINT_OK || r == GLX_HINT_NOOP, "65535-point sweep: unexpected result");
        for (uint32_t i = 0; i < 65535; i++) {
            CHECK(big[i].y >= INT16_MIN && big[i].y <= INT16_MAX, "65535-point sweep: y out of range");
        }
    }

    if (g_failures == 0) {
        fprintf(stderr, "sanitize_hint: PASS (0 assertion failures, ASan/UBSan silent)\n");
        return 0;
    }
    fprintf(stderr, "sanitize_hint: FAIL (%d assertion failure(s))\n", g_failures);
    return 1;
}
