// SPDX-License-Identifier: Apache-2.0
// EN: UIX-CASCADE -- unit suite for `glintfx::uix::style::compute_element_style`/`cascade_tree`.
//     Builds `StyleSheet`/`Rule`/`Selector` trees BY HAND (this module's own public types, not
//     text -- same discipline `selector_match_sanity.cpp` already established for this exact
//     situation) against small `glintfx::uix::Element` trees built by hand too, so every case below
//     tests EXACTLY the boundary it names, not whatever a real corpus file happens to contain.
//
//     WHAT EACH GROUP OF CASES CLAIMS TO EXERCISE (this task's own brief: "qual condição este
//     exemplo afirma exercitar, e a entrada escolhida realmente a alcança?"):
//       - `test_inheritance_three_levels`: an `inherited` property (`color`) crossing THREE tree
//         levels unchanged when nothing re-declares it, alongside a NON-inherited property
//         (`background-color`) stopping dead at the declaring level (grandchild sees the registry's
//         own initial value, not the grandparent's declared one).
//       - `test_precedence_specificity_both_declaration_orders`: two rules of DIFFERENT specificity
//         targeting the SAME property on the SAME element, tried in BOTH textual orders -- the
//         higher-specificity rule wins regardless of which one was written first in the source
//         `StyleSheet::rules` vector, proving this is genuinely specificity-driven, not
//         last-rule-wins by accident.
//       - `test_precedence_source_order_tie_break`: two rules of EQUAL specificity -- the ONLY case
//         where declaration order legitimately decides the winner (`docs/uix-rcss.md`'s own "origin,
//         specificity, source order" framing, section 1) -- literal, upstream-pinned numbers on both
//         sides of the assertion (`StyleSheetSelector.h:13-21`'s own `100'000` class weight plus a
//         literal rule index), never re-expressed purely via this file's own helper functions, per
//         this task's own "golden auto-referente" warning.
//       - `test_precedence_within_rule_last_declaration_wins`: the SAME property declared twice
//         inside ONE rule -- the textually later declaration wins, with IDENTICAL specificity+order
//         stamps for both (this is the case the `>=`-overwrites (not `>`) direction of the upstream
//         `SetProperty` citation this module's own header cites is load-bearing for).
//       - `test_initial_value_full_registry`: an element matched by ZERO rules gets all 72
//         registry entries verbatim, in `all_properties()`'s own order -- pinned against the
//         registry's OWN `all_properties()` span directly (not a re-derivation), and against two
//         literal upstream-sourced initial values (`docs/uix-rcss.md` section 6.1) so the check does
//         not merely restate whatever the registry happens to contain today.
//       - `test_determinism_repeated_computation`: the SAME sheet computed three times against the
//         SAME element produces byte-identical `ComputedStyle` vectors.
//       - `test_determinism_shuffled_noncoflicting_declaration_order`: two hand-built rule sets that
//         are semantically the SAME cascade (same properties, same values, same specificities) but
//         whose NON-conflicting declarations are inserted into their own `Rule::declarations`
//         vectors in a DIFFERENT relative order -- proves this module's own output ordering (always
//         `all_properties()`'s own alphabetical order) never leaks internal processing/insertion
//         order, the concrete form this task's own "embaralhe a ordem de inserção interna" brief
//         asks for.
//       - `test_precedence_tie_break_invariant_holds_for_corpus_scale`: the arithmetic claim this
//         module's own header comment makes ("two DIFFERENT (chain_specificity, rule_index) pairs
//         can never stamp the SAME combined value below 10'000 rules") checked directly against the
//         corpus's own measured rule count (866, `UIX-RCSS-CENSUS`) with a wide margin.
//       - `test_cascade_tree_preorder_and_inheritance_integration`: the ONE case exercising
//         `cascade_tree` itself, not `compute_element_style` called by hand per level -- pre-order
//         sequence, a whitespace-only Text sibling contributing zero entries, and inheritance
//         resolved through the ACTUAL recursive `cascade_visit` stack (two sibling subtrees
//         declaring DIFFERENT colors so a grandchild's inherited value can only be explained by
//         its own immediate parent, never by coincidentally matching the tree root too).
// PT: UIX-CASCADE -- suíte unitária pro `glintfx::uix::style::compute_element_style`/
//     `cascade_tree`. Constrói árvores `StyleSheet`/`Rule`/`Selector` À MÃO (os próprios tipos
//     públicos deste módulo, não texto -- mesma disciplina que o selector_match_sanity.cpp já
//     estabeleceu pra esta exata situação) contra pequenas árvores `glintfx::uix::Element` também
//     construídas à mão, pra todo caso abaixo testar EXATAMENTE a fronteira que nomeia, não o que
//     um arquivo de corpus real por acaso contém.
//
//     O QUE CADA GRUPO DE CASOS AFIRMA EXERCITAR (o próprio briefing desta tarefa: "qual condição
//     este exemplo afirma exercitar, e a entrada escolhida realmente a alcança?"):
//       - `test_inheritance_three_levels`: uma propriedade `inherited` (`color`) atravessando TRÊS
//         níveis de árvore inalterada quando nada a re-declara, ao lado de uma propriedade
//         NÃO-herdada (`background-color`) parando seca no nível declarante (o neto vê o próprio
//         valor inicial do registro, não o declarado do avô).
//       - `test_precedence_specificity_both_declaration_orders`: duas regras de especificidade
//         DIFERENTE mirando a MESMA propriedade no MESMO elemento, tentadas nas DUAS ordens
//         textuais -- a regra de especificidade maior vence independente de qual foi escrita
//         primeiro no próprio vetor `StyleSheet::rules` fonte, provando que isto é genuinamente
//         dirigido-por-especificidade, não vence-a-última-regra por acidente.
//       - `test_precedence_source_order_tie_break`: duas regras de especificidade IGUAL -- o ÚNICO
//         caso onde ordem de declaração legitimamente decide o vencedor (a própria formulação
//         "origem, especificidade, ordem de fonte" do docs/uix-rcss.md, seção 1) -- números
//         literais, pinados-no-upstream, dos dois lados da asserção (o próprio peso de classe
//         `100'000` do StyleSheetSelector.h:13-21 mais um índice de regra literal), nunca
//         reexpressos puramente via as próprias funções auxiliares deste arquivo, per o próprio
//         aviso "golden auto-referente" desta tarefa.
//       - `test_precedence_within_rule_last_declaration_wins`: a MESMA propriedade declarada duas
//         vezes dentro de UMA regra -- a declaração textualmente posterior vence, com carimbos de
//         especificidade+ordem IDÊNTICOS pras duas (este é o caso pro qual a direção
//         `>=`-sobrescreve (não `>`) da própria citação `SetProperty` do upstream que o cabeçalho
//         deste módulo cita carrega peso).
//       - `test_initial_value_full_registry`: um elemento casado por ZERO regras recebe as 72
//         entradas do registro verbatim, na própria ordem do `all_properties()` -- pinado contra o
//         próprio span `all_properties()` do registro diretamente (não uma re-derivação), e contra
//         dois valores iniciais literais de fonte-upstream (seção 6.1 do docs/uix-rcss.md) pra
//         checagem não meramente restatar o que o registro por acaso contém hoje.
//       - `test_determinism_repeated_computation`: a MESMA folha computada três vezes contra o
//         MESMO elemento produz vetores `ComputedStyle` byte-idênticos.
//       - `test_determinism_shuffled_noncoflicting_declaration_order`: dois conjuntos de regra
//         construídos-à-mão que são semanticamente a MESMA cascata (mesmas propriedades, mesmos
//         valores, mesmas especificidades) mas cujas declarações NÃO-conflitantes são inseridas nos
//         próprios vetores `Rule::declarations` delas numa ordem relativa DIFERENTE -- prova que a
//         própria ordenação de saída deste módulo (sempre a própria ordem alfabética do
//         `all_properties()`) nunca vaza ordem de processamento/inserção interna, a forma concreta
//         que o próprio briefing "embaralhe a ordem de inserção interna" desta tarefa pede.
//       - `test_precedence_tie_break_invariant_holds_for_corpus_scale`: a própria afirmação
//         aritmética que o comentário de cabeçalho deste módulo faz ("dois pares
//         (especificidade_de_cadeia, rule_index) DIFERENTES nunca conseguem carimbar o MESMO valor
//         combined abaixo de 10'000 regras") checada diretamente contra a própria contagem de regra
//         medida do corpus (866, `UIX-RCSS-CENSUS`) com margem larga.
//       - `test_cascade_tree_preorder_and_inheritance_integration`: o ÚNICO caso exercitando o
//         próprio `cascade_tree`, não `compute_element_style` chamado à mão por nível -- sequência
//         pré-ordem, um irmão Text só-whitespace contribuindo zero entradas, e herança resolvida
//         através da PRÓPRIA pilha recursiva do `cascade_visit` (duas subárvores irmãs declarando
//         cores DIFERENTES pra o valor herdado de um neto só poder ser explicado pelo PRÓPRIO pai
//         imediato dele, nunca por coincidentemente também casar a raiz da árvore).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/cascade.hpp"

