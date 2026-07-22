// SPDX-License-Identifier: MPL-2.0
// EN: SOV-SFNT (FT-F1) -- implementation. See include/core/sfnt.h's file header for the full
//     clean-room rationale, scope, and hardening posture; this file's own comments focus on the
//     LOCAL "how", not re-derived "why" that already lives there.
//
//     BUILD-TWICE NOTE (mirrors src/core_api.c's own file header): this file is compiled TWICE by
//     the Makefile like every other src/*.c file -- once normally into `build/obj/` (linked into
//     `tests/test_sfnt.c` and every other freestanding gate program), once again into
//     `build/objcore/` under `$(CORE_RENAME_FLAGS)` for `build/libcore.a`. Unlike src/alloc.c's
//     `malloc`/`free`/`memcpy`/etc., NONE of this file's identifiers collide with a libc name --
//     every externally-visible symbol already carries the `glx_sfnt_` prefix (namespaced by
//     construction, not by a build-time rename), and every OTHER identifier below is `static`
//     (internal linkage, invisible to `nm -g`, does not even need a whitelist entry -- see
//     tools/check_libcore_symbols.sh, which only needed a `glx_sfnt_*` pattern added, not a new
//     `$(CORE_RENAME_FLAGS)` entry). This file also does not `#include` `alloc.h`/`mem.h` --
//     see sfnt.h's "NO DYNAMIC ALLOCATION" note -- so it has zero exposure to the rename
//     machinery either way.
// PT: SOV-SFNT (FT-F1) -- implementação. Ver o cabeçalho de arquivo do include/core/sfnt.h pro
//     racional clean-room completo, escopo, e postura de hardening; os comentários deste arquivo
//     focam no "como" LOCAL, não no "porquê" já re-derivado que mora lá.
//
//     NOTA DE COMPILAÇÃO-DUPLA (espelha o próprio cabeçalho de arquivo do src/core_api.c): este
//     arquivo é compilado DUAS VEZES pelo Makefile como todo outro src/*.c -- uma vez normalmente
//     pra `build/obj/` (linkado em `tests/test_sfnt.c` e todo outro programa-gate freestanding),
//     outra vez pra `build/objcore/` sob `$(CORE_RENAME_FLAGS)` pra `build/libcore.a`. Diferente
//     do `malloc`/`free`/`memcpy`/etc. do src/alloc.c, NENHUM identificador deste arquivo colide
//     com nome de libc -- todo símbolo externamente visível já carrega o prefixo `glx_sfnt_`
//     (namespaced por construção, não por rename em tempo de build), e todo OUTRO identificador
//     abaixo é `static` (linkage interno, invisível pro `nm -g`, nem precisa de entrada de
//     whitelist -- ver tools/check_libcore_symbols.sh, que só precisou de um padrão `glx_sfnt_*`
//     acrescentado, não uma entrada nova em `$(CORE_RENAME_FLAGS)`). Este arquivo também não
//     `#include`ia `alloc.h`/`mem.h` -- ver a nota "SEM ALOCAÇÃO DINÂMICA" do sfnt.h -- então tem
//     exposição zero à maquinaria de rename de qualquer jeito.
// Copyright (c) 2026 Petrus Silva Costa
#include "core/sfnt.h"

// ============================================================================================
// EN: Bounds-checked primitive readers. EVERY multi-byte field this file ever reads goes
//     through one of these -- see include/core/sfnt.h's "HARDENING POSTURE" for the
//     overflow-safe shape (`offset > len || size > len - offset`, never `offset + size`).
// PT: Leitores primitivos com checagem de limite. TODO campo multi-byte que este arquivo algum
//     dia lê passa por um destes -- ver "HARDENING POSTURE" do include/core/sfnt.h pro formato
//     seguro-contra-overflow (`offset > len || size > len - offset`, nunca `offset + size`).
// ============================================================================================

// EN: The one true bounds check: is `[off, off+need)` entirely within `[0, len)`? Written to
//     never compute `off + need` (a hostile/huge `off` near `SIZE_MAX` could overflow that sum
//     and wrap back into range) -- `off > len` is checked FIRST (cheap, no overflow possible: both
//     operands are already valid `size_t`s), which makes `len - off` safe to compute afterwards.
// PT: A checagem de limite verdadeira única: `[off, off+need)` está inteiramente dentro de
//     `[0, len)`? Escrita pra nunca computar `off + need` (um `off` hostil/enorme perto de
//     `SIZE_MAX` poderia estourar essa soma e dar a volta de novo pra dentro da faixa) --
//     `off > len` é checado PRIMEIRO (barato, sem overflow possível: os dois operandos já são
//     `size_t` válidos), o que torna `len - off` seguro de computar depois.
static int bounds_ok(size_t len, size_t off, size_t need) {
    if (off > len) return 0;
    if (need > len - off) return 0;
    return 1;
}

static int rd_u8(const uint8_t* b, size_t len, size_t off, uint8_t* out) {
    if (!bounds_ok(len, off, 1)) return 0;
    *out = b[off];
    return 1;
}

static int rd_u16(const uint8_t* b, size_t len, size_t off, uint16_t* out) {
    if (!bounds_ok(len, off, 2)) return 0;
    *out = (uint16_t)(((uint16_t)b[off] << 8) | (uint16_t)b[off + 1]);
    return 1;
}

static int rd_i16(const uint8_t* b, size_t len, size_t off, int16_t* out) {
    uint16_t u;
    if (!rd_u16(b, len, off, &u)) return 0;
    *out = (int16_t)u;
    return 1;
}

static int rd_u32(const uint8_t* b, size_t len, size_t off, uint32_t* out) {
    if (!bounds_ok(len, off, 4)) return 0;
    *out = ((uint32_t)b[off] << 24) | ((uint32_t)b[off + 1] << 16) |
           ((uint32_t)b[off + 2] << 8) | (uint32_t)b[off + 3];
    return 1;
}

// EN: Signed 8-bit read (composite glyph component arguments, when `ARG_1_AND_2_ARE_WORDS` is
//     unset). `int8_t` is one of the fixed-width types `<stdint.h>` guarantees.
// PT: Leitura de 8 bits com sinal (argumentos de componente de glyph composto, quando
//     `ARG_1_AND_2_ARE_WORDS` está desmarcado). `int8_t` é um dos tipos de largura fixa que o
//     `<stdint.h>` garante.
static int rd_i8(const uint8_t* b, size_t len, size_t off, int8_t* out) {
    uint8_t u;
    if (!rd_u8(b, len, off, &u)) return 0;
    *out = (int8_t)u;
    return 1;
}

// ============================================================================================
// EN: glx_sfnt_open -- table directory + head/maxp/hhea/hmtx/loca/cmap structural validation.
// PT: glx_sfnt_open -- diretório de tabela + validação estrutural de
//     head/maxp/hhea/hmtx/loca/cmap.
// ============================================================================================

// EN: One table directory record's offset/length within `blob` -- local scratch, not exposed
//     (the public `glx_sfnt_face` only keeps what `hmtx`/`loca`/`glyf`-reading functions need
//     later; `head`/`maxp`/`hhea`/`cmap` are fully consumed inside `glx_sfnt_open` itself).
// PT: Offset/tamanho de um registro de diretório de tabela dentro de `blob` -- rascunho local,
//     não exposto (o `glx_sfnt_face` público só guarda o que as funções de leitura de
//     `hmtx`/`loca`/`glyf` vão precisar depois; `head`/`maxp`/`hhea`/`cmap` são totalmente
//     consumidos dentro do próprio `glx_sfnt_open`).
typedef struct {
    size_t off;
    size_t len;
    int found;
} table_slot_t;

// EN: `sfntVersion` values this parser accepts -- both are TrueType-outline (`glyf`-based)
//     variants; `0x00010000` is the common case, `0x74727565` (`'true'`) is an older
//     Mac-originated variant of the same outline format. `'OTTO'` (CFF/OTF, PostScript cubic
//     outlines) is deliberately NOT here -- see include/core/sfnt.h's scope note.
// PT: Valores de `sfntVersion` que este parser aceita -- ambos são variantes de outline
//     TrueType (baseadas em `glyf`); `0x00010000` é o caso comum, `0x74727565` (`'true'`) é uma
//     variante mais antiga, de origem Mac, do mesmo formato de outline. `'OTTO'` (CFF/OTF,
//     outlines cúbicas PostScript) deliberadamente NÃO está aqui -- ver a nota de escopo do
//     include/core/sfnt.h.
#define SFNT_VERSION_1_0 0x00010000u
#define SFNT_VERSION_TRUE 0x74727565u

// EN: Scans the (already bounds-validated) table directory for the 4-byte `tag`, filling `slot`
//     if found AND the found table's own `[offset, length)` fits within the blob. Linear scan
//     (`num_tables` is a `uint16_t`, at most 65535, and real fonts carry a few dozen at most) --
//     correctness-first, same trade-off documented at length in src/alloc.c for its free-list
//     walk.
// PT: Varre o diretório de tabela (já validado de limite) pelo `tag` de 4 bytes, preenchendo
//     `slot` se achado E o próprio `[offset, length)` da tabela achada couber dentro do blob.
//     Varredura linear (`num_tables` é `uint16_t`, no máximo 65535, e fontes reais carregam
//     no máximo umas dezenas) -- correção-primeiro, mesma troca documentada em detalhe no
//     src/alloc.c pra sua varredura de free-list.
static int find_table(const uint8_t* blob, size_t len, size_t dir_off, uint16_t num_tables,
                       const char tag[4], table_slot_t* slot) {
    for (uint16_t i = 0; i < num_tables; i++) {
        size_t rec_off = dir_off + (size_t)i * 16;
        // EN: Review-adversarial COSMETIC fix (SOV-SFNT review): these 4 tag bytes are safe
        //     TODAY (the caller, `glx_sfnt_open`, already proved
        //     `dir_off + num_tables*16 <= len` before this loop ever runs, so `rec_off..+3` is
        //     always in-bounds for every `i < num_tables`) but were direct `blob[...]` reads
        //     with no bounds check of their OWN -- fragile if this function is ever reused/
        //     refactored with a looser caller invariant. Routed through `rd_u8` (the same
        //     bounds-checked primitive every other read in this file uses) so the invariant is
        //     enforced HERE too, not just trusted from the caller.
        // PT: Fix do COSMÉTICO do review adversarial (review do SOV-SFNT): esses 4 bytes de tag
        //     são seguros HOJE (quem chama, `glx_sfnt_open`, já provou
        //     `dir_off + num_tables*16 <= len` antes deste laço rodar, então `rec_off..+3` está
        //     sempre dentro de limite pra todo `i < num_tables`) mas eram leituras `blob[...]`
        //     diretas sem checagem de limite PRÓPRIA -- frágil se esta função algum dia for
        //     reusada/refatorada com um invariante mais frouxo de quem chama. Roteado por
        //     `rd_u8` (a mesma primitiva com checagem de limite que toda outra leitura deste
        //     arquivo usa) pra o invariante ser reforçado AQUI também, não só confiado de quem
        //     chama.
        uint8_t t0, t1, t2, t3;
        if (!rd_u8(blob, len, rec_off, &t0) || !rd_u8(blob, len, rec_off + 1, &t1) ||
            !rd_u8(blob, len, rec_off + 2, &t2) || !rd_u8(blob, len, rec_off + 3, &t3)) {
            return 0;
        }
        if (t0 != (uint8_t)tag[0] || t1 != (uint8_t)tag[1] || t2 != (uint8_t)tag[2] ||
            t3 != (uint8_t)tag[3]) {
            continue;
        }
        uint32_t off32, len32;
        if (!rd_u32(blob, len, rec_off + 8, &off32)) return 0;
        if (!rd_u32(blob, len, rec_off + 12, &len32)) return 0;
        if (!bounds_ok(len, (size_t)off32, (size_t)len32)) return 0;
        slot->off = (size_t)off32;
        slot->len = (size_t)len32;
        slot->found = 1;
        return 1;
    }
    slot->found = 0;
    return 1;
}

