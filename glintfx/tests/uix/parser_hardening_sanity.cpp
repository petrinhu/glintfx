// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S3 -- hardening/fail-high unit test for glintfx::uix::parse_document: every
//     construction this parser rejects, and that the rejection is a diagnosable `ParseError`
//     (never a crash, never a silently-wrong tree). Same discipline as
//     lexer_hardening_sanity.cpp: proving the REJECT side of a boundary, not just "it didn't
//     crash". See glintfx/src/uix/dom/parser.hpp's header comment, "Top-level document grammar",
//     for the grammar every one of these cases traces back to.
// PT: RMLX-1/S3 -- teste unit de hardening/fail-high pro glintfx::uix::parse_document: toda
//     construção que este parser rejeita, e que a rejeição é um `ParseError` diagnosticável
//     (nunca um crash, nunca uma árvore errada em silêncio). Mesma disciplina do
//     lexer_hardening_sanity.cpp: provar o lado REJEITA de uma fronteira, não só "não crashou".
//     Ver o comentário de cabeçalho do glintfx/src/uix/dom/parser.hpp, "Gramática de topo de
//     documento", pra gramática a que todo caso abaixo remonta.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"

#include <cstdio>
#include <string>
#include <string_view>

#include "uix/dom/lexer.hpp"

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;

// EN: Asserts `source` is REJECTED (a ParseError, never a document), and that the message is
//     non-empty and line/column are sane (>= 1) -- a ParseError with an empty message or a
//     0/0 position would be a diagnosable-in-name-only failure, exactly what this project's own
//     fail-high discipline warns against.
// PT: Garante que `source` é REJEITADO (um ParseError, nunca um documento), e que a mensagem é
//     não-vazia e linha/coluna são sensatas (>= 1) -- um ParseError com mensagem vazia ou posição
//     0/0 seria uma falha diagnosticável-só-de-nome, exatamente o que a própria disciplina
//     fail-high deste projeto avisa contra.
void expect_reject(std::string_view source, const char* what) {
  ParseResult r = parse_document(source);
  if (!r.error.has_value()) {
    std::fprintf(stderr, "FAIL: %s (expected ParseError, got a document instead)\n", what);
    ++g_failures;
    return;
  }
  check(r.document == nullptr, what);
  check(!r.error->message.empty(), what);
  check(r.error->line >= 1 && r.error->column >= 1, what);
}

void expect_ok(std::string_view source, const char* what) {
  ParseResult r = parse_document(source);
  if (r.error.has_value()) {
    std::fprintf(stderr, "FAIL: %s (unexpected ParseError at %zu:%zu: %s)\n", what,
                 r.error->line, r.error->column, r.error->message.c_str());
    ++g_failures;
    return;
  }
  check(r.document != nullptr, what);
}

void test_missing_rml_wrapper() {
  expect_reject("<body>hello</body>", "reject: bare <body> with no <rml> wrapper");
}

void test_missing_body() {
  expect_reject("<rml><head></head></rml>", "reject: <rml> with <head> but no <body>");
  expect_reject("<rml/>", "reject: self-closed <rml/> (no body possible)");
}

void test_duplicate_head() {
  expect_reject("<rml><head></head><head></head><body>x</body></rml>",
                "reject: duplicate <head>");
}

void test_duplicate_body() {
  expect_reject("<rml><body>a</body><body>b</body></rml>", "reject: duplicate <body>");
}

void test_head_after_body() {
  expect_reject("<rml><body>a</body><head></head></rml>", "reject: <head> after <body>");
}

void test_stray_element_at_rml_level() {
  expect_reject("<rml><meta/><body>x</body></rml>",
                "reject: unexpected element directly inside <rml> (not head/body)");
}

void test_nonwhitespace_text_at_rml_level() {
  expect_reject("<rml>stray<body>x</body></rml>",
                "reject: non-whitespace text directly inside <rml>");
}

void test_trailing_content_after_rml() {
  expect_reject("<rml><body>x</body></rml>stray",
                "reject: trailing non-whitespace content after </rml>");
}

void test_head_nested_inside_body() {
  expect_reject("<rml><body><head></head></body></rml>",
                "reject: <head> is only valid as a direct child of <rml>, not nested in <body>");
}

void test_mismatched_close_tag() {
  expect_reject("<rml><body><div></span></body></rml>",
                "reject: mismatched closing tag name (parser-level, lexer accepts this token)");
}

