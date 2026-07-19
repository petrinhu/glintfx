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
//
//     SOV-FCONV (appended later, TODO.md W15/P2): `ftoa`/`atof` (double<->string) test block,
//     down near the end of `main`, right before `TEST_PASS()`. Covers round-trip (delta-bounded,
//     two tolerance regimes -- see `t_close_abs`/`t_close_rel` above `main`), +-0.0 sign
//     preservation, Infinity/NaN both directions, smallest denormal, `DBL_MAX`/exponent overflow
//     (including the DOCUMENTED top-of-range round-trip-to-Infinity edge case -- see
//     include/conv.h), half-away-from-zero rounding at a halfway boundary, and malformed input.
//     Own local bit-pattern test helpers (`t_bits`/`t_sign`/`t_is_nan`/`t_is_pos_inf`/
//     `t_is_neg_inf`/`t_abs`/`t_close_abs`/`t_close_rel`) sit just above `main` -- not part of the
//     public API, self-contained test plumbing (this project has no libm `isnan`/`isinf`/
//     `signbit`/`fabs`).
//     SOV-FCONV (acrescentado depois, TODO.md W15/P2): bloco de teste do `ftoa`/`atof`
//     (double<->string), perto do fim de `main`, logo antes do `TEST_PASS()`. Cobre round-trip
//     (limitado por delta, dois regimes de tolerancia -- ver `t_close_abs`/`t_close_rel` acima de
//     `main`), preservacao de sinal de +-0.0, Infinito/NaN nas duas direcoes, menor denormal,
//     `DBL_MAX`/overflow de expoente (incluindo o caso de fronteira DOCUMENTADO de
//     round-trip-vira-Infinito no topo da faixa -- ver include/conv.h), arredondamento
//     metade-pra-longe-de-zero numa fronteira de meio-caminho, e input malformado. Helpers de
//     teste locais de padrao-de-bits proprios (`t_bits`/`t_sign`/`t_is_nan`/`t_is_pos_inf`/
//     `t_is_neg_inf`/`t_abs`/`t_close_abs`/`t_close_rel`) ficam logo acima de `main` -- nao fazem
//     parte da API publica, andaime de teste autocontido (este projeto nao tem `isnan`/`isinf`/
//     `signbit`/`fabs` de libm).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "conv.h"
#include "str.h"
#include "limits.h"
#include "types.h"

