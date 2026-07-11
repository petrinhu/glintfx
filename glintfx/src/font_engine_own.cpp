// SPDX-License-Identifier: MPL-2.0
// EN: FontEngineOwn implementation. See font_engine_own.hpp for the full clean-room/scope
//     writeup -- this file only adds the per-function derivations not already covered there.
// PT: Implementação do FontEngineOwn. Ver font_engine_own.hpp pro relato completo de
//     clean-room/escopo -- este arquivo só acrescenta as derivações por-função ainda não
//     cobertas lá.
// Copyright (c) 2026 Petrus Silva Costa
#include "font_engine_own.hpp"

// EN: font_engine_own.hpp (above) pulls in "glintfx/config.hpp" unconditionally, first thing,
//     outside any guard -- GLINTFX_OWN_FONT_ENGINE is therefore always defined (0 or 1, never
//     undefined, `#cmakedefine01`) by this point. When OFF, everything below -- the entire body
//     of this translation unit -- compiles away to nothing: this .cpp exists ONLY to implement
//     the glintfx::FontEngineOwn class the header declares under its OWN `#if
//     GLINTFX_OWN_FONT_ENGINE` guard, so there is nothing left to define when that class itself
//     is not declared. This keeps the file self-guarding regardless of who globs/compiles it
//     (see the .hpp's own top-of-file comment for why that matters to clang-tidy's CI lint pass,
//     which globs every glintfx/src/*.cpp file unconditionally, independent of
//     glintfx/CMakeLists.txt's `if(GLINTFX_OWN_FONT_ENGINE)` build-target gate).
// PT: font_engine_own.hpp (acima) traz "glintfx/config.hpp" incondicionalmente, primeira coisa,
//     fora de qualquer guard -- GLINTFX_OWN_FONT_ENGINE portanto está sempre definida (0 ou 1,
//     nunca indefinida, `#cmakedefine01`) neste ponto. Quando OFF, tudo abaixo -- o corpo
//     inteiro desta unidade de tradução -- compila pra nada: este .cpp existe SÓ para implementar
//     a classe glintfx::FontEngineOwn que o header declara sob o PRÓPRIO guard `#if
//     GLINTFX_OWN_FONT_ENGINE`, então não sobra nada pra definir quando aquela classe em si não
//     é declarada. Isso mantém o arquivo auto-protegido independente de quem o globa/compila
//     (ver o comentário de topo-de-arquivo do próprio .hpp pro motivo disso importar pro passe
//     de lint do CI via clang-tidy, que varre todo arquivo glintfx/src/*.cpp
//     incondicionalmente, independente do gate de alvo-de-build `if(GLINTFX_OWN_FONT_ENGINE)`
//     do glintfx/CMakeLists.txt).
#if GLINTFX_OWN_FONT_ENGINE

#include <algorithm>
#include <cmath>

