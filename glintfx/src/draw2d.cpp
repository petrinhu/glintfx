// SPDX-License-Identifier: MPL-2.0
// EN: D2D-1B -- the GL 3.3 core execution of what `sprite_batch.hpp`'s pure batcher decides
//     (plan docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decisions D2/D7/D9). Zero
//     RmlUi dependency (grep-checked: no `RmlUi/` include anywhere in this file) -- Draw2D shares
//     nothing with the ui module beyond the private `gl_loader.h`/`image_decode.hpp` seams both
//     already depend on (ADR-0017 axis 1, decision (1a)).
//
//     Textured-quad shader (one program, compiled once in init()): the vertex stage maps
//     `sprite_batch.hpp`'s screen-px/top-left/y-down vertex positions (D5) into GL clip space via
//     a plain `uTargetSize`-driven ortho transform, flipping Y internally so the caller never
//     reasons about NDC. The fragment stage applies the D8 tint formula verbatim:
//     `out = texel_premult * vec4(tint.rgb * tint.a, tint.a)`.
//
//     D9 (the anti-GlStateGuard-bug contract): `set_gl_state_for_draw()` below is called at
//     EVERY flush and sets every piece of state this module depends on (program, VAO, blend
//     func+equation+enable, active texture unit + 2D binding, depth/cull/scissor explicitly
//     OFF) -- it assumes NOTHING survives from the previous frame or a cohabiting renderer, and
//     NOTHING here is ever restored afterwards (see draw2d.hpp's file header comment for who owns
//     restoration in App vs. embed mode).
// PT: D2D-1B -- a execução GL 3.3 core do que o batcher puro de `sprite_batch.hpp` decide (plano
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decisões D2/D7/D9). Zero
//     dependência de RmlUi (checado por grep: nenhum include `RmlUi/` neste arquivo) -- o Draw2D
//     não compartilha nada com o módulo ui além das costuras privadas `gl_loader.h`/
//     `image_decode.hpp` das quais os dois já dependem (ADR-0017 eixo 1, decisão (1a)).
//
//     Shader de quad texturizado (um programa, compilado uma vez no init()): o estágio de vértice
//     mapeia as posições de vértice em px de tela/topo-esquerda/y-pra-baixo do sprite_batch.hpp
//     (D5) pro clip space GL via uma transformada ortho simples dirigida por `uTargetSize`,
//     invertendo Y internamente pra que o chamador nunca raciocine sobre NDC. O estágio de
//     fragmento aplica a fórmula de tint D8 ao pé da letra:
//     `out = texel_premult * vec4(tint.rgb * tint.a, tint.a)`.
//
//     D9 (o contrato anti-bug-GlStateGuard): `set_gl_state_for_draw()` abaixo é chamado a CADA
//     flush e seta todo pedaço de estado de que este módulo depende (programa, VAO, blend
//     func+equation+enable, unidade de textura ativa + binding 2D, depth/cull/scissor
//     explicitamente OFF) -- não assume NADA sobrevivendo do frame anterior nem de um renderer
//     coabitante, e NADA aqui é restaurado depois (ver o comentário de cabeçalho de arquivo de
//     draw2d.hpp pra quem é dono da restauração em App vs. modo embed).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/draw2d.hpp>

#include "gl_loader.h"
#include "image_decode.hpp"
#include "layer_queue.hpp"
#include "log_dedup.hpp"
#include "primitives2d.hpp"
#include "sprite_batch.hpp"
#include "transform2d.hpp"

#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace glintfx {

namespace {

// EN: One GL 3.3 core program: position (screen px, D5) -> clip space via an ortho transform
//     driven by `uTargetSize`; uv/tint pass through to the fragment stage. `uTargetSize` is
//     re-set on every flush (D9) -- cheap (one glUniform2f), and correct even if the bracket's
//     target size differs from the previous flush's (it never does within one begin()/end() pair
//     in this slice, but nothing in this shader assumes that).
// PT: Um programa GL 3.3 core: posição (px de tela, D5) -> clip space via transformada ortho
//     dirigida por `uTargetSize`; uv/tint passam direto pro estágio de fragmento. `uTargetSize` é
//     resetado a cada flush (D9) -- barato (um glUniform2f), e correto mesmo que o tamanho-alvo
//     do bracket difira do flush anterior (nunca difere dentro de um par begin()/end() nesta
//     fatia, mas nada neste shader assume isso).
const char* kVertexSrc = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
uniform vec2 uTargetSize;
out vec2 vUV;
out vec4 vColor;
void main() {
  vec2 ndc = vec2(
      (aPos.x / uTargetSize.x) * 2.0 - 1.0,
      1.0 - (aPos.y / uTargetSize.y) * 2.0);
  gl_Position = vec4(ndc, 0.0, 1.0);
  vUV = aUV;
  vColor = aColor;
}
)";

// EN: D8 tint formula, verbatim -- `texel` is already alpha-premultiplied on upload (D7), so
//     `vColor.rgb * vColor.a` premultiplies the straight tint into the SAME space before
//     modulating, and `vColor.a` alone modulates the (already-premultiplied) alpha channel.
// PT: Fórmula de tint D8, ao pé da letra -- `texel` já está alpha-premultiplicado no upload
//     (D7), então `vColor.rgb * vColor.a` premultiplica o tint straight pro MESMO espaço antes
//     de modular, e `vColor.a` sozinho modula o canal alpha (já premultiplicado).
const char* kFragmentSrc = R"(#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTex;
out vec4 finalColor;
void main() {
  vec4 texel = texture(uTex, vUV);
  finalColor = texel * vec4(vColor.rgb * vColor.a, vColor.a);
}
)";

// EN: Mirrors src/fx/effects_gl3.cpp's CompileGlintfxStage -- same GL call order, same
//     compile-status/info-log check. Duplicated rather than shared: that helper lives in the fx
//     module's TU (glintfx_fx OBJECT library), which Draw2D must NOT depend on (ADR-0017 axis 1,
//     (1a) -- "not ui, not fx, not window").
// PT: Espelha o CompileGlintfxStage de src/fx/effects_gl3.cpp -- mesma ordem de chamada GL, mesma
//     checagem de compile-status/info-log. Duplicado em vez de compartilhado: aquele helper vive
//     na TU do módulo fx (biblioteca OBJECT glintfx_fx), da qual o Draw2D NÃO pode depender
//     (ADR-0017 eixo 1, (1a) -- "não ui, não fx, não window").
bool compile_stage(GLuint& out_shader, GLenum stage, const char* src, std::string& out_error) {
  GLuint id = glCreateShader(stage);
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);
  GLint status = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint log_len = 0;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len);
    std::vector<char> log(static_cast<std::size_t>(log_len) + 1, '\0');
    glGetShaderInfoLog(id, log_len, nullptr, log.data());
    out_error = log.data();
    glDeleteShader(id);
    return false;
  }
  out_shader = id;
  return true;
}

} // namespace

struct Draw2d::Impl {
  bool initialized = false;

  GLuint program = 0;
  GLint uniform_target_size = -1;
  GLint uniform_tex = -1;
  GLuint vao = 0;
  GLuint vbo = 0;

  // D7 -- texture registry: id (1-based, 0 == invalid sentinel, same convention as
  // Audio::SoundId) indexes this vector; generation detects a stale/reused handle.
  struct TextureSlot {
    GLuint gl_name = 0;
    int width = 0;
    int height = 0;
    std::uint32_t generation = 1;
    bool alive = false;
    // D2D-3, D29 -- computed ONCE in load_texture() (compute_content_bbox(), on the decoded
    // pixels that already exist on the CPU there, discarded right after) and cached here (16
    // bytes) -- lives and dies with this slot (D32). See texture_content_bbox() below for the
    // full D7 handle-validation chain that gates reading it through the public API.
    ContentBbox bbox{};
  };
  std::vector<TextureSlot> textures;
  std::vector<std::size_t> free_slots;

  draw2d_detail::SpriteBatch batch{4096};
  int target_w = 0;
  int target_h = 0;

  // D2D-3, D23 -- the internal 1x1 white premultiplied texture (A1 (a)): a NORMAL registry slot
  // (created in init(), id 1 -- the FIRST consumer load_texture() therefore yields id 2,
  // unobservable through the public API since ids are opaque/private), never handed out as a
  // public Texture2d. Every untextured primitive (draw_filled_rect/draw_filled_quad/draw_line/
  // draw_rect_outline) draws THROUGH the same batcher path as draw_sprite() against this id, the
  // D8 tint formula on its white premultiplied texel yielding exactly `color`. Released by the
  // SAME shutdown() texture-teardown loop as every other slot (zero diff there).
  std::uint32_t white_texture_id = 0;

  // D2D-3, D28 -- the scissor is Draw2d STATE, sticky across draws AND brackets (like the
  // camera) until set_scissor()/reset_scissor()/shutdown() change it. `scissor_active == false`
  // is the default (no clip, the v0.21.0-literal path); `scissor_rect` is only meaningful when
  // `scissor_active` is true. Read by set_gl_state_for_draw() at EVERY flush (D9's own no-
  // caching rule, unchanged) and captured into each LayerCommand's own snapshot when a draw is
  // buffered (D27).
  bool scissor_active = false;
  RectF scissor_rect{};

