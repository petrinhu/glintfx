// SPDX-License-Identifier: MPL-2.0
// EN: SOV-ALLOC (W15) -- the fifth NAMED syscall wrapper (ADR-0001), `sys_mmap`'s natural pair.
//     Mirrors POSIX `munmap()`: unmaps `len` bytes starting at `addr` (both MUST come from a
//     prior successful `sys_mmap()` call in this process -- there is no libc to validate the
//     region for us, an arbitrary/misaligned/foreign address is undefined kernel behaviour,
//     same caveat as raw `munmap(2)`). Returns the raw kernel value: `0` on success, a negative
//     `-errno` (ADR-0002) on failure -- the caller decides whether/how to interpret it, same
//     division of labour as every other `sys_*` wrapper in this codebase. The only caller today
//     is E1's allocator (src/alloc.c), returning a fully-empty arena to the kernel when its
//     last live block is freed -- an OPTIONAL stretch goal of SOV-ALLOC's free-list rewrite,
//     not part of the malloc/free/realloc CONTRACT itself (ADR-0004): if `sys_munmap` ever
//     fails there, the allocator simply leaves that arena mapped (a tolerated leak, never a
//     crash or corruption -- see src/alloc.c's own header comment for the full rationale).
// PT: SOV-ALLOC (W15) -- o quinto wrapper de syscall NOMEADO (ADR-0001), o par natural do
//     `sys_mmap`. Espelha o `munmap()` POSIX: desmapeia `len` bytes a partir de `addr` (os dois
//     PRECISAM vir de uma chamada `sys_mmap()` bem-sucedida anterior neste processo -- não há
//     libc pra validar a região por nós, um endereço arbitrário/desalinhado/alheio é
//     comportamento indefinido do kernel, mesma ressalva do `munmap(2)` cru). Retorna o valor
//     cru do kernel: `0` em sucesso, um `-errno` negativo (ADR-0002) em falha -- quem chama
//     decide se/como interpretar, mesma divisão de trabalho de todo outro wrapper `sys_*` neste
//     código-base. O único chamador hoje é o alocador do E1 (src/alloc.c), devolvendo ao kernel
//     uma arena inteiramente vazia quando seu último bloco vivo é liberado -- um stretch goal
//     OPCIONAL da reescrita free-list do SOV-ALLOC, não parte do CONTRATO malloc/free/realloc
//     em si (ADR-0004): se o `sys_munmap` alguma vez falhar ali, o alocador simplesmente deixa
//     aquela arena mapeada (um leak tolerado, nunca um crash ou corrupção -- ver o próprio
//     comentário de cabeçalho de src/alloc.c pro racional completo).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

long sys_munmap(void* addr, size_t len);
