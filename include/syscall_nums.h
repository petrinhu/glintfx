// SPDX-License-Identifier: MPL-2.0
// EN: x86-64 Linux syscall numbers used by this increment. Grows incrementally with each
//     new wave (see TODO.md) -- intentionally NOT a full syscall table today (YAGNI): only
//     what B5's sys_exit needs. B6 (sys_write) appends SYS_write here, and so on. Mirrored in
//     syscall_nums.inc (NASM) -- kept in sync by hand; revisit generation only if this drifts
//     into a real maintenance burden (one constant today does not justify tooling).
// PT: Números de syscall x86-64 Linux usados por este incremento. Cresce incrementalmente a
//     cada nova onda (ver TODO.md) -- propositalmente NÃO é uma tabela completa de syscalls
//     hoje (YAGNI): só o que o sys_exit da B5 precisa. A B6 (sys_write) acrescenta SYS_write
//     aqui, e assim por diante. Espelhado em syscall_nums.inc (NASM) -- mantido em sincronia
//     manualmente; revisitar geração automática só se isso virar dor real de manutenção (uma
//     constante hoje não justifica tooling).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#define SYS_exit 60
