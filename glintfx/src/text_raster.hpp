// SPDX-License-Identifier: MPL-2.0
// EN: D2D-TEXT (Onda 6, T1a -- docs/superpowers/plans/2026-07-24-onda6-draw2d-text.md, ADR-0018)
//     -- the RASTER seam of Draw2D's text machinery (TX2 (a)). Pure CPU: ZERO GL, ZERO RmlUi
//     anywhere in this header or its .cpp -- same "bytes in, data out" discipline as
//     src/image_decode.hpp, built DIRECTLY on the already-vendored sovereign C core
//     (glintfx/vendor/core/include/core/{sfnt,raster,hint}.h, ADR-0011/L1.20-FONTFLIP), NOT a
//     reuse of font_engine_own.hpp's RmlUi-fused shell (ADR-0018 decision (a) -- that shell
//     cannot be carved out without touching the production ui font path).
//
//     WHAT LIVES HERE (TX2 (a)): UTF-8 decode with the U+FFFD replacement policy
//     (decode_utf8_codepoint); face lifecycle over glx_sfnt_open (TextFace -- OWNS a private copy
//     of the font blob for its own lifetime, since glx_sfnt_face only BORROWS pointers into
//     whatever blob it is opened against, include/core/sfnt.h's own file header); per-glyph
//     advance/kern queries (TextFace::glyph_advance_px/kern_px); and the glyph BAKE pipeline
//     (plan_glyph_bake -> bake_glyph_into, outline -> glx_hint_outline -> glx_rasterize_outline
//     into caller-owned scratch, mirroring the proven bake math src/font_engine_own.cpp's
//     BakeFaceInstance() already established and this codebase already ships in production).
//
//     THE FROZEN CONTRACT (GlyphAdvanceSource, below): text_layout.hpp (T1b, TX2 (b)) is pure
//     POSITIONING math -- zero raster, zero SFNT -- and consumes ONLY this small function-pointer
//     struct, never TextFace/glx_sfnt_* directly. This keeps text_layout.hpp's own "zero raster"
//     purity a COMPILE-TIME fact (it cannot even #include this file's raster-heavy internals) --
//     not a convention someone can quietly violate. GlyphAdvanceSource's shape is the first thing
//     this slice commits, on purpose, so T1b can start against it without waiting for the rest of
//     this file.
//
//     ADVANCE/BAKE SPLIT (why glyph_advance_px/kern_px never take a "snap" argument): TX15/TX16
//     (word-wrap, alignment) are computed with "the SAME advance+kern accumulation as everything
//     else" (single source) -- wrap/align results must NOT depend on whether a camera happens to
//     be active. Pixel snapping (TX8: pen-snap X + vsnap Y, no-camera/integer-size path only) is
//     therefore strictly a RENDER-TIME/bake-time concern, layered on top of the already-computed
//     float layout by the caller (Draw2d, T2) -- never fed back into layout math. Consequently:
//     glyph_advance_px/kern_px always return the raw, unsnapped float value (used identically by
//     text_layout AND by the final glyph placement); plan_glyph_bake/bake_glyph_into take a
//     `vsnap` flag that ONLY affects the BITMAP's own internal pixel alignment via
//     glx_hint_outline (Y axis -- the core has no X-axis grid-fitter, include/core/hint.h). Pen-
//     snap X (rounding WHERE the already-baked quad is placed on screen) is entirely the caller's
//     job -- this seam has no opinion on it.
//
//     HOSTILE-INPUT POSTURE (TX6, fail-high, central -- font parsing is the canonical hostile-
//     input surface, see memory feedback_auditoria_domino): every public entry point here
//     validates BEFORE touching the underlying core, never crashes, never reads/writes past a
//     caller-owned buffer. A malformed/truncated/adversarial TTF is exactly what the clean-room
//     parser (SOV-SFNT) was built and reviewed for -- this seam inherits that wall for free and
//     adds its own guards on top (blob/call-size caps, gid range checks, finite-float checks)
//     BEFORE calling into the core, same "guard 1/2 + guard 2/2" belt-and-suspenders idiom
//     src/image_decode.hpp's own file header already documents for the same reason.
//
//     NO LOGGING (same discipline as src/image_decode.hpp): a failure here is communicated ONLY
//     via TextFace::ok()/GlyphBakePlan::ok/bake_glyph_into's bool return -- this header cannot
//     reach Rml::Log and does not try to. The caller (Draw2d, T2) decides what to log and how to
//     degrade.
// PT: D2D-TEXT (Onda 6, T1a -- docs/superpowers/plans/2026-07-24-onda6-draw2d-text.md, ADR-0018)
//     -- a costura de RASTER da maquinaria de texto do Draw2D (TX2 (a)). CPU pura: ZERO GL, ZERO
//     RmlUi neste header ou no .cpp dele -- mesma disciplina "bytes entram, dado sai" do
//     src/image_decode.hpp, construída DIRETO sobre o núcleo C soberano já vendorizado
//     (glintfx/vendor/core/include/core/{sfnt,raster,hint}.h, ADR-0011/L1.20-FONTFLIP), NÃO um
//     reuso da casca do font_engine_own.hpp fundida ao RmlUi (decisão (a) do ADR-0018 -- aquela
//     casca não é carvável sem mexer no caminho de fonte da ui em produção).
//
//     O QUE MORA AQUI (TX2 (a)): decode UTF-8 com a política de substituição U+FFFD
//     (decode_utf8_codepoint); ciclo de vida da face sobre o glx_sfnt_open (TextFace -- POSSUI
//     uma cópia privada do blob de fonte pela própria vida, já que o glx_sfnt_face só EMPRESTA
//     ponteiros pra dentro do blob contra o qual foi aberto, cabeçalho próprio do
//     include/core/sfnt.h); consultas de avanço/kern por-glifo (TextFace::glyph_advance_px/
//     kern_px); e o pipeline de BAKE de glifo (plan_glyph_bake -> bake_glyph_into, outline ->
//     glx_hint_outline -> glx_rasterize_outline em scratch possuído por quem chama, espelhando a
//     matemática de bake já provada que o BakeFaceInstance() do src/font_engine_own.cpp já
//     estabeleceu e este código-base já embarca em produção).
//
//     O CONTRATO CONGELADO (GlyphAdvanceSource, abaixo): text_layout.hpp (T1b, TX2 (b)) é
//     matemática PURA de posicionamento -- zero raster, zero SFNT -- e consome SÓ esta struct
//     pequena de ponteiro de função, nunca TextFace/glx_sfnt_* direto. Isso mantém a pureza "zero
//     raster" do text_layout.hpp um fato de COMPILE-TIME (ele nem consegue incluir os internos
//     pesados-de-raster deste arquivo) -- não uma convenção que alguém pode violar em silêncio. O
//     formato de GlyphAdvanceSource é a primeira coisa que esta fatia commita, de propósito, pro
//     T1b poder começar contra ele sem esperar o resto deste arquivo.
//
//     DIVISÃO AVANÇO/BAKE (por que glyph_advance_px/kern_px nunca recebem um argumento "snap"):
//     TX15/TX16 (word-wrap, alinhamento) são computados com "a MESMA acumulação avanço+kern de
//     tudo o mais" (fonte única) -- resultados de wrap/align NÃO podem depender de haver câmera
//     ativa. Snap de pixel (TX8: pen-snap X + vsnap Y, só no caminho sem-câmera/tamanho-inteiro)
//     é portanto estritamente uma preocupação de RENDER-TIME/bake-time, sobreposta ao layout
//     float já computado por quem chama (Draw2d, T2) -- nunca realimentada na matemática de
//     layout. Consequência: glyph_advance_px/kern_px sempre retornam o valor float cru,
//     não-snapado (usado identicamente pelo text_layout E pelo posicionamento final de glifo);
//     plan_glyph_bake/bake_glyph_into recebem uma flag `vsnap` que SÓ afeta o alinhamento de
//     pixel interno do PRÓPRIO bitmap via glx_hint_outline (eixo Y -- o core não tem grid-fitter
//     de eixo X, include/core/hint.h). Pen-snap X (arredondar ONDE o quad já assado é posto na
//     tela) é inteiramente trabalho de quem chama -- esta costura não tem opinião sobre isso.
//
//     POSTURA DE INPUT HOSTIL (TX6, fail-high, central -- parsing de fonte é a superfície clássica
//     de input hostil, ver memória feedback_auditoria_domino): todo ponto de entrada público aqui
//     valida ANTES de tocar o core por baixo, nunca crasha, nunca lê/escreve além de um buffer
//     possuído por quem chama. Uma TTF malformada/truncada/adversarial é exatamente pra que o
//     parser clean-room (SOV-SFNT) foi construído e revisado -- esta costura herda aquela muralha
//     de graça e adiciona as próprias guardas em cima (tetos de blob/chamada, checagem de faixa
//     de gid, checagem de float finito) ANTES de chamar o core, mesmo idioma cinto-e-suspensório
//     "guarda 1/2 + guarda 2/2" que o cabeçalho próprio do src/image_decode.hpp já documenta pelo
//     mesmo motivo.
//
//     SEM LOG (mesma disciplina do src/image_decode.hpp): uma falha aqui é comunicada SÓ via
//     TextFace::ok()/GlyphBakePlan::ok/o retorno bool do bake_glyph_into -- este header não
//     alcança o Rml::Log e não tenta. Quem chama (Draw2d, T2) decide o que logar e como degradar.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "core/hint.h"
#include "core/raster.h"
#include "core/sfnt.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace glintfx {

