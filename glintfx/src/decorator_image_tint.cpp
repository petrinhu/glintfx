// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of ImageTintDecorator / ImageTintDecoratorInstancer -- see
//     decorator_image_tint.hpp for the design rationale and ADR-0010 for the full derivation.
//
//     Per-element data (ImageTintElementData, below) carries TWO independent GPU resources built
//     from the SAME box-corner quad geometry:
//       (1) an Rml::Geometry (RmlUi-owned, RAII) -- used for the mode=None/vanilla path
//           (Geometry::Render(translation, texture)) AND, unavoidably, also passed to
//           Geometry::Render(translation, texture, shader) for every TINTED draw too: RmlUi's
//           own RenderManager::Render() asserts on a valid Geometry and always resolves/compiles
//           its opaque Rml::CompiledGeometryHandle before calling into RenderShader -- see
//           ADR-0010's "Investigation summary" for why that handle is legally undereferenceable
//           from this translation unit. Gl3RenderInterface::RenderShader (render_gl3.cpp) is
//           written to NEVER touch that handle for a tint draw; it is accepted, resolved by
//           RmlUi (a real but wasted GL VAO compile the very first time a tinted element
//           renders), and then ignored -- see ADR-0010 Decision (a)/Consequences for why this
//           small waste is the accepted cost of staying inside RmlUi's public API surface.
//       (2) a raw GL VAO/VBO/EBO (glintfx-owned, gl3w calls, released by hand in
//           ReleaseElementData) -- the ONLY geometry the "glintfx-tint" shader actually draws
//           from, via Gl3RenderInterface::RenderShader's tint branch.
//     Both resources are rebuilt only when GenerateElementData re-runs (source `src` argument
//     change / box resize) -- NOT on every RenderElement call. The tint colour/mode/threshold
//     (and CSS opacity), in sharp contrast, are read FRESH every RenderElement call (see
//     ResolveTintState below) -- they are ordinary properties, not decorator arguments, so RmlUi
//     does not re-run GenerateElementData when only they change; baking them once here would go
//     visually stale under `transition:`/`@keyframes` (ADR-0010 Decision (c)).
//
// PT: Implementação de ImageTintDecorator / ImageTintDecoratorInstancer -- ver
//     decorator_image_tint.hpp para a racional de design e o ADR-0010 para a derivação completa.
//
//     O dado por-elemento (ImageTintElementData, abaixo) carrega DOIS recursos de GPU
//     independentes construídos a partir da MESMA geometria de quad de cantos de caixa:
//       (1) um Rml::Geometry (dono RmlUi, RAII) -- usado no caminho mode=None/vanilla
//           (Geometry::Render(translation, texture)) E, inevitavelmente, também passado a
//           Geometry::Render(translation, texture, shader) pra TODO draw TINGIDO também: o
//           próprio RenderManager::Render() do RmlUi faz assert numa Geometry válida e sempre
//           resolve/compila seu Rml::CompiledGeometryHandle opaco antes de chamar RenderShader --
//           ver o "Resumo da investigação" do ADR-0010 pro motivo desse handle ser
//           legalmente indesreferenciável a partir desta translation unit.
//           Gl3RenderInterface::RenderShader (render_gl3.cpp) é escrito pra NUNCA tocar esse
//           handle num draw de tint; ele é aceito, resolvido pelo RmlUi (uma compilação de VAO GL
//           real mas desperdiçada na primeira vez que um elemento tingido renderiza), e então
//           ignorado -- ver a Decisão (a)/Consequências do ADR-0010 pra por que esse pequeno
//           desperdício é o custo aceito de permanecer dentro da superfície pública do RmlUi.
//       (2) uma VAO/VBO/EBO GL crua (dono glintfx, chamadas gl3w, liberada à mão em
//           ReleaseElementData) -- a ÚNICA geometria de onde o shader "glintfx-tint" de fato
//           desenha, via o ramo de tint de Gl3RenderInterface::RenderShader.
//     Os dois recursos só são reconstruídos quando GenerateElementData roda de novo (mudança do
//     argumento `src` / redimensionamento da caixa) -- NÃO a cada chamada de RenderElement. A
//     cor/modo/threshold de tingimento (e a opacity do CSS), em contraste nítido, são lidas
//     FRESCAS a cada chamada de RenderElement (ver ResolveTintState abaixo) -- são propriedades
//     comuns, não argumentos de decorator, então o RmlUi não re-roda GenerateElementData quando só
//     elas mudam; embuti-las aqui uma vez ficaria visualmente obsoleto sob `transition:`/
//     `@keyframes` (Decisão (c) do ADR-0010).
// Copyright (c) 2026 Petrus Silva Costa

