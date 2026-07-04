// SPDX-License-Identifier: MPL-2.0
// EN: D3 -- our own freestanding <limits.h>-equivalent, minimal (only what D3 needs: `int`/
//     `unsigned` extremes for x86-64 Linux LP64, where `int` is 4 bytes regardless of the LP64
//     data model). No libc `<limits.h>` exists to pull this from.
//
//     `INT_MIN` is written as `(-INT_MAX - 1)`, NOT the literal `-2147483648` -- the classic C
//     footgun: `2147483648` alone overflows `int` (max positive `int` is 2147483647), so the
//     unary `-` would apply to an already-overflowed/promoted value. `-INT_MAX - 1` computes the
//     same bit pattern (0x80000000) with no intermediate value ever exceeding `INT_MAX`.
// PT: D3 -- nosso proprio equivalente freestanding a `<limits.h>`, minimo (so o que o D3
//     precisa: extremos de `int`/`unsigned` pra x86-64 Linux LP64, onde `int` tem 4 bytes
//     independente do modelo de dados LP64). Nao existe `<limits.h>` de libc de onde puxar isso.
//
//     `INT_MIN` e' escrito como `(-INT_MAX - 1)`, NAO o literal `-2147483648` -- o footgun
//     classico de C: `2147483648` sozinho estoura `int` (o maximo positivo de `int` e'
//     2147483647), entao o `-` unario se aplicaria a um valor ja estourado/promovido.
//     `-INT_MAX - 1` computa o mesmo padrao de bits (0x80000000) sem nenhum valor intermediario
//     jamais exceder `INT_MAX`.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#define INT_MAX  2147483647
#define INT_MIN  (-INT_MAX - 1)
#define UINT_MAX 4294967295u
