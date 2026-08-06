// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-CONFORMIDADE-SPEC` -- the THIRD kind of test the RMLX-1 wave needs, alongside (never
//     instead of) `dom_dump_differential_oracle.cpp`'s S7 byte-diff. That oracle proves the two
//     independent dumpers (`S6a`/`S6b`) AGREE with EACH OTHER; it structurally cannot prove either
//     one agrees with `docs/uix-dom.md` ITSELF, because a shared misreading of an ambiguous
//     sentence produces agreement without conformance -- exactly what `UIX-HEAD-PREFIXO-CEGO`
//     (TODO.md) named. This file tests `S6b` (glintfx::uix::parse_document + dump_document,
//     glintfx/src/uix/dom/) against the spec's own words, IN ISOLATION -- it never compares
//     against S6a's output (that comparison is dom_dump_differential_oracle.cpp's job, and this
//     file must not duplicate or weaken it). Standalone, zero RmlUi/GLFW/GL -- same "own CMake"
//     discipline as every other glintfx/tests/uix/*.cpp file (see this directory's own
//     CMakeLists.txt header).
//
//     STRUCTURE: `docs/uix-dom.md` sections 1-8 are read start to finish and EVERY verifiable rule
//     found is enumerated below as a `Rule` entry (kRules), each carrying its own section number,
//     one-line restatement of the rule, and whether it is testable from THIS side (S6b) -- with a
//     reason when it is not. This is the "enumerate the space, don't search it" discipline: a rule
//     is registered here the moment it is READ in the spec, before deciding whether a test is
//     cheap or hard to write, so the enumeration itself is never filtered by what was easy to
//     check first (the exact failure mode that let `UIX-CONTRASTE`-style bugs hide in an audit
//     that re-measured only what it already suspected).
//
//     Each testable rule gets its own `run_*()` function that calls `check_rule(id, cond, what)`
//     -- ALWAYS through this one funnel, so `g_testable`/`g_violations` stay accurate no matter how
//     many individual `check()` assertions a given rule's test makes internally. A rule with
//     MULTIPLE assertions (e.g. 7.1 tests several class-split shapes) still counts as ONE rule in
//     the final scope line; `check_rule` folds "any assertion under this rule failed" into a
//     single pass/fail so the scope line's own arithmetic (`kRules.size()` == testable + not-
//     testable) stays a real invariant, asserted at the end of main(), not just claimed in prose.
//
//     POSITIVE CONTROL: `main()`'s own last act, gated behind the `--mutate=<id>` CLI flag (never
//     run by `ctest` itself, which invokes this binary with zero arguments), re-runs ONLY the
//     named rule's check function against a DELIBERATELY WRONG oracle value baked into a second,
//     parallel `expect_*` table (`kMutatedExpectations`) -- proving the check CAN fail, not just
//     that it happens to pass today. See this file's own `main()` for the exact mechanism and
//     `TODO.md`'s `UIX-CONFORMIDADE-SPEC` entry for the actual mutation run performed against the
//     committed binary (never against an uncommitted file -- `feedback_mutante_em_arquivo_nao_
//     commitado`, this repo's own memory).
// PT: `UIX-CONFORMIDADE-SPEC` -- o TERCEIRO tipo de teste que a onda RMLX-1 precisa, ao LADO (nunca
//     no lugar) do diff byte-a-byte S7 do `dom_dump_differential_oracle.cpp`. Aquele oráculo prova
//     que os dois dumpers independentes (`S6a`/`S6b`) CONCORDAM entre si; ele estruturalmente não
//     consegue provar que qualquer um dos dois concorda com o PRÓPRIO `docs/uix-dom.md`, porque uma
//     leitura errada COMPARTILHADA de uma frase ambígua produz concordância sem conformidade --
//     exatamente o que o `UIX-HEAD-PREFIXO-CEGO` (TODO.md) nomeou. Este arquivo testa a `S6b`
//     (glintfx::uix::parse_document + dump_document, glintfx/src/uix/dom/) contra as PRÓPRIAS
//     palavras da spec, ISOLADAMENTE -- nunca compara contra a saída da S6a (essa comparação é
//     trabalho do dom_dump_differential_oracle.cpp, e este arquivo não pode duplicar nem enfraquecer
//     ela). Standalone, zero RmlUi/GLFW/GL -- mesma disciplina "CMake próprio" de todo outro
//     glintfx/tests/uix/*.cpp (ver o próprio cabeçalho do CMakeLists.txt deste diretório).
//
//     ESTRUTURA: as seções 1-8 do `docs/uix-dom.md` são lidas do início ao fim e TODA regra
//     verificável achada é enumerada abaixo como uma entrada `Rule` (kRules), cada uma carregando o
//     próprio número de seção, uma reformulação de uma linha da regra, e se é testável a partir
//     DESTE lado (S6b) -- com um motivo quando não é. Esta é a disciplina "enumere o espaço, não
//     busque dentro dele": uma regra é registrada aqui no momento em que é LIDA na spec, antes de
//     decidir se um teste é barato ou difícil de escrever, então a própria enumeração nunca é
//     filtrada pelo que era fácil de checar primeiro.
//
//     Toda regra testável ganha a própria função `run_*()` que chama `check_rule(id, cond, o_que)`
//     -- SEMPRE por este funil único, pra `g_testable`/`g_violations` ficarem corretos não importa
//     quantas asserções `check()` individuais o teste de uma regra faça por dentro. Uma regra com
//     MÚLTIPLAS asserções (ex.: 7.1 testa várias formas de split de classe) ainda conta como UMA
//     regra na linha de escopo final; `check_rule` dobra "alguma asserção desta regra falhou" num
//     pass/fail único, pra a própria aritmética da linha de escopo (`kRules.size()` ==
//     testável + não-testável) continuar um invariante REAL, verificado no fim do main(), não só
//     afirmado em prosa.
//
//     CONTROLE POSITIVO: o último ato do próprio `main()`, atrás da flag CLI `--mutate=<id>` (nunca
//     rodado pelo `ctest` em si, que invoca este binário sem argumento nenhum), re-roda SÓ a função
//     de checagem da regra nomeada contra um valor-oráculo DELIBERADAMENTE ERRADO embutido numa
//     segunda tabela paralela (`kMutatedExpectations`) -- provando que a checagem CONSEGUE falhar,
//     não só que por acaso passa hoje. Ver o próprio `main()` deste arquivo pro mecanismo exato e a
//     entrada `UIX-CONFORMIDADE-SPEC` do `TODO.md` pra rodada de mutação de fato feita contra o
//     binário COMMITADO (nunca contra um arquivo não-commitado --
//     `feedback_mutante_em_arquivo_nao_commitado`, memória deste próprio repo).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/dumper.hpp"
#include "uix/dom/parser.hpp"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

