// SPDX-License-Identifier: MPL-2.0
// EN: E1 -- see include/alloc.h for the full public contract. This file is the internal
//     bump-over-mmap strategy ADR-0004 chose for bootstrap; everything below is free to change
//     later (e.g. a real free-list) without touching alloc.h.
//
//     Arena strategy: a global bump pointer (`g_bump`) walks forward inside the CURRENT arena
//     (`[g_arena_base, g_arena_end)`), a single `mmap` anonymous mapping. When a request does
//     not fit in what is left of the current arena, a NEW arena is `mmap`'d and becomes
//     current -- the old arena's already-handed-out pointers stay perfectly valid (they point
//     into a still-mapped region; nothing is ever unmapped, consistent with `free` being a
//     no-op) but the bump pointer never goes back to them. `ARENA_SIZE` (1 MiB) is a bootstrap
//     guess, not a tuned constant: anonymous `mmap` is lazily backed by the kernel (pages are
//     only committed to physical RAM on first touch/page-fault), so reserving 1 MiB of address
//     space per arena costs effectively nothing until it's written to -- large enough to
//     amortize the `mmap` syscall's own overhead across many small allocations, small enough
//     that a program making just one or two tiny allocations does not look wasteful in
//     `pmap`/`/proc/self/maps`. A single oversized request (bigger than `ARENA_SIZE`) gets its
//     own dedicated arena sized exactly to fit it (see `arena_size_for`) -- it does not force
//     every future small allocation into a giant arena.
//
//     Per-allocation header: the bump strategy has no free-list bookkeeping to recover a
//     block's size later, but `realloc` MUST know the old size to decide how many bytes to
//     copy. The fix is a fixed-size header living immediately BEFORE the pointer handed to the
//     caller: `alloc_header_t` is exactly two `size_t` fields (16 bytes on this LP64 target --
//     see types.h) even though only the first (`size`) is meaningful; the second exists purely
//     so `sizeof(alloc_header_t) == 16`, keeping the user-visible pointer 16-byte aligned
//     whenever the header's own address is 16-byte aligned. Every block's total footprint
//     (header + body) is rounded up to a multiple of 16 (`align_up`) so the NEXT header in the
//     same arena also lands 16-aligned -- alignment is therefore an invariant maintained by
//     construction across the whole bump sequence, not re-derived per call. The arena's own
//     base address is whatever `mmap` returns, which the kernel always page-aligns (4096 on
//     x86-64) -- far more than the 16 bytes required, so the very first header in a fresh
//     arena starts aligned too.
//
//     Integer-overflow guard: `size + sizeof(header)` rounded up to 16 could wrap around
//     `size_t` for a maliciously/accidentally huge `size` (close to `SIZE_MAX`). `malloc`
//     rejects any `size` that would make that arithmetic overflow BEFORE doing any of it,
//     returning `NULL` -- cheap defense-in-depth against a wraparound that would otherwise
//     hand back a too-small buffer the caller believes is huge (a classic integer-overflow-to-
//     heap-overflow bug class). Flagged for `AUD-SEC` (W12) to re-verify independently.
// PT: E1 -- ver include/alloc.h pro contrato público completo. Este arquivo é a estratégia
//     interna bump-sobre-mmap que a ADR-0004 escolheu pro bootstrap; tudo abaixo é livre pra
//     mudar depois (ex.: uma free-list de verdade) sem tocar em alloc.h.
//
//     Estratégia de arena: um ponteiro-bump global (`g_bump`) caminha pra frente dentro da
//     arena ATUAL (`[g_arena_base, g_arena_end)`), um único mapeamento `mmap` anônimo. Quando
//     um pedido não cabe no que sobra da arena atual, uma arena NOVA é `mmap`'ada e vira a
//     atual -- os ponteiros já entregues da arena antiga continuam perfeitamente válidos
//     (apontam pra uma região ainda mapeada; nada é desmapeado nunca, consistente com `free`
//     ser no-op) mas o ponteiro-bump nunca volta pra eles. `ARENA_SIZE` (1 MiB) é um chute de
//     bootstrap, não uma constante afinada: `mmap` anônimo é lastreado preguiçosamente pelo
//     kernel (páginas só são comprometidas em RAM física no primeiro toque/page-fault), então
//     reservar 1 MiB de espaço de endereço por arena custa efetivamente nada até ser escrito --
//     grande o bastante pra amortizar o custo da própria syscall `mmap` ao longo de muitas
//     alocações pequenas, pequeno o bastante pra um programa que faz só uma ou duas alocações
//     minúsculas não parecer perdulário em `pmap`/`/proc/self/maps`. Um pedido único grande
//     demais (maior que `ARENA_SIZE`) ganha sua própria arena dedicada, dimensionada
//     exatamente pra caber (ver `arena_size_for`) -- não força toda alocação pequena futura
//     numa arena gigante.
//
//     Header por alocação: a estratégia bump não tem contabilidade de free-list pra recuperar
//     o tamanho de um bloco depois, mas o `realloc` PRECISA saber o tamanho antigo pra decidir
//     quantos bytes copiar. A solução é um header de tamanho fixo morando imediatamente ANTES
//     do ponteiro entregue a quem chamou: `alloc_header_t` é exatamente dois campos `size_t`
//     (16 bytes neste alvo LP64 -- ver types.h) mesmo que só o primeiro (`size`) seja
//     significativo; o segundo existe puramente pra que `sizeof(alloc_header_t) == 16`,
//     mantendo o ponteiro visível ao usuário alinhado a 16 bytes sempre que o próprio endereço
//     do header esteja alinhado a 16. O footprint total de todo bloco (header + corpo) é
//     arredondado pra cima pra um múltiplo de 16 (`align_up`) pra que o PRÓXIMO header na mesma
//     arena também caia alinhado a 16 -- o alinhamento é, portanto, um invariante mantido por
//     construção ao longo de toda a sequência bump, não re-derivado a cada chamada. O
//     endereço-base da própria arena é o que `mmap` retornar, que o kernel sempre alinha a
//     página (4096 em x86-64) -- bem mais que os 16 bytes exigidos, então o primeiríssimo
//     header numa arena nova também começa alinhado.
//
//     Guarda de overflow de inteiro: `size + sizeof(header)` arredondado pra 16 poderia dar
//     wraparound em `size_t` pra um `size` maliciosa/acidentalmente enorme (perto de
//     `SIZE_MAX`). O `malloc` rejeita qualquer `size` que faria essa aritmética estourar ANTES
//     de fazer qualquer parte dela, retornando `NULL` -- defesa em profundidade barata contra
//     um wraparound que de outra forma devolveria um buffer pequeno demais que quem chamou
//     acredita ser enorme (uma classe clássica de bug integer-overflow-vira-heap-overflow).
//     Sinalizado pro `AUD-SEC` (W12) reverificar de forma independente.
// Copyright (c) 2026 Petrus Silva Costa
#include "alloc.h"
#include "mem.h"
#include "mmap.h"
#include "sys_mmap.h"

