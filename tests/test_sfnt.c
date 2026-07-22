// SPDX-License-Identifier: MPL-2.0
// EN: SOV-SFNT (FT-F1) gate program -- TDD RED-then-GREEN suite for src/sfnt.c against a REAL
//     TrueType font (Open Sans, the same file glintfx's showcase demo already ships, duplicated
//     at tests/fixtures/opensans_regular.ttf -- see the Makefile's `$(OBJ)/opensans_ttf.o` rule
//     for HOW it gets linked in without a syscall, `open()` does not exist in this Layer) plus
//     two small, HAND-BUILT synthetic fonts (byte-for-byte constructed in this file, see below)
//     exercising paths Open Sans itself does not: `cmap` format 12, and a self-referencing
//     composite glyph.
//
//     ORACLE METHOD (this ticket's brief requires documenting exactly how the "expected" values
//     were produced -- "só p/ gerar os esperados, nunca código"): every numeric constant compared
//     against below (unitsPerEm, numGlyphs, ascender/descender/lineGap, numHMetrics, the
//     cmap('A'/'i'/'M'/'H'/'á')->gid mappings, hmtx advance/lsb per glyph, H's/`.notdef`'s
//     simple-glyph contour/point/bbox data, `á`'s composite structure -- 2 components, `a`+`acute`,
//     BOTH with a plain xy offset and NO scale -- and, added for `glx_sfnt_kern`'s own suite
//     below, the `kern` format-0 pair values for `'A'+'V'`/`'T'+'.'`/`'V'+'a'`/`'A'+'T'`/`'T'+'A'`,
//     each cross-checked TWICE independently -- once via `fontTools`'s own decoded
//     `kernTables[0].kernTable` dict, once via a from-scratch Python `struct` walk of the raw
//     format-0 pair bytes against the public spec, both agreeing) was independently derived with
//     `fontTools`
//     (https://github.com/fonttools/fonttools, MIT-licensed, a Python OpenType library -- NOT
//     FreeType, unrelated codebase) via `TTFont(...)`'s public table-accessor API, and separately
//     cross-checked by manually walking the RAW BYTES with Python's `struct` module against the
//     public OpenType spec (the exact same field-by-field reading src/sfnt.c itself does) --
//     these two independent readings agreed on every value below. `fontTools`/`struct` were used
//     ONLY to produce these expected numbers; **no code from either was read to write src/sfnt.c**
//     (see include/core/sfnt.h's file header for the clean-room statement this ticket's CLEAN-ROOM
//     constraint requires). The exact derivation commands/scripts are not committed (this file's
//     comments ARE the record of every value used and where it came from); reproducible with
//     `pip install fonttools` and `TTFont("glintfx/demos/showcase/assets/OpenSans-Regular.ttf")`.
//
//     The two SYNTHETIC fonts below (`kSyntheticFontF12`, `kSyntheticFontLocaDescending`,
//     `kSyntheticFontSelfRefComposite`) are constructed BYTE-BY-BYTE in this file's comments from
//     the public spec directly (table directory + head/maxp/hhea/hmtx/loca/glyf/cmap fields laid
//     out by hand) -- their correctness was cross-verified once, offline, by round-tripping
//     through `fontTools.ttLib.TTFont` (which independently confirmed e.g. the format-12 mapping
//     U+1F600 -> glyph `u1F600` = gid 1, and the simple triangle glyph's 3 on-curve points) --
//     again, `fontTools` used only as an independent spec-conformance oracle, never as a source
//     of implementation code.
// PT: Programa-gate do SOV-SFNT (FT-F1) -- suíte TDD VERMELHO-depois-VERDE pro src/sfnt.c contra
//     uma fonte TrueType REAL (Open Sans, o mesmo arquivo que o demo showcase do glintfx já
//     distribui, duplicado em tests/fixtures/opensans_regular.ttf -- ver a regra
//     `$(OBJ)/opensans_ttf.o` do Makefile pro COMO ela é linkada sem uma syscall, `open()` não
//     existe nesta Camada) mais duas fontes sintéticas pequenas, CONSTRUÍDAS À MÃO (byte a byte
//     neste arquivo, ver abaixo) exercitando caminhos que a própria Open Sans não exercita:
//     `cmap` formato 12, e um glyph composto auto-referente.
//
//     MÉTODO DO ORÁCULO (o brief desta tarefa exige documentar exatamente como os valores
//     "esperados" foram produzidos -- "só p/ gerar os esperados, nunca código"): toda constante
//     numérica comparada abaixo (unitsPerEm, numGlyphs, ascender/descender/lineGap, numHMetrics,
//     os mapeamentos cmap('A'/'i'/'M'/'H'/'á')->gid, advance/lsb do hmtx por glyph, dado de
//     contorno/ponto/bbox de glyph simples do H/`.notdef`, estrutura composta do `á` -- 2
//     componentes, `a`+`acute`, AMBOS com offset xy puro e SEM escala) foi derivada
//     independentemente com `fontTools` (https://github.com/fonttools/fonttools, licença MIT, uma
//     biblioteca OpenType Python -- NÃO o FreeType, código-base não-relacionado) via a API pública
//     de acesso a tabela do `TTFont(...)`, e cruzada separadamente andando à mão pelos BYTES
//     CRUS com o módulo `struct` do Python conforme a spec OpenType pública (exatamente a mesma
//     leitura campo-a-campo que o próprio src/sfnt.c faz) -- essas duas leituras independentes
//     concordaram em todo valor abaixo. `fontTools`/`struct` foram usados SÓ pra produzir esses
//     números esperados; **nenhum código de nenhum dos dois foi lido pra escrever o src/sfnt.c**
//     (ver o cabeçalho de arquivo do include/core/sfnt.h pra declaração clean-room que a
//     restrição CLEAN-ROOM desta tarefa exige). Os scripts/comandos exatos de derivação não são
//     commitados (os comentários deste arquivo SÃO o registro de todo valor usado e de onde
//     veio); reproduzível com `pip install fonttools` e
//     `TTFont("glintfx/demos/showcase/assets/OpenSans-Regular.ttf")`.
//
//     As duas fontes SINTÉTICAS abaixo (`kSyntheticFontF12`, `kSyntheticFontLocaDescending`,
//     `kSyntheticFontSelfRefComposite`) são construídas BYTE A BYTE nos comentários deste arquivo
//     direto da spec pública (diretório de tabela + campos
//     head/maxp/hhea/hmtx/loca/glyf/cmap dispostos à mão) -- a corretude delas foi cruzada uma
//     vez, offline, fazendo o caminho de volta pelo `fontTools.ttLib.TTFont` (que confirmou
//     independentemente, ex., o mapeamento formato-12 U+1F600 -> glyph `u1F600` = gid 1, e os 3
//     pontos on-curve do glyph triângulo simples) -- de novo, `fontTools` usado só como um
//     oráculo independente de conformidade-com-a-spec, nunca como fonte de código de
//     implementação.
// Copyright (c) 2026 Petrus Silva Costa
#include "test.h"
#include "core/sfnt.h"
#include "mem.h"

// EN: The Open Sans fixture, embedded at LINK TIME (see the Makefile's `$(OBJ)/opensans_ttf.o`
//     rule) rather than read from disk -- Layer 0 has no `open`/`openat` syscall wrapper, and
//     `glx_sfnt_open` is a pure blob-in parser by design (see include/core/sfnt.h). `ld -r -b
//     binary` names the symbols after the exact file argument it was given
//     (`opensans_regular.ttf`, run from inside tests/fixtures/ so no directory component leaks
//     into the symbol name) -- `_start`/`_end` bound the raw bytes, `blob`/`len` below are the
//     ordinary pointer+length view `glx_sfnt_open` expects.
// PT: A fixture Open Sans, embutida em TEMPO DE LINK (ver a regra `$(OBJ)/opensans_ttf.o` do
//     Makefile) em vez de lida do disco -- a Camada 0 não tem wrapper de syscall
//     `open`/`openat`, e o `glx_sfnt_open` é um parser puro blob-entra por desenho (ver
//     include/core/sfnt.h). O `ld -r -b binary` nomeia os símbolos conforme o argumento exato de
//     arquivo que recebeu (`opensans_regular.ttf`, rodado de dentro de tests/fixtures/ pra
//     nenhum componente de diretório vazar pro nome de símbolo) -- `_start`/`_end` demarcam os
//     bytes crus, `blob`/`len` abaixo são a visão ponteiro+tamanho comum que o `glx_sfnt_open`
//     espera.
extern const unsigned char _binary_opensans_regular_ttf_start[];
extern const unsigned char _binary_opensans_regular_ttf_end[];

// ================================================================================================
// EN: Synthetic font A -- `cmap` format 12 coverage. Open Sans' own `cmap` is format-4-only (BMP)
//     -- format 12 (full Unicode, segmented coverage) needs its own fixture. Hand-built per the
//     public OpenType spec: sfntVersion 0x00010000, 7 tables (cmap/glyf/head/hhea/hmtx/loca/maxp),
//     unitsPerEm=1000, numGlyphs=2 (`.notdef` empty + one simple triangle glyph), a SINGLE cmap
//     format-12 group mapping U+1F600 (a non-BMP codepoint -- format 4 could never represent it,
//     which is exactly why this fixture exists) to glyph id 1. Cross-verified once offline via
//     `fontTools.ttLib.TTFont` (see this file's header) before being transcribed here.
// PT: Fonte sintética A -- cobertura de `cmap` formato 12. O próprio `cmap` da Open Sans é
//     só-formato-4 (BMP) -- o formato 12 (Unicode completo, cobertura segmentada) precisa da
//     própria fixture. Construída à mão conforme a spec OpenType pública: sfntVersion
//     0x00010000, 7 tabelas (cmap/glyf/head/hhea/hmtx/loca/maxp), unitsPerEm=1000, numGlyphs=2
//     (`.notdef` vazio + um glyph triângulo simples), UM ÚNICO grupo cmap formato-12 mapeando
//     U+1F600 (um codepoint não-BMP -- o formato 4 nunca conseguiria representar, exatamente por
//     isso esta fixture existe) pro id de glyph 1. Cruzada uma vez offline via
//     `fontTools.ttLib.TTFont` (ver o cabeçalho deste arquivo) antes de ser transcrita aqui.
static const unsigned char kSyntheticFontF12[324] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x28, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x14, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x24, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xf6, 0x00, 0x00, 0x01, 0xf6, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x02,
    0x00, 0x00, 0x31, 0x33, 0x27, 0x64, 0x32, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x03, 0x20, 0xff, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// EN: HOSTILE variant of the font above: byte offset 286 (`loca` table's own byte offset 284 +
//     2, i.e. `loca[1]`) is patched from raw `0x0000` to raw `0x000f` (15) -- since
//     `indexToLocFormat == 0` (short), `loca[1]`'s real byte offset becomes `15*2 = 30`, which is
//     now GREATER than `loca[2]`'s real byte offset (`10*2 = 20`, unchanged) -- i.e.
//     `loca[gid] > loca[gid+1]` for `gid == 1`, the EXACT descending-`loca` hostile shape this
//     ticket's brief names explicitly ("`loca` com offset descendente"). `glx_sfnt_open` still
//     SUCCEEDS on this blob (the size-consistency checks it does are all still satisfied) --
//     the inconsistency is only caught, cleanly, when `glx_sfnt_glyph_outline(gid=1)` actually
//     tries to use this specific pair (see the hostile-input section below).
// PT: Variante HOSTIL da fonte acima: o byte de offset 286 (o próprio offset de byte 284 da
//     tabela `loca` + 2, ou seja `loca[1]`) é remendado do raw `0x0000` pro raw `0x000f` (15) --
//     como `indexToLocFormat == 0` (short), o offset de byte real de `loca[1]` vira
//     `15*2 = 30`, agora MAIOR que o offset de byte real de `loca[2]` (`10*2 = 20`, inalterado)
//     -- ou seja, `loca[gid] > loca[gid+1]` pra `gid == 1`, o formato EXATO de `loca` descendente
//     que o brief desta tarefa nomeia explicitamente ("`loca` com offset descendente"). O
//     `glx_sfnt_open` ainda TEM SUCESSO neste blob (as checagens de consistência de tamanho que
//     ele faz continuam todas satisfeitas) -- a inconsistência só é pega, de forma limpa, quando
//     o `glx_sfnt_glyph_outline(gid=1)` de fato tenta usar esse par específico (ver a seção de
//     input hostil abaixo).
static const unsigned char kSyntheticFontLocaDescending[324] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x28, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x14, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x24, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xf6, 0x00, 0x00, 0x01, 0xf6, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x02,
    0x00, 0x00, 0x31, 0x33, 0x27, 0x64, 0x32, 0x64, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x03, 0x20, 0xff, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// EN: Synthetic font B -- a SINGLE glyph (gid 0) whose own outline is a composite component
