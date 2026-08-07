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
//       - `test_precedence_holds_beyond_former_rule_index_ceiling` (`UIX-CASCADE-TETO-REGRAS`):
//         a RETIRED encoding (`combined = chain_specificity + rule_index`, summed into ONE integer)
//         used to silently INVERT precedence once two rules' own `rule_index` gap reached their
//         specificity gap -- this test pins the exact former failure boundary (one
//         `kSpecificityWeightTag` gap, the smallest possible) directly against
//         `compute_element_style`, at three points: one step BELOW the boundary (must always have
//         passed -- proves this is not a fresh regression), EXACTLY at it (the retired encoding's
//         own first failure point, a false tie), and one step BEYOND it (a clean inversion).
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
//       - `test_precedence_holds_beyond_former_rule_index_ceiling` (`UIX-CASCADE-TETO-REGRAS`): uma
//         codificação APOSENTADA (`combined = especificidade_de_cadeia + rule_index`, somada num
//         inteiro só) costumava INVERTER a precedência em silêncio assim que a diferença de
//         `rule_index` de duas regras alcançava a diferença de especificidade delas -- este teste
//         prova a própria fronteira de falha anterior exata (uma gap de um `kSpecificityWeightTag`,
//         a menor possível) diretamente contra o `compute_element_style`, em três pontos: um passo
//         ABAIXO da fronteira (sempre tinha que ter passado -- prova que isto não é uma regressão
//         nova), EXATAMENTE nela (o próprio primeiro ponto de falha da codificação aposentada, um
//         empate falso), e um passo ALÉM dela (uma inversão limpa).
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

// EN: A tag-only compound -- `kSpecificityWeightTag` (10'000) ALONE, the smallest non-zero weight
//     this module defines. Used by `test_precedence_holds_beyond_former_rule_index_ceiling` below
//     to build the exact minimal specificity GAP (one `kSpecificityWeightTag`) the retired
//     `UIX-CASCADE-TETO-REGRAS` bug needed to invert.
// PT: Um compound só-tag -- `kSpecificityWeightTag` (10'000) SOZINHO, o menor peso não-zero que
//     este módulo define. Usado pelo `test_precedence_holds_beyond_former_rule_index_ceiling`
//     abaixo pra construir a GAP de especificidade mínima exata (um `kSpecificityWeightTag`) que o
//     bug aposentado `UIX-CASCADE-TETO-REGRAS` precisava pra inverter.
CompoundSelector tag_only(std::string_view tag) {
  CompoundSelector c;
  c.tag = tag;
  return c;
}

Selector chain1(CompoundSelector a) {
  Selector s;
  s.compounds = {a};
  return s;
}