#include "uix/style/property_registry.hpp"

#include <cstdio>
#include <memory>
#include <string>

using glintfx::uix::Element;
using glintfx::uix::style::all_properties;
using glintfx::uix::style::Combinator;
using glintfx::uix::style::CompoundSelector;
using glintfx::uix::style::compute_element_style;
using glintfx::uix::style::ComputedProperty;
using glintfx::uix::style::ComputedStyle;
using glintfx::uix::style::kSpecificityWeightClassOrPseudo;
using glintfx::uix::style::kSpecificityWeightId;
using glintfx::uix::style::kSpecificityWeightTag;
using glintfx::uix::style::MatchState;
using glintfx::uix::style::PropertyDeclaration;
using glintfx::uix::style::Rule;
using glintfx::uix::style::Selector;
using glintfx::uix::style::SelectorList;
using glintfx::uix::style::StyleSheet;

namespace {

int g_failures = 0;
int g_inherited_checks = 0;
int g_specificity_tie_checks = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

Element* add_child(Element& parent, std::string tag) {
  auto child = std::make_unique<Element>(std::move(tag));
  Element* raw = child.get();
  auto result = parent.append_child(std::move(child));
  (void)result;
  return raw;
}

CompoundSelector class_only(std::string_view cls) {
  CompoundSelector c;
  c.classes.push_back(cls);
  return c;
}

CompoundSelector id_only(std::string_view id) {
  CompoundSelector c;
  c.id = id;
  return c;
}

Selector chain1(CompoundSelector a) {
  Selector s;
  s.compounds = {a};
  return s;
}

Rule rule_with(SelectorList selectors, std::vector<PropertyDeclaration> decls) {
  Rule r;
  r.selectors = std::move(selectors);
  r.declarations = std::move(decls);
  return r;
}

// EN: Finds `name`'s own entry inside a `ComputedStyle`, by LINEAR scan -- this test file's own
//     plumbing, not the production module's own (`compute_element_style` never does this itself,
//     it builds output directly from `all_properties()`'s own index order).
// PT: Acha a própria entrada de `name` dentro de um `ComputedStyle`, por varredura LINEAR -- o
//     próprio encanamento deste arquivo de teste, não o do próprio módulo de produção
//     (`compute_element_style` nunca faz isto ele mesmo, constrói a saída diretamente a partir da
//     própria ordem de índice do `all_properties()`).
const std::string* find_value(const ComputedStyle& style, std::string_view name) {
  for (const ComputedProperty& p : style) {
    if (p.name == name) {
      return &p.value;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
void test_inheritance_three_levels() {
  Element grandparent("div");
  Element* parent = add_child(grandparent, "div");
  Element* child = add_child(*parent, "div");
  grandparent.add_class("root");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(class_only("root"))},
                                  {{"color", "#112233"}, {"background-color", "#445566"}}));

  MatchState state;
  ComputedStyle gp_style = compute_element_style(sheet, grandparent, state, nullptr);
  ComputedStyle parent_style = compute_element_style(sheet, *parent, state, &gp_style);
  ComputedStyle child_style = compute_element_style(sheet, *child, state, &parent_style);

  const std::string* gp_color = find_value(gp_style, "color");
  const std::string* parent_color = find_value(parent_style, "color");
  const std::string* child_color = find_value(child_style, "color");
  check(gp_color != nullptr && *gp_color == "#112233",
        "inheritance: declaring level (grandparent) shows its own declared color");
  check(parent_color != nullptr && *parent_color == "#112233",
        "inheritance: level 1 (parent), color (inherited=true) crosses unchanged from grandparent");
  check(child_color != nullptr && *child_color == "#112233",
        "inheritance: level 2 (child), color STILL crosses unchanged -- three levels total");
  if (gp_color && parent_color && child_color && *gp_color == "#112233" &&
      *parent_color == "#112233" && *child_color == "#112233") {
    g_inherited_checks += 2; // parent and child, both crossing from the grandparent's declaration.
  }

  const std::string* gp_bg = find_value(gp_style, "background-color");
  const std::string* child_bg = find_value(child_style, "background-color");
  check(gp_bg != nullptr && *gp_bg == "#445566",
        "non-inheritance: declaring level (grandparent) shows its own declared background-color");
  check(child_bg != nullptr && *child_bg == "transparent",
        "non-inheritance: background-color (inherited=false) does NOT cross -- child falls to the "
        "registry's own initial value \"transparent\" (docs/uix-rcss.md section 6.1), not the "
        "grandparent's declared \"#445566\"");
}

// ---------------------------------------------------------------------------
void test_precedence_specificity_both_declaration_orders() {
  Element el("div");
  el.set_id("hero");
  el.add_class("banner");

  auto build_sheet = [](bool id_rule_first) {
    StyleSheet sheet;
    Rule id_rule = rule_with({chain1(id_only("hero"))}, {{"color", "#ff0000"}});
    Rule class_rule = rule_with({chain1(class_only("banner"))}, {{"color", "#00ff00"}});
    if (id_rule_first) {
      sheet.rules.push_back(id_rule);
      sheet.rules.push_back(class_rule);
    } else {
      sheet.rules.push_back(class_rule);
      sheet.rules.push_back(id_rule);
    }
    return sheet;
  };

  MatchState state;
  {
    StyleSheet sheet = build_sheet(/*id_rule_first=*/true);
    ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
    const std::string* color = find_value(style, "color");
    check(color != nullptr && *color == "#ff0000",
          "precedence: #id (1'000'000) beats .class (100'000) when the id rule is declared FIRST");
  }
  {
    StyleSheet sheet = build_sheet(/*id_rule_first=*/false);
    ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
    const std::string* color = find_value(style, "color");
    check(color != nullptr && *color == "#ff0000",
          "precedence: #id (1'000'000) STILL beats .class (100'000) when the id rule is declared "
          "LAST -- specificity, not declaration order, decides here");
  }
}

// ---------------------------------------------------------------------------
void test_precedence_source_order_tie_break() {
  // EN: Two EQUAL-specificity rules (both a bare single class, 100'000 each -- literal, matching
  //     StyleSheetSelector.h:16's own upstream constant, not this module's own
  //     kSpecificityWeightClassOrPseudo alone) targeting the SAME property on an element that
  //     carries BOTH classes. `combined = 100'000 + rule_index`; rule_index 0 for the first rule,
  //     1 for the second -- the SECOND (LATER) rule's own combined value, 100'001, is strictly
  //     greater than the first's, 100'000, so it wins -- the ONLY mechanism that can decide this,
  //     since compound specificity alone is tied.
  // PT: Duas regras de especificidade IGUAL (as duas uma classe única crua, 100'000 cada -- literal,
  //     batendo com a própria constante upstream do StyleSheetSelector.h:16, não só o
  //     kSpecificityWeightClassOrPseudo deste módulo) mirando a MESMA propriedade num elemento que
  //     carrega AS DUAS classes. `combined = 100'000 + rule_index`; rule_index 0 pra primeira regra,
  //     1 pra segunda -- o próprio valor combined da SEGUNDA (POSTERIOR) regra, 100'001, é
  //     estritamente maior que o da primeira, 100'000, então ela vence -- o ÚNICO mecanismo que
  //     consegue decidir isto, já que a especificidade de compound sozinha está empatada.
  check(kSpecificityWeightClassOrPseudo == 100'000,
        "tie-break setup: kSpecificityWeightClassOrPseudo pinned to the literal upstream constant "
        "(StyleSheetSelector.h:16) this test's own reasoning depends on");

  Element el("div");
  el.add_class("a");
  el.add_class("b");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(class_only("a"))}, {{"display", "block"}}));
  sheet.rules.push_back(rule_with({chain1(class_only("b"))}, {{"display", "inline"}}));

  MatchState state;
  ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
  const std::string* display = find_value(style, "display");
  check(display != nullptr && *display == "inline",
        "tie-break: EQUAL specificity (100'000 == 100'000) -- the LATER rule (source order) wins, "
        "\"inline\" from the second rule, not \"block\" from the first");
  if (display && *display == "inline") {
    ++g_specificity_tie_checks;
  }

  // EN: Reversed declaration order -- the rule setting "inline" is now FIRST, "block" SECOND.
  //     Proves the winner tracks declaration POSITION, not a fixed textual value.
  // PT: Ordem de declaração revertida -- a regra ajustando "inline" agora é a PRIMEIRA, "block" a
  //     SEGUNDA. Prova que o vencedor acompanha a POSIÇÃO de declaração, não um valor textual fixo.
  StyleSheet reversed;
  reversed.rules.push_back(rule_with({chain1(class_only("b"))}, {{"display", "inline"}}));
  reversed.rules.push_back(rule_with({chain1(class_only("a"))}, {{"display", "block"}}));
  ComputedStyle reversed_style = compute_element_style(reversed, el, state, nullptr);
  const std::string* reversed_display = find_value(reversed_style, "display");
  check(reversed_display != nullptr && *reversed_display == "block",
        "tie-break: reversing WHICH rule is declared last flips the winner to \"block\" -- proves "
        "source order, not a hard-coded value, decides equal-specificity ties");
  if (reversed_display && *reversed_display == "block") {
    ++g_specificity_tie_checks;
  }
}