//     REFERENCING ITSELF (`glyphIndex == 0`, the composite record's own gid), no
//     `MORE_COMPONENTS` flag. This is the exact hostile shape this ticket's brief calls out
//     ("evite loop infinito de componente auto-referente") -- `glx_sfnt_glyph_outline` on this
//     font must terminate cleanly with `GLX_SFNT_ERR_TOO_DEEP` (bounded by
//     `GLX_SFNT_MAX_COMPOSITE_DEPTH` recursive calls), never hang or crash the stack.
// PT: Fonte sintética B -- um ÚNICO glyph (gid 0) cujo próprio outline é um componente composto
//     que REFERENCIA A SI MESMO (`glyphIndex == 0`, o próprio gid do registro composto), sem
//     flag `MORE_COMPONENTS`. Este é o formato hostil exato que o brief desta tarefa chama
//     ("evite loop infinito de componente auto-referente") -- o `glx_sfnt_glyph_outline` nesta
//     fonte precisa terminar de forma limpa com `GLX_SFNT_ERR_TOO_DEEP` (limitado por
//     `GLX_SFNT_MAX_COMPOSITE_DEPTH` chamadas recursivas), nunca travar ou estourar a pilha.
static const unsigned char kSyntheticFontSelfRefComposite[276] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x10, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x04, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x04, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5,
    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// EN: Synthetic font C -- review-adversarial regression fixture (SOV-SFNT review, CRITICAL fix):
//     2 glyphs. gid 0 is a SIMPLE glyph, one on-curve point at `(20000, 0)` -- a value well
//     within `int16_t` range (`int16_t` is the wire type `xCoordinates` is defined in terms of,
//     see `parse_simple_glyph`), i.e. an ORDINARY, non-hostile point. gid 1 is a COMPOSITE glyph
//     with a SINGLE component: `ARG_1_AND_2_ARE_WORDS|ARGS_ARE_XY_VALUES` (a plain xy translate,
//     no scale -- the identity-2x2 shape Open Sans' own `á`/`ç`/`ã` composites use, see the
//     `kSyntheticFontF12`/real-font composite tests above), referencing gid 0, `dx=20000, dy=0`.
//     `20000 + 20000 = 40000` -- OUTSIDE `int16_t` range (`[-32768, 32767]`) -- this is the EXACT
//     reviewer repro from the SOV-SFNT review: `apply_component_transform`'s old
//     `(int16_t)(fx +/- 0.5f)` cast, with NO range check, was undefined behaviour (C 6.3.1.4)
//     here, silently producing `-25536` while still returning `GLX_SFNT_OK`. NEITHER glyph is a
//     malformed/hostile INPUT by itself -- `20000` fits `int16_t` fine as a raw point, `dx=20000`
//     fits `int16_t` fine as a raw composite argument -- the out-of-range value only appears as
//     the ARITHMETIC RESULT of a spec-legitimate transform, which is exactly why this needed a
//     saturating cast fix (see `round_and_saturate_i16` in src/sfnt.c) rather than being catchable
//     as a `GLX_SFNT_ERR_BAD_GLYPH` input-validation failure anywhere upstream.
// PT: Fonte sintética C -- fixture de regressão do review adversarial (review do SOV-SFNT, fix do
//     CRÍTICO): 2 glyphs. gid 0 é um glyph SIMPLES, um ponto on-curve em `(20000, 0)` -- um valor
//     bem dentro da faixa `int16_t` (`int16_t` é o tipo de fio em que `xCoordinates` é definido,
//     ver `parse_simple_glyph`), ou seja, um ponto ORDINÁRIO, não-hostil. gid 1 é um glyph
//     COMPOSTO com UM ÚNICO componente: `ARG_1_AND_2_ARE_WORDS|ARGS_ARE_XY_VALUES` (uma translação
//     xy pura, sem escala -- a forma 2x2 identidade que os próprios compostos `á`/`ç`/`ã` da Open
//     Sans usam, ver os testes de composto da `kSyntheticFontF12`/fonte real acima), referenciando
//     o gid 0, `dx=20000, dy=0`. `20000 + 20000 = 40000` -- FORA da faixa `int16_t`
//     (`[-32768, 32767]`) -- este é o repro EXATO do revisor do review do SOV-SFNT: o cast antigo
//     `(int16_t)(fx +/- 0.5f)` do `apply_component_transform`, SEM checagem de faixa, era
//     comportamento indefinido (C 6.3.1.4) aqui, produzindo silenciosamente `-25536` enquanto
//     ainda retornava `GLX_SFNT_OK`. NENHUM dos dois glyphs é um INPUT malformado/hostil por si só
//     -- `20000` cabe em `int16_t` numa boa como ponto cru, `dx=20000` cabe em `int16_t` numa boa
//     como argumento composto cru -- o valor fora de faixa só aparece como RESULTADO ARITMÉTICO de
//     uma transformação espec-legítima, exatamente por isso precisou de um fix de cast saturante
//     (ver `round_and_saturate_i16` no src/sfnt.c) em vez de ser pegável como uma falha de
//     validação-de-input `GLX_SFNT_ERR_BAD_GLYPH` em algum lugar upstream.
static const unsigned char kSyntheticFontCompositeSaturate[300] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x24, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x06, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x4e, 0x20, 0x00, 0x00, 0x4e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x4e,
    0x20, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x4e, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x64,
    0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x12, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// EN: Synthetic font D -- review-adversarial regression fixture (SOV-SFNT dominoed re-review,
//     2nd IMPORTANT fix, sibling of the CRITICAL fix `kSyntheticFontCompositeSaturate` above
//     regression-tests): 1 glyph (gid 0), a SIMPLE glyph, 1 contour, 2 points, BOTH points
//     encoded with flag `0x21` (`ON_CURVE_POINT | Y_IS_SAME_OR_POSITIVE`, neither
//     `X_SHORT_VECTOR` nor `Y_SHORT_VECTOR` set) -- per spec this means each point's `y` delta is
//     `0` (no bytes consumed) and each point's `x` delta is a FULL signed `int16` read from the
//     stream. Both x-deltas here are `+32767` (`0x7FFF`, itself an ORDINARY, perfectly in-range
//     `int16_t` value -- this is not a truncated/malformed delta): `parse_simple_glyph`'s running
//     accumulator (`x_acc`, an `int32_t`) is `0 -> 32767 -> 65534` -- `32767` fits `int16_t` fine
//     (point 0, unremarkable), `65534` does NOT (`INT16_MAX == 32767`) -- this is the EXACT
//     reviewer repro from the dominoed re-review: the un-clamped `(int16_t)x_acc` cast at point 1
//     is well-defined-but-corrupting IMPLEMENTATION-DEFINED narrowing (C 6.3.1.3p3, NOT undefined
//     behaviour -- this is why `make sanitize-sfnt`/ASan/UBSan do NOT catch this class, only a
//     VALUE assertion does), silently two's-complement-wrapping `65534` down to `-2` while
//     `glx_sfnt_glyph_outline` still returns `GLX_SFNT_OK`. NEITHER the individual delta bytes
//     nor the point count are a malformed/hostile INPUT by themselves -- `+32767` is a perfectly
//     ordinary single delta, two of them in a row is unremarkable wire data -- the out-of-range
//     value only appears as the ARITHMETIC RESULT of accumulating otherwise-valid deltas, which
//     is exactly why this needed a saturating-cast fix (see `saturate_i32_to_i16` in src/sfnt.c)
//     rather than being catchable as a `GLX_SFNT_ERR_BAD_GLYPH` input-validation failure anywhere
//     upstream -- same shape, same fix philosophy, as the CRITICAL composite-transform bug this
//     sibling fixture regression-tests.
//
//     ORACLE CROSS-CHECK (per this file's header's "só p/ gerar os esperados, nunca código"):
//     built byte-by-byte from the public OpenType `glyf` simple-glyph spec (see the field-by-field
//     layout comments in `parse_simple_glyph`, `src/sfnt.c`), then independently round-tripped
//     through `fontTools.ttLib.TTFont` / `fontTools.ttLib.tables._g_l_y_f` (which decodes deltas
//     into an unbounded Python int, no int16 narrowing at all) -- it reports this glyph's TWO
//     points as coordinates `(32767, 0)` and `(65534, 0)`, confirming `65534` is the mathematically
//     correct pre-narrowing sum the spec's delta-encoding demands, i.e. confirming the SATURATED
//     `32767` (not the wrapped `-2`) is the correct post-fix `int16_t` value for point 1.
// PT: Fonte sintética D -- fixture de regressão do review adversarial (varredura-dominó do
//     re-review do SOV-SFNT, 2º fix do IMPORTANTE, irmão da fixture do fix do CRÍTICO
//     `kSyntheticFontCompositeSaturate` acima que ela regride): 1 glyph (gid 0), um glyph SIMPLES,
//     1 contorno, 2 pontos, AMBOS os pontos codificados com flag `0x21`
//     (`ON_CURVE_POINT | Y_IS_SAME_OR_POSITIVE`, nem `X_SHORT_VECTOR` nem `Y_SHORT_VECTOR`
//     marcados) -- pela spec isso significa que o delta `y` de cada ponto é `0` (nenhum byte
//     consumido) e o delta `x` de cada ponto é um `int16` com sinal COMPLETO lido do stream. Os
//     dois deltas-x aqui são `+32767` (`0x7FFF`, em si um valor `int16_t` ORDINÁRIO, perfeitamente
//     dentro de faixa -- isto não é um delta truncado/malformado): o acumulador corrido do
//     `parse_simple_glyph` (`x_acc`, um `int32_t`) é `0 -> 32767 -> 65534` -- `32767` cabe em
//     `int16_t` numa boa (ponto 0, comum), `65534` NÃO cabe (`INT16_MAX == 32767`) -- este é o
//     repro EXATO do revisor da varredura-dominó do re-review: o cast `(int16_t)x_acc` sem clamp
//     no ponto 1 é estreitamento definido-pela-implementação bem-definido-mas-corruptor (C
//     6.3.1.3p3, NÃO comportamento indefinido -- por isso o `make sanitize-sfnt`/ASan/UBSan NÃO
//     pegam essa classe, só uma asserção de VALOR pega), dando wrap dois-complemento silencioso de
//     `65534` pra `-2` enquanto o `glx_sfnt_glyph_outline` ainda retorna `GLX_SFNT_OK`. NEM os
//     bytes de delta individuais NEM a contagem de pontos são um INPUT malformado/hostil por si só
//     -- `+32767` é um delta único perfeitamente comum, dois deles em sequência é dado de fio sem
//     nada de notável -- o valor fora de faixa só aparece como RESULTADO ARITMÉTICO de acumular
//     deltas de outra forma válidos, exatamente por isso precisou de um fix de cast saturante (ver
//     `saturate_i32_to_i16` no src/sfnt.c) em vez de ser pegável como uma falha de
//     validação-de-input `GLX_SFNT_ERR_BAD_GLYPH` em algum lugar upstream -- mesma forma, mesma
//     filosofia de fix, do bug CRÍTICO de transformação-composta que esta fixture irmã regride.
//
//     CRUZAMENTO DE ORÁCULO (conforme o "só p/ gerar os esperados, nunca código" do cabeçalho
//     deste arquivo): construída byte a byte a partir da spec pública OpenType de glyph-simples
//     `glyf` (ver os comentários de layout campo-a-campo do `parse_simple_glyph`, `src/sfnt.c`),
//     depois cruzada independentemente fazendo o caminho de volta pelo `fontTools.ttLib.TTFont` /
//     `fontTools.ttLib.tables._g_l_y_f` (que decodifica deltas num int Python sem limite, nenhum
//     estreitamento int16 nenhum) -- ele reporta os DOIS pontos deste glyph como coordenadas
//     `(32767, 0)` e `(65534, 0)`, confirmando que `65534` é a soma pré-estreitamento
//     matematicamente correta que a codificação-por-delta da spec exige, ou seja, confirmando que
//     o `32767` SATURADO (não o `-2` com wrap) é o valor `int16_t` pós-fix correto pro ponto 1.
static const unsigned char kSyntheticFontSimpleDeltaOverflow[284] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x24, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x14, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x04, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x12, 0x00, 0x00, 0x00, 0x04, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x21, 0x21,
    0x7f, 0xff, 0x7f, 0xff, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5f, 0x0f, 0x3c, 0xf5, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x20,
    0xff, 0x38, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0xe8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x50, 0x00, 0x00, 0x01
};