namespace glintfx {

// EN: A/B TEST HOOK storage (L1.20-FONTFLIP, FT-F4). See the long doc-comment on this function's
//     declaration in font_engine_own.hpp for the full contract. Function-local static so the
//     `false` default is established on first use with no static-init-order dependency; a
//     reference is returned so a test can flip it (`glintfx::own_font_engine_ab_bypass() = true;`)
//     before constructing the App/UiLayer whose Bootstrap::init() will read it. Lives in this
//     translation unit (compiled ONLY when GLINTFX_OWN_FONT_ENGINE=ON) so it never adds a symbol
//     to a default OFF build.
// PT: Armazenamento do HOOK DE TESTE A/B (L1.20-FONTFLIP, FT-F4). Ver o doc-comment longo na
//     declaração desta função em font_engine_own.hpp pro contrato completo. static local de
//     função para o default `false` ser estabelecido no primeiro uso sem dependência de ordem de
//     init estática; uma referência é retornada para um teste poder virá-lo
//     (`glintfx::own_font_engine_ab_bypass() = true;`) antes de construir o App/UiLayer cujo
//     Bootstrap::init() o lerá. Vive nesta unidade de tradução (compilada SÓ quando
//     GLINTFX_OWN_FONT_ENGINE=ON) para nunca adicionar um símbolo a um build OFF padrão.
bool& own_font_engine_ab_bypass() {
  static bool bypass = false;
  return bypass;
}

namespace {

// EN: The fixed bake set (see font_engine_own.hpp's "SCOPE" section). Still an EAGER, one-shot,
//     FIXED set -- this ticket (L1.20-FONTFLIP sub-phase 2.1) only WIDENED it for general-purpose
//     distribution (the product ships worldwide, not just pt-br), it did NOT change the eager
//     fixed-set architecture (lazy/on-demand bake is a separately-decided future sub-phase). Three
//     bands, all deduped by construction (no codepoint appears twice):
//       1. Printable ASCII                U+0020..U+007E (95 cp) -- unchanged.
//       2. Latin-1 Supplement printable   U+00A0..U+00FF (96 cp) -- SUPERSETS the old hand-picked
//          pt-br list (á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ + uppercase all live in U+00C0..U+00FF), so
//          nothing regresses; it also picks up the previously-MISSING î (U+00EE) plus the full
//          Western-European accent coverage and the Latin-1 symbols (© ° ± × ÷ µ ¿ ¡ £ ¢ ¥ § ...).
//          U+00A0 is the non-breaking space; U+00AD is the soft hyphen -- both harmless (a glyph
//          the face lacks bakes to nothing, see the gid==0 guard in BakeFaceInstance()).
//       3. Common typographic punctuation (General Punctuation block) that the previously-fixed
//          Latin-1-only set silently dropped: em-dash/en-dash, the curved single+double quotes
//          (the last of which doubles as the typographic apostrophe U+2019), the horizontal
//          ellipsis, and the bullet. These were the other half of the "î and — vanish" report.
//     A codepoint the FACE does not actually contain (glyph id 0 / .notdef) is skipped in
//     BakeFaceInstance() rather than baked, so listing a codepoint here that Open Sans happens to
//     lack degrades cleanly (no .notdef box polluting the atlas) -- listing is a request, not a
//     guarantee the face satisfies it.
//     A plain function (not a file-scope static container) so it has no static-init-order concerns
//     and is trivially re-callable from BakeFaceInstance() without exposing a mutable global.
// PT: O conjunto fixo empacotado (ver a seção "ESCOPO" de font_engine_own.hpp). Continua um
//     conjunto ÁVIDO, único, FIXO -- esta tarefa (sub-fase 2.1 do L1.20-FONTFLIP) só o AMPLIOU pra
//     distribuição geral (o produto vai pro mundo, não só pt-br), NÃO mudou a arquitetura de
//     conjunto-fixo-ávido (bake preguiçoso/sob-demanda é uma sub-fase futura decidida à parte).
//     Três faixas, todas sem duplicata por construção (nenhum codepoint aparece duas vezes):
//       1. ASCII imprimível              U+0020..U+007E (95 cp) -- inalterado.
//       2. Latin-1 Supplement imprimível U+00A0..U+00FF (96 cp) -- SUPERCONJUNTO da antiga lista
//          pt-br escolhida a dedo (á/à/â/ã/ç/é/ê/í/ó/ô/õ/ú/ü/ñ + maiúsculas todos vivem em
//          U+00C0..U+00FF), então nada regride; também apanha o î (U+00EE) que FALTAVA mais a
//          cobertura completa de acentos da Europa Ocidental e os símbolos Latin-1
//          (© ° ± × ÷ µ ¿ ¡ £ ¢ ¥ § ...). U+00A0 é o espaço não-quebrável; U+00AD é o hífen suave
//          -- ambos inofensivos (um glyph que a face não tem empacota pra nada, ver a guarda
//          gid==0 em BakeFaceInstance()).
//       3. Pontuação tipográfica comum (bloco General Punctuation) que o conjunto fixo antigo
//          só-Latin-1 largava em silêncio: em-dash/en-dash, as aspas curvas simples+duplas (a
//          última das quais também é o apóstrofo tipográfico U+2019), as reticências horizontais e
//          o bullet. Eram a outra metade do relato "î e — somem".
//     Um codepoint que a FACE de fato não contém (glyph id 0 / .notdef) é pulado em
//     BakeFaceInstance() em vez de empacotado, então listar aqui um codepoint que a Open Sans por
//     acaso não tenha degrada limpo (sem caixa .notdef poluindo o atlas) -- listar é um pedido,
//     não garantia de que a face o satisfaça.
//     Uma função simples (não um container static de escopo de arquivo) para não ter preocupação
//     de ordem de inicialização estática e ser trivialmente re-chamável a partir de
//     BakeFaceInstance() sem expor um global mutável.
std::vector<uint32_t> BuildBakeSet() {
  std::vector<uint32_t> set;
  set.reserve(95 + 96 + 12);
  // 1. Printable ASCII / ASCII imprimível.
  for (uint32_t cp = 0x20; cp <= 0x7E; ++cp) set.push_back(cp);
  // 2. Latin-1 Supplement printable / Latin-1 Supplement imprimível (includes all old pt-br + î).
  for (uint32_t cp = 0xA0; cp <= 0xFF; ++cp) set.push_back(cp);
  // 3. Common typographic punctuation / Pontuação tipográfica comum.
  static const uint32_t kTypographic[] = {
      0x2013, // – en-dash
      0x2014, // — em-dash
      0x2018, // ' left single quote
      0x2019, // ' right single quote / typographic apostrophe
      0x201C, // " left double quote
      0x201D, // " right double quote
      0x2022, // • bullet
      0x2026, // … horizontal ellipsis
  };
  for (uint32_t cp : kTypographic) set.push_back(cp);
  return set;
}

// EN: Generous fixed caps for glx_sfnt_glyph_outline()'s caller-owned buffers -- Open Sans'
//     own letter glyphs run well under 100 points/10 contours; the pt-br accented composites
//     (base letter + accent mark, ONE level deep per SOV-SFNT's own header) stay comfortably
//     within these. A glyph that legitimately needs more (GLX_SFNT_ERR_BUFFER_TOO_SMALL) is
//     simply left un-baked (see BakeFaceInstance()) rather than growing the buffer dynamically
//     -- consistent with this ticket's eager, one-shot, fixed-set bake design.
// PT: Tetos fixos generosos para os buffers possuídos-por-quem-chama do
//     glx_sfnt_glyph_outline() -- os próprios glyphs de letra da Open Sans ficam bem abaixo de
//     100 pontos/10 contornos; os compostos acentuados pt-br (letra base + acento, UM nível de
//     profundidade conforme o próprio cabeçalho do SOV-SFNT) cabem confortavelmente aqui. Um
//     glyph que legitimamente precisa de mais (GLX_SFNT_ERR_BUFFER_TOO_SMALL) simplesmente fica
//     não-empacotado (ver BakeFaceInstance()) em vez de o buffer crescer dinamicamente --
//     consistente com o design de empacotamento ávido, único, de conjunto fixo desta tarefa.
constexpr uint16_t kMaxOutlinePoints = 768;
constexpr uint16_t kMaxOutlineContours = 48;

} // namespace

void FontEngineOwn::Shutdown() {
  // EN: Dropping instances_ destroys every FaceInstance, including its Rml::CallbackTexture
  //     member -- Rml::UniqueRenderResource's destructor auto-releases the GPU resource via
  //     the RenderManager it was created from, PROVIDED that RenderManager (owned by an
  //     Rml::Context) is still alive at this point. See this method's own doc-comment in
  //     font_engine_own.hpp for why src/bootstrap.cpp's Bootstrap::shutdown() calls this
  //     EXPLICITLY before Rml::Shutdown() -- Rml::Shutdown() itself calls
  //     `core_data->contexts.clear()` (destroying every Context's RenderManager) BEFORE it
  //     calls `font_interface->Shutdown()` (confirmed reading the pinned RmlUi source's
  //     Core.cpp), so relying on THIS override alone, invoked only via that internal
  //     mechanism, would run too late -- the "Leaking CallbackTexture detected... will likely
  //     result in memory corruption" RMLUI_ERROR (TextureDatabase.cpp) plus a hard crash on
  //     shutdown is exactly what was observed before this explicit-early-call fix.
  //     faces_ (CPU-only blobs, no GPU resource) is cleared too for a clean, symmetric full
  //     reset -- harmless either way.
  // PT: Descartar instances_ destrói toda FaceInstance, inclusive seu membro
  //     Rml::CallbackTexture -- o destrutor do Rml::UniqueRenderResource auto-libera o
  //     recurso GPU via o RenderManager do qual foi criado, DESDE QUE aquele RenderManager
  //     (possuído por um Rml::Context) ainda esteja vivo neste ponto. Ver o doc-comment
  //     próprio deste método em font_engine_own.hpp pro motivo do Bootstrap::shutdown() do
  //     src/bootstrap.cpp chamar isto EXPLICITAMENTE antes do Rml::Shutdown() -- o próprio
  //     Rml::Shutdown() chama `core_data->contexts.clear()` (destruindo o RenderManager de
  //     todo Context) ANTES de chamar `font_interface->Shutdown()` (confirmado lendo o
  //     Core.cpp do source pinado do RmlUi), então confiar só neste override, invocado só via
  //     aquele mecanismo interno, rodaria tarde demais -- o RMLUI_ERROR "Leaking
  //     CallbackTexture detected... will likely result in memory corruption"
  //     (TextureDatabase.cpp) mais um crash duro no shutdown é exatamente o que foi observado
  //     antes deste fix de chamada-explícita-antecipada. faces_ (blobs só-CPU, sem recurso
  //     GPU) também é limpo, por um reset completo limpo e simétrico -- inofensivo de
  //     qualquer forma.
  instances_.clear();
  faces_.clear();
}

bool FontEngineOwn::RegisterFace(std::vector<uint8_t>&& blob, int /*face_index*/, const Rml::String& family,
                                  Rml::Style::FontStyle style, Rml::Style::FontWeight weight,
                                  bool fallback_face) {
  // EN: face_index (TrueType-collection sub-face selector) is accepted for interface parity
  //     but ignored -- SOV-SFNT parses a single sfnt table directory at blob offset 0 (no .ttc
  //     support, by its own documented scope). Every face this ticket targets (Open Sans) is a
  //     plain, non-collection .ttf, so this is a documented limitation, not a silent bug.
  // PT: face_index (seletor de sub-face de coleção TrueType) é aceito por paridade de
  //     interface mas ignorado -- o SOV-SFNT parseia um único diretório de tabela sfnt no
  //     offset 0 do blob (sem suporte a .ttc, por escopo documentado próprio). Toda face que
  //     esta tarefa mira (Open Sans) é um .ttf comum, não-coleção, então isto é uma limitação
  //     documentada, não um bug silencioso.
  if (blob.empty()) return false;

  auto lf = std::make_unique<LoadedFace>();
  lf->blob = std::move(blob);
  if (glx_sfnt_open(lf->blob.data(), lf->blob.size(), &lf->sfnt) != GLX_SFNT_OK) return false;
  if (lf->sfnt.units_per_em == 0) return false; // guards the scale-factor division in BakeFaceInstance().

  lf->family = family;
  lf->style = style;
  // EN: Style::FontWeight::Auto (0) is not a real weight to MATCH against later -- normalise to
  //     Normal (400), RmlUi's own "no weight specified" convention (StyleTypes.h comment: "Any
  //     definite value in the range [1,1000] is valid").
  // PT: Style::FontWeight::Auto (0) não é um peso real pra COMPARAR depois -- normaliza para
  //     Normal (400), a própria convenção do RmlUi pra "nenhum peso especificado" (comentário
  //     de StyleTypes.h: "Any definite value in the range [1,1000] is valid").
  lf->weight = (weight == Rml::Style::FontWeight::Auto) ? Rml::Style::FontWeight::Normal : weight;
  lf->fallback_face = fallback_face;

  faces_.push_back(std::move(lf));
  return true;
}

bool FontEngineOwn::LoadFontFace(const Rml::String& file_name, int face_index, bool fallback_face,
                                  Rml::Style::FontWeight weight) {
  // EN: Family-less overload -- no confirmed RmlUi call site (see this file's header "SCOPE");
  //     derive a best-effort family from the file name's own stem (SOV-SFNT does not parse the
  //     'name' table). Kept for API completeness / direct Rml::LoadFontFace(path, ...) callers.
  // PT: Sobrecarga sem-família -- sem ponto de chamada confirmado no RmlUi (ver a seção
  //     "ESCOPO" do cabeçalho deste arquivo); deriva uma família de melhor esforço do próprio
  //     stem do nome de arquivo (o SOV-SFNT não parseia a tabela 'name'). Mantida por
  //     completude de API / chamadores diretos de Rml::LoadFontFace(path, ...).
  Rml::String data;
  if (!Rml::GetFileInterface() || !Rml::GetFileInterface()->LoadFile(file_name, data)) return false;

  const size_t slash = file_name.find_last_of("/\\");
  const size_t start = (slash == Rml::String::npos) ? 0 : slash + 1;
  const size_t dot = file_name.find_last_of('.');
  const size_t len = (dot == Rml::String::npos || dot < start) ? Rml::String::npos : dot - start;
  const Rml::String family = file_name.substr(start, len);

  std::vector<uint8_t> blob(data.begin(), data.end());
  return RegisterFace(std::move(blob), face_index, family, Rml::Style::FontStyle::Normal, weight, fallback_face);
}

bool FontEngineOwn::LoadFontFace(const Rml::String& file_name, int face_index, const Rml::String& family,
                                  Rml::Style::FontStyle style, Rml::Style::FontWeight weight,
                                  bool fallback_face) {
  // EN: The overload actually invoked by RCSS @font-face blocks -- confirmed by reading the
  //     pinned RmlUi source's StyleSheetParser::ParseFontFaceBlock (Rml::LoadFontFace(src,
  //     family, style, weight, is_fallback, face_index), which resolves to this exact
  //     FontEngineInterface signature per Core.cpp). Uses Rml::GetFileInterface() (NOT a bare
  //     std::ifstream) so glintfx's own BaseUrlFileInterface (asset_base_url prefixing,
  //     src/base_url_file_interface.hpp) is honoured exactly like every other resource load in
  //     this codebase -- FileInterface abstraction is a generic RmlUi convention (also used by
  //     texture loading), not FreeType-specific.
  // PT: A sobrecarga de fato invocada por blocos RCSS @font-face -- confirmado lendo o
  //     ParseFontFaceBlock do StyleSheetParser do source pinado do RmlUi (Rml::LoadFontFace(src,
  //     family, style, weight, is_fallback, face_index), que resolve para esta assinatura exata
  //     da FontEngineInterface conforme o Core.cpp). Usa Rml::GetFileInterface() (NÃO um
  //     std::ifstream cru) para que o BaseUrlFileInterface próprio do glintfx (prefixo de
  //     asset_base_url, src/base_url_file_interface.hpp) seja honrado exatamente como todo outro
  //     carregamento de recurso neste código-base -- a abstração FileInterface é uma convenção
  //     genérica do RmlUi (também usada pelo carregamento de textura), não específica do FreeType.
  Rml::String data;
  if (!Rml::GetFileInterface() || !Rml::GetFileInterface()->LoadFile(file_name, data)) return false;
  std::vector<uint8_t> blob(data.begin(), data.end());
  return RegisterFace(std::move(blob), face_index, family, style, weight, fallback_face);
}

bool FontEngineOwn::LoadFontFace(Rml::Span<const Rml::byte> data, int face_index, const Rml::String& family,
                                  Rml::Style::FontStyle style, Rml::Style::FontWeight weight,
                                  bool fallback_face) {
  // EN: Memory-blob overload -- e.g. RmlUi's debugger plugin loads its embedded font this way.
  //     Rml::byte == unsigned char, matching glx_sfnt_open's uint8_t blob exactly.
  // PT: Sobrecarga de blob-em-memória -- ex.: o plugin debugger do RmlUi carrega sua fonte
  //     embutida deste jeito. Rml::byte == unsigned char, batendo exatamente com o blob
  //     uint8_t do glx_sfnt_open.
  std::vector<uint8_t> blob(data.begin(), data.end());
  return RegisterFace(std::move(blob), face_index, family, style, weight, fallback_face);
}

const FontEngineOwn::LoadedFace* FontEngineOwn::FindBestFace(const Rml::String& family,
                                                               Rml::Style::FontStyle style,
                                                               Rml::Style::FontWeight weight) const {
  const LoadedFace* best = nullptr;
  int best_score = -1;
  for (const auto& f : faces_) {
    if (f->family != family) continue; // family is a hard requirement among family-matching candidates.
    int score = 100;
    if (f->style == style) score += 10;
    if (f->weight == weight) score += 5;
    if (score > best_score) {
      best_score = score;
      best = f.get();
    }
  }
  if (best) return best;

  // EN: No family match -- fall back to a face explicitly registered as a fallback face
  //     ("fallback_face=true" in the @font-face block, RmlUi's own convention for "use this
  //     face for unknown characters in other faces"), else the first loaded face (better to
  //     show SOMETHING than nothing when a document names an unregistered family).
  // PT: Sem match de família -- cai numa face explicitamente registrada como fallback
  //     ("fallback_face=true" no bloco @font-face, convenção própria do RmlUi para "usar esta
  //     face pra caracteres desconhecidos em outras faces"), senão a 1ª face carregada (melhor
  //     mostrar ALGO do que nada quando um documento nomeia uma família não registrada).
  for (const auto& f : faces_)
    if (f->fallback_face) return f.get();
  if (!faces_.empty()) return faces_.front().get();
  return nullptr;
}

void FontEngineOwn::BakeFaceInstance(FaceInstance& inst) const {
  const glx_sfnt_face& sf = inst.face->sfnt;
  const float scale = (float)inst.pixel_size / (float)sf.units_per_em; // caller already guards units_per_em>0.

  // EN: FontMetrics derivation from head/hhea (font units) scaled to this instance's pixel
  //     size. `descender` is negative in TrueType's own convention (below the baseline);
  //     FontMetrics::descent wants a POSITIVE distance below the baseline, hence the negation.
  //     x_height is refined below to the RASTERIZED 'x' glyph's own bbox height when 'x' bakes
  //     successfully (more accurate than the ascent*0.5 placeholder -- SOV-SFNT does not expose
  //     the OS/2 table's sxHeight field, out of its own documented scope).
  //     has_ellipsis starts false and is flipped true below ONLY if U+2026 actually rasterises for
  //     this face (it is now in the widened bake set, sub-phase 2.1) -- gating on the real bake,
  //     not mere set membership, keeps it honest for a face that happens to lack the glyph (then
  //     RmlUi falls back to its own "..." three-dot construction for text-overflow:ellipsis).
  // PT: Derivação de FontMetrics a partir de head/hhea (unidades de fonte) escaladas pro
  //     tamanho-em-px desta instância. `descender` é negativo na própria convenção do TrueType
  //     (abaixo da linha de base); FontMetrics::descent quer uma distância POSITIVA abaixo da
  //     linha de base, daí a negação. x_height é refinado abaixo pra própria altura de bbox do
  //     glyph 'x' RASTERIZADO quando 'x' empacota com sucesso (mais preciso que o placeholder
  //     ascent*0.5 -- o SOV-SFNT não expõe o campo sxHeight da tabela OS/2, fora do próprio
  //     escopo documentado). has_ellipsis é deliberadamente false: U+2026 não está no conjunto
  //     fixo empacotado desta tarefa (ver a seção "ESCOPO" do cabeçalho deste arquivo) -- o
  //     RmlUi cai pra própria construção de três-pontos "..." pra text-overflow:ellipsis, o que
  //     não é exercitado pelo teste de aceite desta tarefa.
  Rml::FontMetrics m{};
  m.size = inst.pixel_size;
  m.ascent = (float)sf.ascender * scale;
  m.descent = std::max(0.0f, -(float)sf.descender * scale);
  m.line_spacing = ((float)sf.ascender - (float)sf.descender + (float)sf.line_gap) * scale;
  m.x_height = m.ascent * 0.5f;
  m.underline_position = m.descent * 0.5f;
  m.underline_thickness = std::max(1.0f, (float)inst.pixel_size / 14.0f);
  m.has_ellipsis = false;
  inst.metrics = m;

  struct Baked {
    uint32_t cp = 0;
    uint32_t gid = 0; // for glx_sfnt_kern() -- the 'kern' table is keyed by gid, not codepoint.
    int gw = 0, gh = 0;
    std::vector<uint8_t> coverage; // gw*gh coverage bytes, empty for a blank/un-rasterized glyph.
    float advance = 0.f, offset_x = 0.f, offset_y = 0.f;
    int atlas_x = 0, atlas_y = 0; // filled by the shelf-pack pass below.
  };
  std::vector<Baked> baked;
  baked.reserve(128);

  std::vector<glx_sfnt_point> points(kMaxOutlinePoints);
  std::vector<uint16_t> contour_ends(kMaxOutlineContours);

  for (uint32_t cp : BuildBakeSet()) {
    const uint32_t gid = glx_sfnt_glyph_id(&sf, cp);

    // EN: gid 0 is .notdef -- glx_sfnt_glyph_id() returns it for any codepoint the face's cmap
    //     does not map (include/core/sfnt.h's documented convention). With the widened bake set
    //     (Latin-1 Supplement + typographic punctuation) some listed codepoints may legitimately
    //     be absent from a given face, so skip them here rather than baking .notdef's own box
    //     outline under a real codepoint -- that would pollute the atlas with a visible tofu box
    //     and make FindGlyph() return a bogus quad for a character the font never had. Skipping
    //     leaves the codepoint out of inst.glyphs entirely: FindGlyph() returns nullptr, the
    //     string still measures/renders, that one character is simply blank (advance 0, no quad) --
    //     the exact "codepoint outside the baked set" degradation the header's SCOPE promises. cp 0
    //     is never in BuildBakeSet(), so this never wrongly drops a real glyph mapped to gid 0.
    // PT: gid 0 é .notdef -- glx_sfnt_glyph_id() o retorna pra qualquer codepoint que o cmap da
    //     face não mapeia (convenção documentada de include/core/sfnt.h). Com o conjunto ampliado
    //     (Latin-1 Supplement + pontuação tipográfica) alguns codepoints listados podem
    //     legitimamente faltar numa dada face, então pula-os aqui em vez de empacotar o próprio
    //     outline de caixa do .notdef sob um codepoint real -- isso poluiria o atlas com uma caixa
    //     tofu visível e faria o FindGlyph() devolver um quad espúrio pra um caractere que a fonte
    //     nunca teve. Pular deixa o codepoint totalmente fora de inst.glyphs: FindGlyph() devolve
    //     nullptr, a string ainda mede/renderiza, aquele caractere só fica em branco (avanço 0, sem
    //     quad) -- exatamente a degradação "codepoint fora do conjunto empacotado" que o SCOPE do
    //     header promete. cp 0 nunca está em BuildBakeSet(), então isto nunca larga por engano um
    //     glyph real mapeado a gid 0.
    if (gid == 0) continue;

    uint16_t advance_units = 0;
    int16_t lsb = 0;
    if (glx_sfnt_hmetrics(&sf, gid, &advance_units, &lsb) != GLX_SFNT_OK) continue; // gid out of range -- skip.

    Baked b;
    b.cp = cp;
    b.gid = gid;
    b.advance = (float)advance_units * scale;

    glx_sfnt_outline outline{};
    const glx_sfnt_result r =
        glx_sfnt_glyph_outline(&sf, gid, points.data(), (uint16_t)points.size(), contour_ends.data(),
                                (uint16_t)contour_ends.size(), &outline);

    // EN: r != GLX_SFNT_OK (e.g. GLX_SFNT_ERR_BUFFER_TOO_SMALL) or an empty outline (space,
    //     num_points==0) both fall through to push_back(b) below with gw==gh==0 -- a "baked,
    //     zero-size" glyph: still has a valid advance, deliberately emits no quad (see
    //     GlyphInfo's own doc-comment in the header for this exact distinction).
    // PT: r != GLX_SFNT_OK (ex.: GLX_SFNT_ERR_BUFFER_TOO_SMALL) ou um outline vazio (espaço,
    //     num_points==0) ambos caem no push_back(b) abaixo com gw==gh==0 -- um glyph
    //     "empacotado, tamanho-zero": ainda tem um avanço válido, deliberadamente não emite
    //     quad (ver o próprio doc-comment de GlyphInfo no header pra essa distinção exata).
    if (r == GLX_SFNT_OK && outline.num_points > 0 && outline.num_contours > 0) {
      const float fx_min = (float)outline.x_min * scale;
      const float fx_max = (float)outline.x_max * scale;
      const float fy_min = (float)outline.y_min * scale;
      const float fy_max = (float)outline.y_max * scale;

      constexpr int kPad = 1; // 1px transparent border -- avoids AA bleed between packed atlas cells.
      const int gw = (int)std::ceil(fx_max - fx_min) + kPad * 2;
      const int gh = (int)std::ceil(fy_max - fy_min) + kPad * 2;

      if (gw > 0 && gh > 0 && gw <= 4096 && gh <= 4096) {
        // EN: (scale_num, scale_den) is the rational font-unit->26.6-device-pixel scale factor
        //     glx_rasterize_outline's own doc-comment specifies: (px_size_26_6, units_per_em).
        //     origin_x/y_26_6 place this glyph's own (x_min, y_max) at bitmap pixel (kPad,
        //     kPad) -- see include/core/raster.h's "COORDINATE MAPPING" note for the
        //     device_x/y_26_6 formulas this inverts (origin chosen so
        //     device_x(x_min)==kPad*64, device_y(y_max)==kPad*64).
        // PT: (scale_num, scale_den) é o fator de escala racional unidade-de-fonte->pixel-de-
        //     dispositivo-26.6 que o próprio doc-comment do glx_rasterize_outline especifica:
        //     (px_size_26_6, units_per_em). origin_x/y_26_6 posiciona o próprio (x_min, y_max)
        //     deste glyph no pixel de bitmap (kPad, kPad) -- ver a nota "MAPEAMENTO DE
        //     COORDENADA" do include/core/raster.h pras fórmulas device_x/y_26_6 que isto
        //     inverte (origem escolhida para device_x(x_min)==kPad*64, device_y(y_max)==kPad*64).
        const int32_t scale_num = (int32_t)std::lround((double)inst.pixel_size * 64.0);
        const int32_t scale_den = (int32_t)sf.units_per_em;
        const int32_t origin_x_26_6 = (int32_t)std::lround(((double)-fx_min + kPad) * 64.0);
        const int32_t origin_y_26_6 = (int32_t)std::lround(((double)fy_max + kPad) * 64.0);

        std::vector<uint8_t> bitmap((size_t)gw * (size_t)gh, 0);
        const size_t scratch_n = glx_raster_scratch_floats(gw, gh);
        if (scratch_n > 0) {
          std::vector<float> scratch(scratch_n);
          const glx_raster_result rr =
              glx_rasterize_outline(points.data(), outline.num_points, contour_ends.data(), outline.num_contours,
                                     scale_num, scale_den, origin_x_26_6, origin_y_26_6, bitmap.data(), gw, gh, gw,
                                     scratch.data(), scratch.size());
          if (rr == GLX_RASTER_OK) {
            b.gw = gw;
            b.gh = gh;
            b.coverage = std::move(bitmap);
            // EN: quad top-left offset from (pen_x, baseline_y), screen space y-down -- derived
            //     in font_engine_own.hpp's GlyphInfo doc-comment / IterateGlyphs' contract.
            // PT: offset do canto superior-esquerdo do quad em relação a (pen_x, baseline_y),
            //     espaço de tela y-para-baixo -- derivado no doc-comment de GlyphInfo em
            //     font_engine_own.hpp / no contrato de IterateGlyphs.
            b.offset_x = fx_min - kPad;
            b.offset_y = -(fy_max + kPad);
            if (cp == 0x78) inst.metrics.x_height = fy_max - fy_min; // 'x' baked -- refine x_height.
            // EN: U+2026 rasterised for this face -- RmlUi may use the native ellipsis glyph.
            // PT: U+2026 rasterizou pra esta face -- o RmlUi pode usar o glyph nativo de reticências.
            if (cp == 0x2026) inst.metrics.has_ellipsis = true;
          }
        }
      }
    }

    baked.push_back(std::move(b));
  }

  // EN: Shelf-pack pass -- fixed-width shelves (grown to fit the widest single glyph, if any
  //     exceeds the 512px default), rows advance downward as a shelf fills. Deterministic,
  //     single-pass, good enough for this ticket's fixed set (~199 codepoints after the sub-phase
  //     2.1 widening -- printable ASCII + full Latin-1 Supplement + typographic punctuation, minus
  //     any the face lacks; no need for a general-purpose bin packer here). Only the atlas HEIGHT
  //     grows to fit the extra shelves (pen_y accumulates downward, atlas_h derived below); the
  //     512px shelf WIDTH is untouched -- no single Open Sans glyph at any UI size approaches it,
  //     so the initial texture size did NOT need to grow for the larger set.
  // PT: Passe de empacotamento em prateleiras -- prateleiras de largura fixa (cresce pra caber
  //     o glyph mais largo, se algum exceder o padrão de 512px), linhas avançam pra baixo à
  //     medida que uma prateleira enche. Determinístico, passe único, suficiente pro conjunto
  //     fixo de ~120 glyphs desta tarefa (sem necessidade de um bin packer de propósito geral
  //     aqui).
  int atlas_w = 512;
  for (const Baked& b : baked) atlas_w = std::max(atlas_w, b.gw);

  int pen_x = 0, pen_y = 0, shelf_h = 0;
  for (Baked& b : baked) {
    if (b.gw <= 0 || b.gh <= 0) continue;
    if (pen_x + b.gw > atlas_w) {
      pen_y += shelf_h;
      pen_x = 0;
      shelf_h = 0;
    }
    b.atlas_x = pen_x;
    b.atlas_y = pen_y;
    pen_x += b.gw;
    shelf_h = std::max(shelf_h, b.gh);
  }
  const int atlas_h = std::max(1, pen_y + shelf_h);

  inst.atlas_w = atlas_w;
  inst.atlas_h = atlas_h;
  inst.atlas_rgba.assign((size_t)atlas_w * (size_t)atlas_h * 4, 0);

  for (const Baked& b : baked) {
    GlyphInfo gi{};
    gi.baked = true;
    gi.gid = b.gid;
    gi.advance = b.advance;
    if (b.gw > 0 && b.gh > 0) {
      gi.atlas_x = b.atlas_x;
      gi.atlas_y = b.atlas_y;
      gi.w = b.gw;
      gi.h = b.gh;
      gi.offset_x = b.offset_x;
      gi.offset_y = b.offset_y;
      // EN: Blit coverage -> RGBA8 as (cov,cov,cov,cov) -- "premultiplied white": a fully
      //     covered texel is opaque white (255,255,255,255); a text colour vertex, itself
      //     premultiplied (ColourbPremultiplied), multiplied component-wise against this in the
      //     fragment shader (RmlUi_Renderer_GL3's own text-quad blend, GL_ONE,
      //     GL_ONE_MINUS_SRC_ALPHA) yields the correctly coverage-modulated premultiplied
      //     colour -- the same trick every coverage-atlas text renderer uses (stb_truetype's
      //     STBTT_bitmap included), not FreeType-specific.
      // PT: Blita cobertura -> RGBA8 como (cov,cov,cov,cov) -- "branco premultiplicado": um
      //     texel totalmente coberto é branco opaco (255,255,255,255); um vértice de cor de
      //     texto, ele mesmo premultiplicado (ColourbPremultiplied), multiplicado
      //     componente-a-componente contra isto no fragment shader (blend próprio de quad de
      //     texto do RmlUi_Renderer_GL3, GL_ONE, GL_ONE_MINUS_SRC_ALPHA) resulta na cor
      //     premultiplicada corretamente modulada por cobertura -- o mesmo truque que todo
      //     renderizador de texto por atlas-de-cobertura usa (STBTT_bitmap do stb_truetype
      //     incluso), não específico do FreeType.
      for (int y = 0; y < b.gh; ++y) {
        const uint8_t* src_row = &b.coverage[(size_t)y * b.gw];
        uint8_t* dst_row = &inst.atlas_rgba[((size_t)(b.atlas_y + y) * atlas_w + (size_t)b.atlas_x) * 4];
        for (int x = 0; x < b.gw; ++x) {
          const uint8_t cov = src_row[x];
          dst_row[x * 4 + 0] = cov;
          dst_row[x * 4 + 1] = cov;
          dst_row[x * 4 + 2] = cov;
          dst_row[x * 4 + 3] = cov;
        }
      }
    }
    inst.glyphs.emplace(b.cp, gi);
  }
}

const FontEngineOwn::GlyphInfo* FontEngineOwn::FindGlyph(const FaceInstance& inst, uint32_t codepoint) {
  const auto it = inst.glyphs.find(codepoint);
  return (it != inst.glyphs.end() && it->second.baked) ? &it->second : nullptr;
}

float FontEngineOwn::IterateGlyphs(const FaceInstance& inst, Rml::StringView string, float letter_spacing,
                                    const std::function<void(float pen_x, const GlyphInfo*)>& emit) {
  // EN: (L1.19-FONTENG/L1.20-FONTFLIP amadurecimento) Same font-unit->px scale BakeFaceInstance()
  //     used to derive every glyph's own `advance`/outline for this instance -- glx_sfnt_kern()
  //     returns its adjustment in FUnits (head::units_per_em-scaled, per include/core/sfnt.h),
  //     so it must go through the identical `pixel_size / units_per_em` factor to land in the
  //     same px space `pen` already accumulates in. `inst.face` is guaranteed non-null here (every
  //     FaceInstance is baked from a resolved LoadedFace* in GetFontFaceHandle()).
  // PT: Mesma escala unidade-de-fonte->px que BakeFaceInstance() usou pra derivar o próprio
  //     `advance`/outline de todo glyph desta instância -- glx_sfnt_kern() devolve o ajuste dele
  //     em FUnits (escalado por head::units_per_em, conforme include/core/sfnt.h), então precisa
  //     passar pelo mesmo fator `pixel_size / units_per_em` pra cair no mesmo espaço-px em que
  //     `pen` já acumula. `inst.face` é garantidamente não-nulo aqui (toda FaceInstance é
  //     empacotada a partir de uma LoadedFace* resolvida em GetFontFaceHandle()).
  const glx_sfnt_face& sf = inst.face->sfnt;
  const float scale = (float)inst.pixel_size / (float)sf.units_per_em;

  float pen = 0.f;
  bool has_prev = false;
  uint32_t prev_gid = 0;
  for (Rml::StringIteratorU8 it(string.begin(), string.begin(), string.end()); it; ++it) {
    const Rml::Character c = *it;
    const uint32_t cp = (uint32_t)c;
    const GlyphInfo* g = FindGlyph(inst, cp);

    // EN: Kerning applies ONLY between two CONSECUTIVE baked glyphs (both have a real gid) --
    //     folded into `pen` BEFORE `emit()` so the current glyph's own quad/measurement already
    //     reflects the tightened (or loosened) gap to its predecessor. See this method's
    //     doc-comment in font_engine_own.hpp for the no-rounding-mid-loop rationale.
    // PT: Kerning se aplica SÓ entre dois glyphs CONSECUTIVOS empacotados (ambos com gid real) --
    //     dobrado em `pen` ANTES do `emit()` pra que o próprio quad/medição do glyph atual já
    //     reflita o vão apertado (ou afrouxado) até o predecessor. Ver o doc-comment deste método
    //     em font_engine_own.hpp pro motivo do sem-arredondamento-no-meio-do-laço.
    if (has_prev && g) {
      const int16_t kern_funits = glx_sfnt_kern(&sf, prev_gid, g->gid);
      if (kern_funits != 0) pen += (float)kern_funits * scale;
    }

    emit(pen, g);
    pen += (g ? g->advance : 0.0f) + letter_spacing;

    has_prev = (g != nullptr);
    prev_gid = g ? g->gid : 0;
  }
  return pen;
}

Rml::FontFaceHandle FontEngineOwn::GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style,
                                                       Rml::Style::FontWeight weight, int size) {
  if (size <= 0) return 0;
  const LoadedFace* face = FindBestFace(family, style, weight);
  if (!face) return 0;

  // EN: Cache hit -- an instance for this exact (face, pixel_size) was already baked.
  // PT: Acerto de cache -- uma instância pra este (face, pixel_size) exato já foi empacotada.
  for (const auto& inst : instances_)
    if (inst->face == face && inst->pixel_size == size)
      return reinterpret_cast<Rml::FontFaceHandle>(inst.get());

  // EN: "size, in points" per the base class's own doc-comment -- treated as PHYSICAL PIXELS
  //     directly here (no additional pt->px conversion), matching this codebase's own
  //     dp_ratio design: App::set_dp_ratio()/UiLayer::set_dp_ratio() already scale the `dp`
  //     RCSS unit to physical pixels UPSTREAM of RmlUi's layout engine (see
  //     docs/embed-integration.md) -- by the time a `font-size` property reaches
  //     GetFontFaceHandle(), it is already in the same physical-pixel space every other sizing
  //     property in this codebase uses. Documented assumption, not independently re-verified
  //     against RmlUi's internal pt/px handling for THIS ticket.
  // PT: "size, em pontos" conforme o próprio doc-comment da classe-base -- tratado aqui
  //     diretamente como PIXELS FÍSICOS (sem conversão pt->px adicional), batendo com o
  //     próprio design de dp_ratio deste código-base: App::set_dp_ratio()/
  //     UiLayer::set_dp_ratio() já escalam a unidade RCSS `dp` pra pixels físicos A MONTANTE do
  //     motor de layout do RmlUi (ver docs/embed-integration.md) -- quando uma propriedade
  //     `font-size` chega em GetFontFaceHandle(), já está no mesmo espaço de pixel físico que
  //     toda outra propriedade de tamanho deste código-base usa. Suposição documentada, não
  //     re-verificada independentemente contra o tratamento interno de pt/px do RmlUi PRA ESTA
  //     TAREFA.
  auto inst = std::make_unique<FaceInstance>();
  inst->face = face;
  inst->pixel_size = size;
  BakeFaceInstance(*inst);

  const Rml::FontFaceHandle handle = reinterpret_cast<Rml::FontFaceHandle>(inst.get());
  instances_.push_back(std::move(inst));
  return handle;
}

