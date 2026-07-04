// SPDX-License-Identifier: MPL-2.0
// EN: D3 -- see include/conv.h for the contract. Digit-at-a-time implementations, same
//     philosophy as D1 (mem.c)/D2 (str.c): correctness and auditability first, no
//     table-driven/SIMD tricks.
//
//     utoa's core loop generates digits LEAST-significant-first (the only way division/modulo
//     by `base` can produce them), writing them into `buf` in that (backwards) order, then
//     reverses the written span IN PLACE with a two-pointer swap -- this is the "reversal"
//     gotcha the D3 test suite checks explicitly (a missing/wrong reversal step would emit
//     "321" for `utoa(123, buf, 10)` instead of "123").
//
//     itoa reuses utoa for both bases, never duplicating the digit-emission loop:
//       - base 16: `value` is handed to utoa as `(unsigned)value` with NO other transform --
//         the implicit signed-to-unsigned conversion IS the "reinterpret as raw 32-bit bit
//         pattern" the header promises (two's complement -> the same bits read as unsigned),
//         so `itoa(-1, buf, 16)` naturally becomes `utoa(0xFFFFFFFFu, buf, 16)` -> "ffffffff".
//       - base 10 (or any other, folded to 10): the SIGN is handled by conv.c itself, THEN the
//         MAGNITUDE is handed to utoa as an unsigned value. THE `INT_MIN` GOTCHA lives entirely
//         in how that magnitude is computed: `-value` would itself overflow `int` for
//         `value == INT_MIN` (undefined behaviour in C -- `-INT_MIN` is not representable as
//         `int`). The safe formula is `(unsigned)(-(value + 1)) + 1u`: `value + 1` is always
//         representable as `int` (even `INT_MIN + 1`), negating THAT never overflows either
//         (its magnitude is at most `INT_MAX`), converting to `unsigned` is exact, and the `+1u`
//         after the conversion restores the one unit subtracted before negation -- all in
//         unsigned arithmetic, which never overflows (it wraps by definition, and here it never
//         even needs to). Verified explicitly by the test suite: `itoa(INT_MIN, buf, 10)` ->
//         "-2147483648".
//
//     atoi/atou share the same "parse digits into an unsigned accumulator, sign is separate"
//     shape, and the same SATURATING-overflow technique: before folding in the next digit,
//     check `mag > (limit - digit) / 10` -- if true, `mag * 10 + digit` would exceed `limit`,
//     so clamp to `limit` instead of computing the (would-be UB, for atoi's signed result)
//     multiply-and-add. Once `mag` reaches `limit`, the SAME check keeps re-triggering on every
//     subsequent digit (dividing an already-at-the-cap `limit` by 10 always yields something
//     smaller than `limit` itself), so no separate "already overflowed" flag is needed -- the
//     saturation is self-stabilizing. atoi's negative `limit` is `(unsigned)INT_MAX + 1u`
//     (`INT_MIN`'s magnitude, which does not fit in a signed int either -- same unsigned-domain
//     reasoning as the itoa gotcha above); atoi detects "the accumulated magnitude equals
//     exactly the negative limit" as its own special case and returns `INT_MIN` directly
//     (`-(int)mag` would itself be UB for that one value, mirroring the itoa gotcha in reverse).
//
//     atou's "-5" -> 0 gotcha needs NO special-case code at all: a leading '-' is simply never
//     consumed (only '+' is), so the digit-scan loop starts sitting ON the '-' character, which
//     fails the `'0'..'9'` range check immediately -- the loop body never executes, `mag` stays
//     at its initial 0.
//
//     THE `-fno-builtin` gotcha does not really apply here the way it did to D1/D2: there is no
//     libc `itoa`/`atoi`/`utoa`/`atou` idiom clang's loop-idiom recognition pattern-matches
//     against (unlike memcpy-shaped or strlen-shaped loops) -- but `nm build/obj/conv.o` is
//     still checked per this project's standing verification habit (see task report): four `T`
//     symbols (utoa/itoa/atoi/atou, defined here), zero `U` symbols of any kind (no dependency
//     on D1/D2 or libc -- unlike str.c, conv.c does not even need memcpy/memset/strlen for this
//     digit-only logic).
// PT: D3 -- ver include/conv.h pro contrato. Implementacoes digito-a-digito, mesma filosofia do
//     D1 (mem.c)/D2 (str.c): corretude e auditabilidade primeiro, sem truques de
//     tabela/SIMD.
//
//     O laco nucleo do utoa gera digitos do MENOS-significativo-primeiro (o unico jeito que
//     divisao/modulo por `base` consegue produzi-los), escrevendo-os em `buf` nessa ordem (de
//     tras pra frente), depois reverte o trecho escrito NO LUGAR com uma troca de dois
//     ponteiros -- esse e' o gotcha de "reversao" que a suite de teste do D3 checa
//     explicitamente (um passo de reversao ausente/errado emitiria "321" pra
//     `utoa(123, buf, 10)` em vez de "123").
//
//     itoa reusa utoa pras duas bases, nunca duplicando o laco de emissao de digito:
//       - base 16: `value` e' repassado ao utoa como `(unsigned)value` SEM nenhuma outra
//         transformacao -- a conversao implicita signed-pra-unsigned E' o "reinterpretar como
//         padrao de bits cru de 32 bits" que o header promete (complemento de dois -> os
//         mesmos bits lidos como unsigned), entao `itoa(-1, buf, 16)` naturalmente vira
//         `utoa(0xFFFFFFFFu, buf, 16)` -> "ffffffff".
//       - base 10 (ou qualquer outra, dobrada pra 10): o SINAL e' tratado pelo proprio conv.c,
//         DEPOIS a MAGNITUDE e' repassada ao utoa como valor unsigned. O gotcha do `INT_MIN`
//         mora inteiramente em como essa magnitude e' computada: `-value` por si so estouraria
//         `int` pra `value == INT_MIN` (comportamento indefinido em C -- `-INT_MIN` nao e'
//         representavel como `int`). A formula segura e' `(unsigned)(-(value + 1)) + 1u`:
//         `value + 1` e' sempre representavel como `int` (mesmo `INT_MIN + 1`), negar ISSO
//         nunca estoura tambem (sua magnitude e' no maximo `INT_MAX`), converter pra `unsigned`
//         e' exato, e o `+1u` depois da conversao restaura a unidade subtraida antes da negacao
//         -- tudo em aritmetica unsigned, que nunca estoura (da wrap por definicao, e aqui nem
//         chega a precisar). Verificado explicitamente pela suite de teste:
//         `itoa(INT_MIN, buf, 10)` -> "-2147483648".
//
//     atoi/atou compartilham a mesma forma "parseia digitos num acumulador unsigned, sinal e'
//     separado", e a mesma tecnica de overflow SATURANTE: antes de dobrar o proximo digito no
//     acumulador, checa `mag > (limit - digit) / 10` -- se verdadeiro, `mag * 10 + digit`
//     excederia `limit`, entao satura em `limit` em vez de computar a multiplicacao-e-soma (que
//     seria UB, pro resultado com sinal do atoi). Uma vez que `mag` alcanca `limit`, a MESMA
//     checagem continua re-disparando a cada digito subsequente (dividir um `limit` ja-no-teto
//     por 10 sempre da algo menor que o proprio `limit`), entao nenhuma flag separada de "ja
//     estourou" e' necessaria -- a saturacao e' auto-estabilizante. O `limit` negativo do atoi
//     e' `(unsigned)INT_MAX + 1u` (a magnitude de `INT_MIN`, que tambem nao cabe num int com
//     sinal -- mesmo raciocinio em dominio unsigned do gotcha do itoa acima); o atoi detecta "a
//     magnitude acumulada e' exatamente igual ao limite negativo" como seu proprio caso especial
//     e retorna `INT_MIN` diretamente (`-(int)mag` seria ele mesmo UB pra esse unico valor,
//     espelhando o gotcha do itoa ao contrario).
//
//     O gotcha "-5" -> 0 do atou nao precisa de NENHUM codigo de caso especial: um '-' inicial
//     simplesmente nunca e' consumido (so' '+' e'), entao o laco de varredura de digito comeca
//     sentado NO caractere '-', que falha a checagem de faixa `'0'..'9'` imediatamente -- o
//     corpo do laco nunca executa, `mag` fica no seu 0 inicial.
//
//     O gotcha `-fno-builtin` nao se aplica de verdade aqui do jeito que se aplicou ao D1/D2:
//     nao existe idioma de libc `itoa`/`atoi`/`utoa`/`atou` que o loop-idiom recognition do
//     clang pattern-matche (diferente de lacos com forma de memcpy ou de strlen) -- mas
//     `nm build/obj/conv.o` e' checado assim mesmo, pelo habito de verificacao ja padrao deste
//     projeto (ver relatorio da tarefa): quatro simbolos `T` (utoa/itoa/atoi/atou, definidos
//     aqui), zero simbolos `U` de qualquer tipo (nenhuma dependencia de D1/D2 ou libc -- ao
//     contrario do str.c, o conv.c nem precisa de memcpy/memset/strlen pra essa logica so' de
//     digito).
// Copyright (c) 2026 Petrus Silva Costa
#include "conv.h"
#include "limits.h"

