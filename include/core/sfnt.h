// SPDX-License-Identifier: MPL-2.0
// EN: SOV-SFNT (FT-F1, AUD-TRANS-2's 2nd piece) -- a clean-room SFNT/TrueType font PARSER, built
//     entirely from the PUBLIC OpenType/TrueType format specification (Microsoft OpenType spec /
//     Apple TrueType Reference Manual -- both freely published descriptions of a binary file
//     format, not any particular implementation's source code). **No FreeType code was read to
//     write this file** -- FreeType is not even vendored in this repository (see CLAUDE.md's
//     "ZERO bibliotecas" rule and the CLEAN-ROOM constraint every internalization ticket carries).
//     Any resemblance to FreeType's public API shape is a resemblance to the SPEC both parse, not
//     to FreeType's code.
//
//     WHAT THIS FILE IS: a PURE, blob-in/data-out parser -- `glx_sfnt_open` takes a pointer+length
//     the CALLER already has in memory and never performs file I/O itself (opening/reading a
//     `.ttf` off disk is the eventual `L1.19-FONTENG` C++ shell's job, per this ticket's brief --
//     Layer 0 has no `open`/`openat` syscall wrapper today and this parser does not need one).
//     Every public function is REENTRANT and touches no global/static mutable state -- multiple
//     `glx_sfnt_face` values from different (or the same) blobs coexist freely; a `glx_sfnt_face`
//     is a small, caller-owned, memcpy-able value (no internal pointers need `free`ing) that
//     borrows read-only pointers INTO the caller's blob -- the blob must outlive every
//     `glx_sfnt_face` derived from it and every call using one.
//
//     WHY `<stdint.h>`/`<stddef.h>`, NOT `include/types.h`: this header lives under
//     `include/core/` (ADR-0009's physical boundary, see include/core/core.h's own file header)
//     because SOV-SFNT is meant to be consumable from a future HOSTED C++ shell
//     (`L1.19-FONTENG`, glintfx) the same way `glx_malloc`/`glx_memcpy` already are -- so, like
//     `core.h`, this header pulls in ONLY freestanding-SAFE, compiler-provided standard headers
//     (`<stdint.h>` for the fixed-width `uint8_t`/`uint16_t`/`int16_t`/`uint32_t` types the binary
//     format is defined in terms of, `<stddef.h>` for `size_t`) -- both are among the handful of
//     headers the C standard GUARANTEES exist under `-ffreestanding` because they are
//     compiler-provided, not libc-linked (see `include/printf.h`'s file header for the same
//     argument already made for `<stdarg.h>`, and `core.h`'s for `<stddef.h>`). It never includes
//     Layer 0's own freestanding-internal `types.h`/`alloc.h`/`mem.h`.
//
//     NO DYNAMIC ALLOCATION: every function here either returns fixed-size scalar output
//     (`glx_sfnt_face`, `glx_sfnt_hmetrics`) or writes into CALLER-SUPPLIED, fixed-capacity
//     buffers (`glx_sfnt_glyph_outline`'s `points_out`/`contour_ends_out`) -- there is no
//     `glx_sfnt_close`/`glx_sfnt_free` because there is nothing to free: `src/sfnt.c` never calls
//     `malloc`/`glx_malloc`. This sidesteps the whole heap-ownership question (ADR-0009 gate 3)
//     for this ticket entirely and keeps the parser's own attack surface (a file format parser is
//     a classic hostile-input surface -- see `docs/`/memory `feedback_auditoria_domino`) as small
//     as possible: no allocator to corrupt, no `free` to double-call, only bounds-checked reads
//     over a blob the caller owns.
//
//     SCOPE (v1, per this ticket's brief -- "só o que a Open Sans + a UI do glintfx precisam"):
//     `head`/`maxp`/`hhea`/`hmtx` (metrics), `cmap` formats 4 and 12 ONLY (Open Sans' own cmap is
//     format-4-only, BMP; format 12 is exercised by a hand-built synthetic fixture in
//     `tests/test_sfnt.c` -- see that file), `loca`/`glyf` (quadratic outlines, INCLUDING
//     composite glyphs with a translate + optional 2x2/scale transform -- required for the pt-br
//     accented glyphs `á`/`ç`/`ã`, all composites in Open Sans). Deliberately OUT of scope (YAGNI,
//     same anti-over-engineering discipline this whole codebase applies): CFF/OTF outlines
//     (PostScript cubics -- `sfntVersion == 'OTTO'` is rejected with `GLX_SFNT_ERR_BAD_VERSION`),
//     `kern`/`GPOS` kerning, hinting bytecode (`glyf`'s `instructions` are parsed-past, never
//     interpreted -- SOV-RAST, the next piece, rasterizes UN-hinted outlines, same trade-off
//     `stb_truetype` makes), composite ARGS_ARE_XY_VALUES==0 (point-matching component
//     positioning, a rare mode no glyph in Open Sans uses -- `GLX_SFNT_ERR_UNSUPPORTED`).
//
//     HARDENING POSTURE (this is a FILE FORMAT PARSER -- the canonical hostile-input attack
//     surface): every multi-byte read anywhere in this file goes through `rd_u8`/`rd_i8`/`rd_u16`/
//     `rd_i16`/`rd_u32` (src/sfnt.c), which bounds-check `offset + size <= blob_len`
//     WITHOUT ever computing `offset + size` directly (that addition could itself overflow a
//     hostile `size_t offset` near `SIZE_MAX` -- the check is always the overflow-safe
//     `offset > len || size > len - offset` shape, `offset <= len` established first). A failed
//     bounds check returns a `glx_sfnt_result` error code, NEVER touches memory outside `[blob,
//     blob+len)`, NEVER asserts/aborts the process, and NEVER loops without a hard bound (composite
//     glyph recursion is capped at `GLX_SFNT_MAX_COMPOSITE_DEPTH`, below -- this is what turns a
//     self-referencing composite component, or a deep chain of them, into a clean
//     `GLX_SFNT_ERR_TOO_DEEP` instead of a stack-exhausting infinite recursion). See
//     `tests/test_sfnt.c`'s "hostile" section for the executed proof: truncated tables, an
//     out-of-blob table offset, a descending `loca` (the specific case this ticket's brief names),
//     a lying `numGlyphs`, and a malformed `cmap` all resolve to a clean error return, verified
//     under the C1 harness (and, where practical, `valgrind`).
// PT: SOV-SFNT (FT-F1, 2ª peça da AUD-TRANS-2) -- um PARSER de fonte SFNT/TrueType clean-room,
//     construído inteiramente a partir da especificação PÚBLICA do formato OpenType/TrueType
//     (spec OpenType da Microsoft / TrueType Reference Manual da Apple -- ambas descrições
//     livremente publicadas de um formato de arquivo binário, não código-fonte de implementação
//     nenhuma). **Nenhum código do FreeType foi lido pra escrever este arquivo** -- o FreeType
//     nem está vendorizado neste repositório (ver a regra "ZERO bibliotecas" do CLAUDE.md e a
//     restrição CLEAN-ROOM que toda tarefa de internalização carrega). Qualquer semelhança com o
//     formato de API pública do FreeType é semelhança com a ESPECIFICAÇÃO que ambos parseiam, não
//     com o código do FreeType.
//
//     O QUE ESTE ARQUIVO É: um parser PURO, blob-entra/dado-sai -- `glx_sfnt_open` recebe um
//     ponteiro+tamanho que quem CHAMA já tem em memória e nunca faz I/O de arquivo por conta
//     própria (abrir/ler um `.ttf` do disco é trabalho da futura casca C++ `L1.19-FONTENG`,
//     conforme o brief desta tarefa -- a Camada 0 não tem wrapper de syscall `open`/`openat` hoje
//     e este parser não precisa de um). Toda função pública é REENTRANTE e não toca estado
//     global/estático mutável nenhum -- múltiplos `glx_sfnt_face` de blobs diferentes (ou do
//     mesmo) coexistem livremente; um `glx_sfnt_face` é um valor pequeno, possuído por quem chama,
//     copiável por memcpy (sem ponteiro interno que precise de `free`) que empresta ponteiros
//     somente-leitura PRA DENTRO do blob de quem chama -- o blob precisa sobreviver a todo
//     `glx_sfnt_face` derivado dele e a toda chamada que o usa.
//
//     POR QUE `<stdint.h>`/`<stddef.h>`, NÃO `include/types.h`: este header mora sob
//     `include/core/` (a fronteira física da ADR-0009, ver o cabeçalho de arquivo do próprio
//     include/core/core.h) porque o SOV-SFNT é pensado pra ser consumível a partir de uma futura
//     casca C++ HOSTED (`L1.19-FONTENG`, glintfx) do mesmo jeito que `glx_malloc`/`glx_memcpy` já
//     são -- então, como o `core.h`, este header puxa SÓ headers padrão SEGUROS em freestanding,
//     fornecidos pelo compilador (`<stdint.h>` pros tipos de largura fixa
//     `uint8_t`/`uint16_t`/`int16_t`/`uint32_t` em que o formato binário é definido, `<stddef.h>`
//     pro `size_t`) -- ambos entre o punhado de headers que o padrão C GARANTE existirem sob
//     `-ffreestanding` porque são fornecidos pelo compilador, não linkados da libc (ver o
//     cabeçalho de arquivo do `include/printf.h` pro mesmo argumento já feito pro `<stdarg.h>`, e
//     o do `core.h` pro `<stddef.h>`). Nunca inclui o `types.h`/`alloc.h`/`mem.h` freestanding
//     INTERNOS da Camada 0.
//
//     SEM ALOCAÇÃO DINÂMICA: toda função aqui ou retorna saída escalar de tamanho fixo
//     (`glx_sfnt_face`, `glx_sfnt_hmetrics`) ou escreve em buffers de capacidade fixa FORNECIDOS
//     por quem chama (`points_out`/`contour_ends_out` do `glx_sfnt_glyph_outline`) -- não há
//     `glx_sfnt_close`/`glx_sfnt_free` porque não há nada pra liberar: `src/sfnt.c` nunca chama
//     `malloc`/`glx_malloc`. Isso contorna a pergunta inteira de ownership de heap (gate 3 da
//     ADR-0009) pra esta tarefa e mantém a superfície de ataque do próprio parser (um parser de
//     formato de arquivo é a superfície clássica de input hostil -- ver docs/memória
//     `feedback_auditoria_domino`) a mais estreita possível: nenhum alocador pra corromper, nenhum
//     `free` pra chamar duas vezes, só leituras com checagem de limite sobre um blob que quem
//     chama possui.
//
//     ESCOPO (v1, conforme o brief desta tarefa -- "só o que a Open Sans + a UI do glintfx
//     precisam"): `head`/`maxp`/`hhea`/`hmtx` (métricas), `cmap` formatos 4 e 12 SÓ (o cmap da
//     própria Open Sans é só-formato-4, BMP; o formato 12 é exercitado por uma fixture sintética
//     feita à mão em `tests/test_sfnt.c` -- ver aquele arquivo), `loca`/`glyf` (outlines
//     quadráticos, INCLUINDO glyphs compostos com translação + transformação opcional 2x2/escala
//     -- necessário pros glyphs acentuados pt-br `á`/`ç`/`ã`, todos compostos na Open Sans). Fora
//     de escopo de propósito (YAGNI, mesma disciplina anti over-engineering deste código-base
//     inteiro): outlines CFF/OTF (cúbicas PostScript -- `sfntVersion == 'OTTO'` é rejeitado com
//     `GLX_SFNT_ERR_BAD_VERSION`), kerning `kern`/`GPOS`, bytecode de hinting (as `instructions`
//     do `glyf` são só puladas na leitura, nunca interpretadas -- o SOV-RAST, a próxima peça,
//     rasteriza outlines SEM hinting, mesma troca que o `stb_truetype` faz), componente composto
//     com ARGS_ARE_XY_VALUES==0 (posicionamento por casamento de ponto, um modo raro que nenhum
//     glyph na Open Sans usa -- `GLX_SFNT_ERR_UNSUPPORTED`).
//
//     POSTURA DE HARDENING (isto É um PARSER DE FORMATO DE ARQUIVO -- a superfície canônica de
//     input hostil): toda leitura multi-byte em qualquer lugar deste arquivo passa por
//     `rd_u8`/`rd_i8`/`rd_u16`/`rd_i16`/`rd_u32` (src/sfnt.c), que checam limite de
//     `offset + size <= blob_len` SEM nunca computar `offset + size` diretamente (essa soma
//     poderia ela mesma estourar um `size_t offset` hostil perto de `SIZE_MAX` -- a checagem é
//     sempre no formato seguro-contra-overflow `offset > len || size > len - offset`, com
//     `offset <= len` estabelecido primeiro). Uma checagem de limite que falha retorna um código
//     de erro `glx_sfnt_result`, NUNCA toca memória fora de `[blob, blob+len)`, NUNCA
//     aserta/aborta o processo, e NUNCA laça sem limite rígido (a recursão de glyph composto é
//     limitada a `GLX_SFNT_MAX_COMPOSITE_DEPTH`, abaixo -- é isso que transforma um componente
//     composto auto-referente, ou uma cadeia funda deles, num `GLX_SFNT_ERR_TOO_DEEP` limpo em vez
//     de uma recursão infinita que esgota a pilha). Ver a seção "hostil" de `tests/test_sfnt.c`
//     pra prova executada: tabelas truncadas, um offset de tabela fora do blob, um `loca`
//     descendente (o caso específico que o brief desta tarefa nomeia), um `numGlyphs` mentiroso, e
//     um `cmap` malformado todos resolvem pra um retorno de erro limpo, verificado sob o harness
//     C1 (e, onde prático, `valgrind`).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// EN: Composite glyph recursion cap (a component referencing a component referencing a
//     component...). 8 is generous for anything a real font legitimately does (Open Sans' own
//     accented glyphs are ONE level deep: `á` = base letter + accent mark, no glyph nests
//     further) while still being a small, cheap-to-bound-check integer -- a hostile font with a
//     self-referencing or deeply-chained composite glyph hits this cap and gets a clean
//     `GLX_SFNT_ERR_TOO_DEEP` instead of unbounded recursion (see this file's header for the full
//     rationale).
// PT: Teto de recursão de glyph composto (um componente referenciando um componente
//     referenciando um componente...). 8 é generoso pra qualquer coisa que uma fonte real
//     legitimamente faz (os próprios glyphs acentuados da Open Sans têm UM nível de
//     profundidade: `á` = letra base + acento, nenhum glyph aninha mais fundo) e ainda assim é um
//     inteiro pequeno e barato de checar como limite -- uma fonte hostil com um glyph composto
//     auto-referente ou encadeado fundo bate nesse teto e recebe um `GLX_SFNT_ERR_TOO_DEEP` limpo
//     em vez de recursão sem limite (ver o cabeçalho deste arquivo pro racional completo).
#define GLX_SFNT_MAX_COMPOSITE_DEPTH 8

