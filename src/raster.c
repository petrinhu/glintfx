// SPDX-License-Identifier: MPL-2.0
// EN: SOV-RAST (FT-F2) -- implementation. See include/core/raster.h's file header for the full
//     clean-room rationale, API contract, and fixed-point/float precision split; this file's own
//     comments focus on the LOCAL "how" (the two algorithms: quadratic flattening, analytic
//     coverage rasterization), not re-derived "why" that already lives there.
//
//     BUILD-TWICE / HOSTED-COMPANION NOTE (mirrors src/sfnt.c's own file header): like every other
//     src/*.c file, this one is compiled TWICE by the Makefile -- once freestanding into
//     build/obj/ (tests/test_raster.c and every other gate program), once again under
//     $(CORE_RENAME_FLAGS) into build/objcore/ for build/libcore.a. This file's only externally-
//     visible symbols already carry the `glx_raster_`/`glx_rasterize_` prefix (namespaced by
//     construction, not a build-time rename -- see Makefile/tools/check_libcore_symbols.sh, which
//     only needed a literal name added to its whitelist, no new $(CORE_RENAME_FLAGS) entry).
//     Every OTHER identifier below is `static` (internal linkage). This file also does not
//     `#include` alloc.h/mem.h/types.h -- ONLY <stdint.h>/<stddef.h>/"core/sfnt.h" (a sibling
//     include/core/ header, structural-only dependency, see raster.h) -- so, exactly like
//     src/sfnt.c, it compiles UNCHANGED as a plain hosted (libc-linked) translation unit too, which
//     is what lets tests/sanitize_raster.c exist as a hosted-companion ASan/UBSan gate the same
//     shape as tests/sanitize_sfnt.c (TESTES.md's TST-SFNT-SAN entry named this exact reuse in
//     advance: "SOV-RAST ... deve crescer um sanitize-rast irmão na mesma forma").
//
//     THE TWO ALGORITHMS (both re-derived from first principles in this file, not read from any
//     rasterizer's source -- see raster.h's CLEAN-ROOM note):
//
//     (1) QUADRATIC FLATTENING (`flatten_quad`, fixed-point 26.6 int32): De Casteljau subdivision
//         -- recursively bisect the control polygon (Q0 = midpoint(P0,P1), Q1 = midpoint(P1,P2),
//         R = midpoint(Q0,Q1); the curve splits into [P0,Q0,R] and [R,Q1,P2], each a valid
//         quadratic Bezier covering half the original parameter range) until a FLATNESS test says
//         the curve is close enough to its own chord to approximate with a straight line, or a
//         hard depth cap is hit (whichever first -- the depth cap is what makes this provably
//         terminating even for a pathological/hostile curve the flatness heuristic never
//         satisfies, e.g. a "loop" quadratic with P0==P2). All midpoint arithmetic is `(a+b)>>1`
//         (int32_t, C23's well-defined arithmetic-right-shift-of-negative-int semantics on this
//         compiler/target -- see this project's CLAUDE.md toolchain table, clang/x86-64) -- exact
//         floor-division-by-2, zero float, zero libm, per this ticket's brief. The flatness test
//         itself is a self-derived, PURELY INTEGER, division-free heuristic: `cross(P1-P0,P2-P0)`
//         (the standard 2D cross product, proportional to TWICE the triangle area P0-P1-P2, hence
//         to how far P1 bows away from the P0-P2 chord) compared against `FLATNESS_TOL * max(|dx|,
//         |dy|, 1)` (`dx,dy` = the chord's own extent) -- multiplying the tolerance by the chord's
//         extent instead of dividing the cross product by the chord's LENGTH (which would need a
//         `sqrt`, i.e. libm) is what keeps this integer-only; it is a looser bound than the exact
//         perpendicular-distance test (it does not correct for the chord's diagonal-vs-axis-
//         aligned length), but is more than tight enough at `FLATNESS_TOL` chosen for sub-1/8px
//         deviation, and the hard depth cap (`GLX_RASTER_MAX_QUAD_DEPTH`) bounds the worst case
//         regardless.
//
//     (2) ANALYTIC COVERAGE RASTERIZATION (`accum_row_segment`/`accum_column`/finishing sweep,
//         `float`): every flattened line segment is decomposed, scanline row by scanline row, into
//         per-pixel-cell "area" and "cover" deltas via Green's-theorem trapezoid reasoning (see
//         `accum_row_segment`'s own derivation comment, right above it, for the exact formulas) --
//         accumulated into two `w*h` scratch planes plus one `h`-sized "leading cover" column for
//         segments that cross the bitmap's LEFT edge (`x < 0`, which cannot be folded into a
//         "virtual column -1" of the `w*h` planes without a separate array, since `cover[c]`'s
//         effect starts propagating only at column `c+1`, not `c` itself -- see this function's
//         own comment). A SINGLE finishing left-to-right sweep per row then turns those deltas
//         into final per-pixel signed coverage (`area[x] + running-sum-of-cover-for-columns-<x`),
//         and `abs(...)` + round + clamp turns that into the final 8-bit alpha (NON-ZERO winding:
//         a hole wound the opposite direction cancels the outer contour's coverage back toward
//         zero exactly where the two overlap, by construction of the signed-area sum -- no special
//         "is this contour a hole" bookkeeping needed, or possible to get wrong, because the
//         algorithm never reasons about contours as a whole, only about the SIGNED area swept by
//         each individual edge). This is the "1 passe" (one accumulation pass over the edges, one
//         finishing sweep over the pixels) this ticket's brief calls for, as opposed to
//         supersampling (which would need multiple coverage SAMPLES per pixel).
// PT: SOV-RAST (FT-F2) -- implementação. Ver o cabeçalho de arquivo do include/core/raster.h pro
//     racional clean-room completo, contrato de API, e divisão de precisão fixed-point/float; os
//     comentários deste arquivo focam no "como" LOCAL (os dois algoritmos: flattening quadrático,
//     rasterização de cobertura analítica), não no "porquê" já re-derivado que mora lá.
//
//     NOTA DE COMPILAÇÃO-DUPLA / HOSTED-COMPANION (espelha o próprio cabeçalho de arquivo do
//     src/sfnt.c): como todo outro src/*.c, este arquivo é compilado DUAS VEZES pelo Makefile --
//     uma vez freestanding pra build/obj/ (tests/test_raster.c e todo outro programa-gate), outra
//     vez sob $(CORE_RENAME_FLAGS) pra build/objcore/ pra build/libcore.a. Os únicos símbolos
//     externamente visíveis deste arquivo já carregam o prefixo `glx_raster_`/`glx_rasterize_`
//     (namespaced por construção, não um rename em tempo de build -- ver Makefile/
//     tools/check_libcore_symbols.sh, que só precisou de um nome literal acrescentado à whitelist,
//     nenhuma entrada nova em $(CORE_RENAME_FLAGS)). Todo OUTRO identificador abaixo é `static`
//     (linkage interno). Este arquivo também não `#include`ia alloc.h/mem.h/types.h -- SÓ
//     <stdint.h>/<stddef.h>/"core/sfnt.h" (um header irmão de include/core/, dependência só
//     estrutural, ver raster.h) -- então, exatamente como o src/sfnt.c, compila INALTERADO como
//     uma unidade de tradução hosted (linkada com libc) comum também, o que é o que permite
//     tests/sanitize_raster.c existir como um portão hosted-companion ASan/UBSan da mesma forma do
//     tests/sanitize_sfnt.c (a entrada TST-SFNT-SAN do TESTES.md já nomeou esse reuso exato de
//     antemão: "SOV-RAST ... deve crescer um sanitize-rast irmão na mesma forma").
//
//     OS DOIS ALGORITMOS (ambos re-derivados do zero neste arquivo, não lidos do fonte de rasterizador
//     nenhum -- ver a nota CLEAN-ROOM do raster.h):
//
//     (1) FLATTENING QUADRÁTICO (`flatten_quad`, fixed-point 26.6 int32): subdivisão De Casteljau
//         -- bisecta recursivamente o polígono de controle (Q0 = ponto-médio(P0,P1), Q1 =
//         ponto-médio(P1,P2), R = ponto-médio(Q0,Q1); a curva se divide em [P0,Q0,R] e [R,Q1,P2],
//         cada uma uma Bezier quadrática válida cobrindo metade da faixa de parâmetro original) até
//         um teste de FLATNESS dizer que a curva está perto o bastante da própria corda pra
//         aproximar com uma reta, ou um teto rígido de profundidade ser atingido (o que vier
//         primeiro -- o teto de profundidade é o que torna isso comprovadamente terminante mesmo
//         pra uma curva patológica/hostil que a heurística de flatness nunca satisfaz, ex.: uma
//         quadrática "laço" com P0==P2). Toda aritmética de ponto-médio é `(a+b)>>1` (int32_t,
//         semântica bem-definida do C23 de shift-aritmético-à-direita-de-int-negativo neste
//         compilador/alvo -- ver a tabela de toolchain do CLAUDE.md deste projeto, clang/x86-64) --
//         divisão-por-2-com-floor exata, zero float, zero libm, conforme o brief desta tarefa. O
//         próprio teste de flatness é uma heurística auto-derivada, PURAMENTE INTEIRA, sem divisão:
//         `cross(P1-P0,P2-P0)` (o produto vetorial 2D padrão, proporcional a DUAS VEZES a área do
//         triângulo P0-P1-P2, logo a quão longe P1 se afasta da corda P0-P2) comparado contra
//         `FLATNESS_TOL * max(|dx|, |dy|, 1)` (`dx,dy` = a própria extensão da corda) --
//         multiplicar a tolerância pela extensão da corda em vez de dividir o produto vetorial pelo
//         COMPRIMENTO da corda (que precisaria de uma `sqrt`, ou seja, libm) é o que mantém isso
//         só-inteiro; é um limite mais frouxo que o teste exato de distância perpendicular (não
//         corrige pro comprimento diagonal-vs-alinhado-a-eixo da corda), mas é folgado o bastante
//         no `FLATNESS_TOL` escolhido pra desvio sub-1/8px, e o teto rígido de profundidade
//         (`GLX_RASTER_MAX_QUAD_DEPTH`) limita o pior caso de qualquer jeito.
//
//     (2) RASTERIZAÇÃO DE COBERTURA ANALÍTICA (`accum_row_segment`/`accum_column`/varredura final,
//         `float`): todo segmento de reta achatado é decomposto, linha de scanline por linha de
//         scanline, em deltas de "area" e "cover" por-célula-de-pixel via raciocínio trapezoidal do
//         teorema de Green (ver o próprio comentário de derivação do `accum_row_segment`, logo
//         acima dele, pras fórmulas exatas) -- acumulados em dois planos de rascunho `w*h` mais uma
//         coluna de tamanho `h` de "cover à esquerda" pra segmentos que cruzam a borda ESQUERDA do
//         bitmap (`x < 0`, que não pode ser dobrada numa "coluna virtual -1" dos planos `w*h` sem
//         um array separado, já que o efeito do `cover[c]` só começa a se propagar na coluna `c+1`,
//         não `c` em si -- ver o comentário próprio desta função). Uma ÚNICA varredura final da
//         esquerda-pra-direita por linha então transforma esses deltas em cobertura final com sinal
//         por-pixel (`area[x] + soma-corrente-de-cover-das-colunas-<x`), e `abs(...)` + arredonda +
//         limita transforma isso no alpha 8-bit final (winding NÃO-ZERO: um buraco enrolado na
//         direção oposta cancela a cobertura do contorno externo de volta pra zero exatamente onde
//         os dois se sobrepõem, por construção da soma de área com sinal -- nenhuma contabilidade
//         especial de "este contorno é um buraco" necessária, ou possível de errar, porque o
//         algoritmo nunca raciocina sobre contornos como um todo, só sobre a área COM SINAL varrida
//         por cada aresta individual). Este é o "1 passe" (uma passada de acumulação sobre as
//         arestas, uma varredura final sobre os pixels) que o brief desta tarefa pede, em oposição
//         a supersampling (que precisaria de múltiplas AMOSTRAS de cobertura por pixel).
// Copyright (c) 2026 Petrus Silva Costa
#include "core/raster.h"

