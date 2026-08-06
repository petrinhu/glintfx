// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-LEXER -- hardening/fail-high unit test for glintfx::uix::style::Lexer: byte
//     ceilings, error/EOF stickiness, and the deliberate divergences from upstream's own
//     leniency this module documents in lexer.hpp (never silently -- every case here traces to a
//     named header-comment paragraph). Mirrors glintfx/tests/uix/lexer_hardening_sanity.cpp's own
//     shape and "input validation em borda" discipline (this project's own convention, cited in
//     both lexer.hpp files).
// PT: UIX-RCSS-LEXER -- teste unit de hardening/fail-high pro glintfx::uix::style::Lexer: tetos de
//     byte, pegajosidade de erro/EOF, e as divergências deliberadas da própria leniência do
//     upstream que este módulo documenta no lexer.hpp (nunca em silêncio -- todo caso aqui remonta
//     a um parágrafo nomeado do comentário de cabeçalho). Espelha a própria forma e disciplina
//     "input validation em borda" do glintfx/tests/uix/lexer_hardening_sanity.cpp (convenção do
//     próprio projeto, citada nos dois lexer.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/lexer.hpp"

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

using glintfx::uix::style::kMaxInputBytes;
using glintfx::uix::style::kMaxNestDepth;
using glintfx::uix::style::kMaxTokenBytes;
using glintfx::uix::style::Lexer;
using glintfx::uix::style::Token;
using glintfx::uix::style::TokenKind;

// ---------------------------------------------------------------------------
// EN: `kMaxInputBytes` -- the whole source buffer, checked once at construction; exceeding it
//     makes the FIRST `next()` return `Error` at offset 0. Same contract as the DOM sibling's own
//     `Lexer::Lexer` (lexer.hpp header, "Hardening").
// PT: `kMaxInputBytes` -- o buffer-fonte inteiro, checado uma vez na construção; excedê-lo faz a
//     PRIMEIRA chamada de `next()` retornar `Error` no offset 0. Mesmo contrato do próprio
//     `Lexer::Lexer` do irmão DOM (comentário de cabeçalho do lexer.hpp, "Hardening").
// ---------------------------------------------------------------------------
void test_max_input_bytes_ceiling() {
  std::string huge(kMaxInputBytes + 1, 'a');
  Lexer lex(huge);
  Token tok = lex.next();
  check(tok.kind == TokenKind::Error, "max-input: exceeding kMaxInputBytes is Error");
  check(tok.offset == 0, "max-input: Error reported at offset 0");

  // EN: A single kMaxInputBytes-sized homogeneous run (e.g. all spaces) would itself exceed
  //     kMaxTokenBytes as ONE giant Prelude token -- that would test the WRONG ceiling. Build
  //     exactly kMaxInputBytes bytes out of many small, well-formed rules instead, so the input-
  //     size ceiling is exercised in isolation from the token-size one.
  // PT: Um único trecho homogêneo do tamanho de kMaxInputBytes (ex. tudo espaço) ele mesmo
  //     excederia kMaxTokenBytes como UM token de Prelude gigante -- isso testaria o teto ERRADO.
  //     Constrói exatamente kMaxInputBytes bytes a partir de muitas regras pequenas, bem-formadas,
  //     em vez disso, pra o teto de tamanho-de-input ser exercitado isolado do de tamanho-de-token.
  const std::string unit = ".a{color:red;}";
  std::string ok_size;
  ok_size.reserve(kMaxInputBytes);
  while (ok_size.size() + unit.size() <= kMaxInputBytes) {
    ok_size += unit;
  }
  while (ok_size.size() < kMaxInputBytes) {
    ok_size += '.';
  }
  check(ok_size.size() == kMaxInputBytes, "max-input: test fixture is exactly kMaxInputBytes");

  Lexer lex_ok(ok_size);
  Token tok_ok = lex_ok.next();
  check(tok_ok.kind == TokenKind::Prelude && tok_ok.kind != TokenKind::Error,
        "max-input: exactly kMaxInputBytes is still accepted (off-by-one on the right side)");
}

// ---------------------------------------------------------------------------
// EN: `kMaxTokenBytes` -- a single Prelude run and a single Declaration value are both bounded
//     independently, same "belt-and-suspenders distinct from kMaxInputBytes" reasoning as the DOM
//     sibling (lexer.hpp header, "Hardening").
// PT: `kMaxTokenBytes` -- um único trecho de Prelude e um único valor de Declaration são os dois
//     limitados de forma independente, mesmo raciocínio "belt-and-suspenders distinto de
//     kMaxInputBytes" do irmão DOM (comentário de cabeçalho do lexer.hpp, "Hardening").
// ---------------------------------------------------------------------------
void test_max_token_bytes_ceiling_prelude() {
  std::string src(kMaxTokenBytes + 16, '.');
  src += "{color:red;}";
  Lexer lex(src);
  Token tok = lex.next();
  check(tok.kind == TokenKind::Error, "max-token-prelude: oversized Prelude run is Error");
}