char* utoa(unsigned value, char* buf, int base) {
    if (base != 10 && base != 16) {
        base = 10;
    }

    size_t i = 0;
    if (value == 0) {
        buf[i++] = '0';
    } else {
        // EN: Digits come out least-significant-first (division/modulo has no other way to
        //     produce them) -- written forward into buf for now, reversed below.
        // PT: Digitos saem do menos-significativo-primeiro (divisao/modulo nao tem outro jeito
        //     de produzi-los) -- escritos pra frente em buf por enquanto, revertidos abaixo.
        while (value != 0) {
            unsigned digit = value % (unsigned)base;
            value /= (unsigned)base;
            buf[i++] = (digit < 10) ? (char)('0' + digit) : (char)('a' + (digit - 10));
        }
    }
    buf[i] = '\0';

    // EN: Reverse buf[0..i) in place, two-pointer swap -- turns the least-significant-first
    //     scratch order above into the correct most-significant-first string.
    // PT: Reverte buf[0..i) no lugar, troca de dois ponteiros -- transforma a ordem de
    //     rascunho menos-significativo-primeiro acima na string correta
    //     mais-significativo-primeiro.
    size_t lo = 0;
    size_t hi = i;
    while (hi > 0 && lo < hi - 1) {
        char tmp = buf[lo];
        buf[lo] = buf[hi - 1];
        buf[hi - 1] = tmp;
        lo++;
        hi--;
    }

    return buf;
}

