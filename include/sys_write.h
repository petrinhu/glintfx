// SPDX-License-Identifier: MPL-2.0
// EN: The second NAMED syscall wrapper (ADR-0001 -- "named, typed C helpers are thin
//     functions layered on top of syscall1/3/6"). Mirrors POSIX write(): writes up to `count`
//     bytes from `buf` to file descriptor `fd`. Returns the raw kernel value in `rax`: on
//     success, the number of bytes actually written (may be less than `count` -- short writes
//     are a real possibility even on Linux, this helper does NOT loop/retry); on failure, a
//     negative `-errno` (ADR-0002) -- the caller decides whether/how to interpret it. This is
//     the pure, minimal helper: no libc `write(3)` semantics (no `errno` global, no buffering).
// PT: O segundo wrapper de syscall NOMEADO (ADR-0001 -- "helpers C nomeados e tipados sao
//     funcoes finas por cima" das syscall1/3/6). Espelha o write() POSIX: escreve ate `count`
//     bytes de `buf` no descritor de arquivo `fd`. Retorna o valor cru do kernel em `rax`: em
//     sucesso, o numero de bytes de fato escritos (pode ser menor que `count` -- short writes
//     sao uma possibilidade real mesmo no Linux, este helper NAO faz loop/retry); em falha, um
//     `-errno` negativo (ADR-0002) -- quem chama decide se/como interpretar. Este e' o helper
//     puro e minimo: sem a semantica do write(3) da libc (sem `errno` global, sem buffering).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

ssize_t sys_write(int fd, const void* buf, size_t count);
