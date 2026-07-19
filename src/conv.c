// SPDX-License-Identifier: MPL-2.0
// EN: D3 -- see include/conv.h for the contract. Digit-at-a-time implementations, same
//     philosophy as D1 (mem.c)/D2 (str.c): correctness and auditability first, no
//     table-driven/SIMD tricks. (SOV-FCONV, appended later: `ftoa`/`atof`, double<->string --
//     own dedicated file-header block further down, right before their code.)
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

// ================================================================================================
// SOV-FCONV: float<->string (double only). See include/conv.h for the full contract; this block
// covers the IMPLEMENTATION-level gotchas the header does not (buffer layout, bit-pattern
// constants, the two internal DoS-defensive iteration caps).
//
// EN: `unsigned long` is used as the 64-bit bit-punning type throughout (never a named `uint64_t`
//     -- this codebase has no `<stdint.h>`-equivalent typedef for it; `types.h` documents x86-64
//     Linux as LP64, so `unsigned long` IS 8 bytes here, same basis as `types.h`'s own
//     `uintptr_t`). All double<->bits reinterpretation goes through a `union { double d; unsigned
//     long u; }` (type-punning via a union member, not a pointer cast) -- this is the
//     standard-sanctioned way to reinterpret an object's representation in C (a pointer-cast
//     `*(unsigned long*)&value` would be a strict-aliasing violation, UB).
// PT: `unsigned long` e' usado como o tipo de bit-punning de 64 bits em todo este bloco (nunca um
//     `uint64_t` nomeado -- este codebase nao tem um typedef equivalente a `<stdint.h>` pra isso;
//     o `types.h` documenta x86-64 Linux como LP64, entao `unsigned long` TEM 8 bytes aqui, mesma
//     base do proprio `uintptr_t` do `types.h`). Toda reinterpretacao double<->bits passa por uma
//     `union { double d; unsigned long u; }` (type-punning via membro de union, nao cast de
//     ponteiro) -- e' o jeito sancionado pelo padrao de reinterpretar a representacao de um objeto
//     em C (um cast de ponteiro `*(unsigned long*)&value` seria violacao de strict-aliasing, UB).

#define FCONV_SIGN_MASK 0x8000000000000000UL
#define FCONV_EXP_MASK  0x7FF0000000000000UL
#define FCONV_MANT_MASK 0x000FFFFFFFFFFFFFUL
#define FCONV_EXP_SHIFT 52

#define FCONV_POS_INF_BITS 0x7FF0000000000000UL
#define FCONV_NEG_INF_BITS 0xFFF0000000000000UL
#define FCONV_NAN_BITS      0x7FF8000000000000UL // quiet NaN, unsigned

// EN: Two DIFFERENT kinds of cap live here -- do not conflate them (AUD-SEC-Delta finding, see
//     below): `FCONV_MAX_SCALE_ITER`/`FCONV_MAX_EXP_MAG` are pure DoS-defensive work-bounds (both
//     are FAR beyond any exponent a legitimate finite/denormal double could need -- usable double
//     decimal exponent range is roughly [-324, 308] -- so neither ever changes the result for real
//     input, they only bound worst-case work for a hostile string like "1e999999999"). But
//     `FCONV_MAX_SIG_FRAC_DIGITS` (below) is a PRECISION cap, not (only) a DoS one: it DOES change
//     the result for real input whenever a fractional part has more than ~16 significant digits --
//     it deliberately drops digits a `double` cannot resolve anyway (see the macro's own
//     doc-comment right above its definition for the corrected reasoning; the previous revision of
//     this comment claimed NO cap here ever changes the result, which was empirically FALSE for
//     the old, now-replaced fractional-accumulation technique -- `atof("0." + "9"*309)` returned
//     `+Inf` instead of a value `< 1.0`, proven live before this fix).
// PT: Dois tipos DIFERENTES de limite moram aqui -- nao conflar os dois (achado AUD-SEC-Delta, ver
//     abaixo): `FCONV_MAX_SCALE_ITER`/`FCONV_MAX_EXP_MAG` sao limites de trabalho puramente
//     anti-DoS (os dois estao BEM alem de qualquer expoente que um double finito/denormal
//     legitimo precisaria -- a faixa util de expoente decimal de um double e' aproximadamente
//     [-324, 308] -- entao nenhum dos dois muda o resultado pra input real, so' limitam o trabalho
//     de pior-caso pra uma string hostil tipo "1e999999999"). Mas o `FCONV_MAX_SIG_FRAC_DIGITS`
//     (abaixo) e' um limite de PRECISAO, nao (so') anti-DoS: ELE MUDA o resultado pra input real
//     sempre que uma parte fracionaria tem mais de ~16 digitos significativos -- descarta de
//     proposito digitos que um `double` nao consegue resolver de qualquer jeito (ver o
//     doc-comment do proprio macro logo acima da sua definicao pro raciocinio corrigido; a revisao
//     anterior deste comentario afirmava que NENHUM limite aqui muda o resultado, o que era
//     empiricamente FALSO pra tecnica antiga (agora substituida) de acumulacao fracionaria --
//     `atof("0." + "9"*309)` retornava `+Inf` em vez de um valor `< 1.0`, provado ao vivo antes
//     deste fix).
#define FCONV_MAX_SCALE_ITER  400
#define FCONV_MAX_EXP_MAG     1000000

