// SPDX-License-Identifier: MPL-2.0
// EN: E1/SOV-ALLOC -- see include/alloc.h for the full public contract. This file is the
//     internal strategy ADR-0004 always anticipated evolving to: a real free-list allocator
//     with boundary-tag coalescing over `mmap`-backed arenas. `free` genuinely returns memory
//     for reuse now (it was a leak-by-design no-op before SOV-ALLOC, W15); the public
//     `malloc`/`free`/`realloc` signatures have NOT changed (that was the whole point of fixing
//     them in ADR-0004).
//
//     DESIGN TRADE-OFF (documented per SOV-ALLOC's brief): a SINGLE, unsorted, first-fit
//     free-list -- NOT a segregated/binned free-list (e.g. power-of-two size classes, each with
//     its own list head). Segregated fit gives closer-to-O(1) allocation by jumping straight to
//     a bin likely to fit, at the cost of more bookkeeping (N list heads, size-class rounding,
//     cross-bin splitting/coalescing edge cases) and more code surface to get right in a
//     from-scratch, zero-libc codebase with no fuzzer yet. This increment's real workload (this
//     file's own test suite today; the eventual FreeType/RmlUi internalization tomorrow, per
//     AUD-TRANS-0) is not yet profiled. A single first-fit list is O(number of free blocks
//     currently outstanding) per `malloc` -- the correct trade for CORRECTNESS-FIRST bootstrap
//     (fewer invariants to prove, easier to review adversarially) at a bounded, revisitable
//     performance cost. Upgrading to segregated/best-fit/a balanced-tree free-list later is the
//     SAME kind of internal-only change ADR-0004 always intended -- behind this exact
//     `malloc`/`free`/`realloc` signature, zero caller-visible change.
//
//     BLOCK LAYOUT (every block, free or allocated, in every arena):
//
//         [ block_header_t (16B) ][ ... payload ... ][ block_footer_t (8B) ]
//         ^ user pointer returned by malloc() points HERE + 16
//
//     `block_header_t` is exactly 16 bytes on this LP64 target (one `size_t` + one pointer),
//     which is also `ALLOC_ALIGN` -- so the user pointer right after it is always 16-byte
//     aligned whenever the header's own address is (true by construction: every block's total
//     footprint is rounded up to a multiple of 16, so headers only ever land on 16-aligned
//     addresses, all the way from an arena's own page-aligned base). `size_and_flags` packs the
//     block's TOTAL footprint (header+payload+footer, always a multiple of `ALLOC_ALIGN`) in
//     its high bits and a single `FREE_BIT` in bit 0 -- safe because a multiple of 16 never has
//     that bit set on its own. `arena` points back at the owning arena's descriptor (see
//     below) -- an O(1) way to find that arena's `[base, end)` bounds during coalescing,
//     without a global arena search on every `free()`.
//
//     `block_footer_t` (8 bytes, just a mirror of `size_and_flags`) sits at the END of every
//     block. This is the classic boundary-tag trick: to coalesce a freed block with its
//     PHYSICALLY PRECEDING neighbour, read the footer that must live immediately before this
//     block's own header (`this_header - 8`) -- its `size_and_flags` says both whether that
//     neighbour is free AND, if so, exactly how far back its own header starts (subtract its
//     size). Forward coalescing (with the block that starts right after this one ends) does not
//     need a footer lookup -- the next header's address is simply `this_header + this_size`.
//
//     EXPLICIT FREE-LIST, links borrowed from the payload: a free block additionally has a
//     `free_link_t` (two pointers, `prev`/`next`) written into the FIRST 16 bytes of its
//     payload region (payload is otherwise unused while free -- nothing but the header/footer
//     touch a free block). This is why every block's total footprint has an enforced minimum
//     (`MIN_BLOCK_TOTAL`, see below): a block too small to ever hold a `free_link_t` once freed
//     would corrupt whichever OTHER free block happens to sit at that address the next time the
//     list is walked.
//
//     ARENA DESCRIPTOR embedded at the front of every `mmap`'d region (`arena_desc_t`, 32
//     bytes = 4 `size_t`-sized fields, itself 16-aligned since `mmap` always returns a
//     page-aligned address): `next` chains all arenas into one singly-linked list (`g_arenas`,
//     for finding+unlinking an arena when it becomes fully empty -- see the `munmap` note
//     below); `base`/`end` are this arena's OWN `[base, end)` usable-region bounds (i.e. AFTER
//     the descriptor itself), the exact boundary coalescing must never read across (two
//     different `mmap` calls are not guaranteed to land in adjacent address ranges -- treating
//     one arena's tail as if it bordered another arena's head would read/write memory that may
//     not even be mapped). A freshly `mmap`'d arena is formatted as ONE BIG FREE BLOCK spanning
//     its entire usable region and linked into the free-list immediately -- `malloc` never
//     special-cases "virgin, uncarved arena space" vs. "a block that was freed and coalesced
//     back to arena size": by construction they are the SAME free-list entry.
//
//     MIN_BLOCK_TOTAL (48 bytes): the smallest total footprint any block -- allocated or free
//     -- is ever given. Derived from three requirements stacking up: 16 (header) + 16 (room for
//     a `free_link_t` once freed) + 8 (footer) = 40, rounded up to the next multiple of
//     `ALLOC_ALIGN` (16) = 48. `malloc` enforces this floor on every request's computed total
//     BEFORE searching the free-list; a split during allocation is only performed when the
//     REMAINDER left behind would also clear this floor (see `malloc`'s body) -- otherwise the
//     whole matched free block is handed out as-is (bounded internal fragmentation, never an
//     invalid undersized free block).
//
//     OPTIONAL STRETCH (SOV-ALLOC's brief explicitly allowed skipping this): when `free()`
//     coalesces a block all the way back to spanning its ENTIRE owning arena's `[base, end)` --
//     i.e. that arena now holds zero live allocations -- the arena is unlinked from `g_arenas`,
//     removed from the free-list, and returned to the kernel via `sys_munmap` (a new wrapper +
//     the re-added `syscall2`, see src/sys_munmap.c) instead of being kept mapped forever. This
//     is a pure implementation optimization, invisible to callers except that (as documented in
//     include/alloc.h) touching memory after `free()` is now genuinely unsafe -- it may fault
//     immediately instead of merely risking silent corruption later. KNOWN LIMITATION (not
//     fixed by this increment, acceptable for bootstrap): a workload that repeatedly allocates
//     and frees a block that happens to be its arena's ONLY occupant will thrash `mmap`/`munmap`
//     on every cycle -- no retention/hysteresis policy exists yet. Revisit if a real workload
//     profile shows this cost matters (YAGNI today).
//
//     DOUBLE-FREE: see include/alloc.h's contract section -- UB, with a cheap (already-paid-for)
//     flag check as defense-in-depth, `sys_exit()` on detection rather than silent corruption.
//
//     Integer-overflow guard (unchanged principle from the bump-allocator era, formula updated
//     for the footer's extra 8 bytes): `malloc` rejects any `size` that would make
//     `HEADER + size + FOOTER`, rounded up to `ALLOC_ALIGN`, wrap `size_t` -- BEFORE computing
//     any of that arithmetic, returning `NULL`. Cheap defense-in-depth against a wraparound
//     that would otherwise hand back a too-small buffer the caller believes is huge (a classic
//     integer-overflow-to-heap-overflow bug class).
// PT: E1/SOV-ALLOC -- ver include/alloc.h pro contrato público completo. Este arquivo é a
//     estratégia interna que a ADR-0004 sempre previu evoluir pra: um alocador free-list de
//     verdade com coalescência via boundary tags sobre arenas lastreadas em `mmap`. O `free`
//     agora devolve memória de verdade pra reuso (era um no-op leak-by-design antes do
//     SOV-ALLOC, W15); as assinaturas públicas `malloc`/`free`/`realloc` NÃO mudaram (esse era
//     o ponto inteiro de fixá-las na ADR-0004).
//
//     TRADE-OFF DE DESENHO (documentado conforme o brief do SOV-ALLOC): uma ÚNICA free-list, não
//     ordenada, first-fit -- NÃO uma free-list segregada/em-bins (ex.: classes de tamanho
//     potência-de-dois, cada uma com sua própria cabeça de lista). Fit segregado dá alocação
//     mais perto de O(1) pulando direto pra um bin que provavelmente cabe, ao custo de mais
//     contabilidade (N cabeças de lista, arredondamento de classe de tamanho, casos de borda de
//     split/coalescência entre bins) e mais superfície de código pra acertar num código-base do
//     zero, zero-libc, ainda sem fuzzer. A carga de trabalho real deste incremento (a suíte
//     própria deste arquivo hoje; a futura internalização FreeType/RmlUi amanhã, conforme
//     AUD-TRANS-0) ainda não foi perfilada. Uma única lista first-fit é O(número de blocos
//     livres pendentes atualmente) por `malloc` -- o trade-off correto pra bootstrap
//     CORRETUDE-PRIMEIRO (menos invariantes pra provar, mais fácil de revisar adversarialmente)
//     a um custo de performance limitado e revisitável. Evoluir pra free-list segregada/
//     best-fit/uma árvore balanceada depois é o MESMO tipo de mudança só-interna que a ADR-0004
//     sempre pretendeu -- atrás desta mesma assinatura `malloc`/`free`/`realloc`, zero mudança
//     visível a quem chama.
//
//     LAYOUT DE BLOCO (todo bloco, livre ou alocado, em toda arena):
//
//         [ block_header_t (16B) ][ ... payload ... ][ block_footer_t (8B) ]
//         ^ ponteiro do usuário retornado por malloc() aponta AQUI + 16
//
//     `block_header_t` tem exatamente 16 bytes neste alvo LP64 (um `size_t` + um ponteiro), que
//     também é `ALLOC_ALIGN` -- então o ponteiro do usuário logo depois dele está sempre
//     alinhado a 16 bytes sempre que o próprio endereço do header está (verdade por construção:
//     o footprint total de todo bloco é arredondado pra cima pra um múltiplo de 16, então
//     headers só caem em endereços alinhados a 16, desde a própria base alinhada-a-página de uma
//     arena). `size_and_flags` empacota o footprint TOTAL do bloco (header+payload+footer,
//     sempre múltiplo de `ALLOC_ALIGN`) nos bits altos e um único `FREE_BIT` no bit 0 -- seguro
//     porque um múltiplo de 16 nunca tem esse bit setado sozinho. `arena` aponta de volta pro
//     descritor da arena dona (ver abaixo) -- uma forma O(1) de achar os limites `[base, end)`
//     daquela arena durante a coalescência, sem busca global de arena a cada `free()`.
//
//     `block_footer_t` (8 bytes, só um espelho de `size_and_flags`) fica no FIM de todo bloco.
//     Este é o truque clássico de boundary-tag: pra coalescer um bloco liberado com seu vizinho
//     FISICAMENTE PRECEDENTE, lê o footer que precisa morar imediatamente antes do header deste
//     bloco (`this_header - 8`) -- o `size_and_flags` dele diz tanto se aquele vizinho está
//     livre QUANTO, se estiver, exatamente quão atrás o header dele começa (subtrai o tamanho
//     dele). A coalescência pra frente (com o bloco que começa logo depois deste terminar) não
//     precisa de busca de footer -- o endereço do próximo header é simplesmente
//     `this_header + this_size`.
//
//     FREE-LIST EXPLÍCITA, links emprestados do payload: um bloco livre adicionalmente tem um
//     `free_link_t` (dois ponteiros, `prev`/`next`) escrito nos PRIMEIROS 16 bytes da região de
//     payload dele (payload é de outra forma não-usado enquanto livre -- nada além do
//     header/footer toca um bloco livre). É por isso que o footprint total de todo bloco tem um
//     mínimo forçado (`MIN_BLOCK_TOTAL`, ver abaixo): um bloco pequeno demais pra jamais caber um
//     `free_link_t` uma vez liberado corromperia qualquer OUTRO bloco livre que por acaso esteja
//     naquele endereço na próxima vez que a lista for percorrida.
//
//     DESCRITOR DE ARENA embutido no início de toda região `mmap`ada (`arena_desc_t`, 32 bytes
//     = 4 campos do tamanho de `size_t`, ele mesmo alinhado a 16 já que `mmap` sempre retorna um
//     endereço alinhado a página): `next` encadeia todas as arenas numa única lista
//     simplesmente-ligada (`g_arenas`, pra achar+desligar uma arena quando ela fica totalmente
//     vazia -- ver a nota de `munmap` abaixo); `base`/`end` são os limites `[base, end)` da
//     região utilizável DESTA arena (i.e. DEPOIS do próprio descritor), o limite exato que a
//     coalescência nunca pode ler além -- duas chamadas `mmap` diferentes não têm garantia de
//     cair em faixas de endereço adjacentes (tratar a cauda de uma arena como se fizesse
//     fronteira com o início de outra leria/escreveria memória que pode nem estar mapeada). Uma
//     arena recém-`mmap`ada é formatada como UM ÚNICO BLOCO LIVRE GRANDE cobrindo toda sua
//     região utilizável e encadeada na free-list imediatamente -- o `malloc` nunca trata
//     separadamente "espaço de arena virgem, não-esculpido" de "um bloco que foi liberado e
//     coalescido de volta pro tamanho da arena": por construção são a MESMA entrada de
//     free-list.
//
//     MIN_BLOCK_TOTAL (48 bytes): o menor footprint total que qualquer bloco -- alocado ou livre
//     -- jamais recebe. Derivado de três exigências empilhadas: 16 (header) + 16 (espaço pra um
//     `free_link_t` uma vez liberado) + 8 (footer) = 40, arredondado pra cima pro próximo
//     múltiplo de `ALLOC_ALIGN` (16) = 48. O `malloc` força este piso no total calculado de todo
//     pedido ANTES de buscar na free-list; um split durante alocação só é feito quando o
//     RESTANTE deixado pra trás também cumpriria este piso (ver o corpo do `malloc`) -- senão o
//     bloco livre encontrado inteiro é entregue como está (fragmentação interna limitada, nunca
//     um bloco livre subdimensionado inválido).
//
//     STRETCH OPCIONAL (o brief do SOV-ALLOC permitiu explicitamente pular isto): quando o
//     `free()` coalesce um bloco até cobrir inteiramente o `[base, end)` da arena dona dele --
//     i.e. aquela arena agora não tem nenhuma alocação viva -- a arena é desligada de
//     `g_arenas`, removida da free-list, e devolvida ao kernel via `sys_munmap` (um wrapper novo
//     + a `syscall2` readicionada, ver src/sys_munmap.c) em vez de ficar mapeada pra sempre.
//     Isto é uma otimização de implementação pura, invisível a quem chama exceto que (conforme
//     documentado em include/alloc.h) tocar memória depois do `free()` agora é genuinamente
//     inseguro -- pode faltar imediatamente em vez de só arriscar corrupção silenciosa depois.
//     LIMITAÇÃO CONHECIDA (não corrigida por este incremento, aceitável pro bootstrap): uma
//     carga de trabalho que repetidamente aloca e libera um bloco que por acaso é o ÚNICO
//     ocupante da arena dele vai bater `mmap`/`munmap` a cada ciclo -- nenhuma política de
//     retenção/histerese existe ainda. Revisitar se um perfil de carga real mostrar que este
//     custo importa (YAGNI hoje).
//
//     DOUBLE-FREE: ver a seção de contrato do include/alloc.h -- UB, com uma checagem de flag
//     barata (já paga) como defesa em profundidade, `sys_exit()` na detecção em vez de
//     corrupção silenciosa.
//
//     Guarda de overflow de inteiro (mesmo princípio da era do bump allocator, fórmula
//     atualizada pros 8 bytes extras do footer): o `malloc` rejeita qualquer `size` que faria
//     `HEADER + size + FOOTER`, arredondado pra `ALLOC_ALIGN`, estourar `size_t` -- ANTES de
//     computar qualquer parte dessa aritmética, retornando `NULL`. Defesa em profundidade barata
//     contra um wraparound que de outra forma devolveria um buffer pequeno demais que quem
//     chamou acredita ser enorme (uma classe clássica de bug integer-overflow-vira-heap-
//     overflow).
// Copyright (c) 2026 Petrus Silva Costa
#include "alloc.h"
#include "mem.h"
#include "mmap.h"
#include "sys_mmap.h"
#include "sys_munmap.h"
#include "sys_exit.h"

