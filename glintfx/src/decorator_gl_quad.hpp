// SPDX-License-Identifier: MPL-2.0
// EN: Internal header (src/-only, NOT under glintfx/include/glintfx/ -- the include-based
//     encapsulation gate (tools/check_encapsulation.sh) does not cover this file) -- unifies the
//     box-corner quad geometry + raw GL VAO/VBO/EBO scaffold shared BYTE-IDENTICALLY by the two
//     custom-shader decorators (decorator_image_tint.cpp, decorator_ripple.cpp; see ADR-0010
//     Decision (a) for why a custom-shader decorator needs its own raw VAO in the first place).
//     Extracted by AUD-L1-QUALITY (docs/auditoria/AUD-L1-QUALITY.md, IMPORTANTE #2): both
//     `QuadCorners`/`BuildQuadCorners` and the create/destroy GL sequence were byte-identical
//     copies that had already drifted cosmetically (comment divergence) -- a third custom-shader
//     decorator (the natural next catalog item) would have copied a third time, and a future fix
//     (VAO leak, attrib-layout change) would have needed to land in N places. Pure refactor: the
//     GL calls below, in this exact order, produce EXACTLY the vertex/attribute/buffer state the
//     two call sites built by hand before this extraction (byte-for-byte, cross-checked against
//     the two .cpp files' pre-extraction source).
//     What is DELIBERATELY NOT here (kept local to each decorator, they legitimately diverge):
//     the vertex COLOUR baked into each decorator's own Rml::Mesh/Rml::Geometry (image-tint bakes
//     opacity-premultiplied white for its vanilla-path mesh; ripple bakes a fixed opaque white
//     purely to satisfy RenderManager::Render's API contract) -- that Rml::Mesh construction is
//     unrelated to the raw VAO this header builds and is NOT shared code between the two
//     decorators. Also NOT here: each decorator's own CompileShader/RenderElement/shader-uniform
//     logic (tint mode/threshold vs. ripple origin/phase/strength/width) -- genuinely divergent
//     per-decorator behaviour, not scaffold.
// PT: Header interno (só-src/, NÃO sob glintfx/include/glintfx/ -- o gate include-based de
//     encapsulamento (tools/check_encapsulation.sh) não cobre este arquivo) -- unifica a
//     geometria de quad de cantos de caixa + o andaime GL cru de VAO/VBO/EBO compartilhado
//     BYTE-IDENTICAMENTE pelos dois decorators de shader customizado (decorator_image_tint.cpp,
//     decorator_ripple.cpp; ver a Decisão (a) do ADR-0010 pra por que um decorator de shader
//     customizado precisa da própria VAO crua, pra começo de conversa). Extraído pela
//     AUD-L1-QUALITY (docs/auditoria/AUD-L1-QUALITY.md, IMPORTANTE #2): tanto
//     `QuadCorners`/`BuildQuadCorners` quanto a sequência GL de criar/destruir eram cópias
//     byte-idênticas que já tinham derivado cosmeticamente (divergência de comentário) -- um
//     terceiro decorator de shader customizado (o próximo item natural de catálogo) teria copiado
//     pela terceira vez, e um fix futuro (leak de VAO, mudança de layout de attrib) precisaria
//     pousar em N lugares. Refactor puro: as chamadas GL abaixo, nesta ordem exata, produzem
//     EXATAMENTE o estado de vértice/atributo/buffer que os dois sítios de chamada construíam à
//     mão antes desta extração (byte a byte, cruzado contra o fonte pré-extração dos dois .cpp).
//     O que DELIBERADAMENTE NÃO está aqui (fica local a cada decorator, divergem legitimamente):
//     a COR de vértice embutida no próprio Rml::Mesh/Rml::Geometry de cada decorator (image-tint
//     embute branco premultiplicado por opacidade pro mesh do caminho vanilla; ripple embute um
//     branco opaco fixo só pra satisfazer o contrato de API de RenderManager::Render) -- essa
//     construção de Rml::Mesh não tem relação com a VAO crua que este header constrói e NÃO é
//     código compartilhado entre os dois decorators. Também NÃO está aqui: a lógica própria de
//     cada decorator de CompileShader/RenderElement/uniform de shader (modo/threshold do tint vs.
//     origem/fase/força/largura do ripple) -- comportamento genuinamente divergente por-decorator,
//     não andaime.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

