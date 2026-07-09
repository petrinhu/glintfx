// SPDX-License-Identifier: MPL-2.0
// EN: AUD-C0-5.3 (AUDIT_FIND.md) gate program: the NEGATIVE-path counterpart to
//     tests/selftest.c. selftest.c's own file header explains why this proof could not live
//     there: a program whose job is to sys_exit(1) cannot also be a member of the all-green
//     suite selftest.c belongs to, and until now the proof that TEST_ASSERT correctly ABORTS
//     with the right "FAIL: <file>:<line>: <expr>" message on a FAILING condition was done
//     manually with a throwaway probe program, never committed. This file IS that probe,
//     committed and wired into two permanent regression checks (Makefile):
//       1. `make test` (the generic loop over every tests/*.c program, TST-INT): this program
//          is picked up automatically by the Makefile's wildcard build (no Makefile edit
//          needed to add a new gate program), and tests/expected_exit.txt registers its
//          expected exit code as 1 (not 0) -- the SAME mechanism tests/expected_exit.txt
//          already uses for exit42's "expected 42", so a non-zero-but-EXPECTED exit code is not
//          a new concept here. This alone versions "TEST_ASSERT aborts (exit != 0) on a false
//          condition" as a standing regression check.
//       2. `make test-negative` / the stderr-golden branch of `make test`
//          (tests/expected_stderr.txt): additionally diffs this program's stderr, byte-exact,
//          against tests/golden/negative_probe.expected -- versioning the STRICTER claim that
//          the message is not just "some non-zero exit" but the exact
//          "FAIL: tests/negative_probe.c:<N>: 1 == 2" text TEST_ASSERT (include/test.h) is
//          documented to produce, with the correct file:line pointing at the exact TEST_ASSERT
//          call below.
//
//     THE CONDITION: `TEST_ASSERT(1 == 2)` is deliberately false, unconditionally and
//     trivially (no runtime state to reason about) -- the point of this probe is to prove the
//     HARNESS's failure path, not to exercise any interesting program logic. The line number of
//     that call is baked into the committed golden file (tests/golden/negative_probe.expected),
//     generated from this program's REAL captured stderr output, not hand-typed -- if this file
//     is ever edited and the TEST_ASSERT call moves to a different line, the golden file must be
//     regenerated (run the built binary, capture fd 2, overwrite the golden) or `make
//     test-negative` will correctly FAIL on the file:line mismatch.
// PT: Programa-gate do AUD-C0-5.3 (AUDIT_FIND.md): a contraparte do caminho NEGATIVO de
//     tests/selftest.c. O proprio cabecalho de arquivo do selftest.c explica por que essa prova
//     nao podia morar la: um programa cujo trabalho e' dar sys_exit(1) nao pode tambem ser
//     membro da suite toda-verde a qual o selftest.c pertence, e ate agora a prova de que o
//     TEST_ASSERT ABORTA corretamente com a mensagem certa "FAIL: <arquivo>:<linha>: <expr>"
//     numa condicao que FALHA era feita manualmente com um programa-sonda descartavel, nunca
//     commitado. Este arquivo E' essa sonda, commitada e ligada a duas checagens de regressao
//     permanentes (Makefile):
//       1. `make test` (o laco generico sobre todo programa tests/*.c, TST-INT): este programa
//          e' pego automaticamente pelo build wildcard do Makefile (nenhuma edicao do Makefile
//          e' necessaria pra acrescentar um novo programa-gate), e tests/expected_exit.txt
//          registra o exit code esperado dele como 1 (nao 0) -- o MESMO mecanismo que
//          tests/expected_exit.txt ja usa pro "esperado 42" do exit42, entao um exit code
//          nao-zero-mas-ESPERADO nao e' um conceito novo aqui. So' isso ja versiona "o
//          TEST_ASSERT aborta (exit != 0) numa condicao falsa" como uma checagem de regressao
//          permanente.
//       2. `make test-negative` / o ramo golden-de-stderr do `make test`
//          (tests/expected_stderr.txt): adicionalmente compara o stderr deste programa,
//          byte-a-byte, contra tests/golden/negative_probe.expected -- versionando a alegacao
//          MAIS RIGOROSA de que a mensagem nao e' so' "algum exit code nao-zero" mas o texto
//          EXATO "FAIL: tests/negative_probe.c:<N>: 1 == 2" que o TEST_ASSERT (include/test.h)
//          e' documentado a produzir, com o file:linha correto apontando pra chamada
//          TEST_ASSERT exata abaixo.
//
//     A CONDICAO: `TEST_ASSERT(1 == 2)` e' deliberadamente falsa, incondicional e trivialmente
//     (sem estado de runtime pra raciocinar sobre) -- o ponto desta sonda e' provar o caminho de
//     falha do HARNESS, nao exercitar logica de programa interessante nenhuma. O numero da linha
//     dessa chamada esta' embutido no golden file commitado
//     (tests/golden/negative_probe.expected), gerado a partir da saida REAL capturada de stderr
//     deste programa, nao digitado a mao -- se este arquivo for editado algum dia e a chamada
//     TEST_ASSERT mudar de linha, o golden file precisa ser regenerado (rodar o binario
//     buildado, capturar o fd 2, sobrescrever o golden) ou o `make test-negative` vai FALHAR
//     corretamente pela divergencia de file:linha.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // EN: Deliberately false -- see file header. Line number is load-bearing (baked into the
    //     golden file); do not reformat this call across multiple lines.
    // PT: Deliberadamente falsa -- ver cabecalho do arquivo. O numero da linha e' estrutural
    //     (embutido no golden file); nao reformate esta chamada em varias linhas.
    TEST_ASSERT(1 == 2);

    // EN: Unreachable -- TEST_ASSERT above always aborts via sys_exit(1) first.
    // PT: Inalcancavel -- o TEST_ASSERT acima sempre aborta via sys_exit(1) primeiro.
    sys_exit(0);
    return 0;
}
