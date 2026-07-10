// SPDX-License-Identifier: MPL-2.0
// EN: E2 -- the sovereign runtime's own minimal `printf` (no libc, no `<stdio.h>`). Named
//     `printf.h` (not `stdio.h`): same "replace what it provides, not the header name"
//     convention as D3's `conv.h`. Supports exactly `%d %u %x %s %c %f %%` -- no width/precision/
//     flags/length modifiers (YAGNI, same philosophy as D3's "only base 10/16" call: nothing in
//     this codebase needs more yet; extend when a real need appears). `%f` (SOV-FCONV, added
//     later) is built directly on D3's `ftoa` at its DEFAULT precision (6) -- see conv.h. `%g` is
//     deliberately NOT implemented: it would need scientific-notation formatting `ftoa` does not
//     provide (fixed-point only); registered as a TODO.md follow-up rather than built here.
//
//     THE `<stdarg.h>` BOUNDARY DECISION: a variadic function's `...` can only be walked via
//     `va_list`/`va_start`/`va_arg`/`va_end`, which are COMPILER BUILTINS (`__builtin_va_*` under
//     the hood) -- there is no way to hand-roll them in portable C, they depend on the target
//     ABI's calling convention (which registers/stack slots hold which argument, System V AMD64
//     in our case) in a way only the compiler front-end knows. `<stdarg.h>` is one of the
//     handful of headers the C standard GUARANTEES exist in a FREESTANDING implementation
//     (alongside `<stddef.h>`, `<float.h>`, `<limits.h>`, `<stdint.h>`) precisely because they
//     are compiler-provided, not linked from a runtime library -- unlike `<stdio.h>`/
//     `<string.h>`/`<stdlib.h>`, there is no `.so`/`.a` object backing `<stdarg.h>`, so including
//     it does NOT violate "zero libc": nothing gets linked in, it only unlocks compiler
//     intrinsics. This mirrors how `syscall` is our irreducible boundary for I/O -- `<stdarg.h>`
//     is the irreducible boundary for variadic functions. We do NOT attempt to reimplement
//     `va_arg` (impossible without the compiler).
//
//     TESTABILITY DESIGN (why there are two functions, not one): the Makefile's test harness
//     (tests/expected_exit.txt) only checks a program's EXIT CODE, it does not capture stdout --
//     so a `mini_printf` that only writes to fd 1 cannot be asserted against in TDD without
//     shelling out to `od -c` by hand. The fix: split the format LOGIC from the I/O SINK.
//     `mini_vsnprintf` formats into a caller-owned buffer (like C's `vsnprintf`) -- fully
//     deterministic and `strcmp`-able from tests/test_printf.c, zero I/O. `mini_printf` is a
//     thin variadic wrapper that reuses the SAME formatting core (src/printf.c's internal
//     `mini_format_core`, sink-based -- see that file) but streams to `sys_write` instead of a
//     bounded buffer, so it does NOT truncate long output (unlike `mini_vsnprintf`, which is
//     buffer-bounded like real `vsnprintf`). This is the "(b)" option from the task brief.
//
//     BUFFER/FLUSH DESIGN for `mini_printf` (src/printf.c): output accumulates in a 256-byte
//     static buffer local to the call; `sys_write` fires only when that buffer FILLS UP or at
//     the very end (NOT one syscall per character, NOT per format directive) -- a single `%s`
//     longer than 256 bytes correctly triggers a mid-string flush-and-continue, never overflows
//     the buffer and never truncates the output (see src/printf.c for the loop).
// PT: E2 -- o `printf` minimo proprio da runtime soberana (sem libc, sem `<stdio.h>`). Nomeado
//     `printf.h` (nao `stdio.h`): mesma convencao "substitui o que fornece, nao o nome do
//     header" do `conv.h` do D3. Suporta exatamente `%d %u %x %s %c %%` -- sem
//     largura/precisao/flags/modificador-de-tamanho (YAGNI, mesma filosofia do "so base 10/16"
//     do D3: nada neste codebase precisa de mais por enquanto; estender quando uma necessidade
//     real aparecer).
//
//     A DECISAO DE FRONTEIRA do `<stdarg.h>`: o `...` de uma funcao variadica so pode ser
//     percorrido via `va_list`/`va_start`/`va_arg`/`va_end`, que sao BUILTINS DO COMPILADOR
//     (`__builtin_va_*` por baixo) -- nao ha como implementar isso a mao em C portavel, eles
//     dependem da convencao de chamada da ABI do alvo (quais registradores/slots de pilha
//     guardam qual argumento, System V AMD64 no nosso caso) de um jeito que so o front-end do
//     compilador conhece. `<stdarg.h>` e' um dos poucos headers que o padrao C GARANTE existir
//     numa implementacao FREESTANDING (junto de `<stddef.h>`, `<float.h>`, `<limits.h>`,
//     `<stdint.h>`) precisamente porque sao fornecidos pelo compilador, nao linkados de uma
//     biblioteca de runtime -- diferente de `<stdio.h>`/`<string.h>`/`<stdlib.h>`, nao ha
//     `.so`/`.a` por tras de `<stdarg.h>`, entao incluir ele NAO viola "zero libc": nada e'
//     linkado, so' destrava intrinsecos do compilador. Isso espelha como `syscall` e' nossa
//     fronteira irredutivel pra I/O -- `<stdarg.h>` e' a fronteira irredutivel pra funcoes
//     variadicas. NAO tentamos reimplementar `va_arg` (impossivel sem o compilador).
//
//     DESIGN DE TESTABILIDADE (por que existem duas funcoes, nao uma): o harness de teste do
//     Makefile (tests/expected_exit.txt) so checa o EXIT CODE de um programa, nao captura
//     stdout -- entao um `mini_printf` que so escreve no fd 1 nao pode ser verificado em TDD sem
//     recorrer a `od -c` na mao. A solucao: separar a LOGICA de formatacao do SINK de I/O.
//     `mini_vsnprintf` formata num buffer de posse do chamador (como o `vsnprintf` de C) --
//     totalmente deterministico e comparavel via `strcmp` a partir de tests/test_printf.c, zero
//     I/O. `mini_printf` e' um wrapper variadico fino que reusa o MESMO nucleo de formatacao
//     (`mini_format_core` interno de src/printf.c, baseado em sink -- ver aquele arquivo) mas
//     transmite pro `sys_write` em vez de um buffer limitado, entao NAO trunca saida longa
//     (diferente do `mini_vsnprintf`, que e' limitado por buffer como o `vsnprintf` de verdade).
//     Essa e' a opcao "(b)" do brief da tarefa.
//
//     DESIGN DE BUFFER/FLUSH pro `mini_printf` (src/printf.c): a saida acumula num buffer
//     estatico de 256 bytes local a chamada; o `sys_write` dispara so quando esse buffer ENCHE
//     ou no fim de tudo (NAO uma syscall por caractere, NAO por diretiva de formato) -- um `%s`
//     sozinho maior que 256 bytes corretamente dispara um flush-e-continua no meio da string,
//     nunca estoura o buffer e nunca trunca a saida (ver src/printf.c pro laco).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "types.h"
#include <stdarg.h>

