// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-RCSS-ORACULO` -- THE DIFFERENTIAL ORACLE for `RMLX-2`. For every fixture this harness
//     accepts (see "CORPUS SCOPE" below), it runs BOTH independently-written computed-value
//     dumpers over the SAME source bytes and compares their output BYTE FOR BYTE:
//       - side A (`glintfx/src/rml/rcss_dump.cpp`): walks the REAL `Rml::ElementDocument`'s
//         `Style::ComputedValues`/`Element::GetProperty`, produced by RmlUi's own cascade;
//       - side B (`glintfx/src/uix/style/dumper.cpp`): walks glintfx's OWN
//         `lexer -> parser -> selector_match -> cascade -> value_compute` pipeline.
//     Both emit the format `docs/uix-rcss.md` ("the spec" hereafter) defines -- that document is
//     the SOLE contract; this file is written by a THIRD, independent author who read neither
//     dumper's own source before writing the comparison logic below (mirrors the exact
//     "author independence" discipline `docs/uix-rcss.md`'s own header names for the two dumpers
//     themselves, extended one more step: the referee does not umpire by re-reading the players'
//     playbooks).
//
//     🔴 THE RULE THIS FILE EXISTS TO ENFORCE: a divergence this harness finds is a RESULT, not a
//     failure to patch away. Editing either dumper (or this harness's own comparison) to flatten a
//     difference and make the diff go green is FRAUDING THE ORACLE. Every NEW divergence is
//     reported here with the exact byte pair and MUST fail the run -- classifying it (our bug /
//     expected upstream normalization / out-of-subset / a NEW deliberate exception needing the
//     líder's own sign-off) is this task's OWN delivery report's job, never a silent code change
//     in this file.
//
//     🔵 THE ONE DOCUMENTED EXCEPTION -- `docs/uix-rcss.md` section 14.1 (`UIX-RCSS-ERRATA-5`,
//     2026-08-06) -- is the SOLE case this harness is allowed to swallow, and it is swallowed
//     through exactly one generic, table-driven mechanism (`kKnownDivergences` / `matches()`
//     below), never an `if` naming `border-top` or any other property inline in the diff loop
//     (section 14.1's own normative requirement 1, "the harness owns zero exception logic of its
//     own"). `kDeclaredDivergenceCount` mirrors section 14.1's own "Deliberate divergences: N"
//     count line and is asserted equal to `kKnownDivergences.size()` at startup -- a silent drift
//     between the document's own declared count and this table is itself a failure, not a warning
//     (this task's own brief, "conferir a contagem declarada... divergir é falha, não aviso").
//
//     CORPUS SCOPE, AND WHY IT IS NARROWER THAN "every .rml this repo has". Side B's own
//     `parse_stylesheet()` takes RCSS TEXT directly -- it has no file-interface, no `<link
//     type="text/rcss">` resolution (that is RmlUi's own loader machinery, `glintfx::Bootstrap`,
//     confined to `glintfx/src/rml/` and never exercised by the standalone `glintfx::uix::style`
//     module). A fixture whose RCSS lives in an EXTERNAL `.rcss` file reachable only via `<link>`
//     would therefore feed side A the real, external, merged stylesheet and side B an artificially
//     EMPTY one -- a guaranteed, spurious "divergence" that measures this harness's own inability
//     to fetch a second file, not a real dumper disagreement. `cascade_corpus_sanity.cpp` /
//     `selector_match_corpus_sanity.cpp` (this repo's own two closest precedents for combining
//     real corpus DOM+RCSS) already drew this exact line (`find_rml_with_style`, `<style>` tags
//     only) -- this harness inherits it, narrowed one step further to ALSO reject any fixture that
//     ALSO carries a `<link>` (a `<style>` fixture that additionally links external RCSS would
//     still feed side A more than side B): confirmed by measurement, not assumed --
//     `glintfx/tests/log_throttle_scene.rml` is the one fixture in this repo's corpus with BOTH,
//     and is excluded for exactly this reason. Multiple `<style>` blocks in ONE fixture ARE
//     supported (real, measured cases exist under `glintfx/src/uix/dom/test_fixtures/`): their raw
//     text is concatenated, source order preserved, into ONE `parse_stylesheet()` call -- matching
//     what RmlUi itself does when it merges sequential `<style>` tags into one cascade (source
//     order is load-bearing for same-specificity tie-breaks, so blocks are never parsed and
//     cascaded SEPARATELY the way `cascade_corpus_sanity.cpp`'s own per-block loop does for ITS
//     narrower, non-differential purpose).
//
//     FOUR corpus directories, the same absolute SOURCE-tree-path discipline `dom_dump_
//     differential_oracle.cpp`'s own header already established (so BOTH sides read the exact same
//     bytes off disk): `glintfx/tests/`, `glintfx/demos/showcase/` (the same two
//     `rcss_dump_corpus_sanity.cpp` already scans), `glintfx/src/uix/dom/test_fixtures/` (the
//     RMLX-1 corpus, 16 real GusWorld screens, already reused by the `uix_style_*_corpus_sanity`
//     suite for the exact same reason), and `glintfx/src/rml/rcss_dump_test_fixtures/` -- a FIFTH,
//     NEW, dedicated directory this task's own delivery adds, scanned by NO other test in this
//     repo, holding exactly one fixture: `uix_rcss_errata5_border_top_reversed.rml`, the verbatim
//     `#a`/`#b` RCSS from the spec's own section 15.2, as a REAL FILE ON DISK rather than an inline
//     C++ string literal -- section 14.1's own normative requirement 3 ("every exception MUST be
//     exercised by at least one fixture, and the oracle MUST fail if any exception is never
//     exercised") needs a fixture THIS oracle itself feeds through BOTH dumpers over the SAME
//     bytes; the two existing per-side inline-string tests (`rcss_dump_worked_examples.cpp`'s own
//     `test_15_2_shorthand_order_border_top`, `dumper_sanity.cpp`'s own `test_worked_example_15_2_
//     shorthand_order`) each already prove their OWN side's byte in isolation (section 14.1's own
//     ledger row says so explicitly, "Still missing: the differential harness itself") but never
//     ran the SAME file through a diff-and-consult-section-14.1 comparison before this task.
//
//     A DOM-parse or RCSS-parse failure on a fixture this scope accepts is NOT this harness's own
//     concern (same "not this item's own concern" precedent `cascade_corpus_sanity.cpp` already
//     established for the DOM/RCSS parsers' own regressions) -- SKIPPED, counted, and named in the
//     scope line, never silently dropped and never counted as a divergence. A side-A `engine.
//     load()` failure on a fixture side B successfully parsed IS this harness's own concern (the
//     "same bytes" precondition the whole oracle rests on has been violated) and DOES fail the run.
//
//     WHY THIS FILE LIVES IN glintfx/src/rml/ AND NOT glintfx/tests/: it references
//     `Rml::Context`/`Rml::ElementDocument` directly (half the oracle IS the real RmlUi tree), and
//     `tools/check_rml_whitelist.sh` checks (b)/(c) forbid exactly that in a NEW file physically
//     located under `glintfx/tests/` (frozen by `RMLX-0`'s own brief). Same reasoning, same
//     directory, as `dom_dump_differential_oracle.cpp` (its own `RMLX-1`/S7 precedent) and
//     `rcss_dump_worked_examples.cpp`/`rcss_dump_corpus_sanity.cpp` (this module's own side-A test
//     siblings). Registered with CTest from `glintfx/tests/CMakeLists.txt` (a `CMakeLists.txt` is
//     never scanned by the gate, whatever source path it names).
// PT: `UIX-RCSS-ORACULO` -- O ORÁCULO DIFERENCIAL da `RMLX-2`. Para toda fixture que este harness
//     aceita (ver "ESCOPO DE CORPUS" abaixo), ele roda OS DOIS dumpers de valor-computado escritos
//     de forma independente sobre os MESMOS bytes-fonte e compara a saída BYTE A BYTE:
//       - lado A (`glintfx/src/rml/rcss_dump.cpp`): percorre o `Style::ComputedValues`/
//         `Element::GetProperty` do `Rml::ElementDocument` REAL, produzido pela própria cascata do
//         RmlUi;
//       - lado B (`glintfx/src/uix/style/dumper.cpp`): percorre o próprio pipeline
//         `lexer -> parser -> selector_match -> cascade -> value_compute` da glintfx.
//     Os dois emitem o formato que o docs/uix-rcss.md ("a spec" daqui em diante) define -- aquele
//     documento é o ÚNICO contrato; este arquivo é escrito por um TERCEIRO autor, independente, que
//     não leu o fonte de nenhum dos dois dumpers antes de escrever a lógica de comparação abaixo
//     (espelha a mesma disciplina de "independência de autor" que o próprio cabeçalho do
//     docs/uix-rcss.md nomeia pros dois dumpers, estendida mais um passo: o árbitro não apita
//     relendo o livro de jogadas dos jogadores).
//
//     🔴 A REGRA QUE ESTE ARQUIVO EXISTE PRA APLICAR: uma divergência que este harness acha é
//     RESULTADO, não uma falha pra remendar. Editar qualquer um dos dumpers (ou a própria
//     comparação deste harness) pra achatar uma diferença e fazer o diff dar verde É FRAUDAR O
//     ORÁCULO. Toda divergência NOVA é reportada aqui com o par de byte exato e TEM que reprovar a
//     rodada -- classificá-la (nosso bug / normalização esperada de upstream / fora-de-escopo / uma
//     NOVA exceção deliberada exigindo o próprio aval do líder) é trabalho do PRÓPRIO relatório de
//     entrega desta tarefa, nunca uma mudança de código silenciosa neste arquivo.
//
//     🔵 A ÚNICA EXCEÇÃO DOCUMENTADA -- seção 14.1 do docs/uix-rcss.md (`UIX-RCSS-ERRATA-5`,
//     2026-08-06) -- é o ÚNICO caso que este harness tem permissão de engolir, e é engolido por
//     exatamente um mecanismo genérico, guiado-por-tabela (`kKnownDivergences`/`matches()` abaixo),
//     nunca um `if` nomeando `border-top` ou qualquer outra propriedade inline no laço de diff (o
//     próprio requisito normativo 1 da seção 14.1, "o harness não possui lógica de exceção
//     própria nenhuma"). `kDeclaredDivergenceCount` espelha a própria linha de contagem "Deliberate
//     divergences: N" da seção 14.1 e é asserido igual a `kKnownDivergences.size()` na partida --
//     uma deriva silenciosa entre a contagem declarada do documento e esta tabela é, ela mesma, uma
//     falha, não um aviso (o próprio briefing desta tarefa, "conferir a contagem declarada...
//     divergir é falha, não aviso").
//
//     ESCOPO DE CORPUS, E POR QUE É MAIS ESTREITO QUE "todo .rml deste repo". O próprio
//     `parse_stylesheet()` do lado B recebe TEXTO RCSS direto -- não tem interface de arquivo
//     nenhuma, não resolve `<link type="text/rcss">` nenhum (isso é maquinaria de carregador
//     própria do RmlUi, `glintfx::Bootstrap`, confinada a `glintfx/src/rml/` e nunca exercitada
//     pelo módulo standalone `glintfx::uix::style`). Uma fixture cujo RCSS mora num arquivo `.rcss`
//     EXTERNO alcançável só via `<link>` alimentaria portanto o lado A com a folha real, externa,
//     mesclada, e o lado B com uma vazia ARTIFICIALMENTE -- uma "divergência" espúria garantida que
//     mede a própria incapacidade deste harness de buscar um segundo arquivo, não um desacordo real
//     entre dumpers. O `cascade_corpus_sanity.cpp`/`selector_match_corpus_sanity.cpp` (os dois
//     precedentes mais próximos deste repo pra combinar DOM+RCSS de corpus real) já traçaram
//     exatamente esta linha (`find_rml_with_style`, só tags `<style>`) -- este harness herda isso,
//     estreitado mais um passo pra TAMBÉM rejeitar qualquer fixture que TAMBÉM carregue um
//     `<link>` (uma fixture `<style>` que adicionalmente linka RCSS externo ainda alimentaria o
//     lado A com mais que o lado B): confirmado por medição, não suposto --
//     `glintfx/tests/log_throttle_scene.rml` é a única fixture do corpus deste repo com OS DOIS, e
//     é excluída exatamente por isso. Múltiplos blocos `<style>` numa ÚNICA fixture SÃO suportados
//     (casos reais, medidos, existem sob `glintfx/src/uix/dom/test_fixtures/`): o texto cru deles é
//     concatenado, ordem-fonte preservada, numa ÚNICA chamada `parse_stylesheet()` -- batendo com o
//     que o próprio RmlUi faz ao mesclar tags `<style>` sequenciais numa cascata só (ordem-fonte é
//     load-bearing pra desempate de mesma-especificidade, então blocos NUNCA são parseados e
//     cascateados SEPARADAMENTE do jeito que o próprio laço por-bloco do cascade_corpus_sanity.cpp
//     faz pro próprio propósito, mais estreito e não-diferencial, dele).
//
//     QUATRO diretórios de corpus, a mesma disciplina de caminho absoluto NA ÁRVORE-FONTE que o
//     próprio cabeçalho do dom_dump_differential_oracle.cpp já estabeleceu (pra que OS DOIS lados
//     leiam exatamente os mesmos bytes do disco): `glintfx/tests/`, `glintfx/demos/showcase/` (os
//     MESMOS dois que o rcss_dump_corpus_sanity.cpp já varre), `glintfx/src/uix/dom/test_fixtures/`
//     (o corpus da RMLX-1, 16 telas reais do GusWorld, já reusado pela própria suíte
//     `uix_style_*_corpus_sanity` pelo mesmo motivo exato), e `glintfx/src/rml/rcss_dump_test_
//     fixtures/` -- um QUINTO diretório, NOVO, dedicado, que a entrega desta tarefa soma, varrido
//     por NENHUM outro teste deste repo, guardando exatamente uma fixture:
//     `uix_rcss_errata5_border_top_reversed.rml`, o próprio RCSS `#a`/`#b` verbatim da seção 15.2
//     da spec, como ARQUIVO REAL EM DISCO em vez de literal de string C++ inline -- o próprio
//     requisito normativo 3 da seção 14.1 ("toda exceção TEM que ser exercitada por pelo menos uma
//     fixture, e o oráculo TEM que falhar se alguma exceção nunca for exercitada") precisa de uma
//     fixture que ESTE PRÓPRIO oráculo alimenta pelos DOIS dumpers sobre os MESMOS bytes; os dois
//     testes por-lado já existentes, de string inline (`test_15_2_shorthand_order_border_top` do
//     próprio rcss_dump_worked_examples.cpp; `test_worked_example_15_2_shorthand_order` do próprio
//     dumper_sanity.cpp) cada um já prova o próprio byte do PRÓPRIO lado isoladamente (a própria
//     linha do ledger da seção 14.1 diz isso explicitamente, "Ainda faltando: o próprio harness
//     diferencial") mas nunca rodaram o MESMO arquivo por uma comparação diff-e-consulta-seção-
//     14.1 antes desta tarefa.
//
//     Uma falha de parse DOM ou RCSS numa fixture que este escopo aceita NÃO é preocupação deste
//     harness (mesmo precedente "não é preocupação deste item" que o próprio cascade_corpus_
//     sanity.cpp já estabeleceu pras próprias regressões dos parsers DOM/RCSS) -- PULADA, contada,
//     e nomeada na linha de escopo, nunca descartada em silêncio nem contada como divergência. Uma
//     falha de `engine.load()` do lado A numa fixture que o lado B parseou com sucesso É
//     preocupação deste harness (a própria precondição "mesmos bytes" sobre a qual o oráculo
//     inteiro se apoia foi violada) e REPROVA a rodada.
//
//     POR QUE ESTE ARQUIVO MORA EM glintfx/src/rml/ E NÃO EM glintfx/tests/: ele referencia
//     `Rml::Context`/`Rml::ElementDocument` diretamente (metade do oráculo É a árvore real do
//     RmlUi), e os checks (b)/(c) do tools/check_rml_whitelist.sh proíbem exatamente isso num
//     arquivo NOVO fisicamente localizado sob glintfx/tests/ (congelado pelo próprio briefing da
//     RMLX-0). Mesmo raciocínio, mesmo diretório, do dom_dump_differential_oracle.cpp (o próprio
//     precedente RMLX-1/S7 dele) e do rcss_dump_worked_examples.cpp/rcss_dump_corpus_sanity.cpp
//     (os próprios irmãos de teste do lado A deste módulo). Registrado no CTest a partir do
//     glintfx/tests/CMakeLists.txt (um CMakeLists.txt nunca é varrido pelo gate, seja qual for o
//     caminho-fonte que ele nomeia).
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "rcss_dump.hpp"
#include "system_clock.hpp"

