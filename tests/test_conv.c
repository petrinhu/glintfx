// SPDX-License-Identifier: MPL-2.0
// EN: D3 gate program -- TDD RED first: written against include/conv.h BEFORE src/conv.c has a
//     real body, so the link step fails (undefined reference to utoa/itoa/atoi/atou) until D3
//     is implemented. Covers the spec cases from the task brief: utoa (0, small value, UINT_MAX
//     base 10, base-16 lowercase, digit-reversal correctness), itoa base 10 (0, positive,
//     negative, THE `INT_MIN` gotcha, `INT_MAX`), itoa base 16 (THE bit-pattern gotcha for -1,
//     positive), atoi (basic, whitespace+sign skip, trailing-garbage stop, empty string,
//     saturating overflow both directions), atou (basic, `UINT_MAX`, leading '+', THE "-5" -> 0
//     gotcha, saturating overflow). String results are checked via OUR OWN `strcmp` (D2,
//     already audited/approved -- not circular, since strcmp itself is not under test here).
//     A round-trip (`atoi(itoa(x)) == x`) closes out each itoa-base-10 case as a SECONDARY
//     check, not the primary assertion (per task brief: primary checks are always the literal
//     string/value).
// PT: Programa-gate do D3 -- TDD VERMELHO primeiro: escrito contra include/conv.h ANTES de
//     src/conv.c ter corpo de verdade, entao o link falha (undefined reference a
//     utoa/itoa/atoi/atou) ate o D3 ser implementado. Cobre os casos da especificacao do brief
//     da tarefa: utoa (0, valor pequeno, UINT_MAX base 10, base-16 minusculo, corretude da
//     reversao de digitos), itoa base 10 (0, positivo, negativo, O gotcha do `INT_MIN`,
//     `INT_MAX`), itoa base 16 (O gotcha de bit-pattern pro -1, positivo), atoi (basico, pular
//     whitespace+sinal, parar em lixo final, string vazia, overflow saturante nas duas
//     direcoes), atou (basico, `UINT_MAX`, '+' inicial, O gotcha "-5" -> 0, overflow saturante).
//     Resultados de string sao checados via o NOSSO PROPRIO `strcmp` (D2, ja auditado/aprovado
//     -- nao e' circular, ja que o proprio strcmp nao esta sob teste aqui). Um round-trip
//     (`atoi(itoa(x)) == x`) fecha cada caso de itoa-base-10 como checagem SECUNDARIA, nao a
//     asserção primaria (conforme brief da tarefa: as checagens primarias sao sempre a
//     string/valor literal).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "conv.h"
#include "str.h"
#include "limits.h"
#include "types.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- utoa -------------------------------------------------------------------
    // EN: Zero -> "0", any base.
    // PT: Zero -> "0", qualquer base.
    {
        char buf[CONV_UTOA_BUF_MIN];
        char* ret = utoa(0, buf, 10);
        TEST_ASSERT_EQ((void*)ret, (void*)buf);
        TEST_ASSERT_EQ(strcmp(buf, "0"), 0);
    }

    // EN: Small value, base 10 -- also the digit-REVERSAL check: digits are generated
    //     least-significant-first internally, so a naive implementation could emit "321"
    //     instead of "123" if the reversal step is missing/wrong.
    // PT: Valor pequeno, base 10 -- tambem a checagem de REVERSAO de digitos: digitos sao
    //     gerados do menos-significativo-primeiro internamente, entao uma implementacao
    //     ingenua poderia emitir "321" em vez de "123" se o passo de reversao estiver
    //     ausente/errado.
    {
        char buf[CONV_UTOA_BUF_MIN];
        utoa(123, buf, 10);
        TEST_ASSERT_EQ(strcmp(buf, "123"), 0);
    }

    // EN: UINT_MAX, base 10 -- longest possible base-10 utoa output.
    // PT: UINT_MAX, base 10 -- maior saida possivel de utoa em base 10.
    {
        char buf[CONV_UTOA_BUF_MIN];
        utoa(UINT_MAX, buf, 10);
        TEST_ASSERT_EQ(strcmp(buf, "4294967295"), 0);
    }

    // EN: Base 16, LOWERCASE digits, no "0x" prefix.
    // PT: Base 16, digitos MINUSCULOS, sem prefixo "0x".
    {
        char buf[CONV_UTOA_BUF_MIN];
        utoa(255u, buf, 16);
        TEST_ASSERT_EQ(strcmp(buf, "ff"), 0);
    }

    // EN: Base 16, a value that exercises all hex digit ranges ('a'-'f' AND '0'-'9').
    // PT: Base 16, um valor que exercita todas as faixas de digito hex ('a'-'f' E '0'-'9').
    {
        char buf[CONV_UTOA_BUF_MIN];
        utoa(0xdeadbeefu, buf, 16);
        TEST_ASSERT_EQ(strcmp(buf, "deadbeef"), 0);
    }

    // ---- itoa base 10 ------------------------------------------------------------
    // EN: Zero -> "0", no sign.
    // PT: Zero -> "0", sem sinal.
    {
        char buf[CONV_ITOA_BUF_MIN];
        char* ret = itoa(0, buf, 10);
        TEST_ASSERT_EQ((void*)ret, (void*)buf);
        TEST_ASSERT_EQ(strcmp(buf, "0"), 0);
    }

    // EN: Positive value -- no leading '+'.
    // PT: Valor positivo -- sem '+' inicial.
    {
        char buf[CONV_ITOA_BUF_MIN];
        itoa(123, buf, 10);
        TEST_ASSERT_EQ(strcmp(buf, "123"), 0);
        TEST_ASSERT_EQ(atoi(itoa(123, buf, 10)), 123); // round-trip, secondary
    }

    // EN: Negative value -- leading '-', magnitude follows.
    // PT: Valor negativo -- '-' inicial, magnitude em seguida.
    {
        char buf[CONV_ITOA_BUF_MIN];
        itoa(-123, buf, 10);
        TEST_ASSERT_EQ(strcmp(buf, "-123"), 0);
        TEST_ASSERT_EQ(atoi(itoa(-123, buf, 10)), -123); // round-trip, secondary
    }

    // EN: THE `INT_MIN` gotcha -- magnitude of INT_MIN does not fit in a signed int, must be
    //     computed via unsigned arithmetic (`(unsigned)(-(value+1))+1u`), never a raw `-value`.
    //     Explicitly verified here, not just via round-trip.
    // PT: O gotcha do `INT_MIN` -- a magnitude de INT_MIN nao cabe num int com sinal, deve ser
    //     computada via aritmetica unsigned (`(unsigned)(-(value+1))+1u`), nunca um `-value`
    //     cru. Verificado explicitamente aqui, nao so via round-trip.
    {
        char buf[CONV_ITOA_BUF_MIN];
        char* ret = itoa(INT_MIN, buf, 10);
        TEST_ASSERT_EQ((void*)ret, (void*)buf);
        TEST_ASSERT_EQ(strcmp(buf, "-2147483648"), 0);
        TEST_ASSERT_EQ(atoi(itoa(INT_MIN, buf, 10)), INT_MIN); // round-trip, secondary
    }

    // EN: INT_MAX -- longest possible positive base-10 itoa output.
    // PT: INT_MAX -- maior saida positiva possivel de itoa em base 10.
    {
        char buf[CONV_ITOA_BUF_MIN];
        itoa(INT_MAX, buf, 10);
        TEST_ASSERT_EQ(strcmp(buf, "2147483647"), 0);
        TEST_ASSERT_EQ(atoi(itoa(INT_MAX, buf, 10)), INT_MAX); // round-trip, secondary
    }

    // ---- itoa base 16 (bit-pattern, unsigned, no sign) ----------------------------
    // EN: THE bit-pattern gotcha -- `itoa(-1, buf, 16)` reinterprets -1's raw 32-bit two's
    //     complement bit pattern (0xFFFFFFFF) as UNSIGNED and prints it WITHOUT a sign.
    // PT: O gotcha de bit-pattern -- `itoa(-1, buf, 16)` reinterpreta o padrao de bits cru de
    //     32-bit complemento-de-dois de -1 (0xFFFFFFFF) como UNSIGNED e imprime SEM sinal.
    {
        char buf[CONV_ITOA_BUF_MIN];
        itoa(-1, buf, 16);
        TEST_ASSERT_EQ(strcmp(buf, "ffffffff"), 0);
    }

    // EN: Positive value, base 16 -- same lowercase rule as utoa.
    // PT: Valor positivo, base 16 -- mesma regra de minusculo do utoa.
    {
        char buf[CONV_ITOA_BUF_MIN];
        itoa(255, buf, 16);
        TEST_ASSERT_EQ(strcmp(buf, "ff"), 0);
    }

    // ---- atoi ----------------------------------------------------------------------
    // EN: Basic cases -- "0", plain positive, plain negative.
    // PT: Casos basicos -- "0", positivo simples, negativo simples.
    {
        TEST_ASSERT_EQ(atoi("0"), 0);
        TEST_ASSERT_EQ(atoi("123"), 123);
        TEST_ASSERT_EQ(atoi("-123"), -123);
    }

    // EN: Leading whitespace skipped, then an explicit '+' consumed.
    // PT: Whitespace inicial pulado, depois um '+' explicito consumido.
    {
        TEST_ASSERT_EQ(atoi("  +42"), 42);
    }

    // EN: Trailing non-digit garbage stops the scan but does not invalidate what was parsed.
    // PT: Lixo final nao-digito para a varredura mas nao invalida o que foi parseado.
    {
        TEST_ASSERT_EQ(atoi("12abc"), 12);
    }

    // EN: Empty string -> 0 (no digits at all).
    // PT: String vazia -> 0 (nenhum digito).
    {
        TEST_ASSERT_EQ(atoi(""), 0);
    }

    // EN: Overflow SATURATES (defined behaviour, unlike standard C's atoi) -- both directions.
    // PT: Overflow SATURA (comportamento definido, diferente do atoi padrao de C) -- nas duas
    //     direcoes.
    {
        TEST_ASSERT_EQ(atoi("9999999999"), INT_MAX);
        TEST_ASSERT_EQ(atoi("-9999999999"), INT_MIN);
    }

    // ---- atou ------------------------------------------------------------------------
    // EN: Basic cases -- "0", UINT_MAX exactly, leading '+'.
    // PT: Casos basicos -- "0", UINT_MAX exato, '+' inicial.
    {
        TEST_ASSERT_EQ(atou("0"), 0u);
        TEST_ASSERT_EQ(atou("4294967295"), UINT_MAX);
        TEST_ASSERT_EQ(atou("+7"), 7u);
    }

    // EN: THE "-5" -> 0 gotcha -- a leading '-' is NOT part of a valid unsigned number and is
    //     NOT consumed; the digit scan starts at the '-' itself, finds no digit, so the result
    //     is 0 (NOT a silent negate/wrap of 5).
    // PT: O gotcha "-5" -> 0 -- um '-' inicial NAO faz parte de um numero unsigned valido e NAO
    //     e' consumido; a varredura de digito comeca no proprio '-', nao acha digito, entao o
    //     resultado e' 0 (NAO um negate/wrap silencioso de 5).
    {
        TEST_ASSERT_EQ(atou("-5"), 0u);
    }

    // EN: Overflow SATURATES at UINT_MAX.
    // PT: Overflow SATURA em UINT_MAX.
    {
        TEST_ASSERT_EQ(atou("99999999999"), UINT_MAX);
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
