// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S7 -- THE DIFFERENTIAL ORACLE. For every fixture in the `RMLX-1` corpus
//     (glintfx/src/uix/dom/test_fixtures/, the 16 real GusWorld screens captured verbatim), this
//     harness runs BOTH independently-written dumpers over the SAME source bytes and compares
//     their output BYTE FOR BYTE:
//       - side A (`S6a`, glintfx/src/rml/dom_dump.cpp): walks the REAL `Rml::ElementDocument`
//         produced by RmlUi's own parser;
//       - side B (`S6b`, glintfx/src/uix/dom/dumper.cpp): walks glintfx's OWN DOM, produced by
//         `glintfx::uix::parse_document()`.
//     Both emit the canonical format `docs/uix-dom.md` defines -- that document is the SOLE
//     contract; neither dumper's author read the other's source (uix-dom.md's own "Why this
//     document exists" section explains why a shared author would make the oracle agree with
//     itself instead of with reality).
//
//     🔴 THE RULE THIS FILE EXISTS TO ENFORCE, stated where a future maintainer cannot miss it:
//     a divergence this harness finds is a RESULT, not a failure. Editing either dumper (or this
//     harness's comparison) to flatten a difference and make the diff go green is FRAUDING THE
//     ORACLE -- the single worst outcome of this wave, because it converts a measurement into a
//     tautology. Every real divergence goes into `docs/uix-dom.md` section 9's ledger AND into
//     `kKnownDivergences` below, where it is PINNED: the harness fails both when a NEW divergence
//     appears and when a KNOWN one silently disappears. "Known" here means "measured, named and
//     written down", never "tolerated and forgotten".
//
//     WHY THIS FILE LIVES IN glintfx/src/rml/ AND NOT glintfx/tests/: it references
//     `Rml::Context`/`Rml::ElementDocument` directly (it has to -- half the oracle IS the real
//     RmlUi tree), and `tools/check_rml_whitelist.sh` checks (b)/(c) forbid exactly that in a NEW
//     file physically located under glintfx/tests/ (that whitelist is deliberately frozen by the
//     RMLX-0 brief that built the gate). Same reasoning, same directory, as its two siblings
//     dom_dump_corpus_sanity.cpp / dom_dump_determinism_sanity.cpp -- read either of their header
//     comments for the longer version. It is registered with CTest from
//     glintfx/tests/CMakeLists.txt (a CMakeLists.txt is never scanned by the gate, whatever source
//     path it names, so wiring the build there does not reopen the hole the gate closes).
//
//     SAME BYTES INTO BOTH SIDES -- the property without which this whole harness proves nothing:
//     fixtures are addressed by ABSOLUTE SOURCE-TREE path (GLINTFX_RMLX1_CORPUS_DIR, resolved at
//     CONFIGURE time), never a build-dir copy. `engine.load(abspath)` makes RmlUi read that exact
//     file, and `read_file(abspath)` reads the same file for (i) `dom_dump_document`'s own raw
//     `<head>` scan and (ii) `parse_document()`'s entire input. If the two sides could ever read
//     different bytes -- a stale copy, a configure_file() that normalized a line ending -- the
//     diff would be measuring the copy step, not the parsers.
// PT: RMLX-1/S7 -- O ORÁCULO DIFERENCIAL. Para toda fixture do corpus da `RMLX-1`
//     (glintfx/src/uix/dom/test_fixtures/, as 16 telas reais do GusWorld capturadas verbatim),
//     este harness roda os DOIS dumpers escritos de forma independente sobre os MESMOS bytes-fonte
//     e compara a saída BYTE A BYTE:
//       - lado A (`S6a`, glintfx/src/rml/dom_dump.cpp): percorre o `Rml::ElementDocument` REAL
//         produzido pelo próprio parser do RmlUi;
//       - lado B (`S6b`, glintfx/src/uix/dom/dumper.cpp): percorre o DOM PRÓPRIO da glintfx,
//         produzido por `glintfx::uix::parse_document()`.
//     Os dois emitem o formato canônico que o `docs/uix-dom.md` define -- aquele documento é o
//     ÚNICO contrato; nenhum dos dois autores leu o fonte do outro (a própria seção "Por que este
//     documento existe" do uix-dom.md explica por que um autor compartilhado faria o oráculo
//     concordar consigo mesmo em vez de com a realidade).
//
//     🔴 A REGRA QUE ESTE ARQUIVO EXISTE PRA APLICAR, escrita onde um mantenedor futuro não tem
//     como não ver: uma divergência que este harness acha é RESULTADO, não falha. Editar qualquer
//     um dos dumpers (ou a comparação deste harness) pra achatar uma diferença e fazer o diff dar
//     verde é FRAUDAR O ORÁCULO -- o pior desfecho possível desta onda, porque converte uma
//     medição numa tautologia. Toda divergência real vai pro ledger da seção 9 do
//     `docs/uix-dom.md` E pro `kKnownDivergences` abaixo, onde fica PINADA: o harness falha tanto
//     quando uma divergência NOVA aparece quanto quando uma CONHECIDA some em silêncio.
//     "Conhecida" aqui significa "medida, nomeada e escrita", nunca "tolerada e esquecida".
//
//     POR QUE ESTE ARQUIVO MORA EM glintfx/src/rml/ E NÃO EM glintfx/tests/: ele referencia
//     `Rml::Context`/`Rml::ElementDocument` diretamente (tem que referenciar -- metade do oráculo É
//     a árvore real do RmlUi), e os checks (b)/(c) do `tools/check_rml_whitelist.sh` proíbem
//     exatamente isso num arquivo NOVO fisicamente localizado sob glintfx/tests/ (aquela whitelist
//     é deliberadamente congelada pelo próprio brief da RMLX-0 que construiu o gate). Mesmo
//     raciocínio, mesmo diretório, dos dois irmãos dom_dump_corpus_sanity.cpp /
//     dom_dump_determinism_sanity.cpp -- ler o comentário de cabeçalho de qualquer um dos dois pra
//     versão longa. É registrado no CTest a partir de glintfx/tests/CMakeLists.txt (um
//     CMakeLists.txt nunca é varrido pelo gate, seja qual for o caminho-fonte que ele nomeia, então
//     conectar o build lá não reabre o furo que o gate fecha).
//
//     OS MESMOS BYTES NOS DOIS LADOS -- a propriedade sem a qual este harness inteiro não prova
//     nada: fixtures são endereçadas por caminho ABSOLUTO NA ÁRVORE-FONTE
//     (GLINTFX_RMLX1_CORPUS_DIR, resolvido em tempo de CONFIGURE), nunca uma cópia no dir de build.
//     `engine.load(abspath)` faz o RmlUi ler exatamente aquele arquivo, e `read_file(abspath)` lê o
//     mesmo arquivo para (i) o próprio scan cru de `<head>` do `dom_dump_document` e (ii) a entrada
//     inteira do `parse_document()`. Se os dois lados pudessem ler bytes diferentes -- uma cópia
//     velha, um configure_file() que normalizou uma quebra de linha -- o diff estaria medindo o
//     passo de cópia, não os parsers.
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "dom_dump.hpp"
#include "system_clock.hpp"