// EN: Every fallible operation in this API returns one of these. `GLX_SFNT_OK == 0` (the usual C
//     "0 is success" convention) so `if (glx_sfnt_open(...) != GLX_SFNT_OK)` and
//     `if (glx_sfnt_open(...))` (as an error check) both read naturally; this codebase's own
//     style prefers the explicit `!= GLX_SFNT_OK` form (see tests/test_sfnt.c).
// PT: Toda operação falível desta API retorna um destes. `GLX_SFNT_OK == 0` (a convenção C usual
//     de "0 é sucesso") então `if (glx_sfnt_open(...) != GLX_SFNT_OK)` e
//     `if (glx_sfnt_open(...))` (como checagem de erro) leem-se naturalmente ambos; o estilo
//     próprio deste código-base prefere a forma explícita `!= GLX_SFNT_OK` (ver
//     tests/test_sfnt.c).
typedef enum {
    GLX_SFNT_OK = 0,
    // EN: A NULL required pointer, a zero-length blob, or an internally-inconsistent call
    //     (e.g. a non-NULL capacity with a NULL buffer pointer).
    // PT: Um ponteiro obrigatório NULL, um blob de tamanho zero, ou uma chamada internamente
    //     inconsistente (ex.: uma capacidade não-NULL com um ponteiro de buffer NULL).
    GLX_SFNT_ERR_INVALID_ARG,
    // EN: A read (table directory, or any field inside head/maxp/hhea/hmtx/cmap/loca/glyf) fell
    //     partially or fully outside `[blob, blob+len)` -- "not enough bytes for what was
    //     declared", the umbrella result for every bounds-checked read failure in this file.
    // PT: Uma leitura (diretório de tabela, ou qualquer campo dentro de
    //     head/maxp/hhea/hmtx/cmap/loca/glyf) caiu parcial ou totalmente fora de
    //     `[blob, blob+len)` -- "bytes insuficientes pro que foi declarado", o resultado
    //     guarda-chuva pra toda falha de leitura com checagem de limite neste arquivo.
    GLX_SFNT_ERR_TRUNCATED,
    // EN: `sfntVersion` is not a supported TrueType-outline variant (e.g. `'OTTO'`, a CFF/OTF
    //     font with PostScript cubic outlines -- out of scope, see this file's header).
    // PT: `sfntVersion` não é uma variante suportada de outline TrueType (ex.: `'OTTO'`, uma
    //     fonte CFF/OTF com outlines cúbicas PostScript -- fora de escopo, ver o cabeçalho deste
    //     arquivo).
    GLX_SFNT_ERR_BAD_VERSION,
    // EN: A required table tag (`head`/`maxp`/`hhea`/`hmtx`/`cmap`/`loca`/`glyf`) is absent from
    //     the table directory.
    // PT: Um tag de tabela obrigatória (`head`/`maxp`/`hhea`/`hmtx`/`cmap`/`loca`/`glyf`) está
    //     ausente do diretório de tabela.
    GLX_SFNT_ERR_TABLE_MISSING,
    // EN: An internally-inconsistent offset/size relationship was detected even though the bytes
    //     involved DO exist in the blob -- e.g. a `loca` entry that decreases
    //     (`loca[gid] > loca[gid+1]`, the specific hostile case this ticket's brief names),
    //     `numberOfHMetrics == 0` or `> numGlyphs`, or a table's declared `[offset, offset+length)`
    //     falling outside the blob at directory-validation time.
    // PT: Uma relação offset/tamanho internamente inconsistente foi detectada mesmo os bytes
    //     envolvidos EXISTINDO no blob -- ex.: uma entrada de `loca` que decresce
    //     (`loca[gid] > loca[gid+1]`, o caso hostil específico que o brief desta tarefa nomeia),
    //     `numberOfHMetrics == 0` ou `> numGlyphs`, ou um `[offset, offset+length)` declarado de
    //     uma tabela caindo fora do blob no momento de validação do diretório.
    GLX_SFNT_ERR_BAD_OFFSET,
    // EN: A malformed `glyf` record: `numberOfContours < -1`, non-increasing
    //     `endPtsOfContours`, or an out-of-range glyph id referenced by a composite component.
    // PT: Um registro `glyf` malformado: `numberOfContours < -1`, `endPtsOfContours`
    //     não-crescente, ou um id de glyph fora de faixa referenciado por um componente composto.
    GLX_SFNT_ERR_BAD_GLYPH,
    // EN: A recognized-but-unsupported feature was encountered -- today, only a composite
    //     component with `ARGS_ARE_XY_VALUES` unset (point-matching positioning mode instead of
    //     an xy offset; no glyph in Open Sans uses it, see this file's header).
    // PT: Um recurso reconhecido-mas-não-suportado foi encontrado -- hoje, só um componente
    //     composto com `ARGS_ARE_XY_VALUES` desmarcado (modo de posicionamento por casamento de
    //     ponto em vez de offset xy; nenhum glyph na Open Sans usa isso, ver o cabeçalho deste
    //     arquivo).
    GLX_SFNT_ERR_UNSUPPORTED,
    // EN: Composite glyph recursion exceeded `GLX_SFNT_MAX_COMPOSITE_DEPTH`.
    // PT: Recursão de glyph composto excedeu `GLX_SFNT_MAX_COMPOSITE_DEPTH`.
    GLX_SFNT_ERR_TOO_DEEP,
    // EN: The caller-supplied `points_out`/`contour_ends_out` capacity is too small to hold this
    //     glyph's (possibly flattened-composite) outline. On this result, `*out` and the two
    //     buffers' CONTENTS are unspecified/partial -- only a `GLX_SFNT_OK` result guarantees
    //     `*out` is meaningful.
    // PT: A capacidade de `points_out`/`contour_ends_out` fornecida por quem chama é pequena
    //     demais pra caber o outline (possivelmente composto-achatado) deste glyph. Nesse
    //     resultado, `*out` e o CONTEÚDO dos dois buffers ficam não-especificados/parciais -- só
    //     um resultado `GLX_SFNT_OK` garante que `*out` é significativo.
    GLX_SFNT_ERR_BUFFER_TOO_SMALL,
} glx_sfnt_result;