#define ALLOC_ALIGN ((size_t)16)

// EN: Arena descriptor, embedded at the front of every `mmap`'d region -- see file header.
//     4 `size_t`-sized fields = 32 bytes, itself a multiple of ALLOC_ALIGN. `_reserved` is
//     deliberately unused padding (same convention the old bump-era header used for the same
//     reason: a named field reads clearer than a raw `char _pad[8]`, either works).
// PT: Descritor de arena, embutido no início de toda região `mmap`ada -- ver cabeçalho do
//     arquivo. 4 campos do tamanho de `size_t` = 32 bytes, ele mesmo múltiplo de ALLOC_ALIGN.
//     `_reserved` é padding deliberadamente não-usado (mesma convenção que o header da era
//     bump usava pelo mesmo motivo: um campo nomeado lê mais claro que um `char _pad[8]` cru,
//     qualquer um funcionaria).
typedef struct arena_desc {
    struct arena_desc* next;
    unsigned char*      base; /* usable region start (right after this descriptor) */
    unsigned char*      end;  /* usable region end, exclusive */
    size_t               _reserved;
} arena_desc_t;

#define ARENA_DESC_SIZE ((size_t)sizeof(arena_desc_t))

// EN: Per-block header -- see file header for the full layout rationale. `size_and_flags`
//     packs the block's TOTAL footprint (multiple of ALLOC_ALIGN, so bits [3:0] are otherwise
//     always zero) with FREE_BIT in bit 0.
// PT: Header por bloco -- ver cabeçalho do arquivo pro racional completo do layout.
//     `size_and_flags` empacota o footprint TOTAL do bloco (múltiplo de ALLOC_ALIGN, então os
//     bits [3:0] seriam sempre zero de outra forma) com FREE_BIT no bit 0.
typedef struct {
    size_t         size_and_flags;
    arena_desc_t*  arena;
} block_header_t;