Rml::FontEffectsHandle FontEngineOwn::PrepareFontEffects(Rml::FontFaceHandle /*handle*/,
                                                          const Rml::FontEffectList& /*font_effects*/) {
  // EN: No-op -- font effects (text-shadow/outline-via-font-effect) are out of scope for this
  //     ticket (see this file's header "SCOPE"). RmlUi tolerates a `0` effects handle: text
  //     still renders via GenerateString(), just without the effect.
  // PT: No-op -- efeitos de fonte (text-shadow/contorno via efeito-de-fonte) estão fora de
  //     escopo desta tarefa (ver a seção "ESCOPO" do cabeçalho deste arquivo). O RmlUi tolera
  //     um handle de efeito `0`: o texto ainda renderiza via GenerateString(), só sem o efeito.
  return 0;
}

const Rml::FontMetrics& FontEngineOwn::GetFontMetrics(Rml::FontFaceHandle handle) {
  static const Rml::FontMetrics kEmpty{};
  const auto* inst = reinterpret_cast<const FaceInstance*>(handle);
  return inst ? inst->metrics : kEmpty;
}

int FontEngineOwn::GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string,
                                   const Rml::TextShapingContext& text_shaping_context,
                                   Rml::Character /*prior_character*/) {
  const auto* inst = reinterpret_cast<const FaceInstance*>(handle);
  if (!inst) return 0;
  const float width =
      IterateGlyphs(*inst, string, text_shaping_context.letter_spacing, [](float, const GlyphInfo*) {});
  return (int)std::lround(width);
}

