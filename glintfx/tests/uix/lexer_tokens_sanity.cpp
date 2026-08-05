// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- functional/token-shape unit test for glintfx::uix::Lexer. Standalone, no
//     tree, no RmlUi/GLFW/GL -- see glintfx/src/uix/dom/lexer.hpp's own header comment for the
//     full scope/boundary this module holds itself to. Every case below traces back to a
//     specific corpus fact or a specific docs/rmlx-subset.md ("uix-dom.md") decision cited inline
//     -- this is deliberately not "make up plausible XML edge cases", it is "prove the lexer
//     handles what the real corpus needs and nothing this arc has not decided yet".
// PT: RMLX-1/S1 -- teste unit de forma-de-token/funcional pro glintfx::uix::Lexer. Standalone,
//     sem árvore, sem RmlUi/GLFW/GL -- ver o próprio comentário de cabeçalho do
//     glintfx/src/uix/dom/lexer.hpp pro escopo/fronteira completos a que este módulo se prende.
//     Todo caso abaixo remonta a um fato de corpus específico ou uma decisão específica do
//     docs/rmlx-subset.md ("uix-dom.md") citada inline -- deliberadamente isto não é "inventar
//     casos de borda XML plausíveis", é "provar que o lexer trata o que o corpus real precisa e
//     nada que este arco ainda não decidiu".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/lexer.hpp"

#include <cstdio>
#include <string_view>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void check_eq(std::string_view got, std::string_view want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::Lexer;
using glintfx::uix::Token;
using glintfx::uix::TokenKind;

const char* kind_name(TokenKind k) {
  switch (k) {
    case TokenKind::TagOpenStart:
      return "TagOpenStart";
    case TokenKind::Attr:
      return "Attr";
    case TokenKind::TagOpenEnd:
      return "TagOpenEnd";
    case TokenKind::TagSelfClose:
      return "TagSelfClose";
    case TokenKind::TagClose:
      return "TagClose";
    case TokenKind::Text:
      return "Text";
    case TokenKind::Comment:
      return "Comment";
    case TokenKind::EndOfFile:
      return "EndOfFile";
    case TokenKind::Error:
      return "Error";
  }
  return "?";
}