// EN: gl_loader.h FIRST -- same repo-wide convention decorator_image_tint.cpp/decorator_ripple.cpp
//     themselves follow (defines all GL 3.x function pointers loaded at runtime); harmless to
//     repeat here since both call sites already include it before this header, but this header
//     must not rely on that call-site ordering to stand on its own.
// PT: gl_loader.h PRIMEIRO -- mesma convenção repo-wide que os próprios
//     decorator_image_tint.cpp/decorator_ripple.cpp seguem (define todos os ponteiros de função
//     GL 3.x carregados em tempo de execução); inofensivo repetir aqui já que os dois sítios de
//     chamada já o incluem antes deste header, mas este header não deve depender dessa ordem do
//     sítio de chamada pra se sustentar sozinho.
#include "gl_loader.h"

#include <RmlUi/Core/Types.h>

#include <cstddef> // EN: offsetof. PT: offsetof.

namespace glintfx {

// EN: Single vertex layout shared by BOTH the Rml::Mesh (vanilla/RmlUi-owned path, where
//     applicable) and the raw VAO (custom-shader path) built from it below -- see
//     BuildQuadCorners.
// PT: Layout de vértice único compartilhado TANTO pelo Rml::Mesh (caminho vanilla/dono RmlUi,
//     onde aplicável) QUANTO pela VAO crua (caminho de shader customizado) construída a partir
//     dele abaixo -- ver BuildQuadCorners.
struct QuadCorners {
  Rml::Vector2f position[4];  // EN: border-box-relative, CW from top-left. PT: relativo à border-box, CW a partir do topo-esquerda.
  Rml::Vector2f tex_coord[4]; // EN: (0,0)..(1,1), same winding. PT: (0,0)..(1,1), mesmo giro.
};

// EN: `offset`/`size` come from Rml::RenderBox::GetFillOffset()/GetFillSize() at the caller --
//     see either decorator's GenerateElementData for the degenerate-size guard that must run
//     BEFORE this is called.
// PT: `offset`/`size` vêm de Rml::RenderBox::GetFillOffset()/GetFillSize() no chamador -- ver o
//     GenerateElementData de qualquer um dos dois decorators pra guarda de tamanho degenerado que
//     precisa rodar ANTES desta função ser chamada.
inline QuadCorners BuildQuadCorners(Rml::Vector2f offset, Rml::Vector2f size) {
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

// EN: Interleaved x/y/u/v vertex struct fed to the raw VAO below -- 2 attributes only (position
//     @location=0, tex_coord @location=1); no vertex-colour attribute at all (both decorators'
//     custom fragment shaders read tint/opacity/ripple state as UNIFORMS instead, re-resolved
//     fresh every RenderElement call -- see each decorator's own RenderElement/ResolveState
//     comment). Layout(location=N) qualifiers in the GLSL (render_gl3.cpp) mean no
//     glBindAttribLocation/glGetAttribLocation dance is needed at the call site.
// PT: Struct de vértice intercalado x/y/u/v alimentado à VAO crua abaixo -- só 2 atributos
//     (posição @location=0, tex_coord @location=1); nenhum atributo de cor de vértice (os shaders
//     de fragmento customizados dos dois decorators leem estado de tint/opacity/ripple como
//     UNIFORMS em vez disso, reresolvidos frescos a cada chamada de RenderElement -- ver o próprio
//     comentário de RenderElement/ResolveState de cada decorator). Qualificadores
//     layout(location=N) no GLSL (render_gl3.cpp) significam que nenhuma dança de
//     glBindAttribLocation/glGetAttribLocation é necessária no sítio de chamada.
struct GlQuadVertex {
  float x, y, u, v;
};

// EN: Builds, uploads and configures a VAO/VBO/EBO for one box-corner quad, in the EXACT GL call
//     order the two decorators built by hand before this extraction (byte-for-byte, see the file
//     header comment). `vao`/`vbo`/`ebo` are OUT parameters (caller owns the storage, typically
//     fields of its own per-element data struct) -- this function does not allocate/free the
//     handles' storage, only the GL objects they name. Leaves GL_ARRAY_BUFFER/VAO unbound on
//     return (see the "Unbind the VAO FIRST" note below).
// PT: Constrói, envia e configura uma VAO/VBO/EBO para um quad de cantos de caixa, na ordem EXATA
//     de chamadas GL que os dois decorators construíam à mão antes desta extração (byte a byte,
//     ver o comentário de cabeçalho do arquivo). `vao`/`vbo`/`ebo` são parâmetros OUT (o chamador
//     é dono do armazenamento, tipicamente campos da própria struct de dado por-elemento) -- esta
//     função não aloca/libera o armazenamento dos handles, só os objetos GL que eles nomeiam.
//     Deixa GL_ARRAY_BUFFER/VAO desvinculados ao retornar (ver a nota "Desvincula a VAO PRIMEIRO"
//     abaixo).
inline void CreateGlQuadBuffers(const QuadCorners& quad, GLuint& vao, GLuint& vbo, GLuint& ebo) {
  const GlQuadVertex verts[4] = {
      {quad.position[0].x, quad.position[0].y, quad.tex_coord[0].x, quad.tex_coord[0].y},
      {quad.position[1].x, quad.position[1].y, quad.tex_coord[1].x, quad.tex_coord[1].y},
      {quad.position[2].x, quad.position[2].y, quad.tex_coord[2].x, quad.tex_coord[2].y},
      {quad.position[3].x, quad.position[3].y, quad.tex_coord[3].x, quad.tex_coord[3].y},
  };
  const unsigned int idx[6] = {0, 1, 2, 0, 2, 3};

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlQuadVertex), reinterpret_cast<const void*>(offsetof(GlQuadVertex, x)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GlQuadVertex), reinterpret_cast<const void*>(offsetof(GlQuadVertex, u)));
  // EN: Unbind the VAO FIRST -- the GL_ELEMENT_ARRAY_BUFFER binding is part of VAO state (unlike
  //     GL_ARRAY_BUFFER, which is global GL state); unbinding the VAO here preserves `ebo` as ITS
  //     recorded element-buffer binding. Only then reset the (harmless, global) GL_ARRAY_BUFFER
  //     binding for hygiene.
  // PT: Desvincula a VAO PRIMEIRO -- o binding de GL_ELEMENT_ARRAY_BUFFER é parte do estado da
  //     VAO (diferente de GL_ARRAY_BUFFER, que é estado GL global); desvincular a VAO aqui
  //     preserva `ebo` como o binding de buffer-de-elemento REGISTRADO dela. Só depois reseta o
  //     binding (inofensivo, global) de GL_ARRAY_BUFFER por higiene.
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// EN: Releases a VAO/VBO/EBO trio built by CreateGlQuadBuffers above -- NOT owned by any
//     CompiledShader wrapper (render_gl3.cpp); safe to call with zero handles (no-op per handle,
//     mirrors the guarded deletes the two decorators wrote by hand before this extraction).
// PT: Libera um trio VAO/VBO/EBO construído por CreateGlQuadBuffers acima -- NÃO é dono de
//     nenhum wrapper CompiledShader (render_gl3.cpp); seguro chamar com handles zero (no-op por
//     handle, espelha os deletes protegidos que os dois decorators escreviam à mão antes desta
//     extração).
inline void DestroyGlQuadBuffers(GLuint vao, GLuint vbo, GLuint ebo) {
  if (vao)
    glDeleteVertexArrays(1, &vao);
  if (vbo)
    glDeleteBuffers(1, &vbo);
  if (ebo)
    glDeleteBuffers(1, &ebo);
}

} // namespace glintfx