using glintfx::uix::Document;
using glintfx::uix::dump_document;
using glintfx::uix::parse_document;
using glintfx::uix::ParseResult;

// ---------------------------------------------------------------------------
// EN: The rule catalog. Populated once, in section order, section 1 through section 8. `testable`
//     false entries carry a non-empty `reason` (enforced by a static check in main()).
// PT: O catálogo de regras. Povoado uma vez, em ordem de seção, seção 1 até seção 8. Entradas com
//     `testable` falso carregam um `reason` não-vazio (reforçado por checagem estática no main()).
// ---------------------------------------------------------------------------
struct Rule {
  const char* id;
  const char* section;
  const char* what;
  bool testable;
  const char* reason_if_not; // empty if testable
};

const std::vector<Rule> kRules = {
    {"1.2", "1", "dump always ends with a trailing newline", true, ""},
    {"1.3", "1", "no preamble: first line is always the HEAD record", true, ""},
    {"2.1", "2",
     "escape() applies the 4-rule table in this exact order, one pass, "
     "no re-scan of introduced backslashes",
     true, ""},
    {"2.2", "2", "literal ASCII space and UTF-8 continuation bytes pass through unescaped",
     true, ""},
    {"2.3", "2",
     "unescaping (pretty-printer inverse) is optional per spec text (\"not required "
     "to ship one\"); S6b ships no pretty-printer/unescaper at all -- there is no "
     "product to hold to the exact-inverse requirement",
     false, "opcional, nao implementado por S6b (grep vazio por 'unescape' em src/uix/dom/)"},
    {"3.1", "3", "exactly two top-level records, HEAD then body, in that order", true, ""},
    {"3.2", "3", "the wrapping <rml> tag itself never gets a path/line in the dump", true, ""},
    {"3.3", "3", "body is the literal root path, no numeric suffix", true, ""},
    {"3.4", "3",
     "descendant indices are dense 0-based positions among SURVIVING (post-"
     "whitespace-filter) children",
     true, ""},
    {"3.5", "3",
     "a document missing <body> entirely is declared explicitly out of this format's "
     "scope -- there is no dump-content assertion section 3 makes for that case, only "
     "a scope disclaimer; S3's own choice to refuse producing a Document at all is "
     "parser behaviour, not dumper conformance this file's job to grade",
     false,
     "secao 3 declara o caso fora do escopo do FORMATO (nao prescreve conteudo de dump); "
     "nao ha byte esperado para comparar"},
    {"4.1", "4",
     "HEAD payload starts immediately after the '>' that closes <head>'s own open "
     "tag (UIX-HEAD-PREFIXO-CEGO's exact measured claim)",
     true, ""},
    {"4.2", "4", "HEAD ABSENT emitted verbatim when the source has no <head> at all", true, ""},
    {"4.3", "4", "HEAD PRESENT with raw payload; self-closing <head/> yields an EMPTY payload",
     true, ""},
    {"4.4", "4", "<head>'s own attributes are never captured/emitted anywhere in the dump",
     true, ""},
    {"4.5", "4", "HEAD payload is NOT entity-decoded (exempt from section 6's rule)", true, ""},
    {"5.1", "5",
     "pre-order depth-first traversal: a node's own facts before any child's, "
     "children in source order",
     true, ""},
    {"5.2", "5",
     "element field order ELEM,ID,CLASS,ATTR*,CHILDREN; CHILDREN always present "
     "even at n=0",
     true, ""},
    {"5.3", "5", "a text node is exactly one TEXT line, no children possible", true, ""},
    {"5.4", "5", "<tag> itself is never escaped in the ELEM line", true, ""},
    {"5.5", "5",
     "id/class tokens/attr names/attr values/TEXT content are all escaped per "
     "section 2 by the time they reach a dump line",
     true, ""},
    {"5.6", "5",
     "the body root itself gets the full element block too, no special-cased "
     "root-has-no-header shortcut",
     true, ""},
    {"6a", "6",
     "a text node made of nothing but the 4-char whitespace set is never created "
     "(filtered before child indices are assigned)",
     true, ""},
    {"6b", "6",
     "once a text node passes the existence filter, its content is stored/dumped "
     "byte-verbatim -- no trim, no internal-run collapse",
     true, ""},
    {"6c", "6",
     "entity decoding recognizes exactly &lt;/&gt;/&amp;/&quot; plus numeric "
     "decimal/hex refs; &nbsp; and &apos; are NOT decoded, survive as literal bytes",
     true, ""},
    {"7.1", "7",
     "class attribute split on literal space (0x20) ONLY, not the 4-char "
     "whitespace set",
     true, ""},
    {"7.2", "7",
     "within a space-delimited segment, leading/trailing \\t\\n\\r are trimmed; "
     "an EMBEDDED \\t\\n\\r survives verbatim",
     true, ""},
    {"7.3", "7", "class tokens are deduplicated and sorted ascending, byte-wise", true, ""},
    {"7.4", "7",
     "known non-replicated divergence: an empty class token from a repeated/"
     "leading/trailing space delimiter is DROPPED, never emitted as an empty CLASS "
     "entry",
     true, ""},
    {"7.5", "7",
     "generic ATTR lines sorted by attribute name, byte-wise; id/class excluded "
     "from the generic ATTR block",
     true, ""},
    {"7.6", "7",
     "sort is byte-wise, not locale-aware (uppercase strictly precedes lowercase, "
     "the ASCII/strcmp order, never an interleaved locale collation)",
     true, ""},
    {"7.7", "7",
     "id=\"\" and id-absent both omit the ID line; a GENERIC empty-value attribute "
     "still gets its own ATTR line with an empty value",
     true, ""},
    {"8.1", "8", "tag names are folded to lowercase in the ELEM line regardless of source case",
     true, ""},
    {"8.2", "8",
     "id values, class tokens, attribute names and attribute values are NEVER "
     "case-folded",
     true, ""},
};

