// SPDX-License-Identifier: MPL-2.0
// EN: Custom RmlUi decorator -- "polygon(<sides>, <fill>[, <rotation>])", where <fill> is either a
//     solid <color> OR a "radial-gradient(...)" / "linear-gradient(...)" function. Fills the
//     element's paint area with a regular N-gon (triangle-fan around the box centre, radius = half
//     of the SHORTER box dimension). Internal src/ header: freely uses Rml::* types, per the same
//     convention as bootstrap.cpp/render_gl3.cpp -- the encapsulation gate only covers the public
//     glintfx/include/glintfx/ headers, not this file.
//     Molded directly on Rml::DecoratorStraightGradient/DecoratorLinearGradient/
//     DecoratorRadialGradient (examples/RmlUi/Source/Core/DecoratorGradient.{h,cpp}, pinned commit
//     2cd28864ae25ed345b70598751703a5433b12356 -- the exact FetchContent pin in CMakeLists.txt):
//     same GenerateElementData/ReleaseElementData/RenderElement + Geometry*/CompiledShader handle
//     pattern, same RegisterProperty/RegisterShorthand instancer idiom.
//     v0.3.1 GRADIENT FILL (consumer-driven, GusWorld): reuses RmlUi's REAL "radial-gradient"/
//     "linear-gradient" fragment shader (Rml::RenderManager::CompileShader, the exact same public
//     API DecoratorRadialGradient/DecoratorLinearGradient call) instead of hand-rolled vertex-colour
//     rings. Because the shader colours each FRAGMENT from an interpolated tex_coord (an affine,
//     i.e. planar, function of vertex position), the result is bit-for-bit the same whether the
//     mesh under it is a full quad (the upstream decorators) or our triangle-fan polygon -- so the
//     polygon gets a perfectly circular/linear blend, not a faceted approximation. No shader code
//     was written for this feature; the fragment shader itself lives in RmlUi's shipped GL3 backend
//     (examples/RmlUi/Backends/RmlUi_Renderer_GL3.cpp, `shader_frag_gradient`) and is reached
//     exclusively through public headers (RenderManager::CompileShader, Geometry::Render with a
//     CompiledShader). No shader, no third-party patch, no change to render_gl3.cpp or RmlUi core.
//     All gradient-syntax parsing below also reuses RmlUi's own public parsers -- looked up by name
//     via Rml::StyleSheetSpecification::GetParser("color" / "color_stop_list" / "angle" /
//     "number_percent") -- rather than reimplementing colour/stop/angle parsing; see
//     decorator_polygon.cpp's ParseFill() for the full rationale and the exact accepted syntax.
// PT: Decorator customizado do RmlUi -- "polygon(<lados>, <preenchimento>[, <rotacao>])", onde
//     <preenchimento> é uma <cor> sólida OU uma função "radial-gradient(...)" / "linear-gradient(...)".
//     Preenche a área de pintura do elemento com um N-ágono regular (triangle-fan ao redor do
//     centro da caixa, raio = metade da dimensão MENOR da caixa). Header interno de src/: usa
//     tipos Rml::* livremente, mesma convenção de bootstrap.cpp/render_gl3.cpp -- o gate de
//     encapsulamento só cobre os headers públicos em glintfx/include/glintfx/, não este arquivo.
//     Moldado diretamente em Rml::DecoratorStraightGradient/DecoratorLinearGradient/
//     DecoratorRadialGradient (examples/RmlUi/Source/Core/DecoratorGradient.{h,cpp}, commit pinado
//     2cd28864ae25ed345b70598751703a5433b12356 -- o mesmo pin do FetchContent em CMakeLists.txt):
//     mesmo padrão de handle GenerateElementData/ReleaseElementData/RenderElement +
//     Geometry*/CompiledShader, mesmo idioma de instancer RegisterProperty/RegisterShorthand.
//     PREENCHIMENTO EM GRADIENTE v0.3.1 (consumer-driven, GusWorld): reusa o shader de fragmento
//     REAL "radial-gradient"/"linear-gradient" do RmlUi (Rml::RenderManager::CompileShader, a MESMA
//     API pública que DecoratorRadialGradient/DecoratorLinearGradient chamam) em vez de anéis de
//     vertex-color feitos à mão. Como o shader colore cada FRAGMENTO a partir de um tex_coord
//     interpolado (uma função afim, isto é, planar, da posição do vértice), o resultado é idêntico
//     bit-a-bit seja a malha por baixo um quad cheio (os decorators upstream) ou o nosso
//     triangle-fan poligonal -- então o polígono recebe um blend perfeitamente circular/linear, não
//     uma aproximação facetada. Nenhum código de shader foi escrito para esta feature; o shader de
//     fragmento em si mora no backend GL3 já embarcado do RmlUi
//     (examples/RmlUi/Backends/RmlUi_Renderer_GL3.cpp, `shader_frag_gradient`) e é alcançado
//     exclusivamente por headers públicos (RenderManager::CompileShader, Geometry::Render com um
//     CompiledShader). Sem shader novo, sem patch de terceiro, sem tocar render_gl3.cpp ou o core
//     do RmlUi. Todo o parsing de sintaxe de gradiente abaixo também reusa os parsers públicos do
//     próprio RmlUi -- buscados por nome via Rml::StyleSheetSpecification::GetParser("color" /
//     "color_stop_list" / "angle" / "number_percent") -- em vez de reimplementar parsing de
//     cor/stop/ângulo; ver ParseFill() em decorator_polygon.cpp para a racional completa e a
//     sintaxe exata aceita.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Types.h>