#include "uix/dom/dumper.hpp"
#include "uix/dom/parser.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef GLINTFX_RMLX1_CORPUS_DIR
#error "GLINTFX_RMLX1_CORPUS_DIR must be defined by CMake (glintfx/tests/CMakeLists.txt)"
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
  std::sort(out.begin(), out.end());
  return out;
}

// EN: Split a dump into lines WITHOUT the trailing '\n'. The dump always ends with exactly one
//     trailing newline (uix-dom.md section 1), so the final empty segment is dropped -- keeping it
//     would add a phantom "" line to every comparison. The trailing-newline property itself is
//     checked separately (see main()), never inferred from the line split.
// PT: Fatia um dump em linhas SEM o '\n' final. O dump sempre termina com exatamente um newline
//     final (seção 1 do uix-dom.md), então o segmento vazio final é descartado -- mantê-lo somaria
//     uma linha "" fantasma a toda comparação. A própria propriedade do newline final é checada em
//     separado (ver main()), nunca inferida da fatia em linhas.
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

// EN: True if `s` contains any byte outside printable ASCII (0x20..0x7E). Such a line is ALSO
//     printed hex-escaped by `report_line()` -- uix-dom.md section 6c's own warning is that
//     U+00A0 (UTF-8 C2 A0) is visually indistinguishable from a plain space in every diff viewer,
//     so a divergence that "looks like a no-op" must never be reportable as one here.
// PT: Verdadeiro se `s` contém qualquer byte fora do ASCII imprimível (0x20..0x7E). Uma linha
//     assim é TAMBÉM impressa escapada-em-hex por `report_line()` -- o próprio aviso da seção 6c
//     do uix-dom.md é que U+00A0 (UTF-8 C2 A0) é visualmente indistinguível de um espaço comum em
//     todo visualizador de diff, então uma divergência que "parece um no-op" nunca pode ser
//     reportável como tal aqui.
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

