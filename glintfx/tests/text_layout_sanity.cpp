// SPDX-License-Identifier: MPL-2.0
// EN: text_layout_sanity (D2D-TEXT, T1b, Onda 6 -- docs/superpowers/plans/2026-07-24-onda6-draw2d-
//     text.md, TX2 (b), TX15 word-wrap, TX16 align/justify, TX17's hand-computed-value gate,
//     layout half) -- PURE unit test for src/text_layout.hpp: no window, no GL context, no
//     RmlUi/GLFW dependency, and -- unlike text_raster_sanity -- NO font and NO sovereign-core link
//     whatsoever. text_layout.hpp is pure positioning math over the frozen GlyphAdvanceSource
//     (text_raster.hpp:225); this test feeds it a MONOSPACE MOCK of that interface (every advance a
//     known constant, kern 0, fixed ascent/line-height) so every expected break, offset and pen
//     position below is a LITERAL number computed BY HAND in the test source from TX15/TX16's own
//     formulas -- never by calling the code under test and comparing it to itself (the TX17 "a test
//     that cannot distinguish the mutation does not exist" discipline). Because the fixture is
//     monospace with integer advances, every expected pen_x/pen_y is EXACTLY representable in
//     IEEE754 float and asserted with `==`, no epsilon (a sign flip, a swapped Center/Right, or an
//     off-by-one gap count in justify cannot slip through a fuzzy compare).
//
//     LINE IDENTITY VIA pen_y: LayoutResult.glyphs is a FLAT reading-order list; a glyph's line is
//     read off its pen_y (baseline of line i == ascent + i*line_height). With ascent=8,
//     line_height=12 the fixture's line baselines are 8, 20, 32, ... -- the assertions below index
//     the flat list in reading order and check {gid, pen_x, pen_y} against the hand table, so line
//     membership, wrap points and vertical stacking are all pinned by the same literal checks.
//
//     MUTATIONS THIS FILE KILLS (the section-4 gate, layout half): the word-overflow break lands at
//     the hand-computed glyph (test_wrap_word_overflow / test_forcebreak_*); Center's offset
//     differs from Right's by construction (test_center vs test_right -- swapping the two formulas
//     fails LOUD, 10 vs 20 on the same line); justify distributes +5/+10 across exactly 2 gaps
//     (test_justify_distributes -- counting 3 gaps yields 10/3 != 5, an off-by-one fails); the
//     block's last line and any '\n'-terminated line are NOT justified (test_justify_last_line_*);
//     and kern is folded into layout identically to advance (test_kern_applied -- dropping kern
//     moves V from 7 to 10).
// PT: text_layout_sanity (D2D-TEXT, T1b, Onda 6 -- docs/superpowers/plans/2026-07-24-onda6-draw2d-
//     text.md, TX2 (b), word-wrap TX15, align/justify TX16, gate de valor-à-mão do TX17, metade de
//     layout) -- teste unit PURO para src/text_layout.hpp: sem janela, sem contexto GL, sem
//     dependência de RmlUi/GLFW, e -- diferente do text_raster_sanity -- SEM fonte e SEM link do
//     núcleo soberano de espécie nenhuma. O text_layout.hpp é matemática pura de posicionamento
//     sobre a GlyphAdvanceSource congelada (text_raster.hpp:225); este teste alimenta um MOCK
//     MONOESPAÇADO daquela interface (todo avanço uma constante conhecida, kern 0, ascent/line-
//     height fixos) pra que toda quebra, offset e posição de pena esperada abaixo seja um número
//     LITERAL computado À MÃO no fonte do teste a partir das próprias fórmulas do TX15/TX16 --
//     nunca chamando o código sob teste e comparando com ele mesmo (a disciplina do TX17 "teste que
//     não distingue a mutação não existe"). Como a fixture é monoespaçada com avanços inteiros, todo
//     pen_x/pen_y esperado é EXATAMENTE representável em float IEEE754 e testado com `==`, sem
//     epsilon (um sinal trocado, um Center/Right permutado, ou um off-by-one na contagem de gaps do
//     justify não escapa por uma comparação difusa).
//
//     IDENTIDADE DE LINHA VIA pen_y: LayoutResult.glyphs é uma lista PLANA em ordem de leitura; a
//     linha de um glifo é lida do pen_y dele (baseline da linha i == ascent + i*line_height). Com
//     ascent=8, line_height=12 as baselines da fixture são 8, 20, 32, ... -- as asserções abaixo
//     indexam a lista plana em ordem de leitura e checam {gid, pen_x, pen_y} contra a tabela à mão,
//     então pertinência de linha, pontos de quebra e empilhamento vertical ficam todos fixados
//     pelos mesmos cheques literais.
//
//     MUTAÇÕES QUE ESTE ARQUIVO MATA (o gate da seção 4, metade de layout): a quebra por estouro de
//     palavra cai no glifo computado à mão; o offset do Center difere do Right por construção
//     (10 vs 20, permutá-los falha ALTO); o justify distribui +5/+10 entre exatamente 2 gaps
//     (contar 3 gaps dá 10/3 != 5, off-by-one falha); a última linha do bloco e qualquer linha
//     terminada por '\n' NÃO justificam; e o kern entra no layout idêntico ao avanço (tirá-lo move
//     o V de 7 pra 10).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/text_layout.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

