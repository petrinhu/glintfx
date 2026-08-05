// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- hardening unit test for glintfx::uix::Lexer: the two byte ceilings
//     (kMaxInputBytes/kMaxTokenBytes), malformed-input rejection, and the sticky-Error/sticky-
//     EndOfFile contract. See glintfx/src/uix/dom/lexer.hpp's own header comment, "Hardening" and
//     "Error stickiness" paragraphs, for the derivation/rationale each case here exercises.
//
//     Boundary discipline (this project's own "measure the boundary, not the exaggeration"
//     lesson -- see e.g. clock_sanity.cpp's TST-CLOCK-UNIT header note): every ceiling below is
//     tested AT the exact boundary (accepted) AND one byte past it (rejected), never just "a big
//     number" on one side.
// PT: RMLX-1/S1 -- teste unit de hardening pro glintfx::uix::Lexer: os dois tetos de byte
//     (kMaxInputBytes/kMaxTokenBytes), rejeição de input malformado, e o contrato de
//     Error-pegajoso/EndOfFile-pegajoso. Ver os parágrafos "Hardening" e "Pegajosidade de erro" do
//     próprio comentário de cabeçalho do glintfx/src/uix/dom/lexer.hpp pra derivação/racional de
//     cada caso exercitado aqui.
//
//     Disciplina de fronteira (a própria lição deste projeto de "medir a fronteira, não o
//     exagero" -- ver ex. a nota de cabeçalho TST-CLOCK-UNIT do clock_sanity.cpp): todo teto
//     abaixo é testado NA fronteira exata (aceito) E um byte além dela (rejeitado), nunca só "um
//     número grande" de um lado só.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/lexer.hpp"

#include <cstdio>
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

using glintfx::uix::kMaxInputBytes;
using glintfx::uix::kMaxTokenBytes;
using glintfx::uix::Lexer;
using glintfx::uix::Token;
using glintfx::uix::TokenKind;

// ---------------------------------------------------------------------------
// EN: kMaxInputBytes -- exact boundary accepted, one byte past it rejected at construction time
//     (the FIRST next() call, offset 0, per lexer.hpp's own documented contract).
// PT: kMaxInputBytes -- fronteira exata aceita, um byte além dela rejeitado no momento da
//     construção (a PRIMEIRA chamada de next(), offset 0, pelo próprio contrato documentado do
//     lexer.hpp).
// ---------------------------------------------------------------------------
void test_max_input_bytes_boundary() {
  // EN: A well-formed, minimal, but SIZE-PADDED document: a self-closed <a/> plus enough leading
  //     text to hit the ceiling exactly. Text is chosen (not e.g. a giant attribute value) so
  //     this case tests ONLY kMaxInputBytes, not kMaxTokenBytes (a text run this long would ALSO
  //     trip kMaxTokenBytes if it were a single token -- see the "text run" ceiling test below for
  //     that one in isolation). Splitting the padding into many small tags keeps this case
  //     isolated to the INPUT ceiling alone.
  // PT: Um documento bem-formado, mínimo, mas PREENCHIDO EM TAMANHO: um <a/> auto-fechado mais
  //     texto inicial suficiente pra bater o teto exatamente. Texto é escolhido (não, por
  //     exemplo, um valor de atributo gigante) pra este caso testar SÓ o kMaxInputBytes, não o
  //     kMaxTokenBytes (um trecho de texto tão longo TAMBÉM disparia o kMaxTokenBytes se fosse um
  //     único token -- ver o teste de teto "trecho de texto" abaixo pra esse isolado). Dividir o
  //     preenchimento em muitas tags pequenas mantém este caso isolado só ao teto de INPUT.
  const std::string tag = "<a/>";
  std::string padded;
  padded.reserve(kMaxInputBytes);
  while (padded.size() + tag.size() <= kMaxInputBytes) {
    padded += tag;
  }
  // EN: Pad the remainder with more <a/> markers is not always exact -- top off with '<a/>' sized
  //     chunks already guarantees padded.size() <= kMaxInputBytes; fill the last few bytes with a
  //     trailing comment marker sized to land EXACTLY on the ceiling.
  // PT: Preencher o restante com mais marcadores <a/> nem sempre é exato -- completar com blocos
  //     do tamanho de '<a/>' já garante padded.size() <= kMaxInputBytes; preenche os últimos bytes
  //     com um marcador de comentário final dimensionado pra pousar EXATAMENTE no teto.
  while (padded.size() < kMaxInputBytes) {
    padded += ' ';
  }
  check(padded.size() == kMaxInputBytes, "input ceiling setup: padded doc is EXACTLY kMaxInputBytes");

  {
    Lexer lex(padded);
    Token first = lex.next();
    check(first.kind != TokenKind::Error,
          "kMaxInputBytes AT the boundary: accepted (no Error on construction)");
  }

  {
    std::string over_by_one = padded + "x";
    check(over_by_one.size() == kMaxInputBytes + 1,
          "input ceiling setup: over_by_one is EXACTLY kMaxInputBytes + 1");
    Lexer lex(over_by_one);
    Token first = lex.next();
    check(first.kind == TokenKind::Error,
          "kMaxInputBytes ONE BYTE PAST the boundary: rejected (Error on first next())");
    check(first.offset == 0,
          "kMaxInputBytes violation: Error reported at offset 0 (whole-input "
          "ceiling, not a mid-scan position)");

    // EN: Sticky-Error contract: three more calls, same token.
    // PT: Contrato de Error-pegajoso: mais três chamadas, mesmo token.
    Token second = lex.next();
    Token third = lex.next();
    check(second.kind == TokenKind::Error && third.kind == TokenKind::Error,
          "kMaxInputBytes violation: Error is sticky across repeated next() calls");
    check(second.offset == first.offset && third.offset == first.offset,
          "kMaxInputBytes violation: sticky Error keeps the SAME offset every call");
  }
}