#include "uix/dom/dom_tree.hpp"
#include "uix/dom/parser.hpp"
#include "uix/style/dumper.hpp"
#include "uix/style/parser.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLINTFX_RCSS2_TESTS_DIR
#error "GLINTFX_RCSS2_TESTS_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif
#ifndef GLINTFX_RCSS2_DEMOS_DIR
#error "GLINTFX_RCSS2_DEMOS_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif
#ifndef GLINTFX_RCSS2_DOM_FIXTURES_DIR
#error "GLINTFX_RCSS2_DOM_FIXTURES_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif
#ifndef GLINTFX_RCSS2_ORACLE_FIXTURES_DIR
#error "GLINTFX_RCSS2_ORACLE_FIXTURES_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif
#ifndef GLINTFX_RCSS2_UA_STYLESHEET
#error "GLINTFX_RCSS2_UA_STYLESHEET must be defined by CMake (glintfx/tests/CMakeLists.txt)"
#endif

namespace {

namespace fs = std::filesystem;

int g_failures = 0;

void fail(const std::string& what) {
  std::fprintf(stderr, "FAIL: %s\n", what.c_str());
  ++g_failures;
}

std::string read_file(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::vector<fs::path> rml_fixtures_in(const fs::path& dir) {
  std::vector<fs::path> out;
  if (!fs::exists(dir)) return out;
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".rml") out.push_back(entry.path());
  }
  return out;
}

