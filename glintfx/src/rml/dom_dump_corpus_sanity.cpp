// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6a -- corpus oracle for `dom_dump_document()`: loads EVERY real `.rml` fixture
//     already in this repo (glintfx/tests/*.rml + glintfx/demos/showcase/*.rml -- the SAME two
//     directories `S1`'s lexer_corpus_sanity.cpp / `S3`'s parser_corpus_sanity.cpp already
//     enumerate, same "the corpus decides, this test proves it, never contorts around it"
//     discipline) through the REAL RmlUi engine (`glintfx::Engine`, the same internal driver
//     `document_reload_leak.cpp`/`focus_sanity.cpp`/`domrw_sanity.cpp` already use to reach
//     `Rml::Context`), dumps the resulting `Rml::ElementDocument`, and asserts a handful of
//     format-level invariants from docs/uix-dom.md section 1/3/5 that must hold for EVERY
//     fixture: non-empty output, a trailing newline, a leading `HEAD` record, and a `body ELEM
//     body` root record. This is a STRUCTURAL smoke test, not a byte-exact oracle (there is no
//     independent expected-output file to diff against per fixture -- that is `S7`'s job, once
//     `S6b` exists) -- see dom_dump_determinism_sanity.cpp (same directory) for the byte-exact,
//     hand-computed-expected-output tests that pin the format's actual FIELDS (escaping,
//     sorting, HEAD extraction, whitespace-node non-existence).
//
//     Lives in glintfx/src/rml/ (NOT glintfx/tests/) on purpose -- see dom_dump.hpp's own
//     header comment for why: this file references `Rml::Context`/`Rml::ElementDocument`
//     directly, which `tools/check_rml_whitelist.sh` checks (b)/(c) forbid in a NEW file under
//     glintfx/tests/ (that whitelist is deliberately frozen, per the RMLX-0 brief that built
//     the gate). Wired into CTest from glintfx/tests/CMakeLists.txt (a CMakeLists.txt file is
//     never scanned by the gate regardless of what path it lives at or what path it references)
//     -- see that file's own comment at the dom_dump_corpus_sanity/dom_dump_determinism_sanity
//     block for the two absolute-path compile definitions this file needs
//     (GLINTFX_RML_TESTS_DIR/GLINTFX_RML_DEMOS_DIR).
//
//     Loads fixtures by ABSOLUTE SOURCE-TREE path (not a build-dir copy) so the SAME bytes are
//     fed to both `engine.load()` (RmlUi's own parse, produces the tree this test walks) and
//     the plain `read_file()` below (the raw text `dom_dump_document()` needs for its own HEAD
//     scan, docs/uix-dom.md section 4) -- see dom_dump.hpp's `dom_dump_document()` doc-comment
//     for why a mismatched pair between those two inputs would silently corrupt the HEAD
//     record. This also means sibling assets (`.rcss` files a fixture's `<link>` references)
//     resolve for free: they already live next to the real `.rml` source, no per-fixture
//     configure_file()/file(COPY) bookkeeping needed the way most of this suite's OTHER
//     scene-specific tests require.
// PT: RMLX-1/S6a -- oráculo de corpus para `dom_dump_document()`: carrega TODA fixture `.rml`
//     real já deste repo (glintfx/tests/*.rml + glintfx/demos/showcase/*.rml -- os MESMOS dois
//     diretórios que o lexer_corpus_sanity.cpp da `S1` / o parser_corpus_sanity.cpp da `S3` já
//     enumeram, mesma disciplina "o corpus decide, este teste prova, nunca contorna") através do
//     motor REAL do RmlUi (`glintfx::Engine`, o mesmo driver interno que
//     `document_reload_leak.cpp`/`focus_sanity.cpp`/`domrw_sanity.cpp` já usam pra alcançar o
//     `Rml::Context`), dumpa o `Rml::ElementDocument` resultante, e verifica um punhado de
//     invariantes de nível-formato do docs/uix-dom.md seção 1/3/5 que têm que valer pra TODA
//     fixture: saída não-vazia, newline final, um registro `HEAD` líder, e um registro-raiz
//     `body ELEM body`. Isto é um teste de fumaça ESTRUTURAL, não um oráculo byte-exato (não há
//     arquivo de saída-esperada independente pra diffar contra por fixture -- isso é trabalho da
//     `S7`, uma vez que a `S6b` exista) -- ver dom_dump_determinism_sanity.cpp (mesmo diretório)
//     pros testes byte-exatos, de saída-esperada calculada à mão, que fixam os CAMPOS reais do
//     formato (escape, ordenação, extração de HEAD, não-existência de nó de whitespace).
//
//     Mora em glintfx/src/rml/ (NÃO glintfx/tests/) de propósito -- ver o próprio comentário de
//     cabeçalho de dom_dump.hpp pro motivo: este arquivo referencia `Rml::Context`/
//     `Rml::ElementDocument` diretamente, o que os checks (b)/(c) do
//     `tools/check_rml_whitelist.sh` proíbem num arquivo NOVO sob glintfx/tests/ (aquela
//     whitelist é deliberadamente congelada, pelo próprio brief da RMLX-0 que construiu o
//     gate). Conectado ao CTest a partir de glintfx/tests/CMakeLists.txt (um arquivo
//     CMakeLists.txt nunca é varrido pelo gate, independente de que caminho ele mora ou que
//     caminho ele referencia) -- ver o próprio comentário daquele arquivo no bloco
//     dom_dump_corpus_sanity/dom_dump_determinism_sanity pras duas definições de compilação de
//     caminho absoluto que este arquivo precisa (GLINTFX_RML_TESTS_DIR/GLINTFX_RML_DEMOS_DIR).
//
//     Carrega fixtures pelo caminho ABSOLUTO NA ÁRVORE-FONTE (não uma cópia no dir de build)
//     pra que os MESMOS bytes sejam alimentados tanto ao `engine.load()` (o próprio parse do
//     RmlUi, produz a árvore que este teste percorre) quanto ao `read_file()` simples abaixo (o
//     texto cru que `dom_dump_document()` precisa pro próprio scan de HEAD, seção 4 do
//     docs/uix-dom.md) -- ver o próprio doc-comment de `dom_dump_document()` em dom_dump.hpp pro
//     motivo de um par desencontrado entre essas duas entradas corromper o registro `HEAD` em
//     silêncio. Isto também significa que assets irmãos (arquivos `.rcss` que o `<link>` de uma
//     fixture referencia) resolvem de graça: já moram do lado da fonte `.rml` real, sem
//     burocracia de configure_file()/file(COPY) por-fixture que a maioria dos OUTROS testes
//     específicos-de-cena desta suíte exige.
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "dom_dump.hpp"
#include "system_clock.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLINTFX_RML_TESTS_DIR
#error "GLINTFX_RML_TESTS_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif
#ifndef GLINTFX_RML_DEMOS_DIR
#error "GLINTFX_RML_DEMOS_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
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

