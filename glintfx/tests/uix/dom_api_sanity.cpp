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
using glintfx::uix::kMaxElementDepth;
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
// EN: Case 2 -- set_text called a SECOND time on the same id replaces the existing sole Text
//     child's CONTENT correctly. 🟢 UIX-REMOVE-CHILD note: the object-identity claim this test
//     ORIGINALLY made ("mutated in place, not replaced") is GONE as of UIX-REMOVE-CHILD: set_text
//     now unconditionally clear_children()s before appending, so the OLD Text object is destroyed
//     and a NEW one is constructed -- see dom_api.hpp's own set_text doc-comment for the full
//     argument for why this is a safe, deliberate closure (no caller in this codebase holds a
//     Text*/Node* observer pointer across a set_text call). This test is UPDATED (not renamed
//     away) to assert what remains true (content correctness, exactly one child, still a Text
//     node) and to stop asserting what is no longer true (pointer identity).
// PT: Caso 2 -- set_text chamado uma SEGUNDA vez no mesmo id substitui o CONTEÚDO do único filho
//     Text existente corretamente. 🟢 nota UIX-REMOVE-CHILD: a afirmação de identidade de objeto
//     que este teste ORIGINALMENTE fazia ("mutado no lugar, não substituído") SUMIU a partir da
//     UIX-REMOVE-CHILD: set_text agora faz clear_children() incondicional antes de somar, então o
//     objeto Text ANTIGO é destruído e um NOVO é construído -- ver o próprio doc-comment do
//     set_text no dom_api.hpp pro argumento completo de por que isto é um fechamento seguro e
//     deliberado (nenhum chamador deste codebase segura um ponteiro observador Text*/Node* através
//     de uma chamada set_text). Este teste é ATUALIZADO (não renomeado à toa) pra afirmar o que
//     continua verdadeiro (conteúdo correto, exatamente um filho, ainda um nó Text) e pra parar de
//     afirmar o que não é mais verdadeiro (identidade de ponteiro).
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
  if (first_child != nullptr) {
    check_eq(first_child->content(), "first", "after set_text #1: content is \"first\"");
  }

  check(set_text(doc, "label", "second"), "set_text #2 (overwrite): returns true");
  check(label_raw->child_count() == 1,
        "after set_text #2: STILL exactly 1 child (no leftover sibling)");
  const Text* second_child = as_text(label_raw->children()[0].get());
  check(second_child != nullptr, "after set_text #2: sole child is still Text");
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
// EN: Case 5 -- 🟢 UIX-REMOVE-CHILD closure: `set_text(doc, id, "")` REPEATED on an element that
//     already holds a sole Text child now REVERTS to zero children, identical to calling
//     set_text(doc, id, "") on a childless element (Case 3) -- matching upstream
//     `SetInnerRML("")` exactly (clears unconditionally, then rml.empty() -> Factory never
//     called). This REPLACES `test_set_text_whitespace_on_existing_text_child_leaves_empty_
//     residual`, which pinned the OLD, divergent behaviour (an empty Text residual, child_count
//     staying 1) that dom_api.hpp's own set_text doc-comment names as now-closed -- the old test
//     is not silently deleted, it is REPLACED by this one asserting the opposite, correct outcome.
// PT: Caso 5 -- 🟢 fechamento da UIX-REMOVE-CHILD: `set_text(doc, id, "")` REPETIDO num elemento
//     que já guarda um único filho Text agora REVERTE pra zero filhos, idêntico a chamar
//     set_text(doc, id, "") num elemento sem filhos (Caso 3) -- batendo exatamente com o
//     `SetInnerRML("")` do upstream (limpa incondicionalmente, depois rml.empty() -> Factory nunca
//     chamado). Isto SUBSTITUI o
//     `test_set_text_whitespace_on_existing_text_child_leaves_empty_residual`, que fixava o
//     comportamento ANTIGO, divergente (um residual Text vazio, child_count continuando 1) que o
//     próprio doc-comment do set_text no dom_api.hpp nomeia como agora fechado -- o teste antigo
//     não é apagado em silêncio, é SUBSTITUÍDO por este, afirmando o resultado oposto, correto.
// ---------------------------------------------------------------------------
void test_set_text_repeated_empty_reverts_to_zero_children() {
  Document doc;
  Element& body = doc.body();

  auto label = std::make_unique<Element>("span");
  label->set_id("label");
  Element* label_raw = label.get();
  body.append_child(std::move(label));

  check(set_text(doc, "label", "hi"), "set_text #1 (\"hi\"): returns true");
  check(label_raw->child_count() == 1, "after set_text #1: exactly 1 child");

  bool ok = set_text(doc, "label", "");
  check(ok, "set_text #2 (\"\") over an existing Text child: returns true");
  check(label_raw->child_count() == 0,
        "🟢 UIX-REMOVE-CHILD closure: child_count REVERTS to 0, matching upstream and matching "
        "the childless-element case exactly -- no more empty-Text residual");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- 🟢 UIX-REMOVE-CHILD closure: an element whose children are NOT the "zero, or one
//     Text" shape is now WITHIN set_text's reach -- every existing child is destroyed
//     (Element::clear_children), the new text becomes the ONLY child (subject to the same
//     whitespace-existence filter every other shape already goes through). This REPLACES
//     `test_set_text_element_with_non_text_or_multiple_children_refused`, which pinned the OLD
//     refusal (false, tree untouched) that dom_api.hpp's own set_text doc-comment names as the
//     closed "load-bearing gap" -- the old test is not silently deleted, it is REPLACED by this
//     one asserting the opposite, correct (upstream-matching) outcome. Three independent
//     sub-cases: a single Element child, two Text children, and a MIX of Element and Text children
//     (the shape neither the old "exactly one Text child" shape nor a naive "sole child is
//     Element" check alone would cover).
// PT: Caso 6 -- 🟢 fechamento da UIX-REMOVE-CHILD: um elemento cujos filhos NÃO são a forma "zero,
//     ou um Text" agora está DENTRO do alcance do set_text -- todo filho existente é destruído
//     (Element::clear_children), o texto novo vira o ÚNICO filho (sujeito ao mesmo filtro de
//     existência-de-whitespace que toda outra forma já passa). Isto SUBSTITUI o
//     `test_set_text_element_with_non_text_or_multiple_children_refused`, que fixava a recusa
//     ANTIGA (false, árvore intocada) que o próprio doc-comment do set_text no dom_api.hpp nomeia
//     como a "lacuna que carrega peso" agora fechada -- o teste antigo não é apagado em silêncio, é
//     SUBSTITUÍDO por este, afirmando o resultado oposto, correto (batendo com upstream). Três
//     subcasos independentes: um único filho Element, dois filhos Text, e uma MISTURA de Element e
//     Text (a forma que nem a antiga forma "exatamente um filho Text" nem uma checagem ingênua de
//     "sole child é Element" sozinha cobririam).
// ---------------------------------------------------------------------------
void test_set_text_element_with_non_text_or_multiple_children_replaces_all() {
  // Sub-case 6a: sole child is an Element, not Text.
  {
    Document doc;
    Element& body = doc.body();

    auto panel = std::make_unique<Element>("div");
    panel->set_id("panel");
    Element* panel_raw = panel.get();
    body.append_child(std::move(panel));

    panel_raw->append_child(std::make_unique<Element>("icon"));

    bool ok = set_text(doc, "panel", "replacement");
    check(ok, "6a: set_text on an element whose sole child is an Element: now succeeds (true)");
    check(panel_raw->child_count() == 1, "6a: panel has exactly 1 child afterward (the new Text)");
    const Text* only = as_text(panel_raw->children()[0].get());
    check(only != nullptr, "6a: panel's only child is now a Text node (the old Element is gone)");
    if (only != nullptr) {
      check_eq(only->content(), "replacement", "6a: the new Text's content is \"replacement\"");
    }
  }

  // Sub-case 6b: two Text children.
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
    check(ok, "6b: set_text on an element with 2 Text children: now succeeds (true)");
    check(panel_raw->child_count() == 1,
          "6b: panel has exactly 1 child afterward (both old Text children are gone)");
    const Text* only = as_text(panel_raw->children()[0].get());
    check(only != nullptr, "6b: panel's only child is a Text node");
    if (only != nullptr) {
      check_eq(only->content(), "replacement", "6b: the new Text's content is \"replacement\"");
    }
  }

  // Sub-case 6c: a MIX of Element and Text children -- neither the old "exactly one Text child"
  // shape nor a naive "sole child is Element" check alone would have this shape in view.
  {
    Document doc;
    Element& body = doc.body();

    auto panel = std::make_unique<Element>("div");
    panel->set_id("panel");
    Element* panel_raw = panel.get();
    body.append_child(std::move(panel));

    panel_raw->append_child(std::make_unique<Text>("before"));
    panel_raw->append_child(std::make_unique<Element>("icon"));
    panel_raw->append_child(std::make_unique<Text>("after"));
    check(panel_raw->child_count() == 3, "6c precondition: panel has 3 mixed children");

    bool ok = set_text(doc, "panel", "replacement");
    check(ok, "6c: set_text on an element with mixed Element/Text children: succeeds (true)");
    check(panel_raw->child_count() == 1,
          "6c: panel has exactly 1 child afterward (all 3 old children are gone)");
    const Text* only = as_text(panel_raw->children()[0].get());
    check(only != nullptr, "6c: panel's only child is a Text node");
    if (only != nullptr) {
      check_eq(only->content(), "replacement", "6c: the new Text's content is \"replacement\"");
    }
  }
}

