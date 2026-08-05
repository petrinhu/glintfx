// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6b -- regression oracle: this dumper must produce a well-formed uix-dom.md dump
//     (glintfx/src/uix/dom/dumper.{hpp,cpp}) for EVERY real `.rml` fixture in this repo
//     (glintfx/tests/ + glintfx/demos/ + this arc's own glintfx/src/uix/dom/test_fixtures/, the
//     same three directories parser_corpus_sanity.cpp already enumerates) that parses cleanly.
//     This is this slice's own acceptance criterion, restated as a test: "emits the format of
//     docs/uix-dom.md for the whole corpus of fixtures the wave gathers (60, measured
//     2026-08-05 -- RMLX1-CORPUS, TODO.md, grew the third directory's original 1 fixture by 15
//     more real GusWorld runtime-captured screens, 45 -> 60)". Deliberately does NOT re-prove
//     determinism (dumper_determinism_sanity.cpp's
//     own job) or field-level byte-exactness against the spec's worked example
//     (dumper_sanity.cpp's own job) -- this file's only job is SHAPE, at corpus scale: every
//     fixture dumps without crashing, the first line is always a HEAD record, the output always
//     ends in a single trailing newline (uix-dom.md section 1), and every other line starts with
//     one of the record kinds section 5 defines.
// PT: RMLX-1/S6b -- oráculo de regressão: este dumper precisa produzir um dump bem-formado do
//     uix-dom.md (glintfx/src/uix/dom/dumper.{hpp,cpp}) pra TODA fixture `.rml` real deste repo
//     (glintfx/tests/ + glintfx/demos/ + o test_fixtures/ próprio deste arco em
//     glintfx/src/uix/dom/, os MESMOS três diretórios que o parser_corpus_sanity.cpp já enumera)
//     que parseia limpo. Este é o próprio critério de aceite desta fatia, reformulado como teste:
//     "emite o formato do docs/uix-dom.md pro corpus inteiro de fixtures que a onda reúne (60,
//     medidas 2026-08-05 -- a RMLX1-CORPUS, TODO.md, fez a 1 fixture original do terceiro
//     diretório crescer com mais 15 telas reais do GusWorld capturadas em runtime, 45 -> 60)".
//     Deliberadamente NÃO reprova determinismo (trabalho próprio do
//     dumper_determinism_sanity.cpp) nem exatidão-de-byte em nível de campo contra o exemplo
//     trabalhado da spec (trabalho próprio do dumper_sanity.cpp) -- o único trabalho deste
//     arquivo é FORMA, em escala de corpus: toda fixture dumpa sem crashar, a primeira linha é
//     sempre um registro HEAD, a saída sempre termina numa única newline final (seção 1 do
//     uix-dom.md), e toda outra linha começa com um dos tipos de registro que a seção 5 define.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dumper.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

using glintfx::uix::dump_document;
using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;