int FontEngineOwn::GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle,
                                   Rml::FontEffectsHandle /*font_effects_handle*/, Rml::StringView string,
                                   Rml::Vector2f position, Rml::ColourbPremultiplied colour, float /*opacity*/,
                                   const Rml::TextShapingContext& text_shaping_context,
                                   Rml::TexturedMeshList& mesh_list) {
  auto* inst = reinterpret_cast<FaceInstance*>(face_handle);
  if (!inst) return 0;

  // EN: Lazy, ONE-SHOT GPU texture creation -- see font_engine_own.hpp's "SCOPE" section for
  //     why this cannot happen earlier (GetFontFaceHandle() is not passed a RenderManager&).
  //     The lambda captures `inst` (a stable pointer -- `instances_` holds it by
  //     std::unique_ptr, never reallocated/moved) rather than copying atlas_rgba, so it reads
  //     whatever is current in inst->atlas_rgba at the time the callback actually runs (which,
  //     for this ticket's eager-bake design, is already final by this point regardless).
  // PT: Criação de textura GPU preguiçosa, ÚNICA -- ver a seção "ESCOPO" de font_engine_own.hpp
  //     pro motivo de isto não poder acontecer antes (GetFontFaceHandle() não recebe um
  //     RenderManager&). A lambda captura `inst` (um ponteiro estável -- `instances_` o guarda
  //     via std::unique_ptr, nunca realocado/movido) em vez de copiar atlas_rgba, então lê o
  //     que estiver atual em inst->atlas_rgba no momento em que o callback de fato roda (que,
  //     pro design de empacotamento ávido desta tarefa, já é final até este ponto de qualquer
  //     forma).
  if (!inst->texture_created) {
    inst->texture_created = true;
    inst->texture = render_manager.MakeCallbackTexture(
        [inst](const Rml::CallbackTextureInterface& texture_interface) -> bool {
          return texture_interface.GenerateTexture(
              Rml::Span<const Rml::byte>(inst->atlas_rgba.data(), inst->atlas_rgba.size()),
              Rml::Vector2i(inst->atlas_w, inst->atlas_h));
        });
  }

  Rml::Mesh mesh;
  const float atlas_w_f = (float)inst->atlas_w;
  const float atlas_h_f = (float)inst->atlas_h;

  auto push_quad = [&](float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1) {
    const int base = (int)mesh.vertices.size();
    Rml::Vertex v;
    v.colour = colour;
    v.position = Rml::Vector2f(x0, y0);
    v.tex_coord = Rml::Vector2f(u0, v0);
    mesh.vertices.push_back(v);
    v.position = Rml::Vector2f(x1, y0);
    v.tex_coord = Rml::Vector2f(u1, v0);
    mesh.vertices.push_back(v);
    v.position = Rml::Vector2f(x1, y1);
    v.tex_coord = Rml::Vector2f(u1, v1);
    mesh.vertices.push_back(v);
    v.position = Rml::Vector2f(x0, y1);
    v.tex_coord = Rml::Vector2f(u0, v1);
    mesh.vertices.push_back(v);
    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 0);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
  };

  const float width =
      IterateGlyphs(*inst, string, text_shaping_context.letter_spacing, [&](float pen_x, const GlyphInfo* g) {
        if (!g || g->w <= 0 || g->h <= 0) return; // un-baked or deliberately-empty (space) glyph -- no quad.
        const float x0 = position.x + pen_x + g->offset_x;
        const float y0 = position.y + g->offset_y;
        const float x1 = x0 + (float)g->w;
        const float y1 = y0 + (float)g->h;
        const float u0 = (float)g->atlas_x / atlas_w_f;
        const float v0 = (float)g->atlas_y / atlas_h_f;
        const float u1 = (float)(g->atlas_x + g->w) / atlas_w_f;
        const float v1 = (float)(g->atlas_y + g->h) / atlas_h_f;
        push_quad(x0, y0, x1, y1, u0, v0, u1, v1);
      });

  if (!mesh.indices.empty()) {
    Rml::TexturedMesh tm;
    tm.mesh = std::move(mesh);
    tm.texture = inst->texture; // CallbackTexture -> Texture implicit conversion (Texture.h).
    mesh_list.push_back(std::move(tm));
  }

  return (int)std::lround(width);
}

int FontEngineOwn::GetVersion(Rml::FontFaceHandle handle) {
  // EN: Constant `1` for any valid handle -- see font_engine_own.hpp's "SCOPE" section for why
  //     this instance's atlas never changes after its one-shot bake (nothing to invalidate
  //     RmlUi's cached geometry over).
  // PT: Constante `1` pra qualquer handle válido -- ver a seção "ESCOPO" de font_engine_own.hpp
  //     pro motivo do atlas desta instância nunca mudar depois do empacotamento único (nada pra
  //     invalidar na geometria cacheada do RmlUi).
  return handle ? 1 : 0;
}

} // namespace glintfx

#endif // GLINTFX_OWN_FONT_ENGINE