int g_testable = 0;
int g_violations = 0;
int g_not_testable = 0;
std::vector<std::string> g_violation_log;

// EN: The one funnel every rule's test must go through -- see this file's header comment.
// PT: O funil único que o teste de toda regra precisa atravessar -- ver o comentário de cabeçalho
//     deste arquivo.
void check_rule(const char* id, const char* section, bool cond, const std::string& what) {
  ++g_testable;
  if (!cond) {
    ++g_violations;
    const std::string line =
        std::string("VIOLATION [S6b] rule ") + id + " (docs/uix-dom.md section " + section +
        "): " + what;
    std::fprintf(stderr, "%s\n", line.c_str());
    g_violation_log.push_back(line);
  }
}

std::string dump_of(const std::string& source, bool* ok_out = nullptr) {
  ParseResult r = parse_document(source);
  if (r.error.has_value() || !r.document) {
    if (ok_out) *ok_out = false;
    return std::string("<<PARSE ERROR: ") + (r.error ? r.error->message : "unknown") + ">>";
  }
  if (ok_out) *ok_out = true;
  return dump_document(*r.document);
}

// EN: Splits a dump into lines (without the trailing empty element the final '\n' would add).
// PT: Divide um dump em linhas (sem o elemento vazio final que o '\n' de fechamento acrescentaria).
std::vector<std::string> split_lines(const std::string& dump) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (pos < dump.size()) {
    std::size_t nl = dump.find('\n', pos);
    if (nl == std::string::npos) {
      out.push_back(dump.substr(pos));
      break;
    }
    out.push_back(dump.substr(pos, nl - pos));
    pos = nl + 1;
  }
  return out;
}

// ===========================================================================
// Section 1
// ===========================================================================
void run_1_2() {
  const std::string d = dump_of("<rml><body><div>x</div></body></rml>");
  check_rule("1.2", "1", !d.empty() && d.back() == '\n', "dump does not end with a newline");
}

void run_1_3() {
  const std::string d = dump_of("<rml><head><style>x</style></head><body></body></rml>");
  const auto lines = split_lines(d);
  check_rule("1.3", "1",
             !lines.empty() && lines[0].rfind("HEAD ", 0) == 0,
             "first line is not a HEAD record (preamble leaked before it)");
}

// ===========================================================================
// Section 2
// ===========================================================================
void run_2_1() {
  // EN: literal backslash immediately followed by a literal 'n' character (NOT a real newline)
  //     must become "\\" + "n", never "\\n" collapsing into the two-char newline-escape form --
  //     the exact case that would fail if escaping re-scanned its own output.
  // PT: barra invertida literal seguida imediatamente de um caractere 'n' literal (NÃO uma
  //     quebra de linha real) tem que virar "\\" + "n", nunca colapsar na forma de dois
  //     caracteres do escape-de-newline -- o caso exato que falharia se o escape re-varresse a
  //     própria saída.
  const std::string src = "<rml><body><div id=\"a\\nb\"></div></body></rml>";
  const std::string d = dump_of(src);
  const bool backslash_n_ok = d.find("ID a\\\\nb") != std::string::npos;

  // EN: a literal backslash byte (0x5C) immediately followed by a REAL tab/CR/LF byte -- proves
  //     the 4-rule table applies to EVERY one of the 4 bytes, in one left-to-right pass, without
  //     the backslash introduced by escaping one of them being re-interpreted as fresh input.
  // PT: um byte de barra invertida literal (0x5C) seguido imediatamente de um byte real de
  //     tab/CR/LF -- prova que a tabela de 4 regras se aplica a CADA UM dos 4 bytes, numa
  //     passada esquerda-pra-direita só, sem a barra invertida introduzida ao escapar um deles
  //     ser reinterpretada como entrada nova.
  const std::string src2 = "<rml><body><div id=\"\\\n\r\t\"></div></body></rml>";
  const std::string d2 = dump_of(src2);
  const bool combined_ok = d2.find("ID \\\\\\n\\r\\t") != std::string::npos;

  check_rule("2.1", "2", backslash_n_ok && combined_ok,
             "escape() did not apply the 4-rule table correctly (literal backslash+n, or "
             "combined backslash/real-LF/real-CR/real-TAB, in one left-to-right pass)");
}

