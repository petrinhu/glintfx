// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SELECTOR-MATCH -- implementation. See selector_match.hpp's own header comment for the
//     full contract (specificity formula + citation, comma-list semantics, pseudo-class state,
//     complexity). This file restates ONLY the one piece of ground truth that comment defers here:
//     the exact per-compound match predicate, mirroring upstream's own `StyleSheetNode::Match`
//     (`examples/RmlUi/Source/Core/StyleSheetNode.cpp:152-179`) restricted to the 4 fields this
//     subset's `CompoundSelector` actually has (tag/id/classes/pseudo_hover -- no attributes, no
//     structural selectors, both zero-measured and out of subset), and the backtracking ancestor
//     walk mirroring `StyleSheetNode::TraverseMatch` (`:267-328`) restricted to the 2 combinators
//     this subset authorizes (`Descendant`/`Child` -- no sibling combinators, also out of subset).
// PT: UIX-SELECTOR-MATCH -- implementação. Ver o próprio comentário de cabeçalho do
//     selector_match.hpp pro contrato completo (fórmula de especificidade + citação, semântica de
//     lista-vírgula, estado de pseudo-classe, complexidade). Este arquivo restata SÓ a única peça
//     de verdade-de-chão que aquele comentário adia pra cá: o predicado exato de casamento
//     por-compound, espelhando o próprio `StyleSheetNode::Match` do upstream
//     (`examples/RmlUi/Source/Core/StyleSheetNode.cpp:152-179`) restrito aos 4 campos que o
//     `CompoundSelector` deste subconjunto de fato tem (tag/id/classes/pseudo_hover -- sem
//     atributo, sem seletor estrutural, os dois zero-medidos e fora do subconjunto), e a
//     caminhada-de-ancestral com backtrack espelhando o `StyleSheetNode::TraverseMatch`
//     (`:267-328`) restrita aos 2 combinadores que este subconjunto autoriza (`Descendant`/`Child`
//     -- sem combinador de irmão, também fora do subconjunto).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/selector_match.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>