void test_max_token_bytes_ceiling_declaration_value() {
  std::string src = "a{color:";
  src += std::string(kMaxTokenBytes + 16, 'r');
  src += ";}";
  Lexer lex(src);
  Token prelude = lex.next();
  check(prelude.kind == TokenKind::Prelude, "max-token-value: Prelude ok before the giant value");
  Token brace = lex.next();
  check(brace.kind == TokenKind::BraceOpen, "max-token-value: BraceOpen ok");
  Token tok = lex.next();
  check(tok.kind == TokenKind::Error, "max-token-value: oversized declaration value is Error");
}

// ---------------------------------------------------------------------------
// EN: Unterminated `/* ...` (no matching `*/` before EOF) is a fail-high `Error`, same discipline
//     as the DOM sibling's own unterminated-comment case (lexer.hpp header, "Error stickiness").
// PT: `/* ...` não-terminado (nenhum `*/` correspondente antes do EOF) é um `Error` fail-high,
//     mesma disciplina do próprio caso de comentário não-terminado do irmão DOM (comentário de
//     cabeçalho do lexer.hpp, "Pegajosidade de erro").
// ---------------------------------------------------------------------------
void test_unterminated_comment_is_error() {
  // EN: No whitespace between the closing '}' and the comment start -- a leading space there
  //     would itself become its own (well-formed) Prelude token first, matching the "Prelude is
  //     byte-verbatim, comments only recognised at a fresh scan start" contract (lexer.hpp header,
  //     "Comment handling"); this test isolates the unterminated-comment case specifically.
  // PT: Nenhum whitespace entre o '}' de fechamento e o início do comentário -- um espaço
  //     ali viraria o PRÓPRIO token (bem-formado) de Prelude primeiro, casando com o contrato
  //     "Prelude é byte-verbatim, comentário só é reconhecido num início de scan fresco"
  //     (comentário de cabeçalho do lexer.hpp, "Trato de comentário"); este teste isola o caso de
  //     comentário não-terminado especificamente.
  Lexer lex("a { color: red; }/* never closed");
  lex.next(); // Prelude "a "
  lex.next(); // BraceOpen
  lex.next(); // Declaration
  lex.next(); // BraceClose
  Token err = lex.next();
  check(err.kind == TokenKind::Error, "unterminated-comment: Error, not a truncated Comment");
}

// ---------------------------------------------------------------------------
// EN: An unterminated quoted declaration value (`"` opened, never closed before EOF) is a
//     fail-high `Error` -- this module's OWN choice, STRICTER than upstream's own leniency (which
//     silently accepts a dangling last declaration at EOF, `StyleSheetParser.cpp`'s own post-loop
//     `if (state == VALUE && ...)` handling) -- see lexer.hpp header, "EOF permissiveness, and
//     where this module is stricter than upstream", for the declared, non-silent divergence.
// PT: Um valor de declaração entre aspas não-terminado (`"` aberto, nunca fechado antes do EOF) é
//     um `Error` fail-high -- escolha PRÓPRIA deste módulo, MAIS ESTRITA que a própria leniência do
//     upstream (que aceita em silêncio uma última declaração pendurada no EOF, o próprio
//     tratamento pós-laço `if (state == VALUE && ...)` do `StyleSheetParser.cpp`) -- ver o
//     cabeçalho do lexer.hpp, "Permissividade de EOF, e onde este módulo é mais estrito que o
//     upstream", pra divergência declarada, não-silenciosa.
// ---------------------------------------------------------------------------
void test_unterminated_quoted_value_is_error() {
  Lexer lex(R"(a { font-family: "never closed)");
  lex.next(); // Prelude
  lex.next(); // BraceOpen
  Token err = lex.next();
  check(err.kind == TokenKind::Error, "unterminated-quote: Error, stricter than upstream's EOF leniency");
}

// ---------------------------------------------------------------------------
// EN: A declaration name that runs off EOF without ever finding a `:` is a fail-high `Error` --
//     same "genuinely malformed/truncated input" reasoning as the quote case above.
// PT: Um nome de declaração que roda até o EOF sem nunca achar um `:` é um `Error` fail-high --
//     mesmo raciocínio "input genuinamente malformado/truncado" do caso de aspas acima.
// ---------------------------------------------------------------------------
void test_declaration_name_runs_off_eof_without_colon_is_error() {
  Lexer lex("a { color");
  lex.next(); // Prelude
  lex.next(); // BraceOpen
  Token err = lex.next();
  check(err.kind == TokenKind::Error, "name-eof: Error, no ':' ever found before EOF");
}

