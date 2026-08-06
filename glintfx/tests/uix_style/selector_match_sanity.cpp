// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SELECTOR-MATCH -- unit suite for `glintfx::uix::style::match_selector`/
//     `match_selector_list`/`compound_specificity`/`selector_specificity`. Builds `Selector`/
//     `CompoundSelector` trees BY HAND (this module's own public types, not text -- see
//     selector_match.hpp's own header comment) against small `glintfx::uix::Element` trees built by
//     hand too, so every case below tests EXACTLY the boundary it names, not whatever a real corpus
//     file happens to contain. Every one of the 8 authorized selector forms
//     (`docs/rmlx-subset.md` section 6.2) gets a POSITIVE and a NEGATIVE case; every boundary this
//     module introduces gets the boundary case AND one step outside it, per this task's own brief.
//
//     WHAT EACH GROUP OF CASES CLAIMS TO EXERCISE (this task's own brief: "qual condição este
//     exemplo afirma exercitar, e a entrada escolhida realmente a alcança?"):
//       - `compound_specificity_*`: the exact per-weight arithmetic (`kSpecificityWeightTag`/
//         `Id`/`ClassOrPseudo`), one boundary per weight (present vs. absent, one class vs. two).
//       - `class/id/tag/descendant/child/compound/hover/comma` groups: one POSITIVE + one NEGATIVE
//         per form, plus the form's own load-bearing boundary (id vs. no-id asymmetry; case
//         sensitivity; nearest-ancestor-fails-but-next-succeeds backtracking; child-vs-descendant
//         divergence on the SAME tree shape; near-miss class count; hover flag toggling with
//         nothing else in the compound changing).
//       - `comma_list_returns_max_specificity_among_matches`: the ONE genuinely non-obvious design
//         decision this module makes (selector_match.hpp's own "Comma-list semantics" paragraph) --
//         constructed so "first match wins" and "max match wins" give VISIBLY DIFFERENT answers,
//         proving the implementation picked the documented one, not merely that some entry matched.
// PT: UIX-SELECTOR-MATCH -- suíte unitária pro `glintfx::uix::style::match_selector`/
//     `match_selector_list`/`compound_specificity`/`selector_specificity`. Constrói árvores
//     `Selector`/`CompoundSelector` À MÃO (os próprios tipos públicos deste módulo, não texto -- ver
//     o comentário de cabeçalho do selector_match.hpp) contra pequenas árvores
//     `glintfx::uix::Element` também construídas à mão, pra todo caso abaixo testar EXATAMENTE a
//     fronteira que nomeia, não o que um arquivo de corpus real por acaso contém. Toda uma das 8
//     formas de seletor autorizadas (seção 6.2 do `docs/rmlx-subset.md`) recebe um caso POSITIVO e
//     um NEGATIVO; toda fronteira que este módulo introduz recebe o caso de fronteira E um passo
//     fora dela, per o próprio briefing desta tarefa.
//
//     O QUE CADA GRUPO DE CASOS AFIRMA EXERCITAR (o próprio briefing desta tarefa: "qual condição
//     este exemplo afirma exercitar, e a entrada escolhida realmente a alcança?"):
//       - `compound_specificity_*`: a aritmética exata por-peso (`kSpecificityWeightTag`/`Id`/
//         `ClassOrPseudo`), uma fronteira por peso (presente vs. ausente, uma classe vs. duas).
//       - grupos `class/id/tag/descendant/child/compound/hover/comma`: um caso POSITIVO + um
//         NEGATIVO por forma, mais a própria fronteira que carrega peso da forma (assimetria
//         id-vs-sem-id; sensibilidade a caixa; backtracking ancestral-mais-próximo-falha-mas-
//         próximo-funciona; divergência filho-vs-descendente na MESMA forma de árvore; contagem de
//         classe quase-igual; flag de hover alternando sem mais nada mudar no compound).
//       - `comma_list_returns_max_specificity_among_matches`: a ÚNICA decisão de desenho
//         genuinamente não-óbvia que este módulo toma (o próprio parágrafo "Semântica de
//         lista-vírgula" do selector_match.hpp) -- construído pra "primeiro casamento vence" e
//         "casamento máximo vence" darem respostas VISIVELMENTE DIFERENTES, provando que a
//         implementação escolheu a documentada, não só que alguma entrada casou.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/selector_match.hpp"