  // D2D-3, D27 -- draw-order state: opt-in, default OFF (a bracket that never calls set_layer()
  // never touches either field below, and every draw streams through `batch` exactly as
  // v0.21.0). `layer_armed` is bracket-scoped, reset by end() (and discarded by shutdown() mid-
  // bracket, D32); `current_layer` tags every subsequent draw_sprite()/primitive call while
  // armed (sticky WITHIN the bracket, D27). `layer_queue` buffers the post-validation, post-
  // projection commands until end()'s replay.
  bool layer_armed = false;
  int current_layer = 0;
  draw2d_detail::LayerQueue layer_queue;

  // EN: D2D-2B, Q1 (c)/D22 -- the camera is `Draw2d` STATE, sticky until set_camera()/
  //     reset_camera()/shutdown() change it. `has_camera == false` is the default,
  //     v0.20.0-literal screen-space path; `camera` itself is only meaningful when
  //     `has_camera` is true (a stored camera is valid by construction, D15 -- set_camera()
  //     never lets a non-finite/zoom<=0 value through).
  // PT: D2D-2B, Q1 (c)/D22 -- a câmera é ESTADO do `Draw2d`, sticky até set_camera()/
  //     reset_camera()/shutdown() mudarem. `has_camera == false` é o default, o caminho de
  //     espaço de tela literal da v0.20.0; `camera` em si só é significativo quando
  //     `has_camera` é true (uma câmera armazenada é válida por construção, D15 --
  //     set_camera() nunca deixa passar um valor não-finito/zoom<=0).
  bool has_camera = false;
  Camera2d camera{};

  // EN: log_dedup.hpp is ZERO-RmlUi (its own header comment) -- Draw2D has no Rml::Log to reach
  //     (unlike render_gl3.cpp/decorator_polygon.cpp), so this dedup table gates a plain
  //     std::fprintf(stderr, ...) instead. `type` is always 0 (this module has only one log
  //     "channel"; the message text itself disambiguates, same key discipline log_dedup.hpp
  //     documents).
  // PT: log_dedup.hpp é ZERO-RmlUi (comentário próprio do header) -- o Draw2D não tem um
  //     Rml::Log pra alcançar (diferente de render_gl3.cpp/decorator_polygon.cpp), então esta
  //     tabela de dedup gateia um std::fprintf(stderr, ...) puro em vez disso. `type` é sempre 0
  //     (este módulo só tem um "canal" de log; o próprio texto da mensagem desambigua, mesma
  //     disciplina de chave que log_dedup.hpp documenta).
  LogDedup log{LogDedup::kDefaultCapacity};

  void log_warn(const std::string& message) {
    std::string line;
    if (log.should_print(0, message, /*is_error=*/false, line))
      std::fprintf(stderr, "glintfx::Draw2d: %s\n", line.c_str());
  }

  bool compile_program() {
    GLuint vs = 0, fs = 0;
    std::string err;
    if (!compile_stage(vs, GL_VERTEX_SHADER, kVertexSrc, err)) {
      log_warn("init(): vertex shader compile failure: " + err);
      return false;
    }
    if (!compile_stage(fs, GL_FRAGMENT_SHADER, kFragmentSrc, err)) {
      log_warn("init(): fragment shader compile failure: " + err);
      glDeleteShader(vs);
      return false;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint link_status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
      GLint log_len = 0;
      glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
      std::vector<char> buf(static_cast<std::size_t>(log_len) + 1, '\0');
      glGetProgramInfoLog(prog, log_len, nullptr, buf.data());
      log_warn(std::string("init(): shader link failure: ") + buf.data());
      glDeleteProgram(prog);
      return false;
    }
    program = prog;
    uniform_target_size = glGetUniformLocation(program, "uTargetSize");
    uniform_tex = glGetUniformLocation(program, "uTex");
    return true;
  }