#define ALLOC_HEADER_SIZE ((size_t)sizeof(block_header_t))
#define FREE_BIT           ((size_t)1)
#define SIZE_MASK           (~(size_t)(ALLOC_ALIGN - 1))

// EN: Per-block footer -- boundary tag mirroring the header's size_and_flags, see file header.
// PT: Footer por bloco -- boundary tag espelhando o size_and_flags do header, ver cabeçalho do
//     arquivo.
typedef struct {
    size_t size_and_flags;
} block_footer_t;

#define ALLOC_FOOTER_SIZE ((size_t)sizeof(block_footer_t))

// EN: Explicit free-list doubly-linked node, overlaid onto a FREE block's payload (see file
//     header -- this is why MIN_BLOCK_TOTAL reserves 16 bytes of payload).
// PT: Nó duplamente-ligado da free-list explícita, sobreposto ao payload de um bloco LIVRE (ver
//     cabeçalho do arquivo -- é por isso que MIN_BLOCK_TOTAL reserva 16 bytes de payload).
typedef struct {
    block_header_t* prev;
    block_header_t* next;
} free_link_t;

// EN: Smallest total footprint any block is ever given -- see file header for the 16+16+8
//     derivation. `ARENA_SIZE` (1 MiB) is the same bootstrap-guess default the old bump
//     allocator used (see its prior file header, still a reasonable amortization/waste
//     trade-off): now the TOTAL `mmap` size (arena descriptor + usable region), not just usable
//     bytes -- the 32-byte descriptor overhead is noise against 1 MiB.
// PT: Menor footprint total que qualquer bloco jamais recebe -- ver cabeçalho do arquivo pra
//     derivação 16+16+8. `ARENA_SIZE` (1 MiB) é o mesmo chute-padrão de bootstrap que o bump
//     allocator antigo usava (ver o cabeçalho de arquivo anterior dele, ainda um trade-off
//     razoável de amortização/desperdício): agora é o tamanho TOTAL do `mmap` (descritor de
//     arena + região utilizável), não só bytes utilizáveis -- os 32 bytes de overhead do
//     descritor são ruído contra 1 MiB.
#define MIN_BLOCK_TOTAL ((size_t)48)
#define ARENA_SIZE      ((size_t)(1 * 1024 * 1024))