// EN: A parsed font face -- table offsets/lengths and top-level metrics extracted once by
//     `glx_sfnt_open`, everything else (individual glyph lookups/outlines) resolved lazily,
//     bounds-checked again on every call. Every field here is a PLAIN VALUE (no owned pointers,
//     `blob` is a BORROWED read-only view into the caller's buffer) -- safe to copy, no
//     destructor needed, no `glx_sfnt_close`.
// PT: Uma face de fonte parseada -- offsets/tamanhos de tabela e métricas de topo extraídos uma
//     vez pelo `glx_sfnt_open`, todo o resto (lookups/outlines de glyph individuais) resolvido
//     preguiçosamente, checado de limite de novo a cada chamada. Todo campo aqui é um VALOR PURO
//     (sem ponteiro possuído, `blob` é uma visão somente-leitura EMPRESTADA do buffer de quem
//     chama) -- seguro de copiar, sem destrutor necessário, sem `glx_sfnt_close`.
typedef struct {
    const uint8_t* blob;
    size_t len;

    // EN: `head` -- `unitsPerEm` is the font-design coordinate space every other font-unit value
    //     in this API (bbox, advance widths, glyph point coordinates) is expressed in;
    //     `index_to_loc_format` is `loca`'s own entry width (0 = `uint16` halved, 1 = `uint32`
    //     direct) -- exposed for completeness, callers normally never need it directly.
    // PT: `head` -- `units_per_em` é o espaço de coordenada de desenho da fonte em que todo
    //     outro valor em unidade-de-fonte desta API (bbox, larguras de avanço, coordenadas de
    //     ponto de glyph) é expresso; `index_to_loc_format` é a própria largura de entrada do
    //     `loca` (0 = `uint16` pela metade, 1 = `uint32` direto) -- exposto por completude,
    //     quem chama normalmente nunca precisa dele diretamente.
    uint16_t units_per_em;
    int16_t index_to_loc_format;

    // EN: `maxp` -- total glyph count; every valid glyph id satisfies `0 <= gid < num_glyphs`.
    // PT: `maxp` -- contagem total de glyph; todo id de glyph válido satisfaz
    //     `0 <= gid < num_glyphs`.
    uint16_t num_glyphs;

    // EN: `hhea` -- horizontal line metrics (font units), the values a text layout engine sums
    //     to compute line height/baseline placement.
    // PT: `hhea` -- métricas de linha horizontal (unidades de fonte), os valores que um motor de
    //     layout de texto soma pra calcular altura de linha/posicionamento de baseline.
    int16_t ascender;
    int16_t descender;
    int16_t line_gap;
    uint16_t num_h_metrics;

    // EN: Absolute byte offsets/lengths of `hmtx`/`loca`/`glyf` WITHIN `blob`, already validated
    //     by `glx_sfnt_open` (every byte `glx_sfnt_hmetrics`/`glx_sfnt_glyph_outline` will ever
    //     read from these three tables is within `[blob, blob+len)` -- see this file's header).
    //     Layer-0-internal shape (not meant to be poked by callers), kept in the struct rather
    //     than a private/opaque handle to avoid needing an allocation to own one (see this file's
    //     header, "NO DYNAMIC ALLOCATION").
    // PT: Offsets/tamanhos de byte absolutos de `hmtx`/`loca`/`glyf` DENTRO de `blob`, já
    //     validados pelo `glx_sfnt_open` (todo byte que `glx_sfnt_hmetrics`/
    //     `glx_sfnt_glyph_outline` algum dia vão ler dessas três tabelas está dentro de
    //     `[blob, blob+len)` -- ver o cabeçalho deste arquivo). Formato interno-da-Camada-0 (não
    //     é pra quem chama cutucar), mantido na struct em vez de um handle privado/opaco pra
    //     evitar precisar de uma alocação pra possuir um (ver o cabeçalho deste arquivo, "SEM
    //     ALOCAÇÃO DINÂMICA").
    size_t hmtx_off;
    size_t loca_off;
    size_t glyf_off;
    size_t glyf_len;

    // EN: The chosen `cmap` subtable (format 12 preferred over format 4 when both are present --
    //     format 12 covers all of Unicode, format 4 only the Basic Multilingual Plane). `0`/`0`
    //     if no usable format-4-or-12 subtable was found -- `glx_sfnt_open` still SUCCEEDS in
    //     that case (a font missing a usable cmap can still serve glyph outlines/metrics by raw
    //     gid), `glx_sfnt_glyph_id` then always returns `0` (`.notdef`).
    // PT: A subtabela `cmap` escolhida (formato 12 preferido sobre formato 4 quando ambos
    //     presentes -- formato 12 cobre todo o Unicode, formato 4 só o Plano Multilingual
    //     Básico). `0`/`0` se nenhuma subtabela formato-4-ou-12 utilizável foi encontrada --
    //     `glx_sfnt_open` ainda assim TEM SUCESSO nesse caso (uma fonte sem um cmap utilizável
    //     ainda pode servir outlines/métricas de glyph por gid cru), `glx_sfnt_glyph_id` então
    //     sempre retorna `0` (`.notdef`).
    size_t cmap_subtable_off;
    uint16_t cmap_format;
} glx_sfnt_face;