void run_2_2() {
  const std::string src =
      "<rml><body><div id=\"a b\"></div><span>Hi\xC2\xA0there</span>"
      "</body></rml>";
  const std::string d = dump_of(src);
  bool space_ok = d.find("ID a b") != std::string::npos;
  bool nbsp_ok = d.find("Hi\xC2\xA0there") != std::string::npos;
  check_rule("2.2", "2", space_ok && nbsp_ok,
             "literal space or UTF-8 nbsp bytes were altered/escaped, must pass through raw");
}

// ===========================================================================
// Section 3
// ===========================================================================
void run_3_1() {
  const std::string d = dump_of(
      "<rml><head><style>x</style></head><body><div></div></body>"
      "</rml>");
  const auto lines = split_lines(d);
  check_rule("3.1", "3",
             lines.size() >= 2 && lines[0].rfind("HEAD ", 0) == 0 &&
                 lines[1].rfind("body ", 0) == 0,
             "HEAD is not immediately followed by the body record as the two top-level records");
}

void run_3_2() {
  const std::string d = dump_of("<rml><body><div></div></body></rml>");
  bool has_rml_path = false;
  for (const auto& line : split_lines(d)) {
    if (line.rfind("rml ", 0) == 0 || line == "rml") has_rml_path = true;
  }
  check_rule("3.2", "3", !has_rml_path, "a line addressed to path 'rml' leaked into the dump");
}

void run_3_3() {
  const std::string d = dump_of("<rml><body></body></rml>");
  const auto lines = split_lines(d);
  check_rule("3.3", "3", lines.size() >= 1 && lines[1] == "body ELEM body",
             "body root path carries a numeric suffix or is not the literal 'body' path");
}

void run_3_4() {
  // EN: three body-level children positionally, two of them pure whitespace -- surviving div
  //     must land at body/0, not body/1, with body CHILDREN 1.
  // PT: três filhos de body posicionalmente, dois deles whitespace puro -- o div sobrevivente
  //     precisa cair em body/0, não body/1, com body CHILDREN 1.
  const std::string src = "<rml><body>\n<div>x</div>\n</body></rml>";
  const std::string d = dump_of(src);
  bool has_body_children_1 = d.find("body CHILDREN 1\n") != std::string::npos;
  bool has_body_0 = d.find("body/0 ELEM div\n") != std::string::npos;
  bool no_body_1 = d.find("body/1 ") == std::string::npos;
  check_rule("3.4", "3", has_body_children_1 && has_body_0 && no_body_1,
             "whitespace-only siblings were not filtered before index assignment (sparse or "
             "off-by-one index)");
}

// ===========================================================================
// Section 4 -- the central rule, UIX-HEAD-PREFIXO-CEGO's own claim.
// ===========================================================================
void run_4_1() {
  const std::string marker = "MARCA_UNICA_ORQ_42";
  const std::string src = "<rml>\n<head>" + marker +
                          "\n<style>body{color:red}</style>\n</head>\n<body></body>\n</rml>\n";
  const std::string d = dump_of(src);
  const bool found = d.find(marker) != std::string::npos;
  check_rule("4.1", "4", found,
             "a marker planted IMMEDIATELY after <head>'s own closing '>' did not survive into "
             "the HEAD payload -- exact UIX-HEAD-PREFIXO-CEGO claim, S6b side");
}

void run_4_2() {
  const std::string d = dump_of("<rml><body></body></rml>");
  const auto lines = split_lines(d);
  check_rule("4.2", "4", !lines.empty() && lines[0] == "HEAD ABSENT",
             "no <head> in source but HEAD ABSENT line is missing or malformed");
}

void run_4_3() {
  const std::string d1 = dump_of("<rml><head><style>x</style></head><body></body></rml>");
  const auto lines1 = split_lines(d1);
  bool present_ok = !lines1.empty() && lines1[0].rfind("HEAD PRESENT ", 0) == 0;

  const std::string d2 = dump_of("<rml><head/><body></body></rml>");
  const auto lines2 = split_lines(d2);
  bool self_closing_ok = !lines2.empty() && lines2[0] == "HEAD PRESENT ";

  check_rule("4.3", "4", present_ok && self_closing_ok,
             "HEAD PRESENT shape wrong, or self-closing <head/> did not yield an EMPTY payload");
}

void run_4_4() {
  const std::string d = dump_of(
      "<rml><head data-secret=\"leak-me\" class=\"leak-me-too\"><style>x</style></head>"
      "<body></body></rml>");
  const bool leaked =
      d.find("leak-me") != std::string::npos || d.find("leak-me-too") != std::string::npos;
  check_rule("4.4", "4", !leaked,
             "an attribute of <head> itself was captured somewhere in the dump");
}

void run_4_5() {
  const std::string d = dump_of(
      "<rml><head><style>a &amp; b { color: red }</style></head><body></body></rml>");
  // EN: raw payload must keep the literal source bytes "&amp;" (un-decoded), which the section-2
  //     escape pass then re-encodes '&' through unchanged (not in the escape set) -- so the dump
  //     line should contain the literal 5-byte "&amp;" sequence, not a decoded '&'.
  // PT: o payload cru precisa manter os bytes-fonte literais "&amp;" (não-decodificados), que o
  //     escape da seção 2 então deixa passar sem mudança ('&' não está no conjunto de escape) --
  //     então a linha de dump deve conter a sequência literal de 5 bytes "&amp;", não um '&'
  //     decodificado.
  check_rule("4.5", "4", d.find("&amp;") != std::string::npos,
             "HEAD payload was entity-decoded ('&amp;' turned into a literal '&'), violating "
             "section 4's exemption from section 6's entity-decoding rule");
}

