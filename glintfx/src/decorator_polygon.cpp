// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of PolygonDecorator / PolygonDecoratorInstancer -- see decorator_polygon.hpp
//     for the design rationale and the upstream reference (DecoratorGradient.{h,cpp}).
//
//     Geometry: a triangle-fan of (sides_+1) vertices -- one hub vertex at the box centre plus
//     `sides_` perimeter vertices evenly spaced on a circle of radius = half the SHORTER
//     dimension of the element's paint-area box (so the polygon is always fully inscribed, even
//     in a non-square element -- see the design-decision note on GenerateElementData below).
//     For a Solid fill all vertices share the same premultiplied fill colour; for a gradient fill
//     (v0.3.1) every vertex instead carries a UNIFORM opaque-white premultiplied-by-opacity colour
//     plus a per-vertex tex_coord (fill-box-local position), and the actual colour ramp is computed
//     per-fragment by RmlUi's own gradient shader -- see PolygonFillMode in decorator_polygon.hpp
//     and the shader-reuse note in GenerateElementData below. Either way, `sides_` triangles connect
//     the hub to each adjacent pair of perimeter vertices (hub, v[i], v[i+1 mod sides_]).
//
//     Vertex 0 (before any rotation) sits at the TOP of the circle (RCSS default: "first vertex
//     pointing up"), matching the spec: angle(i) = rotation - 90deg + i * (360deg / sides), with
//     angle 0deg = local +x (east) and angle 90deg = local +y -- which is DOWN in RmlUi's
//     top-left-origin, y-down vertex space. So -90deg is straight up, exactly the desired
//     default.
//
//     No edge anti-aliasing ring: RMLUI_NUM_MSAA_SAMPLES=0 is already set repo-wide for this
//     render layer (Mesa/llvmpipe MSAA-resolve bug, see CMakeLists.txt), and none of the other
//     hand-built meshes in this codebase (background quads, border-radius corners) add a feather
//     ring either -- they rely on the rasterizer's own edge coverage. A polygon with a hard edge
//     is consistent with that existing baseline; adding a soft-alpha feather ring here would be
//     the ONE decorator in the codebase doing so, which is an inconsistency, not an improvement.
//     If AA becomes a real ask (e.g. a very large, slow-rotating polygon where jaggies read as
//     visible), the natural follow-up is a second ring of `sides_` vertices at radius scaled
//     slightly ABOVE 1.0 with alpha 0, connected by a quad (2 triangles) per edge -- flagged in
//     TODO.md as a possible follow-up, not implemented here.
//
// PT: Implementação de PolygonDecorator / PolygonDecoratorInstancer -- ver decorator_polygon.hpp
//     para a racional de design e a referência upstream (DecoratorGradient.{h,cpp}).
//
//     Geometria: um triangle-fan de (sides_+1) vértices -- um vértice-eixo no centro da caixa
//     mais `sides_` vértices de perímetro igualmente espaçados num círculo de raio = metade da
//     dimensão MENOR da caixa de área-de-pintura do elemento (assim o polígono está sempre
//     totalmente inscrito, mesmo num elemento não-quadrado -- ver nota de decisão de design em
//     GenerateElementData abaixo). Para um preenchimento Solid todos os vértices compartilham a
//     mesma cor premultiplicada; para um preenchimento em gradiente (v0.3.1) cada vértice carrega
//     em vez disso uma cor UNIFORME branco-opaco premultiplicada pela opacidade mais um tex_coord
//     por vértice (posição local à caixa de preenchimento), e o degradê de cor real é calculado
//     por FRAGMENTO pelo próprio shader de gradiente do RmlUi -- ver PolygonFillMode em
//     decorator_polygon.hpp e a nota de reuso de shader em GenerateElementData abaixo. De um jeito
//     ou de outro, `sides_` triângulos conectam o eixo a cada par adjacente de vértices de
//     perímetro (eixo, v[i], v[i+1 mod sides_]).
//
//     O vértice 0 (antes de qualquer rotação) fica no TOPO do círculo (padrão do RCSS: "primeiro
//     vértice apontando pra cima"), casando com a spec: ângulo(i) = rotação - 90deg + i * (360deg
//     / lados), com ângulo 0deg = +x local (leste) e ângulo 90deg = +y local -- que é PRA BAIXO
//     no espaço de vértice do RmlUi (origem topo-esquerda, y pra baixo). Então -90deg é
//     exatamente pra cima, o padrão desejado.
//
//     Sem anel de anti-aliasing de borda: RMLUI_NUM_MSAA_SAMPLES=0 já está fixado em todo o
//     repo para este render layer (bug de resolve MSAA do Mesa/llvmpipe, ver CMakeLists.txt), e
//     nenhuma outra mesh feita à mão neste código (quads de background, cantos de border-radius)
//     adiciona anel de feather tampouco -- todas dependem da cobertura de borda do próprio
//     rasterizador. Um polígono com borda dura é consistente com essa base já existente;
//     adicionar um anel de feather aqui seria o ÚNICO decorator do código fazendo isso, o que é
//     inconsistência, não melhoria. Se AA virar um pedido real (ex.: um polígono grande e lento
//     de girar onde o serrilhado fica visível), o follow-up natural é um segundo anel de
//     `sides_` vértices em raio ligeiramente ACIMA de 1.0 com alpha 0, conectado por um quad (2
//     triângulos) por aresta -- sinalizado no TODO.md como possível follow-up, não implementado
//     aqui.
// Copyright (c) 2026 Petrus Silva Costa
#include "decorator_polygon.hpp"

#include <RmlUi/Core/CompiledFilterShader.h>  // EN: complete Rml::CompiledShader type. PT: tipo Rml::CompiledShader completo.
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/NumericValue.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertyParser.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleSheetSpecification.h>

#include <cmath>    // EN: std::lround/std::isfinite. PT: std::lround/std::isfinite.
#include <utility>  // EN: std::move. PT: std::move.