  // EN: One dynamic VAO/VBO, orphaned (re-specified via glBufferData) at every flush -- avoids a
  //     GPU stall against a buffer the driver may still be reading from a previous draw (D2). The
  //     attribute pointers are set up ONCE here, while `vbo` is bound: they resolve against the
  //     buffer OBJECT, not a specific store, so a later orphaning glBufferData call does not
  //     require re-calling glVertexAttribPointer.
  // PT: Uma VAO/VBO dinâmica só, orphaned (re-especificada via glBufferData) a cada flush -- evita
  //     um stall de GPU contra um buffer que o driver ainda pode estar lendo de um draw anterior
  //     (D2). Os attribute pointers são setados UMA VEZ aqui, enquanto `vbo` está bound: eles
  //     resolvem contra o OBJETO buffer, não um store específico, então uma chamada glBufferData
  //     de orphaning posterior não exige re-chamar glVertexAttribPointer.
  void setup_vao_vbo() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    using V = draw2d_detail::Vertex;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(V),
                          reinterpret_cast<const void*>(offsetof(V, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(V),
                          reinterpret_cast<const void*>(offsetof(V, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(V),
                          reinterpret_cast<const void*>(offsetof(V, r)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  // EN: D2D-3, D23 -- creates the internal 1x1 RGBA8 premultiplied WHITE texture through the
  //     SAME upload_gl_texture() path load_texture() itself uses, and registers it as a NORMAL
  //     registry slot (single source, D23's own literal claim: "through the same upload_gl_texture
  //     path... as a NORMAL registry slot"). Premultiplied opaque white is its own fixed point
  //     (R=R*A/255 = 255*255/255 = 255), so the raw bytes below already ARE the premultiplied
  //     form -- no decode/premultiply step needed for a 1x1 constant. Returns false on GL upload
  //     failure (fail-high: init() itself fails, D23's own "a Draw2d that cannot draw primitives
  //     it advertises does not half-initialize" rule).
  // PT: D2D-3, D23 -- cria a textura branca 1x1 RGBA8 premultiplicada interna pelo MESMO caminho
  //     upload_gl_texture() que o próprio load_texture() usa, e a registra como um slot NORMAL do
  //     registry (fonte única, a própria alegação literal do D23: "pelo mesmo caminho
  //     upload_gl_texture... como um slot NORMAL do registry"). Branco opaco premultiplicado é o
  //     próprio ponto fixo (R=R*A/255 = 255*255/255 = 255), então os bytes crus abaixo JÁ SÃO a
  //     forma premultiplicada -- nenhum passo de decode/premultiply necessário pra uma constante
  //     1x1. Devolve false em falha de upload GL (fail-high: o próprio init() falha, a regra
  //     própria do D23 "um Draw2d que não consegue desenhar as primitivas que anuncia não
  //     meio-inicializa").
  bool create_white_texture() {
    DecodedImage white;
    white.ok = true;
    white.width = 1;
    white.height = 1;
    white.rgba = {255, 255, 255, 255};
    const GLuint gl_name = upload_gl_texture(white);
    if (!gl_name) return false;

    std::size_t idx;
    if (!free_slots.empty()) {
      idx = free_slots.back();
      free_slots.pop_back();
    } else {
      idx = textures.size();
      textures.push_back(TextureSlot{});
    }
    TextureSlot& slot = textures[idx];
    slot.gl_name = gl_name;
    slot.width = 1;
    slot.height = 1;
    slot.alive = true;
    slot.bbox = ContentBbox{true, 0, 0, 1, 1}; // D29: a 1x1 opaque texture's own documented bbox.
    white_texture_id = static_cast<std::uint32_t>(idx) + 1;
    return true;
  }

  // cppcheck functionStatic: touches no Impl member, only its argument and GL global state --
  // called as impl_->upload_gl_texture(decoded), unaffected by the added `static`.
  static GLuint upload_gl_texture(const DecodedImage& decoded) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, decoded.width, decoded.height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, decoded.rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
  }

  // EN: D9 -- sets EVERY piece of state a flush depends on. Called once PER FLUSH RANGE (not
  //     once per bracket): a bracket with 3 texture changes calls this 3 times, each redundant
  //     re-assertion cheap relative to the draw call itself and deliberately NOT skipped/cached
  //     -- caching "did I already set this" is exactly the kind of assumption-about-previous-
  //     state D9 exists to forbid.
  // PT: D9 -- seta TODO pedaço de estado de que um flush depende. Chamado uma vez POR FAIXA DE
  //     FLUSH (não uma vez por bracket): um bracket com 3 trocas de textura chama isto 3 vezes,
  //     cada reafirmação redundante barata frente ao próprio draw call e deliberadamente NÃO
  //     pulada/cacheada -- cachear "já setei isto" é exatamente o tipo de suposição-sobre-
  //     estado-anterior que a D9 existe pra proibir.
  void set_gl_state_for_draw(GLuint tex_gl_name) {
    glUseProgram(program);
    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    // D2D-3, D28 -- the ONE declared behavioural diff in existing code (D31): this used to be an
    // unconditional glDisable(GL_SCISSOR_TEST). With no scissor EVER set (scissor_active stays
    // false, the default) the emitted GL calls are IDENTICAL to v0.21.0 -- still asserting
    // EVERYTHING this state depends on at every flush, per D9's own no-caching rule, unchanged.
    if (scissor_active) {
      const draw2d_detail::ScissorGl mapped =
          draw2d_detail::map_scissor_to_gl(scissor_rect, target_h);
      glEnable(GL_SCISSOR_TEST);
      glScissor(mapped.x, mapped.y, mapped.w, mapped.h);
    } else {
      glDisable(GL_SCISSOR_TEST);
    }
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_gl_name);
    if (uniform_tex >= 0) glUniform1i(uniform_tex, 0);
    if (uniform_target_size >= 0)
      glUniform2f(uniform_target_size, static_cast<float>(target_w), static_cast<float>(target_h));
  }

  // EN: Drains sprite_batch.hpp's ready queue and issues one GL draw per Flush, using
  //     `target_w`/`target_h` AS THEY STAND AT THE MOMENT OF THE CALL (Draw2d::begin() drains
  //     BEFORE updating these to the new bracket's values, so a leftover run from a
  //     nested-begin's implicit end() is drawn against the OLD target size it was generated
  //     against -- see Draw2d::begin()).
  // PT: Drena a fila pronta de sprite_batch.hpp e emite um draw GL por Flush, usando
  //     `target_w`/`target_h` COMO ESTÃO NO MOMENTO DA CHAMADA (Draw2d::begin() drena ANTES de
  //     atualizá-los pros valores do bracket novo, então uma corrida sobrando do end() implícito
  //     de um begin aninhado é desenhada contra o tamanho-alvo ANTIGO contra o qual foi gerada --
  //     ver Draw2d::begin()).
  void drain_ready() {
    std::vector<draw2d_detail::Flush> flushes = batch.take_ready();
    for (const draw2d_detail::Flush& f : flushes) {
      if (f.vertices.empty()) continue;
      const std::size_t idx = static_cast<std::size_t>(f.texture_id) - 1;
      // EN: Defensive re-check -- Draw2d::draw_sprite() already validated the handle before
      // handing texture_id to the batch, and this is a single-threaded API (same discipline as
      // Audio, audio.hpp's own threading note), so this branch is provably unreachable in
      // practice. Kept anyway: fail-high over trusting an invariant silently, never a raw GL
      // name dereferenced on a "can't happen".
      // PT: Recheque defensivo -- Draw2d::draw_sprite() já validou o handle antes de entregar
      // texture_id ao batch, e esta é uma API single-threaded (mesma disciplina do Audio, nota
      // de threading própria do audio.hpp), então este ramo é comprovadamente inalcançável na
      // prática. Mantido mesmo assim: fail-high em vez de confiar num invariante em silêncio,
      // nunca um nome GL cru desreferenciado num "não pode acontecer".
      if (f.texture_id == 0 || idx >= textures.size() || !textures[idx].alive) {
        log_warn("internal: flush referenced an unknown texture id -- skipped.");
        continue;
      }
      set_gl_state_for_draw(textures[idx].gl_name);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(f.vertices.size() * sizeof(draw2d_detail::Vertex)),
                   f.vertices.data(), GL_DYNAMIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(f.vertices.size()));
    }
  }

  // EN: D2D-3, D24 -- per-corner camera projection shared by every NEW primitive method
  //     (draw_filled_rect/draw_filled_quad/draw_line/draw_rect_outline). Deliberately NOT shared
  //     with draw_sprite(transform)'s own existing camera-path code below -- that overload's
  //     inline projection stays byte-identical (D31's zero-diff set); this is a fresh copy for
  //     the fresh call sites this slice adds. No divergence is possible either way: both this
  //     method and draw_sprite(transform) ultimately call the SAME
  //     draw2d_detail::world_to_screen() (transform2d.hpp, single source, D12).
  // PT: D2D-3, D24 -- projeção de câmera por-canto compartilhada por todo método de primitiva
  //     NOVO (draw_filled_rect/draw_filled_quad/draw_line/draw_rect_outline). Deliberadamente NÃO
  //     compartilhada com o próprio código de câmera já existente do draw_sprite(transform)
  //     abaixo -- aquele overload fica byte-idêntico (conjunto zero-diff do D31); esta é uma
  //     cópia nova pros sítios de chamada novos que esta fatia adiciona. Nenhuma divergência é
  //     possível de qualquer forma: tanto este método quanto o draw_sprite(transform) no fim
  //     chamam o MESMO draw2d_detail::world_to_screen() (transform2d.hpp, fonte única, D12).
  draw2d_detail::SpriteCorners project_corners(const draw2d_detail::SpriteCorners& local) const {
    if (!has_camera) return local;
    return draw2d_detail::SpriteCorners{
        draw2d_detail::world_to_screen(camera, target_w, target_h, local.tl),
        draw2d_detail::world_to_screen(camera, target_w, target_h, local.tr),
        draw2d_detail::world_to_screen(camera, target_w, target_h, local.br),
        draw2d_detail::world_to_screen(camera, target_w, target_h, local.bl)};
  }

  // EN: D2D-3, D24/D27 -- the shared "queue or stream" tail every primitive/sprite draw method
  //     funnels through, AFTER its own degenerate check (a degenerate rect/line/outline is a
  //     silent no-op the CALLER already returned on before ever reaching here -- mirrors
  //     SpriteBatch::draw_sprite()'s own split of "degenerate before the queueing step").
  //     `corners` are FINAL, already-projected screen-space TL,TR,BR,BL (D24's "one composition
  //     story"). Checks OutsideBracket via batch.in_bracket() (the public introspection accessor
  //     SpriteBatch already exposed -- D31: zero change to SpriteBatch needed to read it) and
  //     re-validates finiteness of corners/src_px/tint (D10-style defence in depth -- the SAME
  //     re-check SpriteBatch::draw_quad() already applies to its own caller-supplied corners,
  //     provably unreachable in practice since every call site already validated before reaching
  //     here, kept anyway per this module's own "fail-high over trusting an invariant silently"
  //     discipline, see drain_ready()'s comment above). When set_layer() has armed buffered mode
  //     (D27), the draw is captured into layer_queue with a snapshot of the CURRENT layer/scissor
  //     state instead of reaching the batcher; otherwise it streams through batch.draw_quad() +
  //     drain_ready() exactly as the D2D-2B corner-based path already does.
  // PT: D2D-3, D24/D27 -- a cauda compartilhada "enfileira ou streama" por onde todo método de
  //     desenho de primitiva/sprite passa, DEPOIS do próprio cheque de degenerescência (um
  //     retângulo/linha/contorno degenerado é um no-op silencioso que o CHAMADOR já retornou
  //     antes de sequer chegar aqui -- espelha o próprio split "degenerado antes do passo de
  //     enfileiramento" do SpriteBatch::draw_sprite()). `corners` são FINAIS, já projetados em
  //     espaço de tela TL,TR,BR,BL ("uma história de composição só" do D24). Checa OutsideBracket
  //     via batch.in_bracket() (o acessor de introspecção público que o SpriteBatch já expunha --
  //     D31: zero mudança no SpriteBatch necessária pra lê-lo) e revalida a finitude de
  //     cantos/src_px/tint (defesa-em-profundidade estilo D10 -- o MESMO recheque que
  //     SpriteBatch::draw_quad() já aplica sobre os próprios cantos fornecidos pelo chamador,
  //     comprovadamente inalcançável na prática já que todo sítio de chamada já validou antes de
  //     chegar aqui, mantido mesmo assim pela própria disciplina "fail-high em vez de confiar num
  //     invariante em silêncio" deste módulo, ver o comentário do drain_ready() acima). Quando
  //     set_layer() armou o modo bufferizado (D27), o desenho é capturado em layer_queue com um
  //     snapshot do estado de camada/scissor CORRENTE em vez de alcançar o batcher; senão,
  //     streama por batch.draw_quad() + drain_ready() exatamente como o caminho por-cantos do
  //     D2D-2B já faz.
  void submit_quad(const draw2d_detail::SpriteCorners& corners, std::uint32_t texture_id, int tex_w,
                   int tex_h, const RectF& src_px, const ColorF& tint, const char* what) {
    if (!batch.in_bracket()) {
      log_warn(std::string(what) + "() called outside a begin()/end() bracket -- ignored (D4).");
      return;
    }
    if (!draw2d_detail::is_finite(corners) || !draw2d_detail::is_finite(src_px) ||
        !draw2d_detail::is_finite(tint)) {
      log_warn(std::string(what) + "() rejected a non-finite (NaN/Inf) value (D10).");
      return;
    }

    if (layer_armed) {
      draw2d_detail::LayerCommand cmd;
      cmd.corners = corners;
      cmd.texture_id = texture_id;
      cmd.tex_w = tex_w;
      cmd.tex_h = tex_h;
      cmd.src_px = src_px;
      cmd.tint = tint;
      cmd.scissor = draw2d_detail::ScissorSnapshot{scissor_active, scissor_rect};
      cmd.layer = current_layer;
      if (layer_queue.push(cmd) == draw2d_detail::PushResult::CapExceeded)
        log_warn(
            "set_layer(): buffered command dropped -- the 262144-command cap was reached (D27).");
      return;
    }

    Vec2F arr[4] = {corners.tl, corners.tr, corners.br, corners.bl};
    const draw2d_detail::SpriteBatch::DrawResult result =
        batch.draw_quad(texture_id, tex_w, tex_h, arr, src_px, tint);
    if (result == draw2d_detail::SpriteBatch::DrawResult::NonFinite)
      log_warn(std::string(what) + "() rejected a non-finite value at the batcher (D10)."); // unreachable in practice, see the comment above.
    drain_ready();
  }

  // EN: D2D-3, D27/D28 -- end()'s buffered-mode replay: drains layer_queue already stable-sorted
  //     by layer AND grouped into maximal same-scissor runs (LayerQueue::drain_grouped(), D27/D28's
  //     own contract), then feeds the batcher through batch.draw_quad() group by group -- each
  //     group applies its OWN scissor state to `scissor_active`/`scissor_rect` BEFORE its
  //     commands are fed to the batcher, then forces a flush boundary (flush_pending()+
  //     drain_ready()) so THAT group's GL draws execute under the RIGHT glScissor (read by
  //     set_gl_state_for_draw() at drain time). Runs entirely BEFORE end()'s own
  //     batch.end()/drain_ready() lines (unchanged) -- by the time those run, every group has
  //     already been flushed, so they are a harmless no-op tail, not a duplicate emission.
  // PT: D2D-3, D27/D28 -- o replay de modo bufferizado do end(): drena a layer_queue já
  //     ordenada-de-forma-estável por camada E agrupada em corridas maximais de mesmo scissor (o
  //     próprio contrato de LayerQueue::drain_grouped(), D27/D28), depois alimenta o batcher por
  //     batch.draw_quad() grupo a grupo -- cada grupo aplica o PRÓPRIO estado de scissor a
  //     `scissor_active`/`scissor_rect` ANTES dos comandos dele serem entregues ao batcher, depois
  //     força uma fronteira de flush (flush_pending()+drain_ready()) pra que os draws GL DAQUELE
  //     grupo executem sob o glScissor CERTO (lido por set_gl_state_for_draw() no momento do
  //     dreno). Roda inteiramente ANTES das próprias linhas batch.end()/drain_ready() do end()
  //     (inalteradas) -- quando elas rodam, todo grupo já foi flushado, então são uma cauda
  //     no-op inofensiva, não uma emissão duplicada.
  void replay_layer_queue() {
    std::vector<draw2d_detail::LayerGroup> groups = layer_queue.drain_grouped();
    for (const draw2d_detail::LayerGroup& group : groups) {
      scissor_active = group.scissor.active;
      scissor_rect = group.scissor.rect;
      for (const draw2d_detail::LayerCommand& cmd : group.commands) {
        Vec2F arr[4] = {cmd.corners.tl, cmd.corners.tr, cmd.corners.br, cmd.corners.bl};
        batch.draw_quad(cmd.texture_id, cmd.tex_w, cmd.tex_h, arr, cmd.src_px, cmd.tint);
        // NonFinite/OutsideBracket at replay time are unreachable in practice (every command was
        // already validated finite, and the bracket that BUFFERED it is the SAME one still open
        // here, D10) -- no per-command log, same "defence-in-depth, not spammed" discipline
        // drain_ready()'s own comment documents for its unreachable branch.
      }
      batch.flush_pending();
      drain_ready();
    }
  }
};

Draw2d::Draw2d() : impl_(std::make_unique<Impl>()) {}
Draw2d::~Draw2d() { shutdown(); }
Draw2d::Draw2d(Draw2d&&) noexcept = default;
Draw2d& Draw2d::operator=(Draw2d&&) noexcept = default;

bool Draw2d::init() {
  if (impl_->initialized) {
    impl_->log_warn("init() called on an already-initialized instance -- call shutdown() first.");
    return false;
  }
  // D2: idempotent-safe even when a cohabiting UiLayer/App already loaded the same table.
  if (glx_gl_load() != 0) {
    impl_->log_warn("init(): glx_gl_load() failed to resolve one or more core GL symbols.");
    return false;
  }
  if (!impl_->compile_program()) return false;
  impl_->setup_vao_vbo();
  // D2D-3, D23 -- fail-high: a Draw2d that cannot create the white texture its primitives
  // advertise does not half-initialize (init() itself fails, `initialized` stays false).
  if (!impl_->create_white_texture()) {
    impl_->log_warn("init(): failed to create the internal 1x1 white texture (D23) -- aborting.");
    return false;
  }
  impl_->initialized = true;
  return true;
}

bool Draw2d::ok() const { return impl_ && impl_->initialized; }

void Draw2d::shutdown() {
  // EN: Null-safe on purpose (unlike App's stricter "UB on moved-from" contract, app.hpp) -- a
  // moved-from Draw2d (impl_ == nullptr, see the move members' `= default` above) behaves exactly
  // like a never-initialized one: every public method here is a safe no-op, matching this
  // module's own fail-high discipline more than it needs to match App's.
  // PT: Null-safe de propósito (diferente do contrato mais estrito "UB em movido-de" do App,
  // app.hpp) -- um Draw2d movido-de (impl_ == nullptr, ver os membros de move `= default` acima)
  // se comporta exatamente como um nunca-inicializado: todo método público aqui é um no-op
  // seguro, seguindo a própria disciplina fail-high deste módulo mais do que precisaria seguir o
  // contrato do App.
  if (!impl_ || !impl_->initialized) return; // D11: idempotent no-op.
  for (Impl::TextureSlot& slot : impl_->textures) {
    if (slot.alive && slot.gl_name) glDeleteTextures(1, &slot.gl_name);
  }
  impl_->textures.clear();
  impl_->free_slots.clear();
  if (impl_->vao) {
    glDeleteVertexArrays(1, &impl_->vao);
    impl_->vao = 0;
  }
  if (impl_->vbo) {
    glDeleteBuffers(1, &impl_->vbo);
    impl_->vbo = 0;
  }
  if (impl_->program) {
    glDeleteProgram(impl_->program);
    impl_->program = 0;
  }
  impl_->uniform_target_size = -1;
  impl_->uniform_tex = -1;
  impl_->batch = draw2d_detail::SpriteBatch{4096}; // discards any open/partial bracket state.
  impl_->target_w = 0;
  impl_->target_h = 0;
  impl_->has_camera = false; // D22: a re-init()ed instance starts in screen space.
  impl_->camera = Camera2d{};
  impl_->white_texture_id = 0;   // D23: released above by the generic texture-teardown loop.
  impl_->scissor_active = false; // D28: a re-init()ed instance starts with no clip.
  impl_->scissor_rect = RectF{};
  impl_->layer_armed = false; // D27/D32: shutdown() discards any buffer, mid-bracket or not.
  impl_->current_layer = 0;
  impl_->layer_queue.clear();
  impl_->initialized = false;
}

Texture2d Draw2d::load_texture(const char* path) {
  Texture2d out; // ok_ == false by default.
  // D2D-1B fix: a moved-from Draw2d has impl_ == nullptr (see shutdown()'s comment on this
  // module's null-safe contract) -- there is no Impl to log through in that case, so the two
  // conditions below MUST short-circuit separately instead of being folded into one `||` (the
  // fold used to deref impl_->log_warn() on a null impl_, cppcheck nullPointer, D2D-1B onda3
  // lint gate). A never-initialized-but-non-null Impl still gets the diagnostic.
  if (!impl_) return out;
  if (!impl_->initialized) {
    impl_->log_warn("load_texture() called before init()/after shutdown() -- ignored.");
    return out;
  }
  if (path == nullptr) {
    impl_->log_warn("load_texture(nullptr) -- ignored.");
    return out;
  }

  // D7: const char* ifstream overload -- avoids std::filesystem::path::c_str()'s wchar_t trap
  // on MSVC (a std::string/const char* path never goes through std::filesystem here).
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    impl_->log_warn(std::string("load_texture(): could not open '") + path + "'.");
    return out;
  }
  file.seekg(0, std::ios::end);
  const std::streamoff len_off = file.tellg();
  if (len_off < 0) {
    impl_->log_warn(std::string("load_texture(): could not determine size of '") + path + "'.");
    return out;
  }
  const std::size_t len = static_cast<std::size_t>(len_off);
  // D7: 256 MiB cap, same ceiling image_decode.hpp enforces internally (kMaxImageDecodeBytes) --
  // rejected here BEFORE ever allocating a buffer for a hostile/huge asset (same "guard 1/2,
  // pre-allocation" idiom render_gl3.cpp's own LoadTexture uses for the ui-side loader).
  if (len == 0 || len > kMaxImageDecodeBytes) {
    impl_->log_warn(std::string("load_texture(): '") + path + "' is " + std::to_string(len) +
                    " bytes (0 or over the " + std::to_string(kMaxImageDecodeBytes) +
                    " byte cap) -- refusing to load.");
    return out;
  }
  file.seekg(0, std::ios::beg);
  std::vector<unsigned char> buf(len);
  if (!file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(len))) {
    impl_->log_warn(std::string("load_texture(): short read on '") + path + "'.");
    return out;
  }