// ================================================================================================
// EN: Small integer/float helpers -- no libc, no libm. Same manual-guard idioms src/sfnt.c already
//     established (round_and_saturate_i16's NaN-guard-then-saturate shape).
// PT: Pequenos helpers de inteiro/float -- sem libc, sem libm. Mesmos idiomas de guarda manual que
//     o src/sfnt.c já estabeleceu (a forma guarda-NaN-depois-satura do round_and_saturate_i16).
// ================================================================================================

// EN: QW-IABS32 (INBOX drain, Onda 1) -- INT32_MIN guard, same guard-then-operate shape as
//     `round_and_saturate_i16`'s NaN-guard above (this file's header note). `-v` on `v ==
//     INT32_MIN` is signed-integer-overflow (the mathematical result, `+2147483648`, does not
//     fit back into `int32_t`) -- UB per C 6.5p5, not merely "implementation-defined" like the
//     narrowing casts elsewhere in this trio. REACHABILITY, stated honestly: this file's only two
//     call sites (`flatten_quad`'s `iabs32(dx)`/`iabs32(dy)`) feed `dx = p2x - p0x`/`dy = p2y -
//     p0y` where both operands are already `saturate_fx`-bounded to `[-GLX_RASTER_FX_MAX,
//     GLX_RASTER_FX_MAX]`, far from INT32_MIN -- `tests/sanitize_hint.c`'s own header already
//     recorded this exact domino finding as "confirmed unreachable". The guard below is
//     therefore defense in depth for a `static` helper with no unbounded caller TODAY, proven
//     (not merely argued) by the hosted-companion probe `tests/probe_iabs32.c` (`make
//     probe-iabs32`), which #includes this file to reach `iabs32` directly and would abort under
//     `-fsanitize=undefined` if this guard were ever removed.
// PT: Guarda de INT32_MIN do QW-IABS32 (esvaziamento da INBOX, Onda 1) -- mesma forma
//     guarda-depois-opera da guarda de NaN do `round_and_saturate_i16` acima (nota do cabeçalho
//     deste arquivo). `-v` com `v == INT32_MIN` é overflow de inteiro com sinal (o resultado
//     matemático, `+2147483648`, não cabe de volta em `int32_t`) -- UB pelo C 6.5p5, não meramente
//     "definido-pela-implementação" como os casts de estreitamento em outros pontos deste trio.
//     ALCANÇABILIDADE, dita com honestidade: os dois únicos pontos de chamada deste arquivo
//     (`iabs32(dx)`/`iabs32(dy)` do `flatten_quad`) alimentam `dx = p2x - p0x`/`dy = p2y - p0y`
//     onde os dois operandos já são limitados por `saturate_fx` a `[-GLX_RASTER_FX_MAX,
//     GLX_RASTER_FX_MAX]`, longe de INT32_MIN -- o próprio cabeçalho do `tests/sanitize_hint.c` já
//     registrou esse achado dominó exato como "confirmada inalcançável". A guarda abaixo é
//     portanto defesa-em-profundidade pra um helper `static` sem chamador não-limitado HOJE,
//     provada (não só argumentada) pela sonda hosted-companion `tests/probe_iabs32.c` (`make
//     probe-iabs32`), que `#include`ia este arquivo pra alcançar o `iabs32` direto e abortaria sob
//     `-fsanitize=undefined` se esta guarda algum dia fosse removida.
static int32_t iabs32(int32_t v) {
    if (v == INT32_MIN) return INT32_MAX;
    return (v < 0) ? -v : v;
}

