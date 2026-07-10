// SPDX-License-Identifier: MPL-2.0
// EN: SOV-RAST (FT-F2) -- functional/oracle test suite (C1 harness, freestanding). See
//     include/core/raster.h and src/raster.c for the full contract/algorithm; this file is the
//     ANALYTIC ORACLE this ticket's brief mandates ("golden pixel-exato é flaky, PROIBIDO como
//     gate") -- every expected value below is derived BY HAND from closed-form geometry (Green's
//     theorem / elementary area formulas), documented inline with the derivation, never a binary
//     golden fixture.
//
//     WHY NO DIRECT UNIT TEST OF `flatten_quad`/`accum_row_segment` (this ticket's brief also asks
//     for "teste unitário do flattening"): every helper in src/raster.c is `static` (internal
//     linkage, same posture src/sfnt.c already established -- see that file's own header) --
//     unreachable from a different translation unit. Per this codebase's existing precedent
//     (tests/test_sfnt.c only ever calls the 4 public `glx_sfnt_*` entry points, never `rd_u16`/
//     `parse_simple_glyph`/etc. directly), this file tests the flattening algorithm INDIRECTLY but
//     EXACTLY, through the public `glx_rasterize_outline` entry point: `test_quadratic_curve_area`
//     feeds a single hand-built quadratic contour and asserts the rasterized bitmap's total
//     coverage matches a closed-form Green's-theorem area formula (derived from first principles
//     right below, self-contained, independent of src/raster.c's own header comment) to within a
//     tight tolerance -- this is a STRONGER proof of flattening correctness than checking a list
//     of expected intermediate points would be (an exact area match, integrated over the WHOLE
//     curve, cannot pass by accident the way an isolated point-position check could if two
//     unrelated bugs happened to cancel at that one sampled point).
//
//     THE GREEN'S THEOREM DERIVATION (self-derived, public undergraduate-level computational
//     geometry -- see include/core/raster.h's CLEAN-ROOM note for why this matters): for a
//     quadratic Bezier B(t) = (1-t)^2*P0 + 2t(1-t)*P1 + t^2*P2, `t` in `[0,1]`, closed by a
//     STRAIGHT line back from P2 to P0, the enclosed signed area (via `(1/2) * oint(x dy - y dx)`)
//     works out to EXACTLY `K/3` where `K = cross(P1-P0, P2-P0)` (`cross(u,v) = ux*vy - uy*vx`,
//     the standard 2D cross product). Proof sketch: translate so `P0` is the origin (translation-
//     invariant since we integrate over the WHOLE closed contour); the straight closing chord from
//     `P2` to the origin then contributes EXACTLY ZERO to `oint(x dy - y dx)` (any line segment
//     through the origin does -- `(x,y)` stays parallel to `(dx,dy)` along it, so their cross
//     product/integrand is 0 pointwise); substituting `B(t)`'s own `x(t)`/`y(t)`/`dx`/`dy` into
//     the curve's own share of the integral and simplifying (the algebra cancels down to a single
//     `K*t^2` term, `K` constant) leaves `(1/2) * integral_0^1 2*K*t^2 dt = (1/2)*(2K/3) = K/3`.
//     `test_quadratic_curve_area` below picks concrete `P0`/`P1`/`P2` and states the resulting `K/3`
//     with the arithmetic spelled out.
// PT: SOV-RAST (FT-F2) -- suíte de teste funcional/oráculo (harness C1, freestanding). Ver
//     include/core/raster.h e src/raster.c pro contrato/algoritmo completo; este arquivo é o
//     ORÁCULO ANALÍTICO que o brief desta tarefa manda ("golden pixel-exato é flaky, PROIBIDO como
//     gate") -- todo valor esperado abaixo é derivado À MÃO de geometria de forma fechada (teorema
//     de Green / fórmulas de área elementares), documentado inline com a derivação, nunca uma
//     fixture golden binária.
//
//     POR QUE NENHUM TESTE UNITÁRIO DIRETO DO `flatten_quad`/`accum_row_segment` (o brief desta
//     tarefa também pede "teste unitário do flattening"): todo helper do src/raster.c é `static`
//     (linkage interno, mesma postura que o src/sfnt.c já estabeleceu -- ver o cabeçalho próprio
//     daquele arquivo) -- inalcançável de uma unidade de tradução diferente. Conforme o precedente
//     já existente deste código-base (o tests/test_sfnt.c só chama os 4 pontos de entrada públicos
//     `glx_sfnt_*`, nunca `rd_u16`/`parse_simple_glyph`/etc. diretamente), este arquivo testa o
//     algoritmo de flattening de forma INDIRETA mas EXATA, através do ponto de entrada público
//     `glx_rasterize_outline`: o `test_quadratic_curve_area` alimenta um único contorno quadrático
//     construído à mão e asserta que a cobertura total do bitmap rasterizado bate com uma fórmula
//     de área de forma fechada via teorema de Green (derivada do zero logo abaixo, autocontida,
//     independente do comentário de cabeçalho próprio do src/raster.c) dentro de uma tolerância
//     apertada -- isto é uma prova MAIS FORTE de corretude de flattening do que checar uma lista
//     de pontos intermediários esperados seria (um casamento exato de área, integrado sobre a
//     curva INTEIRA, não consegue passar por acidente do jeito que uma checagem isolada de posição
//     de ponto poderia se dois bugs não-relacionados por acaso se cancelassem naquele ponto
//     amostrado).
//
//     A DERIVAÇÃO DO TEOREMA DE GREEN (auto-derivada, geometria computacional de nível de
//     graduação pública -- ver a nota CLEAN-ROOM do include/core/raster.h pro porquê disso
//     importar): pra uma Bezier quadrática B(t) = (1-t)^2*P0 + 2t(1-t)*P1 + t^2*P2, `t` em `[0,1]`,
//     fechada por uma reta de volta de P2 pra P0, a área com sinal englobada (via `(1/2) *
//     oint(x dy - y dx)`) dá EXATAMENTE `K/3` onde `K = cross(P1-P0, P2-P0)` (`cross(u,v) =
//     ux*vy - uy*vx`, o produto vetorial 2D padrão). Esboço de prova: translada pra `P0` ser a
//     origem (invariante por translação já que integramos sobre o contorno FECHADO inteiro); a
//     corda reta de fechamento de `P2` pra origem então contribui EXATAMENTE ZERO pro
//     `oint(x dy - y dx)` (qualquer segmento de reta através da origem contribui -- `(x,y)`
//     permanece paralelo a `(dx,dy)` ao longo dele, então o produto vetorial/integrando deles é 0
//     pontualmente); substituindo o próprio `x(t)`/`y(t)`/`dx`/`dy` do `B(t)` na parcela da curva
//     na integral e simplificando (a álgebra cancela até um único termo `K*t^2`, `K` constante)
//     deixa `(1/2) * integral_0^1 2*K*t^2 dt = (1/2)*(2K/3) = K/3`. O `test_quadratic_curve_area`
//     abaixo escolhe `P0`/`P1`/`P2` concretos e afirma o `K/3` resultante com a aritmética por
//     extenso.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "core/raster.h"
#include "core/sfnt.h"

