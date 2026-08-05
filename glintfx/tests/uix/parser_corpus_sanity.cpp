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
//     PLACEMENT, NOT AN ACCIDENT: `glintfx/src/uix/dom/test_fixtures/` is a THIRD directory, not
//     one of the two `lexer_corpus_sanity.cpp` (S1's own test, not this file's to edit) already
//     scans. The GusWorld fixture's real `<style>` block contains a literal `<<` (pt-br prose
//     inside an RCSS comment, "~128dp << 228dp", meaning "much less than") -- this parser handles
//     it correctly (its own `<head>`-opacity raw-byte-scan, see parser.hpp's header comment, never
//     tokenizes INSIDE `<head>` at all, so the `<<` is never seen as markup), but the BARE
//     `Lexer`, run standalone over the WHOLE document (exactly what `lexer_corpus_sanity.cpp`
//     does), chokes on it: real upstream RmlUi tolerates this because `script`/`style` are
//     registered, at the TOKENIZER layer, as CDATA tags (`Factory.cpp:256-257`,
//     `XMLParser::RegisterPersistentCDATATag("style")`) whose content is scanned by
//     `BaseXMLParser::ReadCDATA` -- a dedicated raw scan that only treats `<` as markup when
//     immediately followed by `/` + the terminating tag's own name, treating every OTHER `<`
//     (including a stray `<<`) as literal content. This project's own `Lexer` (`lexer.cpp`) was
//     deliberately built with NO tag-name awareness at all (`lexer.hpp`'s own header: "This lexer
//     does NOT special-case the tag name `head`" -- and by the same reasoning, none of `script`/
//     `style` either), so it has no CDATA-tag concept and cannot replicate this upstream
//     tolerance on its own. Fixing that is a `lexer.cpp` change -- OUT OF THIS SLICE'S FILE
//     OWNERSHIP (`parser.{hpp,cpp}`, `parser_*.cpp` only) -- flagged to the orchestrator/líder as
//     a follow-up item, not silently patched around here. Placing the fixture in a directory
//     `lexer_corpus_sanity.cpp` never scans keeps THAT already-green, unrelated test green without
//     touching a single byte of S1's files, while this test (parse_document, which IS immune to
//     the gap by construction) still exercises the fixture in full. See
//     `parser_hardening_sanity.cpp`'s own dedicated case for the explicit, non-silent pin of this
//     exact finding (both the Lexer-level reject AND this parser's own immunity, side by side).
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
//     DELIBERADO E DOCUMENTADO, NÃO UM ACIDENTE: `glintfx/src/uix/dom/test_fixtures/` é um
//     TERCEIRO diretório, não um dos dois que o lexer_corpus_sanity.cpp (teste da própria S1, não
//     deste arquivo pra editar) já varre. O bloco `<style>` real da fixture do GusWorld contém um
//     `<<` literal (prosa pt-br dentro de um comentário RCSS, "~128dp << 228dp", significando
//     "muito menor que") -- este parser trata corretamente (o próprio scan cru de byte de
//     opacidade de `<head>`, ver o comentário de cabeçalho do parser.hpp, nunca tokeniza DENTRO
//     de `<head>` nenhuma vez, então o `<<` nunca é visto como markup), mas o `Lexer` CRU,
//     rodando standalone sobre o documento INTEIRO (exatamente o que o lexer_corpus_sanity.cpp
//     faz), engasga: o RmlUi upstream real tolera isto porque `script`/`style` são registradas,
//     na camada de TOKENIZADOR, como tags CDATA (`Factory.cpp:256-257`,
//     `XMLParser::RegisterPersistentCDATATag("style")`), cujo conteúdo é escaneado pelo
//     `BaseXMLParser::ReadCDATA` -- um scan cru dedicado que só trata `<` como markup quando
//     imediatamente seguido de `/` + o próprio nome da tag terminadora, tratando todo OUTRO `<`
//     (inclusive um `<<` perdido) como conteúdo literal. O próprio `Lexer` deste projeto
//     (`lexer.cpp`) foi construído deliberadamente SEM consciência nenhuma de nome-de-tag (o
//     próprio cabeçalho do lexer.hpp: "Este lexer NÃO trata especialmente o nome de tag `head`" --
//     e pelo mesmo raciocínio, nem `script`/`style` tampouco), então não tem conceito de tag-CDATA
//     nenhum e não consegue replicar essa tolerância upstream sozinho. Consertar isso é mudança de
//     `lexer.cpp` -- FORA DA POSSE DE ARQUIVO DESTA FATIA (`parser.{hpp,cpp}`, `parser_*.cpp` só)
//     -- sinalizado ao orquestrador/líder como item de acompanhamento, não remendado em silêncio
//     aqui. Colocar a fixture num diretório que o lexer_corpus_sanity.cpp nunca varre mantém
//     AQUELE teste já-verde e não-relacionado verde sem tocar um byte sequer dos arquivos da S1,
//     enquanto este teste (parse_document, que É imune à lacuna por construção) segue exercitando
//     a fixture por completo. Ver o próprio caso dedicado do `parser_hardening_sanity.cpp` pro
//     pin explícito, não-silencioso, deste achado exato (tanto a rejeição em nível de Lexer QUANTO
//     a imunidade deste parser, lado a lado).
//
//     🔴 RMLX1-CORPUS (TODO.md, 2026-08-05) -- 15 MORE real GusWorld screens joined
//     `gusworld_battle_cockpit.rml` in that same third directory (16 files total now), all
//     runtime-captured, never hand-retyped (see each file's own provenance header). 5 of the 15
//     reproduce the identical class of gap this comment already describes (a literal `<` inside
//     `<style>` comment prose the standalone Lexer cannot tokenize) at a DIFFERENT byte offset
//     (a stray "delta < 0.01px" in prose, not the "<<" this comment names above) -- same
//     mechanism, same reason they live here and not in one of lexer_corpus_sanity.cpp's two
//     directories, this parser immune to all of them for the identical reason.
// PT: 🔴 RMLX1-CORPUS (TODO.md, 2026-08-05) -- mais 15 telas reais do GusWorld se juntaram à
//     `gusworld_battle_cockpit.rml` nesse mesmo terceiro diretório (16 arquivos agora), todas
//     capturadas em runtime, nunca retipadas à mão (ver o próprio cabeçalho de proveniência de
//     cada arquivo). 5 das 15 reproduzem a MESMA classe de lacuna que este comentário já descreve
//     (um `<` literal dentro de prosa de comentário `<style>` que o Lexer standalone não
//     consegue tokenizar) num offset de byte DIFERENTE (um "delta < 0.01px" perdido em prosa, não
//     o "<<" que este comentário nomeia acima) -- mesmo mecanismo, mesmo motivo de morarem aqui e
//     não num dos dois diretórios do lexer_corpus_sanity.cpp, este parser imune a todas elas pelo
//     mesmo motivo.
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
