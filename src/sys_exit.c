// SPDX-License-Identifier: MPL-2.0
// EN: sys_exit -- see include/sys_exit.h for the contract. Implemented as a 1-line wrapper
//     over syscall1(SYS_exit, code) (B3/ADR-0001): all the register-shuffling logic lives
//     once, in syscall.asm -- this file adds only the name and the constant.
// PT: sys_exit -- ver include/sys_exit.h pro contrato. Implementado como um wrapper de 1
//     linha sobre syscall1(SYS_exit, code) (B3/ADR-0001): toda a logica de reorganizacao de
//     registrador mora uma unica vez, em syscall.asm -- este arquivo so acrescenta o nome e
//     a constante.
// Copyright (c) 2026 Petrus Silva Costa
#include "sys_exit.h"
#include "syscall.h"
#include "syscall_nums.h"

void sys_exit(int code) {
    syscall1(SYS_exit, code);
    __builtin_unreachable(); // SYS_exit never returns to here.
}
