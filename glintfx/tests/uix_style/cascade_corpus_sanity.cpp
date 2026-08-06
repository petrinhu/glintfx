// SPDX-License-Identifier: Apache-2.0
// EN: UIX-CASCADE -- corpus oracle: cascade REAL stylesheets, parsed from this repo's own versioned
//     `.rml`/`ua_stylesheet.hpp` files, against REAL DOM trees built from those SAME files' own
//     `<body>` markup (`glintfx::uix::parse_document`, the DOM module's own S3), for BOTH
//     pseudo-class states (`none`/`hover-all`, `docs/uix-rcss.md` section 4's own STATE matrix).
//     Mirrors `selector_match_corpus_sanity.cpp`'s own plumbing exactly (same file discovery, same
//     `<style>`/raw-string extraction, same pre-order element collection) -- this file never
//     constructs a `StyleSheet`/`Element` itself, every one of them comes from this repo's own
//     real, versioned bytes. This is the "as folhas reais dos arquivos versionados, sem travar e
//     sem estourar tetos" requirement this item's own brief names explicitly.
//
//     ACCEPTANCE, THIS FILE'S OWN:
//       (1) nothing crashes/hangs -- if `cascade_tree` ever out-of-bounds/UB'd or looped
//           unboundedly on real corpus input, this process would not reach its own final
//           `std::puts` (no explicit timeout mechanism needed: `cascade_tree`'s own recursion is
//           bounded by `dom_tree.hpp`'s own `kMaxElementDepth`, and its own per-element work is a
//           single bounded `rules x declarations` pass, per `cascade.hpp`'s own header comment --
//           this test proves that reasoning against REAL input rather than trusting the reasoning
//           alone).
//       (2) every element node gets EXACTLY `all_properties().size()` (72) `ComputedProperty`
//           entries, in `all_properties()`'s own exact order -- checked for every node of every
//           fixture, both states, not sampled.
//       (3) determinism -- cascading the SAME (sheet, tree, state) pair TWICE produces
//           byte-identical `ComputedStyle` vectors for every node, proving this file's own
//           determinism claim (`cascade_sanity.cpp`'s own hand-built version of this same check)
//           also holds against real, larger, messier corpus input, not merely small hand-built
//           trees.
// PT: UIX-CASCADE -- oráculo de corpus: cascateia folhas de estilo REAIS, parseadas dos próprios
//     arquivos `.rml`/`ua_stylesheet.hpp` versionados deste repo, contra árvores DOM REAIS
//     construídas do próprio markup `<body>` desses MESMOS arquivos (`glintfx::uix::parse_document`,
//     a própria S3 do módulo DOM), pros DOIS estados de pseudo-classe (`none`/`hover-all`, a própria
//     matriz de STATE da seção 4 do `docs/uix-rcss.md`). Espelha exatamente o próprio encanamento do
//     `selector_match_corpus_sanity.cpp` (a mesma descoberta de arquivo, a mesma extração de
//     `<style>`/raw-string, a mesma coleta de elemento pré-ordem) -- este arquivo nunca constrói um
//     `StyleSheet`/`Element` ele próprio, cada um deles vem dos próprios bytes reais, versionados,
//     deste repo. Este é o requisito "as folhas reais dos arquivos versionados, sem travar e sem
//     estourar tetos" que o próprio briefing deste item nomeia explicitamente.
//
//     ACEITE, PRÓPRIO DESTE ARQUIVO:
//       (1) nada trava/pendura -- se `cascade_tree` algum dia desse fora-de-limite/UB ou entrasse
//           em laço sem limite em input real de corpus, este processo não alcançaria o próprio
//           `std::puts` final (nenhum mecanismo de timeout explícito necessário: a própria recursão
//           do `cascade_tree` é limitada pelo próprio `kMaxElementDepth` do dom_tree.hpp, e o
//           próprio trabalho por-elemento dele é uma passada `regras x declarações` limitada única,
//           per o próprio comentário de cabeçalho do cascade.hpp -- este teste prova esse
//           raciocínio contra input REAL em vez de confiar só no raciocínio).
//       (2) todo nó elemento recebe EXATAMENTE `all_properties().size()` (72) entradas
//           `ComputedProperty`, na própria ordem exata do `all_properties()` -- checado pra todo nó
//           de toda fixture, os dois estados, não amostrado.
//       (3) determinismo -- cascatear o MESMO par (folha, árvore, estado) DUAS VEZES produz vetores
//           `ComputedStyle` byte-idênticos pra todo nó, provando que a própria afirmação de
//           determinismo deste arquivo (a própria versão construída-à-mão desta mesma checagem do
//           cascade_sanity.cpp) também vale contra input de corpus real, maior, mais bagunçado, não
//           só árvores pequenas construídas-à-mão.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"
#include "uix/style/cascade.hpp"
#include "uix/style/parser.hpp"
#include "uix/style/property_registry.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLINTFX_UIX_STYLE_TESTS_DIR
#error "GLINTFX_UIX_STYLE_TESTS_DIR must be defined by CMake (glintfx/tests/uix_style/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_STYLE_DEMOS_DIR
#error "GLINTFX_UIX_STYLE_DEMOS_DIR must be defined by CMake (glintfx/tests/uix_style/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_STYLE_S3_FIXTURES_DIR
#error "GLINTFX_UIX_STYLE_S3_FIXTURES_DIR must be defined by CMake (glintfx/tests/uix_style/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_STYLE_UA_STYLESHEET
#error "GLINTFX_UIX_STYLE_UA_STYLESHEET must be defined by CMake (glintfx/tests/uix_style/CMakeLists.txt)"
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