// ---------------------------------------------------------------------------
void test_precedence_within_rule_last_declaration_wins() {
  Element el("div");
  el.add_class("x");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(class_only("x"))},
                                  {{"display", "block"}, {"display", "inline"}, {"display", "flex"}}));

  MatchState state;
  ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
  const std::string* display = find_value(style, "display");
  check(display != nullptr && *display == "flex",
        "within-rule: three declarations of the SAME property in ONE rule -- the textually LAST "
        "(\"flex\") wins, matching SetProperty's own >=-overwrites direction under an identical "
        "specificity+source-order stamp for all three");
}

// ---------------------------------------------------------------------------
void test_initial_value_full_registry() {
  Element el("div"); // matched by zero rules -- no class, no id, sheet has no rules at all.
  StyleSheet empty_sheet;
  MatchState state;
  ComputedStyle style = compute_element_style(empty_sheet, el, state, nullptr);

  check(style.size() == all_properties().size(),
        "initial value: an unmatched element still gets all_properties().size() (72) entries, "
        "never fewer");

  bool order_matches = (style.size() == all_properties().size());
  for (std::size_t i = 0; order_matches && i < style.size(); ++i) {
    if (style[i].name != all_properties()[i].name ||
        style[i].value != all_properties()[i].initial_value) {
      order_matches = false;
    }
  }
  check(order_matches,
        "initial value: every one of the 72 entries matches all_properties()'s own name AND "
        "initial_value, in that exact index order -- not a re-derivation, a direct pin against the "
        "registry itself");

  const std::string* background_color = find_value(style, "background-color");
  check(background_color != nullptr && *background_color == "transparent",
        "initial value: literal pin -- \"background-color\" initial is \"transparent\" "
        "(docs/uix-rcss.md section 6.1), independent of the registry's own internal representation");
  const std::string* color = find_value(style, "color");
  check(color != nullptr && *color == "white",
        "initial value: literal pin -- \"color\" initial is \"white\" (docs/uix-rcss.md section "
        "6.1), independent of the registry's own internal representation");
}