void test_unclosed_element_eof() {
  expect_reject("<rml><body><div><span>x", "reject: unclosed elements, EOF mid-document");
  expect_reject("<rml><body>", "reject: EOF right after <body>'s TagOpenEnd, no content/close");
  expect_reject("<rml><head>", "reject: EOF inside an unterminated <head> (no </head> anywhere)");
}

void test_lexer_error_propagates() {
  // EN: DOCTYPE -- lexer rejects '<!' not followed by '--' (lexer.hpp's own "Deliberately NOT
  //     this module's job" list; zero corpus occurrences).
  // PT: DOCTYPE -- o lexer rejeita '<!' não seguido de '--' (a própria lista "Deliberadamente NÃO
  //     é trabalho deste módulo" do lexer.hpp; zero ocorrências no corpus).
  expect_reject("<rml><body><!DOCTYPE html></body></rml>", "reject: DOCTYPE (lexer Error propagated)");
  // EN: XML declaration -- '<?' matches nothing in this grammar.
  // PT: Declaração XML -- '<?' não casa com nada nesta gramática.
  expect_reject("<?xml version=\"1.0\"?><rml><body>x</body></rml>",
                "reject: XML declaration (lexer Error propagated)");
  // EN: CDATA -- '<![CDATA[' is '<!' not followed by '--'.
  // PT: CDATA -- '<![CDATA[' é '<!' não seguido de '--'.
  expect_reject("<rml><body><![CDATA[x]]></body></rml>", "reject: CDATA (lexer Error propagated)");
  // EN: Namespaced attribute name -- ':' is not in this grammar's name-char class.
  // PT: Nome de atributo com namespace -- ':' não está na classe de name-char desta gramática.
  expect_reject("<rml><body><div xml:lang=\"en\">x</div></body></rml>",
                "reject: namespaced attribute name (lexer Error propagated)");
  // EN: Unquoted attribute value.
  // PT: Valor de atributo sem aspas.
  expect_reject("<rml><body><div id=panel>x</div></body></rml>",
                "reject: unquoted attribute value (lexer Error propagated)");
  // EN: Unterminated comment.
  // PT: Comentário não terminado.
  expect_reject("<rml><body><!-- never closes</body></rml>",
                "reject: unterminated comment (lexer Error propagated)");
}

void test_entity_hardening() {
  expect_reject("<rml><body>bad &foo; entity</body></rml>",
                "reject: unrecognised named entity");
  expect_reject("<rml><body>AT&T no semicolon</body></rml>",
                "reject: bare '&' never closed by ';'");
  expect_reject("<rml><body>&;</body></rml>", "reject: empty entity name '&;'");
  expect_reject("<rml><body>&#x110000;</body></rml>",
                "reject: numeric entity above the Unicode max (0x10FFFF)");
  expect_reject("<rml><body>&#xD800;</body></rml>",
                "reject: numeric entity in the UTF-16 surrogate range (invalid codepoint)");
  expect_reject("<rml><body>&#0;</body></rml>", "reject: numeric entity for NUL (codepoint 0)");
  expect_reject("<rml><body><div title=\"bad &foo; entity\">x</div></body></rml>",
                "reject: unrecognised entity inside an ATTRIBUTE VALUE too, not just text");

  // EN: The accept side of the exact same boundary -- proves this is a real reject, not an
  //     accidentally-broken decoder.
  // PT: O lado aceita da mesma fronteira exata -- prova que isto é uma rejeição real, não um
  //     decodificador quebrado por acidente.
  expect_ok("<rml><body>&amp;&lt;&gt;&quot;&apos;&nbsp;&#65;&#x41;</body></rml>",
            "accept: every recognised entity form, named + numeric decimal + numeric hex");
}

// ---------------------------------------------------------------------------
// EN: Nesting-depth ceiling -- reuses glintfx::uix::kMaxElementDepth (256, dom_tree.hpp). Proves
//     BOTH sides of the boundary: exactly at the ceiling accepts, one level past it rejects --
//     per this house's "widening the boundary keeps the edge inside; also test one step past"
//     convention (a test only at the edge would not catch an off-by-one in either direction).
//     `body` itself is depth 1, so a document with N nested <div>s inside <body> reaches depth
//     N+1 at the deepest <div>.
// PT: Teto de profundidade de aninhamento -- reusa glintfx::uix::kMaxElementDepth (256,
//     dom_tree.hpp). Prova OS DOIS lados da fronteira: exatamente no teto aceita, um nível além
//     rejeita -- pela convenção da casa "alargar a fronteira mantém a borda dentro; testar também
//     um passo além". `body` em si é profundidade 1, então um documento com N <div>s aninhados
//     dentro de <body> alcança profundidade N+1 no <div> mais fundo.
// ---------------------------------------------------------------------------
std::string build_nested_document(int div_count) {
  std::string s = "<rml><body>";
  for (int i = 0; i < div_count; ++i) {
    s += "<div>";
  }
  s += "x";
  for (int i = 0; i < div_count; ++i) {
    s += "</div>";
  }
  s += "</body></rml>";
  return s;
}

