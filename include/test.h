// SPDX-License-Identifier: MPL-2.0
// EN: C1 -- the sovereign runtime's own test harness (no libc, no `assert.h`). A single-header
//     minimalist assert-macro: on failure, writes a human-readable "FAIL: <file>:<line>: <expr>"
//     message to stderr (fd 2) via sys_write (B6) and aborts immediately via sys_exit(1) (B5) --
//     first-failure-aborts, no accumulation (approved scope, anti-over-engineering). On success
//     it does nothing at runtime (no output, no branch taken beyond the `if`) -- a passing test
//     that only calls TEST_ASSERT* writes NOTHING to any fd and exits via its own explicit
//     `sys_exit(0)`/TEST_PASS() at the end, matching the Makefile harness contract
//     (tests/expected_exit.txt: "<name> <expected-code>", pass == expected exit code).
//
//     The line-number problem: we cannot format `__LINE__` (an integer) into the message at
//     RUNTIME because there is no `itoa`/integer-to-string primitive yet (D3, not built --
//     TODO.md). The fix is two-level C preprocessor STRINGIZE (`#` operator applied twice):
//     `__LINE__` first macro-expands to its numeric token (e.g. `42`), and a second-level `#`
//     then stringizes THAT expanded token into the string literal `"42"`. A single-level `#`
//     would instead stringize the literal text `__LINE__` itself (wrong). This is resolved
//     ENTIRELY by the compiler's preprocessor, at compile time -- zero runtime cost, zero libc.
//     The whole failure message is therefore just adjacent string-literal concatenation (a C
//     language feature, not a printf-style format) of `__FILE__`, the stringized `__LINE__`,
//     and the stringized condition expression (`#cond`, one-level -- it is never macro-expanded
//     itself, so one level of `#` is correct there).
//
//     TEST_ASSERT_EQ(a, b) is sugar for `TEST_ASSERT((a) == (b))` -- it does NOT print the
//     actual runtime VALUES of `a`/`b` on mismatch (e.g. "expected 4, got 5"). Doing that would
//     require converting an arbitrary integer to a string at runtime, i.e. `itoa`, which is D3
//     (libc-core, not yet built -- see TODO.md). Until D3 lands, the failure message only shows
//     the SOURCE EXPRESSION (`a == b`) and the exact file:line -- sufficient to locate the
//     failing assertion in the source; the developer re-derives the values by reading the code
//     at that line or attaching a debugger (gdb, per CLAUDE.md "Inspecao / debug").
// PT: C1 -- o harness de teste proprio da runtime soberana (sem libc, sem `assert.h`). Uma
//     assert-macro minimalista em header unico: em falha, escreve uma mensagem legivel
//     "FAIL: <arquivo>:<linha>: <expr>" no stderr (fd 2) via sys_write (B6) e aborta
//     imediatamente via sys_exit(1) (B5) -- aborta na 1a falha, sem acumular (escopo aprovado,
//     anti over-engineering). Em sucesso nao faz nada em runtime (nenhuma saida, nenhum branch
//     tomado alem do `if`) -- um teste que passa e so chama TEST_ASSERT* nao escreve NADA em
//     fd nenhum e sai via seu proprio `sys_exit(0)`/TEST_PASS() explicito no final, batendo com
//     o contrato do harness do Makefile (tests/expected_exit.txt: "<nome> <codigo-esperado>",
//     passar == sair com o codigo esperado).
//
//     O problema do numero de linha: nao podemos formatar `__LINE__` (um inteiro) na mensagem
//     em RUNTIME porque nao existe ainda primitiva `itoa`/inteiro-para-string (D3, nao
//     construida -- TODO.md). A solucao e' STRINGIZE de dois niveis do pre-processador C
//     (operador `#` aplicado duas vezes): `__LINE__` primeiro se expande macro para seu token
//     numerico (ex.: `42`), e um segundo nivel de `#` entao stringiza ESSE token ja expandido
//     no literal de string `"42"`. Um `#` de nivel unico stringizaria em vez disso o texto
//     literal `__LINE__` (errado). Isso e' resolvido INTEIRAMENTE pelo pre-processador do
//     compilador, em tempo de compilacao -- custo zero em runtime, zero libc. A mensagem de
//     falha inteira e', portanto, so' concatenacao de literais de string adjacentes (um recurso
//     da linguagem C, nao um formato estilo printf) de `__FILE__`, o `__LINE__` stringizado, e
//     a expressao da condicao stringizada (`#cond`, nivel unico -- ela mesma nunca e'
//     macro-expandida, entao um nivel de `#` e' correto ali).
//
//     TEST_ASSERT_EQ(a, b) e' acucar pra `TEST_ASSERT((a) == (b))` -- NAO imprime os VALORES
//     de runtime de `a`/`b` em caso de divergencia (ex.: "esperado 4, obtive 5"). Fazer isso
//     exigiria converter um inteiro arbitrario pra string em runtime, ou seja, `itoa`, que e' o
//     D3 (libc-nucleo, ainda nao construido -- ver TODO.md). Ate o D3 chegar, a mensagem de
//     falha so mostra a EXPRESSAO-FONTE (`a == b`) e o file:linha exato -- suficiente pra
//     localizar a asserção que falhou no fonte; o desenvolvedor re-deriva os valores lendo o
//     codigo naquela linha ou anexando um debugger (gdb, conforme CLAUDE.md "Inspecao / debug").
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"
#include "sys_write.h"
#include "sys_exit.h"

