// SPDX-License-Identifier: MPL-2.0
// EN: Our own freestanding <stddef.h>/<stdint.h>-equivalent -- no libc header exists to pull
//     this from. `bool`/`true`/`false` are DELIBERATELY NOT defined here: under `-std=c23`
//     (this project's floor) they are language KEYWORDS -- C23 promoted them from
//     <stdbool.h> macros to reserved words. Redefining them (even via typedef) would collide
//     with the reserved keyword and very likely fail to compile. This is a real C23 behaviour
//     change, flagged explicitly in the implementation plan for review.
// PT: Nosso próprio equivalente freestanding a <stddef.h>/<stdint.h> -- não existe header de
//     libc de onde puxar isso. `bool`/`true`/`false` DELIBERADAMENTE NÃO são definidos aqui:
//     sob `-std=c23` (piso deste projeto) eles são PALAVRAS-CHAVE da linguagem -- o C23
//     promoveu de macros de <stdbool.h> pra palavra reservada. Redefini-los (mesmo via
//     typedef) colidiria com a palavra reservada e muito provavelmente falharia ao compilar.
//     É uma mudança de comportamento real do C23, sinalizada explicitamente no plano de
//     implementação para revisão.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: x86-64 Linux is LP64: `long`/`unsigned long` are 8 bytes, the same width as a pointer --
//     used directly as the backing type, no `__SIZE_TYPE__` compiler magic needed.
// PT: x86-64 Linux é LP64: `long`/`unsigned long` têm 8 bytes, a mesma largura de um
//     ponteiro -- usado diretamente como tipo de base, sem precisar de mágica de compilador
//     tipo `__SIZE_TYPE__`.
typedef unsigned long size_t;
typedef long           ssize_t;
typedef unsigned long uintptr_t;

#define NULL ((void*)0)
