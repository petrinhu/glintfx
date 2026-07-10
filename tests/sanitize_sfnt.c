// SPDX-License-Identifier: MPL-2.0
// EN: SOV-SFNT (FT-F1) SANITIZER GATE -- review-adversarial DEFENSE-IN-DEPTH addition (SOV-SFNT
//     review): the CRITICAL float->int16 UB in `apply_component_transform` (src/sfnt.c,
//     composite glyph transform) was invisible to `make test`'s freestanding harness (`tests/
//     test_sfnt.c`, `include/test.h`'s `TEST_ASSERT_EQ`) -- that harness runs the REAL
//     freestanding binary, with no sanitizer instrumentation, and a `(int16_t)` cast on an
//     out-of-range `float` truncates SILENTLY (no crash, no signal) on every mainstream x86-64
//     target; the bug only surfaces as garbage data, which a plain exit-code/value assertion
//     without `-fsanitize=undefined` cannot distinguish from a coincidentally-passing test. This
//     file exists to close that gap: it is a plain HOSTED (NOT freestanding) C program -- links
//     the ordinary libc (`fopen`/`malloc`/`fprintf`), deliberately OUTSIDE Layer 0's
//     `-ffreestanding -nostdlib` build (mirrors `tests/libcore_consumer.cpp`'s own precedent: a
//     hosted gate program that is not part of `$(PROGRAMS)`/`make test`, built and run by its
//     own dedicated Makefile target) -- so it, and ONLY it, can be compiled with
//     `-fsanitize=address,undefined` and actually catch this class of bug: ASan for any stray
//     out-of-bounds read a hostile/malformed font might still trigger, UBSan for EXACTLY the
//     float-cast-out-of-range class that slipped through before (and signed overflow, shift
//     UB, etc., for free). `src/sfnt.c` itself is UNCHANGED here -- no `#ifdef`, no
//     sanitizer-only code path -- it compiles hosted with zero modification because it only ever
//     `#include`s `<stdint.h>`/`<stddef.h>` and calls no freestanding-internal symbol (see
//     include/core/sfnt.h's file header, "NO DYNAMIC ALLOCATION" / "WHY <stdint.h>/<stddef.h>,
//     NOT include/types.h") -- the exact same source file `make build`/`make test` compile
//     freestanding is what gets sanitized here, not a fork or a copy.
//
//     COVERAGE: (1) the REAL Open Sans fixture (`tests/fixtures/opensans_regular.ttf`, read via
//     `fopen` -- this hosted program is allowed to touch the filesystem, unlike Layer 0's own
//     code) through `glx_sfnt_open`/`glx_sfnt_glyph_id`/`glx_sfnt_hmetrics`/
//     `glx_sfnt_glyph_outline`, INCLUDING the composite glyph 'á' (gid 163, the pt-br-critical
//     path this whole ticket exists for) and the `GLX_SFNT_ERR_BUFFER_TOO_SMALL`/
//     `GLX_SFNT_ERR_INVALID_ARG` edges; (2) `kSyntheticFontCompositeSaturate`, BYTE-FOR-BYTE THE
//     SAME fixture as `tests/test_sfnt.c`'s (see that file's own comment for the full
//     byte-layout derivation and clean-room provenance -- duplicated here, not shared, so this
//     sanitizer gate stays a fully independent, standalone hosted program with no dependency on
//     the freestanding harness) -- the reviewer's EXACT repro (`dx=20000` composited onto a
//     `x=20000` point, `fx=40000.5`, previously UB, now saturates to `INT16_MAX`); (3)
//     `kSyntheticFontSelfRefComposite` (self-referencing composite, proves the recursion-depth
//     cap terminates cleanly under ASan/UBSan too, not just the freestanding harness); (4)
//     `kSyntheticFontLocaDescending` (the descending-`loca` hostile case, `GLX_SFNT_ERR_BAD_
//     OFFSET`). A CLEAN run (every check below passes AND the process exits 0 with the
//     sanitizers attached) is the actual proof this ticket's CRITICAL fix eliminated the UB --
//     not just "the assertions still pass" (they did even with the bug, see above), but "ASan/
//     UBSan, watching every read/cast, found nothing to say."
//
//     FUTURE REUSE: this Makefile target (`make sanitize-sfnt`) is deliberately NAMED for the
//     `src/sfnt.c` translation unit specifically, not the whole `src/` tree -- SOV-RAST (the next
//     piece, the un-hinted quadratic-curve rasterizer that will consume this parser's point
//     stream) is expected to grow its own `sanitize-rast` sibling target the same shape, per this
//     ticket's brief.
// PT: PORTÃO SANITIZER do SOV-SFNT (FT-F1) -- acréscimo de DEFESA-EM-PROFUNDIDADE do review
//     adversarial (review do SOV-SFNT): o UB CRÍTICO de float->int16 no
//     `apply_component_transform` (src/sfnt.c, transformação de glyph composto) era invisível pro
//     harness freestanding do `make test` (`tests/test_sfnt.c`, `TEST_ASSERT_EQ` do
//     `include/test.h`) -- aquele harness roda o binário freestanding DE VERDADE, sem
//     instrumentação de sanitizer, e um cast `(int16_t)` num `float` fora de faixa trunca EM
//     SILÊNCIO (sem crash, sem sinal) em todo alvo x86-64 mainstream; o bug só aparece como dado
//     lixo, que uma asserção simples de exit-code/valor sem `-fsanitize=undefined` não consegue
//     distinguir de um teste passando por coincidência. Este arquivo existe pra fechar essa
//     lacuna: é um programa C HOSTED comum (NÃO freestanding) -- linka a libc normal
//     (`fopen`/`malloc`/`fprintf`), deliberadamente FORA do build `-ffreestanding -nostdlib` da
//     Camada 0 (espelha o próprio precedente do `tests/libcore_consumer.cpp`: um programa-gate
//     hosted que não é parte de `$(PROGRAMS)`/`make test`, buildado e rodado pelo próprio alvo
//     dedicado de Makefile) -- então ele, e SÓ ele, pode ser compilado com
//     `-fsanitize=address,undefined` e de fato pegar esta classe de bug: ASan pra qualquer
//     leitura fora de limite perdida que uma fonte hostil/malformada ainda dispare, UBSan pra
//     EXATAMENTE a classe cast-de-float-fora-de-faixa que escapou antes (e overflow com sinal,
//     UB de shift, etc., de graça). O próprio `src/sfnt.c` fica INALTERADO aqui -- sem `#ifdef`,
//     sem caminho de código só-de-sanitizer -- ele compila hosted com zero modificação porque só
//     `#include`ia `<stdint.h>`/`<stddef.h>` e não chama símbolo interno-freestanding nenhum (ver
//     o cabeçalho de arquivo do include/core/sfnt.h, "SEM ALOCAÇÃO DINÂMICA" / "POR QUE
//     <stdint.h>/<stddef.h>, NÃO include/types.h") -- o MESMO arquivo-fonte exato que o `make
//     build`/`make test` compilam freestanding é o que é sanitizado aqui, não um fork ou uma
//     cópia.
//
//     COBERTURA: (1) a fixture Open Sans REAL (`tests/fixtures/opensans_regular.ttf`, lida via
//     `fopen` -- este programa hosted tem permissão de tocar o sistema de arquivos, diferente do
//     próprio código da Camada 0) através de
//     `glx_sfnt_open`/`glx_sfnt_glyph_id`/`glx_sfnt_hmetrics`/`glx_sfnt_glyph_outline`, INCLUINDO
//     o glyph composto 'á' (gid 163, o caminho crítico-pt-br pelo qual esta tarefa inteira
//     existe) e as bordas `GLX_SFNT_ERR_BUFFER_TOO_SMALL`/`GLX_SFNT_ERR_INVALID_ARG`; (2)
//     `kSyntheticFontCompositeSaturate`, BYTE-A-BYTE A MESMA fixture do `tests/test_sfnt.c` (ver
//     o comentário próprio daquele arquivo pra derivação completa de layout-de-byte e proveniência
//     clean-room -- duplicada aqui, não compartilhada, pra este portão sanitizer continuar sendo
//     um programa hosted independente e autônomo, sem dependência do harness freestanding) -- o
//     repro EXATO do revisor (`dx=20000` composto sobre um ponto `x=20000`, `fx=40000.5`,
//     antes UB, agora satura pro `INT16_MAX`); (3) `kSyntheticFontSelfRefComposite` (composto
//     auto-referente, prova que o teto de profundidade de recursão termina de forma limpa sob
//     ASan/UBSan também, não só o harness freestanding); (4) `kSyntheticFontLocaDescending` (o
//     caso hostil de `loca` descendente, `GLX_SFNT_ERR_BAD_OFFSET`). Uma execução LIMPA (toda
//     checagem abaixo passa E o processo sai com 0 com os sanitizers presos) é a prova de fato de
//     que o fix CRÍTICO desta tarefa eliminou o UB -- não só "as asserções ainda passam" (passavam
//     mesmo com o bug, ver acima), mas "ASan/UBSan, vigiando toda leitura/cast, não acharam nada
//     pra dizer."
//
//     REUSO FUTURO: este alvo de Makefile (`make sanitize-sfnt`) é deliberadamente NOMEADO pra
//     unidade de tradução `src/sfnt.c` especificamente, não a árvore `src/` inteira -- o SOV-RAST
//     (a próxima peça, o rasterizador de curva quadrática sem hinting que vai consumir o fluxo de
//     ponto deste parser) tem expectativa de crescer seu próprio alvo irmão `sanitize-rast` na
//     mesma forma, conforme o brief desta tarefa.
// Copyright (c) 2026 Petrus Silva Costa
#include "core/sfnt.h"