// EN: gl3w FIRST -- defines all GL 3.x function pointers loaded at runtime (same repo-wide
//     convention as render_gl3.cpp; this is the first *decorator* file to need it, since it owns
//     a raw VAO/VBO/EBO -- see the file header above and ADR-0010).
// PT: gl3w PRIMEIRO -- define todos os ponteiros de função GL 3.x carregados em tempo de execução
//     (mesma convenção do repo inteiro que render_gl3.cpp; este é o primeiro arquivo de
//     *decorator* a precisar dele, já que é dono de uma VAO/VBO/EBO crua -- ver o cabeçalho do
//     arquivo acima e o ADR-0010).
#include <GL/gl3w.h>

#include "decorator_image_tint.hpp"

#include <RmlUi/Core/CompiledFilterShader.h>  // EN: complete Rml::CompiledShader type. PT: tipo Rml::CompiledShader completo.
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/StyleSheetSpecification.h>

#include <algorithm>  // EN: std::clamp. PT: std::clamp.
#include <cmath>      // EN: std::isfinite. PT: std::isfinite.
#include <cstddef>    // EN: offsetof. PT: offsetof.
#include <utility>    // EN: std::move. PT: std::move.

namespace glintfx {

namespace {

// EN: Per-element decorator data -- see the file header above for the full rationale behind
//     carrying BOTH an Rml::Geometry (RAII, RmlUi-owned) and a raw VAO/VBO/EBO (glintfx-owned).
// PT: Dado de decorator por-elemento -- ver o cabeçalho do arquivo acima para a racional completa
//     de carregar TANTO um Rml::Geometry (RAII, dono RmlUi) QUANTO uma VAO/VBO/EBO crua (dono
//     glintfx).
struct ImageTintElementData {
  Rml::Geometry geometry;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int index_count = 0;
};

// EN: Resolved, hardened tint state for one RenderElement call -- see ResolveTintState below.
// PT: Estado de tingimento resolvido e protegido para uma chamada de RenderElement -- ver
//     ResolveTintState abaixo.
struct TintState {
  ImageTintMode mode = ImageTintMode::None;
  Rml::Colourf color{1.f, 1.f, 1.f, 1.f};  // EN: straight (non-premultiplied). PT: straight (não-premultiplicada).
  float threshold = 0.55f;
};

// EN: Reads the three global "image-tint-*" properties directly off `element` and hardens each
//     one -- called FRESH every RenderElement call, never cached (see the file header + ADR-0010
//     Decision (c)). Hardening specifics (líder's explicit ask, mirrors decorator_polygon.cpp's
//     established philosophy: "the RCSS parser is a transport, not a gate"):
//       -mode: the "keyword" parser already gates invalid values at RCSS-parse time (an unknown
//         keyword never reaches C++ -- the property falls back to its own default/inherited
//         value automatically). A defense-in-depth `default:` case is kept anyway below, in case
//         a caller ever constructs/sets a raw Rml::Property with an out-of-range int directly
//         (bypassing the parser) -- same "auditoria dominó" spirit as decorator_polygon.cpp's own
//         belt-and-suspenders switch.
//       -color: any resolved Rml::Colourb is valid -- no extra clamp needed.
//       -threshold: `!isfinite` (NaN/Inf, e.g. from a broken animation) falls back to the
//         property's own documented default (0.55) FIRST, then the (now guaranteed-finite) value
//         is clamped to [0.0, 0.999] -- the upper bound is STRICTLY below 1.0, not 1.0 itself: the
//         fragment shader (render_gl3.cpp) feeds this value straight into
//         `smoothstep(_threshold, 1.0, L)`, and GLSL's smoothstep is explicitly UNDEFINED BEHAVIOUR
//         when edge0 >= edge1 (the spec's own wording) -- an author-supplied `threshold: 1.0` (or
//         anything above 1, which a [0,1] clamp would previously flatten TO exactly 1.0) used to
//         produce edge0==edge1==1.0 on every such element. Mesa/llvmpipe (this repo's CI) happens
//         to tolerate it, but real GPU drivers (NVIDIA/AMD/Intel) are free to return NaN, which
//         `mix()` then propagates into the final colour -- a real flicker/corruption bug on
//         hardware this CI cannot see. NOTE this is still a *clamp*, not a *reject-the-whole-
//         decorator*, deliberately DIFFERENT from decorator_polygon.cpp's `sides` guard (which
//         rejects): an out-of-range threshold is not destructive/DoS-shaped the way a
//         billion-sided polygon is, so degrading gracefully (clamp) rather than discarding the
//         whole decorator is the right shape here.
// PT: Lê as três propriedades globais "image-tint-*" diretamente de `element` e protege cada uma
//     -- chamada FRESCA a cada chamada de RenderElement, nunca cacheada (ver o cabeçalho do
//     arquivo + a Decisão (c) do ADR-0010). Especificidades de hardening (pedido explícito do
//     líder, espelha a filosofia já estabelecida de decorator_polygon.cpp: "o parser RCSS é um
//     transporte, não um portão"):
//       -mode: o parser "keyword" já protege valores inválidos no momento do parse RCSS (uma
//         keyword desconhecida nunca chega ao C++ -- a propriedade cai de volta ao seu próprio
//         valor default/herdado automaticamente). Um caso `default:` de defesa-em-profundidade é
//         mantido mesmo assim abaixo, caso algum chamador algum dia construa/sete um
//         Rml::Property cru com um int fora de faixa diretamente (contornando o parser) -- mesmo
//         espírito de "auditoria dominó" do próprio switch cinto-e-suspensório de
//         decorator_polygon.cpp.
//       -color: qualquer Rml::Colourb resolvido é válido -- sem clamp extra necessário.
//       -threshold: `!isfinite` (NaN/Inf, ex.: de uma animação quebrada) cai de volta ao default
//         documentado da própria propriedade (0.55) PRIMEIRO, depois o valor (agora
//         garantidamente finito) é clampado a [0.0, 0.999] -- o limite superior é ESTRITAMENTE
//         abaixo de 1.0, não 1.0 em si: o shader de fragmento (render_gl3.cpp) alimenta esse valor
//         direto em `smoothstep(_threshold, 1.0, L)`, e o smoothstep do GLSL é explicitamente
//         COMPORTAMENTO INDEFINIDO quando edge0 >= edge1 (a própria redação da spec) -- um
//         `threshold: 1.0` fornecido pelo autor (ou qualquer valor acima de 1, que um clamp [0,1]
//         antes achatava PRA exatamente 1.0) produzia edge0==edge1==1.0 em todo elemento assim.
//         Mesa/llvmpipe (o CI deste repo) por acaso tolera, mas drivers de GPU reais
//         (NVIDIA/AMD/Intel) são livres para devolver NaN, que `mix()` então propaga pra cor
//         final -- um bug real de flicker/corrupção em hardware que este CI não enxerga. NOTE que
//         isso continua sendo um *clamp*, não uma *rejeição do decorator inteiro*, deliberadamente
//         DIFERENTE da guarda `sides` de decorator_polygon.cpp (que rejeita): um threshold fora de
//         faixa não é destrutivo/formato-de-DoS do jeito que um polígono de um bilhão de lados é,
//         então degradar graciosamente (clamp) em vez de descartar o decorator inteiro é a forma
//         certa aqui.
void ResolveTintState(Rml::Element* element, TintState& out) {
  if (const Rml::Property* p_mode = element->GetProperty("image-tint-mode")) {
    switch (p_mode->Get<int>()) {
      case static_cast<int>(ImageTintMode::None): out.mode = ImageTintMode::None; break;
      case static_cast<int>(ImageTintMode::Multiply): out.mode = ImageTintMode::Multiply; break;
      case static_cast<int>(ImageTintMode::LuminanceMultiply): out.mode = ImageTintMode::LuminanceMultiply; break;
      case static_cast<int>(ImageTintMode::Screen): out.mode = ImageTintMode::Screen; break;
      default: out.mode = ImageTintMode::None; break;  // Defense-in-depth -- see doc comment above.
    }
  }

  if (const Rml::Property* p_color = element->GetProperty("image-tint-color")) {
    const Rml::Colourb c = p_color->Get<Rml::Colourb>();
    out.color = Rml::Colourf(c.red / 255.f, c.green / 255.f, c.blue / 255.f, c.alpha / 255.f);
  }

  if (const Rml::Property* p_threshold = element->GetProperty("image-tint-threshold")) {
    float t = p_threshold->Get<float>();
    if (!std::isfinite(t))
      t = 0.55f;
    // EN: Upper bound is 0.999f, NOT 1.f -- see the doc comment above ("-threshold:") for why
    //     edge0==edge1==1.0 in the shader's smoothstep() is GLSL UB, not just a visual edge case.
    // PT: Limite superior é 0.999f, NÃO 1.f -- ver o doc-comment acima ("-threshold:") pra por que
    //     edge0==edge1==1.0 no smoothstep() do shader é UB do GLSL, não só um edge case visual.
    out.threshold = std::clamp(t, 0.f, 0.999f);
  }
}

// EN: Single vertex layout shared by BOTH the Rml::Mesh (vanilla path) and the raw VAO (tint
//     path) below -- built ONCE per GenerateElementData call, see BuildQuadCorners.
// PT: Layout de vértice único compartilhado TANTO pelo Rml::Mesh (caminho vanilla) QUANTO pela
//     VAO crua (caminho de tint) abaixo -- construído UMA VEZ por chamada de GenerateElementData,
//     ver BuildQuadCorners.
struct QuadCorners {
  Rml::Vector2f position[4];   // EN: border-box-relative, CW from top-left. PT: relativo à border-box, CW a partir do topo-esquerda.
  Rml::Vector2f tex_coord[4];  // EN: (0,0)..(1,1), same winding. PT: (0,0)..(1,1), mesmo giro.
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

void ImageTintDecorator::Initialise(Rml::Texture texture) {
  texture_ = std::move(texture);
}

Rml::DecoratorDataHandle ImageTintDecorator::GenerateElementData(Rml::Element* element, Rml::BoxArea paint_area) const {
  Rml::RenderManager* render_manager = element->GetRenderManager();
  if (!render_manager)
    return INVALID_DECORATORDATAHANDLE;

  const Rml::RenderBox render_box = element->GetRenderBox(paint_area);
  const Rml::Vector2f offset = render_box.GetFillOffset();
  const Rml::Vector2f size = render_box.GetFillSize();
  if (size.x <= 0.f || size.y <= 0.f)
    return INVALID_DECORATORDATAHANDLE;

  const QuadCorners quad = BuildQuadCorners(offset, size);

  // EN: Vanilla-path mesh -- vertex colour is opaque-white premultiplied by the element's CURRENT
  //     opacity, same convention decorator_polygon.cpp uses for its own solid-fill vertices --
  //     this is what makes the mode=None path visually IDENTICAL to the native "image" decorator
  //     (whose own DecoratorTiledImage::GenerateElementData feeds the same opacity-premultiplied
  //     white through Tile::GenerateGeometry). NOTE: this vertex colour is only ever consumed by
  //     the vanilla Geometry::Render(translation, texture) path (RmlUi's own RenderGeometry
  //     multiplies texel-by-vertex-colour) -- the "glintfx-tint" fragment shader (render_gl3.cpp)
  //     ignores vertex colour entirely (opacity is re-read fresh as a uniform there instead, see
  //     RenderElement below) -- it is still baked into the Rml::Mesh here because RmlUi's own
  //     Vertex struct requires one, and because that same Rml::Geometry is unavoidably also
  //     passed through for the TINTED draw path too (see the file header comment above).
  // PT: Mesh do caminho vanilla -- a cor de vértice é branco-opaco premultiplicado pela opacity
  //     ATUAL do elemento, mesma convenção que decorator_polygon.cpp usa pros próprios vértices
  //     de preenchimento sólido -- é isso que torna o caminho mode=None visualmente IDÊNTICO ao
  //     decorator "image" nativo (cujo próprio DecoratorTiledImage::GenerateElementData alimenta
  //     o mesmo branco premultiplicado por opacidade através de Tile::GenerateGeometry). NOTE:
  //     esta cor de vértice só é consumida pelo caminho vanilla Geometry::Render(translation,
  //     texture) (o RenderGeometry do próprio RmlUi multiplica texel-por-cor-de-vértice) -- o
  //     shader de fragmento "glintfx-tint" (render_gl3.cpp) ignora cor de vértice inteiramente (a
  //     opacity é relida fresca como uniform lá, ver RenderElement abaixo) -- ainda assim é
  //     embutida no Rml::Mesh aqui porque a própria struct Vertex do RmlUi exige uma, e porque
  //     esse mesmo Rml::Geometry é inevitavelmente também repassado pro caminho de draw TINGIDO
  //     também (ver o comentário de cabeçalho do arquivo acima).
  const float opacity = element->GetComputedValues().opacity();
  const Rml::ColourbPremultiplied vertex_colour = Rml::Colourb(255, 255, 255, 255).ToPremultiplied(opacity);

  Rml::Mesh mesh;
  mesh.vertices.reserve(4);
  mesh.indices.reserve(6);
  for (int i = 0; i < 4; ++i)
    mesh.vertices.push_back(Rml::Vertex{quad.position[i], vertex_colour, quad.tex_coord[i]});
  mesh.indices = {0, 1, 2, 0, 2, 3};

  auto* data = new ImageTintElementData{render_manager->MakeGeometry(std::move(mesh)), 0, 0, 0, 0};

  // EN: Raw VAO/VBO/EBO -- the geometry the "glintfx-tint" fragment shader actually draws from
  //     (Gl3RenderInterface::RenderShader's tint branch, render_gl3.cpp) -- 2 attributes only
  //     (position @location=0, tex_coord @location=1), no vertex colour attribute at all (tint/
  //     opacity are shader uniforms, re-read fresh every RenderElement call -- see ADR-0010
  //     Decision (c)). Layout(location=N) qualifiers in the GLSL (render_gl3.cpp) mean no
  //     glBindAttribLocation/glGetAttribLocation dance is needed here.
  // PT: VAO/VBO/EBO crua -- a geometria de onde o shader de fragmento "glintfx-tint" de fato
  //     desenha (ramo de tint de Gl3RenderInterface::RenderShader, render_gl3.cpp) -- só 2
  //     atributos (posição @location=0, tex_coord @location=1), nenhum atributo de cor de vértice
  //     (tint/opacity são uniforms de shader, relidos frescos a cada chamada de RenderElement --
  //     ver a Decisão (c) do ADR-0010). Qualificadores layout(location=N) no GLSL (render_gl3.cpp)
  //     significam que nenhuma dança de glBindAttribLocation/glGetAttribLocation é necessária
  //     aqui.
  struct TintVertex {
    float x, y, u, v;
  };
  const TintVertex tv[4] = {
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
  glBufferData(GL_ARRAY_BUFFER, sizeof(tv), tv, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TintVertex), reinterpret_cast<const void*>(offsetof(TintVertex, x)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TintVertex), reinterpret_cast<const void*>(offsetof(TintVertex, u)));
  // EN: Unbind the VAO FIRST -- the GL_ELEMENT_ARRAY_BUFFER binding is part of VAO state (unlike
  //     GL_ARRAY_BUFFER, which is global GL state); unbinding the VAO here preserves `data->ebo`
  //     as ITS recorded element-buffer binding. Only then reset the (harmless, global)
  //     GL_ARRAY_BUFFER binding for hygiene.
  // PT: Desvincula a VAO PRIMEIRO -- o binding de GL_ELEMENT_ARRAY_BUFFER é parte do estado da
  //     VAO (diferente de GL_ARRAY_BUFFER, que é estado GL global); desvincular a VAO aqui
  //     preserva `data->ebo` como o binding de buffer-de-elemento REGISTRADO dela. Só depois
  //     reseta o binding (inofensivo, global) de GL_ARRAY_BUFFER por higiene.
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  data->index_count = 6;

  return reinterpret_cast<Rml::DecoratorDataHandle>(data);
}

void ImageTintDecorator::ReleaseElementData(Rml::DecoratorDataHandle element_data) const {
  auto* data = reinterpret_cast<ImageTintElementData*>(element_data);
  // EN: The VAO/VBO/EBO are NOT owned by any CompiledShader -- see render_gl3.cpp's ReleaseShader
  //     doc comment for why that ownership split matters (a double-free/use-after-free risk spot
  //     flagged explicitly in ADR-0010 Decision (a)). They are released HERE, once per element,
  //     never per-frame/per-CompileShader-call.
  // PT: A VAO/VBO/EBO NÃO são donas de nenhum CompiledShader -- ver o comentário de doc do
  //     ReleaseShader em render_gl3.cpp pra por que essa divisão de posse importa (um ponto de
  //     risco de double-free/use-after-free sinalizado explicitamente na Decisão (a) do
  //     ADR-0010). São liberadas AQUI, uma vez por elemento, nunca por-frame/por-chamada-de-
  //     CompileShader.
  if (data->vao)
    glDeleteVertexArrays(1, &data->vao);
  if (data->vbo)
    glDeleteBuffers(1, &data->vbo);
  if (data->ebo)
    glDeleteBuffers(1, &data->ebo);
  delete data;
}

void ImageTintDecorator::RenderElement(Rml::Element* element, Rml::DecoratorDataHandle element_data) const {
  auto* data = reinterpret_cast<ImageTintElementData*>(element_data);
  const Rml::Vector2f translation = element->GetAbsoluteOffset(Rml::BoxArea::Border);

  TintState state;
  ResolveTintState(element, state);

  if (state.mode == ImageTintMode::None) {
    // EN: Vanilla path -- IDENTICAL cost/behaviour to a plain "image()" decorator: no shader
    //     compiled, no custom VAO drawn, plain per-vertex-colour RenderGeometry (see ADR-0010
    //     Decision (b) and Consequences).
    // PT: Caminho vanilla -- custo/comportamento IDÊNTICO a um decorator "image()" simples:
    //     nenhum shader compilado, nenhuma VAO customizada desenhada, RenderGeometry simples
    //     por-cor-de-vértice (ver a Decisão (b) e Consequências do ADR-0010).
    data->geometry.Render(translation, texture_);
    return;
  }

  Rml::RenderManager* render_manager = element->GetRenderManager();
  if (!render_manager) {
    data->geometry.Render(translation, texture_);
    return;
  }

  // EN: Opacity read FRESH here too (not cached in GenerateElementData) -- same reasoning as
  //     ResolveTintState above (ADR-0010 Decision (c)'s "opacity staleness" risk note): CSS
  //     `opacity` is an ordinary transitionable property, so an `opacity` animation running
  //     alongside a static image-tint-mode must still composite correctly every frame.
  // PT: Opacity também lida FRESCA aqui (não cacheada em GenerateElementData) -- mesma
  //     racional de ResolveTintState acima (nota de risco "obsolescência de opacity" da Decisão
  //     (c) do ADR-0010): `opacity` do CSS é uma propriedade transicionável comum, então uma
  //     animação de `opacity` rodando junto de um image-tint-mode estático ainda precisa compor
  //     corretamente a cada frame.
  const float opacity = element->GetComputedValues().opacity();

  Rml::CompiledShader shader = render_manager->CompileShader("glintfx-tint",
      Rml::Dictionary{
          {"vao", Rml::Variant(static_cast<int>(data->vao))},
          {"index_count", Rml::Variant(data->index_count)},
          {"tint_color", Rml::Variant(state.color)},
          {"mode", Rml::Variant(static_cast<int>(state.mode))},
          {"threshold", Rml::Variant(state.threshold)},
          {"opacity", Rml::Variant(opacity)},
      });

  if (!shader) {
    // EN: Compile failure (e.g. driver rejected the GLSL) -- degrade to the vanilla textured
    //     quad rather than propagate an invalid shader forward. See render_gl3.cpp's
    //     EnsureTintProgramCompiled doc comment for when this can happen.
    // PT: Falha de compilação (ex.: driver rejeitou o GLSL) -- degrada pro quad texturizado
    //     vanilla em vez de propagar um shader inválido adiante. Ver o comentário de doc de
    //     EnsureTintProgramCompiled em render_gl3.cpp pra quando isso pode acontecer.
    data->geometry.Render(translation, texture_);
    return;
  }

  // EN: `data->geometry` is passed here ONLY to satisfy RenderManager::Render's API contract
  //     (RMLUI_ASSERT(geometry), and it resolves/compiles the opaque Rml::CompiledGeometryHandle
  //     forwarded as RenderShader's 2nd parameter) -- Gl3RenderInterface::RenderShader's tint
  //     branch (render_gl3.cpp) NEVER dereferences that handle; it draws from `data->vao`
  //     (threaded through the CompiledShader's own Dictionary parameters above) instead. See the
  //     file header comment for the full rationale.
  // PT: `data->geometry` é passado aqui SÓ pra satisfazer o contrato de API de
  //     RenderManager::Render (RMLUI_ASSERT(geometry), e ele resolve/compila o
  //     Rml::CompiledGeometryHandle opaco repassado como 2º parâmetro de RenderShader) -- o ramo
  //     de tint de Gl3RenderInterface::RenderShader (render_gl3.cpp) NUNCA desreferencia esse
  //     handle; desenha a partir de `data->vao` (repassado pelos parâmetros do próprio
  //     Dictionary do CompiledShader acima) em vez disso. Ver o comentário de cabeçalho do
  //     arquivo pra racional completa.
  data->geometry.Render(translation, texture_, shader);
}

ImageTintDecoratorInstancer::ImageTintDecoratorInstancer() {
  // EN: `src` default is "" (empty) -- an author who writes "decorator: image-tint();" (empty
  //     parentheses) supplies no positional value, so RmlUi fills it with THIS default. The
  //     passthrough "string" parser (Source/Core/PropertyParserString.cpp) accepts ANY text --
  //     including empty -- unconditionally; InstanceDecorator below is where the empty-string
  //     case is actually rejected (fail-high, mirrors decorator_polygon.cpp's own
  //     empty-parens-resolves-to-an-invalid-default trick for `sides`).
  // PT: O default de `src` é "" (vazio) -- um autor que escreve "decorator: image-tint();"
  //     (parênteses vazios) não fornece valor posicional, então o RmlUi o preenche com ESTE
  //     default. O parser passthrough "string" (Source/Core/PropertyParserString.cpp) aceita
  //     QUALQUER texto -- inclusive vazio -- incondicionalmente; o InstanceDecorator abaixo é
  //     onde o caso de string vazia é de fato rejeitado (fail-high, espelha o próprio truque de
  //     "parênteses vazios resolve pra um default inválido" de decorator_polygon.cpp para
  //     `sides`).
  src_id_ = RegisterProperty("src", "").AddParser("string").GetId();
  // EN: A SINGLE positional argument needs no comma-splitting -- ShorthandType::FallThrough
  //     (confirmed against decorator_polygon.cpp's own design note on FallThrough vs.
  //     RecursiveCommaSeparated) maps the decorator's entire inner text directly to the one
  //     registered item.
  // PT: UM ÚNICO argumento posicional não precisa de separação por vírgula --
  //     ShorthandType::FallThrough (confirmado contra a própria nota de design de
  //     decorator_polygon.cpp sobre FallThrough vs. RecursiveCommaSeparated) mapeia o texto
  //     interno inteiro do decorator diretamente pro único item registrado.
  RegisterShorthand("decorator", "src", Rml::ShorthandType::FallThrough);

  // EN: The three tint properties are GLOBAL, standalone RCSS element properties -- NOT
  //     decorator-shorthand arguments (see decorator_image_tint.hpp's class doc comment and
  //     ADR-0010 Decision (b)). Registered here, from this instancer's OWN constructor, for the
  //     exact same lifetime-timing reason PolygonDecoratorInstancer's ctor registers `sides`/
  //     `fill`/`rotation` (see bootstrap.cpp's Impl::polygon_instancer doc comment, mirrored by
  //     Impl::tint_instancer): this ctor only ever runs AFTER Rml::Initialise() has populated
  //     StyleSheetSpecification's parser/PropertyId registry (see Bootstrap::init()).
  //     Ordinal-to-enum mapping for `image-tint-mode` -- see ImageTintMode's own doc comment in
  //     decorator_image_tint.hpp for why the four names below must stay in this exact order.
  // PT: As três propriedades de tingimento são propriedades RCSS de elemento GLOBAIS, standalone
  //     -- NÃO argumentos de shorthand de decorator (ver o doc-comment da classe em
  //     decorator_image_tint.hpp e a Decisão (b) do ADR-0010). Registradas aqui, no PRÓPRIO
  //     construtor deste instancer, pelo mesmíssimo motivo de timing-de-lifetime que o ctor de
  //     PolygonDecoratorInstancer registra `sides`/`fill`/`rotation` (ver o doc-comment de
  //     Impl::polygon_instancer em bootstrap.cpp, espelhado por Impl::tint_instancer): este ctor
  //     só roda DEPOIS de Rml::Initialise() ter populado o registro de parser/PropertyId do
  //     StyleSheetSpecification (ver Bootstrap::init()).
  //     Mapeamento ordinal-para-enum de `image-tint-mode` -- ver o próprio doc-comment de
  //     ImageTintMode em decorator_image_tint.hpp pra por que os quatro nomes abaixo precisam
  //     ficar exatamente nesta ordem.
  Rml::StyleSheetSpecification::RegisterProperty("image-tint-color", "white", /*inherited=*/false).AddParser("color");
  Rml::StyleSheetSpecification::RegisterProperty("image-tint-mode", "none", /*inherited=*/false)
      .AddParser("keyword", "none, multiply, luminance-multiply, screen");
  Rml::StyleSheetSpecification::RegisterProperty("image-tint-threshold", "0.55", /*inherited=*/false).AddParser("number");
}

Rml::SharedPtr<Rml::Decorator> ImageTintDecoratorInstancer::InstanceDecorator(const Rml::String& /*name*/,
    const Rml::PropertyDictionary& properties, const Rml::DecoratorInstancerInterface& instancer_interface) {
  const Rml::Property* p_src = properties.GetProperty(src_id_);
  if (!p_src)
    return nullptr;

  const Rml::String src_raw = p_src->Get<Rml::String>();
  if (src_raw.empty()) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "decorator: image-tint() requires a texture url -- decorator ignored "
        "(e.g. an empty 'image-tint()' has no src).");
    return nullptr;
  }