// EN: A two-compound DESCENDANT chain -- specificity is the SUM of both compounds' own weights
//     (selector_match.cpp's own `selector_specificity`), so two tag-only compounds here stamp
//     `2 * kSpecificityWeightTag` (20'000), exactly DOUBLE `chain1(tag_only(...))`'s own 10'000 --
//     the same worked-example gap (`10'000` vs. `20'000`) `cascade.hpp`'s own header comment names
//     for the retired bug.
// PT: Uma cadeia DESCENDENTE de dois compounds -- a especificidade é a SOMA dos próprios pesos dos
//     dois compounds (o próprio `selector_specificity` do selector_match.cpp), então dois compounds
//     só-tag aqui carimbam `2 * kSpecificityWeightTag` (20'000), exatamente O DOBRO do próprio
//     10'000 do `chain1(tag_only(...))` -- a mesma gap do exemplo trabalhado (`10'000` vs.
//     `20'000`) que o próprio comentário de cabeçalho do cascade.hpp nomeia pro bug aposentado.
Selector descendant_chain2(CompoundSelector ancestor, CompoundSelector self) {
  Selector s;
  s.compounds = {ancestor, self};
  s.combinators = {Combinator::Descendant};
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
// EN: Builds a `StyleSheet` with a HIGH-specificity rule (a 2-tag descendant chain, 20'000, "html
//     div" -- see `descendant_chain2`) at `rule_index` 0, then `filler_count` non-matching filler
//     rules (id-only compounds naming an id no element in this test ever has -- they still
//     increment source position, they just never enter the winner-selection inner loop), then a
//     LOW-specificity rule (a single tag compound, 10'000, "div" -- see `tag_only`) declaring the
//     SAME property LAST, at `rule_index = filler_count + 1`. Both rules match `target` (tag
//     "div", child of an "html" element) -- see the call sites below for why `filler_count` is
//     chosen to land the LOW rule's own `rule_index` at exactly `gap` past the HIGH rule's.
// PT: Constrói um `StyleSheet` com uma regra de especificidade ALTA (uma cadeia descendente de 2
//     tags, 20'000, "html div" -- ver `descendant_chain2`) no `rule_index` 0, depois
//     `filler_count` regras de preenchimento não-casantes (compounds só-id nomeando um id que
//     nenhum elemento deste teste tem -- ainda incrementam a posição-fonte, só nunca entram no
//     laço interno de seleção-de-vencedor), depois uma regra de especificidade BAIXA (um compound
//     de tag único, 10'000, "div" -- ver `tag_only`) declarando a MESMA propriedade POR ÚLTIMO, no
//     `rule_index = filler_count + 1`. As duas regras casam com `target` (tag "div", filho de um
//     elemento "html") -- ver os pontos de chamada abaixo pro porquê de `filler_count` ser escolhido
//     pra pousar o próprio `rule_index` da regra BAIXA exatamente `gap` regras depois da ALTA.
StyleSheet build_high_then_low_sheet(long long gap) {
  StyleSheet sheet;
  sheet.rules.push_back(
      rule_with({descendant_chain2(tag_only("html"), tag_only("div"))}, {{"color", "#high0000"}}));
  for (long long i = 1; i < gap; ++i) {
    sheet.rules.push_back(rule_with({chain1(id_only("__filler_never_matches__"))},
                                    {{"color", "#filler00"}}));
  }
  sheet.rules.push_back(rule_with({chain1(tag_only("div"))}, {{"color", "#low00000"}}));
  return sheet;
}

// ---------------------------------------------------------------------------
void test_precedence_holds_beyond_former_rule_index_ceiling() {
  // EN: `UIX-CASCADE-TETO-REGRAS` -- a RETIRED revision of `compute_element_style` stamped
  //     `combined = chain_specificity + rule_index` into ONE integer per winning declaration. Once
  //     the GAP between the LOW rule's own `rule_index` and the HIGH rule's reached
  //     `kSpecificityWeightTag` (10'000 -- the exact specificity gap between a 1-tag chain and a
  //     2-tag chain, the smallest gap two DIFFERENT chain specificities can ever have), that sum
  //     let the LOW (weaker) rule outrank the HIGH (stronger, DOUBLE the specificity) one -- a real
  //     INVERSION, not upstream's own bounded ambiguity-between-equals. This test pins the exact
  //     former failure boundary directly against `compute_element_style` (never against the retired
  //     formula's own arithmetic in isolation), at three points named by this task's own "fronteira
  //     exata E um passo pra fora" discipline:
  //       - `gap = kSpecificityWeightTag - 1` (9'999): one step BELOW the boundary -- HIGH must
  //         win. Sanity baseline: this case passed even under the retired formula, so a failure
  //         here would mean this test itself is miscalibrated, not that the ceiling moved.
  //       - `gap = kSpecificityWeightTag` (10'000): EXACTLY at the boundary -- HIGH must win. This
  //         is the retired formula's own FIRST failure point: `combined_low` (10'000 + 10'000) tied
  //         `combined_high` (20'000 + 0) exactly, and the retired `>=`-overwrites direction made
  //         the LATER-processed (LOW) rule win a tie that was never a legitimate tie (the two chain
  //         specificities are NOT equal, one is double the other).
  //       - `gap = kSpecificityWeightTag + 1` (10'001): one step BEYOND the boundary -- HIGH must
  //         win. This is the retired formula's own clean inversion case (`combined_low` = 20'001 >
  //         `combined_high` = 20'000), the worked example `cascade.hpp`'s own header comment names.
  // PT: `UIX-CASCADE-TETO-REGRAS` -- uma revisão APOSENTADA do `compute_element_style` carimbava
  //     `combined = especificidade_de_cadeia + rule_index` num inteiro só por declaração vencedora.
  //     Assim que a GAP entre o próprio `rule_index` da regra BAIXA e o da ALTA alcançava
  //     `kSpecificityWeightTag` (10'000 -- a gap de especificidade exata entre uma cadeia de 1 tag
  //     e uma de 2 tags, a menor gap que duas especificidades de cadeia DIFERENTES conseguem ter),
  //     aquela soma deixava a regra BAIXA (mais fraca) superar a ALTA (mais forte, O DOBRO da
  //     especificidade) -- uma INVERSÃO de verdade, não a própria ambiguidade limitada-entre-iguais
  //     do upstream. Este teste prova a própria fronteira de falha anterior exata diretamente contra
  //     o `compute_element_style` (nunca contra a própria aritmética da fórmula aposentada isolada),
  //     em três pontos nomeados pela própria disciplina "fronteira exata E um passo pra fora" desta
  //     tarefa:
  //       - `gap = kSpecificityWeightTag - 1` (9'999): um passo ABAIXO da fronteira -- ALTA precisa
  //         vencer. Linha de base de sanidade: este caso já passava mesmo sob a fórmula aposentada,
  //         então uma falha aqui significaria que este próprio teste está descalibrado, não que a
  //         fronteira se moveu.
  //       - `gap = kSpecificityWeightTag` (10'000): EXATAMENTE na fronteira -- ALTA precisa vencer.
  //         Este é o próprio PRIMEIRO ponto de falha da fórmula aposentada: `combined_low` (10'000 +
  //         10'000) empatava `combined_high` (20'000 + 0) exatamente, e a própria direção
  //         `>=`-sobrescreve aposentada fazia a regra processada DEPOIS (BAIXA) vencer um empate que
  //         nunca foi legítimo (as duas especificidades de cadeia NÃO são iguais, uma é o dobro da
  //         outra).
  //       - `gap = kSpecificityWeightTag + 1` (10'001): um passo ALÉM da fronteira -- ALTA precisa
  //         vencer. Este é o próprio caso de inversão limpa da fórmula aposentada (`combined_low` =
  //         20'001 > `combined_high` = 20'000), o exemplo trabalhado que o próprio comentário de
  //         cabeçalho do cascade.hpp nomeia.
  Element root_html("html");
  Element* target = add_child(root_html, "div");
  MatchState state;

  const struct {
    long long gap;
    const char* label;
  } kPoints[] = {
      {kSpecificityWeightTag - 1, "one step BELOW the former ceiling (9'999)"},
      {kSpecificityWeightTag, "EXACTLY at the former ceiling (10'000)"},
      {kSpecificityWeightTag + 1, "one step BEYOND the former ceiling (10'001)"},
  };

  for (const auto& point : kPoints) {
    StyleSheet sheet = build_high_then_low_sheet(point.gap);
    ComputedStyle style = compute_element_style(sheet, *target, state, nullptr);
    const std::string* color = find_value(style, "color");
    check(color != nullptr && *color == "#high0000",
          std::string("former ceiling: ") + point.label +
              " -- the HIGH-specificity rule (2-tag chain, 20'000) must still beat the "
              "LOW-specificity rule (1-tag chain, 10'000) declared LATER, regardless of how far "
              "apart their own rule_index values are");
  }
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

// ---------------------------------------------------------------------------
// EN: `UIX-INLINE-STYLE` -- the `style="..."` attribute's OWN precedence: it wins over EVERY
//     `StyleSheet` rule matching this element, REGARDLESS of that rule's own specificity -- real
//     upstream's own `ElementStyle::GetLocalProperty` (`examples/RmlUi/Source/Core/
//     ElementStyle.cpp:48-60`) checks `inline_properties` FIRST, unconditionally, before ever
//     consulting the cascade-matched `definition` at all; there is no numeric specificity an
//     inline declaration is compared against. This test builds the STRONGEST possible rule
//     opponent this module can construct -- an `#id` selector (`kSpecificityWeightId`, 1'000'000,
//     the single highest per-compound weight this module defines) -- and proves the inline
//     attribute still wins, so this is never "inline just happens to have a bigger number", it is
//     a genuinely different, unconditional mechanism.
// PT: `UIX-INLINE-STYLE` -- a PRÓPRIA precedência do atributo `style="..."`: ela vence QUALQUER
//     regra de `StyleSheet` que case com este elemento, INDEPENDENTE da própria especificidade
//     daquela regra -- o próprio `ElementStyle::GetLocalProperty` do upstream real
//     (`examples/RmlUi/Source/Core/ElementStyle.cpp:48-60`) checa `inline_properties` PRIMEIRO,
//     incondicionalmente, antes de sequer consultar o `definition` casado-pela-cascata; não existe
//     especificidade numérica nenhuma contra a qual uma declaração inline é comparada. Este teste
//     constrói o oponente de regra MAIS FORTE que este módulo consegue construir -- um seletor
//     `#id` (`kSpecificityWeightId`, 1'000'000, o único maior peso por-compound que este módulo
//     define) -- e prova que o atributo inline AINDA vence, então isto nunca é "inline só por
//     acaso tem um número maior", é um mecanismo genuinamente diferente, incondicional.
// ---------------------------------------------------------------------------
void test_inline_style_wins_over_highest_specificity_rule() {
  Element el("div");
  el.set_id("hero");
  const bool set_ok = el.set_attribute("style", "color: #00ff00;");
  check(set_ok, "inline_style_precedence: set_attribute(\"style\", ...) succeeds");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(id_only("hero"))}, {{"color", "#ff0000"}}));

  MatchState state;
  ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
  const std::string* color = find_value(style, "color");
  check(color != nullptr && *color == "#00ff00",
        "inline_style_precedence: the style=\"...\" attribute (#00ff00) wins over an #id rule "
        "(#ff0000, kSpecificityWeightId == 1'000'000, the single highest per-compound weight this "
        "module defines) -- inline precedence is unconditional, never a specificity comparison");
}

