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

  // EN: UIX-REMOVE-CHILD -- unconditional clear FIRST, exactly mirroring `Rml::
  //     Element::SetInnerRML`'s own order (clear every existing child regardless of what was
  //     there or whether the new text is empty, THEN try to instance the new text). See
  //     dom_api.hpp's own set_text doc-comment for the full before/after this replaces (the old
  //     three-shape special-casing) and the one new RejectedDepthCeiling edge this ordering
  //     introduces (a no-risk edge -- see that doc-comment's own "⚠️" paragraph).
  // PT: UIX-REMOVE-CHILD -- clear incondicional PRIMEIRO, espelhando exatamente a própria ordem do
  //     `Rml::Element::SetInnerRML` (limpa todo filho existente independente do que havia ou de o
  //     texto novo ser vazio, DEPOIS tenta instanciar o texto novo). Ver o próprio doc-comment do
  //     set_text no dom_api.hpp pro antes/depois completo que isto substitui (a antiga divisão em
  //     três formas) e a nova borda RejectedDepthCeiling que esta ordem introduz (uma borda sem
  //     risco -- ver o próprio parágrafo "⚠️" daquele doc-comment).
  el->clear_children();

  AppendResult result = el->append_child(std::make_unique<Text>(std::move(text)));
  return result.outcome != AppendOutcome::RejectedDepthCeiling;
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