// ================================================================================================
// SOV-FCONV: ftoa/atof test helpers -- bit-pattern inspection for `double`, hand-rolled here
// (NOT a dependency on conv.c's own `static` internal helpers, which are not exported; this is
// intentionally self-contained test plumbing, same technique documented in src/conv.c).
//
// EN: These are NOT part of the public API under test -- just local test infrastructure, needed
//     because this project has no libm `isnan`/`isinf`/`signbit`/`fabs` to check special-value
//     results with, and the harness (include/test.h) cannot print actual runtime double values
//     (same D3-era limitation documented there) -- so every assertion below compares against a
//     LITERAL expected bit pattern or uses one of these tiny helpers, never a printed diff.
// PT: Isso NAO faz parte da API publica sob teste -- so andaime de teste local, necessario porque
//     este projeto nao tem `isnan`/`isinf`/`signbit`/`fabs` de libm pra checar resultados de valor
//     especial, e o harness (include/test.h) nao consegue imprimir valores double reais de
//     runtime (mesma limitacao da era-D3 documentada la) -- entao toda asserção abaixo compara
//     contra um padrao de bits esperado LITERAL ou usa um destes pequenos helpers, nunca um diff
//     impresso.
static unsigned long t_bits(double d) {
    union { double d; unsigned long u; } v;
    v.d = d;
    return v.u;
}
static int t_sign(double d) {
    return (int)(t_bits(d) >> 63);
}
static int t_is_nan(double d) {
    unsigned long b = t_bits(d);
    unsigned long exp = (b >> 52) & 0x7FFUL;
    unsigned long mant = b & 0xFFFFFFFFFFFFFUL;
    return exp == 0x7FFUL && mant != 0UL;
}
static int t_is_pos_inf(double d) {
    return t_bits(d) == 0x7FF0000000000000UL;
}
static int t_is_neg_inf(double d) {
    return t_bits(d) == 0xFFF0000000000000UL;
}
static double t_abs(double d) {
    union { unsigned long u; double d; } v;
    v.u = t_bits(d) & 0x7FFFFFFFFFFFFFFFUL;
    return v.d;
}
// EN: Absolute-delta closeness -- used for "human scale" round-trip magnitudes, where `ftoa`'s
//     FIXED (not significant-figure) 6-decimal precision bounds the error in absolute terms
//     regardless of magnitude. `1e-6` is deliberately a touch looser than the exact
//     half-ULP-of-precision (5e-7) to leave room for the digit-extraction loop's own ordinary
//     floating-point rounding noise (empirically <=1e-12 for every value in the vector below,
//     see task report) -- still tight/meaningful, not a rubber stamp.
// PT: Proximidade por delta ABSOLUTO -- usado pras magnitudes de round-trip em "escala humana",
//     onde a precisao FIXA (nao por-digito-significativo) de 6 decimais do `ftoa` limita o erro
//     em termos absolutos independente da magnitude. `1e-6` e' deliberadamente um pouco mais
//     folgado que o meio-ULP-de-precisao exato (5e-7) pra deixar espaco pro ruido de
//     arredondamento de ponto flutuante comum do proprio laco de extracao de digito
//     (empiricamente <=1e-12 pra todo valor no vetor abaixo, ver relatorio da tarefa) -- ainda
//     assim apertado/significativo, nao um carimbo de borracha.
static int t_close_abs(double a, double b, double tol) {
    return t_abs(a - b) <= tol;
}
// EN: Relative-delta closeness -- used for the huge-magnitude round-trip cases (1e100..1e300),
//     where the double's own ~15-17 significant decimal digits (not `ftoa`'s fixed 6 fractional
//     digits) are the binding precision limit. `1e-13` leaves a comfortable ~100x margin over the
//     ~2e-15 typical relative error observed empirically in this range (task report) -- see
//     include/conv.h's "KNOWN EDGE BEHAVIOUR" note for why values EVEN CLOSER to `DBL_MAX`
//     (roughly the top decade, ~1e303 and up) are explicitly NOT exercised via this round-trip
//     helper (they can legitimately overflow to Infinity instead -- tested separately, on
//     purpose, below).
// PT: Proximidade por delta RELATIVO -- usado pros casos de round-trip de magnitude enorme
//     (1e100..1e300), onde os ~15-17 digitos decimais significativos do proprio double (nao os 6
//     digitos fracionarios FIXOS do `ftoa`) sao o limite de precisao que vale. `1e-13` deixa uma
//     margem confortavel de ~100x sobre o erro relativo tipico de ~2e-15 observado
//     empiricamente nessa faixa (relatorio da tarefa) -- ver a nota "COMPORTAMENTO DE FRONTEIRA
//     CONHECIDO" do include/conv.h pro motivo de valores AINDA MAIS PERTO de `DBL_MAX`
//     (aproximadamente a decada mais alta, ~1e303 pra cima) NAO serem exercitados de proposito
//     via este helper de round-trip (eles podem legitimamente estourar pra Infinito em vez disso
//     -- testado separadamente, de proposito, abaixo).
static int t_close_rel(double a, double b, double tol) {
    return t_abs(a - b) <= tol * t_abs(b);
}

