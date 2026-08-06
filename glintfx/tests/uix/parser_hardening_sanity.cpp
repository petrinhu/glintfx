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

// EN: Same shape as parser_tokens_sanity.cpp's own check_eq() -- added here (UIX-ENTITY-PARIDADE)
//     because this file's new tolerance tests need to assert exact decoded TEXT content, not just
//     accept/reject.
// PT: Mesma forma do próprio check_eq() do parser_tokens_sanity.cpp -- somado aqui
//     (UIX-ENTITY-PARIDADE) porque os novos testes de tolerância deste arquivo precisam afirmar o
//     conteúdo TEXT decodificado exato, não só aceita/rejeita.
void check_eq(std::string_view got, std::string_view want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::as_text;
using glintfx::uix::Element;
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

// EN: Overload for a caller that already HAS a `ParseResult` and wants to inspect the tree
//     afterwards (the `source`-taking overload above throws the tree away) -- same shape as
//     parser_tokens_sanity.cpp's own `expect_ok(ParseResult&, const char*)`, added here
//     (UIX-ENTITY-PARIDADE) for this file's new decoded-content assertions. Returns whether it is
//     safe to keep inspecting `r.document` (false on failure, after already recording it).
// PT: Sobrecarga pra um chamador que já TEM um `ParseResult` e quer inspecionar a árvore depois
//     (a sobrecarga que recebe `source` acima descarta a árvore) -- mesma forma do próprio
//     `expect_ok(ParseResult&, const char*)` do parser_tokens_sanity.cpp, somada aqui
//     (UIX-ENTITY-PARIDADE) pras novas asserções de conteúdo decodificado deste arquivo. Devolve
//     se é seguro seguir inspecionando `r.document` (false na falha, já registrada).
bool expect_ok(ParseResult& r, const char* what) {
  if (r.error.has_value()) {
    std::fprintf(stderr, "FAIL: %s (unexpected ParseError at %zu:%zu: %s)\n", what,
                 r.error->line, r.error->column, r.error->message.c_str());
    ++g_failures;
    return false;
  }
  check(r.document != nullptr, what);
  return r.document != nullptr;
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

// ---------------------------------------------------------------------------
// EN: UIX-ENTITY-PARIDADE (2026-08, RMLX-1) -- entity syntax this parser used to FAIL-HIGH on is
//     no longer rejected: upstream's own `StringUtilities::DecodeRml`
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:128-218) NEVER fails for any of these forms
//     -- every one of its four fixed literal-character checks either matches and decodes, or the
//     WHOLE function falls through to its own final `result += s[i]; i += 1;`
//     (StringUtilities.cpp:214-215), copying the lone '&' byte literally and resuming right after
//     it. A real fixture GusWorld measured (`system_menu__config_controles_tabela.rml`, a rótulo
//     "Interagir & Diálogo" with a bare literal '&') loads fine in RmlUi today and would have
//     failed to load in THIS parser at the old fail-high boundary -- exactly the parity break
//     UIX-ENTITY-PARIDADE closes. See decode_entities()'s own header comment
//     (glintfx/src/uix/dom/parser.cpp) for the full construction-by-construction proof against
//     the upstream source.
// PT: UIX-ENTITY-PARIDADE (2026-08, RMLX-1) -- sintaxe de entidade que este parser costumava
//     FAIL-HIGH não é mais rejeitada: o próprio `StringUtilities::DecodeRml` do upstream
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:128-218) NUNCA falha pra nenhuma destas
//     formas -- cada uma das quatro checagens de caractere-literal fixas dele ou casa e decodifica,
//     ou a função INTEIRA cai pro próprio `result += s[i]; i += 1;` final
//     (StringUtilities.cpp:214-215), copiando o byte '&' solitário literalmente e retomando logo
//     depois dele. Uma fixture real que o GusWorld mediu
//     (`system_menu__config_controles_tabela.rml`, um rótulo "Interagir & Diálogo" com um '&'
//     literal cru) carrega normal no RmlUi hoje e teria falhado ao carregar NESTE parser na
//     antiga fronteira fail-high -- exatamente a quebra de paridade que a UIX-ENTITY-PARIDADE
//     fecha. Ver o próprio comentário de cabeçalho do decode_entities()
//     (glintfx/src/uix/dom/parser.cpp) pra prova completa, construção por construção, contra o
//     fonte upstream.
// ---------------------------------------------------------------------------
void test_entity_tolerance() {
  // EN: The exact real-world shape (bare '&' in prose, no entity intended at all) -- the fixture
  //     that drove this slice.
  // PT: A forma exata do mundo real (um '&' cru em prosa, entidade nenhuma pretendida) -- a
  //     fixture que dirigiu esta fatia.
  {
    ParseResult r = parse_document(
        "<rml><body><div>Interagir &amp; Di\xC3\xA1logo</div><div>Interagir & "
        "Di\xC3\xA1logo</div></body></rml>");
    expect_ok(r, "accept: bare '&' in text, GusWorld's own measured shape");
  }

  expect_ok("<rml><body>bad &foo; entity</body></rml>",
            "accept: unrecognised named entity falls through literal, matching DecodeRml");
  expect_ok("<rml><body>AT&T no semicolon</body></rml>",
            "accept: bare '&' never closed by ';' falls through literal");
  expect_ok("<rml><body>&;</body></rml>", "accept: empty entity name '&;' falls through literal");
  expect_ok("<rml><body>&#x110000;</body></rml>",
            "accept: numeric entity above the Unicode max (0x10FFFF) falls through literal "
            "(known, argued byte-content divergence from upstream -- see decode_entities() "
            "header)");
  expect_ok("<rml><body>&#xD800;</body></rml>",
            "accept: numeric entity in the UTF-16 surrogate range falls through literal (same "
            "known, argued divergence)");
  expect_ok("<rml><body>&#0;</body></rml>",
            "accept: numeric entity for NUL (codepoint 0) falls through literal -- this one IS "
            "byte-parity with DecodeRml (its own `code_point != 0` guard routes here too)");
  expect_ok("<rml><body><div title=\"bad &foo; entity\">x</div></body></rml>",
            "accept: unrecognised entity inside an ATTRIBUTE VALUE too, not just text");

  // EN: An unrecognised name is NOT a licence to swallow a real entity right after it -- the
  //     fallback consumes ONLY the lone '&', never the whole scanned span up to a found ';', so a
  //     genuine `&amp;` immediately after a malformed `&foo` (no ';' before it) still decodes.
  //     Mirrors DecodeRml's own per-character resumption (StringUtilities.cpp:215, `i += 1`, not
  //     "skip past whatever this position's failed match consumed").
  // PT: Um nome não-reconhecido NÃO é licença pra engolir uma entidade real logo depois -- o
  //     fallback consome SÓ o '&' solitário, nunca o span inteiro escaneado até um ';' achado,
  //     então um `&amp;` genuíno logo após um `&foo` malformado (sem ';' antes) ainda decodifica.
  //     Espelha a própria retomada por-caractere do DecodeRml (StringUtilities.cpp:215, `i += 1`,
  //     não "pula o que quer que esta posição tenha consumido na tentativa falha").
  {
    ParseResult r = parse_document("<rml><body>&foo&amp;bar;</body></rml>");
    if (!expect_ok(r, "accept: malformed entity immediately followed by a real one")) return;
    Element& body = r.document->body();
    auto* text = glintfx::uix::as_text(body.children()[0].get());
    check(text != nullptr, "fallback-resync: child is a Text node");
    if (text != nullptr) {
      check_eq(text->content(), "&foo&bar;",
               "fallback-resync: only the lone '&' is swallowed by the failed match, the real "
               "&amp; right after it still decodes");
    }
  }

  // EN: `apos` and `nbsp` are BOTH REMOVED from the recognised table (UIX-ENTITY-PARIDADE) --
  //     DecodeRml never had either (see decode_entities()/decode_named_entity() headers; the
  //     `nbsp` removal specifically closes the SECOND, more treacherous parity break this slice
  //     found, confirmed against a live RmlUi run by another agent's corrected docs/uix-dom.md
  //     section 6c after this slice's own first pass had ARGUED to keep it decoding -- see
  //     decode_named_entity()'s own header for the full history). `&apos;`/`&nbsp;` now BOTH fall
  //     through as literal text, same as any other unrecognised name -- `&nbsp;` survives as its
  //     own SIX literal ASCII bytes, never U+00A0.
  // PT: `apos` E `nbsp` foram AMBOS REMOVIDOS da tabela reconhecida (UIX-ENTITY-PARIDADE) -- o
  //     DecodeRml nunca teve nenhum dos dois (ver cabeçalhos do decode_entities()/
  //     decode_named_entity(); a remoção de `nbsp` especificamente fecha a SEGUNDA quebra de
  //     paridade, mais traiçoeira, que esta fatia achou, confirmada contra uma rodada ao vivo do
  //     RmlUi pelo docs/uix-dom.md corrigido de outro agente da seção 6c depois da PRIMEIRA
  //     passada desta fatia ter ARGUMENTADO por manter decodificando -- ver o próprio cabeçalho
  //     do decode_named_entity() pro histórico completo). `&apos;`/`&nbsp;` agora caem AMBOS como
  //     texto literal, igual qualquer outro nome não-reconhecido -- `&nbsp;` sobrevive como os
  //     próprios SEIS bytes ASCII literais, nunca U+00A0.
  {
    ParseResult r = parse_document("<rml><body>&apos;&nbsp;</body></rml>");
    if (!expect_ok(r, "accept: &apos; and &nbsp; no longer decoded")) return;
    Element& body = r.document->body();
    auto* text = glintfx::uix::as_text(body.children()[0].get());
    check(text != nullptr, "apos-nbsp-literal: child is a Text node");
    if (text != nullptr) {
      check_eq(text->content(), "&apos;&nbsp;",
               "apos-nbsp-literal: both pass through verbatim (matches DecodeRml, which has no "
               "apos/nbsp branch -- 'n' matches none of 'l'/'g'/'a'/'q'), never U+00A0/a quote");
    }
  }

  // EN: The accept side of the STILL-RECOGNISED forms -- proves the table narrowing didn't break
  //     what upstream really does decode. `&#160;` (the NUMERIC spelling of the exact same
  //     codepoint `&nbsp;` used to alias) is the replacement this suite now uses wherever a real
  //     U+00A0 byte sequence is actually wanted -- it IS one of DecodeRml's five recognised forms
  //     (a numeric character reference), unlike `&nbsp;`.
  // PT: O lado aceita das formas AINDA-RECONHECIDAS -- prova que o estreitamento da tabela não
  //     quebrou o que o upstream de fato decodifica. `&#160;` (a grafia NUMÉRICA do exato mesmo
  //     codepoint que `&nbsp;` costumava apelidar) é a substituição que esta suíte agora usa onde
  //     um byte U+00A0 de verdade é de fato desejado -- ELE é uma das cinco formas reconhecidas do
  //     DecodeRml (uma referência numérica de caractere), diferente de `&nbsp;`.
  {
    ParseResult r = parse_document("<rml><body>&amp;&lt;&gt;&quot;&#160;&#65;&#x41;</body></rml>");
    if (!expect_ok(r,
                   "accept: every still-recognised entity form, named + numeric decimal + "
                   "numeric hex, including &#160; as the real U+00A0 spelling")) {
      return;
    }
    Element& body = r.document->body();
    auto* text = glintfx::uix::as_text(body.children()[0].get());
    check(text != nullptr, "still-recognised: child is a Text node");
    if (text != nullptr) {
      check_eq(text->content(),
               "&<>\"\xC2\xA0"
               "AA",
               "still-recognised: &#160; decodes to the real 2-byte U+00A0 (C2 A0), unlike "
               "&nbsp; above");
    }
  }
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
// EN: 🔴 `UIX-LEXER-OPACO` -- FORMERLY A KNOWN GAP, NOW FIXED AT THE LEXER LAYER. THIS TEST USED
//     TO BE `test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt`, and its own comment
//     said: "if this ever starts PASSING, the gap was fixed and this pin's own comment/report is
//     stale, go update them" -- IT STARTED PASSING (2026-08-06). This is that update, not a
//     silent deletion: the OLD gap (glintfx::uix::Lexer, run STANDALONE over a whole document,
//     could not tokenize `<style>`/`<script>` content containing a literal, un-escaped '<' -- real
//     GusWorld RCSS prose, "~128dp << 228dp", pt-br for "much less than") is CLOSED. See
//     glintfx/src/uix/dom/lexer.hpp's own header comment, "RESOLVED (UIX-LEXER-OPACO)" paragraph,
//     for the fix's full argument, upstream citations (`Factory.cpp:255-257`,
//     `BaseXMLParser.cpp:243-262,349-390`), and declared scope teto (exactly "style"/"script",
//     ASCII case-folded tag-name match, literal non-tag-name-verified closing search -- converged
//     with `dom_dump.cpp`'s own `locate_head_tag_span()` teto for the identical upstream
//     mechanism).
//
//     THIS TEST NOW PROVES BOTH HALVES ARE GREEN FOR THE SAME REASON, SIDE BY SIDE, SO NEITHER
//     REGRESSION CAN GO UNNOTICED:
//       (1) the bare Lexer, run standalone start to finish (exactly what
//           lexer_corpus_sanity.cpp does, which as of this same fix sweeps this arc's own third
//           fixture directory too -- see that file's own header comment), now tokenizes the
//           `<<` cleanly to EndOfFile -- the CDATA-mode fix in lexer.cpp, not a change to this
//           parser;
//       (2) THIS parser (`parse_document`) was ALWAYS immune when the same content sits inside
//           `<head>` (its universal real-world position -- 100% of the corpus) -- parser.cpp's
//           own `<head>`-opacity raw-byte-scan (see parser.hpp header) never ran the Lexer over
//           `<head>`'s interior AT ALL, before or after this fix, and the dump format this
//           repo's differential oracle checks byte-for-byte is UNCHANGED by it (see this fix's
//           own commit message for the measured `ctest -R differential` SCOPE line, green both
//           before and after).
//     Kept as its own dedicated test (not folded into another) for the same reason the ORIGINAL
//     pin was dedicated: a reader should be able to find "does the corpus's `<<` case work" as
//     ONE named thing, without reconstructing it from a general corpus sweep.
// PT: 🔴 `UIX-LEXER-OPACO` -- ANTES UMA LACUNA CONHECIDA, AGORA CONSERTADA NA CAMADA DO LEXER.
//     Este teste costumava se chamar
//     `test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt`, e o próprio comentário dele
//     dizia: "se isto algum dia começar a PASSAR, a lacuna foi consertada e este pin's próprio
//     comentário/relato ficou velho, vá atualizá-los" -- COMEÇOU A PASSAR (2026-08-06). Esta é
//     essa atualização, não uma deleção silenciosa: a lacuna ANTIGA (glintfx::uix::Lexer, rodando
//     STANDALONE sobre um documento inteiro, não conseguia tokenizar conteúdo de
//     `<style>`/`<script>` contendo um '<' literal, não-escapado -- prosa RCSS real do GusWorld,
//     "~128dp << 228dp", pt-br pra "muito menor que") está FECHADA. Ver o próprio comentário de
//     cabeçalho do glintfx/src/uix/dom/lexer.hpp, parágrafo "RESOLVED (UIX-LEXER-OPACO)", pro
//     argumento completo do conserto, citações do upstream (`Factory.cpp:255-257`,
//     `BaseXMLParser.cpp:243-262,349-390`), e o teto de escopo declarado (exatamente
//     "style"/"script", casamento de nome-de-tag dobrado por ASCII, busca de fechamento literal
//     sem verificação-de-nome -- convergida com o próprio teto do locate_head_tag_span() do
//     dom_dump.cpp pro mecanismo upstream idêntico).
//
//     ESTE TESTE AGORA PROVA AS DUAS METADES VERDES PELO MESMO MOTIVO, LADO A LADO, PRA NENHUMA
//     REGRESSÃO PASSAR DESPERCEBIDA:
//       (1) o Lexer cru, rodando standalone do início ao fim (exatamente o que o
//           lexer_corpus_sanity.cpp faz, que a partir deste mesmo conserto também varre o próprio
//           terceiro diretório de fixture desta fatia -- ver o próprio comentário de cabeçalho
//           daquele arquivo), agora tokeniza o `<<` limpo até EndOfFile -- o conserto de modo-CDATA
//           no lexer.cpp, não uma mudança neste parser;
//       (2) ESTE parser (`parse_document`) SEMPRE foi imune quando o mesmo conteúdo está dentro de
//           `<head>` (a posição universal dele no mundo real -- 100% do corpus) -- o próprio scan
//           cru de byte de opacidade de `<head>` do parser.cpp (ver cabeçalho do parser.hpp) nunca
//           rodou o Lexer sobre o interior de `<head>` NENHUMA VEZ, antes ou depois deste conserto,
//           e o formato de dump que o oráculo diferencial deste repo confere byte-a-byte é
//           INALTERADO por ele (ver a própria mensagem de commit deste conserto pra linha SCOPE
//           medida do `ctest -R differential`, verde antes E depois).
//     Mantido como teste próprio dedicado (não dobrado em outro) pelo mesmo motivo do pin
//     ORIGINAL ser dedicado: um leitor deve conseguir achar "o caso `<<` do corpus funciona" como
//     UMA coisa nomeada, sem reconstruir isso a partir de uma varredura geral de corpus.
// ---------------------------------------------------------------------------
void test_style_stray_lt_tokenizes_and_parses_cleanly() {
  using glintfx::uix::Lexer;
  using glintfx::uix::Token;
  using glintfx::uix::TokenKind;

  const char* kStyleWithStrayLt =
      "<rml><head><style>#x { width: 110dp; } /* 128dp << 228dp */</style></head>"
      "<body>ok</body></rml>";

  // (1) The bare Lexer, tokenizing generically start to finish, now reaches a clean EndOfFile --
  //     the `<<` inside <style> is CDATA-like content (UIX-LEXER-OPACO), never re-entered as tag
  //     grammar.
  Lexer lex(kStyleWithStrayLt);
  Token tok;
  bool saw_error = false;
  int drained = 0;
  do {
    tok = lex.next();
    ++drained;
  } while (tok.kind != TokenKind::EndOfFile && tok.kind != TokenKind::Error && drained < 1000);
  saw_error = (tok.kind == TokenKind::Error);
  check(!saw_error,
        "UIX-LEXER-OPACO: bare Lexer standalone now tokenizes '<<' inside <style> content "
        "cleanly to EndOfFile, no Error -- the fixed gap");
  check(tok.kind == TokenKind::EndOfFile,
        "UIX-LEXER-OPACO: bare Lexer reaches a clean EndOfFile, not stuck mid-stream");

  // (2) THIS parser remains immune -- the exact same bytes, inside <head>, parse cleanly, because
  //     handle_head()'s raw-byte-scan never re-tokenizes <head>'s interior with the Lexer. This
  //     half was ALREADY true before UIX-LEXER-OPACO; kept here so a reader sees both halves
  //     agree for the SAME construction, not just "the parser happens to also pass".
  expect_ok(kStyleWithStrayLt,
            "UIX-LEXER-OPACO: THIS parser remains immune -- '<<' inside <head>'s opaque <style> "
            "payload never reached any tokenizer at all, before or after this fix");
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
  test_entity_tolerance();
  test_depth_ceiling();
  test_style_stray_lt_tokenizes_and_parses_cleanly();

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_hardening_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_hardening_sanity: PASS");
  return 0;
}
