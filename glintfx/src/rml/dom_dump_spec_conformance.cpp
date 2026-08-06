// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-CONFORMIDADE-SPEC` -- S6a's half of the THIRD oracle type this repo's own
//     `dom_dump_differential_oracle.cpp` (S7) cannot be: that oracle proves `S6a`/`S6b` AGREE
//     with EACH OTHER, byte for byte; it structurally cannot prove either one agrees with
//     `docs/uix-dom.md` ITSELF, because a shared misreading of an ambiguous sentence produces
//     agreement without conformance -- the exact failure mode `UIX-HEAD-PREFIXO-CEGO` (TODO.md)
//     named. This file tests `dom_dump_scan_head()`/`dom_dump_escape()`/`dom_dump_document()`
//     (glintfx/src/rml/dom_dump.{hpp,cpp}, the `S6a` half -- walks the REAL `Rml::ElementDocument`
//     tree) against the spec's own words, IN ISOLATION -- it never compares against `S6b`'s
//     output (that comparison is dom_dump_differential_oracle.cpp's job, and this file must not
//     duplicate or weaken it). Confined to `glintfx/src/rml/` -- the ONLY place permitted to
//     `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`) -- same reason
//     dom_dump_corpus_sanity.cpp/dom_dump_determinism_sanity.cpp/dom_dump_differential_oracle.cpp
//     already live here rather than in `glintfx/tests/`.
//
//     STRUCTURE, RULE CATALOG, POSITIVE-CONTROL MECHANISM: identical discipline and identical
//     rule IDs to this item's `S6b` sibling
//     (glintfx/tests/uix/spec_conformance_sanity.cpp) -- see THAT file's own header comment for
//     the full rationale (enumerate-before-filtering, one funnel per rule, `--mutate=<id>`
//     positive-control flag). The two files are DELIBERATELY not a shared header/table: `S6a`'s
//     author must not read `S6b`'s source (uix-dom.md's own "why this document exists" section),
//     so even this conformance harness keeps the same two-independent-authors wall the dumpers
//     themselves are built under -- a shared rule-table header would be exactly the kind of
//     shared-context leak that section warns against, just moved one layer up into the test
//     harness instead of the product code.
// PT: `UIX-CONFORMIDADE-SPEC` -- a metade da S6a do TERCEIRO tipo de oráculo que o próprio
//     `dom_dump_differential_oracle.cpp` (S7) deste repo não consegue ser: aquele oráculo prova
//     que `S6a`/`S6b` CONCORDAM entre si, byte a byte; ele estruturalmente não consegue provar que
//     qualquer um dos dois concorda com o PRÓPRIO `docs/uix-dom.md`, porque uma leitura errada
//     COMPARTILHADA de uma frase ambígua produz concordância sem conformidade -- o modo de falha
//     exato que o `UIX-HEAD-PREFIXO-CEGO` (TODO.md) nomeou. Este arquivo testa
//     `dom_dump_scan_head()`/`dom_dump_escape()`/`dom_dump_document()`
//     (glintfx/src/rml/dom_dump.{hpp,cpp}, a metade `S6a` -- percorre a árvore REAL
//     `Rml::ElementDocument`) contra as PRÓPRIAS palavras da spec, ISOLADAMENTE -- nunca compara
//     contra a saída da `S6b` (essa comparação é trabalho do dom_dump_differential_oracle.cpp, e
//     este arquivo não pode duplicar nem enfraquecer ela). Confinado a `glintfx/src/rml/` -- o
//     ÚNICO lugar com permissão de `#include <RmlUi/...>` (`tools/check_rml_whitelist.sh`) --
//     mesmo motivo de dom_dump_corpus_sanity.cpp/dom_dump_determinism_sanity.cpp/
//     dom_dump_differential_oracle.cpp já morarem aqui em vez de em `glintfx/tests/`.
//
//     ESTRUTURA, CATÁLOGO DE REGRAS, MECANISMO DE CONTROLE POSITIVO: mesma disciplina e mesmos
//     IDs de regra do irmão `S6b` deste item (glintfx/tests/uix/spec_conformance_sanity.cpp) --
//     ver o próprio comentário de cabeçalho DAQUELE arquivo pro racional completo. Os dois
//     arquivos são DE PROPÓSITO não uma tabela/header compartilhados: o autor da `S6a` não pode
//     ler o fonte da `S6b` (própria seção "por que este documento existe" do uix-dom.md), então
//     até este harness de conformidade mantém a mesma parede de dois-autores-independentes sob a
//     qual os próprios dumpers são construídos.
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "dom_dump.hpp"
#include "system_clock.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Rule {
  const char* id;
  const char* section;
  const char* what;
  bool testable;
  const char* reason_if_not;
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
     "unescaping (pretty-printer inverse) is optional per spec text; S6a ships no "
     "pretty-printer/unescaper at all -- there is no product to hold to the "
     "exact-inverse requirement",
     false, "opcional, nao implementado por S6a (dom_dump.hpp expoe so escape(), nunca unescape())"},
    {"3.1", "3", "exactly two top-level records, HEAD then body, in that order", true, ""},
    {"3.2", "3", "the wrapping <rml> tag itself never gets a path/line in the dump", true, ""},
    {"3.3", "3", "body is the literal root path, no numeric suffix", true, ""},
    {"3.4", "3",
     "descendant indices are dense 0-based positions among SURVIVING (post-"
     "whitespace-filter) children",
     true, ""},
    {"3.5", "3",
     "a document missing <body> entirely is declared explicitly out of this format's "
     "scope -- section 3 makes no dump-content assertion for that case, only a scope "
     "disclaimer; whether Rml::Context::LoadDocumentFromMemory even PRODUCES an "
     "Rml::ElementDocument for such input is upstream RmlUi's own parser behaviour, "
     "not this dumper's conformance to grade",
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
    {"8.2", "8", "id values, class tokens and attribute values are NEVER case-folded", true, ""},
};

