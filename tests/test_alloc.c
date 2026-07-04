// SPDX-License-Identifier: MPL-2.0
// EN: E1 gate program -- TDD RED first: written against include/alloc.h BEFORE src/alloc.c
//     had a real body, so the link step failed (undefined reference to malloc/free/realloc)
//     until E1 was implemented. See task report for the captured red-phase error. Covers the
//     spec cases from the task brief: malloc (non-NULL + 16-byte alignment, memory is
//     writable/readable), multiple mallocs (distinct, non-overlapping -- proved by writing a
//     unique pattern into each block and checking none corrupted another), realloc (growing
//     preserves old content, realloc(NULL,n)==malloc, realloc(p,0) per this project's
//     documented choice), free(NULL) and free(p) as safe no-ops, malloc(0) per this project's
//     documented choice, and exhausting the first arena to force a SECOND `mmap`'d arena
//     (proves arena growth -- pointers from the second arena are equally valid/writable).
// PT: Programa-gate do E1 -- TDD VERMELHO primeiro: escrito contra include/alloc.h ANTES de
//     src/alloc.c ter corpo de verdade, entao o link falhou (undefined reference a
//     malloc/free/realloc) ate o E1 ser implementado. Ver relatorio da tarefa pro erro
//     capturado na fase vermelha. Cobre os casos da especificacao do brief da tarefa: malloc
//     (nao-NULL + alinhamento a 16 bytes, memoria e' escrivivel/legivel), multiplos mallocs
//     (distintos, nao-sobrepostos -- provado escrevendo um padrao unico em cada bloco e
//     checando que nenhum corrompeu o outro), realloc (crescer preserva conteudo antigo,
//     realloc(NULL,n)==malloc, realloc(p,0) conforme a escolha documentada deste projeto),
//     free(NULL) e free(p) como no-ops seguros, malloc(0) conforme a escolha documentada deste
//     projeto, e esgotar a primeira arena pra forcar uma SEGUNDA arena `mmap`'ada (prova o
//     crescimento de arena -- ponteiros da segunda arena sao igualmente validos/escriviveis).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "alloc.h"
#include "types.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- malloc: basic allocation, alignment, writability ------------------
    // EN: Non-NULL, aligned to 16, and the returned memory is genuinely writable/readable
    //     (a wild/garbage pointer would fault on write under a real mmap-backed page).
    // PT: Nao-NULL, alinhado a 16, e a memoria retornada e' genuinamente
    //     escrivivel/legivel (um ponteiro selvagem/lixo daria fault ao escrever sob uma
    //     pagina real lastreada por mmap).
    {
        void* p = malloc(64);
        TEST_ASSERT(p != NULL);
        TEST_ASSERT_EQ((uintptr_t)p % 16, (uintptr_t)0);

        unsigned char* bytes = (unsigned char*)p;
        for (size_t i = 0; i < 64; i++) {
            bytes[i] = (unsigned char)(i & 0xFF);
        }
        for (size_t i = 0; i < 64; i++) {
            TEST_ASSERT_EQ(bytes[i], (unsigned char)(i & 0xFF));
        }
    }

    // EN: A second, unrelated size -- also aligned. Different allocation sizes must not break
    //     the 16-byte invariant.
    // PT: Um segundo tamanho, sem relacao -- tambem alinhado. Tamanhos de alocacao diferentes
    //     nao podem quebrar o invariante de 16 bytes.
    {
        void* p = malloc(1);
        TEST_ASSERT(p != NULL);
        TEST_ASSERT_EQ((uintptr_t)p % 16, (uintptr_t)0);
    }

    // ---- multiple mallocs: distinct, non-overlapping ------------------------
    // EN: Three blocks of different sizes, each stamped with a UNIQUE byte pattern, then all
    //     three re-checked -- if the bump allocator mis-sized or mis-aligned any block, one
    //     write would have clobbered a neighbour and this would catch it.
    // PT: Tres blocos de tamanhos diferentes, cada um carimbado com um padrao de byte UNICO,
    //     depois os tres reconferidos -- se o bump allocator dimensionou ou alinhou mal algum
    //     bloco, uma escrita teria atropelado um vizinho e isto pegaria.
    {
        void* a = malloc(10);
        void* b = malloc(20);
        void* c = malloc(30);
        TEST_ASSERT(a != NULL);
        TEST_ASSERT(b != NULL);
        TEST_ASSERT(c != NULL);
        TEST_ASSERT(a != b);
        TEST_ASSERT(b != c);
        TEST_ASSERT(a != c);

        unsigned char* ba = (unsigned char*)a;
        unsigned char* bb = (unsigned char*)b;
        unsigned char* bc = (unsigned char*)c;
        for (size_t i = 0; i < 10; i++) ba[i] = 0xAA;
        for (size_t i = 0; i < 20; i++) bb[i] = 0xBB;
        for (size_t i = 0; i < 30; i++) bc[i] = 0xCC;

        for (size_t i = 0; i < 10; i++) TEST_ASSERT_EQ(ba[i], (unsigned char)0xAA);
        for (size_t i = 0; i < 20; i++) TEST_ASSERT_EQ(bb[i], (unsigned char)0xBB);
        for (size_t i = 0; i < 30; i++) TEST_ASSERT_EQ(bc[i], (unsigned char)0xCC);
    }

    // ---- realloc: growing preserves content ---------------------------------
    {
        unsigned char* p = (unsigned char*)malloc(8);
        TEST_ASSERT(p != NULL);
        for (size_t i = 0; i < 8; i++) p[i] = (unsigned char)(i + 1);

        unsigned char* grown = (unsigned char*)realloc(p, 64);
        TEST_ASSERT(grown != NULL);
        TEST_ASSERT_EQ((uintptr_t)grown % 16, (uintptr_t)0);
        for (size_t i = 0; i < 8; i++) {
            TEST_ASSERT_EQ(grown[i], (unsigned char)(i + 1));
        }
        // EN: The newly-grown tail is at least writable (not a correctness claim about its
        //     contents -- realloc does not zero-fill growth, only libc's calloc would).
        // PT: A cauda recem-crescida e' pelo menos escrivivel (nao e' uma afirmacao de
        //     corretude sobre o conteudo dela -- realloc nao zera o crescimento, so' o
        //     calloc da libc faria isso).
        grown[63] = 0x42;
        TEST_ASSERT_EQ(grown[63], (unsigned char)0x42);
    }

    // EN: realloc(NULL, n) behaves exactly like malloc(n).
    // PT: realloc(NULL, n) se comporta exatamente como malloc(n).
    {
        void* p = realloc(NULL, 32);
        TEST_ASSERT(p != NULL);
        TEST_ASSERT_EQ((uintptr_t)p % 16, (uintptr_t)0);
    }

    // EN: realloc(p, 0) -- this project's documented choice: behaves like free(p) (a no-op)
    //     and returns NULL.
    // PT: realloc(p, 0) -- a escolha documentada deste projeto: se comporta como free(p) (um
    //     no-op) e retorna NULL.
    {
        void* p = malloc(16);
        TEST_ASSERT(p != NULL);
        void* ret = realloc(p, 0);
        TEST_ASSERT(ret == NULL);
    }

    // ---- free: safe no-op ----------------------------------------------------
    {
        free(NULL);
        void* p = malloc(8);
        TEST_ASSERT(p != NULL);
        free(p);
        // EN: Leak-by-design (ADR-0004): the memory is not reused, but touching it after
        //     free() must not crash (it is still a valid, mapped page -- free() never
        //     unmaps/decommits anything).
        // PT: Leak-by-design (ADR-0004): a memoria nao e' reusada, mas toca-la depois do
        //     free() nao pode travar (ainda e' uma pagina valida, mapeada -- free() nunca
        //     desmapeia/descompromete nada).
        ((unsigned char*)p)[0] = 0x99;
        TEST_ASSERT_EQ(((unsigned char*)p)[0], (unsigned char)0x99);
    }

    // ---- malloc(0): this project's documented choice = NULL ------------------
    {
        void* p = malloc(0);
        TEST_ASSERT(p == NULL);
    }

    // ---- arena exhaustion: force a second mmap'd arena ------------------------
    // EN: The bootstrap arena is 1 MiB (documented in src/alloc.c). Allocate past that in
    //     large chunks to force at least one new arena `mmap`, then prove a pointer obtained
    //     AFTER the rollover is just as valid/writable/aligned as one from the first arena.
    // PT: A arena de bootstrap e' 1 MiB (documentado em src/alloc.c). Aloca alem disso em
    //     pedacos grandes pra forcar pelo menos um novo `mmap` de arena, depois prova que um
    //     ponteiro obtido DEPOIS do rollover e' tao valido/escrivivel/alinhado quanto um da
    //     primeira arena.
    {
        const size_t chunk       = 128 * 1024; /* 128 KiB per chunk */
        const size_t chunk_count = 16;         /* 16 * 128 KiB = 2 MiB > the 1 MiB arena */
        void* last = NULL;
        for (size_t i = 0; i < chunk_count; i++) {
            void* p = malloc(chunk);
            TEST_ASSERT(p != NULL);
            TEST_ASSERT_EQ((uintptr_t)p % 16, (uintptr_t)0);
            last = p;
        }
        TEST_ASSERT(last != NULL);
        unsigned char* lb = (unsigned char*)last;
        lb[0]          = 0x7E;
        lb[chunk - 1]  = 0x7F;
        TEST_ASSERT_EQ(lb[0], (unsigned char)0x7E);
        TEST_ASSERT_EQ(lb[chunk - 1], (unsigned char)0x7F);
    }

    // ---- a single oversized request also works (own dedicated arena) ---------
    {
        const size_t big = 4 * 1024 * 1024; /* 4 MiB, bigger than the 1 MiB default arena */
        unsigned char* p = (unsigned char*)malloc(big);
        TEST_ASSERT(p != NULL);
        TEST_ASSERT_EQ((uintptr_t)p % 16, (uintptr_t)0);
        p[0]       = 0x11;
        p[big - 1] = 0x22;
        TEST_ASSERT_EQ(p[0], (unsigned char)0x11);
        TEST_ASSERT_EQ(p[big - 1], (unsigned char)0x22);
    }

    TEST_PASS();
}
