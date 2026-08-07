// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-LEXER -- implementation. See lexer.hpp's own header comment for the full scope/
//     boundary/hardening rationale this file holds itself to; the comments here are about HOW,
//     not WHY (the WHY already lives in the header, once, not duplicated per function -- same
//     convention the DOM sibling's own lexer.cpp states up front).
// PT: UIX-RCSS-LEXER -- implementação. Ver o próprio comentário de cabeçalho do lexer.hpp pro
//     escopo/fronteira/racional-de-hardening completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função -- mesma convenção que o próprio lexer.cpp do irmão DOM declara de
//     saída).
// Copyright (c) 2026 Petrus Silva Costa
#include "lexer.hpp"

namespace glintfx::uix::style {

namespace {

// EN: Same 4-character whitespace set the DOM sibling anchors on (space, '\t', '\n', '\r') --
//     used ONLY to strip a Declaration's own name/value at the end of each run (matching
//     upstream's own `StringUtilities::StripWhitespace`), never to skip whitespace INSIDE a
//     Prelude run (Prelude is byte-verbatim, unstripped -- see lexer.hpp header, "Token model").
// PT: Mesmo conjunto de 4 caracteres de whitespace em que o irmão DOM se ancora (espaço, '\t',
//     '\n', '\r') -- usado SÓ pra tirar o whitespace do próprio nome/valor de uma Declaration no
//     fim de cada trecho (casando com o próprio `StringUtilities::StripWhitespace` do upstream),
//     nunca pra pular whitespace DENTRO de um trecho de Prelude (Prelude é byte-verbatim, não-
//     stripado -- ver o cabeçalho do lexer.hpp, "Modelo de token").
constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view strip_whitespace(std::string_view s) {
  std::size_t begin = 0;
  while (begin < s.size() && is_whitespace(s[begin])) {
    ++begin;
  }
  std::size_t end = s.size();
  while (end > begin && is_whitespace(s[end - 1])) {
    --end;
  }
  return s.substr(begin, end - begin);
}

// EN: Bounds-safe literal match at a specific offset -- same shape as the DOM sibling's own
//     starts_with_at, used here for the 2 multi-character delimiters this grammar's comment
//     syntax has: "/*", "*/".
// PT: Casamento de literal com limites seguros num offset específico -- mesma forma do próprio
//     starts_with_at do irmão DOM, usado aqui pros 2 delimitadores multi-caractere que a sintaxe
//     de comentário desta gramática tem: "/*", "*/".
constexpr bool starts_with_at(std::string_view s, std::size_t at, std::string_view lit) {
  if (at > s.size() || lit.size() > s.size() - at) {
    return false;
  }
  return s.substr(at, lit.size()) == lit;
}

} // namespace

Lexer::Lexer(std::string_view source, bool start_in_declaration_mode) : source_(source) {
  mode_stack_.push_back(start_in_declaration_mode ? Mode::Declaration : Mode::Structural);
  if (source_.size() > kMaxInputBytes) {
    done_ = true;
    sticky_ = Token{TokenKind::Error,
                    "input exceeds kMaxInputBytes (1 MiB) ceiling -- see lexer.hpp header, "
                    "'Hardening' paragraph",
                    {},
                    0,
                    source_.size()};
  }
}

Token Lexer::make_error(std::size_t offset, std::string_view message) {
  done_ = true;
  const std::size_t length = (pos_ >= offset) ? (pos_ - offset) : 0;
  sticky_ = Token{TokenKind::Error, message, {}, offset, length};
  return sticky_;
}

bool Lexer::try_scan_comment(Token* out) {
  if (!starts_with_at(source_, pos_, "/*")) {
    return false;
  }
  const std::size_t start = pos_;
  pos_ += 2; // consume "/*"
  const std::size_t content_start = pos_;

  while (!starts_with_at(source_, pos_, "*/")) {
    if (pos_ >= source_.size()) {
      *out = make_error(start, "unterminated comment: no matching '*/' before end of input");
      return true;
    }
    ++pos_;
    if (pos_ - content_start > kMaxTokenBytes) {
      *out = make_error(start,
                        "comment body exceeds kMaxTokenBytes (64 KiB) ceiling -- see lexer.hpp "
                        "header, 'Hardening' paragraph");
      return true;
    }
  }

  const std::string_view content = source_.substr(content_start, pos_ - content_start);
  pos_ += 2; // consume "*/"
  *out = Token{TokenKind::Comment, content, {}, start, pos_ - start};
  return true;
}

// EN: UIX-LEXER-COMENT-ESPACO -- see lexer.hpp header's own comment on this method's declaration
//     for the full rationale/history; this is HOW, not WHY (per this file's own top-of-file
//     convention). Peek past whitespace ONLY to look for a comment; if found, the whitespace is
//     never materialised into any token (folded away exactly like a stray ';' already is in
//     scan_declaration()'s own EOF-permissiveness path). If not found, roll pos_ back so the
//     caller's own subsequent scan (Prelude raw run, or Declaration NAME run) starts at EXACTLY the
//     same byte it always did -- zero behaviour change for every other token shape.
// PT: UIX-LEXER-COMENT-ESPACO -- ver o próprio comentário de cabeçalho da declaração deste método no
//     lexer.hpp pro racional/histórico completos; isto é COMO, não PORQUÊ (pela própria convenção de
//     topo-de-arquivo deste arquivo). Espia passando por whitespace SÓ pra procurar um comentário; se
//     achado, o whitespace nunca é materializado em token nenhum (dobrado embora exatamente como um
//     ';' solto já é no próprio caminho de permissividade-de-EOF do scan_declaration()). Se não
//     achado, reverte pos_ pra que o próprio scan subsequente do chamador (trecho-cru de Prelude, ou
//     trecho de NOME de Declaration) comece em EXATAMENTE o mesmo byte que sempre começou -- zero
//     mudança de comportamento pra todo outro formato de token.
bool Lexer::try_scan_comment_at_fresh_start(Token* out) {
  const std::size_t saved = pos_;
  while (pos_ < source_.size() && is_whitespace(source_[pos_])) {
    ++pos_;
  }
  if (try_scan_comment(out)) {
    return true;
  }
  pos_ = saved;
  return false;
}

Token Lexer::next() {
  if (done_) {
    return sticky_;
  }
  if (mode_stack_.back() == Mode::Structural) {
    return scan_structural();
  }
  return scan_declaration();
}

Token Lexer::scan_structural() {
  if (pos_ >= source_.size()) {
    done_ = true;
    sticky_ = Token{TokenKind::EndOfFile, {}, {}, pos_, 0};
    return sticky_;
  }

  Token comment;
  if (try_scan_comment_at_fresh_start(&comment)) {
    return comment;
  }

  const char c = source_[pos_];

  if (c == '{') {
    const std::size_t start = pos_;
    ++pos_;
    // EN: See lexer.hpp header, "At-rule mode table" -- the mode decision, consumed exactly once.
    // PT: Ver o cabeçalho do lexer.hpp, "Tabela de modo de at-rule" -- a decisão de modo,
    //     consumida exatamente uma vez.
    const Mode child = (pending_at_rule_word_ == "keyframes") ? Mode::Structural : Mode::Declaration;
    pending_at_rule_word_ = {};
    if (mode_stack_.size() >= kMaxNestDepth) {
      return make_error(start,
                        "nesting exceeds kMaxNestDepth ceiling -- see lexer.hpp header, "
                        "'Hardening' paragraph");
    }
    mode_stack_.push_back(child);
    return Token{TokenKind::BraceOpen, {}, {}, start, 1};
  }

  if (c == '}') {
    const std::size_t start = pos_;
    ++pos_;
    if (mode_stack_.size() > 1) {
      mode_stack_.pop_back();
    }
    return Token{TokenKind::BraceClose, {}, {}, start, 1};
  }

  if (c == '@') {
    const std::size_t start = pos_;
    ++pos_;
    awaiting_at_rule_word_ = true;
    return Token{TokenKind::At, {}, {}, start, 1};
  }

  // EN: Prelude run -- raw bytes up to '{'/'}'/'@'/the start of "/*". See lexer.hpp header,
  //     "Comment handling" point (1): a "/*" is only recognised at a FRESH scan start (the check
  //     above, before this run begins), never interrupting a run already in progress.
  // PT: Trecho de Prelude -- bytes crus até '{'/'}'/'@'/o início de "/*". Ver o cabeçalho do
  //     lexer.hpp, ponto (1) de "Trato de comentário": um "/*" só é reconhecido num início de scan
  //     FRESCO (o check acima, antes deste trecho começar), nunca interrompendo um trecho já em
  //     andamento.
  const std::size_t start = pos_;
  while (pos_ < source_.size() && source_[pos_] != '{' && source_[pos_] != '}' &&
         source_[pos_] != '@' && !starts_with_at(source_, pos_, "/*")) {
    ++pos_;
    if (pos_ - start > kMaxTokenBytes) {
      return make_error(start,
                        "prelude run exceeds kMaxTokenBytes (64 KiB) ceiling -- see lexer.hpp "
                        "header, 'Hardening' paragraph");
    }
  }
  const std::string_view raw = source_.substr(start, pos_ - start);

  // EN: See lexer.hpp header, "At-rule mode table" -- capture the at-rule's own first word,
  //     ONLY when this Prelude immediately follows an At token, using the RAW (unstripped) text
  //     and a literal-space-only split, matching upstream's own
  //     `pre_token_str.substr(0, pre_token_str.find(' '))` exactly.
  // PT: Ver o cabeçalho do lexer.hpp, "Tabela de modo de at-rule" -- captura a própria primeira
  //     palavra do at-rule, SÓ quando este Prelude segue imediatamente um token At, usando o
  //     texto CRU (não-stripado) e uma divisão só-por-espaço-literal, casando exatamente com o
  //     próprio `pre_token_str.substr(0, pre_token_str.find(' '))` do upstream.
  if (awaiting_at_rule_word_) {
    const std::size_t space = raw.find(' ');
    pending_at_rule_word_ = (space == std::string_view::npos) ? raw : raw.substr(0, space);
    awaiting_at_rule_word_ = false;
  }

  return Token{TokenKind::Prelude, raw, {}, start, pos_ - start};
}

Token Lexer::scan_declaration() {
  for (;;) {
    if (pos_ >= source_.size()) {
      done_ = true;
      sticky_ = Token{TokenKind::EndOfFile, {}, {}, pos_, 0};
      return sticky_;
    }

    Token comment;
    if (try_scan_comment_at_fresh_start(&comment)) {
      return comment;
    }

    if (source_[pos_] == '}') {
      const std::size_t start = pos_;
      ++pos_;
      if (mode_stack_.size() > 1) {
        mode_stack_.pop_back();
      }
      return Token{TokenKind::BraceClose, {}, {}, start, 1};
    }

    // EN: NAME run -- raw bytes up to ':'/';'/'}' -- matches upstream's own NAME state exactly:
    //     no comment-interrupt mid-run (see lexer.hpp header, "Comment handling" point (1)), no
    //     '{'-termination (a stray '{' here just becomes part of the name, same as upstream).
    // PT: Trecho de NOME -- bytes crus até ':'/';'/'}' -- casa com o próprio estado NAME do
    //     upstream exatamente: sem interrupção-por-comentário no meio do trecho (ver o cabeçalho
    //     do lexer.hpp, ponto (1) de "Trato de comentário"), sem terminação-por-'{' (um '{' solto
    //     aqui só vira parte do nome, igual ao upstream).
    const std::size_t decl_start = pos_;
    const std::size_t name_start = pos_;
    while (pos_ < source_.size() && source_[pos_] != ':' && source_[pos_] != ';' &&
           source_[pos_] != '}') {
      ++pos_;
      if (pos_ - name_start > kMaxTokenBytes) {
        return make_error(decl_start,
                          "declaration name run exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                          "lexer.hpp header, 'Hardening' paragraph");
      }
    }

    if (pos_ >= source_.size()) {
      // EN: See lexer.hpp header, "EOF permissiveness", divergence (b) -- REFINED here: a
      //     genuinely PARTIAL identifier hanging at EOF (non-empty after stripping) is a
      //     fail-high Error, stricter than upstream. But incidental TRAILING WHITESPACE with
      //     nothing after it (empty after stripping -- e.g. a file that simply ends right after
      //     the last declaration's ';') is NOT an attempted declaration at all -- treated as clean
      //     EOF, matching divergence (a)'s own "the corpus never needs the rejection" reasoning
      //     for a DIFFERENT leniency, extended here. This mirrors upstream's own post-loop
      //     `else if (!StripWhitespace(name).empty() || !value.empty())` gate exactly: an
      //     all-whitespace dangling name produces NO warning in upstream either.
      // PT: Ver o cabeçalho do lexer.hpp, "Permissividade de EOF", divergência (b) -- REFINADO
      //     aqui: um identificador genuinamente PARCIAL pendurado no EOF (não-vazio depois de
      //     stripar) é um Error fail-high, mais estrito que o upstream. Mas whitespace FINAL
      //     incidental sem nada depois (vazio depois de stripar -- ex. um arquivo que
      //     simplesmente termina logo após o ';' da última declaração) NÃO é uma declaração
      //     tentada nenhuma -- tratado como EOF limpo, casando com o próprio raciocínio "o corpus
      //     nunca precisa da rejeição" da divergência (a) pra uma leniência DIFERENTE, estendido
      //     aqui. Isto espelha exatamente o próprio portão pós-laço
      //     `else if (!StripWhitespace(name).empty() || !value.empty())` do upstream: um nome
      //     pendurado só-whitespace também não produz aviso nenhum no upstream.
      if (strip_whitespace(source_.substr(name_start, pos_ - name_start)).empty()) {
        done_ = true;
        sticky_ = Token{TokenKind::EndOfFile, {}, {}, pos_, 0};
        return sticky_;
      }
      return make_error(decl_start,
                        "unterminated declaration: reached end of input scanning a property name "
                        "(no ':' found) -- see lexer.hpp header, 'EOF permissiveness'");
    }

    if (source_[pos_] == '}') {
      // EN: Matches upstream's own "End of rule encountered while parsing property declaration"
      //     path: discard the partial name silently (zero logging, same precedent the DOM
      //     sibling's own dom_tree.cpp already established for this repo), do NOT consume the
      //     '}' here -- let it be re-dispatched as its own ordinary BraceClose token on the very
      //     next next() call, keeping "a Declaration token never swallows a brace" uniform (see
      //     lexer.hpp header's own reasoning against upstream's byte-swallowing quirk).
      // PT: Casa com o próprio caminho "End of rule encountered while parsing property
      //     declaration" do upstream: descarta o nome parcial em silêncio (zero logging, mesmo
      //     precedente que o próprio dom_tree.cpp do irmão DOM já estabeleceu pra este repo), NÃO
      //     consome o '}' aqui -- deixa ele ser re-despachado como o próprio token BraceClose
      //     comum na PRÓXIMA chamada de next(), mantendo "uma Declaration nunca engole uma chave"
      //     uniforme (ver o próprio raciocínio do cabeçalho do lexer.hpp contra a peculiaridade de
      //     engolir-byte do upstream).
      continue;
    }

    if (source_[pos_] == ';') {
      // EN: Matches upstream's own NAME-state ';' handling: an empty (whitespace-only) name is a
      //     silently-skipped empty declaration; a NON-empty name with no ':' ever found is ALSO
      //     discarded (upstream logs "Found name with no value"; this module has no logger, same
      //     zero-logging precedent cited above). Either way: consume the ';', loop for the next
      //     declaration, never emit a token for this one.
      // PT: Casa com o próprio tratamento de ';' do estado NAME do upstream: um nome vazio (só
      //     whitespace) é uma declaração vazia pulada em silêncio; um nome NÃO-vazio sem nenhum
      //     ':' nunca achado TAMBÉM é descartado (o upstream loga "Found name with no value";
      //     este módulo não tem logger, mesmo precedente de zero-logging citado acima). Dos dois
      //     jeitos: consome o ';', itera pra próxima declaração, nunca emite token pra esta.
      ++pos_;
      continue;
    }

    // EN: source_[pos_] == ':' -- name captured, strip whitespace (matches upstream's own
    //     StripWhitespace), proceed to the VALUE run.
    // PT: source_[pos_] == ':' -- nome capturado, tira whitespace (casa com o próprio
    //     StripWhitespace do upstream), segue pro trecho de VALUE.
    const std::string_view name = strip_whitespace(source_.substr(name_start, pos_ - name_start));
    ++pos_; // consume ':'

    const std::size_t value_start = pos_;
    bool in_quote = false;
    while (pos_ < source_.size()) {
      const char vc = source_[pos_];
      if (in_quote) {
        if (vc == '"') {
          in_quote = false;
        }
        ++pos_;
      } else {
        if (vc == '"') {
          in_quote = true;
          ++pos_;
        } else if (vc == ';' || vc == '}') {
          break;
        } else {
          ++pos_;
        }
      }
      if (pos_ - value_start > kMaxTokenBytes) {
        return make_error(decl_start,
                          "declaration value run exceeds kMaxTokenBytes (64 KiB) ceiling -- see "
                          "lexer.hpp header, 'Hardening' paragraph");
      }
    }

    if (pos_ >= source_.size()) {
      // EN: See lexer.hpp header, "EOF permissiveness", divergence (b) -- ran off the end still
      //     inside a quote, OR never reached ';'/'}' at all: both are fail-high Error, stricter
      //     than upstream's own dangling-last-declaration leniency.
      // PT: Ver o cabeçalho do lexer.hpp, "Permissividade de EOF", divergência (b) -- rodou até o
      //     fim ainda dentro de uma aspa, OU nunca chegou em ';'/'}': os dois são Error fail-high,
      //     mais estrito que a própria leniência de última-declaração-pendurada do upstream.
      return make_error(decl_start,
                        in_quote ? "unterminated declaration: quoted value never closed before "
                                   "end of input -- see lexer.hpp header, 'EOF permissiveness'"
                                 : "unterminated declaration: value never reached ';' or '}' "
                                   "before end of input -- see lexer.hpp header, "
                                   "'EOF permissiveness'");
    }

    const std::string_view value = strip_whitespace(source_.substr(value_start, pos_ - value_start));

    if (source_[pos_] == ';') {
      ++pos_; // consume ';' -- folded into this Declaration token's own span, never its own token
    }
    // EN: source_[pos_] == '}' here is deliberately NOT consumed -- see the '}'-mid-NAME comment
    //     above for the "a Declaration token never swallows a brace" reasoning, which applies
    //     identically to a value terminated by '}'. `decl_len` is computed AFTER this decision so
    //     a consumed trailing ';' is included in the token's own span, but a not-consumed '}' is
    //     not.
    // PT: source_[pos_] == '}' aqui é deliberadamente NÃO consumido -- ver o comentário de '}'
    //     no-meio-do-NOME acima pro raciocínio "uma Declaration nunca engole uma chave", que se
    //     aplica identicamente a um valor terminado por '}'. `decl_len` é computado DEPOIS desta
    //     decisão pra um ';' final consumido entrar no próprio trecho do token, mas um '}'
    //     não-consumido não.
    const std::size_t decl_len = pos_ - decl_start;

    return Token{TokenKind::Declaration, name, value, decl_start, decl_len};
  }
}

} // namespace glintfx::uix::style
