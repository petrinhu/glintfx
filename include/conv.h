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

// ---- SOV-FCONV: float<->string (double only; caller-owned buffers, zero allocation) ----------

// EN: `ftoa`'s default fractional precision (6, matching C's default `%f` behaviour) and the
//     internal clamp on how many fractional digits a caller may request. `CONV_FTOA_MAX_INT_DIGITS`
//     is the worst-case DIGIT COUNT of the integer part of any finite `double`: `DBL_MAX` ~=
//     1.7976931348623157e308 has exactly 309 decimal digits before the point (`DBL_MAX < 10^309`
//     and `DBL_MAX >= 10^308`), and no finite double can have a larger integer part -- 309 is a
//     hard ceiling, not a guess. `+1` is a DEFENSIVE headroom slot for the generic rounding-carry
//     code path (`ftoa`'s carry-into-the-integer-part logic is shared/generic; the +1 is never
//     actually reachable for a legitimately finite double, since any carry that would need a
//     310th digit would itself exceed `DBL_MAX` and therefore cannot originate from a real finite
//     double in the first place -- kept anyway, cheap stack space, in case that reasoning is ever
//     wrong). These two constants derive `CONV_FTOA_BUF_FOR(precision)`, the worst-case buffer
//     size for `ftoa(..., precision)`: `1` (optional '-') + `CONV_FTOA_MAX_INT_DIGITS` (integer
//     digits) + `1` ('.') + `precision` (fractional digits) + `1` ('\0'). `CONV_FTOA_BUF_MIN` is
//     that formula evaluated at the DEFAULT precision (6) -- `1+310+1+6+1 = 319` bytes -- and is
//     what most callers should use; callers requesting a non-default `precision` must size their
//     buffer with `CONV_FTOA_BUF_FOR(precision)` instead. Both formulas comfortably cover the
//     short "inf"/"-inf"/"nan" special-value outputs too (5 bytes max including '\0').
// PT: A precisao fracionaria PADRAO do `ftoa` (6, batendo com o comportamento padrao do `%f` de
//     C) e o teto interno de quantos digitos fracionarios um chamador pode pedir.
//     `CONV_FTOA_MAX_INT_DIGITS` e' a CONTAGEM DE DIGITOS de pior caso da parte inteira de
//     qualquer `double` finito: `DBL_MAX` ~= 1.7976931348623157e308 tem exatamente 309 digitos
//     decimais antes do ponto (`DBL_MAX < 10^309` e `DBL_MAX >= 10^308`), e nenhum double finito
//     pode ter uma parte inteira maior -- 309 e' um teto duro, nao um chute. O `+1` e' uma folga
//     DEFENSIVA pro caminho de codigo generico de carry-de-arredondamento (a logica de
//     carry-pra-dentro-da-parte-inteira do `ftoa` e' compartilhada/generica; o +1 nunca e' de fato
//     alcancavel por um double finito legitimo, ja que qualquer carry que precisasse de um 310o
//     digito ele mesmo excederia `DBL_MAX` e portanto nao poderia se originar de um double finito
//     real em primeiro lugar -- mantido assim mesmo, espaco de pilha barato, caso esse raciocinio
//     algum dia esteja errado). Essas duas constantes derivam `CONV_FTOA_BUF_FOR(precision)`, o
//     tamanho de buffer de pior caso pra `ftoa(..., precision)`: `1` ('-' opcional) +
//     `CONV_FTOA_MAX_INT_DIGITS` (digitos inteiros) + `1` ('.') + `precision` (digitos
//     fracionarios) + `1` ('\0'). `CONV_FTOA_BUF_MIN` e' essa formula avaliada na precisao PADRAO
//     (6) -- `1+310+1+6+1 = 319` bytes -- e e' o que a maioria dos chamadores deve usar; chamadores
//     que pedem uma `precision` nao-padrao devem dimensionar o buffer com
//     `CONV_FTOA_BUF_FOR(precision)` em vez disso. As duas formulas cobrem folgadamente tambem as
//     saidas curtas de valor especial "inf"/"-inf"/"nan" (5 bytes no maximo incluindo '\0').
#define CONV_FTOA_DEFAULT_PRECISION 6u
#define CONV_FTOA_MAX_PRECISION     32u
#define CONV_FTOA_MAX_INT_DIGITS    310u
#define CONV_FTOA_BUF_FOR(precision) \
    (1u + CONV_FTOA_MAX_INT_DIGITS + 1u + (unsigned)(precision) + 1u)
