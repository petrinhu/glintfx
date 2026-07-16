// SPDX-License-Identifier: MPL-2.0
// EN: Custom RmlUi decorator -- "ripple([<max-radius>])" -- a SCREEN-SPACE distortion effect:
//     the element's own paint box is filled by sampling the glintfx-captured FBO-0 backdrop
//     (RenderGl3::begin_frame_compose -> Gl3RenderInterface::EnsureBackdropCaptured,
//     render_gl3.cpp, L1.22-CAPTURE) through a radial refraction ring that expands over time --
//     a "time distortion" look. Composited ONLY inside a smoothstep-shaped ring around
//     `distance(fragment, origin) == phase * max_radius`; everywhere else the element is fully
//     transparent (alpha=0), so an author places this decorator on a fullscreen-covering
//     element and only the travelling ring visibly perturbs the host's content underneath.
//
//     Internal src/ header: freely uses Rml::* types, same convention as
//     decorator_image_tint.hpp/decorator_polygon.hpp/bootstrap.cpp/render_gl3.cpp -- the
//     encapsulation gate only covers the public glintfx/include/glintfx/ headers, not this file.
//
//     Molded DIRECTLY on decorator_image_tint.{hpp,cpp} (L1.22-WAVE mirrors GLINTFX-TINT-2/3):
//     same GenerateElementData/ReleaseElementData/RenderElement shape, same per-element raw GL
//     VAO/VBO/EBO (box-corner quad, glintfx-owned, NOT an Rml::Geometry-driven draw) dispatched
//     through a new "glintfx-ripple"-tagged trio of Gl3RenderInterface::CompileShader/
//     RenderShader/ReleaseShader overrides (render_gl3.cpp) -- for the identical reason
//     ADR-0010 Decision (a) gives for image-tint: the actual GL draw call (and
//     this->GetTransform()/this->ResetProgram()) is only reachable from inside a
//     Gl3RenderInterface method, never from decorator code, and RenderShader's own
//     `geometry_handle` parameter (RmlUi's opaque Rml::CompiledGeometryHandle) is NEVER
//     dereferenced for a ripple draw -- we draw from the glintfx-owned VAO instead (see
//     GlintfxRippleShaderData's doc comment in render_gl3.cpp).
//
//     UNLIKE image-tint, this decorator carries NO Rml::Texture at all (there is no author-
//     supplied image -- the "texture" being sampled is the glintfx-captured backdrop, bound
//     directly by Gl3RenderInterface::RenderShader's ripple branch, never through RmlUi's own
//     texture-handle plumbing) -- Geometry::Render is called with a default-constructed
//     (falsy/invalid) Rml::Texture purely to satisfy RenderManager::Render's API contract
//     (RMLUI_ASSERT(geometry); a falsy `texture` short-circuits RenderManager::Render's own
//     texture-handle resolution, see RenderManager.cpp:219 -- confirmed by reading the pinned
//     source before writing this comment).
//
//     COST-ZERO-WHEN-INACTIVE (L1.22-CAPTURE): THIS DECORATOR NO LONGER OWNS ANY GATE COUNTER --
//     a prior revision (commit 647350f) had RippleDecoratorInstancer track a shared
//     `active_instances_` refcount (incremented in GenerateElementData / decremented in
//     ReleaseElementData) that Gl3RenderInterface read BEFORE Rml::Context::Render() ran, at the
//     top of begin_frame_compose. That refcount is only accurate DURING/AFTER Context::Render()'s
//     own tree walk (GenerateElementData is called from Element::Render() -> RenderEffects(),
//     confirmed by reading the pinned Source/Core/ElementEffects.cpp), which made the gate read
//     STALE (last frame's) data on the very FIRST frame a ripple decorator became active -- a
//     cold-start bug an adversarial QA gate (ripple_sanity.cpp) caught and reproduced. The fix
//     (render_gl3.cpp's ArmBackdropCapture/EnsureBackdropCaptured pair) makes the "is any ripple
//     active this frame" question implicit and free instead: the real GL capture now runs
//     on-demand, from INSIDE the "glintfx-ripple" RenderShader branch itself, the first time (per
//     frame) that branch is actually reached -- which can only happen if a RippleDecorator is
//     actually drawing. See render_gl3.cpp's ArmBackdropCapture/EnsureBackdropCaptured doc
//     comment for the full derivation, including why this also sidesteps a second latent risk
//     the old counter design had (RmlUi's own Decorator instances can be shared/cached across
//     elements with an identical declaration, StyleSheet::InstanceDecorators -- counting
//     CONSTRUCTED instances would have needed to reason about that sharing too).
// PT: Decorator customizado do RmlUi -- "ripple([<raio-max>])" -- um efeito de distorção
//     EM ESPAÇO DE TELA: a própria caixa de pintura do elemento é preenchida amostrando o
//     backdrop do FBO-0 capturado pela glintfx (RenderGl3::begin_frame_compose ->
//     Gl3RenderInterface::EnsureBackdropCaptured, render_gl3.cpp, L1.22-CAPTURE) através de um
//     anel de refração radial que se expande no tempo -- um visual de "distorção de tempo".
//     Composto SÓ dentro de um anel em forma de smoothstep ao redor de
//     `distance(fragmento, origem) == phase * raio_max`; em todo o resto o elemento é
//     inteiramente transparente (alpha=0), então um autor posiciona este decorator num elemento
//     que cobre a tela inteira e só o anel viajante perturba visivelmente o conteúdo do host por
//     baixo.
//
//     Header interno de src/: usa tipos Rml::* livremente, mesma convenção de
//     decorator_image_tint.hpp/decorator_polygon.hpp/bootstrap.cpp/render_gl3.cpp -- o gate de
//     encapsulamento só cobre os headers públicos em glintfx/include/glintfx/, não este arquivo.
//
//     Moldado DIRETAMENTE em decorator_image_tint.{hpp,cpp} (L1.22-WAVE espelha
//     GLINTFX-TINT-2/3): mesma forma GenerateElementData/ReleaseElementData/RenderElement, mesma
//     VAO/VBO/EBO GL crua por-elemento (quad de cantos de caixa, dona glintfx, NÃO um draw
//     conduzido por Rml::Geometry) despachada por um novo trio de overrides marcados
//     "glintfx-ripple" de Gl3RenderInterface::CompileShader/RenderShader/ReleaseShader
//     (render_gl3.cpp) -- pelo motivo idêntico que a Decisão (a) do ADR-0010 dá para o
//     image-tint: a chamada de draw GL de fato (e this->GetTransform()/this->ResetProgram()) só
//     é alcançável de dentro de um método de Gl3RenderInterface, nunca do código de decorator, e
//     o próprio parâmetro `geometry_handle` de RenderShader (o Rml::CompiledGeometryHandle
//     opaco do RmlUi) NUNCA é desreferenciado num draw de ripple -- desenhamos a partir da VAO
//     dona da glintfx em vez disso (ver o doc-comment de GlintfxRippleShaderData em
//     render_gl3.cpp).
//
//     DIFERENTE do image-tint, este decorator não carrega NENHUM Rml::Texture (não há imagem
//     fornecida pelo autor -- a "textura" amostrada é o backdrop capturado pela glintfx,
//     vinculado diretamente pelo ramo de ripple de Gl3RenderInterface::RenderShader, nunca pela
//     própria tubulação de texture-handle do RmlUi) -- Geometry::Render é chamado com um
//     Rml::Texture default-construído (falso/inválido) só para satisfazer o contrato de API de
//     RenderManager::Render (RMLUI_ASSERT(geometry); um `texture` falso faz curto-circuito na
//     própria resolução de texture-handle de RenderManager::Render, ver
//     RenderManager.cpp:219 -- confirmado lendo o source pinado antes de escrever este
//     comentário).
//
//     CUSTO-ZERO-QUANDO-INATIVO (L1.22-CAPTURE): ESTE DECORATOR NÃO É MAIS DONO DE NENHUM
//     CONTADOR DE GATE -- uma revisão anterior (commit 647350f) fazia RippleDecoratorInstancer
//     rastrear um refcount `active_instances_` compartilhado (incrementado em
//     GenerateElementData / decrementado em ReleaseElementData) que Gl3RenderInterface lia ANTES
//     de Rml::Context::Render() rodar, no topo de begin_frame_compose. Esse refcount só é preciso
//     DURANTE/DEPOIS da própria caminhada de árvore de Context::Render() (GenerateElementData é
//     chamado a partir de Element::Render() -> RenderEffects(), confirmado lendo o
//     Source/Core/ElementEffects.cpp pinado), o que fazia o gate ler dado OBSOLETO (do frame
//     anterior) no PRIMEIRO frame em que um decorator ripple ficava ativo -- um bug de cold-start
//     que um gate adversarial de QA (ripple_sanity.cpp) pegou e reproduziu. O fix (o par
//     ArmBackdropCapture/EnsureBackdropCaptured de render_gl3.cpp) torna a pergunta "algum ripple
//     está ativo neste frame" implícita e grátis em vez disso: a captura GL real agora roda sob
//     demanda, de DENTRO do próprio ramo de RenderShader "glintfx-ripple", na primeira vez (por
//     frame) que aquele ramo é de fato alcançado -- o que só pode acontecer se um RippleDecorator
//     estiver de fato desenhando. Ver o doc-comment de ArmBackdropCapture/EnsureBackdropCaptured
//     de render_gl3.cpp pra derivação completa, inclusive por que isso também contorna um segundo
//     risco latente que o design de contador antigo tinha (as próprias instâncias de Decorator
//     do RmlUi podem ser compartilhadas/cacheadas entre elementos com uma declaração idêntica,
//     StyleSheet::InstanceDecorators -- contar instâncias CONSTRUÍDAS precisaria ter raciocinado
//     sobre esse compartilhamento também).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/Types.h>

