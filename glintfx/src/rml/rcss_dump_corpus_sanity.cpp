// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-RCSS-DUMP-A` -- corpus oracle for `rcss_dump_document()`: loads every real `.rml`
//     fixture already in this repo (`glintfx/tests/*.rml` + `glintfx/demos/showcase/*.rml` -- the
//     SAME two directories `dom_dump_corpus_sanity.cpp`'s own `RMLX-1` sibling already enumerates,
//     same "the corpus decides, this test proves it, never contorts around it" discipline) through
//     the REAL RmlUi engine, dumps BOTH `STATE` blocks per fixture, and asserts a handful of
//     format-level invariants from docs/uix-rcss.md section 1/3/4 that must hold for EVERY
//     fixture: non-empty output, exactly two `STATE` blocks in the right order/names, and every
//     `PROPS` count equal to the registry's own fixed size. This is a STRUCTURAL smoke test, not
//     a byte-exact oracle (there is no independent side-B output to diff against yet -- that is
//     a future `RMLX-2` slice's job) -- see `rcss_dump_worked_examples.cpp` (same directory) for
//     the byte-exact, spec-worked-example tests that pin the format's actual field values.
//
//     Lives in glintfx/src/rml/ (NOT glintfx/tests/) for the SAME confinement reason
//     `dom_dump_corpus_sanity.cpp`'s own header comment states -- this file drives
//     `Rml::Context`/`Rml::ElementDocument` directly, which `tools/check_rml_whitelist.sh`
//     forbids in a NEW file under `glintfx/tests/`.
//
//     🔴 Scope line, printed even when every count is zero -- "zero declared distinguishes
//     'nothing found' from 'nobody looked'" per this task's own definition-of-done: "N fixtures,
//     M estados, K nós, 0 erros internos".
// PT: `UIX-RCSS-DUMP-A` -- oráculo de corpus para `rcss_dump_document()`: carrega toda fixture
//     `.rml` real já deste repo (`glintfx/tests/*.rml` + `glintfx/demos/showcase/*.rml` -- os
//     MESMOS dois diretórios que o próprio irmão `RMLX-1` `dom_dump_corpus_sanity.cpp` já
//     enumera, mesma disciplina "o corpus decide, este teste prova, nunca contorna") através do
//     motor REAL do RmlUi, dumpa OS DOIS blocos `STATE` por fixture, e verifica um punhado de
//     invariantes de nível-formato da seção 1/3/4 do docs/uix-rcss.md que têm que valer pra TODA
//     fixture: saída não-vazia, exatamente dois blocos `STATE` na ordem/nomes certos, e toda
//     contagem `PROPS` igual ao tamanho fixo do próprio registro. Isto é um teste de fumaça
//     ESTRUTURAL, não um oráculo byte-exato (não há saída de lado-B independente pra diffar
//     contra ainda -- isso é trabalho de uma fatia futura da `RMLX-2`) -- ver
//     `rcss_dump_worked_examples.cpp` (mesmo diretório) pros testes byte-exatos,
//     de-exemplo-trabalhado-da-spec, que fixam os valores de campo reais do formato.
//
//     Mora em glintfx/src/rml/ (NÃO glintfx/tests/) pelo MESMO motivo de confinamento que o
//     próprio comentário de cabeçalho de dom_dump_corpus_sanity.cpp declara -- este arquivo
//     dirige `Rml::Context`/`Rml::ElementDocument` direto, o que `tools/check_rml_whitelist.sh`
//     proíbe num arquivo NOVO sob `glintfx/tests/`.
//
//     🔴 Linha de escopo, impressa mesmo quando toda contagem é zero -- "zero declarado distingue
//     'nada achado' de 'ninguém olhou'" pela própria definição-de-pronto desta tarefa: "N
//     fixtures, M estados, K nós, 0 erros internos".
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "rcss_dump.hpp"
#include "system_clock.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
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
int g_internal_errors = 0;
std::size_t g_states_seen = 0;
std::size_t g_nodes_seen = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

std::vector<fs::path> rml_fixtures_in(const fs::path& dir) {
  std::vector<fs::path> out;
  if (!fs::exists(dir)) return out;
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".rml") out.push_back(entry.path());
  }
  return out;
}

