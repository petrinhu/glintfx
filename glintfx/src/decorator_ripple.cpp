// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of RippleDecorator / RippleDecoratorInstancer -- see decorator_ripple.hpp
//     for the full design rationale (mirrors decorator_image_tint.cpp/ADR-0010's precedent
//     almost line-for-line). This file only differs from decorator_image_tint.cpp in:
//       (a) no Rml::Texture at all (the "texture" sampled is the glintfx-captured backdrop,
//           bound directly inside Gl3RenderInterface::RenderShader's ripple branch);
//       (b) an optional (not required) decorator-shorthand argument (`max-radius`, default 0 ==
//           auto), instead of image-tint's required `src`.
//     NOTE: this decorator does NOT own or touch any L1.22-CAPTURE gate counter -- an earlier
//     revision (commit 647350f) did (an `active_instances_` refcount incremented/decremented
//     here), but it caused a cold-start bug (the counter was read too early, before
//     Context::Render() made it accurate) -- see render_gl3.cpp's ArmBackdropCapture/
//     EnsureBackdropCaptured doc comment and decorator_ripple.hpp's own doc comment for the fix
//     and the full derivation.
// PT: Implementação de RippleDecorator / RippleDecoratorInstancer -- ver decorator_ripple.hpp
//     para a racional de design completa (espelha o precedente de
//     decorator_image_tint.cpp/ADR-0010 quase linha a linha). Este arquivo só difere de
//     decorator_image_tint.cpp em:
//       (a) nenhum Rml::Texture (a "textura" amostrada é o backdrop capturado pela glintfx,
//           vinculado diretamente dentro do ramo de ripple de Gl3RenderInterface::RenderShader);
//       (b) um argumento de shorthand de decorator OPCIONAL (não obrigatório) (`max-radius`,
//           default 0 == auto), em vez do `src` obrigatório do image-tint.
//     NOTE: este decorator NÃO é dono nem toca nenhum contador de gate do L1.22-CAPTURE -- uma
//     revisão anterior (commit 647350f) era (um refcount `active_instances_` incrementado/
//     decrementado aqui), mas causava um bug de cold-start (o contador era lido cedo demais,
//     antes de Context::Render() torná-lo preciso) -- ver o doc-comment de
//     ArmBackdropCapture/EnsureBackdropCaptured de render_gl3.cpp e o próprio doc-comment de
//     decorator_ripple.hpp pro fix e a derivação completa.
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl_loader.h FIRST -- defines all GL 3.x function pointers loaded at runtime (same
//     repo-wide convention as decorator_image_tint.cpp; this decorator also owns a raw
//     VAO/VBO/EBO -- see decorator_ripple.hpp and ADR-0010's precedent for image-tint).
// PT: gl_loader.h PRIMEIRO -- define todos os ponteiros de função GL 3.x carregados em tempo de
//     execução (mesma convenção repo-wide de decorator_image_tint.cpp; este decorator também é
//     dono de uma VAO/VBO/EBO crua -- ver decorator_ripple.hpp e o precedente do ADR-0010 para o
//     image-tint).
#include "gl_loader.h"

#include "decorator_ripple.hpp"

#include <RmlUi/Core/CompiledFilterShader.h>  // EN: complete Rml::CompiledShader type. PT: tipo Rml::CompiledShader completo.
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <RmlUi/Core/Texture.h>

#include <algorithm>  // EN: std::max. PT: std::max.
#include <cmath>      // EN: std::isfinite. PT: std::isfinite.
#include <cstddef>    // EN: offsetof. PT: offsetof.
#include <utility>    // EN: std::move. PT: std::move.

namespace glintfx {

namespace {

// EN: Per-element decorator data -- ONLY a raw VAO/VBO/EBO (glintfx-owned) plus the
//     Rml::Geometry required purely to satisfy RenderManager::Render's API contract (see
//     decorator_ripple.hpp's class-level doc comment) -- UNLIKE ImageTintElementData, there is
//     no vanilla/untinted rendering mode here to fall back to, so this struct carries no extra
//     "mode=None" bookkeeping.
// PT: Dado de decorator por-elemento -- SÓ uma VAO/VBO/EBO crua (dona glintfx) mais o
//     Rml::Geometry exigido só para satisfazer o contrato de API de RenderManager::Render (ver o
//     doc-comment de nível de classe de decorator_ripple.hpp) -- DIFERENTE de
//     ImageTintElementData, não há modo de renderização vanilla/sem-tingimento para cair de
//     volta aqui, então esta struct não carrega nenhuma contabilidade extra de "mode=None".
struct RippleElementData {
  Rml::Geometry geometry;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int index_count = 0;
};

// EN: Resolved, hardened ripple state for one RenderElement call -- read FRESH every call, same
//     discipline as decorator_image_tint.cpp's TintState/ResolveTintState (so `transition`/
//     `@keyframes` animating `ripple-phase` never goes visually stale). Defaults mirror the
//     property defaults registered in RippleDecoratorInstancer's ctor below.
// PT: Estado de ripple resolvido e protegido para uma chamada de RenderElement -- lido FRESCO a
//     cada chamada, mesma disciplina do TintState/ResolveTintState de decorator_image_tint.cpp
//     (para que `transition`/`@keyframes` animando `ripple-phase` nunca fique visualmente
//     obsoleto). Os defaults espelham os defaults de propriedade registrados no ctor de
//     RippleDecoratorInstancer abaixo.
struct RippleState {
  float origin_x = 0.f;
  float origin_y = 0.f;
  float phase = 0.f;
  float strength = 0.f;
  float width = 48.f;
};

// EN: Reads one "ripple-*" number property off `element`, hardened with the same
//     `!isfinite -> fall back to the documented default` idiom decorator_image_tint.cpp's
//     ResolveTintState uses for `image-tint-threshold` (NaN/Inf from a broken animation must
//     never reach the GLSL uniforms below).
// PT: Lê uma propriedade número "ripple-*" de `element`, protegida com o mesmo idioma
//     `!isfinite -> cai de volta ao default documentado` que ResolveTintState de
//     decorator_image_tint.cpp usa para `image-tint-threshold` (NaN/Inf de uma animação quebrada
//     nunca deve alcançar os uniforms GLSL abaixo).
float ReadRippleNumber(Rml::Element* element, const char* property_name, float fallback) {
  if (const Rml::Property* p = element->GetProperty(property_name)) {
    const float v = p->Get<float>();
    if (std::isfinite(v))
      return v;
  }
  return fallback;
}

void ResolveRippleState(Rml::Element* element, RippleState& out) {
  out.origin_x = ReadRippleNumber(element, "ripple-origin-x", out.origin_x);
  out.origin_y = ReadRippleNumber(element, "ripple-origin-y", out.origin_y);
  out.phase = ReadRippleNumber(element, "ripple-phase", out.phase);
  out.strength = ReadRippleNumber(element, "ripple-strength", out.strength);
  // EN: `width` is additionally floored to a small positive epsilon (NOT just finite) -- see the
  //     "auditoria dominó" note in render_gl3.cpp's kGlintfxRippleFragmentShaderSrc for the
  //     GLSL-UB class of bug (smoothstep(edge0,edge1,x) with edge0>=edge1) this specific class of
  //     value feeds; the CPU-side floor here is belt-and-suspenders on top of that shader-side
  //     rewrite, exactly the same "clamp, don't reject" philosophy ResolveTintState documents
  //     for `image-tint-threshold`.
  // PT: `width` é adicionalmente clampado por baixo a um epsilon positivo pequeno (NÃO só
  //     finito) -- ver a nota de "auditoria dominó" em kGlintfxRippleFragmentShaderSrc de
  //     render_gl3.cpp pra classe de bug de UB do GLSL (smoothstep(edge0,edge1,x) com
  //     edge0>=edge1) que este valor especificamente alimenta; o floor do lado da CPU aqui é
  //     cinto-e-suspensório em cima daquela reescrita do lado do shader, exatamente a mesma
  //     filosofia "clampa, não rejeita" que ResolveTintState documenta pra
  //     `image-tint-threshold`.
  out.width = std::max(ReadRippleNumber(element, "ripple-width", out.width), 0.0001f);
}

// EN: Same box-corner-quad vertex layout as decorator_image_tint.cpp's own QuadCorners/
//     BuildQuadCorners -- kept as an independent copy (not shared) since these are two
//     independent, single-purpose translation units and the geometry math is a handful of
//     lines; see decorator_image_tint.cpp for the identical reference implementation.
// PT: Mesmo layout de quad de cantos de caixa do próprio QuadCorners/BuildQuadCorners de
//     decorator_image_tint.cpp -- mantido como cópia independente (não compartilhada) já que
//     estas são duas translation units independentes de propósito único e a matemática de
//     geometria é um punhado de linhas; ver decorator_image_tint.cpp pra implementação de
//     referência idêntica.
struct QuadCorners {
  Rml::Vector2f position[4];
  Rml::Vector2f tex_coord[4];
};

QuadCorners BuildQuadCorners(Rml::Vector2f offset, Rml::Vector2f size) {
  QuadCorners q;
  q.position[0] = offset;
  q.position[1] = offset + Rml::Vector2f(size.x, 0.f);
  q.position[2] = offset + size;
  q.position[3] = offset + Rml::Vector2f(0.f, size.y);
  q.tex_coord[0] = Rml::Vector2f(0.f, 0.f);
  q.tex_coord[1] = Rml::Vector2f(1.f, 0.f);
  q.tex_coord[2] = Rml::Vector2f(1.f, 1.f);
  q.tex_coord[3] = Rml::Vector2f(0.f, 1.f);
  return q;
}

}  // namespace

void RippleDecorator::Initialise(float max_radius) {
  max_radius_ = max_radius;
}

Rml::DecoratorDataHandle RippleDecorator::GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const {
  Rml::RenderManager* render_manager = element->GetRenderManager();
  if (!render_manager)
    return INVALID_DECORATORDATAHANDLE;

  const Rml::RenderBox render_box = element->GetRenderBox(paint_area);
  const Rml::Vector2f offset = render_box.GetFillOffset();
  const Rml::Vector2f size = render_box.GetFillSize();
  if (size.x <= 0.f || size.y <= 0.f)
    return INVALID_DECORATORDATAHANDLE;

  const QuadCorners quad = BuildQuadCorners(offset, size);

  // EN: Vertex colour is unused by the ripple draw path (RenderShader's ripple branch,
  //     render_gl3.cpp, ignores it entirely -- there is no vertex-colour attribute in the raw
  //     VAO below at all) -- opaque white is filled purely because Rml::Vertex requires SOME
  //     colour value to construct the (otherwise-unused) Rml::Geometry passed through to satisfy
  //     RenderManager::Render's API contract, same rationale as decorator_image_tint.cpp's own
  //     mesh vertex colour.
  // PT: Cor de vértice não é usada pelo caminho de draw de ripple (o ramo de ripple de
  //     RenderShader, render_gl3.cpp, a ignora inteiramente -- não há atributo de cor de vértice
  //     nenhum na VAO crua abaixo) -- branco opaco é preenchido só porque Rml::Vertex exige
  //     ALGUM valor de cor pra construir o Rml::Geometry (de outra forma não usado) repassado
  //     pra satisfazer o contrato de API de RenderManager::Render, mesma racional da própria cor
  //     de vértice de mesh de decorator_image_tint.cpp.
  Rml::Mesh mesh;
  mesh.vertices.reserve(4);
  mesh.indices.reserve(6);
  const Rml::ColourbPremultiplied opaque_white = Rml::Colourb(255, 255, 255, 255).ToPremultiplied(1.f);
  for (int i = 0; i < 4; ++i)
    mesh.vertices.push_back(Rml::Vertex{quad.position[i], opaque_white, quad.tex_coord[i]});
  mesh.indices = {0, 1, 2, 0, 2, 3};

  auto* data = new RippleElementData{render_manager->MakeGeometry(std::move(mesh)), 0, 0, 0, 0};

  // EN: Raw VAO/VBO/EBO -- the ONLY geometry the "glintfx-ripple" fragment shader draws from
  //     (Gl3RenderInterface::RenderShader's ripple branch, render_gl3.cpp) -- 2 attributes only
  //     (position @location=0, tex_coord @location=1), identical layout to
  //     decorator_image_tint.cpp's own TintVertex (see that file for the full attribute-layout
  //     rationale, which applies verbatim here).
  // PT: VAO/VBO/EBO crua -- a ÚNICA geometria de onde o shader de fragmento "glintfx-ripple"
  //     desenha (ramo de ripple de Gl3RenderInterface::RenderShader, render_gl3.cpp) -- só 2
  //     atributos (posição @location=0, tex_coord @location=1), layout idêntico ao próprio
  //     TintVertex de decorator_image_tint.cpp (ver aquele arquivo pra racional completa de
  //     layout de atributo, que se aplica literalmente aqui).
  struct RippleVertex {
    float x, y, u, v;
  };
  const RippleVertex rv[4] = {
      {quad.position[0].x, quad.position[0].y, quad.tex_coord[0].x, quad.tex_coord[0].y},
      {quad.position[1].x, quad.position[1].y, quad.tex_coord[1].x, quad.tex_coord[1].y},
      {quad.position[2].x, quad.position[2].y, quad.tex_coord[2].x, quad.tex_coord[2].y},
      {quad.position[3].x, quad.position[3].y, quad.tex_coord[3].x, quad.tex_coord[3].y},
  };
  const unsigned int idx[6] = {0, 1, 2, 0, 2, 3};

  glGenVertexArrays(1, &data->vao);
  glGenBuffers(1, &data->vbo);
  glGenBuffers(1, &data->ebo);

  glBindVertexArray(data->vao);
  glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(rv), rv, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RippleVertex), reinterpret_cast<const void*>(offsetof(RippleVertex, x)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RippleVertex), reinterpret_cast<const void*>(offsetof(RippleVertex, u)));
  // EN: Unbind the VAO FIRST -- see decorator_image_tint.cpp's identical comment (GL_ELEMENT_
  //     ARRAY_BUFFER binding is VAO state; unbinding the VAO preserves `data->ebo` as its
  //     recorded element-buffer binding).
  // PT: Desvincula a VAO PRIMEIRO -- ver o comentário idêntico de decorator_image_tint.cpp (o
  //     binding de GL_ELEMENT_ARRAY_BUFFER é estado da VAO; desvincular a VAO preserva
  //     `data->ebo` como o binding de buffer-de-elemento registrado dela).
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  data->index_count = 6;

  return reinterpret_cast<Rml::DecoratorDataHandle>(data);
}

