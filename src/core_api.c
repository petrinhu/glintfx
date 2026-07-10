// SPDX-License-Identifier: MPL-2.0
// EN: SOV-LIBCORE (ADR-0009, FT-F0) -- the `glx_*` front door declared in include/core/core.h.
//     Every function here is a THIN wrapper (no new logic beyond the ADR-0009 gate-3 boundary
//     check in `glx_free`) around this project's own, already-existing, already-tested
//     freestanding primitives (`malloc`/`free`/`realloc` from src/alloc.c, `memcpy`/`memset`
//     from src/mem.c) -- see include/core/core.h's file header for WHY this file exists at all
//     (the ADR-0009 physical boundary) and the alias-vs-rename decision this ticket's brief left
//     open.
//
//     THE RENAME MECHANISM (this is the load-bearing trick, read carefully): this exact source
//     file is compiled TWICE by the Makefile, with DIFFERENT preprocessor macros each time --
//     - Normal build (`make build`/`make test`, object dir `build/obj/`): compiled with the
//       SAME `$(CFLAGS)` every other src/*.c file gets, NO renaming. `malloc(size)` below
//       literally calls src/alloc.c's own `malloc` (built the normal way too, in the SAME
//       `build/obj/` tree) -- this variant gets linked into every freestanding gate program in
//       tests/*.c via `$(RUNTIME_OBJS)` (the Makefile's `src/*.c` wildcard picks this file up
//       automatically, exactly like every other runtime primitive; see the top-of-Makefile
//       comment on that wildcard design). Harmless there: `glx_malloc` etc. are simply extra,
//       unused-by-any-gate-program functions riding along -- zero new libc dependency (they
//       call OUR OWN malloc/memcpy, already zero-libc), zero behaviour change to any existing
//       test.
//     - Archive build (`make libcore`, object dir `build/objcore/`): compiled with
//       `$(CORE_RENAME_FLAGS)` (Makefile), applied UNIFORMLY to every src/*.c file that lands in
//       `build/objcore/` -- so `src/alloc.c`'s definitions of `malloc`/`free`/`realloc`/
//       `alloc_owns_ptr`, `src/mem.c`'s definitions of `memcpy`/`memset`/`memmove`/`memcmp`,
//       `src/str.c`'s definitions of `strlen`/`strcmp`/`strncmp`/`strcpy`/`strncpy`/`strcat`/
//       `strchr`, `src/conv.c`'s definitions of `atoi`/`atof`/`atou`/`itoa`/`utoa`/`ftoa`, and
//       `src/printf.c`'s definitions of `mini_printf`/`mini_vsnprintf` are ALL textually renamed
//       to `glx_core_<name>_impl` BEFORE the compiler ever sees them (a plain object-like
//       `#define` substitutes every occurrence of the bare identifier, including a file's OWN
//       internal calls -- e.g. src/alloc.c's `realloc()` calling `malloc`/`free`/`memcpy`
//       internally, or src/printf.c calling `strlen`/`itoa`/`utoa`/`ftoa` internally -- the
//       rename applies there too, consistently, since $(CORE_RENAME_FLAGS) is the SAME flag set
//       for every TU in this build). SECURITY-ENGINEER FINDING (post-decb2fb review, CRITICAL,
//       fixed by widening this list -- see the Makefile's $(CORE_RENAME_FLAGS) comment for the
//       full story): the ORIGINAL 7-name list (malloc/free/realloc/mem*) left str.c/conv.c/
//       printf.c's libc-shaped names BARE in the archive, since `wildcard src/*.c` archives
//       every src/*.c file regardless of whether core_api.c itself calls into it -- a hosted
//       consumer linking `libcore.a` would silently hijack glibc's own `strlen`/`atoi`/etc. for
//       the WHOLE process. The calls in THIS file (`malloc(size)`, `free(ptr)`, etc. below)
//       undergo the EXACT SAME substitution when this file itself is compiled into
//       `build/objcore/core_api.o` -- so the object that actually lands in `build/libcore.a`
//       calls `glx_core_malloc_impl` internally and NEVER references (let alone defines) a bare
//       `malloc` symbol. Net result: `build/libcore.a`'s `alloc.o`/`mem.o`/`str.o`/`conv.o`/
//       `printf.o` members define ONLY `glx_core_*_impl` names (never a bare libc-shaped
//       identifier), and `core_api.o` defines ONLY the public `glx_malloc`/`glx_free`/
//       `glx_realloc`/`glx_memcpy`/`glx_memset` (see `tools/check_libcore_symbols.sh`, run by
//       `make libcore-test`, which verifies this EXHAUSTIVELY with `nm -g` against a positive
//       whitelist -- not a blacklist of the 7 names that bit us the first time).
//
//     WHY NOT `objcopy --localize-symbol` INSTEAD (the mechanism briefly considered and
//     rejected): stripping `malloc`'s GLOBAL binding down to LOCAL on the archived copy of
//     `alloc.o` AFTER compiling it would ALSO hide it from `ar`'s own per-member symbol INDEX --
//     which is exactly what a static archive's lazy member-pulling relies on to resolve
//     `core_api.o`'s own call to `malloc()` when `core_api.o` gets pulled into a real link (a
//     consumer referencing `glx_malloc`). That would break the archive at link time
//     ("undefined reference to `malloc`") instead of merely hiding it from the FINAL consumer
//     binary's export table. The `#define`-rename approach sidesteps this entirely: there is
//     simply no symbol named bare `malloc` anywhere in the archive's core objects to hide --
//     `core_api.o`'s undefined reference is to `glx_core_malloc_impl`, which `alloc.o` (archive
//     variant) DOES export globally, so intra-archive resolution still works normally.
//
//     `alloc_owns_ptr` (declared in include/alloc_internal.h, defined in src/alloc.c) IS renamed
//     by `$(CORE_RENAME_FLAGS)` too (`-Dalloc_owns_ptr=glx_core_alloc_owns_ptr_impl`) -- it was
//     never a colliding libc name to begin with (no libc ships a function called
//     `alloc_owns_ptr`), but it is an internal helper with no business being a bare, exported
//     symbol in `build/libcore.a` either; folding it into the same `glx_core_*_impl` family is
//     simpler than carving out a bespoke whitelist entry for one extra name (see
//     tools/check_libcore_symbols.sh). It is called here under its own, unmodified name in the
//     NORMAL build variant (`build/obj/`, no renaming), and under `glx_core_alloc_owns_ptr_impl`
//     in the archive variant -- same textual-substitution mechanism as every other name above.
// PT: SOV-LIBCORE (ADR-0009, FT-F0) -- a porta-da-frente `glx_*` declarada em
//     include/core/core.h. Toda função aqui é um wrapper FINO (nenhuma lógica nova além da
//     checagem de fronteira do gate 3 da ADR-0009 no `glx_free`) por cima das primitivas
//     freestanding PRÓPRIAS deste projeto, já existentes, já testadas (`malloc`/`free`/`realloc`
//     do src/alloc.c, `memcpy`/`memset` do src/mem.c) -- ver o cabeçalho de arquivo do
//     include/core/core.h pro PORQUÊ deste arquivo existir (a fronteira física da ADR-0009) e a
//     decisão alias-vs-rename que o brief desta tarefa deixou em aberto.
//
//     O MECANISMO DE RENAME (este é o truque que sustenta tudo, leia com atenção): este exato
//     arquivo-fonte é compilado DUAS VEZES pelo Makefile, com macros de pré-processador
//     DIFERENTES cada vez --
//     - Build normal (`make build`/`make test`, diretório de objeto `build/obj/`): compilado com
//       o MESMO `$(CFLAGS)` que todo outro src/*.c recebe, SEM rename. O `malloc(size)` abaixo
//       chama literalmente o `malloc` próprio do src/alloc.c (também compilado do jeito normal,
//       na MESMA árvore `build/obj/`) -- esta variante é linkada em todo programa-gate
//       freestanding de tests/*.c via `$(RUNTIME_OBJS)` (o wildcard `src/*.c` do Makefile pega
//       este arquivo automaticamente, exatamente como toda outra primitiva de runtime; ver o
//       comentário de topo-do-Makefile sobre esse design de wildcard). Inofensivo ali:
//       `glx_malloc` etc. são simplesmente funções extras, não-usadas-por-nenhum-programa-gate,
//       viajando junto -- zero dependência nova de libc (chamam nosso PRÓPRIO malloc/memcpy, já
//       zero-libc), zero mudança de comportamento em teste nenhum existente.
//     - Build do archive (`make libcore`, diretório de objeto `build/objcore/`): compilado com
//       `$(CORE_RENAME_FLAGS)` (Makefile), aplicado UNIFORMEMENTE a todo src/*.c que cai em
//       `build/objcore/` -- então as definições de `malloc`/`free`/`realloc`/`alloc_owns_ptr` do
//       src/alloc.c, de `memcpy`/`memset`/`memmove`/`memcmp` do src/mem.c, de
//       `strlen`/`strcmp`/`strncmp`/`strcpy`/`strncpy`/`strcat`/`strchr` do src/str.c, de
//       `atoi`/`atof`/`atou`/`itoa`/`utoa`/`ftoa` do src/conv.c, e de
//       `mini_printf`/`mini_vsnprintf` do src/printf.c são TODAS renomeadas textualmente pra
//       `glx_core_<nome>_impl` ANTES do compilador sequer vê-las (um `#define` simples estilo
//       object-like substitui toda ocorrência do identificador cru, incluindo chamadas INTERNAS
//       próprias de um arquivo -- ex.: o `realloc()` do src/alloc.c chamando
//       `malloc`/`free`/`memcpy` internamente, ou o src/printf.c chamando
//       `strlen`/`itoa`/`utoa`/`ftoa` internamente -- o rename se aplica ali também, de forma
//       consistente, já que $(CORE_RENAME_FLAGS) é o MESMO conjunto de flag pra toda TU deste
//       build). ACHADO DO SECURITY-ENGINEER (revisão pós-decb2fb, CRÍTICO, corrigido ampliando
//       esta lista -- ver o comentário do $(CORE_RENAME_FLAGS) no Makefile pra história
//       completa): a lista ORIGINAL de 7 nomes (malloc/free/realloc/mem*) deixava os nomes no
//       formato de libc do str.c/conv.c/printf.c CRUS no archive, já que `wildcard src/*.c`
//       arquiva todo src/*.c independente de o próprio core_api.c chamar nele -- um consumidor
//       hosted linkando `libcore.a` sequestraria em silêncio o `strlen`/`atoi`/etc. PRÓPRIO da
//       glibc pro processo INTEIRO. As chamadas NESTE arquivo (`malloc(size)`, `free(ptr)`, etc.
//       abaixo) passam pela EXATA MESMA substituição quando este arquivo é compilado em
//       `build/objcore/core_api.o` -- então o objeto que de fato cai na `build/libcore.a` chama
//       `glx_core_malloc_impl` internamente e NUNCA referencia (muito menos define) um símbolo
//       `malloc` cru. Resultado líquido: os membros `alloc.o`/`mem.o`/`str.o`/`conv.o`/`printf.o`
//       da `build/libcore.a` definem SÓ nomes `glx_core_*_impl` (nunca um identificador cru no
//       formato de libc), e o `core_api.o` define SÓ o
//       `glx_malloc`/`glx_free`/`glx_realloc`/`glx_memcpy`/`glx_memset` públicos (ver
//       `tools/check_libcore_symbols.sh`, rodado pelo `make libcore-test`, que verifica isso
//       EXAUSTIVAMENTE com `nm -g` contra uma whitelist positiva -- não uma lista negra dos 7
//       nomes que nos morderam da primeira vez).
//
//     POR QUE NÃO `objcopy --localize-symbol` EM VEZ DISSO (o mecanismo brevemente considerado e
//     rejeitado): rebaixar o binding GLOBAL do `malloc` pra LOCAL na cópia archived do `alloc.o`
//     DEPOIS de compilá-lo TAMBÉM esconderia ele do ÍNDICE de símbolo por-membro do próprio `ar`
//     -- que é exatamente o que o pull preguiçoso de membro de um archive estático depende pra
//     resolver a própria chamada do `core_api.o` a `malloc()` quando `core_api.o` é puxado pra
//     dentro de um link de verdade (um consumidor referenciando `glx_malloc`). Isso quebraria o
//     archive em tempo de link ("undefined reference to `malloc`") em vez de só esconder ele da
//     tabela de exportação do binário FINAL do consumidor. A abordagem de rename via `#define`
//     evita isso completamente: simplesmente não existe símbolo nenhum chamado `malloc` cru em
//     lugar nenhum dos objetos-núcleo do archive pra esconder -- a referência não-resolvida do
//     `core_api.o` é pro `glx_core_malloc_impl`, que o `alloc.o` (variante archive) EXPORTA
//     globalmente, então a resolução intra-archive continua funcionando normalmente.
//
//     O `alloc_owns_ptr` (declarado em include/alloc_internal.h, definido em src/alloc.c) TAMBÉM
//     é renomeado pelo `$(CORE_RENAME_FLAGS)` agora (`-Dalloc_owns_ptr=glx_core_alloc_owns_ptr_impl`)
//     -- nunca foi um nome colidente de libc pra começo de conversa (nenhuma libc entrega uma
//     função chamada `alloc_owns_ptr`), mas é um helper interno sem motivo pra ser um símbolo
//     cru, exportado, na `build/libcore.a` tampouco; dobrar ele na mesma família
//     `glx_core_*_impl` é mais simples que abrir uma entrada de whitelist sob-medida pra um nome
//     extra (ver tools/check_libcore_symbols.sh). É chamado aqui sob o próprio nome, sem
//     modificação, na variante NORMAL de build (`build/obj/`, sem rename), e sob
//     `glx_core_alloc_owns_ptr_impl` na variante archive -- mesmo mecanismo de substituição
//     textual de todo outro nome acima.
// Copyright (c) 2026 Petrus Silva Costa
#include "core/core.h"