// ---------------------------------------------------------------------------
// EN: kMaxTokenBytes -- single text run at the exact boundary (accepted) vs. one byte past it
//     (rejected). The corpus's own longest single line is 230 bytes (fonteng_sup_scene.rml,
//     measured via `awk '{print length}' glintfx/tests/*.rml glintfx/demos/**/*.rml | sort -rn |
//     head -1`) -- kMaxTokenBytes (64 KiB) is ~285x that, so this boundary is never touched by
//     real content, only by a deliberately hostile probe like this one.
// PT: kMaxTokenBytes -- um único trecho de texto na fronteira exata (aceito) vs. um byte além dela
//     (rejeitado). A maior linha única do próprio corpus são 230 bytes (fonteng_sup_scene.rml,
//     medido via `awk '{print length}' glintfx/tests/*.rml glintfx/demos/**/*.rml | sort -rn |
//     head -1`) -- kMaxTokenBytes (64 KiB) é ~285x isso, então esta fronteira nunca é tocada por
//     conteúdo real, só por uma sonda deliberadamente hostil como esta.
// ---------------------------------------------------------------------------
void test_max_token_bytes_boundary_text_run() {
  {
    // EN: A text run of EXACTLY kMaxTokenBytes bytes, wrapped so the lexer sees it as a single
    //     Text token (bounded by '<' on both sides -- a real document's <body>...</body>).
    // PT: Um trecho de texto de EXATAMENTE kMaxTokenBytes bytes, envolto pra o lexer ver como um
    //     único token Text (limitado por '<' dos dois lados -- um <body>...</body> de documento
    //     real).
    std::string doc = "<body>" + std::string(kMaxTokenBytes, 'x') + "</body>";
    Lexer lex(doc);
    Token open = lex.next();
    check(open.kind == TokenKind::TagOpenStart, "token ceiling setup: TagOpenStart before probe");
    Token end = lex.next();
    check(end.kind == TokenKind::TagOpenEnd, "token ceiling setup: TagOpenEnd before probe");
    Token text = lex.next();
    check(text.kind == TokenKind::Text,
          "kMaxTokenBytes AT the boundary (text run): accepted, NOT an Error");
    check(text.text.size() == kMaxTokenBytes,
          "kMaxTokenBytes AT the boundary (text run): full content preserved, no truncation");
  }

  {
    std::string doc = "<body>" + std::string(kMaxTokenBytes + 1, 'x') + "</body>";
    Lexer lex(doc);
    lex.next(); // TagOpenStart
    lex.next(); // TagOpenEnd
    Token text = lex.next();
    check(text.kind == TokenKind::Error,
          "kMaxTokenBytes ONE BYTE PAST the boundary (text run): rejected (Error), never "
          "truncated-and-continued (fail-high, see lexer.hpp header)");
  }
}

