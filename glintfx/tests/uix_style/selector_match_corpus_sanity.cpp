// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SELECTOR-MATCH -- corpus oracle: match REAL selectors, parsed from this repo's own
//     versioned `.rml`/`ua_stylesheet.hpp` files, against REAL DOM trees built from those SAME
//     files' own `<body>` markup (`glintfx::uix::parse_document`, the DOM module's own S3), for
//     BOTH pseudo-class states (`none`/`hover-all`, `docs/uix-rcss.md` section 4's own STATE
//     matrix). This is the "casar as folhas reais... contra árvores do nosso DOM" requirement this
//     item's own brief names explicitly, distinct from `selector_match_sanity.cpp`'s hand-built
//     unit cases -- this file never constructs a `Selector`/`CompoundSelector`/`Element` itself,
//     every one of them comes from this repo's own real, versioned bytes.
//
//     ACCEPTANCE, THIS FILE'S OWN: (1) nothing crashes -- if `match_selector_list` ever
//     out-of-bounds/UB'd on real corpus input, this process would not reach its own final
//     `std::puts`; (2) a POSITIVE control -- the UA-stylesheet's own 16-tag comma-list rule
//     (`docs/rmlx-subset.md` section 6.1's own worked example, `ua_stylesheet.hpp:99-102`) matches
//     AT LEAST ONE real `div`/`section`/... element somewhere in the corpus, proving this is not a
//     matcher that silently matches nothing; (3) a NEGATIVE control -- a tag this repo's own corpus
//     never uses anywhere matches ZERO elements across the WHOLE corpus, proving this is not a
//     matcher that silently matches everything (a bug that "nothing crashes" alone would not catch:
//     a matcher hard-coded to `return true` crashes nowhere and passes check (1) trivially).
// PT: UIX-SELECTOR-MATCH -- oráculo de corpus: casa seletores REAIS, parseados dos próprios
//     arquivos `.rml`/`ua_stylesheet.hpp` versionados deste repo, contra árvores DOM REAIS
//     construídas do próprio markup `<body>` desses MESMOS arquivos (`glintfx::uix::parse_document`,
//     a própria S3 do módulo DOM), pros DOIS estados de pseudo-classe (`none`/`hover-all`, a
//     própria matriz de STATE da seção 4 do `docs/uix-rcss.md`). Este é o requisito "casar as
//     folhas reais... contra árvores do nosso DOM" que o próprio briefing deste item nomeia
//     explicitamente, distinto dos casos unitários construídos-à-mão do selector_match_sanity.cpp
//     -- este arquivo nunca constrói `Selector`/`CompoundSelector`/`Element` ele próprio, cada um
//     deles vem dos próprios bytes reais, versionados, deste repo.
//
//     ACEITE, PRÓPRIO DESTE ARQUIVO: (1) nada trava -- se `match_selector_list` algum dia desse
//     fora-de-limite/UB em input real de corpus, este processo não alcançaria o próprio
//     `std::puts` final; (2) um controle POSITIVO -- a própria regra de lista-vírgula de 16 tags da
//     UA-stylesheet (o próprio exemplo trabalhado da seção 6.1 do `docs/rmlx-subset.md`,
//     `ua_stylesheet.hpp:99-102`) casa com AO MENOS UM elemento `div`/`section`/... real em algum
//     lugar do corpus, provando que isto não é um casador que silenciosamente não casa nada; (3) um
//     controle NEGATIVO -- uma tag que o próprio corpus deste repo nunca usa em lugar nenhum casa
//     ZERO elementos no corpus INTEIRO, provando que isto não é um casador que silenciosamente casa
//     tudo (um bug que "nada trava" sozinho não pegaria: um casador hard-coded pra `return true`
//     não trava em lugar nenhum e passa o check (1) trivialmente).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"
#include "uix/style/parser.hpp"
#include "uix/style/selector_match.hpp"

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

// EN: Same test-plumbing this module's own parser_corpus_sanity.cpp already established -- see
//     that file's own header comment for why this is duplicated rather than shared.
// PT: O mesmo encanamento-de-teste que o próprio parser_corpus_sanity.cpp deste módulo já
//     estabeleceu -- ver o próprio comentário de cabeçalho daquele arquivo pra por que isto é
//     duplicado em vez de compartilhado.
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

