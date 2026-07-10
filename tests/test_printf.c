// SPDX-License-Identifier: MPL-2.0
// EN: E2 gate program -- TDD RED first: written against include/printf.h BEFORE src/printf.c has
//     a real body, so the link step fails (undefined reference to `mini_vsnprintf`) until E2 is
//     implemented. Tests ONLY `mini_vsnprintf` (the buffer-formatting core), NOT `mini_printf`
//     (the stdout-writing wrapper) -- see include/printf.h for why: the Makefile harness checks
//     exit codes only, it cannot capture stdout, so asserting against `mini_printf`'s real I/O
//     would require manual `od -c` inspection, not an automatable TDD gate. `mini_printf` shares
//     the exact same formatting core (src/printf.c's `mini_format_core`) so exercising every
//     conversion through `mini_vsnprintf` here covers `mini_printf`'s formatting logic too --
//     only the I/O sink differs between the two, and that sink-selection is the one part NOT
//     under automated test in this harness (a manual `od -c` smoke check against `mini_printf`
//     is documented in the task report instead).
//
//     `fmt_into` is a tiny local variadic-to-va_list adapter (this file's only variadic
//     function) so each test case below can call `mini_vsnprintf` with an ordinary argument list
//     instead of hand-building a `va_list` -- it is NOT part of the public API, just test
//     plumbing living in this file.
// PT: Programa-gate do E2 -- TDD VERMELHO primeiro: escrito contra include/printf.h ANTES de
//     src/printf.c ter corpo de verdade, entao o link falha (undefined reference a
//     `mini_vsnprintf`) ate o E2 ser implementado. Testa APENAS o `mini_vsnprintf` (o nucleo de
//     formatacao em buffer), NAO o `mini_printf` (o wrapper que escreve no stdout) -- ver
//     include/printf.h pro por que: o harness do Makefile so checa exit codes, nao consegue
//     capturar stdout, entao verificar o `mini_printf` de verdade exigiria inspecao manual via
//     `od -c`, nao um gate de TDD automatizavel. O `mini_printf` compartilha exatamente o mesmo
//     nucleo de formatacao (`mini_format_core` de src/printf.c) entao exercitar toda conversao
//     via `mini_vsnprintf` aqui cobre a logica de formatacao do `mini_printf` tambem -- so o
//     sink de I/O difere entre os dois, e essa escolha-de-sink e' a unica parte SEM teste
//     automatizado neste harness (uma checagem manual via `od -c` contra o `mini_printf` esta
//     documentada no relatorio da tarefa em vez disso).
//
//     `fmt_into` e' um pequeno adaptador local variadico-para-va_list (a unica funcao variadica
//     deste arquivo) pra cada caso de teste abaixo poder chamar `mini_vsnprintf` com uma lista
//     de argumentos comum em vez de montar um `va_list` a mao -- NAO faz parte da API publica,
//     e' so' andaime de teste morando neste arquivo.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "printf.h"
#include "str.h"
#include "limits.h"
#include "types.h"