// ---------------------------------------------------------------------------
// EN: kMaxTokenBytes also bounds a single ATTRIBUTE VALUE -- a distinct call site from the text-
//     run case above (see lexer.cpp's scan_in_tag()), so it needs its own boundary probe rather
//     than assuming the text-run test above covers it too.
// PT: kMaxTokenBytes também limita um único VALOR DE ATRIBUTO -- um call site distinto do caso de
//     trecho-de-texto acima (ver o scan_in_tag() do lexer.cpp), então precisa da própria sonda de
//     fronteira em vez de assumir que o teste de trecho-de-texto acima também o cobre.
// ---------------------------------------------------------------------------
void test_max_token_bytes_boundary_attr_value() {
  {
    std::string doc = "<div title=\"" + std::string(kMaxTokenBytes, 'x') + "\"/>";
    Lexer lex(doc);
    lex.next(); // TagOpenStart
    Token attr = lex.next();
    check(attr.kind == TokenKind::Attr,
          "kMaxTokenBytes AT the boundary (attr value): accepted, NOT an Error");
    check(attr.value.size() == kMaxTokenBytes,
          "kMaxTokenBytes AT the boundary (attr value): full value preserved");
  }
  {
    std::string doc = "<div title=\"" + std::string(kMaxTokenBytes + 1, 'x') + "\"/>";
    Lexer lex(doc);
    lex.next(); // TagOpenStart
    Token attr = lex.next();
    check(attr.kind == TokenKind::Error,
          "kMaxTokenBytes ONE BYTE PAST the boundary (attr value): rejected (Error)");
  }
}

// ---------------------------------------------------------------------------
// EN: Malformed-input rejection, one case per distinct failure mode this lexer's grammar
//     recognises (see lexer.hpp header comment for why each is fail-high, never guessed-at).
// PT: Rejeição de input malformado, um caso por modo de falha distinto que a gramática deste
//     lexer reconhece (ver o comentário de cabeçalho do lexer.hpp pro motivo de cada um ser
//     fail-high, nunca chutado).
// ---------------------------------------------------------------------------
void test_malformed_inputs_reject() {
  struct Case {
    const char* source;
    const char* what;
  };
  const Case cases[] = {
      {"<1x>", "tag name cannot START with a digit (is_name_start rejects digits)"},
      {"< span>", "'<' followed by whitespace is not a tag/comment/close-tag"},
      {"<>", "'<' followed immediately by '>' names no tag"},
      {"<div", "unterminated tag: EOF while still inside '<div' (no '>' or '/>')"},
      {"<div attr>",
       "attribute with no '=value' (bare/boolean attribute) is rejected -- RML/RCSS "
       "has none in the corpus, see lexer.hpp header"},
      {"<div attr=>", "attribute '=' not followed by a quote character"},
      {"<div attr=\"unterminated>", "attribute value quote never closes before EOF"},
      {"<div attr=\"a<b\">", "a literal '<' inside a quoted attribute value is rejected"},
      {"<!-- unterminated", "comment never sees a closing '-->' before EOF"},
      {"<!DOCTYPE html>",
       "DOCTYPE is out of the RMLX-1..11 subset (docs/rmlx-subset.md) -- "
       "zero corpus occurrences, rejected rather than guessed at"},
      {"<?xml version=\"1.0\"?>", "XML declaration is out of subset, same reasoning as DOCTYPE"},
      {"<![CDATA[x]]>", "CDATA is out of subset, same reasoning as DOCTYPE"},
      {"</1x>", "closing tag name cannot start with a digit either"},
      {"</div", "unterminated closing tag: EOF before the closing '>'"},
  };

  for (const Case& c : cases) {
    Lexer lex(c.source);
    Token tok = lex.next();
    // EN: A malformed input might take more than one token to surface as an Error (e.g. "<div"
    //     alone is a valid TagOpenStart, THEN errors on the next() call once InTag scanning hits
    //     EOF) -- drain up to 4 tokens looking for the Error, matching how a real consumer would
    //     drive this Lexer in a loop.
    // PT: Um input malformado pode levar mais de um token pra aparecer como Error (ex.: "<div"
    //     sozinho é um TagOpenStart válido, DEPOIS erra na chamada de next() seguinte quando o
    //     scan InTag bate no EOF) -- drena até 4 tokens procurando o Error, espelhando como um
    //     consumidor real dirigiria este Lexer num laço.
    int drained = 0;
    while (tok.kind != TokenKind::Error && tok.kind != TokenKind::EndOfFile && drained < 4) {
      tok = lex.next();
      ++drained;
    }
    check(tok.kind == TokenKind::Error, c.what);
  }
}

