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

// EN: Y-HINT BYPASS storage (L1.20-FONTFLIP, FT-F4, sub-phase 1.4). See the long doc-comment on
//     this function's declaration in font_engine_own.hpp for the full contract. Same
//     function-local-static pattern as own_font_engine_ab_bypass() above (default `false` on first
//     use, no static-init-order dependency, reference returned so a test can flip it). Default
//     `false` == "apply Y grid-fitting", so a normal ON build ships WITH hinting; a test flips it
//     to `true` to measure the raw, un-hinted own-engine path (the "own_nohint" A/B leg).
// PT: Armazenamento do BYPASS do Y-HINT (L1.20-FONTFLIP, FT-F4, sub-fase 1.4). Ver o doc-comment
//     longo na declaração desta função em font_engine_own.hpp pro contrato completo. Mesmo padrão
//     static-local-de-função do own_font_engine_ab_bypass() acima (default `false` no primeiro
//     uso, sem dependência de ordem de init estática, referência retornada pra um teste poder
//     virá-lo). Default `false` == "aplica grid-fitting Y", então um build ON normal já sai COM
//     hinting; um teste o vira pra `true` pra medir o caminho cru, sem-hint, do motor próprio (a
//     perna A/B "own_nohint").
bool& own_font_engine_hint_bypass() {
  static bool bypass = false;
  return bypass;
}

// EN: STEM-DARKENING ENABLE storage (L1.20-FONTFLIP, FT-F4, sub-phase 1.5). See the long doc-comment
//     on this function's declaration in font_engine_own.hpp for the full contract AND the measured
//     negative result. OPT-IN: default `false` == "raw coverage, NO darkening", the shipping
//     behaviour (the coverage curve did not close the stem-sharpness gap -- see the header). A test
//     flips it to `true` to measure the darkened path (the "own_yhint_dark" A/B leg). Same
//     function-local-static pattern as the two hooks above; only the default polarity differs
//     (opt-in, not opt-out), which is why this one is named `_enable`, not `_bypass`.
// PT: Armazenamento do ENABLE de STEM-DARKENING (L1.20-FONTFLIP, FT-F4, sub-fase 1.5). Ver o
//     doc-comment longo na declaração desta função em font_engine_own.hpp pro contrato completo E o
//     resultado negativo medido. OPT-IN: default `false` == "cobertura crua, SEM darkening", o
//     comportamento de release (a curva de cobertura não fechou o gap de nitidez de haste -- ver o
//     header). Um teste o vira pra `true` pra medir o caminho escurecido (a perna A/B
//     "own_yhint_dark"). Mesmo padrão static-local-de-função dos dois hooks acima; só a polaridade
//     do default difere (opt-in, não opt-out), motivo de este se chamar `_enable`, não `_bypass`.
bool& own_font_engine_darken_enable() {
  static bool enable = false;
  return enable;
}

// EN: PEN-SNAP BYPASS storage (L1.20-FONTFLIP, FT-F4). See the long doc-comment on this function's
//     declaration in font_engine_own.hpp for the full contract. Same function-local-static pattern
//     as own_font_engine_hint_bypass() above (default `false` on first use, no static-init-order
//     dependency, reference returned so a test can flip it). Default `false` == "apply pen-snap"
//     (round each per-glyph pen component to a whole pixel, FreeType-like), so a normal ON build
//     ships WITH snapping; a test flips it to `true` to measure the raw, un-snapped float pen (the
//     "pen float" A/B leg).
// PT: Armazenamento do BYPASS do PEN-SNAP (L1.20-FONTFLIP, FT-F4). Ver o doc-comment longo na
//     declaração desta função em font_engine_own.hpp pro contrato completo. Mesmo padrão
//     static-local-de-função do own_font_engine_hint_bypass() acima (default `false` no primeiro
//     uso, sem dependência de ordem de init estática, referência retornada pra um teste poder
//     virá-lo). Default `false` == "aplica pen-snap" (arredonda cada componente do pen por-glyph a
//     um pixel inteiro, FreeType-like), então um build ON normal já sai COM snapping; um teste o
//     vira pra `true` pra medir o pen float cru, sem-snap (a perna A/B "pen float").
bool& own_font_engine_pensnap_bypass() {
  static bool bypass = false;
  return bypass;
}

// EN: VSNAP BYPASS storage (L1.20-FONTFLIP, FT-F4). See the long doc-comment on this function's
//     declaration in font_engine_own.hpp for the full contract. Same function-local-static pattern
//     as own_font_engine_pensnap_bypass() above (default `false` on first use, no static-init-order
//     dependency, reference returned so a test can flip it). Default `false` == "apply vsnap" (snap
//     the glyph's vertical origin/offset -- derived from fy_max -- to a whole pixel so the Y-hinted
//     edge lands on an integer atlas row and the quad carries a single size-independent vertical
//     phase), so a normal ON build ships WITH vertical snapping; a test flips it to `true` to measure
//     the raw fractional vertical origin/offset (the "own, no vsnap" A/B leg).
// PT: Armazenamento do BYPASS do VSNAP (L1.20-FONTFLIP, FT-F4). Ver o doc-comment longo na
//     declaração desta função em font_engine_own.hpp pro contrato completo. Mesmo padrão
//     static-local-de-função do own_font_engine_pensnap_bypass() acima (default `false` no primeiro
//     uso, sem dependência de ordem de init estática, referência retornada pra um teste poder
//     virá-lo). Default `false` == "aplica vsnap" (snapa a origem/offset vertical do glyph --
//     derivada de fy_max -- a um pixel inteiro pra a aresta hintada em Y pousar numa row inteira do
//     atlas e o quad carregar uma fase vertical única independente de tamanho), então um build ON
//     normal já sai COM snapping vertical; um teste o vira pra `true` pra medir a origem/offset
//     vertical fracionária crua (a perna A/B "próprio, sem vsnap").
bool& own_font_engine_vsnap_bypass() {
  static bool bypass = false;
  return bypass;
}

// EN: DEBUG registered-face-COUNT storage (L1.20-FONTFLIP, FT-F4, dedup leak fix). See the long
//     doc-comment on this function's declaration in font_engine_own.hpp for the full contract.
//     Same function-local-static pattern as the five hooks above; RegisterFace() increments it
//     once per successful faces_.push_back() and leaves it untouched on a dedup early-return.
// PT: Armazenamento da CONTAGEM de faces registradas, de debug (L1.20-FONTFLIP, FT-F4, fix de
//     vazamento do dedup). Ver o doc-comment longo na declaração desta função em
//     font_engine_own.hpp pro contrato completo. Mesmo padrão static-local-de-função dos cinco
//     hooks acima; o RegisterFace() o incrementa uma vez por faces_.push_back() bem-sucedido e o
//     deixa intocado num retorno antecipado de dedup.
size_t& own_font_engine_debug_registered_face_count() {
  static size_t count = 0;
  return count;
}

