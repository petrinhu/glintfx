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

// EN: `ESC-8` -- a compound whose ONLY discriminator is `universal == true`, everything else
//     default (empty tag/id/classes, hover false) -- the exact shape `parse_compound` produces for
//     a bare `*`. Named to match this file's own `tag_only`/`id_only`/`class_only` trio.
// PT: `ESC-8` -- um compound cujo ÚNICO discriminador é `universal == true`, tudo mais default (tag/
//     id/classes vazios, hover falso) -- a forma exata que `parse_compound` produz pra um `*` cru.
//     Nomeado pra casar com o próprio trio `tag_only`/`id_only`/`class_only` deste arquivo.
CompoundSelector universal_only() {
  CompoundSelector c;
  c.universal = true;
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
  // EN: `ESC-8` relabel -- this check used to be captioned "defensive baseline, unreachable via
  //     the real parser", true only of `CompoundSelector{}` LITERALLY (default `universal == false`
  //     too, so `any` would stay false and `parse_compound` would refuse it -- still an unreachable
  //     shape on its own). What changed is the ARITHMETIC's own MEANING: this exact zero-weight sum
  //     (every field empty/false) is now ALSO what a REAL, reachable compound gets --
  //     `universal_only()` below (`* { }` alone, `universal == true`, everything else identically
  //     empty) -- because `compound_specificity` never reads `universal` at all (selector_match.cpp
  //     is unchanged by `ESC-8`, see that file's own `compound_matches`/this function's own
  //     doc-comment). The zero is no longer merely defensive arithmetic that happens to also be
  //     correct; it IS the documented specificity of an authorized selector form. See
  //     `test_form_universal_specificity_is_exact_zero` below for the reachable case this same
  //     arithmetic now also covers.
  // PT: Rerótulo `ESC-8` -- esta checagem costumava ser legendada "linha de base defensiva,
  //     inalcançável via o parser real", verdadeiro só do `CompoundSelector{}` LITERAL (`universal
  //     == false` default também, então `any` ficaria falso e `parse_compound` recusaria -- ainda
  //     uma forma inalcançável sozinha). O que mudou foi o PRÓPRIO SIGNIFICADO da ARITMÉTICA: esta
  //     exata soma-zero (todo campo vazio/falso) agora é TAMBÉM o que um compound REAL, alcançável,
  //     recebe -- o `universal_only()` abaixo (`* { }` sozinho, `universal == true`, tudo mais
  //     identicamente vazio) -- porque `compound_specificity` nunca lê `universal` de jeito nenhum
  //     (o selector_match.cpp é inalterado pela `ESC-8`, ver o próprio comentário de doc de
  //     `compound_matches` daquele arquivo/desta função). O zero deixou de ser mera aritmética
  //     defensiva que por acaso também está certa; ELE É a especificidade documentada de uma forma
  //     de seletor autorizada. Ver o `test_form_universal_specificity_is_exact_zero` abaixo pro caso
  //     alcançável que esta mesma aritmética agora também cobre.
  check(compound_specificity(CompoundSelector{}) == 0,
        "compound_specificity: an entirely empty, non-universal compound weighs 0 (still "
        "unreachable via the real parser ON ITS OWN -- universal defaults to false too, so `any` "
        "stays false) -- but see test_form_universal_specificity_is_exact_zero below: this SAME "
        "zero-weight arithmetic is what a REAL, reachable universal-only compound gets, per ESC-8");

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
// EN: PINNED AGAINST THE UPSTREAM LITERAL, NOT AGAINST THIS MODULE'S OWN CONSTANT (QA finding,
//     2026-08-06 -- reproduced and closed here, cite UIX-SELECTOR-MATCH). Every check above this
//     point compares `compound_specificity(...)`/`selector_specificity(...)` against
//     `kSpecificityWeight*` -- the SAME named constant `selector_match.cpp` itself uses to compute
//     the answer. That proves INTERNAL CONSISTENCY (the function really does add up the constants
//     it claims to) but is a SELF-REFERENTIAL golden: sabotaging `kSpecificityWeightId` from
//     `1'000'000` to `100'000` moves BOTH sides of every `== kSpecificityWeightId` check together,
//     so none of them can ever go red -- proven live: the QA's own mutation on exactly that
//     constant passed all 11 tests. The two checks below are the ONE thing this file did not yet
//     have: an assertion whose RIGHT-HAND SIDE is the bare upstream number, immune to a mutation of
//     the constant itself, because nothing here reads the constant to compute the expected value.
//     Citation: `examples/RmlUi/Source/Core/StyleSheetSelector.h:13-21` -- the exact `enum` this
//     module's own `kSpecificityWeightTag`/`kSpecificityWeightClassOrPseudo`/`kSpecificityWeightId`
//     mirror (selector_match.hpp's own header comment, "The specificity algorithm").
//
//     ⚠️ KEEP BOTH KINDS OF ASSERTION, DO NOT "SIMPLIFY" ONE AWAY: the constant-vs-constant checks
//     above (`compound_specificity(id_c) == kSpecificityWeightId`) still matter -- they are what
//     proves the ARITHMETIC inside `compound_specificity`/`selector_specificity` is wired
//     correctly (no dropped term, no wrong operator). The literal-vs-constant checks here are what
//     proves the CONSTANT ITSELF still equals the number the upstream RE-study evidence says it
//     must. Removing either half re-opens exactly one of the two gaps the other half does not
//     cover -- a future reader "cleaning up the duplication" between this function and
//     `test_compound_specificity_weights` above would silently reopen the mutation this section
//     exists to close.
// PT: ANCORADO NO LITERAL DO UPSTREAM, NÃO NA PRÓPRIA CONSTANTE DESTE MÓDULO (achado do QA,
//     2026-08-06 -- reproduzido e fechado aqui, cita UIX-SELECTOR-MATCH). Toda checagem acima
//     deste ponto compara `compound_specificity(...)`/`selector_specificity(...)` contra
//     `kSpecificityWeight*` -- a MESMA constante nomeada que o próprio `selector_match.cpp` usa
//     pra computar a resposta. Isso prova CONSISTÊNCIA INTERNA (a função de fato soma as
//     constantes que afirma somar) mas é um golden AUTO-REFERENTE: sabotar
//     `kSpecificityWeightId` de `1'000'000` pra `100'000` move OS DOIS LADOS de toda checagem
//     `== kSpecificityWeightId` juntos, então nenhuma delas consegue algum dia ficar vermelha --
//     provado ao vivo: a própria mutação do QA nessa constante exata passou nos 11 testes. As duas
//     checagens abaixo são a ÚNICA coisa que este arquivo ainda não tinha: uma asserção cujo LADO
//     DIREITO é o número cru do upstream, imune a uma mutação da própria constante, porque nada
//     aqui lê a constante pra computar o valor esperado. Citação:
//     `examples/RmlUi/Source/Core/StyleSheetSelector.h:13-21` -- o `enum` exato que os próprios
//     `kSpecificityWeightTag`/`kSpecificityWeightClassOrPseudo`/`kSpecificityWeightId` deste
//     módulo espelham (o próprio comentário de cabeçalho do selector_match.hpp, "O algoritmo de
//     especificidade").
//
//     ⚠️ MANTENHA OS DOIS TIPOS DE ASSERÇÃO, NÃO "SIMPLIFIQUE" TIRANDO UM: as checagens
//     constante-contra-constante acima (`compound_specificity(id_c) == kSpecificityWeightId`)
//     ainda importam -- são o que prova que a ARITMÉTICA dentro de
//     `compound_specificity`/`selector_specificity` está fiada corretamente (nenhum termo
//     derrubado, nenhum operador errado). As checagens literal-contra-constante aqui são o que
//     prova que a PRÓPRIA CONSTANTE ainda é igual ao número que a evidência de RE-estudo do
//     upstream diz que precisa ser. Remover qualquer uma das metades reabre exatamente um dos dois
//     buracos que a outra metade não cobre -- um leitor futuro "limpando a duplicação" entre esta
//     função e a `test_compound_specificity_weights` acima reabriria em silêncio a mutação que
//     esta seção existe pra fechar.
// ---------------------------------------------------------------------------
void test_specificity_pinned_against_upstream_literals_and_order() {
  // EN: The bare upstream numbers, StyleSheetSelector.h:15/16/19 -- `Tag = 10'000`,
  //     `Class = Attribute = PseudoClass = 100'000`, `ID = 1'000'000`. NOT re-expressed via the
  //     kSpecificityWeight* constants on either side of `==` -- that is the whole point of this
  //     check.
  // PT: Os números crus do upstream, StyleSheetSelector.h:15/16/19 -- `Tag = 10'000`,
  //     `Class = Attribute = PseudoClass = 100'000`, `ID = 1'000'000`. NÃO reexpressos via as
  //     constantes kSpecificityWeight* de nenhum dos lados do `==` -- esse é o ponto inteiro desta
  //     checagem.
  check(kSpecificityWeightTag == 10'000,
        "kSpecificityWeightTag pinned to the LITERAL upstream value 10'000 "
        "(StyleSheetSelector.h:15), not to itself");
  check(kSpecificityWeightClassOrPseudo == 100'000,
        "kSpecificityWeightClassOrPseudo pinned to the LITERAL upstream value 100'000 "
        "(StyleSheetSelector.h:16-18, Class==Attribute==PseudoClass), not to itself");
  check(kSpecificityWeightId == 1'000'000,
        "kSpecificityWeightId pinned to the LITERAL upstream value 1'000'000 "
        "(StyleSheetSelector.h:19), not to itself -- this is the EXACT constant the QA's own "
        "mutation (1'000'000 -> 100'000) sabotaged; this line is the one that must go red for it");

  // EN: ORDER, the fact the whole cascade depends on: ID outranks class/pseudo outranks tag.
  //     Compared as LITERAL inequalities, not derived from one constant times another, so a
  //     mutation that made two weights EQUAL (not just wrong) still gets caught here even if it
  //     somehow preserved every individual `== <literal>` check above (it would not, in this
  //     module's own three constants, but the order check is a second, independent line of
  //     defence, not a restatement of the same fact).
  // PT: ORDEM, o fato de que a cascata inteira depende: ID vence classe/pseudo vence tag.
  //     Comparado como desigualdade LITERAL, não derivado de uma constante vezes outra, então uma
  //     mutação que igualasse dois pesos (não só errasse o valor) ainda seria pega aqui mesmo que
  //     de algum jeito preservasse toda checagem individual `== <literal>` acima (não preservaria,
  //     nas três constantes próprias deste módulo, mas a checagem de ordem é uma segunda linha de
  //     defesa independente, não uma repetição do mesmo fato).
  check(kSpecificityWeightId > kSpecificityWeightClassOrPseudo,
        "ORDER: weight(ID) > weight(class/pseudo) -- an id ALWAYS outranks any number of classes "
        "up to the margin checked below");
  check(kSpecificityWeightClassOrPseudo > kSpecificityWeightTag,
        "ORDER: weight(class/pseudo) > weight(tag)");

  // EN: MARGIN, exact: the upstream ratio is precisely 10x at each step (1'000'000 / 100'000 ==
  //     100'000 / 10'000 == 10) -- this is what STOPS a handful of classes from silently
  //     outranking an id, or a handful of tags from outranking a class, on a real, multi-compound
  //     selector chain. Checked as an exact `==`, not `>=`/a tolerance, because the upstream
  //     algorithm's own ratio IS exact, not a "generous enough" approximation.
  // PT: MARGEM, exata: a razão do upstream é precisamente 10x em cada degrau (1'000'000 / 100'000
  //     == 100'000 / 10'000 == 10) -- isto é o que IMPEDE um punhado de classes de vencer em
  //     silêncio um id, ou um punhado de tags de vencer uma classe, numa cadeia de seletor real,
  //     multi-compound. Checado como `==` exato, não `>=`/uma tolerância, porque a própria razão
  //     do algoritmo do upstream É exata, não uma aproximação "generosa o bastante".
  check(kSpecificityWeightId == 10 * kSpecificityWeightClassOrPseudo,
        "MARGIN: weight(ID) is EXACTLY 10x weight(class/pseudo) -- 1'000'000 / 100'000 == 10, "
        "matching StyleSheetSelector.h's own ID/Class ratio");
  check(kSpecificityWeightClassOrPseudo == 10 * kSpecificityWeightTag,
        "MARGIN: weight(class/pseudo) is EXACTLY 10x weight(tag) -- 100'000 / 10'000 == 10, "
        "matching StyleSheetSelector.h's own Class/Tag ratio");

  // EN: The margin's own CONCRETE CONSEQUENCE at the compound level, not just an abstract ratio:
  //     a compound with 9 classes is STILL outranked by a compound with a single id (one class
  //     short of the tie, mirroring this file's own "near-miss" boundary discipline used
  //     elsewhere), and a compound with EXACTLY 10 classes TIES a compound with a single id --
  //     this is a real, unavoidable consequence of the upstream's own exact-10x ratio, not a bug
  //     this module invented; stated explicitly here so nobody mistakes the tie for a defect
  //     later. `docs/rmlx-subset.md`'s own corpus measured at most 2 classes in a real compound
  //     (`compound, no combinator`, section 6.2), so this 9/10-class boundary is deliberately far
  //     past anything the real corpus exercises -- it exists to pin the ALGORITHM's own arithmetic
  //     fact, not to model a realistic selector.
  // PT: A própria CONSEQUÊNCIA CONCRETA da margem no nível de compound, não só uma razão
  //     abstrata: um compound com 9 classes AINDA perde pra um compound com um único id (uma
  //     classe a menos que o empate, espelhando a mesma disciplina "quase-empate" que este
  //     arquivo já usa em outro lugar), e um compound com EXATAMENTE 10 classes EMPATA com um
  //     compound com um único id -- isto é uma consequência real, inevitável, da própria razão
  //     exata-de-10x do upstream, não um bug que este módulo inventou; declarado explicitamente
  //     aqui pra ninguém confundir o empate com um defeito depois. O próprio corpus do
  //     `docs/rmlx-subset.md` mediu no máximo 2 classes num compound real (`composto, sem
  //     combinador`, seção 6.2), então esta fronteira de 9/10 classes é deliberadamente muito além
  //     do que o corpus real exercita -- existe pra ancorar o próprio fato aritmético do
  //     ALGORITMO, não pra modelar um seletor realista.
  CompoundSelector nine_classes;
  for (int i = 0; i < 9; ++i) {
    nine_classes.classes.push_back("c");
  }
  CompoundSelector ten_classes;
  for (int i = 0; i < 10; ++i) {
    ten_classes.classes.push_back("c");
  }
  CompoundSelector one_id = id_only("panel");

  check(compound_specificity(nine_classes) < compound_specificity(one_id),
        "MARGIN consequence: 9 classes (900'000) is STILL less than 1 id (1'000'000) -- one class "
        "short of the exact tie point");
  check(compound_specificity(ten_classes) == compound_specificity(one_id),
        "MARGIN consequence: 10 classes (1'000'000) EXACTLY TIES 1 id (1'000'000) -- the real, "
        "documented consequence of the upstream's own exact-10x ratio, not a defect");
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
// EN: Form 9/9: universal `*` (`ESC-8`) -- positive on a root, a deeply nested leaf, and an
//     element of a completely different tag every time, with specificity EXACTLY zero on every
//     one of them (never merely "low", never a partial/rounded weight).
// PT: Forma 9/9: universal `*` (`ESC-8`) -- positivo numa raiz, numa folha profundamente aninhada,
//     e num elemento de tag completamente diferente todas as vezes, com especificidade EXATAMENTE
//     zero em cada uma delas (nunca meramente "baixa", nunca um peso parcial/arredondado).
// ---------------------------------------------------------------------------
void test_form_universal() {
  Selector s = chain1(universal_only());

  Element root("body");
  MatchResult root_result = match_selector(s, root, MatchState{});
  check(root_result.matched, "form *: POSITIVE -- matches the root element");
  check(root_result.specificity == 0, "form *: POSITIVE -- root match has EXACT zero specificity");

  Element* mid = add_child(root, "section");
  Element* leaf = add_child(*mid, "span");
  MatchResult leaf_result = match_selector(s, *leaf, MatchState{});
  check(leaf_result.matched, "form *: POSITIVE -- matches a deeply nested leaf");
  check(leaf_result.specificity == 0, "form *: POSITIVE -- leaf match has EXACT zero specificity");

  Element other_tag("input");
  MatchResult other_result = match_selector(s, other_tag, MatchState{});
  check(other_result.matched, "form *: POSITIVE -- matches an element of a completely different tag");
  check(other_result.specificity == 0,
        "form *: POSITIVE -- different-tag match has EXACT zero specificity");
}

// ---------------------------------------------------------------------------
// EN: `*.foo` -- the class alone decides the match (universal waives nothing), and the
//     specificity is EXACTLY the class weight -- universal contributed NOTHING to the sum.
// PT: `*.foo` -- a classe sozinha decide o casamento (universal não dispensa nada), e a
//     especificidade é EXATAMENTE o peso da classe -- universal não somou NADA.
// ---------------------------------------------------------------------------
void test_form_universal_class_specificity() {
  CompoundSelector c = universal_only();
  c.classes.push_back("foo");
  Selector s = chain1(c);

  Element with_class("div");
  with_class.add_class("foo");
  MatchResult positive = match_selector(s, with_class, MatchState{});
  check(positive.matched, "form *.foo: POSITIVE -- element carrying the class matches");
  check(positive.specificity == kSpecificityWeightClassOrPseudo,
        "form *.foo: POSITIVE -- specificity is EXACTLY one class weight, universal added nothing");

  Element without_class("div");
  MatchResult negative = match_selector(s, without_class, MatchState{});
  check(!negative.matched,
        "form *.foo: NEGATIVE -- element without the class does not match despite universal");
  check(negative.specificity == 0, "form *.foo: NEGATIVE -- specificity is 0 on a non-match");
}

// ---------------------------------------------------------------------------
// EN: `*div` -- the pin's own fall-through accident (`StyleSheetParser.cpp:1105-1106`), matching
//     `parser_selector_sanity.cpp`'s own `test_form_universal_tag_accident` at the parse layer.
//     Specificity comes ONLY from the tag weight -- universal contributed NOTHING, EXACTLY the tag
//     weight, not "the tag weight plus a little".
// PT: `*div` -- o próprio acidente de fall-through do pin (`StyleSheetParser.cpp:1105-1106`),
//     casando com o próprio `test_form_universal_tag_accident` do parser_selector_sanity.cpp na
//     camada de parse. A especificidade vem SÓ do peso da tag -- universal não somou NADA,
//     EXATAMENTE o peso da tag, não "o peso da tag mais um pouco".
// ---------------------------------------------------------------------------
void test_form_universal_tag_accident_specificity() {
  CompoundSelector c = universal_only();
  c.tag = "div";
  Selector s = chain1(c);

  Element div_el("div");
  MatchResult positive = match_selector(s, div_el, MatchState{});
  check(positive.matched, "form *div: POSITIVE -- a 'div' element matches");
  check(positive.specificity == kSpecificityWeightTag,
        "form *div: POSITIVE -- specificity is EXACTLY the tag weight, universal added nothing");

  Element span_el("span");
  check(!match_selector(s, span_el, MatchState{}).matched,
        "form *div: NEGATIVE -- a 'span' element does not match (tag constraint still enforced)");
}

// ---------------------------------------------------------------------------
// EN: Universal combined with BOTH combinators this subset authorizes -- `div > *` (Child: matches
//     a DIRECT child of a div, never the div itself), `* > div` (Child: any parent at all, rejects
//     a root div with no parent), `div *` (Descendant: matches at ANY depth below a div).
// PT: Universal combinado com OS DOIS combinadores que este subconjunto autoriza -- `div > *`
//     (Filho: casa um filho DIRETO de um div, nunca o div em si), `* > div` (Filho: qualquer pai,
//     rejeita um div-raiz sem pai), `div *` (Descendente: casa em QUALQUER profundidade abaixo de
//     um div).
// ---------------------------------------------------------------------------
void test_form_universal_combinators() {
  {
    Element parent("div");
    Element* direct_child = add_child(parent, "span");
    Selector s = chain2(tag_only("div"), Combinator::Child, universal_only());
    check(match_selector(s, *direct_child, MatchState{}).matched,
          "form div > *: POSITIVE -- direct child of a div matches");
    check(!match_selector(s, parent, MatchState{}).matched,
          "form div > *: NEGATIVE -- the div itself does not match its own child-combinator "
          "selector");
  }
  {
    Selector s = chain2(universal_only(), Combinator::Child, tag_only("div"));
    Element root_div("div");
    check(!match_selector(s, root_div, MatchState{}).matched,
          "form * > div: BOUNDARY -- a root div (no parent) cannot satisfy '* > div', needs a "
          "parent");
    Element wrapper("section");
    Element* child_div = add_child(wrapper, "div");
    check(match_selector(s, *child_div, MatchState{}).matched,
          "form * > div: POSITIVE -- a div with ANY parent matches");
  }
  {
    Selector s = chain2(tag_only("div"), Combinator::Descendant, universal_only());
    Element root_div("div");
    Element* mid = add_child(root_div, "section");
    Element* deep = add_child(*mid, "span");
    check(match_selector(s, *mid, MatchState{}).matched,
          "form div *: POSITIVE -- immediate child of div matches");
    check(match_selector(s, *deep, MatchState{}).matched,
          "form div *: POSITIVE -- grandchild of div ALSO matches (any depth)");
    check(!match_selector(s, root_div, MatchState{}).matched,
          "form div *: NEGATIVE -- the div itself is not its own descendant");
  }
}

// ---------------------------------------------------------------------------
// EN: The NEW invariant this item introduces, replacing the one selector_match.hpp's own
//     `MatchResult` doc-comment used to state ("matched == true always carries specificity >=
//     kSpecificityWeightTag") -- see that doc-comment's own rewrite for the full account.
//     matched == true && specificity == 0 is now a LEGAL, REACHABLE state: a universal-only
//     compound ('*' alone) matches unconditionally with zero weight. This is the twin of the OLD
//     invariant that dies with this item, stated explicitly so a future reader never has to
//     re-derive it from the arithmetic alone.
// PT: O invariante NOVO que este item introduz, substituindo o que o próprio doc-comment de
//     `MatchResult` do selector_match.hpp costumava declarar ("matched == true sempre carrega
//     specificity >= kSpecificityWeightTag") -- ver a própria reescrita daquele doc-comment pro
//     relato completo. matched == true && specificity == 0 agora é um estado LEGAL, ALCANÇÁVEL: um
//     compound só-universal ('*' sozinho) casa incondicionalmente com peso zero. Este é o gêmeo do
//     invariante ANTIGO que morre com este item, declarado explicitamente pra um leitor futuro
//     nunca precisar re-derivá-lo só da aritmética.
// ---------------------------------------------------------------------------
void test_form_universal_specificity_is_exact_zero() {
  Selector s = chain1(universal_only());
  Element el("div");
  MatchResult r = match_selector(s, el, MatchState{});
  check(r.matched && r.specificity == 0,
        "NEW invariant: matched == true && specificity == 0 is legal (universal-only compound) -- "
        "the OLD invariant (matched implies specificity >= 10'000) is retired by this item");
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

// ---------------------------------------------------------------------------
// EN: `ESC-8` -- comma-list with a universal entry alongside a class entry, on an element carrying
//     that class: BOTH entries match (universal unconditionally, the class entry because the
//     element carries it), and the list returns the MAXIMUM of the two -- the class entry's
//     100'000, never universal's 0, and never their SUM (which a bug summing instead of
//     max-ing across entries would wrongly produce, 100'000 either way here since 0+100'000 ==
//     max(0,100'000) by coincidence -- see the `nine_classes`/`ten_classes` case in
//     `test_specificity_pinned_against_upstream_literals_and_order` above for a pair where sum and
//     max would visibly diverge if this module ever combined two DIFFERENT non-zero entries; this
//     test's own job is narrower and load-bearing in a different way: proving universal's own zero
//     never accidentally WINS a max() against a real discriminator, which a bug computing MINIMUM
//     instead of maximum would get wrong).
// PT: `ESC-8` -- lista-vírgula com uma entrada universal ao lado de uma entrada de classe, num
//     elemento carregando aquela classe: AS DUAS entradas casam (universal incondicionalmente, a
//     entrada de classe porque o elemento a carrega), e a lista retorna o MÁXIMO das duas -- o
//     100'000 da entrada de classe, nunca o 0 do universal, e nunca a SOMA dos dois (que um bug
//     somando em vez de tirar-o-máximo entre entradas produziria errado, 100'000 dos dois jeitos
//     aqui já que 0+100'000 == max(0,100'000) por coincidência -- ver o caso
//     `nine_classes`/`ten_classes` do `test_specificity_pinned_against_upstream_literals_and_order`
//     acima pra um par onde soma e máximo divergiriam visivelmente se este módulo algum dia
//     combinasse duas entradas DIFERENTES não-zero; o próprio trabalho deste teste é mais estreito e
//     porta um peso diferente: provar que o próprio zero do universal nunca vence um max() contra um
//     discriminador real por acidente, o que um bug computando MÍNIMO em vez de máximo erraria).
// ---------------------------------------------------------------------------
void test_comma_list_universal_max_specificity() {
  Element el("div");
  el.add_class("foo");

  SelectorList list = {chain1(universal_only()), chain1(class_only("foo"))};
  MatchResult r = match_selector_list(list, el, MatchState{});
  check(r.matched, "comma-list * , .foo: both entries match");
  check(r.specificity == kSpecificityWeightClassOrPseudo,
        "comma-list * , .foo: returns the MAXIMUM specificity (100'000, the .foo entry), not "
        "the universal entry's 0");
}

} // namespace

int main() {
  test_compound_specificity_weights();
  test_selector_specificity_sums_chain();
  test_specificity_pinned_against_upstream_literals_and_order();
  test_form_class();
  test_form_id();
  test_form_descendant();
  test_form_tag();
  test_form_hover();
  test_form_compound_no_combinator();
  test_form_child();
  test_form_universal();
  test_form_universal_class_specificity();
  test_form_universal_tag_accident_specificity();
  test_form_universal_combinators();
  test_form_universal_specificity_is_exact_zero();
  test_boundary_near_miss_class_count();
  test_boundary_root_element_has_no_ancestors();
  test_boundary_empty_selector_defensive();
  test_comma_list_forms();
  test_comma_list_universal_max_specificity();

  std::printf(
      "SCOPE: 9 formas cobertas de 9 enumeradas, 0 casos de corpus (ver "
      "selector_match_corpus_sanity), 0 travamentos, 0 formas fora do subset\n");

  if (g_failures > 0) {
    std::fprintf(stderr, "selector_match_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("selector_match_sanity: PASS");
  return 0;
}
