// SPDX-License-Identifier: MPL-2.0
// EN: FontEngineOwn (L1.19-FONTENG, FT-F3) — the "hosted shell" half of ADR-0009's mandatory
//     "hosted shell + C meat" pattern: an ordinary, hosted C++ implementation of RmlUi's
//     Rml::FontEngineInterface, whose only job is glue -- resolving/caching font faces, packing
//     a glyph atlas, and emitting Rml::Mesh/Rml::Texture geometry -- while every byte of the
//     actual font-format algorithm (SFNT/TrueType parsing, quadratic-outline anti-aliased
//     rasterization) is delegated to Layer 0's sovereign C, `extern "C"`-linked in:
//     SOV-SFNT (include/core/sfnt.h, src/sfnt.c) and SOV-RAST (include/core/raster.h,
//     src/raster.c). This file is the JUNCTION piece between the two halves of this repository
//     (see CLAUDE.md's "Duas camadas" section) -- see ADR-0009 for why the split is drawn here
//     (RmlUi's FontEngineInterface is a C++ virtual-class CONTRACT that cannot itself be
//     freestanding C without a full C++ ABI shim (SOV-CXXRT), which ADR-0009 makes optional
//     rather than a blocker).
//
//     ⛔ CLEAN-ROOM: this file was written reading ONLY RmlUi's PUBLIC interface headers
//     (FontEngineInterface.h, FontMetrics.h, Mesh.h, RenderManager.h, CallbackTexture.h,
//     Texture.h, Vertex.h, StringUtilities.h, TextShapingContext.h, Span.h, Types.h -- all MIT,
//     to understand the CONTRACT: what GenerateString must return, the vertex/quad/premultiplied
//     -alpha format, how a texture is created via RenderManager::MakeCallbackTexture()) and the
//     GENERIC RmlUi calling convention confirmed from StyleSheetParser.cpp's ParseFontFaceBlock
//     (family/style/weight come from the RCSS @font-face block itself, not from the font file --
//     this is a fact about RmlUi's own RCSS grammar, not about FreeType). **No FreeType code, and
//     no FreeType-touching part of RmlUi's own default font engine, was read to write this file**
//     -- the algorithmic core (glyph lookup, metrics, outline rasterization) is 100% this
//     repository's own SOV-SFNT/SOV-RAST, built from the public OpenType/TrueType spec and
//     generic computational-geometry knowledge (see those files' own headers).
//
//     SCOPE (v1, MVP -- an autonomous, documented scope decision made while implementing this
//     ticket, not a request for a design review): a font FACE INSTANCE (one family/style/weight/
//     pixel-size combination) BAKES its ENTIRE glyph atlas EAGERLY, ONCE, the first time
//     GetFontFaceHandle() resolves it -- a FIXED codepoint set (printable ASCII U+0020..U+007E,
//     the full printable Latin-1 Supplement U+00A0..U+00FF -- which covers Western-European accents
//     incl. the pt-br set á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ + uppercase, the previously-missing î, and
//     the Latin-1 symbols -- plus common typographic punctuation em/en-dash, curved quotes,
//     ellipsis and bullet; widened for general-purpose distribution in sub-phase 2.1, see
//     BuildBakeSet() in the .cpp for the exact bands). There is NO lazy/incremental atlas growth
//     after that: a codepoint outside the baked set (or one the face itself lacks -- gid 0/.notdef,
//     skipped, never baked as a tofu box) silently has no glyph (advance 0, no quad -- the string
//     still measures/renders, just missing that one character). This was chosen over
//     lazy-bake-on-demand specifically because GetFontFaceHandle() (where a face instance is
//     first resolved) is NOT passed a Rml::RenderManager& (only GenerateString() is) -- eager
//     CPU-side baking needs no RenderManager at all, so it happens synchronously in
//     GetFontFaceHandle(); only the GPU texture upload (MakeCallbackTexture(), which DOES need a
//     RenderManager&) is deferred to the first GenerateString() call for that handle, and is
//     never repeated after that (GetVersion() is therefore a constant `1` for a valid handle --
//     the atlas for a given handle never changes after its first bake, so there is nothing to
//     invalidate RmlUi's cached geometry over). Font effects (PrepareFontEffects) are a pure
//     no-op returning handle `0` -- text-shadow/outline-via-font-effect is out of scope for this
//     ticket (RmlUi tolerates an effects handle of `0`, text still renders without the effect).
//     Kerning (L1.19-FONTENG/L1.20-FONTFLIP amadurecimento): APPLIED between every consecutive
//     baked-glyph pair, via SOV-SFNT's `glx_sfnt_kern` (classic `kern` table, format-0 horizontal
//     subtable only -- see include/core/sfnt.h's own SCOPE note) -- IterateGlyphs() below folds
//     the FUnits adjustment it returns into the pen position, same font-unit->px scale as every
//     other metric this file derives. GPOS kerning/pair-positioning is still out of scope (SOV-SFNT
//     does not parse GPOS at all). letter-spacing (TextShapingContext::letter_spacing) is honoured
//     independently, added AFTER the kern adjustment (both apply, they answer different questions:
//     kern is a font-authored per-PAIR correction, letter-spacing a caller-requested UNIFORM
//     addition). No font collection (.ttc) support -- `face_index` is accepted but ignored (SOV-SFNT parses a single
//     sfnt directory at blob offset 0, matching its own documented scope). No 'name' table
//     parsing -- the family-less LoadFontFace() overload (no confirmed RmlUi call site: RCSS
//     @font-face always supplies family/style/weight explicitly, confirmed by reading the pinned
//     RmlUi source's StyleSheetParser::ParseFontFaceBlock) derives a family from the file name's
//     own stem as a best-effort fallback.
// PT: FontEngineOwn (L1.19-FONTENG, FT-F3) — a metade "casca hosted" do padrão obrigatório
//     "casca-hosted + carne-C" da ADR-0009: uma implementação C++ comum, hosted, da
//     Rml::FontEngineInterface do RmlUi, cujo único trabalho é cola -- resolver/cachear faces de
//     fonte, empacotar um atlas de glyph, e emitir geometria Rml::Mesh/Rml::Texture -- enquanto
//     cada byte do algoritmo real de formato de fonte (parsing SFNT/TrueType, rasterização
//     anti-aliased de outline quadrático) é delegado ao C soberano da Camada 0, linkado via
//     `extern "C"`: SOV-SFNT (include/core/sfnt.h, src/sfnt.c) e SOV-RAST (include/core/raster.h,
//     src/raster.c). Este arquivo é a peça de JUNÇÃO entre as duas metades deste repositório (ver
//     a seção "Duas camadas" do CLAUDE.md) -- ver a ADR-0009 pro motivo da divisão ser traçada
//     aqui (a FontEngineInterface do RmlUi é um CONTRATO de classe virtual C++ que não pode ela
//     mesma ser C freestanding sem uma camada completa de ABI C++ (SOV-CXXRT), que a ADR-0009
//     torna opcional em vez de bloqueador).
//
//     ⛔ CLEAN-ROOM: este arquivo foi escrito lendo SÓ os headers de interface PÚBLICA do RmlUi
//     (FontEngineInterface.h, FontMetrics.h, Mesh.h, RenderManager.h, CallbackTexture.h,
//     Texture.h, Vertex.h, StringUtilities.h, TextShapingContext.h, Span.h, Types.h -- todos MIT,
//     para entender o CONTRATO: o que GenerateString deve retornar, o formato de vértice/quad/
//     alpha-premultiplicado, como uma textura é criada via RenderManager::MakeCallbackTexture())
//     e a convenção de chamada GENÉRICA do RmlUi confirmada a partir do ParseFontFaceBlock do
//     StyleSheetParser.cpp (family/style/weight vêm do próprio bloco RCSS @font-face, não do
//     arquivo de fonte -- isso é um fato sobre a própria gramática RCSS do RmlUi, não sobre o
//     FreeType). **Nenhum código do FreeType, e nenhuma parte que toca FreeType do próprio motor
//     de fonte default do RmlUi, foi lida pra escrever este arquivo** -- o núcleo algorítmico
//     (lookup de glyph, métricas, rasterização de outline) é 100% SOV-SFNT/SOV-RAST próprios
//     deste repositório, construídos a partir da spec pública OpenType/TrueType e conhecimento
//     genérico de geometria computacional (ver os cabeçalhos próprios daqueles arquivos).
//
//     ESCOPO (v1, MVP -- uma decisão de escopo autônoma e documentada tomada ao implementar esta
//     tarefa, não um pedido de revisão de design): uma INSTÂNCIA de face de fonte (uma combinação
//     família/estilo/peso/tamanho-em-px) empacota o atlas de glyph INTEIRO AVIDAMENTE, UMA VEZ, na
//     primeira vez que GetFontFaceHandle() a resolve -- um conjunto FIXO de codepoint (ASCII
//     imprimível U+0020..U+007E, o Latin-1 Supplement imprimível completo U+00A0..U+00FF -- que
//     cobre acentos da Europa Ocidental incl. o conjunto pt-br á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ +
//     maiúsculas, o î que faltava, e os símbolos Latin-1 -- mais pontuação tipográfica comum
//     em/en-dash, aspas curvas, reticências e bullet; ampliado pra distribuição geral na sub-fase
//     2.1, ver BuildBakeSet() no .cpp pras faixas exatas). NÃO há crescimento de atlas preguiçoso/
//     incremental depois disso: um codepoint fora do conjunto empacotado (ou um que a própria face
//     não tenha -- gid 0/.notdef, pulado, nunca empacotado como caixa tofu) silenciosamente não tem
//     glyph (avanço 0, sem quad -- a string ainda mede/renderiza, só falta aquele caractere). Isso
//     foi escolhido em vez de empacotamento
//     preguiçoso-sob-demanda especificamente porque GetFontFaceHandle() (onde uma instância de
//     face é resolvida pela 1ª vez) NÃO recebe um Rml::RenderManager& (só GenerateString()
//     recebe) -- o empacotamento CPU-side ávido não precisa de RenderManager nenhum, então
//     acontece sincronamente em GetFontFaceHandle(); só o upload de textura GPU
//     (MakeCallbackTexture(), que PRECISA de um RenderManager&) é adiado pra 1ª chamada de
//     GenerateString() daquele handle, e nunca é repetido depois disso (GetVersion() portanto é
//     uma constante `1` pra um handle válido -- o atlas de um dado handle nunca muda depois do
//     1º empacotamento, então não há nada pra invalidar na geometria cacheada do RmlUi). Efeitos
//     de fonte (PrepareFontEffects) são um no-op puro retornando handle `0` -- text-shadow/
//     contorno via efeito-de-fonte está fora de escopo desta tarefa (o RmlUi tolera um handle de
//     efeito `0`, o texto ainda renderiza sem o efeito). Kerning (amadurecimento L1.19-FONTENG/
//     L1.20-FONTFLIP): APLICADO entre todo par consecutivo de glyph empacotado, via
//     `glx_sfnt_kern` do SOV-SFNT (tabela `kern` clássica, só subtable formato-0 horizontal --
//     ver a própria nota de ESCOPO do include/core/sfnt.h) -- o IterateGlyphs() abaixo dobra o
//     ajuste em FUnits que ele devolve na posição da pena, na mesma escala unidade-de-fonte->px
//     de toda outra métrica que este arquivo deriva. Kerning via GPOS/pair-positioning segue fora
//     de escopo (o SOV-SFNT não parseia GPOS de forma nenhuma). letter-spacing
//     (TextShapingContext::letter_spacing) é honrado independentemente, somado DEPOIS do ajuste
//     de kern (os dois se aplicam, respondem perguntas diferentes: kern é uma correção
//     por-PAR autorada-pela-fonte, letter-spacing uma adição UNIFORME pedida por quem chama).
//     Sem suporte a coleção de fonte (.ttc) --
//     `face_index` é aceito mas ignorado (SOV-SFNT parseia um único diretório sfnt no offset 0 do
//     blob, batendo com o próprio escopo documentado dele). Sem parsing de tabela 'name' -- a
//     sobrecarga sem-família de LoadFontFace() (sem ponto de chamada confirmado no RmlUi: o RCSS
//     @font-face sempre fornece família/estilo/peso explicitamente, confirmado lendo o
//     StyleSheetParser::ParseFontFaceBlock do source pinado do RmlUi) deriva uma família do
//     próprio stem do nome de arquivo como fallback de melhor esforço.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: Pulled in UNCONDITIONALLY, first, outside any guard -- this is where the
//     GLINTFX_OWN_FONT_ENGINE macro itself (`#cmakedefine01`, always literally 0 or 1, never
//     undefined) comes from. Needed so THIS header can self-guard below regardless of who
//     #includes it or in which CMake config -- see the `#if GLINTFX_OWN_FONT_ENGINE` block
//     right after: in an GLINTFX_OWN_FONT_ENGINE=OFF build (the default), this header must be a
//     harmless, empty-of-Layer-0-dependencies no-op, because clang-tidy's CI lint pass globs
//     EVERY glintfx/src/*.cpp file unconditionally (glintfx/CMakeLists.txt's own
//     `if(GLINTFX_OWN_FONT_ENGINE)` only gates whether font_engine_own.cpp is ADDED TO THE BUILD
//     TARGET -- it does not stop a filesystem glob like `clang-tidy .../src/*.cpp` from finding
//     and parsing the file anyway, at which point THIS header, still #included by that .cpp
//     unconditionally, must not go hunting for "core/raster.h"/"core/sfnt.h" on an include path
//     that set_source_files_properties() never wired in for that config).
// PT: Trazido INCONDICIONALMENTE, primeiro, fora de qualquer guard -- é daqui que vem a própria
//     macro GLINTFX_OWN_FONT_ENGINE (`#cmakedefine01`, sempre literalmente 0 ou 1, nunca
//     indefinida). Necessário para que ESTE header consiga se auto-proteger abaixo
//     independentemente de quem o #inclui ou em qual config do CMake -- ver o bloco
//     `#if GLINTFX_OWN_FONT_ENGINE` logo a seguir: numa build GLINTFX_OWN_FONT_ENGINE=OFF (o
//     padrão), este header precisa ser um no-op inofensivo, sem dependências da Camada 0, porque
//     o passe de lint do CI via clang-tidy varre TODO arquivo glintfx/src/*.cpp
//     incondicionalmente (o próprio `if(GLINTFX_OWN_FONT_ENGINE)` do glintfx/CMakeLists.txt só
//     controla se font_engine_own.cpp é ADICIONADO AO ALVO DE BUILD -- não impede um glob de
//     sistema de arquivos como `clang-tidy .../src/*.cpp` de achar e parsear o arquivo do mesmo
//     jeito, ponto em que ESTE header, ainda #incluído por aquele .cpp incondicionalmente, não
//     pode sair caçando "core/raster.h"/"core/sfnt.h" num include path que o
//     set_source_files_properties() nunca conectou pra aquela config).
#include <glintfx/config.hpp>