// EN: 🔴 MEASURED BUG, FOUND AND FIXED WHILE BUILDING THIS HARNESS -- NOT a dumper divergence,
//     entirely this harness's OWN fixture-ingestion defect, so fixed here rather than reported as
//     a finding to classify. `cascade_corpus_sanity.cpp`/`selector_match_corpus_sanity.cpp`/
//     `lexer_corpus_sanity.cpp`/`parser_corpus_sanity.cpp` (this repo's own four existing
//     precedents for `extract_style_blocks`) all do a NAIVE, comment-blind substring search for
//     `"<style"`/`"</style"` -- harmless for THEM (none of the four ever diffs the result against
//     side A, so a corrupted extraction never surfaces as a wrong answer, only as noise inside
//     their own already-permissive "parse error? skip, not our concern" branches). It is NOT
//     harmless here: `glintfx/src/uix/dom/test_fixtures/difficulty_menu__lista_hardcore_
//     bloqueado.rml`'s own header COMMENT (an HTML `<!-- ... -->` block) contains the literal prose
//     "files' `<head>/<style>` blocks contains a literal, un-escaped '<'" -- the naive scan finds
//     THAT `<style` (inside the comment, byte-identical to a real open tag from a substring
//     search's point of view) as the block's own start, then finds the file's own REAL `</style>`
//     hundreds of bytes later as the end, and hands `parse_stylesheet()` a "block" that begins with
//     comment PROSE and HTML markup (`-->`, `<rml>`, `<head>`, the real `<style>` tag itself, all as
//     literal bytes) glued in front of the real RCSS. 11 of this repo's own 16 `RMLX-1` corpus
//     fixtures carry this exact boilerplate-comment shape (measured: `grep -c "<style"` returns 2
//     or 3, never 1, on every one of them except `gusworld_battle_cockpit.rml`) -- this was the
//     SINGLE LARGEST contributor to this harness's own first, pre-fix run (most of that run's
//     "divergent" fixtures showed `width`/`height`/`font-family` on `body` silently reverting to
//     their own registry initial, exactly what a garbage-prefixed parse would produce if the
//     REAL `body { ... }` rule further down still happened to parse, sometimes, by accident).
//     THE FIX: strip every `<!-- ... -->` span from the source FIRST (`strip_html_comments()`
//     below), then run the exact same tag-scan on the comment-free text -- mirrors this repo's own
//     precedent for the identical class of bug one layer down (`dom_dump.cpp`'s own `locate_head_
//     tag_span()`/`UIX-HEAD-COMENTARIO`, cited in `dom_dump_differential_oracle.cpp`'s own
//     `kKnownDivergences` comment: made comment/quote/opaque-content-aware after the SAME "the raw
//     scan matched something inside a comment" failure mode). NOT reported as a `docs/uix-rcss.md`
//     section 14 finding, because it never was a dumper disagreement -- see this task's own
//     delivery report for the "harness bug, not dumper divergence" write-up, and for flagging the
//     SAME latent defect (dormant, not yet exercised) in the four sibling files named above.
// PT: 🔴 BUG MEDIDO, ACHADO E CONSERTADO ENQUANTO ESTE HARNESS ERA CONSTRUÍDO -- NÃO é divergência
//     de dumper, é defeito inteiramente PRÓPRIO da ingestão-de-fixture deste harness, então
//     consertado aqui em vez de reportado como achado pra classificar. O cascade_corpus_sanity.cpp/
//     selector_match_corpus_sanity.cpp/lexer_corpus_sanity.cpp/parser_corpus_sanity.cpp (os quatro
//     próprios precedentes existentes deste repo pro extract_style_blocks) todos fazem uma busca de
//     substring INGÊNUA, cega-a-comentário, por `"<style"`/`"</style"` -- inofensiva PRA ELES
//     (nenhum dos quatro nunca diffa o resultado contra o lado A, então uma extração corrompida
//     nunca aparece como resposta errada, só como ruído dentro do próprio ramo já-permissivo deles
//     de "erro de parse? pula, não é minha preocupação"). NÃO é inofensiva aqui: o próprio
//     COMENTÁRIO de cabeçalho (um bloco HTML `<!-- ... -->`) do
//     `glintfx/src/uix/dom/test_fixtures/difficulty_menu__lista_hardcore_bloqueado.rml` contém a
//     prosa literal "files' `<head>/<style>` blocks contains a literal, un-escaped '<'" -- a busca
//     ingênua acha AQUELE `<style` (dentro do comentário, byte-idêntico a uma tag de abertura real
//     do ponto de vista de uma busca de substring) como o próprio início do bloco, depois acha o
//     próprio `</style>` REAL do arquivo centenas de bytes depois como o fim, e entrega ao
//     `parse_stylesheet()` um "bloco" que começa com PROSA de comentário e marcação HTML (`-->`,
//     `<rml>`, `<head>`, a própria tag `<style>` real, tudo como bytes literais) colada na frente do
//     RCSS real. 11 das próprias 16 fixtures de corpus da `RMLX-1` deste repo carregam exatamente
//     essa forma de comentário boilerplate (medido: `grep -c "<style"` devolve 2 ou 3, nunca 1, em
//     toda uma delas exceto o `gusworld_battle_cockpit.rml`) -- este foi o MAIOR contribuinte único
//     da primeira rodada, pré-conserto, deste próprio harness (a maioria das fixtures "divergentes"
//     daquela rodada mostrava `width`/`height`/`font-family` de `body` revertendo em silêncio pro
//     próprio inicial de registro, exatamente o que um parse prefixado-de-lixo produziria se a
//     regra `body { ... }` REAL mais abaixo ainda por acaso conseguisse parsear, às vezes).
//     O CONSERTO: tira todo trecho `<!-- ... -->` da fonte PRIMEIRO (`strip_html_comments()`
//     abaixo), depois roda o MESMO scan de tag sobre o texto sem comentário -- espelha o próprio
//     precedente deste repo pra classe idêntica de bug uma camada abaixo (o próprio `locate_head_
//     tag_span()` do dom_dump.cpp/`UIX-HEAD-COMENTARIO`, citado no próprio comentário do
//     `kKnownDivergences` do dom_dump_differential_oracle.cpp: ficou consciente de
//     comentário/aspas/conteúdo-opaco depois da MESMA falha "o scan cru casou algo dentro de um
//     comentário"). NÃO reportado como achado da seção 14 do docs/uix-rcss.md, porque nunca foi
//     desacordo de dumper -- ver o próprio relatório de entrega desta tarefa pro texto "bug de
//     harness, não divergência de dumper", e pra sinalizar o MESMO defeito latente (adormecido,
//     ainda não exercitado) nos quatro arquivos irmãos nomeados acima.
std::string strip_html_comments(const std::string& source) {
  std::string out;
  out.reserve(source.size());
  std::size_t pos = 0;
  while (pos < source.size()) {
    const std::size_t open = source.find("<!--", pos);
    if (open == std::string::npos) {
      out.append(source, pos, std::string::npos);
      break;
    }
    out.append(source, pos, open - pos);
    const std::size_t close = source.find("-->", open + 4);
    if (close == std::string::npos) {
      // EN: Unterminated comment -- everything from here on is inside it (never re-enters "real"
      //     markup), so this harness treats the rest of the file as consumed and stops.
      // PT: Comentário sem terminador -- tudo daqui em diante está dentro dele (nunca volta pra
      //     marcação "real"), então este harness trata o resto do arquivo como consumido e para.
      break;
    }
    pos = close + 3;
  }
  return out;
}