using glintfx::GlyphAdvanceSource;
using glintfx::text_detail::Align;
using glintfx::text_detail::layout_text;
using glintfx::text_detail::LayoutResult;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

// ---------------------------------------------------------------------------
// EN: The monospace mock -- the cheapest mutation-killer there is (plan section 4): every glyph
//     advances by exactly `adv`, kern is 0, ascent/line-height are fixed. glyph_id is identity
//     (gid == codepoint) so the assertions can name gids by their ASCII/codepoint value.
// PT: O mock monoespaçado -- o mata-mutante mais barato que existe (seção 4 do plano): todo glifo
//     avança exatamente `adv`, kern 0, ascent/line-height fixos. glyph_id é identidade (gid ==
//     codepoint) pra que as asserções nomeiem gids pelo valor ASCII/codepoint.
// ---------------------------------------------------------------------------
struct MonoCtx {
  float adv = 10.f;
  float ascent = 8.f;
  float line_height = 12.f;
};
uint32_t mono_glyph_id(const void*, uint32_t cp) { return cp; }
float mono_advance(const void* c, uint32_t) { return static_cast<const MonoCtx*>(c)->adv; }
float mono_kern(const void*, uint32_t, uint32_t) { return 0.f; }
float mono_ascent(const void* c) { return static_cast<const MonoCtx*>(c)->ascent; }
float mono_line_height(const void* c) { return static_cast<const MonoCtx*>(c)->line_height; }

GlyphAdvanceSource make_mono(const MonoCtx& m) {
  GlyphAdvanceSource s;
  s.ctx = &m;
  s.glyph_id = mono_glyph_id;
  s.advance_px = mono_advance;
  s.kern_px = mono_kern;
  s.ascent_px = mono_ascent;
  s.line_height_px = mono_line_height;
  return s;
}

// EN: A proportional mock with ONE nonzero kern pair (gid 'A'->'V' == -3, else 0) -- isolates that
//     layout folds kern into the pen exactly like advance (test_kern_applied).
// PT: Um mock proporcional com UM par de kern não-zero (gid 'A'->'V' == -3, senão 0) -- isola que o
//     layout embute o kern na pena exatamente como o avanço (test_kern_applied).
float kern_AV(const void*, uint32_t l, uint32_t r) { return (l == 'A' && r == 'V') ? -3.f : 0.f; }
GlyphAdvanceSource make_kern_mock(const MonoCtx& m) {
  GlyphAdvanceSource s = make_mono(m);
  s.kern_px = kern_AV;
  return s;
}

// EN: Assert the k-th emitted glyph (reading order) is exactly {gid, pen_x, pen_y}. Exact `==`:
//     every fixture value is an integer sum, IEEE754-exact.
// PT: Testa que o k-ésimo glifo emitido (ordem de leitura) é exatamente {gid, pen_x, pen_y}. `==`
//     exato: todo valor da fixture é uma soma de inteiros, exata em IEEE754.
void expect_glyph(const LayoutResult& r, std::size_t k, uint32_t gid, float x, float y,
                  const char* tag) {
  if (k >= r.glyphs.size()) {
    std::printf("FAIL: %s (index %zu out of range, only %zu glyphs)\n", tag, k, r.glyphs.size());
    ++g_failures;
    return;
  }
  const auto& g = r.glyphs[k];
  check(g.gid == gid && g.pen_x == x && g.pen_y == y, tag);
}