#include <stdio.h>
#include <stdlib.h>

// EN: Simple PASS/FAIL bookkeeping -- deliberately NOT `include/test.h` (that harness is
//     freestanding, `sys_exit`-based, and this program is hosted -- reusing it would defeat the
//     point of being a plain hosted program a sanitizer can attach to normally).
// PT: Contabilidade PASS/FAIL simples -- deliberadamente NÃO o `include/test.h` (aquele harness é
//     freestanding, baseado em `sys_exit`, e este programa é hosted -- reusá-lo derrotaria o
//     propósito de ser um programa hosted comum que um sanitizer consegue prender normalmente).
static int g_failures = 0;

#define CHECK(cond, msg)                                                                        \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));                     \
            g_failures++;                                                                        \
        }                                                                                         \
    } while (0)

// ================================================================================================
// EN: `kSyntheticFontCompositeSaturate` -- BYTE-FOR-BYTE identical to `tests/test_sfnt.c`'s copy;
//     see that file's comment immediately above its own declaration for the full derivation (2
//     glyphs: gid 0 simple, one on-curve point at (20000, 0); gid 1 composite, translates gid 0
//     by dx=20000, dy=0 -- `fx = 40000.0f`, the reviewer's exact UB repro).
// PT: `kSyntheticFontCompositeSaturate` -- idêntica byte-a-byte à cópia do `tests/test_sfnt.c`;
//     ver o comentário daquele arquivo logo acima da própria declaração pra derivação completa (2
//     glyphs: gid 0 simples, um ponto on-curve em (20000, 0); gid 1 composto, translada o gid 0
//     por dx=20000, dy=0 -- `fx = 40000.0f`, o repro exato de UB do revisor).
// ================================================================================================
static const unsigned char kSyntheticFontCompositeSaturate[300] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x24, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x4e, 0x20, 0x00, 0x00, 0x4e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x4e,
    0x20, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x4e, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x64,
    0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x12, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// EN: `kSyntheticFontSelfRefComposite` -- byte-for-byte identical to `tests/test_sfnt.c`'s copy