namespace glintfx {

// EN: The decorator itself -- holds the resolved (constant, decorator-argument) `max_radius`
//     value. `max_radius_` of 0.f means "auto" -- Gl3RenderInterface::RenderShader (the only
//     place that KNOWS the current capture resolution) substitutes the captured backdrop's own
//     diagonal in that case; see render_gl3.cpp's GlintfxRippleShaderData::max_radius_arg doc
//     comment.
// PT: O decorator em si -- guarda o valor `max_radius` resolvido (constante, argumento de
//     decorator). `max_radius_` de 0.f significa "auto" -- Gl3RenderInterface::RenderShader (o
//     único lugar que CONHECE a resolução de captura atual) substitui pela própria diagonal do
//     backdrop capturado nesse caso; ver o doc-comment de
//     GlintfxRippleShaderData::max_radius_arg em render_gl3.cpp.
class RippleDecorator : public Rml::Decorator {
public:
  RippleDecorator() = default;
  ~RippleDecorator() override = default;

  void Initialise(float max_radius);

  Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const override;
  void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
  void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
  float max_radius_ = 0.f;
};

// EN: Parses "decorator: ripple([<max-radius>]);" -- the ONLY decorator-shorthand argument is
//     the optional positional `max-radius` (a plain <number>, default "0" == auto); unlike
//     image-tint's `src` this one is genuinely optional -- an empty "ripple()" (or bare
//     "ripple") is a perfectly valid, common case, NOT rejected (contrast with
//     ImageTintDecoratorInstancer::InstanceDecorator's empty-src rejection -- there is no
//     equivalent "nothing to draw" failure mode here since the effect degrades to radius=auto,
//     never to an unusable decorator).
//     The five "ripple-origin-x"/"ripple-origin-y"/"ripple-phase"/"ripple-strength"/
//     "ripple-width" properties are NOT decorator arguments -- exactly like image-tint's three
//     "image-tint-*" properties, they are registered as ordinary, globally-scoped RCSS element
//     properties (Rml::StyleSheetSpecification::RegisterProperty, in this instancer's ctor,
//     decorator_ripple.cpp) and read directly off the element inside
//     RippleDecorator::RenderElement -- independently animatable via ordinary
//     `transition`/`@keyframes`, which is exactly how an author drives `ripple-phase` from 0 to
//     1 over time to animate the travelling ring.
// PT: Parseia "decorator: ripple([<raio-max>]);" -- o ÚNICO argumento de shorthand de decorator
//     é o `max-radius` posicional opcional (um <number> comum, default "0" == auto); diferente
//     do `src` do image-tint este é genuinamente opcional -- um "ripple()" vazio (ou "ripple"
//     nu) é um caso perfeitamente válido e comum, NÃO rejeitado (contraste com a rejeição de
//     src vazio de ImageTintDecoratorInstancer::InstanceDecorator -- não há modo de falha
//     equivalente "nada para desenhar" aqui já que o efeito degrada para raio=auto, nunca para
//     um decorator inutilizável).
//     As cinco propriedades "ripple-origin-x"/"ripple-origin-y"/"ripple-phase"/
//     "ripple-strength"/"ripple-width" NÃO são argumentos de decorator -- exatamente como as
//     três propriedades "image-tint-*" do image-tint, são registradas como propriedades RCSS de
//     elemento comuns, de escopo global (Rml::StyleSheetSpecification::RegisterProperty, no ctor
//     deste instancer, decorator_ripple.cpp) e lidas diretamente do elemento dentro de
//     RippleDecorator::RenderElement -- animáveis independentemente via `transition`/
//     `@keyframes` comuns, que é exatamente como um autor conduz `ripple-phase` de 0 a 1 ao
//     longo do tempo para animar o anel viajante.
class RippleDecoratorInstancer : public Rml::DecoratorInstancer {
public:
  RippleDecoratorInstancer();
  ~RippleDecoratorInstancer() override = default;

  Rml::SharedPtr<Rml::Decorator> InstanceDecorator(const Rml::String& name, const Rml::PropertyDictionary& properties,
      const Rml::DecoratorInstancerInterface& instancer_interface) override;

private:
  Rml::PropertyId max_radius_id_{};
};

} // namespace glintfx
