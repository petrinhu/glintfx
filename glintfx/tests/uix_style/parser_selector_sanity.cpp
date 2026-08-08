// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SHEET-PARSER -- selector-structuring coverage. One test per authorized selector FORM
//     (docs/rmlx-subset.md section 6.2's own closed, corpus-counted table: `.class`, `#id`,
//     descendant, tag, compound-no-combinator, pseudo-class `:hover`, comma-list, child `>`,
//     universal `*` -- the last one delivered by `ESC-8`, docs/rmlx-subset.md section 6.2's own
//     row, authorized by §7 2026-08-07's parity rule) and one test per FORM that table STILL
//     fail-highs (attribute `[x]`, sibling `+`/`~`, `:not(...)`/`nth-child(...)`) -- exactly this
//     item's own Definition of Done: "cada forma de seletor da lista acima com teste próprio, e
//     cada forma fora do escopo com teste provando o fail-high". Zero RmlUi/GLFW/GL, same
//     standalone discipline as every other executable in this directory.
// PT: UIX-SHEET-PARSER -- cobertura de estruturação de seletor. Um teste por FORMA de seletor
//     autorizada (a própria tabela fechada, contada-por-corpus, da seção 6.2 do
//     docs/rmlx-subset.md: `.classe`, `#id`, descendente, tag, composto-sem-combinador,
//     pseudo-classe `:hover`, lista-vírgula, filho `>`, universal `*` -- a última entregue pela
//     `ESC-8`, própria linha da seção 6.2 do docs/rmlx-subset.md, autorizada pela regra de
//     paridade da §7 2026-08-07) e um teste por FORMA que aquela tabela AINDA fail-higha (atributo
//     `[x]`, irmão `+`/`~`, `:not(...)`/`nth-child(...)`) -- exatamente o próprio Definition of
//     Done deste item: "cada forma de seletor da lista acima com teste próprio, e cada forma fora
//     do escopo com teste provando o fail-high". Zero RmlUi/GLFW/GL, mesma disciplina standalone
//     de todo outro executável deste diretório.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/parser.hpp"

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

using glintfx::uix::style::Combinator;
using glintfx::uix::style::parse_stylesheet;
using glintfx::uix::style::Rule;
using glintfx::uix::style::SheetParseResult;

// EN: Parses `rcss` and asserts exactly ONE `Rule` registered (the common shape every case below
//     needs) with zero fatal error -- returns that `Rule` for the caller's own selector-shape
//     assertions. ⚠️ `rcss` is `std::string_view`, NOT `const std::string&`, DELIBERATELY: every
//     call site below passes a string LITERAL, and `std::string_view` binds to it directly (static
//     storage duration, safe for this function's whole caller's own lifetime); `const std::string&`
//     would implicitly materialize a TEMPORARY `std::string` for that literal, whose lifetime ends
//     at the end of the CALL statement -- before the caller's own subsequent `check()` calls read
//     `Rule`'s own `string_view` fields, which alias that buffer per parser.hpp's own "zero-copy"
//     contract. Caught by this exact bug during this item's own manual verification (12 spurious
//     assertion failures, all downstream of a dangling `string_view`) before this file was fixed.
// PT: Parseia `rcss` e afirma exatamente UMA `Rule` registrada (a forma comum que todo caso abaixo
//     precisa) sem erro fatal -- retorna aquela `Rule` pras próprias afirmações de forma-de-seletor
//     do chamador. ⚠️ `rcss` é `std::string_view`, NÃO `const std::string&`, DELIBERADAMENTE: toda
//     chamada abaixo passa um literal de string, e `std::string_view` vincula direto a ele (duração
//     de armazenamento estática, seguro pela vida inteira do PRÓPRIO chamador desta função);
//     `const std::string&` materializaria implicitamente uma TEMPORÁRIA `std::string` pra aquele
//     literal, cuja vida termina no fim da instrução de CHAMADA -- antes das próprias chamadas
//     `check()` subsequentes do chamador lerem os campos `string_view` da `Rule`, que aliasam aquele
//     buffer per o próprio contrato "zero-cópia" do parser.hpp. Pego por este exato bug durante a
//     própria verificação manual deste item (12 falhas de asserção espúrias, todas rio-abaixo de um
//     `string_view` pendurado) antes deste arquivo ser corrigido.
const Rule* parse_single_rule(std::string_view rcss, SheetParseResult* result, const char* label) {
  *result = parse_stylesheet(rcss);
  check(!result->error.has_value(), (std::string(label) + ": no fatal error").c_str());
  if (!result->sheet) {
    return nullptr;
  }
  if (result->sheet->rules.size() != 1) {
    std::fprintf(stderr, "FAIL: %s: expected exactly 1 rule, got %zu\n", label,
                 result->sheet->rules.size());
    ++g_failures;
    return nullptr;
  }
  return &result->sheet->rules[0];
}