// EN: TX6 caps this seam owns. kMaxFontBlobBytes gates TextFace::open() (a load_font() blob, T2's
//     own file-read guard mirrors src/draw2d.cpp's load_texture() 256 MiB pattern at a smaller,
//     font-appropriate ceiling -- TX6 names 64 MiB explicitly). kMaxTextCallBytes is the 1 MiB
//     per-draw_text/measure_text-call cap TX6 names -- this header has no string-walking loop of
//     its own to enforce it against (decode_utf8_codepoint decodes ONE codepoint per call), so
//     enforcement is the CALLER's job (text_layout.hpp/draw2d.cpp's own iteration loop); the
//     constant is declared here because it is a decode-adjacent limit, not because this file
//     enforces it.
// PT: Tetos do TX6 que esta costura possui. kMaxFontBlobBytes gateia o TextFace::open() (um blob
//     de load_font(), a própria guarda de leitura-de-arquivo do T2 espelha o padrão de 256 MiB do
//     load_texture() de src/draw2d.cpp, num teto menor, apropriado pra fonte -- o TX6 nomeia 64
//     MiB explicitamente). kMaxTextCallBytes é o teto de 1 MiB por-chamada de
//     draw_text/measure_text que o TX6 nomeia -- este header não tem laço próprio de percorrer
//     string pra impor isso (decode_utf8_codepoint decodifica UM codepoint por chamada), então a
//     imposição é trabalho de quem chama (o laço de iteração do text_layout.hpp/draw2d.cpp); a
//     constante é declarada aqui porque é um limite adjacente ao decode, não porque este arquivo
//     o impõe.
inline constexpr std::size_t kMaxFontBlobBytes = 64u * 1024u * 1024u; // 64 MiB (TX6).
inline constexpr std::size_t kMaxTextCallBytes = 1u * 1024u * 1024u;  // 1 MiB per call (TX6).