// EN: AUD-SEC-Delta fix -- `FCONV_MAX_SIG_FRAC_DIGITS` caps how many fractional digits are ever
//     FOLDED into the running fractional accumulator (`frac_value` in `atof`, below); digits past
//     this cap are still CONSUMED (advance the parse position, same "trailing garbage does not
//     invalidate what was parsed" spirit as the rest of this file) but simply never added.
//     16 is not an arbitrary round number: it is the largest N for which the worst-case fractional
//     string "9" * N -- the value closest to the 1.0 boundary a fractional accumulator can ever
//     produce -- still rounds to a `double` STRICTLY below 1.0. Empirically verified (see task
//     report) across n in {300, 309, 400} nines and across `-O0`/`-O2`/`-ffp-contract=fast|off`:
//     N=16 always yields ~0.9999999999999999 (< 1.0); N=17 already rounds UP to exactly 1.0 (the
//     mathematically "correctly rounded" answer for the true limit of the underlying geometric
//     series -- see below -- but this file's `atof` is explicitly NOT a correctly-rounded
//     converter, and the contract promised to `include/conv.h`'s callers is "never >= 1.0 for an
//     input that is mathematically < 1.0"). This lines up with the well-known fact that a `double`
//     has only ~15-17 significant decimal digits of real precision (`DBL_DIG` == 15 is the
//     ALWAYS-safe count; 16-17 is the boundary where behaviour starts depending on the specific
//     digit pattern) -- 16 sits deliberately on the safe side of that boundary, not the risky one.
// PT: Fix AUD-SEC-Delta -- `FCONV_MAX_SIG_FRAC_DIGITS` limita quantos digitos fracionarios sao
//     alguma vez DOBRADOS no acumulador fracionario corrente (`frac_value` no `atof`, abaixo);
//     digitos alem desse limite ainda sao CONSUMIDOS (avancam a posicao de parse, mesmo espirito
//     "lixo final nao invalida o que foi parseado" do resto deste arquivo) mas simplesmente nunca
//     somados.
//     16 nao e' um numero redondo arbitrario: e' o maior N pro qual a string fracionaria de
//     pior-caso "9" * N -- o valor mais proximo da fronteira de 1.0 que um acumulador fracionario
//     consegue produzir -- ainda arredonda pra um `double` ESTRITAMENTE abaixo de 1.0.
//     Verificado empiricamente (ver relatorio da tarefa) pra n em {300, 309, 400} noves e pra
//     `-O0`/`-O2`/`-ffp-contract=fast|off`: N=16 sempre da ~0.9999999999999999 (< 1.0); N=17 ja
//     arredonda PRA CIMA pra exatamente 1.0 (a resposta matematicamente "corretamente arredondada"
//     pro limite verdadeiro da serie geometrica subjacente -- ver abaixo -- mas o `atof` deste
//     arquivo explicitamente NAO e' um conversor corretamente-arredondado, e o contrato prometido
//     aos chamadores do `include/conv.h` e' "nunca >= 1.0 pra um input que e' matematicamente <
//     1.0"). Isso bate com o fato conhecido de que um `double` so tem ~15-17 digitos decimais
//     significativos de precisao real (`DBL_DIG` == 15 e' a contagem SEMPRE segura; 16-17 e' a
//     fronteira onde o comportamento passa a depender do padrao especifico de digitos) -- 16 fica
//     deliberadamente do lado seguro dessa fronteira, nao do arriscado.
#define FCONV_MAX_SIG_FRAC_DIGITS 16

static unsigned long fconv_bits(double d) {
    union { double d; unsigned long u; } v;
    v.d = d;
    return v.u;
}

static double fconv_from_bits(unsigned long u) {
    union { unsigned long u; double d; } v;
    v.u = u;
    return v.d;
}

