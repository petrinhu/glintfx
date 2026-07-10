// SPDX-License-Identifier: MPL-2.0
// EN: SOV-LIBCORE (ADR-0009, FT-F0) -- Layer-0-INTERNAL companion to include/alloc.h. NOT part
//     of ADR-0004's frozen `malloc`/`free`/`realloc` contract (that header's signature has not
//     changed and does not change here either) -- this is a SEPARATE, additive helper consumed
//     exclusively by src/core_api.c's `glx_free()` to implement ADR-0009 gate 3 (the heap
//     ownership rule: a pointer handed out by `glx_malloc` must never reach glibc's `free()`,
//     and a pointer from a foreign allocator must never reach `glx_free()`).
// PT: SOV-LIBCORE (ADR-0009, FT-F0) -- companheiro INTERNO da Camada 0 do include/alloc.h. NÃO
//     faz parte do contrato congelado `malloc`/`free`/`realloc` da ADR-0004 (a assinatura
//     daquele header não mudou e não muda aqui também) -- este é um helper SEPARADO, aditivo,
//     consumido exclusivamente pelo `glx_free()` do src/core_api.c pra implementar o gate 3 da
//     ADR-0009 (a regra de ownership de heap: um ponteiro entregue pelo `glx_malloc` nunca pode
//     chegar ao `free()` da glibc, e um ponteiro de um alocador estrangeiro nunca pode chegar ao
//     `glx_free()`).
#pragma once

#include "types.h"

// EN: Returns 1 if `ptr` LOOKS LIKE a pointer this allocator itself handed out -- i.e. its
//     header address (`ptr - ALLOC_HEADER_SIZE`, mirroring src/alloc.c's own block layout)
//     falls within the `[base, end)` usable region of SOME arena currently tracked in
//     src/alloc.c's `g_arenas` registry. Returns 0 for `ptr == NULL` or any pointer outside
//     every known arena. O(number of live arenas) -- cheap, no free-list walk, no heap
//     mutation, safe to call before deciding whether to proceed with a real `free()`.
//
//     WHAT THIS DOES NOT GUARANTEE (best-effort, not a formal proof -- same honesty standard
//     include/alloc.h already holds itself to for double-free detection): a foreign pointer
//     that happens to alias an address inside one of THIS allocator's live `mmap`'d arenas by
//     sheer coincidence would incorrectly pass. In practice this allocator's arenas (anonymous
//     `mmap` regions, see src/alloc.c) and glibc's heap (`brk`-grown main arena plus its own
//     independently `mmap`'d large-allocation regions) occupy DISJOINT address ranges under the
//     kernel's normal `mmap` placement policy -- there is no adversarial input a CALLER controls
//     that can force such an alias, so this is a real, cheap, high-value defense-in-depth
//     signal for the ADR-0009 gate-3 misuse case (a plain, unmodified pointer from glibc's
//     `malloc()` crossing the boundary), not a cryptographic guarantee against a
//     deliberately-crafted adversarial pointer.
// PT: Retorna 1 se `ptr` PARECE um ponteiro que este alocador entregou ele mesmo -- i.e. o
//     endereço do header dele (`ptr - ALLOC_HEADER_SIZE`, espelhando o layout de bloco do
//     próprio src/alloc.c) cai dentro da região utilizável `[base, end)` de ALGUMA arena
//     atualmente rastreada no registro `g_arenas` do src/alloc.c. Retorna 0 pra `ptr == NULL` ou
//     qualquer ponteiro fora de toda arena conhecida. O(número de arenas vivas) -- barato, sem
//     varredura de free-list, sem mutação de heap, seguro de chamar antes de decidir se segue
//     com um `free()` de verdade.
//
//     O QUE ISSO NÃO GARANTE (melhor esforço, não uma prova formal -- mesmo padrão de honestidade
//     que o include/alloc.h já mantém pra detecção de double-free): um ponteiro estrangeiro que
//     por pura coincidência aliase um endereço dentro de uma das arenas `mmap`adas VIVAS deste
//     alocador passaria incorretamente. Na prática as arenas deste alocador (regiões `mmap`
//     anônimas, ver src/alloc.c) e o heap da glibc (arena principal crescida via `brk` mais suas
//     próprias regiões `mmap`adas independentes pra alocações grandes) ocupam faixas de endereço
//     DISJUNTAS sob a política normal de posicionamento de `mmap` do kernel -- não há input
//     adversarial que quem CHAMA controle capaz de forçar esse alias, então este é um sinal de
//     defesa em profundidade real, barato e de alto valor pro caso de mau-uso do gate 3 da
//     ADR-0009 (um ponteiro simples, não-modificado, do `malloc()` da glibc cruzando a
//     fronteira), não uma garantia criptográfica contra um ponteiro adversarial deliberadamente
//     forjado.
int alloc_owns_ptr(const void* ptr);