// EN: SOV-SFNT fixture (real Open Sans), same `ld -r -b binary` embedding as tests/test_sfnt.c --
//     see the Makefile's own comment for the mechanism. Used only by `test_glyph_o_symmetry`
//     below.
// PT: Fixture do SOV-SFNT (Open Sans real), mesmo embutimento `ld -r -b binary` do
//     tests/test_sfnt.c -- ver o comentário próprio do Makefile pro mecanismo. Usada só pelo
//     `test_glyph_o_symmetry` abaixo.
extern const unsigned char _binary_opensans_regular_ttf_start[];
extern const unsigned char _binary_opensans_regular_ttf_end[];

// EN: Shared scratch -- sized generously for the largest test below (`test_glyph_o_symmetry`'s
//     80x80 bitmap: `2*80*80 + 80 = 12880` floats). One `static` array, reused across every test
//     function (freestanding -- no heap, see include/core/raster.h "NO DYNAMIC ALLOCATION").
// PT: Scratch compartilhado -- dimensionado generosamente pro maior teste abaixo (o bitmap 80x80
//     do `test_glyph_o_symmetry`: `2*80*80 + 80 = 12880` floats). Um array `static`, reusado por
//     toda função de teste (freestanding -- sem heap, ver "SEM ALOCAÇÃO DINÂMICA" do
//     include/core/raster.h).
static float g_scratch[13000];

// EN: Sums every alpha byte in `bitmap[0..w*h)` (`stride == w` for every test in this file, no
//     padding) as a plain integer accumulator (no overflow risk: `w*h*255` for this file's largest
//     bitmap, `80*80*255 = 1632000`, comfortably inside `int`).
// PT: Soma todo byte de alpha em `bitmap[0..w*h)` (`stride == w` em todo teste deste arquivo, sem
//     padding) como um acumulador inteiro simples (sem risco de overflow: `w*h*255` pro maior
//     bitmap deste arquivo, `80*80*255 = 1632000`, cabe confortavelmente em `int`).
static long sum_alpha(const uint8_t* bitmap, int w, int h) {
    long s = 0;
    for (int i = 0; i < w * h; i++) s += bitmap[i];
    return s;
}

static int iabs_test(int v) { return (v < 0) ? -v : v; }

