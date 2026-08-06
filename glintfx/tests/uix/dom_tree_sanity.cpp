// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S2 -- functional unit test for glintfx::uix::{Node,Element,Text,Document}.
//     Standalone, no RmlUi/GLFW/GL -- see glintfx/src/uix/dom/dom_tree.hpp's own header comment
//     for the full scope/boundary this module holds itself to. Every case below traces back to a
//     specific uix-dom.md ("docs/uix-dom.md") section, or to a design decision this slice's own
//     header comment argues for -- deliberately not "make up plausible tree edge cases", it is
//     "prove the model does exactly what S3/S4 will need and nothing this arc has not decided".
// PT: RMLX-1/S2 -- teste unit funcional pro glintfx::uix::{Node,Element,Text,Document}.
//     Standalone, sem RmlUi/GLFW/GL -- ver o próprio comentário de cabeçalho do
//     glintfx/src/uix/dom/dom_tree.hpp pro escopo/fronteira completos a que este módulo se prende.
//     Todo caso abaixo remonta a uma seção específica do uix-dom.md ("docs/uix-dom.md") ou a uma
//     decisão de desenho que o próprio comentário de cabeçalho desta fatia argumenta --
//     deliberadamente isto não é "inventar casos de borda de árvore plausíveis", é "provar que o
//     modelo faz exatamente o que a S3/S4 vão precisar e nada que este arco ainda não decidiu".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dom_tree.hpp"

#include <cstdio>
#include <memory>
#include <string_view>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void check_eq(std::string_view got, std::string_view want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::AppendOutcome;
using glintfx::uix::as_element;
using glintfx::uix::as_text;
using glintfx::uix::Document;
using glintfx::uix::Element;
using glintfx::uix::Node;
using glintfx::uix::NodeKind;
using glintfx::uix::Text;