// EN: Per-glyph bitmap dimension cap (TX6, this seam's own -- independent from TX3/TX4's
//     atlas-level 4096x4096 PAGE cap, which is Draw2d/T2's concern): a hostile/degenerate font
//     (an absurd units_per_em vs. bbox combination) could otherwise make ONE glyph request a
//     bitmap large enough to be its own resource-exhaustion vector against the caller's atlas,
//     independent of how many distinct glyphs stream through. Same magnitude as the proven,
//     reviewed cap src/font_engine_own.cpp's BakeFaceInstance() already uses for the identical
//     reason (`gw <= 4096 && gh <= 4096`).
// PT: Teto de dimensão de bitmap por-glifo (TX6, próprio desta costura -- independente do teto de
//     PÁGINA de atlas 4096x4096 do TX3/TX4, que é preocupação do Draw2d/T2): uma fonte
//     hostil/degenerada (uma combinação absurda de units_per_em vs. bbox) poderia, senão, fazer
//     UM glifo pedir um bitmap grande o bastante pra ser o próprio vetor de exaustão de recurso
//     contra o atlas de quem chama, independente de quantos glifos distintos passarem. Mesma
//     magnitude do teto provado e revisado que o BakeFaceInstance() do src/font_engine_own.cpp já
//     usa pelo mesmo motivo (`gw <= 4096 && gh <= 4096`).
inline constexpr int kMaxGlyphBitmapDim = 4096;

// ---------------------------------------------------------------------------------------------
// UTF-8 decode (TX2 (a); TX6/TX15's U+FFFD replacement policy)
// ---------------------------------------------------------------------------------------------

// EN: One decoded step: `codepoint` (U+FFFD on any invalid/overlong/truncated/out-of-range
//     sequence) and `consumed` (how many input bytes this step accounts for -- ALWAYS >= 1 when
//     `len >= 1`, guaranteeing a caller's `while (i < len) { auto d = decode(...); i +=
//     d.consumed; }` loop always makes forward progress, never spins on hostile input).
// PT: Um passo decodificado: `codepoint` (U+FFFD em qualquer sequência inválida/overlong/
//     truncada/fora-de-faixa) e `consumed` (quantos bytes de entrada este passo consome --
//     SEMPRE >= 1 quando `len >= 1`, garantindo que um laço `while (i < len) { auto d =
//     decode(...); i += d.consumed; }` de quem chama sempre avança, nunca gira em input hostil).
struct Utf8Decoded {
  uint32_t codepoint = 0xFFFDu;
  std::size_t consumed = 0;
};