static int fmt_into(char* buf, size_t cap, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mini_vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    return ret;
}

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- %d --------------------------------------------------------------------------
    // EN: Zero, positive, negative, THE `INT_MIN` gotcha (round-trips through D3's itoa, which
    //     already handles it -- this just proves the plumbing passes the right value through).
    // PT: Zero, positivo, negativo, O gotcha do `INT_MIN` (passa pelo itoa do D3, que ja trata
    //     isso -- so prova que a canalizacao repassa o valor certo).
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%d", 0);
        TEST_ASSERT_EQ(strcmp(buf, "0"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%d", 123);
        TEST_ASSERT_EQ(strcmp(buf, "123"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%d", -123);
        TEST_ASSERT_EQ(strcmp(buf, "-123"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%d", INT_MIN);
        TEST_ASSERT_EQ(strcmp(buf, "-2147483648"), 0);
    }

    // ---- %u --------------------------------------------------------------------------
    // EN: Zero, small value, UINT_MAX (longest possible base-10 output).
    // PT: Zero, valor pequeno, UINT_MAX (maior saida possivel em base 10).
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%u", 0u);
        TEST_ASSERT_EQ(strcmp(buf, "0"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%u", 42u);
        TEST_ASSERT_EQ(strcmp(buf, "42"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%u", UINT_MAX);
        TEST_ASSERT_EQ(strcmp(buf, "4294967295"), 0);
    }

    // ---- %x --------------------------------------------------------------------------
    // EN: LOWERCASE hex, no "0x" prefix; UINT_MAX -> "ffffffff".
    // PT: Hex MINUSCULO, sem prefixo "0x"; UINT_MAX -> "ffffffff".
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%x", 0xdeadbeefu);
        TEST_ASSERT_EQ(strcmp(buf, "deadbeef"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%x", UINT_MAX);
        TEST_ASSERT_EQ(strcmp(buf, "ffffffff"), 0);
    }

    // ---- %f (SOV-FCONV) ----------------------------------------------------------------
    // EN: Default precision only (6, D3's `CONV_FTOA_DEFAULT_PRECISION`) -- no width/precision
    //     modifiers, same YAGNI stance as every other conversion here. Just plumbing checks (the
    //     conversion LOGIC itself -- rounding, signed zero, Infinity/NaN, DBL_MAX -- is D3's own
    //     responsibility and is exhaustively covered by tests/test_conv.c; this file only proves
    //     `%f` reads `va_arg(ap, double)` correctly and reuses `ftoa` faithfully through the sink).
    // PT: So a precisao padrao (6, `CONV_FTOA_DEFAULT_PRECISION` do D3) -- sem modificadores de
    //     largura/precisao, mesma postura YAGNI de toda outra conversao aqui. So checagens de
    //     canalizacao (a LOGICA de conversao em si -- arredondamento, zero com sinal,
    //     Infinito/NaN, DBL_MAX -- e' responsabilidade propria do D3 e e' coberta exaustivamente
    //     por tests/test_conv.c; este arquivo so prova que o `%f` le `va_arg(ap, double)`
    //     corretamente e reusa o `ftoa` fielmente atraves do sink).
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%f", 3.5);
        TEST_ASSERT_EQ(strcmp(buf, "3.500000"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%f", -1.0);
        TEST_ASSERT_EQ(strcmp(buf, "-1.000000"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "x=%d y=%f", 7, 0.5);
        TEST_ASSERT_EQ(strcmp(buf, "x=7 y=0.500000"), 0); // mixed with %d, same as %s's own case
    }

    // ---- %s --------------------------------------------------------------------------
    // EN: Normal string, empty string, and this project's documented `NULL` behaviour --
    //     prints "(null)" rather than crashing (no libc, no segfault-avoiding guard otherwise).
    // PT: String normal, string vazia, e o comportamento documentado deste projeto pra `NULL`
    //     -- imprime "(null)" em vez de estourar (sem libc, sem guarda anti-segfault de outro
    //     jeito).
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%s", "hi");
        TEST_ASSERT_EQ(strcmp(buf, "hi"), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%s", "");
        TEST_ASSERT_EQ(strcmp(buf, ""), 0);
    }
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%s", (const char*)NULL);
        TEST_ASSERT_EQ(strcmp(buf, "(null)"), 0);
    }

    // ---- %c --------------------------------------------------------------------------
    // EN: One character, passed as `int` per default argument promotion (see file header).
    // PT: Um caractere, passado como `int` pela promocao de argumento padrao (ver cabecalho do
    //     arquivo).
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%c", 'Q');
        TEST_ASSERT_EQ(strcmp(buf, "Q"), 0);
    }

    // ---- %% --------------------------------------------------------------------------
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "100%%");
        TEST_ASSERT_EQ(strcmp(buf, "100%"), 0);
    }

    // ---- mixed literal + conversions --------------------------------------------------
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "x=%d y=%s", 7, "abc");
        TEST_ASSERT_EQ(strcmp(buf, "x=7 y=abc"), 0);
    }

    // ---- unknown directive -------------------------------------------------------------
    // EN: `%z` is not a recognised conversion -- documented behaviour: print '%' and the
    //     following character literally, consume no variadic argument.
    // PT: `%z` nao e' uma conversao reconhecida -- comportamento documentado: imprime '%' e o
    //     caractere seguinte literalmente, nao consome argumento variadico nenhum.
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "%z");
        TEST_ASSERT_EQ(strcmp(buf, "%z"), 0);
    }

    // ---- trailing lone '%' ---------------------------------------------------------------
    {
        char buf[64];
        fmt_into(buf, sizeof(buf), "abc%");
        TEST_ASSERT_EQ(strcmp(buf, "abc%"), 0);
    }

    // ---- empty format string -------------------------------------------------------------
    {
        char buf[64];
        int ret = fmt_into(buf, sizeof(buf), "");
        TEST_ASSERT_EQ(strcmp(buf, ""), 0);
        TEST_ASSERT_EQ(ret, 0);
    }

    // ---- return value: logical length, C99 vsnprintf convention --------------------------
    // EN: Return value is the length that WOULD have been produced, regardless of truncation.
    // PT: Valor de retorno e' o tamanho que TERIA sido produzido, independente de truncamento.
    {
        char buf[64];
        int ret = fmt_into(buf, sizeof(buf), "x=%d y=%s", 7, "abc");
        TEST_ASSERT_EQ(ret, 9); // "x=7 y=abc" is 9 chars
    }

    // ---- cap == 0: buf untouched (may be NULL), only the logical length is computed ------
    {
        int ret = fmt_into((char*)NULL, 0, "hello %d", 5);
        TEST_ASSERT_EQ(ret, 7); // "hello 5" is 7 chars
    }

    // ---- truncation: buffer smaller than needed, NEVER overflows -------------------------
    // EN: `cap == 4` -> room for 3 formatted bytes + '\0'. Source would be "hello" (5 chars);
    //     truncated to "hel" + '\0'. Return value is still the FULL logical length (5), per the
    //     vsnprintf convention verified above -- callers detect truncation via `ret >= cap`.
    // PT: `cap == 4` -> espaco pra 3 bytes formatados + '\0'. A fonte seria "hello" (5 chars);
    //     truncada pra "hel" + '\0'. O valor de retorno ainda e' o tamanho logico TOTAL (5),
    //     seguindo a convencao do vsnprintf verificada acima -- quem chama detecta truncamento
    //     via `ret >= cap`.
    {
        char buf[4];
        int ret = fmt_into(buf, sizeof(buf), "hello");
        TEST_ASSERT_EQ(strcmp(buf, "hel"), 0);
        TEST_ASSERT_EQ(ret, 5);
        TEST_ASSERT(strlen(buf) < sizeof(buf)); // NUL-terminated within bounds, never overflowed
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