// EN: Truncates `x` toward zero (like C's `trunc()`, hand-rolled -- no libm). Works for any sign
//     (never called with negative input in this file, but kept general/correct regardless).
//     Pure bit manipulation on the IEEE-754 binary64 layout: for `|x| < 1.0` the answer is a
//     SIGNED zero (sign bit copied from `x`, everything else cleared) -- this is what makes
//     `dbl_trunc(-0.0) == -0.0` and, combined with unary negation elsewhere in this file,
//     preserves the `-0.0` sign end to end. For `|x| >= 2^52` there are no fractional MANTISSA
//     bits to clear (a double's 52-bit mantissa cannot represent a fraction at that magnitude), so
//     `x` is already integral and is returned unchanged. Otherwise, exactly the low
//     `(52 - unbiased_exponent)` mantissa bits are cleared -- this NEVER touches the sign or
//     exponent bits (both live above bit 52), so the sign-preservation above holds regardless of
//     which branch is taken.
// PT: Trunca `x` em direcao a zero (como o `trunc()` de C, feito a mao -- sem libm). Funciona pra
//     qualquer sinal (nunca chamado com input negativo neste arquivo, mas mantido
//     geral/correto mesmo assim). Manipulacao pura de bits sobre o layout IEEE-754 binary64: pra
//     `|x| < 1.0` a resposta e' um zero COM SINAL (bit de sinal copiado de `x`, resto zerado) --
//     e' isso que faz `dbl_trunc(-0.0) == -0.0` e, combinado com negacao unaria em outro ponto
//     deste arquivo, preserva o sinal de `-0.0` ponta a ponta. Pra `|x| >= 2^52` nao ha bits
//     fracionarios de MANTISSA pra limpar (a mantissa de 52 bits de um double nao consegue
//     representar uma fracao nessa magnitude), entao `x` ja e' integral e e' retornado sem
//     mudanca. Fora isso, exatamente os `(52 - expoente_nao_enviesado)` bits baixos da mantissa
//     sao zerados -- isso NUNCA toca os bits de sinal ou expoente (ambos moram acima do bit 52),
//     entao a preservacao de sinal acima vale independente de qual branch e' tomado.
static double dbl_trunc(double x) {
    unsigned long bits = fconv_bits(x);
    unsigned long sign = bits & FCONV_SIGN_MASK;
    long exp = (long)((bits & FCONV_EXP_MASK) >> FCONV_EXP_SHIFT) - 1023L;

    if (exp < 0) {
        // |x| < 1.0 (also covers subnormals and +-0.0) -- truncates to a SIGNED zero.
        return fconv_from_bits(sign);
    }
    if (exp >= 52) {
        // No fractional mantissa bits exist at this magnitude (also covers Inf/NaN, though
        // dbl_trunc is never invoked on those in this file's call sites).
        return x;
    }
    unsigned long frac_mask = (1UL << (unsigned)(52 - exp)) - 1UL;
    return fconv_from_bits(bits & ~frac_mask);
}

