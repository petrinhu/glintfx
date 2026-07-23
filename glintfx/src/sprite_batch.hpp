// SPDX-License-Identifier: MPL-2.0
// EN: D2D-1B -- the PURE batching policy behind `glintfx::Draw2d` (plan
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decisions D4/D5/D8/D10). ZERO GL
//     dependency in this header -- same discipline as `log_dedup.hpp`/`input_state.hpp`: this
//     class takes draw commands and emits vertex data + flush ranges, headless-testable
//     (`tests/sprite_batch_sanity.cpp`), with `draw2d.cpp` owning ONLY the GL execution of what
//     this class decided. Includes `<glintfx/draw2d.hpp>` for `RectF`/`ColorF` (public types,
//     not third-party -- reusing them here avoids a duplicate POD family).
//
//     MEMORY-BOUND CONTRACT (the "hostile 100k-draw stream" guard, D4): this class bounds ONE
//     RUN's vertex buffer at `max_quads_per_flush` (default 4096) -- reaching that count inside a
//     bracket forces an internal flush (the run is moved into the ready queue and a fresh, empty
//     run starts under the SAME texture). It does NOT bound the READY QUEUE itself: a caller that
//     never drains `take_ready()` between `draw_sprite()` calls can still grow memory without
//     limit. `Draw2d::draw_sprite()` (draw2d.cpp) drains the ready queue after EVERY call --
//     mirror that same drain discipline in any other caller (including this header's own test).
// PT: D2D-1B -- a política de batching PURA por trás do `glintfx::Draw2d` (plano
//     docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md, decisões D4/D5/D8/D10). ZERO
//     dependência de GL neste header -- mesma disciplina de `log_dedup.hpp`/`input_state.hpp`:
//     esta classe recebe comandos de desenho e emite dado de vértice + faixas de flush, testável
//     headless (`tests/sprite_batch_sanity.cpp`), com o `draw2d.cpp` sendo dono SÓ da execução GL
//     do que esta classe decidiu. Inclui `<glintfx/draw2d.hpp>` para `RectF`/`ColorF` (tipos
//     públicos, não de terceiro -- reusá-los aqui evita uma família de POD duplicada).
//
//     CONTRATO DE LIMITE DE MEMÓRIA (a guarda do "stream hostil de 100k desenhos", D4): esta
//     classe limita o buffer de vértice de UMA CORRIDA em `max_quads_per_flush` (padrão 4096) --
//     atingir essa contagem dentro de um bracket força um flush interno (a corrida é movida pra
//     fila pronta e uma corrida nova, vazia, começa sob a MESMA textura). NÃO limita a PRÓPRIA
//     fila pronta: um chamador que nunca drena `take_ready()` entre chamadas `draw_sprite()` ainda
//     pode crescer memória sem limite. `Draw2d::draw_sprite()` (draw2d.cpp) drena a fila pronta
//     após TODA chamada -- espelhe essa mesma disciplina de dreno em qualquer outro chamador
//     (inclusive o próprio teste deste header).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <glintfx/draw2d.hpp>

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace glintfx::draw2d_detail {

// EN: One GL vertex, laid out to match draw2d.cpp's VAO attribute pointers 1:1 (position @0, uv
//     @1, colour @2 -- see CreateGlQuadBuffers/decorator_gl_quad.hpp for the sibling idiom this
//     mirrors, minus the vertex-colour attribute that header deliberately omits and this one
//     deliberately adds, since Draw2D's tint is per-sprite, not per-decorator-instance-uniform).
//     All-float, naturally packed (no padding), sizeof(Vertex) == 32 bytes.
// PT: Um vértice GL, layout batendo 1:1 com os attribute pointers da VAO de draw2d.cpp (posição
//     @0, uv @1, cor @2 -- ver CreateGlQuadBuffers/decorator_gl_quad.hpp pro idioma irmão que
//     isto espelha, menos o atributo de cor-de-vértice que aquele header omite de propósito e
//     este adiciona de propósito, já que o tint do Draw2D é por-sprite, não uniform
//     por-instância-de-decorator). Tudo float, naturalmente empacotado (sem padding),
//     sizeof(Vertex) == 32 bytes.
struct Vertex {
  float x = 0.f, y = 0.f;             // screen px, top-left origin, y-down (D5).
  float u = 0.f, v = 0.f;             // texel-normalized [0,1], y-down (matches image row order, D5).
  float r = 1.f, g = 1.f, b = 1.f, a = 1.f; // straight tint, already validated/clamped (D8/D10).
};

// EN: One drained run, ready for GL execution -- owns its own vertex data (a snapshot, not a view
//     into a shared buffer) so the caller can hold it across the internal buffer being cleared
//     and reused for the next run.
// PT: Uma corrida drenada, pronta pra execução GL -- é dona do próprio dado de vértice (um
//     snapshot, não uma view sobre um buffer compartilhado) para que o chamador possa segurá-la
//     enquanto o buffer interno é limpo e reusado pra próxima corrida.
struct Flush {
  std::uint32_t texture_id = 0; // opaque, carried through unchanged -- Draw2d resolves this to a GL name.
  std::vector<Vertex> vertices; // always a multiple of 6 (2 triangles per quad).
};

// EN: Pure per-texture sprite batcher (D4). No GL call anywhere in this class.
// PT: Batcher de sprite puro, por-textura (D4). Nenhuma chamada GL em nenhum lugar desta classe.
class SpriteBatch {
public:
  explicit SpriteBatch(std::size_t max_quads_per_flush = 4096)
      : max_quads_(max_quads_per_flush > 0 ? max_quads_per_flush : 1) {}

  // EN: Opens a bracket. Returns false when a PREVIOUS bracket was still open (nested begin, D4)
  //     -- the previous one's pending run is still finalized into the ready queue by this same
  //     call (implicit end()), so `Draw2d::begin()` can drain it (using the OLD target size)
  //     before switching its own target-size bookkeeping to the new values; returns true
  //     otherwise. `target_w <= 0 || target_h <= 0` marks this bracket degenerate (D10): every
  //     draw_sprite() call until the matching end() returns DrawResult::BracketInvalid without
  //     touching any state.
  // PT: Abre um bracket. Retorna false quando um bracket ANTERIOR ainda estava aberto (begin
  //     aninhado, D4) -- a corrida pendente do anterior ainda é finalizada na fila pronta por
  //     esta mesma chamada (end() implícito), para que `Draw2d::begin()` possa drená-la (usando o
  //     tamanho-alvo ANTIGO) antes de trocar a própria contabilidade de tamanho-alvo pros valores
  //     novos; retorna true caso contrário. `target_w <= 0 || target_h <= 0` marca este bracket
  //     degenerado (D10): toda chamada draw_sprite() até o end() correspondente retorna
  //     DrawResult::BracketInvalid sem tocar em nenhum estado.
  bool begin(int target_w, int target_h) {
    bool clean = true;
    if (in_bracket_) {
      clean = false;
      finalize_current_run();
    }
    in_bracket_ = true;
    bracket_degenerate_ = (target_w <= 0 || target_h <= 0);
    has_current_texture_ = false;
    current_texture_id_ = 0;
    current_vertices_.clear();
    return clean;
  }

  enum class DrawResult {
    Ok,              // queued (possibly after an internal texture-change or capacity flush).
    OutsideBracket,  // no begin() in effect (or already end()-ed) -- D4.
    BracketInvalid,  // this bracket's begin() had target_w/h<=0 -- D10, silent per-call (already
                      // logged once at begin() by the caller).
    NonFinite,       // dst/src_px/tint carried a NaN/Inf member -- D10, checked before any math.
    Degenerate,      // dst.w<=0 || dst.h<=0 -- a legal no-op sprite, not an error (D10).
  };

  // EN: `tex_w`/`tex_h` are the texture's OWN pixel dimensions (the caller has ALREADY validated
  //     `texture_id` against its registry before calling this -- this class never rejects a
  //     texture id itself, only the geometry/colour it is handed). Resolves the `{0,0,0,0}`
  //     "whole texture" `src_px` sentinel and clamps any other `src_px` to `[0,tex_w]x[0,tex_h]`
  //     (D5/D10) before generating vertices.
  // PT: `tex_w`/`tex_h` são as próprias dimensões em pixel da textura (o chamador JÁ validou
  //     `texture_id` contra o próprio registry antes de chamar isto -- esta classe nunca rejeita
  //     um id de textura em si, só a geometria/cor que recebe). Resolve a sentinela `{0,0,0,0}`
  //     de `src_px` "textura inteira" e clampa qualquer outro `src_px` a `[0,tex_w]x[0,tex_h]`
  //     (D5/D10) antes de gerar vértices.
  DrawResult draw_sprite(std::uint32_t texture_id, int tex_w, int tex_h, const RectF& dst,
                          const RectF& src_px, const ColorF& tint) {
    if (!in_bracket_) return DrawResult::OutsideBracket;
    if (bracket_degenerate_) return DrawResult::BracketInvalid;

    // D10: validate every float BEFORE any cast or arithmetic touches it.
    if (!all_finite(dst) || !all_finite(src_px) || !all_finite(tint)) return DrawResult::NonFinite;
    if (dst.w <= 0.f || dst.h <= 0.f) return DrawResult::Degenerate;

    // D4: flush the run so far on a texture change (first draw of the bracket counts as one).
    if (!has_current_texture_ || current_texture_id_ != texture_id) {
      finalize_current_run();
      current_texture_id_ = texture_id;
      has_current_texture_ = true;
    }

    const float tw = tex_w > 0 ? static_cast<float>(tex_w) : 0.f;
    const float th = tex_h > 0 ? static_cast<float>(tex_h) : 0.f;
    const RectF src = resolve_src(src_px, tw, th);
    const ColorF tint_clamped{clampf(tint.r, 0.f, 1.f), clampf(tint.g, 0.f, 1.f),
                               clampf(tint.b, 0.f, 1.f), clampf(tint.a, 0.f, 1.f)};

    emit_quad(dst, src, tint_clamped, tw, th);

    // D4: capacity flush -- bounds ONE run's CPU memory against a hostile/degenerate stream
    // (this is a memory bound, not a performance claim -- D2D-3 measures throughput later).
    // Keeps the SAME texture for the next run: a hostile stream that never changes texture
    // still batches maximally, it just flushes every max_quads_ quads instead of once.
    if (current_vertices_.size() / 6 >= max_quads_) {
      finalize_current_run();
      current_texture_id_ = texture_id;
      has_current_texture_ = true;
    }

    return DrawResult::Ok;
  }

  // EN: Closes the bracket, finalizing whatever run was pending (D4) -- unconditionally, even an
  //     empty one (finalize_current_run() is itself a no-op on an empty run, so end() on an
  //     untouched bracket queues nothing). Safe to call without an open bracket (no-op).
  // PT: Fecha o bracket, finalizando a corrida que estava pendente (D4) -- incondicionalmente,
  //     mesmo uma vazia (finalize_current_run() já é no-op numa corrida vazia, então end() num
  //     bracket intocado não enfileira nada). Seguro chamar sem bracket aberto (no-op).
  void end() {
    if (!in_bracket_) return;
    finalize_current_run();
    in_bracket_ = false;
    bracket_degenerate_ = false;
  }

  // EN: True when at least one Flush is waiting to be drained.
  // PT: True quando ao menos um Flush está esperando pra ser drenado.
  bool has_ready() const { return !ready_.empty(); }

  // EN: Drains and returns every ready Flush, in the order they were finalized (texture-change/
  //     capacity/end() order) -- the queue is empty after this call. See this file's header
  //     comment for why the CALLER must drain regularly to keep memory bounded under a hostile
  //     stream.
  // PT: Drena e retorna todo Flush pronto, na ordem em que foram finalizados (ordem de
  //     troca-de-textura/capacidade/end()) -- a fila fica vazia depois desta chamada. Ver o
  //     comentário de cabeçalho deste arquivo pro porquê do CHAMADOR precisar drenar
  //     regularmente pra manter memória limitada sob um stream hostil.
  std::vector<Flush> take_ready() {
    std::vector<Flush> out = std::move(ready_);
    ready_.clear();
    return out;
  }

  // EN: Test/introspection helpers -- not used by draw2d.cpp itself.
  // PT: Helpers de teste/introspecção -- não usados pelo próprio draw2d.cpp.
  bool in_bracket() const { return in_bracket_; }
  std::size_t max_quads() const { return max_quads_; }

private:
  static bool all_finite(const RectF& r) {
    return std::isfinite(r.x) && std::isfinite(r.y) && std::isfinite(r.w) && std::isfinite(r.h);
  }
  static bool all_finite(const ColorF& c) {
    return std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b) && std::isfinite(c.a);
  }
  static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

  // EN: D5/D10 -- resolves the {0,0,0,0} "whole texture" sentinel, then clamps to
  //     [0,tex_w]x[0,tex_h] (an out-of-bounds src_px is a documented legitimate case -- animation
  //     math jitter on the edge of an atlas frame -- not an error).
  // PT: D5/D10 -- resolve a sentinela {0,0,0,0} de "textura inteira", depois clampa a
  //     [0,tex_w]x[0,tex_h] (um src_px fora de faixa é um caso legítimo documentado -- jitter de
  //     matemática de animação na borda de um frame de atlas -- não um erro).
  static RectF resolve_src(const RectF& src_px, float tex_w, float tex_h) {
    const bool sentinel = (src_px.x == 0.f && src_px.y == 0.f && src_px.w == 0.f && src_px.h == 0.f);
    if (sentinel) return RectF{0.f, 0.f, tex_w, tex_h};
    const float sx = clampf(src_px.x, 0.f, tex_w);
    const float sy = clampf(src_px.y, 0.f, tex_h);
    const float sw = clampf(src_px.w, 0.f, tex_w - sx);
    const float sh = clampf(src_px.h, 0.f, tex_h - sy);
    return RectF{sx, sy, sw, sh};
  }

  // EN: D5 vertex generation -- CW winding from top-left (position AND uv), same convention as
  //     decorator_gl_quad.hpp's BuildQuadCorners/CreateGlQuadBuffers, 2 triangles (TL,TR,BR /
  //     TL,BR,BL), no index buffer (draw2d.cpp issues a plain glDrawArrays(GL_TRIANGLES, ...)).
  // PT: Geração de vértice D5 -- giro CW a partir do topo-esquerda (posição E uv), mesma
  //     convenção de BuildQuadCorners/CreateGlQuadBuffers de decorator_gl_quad.hpp, 2 triângulos
  //     (TL,TR,BR / TL,BR,BL), sem index buffer (draw2d.cpp emite um
  //     glDrawArrays(GL_TRIANGLES, ...) puro).
  void emit_quad(const RectF& dst, const RectF& src, const ColorF& tint, float tex_w, float tex_h) {
    const float u0 = tex_w > 0.f ? src.x / tex_w : 0.f;
    const float v0 = tex_h > 0.f ? src.y / tex_h : 0.f;
    const float u1 = tex_w > 0.f ? (src.x + src.w) / tex_w : 0.f;
    const float v1 = tex_h > 0.f ? (src.y + src.h) / tex_h : 0.f;

    const Vertex tl{dst.x, dst.y, u0, v0, tint.r, tint.g, tint.b, tint.a};
    const Vertex tr{dst.x + dst.w, dst.y, u1, v0, tint.r, tint.g, tint.b, tint.a};
    const Vertex br{dst.x + dst.w, dst.y + dst.h, u1, v1, tint.r, tint.g, tint.b, tint.a};
    const Vertex bl{dst.x, dst.y + dst.h, u0, v1, tint.r, tint.g, tint.b, tint.a};

    current_vertices_.push_back(tl);
    current_vertices_.push_back(tr);
    current_vertices_.push_back(br);
    current_vertices_.push_back(tl);
    current_vertices_.push_back(br);
    current_vertices_.push_back(bl);
  }

  void finalize_current_run() {
    if (!current_vertices_.empty())
      ready_.push_back(Flush{current_texture_id_, std::move(current_vertices_)});
    current_vertices_.clear();
    has_current_texture_ = false;
  }

  bool in_bracket_ = false;
  bool bracket_degenerate_ = false;
  bool has_current_texture_ = false;
  std::uint32_t current_texture_id_ = 0;
  std::size_t max_quads_;
  std::vector<Vertex> current_vertices_;
  std::vector<Flush> ready_;
};

} // namespace glintfx::draw2d_detail
