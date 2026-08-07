// SPDX-License-Identifier: Apache-2.0
// EN: UIX-LEXER-COMENT-ESPACO -- regression suite for the corpus-measured defect this item fixes:
//     `glintfx::uix::style::Lexer` recognises `/* ... */` as its own `Comment` token only at a
//     "fresh scan start" (lexer.hpp header, "Comment handling" point (1)), but BEFORE this fix that
//     fresh-scan-start check required the comment's own `/*` to sit at EXACTLY the byte immediately
//     following the delimiter/Declaration -- zero bytes of whitespace tolerated. No real CSS author
//     writes that way: `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` lines 29-31/
//     44/47 all have a comment preceded by at least one space/newline, and the destructive
//     consequence was measured directly by `UIX-RCSS-ORACULO` (TODO.md): the comment's own bytes
//     fused into the NAME of the declaration that should have come after it, corrupting/destroying
//     that declaration (12 `ParseDiagnostic`s from that ONE fixture, explaining the majority of its
//     102 divergent dump lines against the RmlUi oracle).
//
//     This file is organised in the SAME order as the item's own brief: (1) the destructive bug in
//     Declaration mode, (2) its non-destructive-but-contract-violating twin in Structural mode
//     (found by this item's own domino audit of every fresh-scan-start dispatch point in the
//     file -- there are exactly two, `scan_structural()`'s own entry and `scan_declaration()`'s own
//     loop top, both were affected), (3) the "still works" regression case (comment glued directly,
//     zero whitespace -- already covered by lexer_tokens_sanity.cpp's own
//     test_comments_before_selector_and_between_declarations, re-asserted here so THIS file is a
//     standalone, complete proof of the fix), and (4) the two DELIBERATE mid-run divergences this
//     fix must NOT touch (lexer.hpp header, "Comment handling" point (1), both halves -- the
//     Declaration-name/value case AND the Prelude case this item's own audit found and documented
//     for the first time).
// PT: UIX-LEXER-COMENT-ESPACO -- suíte de regressão pro defeito medido-por-corpus que este item
//     conserta: o `glintfx::uix::style::Lexer` reconhece `/* ... */` como o próprio token `Comment`
//     só num "início de scan fresco" (comentário de cabeçalho do lexer.hpp, "Trato de comentário",
//     ponto (1)), mas ANTES deste conserto esse check de início-de-scan-fresco exigia que o próprio
//     `/*` do comentário estivesse EXATAMENTE no byte imediatamente seguinte ao delimitador/
//     Declaration -- zero bytes de whitespace tolerados. Nenhum autor CSS real escreve assim: o
//     `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` linhas 29-31/44/47 têm todos
//     um comentário precedido de pelo menos um espaço/quebra-de-linha, e a consequência destrutiva
//     foi medida diretamente pelo `UIX-RCSS-ORACULO` (TODO.md): os bytes do próprio comentário se
//     fundiam no NOME da declaração que deveria vir depois dele, corrompendo/destruindo aquela
//     declaração (12 `ParseDiagnostic`s dessa ÚNICA fixture, explicando a maioria das 102 linhas
//     divergentes do dump dela contra o oráculo do RmlUi).
//
//     Este arquivo está organizado na MESMA ordem do próprio briefing do item: (1) o bug destrutivo
//     em modo Declaration, (2) o gêmeo não-destrutivo-mas-que-viola-contrato dele em modo
//     Structural (achado pela própria auditoria-dominó deste item em todo ponto de despacho de
//     início-de-scan-fresco do arquivo -- existem exatamente dois, a própria entrada do
//     scan_structural() e o próprio topo-de-laço do scan_declaration(), os dois estavam afetados),
//     (3) o caso de regressão "continua funcionando" (comentário colado direto, zero whitespace --
//     já coberto pelo próprio test_comments_before_selector_and_between_declarations do
//     lexer_tokens_sanity.cpp, re-asserido aqui pra ESTE arquivo ser uma prova standalone, completa,
//     do conserto), e (4) as duas divergências DELIBERADAS de meio-de-trecho que este conserto NÃO
//     pode tocar (comentário de cabeçalho do lexer.hpp, "Trato de comentário", ponto (1), as duas
//     metades -- o caso de nome/valor de Declaration E o caso de Prelude que a própria auditoria
//     deste item achou e documentou pela primeira vez).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/lexer.hpp"