// ---------------------------------------------------------------------------
// EN: Form 1 -- `.class` (590 measured, docs/rmlx-subset.md section 6.2).
// PT: Forma 1 -- `.classe` (590 medidos, seção 6.2 do docs/rmlx-subset.md).
// ---------------------------------------------------------------------------
void test_form_class() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule(".section-dark { color: white; }", &result, "form_class");
  if (!rule) return;
  check(rule->selectors.size() == 1, "form_class: one selector in the list");
  check(rule->selectors[0].compounds.size() == 1, "form_class: one compound");
  const auto& c = rule->selectors[0].compounds[0];
  check(c.tag.empty(), "form_class: no tag");
  check(c.id.empty(), "form_class: no id");
  check(c.classes.size() == 1 && c.classes[0] == "section-dark", "form_class: classes == {'section-dark'}");
  check(!c.pseudo_hover, "form_class: no :hover");
}

// ---------------------------------------------------------------------------
// EN: Form 2 -- `#id` (183 measured).
// PT: Forma 2 -- `#id` (183 medidos).
// ---------------------------------------------------------------------------
void test_form_id() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("#btn_a { display: block; }", &result, "form_id");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.tag.empty(), "form_id: no tag");
  check(c.id == "btn_a", "form_id: id == 'btn_a'");
  check(c.classes.empty(), "form_id: no classes");
}

// ---------------------------------------------------------------------------
// EN: Form 3 -- descendant, space combinator (131 measured).
// PT: Forma 3 -- descendente, combinador de espaço (131 medidos).
// ---------------------------------------------------------------------------
void test_form_descendant() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("#panel span { color: red; }", &result, "form_descendant");
  if (!rule) return;
  const auto& sel = rule->selectors[0];
  check(sel.compounds.size() == 2, "form_descendant: two compounds");
  check(sel.combinators.size() == 1 && sel.combinators[0] == Combinator::Descendant,
        "form_descendant: one Descendant combinator");
  check(sel.compounds[0].id == "panel", "form_descendant: first compound is #panel");
  check(sel.compounds[1].tag == "span", "form_descendant: second compound is tag 'span'");
}

// ---------------------------------------------------------------------------
// EN: Form 4 -- bare tag (110 measured).
// PT: Forma 4 -- tag pura (110 medidos).
// ---------------------------------------------------------------------------
void test_form_tag() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("body { display: block; }", &result, "form_tag");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.tag == "body", "form_tag: tag == 'body'");
  check(c.id.empty() && c.classes.empty() && !c.pseudo_hover, "form_tag: no id/class/hover");
}

// ---------------------------------------------------------------------------
// EN: Form 5 -- pseudo-class composite, `:hover` only (37 measured).
// PT: Forma 5 -- pseudo-classe composta, só `:hover` (37 medidos).
// ---------------------------------------------------------------------------
void test_form_pseudo_hover() {
  SheetParseResult result;
  const Rule* rule =
      parse_single_rule(".difficulty-item:hover { color: yellow; }", &result, "form_pseudo_hover");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.classes.size() == 1 && c.classes[0] == "difficulty-item",
        "form_pseudo_hover: class == 'difficulty-item'");
  check(c.pseudo_hover, "form_pseudo_hover: pseudo_hover == true");
}

