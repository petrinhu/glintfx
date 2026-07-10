// SPDX-License-Identifier: MPL-2.0
// EN: SOV-RAST (FT-F2, AUD-TRANS-2's 3rd piece) -- a clean-room, un-hinted quadratic-outline
//     ANALYTIC ANTI-ALIASED RASTERIZER, consuming `glx_sfnt_point`/contour-end streams (SOV-SFNT,
//     `include/core/sfnt.h`) and producing an 8-bit alpha-coverage bitmap. Built entirely from
//     PUBLIC, GENERIC computational-geometry knowledge -- Green's theorem / the shoelace formula
//     for signed polygon area, and the "trapezoidal signed-area accumulation" scanline technique
//     for exact analytic coverage (a general method taught in computer graphics courses and
//     described in multiple public, non-FreeType sources, e.g. Anti-Grain Geometry's public
//     "Anti-Aliasing" articles). **No FreeType code was read to write this file** -- FreeType is
//     not even vendored in this repository (CLAUDE.md's "ZERO bibliotecas" rule, and the
//     CLEAN-ROOM constraint this ticket's brief calls out explicitly as its highest-temptation
//     item). Every formula below is re-derived from first principles in this file's own comments
//     (see `flatten_quad`'s Green's-theorem area note and `accum_row_segment`'s trapezoid-area
//     derivation) -- not transcribed from any single library's internals.
//
//     WHAT THIS FILE IS: a PURE, points-in/bitmap-out rasterizer -- `glx_rasterize_outline` takes
//     a point+contour-end stream the CALLER already has (typically fresh out of
//     `glx_sfnt_glyph_outline`, but any caller-constructed contour works too -- this file has NO
//     dependency on `glx_sfnt_face`/`glx_sfnt_open`, only on the two small POD types
//     `glx_sfnt_point`/the `uint16_t` contour-end array shape SOV-SFNT already defined) and writes
//     directly into a caller-owned 8-bit bitmap. NO DYNAMIC ALLOCATION: the only scratch memory
//     this file ever touches is the caller-supplied `float* scratch` buffer (sized via
//     `glx_raster_scratch_floats`, below) -- there is no `glx_raster_close`/`glx_raster_free`
//     because there is nothing to free, matching SOV-SFNT's own "no allocator to corrupt" posture
//     (see `include/core/sfnt.h`'s file header for the shared rationale, hostile-input attack
//     surface first).
//
//     WHY <stdint.h>/<stddef.h>, NOT include/types.h; WHY #include "sfnt.h": same reasoning
//     `include/core/sfnt.h` already documents for itself -- this header lives under
//     `include/core/` (ADR-0009's physical boundary) so it is consumable from a future HOSTED C++
//     shell (`L1.19-FONTENG`, glintfx) the same way `glx_malloc`/`glx_sfnt_open` already are.
//     Including `"sfnt.h"` (a sibling `include/core/` header, not a Layer-0-internal one) is a
//     STRUCTURAL reuse of `glx_sfnt_point`'s type shape only -- this file has zero *semantic*
//     coupling to SFNT/TrueType concepts (`units_per_em`, `cmap`, `glyf`) beyond that one struct;
//     see `glx_rasterize_outline`'s own doc-comment for why the scale factor below is a generic
//     rational `(scale_num, scale_den)` pair, not an SFNT-flavoured `px_size`/`units_per_em` pair.
//
//     FIXED-POINT 26.6 (font-unit scaling + quadratic FLATTENING) vs FLOAT (analytic coverage
//     RASTERIZATION) -- a DELIBERATE, DOCUMENTED split, not an inconsistency: this ticket's brief
//     mandates fixed-point 26.6 (1 unit = 1/64 px) for the scale-to-device-space step and for
//     quadratic subdivision ("floor/ceil = shifts, NUNCA libm/float pra isso"), which `src/raster.c`
//     honours exactly -- `scale_to_fx` and `flatten_quad`'s De Casteljau midpoints are 100%
//     `int32_t` arithmetic (`+`, `-`, `*`, `>>1`, no division except the one bounded, overflow-
//     checked `int64_t` division in `scale_to_fx` itself). The brief ALSO explicitly allows float
//     "SE justificar" -- the justification here: exact analytic per-pixel trapezoidal-area
//     coverage inherently needs fractional-pixel multiply/divide (the `t`/`avg_x_local` fractions
//     in `accum_row_segment`/`accum_column`), and hand-rolling THAT in fixed-point risks silent
//     precision bugs for a net-zero determinism benefit (coverage output is already an 8-bit
//     ROUNDED alpha, so 26.6's 1/64 precision buys nothing float32's ~7 decimal digits does not
//     already dwarf). This mirrors an established precedent already in this exact codebase, not a
//     new "is float safe here" question -- `src/sfnt.c`'s `apply_component_transform` (composite
//     glyph 2x2 transform) and `src/conv.c`'s `ftoa`/`atof` (SOV-FCONV) both already use plain
//     compiler-native `float`/`double` arithmetic (`+ - * /`, never a `<math.h>`/libm CALL --
//     `sqrtf`/`floorf`/`sinf` etc. remain off-limits, this file calls none of them, see
//     `src/raster.c`'s own header for the manual floor/round/abs/NaN-guard idioms used instead,
//     copied from `src/sfnt.c`'s own `round_and_saturate_i16` pattern). Zero libm, proven the same
//     way SOV-FCONV proved it: `nm -u` on the compiled object shows no undefined libm symbol.
// PT: SOV-RAST (FT-F2, 3ª peça da AUD-TRANS-2) -- um RASTERIZADOR ANTI-ALIASED ANALÍTICO de
//     outline quadrático SEM HINTING, clean-room, consumindo os fluxos de `glx_sfnt_point`/fim-de-
//     contorno (SOV-SFNT, `include/core/sfnt.h`) e produzindo um bitmap de cobertura alpha 8-bit.
//     Construído inteiramente a partir de conhecimento PÚBLICO e GENÉRICO de geometria
//     computacional -- teorema de Green / fórmula do shoelace pra área de sinal de polígono, e a
//     técnica de scanline "acumulação de área com sinal trapezoidal" pra cobertura analítica exata
//     (um método geral ensinado em cursos de computação gráfica e descrito em múltiplas fontes
//     públicas não-FreeType, ex.: os artigos públicos "Anti-Aliasing" do Anti-Grain Geometry).
//     **Nenhum código do FreeType foi lido pra escrever este arquivo** -- o FreeType nem está
//     vendorizado neste repositório (regra "ZERO bibliotecas" do CLAUDE.md, e a restrição
//     CLEAN-ROOM que o brief desta tarefa nomeia explicitamente como seu item de maior tentação).
//     Toda fórmula abaixo é re-derivada do zero nos próprios comentários deste arquivo (ver a nota
//     de área via teorema de Green do `flatten_quad` e a derivação de área trapezoidal do
//     `accum_row_segment`) -- não transcrita de biblioteca nenhuma.
//
//     O QUE ESTE ARQUIVO É: um rasterizador PURO, ponto-entra/bitmap-sai -- `glx_rasterize_outline`
//     recebe um fluxo de ponto+fim-de-contorno que quem CHAMA já tem (tipicamente saído fresco do
//     `glx_sfnt_glyph_outline`, mas qualquer contorno construído por quem chama funciona também --
//     este arquivo NÃO tem dependência de `glx_sfnt_face`/`glx_sfnt_open`, só dos dois tipos POD
//     pequenos `glx_sfnt_point`/o formato de array `uint16_t` de fim-de-contorno que o SOV-SFNT já
//     definiu) e escreve direto num bitmap 8-bit possuído por quem chama. SEM ALOCAÇÃO DINÂMICA: a
//     única memória de rascunho que este arquivo algum dia toca é o buffer `float* scratch`
//     fornecido por quem chama (dimensionado via `glx_raster_scratch_floats`, abaixo) -- não há
//     `glx_raster_close`/`glx_raster_free` porque não há nada pra liberar, espelhando a própria
//     postura "nenhum alocador pra corromper" do SOV-SFNT (ver o cabeçalho de arquivo do
//     `include/core/sfnt.h` pro racional compartilhado, superfície de ataque de input hostil
//     primeiro).
//
//     POR QUE <stdint.h>/<stddef.h>, NÃO include/types.h; POR QUE #include "sfnt.h": mesmo
//     raciocínio que o `include/core/sfnt.h` já documenta pra si mesmo -- este header mora sob
//     `include/core/` (fronteira física da ADR-0009) então é consumível de uma futura casca C++
//     HOSTED (`L1.19-FONTENG`, glintfx) do mesmo jeito que `glx_malloc`/`glx_sfnt_open` já são.
//     Incluir `"sfnt.h"` (um header irmão de `include/core/`, não um interno-à-Camada-0) é um reuso
//     ESTRUTURAL só do formato de tipo do `glx_sfnt_point` -- este arquivo tem acoplamento
//     *semântico* zero a conceitos SFNT/TrueType (`units_per_em`, `cmap`, `glyf`) além dessa
//     struct; ver o próprio doc-comment do `glx_rasterize_outline` pro porquê do fator de escala
//     abaixo ser um par racional genérico `(scale_num, scale_den)`, não um par `px_size`/
//     `units_per_em` com sabor SFNT.
//
//     FIXED-POINT 26.6 (escala de unidade-de-fonte + FLATTENING quadrático) vs FLOAT (cobertura
//     RASTERIZAÇÃO analítica) -- uma divisão DELIBERADA, DOCUMENTADA, não uma inconsistência: o
//     brief desta tarefa manda fixed-point 26.6 (1 unidade = 1/64 px) pro passo de escala-pra-
//     espaço-de-dispositivo e pra subdivisão quadrática ("floor/ceil = shifts, NUNCA libm/float pra
//     isso"), que o `src/raster.c` honra exatamente -- os midpoints De Casteljau do `scale_to_fx` e
//     `flatten_quad` são 100% aritmética `int32_t` (`+`, `-`, `*`, `>>1`, sem divisão exceto a
//     única divisão `int64_t` limitada e checada-contra-overflow no próprio `scale_to_fx`). O
//     brief TAMBÉM permite float explicitamente "SE justificar" -- a justificativa aqui: cobertura
//     trapezoidal analítica exata por-pixel precisa inerentemente de multiplicação/divisão de
//     pixel fracionário (as frações `t`/`avg_x_local` no `accum_row_segment`/`accum_column`), e
//     fazer ISSO à mão em fixed-point arrisca bugs de precisão silenciosos por um benefício líquido
//     de determinismo zero (a saída de cobertura já é um alpha 8-bit ARREDONDADO, então a precisão
//     de 1/64 do 26.6 não compra nada que os ~7 dígitos decimais do float32 já não ofusquem). Isso
//     espelha um precedente já estabelecido neste MESMO código-base, não uma pergunta nova de
//     "float é seguro aqui" -- o `apply_component_transform` do `src/sfnt.c` (transformação 2x2 de
//     glyph composto) e o `ftoa`/`atof` do `src/conv.c` (SOV-FCONV) já usam aritmética `float`/
//     `double` nativa do compilador comum (`+ - * /`, nunca uma CHAMADA `<math.h>`/libm --
//     `sqrtf`/`floorf`/`sinf` etc. continuam fora de cogitação, este arquivo não chama nenhuma
//     delas, ver o cabeçalho próprio do `src/raster.c` pros idiomas manuais de
//     floor/round/abs/guarda-NaN usados em vez disso, copiados do próprio padrão
//     `round_and_saturate_i16` do `src/sfnt.c`). Zero libm, provado do mesmo jeito que o SOV-FCONV
//     provou: `nm -u` no objeto compilado não mostra símbolo libm indefinido.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/sfnt.h"