char* itoa(int value, char* buf, int base) {
    if (base != 10 && base != 16) {
        base = 10;
    }

    if (base == 16) {
        // EN: Base 16: no sign, raw bit-pattern reinterpretation. The implicit signed->unsigned
        //     conversion below IS the reinterpretation (two's complement bits read as unsigned).
        // PT: Base 16: sem sinal, reinterpretacao crua de bit-pattern. A conversao implicita
        //     signed->unsigned abaixo E' a reinterpretacao (bits de complemento de dois lidos
        //     como unsigned).
        return utoa((unsigned)value, buf, 16);
    }

    // EN: Base 10: SIGNED decimal. Sign handled here; magnitude handed to utoa as unsigned.
    // PT: Base 10: decimal COM SINAL. Sinal tratado aqui; magnitude repassada ao utoa como
    //     unsigned.
    if (value < 0) {
        buf[0] = '-';
        // EN: THE INT_MIN gotcha -- see file header for why this exact formula (never `-value`
        //     directly, which would overflow `int` for value == INT_MIN).
        // PT: O gotcha do INT_MIN -- ver cabecalho do arquivo pra por que exatamente essa
        //     formula (nunca `-value` direto, que estouraria `int` pra value == INT_MIN).
        unsigned magnitude = (unsigned)(-(value + 1)) + 1u;
        utoa(magnitude, buf + 1, 10);
        return buf;
    }

    return utoa((unsigned)value, buf, 10);
}