  const DecodedImage decoded = decode_premultiplied_rgba(buf.data(), buf.size());
  if (!decoded.ok) {
    impl_->log_warn(std::string("load_texture(): '") + path +
                    "' failed to decode (unknown/corrupt format).");
    return out;
  }

  const GLuint gl_name = impl_->upload_gl_texture(decoded);
  if (!gl_name) {
    impl_->log_warn(std::string("load_texture(): GL texture upload failed for '") + path + "'.");
    return out;
  }

  // D2D-3, D29 -- computed ONCE here, on the decoded pixels that already exist on the CPU (about
  // to be discarded when `decoded` goes out of scope after this function returns) -- see
  // image_decode.hpp's own header comment for why this seam, not a retained buffer nor a
  // glGetTexImage readback.
  const ContentBbox bbox = compute_content_bbox(decoded.rgba.data(), decoded.width, decoded.height);

  std::size_t idx;
  if (!impl_->free_slots.empty()) {
    idx = impl_->free_slots.back();
    impl_->free_slots.pop_back();
  } else {
    idx = impl_->textures.size();
    impl_->textures.push_back(Impl::TextureSlot{});
  }
  Impl::TextureSlot& slot = impl_->textures[idx];
  slot.gl_name = gl_name;
  slot.width = decoded.width;
  slot.height = decoded.height;
  slot.alive = true;
  slot.bbox = bbox; // D29: cached, lives and dies with this slot (D32).
  // slot.generation is left as-is: 1 for a never-used slot, already bumped by destroy_texture()
  // for a reused one -- either way it is the CURRENT valid generation for this slot.