#if GLINTFX_OWN_FONT_ENGINE

#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/StringUtilities.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

// EN: Layer 0's sovereign C, `extern "C"`-linked in per ADR-0009's "hosted shell + C meat"
//     pattern -- the actual SFNT parsing / outline rasterization algorithms live here, NOT in
//     this file. `include/core/` is added to this translation unit's include path only when
//     GLINTFX_OWN_FONT_ENGINE=ON (glintfx/CMakeLists.txt) -- reachable at this point precisely
//     BECAUSE the `#if GLINTFX_OWN_FONT_ENGINE` guard above already proved this build wired that
//     include path in (see glintfx/CMakeLists.txt's set_source_files_properties() call).
// PT: C soberano da Camada 0, linkado via `extern "C"` conforme o padrão "casca-hosted + carne-C"
//     da ADR-0009 -- os algoritmos reais de parsing SFNT/rasterização de outline moram aqui, NÃO
//     neste arquivo. `include/core/` só é adicionado ao include path desta unidade de tradução
//     quando GLINTFX_OWN_FONT_ENGINE=ON (glintfx/CMakeLists.txt) -- alcançável neste ponto
//     precisamente PORQUE o guard `#if GLINTFX_OWN_FONT_ENGINE` acima já provou que esta build
//     conectou aquele include path (ver a chamada set_source_files_properties() do
//     glintfx/CMakeLists.txt).
extern "C" {
#include "core/hint.h"
#include "core/raster.h"
#include "core/sfnt.h"
}