// ---------------------------------------------------------------------------
// EN: Form 6 -- comma-list, multiple selectors sharing one declaration block (15 measured; the
//     16-tag UA-stylesheet rule is the load-bearing real-world instance, docs/rmlx-subset.md
//     section 6.1).
// PT: Forma 6 -- lista-vírgula, múltiplos seletores compartilhando um bloco de declaração (15
//     medidos; a regra de 16 tags da UA-stylesheet é a instância real, portante, da seção 6.1 do
//     docs/rmlx-subset.md).
// ---------------------------------------------------------------------------
void test_form_comma_list() {
  SheetParseResult result;
  const Rule* rule =
      parse_single_rule("div, p, h1, h2 { display: block; }", &result, "form_comma_list");
  if (!rule) return;
  check(rule->selectors.size() == 4, "form_comma_list: 4 selectors in the list");
  check(rule->selectors[0].compounds[0].tag == "div", "form_comma_list: selector 0 tag == 'div'");
  check(rule->selectors[1].compounds[0].tag == "p", "form_comma_list: selector 1 tag == 'p'");
  check(rule->selectors[2].compounds[0].tag == "h1", "form_comma_list: selector 2 tag == 'h1'");
  check(rule->selectors[3].compounds[0].tag == "h2", "form_comma_list: selector 3 tag == 'h2'");
  // EN: Same declaration block applies to every selector in the list -- there is exactly ONE
  //     `Rule`, not four.
  // PT: O mesmo bloco de declaração se aplica a todo seletor da lista -- existe exatamente UMA
  //     `Rule`, não quatro.
  check(rule->declarations.size() == 1, "form_comma_list: one shared declaration block");
}

// ---------------------------------------------------------------------------
// EN: 🔴 UIX-RCSS-ERRATA-2 (orchestrator relay, in-flight errata against 7 byte-exact divergences
//     from real RmlUi found by an independent audit of docs/uix-rcss.md's 68 normative rules): an
//     INVALID selector INSIDE a comma-list drops ONLY THAT ONE ENTRY, never the whole rule --
//     docs/uix-rcss.md section 11's own "the whole rule (not just one selector in a comma-list)
//     fails to register" phrasing does NOT match real RmlUi's own per-entry recovery (the errata's
//     own source citation; `docs/rmlx-subset.md`'s own "where the spec and RmlUi disagree, RmlUi
//     wins" is the standing rule this test enforces). This matters concretely, not academically:
//     the 16-tag rule this repo's OWN `ua_stylesheet.hpp` opens with (docs/rmlx-subset.md section
//     6.1) sets `display: block` on every structural element of every document glintfx renders --
//     ONE bad tag name anywhere in a future edit of that list must not silently disable `display:
//     block` for the OTHER 15.
// PT: 🔴 UIX-RCSS-ERRATA-2 (relay do orquestrador, errata em voo contra 7 divergências
//     byte-exatas do RmlUi real achadas por uma auditoria independente das 68 regras normativas do
//     docs/uix-rcss.md): um seletor INVÁLIDO DENTRO de uma lista-vírgula derruba SÓ AQUELA UMA
//     ENTRADA, nunca a regra inteira -- a própria frase "a regra inteira (não só um seletor numa
//     lista-vírgula) reprova de registrar" da seção 11 do docs/uix-rcss.md NÃO bate com a própria
//     recuperação por-entrada do RmlUi real (a própria citação-fonte da errata; a própria "onde a
//     spec e o RmlUi discordam, quem manda é o RmlUi" do docs/rmlx-subset.md é a regra em vigor que
//     este teste reforça). Isto importa concretamente, não academicamente: a regra de 16 tags com
//     que o PRÓPRIO `ua_stylesheet.hpp` deste repo abre (seção 6.1 do docs/rmlx-subset.md) dá
//     `display: block` a todo elemento estrutural de todo documento que a glintfx renderiza -- UM
//     nome de tag ruim em qualquer edição futura daquela lista não pode desligar `display: block`
//     em silêncio pras OUTRAS 15.
// ---------------------------------------------------------------------------
void test_comma_list_invalid_entry_drops_only_itself() {
  // EN: `[bad]` (an attribute selector), NOT `*` -- since `ESC-8`, `*` is a valid, authorized
  //     compound (docs/rmlx-subset.md section 6.2), so it can no longer stand in as "the invalid
  //     entry" this test needs; `[bad]` (attribute selectors, still zero-measured/unsupported,
  //     `ESC-9`'s own territory) preserves this test's original intent unchanged.
  // PT: `[bad]` (um seletor de atributo), NÃO `*` -- desde a `ESC-8`, `*` é um compound válido,
  //     autorizado (seção 6.2 do docs/rmlx-subset.md), então ele não consegue mais servir de "a
  //     entrada inválida" que este teste precisa; `[bad]` (seletores de atributo, ainda
  //     zero-medidos/não-suportados, território da própria `ESC-9`) preserva a intenção original
  //     deste teste sem mudança.
  auto result = parse_stylesheet(".a, [bad], .b { color: red; }");
  check(!result.error.has_value(), "comma_list_per_entry: no fatal error");
  check(result.sheet != nullptr, "comma_list_per_entry: sheet is non-null");
  if (!result.sheet) return;
  check(result.sheet->rules.size() == 1,
        "comma_list_per_entry: the rule STILL registers (not dropped whole)");
  if (result.sheet->rules.empty()) return;
  const Rule& rule = result.sheet->rules[0];
  check(rule.selectors.size() == 2,
        "comma_list_per_entry: only '[bad]' was dropped -- '.a' and '.b' both survive");
  if (rule.selectors.size() == 2) {
    check(rule.selectors[0].compounds[0].classes[0] == "a", "comma_list_per_entry: entry 0 == '.a'");
    check(rule.selectors[1].compounds[0].classes[0] == "b", "comma_list_per_entry: entry 1 == '.b'");
  }
  check(rule.declarations.size() == 1,
        "comma_list_per_entry: the declaration block still applies to the surviving entries");
  check(!result.diagnostics.empty(),
        "comma_list_per_entry: the dropped '[bad]' entry is still logged, not silently swallowed");
}