// EN: `tag` is the side marker ("rml"/"uix"), `n` the 1-based line number in THAT side's dump.
//     A null `line` means the side emits nothing at this position at all -- printed explicitly,
//     never as an empty string (an empty LINE and an ABSENT line are different findings).
// PT: `tag` é o marcador de lado ("rml"/"uix"), `n` o número de linha 1-based no dump DAQUELE
//     lado. Um `line` nulo significa que o lado não emite nada nesta posição -- impresso
//     explicitamente, nunca como string vazia (uma linha VAZIA e uma linha AUSENTE são achados
//     diferentes).
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

// EN: One differing line pair. `rml_line`/`uix_line` hold the FULL line text (never truncated --
//     a truncated record could not be compared for equality against a future run). `present_*`
//     distinguishes "this side emitted an empty line" from "this side has no line here at all".
// PT: Um par de linhas divergentes. `rml_line`/`uix_line` guardam o texto COMPLETO da linha (nunca
//     truncado -- um registro truncado não poderia ser comparado por igualdade contra uma execução
//     futura). `present_*` distingue "este lado emitiu uma linha vazia" de "este lado não tem
//     linha nenhuma aqui".
struct DiffEntry {
  std::size_t rml_lineno = 0; // 1-based; meaningful only when present_rml
  std::size_t uix_lineno = 0; // 1-based; meaningful only when present_uix
  bool present_rml = false;
  bool present_uix = false;
  std::string rml_line;
  std::string uix_line;
};