void RippleDecorator::ReleaseElementData(Rml::DecoratorDataHandle element_data) const {
  auto* data = reinterpret_cast<RippleElementData*>(element_data);
  // EN: See decorator_image_tint.cpp's identical ReleaseElementData comment -- the VAO/VBO/EBO
  //     are NOT owned by any CompiledShader wrapper (render_gl3.cpp), released HERE, once per
  //     element, never per-frame/per-CompileShader-call.
  // PT: Ver o comentário idêntico de ReleaseElementData de decorator_image_tint.cpp -- a
  //     VAO/VBO/EBO NÃO são donas de nenhum wrapper CompiledShader (render_gl3.cpp), liberadas
  //     AQUI, uma vez por elemento, nunca por-frame/por-chamada-de-CompileShader.
  if (data->vao)
    glDeleteVertexArrays(1, &data->vao);
  if (data->vbo)
    glDeleteBuffers(1, &data->vbo);
  if (data->ebo)
    glDeleteBuffers(1, &data->ebo);
  delete data;
}

void RippleDecorator::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const {
  auto* data = reinterpret_cast<RippleElementData*>(element_data);
  const Rml::Vector2f translation = element->GetAbsoluteOffset(Rml::BoxArea::Border);

  Rml::RenderManager* render_manager = element->GetRenderManager();
  if (!render_manager)
    return;

  RippleState state;
  ResolveRippleState(element, state);

  Rml::CompiledShader shader = render_manager->CompileShader("glintfx-ripple",
      Rml::Dictionary{
          {"vao", Rml::Variant(static_cast<int>(data->vao))},
          {"index_count", Rml::Variant(data->index_count)},
          {"origin_x", Rml::Variant(state.origin_x)},
          {"origin_y", Rml::Variant(state.origin_y)},
          {"phase", Rml::Variant(state.phase)},
          {"strength", Rml::Variant(state.strength)},
          {"width", Rml::Variant(state.width)},
          {"max_radius", Rml::Variant(max_radius_)},
      });

  if (!shader) {
    // EN: Compile failure (e.g. driver rejected the GLSL) -- UNLIKE image-tint, there is no
    //     "vanilla textured quad" to degrade to (this decorator has no source image at all): the
    //     only safe fallback is to draw NOTHING, i.e. return without calling
    //     data->geometry.Render at all. A broken ripple effect must never paint an opaque quad
    //     over the host's backdrop.
    // PT: Falha de compilação (ex.: driver rejeitou o GLSL) -- DIFERENTE do image-tint, não há
    //     "quad texturizado vanilla" pra degradar (este decorator não tem imagem de origem
    //     nenhuma): o único fallback seguro é desenhar NADA, isto é, retornar sem chamar
    //     data->geometry.Render de forma alguma. Um efeito de ripple quebrado nunca deve pintar
    //     um quad opaco sobre o backdrop do host.
    return;
  }

  // EN: `Rml::Texture()` default-constructs a falsy/invalid texture -- RenderManager::Render
  //     short-circuits its own texture-handle resolution for a falsy `texture` (RenderManager.cpp
  //     :219, `texture && texture.render_manager != this`), so `texture_handle` stays 0 and
  //     Gl3RenderInterface::RenderShader's `texture` parameter is simply unused by the ripple
  //     branch (see decorator_ripple.hpp's class-level doc comment) -- the real sampled image is
  //     the glintfx-captured backdrop, bound directly inside that branch, never through this
  //     parameter.
  // PT: `Rml::Texture()` default-constrói uma textura falsa/inválida -- RenderManager::Render
  //     faz curto-circuito na própria resolução de texture-handle para um `texture` falso
  //     (RenderManager.cpp:219, `texture && texture.render_manager != this`), então
  //     `texture_handle` fica em 0 e o parâmetro `texture` de
  //     Gl3RenderInterface::RenderShader simplesmente não é usado pelo ramo de ripple (ver o
  //     doc-comment de nível de classe de decorator_ripple.hpp) -- a imagem de fato amostrada é
  //     o backdrop capturado pela glintfx, vinculado diretamente dentro daquele ramo, nunca por
  //     este parâmetro.
  data->geometry.Render(translation, Rml::Texture(), shader);
}

