// SPDX-License-Identifier: MPL-2.0
// EN: sprite_batch_sanity (D2D-1B) -- headless unit test for the PURE batching policy in
//     src/sprite_batch.hpp (plan docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md,
//     decisions D4/D5/D8/D10). No GL, no Xvfb, no window -- runs in BOTH build configs, same
//     class of test as image_decode_sanity.cpp/log_dedup_sanity.cpp/input_state_sanity.cpp.
//     Covers: same-texture accumulation, flush on texture-change/capacity/end(), vertex
//     generation (positions from dst, UVs from src_px including the whole-texture sentinel and
//     out-of-bounds clamp), tint clamping, NaN/Inf rejection BEFORE any vertex math, degenerate
//     dst as a silent no-op, draw-outside-bracket, nested-begin, and a 100k-draw hostile stream
//     staying memory-bounded when the caller drains take_ready() after every call (the same
//     discipline draw2d.cpp itself follows -- see sprite_batch.hpp's own header comment on why
//     the ready queue is NOT bounded on its own).
// PT: sprite_batch_sanity (D2D-1B) -- teste unitário headless para a política de batching PURA
//     em src/sprite_batch.hpp (plano docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md,
//     decisões D4/D5/D8/D10). Sem GL, sem Xvfb, sem janela -- roda em AMBAS as configs de build,
//     mesma classe de teste de image_decode_sanity.cpp/log_dedup_sanity.cpp/
//     input_state_sanity.cpp. Cobre: acumulação por-mesma-textura, flush em
//     troca-de-textura/capacidade/end(), geração de vértice (posições de dst, UVs de src_px
//     incluindo a sentinela de textura-inteira e clamp fora-de-faixa), clamp de tint, rejeição de
//     NaN/Inf ANTES de qualquer matemática de vértice, dst degenerado como no-op silencioso,
//     draw-fora-de-bracket, begin aninhado, e um stream hostil de 100k desenhos permanecendo
//     limitado em memória quando o chamador drena take_ready() a cada chamada (a mesma
//     disciplina que o próprio draw2d.cpp segue -- ver o comentário de cabeçalho de
//     sprite_batch.hpp pro porquê da fila pronta NÃO ser limitada sozinha).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/sprite_batch.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

using glintfx::ColorF;
using glintfx::RectF;
using glintfx::draw2d_detail::Flush;
using glintfx::draw2d_detail::SpriteBatch;
using glintfx::draw2d_detail::Vertex;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

// EN: All 6 vertices of one quad, in CreateGlQuadBuffers/BuildQuadCorners winding
//     (TL,TR,BR,TL,BR,BL, D5).
// PT: Os 6 vértices de um quad, no giro de CreateGlQuadBuffers/BuildQuadCorners
//     (TL,TR,BR,TL,BR,BL, D5).
bool quad_matches(const Vertex* v, float x, float y, float w, float h, float u0, float v0,
                   float u1, float v1) {
  const bool tl_ok = near(v[0].x, x) && near(v[0].y, y) && near(v[0].u, u0) && near(v[0].v, v0);
  const bool tr_ok = near(v[1].x, x + w) && near(v[1].y, y) && near(v[1].u, u1) && near(v[1].v, v0);
  const bool br_ok = near(v[2].x, x + w) && near(v[2].y, y + h) && near(v[2].u, u1) && near(v[2].v, v1);
  const bool tl2_ok = near(v[3].x, x) && near(v[3].y, y) && near(v[3].u, u0) && near(v[3].v, v0);
  const bool br2_ok = near(v[4].x, x + w) && near(v[4].y, y + h) && near(v[4].u, u1) && near(v[4].v, v1);
  const bool bl_ok = near(v[5].x, x) && near(v[5].y, y + h) && near(v[5].u, u0) && near(v[5].v, v1);
  return tl_ok && tr_ok && br_ok && tl2_ok && br2_ok && bl_ok;
}