#include "alloc.h"
#include "alloc_internal.h"
#include "mem.h"
#include "sys_exit.h"

// EN: Distinctive exit code for the ADR-0009 gate-3 boundary-misuse detection below -- NOT the
//     same as src/alloc.c's own `ALLOC_DOUBLE_FREE_EXIT_CODE` (111), a DIFFERENT failure class
//     (foreign pointer, not a legitimate pointer freed twice). See tests/libcore_consumer.cpp
//     for the forked, live-executed proof this code is what actually terminates the process.
// PT: Código de saída distintivo pra detecção de mau-uso de fronteira do gate 3 da ADR-0009
//     abaixo -- NÃO o mesmo `ALLOC_DOUBLE_FREE_EXIT_CODE` (111) do src/alloc.c, uma classe de
//     falha DIFERENTE (ponteiro estrangeiro, não um ponteiro legítimo liberado duas vezes). Ver
//     tests/libcore_consumer.cpp pra prova viva, executada via fork, de que este código é o que
//     de fato termina o processo.
#define GLX_FREE_FOREIGN_PTR_EXIT_CODE 112

void* glx_malloc(size_t size) {
    return malloc(size);
}

void glx_free(void* ptr) {
    // EN: ADR-0009 gate 3, enforced (not just documented): refuse to hand a pointer this
    //     allocator never issued down into `free()`'s header-arithmetic/coalescing logic, which
    //     assumes EVERY byte at `ptr - ALLOC_HEADER_SIZE` onward is one of THIS allocator's own
    //     block headers. A foreign pointer (e.g. straight from glibc's `malloc()`) almost always
    //     fails this cheap check and dies here, LOUDLY, via `sys_exit` -- never silently
    //     corrupting either heap. See include/alloc_internal.h for exactly what `alloc_owns_ptr`
    //     does and does not guarantee.
    // PT: Gate 3 da ADR-0009, APLICADO (não só documentado): recusa entregar um ponteiro que
    //     este alocador nunca emitiu pra dentro da lógica de aritmética-de-header/coalescência
    //     do `free()`, que assume que TODO byte a partir de `ptr - ALLOC_HEADER_SIZE` é um dos
    //     headers de bloco DESTE alocador. Um ponteiro estrangeiro (ex.: direto do `malloc()` da
    //     glibc) quase sempre falha nessa checagem barata e morre aqui, ALTO, via `sys_exit` --
    //     nunca corrompendo nenhum dos dois heaps em silêncio. Ver include/alloc_internal.h pro
    //     que exatamente o `alloc_owns_ptr` garante e não garante.
    if (ptr != NULL && !alloc_owns_ptr(ptr)) {
        sys_exit(GLX_FREE_FOREIGN_PTR_EXIT_CODE);
    }
    free(ptr);
}

void* glx_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void* glx_memcpy(void* dst, const void* src, size_t n) {
    return memcpy(dst, src, n);
}

void* glx_memset(void* dst, int c, size_t n) {
    return memset(dst, c, n);
}