  out.ok_ = true;
  out.width_ = decoded.width;
  out.height_ = decoded.height;
  out.id_ = static_cast<std::uint32_t>(idx) + 1; // 1-based, 0 stays the invalid sentinel.
  out.generation_ = slot.generation;
  out.owner_ = impl_.get(); // D7: rejects a handle from a DIFFERENT Draw2d instance by construction.
  return out;
}

void Draw2d::destroy_texture(Texture2d& tex) {
  if (!impl_ || !impl_->initialized) {
    tex = Texture2d{};
    return;
  }
  if (tex.id_ == 0) return; // already-invalid handle -- no-op, nothing to log (not hostile, just idle).

  if (tex.owner_ != impl_.get()) {
    impl_->log_warn("destroy_texture(): handle issued by a different Draw2d instance -- ignored (D7).");
    tex = Texture2d{};
    return;
  }
  const std::size_t idx = static_cast<std::size_t>(tex.id_) - 1;
  if (idx >= impl_->textures.size()) {
    impl_->log_warn("destroy_texture(): out-of-range handle -- ignored (D7).");
    tex = Texture2d{};
    return;
  }
  Impl::TextureSlot& slot = impl_->textures[idx];
  if (!slot.alive || slot.generation != tex.generation_) {
    impl_->log_warn("destroy_texture(): stale/already-destroyed/tampered handle -- ignored (D7).");
    tex = Texture2d{};
    return;
  }

  if (slot.gl_name) glDeleteTextures(1, &slot.gl_name);
  slot.gl_name = 0;
  slot.width = 0;
  slot.height = 0;
  slot.bbox = ContentBbox{}; // D29/D32: the bbox cache lives and dies with this slot.
  slot.alive = false;
  slot.generation++; // any surviving copy of `tex` (or of the handle this call zeroes) is now stale.
  impl_->free_slots.push_back(idx);
  tex = Texture2d{};
}

void Draw2d::begin(int target_width, int target_height) {
  if (!impl_ || !impl_->initialized) return;
  const bool clean = impl_->batch.begin(target_width, target_height);
  if (!clean) {
    impl_->log_warn(
        "begin() called while a previous bracket was still open -- "
        "implicitly closing it (D4).");
    // Drain BEFORE updating target_w/target_h below: the leftover run this implicit end()
    // finalized was generated against the OLD target size and must be drawn against it.
    impl_->drain_ready();
  }
  impl_->target_w = target_width;
  impl_->target_h = target_height;
  if (target_width <= 0 || target_height <= 0) {
    impl_->log_warn(
        "begin() called with a non-positive target size -- "
        "this bracket draws no-op until end() (D10).");
  }
}

void Draw2d::draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px,
                         const ColorF& tint) {
  if (!impl_ || !impl_->initialized) return;

  if (!tex.ok_ || tex.id_ == 0) {
    impl_->log_warn("draw_sprite(): invalid (never-loaded) texture handle -- ignored.");
    return;
  }
  if (tex.owner_ != impl_.get()) {
    impl_->log_warn("draw_sprite(): texture handle issued by a different Draw2d instance -- ignored (D7).");
    return;
  }
  const std::size_t idx = static_cast<std::size_t>(tex.id_) - 1;
  if (idx >= impl_->textures.size() || !impl_->textures[idx].alive ||
      impl_->textures[idx].generation != tex.generation_) {
    impl_->log_warn("draw_sprite(): unknown/stale/tampered texture handle -- ignored (D7).");
    return;
  }
  const Impl::TextureSlot& slot = impl_->textures[idx];

  // D2D-3, D27 -- a layer-armed bracket buffers this draw instead of streaming it (the CURRENT
  // layer tags sprites too, not just primitives -- draw2d.hpp's own class comment on
  // set_layer()). Guard order mirrors SpriteBatch::draw_sprite()'s own literal order (finite
  // check, THEN the degenerate no-op) so a hostile dst/src_px/tint is rejected IDENTICALLY
  // whether this bracket is streaming or buffered -- dst is expanded to its own axis-aligned
  // screen-space corners directly (this overload never consults the camera, D5's literal
  // no-camera contract, unchanged either way).
  if (impl_->layer_armed) {
    if (!draw2d_detail::is_finite(dst) || !draw2d_detail::is_finite(src_px) ||
        !draw2d_detail::is_finite(tint)) {
      impl_->log_warn("draw_sprite() rejected a non-finite (NaN/Inf) dst/src_px/tint value (D10).");
      return;
    }
    if (dst.w <= 0.f || dst.h <= 0.f) return; // D10: silent legal no-op.
    impl_->submit_quad(
        draw2d_detail::SpriteCorners{Vec2F{dst.x, dst.y}, Vec2F{dst.x + dst.w, dst.y},
                                     Vec2F{dst.x + dst.w, dst.y + dst.h}, Vec2F{dst.x, dst.y + dst.h}},
        tex.id_, slot.width, slot.height, src_px, tint, "draw_sprite");
    return;
  }

  const draw2d_detail::SpriteBatch::DrawResult result =
      impl_->batch.draw_sprite(tex.id_, slot.width, slot.height, dst, src_px, tint);
  using DR = draw2d_detail::SpriteBatch::DrawResult;
  if (result == DR::OutsideBracket)
    impl_->log_warn("draw_sprite() called outside a begin()/end() bracket -- ignored (D4).");
  else if (result == DR::NonFinite)
    impl_->log_warn("draw_sprite() rejected a non-finite (NaN/Inf) dst/src_px/tint value (D10).");
  // BracketInvalid / Degenerate / Ok are legal no-ops or successful queues -- nothing to log
  // per-call (BracketInvalid was already logged once at the offending begin(); Degenerate is a
  // documented legal no-op, not an error, D10).

  impl_->drain_ready();
}

// EN: D2D-2B, Q1 (c)/D15 -- fail-high: any non-finite `camera` member or `camera.zoom <= 0`
//     rejects the call, KEEPING the previous state (including "no camera") -- one dedup'd
//     log line, never a partial/corrupt camera write. `draw2d_detail::is_finite(Camera2d)`
//     is transform2d.hpp's own predicate (D2D-2A), reused here rather than re-derived --
//     single source, same rationale D12's own header comment gives for the projection
//     formulas themselves.
// PT: D2D-2B, Q1 (c)/D15 -- fail-high: qualquer membro não-finito de `camera` ou
//     `camera.zoom <= 0` rejeita a chamada, MANTENDO o estado anterior (inclusive "sem
//     câmera") -- uma linha de log dedup'd, nunca uma escrita parcial/corrompida de câmera.
//     `draw2d_detail::is_finite(Camera2d)` é o próprio predicado de transform2d.hpp
//     (D2D-2A), reusado aqui em vez de re-derivado -- fonte única, mesma racional que o
//     próprio comentário de cabeçalho do D12 dá pras fórmulas de projeção em si.
void Draw2d::set_camera(const Camera2d& camera) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(camera) || camera.zoom <= 0.f) {
    impl_->log_warn(
        "set_camera(): rejected a non-finite member or zoom<=0 -- "
        "previous camera state kept (D15).");
    return;
  }
  impl_->has_camera = true;
  impl_->camera = camera;
}

// EN: D2D-2B, Q1 (c) -- idempotent, safe to call with no camera set (a no-op in that case).
//     `shutdown()` already does this implicitly (D22).
// PT: D2D-2B, Q1 (c) -- idempotente, seguro chamar sem câmera setada (um no-op nesse caso).
//     `shutdown()` já faz isso implicitamente (D22).
void Draw2d::reset_camera() {
  if (!impl_ || !impl_->initialized) return;
  impl_->has_camera = false;
  impl_->camera = Camera2d{};
}

