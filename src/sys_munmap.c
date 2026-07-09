// SPDX-License-Identifier: MPL-2.0
// EN: sys_munmap -- see include/sys_munmap.h for the contract. Implemented as a 1-line wrapper
//     over syscall2(SYS_munmap, ...) (ADR-0001, syscall2 re-added by this same feature): all
//     the register-shuffling logic lives once, in syscall.asm -- this file adds only the name,
//     the constant, and the pointer/size-to-`long` casts the raw syscall ABI (all-`long` args)
//     requires.
// PT: sys_munmap -- ver include/sys_munmap.h pro contrato. Implementado como um wrapper de 1
//     linha sobre syscall2(SYS_munmap, ...) (ADR-0001, syscall2 readicionada por esta mesma
//     feature): toda a lógica de reorganização de registrador mora uma única vez, em
//     syscall.asm -- este arquivo só acrescenta o nome, a constante, e os casts
//     ponteiro/tamanho-pra-`long` que a ABI crua da syscall (argumentos todos `long`) exige.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_munmap.h"
#include "syscall.h"
#include "syscall_nums.h"

// EN: TST-STATIC (TODO.md, W11): cppcheck would flag `addr` as "can be declared as pointer to
//     const" for the same reason as sys_mmap.c's `addr`/sys_read.c's `buf` -- this function
//     body never writes through it. Kept non-const on purpose to mirror POSIX `munmap(void*,
//     size_t)`'s public signature (include/sys_munmap.h); narrowing it here would desync the
//     definition from that declaration for zero real safety gain. Suppressed cirurgically, not
//     the whole check (same pattern as src/sys_mmap.c/src/sys_read.c).
// PT: TST-STATIC (TODO.md, W11): o cppcheck apontaria `addr` como "pode ser declarado como
//     ponteiro pra const" pelo mesmo motivo do `addr` de sys_mmap.c/`buf` de sys_read.c -- o
//     corpo desta função nunca escreve através dele. Mantido não-const de propósito, espelhando
//     a assinatura pública do `munmap(void*, size_t)` POSIX (include/sys_munmap.h); estreitar
//     aqui dessincronizaria a definição daquela declaração sem ganho real de segurança.
//     Suprimido cirurgicamente, não a checagem inteira (mesmo padrão de src/sys_mmap.c/
//     src/sys_read.c).
// cppcheck-suppress constParameterPointer
long sys_munmap(void* addr, size_t len) {
    return syscall2(SYS_munmap, (long)addr, (long)len);
}