namespace glintfx {

// EN: The three fill kinds "polygon()" accepts as its 2nd argument.
// PT: Os três tipos de preenchimento que "polygon()" aceita como 2º argumento.
enum class PolygonFillMode { Solid, RadialGradient, LinearGradient };

// EN: Fully-resolved fill descriptor, produced by ParseFill() (decorator_polygon.cpp) from the raw
//     RCSS text and handed to PolygonDecorator::Initialise(). Only the fields relevant to `mode`
//     are meaningful; the others sit at their default. `stops` positions are ALWAYS normalised to
//     Unit::NUMBER in [0,1] by the time this struct is filled in (see ResolveStopPositions) --
//     GenerateElementData never resolves percentages itself.
// PT: Descritor de preenchimento totalmente resolvido, produzido por ParseFill()
//     (decorator_polygon.cpp) a partir do texto RCSS bruto e entregue a
//     PolygonDecorator::Initialise(). Só os campos relevantes ao `mode` têm significado; os demais
//     ficam no default. As posições de `stops` estão SEMPRE normalizadas para Unit::NUMBER em
//     [0,1] no momento em que esta struct é preenchida (ver ResolveStopPositions) --
//     GenerateElementData nunca resolve porcentagens por conta própria.
struct PolygonFill {
  PolygonFillMode mode = PolygonFillMode::Solid;
  // Solid only.
  Rml::Colourb solid_color{255, 255, 255, 255};
  // Radial and Linear: colour stops, positions normalised to Unit::NUMBER in [0,1], ascending.
  Rml::ColorStopList stops;
  // Radial only: gradient centre as a fraction of the element's own fill box ("at <x%> <y%>"),
  // default {0.5, 0.5} (box centre). t=1 (the last stop) lands on the polygon's own inscribed-
  // circle radius, NOT the CSS default farthest-corner -- see decorator_polygon.cpp.
  Rml::Vector2f radial_center_fraction{0.5f, 0.5f};
  // Linear only: gradient direction, CSS `linear-gradient(<angle>, ...)` convention (0deg = up,
  // clockwise). The gradient LINE spans exactly the polygon's inscribed-circle diameter, centred
  // on the polygon centre -- independent of the `rotation` argument (rotation only turns the
  // N-gon's vertices, never the fill).
  float linear_angle_radians = 0.f;
};

// EN: The decorator itself -- holds the resolved (sides, color, rotation) triple and generates
//     one triangle-fan Geometry per decorated element.
// PT: O decorator em si -- guarda a tripla resolvida (lados, cor, rotação) e gera uma Geometry
//     de triangle-fan por elemento decorado.
class PolygonDecorator : public Rml::Decorator {
public:
  PolygonDecorator() = default;
  ~PolygonDecorator() override = default;

  // EN: Returns false (and leaves the decorator unusable) if sides < 3 -- caller must not use
  //     this decorator instance in that case (mirrors DecoratorStraightGradient::Initialise's
  //     bool-return convention; the instancer turns false into a nullptr SharedPtr<Decorator>).
  // PT: Retorna false (e deixa o decorator inutilizável) se sides < 3 -- o chamador não deve
  //     usar esta instância nesse caso (espelha a convenção de retorno bool de
  //     DecoratorStraightGradient::Initialise; o instancer transforma false em SharedPtr<Decorator>
  //     nulo).
  //     `rotation_radians` is already resolved to radians by the instancer (RCSS "deg"/"rad"
  //     units are both normalised there) -- this class stores and consumes radians only.
  bool Initialise(int sides, PolygonFill fill, float rotation_radians);

  Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const override;
  void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
  void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
  int sides_ = 6;
  PolygonFill fill_;
  float rotation_radians_ = 0.f;
};

// EN: Parses "decorator: polygon(<sides>, <fill>[, <rotation>]);" from RCSS, where <fill> is a
//     solid <color>, a "radial-gradient(...)", or a "linear-gradient(...)" -- see ParseFill() in
//     decorator_polygon.cpp for the exact accepted syntax of each.
// PT: Parseia "decorator: polygon(<lados>, <preenchimento>[, <rotacao>]);" do RCSS, onde
//     <preenchimento> é uma <cor> sólida, um "radial-gradient(...)", ou um "linear-gradient(...)"
//     -- ver ParseFill() em decorator_polygon.cpp para a sintaxe exata aceita de cada um.
class PolygonDecoratorInstancer : public Rml::DecoratorInstancer {
public:
  PolygonDecoratorInstancer();
  ~PolygonDecoratorInstancer() override = default;

  Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties,
      const Rml::DecoratorInstancerInterface& instancer_interface) override;

private:
  struct PropertyIds {
    Rml::PropertyId sides, fill, rotation;
  };
  PropertyIds ids_{};
};

} // namespace glintfx
