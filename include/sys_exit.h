// SPDX-License-Identifier: MPL-2.0
// EN: The first NAMED syscall wrapper (ADR-0001 -- "named, typed C helpers are thin
//     functions layered on top of syscall0..6"). Mirrors POSIX _exit(): terminates the
//     calling process/thread immediately, never returns. No error contract (ADR-0002)
//     applies -- there is no failure mode to propagate, the syscall does not return at all.
//     Declared `_Noreturn` (C23 keyword): lets the compiler prove callers never fall through
//     after calling it (e.g. the B4 _start fallback path, or a bare `sys_exit(code);` with no
//     trailing statement) and enables dead-code diagnostics/optimizations downstream.
// PT: O primeiro wrapper de syscall NOMEADO (ADR-0001 -- "helpers C nomeados e tipados sao
//     funcoes finas por cima" das syscall0..6). Espelha o _exit() POSIX: encerra o
//     processo/thread chamador imediatamente, nunca retorna. Nenhum contrato de erro
//     (ADR-0002) se aplica -- nao ha modo de falha a propagar, a syscall simplesmente nao
//     retorna. Declarado `_Noreturn` (palavra-chave C23): permite ao compilador provar que
//     quem chama nunca cai adiante depois de chamar (ex.: o caminho de fallback do _start da
//     B4, ou um `sys_exit(code);` isolado sem statement seguinte) e habilita
//     diagnosticos/otimizacoes de codigo-morto downstream.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

_Noreturn void sys_exit(int code);
