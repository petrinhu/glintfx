// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- regression oracle: this lexer must tokenize EVERY real `.rml` fixture in this
//     repo (glintfx/tests/ + glintfx/demos/) to a clean EndOfFile, never an Error. This is the
//     literal enforcement of this slice's own "the corpus decides" discipline and of
//     docs/rmlx-subset.md's frozen-boundary clause -- if a real fixture makes this lexer emit
//     Error, that is either (a) a bug in this lexer, or (c) a real construct outside the frozen
//     subset that needs the líder's sign-off before being implemented (docs/rmlx-subset.md's own
//     divergence-ledger taxonomy, uix-dom.md section 9) -- never something to silently work
//     around inside this test.
//
//     Enumerated at RUNTIME via std::filesystem (not a hand-typed file list) specifically so a
//     FUTURE fixture added to either directory is picked up automatically without anyone
//     remembering to update this test -- the same "the lexer has to fit what exists" discipline
//     this slice's own brief names, extended to "and stay fitting as the corpus grows".
// PT: RMLX-1/S1 -- oráculo de regressão: este lexer precisa tokenizar TODA fixture `.rml` real
//     deste repo (glintfx/tests/ + glintfx/demos/) até um EndOfFile limpo, nunca um Error. É a
//     aplicação literal da própria disciplina "o corpus decide" desta fatia e da cláusula de
//     fronteira-congelada do docs/rmlx-subset.md -- se uma fixture real faz este lexer emitir
//     Error, isso é ou (a) um bug deste lexer, ou (c) uma construção real fora do subconjunto
//     congelado que precisa do aval do líder antes de ser implementada (a própria taxonomia de
//     divergence-ledger do docs/rmlx-subset.md, uix-dom.md seção 9) -- nunca algo pra contornar em
//     silêncio dentro deste teste.
//
//     Enumerado em TEMPO DE EXECUÇÃO via std::filesystem (não uma lista de arquivo digitada à
//     mão) especificamente pra uma fixture FUTURA somada a qualquer um dos dois diretórios ser
//     pega automaticamente sem ninguém precisar lembrar de atualizar este teste -- a mesma
//     disciplina "o lexer tem que caber no que existe" que o próprio briefing desta fatia nomeia,
//     estendida pra "e continuar cabendo à medida que o corpus cresce".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/lexer.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLINTFX_UIX_TESTS_DIR
#error "GLINTFX_UIX_TESTS_DIR must be defined by CMake (glintfx/tests/uix/CMakeLists.txt)"
#endif
#ifndef GLINTFX_UIX_DEMOS_DIR
#error "GLINTFX_UIX_DEMOS_DIR must be defined by CMake (glintfx/tests/uix/CMakeLists.txt)"
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

// EN: Binary-mode read -- preserves bytes exactly (UTF-8 multi-byte sequences, no CRLF/LF
//     translation), matching this lexer's own "operates byte-wise, never decodes UTF-8" contract
//     (lexer.hpp header comment).
// PT: Leitura em modo binário -- preserva bytes exatamente (sequências multi-byte UTF-8, sem
//     tradução CRLF/LF), casando com o próprio contrato "opera byte-a-byte, nunca decodifica
//     UTF-8" deste lexer (comentário de cabeçalho do lexer.hpp).
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

using glintfx::uix::Lexer;
using glintfx::uix::Token;
using glintfx::uix::TokenKind;

void lex_one_fixture(const fs::path& path, std::size_t& total_tokens_out) {
  const std::string source = read_file_binary(path);
  if (source.empty()) {
    check(false, "corpus fixture is unexpectedly empty (read failed?): " + path.string());
    return;
  }

  Lexer lex(source);
  std::size_t tokens = 0;
  Token tok;
  // EN: kMaxTokenBytes+kMaxInputBytes ceilings bound worst-case scan cost per token/per document
  //     (see lexer.hpp header) -- this loop still caps its OWN iteration count as an independent,
  //     test-side belt-and-suspenders against an infinite loop if a future lexer bug ever makes
  //     `next()` fail to advance `pos_` on some input (a bug in THIS lexer, not a reason to trust
  //     it blindly from a test that is specifically here to catch lexer bugs).
  // PT: Os tetos kMaxTokenBytes+kMaxInputBytes limitam o custo de scan de pior-caso por
  //     token/por documento (ver cabeçalho do lexer.hpp) -- este laço ainda limita a PRÓPRIA
  //     contagem de iteração como belt-and-suspenders independente, do lado do teste, contra um
  //     laço infinito se um bug futuro do lexer algum dia fizer o `next()` falhar em avançar
  //     `pos_` nalgum input (um bug DESTE lexer, não um motivo pra confiar cegamente nele vindo
  //     de um teste que está aqui especificamente pra pegar bug de lexer).
  const std::size_t kIterationCeiling = 10'000'000;
  do {
    tok = lex.next();
    ++tokens;
    if (tokens > kIterationCeiling) {
      check(false,
            "corpus fixture exceeded the test's own iteration ceiling (lexer bug, not "
            "advancing pos_?): " +
                path.string());
      return;
    }
  } while (tok.kind != TokenKind::EndOfFile && tok.kind != TokenKind::Error);

  if (tok.kind == TokenKind::Error) {
    check(false, "corpus fixture failed to lex cleanly -- " + path.string() +
                     " -- Error at offset " + std::to_string(tok.offset) + ": " +
                     std::string(tok.text));
    return;
  }

  check(tok.kind == TokenKind::EndOfFile, "corpus fixture reached a clean EndOfFile: " + path.string());
  check(tokens > 1,
        "corpus fixture produced at least one real token before EndOfFile (not an "
        "empty/degenerate lex): " +
            path.string());
  total_tokens_out += tokens;
}

} // namespace

int main() {
  std::vector<fs::path> fixtures = find_rml_fixtures(GLINTFX_UIX_TESTS_DIR);
  const std::vector<fs::path> demo_fixtures = find_rml_fixtures(GLINTFX_UIX_DEMOS_DIR);
  fixtures.insert(fixtures.end(), demo_fixtures.begin(), demo_fixtures.end());

  // EN: This repo's own header-comment citations (glintfx/src/uix/dom/lexer.hpp) rely on the
  //     corpus being non-trivially sized (44 `.rml` fixtures, measured 2026-08-05) -- if this
  //     ever drops to zero, that is itself a signal something is wrong with the test's own
  //     directory wiring (a silently-vacuous "pass" over zero files proves nothing), not a
  //     legitimate green.
  // PT: As próprias citações de comentário de cabeçalho deste repo (glintfx/src/uix/dom/
  //     lexer.hpp) dependem do corpus ter tamanho não-trivial (44 fixtures `.rml`, medido
  //     2026-08-05) -- se isto algum dia cair pra zero, isso é em si um sinal de que algo está
  //     errado com a própria fiação de diretório deste teste (um "passa" vazio em silêncio sobre
  //     zero arquivos não prova nada), não um verde legítimo.
  check(fixtures.size() >= 40,
        "corpus discovery found a plausible number of .rml fixtures "
        "(>= 40, measured 44 on 2026-08-05) -- directory wiring sanity");

  std::size_t total_tokens = 0;
  for (const fs::path& fixture : fixtures) {
    lex_one_fixture(fixture, total_tokens);
  }

  std::printf("lexer_corpus_sanity: %zu fixtures, %zu total tokens\n", fixtures.size(), total_tokens);

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_corpus_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_corpus_sanity: PASS");
  return 0;
}