// EN: Parses the SFNT table directory + `head`/`maxp`/`hhea`/`hmtx`-size/`loca`-size/`cmap`
//     structural validation from `blob[0..len)`, filling `*out` on success. Does NOT copy
//     `blob`'s bytes -- `*out` borrows read-only pointers into it, so `blob` must remain valid
//     and unchanged for the lifetime of every subsequent call using `*out`. Returns
//     `GLX_SFNT_ERR_INVALID_ARG` if `blob == NULL`, `len == 0`, or `out == NULL`; see
//     `glx_sfnt_result` for every other failure this can return -- ALL of them are clean (no
//     partial/undefined `*out` is ever read past this call's own return, and `*out` itself is
//     left untouched on any non-`GLX_SFNT_OK` result).
// PT: Parseia o diretório de tabela SFNT + validação estrutural de
//     `head`/`maxp`/`hhea`/tamanho-de-`hmtx`/tamanho-de-`loca`/`cmap` a partir de `blob[0..len)`,
//     preenchendo `*out` em sucesso. NÃO copia os bytes de `blob` -- `*out` empresta ponteiros
//     somente-leitura pra dentro dele, então `blob` precisa continuar válido e inalterado pela
//     vida de toda chamada subsequente usando `*out`. Retorna `GLX_SFNT_ERR_INVALID_ARG` se
//     `blob == NULL`, `len == 0`, ou `out == NULL`; ver `glx_sfnt_result` pra toda outra falha
//     que isto pode retornar -- TODAS elas são limpas (nenhum `*out` parcial/indefinido é lido
//     além do retorno desta própria chamada, e `*out` em si fica intocado em qualquer resultado
//     diferente de `GLX_SFNT_OK`).
glx_sfnt_result glx_sfnt_open(const uint8_t* blob, size_t len, glx_sfnt_face* out);