#ifdef __cplusplus
extern "C" {
#endif

// EN: Every fallible operation in this API returns one of these. `GLX_RASTER_OK == 0`, same
//     convention `glx_sfnt_result` already establishes (`include/core/sfnt.h`).
// PT: Toda operação falível desta API retorna um destes. `GLX_RASTER_OK == 0`, mesma convenção
//     que o `glx_sfnt_result` já estabelece (`include/core/sfnt.h`).
typedef enum {
    GLX_RASTER_OK = 0,
    // EN: A NULL required pointer, `num_points`/`num_contours` inconsistent with the buffers
    //     (e.g. non-zero count with a NULL pointer), `scale_den <= 0`, or `w <= 0`/`h <= 0`/
    //     `stride < w` -- or the `(w, h)` pair is large enough that `glx_raster_scratch_floats`
    //     itself cannot compute the required scratch size without overflowing `size_t` (see that
    //     function's own doc-comment).
    // PT: Um ponteiro obrigatório NULL, `num_points`/`num_contours` inconsistente com os buffers
    //     (ex.: contagem não-zero com um ponteiro NULL), `scale_den <= 0`, ou `w <= 0`/`h <= 0`/
    //     `stride < w` -- ou o par `(w, h)` é grande o bastante que o próprio
    //     `glx_raster_scratch_floats` não consegue computar o tamanho de scratch necessário sem
    //     estourar `size_t` (ver o doc-comment próprio daquela função).
    GLX_RASTER_ERR_INVALID_ARG,
    // EN: `scratch_capacity_floats` is smaller than `glx_raster_scratch_floats(w, h)` demands.
    //     `bitmap_out`'s contents are UNSPECIFIED on this result (nothing was written) -- only a
    //     `GLX_RASTER_OK` result guarantees every one of `bitmap_out`'s `[0, w) x [0, h)` cells was
    //     written.
    // PT: `scratch_capacity_floats` é menor do que `glx_raster_scratch_floats(w, h)` exige. O
    //     CONTEÚDO de `bitmap_out` fica NÃO-ESPECIFICADO neste resultado (nada foi escrito) -- só
    //     um resultado `GLX_RASTER_OK` garante que toda célula `[0, w) x [0, h)` de `bitmap_out`
    //     foi escrita.
    GLX_RASTER_ERR_SCRATCH_TOO_SMALL,
} glx_raster_result;

// EN: Computes the exact `float` count `glx_rasterize_outline` needs in its `scratch` buffer for
//     a `(w, h)`-sized bitmap: two full `w*h` planes (per-cell "area" and "cover" deltas, see
//     `src/raster.c`'s `accum_column`/`accum_row_segment`) plus one `h`-sized "leading cover"
//     column (edges that cross the LEFT edge of the bitmap, `x < 0` -- see `src/raster.c`'s file
//     header for why that needs its own row-indexed scratch rather than a `w`-th "virtual"
//     column). Returns `0` if `w <= 0`, `h <= 0`, or the computation `2*(size_t)w*(size_t)h +
//     (size_t)h` would overflow `size_t` -- `0` is never a legitimate scratch requirement for a
//     positive-area bitmap, so callers can treat it as an unambiguous "invalid dimensions" signal
//     (mirrored by `glx_rasterize_outline` itself returning `GLX_RASTER_ERR_INVALID_ARG` for the
//     same `(w, h)`).
// PT: Computa a contagem exata de `float` que o `glx_rasterize_outline` precisa no buffer
//     `scratch` pra um bitmap de tamanho `(w, h)`: dois planos `w*h` inteiros (deltas de "area" e
//     "cover" por-célula, ver `accum_column`/`accum_row_segment` do `src/raster.c`) mais uma
//     coluna de tamanho `h` de "cover à esquerda" (arestas que cruzam a borda ESQUERDA do bitmap,
//     `x < 0` -- ver o cabeçalho de arquivo do `src/raster.c` pro porquê disso precisar do próprio
//     scratch indexado-por-linha em vez de uma coluna "virtual" `w`-ésima). Retorna `0` se
//     `w <= 0`, `h <= 0`, ou a computação `2*(size_t)w*(size_t)h + (size_t)h` estourasse `size_t`
//     -- `0` nunca é uma exigência de scratch legítima pra um bitmap de área positiva, então quem
//     chama pode tratar isso como um sinal inequívoco de "dimensões inválidas" (espelhado pelo
//     próprio `glx_rasterize_outline` retornando `GLX_RASTER_ERR_INVALID_ARG` pro mesmo `(w, h)`).
size_t glx_raster_scratch_floats(int w, int h);

// EN: Flattens+rasterizes `points[0..num_points)` (partitioned into `num_contours` closed
//     contours via `contour_ends`, EXACTLY the `glx_sfnt_glyph_outline` convention: contour `i`
//     spans `points[contour_ends[i-1]+1 .. contour_ends[i]]`, `contour_ends[-1]` implicitly `-1`
//     for `i == 0`) into `bitmap_out`'s `[0, w) x [0, h)` cells (row-major, `stride` bytes per
//     row -- `bitmap_out[row*stride + col]` is coverage `0..255`), using the NON-ZERO winding
//     rule (a hole -- a second contour wound the opposite direction, e.g. the middle of 'O' --
//     correctly renders as uncovered; see `src/raster.c`'s file header for the analytic-coverage
//     algorithm). EVERY cell in `[0, w) x [0, h)` is written on `GLX_RASTER_OK` (full overwrite,
//     no dependency on `bitmap_out`'s prior contents).
//
//     COORDINATE MAPPING (font-unit point -> device pixel, see this header's own "FIXED-POINT
//     26.6 vs FLOAT" note for the two-stage precision split): for a font-unit point `(fx, fy)`,
//     `device_x_26_6 = origin_x_26_6 + round(fx * scale_num / scale_den)` and
//     `device_y_26_6 = origin_y_26_6 - round(fy * scale_num / scale_den)` -- note the MINUS on
//     the `y` term: TrueType/SFNT outlines are Y-UP (font_y increases upward, `glx_sfnt_point`'s
//     own convention, inherited from `include/core/sfnt.h`), every pixel bitmap is Y-DOWN (row 0
//     is the TOP row) -- this flip is a fixed, non-optional part of the format conversion this
//     function performs, not a caller-configurable policy. `(scale_num, scale_den)` is a generic
//     rational scale factor (`scale_den > 0` required) -- an SFNT caller passes
//     `(px_size_26_6, face->units_per_em)` (`px_size_26_6` = desired em size in 26.6 pixels, e.g.
//     `16.0px` -> `1024`); a caller rasterizing a hand-built, already-device-scaled contour (e.g.
//     this ticket's own oracle tests, `tests/test_raster.c`) passes `(64, 1)` for a 1:1 font-
//     unit-equals-pixel mapping (`64` because `glx_sfnt_point`'s `x`/`y` fields are `int16_t`
//     FONT units, and this function's own internal fixed-point unit is 1/64 px -- `scale_num=64,
//     scale_den=1` makes 1 font-unit equal exactly 1 device pixel). `(origin_x_26_6,
//     origin_y_26_6)` is a caller-chosen 26.6 device-space offset added AFTER scaling -- lets a
//     caller position a glyph's own bbox (`glx_sfnt_outline::x_min`/`y_max`) at any pixel offset
//     within `bitmap_out` (typically `origin_x_26_6 = -scale(x_min)`, `origin_y_26_6 =
//     scale(y_max)`, left/top-aligning the glyph inside the bitmap, plus any margin the caller
//     wants).
//
//     OVERFLOW: every scaled+offset device coordinate is SATURATED into
//     `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]` (`src/raster.c`) before any further arithmetic --
//     a hostile/degenerate `(scale_num, scale_den, origin)` combination that would otherwise
//     overflow `int32_t` 26.6 math clamps instead of wrapping/UB-ing (see this ticket's brief,
//     "Overflow de fixed-point ... satura/clampa, provado por teste" -- `tests/test_raster.c`'s
//     saturation case is the executed proof).
//
//     CLIPPING (hostile-input posture -- points/contours come from an UNTRUSTED font, per this
//     ticket's brief): any device coordinate landing outside `[0, w) x [0, h)` is CLIPPED at the
//     analytic-coverage level (`src/raster.c`'s `accum_row_segment`) -- NEVER an out-of-bounds
//     write to `bitmap_out`, `scratch`, or any other memory, for ANY input, including
//     `num_points == 0` (an empty glyph, e.g. `space` -- returns `GLX_RASTER_OK` with every cell
//     of `bitmap_out` set to `0`) and a degenerate 0- or 1-point "contour" (`contour_ends[i] ==
//     contour_ends[i-1] + 1` or less -- silently contributes zero area, per this ticket's brief).
//     A horizontal edge (two consecutive points sharing the same `device_y_26_6`, after scaling)
//     contributes NOTHING to winding (skipped before any division by a Y delta could occur) --
//     the specific "divisão por zero no cálculo de área/inclinação" case this ticket's brief
//     names.
//
//     Returns `GLX_RASTER_ERR_INVALID_ARG` for: `points == NULL` with `num_points > 0`,
//     `contour_ends == NULL` with `num_contours > 0`, `bitmap_out == NULL`, `scratch == NULL`
//     with a non-zero `scratch_capacity_floats` requirement, `scale_den <= 0`, `w <= 0`, `h <= 0`,
//     or `stride < w`. Returns `GLX_RASTER_ERR_SCRATCH_TOO_SMALL` if `scratch_capacity_floats <
//     glx_raster_scratch_floats(w, h)` (see that function's own doc-comment for the `0`-on-
//     overflow case, which this function also surfaces as `GLX_RASTER_ERR_INVALID_ARG` instead,
//     ahead of the scratch-size check, since an unrepresentable `(w, h)` is an argument problem,
//     not a "your buffer was merely too small" one).
// PT: Achata+rasteriza `points[0..num_points)` (particionado em `num_contours` contornos fechados
//     via `contour_ends`, EXATAMENTE a convenção do `glx_sfnt_glyph_outline`: o contorno `i` cobre
//     `points[contour_ends[i-1]+1 .. contour_ends[i]]`, `contour_ends[-1]` implicitamente `-1` pra
//     `i == 0`) nas células `[0, w) x [0, h)` de `bitmap_out` (row-major, `stride` bytes por linha
//     -- `bitmap_out[row*stride + col]` é cobertura `0..255`), usando a regra de winding NÃO-ZERO
//     (um buraco -- um segundo contorno enrolado na direção oposta, ex.: o meio do 'O' --
//     renderiza corretamente como descoberto; ver o cabeçalho de arquivo do `src/raster.c` pro
//     algoritmo de cobertura analítica). TODA célula em `[0, w) x [0, h)` é escrita em
//     `GLX_RASTER_OK` (sobrescrita total, sem dependência do conteúdo prévio de `bitmap_out`).
//
//     MAPEAMENTO DE COORDENADA (ponto em unidade-de-fonte -> pixel de dispositivo, ver a própria
//     nota "FIXED-POINT 26.6 vs FLOAT" deste header pra divisão de precisão em dois estágios):
//     pra um ponto em unidade-de-fonte `(fx, fy)`, `device_x_26_6 = origin_x_26_6 +
//     round(fx * scale_num / scale_den)` e `device_y_26_6 = origin_y_26_6 -
//     round(fy * scale_num / scale_den)` -- note o MENOS no termo `y`: outlines TrueType/SFNT são
//     Y-PRA-CIMA (font_y cresce pra cima, a própria convenção do `glx_sfnt_point`, herdada do
//     `include/core/sfnt.h`), todo bitmap de pixel é Y-PRA-BAIXO (linha 0 é a linha do TOPO) --
//     esse flip é uma parte fixa, não-opcional, da conversão de formato que esta função executa,
//     não uma política configurável por quem chama. `(scale_num, scale_den)` é um fator de escala
//     racional genérico (`scale_den > 0` exigido) -- um chamador SFNT passa `(px_size_26_6,
//     face->units_per_em)` (`px_size_26_6` = tamanho de em desejado em pixels 26.6, ex.: `16.0px`
//     -> `1024`); um chamador rasterizando um contorno construído à mão, já em escala-de-
//     dispositivo (ex.: os próprios testes-oráculo desta tarefa, `tests/test_raster.c`) passa
//     `(64, 1)` pra um mapeamento 1:1 unidade-de-fonte-igual-a-pixel (`64` porque os campos
//     `x`/`y` do `glx_sfnt_point` são unidades de fonte `int16_t`, e a própria unidade interna de
//     fixed-point desta função é 1/64 px -- `scale_num=64, scale_den=1` faz 1 unidade-de-fonte
//     igualar exatamente 1 pixel de dispositivo). `(origin_x_26_6, origin_y_26_6)` é um offset de
//     espaço-de-dispositivo 26.6 escolhido por quem chama, somado DEPOIS da escala -- permite quem
//     chama posicionar a própria bbox de um glyph (`glx_sfnt_outline::x_min`/`y_max`) em qualquer
//     offset de pixel dentro de `bitmap_out` (tipicamente `origin_x_26_6 = -scale(x_min)`,
//     `origin_y_26_6 = scale(y_max)`, alinhando o glyph à esquerda/topo dentro do bitmap, mais
//     qualquer margem que quem chama quiser).
//
//     OVERFLOW: toda coordenada de dispositivo escalada+deslocada é SATURADA em
//     `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]` (`src/raster.c`) antes de qualquer aritmética
//     posterior -- uma combinação `(scale_num, scale_den, origin)` hostil/degenerada que de outro
//     jeito estouraria a aritmética `int32_t` 26.6 satura em vez de dar a volta/UB (ver o brief
//     desta tarefa, "Overflow de fixed-point ... satura/clampa, provado por teste" -- o caso de
//     saturação do `tests/test_raster.c` é a prova executada).
//
//     CLIPPING (postura de input hostil -- pontos/contornos vêm de uma fonte NÃO-CONFIÁVEL,
//     conforme o brief desta tarefa): qualquer coordenada de dispositivo caindo fora de
//     `[0, w) x [0, h)` é CLIPADA no nível de cobertura analítica (`accum_row_segment` do
//     `src/raster.c`) -- NUNCA uma escrita fora de limite em `bitmap_out`, `scratch`, ou qualquer
//     outra memória, pra QUALQUER input, inclusive `num_points == 0` (um glyph vazio, ex.:
//     `space` -- retorna `GLX_RASTER_OK` com toda célula de `bitmap_out` em `0`) e um "contorno"
//     degenerado de 0 ou 1 ponto (`contour_ends[i] == contour_ends[i-1] + 1` ou menos --
//     contribui silenciosamente área zero, conforme o brief desta tarefa). Uma aresta horizontal
//     (dois pontos consecutivos compartilhando o mesmo `device_y_26_6`, depois da escala) não
//     contribui NADA pro winding (pulada antes de qualquer divisão por um delta de Y poder
//     ocorrer) -- o caso específico de "divisão por zero no cálculo de área/inclinação" que o
//     brief desta tarefa nomeia.
//
//     Retorna `GLX_RASTER_ERR_INVALID_ARG` pra: `points == NULL` com `num_points > 0`,
//     `contour_ends == NULL` com `num_contours > 0`, `bitmap_out == NULL`, `scratch == NULL` com
//     uma exigência não-zero de `scratch_capacity_floats`, `scale_den <= 0`, `w <= 0`, `h <= 0`,
//     ou `stride < w`. Retorna `GLX_RASTER_ERR_SCRATCH_TOO_SMALL` se `scratch_capacity_floats <
//     glx_raster_scratch_floats(w, h)` (ver o doc-comment próprio daquela função pro caso `0`-em-
//     overflow, que esta função também expõe como `GLX_RASTER_ERR_INVALID_ARG` em vez disso, antes
//     da checagem de tamanho-de-scratch, já que um `(w, h)` não-representável é um problema de
//     argumento, não um "seu buffer só era pequeno demais").
glx_raster_result glx_rasterize_outline(const glx_sfnt_point* points, uint16_t num_points,
                                         const uint16_t* contour_ends, uint16_t num_contours,
                                         int32_t scale_num, int32_t scale_den,
                                         int32_t origin_x_26_6, int32_t origin_y_26_6,
                                         uint8_t* bitmap_out, int w, int h, int stride,
                                         float* scratch, size_t scratch_capacity_floats);

#ifdef __cplusplus
}
#endif