// ================================================================================================
// EN: (1) Axis-aligned RECTANGLE, half-pixel boundaries -- the brief's own worked example
//     ("borda em x=10.5 -> coluna 10 tem alpha~=128"). Font-unit scale chosen as
//     `scale_num=32, scale_den=1` -- 1 font-unit = `32/64 = 0.5` device px exactly -- so an
//     integer font-unit rectangle lands on exact HALF-pixel device boundaries. `origin_y_26_6 =
//     640` (`10.0px`) combined with the mandatory Y-flip (`device_y = origin_y - font_y*scale`,
//     include/core/raster.h) places the rectangle's device-space box at EXACTLY
//     `x in [2.5, 10.5], y in [2.5, 7.5]` -- worked out by hand below, right above the assertions.
// PT: (1) RETÂNGULO alinhado a eixo, fronteiras de meio-pixel -- o próprio exemplo trabalhado do
//     brief ("borda em x=10.5 -> coluna 10 tem alpha~=128"). Escala de unidade-de-fonte escolhida
//     como `scale_num=32, scale_den=1` -- 1 unidade-de-fonte = `32/64 = 0.5` px de dispositivo
//     exatamente -- então um retângulo de unidade-de-fonte inteira cai em fronteiras de
//     dispositivo de exato MEIO-PIXEL. `origin_y_26_6 = 640` (`10.0px`) combinado com o Y-flip
//     obrigatório (`device_y = origin_y - font_y*scale`, include/core/raster.h) posiciona a caixa
//     em espaço-de-dispositivo do retângulo em EXATAMENTE `x em [2.5, 10.5], y em [2.5, 7.5]` --
//     trabalhado à mão abaixo, logo acima das asserções.
// ================================================================================================
static void test_rectangle_halfpixel(void) {
    // EN: Font-unit corners (on-curve only, 4-point straight-edge contour):
    //     P0=(5,5) P1=(21,5) P2=(21,15) P3=(5,15), closed P3->P0.
    //     Device (`x_dev = fx*0.5`, `y_dev = 10 - fy*0.5`):
    //       P0'=(2.5,7.5)  P1'=(10.5,7.5)  P2'=(10.5,2.5)  P3'=(2.5,2.5)
    //     -> axis-aligned box x in [2.5,10.5] (width 8), y in [2.5,7.5] (height 5), area = 40.
    // PT: Cantos em unidade-de-fonte (só on-curve, contorno reta de 4 pontos):
    //     P0=(5,5) P1=(21,5) P2=(21,15) P3=(5,15), fechado P3->P0.
    //     Dispositivo (`x_dev = fx*0.5`, `y_dev = 10 - fy*0.5`):
    //       P0'=(2.5,7.5)  P1'=(10.5,7.5)  P2'=(10.5,2.5)  P3'=(2.5,2.5)
    //     -> caixa alinhada a eixo x em [2.5,10.5] (largura 8), y em [2.5,7.5] (altura 5),
    //        área = 40.
    static const glx_sfnt_point pts[4] = {
        {5, 5, 1},
        {21, 5, 1},
        {21, 15, 1},
        {5, 15, 1},
    };
    static const uint16_t ends[1] = {3};

    const int W = 14, H = 11;
    uint8_t bmp[14 * 11];
    size_t need = glx_raster_scratch_floats(W, H);
    TEST_ASSERT(need <= (sizeof(g_scratch) / sizeof(g_scratch[0])));

    glx_raster_result r = glx_rasterize_outline(pts, 4, ends, 1, 32, 1, 0, 640, bmp, W, H, W,
                                                 g_scratch, need);
    TEST_ASSERT_EQ(r, GLX_RASTER_OK);

    // EN: Interior cell (row 4 in [3,6] full rows, col 5 in [3,9] full cols) -> fully covered.
    // PT: Célula interior (linha 4 em [3,6] linhas cheias, col 5 em [3,9] colunas cheias) ->
    //     totalmente coberta.
    TEST_ASSERT_EQ(bmp[4 * W + 5], (uint8_t)255);

    // EN: Full row (4), partial column (10 -- the brief's own example: half-width sliver at the
    //     right edge, x in [10,10.5]) -> coverage 0.5*1.0 = 0.5 -> alpha round(127.5) = 128.
    // PT: Linha cheia (4), coluna parcial (10 -- o próprio exemplo do brief: fatia de meia-largura
    //     na borda direita, x em [10,10.5]) -> cobertura 0.5*1.0 = 0.5 -> alpha round(127.5) = 128.
    {
        int a = bmp[4 * W + 10];
        TEST_ASSERT(a >= 126 && a <= 130);
    }

    // EN: Partial row (2 -- y in [2.5,3]), full column (5) -> coverage 1.0*0.5 = 0.5 -> ~128.
    // PT: Linha parcial (2 -- y em [2.5,3]), coluna cheia (5) -> cobertura 1.0*0.5 = 0.5 -> ~128.
    {
        int a = bmp[2 * W + 5];
        TEST_ASSERT(a >= 126 && a <= 130);
    }

    // EN: Corner cell (row 2 partial, col 2 partial: x in [2.5,3], y in [2.5,3]) ->
    //     coverage 0.5*0.5 = 0.25 -> alpha round(63.75) = 64.
    // PT: Célula de canto (linha 2 parcial, col 2 parcial: x em [2.5,3], y em [2.5,3]) ->
    //     cobertura 0.5*0.5 = 0.25 -> alpha round(63.75) = 64.
    {
        int a = bmp[2 * W + 2];
        TEST_ASSERT(a >= 61 && a <= 67);
    }

    // EN: Clearly outside the box (row 0, col 0 -- box starts at x=2.5,y=2.5) -> uncovered.
    // PT: Claramente fora da caixa (linha 0, col 0 -- a caixa começa em x=2.5,y=2.5) -> descoberto.
    TEST_ASSERT_EQ(bmp[0 * W + 0], (uint8_t)0);

    // EN: Total coverage: analytic area is EXACTLY 40.0 px^2 (28 full cells*255 + 22 half-edge
    //     cells*127.5 + 4 corner cells*63.75, all divided by 255, sums to exactly 40 in the
    //     continuous case -- per-pixel integer rounding introduces at most a few ULPs of noise).
    // PT: Cobertura total: a área analítica é EXATAMENTE 40.0 px^2 (28 células cheias*255 + 22
    //     células de meia-borda*127.5 + 4 células de canto*63.75, tudo dividido por 255, soma
    //     exatamente 40 no caso contínuo -- arredondamento inteiro por-pixel introduz no máximo
    //     alguns ULPs de ruído).
    long s = sum_alpha(bmp, W, H);
    long expect_255 = 40L * 255L;
    TEST_ASSERT(iabs_test((int)(s - expect_255)) <= 255); // within 1.0 px^2
}