// EN: Same `<style>`-block extraction `cascade_corpus_sanity.cpp`/`selector_match_corpus_
//     sanity.cpp` already established (duplicated per this repo's own "same test-plumbing,
//     duplicated rather than shared" convention those two files' own header comments state) --
//     returns each `<style>...</style>` block's own INNER text, in source order. Called on
//     `strip_html_comments(rml_source)`'s own output, never the raw source directly -- see that
//     function's own header comment for why.
// PT: A MESMA extração de bloco `<style>` que o cascade_corpus_sanity.cpp/selector_match_corpus_
//     sanity.cpp já estabeleceram (duplicada pela própria convenção "mesmo encanamento-de-teste,
//     duplicado em vez de compartilhado" que o comentário de cabeçalho dos dois já declara) --
//     devolve o próprio texto INTERNO de cada bloco `<style>...</style>`, em ordem-fonte. Chamada
//     sobre a PRÓPRIA saída do strip_html_comments(rml_source), nunca a fonte crua direto -- ver o
//     próprio comentário de cabeçalho daquela função pro motivo.
std::vector<std::string> extract_style_blocks(const std::string& rml_source) {
  std::vector<std::string> blocks;
  std::size_t pos = 0;
  while (true) {
    const std::size_t open_tag = rml_source.find("<style", pos);
    if (open_tag == std::string::npos) break;
    const std::size_t open_end = rml_source.find('>', open_tag);
    if (open_end == std::string::npos) break;
    const std::size_t close_tag = rml_source.find("</style", open_end);
    if (close_tag == std::string::npos) break;
    blocks.push_back(rml_source.substr(open_end + 1, close_tag - (open_end + 1)));
    pos = close_tag + 7;
  }
  return blocks;
}

// EN: Same `ua_stylesheet.hpp` raw-string extraction `lexer_corpus_sanity.cpp`/`parser_corpus_
//     sanity.cpp`/`selector_match_corpus_sanity.cpp` already established (duplicated per this
//     repo's own "same test-plumbing, duplicated rather than shared" convention) -- pulls the
//     `R"rcss( ... )rcss"` payload out of the header's own C++ source text.
//
//     🔴 WHY THIS ORACLE NEEDS IT AND ITS THREE SIBLINGS ABOVE DID NOT (MEASURED, NOT ASSUMED --
//     the first run of this harness against real corpus fixtures reported the UA-stylesheet-driven
//     `display`/`position` defaults on `body`/every `div` as a "divergence" on EVERY SINGLE
//     fixture before this fix): `Bootstrap::load()` (`glintfx/src/rml/bootstrap.cpp`) ALWAYS merges
//     this header's own `kUaStylesheetRcss` into the real RmlUi document as a low-specificity base
//     (`StyleSheetContainer::CombineStyleSheetContainer`, `ua_stylesheet.hpp`'s own header comment)
//     -- side A's `engine.load()` therefore NEVER produces a "just the fixture's own rules"
//     `Style::ComputedValues`, full stop. `cascade_corpus_sanity.cpp`/`selector_match_corpus_
//     sanity.cpp` (this repo's own two closest precedents this file otherwise mirrors) get away
//     WITHOUT merging it because neither ever diffs against side A -- one only checks side B's OWN
//     internal invariants (property count, determinism), the other only uses the UA sheet as an
//     isolated positive-control assertion, never actually cascaded together with a real fixture's
//     own rules. This harness is the FIRST piece of test code in this repo that needs side B to
//     reproduce side A's REAL, UA-plus-author cascade, so it is also the first that must merge the
//     two, matching `Bootstrap::load()`'s own documented merge ORDER (UA rules first / lowest
//     priority, the fixture's own `<style>` rules appended after -- `ua_stylesheet.hpp`'s own
//     header comment: "the document's OWN sheet on TOP of this one", so an author rule of EQUAL
//     specificity to a UA rule still wins).
// PT: A MESMA extração de raw-string do `ua_stylesheet.hpp` que o lexer_corpus_sanity.cpp/parser_
//     corpus_sanity.cpp/selector_match_corpus_sanity.cpp já estabeleceram (duplicada pela própria
//     convenção deste repo) -- extrai o payload `R"rcss( ... )rcss"` do próprio texto-fonte C++ do
//     header.
//
//     🔴 POR QUE ESTE ORÁCULO PRECISA DISTO E OS TRÊS IRMÃOS ACIMA NÃO (MEDIDO, NÃO SUPOSTO -- a
//     primeira rodada deste harness contra fixtures de corpus reais reportou os defaults `display`/
//     `position` vindos da UA-stylesheet em `body`/todo `div` como "divergência" em TODA fixture
//     antes deste conserto): o `Bootstrap::load()` (`glintfx/src/rml/bootstrap.cpp`) SEMPRE mescla
//     o próprio `kUaStylesheetRcss` deste header no documento RmlUi real como base de baixa
//     especificidade (`StyleSheetContainer::CombineStyleSheetContainer`, o próprio comentário de
//     cabeçalho do ua_stylesheet.hpp) -- o `engine.load()` do lado A portanto NUNCA produz um
//     `Style::ComputedValues` "só as regras da própria fixture", ponto final. O cascade_corpus_
//     sanity.cpp/selector_match_corpus_sanity.cpp (os dois precedentes mais próximos deste repo que
//     este arquivo espelha) escapam de mesclar SEM isso porque nenhum dos dois nunca diffa contra o
//     lado A -- um só checa os PRÓPRIOS invariantes internos do lado B (contagem de propriedade,
//     determinismo), o outro só usa a UA sheet como asserção isolada de controle positivo, nunca de
//     fato cascateada junto das regras reais de uma fixture. Este harness é a PRIMEIRA peça de
//     código de teste deste repo que precisa do lado B reproduzir a cascata REAL, UA-mais-autor, do
//     lado A, então é também o primeiro que precisa mesclar os dois, batendo com a própria ORDEM de
//     merge documentada do `Bootstrap::load()` (regras UA primeiro/prioridade mais baixa, as
//     regras `<style>` PRÓPRIAS da fixture somadas depois -- o próprio comentário de cabeçalho do
//     ua_stylesheet.hpp: "a sheet PRÓPRIA do documento POR CIMA desta aqui", então uma regra de
//     autor de especificidade IGUAL a uma regra UA ainda vence).
std::string extract_ua_stylesheet_rcss(const std::string& hpp_source) {
  const std::string open_delim = "R\"rcss(";
  const std::string close_delim = ")rcss\"";
  const std::size_t open = hpp_source.find(open_delim);
  if (open == std::string::npos) return "";
  const std::size_t content_start = open + open_delim.size();
  const std::size_t close = hpp_source.find(close_delim, content_start);
  if (close == std::string::npos) return "";
  return hpp_source.substr(content_start, close - content_start);
}