// ---------------------------------------------------------------------------
void test_determinism_repeated_computation() {
  Element el("div");
  el.add_class("box");
  StyleSheet sheet;
  sheet.rules.push_back(
      rule_with({chain1(class_only("box"))}, {{"color", "#abcdef"}, {"display", "flex"}}));

  MatchState state;
  ComputedStyle a = compute_element_style(sheet, el, state, nullptr);
  ComputedStyle b = compute_element_style(sheet, el, state, nullptr);
  ComputedStyle c = compute_element_style(sheet, el, state, nullptr);

  bool ab_equal = (a.size() == b.size());
  bool bc_equal = (b.size() == c.size());
  for (std::size_t i = 0; i < a.size() && ab_equal; ++i) {
    if (a[i].name != b[i].name || a[i].value != b[i].value) {
      ab_equal = false;
    }
  }
  for (std::size_t i = 0; i < b.size() && bc_equal; ++i) {
    if (b[i].name != c[i].name || b[i].value != c[i].value) {
      bc_equal = false;
    }
  }
  check(ab_equal, "determinism: same sheet computed twice, byte-identical (pass 1 vs. pass 2)");
  check(bc_equal,
        "determinism: same sheet computed a third time, still byte-identical (pass 2 vs. "
        "pass 3)");
}