#define CONV_FTOA_BUF_MIN CONV_FTOA_BUF_FOR(CONV_FTOA_DEFAULT_PRECISION)

// EN: Formats `value` (a `double`) into `buf` as FIXED-point decimal with exactly `precision`
//     digits after the decimal point (rounded HALF-AWAY-FROM-ZERO -- e.g. `2.5`/`-2.5` at
//     precision 0 round to `"3"`/`"-3"`, never round-half-to-even/banker's-rounding). `precision`
//     is silently clamped to `CONV_FTOA_MAX_PRECISION` (32) if larger (same "no errno, documented
//     fallback" philosophy as `utoa`'s base fallback) -- 32 fractional digits already exceeds a
//     double's ~15-17 significant decimal digits of real precision by a wide margin, so the clamp
//     never discards meaningful information. `precision == 0` omits the decimal point entirely
//     (matches C's `%.0f`). Special values: `+0.0`/`-0.0` print `"0.000000"`/`"-0.000000"` (SIGN
//     PRESERVED -- this is why sign is read from the raw bit pattern, never from `value < 0.0`,
//     which is false for both signs of zero); `+Infinity`/`-Infinity` print `"inf"`/`"-inf"`;
//     any NaN prints `"nan"` (the NaN's own sign bit is NOT represented in the output -- a
//     documented simplification, unlike some `libc`s' `"-nan"`). Returns `buf`.
//
//     THIS IS NOT A SHORTEST-ROUND-TRIP / CORRECTLY-ROUNDED CONVERTER (no Ryu, no Grisu, no
//     Dragon4) -- it is a FIXED-precision digit-by-digit extractor (same "correctness and
//     auditability first" philosophy as D3's `itoa`/`utoa`): the integer part is extracted one
//     decimal digit at a time via repeated `trunc(x/10)`/`x - 10*trunc(x/10)` (mirroring `utoa`'s
//     least-significant-first-then-reverse shape, just in the `double` domain), and the
//     fractional part is extracted one digit at a time via repeated `x *= 10; digit = trunc(x)`.
//     For large-magnitude values (many integer digits) this accumulates ordinary floating-point
//     rounding error across the digit-extraction loop -- the output is a faithful, useful decimal
//     rendering, but is NOT guaranteed to be the exact/nearest decimal string for every possible
//     `double` (this is the explicit tradeoff this task brief calls for: "precisão FIXA
//     documentada... NÃO é shortest-round-trip"). `trunc`/`isfinite` classification are our OWN
//     hand-rolled bit-pattern logic (IEEE-754 binary64 layout assumed -- guaranteed by the x86-64
//     System V ABI this whole project targets), never a libm/compiler-builtin call: every
//     arithmetic step here (`+ - * /` on `double`) compiles to native SSE2 instructions
//     (`addsd`/`subsd`/`mulsd`/`divsd`/`cvtsi2sd`/`cvttsd2si`) on x86-64 -- ZERO external symbols,
//     verified via `nm -u` on the test binaries (see task report).
//
//     KNOWN EDGE BEHAVIOUR AT THE VERY TOP OF THE `double` RANGE (empirically verified, not
//     hypothetical): for values in roughly the TOP DECADE below `DBL_MAX` (~1e307 to
//     1.7976931348623157e308, `DBL_MAX` itself included), the compounding rounding error
//     described above can push the digit string's represented decimal value a hair ABOVE the
//     true value -- and because these inputs already sit right at the finite/`Infinity` boundary,
//     re-parsing that string with `atof` can legitimately OVERFLOW to `Infinity` instead of
//     recovering something close to the original finite `double`. This is a DETERMINISTIC,
//     understood consequence of the fixed-precision (non-arbitrary-precision) approach -- not
//     random, not a crash, not silent data corruption -- and it is the direct, disclosed price of
//     NOT implementing an arbitrary-precision/bignum-backed exact decimal expansion (which is
//     what real `libc`s use for `%f` and is explicitly out of scope here, see above). Values
//     further from the ceiling (tested up to ~1e300) round-trip with a tiny RELATIVE error
//     instead (a few parts in 1e15, i.e. a handful of `double` ULPs) and do NOT exhibit this
//     overflow. `ftoa` itself never crashes or produces a malformed string for any finite
//     `double`, including `DBL_MAX` -- only a SUBSEQUENT `atof` re-parse of a top-decade value's
//     string can overflow.
//
//     Buffer is entirely CALLER-OWNED (this function never allocates) -- size it with
//     `CONV_FTOA_BUF_MIN` (default precision) or `CONV_FTOA_BUF_FOR(precision)` (custom
//     precision); ftoa itself performs NO bounds checking against a caller-supplied capacity
//     (there is no `cap` parameter, mirroring `itoa`/`utoa`'s caller-sizing contract) -- an
//     under-sized buffer is a caller bug, exactly like `itoa`/`utoa`.
// PT: Formata `value` (um `double`) em `buf` como decimal de PONTO FIXO com exatamente
//     `precision` digitos depois do ponto decimal (arredondado METADE-PARA-LONGE-DE-ZERO -- ex.:
//     `2.5`/`-2.5` na precisao 0 arredondam pra `"3"`/`"-3"`, nunca
//     arredondamento-metade-pra-par/bankers-rounding). `precision` e' silenciosamente limitada a
//     `CONV_FTOA_MAX_PRECISION` (32) se maior (mesma filosofia "sem errno, fallback documentado"
//     do fallback de base do `utoa`) -- 32 digitos fracionarios ja excede os ~15-17 digitos
//     decimais significativos de precisao real de um double por uma margem larga, entao o limite
//     nunca descarta informacao significativa. `precision == 0` omite o ponto decimal por inteiro
//     (bate com o `%.0f` de C). Valores especiais: `+0.0`/`-0.0` imprimem
//     `"0.000000"`/`"-0.000000"` (SINAL PRESERVADO -- e' por isso que o sinal e' lido do padrao de
//     bits cru, nunca de `value < 0.0`, que e' falso pros dois sinais de zero);
//     `+Infinito`/`-Infinito` imprimem `"inf"`/`"-inf"`; qualquer NaN imprime `"nan"` (o proprio
//     bit de sinal do NaN NAO e' representado na saida -- simplificacao documentada, diferente do
//     `"-nan"` de algumas `libc`s). Retorna `buf`.
//
//     ISSO NAO E' UM CONVERSOR SHORTEST-ROUND-TRIP / CORRETAMENTE-ARREDONDADO (sem Ryu, sem
//     Grisu, sem Dragon4) -- e' um extrator digito-a-digito de precisao FIXA (mesma filosofia
//     "corretude e auditabilidade primeiro" do `itoa`/`utoa` do D3): a parte inteira e' extraida
//     um digito decimal por vez via `trunc(x/10)`/`x - 10*trunc(x/10)` repetido (espelhando a
//     forma menos-significativo-primeiro-depois-reverte do `utoa`, so' no dominio `double`), e a
//     parte fracionaria e' extraida um digito por vez via `x *= 10; digito = trunc(x)` repetido.
//     Pra valores de magnitude grande (muitos digitos inteiros) isso acumula erro de
//     arredondamento de ponto flutuante comum ao longo do laco de extracao de digito -- a saida e'
//     uma renderizacao decimal fiel e util, mas NAO e' garantida ser a string decimal
//     exata/mais-proxima pra todo `double` possivel (essa e' a troca explicita que o brief desta
//     tarefa pede: "precisao FIXA documentada... NAO e' shortest-round-trip"). A classificacao
//     `trunc`/`isfinite` e' logica PROPRIA nossa de padrao-de-bits (layout IEEE-754 binary64
//     assumido -- garantido pela ABI System V x86-64 que este projeto inteiro mira), nunca uma
//     chamada libm/builtin-de-compilador: todo passo aritmetico aqui (`+ - * /` em `double`)
//     compila pra instrucoes SSE2 nativas (`addsd`/`subsd`/`mulsd`/`divsd`/`cvtsi2sd`/`cvttsd2si`)
//     em x86-64 -- ZERO simbolos externos, verificado via `nm -u` nos binarios de teste (ver
//     relatorio da tarefa).
//
//     COMPORTAMENTO DE FRONTEIRA CONHECIDO no TOPO da faixa de `double` (verificado
//     empiricamente, nao hipotetico): pra valores na DECADA MAIS ALTA abaixo de `DBL_MAX`
//     (~1e307 a 1.7976931348623157e308, `DBL_MAX` incluso), o erro de arredondamento acumulado
//     descrito acima pode empurrar o valor decimal representado pela string de digitos um pouco
//     ACIMA do valor verdadeiro -- e como esses inputs ja estao bem na fronteira
//     finito/`Infinito`, reparsear essa string com `atof` pode legitimamente ESTOURAR pra
//     `Infinito` em vez de recuperar algo proximo ao `double` finito original. Isso e' uma
//     consequencia DETERMINISTICA e compreendida da abordagem de precisao fixa (nao-precisao-
//     arbitraria) -- nao aleatoria, nao um crash, nao corrupcao silenciosa de dado -- e e' o preco
//     direto e revelado de NAO implementar uma expansao decimal exata baseada em
//     precisao-arbitraria/bignum (que e' o que `libc`s de verdade usam pro `%f` e esta
//     explicitamente fora de escopo aqui, ver acima). Valores mais longe do teto (testados ate
//     ~1e300) fazem round-trip com um erro RELATIVO pequeno em vez disso (uns poucos por 1e15,
//     ou seja um punhado de ULPs de `double`) e NAO exibem esse overflow. O proprio `ftoa` nunca
//     crasha nem produz uma string malformada pra nenhum `double` finito, incluindo `DBL_MAX` --
//     so' um reparse SUBSEQUENTE via `atof` de uma string de valor da decada mais alta pode
//     estourar.
//
//     Buffer e' inteiramente de posse do CHAMADOR (essa funcao nunca aloca) -- dimensione com
//     `CONV_FTOA_BUF_MIN` (precisao padrao) ou `CONV_FTOA_BUF_FOR(precision)` (precisao
//     customizada); o proprio ftoa NAO faz checagem de limites contra uma capacidade fornecida
//     pelo chamador (nao ha parametro `cap`, espelhando o contrato de dimensionamento-pelo-
//     chamador do `itoa`/`utoa`) -- um buffer subdimensionado e' um bug do chamador, exatamente
//     como no `itoa`/`utoa`.
char* ftoa(double value, char* buf, unsigned precision);