int g_testable = 0;
int g_violations = 0;
int g_not_testable = 0;

void check_rule(const char* id, const char* section, bool cond, const std::string& what) {
  ++g_testable;
  if (!cond) {
    ++g_violations;
    std::fprintf(stderr, "VIOLATION [S6a] rule %s (docs/uix-dom.md section %s): %s\n", id,
                 section, what.c_str());
  }
}

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

// EN: Shared per-process Rml::Context (own window, own Engine) every rule's runner reaches
//     through this file-scope pointer -- same "one shared fixture for the whole tier" pattern
//     dom_dump_determinism_sanity.cpp's own Tier 3 already uses, constructed once at the top of
//     main() and torn down at the very end.
// PT: Rml::Context compartilhado por processo (janela própria, Engine próprio) que o runner de
//     cada regra alcança através deste ponteiro de escopo-de-arquivo -- mesmo padrão "uma fixture
//     compartilhada pro nível inteiro" que o próprio Tier 3 do dom_dump_determinism_sanity.cpp já
//     usa, construído uma vez no topo do main() e desmontado no finalzinho.
Rml::Context* g_ctx = nullptr;
glintfx::Engine* g_engine = nullptr;

std::string dump_of(const std::string& source) {
  Rml::ElementDocument* doc = g_ctx->LoadDocumentFromMemory(source, "[dom_dump_spec_conformance]");
  if (!doc) return "<<LoadDocumentFromMemory FAILED>>";
  const std::string result = glintfx::dom_dump_document(doc, source);
  doc->Close();
  g_engine->update();
  return result;
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
  check_rule("1.3", "1", !lines.empty() && lines[0].rfind("HEAD ", 0) == 0,
             "first line is not a HEAD record (preamble leaked before it)");
}

// ===========================================================================
// Section 2 -- via dom_dump_escape() directly (pure function, no tree needed) AND via a real
// document so the DUMPER is proven to actually invoke it on every value-bearing field (5.5
// re-checks this at the id/class/attr/text level; this rule stays scoped to the function itself).
// ===========================================================================
void run_2_1() {
  check_rule(
      "2.1", "2",
      glintfx::dom_dump_escape(std::string("a\\nb")) == "a\\\\nb" &&
          glintfx::dom_dump_escape(std::string("\\\n\r\t")) == "\\\\\\n\\r\\t",
      "escape() did not apply the 4-rule table correctly (literal backslash+n, or combined "
      "backslash/real-LF/real-CR/real-TAB, in one left-to-right pass)");
}

void run_2_2() {
  check_rule("2.2", "2",
             glintfx::dom_dump_escape("a b") == "a b" &&
                 glintfx::dom_dump_escape(std::string("Hi\xC2\xA0there")) == "Hi\xC2\xA0there",
             "literal space or a UTF-8 continuation byte (nbsp) was altered/escaped");
}

