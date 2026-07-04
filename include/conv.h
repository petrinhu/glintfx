// SPDX-License-Identifier: MPL-2.0
// EN: D3 -- the sovereign runtime's own int<->string conversions (no libc, no `<stdlib.h>`).
//     Four functions: `utoa`/`itoa` (integer -> string) and `atoi`/`atou` (string -> integer).
//     This is the piece that unlocks printing numbers at all -- E2 (mini-printf, TODO.md, W10)
//     is built entirely on top of `itoa`/`utoa`. Named `conv.h` (not `stdlib.h`): same
//     "replace what it provides, not the header name" convention as D1's `mem.h`/D2's `str.h`.
//
//     YAGNI on bases: only base 10 and base 16 are supported (no base 2/8/arbitrary-base) --
//     nothing in this codebase needs more yet; extend when a real need appears.
//
//     Buffer sizing is entirely the CALLER's responsibility (these functions never allocate,
//     per this project's "no malloc in hot path" stance carried down even to freestanding
//     Layer 0) -- use the `CONV_*_BUF_MIN` constants below to size stack buffers safely:
//       - `CONV_ITOA_BUF_MIN` (12): worst case is `itoa(INT_MIN, ..., 10)` ->
//         "-2147483648" (11 chars) + '\0' = 12 bytes. Covers itoa in EITHER base (base-16
//         output is always shorter, max 8 hex digits + '\0' = 9).
//       - `CONV_UTOA_BUF_MIN` (11): worst case is `utoa(UINT_MAX, ..., 10)` ->
//         "4294967295" (10 chars) + '\0' = 11 bytes. Covers utoa in EITHER base (base-16
//         output is at most 8 hex digits + '\0' = 9).
//     Base-2 output is NOT supported (see above) so no 32/33-byte binary buffer size is
//     defined here.
// PT: D3 -- as conversoes int<->string proprias da runtime soberana (sem libc, sem
//     `<stdlib.h>`). Quatro funcoes: `utoa`/`itoa` (inteiro -> string) e `atoi`/`atou`
//     (string -> inteiro). E' a peca que destrava imprimir numeros de vez -- o E2
//     (mini-printf, TODO.md, W10) e' construido inteiramente em cima de `itoa`/`utoa`.
//     Nomeado `conv.h` (nao `stdlib.h`): mesma convencao "substitui o que fornece, nao o nome
//     do header" do `mem.h` do D1/`str.h` do D2.
//
//     YAGNI nas bases: so base 10 e base 16 sao suportadas (sem base 2/8/base-arbitraria) --
//     nada neste codebase precisa de mais por enquanto; estender quando uma necessidade real
//     aparecer.
//
//     O dimensionamento do buffer e' inteiramente responsabilidade do CHAMADOR (essas funcoes
//     nunca alocam, seguindo a postura deste projeto de "sem malloc em hot path" carregada ate
//     a Camada 0 freestanding) -- use as constantes `CONV_*_BUF_MIN` abaixo pra dimensionar
//     buffers de pilha com seguranca:
//       - `CONV_ITOA_BUF_MIN` (12): o pior caso e' `itoa(INT_MIN, ..., 10)` ->
//         "-2147483648" (11 chars) + '\0' = 12 bytes. Cobre itoa em QUALQUER base (a saida em
//         base-16 e' sempre mais curta, no maximo 8 digitos hex + '\0' = 9).
//       - `CONV_UTOA_BUF_MIN` (11): o pior caso e' `utoa(UINT_MAX, ..., 10)` ->
//         "4294967295" (10 chars) + '\0' = 11 bytes. Cobre utoa em QUALQUER base (a saida em
//         base-16 e' no maximo 8 digitos hex + '\0' = 9).
//     Saida em base-2 NAO e' suportada (ver acima) entao nenhum tamanho de buffer binario de
//     32/33 bytes e' definido aqui.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"

#define CONV_ITOA_BUF_MIN 12
#define CONV_UTOA_BUF_MIN 11

// EN: Converts `value` (unsigned) to a NUL-terminated string in `buf`, in the given `base`.
//     `base` must be 10 or 16; any other value is treated as 10 (documented fallback, not an
//     error -- there is no libc `errno`/abort mechanism to report one). Base 16 uses LOWERCASE
//     digits ('0'-'9','a'-'f'), no "0x" prefix, no sign (the value is already unsigned).
//     `utoa(0, buf, <any base>)` -> "0". Returns `buf`. Caller-owned buffer, see
//     `CONV_UTOA_BUF_MIN` above for the minimum safe size.
// PT: Converte `value` (sem sinal) numa string terminada em NUL em `buf`, na `base` dada.
//     `base` deve ser 10 ou 16; qualquer outro valor e' tratado como 10 (fallback documentado,
//     nao um erro -- nao ha mecanismo de `errno`/abort de libc pra reportar um). Base 16 usa
//     digitos MINUSCULOS ('0'-'9','a'-'f'), sem prefixo "0x", sem sinal (o valor ja e' sem
//     sinal). `utoa(0, buf, <base qualquer>)` -> "0". Retorna `buf`. Buffer de posse do
//     chamador, ver `CONV_UTOA_BUF_MIN` acima pro tamanho minimo seguro.
char* utoa(unsigned value, char* buf, int base);

