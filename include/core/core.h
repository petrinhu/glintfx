// SPDX-License-Identifier: MPL-2.0
// EN: SOV-LIBCORE (ADR-0009, FT-F0) -- the PUBLIC consumption surface of `build/libcore.a`, the
//     physical boundary the internalization ADR calls for: a static archive whose EXPORTED
//     symbols carry the `glx_` namespace prefix, safe to `#include`/link from ordinary HOSTED
//     C++ (a future glintfx build, today `tests/libcore_consumer.cpp`) sitting right next to a
//     normal libc/libstdc++. This header is the ONLY thing a hosted consumer needs to `#include`
//     -- it never pulls in this project's own freestanding `include/types.h`/`include/alloc.h`/
//     `include/mem.h` (those stay Layer-0-internal), only the standard, freestanding-SAFE
//     `<stddef.h>` (guaranteed to exist even under `-ffreestanding` -- same reasoning
//     `include/printf.h` already documents for `<stdarg.h>`: compiler-provided, not
//     libc-linked, so including it violates neither Layer 0's "zero libc" rule NOR a hosted
//     TU's normal `<stddef.h>`).
//
//     ALIAS-VS-RENAME DECISION (this ticket's brief left it to the implementer -- "menor churn
//     que satisfaça o ADR"): chose **alias via thin wrapper, backed by a build-time RENAME of
//     the archived-only copy of the colliding internal symbols** -- see src/core_api.c's file
//     header for the full mechanism. `include/alloc.h`'s `malloc`/`free`/`realloc` (ADR-0004,
//     one-way-door) and `include/mem.h`'s `memcpy`/`memset` are UNTOUCHED -- same names, same
//     signatures, same freestanding gate-program tests as before (`make test` unaffected). What
//     is NEW is a second, ARCHIVE-ONLY compilation of `src/alloc.c`/`src/mem.c` (via the
//     Makefile's `libcore` target, object dir `build/objcore/`) with those five identifiers
//     `#define`-renamed to non-colliding `glx_core_*_impl` names BEFORE the compiler ever sees
//     them -- so the .o files that actually land in `build/libcore.a` never define a bare
//     `malloc`/`free`/`realloc`/`memcpy`/`memset` symbol at all. `glx_malloc()` etc. below (thin
//     wrappers in src/core_api.c, compiled the SAME way) call straight through to those renamed
//     internals. Net effect: zero signature/behaviour change to the ADR-0004-frozen contract,
//     zero symbol collision risk for a hosted binary that also links glibc's own
//     malloc/free/realloc/memcpy/memset (see ADR-0009's own words: "symbol collision -- the
//     `glx_` prefix handles that trivially").
// PT: SOV-LIBCORE (ADR-0009, FT-F0) -- a superfície PÚBLICA de consumo da `build/libcore.a`, a
//     fronteira física que o ADR de internalização pede: um archive estático cujos símbolos
//     EXPORTADOS carregam o prefixo de namespace `glx_`, seguro de `#include`/linkar a partir de
//     C++ HOSTED comum (um futuro build do glintfx, hoje `tests/libcore_consumer.cpp`) sentado
//     bem ao lado de uma libc/libstdc++ normal. Este header é a ÚNICA coisa que um consumidor
//     hosted precisa `#include`ar -- ele nunca puxa o `include/types.h`/`include/alloc.h`/
//     `include/mem.h` freestanding PRÓPRIOS deste projeto (esses ficam internos à Camada 0), só
//     o `<stddef.h>` padrão, SEGURO em contexto freestanding (garantido existir mesmo sob
//     `-ffreestanding` -- mesmo raciocínio que o `include/printf.h` já documenta pro
//     `<stdarg.h>`: fornecido pelo compilador, não linkado da libc, então incluí-lo não viola
//     nem a regra "zero libc" da Camada 0 NEM o `<stddef.h>` normal de uma TU hosted).
//
//     DECISÃO ALIAS-VS-RENAME (o brief desta tarefa deixou pro implementador -- "menor churn que
//     satisfaça o ADR"): escolhido **alias via wrapper fino, lastreado por um RENAME em tempo de
//     build da cópia archive-only dos símbolos internos colidentes** -- ver o cabeçalho de
//     arquivo do src/core_api.c pro mecanismo completo. O `malloc`/`free`/`realloc` do
//     `include/alloc.h` (ADR-0004, one-way-door) e o `memcpy`/`memset` do `include/mem.h` ficam
//     INTOCADOS -- mesmos nomes, mesmas assinaturas, mesmos testes de programa-gate freestanding
//     de antes (`make test` não afetado). O que é NOVO é uma segunda compilação, SÓ PRO ARCHIVE,
//     de `src/alloc.c`/`src/mem.c` (via o alvo `libcore` do Makefile, diretório de objeto
//     `build/objcore/`) com esses cinco identificadores renomeados via `#define` pra nomes
//     não-colidentes `glx_core_*_impl` ANTES do compilador sequer vê-los -- então os `.o` que de
//     fato entram na `build/libcore.a` nunca definem um símbolo `malloc`/`free`/`realloc`/
//     `memcpy`/`memset` cru. O `glx_malloc()` etc. abaixo (wrappers finos em src/core_api.c,
//     compilados do MESMO jeito) chamam direto pra esses internos renomeados. Efeito líquido:
//     zero mudança de assinatura/comportamento no contrato congelado pela ADR-0004, zero risco
//     de colisão de símbolo pra um binário hosted que também linka o
//     malloc/free/realloc/memcpy/memset PRÓPRIOS da glibc (nas palavras da própria ADR-0009:
//     "colisão de símbolo -- o prefixo `glx_` resolve isso trivialmente").
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// EN: `glx_malloc`/`glx_free`/`glx_realloc` -- the ADR-0009 gate-3 essential trio ("o alocador
//     ... é o essencial pro épico FreeType"). Contract is IDENTICAL to include/alloc.h's
//     `malloc`/`free`/`realloc` in every respect (NULL-on-failure, 16-byte alignment,
//     malloc(0)==NULL, double-free is UB with best-effort detection, realloc semantics) -- see
//     that header for the full prose; not re-derived here to avoid two documents drifting apart.
//     ONE ADDITIONAL RULE that did NOT exist on the freestanding-only `free()`:
//     **`glx_free(ptr)` must NEVER be called with a pointer this allocator did not itself hand
//     out** (e.g. a pointer from glibc's own `malloc()`/`new`) -- this is gate 3 of ADR-0009 ("a
//     pointer allocated by `glx_*` must never be passed to glibc's `free()`, and vice-versa").
//     `glx_free` DETECTS this specific misuse as defense-in-depth (src/core_api.c's
//     `alloc_owns_ptr` boundary check, src/alloc_internal.h) and terminates the process via
//     `sys_exit` rather than silently corrupting either heap -- see src/core_api.c's file header
//     for exactly what this check does and does not guarantee, and
//     tests/libcore_consumer.cpp for a live, executed proof (a forked child process that
//     deliberately misuses it and is asserted to die with the documented exit code).
// PT: `glx_malloc`/`glx_free`/`glx_realloc` -- o trio essencial do gate 3 da ADR-0009 ("o
//     alocador ... é o essencial pro épico FreeType"). Contrato é IDÊNTICO ao
//     `malloc`/`free`/`realloc` do include/alloc.h em todo aspecto (NULL em falha, alinhamento a
//     16 bytes, malloc(0)==NULL, double-free é UB com detecção de melhor esforço, semântica do
//     realloc) -- ver aquele header pra prosa completa; não re-derivado aqui pra não deixar dois
//     documentos driftarem um do outro. UMA REGRA ADICIONAL que NÃO existia no `free()`
//     freestanding-só: **`glx_free(ptr)` NUNCA pode ser chamado com um ponteiro que este
//     alocador não entregou ele mesmo** (ex.: um ponteiro do `malloc()`/`new` da própria glibc)
//     -- isto é o gate 3 da ADR-0009 ("um ponteiro alocado por `glx_*` nunca pode ser passado
//     pro `free()` da glibc, nem vice-versa"). O `glx_free` DETECTA esse mau-uso específico como
//     defesa em profundidade (checagem de fronteira `alloc_owns_ptr` do src/core_api.c,
//     src/alloc_internal.h) e termina o processo via `sys_exit` em vez de corromper qualquer um
//     dos dois heaps em silêncio -- ver o cabeçalho de arquivo do src/core_api.c pro que essa
//     checagem garante e não garante exatamente, e tests/libcore_consumer.cpp pra uma prova
//     viva, executada (um processo filho via fork que mau-usa de propósito e é verificado morrer
//     com o exit code documentado).
void* glx_malloc(size_t size);
void  glx_free(void* ptr);
void* glx_realloc(void* ptr, size_t size);

