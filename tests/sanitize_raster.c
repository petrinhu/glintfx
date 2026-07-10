// SPDX-License-Identifier: MPL-2.0
// EN: SOV-RAST (FT-F2) SANITIZER GATE -- hosted-companion ASan/UBSan defense-in-depth, the exact
//     shape tests/sanitize_sfnt.c already established (see that file's own header for the full
//     rationale, and TESTES.md's TST-SFNT-SAN entry, which named this reuse in advance: "SOV-RAST
//     ... deve crescer um sanitize-rast irmão na mesma forma"). `src/raster.c` (and `src/sfnt.c`,
//     linked alongside it here to exercise the real-glyph path) are UNCHANGED -- no `#ifdef`, no
//     sanitizer-only code path -- both compile hosted with zero modification because neither
//     `#include`s a freestanding-internal symbol (`include/core/raster.h`'s own file header, "WHY
//     <stdint.h>/<stddef.h>, NOT include/types.h").
//
//     WHY THIS FILE MATTERS SPECIFICALLY FOR SOV-RAST: this ticket's brief calls a rasterizer's
//     "aritmética de ponteiro/índice pesada" out by name as an especially valuable sanitizer
//     target -- `src/raster.c`'s `accum_row_segment`/`accum_column` do a LOT of float division,
//     column-index arithmetic, and array indexing into the caller's `scratch` buffer, exactly the
//     class of code where an off-by-one or a wrong clamp produces a silent out-of-bounds write
//     under `make test`'s freestanding, non-sanitized harness (no crash, just corrupted
//     neighbouring scratch data, an artifact that could easily be misread as "flattening
//     imprecision" instead of "memory corruption" by a purely analytic pixel-value assertion).
//     ASan is what turns that class of bug into an immediate, loud, exact-location abort instead
//     of quiet data corruption `tests/test_raster.c`'s tolerance-based oracle assertions could
//     miss entirely (a wrong value within tolerance passes either way; ASan does not care about
//     tolerance, only about whether the memory access itself was in-bounds).
//
//     COVERAGE: the same 6 scenarios `tests/test_raster.c` already covers (rectangle, triangle,
//     quadratic curve, overflow-saturation, invalid/degenerate args, real Open Sans 'O' glyph via
//     `glx_sfnt_open`/`glx_sfnt_glyph_id`/`glx_sfnt_glyph_outline`) -- NOT re-derived/re-asserted
//     with the same tight analytic tolerances here (that is `tests/test_raster.c`'s job, the
//     freestanding functional oracle); this file's job is narrower and different: run the exact
//     same INPUTS through the exact same `src/raster.c` under ASan/UBSan and confirm the
//     sanitizers stay SILENT (no OOB read/write, no UB) end to end, with only loose PASS/FAIL
//     sanity checks (`CHECK`, mirroring sanitize_sfnt.c's own shape) on top -- a duplicate,
//     independent, standalone hosted program with no dependency on the freestanding harness or
//     `include/test.h` (same reasoning `tests/sanitize_sfnt.c`'s own header already gives).
// PT: PORTÃO SANITIZER do SOV-RAST (FT-F2) -- defesa-em-profundidade hosted-companion ASan/UBSan,
//     exatamente a forma que o tests/sanitize_sfnt.c já estabeleceu (ver o cabeçalho próprio
//     daquele arquivo pro racional completo, e a entrada TST-SFNT-SAN do TESTES.md, que já nomeou
//     esse reuso de antemão: "SOV-RAST ... deve crescer um sanitize-rast irmão na mesma forma").
//     `src/raster.c` (e `src/sfnt.c`, linkado junto aqui pra exercitar o caminho de glyph real)
//     ficam INALTERADOS -- sem `#ifdef`, sem caminho de código só-de-sanitizer -- os dois compilam
//     hosted com zero modificação porque nenhum `#include`ia símbolo interno-freestanding
//     (cabeçalho de arquivo próprio do include/core/raster.h, "POR QUE <stdint.h>/<stddef.h>, NÃO
//     include/types.h").
//
//     POR QUE ESTE ARQUIVO IMPORTA ESPECIFICAMENTE PRO SOV-RAST: o brief desta tarefa nomeia por
//     nome a "aritmética de ponteiro/índice pesada" de um rasterizador como um alvo de sanitizer
//     especialmente valioso -- o `accum_row_segment`/`accum_column` do `src/raster.c` fazem MUITA
//     divisão float, aritmética de índice-de-coluna, e indexação de array no buffer `scratch` de
//     quem chama, exatamente a classe de código onde um off-by-one ou um clamp errado produz uma
//     escrita fora-de-limite silenciosa sob o harness freestanding, não-sanitizado, do `make test`
//     (sem crash, só dado de scratch vizinho corrompido, um artefato que poderia facilmente ser
//     lido errado como "imprecisão de flattening" em vez de "corrupção de memória" por uma
//     asserção de valor de pixel puramente analítica). ASan é o que transforma essa classe de bug
//     num abort imediato, alto, de localização exata em vez de corrupção de dado silenciosa que as
//     asserções de oráculo baseadas em tolerância do `tests/test_raster.c` poderiam perder
//     inteiramente (um valor errado dentro da tolerância passa de qualquer jeito; ASan não liga
//     pra tolerância, só pra se o próprio acesso de memória estava dentro dos limites).
//
//     COBERTURA: os mesmos 6 cenários que o tests/test_raster.c já cobre (retângulo, triângulo,
//     curva quadrática, saturação-de-overflow, argumentos inválidos/degenerados, glyph 'O' real da
//     Open Sans via `glx_sfnt_open`/`glx_sfnt_glyph_id`/`glx_sfnt_glyph_outline`) -- NÃO
//     re-derivados/re-asserted com as mesmas tolerâncias analíticas apertadas aqui (esse é o
//     trabalho do tests/test_raster.c, o oráculo funcional freestanding); o trabalho deste arquivo
//     é mais estreito e diferente: rodar os MESMOS INPUTS através do MESMO src/raster.c sob
//     ASan/UBSan e confirmar que os sanitizers ficam CALADOS de ponta a ponta (sem leitura/escrita
//     fora de limite, sem UB), com só checagens de sanidade PASS/FAIL frouxas (`CHECK`, espelhando
//     a própria forma do sanitize_sfnt.c) em cima -- um programa hosted duplicado, independente,
//     autônomo, sem dependência do harness freestanding ou do `include/test.h` (mesmo raciocínio
//     que o cabeçalho próprio do tests/sanitize_sfnt.c já dá).
// Copyright (c) 2026 Petrus Silva Costa
#include "core/raster.h"
#include "core/sfnt.h"