// ===========================================================================
// Section 3
// ===========================================================================
void run_3_1() {
  const std::string d =
      dump_of("<rml><head><style>x</style></head><body><div></div></body></rml>");
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
  check_rule("3.3", "3", lines.size() >= 2 && lines[1] == "body ELEM body",
             "body root path carries a numeric suffix or is not the literal 'body' path");
}

void run_3_4() {
  const std::string d = dump_of("<rml><body>\n<div>x</div>\n</body></rml>");
  bool has_body_children_1 = d.find("body CHILDREN 1\n") != std::string::npos;
  bool has_body_0 = d.find("body/0 ELEM div\n") != std::string::npos;
  bool no_body_1 = d.find("body/1 ") == std::string::npos;
  check_rule("3.4", "3", has_body_children_1 && has_body_0 && no_body_1,
             "whitespace-only siblings were not filtered before index assignment (sparse or "
             "off-by-one index)");
}

// ===========================================================================
// Section 4 -- the central rule, UIX-HEAD-PREFIXO-CEGO's own claim. Uses dom_dump_scan_head()
// directly (the exact function the original finding's probe exercised) for 4.1-4.3, and
// dump_of() (real tree) for 4.4/4.5 since those concern the FULL dump, not just the raw scan.
// ===========================================================================
void run_4_1() {
  const std::string marker = "MARCA_UNICA_ORQ_42";
  const std::string src = "<rml>\n<head>" + marker +
                          "\n<style>body{color:red}</style>\n</head>\n<body></body>\n</rml>\n";
  const auto r = glintfx::dom_dump_scan_head(src);
  const bool found_in_scan = r.present && r.raw.find(marker) != std::string::npos;
  const std::string d = dump_of(src);
  const bool found_in_dump = d.find(marker) != std::string::npos;
  check_rule("4.1", "4", found_in_scan && found_in_dump,
             "a marker planted IMMEDIATELY after <head>'s own closing '>' did not survive into "
             "the HEAD payload -- exact UIX-HEAD-PREFIXO-CEGO claim, S6a side");
}

void run_4_2() {
  const auto r = glintfx::dom_dump_scan_head("<rml><body>x</body></rml>");
  const std::string d = dump_of("<rml><body></body></rml>");
  const auto lines = split_lines(d);
  check_rule("4.2", "4",
             !r.present && r.raw.empty() && !lines.empty() && lines[0] == "HEAD ABSENT",
             "no <head> in source but HEAD ABSENT is missing/malformed in scan and/or dump");
}

void run_4_3() {
  const auto r1 = glintfx::dom_dump_scan_head(
      "<rml><head><style>x</style></head><body>"
      "</body></rml>");
  bool present_ok = r1.present && r1.raw == "<style>x</style>";
  const auto r2 = glintfx::dom_dump_scan_head("<rml><head/><body>x</body></rml>");
  bool self_closing_ok = r2.present && r2.raw.empty();
  check_rule("4.3", "4", present_ok && self_closing_ok,
             "HEAD PRESENT payload wrong, or self-closing <head/> did not yield an EMPTY "
             "payload");
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
  check_rule("4.5", "4", d.find("&amp;") != std::string::npos,
             "HEAD payload was entity-decoded ('&amp;' turned into a literal '&'), violating "
             "section 4's exemption from section 6's entity-decoding rule");
}

// ===========================================================================
// Section 5
// ===========================================================================
void run_5_1() {
  const std::string d = dump_of("<rml><body><div>A<span>B</span>C</div></body></rml>");
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
  const std::string d = dump_of(
      "<rml><body><div id=\"i\" class=\"c\" data-x=\"1\"><span>t</span></div></body></rml>");
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

  const std::string d_empty = dump_of("<rml><body><div></div></body></rml>");
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
  const std::string d = dump_of("<rml><body><div id=\"a\tb\"></div></body></rml>");
  check_rule("5.5", "5", d.find("ID a\\tb") != std::string::npos,
             "a real TAB byte reaching the ID line was not escaped to the two-character \\t "
             "form by the dumper");
}

void run_5_6() {
  const std::string d = dump_of("<rml><body id=\"root-id\" class=\"root-class\"></body></rml>");
  const auto lines = split_lines(d);
  bool has_elem = lines.size() >= 2 && lines[1] == "body ELEM body";
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
  const std::string d = dump_of("<rml><body><div> \t\n\r </div></body></rml>");
  check_rule("6a", "6", d.find("body/0 CHILDREN 0\n") != std::string::npos,
             "a text node made of only the 4-char whitespace set was NOT filtered out");
}