std::vector<uint32_t> cps(std::initializer_list<uint32_t> l) { return std::vector<uint32_t>(l); }

LayoutResult run(const GlyphAdvanceSource& s, const std::vector<uint32_t>& c, float max_width,
                 Align align) {
  return layout_text(s, c.data(), c.size(), max_width, align);
}

// ---------------------------------------------------------------------------
// Baseline geometry: single line + hard '\n' break
// ---------------------------------------------------------------------------
void test_single_line() {
  const MonoCtx m; // adv 10, ascent 8, line_height 12
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'b', 'c'}), 0.f, Align::Left);
  check(r.line_count == 1, "single_line: line_count == 1");
  check(r.width == 30.f && r.height == 12.f, "single_line: width 30, height 12");
  check(r.ascent == 8.f && r.line_height == 12.f, "single_line: echoed face metrics");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "single_line: a@(0,8)");
  expect_glyph(r, 1, 'b', 10.f, 8.f, "single_line: b@(10,8)");
  expect_glyph(r, 2, 'c', 20.f, 8.f, "single_line: c@(20,8)");
}

void test_hard_break() {
  const MonoCtx m;
  const auto s = make_mono(m);
  // "ab\ncd" -> two lines, line1 baseline = ascent + line_height = 20
  const auto r = run(s, cps({'a', 'b', 0x0A, 'c', 'd'}), 0.f, Align::Left);
  check(r.line_count == 2, "hard_break: line_count == 2");
  check(r.width == 20.f && r.height == 24.f, "hard_break: width 20, height 24");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "hard_break: a@(0,8) line0");
  expect_glyph(r, 1, 'b', 10.f, 8.f, "hard_break: b@(10,8) line0");
  expect_glyph(r, 2, 'c', 0.f, 20.f, "hard_break: c@(0,20) line1");
  expect_glyph(r, 3, 'd', 10.f, 20.f, "hard_break: d@(10,20) line1");
}

// ---------------------------------------------------------------------------
// TX15 -- word wrap
// ---------------------------------------------------------------------------
void test_wrap_word_overflow() {
  // adv 10, max_width 50 (== 5*adv). "aaa bb cc": "aaa bb" is 60 > 50, so it breaks after "aaa";
  // "bb cc" is exactly 50 and fits. Lines: ["aaa", "bb cc"] (the plan's own worked example).
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'a', 'a', 0x20, 'b', 'b', 0x20, 'c', 'c'}), 50.f, Align::Left);
  check(r.line_count == 2, "wrap_word_overflow: line_count == 2");
  // line0 "aaa" (baseline 8)
  expect_glyph(r, 0, 'a', 0.f, 8.f, "wrap: aaa a@(0,8)");
  expect_glyph(r, 1, 'a', 10.f, 8.f, "wrap: aaa a@(10,8)");
  expect_glyph(r, 2, 'a', 20.f, 8.f, "wrap: aaa a@(20,8)");
  // line1 "bb cc" (baseline 20): b b <space> c c -> 0,10,20,30,40 (the break space was consumed,
  // this interior space is rendered)
  expect_glyph(r, 3, 'b', 0.f, 20.f, "wrap: bb-cc b@(0,20) -- break landed before 'b'");
  expect_glyph(r, 4, 'b', 10.f, 20.f, "wrap: bb-cc b@(10,20)");
  expect_glyph(r, 5, 0x20, 20.f, 20.f, "wrap: bb-cc space@(20,20) rendered interior");
  expect_glyph(r, 6, 'c', 30.f, 20.f, "wrap: bb-cc c@(30,20)");
  expect_glyph(r, 7, 'c', 40.f, 20.f, "wrap: bb-cc c@(40,20)");
}

void test_forcebreak_long_word() {
  // adv 10, max_width 25 (2 glyphs fit == 20, 3 == 30 > 25). "aaaaa" force-breaks: [aa][aa][a].
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'a', 'a', 'a', 'a'}), 25.f, Align::Left);
  check(r.line_count == 3, "forcebreak_long_word: line_count == 3");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "forcebreak: L0 a@(0,8)");
  expect_glyph(r, 1, 'a', 10.f, 8.f, "forcebreak: L0 a@(10,8)");
  expect_glyph(r, 2, 'a', 0.f, 20.f, "forcebreak: L1 a@(0,20)");
  expect_glyph(r, 3, 'a', 10.f, 20.f, "forcebreak: L1 a@(10,20)");
  expect_glyph(r, 4, 'a', 0.f, 32.f, "forcebreak: L2 a@(0,32)");
}