namespace glintfx {

// EN: Concrete Rml::FontEngineInterface implementation backed by SOV-SFNT/SOV-RAST. See this
//     file's header above for the full scope/design writeup. Installed via
//     Rml::SetFontEngineInterface() in Bootstrap::init() (src/bootstrap.cpp), guarded by
//     `#if GLINTFX_OWN_FONT_ENGINE` -- one instance per Bootstrap::Impl, outliving
//     Rml::Shutdown() (same lifetime discipline as Impl::file_iface).
// PT: Implementação concreta de Rml::FontEngineInterface apoiada em SOV-SFNT/SOV-RAST. Ver o
//     cabeçalho deste arquivo acima pro relato completo de escopo/design. Instalada via
//     Rml::SetFontEngineInterface() em Bootstrap::init() (src/bootstrap.cpp), protegida por
//     `#if GLINTFX_OWN_FONT_ENGINE` -- uma instância por Bootstrap::Impl, sobrevivendo ao
//     Rml::Shutdown() (mesma disciplina de lifetime do Impl::file_iface).
class FontEngineOwn final : public Rml::FontEngineInterface {
public:
  FontEngineOwn() = default;
  ~FontEngineOwn() override = default;

  FontEngineOwn(const FontEngineOwn&) = delete;
  FontEngineOwn& operator=(const FontEngineOwn&) = delete;

  // EN: Releases every cached FaceInstance -- critically, its Rml::CallbackTexture member,
  //     whose destructor auto-releases the GPU resource via the RenderManager it was created
  //     from (Rml::UniqueRenderResource's own RAII contract, confirmed reading
  //     UniqueRenderResource.h). MUST be called BEFORE Rml::Shutdown() actually runs -- NOT
  //     relied upon via the FontEngineInterface::Shutdown() override alone (see this method's
  //     .cpp doc-comment for why the base class's own Shutdown() hook fires too LATE for this:
  //     Rml::Shutdown() calls `core_data->contexts.clear()` -- which destroys every Context's
  //     RenderManager/TextureDatabase -- BEFORE it calls `font_interface->Shutdown()`,
  //     confirmed reading the pinned RmlUi source's Core.cpp). src/bootstrap.cpp's
  //     Bootstrap::shutdown() calls this EXPLICITLY, ahead of its own Rml::Shutdown() call, to
  //     guarantee the correct ordering regardless of RmlUi's internal hook timing. Also
  //     overrides the base class's virtual Shutdown() (called by Rml::Shutdown() too, just too
  //     late for the GPU-release ordering above) so it is idempotent either way -- clearing an
  //     already-empty instances_ a second time is a harmless no-op.
  // PT: Libera toda FaceInstance cacheada -- criticamente, seu membro Rml::CallbackTexture,
  //     cujo destrutor auto-libera o recurso GPU via o RenderManager do qual foi criado
  //     (próprio contrato RAII do Rml::UniqueRenderResource, confirmado lendo
  //     UniqueRenderResource.h). PRECISA ser chamado ANTES do Rml::Shutdown() de fato rodar --
  //     NÃO confiado só ao override de FontEngineInterface::Shutdown() (ver o doc-comment .cpp
  //     deste método pro motivo do hook próprio da classe-base disparar TARDE demais pra isto:
  //     Rml::Shutdown() chama `core_data->contexts.clear()` -- que destrói o RenderManager/
  //     TextureDatabase de todo Context -- ANTES de chamar `font_interface->Shutdown()`,
  //     confirmado lendo o Core.cpp do source pinado do RmlUi). O Bootstrap::shutdown() do
  //     src/bootstrap.cpp chama isto EXPLICITAMENTE, antes da própria chamada a
  //     Rml::Shutdown(), pra garantir a ordenação correta independente do timing de hook
  //     interno do RmlUi. Também sobrepõe o Shutdown() virtual da classe-base (também chamado
  //     por Rml::Shutdown(), só tarde demais pra ordenação de liberação GPU acima) para que
  //     seja idempotente de qualquer forma -- limpar um instances_ já-vazio uma 2ª vez é um
  //     no-op inofensivo.
  void Shutdown() override;

  bool LoadFontFace(const Rml::String& file_name, int face_index, bool fallback_face,
                     Rml::Style::FontWeight weight) override;
  bool LoadFontFace(const Rml::String& file_name, int face_index, const Rml::String& family,
                     Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face) override;
  bool LoadFontFace(Rml::Span<const Rml::byte> data, int face_index, const Rml::String& family,
                     Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face) override;

  Rml::FontFaceHandle GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style,
                                         Rml::Style::FontWeight weight, int size) override;

  Rml::FontEffectsHandle PrepareFontEffects(Rml::FontFaceHandle handle,
                                             const Rml::FontEffectList& font_effects) override;

  const Rml::FontMetrics& GetFontMetrics(Rml::FontFaceHandle handle) override;

  int GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string,
                      const Rml::TextShapingContext& text_shaping_context,
                      Rml::Character prior_character = Rml::Character::Null) override;

  int GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle,
                      Rml::FontEffectsHandle font_effects_handle, Rml::StringView string,
                      Rml::Vector2f position, Rml::ColourbPremultiplied colour, float opacity,
                      const Rml::TextShapingContext& text_shaping_context,
                      Rml::TexturedMeshList& mesh_list) override;

  int GetVersion(Rml::FontFaceHandle handle) override;

