// SPDX-License-Identifier: MPL-2.0
// EN: Generic arity-based raw syscall wrappers (ADR-0001). One NASM leaf function per arg
//     count -- only arities 1, 3 and 6 exist; AUD-C0-2 pruned syscall0/2/4/5 as dead code
//     (no real call-site), and ADR-0001 already says re-adding an arity is cheap if a future
//     syscall needs it. Each wrapper moves the System V C-ABI argument registers into the raw
//     `syscall` instruction's own convention (rax=nr, rdi/rsi/rdx/r10/r8/r9=args) and returns
//     the kernel's raw value in rax -- callers apply ADR-0002's `-errno` contract themselves
//     (these wrappers never inspect or translate the return value).
// PT: Wrappers genericos de syscall crua por aridade (ADR-0001). Uma funcao NASM leaf por
//     numero de args -- so existem as aridades 1, 3 e 6; a AUD-C0-2 podou syscall0/2/4/5 como
//     codigo morto (sem call-site real), e o ADR-0001 ja diz que readicionar uma aridade e
//     barato se uma syscall futura precisar. Cada wrapper move os registradores de argumento
//     da C-ABI System V pra convencao propria da instrucao `syscall` (rax=nr,
//     rdi/rsi/rdx/r10/r8/r9=args) e retorna o valor cru do kernel em rax -- quem chama aplica
//     o contrato `-errno` do ADR-0002 por conta propria (estes wrappers nunca inspecionam nem
//     traduzem o retorno).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

long syscall1(long nr, long a1);
long syscall3(long nr, long a1, long a2, long a3);
long syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6);
