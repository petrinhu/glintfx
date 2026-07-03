// SPDX-License-Identifier: MPL-2.0
// EN: x86-64 Linux syscall numbers used by this increment. Grows incrementally with each
//     new wave (see TODO.md) -- intentionally NOT a full syscall table today (YAGNI): only
//     what B5's sys_exit, B6's sys_write, and B7's sys_read need. Mirrored in
//     syscall_nums.inc (NASM) -- kept in sync by hand; revisit generation only if this drifts
//     into a real maintenance burden (a couple of constants today does not justify tooling;
//     see TODO.md INBOX watch-item on drift between the two copies, planned to gain an
//     automated checker at TST-STATIC/W11).
// PT: Números de syscall x86-64 Linux usados por este incremento. Cresce incrementalmente a
//     cada nova onda (ver TODO.md) -- propositalmente NÃO é uma tabela completa de syscalls
//     hoje (YAGNI): só o que o sys_exit da B5, o sys_write da B6, e o sys_read da B7
//     precisam. Espelhado em syscall_nums.inc (NASM) -- mantido em sincronia manualmente;
//     revisitar geração automática só se isso virar dor real de manutenção (um par de
//     constantes hoje não justifica tooling; ver watch-item da INBOX no TODO.md sobre drift
//     entre as 2 cópias, planejado pra ganhar um checker automatico na TST-STATIC/W11).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#define SYS_read 0
#define SYS_write 1
#define SYS_exit 60