// ================================================================================================
// EN: Synthetic font E -- A4-EMOJI's own `COLR` v0 + `CPAL` v0 fixture. Hand-built per the public
//     OpenType spec (COLR/CPAL clause), same technique as `kSyntheticFontF12` above: sfntVersion
//     0x00010000, 9 tables (COLR/CPAL/cmap/glyf/head/hhea/hmtx/loca/maxp), unitsPerEm=1000,
//     numGlyphs=5 -- `.notdef` (gid0, empty), a triangle (gid1, simple glyph, 3 on-curve points --
//     REUSED verbatim as a `COLR` layer shape, `COLR` layers are spec-defined to be ordinary
//     `glyf` outlines, no new raster feature needed), a square (gid2, simple glyph, 4 on-curve
//     points, also reused as a layer shape), and TWO colour base glyphs with no outline of their
//     own (`glyf` empty, same "not directly rendered" shape as `.notdef` -- real colour fonts
//     commonly leave the base glyph blank since `COLR` layers are what actually paints): gid3
//     ("two solid layers") = triangle tinted palette entry 0 (opaque red) UNDER square tinted
//     palette entry 1 (opaque blue); gid4 ("foreground sentinel") = triangle tinted palette index
//     `0xFFFF`, the spec sentinel for "use the text/foreground colour, not a palette entry" (see
//     `glx_sfnt_colr_layer`'s own doc, include/core/sfnt.h) -- exercising the "um glifo usando
//     paleta 0xFFFF" fixture requirement this ticket's brief names explicitly.
//
//     ORACLE CROSS-CHECK (per this file's header's "só p/ gerar os esperados, nunca código"):
//     assembled byte-by-byte with a throwaway Python `struct`-based writer (not committed, same
//     "the exact derivation commands/scripts are not committed" posture this file's own header
//     already states for the OTHER synthetic fonts above), then independently round-tripped
//     through `fontTools.ttLib.TTFont` (MIT-licensed, unrelated to FreeType -- see this file's
//     header for the clean-room statement) before being transcribed here -- `fontTools` reports
//     back EXACTLY the fixture as designed: `glyph00001`/`glyph00002` (gid1/gid2) with coordinates
//     `[(0,0),(100,0),(50,100)]`/`[(0,0),(100,0),(100,100),(0,100)]`; `COLR.ColorLayers['glyph00003']
//     == [('glyph00001', 0), ('glyph00002', 1)]`; `COLR.ColorLayers['glyph00004'] ==
//     [('glyph00001', 65535)]`; `CPAL.palettes[0] == [RGBA(255,0,0,255), RGBA(0,0,255,255)]` --
//     confirming both the `glyf`/`loca` layout AND every `COLR`/`CPAL` field below independently,
//     the same two-reading-agreement bar this file's header sets for every other expected value.
// PT: Fonte sintética E -- a própria fixture `COLR` v0 + `CPAL` v0 do A4-EMOJI. Construída à mão
//     conforme a spec OpenType pública (cláusula COLR/CPAL), mesma técnica do `kSyntheticFontF12`
//     acima: sfntVersion 0x00010000, 9 tabelas (COLR/CPAL/cmap/glyf/head/hhea/hmtx/loca/maxp),
//     unitsPerEm=1000, numGlyphs=5 -- `.notdef` (gid0, vazio), um triângulo (gid1, glyph simples,
//     3 pontos on-curve -- REUSADO ao pé da letra como forma de camada `COLR`, camadas `COLR` são,
//     pela spec, outlines `glyf` comuns, nenhuma feature de raster nova necessária), um quadrado
//     (gid2, glyph simples, 4 pontos on-curve, também reusado como forma de camada), e DOIS
//     glyphs-base coloridos sem outline próprio (`glyf` vazio, mesmo formato "não renderizado
//     diretamente" do `.notdef` -- fontes coloridas reais comumente deixam o glyph-base em branco
//     já que quem de fato pinta são as camadas `COLR`): gid3 ("duas camadas sólidas") = triângulo
//     tingido com a entrada de paleta 0 (vermelho opaco) SOB o quadrado tingido com a entrada de
//     paleta 1 (azul opaco); gid4 ("sentinela de foreground") = triângulo tingido com o índice de
//     paleta `0xFFFF`, o sentinela da spec pra "usa a cor de texto/foreground, não uma entrada de
//     paleta" (ver o doc próprio do `glx_sfnt_colr_layer`, include/core/sfnt.h) -- exercitando o
//     requisito de fixture "um glifo usando paleta 0xFFFF" que o brief desta tarefa nomeia
//     explicitamente.
//
//     CRUZAMENTO DE ORÁCULO (conforme o "só p/ gerar os esperados, nunca código" do cabeçalho
//     deste arquivo): montada byte a byte com um escritor Python baseado em `struct`, descartável
//     (não commitado, mesma postura "os scripts/comandos exatos de derivação não são commitados"
//     que o cabeçalho deste arquivo já declara pras OUTRAS fontes sintéticas acima), depois
//     cruzada independentemente fazendo o caminho de volta pelo `fontTools.ttLib.TTFont` (licença
//     MIT, não-relacionado ao FreeType -- ver o cabeçalho deste arquivo pra declaração clean-room)
//     antes de ser transcrita aqui -- o `fontTools` reporta de volta EXATAMENTE a fixture como
//     desenhada: `glyph00001`/`glyph00002` (gid1/gid2) com coordenadas
//     `[(0,0),(100,0),(50,100)]`/`[(0,0),(100,0),(100,100),(0,100)]`;
//     `COLR.ColorLayers['glyph00003'] == [('glyph00001', 0), ('glyph00002', 1)]`;
//     `COLR.ColorLayers['glyph00004'] == [('glyph00001', 65535)]`;
//     `CPAL.palettes[0] == [RGBA(255,0,0,255), RGBA(0,0,255,255)]` -- confirmando tanto o layout
//     `glyf`/`loca` QUANTO todo campo `COLR`/`CPAL` abaixo independentemente, a mesma barra de
//     concordância-de-duas-leituras que o cabeçalho deste arquivo estabelece pra todo outro valor
//     esperado.
static const unsigned char kSyntheticFontColr[452] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x80, 0x00, 0x03, 0x00, 0x10, 0x43, 0x4f, 0x4c, 0x52,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x26, 0x43, 0x50, 0x41, 0x4c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x16, 0x63, 0x6d, 0x61, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x24, 0x67, 0x6c, 0x79, 0x66,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x68, 0x65, 0x61, 0x64,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x36, 0x68, 0x68, 0x65, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x78, 0x00, 0x00, 0x00, 0x24, 0x68, 0x6d, 0x74, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x9c, 0x00, 0x00, 0x00, 0x14, 0x6c, 0x6f, 0x63, 0x61,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xb0, 0x00, 0x00, 0x00, 0x0c, 0x6d, 0x61, 0x78, 0x70,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbc, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x04, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0e,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x64, 0xff, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x03, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0xff, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x0f, 0x3c, 0xf5,
    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0xe8, 0xff, 0x38,
    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x03, 0xe8, 0x00, 0x00,
    0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x50, 0x00,
    0x00, 0x05, 0x00, 0x00,
};

// EN: Byte offsets into `kSyntheticFontColr` for the hostile byte-patch tests below (mirrors this
//     file's own `patch_u16_be`/`patch_u32_be` pattern already used on the real Open Sans fixture
//     above) -- located by construction (this fixture's own Python builder script, see the
//     ORACLE CROSS-CHECK comment above), not by manual byte-counting.
// PT: Offsets de byte em `kSyntheticFontColr` pros testes hostis de remendo-de-byte abaixo
//     (espelha o próprio padrão `patch_u16_be`/`patch_u32_be` deste arquivo já usado na fixture
//     Open Sans real acima) -- localizados por construção (o próprio script Python que constrói
//     esta fixture, ver o comentário CRUZAMENTO DE ORÁCULO acima), não por contagem manual de
//     byte.
#define COLR_FIXTURE_DIR_COLR_LEN_OFF 24 // COLR's own table-directory record, `length` field
#define COLR_FIXTURE_DIR_CPAL_LEN_OFF 40 // CPAL's own table-directory record, `length` field
#define COLR_FIXTURE_NUM_BASE_RECORDS_OFF 158 // COLR.numBaseGlyphRecords
#define COLR_FIXTURE_BASE_REC0_GID_OFF 170 // COLR.BaseGlyphRecords[0].glyphID (== 3)
#define COLR_FIXTURE_BASE_REC0_NUMLAYERS_OFF 174 // COLR.BaseGlyphRecords[0].numLayers (== 2)
#define COLR_FIXTURE_BASE_REC1_GID_OFF 176 // COLR.BaseGlyphRecords[1].glyphID (== 4)
#define CPAL_FIXTURE_NUM_COLOR_RECORDS_OFF 202 // CPAL.numColorRecords

// EN: Scratch buffer for the byte-patch hostile tests against the REAL 217360-byte Open Sans
//     fixture (BAD_VERSION, a lying `numGlyphs`, an out-of-blob table offset, a malformed
//     `cmap`) -- static storage duration (.bss), not the stack (217KB would be an unreasonable
//     stack frame). `memcpy`/writes are this codebase's own D1/mem.h primitives, zero libc.
// PT: Buffer de rascunho pros testes hostis de remendo-de-byte contra a fixture Open Sans REAL
//     de 217360 bytes (BAD_VERSION, um `numGlyphs` mentiroso, um offset de tabela fora do blob,
//     um `cmap` malformado) -- duração de armazenamento estática (.bss), não a pilha (217KB
//     seria um frame de pilha desarrazoado). `memcpy`/escritas são as primitivas D1/mem.h
//     próprias deste código-base, zero libc.
#define OPENSANS_LEN 217360
static unsigned char g_patch_buf[OPENSANS_LEN];