// ---------------------------------------------------------------------------
// EN: Generic line-diff plumbing -- SAME algorithm `dom_dump_differential_oracle.cpp` already
//     uses (equal-length: pairwise; unequal-length: common-prefix/common-suffix trim around ONE
//     hunk), duplicated rather than shared per this repo's own convention (see that file's own
//     header comment for the full rationale this harness does not re-derive). Unlike the DOM
//     oracle, this dump's own line COUNT is expected to be IDENTICAL on both sides for every
//     structurally-agreeing fixture (both dumps enumerate the SAME fixed 72-entry registry per
//     node, per docs/uix-rcss.md section 3 -- there is no DOM-style "inserting one node renumbers
//     every following sibling's own subtree path" effect here), so the unequal-length branch below
//     firing at all is itself a signal worth reporting distinctly (a structural, not merely
//     content, disagreement -- see main()'s own handling).
// PT: Encanamento de diff de linha genérico -- MESMO algoritmo que o dom_dump_differential_
//     oracle.cpp já usa (mesmo comprimento: par-a-par; comprimento diferente: apara de
//     prefixo/sufixo comum ao redor de UM hunk), duplicado em vez de compartilhado pela própria
//     convenção deste repo (ver o próprio comentário de cabeçalho daquele arquivo pro racional
//     completo, não re-derivado aqui). Diferente do oráculo do DOM, o próprio comprimento de linha
//     deste dump é esperado IDÊNTICO nos dois lados pra toda fixture estruturalmente concordante
//     (os dois dumps enumeram o MESMO registro fixo de 72 entradas por nó, pela seção 3 do
//     docs/uix-rcss.md -- não há efeito estilo-DOM "inserir um nó renumera a subárvore inteira de
//     todo irmão seguinte" aqui), então o próprio ramo de comprimento-diferente abaixo disparar já
//     é, ele mesmo, um sinal que vale reportar distintamente (um desacordo ESTRUTURAL, não só de
//     conteúdo -- ver o próprio tratamento do main()).
// ---------------------------------------------------------------------------
std::vector<std::string> split_lines(const std::string& dump) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start < dump.size()) {
    const std::size_t nl = dump.find('\n', start);
    if (nl == std::string::npos) {
      lines.push_back(dump.substr(start));
      break;
    }
    lines.push_back(dump.substr(start, nl - start));
    start = nl + 1;
  }
  return lines;
}

bool has_non_printable(const std::string& s) {
  return std::any_of(s.begin(), s.end(), [](char c) {
    const unsigned char u = static_cast<unsigned char>(c);
    return u < 0x20 || u > 0x7E;
  });
}

std::string hex_escaped(const std::string& s) {
  static const char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(s.size() * 2);
  for (const char c : s) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u < 0x20 || u > 0x7E) {
      out += "\\x";
      out += kHex[u >> 4];
      out += kHex[u & 0x0F];
    } else {
      out += static_cast<char>(u);
    }
  }
  return out;
}

void report_line(const char* tag, std::size_t n, const std::string* line) {
  if (line == nullptr) {
    std::fprintf(stderr, "      %s: <no line at this position>\n", tag);
    return;
  }
  std::fprintf(stderr, "      %s L%zu: %s\n", tag, n, line->c_str());
  if (has_non_printable(*line)) {
    std::fprintf(stderr, "      %s hex: %s\n", tag, hex_escaped(*line).c_str());
  }
}

struct DiffEntry {
  std::size_t rml_lineno = 0; // 1-based; meaningful only when present_rml
  std::size_t uix_lineno = 0; // 1-based; meaningful only when present_uix
  bool present_rml = false;
  bool present_uix = false;
  std::string rml_line;
  std::string uix_line;
};

std::vector<DiffEntry> diff_lines(const std::vector<std::string>& a, const std::vector<std::string>& b) {
  std::vector<DiffEntry> out;
  if (a.size() == b.size()) {
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (a[i] == b[i]) continue;
      DiffEntry e;
      e.rml_lineno = i + 1;
      e.uix_lineno = i + 1;
      e.present_rml = true;
      e.present_uix = true;
      e.rml_line = a[i];
      e.uix_line = b[i];
      out.push_back(std::move(e));
    }
    return out;
  }

  std::size_t prefix = 0;
  while (prefix < a.size() && prefix < b.size() && a[prefix] == b[prefix]) ++prefix;
  const std::size_t max_suffix = std::min(a.size(), b.size()) - prefix;
  std::size_t suffix = 0;
  while (suffix < max_suffix && a[a.size() - 1 - suffix] == b[b.size() - 1 - suffix]) ++suffix;
  const std::size_t a_end = a.size() - suffix;
  const std::size_t b_end = b.size() - suffix;
  const std::size_t hunk = std::max(a_end - prefix, b_end - prefix);
  for (std::size_t k = 0; k < hunk; ++k) {
    DiffEntry e;
    if (prefix + k < a_end) {
      e.present_rml = true;
      e.rml_lineno = prefix + k + 1;
      e.rml_line = a[prefix + k];
    }
    if (prefix + k < b_end) {
      e.present_uix = true;
      e.uix_lineno = prefix + k + 1;
      e.uix_line = b[prefix + k];
    }
    out.push_back(std::move(e));
  }
  return out;
}

// EN: `line` is a full `<path> PROP <name>=<value>` dump record (never a `STATE ...`/`<path>
//     PROPS <n>` structural line -- callers check `.has_value()` and treat a `nullopt` structural
//     line as UNKNOWN by construction, never attempted against `kKnownDivergences`, since every
//     row there is a property-VALUE mismatch by definition).
// PT: `line` é um registro `<caminho> PROP <nome>=<valor>` completo de dump (nunca uma linha
//     estrutural `STATE ...`/`<caminho> PROPS <n>` -- chamadores checam `.has_value()` e tratam
//     uma linha estrutural com `nullopt` como UNKNOWN por construção, nunca tentada contra
//     `kKnownDivergences`, já que toda linha de lá é um descasamento de VALOR de propriedade por
//     definição).
struct ParsedProp {
  std::string path;
  std::string name;
  std::string value;
};

std::optional<ParsedProp> parse_prop_line(const std::string& line) {
  static const std::string kMarker = " PROP ";
  const std::size_t marker_pos = line.find(kMarker);
  if (marker_pos == std::string::npos) return std::nullopt;
  const std::size_t eq_pos = line.find('=', marker_pos + kMarker.size());
  if (eq_pos == std::string::npos) return std::nullopt;
  ParsedProp p;
  p.path = line.substr(0, marker_pos);
  p.name = line.substr(marker_pos + kMarker.size(), eq_pos - (marker_pos + kMarker.size()));
  p.value = line.substr(eq_pos + 1);
  return p;
}

