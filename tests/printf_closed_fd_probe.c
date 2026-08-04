// SPDX-License-Identifier: Apache-2.0
// EN: C0-PRINTF-SW (TODO.md, W26) -- PINS the CURRENT, deliberately-NOT-hardened contract of
//     `mini_printf`'s stdout error path with an executable test, instead of leaving that
//     decision documented only in prose (src/printf.c's file header, `mini_flush`'s own
//     comment, ADR-0002). This program does NOT change `mini_printf`'s behaviour and does NOT
//     add a retry loop -- see TODO.md's `C0-PRINTF-SW` entry ("Minor, YAGNI, no active
//     trigger... harden only if a real consumer needs write-completeness guarantees") for why
//     hardening is explicitly OUT of scope here.
//
//     HOW THE CLOSED FD IS PRODUCED WITHOUT LIBC: this project has no `close()` wrapper yet
//     (Linux x86-64 `close` is syscall number 3; nothing else in this codebase calls it, so it
//     is not mirrored in the shared include/syscall_nums.h table). Rather than add a shared
//     production wrapper for a single test-only consumer, this file defines the syscall number
//     LOCALLY (`PROBE_SYS_close` below) and calls it directly through the already-public
//     `syscall1()` (include/syscall.h, ADR-0001) -- this keeps the change scoped to `tests/`
//     only, per this slice's ownership boundary. Closing fd 1 (stdout) BEFORE calling
//     `mini_printf` makes every `sys_write(1, ...)` inside it return a negative `-errno`
//     (`EBADF`, per ADR-0002's raw-errno contract) -- a PERMANENT, unrecoverable failure for
//     the rest of this process, which is the sharpest available proxy for "short write/error"
//     a freestanding test can produce without a pipe/fifo (this project has neither `pipe()`
//     nor `dup2()` wrappers either).
//
//     WHY THE SAME 10x32-BYTE BLOCK AS printf_e2e.c (not a short format string): reusing
//     printf_e2e.c's exact byte accounting (10 `%s` blocks of 32 bytes + one '\n' = 321 logical
//     bytes) forces the identical TWO-flush shape under `mini_printf` -- a MID-LOOP flush when
//     `mini_flush_sink`'s accumulator hits the 256-byte scratch boundary (inside
//     `mini_format_core`, before the format string is even fully consumed), then a FINAL flush
//     after it returns -- but with BOTH flushes now failing instead of both succeeding. A
//     single short `mini_printf("hi")` call would only ever trigger the final flush and would
//     prove point 1 below, not points 2/3.
//
//     WHAT THIS PINS:
//       1. `total` reflects the error, not the intended byte count: `mini_flush`
//          (src/printf.c) only adds `sys_write`'s return to `ctx->total` when it is positive,
//          so a permanently-failing fd 1 makes `mini_printf` return 0 no matter how much was
//          formatted -- asserted below as `written == 0`, even though 321 bytes were logically
//          produced.
//       2. The scratch buffer resets on error too, not just on success: `mini_flush`'s
//          `ctx->used = 0` runs unconditionally, AFTER the write attempt, regardless of its
//          result. Not just documentation -- load-bearing for point 3.
//       3. No retry, no hang, from EITHER of two independent causes -- and this probe's own
//          COMPLETION (reaching `TEST_ASSERT_EQ` and `sys_exit(0)`, instead of `make test`
//          never returning for this program) is the proof neither exists today:
//            (a) if `mini_flush` ever grew a retry-until-success loop, it would spin forever
//                here -- fd 1 stays closed for the rest of the process, so success never comes;
//            (b) if point 2's unconditional reset were ever dropped, the SECOND flush inside
//                `mini_flush_sink`'s `while (n > 0)` loop would see `room == 0` forever
//                (`ctx->used` stuck at 256) and `to_copy` would stay 0 forever too -- an
//                infinite loop from stale scratch state, not from a retry.
//          Both regressions present the SAME externally-observable symptom under this probe
//          (the process never reaches `sys_exit`), which is exactly why the probe is built to
//          exercise both flushes in one call instead of stopping at point 1.
//       4. No crash: reaching the assertion/`sys_exit(0)` below -- with fd 2 still open and
//          used normally by `TEST_ASSERT`/`TEST_ASSERT_EQ` on failure -- is itself proof the
//          closed-fd write attempts did not abort/segfault the process.
// PT: C0-PRINTF-SW (TODO.md, W26) -- FIXA o contrato ATUAL, deliberadamente NÃO-endurecido, do
//     caminho de erro de stdout do `mini_printf` com um teste executável, em vez de deixar essa
//     decisão documentada só em prosa (cabeçalho de arquivo do src/printf.c, comentário próprio
//     do `mini_flush`, ADR-0002). Este programa NÃO muda o comportamento do `mini_printf` e NÃO
//     acrescenta laço de retry -- ver a entrada `C0-PRINTF-SW` do TODO.md ("Minor, YAGNI, sem
//     gatilho ativo... endurecer só se um consumidor real precisar de garantia de escrita
//     completa") pro porquê de endurecer estar explicitamente FORA de escopo aqui.
//
//     COMO O FD FECHADO É PRODUZIDO SEM LIBC: este projeto ainda não tem wrapper de `close()`
//     (o `close` do Linux x86-64 é a syscall número 3; nada mais neste codebase a chama, então
//     ela não está espelhada na tabela compartilhada include/syscall_nums.h). Em vez de
//     acrescentar um wrapper de produção compartilhado pra um único consumidor só-de-teste,
//     este arquivo define o número da syscall LOCALMENTE (`PROBE_SYS_close` abaixo) e chama ela
//     direto através do já-público `syscall1()` (include/syscall.h, ADR-0001) -- isso mantém a
//     mudança restrita a `tests/` apenas, conforme a fronteira de posse desta fatia. Fechar o
//     fd 1 (stdout) ANTES de chamar `mini_printf` faz todo `sys_write(1, ...)` de dentro dele
//     retornar um `-errno` negativo (`EBADF`, conforme o contrato de errno cru do ADR-0002) --
//     uma falha PERMANENTE, irrecuperável, pro resto deste processo, que é o proxy mais afiado
//     disponível pra "short write/erro" que um teste freestanding consegue produzir sem um
//     pipe/fifo (este projeto também não tem wrapper de `pipe()` nem de `dup2()`).
//
//     POR QUE O MESMO BLOCO 10x32 BYTES DO printf_e2e.c (não uma string de formato curta):
//     reusar a contabilidade de bytes exata do printf_e2e.c (10 blocos `%s` de 32 bytes + um
//     '\n' = 321 bytes lógicos) força a MESMA forma de dois flushes sob o `mini_printf` -- um
//     flush NO MEIO DO LAÇO quando o acumulador do `mini_flush_sink` bate a fronteira de 256
//     bytes do scratch (dentro do `mini_format_core`, antes mesmo da string de formato ser
//     totalmente consumida), seguido de um flush FINAL depois dele retornar -- mas agora com OS
//     DOIS flushes falhando em vez dos dois tendo sucesso. Uma chamada `mini_printf("hi")`
//     curta única só dispararia o flush final e provaria o ponto 1 abaixo, não os pontos 2/3.
//
//     O QUE ISTO FIXA:
//       1. `total` reflete o erro, não a contagem de bytes pretendida: o `mini_flush`
//          (src/printf.c) só soma o retorno do `sys_write` ao `ctx->total` quando é positivo,
//          então um fd 1 permanentemente falho faz o `mini_printf` retornar 0 não importa
//          quanto foi formatado -- checado abaixo como `written == 0`, mesmo com 321 bytes
//          logicamente produzidos.
//       2. O buffer de rascunho reseta em erro também, não só em sucesso: o `ctx->used = 0` do
//          `mini_flush` roda incondicionalmente, DEPOIS da tentativa de escrita, independente
//          do resultado dela. Não é só documentação -- é estrutural pro ponto 3.
//       3. Sem retry, sem travamento, por QUALQUER uma de duas causas independentes -- e a
//          própria CONCLUSÃO desta sonda (chegar ao `TEST_ASSERT_EQ` e ao `sys_exit(0)`, em vez
//          de o `make test` nunca retornar pra este programa) é a prova de que nenhuma das duas
//          existe hoje:
//            (a) se o `mini_flush` algum dia ganhasse um laço de retry-até-sucesso, ele giraria
//                pra sempre aqui -- o fd 1 fica fechado pro resto do processo, entao sucesso
//                nunca vem;
//            (b) se o reset incondicional do ponto 2 algum dia fosse removido, o SEGUNDO flush
//                dentro do laço `while (n > 0)` do `mini_flush_sink` veria `room == 0` pra
//                sempre (`ctx->used` travado em 256) e `to_copy` ficaria 0 pra sempre também --
//                um laço infinito por estado de rascunho obsoleto, não por retry.
//          As duas regressoes apresentam o MESMO sintoma observavel externamente sob esta sonda
//          (o processo nunca chega ao `sys_exit`), que e' exatamente por que a sonda e'
//          construida pra exercitar os dois flushes numa unica chamada em vez de parar no
//          ponto 1.
//       4. Sem crash: chegar a asserção/`sys_exit(0)` abaixo -- com o fd 2 continuando aberto e
//          usado normalmente pelo `TEST_ASSERT`/`TEST_ASSERT_EQ` em caso de falha -- e' em si a
//          prova de que as tentativas de escrita no fd fechado não abortaram/crasharam o
//          processo.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "printf.h"
#include "syscall.h"
#include "sys_exit.h"