// EN: Decodes ONE UTF-8 codepoint starting at utf8[0..len). Never reads past `len`, never reads
//     `utf8` at all if `utf8 == nullptr` or `len == 0` (returns `{0xFFFD, 0}` -- a caller loop
//     bounded by `i < len` never calls this with `len == 0` in the first place, this is a pure
//     defensive floor). On a valid, minimal, in-range sequence, returns the real codepoint and
//     the sequence's true byte length (1/2/3/4). On ANY of: a truncated multi-byte sequence
//     (not enough bytes left in `[0, len)`), an overlong encoding (a codepoint encoded in more
//     bytes than its value requires), a UTF-16 surrogate codepoint (U+D800..U+DFFF -- illegal in
//     UTF-8 by definition), a codepoint above U+10FFFF, a stray continuation byte (0x80..0xBF)
//     with no lead byte before it, or an invalid lead byte (0xF8..0xFF) -- returns U+FFFD and
//     consumes the "maximal subpart" (the lead byte plus however many trailing bytes were
//     themselves valid continuation bytes BEFORE the sequence broke, per Unicode's own
//     recommended replacement policy) so a byte that did not participate in the broken sequence
//     is re-examined fresh on the next call, never silently swallowed.
// PT: Decodifica UM codepoint UTF-8 começando em utf8[0..len). Nunca lê além de `len`, nunca lê
//     `utf8` de jeito nenhum se `utf8 == nullptr` ou `len == 0` (retorna `{0xFFFD, 0}` -- um laço
//     de quem chama limitado por `i < len` nunca chama isto com `len == 0` de primeira, isto é um
//     piso defensivo puro). Numa sequência válida, mínima, dentro-de-faixa, retorna o codepoint
//     real e o comprimento verdadeiro em byte da sequência (1/2/3/4). Em QUALQUER de: uma
//     sequência multi-byte truncada (bytes insuficientes restantes em `[0, len)`), uma codificação
//     overlong (um codepoint codificado em mais bytes do que o valor dele exige), um codepoint de
//     surrogate UTF-16 (U+D800..U+DFFF -- ilegal em UTF-8 por definição), um codepoint acima de
//     U+10FFFF, um byte de continuação (0x80..0xBF) solto sem lead byte antes, ou um lead byte
//     inválido (0xF8..0xFF) -- retorna U+FFFD e consome a "subparte máxima" (o lead byte mais
//     quantos bytes seguintes forem eles próprios bytes de continuação válidos ANTES da sequência
//     quebrar, conforme a própria política de substituição recomendada do Unicode), então um byte
//     que não participou da sequência quebrada é reexaminado do zero na próxima chamada, nunca
//     engolido em silêncio.
Utf8Decoded decode_utf8_codepoint(const char* utf8, std::size_t len);

// ---------------------------------------------------------------------------------------------
// TX2 (b)'s frozen contract -- text_layout.hpp (T1b) codes against ONLY this struct, never
// against TextFace/glx_sfnt_* directly. Committed FIRST and standalone, deliberately never
// changed shape after that first commit (see this file's own top comment).
// ---------------------------------------------------------------------------------------------

// EN: A pure, function-pointer measuring interface -- zero allocation, zero type erasure (no
//     std::function), so text_layout.hpp's own hot wrap/align loop pays nothing beyond one
//     indirect call per query. `ctx` is an opaque, READ-ONLY pointer the 5 function pointers
//     below receive back unchanged (the "this" a plain C-style callback needs -- `const` because
//     none of them ever mutate through it); `codepoint`/`gid` are exactly what
//     TextFace::glyph_id()/glyph_advance_px()/kern_px() take and return -- see those methods' own
//     doc-comments below for what each one degrades to on invalid input (0/0.f, never UB, never a
//     thrown exception -- this interface's own function pointers are trusted to have that SAME
//     fail-high posture, since make_advance_source() below is their only intended producer).
// PT: Uma interface de medição pura, de ponteiro de função -- zero alocação, zero apagamento de
//     tipo (sem std::function), então o próprio laço quente de wrap/align do text_layout.hpp não
//     paga nada além de uma chamada indireta por consulta. `ctx` é um ponteiro opaco, SOMENTE-
//     LEITURA, que os 5 ponteiros de função abaixo recebem de volta sem mudança (o "this" que um
//     callback estilo-C puro precisa -- `const` porque nenhum deles muta através dele);
//     `codepoint`/`gid` são exatamente o que TextFace::glyph_id()/glyph_advance_px()/kern_px()
//     recebem e retornam -- ver os doc-comments próprios daqueles métodos abaixo pro que cada um
//     degrada em input inválido (0/0.f, nunca UB, nunca uma exceção lançada -- os próprios
//     ponteiros de função desta interface são confiados a ter essa MESMA postura fail-high, já
//     que o make_advance_source() abaixo é o único produtor pretendido deles).
struct GlyphAdvanceSource {
  const void* ctx = nullptr;
  uint32_t (*glyph_id)(const void* ctx, uint32_t codepoint) = nullptr;
  float (*advance_px)(const void* ctx, uint32_t gid) = nullptr;
  float (*kern_px)(const void* ctx, uint32_t left_gid, uint32_t right_gid) = nullptr;
  float (*ascent_px)(const void* ctx) = nullptr;
  float (*line_height_px)(const void* ctx) = nullptr;

  // EN: True only when EVERY function pointer above is non-null -- text_layout.hpp's own entry
  //     point checks this FIRST (TX6 fail-high) and treats a not-ok() source the same as an
  //     empty/degenerate string (never dereferences a null function pointer).
  // PT: Verdadeiro só quando TODO ponteiro de função acima é não-nulo -- o próprio ponto de
  //     entrada do text_layout.hpp checa isto PRIMEIRO (fail-high do TX6) e trata uma source
  //     não-ok() igual a uma string vazia/degenerada (nunca desreferencia um ponteiro de função
  //     nulo).
  bool ok() const {
    return glyph_id != nullptr && advance_px != nullptr && kern_px != nullptr &&
           ascent_px != nullptr && line_height_px != nullptr;
  }
};

