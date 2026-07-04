// SPDX-License-Identifier: MPL-2.0
// EN: Custom RmlUi decorator -- "polygon(<sides>, <color>[, <rotation>])". Fills the element's
//     paint area with a solid-color regular N-gon (triangle-fan around the box centre, radius =
//     half of the SHORTER box dimension). Internal src/ header: freely uses Rml::* types, per
//     the same convention as bootstrap.cpp/render_gl3.cpp -- the encapsulation gate only covers
//     the public glintfx/include/glintfx/ headers, not this file.
//     Molded directly on Rml::DecoratorStraightGradient / DecoratorStraightGradientInstancer
//     (examples/RmlUi/Source/Core/DecoratorGradient.{h,cpp}, pinned commit
//     2cd28864ae25ed345b70598751703a5433b12356 -- the exact FetchContent pin in CMakeLists.txt):
//     same GenerateElementData/ReleaseElementData/RenderElement + Geometry* handle pattern, same
//     RegisterProperty/RegisterShorthand instancer idiom. No shader, no third-party patch, no
//     change to render_gl3.cpp or RmlUi core -- pure extension via the public Decorator/
//     DecoratorInstancer interface.
// PT: Decorator customizado do RmlUi -- "polygon(<lados>, <cor>[, <rotacao>])". Preenche a área
//     de pintura do elemento com um N-ágono regular de cor sólida (triangle-fan ao redor do
//     centro da caixa, raio = metade da dimensão MENOR da caixa). Header interno de src/: usa
//     tipos Rml::* livremente, mesma convenção de bootstrap.cpp/render_gl3.cpp -- o gate de
//     encapsulamento só cobre os headers públicos em glintfx/include/glintfx/, não este arquivo.
//     Moldado diretamente em Rml::DecoratorStraightGradient / DecoratorStraightGradientInstancer
//     (examples/RmlUi/Source/Core/DecoratorGradient.{h,cpp}, commit pinado
//     2cd28864ae25ed345b70598751703a5433b12356 -- o mesmo pin do FetchContent em
//     CMakeLists.txt): mesmo padrão de handle GenerateElementData/ReleaseElementData/
//     RenderElement + Geometry*, mesmo idioma de instancer RegisterProperty/RegisterShorthand.
//     Sem shader, sem patch de terceiro, sem tocar render_gl3.cpp ou o core do RmlUi -- pura
//     extensão via a interface pública Decorator/DecoratorInstancer.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/Types.h>

namespace glintfx {

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
  bool Initialise(int sides, Rml::Colourb color, float rotation_radians);

  Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const override;
  void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
  void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
  int sides_ = 6;
  Rml::Colourb color_{255, 255, 255, 255};
  float rotation_radians_ = 0.f;
};

// EN: Parses "decorator: polygon(<sides>, <color>[, <rotation>]);" from RCSS.
// PT: Parseia "decorator: polygon(<lados>, <cor>[, <rotacao>]);" do RCSS.
class PolygonDecoratorInstancer : public Rml::DecoratorInstancer {
public:
  PolygonDecoratorInstancer();
  ~PolygonDecoratorInstancer() override = default;

  Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties,
      const Rml::DecoratorInstancerInterface& instancer_interface) override;

private:
  struct PropertyIds {
    Rml::PropertyId sides, color, rotation;
  };
  PropertyIds ids_{};
};

} // namespace glintfx