// ---------------------------------------------------------------------------
// EN: Error is sticky: every subsequent `next()` call returns the SAME Error (kind/text/offset),
//     never attempts to resync -- same "fail-high, not fail-plausible" reasoning as the DOM
//     sibling (lexer.hpp header, "Error stickiness").
// PT: Error é pegajoso: toda chamada subsequente de `next()` retorna o MESMO Error
//     (kind/text/offset), nunca tenta ressincronizar -- mesmo raciocínio "fail-high, não
//     fail-plausível" do irmão DOM (comentário de cabeçalho do lexer.hpp, "Pegajosidade de erro").
// ---------------------------------------------------------------------------
void test_error_is_sticky() {
  Lexer lex("a { font-family: \"never closed");
  lex.next();
  lex.next();
  Token first_err = lex.next();
  check(first_err.kind == TokenKind::Error, "sticky-error: first call is Error");
  Token second_err = lex.next();
  check(second_err.kind == TokenKind::Error && second_err.text == first_err.text &&
            second_err.offset == first_err.offset,
        "sticky-error: identical Error forever, never resyncs");
}

// ---------------------------------------------------------------------------
// EN: EndOfFile is sticky too, same contract as the DOM sibling.
// PT: EndOfFile também é pegajoso, mesmo contrato do irmão DOM.
// ---------------------------------------------------------------------------
void test_eof_is_sticky() {
  Lexer lex("");
  Token first = lex.next();
  check(first.kind == TokenKind::EndOfFile, "sticky-eof: empty source is immediately EndOfFile");
  Token second = lex.next();
  check(second.kind == TokenKind::EndOfFile, "sticky-eof: stays EndOfFile forever");
}

// ---------------------------------------------------------------------------
// EN: EOF-permissiveness for an UNCLOSED top-level block (a `{` with no matching `}` anywhere
//     before EOF) is NOT an error -- this module's own "permissive, documented characteristic",
//     mirroring the DOM sibling's own precedent for a DIFFERENT leniency (no-whitespace-between-
//     attributes) rather than inventing a new house rule -- see lexer.hpp header for the citation.
//     Contrast with test_declaration_name_runs_off_eof_without_colon_is_error above: a name/value
//     scan that genuinely never finds ITS OWN terminator is Error, but running out of input while
//     simply between well-formed declarations (no dangling name/value) is not.
// PT: Permissividade de EOF pra um bloco de topo-de-nível NÃO-FECHADO (um `{` sem `}`
//     correspondente em lugar nenhum antes do EOF) NÃO é erro -- a própria "característica
//     permissiva e documentada" deste módulo, espelhando o próprio precedente do irmão DOM pra uma
//     leniência DIFERENTE (sem-whitespace-entre-atributos) em vez de inventar uma regra da casa
//     nova -- ver o cabeçalho do lexer.hpp pra citação. Contrastar com o
//     test_declaration_name_runs_off_eof_without_colon_is_error acima: um scan de nome/valor que
//     genuinamente nunca acha O PRÓPRIO terminador é Error, mas ficar sem input estando
//     simplesmente ENTRE declarações bem-formadas (sem nome/valor pendurado) não é.
// ---------------------------------------------------------------------------
void test_unclosed_block_at_eof_is_not_an_error() {
  Lexer lex("a { color: red; ");
  lex.next(); // Prelude
  lex.next(); // BraceOpen
  Token decl = lex.next();
  check(decl.kind == TokenKind::Declaration, "unclosed-block: the one real declaration still comes through");
  Token eof = lex.next();
  check(eof.kind == TokenKind::EndOfFile,
        "unclosed-block: EOF, not Error, even though the '{' was never matched by a '}'");
}