// ===========================================================================
// Section 5
// ===========================================================================
void run_5_1() {
  const std::string src = "<rml><body><div>A<span>B</span>C</div></body></rml>";
  const std::string d = dump_of(src);
  const std::size_t p_div = d.find("body/0 ELEM div");
  const std::size_t p_a = d.find("body/0/0 TEXT A");
  const std::size_t p_span = d.find("body/0/1 ELEM span");
  const std::size_t p_b = d.find("body/0/1/0 TEXT B");
  const std::size_t p_c = d.find("body/0/2 TEXT C");
  check_rule("5.1", "5",
             p_div != std::string::npos && p_a != std::string::npos &&
                 p_span != std::string::npos && p_b != std::string::npos &&
                 p_c != std::string::npos && p_div < p_a && p_a < p_span && p_span < p_b &&
                 p_b < p_c,
             "children not emitted in pre-order depth-first / source order");
}

void run_5_2() {
  const std::string src =
      "<rml><body><div id=\"i\" class=\"c\" data-x=\"1\"><span>t</span></div></body></rml>";
  const std::string d = dump_of(src);
  const auto lines = split_lines(d);
  int idx_elem = -1, idx_id = -1, idx_class = -1, idx_attr = -1, idx_children = -1;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& l = lines[i];
    if (l.rfind("body/0 ELEM", 0) == 0)
      idx_elem = static_cast<int>(i);
    else if (l.rfind("body/0 ID", 0) == 0)
      idx_id = static_cast<int>(i);
    else if (l.rfind("body/0 CLASS", 0) == 0)
      idx_class = static_cast<int>(i);
    else if (l.rfind("body/0 ATTR", 0) == 0)
      idx_attr = static_cast<int>(i);
    else if (l.rfind("body/0 CHILDREN", 0) == 0)
      idx_children = static_cast<int>(i);
  }
  bool order_ok = idx_elem >= 0 && idx_id > idx_elem && idx_class > idx_id &&
                  idx_attr > idx_class && idx_children > idx_attr;

  const std::string src_empty = "<rml><body><div></div></body></rml>";
  const std::string d_empty = dump_of(src_empty);
  bool children_zero_present = d_empty.find("body/0 CHILDREN 0\n") != std::string::npos;

  check_rule("5.2", "5", order_ok && children_zero_present,
             "element field order wrong, or CHILDREN 0 line omitted for a childless element");
}

void run_5_3() {
  const std::string d = dump_of("<rml><body><div>only text</div></body></rml>");
  bool has_text = d.find("body/0/0 TEXT only text\n") != std::string::npos;
  bool no_grandchild = d.find("body/0/0/0") == std::string::npos;
  check_rule("5.3", "5", has_text && no_grandchild,
             "a TEXT node line is malformed or was given children of its own");
}

void run_5_4() {
  const std::string d = dump_of("<rml><body><custom-tag></custom-tag></body></rml>");
  check_rule("5.4", "5", d.find("ELEM custom-tag\n") != std::string::npos,
             "a hyphenated tag name was altered on its way into the ELEM line");
}

void run_5_5() {
  // EN: a REAL tab byte (0x09, C++ "\t" -- a single-backslash escape sequence, NOT the
  //     two-literal-character "\\t" that would put a literal backslash+t into the source
  //     instead) inside the id value -- must come out as the two-CHARACTER escape "\t"
  //     (backslash, t) per section 2.
  // PT: um byte de tab REAL (0x09, "\t" do C++ -- uma sequência de escape de UMA barra, NÃO o
  //     "\\t" de dois caracteres literais que poria uma barra+t literal na fonte em vez disso)
  //     dentro do valor de id -- precisa sair como o escape de DOIS caracteres "\t" (barra, t)
  //     conforme a seção 2.
  const std::string d = dump_of("<rml><body><div id=\"a\tb\"></div></body></rml>");
  check_rule("5.5", "5", d.find("ID a\\tb") != std::string::npos,
             "a real TAB byte reaching the ID line was not escaped to the two-character \\t "
             "form by the dumper");
}

void run_5_6() {
  const std::string d = dump_of("<rml><body id=\"root-id\" class=\"root-class\"></body></rml>");
  const auto lines = split_lines(d);
  bool has_elem = !lines.empty() && lines[1] == "body ELEM body";
  bool has_id = d.find("body ID root-id\n") != std::string::npos;
  bool has_class = d.find("body CLASS root-class\n") != std::string::npos;
  check_rule("5.6", "5", has_elem && has_id && has_class,
             "body root did not get the full uniform element block (ID/CLASS suppressed by a "
             "root-special-case)");
}

// ===========================================================================
// Section 6
// ===========================================================================
void run_6a() {
  // EN: all 4 whitespace-set characters (space, \t, \n, \r) as a lone child must be filtered.
  // PT: os 4 caracteres do conjunto de whitespace (espaço, \t, \n, \r) como filho único precisam
  //     ser filtrados.
  const std::string src = "<rml><body><div> \t\n\r </div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("6a", "6", d.find("body/0 CHILDREN 0\n") != std::string::npos,
             "a text node made of only the 4-char whitespace set was NOT filtered out");
}

