// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- implementation. See lexer.hpp's own header comment for the full scope/
//     boundary/hardening rationale this file holds itself to; the comments here are about HOW,
//     not WHY (the WHY already lives in the header, once, not duplicated per function).
// PT: RMLX-1/S1 -- implementação. Ver o próprio comentário de cabeçalho do lexer.hpp pro
//     escopo/fronteira/racional-de-hardening completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função).
// Copyright (c) 2026 Petrus Silva Costa
#include "lexer.hpp"

namespace glintfx::uix {

namespace {

// EN: The 4-character whitespace set uix-dom.md sections 2/6 both anchor on (space, '\t', '\n',
//     '\r') -- the SAME set RmlUi's own `StringUtilities::IsWhitespace` uses (cited in lexer.hpp
//     header). Whitespace SEPARATES tokens inside a tag (attribute-to-attribute, tag-name-to-
//     first-attribute); it is never itself tokenized there (unlike top-level text, where a run of
//     these same 4 characters IS a Text token -- whitespace-EXISTENCE filtering is explicitly
//     S2/S3's job, not this file's, see lexer.hpp header section 6(a) discussion).
// PT: O conjunto de 4 caracteres de whitespace em que as seções 2/6 do uix-dom.md se ancoram
//     (espaço, '\t', '\n', '\r') -- o MESMO conjunto que o próprio `StringUtilities::IsWhitespace`
//     do RmlUi usa (citado no cabeçalho do lexer.hpp). Whitespace SEPARA tokens dentro de uma tag
//     (atributo-pra-atributo, nome-de-tag-pro-primeiro-atributo); nunca é ele mesmo tokenizado lá
//     (diferente do texto de topo-de-nível, onde um trecho destes mesmos 4 caracteres É um token
//     Text -- filtragem de EXISTÊNCIA de whitespace é explicitamente trabalho da S2/S3, não deste
//     arquivo, ver a discussão da seção 6(a) do cabeçalho do lexer.hpp).
constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// EN: Tag/attribute name grammar: [A-Za-z_][A-Za-z0-9_-]* -- ASCII letters, underscore, and (only
//     after the first character) digits and hyphens. Matches every real tag/attribute name in the
//     corpus (`div`, `data-if`, `dp_ratio`, `h1`..`h6`, `units_per_em`, ...) and deliberately
//     excludes ':' (namespaced names, zero corpus occurrences -- see lexer.hpp header).
// PT: Gramática de nome de tag/atributo: [A-Za-z_][A-Za-z0-9_-]* -- letras ASCII, underscore, e
//     (só depois do primeiro caractere) dígitos e hífens. Casa com todo nome real de tag/atributo
//     do corpus (`div`, `data-if`, `dp_ratio`, `h1`..`h6`, `units_per_em`, ...) e exclui
//     deliberadamente ':' (nomes com namespace, zero ocorrências no corpus -- ver cabeçalho do
//     lexer.hpp).
constexpr bool is_name_start(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

constexpr bool is_name_char(char c) {
  return is_name_start(c) || (c >= '0' && c <= '9') || c == '-';
}

// EN: Bounds-safe literal match at a specific offset -- never reads past `s.size()`. Used to
//     recognise the 3 multi-character delimiters this grammar has: "<!--", "</", "-->".
// PT: Casamento de literal com limites seguros num offset específico -- nunca lê além de
//     `s.size()`. Usado pra reconhecer os 3 delimitadores multi-caractere que esta gramática tem:
//     "<!--", "</", "-->".
constexpr bool starts_with_at(std::string_view s, std::size_t at, std::string_view lit) {
  if (at > s.size() || lit.size() > s.size() - at) {
    return false;
  }
  return s.substr(at, lit.size()) == lit;
}

} // namespace

Lexer::Lexer(std::string_view source) : source_(source) {
  if (source_.size() > kMaxInputBytes) {
    state_ = State::Done;
    sticky_ = Token{TokenKind::Error,
                    "input exceeds kMaxInputBytes (1 MiB) ceiling -- see lexer.hpp header, "
                    "'Hardening' paragraph",
                    {},
                    0,
                    source_.size()};
  }
}

Token Lexer::next() {
  if (state_ == State::Done) {
    return sticky_;
  }
  if (state_ == State::InTag) {
    return scan_in_tag();
  }

  // state_ == State::Text
  if (pos_ >= source_.size()) {
    state_ = State::Done;
    sticky_ = Token{TokenKind::EndOfFile, {}, {}, pos_, 0};
    return sticky_;
  }

  if (source_[pos_] != '<') {
    return scan_text();
  }

  if (starts_with_at(source_, pos_, "<!--")) {
    return scan_comment();
  }
  if (starts_with_at(source_, pos_, "</")) {
    return scan_tag_close();
  }
  if (pos_ + 1 < source_.size() && is_name_start(source_[pos_ + 1])) {
    return scan_tag_open_start();
  }

  // EN: '<' followed by anything else (whitespace, digit, '!' not "<!--", '?', end of input right
  //     after '<', ...) is out of this grammar entirely -- see lexer.hpp header's "Deliberately
  //     NOT this module's job" list (DOCTYPE/XML-decl/CDATA all land here) and
  //     lexer_hardening_sanity.cpp's `test_malformed_inputs_reject` for the exact cases proven.
  // PT: '<' seguido de qualquer outra coisa (whitespace, dígito, '!' que não "<!--", '?', fim de
  //     input logo após '<', ...) está inteiramente fora desta gramática -- ver a lista
  //     "Deliberadamente NÃO é trabalho deste módulo" do cabeçalho do lexer.hpp (DOCTYPE/decl-
  //     XML/CDATA todos caem aqui) e o `test_malformed_inputs_reject` do
  //     lexer_hardening_sanity.cpp pros casos exatos provados.
  return make_error(pos_,
                    "malformed '<': expected a tag name, '/' (closing tag), or '!--' (comment)");
}

Token Lexer::make_error(std::size_t offset, std::string_view message) {
  state_ = State::Done;
  const std::size_t length = (pos_ >= offset) ? (pos_ - offset) : 0;
  sticky_ = Token{TokenKind::Error, message, {}, offset, length};
  return sticky_;
}

Token Lexer::scan_text() {
  const std::size_t start = pos_;
  while (pos_ < source_.size() && source_[pos_] != '<') {
    ++pos_;
    if (pos_ - start > kMaxTokenBytes) {
      return make_error(start,
                        "text run exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }
  const std::size_t len = pos_ - start;
  return Token{TokenKind::Text, source_.substr(start, len), {}, start, len};
}

Token Lexer::scan_comment() {
  const std::size_t start = pos_;
  pos_ += 4; // consume "<!--"
  const std::size_t content_start = pos_;

  while (!starts_with_at(source_, pos_, "-->")) {
    if (pos_ >= source_.size()) {
      return make_error(start, "unterminated comment: no matching '-->' before end of input");
    }
    ++pos_;
    if (pos_ - content_start > kMaxTokenBytes) {
      return make_error(start,
                        "comment body exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }

  const std::string_view content = source_.substr(content_start, pos_ - content_start);
  pos_ += 3; // consume "-->"
  return Token{TokenKind::Comment, content, {}, start, pos_ - start};
}

Token Lexer::scan_tag_open_start() {
  const std::size_t start = pos_;
  ++pos_; // consume '<'
  const std::size_t name_start = pos_;
  while (pos_ < source_.size() && is_name_char(source_[pos_])) {
    ++pos_;
    if (pos_ - name_start > kMaxTokenBytes) {
      return make_error(start,
                        "tag name exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }
  const std::string_view name = source_.substr(name_start, pos_ - name_start);
  state_ = State::InTag;
  return Token{TokenKind::TagOpenStart, name, {}, start, pos_ - start};
}

Token Lexer::scan_tag_close() {
  const std::size_t start = pos_;
  pos_ += 2; // consume "</"

  if (pos_ >= source_.size() || !is_name_start(source_[pos_])) {
    return make_error(start, "malformed closing tag: missing name after '</'");
  }
  const std::size_t name_start = pos_;
  while (pos_ < source_.size() && is_name_char(source_[pos_])) {
    ++pos_;
    if (pos_ - name_start > kMaxTokenBytes) {
      return make_error(start,
                        "closing tag name exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }
  const std::string_view name = source_.substr(name_start, pos_ - name_start);

  while (pos_ < source_.size() && is_whitespace(source_[pos_])) {
    ++pos_;
  }
  if (pos_ >= source_.size() || source_[pos_] != '>') {
    return make_error(start, "malformed closing tag: expected '>' after name");
  }
  ++pos_; // consume '>'

  return Token{TokenKind::TagClose, name, {}, start, pos_ - start};
}

Token Lexer::scan_in_tag() {
  while (pos_ < source_.size() && is_whitespace(source_[pos_])) {
    ++pos_;
  }
  if (pos_ >= source_.size()) {
    return make_error(pos_, "unterminated tag: end of input inside '<...' (missing '>' or '/>')");
  }

  if (source_[pos_] == '>') {
    const std::size_t start = pos_;
    ++pos_;
    state_ = State::Text;
    return Token{TokenKind::TagOpenEnd, {}, {}, start, 1};
  }

  if (source_[pos_] == '/') {
    if (starts_with_at(source_, pos_, "/>")) {
      const std::size_t start = pos_;
      pos_ += 2;
      state_ = State::Text;
      return Token{TokenKind::TagSelfClose, {}, {}, start, 2};
    }
    return make_error(pos_, "malformed tag: stray '/' not followed by '>'");
  }

  if (!is_name_start(source_[pos_])) {
    return make_error(pos_, "malformed tag: expected an attribute name, '/>' or '>'");
  }

  // EN: Attribute -- see lexer.hpp header's "Permissive, documented characteristic" paragraph:
  //     no whitespace is required before this point (i.e. immediately after a previous
  //     attribute's closing quote), only handled generically by the whitespace-skip loop above,
  //     which tolerates zero whitespace bytes just as well as one or many.
  // PT: Atributo -- ver o parágrafo "Característica permissiva e documentada" do cabeçalho do
  //     lexer.hpp: nenhum whitespace é exigido antes deste ponto (i.e. logo após a aspa de
  //     fechamento de um atributo anterior), só tratado genericamente pelo laço de pular
  //     whitespace acima, que tolera zero bytes de whitespace tão bem quanto um ou muitos.
  const std::size_t start = pos_;
  const std::size_t name_start = pos_;
  while (pos_ < source_.size() && is_name_char(source_[pos_])) {
    ++pos_;
    if (pos_ - name_start > kMaxTokenBytes) {
      return make_error(start,
                        "attribute name exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }
  const std::string_view name = source_.substr(name_start, pos_ - name_start);

  while (pos_ < source_.size() && is_whitespace(source_[pos_])) {
    ++pos_;
  }
  if (pos_ >= source_.size() || source_[pos_] != '=') {
    return make_error(start,
                      "malformed attribute: expected '=' after name (bare/boolean "
                      "attributes are rejected -- see lexer.hpp header)");
  }
  ++pos_; // consume '='

  while (pos_ < source_.size() && is_whitespace(source_[pos_])) {
    ++pos_;
  }
  if (pos_ >= source_.size() || (source_[pos_] != '"' && source_[pos_] != '\'')) {
    return make_error(start, "malformed attribute: expected a quoted value (\" or ')");
  }
  const char quote = source_[pos_];
  ++pos_; // consume opening quote
  const std::size_t value_start = pos_;

  while (pos_ < source_.size() && source_[pos_] != quote) {
    if (source_[pos_] == '<') {
      return make_error(start,
                        "malformed attribute value: literal '<' is not allowed inside a quoted "
                        "value");
    }
    ++pos_;
    if (pos_ - value_start > kMaxTokenBytes) {
      return make_error(start,
                        "attribute value exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }
  if (pos_ >= source_.size()) {
    return make_error(start, "malformed attribute: unterminated quoted value");
  }
  const std::string_view value = source_.substr(value_start, pos_ - value_start);
  ++pos_; // consume closing quote

  return Token{TokenKind::Attr, name, value, start, pos_ - start};
}

} // namespace glintfx::uix