// EN: Parses a `double` out of the front of `s`: skips leading whitespace (same set as `atoi`),
//     accepts an optional single leading '+' or '-', then recognises -- in order -- (1) "inf" or
//     "infinity" (case-INsensitive, e.g. "INF"/"Infinity"/"iNfInItY" all match) -> signed
//     Infinity; (2) "nan" (case-insensitive) -> NaN (the leading sign, if any, sets the NaN's
//     sign bit, even though `ftoa` never emits a signed NaN -- see `ftoa`'s doc-comment); (3)
//     otherwise, a decimal number: optional integer digits, optional '.' + fractional digits,
//     optional `[eE][+-]digits` exponent -- exactly the classic `strtod` grammar, MINUS
//     hex-float support (`"0x1.8p3"`-style -- YAGNI, nothing in this codebase needs it).
//     TRAILING GARBAGE after a successfully-parsed number is ignored (same philosophy as `atoi`/
//     `atou` -- e.g. `atof("3.14xyz")` -> `3.14`, the "xyz" is simply not consumed).
//
//     MALFORMED INPUT (no valid number found at all: `""`, `"."`, `"e5"` (no mantissa),
//     `"--1"` (a second leading sign is not part of a valid number)) returns `+0.0` -- same "no
//     digits -> 0" philosophy as `atoi`/`atou`, and NOTE this early-return path always yields
//     POSITIVE zero regardless of any sign character consumed before the digit scan failed (a
//     leading '-' alone is not enough to produce `-0.0`; only a LEGITIMATELY parsed "-0"/"-0.0"
//     does, since that path has real digits and reaches the sign-application step). A LONE
//     trailing `[eE]` with no exponent digits after it (`"1e"`) is NOT consumed as part of the
//     exponent -- it is trailing garbage after the valid mantissa, so `atof("1e")` -> `1.0` (not
//     `0.0`), mirroring `atoi`'s "trailing garbage does not invalidate what was already parsed"
//     rule.
//
//     OVERFLOW of the exponent (e.g. `atof("1e400")`, `atof("-1e400")`) SATURATES to signed
//     `+Infinity`/`-Infinity` -- reached NATURALLY via IEEE-754 arithmetic overflow during the
//     scaling step (multiplying by 10.0 repeatedly; x86-64 SSE2 does not trap on overflow by
//     default, it produces `Infinity`), not via a special-cased branch. UNDERFLOW (e.g.
//     `atof("1e-400")`) similarly reaches `0.0`/a denormal via NATURAL IEEE-754 gradual underflow
//     during the scaling-down step -- "best effort", no special-casing, no guarantee of the
//     nearest representable denormal. Both the exponent-magnitude accumulator and the
//     scale-by-10-repeatedly loop are internally CAPPED (documented in `src/conv.c`) purely as a
//     defensive bound against pathological/hostile inputs (e.g. `"1e999999999"`) -- the cap is
//     set well beyond any exponent a real finite/denormal `double` could need, so it never
//     changes the result for legitimate input, only bounds worst-case work for hostile input.
//
//     Same "no libm, native SSE2 double arithmetic only" implementation stance as `ftoa` -- see
//     that doc-comment. This is the complementary "not correctly-rounded" converter: INTEGER-part
//     digit accumulation is a plain `mantissa = mantissa*10.0 + digit` loop (not an
//     arbitrary-precision or correctly-rounded `strtod` algorithm), so for very long digit strings
//     (many more than a double's ~15-17 significant digits) the result carries ordinary
//     accumulated floating-point rounding error -- documented tradeoff, matching `ftoa`'s own "not
//     shortest-round-trip" disclaimer in spirit. FRACTIONAL-part digits use a DIFFERENT technique
//     (`src/conv.c`, AUD-SEC-Delta fix): each digit is scaled down and added directly (never
//     multiplied forward like the integer part), and accumulation of fractional digits is capped
//     at the first ~16 SIGNIFICANT ones (further digits are still consumed as trailing-garbage-
//     shaped syntax but no longer folded in) -- unlike the exponent caps above, THIS cap DOES
//     change the result for real input whenever a fractional part has more than ~16 significant
//     digits, by design: a `double` cannot resolve finer fractional resolution than that anyway,
//     and capping there is what GUARANTEES `atof` of a string of fractional digits that is
//     mathematically `< 1.0` (e.g. `"0." + "9"*309`) never returns `>= 1.0` or `+Infinity` -- a
//     prior revision of this converter used the same `mantissa*10+digit` technique for fractional
//     digits too (deferring the rescale to the very end), which let a long-enough fractional part
//     inflate `mantissa` past `DBL_MAX` BEFORE the rescale ever ran, empirically confirmed to
//     return `+Inf` (and, below the old 400-digit cap, silently wrong values `>= 1.0`) for exactly
//     that input shape.
// PT: Parseia um `double` do inicio de `s`: pula whitespace inicial (mesmo conjunto do `atoi`),
//     aceita um '+' ou '-' inicial opcional unico, depois reconhece -- em ordem -- (1) "inf" ou
//     "infinity" (case-INsensivel, ex.: "INF"/"Infinity"/"iNfInItY" todos casam) -> Infinito com
//     sinal; (2) "nan" (case-insensivel) -> NaN (o sinal inicial, se houver, seta o bit de sinal
//     do NaN, mesmo que o `ftoa` nunca emita um NaN com sinal -- ver o doc-comment do `ftoa`); (3)
//     senao, um numero decimal: digitos inteiros opcionais, '.' opcional + digitos fracionarios,
//     expoente `[eE][+-]digitos` opcional -- exatamente a gramatica classica do `strtod`, MENOS
//     suporte a float hexadecimal (estilo `"0x1.8p3"` -- YAGNI, nada neste codebase precisa
//     disso).
//
//     LIXO FINAL depois de um numero parseado com sucesso e' ignorado (mesma filosofia do
//     `atoi`/`atou` -- ex.: `atof("3.14xyz")` -> `3.14`, o "xyz" simplesmente nao e' consumido).
//
//     INPUT MALFORMADO (nenhum numero valido encontrado: `""`, `"."`, `"e5"` (sem mantissa),
//     `"--1"` (um segundo sinal inicial nao faz parte de um numero valido)) retorna `+0.0` --
//     mesma filosofia "sem digitos -> 0" do `atoi`/`atou`, e NOTE que esse caminho de retorno
//     antecipado sempre da zero POSITIVO independente de qualquer caractere de sinal consumido
//     antes da varredura de digito falhar (um '-' inicial sozinho nao basta pra produzir `-0.0`;
//     so' um "-0"/"-0.0" LEGITIMAMENTE parseado da, ja que esse caminho tem digitos de verdade e
//     alcanca o passo de aplicacao de sinal). Um `[eE]` final SOZINHO sem digitos de expoente
//     depois dele (`"1e"`) NAO e' consumido como parte do expoente -- e' lixo final depois da
//     mantissa valida, entao `atof("1e")` -> `1.0` (nao `0.0`), espelhando a regra do `atoi` de
//     "lixo final nao invalida o que ja foi parseado".
//
//     OVERFLOW do expoente (ex.: `atof("1e400")`, `atof("-1e400")`) SATURA em
//     `+Infinito`/`-Infinito` com sinal -- alcancado NATURALMENTE via overflow aritmetico
//     IEEE-754 durante o passo de escala (multiplicar por 10.0 repetidamente; SSE2 x86-64 nao
//     dispara trap em overflow por padrao, produz `Infinito`), nao via um branch de caso
//     especial. UNDERFLOW (ex.: `atof("1e-400")`) similarmente alcanca `0.0`/um denormal via
//     underflow gradual NATURAL do IEEE-754 durante o passo de escala-pra-baixo -- "melhor
//     esforco", sem caso especial, sem garantia de ser o denormal representavel mais proximo.
//     Tanto o acumulador de magnitude do expoente quanto o laco de escala-por-10-repetida sao
//     internamente LIMITADOS (documentado em `src/conv.c`) puramente como um limite defensivo
//     contra input patologico/hostil (ex.: `"1e999999999"`) -- o limite e' setado bem alem de
//     qualquer expoente que um `double` finito/denormal real precisaria, entao nunca muda o
//     resultado pra input legitimo, so' limita o trabalho de pior-caso pra input hostil.
//
//     Mesma postura de implementacao "sem libm, so aritmetica double SSE2 nativa" do `ftoa` -- ver
//     aquele doc-comment. Este e' o conversor complementar "nao corretamente-arredondado":
//     acumulacao de digito da parte INTEIRA e' um laco simples `mantissa = mantissa*10.0 + digito`
//     (nao um algoritmo `strtod` de precisao-arbitraria ou corretamente-arredondado), entao pra
//     strings de digito muito longas (bem mais que os ~15-17 digitos significativos de um double)
//     o resultado carrega erro de arredondamento de ponto flutuante acumulado comum -- troca
//     documentada, batendo em espirito com o proprio aviso "nao shortest-round-trip" do `ftoa`.
//     Digitos da parte FRACIONARIA usam uma tecnica DIFERENTE (`src/conv.c`, fix AUD-SEC-Delta):
//     cada digito e' escalonado pra baixo e somado diretamente (nunca multiplicado pra frente como
//     a parte inteira), e a acumulacao de digitos fracionarios e' limitada aos primeiros ~16
//     SIGNIFICATIVOS (digitos alem disso ainda sao consumidos como sintaxe de lixo-final-formato
//     mas nao mais dobrados) -- diferente dos limites de expoente acima, ESSE limite MUDA o
//     resultado pra input real sempre que uma parte fracionaria tem mais de ~16 digitos
//     significativos, de proposito: um `double` nao consegue resolver resolucao fracionaria mais
//     fina que isso de qualquer jeito, e limitar ali e' o que GARANTE que `atof` de uma string de
//     digitos fracionarios que e' matematicamente `< 1.0` (ex.: `"0." + "9"*309`) nunca retorna
//     `>= 1.0` nem `+Infinito` -- uma revisao anterior deste conversor usava a mesma tecnica
//     `mantissa*10+digito` tambem pros digitos fracionarios (adiando o reescalonamento pro final),
//     o que deixava uma parte fracionaria longa o bastante inflar o `mantissa` alem de `DBL_MAX`
//     ANTES do reescalonamento alguma vez rodar, confirmado empiricamente retornando `+Inf` (e,
//     abaixo do antigo limite de 400 digitos, valores silenciosamente errados `>= 1.0`) exatamente
//     pra esse formato de input.
double atof(const char* s);