// ---------------------------------------------------------------------------
// EN: Documented PERMISSIVE decision (see lexer.cpp's own comment on this): NO whitespace
//     required between two adjacent attributes. Real strict-XML grammar requires it; this
//     lexer does not enforce it because the corpus never needs the rejection and the leniency
//     introduces no ambiguity (the closing quote unambiguously ends the previous attribute).
//     Locked here as an explicit, intentional, tested characteristic -- not an accidental gap
//     discovered later. See this slice's own report to the orchestrator for the open-question
//     framing of this exact choice.
// PT: Decisão PERMISSIVA documentada (ver o próprio comentário do lexer.cpp sobre isto): NENHUM
//     whitespace exigido entre dois atributos adjacentes. A gramática XML estrita de verdade
//     exige; este lexer não a impõe porque o corpus nunca precisa da rejeição e a leniência não
//     introduz ambiguidade nenhuma (a aspa de fechamento encerra o atributo anterior sem
//     ambiguidade). Travado aqui como característica explícita, intencional e testada -- não uma
//     lacuna acidental descoberta depois. Ver o próprio relato desta fatia ao orquestrador pro
//     enquadramento de ponto-aberto desta escolha exata.
// ---------------------------------------------------------------------------
void test_permissive_no_whitespace_between_attrs() {
  Lexer lex("<div id=\"x\"class=\"y\">");
  lex.next(); // TagOpenStart
  Token a1 = lex.next();
  Token a2 = lex.next();
  Token end = lex.next();
  check(a1.kind == TokenKind::Attr && a1.text == "id" && a1.value == "x",
        "permissive-no-ws: first attr ('id') still scanned correctly");
  check(a2.kind == TokenKind::Attr && a2.text == "class" && a2.value == "y",
        "permissive-no-ws: second attr ('class') scanned WITHOUT a preceding whitespace token, "
        "accepted (documented permissive characteristic, not an Error)");
  check(end.kind == TokenKind::TagOpenEnd, "permissive-no-ws: TagOpenEnd follows normally");
}

// ---------------------------------------------------------------------------
// EN: EndOfFile stickiness -- past the natural end of a well-formed document, every further
//     next() call returns the identical EndOfFile token.
// PT: Pegajosidade de EndOfFile -- além do fim natural de um documento bem-formado, toda chamada
//     seguinte de next() retorna o mesmo token EndOfFile idêntico.
// ---------------------------------------------------------------------------
void test_end_of_file_sticky() {
  Lexer lex("<a/>");
  lex.next(); // TagOpenStart
  lex.next(); // TagSelfClose
  Token first_eof = lex.next();
  check(first_eof.kind == TokenKind::EndOfFile, "EOF-sticky: natural end reached");
  Token second_eof = lex.next();
  Token third_eof = lex.next();
  check(second_eof.kind == TokenKind::EndOfFile && third_eof.kind == TokenKind::EndOfFile,
        "EOF-sticky: repeated next() calls past the end keep returning EndOfFile");
  check(first_eof.offset == second_eof.offset && second_eof.offset == third_eof.offset,
        "EOF-sticky: offset stays identical across repeated calls");
}

} // namespace

int main() {
  test_max_input_bytes_boundary();
  test_max_token_bytes_boundary_text_run();
  test_max_token_bytes_boundary_attr_value();
  test_malformed_inputs_reject();
  test_permissive_no_whitespace_between_attrs();
  test_end_of_file_sticky();

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_hardening_sanity: PASS");
  return 0;
}