char* ftoa(double value, char* buf, unsigned precision) {
    unsigned long bits = fconv_bits(value);
    unsigned long sign_bit = (bits & FCONV_SIGN_MASK) ? 1UL : 0UL;
    unsigned long exp_field = bits & FCONV_EXP_MASK;
    unsigned long mant_field = bits & FCONV_MANT_MASK;

    char* p = buf;

    // EN: Infinity/NaN classification -- our own bit-pattern check, never libm's isinf/isnan.
    // PT: Classificacao de Infinito/NaN -- nossa propria checagem de padrao-de-bits, nunca o
    //     isinf/isnan da libm.
    if (exp_field == FCONV_EXP_MASK) {
        if (mant_field == 0UL) {
            if (sign_bit) {
                *p++ = '-';
            }
            *p++ = 'i';
            *p++ = 'n';
            *p++ = 'f';
        } else {
            // NaN -- sign not represented on output, see include/conv.h doc-comment.
            *p++ = 'n';
            *p++ = 'a';
            *p++ = 'n';
        }
        *p = '\0';
        return buf;
    }

    if (precision > CONV_FTOA_MAX_PRECISION) {
        precision = CONV_FTOA_MAX_PRECISION; // documented clamp, see include/conv.h
    }

    // EN: `av` is the ABSOLUTE VALUE of `value`, obtained via unary negation (never `0.0 - value`,
    //     which would give the WRONG answer for value==-0.0: `0.0 - (-0.0) == 0.0`, correct by
    //     luck there, but `0.0 - (+0.0) == 0.0` too, and more importantly `-x` is the operation
    //     IEEE-754/C actually guarantees flips exactly the sign bit for every input including
    //     zero/Inf/NaN -- `0.0 - x` is a SUBTRACTION with its own rounding rules, not a guaranteed
    //     sign flip). Sign is written from `sign_bit` (the bit pattern), never from `value < 0.0`
    //     (false for both +0.0 and -0.0).
    // PT: `av` e' o VALOR ABSOLUTO de `value`, obtido via negacao unaria (nunca `0.0 - value`, que
    //     daria a resposta ERRADA pra value==-0.0: `0.0 - (-0.0) == 0.0`, correto por sorte ali,
    //     mas `0.0 - (+0.0) == 0.0` tambem, e mais importante `-x` e' a operacao que
    //     IEEE-754/C de fato garante que inverte exatamente o bit de sinal pra todo input incluindo
    //     zero/Inf/NaN -- `0.0 - x` e' uma SUBTRACAO com suas proprias regras de arredondamento,
    //     nao uma inversao de sinal garantida). Sinal e' escrito a partir de `sign_bit` (o padrao
    //     de bits), nunca de `value < 0.0` (falso pros dois +0.0 e -0.0).
    double av = value;
    if (sign_bit) {
        av = -av;
        *p++ = '-';
    }

    double ip = dbl_trunc(av);
    double frac = av - ip;

    // ---- integer part digits, least-significant-first (mirrors utoa's shape) -----------------
    unsigned char idigits[CONV_FTOA_MAX_INT_DIGITS + 1u];
    size_t n_int = 0;
    if (ip == 0.0) {
        idigits[n_int++] = 0;
    } else {
        while (ip != 0.0 && n_int < CONV_FTOA_MAX_INT_DIGITS) {
            double q = dbl_trunc(ip / 10.0);
            double d = ip - q * 10.0;
            // EN: THE mandated guard -- validate/clamp the double BEFORE any cast to an integer
            //     type. Mathematically `d` is always in [0,9], but this defends against any
            //     residual floating-point rounding pushing it a hair outside that range (e.g.
            //     -epsilon or 9+epsilon) -- without this clamp, such a value cast to
            //     `unsigned char` would be undefined behaviour (a double outside the target
            //     integer type's representable range is UB to cast, per the task's own mandate).
            // PT: A guarda MANDATADA -- validar/limitar o double ANTES de qualquer cast pra tipo
            //     inteiro. Matematicamente `d` esta sempre em [0,9], mas isso defende contra
            //     algum arredondamento residual de ponto flutuante empurrando ele um pouco pra
            //     fora dessa faixa (ex.: -epsilon ou 9+epsilon) -- sem essa limitacao, um valor
            //     assim convertido pra `unsigned char` seria comportamento indefinido (um double
            //     fora da faixa representavel do tipo inteiro alvo e' UB pra converter, conforme
            //     o proprio mandato da tarefa).
            if (d < 0.0) {
                d = 0.0;
            } else if (d > 9.0) {
                d = 9.0;
            }
            idigits[n_int++] = (unsigned char)d;
            ip = q;
        }
    }

    // ---- fractional digits + one extra ROUNDING GUARD digit ----------------------------------
    unsigned char fdigits[CONV_FTOA_MAX_PRECISION + 1u];
    for (unsigned k = 0; k <= precision; k++) {
        frac *= 10.0;
        double d = dbl_trunc(frac);
        if (d < 0.0) { // same validate-before-cast guard as above
            d = 0.0;
        } else if (d > 9.0) {
            d = 9.0;
        }
        fdigits[k] = (unsigned char)d;
        frac -= d;
    }

    // ---- round HALF-AWAY-FROM-ZERO using the guard digit, propagate carry --------------------
    int carry = (fdigits[precision] >= 5) ? 1 : 0;
    for (long k = (long)precision - 1; k >= 0 && carry; k--) {
        unsigned char nv = (unsigned char)(fdigits[k] + 1u);
        if (nv == 10u) {
            fdigits[k] = 0;
            carry = 1;
        } else {
            fdigits[k] = nv;
            carry = 0;
        }
    }
    if (carry) {
        for (size_t k = 0; k < n_int && carry; k++) {
            unsigned char nv = (unsigned char)(idigits[k] + 1u);
            if (nv == 10u) {
                idigits[k] = 0;
                carry = 1;
            } else {
                idigits[k] = nv;
                carry = 0;
            }
        }
        if (carry && n_int < CONV_FTOA_MAX_INT_DIGITS + 1u) {
            // EN: All existing integer digits rolled over (e.g. "9.9995" at precision 3) -- one
            //     new most-significant digit ('1') is appended at the END of this
            //     least-significant-first array; after the MSD-first write loop below, it
            //     correctly becomes the FIRST character printed.
            // PT: Todos os digitos inteiros existentes deram carry (ex.: "9.9995" na precisao 3)
            //     -- um novo digito mais-significativo ('1') e' anexado no FIM deste array
            //     menos-significativo-primeiro; depois do laco de escrita
            //     mais-significativo-primeiro abaixo, ele corretamente vira o PRIMEIRO caractere
            //     impresso.
            idigits[n_int++] = 1;
        }
    }

    // ---- write integer digits MSD-first -------------------------------------------------------
    for (size_t k = n_int; k > 0; k--) {
        *p++ = (char)('0' + idigits[k - 1]);
    }

    // ---- write fractional digits (no '.' at all when precision == 0, matches C's "%.0f") ------
    if (precision > 0) {
        *p++ = '.';
        for (unsigned k = 0; k < precision; k++) {
            *p++ = (char)('0' + fdigits[k]);
        }
    }

    *p = '\0';
    return buf;
}