// ---------------------------------------------------------------------------
// EN: Case 12 -- UIX-REMOVE-CHILD: on the `RejectedDepthCeiling` case specifically. An element
//     sitting exactly AT `kMaxElementDepth` can never have successfully adopted a child in the
//     first place (the depth check `depth_ + 1 > kMaxElementDepth` is a function of the PARENT's
//     depth alone, identical for every child that parent is ever offered) -- so `clear_children()`
//     on such a target is GUARANTEED to be a no-op before the doomed append is even attempted.
//     There is no scenario where real content is destroyed and then not replaced; a
//     `RejectedDepthCeiling` target is, and always was, childless. See dom_api.hpp's own set_text
//     doc-comment for the full argument.
// PT: Caso 12 -- UIX-REMOVE-CHILD: sobre o caso `RejectedDepthCeiling` especificamente. Um
//     elemento sentado exatamente EM `kMaxElementDepth` nunca conseguiu adotar um filho, de
//     jeito nenhum (a checagem de profundidade `depth_ + 1 > kMaxElementDepth` é função só da
//     profundidade do PAI, idêntica pra todo filho que aquele pai algum dia recebeu) -- então
//     `clear_children()` num alvo desses é GARANTIDAMENTE um no-op antes mesmo do append fadado
//     ser tentado. Não existe cenário em que conteúdo real é destruído e depois não substituído;
//     um alvo `RejectedDepthCeiling` é, e sempre foi, sem filhos. Ver o próprio doc-comment do
//     set_text no dom_api.hpp pro argumento completo.
// ---------------------------------------------------------------------------
void test_set_text_depth_ceiling_rejection_on_always_childless_element() {
  Document doc;
  Element* cursor = &doc.body();
  for (std::size_t i = 2; i <= kMaxElementDepth; ++i) {
    auto child = std::make_unique<Element>("div");
    auto result = cursor->append_child(std::move(child));
    cursor = static_cast<Element*>(result.node);
  }
  check(cursor != nullptr && cursor->depth() == kMaxElementDepth,
        "precondition: deepest node built is exactly at the ceiling");
  cursor->set_id("deepest");
  check(cursor->child_count() == 0,
        "precondition: an element AT the ceiling can never have adopted a child -- starts empty");

  bool ok = set_text(doc, "deepest", "won't fit");
  check(!ok, "set_text at the depth ceiling: returns false (RejectedDepthCeiling)");
  check(cursor->child_count() == 0,
        "set_text at the depth ceiling: still zero children -- clear_children() was a guaranteed "
        "no-op, nothing real was ever destroyed");
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
  test_set_text_repeated_empty_reverts_to_zero_children();
  test_set_text_element_with_non_text_or_multiple_children_replaces_all();
  test_set_text_depth_ceiling_rejection_on_always_childless_element();
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
