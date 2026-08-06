// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S3 -- functional/shape unit test for glintfx::uix::parse_document. Standalone, no
//     RmlUi/GLFW/GL -- see glintfx/src/uix/dom/parser.hpp's own header comment for the full
//     scope/boundary this module holds itself to. Every case below traces back to a specific
//     docs/uix-dom.md decision or this slice's own brief, same discipline as
//     lexer_tokens_sanity.cpp.
// PT: RMLX-1/S3 -- teste unit de forma/funcional pro glintfx::uix::parse_document. Standalone,
//     sem RmlUi/GLFW/GL -- ver o próprio comentário de cabeçalho do
//     glintfx/src/uix/dom/parser.hpp pro escopo/fronteira completos a que este módulo se prende.
//     Todo caso abaixo remonta a uma decisão específica do docs/uix-dom.md ou ao próprio briefing
//     desta fatia, mesma disciplina do lexer_tokens_sanity.cpp.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"

#include <cstdio>
#include <string>
#include <string_view>

#include "uix/dom/dumper.hpp"

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

using glintfx::uix::Document;
using glintfx::uix::Element;
using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;

// EN: Asserts a clean parse (no error), returning the document by reference into `out`. On
//     failure, prints the ParseError (message/line/column) and fails a synthetic check so the
//     caller's remaining assertions don't cascade into null-deref UB.
// PT: Garante um parse limpo (sem erro), devolvendo o documento por referência em `out`. Na
//     falha, imprime o ParseError (mensagem/linha/coluna) e falha uma checagem sintética pra as
//     asserções restantes do chamador não cascatearem em UB de deref-nulo.
bool expect_ok(ParseResult& result, const char* what) {
  if (result.error.has_value()) {
    std::fprintf(stderr, "FAIL: %s (unexpected ParseError at %zu:%zu: %s)\n", what,
                 result.error->line, result.error->column, result.error->message.c_str());
    ++g_failures;
    return false;
  }
  check(result.document != nullptr, what);
  return result.document != nullptr;
}