int atoi(const char* s) {
    size_t i = 0;
    // EN: Skip leading whitespace: ' ', '\t', '\n', '\v', '\f', '\r' (exact set per header).
    // PT: Pula whitespace inicial: ' ', '\t', '\n', '\v', '\f', '\r' (conjunto exato conforme
    //     header).
    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\v' || s[i] == '\f' ||
           s[i] == '\r') {
        i++;
    }

    int negative = 0;
    if (s[i] == '+') {
        i++;
    } else if (s[i] == '-') {
        negative = 1;
        i++;
    }

    // EN: `limit` is the magnitude ceiling for the current sign -- INT_MAX for positive,
    //     INT_MIN's magnitude (which itself does not fit in `int`, hence unsigned here) for
    //     negative. Accumulation and saturation both happen in this unsigned domain.
    // PT: `limit` e' o teto de magnitude pro sinal atual -- INT_MAX pro positivo, a magnitude
    //     de INT_MIN (que ela mesma nao cabe em `int`, daqui o unsigned aqui) pro negativo.
    //     Acumulacao e saturacao acontecem as duas nesse dominio unsigned.
    unsigned limit = negative ? ((unsigned)INT_MAX + 1u) : (unsigned)INT_MAX;

    unsigned magnitude = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        unsigned digit = (unsigned)(s[i] - '0');
        // EN: Saturating check BEFORE the multiply-add -- self-stabilizing once triggered (see
        //     file header): avoids ever computing an accumulation that would exceed `limit`.
        // PT: Checagem saturante ANTES da multiplicacao-soma -- auto-estabilizante uma vez
        //     disparada (ver cabecalho do arquivo): evita computar uma acumulacao que
        //     excederia `limit`.
        if (magnitude > (limit - digit) / 10) {
            magnitude = limit;
        } else {
            magnitude = magnitude * 10 + digit;
        }
        i++;
    }

    if (!negative) {
        return (int)magnitude;
    }
    // EN: THE INT_MIN gotcha in reverse -- `magnitude == limit` here means the negative-limit
    //     magnitude itself (INT_MIN's), which does NOT fit as a positive `int` to negate
    //     (`-(int)magnitude` would be UB for exactly this value). Return INT_MIN directly.
    // PT: O gotcha do INT_MIN ao contrario -- `magnitude == limit` aqui significa a propria
    //     magnitude do limite negativo (a de INT_MIN), que NAO cabe como `int` positivo pra
    //     negar (`-(int)magnitude` seria UB exatamente pra esse valor). Retorna INT_MIN
    //     diretamente.
    if (magnitude == limit) {
        return INT_MIN;
    }
    return -(int)magnitude;
}

unsigned atou(const char* s) {
    size_t i = 0;
    // EN: Same whitespace set as atoi.
    // PT: Mesmo conjunto de whitespace do atoi.
    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\v' || s[i] == '\f' ||
           s[i] == '\r') {
        i++;
    }

    if (s[i] == '+') {
        i++;
    }
    // EN: A leading '-' is deliberately NOT handled/consumed here -- see file header: it is
    //     simply left in place, so the digit-scan loop below fails immediately on it, giving
    //     the documented atou("-5") -> 0 without any special-case code.
    // PT: Um '-' inicial deliberadamente NAO e' tratado/consumido aqui -- ver cabecalho do
    //     arquivo: e' simplesmente deixado no lugar, entao o laco de varredura de digito abaixo
    //     falha imediatamente nele, dando o atou("-5") -> 0 documentado sem nenhum codigo de
    //     caso especial.

    unsigned magnitude = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        unsigned digit = (unsigned)(s[i] - '0');
        if (magnitude > (UINT_MAX - digit) / 10) {
            magnitude = UINT_MAX;
        } else {
            magnitude = magnitude * 10 + digit;
        }
        i++;
    }

    return magnitude;
}
