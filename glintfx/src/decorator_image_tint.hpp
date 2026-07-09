// SPDX-License-Identifier: MPL-2.0
// EN: Custom RmlUi decorator -- "image-tint(<url>)" -- draws a texture like the native "image()"
//     decorator, but three standalone (non-decorator-argument) RCSS properties --
//     "image-tint-color"/"image-tint-mode"/"image-tint-threshold" -- select a LUMINANCE-KEY tint:
//     recolour only light+neutral texels (weighted by smoothstep(threshold,1,luminance) *
//     (1-saturation)), leaving already-saturated (e.g. gold trim) and dark (e.g. stone) texels
//     alone. This is NOT the same as RmlUi's native "image-color" (a uniform multiply that tints
//     every texel equally). See ADR-0010 (docs/adr/0010-image-tint-luminance-key.md) for the full
//     design rationale -- especially why the decorator function itself needs its OWN name
//     ("image-tint", not "image") even though the three tint properties are ordinary, globally-
//     registered, independently-animatable RCSS properties, exactly as originally specified.
//     Internal src/ header: freely uses Rml::* types, same convention as decorator_polygon.hpp/
//     bootstrap.cpp/render_gl3.cpp -- the encapsulation gate only covers the public
//     glintfx/include/glintfx/ headers, not this file.
//     Molded on decorator_polygon.hpp's Initialise/GenerateElementData/ReleaseElementData/
//     RenderElement shape and instancer-registers-properties-in-ctor idiom -- see that file for
//     the established conventions this one follows. UNLIKE polygon(), this decorator's per-
//     element data (decorator_image_tint.cpp) also owns a raw GL VAO/VBO/EBO (glintfx's first
//     hand-rolled GL geometry outside of what RmlUi itself manages) -- the custom fragment shader
//     that consumes it is compiled/dispatched via new "glintfx-tint"-tagged overrides of
//     Gl3RenderInterface::CompileShader/RenderShader/ReleaseShader (render_gl3.cpp) -- see
//     ADR-0010 Decision (a) for why the draw must happen from inside those overrides (this->
//     GetTransform()/this->ResetProgram() are only reachable there) and never touches the RmlUi-
//     opaque Rml::CompiledGeometryHandle that RenderShader also receives.
// PT: Decorator customizado do RmlUi -- "image-tint(<url>)" -- desenha uma textura como o
//     decorator nativo "image()", mas três propriedades RCSS standalone (não argumento de
//     decorator) -- "image-tint-color"/"image-tint-mode"/"image-tint-threshold" -- selecionam um
//     tingimento LUMINANCE-KEY: recolore só texels claros+neutros (ponderado por
//     smoothstep(threshold,1,luminância) * (1-saturação)), preservando texels já saturados (ex.:
//     trim ouro) e escuros (ex.: pedra). Isso NÃO é o mesmo que o "image-color" nativo do RmlUi
//     (um multiply uniforme que tinge todo texel igualmente). Ver ADR-0010
//     (docs/adr/0010-image-tint-luminance-key.md) para a racional completa de design --
//     especialmente por que o próprio decorator precisa do SEU PRÓPRIO nome ("image-tint", não
//     "image") mesmo as três propriedades de tingimento sendo propriedades RCSS comuns,
//     registradas globalmente, animáveis independentemente, exatamente como originalmente
//     especificado.
//     Header interno de src/: usa tipos Rml::* livremente, mesma convenção de
//     decorator_polygon.hpp/bootstrap.cpp/render_gl3.cpp -- o gate de encapsulamento só cobre os
//     headers públicos em glintfx/include/glintfx/, não este arquivo.
//     Moldado na forma Initialise/GenerateElementData/ReleaseElementData/RenderElement de
//     decorator_polygon.hpp e no idioma de instancer-registra-propriedades-no-ctor -- ver aquele
//     arquivo para as convenções já estabelecidas que este segue. DIFERENTE de polygon(), o dado
//     por-elemento deste decorator (decorator_image_tint.cpp) também é dono de uma VAO/VBO/EBO GL
//     crua (a primeira geometria GL feita à mão da glintfx fora do que o próprio RmlUi gerencia)
//     -- o shader de fragmento customizado que a consome é compilado/despachado via novos
//     overrides marcados "glintfx-tint" de Gl3RenderInterface::CompileShader/RenderShader/
//     ReleaseShader (render_gl3.cpp) -- ver a Decisão (a) do ADR-0010 pra por que o draw precisa
//     acontecer de dentro desses overrides (this->GetTransform()/this->ResetProgram() só são
//     alcançáveis lá) e nunca toca o Rml::CompiledGeometryHandle opaco do RmlUi que RenderShader
//     também recebe.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/Texture.h>
#include <RmlUi/Core/Types.h>

