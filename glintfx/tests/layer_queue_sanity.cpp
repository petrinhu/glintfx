// SPDX-License-Identifier: MPL-2.0
// EN: layer_queue_sanity (D2D-3B) -- headless unit test for the PURE buffered-command queue in
//     src/layer_queue.hpp (D27 stable sort, D28 scissor grouping, the 262144 memory cap). No GL,
//     no Xvfb, no window -- runs in BOTH build configs, same class of test as
//     primitives2d_sanity.cpp/transform2d_sanity.cpp/sprite_batch_sanity.cpp. Every command below
//     is tagged with a distinguishing `texture_id` marker (never a real GL texture -- this file
//     never touches GL) so the tests can assert ORDER, not just presence.
//     Covers: stable order WITHIN a layer (equal-layer commands keep push order -- the
//     std::stable_sort mutation killer this file's own header comment names: a plain std::sort
//     offers no such guarantee and could silently permute these); negative and interleaved
//     layers sorting ascending with same-layer sub-order preserved; scissor grouping that splits
//     EXACTLY at snapshot changes IN THE SORTED order (not the original push order -- the
//     interleaved-layers-and-scissors case below is the mutation killer for "grouping happens
//     before sort instead of after": grouping-before-sort would produce 4 alternating groups
//     where grouping-after-sort produces exactly 2); the 262144 cap dropping and reporting past
//     that point while the queue itself never exceeds it; and that drained commands carry back
//     the EXACT corners/texture id/tex dims/src_px/tint they were pushed with (the
//     "camera-at-call-time" pin -- nothing is recomputed at drain time).
// PT: layer_queue_sanity (D2D-3B) -- teste unitário headless pra fila PURA de comandos
//     bufferizados em src/layer_queue.hpp (ordenação estável D27, agrupamento por scissor D28, o
//     teto de memória de 262144). Sem GL, sem Xvfb, sem janela -- roda nas DUAS configs de build,
//     mesma classe de teste de primitives2d_sanity.cpp/transform2d_sanity.cpp/
//     sprite_batch_sanity.cpp. Todo comando abaixo é marcado com um `texture_id` distinguidor
//     (nunca uma textura GL de verdade -- este arquivo nunca toca em GL) pra que os testes possam
//     afirmar ORDEM, não só presença.
//     Cobre: ordem estável DENTRO de uma camada (comandos de mesma camada mantêm a ordem de push
//     -- o mata-mutação do std::stable_sort que o próprio comentário de cabeçalho deste arquivo
//     nomeia: um std::sort simples não oferece essa garantia e poderia permutar isto em
//     silêncio); camadas negativas e intercaladas ordenando ascendente com a sub-ordem de mesma
//     camada preservada; agrupamento por scissor que quebra EXATAMENTE nas mudanças de snapshot
//     na ordem ORDENADA (não na ordem original de push -- o caso de camadas-e-scissors
//     intercalados abaixo é o mata-mutação pra "agrupamento acontece antes da ordenação em vez de
//     depois": agrupar-antes-de-ordenar produziria 4 grupos alternados onde agrupar-depois
//     produz exatamente 2); o teto de 262144 descartando e reportando passado esse ponto enquanto
//     a própria fila nunca o ultrapassa; e que comandos drenados carregam de volta os EXATOS
//     cantos/id-de-textura/dims-de-textura/src_px/tint com que foram empurrados (a fixação
//     "câmera-no-momento-da-chamada" -- nada é recomputado no momento do dreno).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/layer_queue.hpp"

#include <cstdio>
#include <vector>