// EN: D2D-2B, D18/D20 overload -- `transform` applies pivot/rotation/scale/flip about `dst`
//     in whichever space is currently active (world units with a camera set, screen px
//     otherwise). Fail-high order, literal (D20): (1) every `transform` member finite
//     BEFORE any arithmetic; (2) `dst.w <= 0 || dst.h <= 0` stays the silent legal no-op the
//     4-arg overload already has (unchanged); (3) the texture-handle validation the 4-arg
//     overload already runs (D7, duplicated here rather than factored out -- the 4-arg path
//     must stay byte-for-byte untouched, Q1 (c)'s compatibility guarantee); (4) corners via
//     `transform2d.hpp::compute_sprite_corners` (D2D-2A, single source); (5) WITH a camera,
//     each corner projected through `world_to_screen` (D12) -- CPU-side, pre-batcher (D13),
//     so this never forces a flush; (6) the post-math guard: the FINAL (post-projection)
//     corners must be finite, or the draw is skipped (D20's second guard, overflow with no
//     v0.20.0 twin by design -- catches overflow from EITHER the corner math or the camera
//     projection, whichever ran). flip_h/flip_v (D18) swap UV on the resolved src_px
//     sub-rect -- implemented here (not by SpriteTransform itself) by swapping which UV
//     corner each screen corner receives, same idiom draw_quad's own u0/v0/u1/v1 pairing
//     already uses.
// PT: D2D-2B, overload D18/D20 -- `transform` aplica pivô/rotação/escala/flip sobre `dst` no
//     espaço que estiver ativo no momento (unidades de mundo com câmera setada, px de tela
//     caso contrário). Ordem fail-high, literal (D20): (1) todo membro de `transform` finito
//     ANTES de qualquer conta; (2) `dst.w <= 0 || dst.h <= 0` continua o no-op legal
//     silencioso que o overload de 4 args já tem (inalterado); (3) a validação de handle de
//     textura que o overload de 4 args já roda (D7, duplicada aqui em vez de fatorada -- o
//     caminho de 4 args precisa ficar byte-a-byte intocado, a garantia de compatibilidade da
//     Q1 (c)); (4) cantos via `transform2d.hpp::compute_sprite_corners` (D2D-2A, fonte
//     única); (5) COM câmera, cada canto projetado por `world_to_screen` (D12) -- CPU-side,
//     pré-batcher (D13), então isto nunca força um flush; (6) a guarda pós-conta: os cantos
//     FINAIS (pós-projeção) precisam ser finitos, ou o desenho é pulado (a segunda guarda do
//     D20, overflow sem gêmeo na v0.20.0 por design -- pega overflow tanto da matemática de
//     canto quanto da projeção de câmera, qualquer uma que tenha rodado). flip_h/flip_v
//     (D18) trocam UV no sub-retângulo `src_px` resolvido -- implementado aqui (não pelo
//     próprio SpriteTransform) trocando qual canto UV cada canto de tela recebe, mesmo
//     idioma que o próprio pareamento u0/v0/u1/v1 do draw_quad já usa.
void Draw2d::draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px,
                         const ColorF& tint, const SpriteTransform& transform) {
  if (!impl_ || !impl_->initialized) return;

  if (!draw2d_detail::is_finite(transform)) {
    impl_->log_warn(
        "draw_sprite(transform): rejected a non-finite (NaN/Inf) SpriteTransform member (D20).");
    return;
  }
  if (dst.w <= 0.f || dst.h <= 0.f) return; // D10: silent legal no-op, unchanged from the 4-arg overload.

  if (!tex.ok_ || tex.id_ == 0) {
    impl_->log_warn("draw_sprite(transform): invalid (never-loaded) texture handle -- ignored.");
    return;
  }
  if (tex.owner_ != impl_.get()) {
    impl_->log_warn(
        "draw_sprite(transform): texture handle issued by a different Draw2d instance -- ignored (D7).");
    return;
  }
  const std::size_t idx = static_cast<std::size_t>(tex.id_) - 1;
  if (idx >= impl_->textures.size() || !impl_->textures[idx].alive ||
      impl_->textures[idx].generation != tex.generation_) {
    impl_->log_warn("draw_sprite(transform): unknown/stale/tampered texture handle -- ignored (D7).");
    return;
  }
  const Impl::TextureSlot& slot = impl_->textures[idx];

  const draw2d_detail::SpriteCorners local = draw2d_detail::compute_sprite_corners(dst, transform);

  Vec2F projected[4];
  if (impl_->has_camera) {
    projected[0] =
        draw2d_detail::world_to_screen(impl_->camera, impl_->target_w, impl_->target_h, local.tl);
    projected[1] =
        draw2d_detail::world_to_screen(impl_->camera, impl_->target_w, impl_->target_h, local.tr);
    projected[2] =
        draw2d_detail::world_to_screen(impl_->camera, impl_->target_w, impl_->target_h, local.br);
    projected[3] =
        draw2d_detail::world_to_screen(impl_->camera, impl_->target_w, impl_->target_h, local.bl);
  } else {
    projected[0] = local.tl;
    projected[1] = local.tr;
    projected[2] = local.br;
    projected[3] = local.bl;
  }

  const draw2d_detail::SpriteCorners final_corners{projected[0], projected[1], projected[2],
                                                   projected[3]};
  if (!draw2d_detail::is_finite(final_corners)) {
    impl_->log_warn(
        "draw_sprite(transform): rejected a non-finite computed corner (overflow, D20).");
    return;
  }

  // D18: flip_h/flip_v swap UV on the RESOLVED src_px sub-rect, never touching src_px
  // itself -- draw_quad() below always assigns UV by SLOT (slot0->u0v0, slot1->u1v0,
  // slot2->u1v1, slot3->u0v1, the same TL/TR/BR/BL pairing draw_sprite's own emit_quad()
  // uses), so re-ordering which PHYSICAL corner occupies which slot flips which texel that
  // physical corner samples, with the quad's screen-space footprint (the SET of 4 physical
  // positions) unchanged. flip_h swaps the two UV-column slot-pairs (0,1) and (2,3);
  // flip_v swaps the two UV-row slot-pairs (0,3) and (1,2); applying both composes to the
  // 180-degree UV permutation (each physical corner samples its diagonal opposite's texel).
  Vec2F quad_corners[4] = {projected[0], projected[1], projected[2], projected[3]};
  if (transform.flip_h) {
    std::swap(quad_corners[0], quad_corners[1]);
    std::swap(quad_corners[2], quad_corners[3]);
  }
  if (transform.flip_v) {
    std::swap(quad_corners[0], quad_corners[3]);
    std::swap(quad_corners[1], quad_corners[2]);
  }

  // D2D-3, D27 -- a layer-armed bracket buffers this draw instead of streaming it (the CURRENT
  // layer tags sprites too, matching this method's own class comment above). `quad_corners`
  // here are ALREADY the final, post-flip, post-projection screen corners this function computed
  // above -- reused as-is, no re-derivation.
  if (impl_->layer_armed) {
    impl_->submit_quad(
        draw2d_detail::SpriteCorners{quad_corners[0], quad_corners[1], quad_corners[2],
                                     quad_corners[3]},
        tex.id_, slot.width, slot.height, src_px, tint, "draw_sprite(transform)");
    return;
  }

  const draw2d_detail::SpriteBatch::DrawResult result =
      impl_->batch.draw_quad(tex.id_, slot.width, slot.height, quad_corners, src_px, tint);
  using DR = draw2d_detail::SpriteBatch::DrawResult;
  if (result == DR::OutsideBracket)
    impl_->log_warn("draw_sprite(transform) called outside a begin()/end() bracket -- ignored (D4).");
  else if (result == DR::NonFinite)
    impl_->log_warn(
        "draw_sprite(transform) rejected a non-finite (NaN/Inf) src_px/tint value (D10).");

  impl_->drain_ready();
}

// EN: D2D-3, D23/D24 -- untextured filled rectangle: geometry in the ACTIVE space (world units
//     with a camera set, screen px otherwise, D18's sprite rule), through the SAME batcher path
//     as draw_sprite() against the internal white texture (D23). Guard order, literal: finite
//     dst/color BEFORE any arithmetic; dst.w<=0||dst.h<=0 a SILENT legal no-op (D10); THEN
//     project (world_to_screen per corner when a camera is set); THEN the post-math guard on the
//     PROJECTED corners (D20's second guard, overflow). See draw2d.hpp's own doc-comment for the
//     full contract.
// PT: D2D-3, D23/D24 -- retângulo preenchido não-texturizado: geometria no espaço ATIVO (unidades
//     de mundo com câmera setada, px de tela caso contrário, regra de sprite do D18), pelo MESMO
//     caminho de batcher do draw_sprite() contra a textura branca interna (D23). Ordem de guarda,
//     literal: dst/color finitos ANTES de qualquer conta; dst.w<=0||dst.h<=0 um no-op legal
//     SILENCIOSO (D10); DEPOIS projeta (world_to_screen por canto quando há câmera); DEPOIS a
//     guarda pós-conta sobre os cantos PROJETADOS (segunda guarda do D20, overflow). Ver o
//     próprio doc-comment de draw2d.hpp pro contrato completo.
void Draw2d::draw_filled_rect(const RectF& dst, const ColorF& color) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(dst) || !draw2d_detail::is_finite(color)) {
    impl_->log_warn("draw_filled_rect(): rejected a non-finite (NaN/Inf) dst/color value (D10).");
    return;
  }
  if (dst.w <= 0.f || dst.h <= 0.f) return; // D10: silent legal no-op.

  const draw2d_detail::SpriteCorners local{
      Vec2F{dst.x, dst.y}, Vec2F{dst.x + dst.w, dst.y}, Vec2F{dst.x + dst.w, dst.y + dst.h},
      Vec2F{dst.x, dst.y + dst.h}};
  const draw2d_detail::SpriteCorners projected = impl_->project_corners(local);
  if (!draw2d_detail::is_finite(projected)) {
    impl_->log_warn("draw_filled_rect(): rejected a non-finite projected corner (overflow, D20).");
    return;
  }
  impl_->submit_quad(projected, impl_->white_texture_id, 1, 1, RectF{}, color, "draw_filled_rect");
}