// ---------------------------------------------------------------------------
// EN: The real, load-bearing shape (docs/rmlx-subset.md section 6.1): the UA-stylesheet's own
//     16-tag rule, with one tag deliberately mistyped, still applies `display: block` to the OTHER
//     15 -- the concrete consequence UIX-RCSS-ERRATA-2 exists to prevent regressing.
// PT: A forma real, portante (seção 6.1 do docs/rmlx-subset.md): a própria regra de 16 tags da
//     UA-stylesheet, com uma tag deliberadamente digitada errado, ainda aplica `display: block` às
//     OUTRAS 15 -- a consequência concreta que a UIX-RCSS-ERRATA-2 existe pra evitar regredir.
// ---------------------------------------------------------------------------
void test_comma_list_ua_stylesheet_shape_survives_one_bad_tag() {
  auto result = parse_stylesheet(
      "div, p, h1, h2, h3, h4, h5, h6, ul, ol, [bad], section, article, header, footer, nav, main "
      "{ display: block; }");
  check(result.sheet && result.sheet->rules.size() == 1,
        "comma_list_ua_shape: the rule registers despite one bad entry");
  if (!result.sheet || result.sheet->rules.empty()) return;
  check(result.sheet->rules[0].selectors.size() == 16,
        "comma_list_ua_shape: 16 of the 17 written entries survive (the '[bad]' one dropped)");
}

// ---------------------------------------------------------------------------
// EN: Form 7 -- compound, no combinator (`tag.class`, 5 measured).
// PT: Forma 7 -- composto, sem combinador (`tag.classe`, 5 medidos).
// ---------------------------------------------------------------------------
void test_form_compound_no_combinator() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("div.row { display: block; }", &result, "form_compound");
  if (!rule) return;
  check(rule->selectors[0].compounds.size() == 1, "form_compound: one compound (no combinator)");
  const auto& c = rule->selectors[0].compounds[0];
  check(c.tag == "div", "form_compound: tag == 'div'");
  check(c.classes.size() == 1 && c.classes[0] == "row", "form_compound: class == 'row'");
}

// ---------------------------------------------------------------------------
// EN: Form 8 -- child combinator, `>` (2 measured).
// PT: Forma 8 -- combinador filho, `>` (2 medidos).
// ---------------------------------------------------------------------------
void test_form_child_combinator() {
  SheetParseResult result;
  const Rule* rule =
      parse_single_rule("#scroller > div { display: block; }", &result, "form_child_combinator");
  if (!rule) return;
  const auto& sel = rule->selectors[0];
  check(sel.compounds.size() == 2, "form_child_combinator: two compounds");
  check(sel.combinators.size() == 1 && sel.combinators[0] == Combinator::Child,
        "form_child_combinator: one Child combinator");
}