namespace glintfx {

// EN: Upper bound on the polygon side count, part of the input-surface hardening (v0.2.6 review):
//     RCSS is not always author-trusted -- it can arrive from a theme, a mod, or dynamically
//     generated content -- so a hostile or typo'd "polygon(999999999, ...)" must not be allowed
//     to tessellate ~a billion triangles (a DoS-by-input). 1024 is deliberately generous: past
//     ~64 sides a filled polygon is already visually indistinguishable from a circle in a UI, so
//     1024 never constrains a real use case while still bounding the vertex/index buffers to a
//     few KB. Enforced together with the sides>=3 floor as a single range check in
//     InstanceDecorator() below; out-of-range input is rejected (decorator ignored + log), same
//     fail-high path as every other invalid input.
// PT: Limite superior da contagem de lados do polígono, parte do hardening da superfície de
//     entrada (review v0.2.6): RCSS nem sempre é confiável do autor -- pode vir de um tema, um
//     mod, ou conteúdo gerado dinamicamente -- então um "polygon(999999999, ...)" hostil ou com
//     typo não pode tesselar ~um bilhão de triângulos (um DoS-por-entrada). 1024 é
//     deliberadamente folgado: acima de ~64 lados um polígono preenchido já é visualmente
//     indistinguível de um círculo numa UI, então 1024 nunca limita um caso de uso real e ainda
//     assim limita os buffers de vértice/índice a poucos KB. Aplicado junto com o piso sides>=3
//     como uma única checagem de range em InstanceDecorator() abaixo; entrada fora do range é
//     rejeitada (decorator ignorado + log), mesmo caminho fail-high de toda outra entrada
//     inválida.
static constexpr int POLYGON_MAX_SIDES = 1024;

// EN: The "angle" RCSS parser (PropertyParserNumber with Units=ANGLE, zero_unit=RAD) stores the
//     raw number tagged with either Unit::DEG or Unit::RAD (or RAD for a bare "0"). RmlUi's own
//     degree/radian resolution helper (ComputeAngle) lives in Source/Core/ComputeProperty.h,
//     a RmlUi-PRIVATE header (not part of Include/RmlUi/Core/, not an installed/public API) --
//     reaching into it from this internal glintfx source file would couple us to an
//     implementation-detail header that RmlUi is free to change without notice. The conversion
//     itself is 4 lines using only public API (Rml::Math::DegreesToRadians + the public Unit
//     enum), so it is reimplemented locally instead.
// PT: O parser RCSS "angle" (PropertyParserNumber com Units=ANGLE, zero_unit=RAD) guarda o
//     número bruto marcado com Unit::DEG ou Unit::RAD (ou RAD pra um "0" nu). O helper do
//     próprio RmlUi de resolução grau/radiano (ComputeAngle) mora em
//     Source/Core/ComputeProperty.h, um header PRIVADO do RmlUi (não faz parte de
//     Include/RmlUi/Core/, não é API instalada/pública) -- alcançá-lo a partir deste arquivo
//     interno da glintfx nos acoplaria a um header de detalhe-de-implementação que o RmlUi é
//     livre para mudar sem aviso. A conversão em si são 4 linhas usando só API pública
//     (Rml::Math::DegreesToRadians + o enum Unit público), então é reimplementada localmente.
static float ResolveAngleRadians(const Rml::NumericValue& value) {
  if (value.unit == Rml::Unit::DEG)
    return Rml::Math::DegreesToRadians(value.number);
  return value.number; // Unit::RAD (or the zero_unit fallback for a bare "0").
}

// ---------------------------------------------------------------------------------------------
// EN: FILL PARSING (v0.3.1 -- gradient support). Everything below is anonymous-namespace, used
//     only from PolygonDecoratorInstancer::InstanceDecorator().
//
//     DESIGN: "polygon()"'s 2nd argument is registered with the built-in PASSTHROUGH "string"
//     parser (see the instancer ctor below), not "color". The string parser (Source/Core/
//     PropertyParserString.cpp) always returns true and stores the raw text verbatim -- it does
//     ZERO validation. All real validation happens HERE, in ParseFill(), called from
//     InstanceDecorator() exactly like the existing `sides` hardening: the RCSS-level parser is a
//     transport, not a gate. This mirrors the codebase's existing convention (the "number" parser
//     for `sides` is equally permissive; InstanceDecorator does the actual range check) instead of
//     introducing a second, inconsistent hardening style for the new argument.
//
//     WHY NOT NEST A GRADIENT FUNCTION AS A NATIVE RCSS SUB-PROPERTY (e.g. giving `fill` a
//     "color_stop_list"-typed shorthand item directly, the way DecoratorRadialGradient's own
//     "decorator" shorthand does): RmlUi's RecursiveCommaSeparated shorthand walks its comma-split
//     items POSITIONALLY against a FIXED list of named properties ("sides, fill, rotation?"); there
//     is no way to make one positional item conditionally resolve to a DIFFERENT property
//     definition (colour vs. stop-list) based on its own textual content. A "string" passthrough
//     for the 2nd slot, deferring the colour-vs-gradient decision to plain C++ string inspection,
//     is the smallest surface that stays inside the shorthand system while still accepting a
//     "radial-gradient(...)" / "linear-gradient(...)" token as a single, INTACT comma-item --
//     confirmed by reading PropertySpecification::ParsePropertyValues (see PropertySpecification.cpp
//     around VALUE_PARENTHESIS): a comma split ALREADY treats "(...)" as opaque, so
//     "polygon(6, radial-gradient(#fff, #000 55%))" arrives at our "fill" property as ONE untouched
//     token, its internal comma preserved. No shorthand-grammar workaround was needed for nesting;
//     the only real obstacle was the closed set of Property/Variant types (see below).
//
//     WHY THE RAW STRING IS RE-PARSED HERE INSTEAD OF STORING A STRUCTURED PolygonFill DIRECTLY IN
//     THE Property: Rml::Variant (Include/RmlUi/Core/Variant.h) is a CLOSED, hand-enumerated set of
//     types (BOOL, COLOURB, COLORSTOPLIST, STRING, ...) -- there is no public escape hatch to store
//     an arbitrary custom C++ struct in it without patching RmlUi core, which is off the table (see
//     decorator_polygon.hpp's file header and CLAUDE.md's Camada-1 rule: only NOTICE-listed deps,
//     no core patches). A raw Rml::String round-trip is the pragmatic, zero-patch alternative.
//
//     REUSE, DON'T REINVENT: every actual parsing primitive below is RmlUi's OWN public parser,
//     fetched by name via the public Rml::StyleSheetSpecification::GetParser() API (the very same
//     lookup RmlUi's shorthand system uses internally) -- "color" (hex/rgb/rgba/hsl/named colours,
//     identical to plain "polygon(6, red)"), "color_stop_list" (the exact stop-list grammar
//     "<color> <position>?, <color> <position>?, ..." used by the REAL radial-gradient/
//     linear-gradient decorators), "angle" (identical to the `rotation` argument's own parser),
//     and "number_percent" (bare fraction or percentage, for the "at <x> <y>" centre). Zero
//     hand-rolled colour/number lexing exists in this file.
// PT: PARSING DE PREENCHIMENTO (v0.3.1 -- suporte a gradiente). Tudo abaixo é do namespace anônimo,
//     usado só a partir de PolygonDecoratorInstancer::InstanceDecorator().
//
//     DESIGN: o 2º argumento de "polygon()" é registrado com o parser PASSTHROUGH embutido
//     "string" (ver o ctor do instancer abaixo), não "color". O parser de string
//     (Source/Core/PropertyParserString.cpp) sempre retorna true e guarda o texto bruto ao pé da
//     letra -- faz ZERO validação. Toda validação real acontece AQUI, em ParseFill(), chamada a
//     partir de InstanceDecorator() exatamente como o hardening já existente de `sides`: o parser
//     em nível de RCSS é um transporte, não um portão. Isso espelha a convenção já existente no
//     código (o parser "number" de `sides` é igualmente permissivo; o InstanceDecorator faz a
//     checagem de range de fato) em vez de introduzir um segundo estilo de hardening inconsistente
//     para o novo argumento.
//
//     POR QUE NÃO ANINHAR UMA FUNÇÃO DE GRADIENTE COMO SUB-PROPRIEDADE RCSS NATIVA (ex.: dar a
//     `fill` um item de shorthand tipado "color_stop_list" diretamente, do jeito que o próprio
//     shorthand "decorator" de DecoratorRadialGradient faz): o shorthand RecursiveCommaSeparated do
//     RmlUi percorre seus itens separados por vírgula POSICIONALMENTE contra uma lista FIXA de
//     propriedades nomeadas ("sides, fill, rotation?"); não há como fazer um item posicional
//     resolver condicionalmente para uma definição de propriedade DIFERENTE (cor vs. lista-de-stops)
//     com base no próprio conteúdo textual. Um passthrough "string" para o 2º slot, adiando a
//     decisão cor-vs-gradiente para inspeção de string em C++ puro, é a menor superfície que
//     permanece dentro do sistema de shorthand e ainda assim aceita um token
//     "radial-gradient(...)" / "linear-gradient(...)" como um único item de vírgula INTACTO --
//     confirmado lendo PropertySpecification::ParsePropertyValues (ver PropertySpecification.cpp
//     em torno de VALUE_PARENTHESIS): um split por vírgula JÁ trata "(...)" como opaco, então
//     "polygon(6, radial-gradient(#fff, #000 55%))" chega na nossa propriedade "fill" como UM
//     token intocado, com sua vírgula interna preservada. Nenhum contorno de gramática de
//     shorthand foi necessário para o aninhamento; o único obstáculo real foi o conjunto fechado
//     de tipos Property/Variant (ver abaixo).
//
//     POR QUE A STRING BRUTA É RE-PARSEADA AQUI EM VEZ DE GUARDAR UM PolygonFill ESTRUTURADO
//     DIRETO NA Property: Rml::Variant (Include/RmlUi/Core/Variant.h) é um conjunto FECHADO,
//     enumerado à mão, de tipos (BOOL, COLOURB, COLORSTOPLIST, STRING, ...) -- não há escape
//     hatch público para guardar uma struct C++ arbitrária nele sem remendar o core do RmlUi, o
//     que está fora de cogitação (ver o cabeçalho de decorator_polygon.hpp e a regra da Camada 1
//     no CLAUDE.md: só deps listadas no NOTICE, sem patch de core). Um round-trip por
//     Rml::String bruta é a alternativa pragmática e sem patch.
//
//     REUSAR, NÃO REINVENTAR: toda primitiva de parsing de fato abaixo é um parser PRÓPRIO do
//     RmlUi, buscado por nome via a API pública Rml::StyleSheetSpecification::GetParser() (a
//     MESMA busca que o sistema de shorthand do RmlUi usa internamente) -- "color"
//     (hex/rgb/rgba/hsl/cores nomeadas, idêntico a um simples "polygon(6, red)"),
//     "color_stop_list" (a gramática exata de lista-de-stops "<cor> <posição>?, <cor> <posição>?,
//     ..." usada pelos decorators REAIS radial-gradient/linear-gradient), "angle" (idêntico ao
//     parser do próprio argumento `rotation`), e "number_percent" (fração nua ou porcentagem,
//     para o centro "at <x> <y>"). Zero lexing de cor/número feito à mão existe neste arquivo.
// ---------------------------------------------------------------------------------------------
namespace {

// EN: Looks up a global RmlUi property parser by name and runs it once. `token` must already be
//     the exact substring to hand the parser (any outer trimming/splitting done by the caller).
// PT: Busca um parser de propriedade global do RmlUi por nome e o executa uma vez. `token` já
//     deve ser a substring exata a entregar ao parser (qualquer trim/split externo é feito pelo
//     chamador).
bool RunNamedParser(const char* parser_name, const Rml::String& token, Rml::Property& out_property) {
  Rml::PropertyParser* parser = Rml::StyleSheetSpecification::GetParser(parser_name);
  if (!parser)
    return false;
  return parser->ParseValue(out_property, token, Rml::ParameterMap());
}

// EN: "<number>" (a bare fraction, e.g. "0.3") or "<percentage>" (e.g. "30%"), both normalised to
//     a plain fraction. Used only for the radial "at <x> <y>" centre -- deliberately NOT the same
//     "length_percent" parser the real CSS radial-gradient position uses, since a length in px has
//     no well-defined meaning for a shape that is not the CSS box model (see PolygonFill's doc
//     comment on radial_center_fraction) -- restricting to number|percent keeps that ambiguity out
//     of scope entirely rather than silently mis-resolving it.
// PT: "<número>" (uma fração nua, ex.: "0.3") ou "<porcentagem>" (ex.: "30%"), ambos normalizados
//     para uma fração simples. Usado só para o centro radial "at <x> <y>" -- deliberadamente NÃO o
//     mesmo parser "length_percent" que a posição real do radial-gradient do CSS usa, já que um
//     comprimento em px não tem significado bem definido para uma forma que não é o box model do
//     CSS (ver o doc-comment de radial_center_fraction em PolygonFill) -- restringir a
//     número|porcentagem mantém essa ambiguidade inteiramente fora de escopo em vez de
//     resolvê-la mal silenciosamente.
bool ParsePercentOrNumber01(const Rml::String& token, float& out_fraction) {
  Rml::Property p;
  if (!RunNamedParser("number_percent", token, p))
    return false;
  const float n = p.Get<float>();
  if (!std::isfinite(n))
    return false;
  out_fraction = (p.unit == Rml::Unit::PERCENT) ? n * 0.01f : n;
  return std::isfinite(out_fraction);
}

// EN: Delegates to the exact same "angle" parser (and the same ResolveAngleRadians helper) already
//     used for the `rotation` argument -- see the comment on ResolveAngleRadians above for why it
//     is reimplemented locally instead of reaching into RmlUi's private ComputeAngle.
// PT: Delega para o mesmíssimo parser "angle" (e o mesmo helper ResolveAngleRadians) já usado para
//     o argumento `rotation` -- ver o comentário de ResolveAngleRadians acima sobre por que é
//     reimplementado localmente em vez de alcançar o ComputeAngle privado do RmlUi.
bool ParseAngleRadians(const Rml::String& token, float& out_radians) {
  Rml::Property p;
  if (!RunNamedParser("angle", token, p))
    return false;
  out_radians = ResolveAngleRadians(p.GetNumericValue());
  return std::isfinite(out_radians);
}

// EN: Parses a comma-joined stop list ("<color> <position>?, <color> <position>?, ...") through
//     RmlUi's own "color_stop_list" parser (Source/Core/PropertyParserColorStopList.cpp) -- the
//     SAME parser DecoratorRadialGradient/DecoratorLinearGradient/DecoratorConicGradient register
//     for their own "color-stops" shorthand item. It already splits on top-level commas itself
//     (StringUtilities::ExpandString with '(' / ')' as quote characters), so `csv` is handed over
//     whole, unsplit by us.
// PT: Parseia uma lista de stops unida por vírgula ("<cor> <posição>?, <cor> <posição>?, ...")
//     através do próprio parser "color_stop_list" do RmlUi
//     (Source/Core/PropertyParserColorStopList.cpp) -- o MESMO parser que
//     DecoratorRadialGradient/DecoratorLinearGradient/DecoratorConicGradient registram para seu
//     próprio item de shorthand "color-stops". Ele já separa por vírgulas de topo por conta
//     própria (StringUtilities::ExpandString com '(' / ')' como caracteres de aspas), então `csv`
//     é entregue inteiro, sem separação nossa.
bool ParseColorStopsCsv(const Rml::String& csv, Rml::ColorStopList& out_stops) {
  if (csv.empty())
    return false;
  Rml::Property p;
  if (!RunNamedParser("color_stop_list", csv, p))
    return false;
  if (p.unit != Rml::Unit::COLORSTOPLIST)
    return false;
  out_stops = p.value.GetReference<Rml::ColorStopList>();
  return !out_stops.empty();
}

// EN: Resolves every stop's position to Unit::NUMBER in [0,1], in place. "color_stop_list" leaves
//     an omitted position at its default-constructed NumericValue (Unit::UNKNOWN, i.e. "auto");
//     a given position may be Unit::PERCENT (accepted, divided by 100) or -- since the shared
//     parser also accepts plain lengths (px, em, ...) when no "angle" constraint is passed to it
//     -- some other length unit, which THIS decorator does NOT support (a polygon has no CSS box
//     model to resolve a length against) and therefore REJECTS (fail-high, same philosophy as the
//     `sides` hardening: reject rather than silently misinterpret). Auto positions are resolved
//     with a simplified version of the CSS auto-stop algorithm (first/last default to 0/1, any
//     interior run of autos is evenly spaced between its resolved neighbours) -- deliberately
//     simpler than RmlUi's own ResolveColorStops (DecoratorGradient.cpp, private/static, not
//     reachable from here): it skips the 1px-minimum anti-aliasing spacing nudge, which matters
//     for a screen-space gradient line measured in pixels but not for our normalised [0,1] stop
//     space. A final monotonic-non-decreasing pass guards against an out-of-order stop list
//     (e.g. "50%, 20%") inverting part of the gradient.
// PT: Resolve a posição de cada stop para Unit::NUMBER em [0,1], in-place. "color_stop_list" deixa
//     uma posição omitida no seu NumericValue default-construído (Unit::UNKNOWN, ou seja, "auto");
//     uma posição dada pode ser Unit::PERCENT (aceita, dividida por 100) ou -- já que o parser
//     compartilhado também aceita comprimentos simples (px, em, ...) quando nenhuma restrição
//     "angle" é passada a ele -- alguma outra unidade de comprimento, que ESTE decorator NÃO
//     suporta (um polígono não tem box model do CSS contra o qual resolver um comprimento) e
//     portanto REJEITA (fail-high, mesma filosofia do hardening de `sides`: rejeitar em vez de
//     interpretar mal silenciosamente). Posições auto são resolvidas com uma versão simplificada
//     do algoritmo de auto-stop do CSS (primeiro/último default para 0/1, qualquer sequência
//     interior de autos é igualmente espaçada entre seus vizinhos resolvidos) -- deliberadamente
//     mais simples que o próprio ResolveColorStops do RmlUi (DecoratorGradient.cpp,
//     privado/estático, inalcançável daqui): pula o ajuste de espaçamento mínimo de 1px
//     anti-serrilhado, que importa para uma linha de gradiente em espaço de tela medida em pixels
//     mas não para o nosso espaço de stop normalizado [0,1]. Uma passada final de não-decrescente
//     monotônico guarda contra uma lista de stops fora de ordem (ex.: "50%, 20%") invertendo parte
//     do gradiente.
bool ResolveStopPositions(Rml::ColorStopList& stops) {
  const int n = static_cast<int>(stops.size());
  if (n == 0)
    return false;

  for (Rml::ColorStop& stop : stops) {
    if (stop.position.unit == Rml::Unit::PERCENT) {
      if (!std::isfinite(stop.position.number))
        return false;
      stop.position = Rml::NumericValue(stop.position.number * 0.01f, Rml::Unit::NUMBER);
    } else if (stop.position.unit != Rml::Unit::UNKNOWN) {
      return false; // Unsupported unit (length, angle, ...) for a polygon fill stop.
    }
  }

  if (stops[0].position.unit != Rml::Unit::NUMBER)
    stops[0].position = Rml::NumericValue(0.f, Rml::Unit::NUMBER);
  if (stops[n - 1].position.unit != Rml::Unit::NUMBER)
    stops[n - 1].position = Rml::NumericValue(1.f, Rml::Unit::NUMBER);

  int auto_begin = -1;
  for (int i = 1; i < n; ++i) {
    if (stops[i].position.unit != Rml::Unit::NUMBER) {
      if (auto_begin < 0)
        auto_begin = i;
      continue;
    }
    if (auto_begin >= 0) {
      const float t0 = stops[auto_begin - 1].position.number;
      const float t1 = stops[i].position.number;
      const int count = i - auto_begin;
      for (int j = 0; j < count; ++j) {
        const float fraction = float(j + 1) / float(count + 1);
        stops[auto_begin + j].position = Rml::NumericValue(t0 + (t1 - t0) * fraction, Rml::Unit::NUMBER);
      }
      auto_begin = -1;
    }
  }

  float prev = stops[0].position.number;
  for (int i = 1; i < n; ++i) {
    if (!std::isfinite(stops[i].position.number))
      return false;
    if (stops[i].position.number < prev)
      stops[i].position.number = prev;
    prev = stops[i].position.number;
  }
  return true;
}

// EN: Recognises an OPTIONAL leading descriptor token of a radial-gradient's inner argument list:
//     "circle at <x> <y>", "at <x> <y>", or a bare "circle" (default centre, explicit shape). Only
//     "circle" is supported (no "ellipse") since the polygon's own fill shape is always the
//     inscribed circle -- see decorator_polygon.hpp's radial_center_fraction doc comment. Absence
//     of any such descriptor is NOT an error: `tokens[0]` is then simply the first colour stop,
//     and the centre stays at its default (0.5, 0.5). A `"circle"`-prefixed word match is
//     word-boundary-checked (index 6 must be end-of-string or a space) so a stop colour that
//     happens to start with the literal substring "circle" cannot be misread as the shape keyword
//     (no such CSS named colour exists today, but the check costs nothing and removes the
//     assumption).
// PT: Reconhece um token descritor OPCIONAL líder da lista de argumentos interna de um
//     radial-gradient: "circle at <x> <y>", "at <x> <y>", ou um "circle" nu (centro default, forma
//     explícita). Só "circle" é suportado (sem "ellipse") já que a forma de preenchimento do
//     próprio polígono é sempre o círculo inscrito -- ver o doc-comment de radial_center_fraction
//     em decorator_polygon.hpp. Ausência de tal descritor NÃO é erro: `tokens[0]` é então
//     simplesmente o primeiro stop de cor, e o centro fica no seu default (0.5, 0.5). Um match do
//     prefixo `"circle"` é checado por fronteira de palavra (índice 6 deve ser fim-de-string ou um
//     espaço) para que uma cor de stop que por acaso comece com a substring literal "circle" não
//     seja mal-interpretada como a palavra-chave de forma (nenhuma cor nomeada do CSS assim existe
//     hoje, mas a checagem não custa nada e remove a suposição).
bool ParseRadialPositionPrefix(const Rml::StringList& tokens, Rml::Vector2f& out_fraction, size_t& first_stop_index) {
  out_fraction = Rml::Vector2f(0.5f, 0.5f);
  first_stop_index = 0;
  if (tokens.empty())
    return true;

  const Rml::String lower = Rml::StringUtilities::ToLower(tokens[0]);
  const bool starts_with_at = Rml::StringUtilities::StartsWith(lower, "at ");
  const bool starts_with_circle = lower.size() >= 6 && Rml::StringUtilities::StartsWith(lower, "circle") &&
                                   (lower.size() == 6 || lower[6] == ' ');
  const size_t at_pos = lower.find(" at ");

  if (!starts_with_at && !starts_with_circle && at_pos == Rml::String::npos)
    return true; // No descriptor -- tokens[0] is the first colour stop.

  size_t coord_pos = Rml::String::npos;
  if (starts_with_at)
    coord_pos = 3; // strlen("at ")
  else if (at_pos != Rml::String::npos)
    coord_pos = at_pos + 4; // strlen(" at ")
  else {
    // Bare "circle" (no "at ..." clause): valid, default centre, nothing else may trail it.
    if (!Rml::StringUtilities::StripWhitespace(tokens[0].substr(6)).empty())
      return false;
    first_stop_index = 1;
    return true;
  }

  Rml::StringList xy;
  Rml::StringUtilities::ExpandString(xy, Rml::StringUtilities::StripWhitespace(tokens[0].substr(coord_pos)), ' ', true);
  if (xy.size() != 2)
    return false;
  if (!ParsePercentOrNumber01(xy[0], out_fraction.x))
    return false;
  if (!ParsePercentOrNumber01(xy[1], out_fraction.y))
    return false;
  first_stop_index = 1;
  return true;
}

// EN: Top-level entry point: parses the raw "fill" token (the decorator's 2nd positional argument)
//     into a fully-resolved PolygonFill. Three syntaxes are accepted:
//       <color>                                        -- e.g. "#C9A24B", "red", "rgba(0,0,0,.5)".
//       radial-gradient([circle [at <x> <y>],] <stops>) -- "circle"/"at" prefix optional.
//       linear-gradient(<angle>, <stops>)               -- angle is MANDATORY here (this
//                                                           implementation does not support CSS's
//                                                           "to <side>" keyword form).
//     where <stops> is "<color> <position>?" repeated, comma-separated (see ParseColorStopsCsv).
//     Any parse failure returns false -- InstanceDecorator() below turns that into a warning log
//     and a nullptr decorator (fail-high, decorator ignored, box background/underlying content
//     shows through), matching the `sides` hardening's philosophy exactly.
// PT: Ponto de entrada principal: parseia o token "fill" bruto (o 2º argumento posicional do
//     decorator) para um PolygonFill totalmente resolvido. Três sintaxes são aceitas:
//       <cor>                                          -- ex.: "#C9A24B", "red", "rgba(0,0,0,.5)".
//       radial-gradient([circle [at <x> <y>],] <stops>) -- prefixo "circle"/"at" opcional.
//       linear-gradient(<ângulo>, <stops>)              -- o ângulo é OBRIGATÓRIO aqui (esta
//                                                           implementação não suporta a forma de
//                                                           palavra-chave "to <side>" do CSS).
//     onde <stops> é "<cor> <posição>?" repetido, separado por vírgula (ver ParseColorStopsCsv).
//     Qualquer falha de parse retorna false -- o InstanceDecorator() abaixo transforma isso num
//     log de aviso e um decorator nulo (fail-high, decorator ignorado, background da
//     caixa/conteúdo por baixo aparece), casando exatamente com a filosofia do hardening de
//     `sides`.
bool ParseFill(const Rml::String& raw, PolygonFill& out) {
  const Rml::String value = Rml::StringUtilities::StripWhitespace(raw);
  if (value.empty())
    return false;

  const bool is_radial = Rml::StringUtilities::StartsWith(value, "radial-gradient(");
  const bool is_linear = Rml::StringUtilities::StartsWith(value, "linear-gradient(");

  if (!is_radial && !is_linear) {
    Rml::Property p;
    if (!RunNamedParser("color", value, p))
      return false;
    out.mode = PolygonFillMode::Solid;
    out.solid_color = p.Get<Rml::Colourb>();
    return true;
  }

  if (!Rml::StringUtilities::EndsWith(value, ")"))
    return false;
  const size_t open = value.find('(');
  if (open == Rml::String::npos || value.size() < open + 2)
    return false;
  const Rml::String inner = Rml::StringUtilities::StripWhitespace(value.substr(open + 1, value.size() - open - 2));
  if (inner.empty())
    return false;

  Rml::StringList tokens;
  Rml::StringUtilities::ExpandString(tokens, inner, ',', '(', ')');
  if (tokens.empty())
    return false;

  if (is_radial) {
    Rml::Vector2f center_fraction;
    size_t first_stop = 0;
    if (!ParseRadialPositionPrefix(tokens, center_fraction, first_stop))
      return false;
    if (first_stop >= tokens.size())
      return false;

    Rml::String stops_csv;
    Rml::StringList stop_tokens(tokens.begin() + static_cast<Rml::StringList::difference_type>(first_stop), tokens.end());
    Rml::StringUtilities::JoinString(stops_csv, stop_tokens, ',');

    Rml::ColorStopList stops;
    if (!ParseColorStopsCsv(stops_csv, stops) || !ResolveStopPositions(stops))
      return false;

    out.mode = PolygonFillMode::RadialGradient;
    out.stops = std::move(stops);
    out.radial_center_fraction = center_fraction;
    return true;
  }

  // Linear: tokens[0] MUST be an angle.
  float angle_rad = 0.f;
  if (!ParseAngleRadians(tokens[0], angle_rad) || tokens.size() < 2)
    return false;

  Rml::String stops_csv;
  Rml::StringList stop_tokens(tokens.begin() + 1, tokens.end());
  Rml::StringUtilities::JoinString(stops_csv, stop_tokens, ',');

  Rml::ColorStopList stops;
  if (!ParseColorStopsCsv(stops_csv, stops) || !ResolveStopPositions(stops))
    return false;

  out.mode = PolygonFillMode::LinearGradient;
  out.stops = std::move(stops);
  out.linear_angle_radians = angle_rad;
  return true;
}

} // namespace

bool PolygonDecorator::Initialise(int sides, PolygonFill fill, float rotation_radians) {
  if (sides < 3)
    return false;
  sides_ = sides;
  fill_ = std::move(fill);
  rotation_radians_ = rotation_radians;
  return true;
}

namespace {
// EN: Per-element decorator data. `shader` is a default-constructed (falsy) Rml::CompiledShader
//     for a Solid fill -- see the shader-reuse note in GenerateElementData below for why a single
//     struct/render path covers both Solid and gradient fills. Both members are RAII
//     (Rml::UniqueRenderResource): letting `delete` run their destructors in ReleaseElementData is
//     sufficient to release the underlying GPU geometry/shader resources, no explicit
//     RenderManager::ReleaseResource() call needed -- same pattern this file already relied on for
//     the bare `Rml::Geometry*` before this feature.
// PT: Dado de decorator por-elemento. `shader` é um Rml::CompiledShader default-construído (falso)
//     para um preenchimento Solid -- ver a nota de reuso de shader em GenerateElementData abaixo
//     sobre por que uma única struct/caminho de render cobre tanto Solid quanto gradiente. Ambos
//     os membros são RAII (Rml::UniqueRenderResource): deixar o `delete` rodar seus destrutores em
//     ReleaseElementData é suficiente para liberar os recursos de GPU de geometria/shader
//     subjacentes, sem precisar de chamada explícita a RenderManager::ReleaseResource() -- mesmo
//     padrão que este arquivo já usava para o `Rml::Geometry*` nu antes desta feature.
struct PolygonElementData {
  Rml::Geometry geometry;
  Rml::CompiledShader shader;
};
} // namespace

Rml::DecoratorDataHandle PolygonDecorator::GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const {
  Rml::RenderManager* render_manager = element->GetRenderManager();
  if (!render_manager)
    return INVALID_DECORATORDATAHANDLE;

  const Rml::RenderBox render_box = element->GetRenderBox(paint_area);
  const Rml::Vector2f offset = render_box.GetFillOffset();
  const Rml::Vector2f size = render_box.GetFillSize();
  if (size.x <= 0.f || size.y <= 0.f)
    return INVALID_DECORATORDATAHANDLE;

  const Rml::ComputedValues& computed = element->GetComputedValues();
  const float opacity = computed.opacity();

  // EN: DESIGN DECISION (non-obvious, flagged for the lead): the polygon is inscribed using
  //     radius = half of the SHORTER box dimension, centred in the box. For a square element
  //     (the GusWorld hex-slider-node case) this is identical to "inscribed in the border/
  //     padding box". For a NON-square element (e.g. a wide rectangle) this means the polygon
  //     does NOT stretch to fill the longer axis -- it stays a REGULAR (equilateral) polygon,
  //     centred, with empty box margin on the longer axis. The alternative (ellipse-style
  //     independent x/y radii, stretching the polygon into an irregular N-gon to fill a
  //     non-square box) was rejected: it would silently distort a "regular polygon" primitive
  //     into a shape that is regular only for square elements, with no RCSS knob to opt out.
  //     Callers who want a non-square footprint should size a square element and lay it out
  //     with margin/position, same as they would for a circular border-radius today.
  // PT: DECISÃO DE DESIGN (não-óbvia, sinalizada pro líder): o polígono é inscrito usando
  //     raio = metade da dimensão MENOR da caixa, centrado na caixa. Para um elemento
  //     quadrado (o caso do nó hex-slider do GusWorld) isso é idêntico a "inscrito na caixa
  //     border/padding". Para um elemento NÃO-quadrado (ex.: um retângulo largo) isso
  //     significa que o polígono NÃO estica para preencher o eixo mais longo -- permanece um
  //     polígono REGULAR (equilátero), centrado, com margem vazia de caixa no eixo mais longo.
  //     A alternativa (raios x/y independentes estilo elipse, esticando o polígono num N-ágono
  //     irregular para preencher uma caixa não-quadrada) foi rejeitada: distorceria
  //     silenciosamente uma primitiva de "polígono regular" numa forma que só é regular para
  //     elementos quadrados, sem chave RCSS pra desligar isso. Chamadores que querem uma
  //     pegada não-quadrada devem dimensionar um elemento quadrado e posicioná-lo com
  //     margin/position, do mesmo jeito que fariam hoje para um border-radius circular.
  // EN: `local_center`/`local_dir` are in FILL-BOX-LOCAL space (0,0 = fill-box top-left, same
  //     origin the tex_coord below uses) -- this is the SAME coordinate space
  //     DecoratorRadialGradient/DecoratorLinearGradient compute their shader "center"/"p0"/"p1"
  //     uniforms in (see CalculateRadialGradientShape/CalculateShape, both fed `render_box.
  //     GetFillSize()` with an implicit local origin). `center` is the same point translated to
  //     ABSOLUTE box space, used for the actual mesh vertex positions (as before this feature).
  // PT: `local_center`/`local_dir` estão no espaço LOCAL-À-CAIXA-DE-PREENCHIMENTO (0,0 = canto
  //     superior-esquerdo da caixa de preenchimento, a mesma origem que o tex_coord abaixo usa)
  //     -- este é o MESMO espaço de coordenadas em que DecoratorRadialGradient/
  //     DecoratorLinearGradient calculam seus uniforms de shader "center"/"p0"/"p1" (ver
  //     CalculateRadialGradientShape/CalculateShape, ambos alimentados com `render_box.
  //     GetFillSize()` com origem local implícita). `center` é o mesmo ponto transladado para o
  //     espaço ABSOLUTO da caixa, usado para as posições de vértice de fato da mesh (como antes
  //     desta feature).
  const Rml::Vector2f local_center = size * 0.5f;
  const Rml::Vector2f center = offset + local_center;
  const float radius = 0.5f * Rml::Math::Min(size.x, size.y);
  const float rotation_rad = rotation_radians_;

  const bool is_gradient = (fill_.mode != PolygonFillMode::Solid);
  const Rml::ColourbPremultiplied solid_fill_colour = fill_.solid_color.ToPremultiplied(opacity);
  // EN: Uniform opaque-white-times-opacity vertex colour, fed to fragColor in the shared "gradient"
  //     fragment shader (`finalColor = fragColor * mix_stop_colors(t);`) -- the exact same pattern
  //     DecoratorLinearGradient/DecoratorRadialGradient use to apply element opacity to a gradient
  //     whose STOP colours are otherwise opacity-independent.
  // PT: Cor de vértice uniforme branco-opaco-vezes-opacidade, alimentada ao fragColor no shader de
  //     fragmento "gradient" compartilhado (`finalColor = fragColor * mix_stop_colors(t);`) -- o
  //     mesmíssimo padrão que DecoratorLinearGradient/DecoratorRadialGradient usam para aplicar a
  //     opacidade do elemento a um gradiente cujas cores de STOP são, de outro modo,
  //     independentes de opacidade.
  const Rml::byte alpha_byte = Rml::byte(opacity * 255.f);
  const Rml::ColourbPremultiplied gradient_vertex_colour(alpha_byte, alpha_byte);

  Rml::Mesh mesh;
  Rml::Vector<Rml::Vertex>& vertices = mesh.vertices;
  Rml::Vector<int>& indices = mesh.indices;
  vertices.reserve(static_cast<size_t>(sides_) + 1);
  indices.reserve(static_cast<size_t>(sides_) * 3);

  // EN: Vertex 0 is the fan hub (box centre); vertices 1..sides_ are the perimeter, in order.
  // PT: O vértice 0 é o eixo do fan (centro da caixa); vértices 1..sides_ são o perímetro, em ordem.
  vertices.push_back(Rml::Vertex{center, is_gradient ? gradient_vertex_colour : solid_fill_colour,
      is_gradient ? local_center : Rml::Vector2f(0, 0)});

  const float step = 2.f * Rml::Math::RMLUI_PI / float(sides_);
  for (int i = 0; i < sides_; ++i) {
    // EN: angle 0 = local +x (east); -90deg (= -PI/2) rotates the FIRST perimeter vertex (i=0,
    //     before any author rotation) to local +y-negative, i.e. straight UP in this y-down
    //     vertex space -- "first vertex pointing up" per spec.
    // PT: ângulo 0 = +x local (leste); -90deg (= -PI/2) rotaciona o PRIMEIRO vértice de
    //     perímetro (i=0, antes de qualquer rotação autorada) para +y-negativo local, ou seja,
    //     RETO PRA CIMA neste espaço de vértice y-para-baixo -- "primeiro vértice apontando pra
    //     cima" conforme a spec.
    const float theta = rotation_rad - (Rml::Math::RMLUI_PI * 0.5f) + float(i) * step;
    const Rml::Vector2f dir(Rml::Math::Cos(theta), Rml::Math::Sin(theta));
    const Rml::Vector2f p = center + radius * dir;
    vertices.push_back(Rml::Vertex{p, is_gradient ? gradient_vertex_colour : solid_fill_colour,
        is_gradient ? (local_center + radius * dir) : Rml::Vector2f(0, 0)});
  }

  for (int i = 0; i < sides_; ++i) {
    const int a = 1 + i;
    const int b = 1 + (i + 1) % sides_;
    indices.push_back(0);
    indices.push_back(a);
    indices.push_back(b);
  }

  // EN: SHADER REUSE (v0.3.1): for a gradient fill, compile RmlUi's own "radial-gradient"/
  //     "linear-gradient" fragment shader (Rml::RenderManager::CompileShader, the exact public API
  //     -- and exact Dictionary key names ("center"/"radius" or "p0"/"p1", "repeating",
  //     "color_stop_list") -- DecoratorRadialGradient/DecoratorLinearGradient use; see
  //     RmlUi_Renderer_GL3.cpp's CompileShader for the authoritative key contract). The polygon's
  //     own inscribed-circle radius is deliberately used as the gradient's SIZE (t=1 at the
  //     polygon's own circumradius) instead of CSS's default farthest-corner sizing: the polygon
  //     is already bounded by that circle, so this is the only sizing that puts the LAST stop
  //     exactly on the shape's own edge -- e.g. the "screw head" look the GusWorld brass hexagons
  //     need (bright centre -> dark edge, with the dark edge landing exactly on the hex's border).
  //     For a Solid fill `shader` is left default-constructed (falsy); Geometry::Render's shader
  //     parameter then takes RenderManager::Render's `else` branch (plain per-vertex-colour
  //     RenderGeometry, unchanged from pre-v0.3.1 behaviour) -- see RenderElement below.
  // PT: REUSO DE SHADER (v0.3.1): para um preenchimento em gradiente, compila o próprio shader de
  //     fragmento "radial-gradient"/"linear-gradient" do RmlUi (Rml::RenderManager::CompileShader,
  //     a mesma API pública -- e os mesmos nomes de chave do Dictionary ("center"/"radius" ou
  //     "p0"/"p1", "repeating", "color_stop_list") -- que DecoratorRadialGradient/
  //     DecoratorLinearGradient usam; ver o CompileShader de RmlUi_Renderer_GL3.cpp para o
  //     contrato de chaves autoritativo). O próprio raio do círculo inscrito do polígono é
  //     deliberadamente usado como o TAMANHO do gradiente (t=1 no circunraio do próprio polígono)
  //     em vez do dimensionamento default farthest-corner do CSS: o polígono já é limitado por
  //     esse círculo, então esse é o único dimensionamento que põe o ÚLTIMO stop exatamente na
  //     borda da própria forma -- ex.: o visual de "cabeça de parafuso" que os hexágonos de latão
  //     do GusWorld precisam (centro claro -> borda escura, com a borda escura caindo exatamente
  //     na borda do hexágono). Para um preenchimento Solid, `shader` fica default-construído
  //     (falso); o parâmetro shader de Geometry::Render então toma o ramo `else` de
  //     RenderManager::Render (RenderGeometry por-vértice-cor simples, inalterado desde antes da
  //     v0.3.1) -- ver RenderElement abaixo.
  Rml::CompiledShader shader;
  if (fill_.mode == PolygonFillMode::RadialGradient) {
    const Rml::Vector2f gradient_center(fill_.radial_center_fraction.x * size.x, fill_.radial_center_fraction.y * size.y);
    shader = render_manager->CompileShader("radial-gradient", Rml::Dictionary{
                                                                   {"center", Rml::Variant(gradient_center)},
                                                                   {"radius", Rml::Variant(Rml::Vector2f(radius, radius))},
                                                                   {"repeating", Rml::Variant(false)},
                                                                   {"color_stop_list", Rml::Variant(fill_.stops)},
                                                               });
    if (!shader)
      return INVALID_DECORATORDATAHANDLE;
  } else if (fill_.mode == PolygonFillMode::LinearGradient) {
    // Same convention as DecoratorLinearGradient::CalculateShape's line_vector: angle 0 = up.
    const Rml::Vector2f dir(Rml::Math::Sin(fill_.linear_angle_radians), -Rml::Math::Cos(fill_.linear_angle_radians));
    const Rml::Vector2f p0 = local_center - radius * dir;
    const Rml::Vector2f p1 = local_center + radius * dir;
    shader = render_manager->CompileShader("linear-gradient", Rml::Dictionary{
                                                                   {"p0", Rml::Variant(p0)},
                                                                   {"p1", Rml::Variant(p1)},
                                                                   {"repeating", Rml::Variant(false)},
                                                                   {"color_stop_list", Rml::Variant(fill_.stops)},
                                                               });
    if (!shader)
      return INVALID_DECORATORDATAHANDLE;
  }

  auto* element_data = new PolygonElementData{render_manager->MakeGeometry(std::move(mesh)), std::move(shader)};
  return reinterpret_cast<Rml::DecoratorDataHandle>(element_data);
}

void PolygonDecorator::ReleaseElementData(Rml::DecoratorDataHandle element_data) const {
  delete reinterpret_cast<PolygonElementData*>(element_data);
}

void PolygonDecorator::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const {
  auto* data = reinterpret_cast<PolygonElementData*>(element_data);
  // EN: Same translation convention as every other hand-built decorator in RmlUi (see
  //     DecoratorStraightGradient::RenderElement): mesh vertices are generated relative to the
  //     border-box origin, translated here by the element's absolute border-box offset. Passing
  //     `data->shader` unconditionally is safe: a default-constructed (Solid-fill) CompiledShader
  //     is falsy, and RenderManager::Render dispatches on that truthiness itself (RenderShader vs.
  //     plain RenderGeometry) -- see RenderManager.cpp's `if (shader) ... else ...`.
  // PT: Mesma convenção de translação de todo outro decorator feito à mão no RmlUi (ver
  //     DecoratorStraightGradient::RenderElement): os vértices da mesh são gerados relativos à
  //     origem da border-box, translados aqui pelo offset absoluto de border-box do elemento.
  //     Passar `data->shader` incondicionalmente é seguro: um CompiledShader default-construído
  //     (preenchimento Solid) é falso, e RenderManager::Render despacha nessa veracidade por
  //     conta própria (RenderShader vs. RenderGeometry simples) -- ver o `if (shader) ... else
  //     ...` de RenderManager.cpp.
  data->geometry.Render(element->GetAbsoluteOffset(Rml::BoxArea::Border), {}, data->shader);
}

PolygonDecoratorInstancer::PolygonDecoratorInstancer() {
  // EN: `sides` default is "0" (an INVALID value, below the sides>=3 minimum), NOT "6". This is
  //     deliberate: an author who writes "decorator: polygon();" (empty parentheses) supplies no
  //     positional value for `sides`, so RmlUi fills it with THIS default. A default of "6"
  //     would silently turn "polygon()" into an opaque white hexagon -- an asymmetric surprise,
  //     since "polygon(6)" (sides given, colour omitted) already fails to parse. With a default
  //     of "0", "polygon()" resolves sides=0, which falls into the existing `sides < 3`
  //     rejection in InstanceDecorator() below (decorator ignored + a warning log) -- fail-high
  //     on empty/invalid input rather than fail-silent. An EXPLICIT sides value in the RCSS
  //     (e.g. "polygon(6, #fff)") overrides this default, so correct usage is unaffected.
  // PT: O default de `sides` é "0" (um valor INVÁLIDO, abaixo do mínimo sides>=3), NÃO "6". Isso
  //     é deliberado: um autor que escreve "decorator: polygon();" (parênteses vazios) não
  //     fornece valor posicional pra `sides`, então o RmlUi o preenche com ESTE default. Um
  //     default de "6" transformaria silenciosamente "polygon()" num hexágono branco opaco --
  //     uma surpresa assimétrica, já que "polygon(6)" (sides dado, cor omitida) já falha o
  //     parse. Com default "0", "polygon()" resolve sides=0, que cai na rejeição `sides < 3` já
  //     existente em InstanceDecorator() abaixo (decorator ignorado + log de aviso) -- fail-high
  //     em entrada vazia/inválida em vez de fail-silent. Um valor de sides EXPLÍCITO no RCSS
  //     (ex.: "polygon(6, #fff)") sobrepõe este default, então o uso correto não é afetado.
  ids_.sides = RegisterProperty("sides", "0").AddParser("number").GetId();
  // EN: `fill` (v0.3.1, formerly a plain "color"-parsed property) is now registered with the
  //     built-in PASSTHROUGH "string" parser -- see the "FILL PARSING" block above
  //     PolygonDecorator::Initialise for the full rationale (closed Property/Variant type set,
  //     shorthand grammar limits) and ParseFill() for the actual colour-vs-gradient parsing, which
  //     runs later in InstanceDecorator(). The registered property NAME ("fill") is not
  //     author-visible -- shorthand items are matched purely positionally -- so this rename from
  //     "color" has zero RCSS-visible effect.
  // PT: `fill` (v0.3.1, antes uma propriedade parseada como "color" simples) agora é registrada
  //     com o parser PASSTHROUGH embutido "string" -- ver o bloco "FILL PARSING" acima de
  //     PolygonDecorator::Initialise para a racional completa (conjunto fechado de tipos
  //     Property/Variant, limites de gramática do shorthand) e ParseFill() para o parsing
  //     cor-vs-gradiente de fato, que roda depois em InstanceDecorator(). O NOME de propriedade
  //     registrado ("fill") não é visível ao autor -- itens de shorthand são casados puramente
  //     por posição -- então este rename de "color" não tem efeito nenhum visível em RCSS.
  ids_.fill = RegisterProperty("fill", "#ffffff").AddParser("string").GetId();
  ids_.rotation = RegisterProperty("rotation", "0deg").AddParser("angle").GetId();
  // EN: DESIGN NOTE (found the hard way -- see polygon_sanity's first failed run): the spec's
  //     syntax "polygon(<sides>, <color>[, <rotation>])" is COMMA-separated. RmlUi's
  //     ShorthandType::FallThrough (used by DecoratorStraightGradientInstancer's own "decorator"
  //     shorthand) splits its argument string by WHITESPACE, not comma -- confirmed both by
  //     reading PropertySpecification::ParseShorthandDeclaration's `split_option` selection
  //     (Comma only for RecursiveCommaSeparated, Whitespace for every other type including
  //     FallThrough) and by the one FallThrough decorator actually exercised in this codebase,
  //     `mask-image: horizontal-gradient(#000f #0000);` in showcase.rcss -- SPACE-separated, no
  //     commas. A comma-separated, purely positional shorthand with an optional TRAILING item
  //     is exactly what ShorthandType::RecursiveCommaSeparated already implements (used by
  //     DecoratorRadialGradient/DecoratorConicGradient's own "decorator" shorthand, e.g.
  //     "shape?, color-stops#"): it splits by comma, then walks items against values
  //     positionally, skipping (not erroring) an item marked optional (`?` suffix) when there is
  //     no value left for it. With plain (non-repeating, non-nested) items as used here, that
  //     positional walk is exactly "sides, color, then rotation only if a 3rd comma value is
  //     present".
  // PT: NOTA DE DESIGN (descoberta na marra -- ver a 1ª execução falha do polygon_sanity): a
  //     sintaxe da spec "polygon(<lados>, <cor>[, <rotação>])" é separada por VÍRGULA. O
  //     ShorthandType::FallThrough do RmlUi (usado pelo próprio shorthand "decorator" de
  //     DecoratorStraightGradientInstancer) separa a string de argumentos por ESPAÇO, não
  //     vírgula -- confirmado tanto lendo a seleção de `split_option` de
  //     PropertySpecification::ParseShorthandDeclaration (Comma só para RecursiveCommaSeparated,
  //     Whitespace para todo o resto incluindo FallThrough) quanto pelo único decorator
  //     FallThrough de fato exercitado neste código, `mask-image: horizontal-gradient(#000f
  //     #0000);` no showcase.rcss -- separado por ESPAÇO, sem vírgulas. Um shorthand puramente
  //     posicional separado por vírgula com um item FINAL opcional é exatamente o que
  //     ShorthandType::RecursiveCommaSeparated já implementa (usado pelo próprio shorthand
  //     "decorator" de DecoratorRadialGradient/DecoratorConicGradient, ex.: "shape?,
  //     color-stops#"): separa por vírgula, depois percorre os itens contra os valores
  //     posicionalmente, pulando (sem erro) um item marcado opcional (sufixo `?`) quando não
  //     sobra valor pra ele. Com itens simples (sem repetição, sem aninhamento) como usado aqui,
  //     essa caminhada posicional é exatamente "sides, color, e rotation só se houver um 3º
  //     valor separado por vírgula".
  RegisterShorthand("decorator", "sides, fill, rotation?", Rml::ShorthandType::RecursiveCommaSeparated);
}

Rml::SharedPtr<Rml::Decorator> PolygonDecoratorInstancer::InstanceDecorator(const Rml::String& /*name*/,
    const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& /*instancer_interface*/) {
  const Rml::Property* p_sides = properties.GetProperty(ids_.sides);
  const Rml::Property* p_fill = properties.GetProperty(ids_.fill);
  const Rml::Property* p_rotation = properties.GetProperty(ids_.rotation);
  if (!p_sides || !p_fill || !p_rotation)
    return nullptr;

  // EN: INPUT HARDENING (v0.2.6 review): RCSS is not always author-trusted -- it can arrive from
  //     a theme, a mod, or dynamically generated content -- so `sides` is validated defensively.
  //     The "number" parser stores the value as a plain float (see PropertyParserNumber::
  //     ParseValue). CRITICAL: the range check is done ON THE FLOAT, BEFORE any conversion to int.
  //     Converting an out-of-int-range or non-finite float to int is UNDEFINED BEHAVIOUR in C++
  //     (float-cast-overflow) -- "polygon(1e30)", "polygon(1e400)" (parses to +inf), or a NaN
  //     could otherwise yield INT_MIN/garbage/a UBSan abort BEFORE any `> 1024` check on the int
  //     ran. So: reject first (single unified check: `!isfinite` catches NaN/inf; `< 3` catches
  //     the floor, the empty "polygon()" whose default sides="0" resolves to 0.0f, and every
  //     negative -- including huge negatives like -1e30; `> MAX` catches the ceiling / hostile
  //     huge positives). Only AFTER the float is proven finite and within [3, POLYGON_MAX_SIDES]
  //     is std::lround/cast reached -- provably overflow-free. The log names the value RECEIVED
  //     (the raw float, %g, so "polygon(-1)"/"polygon(100000)" report -1/100000, not a rounded or
  //     clamped surrogate) and the valid range. Fail-high on invalid input, never fail-silent.
  // PT: HARDENING DE ENTRADA (review v0.2.6): RCSS nem sempre é confiável do autor -- pode vir de
  //     um tema, um mod, ou conteúdo gerado dinamicamente -- então `sides` é validado
  //     defensivamente. O parser "number" guarda o valor como float puro (ver
  //     PropertyParserNumber::ParseValue). CRÍTICO: a checagem de range é feita NO FLOAT, ANTES de
  //     qualquer conversão pra int. Converter um float fora do range de int ou não-finito pra int
  //     é COMPORTAMENTO INDEFINIDO em C++ (float-cast-overflow) -- "polygon(1e30)", "polygon(1e400)"
  //     (parseia pra +inf), ou um NaN poderiam senão produzir INT_MIN/lixo/um abort do UBSan
  //     ANTES de qualquer checagem `> 1024` no int rodar. Então: rejeita primeiro (checagem única
  //     unificada: `!isfinite` pega NaN/inf; `< 3` pega o piso, o "polygon()" vazio cujo default
  //     sides="0" resolve pra 0.0f, e todo negativo -- incluindo negativos gigantes como -1e30;
  //     `> MAX` pega o teto / positivos gigantes hostis). Só DEPOIS de o float ser provado finito
  //     e dentro de [3, POLYGON_MAX_SIDES] o std::lround/cast é alcançado -- provadamente sem
  //     overflow. O log nomeia o valor RECEBIDO (o float bruto, %g, para que
  //     "polygon(-1)"/"polygon(100000)" reportem -1/100000, não um substituto arredondado ou
  //     clampado) e o range válido. Fail-high em entrada inválida, nunca fail-silent.
  const float sides_f = p_sides->Get<float>();
  if (!std::isfinite(sides_f) || sides_f < 3.0f || sides_f > float(POLYGON_MAX_SIDES)) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "decorator: polygon() requires sides in [3, %d], got %g -- decorator ignored "
        "(e.g. an empty 'polygon()' has no sides value).", POLYGON_MAX_SIDES, sides_f);
    return nullptr;
  }
  // EN: sides_f is now proven finite and within [3, POLYGON_MAX_SIDES] -- std::lround (nearest,
  //     round-half-away-from-zero) and the int cast are both overflow-free here. Rounding (vs a
  //     bare truncating cast) makes "polygon(6.0)" and any interpolated fractional resolve to the
  //     nearest side count.
  // PT: sides_f agora é provadamente finito e dentro de [3, POLYGON_MAX_SIDES] -- std::lround
  //     (mais próximo, half-away-from-zero) e o cast pra int são ambos sem overflow aqui. O
  //     arredondamento (vs um cast truncante nu) faz "polygon(6.0)" e qualquer fracionário
  //     interpolado resolver para a contagem de lados mais próxima.
  const int sides = static_cast<int>(std::lround(sides_f));

  // EN: FILL HARDENING (v0.3.1): `fill` went through the passthrough "string" parser above (see
  //     the instancer ctor), so it has had ZERO validation yet -- ParseFill() (the "FILL PARSING"
  //     block preceding PolygonDecorator::Initialise) is where a solid colour, a
  //     "radial-gradient(...)", or a "linear-gradient(...)" is actually recognised and validated
  //     (colour syntax, stop count/positions, angle finiteness -- all via RmlUi's own public
  //     parsers, see that block's doc comment). Any parse failure is fail-high here, exactly like
  //     the `sides` guard above: log the raw text received and reject the whole decorator (return
  //     nullptr) rather than rendering a partial/garbage fill.
  // PT: HARDENING DE PREENCHIMENTO (v0.3.1): `fill` passou pelo parser passthrough "string" acima
  //     (ver o ctor do instancer), então não teve NENHUMA validação ainda -- ParseFill() (o bloco
  //     "FILL PARSING" antes de PolygonDecorator::Initialise) é onde uma cor sólida, um
  //     "radial-gradient(...)", ou um "linear-gradient(...)" é de fato reconhecido e validado
  //     (sintaxe de cor, contagem/posições de stop, finitude de ângulo -- tudo via parsers
  //     próprios do RmlUi, ver o doc-comment daquele bloco). Qualquer falha de parse é fail-high
  //     aqui, exatamente como a guarda de `sides` acima: loga o texto bruto recebido e rejeita o
  //     decorator inteiro (retorna nullptr) em vez de renderizar um preenchimento
  //     parcial/lixo.
  const Rml::String fill_raw = p_fill->Get<Rml::String>();
  PolygonFill fill;
  if (!ParseFill(fill_raw, fill)) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "decorator: polygon() has an invalid fill '%s' -- expected a colour, radial-gradient(...), "
        "or linear-gradient(...) -- decorator ignored.",
        fill_raw.c_str());
    return nullptr;
  }

  // EN: `rotation` is intentionally NOT range-validated: any finite angle is legitimate. Negative
  //     ("polygon(6, #fff, -90deg)" spins counter-clockwise) and >360deg angles are all valid and
  //     must render -- the angle is periodic and fed straight to sin/cos, which accept any finite
  //     real. The ONLY defence here is finiteness: a non-finite angle (a NaN/inf, e.g. from an
  //     animation gone wrong) would produce NaN vertex positions and a garbage/absent polygon, so
  //     it is coerced to 0deg (render unrotated) rather than propagated. This is deliberately
  //     narrower than the `sides` handling -- do NOT add sign/magnitude checks on rotation.
  // PT: `rotation` intencionalmente NÃO é validado por range: qualquer ângulo finito é legítimo.
  //     Negativo ("polygon(6, #fff, -90deg)" gira anti-horário) e ângulos >360deg são todos
  //     válidos e devem renderizar -- o ângulo é periódico e vai direto pro sin/cos, que aceitam
  //     qualquer real finito. A ÚNICA defesa aqui é finitude: um ângulo não-finito (um NaN/inf,
  //     ex.: de uma animação deu errado) produziria posições de vértice NaN e um polígono
  //     lixo/ausente, então é coagido pra 0deg (renderiza sem rotação) em vez de propagado. Isso é
  //     deliberadamente mais estreito que o tratamento de `sides` -- NÃO adicione checagens de
  //     sinal/magnitude na rotation.
  float rotation_rad = ResolveAngleRadians(p_rotation->GetNumericValue());
  if (!std::isfinite(rotation_rad))
    rotation_rad = 0.0f;

  auto decorator = Rml::MakeShared<PolygonDecorator>();
  if (decorator->Initialise(sides, std::move(fill), rotation_rad))
    return decorator;

  return nullptr;
}

} // namespace glintfx