// EN: D2D-3, D18/D24 -- the transform overload: `transform` applies pivot/rotation/scale/flip
//     about `dst` in the ACTIVE space (same corner math draw_sprite(transform) uses,
//     compute_sprite_corners(), transform2d.hpp, single source), each resulting corner then
//     projected when a camera is set. flip_h/flip_v have no visual effect on a solid fill (no
//     texel to flip, the white texture's own {0,0,0,0} sentinel src_px covers it uniformly) but
//     are accepted without error, same total-input contract as every SpriteTransform consumer.
// PT: D2D-3, D18/D24 -- o overload de transform: `transform` aplica pivô/rotação/escala/flip
//     sobre `dst` no espaço ATIVO (mesma matemática de canto que draw_sprite(transform) usa,
//     compute_sprite_corners(), transform2d.hpp, fonte única), cada canto resultante então
//     projetado quando há câmera. flip_h/flip_v não têm efeito visual sobre um preenchimento
//     sólido (nenhum texel pra espelhar, o próprio src_px sentinela {0,0,0,0} da textura branca
//     cobre uniformemente) mas são aceitos sem erro, mesmo contrato de input total de todo
//     consumidor de SpriteTransform.
void Draw2d::draw_filled_rect(const RectF& dst, const ColorF& color,
                              const SpriteTransform& transform) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(transform)) {
    impl_->log_warn(
        "draw_filled_rect(transform): rejected a non-finite (NaN/Inf) SpriteTransform member (D20).");
    return;
  }
  if (!draw2d_detail::is_finite(dst) || !draw2d_detail::is_finite(color)) {
    impl_->log_warn("draw_filled_rect(transform): rejected a non-finite (NaN/Inf) dst/color value (D10).");
    return;
  }
  if (dst.w <= 0.f || dst.h <= 0.f) return; // D10: silent legal no-op.

  const draw2d_detail::SpriteCorners local = draw2d_detail::compute_sprite_corners(dst, transform);
  const draw2d_detail::SpriteCorners projected = impl_->project_corners(local);
  if (!draw2d_detail::is_finite(projected)) {
    impl_->log_warn(
        "draw_filled_rect(transform): rejected a non-finite projected corner (overflow, D20).");
    return;
  }
  impl_->submit_quad(projected, impl_->white_texture_id, 1, 1, RectF{}, color,
                     "draw_filled_rect(transform)");
}

// EN: D2D-3, D24 -- arbitrary quad, corners supplied directly by the caller in TL,TR,BR,BL order
//     (D5/D19's convention), in the ACTIVE space, each projected when a camera is set. A
//     degenerate or self-crossing (bowtie) quad is LEGAL (the GPU's own business, documented) --
//     no dst.w/h-style degenerate check exists here on purpose, there is no dst rect.
// PT: D2D-3, D24 -- quad arbitrário, cantos fornecidos direto pelo chamador na ordem TL,TR,BR,BL
//     (convenção D5/D19), no espaço ATIVO, cada um projetado quando há câmera. Um quad degenerado
//     ou auto-cruzado (bowtie) é LEGAL (negócio da própria GPU, documentado) -- nenhum cheque de
//     degenerescência estilo dst.w/h existe aqui de propósito, não há retângulo dst.
void Draw2d::draw_filled_quad(Vec2F tl, Vec2F tr, Vec2F br, Vec2F bl, const ColorF& color) {
  if (!impl_ || !impl_->initialized) return;
  const draw2d_detail::SpriteCorners local{tl, tr, br, bl};
  if (!draw2d_detail::is_finite(local) || !draw2d_detail::is_finite(color)) {
    impl_->log_warn("draw_filled_quad(): rejected a non-finite (NaN/Inf) corner/color value (D10).");
    return;
  }
  const draw2d_detail::SpriteCorners projected = impl_->project_corners(local);
  if (!draw2d_detail::is_finite(projected)) {
    impl_->log_warn("draw_filled_quad(): rejected a non-finite projected corner (overflow, D20).");
    return;
  }
  impl_->submit_quad(projected, impl_->white_texture_id, 1, 1, RectF{}, color, "draw_filled_quad");
}

// EN: D2D-3, D25 -- a line segment of `thickness` ACTIVE-space units, butt-capped, geometry via
//     src/primitives2d.hpp's compute_line_corners() (single source, D25's own literal formula).
//     Guard order, literal: a/b/thickness/color ALL finite BEFORE any arithmetic (the sqrt
//     included, is_degenerate_line() itself does not check finiteness -- see that predicate's
//     own header comment); `a == b` (exactly) or `thickness <= 0` a SILENT legal no-op BEFORE the
//     division (no divide-by-zero path exists); THEN project; THEN the post-math guard.
// PT: D2D-3, D25 -- um segmento de linha de `thickness` unidades do espaço ATIVO, com tampa butt,
//     geometria via compute_line_corners() de src/primitives2d.hpp (fonte única, a própria
//     fórmula literal do D25). Ordem de guarda, literal: a/b/thickness/color TODOS finitos ANTES
//     de qualquer conta (o sqrt incluso, o próprio is_degenerate_line() não checa finitude -- ver
//     o comentário de cabeçalho daquele predicado); `a == b` (exatamente) ou `thickness <= 0` um
//     no-op legal SILENCIOSO ANTES da divisão (nenhum caminho de divisão-por-zero existe); DEPOIS
//     projeta; DEPOIS a guarda pós-conta.
void Draw2d::draw_line(Vec2F a, Vec2F b, float thickness, const ColorF& color) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(a) || !draw2d_detail::is_finite(b) || !std::isfinite(thickness) ||
      !draw2d_detail::is_finite(color)) {
    impl_->log_warn("draw_line(): rejected a non-finite (NaN/Inf) a/b/thickness/color value (D10).");
    return;
  }
  if (draw2d_detail::is_degenerate_line(a, b, thickness)) return; // D10: silent legal no-op.

  const draw2d_detail::SpriteCorners local = draw2d_detail::compute_line_corners(a, b, thickness);
  const draw2d_detail::SpriteCorners projected = impl_->project_corners(local);
  if (!draw2d_detail::is_finite(projected)) {
    impl_->log_warn("draw_line(): rejected a non-finite projected corner (overflow, D20).");
    return;
  }
  impl_->submit_quad(projected, impl_->white_texture_id, 1, 1, RectF{}, color, "draw_line");
}

// EN: D2D-3, D26 -- a rectangular outline of `thickness` ACTIVE-space units, decomposed into 4
//     NON-OVERLAPPING strips via src/primitives2d.hpp's compute_outline_strips() (single source,
//     the collapse clamp `t = min(thickness, min(dst.w,dst.h)/2)` tiling `dst` exactly with no
//     special-case branch once `2*thickness >= min(dst.w,dst.h)`). Each strip is projected and
//     submitted independently -- a non-finite PROJECTED corner on one strip skips only THAT
//     strip (logged once, dedup'd), the others still draw (a strip-scoped D20 guard, not an
//     all-or-nothing abort for the whole outline).
// PT: D2D-3, D26 -- um contorno retangular de `thickness` unidades do espaço ATIVO, decomposto em
//     4 faixas SEM SOBREPOSIÇÃO via compute_outline_strips() de src/primitives2d.hpp (fonte
//     única, o clamp de colapso `t = min(thickness, min(dst.w,dst.h)/2)` ladrilhando `dst`
//     exatamente sem ramo de caso especial quando `2*thickness >= min(dst.w,dst.h)`). Cada faixa
//     é projetada e submetida independentemente -- um canto PROJETADO não-finito numa faixa pula
//     só AQUELA faixa (logado uma vez, dedup'd), as outras ainda desenham (uma guarda D20
//     escopada por-faixa, não um aborto tudo-ou-nada do contorno inteiro).
void Draw2d::draw_rect_outline(const RectF& dst, float thickness, const ColorF& color) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(dst) || !std::isfinite(thickness) ||
      !draw2d_detail::is_finite(color)) {
    impl_->log_warn(
        "draw_rect_outline(): rejected a non-finite (NaN/Inf) dst/thickness/color value (D10).");
    return;
  }
  if (draw2d_detail::is_degenerate_outline(dst, thickness)) return; // D10: silent legal no-op.

  const draw2d_detail::OutlineStrips strips = draw2d_detail::compute_outline_strips(dst, thickness);
  const RectF strip_rects[4] = {strips.top, strips.bottom, strips.left, strips.right};
  for (const RectF& strip : strip_rects) {
    const draw2d_detail::SpriteCorners local{
        Vec2F{strip.x, strip.y}, Vec2F{strip.x + strip.w, strip.y},
        Vec2F{strip.x + strip.w, strip.y + strip.h}, Vec2F{strip.x, strip.y + strip.h}};
    const draw2d_detail::SpriteCorners projected = impl_->project_corners(local);
    if (!draw2d_detail::is_finite(projected)) {
      impl_->log_warn("draw_rect_outline(): rejected a non-finite projected corner (overflow, D20).");
      continue;
    }
    impl_->submit_quad(projected, impl_->white_texture_id, 1, 1, RectF{}, color, "draw_rect_outline");
  }
}