// EN: Maps a Unicode codepoint to a glyph id via `face`'s chosen `cmap` subtable (format 4 or
//     12, see `glx_sfnt_face::cmap_format`). Returns `0` (`.notdef`, TrueType's own convention
//     for "no glyph") for: `face == NULL`, no usable cmap subtable, the codepoint has no mapping,
//     or (defense-in-depth) a mapping was found but resolved to a glyph id `>= num_glyphs` --
//     this function never fails loudly, `.notdef` is always a safe fallback a caller can render
//     as-is. Format-4-only fonts (Open Sans included) return `0` for any codepoint above
//     `U+FFFF` (outside the Basic Multilingual Plane, which format 4 cannot represent) exactly as
//     if it were unmapped.
// PT: Mapeia um codepoint Unicode pra um id de glyph via a subtabela `cmap` escolhida de `face`
//     (formato 4 ou 12, ver `glx_sfnt_face::cmap_format`). Retorna `0` (`.notdef`, a própria
//     convenção do TrueType pra "sem glyph") pra: `face == NULL`, nenhuma subtabela cmap
//     utilizável, o codepoint não tem mapeamento, ou (defesa em profundidade) um mapeamento foi
//     achado mas resolveu pra um id de glyph `>= num_glyphs` -- esta função nunca falha alto,
//     `.notdef` é sempre um fallback seguro que quem chama pode renderizar como está. Fontes
//     só-formato-4 (Open Sans inclusa) retornam `0` pra qualquer codepoint acima de `U+FFFF`
//     (fora do Plano Multilingual Básico, que o formato 4 não consegue representar) exatamente
//     como se não estivesse mapeado.
uint32_t glx_sfnt_glyph_id(const glx_sfnt_face* face, uint32_t codepoint);