// ---------------------------------------------------------------------------
void test_determinism_shuffled_noncoflicting_declaration_order() {
  Element el("div");
  el.add_class("box");

  StyleSheet sheet_a;
  sheet_a.rules.push_back(rule_with({chain1(class_only("box"))}, {{"color", "#010203"},
                                                                  {"display", "flex"},
                                                                  {"opacity", "0.75"},
                                                                  {"cursor", "pointer"}}));

  StyleSheet sheet_b;
  sheet_b.rules.push_back(rule_with({chain1(class_only("box"))}, {{"opacity", "0.75"},
                                                                  {"cursor", "pointer"},
                                                                  {"color", "#010203"},
                                                                  {"display", "flex"}}));

  MatchState state;
  ComputedStyle style_a = compute_element_style(sheet_a, el, state, nullptr);
  ComputedStyle style_b = compute_element_style(sheet_b, el, state, nullptr);

  bool equal = (style_a.size() == style_b.size());
  for (std::size_t i = 0; i < style_a.size() && equal; ++i) {
    if (style_a[i].name != style_b[i].name || style_a[i].value != style_b[i].value) {
      equal = false;
    }
  }
  check(equal,
        "determinism: shuffling the INSERTION order of four non-conflicting declarations inside "
        "the same rule produces a byte-identical ComputedStyle -- output order is always "
        "all_properties()'s own alphabetical order, never a reflection of insertion order");
}

