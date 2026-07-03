// SPDX-License-Identifier: MPL-2.0
// EN: D2 gate program -- TDD RED first: written against include/str.h BEFORE src/str.c has a
//     real body, so the link step fails (undefined reference to strlen/strcmp/strncmp/strcpy/
//     strncpy/strcat/strchr) until D2 is implemented. See task report for the captured red-phase
//     error. Covers the spec cases from the task brief: strlen (empty, non-empty), strcmp
//     (equal, first-less, first-greater, prefix), strncmp (n=0, equal-within-n, diff-beyond-n
//     does not count), strcpy (basic + NUL copied), strncpy (THE gotcha -- src shorter than n
//     pads with `\0`, src == n has no terminator, src > n truncates without terminator, n=0),
//     strcat (basic append, append onto non-empty dst), strchr (found mid-string, not found ->
//     NULL, `\0` matches the terminator itself, first of multiple occurrences).
// PT: Programa-gate do D2 -- TDD VERMELHO primeiro: escrito contra include/str.h ANTES de
//     src/str.c ter corpo de verdade, entao o link falha (undefined reference a strlen/strcmp/
//     strncmp/strcpy/strncpy/strcat/strchr) ate o D2 ser implementado. Ver relatorio da tarefa
//     pro erro capturado na fase vermelha. Cobre os casos da especificacao do brief da tarefa:
//     strlen (vazia, nao-vazia), strcmp (iguais, primeiro-menor, primeiro-maior, prefixo),
//     strncmp (n=0, iguais-dentro-de-n, diferenca-alem-de-n nao conta), strcpy (basico + NUL
//     copiado), strncpy (O gotcha -- src mais curto que n preenche com `\0`, src == n sem
//     terminador, src > n trunca sem terminador, n=0), strcat (append basico, append em cima de
//     dst nao-vazio), strchr (achada no meio da string, nao achada -> NULL, `\0` casa com o
//     proprio terminador, primeira de multiplas ocorrencias).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "str.h"
#include "types.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- strlen -------------------------------------------------------------
    // EN: Empty string -> 0.
    // PT: String vazia -> 0.
    {
        TEST_ASSERT_EQ(strlen(""), (size_t)0);
    }

    // EN: Non-empty string -> byte count excluding the terminator.
    // PT: String nao-vazia -> contagem de bytes excluindo o terminador.
    {
        TEST_ASSERT_EQ(strlen("hello"), (size_t)5);
    }

    // ---- strcmp ---------------------------------------------------------------
    // EN: Equal strings -> 0.
    // PT: Strings iguais -> 0.
    {
        TEST_ASSERT_EQ(strcmp("abc", "abc"), 0);
    }

    // EN: First differing byte in `a` is smaller -> negative.
    // PT: Primeiro byte diferente em `a` e' menor -> negativo.
    {
        TEST_ASSERT(strcmp("abc", "abd") < 0);
    }

    // EN: First differing byte in `a` is larger -> positive.
    // PT: Primeiro byte diferente em `a` e' maior -> positivo.
    {
        TEST_ASSERT(strcmp("abd", "abc") > 0);
    }

    // EN: `a` is a strict prefix of `b` -> negative (`\0` < any other byte).
    // PT: `a` e' prefixo estrito de `b` -> negativo (`\0` < qualquer outro byte).
    {
        TEST_ASSERT(strcmp("ab", "abc") < 0);
    }

    // ---- strncmp --------------------------------------------------------------
    // EN: n=0 -> 0, regardless of contents.
    // PT: n=0 -> 0, independente do conteudo.
    {
        TEST_ASSERT_EQ(strncmp("abc", "xyz", 0), 0);
    }

    // EN: Equal within the first n bytes -> 0, even though the strings differ later.
    // PT: Iguais dentro dos primeiros n bytes -> 0, mesmo as strings diferindo depois.
    {
        TEST_ASSERT_EQ(strncmp("abcXX", "abcYY", 3), 0);
    }

    // EN: Difference BEYOND n must not count.
    // PT: Diferenca ALEM de n nao deve contar.
    {
        TEST_ASSERT_EQ(strncmp("abc", "abcd", 3), 0);
    }

    // EN: Difference WITHIN n must count, same sign rules as strcmp.
    // PT: Diferenca DENTRO de n deve contar, mesmas regras de sinal do strcmp.
    {
        TEST_ASSERT(strncmp("abc", "abd", 3) < 0);
    }

    // ---- strcpy -----------------------------------------------------------------
    // EN: Basic copy, including the terminating `\0`; return value is `dst`.
    // PT: Copia basica, incluindo o `\0` terminador; valor de retorno e' `dst`.
    {
        char dst[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strcpy(dst, "hi");
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'h');
        TEST_ASSERT_EQ(dst[1], 'i');
        TEST_ASSERT_EQ(dst[2], '\0');
        // EN: Bytes beyond the copied `\0` are untouched -- strcpy does not pad.
        // PT: Bytes alem do `\0` copiado ficam intocados -- strcpy nao faz padding.
        TEST_ASSERT_EQ(dst[3], 'X');
    }

    // ---- strncpy -- THE gotcha, three cases ------------------------------------
    // EN: Case 1: src SHORTER than n -- rest of dst up to n is zero-padded (not just one \0).
    // PT: Caso 1: src MAIS CURTO que n -- resto de dst ate n e' preenchido com zero (nao so
    //     um `\0`).
    {
        char dst[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strncpy(dst, "ab", 5);
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'a');
        TEST_ASSERT_EQ(dst[1], 'b');
        TEST_ASSERT_EQ(dst[2], '\0');
        TEST_ASSERT_EQ(dst[3], '\0');
        TEST_ASSERT_EQ(dst[4], '\0');
        // EN: Byte at index 5 (beyond n=5) is untouched.
        // PT: Byte no indice 5 (alem de n=5) fica intocado.
        TEST_ASSERT_EQ(dst[5], 'X');
    }

    // EN: Case 2: src length == n exactly -- n bytes copied, NO terminator added.
    // PT: Caso 2: tamanho de src == n exatamente -- n bytes copiados, SEM terminador
    //     adicionado.
    {
        char dst[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strncpy(dst, "abc", 3); // strlen("abc") == 3 == n
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'a');
        TEST_ASSERT_EQ(dst[1], 'b');
        TEST_ASSERT_EQ(dst[2], 'c');
        // EN: dst[3] must remain untouched -- no NUL was written, this is the dangerous part.
        // PT: dst[3] deve permanecer intocado -- nenhum NUL foi escrito, essa e' a parte
        //     perigosa.
        TEST_ASSERT_EQ(dst[3], 'X');
    }

    // EN: Case 3: src LONGER than n -- truncates to n bytes, NO terminator added.
    // PT: Caso 3: src MAIS LONGO que n -- trunca pra n bytes, SEM terminador adicionado.
    {
        char dst[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strncpy(dst, "abcdef", 3); // strlen 6 > n=3
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'a');
        TEST_ASSERT_EQ(dst[1], 'b');
        TEST_ASSERT_EQ(dst[2], 'c');
        TEST_ASSERT_EQ(dst[3], 'X'); // untouched
    }

    // EN: Case 4: n=0 -- must not touch dst at all.
    // PT: Caso 4: n=0 -- nao deve tocar dst de jeito nenhum.
    {
        char dst[3] = {'X', 'Y', 'Z'};
        char* ret = strncpy(dst, "ab", 0);
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'X');
        TEST_ASSERT_EQ(dst[1], 'Y');
        TEST_ASSERT_EQ(dst[2], 'Z');
    }

    // ---- strcat -----------------------------------------------------------------
    // EN: Basic append onto an empty dst.
    // PT: Append basico em cima de um dst vazio.
    {
        char dst[6] = {'\0', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strcat(dst, "hi");
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'h');
        TEST_ASSERT_EQ(dst[1], 'i');
        TEST_ASSERT_EQ(dst[2], '\0');
    }

    // EN: Append onto a non-empty dst -- must find dst's own `\0` first.
    // PT: Append em cima de um dst nao-vazio -- deve achar o proprio `\0` de dst primeiro.
    {
        char dst[8] = {'a', 'b', '\0', 'X', 'X', 'X', 'X', 'X'};
        char* ret = strcat(dst, "cd");
        TEST_ASSERT_EQ((void*)ret, (void*)dst);
        TEST_ASSERT_EQ(dst[0], 'a');
        TEST_ASSERT_EQ(dst[1], 'b');
        TEST_ASSERT_EQ(dst[2], 'c');
        TEST_ASSERT_EQ(dst[3], 'd');
        TEST_ASSERT_EQ(dst[4], '\0');
    }

    // ---- strchr -----------------------------------------------------------------
    // EN: Found mid-string -- pointer arithmetic checked against the source array's address.
    // PT: Achada no meio da string -- aritmetica de ponteiro checada contra o endereco do
    //     array-fonte.
    {
        const char* s = "hello";
        char* p = strchr(s, 'l');
        TEST_ASSERT_EQ((void*)p, (void*)(s + 2)); // first 'l' -- index 2
    }

    // EN: Not found -> NULL.
    // PT: Nao achada -> NULL.
    {
        const char* s = "hello";
        char* p = strchr(s, 'z');
        TEST_ASSERT_EQ((void*)p, (void*)NULL);
    }

    // EN: c == '\0' matches the terminator itself -- pointer to the end of the string, NOT
    //     NULL.
    // PT: c == '\0' casa com o proprio terminador -- ponteiro pro fim da string, NAO NULL.
    {
        const char* s = "hi";
        char* p = strchr(s, '\0');
        TEST_ASSERT_EQ((void*)p, (void*)(s + 2)); // points at the NUL, index 2
    }

    // EN: First of multiple occurrences.
    // PT: Primeira de multiplas ocorrencias.
    {
        const char* s = "banana";
        char* p = strchr(s, 'a');
        TEST_ASSERT_EQ((void*)p, (void*)(s + 1)); // index 1, not 3 or 5
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