// EN: Two-level stringize idiom. GLX_STR2 is the raw `#` (stringizes its argument's literal
//     text, unexpanded); GLX_STR forces one macro-expansion pass on `x` BEFORE handing it to
//     GLX_STR2 -- this indirection is what lets GLX_STR(__LINE__) yield "42" instead of the
//     literal text "__LINE__". Prefixed `GLX_` (glintfx/loucura shared convention) to avoid
//     colliding with any future/third-party `STR`/`STR2` macro.
// PT: Idioma de stringize em dois niveis. GLX_STR2 e' o `#` cru (stringiza o texto literal do
//     argumento, sem expandir); GLX_STR forca uma passada de macro-expansao em `x` ANTES de
//     repassar pro GLX_STR2 -- essa indirecao e' o que permite GLX_STR(__LINE__) resultar em
//     "42" em vez do texto literal "__LINE__". Prefixo `GLX_` (convencao compartilhada
//     glintfx/loucura) pra evitar colisao com alguma futura macro `STR`/`STR2` de terceiros.
#define GLX_STR2(x) #x
#define GLX_STR(x)  GLX_STR2(x)

// EN: Core assertion. `do { ... } while (0)` makes it a single statement (safe after `if` with
//     no braces, safe with a trailing `;`). The message is `static const char[]` -- a single
//     compile-time-sized array living in .rodata, no runtime string-building, no allocation.
//     `sizeof(_glx_msg) - 1` drops the implicit trailing '\0' the compiler appends to every
//     string literal (sys_write takes an exact byte count, not a NUL-terminated C string --
//     this project has no `strlen` (D2) to compute it from a raw `char*` even if we wanted to).
// PT: Asserção nucleo. `do { ... } while (0)` a torna uma unica instrucao (segura depois de
//     `if` sem chaves, segura com `;` no final). A mensagem e' `static const char[]` -- um
//     array de tamanho fixo em tempo de compilacao, morando em .rodata, sem construcao de
//     string em runtime, sem alocacao. `sizeof(_glx_msg) - 1` descarta o '\0' final implicito
//     que o compilador acrescenta a todo literal de string (sys_write recebe uma contagem
//     exata de bytes, nao uma C string terminada em NUL -- este projeto nao tem `strlen` (D2)
//     pra calcular isso a partir de um `char*` cru mesmo que quisessemos).
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            static const char _glx_msg[] = \
                "FAIL: " __FILE__ ":" GLX_STR(__LINE__) ": " #cond "\n"; \
            sys_write(2, _glx_msg, sizeof(_glx_msg) - 1); \
            sys_exit(1); \
        } \
    } while (0)

// EN: Equality sugar. See file header: does NOT print the actual values of `a`/`b` (needs
//     `itoa`, D3, not built yet) -- only the source expression and file:line.
// PT: Acucar de igualdade. Ver cabecalho do arquivo: NAO imprime os valores reais de `a`/`b`
//     (precisaria de `itoa`, D3, ainda nao construida) -- so a expressao-fonte e o file:linha.
#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

// EN: Optional end-of-test sugar: writes "PASS: <file>\n" to stdout (fd 1) and exits 0. Purely
//     cosmetic (the Makefile harness only checks the exit code, per tests/expected_exit.txt) --
//     a bare `sys_exit(0);` is equally valid; use whichever reads better at each test's tail.
// PT: Acucar opcional de fim-de-teste: escreve "PASS: <arquivo>\n" no stdout (fd 1) e sai 0.
//     Puramente cosmetico (o harness do Makefile so checa o exit code, conforme
//     tests/expected_exit.txt) -- um `sys_exit(0);` seco e' igualmente valido; use o que ler
//     melhor no fim de cada teste.
#define TEST_PASS() \
    do { \
        static const char _glx_pass_msg[] = "PASS: " __FILE__ "\n"; \
        sys_write(1, _glx_pass_msg, sizeof(_glx_pass_msg) - 1); \
        sys_exit(0); \
    } while (0)