// EN: Case-insensitive literal match: does `s[i..]` start with `lit` (a NUL-terminated, all-
//     lowercase literal), comparing case-insensitively? Returns the number of characters matched
//     (== `strlen(lit)`) on success, 0 on the first mismatch (including running into `s`'s own
//     NUL terminator early, which simply fails to match any non-NUL `lit` character and returns 0
//     -- no out-of-bounds read: reading exactly `s`'s NUL byte is always safe, and this function
//     never reads past the position where it detects a mismatch or reaches the NUL).
// PT: Casamento de literal case-insensitivo: `s[i..]` comeca com `lit` (um literal terminado em
//     NUL, todo minusculo), comparando sem diferenciar maiusculas/minusculas? Retorna o numero de
//     caracteres casados (== `strlen(lit)`) em sucesso, 0 no primeiro descasamento (inclusive
//     esbarrar no proprio NUL terminador de `s` cedo, que simplesmente falha em casar qualquer
//     caractere nao-NUL de `lit` e retorna 0 -- sem leitura fora dos limites: ler exatamente o
//     byte NUL de `s` e' sempre seguro, e essa funcao nunca le alem da posicao onde detecta um
//     descasamento ou alcanca o NUL).
static int fconv_ci_match(const char* s, size_t i, const char* lit) {
    size_t n = 0;
    while (lit[n] != '\0') {
        char c = s[i + n];
        char lc = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
        if (lc != lit[n]) {
            return 0;
        }
        n++;
    }
    return (int)n;
}

