// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- regression oracle: this lexer must tokenize EVERY real `.rml` fixture in this
//     repo (glintfx/tests/ + glintfx/demos/ + this arc's own glintfx/src/uix/dom/test_fixtures/,
//     the same THIRD directory parser_corpus_sanity.cpp/dumper_corpus_sanity.cpp already scan --
//     see below for why this target only ADDED that third directory on 2026-08-06) to a clean
//     EndOfFile, never an Error. This is the literal enforcement of this slice's own "the corpus
//     decides" discipline and of docs/rmlx-subset.md's frozen-boundary clause -- if a real fixture
//     makes this lexer emit Error, that is either (a) a bug in this lexer, or (c) a real construct
//     outside the frozen subset that needs the líder's sign-off before being implemented
//     (docs/rmlx-subset.md's own divergence-ledger taxonomy, uix-dom.md section 9) -- never
//     something to silently work around inside this test.
//
//     🔴 `UIX-LEXER-OPACO` (2026-08-06) -- WHY THE THIRD DIRECTORY WAS ADDED HERE, NOT FROM DAY
//     ONE: `glintfx/src/uix/dom/test_fixtures/`'s 16 real GusWorld screens used to make THIS
//     lexer, run standalone exactly as this file does, emit `Error` -- their `<style>` blocks
//     contain literal, un-escaped runs of two or more `<` in RCSS comment prose (the original
//     `gusworld_battle_cockpit.rml`'s "~128dp << 228dp", pt-br for "much less than", plus 5 more
//     fixtures with the same class of gap at a different byte offset, "delta < 0.01px" -- see
//     parser_corpus_sanity.cpp's own header comment for the full history). Keeping that directory
//     OUT of this target's scan (while `parser_corpus_sanity.cpp`, immune by construction via its
//     own `<head>`-opacity raw-byte-scan, exercised it in full) was the deliberate way to keep
//     THIS suite green for a gap this slice's own S1 file ownership had not yet fixed --
//     `parser_hardening_sanity.cpp`'s dedicated case was the explicit, non-silent pin of that
//     finding. `UIX-LEXER-OPACO` closed the gap AT THIS LEXER (a `<style>`/`<script>`
//     CDATA-tag concept, matching real upstream RmlUi -- see lexer.hpp's own header comment,
//     "RESOLVED (UIX-LEXER-OPACO)" paragraph, for the full argument and declared scope teto), so
//     the reason to exclude the directory is gone: this target now sweeps all three directories,
//     60 fixtures total (measured 2026-08-06: 44 across the original two + 16 in the third).
//
//     Enumerated at RUNTIME via std::filesystem (not a hand-typed file list) specifically so a
//     FUTURE fixture added to any of the three directories is picked up automatically without
//     anyone remembering to update this test -- the same "the lexer has to fit what exists"
//     discipline this slice's own brief names, extended to "and stay fitting as the corpus grows".
// PT: RMLX-1/S1 -- oráculo de regressão: este lexer precisa tokenizar TODA fixture `.rml` real
//     deste repo (glintfx/tests/ + glintfx/demos/ + o test_fixtures/ próprio deste arco em
//     glintfx/src/uix/dom/, o MESMO terceiro diretório que parser_corpus_sanity.cpp/
//     dumper_corpus_sanity.cpp já varrem -- ver abaixo pro porquê deste alvo só ter SOMADO esse
//     terceiro diretório em 2026-08-06) até um EndOfFile limpo, nunca um Error. É a aplicação
//     literal da própria disciplina "o corpus decide" desta fatia e da cláusula de
//     fronteira-congelada do docs/rmlx-subset.md -- se uma fixture real faz este lexer emitir
//     Error, isso é ou (a) um bug deste lexer, ou (c) uma construção real fora do subconjunto
//     congelado que precisa do aval do líder antes de ser implementada (a própria taxonomia de
//     divergence-ledger do docs/rmlx-subset.md, uix-dom.md seção 9) -- nunca algo pra contornar em
//     silêncio dentro deste teste.
//
//     🔴 `UIX-LEXER-OPACO` (2026-08-06) -- POR QUE O TERCEIRO DIRETÓRIO FOI SOMADO AQUI, NÃO DESDE
//     O PRIMEIRO DIA: as 16 telas reais do GusWorld do `glintfx/src/uix/dom/test_fixtures/`
//     costumavam fazer ESTE lexer, rodando standalone exatamente como este arquivo faz, emitir
//     `Error` -- os blocos `<style>` delas contêm trechos literais, não-escapados, de dois ou mais
//     `<` em prosa de comentário RCSS (o "~128dp << 228dp" original do
//     `gusworld_battle_cockpit.rml`, pt-br pra "muito menor que", mais 5 outras fixtures com a
//     mesma classe de lacuna num offset de byte diferente, "delta < 0.01px" -- ver o próprio
//     comentário de cabeçalho do parser_corpus_sanity.cpp pro histórico completo). Manter aquele
//     diretório FORA do escopo deste alvo (enquanto o `parser_corpus_sanity.cpp`, imune por
//     construção via o próprio scan cru de byte de opacidade de `<head>`, o exercitava por
//     completo) era a forma deliberada de manter ESTA suíte verde por uma lacuna que a posse de
//     arquivo da própria S1 desta fatia ainda não tinha consertado -- o caso dedicado do
//     `parser_hardening_sanity.cpp` era o pin explícito, não-silencioso, daquele achado.
//     `UIX-LEXER-OPACO` fechou a lacuna NESTE LEXER (um conceito de tag-CDATA
//     `<style>`/`<script>`, batendo com o RmlUi upstream real -- ver o próprio comentário de
//     cabeçalho do lexer.hpp, parágrafo "RESOLVED (UIX-LEXER-OPACO)", pro argumento completo e o
//     teto de escopo declarado), então o motivo de excluir o diretório sumiu: este alvo agora
//     varre os três diretórios, 60 fixtures no total (medido 2026-08-06: 44 nos dois originais +
//     16 no terceiro).
//
//     Enumerado em TEMPO DE EXECUÇÃO via std::filesystem (não uma lista de arquivo digitada à
//     mão) especificamente pra uma fixture FUTURA somada a qualquer um dos três diretórios ser
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
  // EN: `UIX-LEXER-OPACO` (2026-08-06) -- see this file's own header comment for why this third
  //     sweep was ADDED here rather than present from day one.
  // PT: `UIX-LEXER-OPACO` (2026-08-06) -- ver o próprio comentário de cabeçalho deste arquivo pro
  //     motivo desta terceira varredura ter sido SOMADA aqui em vez de presente desde o dia um.
  const std::vector<fs::path> s3_fixtures = find_rml_fixtures(GLINTFX_UIX_S3_FIXTURES_DIR);
  fixtures.insert(fixtures.end(), s3_fixtures.begin(), s3_fixtures.end());

  // EN: This repo's own header-comment citations (glintfx/src/uix/dom/lexer.hpp) rely on the
  //     corpus being non-trivially sized (60 `.rml` fixtures -- 44 across the original two
  //     directories + 16 in the third, measured 2026-08-06) -- if this ever drops to zero, that
  //     is itself a signal something is wrong with the test's own directory wiring (a
  //     silently-vacuous "pass" over zero files proves nothing), not a legitimate green. The
  //     floor itself stays a conservative >= 40 (not bumped to 60) on purpose -- same reasoning
  //     parser_corpus_sanity.cpp's own identical floor gives: this is a directory-wiring smoke
  //     check, not a re-assertion of the exact corpus size.
  // PT: As próprias citações de comentário de cabeçalho deste repo (glintfx/src/uix/dom/
  //     lexer.hpp) dependem do corpus ter tamanho não-trivial (60 fixtures `.rml` -- 44 nos dois
  //     diretórios originais + 16 no terceiro, medido 2026-08-06) -- se isto algum dia cair pra
  //     zero, isso é em si um sinal de que algo está errado com a própria fiação de diretório
  //     deste teste (um "passa" vazio em silêncio sobre zero arquivos não prova nada), não um
  //     verde legítimo. O piso em si segue conservador em >= 40 (não elevado a 60) de propósito --
  //     mesmo raciocínio que o próprio piso idêntico do parser_corpus_sanity.cpp dá: é uma
  //     checagem de fumaça de fiação de diretório, não uma reafirmação do tamanho exato do corpus.
  check(fixtures.size() >= 40,
        "corpus discovery found a plausible number of .rml fixtures "
        "(>= 40, measured 60 across all three directories on 2026-08-06) -- directory wiring "
        "sanity");

  // EN: Exact count of the third directory's own fixtures -- mirrors
  //     parser_corpus_sanity.cpp's identical assertion, so a future accidental deletion (or a
  //     future fixture silently NOT landing there) fails loud instead of hiding inside the >= 40
  //     floor above.
  // PT: Contagem exata das fixtures do terceiro diretório em si -- espelha a afirmação idêntica do
  //     parser_corpus_sanity.cpp, pra uma futura deleção acidental (ou uma futura fixture que
  //     silenciosamente NÃO caia lá) falhar alto em vez de se esconder dentro do piso >= 40 acima.
  check(s3_fixtures.size() == 16,
        "this arc's own third fixture directory (glintfx/src/uix/dom/test_fixtures/) holds "
        "exactly 16 .rml files, all now lexing cleanly since UIX-LEXER-OPACO (2026-08-06)");

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