RippleDecoratorInstancer::RippleDecoratorInstancer() {
  // EN: `max-radius` default is "0" -- 0.f means "auto": Gl3RenderInterface::RenderShader (the
  //     only place that knows the CURRENT capture resolution) substitutes the captured
  //     backdrop's own diagonal when it sees max_radius_arg<=0 -- see render_gl3.cpp's
  //     GlintfxRippleShaderData doc comment. A bare "decorator: ripple;" (no parens at all) and
  //     an explicit "decorator: ripple();" both resolve to this same default -- neither is
  //     rejected, UNLIKE image-tint's empty-src rejection (see decorator_ripple.hpp's
  //     RippleDecoratorInstancer doc comment for why: there is no "nothing to draw" failure mode
  //     here).
  // PT: O default de `max-radius` é "0" -- 0.f significa "auto": Gl3RenderInterface::RenderShader
  //     (o único lugar que conhece a resolução de captura ATUAL) substitui pela própria diagonal
  //     do backdrop capturado quando vê max_radius_arg<=0 -- ver o doc-comment de
  //     GlintfxRippleShaderData em render_gl3.cpp. Um "decorator: ripple;" nu (sem parênteses
  //     nenhum) e um "decorator: ripple();" explícito ambos resolvem para este mesmo default --
  //     nenhum dos dois é rejeitado, DIFERENTE da rejeição de src vazio do image-tint (ver o
  //     doc-comment de RippleDecoratorInstancer em decorator_ripple.hpp pro motivo: não há modo
  //     de falha "nada para desenhar" aqui).
  max_radius_id_ = RegisterProperty("max-radius", "0").AddParser("number").GetId();
  // EN: A SINGLE positional argument needs no comma-splitting -- ShorthandType::FallThrough,
  //     same idiom as ImageTintDecoratorInstancer's own `src` shorthand
  //     (decorator_image_tint.cpp) and PolygonDecoratorInstancer's `sides`/`fill`/`rotation`
  //     RecursiveCommaSeparated precedent for the general pattern.
  // PT: UM ÚNICO argumento posicional não precisa de separação por vírgula --
  //     ShorthandType::FallThrough, mesmo idioma do próprio shorthand `src` de
  //     ImageTintDecoratorInstancer (decorator_image_tint.cpp) e do precedente
  //     RecursiveCommaSeparated de `sides`/`fill`/`rotation` de PolygonDecoratorInstancer pro
  //     padrão geral.
  RegisterShorthand("decorator", "max-radius", Rml::ShorthandType::FallThrough);

  // EN: The five ripple properties are GLOBAL, standalone RCSS element properties -- NOT
  //     decorator-shorthand arguments (see decorator_ripple.hpp's class-level doc comment and
  //     ADR-0010 Decision (b), which this mirrors for image-tint's own three tint properties).
  //     Registered here, from this instancer's OWN constructor, for the exact same
  //     lifetime-timing reason ImageTintDecoratorInstancer's/PolygonDecoratorInstancer's ctors
  //     register their own properties (see bootstrap.cpp's Impl::polygon_instancer/tint_instancer
  //     doc comments, mirrored here by Impl::ripple_instancer): this ctor only ever runs AFTER
  //     Rml::Initialise() has populated StyleSheetSpecification's parser/PropertyId registry (see
  //     Bootstrap::init()).
  // PT: As cinco propriedades de ripple são propriedades RCSS de elemento GLOBAIS, standalone --
  //     NÃO argumentos de shorthand de decorator (ver o doc-comment de nível de classe de
  //     decorator_ripple.hpp e a Decisão (b) do ADR-0010, que isto espelha pras próprias três
  //     propriedades de tint do image-tint). Registradas aqui, no PRÓPRIO construtor deste
  //     instancer, pelo mesmíssimo motivo de timing-de-lifetime que os ctores de
  //     ImageTintDecoratorInstancer/PolygonDecoratorInstancer registram as próprias propriedades
  //     (ver os doc-comments de Impl::polygon_instancer/tint_instancer em bootstrap.cpp,
  //     espelhado aqui por Impl::ripple_instancer): este ctor só roda DEPOIS de
  //     Rml::Initialise() ter populado o registro de parser/PropertyId do
  //     StyleSheetSpecification (ver Bootstrap::init()).
  Rml::StyleSheetSpecification::RegisterProperty("ripple-origin-x", "0", /*inherited=*/false).AddParser("number");
  Rml::StyleSheetSpecification::RegisterProperty("ripple-origin-y", "0", /*inherited=*/false).AddParser("number");
  Rml::StyleSheetSpecification::RegisterProperty("ripple-phase", "0", /*inherited=*/false).AddParser("number");
  Rml::StyleSheetSpecification::RegisterProperty("ripple-strength", "0", /*inherited=*/false).AddParser("number");
  Rml::StyleSheetSpecification::RegisterProperty("ripple-width", "48", /*inherited=*/false).AddParser("number");
}

