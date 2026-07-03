// SPDX-License-Identifier: MPL-2.0
// EN: D1 -- the sovereign runtime's own memory primitives (no libc, no `<string.h>`). Four
//     functions with EXACT C-standard `<string.h>` semantics (`memcpy`/`memset`/`memmove`/
//     `memcmp`) but a from-scratch implementation living in src/mem.c -- this is Layer 0
//     rebuilding the piece of libc every C program takes for granted. Named `mem.h` (not
//     `string.h`): this project has no ambition to shadow the *name* of a libc header, only
//     to replace what it provides; D2 (tests/TODO.md, W8) will add a separate `str.h` for the
//     `strlen`/`strcmp`/... family once it exists.
//
//     Why these four, together, first (D1, W7): every other Layer-0 primitive that touches
//     raw bytes -- the future allocator (E1), `mini-printf` (E2), string ops (D2) -- needs at
//     least one of these underneath. They are the bedrock, hence first in the libc-core wave.
//
//     `-fno-builtin` gotcha (documented at length in src/mem.c): clang's LOOP-IDIOM
//     RECOGNITION can rewrite a hand-written byte-copy loop into a call to the very libc
//     function we do not have -- including a self-call inside `memcpy` itself. The Makefile
//     already carries `-fno-builtin` (and the `-fno-builtin-*` family via `-fno-builtin`),
//     verified empirically for this file via `nm`/`objdump` (see task report) -- no source
//     workaround (`__attribute__((optnone))` etc.) was needed.
// PT: D1 -- as primitivas de memoria proprias da runtime soberana (sem libc, sem
//     `<string.h>`). Quatro funcoes com a semantica EXATA do `<string.h>` do padrao C
//     (`memcpy`/`memset`/`memmove`/`memcmp`) mas implementacao do zero morando em src/mem.c --
//     e' a Camada 0 reconstruindo o pedaco da libc que todo programa C da por garantido.
//     Nomeado `mem.h` (nao `string.h`): este projeto nao tem ambicao de sombrear o *nome* de
//     um header de libc, so' de substituir o que ele fornece; o D2 (tests/TODO.md, W8) vai
//     acrescentar um `str.h` separado pra familia `strlen`/`strcmp`/... quando existir.
//
//     Por que essas quatro, juntas, primeiro (D1, W7): toda outra primitiva da Camada 0 que
//     toca bytes crus -- o futuro alocador (E1), o `mini-printf` (E2), as operacoes de string
//     (D2) -- precisa de pelo menos uma destas por baixo. Sao o alicerce, por isso primeiro na
//     onda libc-nucleo.
//
//     Gotcha `-fno-builtin` (documentado em detalhe em src/mem.c): o LOOP-IDIOM RECOGNITION
//     do clang pode reescrever um laco de copia de byte escrito a mao numa chamada pra propria
//     funcao de libc que nao temos -- inclusive uma auto-chamada dentro do proprio `memcpy`. O
//     Makefile ja carrega `-fno-builtin` (e a familia `-fno-builtin-*` via `-fno-builtin`),
//     verificado empiricamente pra este arquivo via `nm`/`objdump` (ver relatorio da tarefa) --
//     nenhum contorno no fonte (`__attribute__((optnone))` etc.) foi necessario.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

// EN: Copies `n` bytes from `src` to `dst`. Behaviour is UNDEFINED if the two regions overlap
//     (use memmove for that case). Returns `dst`.
// PT: Copia `n` bytes de `src` pra `dst`. Comportamento e' INDEFINIDO se as duas regioes se
//     sobrepoem (use memmove nesse caso). Retorna `dst`.
void* memcpy(void* dst, const void* src, size_t n);

// EN: Fills the first `n` bytes of `dst` with `(unsigned char)c`. Returns `dst`.
// PT: Preenche os primeiros `n` bytes de `dst` com `(unsigned char)c`. Retorna `dst`.
void* memset(void* dst, int c, size_t n);

// EN: Copies `n` bytes from `src` to `dst`, correctly handling OVERLAPPING regions (copies
//     forward or backward as needed so the source is fully read before being overwritten).
//     Returns `dst`.
// PT: Copia `n` bytes de `src` pra `dst`, tratando corretamente regioes que se SOBREPOEM
//     (copia pra frente ou pra tras conforme necessario pra ler toda a fonte antes dela ser
//     sobrescrita). Retorna `dst`.
void* memmove(void* dst, const void* src, size_t n);

// EN: Compares the first `n` bytes of `a` and `b` as `unsigned char`. Returns 0 if equal,
//     a negative value if the first differing byte in `a` is less than in `b`, positive
//     otherwise. Returns 0 if `n == 0` (no bytes compared).
// PT: Compara os primeiros `n` bytes de `a` e `b` como `unsigned char`. Retorna 0 se iguais,
//     um valor negativo se o primeiro byte diferente em `a` for menor que em `b`, positivo
//     caso contrario. Retorna 0 se `n == 0` (nenhum byte comparado).
int memcmp(const void* a, const void* b, size_t n);
