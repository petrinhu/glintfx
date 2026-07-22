// SPDX-License-Identifier: MPL-2.0
// EN: QW-IABS32 (INBOX drain, Onda 1, PROG-1) -- hosted-companion PROBE for `iabs32`, the one
//     `static` (internal-linkage) helper in src/raster.c this ticket's brief singles out for a
//     defensive INT32_MIN guard. Unlike glx_raster_scratch_floats/glx_rasterize_outline (the two
//     PUBLIC entry points tests/sanitize_raster.c already exercises as a black box, LINKING
//     src/raster.c as a separate translation unit), `iabs32` has no public name callable from
//     outside its own TU -- the only honest way to exercise it directly, instead of merely
//     ARGUING "unreachable from any real caller today" (see src/raster.c's own doc-comment on the
//     guard), is to become the SAME translation unit: this file #includes src/raster.c
//     TEXTUALLY (not linked, INCLUDED), the ordinary "unity build" idiom for white-box unit
//     testing a C file's own statics, giving this probe direct access to `iabs32` while leaving
//     src/raster.c's own source completely unmodified (no #ifdef, no test-only export, no
//     `static` removed).
//
//     WHY THIS IS A SEPARATE PROGRAM, NOT FOLDED INTO tests/sanitize_raster.c: that file already
//     LINKS src/raster.c as its own object (Makefile's `sanitize-raster` target: `$(CC) ...
//     tests/sanitize_raster.c src/raster.c src/sfnt.c`) -- #including src/raster.c textually a
//     SECOND time into a probe sharing that same binary would redefine every PUBLIC symbol
//     (glx_raster_scratch_floats, glx_rasterize_outline) twice in one link, a hard
//     duplicate-symbol error. This file is therefore its own tiny hosted program (own `main`, own
//     Makefile target `probe-iabs32`, EXCLUDED from $(PROGRAM_SRCS) the same way
//     sanitize_sfnt.c/sanitize_raster.c/sanitize_hint.c already are) -- the ONLY translation unit
//     in the whole build that #includes src/raster.c rather than linking it.
//
//     THE GUARD UNDER TEST: `iabs32(INT32_MIN)` -- negating INT32_MIN (`-(-2147483648)`) is
//     signed-integer-overflow, UB per C 6.5p5 (the mathematical result, +2147483648, does not fit
//     back into int32_t). Built with `-fsanitize=undefined` specifically so that REMOVING the
//     guard (revert the one-line fix in src/raster.c and rebuild this exact probe) makes UBSan
//     abort loudly on this exact call -- proving the guard fixes a REAL UB, not a cosmetic one.
//
//     REACHABILITY, HONESTLY STATED: every call site TODAY (`flatten_quad`'s
//     `iabs32(dx)`/`iabs32(dy)`, src/raster.c's own file header item (1)) feeds `dx = p2x - p0x`/
//     `dy = p2y - p0y` where both operands are already `saturate_fx`-bounded to
//     `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]`, so `dx`/`dy` can never approach INT32_MIN
//     (tests/sanitize_hint.c's own header already recorded this exact domino finding: "confirmed
//     unreachable there since dx/dy are GLX_RASTER_FX_MAX-bounded far from INT32_MIN"). The guard
//     is therefore DEFENSE IN DEPTH for a `static` helper with no unbounded caller today, not a
//     fix for a live bug -- this probe exists so that claim is PROVEN, not merely argued, and so a
//     future caller/refactor that ever feeds an unbounded `int32_t` into `iabs32` inherits a
//     function that is safe by construction, not safe by coincidence of today's callers.
// PT: SONDA hosted-companion do QW-IABS32 (esvaziamento da INBOX, Onda 1, PROG-1) pro `iabs32`, o
//     único helper `static` (linkage interno) do src/raster.c que o brief desta tarefa aponta por
//     nome pra uma guarda defensiva de INT32_MIN. Diferente do
//     glx_raster_scratch_floats/glx_rasterize_outline (os dois pontos de entrada PÚBLICOS que o
//     tests/sanitize_raster.c já exercita como caixa-preta, LINKANDO src/raster.c como uma unidade
//     de tradução separada), o `iabs32` não tem nome público nenhum chamável de fora da própria
//     TU -- o único jeito honesto de exercitá-lo direto, em vez de só ARGUMENTAR "inalcançável por
//     qualquer chamador real hoje" (ver o doc-comment próprio da guarda no src/raster.c), é virar a
//     MESMA unidade de tradução: este arquivo `#include`ia o src/raster.c TEXTUALMENTE (não
//     linkado, INCLUÍDO), o idioma comum "unity build" pra testar caixa-branca os statics de um
//     arquivo C, dando a esta sonda acesso direto ao `iabs32` enquanto deixa o próprio fonte do
//     src/raster.c completamente intocado (sem #ifdef, sem export só-de-teste, nenhum `static`
//     removido).
//
//     POR QUE ISTO É UM PROGRAMA SEPARADO, NÃO DOBRADO NO tests/sanitize_raster.c: aquele arquivo
//     já LINKA o src/raster.c como objeto próprio (alvo `sanitize-raster` do Makefile: `$(CC) ...
//     tests/sanitize_raster.c src/raster.c src/sfnt.c`) -- `#include`ar o src/raster.c
//     textualmente uma SEGUNDA vez numa sonda compartilhando o mesmo binário redefiniria todo
//     símbolo PÚBLICO (glx_raster_scratch_floats, glx_rasterize_outline) duas vezes num só link,
//     um erro rígido de símbolo duplicado. Este arquivo é portanto um programinha hosted próprio
//     (`main` próprio, alvo de Makefile próprio `probe-iabs32`, EXCLUÍDO do $(PROGRAM_SRCS) do
//     mesmo jeito que o sanitize_sfnt.c/sanitize_raster.c/sanitize_hint.c já são) -- a ÚNICA
//     unidade de tradução de todo o build que `#include`ia o src/raster.c em vez de linká-lo.
//
//     A GUARDA SOB TESTE: `iabs32(INT32_MIN)` -- negar INT32_MIN (`-(-2147483648)`) é overflow de
//     inteiro com sinal, UB pelo C 6.5p5 (o resultado matemático, +2147483648, não cabe de volta
//     em int32_t). Buildado com `-fsanitize=undefined` especificamente pra que REMOVER a guarda
//     (reverter o fix de uma linha no src/raster.c e rebuildar esta sonda exata) faça o UBSan
//     abortar alto nesta chamada exata -- provando que a guarda conserta um UB DE VERDADE, não
//     cosmético.
//
//     ALCANÇABILIDADE, DITA COM HONESTIDADE: todo ponto de chamada HOJE (`iabs32(dx)`/
//     `iabs32(dy)` do `flatten_quad`, item (1) do cabeçalho próprio do src/raster.c) alimenta
//     `dx = p2x - p0x`/`dy = p2y - p0y` onde os dois operandos já são limitados por `saturate_fx`
//     a `[-GLX_RASTER_FX_MAX, GLX_RASTER_FX_MAX]`, então `dx`/`dy` nunca conseguem chegar perto de
//     INT32_MIN (o próprio cabeçalho do tests/sanitize_hint.c já registrou esse achado dominó
//     exato: "confirmada inalcançável ali já que dx/dy são limitados por GLX_RASTER_FX_MAX bem
//     longe de INT32_MIN"). A guarda é portanto defesa-em-profundidade pra um helper `static` sem
//     chamador não-limitado hoje, não o conserto de um bug vivo -- esta sonda existe pra que essa
//     alegação seja PROVADA, não só argumentada, e pra que um futuro chamador/refactor que algum
//     dia alimente um `int32_t` não-limitado no `iabs32` herde uma função segura por construção,
//     não por coincidência dos chamadores de hoje.
// Copyright (c) 2026 Petrus Silva Costa
#include <stdint.h>
#include <stdio.h>