// ---------------------------------------------------------------------------
// EN: Form 9 -- universal `*` (0 measured, docs/rmlx-subset.md section 6.2's own table --
//     authorized by §7 2026-08-07's parity rule, delivered by `ESC-8`). `*` alone is CONSUMED by
//     `parse_compound` and sets `CompoundSelector::universal`; it never becomes a token or field of
//     its own kind -- see parser.hpp's own `CompoundSelector` doc-comment. `*` is no longer a
//     FAIL-HIGH form (contrast the 3 that remain below, renumbered from the 4 this file used to
//     name).
// PT: Forma 9 -- universal `*` (0 medidos, própria tabela da seção 6.2 do docs/rmlx-subset.md --
//     autorizada pela regra de paridade da §7 2026-08-07, entregue pela `ESC-8`). `*` sozinho é
//     CONSUMIDO pelo `parse_compound` e seta `CompoundSelector::universal`; nunca vira token ou
//     campo de espécie própria -- ver o próprio comentário de doc de `CompoundSelector` do
//     parser.hpp. `*` deixou de ser forma FAIL-HIGH (contraste as 3 que restam abaixo, renumeradas
//     das 4 que este arquivo nomeava).
// ---------------------------------------------------------------------------
void test_form_universal_alone() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("* { color: red; }", &result, "form_universal_alone");
  if (!rule) return;
  check(rule->selectors.size() == 1, "form_universal_alone: one selector in the list");
  check(rule->selectors[0].compounds.size() == 1, "form_universal_alone: one compound");
  const auto& c = rule->selectors[0].compounds[0];
  check(c.universal, "form_universal_alone: universal == true");
  check(c.tag.empty(), "form_universal_alone: no tag");
  check(c.id.empty(), "form_universal_alone: no id");
  check(c.classes.empty(), "form_universal_alone: no classes");
  check(!c.pseudo_hover, "form_universal_alone: no :hover");
}

// ---------------------------------------------------------------------------
// EN: `*` glued to a discriminator -- `universal` is set AND the discriminator's own field is set,
//     never one instead of the other.
// PT: `*` colado a um discriminador -- `universal` é setado E o próprio campo do discriminador é
//     setado, nunca um em vez do outro.
// ---------------------------------------------------------------------------
void test_form_universal_with_class() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("*.foo { color: red; }", &result, "form_universal_class");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.universal, "form_universal_class: universal == true");
  check(c.classes.size() == 1 && c.classes[0] == "foo",
        "form_universal_class: classes == {'foo'}");
  check(c.tag.empty() && c.id.empty() && !c.pseudo_hover,
        "form_universal_class: no tag/id/hover");
}

void test_form_universal_with_id() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("*#bar { color: red; }", &result, "form_universal_id");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.universal, "form_universal_id: universal == true");
  check(c.id == "bar", "form_universal_id: id == 'bar'");
  check(c.tag.empty() && c.classes.empty() && !c.pseudo_hover,
        "form_universal_id: no tag/classes/hover");
}

void test_form_universal_with_hover() {
  SheetParseResult result;
  const Rule* rule = parse_single_rule("*:hover { color: red; }", &result, "form_universal_hover");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.universal, "form_universal_hover: universal == true");
  check(c.pseudo_hover, "form_universal_hover: pseudo_hover == true");
  check(c.tag.empty() && c.id.empty() && c.classes.empty(),
        "form_universal_hover: no tag/id/classes");
}

