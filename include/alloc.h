// SPDX-License-Identifier: MPL-2.0
// EN: E1 -- the sovereign runtime's own dynamic allocator (no libc `malloc`). Final,
//     POSIX-shaped API fixed by ADR-0004 (one-way-door): `malloc`/`free`/`realloc`. The
//     INTERNAL strategy is deliberately the simplest one that satisfies the contract today --
//     a bump allocator carved out of `mmap`-backed arenas, `free` a no-op, `realloc` a
//     copy-and-abandon -- and is free to evolve to a real free-list LATER without touching
//     this signature (that is the whole point of fixing the signature now, per the ADR).
//
//     Contract details a caller must know (all documented here because there is no man page
//     to fall back on):
//
//     - `malloc(size)`: returns a pointer to at least `size` bytes, ALIGNED TO 16 BYTES (the
//       widest natural alignment on x86-64 -- covers `long double`/SSE types/any smaller
//       scalar), or `NULL` if the kernel could not back it (`mmap` failure) or `size` is large
//       enough that `size + header` would overflow `size_t`. `malloc(0)` returns `NULL`
//       (ADR-0004 follow-on decision, documented in src/alloc.c -- POSIX explicitly allows
//       either `NULL` or a unique non-dereferenceable pointer here; NULL was chosen as the
//       simpler, unambiguous-to-implement option with no `free`-tracking cost since `free` is
//       a no-op anyway).
//     - `free(ptr)`: NO-OP by design (ADR-0004, leak-by-design in this bootstrap increment,
//       flagged for `TST-MEM`, W11). Safe to call with `ptr == NULL` (also a no-op) or with any
//       pointer previously returned by `malloc`/`realloc` -- it never touches the memory.
//     - `realloc(ptr, size)`: `realloc(NULL, size)` behaves exactly like `malloc(size)`.
//       `realloc(ptr, 0)` calls `free(ptr)` (a no-op) and returns `NULL`. Otherwise: allocates
//       a NEW block of `size` bytes, copies `min(old_size, size)` bytes from `ptr` into it
//       (the old block's size is recovered from its header -- see src/alloc.c), frees the old
//       block (no-op), and returns the new pointer. If the new allocation fails, returns `NULL`
//       and leaves `ptr` and its contents COMPLETELY UNTOUCHED and still valid -- this is the
//       standard `realloc` contract and callers may rely on it.
//
//     What this header does NOT expose (deliberately, YAGNI for this increment): no
//     `calloc`, no `aligned_alloc`, no way to query the arena's total/used size, no
//     `malloc_stats`-style introspection. Add only when a real caller needs one.
// PT: E1 -- o alocador dinâmico próprio da runtime soberana (sem `malloc` de libc). API final,
//     no formato POSIX, fixada pela ADR-0004 (one-way-door): `malloc`/`free`/`realloc`. A
//     estratégia INTERNA é deliberadamente a mais simples que satisfaz o contrato hoje -- um
//     bump allocator recortado de arenas lastreadas em `mmap`, `free` um no-op, `realloc` uma
//     cópia-e-abandono -- e é livre pra evoluir pra uma free-list de verdade DEPOIS sem tocar
//     nesta assinatura (esse é o ponto inteiro de fixar a assinatura agora, conforme a ADR).
//
//     Detalhes de contrato que quem chama precisa saber (tudo documentado aqui porque não há
//     man page pra recorrer):
//
//     - `malloc(size)`: retorna um ponteiro pra pelo menos `size` bytes, ALINHADO A 16 BYTES
//       (o alinhamento natural mais largo em x86-64 -- cobre `long double`/tipos SSE/qualquer
//       escalar menor), ou `NULL` se o kernel não conseguiu lastrear (falha do `mmap`) ou
//       `size` é grande o bastante pra `size + header` estourar `size_t`. `malloc(0)` retorna
//       `NULL` (decisão derivada da ADR-0004, documentada em src/alloc.c -- POSIX permite
//       explicitamente `NULL` OU um ponteiro único não-desreferenciável aqui; NULL foi
//       escolhido como a opção mais simples e inambígua de implementar, sem custo de rastrear
//       pra `free` já que `free` é no-op de qualquer jeito).
//     - `free(ptr)`: NO-OP por desenho (ADR-0004, leak-by-design neste incremento de
//       bootstrap, sinalizado pro `TST-MEM`, W11). Seguro chamar com `ptr == NULL` (também
//       no-op) ou com qualquer ponteiro previamente retornado por `malloc`/`realloc` -- nunca
//       toca a memória.
//     - `realloc(ptr, size)`: `realloc(NULL, size)` se comporta exatamente como
//       `malloc(size)`. `realloc(ptr, 0)` chama `free(ptr)` (um no-op) e retorna `NULL`. Caso
//       contrário: aloca um bloco NOVO de `size` bytes, copia `min(old_size, size)` bytes de
//       `ptr` pra ele (o tamanho do bloco antigo é recuperado do seu header -- ver
//       src/alloc.c), libera o bloco antigo (no-op), e retorna o ponteiro novo. Se a alocação
//       nova falhar, retorna `NULL` e deixa `ptr` e seu conteúdo COMPLETAMENTE INTOCADOS e
//       ainda válidos -- este é o contrato padrão do `realloc` e quem chama pode confiar nisso.
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