// EN: AUD-SEC-Delta regression helper -- fills `buf` with "0." followed by `n` '9' characters,
//     NUL-terminated. Local test plumbing only (mirrors the `t_*` helpers above), not part of the
//     public API under test. Caller sizes `buf` to at least `n + 3` bytes.
// PT: Auxiliar de regressao do AUD-SEC-Delta -- preenche `buf` com "0." seguido de `n` caracteres
//     '9', terminado em NUL. Andaime de teste local apenas (espelha os helpers `t_*` acima), nao
//     faz parte da API publica sob teste. Chamador dimensiona `buf` com pelo menos `n + 3` bytes.
static void t_fill_point9(char* buf, int n) {
    buf[0] = '0';
    buf[1] = '.';
    for (int k = 0; k < n; k++) {
        buf[2 + k] = '9';
    }
    buf[2 + n] = '\0';
}

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

    // EN: E2 watch-item (ex-INBOX, closed here): a case right at the saturation BOUNDARY that
    //     must NOT saturate -- guards the `>` vs `>=` off-by-one in the saturating check
    //     (src/conv.c: `if (magnitude > (limit - digit) / 10)`). The implementation was already
    //     correct; this test only closes the missing regression coverage the D3 review flagged.
    //     `2147483640` is `INT_MAX` (2147483647) minus 7 -- comfortably below the ceiling, so a
    //     `>=` regression (which would incorrectly saturate values strictly less than the limit)
    //     would be caught here.
    // PT: Item-vigia do E2 (ex-INBOX, fechado aqui): um caso bem na FRONTEIRA de saturacao que
    //     NAO deve saturar -- guarda o off-by-one `>` vs `>=` na checagem saturante
    //     (src/conv.c: `if (magnitude > (limit - digit) / 10)`). A implementacao ja estava
    //     correta; este teste so fecha a cobertura de regressao que faltava, apontada pelo
    //     review do D3. `2147483640` e' `INT_MAX` (2147483647) menos 7 -- confortavelmente
    //     abaixo do teto, entao uma regressao pra `>=` (que saturaria incorretamente valores
    //     estritamente menores que o limite) seria pega aqui.
    {
        TEST_ASSERT_EQ(atoi("2147483640"), 2147483640);
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

    // EN: E2 watch-item (ex-INBOX, closed here): the same near-boundary guard as atoi's above,
    //     for atou's saturating check. `4294967290` is `UINT_MAX` (4294967295) minus 5.
    // PT: Item-vigia do E2 (ex-INBOX, fechado aqui): a mesma guarda de quase-fronteira do atoi
    //     acima, pra checagem saturante do atou. `4294967290` e' `UINT_MAX` (4294967295) menos 5.
    {
        TEST_ASSERT_EQ(atou("4294967290"), 4294967290u);
    }

    // ================================================================================================
    // SOV-FCONV: ftoa/atof (double<->string). See include/conv.h for the full contract, including
    // the explicit "NOT shortest-round-trip / not correctly-rounded" disclaimer and the "KNOWN EDGE
    // BEHAVIOUR" note this test suite is built around (not fighting against).
    // ================================================================================================

    // ---- ftoa: basics, sign of zero preserved -------------------------------------------------
    // EN: `-0.0` here is the actual negative-zero bit pattern (not just "a small negative
    //     number") -- self-checked via `t_sign` before trusting `ftoa`'s output, since a compiler
    //     could theoretically constant-fold `-0.0` differently (it does not, on this toolchain,
    //     but the self-check makes that an explicit, verified assumption rather than a silent
    //     one).
    // PT: `-0.0` aqui e' o padrao de bits de zero-negativo de verdade (nao so "um numero negativo
    //     pequeno") -- auto-checado via `t_sign` antes de confiar na saida do `ftoa`, ja que um
    //     compilador poderia teoricamente fazer constant-fold de `-0.0` diferente (nao faz, neste
    //     toolchain, mas o auto-check torna isso uma suposicao explicita e verificada, nao
    //     silenciosa).
    {
        double negzero = -0.0;
        TEST_ASSERT_EQ(t_sign(negzero), 1);
        TEST_ASSERT_EQ(t_sign(0.0), 0);

        char buf[CONV_FTOA_BUF_MIN];
        char* ret = ftoa(0.0, buf, 6);
        TEST_ASSERT_EQ((void*)ret, (void*)buf);
        TEST_ASSERT_EQ(strcmp(buf, "0.000000"), 0);
        ftoa(negzero, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "-0.000000"), 0); // sign PRESERVED, not collapsed to "0.000000"
    }

    // ---- ftoa: ordinary values, default precision ----------------------------------------------
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(3.14159265, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "3.141593"), 0); // rounds the 7th digit (5) up
    }
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(-42.0, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "-42.000000"), 0);
    }

    // ---- ftoa: half-away-from-zero rounding at precision 0 (0.5, 2.5/-2.5) --------------------
    // EN: `precision == 0` also omits the '.' entirely.
    // PT: `precision == 0` tambem omite o '.' por inteiro.
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(0.5, buf, 0);
        TEST_ASSERT_EQ(strcmp(buf, "1"), 0);
    }
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(2.5, buf, 0);
        TEST_ASSERT_EQ(strcmp(buf, "3"), 0);
    }
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(-2.5, buf, 0);
        TEST_ASSERT_EQ(strcmp(buf, "-3"), 0); // magnitude rounds the same way regardless of sign
    }

    // ---- ftoa: "x.xx5" halfway rounding at a non-zero precision (0.125 is EXACT in binary, so
    //     this isolates the rounding POLICY from any binary-representation ambiguity) -----------
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(0.125, buf, 2);
        TEST_ASSERT_EQ(strcmp(buf, "0.13"), 0);
    }

    // ---- ftoa: rounding CARRY propagates into a brand-new leading integer digit ----------------
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(9.6, buf, 0);
        TEST_ASSERT_EQ(strcmp(buf, "10"), 0);
    }
    {
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(99.95, buf, 1);
        TEST_ASSERT_EQ(strcmp(buf, "100.0"), 0); // carry ripples through BOTH integer digits
    }

    // ---- ftoa: Infinity / NaN, both signs of Infinity, NaN sign NOT represented on output ------
    {
        char buf[CONV_FTOA_BUF_MIN];
        double pos_inf = 1.0;
        double neg_inf = 1.0;
        // EN: Constructed via division-by-zero (well-defined IEEE-754 result on this ABI, no
        //     trap by default) rather than a raw bit-pattern literal here, to also exercise that
        //     ftoa correctly classifies an Infinity that did not come from conv.c's own internal
        //     constants.
        // PT: Construido via divisao-por-zero (resultado IEEE-754 bem definido nesta ABI, sem
        //     trap por padrao) em vez de um literal de padrao-de-bits cru aqui, pra tambem
        //     exercitar que o ftoa classifica corretamente um Infinito que nao veio das
        //     constantes internas do proprio conv.c.
        pos_inf = pos_inf / 0.0;
        neg_inf = -neg_inf / 0.0;
        TEST_ASSERT(t_is_pos_inf(pos_inf));
        TEST_ASSERT(t_is_neg_inf(neg_inf));

        ftoa(pos_inf, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "inf"), 0);
        ftoa(neg_inf, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "-inf"), 0);

        double nan_val = 0.0 / 0.0;
        TEST_ASSERT(t_is_nan(nan_val));
        ftoa(nan_val, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "nan"), 0); // sign bit (if any) NOT represented, see conv.h
    }

    // ---- ftoa: smallest positive denormal -- below any requested precision, prints as zero -----
    // EN: This is NOT a special-cased branch in ftoa -- it falls out naturally: `frac < 10^-6`
    //     means every one of the 6 requested fractional digits legitimately IS `0`. Sign is still
    //     preserved (mirrors the +-0.0 case).
    // PT: Isso NAO e' um branch de caso-especial no ftoa -- cai naturalmente: `frac < 10^-6`
    //     significa que cada um dos 6 digitos fracionarios pedidos legitimamente E' `0`. O sinal
    //     ainda e' preservado (espelha o caso +-0.0).
    {
        union { unsigned long u; double d; } v;
        v.u = 1UL; // smallest positive denormal (~4.9406564584124654e-324)
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(v.d, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "0.000000"), 0);
        double neg_denorm = -v.d;
        TEST_ASSERT_EQ(t_sign(neg_denorm), 1);
        ftoa(neg_denorm, buf, 6);
        TEST_ASSERT_EQ(strcmp(buf, "-0.000000"), 0);
    }

    // ---- ftoa: DBL_MAX -- correct sign/leading-digit prefix/length, no crash, no corruption ----
    // EN: Verifies `ftoa` handles the single largest finite `double` cleanly (right buffer size,
    //     right leading digits -- the ~15-17 significant digits a `double` can actually resolve).
    //     Does NOT assert a round-trip here (see the DEDICATED, EXPLICIT-overflow test right
    //     after this one, and include/conv.h's "KNOWN EDGE BEHAVIOUR" note for why).
    // PT: Verifica que o `ftoa` trata o maior `double` finito unico de forma limpa (tamanho de
    //     buffer certo, digitos iniciais certos -- os ~15-17 digitos significativos que um double
    //     de fato consegue resolver). NAO afirma um round-trip aqui (ver o teste DEDICADO,
    //     EXPLICITO-de-overflow logo depois deste, e a nota "COMPORTAMENTO DE FRONTEIRA
    //     CONHECIDO" do include/conv.h pro motivo).
    {
        double dbl_max = 1.7976931348623157e308;
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(dbl_max, buf, 6);
        TEST_ASSERT_EQ(buf[0], '1'); // no leading '-'
        TEST_ASSERT(strncmp(buf, "179769313486231", 15) == 0); // ~15 trustworthy leading digits
        TEST_ASSERT_EQ(strlen(buf), (size_t)(309 + 1 + 6)); // 309 int digits + '.' + 6 frac digits
        TEST_ASSERT(strlen(buf) < CONV_FTOA_BUF_MIN); // buffer-sizing macro's math holds, with room
    }

    // ---- atof: malformed input -> +0.0 (never a crash, sign never applied on this path) --------
    {
        TEST_ASSERT_EQ(atof(""), 0.0);
        TEST_ASSERT_EQ(t_sign(atof("")), 0);
        TEST_ASSERT_EQ(atof("."), 0.0);
        TEST_ASSERT_EQ(atof("e5"), 0.0);
        TEST_ASSERT_EQ(atof("--1"), 0.0);
        TEST_ASSERT_EQ(t_sign(atof("--1")), 0); // the FIRST '-' is consumed as a sign attempt,
                                                  // but the early "no digits" return always yields
                                                  // POSITIVE zero regardless -- see conv.h
    }
    // EN: "1e" -- lone trailing 'e' with no exponent digits is NOT consumed; the '1' before it
    //     WAS validly parsed, so the result is `1.0`, not `0.0` (trailing-garbage-ignored, same
    //     philosophy as atoi's "12abc" -> 12).
    // PT: "1e" -- 'e' final sozinho sem digitos de expoente NAO e' consumido; o '1' antes dele
    //     FOI parseado validamente, entao o resultado e' `1.0`, nao `0.0`
    //     (lixo-final-ignorado, mesma filosofia do "12abc" -> 12 do atoi).
    {
        TEST_ASSERT_EQ(atof("1e"), 1.0);
    }

    // ---- atof: basic decimal, whitespace/sign skip, trailing garbage ---------------------------
    {
        TEST_ASSERT(t_close_abs(atof("3.14159"), 3.14159, 1e-9));
        TEST_ASSERT(t_close_abs(atof("  +42.5abc"), 42.5, 1e-9));
    }

    // ---- atof: signed zero -- +-0.0 both reachable and distinguishable via a LEGITIMATE parse --
    {
        TEST_ASSERT_EQ(t_sign(atof("-0")), 1);
        TEST_ASSERT_EQ(t_sign(atof("-0.0")), 1);
        TEST_ASSERT_EQ(t_sign(atof("0")), 0);
        TEST_ASSERT_EQ(t_sign(atof("0.0")), 0);
    }

    // ---- atof: Infinity, case-insensitive, both spellings, both signs (fronteira "2 sentidos": -
    //     ftoa already checked above producing the strings; this checks atof CONSUMING them) ----
    {
        TEST_ASSERT(t_is_pos_inf(atof("inf")));
        TEST_ASSERT(t_is_pos_inf(atof("INF")));
        TEST_ASSERT(t_is_pos_inf(atof("Infinity")));
        TEST_ASSERT(t_is_pos_inf(atof("iNfInItY")));
        TEST_ASSERT(t_is_neg_inf(atof("-inf")));
        TEST_ASSERT(t_is_neg_inf(atof("-INFINITY")));
    }

    // ---- atof: NaN, case-insensitive, sign bit honoured on input (even though ftoa never emits
    //     a signed NaN on output -- see conv.h) --------------------------------------------------
    {
        TEST_ASSERT(t_is_nan(atof("nan")));
        TEST_ASSERT(t_is_nan(atof("NAN")));
        double neg_nan = atof("-nan");
        TEST_ASSERT(t_is_nan(neg_nan));
        TEST_ASSERT_EQ(t_sign(neg_nan), 1);
    }

    // ---- atof: exponent OVERFLOW saturates to signed Infinity (fronteira "DBL_MAX/overflow de
    //     expoente") -------------------------------------------------------------------------------
    {
        TEST_ASSERT(t_is_pos_inf(atof("1e400")));
        TEST_ASSERT(t_is_neg_inf(atof("-1e400")));
    }

    // ---- atof: AUD-SEC-Delta regression -- a pathological ALL-NINES fractional string must NEVER
    //     overflow to +Infinity nor round to `>= 1.0` (mathematically always `< 1.0` for any finite
    //     count of digits) -- proven BROKEN before this fix: `n=300` (well under the OLD
    //     400-digit cap, already wrong before the fix) returned a value `>= 1.0`; `n=309` (the
    //     exact string from the audit finding) and `n=400` (the old cap's own boundary) both
    //     returned `+Inf`. See `src/conv.c`'s `FCONV_MAX_SIG_FRAC_DIGITS` doc-comment for the fix
    //     mechanism (a separate, geometrically-bounded fractional accumulator that can never
    //     overflow by construction).
    // PT: Regressao do AUD-SEC-Delta -- uma string fracionaria TODA-NOVES patologica NUNCA deve
    //     estourar pra +Infinito nem arredondar pra `>= 1.0` (matematicamente sempre `< 1.0` pra
    //     qualquer contagem finita de digitos) -- provado QUEBRADO antes deste fix: `n=300`
    //     (confortavelmente abaixo do ANTIGO limite de 400 digitos, ja errado antes do fix)
    //     retornava um valor `>= 1.0`; `n=309` (a string exata do achado da auditoria) e `n=400`
    //     (a propria fronteira do antigo limite) retornavam `+Inf`. Ver o doc-comment do
    //     `FCONV_MAX_SIG_FRAC_DIGITS` no `src/conv.c` pro mecanismo do fix (um acumulador
    //     fracionario separado, limitado geometricamente, que nunca consegue estourar por
    //     construcao).
    {
        static const int ns[] = {300, 309, 400};
        char buf[410];
        for (size_t vi = 0; vi < sizeof(ns) / sizeof(ns[0]); vi++) {
            t_fill_point9(buf, ns[vi]);
            double v = atof(buf);
            TEST_ASSERT(!t_is_pos_inf(v));
            TEST_ASSERT(!t_is_nan(v));
            TEST_ASSERT(v < 1.0);  // THE invariant the audit finding demanded
            TEST_ASSERT(v > 0.99); // sanity: still a plausible "almost 1" value, not e.g. 0.0
            TEST_ASSERT_EQ(t_sign(v), 0); // no leading '-' in the input -> positive
        }
    }

    // ---- atof: AUD-SEC-Delta sibling case (bonus, found while designing the fix, NOT part of the
    //     original audit finding) -- a LARGE INTEGER part combined with a fractional part must also
    //     stay finite when it legitimately should: the OLD "multiply mantissa forward for
    //     fractional digits too" technique could push a near-DBL_MAX integer part over the edge to
    //     `+Infinity` purely from appending fractional digits, even though the true combined value
    //     is comfortably finite. 308 nines ~= 9.99...e307 (well below `DBL_MAX` ~1.798e308);
    //     appending ".5" must NOT flip the result to `+Infinity`.
    // PT: Caso irmao do AUD-SEC-Delta (bonus, encontrado ao desenhar o fix, NAO parte do achado
    //     original da auditoria) -- uma parte INTEIRA GRANDE combinada com uma parte fracionaria
    //     tambem deve ficar finita quando legitimamente deveria: a tecnica ANTIGA de "multiplicar
    //     mantissa pra frente tambem pros digitos fracionarios" conseguia empurrar uma parte
    //     inteira perto de `DBL_MAX` alem da borda pra `+Infinito` so' por anexar digitos
    //     fracionarios, mesmo o valor combinado verdadeiro sendo confortavelmente finito. 308
    //     noves ~= 9.99...e307 (bem abaixo de `DBL_MAX` ~1.798e308); anexar ".5" NAO deve virar o
    //     resultado pra `+Infinito`.
    {
        char buf[320];
        for (int k = 0; k < 308; k++) {
            buf[k] = '9';
        }
        buf[308] = '.';
        buf[309] = '5';
        buf[310] = '\0';
        double v = atof(buf);
        TEST_ASSERT(!t_is_pos_inf(v));
        TEST_ASSERT(v > 9.9e307); // still comfortably in the ~1e308 magnitude, not blown to Inf
    }

    // ---- atof: exponent UNDERFLOW -- best-effort signed zero/denormal, no crash ---------------
    {
        double u = atof("1e-400");
        TEST_ASSERT_EQ(u, 0.0);
        TEST_ASSERT_EQ(t_sign(u), 0); // no leading '-' in the input -> positive underflow
        double denorm = atof("4.9406564584124654e-324"); // smallest positive denormal, in decimal
        TEST_ASSERT(denorm > 0.0); // best-effort: recovered as a genuine (tiny) positive value
    }

    // ---- atof: "1e308"/"1e-308" boundary, parsed directly (exponent-scaling path, NOT a
    //     digit-string reparse -- this is the well-behaved direction, see include/conv.h) --------
    {
        TEST_ASSERT(t_close_rel(atof("1e308"), 1e308, 1e-13));
        TEST_ASSERT(t_close_rel(atof("-1e308"), -1e308, 1e-13));
        TEST_ASSERT(t_close_rel(atof("1e-308"), 1e-308, 1e-13));
    }

    // ---- ftoa(DBL_MAX) round-tripped through atof: the DOCUMENTED, DETERMINISTIC top-of-range
    //     overflow from include/conv.h's "KNOWN EDGE BEHAVIOUR" note -- asserted EXPLICITLY as
    //     the real, understood outcome (not silently tolerated, not swept under the rug). This is
    //     the honest complement to the "round-trip vector" test below, which deliberately stays
    //     clear of this top-decade zone. -------------------------------------------------------
    {
        double dbl_max = 1.7976931348623157e308;
        char buf[CONV_FTOA_BUF_MIN];
        ftoa(dbl_max, buf, 6);
        double back = atof(buf);
        TEST_ASSERT(t_is_pos_inf(back)); // documented, deterministic -- see conv.h
    }

    // ---- round-trip vector: atof(ftoa(x)) close to x, "human scale" magnitudes (absolute
    //     delta) + huge-but-SAFE magnitudes up to 1e300 (relative delta) -- see t_close_abs/
    //     t_close_rel above for the tolerance rationale. Deliberately EXCLUDES the top decade
    //     below DBL_MAX (~1e303+), which is covered by the dedicated overflow test above instead.
    // -----------------------------------------------------------------------------------------
    {
        static const double human_vec[] = {
            3.14159265, 123.456, -42.0, 0.0001, 1000000.5, 2.5, -2.5, 0.125, 9999.9999, 0.0, -0.0,
        };
        for (size_t vi = 0; vi < sizeof(human_vec) / sizeof(human_vec[0]); vi++) {
            char buf[CONV_FTOA_BUF_MIN];
            ftoa(human_vec[vi], buf, 6);
            double back = atof(buf);
            TEST_ASSERT(t_close_abs(back, human_vec[vi], 1e-6));
        }

        static const double huge_vec[] = {1e100, 1e200, 1e250, 1e300, -1e300};
        for (size_t vi = 0; vi < sizeof(huge_vec) / sizeof(huge_vec[0]); vi++) {
            char buf[CONV_FTOA_BUF_MIN];
            ftoa(huge_vec[vi], buf, 6);
            double back = atof(buf);
            TEST_ASSERT(!t_is_pos_inf(back) && !t_is_neg_inf(back)); // still in the safe zone
            TEST_ASSERT(t_close_rel(back, huge_vec[vi], 1e-13));
        }
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