// EN: One line's leading token must be one of these 5 record kinds (uix-dom.md sections 4/5), or
//     the whole line be exactly "HEAD ABSENT" -- a crude but effective shape check independent
//     of this test's own dumper implementation (it re-derives the check from the spec text, not
//     from dumper.cpp's own internals).
// PT: O token inicial de uma linha precisa ser um destes 5 tipos de registro (seções 4/5 do
//     uix-dom.md), ou a linha inteira ser exatamente "HEAD ABSENT" -- uma checagem de forma
//     crua mas eficaz, independente da própria implementação do dumper deste teste (ela
//     re-deriva a checagem do texto da spec, não dos internos do próprio dumper.cpp).
bool line_has_known_record_kind(std::string_view line) {
  static constexpr std::string_view kKinds[] = {" ELEM ", " ID ", " CLASS", " ATTR ",
                                                " CHILDREN ", " TEXT "};
  if (line == "HEAD ABSENT") {
    return true;
  }
  if (line.rfind("HEAD PRESENT", 0) == 0) {
    return true;
  }
  for (std::string_view kind : kKinds) {
    if (line.find(kind) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

void check_dump_shape(const std::string& dump, const fs::path& fixture) {
  check(!dump.empty(), "dump is non-empty: " + fixture.string());
  if (dump.empty()) {
    return;
  }
  check(dump.back() == '\n', "dump ends in a trailing newline (uix-dom.md section 1): " +
                                 fixture.string());
  check(dump.rfind("HEAD ABSENT\n", 0) == 0 || dump.rfind("HEAD PRESENT", 0) == 0,
        "first line is always a HEAD record (uix-dom.md section 3/4): " + fixture.string());

  std::size_t start = 0;
  std::size_t line_no = 0;
  while (start < dump.size()) {
    const std::size_t end = dump.find('\n', start);
    const std::string_view line(dump.data() + start, (end == std::string::npos ? dump.size() : end) - start);
    ++line_no;
    check(line_has_known_record_kind(line),
          "line " + std::to_string(line_no) + " has a recognised record kind: " +
              fixture.string() + " -- \"" + std::string(line) + "\"");
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
}

} // namespace

int main() {
  std::vector<fs::path> fixtures = find_rml_fixtures(GLINTFX_UIX_TESTS_DIR);
  const std::vector<fs::path> demo_fixtures = find_rml_fixtures(GLINTFX_UIX_DEMOS_DIR);
  fixtures.insert(fixtures.end(), demo_fixtures.begin(), demo_fixtures.end());
  const std::vector<fs::path> s3_fixtures = find_rml_fixtures(GLINTFX_UIX_S3_FIXTURES_DIR);
  fixtures.insert(fixtures.end(), s3_fixtures.begin(), s3_fixtures.end());

  // EN: same >= 40 sanity floor as parser_corpus_sanity.cpp -- 60 measured 2026-08-05
  //     (RMLX1-CORPUS, TODO.md, grew the third directory from 1 to 16 real GusWorld fixtures).
  // PT: mesmo piso de sanidade >= 40 do parser_corpus_sanity.cpp -- 60 medidas 2026-08-05 (a
  //     RMLX1-CORPUS, TODO.md, fez o terceiro diretório crescer de 1 pra 16 fixtures reais do
  //     GusWorld).
  check(fixtures.size() >= 40,
        "corpus discovery found a plausible number of .rml fixtures (>= 40) -- directory "
        "wiring sanity");

  // EN: RMLX1-CORPUS (TODO.md) -- same exact-count guard parser_corpus_sanity.cpp added: pins
  //     the third directory's own size so a future accidental deletion (or a fixture silently
  //     not landing here) fails loud instead of hiding inside the >= 40 floor above.
  // PT: RMLX1-CORPUS (TODO.md) -- mesma guarda de contagem exata que o parser_corpus_sanity.cpp
  //     somou: fixa o tamanho do terceiro diretório em si pra uma deleção acidental futura (ou
  //     uma fixture que silenciosamente não caia aqui) falhar alto em vez de se esconder dentro
  //     do piso >= 40 acima.
  check(s3_fixtures.size() == 16,
        "this arc's own third fixture directory (glintfx/src/uix/dom/test_fixtures/) holds "
        "exactly 16 .rml files -- the 1 original gusworld_battle_cockpit.rml plus the 15 real "
        "GusWorld runtime-captured screens RMLX1-CORPUS added, none missing, none extra");

  std::size_t dumped = 0;
  for (const fs::path& fixture : fixtures) {
    const std::string source = read_file_binary(fixture);
    if (source.empty()) {
      check(false, "corpus fixture is unexpectedly empty (read failed?): " + fixture.string());
      continue;
    }

    ParseResult result = parse_document(source);
    if (result.error.has_value()) {
      // EN: not this test's job to re-litigate parser correctness -- parser_corpus_sanity.cpp
      //     already proves every corpus fixture parses cleanly; a fixture this parser rejects
      //     has no tree for this dumper to be exercised against.
      // PT: não é trabalho deste teste rejulgar a corretude do parser -- o
      //     parser_corpus_sanity.cpp já prova que toda fixture do corpus parseia limpo; uma
      //     fixture que este parser rejeita não tem árvore nenhuma pra este dumper ser
      //     exercitado contra.
      continue;
    }
    if (!result.document) {
      check(false, "clean ParseResult still has a non-null document: " + fixture.string());
      continue;
    }

    const std::string dump = dump_document(*result.document);
    check_dump_shape(dump, fixture);
    ++dumped;
  }

  check(dumped >= 40,
        "at least 40 corpus fixtures were actually parsed and dumped, not silently skipped");
  std::printf("dumper_corpus_sanity: %zu/%zu fixtures dumped\n", dumped, fixtures.size());

  if (g_failures > 0) {
    std::fprintf(stderr, "dumper_corpus_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("dumper_corpus_sanity: PASS");
  return 0;
}