glx_sfnt_result glx_sfnt_open(const uint8_t* blob, size_t len, glx_sfnt_face* out) {
    if (blob == NULL || len == 0 || out == NULL) return GLX_SFNT_ERR_INVALID_ARG;

    // EN: Minimum SFNT header: sfntVersion(4) + numTables(2) + searchRange(2) +
    //     entrySelector(2) + rangeShift(2) = 12 bytes, before any directory record.
    // PT: Cabeçalho SFNT mínimo: sfntVersion(4) + numTables(2) + searchRange(2) +
    //     entrySelector(2) + rangeShift(2) = 12 bytes, antes de qualquer registro de diretório.
    uint32_t version;
    if (!rd_u32(blob, len, 0, &version)) return GLX_SFNT_ERR_TRUNCATED;
    if (version != SFNT_VERSION_1_0 && version != SFNT_VERSION_TRUE) {
        return GLX_SFNT_ERR_BAD_VERSION;
    }

    uint16_t num_tables;
    if (!rd_u16(blob, len, 4, &num_tables)) return GLX_SFNT_ERR_TRUNCATED;

    // EN: Directory record array: 16 bytes/record, starting right after the 12-byte header.
    //     Hostile case this line guards against ("tabela truncada" / a lying `numTables`): the
    //     whole array must fit, or we refuse to even start scanning it.
    // PT: Array de registro de diretório: 16 bytes/registro, começando logo após o cabeçalho de
    //     12 bytes. Caso hostil que esta linha guarda ("tabela truncada" / um `numTables`
    //     mentiroso): o array inteiro precisa caber, ou recusamos até começar a varrê-lo.
    size_t dir_off = 12;
    if (!bounds_ok(len, dir_off, (size_t)num_tables * 16)) return GLX_SFNT_ERR_TRUNCATED;

    table_slot_t head_t, maxp_t, hhea_t, hmtx_t, cmap_t, loca_t, glyf_t;
    if (!find_table(blob, len, dir_off, num_tables, "head", &head_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "maxp", &maxp_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "hhea", &hhea_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "hmtx", &hmtx_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "cmap", &cmap_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "loca", &loca_t)) return GLX_SFNT_ERR_TRUNCATED;
    if (!find_table(blob, len, dir_off, num_tables, "glyf", &glyf_t)) return GLX_SFNT_ERR_TRUNCATED;

    if (!head_t.found || !maxp_t.found || !hhea_t.found || !hmtx_t.found || !cmap_t.found ||
        !loca_t.found || !glyf_t.found) {
        return GLX_SFNT_ERR_TABLE_MISSING;
    }

    // ---- kern: OPTIONAL table -- absent, or present-but-malformed, both soft-fail to "no kern" -
    // EN: Unlike the 7 REQUIRED tables above (a missing one is `GLX_SFNT_ERR_TABLE_MISSING`),
    //     `kern` is genuinely optional (a font missing kerning data is still a fully valid font --
    //     see include/core/sfnt.h's `glx_sfnt_face::kern_len` doc) AND, if `find_table` itself
    //     fails (the directory record's own `[offset, length)` does not fit the blob -- a hostile/
    //     malformed `kern` entry), that failure is folded into "no kern" too rather than aborting
    //     `glx_sfnt_open` entirely -- same soft-fail posture as `cmap`'s own per-record OOB
    //     handling a few lines below in this same function (a non-critical table's bad entry
    //     should not brick parsing of every OTHER table this font legitimately has). `kern_t` is
    //     explicitly zero-initialized so `kern_t.found` is never read as indeterminate memory in
    //     the `!find_table(...)` branch (that branch returns 0 without itself touching
    //     `slot->found` -- see `find_table`'s own comment above).
    // PT: Diferente das 7 tabelas OBRIGATÓRIAS acima (uma ausente é `GLX_SFNT_ERR_TABLE_MISSING`),
    //     `kern` é genuinamente opcional (uma fonte sem dado de kerning ainda é uma fonte
    //     plenamente válida -- ver o doc do `glx_sfnt_face::kern_len` do include/core/sfnt.h) E,
    //     se o próprio `find_table` falhar (o `[offset, length)` do próprio registro de diretório
    //     não cabe no blob -- uma entrada `kern` hostil/malformada), essa falha também é dobrada
    //     em "sem kern" em vez de abortar o `glx_sfnt_open` inteiro -- mesma postura de falha
    //     suave que o próprio tratamento de OOB por-registro do `cmap` algumas linhas abaixo nesta
    //     mesma função já usa (uma entrada ruim de uma tabela não-crítica não deveria travar o
    //     parse de toda OUTRA tabela que esta fonte legitimamente tem). `kern_t` é zero-
    //     inicializado explicitamente pra `kern_t.found` nunca ser lido como memória
    //     indeterminada no ramo `!find_table(...)` (aquele ramo retorna 0 sem tocar
    //     `slot->found` -- ver o comentário do próprio `find_table` acima).
    // EN: Field-by-field assignment, NOT aggregate `= {0, 0, 0}` -- the clang backend lowers a
    //     3-field aggregate zero-init to a synthesized (non-textual) `memset()` call in some
    //     build configurations, which the preprocessor-macro `-Dmemset=glx_core_memset_impl`
    //     rename ($(CORE_RENAME_FLAGS), Makefile) cannot reach (it only rewrites source-level
    //     identifier tokens, not backend-generated calls). That left `build/objcore/sfnt.o`
    //     importing a BARE `U memset` -- same symbol-hijack class SOV-LIBCORE closed as CRITICAL,
    //     via a new vector (aggregate zero-init). See `tools/check_libcore_symbols.sh`'s `nm -u`
    //     pass for the gate that now catches this class.
    // PT: Atribuição campo-a-campo, NÃO agregado `= {0, 0, 0}` -- o backend do clang às vezes
    //     rebaixa um zero-init de agregado de 3 campos para uma chamada `memset()` SINTETIZADA
    //     (não-textual) em algumas configurações de build, que o rename por macro do
    //     pré-processador `-Dmemset=glx_core_memset_impl` ($(CORE_RENAME_FLAGS), Makefile) não
    //     alcança (ele só reescreve tokens de identificador em nível de fonte, não chamadas
    //     geradas pelo backend). Isso deixava o `build/objcore/sfnt.o` importando um `U memset`
    //     CRU -- mesma classe de symbol-hijack que o SOV-LIBCORE fechou como CRÍTICO, por um
    //     vetor novo (zero-init de agregado). Ver o passo `nm -u` do
    //     `tools/check_libcore_symbols.sh` pro gate que agora pega esta classe.
    table_slot_t kern_t;
    kern_t.off = 0;
    kern_t.len = 0;
    kern_t.found = 0;
    int kern_found = find_table(blob, len, dir_off, num_tables, "kern", &kern_t) && kern_t.found;

    // ---- COLR/CPAL: OPTIONAL tables (A4-EMOJI) -- same soft-fail posture as `kern` above -------
    // EN: `COLR`/`CPAL` are exactly as optional as `kern` (a font with no colour glyphs is still
    //     a fully valid font -- see `glx_sfnt_face::colr_len`'s own doc), so the same reasoning
    //     applies verbatim: absent, OR present-but-a-hostile/malformed directory entry (a
    //     `[offset, length)` that does not itself fit `blob`), both fold into `*_len == 0` rather
    //     than aborting `glx_sfnt_open` over a non-critical table. Field-by-field zero-init, NOT
    //     an aggregate `= {0, 0, 0}` -- see `kern_t`'s own comment above for why (the
    //     `build/objcore/sfnt.o` bare-`memset`-import lesson this codebase already learned once).
    //     `glx_sfnt_open` deliberately does NOT peek at either table's OWN internal header here
    //     (version, record counts, ...) -- unlike the 7 REQUIRED tables' size cross-checks above,
    //     `COLR`/`CPAL`'s internal structure is validated lazily, per-call, by
    //     `glx_sfnt_colr_layers`/`glx_sfnt_cpal_rgba` themselves (same division of labour `kern`'s
    //     own version/subtable-format checks already live entirely inside `glx_sfnt_kern`, not
    //     here).
    // PT: `COLR`/`CPAL` são exatamente tão opcionais quanto `kern` (uma fonte sem glyph colorido
    //     nenhum ainda é uma fonte plenamente válida -- ver o doc próprio do
    //     `glx_sfnt_face::colr_len`), então o mesmo raciocínio se aplica ao pé da letra: ausente,
    //     OU presente-mas-com-entrada-de-diretório-hostil/malformada (um `[offset, length)` que
    //     não cabe no próprio `blob`), ambos dobram em `*_len == 0` em vez de abortar o
    //     `glx_sfnt_open` por causa de uma tabela não-crítica. Zero-init campo-a-campo, NÃO um
    //     agregado `= {0, 0, 0}` -- ver o comentário próprio do `kern_t` acima pro porquê (a
    //     lição do `memset` cru importado no `build/objcore/sfnt.o` que este código-base já
    //     aprendeu uma vez). O `glx_sfnt_open` deliberadamente NÃO espia o próprio header interno
    //     de nenhuma das duas tabelas aqui (versão, contagens de registro, ...) -- diferente das
    //     checagens cruzadas de tamanho das 7 tabelas OBRIGATÓRIAS acima, a estrutura interna do
    //     `COLR`/`CPAL` é validada preguiçosamente, por-chamada, pelo próprio
    //     `glx_sfnt_colr_layers`/`glx_sfnt_cpal_rgba` (mesma divisão de trabalho que as próprias
    //     checagens de versão/formato-de-subtable do `kern` já vivem inteiramente dentro do
    //     `glx_sfnt_kern`, não aqui).
    table_slot_t colr_t;
    colr_t.off = 0;
    colr_t.len = 0;
    colr_t.found = 0;
    int colr_found = find_table(blob, len, dir_off, num_tables, "COLR", &colr_t) && colr_t.found;

    table_slot_t cpal_t;
    cpal_t.off = 0;
    cpal_t.len = 0;
    cpal_t.found = 0;
    int cpal_found = find_table(blob, len, dir_off, num_tables, "CPAL", &cpal_t) && cpal_t.found;

    // ---- head: unitsPerEm @18, indexToLocFormat @50 -----------------------------------------
    uint16_t units_per_em;
    int16_t index_to_loc_format;
    if (!rd_u16(blob, len, head_t.off + 18, &units_per_em)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_i16(blob, len, head_t.off + 50, &index_to_loc_format)) return GLX_SFNT_ERR_TRUNCATED;
    if (index_to_loc_format != 0 && index_to_loc_format != 1) return GLX_SFNT_ERR_BAD_OFFSET;

    // ---- maxp: numGlyphs @4 (same offset in both v0.5 and v1.0) -----------------------------
    uint16_t num_glyphs;
    if (!rd_u16(blob, len, maxp_t.off + 4, &num_glyphs)) return GLX_SFNT_ERR_TRUNCATED;

    // ---- hhea: ascender @4, descender @6, lineGap @8, numberOfHMetrics @34 -------------------
    int16_t ascender, descender, line_gap;
    uint16_t num_h_metrics;
    if (!rd_i16(blob, len, hhea_t.off + 4, &ascender)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_i16(blob, len, hhea_t.off + 6, &descender)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_i16(blob, len, hhea_t.off + 8, &line_gap)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_u16(blob, len, hhea_t.off + 34, &num_h_metrics)) return GLX_SFNT_ERR_TRUNCATED;

    // EN: `numberOfHMetrics` must be in `[1, num_glyphs]` -- 0 would leave `glx_sfnt_hmetrics`
    //     no "last advance width" entry to fall back on for glyphs beyond it (see this file's
    //     hmtx section below), and `> num_glyphs` is simply nonsensical (more explicit metrics
    //     than glyphs that exist).
    // PT: `numberOfHMetrics` precisa estar em `[1, num_glyphs]` -- 0 deixaria o
    //     `glx_sfnt_hmetrics` sem entrada de "última largura de avanço" pra cair de volta pros
    //     glyphs além dele (ver a seção hmtx deste arquivo abaixo), e `> num_glyphs` é
    //     simplesmente sem sentido (mais métricas explícitas do que glyphs que existem).
    if (num_h_metrics == 0 || num_h_metrics > num_glyphs) return GLX_SFNT_ERR_BAD_OFFSET;

    // ---- hmtx: numHMetrics*4 bytes + (numGlyphs-numHMetrics)*2 trailing-lsb bytes ------------
    // EN: Checked against `hmtx_t.len` (the TABLE's OWN declared length from the directory,
    //     already proven to fit within the blob by `find_table`'s own `bounds_ok` call) rather
    //     than the whole blob's `len` -- a real font's `hmtx` table sits among several OTHER
    //     tables, so checking only "fits somewhere before the blob ends" would let a lying
    //     `numGlyphs`/`numberOfHMetrics` silently walk into a NEIGHBOURING table's bytes on a
    //     large multi-table file (not a memory-safety bug -- every later read still goes through
    //     the same `[0, len)` bounds check -- but a real structural inconsistency this ticket's
    //     brief specifically asks to catch here, at `glx_sfnt_open` time).
    // PT: Checado contra `hmtx_t.len` (o PRÓPRIO tamanho declarado da tabela pelo diretório, já
    //     provado caber dentro do blob pela própria chamada `bounds_ok` do `find_table`) em vez
    //     do `len` do blob inteiro -- a tabela `hmtx` de uma fonte real fica no meio de VÁRIAS
    //     outras tabelas, então checar só "cabe em algum lugar antes do blob acabar" deixaria um
    //     `numGlyphs`/`numberOfHMetrics` mentiroso caminhar em silêncio pros bytes de uma tabela
    //     VIZINHA num arquivo grande multi-tabela (não é um bug de segurança-de-memória -- toda
    //     leitura posterior ainda passa pela mesma checagem de limite `[0, len)` -- mas é uma
    //     inconsistência estrutural real que o brief desta tarefa pede especificamente pra pegar
    //     aqui, em tempo de `glx_sfnt_open`).
    size_t hmtx_need = (size_t)num_h_metrics * 4 + (size_t)(num_glyphs - num_h_metrics) * 2;
    if (hmtx_need > hmtx_t.len) return GLX_SFNT_ERR_TRUNCATED;

    // ---- loca: (numGlyphs+1) entries, width depends on indexToLocFormat ----------------------
    // EN: Same "check against the table's OWN declared length" reasoning as `hmtx` above. This
    //     is where a lying `numGlyphs` (the hostile case this ticket's brief names explicitly)
    //     gets caught precisely: a `num_glyphs` inflated beyond what the real `loca` table
    //     declares fails here, regardless of how large the rest of the font file is.
    // PT: Mesmo racional de "checa contra o PRÓPRIO tamanho declarado da tabela" do `hmtx`
    //     acima. É aqui que um `numGlyphs` mentiroso (o caso hostil que o brief desta tarefa
    //     nomeia explicitamente) é pego com precisão: um `num_glyphs` inflado além do que a
    //     tabela `loca` real declara falha aqui, independente de quão grande é o resto do
    //     arquivo de fonte.
    size_t loca_entry_size = (index_to_loc_format == 0) ? 2u : 4u;
    size_t loca_need = ((size_t)num_glyphs + 1) * loca_entry_size;
    if (loca_need > loca_t.len) return GLX_SFNT_ERR_TRUNCATED;

    // ---- cmap: pick the best usable subtable (format 12 preferred over format 4) -------------
    uint16_t cmap_num_tables;
    if (!rd_u16(blob, len, cmap_t.off + 2, &cmap_num_tables)) return GLX_SFNT_ERR_TRUNCATED;
    // EN: The encoding record array itself must fit -- a `cmap` claiming more encoding records
    //     than the blob can hold is a truncated/malformed `cmap` table, one of this ticket's
    //     named hostile cases; this is a hard failure of `glx_sfnt_open` (we cannot safely
    //     enumerate what was declared).
    // PT: O próprio array de registro de encoding precisa caber -- um `cmap` alegando mais
    //     registros de encoding do que o blob consegue conter é uma tabela `cmap`
    //     truncada/malformada, um dos casos hostis nomeados desta tarefa; esta é uma falha
    //     forte do `glx_sfnt_open` (não podemos enumerar com segurança o que foi declarado).
    size_t cmap_recs_off = cmap_t.off + 4;
    if (!bounds_ok(len, cmap_recs_off, (size_t)cmap_num_tables * 8)) return GLX_SFNT_ERR_TRUNCATED;

    size_t best_off = 0;
    uint16_t best_format = 0; // 0 = none found yet
    for (uint16_t i = 0; i < cmap_num_tables; i++) {
        size_t rec_off = cmap_recs_off + (size_t)i * 8;
        uint32_t sub_off32;
        if (!rd_u32(blob, len, rec_off + 4, &sub_off32)) continue; // unreachable given the check above, defensive anyway
        // EN: An individual encoding record's subtable offset pointing outside the blob is a
        //     SOFT failure -- skip this one candidate, keep scanning the rest, do not fail the
        //     whole `glx_sfnt_open` over one bad record (see include/core/sfnt.h's cmap note).
        // PT: O offset de subtabela de um registro de encoding individual apontando fora do
        //     blob é uma falha SUAVE -- pula esse candidato, continua varrendo o resto, não
        //     falha o `glx_sfnt_open` inteiro por causa de um registro ruim (ver a nota de cmap
        //     do include/core/sfnt.h).
        size_t sub_abs = cmap_t.off + (size_t)sub_off32;
        if (sub_abs < cmap_t.off) continue; // overflow guard (sub_off32 wrapped cmap_t.off)
        uint16_t fmt;
        if (!rd_u16(blob, len, sub_abs, &fmt)) continue;
        if (fmt == 12 && best_format != 12) {
            best_off = sub_abs;
            best_format = 12;
        } else if (fmt == 4 && best_format == 0) {
            best_off = sub_abs;
            best_format = 4;
        }
    }

    out->blob = blob;
    out->len = len;
    out->units_per_em = units_per_em;
    out->index_to_loc_format = index_to_loc_format;
    out->num_glyphs = num_glyphs;
    out->ascender = ascender;
    out->descender = descender;
    out->line_gap = line_gap;
    out->num_h_metrics = num_h_metrics;
    out->hmtx_off = hmtx_t.off;
    out->loca_off = loca_t.off;
    out->glyf_off = glyf_t.off;
    out->glyf_len = glyf_t.len;
    out->cmap_subtable_off = best_off;
    out->cmap_format = best_format;
    // EN: `kern_len == 0` is the sentinel `glx_sfnt_kern` checks for "no kern table" -- see this
    //     function's own `kern_t`/`kern_found` comment above and include/core/sfnt.h's
    //     `glx_sfnt_face::kern_len` doc.
    // PT: `kern_len == 0` é o sentinela que o `glx_sfnt_kern` checa pra "sem tabela kern" -- ver o
    //     comentário próprio `kern_t`/`kern_found` desta função acima e o doc do
    //     `glx_sfnt_face::kern_len` do include/core/sfnt.h.
    out->kern_off = kern_found ? kern_t.off : 0;
    out->kern_len = kern_found ? kern_t.len : 0;
    // EN: `colr_len`/`cpal_len == 0` are the sentinels `glx_sfnt_colr_layers`/`glx_sfnt_cpal_rgba`
    //     check for "no COLR"/"no CPAL" -- see this function's own `colr_t`/`cpal_t` comment above
    //     and include/core/sfnt.h's `glx_sfnt_face::colr_len`/`cpal_len` doc.
    // PT: `colr_len`/`cpal_len == 0` são os sentinelas que o `glx_sfnt_colr_layers`/
    //     `glx_sfnt_cpal_rgba` checam pra "sem COLR"/"sem CPAL" -- ver o comentário próprio
    //     `colr_t`/`cpal_t` desta função acima e o doc do `glx_sfnt_face::colr_len`/`cpal_len` do
    //     include/core/sfnt.h.
    out->colr_off = colr_found ? colr_t.off : 0;
    out->colr_len = colr_found ? colr_t.len : 0;
    out->cpal_off = cpal_found ? cpal_t.off : 0;
    out->cpal_len = cpal_found ? cpal_t.len : 0;
    return GLX_SFNT_OK;
}