// EN: Big-endian field patchers -- mirror src/sfnt.c's own byte order (SFNT is big-endian
//     throughout), used only to corrupt a handful of known-offset fields in the copied fixture
//     for the hostile tests below. Offsets were located once by manually walking Open Sans'
//     real table directory with Python's `struct` module (see this file's header's "ORACLE
//     METHOD") -- `sfntVersion`@0, `maxp.numGlyphs`@412 (`maxp` table found at file offset 408,
//     `numGlyphs` field at its own offset 4), the `loca` table directory record's `offset`
//     field@244 (`loca`'s directory record starts at byte 236 -- the 15th of 19 16-byte
//     records starting at byte 12 -- `offset` is that record's own byte 8), `cmap.numTables`@4278
//     (`cmap` table found at file offset 4276, `numTables` field at its own offset 2).
// PT: Remendadores de campo big-endian -- espelham a própria ordem de byte do src/sfnt.c (SFNT
//     é big-endian do início ao fim), usados só pra corromper um punhado de campos de offset
//     conhecido na fixture copiada pros testes hostis abaixo. Offsets foram localizados uma vez
//     andando manualmente pelo diretório de tabela real da Open Sans com o módulo `struct` do
//     Python (ver o "MÉTODO DO ORÁCULO" do cabeçalho deste arquivo) -- `sfntVersion`@0,
//     `maxp.numGlyphs`@412 (tabela `maxp` achada no offset de arquivo 408, campo `numGlyphs` no
//     próprio offset 4), o campo `offset` do registro de diretório da tabela `loca`@244 (o
//     registro de diretório do `loca` começa no byte 236 -- o 15º dos 19 registros de 16 bytes
//     começando no byte 12 -- `offset` é o próprio byte 8 daquele registro),
//     `cmap.numTables`@4278 (tabela `cmap` achada no offset de arquivo 4276, campo `numTables`
//     no próprio offset 2).
static void patch_u16_be(unsigned char* buf, size_t off, unsigned short v) {
    buf[off] = (unsigned char)(v >> 8);
    buf[off + 1] = (unsigned char)(v & 0xFF);
}
static void patch_u32_be(unsigned char* buf, size_t off, unsigned int v) {
    buf[off] = (unsigned char)(v >> 24);
    buf[off + 1] = (unsigned char)(v >> 16);
    buf[off + 2] = (unsigned char)(v >> 8);
    buf[off + 3] = (unsigned char)(v & 0xFF);
}

// ============================================================================================
// EN: FT-F4-KERN-BSEARCH -- independent oracle witness for `glx_sfnt_kern`'s linear-scan-to-
//     binary-search rewrite. `src/sfnt.c` itself no longer contains a linear scan to compare the
//     new bisection against (it was REPLACED, not kept behind a flag -- see this ticket), so
//     `kern_linear_ref` below reproduces that former scan from scratch, independently, directly
//     against `face->blob`/`face->len` (`rd_u16`/`rd_i16`/`bounds_ok` are `static` inside
//     src/sfnt.c, not exported -- `ref_rd_u16`/`ref_rd_i16`/`ref_bounds_ok` below are a clean-room
//     REBUILD of the identical field-by-field walk, not a call into the file under test). This is
//     the TDD RED-then-GREEN witness this ticket's brief requires: written and exercised BEFORE
//     the production swap (RED: passes trivially, both sides ARE the same linear scan), kept
//     unchanged AFTER it (GREEN: production is now a binary search, this stays the old linear
//     scan, and the exhaustive sweep below proves they agree on every one of Open Sans' 18694
//     real pairs -- the actual regression-proof, not the RED/GREEN framing itself).
//     `locate_kern_pairs_ref` factors out the shared "find the classic kern header's sole
//     format-0 horizontal subtable's pair array" walk so both `kern_linear_ref` and the
//     exhaustive sweep test below iterate the exact same `[pairs_off, pairs_off + n_pairs*6)`
//     span without drifting apart on how they got there.
// PT: FT-F4-KERN-BSEARCH -- testemunha-oráculo independente pra reescrita de
//     varredura-linear-pra-busca-binária do `glx_sfnt_kern`. O próprio `src/sfnt.c` não tem mais
//     uma varredura linear pra comparar a nova bisecção (ela foi SUBSTITUÍDA, não mantida atrás
//     de uma flag -- ver esta tarefa), então o `kern_linear_ref` abaixo reproduz aquela
//     varredura anterior do zero, independentemente, direto contra `face->blob`/`face->len`
//     (`rd_u16`/`rd_i16`/`bounds_ok` são `static` dentro do src/sfnt.c, não exportados --
//     `ref_rd_u16`/`ref_rd_i16`/`ref_bounds_ok` abaixo são uma RECONSTRUÇÃO clean-room do mesmo
//     percurso campo-a-campo, não uma chamada pro arquivo sob teste). Esta é a testemunha
//     VERMELHO-depois-VERDE de TDD que o brief desta tarefa exige: escrita e exercitada ANTES do
//     swap de produção (VERMELHO: passa trivialmente, os dois lados SÃO a mesma varredura
//     linear), mantida inalterada DEPOIS dele (VERDE: a produção agora é uma busca binária, este
//     helper continua a varredura linear antiga, e a varredura exaustiva abaixo prova que
//     concordam em cada um dos 18694 pares reais da Open Sans -- a prova-de-regressão de fato,
//     não a moldura VERMELHO/VERDE em si). `locate_kern_pairs_ref` fatora o percurso
//     compartilhado de "achar o array de par da única subtable formato-0 horizontal do header
//     clássico de kern" pra que tanto o `kern_linear_ref` quanto o teste de varredura exaustiva
//     abaixo iterem exatamente o mesmo vão `[pairs_off, pairs_off + n_pairs*6)` sem divergir em
//     como chegaram lá.
// ============================================================================================
static int ref_bounds_ok(size_t len, size_t off, size_t need) {
    if (off > len) return 0;
    if (need > len - off) return 0;
    return 1;
}

static int ref_rd_u16(const unsigned char* b, size_t len, size_t off, uint16_t* out) {
    if (!ref_bounds_ok(len, off, 2)) return 0;
    *out = (uint16_t)(((uint16_t)b[off] << 8) | (uint16_t)b[off + 1]);
    return 1;
}

static int ref_rd_i16(const unsigned char* b, size_t len, size_t off, int16_t* out) {
    uint16_t u;
    if (!ref_rd_u16(b, len, off, &u)) return 0;
    *out = (int16_t)u;
    return 1;
}

static int locate_kern_pairs_ref(const glx_sfnt_face* face, size_t* pairs_off_out,
                                  uint16_t* n_pairs_out) {
    if (face == NULL || face->kern_len == 0) return 0;
    const unsigned char* blob = face->blob;
    size_t len = face->len;
    size_t kern_off = face->kern_off;
    size_t kern_end = kern_off + face->kern_len;

    uint16_t kern_version;
    if (!ref_rd_u16(blob, len, kern_off, &kern_version)) return 0;
    if (kern_version != 0) return 0;

    uint16_t num_subtables;
    if (!ref_rd_u16(blob, len, kern_off + 2, &num_subtables)) return 0;

    size_t cursor = kern_off + 4;
    for (uint16_t i = 0; i < num_subtables; i++) {
        if (!ref_bounds_ok(kern_end, cursor, 6)) return 0;
        uint16_t sub_length, coverage;
        if (!ref_rd_u16(blob, len, cursor + 2, &sub_length)) return 0;
        if (!ref_rd_u16(blob, len, cursor + 4, &coverage)) return 0;
        uint16_t format = (uint16_t)(coverage >> 8);
        int horizontal = coverage & 0x0001;
        if (format == 0 && horizontal) {
            size_t f0_off = cursor + 6;
            uint16_t n_pairs;
            if (!ref_rd_u16(blob, len, f0_off, &n_pairs)) return 0;
            size_t pairs_off = f0_off + 8;
            size_t need = (size_t)n_pairs * 6u;
            if (!ref_bounds_ok(kern_end, pairs_off, need)) return 0;
            *pairs_off_out = pairs_off;
            *n_pairs_out = n_pairs;
            return 1;
        }
        if (sub_length == 0) return 0;
        cursor += sub_length;
        if (cursor > kern_end) return 0;
    }
    return 0;
}

static int16_t kern_linear_ref(const glx_sfnt_face* face, uint32_t left_gid,
                                uint32_t right_gid) {
    if (left_gid > 0xFFFFu || right_gid > 0xFFFFu) return 0;
    size_t pairs_off;
    uint16_t n_pairs;
    if (!locate_kern_pairs_ref(face, &pairs_off, &n_pairs)) return 0;
    const unsigned char* blob = face->blob;
    size_t len = face->len;
    for (uint16_t p = 0; p < n_pairs; p++) {
        size_t po = pairs_off + (size_t)p * 6u;
        uint16_t l, r;
        if (!ref_rd_u16(blob, len, po, &l)) return 0;
        if (!ref_rd_u16(blob, len, po + 2, &r)) return 0;
        if (l == (uint16_t)left_gid && r == (uint16_t)right_gid) {
            int16_t v;
            if (!ref_rd_i16(blob, len, po + 4, &v)) return 0;
            return v;
        }
    }
    return 0;
}