private:
  // EN: One loaded font FILE (a parsed glx_sfnt_face borrowing read-only pointers into `blob`,
  //     see include/core/sfnt.h's own lifetime contract -- `blob` must outlive every use of
  //     `sfnt`, guaranteed here by storing both in the same struct, `blob` never resized/moved
  //     out after RegisterFace() populates it). Kept in a std::vector<std::unique_ptr<...>>
  //     (faces_, below) rather than std::vector<LoadedFace> so that FaceInstance::face (a raw
  //     pointer into this container) stays valid across future faces_.push_back() reallocations.
  // PT: Um ARQUIVO de fonte carregado (uma glx_sfnt_face parseada emprestando ponteiros
  //     somente-leitura pra dentro de `blob`, ver o próprio contrato de lifetime do
  //     include/core/sfnt.h -- `blob` precisa sobreviver a todo uso de `sfnt`, garantido aqui
  //     guardando os dois na mesma struct, `blob` nunca redimensionado/movido para fora depois
  //     que RegisterFace() o popula). Guardado num std::vector<std::unique_ptr<...>> (faces_,
  //     abaixo) em vez de std::vector<LoadedFace> para que FaceInstance::face (um ponteiro cru
  //     pra dentro deste container) continue válido através de futuros faces_.push_back() que
  //     realocam.
  struct LoadedFace {
    std::vector<uint8_t> blob;
    glx_sfnt_face sfnt{};
    Rml::String family;
    Rml::Style::FontStyle style = Rml::Style::FontStyle::Normal;
    Rml::Style::FontWeight weight = Rml::Style::FontWeight::Normal;
    bool fallback_face = false;
    // EN: Vertical (Y-axis) grid-fitting zones for THIS face, in FONT UNITS -- derived ONCE by
    //     DeriveHintZones() in RegisterFace() (baseline/ascender/descender straight from
    //     head/hhea, x_height/cap_height measured from the 'x'/'H' glyph outlines' own y_max) and
    //     handed to glx_hint_outline() (Layer 0 SOV-HINT) for every glyph BakeFaceInstance()
    //     rasterises for this face. `axis_flags` carries GLX_HINT_AXIS_Y (Y grid-fitting on); a
    //     face whose glyph outlines could not be measured still gets a valid, fraction-of-em
    //     fallback (never a crash) -- see DeriveHintZones() in the .cpp. The X axis is NOT hinted
    //     this sub-phase (vertical stems untouched -- a future, líder-gated sub-phase).
    // PT: Zonas de grid-fitting vertical (eixo Y) desta face, em UNIDADES DE FONTE -- derivadas UMA
    //     vez por DeriveHintZones() em RegisterFace() (baseline/ascender/descender direto de
    //     head/hhea, x_height/cap_height medidos do próprio y_max dos outlines dos glyphs 'x'/'H')
    //     e passadas ao glx_hint_outline() (SOV-HINT da Camada 0) pra todo glyph que
    //     BakeFaceInstance() rasteriza pra esta face. `axis_flags` carrega GLX_HINT_AXIS_Y
    //     (grid-fitting Y ligado); uma face cujos outlines de glyph não puderam ser medidos ainda
    //     recebe um fallback válido de fração-de-em (nunca um crash) -- ver DeriveHintZones() no
    //     .cpp. O eixo X NÃO é hintado nesta sub-fase (hastes verticais intocadas -- uma sub-fase
    //     futura, decidida pelo líder).
    glx_hint_zones hint_zones{};
  };

  // EN: One BAKED glyph within a FaceInstance's atlas. `baked == false` (the default, used as
  //     the "not in this ticket's fixed bake set" sentinel by FindGlyph()) means the codepoint
  //     has no atlas entry at all -- distinct from a real, baked-but-EMPTY glyph (space: `baked
  //     == true`, `w == h == 0`, a valid non-zero advance, deliberately no quad emitted).
  // PT: Um glyph EMPACOTADO dentro do atlas de uma FaceInstance. `baked == false` (o padrão,
  //     usado como sentinela "fora do conjunto fixo empacotado desta tarefa" por FindGlyph())
  //     significa que o codepoint não tem entrada de atlas nenhuma -- distinto de um glyph real,
  //     empacotado-mas-VAZIO (espaço: `baked == true`, `w == h == 0`, um avanço não-zero válido,
  //     deliberadamente sem quad emitido).
  struct GlyphInfo {
    bool baked = false;
    // EN: The glyph id SOV-SFNT resolved for this codepoint at bake time (glx_sfnt_glyph_id()
    //     inside BakeFaceInstance()) -- stashed here so IterateGlyphs() can feed CONSECUTIVE
    //     glyphs' gids to glx_sfnt_kern() without re-resolving cp->gid on every pen advance (the
    //     'kern' table itself is keyed by gid, not codepoint, per OpenType spec).
    // PT: O id de glyph que o SOV-SFNT resolveu pra este codepoint em tempo de empacotamento
    //     (glx_sfnt_glyph_id() dentro de BakeFaceInstance()) -- guardado aqui para que
    //     IterateGlyphs() alimente os gids de glyphs CONSECUTIVOS ao glx_sfnt_kern() sem
    //     re-resolver cp->gid a cada avanço de pena (a própria tabela 'kern' é indexada por gid,
    //     não codepoint, conforme a spec OpenType).
    uint32_t gid = 0;
    int atlas_x = 0, atlas_y = 0, w = 0, h = 0;
    // EN: offset_x/offset_y (px): added to (pen_x, baseline_y) to get the quad's TOP-LEFT
    //     corner in screen space (y-down). See BakeFaceInstance()'s derivation in the .cpp.
    // PT: offset_x/offset_y (px): somados a (pen_x, baseline_y) para obter o canto
    //     SUPERIOR-ESQUERDO do quad em espaço de tela (y-para-baixo). Ver a derivação de
    //     BakeFaceInstance() no .cpp.
    float advance = 0.f, offset_x = 0.f, offset_y = 0.f;
  };

  // EN: One (family, style, weight, pixel-size) resolved instance -- an eagerly-baked glyph
  //     atlas (CPU-side `atlas_rgba` always populated by BakeFaceInstance(); the GPU
  //     `texture` is created lazily, once, on the first GenerateString() call for this
  //     instance -- see this file's header "SCOPE" section for why). Kept in a
  //     std::vector<std::unique_ptr<...>> (instances_, below) for pointer stability: a
  //     Rml::FontFaceHandle IS `reinterpret_cast<uintptr_t>(FaceInstance*)`.
  // PT: Uma instância resolvida de (família, estilo, peso, tamanho-em-px) -- um atlas de glyph
  //     empacotado avidamente (`atlas_rgba` lado-CPU sempre populado por BakeFaceInstance(); a
  //     `texture` GPU é criada preguiçosamente, uma vez, na 1ª chamada de GenerateString() desta
  //     instância -- ver a seção "ESCOPO" do cabeçalho deste arquivo pro motivo). Guardada num
  //     std::vector<std::unique_ptr<...>> (instances_, abaixo) para estabilidade de ponteiro: um
  //     Rml::FontFaceHandle É `reinterpret_cast<uintptr_t>(FaceInstance*)`.
  struct FaceInstance {
    const LoadedFace* face = nullptr;
    int pixel_size = 0;
    Rml::FontMetrics metrics{};
    std::unordered_map<uint32_t, GlyphInfo> glyphs;
    std::vector<uint8_t> atlas_rgba; // RGBA8, premultiplied-white-coverage (see .cpp).
    int atlas_w = 0;
    int atlas_h = 0;
    bool texture_created = false;
    Rml::CallbackTexture texture;
  };

  bool RegisterFace(std::vector<uint8_t>&& blob, int face_index, const Rml::String& family,
                     Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face);
  const LoadedFace* FindBestFace(const Rml::String& family, Rml::Style::FontStyle style,
                                  Rml::Style::FontWeight weight) const;
  void BakeFaceInstance(FaceInstance& inst) const;
  static const GlyphInfo* FindGlyph(const FaceInstance& inst, uint32_t codepoint);

  // EN: Shared UTF-8 walk used by BOTH GetStringWidth() and GenerateString() -- guarantees the
  //     width RmlUi's layout engine measures (GetStringWidth's return) always matches the width
  //     the emitted mesh actually spans (GenerateString's return), since both are the SAME
  //     summation. `emit(pen_x, glyph_or_null)` is called once per decoded codepoint, BEFORE
  //     `pen_x` advances past it -- `glyph_or_null` is `nullptr` for a codepoint outside this
  //     ticket's fixed bake set (see this file's header "SCOPE"). Returns the final pen
  //     position (== total advanced width in px).
  //
  //     KERNING (L1.19-FONTENG/L1.20-FONTFLIP amadurecimento): before `pen_x` advances past a
  //     glyph that has a PREVIOUS baked glyph immediately before it in this same string, the
  //     per-pair adjustment `glx_sfnt_kern(face, prev.gid, cur.gid)` (FUnits) is folded into
  //     `pen_x` FIRST, scaled by the exact same `pixel_size / units_per_em` factor
  //     BakeFaceInstance() already uses for advance/outline -- so kerning lands in the identical
  //     px space as everything else this function accumulates, and (since GetStringWidth() and
  //     GenerateString() share this one summation) the returned width and the emitted quads'
  //     positions can never disagree over whether/how much kerning applied. No kerning is applied
  //     around an UN-baked glyph (`glyph_or_null == nullptr`, no gid to look up) or before the
  //     first glyph of the string (no predecessor) -- both leave the running pen exactly as
  //     GetStringWidth()/GenerateString() already handled the no-kerning case before this
  //     amadurecimento. Rounding -- PEN-SNAP (L1.20-FONTFLIP, FT-F4): with pen-snap ON (the
  //     DEFAULT, own_font_engine_pensnap_bypass()==false), each per-glyph pen COMPONENT is rounded
  //     to a whole pixel BEFORE it is folded in -- the glyph's `advance` and left bearing
  //     (`offset_x`) at bake time (BakeFaceInstance()), the `kern` adjustment and `letter_spacing`
  //     here in the loop -- so `pen` holds an INTEGER value at every `emit()`, exactly mirroring
  //     FreeType (integer horiAdvance/kern/bearing, FreeTypeInterface.cpp's `>> 6`). That lands
  //     every glyph on the SAME sub-pixel phase (the shared frac(position.x), common to both
  //     engines), so the GL_LINEAR atlas no longer smears each glyph's stems across a different
  //     fractional phase -- the sharpness win this ticket is about. INVARIANCE is untouched:
  //     GetStringWidth() and GenerateString() still share THIS one summation, so their totals stay
  //     identical whether or not snapping is on (snapping only quantises the increments both
  //     already share; with snap ON `pen` is already integral, so the final `std::lround` in
  //     GetStringWidth()/GenerateString() is a no-op). Flip own_font_engine_pensnap_bypass() to
  //     `true` to disable it (`pen` stays `float`, the pre-snap behaviour) for the A/B leg.
  // PT: Caminhada UTF-8 compartilhada usada TANTO por GetStringWidth() QUANTO por
  //     GenerateString() -- garante que a largura que o motor de layout do RmlUi mede (retorno
  //     de GetStringWidth) sempre bate com a largura que a mesh emitida de fato ocupa (retorno
  //     de GenerateString), já que ambas são a MESMA soma. `emit(pen_x, glyph_ou_null)` é
  //     chamado uma vez por codepoint decodificado, ANTES de `pen_x` avançar além dele --
  //     `glyph_ou_null` é `nullptr` pra um codepoint fora do conjunto fixo empacotado desta
  //     tarefa (ver a seção "ESCOPO" do cabeçalho deste arquivo). Retorna a posição final da
  //     pena (== largura total avançada em px).
  //
  //     KERNING (amadurecimento L1.19-FONTENG/L1.20-FONTFLIP): antes de `pen_x` avançar além de
  //     um glyph que tem um glyph empacotado ANTERIOR imediatamente antes dele nesta mesma
  //     string, o ajuste por-par `glx_sfnt_kern(face, prev.gid, cur.gid)` (FUnits) é dobrado em
  //     `pen_x` PRIMEIRO, escalado pelo mesmo fator exato `pixel_size / units_per_em` que
  //     BakeFaceInstance() já usa pra avanço/outline -- então o kerning cai no mesmo espaço-px
  //     de tudo mais que esta função acumula, e (já que GetStringWidth() e GenerateString()
  //     compartilham esta única soma) a largura devolvida e a posição dos quads emitidos nunca
  //     podem discordar sobre se/quanto kerning se aplicou. Nenhum kerning se aplica ao redor de
  //     um glyph NÃO-empacotado (`glyph_ou_null == nullptr`, sem gid pra buscar) ou antes do 1º
  //     glyph da string (sem predecessor) -- ambos deixam a pena corrente exatamente como
  //     GetStringWidth()/GenerateString() já tratavam o caso sem-kerning antes deste
  //     amadurecimento. Arredondamento -- PEN-SNAP (L1.20-FONTFLIP, FT-F4): com o pen-snap LIGADO
  //     (o DEFAULT, own_font_engine_pensnap_bypass()==false), cada COMPONENTE do pen por-glyph é
  //     arredondado a um pixel inteiro ANTES de ser dobrado -- o `advance` do glyph e o bearing
  //     esquerdo (`offset_x`) em tempo de bake (BakeFaceInstance()), o ajuste de `kern` e o
  //     `letter_spacing` aqui no laço -- então `pen` guarda um valor INTEIRO em todo `emit()`,
  //     espelhando exatamente o FreeType (horiAdvance/kern/bearing inteiros, o `>> 6` do
  //     FreeTypeInterface.cpp). Isso põe todo glyph na MESMA fase sub-pixel (o frac(position.x)
  //     compartilhado, comum aos dois motores), então o atlas GL_LINEAR não borra mais as hastes de
  //     cada glyph numa fase fracionária diferente -- o ganho de nitidez que esta tarefa persegue.
  //     A INVARIÂNCIA fica intacta: GetStringWidth() e GenerateString() ainda compartilham ESTA
  //     única soma, então os totais continuam idênticos com ou sem o snap (o snap só quantiza os
  //     incrementos que ambos já compartilham; com snap LIGADO o `pen` já é inteiro, então o
  //     `std::lround` final em GetStringWidth()/GenerateString() é um no-op). Vire o
  //     own_font_engine_pensnap_bypass() pra `true` pra desligá-lo (`pen` permanece `float`, o
  //     comportamento pré-snap) na perna A/B.
  static float IterateGlyphs(const FaceInstance& inst, Rml::StringView string, float letter_spacing,
                              const std::function<void(float pen_x, const GlyphInfo*)>& emit);

  std::vector<std::unique_ptr<LoadedFace>> faces_;
  std::vector<std::unique_ptr<FaceInstance>> instances_;
};