// ---------------------------------------------------------------------------
// EN: The `style="..."` attribute only overrides the properties it ACTUALLY declares -- every
//     other property on the same element still resolves through the ordinary cascade/inheritance/
//     initial-value pipeline, completely unaffected. Proves this is a per-property patch, never a
//     blanket "ignore the cascade for this element" switch.
// PT: O atributo `style="..."` só sobrescreve as propriedades que REALMENTE declara -- toda outra
//     propriedade do mesmo elemento continua resolvendo pelo próprio pipeline comum de
//     cascata/herança/valor-inicial, completamente inafetada. Prova que isto é um remendo
//     por-propriedade, nunca um interruptor genérico "ignore a cascata pra este elemento".
// ---------------------------------------------------------------------------
void test_inline_style_only_overrides_its_own_declared_properties() {
  Element el("div");
  el.add_class("box");
  const bool set_ok = el.set_attribute("style", "color: #00ff00;");
  check(set_ok, "inline_style_scoped: set_attribute(\"style\", ...) succeeds");

  StyleSheet sheet;
  sheet.rules.push_back(
      rule_with({chain1(class_only("box"))}, {{"color", "#ff0000"}, {"display", "flex"}}));

  MatchState state;
  ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
  const std::string* color = find_value(style, "color");
  const std::string* display = find_value(style, "display");
  check(color != nullptr && *color == "#00ff00",
        "inline_style_scoped: 'color' comes from the inline attribute, not the rule");
  check(display != nullptr && *display == "flex",
        "inline_style_scoped: 'display' is untouched -- inline style never declared it, so the "
        "rule's own \"flex\" still wins normally");
}