// ================================================================================================
// EN: (2) Right TRIANGLE, legs on the bitmap's own axes -- exercises the general (non-axis-
//     aligned) column-spanning path AND the `x<=0` "leading cover" boundary case (the left leg
//     sits EXACTLY at device x=0). Font points A=(0,8) B=(8,8) C=(0,0), `scale_num=64,
//     scale_den=1` (1 font-unit = 1 device px), `origin_y_26_6=512` (`8.0px`) -> device
//     A'=(0,0) B'=(8,0) C'=(0,8). Edges: A->B is horizontal (y=0, skipped, contributes nothing);
//     B->C is the hypotenuse `x = 8 - y`; C->A is the vertical left leg at `x=0` exactly. Area =
//     (1/2)*8*8 = 32.
// PT: (2) TRIÂNGULO retângulo, catetos nos próprios eixos do bitmap -- exercita o caminho geral
//     (não-alinhado-a-eixo) de abrangência-de-coluna E o caso de fronteira "cover à esquerda"
//     `x<=0` (o cateto esquerdo fica EXATAMENTE em x=0 de dispositivo). Pontos de fonte A=(0,8)
//     B=(8,8) C=(0,0), `scale_num=64, scale_den=1` (1 unidade-de-fonte = 1 px de dispositivo),
//     `origin_y_26_6=512` (`8.0px`) -> dispositivo A'=(0,0) B'=(8,0) C'=(0,8). Arestas: A->B é
//     horizontal (y=0, pulada, não contribui nada); B->C é a hipotenusa `x = 8 - y`; C->A é o
//     cateto esquerdo vertical em `x=0` exatamente. Área = (1/2)*8*8 = 32.
// ================================================================================================
static void test_triangle(void) {
    static const glx_sfnt_point pts[3] = {
        {0, 8, 1},
        {8, 8, 1},
        {0, 0, 1},
    };
    static const uint16_t ends[1] = {2};

    const int W = 10, H = 10;
    uint8_t bmp[10 * 10];
    size_t need = glx_raster_scratch_floats(W, H);
    TEST_ASSERT(need <= (sizeof(g_scratch) / sizeof(g_scratch[0])));

    glx_raster_result r = glx_rasterize_outline(pts, 3, ends, 1, 64, 1, 0, 512, bmp, W, H, W,
                                                 g_scratch, need);
    TEST_ASSERT_EQ(r, GLX_RASTER_OK);

    // EN: Deep interior (row 0, col 0 -- hypotenuse boundary at this row is x~=7.5..8) -> nearly
    //     fully covered.
    // PT: Fundo interior (linha 0, col 0 -- a fronteira da hipotenusa nesta linha é x~=7.5..8) ->
    //     quase totalmente coberto.
    TEST_ASSERT(bmp[0 * W + 0] >= 250);

    // EN: Clearly outside the triangle's [0,8]x[0,8] extent.
    // PT: Claramente fora da extensão [0,8]x[0,8] do triângulo.
    TEST_ASSERT_EQ(bmp[9 * W + 9], (uint8_t)0);

    // EN: Exact diagonal cell (row 3, col 4 -- y in [3,4], x in [4,5]): hypotenuse `x = 8-y` sits
    //     at x=5 when y=3 (right edge of this column, fully inside) and x=4 when y=4 (left edge,
    //     fully outside) -- coverage ramps linearly 1.0 -> 0.0 across the row, integrating to
    //     EXACTLY 0.5 -> alpha round(127.5) = 128.
    // PT: Célula diagonal exata (linha 3, col 4 -- y em [3,4], x em [4,5]): a hipotenusa `x = 8-y`
    //     fica em x=5 quando y=3 (borda direita desta coluna, totalmente dentro) e x=4 quando y=4
    //     (borda esquerda, totalmente fora) -- a cobertura rampa linearmente 1.0 -> 0.0 ao longo
    //     da linha, integrando pra EXATAMENTE 0.5 -> alpha round(127.5) = 128.
    {
        int a = bmp[3 * W + 4];
        TEST_ASSERT(a >= 126 && a <= 130);
    }

    // EN: Total coverage: analytic area EXACTLY 32.0 px^2.
    // PT: Cobertura total: área analítica EXATAMENTE 32.0 px^2.
    long s = sum_alpha(bmp, W, H);
    long expect_255 = 32L * 255L;
    TEST_ASSERT(iabs_test((int)(s - expect_255)) <= 255);
}

