// SPDX-License-Identifier: Apache-2.0
// EN: UIX-CASCADE -- implementation. See cascade.hpp's own header comment for the full algorithm,
//     the upstream citations it is re-derived from, and the "provably equivalent" arithmetic
//     argument for why a single ascending-`rule_index` pass replaces upstream's own
//     node-tree-then-merge two-pass shape without changing the outcome.
// PT: UIX-CASCADE -- implementação. Ver o próprio comentário de cabeçalho do cascade.hpp pro
//     algoritmo completo, as citações de upstream de onde ele é re-derivado, e o argumento
//     aritmético "provavelmente equivalente" pro porquê de uma passada única, `rule_index`
//     ascendente, substituir a própria forma de duas-passadas árvore-de-nó-depois-mescla do
//     upstream sem mudar o resultado.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/cascade.hpp"

#include "uix/style/property_registry.hpp"

#include <span>

namespace glintfx::uix::style {

namespace {

// EN: One property's own currently-winning DECLARED value, tracked while walking `sheet.rules` in
//     source order -- `specificity` here is the winning declaration's own CHAIN specificity ALONE,
//     never summed with a rule position (see cascade.hpp's own header comment, "A ceiling this file
//     used to have, and why it does not any more", `UIX-CASCADE-TETO-REGRAS`, for why an earlier
//     revision's `combined = chain_specificity + rule_index` encoding is GONE, not merely widened).
//     Comparing this field directly against a new match's own chain specificity, with `>=`
//     overwriting, matches upstream's own `SetProperty` direction (`>`-keeps existing,
//     `>=`-overwrites) exactly -- source-order tie-break needs no numeric encoding of rule position
//     at all, because `compute_element_style`'s own single ascending-`rule_index` pass below already
//     IS that ordering; visiting two equal-specificity declarations in source order and always
//     overwriting on a tie makes the LATER one win as a byproduct of the loop shape, not a property
//     of the stored number. `value` stays a `string_view` into the CALLER's own `sheet` source
//     buffer for the DURATION of this one function's own winner-selection loop only -- it is copied
//     into an owned `std::string` before this function returns anything to a caller (see
//     `compute_element_style`'s own final loop below), so this local, transient view never escapes
//     this translation unit.
// PT: O próprio valor DECLARADO atualmente-vencedor de uma propriedade, rastreado enquanto percorre
//     `sheet.rules` em ordem-fonte -- `specificity` aqui é a própria especificidade de CADEIA da
//     declaração vencedora SOZINHA, nunca somada com uma posição de regra (ver o próprio comentário
//     de cabeçalho do cascade.hpp, "Um teto que este arquivo tinha, e por que não tem mais",
//     `UIX-CASCADE-TETO-REGRAS`, pro porquê da codificação `combined = especificidade_de_cadeia +
//     rule_index` de uma revisão anterior ter SUMIDO, não meramente alargado). Comparar este campo
//     diretamente contra a própria especificidade de cadeia de um novo casamento, com `>=`
//     sobrescrevendo, bate com a própria direção do `SetProperty` do upstream exatamente
//     (`>`-mantém o existente, `>=`-sobrescreve) -- o desempate por ordem-de-fonte não precisa de
//     codificação numérica de posição-de-regra nenhuma, porque a própria passada única, `rule_index`
//     ascendente, do `compute_element_style` abaixo já É aquela ordenação; visitar duas declarações
//     de especificidade IGUAL em ordem-fonte e sempre sobrescrever em empate faz a POSTERIOR vencer
//     como subproduto da forma do laço, não uma propriedade do número guardado. `value` fica um
//     `string_view` sobre o próprio buffer-fonte do `sheet` do CHAMADOR só pela DURAÇÃO do próprio
//     laço de seleção-de-vencedor desta única função -- é copiado pra um `std::string` de posse
//     antes desta função retornar qualquer coisa a um chamador (ver o próprio laço final do
//     `compute_element_style` abaixo), então esta view local, transiente, nunca escapa desta
//     unidade de tradução.
struct DeclaredWinner {
  bool has_value = false;
  std::string_view value;
  Specificity specificity = 0;
};

} // namespace

ComputedStyle compute_element_style(const StyleSheet& sheet, const glintfx::uix::Element& element,
                                    const MatchState& state, const ComputedStyle* parent_style) {
  const std::span<const PropertyInfo> registry = all_properties();
  std::vector<DeclaredWinner> winners(registry.size());

  // EN: No running `rule_index` counter here any more (an earlier revision had one, ONLY to sum it
  //     into `combined` below -- see `UIX-CASCADE-TETO-REGRAS`, cascade.hpp's own header). Visiting
  //     `sheet.rules` in this vector's own source order, and always overwriting on `>=`, already
  //     IS the source-order tie-break -- no separate number needs to record "which position was
  //     this".
  // PT: Nenhum contador `rule_index` corrente aqui mais (uma revisão anterior tinha um, SÓ pra
  //     somá-lo no `combined` abaixo -- ver `UIX-CASCADE-TETO-REGRAS`, o próprio cabeçalho do
  //     cascade.hpp). Percorrer `sheet.rules` na própria ordem-fonte deste vetor, e sempre
  //     sobrescrever em `>=`, já É o desempate por ordem-de-fonte -- nenhum número separado precisa
  //     registrar "qual posição era essa".
  for (const Rule& rule : sheet.rules) {
    const MatchResult match = match_selector_list(rule.selectors, element, state);
    if (match.matched) {
      for (const PropertyDeclaration& decl : rule.declarations) {
        const PropertyInfo* info = find_property(decl.name);
        if (info == nullptr) {
          // EN: Defensive, not hostile -- unreachable via `parse_stylesheet` (parser.hpp's own
          //     "Declaration expansion" already guarantees every name reaching `Rule::declarations`
          //     is registry-known), same trust-boundary reasoning `selector_match.hpp`'s own
          //     `match_selector` doc-comment states for its own analogous defensive branch. A
          //     hand-built `Rule` in a test naming an unregistered property is the only realistic
          //     way to reach this branch.
          // PT: Defensivo, não hostil -- inalcançável via `parse_stylesheet` (a própria "Expansão de
          //     declaração" do parser.hpp já garante que todo nome chegando em `Rule::declarations`
          //     é conhecido do registro), mesmo raciocínio de fronteira-de-confiança que o próprio
          //     comentário de doc do `match_selector` do selector_match.hpp declara pro próprio ramo
          //     defensivo análogo dele. Uma `Rule` construída-à-mão num teste nomeando uma
          //     propriedade não-registrada é o único jeito realista de alcançar este ramo.
          continue;
        }
        const std::size_t idx = static_cast<std::size_t>(info - registry.data());
        DeclaredWinner& winner = winners[idx];
        if (!winner.has_value || match.specificity >= winner.specificity) {
          winner.has_value = true;
          winner.value = decl.value;
          winner.specificity = match.specificity;
        }
      }
    }
  }

  ComputedStyle out;
  out.reserve(registry.size());
  for (std::size_t i = 0; i < registry.size(); ++i) {
    const PropertyInfo& info = registry[i];
    std::string value;
    if (winners[i].has_value) {
      value.assign(winners[i].value);
    } else if (info.inherited && parent_style != nullptr) {
      // EN: `parent_style` is guaranteed, by every caller in this file (`cascade_tree`'s own
      //     `cascade_visit`), to have been built by THIS SAME function -- so `(*parent_style)[i]`
      //     addresses the SAME property `registry[i]` names, by construction, no name lookup
      //     needed. See cascade.hpp's own `ComputedStyle` doc-comment for this ordering guarantee.
      // PT: `parent_style` é garantido, por todo chamador deste arquivo (o próprio `cascade_visit`
      //     do `cascade_tree`), ter sido construído por ESTA MESMA função -- então
      //     `(*parent_style)[i]` endereça a MESMA propriedade que `registry[i]` nomeia, por
      //     construção, nenhuma busca por nome necessária. Ver o próprio comentário de doc do
      //     `ComputedStyle` do cascade.hpp pra esta garantia de ordenação.
      value = (*parent_style)[i].value;
    } else {
      value.assign(info.initial_value);
    }
    out.push_back(ComputedProperty{info.name, std::move(value)});
  }
  return out;
}

namespace {

// EN: Pre-order recursive walk -- see cascade.hpp's own `cascade_tree` doc-comment for why this
//     function's own recursion depth never exceeds `dom_tree.hpp`'s own `kMaxElementDepth` (256).
//     `style` is a LOCAL variable, computed once, used TWICE: (1) moved into `*out` so the caller
//     sees this node's own entry (pre-order: parent's entry before any child's), (2) its address
//     taken and passed as `parent_style` to every child's own recursive call, which is still safe
//     at that point because `style` (the local) has not gone out of scope yet -- only the COPY
//     pushed into `*out` has moved on; `cascade.hpp`'s own "Ownership" paragraph explains why every
//     `ComputedProperty::value` is an owned `std::string`, which is exactly what makes this
//     stack-local-as-inheritance-source pattern safe: a child's own inherited value is COPIED out
//     of `style` before this function ever returns, never left as a view into it.
// PT: Travessia recursiva pré-ordem -- ver o próprio comentário de doc do `cascade_tree` do
//     cascade.hpp pro porquê da própria profundidade de recursão desta função nunca exceder o
//     próprio `kMaxElementDepth` (256) do dom_tree.hpp. `style` é uma variável LOCAL, computada uma
//     vez, usada DUAS vezes: (1) movida pra dentro de `*out` pro chamador ver a própria entrada
//     deste nó (pré-ordem: entrada do pai antes de qualquer filho), (2) o próprio endereço tomado e
//     passado como `parent_style` pra toda chamada recursiva de filho, o que ainda é seguro naquele
//     ponto porque `style` (a local) ainda não saiu de escopo -- só a CÓPIA empurrada pra `*out` que
//     seguiu adiante; o próprio parágrafo "Posse" do cascade.hpp explica por que todo
//     `ComputedProperty::value` é um `std::string` de posse, que é exatamente o que torna este
//     padrão local-de-stack-como-fonte-de-herança seguro: o próprio valor herdado de um filho é
//     COPIADO pra fora de `style` antes desta função algum dia retornar, nunca deixado como uma
//     view sobre ela.
void cascade_visit(const StyleSheet& sheet, const glintfx::uix::Element& element,
                   const MatchState& state, const ComputedStyle* parent_style,
                   std::vector<NodeStyle>* out) {
  ComputedStyle style = compute_element_style(sheet, element, state, parent_style);
  out->push_back(NodeStyle{&element, style});
  for (const auto& child : element.children()) {
    if (const glintfx::uix::Element* child_el = glintfx::uix::as_element(child.get())) {
      cascade_visit(sheet, *child_el, state, &style, out);
    }
    // EN: Text nodes are skipped -- `docs/uix-rcss.md` section 2's own "only element nodes carry
    //     `PROP` records", same boundary `selector_match_corpus_sanity.cpp`'s own
    //     `collect_elements` helper already applies for an identical reason.
    // PT: Nós de texto são pulados -- o próprio "só nós elemento carregam registros `PROP`" da
    //     seção 2 do docs/uix-rcss.md, mesma fronteira que o próprio helper `collect_elements` do
    //     selector_match_corpus_sanity.cpp já aplica por um motivo idêntico.
  }
}

} // namespace

std::vector<NodeStyle> cascade_tree(const StyleSheet& sheet, const glintfx::uix::Element& root,
                                    const MatchState& state) {
  std::vector<NodeStyle> out;
  cascade_visit(sheet, root, state, nullptr, &out);
  return out;
}

} // namespace glintfx::uix::style
