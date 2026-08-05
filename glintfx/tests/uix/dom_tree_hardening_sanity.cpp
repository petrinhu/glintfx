// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S2 -- hardening test for glintfx::uix::Element's nesting-depth ceiling. Standalone,
//     no RmlUi/GLFW/GL -- see glintfx/src/uix/dom/dom_tree.hpp's own header comment, "Hardening"
//     paragraph, for the derivation of kMaxElementDepth and why append_child fails high (rejects,
//     never truncates/reparents) instead of ever letting the tree nest deep enough to make its own
//     recursive destructor unsafe.
// PT: RMLX-1/S2 -- teste de hardening pro teto de profundidade de aninhamento do
//     glintfx::uix::Element. Standalone, sem RmlUi/GLFW/GL -- ver o próprio comentário de
//     cabeçalho do glintfx/src/uix/dom/dom_tree.hpp, parágrafo "Hardening", pra derivação do
//     kMaxElementDepth e por que append_child falha alto (rejeita, nunca trunca/reparenta) em vez
//     de algum dia deixar a árvore aninhar funda o bastante pra tornar o próprio destrutor
//     recursivo inseguro.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dom_tree.hpp"

#include <cstdio>
#include <memory>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

using glintfx::uix::AppendOutcome;
using glintfx::uix::Document;
using glintfx::uix::Element;
using glintfx::uix::kMaxElementDepth;

// ---------------------------------------------------------------------------
// EN: Build a chain exactly kMaxElementDepth deep (body itself is depth 1, so kMaxElementDepth - 1
//     more Appended calls reach depth kMaxElementDepth exactly), then prove the NEXT append is
//     REJECTED (RejectedDepthCeiling, node == nullptr, the would-be-too-deep child never adopted),
//     while every append up to and including the ceiling itself succeeds -- the fence-post case
//     this ceiling exists to get exactly right, not off-by-one.
// PT: Constrói uma cadeia com exatamente kMaxElementDepth de profundidade (o próprio body já é
//     profundidade 1, então kMaxElementDepth - 1 chamadas Appended a mais alcançam a profundidade
//     kMaxElementDepth exatamente), depois prova que o PRÓXIMO append é REJEITADO
//     (RejectedDepthCeiling, node == nullptr, o filho que ficaria fundo demais nunca é adotado),
//     enquanto todo append até e incluindo o próprio teto tem sucesso -- o caso de poste-de-cerca
//     que este teto existe pra acertar exatamente, não por-um-a-menos.
// ---------------------------------------------------------------------------
void test_depth_ceiling_fence_post() {
  Document doc;
  Element* cursor = &doc.body();
  check(cursor->depth() == 1, "chain start: body is depth 1");

  for (std::size_t i = 2; i <= kMaxElementDepth; ++i) {
    auto child = std::make_unique<Element>("div");
    auto result = cursor->append_child(std::move(child));
    if (result.outcome != AppendOutcome::Appended || result.node == nullptr) {
      std::fprintf(stderr,
                   "FAIL: depth-ceiling chain: append at intended depth %zu was rejected -- "
                   "should have succeeded (ceiling is %zu)\n",
                   i, kMaxElementDepth);
      ++g_failures;
      return;
    }
    auto* next = static_cast<Element*>(result.node);
    if (next->depth() != i) {
      std::fprintf(stderr,
                   "FAIL: depth-ceiling chain: node at intended depth %zu reports depth() == "
                   "%zu\n",
                   i, next->depth());
      ++g_failures;
    }
    cursor = next;
  }
  check(cursor->depth() == kMaxElementDepth,
        "depth-ceiling chain: deepest node built is exactly at the ceiling");

  // EN: One more child would be kMaxElementDepth + 1 -- must be REJECTED, not truncated.
  // PT: Mais um filho seria kMaxElementDepth + 1 -- precisa ser REJEITADO, não truncado.
  auto one_too_deep = std::make_unique<Element>("div");
  auto rejected = cursor->append_child(std::move(one_too_deep));
  check(rejected.outcome == AppendOutcome::RejectedDepthCeiling,
        "depth-ceiling chain: one past the ceiling is REJECTED (fail-high)");
  check(rejected.node == nullptr,
        "depth-ceiling chain: rejected child is not adopted, node is nullptr");
  check(cursor->child_count() == 0,
        "depth-ceiling chain: the node at the ceiling still has zero children after rejection");
}

} // namespace

int main() {
  test_depth_ceiling_fence_post();

  if (g_failures > 0) {
    std::fprintf(stderr, "dom_tree_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dom_tree_hardening_sanity: PASS");
  return 0;
}