#include <cstdio>
#include <memory>
#include <string>

using glintfx::uix::as_element;
using glintfx::uix::Element;
using glintfx::uix::Node;
using glintfx::uix::style::Combinator;
using glintfx::uix::style::compound_specificity;
using glintfx::uix::style::CompoundSelector;
using glintfx::uix::style::kSpecificityWeightClassOrPseudo;
using glintfx::uix::style::kSpecificityWeightId;
using glintfx::uix::style::kSpecificityWeightTag;
using glintfx::uix::style::match_selector;
using glintfx::uix::style::match_selector_list;
using glintfx::uix::style::MatchResult;
using glintfx::uix::style::MatchState;
using glintfx::uix::style::Selector;
using glintfx::uix::style::selector_specificity;
using glintfx::uix::style::SelectorList;

namespace {

int g_failures = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

// EN: `child.tag()` is appended under `parent` and the raw pointer to it returned (owned by
//     `parent`'s own children_ vector -- see dom_tree.hpp's own `AppendResult` doc-comment for why
//     this is safe to keep around for the lifetime of `parent`).
// PT: `child.tag()` é somado sob `parent` e o ponteiro cru pra ele é retornado (de posse do próprio
//     vetor children_ do `parent` -- ver o próprio comentário de doc de `AppendResult` do
//     dom_tree.hpp pra por que é seguro manter isto por aí pela vida de `parent`).
Element* add_child(Element& parent, std::string tag) {
  auto child = std::make_unique<Element>(std::move(tag));
  Element* raw = child.get();
  auto result = parent.append_child(std::move(child));
  (void)result;
  return raw;
}

CompoundSelector tag_only(std::string_view tag) {
  CompoundSelector c;
  c.tag = tag;
  return c;
}

CompoundSelector id_only(std::string_view id) {
  CompoundSelector c;
  c.id = id;
  return c;
}

CompoundSelector class_only(std::string_view cls) {
  CompoundSelector c;
  c.classes.push_back(cls);
  return c;
}

Selector chain1(CompoundSelector a) {
  Selector s;
  s.compounds = {a};
  return s;
}

Selector chain2(CompoundSelector a, Combinator combinator, CompoundSelector b) {
  Selector s;
  s.compounds = {a, b};
  s.combinators = {combinator};
  return s;
}

// ---------------------------------------------------------------------------
// compound_specificity: per-weight arithmetic, one boundary per weight.
// ---------------------------------------------------------------------------
void test_compound_specificity_weights() {
  check(compound_specificity(CompoundSelector{}) == 0,
        "compound_specificity: an entirely empty compound weighs 0 (defensive baseline, "
        "unreachable via the real parser, but the arithmetic itself must not invent a weight)");

  CompoundSelector tag_c;
  tag_c.tag = "div";
  check(compound_specificity(tag_c) == kSpecificityWeightTag,
        "compound_specificity: tag-only == kSpecificityWeightTag (10'000)");

  CompoundSelector id_c;
  id_c.id = "panel";
  check(compound_specificity(id_c) == kSpecificityWeightId,
        "compound_specificity: id-only == kSpecificityWeightId (1'000'000)");

  CompoundSelector one_class;
  one_class.classes = {"wide"};
  check(compound_specificity(one_class) == kSpecificityWeightClassOrPseudo,
        "compound_specificity: one class == kSpecificityWeightClassOrPseudo (100'000)");

  // Boundary: one class vs. two classes -- the weight must scale BY COUNT, not saturate at "has
  // a class" like a boolean would.
  CompoundSelector two_classes;
  two_classes.classes = {"wide", "danger"};
  check(compound_specificity(two_classes) == 2 * kSpecificityWeightClassOrPseudo,
        "compound_specificity: BOUNDARY two classes == 2x the one-class weight, not saturated");

  CompoundSelector hover_only;
  hover_only.pseudo_hover = true;
  check(compound_specificity(hover_only) == kSpecificityWeightClassOrPseudo,
        "compound_specificity: hover-only == kSpecificityWeightClassOrPseudo (same weight as one "
        "class, per StyleSheetSelector.h's own Class==PseudoClass)");

  // Full combination -- every weight summed, proving there is no early-return/short-circuit that
  // silently drops one of the four contributions.
  CompoundSelector everything;
  everything.tag = "div";
  everything.id = "panel";
  everything.classes = {"wide", "danger"};
  everything.pseudo_hover = true;
  const auto expected = kSpecificityWeightTag + kSpecificityWeightId +
                        2 * kSpecificityWeightClassOrPseudo + kSpecificityWeightClassOrPseudo;
  check(compound_specificity(everything) == expected,
        "compound_specificity: tag+id+2 classes+hover all sum, none silently dropped");
}

void test_selector_specificity_sums_chain() {
  // #id descendant .cls -- proves the WHOLE-CHAIN sum, not just a single compound's own weight.
  Selector s = chain2(id_only("panel"), Combinator::Descendant, class_only("wide"));
  check(selector_specificity(s) == kSpecificityWeightId + kSpecificityWeightClassOrPseudo,
        "selector_specificity: #id .cls sums BOTH compounds' weights (1'100'000)");
}

// ---------------------------------------------------------------------------
// Form 1/8: `.classe` -- positive/negative.
// ---------------------------------------------------------------------------
void test_form_class() {
  Element el("div");
  el.add_class("wide");
  Selector s = chain1(class_only("wide"));

  MatchResult positive = match_selector(s, el, MatchState{});
  check(positive.matched, "form .classe: POSITIVE -- element carrying the class matches");
  check(positive.specificity == kSpecificityWeightClassOrPseudo,
        "form .classe: POSITIVE -- specificity is exactly one class weight");

  Element el_missing("div");
  MatchResult negative = match_selector(s, el_missing, MatchState{});
  check(!negative.matched, "form .classe: NEGATIVE -- element without the class does not match");
  check(negative.specificity == 0, "form .classe: NEGATIVE -- specificity is 0 on a non-match");
}

// ---------------------------------------------------------------------------
// Form 2/8: `#id` -- positive/negative, plus the empty-id-vs-no-id asymmetry boundary
// (dom_tree.hpp's own documented policy: empty id == no id).
// ---------------------------------------------------------------------------
void test_form_id() {
  Element el("div");
  el.set_id("panel");
  Selector s = chain1(id_only("panel"));

  check(match_selector(s, el, MatchState{}).matched, "form #id: POSITIVE -- matching id matches");

  Element other_id("div");
  other_id.set_id("sidebar");
  check(!match_selector(s, other_id, MatchState{}).matched,
        "form #id: NEGATIVE -- a DIFFERENT id does not match");

  // Boundary: an element with NO id at all (id() == "", has_id() == false) must not match a
  // selector naming a non-empty id -- this is the "one step outside" of the id-matching boundary,
  // distinct from "wrong id" above (a genuinely id-less element, not a misnamed one).
  Element no_id("div");
  check(!match_selector(s, no_id, MatchState{}).matched,
        "form #id: BOUNDARY -- an id-LESS element does not match a non-empty #id selector");
}

// ---------------------------------------------------------------------------
// Form 3/8: descendant (space) -- positive/negative, plus the backtracking boundary (nearest
// ancestor fails the compound, but a FURTHER ancestor satisfies it -- naive "check only the
// immediate parent" code would wrongly reject this).
// ---------------------------------------------------------------------------
void test_form_descendant() {
  Element root("body");
  Element* mid = add_child(root, "section");
  mid->add_class("wide");
  Element* leaf = add_child(*mid, "span");

  Selector s = chain2(class_only("wide"), Combinator::Descendant, tag_only("span"));
  check(match_selector(s, *leaf, MatchState{}).matched,
        "form descendant: POSITIVE -- .wide span matches a span nested directly under .wide");

  Element root2("body");
  Element* plain_mid = add_child(root2, "section"); // no "wide" class anywhere in this tree
  Element* leaf2 = add_child(*plain_mid, "span");
  check(!match_selector(s, *leaf2, MatchState{}).matched,
        "form descendant: NEGATIVE -- no ancestor carries .wide, no match");

  // Boundary: the NEAREST ancestor does NOT satisfy the compound, but the NEXT one up does --
  // proves the matcher backtracks past a failed nearest ancestor instead of giving up.
  Element root3("body");
  root3.add_class("wide");
  Element* mid3 = add_child(root3, "section"); // does NOT carry .wide
  Element* leaf3 = add_child(*mid3, "span");
  check(match_selector(s, *leaf3, MatchState{}).matched,
        "form descendant: BOUNDARY -- nearest ancestor lacks .wide, but the grandparent has it; "
        "matcher must backtrack past the failed nearest ancestor");
}

// ---------------------------------------------------------------------------
// Form 4/8: tag -- positive/negative, plus the case-sensitivity boundary (upstream's own
// `Match()` uses a plain `!=` on `String`, never folding case -- StyleSheetNode.cpp:154).
// ---------------------------------------------------------------------------
void test_form_tag() {
  Element el("div");
  Selector s = chain1(tag_only("div"));
  check(match_selector(s, el, MatchState{}).matched, "form tag: POSITIVE -- exact tag matches");

  Element other("span");
  check(!match_selector(s, other, MatchState{}).matched,
        "form tag: NEGATIVE -- a different tag does not match");

  // Boundary: case sensitivity -- "Div" must NOT match a "div" selector (one step outside the
  // exact-match boundary in the case dimension, not the identity dimension already covered above).
  Element mixed_case("Div");
  check(!match_selector(s, mixed_case, MatchState{}).matched,
        "form tag: BOUNDARY -- tag matching is case-SENSITIVE, \"Div\" != \"div\"");
}

// ---------------------------------------------------------------------------
// Form 5/8: pseudo-composite (`:hover`) -- positive/negative on the hover flag alone, nothing
// else in the compound changing between the two cases.
// ---------------------------------------------------------------------------
void test_form_hover() {
  Element el("button");
  el.add_class("verb");
  CompoundSelector c = class_only("verb");
  c.pseudo_hover = true;
  Selector s = chain1(c);

  MatchState hover_on;
  hover_on.hover_active = true;
  check(match_selector(s, el, hover_on).matched,
        "form :hover: POSITIVE -- .verb:hover matches when MatchState::hover_active is true");

  MatchState hover_off; // default-constructed, hover_active == false
  check(!match_selector(s, el, hover_off).matched,
        "form :hover: NEGATIVE -- the SAME element/selector does not match when hover_active is "
        "false -- the ONLY thing that changed is the flag");

  // Boundary: hover_active true is not enough on its own -- the REST of the compound (the class)
  // still has to match too.
  Element wrong_class("button");
  wrong_class.add_class("other");
  check(!match_selector(s, wrong_class, hover_on).matched,
        "form :hover: BOUNDARY -- hover_active true does not waive the compound's own class "
        "requirement");
}

// ---------------------------------------------------------------------------
// Form 6/8: comma-list -- covered by its own dedicated section below (test_comma_list_forms),
// including the max-specificity design decision.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Form 7/8: compound, no combinator (`div.hud`) -- positive/negative on EACH half independently,
// proving both tag AND class are required, neither alone suffices.
// ---------------------------------------------------------------------------
void test_form_compound_no_combinator() {
  CompoundSelector c;
  c.tag = "div";
  c.classes = {"hud"};
  Selector s = chain1(c);

  Element both("div");
  both.add_class("hud");
  check(match_selector(s, both, MatchState{}).matched,
        "form compound (div.hud): POSITIVE -- tag AND class both present matches");

  Element tag_only_el("div"); // right tag, missing class
  check(!match_selector(s, tag_only_el, MatchState{}).matched,
        "form compound (div.hud): NEGATIVE A -- right tag, missing class does not match");

  Element class_only_el("span"); // right class, wrong tag
  class_only_el.add_class("hud");
  check(!match_selector(s, class_only_el, MatchState{}).matched,
        "form compound (div.hud): NEGATIVE B -- right class, wrong tag does not match");
}

// ---------------------------------------------------------------------------
// Form 8/8: child (`>`) -- positive/negative, plus the load-bearing child-vs-descendant divergence
// on the EXACT SAME tree shape (a grandparent satisfies the ancestor compound, but is not the
// DIRECT parent) -- this is the one case that would silently pass if Child were implemented as
// Descendant by mistake.
// ---------------------------------------------------------------------------
void test_form_child() {
  Element parent("section");
  parent.add_class("scroller");
  Element* direct_child = add_child(parent, "div");

  Selector s = chain2(class_only("scroller"), Combinator::Child, tag_only("div"));
  check(match_selector(s, *direct_child, MatchState{}).matched,
        "form child (.scroller > div): POSITIVE -- direct parent carries .scroller");

  Element unrelated_parent("section"); // no .scroller anywhere
  Element* unrelated_child = add_child(unrelated_parent, "div");
  check(!match_selector(s, *unrelated_child, MatchState{}).matched,
        "form child (.scroller > div): NEGATIVE -- no .scroller ancestor at all");

  // BOUNDARY, the load-bearing one: .scroller is the GRANDPARENT, not the direct parent -- a
  // Descendant-shaped implementation would wrongly accept this; Child must reject it.
  Element grandparent("body");
  grandparent.add_class("scroller");
  Element* mid = add_child(grandparent, "section"); // direct parent, does NOT carry .scroller
  Element* grandchild = add_child(*mid, "div");
  check(!match_selector(s, *grandchild, MatchState{}).matched,
        "form child (.scroller > div): BOUNDARY -- .scroller is the GRANDPARENT, not the direct "
        "parent; Child combinator must reject what Descendant would accept");

  // One step further outside: prove the SAME tree shape DOES match a Descendant-combinator
  // version of the selector -- this is what pins the divergence, rather than merely asserting
  // Child fails (which a broken matcher that always returns false would also satisfy).
  Selector descendant_version = chain2(class_only("scroller"), Combinator::Descendant, tag_only("div"));
  check(match_selector(descendant_version, *grandchild, MatchState{}).matched,
        "form child: BOUNDARY control -- the IDENTICAL tree DOES match the Descendant-combinator "
        "version of the same selector, pinning that Child's rejection above is combinator-specific, "
        "not a general matcher failure");
}

// ---------------------------------------------------------------------------
// Boundary: near-miss class count -- a compound requiring 2 classes must reject an element
// carrying only 1 of the 2 (one class short, not zero).
// ---------------------------------------------------------------------------
void test_boundary_near_miss_class_count() {
  CompoundSelector c;
  c.classes = {"wide", "danger"};
  Selector s = chain1(c);

  Element both("div");
  both.add_class("wide");
  both.add_class("danger");
  check(match_selector(s, both, MatchState{}).matched,
        "near-miss classes: BOUNDARY -- element carrying BOTH required classes matches");

  Element one_of_two("div");
  one_of_two.add_class("wide"); // missing "danger" -- one class short, not zero
  check(!match_selector(s, one_of_two, MatchState{}).matched,
        "near-miss classes: ONE STEP OUTSIDE -- element carrying only 1 of 2 required classes "
        "(one short, not a total miss) does not match");
}

// ---------------------------------------------------------------------------
// Boundary: a root element (no parent at all) can only ever satisfy a single-compound selector;
// a Child/Descendant chain against it must fail cleanly, never crash on a null parent.
// ---------------------------------------------------------------------------
void test_boundary_root_element_has_no_ancestors() {
  Element root("body");
  root.add_class("wide");

  Selector single = chain1(class_only("wide"));
  check(match_selector(single, root, MatchState{}).matched,
        "root element: BOUNDARY -- a single-compound selector matches the root directly");

  Selector needs_ancestor = chain2(tag_only("html"), Combinator::Descendant, class_only("wide"));
  check(!match_selector(needs_ancestor, root, MatchState{}).matched,
        "root element: ONE STEP OUTSIDE -- a 2-compound selector requiring an ancestor cannot "
        "match the root (no parent to walk), and must not crash doing so");
}

// ---------------------------------------------------------------------------
// Defensive boundary: an empty Selector (compounds.empty()), unreachable via parse_stylesheet's
// own construction invariant, must still be handled as a clean not-matched result, never an
// out-of-bounds access -- this module's own "trust boundary" paragraph.
// ---------------------------------------------------------------------------
void test_boundary_empty_selector_defensive() {
  Selector empty;
  Element el("div");
  MatchResult r = match_selector(empty, el, MatchState{});
  check(!r.matched, "empty Selector: defensive -- compounds.empty() returns not-matched, not UB");
  check(r.specificity == 0, "empty Selector: defensive -- specificity stays 0");
}

// ---------------------------------------------------------------------------
// Comma-list (form 6/8): one entry, matching two entries, no entry matching, and the load-bearing
// max-specificity decision.
// ---------------------------------------------------------------------------
void test_comma_list_forms() {
  Element el("div");
  el.add_class("apnum");

  SelectorList single_entry = {chain1(class_only("apnum"))};
  MatchResult r1 = match_selector_list(single_entry, el, MatchState{});
  check(r1.matched, "comma-list: POSITIVE -- a single-entry list behaves like match_selector");

  SelectorList none_match = {chain1(class_only("mananum")), chain1(tag_only("span"))};
  check(!match_selector_list(none_match, el, MatchState{}).matched,
        "comma-list: NEGATIVE -- no entry matches, the list does not match");

  SelectorList empty_list;
  MatchResult r_empty = match_selector_list(empty_list, el, MatchState{});
  check(!r_empty.matched, "comma-list: defensive -- an empty list is not-matched, not UB");

  // The load-bearing decision: entry[0] has LOWER specificity (a bare .apnum, 100'000) and
  // matches; entry[1] has HIGHER specificity (#id.apnum, 1'100'000) and ALSO matches. A "first
  // match wins" implementation would return 100'000; this module's own documented contract
  // (selector_match.hpp's "Comma-list semantics") requires the MAXIMUM, 1'100'000.
  Element with_id("div");
  with_id.set_id("hpbox");
  with_id.add_class("apnum");

  CompoundSelector higher;
  higher.id = "hpbox";
  higher.classes = {"apnum"};

  SelectorList mixed_specificity = {chain1(class_only("apnum")), chain1(higher)};
  MatchResult r_max = match_selector_list(mixed_specificity, with_id, MatchState{});
  check(r_max.matched, "comma-list: max-specificity case -- both entries match");
  check(r_max.specificity == kSpecificityWeightId + kSpecificityWeightClassOrPseudo,
        "comma-list: max-specificity case -- returns the HIGHER entry's specificity "
        "(1'100'000), proving this is NOT \"first match wins\" (which would wrongly return "
        "100'000, the lower, first-listed entry's own weight)");
}

} // namespace

int main() {
  test_compound_specificity_weights();
  test_selector_specificity_sums_chain();
  test_form_class();
  test_form_id();
  test_form_descendant();
  test_form_tag();
  test_form_hover();
  test_form_compound_no_combinator();
  test_form_child();
  test_boundary_near_miss_class_count();
  test_boundary_root_element_has_no_ancestors();
  test_boundary_empty_selector_defensive();
  test_comma_list_forms();

  std::printf(
      "SCOPE: 8 formas cobertas de 8 enumeradas, 0 casos de corpus (ver "
      "selector_match_corpus_sanity), 0 travamentos, 0 formas fora do subset\n");

  if (g_failures > 0) {
    std::fprintf(stderr, "selector_match_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("selector_match_sanity: PASS");
  return 0;
}