// EN: Per-allocation header, exactly 16 bytes on this LP64 target (two `size_t` fields) so the
//     user pointer right after it stays 16-byte aligned. `_reserved` is deliberately unused --
//     it is padding, not a second piece of bookkeeping (kept as a named field, not raw
//     `char _pad[8]`, only because a named `size_t` reads slightly clearer next to `size`;
//     either would work).
// PT: Header por alocação, exatamente 16 bytes neste alvo LP64 (dois campos `size_t`) pra que
//     o ponteiro do usuário logo depois dele fique alinhado a 16 bytes. `_reserved` é
//     deliberadamente não-usado -- é padding, não uma segunda peça de contabilidade (mantido
//     como campo nomeado, não `char _pad[8]` cru, só porque um `size_t` nomeado lê um pouco
//     mais claro ao lado de `size`; qualquer um funcionaria).
typedef struct {
    size_t size;
    size_t _reserved;
} alloc_header_t;

#define ALLOC_ALIGN       ((size_t)16)
#define ALLOC_HEADER_SIZE (sizeof(alloc_header_t))
#define ARENA_SIZE        ((size_t)(1 * 1024 * 1024)) /* 1 MiB, see file header rationale */

// EN: Current arena's bump cursor and its exclusive end. NULL/0 before the first `malloc` call
//     ever runs -- `g_bump == g_arena_end == NULL` is a valid, deliberately-unmapped "no arena
//     yet" state that the fits-check below handles correctly (an empty range never "fits" a
//     request of size > 0, so it always takes the mmap-a-new-arena path on the very first
//     call).
// PT: Cursor-bump da arena atual e seu fim exclusivo. NULL/0 antes da primeiríssima chamada de
//     `malloc` rodar -- `g_bump == g_arena_end == NULL` é um estado válido, deliberadamente
//     sem-mapa de "ainda sem arena" que a checagem de cabimento abaixo trata corretamente (uma
//     faixa vazia nunca "cabe" um pedido de tamanho > 0, então sempre toma o caminho de
//     mmap-uma-arena-nova na primeiríssima chamada).
static unsigned char* g_bump      = NULL;
static unsigned char* g_arena_end = NULL;

// EN: Rounds `n` up to the next multiple of `align` (`align` must be a power of two -- true for
//     every call site in this file, always `ALLOC_ALIGN`==16).
// PT: Arredonda `n` pra cima pro próximo múltiplo de `align` (`align` precisa ser potência de
//     dois -- verdade em todo ponto de chamada deste arquivo, sempre `ALLOC_ALIGN`==16).
static size_t align_up(size_t n, size_t align) {
    return (n + (align - 1)) & ~(align - 1);
}