// ============================================================================================
// EN: glx_sfnt_glyph_id -- cmap format 4 (BMP, segment-delta) and format 12
//     (segmented-coverage, full Unicode) lookup.
// PT: glx_sfnt_glyph_id -- lookup cmap formato 4 (BMP, delta-de-segmento) e formato 12
//     (cobertura-segmentada, Unicode completo).
// ============================================================================================

// EN: Format 4 layout (all fields big-endian `uint16`/`int16` unless noted): format(2)
//     length(2) language(2) segCountX2(2) searchRange(2) entrySelector(2) rangeShift(2)
//     endCode[segCount](2 each) reservedPad(2) startCode[segCount](2 each)
//     idDelta[segCount](2 each, SIGNED) idRangeOffset[segCount](2 each)
//     glyphIdArray[...](2 each, variable length). Segment search is a LINEAR scan
//     (correctness-first, same trade-off as `find_table` above -- real fonts carry at most a
//     few hundred segments).
// PT: Layout do formato 4 (todo campo `uint16`/`int16` big-endian salvo indicado): format(2)
//     length(2) language(2) segCountX2(2) searchRange(2) entrySelector(2) rangeShift(2)
//     endCode[segCount](2 cada) reservedPad(2) startCode[segCount](2 cada)
//     idDelta[segCount](2 cada, COM SINAL) idRangeOffset[segCount](2 cada)
//     glyphIdArray[...](2 cada, tamanho variável). A busca de segmento é uma varredura LINEAR
//     (correção-primeiro, mesma troca do `find_table` acima -- fontes reais carregam no máximo
//     umas centenas de segmentos).
static uint32_t cmap_lookup_format4(const uint8_t* blob, size_t len, size_t sub_off,
                                     uint32_t codepoint, uint16_t num_glyphs) {
    if (codepoint > 0xFFFFu) return 0; // format 4 cannot represent beyond the BMP

    uint16_t seg_count_x2;
    if (!rd_u16(blob, len, sub_off + 6, &seg_count_x2)) return 0;
    uint16_t seg_count = seg_count_x2 / 2;
    if (seg_count == 0) return 0;

    size_t end_code_off = sub_off + 14;
    size_t start_code_off = end_code_off + (size_t)seg_count * 2 + 2; // +2 skips reservedPad
    size_t id_delta_off = start_code_off + (size_t)seg_count * 2;
    size_t id_range_off_off = id_delta_off + (size_t)seg_count * 2;

    for (uint16_t i = 0; i < seg_count; i++) {
        uint16_t end_code;
        if (!rd_u16(blob, len, end_code_off + (size_t)i * 2, &end_code)) return 0;
        if ((uint32_t)end_code < codepoint) continue;

        uint16_t start_code;
        if (!rd_u16(blob, len, start_code_off + (size_t)i * 2, &start_code)) return 0;
        if ((uint32_t)start_code > codepoint) return 0; // codepoint falls in a gap between segments

        int16_t id_delta;
        if (!rd_i16(blob, len, id_delta_off + (size_t)i * 2, &id_delta)) return 0;
        uint16_t id_range_offset;
        size_t this_range_off_addr = id_range_off_off + (size_t)i * 2;
        if (!rd_u16(blob, len, this_range_off_addr, &id_range_offset)) return 0;

        uint16_t raw_gid;
        if (id_range_offset == 0) {
            raw_gid = (uint16_t)(codepoint + (uint32_t)(uint16_t)id_delta);
        } else {
            // EN: Self-relative glyphIdArray addressing, per spec: the address of the entry to
            //     read is THIS idRangeOffset field's own address, plus its value, plus
            //     2*(codepoint - startCode).
            // PT: Endereçamento auto-relativo do glyphIdArray, conforme a spec: o endereço da
            //     entrada a ler é o próprio endereço deste campo idRangeOffset, mais seu valor,
            //     mais 2*(codepoint - startCode).
            size_t glyph_addr = this_range_off_addr + (size_t)id_range_offset +
                                 2u * (size_t)(codepoint - start_code);
            uint16_t glyph_index;
            if (!rd_u16(blob, len, glyph_addr, &glyph_index)) return 0;
            if (glyph_index == 0) return 0; // explicit "no mapping" marker
            raw_gid = (uint16_t)(glyph_index + (uint32_t)(uint16_t)id_delta);
        }
        if (raw_gid >= num_glyphs) return 0; // defense in depth: never hand back an OOB gid
        return raw_gid;
    }
    return 0;
}

// EN: Format 12 layout: format(2) reserved(2) length(4) language(4) numGroups(4)
//     groups[numGroups] of {startCharCode(4) endCharCode(4) startGlyphID(4)}. Groups are
//     spec-required to be sorted/non-overlapping, but this scan does not rely on that (linear,
//     correctness-first) -- an unsorted/hostile group table simply yields "no match" safely
//     rather than a wrong-but-still-bounds-checked one.
// PT: Layout do formato 12: format(2) reserved(2) length(4) language(4) numGroups(4)
//     groups[numGroups] de {startCharCode(4) endCharCode(4) startGlyphID(4)}. A spec exige
//     grupos ordenados/não-sobrepostos, mas esta varredura não depende disso (linear,
//     correção-primeiro) -- uma tabela de grupo desordenada/hostil simplesmente resulta em
//     "sem match" com segurança, em vez de um match errado-mas-ainda-com-limite-checado.
static uint32_t cmap_lookup_format12(const uint8_t* blob, size_t len, size_t sub_off,
                                      uint32_t codepoint, uint16_t num_glyphs) {
    uint32_t num_groups;
    if (!rd_u32(blob, len, sub_off + 12, &num_groups)) return 0;

    size_t groups_off = sub_off + 16;
    if (!bounds_ok(len, groups_off, (size_t)num_groups * 12)) return 0;

    for (uint32_t i = 0; i < num_groups; i++) {
        size_t g_off = groups_off + (size_t)i * 12;
        uint32_t start_char, end_char, start_gid;
        if (!rd_u32(blob, len, g_off, &start_char)) return 0;
        if (!rd_u32(blob, len, g_off + 4, &end_char)) return 0;
        if (codepoint < start_char || codepoint > end_char) continue;
        if (!rd_u32(blob, len, g_off + 8, &start_gid)) return 0;
        // EN: `codepoint - start_char` cannot underflow here (start_char <= codepoint just
        //     checked); the sum could in principle exceed uint32_t range for a maliciously huge
        //     start_gid, but a resulting wraparound still lands on SOME uint32_t value that the
        //     `>= num_glyphs` check right below rejects if it is not a real glyph id.
        // PT: `codepoint - start_char` não pode dar underflow aqui (start_char <= codepoint
        //     acabou de ser checado); a soma poderia em princípio exceder a faixa de uint32_t
        //     pra um start_gid maliciosamente enorme, mas o wraparound resultante ainda cai em
        //     ALGUM valor uint32_t que a checagem `>= num_glyphs` logo abaixo rejeita se não for
        //     um id de glyph real.
        uint32_t gid = start_gid + (codepoint - start_char);
        if (gid >= (uint32_t)num_glyphs) return 0;
        return gid;
    }
    return 0;
}

uint32_t glx_sfnt_glyph_id(const glx_sfnt_face* face, uint32_t codepoint) {
    if (face == NULL || face->cmap_format == 0) return 0;
    if (face->cmap_format == 4) {
        return cmap_lookup_format4(face->blob, face->len, face->cmap_subtable_off, codepoint,
                                    face->num_glyphs);
    }
    if (face->cmap_format == 12) {
        return cmap_lookup_format12(face->blob, face->len, face->cmap_subtable_off, codepoint,
                                     face->num_glyphs);
    }
    return 0;
}

// ============================================================================================
// EN: glx_sfnt_hmetrics -- hmtx lookup, with the "shared trailing advance width" rule
//     (glyphs beyond numberOfHMetrics reuse the LAST explicit advance width, per spec -- the
//     usual case for monospace-tail fonts, and simply "every glyph has its own entry" when
//     numberOfHMetrics == numGlyphs, which Open Sans does).
// PT: glx_sfnt_hmetrics -- lookup do hmtx, com a regra de "largura de avanço final
//     compartilhada" (glyphs além de numberOfHMetrics reusam a ÚLTIMA largura de avanço
//     explícita, conforme a spec -- o caso usual pra fontes com cauda monoespaçada, e
//     simplesmente "todo glyph tem sua própria entrada" quando numberOfHMetrics == numGlyphs,
//     que é o caso da Open Sans).
// ============================================================================================

glx_sfnt_result glx_sfnt_hmetrics(const glx_sfnt_face* face, uint32_t gid,
                                   uint16_t* advance_width_out, int16_t* lsb_out) {
    if (face == NULL || advance_width_out == NULL || lsb_out == NULL) {
        return GLX_SFNT_ERR_INVALID_ARG;
    }
    if (gid >= face->num_glyphs) return GLX_SFNT_ERR_INVALID_ARG;

    if (gid < face->num_h_metrics) {
        size_t rec_off = face->hmtx_off + (size_t)gid * 4;
        uint16_t adv;
        int16_t lsb;
        if (!rd_u16(face->blob, face->len, rec_off, &adv)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_i16(face->blob, face->len, rec_off + 2, &lsb)) return GLX_SFNT_ERR_TRUNCATED;
        *advance_width_out = adv;
        *lsb_out = lsb;
        return GLX_SFNT_OK;
    }

    // EN: `gid >= num_h_metrics`: advance width is the LAST explicit entry's; lsb comes from
    //     the trailing lsb-only array right after the `numHMetrics` full entries.
    // PT: `gid >= num_h_metrics`: a largura de avanço é da ÚLTIMA entrada explícita; o lsb vem
    //     do array só-de-lsb no final, logo após as `numHMetrics` entradas completas.
    size_t last_full_off = (size_t)(face->num_h_metrics - 1) * 4;
    uint16_t adv;
    if (!rd_u16(face->blob, face->len, face->hmtx_off + last_full_off, &adv)) {
        return GLX_SFNT_ERR_TRUNCATED;
    }
    size_t lsb_off = face->hmtx_off + (size_t)face->num_h_metrics * 4 +
                      (size_t)(gid - face->num_h_metrics) * 2;
    int16_t lsb;
    if (!rd_i16(face->blob, face->len, lsb_off, &lsb)) return GLX_SFNT_ERR_TRUNCATED;
    *advance_width_out = adv;
    *lsb_out = lsb;
    return GLX_SFNT_OK;
}

// ============================================================================================
// EN: glx_sfnt_glyph_outline -- loca/glyf lookup, simple-glyph point/flag decoding, and
//     recursive composite-glyph flattening.
// PT: glx_sfnt_glyph_outline -- lookup de loca/glyf, decodificação de ponto/flag de glyph
//     simples, e achatamento recursivo de glyph composto.
// ============================================================================================