// EN: 0-based line index of the FIRST line equal to `"STATE hover-all"` in `lines`, or
//     `lines.size()` (every index is "before" it) if absent -- absence is itself reported as a
//     format-invariant failure by `main()`'s own STATE-block checks, so this helper never needs to
//     signal it a second way.
// PT: Índice 0-based da PRIMEIRA linha igual a `"STATE hover-all"` em `lines`, ou `lines.size()`
//     (todo índice é "antes" dela) se ausente -- a ausência já é reportada como falha de
//     invariante-de-formato pelas próprias checagens de bloco STATE do main(), então este auxiliar
//     nunca precisa sinalizá-la de uma segunda forma.
std::size_t hover_all_boundary(const std::vector<std::string>& lines) {
  const auto it = std::find(lines.begin(), lines.end(), "STATE hover-all");
  return static_cast<std::size_t>(std::distance(lines.begin(), it));
}

const char* state_of(std::size_t one_based_lineno, std::size_t hover_boundary) {
  return (one_based_lineno - 1 >= hover_boundary) ? "hover-all" : "none";
}

// ---------------------------------------------------------------------------
// EN: `docs/uix-rcss.md` section 14.1's own ledger, MIRRORED here as a generic, table-driven pin
//     -- KEEP IN SYNC WITH THAT DOCUMENT, in BOTH language halves. A row here without a matching
//     row there (or vice versa) is a bookkeeping bug in its own right: the document is what a
//     human/the líder reads and signs off on, this table is what the machine checks, and they are
//     two views of one fact (same "two views" framing `dom_dump_differential_oracle.cpp`'s own
//     `kKnownDivergences` comment already gives its own DOM-ledger mirror).
// PT: O próprio ledger da seção 14.1 do docs/uix-rcss.md, ESPELHADO aqui como um pin genérico,
//     guiado-por-tabela -- MANTER EM SINCRONIA com aquele documento, nas DUAS metades de idioma.
//     Uma linha aqui sem linha correspondente lá (ou vice-versa) é, ela mesma, um bug de
//     escrituração: o documento é o que um humano/o líder lê e aprova, esta tabela é o que a
//     máquina checa, e as duas são duas vistas de um fato só (mesmo enquadramento "duas vistas" que
//     o próprio comentário do `kKnownDivergences` do dom_dump_differential_oracle.cpp já dá pro
//     próprio espelho do ledger do DOM).
// ---------------------------------------------------------------------------
struct KnownDivergence {
  const char* fixture;    // file name only, never the absolute path
  const char* path;       // node path, e.g. "body/1"
  const char* property;   // property name, e.g. "border-top-color"
  const char* rml_value;  // exact side-A byte
  const char* uix_value;  // exact side-B byte
  const char* ledger_ref; // where in docs/uix-rcss.md section 14.1 this is written down
};

// EN: Mirrors `docs/uix-rcss.md` section 14.1's own "Deliberate divergences: N" count line
//     VERBATIM -- asserted equal to `kKnownDivergences.size()` in `main()`. A future implementer
//     who adds a líder-approved row to that document's own table MUST bump this constant in the
//     SAME commit, or this harness fails loudly at startup rather than silently drifting.
// PT: Espelha a própria linha de contagem "Deliberate divergences: N" da seção 14.1 do
//     docs/uix-rcss.md AO PÉ DA LETRA -- asserida igual a `kKnownDivergences.size()` no main(). Um
//     futuro implementador que somar uma linha aprovada-pelo-líder àquela tabela do documento TEM
//     que subir esta constante no MESMO commit, ou este harness falha alto na partida em vez de
//     derivar em silêncio.
constexpr std::size_t kDeclaredDivergenceCount = 1;

const std::vector<KnownDivergence>& known_divergences() {
  static const std::vector<KnownDivergence> rows = {
      // EN: `UIX-RCSS-ERRATA-5`, 2026-08-06 -- `#b { border-top: #7A5A2E 1dp; }` (reversed-order
      //     `border-top` shorthand). Side A (upstream's own in-loop, no-rollback partial
      //     `SetProperty`) keeps `border-top-color`; side B (this engine's own atomic-discard: the
      //     whole malformed shorthand declaration is dropped, both longhands fall back to the
      //     registry initial) does not. Applies in BOTH `STATE` blocks (`none` and `hover-all`) --
      //     `#b` is never touched by any `:hover` rule in this fixture, so the mismatch reproduces
      //     identically in both (`docs/uix-rcss.md` section 15.2's own worked example, `dumper_
      //     sanity.cpp::test_worked_example_15_2_shorthand_order`'s own `count_exact_line(...) == 2`
      //     assertion already proves this); the pin therefore keys on (fixture, path, property,
      //     both byte values) and deliberately does NOT include `STATE` in the match key. `border-
      //     top-width` is explicitly NOT part of this exception (both sides agree on it, for
      //     unrelated reasons on each side) -- it is simply absent from this table, so an
      //     UNEXPECTED future `border-top-width` mismatch on this same fixture would correctly
      //     surface as a brand-new, UNKNOWN divergence, never silently swallowed by this row.
      // PT: `UIX-RCSS-ERRATA-5`, 2026-08-06 -- `#b { border-top: #7A5A2E 1dp; }` (shorthand
      //     `border-top` de ordem revertida). O lado A (a própria escrita parcial, em-laço,
      //     sem-rollback, do `SetProperty` do upstream) mantém `border-top-color`; o lado B (o
      //     próprio descarte atômico deste motor: a declaração de shorthand malformada inteira é
      //     descartada, os dois longhands caem pro inicial de registro) não mantém. Aplica nos DOIS
      //     blocos `STATE` (`none` e `hover-all`) -- `#b` nunca é tocado por regra `:hover` nenhuma
      //     nesta fixture, então o descasamento reproduz identicamente nos dois (o próprio exemplo
      //     trabalhado da seção 15.2 do docs/uix-rcss.md, a própria asserção
      //     `count_exact_line(...) == 2` do `dumper_sanity.cpp::test_worked_example_15_2_
      //     shorthand_order` já prova isto); o pin portanto chaveia em (fixture, caminho,
      //     propriedade, os dois valores de byte) e deliberadamente NÃO inclui `STATE` na chave de
      //     casamento. `border-top-width` está explicitamente FORA desta exceção (os dois lados
      //     concordam nela, por motivos não-relacionados de cada lado) -- está simplesmente ausente
      //     desta tabela, então um descasamento FUTURO inesperado de `border-top-width` nesta mesma
      //     fixture corretamente apareceria como uma divergência NOVA, UNKNOWN, nunca engolida em
      //     silêncio por esta linha.
      {"uix_rcss_errata5_border_top_reversed.rml", "body/1", "border-top-color", "#7a5a2eff", "#000000ff",
       "docs/uix-rcss.md section 14.1, row 1 (UIX-RCSS-ERRATA-5, 2026-08-06)"},
  };
  return rows;
}

bool matches(const KnownDivergence& k, const std::string& fixture, const DiffEntry& d) {
  if (fixture != k.fixture) return false;
  if (!d.present_rml || !d.present_uix) return false;
  const std::optional<ParsedProp> rml_p = parse_prop_line(d.rml_line);
  const std::optional<ParsedProp> uix_p = parse_prop_line(d.uix_line);
  if (!rml_p.has_value() || !uix_p.has_value()) return false;
  if (rml_p->path != k.path || rml_p->name != k.property) return false;
  if (uix_p->path != k.path || uix_p->name != k.property) return false;
  if (rml_p->value != k.rml_value || uix_p->value != k.uix_value) return false;
  return true;
}

