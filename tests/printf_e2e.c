// SPDX-License-Identifier: MPL-2.0
// EN: AUD-C0-3 gate program (AUDIT_FIND.md): the only automated test that exercises
//     `mini_printf`'s REAL stdout path -- `mini_flush`/`mini_flush_sink` (src/printf.c:265-293)
//     -- end to end. Before this program, `tests/test_printf.c` covered only `mini_vsnprintf`
//     (its buffer-formatting twin); the flush-to-fd-1 loop that fires when a single `mini_printf`
//     call's accumulated output crosses the 256-byte scratch buffer had ZERO test coverage (both
//     `include/printf.h` and `tests/test_printf.c` document this gap explicitly -- see their file
//     headers -- because the OLD `make test` harness (tests/expected_exit.txt) only checked exit
//     codes, never stdout). This program is the payload half of the fix; the Makefile's `test`
//     target now also diffs a program's captured stdout against a golden file when one is
//     registered in tests/expected_stdout.txt (see that file and the Makefile).
//
//     WHY 10x A 32-BYTE BLOCK, NOT ONE GIANT STRING: a single `%s` longer than 256 bytes already
//     forces mini_flush_sink's inner `while (n > 0)` loop to iterate more than once (the SAME
//     loop this test targets), but it would only prove the "one run bigger than the whole
//     buffer" sub-case. Ten separate `%s` conversions of the SAME format call instead exercise
//     the more common real-world shape -- many SMALL runs accumulating in `ctx->used` across
//     several `sink()` invocations within one `mini_printf()` call -- and, by construction below,
//     land the accumulation EXACTLY on the 256-byte boundary mid-run (see the byte accounting
//     below), forcing a flush inside the loop followed by a second, final flush after
//     `mini_format_core` returns. That is TWO real `sys_write` calls from ONE `mini_printf` call
//     -- the ">=2 flushes" AUD-C0-3 asks for -- while the resulting BYTES on stdout are
//     indistinguishable from a single contiguous write (the golden file is a plain byte diff,
//     agnostic to how many syscalls produced them).
//
//     BYTE ACCOUNTING (32-byte block, 10 repeats, + 1 trailing '\n', scratch cap = 256):
//     runs 1-7 accumulate 7*32=224 bytes (ctx->used=224, no flush yet, room=256-224=32);
//     run 8 (32 bytes) exactly fills the remaining 32 bytes of room -> ctx->used hits 256 ->
//     mini_flush_sink's own `if (ctx->used == sizeof(scratch))` check fires INSIDE the loop,
//     via sys_write(1, scratch, 256) -- a 256-byte flush containing blocks 1-8 in full;
//     runs 9-10 (64 bytes) + the literal "\n" (1 byte) accumulate into the now-empty scratch ->
//     ctx->used=65 when mini_format_core returns; mini_printf's own trailing `mini_flush(&ctx)`
//     call fires the SECOND sys_write(1, scratch, 65) with blocks 9-10 + the newline.
//     Total: 321 logical bytes (10*32 + 1), 321 bytes actually written (`mini_printf`'s return
//     value == the concatenated length of everything sent, since none of it is truncated -- the
//     stdout path never truncates, unlike `mini_vsnprintf`), across exactly 2 `sys_write` calls.
//
//     The internal invariant (`written == 321`) is checked FIRST via TEST_ASSERT_EQ (aborts to
//     fd 2 on failure, per include/test.h -- never touches fd 1) so a broken byte count fails
//     loudly before the golden-diff step even runs. The program deliberately uses a bare
//     `sys_exit(0)` at the end (NOT `TEST_PASS()`, which itself writes "PASS: ..." to fd 1) --
//     the whole point of this gate is that stdout contains EXACTLY the 321 bytes `mini_printf`
//     produced, nothing more, so the external `cmp` against tests/golden/printf_e2e.expected
//     (wired in the Makefile via tests/expected_stdout.txt) is byte-exact.
// PT: Programa-gate do AUD-C0-3 (AUDIT_FIND.md): o unico teste automatizado que exercita o
//     caminho REAL de stdout do `mini_printf` -- `mini_flush`/`mini_flush_sink`
//     (src/printf.c:265-293) -- ponta a ponta. Antes deste programa, `tests/test_printf.c`
//     cobria so o `mini_vsnprintf` (seu gemeo de formatacao em buffer); o laco de flush pro fd 1
//     que dispara quando a saida acumulada de UMA chamada de `mini_printf` cruza o buffer de
//     rascunho de 256 bytes tinha ZERO cobertura de teste (tanto `include/printf.h` quanto
//     `tests/test_printf.c` documentam essa lacuna explicitamente -- ver os cabecalhos deles --
//     porque o harness ANTIGO do `make test` (tests/expected_exit.txt) so checava exit codes,
//     nunca stdout). Este programa e' a metade "payload" do fix; o alvo `test` do Makefile agora
//     tambem faz diff do stdout capturado de um programa contra um golden file quando um esta
//     registrado em tests/expected_stdout.txt (ver aquele arquivo e o Makefile).
//
//     POR QUE 10x UM BLOCO DE 32 BYTES, NAO UMA STRING GIGANTE: um unico `%s` maior que 256
//     bytes ja forca o laco interno `while (n > 0)` do mini_flush_sink a iterar mais de uma vez
//     (o MESMO laco que este teste mira), mas so provaria o subcaso "uma leva maior que o buffer
//     inteiro". Dez conversoes `%s` separadas na MESMA chamada de formato exercitam em vez disso
//     a forma mais comum no mundo real -- varias levas PEQUENAS acumulando em `ctx->used` ao
//     longo de varias invocacoes de `sink()` dentro de uma unica chamada `mini_printf()` -- e,
//     por construcao abaixo, fazem a acumulacao cair EXATAMENTE na fronteira de 256 bytes no meio
//     de uma leva (ver a contabilidade de bytes abaixo), forcando um flush dentro do laco seguido
//     de um segundo flush final apos o `mini_format_core` retornar. Isso e' DUAS chamadas reais
//     de `sys_write` numa UNICA chamada `mini_printf` -- o ">=2 flushes" que o AUD-C0-3 pede --
//     enquanto os BYTES resultantes no stdout sao indistinguiveis de uma unica escrita continua
//     (o golden file e' um diff de bytes puro, agnostico a quantas syscalls os produziram).
//
//     CONTABILIDADE DE BYTES (bloco de 32 bytes, 10 repeticoes, + 1 '\n' final, cap do
//     scratch = 256): as levas 1-7 acumulam 7*32=224 bytes (ctx->used=224, sem flush ainda,
//     room=256-224=32); a leva 8 (32 bytes) preenche exatamente os 32 bytes restantes de room ->
//     ctx->used chega a 256 -> o proprio check `if (ctx->used == sizeof(scratch))` do
//     mini_flush_sink dispara DENTRO do laco, via sys_write(1, scratch, 256) -- um flush de 256
//     bytes contendo os blocos 1-8 inteiros; as levas 9-10 (64 bytes) + o literal "\n" (1 byte)
//     acumulam no scratch agora vazio -> ctx->used=65 quando o mini_format_core retorna; a propria
//     chamada final `mini_flush(&ctx)` do mini_printf dispara o SEGUNDO sys_write(1, scratch, 65)
//     com os blocos 9-10 + a quebra de linha. Total: 321 bytes logicos (10*32 + 1), 321 bytes
//     realmente escritos (o valor de retorno do `mini_printf` == o tamanho concatenado de tudo
//     que foi enviado, ja que nada e' truncado -- o caminho de stdout nunca trunca, diferente do
//     `mini_vsnprintf`), ao longo de exatamente 2 chamadas `sys_write`.
//
//     O invariante interno (`written == 321`) e' checado PRIMEIRO via TEST_ASSERT_EQ (aborta pro
//     fd 2 em falha, conforme include/test.h -- nunca toca o fd 1) entao uma contagem de bytes
//     quebrada falha ruidosamente antes mesmo do passo de diff-com-golden rodar. O programa usa
//     deliberadamente um `sys_exit(0)` seco no final (NAO `TEST_PASS()`, que ele mesmo escreve
//     "PASS: ..." no fd 1) -- o ponto inteiro deste gate e' que o stdout contenha EXATAMENTE os
//     321 bytes que o `mini_printf` produziu, nada mais, entao o `cmp` externo contra
//     tests/golden/printf_e2e.expected (ligado no Makefile via tests/expected_stdout.txt) e'
//     byte-exato.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "printf.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // EN: Exactly 32 bytes (26 letters + 6 digits) -- see file header's byte accounting, which
    //     depends on this exact length.
    // PT: Exatamente 32 bytes (26 letras + 6 digitos) -- ver a contabilidade de bytes do
    //     cabecalho do arquivo, que depende deste tamanho exato.
    static const char block[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    int written = mini_printf("%s%s%s%s%s%s%s%s%s%s\n", block, block, block, block, block, block,
                               block, block, block, block);

    // EN: 10 * 32 (block) + 1 ('\n') = 321 -- see file header's byte accounting.
    // PT: 10 * 32 (block) + 1 ('\n') = 321 -- ver a contabilidade de bytes do cabecalho do
    //     arquivo.
    TEST_ASSERT_EQ(written, 321);

    sys_exit(0);
    return 0; // unreachable -- sys_exit() above never returns
}
