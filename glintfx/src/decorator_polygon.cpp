// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of PolygonDecorator / PolygonDecoratorInstancer -- see decorator_polygon.hpp
//     for the design rationale and the upstream reference (DecoratorGradient.{h,cpp}).
//
//     Geometry: a triangle-fan of (sides_+1) vertices -- one hub vertex at the box centre plus
//     `sides_` perimeter vertices evenly spaced on a circle of radius = half the SHORTER
//     dimension of the element's paint-area box (so the polygon is always fully inscribed, even
//     in a non-square element -- see the design-decision note on GenerateElementData below).
//     All vertices share the same premultiplied solid fill colour; `sides_` triangles connect
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
//     TODO.md as a possible follow-up, not implemented here (v0.2.6 scope is solid-fill only).
//
// PT: Implementação de PolygonDecorator / PolygonDecoratorInstancer -- ver decorator_polygon.hpp
//     para a racional de design e a referência upstream (DecoratorGradient.{h,cpp}).
//
//     Geometria: um triangle-fan de (sides_+1) vértices -- um vértice-eixo no centro da caixa
//     mais `sides_` vértices de perímetro igualmente espaçados num círculo de raio = metade da
//     dimensão MENOR da caixa de área-de-pintura do elemento (assim o polígono está sempre
//     totalmente inscrito, mesmo num elemento não-quadrado -- ver nota de decisão de design em
//     GenerateElementData abaixo). Todos os vértices compartilham a mesma cor de preenchimento
//     sólida premultiplicada; `sides_` triângulos conectam o eixo a cada par adjacente de
//     vértices de perímetro (eixo, v[i], v[i+1 mod sides_]).
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
//     aqui (escopo da v0.2.6 é preenchimento sólido apenas).
// Copyright (c) 2026 Petrus Silva Costa
#include "decorator_polygon.hpp"

#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/NumericValue.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/RenderManager.h>

namespace glintfx {

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

bool PolygonDecorator::Initialise(int sides, Rml::Colourb color, float rotation_radians) {
  if (sides < 3)
    return false;
  sides_ = sides;
  color_ = color;
  rotation_radians_ = rotation_radians;
  return true;
}

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
  const Rml::ColourbPremultiplied fill = color_.ToPremultiplied(computed.opacity());

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
  const Rml::Vector2f center = offset + size * 0.5f;
  const float radius = 0.5f * Rml::Math::Min(size.x, size.y);
  const float rotation_rad = rotation_radians_;

  Rml::Mesh mesh;
  Rml::Vector<Rml::Vertex>& vertices = mesh.vertices;
  Rml::Vector<int>& indices = mesh.indices;
  vertices.reserve(static_cast<size_t>(sides_) + 1);
  indices.reserve(static_cast<size_t>(sides_) * 3);

  // EN: Vertex 0 is the fan hub (box centre); vertices 1..sides_ are the perimeter, in order.
  // PT: O vértice 0 é o eixo do fan (centro da caixa); vértices 1..sides_ são o perímetro, em ordem.
  vertices.push_back(Rml::Vertex{center, fill, Rml::Vector2f(0, 0)});

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
    const Rml::Vector2f p = center + radius * Rml::Vector2f(Rml::Math::Cos(theta), Rml::Math::Sin(theta));
    vertices.push_back(Rml::Vertex{p, fill, Rml::Vector2f(0, 0)});
  }

  for (int i = 0; i < sides_; ++i) {
    const int a = 1 + i;
    const int b = 1 + (i + 1) % sides_;
    indices.push_back(0);
    indices.push_back(a);
    indices.push_back(b);
  }

  Rml::Geometry* geometry = new Rml::Geometry(render_manager->MakeGeometry(std::move(mesh)));
  return reinterpret_cast<Rml::DecoratorDataHandle>(geometry);
}

void PolygonDecorator::ReleaseElementData(Rml::DecoratorDataHandle element_data) const {
  delete reinterpret_cast<Rml::Geometry*>(element_data);
}

void PolygonDecorator::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const {
  auto* geometry = reinterpret_cast<Rml::Geometry*>(element_data);
  // EN: Same translation convention as every other hand-built decorator in RmlUi (see
  //     DecoratorStraightGradient::RenderElement): mesh vertices are generated relative to the
  //     border-box origin, translated here by the element's absolute border-box offset.
  // PT: Mesma convenção de translação de todo outro decorator feito à mão no RmlUi (ver
  //     DecoratorStraightGradient::RenderElement): os vértices da mesh são gerados relativos à
  //     origem da border-box, translados aqui pelo offset absoluto de border-box do elemento.
  geometry->Render(element->GetAbsoluteOffset(Rml::BoxArea::Border));
}

PolygonDecoratorInstancer::PolygonDecoratorInstancer() {
  ids_.sides = RegisterProperty("sides", "6").AddParser("number").GetId();
  ids_.color = RegisterProperty("color", "#ffffff").AddParser("color").GetId();
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
  RegisterShorthand("decorator", "sides, color, rotation?", Rml::ShorthandType::RecursiveCommaSeparated);
}

Rml::SharedPtr<Rml::Decorator> PolygonDecoratorInstancer::InstanceDecorator(const Rml::String& /*name*/,
    const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& /*instancer_interface*/) {
  const Rml::Property* p_sides = properties.GetProperty(ids_.sides);
  const Rml::Property* p_color = properties.GetProperty(ids_.color);
  const Rml::Property* p_rotation = properties.GetProperty(ids_.rotation);
  if (!p_sides || !p_color || !p_rotation)
    return nullptr;

  // EN: "number" parser always stores its value as a plain float (see
  //     PropertyParserNumber::ParseValue); round-to-nearest before truncating to int so an
  //     author writing "polygon(6.0, ...)" (or a fractional value produced by an animation/
  //     interpolation down the line) does not get floor-truncated to the side below.
  // PT: O parser "number" sempre guarda o valor como float puro (ver
  //     PropertyParserNumber::ParseValue); arredonda antes de truncar pra int para que um autor
  //     escrevendo "polygon(6.0, ...)" (ou um valor fracionário produzido por uma animação/
  //     interpolação no futuro) não seja truncado pra baixo (floor) pro lado de menos.
  const int sides = static_cast<int>(p_sides->Get<float>() + 0.5f);
  if (sides < 3) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "decorator: polygon() requires sides >= 3, got %d -- decorator ignored.", sides);
    return nullptr;
  }

  const Rml::Colourb color = p_color->Get<Rml::Colourb>();
  const float rotation_rad = ResolveAngleRadians(p_rotation->GetNumericValue());

  auto decorator = Rml::MakeShared<PolygonDecorator>();
  if (decorator->Initialise(sides, color, rotation_rad))
    return decorator;

  return nullptr;
}

} // namespace glintfx