static float fabs_f(float v) { return (v < 0.0f) ? -v : v; }

// EN: `a < b` picks the smaller INTEGER pixel index without truncation surprises -- both
//     `int`/`int32_t`, no float involved.
// PT: `a < b` escolhe o menor ÍNDICE INTEIRO de pixel sem surpresa de truncamento -- ambos
//     `int`/`int32_t`, sem float envolvido.
static int imin_i(int a, int b) { return (a < b) ? a : b; }
static int imax_i(int a, int b) { return (a > b) ? a : b; }
static float fmin_f(float a, float b) { return (a < b) ? a : b; }
static float fmax_f(float a, float b) { return (a > b) ? a : b; }

// EN: Fixed-point 26.6 saturation bound -- see include/core/raster.h's "OVERFLOW" note. Chosen
//     generously (`262144.0px` in either axis) -- comfortably beyond any legitimate glyph render
//     size, small enough that doubling it (the largest intermediate sum this file's arithmetic
//     ever needs, e.g. `dx = x1 - x0` with both saturated) never overflows `int32_t`
//     (`2*16777216 = 33554432`, `INT32_MAX ~= 2.1e9`).
// PT: Limite de saturação fixed-point 26.6 -- ver a nota "OVERFLOW" do include/core/raster.h.
//     Escolhido generosamente (`262144.0px` em qualquer eixo) -- confortavelmente além de qualquer
//     tamanho de render de glyph legítimo, pequeno o bastante que dobrar ele (a maior soma
//     intermediária que a aritmética deste arquivo algum dia precisa, ex.: `dx = x1 - x0` com os
//     dois saturados) nunca estoura `int32_t` (`2*16777216 = 33554432`, `INT32_MAX ~= 2.1e9`).
#define GLX_RASTER_FX_MAX ((int32_t)16777216)

static int32_t saturate_fx(int64_t v) {
    if (v > (int64_t)GLX_RASTER_FX_MAX) return GLX_RASTER_FX_MAX;
    if (v < -(int64_t)GLX_RASTER_FX_MAX) return -GLX_RASTER_FX_MAX;
    return (int32_t)v;
}

// EN: `font_unit * scale_num / scale_den`, rounded to nearest (ties away from zero), computed in
//     `int64_t` (a hostile/huge `scale_num` combined with a `font_unit` near `INT16_MIN`/`MAX`
//     cannot overflow: `32767 * INT32_MAX` is ~7e13, comfortably inside `int64_t`'s ~9.2e18 range)
//     and SATURATED into `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]` before ever being returned --
//     this is the "coordenada * 64 pode estourar ... satura/clampa" case this ticket's brief
//     names, and `tests/test_raster.c` proves it with an executed case. `scale_den > 0` is a
//     PRECONDITION this function trusts its caller (`glx_rasterize_outline`) to have already
//     validated (`GLX_RASTER_ERR_INVALID_ARG` for `scale_den <= 0`) -- never called otherwise.
// PT: `font_unit * scale_num / scale_den`, arredondado pro mais próximo (empate longe de zero),
//     computado em `int64_t` (uma combinação hostil/enorme de `scale_num` com um `font_unit` perto
//     de `INT16_MIN`/`MAX` não consegue estourar: `32767 * INT32_MAX` é ~7e13, confortavelmente
//     dentro da faixa ~9.2e18 do `int64_t`) e SATURADA em `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]`
//     antes de algum dia ser retornada -- este é o caso "coordenada * 64 pode estourar ...
//     satura/clampa" que o brief desta tarefa nomeia, e o `tests/test_raster.c` prova com um caso
//     executado. `scale_den > 0` é uma PRECONDIÇÃO que esta função confia que quem chama
//     (`glx_rasterize_outline`) já validou (`GLX_RASTER_ERR_INVALID_ARG` pra `scale_den <= 0`) --
//     nunca chamada de outro jeito.
static int32_t scale_to_fx(int32_t font_unit, int32_t scale_num, int32_t scale_den) {
    int64_t num = (int64_t)font_unit * (int64_t)scale_num;
    int64_t den = (int64_t)scale_den;
    int64_t half = den / 2;
    int64_t r = (num >= 0) ? (num + half) / den : -(((-num) + half) / den);
    return saturate_fx(r);
}

// EN: De Casteljau midpoint, 26.6 fixed-point -- exact floor-division-by-2 via arithmetic right
//     shift (see this file's header, item (1)). `a + b` cannot overflow `int32_t`: both operands
//     are already `saturate_fx`-bounded, `2*GLX_RASTER_FX_MAX` fits comfortably (see
//     `GLX_RASTER_FX_MAX`'s own comment).
// PT: Ponto-médio De Casteljau, fixed-point 26.6 -- divisão-por-2-com-floor exata via shift
//     aritmético à direita (ver o cabeçalho deste arquivo, item (1)). `a + b` não consegue estourar
//     `int32_t`: os dois operandos já são limitados por `saturate_fx`, `2*GLX_RASTER_FX_MAX` cabe
//     confortavelmente (ver o próprio comentário do `GLX_RASTER_FX_MAX`).
static int32_t mid_fx(int32_t a, int32_t b) { return (a + b) >> 1; }

// ================================================================================================
// EN: The rasterization context -- the caller's `scratch` buffer sliced into three regions (see
//     include/core/raster.h's `glx_raster_scratch_floats` doc-comment for the exact layout/sizing
//     this mirrors): `area`/`cover` (`w*h` each, row-major) and `leading` (`h`, one "off-left
//     cover" accumulator per row -- see this file's header, item (2), for why this cannot be
//     folded into a `w`-th virtual column of `area`/`cover`).
// PT: O contexto de rasterização -- o buffer `scratch` de quem chama fatiado em três regiões (ver
//     o doc-comment do `glx_raster_scratch_floats` do include/core/raster.h pro layout/
//     dimensionamento exato que isto espelha): `area`/`cover` (`w*h` cada, row-major) e `leading`
//     (`h`, um acumulador de "cover à esquerda" por linha -- ver o cabeçalho deste arquivo, item
//     (2), pro porquê disso não poder ser dobrado numa coluna virtual `w`-ésima de `area`/`cover`).
// ================================================================================================
typedef struct {
    float* area;
    float* cover;
    float* leading;
    int w, h;
} raster_ctx;

static void ctx_zero(raster_ctx* ctx) {
    size_t plane = (size_t)ctx->w * (size_t)ctx->h;
    for (size_t i = 0; i < plane; i++) {
        ctx->area[i] = 0.0f;
        ctx->cover[i] = 0.0f;
    }
    for (int r = 0; r < ctx->h; r++) ctx->leading[r] = 0.0f;
}

