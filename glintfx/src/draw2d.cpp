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
#include "log_dedup.hpp"
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
  };
  std::vector<TextureSlot> textures;
  std::vector<std::size_t> free_slots;

  draw2d_detail::SpriteBatch batch{4096};
  int target_w = 0;
  int target_h = 0;

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
    glDisable(GL_SCISSOR_TEST);
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

void Draw2d::end() {
  if (!impl_ || !impl_->initialized) return;
  impl_->batch.end();
  impl_->drain_ready();
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