// ---------------------------------------------------------------------------
// EN: The pin's own accident, reproduced DELIBERATELY, not "fixed" (`StyleSheetParser.cpp:
//     1105-1106`): `if (rule[start_index] == '*') start_index += 1;` skips the `*` and falls
//     straight into the SAME tag-read scan a bare `div` would take -- `*div` structurally parses
//     to the exact compound a real author-facing `div` alone would, PLUS `universal == true` on
//     top (a field the pin has no representation for at all, but that changes nothing observable:
//     selector_match.cpp never reads it, see that file's own `compound_matches` doc-comment).
//     `*div == div` is the pin's own real, measured behavior, not a bug this parser introduces.
// PT: O próprio acidente do pin, reproduzido DE PROPÓSITO, não "consertado"
//     (`StyleSheetParser.cpp:1105-1106`): `if (rule[start_index] == '*') start_index += 1;` pula o
//     `*` e cai direto no MESMO scan de leitura-de-tag que um `div` cru tomaria -- `*div` parseia
//     estruturalmente pro exato compound que um `div` cru, de-frente-pro-autor, produziria, MAIS
//     `universal == true` por cima (um campo que o pin não tem representação nenhuma, mas que não
//     muda nada observável: o selector_match.cpp nunca o lê, ver o próprio comentário de doc de
//     `compound_matches` daquele arquivo). `*div == div` é o próprio comportamento real, medido, do
//     pin, não um bug que este parser introduz.
// ---------------------------------------------------------------------------
void test_form_universal_tag_accident() {
  SheetParseResult result;
  const Rule* rule =
      parse_single_rule("*div { color: red; }", &result, "form_universal_tag_accident");
  if (!rule) return;
  const auto& c = rule->selectors[0].compounds[0];
  check(c.universal, "form_universal_tag_accident: universal == true (the '*' was consumed)");
  check(c.tag == "div",
        "form_universal_tag_accident: tag == 'div' (the pin's own fall-through, "
        "StyleSheetParser.cpp:1105-1106)");
  check(c.id.empty() && c.classes.empty() && !c.pseudo_hover,
        "form_universal_tag_accident: no id/classes/hover");
}

// ---------------------------------------------------------------------------
// EN: Universal as the SECOND compound of a Descendant chain -- `tag *` and `.class *`, proving
//     the field survives combinator-joining, not just a lone compound.
// PT: Universal como o SEGUNDO compound de uma cadeia Descendente -- `tag *` e `.classe *`,
//     provando que o campo sobrevive à junção-por-combinador, não só um compound sozinho.
// ---------------------------------------------------------------------------
void test_form_universal_descendant() {
  {
    SheetParseResult result;
    const Rule* rule =
        parse_single_rule("div * { color: red; }", &result, "form_universal_descendant_tag");
    if (rule) {
      const auto& sel = rule->selectors[0];
      check(sel.compounds.size() == 2, "form_universal_descendant_tag: two compounds");
      check(sel.combinators.size() == 1 && sel.combinators[0] == Combinator::Descendant,
            "form_universal_descendant_tag: Descendant combinator");
      check(sel.compounds[0].tag == "div",
            "form_universal_descendant_tag: first compound tag == 'div'");
      check(sel.compounds[1].universal,
            "form_universal_descendant_tag: second compound universal == true");
    }
  }
  {
    SheetParseResult result;
    const Rule* rule = parse_single_rule(".foo * { color: red; }", &result,
                                         "form_universal_descendant_class");
    if (rule) {
      const auto& sel = rule->selectors[0];
      check(sel.compounds.size() == 2, "form_universal_descendant_class: two compounds");
      check(sel.combinators[0] == Combinator::Descendant,
            "form_universal_descendant_class: Descendant combinator");
      check(sel.compounds[0].classes.size() == 1 && sel.compounds[0].classes[0] == "foo",
            "form_universal_descendant_class: first compound class == 'foo'");
      check(sel.compounds[1].universal,
            "form_universal_descendant_class: second compound universal == true");
    }
  }
}