// EN: Adds ONE edge's contribution, within row `row` and column `col`, to `area`/`cover`.
//     DERIVATION (Green's theorem / trapezoid reasoning, self-derived -- see this file's header):
//     within pixel cell `[col, col+1) x [row, row+1)`, an edge crossing at local x-positions
//     `[xL-col, xR-col]` (both in `[0,1]`) over a fraction `signed_dy` of the cell's own height
//     contributes (a) `area[cell] += signed_dy * (1 - avg_local_x)` -- the cell's OWN partial
//     coverage: for each infinitesimal y-slice of height `signed_dy` at local x-position `t`, the
//     fraction of the cell's WIDTH lying to the right of the edge is `(1 - t)`; `avg_local_x` is
//     the average of the two endpoints' local-x (exact, since local x is affine in y over this
//     sub-segment) so the integral collapses to this one multiply; and (b) `cover[cell] +=
//     signed_dy` UNCONDITIONALLY (not scaled by position within the cell) -- every pixel STRICTLY
//     TO THE RIGHT of this cell, in the same row, is entirely to the right of wherever exactly the
//     edge crossed within `[col, col+1)`, so the finishing sweep's running sum (this file's header,
//     item (2)) must add the FULL `signed_dy`, not a partial one, once it moves past this column.
//     Sanity-checked in this file's own header comment against a perfectly vertical edge at the
//     cell's midpoint (`avg_local_x = 0.5` -> `area += 0.5*signed_dy`, `cover += signed_dy` ->
//     the cell itself resolves to exactly half-covered, every cell to its right fully covered,
//     every cell to its left uncovered -- matches the geometric picture exactly).
// PT: Soma a contribuição de UMA aresta, dentro da linha `row` e coluna `col`, pra `area`/`cover`.
//     DERIVAÇÃO (teorema de Green / raciocínio trapezoidal, auto-derivado -- ver o cabeçalho deste
//     arquivo): dentro da célula de pixel `[col, col+1) x [row, row+1)`, uma aresta cruzando em
//     posições-x locais `[xL-col, xR-col]` (ambas em `[0,1]`) ao longo de uma fração `signed_dy` da
//     própria altura da célula contribui (a) `area[célula] += signed_dy * (1 - avg_local_x)` -- a
//     PRÓPRIA cobertura parcial da célula: pra cada fatia-y infinitesimal de altura `signed_dy` na
//     posição-x local `t`, a fração da LARGURA da célula à direita da aresta é `(1 - t)`;
//     `avg_local_x` é a média do x-local dos dois extremos (exata, já que o x local é afim em y
//     sobre este subsegmento) então a integral colapsa pra esta única multiplicação; e (b)
//     `cover[célula] += signed_dy` INCONDICIONALMENTE (não escalado pela posição dentro da célula)
//     -- todo pixel ESTRITAMENTE À DIREITA desta célula, na mesma linha, está inteiramente à
//     direita de onde quer que a aresta tenha cruzado dentro de `[col, col+1)`, então a soma
//     corrente da varredura final (cabeçalho deste arquivo, item (2)) precisa somar o `signed_dy`
//     INTEIRO, não um parcial, assim que passar desta coluna. Conferido por sanidade no próprio
//     comentário de cabeçalho deste arquivo contra uma aresta perfeitamente vertical no meio da
//     célula (`avg_local_x = 0.5` -> `area += 0.5*signed_dy`, `cover += signed_dy` -> a própria
//     célula resolve pra exatamente meio-coberta, toda célula à direita dela totalmente coberta,
//     toda célula à esquerda dela descoberta -- bate exatamente com o quadro geométrico).
static void accum_column(raster_ctx* ctx, int row, int col, float signed_dy, float xL, float xR) {
    float local_lo = xL - (float)col;
    float local_hi = xR - (float)col;
    float avg = 0.5f * (local_lo + local_hi);
    size_t idx = (size_t)row * (size_t)ctx->w + (size_t)col;
    ctx->area[idx] += signed_dy * (1.0f - avg);
    ctx->cover[idx] += signed_dy;
}