// EN: A fixture is IN SCOPE iff it carries at least one `<style` block and zero `<link` tags --
//     see this file's own header comment, "CORPUS SCOPE", for the full reasoning (side B has no
//     external-file resolution, so a `<link>`-based fixture would feed the two sides different
//     bytes by construction, not by dumper disagreement).
// PT: Uma fixture está DENTRO DO ESCOPO sse carrega pelo menos um bloco `<style` e zero tags
//     `<link` -- ver o próprio comentário de cabeçalho deste arquivo, "ESCOPO DE CORPUS", pro
//     racional completo (o lado B não tem resolução de arquivo externo nenhuma, então uma fixture
//     baseada em `<link>` alimentaria os dois lados com bytes diferentes por construção, não por
//     desacordo de dumper).
// EN: Takes the ALREADY comment-stripped source (see `strip_html_comments`'s own header comment)
//     -- a fixture whose only `<style`/`<link` occurrence lives inside an HTML comment must read
//     as carrying NEITHER, matching what `extract_style_blocks` itself will actually find.
// PT: Recebe a fonte JÁ sem comentário (ver o próprio comentário de cabeçalho do
//     strip_html_comments) -- uma fixture cuja única ocorrência de `<style`/`<link` mora dentro de
//     um comentário HTML tem que ler como não carregando NENHUM dos dois, batendo com o que o
//     próprio extract_style_blocks vai de fato achar.
bool in_scope(const std::string& comment_stripped_source) {
  return comment_stripped_source.find("<style") != std::string::npos &&
         comment_stripped_source.find("<link") == std::string::npos;
}

} // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("rcss-dump-differential-oracle", 320, 240)) {
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

  // EN: Loaded ONCE, outside the fixture loop -- the same bytes prepend to every fixture's own
  //     merged style text (see `extract_ua_stylesheet_rcss`'s own header comment for why this
  //     harness, uniquely among this repo's own corpus tests, needs it at all).
  // PT: Carregada UMA VEZ, fora do laço de fixtures -- os mesmos bytes prefixam o texto de estilo
  //     mesclado de toda fixture (ver o próprio comentário de cabeçalho do
  //     extract_ua_stylesheet_rcss pro motivo de este harness, sozinho entre os testes de corpus
  //     deste repo, precisar disto).
  const std::string ua_rcss = extract_ua_stylesheet_rcss(read_file(fs::path(GLINTFX_RCSS2_UA_STYLESHEET)));
  if (ua_rcss.empty()) {
    fail("extract_ua_stylesheet_rcss() found no R\"rcss( ... )rcss\" payload in " +
         std::string(GLINTFX_RCSS2_UA_STYLESHEET) +
         " -- side A's engine.load() still merges it, so a comparison without it would not measure "
         "what this task claims to measure");
  }

  if (known_divergences().size() != kDeclaredDivergenceCount) {
    fail("kKnownDivergences.size() (" + std::to_string(known_divergences().size()) +
         ") != kDeclaredDivergenceCount (" + std::to_string(kDeclaredDivergenceCount) +
         ") -- this table has drifted from docs/uix-rcss.md section 14.1's own declared "
         "\"Deliberate divergences: N\" count line; fix whichever one is stale before trusting "
         "any result below");
  }

  // EN: Collect candidate fixtures from all four corpus directories into a SORTED, DEDUPED set
  //     (a `std::set<fs::path>` rather than a `std::vector` -- the four directories are disjoint
  //     in this repo today, but deduping by construction costs nothing and removes any future
  //     temptation to reason about whether two of them might overlap).
  // PT: Coleta fixtures candidatas dos quatro diretórios de corpus num conjunto ORDENADO,
  //     DEDUPLICADO (um `std::set<fs::path>` em vez de `std::vector` -- os quatro diretórios são
  //     disjuntos neste repo hoje, mas deduplicar por construção não custa nada e remove qualquer
  //     tentação futura de raciocinar se dois deles poderiam se sobrepor).
  std::set<fs::path> fixture_set;
  for (const char* dir : {GLINTFX_RCSS2_TESTS_DIR, GLINTFX_RCSS2_DEMOS_DIR, GLINTFX_RCSS2_DOM_FIXTURES_DIR,
                          GLINTFX_RCSS2_ORACLE_FIXTURES_DIR}) {
    for (fs::path& p : rml_fixtures_in(fs::path(dir))) fixture_set.insert(std::move(p));
  }
  const std::vector<fs::path> fixtures(fixture_set.begin(), fixture_set.end());
  if (fixtures.empty()) {
    fail("corpus is non-empty (found 0 .rml fixtures across the four GLINTFX_RCSS2_* directories)");
  }

  std::size_t scanned = 0;
  std::size_t out_of_scope = 0;
  std::size_t skipped_dom_parse = 0;
  std::size_t skipped_rcss_parse = 0;
  std::size_t compared = 0;
  std::size_t identical = 0;
  std::size_t divergent_fixtures = 0;
  std::size_t structural_mismatches = 0;
  std::vector<bool> known_hit(known_divergences().size(), false);

  for (const fs::path& path : fixtures) {
    ++scanned;
    const std::string abspath = path.string();
    const std::string name = path.filename().string();
    const std::string source = read_file(path);
    const std::string stripped_source = strip_html_comments(source);

    if (!in_scope(stripped_source)) {
      ++out_of_scope;
      continue;
    }

    // EN: side B first -- cheap (no GL, no RmlUi), and a parse failure on either half means there
    //     is nothing to compare, so paying for the RmlUi load would be wasted work (same ordering
    //     rationale `dom_dump_differential_oracle.cpp`'s own main() already gives).
    // PT: lado B primeiro -- barato (sem GL, sem RmlUi), e uma falha de parse em qualquer metade
    //     significa que não há o que comparar, então pagar o load do RmlUi seria trabalho jogado
    //     fora (mesmo racional de ordenação que o próprio main() do dom_dump_differential_
    //     oracle.cpp já dá).
    glintfx::uix::ParseResult dom_result = glintfx::uix::parse_document(source);
    if (dom_result.error.has_value()) {
      ++skipped_dom_parse;
      std::fprintf(stderr, "SKIP (DOM parse error, not this harness's own concern): %s -- %s\n", name.c_str(),
                   dom_result.error->message.c_str());
      continue;
    }

    // EN: `stripped_source`, not `source` -- see `strip_html_comments`'s own header comment.
    //     `parse_document(source)` two lines above deliberately stays on the RAW, unstripped bytes:
    //     the DOM parser has its own real `<!-- ... -->` handling (this repo's own RML/XML-style
    //     comment support), and this harness's crude stripping (delete, no placeholder) is only
    //     safe for THIS file's own naive tag-scan, not a substitute for that parser's own semantics
    //     (a dropped comment could merge two text runs this harness has no business merging).
    // PT: `stripped_source`, não `source` -- ver o próprio comentário de cabeçalho do
    //     strip_html_comments. O parse_document(source) duas linhas acima fica deliberadamente nos
    //     bytes CRUS, sem stripar: o parser de DOM tem o próprio tratamento real de `<!-- ... -->`
    //     (o próprio suporte a comentário estilo RML/XML deste repo), e o stripping cru deste
    //     arquivo (apaga, sem placeholder) só é seguro pro PRÓPRIO scan ingênuo de tag deste
    //     arquivo, não um substituto pra própria semântica daquele parser (um comentário apagado
    //     poderia fundir dois trechos de texto que este harness não tem nada a ver com fundir).
    const std::vector<std::string> style_blocks = extract_style_blocks(stripped_source);
    // EN: UA stylesheet FIRST (lowest priority / earliest in document order), then this fixture's
    //     own `<style>` block(s) in source order -- mirrors `Bootstrap::load()`'s own merge order
    //     exactly (see `extract_ua_stylesheet_rcss`'s own header comment).
    // PT: A UA stylesheet PRIMEIRO (prioridade mais baixa/mais cedo na ordem-fonte), depois o(s)
    //     PRÓPRIO(S) bloco(s) `<style>` desta fixture em ordem-fonte -- espelha exatamente a
    //     própria ordem de merge do Bootstrap::load() (ver o próprio comentário de cabeçalho do
    //     extract_ua_stylesheet_rcss).
    std::string merged_style = ua_rcss;
    for (const std::string& block : style_blocks) {
      merged_style += "\n";
      merged_style += block;
    }
    glintfx::uix::style::SheetParseResult sheet_result = glintfx::uix::style::parse_stylesheet(merged_style);
    if (sheet_result.error.has_value() || !sheet_result.sheet) {
      ++skipped_rcss_parse;
      std::fprintf(stderr, "SKIP (RCSS parse error, not this harness's own concern): %s\n", name.c_str());
      continue;
    }
    const std::string uix_dump =
        glintfx::uix::style::dump_style(*sheet_result.sheet, dom_result.document->body(), 1.0f);

    // --- side A: the real RmlUi engine, dp_ratio left at its own 1.0 default (matching side B's
    //     own dp_ratio argument above, and matching every other UIX-RCSS-DUMP-A test in this repo,
    //     none of which calls set_dp_ratio()). ---------------------------------------------------
    if (!engine.load(abspath.c_str())) {
      fail(name +
           ": engine.load() returned false, but side B parsed this SAME fixture successfully "
           "-- the \"same bytes into both sides\" precondition this whole oracle rests on is "
           "violated for this fixture");
      continue;
    }
    // EN: Close()/UnloadDocument() of the PREVIOUS fixture's document defers actual destruction to
    //     the next Context::Update() (same reasoning `dom_dump_corpus_sanity.cpp`/`dom_dump_
    //     differential_oracle.cpp` already document) -- drive it before reading GetDocument(0) so
    //     THIS fixture's document is unambiguously at index 0.
    // PT: Close()/UnloadDocument() do documento da fixture ANTERIOR difere a destruição real pro
    //     próximo Context::Update() (mesmo raciocínio que o dom_dump_corpus_sanity.cpp/dom_dump_
    //     differential_oracle.cpp já documentam) -- dispara antes de ler GetDocument(0) pra que o
    //     documento DESTA fixture esteja inequivocamente no índice 0.
    engine.update();
    Rml::ElementDocument* doc = ctx->GetDocument(0);
    if (!doc) {
      fail(name + ": GetDocument(0) returned null after a successful load()");
      continue;
    }
    const std::string rml_dump = glintfx::rcss_dump_document(doc);

    ++compared;

    // EN: Format-level invariants on BOTH sides, independent of whether they agree with each
    //     other -- an empty or unterminated dump is a broken dumper, not "a divergence" (same
    //     "conflating the two would let a catastrophic failure masquerade as one more row"
    //     reasoning `dom_dump_differential_oracle.cpp`'s own main() already states).
    // PT: Invariantes de nível-formato nos DOIS lados, independente de eles concordarem entre si --
    //     um dump vazio ou sem terminador é um dumper quebrado, não "uma divergência" (mesmo
    //     raciocínio "confundir os dois deixaria uma falha catastrófica se disfarçar de mais uma
    //     linha" que o próprio main() do dom_dump_differential_oracle.cpp já declara).
    if (rml_dump.empty() || rml_dump.back() != '\n') {
      fail(name + ": rml-side dump is empty or lacks the trailing newline (docs/uix-rcss.md section 3)");
    }
    if (uix_dump.empty() || uix_dump.back() != '\n') {
      fail(name + ": uix-side dump is empty or lacks the trailing newline (docs/uix-rcss.md section 3)");
    }

    if (rml_dump == uix_dump) {
      ++identical;
      std::printf("%s: IDENTICAL (%zu bytes)\n", name.c_str(), rml_dump.size());
      doc->Close();
      continue;
    }

    ++divergent_fixtures;
    const std::vector<std::string> a = split_lines(rml_dump);
    const std::vector<std::string> b = split_lines(uix_dump);
    if (a.size() != b.size()) {
      ++structural_mismatches;
      std::fprintf(stderr,
                   "--- %s: STRUCTURAL mismatch -- rml %zu lines vs uix %zu lines (the two sides "
                   "should ALWAYS agree on line count for this format; this is a stronger signal "
                   "than a same-position content mismatch, see this file's own header comment)\n",
                   name.c_str(), a.size(), b.size());
    }
    const std::size_t rml_hover_boundary = hover_all_boundary(a);
    const std::size_t uix_hover_boundary = hover_all_boundary(b);
    const std::vector<DiffEntry> diffs = diff_lines(a, b);
    std::printf("%s: DIVERGENT (rml %zu bytes/%zu lines, uix %zu bytes/%zu lines, %zu differing line(s))\n",
                name.c_str(), rml_dump.size(), a.size(), uix_dump.size(), b.size(), diffs.size());

    std::size_t unknown = 0;
    std::size_t shown = 0;
    for (const DiffEntry& d : diffs) {
      bool is_known = false;
      for (std::size_t k = 0; k < known_divergences().size(); ++k) {
        if (matches(known_divergences()[k], name, d)) {
          known_hit[k] = true;
          is_known = true;
          break;
        }
      }
      if (is_known) continue;
      ++unknown;
      if (shown++ >= 40) continue; // still counted, just not re-printed
      const char* rml_state = d.present_rml ? state_of(d.rml_lineno, rml_hover_boundary) : "?";
      const char* uix_state = d.present_uix ? state_of(d.uix_lineno, uix_hover_boundary) : "?";
      std::fprintf(stderr, "--- %s: UNKNOWN divergence #%zu (rml STATE %s / uix STATE %s)\n", name.c_str(), unknown,
                   rml_state, uix_state);
      report_line("rml", d.rml_lineno, d.present_rml ? &d.rml_line : nullptr);
      report_line("uix", d.uix_lineno, d.present_uix ? &d.uix_line : nullptr);
    }
    if (unknown > 0) {
      fail(name + ": " + std::to_string(unknown) +
           " divergence(s) not in docs/uix-rcss.md section 14.1's table (see the rml/uix line "
           "pairs above; a NEW divergence is a RESULT to be classified and written up, never a "
           "dumper to be edited until the diff goes green)");
    }

    doc->Close();
  }

  // EN: The other half of the pin -- see `known_divergences()`'s own comment.
  // PT: A outra metade do pin -- ver o próprio comentário do known_divergences().
  for (std::size_t k = 0; k < known_divergences().size(); ++k) {
    if (known_hit[k]) continue;
    fail(std::string("known divergence no longer reproduces (fixture ") + known_divergences()[k].fixture +
         ", ledger: " + known_divergences()[k].ledger_ref +
         ") -- if a dumper legitimately changed, get the líder to update docs/uix-rcss.md section "
         "14.1 and REMOVE the row here in the same commit; do not leave a pin that no longer "
         "measures anything");
  }

  // EN: The scope line, printed unconditionally -- a zero declared distinguishes "there were none"
  //     from "nobody looked" (this repo's own recurring convention, e.g. dom_dump_differential_
  //     oracle.cpp's own identical closing block).
  // PT: A linha de escopo, impressa incondicionalmente -- um zero declarado distingue "não houve"
  //     de "ninguém olhou" (convenção recorrente deste repo, ex. o próprio bloco de fechamento
  //     idêntico do dom_dump_differential_oracle.cpp).
  std::printf(
      "SCOPE: %zu fixtures scanned, %zu out-of-scope (no <style> or has <link>), %zu skipped (DOM "
      "parse error), %zu skipped (RCSS parse error), %zu compared, %zu identical, %zu divergent "
      "(%zu structural), %zu known divergence(s) pinned\n",
      scanned, out_of_scope, skipped_dom_parse, skipped_rcss_parse, compared, identical, divergent_fixtures,
      structural_mismatches, known_divergences().size());

  if (divergent_fixtures > 0) {
    std::printf(
        "WARNING: %zu of %zu fixtures do NOT agree byte-for-byte between the two dumpers. Every "
        "divergence pinned above is KNOWN and WRITTEN DOWN in docs/uix-rcss.md section 14.1 -- "
        "none of them is fixed, and none is a bug to patch away. Read that section before "
        "touching either dumper.\n",
        divergent_fixtures, compared);
  }

  if (g_failures == 0) {
    std::puts("rcss_dump_differential_oracle OK");
    return 0;
  }
  std::printf("rcss_dump_differential_oracle: %d failure(s)\n", g_failures);
  return 10;
}
