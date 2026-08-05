// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6b -- determinism proof for glintfx::uix::dump_document (glintfx/src/uix/dom/
//     dumper.{hpp,cpp}). Standalone, no RmlUi/GLFW/GL. This is the specific test the S6b brief
//     asks for by name: "a test that proves determinism, not just that the output looks right" --
//     dumper_sanity.cpp already pins the exact bytes for known shapes; THIS file instead proves
//     the byte-exactness survives across (a) two independent calls on the SAME tree object, (b)
//     two trees built through DIFFERENT insertion orders that describe the identical final
//     structure, and (c) two independent parses of the SAME source text via parse_document
//     (guards against the class of bug that a hand-built-tree test cannot reach: a dumper that
//     accidentally leaks something order-dependent -- a raw pointer address, an
//     `std::unordered_map`/`std::unordered_set` iteration, a heap-allocator artifact -- into its
//     output would still pass every fixed-expected-string test yet fail THIS one, because a
//     second independent parse of the same bytes gets fresh, differently-addressed objects).
//     uix-dom.md's own "Why this document exists" section is explicit about why this matters:
//     the two oracle dumpers (S6a/S6b) may run on different machines entirely, so anything this
//     dumper reads that is not a pure function of the TREE's own logical content is a latent,
//     spurious-divergence bug waiting for S7 to trip over.
// PT: RMLX-1/S6b -- prova de determinismo pro glintfx::uix::dump_document (glintfx/src/uix/dom/
//     dumper.{hpp,cpp}). Standalone, sem RmlUi/GLFW/GL. Este é o teste específico que o briefing
//     da S6b pede nominalmente: "um teste que prova o determinismo, não só que a saída 'parece
//     certa'" -- o dumper_sanity.cpp já fixa os bytes exatos pra formas conhecidas; ESTE arquivo
//     em vez disso prova que a exatidão-de-byte sobrevive através de (a) duas chamadas
//     independentes sobre o MESMO objeto de árvore, (b) duas árvores construídas por ordens de
//     inserção DIFERENTES que descrevem a estrutura final idêntica, e (c) dois parses
//     independentes do MESMO texto-fonte via parse_document (protege contra a classe de bug que
//     um teste de árvore-feita-à-mão não alcança: um dumper que vaza acidentalmente algo
//     dependente-de-ordem -- um endereço de ponteiro cru, uma iteração de
//     `std::unordered_map`/`std::unordered_set`, um artefato de alocador de heap -- na própria
//     saída ainda passaria todo teste de string-esperada-fixa mas falharia ESTE, porque um
//     segundo parse independente dos mesmos bytes recebe objetos frescos, endereçados de forma
//     diferente). A própria seção "Por que este documento existe" do uix-dom.md é explícita sobre
//     por que isto importa: os dois dumpers do oráculo (S6a/S6b) podem rodar em máquinas
//     inteiramente diferentes, então qualquer coisa que este dumper leia que não seja função pura
//     do próprio conteúdo LÓGICO da árvore é um bug de divergência espúria latente, esperando a
//     S7 tropeçar nele.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dumper.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "uix/dom/dom_tree.hpp"
#include "uix/dom/parser.hpp"

#ifndef GLINTFX_UIX_TESTS_DIR
#error "GLINTFX_UIX_TESTS_DIR must be defined by CMake (glintfx/tests/uix/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_DEMOS_DIR
#error "GLINTFX_UIX_DEMOS_DIR must be defined by CMake (glintfx/tests/uix/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_S3_FIXTURES_DIR
#error "GLINTFX_UIX_S3_FIXTURES_DIR must be defined by CMake (glintfx/tests/uix/CMakeLists.txt)"
#endif