// EN: Handles ONE edge's contribution within a single scanline row `row` (`ry0 < ry1`, both
//     already clipped to `[row, row+1]`), where the edge's x-position at `ry0`/`ry1` is
//     `xr0`/`xr1` (may be equal -- vertical-within-row -- or in either order). `winding_dir` is
//     `+1`/`-1` (the ORIGINAL edge's y-direction, established once by the caller for the whole
//     edge, see `process_edge` below) and is applied here to every `signed_dy` this function
//     computes/passes down (`dy_row`/local fractions are always POSITIVE magnitudes; the sign
//     lives entirely in `winding_dir`, multiplied in at the point each contribution is finally
//     added).
//
//     THREE-WAY SPLIT, avoiding an unbounded column loop (hostile-input safety -- a font-supplied
//     edge can have an arbitrarily large or negative x, e.g. a corrupted/adversarial `glyf`
//     record): this row-segment's x-range may extend left of `x=0` and/or right of `x=w` (the
//     bitmap's own width) by an ARBITRARY amount, but only the portion inside `[0, w]` needs a
//     per-COLUMN loop (bounded to at most `w` iterations, since `w` is the caller's own bitmap
//     width). The two boundary y-values (`y_at0`/`y_atw`, where this row-segment's x hits exactly
//     `0`/`w`) are computed directly via the edge's own x(y) inverse -- no loop needed to find
//     them -- then clamped into `[ry0, ry1]` (a segment entirely on one side of a boundary makes
//     its own boundary-crossing y fall outside `[ry0,ry1]`, and clamping collapses that to a
//     zero-width sub-interval, which the loop below skips harmlessly). Sorting the 4 values
//     `{ry0, y_at0, y_atw, ry1}` (a 4-element insertion sort) yields up to 3 consecutive
//     sub-intervals; each is classified by its OWN midpoint's x (`x < 0` -> folds into
//     `ctx->leading[row]`, cover-only, no visible cell to hold "area" for it; `x >= w` -> dropped
//     entirely, no visible cell exists to its right either; otherwise -> the VISIBLE case, handed
//     to the per-column loop, now guaranteed `x` in `[0, w]`).
// PT: Trata a contribuição de UMA aresta dentro de uma única linha de scanline `row` (`ry0 < ry1`,
//     ambos já clipados a `[row, row+1]`), onde a posição-x da aresta em `ry0`/`ry1` é `xr0`/`xr1`
//     (podem ser iguais -- vertical-dentro-da-linha -- ou em qualquer ordem). `winding_dir` é
//     `+1`/`-1` (a direção-y da aresta ORIGINAL, estabelecida uma vez por quem chama pra aresta
//     inteira, ver `process_edge` abaixo) e é aplicada aqui a todo `signed_dy` que esta função
//     computa/repassa (`dy_row`/frações locais são sempre magnitudes POSITIVAS; o sinal mora
//     inteiramente em `winding_dir`, multiplicado no ponto em que cada contribuição é finalmente
//     somada).
//
//     DIVISÃO EM TRÊS, evitando um laço de coluna sem limite (segurança de input hostil -- uma
//     aresta fornecida por fonte pode ter um x arbitrariamente grande ou negativo, ex.: um registro
//     `glyf` corrompido/adversarial): a faixa-x deste segmento-de-linha pode se estender à esquerda
//     de `x=0` e/ou à direita de `x=w` (a própria largura do bitmap) por uma quantidade
//     ARBITRÁRIA, mas só a porção dentro de `[0, w]` precisa de um laço por-COLUNA (limitado a no
//     máximo `w` iterações, já que `w` é a própria largura do bitmap de quem chama). Os dois
//     valores-y de fronteira (`y_at0`/`y_atw`, onde o x deste segmento-de-linha bate exatamente em
//     `0`/`w`) são computados diretamente via a inversa do próprio x(y) da aresta -- nenhum laço
//     necessário pra achá-los -- depois limitados a `[ry0, ry1]` (um segmento inteiramente de um
//     lado de uma fronteira faz o próprio y de cruzamento-de-fronteira cair fora de `[ry0,ry1]`, e
//     limitar colapsa isso pra um subintervalo de largura zero, que o laço abaixo pula
//     inofensivamente). Ordenar os 4 valores `{ry0, y_at0, y_atw, ry1}` (um insertion sort de 4
//     elementos) produz até 3 subintervalos consecutivos; cada um é classificado pelo x do PRÓPRIO
//     ponto-médio (`x < 0` -> dobra em `ctx->leading[row]`, só-cover, nenhuma célula visível pra
//     segurar "area" pra ela; `x >= w` -> descartado inteiramente, nenhuma célula visível existe à
//     direita dela tampouco; senão -> o caso VISÍVEL, repassado pro laço por-coluna, agora
//     garantido `x` em `[0, w]`).
static void accum_row_segment(raster_ctx* ctx, int row, float ry0, float ry1, float xr0, float xr1,
                               float winding_dir) {
    float dy_row = ry1 - ry0;
    float wf = (float)ctx->w;

    if (xr0 == xr1) {
        float x = xr0;
        float signed_dy_row = winding_dir * dy_row;
        if (x <= 0.0f) {
            ctx->leading[row] += signed_dy_row;
            return;
        }
        if (x >= wf) return;
        int col = (int)x;
        if (col >= ctx->w) col = ctx->w - 1;
        if (col < 0) col = 0;
        accum_column(ctx, row, col, signed_dy_row, x, x);
        return;
    }

    float denom = xr1 - xr0;
    float y_at0 = ry0 + (0.0f - xr0) / denom * dy_row;
    float y_atw = ry0 + (wf - xr0) / denom * dy_row;
    y_at0 = fmax_f(ry0, fmin_f(ry1, y_at0));
    y_atw = fmax_f(ry0, fmin_f(ry1, y_atw));

    float ys[4] = {ry0, y_at0, y_atw, ry1};
    for (int i = 1; i < 4; i++) {
        float key = ys[i];
        int j = i - 1;
        while (j >= 0 && ys[j] > key) {
            ys[j + 1] = ys[j];
            j--;
        }
        ys[j + 1] = key;
    }

    for (int i = 0; i < 3; i++) {
        float s0 = ys[i], s1 = ys[i + 1];
        if (s1 <= s0) continue;
        float local_dy = s1 - s0;
        float local_signed_dy = winding_dir * local_dy;
        float ymid = 0.5f * (s0 + s1);
        float xmid = xr0 + (ymid - ry0) / dy_row * denom;

        if (xmid <= 0.0f) {
            ctx->leading[row] += local_signed_dy;
            continue;
        }
        if (xmid >= wf) continue;

        float xs0 = xr0 + (s0 - ry0) / dy_row * denom;
        float xs1 = xr0 + (s1 - ry0) / dy_row * denom;
        xs0 = fmax_f(0.0f, fmin_f(wf, xs0));
        xs1 = fmax_f(0.0f, fmin_f(wf, xs1));

        float xlo = fmin_f(xs0, xs1);
        float xhi = fmax_f(xs0, xs1);
        int col_a = imin_i(ctx->w - 1, imax_i(0, (int)xs0));
        int col_b = imin_i(ctx->w - 1, imax_i(0, (int)xs1));

        if (col_a == col_b) {
            accum_column(ctx, row, col_a, local_signed_dy, xlo, xhi);
            continue;
        }

        // EN: Spans multiple columns (a shallow-sloped edge within this row) -- walk columns IN Y
        //     ORDER (from the column holding `xs0`, at `y=s0`, to the one holding `xs1`, at
        //     `y=s1`), stepping by `sign(col_b - col_a)`. This is NOT the same as always walking
        //     columns low-to-high: if `xs1 < xs0` (x DECREASES as y increases within this
        //     sub-segment), the column containing `xs0` is the HIGHER index and must be visited
        //     FIRST (it corresponds to the smaller y). Getting this backwards would assign each
        //     column's area/cover to the wrong sub-range of y (still summing to the right TOTAL
        //     `local_signed_dy` across the row, but distributed among columns incorrectly) --
        //     exactly the bug this comment exists to flag for the next reader. `t = (boundary_x -
        //     xs0) / (xs1 - xs0)` is direction-agnostic (valid algebra regardless of the sign of
        //     `xs1 - xs0`), only the WALK ORDER needs the explicit `step`.
        // PT: Se estende por várias colunas (uma aresta de inclinação rasa dentro desta linha) --
        //     percorre colunas NA ORDEM DE Y (da coluna que contém `xs0`, em `y=s0`, até a que
        //     contém `xs1`, em `y=s1`), avançando por `sign(col_b - col_a)`. Isto NÃO é o mesmo que
        //     sempre percorrer colunas de baixo pra cima: se `xs1 < xs0` (x DECRESCE conforme y
        //     cresce dentro deste subsegmento), a coluna contendo `xs0` é o índice MAIOR e precisa
        //     ser visitada PRIMEIRO (corresponde ao y menor). Inverter isso atribuiria a
        //     área/cover de cada coluna à subfaixa errada de y (ainda somando pro TOTAL certo
        //     `local_signed_dy` ao longo da linha, mas distribuído entre colunas incorretamente) --
        //     exatamente o bug que este comentário existe pra sinalizar pro próximo leitor.
        //     `t = (boundary_x - xs0) / (xs1 - xs0)` é agnóstico de direção (álgebra válida
        //     independente do sinal de `xs1 - xs0`), só a ORDEM DE PERCURSO precisa do `step`
        //     explícito.
        int step = (col_b >= col_a) ? 1 : -1;
        float prev_y = s0;
        int c = col_a;
        for (;;) {
            int is_last = (c == col_b);
            float y_here;
            if (is_last) {
                y_here = s1;
            } else {
                float boundary_x = (step > 0) ? (float)(c + 1) : (float)c;
                float t = (boundary_x - xs0) / (xs1 - xs0);
                y_here = s0 + t * (s1 - s0);
                y_here = fmax_f(prev_y, fmin_f(s1, y_here));
            }
            float seg_dy = y_here - prev_y;
            if (seg_dy > 0.0f) {
                float cell_lo = fmax_f((float)c, xlo);
                float cell_hi = fmin_f((float)(c + 1), xhi);
                if (cell_hi < cell_lo) cell_hi = cell_lo;
                accum_column(ctx, row, c, winding_dir * seg_dy, cell_lo, cell_hi);
            }
            prev_y = y_here;
            if (is_last) break;
            c += step;
        }
    }
}