// EN: `loca[gid]`/`loca[gid+1]` -- the glyf-relative byte range for `gid`'s own glyph record.
//     `glx_sfnt_open` already validated `(num_glyphs+1)` entries of the right width fit within
//     the blob, so these reads cannot fail for a `gid < num_glyphs` -- checked again anyway
//     (cheap, and this function does not otherwise know that invariant holds for whatever `face`
//     it was handed).
// PT: `loca[gid]`/`loca[gid+1]` -- o intervalo de byte relativo-ao-glyf pro próprio registro de
//     glyph do `gid`. O `glx_sfnt_open` já validou que `(num_glyphs+1)` entradas da largura
//     certa cabem dentro do blob, então essas leituras não podem falhar pra um
//     `gid < num_glyphs` -- checado de novo assim mesmo (barato, e esta função não sabe de outra
//     forma que esse invariante vale pro `face` que recebeu).
static int loca_read(const glx_sfnt_face* face, uint32_t gid, size_t* off_out) {
    if (face->index_to_loc_format == 0) {
        uint16_t raw;
        if (!rd_u16(face->blob, face->len, face->loca_off + (size_t)gid * 2, &raw)) return 0;
        *off_out = (size_t)raw * 2;
    } else {
        uint32_t raw;
        if (!rd_u32(face->blob, face->len, face->loca_off + (size_t)gid * 4, &raw)) return 0;
        *off_out = (size_t)raw;
    }
    return 1;
}

// EN: Applies a composite component's transform: `(x', y') = (a*x + c*y + dx, b*x + d*y + dy)`,
//     rounded to the nearest `int16_t`. Open Sans' own composites (the pt-br accents this
//     ticket cares about) all use the identity 2x2 (`a=d=1, b=c=0`, plain translate) -- this
//     general form is implemented anyway because the SPEC allows scale/2x2 on any composite
//     component and a hostile or simply different font may use one; `float` is used for the
//     multiply-accumulate (already an established, tested primitive in this codebase --
//     `src/conv.c`'s `ftoa`/`atof`, SOV-FCONV -- not a new "is float safe here" question).
// PT: Aplica a transformação de um componente composto: `(x', y') = (a*x + c*y + dx, b*x + d*y
//     + dy)`, arredondado pro `int16_t` mais próximo. Os próprios compostos da Open Sans (os
//     acentos pt-br com que esta tarefa se importa) todos usam o 2x2 identidade (`a=d=1, b=c=0`,
//     translação pura) -- esta forma geral é implementada assim mesmo porque a SPEC permite
//     escala/2x2 em qualquer componente composto e uma fonte hostil ou simplesmente diferente
//     pode usar uma; `float` é usado pro multiply-accumulate (já uma primitiva estabelecida e
//     testada neste código-base -- `ftoa`/`atof` do src/conv.c, SOV-FCONV -- não uma pergunta
//     nova de "float é seguro aqui").
// EN: Rounds `v` to the nearest integer (the same "round half away from zero" shape the two call
//     sites used inline before this fix) and SATURATES the result into `[INT16_MIN, INT16_MAX]`
//     before ever casting to `int16_t` -- review-adversarial CRITICAL fix (SOV-SFNT review):
//     `(int16_t)(fx +/- 0.5f)` with NO range check is undefined behaviour (C 6.3.1.4) whenever
//     the rounded value falls outside int16 range, and this is NOT a hostile-input-only
//     scenario -- a spec-legitimate composite (translate-only, no scale, exactly the shape Open
//     Sans' own accented glyphs use) whose child glyph + `dx`/`dy` sum lands outside int16 hits
//     it too (reviewer's repro: a simple glyph point at x=20000 plus a composite `dx=20000`
//     computes `fx=40000.5`, casting that to `int16_t` is UB and produced silent garbage,
//     `-25536`, while still returning `GLX_SFNT_OK`). `WE_HAVE_A_SCALE`/2x2 with an F2Dot14 factor
//     up to ~2x reaches the same range from smaller inputs.
//
//     CHOICE -- saturate, don't reject as `GLX_SFNT_ERR_BAD_GLYPH`: a font whose OWN declared
//     `glyf` point range is well within int16 (int16 is literally the wire type for
//     `xCoordinates`/`yCoordinates`, see `parse_simple_glyph`) can still, after a LEGITIMATE
//     transform, produce an out-of-range composite coordinate -- that is a degenerate output
//     (a glyph drawn absurdly far from the origin), not a malformed INPUT; rejecting the whole
//     glyph would punish a real font for a corner case that never affects Open Sans (whose only
//     composites are the identity-2x2 translate-only accents this ticket cares about) while
//     saturating keeps every other point/contour of the SAME glyph usable. Saturating through an
//     intermediate comparison against `(float)INT16_MIN`/`(float)INT16_MAX` (both exactly
//     representable in `float`, `<stdint.h>` int16 range comfortably fits a 24-bit mantissa) means
//     the ONLY value ever hand to `(int16_t)` is either one of those two exact bounds or an
//     already-in-range rounded value -- never a value the standard leaves undefined.
// PT: Arredonda `v` pro inteiro mais próximo (a mesma forma "arredonda a metade pra longe de
//     zero" que os dois pontos de chamada usavam inline antes deste fix) e SATURA o resultado em
//     `[INT16_MIN, INT16_MAX]` antes de qualquer cast pra `int16_t` -- fix do CRÍTICO do review
//     adversarial (review do SOV-SFNT): `(int16_t)(fx +/- 0.5f)` SEM checagem de faixa é
//     comportamento indefinido (C 6.3.1.4) sempre que o valor arredondado cai fora da faixa
//     int16, e isto NÃO é um cenário só-de-input-hostil -- um composto espec-legítimo (só
//     translação, sem escala, exatamente a forma que os próprios glyphs acentuados da Open Sans
//     usam) cujo glyph filho + soma de `dx`/`dy` caia fora do int16 também dispara (repro do
//     revisor: um glyph simples com ponto em x=20000 mais um composto `dx=20000` computa
//     `fx=40000.5`, converter isso pra `int16_t` é UB e produzia lixo silencioso, `-25536`,
//     enquanto ainda retornava `GLX_SFNT_OK`). `WE_HAVE_A_SCALE`/2x2 com um fator F2Dot14 até
//     ~2x alcança a mesma faixa a partir de entradas menores.
//
//     ESCOLHA -- saturar, não rejeitar como `GLX_SFNT_ERR_BAD_GLYPH`: uma fonte cuja PRÓPRIA
//     faixa declarada de ponto `glyf` está bem dentro do int16 (int16 é literalmente o tipo de
//     fio de `xCoordinates`/`yCoordinates`, ver `parse_simple_glyph`) ainda pode, depois de uma
//     transformação LEGÍTIMA, produzir uma coordenada composta fora de faixa -- isso é uma saída
//     degenerada (um glyph desenhado absurdamente longe da origem), não um INPUT malformado;
//     rejeitar o glyph inteiro puniria uma fonte real por um caso de borda que nunca afeta a Open
//     Sans (cujos únicos compostos são os acentos translação-só de 2x2 identidade com que esta
//     tarefa se importa) enquanto saturar mantém todo outro ponto/contorno do MESMO glyph
//     utilizável. Saturar através de uma comparação intermediária contra `(float)INT16_MIN`/
//     `(float)INT16_MAX` (ambos exatamente representáveis em `float`, a faixa int16 do
//     `<stdint.h>` cabe folgadamente numa mantissa de 24 bits) significa que o ÚNICO valor que
//     algum dia chega no `(int16_t)` é ou um daqueles dois limites exatos ou um valor já
//     arredondado dentro de faixa -- nunca um valor que o padrão deixa indefinido.
// EN: CONTRACT -- `v` must be finite (never Inf/NaN) at every call site this file has TODAY
//     (`apply_component_transform`'s `fx`/`fy`: F2Dot14 scale factors are always finite by
//     construction -- `rd_f2dot14` divides a bounded `int16_t` by a compile-time constant -- and
//     `x_in`/`y_in` are `int16_t`, `dx`/`dy` are `int8_t`/`int16_t` widened to `int32_t`, so the
//     multiply-accumulate below can never produce Inf/NaN from THESE inputs). Review-adversarial
//     IMPORTANT fix (SOV-SFNT dominoed re-review, sibling of the CRITICAL saturation fix above):
//     this helper did not GUARD or DOCUMENT that precondition -- if `v` were ever NaN, BOTH
//     `r >= (float)INT16_MAX` and `r <= (float)INT16_MIN` evaluate `false` (every comparison
//     against NaN is `false`, IEEE 754), falling through to `(int16_t)r` with `r` still NaN,
//     which is undefined behaviour (C 6.3.1.4) -- the exact same UB class the CRITICAL fix closed
//     for out-of-range FINITE values, reopened for the NaN case specifically. Unreachable from
//     today's call sites, but latent: a future call site (or a future widening of the F2Dot14/
//     dx/dy input types) could pass NaN without any compiler warning. The guard below is defense
//     in depth, consistent with this helper's existing "saturate, never UB" contract -- NaN has no
//     ordering-based "nearest in-range value" the way a too-large finite float does, so it
//     saturates to `0` (the origin, a degenerate-but-safe coordinate) rather than picking either
//     bound arbitrarily. `v != v` is the portable NaN test (works with `-ffreestanding`, no
//     `<math.h>`/`isnan` -- NaN is the only float value that never compares equal to itself).
// PT: CONTRATO -- `v` precisa ser finito (nunca Inf/NaN) em todo ponto de chamada que este
//     arquivo tem HOJE (`fx`/`fy` do `apply_component_transform`: fatores de escala F2Dot14 são
//     sempre finitos por construção -- `rd_f2dot14` divide um `int16_t` limitado por uma
//     constante de tempo de compilação -- e `x_in`/`y_in` são `int16_t`, `dx`/`dy` são
//     `int8_t`/`int16_t` alargados pra `int32_t`, então o multiply-accumulate abaixo nunca
//     consegue produzir Inf/NaN a partir DESSAS entradas). Fix do IMPORTANTE do review
//     adversarial (varredura-dominó do re-review do SOV-SFNT, irmão do fix do CRÍTICO acima):
//     este helper não GUARDAVA nem DOCUMENTAVA essa precondição -- se `v` algum dia fosse NaN,
//     TANTO `r >= (float)INT16_MAX` QUANTO `r <= (float)INT16_MIN` avaliam `false` (toda
//     comparação contra NaN é `false`, IEEE 754), caindo no `(int16_t)r` com `r` ainda NaN, o que
//     é comportamento indefinido (C 6.3.1.4) -- exatamente a mesma classe de UB que o fix do
//     CRÍTICO fechou pros valores FINITOS fora de faixa, reaberta especificamente pro caso NaN.
//     Inalcançável pelos pontos de chamada de hoje, mas latente: um futuro ponto de chamada (ou
//     um futuro alargamento dos tipos de entrada F2Dot14/dx/dy) poderia passar NaN sem warning
//     nenhum do compilador. A guarda abaixo é defense in depth, consistente com o contrato já
//     existente deste helper de "satura, nunca UB" -- NaN não tem um "valor em-faixa mais
//     próximo" baseado em ordenação como um float finito grande demais tem, então satura pra `0`
//     (a origem, uma coordenada degenerada-mas-segura) em vez de escolher um dos dois limites
//     arbitrariamente. `v != v` é o teste de NaN portável (funciona com `-ffreestanding`, sem
//     `<math.h>`/`isnan` -- NaN é o único valor float que nunca se compara igual a si mesmo).
static int16_t round_and_saturate_i16(float v) {
    if (v != v) return 0; // NaN guard -- see CONTRACT comment above
    float r = (v >= 0.0f) ? v + 0.5f : v - 0.5f;
    if (r >= (float)INT16_MAX) return INT16_MAX;
    if (r <= (float)INT16_MIN) return INT16_MIN;
    return (int16_t)r;
}