// EN: A/B TEST HOOK (L1.20-FONTFLIP, FT-F4) -- a single process-global toggle read exactly once
//     per session, by Bootstrap::init() (src/bootstrap.cpp), guarded there by the same
//     `#if GLINTFX_OWN_FONT_ENGINE` this declaration lives under. When it returns `true`,
//     Bootstrap::init() SKIPS the `Rml::SetFontEngineInterface(&font_engine_own)` call, leaving
//     RmlUi's global `font_interface` pointer null at Rml::Initialise() time -- which makes
//     RmlUi fall back to its built-in FreeType default engine (confirmed reading the pinned
//     RmlUi Source/Core/Core.cpp: Initialise() creates a FontEngineInterfaceDefault only when no
//     interface was set, and Shutdown() resets font_interface back to null, so a subsequent
//     init/shutdown cycle in the SAME process cleanly re-selects whichever engine the toggle
//     asks for). This is the ONE mechanism that lets tests/fonteng_ab_compare.cpp measure the
//     SAME scene through BOTH engines in a SINGLE GLINTFX_OWN_FONT_ENGINE=ON build (no second
//     build, no RmlUi rebuild -- see that file's own header for the full A/B rationale).
//
//     NOT part of glintfx's PUBLIC API: declared only in this src-internal header, and even here
//     only when GLINTFX_OWN_FONT_ENGINE=ON (a build that is OFF by the shipping default, per
//     glintfx/CMakeLists.txt). Default value: `false` -- the own engine is installed as usual,
//     so a normal ON build behaves EXACTLY as before this hook existed; only a test that
//     explicitly flips it observes any difference. Returns a reference to a function-local
//     `static bool` (no static-init-order concern, single-threaded test use only).
// PT: HOOK DE TESTE A/B (L1.20-FONTFLIP, FT-F4) -- um único toggle process-global lido
//     exatamente uma vez por sessão, pelo Bootstrap::init() (src/bootstrap.cpp), protegido lá
//     pelo mesmo `#if GLINTFX_OWN_FONT_ENGINE` sob o qual esta declaração vive. Quando retorna
//     `true`, o Bootstrap::init() PULA a chamada `Rml::SetFontEngineInterface(&font_engine_own)`,
//     deixando o ponteiro global `font_interface` do RmlUi nulo no momento do Rml::Initialise()
//     -- o que faz o RmlUi cair no seu motor FreeType default embutido (confirmado lendo o
//     Source/Core/Core.cpp pinado do RmlUi: o Initialise() cria um FontEngineInterfaceDefault só
//     quando nenhuma interface foi setada, e o Shutdown() reseta font_interface de volta pra
//     nulo, então um ciclo init/shutdown seguinte no MESMO processo re-seleciona limpo qualquer
//     motor que o toggle pedir). É o ÚNICO mecanismo que permite ao tests/fonteng_ab_compare.cpp
//     medir a MESMA cena através dos DOIS motores num ÚNICO build GLINTFX_OWN_FONT_ENGINE=ON (sem
//     segundo build, sem rebuild do RmlUi -- ver o cabeçalho próprio daquele arquivo pro racional
//     A/B completo).
//
//     NÃO faz parte da API PÚBLICA do glintfx: declarado só neste header src-interno, e mesmo
//     aqui só quando GLINTFX_OWN_FONT_ENGINE=ON (um build que é OFF pelo padrão de release, ver
//     glintfx/CMakeLists.txt). Valor default: `false` -- o motor próprio é instalado como de
//     costume, então um build ON normal se comporta EXATAMENTE como antes deste hook existir; só
//     um teste que explicitamente o vira observa alguma diferença. Retorna uma referência a um
//     `static bool` local de função (sem preocupação de ordem de init estática, uso só em teste
//     single-thread).
bool& own_font_engine_ab_bypass();

