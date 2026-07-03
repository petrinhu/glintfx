// SPDX-License-Identifier: MPL-2.0
// EN: The C1 gate program. Proves the test harness (include/test.h) end to end: a battery of
//     TEST_ASSERT/TEST_ASSERT_EQ calls that all PASS (integer equality, arithmetic, boolean
//     logic, pointer comparison), then a bare sys_exit(0) to close out -- deliberately NOT
//     TEST_PASS() here: the verification contract for this gate program is that a fully
//     passing run is completely SILENT (strace shows only `execve` + `exit(0)`, zero syscalls
//     in between), proving TEST_ASSERT truly does nothing observable on the happy path.
//     TEST_PASS() (optional cosmetic sugar, see test.h) does issue a `write`, so using it here
//     would contradict that silent-on-success contract; it remains available in test.h for
//     other tests that prefer a visible "PASS: <file>" line instead. Every assertion here is
//     designed to succeed -- proving the harness stays silent and falls through correctly on
//     the happy path. The companion proof that TEST_ASSERT correctly ABORTS with the right
//     file:line on a FAILING condition is done manually with a throwaway probe program (never
//     committed -- see the C1 task notes) rather than here, since a program whose job is to
//     `sys_exit(1)` cannot also be a member of this all-green suite (tests/expected_exit.txt
//     has an explicit `selftest 0` entry).
// PT: O programa-gate do C1. Prova o harness de teste (include/test.h) ponta a ponta: uma
//     bateria de chamadas TEST_ASSERT/TEST_ASSERT_EQ que TODAS passam (igualdade de inteiros,
//     aritmetica, logica booleana, comparacao de ponteiro), fechando com um sys_exit(0) seco --
//     deliberadamente SEM TEST_PASS() aqui: o contrato de verificacao deste programa-gate e'
//     que uma execucao totalmente aprovada fica completamente SILENCIOSA (strace mostra so
//     `execve` + `exit(0)`, zero syscalls no meio), provando que o TEST_ASSERT realmente nao
//     faz nada observavel no caminho feliz. O TEST_PASS() (acucar cosmetico opcional, ver
//     test.h) de fato emite um `write`, entao usa-lo aqui contradiria esse contrato de
//     silencio-no-sucesso; ele continua disponivel no test.h pra outros testes que preferirem
//     uma linha visivel "PASS: <arquivo>". Toda asserção aqui e' desenhada pra ter sucesso --
//     provando que o harness fica em silencio e cai adiante corretamente no caminho feliz. A
//     prova complementar de que o TEST_ASSERT ABORTA corretamente com o file:linha certo numa
//     condicao que FALHA e' feita manualmente com um programa-sonda descartavel (nunca
//     commitado -- ver as notas da tarefa C1) em vez de aqui, ja que um programa cujo trabalho
//     e' dar `sys_exit(1)` nao pode tambem ser membro desta suite toda-verde
//     (tests/expected_exit.txt tem uma entrada explicita `selftest 0`).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // EN: Trivial truths -- proves TEST_ASSERT itself does not misfire on an obviously true
    //     condition.
    // PT: Verdades triviais -- prova que o TEST_ASSERT em si nao dispara errado numa condicao
    //     obviamente verdadeira.
    TEST_ASSERT(1 == 1);
    TEST_ASSERT(0 == 0);
    TEST_ASSERT(1);

    // EN: TEST_ASSERT_EQ sugar, integer arithmetic.
    // PT: Acucar TEST_ASSERT_EQ, aritmetica de inteiros.
    TEST_ASSERT_EQ(2 + 2, 4);
    TEST_ASSERT_EQ(10 - 3, 7);
    TEST_ASSERT_EQ(6 * 7, 42);
    TEST_ASSERT_EQ(100 / 4, 25);

    // EN: Boolean logic and comparisons.
    // PT: Logica booleana e comparacoes.
    int a = 5;
    int b = 10;
    TEST_ASSERT(a < b);
    TEST_ASSERT(b > a);
    TEST_ASSERT(a != b);
    TEST_ASSERT((a < b) && (b > 0));
    TEST_ASSERT((a > b) || (a == 5));

    // EN: Pointer comparison -- proves the macros work on non-integer operand types too (the
    //     harness makes no assumption about the operand type of `cond`/`a == b`).
    // PT: Comparacao de ponteiro -- prova que as macros funcionam tambem em tipos de operando
    //     nao-inteiros (o harness nao faz suposicao sobre o tipo dos operandos de
    //     `cond`/`a == b`).
    int x = 1;
    int* px = &x;
    int* py = px;
    TEST_ASSERT_EQ(px, py);
    TEST_ASSERT(px != NULL);

    sys_exit(0);
    return 0; // unreachable -- sys_exit() above never returns
}