// EN: Processes ONE flattened line-segment edge, in device PIXEL space (float -- see this file's
//     header for the fixed-point/float split justification), against the whole bitmap height:
//     skips horizontal edges (`y0 == y1`, the "divisão por zero" case this ticket's brief names --
//     a horizontal edge contributes zero winding by construction, no division ever attempted),
//     clips the edge's own y-extent to `[0, h]`, then walks each integer scanline row it crosses,
//     handing each row off to `accum_row_segment` with that row's own clipped `(ry0, ry1, xr0,
//     xr1)` sub-segment.
// PT: Processa UMA aresta de segmento-de-reta achatado, em espaço PIXEL de dispositivo (float --
//     ver o cabeçalho deste arquivo pra justificativa da divisão fixed-point/float), contra a
//     altura inteira do bitmap: pula arestas horizontais (`y0 == y1`, o caso "divisão por zero" que
//     o brief desta tarefa nomeia -- uma aresta horizontal contribui winding zero por construção,
//     nenhuma divisão sequer tentada), clipa a própria extensão-y da aresta a `[0, h]`, depois
//     percorre cada linha de scanline inteira que ela cruza, repassando cada linha pro
//     `accum_row_segment` com o próprio subsegmento clipado `(ry0, ry1, xr0, xr1)` daquela linha.
static void process_edge(raster_ctx* ctx, float x0, float y0, float x1, float y1) {
    if (y0 == y1) return;

    float y_a = fmin_f(y0, y1);
    float y_b = fmax_f(y0, y1);
    float winding_dir = (y1 > y0) ? 1.0f : -1.0f;

    y_a = fmax_f(y_a, 0.0f);
    y_b = fmin_f(y_b, (float)ctx->h);
    if (y_b <= y_a) return;

    float dxdy = (x1 - x0) / (y1 - y0);

    int row_start = imax_i(0, (int)y_a);
    // EN: `(int)y_b` may be ONE ROW PAST the last row this edge actually touches (e.g. `y_b ==
    //     4.0` exactly -> `(int)y_b == 4`, but the edge's own y-extent ends AT row 4's own top
    //     boundary, contributing nothing to row 4 itself) -- deliberately not "corrected" for
    //     that off-by-one here: the loop body's own `if (ry1 <= ry0) continue;` check is the
    //     actual authority (row 4 would compute `ry0 = max(y_a, 4.0) = 4.0`, `ry1 = min(y_b, 5.0)
    //     = 4.0`, `ry1 <= ry0` true, skipped), so this slightly conservative `row_end` costs at
    //     most one harmless extra zero-width loop iteration, never a missed row.
    // PT: `(int)y_b` pode estar UMA LINHA ALÉM da última linha que esta aresta de fato toca (ex.:
    //     `y_b == 4.0` exatamente -> `(int)y_b == 4`, mas a própria extensão-y da aresta termina
    //     NA própria fronteira do topo da linha 4, não contribuindo nada pra linha 4 em si) --
    //     deliberadamente não "corrigido" pra esse off-by-one aqui: a própria checagem
    //     `if (ry1 <= ry0) continue;` do corpo do laço é a autoridade de fato (a linha 4
    //     computaria `ry0 = max(y_a, 4.0) = 4.0`, `ry1 = min(y_b, 5.0) = 4.0`, `ry1 <= ry0`
    //     verdadeiro, pulada), então este `row_end` levemente conservador custa no máximo uma
    //     iteração de laço extra inofensiva de largura zero, nunca uma linha perdida.
    int row_end = imin_i(ctx->h - 1, (int)y_b);

    for (int row = row_start; row <= row_end; row++) {
        float ry0 = fmax_f(y_a, (float)row);
        float ry1 = fmin_f(y_b, (float)(row + 1));
        if (ry1 <= ry0) continue;
        float xr0 = x0 + (ry0 - y0) * dxdy;
        float xr1 = x0 + (ry1 - y0) * dxdy;
        accum_row_segment(ctx, row, ry0, ry1, xr0, xr1, winding_dir);
    }
}

// EN: Converts a device-space 26.6 fixed-point point pair into float pixels and feeds
//     `process_edge` -- the exact boundary between the mandated-fixed-point flattening stage and
//     the justified-float rasterization stage (this file's header).
// PT: Converte um par de ponto 26.6 fixed-point de espaço-de-dispositivo em pixels float e repassa
//     pro `process_edge` -- a fronteira exata entre o estágio de flattening mandado-fixed-point e o
//     estágio de rasterização float-justificado (cabeçalho deste arquivo).
static void emit_line_fx(raster_ctx* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    process_edge(ctx, (float)x0 / 64.0f, (float)y0 / 64.0f, (float)x1 / 64.0f, (float)y1 / 64.0f);
}

// EN: Maximum De Casteljau recursion depth -- a hard backstop (this file's header, item (1)):
//     `2^GLX_RASTER_MAX_QUAD_DEPTH` line segments worst case per quadratic, plenty for any
//     legitimate glyph curve (Open Sans' own curves flatten to well under a dozen segments at
//     typical UI sizes under the flatness test alone) while bounding even a pathological/hostile
//     curve the flatness heuristic never satisfies (e.g. `P0 == P2`, a "loop").
// PT: Profundidade máxima de recursão De Casteljau -- um backstop rígido (cabeçalho deste arquivo,
//     item (1)): `2^GLX_RASTER_MAX_QUAD_DEPTH` segmentos de reta no pior caso por quadrática,
//     folgado pra qualquer curva de glyph legítima (as próprias curvas da Open Sans achatam pra
//     bem menos que uma dúzia de segmentos em tamanhos de UI típicos só com o teste de flatness) ao
//     mesmo tempo que limita mesmo uma curva patológica/hostil que a heurística de flatness nunca
//     satisfaz (ex.: `P0 == P2`, um "laço").
#define GLX_RASTER_MAX_QUAD_DEPTH 8

// EN: Flatness tolerance, 26.6 units -- see this file's header, item (1), for the full
//     cross-product-vs-chord-extent derivation. `8` (26.6 units) = `1/8 px` deviation order of
//     magnitude at the tolerance's OWN scale (the heuristic's looseness means this is not an exact
//     perpendicular-distance bound, but comfortably sub-pixel for anti-aliased glyph rendering).
// PT: Tolerância de flatness, unidades 26.6 -- ver o cabeçalho deste arquivo, item (1), pra
//     derivação completa produto-vetorial-vs-extensão-da-corda. `8` (unidades 26.6) = ordem de
//     grandeza de desvio `1/8 px` na própria escala da tolerância (a soltura da heurística
//     significa que isto não é um limite exato de distância perpendicular, mas confortavelmente
//     sub-pixel pra renderização de glyph anti-aliased).
#define GLX_RASTER_FLATNESS_TOL 8

static void flatten_quad(raster_ctx* ctx, int32_t p0x, int32_t p0y, int32_t p1x, int32_t p1y,
                          int32_t p2x, int32_t p2y, int depth) {
    int32_t dx = p2x - p0x, dy = p2y - p0y;
    int32_t cx = p1x - p0x, cy = p1y - p0y;
    int64_t cross = (int64_t)cx * dy - (int64_t)cy * dx;
    int64_t cross_abs = (cross < 0) ? -cross : cross;
    int32_t chord = imax_i(iabs32(dx), iabs32(dy));
    if (chord < 1) chord = 1;

    if (depth >= GLX_RASTER_MAX_QUAD_DEPTH ||
        cross_abs <= (int64_t)GLX_RASTER_FLATNESS_TOL * (int64_t)chord) {
        emit_line_fx(ctx, p0x, p0y, p2x, p2y);
        return;
    }

    int32_t q0x = mid_fx(p0x, p1x), q0y = mid_fx(p0y, p1y);
    int32_t q1x = mid_fx(p1x, p2x), q1y = mid_fx(p1y, p2y);
    int32_t rx = mid_fx(q0x, q1x), ry = mid_fx(q0y, q1y);

    flatten_quad(ctx, p0x, p0y, q0x, q0y, rx, ry, depth + 1);
    flatten_quad(ctx, rx, ry, q1x, q1y, p2x, p2y, depth + 1);
}

