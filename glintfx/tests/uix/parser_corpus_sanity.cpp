// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S3 -- regression oracle: this parser must build a Document from EVERY real `.rml`
//     fixture in this repo (glintfx/tests/ + glintfx/demos/, the same two directories
//     lexer_corpus_sanity.cpp already enumerates) PLUS this slice's own GusWorld "representative
//     screen" fixture (glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml) to a clean
//     `ParseResult` with no `error`, never a `ParseError`. Same "the corpus decides, this test
//     proves it, never contorts around it" discipline as lexer_corpus_sanity.cpp -- if a real
//     fixture makes this parser emit a ParseError, that is either (a) a bug in this parser, or
//     (c) a real construct outside the frozen subset needing the líder's sign-off
//     (docs/rmlx-subset.md's own divergence-ledger taxonomy) -- never something to silently work
//     around inside this test.
//
//     🔴 WHY THE GUSWORLD FIXTURE LIVES OUTSIDE glintfx/tests/ -- A DELIBERATE, DOCUMENTED
//     PLACEMENT, HISTORICAL BUT NO LONGER A LEXER-LEVEL WORKAROUND: `glintfx/src/uix/dom/
//     test_fixtures/` is a THIRD directory, originally NOT one of the two
//     `lexer_corpus_sanity.cpp` (S1's own test, not this file's to edit) scanned. The GusWorld
//     fixture's real `<style>` block contains a literal `<<` (pt-br prose inside an RCSS comment,
//     "~128dp << 228dp", meaning "much less than") -- this parser has ALWAYS handled it correctly
//     (its own `<head>`-opacity raw-byte-scan, see parser.hpp's header comment, never tokenizes
//     INSIDE `<head>` at all, so the `<<` is never seen as markup). The BARE `Lexer`, run
//     standalone over the WHOLE document (exactly what `lexer_corpus_sanity.cpp` does), USED TO
//     choke on it -- `UIX-LEXER-OPACO` (2026-08-06) fixed that at the TOKENIZER layer, giving
//     `lexer.cpp` the same "script"/"style" persistent-CDATA-tag concept real upstream RmlUi has
//     (`Factory.cpp:255-257`, `BaseXMLParser::ReadCDATA`); see lexer.hpp's own header comment,
//     "RESOLVED (UIX-LEXER-OPACO)" paragraph, for the full argument and declared scope teto. As of
//     that same fix, `lexer_corpus_sanity.cpp` SWEEPS this directory too (see that file's own
//     header comment) -- the original reason to keep this fixture out of its scan is gone. The
//     directory split itself stays (this file's own file-discovery/count assertions below still
//     depend on it, and `dumper_corpus_sanity.cpp`/`dumper_determinism_sanity.cpp` reuse the same
//     `GLINTFX_UIX_S3_FIXTURES_DIR` define), it is simply no longer hiding a lexer-level gap. See
//     `parser_hardening_sanity.cpp`'s own dedicated case
//     (`test_style_stray_lt_tokenizes_and_parses_cleanly`, formerly
//     `test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt`) for the always-run proof
//     that both halves -- the (now-fixed) Lexer-level tokenization AND this parser's own
//     always-true immunity -- stay green side by side.
// PT: RMLX-1/S3 -- oráculo de regressão: este parser precisa construir um Document de TODA
//     fixture `.rml` real deste repo (glintfx/tests/ + glintfx/demos/, os MESMOS dois diretórios
//     que o lexer_corpus_sanity.cpp já enumera) MAIS a própria fixture "tela representativa" do
//     GusWorld desta fatia (glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml) até um
//     `ParseResult` limpo sem `error`, nunca um `ParseError`. Mesma disciplina "o corpus decide,
//     este teste prova, nunca contorna" do lexer_corpus_sanity.cpp -- se uma fixture real faz
//     este parser emitir um ParseError, isso é ou (a) um bug deste parser, ou (c) uma construção
//     real fora do subconjunto congelado que precisa do aval do líder (a própria taxonomia de
//     divergence-ledger do docs/rmlx-subset.md) -- nunca algo pra contornar em silêncio dentro
//     deste teste.
//
//     🔴 POR QUE A FIXTURE DO GUSWORLD MORA FORA DE glintfx/tests/ -- UM POSICIONAMENTO
//     DELIBERADO E DOCUMENTADO, HISTÓRICO MAS NÃO MAIS UM CONTORNO EM NÍVEL DE LEXER:
//     `glintfx/src/uix/dom/test_fixtures/` é um TERCEIRO diretório, originalmente NÃO um dos dois
//     que o lexer_corpus_sanity.cpp (teste da própria S1, não deste arquivo pra editar) varria. O
//     bloco `<style>` real da fixture do GusWorld contém um `<<` literal (prosa pt-br dentro de um
//     comentário RCSS, "~128dp << 228dp", significando "muito menor que") -- este parser SEMPRE
//     tratou corretamente (o próprio scan cru de byte de opacidade de `<head>`, ver o comentário
//     de cabeçalho do parser.hpp, nunca tokeniza DENTRO de `<head>` nenhuma vez, então o `<<`
//     nunca é visto como markup). O `Lexer` CRU, rodando standalone sobre o documento INTEIRO
//     (exatamente o que o lexer_corpus_sanity.cpp faz), COSTUMAVA engasgar nisso --
//     `UIX-LEXER-OPACO` (2026-08-06) consertou isso na camada de TOKENIZADOR, dando ao
//     `lexer.cpp` o mesmo conceito de tag-CDATA-persistente "script"/"style" que o RmlUi upstream
//     real tem (`Factory.cpp:255-257`, `BaseXMLParser::ReadCDATA`); ver o próprio comentário de
//     cabeçalho do lexer.hpp, parágrafo "RESOLVED (UIX-LEXER-OPACO)", pro argumento completo e o
//     teto de escopo declarado. A partir desse mesmo conserto, o lexer_corpus_sanity.cpp VARRE
//     este diretório também (ver o próprio comentário de cabeçalho daquele arquivo) -- o motivo
//     original de manter esta fixture fora do escopo dele sumiu. A divisão de diretório em si
//     fica (as próprias afirmações de descoberta/contagem de arquivo deste arquivo abaixo ainda
//     dependem dela, e o dumper_corpus_sanity.cpp/dumper_determinism_sanity.cpp reaproveitam o
//     mesmo define `GLINTFX_UIX_S3_FIXTURES_DIR`), ela simplesmente não esconde mais uma lacuna em
//     nível de lexer. Ver o próprio caso dedicado do `parser_hardening_sanity.cpp`
//     (`test_style_stray_lt_tokenizes_and_parses_cleanly`, antes
//     `test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt`) pra prova sempre-rodada de
//     que as duas metades -- a tokenização em nível de Lexer (agora consertada) E a imunidade
//     sempre-verdadeira deste parser -- ficam verdes lado a lado.
//
//     🔴 RMLX1-CORPUS (TODO.md, 2026-08-05) -- 15 MORE real GusWorld screens joined
//     `gusworld_battle_cockpit.rml` in that same third directory (16 files total now), all
//     runtime-captured, never hand-retyped (see each file's own provenance header). 5 of the 15
//     reproduce the identical class of gap this comment already describes (a literal `<` inside
//     `<style>` comment prose the standalone Lexer cannot tokenize) at a DIFFERENT byte offset
//     (a stray "delta < 0.01px" in prose, not the "<<" this comment names above) -- same
//     mechanism, same reason they live here and not in one of lexer_corpus_sanity.cpp's two
//     directories, this parser immune to all of them for the identical reason. `UIX-LEXER-OPACO`
//     (2026-08-06) closed this class of gap for all 5 (and the original `<<`) at once, since the
//     fix is tag-name-scoped, not per-occurrence -- see this file's own header comment above.
// PT: 🔴 RMLX1-CORPUS (TODO.md, 2026-08-05) -- mais 15 telas reais do GusWorld se juntaram à
//     `gusworld_battle_cockpit.rml` nesse mesmo terceiro diretório (16 arquivos agora), todas
//     capturadas em runtime, nunca retipadas à mão (ver o próprio cabeçalho de proveniência de
//     cada arquivo). 5 das 15 reproduzem a MESMA classe de lacuna que este comentário já descreve
//     (um `<` literal dentro de prosa de comentário `<style>` que o Lexer standalone não
//     consegue tokenizar) num offset de byte DIFERENTE (um "delta < 0.01px" perdido em prosa, não
//     o "<<" que este comentário nomeia acima) -- mesmo mecanismo, mesmo motivo de morarem aqui e
//     não num dos dois diretórios do lexer_corpus_sanity.cpp, este parser imune a todas elas pelo
//     mesmo motivo. `UIX-LEXER-OPACO` (2026-08-06) fechou esta classe de lacuna pras 5 (e o `<<`
//     original) de uma vez só, já que o conserto é escopado por nome-de-tag, não por-ocorrência --
//     ver o próprio comentário de cabeçalho deste arquivo acima.
//
//     Enumerated at RUNTIME via std::filesystem, same reason lexer_corpus_sanity.cpp gives: a
//     FUTURE fixture is picked up automatically, nobody has to remember to update this list.
// PT: Enumerado em TEMPO DE EXECUÇÃO via std::filesystem, mesmo motivo do lexer_corpus_sanity.cpp:
//     uma fixture FUTURA é pega automaticamente, ninguém precisa lembrar de atualizar esta lista.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"

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