// ---------------------------------------------------------------------------
// EN: `kMaxNestDepth` -- a hostile/pathological input abusing the ONE mechanism that can grow the
//     structural-mode stack (nested `@keyframes` inside `@keyframes` bodies, the same mode-switch
//     lexer.hpp header names as "keyframes nesting") must fail-high once the ceiling is exceeded,
//     rather than growing an unbounded stack. This is the concrete proof the ceiling exists and
//     bites; the real corpus never needs more than 2 levels (`gusworld_battle_cockpit.rml:214`).
// PT: `kMaxNestDepth` -- um input hostil/patológico abusando do ÚNICO mecanismo que consegue
//     crescer a pilha de modo estrutural (`@keyframes` aninhado dentro de corpos de `@keyframes`,
//     a mesma troca de modo que o cabeçalho do lexer.hpp nomeia "aninhamento de keyframes") precisa
//     falhar-alto assim que o teto é excedido, em vez de crescer uma pilha sem limite. Esta é a
//     prova concreta de que o teto existe e morde; o corpus real nunca precisa de mais que 2 níveis
//     (`gusworld_battle_cockpit.rml:214`).
// ---------------------------------------------------------------------------
void test_nest_depth_ceiling() {
  std::string src;
  const std::size_t levels = kMaxNestDepth + 8;
  for (std::size_t i = 0; i < levels; ++i) {
    src += "@keyframes a{";
  }
  Lexer lex(src);

  // EN: Each "@keyframes a{" repetition is 3 tokens (At, Prelude, BraceOpen), not 1 -- the
  //     iteration bound below must account for that or the loop stops well short of the depth
  //     that actually trips the ceiling.
  // PT: Cada repetição "@keyframes a{" são 3 tokens (At, Prelude, BraceOpen), não 1 -- o limite de
  //     iteração abaixo precisa contar com isso, senão o laço para bem antes da profundidade que
  //     de fato faz o teto morder.
  bool saw_error = false;
  for (std::size_t i = 0; i < (levels + 4) * 3; ++i) {
    Token tok = lex.next();
    if (tok.kind == TokenKind::Error) {
      saw_error = true;
      break;
    }
    if (tok.kind == TokenKind::EndOfFile) {
      break;
    }
  }
  check(saw_error, "nest-depth: pathological @keyframes-in-@keyframes nesting is fail-high, not unbounded");
}

// ---------------------------------------------------------------------------
// EN: The `@keyframes` special case is LOAD-BEARING, not a nicety: without it, this exact real
//     corpus line desyncs brace balance (a `{` that should open a NESTED structural region instead
//     gets misread as an ordinary declarations block, and the inner `from { .. }`/`to { .. }`
//     sub-blocks corrupt the declaration-name scan). This test locks in the CORRECT behaviour end
//     to end (same string as lexer_tokens_sanity.cpp's own `test_keyframes_two_level_nesting`,
//     duplicated here deliberately so this hardening suite has its own standalone proof that does
//     not depend on the other file, matching this repo's "prove it, don't just assert it" norm).
// PT: O caso especial de `@keyframes` é PORTANTE, não um capricho: sem ele, esta linha exata de
//     corpus real desincroniza o balanço de chaves (um `{` que deveria abrir uma região estrutural
//     ANINHADA em vez disso é lido errado como um bloco de declarações comum, e os sub-blocos
//     internos `from { .. }`/`to { .. }` corrompem o scan de nome-de-declaração). Este teste trava
//     o comportamento CORRETO ponta a ponta (mesma string do próprio
//     `test_keyframes_two_level_nesting` do lexer_tokens_sanity.cpp, duplicada aqui deliberadamente
//     pra esta suíte de hardening ter a própria prova standalone que não depende do outro arquivo,
//     casando com a norma "prove, não só afirme" deste repo).
// ---------------------------------------------------------------------------
void test_keyframes_special_case_is_load_bearing() {
  Lexer lex("@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }");
  int count = 0;
  bool saw_error = false;
  for (Token tok = lex.next(); tok.kind != TokenKind::EndOfFile; tok = lex.next()) {
    if (tok.kind == TokenKind::Error) {
      saw_error = true;
      break;
    }
    ++count;
    if (count > 64) {
      break; // defensive: would only trip on a genuine desync/hang, never on correct behaviour
    }
  }
  check(!saw_error, "keyframes-load-bearing: the real corpus line never errors");
  check(count == 13,
        "keyframes-load-bearing: At, outer Prelude, outer BraceOpen, 'from' Prelude, 'from' "
        "BraceOpen, 'from' Declaration, 'from' BraceClose, 'to' Prelude, 'to' BraceOpen, 'to' "
        "Declaration, 'to' BraceClose, trailing-space Prelude (Structural mode does not absorb "
        "whitespace before '}' the way Declaration mode does -- see "
        "lexer_tokens_sanity.cpp's own test_keyframes_two_level_nesting comment), outer "
        "BraceClose = 13 tokens before EndOfFile -- exact count proves clean brace-balance "
        "recovery, not just 'no crash'");
}

} // namespace

int main() {
  test_max_input_bytes_ceiling();
  test_max_token_bytes_ceiling_prelude();
  test_max_token_bytes_ceiling_declaration_value();
  test_unterminated_comment_is_error();
  test_unterminated_quoted_value_is_error();
  test_declaration_name_runs_off_eof_without_colon_is_error();
  test_error_is_sticky();
  test_eof_is_sticky();
  test_unclosed_block_at_eof_is_not_an_error();
  test_nest_depth_ceiling();
  test_keyframes_special_case_is_load_bearing();

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_hardening_sanity: PASS");
  return 0;
}
