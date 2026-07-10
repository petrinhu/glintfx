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
//     GetFontFaceHandle() resolves it -- a FIXED codepoint set (printable ASCII U+0020..U+007E
//     plus the pt-br-accented Latin-1 Supplement codepoints á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ and their
//     uppercase forms, see BuildBakeSet() in the .cpp). There is NO lazy/incremental atlas growth
//     after that: a codepoint outside the baked set silently has no glyph (advance 0, no quad --
//     the string still measures/renders, just missing that one character). This was chosen over
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
//     Kerning (kern/GPOS) is 0 (SOV-SFNT itself does not parse those tables, by its own documented
//     scope) and letter-spacing (TextShapingContext::letter_spacing) is honoured. No font
//     collection (.ttc) support -- `face_index` is accepted but ignored (SOV-SFNT parses a single
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
//     imprimível U+0020..U+007E mais os codepoints do Latin-1 Supplement acentuados pt-br
//     á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ e suas formas maiúsculas, ver BuildBakeSet() no .cpp). NÃO há
//     crescimento de atlas preguiçoso/incremental depois disso: um codepoint fora do conjunto
//     empacotado silenciosamente não tem glyph (avanço 0, sem quad -- a string ainda mede/
//     renderiza, só falta aquele caractere). Isso foi escolhido em vez de empacotamento
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
//     efeito `0`, o texto ainda renderiza sem o efeito). Kerning (kern/GPOS) é 0 (o próprio
//     SOV-SFNT não parseia essas tabelas, por escopo documentado próprio) e letter-spacing
//     (TextShapingContext::letter_spacing) é honrado. Sem suporte a coleção de fonte (.ttc) --
//     `face_index` é aceito mas ignorado (SOV-SFNT parseia um único diretório sfnt no offset 0 do
//     blob, batendo com o próprio escopo documentado dele). Sem parsing de tabela 'name' -- a
//     sobrecarga sem-família de LoadFontFace() (sem ponto de chamada confirmado no RmlUi: o RCSS
//     @font-face sempre fornece família/estilo/peso explicitamente, confirmado lendo o
//     StyleSheetParser::ParseFontFaceBlock do source pinado do RmlUi) deriva uma família do
//     próprio stem do nome de arquivo como fallback de melhor esforço.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

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
//     GLINTFX_OWN_FONT_ENGINE=ON (glintfx/CMakeLists.txt) -- this header is therefore only ever
//     #included by glintfx source files themselves guarded by `#if GLINTFX_OWN_FONT_ENGINE`
//     (see src/bootstrap.cpp), never unconditionally.
// PT: C soberano da Camada 0, linkado via `extern "C"` conforme o padrão "casca-hosted + carne-C"
//     da ADR-0009 -- os algoritmos reais de parsing SFNT/rasterização de outline moram aqui, NÃO
//     neste arquivo. `include/core/` só é adicionado ao include path desta unidade de tradução
//     quando GLINTFX_OWN_FONT_ENGINE=ON (glintfx/CMakeLists.txt) -- este header portanto só é
//     #incluído por arquivos-fonte do próprio glintfx protegidos por
//     `#if GLINTFX_OWN_FONT_ENGINE` (ver src/bootstrap.cpp), nunca incondicionalmente.
extern "C" {
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
  // PT: Caminhada UTF-8 compartilhada usada TANTO por GetStringWidth() QUANTO por
  //     GenerateString() -- garante que a largura que o motor de layout do RmlUi mede (retorno
  //     de GetStringWidth) sempre bate com a largura que a mesh emitida de fato ocupa (retorno
  //     de GenerateString), já que ambas são a MESMA soma. `emit(pen_x, glyph_ou_null)` é
  //     chamado uma vez por codepoint decodificado, ANTES de `pen_x` avançar além dele --
  //     `glyph_ou_null` é `nullptr` pra um codepoint fora do conjunto fixo empacotado desta
  //     tarefa (ver a seção "ESCOPO" do cabeçalho deste arquivo). Retorna a posição final da
  //     pena (== largura total avançada em px).
  static float IterateGlyphs(const FaceInstance& inst, Rml::StringView string, float letter_spacing,
                              const std::function<void(float pen_x, const GlyphInfo*)>& emit);

  std::vector<std::unique_ptr<LoadedFace>> faces_;
  std::vector<std::unique_ptr<FaceInstance>> instances_;
};

} // namespace glintfx