// EN: Advances `lex` one token and checks its `kind`, printing BOTH kinds by name on mismatch --
//     every call site below reads as a flat, ordered list of "the Nth token IS this", which is
//     the point: a reader can diff the expected sequence against the source string right above it
//     without mentally simulating the state machine.
// PT: Avança `lex` um token e checa o `kind`, imprimindo os dois nomes de kind no descasamento --
//     todo call site abaixo lê como uma lista plana e ordenada de "o Nº token É isto", que é o
//     ponto: um leitor consegue comparar a sequência esperada contra a string-fonte logo acima
//     sem simular a máquina de estado de cabeça.
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
// EN: Case 1 -- open + text + close, the absolute minimum shape. Mirrors glintfx/tests/min.rml's
//     own `<body>hello glintfx</body>` structure (measured: `cat glintfx/tests/min.rml`).
// PT: Caso 1 -- abertura + texto + fechamento, a forma mínima absoluta. Espelha a própria
//     estrutura `<body>hello glintfx</body>` do glintfx/tests/min.rml (medido: `cat
//     glintfx/tests/min.rml`).
// ---------------------------------------------------------------------------
void test_open_text_close() {
  Lexer lex("<div>hello</div>");

  Token open = expect_kind(lex, TokenKind::TagOpenStart, "open+text+close: TagOpenStart");
  check_eq(open.text, "div", "open+text+close: tag name");

  Token end = expect_kind(lex, TokenKind::TagOpenEnd, "open+text+close: TagOpenEnd");
  check(end.text.empty(), "open+text+close: TagOpenEnd carries no payload");

  Token text = expect_kind(lex, TokenKind::Text, "open+text+close: Text");
  check_eq(text.text, "hello", "open+text+close: text content");

  Token close = expect_kind(lex, TokenKind::TagClose, "open+text+close: TagClose");
  check_eq(close.text, "div", "open+text+close: close tag name");

  expect_kind(lex, TokenKind::EndOfFile, "open+text+close: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- self-closing tag, no attributes. Corpus form (measured:
//     `grep -on '<br/>' glintfx/tests/*.rml` finds it repeatedly; `<br>` bare-form, HTML5-style,
//     is a ZERO-occurrence case per this module's own header comment and NOT supported).
// PT: Caso 2 -- tag auto-fechada, sem atributos. Forma de corpus (medido:
//     `grep -on '<br/>' glintfx/tests/*.rml` acha repetidamente; a forma crua `<br>`, estilo
//     HTML5, é um caso de ZERO ocorrência pelo próprio comentário de cabeçalho deste módulo e NÃO
//     é suportada).
// ---------------------------------------------------------------------------
void test_self_close_no_attrs() {
  Lexer lex("<br/>");

  Token open = expect_kind(lex, TokenKind::TagOpenStart, "self-close: TagOpenStart");
  check_eq(open.text, "br", "self-close: tag name");

  Token self_close = expect_kind(lex, TokenKind::TagSelfClose, "self-close: TagSelfClose");
  check(self_close.length == 2, "self-close: TagSelfClose spans exactly '/>' (2 bytes)");

  expect_kind(lex, TokenKind::EndOfFile, "self-close: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- attributes, BOTH quote styles in the SAME tag (the brief's own explicit minimum:
//     "aspas simples e duplas"), self-closed. Corpus uses double quotes exclusively today
//     (`grep -oh "[a-zA-Z-]*='[^']*'" glintfx/tests/*.rml` => zero matches) -- single-quote
//     support is a grammar-level requirement from this slice's OWN brief, not corpus-derived, and
//     is tested here explicitly for that reason.
// PT: Caso 3 -- atributos, OS DOIS estilos de aspas na MESMA tag (o próprio mínimo explícito do
//     briefing: "aspas simples e duplas"), auto-fechada. O corpus usa aspas duplas
//     exclusivamente hoje (`grep -oh "[a-zA-Z-]*='[^']*'" glintfx/tests/*.rml` => zero
//     casamentos) -- suporte a aspas simples é um requisito de nível-de-gramática do PRÓPRIO
//     briefing desta fatia, não derivado do corpus, e é testado aqui explicitamente por esse
//     motivo.
// ---------------------------------------------------------------------------
void test_attrs_both_quote_styles() {
  Lexer lex("<input type=\"checkbox\" id='chk' data-if=\"flag\"/>");

  Token open = expect_kind(lex, TokenKind::TagOpenStart, "attrs: TagOpenStart");
  check_eq(open.text, "input", "attrs: tag name");

  Token a1 = expect_kind(lex, TokenKind::Attr, "attrs: Attr #1 (double quotes)");
  check_eq(a1.text, "type", "attrs: attr #1 name");
  check_eq(a1.value, "checkbox", "attrs: attr #1 value");

  Token a2 = expect_kind(lex, TokenKind::Attr, "attrs: Attr #2 (single quotes)");
  check_eq(a2.text, "id", "attrs: attr #2 name");
  check_eq(a2.value, "chk", "attrs: attr #2 value");

  Token a3 = expect_kind(lex, TokenKind::Attr, "attrs: Attr #3 (hyphenated name)");
  check_eq(a3.text, "data-if", "attrs: attr #3 name (hyphen char class)");
  check_eq(a3.value, "flag", "attrs: attr #3 value");

  expect_kind(lex, TokenKind::TagSelfClose, "attrs: TagSelfClose");
  expect_kind(lex, TokenKind::EndOfFile, "attrs: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- attribute value containing a byte that is itself grammar-significant elsewhere
//     (`>`), which must NOT end the tag while inside a quoted value. Real corpus fact: `data-
//     class-wide="hp > 50"` (data_model_embed_scene.rml, cited verbatim in this repo's own
//     glintfx/src/uix/dom/lexer.hpp header comment's corpus citations).
// PT: Caso 4 -- valor de atributo contendo um byte que é ele mesmo significativo-de-gramática em
//     outro lugar (`>`), que NÃO pode encerrar a tag enquanto dentro de um valor entre aspas.
//     Fato real de corpus: `data-class-wide="hp > 50"` (data_model_embed_scene.rml, citado ao pé
//     da letra nas próprias citações de corpus do comentário de cabeçalho do
//     glintfx/src/uix/dom/lexer.hpp deste repo).
// ---------------------------------------------------------------------------
void test_attr_value_contains_gt() {
  Lexer lex("<div data-class-wide=\"hp > 50\">&nbsp;</div>");

  expect_kind(lex, TokenKind::TagOpenStart, "gt-in-value: TagOpenStart");
  Token attr = expect_kind(lex, TokenKind::Attr, "gt-in-value: Attr");
  check_eq(attr.text, "data-class-wide", "gt-in-value: attr name");
  check_eq(attr.value, "hp > 50", "gt-in-value: attr value keeps its literal '>' intact");
  expect_kind(lex, TokenKind::TagOpenEnd, "gt-in-value: TagOpenEnd (the REAL tag-closing '>')");

  // EN: Entity decoding decision (see lexer.hpp header, "Deliberately NOT this module's job"):
  //     `&nbsp;` is dumped RAW, byte-verbatim, exactly as written in the source -- this lexer
  //     never decodes it. If this test ever sees U+00A0 (the DECODED byte, 0xC2 0xA0) instead of
  //     the 6 literal ASCII bytes `&nbsp;`, that is this module silently taking on S2/S3's job,
  //     which uix-dom.md section 6(c) explicitly assigns to "text-node-construction time".
  // PT: Decisão de decodificação de entidade (ver cabeçalho do lexer.hpp, "Deliberadamente NÃO é
  //     trabalho deste módulo"): `&nbsp;` é dumpado CRU, byte-verbatim, exatamente como escrito na
  //     fonte -- este lexer nunca a decodifica. Se este teste algum dia ver U+00A0 (o byte
  //     DECODIFICADO, 0xC2 0xA0) em vez dos 6 bytes ASCII literais `&nbsp;`, isto é este módulo
  //     silenciosamente assumindo o trabalho da S2/S3, que o uix-dom.md seção 6(c) atribui
  //     explicitamente a "momento de construção do nó de texto".
  Token text = expect_kind(lex, TokenKind::Text, "gt-in-value: Text (entity RAW, undecoded)");
  check_eq(text.text, "&nbsp;", "gt-in-value: '&nbsp;' passes through as 6 literal ASCII bytes");

  expect_kind(lex, TokenKind::TagClose, "gt-in-value: TagClose");
  expect_kind(lex, TokenKind::EndOfFile, "gt-in-value: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- comment. Corpus form: `<!-- SPDX-License-Identifier: Apache-2.0 -->` (the very
//     first line of most `.rml` fixtures in this repo, e.g. data_model_embed_scene.rml).
// PT: Caso 5 -- comentário. Forma de corpus: `<!-- SPDX-License-Identifier: Apache-2.0 -->` (a
//     primeiríssima linha da maioria das fixtures `.rml` deste repo, ex.:
//     data_model_embed_scene.rml).
// ---------------------------------------------------------------------------
void test_comment() {
  Lexer lex("<!-- SPDX-License-Identifier: Apache-2.0 --><div/>");

  Token comment = expect_kind(lex, TokenKind::Comment, "comment: Comment");
  check_eq(comment.text, " SPDX-License-Identifier: Apache-2.0 ", "comment: raw content, verbatim");

  expect_kind(lex, TokenKind::TagOpenStart, "comment: TagOpenStart after comment");
  expect_kind(lex, TokenKind::TagSelfClose, "comment: TagSelfClose");
  expect_kind(lex, TokenKind::EndOfFile, "comment: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- a comment body itself containing a double-hyphen run that is NOT the closing
//     `-->` (the pt-br em-dash convention this repo's own comments use constantly, e.g.
//     "`white-space: nowrap` -- same get_element_box()-oracle convention", verified NOT to false-
//     terminate: `grep -rn -- '--' glintfx/tests/*.rml | grep -v '<!--\|-->'` shows this exact
//     pattern occurring inside real comments today).
// PT: Caso 6 -- um corpo de comentário contendo ele mesmo um trecho de hífen-duplo que NÃO é o
//     `-->` de fechamento (a convenção de travessão pt-br que os próprios comentários deste repo
//     usam o tempo todo, ex.: "`white-space: nowrap` -- mesma convenção de oráculo
//     get_element_box()", verificado como NÃO falso-terminando: `grep -rn -- '--'
//     glintfx/tests/*.rml | grep -v '<!--\|-->'` mostra este padrão exato ocorrendo dentro de
//     comentários reais hoje).
// ---------------------------------------------------------------------------
void test_comment_with_embedded_double_hyphen() {
  Lexer lex("<!-- a -- b -- c -->");
  Token comment = expect_kind(lex, TokenKind::Comment, "embedded double-hyphen: Comment");
  check_eq(comment.text, " a -- b -- c ",
           "embedded double-hyphen: only the LITERAL '-->' terminates, not any '--' run");
  expect_kind(lex, TokenKind::EndOfFile, "embedded double-hyphen: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- <head> is tokenized GENERICALLY, no opacity special-casing (see lexer.hpp header,
//     "Deliberately NOT this module's job" -- the HEAD-opacity decision belongs to S3). Mirrors
//     the exact worked example from docs/rmlx-subset.md's sibling doc, docs/uix-dom.md section
//     11, MINUS entity decoding and attribute sorting (both explicitly S2/S3+ jobs, not this
//     lexer's) -- this test's own expected sequence is therefore this lexer's HONEST answer to
//     that worked example, not a copy of its `HEAD PRESENT`/sorted-`CLASS` dump lines.
// PT: Caso 7 -- `<head>` é tokenizado GENERICAMENTE, sem tratamento especial de opacidade (ver
//     cabeçalho do lexer.hpp, "Deliberadamente NÃO é trabalho deste módulo" -- a decisão de
//     opacidade de HEAD é da S3). Espelha o exemplo trabalhado exato do doc irmão do
//     docs/rmlx-subset.md, o docs/uix-dom.md seção 11, MENOS decodificação de entidade e
//     ordenação de atributo (os dois explicitamente trabalho da S2/S3+, não deste lexer) -- a
//     própria sequência esperada deste teste é portanto a resposta HONESTA deste lexer àquele
//     exemplo trabalhado, não uma cópia das linhas de dump `HEAD PRESENT`/`CLASS` ordenado dele.
// ---------------------------------------------------------------------------
void test_head_is_not_special_cased() {
  Lexer lex(
      "<rml>\n"
      "<head>\n"
      "<style>body{color:white}</style>\n"
      "</head>\n"
      "<body>\n"
      "<div id=\"panel\" class=\"wide highlighted\" data-if=\"flag\" title=\"Panel\">\n"
      "  <span class=\"highlighted wide\">Hi&nbsp;there</span>\n"
      "</div>\n"
      "</body>\n"
      "</rml>");

  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <rml");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: rml >");
  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before <head>");

  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <head");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: head >");
  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before <style>");

  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <style");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: style >");
  Token css = expect_kind(lex, TokenKind::Text, "head-not-opaque: raw CSS text, tokenized as Text");
  check_eq(css.text, "body{color:white}",
           "head-not-opaque: <style>'s content is an ordinary Text token, NOT an opaque HEAD blob "
           "(that opacity is uix-dom.md's own S3-level decision, not this lexer's)");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </style>");

  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before </head>");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </head>");

  expect_kind(lex, TokenKind::Text,
              "head-not-opaque: '\\n' before <body> -- WHITESPACE-ONLY, "
              "emitted anyway, see uix-dom.md 6(a) note below");
  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <body");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: body >");
  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before <div>");

  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <div");
  // EN: Attribute order preserved EXACTLY as written in source (id, class, data-if, title) -- NOT
  //     re-sorted. uix-dom.md section 7's byte-wise sort is a DUMP-FORMAT policy for a later
  //     wave's oracle output, not this lexer's job; asserting source order here locks that this
  //     lexer never silently starts doing S6's job.
  // PT: Ordem de atributo preservada EXATAMENTE como escrita na fonte (id, class, data-if, title)
  //     -- NÃO reordenada. A ordenação byte-a-byte da seção 7 do uix-dom.md é política de
  //     FORMATO-DE-DUMP pra uma onda posterior, não trabalho deste lexer; verificar a ordem-fonte
  //     aqui trava que este lexer nunca começa a fazer o trabalho da S6 em silêncio.
  Token a_id = expect_kind(lex, TokenKind::Attr, "head-not-opaque: div attr #1");
  check_eq(a_id.text, "id", "head-not-opaque: attr #1 is 'id' (source order, not alphabetical)");
  check_eq(a_id.value, "panel", "head-not-opaque: id value");
  Token a_class = expect_kind(lex, TokenKind::Attr, "head-not-opaque: div attr #2");
  check_eq(a_class.text, "class", "head-not-opaque: attr #2 is 'class'");
  check_eq(a_class.value, "wide highlighted",
           "head-not-opaque: class value NOT split/sorted/deduped -- raw string (that is uix-dom.md "
           "section 7's job, not this lexer's)");
  Token a_if = expect_kind(lex, TokenKind::Attr, "head-not-opaque: div attr #3");
  check_eq(a_if.text, "data-if", "head-not-opaque: attr #3 is 'data-if'");
  Token a_title = expect_kind(lex, TokenKind::Attr, "head-not-opaque: div attr #4");
  check_eq(a_title.text, "title",
           "head-not-opaque: attr #4 is 'title' (LAST in source, not sorted "
           "after 'data-if' as byte-wise sort would put it before)");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: div >");

  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n  ' before <span>");
  expect_kind(lex, TokenKind::TagOpenStart, "head-not-opaque: <span");
  Token span_class = expect_kind(lex, TokenKind::Attr, "head-not-opaque: span class");
  check_eq(span_class.value, "highlighted wide", "head-not-opaque: span class value");
  expect_kind(lex, TokenKind::TagOpenEnd, "head-not-opaque: span >");
  Token span_text = expect_kind(lex, TokenKind::Text, "head-not-opaque: span text");
  check_eq(span_text.text, "Hi&nbsp;there", "head-not-opaque: entity raw, undecoded");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </span>");

  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before </div>");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </div>");
  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before </body>");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </body>");
  expect_kind(lex, TokenKind::Text, "head-not-opaque: '\\n' before </rml>");
  expect_kind(lex, TokenKind::TagClose, "head-not-opaque: </rml>");
  expect_kind(lex, TokenKind::EndOfFile, "head-not-opaque: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- tag name grammar allows a trailing digit (`h1`..`h6`, the UA-stylesheet's own
//     structural-element set, glintfx/src/ua_stylesheet.hpp) but the name-START class does not
//     allow a LEADING digit -- see lexer_hardening_sanity.cpp for the leading-digit rejection
//     case; this test proves the ACCEPT side of that same character-class boundary.
// PT: Caso 8 -- a gramática de nome de tag permite dígito ao FINAL (`h1`..`h6`, o próprio
//     conjunto de elementos estruturais do UA-stylesheet, glintfx/src/ua_stylesheet.hpp), mas a
//     classe de INÍCIO de nome não permite dígito NO INÍCIO -- ver lexer_hardening_sanity.cpp pro
//     caso de rejeição de dígito-inicial; este teste prova o lado de ACEITE dessa mesma fronteira
//     de classe de caractere.
// ---------------------------------------------------------------------------
void test_tag_name_trailing_digit() {
  Lexer lex("<h1>Title</h1>");
  Token open = expect_kind(lex, TokenKind::TagOpenStart, "trailing-digit: TagOpenStart");
  check_eq(open.text, "h1", "trailing-digit: tag name 'h1' accepted whole");
  expect_kind(lex, TokenKind::TagOpenEnd, "trailing-digit: TagOpenEnd");
  expect_kind(lex, TokenKind::Text, "trailing-digit: Text");
  Token close = expect_kind(lex, TokenKind::TagClose, "trailing-digit: TagClose");
  check_eq(close.text, "h1", "trailing-digit: close tag name 'h1'");
  expect_kind(lex, TokenKind::EndOfFile, "trailing-digit: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- mismatched-but-syntactically-valid open/close tag NAMES. This lexer does NOT
//     validate that a TagClose's name matches the innermost open TagOpenStart's name -- that
//     requires a stack, which requires a tree (S2/S3). Proving this ACCEPTS (rather than rejects)
//     locks the boundary in explicitly, so a reader of this test file sees the decision, not just
//     infers it from the source's silence.
// PT: Caso 9 -- NOMES de tag de abertura/fechamento descasados mas sintaticamente válidos. Este
//     lexer NÃO valida que o nome de um TagClose casa com o nome do TagOpenStart aberto mais
//     interno -- isso exige uma pilha, que exige uma árvore (S2/S3). Provar que isto é ACEITO (em
//     vez de rejeitado) trava a fronteira explicitamente, pra um leitor deste arquivo de teste ver
//     a decisão, não só inferi-la do silêncio da fonte.
// ---------------------------------------------------------------------------
void test_mismatched_close_name_not_validated() {
  Lexer lex("<div></span>");
  Token open = expect_kind(lex, TokenKind::TagOpenStart, "mismatched-close: TagOpenStart");
  check_eq(open.text, "div", "mismatched-close: open tag name");
  expect_kind(lex, TokenKind::TagOpenEnd, "mismatched-close: TagOpenEnd");
  Token close = expect_kind(lex, TokenKind::TagClose,
                            "mismatched-close: TagClose still emitted, NOT an Error");
  check_eq(close.text, "span",
           "mismatched-close: close tag name 'span' != open tag name 'div' -- accepted anyway, "
           "name-matching is S3's job (needs a stack), not the lexer's");
  expect_kind(lex, TokenKind::EndOfFile, "mismatched-close: EndOfFile");
}

} // namespace

int main() {
  test_open_text_close();
  test_self_close_no_attrs();
  test_attrs_both_quote_styles();
  test_attr_value_contains_gt();
  test_comment();
  test_comment_with_embedded_double_hyphen();
  test_head_is_not_special_cased();
  test_tag_name_trailing_digit();
  test_mismatched_close_name_not_validated();

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_tokens_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_tokens_sanity: PASS");
  return 0;
}
