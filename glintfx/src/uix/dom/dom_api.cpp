// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S4 -- implementation. See dom_api.hpp's own header/per-function doc-comments for the
//     full scope/boundary/design-decision rationale this file holds itself to; the comments here
//     are about HOW, not WHY (the WHY already lives in the header, once, not duplicated per
//     function -- same discipline dom_tree.cpp already follows for dom_tree.hpp).
// PT: RMLX-1/S4 -- implementação. Ver os próprios doc-comments de cabeçalho/por-função do
//     dom_api.hpp pro escopo/fronteira/racional-de-decisão-de-desenho completos a que este arquivo
//     se prende; os comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma
//     vez só, não duplicado por função -- mesma disciplina que o dom_tree.cpp já segue pro
//     dom_tree.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "dom_api.hpp"

#include <algorithm>
#include <utility>

namespace glintfx::uix {

namespace {

// EN: The SAME 4-character whitespace set `Element::add_class`'s own single-token check uses
//     (space, '\t', '\n', '\r') -- re-declared here, not shared via a common header, for the exact
//     same reason dom_tree.cpp's own `is_whitespace_char` gives for its own duplication of this
//     same set: this is the ONLY other place in this codebase that needs it, and a shared-constant
//     header for one boolean predicate used twice (three times across the two translation units)
//     would be more indirection than the duplication it removes. See dom_api.hpp's own `add_class`
//     doc-comment for why THIS function -- not `Element::add_class` -- is where an invalid `cls`
//     must be caught (the id-found/already-present ambiguity `Element::add_class`'s own `false`
//     cannot resolve on its own).
// PT: O MESMO conjunto de 4 caracteres de whitespace que a própria checagem de token-único do
//     `Element::add_class` usa (espaço, '\t', '\n', '\r') -- redeclarado aqui, não compartilhado
//     via header comum, pelo EXATO mesmo motivo que o próprio `is_whitespace_char` do dom_tree.cpp
//     dá pra própria duplicação deste mesmo conjunto: este é o ÚNICO outro lugar deste codebase que
//     precisa dele, e um header de constante compartilhada pra um único predicado booleano usado
//     duas vezes (três vezes através das duas unidades de tradução) seria mais indireção do que a
//     duplicação que remove. Ver o próprio doc-comment do `add_class` no dom_api.hpp pra por que
//     ESTA função -- não o `Element::add_class` -- é onde um `cls` inválido precisa ser pego (a
//     ambiguidade achado/já-presente que o próprio `false` do `Element::add_class` não consegue
//     resolver sozinho).
constexpr bool is_whitespace_char(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// EN: `cls` is a valid single class token iff non-empty and free of the 4 whitespace characters
//     above -- see dom_api.hpp's `add_class` doc-comment, "RETURNS false when ... cls is
//     structurally invalid" paragraph.
// PT: `cls` é um token de classe único válido sse não-vazio e livre dos 4 caracteres de whitespace
//     acima -- ver o doc-comment do `add_class` no dom_api.hpp, parágrafo "RETORNA false quando ...
//     cls é estruturalmente inválido".
bool is_valid_class_token(std::string_view cls) {
  if (cls.empty()) {
    return false;
  }
  return std::none_of(cls.begin(), cls.end(), is_whitespace_char);
}

} // namespace

bool set_text(Document& doc, std::string_view id, std::string text) {
  Element* el = doc.body().find_by_id(id);
  if (el == nullptr) {
    return false;
  }

  // EN: Shape (a) -- zero existing children. See dom_api.hpp's own doc-comment, shape (a), for the
  //     full rationale, including why an empty/whitespace-only `text` does not need a special-case
  //     branch here: `append_child`'s own existence filter already produces the correct "zero
  //     children, success" outcome for that input, matching upstream exactly.
  // PT: Forma (a) -- zero filhos existentes. Ver o próprio doc-comment do dom_api.hpp, forma (a),
  //     pro racional completo, incluindo por que um `text` vazio/só-whitespace não precisa de
  //     ramo de caso especial aqui: o próprio filtro de existência do `append_child` já produz o
  //     resultado correto "zero filhos, sucesso" pra esse input, batendo exatamente com o upstream.
  if (el->child_count() == 0) {
    AppendResult result = el->append_child(std::make_unique<Text>(std::move(text)));
    return result.outcome != AppendOutcome::RejectedDepthCeiling;
  }

  // EN: Shape (b) -- exactly one existing child, and it is a Text node: mutate it in place via
  //     `Text::set_content`, preserving object identity. `children().front().get()` yields a
  //     mutable `Node*` even though `children()` itself returns a `const` reference -- see
  //     dom_api.hpp's own doc-comment, shape (b), for why that is well-defined (constness does not
  //     propagate through `unique_ptr::get()`), not a `const_cast`.
  // PT: Forma (b) -- exatamente um filho existente, e ele é um nó Text: muta ele no lugar via
  //     `Text::set_content`, preservando identidade de objeto. `children().front().get()` rende um
  //     `Node*` mutável mesmo `children()` em si retornando uma referência `const` -- ver o próprio
  //     doc-comment do dom_api.hpp, forma (b), pra por que isto é bem-definido (constância não se
  //     propaga por `unique_ptr::get()`), não um `const_cast`.
  if (el->child_count() == 1) {
    Text* only = as_text(el->children().front().get());
    if (only != nullptr) {
      only->set_content(std::move(text));
      return true;
    }
  }

  // EN: Shape (c) -- 🔴 the load-bearing gap. See dom_api.hpp's own doc-comment, shape (c), for the
  //     full argument for why this refuses rather than fakes a workaround.
  // PT: Forma (c) -- 🔴 a lacuna que carrega peso. Ver o próprio doc-comment do dom_api.hpp, forma
  //     (c), pro argumento completo de por que isto recusa em vez de forjar um contorno.
  return false;
}

bool add_class(Document& doc, std::string_view id, std::string_view cls) {
  if (!is_valid_class_token(cls)) {
    return false;
  }
  Element* el = doc.body().find_by_id(id);
  if (el == nullptr) {
    return false;
  }
  // EN: `Element::add_class`'s own bool (true = newly inserted, false = already present) is
  //     intentionally NOT forwarded -- see dom_api.hpp's own doc-comment for the full
  //     reconciliation with the facade's "true whenever applied" contract.
  // PT: O próprio booleano do `Element::add_class` (true = inserido agora, false = já presente) é
  //     intencionalmente NÃO encaminhado -- ver o próprio doc-comment do dom_api.hpp pra
  //     reconciliação completa com o contrato "true sempre que aplicado" da fachada.
  el->add_class(cls);
  return true;
}

bool remove_class(Document& doc, std::string_view id, std::string_view cls) {
  if (!is_valid_class_token(cls)) {
    return false;
  }
  Element* el = doc.body().find_by_id(id);
  if (el == nullptr) {
    return false;
  }
  // EN: Same reconciliation as add_class above -- Element::remove_class's own false-if-absent bool
  //     is intentionally not forwarded.
  // PT: Mesma reconciliação do add_class acima -- o próprio booleano
  //     false-se-ausente do Element::remove_class é intencionalmente não encaminhado.
  el->remove_class(cls);
  return true;
}

} // namespace glintfx::uix