// ---------------------------------------------------------------------------------------------
// Face lifecycle (TX2 (a): owns the blob for the face's own lifetime)
// ---------------------------------------------------------------------------------------------

// EN: A parsed, self-contained font face. Unlike glx_sfnt_face (which only BORROWS pointers into
//     a caller-owned blob), TextFace OWNS a private copy of the font bytes -- so a Font2d/T2 can
//     discard its own file-read buffer immediately after TextFace::open() returns, and TextFace
//     itself is safe to store for the lifetime of a loaded font. Copyable/movable like any
//     ordinary value type (the owned std::vector<uint8_t> makes copies real, if rare/unwanted in
//     practice -- T2 is expected to move/hold by reference, never copy per-frame).
// PT: Uma face de fonte parseada, auto-contida. Diferente do glx_sfnt_face (que só EMPRESTA
//     ponteiros pra dentro de um blob possuído por quem chama), TextFace POSSUI uma cópia privada
//     dos bytes da fonte -- então um Font2d/T2 pode descartar o próprio buffer de leitura de
//     arquivo imediatamente depois do TextFace::open() retornar, e o próprio TextFace é seguro de
//     guardar pela vida de uma fonte carregada. Copiável/movível como qualquer tipo-valor comum
//     (o std::vector<uint8_t> possuído torna cópias reais, mesmo que raras/indesejadas na
//     prática -- o T2 é esperado a mover/guardar por referência, nunca copiar por-frame).
class TextFace {
public:
  TextFace() = default;

  // EN: Parses `len` bytes of an in-memory SFNT/TrueType font. Copies the bytes internally (see
  //     this class' own doc-comment above for why) -- `blob`/`len` need not outlive this call.
  //     Returns a TextFace with ok() == false, NEVER throws/crashes, for: `blob == nullptr`,
  //     `len == 0`, `len > kMaxFontBlobBytes` (TX6's 64 MiB font-blob cap), or ANY
  //     glx_sfnt_open() failure (hostile/malformed/unsupported font -- CFF/OTF outlines, a
  //     truncated table, a descending `loca`, ... -- see glx_sfnt_result in include/core/sfnt.h
  //     for the full hostile matrix the core itself already rejects cleanly). A failed TextFace
  //     is a fully valid, inert object -- every other method on it degrades to its own documented
  //     "not ok" answer (0 / 0.f / an all-false struct), never a precondition violation.
  // PT: Parseia `len` bytes de uma fonte SFNT/TrueType em memória. Copia os bytes internamente
  //     (ver o doc-comment próprio desta classe acima pro porquê) -- `blob`/`len` não precisam
  //     sobreviver a esta chamada. Retorna um TextFace com ok() == false, NUNCA lança/crasha,
  //     para: `blob == nullptr`, `len == 0`, `len > kMaxFontBlobBytes` (o teto de 64 MiB de blob
  //     de fonte do TX6), ou QUALQUER falha do glx_sfnt_open() (fonte hostil/malformada/não-
  //     suportada -- outlines CFF/OTF, uma tabela truncada, um `loca` descendente, ... -- ver
  //     glx_sfnt_result em include/core/sfnt.h pra matriz hostil completa que o próprio core já
  //     rejeita de forma limpa). Um TextFace falho é um objeto plenamente válido, inerte -- todo
  //     outro método nele degrada pra própria resposta "não-ok" documentada (0 / 0.f / uma struct
  //     tudo-falso), nunca uma violação de pré-condição.
  static TextFace open(const uint8_t* blob, std::size_t len);

  bool ok() const { return ok_; }

  // EN: Maps a Unicode codepoint to a glyph id via this face's cmap (format 4 or 12). Returns `0`
  //     (`.notdef`, TrueType's own "no glyph" convention -- always safe to bake/render as-is) for
  //     an unmapped codepoint OR a not-ok() face -- never fails loudly (mirrors
  //     glx_sfnt_glyph_id()'s own contract, include/core/sfnt.h).
  // PT: Mapeia um codepoint Unicode pra um id de glifo via o cmap desta face (formato 4 ou 12).
  //     Retorna `0` (`.notdef`, a própria convenção "sem glifo" do TrueType -- sempre seguro de
  //     empacotar/renderizar como está) pra um codepoint não-mapeado OU uma face não-ok() --
  //     nunca falha alto (espelha o próprio contrato do glx_sfnt_glyph_id(), include/core/sfnt.h).
  uint32_t glyph_id(uint32_t codepoint) const;

