// SPDX-License-Identifier: MPL-2.0
// EN: Custom RmlUi decorator -- "ripple([<max-radius>])" -- a SCREEN-SPACE distortion effect:
//     the element's own paint box is filled by sampling the glintfx-captured FBO-0 backdrop
//     (RenderGl3::begin_frame_compose -> Gl3RenderInterface::CaptureBackdrop, render_gl3.cpp,
//     L1.22-CAPTURE) through a radial refraction ring that expands over time -- a "time
//     distortion" look. Composited ONLY inside a smoothstep-shaped ring around
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
//     ACTIVE-INSTANCE GATE (L1.22-CAPTURE, the "cost when nobody uses it is ZERO" requirement):
//     RippleDecoratorInstancer owns a single `int active_instances_` counter, SHARED by every
//     RippleDecorator this instancer ever creates (there is one instancer, but potentially many
//     separate "decorator: ripple(...)" RCSS rules, each producing its own RippleDecorator
//     instance via InstanceDecorator -- the counter must be global across all of them, not
//     per-rule, so Gl3RenderInterface::CaptureBackdrop can ask a single "is ANY ripple element
//     alive right now" question). Each RippleDecorator holds a non-owning `int*` back into that
//     counter (handed in by Initialise, alongside the resolved `max_radius` argument);
//     GenerateElementData increments it on the SUCCESS path only (never on an early
//     INVALID_DECORATORDATAHANDLE return -- RmlUi never calls ReleaseElementData for a handle it
//     never received, so an unconditional increment would leak the count upward forever),
//     ReleaseElementData decrements it unconditionally (RmlUi's own per-element contract
//     guarantees ReleaseElementData runs exactly once for every handle GenerateElementData
//     handed back). Gl3RenderInterface reads `*counter > 0` at the very top of every
//     begin_frame_compose call (via RippleDecoratorInstancer::active_instance_count_ptr(),
//     wired in by bootstrap.cpp right after construction+registration) and skips the entire
//     glCopyTexSubImage2D capture -- no allocation, no GL call at all -- whenever it is zero.
// PT: Decorator customizado do RmlUi -- "ripple([<raio-max>])" -- um efeito de distorção
//     EM ESPAÇO DE TELA: a própria caixa de pintura do elemento é preenchida amostrando o
//     backdrop do FBO-0 capturado pela glintfx (RenderGl3::begin_frame_compose ->
//     Gl3RenderInterface::CaptureBackdrop, render_gl3.cpp, L1.22-CAPTURE) através de um anel de
//     refração radial que se expande no tempo -- um visual de "distorção de tempo". Composto SÓ
//     dentro de um anel em forma de smoothstep ao redor de
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
//     GATE DE INSTÂNCIA ATIVA (L1.22-CAPTURE, o requisito "custo quando ninguém usa é ZERO"):
//     RippleDecoratorInstancer é dono de um único contador `int active_instances_`,
//     COMPARTILHADO por todo RippleDecorator que este instancer já criou (há um instancer, mas
//     potencialmente muitas regras RCSS "decorator: ripple(...)" separadas, cada uma produzindo
//     sua própria instância de RippleDecorator via InstanceDecorator -- o contador precisa ser
//     global entre todas elas, não por-regra, para que Gl3RenderInterface::CaptureBackdrop possa
//     fazer uma única pergunta "existe QUALQUER elemento ripple vivo agora"). Cada
//     RippleDecorator guarda um `int*` não-dono de volta pra esse contador (entregue por
//     Initialise, junto do argumento `max_radius` resolvido); GenerateElementData o incrementa
//     SÓ no caminho de sucesso (nunca num retorno antecipado de INVALID_DECORATORDATAHANDLE --
//     o RmlUi nunca chama ReleaseElementData para um handle que nunca recebeu, então um
//     incremento incondicional vazaria a contagem para cima para sempre), ReleaseElementData o
//     decrementa incondicionalmente (o próprio contrato por-elemento do RmlUi garante que
//     ReleaseElementData roda exatamente uma vez para todo handle que GenerateElementData
//     devolveu). Gl3RenderInterface lê `*counter > 0` bem no topo de toda chamada de
//     begin_frame_compose (via RippleDecoratorInstancer::active_instance_count_ptr(), conectado
//     por bootstrap.cpp logo após construção+registro) e pula a captura glCopyTexSubImage2D
//     inteira -- nenhuma alocação, nenhuma chamada GL nenhuma -- sempre que for zero.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/Types.h>