// EN: Double-free detection's exit code (cheap defense-in-depth, see file header +
//     include/alloc.h). Distinctive, not reused elsewhere in this codebase's test manifests.
// PT: Código de saída da detecção de double-free (defesa em profundidade barata, ver cabeçalho
//     do arquivo + include/alloc.h). Distintivo, não reusado em outro lugar dos manifestos de
//     teste deste código-base.
#define ALLOC_DOUBLE_FREE_EXIT_CODE 111

_Static_assert(sizeof(block_header_t) == ALLOC_ALIGN,
               "block_header_t must be exactly ALLOC_ALIGN bytes so the user pointer right "
               "after it stays 16-byte aligned by construction");
_Static_assert(sizeof(arena_desc_t) % ALLOC_ALIGN == 0,
               "arena_desc_t must be a multiple of ALLOC_ALIGN so the first block header in a "
               "fresh arena starts 16-byte aligned");
_Static_assert(MIN_BLOCK_TOTAL >= ALLOC_HEADER_SIZE + sizeof(free_link_t) + ALLOC_FOOTER_SIZE,
               "MIN_BLOCK_TOTAL must leave room for header + free_link_t + footer");
_Static_assert(MIN_BLOCK_TOTAL % ALLOC_ALIGN == 0,
               "MIN_BLOCK_TOTAL must itself be 16-aligned (it is a block total footprint)");