#include <stdio.h>
#include <stdlib.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                                        \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));                     \
            g_failures++;                                                                        \
        }                                                                                         \
    } while (0)

static float g_scratch[13000];

static unsigned char* read_whole_file(const char* path, size_t* len_out) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) {
        free(buf);
        return NULL;
    }
    *len_out = (size_t)sz;
    return buf;
}

int main(void) {
    // ============================================================================================
    // ---- (1) rectangle, half-pixel boundaries --------------------------------------------------
    // ============================================================================================
    {
        static const glx_sfnt_point pts[4] = {
            {5, 5, 1}, {21, 5, 1}, {21, 15, 1}, {5, 15, 1},
        };
        static const uint16_t ends[1] = {3};
        const int W = 14, H = 11;
        static unsigned char bmp[14 * 11];
        size_t need = glx_raster_scratch_floats(W, H);
        glx_raster_result r = glx_rasterize_outline(pts, 4, ends, 1, 32, 1, 0, 640, bmp, W, H, W,
                                                     g_scratch, need);
        CHECK(r == GLX_RASTER_OK, "rasterize(rectangle) != OK");
        CHECK(bmp[4 * W + 5] == 255, "rectangle interior pixel not fully covered");
    }

    // ============================================================================================
    // ---- (2) triangle (exercises the x<=0 leading-cover path + multi-column spans) ------------
    // ============================================================================================
    {
        static const glx_sfnt_point pts[3] = {{0, 8, 1}, {8, 8, 1}, {0, 0, 1}};
        static const uint16_t ends[1] = {2};
        const int W = 10, H = 10;
        static unsigned char bmp[10 * 10];
        size_t need = glx_raster_scratch_floats(W, H);
        glx_raster_result r =
            glx_rasterize_outline(pts, 3, ends, 1, 64, 1, 0, 512, bmp, W, H, W, g_scratch, need);
        CHECK(r == GLX_RASTER_OK, "rasterize(triangle) != OK");
    }

    // ============================================================================================
    // ---- (3) quadratic curve (exercises flatten_quad's recursive de Casteljau path) -----------
    // ============================================================================================
    {
        static const glx_sfnt_point pts[3] = {{2, 8, 1}, {6, 2, 0}, {10, 8, 1}};
        static const uint16_t ends[1] = {2};
        const int W = 14, H = 12;
        static unsigned char bmp[14 * 12];
        size_t need = glx_raster_scratch_floats(W, H);
        glx_raster_result r =
            glx_rasterize_outline(pts, 3, ends, 1, 64, 1, 0, 640, bmp, W, H, W, g_scratch, need);
        CHECK(r == GLX_RASTER_OK, "rasterize(quadratic) != OK");
    }

    // ============================================================================================
    // ---- (4) overflow saturation (int64 intermediate + clamp path) -----------------------------
    // ============================================================================================
    {
        static const glx_sfnt_point pts[3] = {{10, 10, 1}, {20, 10, 1}, {10, 20, 1}};
        static const uint16_t ends[1] = {2};
        const int W = 8, H = 8;
        static unsigned char bmp[8 * 8];
        size_t need = glx_raster_scratch_floats(W, H);
        glx_raster_result r = glx_rasterize_outline(pts, 3, ends, 1, 2000000000, 1, 0, 0, bmp, W,
                                                     H, W, g_scratch, need);
        CHECK(r == GLX_RASTER_OK, "rasterize(overflow saturation) != OK");
    }

    // ============================================================================================
    // ---- (5) degenerate/invalid inputs -----------------------------------------------------------
    // ============================================================================================
    {
        const int W = 4, H = 4;
        static unsigned char bmp[4 * 4];
        size_t need = glx_raster_scratch_floats(W, H);

        glx_raster_result r =
            glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, W, H, W, g_scratch, need);
        CHECK(r == GLX_RASTER_OK, "rasterize(empty glyph) != OK");

        static const glx_sfnt_point one_pt[1] = {{1, 1, 1}};
        static const uint16_t one_end[1] = {0};
        r = glx_rasterize_outline(one_pt, 1, one_end, 1, 64, 1, 0, 0, bmp, W, H, W, g_scratch,
                                   need);
        CHECK(r == GLX_RASTER_OK, "rasterize(1-point contour) != OK");

        r = glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, W, H, W, g_scratch,
                                   need - 1);
        CHECK(r == GLX_RASTER_ERR_SCRATCH_TOO_SMALL, "rasterize(undersized scratch) wrong result");
    }

    // ============================================================================================
    // ---- (6) real Open Sans 'O' glyph -- full pipeline: parser + flattening + rasterization ----
    // ============================================================================================
    {
        size_t len = 0;
        unsigned char* blob = read_whole_file("tests/fixtures/opensans_regular.ttf", &len);
        CHECK(blob != NULL, "could not read tests/fixtures/opensans_regular.ttf (run from repo root)");
        if (blob != NULL) {
            glx_sfnt_face face;
            glx_sfnt_result sr = glx_sfnt_open(blob, len, &face);
            CHECK(sr == GLX_SFNT_OK, "glx_sfnt_open(Open Sans) != OK");
            if (sr == GLX_SFNT_OK) {
                uint32_t gid = glx_sfnt_glyph_id(&face, 'O');
                CHECK(gid != 0, "glyph_id('O') == .notdef");

                glx_sfnt_point points[256];
                uint16_t contour_ends[8];
                glx_sfnt_outline out;
                sr = glx_sfnt_glyph_outline(&face, gid, points, 256, contour_ends, 8, &out);
                CHECK(sr == GLX_SFNT_OK, "outline('O') != OK");

                if (sr == GLX_SFNT_OK) {
                    int32_t scale_num = 48 * 64;
                    int32_t scale_den = (int32_t)face.units_per_em;
                    int32_t margin_fx = 16 * 64;
                    int32_t origin_x =
                        margin_fx - (int32_t)(((int64_t)out.x_min * scale_num) / scale_den);
                    int32_t origin_y =
                        margin_fx + (int32_t)(((int64_t)out.y_max * scale_num) / scale_den);

                    const int W = 80, H = 80;
                    static unsigned char bmp[80 * 80];
                    size_t need = glx_raster_scratch_floats(W, H);
                    glx_raster_result r = glx_rasterize_outline(
                        points, out.num_points, contour_ends, out.num_contours, scale_num,
                        scale_den, origin_x, origin_y, bmp, W, H, W, g_scratch, need);
                    CHECK(r == GLX_RASTER_OK, "rasterize('O') != OK");
                }
            }
            free(blob);
        }
    }

    if (g_failures == 0) {
        fprintf(stderr, "sanitize_raster: PASS (0 assertion failures, ASan/UBSan silent)\n");
        return 0;
    }
    fprintf(stderr, "sanitize_raster: FAIL (%d assertion failure(s))\n", g_failures);
    return 1;
}