// ---------------------------------------------------------------------------
// EN: An element with NO `style` attribute at all behaves exactly as before this item -- proves
//     `UIX-INLINE-STYLE` is additive, opt-in per element, never a behaviour change for the
//     overwhelming majority of elements that never carry this attribute (every OTHER test in this
//     file already covers this path; this case pins it explicitly, by name, so a future reader
//     never has to infer it from absence).
// PT: Um elemento SEM atributo `style` nenhum se comporta exatamente como antes deste item -- prova
//     que a `UIX-INLINE-STYLE` é aditiva, opt-in por elemento, nunca uma mudança de comportamento
//     pra esmagadora maioria dos elementos que nunca carregam este atributo (todo OUTRO teste deste
//     arquivo já cobre este caminho; este caso o prende explicitamente, por nome, pra um leitor
//     futuro nunca precisar inferir isso da ausência).
// ---------------------------------------------------------------------------
void test_no_style_attribute_is_unaffected() {
  Element el("div");
  el.add_class("box");
  check(!el.has_attribute("style"), "no_style_attribute: setup -- no 'style' attribute present");

  StyleSheet sheet;
  sheet.rules.push_back(rule_with({chain1(class_only("box"))}, {{"color", "#ff0000"}}));

  MatchState state;
  ComputedStyle style = compute_element_style(sheet, el, state, nullptr);
  const std::string* color = find_value(style, "color");
  check(color != nullptr && *color == "#ff0000",
        "no_style_attribute: the rule's own color wins normally, unaffected by UIX-INLINE-STYLE");
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
  test_precedence_holds_beyond_former_rule_index_ceiling();
  test_cascade_tree_preorder_and_inheritance_integration();
  test_inline_style_wins_over_highest_specificity_rule();
  test_inline_style_only_overrides_its_own_declared_properties();
  test_no_style_attribute_is_unaffected();

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