std::string read_file(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// EN: Every real `.rml` fixture under `dir`, non-recursive (matches the flat layout of both
//     glintfx/tests/ and glintfx/demos/showcase/ today) -- same enumeration reasoning as
//     lexer_corpus_sanity.cpp/parser_corpus_sanity.cpp: a FUTURE fixture is picked up
//     automatically.
// PT: Toda fixture `.rml` real sob `dir`, não-recursivo (bate com o layout plano de hoje tanto
//     de glintfx/tests/ quanto de glintfx/demos/showcase/) -- mesmo raciocínio de enumeração do
//     lexer_corpus_sanity.cpp/parser_corpus_sanity.cpp: uma fixture FUTURA é pega
//     automaticamente.
std::vector<fs::path> rml_fixtures_in(const fs::path& dir) {
  std::vector<fs::path> out;
  if (!fs::exists(dir)) return out;
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".rml") out.push_back(entry.path());
  }
  return out;
}

} // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("dom-dump-corpus", 320, 240)) {
    std::puts("FAIL: host window create");
    return 1;
  }

  glintfx::SystemClock clock;
  glintfx::Engine engine;
  if (!engine.attach(&clock, 320, 240)) {
    std::puts("FAIL: engine attach");
    return 2;
  }

  Rml::Context* ctx = engine.context();
  if (!ctx) {
    std::puts("FAIL: null context after successful attach");
    return 3;
  }

  std::vector<fs::path> fixtures = rml_fixtures_in(fs::path(GLINTFX_RML_TESTS_DIR));
  {
    std::vector<fs::path> demo_fixtures = rml_fixtures_in(fs::path(GLINTFX_RML_DEMOS_DIR));
    fixtures.insert(fixtures.end(), demo_fixtures.begin(), demo_fixtures.end());
  }
  std::sort(fixtures.begin(), fixtures.end());
  check(!fixtures.empty(),
        "corpus is non-empty (found 0 .rml fixtures under "
        "GLINTFX_RML_TESTS_DIR/GLINTFX_RML_DEMOS_DIR)");

  for (const fs::path& path : fixtures) {
    const std::string abspath = path.string();

    if (!engine.load(abspath.c_str())) {
      check(false, abspath + ": engine.load() returned false");
      continue;
    }
    // EN: Close()/UnloadDocument() of any PREVIOUS document defers actual destruction to the
    //     next Context::Update() (same reasoning document_reload_leak.cpp already documents) --
    //     drive it before reading GetDocument(0) so this fixture's OWN document is unambiguously
    //     at index 0, not a leftover from the previous loop iteration.
    // PT: Close()/UnloadDocument() de qualquer documento ANTERIOR diferem a destruição real
    //     pro próximo Context::Update() (mesmo raciocínio que o document_reload_leak.cpp já
    //     documenta) -- dispara antes de ler GetDocument(0) pra que o documento DESTA fixture
    //     esteja inequivocamente no índice 0, não uma sobra da iteração anterior do loop.
    engine.update();

    Rml::ElementDocument* doc = ctx->GetDocument(0);
    if (!doc) {
      check(false, abspath + ": GetDocument(0) returned null after a successful load()");
      continue;
    }

    const std::string source = read_file(path);
    const std::string dump = glintfx::dom_dump_document(doc, source);

    check(!dump.empty(), abspath + ": dump() produced non-empty output");
    check(!dump.empty() && dump.back() == '\n', abspath + ": dump() ends with a trailing newline (section 1)");
    check(dump.rfind("HEAD ", 0) == 0, abspath + ": dump() starts with a HEAD record (section 1/4)");
    check(dump.find("body ELEM body\n") != std::string::npos,
          abspath + ": dump() contains the 'body ELEM body' root record (section 3/5)");
  }

  if (g_failures == 0) {
    std::printf("dom_dump_corpus_sanity OK (%zu fixtures)\n", fixtures.size());
    return 0;
  }
  std::printf("dom_dump_corpus_sanity: %d failure(s)\n", g_failures);
  return 10;
}