void test_depth_ceiling() {
  // body is depth 1; kMaxElementDepth-1 nested <div>s puts the deepest <div> at exactly depth
  // kMaxElementDepth (256) -- must be ACCEPTED (the ceiling itself is a valid, reachable depth).
  const std::string at_ceiling = build_nested_document(
      static_cast<int>(glintfx::uix::kMaxElementDepth) - 1);
  expect_ok(at_ceiling, "depth: exactly kMaxElementDepth (256) is ACCEPTED");

  // One <div> further -- depth 257 -- must be REJECTED.
  const std::string past_ceiling =
      build_nested_document(static_cast<int>(glintfx::uix::kMaxElementDepth));
  expect_reject(past_ceiling, "depth: kMaxElementDepth + 1 (257) is REJECTED");
}

// ---------------------------------------------------------------------------
// EN: 🔴 KNOWN GAP, PINNED EXPLICITLY, NEVER SILENT -- glintfx::uix::Lexer, run STANDALONE over
//     a whole document (exactly what lexer_corpus_sanity.cpp does), cannot tokenize `<style>`
//     content containing a literal, un-escaped run of two or more '<' (e.g. real GusWorld RCSS
//     prose, "~128dp << 228dp", pt-br for "much less than"). Real upstream RmlUi tolerates this:
//     `script`/`style` are registered, at the TOKENIZER layer, as CDATA tags
//     (`Factory.cpp:256-257`, `XMLParser::RegisterPersistentCDATATag("style")`), scanned by
//     `BaseXMLParser::ReadCDATA` -- a dedicated raw scan where `<` is only markup when
//     immediately followed by `/` + the terminator tag's own name; every OTHER `<` (a stray `<<`
//     included) is literal content. This project's `Lexer` (lexer.cpp) has NO tag-name awareness
//     by design (lexer.hpp's own header: "does NOT special-case the tag name `head`" -- nor,
//     by the identical reasoning, `script`/`style`), so it cannot replicate that tolerance.
//
//     THIS TEST PROVES BOTH HALVES SIDE BY SIDE, SO NEITHER CAN GO UNNOTICED:
//       (1) the bare Lexer genuinely rejects it (an honest, expected `Error` -- NOT a crash, NOT
//           silently wrong tokenization);
//       (2) THIS parser (`parse_document`) is IMMUNE when the same content sits inside `<head>`
//           (its universal real-world position -- 100% of the corpus), because parser.cpp's own
//           `<head>`-opacity raw-byte-scan (see parser.hpp header) never runs the Lexer over
//           `<head>`'s interior AT ALL.
//     Fixing the Lexer itself (giving it a CDATA-tag concept for `script`/`style`, matching
//     upstream) is a `lexer.cpp` change -- OUT OF THIS SLICE'S FILE OWNERSHIP
//     (`parser.{hpp,cpp}`/`parser_*.cpp` only) -- and is reported to the orchestrator/líder as a
//     follow-up item, not fixed here. This is WHY the real fixture that exercises this
//     (glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml) lives in its OWN directory,
//     not one lexer_corpus_sanity.cpp (S1's file, not this file's to edit) already scans: keeping
//     it there would make S1's own standalone-lexer suite red for a gap this slice cannot fix
//     without touching lexer.cpp; this test is the explicit, named, always-run pin that keeps the
//     finding visible instead.
// PT: 🔴 LACUNA CONHECIDA, PRESA EXPLICITAMENTE, NUNCA SILENCIOSA -- o glintfx::uix::Lexer,
//     rodando STANDALONE sobre um documento inteiro (exatamente o que o lexer_corpus_sanity.cpp
//     faz), não consegue tokenizar conteúdo de `<style>` contendo um trecho literal,
//     não-escapado, de dois ou mais '<' (ex.: prosa RCSS real do GusWorld, "~128dp << 228dp",
//     pt-br pra "muito menor que"). O RmlUi upstream real tolera isto: `script`/`style` são
//     registradas, na camada de TOKENIZADOR, como tags CDATA (`Factory.cpp:256-257`,
//     `XMLParser::RegisterPersistentCDATATag("style")`), escaneadas pelo
//     `BaseXMLParser::ReadCDATA` -- um scan cru dedicado onde `<` só é markup quando
//     imediatamente seguido de `/` + o próprio nome da tag terminadora; todo OUTRO `<` (um `<<`
//     perdido incluso) é conteúdo literal. O `Lexer` deste projeto (lexer.cpp) NÃO TEM
//     consciência de nome-de-tag nenhuma por desenho (o próprio cabeçalho do lexer.hpp: "NÃO
//     trata especialmente o nome de tag `head`" -- nem, pelo raciocínio idêntico, `script`/
//     `style`), então não consegue replicar essa tolerância.
//
//     ESTE TESTE PROVA AS DUAS METADES LADO A LADO, PRA NENHUMA PASSAR DESPERCEBIDA:
//       (1) o Lexer cru genuinamente rejeita (um `Error` honesto e esperado -- NÃO um crash, NÃO
//           uma tokenização errada em silêncio);
//       (2) ESTE parser (`parse_document`) é IMUNE quando o mesmo conteúdo está dentro de
//           `<head>` (a posição universal dele no mundo real -- 100% do corpus), porque o
//           próprio scan cru de byte de opacidade de `<head>` do parser.cpp (ver cabeçalho do
//           parser.hpp) nunca roda o Lexer sobre o interior de `<head>` NENHUMA VEZ.
//     Consertar o próprio Lexer (dar a ele um conceito de tag-CDATA pra `script`/`style`,
//     batendo com o upstream) é mudança de `lexer.cpp` -- FORA DA POSSE DE ARQUIVO DESTA FATIA
//     (`parser.{hpp,cpp}`/`parser_*.cpp` só) -- e é relatado ao orquestrador/líder como item de
//     acompanhamento, não consertado aqui. É POR ISSO que a fixture real que exercita isto
//     (glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml) mora no PRÓPRIO diretório
//     dela, não num dos que o lexer_corpus_sanity.cpp (arquivo da S1, não deste arquivo pra
//     editar) já varre: deixá-la lá deixaria a própria suíte standalone-do-lexer da S1 vermelha
//     por uma lacuna que esta fatia não pode consertar sem tocar lexer.cpp; este teste é o pin
//     explícito, nomeado, sempre-rodado que mantém o achado visível em vez disso.
// ---------------------------------------------------------------------------
void test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt() {
  using glintfx::uix::Lexer;
  using glintfx::uix::Token;
  using glintfx::uix::TokenKind;

  const char* kStyleWithStrayLt =
      "<rml><head><style>#x { width: 110dp; } /* 128dp << 228dp */</style></head>"
      "<body>ok</body></rml>";

  // (1) The bare Lexer, tokenizing generically start to finish, hits the stray '<' inside
  //     <style> and emits an honest Error -- this is the documented, expected boundary, not a
  //     surprise.
  Lexer lex(kStyleWithStrayLt);
  Token tok;
  bool saw_error = false;
  for (int i = 0; i < 1000; ++i) {
    tok = lex.next();
    if (tok.kind == TokenKind::Error) {
      saw_error = true;
      break;
    }
    if (tok.kind == TokenKind::EndOfFile) {
      break;
    }
  }
  check(saw_error,
        "known-gap: bare Lexer standalone genuinely rejects '<<' inside <style> content (pins "
        "the documented gap -- if this ever starts PASSING, the gap was fixed and this pin's "
        "own comment/report is stale, go update them)");

  // (2) THIS parser is immune -- the exact same bytes, inside <head>, parse cleanly, because
  //     handle_head()'s raw-byte-scan never re-tokenizes <head>'s interior with the Lexer.
  expect_ok(kStyleWithStrayLt,
            "known-gap: THIS parser is immune -- '<<' inside <head>'s opaque <style> payload "
            "never reaches any tokenizer at all");
}

} // namespace

int main() {
  test_missing_rml_wrapper();
  test_missing_body();
  test_duplicate_head();
  test_duplicate_body();
  test_head_after_body();
  test_stray_element_at_rml_level();
  test_nonwhitespace_text_at_rml_level();
  test_trailing_content_after_rml();
  test_head_nested_inside_body();
  test_mismatched_close_tag();
  test_unclosed_element_eof();
  test_lexer_error_propagates();
  test_entity_hardening();
  test_depth_ceiling();
  test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt();

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_hardening_sanity: PASS");
  return 0;
}