// EN: Reads `hmtx`'s horizontal metrics (font units) for `gid`: `*advance_width_out` (how far
//     the pen advances after this glyph) and `*lsb_out` (left side bearing -- the gap between the
//     pen position and the glyph outline's own `x_min`). Returns `GLX_SFNT_ERR_INVALID_ARG` if
//     `face == NULL`, `advance_width_out == NULL`, `lsb_out == NULL`, or `gid >= num_glyphs`;
//     `GLX_SFNT_OK` otherwise (this table's own internal consistency was already fully validated
//     by `glx_sfnt_open`, so this call cannot fail any other way).
// PT: Lê as métricas horizontais (unidades de fonte) do `hmtx` pro `gid`: `*advance_width_out`
//     (o quanto a pena avança depois deste glyph) e `*lsb_out` (left side bearing -- o vão entre
//     a posição da pena e o próprio `x_min` do outline do glyph). Retorna
//     `GLX_SFNT_ERR_INVALID_ARG` se `face == NULL`, `advance_width_out == NULL`,
//     `lsb_out == NULL`, ou `gid >= num_glyphs`; `GLX_SFNT_OK` caso contrário (a consistência
//     interna desta tabela já foi totalmente validada pelo `glx_sfnt_open`, então esta chamada
//     não pode falhar de nenhum outro jeito).
glx_sfnt_result glx_sfnt_hmetrics(const glx_sfnt_face* face, uint32_t gid,
                                   uint16_t* advance_width_out, int16_t* lsb_out);

