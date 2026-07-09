// SPDX-License-Identifier: MPL-2.0
// EN: The fourth NAMED syscall wrapper (ADR-0001 -- "named, typed C helpers are thin
//     functions layered on top of syscall1/3/6"). Mirrors POSIX `mmap()`'s argument shape, but
//     is the RAW kernel wrapper, not the libc one -- this is the one genuinely dangerous
//     corner of ADR-0002's `-errno` contract in this codebase: on success the kernel returns
//     the mapped ADDRESS in `rax` (not 0, not a byte count); on failure it returns a negative
//     `-errno` in the SAME register, in the range `[-4095, -1]`. The raw Linux x86-64 ABI does
//     NOT use libc's `MAP_FAILED` sentinel `(void*)-1` -- that is a glibc-level convention
//     built on top of this same raw return, and we do not have glibc. Concretely: a real
//     mapped address can itself numerically equal `(void*)-1`'s neighbourhood only in
//     contrived scenarios that do not occur in practice on a sane 64-bit address space, so the
//     kernel's own convention -- "anything within 4095 of the max unsigned value is an
//     -errno" -- is unambiguous and is exactly what callers (E1's malloc) must check via
//     `(unsigned long)ret >= (unsigned long)-4095`, NOT via `ret == MAP_FAILED`. This helper
//     itself does NOT perform that check -- it is the pure, minimal wrapper; the interpretation
//     lives at the call site (src/alloc.c), same division of labour as sys_read/sys_write.
// PT: O quarto wrapper de syscall NOMEADO (ADR-0001 -- "helpers C nomeados e tipados sao
//     funcoes finas por cima" das syscall1/3/6). Espelha o formato de argumentos do `mmap()`
//     POSIX, mas é o wrapper CRU do kernel, não o da libc -- este é o canto genuinamente
//     perigoso do contrato `-errno` do ADR-0002 neste código-base: em sucesso o kernel retorna
//     o ENDEREÇO mapeado em `rax` (não 0, não uma contagem de bytes); em falha retorna um
//     `-errno` negativo no MESMO registrador, na faixa `[-4095, -1]`. A ABI crua Linux x86-64
//     NÃO usa a sentinela `MAP_FAILED` `(void*)-1` da libc -- isso é uma convenção de nível
//     glibc construída em cima deste mesmo retorno cru, e não temos glibc. Concretamente: um
//     endereço mapeado de verdade só coincidiria numericamente com a vizinhança de
//     `(void*)-1` em cenários forçados que não ocorrem na prática num espaço de endereços de
//     64 bits são, então a convenção do próprio kernel -- "qualquer coisa a até 4095 do maior
//     valor sem sinal é um -errno" -- é inambígua e é exatamente o que quem chama (o malloc do
//     E1) precisa checar via `(unsigned long)ret >= (unsigned long)-4095`, NÃO via
//     `ret == MAP_FAILED`. Este helper em si NÃO faz essa checagem -- é o wrapper puro e
//     mínimo; a interpretação mora no ponto de chamada (src/alloc.c), a mesma divisão de
//     trabalho do sys_read/sys_write.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

void* sys_mmap(void* addr, size_t len, int prot, int flags, int fd, long off);