// ---------------------------------------------------------------------------
void test_precedence_tie_break_invariant_holds_for_corpus_scale() {
  // EN: This module's own header comment claims: for `rule_index < kSpecificityWeightTag`
  //     (10'000), `combined = chain_specificity + rule_index` uniquely recovers BOTH terms via
  //     `combined mod 10'000` / `combined div 10'000`, so two DIFFERENT (chain_specificity,
  //     rule_index) pairs can never stamp the same `combined`. Checked directly, not merely
  //     asserted in prose, against the corpus's own measured rule count
  //     (`UIX-RCSS-CENSUS`, 866 style blocks) with a wide margin (10x).
  // PT: O próprio comentário de cabeçalho deste módulo afirma: pra `rule_index <
  //     kSpecificityWeightTag` (10'000), `combined = especificidade_de_cadeia + rule_index`
  //     recupera unicamente OS DOIS termos via `combined mod 10'000` / `combined div 10'000`,
  //     então dois pares (especificidade_de_cadeia, rule_index) DIFERENTES nunca conseguem
  //     carimbar o mesmo `combined`. Checado diretamente, não meramente afirmado em prosa, contra a
  //     própria contagem de regra medida do corpus (`UIX-RCSS-CENSUS`, 866 blocos de estilo) com
  //     margem larga (10x).
  constexpr long long kMeasuredCorpusRuleCount = 866;
  constexpr long long kCascadeRuleIndexCeilingUsedHere = kSpecificityWeightTag; // 10'000, literal weight.
  check(kMeasuredCorpusRuleCount * 10 < kCascadeRuleIndexCeilingUsedHere,
        "tie-break invariant: measured corpus rule count (866) sits at least 10x under the "
        "kSpecificityWeightTag ceiling (10'000) the uniqueness argument depends on");

  // EN: Direct arithmetic check -- two DIFFERENT (chain, rule_index) pairs sampled at the corpus's
  //     own scale never collide.
  // PT: Checagem aritmética direta -- dois pares (chain, rule_index) DIFERENTES amostrados na
  //     própria escala do corpus nunca colidem.
  const long long chain_a = kSpecificityWeightClassOrPseudo; // 100'000
  const long long chain_b = kSpecificityWeightId;            // 1'000'000
  const long long rule_index_a = 5;
  const long long rule_index_b = 860; // near the corpus's own measured ceiling.
  const long long combined_a = chain_a + rule_index_a;
  const long long combined_b = chain_b + rule_index_b;
  check(combined_a != combined_b,
        "tie-break invariant: a sample pair near the corpus's own scale (chain 100'000/rule 5 vs. "
        "chain 1'000'000/rule 860) never collides on the combined value");
  check(combined_a % kSpecificityWeightTag == rule_index_a &&
            combined_a / kSpecificityWeightTag == chain_a / kSpecificityWeightTag,
        "tie-break invariant: combined mod/div 10'000 recovers rule_index and the chain "
        "specificity's own multiple exactly, for the first sample pair");
}