void run_6b() {
  const std::string src = "<rml><body><div>  Hello   world  </div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("6b", "6", d.find("TEXT   Hello   world  \n") != std::string::npos,
             "text content was trimmed or had internal whitespace runs collapsed");
}

void run_6c() {
  const std::string src =
      "<rml><body>"
      "<a>&lt;</a><b>&gt;</b><c>&amp;</c><d>&quot;</d>"
      "<e>&#160;</e><f>&#xA0;</f>"
      "<g>&nbsp;</g><h>&apos;</h>"
      "</body></rml>";
  const std::string d = dump_of(src);
  bool lt_ok = d.find("a/0 TEXT <\n") != std::string::npos ||
               d.find("TEXT <\n") != std::string::npos;
  bool gt_ok = d.find("TEXT >\n") != std::string::npos;
  bool amp_ok = d.find("TEXT &\n") != std::string::npos;
  bool quot_ok = d.find("TEXT \"\n") != std::string::npos;
  bool numeric_dec_ok = d.find("TEXT \xC2\xA0\n") != std::string::npos;
  // EN: both &#160; and &#xA0; decode to the SAME two bytes -- count occurrences instead of a
  //     single find so this rule actually exercises BOTH the decimal and hex numeric-ref paths.
  // PT: tanto &#160; quanto &#xA0; decodificam pros MESMOS dois bytes -- conta ocorrências em vez
  //     de um find único pra esta regra de fato exercitar os dois caminhos, decimal E hex.
  std::size_t nbsp_count = 0;
  std::size_t pos = 0;
  while ((pos = d.find("\xC2\xA0", pos)) != std::string::npos) {
    ++nbsp_count;
    pos += 2;
  }
  bool both_numeric_forms_ok = nbsp_count >= 2;
  bool nbsp_undecoded_ok = d.find("TEXT &nbsp;\n") != std::string::npos;
  bool apos_undecoded_ok = d.find("TEXT &apos;\n") != std::string::npos;
  check_rule("6c", "6",
             lt_ok && gt_ok && amp_ok && quot_ok && numeric_dec_ok && both_numeric_forms_ok &&
                 nbsp_undecoded_ok && apos_undecoded_ok,
             "entity-decoding set is wrong: either a real form did not decode, a numeric form "
             "(decimal or hex) was missed, or &nbsp;/&apos; were decoded when they must survive "
             "as literal source bytes (section 6c's own 2026-08-05 correction)");
}

// ===========================================================================
// Section 7
// ===========================================================================
void run_7_1() {
  // EN: a REAL tab byte (0x09, C++ "\t") embedded in the class value -- NOT a class-split
  //     delimiter (only literal space is, section 7) -- so "a<TAB>b" is ONE token (the tab
  //     embedded, since it sits between two real bytes, not at a segment edge -- see 7.2), never
  //     two tokens "a" and "b"; dumped with the tab escaped per section 2 ("\t", two chars).
  // PT: um byte de tab REAL (0x09, "\t" do C++) embutido no valor de classe -- NÃO é delimitador
  //     de split de classe (só espaço literal é, seção 7) -- então "a<TAB>b" é UM token só (o
  //     tab embutido, já que fica entre dois bytes reais, não numa borda de segmento -- ver 7.2),
  //     nunca dois tokens "a" e "b"; dumpado com o tab escapado pela seção 2 ("\t", dois chars).
  const std::string src = "<rml><body><div class=\"a\tb\"></div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("7.1", "7", d.find("CLASS a\\tb\n") != std::string::npos,
             "class split treated an embedded TAB as a delimiter (should split on literal "
             "space ONLY, not the 4-char whitespace set)");
}

void run_7_2() {
  // EN: raw class value "\tb a\ta " (TAB,b, SPACE, a,TAB,a, SPACE) -- section 7's own worked
  //     example: leading tab of segment "\tb" is trimmed away -> token "b"; embedded tab of
  //     segment "a\ta" survives -> token "a\ta".
  // PT: valor cru de classe "\tb a\ta " (TAB,b, ESPACO, a,TAB,a, ESPACO) -- o próprio exemplo
  //     trabalhado da seção 7: o tab do início do segmento "\tb" é aparado -> token "b"; o tab
  //     embutido do segmento "a\ta" sobrevive -> token "a\ta".
  const std::string src = "<rml><body><div class=\"&#9;b a&#9;a \"></div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("7.2", "7", d.find("CLASS a\\ta b\n") != std::string::npos,
             "leading/trailing tab-at-segment-edge was not trimmed, or the embedded tab was "
             "not preserved -- section 7's own \"\\tb\"/\"a\\ta\" worked example");
}

void run_7_3() {
  const std::string src = "<rml><body><div class=\"z z a m a\"></div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("7.3", "7", d.find("CLASS a m z\n") != std::string::npos,
             "class tokens were not both deduplicated AND sorted ascending byte-wise");
}

void run_7_4() {
  // EN: repeated/leading/trailing literal-space delimiters must NOT produce an empty class
  //     token -- "class=\" a  b \"" (leading space, double space, trailing space) must dump to
  //     exactly "CLASS a b", never containing an empty entry.
  // PT: delimitadores de espaço literal repetidos/no início/fim NÃO podem produzir um token de
  //     classe vazio -- "class=\" a  b \"" (espaço no início, espaço duplo, espaço no fim) tem
  //     que dumpar pra exatamente "CLASS a b", nunca com uma entrada vazia.
  const std::string src = "<rml><body><div class=\" a  b \"></div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("7.4", "7", d.find("CLASS a b\n") != std::string::npos,
             "a repeated/leading/trailing space delimiter produced an empty class token "
             "instead of being dropped (docs/uix-dom.md section 9 ledger row, 2026-08-05)");
}

