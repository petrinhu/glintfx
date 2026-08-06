// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6a -- byte-exact oracle for `dom_dump_escape()`, `dom_dump_scan_head()`, and
//     `dom_dump_document()`, plus the determinism proof the brief explicitly asked for ("teste
//     que prova o determinismo, não só que a saída 'parece certa'"). Unlike
//     dom_dump_corpus_sanity.cpp (same directory, structural smoke test over the real corpus),
//     every check here compares against a HAND-COMPUTED expected string -- most of them derived
//     directly from docs/uix-dom.md itself (F1 below is section 11's own worked example, copied
//     verbatim on both the input and the expected-output side, so this test is also a live proof
//     that this dumper agrees with the spec's own illustration of itself).
//
//     Split into three tiers, cheapest/most-isolated first:
//       (1) dom_dump_escape() -- pure string function, zero RmlUi, exercises the 4-rule table
//           (section 2) plus the two bytes the spec calls out as deliberately NEVER escaped
//           (literal space, and a UTF-8 continuation byte -- the very nbsp scenario section 6c
//           warns a diff viewer conflates with plain space).
//       (2) dom_dump_scan_head() -- pure string function, zero RmlUi, exercises HEAD ABSENT/
//           PRESENT/self-closing and the quote-aware opening-tag scan (a `>` inside a quoted
//           attribute of `<head ...>` itself must not end the tag early) -- see dom_dump.hpp's
//           own doc-comment for why these edge cases are tested here, in isolation, rather than
//           only through a real corpus fixture that may never happen to exercise them.
//       (3) dom_dump_document() against a REAL Rml::ElementDocument -- F1 (the spec's worked
//           example, full end-to-end byte-exact match), F2 (CLASS/ATTR sorting+dedup via real
//           elements, section 7), F3 (verbatim whitespace-preserving TEXT content, section 6b),
//           F4 (a whitespace-only child produces ZERO body children -- confirms section 6a's
//           filter, already applied by real RmlUi, needs no code of THIS dumper's own to hold),
//           F5/F6 (determinism: same document dumped twice is byte-identical; two documents
//           whose markup differs ONLY in attribute/class SOURCE ORDER dump to the exact SAME
//           bytes -- this is the concrete proof that section 7's sort neutralizes whatever
//           iteration order `Rml::Dictionary` (`SmallUnorderedMap`) happens to produce, which is
//           NOT contractually ordered).
// PT: RMLX-1/S6a -- oráculo byte-exato pro `dom_dump_escape()`, `dom_dump_scan_head()` e
//     `dom_dump_document()`, mais a prova de determinismo que o brief pediu explicitamente
//     ("teste que prova o determinismo, não só que a saída 'parece certa'"). Diferente do
//     dom_dump_corpus_sanity.cpp (mesmo diretório, teste de fumaça estrutural sobre o corpus
//     real), toda verificação aqui compara contra uma string esperada CALCULADA À MÃO -- a
//     maioria delas derivada diretamente do próprio docs/uix-dom.md (o F1 abaixo é o próprio
//     exemplo trabalhado da seção 11, copiado ao pé da letra tanto do lado da entrada quanto do
//     lado da saída-esperada, então este teste também é prova viva de que este dumper concorda
//     com a própria ilustração de si mesmo que a spec dá).
//
//     Dividido em três níveis, mais barato/mais isolado primeiro:
//       (1) dom_dump_escape() -- função de string pura, zero RmlUi, exercita a tabela de 4
//           regras (seção 2) mais os dois bytes que a spec chama de propositalmente NUNCA
//           escapados (espaço literal, e um byte de continuação UTF-8 -- o próprio cenário nbsp
//           que a seção 6c avisa que um visualizador de diff confunde com espaço comum).
//       (2) dom_dump_scan_head() -- função de string pura, zero RmlUi, exercita HEAD ABSENT/
//           PRESENT/auto-fechado e o scan consciente-de-aspas da tag de abertura (um `>` dentro
//           de um atributo entre aspas do próprio `<head ...>` não pode fechar a tag cedo demais)
//           -- ver o próprio doc-comment de dom_dump.hpp pro motivo destes casos de borda serem
//           testados aqui, isolados, em vez de só através de uma fixture real de corpus que pode
//           nunca vir a exercitá-los.
//       (3) dom_dump_document() contra um Rml::ElementDocument REAL -- F1 (o exemplo trabalhado
//           da spec, batida byte-exata ponta-a-ponta completa), F2 (ordenação+dedup de
//           CLASS/ATTR via elementos reais, seção 7), F3 (conteúdo TEXT verbatim preservando
//           whitespace, seção 6b), F4 (um filho só-whitespace produz ZERO filhos de body --
//           confirma que o filtro da seção 6a, já aplicado pelo RmlUi real, não precisa de
//           código NENHUM deste dumper pra valer), F5/F6 (determinismo: o MESMO documento
//           dumpado duas vezes é byte-idêntico; dois documentos cujo markup difere SÓ na ORDEM-
//           FONTE de atributo/classe dumpam pros MESMOS bytes exatos -- esta é a prova concreta
//           de que a ordenação da seção 7 neutraliza qualquer ordem de iteração que
//           `Rml::Dictionary` (`SmallUnorderedMap`) por acaso produza, que NÃO é ordenada por
//           contrato).
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "dom_dump.hpp"
#include "system_clock.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

