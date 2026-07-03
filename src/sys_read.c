// SPDX-License-Identifier: MPL-2.0
// EN: sys_read -- see include/sys_read.h for the contract. Implemented as a 1-line wrapper
//     over syscall3(SYS_read, fd, buf, count) (B3/ADR-0001): all the register-shuffling logic
//     lives once, in syscall.asm -- this file adds only the name, the constant, and the two
//     pointer/size-to-`long` casts the raw syscall ABI (all-`long` args) requires.
// PT: sys_read -- ver include/sys_read.h pro contrato. Implementado como um wrapper de 1
//     linha sobre syscall3(SYS_read, fd, buf, count) (B3/ADR-0001): toda a logica de
//     reorganizacao de registrador mora uma unica vez, em syscall.asm -- este arquivo so
//     acrescenta o nome, a constante, e os dois casts ponteiro/tamanho-pra-`long` que a ABI
//     crua da syscall (argumentos todos `long`) exige.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_read.h"
#include "syscall.h"
#include "syscall_nums.h"

ssize_t sys_read(int fd, void* buf, size_t count) {
    return (ssize_t)syscall3(SYS_read, fd, (long)buf, (long)count);
}