// EN: Y-HINT BYPASS HOOK (L1.20-FONTFLIP, FT-F4, sub-phase 1.4) -- a second src-internal,
//     process-global toggle, sibling to own_font_engine_ab_bypass() above but orthogonal in
//     meaning. It does NOT choose the engine (that is ab_bypass's job); it chooses whether the
//     OWN engine applies its vertical (Y-axis) grid-fitting. Read once PER GLYPH inside
//     BakeFaceInstance() (font_engine_own.cpp): when it returns `false` (the DEFAULT -- Y hinting
//     is the mature, shipping behaviour of the own engine), glx_hint_outline() snaps each glyph's
//     outline Y to the face's zones before rasterisation; when `true`, that call is SKIPPED and
//     the own engine rasterises the raw, un-hinted outline (the "own, no hint" A/B leg). Only
//     meaningful while the own engine is installed (ab_bypass == false) -- with FreeType installed
//     it is inert (FreeType does its own hinting, this hook never reaches it). Default `false`, so
//     a normal ON build ships WITH Y hinting; only a test that explicitly flips it (e.g.
//     fonteng_ab_visual.cpp's "own_nohint" leg) observes the un-hinted path. Returns a reference
//     to a function-local `static bool` (no static-init-order concern, single-threaded test use).
// PT: HOOK DE BYPASS DO Y-HINT (L1.20-FONTFLIP, FT-F4, sub-fase 1.4) -- um segundo toggle
//     src-interno, process-global, irmão do own_font_engine_ab_bypass() acima mas ortogonal em
//     significado. Ele NÃO escolhe o motor (isso é trabalho do ab_bypass); escolhe se o motor
//     PRÓPRIO aplica seu grid-fitting vertical (eixo Y). Lido uma vez POR GLYPH dentro de
//     BakeFaceInstance() (font_engine_own.cpp): quando retorna `false` (o DEFAULT -- o hinting Y é
//     o comportamento maduro, de release, do motor próprio), o glx_hint_outline() snapa o Y do
//     outline de cada glyph nas zonas da face antes da rasterização; quando `true`, essa chamada é
//     PULADA e o motor próprio rasteriza o outline cru, sem hint (a perna A/B "próprio, sem
//     hint"). Só faz sentido enquanto o motor próprio está instalado (ab_bypass == false) -- com o
//     FreeType instalado é inerte (o FreeType faz seu próprio hinting, este hook nunca o alcança).
//     Default `false`, então um build ON normal já sai COM hinting Y; só um teste que
//     explicitamente o vira (ex.: a perna "own_nohint" do fonteng_ab_visual.cpp) observa o caminho
//     sem-hint. Retorna uma referência a um `static bool` local de função (sem preocupação de
//     ordem de init estática, uso só em teste single-thread).
bool& own_font_engine_hint_bypass();

