// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S4 -- functional unit test for glintfx::uix::{set_text,add_class,remove_class}
//     (glintfx/src/uix/dom/dom_api.{hpp,cpp}). Standalone, no RmlUi/GLFW/GL -- same discipline as
//     dom_tree_sanity.cpp. Every case below traces back to a specific paragraph of dom_api.hpp's
//     own doc-comments (the WHY lives there, once; this file proves the WHAT).
// PT: RMLX-1/S4 -- teste unit funcional pro glintfx::uix::{set_text,add_class,remove_class}
//     (glintfx/src/uix/dom/dom_api.{hpp,cpp}). Standalone, sem RmlUi/GLFW/GL -- mesma disciplina do
//     dom_tree_sanity.cpp. Todo caso abaixo remonta a um parágrafo específico dos próprios
//     doc-comments do dom_api.hpp (o PORQUÊ mora lá, uma vez; este arquivo prova o QUÊ).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dom_api.hpp"
#include "uix/dom/dom_tree.hpp"

#include <cstdio>
#include <memory>
#include <string>
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

using glintfx::uix::add_class;
using glintfx::uix::as_element;
using glintfx::uix::as_text;
using glintfx::uix::Document;
using glintfx::uix::Element;
using glintfx::uix::remove_class;
using glintfx::uix::set_text;
using glintfx::uix::Text;

// ---------------------------------------------------------------------------
// EN: Case 1 -- shape (a): set_text on a fresh, childless element appends a single Text child and
//     round-trips: write, then read back BOTH the new content AND that nothing else in the tree
//     moved (dom_api.hpp's set_text doc-comment, shape (a)).
// PT: Caso 1 -- forma (a): set_text num elemento novo, sem filhos, soma um único filho Text e faz
//     round-trip: escreve, depois lê de volta TANTO o conteúdo novo QUANTO que nada mais na árvore
//     se moveu (doc-comment do set_text no dom_api.hpp, forma (a)).
// ---------------------------------------------------------------------------
void test_set_text_on_empty_element_appends_and_roundtrips() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  auto sibling = std::make_unique<Element>("span");
  sibling->set_id("sibling");
  Element* sibling_raw = sibling.get();
  body.append_child(std::move(sibling));

  check(label_raw->child_count() == 0, "precondition: label starts with zero children");

  bool ok = set_text(doc, "label", "Hello");
  check(ok, "set_text(doc, \"label\", \"Hello\"): returns true");
  check(label_raw->child_count() == 1, "after set_text: label has exactly 1 child");
  const Text* content = as_text(label_raw->children()[0].get());
  check(content != nullptr, "after set_text: label's only child is a Text node");
  if (content != nullptr) {
    check_eq(content->content(), "Hello", "after set_text: Text content is exactly \"Hello\"");
  }

  // EN: round-trip's second half -- prove nothing ELSE in the tree moved: sibling untouched by
  //     identity, body's own child count/order untouched.
  // PT: segunda metade do round-trip -- prova que nada MAIS na árvore se moveu: sibling intocado
  //     por identidade, contagem/ordem de filho do próprio body intocada.
  check(body.child_count() == 2, "after set_text: body still has exactly 2 children");
  check(body.children()[0].get() == label_raw, "after set_text: body child 0 is still label");
  check(body.children()[1].get() == sibling_raw, "after set_text: body child 1 is still sibling");
  check(sibling_raw->child_count() == 0, "after set_text: sibling untouched, still zero children");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- shape (b): set_text called a SECOND time on the same id mutates the existing sole