// EN: Global arena registry (singly-linked -- only ever walked linearly to unlink one entry on
//     the rare full-arena-empty path, see `arena_unlink`) and the single unsorted free-list
//     head (doubly-linked, see `free_link_t`). Both NULL/empty before the first `malloc` call.
// PT: Registro global de arenas (simplesmente-ligado -- só percorrido linearmente pra desligar
//     uma entrada no caminho raro de arena-totalmente-vazia, ver `arena_unlink`) e a cabeça da
//     única free-list não-ordenada (duplamente-ligada, ver `free_link_t`). Os dois NULL/vazios
//     antes da primeira chamada de `malloc`.
static arena_desc_t*   g_arenas    = NULL;
static block_header_t* g_free_list = NULL;

// EN: Rounds `n` up to the next multiple of `align` (`align` must be a power of two -- true for
//     every call site in this file, always `ALLOC_ALIGN`==16).
// PT: Arredonda `n` pra cima pro próximo múltiplo de `align` (`align` precisa ser potência de
//     dois -- verdade em todo ponto de chamada deste arquivo, sempre `ALLOC_ALIGN`==16).
static size_t align_up(size_t n, size_t align) {
    return (n + (align - 1)) & ~(align - 1);
}

// EN: Raw mmap/munmap-failure check per ADR-0002/sys_mmap.h: the kernel returns either a real
//     value or a negative `-errno` packed into the same word, in the range `[-4095, -1]`. Read
//     as unsigned, any real `mmap` address on a sane x86-64 process is nowhere near
//     `(unsigned long)-4095`; a `munmap` success value (0) trivially is not either. Shared by
//     both raw wrappers -- no libc `MAP_FAILED`/`-1` sentinel applies to either.
// PT: Checagem de falha crua do mmap/munmap conforme ADR-0002/sys_mmap.h: o kernel retorna ou
//     um valor de verdade ou um `-errno` negativo empacotado na mesma palavra, na faixa
//     `[-4095, -1]`. Lido como unsigned, nenhum endereço `mmap` de verdade num processo x86-64
//     são chega perto de `(unsigned long)-4095`; um valor de sucesso de `munmap` (0)
//     trivialmente também não. Compartilhado pelos dois wrappers crus -- nenhuma sentinela
//     `MAP_FAILED`/`-1` de libc se aplica a nenhum dos dois.
static int syscall_failed(long ret) {
    return (unsigned long)ret >= (unsigned long)-4095;
}

// ---- block accessors -------------------------------------------------------
// EN: Small, named helpers over the raw header/footer bit-packing -- kept short and inlined by
//     the compiler rather than re-deriving the mask/shift logic ad hoc at every call site.
// PT: Helpers pequenos e nomeados por cima do bit-packing cru do header/footer -- mantidos
//     curtos e inlinados pelo compilador em vez de re-derivar a lógica de máscara/shift ad hoc
//     em cada ponto de chamada.

static size_t block_size(const block_header_t* h) {
    return h->size_and_flags & SIZE_MASK;
}

static int block_is_free(const block_header_t* h) {
    return (h->size_and_flags & FREE_BIT) != 0;
}

static block_footer_t* block_footer(block_header_t* h) {
    return (block_footer_t*)((unsigned char*)h + block_size(h) - ALLOC_FOOTER_SIZE);
}

// EN: Writes `size_and_flags` into both the header and its trailing footer -- the only place
//     that touches a block's size/flag bits, so header and footer can never legally disagree.
// PT: Escreve `size_and_flags` tanto no header quanto no footer dele -- o único lugar que toca
//     os bits de tamanho/flag de um bloco, então header e footer nunca podem legitimamente
//     discordar.
static void block_set(block_header_t* h, size_t size, int is_free, arena_desc_t* arena) {
    size_t sf = (size & SIZE_MASK) | (is_free ? FREE_BIT : (size_t)0);
    h->size_and_flags = sf;
    h->arena          = arena;
    block_footer(h)->size_and_flags = sf;
}

static free_link_t* block_free_link(block_header_t* h) {
    return (free_link_t*)((unsigned char*)h + ALLOC_HEADER_SIZE);
}

// ---- free-list: unsorted, doubly-linked, LIFO insertion --------------------

static void free_list_insert(block_header_t* h) {
    free_link_t* link = block_free_link(h);
    link->prev = NULL;
    link->next = g_free_list;
    if (g_free_list != NULL) {
        block_free_link(g_free_list)->prev = h;
    }
    g_free_list = h;
}

static void free_list_remove(block_header_t* h) {
    free_link_t* link = block_free_link(h);
    if (link->prev != NULL) {
        block_free_link(link->prev)->next = link->next;
    } else {
        g_free_list = link->next;
    }
    if (link->next != NULL) {
        block_free_link(link->next)->prev = link->prev;
    }
}

// EN: First-fit search (see file header trade-off note): the first free block whose total
//     footprint is at least `wanted_total`, or NULL if none qualifies.
// PT: Busca first-fit (ver nota de trade-off no cabeçalho do arquivo): o primeiro bloco livre
//     cujo footprint total é pelo menos `wanted_total`, ou NULL se nenhum se qualifica.
static block_header_t* free_list_find_fit(size_t wanted_total) {
    for (block_header_t* h = g_free_list; h != NULL; h = block_free_link(h)->next) {
        if (block_size(h) >= wanted_total) {
            return h;
        }
    }
    return NULL;
}

// ---- arena registry ---------------------------------------------------------

