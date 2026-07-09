// SPDX-License-Identifier: MPL-2.0
// EN: The third NAMED syscall wrapper (ADR-0001 -- "named, typed C helpers are thin
//     functions layered on top of syscall1/3/6"). Mirrors POSIX read(): reads up to `count`
//     bytes from file descriptor `fd` into `buf`. Returns the raw kernel value in `rax`: on
//     success, the number of bytes actually read (may be less than `count` -- short reads are
//     a real possibility, this helper does NOT loop/retry to fill the buffer); `0` means EOF
//     (no more data, fd still valid); on failure, a negative `-errno` (ADR-0002) -- the caller
//     decides whether/how to interpret it. `buf` is deliberately `void*` (non-const, unlike
//     sys_write's `const void*`): the kernel writes INTO this buffer. This is the pure,
//     minimal helper: no libc `read(3)` semantics (no `errno` global, no buffering, no
//     retry-on-EINTR).
// PT: O terceiro wrapper de syscall NOMEADO (ADR-0001 -- "helpers C nomeados e tipados sao
//     funcoes finas por cima" das syscall1/3/6). Espelha o read() POSIX: le ate `count` bytes
//     do descritor de arquivo `fd` para dentro de `buf`. Retorna o valor cru do kernel em
//     `rax`: em sucesso, o numero de bytes de fato lidos (pode ser menor que `count` -- short
//     reads sao uma possibilidade real, este helper NAO faz loop/retry para preencher o
//     buffer); `0` significa EOF (sem mais dados, fd ainda valido); em falha, um `-errno`
//     negativo (ADR-0002) -- quem chama decide se/como interpretar. `buf` e' deliberadamente
//     `void*` (nao-const, diferente do `const void*` do sys_write): o kernel escreve DENTRO
//     deste buffer. Este e' o helper puro e minimo: sem a semantica do read(3) da libc (sem
//     `errno` global, sem buffering, sem retry-on-EINTR).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

ssize_t sys_read(int fd, void* buf, size_t count);