// EN: One outline point (font units, `head::units_per_em`-scaled) -- `on_curve == 1` is a real
//     point the contour passes through, `on_curve == 0` is a quadratic Bezier CONTROL point (the
//     contour passes NEAR it, not through it; two consecutive off-curve points imply an
//     interpolated on-curve point exactly halfway between them, per the TrueType spec -- SOV-RAST,
//     the next piece, is what actually walks these into a rasterizable curve, this struct only
//     carries the raw point stream).
// PT: Um ponto de outline (unidades de fonte, escaladas por `head::units_per_em`) --
//     `on_curve == 1` é um ponto real que o contorno atravessa, `on_curve == 0` é um ponto de
//     CONTROLE de Bezier quadrática (o contorno passa PERTO dele, não através -- dois pontos
//     off-curve consecutivos implicam um ponto on-curve interpolado exatamente na metade entre
//     eles, conforme a spec TrueType -- o SOV-RAST, a próxima peça, é quem de fato percorre isso
//     numa curva rasterizável, esta struct só carrega o fluxo de ponto cru).
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t on_curve;
} glx_sfnt_point;

// EN: Result of `glx_sfnt_glyph_outline` on `GLX_SFNT_OK`: how many of the caller's
//     `points_out`/`contour_ends_out` slots were actually written, plus the glyph's own bounding
//     box (font units, straight from the `glyf` record header -- valid for both simple AND
//     composite glyphs, no recomputation needed). `num_points == num_contours == 0` (bbox all
//     zero) is the correct, non-error result for a glyph with no outline at all (e.g. `space`).
// PT: Resultado do `glx_sfnt_glyph_outline` em `GLX_SFNT_OK`: quantos dos slots de
//     `points_out`/`contour_ends_out` de quem chama foram de fato escritos, mais a própria
//     bounding box do glyph (unidades de fonte, direto do cabeçalho do registro `glyf` -- válida
//     tanto pra glyph simples QUANTO composto, sem recomputação necessária).
//     `num_points == num_contours == 0` (bbox tudo zero) é o resultado correto, sem erro, pra um
//     glyph sem outline nenhum (ex.: `space`).
typedef struct {
    uint16_t num_points;
    uint16_t num_contours;
    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;
} glx_sfnt_outline;