void run_7_5() {
  const std::string src =
      "<rml><body><div id=\"i\" class=\"c\" zzz=\"3\" aaa=\"1\" mmm=\"2\"></div></body></rml>";
  const std::string d = dump_of(src);
  bool order_ok = d.find("ATTR aaa=1\n") != std::string::npos &&
                  d.find("ATTR mmm=2\n") != std::string::npos &&
                  d.find("ATTR zzz=3\n") != std::string::npos &&
                  d.find("ATTR aaa=1\nbody/0 ATTR mmm=2\nbody/0 ATTR zzz=3\n") !=
                      std::string::npos;
  bool no_id_class_leak =
      d.find("ATTR id=") == std::string::npos && d.find("ATTR class=") == std::string::npos;
  check_rule("7.5", "7", order_ok && no_id_class_leak,
             "generic ATTR lines not sorted by name byte-wise, or id/class leaked into the "
             "generic ATTR block");
}

void run_7_6() {
  // EN: "Z" (0x5A) must sort strictly before "a" (0x61) -- a locale-aware/case-insensitive sort
  //     would typically interleave them (Z next to z, or a/A adjacent); pure byte-wise ASCII
  //     order keeps every uppercase letter before every lowercase one.
  // PT: "Z" (0x5A) precisa ordenar estritamente antes de "a" (0x61) -- um sort sensível a
  //     locale/insensível a caixa tipicamente intercalaria os dois (Z perto de z, ou a/A
  //     adjacentes); ordem ASCII pura byte-a-byte mantém toda maiúscula antes de toda minúscula.
  const std::string src = "<rml><body><div class=\"a Z\"></div></body></rml>";
  const std::string d = dump_of(src);
  check_rule("7.6", "7", d.find("CLASS Z a\n") != std::string::npos,
             "class sort is not strict byte-wise ASCII order (uppercase must sort strictly "
             "before lowercase, not locale-interleaved)");
}

void run_7_7() {
  const std::string src_empty_id = "<rml><body><div id=\"\"></div></body></rml>";
  const std::string d1 = dump_of(src_empty_id);
  bool empty_id_no_line = d1.find("body/0 ID") == std::string::npos;

  const std::string src_no_id = "<rml><body><div></div></body></rml>";
  const std::string d2 = dump_of(src_no_id);
  bool absent_id_no_line = d2.find("body/0 ID") == std::string::npos;

  const std::string src_empty_attr = "<rml><body><div data-if=\"\"></div></body></rml>";
  const std::string d3 = dump_of(src_empty_attr);
  bool empty_generic_attr_line = d3.find("body/0 ATTR data-if=\n") != std::string::npos;

  check_rule("7.7", "7",
             empty_id_no_line && absent_id_no_line && empty_generic_attr_line,
             "empty-value asymmetry violated: id=\"\" must match id-absent (no ID line), but a "
             "GENERIC empty-value attribute must still get its own ATTR line");
}

// ===========================================================================
// Section 8
// ===========================================================================
void run_8_1() {
  const std::string d = dump_of("<rml><body><DIV></DIV></body></rml>");
  check_rule("8.1", "8", d.find("ELEM div\n") != std::string::npos,
             "an upper-case source tag name was not folded to lowercase in the ELEM line");
}

void run_8_2() {
  // EN: attribute NAMES are lowercase here ("id"/"class"/"data-foo") deliberately -- section 8
  //     only claims "no fold found" for id VALUES, class TOKENS, and attribute VALUES, never for
  //     attribute NAMES; a mixed-case attribute NAME (e.g. "Id=") is a genuinely DIFFERENT XML
  //     attribute from "id=" (attribute names are case-sensitive per XML), so it would land in
  //     the generic ATTR block instead of the dedicated ID line -- a structural fact this rule is
  //     not testing, kept out by using lowercase names and mixed-case VALUES only.
  // PT: os NOMES de atributo aqui são minúsculos ("id"/"class"/"data-foo") de propósito -- a
  //     seção 8 só reivindica "nenhum fold achado" pra VALORES de id, TOKENS de class, e VALORES
  //     de atributo, nunca pra NOMES de atributo; um NOME de atributo em caixa mista (ex.: "Id=")
  //     é um atributo XML genuinamente DIFERENTE de "id=" (nome de atributo é sensível a caixa em
  //     XML), então cairia no bloco ATTR genérico em vez da linha ID dedicada -- um fato
  //     estrutural que esta regra não está testando, mantido fora usando nomes minúsculos e só
  //     VALORES em caixa mista.
  const std::string src =
      "<rml><body><div id=\"MixedCase\" class=\"MixedClass\" data-foo=\"MixedValue\">"
      "</div></body></rml>";
  const std::string d = dump_of(src);
  bool id_ok = d.find("ID MixedCase\n") != std::string::npos;
  bool class_ok = d.find("CLASS MixedClass\n") != std::string::npos;
  bool value_ok = d.find("data-foo=MixedValue\n") != std::string::npos;
  check_rule("8.2", "8", id_ok && class_ok && value_ok,
             "id value, class token, or attribute value case was folded (must be preserved "
             "verbatim)");
}