namespace {

// EN: The fixed WARM-SET bake set (see font_engine_own.hpp's "SCOPE" section). Still an EAGER,
//     one-shot, FIXED set BuildBakeSet() itself describes -- this ticket (L1.20-FONTFLIP sub-phase
//     2.1) only WIDENED it for general-purpose distribution (the product ships worldwide, not just
//     pt-br). (Sub-phase 2B amadurecimento) The eager fixed-set architecture is no longer the WHOLE
//     story: EnsureGlyphs()/BakeGlyph() (below) additively top-up-bake any codepoint OUTSIDE this set
//     the first time it actually appears in a string -- this function's job stays exactly what it
//     was, the deliberately curated FIRST-PAINT set every instance pays for up front. Three
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
// PT: O conjunto fixo empacotado do WARM SET (ver a seção "ESCOPO" de font_engine_own.hpp). Continua
//     um conjunto ÁVIDO, único, FIXO conforme o próprio BuildBakeSet() descreve -- esta tarefa
//     (sub-fase 2.1 do L1.20-FONTFLIP) só o AMPLIOU pra distribuição geral (o produto vai pro mundo,
//     não só pt-br). (amadurecimento sub-fase 2B) A arquitetura de conjunto-fixo-ávido deixou de ser
//     a história INTEIRA: EnsureGlyphs()/BakeGlyph() (abaixo) empacotam-por-demanda aditivamente
//     qualquer codepoint FORA deste conjunto na 1ª vez que de fato aparece numa string -- o trabalho
//     desta função continua exatamente o mesmo, o conjunto de PRIMEIRA-PINTURA deliberadamente
//     curado que toda instância paga adiantado. Três faixas, todas sem duplicata por construção
//     (nenhum codepoint aparece duas vezes):
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

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.5) STEM-DARKENING gamma for a given pixel size.
//     WHY: grayscale-AA coverage c in [0,1] is the fraction of a pixel the glyph covers. The GL3
//     renderer composites white text with a straight (GL_ONE, GL_ONE_MINUS_SRC_ALPHA) blend using c
//     as alpha WITHOUT an sRGB/linear framebuffer, so the blend happens in the display-encoded
//     (~gamma 2.2) space. A gamma-CORRECT composite would linearise, blend, re-encode -- and a thin
//     vertical stem whose coverage is split ~0.5/0.5 across two columns then emits MORE apparent
//     light than the naive 0.5 blend, because encode(0.5*white_linear) > 0.5*encode(white). Skipping
//     that step is the classic "AA text too thin/light" defect -- precisely our vstem_edge deficit
//     vs FreeType (which stem-darkens AND composites gamma-correct). CHEAP FIX: a per-texel coverage
//     curve c' = c^gamma with gamma<1, applied at atlas-blit time (BakeFaceInstance()). For
//     c in (0,1), c^gamma > c, so midtone edge/stem coverage gains opacity while the endpoints c=0
//     and c=1 are FIXED (background stays clean, glyph interiors stay solid -- letters cannot bleed
//     into each other, the over-darkening failure the brief warns about). This is the standard
//     "gamma correction for text" / stem-darkening approximation; it thickens ALL AA edges slightly
//     (not only vertical stems -- a true geometric stem-darkening would need the Layer-0 rasteriser,
//     deliberately NOT reopened this sub-phase). SIZE FADE: FreeType's stem darkening vanishes as
//     glyphs grow (a fixed em-space darkening becomes negligible at large ppem); we mirror that --
//     gamma ramps LINEARLY from kGammaMin at/below kPxLo back to 1.0 (identity) at/above kPxHi, so
//     24px+ body text is untouched and only the 11..16px small-body regime (where the deficit lives)
//     is darkened. Constants tuned against fonteng_ab_visual's measure_oracle() vstem_edge (see the
//     session report): kGammaMin=0.72 lifts a 0.5/0.5 split stem to ~0.61/0.61 -- a visible stem
//     gain without hazing the sparse AA fringe or pushing total_ink toward a filled block.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.5) Gamma de STEM-DARKENING pra um dado tamanho-em-px.
//     PORQUÊ: a cobertura grayscale-AA c em [0,1] é a fração do pixel que o glyph cobre. O
//     renderizador GL3 compõe texto branco com um blend direto (GL_ONE, GL_ONE_MINUS_SRC_ALPHA)
//     usando c como alpha SEM um framebuffer sRGB/linear, então o blend acontece no espaço
//     display-encoded (~gamma 2.2). Um composite gamma-CORRETO linearizaria, blendaria, re-
//     codificaria -- e uma haste vertical fina cuja cobertura se divide ~0.5/0.5 em duas colunas
//     então emite MAIS luz aparente que o blend ingênuo de 0.5, porque encode(0.5*branco_linear) >
//     0.5*encode(branco). Pular esse passo é o defeito clássico "texto AA fino/claro demais" --
//     exatamente nosso déficit de vstem_edge vs FreeType (que faz stem-darkening E compõe gamma-
//     correto). FIX BARATO: uma curva de cobertura por-texel c' = c^gamma com gamma<1, aplicada no
//     blit do atlas (BakeFaceInstance()). Pra c em (0,1), c^gamma > c, então a cobertura de meia-
//     tinta de borda/haste ganha opacidade enquanto os extremos c=0 e c=1 ficam FIXOS (fundo limpo,
//     interiores de glyph sólidos -- letras não podem se fundir, a falha de over-darkening que o
//     brief alerta). É a aproximação padrão "correção de gamma pra texto" / stem-darkening; engorda
//     TODAS as arestas AA levemente (não só hastes verticais -- um stem-darkening geométrico de
//     verdade precisaria do rasterizador da Camada 0, deliberadamente NÃO reaberto nesta sub-fase).
//     FADE POR TAMANHO: o stem-darkening do FreeType some quando os glyphs crescem (um darkening
//     fixo em espaço-em torna-se desprezível em ppem grande); espelhamos isso -- gamma rampa
//     LINEARMENTE de kGammaMin em/abaixo de kPxLo de volta pra 1.0 (identidade) em/acima de kPxHi,
//     então texto de corpo 24px+ fica intocado e só o regime de corpo pequeno 11..16px (onde o
//     déficit vive) é escurecido. Constantes calibradas contra o vstem_edge do measure_oracle() do
//     fonteng_ab_visual (ver o relatório da sessão): kGammaMin=0.72 eleva uma haste dividida 0.5/0.5
//     pra ~0.61/0.61 -- um ganho de haste visível sem embaçar a franja AA esparsa nem empurrar o
//     total_ink pra um bloco cheio.
float DarkenGamma(int px) {
  constexpr float kGammaMin = 0.72f; // strongest darkening, at/below kPxLo.
  constexpr int   kPxLo = 11;        // small-body regime -- full darkening.
  constexpr int   kPxHi = 24;        // >= this: identity (no darkening) -- large text untouched.
  if (px <= kPxLo) return kGammaMin;
  if (px >= kPxHi) return 1.0f;
  const float t = (float)(px - kPxLo) / (float)(kPxHi - kPxLo);
  return kGammaMin + t * (1.0f - kGammaMin);
}

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.5) Builds the 256-entry coverage remap LUT applied to
//     every atlas texel in BakeFaceInstance() -- computed ONCE per baked instance (not per texel),
//     so the per-pixel blit cost is a single array lookup. `enabled==false` (the darken-bypass leg,
//     or any size >= kPxHi where DarkenGamma()==1) yields the IDENTITY map, so the blit is a
//     byte-for-byte copy exactly as before sub-phase 1.5 -- zero behavioural change on that path.
//     Otherwise fills lut[i] = round(255 * (i/255)^gamma), which fixes lut[0]=0 and lut[255]=255 by
//     construction (endpoints never move -- see DarkenGamma()'s note on why letters cannot merge).
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.5) Constrói a LUT de remapeamento de cobertura de 256
//     entradas aplicada a todo texel do atlas em BakeFaceInstance() -- computada UMA vez por
//     instância empacotada (não por texel), então o custo por-pixel do blit é uma única consulta de
//     array. `enabled==false` (a perna de darken-bypass, ou qualquer tamanho >= kPxHi onde
//     DarkenGamma()==1) devolve o mapa IDENTIDADE, então o blit é uma cópia byte-a-byte exatamente
//     como antes da sub-fase 1.5 -- zero mudança de comportamento nesse caminho. Senão preenche
//     lut[i] = round(255 * (i/255)^gamma), que fixa lut[0]=0 e lut[255]=255 por construção (os
//     extremos nunca se movem -- ver a nota de DarkenGamma() sobre por que letras não podem fundir).
void BuildCoverageLut(uint8_t lut[256], int px, bool enabled) {
  const float g = enabled ? DarkenGamma(px) : 1.0f;
  if (g >= 0.999f) {
    for (int i = 0; i < 256; ++i) lut[i] = (uint8_t)i;
    return;
  }
  for (int i = 0; i < 256; ++i) {
    const float c = (float)i / 255.0f;
    const int v = (int)std::lround((double)std::pow(c, g) * 255.0);
    lut[i] = (uint8_t)std::clamp(v, 0, 255);
  }
}

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.3) The FONT-UNIT y_max (topmost outline point) of the
//     glyph the face maps `cp` to, used to MEASURE the x-height / cap-height reference lines from
//     the real font rather than guessing them. Sets `*ok=true` and returns the measured y_max only
//     when the codepoint is present in the face (gid != 0) AND its outline fetches cleanly AND has
//     at least one real point; otherwise `*ok=false` (the caller falls back to a fraction-of-em
//     estimate). Never allocates beyond two stack-scoped buffers, never crashes on a hostile face
//     (every failure is a clean `*ok=false`) -- it only READS through the same bounds-checked
//     SOV-SFNT API BakeFaceInstance() already uses. `y_max` comes straight from the glyf record
//     header (SOV-SFNT does not recompute it), exactly the value the zones must be expressed in.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.3) O y_max EM UNIDADES DE FONTE (ponto mais alto do
//     outline) do glyph que a face mapeia pra `cp`, usado pra MEDIR as linhas de referência de
//     altura-x / altura-de-maiúscula da fonte real em vez de adivinhá-las. Define `*ok=true` e
//     retorna o y_max medido só quando o codepoint está presente na face (gid != 0) E seu outline
//     lê limpo E tem ao menos um ponto real; senão `*ok=false` (quem chama cai pra uma estimativa
//     de fração-de-em). Nunca aloca além de dois buffers de escopo de pilha, nunca crasha numa
//     face hostil (toda falha é um `*ok=false` limpo) -- só LÊ pela mesma API SOV-SFNT com
//     checagem de limite que o BakeFaceInstance() já usa. `y_max` vem direto do cabeçalho do
//     registro glyf (o SOV-SFNT não o recomputa), exatamente o valor em que as zonas devem ser
//     expressas.
int16_t GlyphYMax(const glx_sfnt_face& sf, uint32_t cp, bool& ok) {
  ok = false;
  const uint32_t gid = glx_sfnt_glyph_id(&sf, cp);
  if (gid == 0) return 0;
  std::vector<glx_sfnt_point> points(kMaxOutlinePoints);
  std::vector<uint16_t> contour_ends(kMaxOutlineContours);
  glx_sfnt_outline o{};
  if (glx_sfnt_glyph_outline(&sf, gid, points.data(), (uint16_t)points.size(), contour_ends.data(),
                             (uint16_t)contour_ends.size(), &o) != GLX_SFNT_OK)
    return 0;
  if (o.num_points == 0 || o.num_contours == 0) return 0;
  ok = true;
  return o.y_max;
}

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.3) Derives THIS face's vertical grid-fitting zones ONCE
//     (called from RegisterFace() right after glx_sfnt_open()). baseline is 0 (TrueType's own
//     origin), ascender/descender come straight from hhea (glx_sfnt_face), and x_height/cap_height
//     are MEASURED from the real 'x'/'H' glyph outlines' own y_max (falling back to 'o'/'O', then
//     to a fraction-of-ascender estimate if the face lacks those glyphs -- degrade, never crash,
//     per this sub-phase's brief). axis_flags carries GLX_HINT_AXIS_Y so glx_hint_outline() grid-
//     fits the Y axis; X is deliberately left un-hinted this sub-phase. Zones need NOT be sorted or
//     distinct -- glx_hint_outline() sorts and de-dups them internally (see include/core/hint.h).
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.3) Deriva UMA vez as zonas de grid-fitting vertical desta
//     face (chamada de RegisterFace() logo após glx_sfnt_open()). baseline é 0 (a própria origem
//     do TrueType), ascender/descender vêm direto do hhea (glx_sfnt_face), e x_height/cap_height
//     são MEDIDOS do próprio y_max dos outlines reais dos glyphs 'x'/'H' (caindo pra 'o'/'O',
//     depois pra uma estimativa de fração-do-ascender se a face não tiver esses glyphs -- degrada,
//     nunca crasha, conforme o brief desta sub-fase). axis_flags carrega GLX_HINT_AXIS_Y para o
//     glx_hint_outline() grid-fitar o eixo Y; o X é deliberadamente deixado sem-hint nesta
//     sub-fase. As zonas NÃO precisam estar ordenadas nem distintas -- o glx_hint_outline() as
//     ordena e de-duplica internamente (ver include/core/hint.h).
glx_hint_zones DeriveHintZones(const glx_sfnt_face& sf) {
  glx_hint_zones z{};
  z.baseline = 0;
  z.ascender = sf.ascender;
  z.descender = sf.descender;

  bool ok = false;
  int16_t v = GlyphYMax(sf, 0x78, ok);          // 'x'
  if (!ok) v = GlyphYMax(sf, 0x6F, ok);         // 'o' (rounded-overshoot but close enough)
  // EN: Fallback: ~half the ascender is a reasonable x-height when the face lacks 'x'/'o'.
  // PT: Fallback: ~metade do ascender é uma altura-x razoável quando a face não tem 'x'/'o'.
  z.x_height = ok ? v : (int16_t)((int)sf.ascender / 2);

  ok = false;
  v = GlyphYMax(sf, 0x48, ok);                  // 'H'
  if (!ok) v = GlyphYMax(sf, 0x4F, ok);         // 'O'
  // EN: Fallback: ~0.72 of the ascender is a typical cap-height when the face lacks 'H'/'O'.
  // PT: Fallback: ~0.72 do ascender é uma altura-de-maiúscula típica quando a face não tem 'H'/'O'.
  z.cap_height = ok ? v : (int16_t)((int)sf.ascender * 72 / 100);

  z.axis_flags = GLX_HINT_AXIS_Y;
  return z;
}

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

  // EN: (L1.20-FONTFLIP, dedup leak fix) DEDUP against already-registered faces BEFORE parsing/
  //     allocating a new LoadedFace -- mirrors the pinned RmlUi source's own
  //     FontProvider::AddFace()/FontFamily::AddFace() ("if (face.face->GetStyle() == style &&
  //     face.face->GetWeight() == weight) return {Duplicate, nullptr};", confirmed reading
  //     examples/RmlUi/Source/Core/FontEngineDefault/FontFamily.cpp), which never duplicates a
  //     face or its font-file blob for an already-loaded (family, style, weight) identity.
  //     WITHOUT this guard, RegisterFace() pushed a brand-new ~214KB-per-Open-Sans-file copy of
  //     `blob` UNCONDITIONALLY on every call -- and Rml::LoadFontFace() is called once per
  //     Context::LoadDocument() for every @font-face block the stylesheet declares (RmlUi's
  //     StyleSheetParser::ParseFontFaceBlock), so reloading the SAME document N times (a normal
  //     usage path: screen navigation, hot-reload -- NOT hostile input) leaked ~214KB*N, measured
  //     +213893 bytes/reload of VmRSS over 300 reloads. FindBestFace()'s `if (score > best_score)`
  //     (strictly greater) means the FIRST-registered face of a tied identity always wins ties, so
  //     every duplicate pushed after the first was already 100% dead weight: registered, never
  //     resolved to, never freed before Shutdown().
  //     CRITERION: (family, style, weight) -- RmlUi's own identity -- PLUS blob-content equality
  //     (exact byte match), narrower than RmlUi's family+style+weight-only rule on purpose: two
  //     GENUINELY DIFFERENT font files sharing a (family, style, weight) triple (e.g. two
  //     different @font-face blocks both claiming "Body/Normal/400" with different src) still
  //     coexist here exactly as before this fix (FindBestFace()'s resolution semantics for
  //     distinct faces are UNCHANGED -- first-registered still wins ties, by design, untouched by
  //     this guard) -- only the common case this ticket targets, the SAME file reloaded, dedupes.
  //     weight is compared post-Auto-normalisation (see the hoisted `normalized_weight` local
  //     right below) so an Auto-weight reload against an already-normalised-Normal entry still
  //     recognises the identity. Cheap: std::vector<uint8_t>::operator== is a single
  //     memcmp-equivalent pass, paid
  //     ONLY once family+style+weight already matched (rare outside the reload case this exists
  //     for), and this early return skips glx_sfnt_open()/DeriveHintZones() entirely for a
  //     dedup hit -- a bonus, not just a memory fix.
  // PT: (L1.20-FONTFLIP, fix de vazamento do dedup) DEDUPLICA contra faces já registradas ANTES de
  //     parsear/alocar uma nova LoadedFace -- espelha o próprio source pinado do RmlUi
  //     (FontProvider::AddFace()/FontFamily::AddFace() -- "if (face.face->GetStyle() == style &&
  //     face.face->GetWeight() == weight) return {Duplicate, nullptr};", confirmado lendo
  //     examples/RmlUi/Source/Core/FontEngineDefault/FontFamily.cpp), que nunca duplica uma face
  //     nem o blob do arquivo de fonte para uma identidade (family, style, weight) já carregada.
  //     SEM esta guarda, RegisterFace() empilhava uma cópia nova de ~214KB (Open Sans) de `blob`
  //     INCONDICIONALMENTE a cada chamada -- e Rml::LoadFontFace() é chamado uma vez por
  //     Context::LoadDocument() pra cada bloco @font-face que a stylesheet declara
  //     (StyleSheetParser::ParseFontFaceBlock do RmlUi), então recarregar o MESMO documento N
  //     vezes (um caminho de uso NORMAL: navegação de tela, hot-reload -- NÃO input hostil)
  //     vazava ~214KB*N, medido em +213893 bytes/reload de VmRSS ao longo de 300 reloads. O
  //     `if (score > best_score)` (estritamente maior) do FindBestFace() faz a face registrada
  //     PRIMEIRO de uma identidade empatada sempre ganhar, então toda duplicata empilhada depois
  //     da primeira já era 100% peso-morto: registrada, nunca resolvida, nunca liberada antes do
  //     Shutdown().
  //     CRITÉRIO: (family, style, weight) -- a identidade do próprio RmlUi -- MAIS igualdade de
  //     conteúdo do blob (match de byte exato), mais estreito que a regra só-family+style+weight
  //     do RmlUi de propósito: dois arquivos de fonte GENUINAMENTE DIFERENTES compartilhando uma
  //     tripla (family, style, weight) (ex.: dois blocos @font-face distintos ambos alegando
  //     "Body/Normal/400" com src diferente) ainda coexistem aqui exatamente como antes deste fix
  //     (a semântica de resolução do FindBestFace() para faces distintas fica INALTERADA -- a
  //     registrada primeiro ainda ganha empates, por design, intocado por esta guarda) -- só o
  //     caso comum que esta tarefa mira, o MESMO arquivo recarregado, deduplica. weight é
  //     comparado pós-normalização-de-Auto (ver a local içada `normalized_weight` logo abaixo) pra
  //     um reload de weight Auto contra uma entrada já-normalizada-Normal ainda reconhecer a
  //     identidade. Barato: std::vector<uint8_t>::operator== é uma única passada equivalente a
  //     memcmp, paga SÓ quando family+style+weight já casaram (raro fora do caso de reload que
  //     isto existe para cobrir), e este retorno antecipado pula glx_sfnt_open()/
  //     DeriveHintZones() inteiramente num acerto de dedup -- um bônus, não só um fix de memória.
  // EN: Style::FontWeight::Auto (0) is not a real weight to MATCH against (here OR below at the
  //     lf->weight assignment, which reuses this SAME variable) -- normalise to Normal (400),
  //     RmlUi's own "no weight specified" convention (StyleTypes.h comment: "Any definite value
  //     in the range [1,1000] is valid"). Hoisted to a single local so the dedup check above and
  //     the LoadedFace field below can never drift out of sync.
  // PT: Style::FontWeight::Auto (0) não é um peso real pra COMPARAR (aqui OU abaixo na atribuição
  //     de lf->weight, que reusa esta MESMA variável) -- normaliza para Normal (400), a própria
  //     convenção do RmlUi pra "nenhum peso especificado" (comentário de StyleTypes.h: "Any
  //     definite value in the range [1,1000] is valid"). Içado pra uma única local pra o cheque de
  //     dedup acima e o campo LoadedFace abaixo nunca dessincronizarem.
  const Rml::Style::FontWeight normalized_weight =
      (weight == Rml::Style::FontWeight::Auto) ? Rml::Style::FontWeight::Normal : weight;
  for (const auto& f : faces_) {
    if (f->family == family && f->style == style && f->weight == normalized_weight &&
        f->blob.size() == blob.size() && f->blob == blob) {
      // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) EDGE: a duplicate-hit face that is ALSO
      //     `fallback_face==true` must still land in `fallback_faces_` if it is not already there --
      //     mirrors the pinned RmlUi source's own FontFamily::AddFace(), which (confirmed reading it)
      //     appends to `fallback_font_faces` even on its "Duplicate" outcome, not only on a fresh
      //     insert. Without this, a document whose @font-face block re-declares the SAME fallback
      //     font (a legitimate reload path, the exact scenario the dedup guard above exists for)
      //     would silently lose per-glyph fallback for every codepoint the primary lacks, on the
      //     2nd+ load.
      // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) EDGE: uma face que bate no dedup mas
      //     TAMBÉM é `fallback_face==true` ainda precisa cair em `fallback_faces_` se ainda não
      //     estiver lá -- espelha o próprio FontFamily::AddFace() do source pinado do RmlUi, que
      //     (confirmado lendo-o) anexa a `fallback_font_faces` mesmo no resultado "Duplicate", não
      //     só numa inserção nova. Sem isto, um documento cujo bloco @font-face redeclara a MESMA
      //     fonte de fallback (um caminho de reload legítimo, exatamente o cenário pra que a guarda
      //     de dedup acima existe) perderia silenciosamente o fallback por-glyph pra todo codepoint
      //     que a primária não tem, na 2ª+ carga.
      if (fallback_face &&
          std::find(fallback_faces_.begin(), fallback_faces_.end(), f.get()) == fallback_faces_.end()) {
        fallback_faces_.push_back(f.get());
      }
      return true; // already registered -- RmlUi's own "Duplicate" outcome is success, not error.
    }
  }

  auto lf = std::make_unique<LoadedFace>();
  lf->blob = std::move(blob);
  if (glx_sfnt_open(lf->blob.data(), lf->blob.size(), &lf->sfnt) != GLX_SFNT_OK) return false;
  if (lf->sfnt.units_per_em == 0) return false; // guards the scale-factor division in BakeFaceInstance().

  // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.3) Derive the Y grid-fitting zones ONCE, here, so every
  //     BakeFaceInstance() of this face (at any pixel size) reuses them -- x_height/cap_height are
  //     measured from the real 'x'/'H' outlines (see DeriveHintZones()). Cheap (a handful of glyph
  //     outline reads, once per loaded FILE), never per-glyph or per-instance.
  // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.3) Deriva as zonas de grid-fitting Y UMA vez, aqui, pra
  //     todo BakeFaceInstance() desta face (em qualquer tamanho-em-px) reusá-las -- x_height/
  //     cap_height são medidos dos outlines reais de 'x'/'H' (ver DeriveHintZones()). Barato (um
  //     punhado de leituras de outline de glyph, uma vez por ARQUIVO carregado), nunca por-glyph
  //     nem por-instância.
  lf->hint_zones = DeriveHintZones(lf->sfnt);

  lf->family = family;
  lf->style = style;
  lf->weight = normalized_weight; // see the hoisted-local comment above the dedup loop.
  lf->fallback_face = fallback_face;

  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) Stash the raw pointer BEFORE the
  //     faces_.push_back() below moves `lf` out -- `.get()` on the (about-to-be-null) unique_ptr
  //     after the move would be undefined, `.get()` on the still-live one right here is not.
  //     Pointer stays valid post-move (faces_ owns a vector of unique_ptr<LoadedFace>, so the
  //     LoadedFace object itself never relocates -- only the unique_ptr handle does).
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) Guarda o ponteiro cru ANTES do
  //     faces_.push_back() abaixo mover `lf` pra fora -- `.get()` no unique_ptr (prestes a ficar
  //     nulo) depois do move seria indefinido, `.get()` no ainda-vivo aqui não é. O ponteiro
  //     continua válido pós-move (faces_ é dono de um vector de unique_ptr<LoadedFace>, então o
  //     próprio objeto LoadedFace nunca realoca -- só o handle unique_ptr).
  const LoadedFace* raw = lf.get();
  faces_.push_back(std::move(lf));
  ++own_font_engine_debug_registered_face_count(); // L1.20-FONTFLIP dedup fix -- see the hook's own doc-comment.
  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) Index into fallback_faces_ IN REGISTRATION
  //     ORDER -- see that member's own doc-comment for the full contract (mirrors RmlUi's
  //     FontProvider::fallback_font_faces).
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) Indexa em fallback_faces_ EM ORDEM DE REGISTRO
  //     -- ver o próprio doc-comment daquele membro pro contrato completo (espelha o
  //     FontProvider::fallback_font_faces do RmlUi).
  if (fallback_face) fallback_faces_.push_back(raw);
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

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) See this method's long doc-comment on its declaration
//     in font_engine_own.hpp for the full DRY-extraction rationale. Caller has ALREADY resolved
//     `gid` (glx_sfnt_glyph_id()) and confirmed it is non-zero -- this function's own `false` return
//     covers the ONE remaining "codepoint doesn't really exist for this face" outcome
//     (glx_sfnt_hmetrics() failing, gid out of the hmtx table's range). Everything below this point
//     is copied VERBATIM from the pre-refactor inline loop (same pen-snap/vsnap/Y-hint constants,
//     same kPad, same origin/scale derivation) -- see the individual EN/PT blocks below for the
//     per-step rationale, unchanged from before this extraction.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Ver o doc-comment longo deste método na sua declaração em
//     font_engine_own.hpp pro racional completo da extração DRY. Quem chama JÁ resolveu `gid`
//     (glx_sfnt_glyph_id()) e confirmou que não é zero -- o retorno `false` desta função cobre o ÚNICO
//     resultado restante de "este codepoint não existe de verdade pra esta face" (glx_sfnt_hmetrics()
//     falhando, gid fora do alcance da tabela hmtx). Tudo abaixo deste ponto é copiado AO PÉ DA LETRA
//     do laço inline pré-refactor (mesmas constantes pen-snap/vsnap/Y-hint, mesmo kPad, mesma
//     derivação de origem/escala) -- ver os blocos EN/PT individuais abaixo pro racional por-passo,
//     inalterado desde antes desta extração.
bool FontEngineOwn::RasterizeGlyph(const FaceInstance& inst, uint32_t cp, uint32_t gid, Baked& out,
                                    const LoadedFace* src) const {
  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) Resolve the default -- see this method's own
  //     doc-comment in the header for why the default cannot be spelled `inst.face` directly in the
  //     signature (a default argument expression cannot name another parameter).
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) Resolve o default -- ver o próprio doc-comment
  //     deste método no header pro motivo do default não poder ser escrito `inst.face` direto na
  //     assinatura (uma expressão de argumento default não pode nomear outro parâmetro).
  if (!src) src = inst.face;
  const glx_sfnt_face& sf = src->sfnt;
  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) CRITICAL: scale MUST divide by `src`'s own
  //     units_per_em, not inst.face's -- see this method's header doc-comment. `RegisterFace()`
  //     already rejects units_per_em==0 for every LoadedFace (including fallback ones), so this
  //     division is safe for any `src` reachable here.
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) CRÍTICO: a escala PRECISA dividir pelo próprio
  //     units_per_em de `src`, não o de inst.face -- ver o doc-comment deste método no header. O
  //     RegisterFace() já rejeita units_per_em==0 pra toda LoadedFace (incluindo as de fallback),
  //     então esta divisão é segura pra qualquer `src` alcançável aqui.
  const float scale = (float)inst.pixel_size / (float)sf.units_per_em; // caller already guards units_per_em>0.

  // EN: (L1.20-FONTFLIP, FT-F4) PEN-SNAP/VSNAP flags, read ONCE per glyph -- see
  //     own_font_engine_pensnap_bypass()/own_font_engine_vsnap_bypass()'s own doc-comments in the
  //     header for the full contract (both hooks now document THIS function, not BakeFaceInstance(),
  //     as sub-phase 2B's shared read site).
  // PT: (L1.20-FONTFLIP, FT-F4) Flags PEN-SNAP/VSNAP, lidos UMA vez por glyph -- ver os próprios
  //     doc-comments de own_font_engine_pensnap_bypass()/own_font_engine_vsnap_bypass() no header pro
  //     contrato completo (os dois hooks agora documentam ESTA função, não BakeFaceInstance(), como o
  //     ponto de leitura compartilhado da sub-fase 2B).
  const bool pen_snap = !own_font_engine_pensnap_bypass();
  const bool v_snap = !own_font_engine_vsnap_bypass();

  out = Baked{};
  out.cp = cp;
  out.gid = gid;

  uint16_t advance_units = 0;
  int16_t lsb = 0;
  if (glx_sfnt_hmetrics(&sf, gid, &advance_units, &lsb) != GLX_SFNT_OK) return false; // gid out of hmtx range.

  // EN: (L1.20-FONTFLIP, FT-F4) advance snapped to a whole pixel under pen-snap -- FreeType
  //     reports an integer advance (FreeTypeInterface.cpp: `horiAdvance >> 6`); an integer advance
  //     keeps the running pen integral in IterateGlyphs() so consecutive glyphs share one sub-pixel
  //     phase. With pen-snap bypassed it is the raw fractional advance (the pre-snap behaviour).
  // PT: (L1.20-FONTFLIP, FT-F4) advance snapado a um pixel inteiro sob pen-snap -- o FreeType
  //     reporta um advance inteiro (FreeTypeInterface.cpp: `horiAdvance >> 6`); um advance inteiro
  //     mantém a pena corrente inteira no IterateGlyphs() pra glyphs consecutivos compartilharem
  //     uma fase sub-pixel. Com o pen-snap bypassado é o advance fracionário cru (comportamento
  //     pré-snap).
  out.advance = pen_snap ? (float)std::lround((double)advance_units * scale) : (float)advance_units * scale;

  std::vector<glx_sfnt_point> points(kMaxOutlinePoints);
  std::vector<uint16_t> contour_ends(kMaxOutlineContours);
  glx_sfnt_outline outline{};
  const glx_sfnt_result r =
      glx_sfnt_glyph_outline(&sf, gid, points.data(), (uint16_t)points.size(), contour_ends.data(),
                              (uint16_t)contour_ends.size(), &outline);

  // EN: r != GLX_SFNT_OK (e.g. GLX_SFNT_ERR_BUFFER_TOO_SMALL) or an empty outline (space,
  //     num_points==0) both leave `out` at gw==gh==0 -- a "baked, zero-size" glyph: still has a
  //     valid advance, deliberately no bitmap (see GlyphInfo's own doc-comment in the header for
  //     this exact distinction). Still returns `true` -- this is a REAL glyph, not a negative-cache
  //     case.
  // PT: r != GLX_SFNT_OK (ex.: GLX_SFNT_ERR_BUFFER_TOO_SMALL) ou um outline vazio (espaço,
  //     num_points==0) ambos deixam `out` com gw==gh==0 -- um glyph "empacotado, tamanho-zero":
  //     ainda tem um avanço válido, deliberadamente sem bitmap (ver o próprio doc-comment de
  //     GlyphInfo no header pra essa distinção exata). Ainda retorna `true` -- é um glyph REAL, não
  //     um caso de cache negativo.
  if (r == GLX_SFNT_OK && outline.num_points > 0 && outline.num_contours > 0) {
    // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.4) VERTICAL (Y-axis) GRID-FITTING -- the step that
    //     makes this ticket real. glx_hint_outline() (Layer 0 SOV-HINT) nudges each point's Y
    //     IN PLACE so the face's zones (baseline / x-height / cap-height / ascender / descender)
    //     land on whole device pixels; X is untouched. It is fed the SAME device scale
    //     (scale_num, scale_den) = (pixel_size*64, units_per_em) the rasteriser below consumes,
    //     so the alignment survives the rasteriser's own re-scale of the (now hinted) font-unit
    //     points by construction (see include/core/hint.h's COORDINATE SPACES note). CRUCIALLY
    //     the glyph BBOX below is still read from `outline` (the glyf-header x_min/y_max, NOT
    //     recomputed from the hinted points): the sub-pixel correction the hint applies and the
    //     sub-pixel origin/offset the bbox implies CANCEL algebraically at composition time
    //     (origin_y places y_max at row kPad, the quad's offset_y subtracts the same y_max, so a
    //     zone snapped to a whole pixel above the baseline lands on a whole screen pixel WHEN the
    //     baseline itself is integer-aligned) -- the whole point of writing the correction back
    //     into font units rather than moving the bitmap. kPad (1px) absorbs the <=0.5px extremal
    //     shift a snap can introduce, so no glyph clips. ON by DEFAULT (the mature behaviour);
    //     own_font_engine_hint_bypass() flips it off for the A/B "own, no hint" leg.
    //     GLX_HINT_NOOP (nothing to fit / already fitted) leaves `points` byte-for-byte
    //     unchanged -- a clean, non-error outcome we neither check nor need to.
    // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.4) GRID-FITTING VERTICAL (eixo Y) -- o passo que
    //     torna esta tarefa real. glx_hint_outline() (SOV-HINT da Camada 0) empurra o Y de cada
    //     ponto IN PLACE pra que as zonas da face (baseline / altura-x / altura-de-maiúscula /
    //     ascender / descender) caiam em pixels device inteiros; o X é intocado. Recebe a MESMA
    //     escala device (scale_num, scale_den) = (pixel_size*64, units_per_em) que o rasterizador
    //     abaixo consome, então o alinhamento sobrevive ao re-escale que o próprio rasterizador
    //     faz dos pontos (agora hintados) em unidade-de-fonte, por construção (ver a nota
    //     ESPAÇOS DE COORDENADA do include/core/hint.h). CRUCIALMENTE a BBOX do glyph abaixo
    //     ainda é lida de `outline` (o x_min/y_max do cabeçalho glyf, NÃO recomputada dos pontos
    //     hintados): a correção sub-pixel que o hint aplica e o origin/offset sub-pixel que a
    //     bbox implica se CANCELAM algebricamente no momento da composição (origin_y põe y_max na
    //     linha kPad, o offset_y do quad subtrai o mesmo y_max, então uma zona snapada a um pixel
    //     inteiro acima da baseline cai num pixel inteiro de tela QUANDO a própria baseline está
    //     alinhada a inteiro) -- exatamente o motivo de escrever a correção de volta em unidade
    //     de fonte em vez de mover o bitmap. O kPad (1px) absorve o deslocamento extremal de
    //     <=0.5px que um snap pode introduzir, então nenhum glyph corta. LIGADO por PADRÃO (o
    //     comportamento maduro); own_font_engine_hint_bypass() o desliga pra a perna A/B "próprio,
    //     sem hint". GLX_HINT_NOOP (nada a fitar / já fitado) deixa `points` byte-a-byte intacto
    //     -- um resultado limpo, sem erro, que nem checamos nem precisamos.
    if (!own_font_engine_hint_bypass()) {
      const int32_t hint_scale_num = (int32_t)std::lround((double)inst.pixel_size * 64.0);
      // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) `src->hint_zones` -- a FALLBACK-supplied
      //     glyph grid-fits against ITS OWN face's zones (baseline/x-height/cap-height measured
      //     from that face's own 'x'/'H' outlines at RegisterFace() time), not the primary's --
      //     the two font files' em-square/glyph proportions are unrelated, so the primary's zones
      //     would misplace a fallback glyph's Y grid-fit.
      // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) `src->hint_zones` -- um glyph suprido por
      //     FALLBACK se grid-fita contra as zonas da PRÓPRIA face (baseline/altura-x/altura-de-
      //     maiúscula medidas dos próprios outlines de 'x'/'H' daquela face em tempo de
      //     RegisterFace()), não as da primária -- as proporções de em-square/glyph dos dois
      //     arquivos de fonte não se relacionam, então as zonas da primária deslocariam mal o
      //     grid-fit Y de um glyph de fallback.
      glx_hint_outline(points.data(), outline.num_points, sf.units_per_em, hint_scale_num,
                       (int32_t)sf.units_per_em, &src->hint_zones);
    }

    const float fx_min = (float)outline.x_min * scale;
    const float fx_max = (float)outline.x_max * scale;
    const float fy_min = (float)outline.y_min * scale;
    const float fy_max = (float)outline.y_max * scale;
    out.y_max_px = fy_max; // see this method's own doc-comment in the header (metrics refinement).
    out.y_min_px = fy_min;

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
      // EN: (L1.20-FONTFLIP, FT-F4) vsnap: place the glyph's y_max on a WHOLE atlas row. Rounding
      //     fy_max first, THEN scaling by 64 (multiply, not lround-of-a-product), forces
      //     origin_y_26_6 to an exact multiple of 64 -- so the edge that glx_hint_outline() already
      //     grid-fitted to an integer device pixel lands on an integer bitmap row, instead of a
      //     fractional one GL_LINEAR would smear back into the texture. The X twin (origin_x_26_6
      //     above) is deliberately NOT changed -- vsnap is Y-only. kPad added as an integer keeps the
      //     result a multiple of 64. Bypassed (own_font_engine_vsnap_bypass()==true) == the raw
      //     fractional origin (round the whole (fy_max+kPad)*64 product, as before).
      // PT: (L1.20-FONTFLIP, FT-F4) vsnap: põe o y_max do glyph numa row INTEIRA do atlas. Arredondar
      //     fy_max primeiro, DEPOIS escalar por 64 (multiplicar, não lround-de-um-produto), força
      //     origin_y_26_6 a um múltiplo exato de 64 -- então a aresta que o glx_hint_outline() já
      //     grid-fitou a um pixel device inteiro pousa numa row inteira do bitmap, em vez de uma
      //     fracionária que o GL_LINEAR borraria de volta na textura. O gêmeo X (origin_x_26_6 acima)
      //     NÃO é mudado de propósito -- vsnap é só-Y. O kPad somado como inteiro mantém o resultado
      //     múltiplo de 64. Bypassado (own_font_engine_vsnap_bypass()==true) == a origem fracionária
      //     crua (arredonda o produto inteiro (fy_max+kPad)*64, como antes).
      const int32_t origin_y_26_6 = v_snap ? ((int32_t)std::lround((double)fy_max) + kPad) * 64
                                           : (int32_t)std::lround(((double)fy_max + kPad) * 64.0);

      std::vector<uint8_t> bitmap((size_t)gw * (size_t)gh, 0);
      const size_t scratch_n = glx_raster_scratch_floats(gw, gh);
      if (scratch_n > 0) {
        std::vector<float> scratch(scratch_n);
        const glx_raster_result rr =
            glx_rasterize_outline(points.data(), outline.num_points, contour_ends.data(), outline.num_contours,
                                   scale_num, scale_den, origin_x_26_6, origin_y_26_6, bitmap.data(), gw, gh, gw,
                                   scratch.data(), scratch.size());
        if (rr == GLX_RASTER_OK) {
          out.gw = gw;
          out.gh = gh;
          out.coverage = std::move(bitmap);
          // EN: quad top-left offset from (pen_x, baseline_y), screen space y-down -- derived
          //     in font_engine_own.hpp's GlyphInfo doc-comment / IterateGlyphs' contract.
          // PT: offset do canto superior-esquerdo do quad em relação a (pen_x, baseline_y),
          //     espaço de tela y-para-baixo -- derivado no doc-comment de GlyphInfo em
          //     font_engine_own.hpp / no contrato de IterateGlyphs.
          // EN: (L1.20-FONTFLIP, FT-F4) left bearing snapped to a whole pixel under pen-snap --
          //     the quad's left edge is `pen_x + offset_x`; with `pen_x` already integral (snapped
          //     advance/kern/letter_spacing) a whole-pixel bearing puts the glyph's ink origin on an
          //     integer column (offset by the shared frac(position.x)), so GL_LINEAR does not smear
          //     the stems -- FreeType likewise rounds its bearing. Rounding fx_min (then subtracting
          //     the integer kPad) keeps offset_x integral. The bitmap itself is untouched (still
          //     rasterised at its natural sub-pixel origin, origin_x_26_6 below unchanged): pen-snap
          //     only quantises WHERE the quad is placed, not the glyph's internal shape (X-axis stem
          //     grid-fit is a separate, future sub-phase). Bypassed == the raw fractional bearing.
          // PT: (L1.20-FONTFLIP, FT-F4) bearing esquerdo snapado a um pixel inteiro sob pen-snap --
          //     a aresta esquerda do quad é `pen_x + offset_x`; com `pen_x` já inteiro
          //     (advance/kern/letter_spacing snapados) um bearing de pixel inteiro põe a origem da
          //     tinta do glyph numa coluna inteira (deslocada pelo frac(position.x) compartilhado),
          //     então o GL_LINEAR não borra as hastes -- o FreeType também arredonda o bearing dele.
          //     Arredondar fx_min (depois subtrair o kPad inteiro) mantém offset_x inteiro. O próprio
          //     bitmap fica intocado (ainda rasterizado na origem sub-pixel natural, origin_x_26_6
          //     abaixo inalterado): o pen-snap só quantiza ONDE o quad é posto, não a forma interna do
          //     glyph (grid-fit de haste no eixo X é uma sub-fase futura, à parte). Bypassado == o
          //     bearing fracionário cru.
          out.offset_x = (pen_snap ? (float)std::lround((double)fx_min) : fx_min) - kPad;
          // EN: (L1.20-FONTFLIP, FT-F4) vsnap: quad top offset from the baseline snapped to a whole
          //     pixel, the vertical twin of the pen-snapped offset_x above. With fy_max rounded, the
          //     quad's top edge is an integer distance above the baseline (`-position.y` in
          //     GenerateString shares the same phase for every glyph), so the Y-hinted rows blit to
          //     integer screen rows rather than a per-size fractional phase that would re-smear them.
          //     Rounding cancels algebraically with the rounded origin_y_26_6 above (both derived
          //     from the same rounded fy_max), preserving the origin/offset cancellation the bake
          //     relies on. Bypassed == the raw fractional offset. X axis (offset_x) untouched here.
          // PT: (L1.20-FONTFLIP, FT-F4) vsnap: offset do topo do quad em relação à baseline snapado a
          //     um pixel inteiro, o gêmeo vertical do offset_x pen-snapado acima. Com fy_max
          //     arredondado, a aresta de topo do quad fica a uma distância inteira acima da baseline
          //     (o `-position.y` no GenerateString compartilha a mesma fase pra todo glyph), então as
          //     rows hintadas em Y blittam pra rows inteiras de tela em vez de uma fase fracionária
          //     por-tamanho que as borraria de novo. O arredondamento cancela algebricamente com o
          //     origin_y_26_6 arredondado acima (ambos derivados do mesmo fy_max arredondado),
          //     preservando o cancelamento origem/offset de que o bake depende. Bypassado == o offset
          //     fracionário cru. Eixo X (offset_x) intocado aqui.
          out.offset_y = v_snap ? -((float)std::lround((double)fy_max) + kPad) : -(fy_max + kPad);
        }
      }
    }
  }
  return true; // gid resolved + hmetrics OK -- a real, "baked" glyph (possibly zero-size).
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

  std::vector<Baked> baked;
  baked.reserve(128);

  for (uint32_t cp : BuildBakeSet()) {
    const uint32_t gid = glx_sfnt_glyph_id(&sf, cp);

    // EN: gid 0 is .notdef -- glx_sfnt_glyph_id() returns it for any codepoint the face's cmap
    //     does not map (include/core/sfnt.h's documented convention). With the widened bake set
    //     (Latin-1 Supplement + typographic punctuation) some listed codepoints may legitimately
    //     be absent from a given face, so skip them here rather than baking .notdef's own box
    //     outline under a real codepoint -- that would pollute the atlas with a visible tofu box
    //     and make FindGlyph() return a bogus quad for a character the font never had. Skipping
    //     leaves the codepoint out of inst.glyphs entirely -- but (sub-phase 2B) that is now merely
    //     "not yet tried": EnsureGlyphs()/BakeGlyph() will negative-cache it (`baked=false`) the
    //     first time this codepoint actually appears in a string, so it is never re-attempted after
    //     that either. FindGlyph() returns nullptr either way: the string still measures/renders,
    //     that one character is simply blank (advance 0, no quad) -- the exact "codepoint the face
    //     lacks" degradation the header's SCOPE promises. cp 0 is never in BuildBakeSet(), so this
    //     never wrongly drops a real glyph mapped to gid 0.
    // PT: gid 0 é .notdef -- glx_sfnt_glyph_id() o retorna pra qualquer codepoint que o cmap da
    //     face não mapeia (convenção documentada de include/core/sfnt.h). Com o conjunto ampliado
    //     (Latin-1 Supplement + pontuação tipográfica) alguns codepoints listados podem
    //     legitimamente faltar numa dada face, então pula-os aqui em vez de empacotar o próprio
    //     outline de caixa do .notdef sob um codepoint real -- isso poluiria o atlas com uma caixa
    //     tofu visível e faria o FindGlyph() devolver um quad espúrio pra um caractere que a fonte
    //     nunca teve. Pular deixa o codepoint fora de inst.glyphs -- mas (sub-fase 2B) isso agora é
    //     só "ainda não tentado": EnsureGlyphs()/BakeGlyph() o cache-negativarão (`baked=false`) na
    //     1ª vez que este codepoint de fato aparecer numa string, então também nunca é reatentado
    //     depois disso. FindGlyph() devolve nullptr de qualquer forma: a string ainda
    //     mede/renderiza, aquele caractere só fica em branco (avanço 0, sem quad) -- exatamente a
    //     degradação "codepoint que a face não tem" que o SCOPE do header promete. cp 0 nunca está
    //     em BuildBakeSet(), então isto nunca larga por engano um glyph real mapeado a gid 0.
    if (gid == 0) continue;

    Baked b;
    // EN: (sub-phase 2B) RasterizeGlyph() does everything the inline loop used to -- hmetrics,
    //     outline, Y-hint, pen-snap/vsnap, rasterization -- and returns `false` only for the OTHER
    //     "codepoint doesn't exist" outcome (hmtx lookup failure); see its own doc-comment.
    // PT: (sub-fase 2B) RasterizeGlyph() faz tudo que o laço inline fazia -- hmetrics, outline,
    //     Y-hint, pen-snap/vsnap, rasterização -- e retorna `false` só pro OUTRO resultado
    //     "codepoint não existe" (falha de lookup no hmtx); ver o próprio doc-comment.
    if (!RasterizeGlyph(inst, cp, gid, b)) continue;

    // EN: Metrics refinement -- gated on b.gw>0 (raster actually produced a bitmap), matching the
    //     pre-refactor `rr==GLX_RASTER_OK` gate exactly (RasterizeGlyph() only sets gw/gh>0 there).
    // PT: Refinamento de métricas -- protegido por b.gw>0 (a rasterização de fato produziu um
    //     bitmap), batendo exatamente com o gate `rr==GLX_RASTER_OK` pré-refactor (RasterizeGlyph()
    //     só seta gw/gh>0 lá).
    if (b.gw > 0 && b.gh > 0) {
      if (cp == 0x78) inst.metrics.x_height = b.y_max_px - b.y_min_px; // 'x' baked -- refine x_height.
      // EN: U+2026 rasterised for this face -- RmlUi may use the native ellipsis glyph.
      // PT: U+2026 rasterizou pra esta face -- o RmlUi pode usar o glyph nativo de reticências.
      if (cp == 0x2026) inst.metrics.has_ellipsis = true;
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
  //     so the initial texture size did NOT need to grow for the larger set. (Sub-phase 2B) The
  //     final pen_x/pen_y/shelf_h below is stashed into inst.pen_x/pen_y/shelf_h -- the PERSISTENT
  //     shelf cursor BakeGlyph() continues from for every subsequent lazily-baked glyph, so a
  //     top-up glyph is APPENDED after the warm set, never re-packed from scratch.
  // PT: Passe de empacotamento em prateleiras -- prateleiras de largura fixa (cresce pra caber
  //     o glyph mais largo, se algum exceder o padrão de 512px), linhas avançam pra baixo à
  //     medida que uma prateleira enche. Determinístico, passe único, suficiente pro conjunto
  //     fixo de ~199 codepoints desta tarefa após a ampliação da sub-fase 2.1 (sem necessidade de
  //     um bin packer de propósito geral aqui). (Sub-fase 2B) O pen_x/pen_y/shelf_h final abaixo é
  //     guardado em inst.pen_x/pen_y/shelf_h -- o cursor de prateleira PERSISTENTE do qual o
  //     BakeGlyph() continua pra todo glyph seguinte empacotado preguiçosamente, então um glyph de
  //     top-up é ANEXADO depois do warm set, nunca reempacotado do zero.
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
  inst.pen_x = pen_x;
  inst.pen_y = pen_y;
  inst.shelf_h = shelf_h;

  // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 1.5) Stem-darkening coverage LUT for THIS instance's
  //     pixel size -- built ONCE here, applied per-texel in the blit below (a single array lookup
  //     per pixel). OPT-IN, OFF by default: own_font_engine_darken_enable() must be flipped `true`
  //     (only the fonteng_ab_visual "own_yhint_dark" A/B leg does) to apply the curve; otherwise
  //     the LUT is the identity map and the blit is byte-identical to the pre-1.5 path. The
  //     darkening is optional machinery -- measured NOT to close the stem-sharpness gap to FreeType
  //     (structural: only X-axis stem grid-fit consolidates a 0.5/0.5-split stem into a crisp
  //     column; a coverage curve only adds weight). See this function's/the hook's header.
  //     (Sub-phase 2B) Stored in inst.cov_lut (a member, not a local array) so BakeGlyph() can
  //     reuse the IDENTICAL curve for a lazily-baked glyph's blit below.
  // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 1.5) LUT de cobertura de stem-darkening pro tamanho-em-px
  //     DESTA instância -- construída UMA vez aqui, aplicada por-texel no blit abaixo (uma consulta
  //     de array por pixel). OPT-IN, OFF por padrão: own_font_engine_darken_enable() precisa ser
  //     virado `true` (só a perna A/B "own_yhint_dark" do fonteng_ab_visual o faz) pra aplicar a
  //     curva; senão a LUT é o mapa identidade e o blit é byte-idêntico ao caminho pré-1.5. O
  //     darkening é maquinaria opcional -- medido como NÃO fechando o gap de nitidez de haste pro
  //     FreeType (estrutural: só o grid-fit de haste no eixo X consolida uma haste dividida 0.5/0.5
  //     numa coluna nítida; uma curva de cobertura só adiciona peso). Ver o header desta função/hook.
  //     (Sub-fase 2B) Guardada em inst.cov_lut (um membro, não um array local) pra o BakeGlyph()
  //     reusar a curva IDÊNTICA no blit de um glyph empacotado preguiçosamente abaixo.
  BuildCoverageLut(inst.cov_lut, inst.pixel_size, own_font_engine_darken_enable());

  for (const Baked& b : baked) {
    GlyphInfo gi{};
    gi.baked = true;
    gi.gid = b.gid;
    // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) The warm set NEVER falls back (see this
    //     file's header "SCOPE"/anti-OE note) -- every glyph it bakes came from the FaceInstance's
    //     own primary face.
    // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) O warm set NUNCA recorre a fallback (ver a
    //     nota "ESCOPO"/anti-OE deste header) -- todo glyph que ele empacota veio da própria face
    //     primária da FaceInstance.
    gi.src_face = inst.face;
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
          // EN: (sub-phase 1.5) coverage -> darkened coverage via the per-instance LUT (identity
          //     when darkening is bypassed or the size is >= kPxHi). Still (cov,cov,cov,cov)
          //     premultiplied-white -- the darkening only reshapes the coverage byte, not the trick.
          // PT: (sub-fase 1.5) cobertura -> cobertura escurecida via a LUT por-instância (identidade
          //     quando o darkening é pulado ou o tamanho é >= kPxHi). Continua (cov,cov,cov,cov)
          //     branco-premultiplicado -- o darkening só remodela o byte de cobertura, não o truque.
          const uint8_t cov = inst.cov_lut[src_row[x]];
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

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) See this method's long doc-comment on its declaration
//     in font_engine_own.hpp for the full contract.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Ver o doc-comment longo deste método na sua declaração
//     em font_engine_own.hpp pro contrato completo.
void FontEngineOwn::BakeGlyph(FaceInstance& inst, uint32_t cp) const {
  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) Resolve against the PRIMARY face first --
  //     exactly the pre-fallback behaviour, unchanged.
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) Resolve contra a face PRIMÁRIA primeiro --
  //     exatamente o comportamento pré-fallback, inalterado.
  uint32_t gid = glx_sfnt_glyph_id(&inst.face->sfnt, cp);
  const LoadedFace* src = inst.face;

  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) THE gap this ticket closes: when the primary
  //     face lacks the glyph (gid==0), walk `fallback_faces_` IN REGISTRATION ORDER -- mirrors the
  //     pinned RmlUi source's own FontFaceHandleDefault.cpp glyph-shaping loop (confirmed reading
  //     it: iterates fallback_font_faces in order, uses the first that resolves the character) --
  //     and take the FIRST fallback face that resolves a non-zero gid for `cp`. `fb == inst.face`
  //     is skipped so a face that is ALSO its own instance's primary (e.g. registered both as the
  //     document's declared family AND, separately, as a fallback) is never re-tried a second time
  //     under the same identity -- it already had its one shot above. `gid` and `src` are left as
  //     the primary's (gid==0, src==inst.face) if NO fallback face has the glyph either, which the
  //     `if (gid == 0)` check right below still catches -- identical negative-cache outcome as
  //     before this loop existed, now just reached after exhausting every registered fallback too.
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) O gap que esta tarefa fecha: quando a face
  //     primária não tem o glyph (gid==0), percorre `fallback_faces_` EM ORDEM DE REGISTRO --
  //     espelha o próprio laço de shaping de glyph do FontFaceHandleDefault.cpp do source pinado do
  //     RmlUi (confirmado lendo-o: itera fallback_font_faces em ordem, usa a primeira que resolve o
  //     caractere) -- e pega a PRIMEIRA face de fallback que resolve um gid não-zero pra `cp`.
  //     `fb == inst.face` é pulada pra uma face que TAMBÉM é a primária da própria instância (ex.:
  //     registrada tanto como a família declarada do documento QUANTO, separadamente, como
  //     fallback) nunca ser retentada uma 2ª vez sob a mesma identidade -- ela já teve sua única
  //     chance acima. `gid` e `src` ficam como os da primária (gid==0, src==inst.face) se NENHUMA
  //     face de fallback tiver o glyph também, o que o cheque `if (gid == 0)` logo abaixo ainda
  //     captura -- resultado de cache-negativo idêntico ao de antes deste laço existir, agora só
  //     alcançado depois de esgotar toda fallback registrada também.
  if (gid == 0) {
    for (const LoadedFace* fb : fallback_faces_) {
      if (fb == inst.face) continue;
      const uint32_t fb_gid = glx_sfnt_glyph_id(&fb->sfnt, cp);
      if (fb_gid != 0) {
        gid = fb_gid;
        src = fb;
        break;
      }
    }
  }

  if (gid == 0) {
    inst.glyphs.emplace(cp, GlyphInfo{}); // negative-cache: no face (primary nor fallback) has `cp`.
    return;
  }

  Baked b;
  if (!RasterizeGlyph(inst, cp, gid, b, src)) {
    inst.glyphs.emplace(cp, GlyphInfo{}); // negative-cache: hmtx lookup failed (gid out of range).
    return;
  }

  GlyphInfo gi{};
  gi.baked = true;
  gi.gid = b.gid;
  gi.src_face = src; // primary or the resolving fallback face -- see GlyphInfo::src_face.
  gi.advance = b.advance;

  if (b.gw > 0 && b.gh > 0) {
    // EN: Persistent shelf cursor -- see FaceInstance::pen_x/pen_y/shelf_h's own doc-comment. Same
    //     shelf-pack algorithm BakeFaceInstance()'s warm-set pass uses, just one glyph at a time
    //     and continuing from wherever the cursor already is (never reset to (0,0)).
    // PT: Cursor de prateleira persistente -- ver o próprio doc-comment de
    //     FaceInstance::pen_x/pen_y/shelf_h. Mesmo algoritmo de empacotamento em prateleiras que o
    //     passe do warm set do BakeFaceInstance() usa, só um glyph de cada vez e continuando de
    //     onde o cursor já está (nunca resetado pra (0,0)).
    if (inst.pen_x + b.gw > inst.atlas_w) {
      inst.pen_y += inst.shelf_h;
      inst.pen_x = 0;
      inst.shelf_h = 0;
    }
    if (inst.pen_y + b.gh > inst.atlas_h || b.gw > inst.atlas_w) {
      const int new_w = std::max(inst.atlas_w, b.gw);
      const int new_h = std::max(inst.atlas_h * 2, inst.pen_y + b.gh); // amortized doubling.
      GrowAtlas(inst, new_w, new_h);
    }

    b.atlas_x = inst.pen_x;
    b.atlas_y = inst.pen_y;
    inst.pen_x += b.gw;
    inst.shelf_h = std::max(inst.shelf_h, b.gh);

    gi.atlas_x = b.atlas_x;
    gi.atlas_y = b.atlas_y;
    gi.w = b.gw;
    gi.h = b.gh;
    gi.offset_x = b.offset_x;
    gi.offset_y = b.offset_y;

    // EN: Same premultiplied-white coverage blit as BakeFaceInstance()'s warm-set loop, through the
    //     SAME per-instance inst.cov_lut -- see that function's own comment for the full rationale.
    // PT: Mesmo blit de cobertura branco-premultiplicado do laço do warm set do BakeFaceInstance(),
    //     pela MESMA inst.cov_lut por-instância -- ver o comentário próprio daquela função pro
    //     racional completo.
    for (int y = 0; y < b.gh; ++y) {
      const uint8_t* src_row = &b.coverage[(size_t)y * b.gw];
      uint8_t* dst_row = &inst.atlas_rgba[((size_t)(b.atlas_y + y) * inst.atlas_w + (size_t)b.atlas_x) * 4];
      for (int x = 0; x < b.gw; ++x) {
        const uint8_t cov = inst.cov_lut[src_row[x]];
        dst_row[x * 4 + 0] = cov;
        dst_row[x * 4 + 1] = cov;
        dst_row[x * 4 + 2] = cov;
        dst_row[x * 4 + 3] = cov;
      }
    }
  }

  inst.glyphs.emplace(cp, gi);
}

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) See this method's long doc-comment on its declaration
//     in font_engine_own.hpp for the full contract.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Ver o doc-comment longo deste método na sua declaração
//     em font_engine_own.hpp pro contrato completo.
void FontEngineOwn::GrowAtlas(FaceInstance& inst, int new_w, int new_h) const {
  new_w = std::max(new_w, inst.atlas_w);
  new_h = std::max(new_h, inst.atlas_h);
  if (new_w == inst.atlas_w && new_h == inst.atlas_h) return; // nothing to do.

  std::vector<uint8_t> grown((size_t)new_w * (size_t)new_h * 4, 0);
  // EN: Row-by-row (strided) copy of the used region -- every already-baked glyph's atlas_x/atlas_y
  //     stays a valid pixel coordinate verbatim in the new buffer (no glyph is ever moved/repacked).
  // PT: Cópia linha-a-linha (com stride) da região usada -- todo atlas_x/atlas_y de glyph já
  //     empacotado continua uma coordenada de pixel válida ao pé da letra no buffer novo (nenhum
  //     glyph é jamais movido/reempacotado).
  for (int y = 0; y < inst.atlas_h; ++y) {
    const uint8_t* src_row = &inst.atlas_rgba[(size_t)y * inst.atlas_w * 4];
    uint8_t* dst_row = &grown[(size_t)y * new_w * 4];
    std::copy(src_row, src_row + (size_t)inst.atlas_w * 4, dst_row);
  }
  inst.atlas_rgba = std::move(grown);
  inst.atlas_w = new_w;
  inst.atlas_h = new_h;
}