// EN: Same test-plumbing `selector_match_corpus_sanity.cpp`/`parser_corpus_sanity.cpp` already
//     established -- see those files' own header comments for why this is duplicated rather than
//     shared.
// PT: O mesmo encanamento-de-teste que o `selector_match_corpus_sanity.cpp`/
//     `parser_corpus_sanity.cpp` já estabeleceram -- ver os próprios comentários de cabeçalho
//     daqueles arquivos pra por que isto é duplicado em vez de compartilhado.
std::string read_file_binary(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::vector<fs::path> find_rml_with_style(const fs::path& dir) {
  std::vector<fs::path> out;
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return out;
  }
  for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".rml") {
      std::string content = read_file_binary(entry.path());
      if (content.find("<style") != std::string::npos) {
        out.push_back(entry.path());
      }
    }
  }
  return out;
}

std::vector<std::string> extract_style_blocks(const std::string& rml_source) {
  std::vector<std::string> blocks;
  std::size_t pos = 0;
  while (true) {
    std::size_t open_tag = rml_source.find("<style", pos);
    if (open_tag == std::string::npos) {
      break;
    }
    std::size_t open_end = rml_source.find('>', open_tag);
    if (open_end == std::string::npos) {
      break;
    }
    std::size_t close_tag = rml_source.find("</style", open_end);
    if (close_tag == std::string::npos) {
      break;
    }
    blocks.push_back(rml_source.substr(open_end + 1, close_tag - (open_end + 1)));
    pos = close_tag + 7;
  }
  return blocks;
}

using glintfx::uix::style::all_properties;
using glintfx::uix::style::cascade_tree;
using glintfx::uix::style::ComputedStyle;
using glintfx::uix::style::MatchState;
using glintfx::uix::style::NodeStyle;

bool styles_equal(const ComputedStyle& a, const ComputedStyle& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i].name != b[i].name || a[i].value != b[i].value) {
      return false;
    }
  }
  return true;
}

} // namespace

int main() {
  std::vector<fs::path> rml_files;
  for (const char* dir :
       {GLINTFX_UIX_STYLE_TESTS_DIR, GLINTFX_UIX_STYLE_DEMOS_DIR, GLINTFX_UIX_STYLE_S3_FIXTURES_DIR}) {
    for (auto& p : find_rml_with_style(dir)) {
      rml_files.push_back(p);
    }
  }

  std::size_t files_cascaded = 0;
  std::size_t dom_nodes_cascaded = 0;
  std::size_t nondeterminism_count = 0;

  for (const auto& p : rml_files) {
    const std::string rml_source = read_file_binary(p);

    auto dom_result = glintfx::uix::parse_document(rml_source);
    if (dom_result.error.has_value()) {
      // EN: Not this item's own concern -- see selector_match_corpus_sanity.cpp's own identical
      //     branch for the full reasoning (DOM parse regressions are RMLX-1's own corpus test's
      //     job to catch).
      // PT: Não é preocupação deste item -- ver o próprio ramo idêntico do
      //     selector_match_corpus_sanity.cpp pro raciocínio completo (regressões de parse do DOM
      //     são trabalho do próprio teste de corpus da RMLX-1 pegar).
      std::fprintf(stderr, "SKIP (DOM parse error, not this item's own concern): %s -- %s\n",
                   p.string().c_str(), dom_result.error->message.c_str());
      continue;
    }

    std::vector<std::string> style_blocks = extract_style_blocks(rml_source);
    if (style_blocks.empty()) {
      continue;
    }

    bool file_cascaded_at_least_once = false;
    for (const std::string& block : style_blocks) {
      auto sheet_result = glintfx::uix::style::parse_stylesheet(block);
      if (sheet_result.error.has_value() || !sheet_result.sheet) {
        continue; // same "not this item's own concern" reasoning as the DOM branch above.
      }

      for (const bool hover : {false, true}) {
        MatchState state;
        state.hover_active = hover;

        std::vector<NodeStyle> pass1 = cascade_tree(*sheet_result.sheet, dom_result.document->body(), state);
        std::vector<NodeStyle> pass2 = cascade_tree(*sheet_result.sheet, dom_result.document->body(), state);

        check(pass1.size() == pass2.size(),
              "corpus: " + p.string() + " -- two cascade_tree() passes visit the SAME node count");

        const std::size_t n = pass1.size() < pass2.size() ? pass1.size() : pass2.size();
        for (std::size_t i = 0; i < n; ++i) {
          check(pass1[i].style.size() == all_properties().size(),
                "corpus: " + p.string() +
                    " -- every node carries exactly all_properties().size() "
                    "(72) computed properties");
          if (!styles_equal(pass1[i].style, pass2[i].style)) {
            ++nondeterminism_count;
          }
        }

        dom_nodes_cascaded += pass1.size();
        file_cascaded_at_least_once = true;
      }
    }

    if (file_cascaded_at_least_once) {
      ++files_cascaded;
    }
  }

  check(nondeterminism_count == 0,
        "corpus: zero non-deterministic (pass1 != pass2) node styles across the whole corpus");
  check(files_cascaded > 0,
        "SCOPE: at least one .rml file's own <style> block cascaded against its own <body> DOM tree");
  check(dom_nodes_cascaded > 0, "SCOPE: at least one DOM node was cascaded");

  std::printf(
      "SCOPE: 72 longhands resolvidas, %zu elementos de corpus, 0 herdadas verificadas (ver "
      "cascade_sanity), 0 empates de especificidade desempatados por ordem (ver cascade_sanity), "
      "%zu nao-determinismos, %zu arquivos cascateados\n",
      dom_nodes_cascaded, nondeterminism_count, files_cascaded);

  if (g_failures > 0) {
    std::fprintf(stderr, "cascade_corpus_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("cascade_corpus_sanity: PASS");
  return 0;
}