// ================================================================================================
// EN: (3) QUADRATIC curve area -- see this file's own header for the full Green's-theorem
//     derivation. Font points P0=(2,8,on) P1=(6,2,off) P2=(10,8,on), `scale_num=64,scale_den=1`,
//     `origin_y_26_6=640` (`10.0px`) -> device P0'=(2,2) P1'=(6,8) P2'=(10,2). K = cross(P1'-P0',
//     P2'-P0') = cross((4,6),(8,0)) = 4*0 - 6*8 = -48 -> |Area| = |K/3| = 16.
// PT: (3) Área de curva QUADRÁTICA -- ver o próprio cabeçalho deste arquivo pra derivação completa
//     via teorema de Green. Pontos de fonte P0=(2,8,on) P1=(6,2,off) P2=(10,8,on),
//     `scale_num=64,scale_den=1`, `origin_y_26_6=640` (`10.0px`) -> dispositivo P0'=(2,2)
//     P1'=(6,8) P2'=(10,2). K = cross(P1'-P0', P2'-P0') = cross((4,6),(8,0)) = 4*0 - 6*8 = -48 ->
//     |Área| = |K/3| = 16.
// ================================================================================================
static void test_quadratic_curve_area(void) {
    static const glx_sfnt_point pts[3] = {
        {2, 8, 1},
        {6, 2, 0},
        {10, 8, 1},
    };
    static const uint16_t ends[1] = {2};

    const int W = 14, H = 12;
    uint8_t bmp[14 * 12];
    size_t need = glx_raster_scratch_floats(W, H);
    TEST_ASSERT(need <= (sizeof(g_scratch) / sizeof(g_scratch[0])));

    glx_raster_result r = glx_rasterize_outline(pts, 3, ends, 1, 64, 1, 0, 640, bmp, W, H, W,
                                                 g_scratch, need);
    TEST_ASSERT_EQ(r, GLX_RASTER_OK);

    long s = sum_alpha(bmp, W, H);
    long expect_255 = 16L * 255L;
    // EN: Looser tolerance than the two straight-edge tests above -- the flattened curve is only
    //     an APPROXIMATION of the true parabola (bounded by GLX_RASTER_FLATNESS_TOL, src/raster.c),
    //     not exact -- 1.5 px^2 comfortably covers the flattening error at this curve's scale
    //     while still failing loudly on a genuinely broken flattening/rasterization path (which,
    //     historically for this class of bug, misses by an order of magnitude more than this).
    // PT: Tolerância mais frouxa que os dois testes de aresta-reta acima -- a curva achatada é só
    //     uma APROXIMAÇÃO da parábola verdadeira (limitada por GLX_RASTER_FLATNESS_TOL,
    //     src/raster.c), não exata -- 1.5 px^2 cobre confortavelmente o erro de flattening na
    //     escala desta curva e ainda assim falha alto num caminho de flattening/rasterização
    //     genuinamente quebrado (que, historicamente pra esta classe de bug, erra por uma ordem de
    //     grandeza a mais que isto).
    TEST_ASSERT(iabs_test((int)(s - expect_255)) <= (int)(1.5 * 255.0));
}