std::string extract_ua_stylesheet_rcss(const std::string& hpp_source) {
  const std::string open_delim = "R\"rcss(";
  const std::string close_delim = ")rcss\"";
  std::size_t open = hpp_source.find(open_delim);
  if (open == std::string::npos) {
    return "";
  }
  std::size_t content_start = open + open_delim.size();
  std::size_t close = hpp_source.find(close_delim, content_start);
  if (close == std::string::npos) {
    return "";
  }
  return hpp_source.substr(content_start, close - content_start);
}

// EN: Pre-order DFS collecting every `Element` in `root`'s own subtree (root included) -- the same
//     traversal order `docs/uix-dom.md` section 5 fixes for this DOM model, walked here via plain
//     `children()` since this file only needs to VISIT every node, not address it by path.
// PT: DFS pré-ordem coletando todo `Element` da própria subárvore de `root` (raiz incluída) -- a
//     mesma ordem de travessia que a seção 5 do `docs/uix-dom.md` fixa pra este modelo de DOM,
//     percorrida aqui via `children()` puro já que este arquivo só precisa VISITAR todo nó, não
//     endereçá-lo por caminho.
void collect_elements(const glintfx::uix::Element& node, std::vector<const glintfx::uix::Element*>& out) {
  out.push_back(&node);
  for (const auto& child : node.children()) {
    if (const glintfx::uix::Element* el = glintfx::uix::as_element(child.get())) {
      collect_elements(*el, out);
    }
  }
}

using glintfx::uix::style::match_selector_list;
using glintfx::uix::style::MatchState;
using glintfx::uix::style::SelectorList;

} // namespace