static void arena_unlink(arena_desc_t* target) {
    arena_desc_t** cur = &g_arenas;
    while (*cur != NULL) {
        if (*cur == target) {
            *cur = target->next;
            return;
        }
        cur = &(*cur)->next;
    }
}

// EN: Picks the total `mmap` size (descriptor + usable region) for a NEW arena that must be
//     able to satisfy a request whose total footprint is `wanted_total`: the bootstrap default
//     `ARENA_SIZE`, or exactly enough to fit `wanted_total` plus the descriptor when the
//     request alone is bigger than the default (mirrors the old bump allocator's
//     `arena_size_for` -- a single giant allocation still gets its own dedicated arena, never
//     inflating every future small one).
// PT: Escolhe o tamanho total de `mmap` (descritor + região utilizável) pra uma arena NOVA que
//     precisa satisfazer um pedido cujo footprint total é `wanted_total`: o padrão de bootstrap
//     `ARENA_SIZE`, ou exatamente o suficiente pra caber `wanted_total` mais o descritor quando
//     o pedido sozinho é maior que o padrão (espelha o `arena_size_for` do bump allocator antigo
//     -- uma alocação gigante sozinha ainda ganha sua própria arena dedicada, nunca inflando
//     toda pequena futura).
static size_t arena_mmap_size_for(size_t wanted_total) {
    size_t needed = ARENA_DESC_SIZE + wanted_total;
    return (needed > ARENA_SIZE) ? needed : ARENA_SIZE;
}

// EN: mmaps a new arena sized to satisfy at least `wanted_total`, formats its ENTIRE usable
//     region as one big free block (see file header), and links both the arena and that block
//     in. Returns 1 on success, 0 on `mmap` failure (arena registry/free-list left untouched).
// PT: Faz mmap de uma arena nova dimensionada pra satisfazer pelo menos `wanted_total`, formata
//     TODA a região utilizável dela como um único bloco livre grande (ver cabeçalho do
//     arquivo), e encadeia tanto a arena quanto aquele bloco. Retorna 1 em sucesso, 0 em falha
//     de `mmap` (registro de arena/free-list deixados intocados).
static int arena_new(size_t wanted_total) {
    size_t mmap_size = arena_mmap_size_for(wanted_total);
    void*  mem = sys_mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                           -1, 0);
    if (syscall_failed((long)(uintptr_t)mem)) {
        return 0;
    }

    arena_desc_t* ad = (arena_desc_t*)mem;
    ad->next      = g_arenas;
    ad->base      = (unsigned char*)mem + ARENA_DESC_SIZE;
    ad->end       = (unsigned char*)mem + mmap_size;
    ad->_reserved = 0;
    g_arenas      = ad;

    block_header_t* whole  = (block_header_t*)ad->base;
    size_t          usable = (size_t)(ad->end - ad->base);
    block_set(whole, usable, /*is_free=*/1, ad);
    free_list_insert(whole);
    return 1;
}

// ---- public API ---------------------------------------------------------