void run_6b() {
  const std::string d = dump_of("<rml><body><div>  Hello   world  </div></body></rml>");
  check_rule("6b", "6", d.find("TEXT   Hello   world  \n") != std::string::npos,
             "text content was trimmed or had internal whitespace runs collapsed");
}

void run_6c() {
  const std::string d = dump_of(
      "<rml><body>"
      "<a>&lt;</a><b>&gt;</b><c>&amp;</c><d>&quot;</d>"
      "<e>&#160;</e><f>&#xA0;</f>"
      "<g>&nbsp;</g><h>&apos;</h>"
      "</body></rml>");
  bool lt_ok = d.find("TEXT <\n") != std::string::npos;
  bool gt_ok = d.find("TEXT >\n") != std::string::npos;
  bool amp_ok = d.find("TEXT &\n") != std::string::npos;
  bool quot_ok = d.find("TEXT \"\n") != std::string::npos;
  std::size_t nbsp_count = 0, pos = 0;
  while ((pos = d.find("\xC2\xA0", pos)) != std::string::npos) {
    ++nbsp_count;
    pos += 2;
  }
  bool both_numeric_forms_ok = nbsp_count >= 2;
  bool nbsp_undecoded_ok = d.find("TEXT &nbsp;\n") != std::string::npos;
  bool apos_undecoded_ok = d.find("TEXT &apos;\n") != std::string::npos;
  check_rule("6c", "6",
             lt_ok && gt_ok && amp_ok && quot_ok && both_numeric_forms_ok && nbsp_undecoded_ok &&
                 apos_undecoded_ok,
             "entity-decoding set is wrong: either a real form did not decode, a numeric form "
             "(decimal or hex) was missed, or &nbsp;/&apos; were decoded when they must survive "
             "as literal source bytes");
}

// ===========================================================================
// Section 7
// ===========================================================================
void run_7_1() {
  const std::string d = dump_of("<rml><body><div class=\"a\tb\"></div></body></rml>");
  check_rule("7.1", "7", d.find("CLASS a\\tb\n") != std::string::npos,
             "class split treated an embedded TAB as a delimiter (should split on literal "
             "space ONLY, not the 4-char whitespace set)");
}

void run_7_2() {
  const std::string d = dump_of("<rml><body><div class=\"&#9;b a&#9;a \"></div></body></rml>");
  check_rule("7.2", "7", d.find("CLASS a\\ta b\n") != std::string::npos,
             "leading/trailing tab-at-segment-edge was not trimmed, or the embedded tab was "
             "not preserved -- section 7's own \"\\tb\"/\"a\\ta\" worked example");
}

void run_7_3() {
  const std::string d = dump_of("<rml><body><div class=\"z z a m a\"></div></body></rml>");
  check_rule("7.3", "7", d.find("CLASS a m z\n") != std::string::npos,
             "class tokens were not both deduplicated AND sorted ascending byte-wise");
}

void run_7_4() {
  const std::string d = dump_of("<rml><body><div class=\" a  b \"></div></body></rml>");
  check_rule("7.4", "7", d.find("CLASS a b\n") != std::string::npos,
             "a repeated/leading/trailing space delimiter produced an empty class token "
             "instead of being dropped (docs/uix-dom.md section 9 ledger row, 2026-08-05)");
}

void run_7_5() {
  const std::string d = dump_of(
      "<rml><body><div id=\"i\" class=\"c\" zzz=\"3\" aaa=\"1\" mmm=\"2\"></div></body></rml>");
  bool order_ok = d.find("body/0 ATTR aaa=1\nbody/0 ATTR mmm=2\nbody/0 ATTR zzz=3\n") !=
                  std::string::npos;
  bool no_id_class_leak =
      d.find("ATTR id=") == std::string::npos && d.find("ATTR class=") == std::string::npos;
  check_rule("7.5", "7", order_ok && no_id_class_leak,
             "generic ATTR lines not sorted by name byte-wise, or id/class leaked into the "
             "generic ATTR block");
}

void run_7_6() {
  const std::string d = dump_of("<rml><body><div class=\"a Z\"></div></body></rml>");
  check_rule("7.6", "7", d.find("CLASS Z a\n") != std::string::npos,
             "class sort is not strict byte-wise ASCII order (uppercase must sort strictly "
             "before lowercase, not locale-interleaved)");
}