// EN: Counts `STATE ` occurrences and `PROPS ` lines in `dump`, both used below for the scope
//     line and the per-fixture structural checks.
// PT: Conta ocorrências de `STATE ` e linhas `PROPS ` em `dump`, ambas usadas abaixo pra linha de
//     escopo e as checagens estruturais por-fixture.
std::size_t count_occurrences(const std::string& haystack, const std::string& needle) {
  std::size_t count = 0, pos = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

} // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("rcss-dump-corpus", 320, 240)) {
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
        "corpus is non-empty (found 0 .rml fixtures under GLINTFX_RML_TESTS_DIR/GLINTFX_RML_DEMOS_DIR)");

  for (const fs::path& path : fixtures) {
    const std::string abspath = path.string();

    if (!engine.load(abspath.c_str())) {
      check(false, abspath + ": engine.load() returned false");
      ++g_internal_errors;
      continue;
    }
    engine.update();

    Rml::ElementDocument* doc = ctx->GetDocument(0);
    if (!doc) {
      check(false, abspath + ": GetDocument(0) returned null after a successful load()");
      ++g_internal_errors;
      continue;
    }

    const std::string dump = glintfx::rcss_dump_document(doc);

    check(!dump.empty(), abspath + ": rcss_dump_document() produced non-empty output");

    const std::size_t none_pos = dump.find("STATE none\n");
    const std::size_t hover_pos = dump.find("STATE hover-all\n");
    check(none_pos == 0, abspath + ": dump starts with 'STATE none' (section 3/4)");
    check(hover_pos != std::string::npos && (none_pos == std::string::npos || hover_pos > none_pos),
          abspath + ": 'STATE hover-all' present and after 'STATE none' (section 4's two-row matrix)");

    const std::size_t state_count = count_occurrences(dump, "STATE ");
    check(state_count == 2, abspath + ": exactly 2 STATE blocks (section 4 -- not more, not fewer)");
    g_states_seen += state_count;

    const std::size_t props_count = count_occurrences(dump, " PROPS ");
    g_nodes_seen += props_count;
    check(props_count > 0, abspath + ": at least one PROPS block emitted");

    // EN: Every `PROPS <n>` line must carry the SAME fixed `n` (section 3: "never fewer, because
    //     this dump enumerates the whole registry per node") -- spot-check via the first
    //     occurrence's own digit run, then require every subsequent one to match it verbatim.
    // PT: Toda linha `PROPS <n>` tem que carregar o MESMO `n` fixo (seção 3: "nunca menos,
    //     porque este dump enumera o registro inteiro por nó") -- spot-check via a própria
    //     sequência de dígitos da primeira ocorrência, depois exige que toda seguinte bata ao pé
    //     da letra.
    {
      const std::string needle = " PROPS ";
      std::size_t pos = dump.find(needle);
      std::string first_n;
      bool all_match = true;
      while (pos != std::string::npos) {
        const std::size_t n_start = pos + needle.size();
        std::size_t n_end = n_start;
        while (n_end < dump.size() && dump[n_end] >= '0' && dump[n_end] <= '9') ++n_end;
        const std::string n = dump.substr(n_start, n_end - n_start);
        if (first_n.empty()) {
          first_n = n;
        } else if (n != first_n) {
          all_match = false;
        }
        pos = dump.find(needle, n_end);
      }
      check(all_match, abspath + ": every PROPS <n> shares the same fixed registry size");
    }
  }

  // EN: The scope line -- printed even when a count is zero, per this task's own definition of
  //     done ("N fixtures, M estados, K nós, 0 erros internos").
  // PT: A linha de escopo -- impressa mesmo quando uma contagem é zero, pela própria
  //     definição-de-pronto desta tarefa ("N fixtures, M estados, K nós, 0 erros internos").
  //
  // EN: CI-TIDY-CRASH-2 (2026-08-06): the word "nós" (UTF-8) was inline in the format
  //     literal below and crashed clang-tidy 18's modernize-use-std-print check with a
  //     SIGSEGV -- its FormatStringConverter reads the UTF-8 continuation byte as a
  //     negative signed char, which passes the `< 32` control-char test and then
  //     indexes llvm::hexdigit() out of bounds (llvm/llvm-project#169198, fixed by
  //     #169215). Moved the word out to a %s argument so the parser never scans a
  //     non-ASCII byte from inside the format string. Printed text is byte-identical.
  //     Safe to move back inline once CI's clang-tidy carries the #169215 fix.
  // PT: CI-TIDY-CRASH-2 (2026-08-06): a palavra "nós" (UTF-8) estava dentro do literal
  //     de formatação abaixo e derrubava o check modernize-use-std-print do clang-tidy
  //     18 com SIGSEGV -- o FormatStringConverter dele lê o byte de continuação UTF-8
  //     como char assinado negativo, que passa no teste `< 32` de caractere-de-controle
  //     e então indexa llvm::hexdigit() fora dos limites (llvm/llvm-project#169198,
  //     corrigido pelo #169215). Movi a palavra para um argumento %s pra o parser nunca
  //     escanear byte não-ASCII de dentro do format-string. Texto impresso é
  //     byte-idêntico. Pode voltar pro literal quando o CI usar clang-tidy com o
  //     #169215.
  std::printf("rcss_dump_corpus_sanity: %zu fixtures, %zu estados, %zu %s, %d erros internos\n", fixtures.size(), g_states_seen, g_nodes_seen,
              "nós", g_internal_errors);

  if (g_failures == 0) {
    std::puts("rcss_dump_corpus_sanity OK");
    return 0;
  }
  std::printf("rcss_dump_corpus_sanity: %d failure(s)\n", g_failures);
  return 10;
}
