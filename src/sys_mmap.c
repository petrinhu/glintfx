// SPDX-License-Identifier: MPL-2.0
// EN: sys_mmap -- see include/sys_mmap.h for the contract. Implemented as a 1-line wrapper
//     over syscall6(SYS_mmap, ...) (B3/ADR-0001): all the register-shuffling logic lives once,
//     in syscall.asm -- this file adds only the name, the constant, and the casts the raw
//     syscall ABI (all-`long` args) requires. `fd` and `off` are conventionally `-1`/`0` for
//     the anonymous mapping E1 uses (MAP_ANONYMOUS ignores them), but this wrapper is generic
//     over both -- it does not hardcode that assumption.
// PT: sys_mmap -- ver include/sys_mmap.h pro contrato. Implementado como um wrapper de 1 linha
//     sobre syscall6(SYS_mmap, ...) (B3/ADR-0001): toda a lógica de reorganização de
//     registrador mora uma única vez, em syscall.asm -- este arquivo só acrescenta o nome, a
//     constante, e os casts que a ABI crua da syscall (argumentos todos `long`) exige. `fd` e
//     `off` são por convenção `-1`/`0` para o mapeamento anônimo que o E1 usa (MAP_ANONYMOUS
//     os ignora), mas este wrapper é genérico sobre os dois -- não fixa essa suposição.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_mmap.h"
#include "syscall.h"
#include "syscall_nums.h"

void* sys_mmap(void* addr, size_t len, int prot, int flags, int fd, long off) {
    return (void*)syscall6(SYS_mmap, (long)addr, (long)len, (long)prot,
                            (long)flags, (long)fd, off);
}