const FontEngineOwn::GlyphInfo* FontEngineOwn::FindGlyph(const FaceInstance& inst, uint32_t codepoint) {
  const auto it = inst.glyphs.find(codepoint);
  return (it != inst.glyphs.end() && it->second.baked) ? &it->second : nullptr;
}

// EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) See this method's long doc-comment on its declaration
//     in font_engine_own.hpp for the full contract. Uses the SAME Rml::StringIteratorU8 walk
//     IterateGlyphs() (below) uses, deliberately duplicated rather than shared -- this pass only
//     needs the decoded codepoint (no pen/emit/kerning bookkeeping), and running it separately here,
//     BEFORE IterateGlyphs() ever starts, is what guarantees every glyph the walk below consults is
//     already resolved.
// PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Ver o doc-comment longo deste método na sua declaração em
//     font_engine_own.hpp pro contrato completo. Usa a MESMA caminhada Rml::StringIteratorU8 que o
//     IterateGlyphs() (abaixo) usa, deliberadamente duplicada em vez de compartilhada -- este passe
//     só precisa do codepoint decodificado (sem a contabilidade de pen/emit/kerning), e rodá-lo
//     separadamente aqui, ANTES do IterateGlyphs() sequer começar, é o que garante que todo glyph
//     que a caminhada abaixo consulta já está resolvido.
void FontEngineOwn::EnsureGlyphs(FaceInstance& inst, Rml::StringView string) const {
  bool baked_any_real = false;
  for (Rml::StringIteratorU8 it(string.begin(), string.begin(), string.end()); it; ++it) {
    const uint32_t cp = (uint32_t)(*it);
    if (inst.glyphs.find(cp) != inst.glyphs.end()) continue; // already tried (real or negative-cached).
    BakeGlyph(inst, cp);
    const auto found = inst.glyphs.find(cp);
    if (found != inst.glyphs.end() && found->second.baked) baked_any_real = true;
  }
  if (baked_any_real) {
    // EN: RmlUi's own "regenerate cached text geometry" signal (GetVersion()) + this engine's own
    //     "re-upload the GPU texture" signal (GenerateString()) -- see FaceInstance::version/
    //     atlas_dirty's own doc-comment in the header.
    // PT: O próprio sinal "regenere a geometria de texto cacheada" do RmlUi (GetVersion()) + o
    //     próprio sinal "reenvie a textura GPU" deste motor (GenerateString()) -- ver o próprio
    //     doc-comment de FaceInstance::version/atlas_dirty no header.
    inst.version++;
    inst.atlas_dirty = true;
  }
}