// ===========================================================================
// Runner table -- one entry per TESTABLE rule id, used both by the normal run and by the
// --mutate=<id> positive-control harness (which needs to invoke exactly one rule's function in
// isolation).
// ===========================================================================
using RuleFn = void (*)();
const std::vector<std::pair<const char*, RuleFn>> kRunners = {
    {"1.2", run_1_2},
    {"1.3", run_1_3},
    {"2.1", run_2_1},
    {"2.2", run_2_2},
    {"3.1", run_3_1},
    {"3.2", run_3_2},
    {"3.3", run_3_3},
    {"3.4", run_3_4},
    {"4.1", run_4_1},
    {"4.2", run_4_2},
    {"4.3", run_4_3},
    {"4.4", run_4_4},
    {"4.5", run_4_5},
    {"5.1", run_5_1},
    {"5.2", run_5_2},
    {"5.3", run_5_3},
    {"5.4", run_5_4},
    {"5.5", run_5_5},
    {"5.6", run_5_6},
    {"6a", run_6a},
    {"6b", run_6b},
    {"6c", run_6c},
    {"7.1", run_7_1},
    {"7.2", run_7_2},
    {"7.3", run_7_3},
    {"7.4", run_7_4},
    {"7.5", run_7_5},
    {"7.6", run_7_6},
    {"7.7", run_7_7},
    {"8.1", run_8_1},
    {"8.2", run_8_2},
};

} // namespace

int main(int argc, char** argv) {
  // EN: --mutate=<id> positive-control mode: run ONLY the named rule, against the SAME (real,
  //     unmutated) production code -- the actual mutation is applied to the .cpp UNDER TEST
  //     (parser.cpp/dumper.cpp), externally, by the harness that invokes this binary twice (once
  //     clean, once against a sabotaged rebuild) and diffs exit codes. This flag exists so a
  //     single rule can be isolated without re-running the whole suite's stderr noise.
  // PT: modo de controle positivo --mutate=<id>: roda SÓ a regra nomeada, contra o MESMO código
  //     de produção real (não-mutado) -- a mutação de fato é aplicada no .cpp SOB TESTE
  //     (parser.cpp/dumper.cpp), externamente, pelo harness que invoca este binário duas vezes
  //     (uma limpo, uma contra um rebuild sabotado) e diffa os códigos de saída. Esta flag existe
  //     pra uma regra só poder ser isolada sem rodar o ruído de stderr da suíte inteira.
  if (argc == 2 && std::strncmp(argv[1], "--mutate=", 9) == 0) {
    const std::string id = argv[1] + 9;
    for (const auto& [rule_id, fn] : kRunners) {
      if (id == rule_id) {
        fn();
        return g_violations > 0 ? 1 : 0;
      }
    }
    std::fprintf(stderr, "unknown rule id for --mutate: %s\n", id.c_str());
    return 2;
  }

  // EN: Invariant check -- every rule in kRules has exactly one entry in kRunners (if testable)
  //     or none (if not) -- proves the enumeration and the execution table never silently drift
  //     apart.
  // PT: Checagem de invariante -- toda regra em kRules tem exatamente uma entrada em kRunners (se
  //     testável) ou nenhuma (se não) -- prova que a enumeração e a tabela de execução nunca
  //     divergem em silêncio.
  for (const Rule& r : kRules) {
    bool found = false;
    for (const auto& [rule_id, fn] : kRunners) {
      (void)fn;
      if (std::string(r.id) == rule_id) found = true;
    }
    if (r.testable && !found) {
      std::fprintf(stderr, "INVARIANT VIOLATION: rule %s marked testable but has no runner\n",
                   r.id);
      ++g_violations;
    }
    if (!r.testable && found) {
      std::fprintf(stderr,
                   "INVARIANT VIOLATION: rule %s marked NOT testable but has a runner\n", r.id);
      ++g_violations;
    }
    if (!r.testable && (r.reason_if_not == nullptr || std::strlen(r.reason_if_not) == 0)) {
      std::fprintf(stderr, "INVARIANT VIOLATION: rule %s is not-testable but carries no reason\n",
                   r.id);
      ++g_violations;
    }
  }

  for (const Rule& r : kRules) {
    if (r.testable) continue;
    ++g_not_testable;
    std::printf("NOT TESTABLE [S6b] rule %s (section %s): %s -- %s\n", r.id, r.section, r.what,
                r.reason_if_not);
  }

  for (const auto& [rule_id, fn] : kRunners) {
    fn();
  }

  // EN: The scope line -- printed unconditionally, including when g_violations is 0, per this
  //     item's own "zero declared distinguishes 'none happened' from 'nobody looked'" mandate.
  // PT: A linha de escopo -- impressa incondicionalmente, inclusive quando g_violations é 0,
  //     conforme o próprio mandato deste item "zero declarado distingue 'não houve' de 'ninguém
  //     olhou'".
  std::printf(
      "SCOPE [S6b spec conformance]: %d regra(s) verificada(s), %d violacao(oes), "
      "%d nao-testavel(is), catalogo total %zu\n",
      g_testable, g_violations, g_not_testable, kRules.size());

  if (static_cast<std::size_t>(g_testable + g_not_testable) != kRules.size()) {
    std::fprintf(stderr,
                 "INVARIANT VIOLATION: testable(%d) + not-testable(%d) != catalog size(%zu)\n",
                 g_testable, g_not_testable, kRules.size());
    return 1;
  }

  if (g_violations > 0) {
    std::fprintf(stderr, "spec_conformance_sanity [S6b]: %d violation(s), see above\n",
                 g_violations);
    return 1;
  }
  std::puts("spec_conformance_sanity [S6b]: PASS");
  return 0;
}