// EN: Raw mmap-failure check per ADR-0002/sys_mmap.h: the kernel returns either a real mapped
//     address or a negative `-errno` packed into the same pointer-sized value, in the range
//     `[-4095, -1]`. Read as unsigned, any real address on a sane x86-64 process is nowhere
//     near `(unsigned long)-4095` (the top of the address space), so this check is unambiguous
//     without needing libc's `MAP_FAILED` sentinel (which does not apply to this raw wrapper).
// PT: Checagem de falha crua do mmap conforme ADR-0002/sys_mmap.h: o kernel retorna ou um
//     endereço mapeado de verdade ou um `-errno` negativo empacotado no mesmo valor do tamanho
//     de um ponteiro, na faixa `[-4095, -1]`. Lido como unsigned, nenhum endereço de verdade
//     num processo x86-64 são chega perto de `(unsigned long)-4095` (o topo do espaço de
//     endereços), então esta checagem é inambígua sem precisar da sentinela `MAP_FAILED` da
//     libc (que não se aplica a este wrapper cru).
static int mmap_failed(const void* ret) {
    return (unsigned long)ret >= (unsigned long)-4095;
}

// EN: Picks how big a NEW arena must be to satisfy a request whose total (header + body,
//     already 16-aligned) footprint is `total`: the bootstrap default `ARENA_SIZE`, or exactly
//     `total` itself when the request alone is bigger than the default (see file header --
//     this is what keeps one giant allocation from inflating every future small one).
// PT: Escolhe o tamanho que uma arena NOVA precisa ter pra satisfazer um pedido cujo footprint
//     total (header + corpo, já alinhado a 16) é `total`: o padrão de bootstrap `ARENA_SIZE`,
//     ou exatamente `total` quando o pedido sozinho é maior que o padrão (ver cabeçalho do
//     arquivo -- é isso que impede uma alocação gigante de inflar toda pequena futura).
static size_t arena_size_for(size_t total) {
    return (total > ARENA_SIZE) ? total : ARENA_SIZE;
}

void* malloc(size_t size) {
    // EN: malloc(0) returns NULL -- see include/alloc.h for the documented rationale.
    // PT: malloc(0) retorna NULL -- ver include/alloc.h pro racional documentado.
    if (size == 0) {
        return NULL;
    }

    // EN: Overflow guard (see file header): reject before computing `total` if
    //     `size + ALLOC_HEADER_SIZE`, once rounded up to ALLOC_ALIGN, would wrap `size_t`.
    // PT: Guarda de overflow (ver cabeçalho do arquivo): rejeita antes de computar `total` se
    //     `size + ALLOC_HEADER_SIZE`, uma vez arredondado pra ALLOC_ALIGN, estouraria `size_t`.
    if (size > (size_t)-1 - ALLOC_HEADER_SIZE - (ALLOC_ALIGN - 1)) {
        return NULL;
    }

    size_t total = align_up(ALLOC_HEADER_SIZE + size, ALLOC_ALIGN);

    // EN: Does the current arena (possibly none yet -- see g_bump's doc comment) have `total`
    //     bytes left? If not, mmap a new one and make it current.
    // PT: A arena atual (possivelmente nenhuma ainda -- ver doc-comment do g_bump) tem `total`
    //     bytes sobrando? Se não, mmap uma nova e faz ela virar a atual.
    if (g_bump == NULL || (size_t)(g_arena_end - g_bump) < total) {
        size_t new_arena_size = arena_size_for(total);
        void* mem = sys_mmap(NULL, new_arena_size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mmap_failed(mem)) {
            return NULL;
        }
        g_bump      = (unsigned char*)mem;
        g_arena_end = g_bump + new_arena_size;
    }

    alloc_header_t* hdr = (alloc_header_t*)g_bump;
    hdr->size      = size;
    hdr->_reserved = 0;

    void* user_ptr = (void*)(g_bump + ALLOC_HEADER_SIZE);
    g_bump += total;
    return user_ptr;
}

void free(void* ptr) {
    // EN: No-op by design -- see include/alloc.h and ADR-0004. Safe with ptr == NULL (there is
    //     nothing to branch on: doing literally nothing is trivially NULL-safe).
    // PT: No-op por desenho -- ver include/alloc.h e a ADR-0004. Seguro com ptr == NULL (não há
    //     nada pra ramificar: não fazer literalmente nada é trivialmente seguro com NULL).
    (void)ptr;
}

void* realloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // EN: Recover the old block's size from its header (immediately before `ptr` -- see file
    //     header). Must read this BEFORE calling malloc(size): if the new allocation fails, we
    //     return NULL having touched neither `ptr` nor its header, per realloc's contract.
    // PT: Recupera o tamanho do bloco antigo a partir do header dele (imediatamente antes de
    //     `ptr` -- ver cabeçalho do arquivo). Precisa ler isso ANTES de chamar malloc(size): se
    //     a alocação nova falhar, retornamos NULL sem ter tocado nem `ptr` nem seu header,
    //     conforme o contrato do realloc.
    const alloc_header_t* old_hdr =
        (const alloc_header_t*)((const unsigned char*)ptr - ALLOC_HEADER_SIZE);
    size_t old_size = old_hdr->size;

    void* new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}