// EN: Textual inclusion -- see this file's own header for why (accessing a `static` helper
//     requires becoming the same translation unit). `src/raster.c` compiles UNCHANGED as a plain
//     hosted TU (its own file header, "BUILD-TWICE / HOSTED-COMPANION NOTE" -- it only
//     `#include`s <stdint.h>/<stddef.h>/"core/sfnt.h", no freestanding-internal symbol).
// PT: Inclusão textual -- ver o cabeçalho próprio deste arquivo pro porquê (acessar um helper
//     `static` exige virar a mesma unidade de tradução). O `src/raster.c` compila INALTERADO como
//     uma TU hosted comum (o próprio cabeçalho de arquivo dele, "NOTA DE COMPILAÇÃO-DUPLA /
//     HOSTED-COMPANION" -- só `#include`ia <stdint.h>/<stddef.h>/"core/sfnt.h", nenhum símbolo
//     interno-freestanding).
#include "../src/raster.c"

static int g_failures = 0;

#define CHECK(cond, msg)                                                                       \
    do {                                                                                        \
        if (!(cond)) {                                                                          \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));                    \
            g_failures++;                                                                       \
        }                                                                                        \
    } while (0)

int main(void) {
    // ---- the one case this probe exists for: the guard itself --------------------------------
    CHECK(iabs32(INT32_MIN) == INT32_MAX,
          "iabs32(INT32_MIN) must saturate to INT32_MAX (defense-in-depth guard), never UB-overflow");

    // ---- ordinary cases, unaffected by the guard -- confirms it did not disturb normal use ----
    CHECK(iabs32(0) == 0, "iabs32(0) must be 0");
    CHECK(iabs32(5) == 5, "iabs32(5) must be 5 (already non-negative, no negation taken)");
    CHECK(iabs32(-5) == 5, "iabs32(-5) must be 5 (ordinary negation path)");
    CHECK(iabs32(INT32_MAX) == INT32_MAX, "iabs32(INT32_MAX) must be INT32_MAX (already non-negative)");

    // ---- the real call sites' own bound, round-tripped through the guarded function -----------
    CHECK(iabs32(-GLX_RASTER_FX_MAX) == GLX_RASTER_FX_MAX,
          "iabs32(-GLX_RASTER_FX_MAX) must round-trip through the real call sites' own bound");
    CHECK(iabs32(GLX_RASTER_FX_MAX) == GLX_RASTER_FX_MAX,
          "iabs32(GLX_RASTER_FX_MAX) must round-trip through the real call sites' own bound");

    if (g_failures == 0) {
        fprintf(stderr, "probe_iabs32: PASS (0 assertion failures, UBSan silent)\n");
        return 0;
    }
    fprintf(stderr, "probe_iabs32: FAIL (%d assertion failure(s))\n", g_failures);
    return 1;
}