// EN: SATURATES `v` into `[INT16_MIN, INT16_MAX]` before casting to `int16_t` -- the int32->int16
//     twin of `round_and_saturate_i16` above (that one saturates a FLOAT rounding result; this one
//     saturates an INTEGER accumulator, no rounding involved, so it is not shared code -- an
//     unconditional `(int16_t)v` cast on an out-of-range `int32_t` is NOT undefined behaviour in
//     C -- narrowing a signed integer to a narrower signed type when the value does not fit is
//     IMPLEMENTATION-DEFINED (C 6.3.1.3p3), not UB -- but "implementation-defined" here still means
//     SILENT, VALUE-CORRUPTING truncation/wraparound on every mainstream two's-complement target
//     (including this codebase's own x86-64/clang), and critically: ASan/UBSan do NOT flag
//     implementation-defined narrowing the way they flag UB, so this class of bug is invisible to
//     `make sanitize-sfnt` and needs a VALUE assertion in `tests/test_sfnt.c` instead (see that
//     file's `kSyntheticFontSimpleDeltaOverflow` regression). Review-adversarial IMPORTANT fix
//     (SOV-SFNT dominoed re-review, 2nd sibling of the CRITICAL fix above): `parse_simple_glyph`'s
//     `x_acc`/`y_acc` are `int32_t` running sums of delta-encoded `xCoordinates`/`yCoordinates`
//     (each individual delta fits `int16_t`, but nothing bounds the RUNNING SUM -- a `glyf` simple
//     glyph is free to walk far from the origin one small step at a time, same "spec-legitimate,
//     not just hostile-input" shape as the CRITICAL composite-transform fix), and the un-clamped
//     `(int16_t)x_acc`/`(int16_t)y_acc` casts silently wrapped the sum modulo 65536 whenever it
//     left `[-32768, 32767]` -- reviewer's live repro: two successive `+32767` x-deltas sum to
//     `65534` (in range for `int32_t`, provably correct per the OpenType delta-encoding spec, cross-
//     checked against `fontTools`), and the pre-fix cast produced `-2` (two's-complement wrap),
//     while `glx_sfnt_glyph_outline` still returned `GLX_SFNT_OK` -- silent wrong geometry, no
//     crash, no sanitizer finding. Same choice as the CRITICAL fix: SATURATE, do not reject the
//     glyph -- an out-of-range running sum is a degenerate OUTPUT (a point drawn absurdly far from
//     the glyph's own origin), not a malformed INPUT (every individual delta byte/word read here
//     was perfectly well-formed), so clamping keeps the rest of the SAME glyph's points usable.
// PT: SATURA `v` em `[INT16_MIN, INT16_MAX]` antes de converter pra `int16_t` -- o gêmeo
//     int32->int16 do `round_and_saturate_i16` acima (aquele satura um resultado de arredondamento
//     de FLOAT; este satura um acumulador INTEIRO, sem arredondamento envolvido, por isso não é
//     código compartilhado -- um cast `(int16_t)v` incondicional sobre um `int32_t` fora de faixa
//     NÃO é comportamento indefinido em C -- estreitar um inteiro com sinal pra um tipo com sinal
//     mais estreito quando o valor não cabe é DEFINIDO-PELA-IMPLEMENTAÇÃO (C 6.3.1.3p3), não UB --
//     mas "definido-pela-implementação" aqui ainda significa truncamento/wraparound SILENCIOSO e
//     CORRUPTOR-DE-VALOR em todo alvo dois-complemento mainstream (inclusive o próprio x86-64/
//     clang deste código-base), e criticamente: ASan/UBSan NÃO sinalizam estreitamento definido-
//     pela-implementação do jeito que sinalizam UB, então essa classe de bug é invisível pro `make
//     sanitize-sfnt` e precisa de uma asserção de VALOR no `tests/test_sfnt.c` em vez disso (ver a
//     regressão `kSyntheticFontSimpleDeltaOverflow` daquele arquivo). Fix do IMPORTANTE do review
//     adversarial (varredura-dominó do re-review do SOV-SFNT, 2º irmão do fix do CRÍTICO acima):
//     `x_acc`/`y_acc` do `parse_simple_glyph` são somas corridas `int32_t` de
//     `xCoordinates`/`yCoordinates` codificados por delta (cada delta individual cabe em
//     `int16_t`, mas nada limita a SOMA CORRIDA -- um glyph simples `glyf` é livre pra caminhar
//     longe da origem um passo pequeno de cada vez, a mesma forma "espec-legítima, não só input-
//     hostil" do fix do CRÍTICO de transformação-composta), e os casts sem clamp
//     `(int16_t)x_acc`/`(int16_t)y_acc` davam wrap silencioso da soma módulo 65536 sempre que ela
//     saía de `[-32768, 32767]` -- repro ao vivo do revisor: dois deltas-x sucessivos `+32767`
//     somam `65534` (dentro de faixa pra `int32_t`, comprovadamente correto conforme a spec de
//     codificação-por-delta do OpenType, cruzado contra `fontTools`), e o cast pré-fix produzia
//     `-2` (wrap dois-complemento), enquanto o `glx_sfnt_glyph_outline` ainda retornava
//     `GLX_SFNT_OK` -- geometria errada silenciosa, sem crash, sem achado de sanitizer. Mesma
//     escolha do fix do CRÍTICO: SATURAR, não rejeitar o glyph -- uma soma corrida fora de faixa é
//     uma SAÍDA degenerada (um ponto desenhado absurdamente longe da própria origem do glyph), não
//     um INPUT malformado (todo byte/word de delta individual lido aqui era perfeitamente
//     bem-formado), então o clamp mantém o resto dos pontos do MESMO glyph utilizável.
static int16_t saturate_i32_to_i16(int32_t v) {
    if (v > INT16_MAX) return INT16_MAX;
    if (v < INT16_MIN) return INT16_MIN;
    return (int16_t)v;
}

static void apply_component_transform(float a, float b, float c, float d, int32_t dx, int32_t dy,
                                       int16_t x_in, int16_t y_in, int16_t* x_out,
                                       int16_t* y_out) {
    float fx = a * (float)x_in + c * (float)y_in + (float)dx;
    float fy = b * (float)x_in + d * (float)y_in + (float)dy;
    *x_out = round_and_saturate_i16(fx);
    *y_out = round_and_saturate_i16(fy);
}

// EN: Reads one F2Dot14 fixed-point value (a signed 16-bit integer, 2 integer bits + 14
//     fractional bits -- `real_value = raw / 16384.0`) used by composite glyph scale/2x2
//     transform fields.
// PT: Lê um valor de ponto-fixo F2Dot14 (um inteiro de 16 bits com sinal, 2 bits inteiros + 14
//     bits fracionários -- `valor_real = raw / 16384.0`) usado pelos campos de transformação
//     escala/2x2 de glyph composto.
static int rd_f2dot14(const uint8_t* b, size_t len, size_t off, float* out) {
    int16_t raw;
    if (!rd_i16(b, len, off, &raw)) return 0;
    *out = (float)raw / 16384.0f;
    return 1;
}