double atof(const char* s) {
    size_t i = 0;
    // EN: Same whitespace set as atoi/atou.
    // PT: Mesmo conjunto de whitespace do atoi/atou.
    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\v' || s[i] == '\f' ||
           s[i] == '\r') {
        i++;
    }

    int neg = 0;
    if (s[i] == '+') {
        i++;
    } else if (s[i] == '-') {
        neg = 1;
        i++;
    }

    // EN: "inf" / "infinity", case-insensitive -- and "nan", case-insensitive. Neither branch
    //     advances `i` after the match: `atof` never reports a "chars consumed" position back to
    //     the caller (same contract as `atoi`/`atou`), so tracking it further would be dead code
    //     (correctly flagged by `check-static`'s cppcheck pass in an earlier revision of this
    //     function -- fixed by simply not writing to `i` here at all). This is also WHY "infinity"
    //     needs no dedicated extra check beyond matching the "inf" prefix: the remaining "inity"
    //     is just trailing garbage that is silently ignored (same "12abc" -> 12 philosophy as
    //     `atoi`), which already produces the exact same return value either way.
    // PT: "inf" / "infinity", case-insensitivo -- e "nan", case-insensitivo. Nenhum dos dois
    //     branches avanca `i` depois do casamento: `atof` nunca reporta uma posicao de
    //     "caracteres consumidos" de volta pro chamador (mesmo contrato do `atoi`/`atou`), entao
    //     continuar rastreando isso seria codigo morto (corretamente sinalizado pela passada de
    //     cppcheck do `check-static` numa revisao anterior desta funcao -- corrigido simplesmente
    //     nao escrevendo em `i` aqui). Isso tambem e' o PORQUE de "infinity" nao precisar de
    //     nenhuma checagem extra dedicada alem de casar o prefixo "inf": o "inity" restante e'
    //     so' lixo final silenciosamente ignorado (mesma filosofia "12abc" -> 12 do `atoi`), que
    //     ja produz exatamente o mesmo valor de retorno de qualquer jeito.
    if (fconv_ci_match(s, i, "inf")) {
        return fconv_from_bits(neg ? FCONV_NEG_INF_BITS : FCONV_POS_INF_BITS);
    }
    if (fconv_ci_match(s, i, "nan")) {
        unsigned long nan_bits = FCONV_NAN_BITS | (neg ? FCONV_SIGN_MASK : 0UL);
        return fconv_from_bits(nan_bits);
    }

    // ---- decimal digits (integer part) ------------------------------------------------------
    double mantissa = 0.0;
    int any_digits = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        mantissa = mantissa * 10.0 + (double)(s[i] - '0');
        i++;
        any_digits = 1;
    }

    // ---- optional '.' + fractional digits (capped, see FCONV_MAX_SIG_FRAC_DIGITS above) -------
    // EN: AUD-SEC-Delta fix -- fractional digits accumulate into their OWN `frac_value`, via a
    //     SEPARATE, monotonically-shrinking `frac_scale` (`*= 0.1` each digit), instead of being
    //     folded into `mantissa` (the integer-part accumulator) via the same "multiply by 10, add
    //     digit" shape used for INTEGER digits above. This is a deliberately different technique
    //     from the integer-part loop, and here is why the difference matters: the OLD code used
    //     the identical `mantissa = mantissa*10 + digit` formula for fractional digits too, only
    //     rescaling by `10^frac_digits` at the very END (in the exponent-scale step below) --
    //     which meant a fractional part with hundreds of digits temporarily inflated `mantissa` as
    //     if those digits were ADDITIONAL INTEGER digits, overflowing `mantissa` to `+Infinity`
    //     WAY BEFORE the later rescale ever had a chance to divide it back down (proven live:
    //     `atof("0." + "9"*309)` returned `+Inf` instead of a value `< 1.0`, and even `"9"*300`
    //     -- comfortably under the old 400-digit cap -- already returned `1.0000...e+00`, i.e.
    //     `>= 1.0`, which is mathematically wrong for an input that is a string of all-9s
    //     fractional digits, always `< 1.0`). The fix here accumulates `frac_value` DIRECTLY as
    //     the true fractional quantity (each digit contributes `digit * frac_scale`, already
    //     correctly scaled, no later rescale needed) -- since `frac_scale` shrinks geometrically
    //     and every term added is bounded by the current `frac_scale`, `frac_value` itself is
    //     mathematically bounded by the convergent series `sum(9 * 0.1^k) = 1.0` and can NEVER
    //     overflow a `double`, no matter how many digits are fed in (this also, as a side
    //     benefit, fixes the SIBLING case the old technique never handled either -- a LARGE
    //     integer part combined with ANY fractional digits, e.g. `"9"*308 + ".5"`, which the old
    //     "multiply mantissa forward by 10 for each frac digit" technique could overflow even
    //     though the true final value is comfortably finite). `mantissa` is combined with
    //     `frac_value` by simple ADDITION once both are fully accumulated (see just below the
    //     exponent-parsing block), never by further multiplication -- addition of a finite `double`
    //     and a `[0.0, 1.0)`-bounded `double` cannot itself overflow unless `mantissa` was already
    //     (legitimately) `+Infinity` from the integer-part loop, in which case `Infinity + x ==
    //     Infinity` is the CORRECT IEEE-754 answer, not a bug.
    // PT: Fix AUD-SEC-Delta -- digitos fracionarios acumulam no PROPRIO `frac_value`, via uma
    //     `frac_scale` SEPARADA e monotonicamente decrescente (`*= 0.1` a cada digito), em vez de
    //     serem dobrados no `mantissa` (o acumulador da parte inteira) via a mesma forma
    //     "multiplica por 10, soma digito" usada pros digitos INTEIROS acima. Essa e' uma tecnica
    //     deliberadamente diferente do laco da parte inteira, e eis o porque a diferenca importa: o
    //     codigo ANTIGO usava a formula identica `mantissa = mantissa*10 + digito` tambem pros
    //     digitos fracionarios, so' reescalonando por `10^frac_digits` no FINAL (no passo de
    //     escala-por-expoente abaixo) -- o que significava que uma parte fracionaria com centenas
    //     de digitos inflava temporariamente o `mantissa` como se esses digitos fossem DIGITOS
    //     INTEIROS ADICIONAIS, estourando o `mantissa` pra `+Infinito` BEM ANTES do reescalonamento
    //     posterior ter chance de dividir ele de volta (provado ao vivo: `atof("0." + "9"*309)`
    //     retornava `+Inf` em vez de um valor `< 1.0`, e ate `"9"*300` -- confortavelmente abaixo
    //     do antigo limite de 400 digitos -- ja retornava `1.0000...e+00`, ou seja `>= 1.0`, o que
    //     e' matematicamente errado pra um input que e' uma string de digitos fracionarios todos-9,
    //     sempre `< 1.0`). O fix aqui acumula `frac_value` DIRETAMENTE como a quantidade fracionaria
    //     verdadeira (cada digito contribui `digito * frac_scale`, ja corretamente escalonado, sem
    //     precisar de reescalonamento depois) -- como `frac_scale` encolhe geometricamente e todo
    //     termo somado e' limitado pela `frac_scale` corrente, o proprio `frac_value` e'
    //     matematicamente limitado pela serie convergente `soma(9 * 0.1^k) = 1.0` e NUNCA consegue
    //     estourar um `double`, nao importa quantos digitos sejam alimentados (isso tambem, como
    //     beneficio colateral, corrige o caso IRMAO que a tecnica antiga tambem nunca tratava --
    //     uma parte inteira GRANDE combinada com QUALQUER digito fracionario, ex.: `"9"*308 +
    //     ".5"`, que a tecnica antiga de "multiplicar mantissa pra frente por 10 a cada digito
    //     fracionario" conseguia estourar mesmo o valor final verdadeiro sendo confortavelmente
    //     finito). O `mantissa` e' combinado com `frac_value` por SOMA simples uma vez que os dois
    //     estejam totalmente acumulados (ver logo abaixo do bloco de parse do expoente), nunca por
    //     mais multiplicacao -- somar um `double` finito com um `double` limitado a `[0.0, 1.0)`
    //     nao consegue estourar por si so', a menos que `mantissa` ja fosse (legitimamente)
    //     `+Infinito` vindo do laco da parte inteira, caso em que `Infinito + x == Infinito` e' a
    //     resposta CORRETA do IEEE-754, nao um bug.
    double frac_value = 0.0;
    if (s[i] == '.') {
        i++;
        double frac_scale = 1.0;
        int frac_sig_digits = 0; // scope reduced to this block only -- unused past it (cppcheck)
        while (s[i] >= '0' && s[i] <= '9') {
            if (frac_sig_digits < FCONV_MAX_SIG_FRAC_DIGITS) {
                frac_scale *= 0.1;
                frac_value += (double)(s[i] - '0') * frac_scale;
                frac_sig_digits++;
            }
            // EN: Digits beyond the cap are still CONSUMED (they are syntactically part of the
            //     number) but no longer accumulated -- see FCONV_MAX_SIG_FRAC_DIGITS above and
            //     include/conv.h's doc-comment: past ~16 significant digits, a `double` cannot
            //     resolve the difference anyway, so folding them in would be pure wasted work
            //     (were it even safe to do so, which the old technique was not).
            // PT: Digitos alem do limite ainda sao CONSUMIDOS (sao sintaticamente parte do
            //     numero) mas nao mais acumulados -- ver FCONV_MAX_SIG_FRAC_DIGITS acima e o
            //     doc-comment do include/conv.h: alem de ~16 digitos significativos, um `double`
            //     nao consegue resolver a diferenca de qualquer jeito, entao dobra-los seria
            //     puro trabalho desperdicado (mesmo que fosse seguro fazer isso, o que a tecnica
            //     antiga nao era).
            i++;
            any_digits = 1;
        }
    }

    if (!any_digits) {
        // EN: No digits anywhere -- "", ".", "e5" (no mantissa before 'e'), "--1" (second sign
        //     is not part of a valid number, so the digit scan never even starts) all land here.
        //     ALWAYS +0.0, regardless of any sign character consumed -- see doc-comment.
        // PT: Nenhum digito em lugar nenhum -- "", ".", "e5" (sem mantissa antes do 'e'), "--1"
        //     (segundo sinal nao faz parte de um numero valido, entao a varredura de digito nem
        //     comeca) todos caem aqui. SEMPRE +0.0, independente de qualquer caractere de sinal
        //     consumido -- ver doc-comment.
        return 0.0;
    }

    // ---- optional [eE][+-]digits exponent -----------------------------------------------------
    int explicit_exp = 0;
    if (s[i] == 'e' || s[i] == 'E') {
        size_t j = i + 1;
        int exp_neg = 0;
        if (s[j] == '+') {
            j++;
        } else if (s[j] == '-') {
            exp_neg = 1;
            j++;
        }
        if (s[j] >= '0' && s[j] <= '9') {
            int exp_mag = 0;
            while (s[j] >= '0' && s[j] <= '9') {
                int d = s[j] - '0';
                // EN: Saturating accumulation (same shape as atoi/atou's technique) -- caps well
                //     beyond FCONV_MAX_SCALE_ITER so the later scale loop's own cap is always
                //     what actually binds, this just keeps `exp_mag` itself from overflowing
                //     `int` for a pathologically long digit run.
                // PT: Acumulacao saturante (mesma forma da tecnica do atoi/atou) -- limita bem
                //     alem do FCONV_MAX_SCALE_ITER entao o proprio limite do laco de escala mais
                //     adiante e' sempre o que de fato vale, isso so evita que o `exp_mag` em si
                //     estoure `int` pra uma sequencia de digitos patologicamente longa.
                if (exp_mag > (FCONV_MAX_EXP_MAG - d) / 10) {
                    exp_mag = FCONV_MAX_EXP_MAG;
                } else {
                    exp_mag = exp_mag * 10 + d;
                }
                j++;
            }
            explicit_exp = exp_neg ? -exp_mag : exp_mag;
            // EN: `i` is intentionally NOT updated to `j` here -- nothing past this point in the
            //     function ever reads `i` again (same "atof does not report a consumed-length
            //     position" reasoning as the inf/nan branches above; `explicit_exp`, already
            //     captured, is the only thing that mattered from this scan).
            // PT: `i` deliberadamente NAO e' atualizado pra `j` aqui -- nada dali em diante na
            //     funcao le `i` de novo (mesmo raciocinio "atof nao reporta uma posicao de
            //     comprimento-consumido" dos branches de inf/nan acima; `explicit_exp`, ja
            //     capturado, e' a unica coisa que importava dessa varredura).
        }
        // EN: else: a lone trailing 'e'/'E' with no exponent digits is simply never matched by
        //     the `if` above -- it is trailing garbage after the already-valid mantissa (see
        //     include/conv.h: `atof("1e")` -> `1.0`).
        // PT: senao: um 'e'/'E' final sozinho sem digitos de expoente simplesmente nunca e'
        //     casado pelo `if` acima -- e' lixo final depois da mantissa ja valida (ver
        //     include/conv.h: `atof("1e")` -> `1.0`).
    }

    // EN: Combine the integer part (`mantissa`) and the already-correctly-scaled fractional part
    //     (`frac_value`, `[0.0, 1.0)`-bounded, see loop above) by plain ADDITION -- this is the
    //     ONE place the two accumulators meet, and it is deliberately NOT a multiplication, so it
    //     cannot itself introduce a new overflow (see the long comment above the fractional loop
    //     for why). `total_exp` is now JUST `explicit_exp`: unlike the old code, it no longer needs
    //     to subtract `frac_digits`, because `frac_value` was already scaled digit-by-digit as it
    //     accumulated (no deferred rescale left to do here).
    // PT: Combina a parte inteira (`mantissa`) e a parte fracionaria ja-corretamente-escalonada
    //     (`frac_value`, limitada a `[0.0, 1.0)`, ver laco acima) por SOMA simples -- esse e' o
    //     UNICO lugar onde os dois acumuladores se encontram, e deliberadamente NAO e' uma
    //     multiplicacao, entao nao consegue introduzir um novo overflow por si so' (ver o
    //     comentario longo acima do laco fracionario pro porque). `total_exp` agora e' SO'
    //     `explicit_exp`: diferente do codigo antigo, nao precisa mais subtrair `frac_digits`,
    //     porque `frac_value` ja foi escalonado digito-a-digito conforme acumulou (nao sobra
    //     reescalonamento adiado pra fazer aqui).
    mantissa += frac_value;
    int total_exp = explicit_exp;

    if (neg) {
        // EN: Unary negation, same "-0.0 from +0.0" correctness reasoning as ftoa's `av`.
        // PT: Negacao unaria, mesmo raciocinio de corretude "-0.0 a partir de +0.0" do `av` do
        //     ftoa.
        mantissa = -mantissa;
    }

    // ---- scale by 10^total_exp, bounded iteration count (DoS-defensive, see macros above) ----
    if (total_exp > 0) {
        unsigned n_iter = (unsigned)total_exp;
        if (n_iter > FCONV_MAX_SCALE_ITER) {
            n_iter = FCONV_MAX_SCALE_ITER;
        }
        for (unsigned k = 0; k < n_iter; k++) {
            mantissa *= 10.0; // overflows to +-Infinity NATURALLY via IEEE-754, no branch needed
        }
    } else if (total_exp < 0) {
        unsigned n_iter = (unsigned)(-total_exp);
        if (n_iter > FCONV_MAX_SCALE_ITER) {
            n_iter = FCONV_MAX_SCALE_ITER;
        }
        for (unsigned k = 0; k < n_iter; k++) {
            mantissa /= 10.0; // underflows to a signed 0.0/denormal NATURALLY, gradual underflow
        }
    }

    return mantissa;
}