// EN: Formats `fmt` + `ap` into `buf` (which must have room for `cap` bytes). Writes at most
//     `cap - 1` formatted bytes plus a terminating `\0` (i.e. never writes outside `buf[0..cap)`)
//     -- same truncation contract as C99's `vsnprintf`. If `cap == 0`, `buf` is NEVER
//     dereferenced (may be NULL) and nothing is written anywhere; the return value is still
//     computed. Returns the number of characters that WOULD have been produced if `cap` were
//     unbounded (NOT counting the terminating `\0`) -- exactly the C99 `vsnprintf` convention,
//     so callers can detect truncation via `return_value >= cap`. Conversions: `%d` (signed
//     decimal, reads `va_arg(ap, int)`), `%u`/`%x` (unsigned decimal/lowercase-hex, read
//     `va_arg(ap, unsigned)`), `%s` (C-string, reads `va_arg(ap, const char*)`; `NULL` prints
//     the literal "(null)"), `%c` (one char, reads `va_arg(ap, int)` -- NOT `char`, see file
//     header's default-argument-promotion note), `%f` (fixed-point `double`, reads `va_arg(ap,
//     double)`, always at D3's `CONV_FTOA_DEFAULT_PRECISION` -- see conv.h's `ftoa` doc-comment
//     for special-value/rounding behaviour), `%%` (one literal `%`). An unknown directive
//     (e.g. `%z`) prints the `%` and the following character literally and consumes no
//     variadic argument. A lone trailing `%` at the very end of `fmt` prints a literal `%`.
// PT: Formata `fmt` + `ap` em `buf` (que deve ter espaco pra `cap` bytes). Escreve no maximo
//     `cap - 1` bytes formatados mais um `\0` terminador (ou seja, nunca escreve fora de
//     `buf[0..cap)`) -- mesmo contrato de truncamento do `vsnprintf` do C99. Se `cap == 0`,
//     `buf` NUNCA e' desreferenciado (pode ser NULL) e nada e' escrito em lugar nenhum; o valor
//     de retorno ainda e' computado. Retorna o numero de caracteres que TERIAM sido produzidos
//     se `cap` fosse ilimitado (NAO contando o `\0` terminador) -- exatamente a convencao do
//     `vsnprintf` do C99, entao quem chama pode detectar truncamento via
//     `valor_retorno >= cap`. Conversoes: `%d` (decimal com sinal, le `va_arg(ap, int)`),
//     `%u`/`%x` (decimal/hex-minusculo sem sinal, leem `va_arg(ap, unsigned)`), `%s` (C-string,
//     le `va_arg(ap, const char*)`; `NULL` imprime o literal "(null)"), `%c` (um char, le
//     `va_arg(ap, int)` -- NAO `char`, ver nota de promocao-de-argumento-padrao no cabecalho do
//     arquivo), `%f` (`double` de ponto fixo, le `va_arg(ap, double)`, sempre na
//     `CONV_FTOA_DEFAULT_PRECISION` do D3 -- ver o doc-comment do `ftoa` em conv.h pro
//     comportamento de valor-especial/arredondamento), `%%` (um `%` literal). Uma diretiva
//     desconhecida (ex.: `%z`) imprime o `%` e o
//     caractere seguinte literalmente e nao consome argumento variadico nenhum. Um `%` sozinho
//     no fim de `fmt` imprime um `%` literal.
int mini_vsnprintf(char* buf, size_t cap, const char* fmt, va_list ap);

// EN: Formats `fmt` + `...` and writes the result directly to stdout (fd 1) via `sys_write`,
//     internally buffered (see file header for the flush design -- NOT one syscall per
//     character, NOT truncated regardless of length). Same conversion set/semantics as
//     `mini_vsnprintf`. Returns the number of bytes ACTUALLY written to stdout (the sum of
//     `sys_write`'s successful returns across however many internal flushes happened) -- this is
//     a real I/O byte count, NOT the "would-be logical length" `mini_vsnprintf` returns.
// PT: Formata `fmt` + `...` e escreve o resultado diretamente no stdout (fd 1) via `sys_write`,
//     bufferizado internamente (ver cabecalho do arquivo pro design de flush -- NAO uma syscall
//     por caractere, NAO truncado independente do tamanho). Mesmo conjunto/semantica de
//     conversao do `mini_vsnprintf`. Retorna o numero de bytes REALMENTE escritos no stdout (a
//     soma dos retornos de sucesso do `sys_write` ao longo de quantos flushes internos
//     aconteceram) -- e' uma contagem de bytes de I/O real, NAO o "tamanho logico que seria"
//     retornado pelo `mini_vsnprintf`.
int mini_printf(const char* fmt, ...);