// EN: Linux x86-64 syscall number for `close(fd)` -- see file header for why this is defined
//     LOCALLY instead of joining the shared include/syscall_nums.h table (single consumer,
//     test-only, this slice's ownership boundary is `tests/` + `src/printf.c` + its header).
// PT: Numero de syscall Linux x86-64 pro `close(fd)` -- ver cabecalho do arquivo pro porque
//     disto ser definido LOCALMENTE em vez de entrar na tabela compartilhada
//     include/syscall_nums.h (consumidor unico, so'-de-teste, a fronteira de posse desta fatia
//     e' `tests/` + `src/printf.c` + o header dele).
#define PROBE_SYS_close 3

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // EN: Close stdout BEFORE calling mini_printf -- see file header. `close(1)` on a freshly
    //     inherited, still-open fd must succeed (rc == 0); this assertion catches an
    //     environment where fd 1 was somehow already invalid at process start, which would
    //     silently invalidate every claim below (a probe that "passes" for the wrong reason is
    //     worse than one that fails loudly here).
    // PT: Fecha o stdout ANTES de chamar mini_printf -- ver cabecalho do arquivo. `close(1)`
    //     sobre um fd recem-herdado, ainda aberto, tem que ter sucesso (rc == 0); esta asserção
    //     pega um ambiente onde o fd 1 de alguma forma ja estava invalido no inicio do
    //     processo, o que invalidaria em silencio toda claim abaixo (uma sonda que "passa" pelo
    //     motivo errado e' pior que uma que falha alto aqui).
    long close_rc = syscall1(PROBE_SYS_close, 1);
    TEST_ASSERT_EQ(close_rc, 0);

    // EN: Same 32-byte block / 10-repeat / trailing '\n' shape as tests/printf_e2e.c -- see that
    //     file's own header for the byte accounting this depends on (321 logical bytes, a
    //     mid-loop flush at the 256-byte boundary, then a final flush).
    // PT: Mesma forma de bloco de 32 bytes / 10 repeticoes / '\n' final que tests/printf_e2e.c
    //     -- ver o cabecalho proprio daquele arquivo pra contabilidade de bytes da qual isto
    //     depende (321 bytes logicos, um flush no meio do laço na fronteira de 256 bytes, e um
    //     flush final).
    static const char block[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    int written = mini_printf("%s%s%s%s%s%s%s%s%s%s\n", block, block, block, block, block, block,
                               block, block, block, block);

    // EN: THE pin: 0, not 321 -- see points 1-3 in the file header. Reaching this line at all
    //     (rather than the process hanging) already pins point 3; the value itself pins point 1.
    // PT: A FIXAÇÃO: 0, não 321 -- ver os pontos 1-3 do cabeçalho do arquivo. Só chegar nesta
    //     linha (em vez do processo travar) já fixa o ponto 3; o valor em si fixa o ponto 1.
    TEST_ASSERT_EQ(written, 0);

    sys_exit(0);
    return 0; // unreachable -- sys_exit() above never returns
}