void* malloc(size_t size) {
    // EN: malloc(0) returns NULL -- see include/alloc.h for the documented rationale.
    // PT: malloc(0) retorna NULL -- ver include/alloc.h pro racional documentado.
    if (size == 0) {
        return NULL;
    }

    // EN: Overflow guard (see file header) -- SOV-ALLOC adversarial-review fix (CWE-190 ->
    //     CWE-476): this is the ONE place `size` is validated, so it must cover EVERY downstream
    //     addition, not just the first (`HEADER + size + FOOTER`, rounded up to ALLOC_ALIGN, into
    //     `wanted_total`). `arena_mmap_size_for` later adds `ARENA_DESC_SIZE` on top of
    //     `wanted_total` when no free block already fits -- a `size` that passed a guard missing
    //     that term could let `ARENA_DESC_SIZE + wanted_total` wrap `size_t` into a tiny `needed`,
    //     which then falls back to the default `ARENA_SIZE`; the freshly mmap'd arena would then
    //     be too small to satisfy `wanted_total`, the guaranteed-to-succeed second
    //     `free_list_find_fit` would return NULL, and `free_list_remove(NULL)` would dereference
    //     a null free-link -- SIGSEGV instead of the documented NULL return
    //     (repro: `malloc((size_t)-1 - 39)`). Reject up front if `HEADER + size + FOOTER +
    //     ARENA_DESC_SIZE`, once rounded up to ALLOC_ALIGN, would wrap `size_t`.
    // PT: Guarda de overflow (ver cabeçalho do arquivo) -- fix do review adversarial do SOV-ALLOC
    //     (CWE-190 -> CWE-476): este é o ÚNICO lugar onde `size` é validado, então precisa cobrir
    //     TODA soma posterior, não só a primeira (`HEADER + size + FOOTER`, arredondado pra
    //     ALLOC_ALIGN, virando `wanted_total`). O `arena_mmap_size_for` depois soma
    //     `ARENA_DESC_SIZE` em cima de `wanted_total` quando nenhum bloco livre já cabe -- um
    //     `size` que passasse por uma guarda sem esse termo poderia deixar `ARENA_DESC_SIZE +
    //     wanted_total` estourar `size_t` pra um `needed` minúsculo, que então cai no padrão
    //     `ARENA_SIZE`; a arena recém mmap'ada ficaria pequena demais pra satisfazer
    //     `wanted_total`, a segunda `free_list_find_fit` (dita garantida) retornaria NULL, e
    //     `free_list_remove(NULL)` desreferenciaria um free-link nulo -- SIGSEGV em vez do NULL
    //     documentado (repro: `malloc((size_t)-1 - 39)`). Rejeita de saída se
    //     `HEADER + size + FOOTER + ARENA_DESC_SIZE`, uma vez arredondado pra ALLOC_ALIGN,
    //     estouraria `size_t`.
    if (size > (size_t)-1 - ALLOC_HEADER_SIZE - ALLOC_FOOTER_SIZE - ARENA_DESC_SIZE -
                   (ALLOC_ALIGN - 1)) {
        return NULL;
    }

    size_t wanted_total = align_up(ALLOC_HEADER_SIZE + size + ALLOC_FOOTER_SIZE, ALLOC_ALIGN);
    if (wanted_total < MIN_BLOCK_TOTAL) {
        wanted_total = MIN_BLOCK_TOTAL;
    }

    block_header_t* blk = free_list_find_fit(wanted_total);
    if (blk == NULL) {
        if (!arena_new(wanted_total)) {
            return NULL;
        }
        // EN: The arena just mmap'd was formatted as one free block covering its entire
        //     usable region, sized to be AT LEAST `wanted_total` (see arena_mmap_size_for) --
        //     this second search SHOULD always succeed. SOV-ALLOC adversarial-review fix
        //     (CWE-476, defense-in-depth): never trust that invariant blindly -- an untested
        //     future edit to `arena_mmap_size_for`/`arena_new` that reintroduces a sizing bug
        //     must fail secure (return NULL) here, not crash on `free_list_remove(NULL)`.
        // PT: A arena que acabou de ser mmap'ada foi formatada como um bloco livre cobrindo
        //     toda sua região utilizável, dimensionada pra ser AO MENOS `wanted_total` (ver
        //     arena_mmap_size_for) -- esta segunda busca DEVERIA sempre ter sucesso. Fix do
        //     review adversarial do SOV-ALLOC (CWE-476, defesa em profundidade): nunca confiar
        //     cegamente nesse invariante -- uma futura edição não testada em
        //     `arena_mmap_size_for`/`arena_new` que reintroduza um bug de dimensionamento
        //     precisa falhar de forma segura (retornar NULL) aqui, não crashar em
        //     `free_list_remove(NULL)`.
        blk = free_list_find_fit(wanted_total);
        if (blk == NULL) {
            return NULL;
        }
    }

    free_list_remove(blk);
    size_t        blk_total = block_size(blk);
    size_t        remainder = blk_total - wanted_total;
    arena_desc_t* arena     = blk->arena;

    if (remainder >= MIN_BLOCK_TOTAL) {
        // EN: Split: shrink `blk` to exactly what was asked for, carve the leftover into a new
        //     free block right after it, and give that leftover back to the free-list -- this
        //     is what keeps a small request from wasting an entire large freed/virgin block.
        // PT: Split: encolhe `blk` pra exatamente o que foi pedido, esculpe o restante num
        //     bloco livre novo logo depois, e devolve esse restante pra free-list -- é isso que
        //     impede um pedido pequeno de desperdiçar um bloco grande inteiro, livre ou virgem.
        block_set(blk, wanted_total, /*is_free=*/0, arena);
        block_header_t* rem = (block_header_t*)((unsigned char*)blk + wanted_total);
        block_set(rem, remainder, /*is_free=*/1, arena);
        free_list_insert(rem);
    } else {
        // EN: Remainder too small to ever be a valid free block on its own (see
        //     MIN_BLOCK_TOTAL) -- hand the whole matched block out as-is; the few extra bytes
        //     become bounded internal fragmentation, not a corrupt free-list entry.
        // PT: Restante pequeno demais pra jamais ser um bloco livre válido sozinho (ver
        //     MIN_BLOCK_TOTAL) -- entrega o bloco inteiro encontrado como está; os poucos bytes
        //     extras viram fragmentação interna limitada, não uma entrada de free-list
        //     corrompida.
        block_set(blk, blk_total, /*is_free=*/0, arena);
    }

    return (void*)((unsigned char*)blk + ALLOC_HEADER_SIZE);
}

