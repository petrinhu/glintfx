// SPDX-License-Identifier: MPL-2.0
// EN: D2 -- the sovereign runtime's own C-string primitives (no libc, no `<string.h>`). Seven
//     functions with EXACT C-standard `<string.h>` semantics (`strlen`/`strcmp`/`strncmp`/
//     `strcpy`/`strncpy`/`strcat`/`strchr`) but a from-scratch implementation living in
//     src/str.c -- this is Layer 0 rebuilding the NUL-terminated-string half of libc, the other
//     half D1 (mem.h) already rebuilt for raw byte buffers. Named `str.h` (not `string.h`), same
//     reasoning as D1's `mem.h`: no ambition to shadow the *name* of a libc header, only to
//     replace what it provides.
//
//     Every function here assumes its `const char*` inputs are NUL-terminated (the C-string
//     contract) -- unlike D1's mem.h family, which takes an explicit `n` and never inspects
//     byte content to find an end. Some implementations in src/str.c REUSE D1's memcpy/memset
//     (our own, `#include "mem.h"`) where that is the clearest way to express the byte-moving
//     part of the semantics (documented per-function in src/str.c) -- this is still zero libc:
//     memcpy/memset here always resolve to OUR OWN symbols in src/mem.c, never a libc one.
//
//     `strncpy`'s GOTCHA (the classic C footgun, preserved on purpose -- this project's
//     mandate is to replace libc.h behaviour EXACTLY, warts included, not to "fix" it): if
//     `strlen(src) < n`, the entire REMAINDER of `dst` up to `n` bytes is zero-padded (not just
//     one terminating NUL); if `strlen(src) >= n`, exactly `n` bytes are copied and dst is
//     NOT NUL-terminated at all. Callers of `strncpy` in THIS codebase must account for that
//     themselves, exactly as libc callers always had to.
// PT: D2 -- as primitivas de C-string proprias da runtime soberana (sem libc, sem
//     `<string.h>`). Sete funcoes com a semantica EXATA do `<string.h>` do padrao C
//     (`strlen`/`strcmp`/`strncmp`/`strcpy`/`strncpy`/`strcat`/`strchr`) mas implementacao do
//     zero morando em src/str.c -- e' a Camada 0 reconstruindo a metade "string terminada em
//     NUL" da libc, a outra metade (D1, mem.h) ja reconstruida pra buffers de byte crus.
//     Nomeado `str.h` (nao `string.h`), mesma logica do `mem.h` do D1: sem ambicao de sombrear
//     o *nome* de um header de libc, so' de substituir o que ele fornece.
//
//     Toda funcao aqui assume que seus inputs `const char*` sao terminados em NUL (o contrato
//     de C-string) -- diferente da familia mem.h do D1, que recebe um `n` explicito e nunca
//     inspeciona o conteudo de byte pra achar um fim. Algumas implementacoes em src/str.c
//     REUSAM o memcpy/memset do D1 (o NOSSO, `#include "mem.h"`) onde isso e' o jeito mais
//     claro de expressar a parte de mover-bytes da semantica (documentado por-funcao em
//     src/str.c) -- ainda e' zero libc: memcpy/memset aqui sempre resolvem pros NOSSOS proprios
//     simbolos em src/mem.c, nunca um de libc.
//
//     O GOTCHA do `strncpy` (o footgun classico de C, preservado de proposito -- o mandato
//     deste projeto e' substituir o comportamento da libc EXATAMENTE, verrugas inclusas, nao
//     "consertar" ele): se `strlen(src) < n`, o RESTANTE inteiro de `dst` ate `n` bytes e'
//     preenchido com zero (nao so um NUL terminador); se `strlen(src) >= n`, exatamente `n`
//     bytes sao copiados e dst NAO fica terminado em NUL de jeito nenhum. Chamadores de
//     `strncpy` NESTE codebase precisam se virar com isso, exatamente como chamadores de libc
//     sempre tiveram que.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

// EN: Number of bytes before the terminating `\0`, NOT counting it. `""` -> 0.
// PT: Numero de bytes antes do `\0` terminador, NAO contando ele. `""` -> 0.
size_t strlen(const char* s);

// EN: Lexicographic compare of two NUL-terminated strings, byte-by-byte as `unsigned char`.
//     Returns 0 if equal, negative if the first differing byte in `a` is less than in `b`
//     (or `a` is a strict prefix of `b`, since `\0` < any other byte), positive otherwise.
// PT: Compara lexicograficamente duas strings terminadas em NUL, byte-a-byte como
//     `unsigned char`. Retorna 0 se iguais, negativo se o primeiro byte diferente em `a` for
//     menor que em `b` (ou `a` for prefixo estrito de `b`, ja que `\0` < qualquer outro byte),
//     positivo caso contrario.
int strcmp(const char* a, const char* b);

// EN: Like strcmp but compares AT MOST `n` bytes; a difference beyond byte `n` does not count.
//     `n == 0` -> 0 (nothing compared).
// PT: Como strcmp mas compara NO MAXIMO `n` bytes; uma diferenca alem do byte `n` nao conta.
//     `n == 0` -> 0 (nada comparado).
int strncmp(const char* a, const char* b, size_t n);

// EN: Copies `src` into `dst`, INCLUDING the terminating `\0`. Returns `dst`. Caller guarantees
//     `dst` has room for `strlen(src) + 1` bytes.
// PT: Copia `src` pra `dst`, INCLUINDO o `\0` terminador. Retorna `dst`. O chamador garante que
//     `dst` tem espaco pra `strlen(src) + 1` bytes.
char* strcpy(char* dst, const char* src);

// EN: Copies at most `n` bytes from `src` into `dst`. If `strlen(src) < n`, the rest of `dst`
//     up to `n` bytes is zero-padded. If `strlen(src) >= n`, exactly `n` bytes are copied and
//     `dst` is NOT NUL-terminated. Returns `dst`. See file header for the full gotcha.
// PT: Copia no maximo `n` bytes de `src` pra `dst`. Se `strlen(src) < n`, o resto de `dst` ate
//     `n` bytes e' preenchido com zero. Se `strlen(src) >= n`, exatamente `n` bytes sao
//     copiados e `dst` NAO fica terminado em NUL. Retorna `dst`. Ver cabecalho do arquivo pro
//     gotcha completo.
char* strncpy(char* dst, const char* src, size_t n);

// EN: Finds the terminating `\0` of `dst` and appends `src` (including its own `\0`) starting
//     there. Returns `dst`. Caller guarantees `dst` has room for `strlen(dst) + strlen(src) + 1`
//     bytes.
// PT: Acha o `\0` terminador de `dst` e anexa `src` (incluindo o proprio `\0`) a partir dali.
//     Retorna `dst`. O chamador garante que `dst` tem espaco pra
//     `strlen(dst) + strlen(src) + 1` bytes.
char* strcat(char* dst, const char* src);

// EN: First occurrence of `(char)c` in `s`. Returns a pointer to it, or NULL if not found.
//     `c == '\0'` matches the terminator itself (returns a pointer to it, never NULL) -- the
//     standard C-string semantics: the terminating `\0` counts as part of the string for this
//     search.
// PT: Primeira ocorrencia de `(char)c` em `s`. Retorna um ponteiro pra ela, ou NULL se nao
//     achar. `c == '\0'` casa com o proprio terminador (retorna ponteiro pra ele, nunca NULL)
//     -- a semantica padrao de C-string: o `\0` terminador conta como parte da string pra essa
//     busca.
char* strchr(const char* s, int c);
