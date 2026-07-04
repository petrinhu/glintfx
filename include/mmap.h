// SPDX-License-Identifier: MPL-2.0
// EN: `mmap(2)` flag constants for x86-64 Linux, used by E1's allocator (the only caller
//     today). Own header, NOT folded into syscall_nums.h: syscall_nums.h holds syscall
//     NUMBERS (the `rax` value); these are syscall ARGUMENT bits (`rdx`/`r10` values) --
//     a different axis, kept separate on purpose (YAGNI: fold them later only if a second
//     mmap-adjacent caller shows an actual need to share more surface). Values verified
//     read-only (NOT linked) against this machine's kernel headers
//     (`/usr/include/asm-generic/mman-common.h` for `PROT_*`/`MAP_ANONYMOUS`,
//     `/usr/include/linux/mman.h` for `MAP_PRIVATE`) -- only the 4 flags E1 actually uses are
//     reproduced here (YAGNI, not a full mman.h port).
// PT: Constantes de flag do `mmap(2)` pra x86-64 Linux, usadas pelo alocador do E1 (o único
//     chamador hoje). Header próprio, NÃO dobrado dentro de syscall_nums.h: syscall_nums.h
//     guarda NÚMEROS de syscall (o valor de `rax`); estas são bits de ARGUMENTO da syscall
//     (valores de `rdx`/`r10`) -- um eixo diferente, mantido separado de propósito (YAGNI:
//     dobrar depois só se um segundo chamador mmap-adjacente mostrar necessidade real de
//     compartilhar mais superfície). Valores conferidos em modo leitura (NÃO linkados) contra
//     os headers do kernel desta máquina (`/usr/include/asm-generic/mman-common.h` pros
//     `PROT_*`/`MAP_ANONYMOUS`, `/usr/include/linux/mman.h` pro `MAP_PRIVATE`) -- só as 4
//     flags que o E1 de fato usa estão reproduzidas aqui (YAGNI, não é um port completo do
//     mman.h).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Memory-protection bits (the `prot` argument).
// PT: Bits de proteção de memória (o argumento `prot`).
#define PROT_READ  0x1
#define PROT_WRITE 0x2

// EN: Mapping-behaviour bits (the `flags` argument). `MAP_PRIVATE` (0x2, from
//     linux/mman.h -- NOT asm-generic/mman-common.h, which only defines `MAP_SHARED`=0x1 and
//     `MAP_SHARED_VALIDATE`=0x3 in that low range) means copy-on-write, not shared with any
//     other process; `MAP_ANONYMOUS` (0x20) means the mapping is backed by zeroed RAM, not a
//     file (so `fd` and `offset` are ignored by the kernel and conventionally passed as -1/0).
// PT: Bits de comportamento do mapeamento (o argumento `flags`). `MAP_PRIVATE` (0x2, de
//     linux/mman.h -- NÃO de asm-generic/mman-common.h, que só define `MAP_SHARED`=0x1 e
//     `MAP_SHARED_VALIDATE`=0x3 nessa faixa baixa) significa copy-on-write, não compartilhado
//     com nenhum outro processo; `MAP_ANONYMOUS` (0x20) significa que o mapeamento é
//     lastreado por RAM zerada, não um arquivo (então `fd` e `offset` são ignorados pelo
//     kernel e passados por convenção como -1/0).
#define MAP_PRIVATE   0x2
#define MAP_ANONYMOUS 0x20