// ================================================================================================
// EN: (4) OVERFLOW SATURATION -- an extreme `scale_num` (near INT32_MAX) collapses every point of
//     a hand-built triangle to the IDENTICAL saturated device coordinate (both `10*2e9` and
//     `20*2e9`, this triangle's two distinct font-unit magnitudes, exceed GLX_RASTER_FX_MAX --
//     src/raster.c's `scale_to_fx` clamps both to the SAME `GLX_RASTER_FX_MAX`, so the "shape"
//     degenerates to a single point). This proves the `int64_t`-intermediate/saturate path in
//     `scale_to_fx` never wraps/UBs even for a caller-supplied scale large enough to overflow
//     naive `int32_t` multiplication -- and gives an unambiguous, deterministic expected result:
//     a zero-area degenerate contour has EVERY edge's `y0 == y1` (skipped as horizontal, see
//     `process_edge`), so the output bitmap must be entirely `0`. This is the executed proof this
//     ticket's brief asks for ("Overflow de fixed-point ... satura/clampa, provado por teste").
// PT: (4) SATURAÇÃO DE OVERFLOW -- um `scale_num` extremo (perto de INT32_MAX) colapsa todo ponto
//     de um triângulo construído à mão pra a MESMA coordenada de dispositivo saturada (tanto
//     `10*2e9` quanto `20*2e9`, as duas magnitudes distintas em unidade-de-fonte deste triângulo,
//     excedem GLX_RASTER_FX_MAX -- o `scale_to_fx` do src/raster.c limita ambas pro MESMO
//     `GLX_RASTER_FX_MAX`, então a "forma" degenera pra um único ponto). Isso prova que o caminho
//     de intermediário-`int64_t`/saturação do `scale_to_fx` nunca dá a volta/UB mesmo pra uma
//     escala fornecida por quem chama grande o bastante pra estourar multiplicação `int32_t`
//     ingênua -- e dá um resultado esperado inequívoco, determinístico: um contorno degenerado de
//     área zero tem TODA aresta com `y0 == y1` (pulada como horizontal, ver `process_edge`), então
//     o bitmap de saída precisa estar inteiramente em `0`. Esta é a prova executada que o brief
//     desta tarefa pede ("Overflow de fixed-point ... satura/clampa, provado por teste").
// ================================================================================================
static void test_overflow_saturation(void) {
    static const glx_sfnt_point pts[3] = {
        {10, 10, 1},
        {20, 10, 1},
        {10, 20, 1},
    };
    static const uint16_t ends[1] = {2};

    const int W = 8, H = 8;
    uint8_t bmp[8 * 8];
    size_t need = glx_raster_scratch_floats(W, H);
    TEST_ASSERT(need <= (sizeof(g_scratch) / sizeof(g_scratch[0])));

    glx_raster_result r =
        glx_rasterize_outline(pts, 3, ends, 1, 2000000000, 1, 0, 0, bmp, W, H, W, g_scratch, need);
    TEST_ASSERT_EQ(r, GLX_RASTER_OK);

    long s = sum_alpha(bmp, W, H);
    TEST_ASSERT_EQ(s, 0L);
}

// ================================================================================================
// EN: (5) INVALID ARGS / degenerate-input hardening -- mirrors tests/test_sfnt.c's own "HOSTILE"
//     section shape. `num_points == 0` (an empty glyph, e.g. `space`) must succeed with an
//     all-zero bitmap (this ticket's brief, "num_points==0 (glyph vazio) -> bitmap todo-zero,
//     OK"); a 1-point "contour" must be silently ignored (brief, "contorno com 1 ponto -> ignora
//     limpo"); `scale_den <= 0`, `w/h <= 0`, `stride < w`, and an undersized `scratch` must all
//     fail cleanly with the documented error code, never crash.
// PT: (5) ARGUMENTOS INVÁLIDOS / hardening de input degenerado -- espelha o formato da própria
//     seção "HOSTILE" do tests/test_sfnt.c. `num_points == 0` (um glyph vazio, ex.: `space`)
//     precisa ter sucesso com um bitmap tudo-zero (brief desta tarefa, "num_points==0 (glyph
//     vazio) -> bitmap todo-zero, OK"); um "contorno" de 1 ponto precisa ser ignorado em silêncio
//     (brief, "contorno com 1 ponto -> ignora limpo"); `scale_den <= 0`, `w/h <= 0`,
//     `stride < w`, e um `scratch` pequeno demais precisam todos falhar de forma limpa com o
//     código de erro documentado, nunca crashar.
// ================================================================================================
static void test_invalid_and_degenerate(void) {
    const int W = 4, H = 4;
    uint8_t bmp[4 * 4];
    size_t need = glx_raster_scratch_floats(W, H);

    // ---- empty glyph: num_points == 0 -------------------------------------------------------
    {
        for (int i = 0; i < W * H; i++) bmp[i] = 0xAB; // poison, must be overwritten with 0
        glx_raster_result r =
            glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, W, H, W, g_scratch, need);
        TEST_ASSERT_EQ(r, GLX_RASTER_OK);
        TEST_ASSERT_EQ(sum_alpha(bmp, W, H), 0L);
    }

    // ---- degenerate 1-point "contour" --------------------------------------------------------
    {
        static const glx_sfnt_point one_pt[1] = {{1, 1, 1}};
        static const uint16_t one_end[1] = {0};
        glx_raster_result r = glx_rasterize_outline(one_pt, 1, one_end, 1, 64, 1, 0, 0, bmp, W, H,
                                                      W, g_scratch, need);
        TEST_ASSERT_EQ(r, GLX_RASTER_OK);
        TEST_ASSERT_EQ(sum_alpha(bmp, W, H), 0L);
    }

    // ---- invalid args --------------------------------------------------------------------------
    TEST_ASSERT_EQ(glx_rasterize_outline(NULL, 0, NULL, 0, 64, 0 /* scale_den<=0 */, 0, 0, bmp, W,
                                          H, W, g_scratch, need),
                   GLX_RASTER_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(
        glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, 0 /* w<=0 */, H, W, g_scratch, need),
        GLX_RASTER_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(
        glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, W, H, 1 /* stride<w */, g_scratch,
                               need),
        GLX_RASTER_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, NULL /* bitmap */, W, H, W,
                                          g_scratch, need),
                   GLX_RASTER_ERR_INVALID_ARG);

    // ---- undersized scratch ---------------------------------------------------------------------
    TEST_ASSERT(need > 0);
    TEST_ASSERT_EQ(
        glx_rasterize_outline(NULL, 0, NULL, 0, 64, 1, 0, 0, bmp, W, H, W, g_scratch, need - 1),
        GLX_RASTER_ERR_SCRATCH_TOO_SMALL);

    // ---- glx_raster_scratch_floats itself: invalid dims -> 0 -----------------------------------
    TEST_ASSERT_EQ(glx_raster_scratch_floats(0, 4), (size_t)0);
    TEST_ASSERT_EQ(glx_raster_scratch_floats(4, -1), (size_t)0);
}