// EN: STEM-DARKENING ENABLE HOOK (L1.20-FONTFLIP, FT-F4, sub-phase 1.5) -- a third src-internal,
//     process-global toggle, sibling to the two above but orthogonal again, and OPT-IN (default
//     `false`, unlike the two `_bypass` siblings whose default IS the applied behaviour). It does
//     NOT choose the engine (ab_bypass) nor whether Y grid-fitting runs (hint_bypass); it chooses
//     whether the OWN engine applies its per-texel COVERAGE-CURVE stem darkening at atlas-blit time.
//     Read once PER BAKED INSTANCE inside BakeFaceInstance() (font_engine_own.cpp): when it returns
//     `true`, a size-dependent gamma curve `c' = c^γ` (γ<1) is applied to every coverage byte as it
//     is written into the RGBA atlas (fading to identity by ~24px); when `false` (the DEFAULT and
//     the SHIPPING behaviour), the LUT is the identity map -- raw coverage, byte-identical to the
//     pre-1.5 blit.
//
//     ⚠ OPTIONAL MACHINERY, OFF BY DEFAULT -- líder-gated decision (sub-phase 1.5). The cheap
//     coverage-curve darkening was measured (fonteng_ab_visual's measure_oracle()) and DID NOT close
//     the stem-sharpness gap to FreeType -- it is a STRUCTURAL/GEOMETRIC limitation, not a tuning
//     one: a thin vertical stem whose coverage splits ~0.5/0.5 across two pixel columns can only be
//     consolidated into one crisp near-white column by X-AXIS STEM GRID-FITTING (snapping the stem
//     to a column), which a per-coverage curve cannot synthesise. Darkening only adds WEIGHT: it
//     measured `vstem_edge` slightly WORSE (fills the inter-stem valleys with grey haze, flattening
//     the column gradient the metric rewards) while pushing `total_ink` +15-25% at 11-13px, past
//     FreeType's own weight. It is therefore kept as OPT-IN machinery (this hook, + the 4-variant
//     A/B in fonteng_ab_visual.cpp's "own_yhint_dark" leg) rather than shipped on. The residual
//     stem-sharpness gap is left to the X-AXIS STEM GRID-FIT (a future, líder-gated sub-phase).
//
//     Only meaningful while the own engine is installed (ab_bypass==false) -- inert with FreeType
//     (it does its own darkening). Returns a reference to a function-local `static bool` (no
//     static-init-order concern, single-threaded test use).
// PT: HOOK DE ENABLE DO STEM-DARKENING (L1.20-FONTFLIP, FT-F4, sub-fase 1.5) -- um terceiro toggle
//     src-interno, process-global, irmão dos dois acima mas de novo ortogonal, e OPT-IN (default
//     `false`, diferente dos dois irmãos `_bypass` cujo default É o comportamento aplicado). Ele NÃO
//     escolhe o motor (ab_bypass) nem se o grid-fitting Y roda (hint_bypass); escolhe se o motor
//     PRÓPRIO aplica seu stem-darkening por CURVA-DE-COBERTURA por-texel no blit do atlas. Lido uma
//     vez POR INSTÂNCIA empacotada dentro de BakeFaceInstance() (font_engine_own.cpp): quando
//     retorna `true`, uma curva de gamma dependente de tamanho `c' = c^γ` (γ<1) é aplicada a cada
//     byte de cobertura ao ser escrito no atlas RGBA (decaindo pra identidade até ~24px); quando
//     `false` (o DEFAULT e o comportamento de release), a LUT é o mapa identidade -- cobertura crua,
//     byte-idêntica ao blit pré-1.5.
//
//     ⚠ MAQUINARIA OPCIONAL, OFF POR PADRÃO -- decisão do líder (sub-fase 1.5). O darkening barato
//     por curva-de-cobertura foi medido (measure_oracle() do fonteng_ab_visual) e NÃO fechou o gap
//     de nitidez de haste pro FreeType -- é uma limitação ESTRUTURAL/GEOMÉTRICA, não de calibragem:
//     uma haste vertical fina cuja cobertura se divide ~0.5/0.5 entre duas colunas de pixel só
//     consolida numa única coluna nítida quase-branca via GRID-FITTING DE HASTE NO EIXO X (snapando
//     a haste numa coluna), o que uma curva por-cobertura não consegue sintetizar. O darkening só
//     adiciona PESO: mediu `vstem_edge` levemente PIOR (preenche os vales entre hastes com névoa
//     cinza, achatando o gradiente de coluna que a métrica premia) enquanto empurra o `total_ink`
//     +15-25% em 11-13px, além do próprio peso do FreeType. É por isso mantido como maquinaria
//     OPT-IN (este hook, + o A/B de 4 variantes na perna "own_yhint_dark" do fonteng_ab_visual.cpp)
//     em vez de shippado ligado. O gap residual de nitidez de haste fica pro GRID-FIT DE HASTE NO
//     EIXO X (uma sub-fase futura, decidida pelo líder).
//
//     Só faz sentido enquanto o motor próprio está instalado (ab_bypass==false) -- inerte com o
//     FreeType (ele faz o próprio darkening). Retorna uma referência a um `static bool` local de
//     função (sem preocupação de ordem de init estática, uso só em teste single-thread).
bool& own_font_engine_darken_enable();

// EN: PEN-SNAP BYPASS HOOK (L1.20-FONTFLIP, FT-F4) -- a fourth src-internal, process-global toggle,
//     sibling to own_font_engine_hint_bypass() above and orthogonal to all three: it does NOT
//     choose the engine (ab_bypass), nor whether Y grid-fitting runs (hint_bypass), nor whether the
//     coverage curve darkens (darken_enable); it chooses whether the OWN engine ROUNDS each
//     per-glyph pen component to a whole pixel ("pen-snap"). Read in BOTH halves of the shared
//     glyph walk: at bake time in BakeFaceInstance() (font_engine_own.cpp), where the glyph
//     `advance` and left bearing (`offset_x`) are snapped to integers, AND per pen-increment in
//     IterateGlyphs(), where the `kern` adjustment and `letter_spacing` are snapped -- so with the
//     DEFAULT `false` (pen-snap ON, do NOT bypass) the running pen is integral at every glyph,
//     exactly like FreeType (integer horiAdvance/kern/bearing). WHY it matters: the own engine's
//     atlas samples through GL_LINEAR, so a glyph whose origin lands on a fractional sub-pixel phase
//     has its vertical stems smeared across two columns; snapping the pen puts every glyph on the
//     SAME phase (the shared frac(position.x), the identical residual FreeType also carries), which
//     is what restores stem crispness at small body sizes. This is the SHIPPING behaviour (ON by
//     default -- the correct, FreeType-like model); a test flips it to `true` to measure the raw,
//     un-snapped float pen (the "pen float" A/B leg). The width invariance
//     (GetStringWidth()==the mesh's own summed extent) holds either way because the snap lives
//     inside the ONE IterateGlyphs()/bake path both consumers share -- see IterateGlyphs()'s
//     doc-comment above. Only meaningful while the own engine is installed (ab_bypass==false) --
//     inert with FreeType (which does its own integer rounding). Returns a reference to a
//     function-local `static bool` (no static-init-order concern, single-threaded test use).
// PT: HOOK DE BYPASS DO PEN-SNAP (L1.20-FONTFLIP, FT-F4) -- um quarto toggle src-interno,
//     process-global, irmão do own_font_engine_hint_bypass() acima e ortogonal aos três: ele NÃO
//     escolhe o motor (ab_bypass), nem se o grid-fitting Y roda (hint_bypass), nem se a curva de
//     cobertura escurece (darken_enable); escolhe se o motor PRÓPRIO ARREDONDA cada componente do
//     pen por-glyph a um pixel inteiro ("pen-snap"). Lido nas DUAS metades da caminhada de glyph
//     compartilhada: em tempo de bake no BakeFaceInstance() (font_engine_own.cpp), onde o `advance`
//     do glyph e o bearing esquerdo (`offset_x`) são snapados a inteiros, E por incremento-de-pen no
//     IterateGlyphs(), onde o ajuste de `kern` e o `letter_spacing` são snapados -- então com o
//     DEFAULT `false` (pen-snap LIGADO, NÃO bypassar) a pena corrente é inteira em todo glyph,
//     exatamente como o FreeType (horiAdvance/kern/bearing inteiros). POR QUE importa: o atlas do
//     motor próprio amostra via GL_LINEAR, então um glyph cuja origem cai numa fase sub-pixel
//     fracionária tem suas hastes verticais borradas entre duas colunas; snapar o pen põe todo glyph
//     na MESMA fase (o frac(position.x) compartilhado, o resíduo idêntico que o FreeType também
//     carrega), que é o que restaura a nitidez de haste em corpos pequenos. É o comportamento de
//     RELEASE (LIGADO por padrão -- o modelo correto, FreeType-like); um teste o vira pra `true` pra
//     medir o pen float cru, sem-snap (a perna A/B "pen float"). A invariância de largura
//     (GetStringWidth()==a própria extensão somada da mesh) vale de qualquer forma porque o snap
//     vive dentro do ÚNICO caminho IterateGlyphs()/bake que os dois consumidores compartilham -- ver
//     o doc-comment do IterateGlyphs() acima. Só faz sentido enquanto o motor próprio está instalado
//     (ab_bypass==false) -- inerte com o FreeType (que faz seu próprio arredondamento inteiro).
//     Retorna uma referência a um `static bool` local de função (sem preocupação de ordem de init
//     estática, uso só em teste single-thread).
bool& own_font_engine_pensnap_bypass();