//     (a single composite glyph, gid 0, referencing ITSELF, no `MORE_COMPONENTS`). See that
//     file's comment above its own declaration for the full derivation.
// PT: `kSyntheticFontSelfRefComposite` -- idêntica byte-a-byte à cópia do `tests/test_sfnt.c` (um
//     único glyph composto, gid 0, referenciando A SI MESMO, sem `MORE_COMPONENTS`). Ver o
//     comentário daquele arquivo acima da própria declaração pra derivação completa.
static const unsigned char kSyntheticFontSelfRefComposite[276] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x10, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x04, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x04, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5,
    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// EN: `kSyntheticFontLocaDescending` -- byte-for-byte identical to `tests/test_sfnt.c`'s copy
//     (a patched variant of the cmap-format-12 fixture where `loca[1] > loca[2]`, the descending-
//     `loca` hostile case). See that file's comment above its own declaration for the full
//     derivation.
// PT: `kSyntheticFontLocaDescending` -- idêntica byte-a-byte à cópia do `tests/test_sfnt.c` (uma
//     variante remendada da fixture cmap-formato-12 onde `loca[1] > loca[2]`, o caso hostil de
//     `loca` descendente). Ver o comentário daquele arquivo acima da própria declaração pra
//     derivação completa.
static const unsigned char kSyntheticFontLocaDescending[324] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x28, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x14, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x24, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xf6, 0x00, 0x00, 0x01, 0xf6, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x02,
    0x00, 0x00, 0x31, 0x33, 0x27, 0x64, 0x32, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x03, 0x20, 0xff, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// EN: Reads `path` fully into a heap buffer (ordinary hosted `fopen`/`fread`/`malloc` -- this
