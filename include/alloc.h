// SPDX-License-Identifier: MPL-2.0
// EN: E1 -- the sovereign runtime's own dynamic allocator (no libc `malloc`). Final,
//     POSIX-shaped API fixed by ADR-0004 (one-way-door): `malloc`/`free`/`realloc`. THIS
//     SIGNATURE HAS NOT CHANGED since ADR-0004 -- that is the whole point of fixing it early.
//     What DID change (SOV-ALLOC, W15): the INTERNAL strategy evolved from a leak-by-design
//     bump allocator to a real free-list allocator with boundary-tag coalescing over `mmap`
//     arenas -- `free` now genuinely returns memory for reuse, exactly the evolution ADR-0004
//     always anticipated ("the internal strategy upgrades to a free-list later without changing
//     the API"). See src/alloc.c's file header for the internal design and its documented
//     trade-offs.
//
//     Contract details a caller must know (all documented here because there is no man page
//     to fall back on):
//
//     - `malloc(size)`: returns a pointer to at least `size` bytes, ALIGNED TO 16 BYTES (the
//       widest natural alignment on x86-64 -- covers `long double`/SSE types/any smaller
//       scalar), or `NULL` if the kernel could not back it (`mmap` failure) or `size` is large
//       enough that the internal bookkeeping arithmetic would overflow `size_t`. `malloc(0)`
//       returns `NULL` (ADR-0004 follow-on decision -- POSIX explicitly allows either `NULL` or
//       a unique non-dereferenceable pointer here; NULL was chosen as the simpler,
//       unambiguous-to-implement option). This 16-byte alignment guarantee holds for EVERY
//       pointer `malloc` can hand back, including one recovered from the free-list and reused
//       after a prior `free()` -- alignment is a structural invariant of every block's
//       total footprint (see src/alloc.c), not something re-derived only for freshly-`mmap`'d
//       memory.
//     - `free(ptr)`: returns the block to the allocator for REUSE by future `malloc`/`realloc`
//       calls (SOV-ALLOC, W15 -- no longer a no-op). Internally: the block is marked free,
//       coalesced with any immediately-adjacent free neighbour (both directions, via boundary
//       tags -- see src/alloc.c), and linked into a free-list a future `malloc` searches
//       first-fit before ever asking the kernel for more memory. Safe to call with
//       `ptr == NULL` (a no-op) or with any pointer previously returned by `malloc`/`realloc`
//       that has not already been freed.
//       **Double-free is UNDEFINED BEHAVIOUR.** Calling `free()` twice on the same pointer (or
//       on a pointer derived from one already freed) without an intervening `malloc`/`realloc`
//       that legitimately re-issued it is a bug in the CALLER, exactly like libc's `free()`. As
//       a cheap defense-in-depth measure (the flag bit this check reads is already touched by
//       every `free()` call, so the cost is effectively zero -- no free-list walk, no
//       redzone/canary scanning), the current implementation DETECTS the specific case where
//       the pointer's own header is STILL MAPPED and already marked free, terminating the
//       process immediately via `sys_exit(111)` rather than silently corrupting the free-list.
//       This is a BEST-EFFORT diagnostic, not a guaranteed contract, and callers must never rely
//       on any particular termination behaviour -- two documented gaps: (1) it cannot catch a
//       double-free through two DIFFERENT stale pointers into a block that has since been
//       reused, split, or coalesced away; (2) if the block being double-freed was the LAST live
//       occupant of its arena, the first `free()` may already have returned that entire arena to
//       the kernel via `munmap` (see the optional stretch in src/alloc.c) -- in that case the
//       SECOND `free()` faults reading the now-unmapped header (`SIGSEGV`) BEFORE the flag check
//       ever runs, instead of the clean `sys_exit(111)` path. Either way the process terminates
//       immediately rather than continuing with a corrupted free-list, but the exact signal/exit
//       code is NOT something to branch on -- simply never double-free.
//       **Touching memory after `free()` is UNDEFINED BEHAVIOUR** (this is a change from the
//       bootstrap bump allocator, which was leak-by-design and safe to poke post-free -- see
//       the "what changed" note at the top of this file). A future `malloc` may hand that exact
//       memory to a DIFFERENT, live allocation (writing through the stale pointer then
//       corrupts unrelated data); the arena that block lived in may even have been returned to
//       the kernel entirely via `munmap` if it became fully empty (an allocator-internal
//       optimization, not part of the contract -- see src/alloc.c), in which case touching it
//       faults the process (`SIGSEGV`).
//     - `realloc(ptr, size)`: `realloc(NULL, size)` behaves exactly like `malloc(size)`.
//       `realloc(ptr, 0)` calls `free(ptr)` (a real free now, see above) and returns `NULL`.
//       Otherwise: allocates a NEW block of `size` bytes, copies bytes from the OLD block into
//       it, frees the old block, and returns the new pointer. The number of bytes copied is
//       `min(old_usable_capacity, size)`, where `old_usable_capacity` is the old block's full
//       usable payload as recovered from its header (`>=` the size originally requested from
//       `malloc`/a prior `realloc` -- internal padding/minimum-block-size rounding can make it
//       strictly larger; this is never fewer bytes than the caller actually wrote, and never an
//       out-of-bounds read: the extra bytes, if any, still belong to the SAME old block). If
//       the new allocation fails, returns `NULL` and leaves `ptr` and its contents COMPLETELY
//       UNTOUCHED and still valid -- this is the standard `realloc` contract and callers may
//       rely on it.
//
//     What this header does NOT expose (deliberately, YAGNI for this increment): no
//     `calloc`, no `aligned_alloc`, no way to query the arena's total/used size, no
//     `malloc_stats`-style introspection. Add only when a real caller needs one.
// PT: E1 -- o alocador dinâmico próprio da runtime soberana (sem `malloc` de libc). API final,
//     no formato POSIX, fixada pela ADR-0004 (one-way-door): `malloc`/`free`/`realloc`. ESTA
//     ASSINATURA NÃO MUDOU desde a ADR-0004 -- esse é o ponto inteiro de fixá-la cedo. O que
//     MUDOU (SOV-ALLOC, W15): a estratégia INTERNA evoluiu de um bump allocator leak-by-design
//     pra um alocador free-list de verdade com coalescência via boundary tags sobre arenas
//     `mmap` -- o `free` agora devolve memória de verdade pra reuso, exatamente a evolução que a
//     ADR-0004 sempre previu ("a estratégia interna evolui para free-list depois sem mudar a
//     API"). Ver o cabeçalho de arquivo de src/alloc.c pro desenho interno e seus trade-offs
//     documentados.
//
//     Detalhes de contrato que quem chama precisa saber (tudo documentado aqui porque não há
//     man page pra recorrer):
//
//     - `malloc(size)`: retorna um ponteiro pra pelo menos `size` bytes, ALINHADO A 16 BYTES
//       (o alinhamento natural mais largo em x86-64 -- cobre `long double`/tipos SSE/qualquer
//       escalar menor), ou `NULL` se o kernel não conseguiu lastrear (falha do `mmap`) ou
//       `size` é grande o bastante pra a aritmética interna de contabilidade estourar `size_t`.
//       `malloc(0)` retorna `NULL` (decisão derivada da ADR-0004 -- POSIX permite explicitamente
//       `NULL` OU um ponteiro único não-desreferenciável aqui; NULL foi escolhido como a opção
//       mais simples e inambígua de implementar). Esta garantia de alinhamento a 16 bytes vale
//       pra TODO ponteiro que o `malloc` pode devolver, inclusive um recuperado da free-list e
//       reusado depois de um `free()` anterior -- alinhamento é um invariante estrutural do
//       footprint total de todo bloco (ver src/alloc.c), não algo re-derivado só pra memória
//       recém-`mmap`ada.
//     - `free(ptr)`: devolve o bloco ao alocador pra REUSO por futuras chamadas de
//       `malloc`/`realloc` (SOV-ALLOC, W15 -- não é mais um no-op). Internamente: o bloco é
//       marcado livre, coalescido com qualquer vizinho livre imediatamente adjacente (nas duas
//       direções, via boundary tags -- ver src/alloc.c), e encadeado numa free-list que um
//       futuro `malloc` busca first-fit antes de sequer pedir mais memória ao kernel. Seguro
//       chamar com `ptr == NULL` (um no-op) ou com qualquer ponteiro previamente retornado por
//       `malloc`/`realloc` que ainda não foi liberado.
//       **Double-free é COMPORTAMENTO INDEFINIDO.** Chamar `free()` duas vezes no mesmo ponteiro
//       (ou num ponteiro derivado de um já liberado) sem um `malloc`/`realloc` intermediário que
//       tenha legitimamente reemitido ele é um bug de quem CHAMA, exatamente como o `free()` da
//       libc. Como medida barata de defesa em profundidade (o bit de flag que esta checagem lê
//       já é tocado por toda chamada de `free()`, então o custo é efetivamente zero -- sem
//       varredura de free-list, sem scan de redzone/canário), a implementação atual DETECTA o
//       caso específico onde o header do próprio ponteiro AINDA ESTÁ MAPEADO e já está marcado
//       livre, terminando o processo imediatamente via `sys_exit(111)` em vez de corromper a
//       free-list em silêncio. Este é um diagnóstico de MELHOR ESFORÇO, não um contrato
//       garantido, e quem chama nunca deve confiar em nenhum comportamento de terminação
//       específico -- duas lacunas documentadas: (1) não pega um double-free através de dois
//       ponteiros stale DIFERENTES pra um bloco que desde então foi reusado, splitado, ou
//       coalescido embora; (2) se o bloco sendo double-freed era o ÚLTIMO ocupante vivo da arena
//       dele, o primeiro `free()` já pode ter devolvido aquela arena inteira ao kernel via
//       `munmap` (ver o stretch opcional em src/alloc.c) -- nesse caso o SEGUNDO `free()` falta
//       lendo o header agora desmapeado (`SIGSEGV`) ANTES da checagem de flag sequer rodar, em
//       vez do caminho limpo `sys_exit(111)`. De um jeito ou de outro o processo termina
//       imediatamente em vez de continuar com uma free-list corrompida, mas o sinal/código de
//       saída exato NÃO é algo pra ramificar -- simplesmente nunca faça double-free.
//       **Tocar memória depois do `free()` é COMPORTAMENTO INDEFINIDO** (isto é uma mudança em
//       relação ao bump allocator de bootstrap, que era leak-by-design e seguro de cutucar
//       pós-free -- ver a nota "o que mudou" no topo deste arquivo). Um `malloc` futuro pode
//       entregar essa memória exata a uma alocação DIFERENTE e viva (escrever através do
//       ponteiro stale então corrompe dado alheio); a arena onde aquele bloco morava pode até
//       ter sido devolvida ao kernel inteiramente via `munmap` se ficou totalmente vazia (uma
//       otimização interna do alocador, não parte do contrato -- ver src/alloc.c), caso em que
//       tocá-la falta o processo (`SIGSEGV`).
//     - `realloc(ptr, size)`: `realloc(NULL, size)` se comporta exatamente como
//       `malloc(size)`. `realloc(ptr, 0)` chama `free(ptr)` (um free de verdade agora, ver
//       acima) e retorna `NULL`. Caso contrário: aloca um bloco NOVO de `size` bytes, copia
//       bytes do bloco ANTIGO pra ele, libera o bloco antigo, e retorna o ponteiro novo. O
//       número de bytes copiados é `min(capacidade_utilizável_antiga, size)`, onde
//       `capacidade_utilizável_antiga` é o payload utilizável completo do bloco antigo,
//       recuperado do header dele (`>=` o tamanho originalmente pedido ao `malloc`/a um
//       `realloc` anterior -- arredondamento de padding/tamanho-mínimo-de-bloco interno pode
//       torná-la estritamente maior; isso nunca é menos bytes do que quem chamou de fato
//       escreveu, e nunca uma leitura fora dos limites: os bytes extras, se houver, ainda
//       pertencem ao MESMO bloco antigo). Se a alocação nova falhar, retorna `NULL` e deixa
//       `ptr` e seu conteúdo COMPLETAMENTE INTOCADOS e ainda válidos -- este é o contrato padrão
//       do `realloc` e quem chama pode confiar nisso.
//
//     O que este header NÃO expõe (deliberadamente, YAGNI pra este incremento): sem
//     `calloc`, sem `aligned_alloc`, sem forma de consultar tamanho total/usado da arena, sem
//     introspecção estilo `malloc_stats`. Acrescentar só quando um chamador real precisar.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

void* malloc(size_t size);
void  free(void* ptr);
void* realloc(void* ptr, size_t size);
