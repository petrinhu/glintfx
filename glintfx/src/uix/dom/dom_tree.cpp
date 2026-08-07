// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S2 -- implementation. See dom_tree.hpp's own header comment for the full scope/
//     boundary/design-decision rationale this file holds itself to; the comments here are about
//     HOW, not WHY (the WHY already lives in the header, once, not duplicated per function --
//     same discipline lexer.cpp already follows for lexer.hpp).
// PT: RMLX-1/S2 -- implementação. Ver o próprio comentário de cabeçalho do dom_tree.hpp pro
//     escopo/fronteira/racional-de-decisão-de-desenho completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função -- mesma disciplina que o lexer.cpp já segue pro lexer.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "dom_tree.hpp"

#include <algorithm>
#include <vector>

namespace glintfx::uix {

namespace {

// EN: The SAME 4-character whitespace set lexer.cpp's own `is_whitespace` uses (space, '\t', '\n',
//     '\r') -- re-declared here, not shared via a common header, because this is the ONLY other
//     place in this codebase that needs it and a shared-constant header for one boolean predicate
//     used twice would be more indirection than the duplication it removes. See dom_tree.hpp's
//     header comment, point (1), for why this predicate lives HERE (the existence filter) rather
//     than being pushed to S3.
// PT: O MESMO conjunto de 4 caracteres de whitespace que o próprio `is_whitespace` do lexer.cpp usa
//     (espaço, '\t', '\n', '\r') -- redeclarado aqui, não compartilhado via header comum, porque
//     este é o ÚNICO outro lugar deste codebase que precisa dele e um header de constante
//     compartilhada pra um único predicado booleano usado duas vezes seria mais indireção do que a
//     duplicação que remove. Ver o comentário de cabeçalho do dom_tree.hpp, ponto (1), pra por que
//     este predicado mora AQUI (o filtro de existência) em vez de ser empurrado pra S3.
constexpr bool is_whitespace_char(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// EN: `std::all_of` semantics over an EMPTY range is vacuously true -- matches
//     `Factory::InstanceElementText`'s own `only_white_space` computation exactly, including for
//     an empty string (see dom_tree.hpp header, point (1)).
// PT: A semântica do `std::all_of` sobre um range VAZIO é vacuamente verdadeira -- bate
//     exatamente com o próprio cálculo de `only_white_space` do `Factory::InstanceElementText`,
//     inclusive pra uma string vazia (ver o cabeçalho do dom_tree.hpp, ponto (1)).
bool is_whitespace_only(std::string_view s) {
  return std::all_of(s.begin(), s.end(), is_whitespace_char);
}

} // namespace

Node::Node(NodeKind kind) : kind_(kind) {}

NodeKind Node::kind() const {
  return kind_;
}

Element* Node::parent() {
  return parent_;
}

const Element* Node::parent() const {
  return parent_;
}

std::size_t Node::depth() const {
  return depth_;
}

Text::Text(std::string content) : Node(NodeKind::Text), content_(std::move(content)) {}

std::string_view Text::content() const {
  return content_;
}

void Text::set_content(std::string new_content) {
  content_ = std::move(new_content);
}

Element::Element(std::string tag) : Node(NodeKind::Element), tag_(std::move(tag)) {}

std::string_view Element::tag() const {
  return tag_;
}

std::string_view Element::id() const {
  return id_;
}

bool Element::has_id() const {
  return !id_.empty();
}

void Element::set_id(std::string new_id) {
  id_ = std::move(new_id);
}

bool Element::has_class(std::string_view cls) const {
  return classes_.find(cls) != classes_.end();
}

bool Element::add_class(std::string_view cls) {
  // EN: UIX-CLASS-SPLIT-2 (RMLX-1, 2026-08-05) -- see dom_tree.hpp's own doc-comment on
  //     `add_class` for the full derivation of the 3 rejection cases and why an EMBEDDED
  //     `\t`/`\n`/`\r` is now accepted (space is the sole RmlUi delimiter; the old any-of-4-chars
  //     check rejected a legitimate RmlUi class token this tree has no business rejecting).
  // PT: UIX-CLASS-SPLIT-2 (RMLX-1, 2026-08-05) -- ver o próprio doc-comment do `add_class` no
  //     dom_tree.hpp pra derivação completa dos 3 casos de rejeição e por que um `\t`/`\n`/`\r`
  //     EMBUTIDO agora é aceito (espaço é o único delimitador do RmlUi; a checagem antiga
  //     any-of-4-chars rejeitava um token de classe legítimo do RmlUi que esta árvore não tinha
  //     motivo pra rejeitar).
  if (cls.empty()) {
    return false; // (1) empty token
  }
  if (cls.find(' ') != std::string_view::npos) {
    return false; // (2) literal space anywhere -- space is the delimiter, this is a caller bug
  }
  if (std::all_of(cls.begin(), cls.end(), is_whitespace_char)) {
    return false; // (3) entirely made of \t/\n/\r (no space, per (2), and nothing else) -- never
                  // a real token upstream's ExpandString captures, see dom_tree.hpp header
  }
  return classes_.emplace(cls).second; // false if cls was already present -- caller-observable
                                       // dedup signal, per the test asserting a repeat add_class
                                       // is a no-op reported as such, not silently swallowed
}

bool Element::remove_class(std::string_view cls) {
  auto it = classes_.find(cls);
  if (it == classes_.end()) {
    return false;
  }
  classes_.erase(it);
  return true;
}

const std::set<std::string, std::less<>>& Element::classes() const {
  return classes_;
}

bool Element::set_attribute(std::string name, std::string value) {
  if (name == "id" || name == "class") {
    return false; // reserved -- see dom_tree.hpp header on why exactly one storage location exists
  }
  attributes_[std::move(name)] = std::move(value);
  return true;
}

bool Element::has_attribute(std::string_view name) const {
  return attributes_.find(name) != attributes_.end();
}

std::optional<std::string_view> Element::attribute(std::string_view name) const {
  auto it = attributes_.find(name);
  if (it == attributes_.end()) {
    return std::nullopt;
  }
  return std::string_view(it->second);
}

bool Element::remove_attribute(std::string_view name) {
  auto it = attributes_.find(name);
  if (it == attributes_.end()) {
    return false;
  }
  attributes_.erase(it);
  return true;
}

const std::map<std::string, std::string, std::less<>>& Element::attributes() const {
  return attributes_;
}

AppendResult Element::append_child(std::unique_ptr<Node> child) {
  // EN: (1) Existence filter FIRST -- routine, per-spec, cheap to decide, not an error (see
  //     dom_tree.hpp header, point (1)).
  // PT: (1) Filtro de existência PRIMEIRO -- rotina, conforme spec, barato de decidir, não é erro
  //     (ver o cabeçalho do dom_tree.hpp, ponto (1)).
  if (child->kind() == NodeKind::Text) {
    const auto& text = static_cast<const Text&>(*child);
    if (is_whitespace_only(text.content())) {
      return AppendResult{nullptr, AppendOutcome::FilteredWhitespaceText};
      // child destroyed here -- unique_ptr parameter goes out of scope.
    }
  }

  // EN: (2) Hardening: depth ceiling, fail-high (see dom_tree.hpp header, "Hardening" paragraph).
  // PT: (2) Hardening: teto de profundidade, fail-high (ver o cabeçalho do dom_tree.hpp,
  //     parágrafo "Hardening").
  if (depth_ + 1 > kMaxElementDepth) {
    return AppendResult{nullptr, AppendOutcome::RejectedDepthCeiling};
  }

  child->parent_ = this;
  child->depth_ = depth_ + 1;
  children_.push_back(std::move(child));
  Node* raw = children_.back().get();
  return AppendResult{raw, AppendOutcome::Appended};
}

// EN: UIX-REMOVE-CHILD -- see dom_tree.hpp's own doc-comment for the full contract/rationale.
// PT: UIX-REMOVE-CHILD -- ver o próprio doc-comment do dom_tree.hpp pro contrato/racional
//     completos.
bool Element::remove_child(const Node* child) {
  if (child == nullptr) {
    return false;
  }
  for (auto it = children_.begin(); it != children_.end(); ++it) {
    if (it->get() == child) {
      children_.erase(it); // destroys child (and, recursively, its own descendants)
      return true;
    }
  }
  return false;
}

std::size_t Element::clear_children() {
  const std::size_t removed = children_.size();
  children_.clear(); // destroys every child (and, recursively, their own descendants)
  return removed;
}

const std::vector<std::unique_ptr<Node>>& Element::children() const {
  return children_;
}

std::size_t Element::child_count() const {
  return children_.size();
}

const Element* Element::find_by_id(std::string_view target) const {
  if (target.empty()) {
    return nullptr;
  }

  // EN: Explicit stack, not recursion -- see dom_tree.hpp header, point (4), for why this stays
  //     iterative even though it would be trivially safe recursively at kMaxElementDepth's
  //     ceiling. Children pushed in REVERSE order so popping (LIFO) visits them in SOURCE order --
  //     i.e. a true pre-order DFS, self first.
  // PT: Pilha explícita, não recursão -- ver o cabeçalho do dom_tree.hpp, ponto (4), pra por que
  //     isto continua iterativo mesmo sendo trivialmente seguro de forma recursiva até o teto do
  //     kMaxElementDepth. Filhos empilhados em ordem REVERSA pra desempilhar (LIFO) visitá-los na
  //     ordem-FONTE -- ou seja, uma DFS pré-ordem de verdade, o próprio nó primeiro.
  std::vector<const Element*> stack{this};
  while (!stack.empty()) {
    const Element* cur = stack.back();
    stack.pop_back();
    if (cur->id() == target) {
      return cur;
    }
    for (auto it = cur->children_.rbegin(); it != cur->children_.rend(); ++it) {
      if (const Element* el = as_element(it->get())) {
        stack.push_back(el);
      }
    }
  }
  return nullptr;
}

Element* Element::find_by_id(std::string_view target) {
  // EN: Safe idiom -- `this` is genuinely non-const here, so casting away the constness WE added
  //     via `static_cast<const Element*>` (not constness the object was born with) is well-defined.
  // PT: Idioma seguro -- `this` é genuinamente não-const aqui, então tirar a constância que NÓS
  //     somamos via `static_cast<const Element*>` (não constância com que o objeto nasceu) é
  //     bem-definido.
  return const_cast<Element*>(static_cast<const Element*>(this)->find_by_id(target));
}

Document::Document() : body_(std::make_unique<Element>("body")) {}

Element& Document::body() {
  return *body_;
}

const Element& Document::body() const {
  return *body_;
}

const HeadContent& Document::head() const {
  return head_;
}

void Document::set_head(std::string raw) {
  head_ = HeadContent{true, std::move(raw)};
}

void Document::clear_head() {
  head_ = HeadContent{};
}

Element* as_element(Node* node) {
  return (node != nullptr && node->kind() == NodeKind::Element) ? static_cast<Element*>(node)
                                                                : nullptr;
}

const Element* as_element(const Node* node) {
  return (node != nullptr && node->kind() == NodeKind::Element)
             ? static_cast<const Element*>(node)
             : nullptr;
}

Text* as_text(Node* node) {
  return (node != nullptr && node->kind() == NodeKind::Text) ? static_cast<Text*>(node) : nullptr;
}

const Text* as_text(const Node* node) {
  return (node != nullptr && node->kind() == NodeKind::Text) ? static_cast<const Text*>(node)
                                                             : nullptr;
}

} // namespace glintfx::uix