#include <cstdio>
#include <string_view>

namespace {

int g_failures = 0;

void check_eq(std::string_view got, std::string_view want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::style::Lexer;
using glintfx::uix::style::Token;
using glintfx::uix::style::TokenKind;

const char* kind_name(TokenKind k) {
  switch (k) {
    case TokenKind::At:
      return "At";
    case TokenKind::Prelude:
      return "Prelude";
    case TokenKind::BraceOpen:
      return "BraceOpen";
    case TokenKind::BraceClose:
      return "BraceClose";
    case TokenKind::Declaration:
      return "Declaration";
    case TokenKind::Comment:
      return "Comment";
    case TokenKind::EndOfFile:
      return "EndOfFile";
    case TokenKind::Error:
      return "Error";
  }
  return "?";
}

Token expect_kind(Lexer& lex, TokenKind want, const char* what) {
  Token tok = lex.next();
  if (tok.kind != want) {
    std::fprintf(stderr, "FAIL: %s (got kind %s, want %s)\n", what, kind_name(tok.kind),
                 kind_name(want));
    ++g_failures;
  }
  return tok;
}

// ---------------------------------------------------------------------------
// EN: Section (1) -- the DESTRUCTIVE bug, Declaration mode: a comment after `;`, preceded by
//     whitespace, must be recognised as its OWN `Comment` token, and the declaration that follows
//     it must survive intact -- NOT be destroyed by having the comment's bytes fused into its name.
//     Four sub-cases: space, tab, newline, and a mixed run (space+newline+tab+space), matching the
//     brief's own "space, de tabulação, de quebra de linha, e de combinações" list.
// PT: Seção (1) -- o bug DESTRUTIVO, modo Declaration: um comentário depois de `;`, precedido de
//     whitespace, precisa ser reconhecido como o PRÓPRIO token `Comment`, e a declaração que vem
//     depois dele precisa sobreviver intacta -- NÃO ser destruída por ter os bytes do comentário
//     fundidos no próprio nome dela. Quatro subcasos: espaço, tabulação, quebra de linha, e um
//     trecho misto (espaço+quebra+tab+espaço), casando com a própria lista "space, de tabulação, de
//     quebra de linha, e de combinações" do briefing.
// ---------------------------------------------------------------------------
void test_comment_after_semicolon_preceded_by_space() {
  Lexer lex("a { width: 1px; /* c */ height: 2px; }");
  expect_kind(lex, TokenKind::Prelude, "after-semi-space: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-semi-space: BraceOpen");
  Token d1 = expect_kind(lex, TokenKind::Declaration, "after-semi-space: Declaration 1");
  check_eq(d1.text, "width", "after-semi-space: decl1 name intact");
  Token c = expect_kind(lex, TokenKind::Comment, "after-semi-space: Comment recognised, not fused");
  check_eq(c.text, " c ", "after-semi-space: comment body");
  Token d2 = expect_kind(lex, TokenKind::Declaration, "after-semi-space: Declaration 2 survives");
  check_eq(d2.text, "height", "after-semi-space: decl2 name NOT corrupted by the comment's bytes");
  check_eq(d2.value, "2px", "after-semi-space: decl2 value intact");
  expect_kind(lex, TokenKind::BraceClose, "after-semi-space: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "after-semi-space: EndOfFile");
}

void test_comment_after_semicolon_preceded_by_tab() {
  Lexer lex("a { width: 1px;\t/* c */\theight: 2px; }");
  expect_kind(lex, TokenKind::Prelude, "after-semi-tab: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-semi-tab: BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "after-semi-tab: Declaration 1");
  expect_kind(lex, TokenKind::Comment, "after-semi-tab: Comment recognised, not fused");
  Token d2 = expect_kind(lex, TokenKind::Declaration, "after-semi-tab: Declaration 2 survives");
  check_eq(d2.text, "height", "after-semi-tab: decl2 name NOT corrupted");
  expect_kind(lex, TokenKind::BraceClose, "after-semi-tab: BraceClose");
}

void test_comment_after_semicolon_preceded_by_newline() {
  Lexer lex("a {\n  width: 1px;\n  /* c */\n  height: 2px;\n}");
  expect_kind(lex, TokenKind::Prelude, "after-semi-newline: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-semi-newline: BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "after-semi-newline: Declaration 1");
  expect_kind(lex, TokenKind::Comment, "after-semi-newline: Comment recognised, not fused");
  Token d2 = expect_kind(lex, TokenKind::Declaration, "after-semi-newline: Declaration 2 survives");
  check_eq(d2.text, "height", "after-semi-newline: decl2 name NOT corrupted");
  expect_kind(lex, TokenKind::BraceClose, "after-semi-newline: BraceClose");
}

void test_comment_after_semicolon_preceded_by_mixed_whitespace_combo() {
  // EN: The exact byte shape of gusworld_battle_cockpit.rml:29-31 -- ';', two spaces, a comment on
  //     the SAME line, then a newline, then a two-space indent, then the next declaration on its
  //     OWN line. This is the concrete real-world fixture that proved the bug, reproduced verbatim
  //     rather than paraphrased.
  // PT: A forma exata de bytes do gusworld_battle_cockpit.rml:29-31 -- ';', dois espaços, um
  //     comentário na MESMA linha, depois uma quebra de linha, depois uma indentação de dois
  //     espaços, depois a próxima declaração na PRÓPRIA linha. Esta é a fixture real, concreta, que
  //     provou o bug, reproduzida verbatim em vez de parafraseada.
  Lexer lex(
      "#cockpit {\n"
      "  decorator: vertical-gradient( #141a2c #0f1322 );  /* topo escurecido */\n"
      "  padding: 10dp 12dp 0dp 12dp;\n"
      "}");
  expect_kind(lex, TokenKind::Prelude, "mixed-combo: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "mixed-combo: BraceOpen");
  Token d1 = expect_kind(lex, TokenKind::Declaration, "mixed-combo: decorator Declaration");
  check_eq(d1.text, "decorator", "mixed-combo: decorator name");
  Token c = expect_kind(lex, TokenKind::Comment, "mixed-combo: Comment recognised, not fused");
  check_eq(c.text, " topo escurecido ", "mixed-combo: comment body");
  Token d2 = expect_kind(lex, TokenKind::Declaration, "mixed-combo: padding Declaration survives");
  check_eq(d2.text, "padding",
           "mixed-combo: padding name -- NOT destroyed, matching the real fix "
           "for gusworld_battle_cockpit.rml:31");
  check_eq(d2.value, "10dp 12dp 0dp 12dp", "mixed-combo: padding value intact");
  expect_kind(lex, TokenKind::BraceClose, "mixed-combo: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "mixed-combo: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Section (1b) -- comment after `{` (Declaration mode: an ordinary rule's own opening brace),
//     preceded by whitespace, must not corrupt the FIRST declaration inside the block.
// PT: Seção (1b) -- comentário depois de `{` (modo Declaration: a própria chave de abertura de uma
//     regra comum), precedido de whitespace, não pode corromper a PRIMEIRA declaração de dentro do
//     bloco.
// ---------------------------------------------------------------------------
void test_comment_after_brace_open_declaration_mode_preceded_by_whitespace() {
  Lexer lex("a {\n  /* c */\n  color: red;\n}");
  expect_kind(lex, TokenKind::Prelude, "after-brace-open-decl: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-open-decl: BraceOpen");
  Token c = expect_kind(lex, TokenKind::Comment, "after-brace-open-decl: Comment recognised");
  check_eq(c.text, " c ", "after-brace-open-decl: comment body");
  Token d = expect_kind(lex, TokenKind::Declaration, "after-brace-open-decl: Declaration survives");
  check_eq(d.text, "color", "after-brace-open-decl: name NOT corrupted");
  check_eq(d.value, "red", "after-brace-open-decl: value intact");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-open-decl: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Section (2) -- the non-destructive twin in Structural mode, found by this item's own domino
//     audit: `scan_structural()`'s own fresh-scan-start dispatch had the IDENTICAL missing-
//     whitespace-skip gap as scan_declaration()'s, just with a cosmetic (not destructive)
//     consequence -- an extra whitespace-only `Prelude` token before the `Comment`, rather than
//     data loss (Structural-mode Prelude runs are byte-verbatim and never corrupt a NAME the way
//     Declaration mode's NAME-run does). Covers `{` opening a NESTED Structural region
//     (`@keyframes`) and `}` closing a region, both at top level and nested, matching the brief's
//     own "comentário após `{` e após `}`".
// PT: Seção (2) -- o gêmeo não-destrutivo em modo Structural, achado pela própria auditoria-dominó
//     deste item: o próprio despacho de início-de-scan-fresco do scan_structural() tinha o MESMO
//     buraco de pular-whitespace-ausente do scan_declaration(), só com consequência cosmética (não
//     destrutiva) -- um token `Prelude` extra, só-whitespace, antes do `Comment`, em vez de perda de
//     dado (trechos de Prelude em modo Structural são byte-verbatim e nunca corrompem um NOME do
//     jeito que o trecho-de-NOME do modo Declaration corrompe). Cobre `{` abrindo uma região
//     Estrutural ANINHADA (`@keyframes`) e `}` fechando uma região, tanto no topo-de-nível quanto
//     aninhado, casando com a própria "comentário após `{` e após `}`" do briefing.
// ---------------------------------------------------------------------------
void test_comment_after_brace_close_structural_mode_top_level_preceded_by_whitespace() {
  Lexer lex("a { color: red; }\n\n/* c */\n\nb { color: blue; }");
  expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct: Prelude a");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-close-struct: BraceOpen a");
  expect_kind(lex, TokenKind::Declaration, "after-brace-close-struct: Declaration a");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-close-struct: BraceClose a");

  // EN: THE FIX: no whitespace-only Prelude token is emitted between the BraceClose and the
  //     Comment -- the Comment comes through as the VERY NEXT token, matching the header's own
  //     "immediately after" contract literally, not just "eventually, after an intervening token".
  // PT: O CONSERTO: nenhum token Prelude só-whitespace é emitido entre o BraceClose e o Comment -- o
  //     Comment vem como o token BEM SEGUINTE, casando com o próprio contrato "imediatamente depois"
  //     do cabeçalho literalmente, não só "eventualmente, depois de um token intermediário".
  Token c = expect_kind(lex, TokenKind::Comment,
                        "after-brace-close-struct: Comment is the VERY NEXT token (no whitespace-"
                        "only Prelude in between)");
  check_eq(c.text, " c ", "after-brace-close-struct: comment body");

  Token prelude_b = expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct: Prelude b");
  check_eq(prelude_b.text, "\n\nb ",
           "after-brace-close-struct: Prelude b keeps ITS OWN leading "
           "whitespace verbatim -- only the whitespace THAT PRECEDED THE "
           "COMMENT was consumed, this one precedes ordinary text and is "
           "untouched, same byte-verbatim Prelude contract as always");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-close-struct: BraceOpen b");
  expect_kind(lex, TokenKind::Declaration, "after-brace-close-struct: Declaration b");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-close-struct: BraceClose b");
  expect_kind(lex, TokenKind::EndOfFile, "after-brace-close-struct: EndOfFile");
}

void test_comment_after_brace_open_structural_mode_nested_keyframes_preceded_by_whitespace() {
  // EN: The `{` here opens a NESTED Structural region (keyframes special case, lexer.hpp header
  //     "At-rule mode table") -- scan_structural() is re-entered, exercising the SAME fresh-scan-
  //     start dispatch point as the top-level case above, just one nesting level deeper.
  // PT: O `{` aqui abre uma região Estrutural ANINHADA (caso especial de keyframes, cabeçalho do
  //     lexer.hpp "Tabela de modo de at-rule") -- o scan_structural() é reentrado, exercitando o
  //     MESMO ponto de despacho de início-de-scan-fresco do caso topo-de-nível acima, só um nível de
  //     aninhamento mais fundo.
  Lexer lex("@keyframes spin {\n  /* c */\n  from { opacity: 0; }\n}");
  expect_kind(lex, TokenKind::At, "after-brace-open-struct: At");
  expect_kind(lex, TokenKind::Prelude, "after-brace-open-struct: outer Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-open-struct: outer BraceOpen (nested mode)");
  Token c = expect_kind(lex, TokenKind::Comment,
                        "after-brace-open-struct: Comment is the VERY NEXT token");
  check_eq(c.text, " c ", "after-brace-open-struct: comment body");
  Token from_prelude = expect_kind(lex, TokenKind::Prelude, "after-brace-open-struct: 'from' Prelude");
  check_eq(from_prelude.text, "\n  from ",
           "after-brace-open-struct: 'from' Prelude keeps its own "
           "leading whitespace, unaffected");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-open-struct: 'from' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "after-brace-open-struct: 'from' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-open-struct: 'from' BraceClose");
}

void test_comment_after_brace_close_structural_mode_nested_keyframes_preceded_by_whitespace() {
  Lexer lex("@keyframes spin { from { opacity: 0; }\n  /* c */\n  to { opacity: 1; } }");
  expect_kind(lex, TokenKind::At, "after-brace-close-struct-nested: At");
  expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct-nested: outer Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-close-struct-nested: outer BraceOpen");
  expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct-nested: 'from' Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-close-struct-nested: 'from' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "after-brace-close-struct-nested: 'from' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-close-struct-nested: 'from' BraceClose");
  Token c = expect_kind(lex, TokenKind::Comment,
                        "after-brace-close-struct-nested: Comment is the VERY NEXT token");
  check_eq(c.text, " c ", "after-brace-close-struct-nested: comment body");
  expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct-nested: 'to' Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "after-brace-close-struct-nested: 'to' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "after-brace-close-struct-nested: 'to' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-close-struct-nested: 'to' BraceClose");
  // EN: The single space between "}" (closing 'to') and the outer "}" is, at this point, back in
  //     Structural mode -- same "trailing-space Prelude" fact lexer_tokens_sanity.cpp's own
  //     test_keyframes_two_level_nesting already documents (Structural mode has no "absorb
  //     whitespace before '}'" loop the way Declaration mode does). Unaffected by this item's fix
  //     (there is no comment here to peek past), listed for completeness of the token sequence.
  // PT: O único espaço entre o "}" (fechando 'to') e o "}" externo está, neste ponto, de volta em
  //     modo Estrutural -- mesmo fato de "Prelude de espaço final" que o próprio
  //     test_keyframes_two_level_nesting do lexer_tokens_sanity.cpp já documenta (modo Estrutural
  //     não tem o laço "absorve whitespace antes de '}'" que o modo Declaration tem). Inafetado pelo
  //     conserto deste item (não há comentário nenhum aqui pra espiar), listado por completude da
  //     sequência de tokens.
  expect_kind(lex, TokenKind::Prelude, "after-brace-close-struct-nested: trailing-space Prelude");
  expect_kind(lex, TokenKind::BraceClose, "after-brace-close-struct-nested: outer BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Section (3) -- glued comment (ZERO whitespace) MUST continue to work exactly as before -- the
//     fresh-scan-start peek must not require whitespace, only tolerate it. Already proven by
//     lexer_tokens_sanity.cpp's own test_comments_before_selector_and_between_declarations;
//     re-asserted here (Declaration mode + Structural mode, both delimiter shapes) so THIS file is
//     a standalone, complete regression proof, matching this repo's own "prove, don't just assert"
//     norm.
// PT: Seção (3) -- comentário colado (ZERO whitespace) PRECISA continuar funcionando exatamente como
//     antes -- o espiar de início-de-scan-fresco não pode EXIGIR whitespace, só tolerá-lo. Já provado
//     pelo próprio test_comments_before_selector_and_between_declarations do
//     lexer_tokens_sanity.cpp; re-asserido aqui (modo Declaration + modo Structural, as duas formas
//     de delimitador) pra ESTE arquivo ser uma prova de regressão standalone, completa, casando com
//     a própria norma "prove, não só afirme" deste repo.
// ---------------------------------------------------------------------------
void test_comment_glued_zero_whitespace_still_works_declaration_mode() {
  Lexer lex("a{/*c*/color:red;}");
  expect_kind(lex, TokenKind::Prelude, "glued-decl: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "glued-decl: BraceOpen");
  Token c = expect_kind(lex, TokenKind::Comment, "glued-decl: Comment, zero leading whitespace");
  check_eq(c.text, "c", "glued-decl: comment body");
  Token d = expect_kind(lex, TokenKind::Declaration, "glued-decl: Declaration survives");
  check_eq(d.text, "color", "glued-decl: name intact");
  expect_kind(lex, TokenKind::BraceClose, "glued-decl: BraceClose");
}

void test_comment_glued_zero_whitespace_still_works_structural_mode() {
  Lexer lex("a{color:red;}/*c*/b{color:blue;}");
  expect_kind(lex, TokenKind::Prelude, "glued-struct: Prelude a");
  expect_kind(lex, TokenKind::BraceOpen, "glued-struct: BraceOpen a");
  expect_kind(lex, TokenKind::Declaration, "glued-struct: Declaration a");
  expect_kind(lex, TokenKind::BraceClose, "glued-struct: BraceClose a");
  Token c = expect_kind(lex, TokenKind::Comment, "glued-struct: Comment, zero leading whitespace");
  check_eq(c.text, "c", "glued-struct: comment body");
  Token prelude_b = expect_kind(lex, TokenKind::Prelude, "glued-struct: Prelude b");
  check_eq(prelude_b.text, "b", "glued-struct: Prelude b, no leading whitespace to begin with");
  expect_kind(lex, TokenKind::BraceOpen, "glued-struct: BraceOpen b");
}

// ---------------------------------------------------------------------------
// EN: Section (4a) -- THE DELIBERATE DIVERGENCE THIS FIX MUST NOT TOUCH: a `/*...*/` appearing
//     MID-RUN inside a Declaration's own NAME (once non-whitespace bytes have already started
//     accumulating) is NOT specially recognised -- `wid/*x*/th` must keep tokenizing as the single,
//     glued identifier "width" (lexer.hpp header, "Comment handling" point (1), first bullet). This
//     is the EXACT example the header cites; reproduced verbatim as the negative-control proof that
//     this item's fix is scoped to fresh-scan-start ONLY, never mid-run.
// PT: Seção (4a) -- A DIVERGÊNCIA DELIBERADA QUE ESTE CONSERTO NÃO PODE TOCAR: um `/*...*/` que
//     aparece NO MEIO de um trecho de NOME de Declaration (uma vez que bytes não-whitespace já
//     começaram a acumular) NÃO é especialmente reconhecido -- `wid/*x*/th` precisa continuar
//     tokenizando como o identificador único, colado, "width" (comentário de cabeçalho do lexer.hpp,
//     "Trato de comentário", ponto (1), primeiro bullet). Este é o EXATO exemplo que o cabeçalho
//     cita; reproduzido verbatim como a prova de controle-negativo de que o conserto deste item é
//     escopado só pra início-de-scan-fresco, nunca meio-de-trecho.
// ---------------------------------------------------------------------------
void test_mid_run_comment_inside_declaration_name_still_fuses_deliberately() {
  Lexer lex("a { wid/*x*/th: 1px; }");
  expect_kind(lex, TokenKind::Prelude, "mid-run-name: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "mid-run-name: BraceOpen");
  Token d = expect_kind(lex, TokenKind::Declaration,
                        "mid-run-name: ONE Declaration, not a Comment interrupting the name");
  check_eq(d.text, "wid/*x*/th",
           "mid-run-name: name keeps the comment's own bytes fused in, "
           "DELIBERATE divergence, must NOT become 'width'");
  check_eq(d.value, "1px", "mid-run-name: value intact");
  expect_kind(lex, TokenKind::BraceClose, "mid-run-name: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Section (4b) -- a SECOND deliberate divergence, found by this item's own domino audit and NOT
//     previously covered by any existing test: a `/*...*/` appearing MID-RUN inside a Prelude
//     (selector/at-rule-prelude text) behaves OPPOSITE to the Declaration-name case above -- the
//     Prelude raw-run loop's own per-iteration stop condition treats "/*" the SAME tier as
//     '{'/'}'/'@', so a comment DOES end an in-progress Prelude run wherever it appears, splitting
//     it into two Prelude tokens with the Comment between them. Locked in here so a future reader
//     has an executable answer instead of having to re-derive it from the source -- this fix does
//     NOT touch scan_structural()'s Prelude-run loop at all, so this behaviour (pre-existing, not
//     introduced by this item) is unaffected either way; the test exists to PROVE that, not to
//     change it.
// PT: Seção (4b) -- uma SEGUNDA divergência deliberada, achada pela própria auditoria-dominó deste
//     item e NÃO coberta por teste nenhum existente antes: um `/*...*/` que aparece NO MEIO de um
//     Prelude (texto de seletor/prelúdio-de-at-rule) se comporta OPOSTO ao caso de nome-de-
//     Declaration acima -- a própria condição-de-parada por-iteração do laço de trecho-cru de
//     Prelude trata "/*" na MESMA categoria de '{'/'}'/'@', então um comentário TERMINA um trecho de
//     Prelude em andamento onde quer que apareça, dividindo-o em dois tokens Prelude com o Comment
//     entre eles. Travado aqui pra um leitor futuro ter uma resposta executável em vez de ter que
//     re-derivar do fonte -- este conserto NÃO toca o laço de trecho-cru de Prelude do
//     scan_structural() em nada, então este comportamento (pré-existente, não introduzido por este
//     item) fica inafetado dos dois jeitos; o teste existe pra PROVAR isso, não pra mudar.
// ---------------------------------------------------------------------------
void test_mid_run_comment_inside_prelude_splits_deliberately_pre_existing_behaviour() {
  Lexer lex(".fo/*x*/o { color: red; }");
  Token p1 = expect_kind(lex, TokenKind::Prelude, "mid-run-prelude: Prelude fragment before comment");
  check_eq(p1.text, ".fo", "mid-run-prelude: first fragment, up to the comment");
  Token c = expect_kind(lex, TokenKind::Comment,
                        "mid-run-prelude: Comment DOES interrupt a Prelude "
                        "run mid-accumulation (opposite of the Declaration-"
                        "name case in section 4a)");
  check_eq(c.text, "x", "mid-run-prelude: comment body");
  Token p2 = expect_kind(lex, TokenKind::Prelude, "mid-run-prelude: Prelude fragment after comment");
  check_eq(p2.text, "o ", "mid-run-prelude: second fragment, resumed after the comment");
  expect_kind(lex, TokenKind::BraceOpen, "mid-run-prelude: BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "mid-run-prelude: Declaration");
  expect_kind(lex, TokenKind::BraceClose, "mid-run-prelude: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Section (5) -- domino check on the at-rule mode table's own `pending_at_rule_word_` capture
//     (lexer.hpp header, "At-rule mode table"): a whitespace-preceded comment between `@` and the
//     at-rule's own Prelude must NOT interfere with the keyframes-name capture -- the Comment token
//     never touches `awaiting_at_rule_word_`/`pending_at_rule_word_`, so the flag survives across it
//     unchanged, same as it already survives across a GLUED comment there (pre-existing behaviour,
//     unaffected by this fix, exercised here with the whitespace this item's fix newly tolerates).
// PT: Seção (5) -- checagem-dominó na própria captura `pending_at_rule_word_` da tabela de modo de
//     at-rule (comentário de cabeçalho do lexer.hpp, "Tabela de modo de at-rule"): um comentário
//     precedido de whitespace entre `@` e o próprio Prelude do at-rule NÃO pode interferir na
//     captura do nome de keyframes -- o token Comment nunca toca
//     `awaiting_at_rule_word_`/`pending_at_rule_word_`, então a flag sobrevive através dele
//     inalterada, igual já sobrevive através de um comentário COLADO ali (comportamento
//     pré-existente, inafetado por este conserto, exercitado aqui com o whitespace que o conserto
//     deste item passa a tolerar).
//
//     ⚠️ DELIBERATE test-source shape, found while writing this exact test: the comment sits GLUED
//     to "keyframes" (zero bytes between `*/` and `keyframes`), NOT `@  /* c */ keyframes` with a
//     trailing space after the comment too. Reason, found by probing the ORIGINAL, unmodified
//     `pending_at_rule_word_` capture algorithm (unrelated to this item's own fix): it splits on the
//     FIRST literal space byte in the raw Prelude text (lexer.hpp header, "At-rule mode table",
//     matching upstream's `pre_token_str.substr(0, pre_token_str.find(' '))` exactly). A Prelude run
//     that itself STARTS with a space (e.g. from `@ keyframes spin {`, no comment involved at all --
//     probed directly, confirmed a PRE-EXISTING characteristic of this unrelated algorithm, not
//     something this item's fix introduces or could fix without widening its own scope) yields an
//     EMPTY first word, so keyframes mode is never entered. This test isolates ONLY the fix's own
//     concern (whitespace BEFORE a comment at a fresh scan start) by keeping the comment glued to
//     the real at-rule word, so it does not accidentally also exercise that separate, pre-existing
//     characteristic.
// PT: ⚠️ Forma de fonte-de-teste DELIBERADA, achada ao escrever este exato teste: o comentário fica
//     COLADO em "keyframes" (zero bytes entre `*/` e "keyframes"), NÃO `@  /* c */ keyframes` com um
//     espaço também depois do comentário. Motivo, achado espiando o próprio algoritmo ORIGINAL,
//     não-modificado, de captura do `pending_at_rule_word_` (não-relacionado ao próprio conserto
//     deste item): ele divide no PRIMEIRO byte de espaço literal do texto cru de Prelude (comentário
//     de cabeçalho do lexer.hpp, "Tabela de modo de at-rule", casando exatamente com
//     `pre_token_str.substr(0, pre_token_str.find(' '))` do upstream). Um trecho de Prelude que ele
//     próprio COMEÇA com um espaço (ex. de `@ keyframes spin {`, nenhum comentário envolvido --
//     espiado diretamente, confirmado uma característica PRÉ-EXISTENTE deste algoritmo
//     não-relacionado, não algo que o conserto deste item introduz ou poderia consertar sem alargar
//     o próprio escopo) produz uma primeira-palavra VAZIA, então o modo keyframes nunca é entrado.
//     Este teste isola SÓ a própria preocupação do conserto (whitespace ANTES de um comentário num
//     início de scan fresco) mantendo o comentário colado na palavra real do at-rule, pra não
//     acidentalmente também exercitar aquela característica separada, pré-existente.
// ---------------------------------------------------------------------------
void test_whitespace_comment_between_at_and_keyframes_prelude_does_not_break_mode_capture() {
  Lexer lex("@ /* c */keyframes spin { from { opacity: 0; } }");
  expect_kind(lex, TokenKind::At, "at-comment-keyframes: At");
  Token c = expect_kind(lex, TokenKind::Comment,
                        "at-comment-keyframes: Comment recognised between "
                        "'@' and the at-rule's own Prelude");
  check_eq(c.text, " c ", "at-comment-keyframes: comment body");
  Token prelude = expect_kind(lex, TokenKind::Prelude, "at-comment-keyframes: Prelude");
  check_eq(prelude.text, "keyframes spin ",
           "at-comment-keyframes: at-rule name+params, unaffected "
           "by the comment before it");
  Token brace = expect_kind(lex, TokenKind::BraceOpen,
                            "at-comment-keyframes: BraceOpen -- must open NESTED Structural mode, "
                            "proving pending_at_rule_word_ == \"keyframes\" survived the Comment");
  (void)brace;
  // EN: If mode dispatch had wrongly fallen to flat Declaration mode, the next token would be a
  //     Declaration ("from { opacity" as a raw name run), not another Prelude -- this assertion is
  //     the concrete proof of which branch was taken, same technique
  //     test_keyframes_match_is_case_sensitive in lexer_tokens_sanity.cpp already uses for the
  //     negative case.
  // PT: Se o despacho de modo tivesse errado caído no modo Declaration plano, o próximo token seria
  //     uma Declaration ("from { opacity" como um trecho-de-nome cru), não outro Prelude -- esta
  //     asserção é a prova concreta de qual ramo foi tomado, mesma técnica que o próprio
  //     test_keyframes_match_is_case_sensitive do lexer_tokens_sanity.cpp já usa pro caso negativo.
  Token from_prelude = expect_kind(lex, TokenKind::Prelude,
                                   "at-comment-keyframes: 'from' Prelude -- proves NESTED "
                                   "Structural mode was entered correctly");
  check_eq(from_prelude.text, " from ", "at-comment-keyframes: 'from' prelude text");
}

} // namespace

int main() {
  test_comment_after_semicolon_preceded_by_space();
  test_comment_after_semicolon_preceded_by_tab();
  test_comment_after_semicolon_preceded_by_newline();
  test_comment_after_semicolon_preceded_by_mixed_whitespace_combo();
  test_comment_after_brace_open_declaration_mode_preceded_by_whitespace();
  test_comment_after_brace_close_structural_mode_top_level_preceded_by_whitespace();
  test_comment_after_brace_open_structural_mode_nested_keyframes_preceded_by_whitespace();
  test_comment_after_brace_close_structural_mode_nested_keyframes_preceded_by_whitespace();
  test_comment_glued_zero_whitespace_still_works_declaration_mode();
  test_comment_glued_zero_whitespace_still_works_structural_mode();
  test_mid_run_comment_inside_declaration_name_still_fuses_deliberately();
  test_mid_run_comment_inside_prelude_splits_deliberately_pre_existing_behaviour();
  test_whitespace_comment_between_at_and_keyframes_prelude_does_not_break_mode_capture();

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_whitespace_comment_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_whitespace_comment_sanity: PASS");
  return 0;
}