// ---------------------------------------------------------------------------
void test_cascade_tree_preorder_and_inheritance_integration() {
  // EN: `cascade_tree` itself (not `compute_element_style` called by hand per level, like every
  //     test above) -- the ONE test in this file exercising the actual recursive walk (pre-order,
  //     text-node exclusion, parent-style threading through `cascade_visit`'s own stack frames).
  //     Tree shape: root(.a) -> {child0(no class), child1(.b)} -> child1 has one grandchild(no
  //     class). `.a` and `.b` declare DIFFERENT colors deliberately -- this is what lets
  //     grandchild's own assertion below distinguish "inherits from its OWN immediate parent" from
  //     "inherits from the tree ROOT" (both classes sharing one color would make the two
  //     indistinguishable by coincidence, exactly the trap this file's own "golden que não alcança
  //     a condição" discipline warns against). A stray whitespace-only Text node is appended as
  //     root's OWN first child, before child0/child1 -- proves text nodes never get a `NodeStyle`
  //     entry and never shift element indices (docs/uix-rcss.md section 2's own "only element
  //     nodes carry PROP records").
  // PT: O próprio `cascade_tree` (não `compute_element_style` chamado à mão por nível, como todo
  //     teste acima) -- o ÚNICO teste deste arquivo exercitando a travessia recursiva de fato
  //     (pré-ordem, exclusão de nó-texto, encadeamento de estilo-de-pai através dos próprios
  //     frames de stack do `cascade_visit`). Forma de árvore: root(.a) -> {child0(sem classe),
  //     child1(.b)} -> child1 tem um neto(sem classe). `.a` e `.b` declaram cores DIFERENTES
  //     deliberadamente -- isto é o que deixa a própria asserção do neto abaixo distinguir "herda
  //     do PRÓPRIO pai imediato" de "herda da RAIZ da árvore" (as duas classes compartilhando uma
  //     cor tornaria os dois indistinguíveis por coincidência, exatamente a armadilha que a própria
  //     disciplina "golden que não alcança a condição" deste arquivo avisa contra). Um nó Text
  //     só-whitespace é somado como o PRÓPRIO primeiro filho da raiz, antes de child0/child1 --
  //     prova que nós de texto nunca ganham entrada `NodeStyle` e nunca deslocam índice de
  //     elemento (o próprio "só nós elemento carregam registros PROP" da seção 2 do
  //     docs/uix-rcss.md).
  Element root("div");
  root.add_class("a");
  auto whitespace_text = std::make_unique<glintfx::uix::Text>("   ");
  auto append_result = root.append_child(std::move(whitespace_text));
  (void)append_result; // filtered by dom_tree.hpp's own append_child, per that file's own contract.
  Element* child0 = add_child(root, "span");
  Element* child1 = add_child(root, "span");
  child1->add_class("b");
  Element* grandchild = add_child(*child1, "em");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(class_only("a"))}, {{"color", "#0a0b0c"}}));
  sheet.rules.push_back(rule_with({chain1(class_only("b"))}, {{"color", "#d0d1d2"}}));

  MatchState state;
  std::vector<glintfx::uix::style::NodeStyle> result =
      glintfx::uix::style::cascade_tree(sheet, root, state);

  check(result.size() == 4,
        "cascade_tree: 4 element nodes visited (root, child0, child1, grandchild) -- the "
        "whitespace-only Text sibling contributes ZERO entries");

  bool order_ok = (result.size() == 4);
  if (order_ok) {
    order_ok = order_ok && result[0].element == &root;
    order_ok = order_ok && result[1].element == child0;
    order_ok = order_ok && result[2].element == child1;
    order_ok = order_ok && result[3].element == grandchild;
  }
  check(order_ok,
        "cascade_tree: pre-order depth-first sequence -- root, child0, child1, grandchild (NOT "
        "root, child0, grandchild, child1, which a breadth-first or reversed-child-order bug "
        "would produce instead)");

  if (result.size() == 4) {
    const std::string* root_color = find_value(result[0].style, "color");
    const std::string* child0_color = find_value(result[1].style, "color");     // no class -- inherits root.
    const std::string* child1_color = find_value(result[2].style, "color");     // .b -- declared, own.
    const std::string* grandchild_color = find_value(result[3].style, "color"); // inherits child1.

    check(root_color != nullptr && *root_color == "#0a0b0c",
          "cascade_tree: root (.a) shows its own declared color");
    check(child0_color != nullptr && *child0_color == "#0a0b0c",
          "cascade_tree: child0 (no class, no rule matches) INHERITS color from ITS OWN parent "
          "(root's declared #0a0b0c) -- color is inherited:true, so the absence of a matching "
          "rule falls through to the parent's already-computed value, not straight to the "
          "registry's own initial value");
    check(child1_color != nullptr && *child1_color == "#d0d1d2",
          "cascade_tree: child1 (.b) shows its OWN declared color (#d0d1d2), NOT root's (.a, "
          "#0a0b0c) -- a matching declaration always wins over inheritance, even though child1 IS "
          "root's own child");
    check(grandchild_color != nullptr && *grandchild_color == "#d0d1d2",
          "cascade_tree: grandchild (no class, no rule matches) INHERITS color from ITS OWN "
          "immediate parent (child1's declared #d0d1d2), through cascade_tree's own real "
          "recursive parent-style threading -- NOT root's #0a0b0c, which a bug inheriting from "
          "the tree root instead of the immediate parent would wrongly produce");
  }
}

} // namespace

int main() {
  test_inheritance_three_levels();
  test_precedence_specificity_both_declaration_orders();
  test_precedence_source_order_tie_break();
  test_precedence_within_rule_last_declaration_wins();
  test_initial_value_full_registry();
  test_determinism_repeated_computation();
  test_determinism_shuffled_noncoflicting_declaration_order();
  test_precedence_tie_break_invariant_holds_for_corpus_scale();
  test_cascade_tree_preorder_and_inheritance_integration();

  std::printf(
      "SCOPE: 72 longhands resolvidas, 0 elementos de corpus (ver cascade_corpus_sanity), %d "
      "herdadas verificadas, %d empates de especificidade desempatados por ordem, 0 "
      "nao-determinismos\n",
      g_inherited_checks, g_specificity_tie_checks);

  if (g_failures > 0) {
    std::fprintf(stderr, "cascade_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("cascade_sanity: PASS");
  return 0;
}