void test_forcebreak_exact_fit() {
  // adv 10, max_width 20 (exactly 2 glyphs == 20). Single unbreakable word "aaa": the 2nd 'a'
  // lands the line at EXACTLY 20, which the force-break's strict `>` KEEPS (20 > 20 is false), so
  // the break falls before the 3rd 'a'. Lines: ["aa", "a"]. This pins the boundary `>` against the
  // `>=` mutant: with `>=` the 2nd 'a' (20 >= 20 true) would break early, giving ["a","a","a"]
  // (line_count 3, and the glyph at index 1 would sit at (0,20) not (10,8)) -- either assertion
  // fails loud, so the mutation dies here.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'a', 'a'}), 20.f, Align::Left);
  check(r.line_count == 2, "forcebreak_exact_fit: line_count == 2 (exact-fit kept by strict >)");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "forcebreak_exact_fit: L0 a@(0,8)");
  expect_glyph(r, 1, 'a', 10.f, 8.f, "forcebreak_exact_fit: L0 a@(10,8) -- exact fit at 20, kept");
  expect_glyph(r, 2, 'a', 0.f, 20.f, "forcebreak_exact_fit: L1 a@(0,20) -- break before 3rd 'a'");
}

void test_forcebreak_min_one_glyph() {
  // adv 10, max_width 5 (< one glyph). Each glyph still gets a line (min 1), never an infinite loop.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'a'}), 5.f, Align::Left);
  check(r.line_count == 2, "forcebreak_min_one: line_count == 2 (min 1 glyph per line)");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "forcebreak_min_one: L0 a@(0,8)");
  expect_glyph(r, 1, 'a', 0.f, 20.f, "forcebreak_min_one: L1 a@(0,20)");
}

// ---------------------------------------------------------------------------
// TX16 -- alignment
// ---------------------------------------------------------------------------
void test_center() {
  // "abc" (width 30) in max_width 50, Center: offset = (50-30)/2 = 10.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'b', 'c'}), 50.f, Align::Center);
  check(r.line_count == 1, "center: line_count == 1");
  expect_glyph(r, 0, 'a', 10.f, 8.f, "center: a@(10,8) -- (50-30)/2");
  expect_glyph(r, 1, 'b', 20.f, 8.f, "center: b@(20,8)");
  expect_glyph(r, 2, 'c', 30.f, 8.f, "center: c@(30,8)");
}

void test_right() {
  // "abc" (width 30) in max_width 50, Right: offset = 50-30 = 20. DIFFERENT from Center's 10 --
  // swapping the Center/Right formulas fails loud here.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'b', 'c'}), 50.f, Align::Right);
  expect_glyph(r, 0, 'a', 20.f, 8.f, "right: a@(20,8) -- 50-30, NOT the center 10");
  expect_glyph(r, 1, 'b', 30.f, 8.f, "right: b@(30,8)");
  expect_glyph(r, 2, 'c', 40.f, 8.f, "right: c@(40,8)");
}

void test_center_multiline_nowrap() {
  // "abc\nde": widths 30 and 20; no wrap -> ref width = widest line = 30. Center: line0 offset 0,
  // line1 offset (30-20)/2 = 5. Pins "centered multi-line \n works with no wrap".
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'b', 'c', 0x0A, 'd', 'e'}), 0.f, Align::Center);
  check(r.line_count == 2 && r.width == 30.f, "center_multiline: 2 lines, block width 30");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "center_multiline: L0 a@(0,8) -- widest line, offset 0");
  expect_glyph(r, 3, 'd', 5.f, 20.f, "center_multiline: L1 d@(5,20) -- (30-20)/2");
  expect_glyph(r, 4, 'e', 15.f, 20.f, "center_multiline: L1 e@(15,20)");
}