// EN: Prints BOTH sides on mismatch (not just "FAIL: <name>") -- a byte-exact oracle that only
//     says "wrong" without showing what it got is nearly useless to debug; escaped-looking
//     control characters in the expected/actual strings are exactly the reason to see them raw.
// PT: Imprime OS DOIS lados em caso de divergência (não só "FAIL: <nome>") -- um oráculo
//     byte-exato que só diz "errado" sem mostrar o que obteve é quase inútil de debugar;
//     caracteres de controle com cara de escapados nas strings esperada/obtida são exatamente o
//     motivo de vê-los crus.
void check_eq(const std::string& actual, const std::string& expected, const std::string& what) {
  if (actual == expected) return;
  std::fprintf(stderr, "FAIL: %s\n  expected (%zu bytes): %s\n  actual   (%zu bytes): %s\n",
               what.c_str(), expected.size(), expected.c_str(), actual.size(), actual.c_str());
  ++g_failures;
}

} // namespace

int main() {
  // ---------------------------------------------------------------------------
  // EN: Tier 1 -- dom_dump_escape(), pure function, no RmlUi.
  // PT: Nível 1 -- dom_dump_escape(), função pura, sem RmlUi.
  // ---------------------------------------------------------------------------
  check_eq(glintfx::dom_dump_escape(""), "", "escape(empty) is empty");
  check_eq(glintfx::dom_dump_escape("plain"), "plain", "escape(no special bytes) is unchanged");
  check_eq(glintfx::dom_dump_escape("a b"), "a b", "escape() never escapes literal ASCII space (section 2)");
  check_eq(glintfx::dom_dump_escape(std::string("a\\b")), "a\\\\b", "escape(backslash) -> two chars \\\\");
  check_eq(glintfx::dom_dump_escape(std::string("a\nb")), "a\\nb", "escape(LF) -> two chars \\n");
  check_eq(glintfx::dom_dump_escape(std::string("a\rb")), "a\\rb", "escape(CR) -> two chars \\r");
  check_eq(glintfx::dom_dump_escape(std::string("a\tb")), "a\\tb", "escape(TAB) -> two chars \\t");
  check_eq(glintfx::dom_dump_escape(std::string("\\\n\r\t")), "\\\\\\n\\r\\t",
           "escape() applies all 4 rules in one pass, each input byte classified once");
  // EN: U+00A0 (nbsp, UTF-8 C2 A0) passes through BYTE-FOR-BYTE unescaped (section 6c) -- this
  //     is the exact scenario a plain diff/eyeball review cannot distinguish from a literal
  //     space; the test compares raw bytes, which is the whole point.
  // PT: U+00A0 (nbsp, UTF-8 C2 A0) passa BYTE A BYTE sem escape (seção 6c) -- este é o cenário
  //     exato que uma revisão de diff/olho não consegue distinguir de um espaço literal; o
  //     teste compara bytes crus, que é o ponto inteiro.
  check_eq(glintfx::dom_dump_escape(std::string("Hi\xC2\xA0there")), "Hi\xC2\xA0there",
           "escape() passes a UTF-8 continuation byte (nbsp) through unchanged");

  // ---------------------------------------------------------------------------
  // EN: Tier 2 -- dom_dump_scan_head(), pure function, no RmlUi.
  // PT: Nível 2 -- dom_dump_scan_head(), função pura, sem RmlUi.
  // ---------------------------------------------------------------------------
  {
    const auto r = glintfx::dom_dump_scan_head("<rml><body>x</body></rml>");
    check(!r.present, "scan_head: no <head> at all -> HEAD ABSENT");
    check_eq(r.raw, "", "scan_head: HEAD ABSENT carries no raw payload");
  }
  {
    const auto r = glintfx::dom_dump_scan_head("<rml>\n<head>\n<style>body{color:white}</style>\n</head>\n<body></body>\n</rml>\n");
    check(r.present, "scan_head: spec worked-example source -> HEAD PRESENT");
    check_eq(r.raw, "\n<style>body{color:white}</style>\n", "scan_head: exact raw span, section 4");
  }
  {
    const auto r = glintfx::dom_dump_scan_head("<rml><head/><body>x</body></rml>");
    check(r.present, "scan_head: self-closing <head/> -> still HEAD PRESENT");
    check_eq(r.raw, "", "scan_head: self-closing <head/> carries an EMPTY raw payload");
  }
  {
    // EN: A `>` inside a quoted attribute VALUE of <head ...> itself must not end the opening
    //     tag early -- the real closing `>` is the one AFTER the quoted attribute.
    // PT: Um `>` dentro do VALOR de um atributo entre aspas do próprio <head ...> não pode
    //     fechar a tag de abertura cedo demais -- o `>` de fechamento real é o que vem DEPOIS
    //     do atributo entre aspas.
    const auto r = glintfx::dom_dump_scan_head("<rml><head data-weird=\"a>b\">CONTENT</head><body>x</body></rml>");
    check(r.present, "scan_head: quoted '>' inside <head>'s own attribute -> still HEAD PRESENT");
    check_eq(r.raw, "CONTENT", "scan_head: quote-aware opening-tag scan finds the REAL closing '>'");
  }
  {
    const auto r = glintfx::dom_dump_scan_head("<rml><HEAD>x</HEAD><body>y</body></rml>");
    check(r.present,
          "scan_head: <HEAD>/</HEAD> upper-case -- matches case-insensitively (section 4, "
          "mirrors XMLParser::HandleElementStart's own ToLower)");
    check_eq(r.raw, "x", "scan_head: upper-case <HEAD> raw span");
  }

  // ---------------------------------------------------------------------------
  // EN: Tier 3 -- dom_dump_document() against a REAL Rml::ElementDocument. One shared
  //     Engine/Context for the whole tier (same GL-context-fixture pattern as
  //     document_reload_leak.cpp), documents loaded directly via
  //     Rml::Context::LoadDocumentFromMemory (bypassing Bootstrap::load(), which only takes a
  //     file path) -- each closed with Close()+update() before the next, same
  //     defer-to-next-Update() reasoning document_reload_leak.cpp already documents.
  // PT: Nível 3 -- dom_dump_document() contra um Rml::ElementDocument REAL. Um único
  //     Engine/Context compartilhado pro nível inteiro (mesmo padrão de fixture de contexto GL
  //     do document_reload_leak.cpp), documentos carregados diretamente via
  //     Rml::Context::LoadDocumentFromMemory (contornando o Bootstrap::load(), que só aceita
  //     caminho de arquivo) -- cada um fechado com Close()+update() antes do próximo, mesmo
  //     raciocínio de diferir-pro-próximo-Update() que o document_reload_leak.cpp já documenta.
  // ---------------------------------------------------------------------------
  glintfx::WindowGlfw host;
  if (!host.create("dom-dump-determinism", 320, 240)) {
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

  auto load_close = [&](const std::string& rml) -> Rml::ElementDocument* {
    return ctx->LoadDocumentFromMemory(rml, "[dom_dump_determinism_sanity]");
  };
  auto close_doc = [&](Rml::ElementDocument* doc) {
    if (!doc) return;
    doc->Close();
    engine.update();
  };

  // -- F1: docs/uix-dom.md section 11's own worked example, copied verbatim. --------------
  {
    const std::string source =
        "<rml>\n"
        "<head>\n"
        "<style>body{color:white}</style>\n"
        "</head>\n"
        "<body>\n"
        "<div id=\"panel\" class=\"wide highlighted\" data-if=\"flag\" title=\"Panel\">\n"
        // EN: &#160; (numeric charref, NOT the named &nbsp;) -- see docs/uix-dom.md section 6c's
        //     2026-08-05 correction: RmlUi's DecodeRml does not recognize &nbsp; at all (only
        //     &lt;/&gt;/&amp;/&quot; and numeric refs), so &nbsp; would survive undecoded as 6
        //     literal ASCII bytes, not U+00A0 -- this fixture's own F1 run against the ORIGINAL
        //     &nbsp; spelling is what caught that bug live.
        // PT: &#160; (referência numérica, NÃO o &nbsp; nomeado) -- ver a correção de 2026-08-05
        //     da seção 6c do docs/uix-dom.md: o DecodeRml do RmlUi não reconhece &nbsp; de jeito
        //     nenhum (só &lt;/&gt;/&amp;/&quot; e referências numéricas), então &nbsp; sobreviveria
        //     não-decodificado como 6 bytes ASCII literais, não U+00A0 -- a rodada desta própria
        //     fixture F1 contra a grafia ORIGINAL &nbsp; foi o que pegou esse bug ao vivo.
        "  <span class=\"highlighted wide\">Hi&#160;there</span>\n"
        "</div>\n"
        "</body>\n"
        "</rml>\n";
    const std::string expected =
        "HEAD PRESENT \\n<style>body{color:white}</style>\\n\n"
        "body ELEM body\n"
        "body CHILDREN 1\n"
        "body/0 ELEM div\n"
        "body/0 ID panel\n"
        "body/0 CLASS highlighted wide\n"
        "body/0 ATTR data-if=flag\n"
        "body/0 ATTR title=Panel\n"
        "body/0 CHILDREN 1\n"
        "body/0/0 ELEM span\n"
        "body/0/0 CLASS highlighted wide\n"
        "body/0/0 CHILDREN 1\n"
        "body/0/0/0 TEXT Hi\xC2\xA0there\n";
    Rml::ElementDocument* doc = load_close(source);
    check(doc != nullptr, "F1: LoadDocumentFromMemory(worked example) succeeded");
    if (doc) {
      check_eq(glintfx::dom_dump_document(doc, source), expected,
               "F1: dump() matches docs/uix-dom.md section 11's own worked example byte-for-byte");
    }
    close_doc(doc);
  }

  // -- F2: CLASS/ATTR sorting+dedup via a real element (section 7). ------------------------
  //    Raw class value has a TAB (&#9;) and repeated/extra-space tokens -- proves this dumper
  //    splits on LITERAL SPACE ONLY (docs/uix-dom.md section 7, revised 2026-08-05,
  //    UIX-CLASS-SPLIT-SPEC), matching upstream RmlUi's ElementStyle::SetClassNames
  //    (ExpandString(raw, ' ')), NOT the 4-char whitespace set section 6 defines. The raw value
  //    is "\tb a\ta " (TAB,b, SPACE, a,TAB,a, SPACE): the leading tab before "b" has no real
  //    content to its left within its space-delimited segment, so it is trimmed away (segment
  //    "\tb" -> token "b"); the tab embedded between the two 'a's sits BETWEEN two real bytes
  //    already inside that segment's span, so it survives verbatim (segment "a\ta" -> token
  //    "a\ta", tab included) -- see section 7's own step-by-step algorithm and its worked
  //    "\tb"/"a\ta" example. Sorted byte-wise, "a\ta" (0x61 0x09 0x61) sorts before "b" (0x62).
  //    "id"/"class" must not reappear in the generic ATTR block; a present-but-empty attribute
  //    (data-if="") still gets its own ATTR line.
  {
    const std::string source =
        "<rml><body>"
        "<div id=\"w\" class=\"&#9;b a&#9;a \" data-z=\"1\" data-a=\"2\" data-if=\"\"></div>"
        "</body></rml>";
    const std::string expected =
        "HEAD ABSENT\n"
        "body ELEM body\n"
        "body CHILDREN 1\n"
        "body/0 ELEM div\n"
        "body/0 ID w\n"
        "body/0 CLASS a\\ta b\n"
        "body/0 ATTR data-a=2\n"
        "body/0 ATTR data-if=\n"
        "body/0 ATTR data-z=1\n"
        "body/0 CHILDREN 0\n";
    Rml::ElementDocument* doc = load_close(source);
    check(doc != nullptr, "F2: LoadDocumentFromMemory(class/attr sorting fixture) succeeded");
    if (doc) {
      check_eq(glintfx::dom_dump_document(doc, source), expected,
               "F2: CLASS split on literal space only (leading tab trimmed, embedded tab "
               "glued/kept, per section 7) + dedup + byte-sort, ATTR sorted by name with "
               "id/class excluded, empty-value ATTR still emitted");
    }
    close_doc(doc);
  }

  // -- F3: verbatim, non-collapsed whitespace inside TEXT content (section 6b). -----------
  {
    const std::string source = "<rml><body><div>  Hello   world  </div></body></rml>";
    const std::string expected =
        "HEAD ABSENT\n"
        "body ELEM body\n"
        "body CHILDREN 1\n"
        "body/0 ELEM div\n"
        "body/0 CHILDREN 1\n"
        "body/0/0 TEXT   Hello   world  \n";
    Rml::ElementDocument* doc = load_close(source);
    check(doc != nullptr, "F3: LoadDocumentFromMemory(verbatim-whitespace fixture) succeeded");
    if (doc) {
      check_eq(glintfx::dom_dump_document(doc, source), expected,
               "F3: TEXT content is byte-verbatim, no trim/collapse of internal or edge spaces");
    }
    close_doc(doc);
  }

  // -- F4: a whitespace-only child is never a node (section 6a), proved via the REAL engine,
  //    zero filtering code of this dumper's own. ------------------------------------------
  {
    const std::string source = "<rml><body><div>   </div></body></rml>";
    const std::string expected =
        "HEAD ABSENT\n"
        "body ELEM body\n"
        "body CHILDREN 1\n"
        "body/0 ELEM div\n"
        "body/0 CHILDREN 0\n";
    Rml::ElementDocument* doc = load_close(source);
    check(doc != nullptr, "F4: LoadDocumentFromMemory(whitespace-only-child fixture) succeeded");
    if (doc) {
      check_eq(glintfx::dom_dump_document(doc, source), expected,
               "F4: a whitespace-only text child produces CHILDREN 0, not CHILDREN 1 -- RmlUi's "
               "own Factory::InstanceElementText filter, never re-implemented by this dumper");
    }
    close_doc(doc);
  }

  // -- F5: determinism -- the SAME document dumped twice is byte-identical. ----------------
  {
    const std::string source = "<rml><body><div id=\"x\" class=\"b a\" data-p=\"1\" data-q=\"2\">hi</div></body></rml>";
    Rml::ElementDocument* doc = load_close(source);
    check(doc != nullptr, "F5: LoadDocumentFromMemory(determinism fixture A) succeeded");
    if (doc) {
      const std::string dump1 = glintfx::dom_dump_document(doc, source);
      const std::string dump2 = glintfx::dom_dump_document(doc, source);
      check_eq(dump2, dump1, "F5: dumping the SAME document twice is byte-identical");
    }
    close_doc(doc);
  }

  // -- F6: determinism -- two documents whose markup differs ONLY in attribute/class SOURCE
  //    ORDER dump to the exact SAME bytes (the concrete proof section 7's sort exists for:
  //    Rml::Dictionary/SmallUnorderedMap is not contractually ordered, so without sorting the
  //    two dumps below could legitimately differ run-to-run even though the documents are
  //    semantically identical). ----------------------------------------------------------
  {
    const std::string source_a = "<rml><body><div id=\"x\" class=\"wide highlighted\" data-p=\"1\" data-q=\"2\">hi</div></body></rml>";
    const std::string source_b = "<rml><body><div data-q=\"2\" class=\"highlighted wide\" id=\"x\" data-p=\"1\">hi</div></body></rml>";
    Rml::ElementDocument* doc_a = load_close(source_a);
    check(doc_a != nullptr, "F6: LoadDocumentFromMemory(reordered fixture A) succeeded");
    std::string dump_a;
    if (doc_a) dump_a = glintfx::dom_dump_document(doc_a, source_a);
    close_doc(doc_a);

    Rml::ElementDocument* doc_b = load_close(source_b);
    check(doc_b != nullptr, "F6: LoadDocumentFromMemory(reordered fixture B) succeeded");
    if (doc_b && !dump_a.empty()) {
      check_eq(glintfx::dom_dump_document(doc_b, source_b), dump_a,
               "F6: attribute/class SOURCE ORDER never affects the dump -- sorting neutralizes it");
    }
    close_doc(doc_b);
  }

  if (g_failures == 0) {
    std::puts("dom_dump_determinism_sanity OK");
    return 0;
  }
  std::printf("dom_dump_determinism_sanity: %d failure(s)\n", g_failures);
  return 10;
}