  // EN: NOTE (finding, not a design flaw): DecoratorInstancerInterface::GetTexture (Decorator.cpp)
  //     resolves to a structurally-valid Texture for ANY non-empty path -- it registers the path
  //     in RmlUi's texture database (RenderManager::LoadTexture) WITHOUT touching the filesystem;
  //     the Texture is only falsy if there is no property_source at all (no document context,
  //     effectively never true for a normally-loaded RCSS rule). A missing/garbage file on disk
  //     is therefore NOT caught here -- it surfaces later, lazily, the first time this element
  //     renders (Gl3RenderInterface::LoadTexture fails to open/decode the file, the resolved
  //     Rml::TextureHandle stays 0) -- and is handled the exact same way the native "image()"
  //     decorator already handles it today (no special-casing added by this decorator): a
  //     texture handle of 0 renders untextured, no crash. The `if (!texture)` guard below is
  //     kept anyway as defense-in-depth (fail-high, mirrors polygon()'s own `sides`/fill
  //     rejection philosophy) for the rarer case it DOES catch.
  // PT: NOTE (achado, não uma falha de design): DecoratorInstancerInterface::GetTexture
  //     (Decorator.cpp) resolve pra um Texture estruturalmente válido pra QUALQUER caminho
  //     não-vazio -- ele registra o caminho no banco de dados de textura do RmlUi
  //     (RenderManager::LoadTexture) SEM tocar o sistema de arquivos; o Texture só é falso se não
  //     houver property_source nenhum (sem contexto de documento, efetivamente nunca verdadeiro
  //     pra uma regra RCSS normalmente carregada). Um arquivo ausente/lixo em disco portanto NÃO
  //     é pego aqui -- surge depois, de forma lazy, na primeira vez que este elemento renderiza
  //     (Gl3RenderInterface::LoadTexture falha ao abrir/decodificar o arquivo, o
  //     Rml::TextureHandle resolvido fica em 0) -- e é tratado exatamente do mesmo jeito que o
  //     decorator "image()" nativo já trata hoje (nenhum tratamento especial adicionado por este
  //     decorator): um texture handle de 0 renderiza sem textura, sem crash. A guarda
  //     `if (!texture)` abaixo é mantida mesmo assim como defesa-em-profundidade (fail-high,
  //     espelha a própria filosofia de rejeição de `sides`/fill do polygon()) pro caso mais raro
  //     em que ELA de fato pega algo.
  Rml::Texture texture = instancer_interface.GetTexture(src_raw);
  if (!texture) {
    Rml::Log::Message(Rml::Log::LT_WARNING,
        "decorator: image-tint() could not resolve texture '%s' -- decorator ignored.", src_raw.c_str());
    return nullptr;
  }

  auto decorator = Rml::MakeShared<ImageTintDecorator>();
  decorator->Initialise(std::move(texture));
  return decorator;
}

}  // namespace glintfx