//     Text child IN PLACE -- proven by pointer identity, not just content -- per dom_api.hpp's
//     "same Node/Text object identity is preserved across the call" claim.
// PT: Caso 2 -- forma (b): set_text chamado uma SEGUNDA vez no mesmo id muta o único filho Text
//     existente NO LUGAR -- provado por identidade de ponteiro, não só por conteúdo -- pela
//     própria afirmação do dom_api.hpp "a MESMA identidade de objeto Node/Text é preservada
//     através da chamada".
// ---------------------------------------------------------------------------
void test_set_text_overwrites_existing_sole_text_child() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  check(set_text(doc, "label", "first"), "set_text #1: returns true");
  check(label_raw->child_count() == 1, "after set_text #1: exactly 1 child");
  const Text* first_child = as_text(label_raw->children()[0].get());
  check(first_child != nullptr, "after set_text #1: sole child is Text");
  const Text* first_child_identity = first_child;

  check(set_text(doc, "label", "second"), "set_text #2 (overwrite): returns true");
  check(label_raw->child_count() == 1,
        "after set_text #2: STILL exactly 1 child (no new sibling created)");
  const Text* second_child = as_text(label_raw->children()[0].get());
  check(second_child != nullptr, "after set_text #2: sole child is still Text");
  check(second_child == first_child_identity,
        "after set_text #2: SAME Text object identity -- mutated in place, not replaced");
  if (second_child != nullptr) {
    check_eq(second_child->content(), "second", "after set_text #2: content is now \"second\"");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- shape (a) + empty text: set_text(doc, id, "") on a childless element stays
//     childless -- matches upstream's `SetInnerRML("")` (rml.empty() -> Factory never called,
//     Element.cpp:1174), returns true (success, zero children is the CORRECT outcome, not a
//     failure).
// PT: Caso 3 -- forma (a) + texto vazio: set_text(doc, id, "") num elemento sem filhos continua
//     sem filhos -- bate com o `SetInnerRML("")` do upstream (rml.empty() -> Factory nunca
//     chamado, Element.cpp:1174), retorna true (sucesso, zero filhos é o resultado CORRETO, não
//     uma falha).
// ---------------------------------------------------------------------------
void test_set_text_empty_string_on_empty_element_stays_empty() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  bool ok = set_text(doc, "label", "");
  check(ok, "set_text(doc, \"label\", \"\"): returns true");
  check(label_raw->child_count() == 0,
        "set_text with empty text on childless element: stays zero children");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- shape (a) + whitespace-only text: filtered by append_child's own existence
//     invariant, EXACT match to Factory::InstanceElementText's only_white_space filter
//     (examples/RmlUi/Source/Core/Factory.cpp:338-341) -- true, zero children, not an error.
// PT: Caso 4 -- forma (a) + texto só-whitespace: filtrado pelo próprio invariante de existência
//     do append_child, casamento EXATO com o filtro only_white_space do
//     Factory::InstanceElementText (examples/RmlUi/Source/Core/Factory.cpp:338-341) -- true, zero
//     filhos, não é erro.
// ---------------------------------------------------------------------------
void test_set_text_whitespace_only_on_empty_element_filtered() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  bool ok = set_text(doc, "label", "   \t\n  ");
  check(ok, "set_text(doc, \"label\", whitespace-only): returns true");
  check(label_raw->child_count() == 0,
        "set_text with whitespace-only text on childless element: stays zero children (filtered)");
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- 🔴 shape (b) + whitespace/empty text: THE DOCUMENTED, NARROWER divergence --
//     set_content has no filter (unlike append_child), so a repeated set_text(doc, id, "") on an
//     element that already holds a sole Text child leaves that child PRESENT with EMPTY content
//     (child_count stays 1), NOT reverting to zero children like the empty-element case (Case 3)
//     does. This is the exact "narrower instance of the load-bearing gap" dom_api.hpp's own
//     doc-comment names -- pinned here, not papered over.
// PT: Caso 5 -- 🔴 forma (b) + texto whitespace/vazio: A DIVERGÊNCIA DOCUMENTADA, MAIS ESTREITA --
//     set_content não tem filtro (diferente do append_child), então um set_text(doc, id, "")
//     repetido num elemento que já guarda um único filho Text deixa aquele filho PRESENTE com
//     conteúdo VAZIO (child_count continua 1), NÃO revertendo pra zero filhos como o caso de
//     elemento-vazio (Caso 3) faz. Esta é exatamente a "instância mais estreita da lacuna
//     carregada-de-peso" que o próprio doc-comment do dom_api.hpp nomeia -- fixada aqui, não
//     encoberta.
// ---------------------------------------------------------------------------
void test_set_text_whitespace_on_existing_text_child_leaves_empty_residual() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  check(set_text(doc, "label", "hi"), "set_text #1 (\"hi\"): returns true");
  check(label_raw->child_count() == 1, "after set_text #1: exactly 1 child");

  bool ok = set_text(doc, "label", "");
  check(ok, "set_text #2 (\"\") over an existing Text child: STILL returns true");
  check(label_raw->child_count() == 1,
        "🔴 known divergence: child_count STAYS 1 (an empty Text residual), does NOT revert to 0");
  const Text* residual = as_text(label_raw->children()[0].get());
  check(residual != nullptr, "the residual child is still a Text node");
  if (residual != nullptr) {
    check_eq(residual->content(), "", "the residual Text node's content is the empty string");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- 🔴 shape (c): an element whose children are NOT the "zero, or one Text" shape is
//     out of set_text's reach given S2's append-only surface -- refuses (false), tree untouched
//     byte-for-byte. Two independent sub-cases: a single Element child, and two Text children.
// PT: Caso 6 -- 🔴 forma (c): um elemento cujos filhos NÃO são a forma "zero, ou um Text" está
//     fora do alcance do set_text dada a superfície só-de-append da S2 -- recusa (false), árvore
//     intocada byte-por-byte. Dois subcasos independentes: um único filho Element, e dois filhos
//     Text.
// ---------------------------------------------------------------------------
void test_set_text_element_with_non_text_or_multiple_children_refused() {
  // Sub-case 6a: sole child is an Element, not Text.
  {
    Document doc;
    Element& body = doc.body();

    auto panel = std::make_unique<Element>("div");
    panel->set_id("panel");
    Element* panel_raw = panel.get();
    body.append_child(std::move(panel));

    auto icon = std::make_unique<Element>("icon");
    Element* icon_raw = icon.get();
    panel_raw->append_child(std::move(icon));

    bool ok = set_text(doc, "panel", "replacement");
    check(!ok, "6a: set_text on an element whose sole child is an Element: refused (false)");
    check(panel_raw->child_count() == 1, "6a: child_count untouched (still 1)");
    check(panel_raw->children()[0].get() == icon_raw,
          "6a: the original Element child is untouched, byte-for-byte, same identity");
  }

  // Sub-case 6b: two Text children (a shape append_child alone can build -- append_child does not
  // reject a SECOND non-whitespace Text child, only a whitespace-only one).
  {
    Document doc;
    Element& body = doc.body();

    auto panel = std::make_unique<Element>("div");
    panel->set_id("panel");
    Element* panel_raw = panel.get();
    body.append_child(std::move(panel));

    panel_raw->append_child(std::make_unique<Text>("first"));
    panel_raw->append_child(std::make_unique<Text>("second"));
    check(panel_raw->child_count() == 2, "6b precondition: panel has 2 Text children");

    bool ok = set_text(doc, "panel", "replacement");
    check(!ok, "6b: set_text on an element with 2 Text children: refused (false)");
    check(panel_raw->child_count() == 2, "6b: child_count untouched (still 2)");
    const Text* c0 = as_text(panel_raw->children()[0].get());
    const Text* c1 = as_text(panel_raw->children()[1].get());
    check(c0 != nullptr && c1 != nullptr, "6b: both children are still Text nodes");
    if (c0 != nullptr) {
      check_eq(c0->content(), "first", "6b: child 0 content untouched (\"first\")");
    }
    if (c1 != nullptr) {
      check_eq(c1->content(), "second", "6b: child 1 content untouched (\"second\")");
    }
  }
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- unknown id: set_text/add_class/remove_class all return false, tree untouched --
//     same AUD-TEC-5 fail-high convention the RmlUi-backed facade already uses.
// PT: Caso 7 -- id desconhecido: set_text/add_class/remove_class os três retornam false, árvore
//     intocada -- mesma convenção fail-high AUD-TEC-5 que a fachada com base em RmlUi já usa.
// ---------------------------------------------------------------------------
void test_unknown_id_returns_false_for_all_three_ops() {
  Document doc;
  Element& body = doc.body();

  auto real = std::make_unique<Element>("span");
  real->set_id("real");
  body.append_child(std::move(real));

  check(!set_text(doc, "nope", "x"), "set_text(unknown id): false");
  check(!add_class(doc, "nope", "glow"), "add_class(unknown id): false");
  check(!remove_class(doc, "nope", "glow"), "remove_class(unknown id): false");
  check(body.child_count() == 1, "unknown-id calls: body's real child untouched");

  // EN: empty id is the SAME state as "no such id" (find_by_id's own contract) -- pin it here too,
  //     at the dom_api layer, not just at dom_tree's.
  // PT: id vazio é o MESMO estado de "id nenhum desses" (o próprio contrato do find_by_id) --
  //     fixa isto aqui também, na camada do dom_api, não só na do dom_tree.
  check(!set_text(doc, "", "x"), "set_text(\"\"): false, empty id never matches");
  check(!add_class(doc, "", "glow"), "add_class(\"\"): false, empty id never matches");
  check(!remove_class(doc, "", "glow"), "remove_class(\"\"): false, empty id never matches");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- duplicate ids: the FIRST element in pre-order is the one every operation resolves
//     to, the second sharing the same id is untouched -- dom_api.hpp's own "inherited, not
//     re-decided" policy, exercised for all three ops (set_text, add_class, remove_class), not
//     just the find_by_id primitive itself.
// PT: Caso 8 -- ids duplicados: o PRIMEIRO elemento em pré-ordem é o que toda operação resolve,
//     o segundo compartilhando o mesmo id fica intocado -- a própria política "herdada, não
//     re-decidida" do dom_api.hpp, exercitada pras três operações (set_text, add_class,
//     remove_class), não só pelo próprio primitivo find_by_id.
// ---------------------------------------------------------------------------
void test_duplicate_id_first_preorder_wins_for_all_three_ops() {
  Document doc;
  Element& body = doc.body();

  auto outer = std::make_unique<Element>("div");
  Element* outer_raw = outer.get();
  body.append_child(std::move(outer));

  auto first = std::make_unique<Element>("span");
  first->set_id("dup");
  Element* first_raw = first.get();
  outer_raw->append_child(std::move(first));

  auto second = std::make_unique<Element>("span");
  second->set_id("dup");
  Element* second_raw = second.get();
  outer_raw->append_child(std::move(second));

  check(set_text(doc, "dup", "hit"), "set_text(\"dup\"): returns true");
  check(first_raw->child_count() == 1 && as_text(first_raw->children()[0].get()) != nullptr,
        "set_text(\"dup\"): the FIRST pre-order element got the Text child");
  check(second_raw->child_count() == 0,
        "set_text(\"dup\"): the SECOND element sharing the id is untouched");

  check(add_class(doc, "dup", "glow"), "add_class(\"dup\"): returns true");
  check(first_raw->has_class("glow"), "add_class(\"dup\"): the FIRST element has the class");
  check(!second_raw->has_class("glow"),
        "add_class(\"dup\"): the SECOND element sharing the id is untouched");

  check(remove_class(doc, "dup", "glow"), "remove_class(\"dup\"): returns true");
  check(!first_raw->has_class("glow"), "remove_class(\"dup\"): the FIRST element lost the class");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- add_class round-trip + idempotence: repeat add_class with the same cls does NOT
//     duplicate the class-set entry, and STILL returns true (dom_api.hpp's own reconciled
//     contract, distinct from Element::add_class's own false-if-already-present bool).
// PT: Caso 9 -- round-trip + idempotência do add_class: add_class repetido com o mesmo cls NÃO
//     duplica a entrada no conjunto de classe, e AINDA retorna true (o próprio contrato
//     reconciliado do dom_api.hpp, distinto do próprio booleano
//     false-se-já-presente do Element::add_class).
// ---------------------------------------------------------------------------
void test_add_class_roundtrip_and_idempotent() {
  Document doc;
  Element& body = doc.body();

  auto el = std::make_unique<Element>("div");
  el->set_id("el");
  Element* el_raw = el.get();
  body.append_child(std::move(el));

  bool first = add_class(doc, "el", "glow");
  check(first, "add_class(\"el\", \"glow\") first time: returns true");
  check(el_raw->has_class("glow"), "after first add_class: has_class(\"glow\") is true");
  check(el_raw->classes().size() == 1, "after first add_class: exactly 1 class");

  bool second = add_class(doc, "el", "glow");
  check(second, "add_class(\"el\", \"glow\") AGAIN: STILL returns true (idempotent contract)");
  check(el_raw->has_class("glow"), "after repeat add_class: has_class(\"glow\") still true");
  check(el_raw->classes().size() == 1,
        "after repeat add_class: STILL exactly 1 class (no duplicate entry)");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- add_class rejects a structurally invalid cls (empty, or embedded whitespace)
//     BEFORE the id lookup -- and leaves the class set completely untouched.
// PT: Caso 10 -- add_class rejeita um cls estruturalmente inválido (vazio, ou whitespace embutido)
//     ANTES do lookup de id -- e deixa o conjunto de classe completamente intocado.
// ---------------------------------------------------------------------------
void test_add_class_rejects_invalid_cls() {
  Document doc;
  Element& body = doc.body();

  auto el = std::make_unique<Element>("div");
  el->set_id("el");
  Element* el_raw = el.get();
  body.append_child(std::move(el));

  check(!add_class(doc, "el", ""), "add_class(\"el\", \"\"): false, empty token rejected");
  check(!add_class(doc, "el", "two words"),
        "add_class(\"el\", \"two words\"): false, embedded whitespace rejected");
  check(el_raw->classes().empty(), "invalid-cls add_class calls: class set stays empty");
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- remove_class round-trip + idempotence: removing an absent class is a safe no-op
//     that STILL returns true (mirrors add_class's own reconciled contract), and removing a class
//     the element does have actually clears it.
// PT: Caso 11 -- round-trip + idempotência do remove_class: remover uma classe ausente é um no-op
//     seguro que AINDA retorna true (espelha o próprio contrato reconciliado do add_class), e
//     remover uma classe que o elemento realmente tem de fato a limpa.
// ---------------------------------------------------------------------------
void test_remove_class_roundtrip_and_idempotent() {
  Document doc;
  Element& body = doc.body();

  auto el = std::make_unique<Element>("div");
  el->set_id("el");
  Element* el_raw = el.get();
  body.append_child(std::move(el));

  check(add_class(doc, "el", "glow"), "precondition: add_class(\"glow\") succeeds");

  bool removed = remove_class(doc, "el", "glow");
  check(removed, "remove_class(\"el\", \"glow\") present: returns true");
  check(!el_raw->has_class("glow"), "after remove_class: has_class(\"glow\") is false");

  bool removed_again = remove_class(doc, "el", "glow");
  check(removed_again,
        "remove_class(\"el\", \"glow\") AGAIN (already absent): STILL returns true (idempotent)");
  check(!el_raw->has_class("glow"), "after repeat remove_class: still false, no crash");
  check(el_raw->classes().empty(), "after repeat remove_class: class set stays empty");

  check(!remove_class(doc, "el", ""), "remove_class(\"el\", \"\"): false, empty token rejected");
  check(!remove_class(doc, "el", "two words"),
        "remove_class(\"el\", \"two words\"): false, embedded whitespace rejected");
}

} // namespace

int main() {
  test_set_text_on_empty_element_appends_and_roundtrips();
  test_set_text_overwrites_existing_sole_text_child();
  test_set_text_empty_string_on_empty_element_stays_empty();
  test_set_text_whitespace_only_on_empty_element_filtered();
  test_set_text_whitespace_on_existing_text_child_leaves_empty_residual();
  test_set_text_element_with_non_text_or_multiple_children_refused();
  test_unknown_id_returns_false_for_all_three_ops();
  test_duplicate_id_first_preorder_wins_for_all_three_ops();
  test_add_class_roundtrip_and_idempotent();
  test_add_class_rejects_invalid_cls();
  test_remove_class_roundtrip_and_idempotent();

  if (g_failures > 0) {
    std::fprintf(stderr, "dom_api_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dom_api_sanity: PASS");
  return 0;
}