// ---------------------------------------------------------------------------
// TX16 -- justify
// ---------------------------------------------------------------------------
void test_justify_distributes() {
  // "a b c dd" in max_width 60, Justify. Greedy: line0 "a b c" (width 50, soft-wrapped because
  // "dd" would push it to 80 > 60); line1 "dd" (the last line). Line0 has 2 gaps, extra = 60-50 =
  // 10, +5 per gap. Base x: a@0 sp@10 b@20 sp@30 c@40. After distribution: a@0, b@25, c@50 (the
  // last glyph lands the line exactly at ref width 60). Counting 3 gaps would give 10/3 != 5.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 0x20, 'b', 0x20, 'c', 0x20, 'd', 'd'}), 60.f, Align::Justify);
  check(r.line_count == 2, "justify_distributes: line_count == 2");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "justify: L0 a@(0,8)");
  expect_glyph(r, 1, 0x20, 10.f, 8.f, "justify: L0 space@(10,8)");
  expect_glyph(r, 2, 'b', 25.f, 8.f, "justify: L0 b@(25,8) -- +5 (1 gap passed)");
  expect_glyph(r, 3, 0x20, 35.f, 8.f, "justify: L0 space@(35,8) -- +5");
  expect_glyph(r, 4, 'c', 50.f, 8.f, "justify: L0 c@(50,8) -- +10 (2 gaps passed), lands at ref");
  // line1 "dd" is the block's last line -> NOT justified (Left), baseline 20
  expect_glyph(r, 5, 'd', 0.f, 20.f, "justify: L1 d@(0,20) -- last line NOT justified");
  expect_glyph(r, 6, 'd', 10.f, 20.f, "justify: L1 d@(10,20)");
}

void test_justify_single_line_is_last() {
  // "a b c" alone in max_width 60, Justify: it is the block's ONLY (== last) line -> NOT justified.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 0x20, 'b', 0x20, 'c'}), 60.f, Align::Justify);
  check(r.line_count == 1, "justify_single_line: line_count == 1");
  expect_glyph(r, 0, 'a', 0.f, 8.f, "justify_single_line: a@(0,8) not stretched");
  expect_glyph(r, 2, 'b', 20.f, 8.f, "justify_single_line: b@(20,8) not stretched");
  expect_glyph(r, 4, 'c', 40.f, 8.f, "justify_single_line: c@(40,8) not stretched");
}

void test_justify_newline_line_not_justified() {
  // "a b c\ndd" in max_width 60, Justify. "a b c" ends by explicit '\n' -> last line of its
  // paragraph -> NOT justified, distinct from the wrap case. Positions identical to plain Left.
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 0x20, 'b', 0x20, 'c', 0x0A, 'd', 'd'}), 60.f, Align::Justify);
  check(r.line_count == 2, "justify_newline: line_count == 2");
  expect_glyph(r, 2, 'b', 20.f, 8.f, "justify_newline: b@(20,8) -- '\\n' line not justified");
  expect_glyph(r, 4, 'c', 40.f, 8.f, "justify_newline: c@(40,8) not stretched");
}

void test_justify_no_maxwidth_degrades_left() {
  // Justify with max_width 0 has no target width -> degrades to Left (TX16).
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 0x20, 'b', 0x20, 'c'}), 0.f, Align::Justify);
  expect_glyph(r, 2, 'b', 20.f, 8.f, "justify_no_maxwidth: b@(20,8) -- degraded to Left");
  expect_glyph(r, 4, 'c', 40.f, 8.f, "justify_no_maxwidth: c@(40,8)");
}

// ---------------------------------------------------------------------------
// Kern is folded into layout identically to advance
// ---------------------------------------------------------------------------
void test_kern_applied() {
  // kern('A','V') = -3, advance 10. "AV": A@0, then pen += advance(10) + kern(-3) -> V@7.
  const MonoCtx m;
  const auto s = make_kern_mock(m);
  const auto r = run(s, cps({'A', 'V'}), 0.f, Align::Left);
  check(r.width == 17.f, "kern_applied: block width 17 (10 + kern(-3) + 10)");
  expect_glyph(r, 0, 'A', 0.f, 8.f, "kern_applied: A@(0,8)");
  expect_glyph(r, 1, 'V', 7.f, 8.f, "kern_applied: V@(7,8) -- kern folded in (10-3), not 10");
}