Rml::SharedPtr<Rml::Decorator> RippleDecoratorInstancer::InstanceDecorator(const Rml::String& /*name*/,
    const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& /*instancer_interface*/) {
  float max_radius = 0.f;
  if (const Rml::Property* p_max_radius = properties.GetProperty(max_radius_id_)) {
    const float v = p_max_radius->Get<float>();
    // EN: `!isfinite -> 0.f` (== "auto", the property's own documented default) FIRST, THEN
    //     floor negative values to 0.f too -- an author-supplied negative max-radius has no
    //     sane geometric meaning (a shrinking-inward ring is not what "max radius" means), and
    //     0.f already means "auto" here, so folding negatives into that same "auto" bucket is a
    //     graceful degrade rather than propagating a negative value into
    //     Gl3RenderInterface::RenderShader's `max_radius_arg<=0 -> auto` check (render_gl3.cpp)
    //     -- same "clamp, don't reject" philosophy as ResolveTintState's `image-tint-threshold`
    //     handling in decorator_image_tint.cpp.
    // PT: `!isfinite -> 0.f` (== "auto", o próprio default documentado da propriedade) PRIMEIRO,
    //     DEPOIS clampa valores negativos por baixo pra 0.f também -- um max-radius negativo
    //     fornecido pelo autor não tem significado geométrico são (um anel encolhendo pra
    //     dentro não é o que "raio máximo" significa), e 0.f já significa "auto" aqui, então
    //     dobrar negativos nesse mesmo balde "auto" é um degrade gracioso em vez de propagar um
    //     valor negativo pro check `max_radius_arg<=0 -> auto` de
    //     Gl3RenderInterface::RenderShader (render_gl3.cpp) -- mesma filosofia "clampa, não
    //     rejeita" do tratamento de `image-tint-threshold` de ResolveTintState em
    //     decorator_image_tint.cpp.
    max_radius = std::isfinite(v) ? std::max(v, 0.f) : 0.f;
  }

  auto decorator = Rml::MakeShared<RippleDecorator>();
  decorator->Initialise(max_radius);
  return decorator;
}

} // namespace glintfx