// ---------------------------------------------------------------------------
// EN: Universal on EITHER side of a Child (`>`) combinator -- `* > div` (first compound universal,
//     requires SOME parent) and `div > *` (second compound universal).
// PT: Universal dos DOIS lados de um combinador Filho (`>`) -- `* > div` (primeiro compound
//     universal, exige ALGUM pai) e `div > *` (segundo compound universal).
// ---------------------------------------------------------------------------
void test_form_universal_child() {
  {
    SheetParseResult result;
    const Rule* rule =
        parse_single_rule("* > div { color: red; }", &result, "form_universal_child_first");
    if (rule) {
      const auto& sel = rule->selectors[0];
      check(sel.compounds.size() == 2, "form_universal_child_first: two compounds");
      check(sel.combinators[0] == Combinator::Child,
            "form_universal_child_first: Child combinator");
      check(sel.compounds[0].universal,
            "form_universal_child_first: first compound universal == true");
      check(sel.compounds[1].tag == "div",
            "form_universal_child_first: second compound tag == 'div'");
    }
  }
  {
    SheetParseResult result;
    const Rule* rule =
        parse_single_rule("div > * { color: red; }", &result, "form_universal_child_second");
    if (rule) {
      const auto& sel = rule->selectors[0];
      check(sel.compounds.size() == 2, "form_universal_child_second: two compounds");
      check(sel.combinators[0] == Combinator::Child,
            "form_universal_child_second: Child combinator");
      check(sel.compounds[0].tag == "div",
            "form_universal_child_second: first compound tag == 'div'");
      check(sel.compounds[1].universal,
            "form_universal_child_second: second compound universal == true");
    }
  }
}

// ---------------------------------------------------------------------------
// EN: Comma-list with a universal entry -- BOTH entries survive (contrast
//     `test_comma_list_invalid_entry_drops_only_itself` above, which now uses `[bad]` instead of
//     `*` for exactly this reason: `*` is no longer the invalid one).
// PT: Lista-vírgula com uma entrada universal -- AS DUAS entradas sobrevivem (contraste o próprio
//     `test_comma_list_invalid_entry_drops_only_itself` acima, que agora usa `[bad]` em vez de `*`
//     exatamente por este motivo: `*` deixou de ser a inválida).
// ---------------------------------------------------------------------------
void test_form_universal_comma_list() {
  SheetParseResult result;
  const Rule* rule =
      parse_single_rule("*, div { color: red; }", &result, "form_universal_comma_list");
  if (!rule) return;
  check(rule->selectors.size() == 2,
        "form_universal_comma_list: BOTH entries survive (universal is no longer fail-high)");
  if (rule->selectors.size() == 2) {
    check(rule->selectors[0].compounds[0].universal,
          "form_universal_comma_list: entry 0 is universal");
    check(rule->selectors[1].compounds[0].tag == "div",
          "form_universal_comma_list: entry 1 tag == 'div'");
  }
}

// ---------------------------------------------------------------------------
// EN: `**` and `div*` are STILL rejected -- not because `*` itself is unsupported (it now is), but
//     because a trailing byte glued directly onto an already-closed compound is trailing garbage
//     (the SAME check that already rejects `a[href]`-shaped input, `parse_compound`'s own final
//     `if`). Equivalent-observable to the pin, which parses both into a LITERAL tag string
//     (`"*"`/`"div*"`) that can never match any real element -- `StyleSheetParser.cpp:1105-1138`'s
//     own `*`-skip fires only ONCE, right at a compound's own start; a SECOND `*` (or one glued
//     onto a preceding identifier with no separator) falls through to the ordinary identifier-token
//     scan and becomes part of a literal tag name nothing in a real document is ever named. Both
//     sides therefore NEVER apply to any element -- the pin via a structurally-valid-but-unmatchable
//     rule, this parser via dropping the entry outright with a diagnostic -- never via the SAME
//     mechanism, but always with the SAME end result: nothing.
// PT: `**` e `div*` CONTINUAM rejeitados -- não porque o `*` em si não é suportado (agora é), mas
//     porque um byte final colado direto num compound já-fechado é lixo final (a MESMA checagem que
//     já rejeita entrada com a forma `a[href]`, o próprio `if` final do `parse_compound`).
//     Equivalente-observável ao pin, que parseia os dois num texto de tag LITERAL (`"*"`/`"div*"`)
//     que nunca consegue casar com nenhum elemento real -- o próprio skip-de-`*` do
//     `StyleSheetParser.cpp:1105-1138` dispara só UMA vez, bem no começo de um compound; um SEGUNDO
//     `*` (ou um colado num identificador precedente sem separador) cai no scan comum de
//     token-identificador e vira parte de um nome de tag literal que nada num documento real tem
//     algum dia o nome. Os dois lados portanto NUNCA se aplicam a elemento nenhum -- o pin via uma
//     regra estruturalmente-válida-mas-nunca-casável, este parser via descartar a entrada de vez com
//     um diagnóstico -- nunca pelo MESMO mecanismo, mas sempre com o MESMO resultado final: nada.
// ---------------------------------------------------------------------------
void test_fail_high_double_universal_and_glued_tag() {
  {
    auto result = parse_stylesheet("** { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_double_universal: '**' rule did NOT register, sibling '.ok' survived");
    check(!result.diagnostics.empty(),
          "fail_high_double_universal: a ParseDiagnostic was recorded");
  }
  {
    auto result = parse_stylesheet("div* { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_glued_tag_star: 'div*' rule did NOT register, sibling '.ok' survived");
    check(!result.diagnostics.empty(), "fail_high_glued_tag_star: a ParseDiagnostic was recorded");
  }
}