// ---------------------------------------------------------------------------
// TX15 -- U+00A0 (nbsp) is a rendered glyph but NOT a break opportunity
// ---------------------------------------------------------------------------
void test_nbsp_not_break_opportunity() {
  // "aaa x yyy" in max_width 45. The wrap opportunity is the U+0020 before 'x' only; the nbsp
  // never breaks the x<nbsp>yyy word. Lines: ["aaa", "x yy", "y"] (the last a force-break of
  // the over-long unbreakable word). The KEY: 'x' (line1, baseline 20) is immediately followed on
  // the SAME line by the nbsp -- proof the nbsp is not a break point (were it one, 'x' could end a
  // line alone as it does with a plain space).
  const MonoCtx m;
  const auto s = make_mono(m);
  const auto r = run(s, cps({'a', 'a', 'a', 0x20, 'x', 0x00A0, 'y', 'y', 'y'}), 45.f, Align::Left);
  check(r.line_count == 3, "nbsp: line_count == 3");
  // line0 "aaa" baseline 8
  expect_glyph(r, 0, 'a', 0.f, 8.f, "nbsp: L0 a@(0,8)");
  // line1 x nbsp y y baseline 20 -- x and nbsp adjacent on the same line
  expect_glyph(r, 3, 'x', 0.f, 20.f, "nbsp: L1 x@(0,20)");
  expect_glyph(r, 4, 0x00A0, 10.f, 20.f, "nbsp: L1 nbsp@(10,20) -- bound to x, NOT a break");
  expect_glyph(r, 5, 'y', 20.f, 20.f, "nbsp: L1 y@(20,20)");
  expect_glyph(r, 6, 'y', 30.f, 20.f, "nbsp: L1 y@(30,20)");
  // line2 last 'y' baseline 32 (force-break of the over-long unbreakable word)
  expect_glyph(r, 7, 'y', 0.f, 32.f, "nbsp: L2 y@(0,32)");
}

// ---------------------------------------------------------------------------
// TX6 -- fail-high totality
// ---------------------------------------------------------------------------
void test_totality() {
  const MonoCtx m;
  const auto s = make_mono(m);
  const std::vector<uint32_t> c = cps({'a', 'b'});

  // not-ok() source (all function pointers null) -> empty, no crash, no null-deref
  GlyphAdvanceSource bad;
  check(!bad.ok(), "totality: default GlyphAdvanceSource is not ok()");
  const auto r_bad = layout_text(bad, c.data(), c.size(), 0.f, Align::Left);
  check(r_bad.line_count == 0 && r_bad.glyphs.empty(), "totality: not-ok source -> empty result");

  // null codepoint pointer -> empty
  const auto r_null = layout_text(s, nullptr, 5, 0.f, Align::Left);
  check(r_null.line_count == 0 && r_null.glyphs.empty(), "totality: null codepoints -> empty");

  // count 0 -> empty
  const auto r_zero = layout_text(s, c.data(), 0, 0.f, Align::Left);
  check(r_zero.line_count == 0 && r_zero.glyphs.empty(), "totality: count 0 -> empty");
}

void test_nonfinite_maxwidth_no_wrap() {
  // A non-finite max_width must NOT wrap and must NOT leak a NaN into any position (every
  // `> max_width` compare against NaN is false -> single line, finite positions).
  const MonoCtx m;
  const auto s = make_mono(m);
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const auto r =
      run(s, cps({'a', 'a', 'a', 0x20, 'b', 'b', 0x20, 'c', 'c'}), nan, Align::Left);
  check(r.line_count == 1, "nonfinite_maxwidth: no wrap -> single line");
  check(r.glyphs.size() == 9, "nonfinite_maxwidth: all 9 glyphs on one line");
  const float last_x = r.glyphs.back().pen_x;
  check(std::isfinite(last_x) && last_x == 80.f, "nonfinite_maxwidth: last glyph finite @80, no NaN");
}

} // namespace

int main() {
  test_single_line();
  test_hard_break();
  test_wrap_word_overflow();
  test_forcebreak_long_word();
  test_forcebreak_exact_fit();
  test_forcebreak_min_one_glyph();
  test_center();
  test_right();
  test_center_multiline_nowrap();
  test_justify_distributes();
  test_justify_single_line_is_last();
  test_justify_newline_line_not_justified();
  test_justify_no_maxwidth_degrades_left();
  test_kern_applied();
  test_nbsp_not_break_opportunity();
  test_totality();
  test_nonfinite_maxwidth_no_wrap();

  if (g_failures == 0) {
    std::puts("text_layout_sanity: PASS");
    return 0;
  }
  std::printf("text_layout_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