void test_same_texture_accumulates() {
  SpriteBatch b;
  b.begin(100, 100);
  const RectF dst{0, 0, 10, 10};
  check(b.draw_sprite(1, 10, 10, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::Ok,
        "same_texture_accumulates: draw 1 Ok");
  check(b.draw_sprite(1, 10, 10, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::Ok,
        "same_texture_accumulates: draw 2 Ok");
  check(b.draw_sprite(1, 10, 10, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::Ok,
        "same_texture_accumulates: draw 3 Ok");
  check(!b.has_ready(), "same_texture_accumulates: no flush before end() (same texture)");
  b.end();
  auto flushes = b.take_ready();
  check(flushes.size() == 1, "same_texture_accumulates: exactly 1 flush after end()");
  if (flushes.size() == 1) {
    check(flushes[0].texture_id == 1, "same_texture_accumulates: flush texture_id");
    check(flushes[0].vertices.size() == 18, "same_texture_accumulates: 3 quads = 18 vertices");
  }
}

void test_texture_change_flushes() {
  SpriteBatch b;
  b.begin(100, 100);
  const RectF dst{0, 0, 10, 10};
  b.draw_sprite(1, 10, 10, dst, RectF{}, ColorF{});
  check(!b.has_ready(), "texture_change_flushes: nothing ready after 1st draw (still texture 1)");
  b.draw_sprite(2, 10, 10, dst, RectF{}, ColorF{});
  check(b.has_ready(), "texture_change_flushes: texture change flushed texture 1's run");
  auto first = b.take_ready();
  check(first.size() == 1 && first[0].texture_id == 1 && first[0].vertices.size() == 6,
        "texture_change_flushes: drained run belongs to texture 1, one quad");
  b.end();
  auto second = b.take_ready();
  check(second.size() == 1 && second[0].texture_id == 2 && second[0].vertices.size() == 6,
        "texture_change_flushes: end() flushes texture 2's run, one quad");
}

void test_capacity_flush() {
  SpriteBatch b(4); // small capacity so the test stays fast.
  b.begin(100, 100);
  const RectF dst{0, 0, 1, 1};
  for (int i = 0; i < 4; ++i)
    b.draw_sprite(7, 10, 10, dst, RectF{}, ColorF{});
  check(b.has_ready(), "capacity_flush: reaching max_quads() flushed internally");
  auto first = b.take_ready();
  check(first.size() == 1 && first[0].vertices.size() == 24 && first[0].texture_id == 7,
        "capacity_flush: first drained run is exactly max_quads() quads, same texture");
  b.draw_sprite(7, 10, 10, dst, RectF{}, ColorF{});
  b.draw_sprite(7, 10, 10, dst, RectF{}, ColorF{});
  check(!b.has_ready(), "capacity_flush: below capacity again, nothing ready yet");
  b.end();
  auto second = b.take_ready();
  check(second.size() == 1 && second[0].vertices.size() == 12,
        "capacity_flush: end() flushes the remaining 2 quads");
}

void test_vertex_generation_whole_texture_sentinel() {
  SpriteBatch b;
  b.begin(100, 100);
  const RectF dst{10, 20, 30, 40};
  check(b.draw_sprite(1, 64, 32, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::Ok,
        "vertex_generation_whole_texture_sentinel: draw Ok");
  b.end();
  auto flushes = b.take_ready();
  check(flushes.size() == 1 && flushes[0].vertices.size() == 6,
        "vertex_generation_whole_texture_sentinel: one quad");
  if (flushes.size() == 1 && flushes[0].vertices.size() == 6) {
    check(quad_matches(flushes[0].vertices.data(), 10, 20, 30, 40, 0.f, 0.f, 1.f, 1.f),
          "vertex_generation_whole_texture_sentinel: {0,0,0,0} src_px maps to full [0,1] UV");
  }
}

void test_vertex_generation_src_px_clamp() {
  SpriteBatch b;
  b.begin(100, 100);
  const RectF dst{0, 0, 8, 8};
  // tex is 64x32; src_px={50,20,30,30} overflows both w (50+30=80>64) and h (20+30=50>32).
  const RectF src{50.f, 20.f, 30.f, 30.f};
  b.draw_sprite(1, 64, 32, dst, src, ColorF{});
  b.end();
  auto flushes = b.take_ready();
  check(flushes.size() == 1 && flushes[0].vertices.size() == 6,
        "vertex_generation_src_px_clamp: one quad");
  if (flushes.size() == 1 && flushes[0].vertices.size() == 6) {
    // Expected clamp: sx=50, sy=20, sw=min(30,64-50)=14, sh=min(30,32-20)=12.
    // u0=50/64, v0=20/32, u1=64/64=1.0, v1=32/32=1.0.
    const float u0 = 50.f / 64.f, v0 = 20.f / 32.f, u1 = 1.f, v1 = 1.f;
    check(quad_matches(flushes[0].vertices.data(), 0, 0, 8, 8, u0, v0, u1, v1),
          "vertex_generation_src_px_clamp: out-of-bounds src_px clamped to texture bounds");
  }
}

void test_tint_clamp() {
  SpriteBatch b;
  b.begin(10, 10);
  const RectF dst{0, 0, 1, 1};
  const ColorF hostile_tint{-1.f, 2.f, 0.5f, 5.f};
  b.draw_sprite(1, 4, 4, dst, RectF{}, hostile_tint);
  b.end();
  auto flushes = b.take_ready();
  check(flushes.size() == 1 && flushes[0].vertices.size() == 6, "tint_clamp: one quad");
  if (flushes.size() == 1 && flushes[0].vertices.size() == 6) {
    const Vertex& v0 = flushes[0].vertices[0];
    check(near(v0.r, 0.f) && near(v0.g, 1.f) && near(v0.b, 0.5f) && near(v0.a, 1.f),
          "tint_clamp: each channel clamped independently to [0,1]");
  }
}

void test_nan_rejection() {
  SpriteBatch b;
  b.begin(10, 10);
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  const RectF good_dst{0, 0, 1, 1};

  check(b.draw_sprite(1, 4, 4, RectF{nan, 0, 1, 1}, RectF{}, ColorF{}) ==
            SpriteBatch::DrawResult::NonFinite,
        "nan_rejection: NaN in dst.x rejected");
  check(b.draw_sprite(1, 4, 4, good_dst, RectF{inf, 0, 1, 1}, ColorF{}) ==
            SpriteBatch::DrawResult::NonFinite,
        "nan_rejection: Inf in src_px.x rejected");
  check(b.draw_sprite(1, 4, 4, good_dst, RectF{}, ColorF{nan, 1, 1, 1}) ==
            SpriteBatch::DrawResult::NonFinite,
        "nan_rejection: NaN in tint.r rejected");
  check(!b.has_ready(), "nan_rejection: no vertex math happened -- nothing queued");
  b.end();
  check(b.take_ready().empty(), "nan_rejection: end() on an all-rejected bracket flushes nothing");
}

void test_degenerate_dst_noop() {
  SpriteBatch b;
  b.begin(10, 10);
  check(b.draw_sprite(1, 4, 4, RectF{0, 0, 0, 5}, RectF{}, ColorF{}) ==
            SpriteBatch::DrawResult::Degenerate,
        "degenerate_dst_noop: dst.w<=0 is Degenerate");
  check(b.draw_sprite(1, 4, 4, RectF{0, 0, 5, 0}, RectF{}, ColorF{}) ==
            SpriteBatch::DrawResult::Degenerate,
        "degenerate_dst_noop: dst.h<=0 is Degenerate");
  // D10: negative dst.x/y is LEGAL, not degenerate -- must still queue.
  check(b.draw_sprite(1, 4, 4, RectF{-50, -50, 5, 5}, RectF{}, ColorF{}) ==
            SpriteBatch::DrawResult::Ok,
        "degenerate_dst_noop: negative dst.x/dst.y is legal, still queues");
}

void test_outside_bracket() {
  SpriteBatch b;
  const RectF dst{0, 0, 1, 1};
  check(b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::OutsideBracket,
        "outside_bracket: draw before any begin()");
  b.begin(10, 10);
  b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{});
  b.end();
  check(b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::OutsideBracket,
        "outside_bracket: draw after end()");
}

void test_bracket_invalid_target() {
  SpriteBatch b;
  const RectF dst{0, 0, 1, 1};
  b.begin(0, 50);
  check(b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::BracketInvalid,
        "bracket_invalid_target: target_w<=0 makes the bracket a no-op");
  b.end();
  check(b.take_ready().empty(), "bracket_invalid_target: nothing was ever queued");

  b.begin(50, -1);
  check(b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{}) == SpriteBatch::DrawResult::BracketInvalid,
        "bracket_invalid_target: target_h<=0 makes the bracket a no-op");
  b.end();
}

void test_nested_begin() {
  SpriteBatch b;
  const RectF dst{0, 0, 1, 1};
  b.begin(100, 100);
  b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{});
  const bool clean = b.begin(200, 200); // implicit end() of the first bracket, D4.
  check(!clean, "nested_begin: begin() while a bracket is open returns false");
  check(b.has_ready(), "nested_begin: the first bracket's pending run was finalized");
  auto leftover = b.take_ready();
  check(leftover.size() == 1 && leftover[0].texture_id == 1 && leftover[0].vertices.size() == 6,
        "nested_begin: leftover run intact after the implicit end()");
  b.draw_sprite(2, 4, 4, dst, RectF{}, ColorF{});
  b.end();
  auto second = b.take_ready();
  check(second.size() == 1 && second[0].texture_id == 2,
        "nested_begin: the new bracket batches independently after the implicit close");
}

void test_hostile_100k_bounded() {
  SpriteBatch b; // default capacity, 4096.
  const RectF dst{0, 0, 1, 1};
  const std::size_t total_draws = 100000;
  std::size_t total_vertices_drained = 0;
  bool capacity_respected = true;

  b.begin(100, 100);
  for (std::size_t i = 0; i < total_draws; ++i) {
    b.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{});
    // Same discipline draw2d.cpp itself follows -- drain after EVERY call (sprite_batch.hpp's
    // own header comment: the ready queue is unbounded unless the caller drains it).
    for (const Flush& f : b.take_ready()) {
      if (f.vertices.size() > b.max_quads() * 6) capacity_respected = false;
      total_vertices_drained += f.vertices.size();
    }
  }
  b.end();
  for (const Flush& f : b.take_ready()) {
    if (f.vertices.size() > b.max_quads() * 6) capacity_respected = false;
    total_vertices_drained += f.vertices.size();
  }

  check(capacity_respected, "hostile_100k_bounded: no single flush ever exceeded max_quads()");
  check(total_vertices_drained == total_draws * 6,
        "hostile_100k_bounded: every draw's vertices were eventually accounted for");
}

} // namespace

int main() {
  test_same_texture_accumulates();
  test_texture_change_flushes();
  test_capacity_flush();
  test_vertex_generation_whole_texture_sentinel();
  test_vertex_generation_src_px_clamp();
  test_tint_clamp();
  test_nan_rejection();
  test_degenerate_dst_noop();
  test_outside_bracket();
  test_bracket_invalid_target();
  test_nested_begin();
  test_hostile_100k_bounded();

  if (g_failures == 0) {
    std::puts("sprite_batch_sanity: PASS");
    return 0;
  }
  std::printf("sprite_batch_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
