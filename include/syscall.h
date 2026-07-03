// SPDX-License-Identifier: MPL-2.0
// EN: Generic arity-based raw syscall wrappers (ADR-0001). One NASM leaf function per arg
//     count (0..6), each moving the System V C-ABI argument registers into the raw `syscall`
//     instruction's own convention (rax=nr, rdi/rsi/rdx/r10/r8/r9=args) and returning the
//     kernel's raw value in rax -- callers apply ADR-0002's `-errno` contract themselves
//     (these wrappers never inspect or translate the return value).
// PT: Wrappers genericos de syscall crua por aridade (ADR-0001). Uma funcao NASM leaf por
//     numero de args (0..6), cada uma movendo os registradores de argumento da C-ABI System V
//     pra convencao propria da instrucao `syscall` (rax=nr, rdi/rsi/rdx/r10/r8/r9=args) e
//     retornando o valor cru do kernel em rax -- quem chama aplica o contrato `-errno` do
//     ADR-0002 por conta propria (estes wrappers nunca inspecionam nem traduzem o retorno).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

long syscall0(long nr);
long syscall1(long nr, long a1);
long syscall2(long nr, long a1, long a2);
long syscall3(long nr, long a1, long a2, long a3);
long syscall4(long nr, long a1, long a2, long a3, long a4);
long syscall5(long nr, long a1, long a2, long a3, long a4, long a5);
long syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6);