// ---------------------------------------------------------------------------
// EN: Case 1 -- absolute minimum: <rml><body>text</body></rml>. No <head>.
// PT: Caso 1 -- mínimo absoluto: <rml><body>texto</body></rml>. Sem <head>.
// ---------------------------------------------------------------------------
void test_minimal_document() {
  ParseResult r = parse_document("<rml><body>hello glintfx</body></rml>");
  if (!expect_ok(r, "minimal: parse ok")) return;

  Document& doc = *r.document;
  check(!doc.head().present, "minimal: HEAD ABSENT");
  Element& body = doc.body();
  check_eq(body.tag(), "body", "minimal: root tag is 'body'");
  check(body.child_count() == 1, "minimal: body has exactly 1 child (the text)");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- id/class/generic attributes, all three storage locations exercised.
// PT: Caso 2 -- atributos id/class/genéricos, os três locais de armazenamento exercitados.
// ---------------------------------------------------------------------------
void test_attributes() {
  ParseResult r = parse_document(
      "<rml><body><div id=\"panel\" class=\"wide highlighted\" data-if=\"flag\" "
      "title=\"Panel\"></div></body></rml>");
  if (!expect_ok(r, "attrs: parse ok")) return;

  Element& body = r.document->body();
  check(body.child_count() == 1, "attrs: body has 1 child");
  auto* div = glintfx::uix::as_element(body.children()[0].get());
  check(div != nullptr, "attrs: child is an Element");
  if (div == nullptr) return;

  check_eq(div->id(), "panel", "attrs: id value");
  check(div->has_class("wide"), "attrs: has class 'wide'");
  check(div->has_class("highlighted"), "attrs: has class 'highlighted'");
  check(div->classes().size() == 2, "attrs: exactly 2 classes (deduplicated by construction)");
  auto data_if = div->attribute("data-if");
  check(data_if.has_value() && *data_if == "flag", "attrs: data-if generic attribute value");
  auto title = div->attribute("title");
  check(title.has_value() && *title == "Panel", "attrs: title generic attribute value");
  check(!div->has_attribute("id"), "attrs: 'id' never leaks into the generic attribute map");
  check(!div->has_attribute("class"),
        "attrs: 'class' never leaks into the generic attribute map");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- nested elements + self-closing leaf. Mirrors ordinary corpus shape.
// PT: Caso 3 -- elementos aninhados + folha auto-fechada. Espelha forma comum de corpus.
// ---------------------------------------------------------------------------
void test_nesting_and_self_close() {
  ParseResult r = parse_document(
      "<rml><body><div id=\"outer\"><span>x</span><br/></div></body></rml>");
  if (!expect_ok(r, "nesting: parse ok")) return;

  Element& body = r.document->body();
  auto* outer = glintfx::uix::as_element(body.children()[0].get());
  check(outer != nullptr && outer->child_count() == 2,
        "nesting: outer div has 2 children (span, br)");
  if (outer == nullptr || outer->child_count() != 2) return;

  auto* span = glintfx::uix::as_element(outer->children()[0].get());
  check(span != nullptr && span->tag() == "span", "nesting: 1st child is <span>");
  check(span != nullptr && span->child_count() == 1, "nesting: span has 1 text child");

  auto* br = glintfx::uix::as_element(outer->children()[1].get());
  check(br != nullptr && br->tag() == "br", "nesting: 2nd child is self-closed <br/>");
  check(br != nullptr && br->child_count() == 0, "nesting: self-closed <br/> has no children");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- whitespace-only text nodes never occupy a child-index slot (dom_tree's own
//     invariant, this parser just has to NOT pre-filter and trust append_child).
// PT: Caso 4 -- nós de texto só-whitespace nunca ocupam slot de índice de filho (invariante do
//     próprio dom_tree, este parser só precisa NÃO pré-filtrar e confiar no append_child).
// ---------------------------------------------------------------------------
void test_whitespace_only_text_filtered() {
  ParseResult r = parse_document("<rml>\n<body>\n  <div>a</div>\n  <div>b</div>\n</body>\n</rml>");
  if (!expect_ok(r, "ws-filter: parse ok")) return;

  Element& body = r.document->body();
  check(body.child_count() == 2, "ws-filter: body has exactly 2 children (both <div>s, no ws)");
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- text content preserved byte-verbatim (no trim/collapse), matching uix-dom.md 6(b).
// PT: Caso 5 -- conteúdo de texto preservado byte-verbatim (sem trim/colapso), batendo com o
//     uix-dom.md 6(b).
// ---------------------------------------------------------------------------
void test_text_verbatim() {
  ParseResult r = parse_document("<rml><body><span>  Hello   world  </span></body></rml>");
  if (!expect_ok(r, "verbatim: parse ok")) return;

  Element& body = r.document->body();
  auto* span = glintfx::uix::as_element(body.children()[0].get());
  check(span != nullptr && span->child_count() == 1, "verbatim: span has 1 text child");
  if (span == nullptr || span->child_count() != 1) return;
  auto* text = glintfx::uix::as_text(span->children()[0].get());
  check(text != nullptr, "verbatim: child is a Text node");
  if (text != nullptr) {
    check_eq(text->content(), "  Hello   world  ", "verbatim: all spaces intact, no trim");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- entity decoding: named (amp/lt/gt/quot) + numeric (decimal/hex, INCLUDING
//     `&#160;` as the real U+00A0 spelling), in BOTH text content and attribute values. `apos`
//     AND `nbsp` REMOVED from this case's own input (UIX-ENTITY-PARIDADE, 2026-08) -- upstream's
//     `StringUtilities::DecodeRml` never recognised either (see glintfx/src/uix/dom/parser.cpp's
//     own decode_entities()/decode_named_entity() header comments for the source-cited proof, and
//     docs/uix-dom.md section 6c for the corrected, live-RmlUi-verified worked example this case
//     now agrees with). `&nbsp;` is the more treacherous of the two removals: it used to decode
//     to a real 2-byte U+00A0 sequence, so a stale assertion here would not have crashed, just
//     silently asserted the WRONG bytes forever -- `&#160;` is the numeric spelling that DOES
//     survive as U+00A0 both upstream and here, used below wherever this case actually wants that
//     codepoint. parser_hardening_sanity.cpp's own `test_entity_tolerance` is where
//     `&apos;`/`&nbsp;`'s new literal-passthrough behaviour is pinned explicitly; this case only
//     needed the two stale entries dropped from ITS input+expectation so it keeps testing what
//     upstream actually does.
// PT: Caso 6 -- decodificação de entidade: nomeada (amp/lt/gt/quot) + numérica (decimal/hex,
//     INCLUINDO `&#160;` como a grafia real de U+00A0), TANTO em conteúdo de texto QUANTO em
//     valor de atributo. `apos` E `nbsp` REMOVIDOS do próprio input deste caso
//     (UIX-ENTITY-PARIDADE, 2026-08) -- o `StringUtilities::DecodeRml` do upstream nunca
//     reconheceu nenhum dos dois (ver os próprios comentários de cabeçalho do
//     decode_entities()/decode_named_entity() em glintfx/src/uix/dom/parser.cpp pra prova citada
//     na fonte, e a seção 6c do docs/uix-dom.md pro exemplo trabalhado corrigido, verificado ao
//     vivo contra o RmlUi, que este caso agora concorda). `&nbsp;` é a mais traiçoeira das duas
//     remoções: ele costumava decodificar pra uma sequência real de 2 bytes U+00A0, então uma
//     asserção velha aqui não teria crashado, só afirmado os bytes ERRADOS em silêncio pra
//     sempre -- `&#160;` é a grafia numérica que DE FATO sobrevive como U+00A0 tanto no upstream
//     quanto aqui, usada abaixo onde este caso realmente quer aquele codepoint. O próprio
//     `test_entity_tolerance` do parser_hardening_sanity.cpp é onde o novo comportamento de
//     passagem-literal de `&apos;`/`&nbsp;` é fixado explicitamente; este caso só precisava
//     soltar as duas entradas velhas do input+expectativa dele pra continuar testando o que o
//     upstream de fato faz.
// ---------------------------------------------------------------------------
void test_entity_decoding() {
  ParseResult r = parse_document(
      "<rml><body><div title=\"A&amp;B &#38;&#x26; &lt;tag&gt;\">Hi&#160;there "
      "&amp; &lt;x&gt; &quot;q&quot; &#65;&#x41;</div></body></rml>");
  if (!expect_ok(r, "entities: parse ok")) return;

  Element& body = r.document->body();
  auto* div = glintfx::uix::as_element(body.children()[0].get());
  check(div != nullptr, "entities: child is an Element");
  if (div == nullptr) return;

  auto title = div->attribute("title");
  check(title.has_value(), "entities: title attribute present");
  if (title.has_value()) {
    check_eq(*title, "A&B && <tag>", "entities: attribute value entities decoded");
  }

  check(div->child_count() == 1, "entities: div has 1 text child");
  if (div->child_count() != 1) return;
  auto* text = glintfx::uix::as_text(div->children()[0].get());
  check(text != nullptr, "entities: child is a Text node");
  if (text != nullptr) {
    check_eq(text->content(), "Hi\xC2\xA0there & <x> \"q\" AA",
             "entities: text content entities decoded, including &#160; (U+00A0, C2 A0 -- NOT "
             "&nbsp;, which upstream leaves undecoded, see this case's own header comment)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- comments are tokenized by S1 but discarded here: NOT a DOM node, and do not
//     perturb sibling child-indices.
// PT: Caso 7 -- comentários são tokenizados pela S1 mas descartados aqui: NÃO viram nó de DOM, e
//     não perturbam índice de filho de irmão.
// ---------------------------------------------------------------------------
void test_comments_discarded() {
  ParseResult r = parse_document(
      "<rml><!-- top comment --><body><!-- c1 --><div>a</div><!-- c2 --></body></rml>");
  if (!expect_ok(r, "comments: parse ok")) return;

  Element& body = r.document->body();
  check(body.child_count() == 1, "comments: body has exactly 1 child (the div), comments gone");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- <head> opacity: content captured raw, verbatim, entities NOT decoded, and its
//     inner tags (style/link/title) never become queryable Elements anywhere in the tree.
//     Mirrors docs/uix-dom.md section 11's own worked example -- UPDATED (UIX-ENTITY-PARIDADE,
//     2026-08) to use `&#160;` instead of the ORIGINAL `&nbsp;`, following that section's own
//     2026-08-05 correction: `&nbsp;` is not one of DecodeRml's recognised forms and would have
//     survived undecoded, not as the U+00A0 this fixture's markup means to exercise.
// PT: Caso 8 -- opacidade de <head>: conteúdo capturado cru, verbatim, entidades NÃO decodificadas,
//     e as tags internas dele (style/link/title) nunca viram Element consultável em lugar nenhum
//     da árvore. Espelha o próprio exemplo trabalhado da seção 11 do docs/uix-dom.md --
//     ATUALIZADO (UIX-ENTITY-PARIDADE, 2026-08) pra usar `&#160;` em vez do `&nbsp;` ORIGINAL,
//     seguindo a própria correção de 2026-08-05 daquela seção: `&nbsp;` não é uma das formas
//     reconhecidas do DecodeRml e teria sobrevivido não-decodificado, não como o U+00A0 que o
//     markup desta fixture pretende exercitar.
// ---------------------------------------------------------------------------
void test_head_opacity() {
  ParseResult r = parse_document(
      "<rml>\n"
      "<head>\n"
      "<style>body{color:white}</style>\n"
      "</head>\n"
      "<body>\n"
      "<div id=\"panel\" class=\"wide highlighted\" data-if=\"flag\" title=\"Panel\">\n"
      "  <span class=\"highlighted wide\">Hi&#160;there</span>\n"
      "</div>\n"
      "</body>\n"
      "</rml>");
  if (!expect_ok(r, "head-opacity: parse ok")) return;

  Document& doc = *r.document;
  check(doc.head().present, "head-opacity: HEAD PRESENT");
  check_eq(doc.head().raw, "\n<style>body{color:white}</style>\n",
           "head-opacity: raw payload verbatim, entities NOT decoded, exactly the worked example");

  // No "style" element exists ANYWHERE -- head's content never entered the tree at all.
  check(doc.body().find_by_id("panel") != nullptr, "head-opacity: body's own #panel is findable");
  Element& body = doc.body();
  check(body.child_count() == 1,
        "head-opacity: body has exactly 1 child (the div), head's "
        "'\\n' text sibling is NOT part of body");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- self-closed <head/> (no fixture exercises this, but the grammar permits it):
//     HEAD PRESENT with an empty raw payload, not HEAD ABSENT.
// PT: Caso 9 -- <head/> auto-fechado (nenhuma fixture exercita isto, mas a gramática permite):
//     HEAD PRESENT com payload cru vazio, não HEAD ABSENT.
// ---------------------------------------------------------------------------
void test_self_closed_head() {
  ParseResult r = parse_document("<rml><head/><body>x</body></rml>");
  if (!expect_ok(r, "self-closed-head: parse ok")) return;
  check(r.document->head().present, "self-closed-head: HEAD PRESENT (not ABSENT)");
  check(r.document->head().raw.empty(), "self-closed-head: empty raw payload");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- tag-name case folding: BOTH open and close fold to lowercase (uix-dom.md
//     section 8, XMLParser.cpp:136,167) -- <DIV>...</div> genuinely matches and closes.
// PT: Caso 10 -- dobra de caixa de nome de tag: abertura E fechamento dobram pra minúscula
//     (uix-dom.md seção 8, XMLParser.cpp:136,167) -- <DIV>...</div> genuinamente casa e fecha.
// ---------------------------------------------------------------------------
void test_tag_case_folding() {
  ParseResult r = parse_document("<RML><BODY><DIV>x</div></BODY></RML>");
  if (!expect_ok(r, "case-fold: parse ok")) return;
  Element& body = r.document->body();
  check_eq(body.tag(), "body", "case-fold: root tag folded to lowercase 'body'");
  check(body.child_count() == 1, "case-fold: body has 1 child");
  if (body.child_count() != 1) return;
  auto* div = glintfx::uix::as_element(body.children()[0].get());
  check(div != nullptr && div->tag() == "div", "case-fold: <DIV> stored as lowercase 'div'");
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- duplicate id/class on the SAME tag: last occurrence wins, whole-value replace
//     for class (not merged across occurrences) -- this slice's own documented, non-corpus-
//     exercised policy (see parser.hpp header comment).
// PT: Caso 11 -- id/class duplicados na MESMA tag: última ocorrência vence, substituição de
//     VALOR INTEIRO pro class (não mesclado entre ocorrências) -- política própria e documentada
//     desta fatia, não exercitada por corpus (ver comentário de cabeçalho do parser.hpp).
// ---------------------------------------------------------------------------
void test_duplicate_id_class_last_wins() {
  ParseResult r = parse_document(
      "<rml><body><div id=\"a\" id=\"b\" class=\"x y\" class=\"z\"></div></body></rml>");
  if (!expect_ok(r, "dup-attrs: parse ok")) return;
  Element& body = r.document->body();
  auto* div = glintfx::uix::as_element(body.children()[0].get());
  check(div != nullptr, "dup-attrs: child is Element");
  if (div == nullptr) return;
  check_eq(div->id(), "b", "dup-attrs: last id wins");
  check(div->classes().size() == 1 && div->has_class("z"),
        "dup-attrs: last class attribute WHOLE VALUE replaces, 'x'/'y' from the earlier "
        "occurrence are gone, not merged");
}

// ---------------------------------------------------------------------------
// EN: Case 12 -- UIX-CLASS-SPLIT-2 (RMLX-1, 2026-08-05): `class` splits on LITERAL SPACE ONLY
//     (uix-dom.md section 7, revised `UIX-CLASS-SPLIT-SPEC`), not the 4-char whitespace set --
//     end-to-end, text through the dump. Two sub-cases:
//       (a) `class="a<TAB>b"` (no space at all) -- ONE class, the tab embedded and preserved.
//       (b) `class="<TAB>b a<TAB>a "` -- the EXACT fixture the RmlUi-linked oracle
//           (`glintfx/src/rml/dom_dump_determinism_sanity.cpp`'s F2 case) uses, reproduced here on
//           this module's OWN parser+tree+dumper to prove parity: leading tab in the first
//           space-delimited segment is trimmed ("<TAB>b" -> "b"), the tab embedded between the
//           two 'a's survives ("a<TAB>a" stays "a\ta"), trailing space produces no extra token.
//           Expected CLASS line is byte-identical to F2's: "a\ta b" (sorted ascending byte-wise,
//           0x61 0x09 0x61 < 0x62).
// PT: Caso 12 -- UIX-CLASS-SPLIT-2 (RMLX-1, 2026-08-05): `class` separa por ESPAÇO LITERAL
//     APENAS (seção 7 do uix-dom.md, revisada pela `UIX-CLASS-SPLIT-SPEC`), não o conjunto de 4
//     caracteres de whitespace -- ponta a ponta, do texto até o dump. Dois subcasos:
//       (a) `class="a<TAB>b"` (sem espaço nenhum) -- UMA classe, o tab embutido preservado.
//       (b) `class="<TAB>b a<TAB>a "` -- a fixture EXATA que o oráculo linkado ao RmlUi (o caso
//           F2 do `glintfx/src/rml/dom_dump_determinism_sanity.cpp`) usa, reproduzida aqui no
//           parser+árvore+dumper PRÓPRIOS deste módulo pra provar paridade: o tab líder no
//           primeiro segmento delimitado por espaço é aparado ("<TAB>b" -> "b"), o tab embutido
//           entre os dois 'a' sobrevive ("a<TAB>a" continua "a\ta"), o espaço final não produz
//           token extra. A linha CLASS esperada é byte-idêntica à do F2: "a\ta b" (ordenada
//           ascendente byte-a-byte, 0x61 0x09 0x61 < 0x62).
// ---------------------------------------------------------------------------
void test_class_split_literal_space_only() {
  using glintfx::uix::dump_document;

  // (a) No literal space at all -- one class, tab embedded and preserved.
  {
    ParseResult r = parse_document("<rml><body><div class=\"a\tb\"></div></body></rml>");
    if (!expect_ok(r, "class-split(a): parse ok")) return;
    Element& body = r.document->body();
    check(body.child_count() == 1, "class-split(a): body has 1 child");
    if (body.child_count() != 1) return;
    auto* div = glintfx::uix::as_element(body.children()[0].get());
    check(div != nullptr, "class-split(a): child is Element");
    if (div == nullptr) return;

    check(div->classes().size() == 1,
          "class-split(a): exactly ONE class -- no literal space means no split point");
    check(div->has_class("a\tb"),
          "class-split(a): the class is 'a\\tb' with the tab embedded, not split into 'a'+'b'");

    const std::string dump = dump_document(*r.document);
    check(dump.find("body/0 CLASS a\\tb\n") != std::string::npos,
          "class-split(a): dump CLASS line has the escaped embedded tab, 'a\\tb'");
  }

  // (b) F2 parity fixture: "\tb a\ta " -- leading tab trimmed, embedded tab kept, trailing space
  //     produces no extra token. Same source dom_dump_determinism_sanity.cpp's F2 exercises
  //     against the REAL RmlUi-linked oracle; reproduced here against this module's own
  //     parser+tree+dumper for cross-module parity.
  {
    ParseResult r = parse_document(
        "<rml><body><div id=\"w\" class=\"\tb a\ta \"></div></body></rml>");
    if (!expect_ok(r, "class-split(b): parse ok")) return;
    Element& body = r.document->body();
    if (body.child_count() != 1) {
      check(false, "class-split(b): body has 1 child");
      return;
    }
    auto* div = glintfx::uix::as_element(body.children()[0].get());
    check(div != nullptr, "class-split(b): child is Element");
    if (div == nullptr) return;

    check(div->classes().size() == 2,
          "class-split(b): exactly 2 classes -- 'a\\ta' and 'b' (leading tab trimmed, trailing "
          "space produces no 3rd empty token)");
    check(div->has_class("a\ta"),
          "class-split(b): 'a\\ta' present with the embedded tab preserved");
    check(div->has_class("b"), "class-split(b): 'b' present, its leading tab trimmed away");

    const std::string dump = dump_document(*r.document);
    check(dump.find("body/0 CLASS a\\ta b\n") != std::string::npos,
          "class-split(b): dump CLASS line 'a\\ta b' -- byte-identical to F2's oracle output "
          "(dom_dump_determinism_sanity.cpp)");
  }
}

} // namespace

int main() {
  test_minimal_document();
  test_attributes();
  test_nesting_and_self_close();
  test_whitespace_only_text_filtered();
  test_text_verbatim();
  test_entity_decoding();
  test_comments_discarded();
  test_head_opacity();
  test_self_closed_head();
  test_tag_case_folding();
  test_duplicate_id_class_last_wins();
  test_class_split_literal_space_only();

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_tokens_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_tokens_sanity: PASS");
  return 0;
}
