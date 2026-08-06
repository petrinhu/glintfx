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

// EN: ASCII-only lowercase fold -- see lexer.hpp header, "RESOLVED (UIX-LEXER-OPACO)" paragraph,
//     bullet on case-folding: mirrors upstream's `StringUtilities::ToLower` (before its
//     `IsCDATATag` lookup) and this repo's own `parser.cpp`'s `to_ascii_lower` (used for "head"/
//     "body" tag-name comparisons) -- same fold, independent copy, same reasoning parser.cpp's own
//     header gives for why it is redeclared per-TU rather than shared via a common header.
// PT: Dobra minúscula só-ASCII -- ver o cabeçalho do lexer.hpp, parágrafo "RESOLVED
//     (UIX-LEXER-OPACO)", item de dobra-de-caixa: espelha o `StringUtilities::ToLower` do upstream
//     (antes do lookup `IsCDATATag`) e o `to_ascii_lower` do PRÓPRIO `parser.cpp` deste repo
//     (usado pras comparações de nome-de-tag "head"/"body") -- mesma dobra, cópia independente,
//     mesmo raciocínio que o próprio cabeçalho do parser.cpp dá pro motivo dele ser redeclarado
//     por-TU em vez de compartilhado via header comum.
constexpr char to_lower_ascii(char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// EN: ASCII case-insensitive equality -- used ONLY to decide whether a just-closed opening tag's
//     name is "style"/"script" (see cdata_kind_for below). `a`/`b` are both already bounded views
//     (tag names are already length-capped by kMaxTokenBytes via is_name_char's own scan loop), so
//     no additional bounds risk here.
// PT: Igualdade insensível-a-caixa só-ASCII -- usada SÓ pra decidir se o nome de uma tag de
//     abertura recém-fechada é "style"/"script" (ver cdata_kind_for abaixo). `a`/`b` são as duas
//     views já limitadas (nomes de tag já são limitados em tamanho por kMaxTokenBytes via o
//     próprio laço de scan do is_name_char), então nenhum risco adicional de limite aqui.
constexpr bool ascii_ieq(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (to_lower_ascii(a[i]) != to_lower_ascii(b[i])) {
      return false;
    }
  }
  return true;
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
  if (state_ == State::RawCData) {
    return scan_raw_cdata();
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
  // EN: Remembered for scan_in_tag()'s TagOpenEnd branch -- see lexer.hpp header, "RESOLVED
  //     (UIX-LEXER-OPACO)" paragraph. Zero-copy (a view into source_, same as `name` above); safe
  //     to overwrite on every TagOpenStart because it is only ever read back by the VERY NEXT
  //     TagOpenEnd/TagSelfClose, before another TagOpenStart could occur (this lexer has no
  //     concept of nesting/a stack -- S2/S3's job, see this header's own "Deliberately NOT this
  //     module's job" list -- so there is exactly one "most recently opened tag name" at a time
  //     from this lexer's own point of view).
  // PT: Lembrado pro ramo TagOpenEnd do scan_in_tag() -- ver o cabeçalho do lexer.hpp, parágrafo
  //     "RESOLVED (UIX-LEXER-OPACO)". Zero-cópia (uma view sobre source_, igual ao `name` acima);
  //     seguro sobrescrever a cada TagOpenStart porque só é lido de volta pelo PRÓXIMO
  //     TagOpenEnd/TagSelfClose imediato, antes de outro TagOpenStart poder ocorrer (este lexer não
  //     tem conceito nenhum de aninhamento/pilha -- trabalho da S2/S3, ver a própria lista
  //     "Deliberadamente NÃO é trabalho deste módulo" deste cabeçalho -- então existe exatamente um
  //     "nome de tag mais recentemente aberto" por vez do próprio ponto de vista deste lexer).
  pending_tag_name_ = name;
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
    // EN: See lexer.hpp header, "RESOLVED (UIX-LEXER-OPACO)" paragraph -- the mode switch. A
    //     TagOpenEnd closing a <style>/<script> opening tag hands the NEXT next() call to
    //     scan_raw_cdata() instead of ordinary State::Text scanning; every other tag name (the
    //     overwhelming majority) is completely unaffected, same State::Text transition as before
    //     this fix.
    // PT: Ver o cabeçalho do lexer.hpp, parágrafo "RESOLVED (UIX-LEXER-OPACO)" -- a troca de modo.
    //     Um TagOpenEnd fechando uma tag de abertura <style>/<script> entrega a PRÓXIMA chamada de
    //     next() ao scan_raw_cdata() em vez do scan comum de State::Text; todo outro nome de tag (a
    //     imensa maioria) fica completamente inalterado, mesma transição State::Text de antes deste
    //     conserto.
    if (ascii_ieq(pending_tag_name_, "style")) {
      raw_kind_ = CDataKind::Style;
      state_ = State::RawCData;
    } else if (ascii_ieq(pending_tag_name_, "script")) {
      raw_kind_ = CDataKind::Script;
      state_ = State::RawCData;
    } else {
      state_ = State::Text;
    }
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

// EN: See lexer.hpp header, "RESOLVED (UIX-LEXER-OPACO)" paragraph. Only ever entered right after
//     a TagOpenEnd closed a <style>/<script> opening tag (state_ == State::RawCData, raw_kind_ !=
//     CDataKind::None). Scans forward byte-wise, same shape as scan_comment()'s own
//     `while (!starts_with_at(...))` loop, treating every byte as literal content until it finds
//     the case-folded literal closer -- NOT a tag-name-verified re-scan, see this function's own
//     "declared scope teto" citation in lexer.hpp for why that simplification was chosen
//     (converged with dom_dump.cpp's locate_head_tag_span(), not re-derived).
// PT: Ver o cabeçalho do lexer.hpp, parágrafo "RESOLVED (UIX-LEXER-OPACO)". Só é entrado logo após
//     um TagOpenEnd fechar uma tag de abertura <style>/<script> (state_ == State::RawCData,
//     raw_kind_ != CDataKind::None). Faz scan pra frente byte-a-byte, mesma forma do próprio laço
//     `while (!starts_with_at(...))` do scan_comment(), tratando todo byte como conteúdo literal
//     até achar o fechamento literal dobrado-por-caixa -- NÃO um re-scan com verificação-de-nome,
//     ver a própria citação "teto de escopo declarado" desta função no lexer.hpp pro motivo desta
//     simplificação ter sido escolhida (convergida com o locate_head_tag_span() do dom_dump.cpp,
//     não re-derivada).
Token Lexer::scan_raw_cdata() {
  const std::size_t start = pos_;
  const std::string_view closer =
      (raw_kind_ == CDataKind::Style) ? std::string_view("</style") : std::string_view("</script");

  auto starts_with_closer_ci = [&](std::size_t at) {
    if (at > source_.size() || closer.size() > source_.size() - at) {
      return false;
    }
    for (std::size_t i = 0; i < closer.size(); ++i) {
      if (to_lower_ascii(source_[at + i]) != closer[i]) { // closer's own literal is already lowercase
        return false;
      }
    }
    return true;
  };

  while (!starts_with_closer_ci(pos_)) {
    if (pos_ >= source_.size()) {
      return make_error(start,
                        "unterminated <style>/<script> content: no matching closing tag before "
                        "end of input -- see lexer.hpp header, 'RESOLVED (UIX-LEXER-OPACO)' "
                        "paragraph");
    }
    ++pos_;
    if (pos_ - start > kMaxTokenBytes) {
      return make_error(start,
                        "<style>/<script> content exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                        "lexer.hpp header, 'Hardening' paragraph");
    }
  }

  // EN: Found the closer. Hand control BACK to the ordinary tokenizer BEFORE deciding what to
  //     return -- state_/raw_kind_ must already reflect "done with raw mode" whether this call
  //     emits a Text token itself or (the zero-length case, `<style></style>`) recurses into
  //     next() to let the ordinary State::Text dispatch produce the </style> TagClose token
  //     instead (mirrors how scan_text() is never called when pos_ is already at '<' -- see
  //     next()'s own dispatch -- so this function never emits a zero-length Text token either).
  // PT: Achou o fechamento. Devolve o controle AO tokenizador comum ANTES de decidir o que
  //     retornar -- state_/raw_kind_ já precisam refletir "acabou o modo cru" tanto se esta
  //     chamada emite um token Text ela mesma quanto (o caso de comprimento-zero,
  //     `<style></style>`) recursa em next() pra deixar o dispatch comum de State::Text produzir o
  //     token TagClose de </style> em vez disso (espelha como scan_text() nunca é chamado quando
  //     pos_ já está em '<' -- ver o próprio dispatch do next() -- então esta função também nunca
  //     emite um token Text de comprimento zero).
  state_ = State::Text;
  raw_kind_ = CDataKind::None;

  if (pos_ == start) {
    return next();
  }

  const std::size_t len = pos_ - start;
  return Token{TokenKind::Text, source_.substr(start, len), {}, start, len};
}

} // namespace glintfx::uix