namespace glintfx::uix::style {

namespace {

// EN: One `CompoundSelector`'s own match predicate against ONE element, structurally -- no
//     ancestor walk here (that is `match_from`'s own job, below). Mirrors upstream's
//     `StyleSheetNode::Match` (`StyleSheetNode.cpp:152-179`) field for field, restricted to this
//     subset's 4 fields:
//       - tag: empty means "no constraint" (matches any tag); non-empty requires an EXACT,
//         case-SENSITIVE match against `element.tag()` -- upstream's own `selector.tag !=
//         element->GetTagName()` (`:154`) is a plain `String` `!=`, never folding case, and
//         `dom_tree.hpp`'s own `Element::tag()` doc-comment states tags are "stored exactly as
//         given, case preserved" for the identical reason (case-folding, if any, is upstream
//         `XMLParser`/S3 territory, never this layer's).
//       - id: empty means "no constraint"; non-empty requires an EXACT match against
//         `element.id()` -- `dom_tree.hpp`'s own documented empty-value asymmetry ("an empty id
//         and no id are the SAME state") means an id-less element's `id()` is `""`, which can never
//         equal a non-empty selector id, so the "element has no id at all" case falls out of this
//         one comparison for free, with no separate `has_id()` check needed.
//       - classes: EVERY class the compound names must be present (`element.has_class`) -- ALL,
//         not any; an empty `classes` vector is vacuously satisfied (a compound with zero classes
//         named imposes zero class constraints), matching `std::all_of`-over-empty-range-is-
//         vacuously-true semantics this whole module already uses elsewhere (`dom_tree.hpp`'s own
//         whitespace-text filter, cited there).
//       - pseudo_hover: `false` means "no constraint"; `true` requires `state.hover_active` --
//         see selector_match.hpp's own "Pseudo-class state" paragraph for why this is a single
//         GLOBAL flag, not per-element.
// PT: O próprio predicado de casamento de UM `CompoundSelector` contra UM elemento,
//     estruturalmente -- nenhuma caminhada de ancestral aqui (isso é trabalho do próprio
//     `match_from`, abaixo). Espelha o próprio `StyleSheetNode::Match` do upstream
//     (`StyleSheetNode.cpp:152-179`) campo por campo, restrito aos 4 campos deste subconjunto:
//       - tag: vazia significa "sem restrição" (casa qualquer tag); não-vazia exige casamento
//         EXATO, sensível-a-CAIXA contra `element.tag()` -- o próprio `selector.tag !=
//         element->GetTagName()` do upstream (`:154`) é um `!=` de `String` puro, nunca dobrando
//         caixa, e o próprio comentário de doc de `Element::tag()` do `dom_tree.hpp` declara que
//         tags são "guardadas exatamente como recebidas, caixa preservada" pelo motivo idêntico
//         (dobrar caixa, se houver, é território do `XMLParser`/S3 upstream, nunca desta camada).
//       - id: vazio significa "sem restrição"; não-vazio exige casamento EXATO contra
//         `element.id()` -- a própria assimetria de valor-vazio documentada do `dom_tree.hpp` ("um
//         id vazio e sem id são o MESMO estado") significa que o `id()` de um elemento sem id é
//         `""`, que nunca consegue igualar um id de seletor não-vazio, então o caso "elemento não
//         tem id nenhum" cai de graça dessa única comparação, sem checagem `has_id()` separada
//         precisar.
//       - classes: TODA classe que o compound nomeia precisa estar presente (`element.has_class`)
//         -- TODAS, não qualquer uma; um vetor `classes` vazio é satisfeito vacuamente (um compound
//         com zero classes nomeadas impõe zero restrição de classe), batendo com a semântica
//         `std::all_of`-sobre-range-vazio-é-vacuamente-verdadeiro que este módulo inteiro já usa em
//         outro lugar (o próprio filtro de texto-só-whitespace do `dom_tree.hpp`, citado lá).
//       - pseudo_hover: `false` significa "sem restrição"; `true` exige `state.hover_active` -- ver
//         o próprio parágrafo "Estado de pseudo-classe" do selector_match.hpp pra por que isto é um
//         único flag GLOBAL, não por-elemento.
//     ⚠️ `ESC-8`: `compound.universal` is deliberately NEVER read below, on purpose, not an
//     oversight -- it carries zero matching semantics of its own. Reproducing the pin's own
//     `*`-skip (`StyleSheetParser.cpp:1105-1106`, `if (rule[start_index] == '*') start_index +=
//     1;`) faithfully means the '*' is CONSUMED at parse time and never becomes a constraint this
//     matcher needs to check -- an all-empty compound (every field above empty/false) already
//     matches ANY element unconditionally, by the four bullets above, each independently: empty
//     tag is "no constraint", empty id is "no constraint", an empty `classes` vector is vacuously
//     satisfied by `std::all_of`, and `pseudo_hover == false` waives the hover check. `universal`
//     changes NONE of those four rules -- it is pure bookkeeping for a caller/test that wants to
//     ask "was this compound literally the bare `*` form" (parser.hpp's own `CompoundSelector::
//     universal` doc-comment has the full account, including the `*div` fall-through accident this
//     field also records). Zero code change was required in this function for `ESC-8` to land.
//     ⚠️ `ESC-8`: `compound.universal` deliberadamente NUNCA é lido abaixo, de propósito, não um
//     descuido -- ele não carrega semântica de casamento nenhuma própria. Reproduzir fielmente o
//     próprio skip-de-`*` do pin (`StyleSheetParser.cpp:1105-1106`, `if (rule[start_index] == '*')
//     start_index += 1;`) significa que o `*` é CONSUMIDO em tempo de parse e nunca vira uma
//     restrição que este casador precisa checar -- um compound inteiramente vazio (todo campo
//     acima vazio/falso) já casa QUALQUER elemento incondicionalmente, pelos quatro bullets acima,
//     cada um independentemente: tag vazia é "sem restrição", id vazio é "sem restrição", um vetor
//     `classes` vazio é satisfeito vacuamente pelo `std::all_of`, e `pseudo_hover == false` dispensa
//     a checagem de hover. `universal` não muda NENHUMA dessas quatro regras -- é contabilidade
//     pura pra um chamador/teste que quer perguntar "este compound era literalmente a forma `*`
//     crua" (o próprio comentário de doc de `CompoundSelector::universal` do parser.hpp tem o
//     relato completo, incluindo o acidente de fall-through do `*div` que este campo também
//     registra). Zero mudança de código foi necessária nesta função pra a `ESC-8` aterrissar.
bool compound_matches(const CompoundSelector& compound, const glintfx::uix::Element& element,
                      const MatchState& state) {
  if (!compound.tag.empty() && compound.tag != element.tag()) {
    return false;
  }
  if (!compound.id.empty() && compound.id != element.id()) {
    return false;
  }
  if (!std::all_of(compound.classes.begin(), compound.classes.end(),
                   [&element](std::string_view cls) { return element.has_class(cls); })) {
    return false;
  }
  if (compound.pseudo_hover && !state.hover_active) {
    return false;
  }
  return true;
}

// EN: Does `selector` match, starting the check at `compound_index` against `element`? Mirrors
//     upstream's `StyleSheetNode::Match` + `TraverseMatch` recursive-pair shape (`:152-179` /
//     `:267-328`): first, does the compound AT `compound_index` match `element` itself; if
//     `compound_index == 0` (the leftmost/outermost compound), that alone is enough -- the whole
//     chain is satisfied, mirroring upstream's own base case (`!parent->parent`, i.e. "no more
//     ancestor compounds left to satisfy"). Otherwise, the combinator BETWEEN `compounds[
//     compound_index - 1]` and `compounds[compound_index]` (per `Selector`'s own doc-comment,
//     "combinators[i] joins compounds[i] and compounds[i+1]") decides how the search for the NEXT
//     compound (index - 1) proceeds:
//       - `Child`: only the DIRECT parent is tried. No parent (a root element) means immediate
//         failure -- mirrors upstream's own "If the node has a child combinator we must match this
//         first ancestor... return false" early-exit (`:285-286`).
//       - `Descendant`: EVERY ancestor, walking outward from the immediate parent, is tried in
//         turn; the first one for which BOTH "the compound at index-1 matches it" AND "the REST of
//         the chain (recursively) also matches starting from it" succeed wins. This is genuine
//         backtracking, not "find the nearest ancestor satisfying compounds[index-1] and stop": a
//         nearest ancestor that satisfies compounds[index-1] but for which the REST of the chain
//         then fails must not short-circuit the search -- mirrors upstream's own for-loop over
//         `element->GetParentNode()` that tries `parent->Match(element, scope) &&
//         parent->TraverseMatch(element, scope)` together, in that order, at every ancestor
//         (`:280-283`).
// PT: `selector` casa, começando a checagem em `compound_index` contra `element`? Espelha a forma
//     de par-recursivo `StyleSheetNode::Match` + `TraverseMatch` do upstream (`:152-179` /
//     `:267-328`): primeiro, o compound EM `compound_index` casa com o próprio `element`; se
//     `compound_index == 0` (o compound mais-à-esquerda/mais-externo), isso sozinho já basta -- a
//     cadeia inteira está satisfeita, espelhando o próprio caso-base do upstream (`!parent->
//     parent`, ou seja, "não sobra compound de ancestral nenhum pra satisfazer"). Senão, o
//     combinador ENTRE `compounds[compound_index - 1]` e `compounds[compound_index]` (pelo próprio
//     comentário de doc de `Selector`, "combinators[i] une compounds[i] e compounds[i+1]") decide
//     como a busca do PRÓXIMO compound (index - 1) prossegue:
//       - `Child`: só o pai DIRETO é tentado. Sem pai (um elemento raiz) significa falha imediata
//         -- espelha a própria saída-antecipada "Se o nó tem combinador filho precisamos casar
//         este primeiro ancestral... retorna false" do upstream (`:285-286`).
//       - `Descendant`: TODO ancestral, caminhando pra fora a partir do pai imediato, é tentado em
//         sequência; o primeiro pro qual TANTO "o compound em index-1 casa com ele" QUANTO "o RESTO
//         da cadeia (recursivamente) também casa começando dele" tenham sucesso vence. Isto é
//         backtracking genuíno, não "ache o ancestral mais próximo satisfazendo compounds[index-1]
//         e pare": um ancestral mais próximo que satisfaz compounds[index-1] mas pro qual o RESTO
//         da cadeia então falha não pode encerrar a busca antecipadamente -- espelha o próprio
//         laço-for do upstream sobre `element->GetParentNode()` que tenta `parent->Match(element,
//         scope) && parent->TraverseMatch(element, scope)` juntos, nessa ordem, em todo ancestral
//         (`:280-283`).
bool match_from(const Selector& selector, std::size_t compound_index,
                const glintfx::uix::Element& element, const MatchState& state) {
  if (!compound_matches(selector.compounds[compound_index], element, state)) {
    return false;
  }
  if (compound_index == 0) {
    return true;
  }
  const Combinator combinator = selector.combinators[compound_index - 1];
  switch (combinator) {
    case Combinator::Child: {
      const glintfx::uix::Element* parent = element.parent();
      if (parent == nullptr) {
        return false;
      }
      return match_from(selector, compound_index - 1, *parent, state);
    }
    case Combinator::Descendant: {
      for (const glintfx::uix::Element* ancestor = element.parent(); ancestor != nullptr;
           ancestor = ancestor->parent()) {
        if (match_from(selector, compound_index - 1, *ancestor, state)) {
          return true;
        }
      }
      return false;
    }
  }
  // EN: Unreachable -- `Combinator` is a closed 2-value enum (see selector_match.hpp's own
  //     "Deliberately not this file's job", sibling combinators are out of subset), and both
  //     values are handled above. Kept explicit rather than `[[fallthrough]]`/no-return to satisfy
  //     `-Wreturn-type` under every compiler this project targets without relying on
  //     enum-exhaustiveness diagnostics alone.
  // PT: Inalcançável -- `Combinator` é um enum fechado de 2 valores (ver o próprio "Deliberadamente
  //     não é trabalho deste arquivo" do selector_match.hpp, combinador de irmão está fora do
  //     subconjunto), e os dois valores são tratados acima. Mantido explícito em vez de
  //     `[[fallthrough]]`/sem-retorno pra satisfazer `-Wreturn-type` em todo compilador que este
  //     projeto alveja sem depender só de diagnóstico de exaustividade-de-enum.
  return false;
}

} // namespace

// EN: `ESC-8`: `compound.universal` is deliberately never read below either -- same reasoning as
//     `compound_matches`'s own doc-comment above. A universal-only compound (every OTHER field
//     empty/false) already sums to 0 by this arithmetic as written, with no branch needed for it:
//     each `if` below only ADDS weight when its own field is non-empty/true, so a compound that is
//     `*` alone (nothing else set) simply skips every branch and falls through to `return weight`
//     at its initial 0 -- exactly `docs/rmlx-subset.md`'s own table entry for the universal form
//     ("0" specificity), by construction, not by a special case this function adds for it.
// PT: `ESC-8`: `compound.universal` deliberadamente nunca é lido abaixo também -- mesmo raciocínio
//     do próprio comentário de doc do `compound_matches` acima. Um compound só-universal (todo
//     OUTRO campo vazio/falso) já soma 0 por esta aritmética como está escrita, sem branch nenhum
//     precisar pra isso: cada `if` abaixo só SOMA peso quando o próprio campo é não-vazio/verdadeiro,
//     então um compound que é `*` sozinho (nada mais setado) simplesmente pula todo branch e cai no
//     `return weight` no próprio 0 inicial -- exatamente a própria entrada de tabela da forma
//     universal do `docs/rmlx-subset.md` ("0" de especificidade), por construção, não por um caso
//     especial que esta função soma pra ele.
Specificity compound_specificity(const CompoundSelector& compound) {
  Specificity weight = 0;
  if (!compound.tag.empty()) {
    weight += kSpecificityWeightTag;
  }
  if (!compound.id.empty()) {
    weight += kSpecificityWeightId;
  }
  weight += kSpecificityWeightClassOrPseudo * static_cast<Specificity>(compound.classes.size());
  if (compound.pseudo_hover) {
    weight += kSpecificityWeightClassOrPseudo;
  }
  return weight;
}

Specificity selector_specificity(const Selector& selector) {
  return std::accumulate(selector.compounds.begin(), selector.compounds.end(),
                         Specificity{0},
                         [](Specificity acc, const CompoundSelector& compound) {
                           return acc + compound_specificity(compound);
                         });
}

MatchResult match_selector(const Selector& selector, const glintfx::uix::Element& element,
                           const MatchState& state) {
  if (selector.compounds.empty()) {
    return MatchResult{};
  }
  const std::size_t last = selector.compounds.size() - 1;
  if (!match_from(selector, last, element, state)) {
    return MatchResult{};
  }
  return MatchResult{true, selector_specificity(selector)};
}

MatchResult match_selector_list(const SelectorList& list, const glintfx::uix::Element& element,
                                const MatchState& state) {
  MatchResult best;
  for (const Selector& selector : list) {
    const MatchResult candidate = match_selector(selector, element, state);
    if (candidate.matched && (!best.matched || candidate.specificity > best.specificity)) {
      best = candidate;
    }
  }
  return best;
}

} // namespace glintfx::uix::style