//     program is explicitly outside Layer 0's zero-libc rule, see this file's header). Returns
//     NULL and leaves `*len_out` untouched on any failure.
// PT: Lê `path` inteiro pra um buffer de heap (`fopen`/`fread`/`malloc` hosted comuns -- este
//     programa está explicitamente fora da regra zero-libc da Camada 0, ver o cabeçalho deste
//     arquivo). Retorna NULL e deixa `*len_out` intocado em qualquer falha.
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
    // ---- (1) real Open Sans, including the pt-br-critical composite 'á' -----------------------
    // ============================================================================================
    {
        size_t len = 0;
        unsigned char* blob = read_whole_file("tests/fixtures/opensans_regular.ttf", &len);
        CHECK(blob != NULL, "could not read tests/fixtures/opensans_regular.ttf (run from repo root)");
        if (blob != NULL) {
            glx_sfnt_face face;
            glx_sfnt_result r = glx_sfnt_open(blob, len, &face);
            CHECK(r == GLX_SFNT_OK, "glx_sfnt_open(real Open Sans) != OK");
            if (r == GLX_SFNT_OK) {
                CHECK(glx_sfnt_glyph_id(&face, 'A') == 36u, "glyph_id('A') mismatch");
                CHECK(glx_sfnt_glyph_id(&face, 0x00E1u) == 163u, "glyph_id('a') mismatch");
                CHECK(glx_sfnt_glyph_id(&face, 0x10FFFFu) == 0u, "glyph_id(unmapped) != .notdef");

                uint16_t adv;
                int16_t lsb;
                CHECK(glx_sfnt_hmetrics(&face, 36, &adv, &lsb) == GLX_SFNT_OK, "hmetrics(A) != OK");
                CHECK(glx_sfnt_hmetrics(&face, 999999, &adv, &lsb) == GLX_SFNT_ERR_INVALID_ARG,
                      "hmetrics(oob gid) != INVALID_ARG");

                glx_sfnt_point points[64];
                uint16_t contour_ends[8];
                glx_sfnt_outline out;

                // 'H' -- simple glyph.
                r = glx_sfnt_glyph_outline(&face, 43, points, 64, contour_ends, 8, &out);
                CHECK(r == GLX_SFNT_OK && out.num_points == 12, "outline('H') mismatch");

                // '.notdef' -- simple glyph, 2 contours.
                r = glx_sfnt_glyph_outline(&face, 0, points, 64, contour_ends, 8, &out);
                CHECK(r == GLX_SFNT_OK && out.num_contours == 2, "outline(.notdef) mismatch");

                // 'á' -- COMPOSITE glyph, 2 components, plain translate -- the exact code path
                // `apply_component_transform` lives on, exercised here under ASan/UBSan.
                r = glx_sfnt_glyph_outline(&face, 163, points, 64, contour_ends, 8, &out);
                CHECK(r == GLX_SFNT_OK && out.num_points == 47, "outline('á', composite) mismatch");

                // BUFFER_TOO_SMALL: 'H' needs 12 points, a 4-slot buffer must fail cleanly.
                glx_sfnt_point small_points[4];
                uint16_t small_ends[4];
                r = glx_sfnt_glyph_outline(&face, 43, small_points, 4, small_ends, 4, &out);
                CHECK(r == GLX_SFNT_ERR_BUFFER_TOO_SMALL, "outline('H', tiny buf) != BUFFER_TOO_SMALL");

                // INVALID_ARG: gid out of range.
                r = glx_sfnt_glyph_outline(&face, 999999, points, 64, contour_ends, 8, &out);
                CHECK(r == GLX_SFNT_ERR_INVALID_ARG, "outline(oob gid) != INVALID_ARG");
            }
            free(blob);
        }
    }

    // ============================================================================================
    // ---- (2) THE regression: composite transform saturates, does not UB -----------------------
    // ============================================================================================
    {
        glx_sfnt_face face;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontCompositeSaturate,
                                           sizeof(kSyntheticFontCompositeSaturate), &face);
        CHECK(r == GLX_SFNT_OK, "glx_sfnt_open(CompositeSaturate) != OK");
        if (r == GLX_SFNT_OK) {
            glx_sfnt_point points[4];
            uint16_t contour_ends[2];
            glx_sfnt_outline out;

            r = glx_sfnt_glyph_outline(&face, 0, points, 4, contour_ends, 2, &out);
            CHECK(r == GLX_SFNT_OK && points[0].x == 20000 && points[0].y == 0,
                  "outline(gid0, base point) mismatch");

            // EN: THIS is the call that was UB before the fix -- `fx = 40000.0f`,
            //     `(int16_t)(fx + 0.5f)` with no range check. Under
            //     `-fsanitize=undefined`, that line would abort the process with a
            //     "runtime error: ... outside range of int16_t" diagnostic instead of
            //     silently returning `-25536` -- a CLEAN pass here (right value, sanitizer
            //     silent) is the actual proof the UB is gone.
            // PT: ESTA é a chamada que era UB antes do fix -- `fx = 40000.0f`,
            //     `(int16_t)(fx + 0.5f)` sem checagem de faixa. Sob `-fsanitize=undefined`,
            //     essa linha abortaria o processo com um diagnóstico "runtime error: ...
            //     outside range of int16_t" em vez de retornar `-25536` silenciosamente --
            //     um passe LIMPO aqui (valor certo, sanitizer calado) é a prova de fato de
            //     que o UB sumiu.
            r = glx_sfnt_glyph_outline(&face, 1, points, 4, contour_ends, 2, &out);
            CHECK(r == GLX_SFNT_OK, "outline(gid1, composite saturate) != OK");
            CHECK(points[0].x == INT16_MAX, "outline(gid1) x not saturated to INT16_MAX");
            CHECK(points[0].y == 0, "outline(gid1) y (unaffected axis) not 0");
        }
    }

    // ============================================================================================
    // ---- (3) self-referencing composite -- terminates cleanly under ASan/UBSan too ------------
    // ============================================================================================
    {
        glx_sfnt_face face;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontSelfRefComposite,
                                           sizeof(kSyntheticFontSelfRefComposite), &face);
        CHECK(r == GLX_SFNT_OK, "glx_sfnt_open(SelfRefComposite) != OK");
        if (r == GLX_SFNT_OK) {
            glx_sfnt_point points[8];
            uint16_t contour_ends[4];
            glx_sfnt_outline out;
            r = glx_sfnt_glyph_outline(&face, 0, points, 8, contour_ends, 4, &out);
            CHECK(r == GLX_SFNT_ERR_TOO_DEEP, "outline(self-ref composite) != TOO_DEEP");
        }
    }

    // ============================================================================================
    // ---- (4) descending loca -- hostile offset relationship caught cleanly --------------------
    // ============================================================================================
    {
        glx_sfnt_face face;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontLocaDescending,
                                           sizeof(kSyntheticFontLocaDescending), &face);
        CHECK(r == GLX_SFNT_OK, "glx_sfnt_open(LocaDescending) != OK");
        if (r == GLX_SFNT_OK) {
            glx_sfnt_point points[8];
            uint16_t contour_ends[4];
            glx_sfnt_outline out;
            r = glx_sfnt_glyph_outline(&face, 1, points, 8, contour_ends, 4, &out);
            CHECK(r == GLX_SFNT_ERR_BAD_OFFSET, "outline(descending loca) != BAD_OFFSET");
        }
    }

    if (g_failures == 0) {
        fprintf(stderr, "sanitize_sfnt: PASS (0 assertion failures, ASan/UBSan silent)\n");
        return 0;
    }
    fprintf(stderr, "sanitize_sfnt: FAIL (%d assertion failure(s))\n", g_failures);
    return 1;
}
