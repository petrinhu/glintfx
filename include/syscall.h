// SPDX-License-Identifier: MPL-2.0
// EN: Generic arity-based raw syscall wrappers (ADR-0001). One NASM leaf function per arg
//     count -- AUD-C0-2 originally pruned syscall0/2/4/5 as dead code (no real call-site at the
//     time), and ADR-0001 already said re-adding an arity is cheap if a future syscall needs
//     it: `syscall2` is back (SOV-ALLOC, W15) because `sys_munmap` (`munmap(addr, len)`, 2
//     arguments) is a genuine new call-site -- `syscall0`/`syscall4`/`syscall5` remain pruned,
//     still no caller. Each wrapper moves the System V C-ABI argument registers into the raw
//     `syscall` instruction's own convention (rax=nr, rdi/rsi/rdx/r10/r8/r9=args) and returns
//     the kernel's raw value in rax -- callers apply ADR-0002's `-errno` contract themselves
//     (these wrappers never inspect or translate the return value).
// PT: Wrappers genericos de syscall crua por aridade (ADR-0001). Uma funcao NASM leaf por
//     numero de args -- a AUD-C0-2 originalmente podou syscall0/2/4/5 como codigo morto (sem
//     call-site real na epoca), e o ADR-0001 ja dizia que readicionar uma aridade e barato se
//     uma syscall futura precisar: o `syscall2` volta (SOV-ALLOC, W15) porque `sys_munmap`
//     (`munmap(addr, len)`, 2 argumentos) e' um call-site novo genuino -- `syscall0`/
//     `syscall4`/`syscall5` continuam podados, ainda sem chamador. Cada wrapper move os
//     registradores de argumento da C-ABI System V pra convencao propria da instrucao
//     `syscall` (rax=nr, rdi/rsi/rdx/r10/r8/r9=args) e retorna o valor cru do kernel em rax --
//     quem chama aplica o contrato `-errno` do ADR-0002 por conta propria (estes wrappers nunca
//     inspecionam nem traduzem o retorno).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

long syscall1(long nr, long a1);
long syscall2(long nr, long a1, long a2);
long syscall3(long nr, long a1, long a2, long a3);
long syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6);
