// SPDX-License-Identifier: MPL-2.0
// EN: The first NAMED syscall wrapper (ADR-0001 -- "named, typed C helpers are thin
//     functions layered on top of syscall0..6"). Mirrors POSIX _exit(): terminates the
//     calling process/thread immediately, never returns. No error contract (ADR-0002)
//     applies -- there is no failure mode to propagate, the syscall does not return at all.
// PT: O primeiro wrapper de syscall NOMEADO (ADR-0001 -- "helpers C nomeados e tipados sao
//     funcoes finas por cima" das syscall0..6). Espelha o _exit() POSIX: encerra o
//     processo/thread chamador imediatamente, nunca retorna. Nenhum contrato de erro
//     (ADR-0002) se aplica -- nao ha modo de falha a propagar, a syscall simplesmente nao
//     retorna.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

void sys_exit(int code);