void free(void* ptr) {
    // EN: Safe no-op with ptr == NULL.
    // PT: No-op seguro com ptr == NULL.
    if (ptr == NULL) {
        return;
    }

    block_header_t* hdr = (block_header_t*)((unsigned char*)ptr - ALLOC_HEADER_SIZE);

    // EN: Double-free cheap defense-in-depth (see file header + include/alloc.h): the flag bit
    //     this reads is already touched by every free() below, so this check is effectively
    //     free. Detected case only -- see include/alloc.h for what this does NOT catch.
    // PT: Defesa em profundidade barata de double-free (ver cabeçalho do arquivo +
    //     include/alloc.h): o bit de flag que isto lê já é tocado por todo free() abaixo, então
    //     esta checagem é efetivamente de graça. Só o caso detectado -- ver include/alloc.h pro
    //     que isto NÃO pega.
    if (block_is_free(hdr)) {
        sys_exit(ALLOC_DOUBLE_FREE_EXIT_CODE);
    }

    arena_desc_t* arena = hdr->arena;
    size_t        size  = block_size(hdr);

    block_set(hdr, size, /*is_free=*/1, arena);

    // EN: Coalesce forward: the block that starts right where this one ends, if it exists
    //     within THIS arena's bounds (never read/interpret memory past `arena->end` -- it may
    //     belong to a different arena or be unmapped entirely) and is itself free.
    // PT: Coalesce pra frente: o bloco que começa bem onde este termina, se existir dentro dos
    //     limites DESTA arena (nunca ler/interpretar memória além de `arena->end` -- pode
    //     pertencer a uma arena diferente ou estar totalmente desmapeada) e ele mesmo estiver
    //     livre.
    unsigned char* next_addr = (unsigned char*)hdr + size;
    if (next_addr < arena->end) {
        block_header_t* next = (block_header_t*)next_addr;
        if (block_is_free(next)) {
            free_list_remove(next);
            size += block_size(next);
            block_set(hdr, size, /*is_free=*/1, arena);
        }
    }

    // EN: Coalesce backward: read the footer immediately before this block's header (must
    //     exist within `arena->base` -- never read before the arena's own usable start) to find
    //     out whether the PRECEDING physical block is free, and if so, exactly where it starts.
    // PT: Coalesce pra trás: lê o footer imediatamente antes do header deste bloco (precisa
    //     existir dentro de `arena->base` -- nunca ler antes do início utilizável da própria
    //     arena) pra descobrir se o bloco físico PRECEDENTE está livre, e se sim, exatamente
    //     onde ele começa.
    if ((unsigned char*)hdr > arena->base) {
        // EN: TST-STATIC (TODO.md, W11): `const` -- this pointer is only ever READ (the write
        //     path for a block's size/flags is exclusively `block_set`, which always operates
        //     on a `block_header_t*`, never a bare footer). cppcheck's constVariablePointer
        //     correctly flagged the non-const declaration here.
        // PT: TST-STATIC (TODO.md, W11): `const` -- este ponteiro só é LIDO (o caminho de
        //     escrita pro tamanho/flags de um bloco é exclusivamente o `block_set`, que sempre
        //     opera sobre um `block_header_t*`, nunca um footer solto). O constVariablePointer
        //     do cppcheck apontou corretamente a declaração não-const aqui.
        const block_footer_t* prev_footer =
            (const block_footer_t*)((unsigned char*)hdr - ALLOC_FOOTER_SIZE);
        if ((prev_footer->size_and_flags & FREE_BIT) != 0) {
            size_t          prev_size = prev_footer->size_and_flags & SIZE_MASK;
            block_header_t* prev      = (block_header_t*)((unsigned char*)hdr - prev_size);
            free_list_remove(prev);
            size = prev_size + size;
            hdr  = prev;
            block_set(hdr, size, /*is_free=*/1, arena);
        }
    }

    // EN: Stretch (OPTIONAL, see file header): if the fully-coalesced block spans this arena's
    //     ENTIRE usable region, the arena holds zero live allocations -- return it to the
    //     kernel instead of keeping it mapped forever. `arena_unlink` first so no other code
    //     path can ever reach this arena again; `sys_munmap` failure is tolerated (a leak, never
    //     a crash -- see include/sys_munmap.h).
    // PT: Stretch (OPCIONAL, ver cabeçalho do arquivo): se o bloco totalmente coalescido cobre
    //     a região utilizável INTEIRA desta arena, a arena não tem nenhuma alocação viva --
    //     devolve ela ao kernel em vez de deixá-la mapeada pra sempre. `arena_unlink` primeiro
    //     pra que nenhum outro caminho de código jamais alcance esta arena de novo; falha de
    //     `sys_munmap` é tolerada (um leak, nunca um crash -- ver include/sys_munmap.h).
    if ((unsigned char*)hdr == arena->base && size == (size_t)(arena->end - arena->base)) {
        arena_unlink(arena);
        // EN: Return value intentionally discarded -- see include/sys_munmap.h: on failure the
        //     allocator simply leaves this arena mapped (tolerated leak, never a crash), since
        //     it has already been unlinked and will never be handed out again either way.
        // PT: Valor de retorno descartado de propósito -- ver include/sys_munmap.h: em falha o
        //     alocador simplesmente deixa esta arena mapeada (leak tolerado, nunca um crash),
        //     já que ela já foi desligada e nunca mais será entregue de qualquer forma.
        (void)sys_munmap((void*)arena, (size_t)(arena->end - (unsigned char*)arena));
        return;
    }

    free_list_insert(hdr);
}

void* realloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // EN: Recover the old block's usable payload capacity from its header (immediately before
    //     `ptr` -- see file header). Must read this BEFORE calling malloc(size): if the new
    //     allocation fails, we return NULL having touched neither `ptr` nor its header, per
    //     realloc's contract. This is the block's full CAPACITY (>= the size originally
    //     requested -- see include/alloc.h), not a separately-tracked "exact requested size":
    //     this allocator, like most real ones, does not remember that number once a block is
    //     carved -- only its total footprint.
    // PT: Recupera a capacidade de payload utilizável do bloco antigo a partir do header dele
    //     (imediatamente antes de `ptr` -- ver cabeçalho do arquivo). Precisa ler isso ANTES de
    //     chamar malloc(size): se a alocação nova falhar, retornamos NULL sem ter tocado nem
    //     `ptr` nem seu header, conforme o contrato do realloc. Esta é a CAPACIDADE completa de
    //     payload do bloco (`>=` o tamanho originalmente pedido -- ver include/alloc.h), não um
    //     "tamanho exato pedido" rastreado separadamente: este alocador, como a maioria dos de
    //     verdade, não lembra desse número depois que um bloco é esculpido -- só o footprint
    //     total dele.
    const block_header_t* old_hdr =
        (const block_header_t*)((const unsigned char*)ptr - ALLOC_HEADER_SIZE);
    size_t old_total    = block_size(old_hdr);
    size_t old_capacity = old_total - ALLOC_HEADER_SIZE - ALLOC_FOOTER_SIZE;

    void* new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    size_t copy_size = (old_capacity < size) ? old_capacity : size;
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}