namespace {

namespace fs = std::filesystem;

int g_failures = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

using glintfx::uix::AppendResult;
using glintfx::uix::Document;
using glintfx::uix::dump_document;
using glintfx::uix::Element;
using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;
using glintfx::uix::Text;

// ---------------------------------------------------------------------------
// EN: Case 1 -- two independent dump_document() calls on the exact same Document object produce
//     byte-identical strings.
// PT: Caso 1 -- duas chamadas independentes de dump_document() sobre o exato mesmo objeto
//     Document produzem strings byte-idênticas.
// ---------------------------------------------------------------------------
void test_same_tree_dumped_twice_identical() {
  Document doc;
  doc.set_head("raw head payload");
  auto el = std::make_unique<Element>("div");
  el->set_id("panel");
  el->add_class("wide");
  el->add_class("highlighted");
  el->set_attribute("data-if", "flag");
  el->append_child(std::make_unique<Text>("hello world"));
  doc.body().append_child(std::move(el));

  const std::string first = dump_document(doc);
  const std::string second = dump_document(doc);
  check(first == second, "dump_document() called twice on the same tree: byte-identical output");
  check(!first.empty(), "dump_document() sanity: output is non-empty for a non-trivial tree");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- two trees built through DIFFERENT insertion orders, describing the SAME final
//     structure, dump to the SAME bytes. Classes and attributes are added in reverse order on
//     the second tree; the two DIFFERENT Element objects (different addresses, different
//     std::set/std::map internal node allocations) must still agree byte-for-byte.
// PT: Caso 2 -- duas árvores construídas por ordens de inserção DIFERENTES, descrevendo a MESMA
//     estrutura final, dumpam pros MESMOS bytes. Classes e atributos são somados em ordem
//     invertida na segunda árvore; os dois objetos Element DIFERENTES (endereços diferentes,
//     alocações internas de nó de std::set/std::map diferentes) precisam concordar byte a byte
//     mesmo assim.
// ---------------------------------------------------------------------------
void test_different_insertion_order_same_output() {
  Document doc_a;
  {
    auto el = std::make_unique<Element>("div");
    el->set_id("panel");
    el->add_class("alpha");
    el->add_class("beta");
    el->add_class("gamma");
    el->set_attribute("aaa", "1");
    el->set_attribute("mmm", "2");
    el->set_attribute("zzz", "3");
    doc_a.body().append_child(std::move(el));
  }

  Document doc_b;
  {
    auto el = std::make_unique<Element>("div");
    el->add_class("gamma");
    el->add_class("alpha");
    el->add_class("beta");
    el->set_attribute("zzz", "3");
    el->set_attribute("aaa", "1");
    el->set_attribute("mmm", "2");
    el->set_id("panel");
    doc_b.body().append_child(std::move(el));
  }

  const std::string dump_a = dump_document(doc_a);
  const std::string dump_b = dump_document(doc_b);
  check(dump_a == dump_b,
        "two trees built with reversed class/attribute/id insertion order: identical dump");
  check(!dump_a.empty(), "sanity: dump is non-empty");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- two independent parse_document() calls on the SAME source string produce two
//     DIFFERENT Document trees (fresh heap objects, no shared state) that must still dump to
//     byte-identical output. This is the strongest guard: it rules out any accidental dependency
//     on object identity/address/allocation order, since the only thing the two calls share is
//     the source bytes themselves.
// PT: Caso 3 -- duas chamadas independentes de parse_document() sobre a MESMA string-fonte
//     produzem duas árvores Document DIFERENTES (objetos de heap frescos, sem estado
//     compartilhado) que ainda assim precisam dumpar pra saída byte-idêntica. Esta é a proteção
//     mais forte: descarta qualquer dependência acidental de identidade/endereço/ordem-de-
//     alocação de objeto, já que a única coisa que as duas chamadas compartilham são os próprios
//     bytes-fonte.
// ---------------------------------------------------------------------------
void test_two_independent_parses_of_same_source_identical_dump() {
  const std::string source =
      "<rml>\n"
      "<head><style>body{color:white}</style></head>\n"
      "<body>\n"
      "<div id=\"panel\" class=\"wide highlighted\" data-if=\"flag\" title=\"Panel\">\n"
      "  <span class=\"highlighted wide\">Hi&nbsp;there</span>\n"
      "</div>\n"
      "</body>\n"
      "</rml>\n";

  ParseResult first = parse_document(source);
  ParseResult second = parse_document(source);
  check(!first.error.has_value(), "first parse of determinism-check source: no ParseError");
  check(!second.error.has_value(), "second parse of determinism-check source: no ParseError");
  if (!first.document || !second.document) {
    return;
  }

  const std::string dump_first = dump_document(*first.document);
  const std::string dump_second = dump_document(*second.document);
  check(dump_first == dump_second,
        "two independent parses of the same source: byte-identical dumps");
  check(!dump_first.empty(), "sanity: dump is non-empty");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- across the WHOLE corpus (same 3 directories parser_corpus_sanity.cpp already
//     scans), every fixture that parses cleanly dumps to the same bytes on two independent
//     dump_document() calls over the same tree AND across two independent parses of the file.
//     This is the corpus-scale version of cases 1 and 3 above, catching a fixture-specific
//     determinism bug (e.g. one that only manifests on a particular attribute-name collision
//     pattern) that the small hand-built cases above would not exercise.
// PT: Caso 4 -- pelo corpus INTEIRO (os mesmos 3 diretórios que o parser_corpus_sanity.cpp já
//     varre), toda fixture que parseia limpo dumpa pros mesmos bytes em duas chamadas
//     independentes de dump_document() sobre a mesma árvore E através de dois parses
//     independentes do arquivo. Esta é a versão em escala-de-corpus dos casos 1 e 3 acima,
//     pegando um bug de determinismo específico-de-fixture (ex.: um que só se manifesta num
//     padrão particular de colisão de nome de atributo) que os casos pequenos feitos à mão acima
//     não exercitariam.
// ---------------------------------------------------------------------------
std::string read_file_binary(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::vector<fs::path> find_rml_fixtures(const fs::path& dir) {
  std::vector<fs::path> out;
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return out;
  }
  for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".rml") {
      out.push_back(entry.path());
    }
  }
  return out;
}

void test_corpus_wide_determinism() {
  std::vector<fs::path> fixtures = find_rml_fixtures(GLINTFX_UIX_TESTS_DIR);
  const std::vector<fs::path> demo_fixtures = find_rml_fixtures(GLINTFX_UIX_DEMOS_DIR);
  fixtures.insert(fixtures.end(), demo_fixtures.begin(), demo_fixtures.end());
  const std::vector<fs::path> s3_fixtures = find_rml_fixtures(GLINTFX_UIX_S3_FIXTURES_DIR);
  fixtures.insert(fixtures.end(), s3_fixtures.begin(), s3_fixtures.end());

  check(fixtures.size() >= 40,
        "corpus discovery found a plausible number of .rml fixtures (>= 40) -- directory "
        "wiring sanity");

  std::size_t dumped = 0;
  for (const fs::path& fixture : fixtures) {
    const std::string source = read_file_binary(fixture);
    if (source.empty()) {
      check(false, "corpus fixture is unexpectedly empty (read failed?): " + fixture.string());
      continue;
    }

    ParseResult first = parse_document(source);
    if (first.error.has_value()) {
      // EN: not this test's job to re-litigate parser correctness (parser_corpus_sanity.cpp
      //     already does); a fixture the parser rejects has no tree for this test to dump.
      // PT: não é trabalho deste teste rejulgar a corretude do parser (o
      //     parser_corpus_sanity.cpp já faz isso); uma fixture que o parser rejeita não tem
      //     árvore nenhuma pra este teste dumpar.
      continue;
    }
    ParseResult second = parse_document(source);
    check(!second.error.has_value(),
          "second independent parse of an already-clean fixture also parses cleanly: " +
              fixture.string());
    if (!first.document || !second.document) {
      continue;
    }

    const std::string dump_same_tree_a = dump_document(*first.document);
    const std::string dump_same_tree_b = dump_document(*first.document);
    check(dump_same_tree_a == dump_same_tree_b,
          "dumping the same parsed tree twice is byte-identical: " + fixture.string());

    const std::string dump_cross_parse = dump_document(*second.document);
    check(dump_same_tree_a == dump_cross_parse,
          "two independent parses of the same fixture dump to identical bytes: " +
              fixture.string());

    ++dumped;
  }

  check(dumped >= 40,
        "at least 40 corpus fixtures were actually parsed and dumped, not silently "
        "skipped by an all-erroring parse loop");
  std::printf("dumper_determinism_sanity: %zu/%zu fixtures dumped and cross-checked\n", dumped,
              fixtures.size());
}

} // namespace

int main() {
  test_same_tree_dumped_twice_identical();
  test_different_insertion_order_same_output();
  test_two_independent_parses_of_same_source_identical_dump();
  test_corpus_wide_determinism();

  if (g_failures > 0) {
    std::fprintf(stderr, "dumper_determinism_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dumper_determinism_sanity: PASS");
  return 0;
}