  // EN: This face's hmtx advance width for `gid`, in PIXELS at `size_px` (font-unit value scaled
  //     by size_px/units_per_em -- see this file's own top comment, "ADVANCE/BAKE SPLIT", for why
  //     this NEVER applies pixel snapping). Returns `0.f` for a not-ok() face, a non-finite or
  //     `<= 0.f` `size_px` (TX6 -- guarded BEFORE any arithmetic), or `gid >= num_glyphs`.
  // PT: A largura de avanço hmtx desta face pro `gid`, em PIXELS em `size_px` (valor em unidade-
  //     de-fonte escalado por size_px/units_per_em -- ver o comentário de topo próprio deste
  //     arquivo, "DIVISÃO AVANÇO/BAKE", pro porquê disto NUNCA aplicar snap de pixel). Retorna
  //     `0.f` pra uma face não-ok(), um `size_px` não-finito ou `<= 0.f` (TX6 -- guardado ANTES de
  //     qualquer conta), ou `gid >= num_glyphs`.
  float glyph_advance_px(uint32_t gid, float size_px) const;

  // EN: The `kern` table adjustment (TX2 (a)) in PIXELS between `left_gid` immediately followed
  //     by `right_gid`, at `size_px`. Returns `0.f` (the neutral "no adjustment" value, always
  //     safe to add unconditionally) for a not-ok() face, a non-finite or `<= 0.f` size_px, a face
  //     with no `kern` table, or any pair with no kerning entry -- mirrors glx_sfnt_kern()'s own
  //     contract (include/core/sfnt.h) exactly, just pre-scaled to pixels.
  // PT: O ajuste da tabela `kern` (TX2 (a)) em PIXELS entre `left_gid` imediatamente seguido de
  //     `right_gid`, em `size_px`. Retorna `0.f` (o valor neutro "sem ajuste", sempre seguro de
  //     somar incondicionalmente) pra uma face não-ok(), um size_px não-finito ou `<= 0.f`, uma
  //     face sem tabela `kern`, ou qualquer par sem entrada de kerning -- espelha o próprio
  //     contrato do glx_sfnt_kern() (include/core/sfnt.h) exatamente, só pré-escalado a pixels.
  float kern_px(uint32_t left_gid, uint32_t right_gid, float size_px) const;

  // EN: This face's line metrics in PIXELS at `size_px` -- `ascent`/`descent` are both POSITIVE
  //     magnitudes (hhea's own `descender` is typically a NEGATIVE font-unit value; this method
  //     sign-flips it so callers never have to re-derive the flip themselves), `line_height =
  //     ascent + descent + line_gap` (item 3 of ADR-0018 (b): "ascent - descent + line-gap" using
  //     hhea's raw signed descender is the SAME quantity this sign-flipped form adds). All fields
  //     stay `0.f` (a valid, inert Metrics{}) for a not-ok() face or a non-finite/`<= 0.f`
  //     `size_px`.
  // PT: As métricas de linha desta face em PIXELS em `size_px` -- `ascent`/`descent` são ambos
  //     magnitudes POSITIVAS (o próprio `descender` do hhea é tipicamente um valor NEGATIVO em
  //     unidade-de-fonte; este método inverte o sinal pra quem chama nunca precisar re-derivar a
  //     inversão sozinho), `line_height = ascent + descent + line_gap` (item 3 do ADR-0018 (b):
  //     "ascent - descent + line-gap" usando o descender cru assinado do hhea é a MESMA grandeza
  //     que esta forma com sinal invertido soma). Todo campo fica `0.f` (um Metrics{} válido,
  //     inerte) pra uma face não-ok() ou um `size_px` não-finito/`<= 0.f`.
  struct Metrics {
    float ascent = 0.f;
    float descent = 0.f;
    float line_gap = 0.f;
    float line_height = 0.f;
  };
  Metrics metrics_px(float size_px) const;

  // EN: Read-only escape hatch for callers that need the raw core face (T2's bake orchestration,
  //     tests) -- returns nullptr for a not-ok() face. Never mutate through this; TextFace's own
  //     methods above are the intended, hardened surface.
  // PT: Válvula de escape somente-leitura pra quem chama e precisa da face crua do core
  //     (orquestração de bake do T2, testes) -- retorna nullptr pra uma face não-ok(). Nunca mute
  //     através disto; os métodos próprios do TextFace acima são a superfície pretendida,
  //     endurecida.
  const glx_sfnt_face* raw() const { return ok_ ? &face_ : nullptr; }

private:
  std::vector<uint8_t> blob_;
  glx_sfnt_face face_{};
  glx_hint_zones hint_zones_{};
  bool ok_ = false;
};

// ---------------------------------------------------------------------------------------------
// Advance-source adapter -- binds a TextFace + size_px into the frozen GlyphAdvanceSource shape.
// ---------------------------------------------------------------------------------------------