// EN: D2D-3, D27 -- arms buffered mode (opt-in, default OFF). The FIRST call arms the CURRENT
//     bracket (or the NEXT one when called outside a bracket -- batch.in_bracket() answers
//     which); already-streamed draws in the current bracket are finalized+drained under the OLD
//     streaming path FIRST (flush_pending(), the D31 additive method, + drain_ready()) so they
//     render below every layer, exactly as documented. Sticky WITHIN the bracket; reset to
//     disarmed/layer 0 by end() (D27's own "not sticky across brackets like the camera" rule).
// PT: D2D-3, D27 -- arma o modo bufferizado (opt-in, default OFF). A PRIMEIRA chamada arma o
//     bracket CORRENTE (ou o PRÓXIMO se chamada fora de um bracket -- batch.in_bracket() responde
//     qual); desenhos já streamados no bracket corrente são finalizados+drenados pelo caminho
//     streaming ANTIGO PRIMEIRO (flush_pending(), o método aditivo do D31, + drain_ready()) pra
//     que renderizem abaixo de toda camada, exatamente como documentado. Sticky DENTRO do
//     bracket; reseta pra desarmado/camada 0 no end() (a própria regra do D27 de "não sticky
//     entre brackets como a câmera").
void Draw2d::set_layer(int layer) {
  if (!impl_ || !impl_->initialized) return;
  if (!impl_->layer_armed) {
    impl_->batch.flush_pending();
    impl_->drain_ready();
    impl_->layer_armed = true;
  }
  impl_->current_layer = layer;
}

// EN: D2D-3, D28 -- rectangular clip, ALWAYS screen px, camera-independent (A3). Validation
//     (D15's idiom): any non-finite `rect_px` member REJECTS the call, PREVIOUS state kept, one
//     dedup'd log; `rect_px.w<=0 || rect_px.h<=0` is LEGAL ("clip everything", GL semantics);
//     negative x/y are LEGAL (clamping is the GPU's job). A mid-bracket change in STREAMING mode
//     forces a run boundary (the pending run is finalized+drained under the OLD scissor state
//     FIRST, via flush_pending()+drain_ready(), same D31 method set_layer() above already uses)
//     -- named CONTRAST with the camera (D13: never forces a flush; scissor DOES). In BUFFERED
//     mode nothing is forced here: each command's OWN scissor snapshot is captured at push()
//     time (impl_->submit_quad()), so this call only updates the STATE that future draws (and
//     future snapshots) will see.
// PT: D2D-3, D28 -- recorte retangular, SEMPRE em px de tela, câmera-independente (A3). Validação
//     (idioma do D15): qualquer membro não-finito de `rect_px` REJEITA a chamada, estado ANTERIOR
//     mantido, um log dedup'd; `rect_px.w<=0 || rect_px.h<=0` é LEGAL ("clipa tudo", semântica
//     GL); x/y negativos são LEGAIS (clampar é trabalho da GPU). Uma mudança no meio do bracket
//     em modo STREAMING força uma fronteira de corrida (a corrida pendente é finalizada+drenada
//     sob o scissor VELHO PRIMEIRO, via flush_pending()+drain_ready(), o mesmo método do D31 que
//     o set_layer() acima já usa) -- CONTRASTE nomeado com a câmera (D13: nunca força um flush; o
//     scissor FORÇA). Em modo BUFFERIZADO nada é forçado aqui: o PRÓPRIO snapshot de scissor de
//     cada comando é capturado no momento do push() (impl_->submit_quad()), então esta chamada só
//     atualiza o ESTADO que desenhos (e snapshots) futuros vão ver.
void Draw2d::set_scissor(const RectF& rect_px) {
  if (!impl_ || !impl_->initialized) return;
  if (!draw2d_detail::is_finite(rect_px)) {
    impl_->log_warn(
        "set_scissor(): rejected a non-finite member -- previous scissor state kept (D15/D28).");
    return;
  }
  if (!impl_->layer_armed) {
    impl_->batch.flush_pending();
    impl_->drain_ready();
  }
  impl_->scissor_active = true;
  impl_->scissor_rect = rect_px;
}

// EN: D2D-3, D28 -- idempotent, safe to call with no scissor set. shutdown() already does this
//     implicitly (same D22 idiom as reset_camera()). Same streaming-run-boundary behaviour as
//     set_scissor() above (a reset IS a scissor state change).
// PT: D2D-3, D28 -- idempotente, seguro chamar sem scissor setado. shutdown() já faz isso
//     implicitamente (mesmo idioma D22 do reset_camera()). Mesmo comportamento de fronteira de
//     corrida em streaming do set_scissor() acima (um reset É uma mudança de estado de scissor).
void Draw2d::reset_scissor() {
  if (!impl_ || !impl_->initialized) return;
  if (!impl_->layer_armed) {
    impl_->batch.flush_pending();
    impl_->drain_ready();
  }
  impl_->scissor_active = false;
  impl_->scissor_rect = RectF{};
}

// EN: D2D-3, D29 -- the smallest rect containing every texel with alpha>0, cached at
//     load_texture() time (impl_->textures[idx].bbox). `found == false` (every field zero) on
//     the full D7 handle-validation chain (invalid/stale/foreign/never-loaded, same rejection
//     story as draw_sprite()) OR a fully-transparent texture.
// PT: D2D-3, D29 -- o menor retângulo contendo todo texel com alpha>0, cacheado no momento do
//     load_texture() (impl_->textures[idx].bbox). `found == false` (todo campo zero) na cadeia
//     completa de validação D7 do handle (inválido/obsoleto/estrangeiro/nunca-carregado, mesma
//     história de rejeição do draw_sprite()) OU uma textura totalmente transparente.
TextureBbox Draw2d::texture_content_bbox(const Texture2d& tex) {
  if (!impl_ || !impl_->initialized) return TextureBbox{};
  if (!tex.ok_ || tex.id_ == 0) {
    impl_->log_warn("texture_content_bbox(): invalid (never-loaded) texture handle -- ignored.");
    return TextureBbox{};
  }
  if (tex.owner_ != impl_.get()) {
    impl_->log_warn(
        "texture_content_bbox(): texture handle issued by a different Draw2d instance -- ignored (D7).");
    return TextureBbox{};
  }
  const std::size_t idx = static_cast<std::size_t>(tex.id_) - 1;
  if (idx >= impl_->textures.size() || !impl_->textures[idx].alive ||
      impl_->textures[idx].generation != tex.generation_) {
    impl_->log_warn("texture_content_bbox(): unknown/stale/tampered texture handle -- ignored (D7).");
    return TextureBbox{};
  }
  const Impl::TextureSlot& slot = impl_->textures[idx];
  if (!slot.bbox.found) return TextureBbox{};
  return TextureBbox{true, slot.bbox.x, slot.bbox.y, slot.bbox.w, slot.bbox.h};
}

void Draw2d::end() {
  if (!impl_ || !impl_->initialized) return;
  // D2D-3, D27 -- the buffered-mode replay runs BEFORE the two existing lines below (unchanged):
  // by the time they run, every layer group has already been flushed+drained by
  // replay_layer_queue() itself, so they are a harmless no-op tail in that case, and the SAME
  // (only) path taken when layer mode was never armed.
  if (impl_->layer_armed) impl_->replay_layer_queue();
  impl_->batch.end();
  impl_->drain_ready();
  impl_->layer_armed = false; // D27: NOT sticky across brackets, unlike the camera/scissor.
  impl_->current_layer = 0;
}

// EN: D2D-2B, D21 -- the PUBLIC free functions declared in draw2d.hpp, defined out-of-line
//     HERE (not header-inline) so `nm` still proves their removal when
//     GLINTFX_MODULE_DRAW2D=OFF. Each is a one-line delegation to the SAME internal
//     formulas `draw_sprite`'s camera path uses above (src/transform2d.hpp, D2D-2A) --
//     single source, the draw path and these helpers cannot diverge (plan section 0, risk
//     2). Qualified calls (`draw2d_detail::...`), not `using` + unqualified: ADL on
//     Camera2d/Vec2F (both `glintfx::`) would otherwise find these very declarations again
//     and the two overloads match identically (see transform2d_sanity.cpp's own w2s/s2w/
//     fit_cam wrapper comment for the same ambiguity, hit independently by D2D-2A's tests).
// PT: D2D-2B, D21 -- as free functions PÚBLICAS declaradas em draw2d.hpp, definidas fora de
//     linha AQUI (não header-inline) pra que o `nm` continue provando a remoção delas
//     quando GLINTFX_MODULE_DRAW2D=OFF. Cada uma é uma delegação de uma linha pras MESMAS
//     fórmulas internas que o caminho de câmera do `draw_sprite` usa acima
//     (src/transform2d.hpp, D2D-2A) -- fonte única, o caminho de desenho e estes helpers não
//     podem divergir (plano seção 0, risco 2). Chamadas qualificadas
//     (`draw2d_detail::...`), não `using` + não-qualificada: ADL em Camera2d/Vec2F (ambos
//     `glintfx::`) acharia estas mesmas declarações de novo e os dois overloads bateriam
//     identicamente (ver o próprio comentário do wrapper w2s/s2w/fit_cam de
//     transform2d_sanity.cpp pra mesma ambiguidade, batida independentemente pelos testes
//     do D2D-2A).
Vec2F world_to_screen(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F world) {
  return draw2d_detail::world_to_screen(cam, viewport_w, viewport_h, world);
}

Vec2F screen_to_world(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F screen) {
  return draw2d_detail::screen_to_world(cam, viewport_w, viewport_h, screen);
}

Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h) {
  return draw2d_detail::camera_from_world_rect(world_rect, viewport_w, viewport_h);
}

} // namespace glintfx
