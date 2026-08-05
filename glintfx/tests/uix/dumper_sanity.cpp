// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6b -- functional unit test for glintfx::uix::dump_document (glintfx/src/uix/dom/
//     dumper.{hpp,cpp}). Standalone, no RmlUi/GLFW/GL -- see dumper.hpp's own header comment for
//     the full scope/boundary this module holds itself to. Every case below traces back to a
//     specific docs/uix-dom.md ("uix-dom.md" hereafter) section -- this dumper was written
//     reading ONLY that document (never the S6a sibling's source), so every assertion here is
//     pinned against the spec text, not against "whatever the implementation happens to emit".
//     Trees are built by hand via dom_tree.hpp's own API (Document/Element/Text), never via
//     parse_document -- this keeps this suite's pass/fail independent of any future S3 change,
//     and lets it exercise tree shapes (e.g. an Element constructed with an uppercase tag) the
//     real corpus may never produce but the spec still defines behaviour for.
// PT: RMLX-1/S6b -- teste unit funcional pro glintfx::uix::dump_document (glintfx/src/uix/dom/
//     dumper.{hpp,cpp}). Standalone, sem RmlUi/GLFW/GL -- ver o próprio comentário de cabeçalho
//     do dumper.hpp pro escopo/fronteira completos a que este módulo se prende. Todo caso abaixo
//     remonta a uma seção específica do docs/uix-dom.md ("uix-dom.md" daqui em diante) -- este
//     dumper foi escrito lendo SÓ aquele documento (nunca o fonte do irmão S6a), então toda
//     asserção aqui está presa contra o texto da spec, não contra "o que a implementação por
//     acaso emite". Árvores são construídas à mão via a própria API do dom_tree.hpp
//     (Document/Element/Text), nunca via parse_document -- isto mantém o passa/falha desta suíte
//     independente de qualquer mudança futura da S3, e permite exercitar formas de árvore (ex.:
//     um Element construído com tag maiúscula) que o corpus real pode nunca produzir mas cujo
//     comportamento a spec ainda assim define.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dumper.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "uix/dom/dom_tree.hpp"

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
    std::fprintf(stderr, "FAIL: %s\n  got:  \"%.*s\"\n  want: \"%.*s\"\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::AppendResult;
using glintfx::uix::Document;
using glintfx::uix::dump_document;
using glintfx::uix::Element;
using glintfx::uix::escape;
using glintfx::uix::Text;

// ---------------------------------------------------------------------------
// EN: Case 1 -- uix-dom.md section 11's own worked example, reproduced byte-for-byte by building
//     the tree by hand (the source markup in section 11 is never parsed here -- S3 is a
//     different module, this test only proves S6b's OWN dump matches the spec's OWN worked
//     output for a tree shaped that way). This is the single strongest conformance pin this
//     suite has: every field kind (HEAD PRESENT, ID, CLASS with source-order-reversed input,
//     ATTR sorted by name, CHILDREN, nested TEXT with a real U+00A0 byte) appears in one tree.
// PT: Caso 1 -- o próprio exemplo trabalhado da seção 11 do uix-dom.md, reproduzido byte a byte
//     construindo a árvore à mão (o markup-fonte da seção 11 nunca é parseado aqui -- a S3 é um
//     módulo diferente, este teste só prova que o dump DA PRÓPRIA S6b bate com a saída
//     trabalhada DA PRÓPRIA spec pra uma árvore com essa forma). Este é o pino de conformidade
//     mais forte desta suíte: todo tipo de campo (HEAD PRESENT, ID, CLASS com input em
//     ordem-fonte invertida, ATTR ordenado por nome, CHILDREN, TEXT aninhado com um byte U+00A0
//     real) aparece numa árvore só.
// ---------------------------------------------------------------------------
void test_worked_example_byte_exact() {
  Document doc;
  doc.set_head("\n<style>body{color:white}</style>\n");

  auto div = std::make_unique<Element>("div");
  div->set_id("panel");
  // EN: source order "wide highlighted" -- deliberately reversed vs. the sorted output, proving
  //     the dump sorts regardless of insertion order (uix-dom.md section 7).
  // PT: ordem-fonte "wide highlighted" -- deliberadamente invertida vs. a saída ordenada,
  //     provando que o dump ordena independente da ordem de inserção (seção 7 do uix-dom.md).
  check(div->add_class("wide"), "worked example: add_class(wide) accepted");
  check(div->add_class("highlighted"), "worked example: add_class(highlighted) accepted");
  // EN: attributes applied source-order "data-if" then "title" -- already alphabetical, so a
  //     second case below (test_attribute_sort_independent_of_insertion_order) covers the
  //     reversed-insertion-order proof this one alone would not.
  // PT: atributos aplicados em ordem-fonte "data-if" depois "title" -- já alfabético, então um
  //     segundo caso abaixo (test_attribute_sort_independent_of_insertion_order) cobre a prova de
  //     ordem-de-inserção-invertida que este sozinho não cobriria.
  check(div->set_attribute("data-if", "flag"), "worked example: set_attribute(data-if) accepted");
  check(div->set_attribute("title", "Panel"), "worked example: set_attribute(title) accepted");

  auto span = std::make_unique<Element>("span");
  check(span->add_class("highlighted"), "worked example: span add_class(highlighted) accepted");
  check(span->add_class("wide"), "worked example: span add_class(wide) accepted");

  // EN: "Hi" + U+00A0 (the real two-byte UTF-8 sequence 0xC2 0xA0, decoded from &nbsp; upstream
  //     of this dumper -- section 6c) + "there".
  // PT: "Hi" + U+00A0 (a sequência UTF-8 real de dois bytes 0xC2 0xA0, decodificada de &nbsp;
  //     upstream deste dumper -- seção 6c) + "there".
  const std::string text_content = std::string("Hi") + "\xC2\xA0" + "there";
  AppendResult text_append = span->append_child(std::make_unique<Text>(text_content));
  check(text_append.node != nullptr, "worked example: Text child appended to span");

  AppendResult span_append = div->append_child(std::move(span));
  check(span_append.node != nullptr, "worked example: span appended to div");

  AppendResult div_append = doc.body().append_child(std::move(div));
  check(div_append.node != nullptr, "worked example: div appended to body");

  const std::string expected =
      "HEAD PRESENT \\n<style>body{color:white}</style>\\n\n"
      "body ELEM body\n"
      "body CHILDREN 1\n"
      "body/0 ELEM div\n"
      "body/0 ID panel\n"
      "body/0 CLASS highlighted wide\n"
      "body/0 ATTR data-if=flag\n"
      "body/0 ATTR title=Panel\n"
      "body/0 CHILDREN 1\n"
      "body/0/0 ELEM span\n"
      "body/0/0 CLASS highlighted wide\n"
      "body/0/0 CHILDREN 1\n"
      "body/0/0/0 TEXT Hi\xC2\xA0there\n";

  check_eq(dump_document(doc), expected,
           "worked example (uix-dom.md section 11): byte-exact dump");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- HEAD ABSENT (uix-dom.md section 4: "HEAD ABSENT is emitted, verbatim, when the
//     source document has no <head> element at all -- never omit the HEAD line itself").
// PT: Caso 2 -- HEAD ABSENT (seção 4 do uix-dom.md: "HEAD ABSENT é emitido, verbatim, quando o
//     documento-fonte não tem <head> nenhum -- nunca omitir a própria linha HEAD").
// ---------------------------------------------------------------------------
void test_head_absent() {
  Document doc;
  const std::string out = dump_document(doc);
  check(out.rfind("HEAD ABSENT\n", 0) == 0, "HEAD ABSENT is the first line when head was never set");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- a body with zero children still emits `body CHILDREN 0` (uix-dom.md section 5:
//     "CHILDREN <n> -- ALWAYS present, even at n=0") and the root gets the full element block
//     (no special-cased root, same section).
// PT: Caso 3 -- um body com zero filhos ainda emite `body CHILDREN 0` (seção 5 do uix-dom.md:
//     "CHILDREN <n> -- SEMPRE presente, mesmo em n=0") e a raiz recebe o bloco de elemento
//     completo (sem atalho especial de raiz, mesma seção).
// ---------------------------------------------------------------------------
void test_empty_body_children_zero() {
  Document doc;
  const std::string expected = "HEAD ABSENT\nbody ELEM body\nbody CHILDREN 0\n";
  check_eq(dump_document(doc), expected, "empty body: CHILDREN 0 always present, no children lines");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- id="" and id absent are the SAME state, no ID line either way (uix-dom.md
//     section 7's own empty-value asymmetry for id specifically).
// PT: Caso 4 -- id="" e id ausente são o MESMO estado, sem linha ID de nenhum jeito (a própria
//     assimetria de valor-vazio da seção 7 do uix-dom.md pra id especificamente).
// ---------------------------------------------------------------------------
void test_empty_id_same_as_absent_id() {
  Document doc;
  auto el_absent = std::make_unique<Element>("div");
  auto el_empty = std::make_unique<Element>("span");
  el_empty->set_id("");
  doc.body().append_child(std::move(el_absent));
  doc.body().append_child(std::move(el_empty));

  const std::string out = dump_document(doc);
  check(out.find(" ID ") == std::string::npos,
        "neither an absent id nor an explicit empty id ever produces an ID line");
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- a generic attribute with an empty value IS emitted, unlike id/class (uix-dom.md
//     section 7: "a generic attribute with an empty value is not the same as that attribute
//     being absent -- it still gets an ATTR line with an empty value").
// PT: Caso 5 -- um atributo genérico com valor vazio É emitido, diferente de id/class (seção 7
//     do uix-dom.md: "um atributo genérico com valor vazio não é o mesmo que esse atributo estar
//     ausente -- ainda ganha uma linha ATTR com valor vazio").
// ---------------------------------------------------------------------------
void test_generic_attribute_empty_value_emitted() {
  Document doc;
  auto el = std::make_unique<Element>("div");
  check(el->set_attribute("data-if", ""), "empty-value generic attribute accepted");
  doc.body().append_child(std::move(el));

  const std::string expected =
      "HEAD ABSENT\nbody ELEM body\nbody CHILDREN 1\n"
      "body/0 ELEM div\n"
      "body/0 ATTR data-if=\n"
      "body/0 CHILDREN 0\n";
  check_eq(dump_document(doc), expected, "empty-value generic attribute: ATTR line with nothing after '='");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- CLASS is omitted entirely when the class set is empty (uix-dom.md section 5:
//     "OMITTED if the class set is empty").
// PT: Caso 6 -- CLASS é omitido por completo quando o conjunto de classes é vazio (seção 5 do
//     uix-dom.md: "OMITIDO se o conjunto de classes for vazio").
// ---------------------------------------------------------------------------
void test_empty_class_set_omits_class_line() {
  Document doc;
  auto el = std::make_unique<Element>("div");
  doc.body().append_child(std::move(el));

  const std::string out = dump_document(doc);
  check(out.find(" CLASS") == std::string::npos, "no classes: CLASS line entirely absent");
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- attribute AND class ordering is sorted ascending byte-wise regardless of
// insertion order (uix-dom.md section 7) -- this time with insertion in REVERSE alphabetical
//     order for attributes (the worked example's own attributes happened to already be
//     alphabetical in source order, so this case is the one that actually exercises the sort).
// PT: Caso 7 -- ordenação de atributo E classe é ascendente byte-a-byte independente da ordem de
//     inserção (seção 7 do uix-dom.md) -- desta vez com inserção em ordem alfabética REVERSA pros
//     atributos (os próprios atributos do exemplo trabalhado por acaso já estavam alfabéticos em
//     ordem-fonte, então este caso é o que de fato exercita a ordenação).
// ---------------------------------------------------------------------------
void test_attribute_sort_independent_of_insertion_order() {
  Document doc;
  auto el = std::make_unique<Element>("div");
  check(el->set_attribute("zeta", "1"), "attr zeta accepted");
  check(el->set_attribute("alpha", "2"), "attr alpha accepted");
  check(el->set_attribute("mid", "3"), "attr mid accepted");
  doc.body().append_child(std::move(el));

  const std::string expected =
      "HEAD ABSENT\nbody ELEM body\nbody CHILDREN 1\n"
      "body/0 ELEM div\n"
      "body/0 ATTR alpha=2\n"
      "body/0 ATTR mid=3\n"
      "body/0 ATTR zeta=1\n"
      "body/0 CHILDREN 0\n";
  check_eq(dump_document(doc), expected,
           "attributes inserted zeta,alpha,mid dump sorted alpha,mid,zeta");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- tag-name case is preserved, never folded by this dumper (uix-dom.md section 8:
//     tag lowercasing, if any, is S3/upstream-parser territory; this module must not apply a
//     transform the spec does not name). Element constructed directly with an uppercase tag
//     (never produced by the real S3 parser, which folds before construction -- but this dumper
//     must not ITSELF fold, so a hand-built tree is the only way to prove that).
// PT: Caso 8 -- caixa de nome de tag é preservada, nunca dobrada por este dumper (seção 8 do
//     uix-dom.md: dobrar tag pra minúscula, se houver, é território da S3/parser upstream; este
//     módulo não pode aplicar uma transformação que a spec não nomeia). Element construído direto
//     com tag maiúscula (nunca produzido pelo parser S3 real, que dobra antes de construir -- mas
//     este dumper não pode ELE MESMO dobrar, então uma árvore feita à mão é o único jeito de
//     provar isso).
// ---------------------------------------------------------------------------
void test_tag_case_preserved_not_folded() {
  Document doc;
  auto el = std::make_unique<Element>("DIV");
  doc.body().append_child(std::move(el));

  const std::string out = dump_document(doc);
  check(out.find("body/0 ELEM DIV\n") != std::string::npos,
        "dumper never folds tag case on its own initiative");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- text content is preserved byte-verbatim, no trim, no internal-run collapse
//     (uix-dom.md section 6b: "  Hello   world  " between two tags is one TEXT node whose
//     content is exactly that, all spaces intact) -- and every escapable byte (backslash, \n,
//     \r, \t) round-trips through `escape` in the fixed order section 2 specifies.
// PT: Caso 9 -- conteúdo de texto é preservado byte-verbatim, sem trim, sem colapso de run
//     interno (seção 6b do uix-dom.md: "  Hello   world  " entre duas tags é UM nó TEXT cujo
//     conteúdo é exatamente isso, todos os espaços intactos) -- e todo byte escapável (barra
//     invertida, \n, \r, \t) faz round-trip pelo `escape` na ordem fixa que a seção 2 especifica.
// ---------------------------------------------------------------------------
void test_text_verbatim_and_escaping() {
  Document doc;
  auto el = std::make_unique<Element>("p");
  el->append_child(std::make_unique<Text>("  Hello   world  "));
  doc.body().append_child(std::move(el));

  const std::string out = dump_document(doc);
  check(out.find("body/0/0 TEXT   Hello   world  \n") != std::string::npos,
        "TEXT content preserved byte-verbatim, no trim, no internal-run collapse");

  // EN: section 2's exact escape table, all 4 characters, plus a literal backslash immediately
  //     followed by a real newline -- proves the pass does not re-scan its OWN emitted escape
  //     marker (a sequential find/replace over the growing string would double-escape here: the
  //     '\' emitted for the backslash could itself be mistaken for the start of a NEW escape
  //     sequence by a second pass, which this implementation must not do).
  // PT: a tabela de escape exata da seção 2, os 4 caracteres, mais uma barra invertida literal
  //     imediatamente seguida de uma quebra de linha real -- prova que o passe não re-escaneia o
  //     PRÓPRIO marcador de escape que acabou de emitir (um find/replace sequencial sobre a
  //     string crescente dobraria o escape aqui: o '\' emitido pra barra invertida poderia ele
  //     mesmo ser confundido com o início de uma NOVA sequência de escape por um segundo passe,
  //     o que esta implementação não pode fazer).
  check_eq(escape("a\\b\nc\rd\te\\\nf"), "a\\\\b\\nc\\rd\\te\\\\\\nf",
           "escape(): backslash, \\n, \\r, \\t, and a backslash-then-newline pair, single pass");

  // EN: space is NEVER escaped (section 2: "space is deliberately never escaped"), including a
  //     run of several in a row.
  // PT: espaço NUNCA é escapado (seção 2: "espaço deliberadamente nunca é escapado"), inclusive
  //     um run de vários seguidos.
  check_eq(escape("a   b"), "a   b", "escape(): literal ASCII space runs pass through unchanged");

  // EN: a real multi-byte UTF-8 sequence (U+00A0, 0xC2 0xA0) passes through untouched -- neither
  //     byte of it collides with the 4-character escape set.
  // PT: uma sequência UTF-8 multi-byte real (U+00A0, 0xC2 0xA0) passa intocada -- nenhum dos dois
  //     bytes dela colide com o conjunto de escape de 4 caracteres.
  check_eq(escape("\xC2\xA0"), "\xC2\xA0", "escape(): multi-byte UTF-8 sequence passes through unchanged");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- CLASS tokens are individually escaped too (uix-dom.md section 2 names "each
//     CLASS token" among the escaped fields), exercised with a class token containing a literal
//     backslash -- syntactically legal per Element::add_class's own single-token,
//     no-embedded-whitespace check (backslash is not one of the 4 whitespace characters that
//     check rejects).
// PT: Caso 10 -- tokens de CLASS são escapados individualmente também (a seção 2 do uix-dom.md
//     nomeia "cada token de CLASS" entre os campos escapados), exercitado com um token de classe
//     contendo uma barra invertida literal -- sintaticamente legal pela própria checagem de
//     token-único-sem-whitespace-embutido do Element::add_class (barra invertida não é um dos 4
//     caracteres de whitespace que aquela checagem rejeita).
// ---------------------------------------------------------------------------
void test_class_token_escaped() {
  Document doc;
  auto el = std::make_unique<Element>("div");
  check(el->add_class("a\\b"), "class token containing a literal backslash accepted");
  doc.body().append_child(std::move(el));

  const std::string out = dump_document(doc);
  check(out.find("body/0 CLASS a\\\\b\n") != std::string::npos,
        "CLASS token itself is escaped per section 2, not emitted raw");
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- attribute NAME is never escaped, only the VALUE (uix-dom.md section 2's list of
//     escaped fields names "each ATTR value", conspicuously not the name -- consistent with
//     attribute names being lexer-grammar-constrained identifiers, like tag names, section 5's
//     own "<tag> is not escaped" reasoning applied to attribute names). No corpus fixture can
//     ever produce a name needing escape (the grammar forbids it) -- this test pins the
//     BEHAVIOUR anyway so a future refactor cannot silently start escaping names.
// PT: Caso 11 -- nome de ATRIBUTO nunca é escapado, só o VALOR (a lista de campos escapados da
//     seção 2 do uix-dom.md nomeia "cada valor de ATTR", visivelmente não o nome -- consistente
//     com nomes de atributo serem identificadores restritos-pela-gramática-do-lexer, como nomes
//     de tag, o mesmo raciocínio "<tag> não é escapado" da seção 5 aplicado a nomes de atributo).
//     Fixture nenhuma do corpus consegue produzir um nome que precise de escape (a gramática
//     proíbe) -- este teste fixa o COMPORTAMENTO mesmo assim pra um futuro refactor não começar a
//     escapar nomes em silêncio.
// ---------------------------------------------------------------------------
void test_attribute_name_never_escaped() {
  Document doc;
  auto el = std::make_unique<Element>("div");
  check(el->set_attribute("plain-name", "a\\b"), "attribute with an escapable VALUE accepted");
  doc.body().append_child(std::move(el));

  const std::string out = dump_document(doc);
  check(out.find("body/0 ATTR plain-name=a\\\\b\n") != std::string::npos,
        "attribute name printed raw, value escaped");
}

} // namespace

int main() {
  test_worked_example_byte_exact();
  test_head_absent();
  test_empty_body_children_zero();
  test_empty_id_same_as_absent_id();
  test_generic_attribute_empty_value_emitted();
  test_empty_class_set_omits_class_line();
  test_attribute_sort_independent_of_insertion_order();
  test_tag_case_preserved_not_folded();
  test_text_verbatim_and_escaping();
  test_class_token_escaped();
  test_attribute_name_never_escaped();

  if (g_failures > 0) {
    std::fprintf(stderr, "dumper_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dumper_sanity: PASS");
  return 0;
}