// EN: `glx_memcpy`/`glx_memset` -- exposed because they are the other primitive the FreeType
//     epic's next phases (SOV-SFNT's blob parsing, SOV-RAST's bitmap writes) will need most,
//     per this ticket's brief ("glx_memcpy/glx_memset se úteis"). Contract IDENTICAL to
//     include/mem.h's `memcpy`/`memset` -- see that header. `memmove`/`memcmp` are NOT exposed
//     here (YAGNI -- no consumer needs them yet; add when one does, same philosophy this whole
//     codebase already applies to every other primitive).
// PT: `glx_memcpy`/`glx_memset` -- expostos porque são a outra primitiva de que as próximas
//     fases do épico FreeType (parsing de blob do SOV-SFNT, escritas de bitmap do SOV-RAST) mais
//     vão precisar, conforme o brief desta tarefa ("glx_memcpy/glx_memset se úteis"). Contrato
//     IDÊNTICO ao `memcpy`/`memset` do include/mem.h -- ver aquele header. `memmove`/`memcmp`
//     NÃO são expostos aqui (YAGNI -- nenhum consumidor precisa deles ainda; acrescentar quando
//     um precisar, mesma filosofia que este código-base inteiro já aplica a toda outra
//     primitiva).
void* glx_memcpy(void* dst, const void* src, size_t n);
void* glx_memset(void* dst, int c, size_t n);

#ifdef __cplusplus
}
#endif