void run_7_7() {
  const std::string d1 = dump_of("<rml><body><div id=\"\"></div></body></rml>");
  bool empty_id_no_line = d1.find("body/0 ID") == std::string::npos;
  const std::string d2 = dump_of("<rml><body><div></div></body></rml>");
  bool absent_id_no_line = d2.find("body/0 ID") == std::string::npos;
  const std::string d3 = dump_of("<rml><body><div data-if=\"\"></div></body></rml>");
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
  const std::string d = dump_of(
      "<rml><body><div id=\"MixedCase\" class=\"MixedClass\" data-foo=\"MixedValue\">"
      "</div></body></rml>");
  bool id_ok = d.find("ID MixedCase\n") != std::string::npos;
  bool class_ok = d.find("CLASS MixedClass\n") != std::string::npos;
  bool value_ok = d.find("data-foo=MixedValue\n") != std::string::npos;
  check_rule("8.2", "8", id_ok && class_ok && value_ok,
             "id value, class token, or attribute value case was folded (must be preserved "
             "verbatim)");
}

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
  // EN: `static` storage duration, not automatic -- `host`/`engine` are reached for the whole
  //     of main() through the file-scope `g_ctx`/`g_engine` pointers above (every run_*()
  //     function calls dump_of(), which dereferences them); a plain stack-local here is a
  //     genuine cppcheck `danglingLifetime` trigger (address-of-local escaping into a
  //     namespace-scope global) even though it is not a real bug in THIS program's control flow
  //     (main() never returns before every run_*() call is done using the pointers) -- `static`
  //     makes the underlying claim ("these two objects live for the whole run") the object's own
  //     actual storage class instead of something only true by construction, which is both more
  //     honest and removes the false positive at the root instead of suppressing the checker.
  // PT: duração de armazenamento `static`, não automática -- `host`/`engine` são alcançados
  //     durante o main() inteiro através dos ponteiros de escopo-de-arquivo `g_ctx`/`g_engine`
  //     acima (toda função run_*() chama dump_of(), que os desreferencia); um local de pilha
  //     comum aqui é um gatilho genuíno do `danglingLifetime` do cppcheck (endereço-de-local
  //     escapando pra um global de escopo-de-namespace) mesmo não sendo um bug real no fluxo de
  //     controle DESTE programa (main() nunca retorna antes de toda chamada run_*() terminar de
  //     usar os ponteiros) -- `static` torna a alegação de fundo ("estes dois objetos vivem pela
  //     execução inteira") a própria classe de armazenamento real do objeto em vez de algo
  //     verdadeiro só por construção, o que é ao mesmo tempo mais honesto e remove o falso
  //     positivo na raiz em vez de suprimir o checker.
  static glintfx::WindowGlfw host;
  if (!host.create("dom-dump-spec-conformance", 320, 240)) {
    std::puts("FAIL: host window create");
    return 1;
  }
  glintfx::SystemClock clock;
  static glintfx::Engine engine;
  if (!engine.attach(&clock, 320, 240)) {
    std::puts("FAIL: engine attach");
    return 2;
  }
  Rml::Context* ctx = engine.context();
  if (!ctx) {
    std::puts("FAIL: null context after successful attach");
    return 3;
  }
  g_ctx = ctx;
  g_engine = &engine;

  // EN: --mutate=<id> positive-control mode -- see the S6b sibling file's own main() comment for
  //     the full mechanism; identical here.
  // PT: modo de controle positivo --mutate=<id> -- ver o próprio comentário do main() do arquivo
  //     irmão S6b pro mecanismo completo; idêntico aqui.
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
    std::printf("NOT TESTABLE [S6a] rule %s (section %s): %s -- %s\n", r.id, r.section, r.what,
                r.reason_if_not);
  }

  for (const auto& [rule_id, fn] : kRunners) {
    fn();
  }

  std::printf(
      "SCOPE [S6a spec conformance]: %d regra(s) verificada(s), %d violacao(oes), "
      "%d nao-testavel(is), catalogo total %zu\n",
      g_testable, g_violations, g_not_testable, kRules.size());

  if (static_cast<std::size_t>(g_testable + g_not_testable) != kRules.size()) {
    std::fprintf(stderr,
                 "INVARIANT VIOLATION: testable(%d) + not-testable(%d) != catalog size(%zu)\n",
                 g_testable, g_not_testable, kRules.size());
    return 1;
  }

  if (g_violations > 0) {
    std::fprintf(stderr, "dom_dump_spec_conformance [S6a]: %d violation(s), see above\n",
                 g_violations);
    return 1;
  }
  std::puts("dom_dump_spec_conformance [S6a]: PASS");
  return 0;
}