// EN: Positional diff with common-prefix/common-suffix trimming -- deliberately NOT an LCS/Myers
//     diff. Rationale, and its cost, stated up front rather than discovered later: uix-dom.md
//     section 1 already documents that inserting or removing one node RENUMBERS every following
//     sibling's whole subtree path, so a single structural divergence legitimately changes a large
//     contiguous block of lines. A minimal-edit-script diff would present that as many small,
//     scattered hunks and invite the reader to treat each as an independent finding; the
//     prefix/suffix form presents it as what it is -- ONE region where the two trees stop
//     agreeing. For the equal-length case (every content-only divergence) this degenerates to the
//     exact line-by-line pairing a reader expects.
// PT: Diff posicional com aparo de prefixo/sufixo comum -- deliberadamente NÃO um diff LCS/Myers.
//     Racional, e o custo dele, declarados de saída em vez de descobertos depois: a seção 1 do
//     uix-dom.md já documenta que inserir ou remover um nó RENUMERA o caminho da subárvore inteira
//     de todo irmão seguinte, então uma única divergência estrutural legitimamente muda um bloco
//     contíguo grande de linhas. Um diff de script-de-edição-mínimo apresentaria isso como muitos
//     hunks pequenos e espalhados e convidaria o leitor a tratar cada um como achado independente;
//     a forma prefixo/sufixo apresenta o que aquilo é -- UMA região onde as duas árvores param de
//     concordar. No caso de mesmo comprimento (toda divergência só-de-conteúdo) isto degenera no
//     pareamento linha-a-linha exato que o leitor espera.
std::vector<DiffEntry> diff_lines(const std::vector<std::string>& a,
                                  const std::vector<std::string>& b) {
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
  // EN: `prefix` is bounded by the SHORTER side, so `shorter - prefix` never underflows and is
  //     exactly how far back a common suffix may reach before it would overlap the common prefix.
  // PT: `prefix` é limitado pelo lado MAIS CURTO, então `shorter - prefix` nunca faz underflow e é
  //     exatamente até onde um sufixo comum pode ir antes de encostar no prefixo comum.
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

// EN: A divergence that has been MEASURED, NAMED in docs/uix-dom.md section 9's ledger, and is
//     therefore expected to reproduce exactly. Matching is on the fixture plus the full text of
//     BOTH sides' lines -- not on a substring, not on a line number (line numbers move whenever a
//     fixture is re-captured; the emitted text does not).
//
//     Pinning is BIDIRECTIONAL by construction (see the reconciliation in main()): an observed
//     divergence with no matching row is a NEW finding and fails; a row that the run does not
//     reproduce is a VANISHED known divergence and also fails. The second half is the one that is
//     easy to forget and the one that matters most -- a known divergence that quietly disappears
//     means somebody changed a dumper, possibly to make the oracle agree with itself, which is
//     exactly the fraud this file's header comment forbids; the suite must say so out loud instead
//     of turning greener.
// PT: Uma divergência que foi MEDIDA, NOMEADA no ledger da seção 9 do docs/uix-dom.md, e portanto
//     é esperada reproduzir exatamente. O casamento é sobre a fixture mais o texto completo das
//     linhas dos DOIS lados -- não sobre um substring, não sobre um número de linha (números de
//     linha se movem sempre que uma fixture é recapturada; o texto emitido não).
//
//     O pinning é BIDIRECIONAL por construção (ver a reconciliação no main()): uma divergência
//     observada sem linha correspondente é um achado NOVO e reprova; uma linha que a execução não
//     reproduz é uma divergência conhecida SUMIDA e também reprova. A segunda metade é a que é
//     fácil esquecer e a que mais importa -- uma divergência conhecida que some em silêncio
//     significa que alguém mexeu num dumper, possivelmente pra fazer o oráculo concordar consigo
//     mesmo, que é exatamente a fraude que o comentário de cabeçalho deste arquivo proíbe; a suíte
//     tem que dizer isso em voz alta em vez de ficar mais verde.
//     WHY THIS TABLE PINS A MECHANISM AND NOT TWO VERBATIM LINE TEXTS: the one divergence this
//     corpus actually produces is a `HEAD PRESENT` line whose payload is the fixture's ENTIRE
//     `<head>` block -- tens of thousands of escaped bytes on some fixtures. Embedding both sides
//     verbatim, 15 times, would put ~500 KB of unreadable string literals in this file, and a
//     reader could not tell WHICH bytes carry the finding. So each row instead records the three
//     facts that fully determine the divergence, all of them exact, none of them a loosened
//     comparison:
//       - `extra_prefix_len`: how many ESCAPED bytes the rml side emits BEFORE the uix side's
//         payload begins;
//       - `extra_prefix_head`: the first bytes of that extra prefix, verbatim -- so a
//         same-length-but-different prefix does NOT silently match;
//       - the structural invariant, checked in code and not storable in a table at all: the rml
//         payload must be exactly `extra_prefix + uix_payload` (the rml side starts EARLIER in the
//         source and stops at the SAME `</head`, so the shorter payload is a strict SUFFIX of the
//         longer one). Any divergence that is not of that shape is not this one, and is reported
//         as UNKNOWN.
//     ⚠️ Note what this does NOT relax: the IDENTICAL-vs-DIVERGENT verdict is still a raw
//     byte-for-byte `==` over the two whole dumps. This table only answers the SECOND question,
//     "is this divergence one we already measured and wrote down", after the first has already
//     said the dumps differ.
// PT: POR QUE ESTA TABELA PINA UM MECANISMO E NÃO DOIS TEXTOS DE LINHA VERBATIM: a única
//     divergência que este corpus de fato produz é uma linha `HEAD PRESENT` cujo payload é o bloco
//     `<head>` INTEIRO da fixture -- dezenas de milhares de bytes escapados em algumas delas.
//     Embutir os dois lados verbatim, 15 vezes, poria ~500 KB de literais de string ilegíveis
//     neste arquivo, e um leitor não conseguiria dizer QUAIS bytes carregam o achado. Então cada
//     linha registra os três fatos que determinam a divergência por completo, todos exatos, nenhum
//     deles uma comparação afrouxada:
//       - `extra_prefix_len`: quantos bytes ESCAPADOS o lado rml emite ANTES de o payload do lado
//         uix começar;
//       - `extra_prefix_head`: os primeiros bytes desse prefixo extra, verbatim -- pra um prefixo
//         de mesmo comprimento e conteúdo diferente NÃO casar em silêncio;
//       - o invariante estrutural, checado em código e nem armazenável numa tabela: o payload rml
//         tem que ser exatamente `prefixo_extra + payload_uix` (o lado rml começa MAIS CEDO na
//         fonte e para no MESMO `</head`, então o payload mais curto é SUFIXO estrito do mais
//         longo). Qualquer divergência que não tenha essa forma não é esta, e é reportada como
//         UNKNOWN.
//     ⚠️ Note o que isto NÃO afrouxa: o veredito IDENTICAL-vs-DIVERGENT continua sendo um `==`
//     byte a byte cru sobre os dois dumps inteiros. Esta tabela só responde à SEGUNDA pergunta,
//     "esta divergência é uma que já medimos e escrevemos", depois de a primeira já ter dito que
//     os dumps diferem.
struct KnownDivergence {
  const char* fixture;           // file name only, never the absolute path
  std::size_t extra_prefix_len;  // escaped bytes the rml side emits before the uix payload
  const char* extra_prefix_head; // verbatim first bytes of that extra prefix
  const char* ledger_ref;        // where in docs/uix-dom.md section 9 this is written down
};

constexpr const char* kHeadPresent = "HEAD PRESENT ";

// EN: How many bytes of `extra_prefix_head` are compared. Long enough that two different prefixes
//     from this corpus cannot collide (the shortest divergent prefix measured is ~800 bytes),
//     short enough that a table row stays one readable line.
// PT: Quantos bytes de `extra_prefix_head` são comparados. Longo o bastante pra dois prefixos
//     diferentes deste corpus não colidirem (o prefixo divergente mais curto medido tem ~800
//     bytes), curto o bastante pra uma linha da tabela caber numa linha legível.
constexpr std::size_t kPrefixHeadLen = 64;

// EN: KEEP IN SYNC WITH docs/uix-dom.md SECTION 9'S LEDGER TABLE, in BOTH language halves. A row
//     here without a ledger row (or vice versa) is a bookkeeping bug in its own right: this table
//     is what the machine checks, the ledger is what a human reads, and they are two views of one
//     fact.
// PT: MANTER EM SINCRONIA COM A TABELA DO LEDGER DA SEÇÃO 9 DO docs/uix-dom.md, nas DUAS metades
//     de idioma. Uma linha aqui sem linha no ledger (ou vice-versa) é, ela mesma, um bug de
//     escrituração: esta tabela é o que a máquina checa, o ledger é o que um humano lê, e as duas
//     são duas vistas de um fato só.
const std::vector<KnownDivergence>& known_divergences() {
  // EN: `kHeadCommentCollision` -- ONE mechanism, 15 fixtures. `S6a`'s `dom_dump_scan_head()` is
  //     a comment-unaware raw text scan: it takes the FIRST literal `<head` in the source, and in
  //     these 15 fixtures that occurrence is inside the leading `<!-- ... -->` provenance comment
  //     (the prose "its own `<head>`-opacity raw-byte-scan..."), not the real `<head>` element.
  //     `S6b` skips comments and starts at the real element. Both stop at the same `</head`, hence
  //     the strict-suffix shape. The 16th fixture, gusworld_battle_cockpit.rml, has no `<head` in
  //     its comment and is byte-IDENTICAL -- which is the control that proves this diagnosis
  //     rather than merely asserting it. Full write-up: docs/uix-dom.md section 9's ledger.
  // PT: `kHeadCommentCollision` -- UM mecanismo, 15 fixtures. O `dom_dump_scan_head()` da `S6a` é
  //     um scan de texto cru sem consciência de comentário: pega o PRIMEIRO `<head` literal da
  //     fonte, e nestas 15 fixtures essa ocorrência está dentro do comentário de proveniência
  //     `<!-- ... -->` inicial (a prosa "its own `<head>`-opacity raw-byte-scan..."), não o
  //     elemento `<head>` real. A `S6b` pula comentários e começa no elemento real. Os dois param
  //     no mesmo `</head`, daí a forma de sufixo estrito. A 16ª fixture,
  //     gusworld_battle_cockpit.rml, não tem `<head` no comentário e é byte-IDÊNTICA -- que é o
  //     controle que PROVA este diagnóstico em vez de só afirmá-lo. Redação completa: o ledger da
  //     seção 9 do docs/uix-dom.md.
  static constexpr const char* kHeadCommentCollision =
      "docs/uix-dom.md section 9, row 2026-08-05 class (a): S6a's HEAD scan is comment-unaware";
  static const std::vector<KnownDivergence> rows = {
      {"difficulty_menu__lista_hardcore_bloqueado.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"difficulty_menu__splash_confirmacao.rml", 835,
       "-opacity raw-byte-scan never\\n     re-tokenizes <head>'s interio", kHeadCommentCollision},
      {"npc_dialogue__no_com_3_escolhas.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"npc_dialogue__no_linear_fala_curta.rml", 835,
       "-opacity raw-byte-scan never\\n     re-tokenizes <head>'s interio", kHeadCommentCollision},
      {"npc_dialogue__no_linear_fala_longa.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"save_load_menu__modo_carregar_dois_slots_ocupados.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"save_load_menu__modo_salvar_com_autosave.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"save_load_menu__modo_salvar_todos_vazios.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"system_menu__config_audio_sliders.rml", 1082,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"system_menu__config_categorias.rml", 1083,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"system_menu__config_controles_tabela.rml", 1083,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"system_menu__confirmacao_menu_inicial.rml", 1083,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"system_menu__pause_raiz.rml", 1082,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior\\n   ", kHeadCommentCollision},
      {"title_menu__com_save.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
      {"title_menu__sem_save.rml", 829,
       "-opacity raw-byte-scan never re-tokenizes <head>'s interior with", kHeadCommentCollision},
  };
  return rows;
}

// EN: `line` is a full dump line; returns its `HEAD PRESENT ` payload, or nullptr if the line is
//     not a `HEAD PRESENT` record at all.
// PT: `line` é uma linha completa de dump; devolve o payload `HEAD PRESENT ` dela, ou nullptr se a
//     linha não for um registro `HEAD PRESENT` de jeito nenhum.
const char* head_present_payload(const std::string& line) {
  const std::size_t n = std::char_traits<char>::length(kHeadPresent);
  if (line.size() < n || line.compare(0, n, kHeadPresent) != 0) return nullptr;
  return line.c_str() + n;
}

// EN: Does `d` reproduce the divergence `k` describes? See KnownDivergence's own comment for why
//     the check is "structure + measured prefix" and not two verbatim line texts.
// PT: `d` reproduz a divergência que `k` descreve? Ver o comentário do próprio KnownDivergence pro
//     motivo de a checagem ser "estrutura + prefixo medido" e não dois textos de linha verbatim.
bool matches(const KnownDivergence& k, const std::string& fixture, const DiffEntry& d) {
  if (fixture != k.fixture) return false;
  if (!d.present_rml || !d.present_uix) return false;
  if (d.rml_lineno != 1 || d.uix_lineno != 1) return false; // HEAD is always the first record
  const char* rml_payload = head_present_payload(d.rml_line);
  const char* uix_payload = head_present_payload(d.uix_line);
  if (rml_payload == nullptr || uix_payload == nullptr) return false;

  const std::string rml(rml_payload);
  const std::string uix(uix_payload);
  if (rml.size() != uix.size() + k.extra_prefix_len) return false;
  if (rml.compare(k.extra_prefix_len, uix.size(), uix) != 0) return false; // strict-suffix shape
  const std::string prefix = rml.substr(0, k.extra_prefix_len);
  const std::size_t cmp = std::min<std::size_t>(kPrefixHeadLen, prefix.size());
  return prefix.compare(0, cmp, k.extra_prefix_head, cmp) == 0;
}

// EN: A C string literal for `s`, so an UNKNOWN divergence can print a paste-ready table row.
//     Deliberately escapes only what a C literal REQUIRES (`\`, `"`) plus the three control
//     characters the dump format can carry -- everything else, including UTF-8, is passed through,
//     because a row a human cannot read is a row a human will not check.
// PT: Um literal de string C pra `s`, pra uma divergência UNKNOWN conseguir imprimir uma linha de
//     tabela pronta pra colar. Escapa deliberadamente só o que um literal C EXIGE (`\`, `"`) mais
//     os três caracteres de controle que o formato de dump consegue carregar -- todo o resto,
//     UTF-8 incluído, passa direto, porque uma linha que um humano não lê é uma linha que um
//     humano não confere.
std::string c_literal(const std::string& s) {
  std::string out = "\"";
  for (const char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
        break;
    }
  }
  out += "\"";
  return out;
}

// EN: Prints the exact `kKnownDivergences` row that would pin `d`, IF and only if `d` has the
//     strict-suffix HEAD shape a row can express. Printing a suggestion is not the same as
//     accepting it: the row must not be pasted until the divergence has a ledger entry in
//     docs/uix-dom.md section 9 explaining WHAT it is. This exists so that step is mechanical
//     bookkeeping instead of a hand-transcription of a 30 KB line.
// PT: Imprime a linha exata de `kKnownDivergences` que pinaria `d`, SE e somente se `d` tiver a
//     forma HEAD de sufixo-estrito que uma linha consegue expressar. Imprimir uma sugestão não é o
//     mesmo que aceitá-la: a linha não pode ser colada antes de a divergência ter entrada no
//     ledger da seção 9 do docs/uix-dom.md explicando O QUE ela é. Isto existe pra esse passo ser
//     escrituração mecânica em vez de transcrição à mão de uma linha de 30 KB.
void suggest_pin_row(const std::string& fixture, const DiffEntry& d) {
  if (!d.present_rml || !d.present_uix) return;
  const char* rml_payload = head_present_payload(d.rml_line);
  const char* uix_payload = head_present_payload(d.uix_line);
  if (rml_payload == nullptr || uix_payload == nullptr) return;
  const std::string rml(rml_payload);
  const std::string uix(uix_payload);
  if (rml.size() <= uix.size()) return;
  const std::size_t extra = rml.size() - uix.size();
  if (rml.compare(extra, uix.size(), uix) != 0) return;
  const std::string head = rml.substr(0, std::min<std::size_t>(kPrefixHeadLen, extra));
  std::fprintf(stderr,
               "      SUGGESTED PIN (paste only AFTER a docs/uix-dom.md section 9 ledger row "
               "exists):\n          {\"%s\", %zu, %s, \"<ledger ref>\"},\n",
               fixture.c_str(), extra, c_literal(head).c_str());
}

} // namespace

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("dom-dump-differential-oracle", 320, 240)) {
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

  const std::vector<fs::path> fixtures = rml_fixtures_in(fs::path(GLINTFX_RMLX1_CORPUS_DIR));
  if (fixtures.empty()) {
    fail("corpus is non-empty (found 0 .rml fixtures under GLINTFX_RMLX1_CORPUS_DIR)");
  }

  std::size_t compared = 0;
  std::size_t divergent = 0;
  std::size_t parse_errors = 0;
  std::vector<bool> known_hit(known_divergences().size(), false);

  for (const fs::path& path : fixtures) {
    const std::string abspath = path.string();
    const std::string name = path.filename().string();
    const std::string source = read_file(path);

    // EN: side B first -- it is cheap (no GL, no RmlUi) and a parse failure means there is
    //     nothing to compare, so paying for the RmlUi load would be wasted work.
    // PT: lado B primeiro -- é barato (sem GL, sem RmlUi) e uma falha de parse significa que não
    //     há o que comparar, então pagar o load do RmlUi seria trabalho jogado fora.
    glintfx::uix::ParseResult parsed = glintfx::uix::parse_document(source);
    if (parsed.error) {
      ++parse_errors;
      fail(name + ": parse_document() failed at line " + std::to_string(parsed.error->line) +
           ", column " + std::to_string(parsed.error->column) + ": " + parsed.error->message);
      continue;
    }
    const std::string uix_dump = glintfx::uix::dump_document(*parsed.document);

    // --- side A: the real RmlUi tree ------------------------------------------------------
    if (!engine.load(abspath.c_str())) {
      fail(name + ": engine.load() returned false");
      continue;
    }
    // EN: Close()/UnloadDocument() of the PREVIOUS fixture's document defers actual destruction
    //     to the next Context::Update() (same reasoning dom_dump_corpus_sanity.cpp documents) --
    //     drive it before reading GetDocument(0) so THIS fixture's document is unambiguously at
    //     index 0, not a leftover from the previous iteration.
    // PT: Close()/UnloadDocument() do documento da fixture ANTERIOR difere a destruição real pro
    //     próximo Context::Update() (mesmo raciocínio que o dom_dump_corpus_sanity.cpp documenta)
    //     -- dispara antes de ler GetDocument(0) pra que o documento DESTA fixture esteja
    //     inequivocamente no índice 0, não uma sobra da iteração anterior.
    engine.update();
    Rml::ElementDocument* doc = ctx->GetDocument(0);
    if (!doc) {
      fail(name + ": GetDocument(0) returned null after a successful load()");
      continue;
    }
    const std::string rml_dump = glintfx::dom_dump_document(doc, source);

    ++compared;

    // EN: Format-level invariants that must hold on BOTH sides regardless of whether they agree
    //     with each other -- a dump that is empty or unterminated is not "a divergence", it is a
    //     broken dumper, and conflating the two would let a catastrophic failure masquerade as
    //     one more ledger row.
    // PT: Invariantes de nível-formato que têm que valer nos DOIS lados independente de eles
    //     concordarem entre si -- um dump vazio ou sem terminador não é "uma divergência", é um
    //     dumper quebrado, e confundir os dois deixaria uma falha catastrófica se disfarçar de
    //     mais uma linha de ledger.
    if (rml_dump.empty() || rml_dump.back() != '\n') {
      fail(name + ": rml-side dump is empty or lacks the trailing newline (uix-dom.md section 1)");
    }
    if (uix_dump.empty() || uix_dump.back() != '\n') {
      fail(name + ": uix-side dump is empty or lacks the trailing newline (uix-dom.md section 1)");
    }

    if (rml_dump == uix_dump) {
      std::printf("%s: IDENTICAL (%zu bytes)\n", name.c_str(), rml_dump.size());
      continue;
    }

    ++divergent;
    const std::vector<std::string> a = split_lines(rml_dump);
    const std::vector<std::string> b = split_lines(uix_dump);
    const std::vector<DiffEntry> diffs = diff_lines(a, b);
    std::printf(
        "%s: DIVERGENT (rml %zu bytes/%zu lines, uix %zu bytes/%zu lines, %zu differing "
        "line(s))\n",
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
      std::fprintf(stderr, "--- %s: UNKNOWN divergence #%zu\n", name.c_str(), unknown);
      report_line("rml", d.rml_lineno, d.present_rml ? &d.rml_line : nullptr);
      report_line("uix", d.uix_lineno, d.present_uix ? &d.uix_line : nullptr);
      suggest_pin_row(name, d);
    }
    if (unknown > 0) {
      fail(name + ": " + std::to_string(unknown) +
           " divergence(s) not in docs/uix-dom.md section 9's ledger (see the rml/uix line pairs "
           "above; a NEW divergence is a RESULT to be written down, never a dumper to be edited "
           "until the diff goes green)");
    }
  }

  // EN: The other half of the pin -- see KnownDivergence's own comment.
  // PT: A outra metade do pin -- ver o comentário do próprio KnownDivergence.
  for (std::size_t k = 0; k < known_divergences().size(); ++k) {
    if (known_hit[k]) continue;
    fail(std::string("known divergence no longer reproduces (fixture ") +
         known_divergences()[k].fixture + ", ledger: " + known_divergences()[k].ledger_ref +
         ") -- if a dumper legitimately changed, REMOVE the ledger row and this table row in the "
         "same commit; do not leave a pin that no longer measures anything");
  }

  // EN: The scope line, printed unconditionally -- a zero declared is what distinguishes "there
  //     were none" from "nobody looked" (uix-dom.md section 4 makes the same argument for the
  //     HEAD ABSENT record, and tools/check_rml_whitelist.sh for its own N=0 report).
  // PT: A linha de escopo, impressa incondicionalmente -- um zero declarado é o que distingue
  //     "não houve" de "ninguém olhou" (a seção 4 do uix-dom.md faz o mesmo argumento pro registro
  //     HEAD ABSENT, e o tools/check_rml_whitelist.sh pro próprio relatório de N=0).
  std::printf(
      "SCOPE: %zu fixtures compared, %zu divergent, %zu parse errors, %zu known "
      "divergence(s) pinned\n",
      compared, divergent, parse_errors, known_divergences().size());

  // EN: A pinned divergence is an OPEN, unfixed defect that this suite has agreed to keep
  //     measuring -- it must never read as "all good". Printed as a loud, unconditional block
  //     whenever the count is non-zero, so a green run still tells whoever reads the log that the
  //     two engines do NOT fully agree yet and where the write-up is. Exit code stays 0 on
  //     purpose: a test that is red forever gets muted, and a muted oracle measures nothing --
  //     the pin is what keeps the finding alive without buying that outcome.
  // PT: Uma divergência pinada é um defeito ABERTO, não consertado, que esta suíte concordou em
  //     continuar medindo -- nunca pode ser lida como "está tudo bem". Impressa como um bloco
  //     alto e incondicional sempre que a contagem for diferente de zero, pra uma rodada verde
  //     ainda dizer a quem lê o log que os dois motores AINDA NÃO concordam por inteiro e onde
  //     está a redação. O código de saída fica 0 de propósito: um teste vermelho pra sempre acaba
  //     silenciado, e um oráculo silenciado não mede nada -- o pin é o que mantém o achado vivo
  //     sem pagar esse preço.
  if (divergent > 0) {
    std::printf(
        "WARNING: %zu of %zu fixtures do NOT agree byte-for-byte between the two dumpers. "
        "Every divergence listed above is KNOWN, PINNED and WRITTEN DOWN -- none of them "
        "is fixed. Read docs/uix-dom.md section 9's ledger before touching either "
        "dumper.\n",
        divergent, compared);
  }

  if (g_failures == 0) {
    std::puts("dom_dump_differential_oracle OK");
    return 0;
  }
  std::printf("dom_dump_differential_oracle: %d failure(s)\n", g_failures);
  return 10;
}