// ================================================================================================
// EN: Contour walking -- reconstructs the TrueType on-curve/off-curve point stream (SOV-SFNT's
//     `glx_sfnt_point`) into a sequence of straight-line and quadratic-Bezier edges, per the
//     PUBLIC TrueType/OpenType outline spec's own reconstruction rule (this ticket's brief
//     restates it: two consecutive off-curve points imply an on-curve point exactly halfway
//     between them; a contour may start off-curve). Device-space (26.6, already scaled+origin-
//     offset+Y-flipped -- see raster.h) points are computed ONCE per source point via `to_device`
//     and cached in a small `device_pt` struct, since a single font-unit point may be READ TWICE
//     (once as this segment's end, once as the next segment's start) but should not be re-scaled
//     twice (harmless either way -- `scale_to_fx` is pure/deterministic -- but wasteful).
// PT: Percurso de contorno -- reconstrói o fluxo de ponto on-curve/off-curve TrueType
//     (`glx_sfnt_point` do SOV-SFNT) numa sequência de arestas reta e Bezier-quadrática, conforme a
//     própria regra de reconstrução da spec PÚBLICA de outline TrueType/OpenType (o brief desta
//     tarefa a reafirma: dois pontos off-curve consecutivos implicam um ponto on-curve exatamente
//     na metade entre eles; um contorno pode começar off-curve). Pontos em espaço-de-dispositivo
//     (26.6, já escalados+deslocados-por-origem+Y-flipados -- ver raster.h) são computados UMA VEZ
//     por ponto-fonte via `to_device` e guardados numa pequena struct `device_pt`, já que um único
//     ponto em unidade-de-fonte pode ser LIDO DUAS VEZES (uma como fim deste segmento, outra como
//     início do próximo) mas não deveria ser re-escalado duas vezes (inofensivo de qualquer jeito
//     -- `scale_to_fx` é pura/determinística -- mas desperdiçado).
// ================================================================================================
typedef struct {
    int32_t x, y;
    uint8_t on_curve;
} device_pt;

static device_pt to_device(const glx_sfnt_point* p, int32_t scale_num, int32_t scale_den,
                            int32_t origin_x, int32_t origin_y) {
    device_pt d;
    d.x = saturate_fx((int64_t)origin_x + (int64_t)scale_to_fx(p->x, scale_num, scale_den));
    d.y = saturate_fx((int64_t)origin_y - (int64_t)scale_to_fx(p->y, scale_num, scale_den));
    d.on_curve = p->on_curve;
    return d;
}

static device_pt device_midpoint(device_pt a, device_pt b) {
    device_pt r;
    r.x = mid_fx(a.x, b.x);
    r.y = mid_fx(a.y, b.y);
    r.on_curve = 1;
    return r;
}

// EN: `pt_at(k)` abstraction for the 3 contour-start cases (this file's header, contour-walking
//     note) -- avoids physically rotating/copying the source point array. `mode`: 0 = contour
//     starts on-curve (`pts[0]`), walk = `pts[(1+k)%n]` for `k` in `[0,n)` (wraps back to `pts[0]`
//     exactly at `k=n-1`, closing for free); 1 = contour starts off-curve but ENDS on-curve
//     (`pts[n-1]`), walk = `pts[k%n]` for `k` in `[0,n)` (identity order, closes for free since
//     `pts[n-1]` IS `start`); 2 = both ends off-curve, `start` is the SYNTHESIZED midpoint of
//     `pts[0]`/`pts[n-1]`, walk = `pts[k]` for `k<n`, then the synthesized `start` again at
//     `k==n` (an explicit close, since no real point plays that role).
// PT: Abstração `pt_at(k)` pros 3 casos de início-de-contorno (nota de percurso-de-contorno do
//     cabeçalho deste arquivo) -- evita rotacionar/copiar fisicamente o array de ponto-fonte.
//     `mode`: 0 = contorno começa on-curve (`pts[0]`), percurso = `pts[(1+k)%n]` pra `k` em
//     `[0,n)` (dá a volta de volta pra `pts[0]` exatamente em `k=n-1`, fechando de graça); 1 =
//     contorno começa off-curve mas TERMINA on-curve (`pts[n-1]`), percurso = `pts[k%n]` pra `k`
//     em `[0,n)` (ordem identidade, fecha de graça já que `pts[n-1]` É `start`); 2 = ambos os
//     extremos off-curve, `start` é o ponto-médio SINTETIZADO de `pts[0]`/`pts[n-1]`, percurso =
//     `pts[k]` pra `k<n`, depois o `start` sintetizado de novo em `k==n` (um fecho explícito, já
//     que nenhum ponto real faz esse papel).
typedef struct {
    const glx_sfnt_point* pts;
    uint16_t n;
    int mode;
    device_pt synthetic_start;
    int32_t scale_num, scale_den, origin_x, origin_y;
} contour_walker;

static device_pt walker_pt_at(const contour_walker* w, uint16_t k) {
    if (w->mode == 0) {
        uint16_t idx = (uint16_t)((1 + (uint32_t)k) % w->n);
        return to_device(&w->pts[idx], w->scale_num, w->scale_den, w->origin_x, w->origin_y);
    }
    if (w->mode == 1) {
        uint16_t idx = (uint16_t)(k % w->n);
        return to_device(&w->pts[idx], w->scale_num, w->scale_den, w->origin_x, w->origin_y);
    }
    // mode == 2
    if (k < w->n) {
        return to_device(&w->pts[k], w->scale_num, w->scale_den, w->origin_x, w->origin_y);
    }
    return w->synthetic_start;
}