// EN: Flattens `gid`'s outline (simple OR composite, recursively resolving composite components
//     up to `GLX_SFNT_MAX_COMPOSITE_DEPTH`) into the caller-owned `points_out`
//     (capacity `points_capacity`) and `contour_ends_out` (capacity `contours_capacity`) arrays,
//     filling `*out` on success. `contour_ends_out[i]` is the INDEX (into `points_out`) of the
//     LAST point of contour `i` (TrueType's own `endPtsOfContours` convention, offsets accumulated
//     correctly across flattened composite components -- contour `i`'s points span
//     `points_out[contour_ends_out[i-1]+1 .. contour_ends_out[i]]`, with `contour_ends_out[-1]`
//     implicitly `-1` for `i == 0`).
//
//     Returns `GLX_SFNT_ERR_INVALID_ARG` if `face == NULL`, `out == NULL`,
//     `gid >= face->num_glyphs`, or a non-zero capacity is paired with a `NULL` buffer pointer;
//     `GLX_SFNT_ERR_BUFFER_TOO_SMALL` if the (possibly flattened) outline needs more points/
//     contours than the supplied capacities hold (see `glx_sfnt_result` for the "contents
//     unspecified on this result" caveat); `GLX_SFNT_ERR_TOO_DEEP` for a composite chain past
//     `GLX_SFNT_MAX_COMPOSITE_DEPTH`; `GLX_SFNT_ERR_UNSUPPORTED` for a point-matching composite
//     component; `GLX_SFNT_ERR_BAD_GLYPH`/`GLX_SFNT_ERR_BAD_OFFSET`/`GLX_SFNT_ERR_TRUNCATED` for a
//     malformed/hostile `glyf` record -- see `glx_sfnt_result` for exactly what each means. Every
//     failure mode is a clean error return, never undefined behaviour (see this file's header).
// PT: Achata o outline do `gid` (simples OU composto, resolvendo componentes compostos
//     recursivamente até `GLX_SFNT_MAX_COMPOSITE_DEPTH`) nos arrays `points_out` (capacidade
//     `points_capacity`) e `contour_ends_out` (capacidade `contours_capacity`) possuídos por quem
//     chama, preenchendo `*out` em sucesso. `contour_ends_out[i]` é o ÍNDICE (em `points_out`) do
//     ÚLTIMO ponto do contorno `i` (a própria convenção `endPtsOfContours` do TrueType, offsets
//     acumulados corretamente através de componentes compostos achatados -- os pontos do contorno
//     `i` cobrem `points_out[contour_ends_out[i-1]+1 .. contour_ends_out[i]]`, com
//     `contour_ends_out[-1]` implicitamente `-1` pra `i == 0`).
//
//     Retorna `GLX_SFNT_ERR_INVALID_ARG` se `face == NULL`, `out == NULL`,
//     `gid >= face->num_glyphs`, ou uma capacidade não-zero pareada com um ponteiro de buffer
//     `NULL`; `GLX_SFNT_ERR_BUFFER_TOO_SMALL` se o outline (possivelmente achatado) precisa de
//     mais pontos/contornos do que as capacidades fornecidas cabem (ver `glx_sfnt_result` pro
//     aviso "conteúdo não-especificado neste resultado"); `GLX_SFNT_ERR_TOO_DEEP` pra uma cadeia
//     de composto além de `GLX_SFNT_MAX_COMPOSITE_DEPTH`; `GLX_SFNT_ERR_UNSUPPORTED` pra um
//     componente composto de casamento-de-ponto;
//     `GLX_SFNT_ERR_BAD_GLYPH`/`GLX_SFNT_ERR_BAD_OFFSET`/`GLX_SFNT_ERR_TRUNCATED` pra um registro
//     `glyf` malformado/hostil -- ver `glx_sfnt_result` pro que cada um significa exatamente.
//     Todo modo de falha é um retorno de erro limpo, nunca comportamento indefinido (ver o
//     cabeçalho deste arquivo).
glx_sfnt_result glx_sfnt_glyph_outline(const glx_sfnt_face* face, uint32_t gid,
                                        glx_sfnt_point* points_out, uint16_t points_capacity,
                                        uint16_t* contour_ends_out, uint16_t contours_capacity,
                                        glx_sfnt_outline* out);

#ifdef __cplusplus
}
#endif