// ================================================================================================
// EN: (6) REAL GLYPH: 'O' from Open Sans -- proves curve flattening + non-zero winding (the
//     outer ring and the inner hole, wound in opposite directions) TOGETHER, on a real font, not
//     just synthetic shapes. Three properties checked, all robust to the specific stroke geometry
//     of Open Sans' actual 'O' (never hand-waving an exact pixel value for a real font's curve --
//     this ticket's brief's own oracle guidance: "assert simetria aproximada ... miolo é 0 ...
//     anel é >0"):
//       (a) the glyph's own bbox-center pixel is UNCOVERED (the hole);
//       (b) SOME pixel on the bbox-center row has strong coverage (the ring exists);
//       (c) approximate horizontal AND vertical mirror symmetry around the bbox center (Open
//           Sans' 'O' is designed near-round -- generous tolerance accounts for real hinting-free
//           AA + any deliberate optical correction a real font may carry).
// PT: (6) GLYPH REAL: 'O' da Open Sans -- prova flattening de curva + winding não-zero (o anel
//     externo e o buraco interno, enrolados em direções opostas) JUNTOS, numa fonte real, não só
//     formas sintéticas. Três propriedades checadas, todas robustas à geometria de traço
//     específica do 'O' de verdade da Open Sans (nunca chutando um valor de pixel exato pra uma
//     curva de fonte real -- a própria orientação de oráculo do brief desta tarefa: "assert
//     simetria aproximada ... miolo é 0 ... anel é >0"):
//       (a) o pixel do centro-da-bbox do próprio glyph está DESCOBERTO (o buraco);
//       (b) ALGUM pixel na linha do centro-da-bbox tem cobertura forte (o anel existe);
//       (c) simetria de espelho horizontal E vertical aproximada em torno do centro da bbox (o
//           'O' da Open Sans é desenhado quase-redondo -- tolerância generosa contempla AA
//           real sem hinting + qualquer correção óptica deliberada que uma fonte real possa ter).
// ================================================================================================
static int32_t scalef_test(int32_t v, int32_t num, int32_t den) {
    return (int32_t)(((int64_t)v * (int64_t)num) / (int64_t)den);
}

