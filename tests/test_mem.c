// SPDX-License-Identifier: MPL-2.0
// EN: D1 gate program -- TDD RED first: written against include/mem.h BEFORE src/mem.c has a
//     real body, so the link step fails (undefined reference to memcpy/memset/memmove/memcmp)
//     until D1 is implemented. See task report for the captured red-phase error. Covers the
//     spec cases from the task brief: memcpy (basic copy, n=0, return value), memset (fill
//     value, n=0, value >255 truncation), memmove (forward overlap, backward overlap, no
//     overlap == memcpy behaviour, dst==src, n==0 -- the last two added as the AUD-C0-1
//     follow-up, AUDIT_FIND.md), memcmp (equal, first-less, first-greater, diff-in-last-
//     byte, n=0, unsigned-byte comparison across 0x7F).
// PT: Programa-gate do D1 -- TDD VERMELHO primeiro: escrito contra include/mem.h ANTES de
//     src/mem.c ter corpo de verdade, entao o link falha (undefined reference a
//     memcpy/memset/memmove/memcmp) ate o D1 ser implementado. Ver relatorio da tarefa pro
//     erro capturado na fase vermelha. Cobre os casos da especificacao do brief da tarefa:
//     memcpy (copia basica, n=0, valor de retorno), memset (preenche valor, n=0, truncamento
//     de valor >255), memmove (overlap forward, overlap backward, sem overlap == comportamento
//     de memcpy, dst==src, n==0 -- os dois ultimos acrescentados como follow-up do AUD-C0-1,
//     AUDIT_FIND.md), memcmp (iguais, primeiro-menor, primeiro-maior, diferenca-no-ultimo-byte,
//     n=0, comparacao unsigned de byte atravessando 0x7F).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "mem.h"
#include "sys_exit.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- memcpy -----------------------------------------------------------
    // EN: Basic copy of a known buffer, no overlap.
    // PT: Copia basica de um buffer conhecido, sem overlap.
    {
        unsigned char src[5] = {1, 2, 3, 4, 5};
        unsigned char dst[5] = {0, 0, 0, 0, 0};
        void* ret = memcpy(dst, src, 5);
        TEST_ASSERT_EQ(ret, dst);
        TEST_ASSERT_EQ(dst[0], 1);
        TEST_ASSERT_EQ(dst[1], 2);
        TEST_ASSERT_EQ(dst[2], 3);
        TEST_ASSERT_EQ(dst[3], 4);
        TEST_ASSERT_EQ(dst[4], 5);
    }

    // EN: n=0 -- must not touch dst, must still return dst.
    // PT: n=0 -- nao deve tocar dst, ainda deve retornar dst.
    {
        unsigned char src[3] = {9, 9, 9};
        unsigned char dst[3] = {1, 2, 3};
        void* ret = memcpy(dst, src, 0);
        TEST_ASSERT_EQ(ret, dst);
        TEST_ASSERT_EQ(dst[0], 1);
        TEST_ASSERT_EQ(dst[1], 2);
        TEST_ASSERT_EQ(dst[2], 3);
    }

    // ---- memset -------------------------------------------------------------
    // EN: Fills with a known byte value.
    // PT: Preenche com um valor de byte conhecido.
    {
        unsigned char buf[4] = {0, 0, 0, 0};
        void* ret = memset(buf, 0xAB, 4);
        TEST_ASSERT_EQ(ret, buf);
        TEST_ASSERT_EQ(buf[0], 0xAB);
        TEST_ASSERT_EQ(buf[1], 0xAB);
        TEST_ASSERT_EQ(buf[2], 0xAB);
        TEST_ASSERT_EQ(buf[3], 0xAB);
    }

    // EN: n=0 -- must not touch buf. `n` kept in a variable (not a literal `0` argument) to
    //     sidestep clang's `-Wmemset-transposed-args` heuristic, which flags a literal 0-sized
    //     memset as a likely swapped-argument typo -- a false positive for this deliberate case.
    // PT: n=0 -- nao deve tocar buf. `n` mantido numa variavel (nao um argumento literal `0`)
    //     pra desviar da heuristica `-Wmemset-transposed-args` do clang, que sinaliza um
    //     memset de tamanho 0 literal como provavel erro de argumentos trocados -- falso
    //     positivo para este caso deliberado.
    {
        unsigned char buf[3] = {1, 2, 3};
        size_t n = 0;
        void* ret = memset(buf, 0xFF, n);
        TEST_ASSERT_EQ(ret, buf);
        TEST_ASSERT_EQ(buf[0], 1);
        TEST_ASSERT_EQ(buf[1], 2);
        TEST_ASSERT_EQ(buf[2], 3);
    }

    // EN: Value >255 must be truncated to a single byte -- 0x1FF -> 0xFF.
    // PT: Valor >255 deve ser truncado pra um unico byte -- 0x1FF -> 0xFF.
    {
        unsigned char buf[2] = {0, 0};
        memset(buf, 0x1FF, 2);
        TEST_ASSERT_EQ(buf[0], 0xFF);
        TEST_ASSERT_EQ(buf[1], 0xFF);
    }

    // ---- memmove --------------------------------------------------------------
    // EN: Forward overlap: dst > src, e.g. memmove(a+2, a, 5) on a 7-byte buffer.
    //     a = {0,1,2,3,4,5,6}; moving a[0..5) into a[2..7) must yield
    //     a = {0,1,0,1,2,3,4} -- computed by hand: the source window is fully read (into a
    //     temporary understanding, not a real temp buffer) before being overwritten.
    // PT: Overlap forward: dst > src, ex.: memmove(a+2, a, 5) num buffer de 7 bytes.
    //     a = {0,1,2,3,4,5,6}; mover a[0..5) pra a[2..7) deve resultar em
    //     a = {0,1,0,1,2,3,4} -- calculado a mao: a janela de origem e' totalmente lida
    //     (mentalmente, nao um buffer temporario real) antes de ser sobrescrita.
    {
        unsigned char a[7] = {0, 1, 2, 3, 4, 5, 6};
        void* ret = memmove(a + 2, a, 5);
        TEST_ASSERT_EQ(ret, (void*)(a + 2));
        unsigned char expected[7] = {0, 1, 0, 1, 2, 3, 4};
        for (size_t i = 0; i < 7; i++) {
            TEST_ASSERT_EQ(a[i], expected[i]);
        }
    }

    // EN: Backward overlap: dst < src, e.g. memmove(a, a+2, 5) on a 7-byte buffer.
    //     a = {0,1,2,3,4,5,6}; moving a[2..7) into a[0..5) must yield
    //     a = {2,3,4,5,6,5,6} -- computed by hand.
    // PT: Overlap backward: dst < src, ex.: memmove(a, a+2, 5) num buffer de 7 bytes.
    //     a = {0,1,2,3,4,5,6}; mover a[2..7) pra a[0..5) deve resultar em
    //     a = {2,3,4,5,6,5,6} -- calculado a mao.
    {
        unsigned char a[7] = {0, 1, 2, 3, 4, 5, 6};
        void* ret = memmove(a, a + 2, 5);
        TEST_ASSERT_EQ(ret, (void*)a);
        unsigned char expected[7] = {2, 3, 4, 5, 6, 5, 6};
        for (size_t i = 0; i < 7; i++) {
            TEST_ASSERT_EQ(a[i], expected[i]);
        }
    }

    // EN: No overlap -- must behave exactly like memcpy.
    // PT: Sem overlap -- deve se comportar exatamente como memcpy.
    {
        unsigned char src[4] = {10, 20, 30, 40};
        unsigned char dst[4] = {0, 0, 0, 0};
        void* ret = memmove(dst, src, 4);
        TEST_ASSERT_EQ(ret, dst);
        TEST_ASSERT_EQ(dst[0], 10);
        TEST_ASSERT_EQ(dst[1], 20);
        TEST_ASSERT_EQ(dst[2], 30);
        TEST_ASSERT_EQ(dst[3], 40);
    }

    // EN: AUD-C0-1 follow-up (AUDIT_FIND.md, annotated in the D1 doc-comment of src/mem.c):
    //     dst == src (same address, du == su in the uintptr_t comparison) -- neither branch of
    //     the if/else-if is taken, the function must be a safe no-op that still returns dst and
    //     leaves every byte untouched (the buffer's own bytes are both source and destination).
    // PT: Follow-up do AUD-C0-1 (AUDIT_FIND.md, anotado no doc-comment do D1 em src/mem.c):
    //     dst == src (mesmo endereco, du == su na comparacao via uintptr_t) -- nenhum dos ramos
    //     if/else-if e' tomado, a funcao deve ser um no-op seguro que ainda assim retorna dst e
    //     deixa todo byte intocado (os bytes do proprio buffer sao origem e destino ao mesmo
    //     tempo).
    {
        unsigned char buf[5] = {1, 2, 3, 4, 5};
        void* ret = memmove(buf, buf, 5);
        TEST_ASSERT_EQ(ret, (void*)buf);
        TEST_ASSERT_EQ(buf[0], 1);
        TEST_ASSERT_EQ(buf[1], 2);
        TEST_ASSERT_EQ(buf[2], 3);
        TEST_ASSERT_EQ(buf[3], 4);
        TEST_ASSERT_EQ(buf[4], 5);
    }

    // EN: AUD-C0-1 follow-up: n == 0 with dst != src -- must not touch dst (either branch's loop
    //     bound is 0 iterations either way), must still return dst.
    // PT: Follow-up do AUD-C0-1: n == 0 com dst != src -- nao deve tocar dst (o limite do laco de
    //     qualquer um dos dois ramos e' 0 iteracoes de qualquer forma), ainda deve retornar dst.
    {
        unsigned char src[3] = {9, 9, 9};
        unsigned char dst[3] = {1, 2, 3};
        void* ret = memmove(dst, src, 0);
        TEST_ASSERT_EQ(ret, dst);
        TEST_ASSERT_EQ(dst[0], 1);
        TEST_ASSERT_EQ(dst[1], 2);
        TEST_ASSERT_EQ(dst[2], 3);
    }

    // ---- memcmp -----------------------------------------------------------
    // EN: Equal buffers -> 0.
    // PT: Buffers iguais -> 0.
    {
        unsigned char a[4] = {1, 2, 3, 4};
        unsigned char b[4] = {1, 2, 3, 4};
        TEST_ASSERT_EQ(memcmp(a, b, 4), 0);
    }

    // EN: First differing byte in `a` is smaller -> negative.
    // PT: Primeiro byte diferente em `a` e' menor -> negativo.
    {
        unsigned char a[3] = {1, 2, 3};
        unsigned char b[3] = {1, 5, 3};
        TEST_ASSERT(memcmp(a, b, 3) < 0);
    }

    // EN: First differing byte in `a` is larger -> positive.
    // PT: Primeiro byte diferente em `a` e' maior -> positivo.
    {
        unsigned char a[3] = {1, 9, 3};
        unsigned char b[3] = {1, 5, 3};
        TEST_ASSERT(memcmp(a, b, 3) > 0);
    }

    // EN: Difference only in the last byte.
    // PT: Diferenca so no ultimo byte.
    {
        unsigned char a[4] = {1, 2, 3, 4};
        unsigned char b[4] = {1, 2, 3, 9};
        TEST_ASSERT(memcmp(a, b, 4) < 0);
    }

    // EN: n=0 -> 0, regardless of contents.
    // PT: n=0 -> 0, independente do conteudo.
    {
        unsigned char a[1] = {1};
        unsigned char b[1] = {2};
        TEST_ASSERT_EQ(memcmp(a, b, 0), 0);
    }

    // EN: Bytes must be compared as UNSIGNED char -- 0xFF vs 0x01 must be POSITIVE (0xFF=255
    //     > 0x01=1 unsigned), not negative (which a naive signed-`char` comparison would give
    //     on a platform where `char` is signed, since (signed char)0xFF == -1 < 1).
    // PT: Bytes devem ser comparados como `char` UNSIGNED -- 0xFF vs 0x01 deve ser POSITIVO
    //     (0xFF=255 > 0x01=1 unsigned), nao negativo (o que uma comparacao ingenua de `char`
    //     signed daria numa plataforma onde `char` e' signed, ja que (signed char)0xFF == -1 < 1).
    {
        unsigned char a[1] = {0xFF};
        unsigned char b[1] = {0x01};
        TEST_ASSERT(memcmp(a, b, 1) > 0);
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