int main(int argc, char** argv, char** envp) {
    (void)argc;
    (void)argv;
    (void)envp;

    const unsigned char* real_blob = _binary_opensans_regular_ttf_start;
    size_t real_len =
        (size_t)(_binary_opensans_regular_ttf_end - _binary_opensans_regular_ttf_start);
    TEST_ASSERT_EQ(real_len, (size_t)OPENSANS_LEN);

    // ============================================================================================
    // ---- glx_sfnt_open: table directory + head/maxp/hhea metrics (real Open Sans) -------------
    // ============================================================================================
    glx_sfnt_face face;
    {
        glx_sfnt_result r = glx_sfnt_open(real_blob, real_len, &face);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face.units_per_em, (unsigned short)2048);
        TEST_ASSERT_EQ(face.index_to_loc_format, (short)0);
        TEST_ASSERT_EQ(face.num_glyphs, (unsigned short)938);
        TEST_ASSERT_EQ(face.ascender, (short)2189);
        TEST_ASSERT_EQ(face.descender, (short)-600);
        TEST_ASSERT_EQ(face.line_gap, (short)0);
        TEST_ASSERT_EQ(face.num_h_metrics, (unsigned short)931);
        TEST_ASSERT_EQ(face.cmap_format, (unsigned short)4); // Open Sans ships format-4-only
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_id: cmap format 4 lookups (real Open Sans) -----------------------------
    // ============================================================================================
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 'A'), (unsigned int)36);
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 'i'), (unsigned int)76);
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 'M'), (unsigned int)48);
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 'H'), (unsigned int)43);
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 0x00E1u /* 'á' LATIN SMALL LETTER A WITH ACUTE */),
                   (unsigned int)163);
    // EN: `.notdef` fallback: an unmapped codepoint always resolves to gid 0.
    // PT: Fallback pro `.notdef`: um codepoint não-mapeado sempre resolve pro gid 0.
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 0x10FFFFu), (unsigned int)0);
    // EN: Beyond the BMP -- format 4 cannot represent it, must resolve to `.notdef`, not crash.
    // PT: Além do BMP -- o formato 4 não consegue representar, precisa resolver pro `.notdef`,
    //     não travar.
    TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face, 0x1F600u), (unsigned int)0);

    // ============================================================================================
    // ---- glx_sfnt_hmetrics (real Open Sans) ----------------------------------------------------
    // ============================================================================================
    {
        unsigned short adv;
        short lsb;
        TEST_ASSERT_EQ(glx_sfnt_hmetrics(&face, 36 /* A */, &adv, &lsb), GLX_SFNT_OK);
        TEST_ASSERT_EQ(adv, (unsigned short)1296);
        TEST_ASSERT_EQ(lsb, (short)0);

        TEST_ASSERT_EQ(glx_sfnt_hmetrics(&face, 76 /* i */, &adv, &lsb), GLX_SFNT_OK);
        TEST_ASSERT_EQ(adv, (unsigned short)518);
        TEST_ASSERT_EQ(lsb, (short)162);

        TEST_ASSERT_EQ(glx_sfnt_hmetrics(&face, 48 /* M */, &adv, &lsb), GLX_SFNT_OK);
        TEST_ASSERT_EQ(adv, (unsigned short)1849);
        TEST_ASSERT_EQ(lsb, (short)201);

        TEST_ASSERT_EQ(glx_sfnt_hmetrics(&face, 43 /* H */, &adv, &lsb), GLX_SFNT_OK);
        TEST_ASSERT_EQ(adv, (unsigned short)1511);
        TEST_ASSERT_EQ(lsb, (short)201);

        // EN: gid >= num_glyphs -> INVALID_ARG, never an OOB read.
        // PT: gid >= num_glyphs -> INVALID_ARG, nunca uma leitura fora dos limites.
        TEST_ASSERT_EQ(glx_sfnt_hmetrics(&face, 999999, &adv, &lsb), GLX_SFNT_ERR_INVALID_ARG);
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_outline: SIMPLE glyph 'H' (gid 43, real Open Sans) ---------------------
    // ============================================================================================
    {
        glx_sfnt_point points[64];
        unsigned short contour_ends[8];
        glx_sfnt_outline out;
        glx_sfnt_result r =
            glx_sfnt_glyph_outline(&face, 43, points, 64, contour_ends, 8, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_contours, (unsigned short)1);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)12);
        TEST_ASSERT_EQ(contour_ends[0], (unsigned short)11);
        TEST_ASSERT_EQ(out.x_min, (short)201);
        TEST_ASSERT_EQ(out.y_min, (short)0);
        TEST_ASSERT_EQ(out.x_max, (short)1311);
        TEST_ASSERT_EQ(out.y_max, (short)1462);

        // EN: Spot-check the full point stream (oracle: fontTools' own `coordinates`/`flags`
        //     lists for glyph 'H', see this file's header) -- every point is on-curve (straight
        //     rectangle-shaped letterform, no curves).
        // PT: Confere o fluxo de ponto inteiro (oráculo: as próprias listas
        //     `coordinates`/`flags` do fontTools pro glyph 'H', ver o cabeçalho deste arquivo)
        //     -- todo ponto é on-curve (forma de letra retangular reta, sem curvas).
        static const short expected_x[12] = {1311, 1141, 1141, 371, 371, 201,
                                              201,  371,  371,  1141, 1141, 1311};
        static const short expected_y[12] = {0,    0,    688, 688, 0,    0,
                                              1462, 1462, 840, 840, 1462, 1462};
        for (int i = 0; i < 12; i++) {
            TEST_ASSERT_EQ(points[i].x, expected_x[i]);
            TEST_ASSERT_EQ(points[i].y, expected_y[i]);
            TEST_ASSERT_EQ(points[i].on_curve, (unsigned char)1);
        }
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_outline: SIMPLE glyph '.notdef' (gid 0, 2 contours, real Open Sans) ----
    // ============================================================================================
    {
        glx_sfnt_point points[64];
        unsigned short contour_ends[8];
        glx_sfnt_outline out;
        glx_sfnt_result r = glx_sfnt_glyph_outline(&face, 0, points, 64, contour_ends, 8, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_contours, (unsigned short)2);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)8);
        TEST_ASSERT_EQ(contour_ends[0], (unsigned short)3);
        TEST_ASSERT_EQ(contour_ends[1], (unsigned short)7);
        TEST_ASSERT_EQ(points[0].x, (short)193);
        TEST_ASSERT_EQ(points[0].y, (short)1462);
        TEST_ASSERT_EQ(points[0].on_curve, (unsigned char)1);
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_outline: COMPOSITE glyph 'á' (gid 163, real Open Sans) -----------------
    //      This is the pt-br-critical case: 2 components (`a` gid 68, offset (0,0); `acute`
    //      gid 118, offset (43,0)), BOTH plain translate, no scale -- flattened total 37+10=47
    //      points, 2+1=3 contours (oracle: fontTools' `.glyf['aacute'].components` walked
    //      independently against the raw component bytes with `struct`, see this file's header).
    // PT: Este é o caso crítico-pt-br: 2 componentes (`a` gid 68, offset (0,0); `acute` gid 118,
    //     offset (43,0)), AMBOS translação pura, sem escala -- total achatado 37+10=47 pontos,
    //     2+1=3 contornos (oráculo: `.glyf['aacute'].components` do fontTools andado
    //     independentemente contra os bytes crus do componente com `struct`, ver o cabeçalho
    //     deste arquivo).
    // ============================================================================================
    {
        glx_sfnt_point points[64];
        unsigned short contour_ends[8];
        glx_sfnt_outline out;
        glx_sfnt_result r =
            glx_sfnt_glyph_outline(&face, 163, points, 64, contour_ends, 8, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_contours, (unsigned short)3);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)47);
        TEST_ASSERT_EQ(out.x_min, (short)94);
        TEST_ASSERT_EQ(out.y_min, (short)-20);
        TEST_ASSERT_EQ(out.x_max, (short)973);
        TEST_ASSERT_EQ(out.y_max, (short)1569);
        // EN: contour_ends must be 0-based indices into the FLATTENED points array: `a`'s own
        //     endPtsOfContours were [25, 36] (37 points), `acute`'s own were [9] (10 points, at
        //     a +37 point offset once flattened).
        // PT: contour_ends precisa ser índices 0-based no array de pontos ACHATADO: os próprios
        //     endPtsOfContours de `a` eram [25, 36] (37 pontos), o próprio de `acute` era [9]
        //     (10 pontos, num offset de +37 pontos uma vez achatado).
        TEST_ASSERT_EQ(contour_ends[0], (unsigned short)25);
        TEST_ASSERT_EQ(contour_ends[1], (unsigned short)36);
        TEST_ASSERT_EQ(contour_ends[2], (unsigned short)46);
        // EN: `a` component's own point 0 = (850, 0), offset (0,0) -> unchanged.
        // PT: ponto 0 do próprio componente `a` = (850, 0), offset (0,0) -> inalterado.
        TEST_ASSERT_EQ(points[0].x, (short)850);
        TEST_ASSERT_EQ(points[0].y, (short)0);
        TEST_ASSERT_EQ(points[0].on_curve, (unsigned char)1);
        // EN: `acute` component's own point 0 = (393, 1266), offset (43,0) -> x+43.
        // PT: ponto 0 do próprio componente `acute` = (393, 1266), offset (43,0) -> x+43.
        TEST_ASSERT_EQ(points[37].x, (short)436);
        TEST_ASSERT_EQ(points[37].y, (short)1266);
        TEST_ASSERT_EQ(points[37].on_curve, (unsigned char)1);
        // EN: `acute` component's own point 1 = (441, 1328), off-curve, offset (43,0) -> x+43.
        // PT: ponto 1 do próprio componente `acute` = (441, 1328), off-curve, offset (43,0) ->
        //     x+43.
        TEST_ASSERT_EQ(points[38].x, (short)484);
        TEST_ASSERT_EQ(points[38].y, (short)1328);
        TEST_ASSERT_EQ(points[38].on_curve, (unsigned char)0);
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_outline: BUFFER_TOO_SMALL (real Open Sans) -----------------------------
    // ============================================================================================
    {
        glx_sfnt_point points[4];
        unsigned short contour_ends[4];
        glx_sfnt_outline out;
        // EN: 'H' needs 12 points -- a 4-slot buffer must fail cleanly, not overflow.
        // PT: 'H' precisa de 12 pontos -- um buffer de 4 slots precisa falhar de forma limpa,
        //     não estourar.
        glx_sfnt_result r = glx_sfnt_glyph_outline(&face, 43, points, 4, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BUFFER_TOO_SMALL);
        // EN: 'á' needs 47 points across 3 contours -- a 10-point buffer must fail cleanly
        //     mid-composite (the `a` component alone already needs 37).
        // PT: 'á' precisa de 47 pontos em 3 contornos -- um buffer de 10 pontos precisa falhar
        //     de forma limpa no meio do composto (só o componente `a` já precisa de 37).
        glx_sfnt_point points2[10];
        unsigned short contour_ends2[8];
        r = glx_sfnt_glyph_outline(&face, 163, points2, 10, contour_ends2, 8, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BUFFER_TOO_SMALL);
    }

    // ============================================================================================
    // ---- glx_sfnt_glyph_outline: gid out of range -> INVALID_ARG (real Open Sans) --------------
    // ============================================================================================
    {
        glx_sfnt_point points[4];
        unsigned short contour_ends[4];
        glx_sfnt_outline out;
        glx_sfnt_result r =
            glx_sfnt_glyph_outline(&face, 999999, points, 4, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_INVALID_ARG);
    }

    // ============================================================================================
    // ---- HOSTILE: glx_sfnt_open input validation ------------------------------------------------
    // ============================================================================================
    TEST_ASSERT_EQ(glx_sfnt_open(NULL, 100, &face), GLX_SFNT_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(glx_sfnt_open(real_blob, 0, &face), GLX_SFNT_ERR_INVALID_ARG);
    TEST_ASSERT_EQ(glx_sfnt_open(real_blob, real_len, NULL), GLX_SFNT_ERR_INVALID_ARG);

    // EN: HOSTILE: table directory itself truncated (19 real tables need 12+19*16=316 bytes;
    //     100 is not enough to even finish scanning the directory).
    // PT: HOSTIL: o próprio diretório de tabela truncado (19 tabelas reais precisam de
    //     12+19*16=316 bytes; 100 não é suficiente pra sequer terminar de varrer o diretório).
    TEST_ASSERT_EQ(glx_sfnt_open(real_blob, 100, &face), GLX_SFNT_ERR_TRUNCATED);

    // EN: HOSTILE: directory fits (needs 316 bytes) but a required table's own declared
    //     `[offset, length)` (e.g. `maxp` at file offset 408, needing up to byte 440) falls past
    //     a shorter blob.
    // PT: HOSTIL: o diretório cabe (precisa de 316 bytes) mas o próprio `[offset, length)`
    //     declarado de uma tabela obrigatória (ex.: `maxp` no offset de arquivo 408, precisando
    //     até o byte 440) cai além de um blob mais curto.
    TEST_ASSERT_EQ(glx_sfnt_open(real_blob, 400, &face), GLX_SFNT_ERR_TRUNCATED);

    memcpy(g_patch_buf, real_blob, real_len);

    // EN: HOSTILE: `sfntVersion` patched to `'OTTO'` (0x4F54544F) -- a CFF/OTF font, PostScript
    //     cubic outlines, explicitly out of this parser's scope (see include/core/sfnt.h).
    // PT: HOSTIL: `sfntVersion` remendado pra `'OTTO'` (0x4F54544F) -- uma fonte CFF/OTF,
    //     outlines cúbicas PostScript, explicitamente fora do escopo deste parser (ver
    //     include/core/sfnt.h).
    {
        patch_u32_be(g_patch_buf, 0, 0x4F54544Fu);
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BAD_VERSION);
        memcpy(g_patch_buf, real_blob, real_len); // restore for the next hostile case
    }

    // EN: HOSTILE: `maxp.numGlyphs` (file offset 412) patched from 938 to 60000 -- the "lying
    //     numGlyphs" case this ticket's brief names explicitly. Caught precisely because the
    //     `loca`-size cross-check compares against `loca`'s OWN declared table length (1878
    //     bytes, 939 real entries), not merely "does 60001*2 bytes fit somewhere before the
    //     217360-byte file ends" (it would -- see src/sfnt.c's comment on this exact point).
    // PT: HOSTIL: `maxp.numGlyphs` (offset de arquivo 412) remendado de 938 pra 60000 -- o caso
    //     de "numGlyphs mentiroso" que o brief desta tarefa nomeia explicitamente. Pego com
    //     precisão porque a checagem cruzada de tamanho do `loca` compara contra o PRÓPRIO
    //     tamanho de tabela declarado do `loca` (1878 bytes, 939 entradas reais), não meramente
    //     "60001*2 bytes cabem em algum lugar antes do arquivo de 217360 bytes acabar" (caberia
    //     -- ver o comentário do src/sfnt.c sobre exatamente este ponto).
    {
        patch_u16_be(g_patch_buf, 412, 60000);
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_TRUNCATED);
        memcpy(g_patch_buf, real_blob, real_len);
    }

    // EN: HOSTILE: the `loca` table directory RECORD's own `offset` field (that record starts
    //     at byte 236 -- the 15th of 19 16-byte records starting at byte 12 -- its `offset`
    //     field is its own byte 8, i.e. absolute byte 244) patched to a huge, out-of-blob value
    //     -- "offset de tabela além do blob".
    // PT: HOSTIL: o próprio campo `offset` do REGISTRO de diretório da tabela `loca` (aquele
    //     registro começa no byte 236 -- o 15º dos 19 registros de 16 bytes começando no byte
    //     12 -- o campo `offset` dele é o próprio byte 8, ou seja byte absoluto 244) remendado
    //     pra um valor enorme, fora do blob -- "offset de tabela além do blob".
    {
        patch_u32_be(g_patch_buf, 244, 0xFFFFFFF0u);
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_TRUNCATED);
        memcpy(g_patch_buf, real_blob, real_len);
    }

    // EN: HOSTILE: `cmap.numTables` (`cmap` table at file offset 4276, `numTables` field at its
    //     own offset 2, i.e. absolute byte 4278) patched to a huge value -- the encoding record
    //     array it implies (`numTables*8` bytes) does not fit in the blob -- "cmap malformada".
    // PT: HOSTIL: `cmap.numTables` (tabela `cmap` no offset de arquivo 4276, campo `numTables`
    //     no próprio offset 2, ou seja byte absoluto 4278) remendado pra um valor enorme -- o
    //     array de registro de encoding que isso implica (`numTables*8` bytes) não cabe no blob
    //     -- "cmap malformada".
    {
        patch_u16_be(g_patch_buf, 4278, 0xFFFFu);
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_TRUNCATED);
        memcpy(g_patch_buf, real_blob, real_len);
    }

    // ============================================================================================
    // ---- HOSTILE: cmap format 12, synthetic font A (kSyntheticFontF12) -------------------------
    // ============================================================================================
    {
        glx_sfnt_face face_a;
        glx_sfnt_result r =
            glx_sfnt_open(kSyntheticFontF12, sizeof(kSyntheticFontF12), &face_a);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face_a.cmap_format, (unsigned short)12);
        TEST_ASSERT_EQ(face_a.num_glyphs, (unsigned short)2);
        // EN: U+1F600, non-BMP, only representable by format 12 -- must resolve to gid 1.
        // PT: U+1F600, não-BMP, só representável pelo formato 12 -- precisa resolver pro gid 1.
        TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face_a, 0x1F600u), (unsigned int)1);
        // EN: An unmapped codepoint on this font -> `.notdef`.
        // PT: Um codepoint não-mapeado nesta fonte -> `.notdef`.
        TEST_ASSERT_EQ(glx_sfnt_glyph_id(&face_a, 'A'), (unsigned int)0);

        glx_sfnt_point points[8];
        unsigned short contour_ends[4];
        glx_sfnt_outline out;
        r = glx_sfnt_glyph_outline(&face_a, 1, points, 8, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_contours, (unsigned short)1);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)3);
        TEST_ASSERT_EQ(points[0].x, (short)0);
        TEST_ASSERT_EQ(points[0].y, (short)0);
        TEST_ASSERT_EQ(points[1].x, (short)100);
        TEST_ASSERT_EQ(points[1].y, (short)0);
        TEST_ASSERT_EQ(points[2].x, (short)50);
        TEST_ASSERT_EQ(points[2].y, (short)100);

        // EN: .notdef (gid 0) on this font is genuinely empty (0-byte glyf record) -- OK,
        //     num_points == num_contours == 0.
        // PT: .notdef (gid 0) nesta fonte é genuinamente vazio (registro glyf de 0 bytes) -- OK,
        //     num_points == num_contours == 0.
        r = glx_sfnt_glyph_outline(&face_a, 0, points, 8, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)0);
        TEST_ASSERT_EQ(out.num_contours, (unsigned short)0);
    }

    // ============================================================================================
    // ---- HOSTILE: descending `loca` (kSyntheticFontLocaDescending) ------------------------------
    // ============================================================================================
    {
        glx_sfnt_face face_bad;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontLocaDescending,
                                           sizeof(kSyntheticFontLocaDescending), &face_bad);
        // EN: `glx_sfnt_open` itself still succeeds -- the inconsistency is only caught when the
        //     specific `gid` is actually resolved.
        // PT: O próprio `glx_sfnt_open` ainda tem sucesso -- a inconsistência só é pega quando o
        //     `gid` específico é de fato resolvido.
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);

        glx_sfnt_point points[8];
        unsigned short contour_ends[4];
        glx_sfnt_outline out;
        r = glx_sfnt_glyph_outline(&face_bad, 1, points, 8, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BAD_OFFSET);
    }

    // ============================================================================================
    // ---- HOSTILE: self-referencing composite glyph -- must terminate, never hang/crash ---------
    // ============================================================================================
    {
        glx_sfnt_face face_c;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontSelfRefComposite,
                                           sizeof(kSyntheticFontSelfRefComposite), &face_c);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);

        glx_sfnt_point points[8];
        unsigned short contour_ends[4];
        glx_sfnt_outline out;
        r = glx_sfnt_glyph_outline(&face_c, 0, points, 8, contour_ends, 4, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_TOO_DEEP);
    }

    // ============================================================================================
    // ---- REGRESSION: composite transform saturates instead of UB (kSyntheticFontCompositeSaturate,
    //      SOV-SFNT review CRITICAL fix) --------------------------------------------------------
    // ============================================================================================
    {
        glx_sfnt_face face_d;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontCompositeSaturate,
                                           sizeof(kSyntheticFontCompositeSaturate), &face_d);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face_d.num_glyphs, (unsigned short)2);

        // EN: gid 0 alone -- an ORDINARY simple glyph, single on-curve point at (20000, 0), well
        //     within int16 range. Not the interesting case, but confirms the fixture's base glyph
        //     is itself unremarkable before composing it.
        // PT: gid 0 sozinho -- um glyph simples ORDINÁRIO, único ponto on-curve em (20000, 0),
        //     bem dentro da faixa int16. Não é o caso interessante, mas confirma que o glyph base
        //     da fixture é em si mesmo comum antes de compô-lo.
        glx_sfnt_point points[4];
        unsigned short contour_ends[2];
        glx_sfnt_outline out;
        r = glx_sfnt_glyph_outline(&face_d, 0, points, 4, contour_ends, 2, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)1);
        TEST_ASSERT_EQ(points[0].x, (short)20000);
        TEST_ASSERT_EQ(points[0].y, (short)0);

        // EN: gid 1 -- the composite: translates gid 0's point by dx=20000, dy=0. Pre-fix,
        //     `fx = 40000.0f`, `(int16_t)(fx + 0.5f)` was UB and this codebase's own build
        //     (clang, `-fsanitize=undefined` when the sanitizer target below is used) observed it
        //     concretely resolve to `-25536` (`(int32_t)40000` truncated to 16 bits, two's
        //     complement) while still returning `GLX_SFNT_OK` -- silent data corruption, not a
        //     crash, which is exactly why this class of bug is dangerous. Post-fix,
        //     `round_and_saturate_i16` SATURATES: `x == INT16_MAX (32767)`, well-defined, no UB,
        //     `GLX_SFNT_OK` still (this is a valid, if degenerate, glyph -- see src/sfnt.c's
        //     `apply_component_transform` comment for why saturation, not `BAD_GLYPH`, was
        //     chosen). `y` (dy=0, no overflow) stays an ordinary rounded `0`, proving the fix does
        //     NOT clamp values that were already in range.
        // PT: gid 1 -- o composto: translada o ponto do gid 0 por dx=20000, dy=0. Pré-fix,
        //     `fx = 40000.0f`, `(int16_t)(fx + 0.5f)` era UB e o próprio build deste código-base
        //     (clang, `-fsanitize=undefined` quando o alvo sanitizer abaixo é usado) observou
        //     concretamente resolver pra `-25536` (`(int32_t)40000` truncado pra 16 bits,
        //     complemento de dois) enquanto ainda retornava `GLX_SFNT_OK` -- corrupção de dado
        //     silenciosa, não um crash, exatamente por isso essa classe de bug é perigosa.
        //     Pós-fix, `round_and_saturate_i16` SATURA: `x == INT16_MAX (32767)`, bem-definido,
        //     sem UB, `GLX_SFNT_OK` ainda (este é um glyph válido, se degenerado -- ver o
        //     comentário do `apply_component_transform` do src/sfnt.c pro porquê de saturação, e
        //     não `BAD_GLYPH`, ter sido escolhida). `y` (dy=0, sem overflow) fica um `0`
        //     arredondado comum, provando que o fix NÃO satura valores que já estavam em faixa.
        r = glx_sfnt_glyph_outline(&face_d, 1, points, 4, contour_ends, 2, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)1);
        TEST_ASSERT_EQ(points[0].x, (short)32767); // INT16_MAX -- saturated, not UB garbage
        TEST_ASSERT_EQ(points[0].y, (short)0);      // unaffected axis: ordinary round, no clamp
        TEST_ASSERT_EQ(points[0].on_curve, (unsigned char)1);
    }

    // ============================================================================================
    // ---- REGRESSION: simple-glyph delta accumulator saturates instead of silently wrapping
    //      (kSyntheticFontSimpleDeltaOverflow, SOV-SFNT dominoed re-review IMPORTANT fix) --------
    // ============================================================================================
    {
        glx_sfnt_face face_e;
        glx_sfnt_result r = glx_sfnt_open(kSyntheticFontSimpleDeltaOverflow,
                                           sizeof(kSyntheticFontSimpleDeltaOverflow), &face_e);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face_e.num_glyphs, (unsigned short)1);

        // EN: gid 0 -- a simple glyph, 1 contour, 2 points, x-deltas `+32767` then `+32767`.
        //     Point 0's accumulated x (`32767`) is ordinary, in-range -- confirms the fixture's
        //     first delta alone is unremarkable. Point 1's accumulated x (`32767 + 32767 =
        //     65534`) is OUT of `int16_t` range: pre-fix, the un-clamped `(int16_t)x_acc` cast
        //     two's-complement-wrapped this down to `-2` (`fontTools`' independent, unbounded-int
        //     decode of these same delta bytes confirms `65534` is the spec-correct pre-narrowing
        //     sum -- see this fixture's declaration comment above for the full oracle cross-check)
        //     while still returning `GLX_SFNT_OK` -- silent wrong geometry, no crash, and (unlike
        //     the CRITICAL float-cast UB fix) NOT catchable by ASan/UBSan at all, since
        //     `int32_t -> int16_t` narrowing of an out-of-range value is well-defined,
        //     implementation-defined behaviour (C 6.3.1.3p3), not UB -- this test's VALUE
        //     assertion is the ONLY gate that catches this class. Post-fix, `saturate_i32_to_i16`
        //     SATURATES: `x == INT16_MAX (32767)`, not `-2`. `y` (both points' deltas are `0`,
        //     `Y_IS_SAME_OR_POSITIVE`, no accumulation ever leaves range) stays an ordinary `0`,
        //     proving the fix does NOT clamp values that were already in range.
        // PT: gid 0 -- um glyph simples, 1 contorno, 2 pontos, deltas-x `+32767` depois `+32767`.
        //     O x acumulado do ponto 0 (`32767`) é comum, dentro de faixa -- confirma que o
        //     primeiro delta da fixture sozinho não tem nada de notável. O x acumulado do ponto 1
        //     (`32767 + 32767 = 65534`) está FORA da faixa `int16_t`: pré-fix, o cast
        //     `(int16_t)x_acc` sem clamp dava wrap dois-complemento disso pra `-2` (a decodificação
        //     independente, com int sem limite, do `fontTools` sobre esses mesmos bytes de delta
        //     confirma que `65534` é a soma pré-estreitamento correta pela spec -- ver o
        //     comentário de declaração desta fixture acima pro cruzamento de oráculo completo)
        //     enquanto ainda retornava `GLX_SFNT_OK` -- geometria errada silenciosa, sem crash, e
        //     (ao contrário do fix do CRÍTICO de UB de cast float) NÃO pegável por ASan/UBSan de
        //     jeito nenhum, já que estreitamento `int32_t -> int16_t` de um valor fora de faixa é
        //     comportamento bem-definido, definido-pela-implementação (C 6.3.1.3p3), não UB --
        //     a asserção de VALOR deste teste é o ÚNICO gate que pega essa classe. Pós-fix, o
        //     `saturate_i32_to_i16` SATURA: `x == INT16_MAX (32767)`, não `-2`. `y` (o delta dos
        //     dois pontos é `0`, `Y_IS_SAME_OR_POSITIVE`, a acumulação nunca sai de faixa) fica um
        //     `0` comum, provando que o fix NÃO satura valores que já estavam em faixa.
        glx_sfnt_point points[4];
        unsigned short contour_ends[2];
        glx_sfnt_outline out;
        r = glx_sfnt_glyph_outline(&face_e, 0, points, 4, contour_ends, 2, &out);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(out.num_points, (unsigned short)2);
        TEST_ASSERT_EQ(points[0].x, (short)32767); // ordinary, in-range, not saturated
        TEST_ASSERT_EQ(points[0].y, (short)0);
        TEST_ASSERT_EQ(points[1].x, (short)32767); // INT16_MAX -- saturated, NOT the -2 wrap
        TEST_ASSERT_EQ(points[1].y, (short)0);
    }

    // ============================================================================================
    // ---- glx_sfnt_kern: 'kern' format-0 horizontal subtable pair lookup (real Open Sans) --------
    //
    //      ORACLE (per this file's header's "só p/ gerar os esperados, nunca código"): every pair
    //      value below was independently derived TWICE against the live font file, both cross-
    //      checking each other -- (1) `fontTools.ttLib.TTFont(...)['kern'].kernTables[0].kernTable`,
    //      a dict keyed by (glyphName1, glyphName2), and (2) a from-scratch Python `struct` walk of
    //      the RAW format-0 pair array (`left(u16) right(u16) value(i16)`, per the public spec),
    //      resolving glyph names via `TTFont.getGlyphOrder()`/`getBestCmap()` purely to LABEL gids
    //      for readability, not to derive the kern values themselves. Both methods agreed on every
    //      value below. `fontTools` is MIT-licensed, unrelated to FreeType -- used only to generate
    //      these expected numbers, no code read from it (see include/core/sfnt.h's file header for
    //      the clean-room statement this ticket's CLEAN-ROOM constraint requires).
    //
    //      Open Sans' own `kern` table (confirmed by both the real-font `glx_sfnt_open` assertions
    //      above and this independent derivation): ONE subtable, classic `version == 0` header,
    //      format 0, horizontal (`coverage == 0x0001`), 18694 pairs, sorted by `(left<<16|right)`.
    //      Its subtable-local `length` FIELD declares `46642` -- a 16-bit overflow of the true
    //      `112178`-byte subtable size (`112178 mod 65536 == 46642`), confirmed by both oracle
    //      methods (`fontTools` itself warns "'kern' subtable longer than defined" while still
    //      correctly parsing all 18694 pairs) -- this is exactly the real-world quirk `glx_sfnt_
    //      kern`'s own doc comment (include/core/sfnt.h) and implementation comment (src/sfnt.c)
    //      document at length: bounds-checking against this field would wrongly REJECT Open Sans'
    //      own legitimate kern data.
    //
    //      Pairs used below (glyph -> gid, from this file's existing cmap-lookup assertions/
    //      comments above): 'A'=36, 'V'=57, 'T'=55, '.'=17 (period), 'a'=68 (also 'á' composite's
    //      own base component, gid 163 is unrelated -- plain 'a' is a separate, simple glyph).
    // ============================================================================================

    // ============================================================================================
    // ---- FT-F4-KERN-BSEARCH: exhaustive equivalence sweep, ALL 18694 real Open Sans pairs -------
    //      This is the regression-proof the ticket brief requires: for every single pair the real
    //      font declares, read `(l, r, value)` straight off the blob (a THIRD, independent read of
    //      the same bytes, not routed through either lookup) and assert both `glx_sfnt_kern`
    //      (production, now a binary search) AND `kern_linear_ref` (this file's own independent
    //      linear-scan oracle, above) return that exact declared `value` -- proving the binary
    //      search finds every existing pair the linear scan used to find, with the identical
    //      result, across the WHOLE table, not a hand-picked sample.
    // PT: FT-F4-KERN-BSEARCH: varredura exaustiva de equivalência, TODOS os 18694 pares reais da
    //      Open Sans -- esta é a prova-de-regressão que o brief da tarefa exige: pra cada par que
    //      a fonte real declara, lê `(l, r, value)` direto do blob (uma TERCEIRA leitura
    //      independente dos mesmos bytes, não passando por nenhuma das duas buscas) e assere que
    //      tanto o `glx_sfnt_kern` (produção, agora uma busca binária) QUANTO o `kern_linear_ref`
    //      (o oráculo de varredura linear independente deste arquivo, acima) retornam exatamente
    //      aquele `value` declarado -- provando que a busca binária acha todo par existente que a
    //      varredura linear achava, com resultado idêntico, sobre a tabela INTEIRA, não uma
    //      amostra escolhida a dedo.
    // ============================================================================================
    {
        size_t pairs_off;
        uint16_t n_pairs;
        int located = locate_kern_pairs_ref(&face, &pairs_off, &n_pairs);
        TEST_ASSERT_EQ(located, 1);
        TEST_ASSERT_EQ((unsigned int)n_pairs, (unsigned int)18694);
        for (uint16_t p = 0; p < n_pairs; p++) {
            size_t po = pairs_off + (size_t)p * 6u;
            uint16_t l, r;
            int16_t v;
            TEST_ASSERT_EQ(ref_rd_u16(face.blob, face.len, po, &l), 1);
            TEST_ASSERT_EQ(ref_rd_u16(face.blob, face.len, po + 2, &r), 1);
            TEST_ASSERT_EQ(ref_rd_i16(face.blob, face.len, po + 4, &v), 1);
            TEST_ASSERT_EQ(glx_sfnt_kern(&face, l, r), v);
            TEST_ASSERT_EQ(kern_linear_ref(&face, l, r), v);
        }
    }

    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 36 /* A */, 57 /* V */), (short)-82);
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 55 /* T */, 17 /* period */), (short)-123);
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 57 /* V */, 68 /* a */), (short)-41);
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 36 /* A */, 55 /* T */), (short)-143);
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 55 /* T */, 36 /* A */), (short)-143);
    // EN: 'A'+'A' -- confirmed absent from the oracle's own pair dict (`kernTable.get(('A','A'))
    //     is None`) -- the ordinary "these two glyphs do not kern" case, must return 0, not a
    //     crash/garbage value.
    // PT: 'A'+'A' -- confirmado ausente do próprio dicionário de par do oráculo
    //     (`kernTable.get(('A','A')) is None`) -- o caso comum "esses dois glyphs não fazem
    //     kern", precisa retornar 0, não um valor lixo/crash.
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 36 /* A */, 36 /* A */), (short)0);
    // EN: Same miss, asserted against the independent linear-scan oracle too (FT-F4-KERN-BSEARCH)
    //     -- both the binary search and the retired linear scan must agree a genuinely absent
    //     pair resolves to the ordinary "no kerning" 0, not just the pairs that DO exist.
    // PT: O mesmo miss, testado também contra o oráculo de varredura linear independente
    //     (FT-F4-KERN-BSEARCH) -- tanto a busca binária quanto a varredura linear aposentada
    //     precisam concordar que um par genuinamente ausente resolve pro `0` comum de "sem
    //     kerning", não só os pares que EXISTEM.
    TEST_ASSERT_EQ(kern_linear_ref(&face, 36 /* A */, 36 /* A */), (short)0);
    // EN: gid above 0xFFFF -- can never match a wire (uint16) pair field, must return 0 cleanly.
    // PT: gid acima de 0xFFFF -- nunca consegue casar um campo de par no fio (uint16), precisa
    //     retornar 0 de forma limpa.
    TEST_ASSERT_EQ(glx_sfnt_kern(&face, 0x10000u, 57), (short)0);
    TEST_ASSERT_EQ(glx_sfnt_kern(NULL, 36, 57), (short)0);

    // ============================================================================================
    // ---- glx_sfnt_kern: font genuinely WITHOUT a 'kern' table (kSyntheticFontF12) ---------------
    // ============================================================================================
    {
        glx_sfnt_face face_no_kern;
        glx_sfnt_result r =
            glx_sfnt_open(kSyntheticFontF12, sizeof(kSyntheticFontF12), &face_no_kern);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        // EN: 7-table synthetic font, `kern` is not one of them -- `kern_len` must be 0 and every
        //     lookup must resolve to 0, never a stale/garbage read.
        // PT: fonte sintética de 7 tabelas, `kern` não é uma delas -- `kern_len` precisa ser 0 e
        //     toda busca precisa resolver pra 0, nunca uma leitura obsoleta/lixo.
        TEST_ASSERT_EQ(face_no_kern.kern_len, (size_t)0);
        TEST_ASSERT_EQ(glx_sfnt_kern(&face_no_kern, 0, 1), (short)0);
    }

    // ============================================================================================
    // ---- HOSTILE: glx_sfnt_kern -- lying nPairs (real Open Sans, byte-patched) -------------------
    //      The format-0 subtable's own `nPairs` field (at file offset 87370 -- `kern` table found
    //      at file offset 87360 per the Makefile-embedded fixture's directory, `+4` for the kern
    //      header, `+6` for the subtable's own version/length/coverage, i.e. absolute byte
    //      87360+4+6=87370, independently confirmed via Python `struct` against the same file this
    //      whole suite embeds) is patched from the real `18694` to `0xFFFF` (65535) -- a byte span
    //      of `65535*6=393210` bytes that does NOT fit before `kern_end` (this subtable's true
    //      pair-array budget is `112164` bytes). Must resolve to a clean `0`, never an
    //      out-of-bounds read.
    // ============================================================================================
    {
        memcpy(g_patch_buf, real_blob, real_len);
        patch_u16_be(g_patch_buf, 87370, 0xFFFFu);
        glx_sfnt_face face_bad_kern;
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face_bad_kern);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK); // glx_sfnt_open itself still succeeds -- kern is optional
        TEST_ASSERT_EQ(glx_sfnt_kern(&face_bad_kern, 36 /* A */, 57 /* V */), (short)0);
        memcpy(g_patch_buf, real_blob, real_len);
    }

    // ============================================================================================
    // ---- HOSTILE: glx_sfnt_kern -- Microsoft "extended" kern header (real Open Sans, patched) ---
    //      The kern table's own `version` field (the first 2 bytes of the table, absolute file
    //      offset 87360) is patched from `0x0000` to `0x0001` -- the exact first-two-bytes shape
    //      the Microsoft "extended" kern header variant uses (a totally different header this
    //      function's scope deliberately excludes, see include/core/sfnt.h). Must be detected and
    //      skipped cleanly (never misread the rest of the bytes as if they were the classic
    //      shape), resolving to 0.
    // ============================================================================================
    {
        memcpy(g_patch_buf, real_blob, real_len);
        patch_u16_be(g_patch_buf, 87360, 0x0001u);
        glx_sfnt_face face_ms_kern;
        glx_sfnt_result r = glx_sfnt_open(g_patch_buf, real_len, &face_ms_kern);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(glx_sfnt_kern(&face_ms_kern, 36 /* A */, 57 /* V */), (short)0);
        memcpy(g_patch_buf, real_blob, real_len);
    }

    // ============================================================================================
    // ---- A4-EMOJI: glx_sfnt_colr_layers / glx_sfnt_cpal_rgba (kSyntheticFontColr) --------------
    // ============================================================================================
    {
        glx_sfnt_face face_colr;
        glx_sfnt_result r =
            glx_sfnt_open(kSyntheticFontColr, sizeof(kSyntheticFontColr), &face_colr);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face_colr.num_glyphs, (unsigned short)5);
        TEST_ASSERT_EQ(face_colr.colr_len, (size_t)38);
        TEST_ASSERT_EQ(face_colr.cpal_len, (size_t)22);

        // ---- happy path: gid1 (triangle) is NOT a colour glyph -- ordinary, most common case --
        glx_sfnt_colr_layer layers[4];
        unsigned short count;
        r = glx_sfnt_colr_layers(&face_colr, 1, layers, 4, &count);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(count, (unsigned short)0);

        // ---- happy path: gid3 -- 2 solid layers (triangle@palette0=red, square@palette1=blue) --
        count = 0xFFFF; // poison -- must be overwritten by the call below
        r = glx_sfnt_colr_layers(&face_colr, 3, layers, 4, &count);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(count, (unsigned short)2);
        TEST_ASSERT_EQ(layers[0].gid, (unsigned short)1);
        TEST_ASSERT_EQ(layers[0].palette_index, (unsigned short)0);
        TEST_ASSERT_EQ(layers[1].gid, (unsigned short)2);
        TEST_ASSERT_EQ(layers[1].palette_index, (unsigned short)1);

        // ---- happy path: gid4 -- 1 layer, palette index 0xFFFF (foreground-colour sentinel) ----
        r = glx_sfnt_colr_layers(&face_colr, 4, layers, 4, &count);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(count, (unsigned short)1);
        TEST_ASSERT_EQ(layers[0].gid, (unsigned short)1);
        TEST_ASSERT_EQ(layers[0].palette_index, (unsigned short)0xFFFF);

        // ---- BUFFER_TOO_SMALL: count_out is ALWAYS the true count on this result (unlike
        //      glx_sfnt_glyph_outline's own BUFFER_TOO_SMALL contract -- see this function's own
        //      doc comment in include/core/sfnt.h) -----------------------------------------------
        glx_sfnt_colr_layer small_layers[1];
        r = glx_sfnt_colr_layers(&face_colr, 3, small_layers, 1, &count);
        TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BUFFER_TOO_SMALL);
        TEST_ASSERT_EQ(count, (unsigned short)2);

        // ---- HOSTILE: bad args -- NULL face/count_out, non-zero cap paired with NULL out, gid
        //      >= num_glyphs -------------------------------------------------------------------
        TEST_ASSERT_EQ(glx_sfnt_colr_layers(NULL, 3, layers, 4, &count), GLX_SFNT_ERR_INVALID_ARG);
        TEST_ASSERT_EQ(glx_sfnt_colr_layers(&face_colr, 3, layers, 4, NULL),
                       GLX_SFNT_ERR_INVALID_ARG);
        TEST_ASSERT_EQ(glx_sfnt_colr_layers(&face_colr, 3, NULL, 4, &count),
                       GLX_SFNT_ERR_INVALID_ARG);
        TEST_ASSERT_EQ(glx_sfnt_colr_layers(&face_colr, 999999, layers, 4, &count),
                       GLX_SFNT_ERR_INVALID_ARG);

        // ---- happy path: glx_sfnt_cpal_rgba, palette 0's own 2 entries (BGRA wire -> RGBA out) -
        unsigned char rgba[4];
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_colr, 0, rgba), GLX_SFNT_OK);
        TEST_ASSERT_EQ(rgba[0], (unsigned char)255); // R
        TEST_ASSERT_EQ(rgba[1], (unsigned char)0);   // G
        TEST_ASSERT_EQ(rgba[2], (unsigned char)0);   // B
        TEST_ASSERT_EQ(rgba[3], (unsigned char)255); // A -- opaque red
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_colr, 1, rgba), GLX_SFNT_OK);
        TEST_ASSERT_EQ(rgba[0], (unsigned char)0);   // R
        TEST_ASSERT_EQ(rgba[1], (unsigned char)0);   // G
        TEST_ASSERT_EQ(rgba[2], (unsigned char)255); // B
        TEST_ASSERT_EQ(rgba[3], (unsigned char)255); // A -- opaque blue

        // ---- HOSTILE: bad args -- NULL face/rgba_out --------------------------------------------
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(NULL, 0, rgba), GLX_SFNT_ERR_INVALID_ARG);
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_colr, 0, NULL), GLX_SFNT_ERR_INVALID_ARG);

        // ---- HOSTILE: "índice de cor fora de faixa" -- an ordinary out-of-range index AND the
        //      spec 0xFFFF foreground-colour sentinel BOTH resolve to the same clean
        //      GLX_SFNT_ERR_INVALID_ARG (see glx_sfnt_colr_layer's own doc: the CALLER, not this
        //      function, must special-case 0xFFFF before ever reaching here) -------------------
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_colr, 5 /* numPaletteEntries == 2 */, rgba),
                       GLX_SFNT_ERR_INVALID_ARG);
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_colr, 0xFFFF, rgba), GLX_SFNT_ERR_INVALID_ARG);
    }

    // ============================================================================================
    // ---- A4-EMOJI: font genuinely WITHOUT COLR/CPAL at all (kSyntheticFontF12) -----------------
    // ============================================================================================
    {
        glx_sfnt_face face_no_colr;
        glx_sfnt_result r =
            glx_sfnt_open(kSyntheticFontF12, sizeof(kSyntheticFontF12), &face_no_colr);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(face_no_colr.colr_len, (size_t)0);
        TEST_ASSERT_EQ(face_no_colr.cpal_len, (size_t)0);

        glx_sfnt_colr_layer layers[4];
        unsigned short count = 0xFFFF; // poison -- must be reset to 0 by the call below
        r = glx_sfnt_colr_layers(&face_no_colr, 1, layers, 4, &count);
        TEST_ASSERT_EQ(r, GLX_SFNT_OK);
        TEST_ASSERT_EQ(count, (unsigned short)0);

        unsigned char rgba[4];
        TEST_ASSERT_EQ(glx_sfnt_cpal_rgba(&face_no_colr, 0, rgba), GLX_SFNT_ERR_TABLE_MISSING);
    }

    // ============================================================================================
    // ---- HOSTILE: glx_sfnt_colr_layers / glx_sfnt_cpal_rgba byte-patch attacks on
    //      kSyntheticFontColr -- same patch_u16_be/patch_u32_be pattern already used against the
    //      real Open Sans fixture above, this time on a small stack scratch buffer (452 bytes,
    //      no .bss needed unlike OPENSANS_LEN) -------------------------------------------------
    // ============================================================================================
    {
        unsigned char colr_patch_buf[sizeof(kSyntheticFontColr)];
        glx_sfnt_face pf;
        glx_sfnt_colr_layer layers[4];
        unsigned short count;
        unsigned char rgba[4];

        // ---- HOSTILE 1: "COLR truncado" -- COLR's own directory record `length` shrunk from 38
        //      to 14 (just the 14-byte header, no BaseGlyphRecords/LayerRecords array) -- the
        //      physical bytes for both arrays still exist right after (this is the SAME blob,
        //      CPAL/cmap/glyf/... still follow), so this proves the bound is checked against the
        //      TABLE's own declared length, not merely "does it fit somewhere before the blob
        //      ends" (same reasoning hmtx/loca/kern's own pair array already document in
        //      src/sfnt.c) -- must NOT silently read past COLR's own truncated span. ------------
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u32_be(colr_patch_buf, COLR_FIXTURE_DIR_COLR_LEN_OFF, 14);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK); // glx_sfnt_open itself still succeeds -- COLR is optional
            count = 0xFFFF; // poison
            r = glx_sfnt_colr_layers(&pf, 3, layers, 4, &count);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            TEST_ASSERT_EQ(count, (unsigned short)0);
        }

        // ---- HOSTILE 2: "CPAL truncado" -- CPAL's own directory record `length` shrunk from 22
        //      to 12 (just the fixed 12-byte header, no colorRecordIndices/colorRecordsArray) --
        //      same "bound against the table's own span" proof as HOSTILE 1, this time on the
        //      function that returns a real ERROR (not a soft 0) for a missing/truncated CPAL. --
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u32_be(colr_patch_buf, COLR_FIXTURE_DIR_CPAL_LEN_OFF, 12);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            r = glx_sfnt_cpal_rgba(&pf, 0, rgba);
            TEST_ASSERT_EQ(r, GLX_SFNT_ERR_TRUNCATED);
        }

        // ---- HOSTILE 3: "counts mentirosos" -- COLR.numBaseGlyphRecords patched from 2 to
        //      0xFFFF (65535) -- the BaseGlyphRecords array this implies (65535*6 bytes) does not
        //      fit within COLR's own declared span, same class as the real-Open-Sans "lying
        //      numGlyphs" hostile test above. -----------------------------------------------------
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u16_be(colr_patch_buf, COLR_FIXTURE_NUM_BASE_RECORDS_OFF, 0xFFFFu);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            count = 0xFFFF; // poison
            r = glx_sfnt_colr_layers(&pf, 3, layers, 4, &count);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            TEST_ASSERT_EQ(count, (unsigned short)0);
        }

        // ---- HOSTILE 4: "índice de layer fora de faixa" -- gid3's own BaseGlyphRecord.numLayers
        //      patched from 2 to 10, so `firstLayerIndex(0) + numLayers(10) == 10` exceeds
        //      `numLayerRecords == 3` -- must be caught cleanly, never read past the LayerRecords
        //      array. -----------------------------------------------------------------------------
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u16_be(colr_patch_buf, COLR_FIXTURE_BASE_REC0_NUMLAYERS_OFF, 10);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            count = 0xFFFF; // poison
            r = glx_sfnt_colr_layers(&pf, 3, layers, 4, &count);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            TEST_ASSERT_EQ(count, (unsigned short)0);
        }

        // ---- HOSTILE 5: "records desordenados" -- BaseGlyphRecords[0]/[1].glyphID SWAPPED
        //      (3<->4, descending instead of the spec-required ascending) -- FT-COLR-BSEARCH's own
        //      doc comment (include/core/sfnt.h) predicts a hostile-unsorted table can only ever
        //      make the bisection MISS a gid that IS present, never read out of bounds or return a
        //      wrong match: searching for gid=4 here bisects to index 1 (glyphID patched to 3),
        //      `3 < 4` steers right past the record actually holding gid=4 (at index 0) -- a clean
        //      miss, not a crash. --------------------------------------------------------------
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u16_be(colr_patch_buf, COLR_FIXTURE_BASE_REC0_GID_OFF, 4);
            patch_u16_be(colr_patch_buf, COLR_FIXTURE_BASE_REC1_GID_OFF, 3);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            count = 0xFFFF; // poison
            r = glx_sfnt_colr_layers(&pf, 4, layers, 4, &count);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            TEST_ASSERT_EQ(count, (unsigned short)0); // bisection miss -- clean, not a crash
        }

        // ---- HOSTILE 6: "índice de cor fora de faixa" (lying numColorRecords) -- CPAL's own
        //      numColorRecords patched from 2 to 0 -- entry_index 0 now resolves to
        //      `rec_index (0) >= num_color_records (0)`, an internally-inconsistent header (the
        //      colorRecordIndices[0]/colour bytes still physically exist -- this is the
        //      GLX_SFNT_ERR_BAD_OFFSET class, "bytes exist, relationship does not make sense",
        //      not GLX_SFNT_ERR_TRUNCATED). ------------------------------------------------------
        {
            memcpy(colr_patch_buf, kSyntheticFontColr, sizeof(kSyntheticFontColr));
            patch_u16_be(colr_patch_buf, CPAL_FIXTURE_NUM_COLOR_RECORDS_OFF, 0);
            glx_sfnt_result r = glx_sfnt_open(colr_patch_buf, sizeof(colr_patch_buf), &pf);
            TEST_ASSERT_EQ(r, GLX_SFNT_OK);
            r = glx_sfnt_cpal_rgba(&pf, 0, rgba);
            TEST_ASSERT_EQ(r, GLX_SFNT_ERR_BAD_OFFSET);
        }
    }

    TEST_PASS();
    return 0; // unreachable -- TEST_PASS() calls sys_exit(0)
}