namespace glintfx {

// EN: The decorator itself -- holds the resolved (constant, decorator-argument) `max_radius`
//     value and the non-owning back-pointer into the owning instancer's shared active-instance
//     counter. `max_radius_` of 0.f means "auto" -- Gl3RenderInterface::RenderShader (the only
//     place that KNOWS the current capture resolution) substitutes the captured backdrop's own
//     diagonal in that case; see render_gl3.cpp's GlintfxRippleShaderData::max_radius_arg doc
//     comment.
// PT: O decorator em si -- guarda o valor `max_radius` resolvido (constante, argumento de
//     decorator) e o contra-ponteiro não-dono para o contador de instância ativa compartilhado
//     do instancer dono. `max_radius_` de 0.f significa "auto" -- Gl3RenderInterface::RenderShader
//     (o único lugar que CONHECE a resolução de captura atual) substitui pela própria diagonal
//     do backdrop capturado nesse caso; ver o doc-comment de
//     GlintfxRippleShaderData::max_radius_arg em render_gl3.cpp.
class RippleDecorator : public Rml::Decorator {
public:
  RippleDecorator() = default;
  ~RippleDecorator() override = default;

  // EN: `active_instance_counter` is a non-owning pointer into the owning
  //     RippleDecoratorInstancer's shared counter -- MUST outlive every RippleDecorator this
  //     instancer creates, which it does per the same lifetime rule bootstrap.cpp documents for
  //     polygon_instancer/tint_instancer (the instancer itself outlives Rml::Shutdown()).
  // PT: `active_instance_counter` é um ponteiro não-dono para o contador compartilhado do
  //     RippleDecoratorInstancer dono -- PRECISA sobreviver a todo RippleDecorator que este
  //     instancer cria, o que acontece pela mesma regra de lifetime que bootstrap.cpp documenta
  //     para polygon_instancer/tint_instancer (o próprio instancer sobrevive ao Rml::Shutdown()).
  void Initialise(float max_radius, int* active_instance_counter);

  Rml::DecoratorDataHandle GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const override;
  void ReleaseElementData(Rml::DecoratorDataHandle element_data) const override;
  void RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const override;

private:
  float max_radius_ = 0.f;
  int* active_instance_counter_ = nullptr;
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

  // EN: L1.22-CAPTURE gate accessor -- returns a pointer to the SHARED active-instance counter
  //     (see the class-level doc comment above), wired into Gl3RenderInterface by bootstrap.cpp
  //     right after this instancer is constructed+registered
  //     (RenderGl3::set_ripple_active_counter). The pointee is `active_instances_` below, which
  //     outlives every RippleDecorator this instancer creates (this instancer itself outlives
  //     Rml::Shutdown(), same lifetime rule as polygon_instancer/tint_instancer).
  // PT: Acessor do gate do L1.22-CAPTURE -- retorna um ponteiro para o contador de instância
  //     ativa COMPARTILHADO (ver o doc-comment de nível de classe acima), conectado a
  //     Gl3RenderInterface por bootstrap.cpp logo após este instancer ser
  //     construído+registrado (RenderGl3::set_ripple_active_counter). O apontado é
  //     `active_instances_` abaixo, que sobrevive a todo RippleDecorator que este instancer
  //     cria (este próprio instancer sobrevive ao Rml::Shutdown(), mesma regra de lifetime de
  //     polygon_instancer/tint_instancer).
  const int* active_instance_count_ptr() const { return &active_instances_; }

private:
  Rml::PropertyId max_radius_id_{};
  // EN: Shared, mutated from the OUTSIDE (via RippleDecorator::GenerateElementData/
  //     ReleaseElementData's non-owning `int*` back-pointer, handed out by InstanceDecorator
  //     below) across every RippleDecorator this instancer creates -- see the class-level doc
  //     comment for why it must be per-INSTANCER, not per-decorator. Not `mutable`: nothing
  //     mutates it through a `const RippleDecoratorInstancer&`; InstanceDecorator (non-const)
  //     takes its address, and active_instance_count_ptr() (const) merely reads/returns that
  //     same address (a `const int*` falls out naturally from a const member function without
  //     needing the member itself to be `mutable`).
  // PT: Compartilhado, mutado de FORA (via o contra-ponteiro `int*` não-dono de
  //     RippleDecorator::GenerateElementData/ReleaseElementData, entregue por InstanceDecorator
  //     abaixo) entre todo RippleDecorator que este instancer cria -- ver o doc-comment de nível
  //     de classe pro motivo de precisar ser por-INSTANCER, não por-decorator. Não é `mutable`:
  //     nada o muta através de uma `const RippleDecoratorInstancer&`; InstanceDecorator
  //     (não-const) toma seu endereço, e active_instance_count_ptr() (const) só lê/retorna esse
  //     mesmo endereço (um `const int*` sai naturalmente de uma função-membro const sem precisar
  //     que o próprio membro seja `mutable`).
  int active_instances_ = 0;
};

} // namespace glintfx