// ---------------------------------------------------------------------------
// EN: FAIL-HIGH form 1 -- attribute selector `[x]` (0 measured).
// PT: Forma FAIL-HIGH 1 -- seletor de atributo `[x]` (0 medidos).
// ---------------------------------------------------------------------------
void test_fail_high_attribute() {
  auto result = parse_stylesheet("a[href] { color: red; } .ok { color: blue; }");
  if (!result.sheet) {
    check(false, "fail_high_attribute: sheet is non-null");
    return;
  }
  check(result.sheet->rules.size() == 1, "fail_high_attribute: the '[href]' rule did NOT register");
  check(!result.diagnostics.empty(), "fail_high_attribute: a ParseDiagnostic was recorded");
}

// ---------------------------------------------------------------------------
// EN: FAIL-HIGH form 2 -- sibling combinators `+`/`~` (0 measured).
// PT: Forma FAIL-HIGH 2 -- combinadores irmão `+`/`~` (0 medidos).
// ---------------------------------------------------------------------------
void test_fail_high_sibling() {
  {
    auto result = parse_stylesheet("a + b { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_sibling: '+' rule did NOT register, sibling '.ok' survived");
  }
  {
    auto result = parse_stylesheet("a ~ b { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_sibling: '~' rule did NOT register, sibling '.ok' survived");
  }
}

// ---------------------------------------------------------------------------
// EN: FAIL-HIGH form 3 -- `:not(...)` and `nth-child(...)` (0 measured, docs/rmlx-subset.md
//     section 2's own real-zero cuts, restated by section 6.2). Only `:hover` is an authorized
//     pseudo-class -- every other pseudo-class name fails the whole rule.
// PT: Forma FAIL-HIGH 3 -- `:not(...)` e `nth-child(...)` (0 medidos, os próprios cortes
//     real-zero da seção 2 do docs/rmlx-subset.md, restatados pela seção 6.2). Só `:hover` é
//     pseudo-classe autorizada -- todo outro nome de pseudo-classe reprova a regra inteira.
// ---------------------------------------------------------------------------
void test_fail_high_unsupported_pseudo_class() {
  {
    auto result = parse_stylesheet(".x:not(.y) { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_unsupported_pseudo_class: ':not(' rule did NOT register");
  }
  {
    auto result = parse_stylesheet("li:nth-child(2) { color: red; } .ok { color: blue; }");
    check(result.sheet && result.sheet->rules.size() == 1,
          "fail_high_unsupported_pseudo_class: ':nth-child(' rule did NOT register");
  }
}

} // namespace

int main() {
  test_form_class();
  test_form_id();
  test_form_descendant();
  test_form_tag();
  test_form_pseudo_hover();
  test_form_comma_list();
  test_comma_list_invalid_entry_drops_only_itself();
  test_comma_list_ua_stylesheet_shape_survives_one_bad_tag();
  test_form_compound_no_combinator();
  test_form_child_combinator();
  test_form_universal_alone();
  test_form_universal_with_class();
  test_form_universal_with_id();
  test_form_universal_with_hover();
  test_form_universal_tag_accident();
  test_form_universal_descendant();
  test_form_universal_child();
  test_form_universal_comma_list();
  test_fail_high_double_universal_and_glued_tag();
  test_fail_high_attribute();
  test_fail_high_sibling();
  test_fail_high_unsupported_pseudo_class();

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_selector_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_selector_sanity: PASS");
  return 0;
}