static glx_sfnt_result parse_simple_glyph(const uint8_t* blob, size_t len, size_t g_off,
                                           int16_t num_contours, glx_sfnt_point* points_out,
                                           uint16_t points_cap, uint16_t* contour_ends_out,
                                           uint16_t contours_cap, uint16_t* num_points_out) {
    // EN: Reads below are bounds-checked against the WHOLE blob (`len`), not re-derived from
    //     the glyph's own declared byte length (`g_end`, deliberately not a parameter here --
    //     see include/core/sfnt.h's header on why a lying table length is a semantic-garbage
    //     risk, not a memory-safety one, in this design).
    // PT: As leituras abaixo têm limite checado contra o blob INTEIRO (`len`), não re-derivado
    //     do próprio tamanho de byte declarado do glyph (`g_end`, deliberadamente não um
    //     parâmetro aqui -- ver o cabeçalho do include/core/sfnt.h sobre por que um tamanho de
    //     tabela mentiroso é um risco de dado-semanticamente-errado, não de segurança-de-memória,
    //     neste desenho).
    if ((uint16_t)num_contours > contours_cap) return GLX_SFNT_ERR_BUFFER_TOO_SMALL;

    size_t cursor = g_off + 10; // past numberOfContours(2) + bbox(4*2)

    // ---- endPtsOfContours[numContours] --------------------------------------------------------
    int32_t prev_end = -1;
    for (int16_t c = 0; c < num_contours; c++) {
        uint16_t end_pt;
        if (!rd_u16(blob, len, cursor, &end_pt)) return GLX_SFNT_ERR_TRUNCATED;
        if ((int32_t)end_pt <= prev_end) return GLX_SFNT_ERR_BAD_GLYPH; // must strictly increase
        prev_end = (int32_t)end_pt;
        contour_ends_out[c] = end_pt;
        cursor += 2;
    }
    uint32_t num_points32 = (uint32_t)prev_end + 1; // prev_end is the last contour's end index
    if (num_points32 > points_cap) return GLX_SFNT_ERR_BUFFER_TOO_SMALL;
    uint16_t num_points = (uint16_t)num_points32;

    // ---- instructions: length-prefixed, skipped (no hinting interpreter -- see sfnt.h scope) --
    uint16_t instr_len;
    if (!rd_u16(blob, len, cursor, &instr_len)) return GLX_SFNT_ERR_TRUNCATED;
    cursor += 2;
    if (!bounds_ok(len, cursor, instr_len)) return GLX_SFNT_ERR_TRUNCATED;
    cursor += instr_len;

    // ---- flags[numPoints], run-length encoded (REPEAT_FLAG=0x08) ------------------------------
    // EN: Stashes the RAW flag byte into `points_out[i].on_curve` as scratch storage -- reused
    //     by the x/y decode passes below to know each point's encoding mode, then reduced to the
    //     real 0/1 on-curve bit in the final pass. See this file's header comment block above
    //     `parse_simple_glyph` -- wait, no such block exists; the rationale is: no malloc, no
    //     separate scratch array, `on_curve` is a `uint8_t` wide enough to hold the whole flag
    //     byte, and every later pass that needs the raw byte reads it back before the final pass
    //     overwrites it.
    // PT: Guarda o BYTE DE FLAG CRU em `points_out[i].on_curve` como armazenamento de rascunho
    //     -- reusado pelas passadas de decodificação x/y abaixo pra saber o modo de codificação
    //     de cada ponto, depois reduzido pro bit real 0/1 de on-curve na passada final. Racional:
    //     sem malloc, sem array de rascunho separado, `on_curve` é um `uint8_t` largo o
    //     suficiente pra caber o byte de flag inteiro, e toda passada posterior que precisa do
    //     byte cru o lê de volta antes da passada final sobrescrever.
    for (uint16_t i = 0; i < num_points;) {
        uint8_t flag;
        if (!rd_u8(blob, len, cursor, &flag)) return GLX_SFNT_ERR_TRUNCATED;
        cursor += 1;
        uint16_t repeat = 0;
        if (flag & 0x08) {
            uint8_t rep;
            if (!rd_u8(blob, len, cursor, &rep)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 1;
            repeat = rep;
        }
        points_out[i].on_curve = flag;
        i++;
        for (uint16_t r = 0; r < repeat && i < num_points; r++, i++) {
            points_out[i].on_curve = flag;
        }
    }

    // ---- xCoordinates[numPoints], delta-encoded per flag bits 1 (short) / 4 (same-or-positive) -
    int32_t x_acc = 0;
    for (uint16_t i = 0; i < num_points; i++) {
        uint8_t flag = points_out[i].on_curve;
        int32_t dx = 0;
        if (flag & 0x02) { // X_SHORT_VECTOR
            uint8_t mag;
            if (!rd_u8(blob, len, cursor, &mag)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 1;
            dx = (flag & 0x10) ? (int32_t)mag : -(int32_t)mag;
        } else if (!(flag & 0x10)) { // not short, not "same" -> full int16 delta
            int16_t d;
            if (!rd_i16(blob, len, cursor, &d)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 2;
            dx = d;
        } // else: X_IS_SAME_OR_POSITIVE with no short vector => delta 0, no bytes consumed
        x_acc += dx;
        points_out[i].x = saturate_i32_to_i16(x_acc); // IMPORTANT fix: was an un-clamped narrowing cast
    }

    // ---- yCoordinates[numPoints], same shape, flag bits 2 (short) / 5 (same-or-positive) ------
    int32_t y_acc = 0;
    for (uint16_t i = 0; i < num_points; i++) {
        uint8_t flag = points_out[i].on_curve;
        int32_t dy = 0;
        if (flag & 0x04) { // Y_SHORT_VECTOR
            uint8_t mag;
            if (!rd_u8(blob, len, cursor, &mag)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 1;
            dy = (flag & 0x20) ? (int32_t)mag : -(int32_t)mag;
        } else if (!(flag & 0x20)) {
            int16_t d;
            if (!rd_i16(blob, len, cursor, &d)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 2;
            dy = d;
        }
        y_acc += dy;
        points_out[i].y = saturate_i32_to_i16(y_acc); // IMPORTANT fix: was an un-clamped narrowing cast
    }

    // ---- finalize: reduce the scratch raw-flag byte down to the real on_curve bit --------------
    for (uint16_t i = 0; i < num_points; i++) {
        points_out[i].on_curve = (uint8_t)(points_out[i].on_curve & 0x01);
    }

    *num_points_out = num_points;
    return GLX_SFNT_OK;
}

// EN: Recursive core shared by the public `glx_sfnt_glyph_outline` -- `depth` starts at 0 and is
//     the ONLY thing that stops a self-referencing (or long-chained) composite from recursing
//     forever, see include/core/sfnt.h's `GLX_SFNT_MAX_COMPOSITE_DEPTH`.
// PT: Núcleo recursivo compartilhado pelo `glx_sfnt_glyph_outline` público -- `depth` começa em
//     0 e é a ÚNICA coisa que impede um composto auto-referente (ou encadeado longamente) de
//     recursar pra sempre, ver o `GLX_SFNT_MAX_COMPOSITE_DEPTH` do include/core/sfnt.h.
static glx_sfnt_result glyph_outline_rec(const glx_sfnt_face* face, uint32_t gid,
                                          glx_sfnt_point* points_out, uint16_t points_cap,
                                          uint16_t* contour_ends_out, uint16_t contours_cap,
                                          uint16_t* num_points_out, uint16_t* num_contours_out,
                                          int depth) {
    if (depth > GLX_SFNT_MAX_COMPOSITE_DEPTH) return GLX_SFNT_ERR_TOO_DEEP;
    if (gid >= face->num_glyphs) return GLX_SFNT_ERR_BAD_GLYPH;

    size_t off_a, off_b;
    if (!loca_read(face, gid, &off_a)) return GLX_SFNT_ERR_TRUNCATED;
    if (!loca_read(face, gid + 1, &off_b)) return GLX_SFNT_ERR_TRUNCATED;
    // EN: The hostile case this ticket's brief names explicitly: a descending `loca` pair.
    // PT: O caso hostil que o brief desta tarefa nomeia explicitamente: um par `loca`
    //     descendente.
    if (off_b < off_a) return GLX_SFNT_ERR_BAD_OFFSET;

    if (off_b == off_a) {
        // EN: Empty glyph (e.g. `space`) -- no glyf record at all, not an error.
        // PT: Glyph vazio (ex.: `space`) -- nenhum registro glyf, sem erro nenhum.
        *num_points_out = 0;
        *num_contours_out = 0;
        return GLX_SFNT_OK;
    }

    size_t g_off = face->glyf_off + off_a;
    if (!bounds_ok(face->len, g_off, off_b - off_a)) return GLX_SFNT_ERR_TRUNCATED;

    int16_t num_contours;
    if (!rd_i16(face->blob, face->len, g_off, &num_contours)) return GLX_SFNT_ERR_TRUNCATED;

    if (num_contours >= 0) {
        glx_sfnt_result r = parse_simple_glyph(face->blob, face->len, g_off, num_contours,
                                                points_out, points_cap, contour_ends_out,
                                                contours_cap, num_points_out);
        if (r != GLX_SFNT_OK) return r;
        *num_contours_out = (uint16_t)num_contours;
        return GLX_SFNT_OK;
    }
    if (num_contours != -1) return GLX_SFNT_ERR_BAD_GLYPH; // spec: only -1 means "composite"

    // ---- composite glyph: sequence of components, each recursively resolved -------------------
    size_t cursor = g_off + 10; // past numberOfContours(2) + bbox(4*2)
    uint16_t point_cursor = 0;
    uint16_t contour_cursor = 0;
    for (;;) {
        uint16_t flags, glyph_index;
        if (!rd_u16(face->blob, face->len, cursor, &flags)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_u16(face->blob, face->len, cursor + 2, &glyph_index)) return GLX_SFNT_ERR_TRUNCATED;
        cursor += 4;

        int32_t dx, dy;
        if (flags & 0x0001) { // ARG_1_AND_2_ARE_WORDS
            int16_t a1, a2;
            if (!rd_i16(face->blob, face->len, cursor, &a1)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_i16(face->blob, face->len, cursor + 2, &a2)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 4;
            dx = a1;
            dy = a2;
        } else {
            int8_t a1, a2;
            if (!rd_i8(face->blob, face->len, cursor, &a1)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_i8(face->blob, face->len, cursor + 1, &a2)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 2;
            dx = a1;
            dy = a2;
        }
        if (!(flags & 0x0002)) {
            // EN: ARGS_ARE_XY_VALUES unset -> point-matching mode, out of scope (see sfnt.h).
            // PT: ARGS_ARE_XY_VALUES desmarcado -> modo de casamento-de-ponto, fora de escopo
            //     (ver sfnt.h).
            return GLX_SFNT_ERR_UNSUPPORTED;
        }

        float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f;
        if (flags & 0x0008) { // WE_HAVE_A_SCALE
            if (!rd_f2dot14(face->blob, face->len, cursor, &a)) return GLX_SFNT_ERR_TRUNCATED;
            d = a;
            cursor += 2;
        } else if (flags & 0x0040) { // WE_HAVE_AN_X_AND_Y_SCALE
            if (!rd_f2dot14(face->blob, face->len, cursor, &a)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_f2dot14(face->blob, face->len, cursor + 2, &d)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 4;
        } else if (flags & 0x0080) { // WE_HAVE_A_TWO_BY_TWO
            if (!rd_f2dot14(face->blob, face->len, cursor, &a)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_f2dot14(face->blob, face->len, cursor + 2, &b)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_f2dot14(face->blob, face->len, cursor + 4, &c)) return GLX_SFNT_ERR_TRUNCATED;
            if (!rd_f2dot14(face->blob, face->len, cursor + 6, &d)) return GLX_SFNT_ERR_TRUNCATED;
            cursor += 8;
        }

        uint16_t child_points, child_contours;
        glx_sfnt_result r = glyph_outline_rec(
            face, glyph_index, points_out + point_cursor,
            (uint16_t)(points_cap - point_cursor), contour_ends_out + contour_cursor,
            (uint16_t)(contours_cap - contour_cursor), &child_points, &child_contours, depth + 1);
        if (r != GLX_SFNT_OK) return r;

        // EN: The child wrote its OWN 0-based contour-end indices into
        //     `contour_ends_out[contour_cursor..)` -- offset them by `point_cursor` so they
        //     index correctly into the PARENT's flattened `points_out` array.
        // PT: O filho escreveu os PRÓPRIOS índices de fim-de-contorno 0-based em
        //     `contour_ends_out[contour_cursor..)` -- desloca eles por `point_cursor` pra
        //     indexarem corretamente no array `points_out` achatado do PAI.
        for (uint16_t k = 0; k < child_contours; k++) {
            contour_ends_out[contour_cursor + k] =
                (uint16_t)(contour_ends_out[contour_cursor + k] + point_cursor);
        }
        // EN: Transform the child's freshly-written points in place.
        // PT: Transforma os pontos recém-escritos do filho no próprio lugar.
        for (uint16_t k = 0; k < child_points; k++) {
            glx_sfnt_point* p = &points_out[point_cursor + k];
            apply_component_transform(a, b, c, d, dx, dy, p->x, p->y, &p->x, &p->y);
        }

        point_cursor = (uint16_t)(point_cursor + child_points);
        contour_cursor = (uint16_t)(contour_cursor + child_contours);

        if (!(flags & 0x0020)) break; // MORE_COMPONENTS unset -> this was the last component
    }

    *num_points_out = point_cursor;
    *num_contours_out = contour_cursor;
    return GLX_SFNT_OK;
}

glx_sfnt_result glx_sfnt_glyph_outline(const glx_sfnt_face* face, uint32_t gid,
                                        glx_sfnt_point* points_out, uint16_t points_capacity,
                                        uint16_t* contour_ends_out, uint16_t contours_capacity,
                                        glx_sfnt_outline* out) {
    if (face == NULL || out == NULL) return GLX_SFNT_ERR_INVALID_ARG;
    if (gid >= face->num_glyphs) return GLX_SFNT_ERR_INVALID_ARG;
    if ((points_capacity != 0 && points_out == NULL) ||
        (contours_capacity != 0 && contour_ends_out == NULL)) {
        return GLX_SFNT_ERR_INVALID_ARG;
    }

    // EN: The glyph record's own bbox (font units) -- valid for BOTH simple and composite
    //     glyphs, read once here regardless of which branch `glyph_outline_rec` takes below (an
    //     empty glyph, per spec, has no glyf record at all, hence no bbox -- all-zero is correct
    //     there).
    // PT: A própria bbox do registro do glyph (unidades de fonte) -- válida tanto pra glyph
    //     simples QUANTO composto, lida uma vez aqui independente de qual ramo o
    //     `glyph_outline_rec` toma abaixo (um glyph vazio, conforme a spec, não tem registro
    //     glyf nenhum, logo sem bbox -- tudo-zero é correto ali).
    int16_t x_min = 0, y_min = 0, x_max = 0, y_max = 0;
    size_t off_a, off_b;
    if (!loca_read(face, gid, &off_a)) return GLX_SFNT_ERR_TRUNCATED;
    if (!loca_read(face, gid + 1, &off_b)) return GLX_SFNT_ERR_TRUNCATED;
    if (off_b < off_a) return GLX_SFNT_ERR_BAD_OFFSET;
    if (off_b > off_a) {
        size_t g_off = face->glyf_off + off_a;
        if (!bounds_ok(face->len, g_off, off_b - off_a)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_i16(face->blob, face->len, g_off + 2, &x_min)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_i16(face->blob, face->len, g_off + 4, &y_min)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_i16(face->blob, face->len, g_off + 6, &x_max)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_i16(face->blob, face->len, g_off + 8, &y_max)) return GLX_SFNT_ERR_TRUNCATED;
    }

    uint16_t num_points, num_contours;
    glx_sfnt_result r =
        glyph_outline_rec(face, gid, points_out, points_capacity, contour_ends_out,
                           contours_capacity, &num_points, &num_contours, 0);
    if (r != GLX_SFNT_OK) return r;

    out->num_points = num_points;
    out->num_contours = num_contours;
    out->x_min = x_min;
    out->y_min = y_min;
    out->x_max = x_max;
    out->y_max = y_max;
    return GLX_SFNT_OK;
}

// ============================================================================================
// EN: glx_sfnt_kern -- 'kern' table, classic (`version == 0`) format-0, horizontal subtable pair
//     lookup. Public API contract, oracle values, and the discovered real-world "subtable length
//     field overflows for Open Sans' own 18694-pair subtable" quirk are all documented on this
//     function's own declaration in include/core/sfnt.h -- this comment block only adds the
//     LOCAL "how" (byte layout, the specific bounds-check shape chosen and why), not re-derived
//     "why"/scope that already lives there.
// PT: glx_sfnt_kern -- lookup de par na subtable clássica (`version == 0`) formato-0, horizontal,
//     da tabela `kern`. O contrato de API pública, os valores de oráculo, e a peculiaridade real
//     descoberta ("o campo de tamanho da subtable transborda pra subtable de 18694 pares da
//     própria Open Sans") estão todos documentados na declaração própria desta função no
//     include/core/sfnt.h -- este bloco de comentário só acrescenta o "como" LOCAL (layout de
//     byte, a forma exata de checagem de limite escolhida e por quê), não o "porquê"/escopo
//     já re-derivado que mora lá.
// ============================================================================================

int16_t glx_sfnt_kern(const glx_sfnt_face* face, uint32_t left_gid, uint32_t right_gid) {
    if (face == NULL || face->kern_len == 0) return 0; // no kern table -- a valid, common case

    // EN: The wire pair fields (`left`/`right` below) are `uint16_t` -- a gid above `0xFFFF`
    //     could never be stored in one, so it can never match any real pair; short-circuiting
    //     here also sidesteps any risk of a `(uint16_t)left_gid` truncation later in this
    //     function accidentally matching an unrelated small gid.
    // PT: Os campos de par no fio (`left`/`right` abaixo) são `uint16_t` -- um gid acima de
    //     `0xFFFF` nunca poderia estar guardado num, então nunca pode casar par nenhum de
    //     verdade; o curto-circuito aqui também evita qualquer risco de um truncamento
    //     `(uint16_t)left_gid` mais adiante nesta função casar por acidente um gid pequeno
    //     não-relacionado.
    if (left_gid > 0xFFFFu || right_gid > 0xFFFFu) return 0;

    const uint8_t* blob = face->blob;
    size_t len = face->len;
    size_t kern_off = face->kern_off;
    // EN: Safe to compute directly (no overflow-guard needed the way `bounds_ok`'s callers guard
    //     `off + need`): `kern_off`/`kern_len` were already proven `kern_off + kern_len <= len`
    //     by `find_table`'s own `bounds_ok` call inside `glx_sfnt_open` -- both are therefore
    //     already small, blob-bounded `size_t`s, nowhere near `SIZE_MAX`.
    // PT: Seguro de computar direto (sem precisar da guarda-contra-overflow que os chamadores do
    //     `bounds_ok` fazem pra `off + need`): `kern_off`/`kern_len` já foram provados
    //     `kern_off + kern_len <= len` pelo próprio `bounds_ok` do `find_table` dentro do
    //     `glx_sfnt_open` -- ambos já são, portanto, `size_t`s pequenos, limitados pelo blob,
    //     longe de `SIZE_MAX`.
    size_t kern_end = kern_off + face->kern_len;

    // ---- kern table header: version(2) nTables(2) -------------------------------------------
    // EN: The Microsoft "extended" kern header is a DIFFERENT shape entirely (a `uint32` version
    //     whose top 16 bits read `0x0001`, followed by a `uint32` table count) -- reading its
    //     first 2 bytes as this classic header's `version` field yields exactly `0x0001`, which
    //     this check rejects, fail-clean, without ever misinterpreting one more byte of an
    //     unsupported header shape as if it were the classic one (see this function's own doc
    //     comment in include/core/sfnt.h for the full MS-variant rationale).
    // PT: O header `kern` "estendido" da Microsoft é um formato inteiramente DIFERENTE (uma
    //     `version` `uint32` cujos 16 bits de cima leem `0x0001`, seguida de uma contagem de
    //     tabela `uint32`) -- ler os primeiros 2 bytes dela como o campo `version` deste header
    //     clássico resulta exatamente em `0x0001`, que esta checagem rejeita, de forma limpa,
    //     sem nunca reinterpretar mais nenhum byte de um formato de header não suportado como se
    //     fosse o clássico (ver o comentário de doc próprio desta função no include/core/sfnt.h
    //     pro racional completo da variante MS).
    uint16_t kern_version;
    if (!rd_u16(blob, len, kern_off, &kern_version)) return 0;
    if (kern_version != 0) return 0;

    uint16_t num_subtables;
    if (!rd_u16(blob, len, kern_off + 2, &num_subtables)) return 0;

    // EN: `cursor` walks the classic kern header's subtable array, one subtable at a time --
    //     bounded by `num_subtables` (a `uint16_t`, at most 65535 iterations) AND by `kern_end`
    //     (checked after every skip below), so this loop is provably finite even against a
    //     hostile `num_subtables`/chain of skipped subtables.
    // PT: `cursor` percorre o array de subtable do header clássico de kern, uma subtable de cada
    //     vez -- limitado por `num_subtables` (um `uint16_t`, no máximo 65535 iterações) E por
    //     `kern_end` (checado depois de todo pulo abaixo), então este laço é provadamente finito
    //     mesmo contra um `num_subtables`/cadeia de subtable pulada hostil.
    size_t cursor = kern_off + 4;
    for (uint16_t i = 0; i < num_subtables; i++) {
        // ---- subtable header: version(2) length(2) coverage(2) ------------------------------
        if (!bounds_ok(kern_end, cursor, 6)) return 0;
        uint16_t sub_length, coverage;
        if (!rd_u16(blob, len, cursor + 2, &sub_length)) return 0;
        if (!rd_u16(blob, len, cursor + 4, &coverage)) return 0;

        // EN: Apple's own kern `coverage` field layout: bits 0-7 are flags (bit0 = horizontal),
        //     bits 8-15 are the subtable FORMAT -- `coverage >> 8` recovers it directly.
        // PT: O layout do próprio campo `coverage` de kern da Apple: bits 0-7 são flags (bit0 =
        //     horizontal), bits 8-15 são o FORMATO da subtable -- `coverage >> 8` recupera ele
        //     direto.
        uint16_t format = (uint16_t)(coverage >> 8);
        int horizontal = coverage & 0x0001;

        if (format == 0 && horizontal) {
            // ---- format 0: nPairs(2) searchRange(2) entrySelector(2) rangeShift(2), pairs... --
            size_t f0_off = cursor + 6;
            uint16_t n_pairs;
            if (!rd_u16(blob, len, f0_off, &n_pairs)) return 0;
            size_t pairs_off = f0_off + 8;

            // EN: Bounds-check the pair array against the OUTER `kern` TABLE's own
            //     `[kern_off, kern_end)` span -- deliberately NOT against this subtable's own
            //     `sub_length` field (read above, otherwise unused from here on): Open Sans' own
            //     real format-0 subtable has 18694 pairs (`18694*6 + 14 == 112178` total bytes)
            //     but its `length` FIELD -- a `uint16_t`, 16 bits, max representable value 65535
            //     -- declares `46642` (`112178 mod 65536 == 46642`, confirmed byte-for-byte
            //     against the live font file). Bounding against `sub_length` here would REJECT
            //     Open Sans' own legitimate kern data outright; bounding against `kern_end`
            //     instead (the outer table's own length, already proven to fit `blob` by
            //     `find_table`'s `bounds_ok` in `glx_sfnt_open`) accepts it correctly while still
            //     catching a genuinely LYING `n_pairs` (any count whose `*6` byte span does not
            //     fit before `kern_end`) with the exact same `bounds_ok` primitive every other
            //     bounds check in this file uses -- `bounds_ok(kern_end, pairs_off, need)` reads
            //     naturally here because `bounds_ok`'s own two parameters are just "exclusive
            //     upper bound" and "offset/size to fit within it", and `kern_end` IS exactly that
            //     upper bound for everything inside this `kern` table.
            // PT: Checa limite do array de par contra o vão `[kern_off, kern_end)` da PRÓPRIA
            //     tabela `kern` externa -- deliberadamente NÃO contra o PRÓPRIO campo
            //     `sub_length` desta subtable (lido acima, sem outro uso daqui em diante): a
            //     própria subtable formato-0 real da Open Sans tem 18694 pares
            //     (`18694*6 + 14 == 112178` bytes totais) mas o CAMPO `length` dela -- um
            //     `uint16_t`, 16 bits, valor máximo representável 65535 -- declara `46642`
            //     (`112178 mod 65536 == 46642`, confirmado byte-a-byte contra o arquivo de fonte
            //     ao vivo). Checar limite contra `sub_length` aqui REJEITARIA de vez o dado de
            //     kern legítimo da própria Open Sans; checar contra `kern_end` em vez disso (o
            //     próprio tamanho da tabela externa, já provado caber em `blob` pelo
            //     `bounds_ok` do `find_table` no `glx_sfnt_open`) aceita corretamente enquanto
            //     ainda pega um `n_pairs` genuinamente MENTIROSO (qualquer contagem cujo vão de
            //     byte `*6` não caiba antes de `kern_end`) com a mesma primitiva `bounds_ok`
            //     exata que toda outra checagem de limite deste arquivo usa --
            //     `bounds_ok(kern_end, pairs_off, need)` lê-se naturalmente aqui porque os
            //     próprios dois parâmetros do `bounds_ok` são só "limite superior exclusivo" e
            //     "offset/tamanho pra caber dentro dele", e `kern_end` É exatamente esse limite
            //     superior pra tudo dentro desta tabela `kern`.
            size_t need = (size_t)n_pairs * 6u;
            if (!bounds_ok(kern_end, pairs_off, need)) return 0;

            // EN: FT-F4-KERN-BSEARCH -- pairs are spec-REQUIRED to be sorted by
            //     `(left<<16 | right)` (confirmed live against Open Sans' own 18694 pairs,
            //     cross-checked independently with `fontTools`); this lookup now ASSUMES that
            //     ordering and bisects `[0, n_pairs)` instead of scanning it linearly -- the
            //     linear scan's own former no-sortedness-assumption stance (kept, unswapped, in
            //     this file's git history and as `kern_linear_ref` in tests/test_sfnt.c, its
            //     independent oracle witness) made a real, measured difference: a linear scan
            //     over Open Sans' 18694-pair table, called once per glyph pair every frame, cost
            //     165x FreeType's own layout time. The correctness-first posture is NOT
            //     abandoned, only re-grounded on what is actually load-bearing: the
            //     `bounds_ok(kern_end, pairs_off, need)` check just above ALREADY proves every
            //     index `m` in `[0, n_pairs)` has its whole 6-byte pair entry inside
            //     `[kern_off, kern_end)` -- and therefore inside `[0, len)`, since `kern_end`
            //     was itself proven `<= len` by `find_table`'s own `bounds_ok` in
            //     `glx_sfnt_open` -- REGARDLESS of the byte values at those offsets, sorted or
            //     not. A hostile/malformed table that violates the sort invariant can only ever
            //     make bisection miss a pair that IS present (this function then returns `0`,
            //     the ordinary "no kerning" value -- cosmetic, not a correctness break: this
            //     function's whole contract is "the kern ADJUSTMENT for this pair, or 0", never
            //     "whether this pair exists"). It can never read outside the pair array (the
            //     bound above is unconditional, independent of the search strategy walking over
            //     it) and it can never return a WRONG value for a pair that exists AND is found
            //     (a match only fires on the exact equality `have == want` below, byte-identical
            //     to the linear scan's own `l == left && r == right`). One residual, benign edge
            //     case: the spec also forbids DUPLICATE `(left, right)` keys, which a hostile
            //     table could still contain -- bisection over duplicates may return a DIFFERENT
            //     matching entry than the linear scan's "first in file order" (both are
            //     "a value this table declares for this pair", so still not a wrong-but-real
            //     value; this can only ever differ from the old behaviour on already-malformed,
            //     spec-violating input, never on any well-formed font, Open Sans included, whose
            //     18694 pairs the exhaustive equivalence test in tests/test_sfnt.c proves are
            //     each returned identically to before). Bounded by `n_pairs` (`uint16_t`, at
            //     most 65535 pairs) either way, and now `O(log n_pairs)` instead of `O(n_pairs)`
            //     -- at most 17 comparisons for Open Sans' 18694, not 18694.
            // PT: FT-F4-KERN-BSEARCH -- pares são EXIGIDOS pela spec de estarem ordenados por
            //     `(left<<16 | right)` (confirmado ao vivo contra os próprios 18694 pares da
            //     Open Sans, cruzado independentemente com `fontTools`); esta busca agora ASSUME
            //     essa ordenação e bisecta `[0, n_pairs)` em vez de varrer linearmente -- a
            //     própria postura anterior de sem-suposição-de-ordenação da varredura linear
            //     (mantida, sem swap, no histórico do git deste arquivo e como
            //     `kern_linear_ref` no tests/test_sfnt.c, sua testemunha-oráculo independente)
            //     fez uma diferença real, medida: uma varredura linear sobre a tabela de 18694
            //     pares da Open Sans, chamada uma vez por par de glyph a cada frame, custava
            //     165x o tempo de layout do próprio FreeType. A postura correção-primeiro NÃO é
            //     abandonada, só reancorada no que de fato sustenta a garantia: a checagem
            //     `bounds_ok(kern_end, pairs_off, need)` logo acima JÁ prova que todo índice `m`
            //     em `[0, n_pairs)` tem sua entrada de par inteira de 6 bytes dentro de
            //     `[kern_off, kern_end)` -- e portanto dentro de `[0, len)`, já que `kern_end`
            //     em si foi provado `<= len` pelo próprio `bounds_ok` do `find_table` no
            //     `glx_sfnt_open` -- INDEPENDENTE dos valores de byte nesses offsets, ordenados
            //     ou não. Uma tabela hostil/malformada que viole o invariante de ordenação só
            //     pode, no máximo, fazer a bisecção perder um par que ESTÁ presente (esta função
            //     então retorna `0`, o valor comum de "sem kerning" -- cosmético, não uma
            //     quebra de corretude: o contrato inteiro desta função é "o AJUSTE de kern
            //     deste par, ou 0", nunca "se este par existe"). Ela nunca consegue ler fora do
            //     array de par (o limite acima é incondicional, independente da estratégia de
            //     busca que percorre por cima dele) e nunca consegue retornar um valor ERRADO
            //     pra um par que existe E é encontrado (um match só dispara na igualdade exata
            //     `have == want` abaixo, idêntica byte-a-byte ao `l == left && r == right` da
            //     própria varredura linear). Um caso-limite residual, benigno: a spec também
            //     proíbe chaves `(left, right)` DUPLICADAS, que uma tabela hostil ainda poderia
            //     conter -- bisecção sobre duplicatas pode retornar uma entrada casada
            //     DIFERENTE da "primeira na ordem do arquivo" da varredura linear (ambas são "um
            //     valor que esta tabela declara pra este par", então ainda não é um valor
            //     errado-mas-real; isso só pode diferir do comportamento antigo em input já
            //     malformado/violador-de-spec, nunca em fonte bem-formada nenhuma, Open Sans
            //     incluída, cujos 18694 pares o teste exaustivo de equivalência no
            //     tests/test_sfnt.c prova serem retornados identicamente a antes). Limitado por
            //     `n_pairs` (`uint16_t`, no máximo 65535 pares) de qualquer jeito, e agora
            //     `O(log n_pairs)` em vez de `O(n_pairs)` -- no máximo 17 comparações pros 18694
            //     da Open Sans, não 18694.
            {
                uint32_t want = ((uint32_t)(uint16_t)left_gid << 16) | (uint16_t)right_gid;
                size_t lo = 0, hi = n_pairs;
                while (lo < hi) {
                    size_t m = lo + (hi - lo) / 2; // anti-overflow idiom, never `(lo + hi) / 2`
                    size_t po = pairs_off + (size_t)m * 6u;
                    uint16_t l, r;
                    if (!rd_u16(blob, len, po, &l)) return 0;
                    if (!rd_u16(blob, len, po + 2, &r)) return 0;
                    uint32_t have = ((uint32_t)l << 16) | r;
                    if (have == want) {
                        int16_t v;
                        if (!rd_i16(blob, len, po + 4, &v)) return 0;
                        return v;
                    }
                    if (have < want) {
                        lo = m + 1;
                    } else {
                        hi = m;
                    }
                }
            }
            return 0; // no matching pair in this subtable -- ordinary "these glyphs do not kern"
        }

        // EN: Not the subtable we want (wrong format, or vertical-only) -- skip to the next one
        //     using ITS OWN declared `sub_length` (the standard way to walk a `kern` subtable
        //     array per spec). This is the ONE place this function still trusts a subtable's own
        //     `length` field -- safe here specifically because we are not reading pairs FROM this
        //     subtable, only skipping PAST it; a `sub_length` of `0` would stall `cursor` forever
        //     (rejected explicitly), and `cursor` overshooting `kern_end` after the skip is
        //     caught immediately below, both fail-clean.
        // PT: Não é a subtable que queremos (formato errado, ou só-vertical) -- pula pra próxima
        //     usando o PRÓPRIO `sub_length` declarado dela (o jeito padrão de percorrer um array
        //     de subtable `kern` conforme a spec). Este é o ÚNICO lugar onde esta função ainda
        //     confia no PRÓPRIO campo `length` de uma subtable -- seguro aqui especificamente
        //     porque não estamos lendo pares DESTA subtable, só pulando POR CIMA dela; um
        //     `sub_length` de `0` travaria o `cursor` pra sempre (rejeitado explicitamente), e o
        //     `cursor` ultrapassando `kern_end` depois do pulo é pego logo abaixo, ambos de forma
        //     limpa.
        if (sub_length == 0) return 0;
        cursor += sub_length;
        if (cursor > kern_end) return 0;
    }

    return 0; // exhausted every subtable without finding a usable format-0 horizontal one
}

// ============================================================================================
// EN: glx_sfnt_colr_layers / glx_sfnt_cpal_rgba -- 'COLR' v0 / 'CPAL' v0 lookup (A4-EMOJI). Public
//     API contract, the `0xFFFF`-sentinel/caller-handled-foreground-colour design, and every
//     hostile-case shape are all documented on each function's own declaration in
//     include/core/sfnt.h -- this comment block only adds the LOCAL "how" (byte layout, the exact
//     bounds-check shape chosen and why), not re-derived "why"/scope that already lives there.
// PT: glx_sfnt_colr_layers / glx_sfnt_cpal_rgba -- lookup de 'COLR' v0 / 'CPAL' v0 (A4-EMOJI). O
//     contrato de API pública, o desenho do sentinela `0xFFFF`/cor-de-foreground-tratada-por-
//     quem-chama, e todo formato de caso hostil estão documentados na declaração própria de cada
//     função no include/core/sfnt.h -- este bloco de comentário só acrescenta o "como" LOCAL
//     (layout de byte, a forma exata de checagem de limite escolhida e por quê), não o
//     "porquê"/escopo já re-derivado que mora lá.
// ============================================================================================

glx_sfnt_result glx_sfnt_colr_layers(const glx_sfnt_face* face, uint32_t gid,
                                      glx_sfnt_colr_layer* out, uint16_t cap,
                                      uint16_t* count_out) {
    if (face == NULL || count_out == NULL) return GLX_SFNT_ERR_INVALID_ARG;
    if (cap != 0 && out == NULL) return GLX_SFNT_ERR_INVALID_ARG;
    if (gid >= face->num_glyphs) return GLX_SFNT_ERR_INVALID_ARG;

    // EN: Default answer, set BEFORE any of the soft-fail early returns below -- every one of
    //     them (no COLR table, bad version, truncated header/arrays, gid not a base glyph,
    //     hostile layer range) is documented to report "0 layers, GLX_SFNT_OK", not leave
    //     `*count_out` stale/indeterminate.
    // PT: Resposta padrão, ajustada ANTES de qualquer um dos retornos antecipados de falha-suave
    //     abaixo -- cada um deles (sem tabela COLR, versão ruim, header/arrays truncados, gid não
    //     é glyph-base, faixa de layer hostil) é documentado pra reportar "0 camadas,
    //     GLX_SFNT_OK", não deixar `*count_out` obsoleto/indeterminado.
    *count_out = 0;

    if (face->colr_len == 0) return GLX_SFNT_OK; // no COLR table -- a valid, common case

    const uint8_t* blob = face->blob;
    size_t len = face->len;
    size_t colr_off = face->colr_off;
    size_t colr_end = colr_off + face->colr_len;

    // ---- COLR header (v0): version(2) numBaseGlyphRecords(2) baseGlyphRecordsOffset(4)
    //      layerRecordsOffset(4) numLayerRecords(2) = 14 bytes -- header fields read against the
    //      WHOLE blob (`len`), same "outer struct header" shape `kern`'s own version/nTables read
    //      uses; only the two RECORD ARRAYS below are bound against `colr_end` (the table's own
    //      span) -----------------------------------------------------------------------------
    uint16_t version;
    if (!rd_u16(blob, len, colr_off, &version)) return GLX_SFNT_OK; // truncated -> soft "no COLR"
    if (version != 0) return GLX_SFNT_OK; // COLRv1+ (paint DAG) out of scope -- soft, not an error

    uint16_t num_base_glyph_records, num_layer_records;
    uint32_t base_glyph_records_offset, layer_records_offset;
    if (!rd_u16(blob, len, colr_off + 2, &num_base_glyph_records)) return GLX_SFNT_OK;
    if (!rd_u32(blob, len, colr_off + 4, &base_glyph_records_offset)) return GLX_SFNT_OK;
    if (!rd_u32(blob, len, colr_off + 8, &layer_records_offset)) return GLX_SFNT_OK;
    if (!rd_u16(blob, len, colr_off + 12, &num_layer_records)) return GLX_SFNT_OK;

    // EN: Both offsets are COLR-table-relative per spec -- `colr_off + offsetN` cannot wrap on a
    //     64-bit `size_t` from any `uint32_t` value in practice (see cmap's own `sub_abs <
    //     cmap_t.off` comment above for the identical, mostly-defensive-documentation reasoning),
    //     kept anyway for the same "invariant enforced here, not merely trusted" discipline.
    // PT: Os dois offsets são relativos-à-tabela-COLR pela spec -- `colr_off + offsetN` não
    //     consegue dar a volta num `size_t` de 64 bits a partir de nenhum valor `uint32_t` na
    //     prática (ver o comentário próprio `sub_abs < cmap_t.off` do cmap acima pro raciocínio
    //     idêntico, majoritariamente documentação-defensiva), mantido assim mesmo pela mesma
    //     disciplina de "invariante reforçado aqui, não meramente confiado".
    size_t base_recs_off = colr_off + (size_t)base_glyph_records_offset;
    if (base_recs_off < colr_off) return GLX_SFNT_OK; // overflow guard, unreachable on 64-bit
    size_t base_recs_need = (size_t)num_base_glyph_records * 6u; // BaseGlyphRecord = 6 bytes
    if (!bounds_ok(colr_end, base_recs_off, base_recs_need)) return GLX_SFNT_OK; // "counts mentirosos"

    size_t layer_recs_off = colr_off + (size_t)layer_records_offset;
    if (layer_recs_off < colr_off) return GLX_SFNT_OK; // overflow guard, unreachable on 64-bit
    size_t layer_recs_need = (size_t)num_layer_records * 4u; // LayerRecord = 4 bytes
    if (!bounds_ok(colr_end, layer_recs_off, layer_recs_need)) return GLX_SFNT_OK; // "COLR truncado"

    // ---- FT-COLR-BSEARCH -- BaseGlyphRecords are spec-REQUIRED sorted ascending by glyphID,
    //      same KERN-BSEARCH shape glx_sfnt_kern already uses: bisect, never scan. The two
    //      bounds_ok calls above already prove every index `m` in `[0, num_base_glyph_records)`
    //      has its whole 6-byte record inside `[colr_off, colr_end)` REGARDLESS of the byte
    //      values there, sorted or not -- a hostile/unsorted table can only ever make this MISS a
    //      gid that IS present (falls through to the "not a colour glyph" `GLX_SFNT_OK`/count==0
    //      return below), never read out of bounds or return a WRONG match. -------------------
    uint16_t want = (uint16_t)gid; // gid < num_glyphs <= 0xFFFF already proven above
    size_t lo = 0, hi = num_base_glyph_records;
    size_t found_idx = SIZE_MAX; // sentinel: "not found yet" (a real index is always < hi <= 0xFFFF)
    while (lo < hi) {
        size_t m = lo + (hi - lo) / 2; // anti-overflow idiom, never `(lo + hi) / 2`
        size_t rec_off = base_recs_off + m * 6u;
        uint16_t have;
        if (!rd_u16(blob, len, rec_off, &have)) return GLX_SFNT_OK; // unreachable, bounds already proven
        if (have == want) {
            found_idx = m;
            break;
        }
        if (have < want) {
            lo = m + 1;
        } else {
            hi = m;
        }
    }
    if (found_idx == SIZE_MAX) return GLX_SFNT_OK; // gid is not a COLR base glyph -- ordinary case

    size_t rec_off = base_recs_off + found_idx * 6u;
    uint16_t first_layer_index, num_layers;
    if (!rd_u16(blob, len, rec_off + 2, &first_layer_index)) return GLX_SFNT_OK; // unreachable, bounds proven
    if (!rd_u16(blob, len, rec_off + 4, &num_layers)) return GLX_SFNT_OK; // unreachable, bounds proven

    // EN: "índice de layer fora de faixa" hostile shape: a BaseGlyphRecord whose own
    //     `firstLayerIndex + numLayers` reaches past the LayerRecords array this SAME COLR table
    //     declares. `uint32_t` arithmetic is overflow-safe here (both operands are `uint16_t`,
    //     max sum ~0x1FFFE, nowhere near UINT32_MAX). `numLayers == 0` is folded into the same
    //     "not usable" answer -- a spec-legitimate base glyph record always has >= 1 layer, but a
    //     hostile one claiming 0 is not memory-unsafe, just a degenerate glyph this function
    //     reports as "not a colour glyph" rather than an empty-but-technically-COLR one.
    // PT: Formato hostil "índice de layer fora de faixa": um BaseGlyphRecord cujo próprio
    //     `firstLayerIndex + numLayers` alcança além do array LayerRecords que esta MESMA tabela
    //     COLR declara. Aritmética `uint32_t` é segura-contra-overflow aqui (os dois operandos
    //     são `uint16_t`, soma máxima ~0x1FFFE, longe de UINT32_MAX). `numLayers == 0` é dobrado
    //     na mesma resposta "não utilizável" -- um registro de glyph-base espec-legítimo sempre
    //     tem >= 1 camada, mas um hostil alegando 0 não é insegurança-de-memória, só um glyph
    //     degenerado que esta função reporta como "não é glyph colorido" em vez de um
    //     tecnicamente-COLR-mas-vazio.
    if (num_layers == 0) return GLX_SFNT_OK;
    if ((uint32_t)first_layer_index + (uint32_t)num_layers > (uint32_t)num_layer_records) {
        return GLX_SFNT_OK;
    }

    // EN: Found, and the layer RANGE is internally consistent -- report the true count BEFORE
    //     the capacity check (see this function's own doc comment in include/core/sfnt.h for why
    //     this differs from glx_sfnt_glyph_outline's BUFFER_TOO_SMALL contract).
    // PT: Achado, e a FAIXA de layer é internamente consistente -- reporta a contagem verdadeira
    //     ANTES da checagem de capacidade (ver o comentário de doc próprio desta função no
    //     include/core/sfnt.h pro porquê disso diferir do contrato BUFFER_TOO_SMALL do
    //     glx_sfnt_glyph_outline).
    *count_out = num_layers;
    if (num_layers > cap) return GLX_SFNT_ERR_BUFFER_TOO_SMALL;

    for (uint16_t i = 0; i < num_layers; i++) {
        size_t lrec_off = layer_recs_off + (size_t)(first_layer_index + i) * 4u;
        uint16_t layer_gid, palette_index;
        // EN: Unreachable in practice -- `layer_recs_off .. + layer_recs_need` was already
        //     proven to fit `[colr_off, colr_end)` above, and `first_layer_index + i <
        //     num_layer_records` by the range check just above -- kept as a real, checked read
        //     (not an assert) anyway, this file's own house style throughout (defense in depth
        //     over trusting an invariant proven a few lines up).
        // PT: Inalcançável na prática -- `layer_recs_off .. + layer_recs_need` já foi provado
        //     caber em `[colr_off, colr_end)` acima, e `first_layer_index + i < num_layer_records`
        //     pela checagem de faixa logo acima -- mantido como uma leitura real, checada (não um
        //     assert) assim mesmo, o próprio estilo de casa deste arquivo do início ao fim (defesa
        //     em profundidade sobre confiar num invariante provado poucas linhas acima).
        if (!rd_u16(blob, len, lrec_off, &layer_gid)) return GLX_SFNT_ERR_TRUNCATED;
        if (!rd_u16(blob, len, lrec_off + 2, &palette_index)) return GLX_SFNT_ERR_TRUNCATED;
        out[i].gid = layer_gid;
        out[i].palette_index = palette_index;
    }
    return GLX_SFNT_OK;
}

glx_sfnt_result glx_sfnt_cpal_rgba(const glx_sfnt_face* face, uint16_t entry_index,
                                    uint8_t rgba_out[4]) {
    if (face == NULL || rgba_out == NULL) return GLX_SFNT_ERR_INVALID_ARG;
    // EN: Unlike glx_sfnt_colr_layers' soft "0 layers" contract, a caller only ever reaches this
    //     function after already confirming a real COLR layer exists -- CPAL's own absence at
    //     that point is a real, reportable failure (see this function's own doc in
    //     include/core/sfnt.h for the full rationale).
    // PT: Diferente do contrato suave "0 camadas" do glx_sfnt_colr_layers, quem chama só chega
    //     nesta função depois de já ter confirmado que uma camada COLR real existe -- a própria
    //     ausência do CPAL nesse ponto é uma falha real, reportável (ver o doc próprio desta
    //     função no include/core/sfnt.h pro racional completo).
    if (face->cpal_len == 0) return GLX_SFNT_ERR_TABLE_MISSING;

    const uint8_t* blob = face->blob;
    size_t len = face->len;
    size_t cpal_off = face->cpal_off;
    size_t cpal_end = cpal_off + face->cpal_len;

    // ---- CPAL header (v0): version(2) numPaletteEntries(2) numPalettes(2) numColorRecords(2)
    //      colorRecordsArrayOffset(4) = 12 bytes -- read against the WHOLE blob (`len`), same
    //      "outer struct header" shape COLR's own header / kern's version/nTables read use ------
    uint16_t version;
    if (!rd_u16(blob, len, cpal_off, &version)) return GLX_SFNT_ERR_TRUNCATED;
    if (version != 0) return GLX_SFNT_ERR_UNSUPPORTED; // CPALv1+ (label tables) out of scope

    uint16_t num_palette_entries, num_palettes, num_color_records;
    uint32_t color_records_array_offset;
    if (!rd_u16(blob, len, cpal_off + 2, &num_palette_entries)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_u16(blob, len, cpal_off + 4, &num_palettes)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_u16(blob, len, cpal_off + 6, &num_color_records)) return GLX_SFNT_ERR_TRUNCATED;
    if (!rd_u32(blob, len, cpal_off + 8, &color_records_array_offset)) return GLX_SFNT_ERR_TRUNCATED;

    // EN: `numPalettes == 0` is internally inconsistent (there is no palette 0 to read
    //     `colorRecordIndices[0]` from, even though CPAL's own fixed 12-byte header parsed fine)
    //     -- same GLX_SFNT_ERR_BAD_OFFSET class `loca[gid] > loca[gid+1]` already uses for
    //     "bytes exist, relationship does not make sense".
    // PT: `numPalettes == 0` é internamente inconsistente (não há paleta 0 pra ler o próprio
    //     `colorRecordIndices[0]`, mesmo o header fixo de 12 bytes do CPAL tendo parseado bem)
    //     -- mesma classe GLX_SFNT_ERR_BAD_OFFSET que `loca[gid] > loca[gid+1]` já usa pra "bytes
    //     existem, a relação não faz sentido".
    if (num_palettes == 0) return GLX_SFNT_ERR_BAD_OFFSET;
    // EN: Caller-supplied index past the table's own declared bound -- same INVALID_ARG posture
    //     glx_sfnt_hmetrics' own `gid >= num_glyphs` check uses (a programming-error-shaped input,
    //     not a font-content problem).
    // PT: Índice fornecido por quem chama além do limite próprio declarado da tabela -- mesma
    //     postura INVALID_ARG que o próprio `gid >= num_glyphs` do glx_sfnt_hmetrics usa (um
    //     input em formato de erro-de-programação, não um problema de conteúdo-de-fonte).
    if (entry_index >= num_palette_entries) return GLX_SFNT_ERR_INVALID_ARG;

    // ---- colorRecordIndices[0]: palette 0's own starting index into colorRecordsArray -- bound
    //      against `cpal_end` (the TABLE's own span), NOT merely `len` (the whole blob), so a
    //      CPAL directory entry trimmed down to just the 12-byte fixed header cannot silently
    //      read a NEIGHBOURING table's bytes as if they were this array -- the "CPAL truncado"
    //      hostile shape, same `hmtx`/`loca`/`kern` pair array reasoning applied here. Only
    //      palette 0 is ever read (see this file's SCOPE note) -----------------------------------
    size_t indices_off = cpal_off + 12;
    if (!bounds_ok(cpal_end, indices_off, 2)) return GLX_SFNT_ERR_TRUNCATED;
    uint16_t palette0_first_index;
    if (!rd_u16(blob, len, indices_off, &palette0_first_index)) return GLX_SFNT_ERR_TRUNCATED;

    // EN: `rec_index >= num_color_records` catches a lying `numColorRecords` (bytes for the
    //     record MAY still exist physically, per the bounds check further below, but the header
    //     itself declares this index does not belong to any real colour record) -- same
    //     GLX_SFNT_ERR_BAD_OFFSET class as the `numPalettes == 0` check above, "internally
    //     inconsistent, not (necessarily) out-of-blob".
    // PT: `rec_index >= num_color_records` pega um `numColorRecords` mentiroso (os bytes do
    //     registro PODEM ainda existir fisicamente, pela checagem de limite mais abaixo, mas o
    //     próprio header declara que este índice não pertence a registro de cor real nenhum) --
    //     mesma classe GLX_SFNT_ERR_BAD_OFFSET da checagem `numPalettes == 0` acima, "internamente
    //     inconsistente, não (necessariamente) fora do blob".
    uint32_t rec_index = (uint32_t)palette0_first_index + (uint32_t)entry_index;
    if (rec_index >= (uint32_t)num_color_records) return GLX_SFNT_ERR_BAD_OFFSET;

    size_t rec_off = cpal_off + (size_t)color_records_array_offset + (size_t)rec_index * 4u;
    if (rec_off < cpal_off) return GLX_SFNT_ERR_TRUNCATED; // overflow guard, unreachable on 64-bit
    if (!bounds_ok(cpal_end, rec_off, 4)) return GLX_SFNT_ERR_TRUNCATED; // "CPAL truncado"

    // EN: ColorRecord wire order is BGRA (blue, green, red, alpha), per spec -- converted to the
    //     ordinary RGBA order this function's own contract promises (see include/core/sfnt.h).
    // PT: A ordem de fio do ColorRecord é BGRA (azul, verde, vermelho, alfa), pela spec --
    //     convertida pra ordem RGBA comum que o próprio contrato desta função promete (ver
    //     include/core/sfnt.h).
    uint8_t blue, green, red, alpha;
    if (!rd_u8(blob, len, rec_off, &blue)) return GLX_SFNT_ERR_TRUNCATED; // unreachable, bounds proven
    if (!rd_u8(blob, len, rec_off + 1, &green)) return GLX_SFNT_ERR_TRUNCATED; // unreachable, bounds proven
    if (!rd_u8(blob, len, rec_off + 2, &red)) return GLX_SFNT_ERR_TRUNCATED; // unreachable, bounds proven
    if (!rd_u8(blob, len, rec_off + 3, &alpha)) return GLX_SFNT_ERR_TRUNCATED; // unreachable, bounds proven

    rgba_out[0] = red;
    rgba_out[1] = green;
    rgba_out[2] = blue;
    rgba_out[3] = alpha;
    return GLX_SFNT_OK;
}