// EN: Converts `value` (signed) to a NUL-terminated string in `buf`, in the given `base`.
//     `base` must be 10 or 16 (same fallback-to-10 rule as `utoa` for any other value).
//     Base 10: SIGNED decimal -- a leading '-' is written for negative values, followed by the
//     magnitude; positive values have no leading '+'. Base 16: `value` is reinterpreted as its
//     raw 32-bit UNSIGNED bit pattern (two's complement) and printed WITHOUT a sign -- e.g.
//     `itoa(-1, buf, 16)` -> "ffffffff" (useful for dumping a register/word in hex, matching
//     how `objdump`/`gdb` show raw words). Returns `buf`. Caller-owned buffer, see
//     `CONV_ITOA_BUF_MIN` above for the minimum safe size (covers `itoa(INT_MIN, ..., 10)`,
//     the longest possible output in either base).
// PT: Converte `value` (com sinal) numa string terminada em NUL em `buf`, na `base` dada.
//     `base` deve ser 10 ou 16 (mesma regra de fallback-pra-10 do `utoa` pra qualquer outro
//     valor). Base 10: decimal COM SINAL -- um '-' inicial e' escrito pra valores negativos,
//     seguido da magnitude; valores positivos nao tem '+' inicial. Base 16: `value` e'
//     reinterpretado como seu padrao de bits UNSIGNED de 32 bits cru (complemento de dois) e
//     impresso SEM sinal -- ex.: `itoa(-1, buf, 16)` -> "ffffffff" (util pra despejar um
//     registrador/palavra em hex, batendo com como `objdump`/`gdb` mostram palavras cruas).
//     Retorna `buf`. Buffer de posse do chamador, ver `CONV_ITOA_BUF_MIN` acima pro tamanho
//     minimo seguro (cobre `itoa(INT_MIN, ..., 10)`, a maior saida possivel em qualquer base).
char* itoa(int value, char* buf, int base);

// EN: Parses a base-10 SIGNED integer out of the front of `s`. Skips leading whitespace
//     (' ', '\t', '\n', '\v', '\f', '\r'), accepts an optional single leading '+' or '-', then
//     consumes decimal digits until the first non-digit (or end of string) -- exactly like C's
//     `atoi`. `"12abc"` -> 12 (trailing garbage ignored). `""` or a string with no digits at
//     all (after skipping whitespace/sign) -> 0. OVERFLOW is DEFINED here (unlike the C
//     standard's `atoi`, which leaves it undefined): the result SATURATES at `INT_MAX` /
//     `INT_MIN` rather than wrapping -- e.g. `"9999999999"` -> `INT_MAX`,
//     `"-9999999999"` -> `INT_MIN`.
// PT: Parseia um inteiro base-10 COM SINAL do inicio de `s`. Pula whitespace inicial
//     (' ', '\t', '\n', '\v', '\f', '\r'), aceita um '+' ou '-' inicial opcional unico, depois
//     consome digitos decimais ate o primeiro nao-digito (ou fim da string) -- exatamente como
//     o `atoi` de C. `"12abc"` -> 12 (lixo final ignorado). `""` ou uma string sem nenhum
//     digito (depois de pular whitespace/sinal) -> 0. OVERFLOW e' DEFINIDO aqui (diferente do
//     `atoi` do padrao C, que deixa indefinido): o resultado SATURA em `INT_MAX` / `INT_MIN`
//     em vez de dar wrap -- ex.: `"9999999999"` -> `INT_MAX`, `"-9999999999"` -> `INT_MIN`.
int atoi(const char* s);

// EN: Parses a base-10 UNSIGNED integer out of the front of `s`. Skips leading whitespace
//     (same set as `atoi`), accepts an optional single leading '+' (there is no negative
//     unsigned value to represent) -- a leading '-' is treated as NOT part of a valid number:
//     it is NOT consumed and the digit scan starts (and typically fails, if what follows isn't
//     a digit) from that same position, so `atou("-5")` -> 0 (design choice, documented here;
//     it does NOT silently negate/wrap like an unsigned cast of a negative value would).
//     Consumes decimal digits until the first non-digit (or end of string). `""` or no digits
//     -> 0. OVERFLOW SATURATES at `UINT_MAX` (e.g. `"99999999999"` -> `UINT_MAX`), same
//     defined-behaviour philosophy as `atoi`.
// PT: Parseia um inteiro base-10 SEM SINAL do inicio de `s`. Pula whitespace inicial (mesmo
//     conjunto do `atoi`), aceita um '+' inicial opcional unico (nao ha valor unsigned
//     negativo pra representar) -- um '-' inicial e' tratado como NAO fazendo parte de um
//     numero valido: NAO e' consumido e a varredura de digito comeca (e tipicamente falha, se
//     o que vem depois nao for digito) a partir dessa mesma posicao, entao `atou("-5")` -> 0
//     (escolha de design, documentada aqui; NAO nega/da wrap silenciosamente como um cast
//     unsigned de um valor negativo faria). Consome digitos decimais ate o primeiro nao-digito
//     (ou fim da string). `""` ou sem digitos -> 0. OVERFLOW SATURA em `UINT_MAX` (ex.:
//     `"99999999999"` -> `UINT_MAX`), mesma filosofia de comportamento-definido do `atoi`.
unsigned atou(const char* s);