using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;

void parse_one_fixture(const fs::path& path) {
  const std::string source = read_file_binary(path);
  if (source.empty()) {
    check(false, "corpus fixture is unexpectedly empty (read failed?): " + path.string());
    return;
  }

  ParseResult result = parse_document(source);
  if (result.error.has_value()) {
    check(false, "corpus fixture failed to parse cleanly -- " + path.string() +
                     " -- ParseError "
                     "at " +
                     std::to_string(result.error->line) + ":" +
                     std::to_string(result.error->column) + ": " + result.error->message);
    return;
  }
  check(result.document != nullptr, "corpus fixture parsed to a non-null Document: " +
                                        path.string());
}

} // namespace

int main() {
  std::vector<fs::path> fixtures = find_rml_fixtures(GLINTFX_UIX_TESTS_DIR);
  const std::vector<fs::path> demo_fixtures = find_rml_fixtures(GLINTFX_UIX_DEMOS_DIR);
  fixtures.insert(fixtures.end(), demo_fixtures.begin(), demo_fixtures.end());
  const std::vector<fs::path> s3_fixtures = find_rml_fixtures(GLINTFX_UIX_S3_FIXTURES_DIR);
  fixtures.insert(fixtures.end(), s3_fixtures.begin(), s3_fixtures.end());

  // EN: >= 40 real fixtures (44 measured 2026-08-05 across the two shared directories, +16 in
  //     this arc's own third directory = 60, measured 2026-08-05: the 1 original GusWorld
  //     battle_cockpit fixture + 15 more real GusWorld runtime-captured screens added by
  //     RMLX1-CORPUS, TODO.md) -- same "a silently-vacuous pass over zero files proves nothing"
  //     sanity lexer_corpus_sanity.cpp already applies. The floor itself stays a conservative
  //     >= 40 (not bumped to 60) on purpose: it is a directory-wiring smoke check, not a
  //     re-assertion of the exact corpus size the two counts below already pin precisely.
  // PT: >= 40 fixtures reais (44 medidas 2026-08-05 nos dois diretórios compartilhados, +16 no
  //     terceiro diretório próprio deste arco = 60, medidas 2026-08-05: a 1 fixture original do
  //     GusWorld battle_cockpit + 15 outras telas reais do GusWorld capturadas em runtime,
  //     somadas pela RMLX1-CORPUS, TODO.md) -- mesma sanidade "um 'passa' vazio em silêncio sobre
  //     zero arquivos não prova nada" que o lexer_corpus_sanity.cpp já aplica. O piso em si segue
  //     conservador em >= 40 (não elevado a 60) de propósito: é uma checagem de fumaça de fiação
  //     de diretório, não uma reafirmação do tamanho exato do corpus que as duas contagens abaixo
  //     já fixam com precisão.
  check(fixtures.size() >= 40,
        "corpus discovery found a plausible number of .rml fixtures (>= 40) -- directory "
        "wiring sanity");

  // EN: RMLX1-CORPUS (TODO.md) -- exact count of the third directory's own fixtures, so a future
  //     accidental deletion (or a future fixture silently NOT landing there) fails loud instead
  //     of hiding inside the >= 40 floor above.
  // PT: RMLX1-CORPUS (TODO.md) -- contagem exata das fixtures do terceiro diretório em si, pra
  //     uma futura deleção acidental (ou uma futura fixture que silenciosamente NÃO caia lá)
  //     falhar alto em vez de se esconder dentro do piso >= 40 acima.
  check(s3_fixtures.size() == 16,
        "this arc's own third fixture directory (glintfx/src/uix/dom/test_fixtures/) holds "
        "exactly 16 .rml files -- the 1 original gusworld_battle_cockpit.rml plus the 15 real "
        "GusWorld runtime-captured screens RMLX1-CORPUS added, none missing, none extra");

  bool found_gusworld_fixture = false;
  for (const fs::path& fixture : fixtures) {
    if (fixture.filename() == "gusworld_battle_cockpit.rml") {
      found_gusworld_fixture = true;
    }
    parse_one_fixture(fixture);
  }
  check(found_gusworld_fixture,
        "the GusWorld representative-screen fixture (gusworld_battle_cockpit.rml) was actually "
        "discovered and exercised, not silently absent from the corpus scan");

  std::printf("parser_corpus_sanity: %zu fixtures parsed\n", fixtures.size());

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_corpus_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_corpus_sanity: PASS");
  return 0;
}