namespace glintfx {

// EN: Ordinal MUST match the 0-based position of each name in the comma-separated keyword list
//     passed to AddParser("keyword", "none, multiply, luminance-multiply, screen") in
//     ImageTintDecoratorInstancer's ctor (decorator_image_tint.cpp) -- RmlUi's keyword property
//     parser (Source/Core/PropertyParserKeyword.cpp) resolves a matched keyword to that ordinal
//     as a plain int (Property::Get<int>()), confirmed against how RmlUi's OWN "display"/
//     "position" keyword properties are read (Source/Core/ElementStyle.cpp:
//     values.display((Display)p->Get<int>())). This enum's ordinals are ALSO the exact `_mode`
//     int values the GLSL fragment shader switches on (render_gl3.cpp) -- keep the three in sync
//     if this list is ever extended.
// PT: O ordinal PRECISA casar com a posição 0-baseada de cada nome na lista de keywords separada
//     por vírgula passada a AddParser("keyword", "none, multiply, luminance-multiply, screen") no
//     ctor de ImageTintDecoratorInstancer (decorator_image_tint.cpp) -- o parser de propriedade
//     keyword do RmlUi (Source/Core/PropertyParserKeyword.cpp) resolve uma keyword casada para
//     esse ordinal como um int puro (Property::Get<int>()), confirmado contra como as próprias
//     propriedades keyword "display"/"position" do RmlUi são lidas (Source/Core/ElementStyle.cpp:
//     values.display((Display)p->Get<int>())). Os ordinais deste enum são TAMBÉM os valores int
//     `_mode` exatos sobre os quais o shader de fragmento GLSL faz switch (render_gl3.cpp) --
//     mantenha os três em sincronia se esta lista for estendida algum dia.
enum class ImageTintMode : int {
  None = 0,
  Multiply = 1,
  LuminanceMultiply = 2,
  Screen = 3,
};

// EN: The decorator itself -- holds the resolved source texture (shared/deduplicated by RmlUi's
//     own texture cache across every element using the same "image-tint(<url>)" rule, exactly
//     like the native "image" decorator -- see ADR-0010 Decision (e)) and generates one textured
//     quad (Rml::Geometry, for the vanilla/mode=None path) PLUS one raw GL VAO/VBO/EBO (for the
//     tint-shader path) per decorated element.
// PT: O decorator em si -- guarda a textura de origem resolvida (compartilhada/deduplicada pelo
//     cache de textura do próprio RmlUi entre todo elemento usando a mesma regra
//     "image-tint(<url>)", exatamente como o decorator nativo "image" -- ver a Decisão (e) do
//     ADR-0010) e gera um quad texturizado (Rml::Geometry, pro caminho vanilla/mode=None) MAIS
//     uma VAO/VBO/EBO GL crua (pro caminho de shader de tingimento) por elemento decorado.
class ImageTintDecorator : public Rml::Decorator {
public:
  ImageTintDecorator() = default;
  ~ImageTintDecorator() override = default;

  // EN: `texture` must already be a truthy (resolved) Rml::Texture -- the instancer rejects a
  //     failed/empty resolution BEFORE constructing this decorator (fail-high, mirrors
  //     PolygonDecorator's Initialise/InstanceDecorator split).
  // PT: `texture` já precisa ser um Rml::Texture verdadeiro (resolvido) -- o instancer rejeita
  //     uma resolução falha/vazia ANTES de construir este decorator (fail-high, espelha a
  //     divisão Initialise/InstanceDecorator de PolygonDecorator).
  void Initialise(Rml::Texture texture);

  Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const override;
  void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
  void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
  Rml::Texture texture_;
};

// EN: Parses "decorator: image-tint(<url>);" -- a SINGLE positional argument, unlike polygon()'s
//     three. The "image-tint-color"/"image-tint-mode"/"image-tint-threshold" properties are NOT
//     decorator arguments -- they are registered as ordinary, globally-scoped RCSS element
//     properties (Rml::StyleSheetSpecification::RegisterProperty, in this instancer's ctor,
//     decorator_image_tint.cpp) and read directly off the element inside
//     ImageTintDecorator::RenderElement -- see ADR-0010 Decision (b) for why (orthogonal,
//     independently-animatable via ordinary `transition`/`@keyframes`, regardless of which
//     decorator function draws the element).
// PT: Parseia "decorator: image-tint(<url>);" -- UM ÚNICO argumento posicional, diferente dos
//     três do polygon(). As propriedades "image-tint-color"/"image-tint-mode"/
//     "image-tint-threshold" NÃO são argumentos de decorator -- são registradas como propriedades
//     RCSS de elemento comuns, de escopo global (Rml::StyleSheetSpecification::RegisterProperty,
//     no ctor deste instancer, decorator_image_tint.cpp) e lidas diretamente do elemento dentro
//     de ImageTintDecorator::RenderElement -- ver a Decisão (b) do ADR-0010 pro motivo (ortogonal,
//     animável independentemente via `transition`/`@keyframes` comuns, independente de qual
//     função de decorator desenha o elemento).
class ImageTintDecoratorInstancer : public Rml::DecoratorInstancer {
public:
  ImageTintDecoratorInstancer();
  ~ImageTintDecoratorInstancer() override = default;

  Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties,
      const Rml::DecoratorInstancerInterface& instancer_interface) override;

private:
  Rml::PropertyId src_id_{};
};

} // namespace glintfx