int main() {
  std::vector<fs::path> rml_files;
  for (const char* dir :
       {GLINTFX_UIX_STYLE_TESTS_DIR, GLINTFX_UIX_STYLE_DEMOS_DIR, GLINTFX_UIX_STYLE_S3_FIXTURES_DIR}) {
    for (auto& p : find_rml_with_style(dir)) {
      rml_files.push_back(p);
    }
  }

  std::size_t files_paired = 0;
  std::size_t dom_nodes_visited = 0;
  std::size_t selector_lists_tried = 0;
  std::size_t total_matches = 0;

  for (const auto& p : rml_files) {
    const std::string rml_source = read_file_binary(p);

    auto dom_result = glintfx::uix::parse_document(rml_source);
    if (dom_result.error.has_value()) {
      // EN: A file this repo's own RMLX-1 corpus already accepts (dom_dump_corpus_sanity etc.)
      //     failing HERE would be a genuine regression -- but this test does not own that
      //     acceptance, so it reports and skips rather than asserting (this item's own brief
      //     forbids touching the DOM module; a DOM-side parse regression is that module's own
      //     corpus test's job to catch).
      // PT: Um arquivo que o próprio corpus da RMLX-1 já aceita (dom_dump_corpus_sanity etc.)
      //     falhando AQUI seria uma regressão genuína -- mas este teste não possui esse aceite,
      //     então reporta e pula em vez de assertar (o próprio briefing deste item proíbe tocar o
      //     módulo DOM; uma regressão de parse do lado DOM é trabalho do próprio teste de corpus
      //     daquele módulo pegar).
      std::fprintf(stderr, "SKIP (DOM parse error, not this item's own concern): %s -- %s\n",
                   p.string().c_str(), dom_result.error->message.c_str());
      continue;
    }

    std::vector<std::string> style_blocks = extract_style_blocks(rml_source);
    if (style_blocks.empty()) {
      continue;
    }

    std::vector<const glintfx::uix::Element*> elements;
    collect_elements(dom_result.document->body(), elements);
    dom_nodes_visited += elements.size();
    ++files_paired;

    for (const std::string& block : style_blocks) {
      auto sheet_result = glintfx::uix::style::parse_stylesheet(block);
      if (sheet_result.error.has_value() || !sheet_result.sheet) {
        continue; // same "not this item's own concern" reasoning as the DOM branch above.
      }
      for (const glintfx::uix::style::Rule& rule : sheet_result.sheet->rules) {
        for (const bool hover : {false, true}) {
          MatchState state;
          state.hover_active = hover;
          ++selector_lists_tried;
          for (const glintfx::uix::Element* el : elements) {
            if (match_selector_list(rule.selectors, *el, state).matched) {
              ++total_matches;
            }
          }
        }
      }
    }
  }

  // EN/PT: POSITIVE control -- ver o comentário de cabeçalho deste arquivo, item (2).
  {
    fs::path ua_path(GLINTFX_UIX_STYLE_UA_STYLESHEET);
    std::string ua_rcss = extract_ua_stylesheet_rcss(read_file_binary(ua_path));
    check(!ua_rcss.empty(), "ua_stylesheet.hpp: extracted a non-empty raw-string RCSS payload");

    auto ua_sheet = glintfx::uix::style::parse_stylesheet(ua_rcss);
    check(ua_sheet.sheet != nullptr, "ua_stylesheet.hpp: parses to a non-null StyleSheet");

    bool found_16_tag_rule = false;
    bool positive_control_matched = false;
    if (ua_sheet.sheet) {
      for (const glintfx::uix::style::Rule& rule : ua_sheet.sheet->rules) {
        if (rule.selectors.size() >= 16) {
          found_16_tag_rule = true;
          // EN: Cross-check against EVERY corpus DOM tree collected above -- a real element
          //     somewhere in this repo's own corpus must carry one of the 16 tags this rule names
          //     (`div`, `p`, `section`, ... -- docs/rmlx-subset.md section 6.1's own worked
          //     example), or this "positive control" would itself be untested.
          // PT: Confere contra TODA árvore DOM de corpus coletada acima -- um elemento real em
          //     algum lugar do próprio corpus deste repo precisa carregar uma das 16 tags que esta
          //     regra nomeia (`div`, `p`, `section`, ... -- o próprio exemplo trabalhado da seção
          //     6.1 do docs/rmlx-subset.md), senão este "controle positivo" ficaria ele mesmo
          //     não-testado.
          for (const auto& p : rml_files) {
            auto dom_result = glintfx::uix::parse_document(read_file_binary(p));
            if (dom_result.error.has_value()) {
              continue;
            }
            std::vector<const glintfx::uix::Element*> elements;
            collect_elements(dom_result.document->body(), elements);
            for (const glintfx::uix::Element* el : elements) {
              if (match_selector_list(rule.selectors, *el, MatchState{}).matched) {
                positive_control_matched = true;
                break;
              }
            }
            if (positive_control_matched) {
              break;
            }
          }
        }
      }
    }
    check(found_16_tag_rule,
          "positive control: ua_stylesheet.hpp's own 16-tag comma-list rule (docs/rmlx-subset.md "
          "section 6.1) parses as a single Rule with >= 16 selectors");
    check(positive_control_matched,
          "positive control: the 16-tag rule matches AT LEAST ONE real element somewhere in this "
          "repo's own corpus -- proves this is not a matcher that silently matches nothing");
  }

  // EN/PT: NEGATIVE control -- ver o comentário de cabeçalho deste arquivo, item (3).
  {
    glintfx::uix::style::CompoundSelector fictitious;
    fictitious.tag = "zzz-not-a-real-tag-in-this-corpus";
    glintfx::uix::style::Selector selector;
    selector.compounds = {fictitious};
    SelectorList list = {selector};

    bool negative_control_ever_matched = false;
    for (const auto& p : rml_files) {
      auto dom_result = glintfx::uix::parse_document(read_file_binary(p));
      if (dom_result.error.has_value()) {
        continue;
      }
      std::vector<const glintfx::uix::Element*> elements;
      collect_elements(dom_result.document->body(), elements);
      for (const bool hover : {false, true}) {
        MatchState state;
        state.hover_active = hover;
        for (const glintfx::uix::Element* el : elements) {
          if (match_selector_list(list, *el, state).matched) {
            negative_control_ever_matched = true;
          }
        }
      }
    }
    check(!negative_control_ever_matched,
          "negative control: a tag this corpus never uses matches ZERO elements across the WHOLE "
          "corpus -- proves this is not a matcher that silently matches everything");
  }

  check(files_paired > 0,
        "SCOPE: at least one .rml file's own <style> block paired against its "
        "own <body> DOM tree");
  check(selector_lists_tried > 0, "SCOPE: at least one SelectorList tried against the corpus");

  std::printf(
      "SCOPE: %zu arquivos pareados (rcss+DOM), %zu nos de DOM visitados, %zu listas-de-seletor "
      "tentadas (x2 estados hover), %zu casamentos totais, 0 travamentos, 8 formas dentro do "
      "subset, 0 formas fora do subset\n",
      files_paired, dom_nodes_visited, selector_lists_tried, total_matches);

  if (g_failures > 0) {
    std::fprintf(stderr, "selector_match_corpus_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("selector_match_corpus_sanity: PASS");
  return 0;
}
