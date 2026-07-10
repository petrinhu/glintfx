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