static void test_glyph_o_symmetry(void) {
    const unsigned char* blob = _binary_opensans_regular_ttf_start;
    size_t len = (size_t)(_binary_opensans_regular_ttf_end - _binary_opensans_regular_ttf_start);

    glx_sfnt_face face;
    TEST_ASSERT_EQ(glx_sfnt_open(blob, len, &face), GLX_SFNT_OK);

    uint32_t gid = glx_sfnt_glyph_id(&face, 'O');
    TEST_ASSERT(gid != 0);

    glx_sfnt_point points[256];
    uint16_t contour_ends[8];
    glx_sfnt_outline out;
    glx_sfnt_result sr =
        glx_sfnt_glyph_outline(&face, gid, points, 256, contour_ends, 8, &out);
    TEST_ASSERT_EQ(sr, GLX_SFNT_OK);
    // EN: 'O' is a ring with a hole -- exactly 2 contours, wound opposite directions.
    // PT: 'O' é um anel com buraco -- exatamente 2 contornos, enrolados em direções opostas.
    TEST_ASSERT_EQ(out.num_contours, (uint16_t)2);

    int32_t scale_num = 48 * 64; // 48px em size
    int32_t scale_den = (int32_t)face.units_per_em;
    int32_t margin_fx = 16 * 64; // 16px margin on every side

    int32_t origin_x = margin_fx - scalef_test(out.x_min, scale_num, scale_den);
    int32_t origin_y = margin_fx + scalef_test(out.y_max, scale_num, scale_den);

    const int W = 80, H = 80;
    static uint8_t bmp[80 * 80];
    size_t need = glx_raster_scratch_floats(W, H);
    TEST_ASSERT(need <= (sizeof(g_scratch) / sizeof(g_scratch[0])));

    glx_raster_result r = glx_rasterize_outline(points, out.num_points, contour_ends,
                                                 out.num_contours, scale_num, scale_den, origin_x,
                                                 origin_y, bmp, W, H, W, g_scratch, need);
    TEST_ASSERT_EQ(r, GLX_RASTER_OK);

    // EN: geometric bbox center, in bitmap pixels (see this file's header derivation for
    //     `origin_x`/`origin_y` above -- `x_min` maps to `margin_fx`/64 px, `x_max` maps to
    //     `margin + width_px`, so the center is `margin + width_px/2`, and likewise for y).
    // PT: centro geométrico da bbox, em pixels de bitmap (ver a derivação de cabeçalho deste
    //     arquivo pra `origin_x`/`origin_y` acima -- `x_min` mapeia pra `margin_fx`/64 px,
    //     `x_max` mapeia pra `margin + width_px`, então o centro é `margin + width_px/2`, e
    //     igualmente pra y).
    int32_t width_fx = scalef_test(out.x_max - out.x_min, scale_num, scale_den);
    int32_t height_fx = scalef_test(out.y_max - out.y_min, scale_num, scale_den);
    int center_col = 16 + (int)(width_fx / 64 / 2);
    int center_row = 16 + (int)(height_fx / 64 / 2);
    TEST_ASSERT(center_col > 0 && center_col < W);
    TEST_ASSERT(center_row > 0 && center_row < H);

    // ---- (a) the hole: bbox center is uncovered ------------------------------------------------
    TEST_ASSERT(bmp[center_row * W + center_col] < 40);

    // ---- (b) the ring exists: some pixel on the center row is strongly covered -----------------
    {
        int found = 0;
        for (int c = 0; c < W; c++) {
            if (bmp[center_row * W + c] > 200) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found);
    }

    // ---- (c) approximate mirror symmetry around the bbox center --------------------------------
    // EN: AGGREGATE half-plane coverage sums, not per-pixel point sampling -- a real 'O''s ring
    //     stroke is only ~4-5px wide at this render size, so a single-pixel sample landing on the
    //     steep AA transition band (empirically confirmed while developing this test: individual
    //     pixel-offset diffs up to 133/255 at some offsets, purely from sub-pixel misalignment
    //     between the two mirrored sample points crossing the SAME steep gradient at slightly
    //     different sub-pixel phase, not from any actual asymmetry) is far too noisy a signal.
    //     Summing every pixel in each half-plane INTEGRATES across that gradient instead of
    //     sampling a single point on it, which is what makes this check robust: empirically,
    //     Open Sans' real 'O' at this render size splits `left=46387 right=46574` (0.4% apart)
    //     and `top=46280 bot=46335` (0.1% apart) around its own bbox center -- the 10% tolerance
    //     below is ~25x looser than that measured real-font asymmetry, comfortably away from a
    //     false failure while still catching a genuinely broken half (e.g. one side rendering as
    //     entirely empty, which this checks would catch instantly).
    // PT: Somas de cobertura de MEIO-PLANO AGREGADAS, não amostragem de ponto por-pixel -- o
    //     traço do anel de um 'O' de verdade tem só ~4-5px de largura neste tamanho de render,
    //     então uma amostra de um único pixel caindo na banda de transição AA íngreme
    //     (confirmado empiricamente ao desenvolver este teste: diffs de offset-de-pixel
    //     individuais de até 133/255 em alguns offsets, puramente de desalinhamento sub-pixel
    //     entre os dois pontos-amostra espelhados cruzando o MESMO gradiente íngreme em fase
    //     sub-pixel levemente diferente, não de assimetria real nenhuma) é um sinal ruidoso
    //     demais. Somar todo pixel em cada meio-plano INTEGRA através desse gradiente em vez de
    //     amostrar um único ponto nele, que é o que torna esta checagem robusta: empiricamente, o
    //     'O' real da Open Sans neste tamanho de render divide `left=46387 right=46574` (0.4% de
    //     diferença) e `top=46280 bot=46335` (0.1% de diferença) em torno do próprio centro da
    //     bbox -- a tolerância de 10% abaixo é ~25x mais frouxa que essa assimetria medida de
    //     fonte real, confortavelmente longe de uma falsa falha ao mesmo tempo que ainda pega uma
    //     metade genuinamente quebrada (ex.: um lado renderizando inteiramente vazio, que esta
    //     checagem pegaria instantaneamente).
    {
        long left = 0, right = 0, top = 0, bot = 0;
        for (int row = 0; row < H; row++) {
            for (int col = 0; col < W; col++) {
                int a = bmp[row * W + col];
                if (col < center_col) left += a;
                else if (col > center_col) right += a;
                if (row < center_row) top += a;
                else if (row > center_row) bot += a;
            }
        }
        TEST_ASSERT(left > 0 && right > 0 && top > 0 && bot > 0);
        // |left-right| <= 10% of (left+right), and likewise for top/bot.
        long hdiff = (left > right) ? (left - right) : (right - left);
        long vdiff = (top > bot) ? (top - bot) : (bot - top);
        TEST_ASSERT(hdiff * 10L <= (left + right));
        TEST_ASSERT(vdiff * 10L <= (top + bot));
    }
}

int main(void) {
    test_rectangle_halfpixel();
    test_triangle();
    test_quadratic_curve_area();
    test_overflow_saturation();
    test_invalid_and_degenerate();
    test_glyph_o_symmetry();
    TEST_PASS();
}