// EN: Binds a `size_px` to a `TextFace` for the lifetime of one text_layout.hpp call.
//     `GlyphAdvanceSource::ctx` (produced by make_advance_source() below) borrows THIS struct's
//     own address -- the caller (Draw2d, T2) constructs one on the stack per
//     draw_text/measure_text invocation and must keep it alive for the whole call into
//     text_layout.hpp's entry point (a synchronous, non-reentrant call in every intended use --
//     never stored past that one call).
// PT: Amarra um `size_px` a um `TextFace` pela vida de uma chamada ao text_layout.hpp.
//     `GlyphAdvanceSource::ctx` (produzido pelo make_advance_source() abaixo) empresta o próprio
//     endereço desta struct -- quem chama (Draw2d, T2) constrói uma na pilha por invocação de
//     draw_text/measure_text e precisa mantê-la viva pela chamada inteira ao ponto de entrada do
//     text_layout.hpp (uma chamada síncrona, não-reentrante em todo uso pretendido -- nunca
//     guardada além dessa uma chamada).
struct AdvanceContext {
  const TextFace* face = nullptr;
  float size_px = 0.f;
};

// EN: Builds the frozen GlyphAdvanceSource text_layout.hpp (T1b) consumes, bound to `ctx` (see
//     AdvanceContext's own doc-comment above for the lifetime rule). Returns a GlyphAdvanceSource
//     whose ok() is false -- every function pointer left null -- when `ctx.face == nullptr`,
//     `!ctx.face->ok()`, or `ctx.size_px` is not finite and `> 0.f` (TX6 -- never hands
//     text_layout.hpp a source that would silently divide-by-zero or read a dead/invalid face).
// PT: Constrói o GlyphAdvanceSource congelado que o text_layout.hpp (T1b) consome, amarrado a
//     `ctx` (ver o doc-comment próprio do AdvanceContext acima pra regra de vida). Retorna um
//     GlyphAdvanceSource cujo ok() é falso -- todo ponteiro de função deixado nulo -- quando
//     `ctx.face == nullptr`, `!ctx.face->ok()`, ou `ctx.size_px` não é finito e `> 0.f` (TX6 --
//     nunca entrega ao text_layout.hpp uma source que dividiria por zero em silêncio ou leria uma
//     face morta/inválida).
GlyphAdvanceSource make_advance_source(const AdvanceContext& ctx);

// ---------------------------------------------------------------------------------------------
// Glyph bake (TX2 (a): outline -> glx_hint_outline (vsnap only, when requested) ->
// glx_rasterize_outline into caller-owned scratch)
// ---------------------------------------------------------------------------------------------

// EN: The bitmap dimensions + pen-relative placement `bake_glyph_into()` would produce for
//     `(face, gid, size_px, vsnap)`, WITHOUT touching a single pixel -- lets the caller (Draw2d's
//     atlas, TX3) size/shelf-pack a slot BEFORE ever calling bake_glyph_into(). `bearing_x` is
//     the offset in PIXELS from the pen origin to the bitmap's own left column; `bearing_y` is
//     the offset in PIXELS from the baseline (pen_y) to the bitmap's own top row (negative --
//     ink sits above the baseline in screen's y-down space) -- both include the same 1px
//     transparent border `bitmap_w`/`bitmap_h` do (kPad, see this file's .cpp), which avoids AA
//     bleed between neighbouring cells once the caller packs this bitmap into a shared atlas.
//     `ok == true` with `bitmap_w == bitmap_h == 0` is the CORRECT, non-error answer for a glyph
//     with no ink at all (space, an empty `.notdef`, or an outline this seam's fixed
//     point/contour buffers are too small for -- degrades to "no bitmap baked", same posture
//     src/font_engine_own.cpp's own BakeFaceInstance() already established, never a hard
//     failure). `ok == false` only for: a not-ok() face, `gid >= num_glyphs`, a non-finite or
//     `<= 0.f` size_px, or a glyph whose device bbox would need a bitmap wider/taller than
//     kMaxGlyphBitmapDim (TX6's own per-glyph cap, above).
// PT: As dimensões de bitmap + o posicionamento relativo-à-pena que o `bake_glyph_into()`
//     produziria pra `(face, gid, size_px, vsnap)`, SEM tocar um único pixel -- deixa quem chama
//     (o atlas do Draw2d, TX3) dimensionar/empacotar um slot em prateleira ANTES de algum dia
//     chamar bake_glyph_into(). `bearing_x` é o offset em PIXELS da origem da pena até a própria
//     coluna esquerda do bitmap; `bearing_y` é o offset em PIXELS da baseline (pen_y) até a
//     própria row de topo do bitmap (negativo -- a tinta fica acima da baseline no espaço y-pra-
//     baixo de tela) -- ambos incluem a mesma borda transparente de 1px que `bitmap_w`/
//     `bitmap_h` incluem (kPad, ver o .cpp deste arquivo), que evita sangramento de AA entre
//     células vizinhas quando quem chama empacota este bitmap num atlas compartilhado. `ok ==
//     true` com `bitmap_w == bitmap_h == 0` é a resposta CORRETA, sem erro, pra um glifo sem
//     tinta nenhuma (espaço, um `.notdef` vazio, ou um outline pro qual os buffers fixos de
//     ponto/contorno desta costura são pequenos demais -- degrada pra "nenhum bitmap assado",
//     mesma postura que o próprio BakeFaceInstance() do src/font_engine_own.cpp já estabeleceu,
//     nunca uma falha dura). `ok == false` só pra: uma face não-ok(), `gid >= num_glyphs`, um
//     size_px não-finito ou `<= 0.f`, ou um glifo cuja bbox de dispositivo precisasse de um
//     bitmap mais largo/alto que kMaxGlyphBitmapDim (o teto próprio por-glifo do TX6, acima).
struct GlyphBakePlan {
  bool ok = false;
  int bitmap_w = 0;
  int bitmap_h = 0;
  float bearing_x = 0.f;
  float bearing_y = 0.f;
};