static void process_contour(raster_ctx* ctx, const glx_sfnt_point* pts, uint16_t n,
                             int32_t scale_num, int32_t scale_den, int32_t origin_x,
                             int32_t origin_y) {
    if (n < 2) return; // EN: degenerate contour, ignored cleanly (this ticket's brief).
                        // PT: contorno degenerado, ignorado limpo (brief desta tarefa).

    contour_walker w;
    w.pts = pts;
    w.n = n;
    w.scale_num = scale_num;
    w.scale_den = scale_den;
    w.origin_x = origin_x;
    w.origin_y = origin_y;

    device_pt start;
    uint16_t walk_len;
    if (pts[0].on_curve) {
        w.mode = 0;
        start = to_device(&pts[0], scale_num, scale_den, origin_x, origin_y);
        walk_len = n;
    } else if (pts[n - 1].on_curve) {
        w.mode = 1;
        start = to_device(&pts[n - 1], scale_num, scale_den, origin_x, origin_y);
        walk_len = n;
    } else {
        device_pt a = to_device(&pts[0], scale_num, scale_den, origin_x, origin_y);
        device_pt b = to_device(&pts[n - 1], scale_num, scale_den, origin_x, origin_y);
        start = device_midpoint(a, b);
        w.mode = 2;
        w.synthetic_start = start;
        walk_len = (uint16_t)(n + 1);
    }

    device_pt current = start;
    uint16_t i = 0;
    while (i < walk_len) {
        device_pt p = walker_pt_at(&w, i);
        if (p.on_curve) {
            emit_line_fx(ctx, current.x, current.y, p.x, p.y);
            current = p;
            i = (uint16_t)(i + 1);
        } else {
            device_pt end;
            uint16_t consumed;
            if (i + 1 < walk_len) {
                device_pt q = walker_pt_at(&w, (uint16_t)(i + 1));
                if (q.on_curve) {
                    end = q;
                    consumed = 2;
                } else {
                    end = device_midpoint(p, q);
                    consumed = 1;
                }
            } else {
                // EN: Defensive fallback -- by construction `walk_len` always ends on an on-curve
                //     point (real or synthetic), so this branch should be unreachable; kept as a
                //     hard, bounded, non-UB terminator in case a future edit ever breaks that
                //     invariant (see this ticket's overarching "NUNCA UB" mandate).
                // PT: Fallback defensivo -- por construção `walk_len` sempre termina num ponto
                //     on-curve (real ou sintético), então este ramo deveria ser inalcançável;
                //     mantido como um terminador rígido, limitado, sem-UB caso uma edição futura
                //     algum dia quebre esse invariante (ver o mandato abrangente "NUNCA UB" desta
                //     tarefa).
                end = start;
                consumed = 1;
            }
            flatten_quad(ctx, current.x, current.y, p.x, p.y, end.x, end.y, 0);
            current = end;
            i = (uint16_t)(i + consumed);
        }
    }
}

// ================================================================================================
// EN: Public API.
// PT: API pública.
// ================================================================================================

size_t glx_raster_scratch_floats(int w, int h) {
    if (w <= 0 || h <= 0) return 0;
    size_t wu = (size_t)w, hu = (size_t)h;
    // EN: overflow-safe `2*w*h + h`: `wh = w*h` checked first, then `2*wh` checked, then `+h`
    //     checked -- every step verified before committing, never trusting `w*h*2+h` to just not
    //     overflow.
    // PT: `2*w*h + h` seguro-contra-overflow: `wh = w*h` checado primeiro, depois `2*wh` checado,
    //     depois `+h` checado -- todo passo verificado antes de comprometer, nunca confiando que
    //     `w*h*2+h` simplesmente não estoure.
    if (wu > (SIZE_MAX / hu)) return 0;
    size_t wh = wu * hu;
    if (wh > (SIZE_MAX - wh)) return 0;
    size_t two_wh = wh + wh;
    if (two_wh > (SIZE_MAX - hu)) return 0;
    return two_wh + hu;
}

glx_raster_result glx_rasterize_outline(const glx_sfnt_point* points, uint16_t num_points,
                                         const uint16_t* contour_ends, uint16_t num_contours,
                                         int32_t scale_num, int32_t scale_den,
                                         int32_t origin_x_26_6, int32_t origin_y_26_6,
                                         uint8_t* bitmap_out, int w, int h, int stride,
                                         float* scratch, size_t scratch_capacity_floats) {
    if (points == NULL && num_points > 0) return GLX_RASTER_ERR_INVALID_ARG;
    if (contour_ends == NULL && num_contours > 0) return GLX_RASTER_ERR_INVALID_ARG;
    if (bitmap_out == NULL) return GLX_RASTER_ERR_INVALID_ARG;
    if (scale_den <= 0) return GLX_RASTER_ERR_INVALID_ARG;
    if (w <= 0 || h <= 0 || stride < w) return GLX_RASTER_ERR_INVALID_ARG;

    size_t needed = glx_raster_scratch_floats(w, h);
    if (needed == 0) return GLX_RASTER_ERR_INVALID_ARG; // unrepresentable (w,h) -- see header.
    if (scratch == NULL) return GLX_RASTER_ERR_INVALID_ARG;
    if (scratch_capacity_floats < needed) return GLX_RASTER_ERR_SCRATCH_TOO_SMALL;

    raster_ctx ctx;
    ctx.area = scratch;
    ctx.cover = scratch + (size_t)w * (size_t)h;
    ctx.leading = scratch + 2 * (size_t)w * (size_t)h;
    ctx.w = w;
    ctx.h = h;
    ctx_zero(&ctx);

    uint16_t start_idx = 0;
    for (uint16_t c = 0; c < num_contours; c++) {
        uint16_t end_idx = contour_ends[c];
        if (end_idx < start_idx || end_idx >= num_points) {
            // EN: A malformed contour_ends entry (out of range / non-increasing) -- skip this
            //     contour cleanly rather than reading out of `points`' bounds. Matches SOV-SFNT's
            //     own "clean error, never UB" posture, adapted here to "skip, keep going" since
            //     unlike a parse failure, a partial glyph raster is still a well-defined bitmap
            //     (all-zero for the skipped contour) rather than something that must abort the
            //     whole call.
            // PT: Uma entrada `contour_ends` malformada (fora de faixa / não-crescente) -- pula
            //     este contorno de forma limpa em vez de ler fora dos limites de `points`. Casa
            //     com a própria postura "erro limpo, nunca UB" do SOV-SFNT, adaptada aqui pra
            //     "pula, continua" já que, diferente de uma falha de parse, um raster parcial de
            //     glyph ainda é um bitmap bem-definido (tudo-zero pro contorno pulado) em vez de
            //     algo que precisa abortar a chamada inteira.
            start_idx = (uint16_t)(end_idx + 1);
            continue;
        }
        uint16_t n = (uint16_t)(end_idx - start_idx + 1);
        process_contour(&ctx, &points[start_idx], n, scale_num, scale_den, origin_x_26_6,
                         origin_y_26_6);
        start_idx = (uint16_t)(end_idx + 1);
    }

    // EN: Finishing sweep -- see this file's header, item (2). One pass per row, left to right:
    //     `running` starts at `leading[row]` (the off-left cover primed BEFORE column 0, this
    //     file's header explains why that cannot live in a `w*h`-indexed cell), each pixel's
    //     coverage is `area[cell] + running` (running = sum of `cover[]` for every column strictly
    //     to the left), then `running` picks up `cover[cell]` for the NEXT column.
    // PT: Varredura final -- ver o cabeçalho deste arquivo, item (2). Uma passada por linha, da
    //     esquerda pra direita: `running` começa em `leading[row]` (o cover-à-esquerda primado
    //     ANTES da coluna 0, o cabeçalho deste arquivo explica por que isso não pode morar numa
    //     célula indexada-por-`w*h`), a cobertura de cada pixel é `area[célula] + running`
    //     (running = soma de `cover[]` de toda coluna estritamente à esquerda), depois `running`
    //     pega o `cover[célula]` pra próxima coluna.
    for (int row = 0; row < h; row++) {
        float running = ctx.leading[row];
        uint8_t* out_row = bitmap_out + (size_t)row * (size_t)stride;
        for (int col = 0; col < w; col++) {
            size_t idx = (size_t)row * (size_t)w + (size_t)col;
            float coverage = ctx.area[idx] + running;
            running += ctx.cover[idx];

            float mag = fabs_f(coverage);
            if (mag != mag) mag = 0.0f; // NaN guard (this file's header idiom, src/sfnt.c's).
            int alpha = (int)(mag * 255.0f + 0.5f);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;
            out_row[col] = (uint8_t)alpha;
        }
    }

    return GLX_RASTER_OK;
}