// ---------------------------------------------------------------------------
// EN: Case 1 -- a fresh Document already owns a body ELEM, per uix-dom.md section 3 ("a document
//     missing <body> entirely is a parse error, out of this format's scope") and section 5 ("the
//     body root itself gets the full element block too, no special-cased root").
// PT: Caso 1 -- um Document novo já é dono de um ELEM body, pela seção 3 do uix-dom.md ("um
//     documento sem <body> nenhum é erro de parse, fora do escopo deste formato") e a seção 5 ("a
//     própria raiz body recebe o bloco de elemento completo também, sem atalho especial de raiz").
// ---------------------------------------------------------------------------
void test_document_body_root_shape() {
  Document doc;
  check_eq(doc.body().tag(), "body", "fresh Document: body tag is 'body'");
  check(doc.body().parent() == nullptr, "fresh Document: body has no parent");
  check(doc.body().depth() == 1, "fresh Document: body depth is 1 (the root)");
  check(doc.body().child_count() == 0, "fresh Document: body starts with zero children");
  check(!doc.body().has_id(), "fresh Document: body has no id");
  check(doc.body().classes().empty(), "fresh Document: body has no classes");
  check(doc.body().attributes().empty(), "fresh Document: body has no attributes");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- HeadContent defaults to ABSENT (uix-dom.md section 4: "HEAD ABSENT is emitted,
//     verbatim, when the source document has no <head> element at all"); set_head/clear_head
//     round-trip.
// PT: Caso 2 -- HeadContent começa em ABSENT (seção 4 do uix-dom.md: "HEAD ABSENT é emitido,
//     verbatim, quando o documento-fonte não tem <head> nenhum"); round-trip de
//     set_head/clear_head.
// ---------------------------------------------------------------------------
void test_document_head_default_absent_and_roundtrip() {
  Document doc;
  check(!doc.head().present, "fresh Document: head defaults to ABSENT");
  check(doc.head().raw.empty(), "fresh Document: head raw is empty when absent");

  doc.set_head("\n<style>body{color:white}</style>\n");
  check(doc.head().present, "after set_head: head is PRESENT");
  check_eq(doc.head().raw, "\n<style>body{color:white}</style>\n",
           "after set_head: raw bytes stored verbatim, un-escaped, un-entity-decoded");

  doc.clear_head();
  check(!doc.head().present, "after clear_head: head is ABSENT again");
  check(doc.head().raw.empty(), "after clear_head: raw is cleared too");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- append_child wires parent()/depth()/kind() correctly, and children() reflects
//     insertion order (uix-dom.md section 5: children emitted "in source order").
// PT: Caso 3 -- append_child conecta parent()/depth()/kind() corretamente, e children() reflete
//     a ordem de inserção (seção 5 do uix-dom.md: filhos emitidos "na ordem da fonte").
// ---------------------------------------------------------------------------
void test_append_child_wires_parent_depth_order() {
  Document doc;
  Element& body = doc.body();

  auto div = std::make_unique<Element>("div");
  Element* div_raw = div.get();
  auto div_append = body.append_child(std::move(div));
  check(div_append.outcome == AppendOutcome::Appended, "append div: outcome Appended");
  check(div_append.node == div_raw, "append div: returned node is the same object");
  check(div_raw->parent() == &body, "append div: parent() is body");
  check(div_raw->depth() == 2, "append div: depth is body's depth + 1 == 2");

  auto span = std::make_unique<Element>("span");
  Element* span_raw = span.get();
  auto span_append = div_raw->append_child(std::move(span));
  check(span_append.outcome == AppendOutcome::Appended, "append span: outcome Appended");
  check(span_raw->parent() == div_raw, "append span: parent() is div");
  check(span_raw->depth() == 3, "append span: depth is div's depth + 1 == 3");

  check(body.child_count() == 1, "body has exactly 1 child (div)");
  check(body.children()[0].get() == div_raw, "body's only child, by position, is div");
  check(div_raw->child_count() == 1, "div has exactly 1 child (span)");
  check(div_raw->children()[0].get() == span_raw, "div's only child, by position, is span");

  // EN: source order is preserved -- append a second child, first still comes first.
  // PT: ordem-fonte é preservada -- soma um segundo filho, o primeiro continua primeiro.
  auto p = std::make_unique<Element>("p");
  Element* p_raw = p.get();
  body.append_child(std::move(p));
  check(body.child_count() == 2, "body now has 2 children (div, p)");
  check(body.children()[0].get() == div_raw, "body child 0 is still div");
  check(body.children()[1].get() == p_raw, "body child 1 is p");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- 🔴 the load-bearing decision: whitespace-only Text is filtered by
//     Element::append_child itself, never occupies a child slot (uix-dom.md section 6a). Mirrors
//     the worked example's own body/0/0 shape (a real fixture-driven fact, not invented): pure
//     whitespace before/after a real element never counts.
// PT: Caso 4 -- 🔴 a decisão que carrega peso: Text só-whitespace é filtrado pelo próprio
//     Element::append_child, nunca ocupa slot de filho (seção 6a do uix-dom.md). Espelha a forma
//     body/0/0 do próprio exemplo trabalhado (um fato guiado por fixture real, não inventado):
//     whitespace puro antes/depois de um elemento real nunca conta.
// ---------------------------------------------------------------------------
void test_append_child_filters_whitespace_only_text() {
  Document doc;
  Element& body = doc.body();

  auto ws = std::make_unique<Text>("\n  \t\r\n");
  auto ws_result = body.append_child(std::move(ws));
  check(ws_result.outcome == AppendOutcome::FilteredWhitespaceText,
        "whitespace-only Text('\\n  \\t\\r\\n'): outcome is FilteredWhitespaceText");
  check(ws_result.node == nullptr,
        "whitespace-only Text: returned node is nullptr (not adopted)");
  check(body.child_count() == 0, "whitespace-only Text: body still has zero children");

  // EN: empty Text is ALSO filtered -- std::all_of over an empty range is vacuously true, exactly
  //     matching Factory::InstanceElementText's own only_white_space computation (see this
  //     module's own header comment, point (1)).
  // PT: Text vazio TAMBÉM é filtrado -- std::all_of sobre um range vazio é vacuamente verdadeiro,
  //     batendo exatamente com o próprio cálculo de only_white_space do
  //     Factory::InstanceElementText (ver o próprio comentário de cabeçalho deste módulo, ponto
  //     (1)).
  auto empty_text = std::make_unique<Text>("");
  auto empty_result = body.append_child(std::move(empty_text));
  check(empty_result.outcome == AppendOutcome::FilteredWhitespaceText,
        "empty Text(''): outcome is FilteredWhitespaceText (empty range -> all_of vacuously true)");
  check(body.child_count() == 0, "empty Text: body still has zero children");

  // EN: mixed content (real text plus padding) is NOT filtered, and is preserved byte-verbatim,
  //     no trim -- uix-dom.md section 6b's own "  Hello   world  " example.
  // PT: conteúdo misto (texto real mais padding) NÃO é filtrado, e é preservado byte-verbatim,
  //     sem trim -- o próprio exemplo "  Hello   world  " da seção 6b do uix-dom.md.
  auto mixed = std::make_unique<Text>("  Hello   world  ");
  Node* mixed_raw = mixed.get();
  auto mixed_result = body.append_child(std::move(mixed));
  check(mixed_result.outcome == AppendOutcome::Appended,
        "mixed-content Text: outcome is Appended (not filtered)");
  check(mixed_result.node == mixed_raw, "mixed-content Text: returned node matches");
  check(body.child_count() == 1, "mixed-content Text: body now has 1 child");
  const Text* as_text_node = as_text(body.children()[0].get());
  check(as_text_node != nullptr, "mixed-content Text: kind() is Text, as_text() succeeds");
  if (as_text_node != nullptr) {
    check_eq(as_text_node->content(), "  Hello   world  ",
             "mixed-content Text: content preserved byte-verbatim, no trim/collapse");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- id: empty == absent (uix-dom.md section 7's empty-value asymmetry, stated
//     explicitly for id specifically).
// PT: Caso 5 -- id: vazio == ausente (a assimetria de valor-vazio da seção 7 do uix-dom.md,
//     declarada explicitamente pro id especificamente).
// ---------------------------------------------------------------------------
void test_element_id_empty_equals_absent() {
  Element el("div");
  check(!el.has_id(), "fresh Element: has_id() is false");
  check_eq(el.id(), "", "fresh Element: id() is empty");

  el.set_id("panel");
  check(el.has_id(), "after set_id('panel'): has_id() is true");
  check_eq(el.id(), "panel", "after set_id('panel'): id() returns it");

  el.set_id("");
  check(!el.has_id(), "after set_id(''): has_id() is false again -- empty == absent");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- classes are a canonical set: sorted ascending byte-wise, deduplicated, and the
//     single-token invariant (not the raw split -- S3's job) is checked against uix-dom.md
//     section 7's REVISED (2026-08-05, `UIX-CLASS-SPLIT-2`) rule: reject a literal SPACE anywhere
//     (the sole RmlUi delimiter) and a token made ENTIRELY of the other 3 whitespace bytes, but
//     ACCEPT `\t`/`\n`/`\r` EMBEDDED between real content -- `"a\tb"` is a legitimate single RmlUi
//     class token (`StringUtilities::ExpandString(raw, ' ')` captures it verbatim, see
//     dom_tree.hpp's own header comment on `add_class`). Mirrors uix-dom.md section 7's own worked
//     example: source "wide highlighted" dumps as "highlighted wide" ('h' 0x68 sorts before 'w'
//     0x77).
// PT: Caso 6 -- classes são um conjunto canônico: ordenado ascendente byte-a-byte, deduplicado, e
//     o invariante de token-único (não a separação crua -- trabalho da S3) é checado contra a
//     regra REVISADA (2026-08-05, `UIX-CLASS-SPLIT-2`) da seção 7 do uix-dom.md: rejeita um
//     ESPAÇO literal em qualquer posição (o único delimitador do RmlUi) e um token feito
//     INTEIRAMENTE dos outros 3 bytes de whitespace, mas ACEITA `\t`/`\n`/`\r` EMBUTIDO entre
//     conteúdo real -- `"a\tb"` é um token de classe único legítimo do RmlUi
//     (`StringUtilities::ExpandString(raw, ' ')` captura ele verbatim, ver o próprio comentário de
//     cabeçalho do `add_class` no dom_tree.hpp). Espelha o próprio exemplo trabalhado da seção 7
//     do uix-dom.md: fonte "wide highlighted" dumpa como "highlighted wide" ('h' 0x68 ordena antes
//     de 'w' 0x77).
// ---------------------------------------------------------------------------
void test_element_classes_sorted_deduped_single_token() {
  Element el("div");
  check(el.add_class("wide"), "add_class('wide'): accepted");
  check(el.add_class("highlighted"), "add_class('highlighted'): accepted");
  check(!el.add_class("wide"), "add_class('wide') again: duplicate token rejected");
  check(!el.add_class(""), "add_class(''): empty token rejected");
  check(!el.add_class("two words"),
        "add_class('two words'): literal space anywhere rejected -- space is the RmlUi "
        "class-list delimiter, UIX-CLASS-SPLIT-2");
  check(el.add_class("a\tb"),
        "add_class('a\\tb'): EMBEDDED tab accepted -- upstream ExpandString(raw, ' ') captures "
        "it verbatim, UIX-CLASS-SPLIT-2");
  check(el.has_class("a\tb"), "has_class('a\\tb'): the embedded-tab token round-trips");
  check(!el.add_class("\t"),
        "add_class('\\t'): a token made ENTIRELY of the other 3 whitespace bytes is still "
        "rejected -- ExpandString never captures a run of only those as a token");
  check(!el.add_class("\n\r"),
        "add_class('\\n\\r'): same rejection for a multi-byte all-whitespace (non-space) run");

  check(el.classes().size() == 3,
        "classes(): exactly 3 distinct tokens ('wide', 'highlighted', "
        "'a\\tb' -- dedup worked, the 2 rejected calls added nothing)");
  auto it = el.classes().begin();
  check_eq(*it, "a\tb", "classes() iteration order [0]: 'a\\tb' (byte-wise 'a'=0x61 sorts first)");
  ++it;
  check_eq(*it, "highlighted",
           "classes() iteration order [1]: 'highlighted' (byte-wise 'h'<'w')");
  ++it;
  check_eq(*it, "wide", "classes() iteration order [2]: 'wide'");

  check(el.has_class("wide"), "has_class('wide'): true");
  check(!el.has_class("absent"), "has_class('absent'): false");

  check(el.remove_class("wide"), "remove_class('wide'): removed, returns true");
  check(!el.has_class("wide"), "after remove_class('wide'): has_class is false");
  check(!el.remove_class("wide"), "remove_class('wide') again: already gone, returns false");
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- generic attributes: id/class are RESERVED (rejected by set_attribute), and the
//     empty-value-vs-absent asymmetry for a GENERIC attribute is the OPPOSITE of id's (uix-dom.md
//     section 7: "a generic attribute with an empty value is NOT the same as that attribute being
//     absent").
// PT: Caso 7 -- atributos genéricos: id/class são RESERVADOS (rejeitados por set_attribute), e a
//     assimetria valor-vazio-vs-ausente pra um atributo GENÉRICO é o OPOSTO da do id (seção 7 do
//     uix-dom.md: "um atributo genérico com valor vazio NÃO é o mesmo que esse atributo estar
//     ausente").
// ---------------------------------------------------------------------------
void test_element_attributes_reserved_names_and_empty_value_asymmetry() {
  Element el("div");
  check(!el.set_attribute("id", "should-not-stick"),
        "set_attribute('id', ...): rejected, id is reserved for set_id()");
  check(!el.has_attribute("id"), "has_attribute('id'): always false, id never in the generic map");
  check(!el.set_attribute("class", "should-not-stick"),
        "set_attribute('class', ...): rejected, class is reserved for add_class()");
  check(!el.has_attribute("class"),
        "has_attribute('class'): always false, class never in the generic map");

  check(el.set_attribute("data-if", "flag"), "set_attribute('data-if', 'flag'): accepted");
  check(el.set_attribute("title", "Panel"), "set_attribute('title', 'Panel'): accepted");
  check(el.attributes().size() == 2, "attributes(): exactly 2 entries");

  auto it = el.attributes().begin();
  check_eq(it->first, "data-if", "attributes() order [0]: 'data-if' (byte-wise before 'title')");
  ++it;
  check_eq(it->first, "title", "attributes() order [1]: 'title'");

  check(!el.has_attribute("data-if-absent"),
        "has_attribute('data-if-absent'): false, never set");

  // EN: present-with-empty-value is a DIFFERENT, real state for a generic attribute.
  // PT: presente-com-valor-vazio é um estado DIFERENTE e real pra um atributo genérico.
  check(el.set_attribute("data-empty", ""), "set_attribute('data-empty', ''): accepted");
  check(el.has_attribute("data-empty"),
        "has_attribute('data-empty'): true -- present with empty value, NOT the same as absent");
  auto value = el.attribute("data-empty");
  check(value.has_value(), "attribute('data-empty'): has a value (empty string, but present)");
  if (value.has_value()) {
    check_eq(*value, "", "attribute('data-empty'): value is the empty string");
  }
  check(!el.attribute("never-set").has_value(),
        "attribute('never-set'): std::nullopt, genuinely absent");

  check(el.remove_attribute("title"), "remove_attribute('title'): removed, returns true");
  check(!el.has_attribute("title"), "after remove_attribute('title'): has_attribute is false");
  check(!el.remove_attribute("title"), "remove_attribute('title') again: returns false");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- find_by_id: pre-order DFS, self included, first match wins on duplicates (a real
//     corpus fact -- see this slice's own header comment, "Lookup policy", for the grep evidence),
//     and an empty target always misses (id empty == absent, never a meaningful lookup).
// PT: Caso 8 -- find_by_id: DFS pré-ordem, incluindo o próprio nó, primeiro casamento vence em
//     duplicatas (um fato real de corpus -- ver o próprio comentário de cabeçalho desta fatia,
//     "Política de lookup", pra evidência do grep), e um alvo vazio sempre erra (id vazio ==
//     ausente, nunca uma busca com sentido).
// ---------------------------------------------------------------------------
void test_find_by_id_preorder_first_match() {
  Document doc;
  Element& body = doc.body();

  auto outer = std::make_unique<Element>("div");
  outer->set_id("outer");
  Element* outer_raw = outer.get();
  body.append_child(std::move(outer));

  auto inner_a = std::make_unique<Element>("span");
  inner_a->set_id("dup");
  Element* inner_a_raw = inner_a.get();
  outer_raw->append_child(std::move(inner_a));

  auto inner_b = std::make_unique<Element>("span");
  inner_b->set_id("dup");
  outer_raw->append_child(std::move(inner_b));

  check(body.find_by_id("outer") == outer_raw, "find_by_id('outer'): finds the direct child");
  check(body.find_by_id("dup") == inner_a_raw,
        "find_by_id('dup'): duplicate ids exist -- first pre-order match wins (inner_a, not "
        "inner_b)");
  check(outer_raw->find_by_id("outer") == outer_raw,
        "find_by_id: pre-order DFS includes self, not just descendants");
  check(body.find_by_id("nonexistent") == nullptr,
        "find_by_id('nonexistent'): nullptr, no such id anywhere");
  check(body.find_by_id("") == nullptr,
        "find_by_id(''): always nullptr, empty id is never a meaningful lookup");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- as_element/as_text: nullptr on kind mismatch, non-null on match, nullptr on a
//     null Node* input.
// PT: Caso 9 -- as_element/as_text: nullptr no descasamento de kind, não-nulo no casamento,
//     nullptr num input Node* nulo.
// ---------------------------------------------------------------------------
void test_as_element_as_text_helpers() {
  Document doc;
  Element& body = doc.body();
  auto text = std::make_unique<Text>("hello");
  Node* text_raw = body.append_child(std::move(text)).node;

  check(as_element(text_raw) == nullptr, "as_element() on a Text node: nullptr");
  check(as_text(text_raw) != nullptr, "as_text() on a Text node: non-null");
  check(as_text(static_cast<Node*>(&body)) == nullptr, "as_text() on an Element node: nullptr");
  check(as_element(static_cast<Node*>(&body)) == &body, "as_element() on an Element node: itself");

  Node* null_node = nullptr;
  check(as_element(null_node) == nullptr, "as_element(nullptr): nullptr, no crash");
  check(as_text(null_node) == nullptr, "as_text(nullptr): nullptr, no crash");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- UIX-REMOVE-CHILD: Element::remove_child removes a direct child by pointer
//     identity, returns true; false (no-op) for nullptr, an already-removed child, and a
//     grandchild (child of a DIFFERENT element) -- see dom_tree.hpp's own doc-comment for why this
//     stays a plain bool, unlike append_child's richer AppendResult.
// PT: Caso 10 -- UIX-REMOVE-CHILD: Element::remove_child remove um filho direto por identidade de
//     ponteiro, retorna true; false (no-op) pra nullptr, um filho já removido, e um neto (filho de
//     um elemento DIFERENTE) -- ver o próprio doc-comment do dom_tree.hpp pra por que isto continua
//     um bool simples, diferente do AppendResult mais rico do append_child.
// ---------------------------------------------------------------------------
void test_remove_child_direct_only_by_identity() {
  Document doc;
  Element& body = doc.body();

  auto div = std::make_unique<Element>("div");
  Element* div_raw = div.get();
  body.append_child(std::move(div));

  auto span = std::make_unique<Element>("span");
  Element* span_raw = span.get();
  div_raw->append_child(std::move(span));

  auto p = std::make_unique<Element>("p");
  Element* p_raw = p.get();
  body.append_child(std::move(p));

  check(body.child_count() == 2, "precondition: body has 2 children (div, p)");

  // EN: nullptr is a no-op, tree untouched.
  // PT: nullptr é no-op, árvore intocada.
  check(!body.remove_child(nullptr), "remove_child(nullptr): false, no-op");
  check(body.child_count() == 2, "remove_child(nullptr): body still has 2 children");

  // EN: span is a GRANDCHILD of body (child of div), not a direct child -- body.remove_child must
  //     NOT find it.
  // PT: span é NETO do body (filho do div), não filho direto -- body.remove_child NÃO pode achá-lo.
  check(!body.remove_child(span_raw), "remove_child(grandchild): false, not a direct child");
  check(body.child_count() == 2, "remove_child(grandchild): body still has 2 children");
  check(div_raw->child_count() == 1, "remove_child(grandchild): div's own child untouched");

  // EN: div.remove_child(span) DOES find it -- span is div's direct child.
  // PT: div.remove_child(span) ACHA -- span é filho direto do div.
  check(div_raw->remove_child(span_raw), "div.remove_child(span): true, span is div's direct child");
  check(div_raw->child_count() == 0, "after remove: div has zero children");

  // EN: removing div from body: found, removed. A repeat attempt afterward is the "already
  //     removed" no-op case.
  // PT: remover div do body: achou, removeu. Uma segunda tentativa depois é o caso de no-op
  //     "já removido".
  check(body.remove_child(div_raw), "body.remove_child(div): true, div is body's direct child");
  check(body.child_count() == 1, "after remove: body has 1 child (p)");
  check(body.children()[0].get() == p_raw, "after remove: body's remaining child is p");
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- UIX-REMOVE-CHILD: Element::clear_children removes every direct child and returns
//     the count removed; idempotent in both directions (clearing an already-empty element returns
//     0, not an error; clearing twice in a row is safe).
// PT: Caso 11 -- UIX-REMOVE-CHILD: Element::clear_children remove todo filho direto e retorna a
//     contagem removida; idempotente nas duas direções (limpar um elemento já vazio retorna 0, não
//     é erro; limpar duas vezes seguidas é seguro).
// ---------------------------------------------------------------------------
void test_clear_children_removes_all_and_reports_count() {
  Document doc;
  Element& body = doc.body();

  // EN: clearing an already-empty element: 0 removed, no crash.
  // PT: limpar um elemento já vazio: 0 removido, sem crash.
  check(body.clear_children() == 0, "clear_children() on empty body: returns 0");
  check(body.child_count() == 0, "clear_children() on empty body: stays empty");

  body.append_child(std::make_unique<Element>("div"));
  body.append_child(std::make_unique<Element>("p"));
  body.append_child(std::make_unique<Text>("hello"));
  check(body.child_count() == 3, "precondition: body has 3 children (div, p, Text)");

  std::size_t removed = body.clear_children();
  check(removed == 3, "clear_children(): returns 3, matching the prior child_count()");
  check(body.child_count() == 0, "after clear_children(): body has zero children");

  // EN: clearing again, now that it is already empty: 0 removed, idempotent.
  // PT: limpar de novo, já vazio: 0 removido, idempotente.
  check(body.clear_children() == 0, "clear_children() called again on now-empty body: returns 0");
}

} // namespace

int main() {
  test_document_body_root_shape();
  test_document_head_default_absent_and_roundtrip();
  test_append_child_wires_parent_depth_order();
  test_append_child_filters_whitespace_only_text();
  test_element_id_empty_equals_absent();
  test_element_classes_sorted_deduped_single_token();
  test_element_attributes_reserved_names_and_empty_value_asymmetry();
  test_find_by_id_preorder_first_match();
  test_as_element_as_text_helpers();
  test_remove_child_direct_only_by_identity();
  test_clear_children_removes_all_and_reports_count();

  if (g_failures > 0) {
    std::fprintf(stderr, "dom_tree_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dom_tree_sanity: PASS");
  return 0;
}