float FontEngineOwn::IterateGlyphs(const FaceInstance& inst, Rml::StringView string, float letter_spacing,
                                    const std::function<void(float pen_x, const GlyphInfo*)>& emit) {
  // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) NO instance-wide font-unit->px `scale` is
  //     hoisted here anymore -- kerning's own scale is now derived PER PAIR, below, from
  //     `g->src_face->sfnt.units_per_em` (the face the CURRENT glyph actually came from), not
  //     `inst.face`'s. Reason: a kerned pair only ever forms between two glyphs from the SAME
  //     src_face (see the `prev_g->src_face == g->src_face` gate below) -- but that shared face may
  //     be a FALLBACK face with a units_per_em different from the instance's primary (e.g. Open
  //     Sans 2048 vs. a fallback face's own 1600), and using the wrong denominator would silently
  //     mis-scale the FUnits kern adjustment glx_sfnt_kern() returns, exactly the class of bug
  //     RasterizeGlyph()'s own `src`-scoped scale exists to avoid for bake-time metrics.
  // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) NENHUM `scale` unidade-de-fonte->px de âmbito
  //     de instância é mais içado aqui -- a própria escala do kerning agora é derivada POR PAR,
  //     abaixo, de `g->src_face->sfnt.units_per_em` (a face de que o glyph ATUAL de fato veio), não
  //     a de `inst.face`. Motivo: um par kernado só se forma entre dois glyphs da MESMA src_face
  //     (ver a guarda `prev_g->src_face == g->src_face` abaixo) -- mas essa face compartilhada pode
  //     ser uma face de FALLBACK com um units_per_em diferente do da primária da instância (ex.:
  //     Open Sans 2048 vs. o 1600 próprio de uma face de fallback), e usar o denominador errado
  //     dimensionaria mal, silenciosamente, o ajuste de kern em FUnits que o glx_sfnt_kern()
  //     devolve -- exatamente a classe de bug que a escala escopada-por-`src` do próprio
  //     RasterizeGlyph() existe pra evitar nas métricas de tempo de bake.

  // EN: (L1.20-FONTFLIP, FT-F4) PEN-SNAP -- the other half of the flag BakeFaceInstance() already
  //     read for `advance`/`offset_x`. Snapping `letter_spacing` to a whole pixel here (once) and
  //     the per-pair `kern` below keeps every pen increment integral, so `pen` is an integer at
  //     each emit() (advance is already integral from bake) -- the FreeType-like phase-locked walk.
  //     Both GetStringWidth() and GenerateString() run THIS same summation, so the width invariance
  //     holds regardless of snap state (see this method's doc-comment in the header).
  // PT: (L1.20-FONTFLIP, FT-F4) PEN-SNAP -- a outra metade do flag que o BakeFaceInstance() já leu
  //     pro `advance`/`offset_x`. Snapar `letter_spacing` a um pixel inteiro aqui (uma vez) e o
  //     `kern` por-par abaixo mantém todo incremento de pen inteiro, então `pen` é um inteiro em
  //     todo emit() (o advance já é inteiro do bake) -- a caminhada com fase travada, FreeType-like.
  //     Tanto GetStringWidth() quanto GenerateString() rodam ESTA mesma soma, então a invariância de
  //     largura vale independente do estado do snap (ver o doc-comment deste método no header).
  const bool pen_snap = !own_font_engine_pensnap_bypass();
  const float ls = pen_snap ? (float)std::lround((double)letter_spacing) : letter_spacing;

  float pen = 0.f;
  bool has_prev = false;
  const GlyphInfo* prev_g = nullptr;
  for (Rml::StringIteratorU8 it(string.begin(), string.begin(), string.end()); it; ++it) {
    const Rml::Character c = *it;
    const uint32_t cp = (uint32_t)c;
    const GlyphInfo* g = FindGlyph(inst, cp);

    // EN: Kerning applies ONLY between two CONSECUTIVE baked glyphs (both have a real gid) SOURCED
    //     FROM THE SAME FACE (L1.20-FONTFLIP, FT-F4, per-glyph fallback: `prev_g->src_face ==
    //     g->src_face` -- the classic `kern` table is keyed by gid WITHIN one physical font file,
    //     so a gid pair straddling a primary/fallback boundary, or two DIFFERENT fallback faces,
    //     shares no meaningful gid-space and must not be looked up together; see
    //     GlyphInfo::src_face's own doc-comment) -- folded into `pen` BEFORE `emit()` so the
    //     current glyph's own quad/measurement already reflects the tightened (or loosened) gap to
    //     its predecessor. See this method's doc-comment in font_engine_own.hpp for the
    //     no-rounding-mid-loop rationale.
    // PT: Kerning se aplica SÓ entre dois glyphs CONSECUTIVOS empacotados (ambos com gid real)
    //     ORIGINADOS DA MESMA FACE (L1.20-FONTFLIP, FT-F4, fallback por-glyph:
    //     `prev_g->src_face == g->src_face` -- a tabela clássica `kern` é indexada por gid DENTRO
    //     de um único arquivo de fonte físico, então um par de gid atravessando uma fronteira
    //     primária/fallback, ou duas faces de fallback DIFERENTES, não compartilha espaço-de-gid
    //     significativo e não pode ser buscado junto; ver o próprio doc-comment de
    //     GlyphInfo::src_face) -- dobrado em `pen` ANTES do `emit()` pra que o próprio
    //     quad/medição do glyph atual já reflita o vão apertado (ou afrouxado) até o predecessor.
    //     Ver o doc-comment deste método em font_engine_own.hpp pro motivo do
    //     sem-arredondamento-no-meio-do-laço.
    if (has_prev && g && prev_g->src_face != nullptr && prev_g->src_face == g->src_face) {
      // EN: (L1.20-FONTFLIP, FT-F4, per-glyph fallback) per-pair scale derived from THIS shared
      //     src_face's own units_per_em -- see this function's own doc-comment above for why.
      // PT: (L1.20-FONTFLIP, FT-F4, fallback por-glyph) escala por-par derivada do próprio
      //     units_per_em desta src_face compartilhada -- ver o doc-comment desta função acima pro
      //     motivo.
      const float kern_scale = (float)inst.pixel_size / (float)g->src_face->sfnt.units_per_em;
      const int16_t kern_funits = glx_sfnt_kern(&g->src_face->sfnt, prev_g->gid, g->gid);
      // EN: (L1.20-FONTFLIP, FT-F4) kern folded in snapped to a whole pixel under pen-snap --
      //     FreeType's kern advance is likewise integer (`>> 6`). Bypassed == the raw fractional
      //     kern (the pre-snap behaviour).
      // PT: (L1.20-FONTFLIP, FT-F4) kern dobrado snapado a um pixel inteiro sob pen-snap -- o kern
      //     do FreeType também é inteiro (`>> 6`). Bypassado == o kern fracionário cru (comportamento
      //     pré-snap).
      if (kern_funits != 0) {
        const float kern_px = (float)kern_funits * kern_scale;
        pen += pen_snap ? (float)std::lround((double)kern_px) : kern_px;
      }
    }

    emit(pen, g);
    pen += (g ? g->advance : 0.0f) + ls;

    has_prev = (g != nullptr);
    prev_g = g;
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
  // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) Non-const now -- EnsureGlyphs() may lazily top-up-bake
  //     a codepoint missing from this instance's atlas. Still never touches the GPU (no
  //     Rml::RenderManager& is even passed to this override) -- see EnsureGlyphs()'s own doc-comment.
  // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Não-const agora -- o EnsureGlyphs() pode empacotar um
  //     codepoint ausente do atlas desta instância por-demanda. Continua nunca tocando a GPU (este
  //     override nem recebe um Rml::RenderManager&) -- ver o próprio doc-comment de EnsureGlyphs().
  auto* inst = reinterpret_cast<FaceInstance*>(handle);
  if (!inst) return 0;
  EnsureGlyphs(*inst, string);
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

  // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) Lazy top-up BEFORE any GPU/UV read -- so by the time
  //     the atlas_w_f/atlas_h_f below are captured and the texture is (re-)created, both already
  //     reflect whatever EnsureGlyphs() just baked/grew. See EnsureGlyphs()'s own doc-comment.
  // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Top-up preguiçoso ANTES de qualquer leitura de GPU/UV
  //     -- pra quando o atlas_w_f/atlas_h_f abaixo forem capturados e a textura for (re)criada,
  //     ambos já refletirem o que o EnsureGlyphs() acabou de empacotar/crescer. Ver o próprio
  //     doc-comment de EnsureGlyphs().
  EnsureGlyphs(*inst, string);

  // EN: GPU texture (re-)creation -- was a lazy, ONE-SHOT creation before sub-phase 2B (see
  //     font_engine_own.hpp's "SCOPE" section for why it cannot happen earlier than the first
  //     GenerateString() call: GetFontFaceHandle() is not passed a RenderManager&). (Sub-phase 2B)
  //     Now ALSO re-creates whenever inst.atlas_dirty is set (EnsureGlyphs() just baked/grew the
  //     CPU-side atlas) -- a stale GL texture would otherwise keep serving the atlas's PRE-top-up
  //     pixels/dimensions forever. The stale texture is explicitly reset to an empty
  //     Rml::CallbackTexture{} first (same reset-then-recreate mechanism Shutdown() uses,
  //     UniqueRenderResource's own RAII contract auto-releasing the old GL resource) so
  //     MakeCallbackTexture() below issues a genuinely fresh GL texture sized to the CURRENT
  //     (possibly grown) atlas dimensions, rather than trying to resize one in place. The lambda
  //     captures `inst` (a stable pointer -- `instances_` holds it by std::unique_ptr, never
  //     reallocated/moved) rather than copying atlas_rgba, so it reads whatever is current in
  //     inst->atlas_rgba at the time the callback actually runs.
  // PT: (Re)criação de textura GPU -- era uma criação preguiçosa, ÚNICA, antes da sub-fase 2B (ver a
  //     seção "ESCOPO" de font_engine_own.hpp pro motivo de não poder acontecer antes da 1ª chamada
  //     de GenerateString(): GetFontFaceHandle() não recebe um RenderManager&). (Sub-fase 2B) Agora
  //     TAMBÉM recria sempre que inst.atlas_dirty está setado (o EnsureGlyphs() acabou de
  //     empacotar/crescer o atlas lado-CPU) -- uma textura GL obsoleta senão continuaria servindo os
  //     pixels/dimensões PRÉ-top-up do atlas pra sempre. A textura obsoleta é explicitamente
  //     resetada pra um Rml::CallbackTexture{} vazio primeiro (mesmo mecanismo reset-depois-recria
  //     que o Shutdown() usa, o próprio contrato RAII do UniqueRenderResource auto-liberando o
  //     recurso GL antigo) pra o MakeCallbackTexture() abaixo emitir uma textura GL genuinamente
  //     nova, dimensionada pro atlas ATUAL (possivelmente crescido), em vez de tentar redimensionar
  //     uma no lugar. A lambda captura `inst` (um ponteiro estável -- `instances_` o guarda via
  //     std::unique_ptr, nunca realocado/movido) em vez de copiar atlas_rgba, então lê o que estiver
  //     atual em inst->atlas_rgba no momento em que o callback de fato roda.
  if (!inst->texture_created || inst->atlas_dirty) {
    if (inst->texture_created) inst->texture = Rml::CallbackTexture{}; // release the stale GL texture.
    inst->texture_created = true;
    inst->atlas_dirty = false;
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
  // EN: (L1.20-FONTFLIP, FT-F4, sub-phase 2B) Per-instance `version` counter -- was a hardcoded
  //     constant `1` before this sub-phase (see font_engine_own.hpp's "SCOPE" section for why: the
  //     atlas used to never change after its one-shot warm-set bake). Now EnsureGlyphs() bumps
  //     inst->version every time it lazily top-up-bakes >=1 REAL new glyph, RmlUi's own standard
  //     "regenerate cached text geometry" signal for THIS handle -- a handle whose string never
  //     needed a codepoint outside the warm set still reports a constant `1` forever, byte-identical
  //     to the pre-2B behaviour.
  // PT: (L1.20-FONTFLIP, FT-F4, sub-fase 2B) Contador `version` por-instância -- era uma constante
  //     fixa `1` antes desta sub-fase (ver a seção "ESCOPO" de font_engine_own.hpp pro motivo: o
  //     atlas antes nunca mudava depois do empacotamento único do warm set). Agora o EnsureGlyphs()
  //     incrementa inst->version toda vez que empacota-por-demanda >=1 glyph novo REAL, o próprio
  //     sinal padrão "regenere a geometria de texto cacheada" do RmlUi pra ESTE handle -- um handle
  //     cuja string nunca precisou de um codepoint fora do warm set continua reportando uma
  //     constante `1` pra sempre, byte-idêntico ao comportamento pré-2B.
  const auto* inst = reinterpret_cast<const FaceInstance*>(handle);
  return inst ? inst->version : 0;
}

} // namespace glintfx

#endif // GLINTFX_OWN_FONT_ENGINE