using glintfx::ColorF;
using glintfx::RectF;
using glintfx::Vec2F;
using glintfx::draw2d_detail::LayerCommand;
using glintfx::draw2d_detail::LayerGroup;
using glintfx::draw2d_detail::LayerQueue;
using glintfx::draw2d_detail::PushResult;
using glintfx::draw2d_detail::ScissorSnapshot;
using glintfx::draw2d_detail::SpriteCorners;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Builds a distinguishable command -- `marker` goes into `texture_id` (never a real GL
//     texture in this pure-header test), everything else defaulted except `layer`/`scissor`.
// PT: Monta um comando distinguível -- `marker` vai pro `texture_id` (nunca uma textura GL de
//     verdade neste teste de header puro), tudo o resto default exceto `layer`/`scissor`.
LayerCommand make_cmd(int layer, std::uint32_t marker, ScissorSnapshot scissor = {}) {
  LayerCommand cmd;
  cmd.corners = SpriteCorners{Vec2F{0.f, 0.f}, Vec2F{1.f, 0.f}, Vec2F{1.f, 1.f}, Vec2F{0.f, 1.f}};
  cmd.texture_id = marker;
  cmd.tex_w = 1;
  cmd.tex_h = 1;
  cmd.src_px = RectF{};
  cmd.tint = ColorF{};
  cmd.scissor = scissor;
  cmd.layer = layer;
  return cmd;
}

// EN: Flattens every group's commands, in group order -- the overall sorted+grouped sequence.
// PT: Achata os comandos de todo grupo, na ordem dos grupos -- a sequência ordenada+agrupada
//     completa.
std::vector<LayerCommand> flatten(const std::vector<LayerGroup>& groups) {
  std::vector<LayerCommand> out;
  for (const LayerGroup& g : groups)
    for (const LayerCommand& c : g.commands) out.push_back(c);
  return out;
}

// EN: D2D-3B QA fix (MAJOR): 32 same-layer commands, not 3 -- an earlier version of this test
//     pushed only 3, which does NOT kill the std::stable_sort -> std::sort mutation this file's
//     own header comment names: libstdc++'s introsort falls back to insertion sort below its
//     small-partition threshold (empirically ~16 elements), and insertion sort on an ALREADY-
//     sorted-by-push-order run (every key here compares equal, so the input is trivially
//     "sorted" under this comparator) preserves order BY ACCIDENT even with a plain, unstable
//     std::sort -- the mutation survived, green for the wrong reason. 32 clears that threshold
//     (confirmed empirically: n=24/32/40/50/64 all reorder under plain std::sort with this
//     all-equal-key pattern, n=8/16 do not) so the mutation actually shuffles `texture_id` and
//     this check catches it.
// PT: Conserto de QA do D2D-3B (MAJOR): 32 comandos de mesma camada, não 3 -- uma versão anterior
//     deste teste empurrava só 3, o que NÃO mata a mutação std::stable_sort -> std::sort que o
//     próprio comentário de cabeçalho deste arquivo nomeia: o introsort da libstdc++ cai pra
//     insertion sort abaixo do próprio teto de partição pequena (empiricamente ~16 elementos), e
//     insertion sort sobre uma corrida JÁ ordenada pela ordem de push (toda chave aqui compara
//     igual, então a entrada é trivialmente "ordenada" sob este comparador) preserva a ordem POR
//     ACIDENTE mesmo com um std::sort simples e instável -- a mutação sobrevivia, verde pelo
//     motivo errado. 32 supera esse teto (confirmado empiricamente: n=24/32/40/50/64 todos
//     reordenam sob um std::sort simples com este padrão de chave-toda-igual, n=8/16 não) então a
//     mutação de fato embaralha `texture_id` e este cheque a pega.
void test_stable_order_within_layer() {
  constexpr int kCount = 32;
  LayerQueue q;
  for (int i = 0; i < kCount; ++i) q.push(make_cmd(0, static_cast<std::uint32_t>(i)));
  const std::vector<LayerGroup> groups = q.drain_grouped();
  check(groups.size() == 1, "stable_order: same layer + same (default-inactive) scissor -> 1 group");
  check(groups.size() == 1 && groups[0].commands.size() == static_cast<std::size_t>(kCount),
        "stable_order: all 32 commands present");
  bool in_order = groups.size() == 1 && groups[0].commands.size() == static_cast<std::size_t>(kCount);
  for (int i = 0; in_order && i < kCount; ++i)
    in_order = groups[0].commands[static_cast<std::size_t>(i)].texture_id == static_cast<std::uint32_t>(i);
  check(in_order,
        "stable_order: 32 equal-layer commands keep PUSH order 0..31 exactly (the std::stable_sort "
        "mutation killer -- a plain std::sort offers no such guarantee and reorders at this count)");
  check(q.empty(), "stable_order: drain_grouped() clears the queue");
}

void test_negative_and_interleaved_layers() {
  LayerQueue q;
  q.push(make_cmd(5, 100));
  q.push(make_cmd(-3, 200));
  q.push(make_cmd(0, 300));
  q.push(make_cmd(-3, 201)); // same layer as marker 200, pushed AFTER it.
  q.push(make_cmd(5, 101));  // same layer as marker 100, pushed AFTER it.
  const std::vector<LayerCommand> flat = flatten(q.drain_grouped());
  check(flat.size() == 5, "interleaved_layers: all 5 commands present after sort+group");
  const std::vector<std::uint32_t> expected = {200, 201, 300, 100, 101}; // -3,-3,0,5,5 ascending.
  bool matches = flat.size() == expected.size();
  for (std::size_t i = 0; matches && i < expected.size(); ++i)
    matches = matches && flat[i].texture_id == expected[i];
  check(matches,
        "interleaved_layers: ascending by layer (negative first), same-layer sub-order preserved "
        "(200 before 201, 100 before 101)");
}

void test_scissor_grouping_splits_at_snapshot_changes() {
  // EN: All layer 0 -- stable sort is a no-op here, isolating the grouping logic on its own.
  //     Push pattern A,A,B,B,A (3 scissor RUNS in push order) -> expect exactly 3 groups with
  //     sizes 2,2,1.
  // PT: Tudo camada 0 -- a ordenação estável é um no-op aqui, isolando a lógica de agrupamento
  //     sozinha. Padrão de push A,A,B,B,A (3 CORRIDAS de scissor na ordem de push) -> espera
  //     exatamente 3 grupos com tamanhos 2,2,1.
  const ScissorSnapshot a{true, RectF{0.f, 0.f, 10.f, 10.f}};
  const ScissorSnapshot b{true, RectF{5.f, 5.f, 20.f, 20.f}};
  LayerQueue q;
  q.push(make_cmd(0, 1, a));
  q.push(make_cmd(0, 2, a));
  q.push(make_cmd(0, 3, b));
  q.push(make_cmd(0, 4, b));
  q.push(make_cmd(0, 5, a));
  const std::vector<LayerGroup> groups = q.drain_grouped();
  check(groups.size() == 3, "scissor_grouping: A,A,B,B,A splits into exactly 3 runs");
  check(groups.size() == 3 && groups[0].commands.size() == 2 && groups[1].commands.size() == 2 &&
            groups[2].commands.size() == 1,
        "scissor_grouping: run sizes are 2,2,1 -- boundaries land exactly at snapshot changes");
}

void test_scissor_grouping_happens_after_sort_not_before() {
  // EN: The mutation killer this file's own header comment names: interleave 2 layers with
  //     DIFFERENT scissors so the PUSH order and the SORTED order disagree. Push order is
  //     layer1/A, layer0/B, layer1/A, layer0/B (alternating -- grouping BEFORE sort would see 4
  //     runs: A,B,A,B). After the D27 stable sort by layer (0s first, keeping their own push
  //     order; then 1s, keeping theirs), the SORTED sequence is layer0/B, layer0/B, layer1/A,
  //     layer1/A -- grouping THAT sequence yields exactly 2 runs: B,B then A,A. A `drain_grouped()`
  //     that grouped in push order (bug) would report 4 groups here, not 2.
  // PT: O mata-mutação que o próprio comentário de cabeçalho deste arquivo nomeia: intercala 2
  //     camadas com scissors DIFERENTES pra que a ordem de PUSH e a ordem ORDENADA discordem.
  //     Ordem de push é layer1/A, layer0/B, layer1/A, layer0/B (alternando -- agrupar ANTES de
  //     ordenar veria 4 corridas: A,B,A,B). Depois da ordenação estável do D27 por camada (0s
  //     primeiro, mantendo a própria ordem de push; depois 1s, mantendo a deles), a sequência
  //     ORDENADA é layer0/B, layer0/B, layer1/A, layer1/A -- agrupar AQUELA sequência produz
  //     exatamente 2 corridas: B,B depois A,A. Um `drain_grouped()` que agrupasse na ordem de
  //     push (bug) reportaria 4 grupos aqui, não 2.
  const ScissorSnapshot a{true, RectF{0.f, 0.f, 10.f, 10.f}};
  const ScissorSnapshot b{true, RectF{99.f, 99.f, 1.f, 1.f}};
  LayerQueue q;
  q.push(make_cmd(1, 10, a));
  q.push(make_cmd(0, 20, b));
  q.push(make_cmd(1, 11, a));
  q.push(make_cmd(0, 21, b));
  const std::vector<LayerGroup> groups = q.drain_grouped();
  check(groups.size() == 2,
        "scissor_after_sort: interleaved layer+scissor push order still yields 2 groups post-sort "
        "(4 would mean grouping ran BEFORE the layer sort, the mutation this test kills)");
  check(groups.size() == 2 && groups[0].commands.size() == 2 && groups[1].commands.size() == 2,
        "scissor_after_sort: each group holds both same-scissor commands");
  check(groups.size() == 2 && groups[0].commands.size() == 2 && groups[0].commands[0].texture_id == 20 &&
            groups[0].commands[1].texture_id == 21,
        "scissor_after_sort: group 0 (layer 0, scissor B) is markers 20,21 in push order");
  check(groups.size() == 2 && groups[1].commands.size() == 2 && groups[1].commands[0].texture_id == 10 &&
            groups[1].commands[1].texture_id == 11,
        "scissor_after_sort: group 1 (layer 1, scissor A) is markers 10,11 in push order");
}

void test_inactive_scissor_groups_with_any_inactive_rect() {
  // EN: Two INACTIVE snapshots are always equal regardless of `rect` bits (scissor_equal()'s own
  //     documented rule -- `rect` is don't-care when `active == false`). A stray non-zero `rect`
  //     on an inactive snapshot (should never happen from real callers, but this is a pure header
  //     with no way to enforce that) must NOT split the group.
  // PT: Dois snapshots INATIVOS são sempre iguais independente dos bits de `rect` (a própria
  //     regra documentada do scissor_equal() -- `rect` é don't-care quando `active == false`). Um
  //     `rect` não-zero perdido num snapshot inativo (nunca deveria acontecer de chamadores reais,
  //     mas este é um header puro sem como forçar isso) NÃO PODE quebrar o grupo.
  LayerQueue q;
  q.push(make_cmd(0, 1, ScissorSnapshot{false, RectF{}}));
  q.push(make_cmd(0, 2, ScissorSnapshot{false, RectF{50.f, 50.f, 1.f, 1.f}})); // stray rect, don't-care.
  const std::vector<LayerGroup> groups = q.drain_grouped();
  check(groups.size() == 1,
        "inactive_scissor: two inactive snapshots stay ONE group regardless of rect bits");
}

void test_cap_262144() {
  LayerQueue q;
  std::size_t ok_count = 0;
  std::size_t cap_count = 0;
  const std::size_t total = LayerQueue::kMaxCommands + 5000; // a 300k-class hostile stream.
  for (std::size_t i = 0; i < total; ++i) {
    const PushResult r = q.push(make_cmd(0, static_cast<std::uint32_t>(i)));
    if (r == PushResult::Ok)
      ++ok_count;
    else
      ++cap_count;
    if (q.size() > LayerQueue::kMaxCommands) break; // would itself be the failure -- stop early.
  }
  check(q.size() <= LayerQueue::kMaxCommands,
        "cap_262144: the queue never exceeds the 262144 hard cap, even under a 300k+ stream");
  check(ok_count == LayerQueue::kMaxCommands,
        "cap_262144: exactly the first 262144 pushes succeed");
  check(cap_count == total - LayerQueue::kMaxCommands,
        "cap_262144: every push past the cap reports CapExceeded (dropped, not silently lost)");
}

void test_final_corners_and_fields_survive_drain_unchanged() {
  // EN: "Camera-at-call-time" pin (D27): a command's corners/texture_id/tex dims/src_px/tint are
  //     stored VERBATIM and come back identical from drain_grouped() -- nothing here re-derives
  //     or re-projects them (that already happened before push(), by contract).
  // PT: Fixação "câmera-no-momento-da-chamada" (D27): os cantos/texture_id/dims-de-textura/
  //     src_px/tint de um comando são guardados AO PÉ DA LETRA e voltam idênticos do
  //     drain_grouped() -- nada aqui os re-deriva ou re-projeta (isso já aconteceu antes do
  //     push(), por contrato).
  LayerQueue q;
  LayerCommand cmd;
  cmd.corners = SpriteCorners{Vec2F{1.5f, 2.5f}, Vec2F{9.5f, 2.5f}, Vec2F{9.5f, 7.5f}, Vec2F{1.5f, 7.5f}};
  cmd.texture_id = 42;
  cmd.tex_w = 64;
  cmd.tex_h = 32;
  cmd.src_px = RectF{4.f, 8.f, 16.f, 16.f};
  cmd.tint = ColorF{0.25f, 0.5f, 0.75f, 1.f};
  cmd.scissor = ScissorSnapshot{true, RectF{0.f, 0.f, 100.f, 100.f}};
  cmd.layer = -7;
  const PushResult r = q.push(cmd);
  check(r == PushResult::Ok, "field_survival: push() accepted the command");

  const std::vector<LayerCommand> flat = flatten(q.drain_grouped());
  check(flat.size() == 1, "field_survival: exactly 1 command drained");
  const LayerCommand& out = flat[0];
  check(out.corners.tl.x == 1.5f && out.corners.tl.y == 2.5f && out.corners.br.x == 9.5f &&
            out.corners.br.y == 7.5f,
        "field_survival: corners come back verbatim (no re-projection)");
  check(out.texture_id == 42 && out.tex_w == 64 && out.tex_h == 32,
        "field_survival: texture identity/dimensions come back verbatim");
  check(out.src_px.x == 4.f && out.src_px.w == 16.f, "field_survival: src_px comes back verbatim");
  check(out.tint.r == 0.25f && out.tint.a == 1.f, "field_survival: tint comes back verbatim");
  check(out.scissor.active && out.scissor.rect.w == 100.f, "field_survival: scissor snapshot comes back verbatim");
}

void test_clear_discards_without_draining() {
  LayerQueue q;
  q.push(make_cmd(0, 1));
  q.push(make_cmd(0, 2));
  check(q.size() == 2, "clear: setup -- 2 commands buffered");
  q.clear();
  check(q.empty(), "clear: discards every buffered command (D27/D32's shutdown()-discards-buffer contract)");
  const std::vector<LayerGroup> groups = q.drain_grouped();
  check(groups.empty(), "clear: a subsequent drain_grouped() on a cleared queue yields nothing");
}

} // namespace

int main() {
  test_stable_order_within_layer();
  test_negative_and_interleaved_layers();
  test_scissor_grouping_splits_at_snapshot_changes();
  test_scissor_grouping_happens_after_sort_not_before();
  test_inactive_scissor_groups_with_any_inactive_rect();
  test_cap_262144();
  test_final_corners_and_fields_survive_drain_unchanged();
  test_clear_discards_without_draining();

  if (g_failures == 0) {
    std::puts("layer_queue_sanity: PASS");
    return 0;
  }
  std::printf("layer_queue_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