// EN: VSNAP BYPASS HOOK (L1.20-FONTFLIP, FT-F4) -- a fifth src-internal, process-global toggle,
//     sibling to own_font_engine_pensnap_bypass() above and orthogonal to all four: it does NOT
//     choose the engine (ab_bypass), nor whether Y grid-fitting runs (hint_bypass), nor whether the
//     coverage curve darkens (darken_enable), nor whether the HORIZONTAL pen is snapped (pensnap);
//     it chooses whether the OWN engine snaps the glyph's VERTICAL origin/offset to a whole pixel
//     ("vsnap"). Read once PER GLYPH in BakeFaceInstance() (font_engine_own.cpp), where the
//     rasteriser origin_y_26_6 and the quad's vertical placement offset_y are both derived from
//     `fy_max = outline.y_max * scale` (a fractional font-unit->pixel value). WHY it matters: the Y
//     grid-fitter glx_hint_outline() (Camada 0, SOV-HINT) already aligns each glyph's horizontal
//     edges (baseline / x-height / cap-height) to whole device pixels IN the outline -- but that
//     work is thrown away downstream if origin_y_26_6 is NOT a multiple of 64, because the
//     already-fitted edge then lands on a FRACTIONAL atlas row and GL_LINEAR smears it back into
//     the texture; and offset_y, if fractional, gives the quad a per-size vertical screen phase
//     that dances as the body size changes. Snapping fy_max to a whole pixel at both points (round
//     fy_max, THEN add/subtract the integer kPad) makes origin_y_26_6 an exact multiple of 64 (the
//     hinted edge lands on an integer atlas row) and puts the quad on a single, size-independent
//     vertical phase off the baseline -- exactly mirroring the pen-snap fix on the X axis. The kPad
//     (1px) absorbs the <=0.5px extremal shift a snap can introduce, so nothing clips (same rationale
//     as the hint). This is the SHIPPING behaviour (ON by default -- the correct model, matching
//     FreeType which snaps its origin/bearing to integers); a test flips it to `true` to measure the
//     raw fractional vertical origin/offset (the "own, no vsnap" A/B leg). The width invariance
//     (GetStringWidth()==the mesh's own summed extent) is UNAFFECTED -- vsnap is a pure Y-axis
//     quantisation, it touches neither advance nor bearing nor any horizontal metric. Only
//     meaningful while the own engine is installed (ab_bypass==false) -- inert with FreeType (which
//     does its own integer origin rounding). Returns a reference to a function-local `static bool`
//     (no static-init-order concern, single-threaded test use).
// PT: HOOK DE BYPASS DO VSNAP (L1.20-FONTFLIP, FT-F4) -- um quinto toggle src-interno,
//     process-global, irmão do own_font_engine_pensnap_bypass() acima e ortogonal aos quatro: ele
//     NÃO escolhe o motor (ab_bypass), nem se o grid-fitting Y roda (hint_bypass), nem se a curva de
//     cobertura escurece (darken_enable), nem se o pen HORIZONTAL é snapado (pensnap); escolhe se o
//     motor PRÓPRIO snapa a origem/offset VERTICAL do glyph a um pixel inteiro ("vsnap"). Lido uma
//     vez POR GLYPH no BakeFaceInstance() (font_engine_own.cpp), onde a origem do rasterizador
//     origin_y_26_6 e o offset de posicionamento vertical do quad offset_y são ambos derivados de
//     `fy_max = outline.y_max * scale` (um valor fracionário unidade-de-fonte->pixel). POR QUE
//     importa: o grid-fitter Y glx_hint_outline() (Camada 0, SOV-HINT) já alinha as arestas
//     horizontais de cada glyph (baseline / altura-x / altura-de-maiúscula) a pixels device inteiros
//     DENTRO do outline -- mas esse trabalho é jogado fora rio-abaixo se origin_y_26_6 NÃO for
//     múltiplo de 64, porque a aresta já fitada pousa então numa row FRACIONÁRIA do atlas e o
//     GL_LINEAR a borra de volta na textura; e o offset_y, se fracionário, dá ao quad uma fase de
//     tela vertical por-tamanho que dança conforme o corpo muda. Snapar fy_max a um pixel inteiro nos
//     dois pontos (arredondar fy_max, DEPOIS somar/subtrair o kPad inteiro) faz origin_y_26_6 um
//     múltiplo exato de 64 (a aresta hintada pousa numa row inteira do atlas) e põe o quad numa fase
//     vertical única, independente de tamanho, a partir da baseline -- espelhando exatamente o
//     pen-snap no eixo X. O kPad (1px) absorve o deslocamento extremal de <=0.5px que um snap pode
//     introduzir, então nada corta (mesmo racional do hint). É o comportamento de RELEASE (LIGADO por
//     padrão -- o modelo correto, igual ao FreeType que snapa sua origem/bearing a inteiros); um
//     teste o vira pra `true` pra medir a origem/offset vertical fracionária crua (a perna A/B
//     "próprio, sem vsnap"). A invariância de largura (GetStringWidth()==a própria extensão somada da
//     mesh) NÃO é afetada -- vsnap é uma quantização de eixo Y pura, não toca advance nem bearing nem
//     nenhuma métrica horizontal. Só faz sentido enquanto o motor próprio está instalado
//     (ab_bypass==false) -- inerte com o FreeType (que faz seu próprio arredondamento inteiro de
//     origem). Retorna uma referência a um `static bool` local de função (sem preocupação de ordem de
//     init estática, uso só em teste single-thread).
bool& own_font_engine_vsnap_bypass();

} // namespace glintfx

#endif // GLINTFX_OWN_FONT_ENGINE
