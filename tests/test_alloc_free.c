// SPDX-License-Identifier: MPL-2.0
// EN: SOV-ALLOC (W15) gate program -- TDD RED first: written against the DOCUMENTED contract
//     of the free-list rewrite (include/alloc.h, src/alloc.c's file header) BEFORE any of it
//     existed in src/alloc.c -- these are exactly the cases the bootstrap bump allocator (a
//     leak-by-design no-op `free()`, ADR-0004's initial increment) could NOT pass: (a) a freed
//     block is REUSED by a later malloc() of the same size, not left permanently abandoned; (b)
//     two adjacent freed blocks COALESCE into one, provably reusable by a request that fits the
//     merged region but not either half alone, while a live third neighbour stays untouched;
//     (c) a small allocation carved from a freed large block SPLITS rather than consuming the
//     whole thing, leaving the remainder available; (d) alloc/free churn with unique
//     per-block byte patterns never corrupts a block that is still LIVE; (e) a REUSED block
//     (recovered from the free-list, not freshly mmap'd) is still 16-byte aligned. A final
//     stretch case exercises the OPTIONAL munmap-of-empty-arena path (see src/alloc.c) without
//     asserting on it directly (there is no `open`/`/proc` reader in this zero-libc codebase to
//     introspect the address space from inside the test program itself -- `strace` is the
//     external oracle for that, run manually per this feature's task brief).
// PT: Programa-gate do SOV-ALLOC (W15) -- TDD VERMELHO primeiro: escrito contra o contrato
//     DOCUMENTADO da reescrita free-list (include/alloc.h, cabeçalho de arquivo de src/alloc.c)
//     ANTES de qualquer parte dele existir em src/alloc.c -- estes são exatamente os casos que
//     o bump allocator de bootstrap (um `free()` no-op leak-by-design, incremento inicial da
//     ADR-0004) NÃO conseguia passar: (a) um bloco liberado é REUSADO por um malloc() posterior
//     do mesmo tamanho, não deixado abandonado pra sempre; (b) dois blocos liberados adjacentes
//     COALESCEM num só, provavelmente reusável por um pedido que cabe na região mesclada mas não
//     em nenhuma metade sozinha, enquanto um terceiro vizinho vivo fica intocado; (c) uma
//     alocação pequena esculpida de um bloco grande liberado FAZ SPLIT em vez de consumir o
//     bloco inteiro, deixando o restante disponível; (d) churn de alloc/free com padrões de
//     byte únicos por bloco nunca corrompe um bloco que ainda está VIVO; (e) um bloco REUSADO
//     (recuperado da free-list, não recém-mmap'ado) ainda está alinhado a 16 bytes. Um caso
//     final de stretch exercita o caminho OPCIONAL de munmap-de-arena-vazia (ver src/alloc.c)
//     sem assertar sobre ele diretamente (não há leitor `open`/`/proc` neste código-base
//     zero-libc pra introspectar o espaço de endereço de dentro do próprio programa de teste --
//     `strace` é o oráculo externo pra isso, rodado manualmente conforme o brief desta feature).
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "alloc.h"
#include "types.h"

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    // ---- (a) reuse: free()d block is handed back out by a later malloc() ----
    // EN: A fresh process (own arena/free-list state -- each tests/*.c is its own program, see
    //     Makefile): the very first malloc(64) carves its block off the front of a brand-new
    //     arena (see src/alloc.c -- a freshly mmap'd arena is formatted as one big free block).
    //     After free(p), that block is the ONLY entry in the free-list; a second malloc(64)
    //     must therefore find and reuse the EXACT SAME address (first-fit over a single
    //     candidate).
    // PT: Um processo novo (estado próprio de arena/free-list -- cada tests/*.c é seu próprio
    //     programa, ver Makefile): o primeiríssimo malloc(64) esculpe seu bloco da frente de
    //     uma arena recém-mmap'ada (ver src/alloc.c -- uma arena recém-mmap'ada é formatada
    //     como um único bloco livre grande). Depois do free(p), aquele bloco é a ÚNICA entrada
    //     da free-list; um segundo malloc(64) portanto precisa achar e reusar o MESMO endereço
    //     exato (first-fit sobre um único candidato).
    {
        void* p = malloc(64);
        TEST_ASSERT(p != NULL);
        unsigned char* bp = (unsigned char*)p;
        for (size_t i = 0; i < 64; i++) bp[i] = (unsigned char)(0xA0 + i);

        free(p);

        void* q = malloc(64);
        TEST_ASSERT(q != NULL);
        TEST_ASSERT(q == p);
        TEST_ASSERT_EQ((uintptr_t)q % 16, (uintptr_t)0);
    }

    // ---- (b) coalescence: two adjacent freed blocks merge into one ------------
    // EN: Three contiguous 64-byte allocations (a, b, c -- carved back-to-back off the same
    //     free block, no gaps -- see src/alloc.c's split logic). Free a then b: they are
    //     PHYSICALLY adjacent, so freeing b must forward-coalesce with a (already free),
    //     merging into one region big enough for a request that would NOT fit in either 64-byte
    //     block alone. `c` is never freed -- it stays live throughout, proving coalescing never
    //     touches a still-allocated neighbour.
    // PT: Três alocações contíguas de 64 bytes (a, b, c -- esculpidas lado a lado do mesmo
    //     bloco livre, sem lacunas -- ver a lógica de split de src/alloc.c). Libera a depois b:
    //     eles são FISICAMENTE adjacentes, então liberar b precisa coalescer pra frente com a
    //     (já livre), mesclando numa região grande o bastante pra um pedido que NÃO caberia em
    //     nenhum bloco de 64 bytes sozinho. `c` nunca é liberado -- fica vivo o tempo todo,
    //     provando que a coalescência nunca toca um vizinho ainda alocado.
    {
        unsigned char* a = (unsigned char*)malloc(64);
        unsigned char* b = (unsigned char*)malloc(64);
        unsigned char* c = (unsigned char*)malloc(64);
        TEST_ASSERT(a != NULL);
        TEST_ASSERT(b != NULL);
        TEST_ASSERT(c != NULL);
        for (size_t i = 0; i < 64; i++) c[i] = (unsigned char)0xCC;

        free(a);
        free(b);

        // EN: 150 bytes needs a total footprint of 176 bytes (header+payload+footer, aligned)
        //     -- bigger than either a or b's own ~96-byte total alone, but well within their
        //     merged ~192-byte region.
        // PT: 150 bytes precisa de um footprint total de 176 bytes (header+payload+footer,
        //     alinhado) -- maior que o total de ~96 bytes de a ou b sozinhos, mas bem dentro da
        //     região mesclada de ~192 bytes dos dois.
        unsigned char* d = (unsigned char*)malloc(150);
        TEST_ASSERT(d != NULL);
        TEST_ASSERT(d == a);
        TEST_ASSERT_EQ((uintptr_t)d % 16, (uintptr_t)0);

        // EN: `c` was never touched by any free()/coalesce -- its content must be exactly what
        //     was stamped before a/b were ever freed.
        // PT: `c` nunca foi tocado por nenhum free()/coalescência -- o conteúdo dele precisa
        //     ser exatamente o que foi carimbado antes de a/b serem liberados.
        for (size_t i = 0; i < 64; i++) TEST_ASSERT_EQ(c[i], (unsigned char)0xCC);
    }

    // ---- (c) split: a small alloc from a freed large block does not waste it --
    // EN: A single big (4 KiB) allocation, freed, then a tiny (16-byte) one carved from it --
    //     `small` must reuse `big`'s exact start address (single free-list candidate), and a
    //     THIRD, mid-sized request must land INSIDE the original 4 KiB region (proving the
    //     small allocation only carved off what it needed -- had it consumed the whole freed
    //     block, `other` would have been forced into a brand-new, far-away mmap'd arena
    //     instead).
    // PT: Uma única alocação grande (4 KiB), liberada, depois uma minúscula (16 bytes)
    //     esculpida dela -- `small` precisa reusar o endereço inicial exato de `big` (único
    //     candidato de free-list), e um TERCEIRO pedido de tamanho médio precisa cair DENTRO da
    //     região original de 4 KiB (provando que a alocação pequena só esculpiu o que
    //     precisava -- se tivesse consumido o bloco liberado inteiro, `other` teria sido forçado
    //     pra uma arena `mmap`ada nova e distante em vez disso).
    {
        const size_t big_size = 4096;
        unsigned char* big = (unsigned char*)malloc(big_size);
        TEST_ASSERT(big != NULL);
        free(big);

        unsigned char* small = (unsigned char*)malloc(16);
        TEST_ASSERT(small != NULL);
        TEST_ASSERT(small == big);
        TEST_ASSERT_EQ((uintptr_t)small % 16, (uintptr_t)0);

        unsigned char* other = (unsigned char*)malloc(2000);
        TEST_ASSERT(other != NULL);
        TEST_ASSERT(other != small);
        TEST_ASSERT(other >= big);
        TEST_ASSERT(other < big + big_size);
    }

    // ---- (d) churn: interleaved alloc/free with unique patterns, live blocks never
    //     corrupted -----------------------------------------------------------
    // EN: 8 blocks of varied sizes, each stamped with a UNIQUE byte pattern. Free every ODD
    //     index, verify every EVEN one is untouched (proves freeing odd blocks -- which forces
    //     coalescing among themselves and/or with virgin arena remainder -- never reaches into
    //     a still-live even block). Reallocate into the freed slots with DIFFERENT sizes and
    //     DIFFERENT patterns (exercises split/first-fit reuse under fragmentation), then
    //     re-verify ALL 8 blocks -- both the freshly-reallocated odd ones and the untouched-
    //     since-the-start even ones -- hold exactly their current expected pattern.
    // PT: 8 blocos de tamanhos variados, cada um carimbado com um padrão de byte ÚNICO. Libera
    //     todo índice ÍMPAR, verifica que todo índice PAR está intocado (prova que liberar os
    //     blocos ímpares -- o que força coalescência entre eles e/ou com o restante de arena
    //     virgem -- nunca alcança um bloco par ainda vivo). Realoca nos slots liberados com
    //     tamanhos DIFERENTES e padrões DIFERENTES (exercita reuso split/first-fit sob
    //     fragmentação), depois reconfere os 8 blocos -- tanto os ímpares recém-realocados
    //     quanto os pares intocados desde o início -- guardam exatamente seu padrão atual
    //     esperado.
    {
        const size_t n = 8;
        unsigned char* blocks[8];
        size_t         sizes[8]    = {24, 40, 96, 8, 200, 32, 500, 16};
        unsigned char  patterns[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

        for (size_t i = 0; i < n; i++) {
            blocks[i] = (unsigned char*)malloc(sizes[i]);
            TEST_ASSERT(blocks[i] != NULL);
            for (size_t j = 0; j < sizes[i]; j++) blocks[i][j] = patterns[i];
        }

        for (size_t i = 1; i < n; i += 2) {
            free(blocks[i]);
        }

        for (size_t i = 0; i < n; i += 2) {
            for (size_t j = 0; j < sizes[i]; j++) {
                TEST_ASSERT_EQ(blocks[i][j], patterns[i]);
            }
        }

        size_t         new_sizes[8]    = {0, 12, 0, 300, 0, 4, 0, 64};
        unsigned char  new_patterns[8] = {0, 0xAA, 0, 0xBB, 0, 0xCC, 0, 0xDD};
        for (size_t i = 1; i < n; i += 2) {
            blocks[i]     = (unsigned char*)malloc(new_sizes[i]);
            sizes[i]      = new_sizes[i];
            patterns[i]   = new_patterns[i];
            TEST_ASSERT(blocks[i] != NULL);
            TEST_ASSERT_EQ((uintptr_t)blocks[i] % 16, (uintptr_t)0);
            for (size_t j = 0; j < sizes[i]; j++) blocks[i][j] = patterns[i];
        }

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < sizes[i]; j++) {
                TEST_ASSERT_EQ(blocks[i][j], patterns[i]);
            }
        }

        for (size_t i = 0; i < n; i++) {
            free(blocks[i]);
        }
    }

    // ---- (e) alignment: a block RECOVERED from the free-list is still 16B-aligned --
    // EN: Belt-and-suspenders on top of the implicit checks in (a)-(d) above: force a
    //     malloc -> free -> malloc(different size) cycle through the SAME free-list entry
    //     (first-fit, single candidate again) and check the reused pointer explicitly.
    // PT: Cinto-e-suspensório em cima das checagens implícitas de (a)-(d) acima: força um ciclo
    //     malloc -> free -> malloc(tamanho diferente) pela MESMA entrada de free-list
    //     (first-fit, único candidato de novo) e checa o ponteiro reusado explicitamente.
    {
        void* p = malloc(100);
        TEST_ASSERT(p != NULL);
        free(p);
        void* q = malloc(40);
        TEST_ASSERT(q != NULL);
        TEST_ASSERT(q == p);
        TEST_ASSERT_EQ((uintptr_t)q % 16, (uintptr_t)0);
    }

    // ---- stretch: freeing a whole dedicated arena is safe (munmap path, if built) --
    // EN: A request bigger than the default arena size gets its own dedicated arena (mirrors
    //     the old bump allocator's behaviour, see src/alloc.c). Freeing its single occupant
    //     coalesces the block back to the arena's ENTIRE usable region -- the OPTIONAL
    //     munmap-of-empty-arena path in src/alloc.c's free(), if present, returns that memory
    //     to the kernel right here. This test does not (and safely cannot, see file header)
    //     assert the unmap happened -- it only proves the allocator survives it and keeps
    //     working: a fresh allocation right after must still succeed. (Verified independently
    //     via `strace` per this feature's task brief: `munmap` shows up in the trace.)
    // PT: Um pedido maior que o tamanho de arena padrão ganha sua própria arena dedicada
    //     (espelha o comportamento do bump allocator antigo, ver src/alloc.c). Liberar o único
    //     ocupante dela coalesce o bloco de volta pra região utilizável INTEIRA da arena -- o
    //     caminho OPCIONAL de munmap-de-arena-vazia em free() de src/alloc.c, se presente,
    //     devolve aquela memória ao kernel bem aqui. Este teste não assere (e não pode
    //     seguramente, ver cabeçalho do arquivo) que o unmap aconteceu -- só prova que o
    //     alocador sobrevive a ele e continua funcionando: uma alocação nova logo depois ainda
    //     precisa ter sucesso. (Verificado independentemente via `strace` conforme o brief desta
    //     feature: `munmap` aparece no trace.)
    {
        const size_t big = 4 * 1024 * 1024; /* bigger than the 1 MiB default arena */
        void* p = malloc(big);
        TEST_ASSERT(p != NULL);
        free(p);

        void* q = malloc(64);
        TEST_ASSERT(q != NULL);
        TEST_ASSERT_EQ((uintptr_t)q % 16, (uintptr_t)0);
        ((unsigned char*)q)[0] = 0x7A;
        TEST_ASSERT_EQ(((unsigned char*)q)[0], (unsigned char)0x7A);
    }

    TEST_PASS();
}