GlyphBakePlan plan_glyph_bake(const TextFace& face, uint32_t gid, float size_px, bool vsnap);

// EN: Forwards to glx_raster_scratch_floats(w, h) (include/core/raster.h) -- exposed here so a
//     caller can size its scratch buffer for bake_glyph_into() without including
//     core/raster.h itself. Returns 0 for w<=0/h<=0/an overflowing (w,h) pair, same contract the
//     wrapped function documents.
// PT: Repassa pro glx_raster_scratch_floats(w, h) (include/core/raster.h) -- exposto aqui pra quem
//     chama dimensionar o próprio buffer de scratch pro bake_glyph_into() sem incluir o
//     core/raster.h em si. Retorna 0 pra w<=0/h<=0/um par (w,h) que estoura, mesmo contrato que a
//     função embrulhada documenta.
std::size_t glyph_bake_scratch_floats(int w, int h);

// EN: Rasterizes `gid`'s outline (at `size_px`, `vsnap` applying glx_hint_outline's Y-axis grid-
//     fit before rasterizing -- see this file's top comment, "ADVANCE/BAKE SPLIT") into
//     `bitmap_out` (row-major, `stride` bytes/row, coverage 0..255). `expect_w`/`expect_h` MUST
//     be the SAME `(face, gid, size_px, vsnap)` plan's `bitmap_w`/`bitmap_h` from
//     plan_glyph_bake() -- this function independently RE-DERIVES the geometry from scratch
//     (single source with plan_glyph_bake(), TX17-class discipline: the two can never silently
//     disagree) and returns `false` WITHOUT writing a single byte if the re-derived dimensions do
//     not match `expect_w`/`expect_h` (a stale/mismatched plan -- e.g. the caller passed a
//     different `size_px` than it planned with). Also returns `false` for every `ok == false`
//     case plan_glyph_bake() itself documents, or `scratch_capacity_floats <
//     glyph_bake_scratch_floats(expect_w, expect_h)`. A `bitmap_w == bitmap_h == 0` plan (a
//     glyph with no ink) is a valid, `true`-returning no-op -- `bitmap_out`/`scratch` are never
//     touched (nothing to write). On any `false`, `bitmap_out`'s contents are UNSPECIFIED
//     (nothing partially written is ever a promised state) -- never a partial write outside
//     `[0, expect_w) x [0, expect_h)`, never UB, for ANY input (hostile font included).
// PT: Rasteriza o outline do `gid` (em `size_px`, `vsnap` aplicando o grid-fit de eixo Y do
//     glx_hint_outline antes de rasterizar -- ver o comentário de topo deste arquivo, "DIVISÃO
//     AVANÇO/BAKE") em `bitmap_out` (row-major, `stride` bytes/row, cobertura 0..255).
//     `expect_w`/`expect_h` PRECISAM ser o `bitmap_w`/`bitmap_h` do MESMO plano `(face, gid,
//     size_px, vsnap)` do plan_glyph_bake() -- esta função RE-DERIVA a geometria do zero,
//     independentemente (fonte única com o plan_glyph_bake(), disciplina classe-TX17: os dois
//     nunca podem discordar em silêncio) e retorna `false` SEM escrever um único byte se as
//     dimensões re-derivadas não baterem com `expect_w`/`expect_h` (um plano obsoleto/
//     descasado -- ex.: quem chama passou um `size_px` diferente do que planejou). Também
//     retorna `false` pra todo caso `ok == false` que o próprio plan_glyph_bake() documenta, ou
//     `scratch_capacity_floats < glyph_bake_scratch_floats(expect_w, expect_h)`. Um plano
//     `bitmap_w == bitmap_h == 0` (um glifo sem tinta) é um no-op válido que retorna `true` --
//     `bitmap_out`/`scratch` nunca são tocados (nada a escrever). Em qualquer `false`, o conteúdo
//     de `bitmap_out` fica NÃO-ESPECIFICADO (nada parcialmente escrito é nunca um estado
//     prometido) -- nunca uma escrita parcial fora de `[0, expect_w) x [0, expect_h)`, nunca UB,
//     pra QUALQUER input (fonte hostil inclusa).
bool bake_glyph_into(const TextFace& face, uint32_t gid, float size_px, bool vsnap, int expect_w,
                     int expect_h, uint8_t* bitmap_out, int stride, float* scratch,
                     std::size_t scratch_capacity_floats);

} // namespace glintfx
