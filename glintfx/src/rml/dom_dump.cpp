// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6a implementation. See dom_dump.hpp's own header comment for the confinement
//     rationale and the two spec-faithfulness decisions (class split from the RAW attribute,
//     not `GetClassNames()`; `<head>` extraction is a plain text scan, not a tree walk) this
//     file implements.
// PT: Implementação da RMLX-1/S6a. Ver o próprio comentário de cabeçalho de dom_dump.hpp pro
//     racional de confinamento e as duas decisões de fidelidade-à-spec (split de classe a
//     partir do atributo CRU, não `GetClassNames()`; extração de `<head>` é um scan de texto
//     puro, não uma travessia de árvore) que este arquivo implementa.
// Copyright (c) 2026 Petrus Silva Costa
#include "dom_dump.hpp"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace glintfx {

namespace {

// EN: The 4-character whitespace set docs/uix-dom.md sections 2/6/7 all anchor on -- the SAME
//     set `StringUtilities::IsWhitespace` uses upstream
//     (`examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64`), not a coincidence (section
//     2's own prose says so). Named once here so every caller below (escape, class-token split,
//     self-closing-tag trailing-space skip) shares the exact same definition -- a SECOND,
//     silently-different "what counts as whitespace" in this file would be the precise kind of
//     private assumption docs/uix-dom.md's own "Why this document exists" section warns about.
// PT: O conjunto de 4 caracteres de whitespace em que as seções 2/6/7 do docs/uix-dom.md se
//     ancoram -- o MESMO conjunto que `StringUtilities::IsWhitespace` usa upstream
//     (`examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64`), não é coincidência (a
//     própria prosa da seção 2 diz isso). Nomeado uma vez aqui pra todo chamador abaixo (escape,
//     split de token de classe, pulo de espaço-final de tag auto-fechada) compartilhar
//     exatamente a mesma definição -- uma SEGUNDA definição de "o que conta como whitespace",
//     silenciosamente diferente, neste arquivo seria exatamente o tipo de suposição privada que
//     a própria seção "Por que este documento existe" do docs/uix-dom.md avisa.
bool is_ws(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char to_lower_ascii(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// EN: Case-insensitive literal substring search, ASCII-fold only (both needles this file
//     searches for -- "<head", "</head" -- are pure ASCII, so folding non-ASCII bytes would be
//     pointless work, not a correctness gap).
// PT: Busca de substring literal insensível a caixa, dobra só ASCII (as duas agulhas que este
//     arquivo busca -- "<head", "</head" -- são ASCII puro, então dobrar byte não-ASCII seria
//     trabalho inútil, não uma lacuna de correção).
std::size_t ifind(const std::string& hay, const char* needle, std::size_t from) {
  const std::size_t needle_len = std::string(needle).size();
  if (needle_len == 0 || hay.size() < needle_len) return std::string::npos;
  for (std::size_t i = from; i + needle_len <= hay.size(); ++i) {
    bool match = true;
    for (std::size_t j = 0; j < needle_len; ++j) {
      if (to_lower_ascii(hay[i + j]) != to_lower_ascii(needle[j])) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return std::string::npos;
}

// EN: Splits `raw` on runs of the 4-char whitespace set, drops empty tokens, deduplicates and
//     sorts ascending byte-wise -- all three in one step via `std::set<std::string>`, whose
//     default `operator<` on `std::string` compares via `char_traits<char>::lt`, DEFINED by the
//     standard as an UNSIGNED-byte comparison (`static_cast<unsigned char>(c1) <
//     static_cast<unsigned char>(c2)`) -- i.e. exactly the "strcmp-style, no locale" byte-wise
//     order docs/uix-dom.md section 7 asks for, with zero extra code needed to get there.
// PT: Divide `raw` em runs do conjunto de 4 caracteres de whitespace, descarta tokens vazios,
//     deduplica e ordena ascendente byte-a-byte -- os três num passo só via
//     `std::set<std::string>`, cujo `operator<` default sobre `std::string` compara via
//     `char_traits<char>::lt`, DEFINIDO pela norma como comparação de byte SEM SINAL
//     (`static_cast<unsigned char>(c1) < static_cast<unsigned char>(c2)`) -- ou seja, exatamente
//     a ordem byte-a-byte "estilo strcmp, sem locale" que a seção 7 do docs/uix-dom.md pede, sem
//     código extra nenhum pra chegar lá.
std::set<std::string> split_dedup_sorted(const std::string& raw) {
  std::set<std::string> out;
  std::size_t i = 0;
  const std::size_t n = raw.size();
  while (i < n) {
    while (i < n && is_ws(raw[i])) ++i;
    const std::size_t start = i;
    while (i < n && !is_ws(raw[i])) ++i;
    if (i > start) out.insert(raw.substr(start, i - start));
  }
  return out;
}

} // namespace

std::string dom_dump_escape(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (unsigned char c : raw) {
    switch (c) {
      case '\\':
        out += "\\\\";
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
        out += static_cast<char>(c);
        break;
    }
  }
  return out;
}

HeadScanResult dom_dump_scan_head(const std::string& source) {
  // EN: 1) Find "<head" as an actual tag-open token, not a longer identifier accidentally
  //     containing that substring ("<headfoo") -- the character right after must be whitespace,
  //     '>' (immediate close, no attributes) or '/' (self-closing, no attributes).
  // PT: 1) Acha "<head" como um token real de abertura de tag, não um identificador mais longo
  //     que contenha esse substring por acidente ("<headfoo") -- o caractere logo depois tem
  //     que ser whitespace, '>' (fechamento imediato, sem atributos) ou '/' (auto-fechamento,
  //     sem atributos).
  std::size_t open_start = std::string::npos;
  {
    std::size_t pos = 0;
    for (;;) {
      const std::size_t p = ifind(source, "<head", pos);
      if (p == std::string::npos) break;
      const std::size_t after = p + 5;
      if (after < source.size()) {
        const char c = source[after];
        if (c == '>' || c == '/' || is_ws(c)) {
          open_start = p;
          break;
        }
      }
      pos = p + 5;
    }
  }
  if (open_start == std::string::npos) return HeadScanResult{false, ""};

  // EN: 2) Scan forward, quote-aware, for the '>' that closes the OPENING tag -- a '>' inside a
  //     quoted attribute value of <head ...> itself must not end the tag early.
  // PT: 2) Varre pra frente, consciente de aspas, pelo '>' que fecha a tag de ABERTURA -- um
  //     '>' dentro de um valor de atributo entre aspas do próprio <head ...> não pode fechar a
  //     tag cedo demais.
  std::size_t close_gt = std::string::npos;
  {
    bool in_squote = false;
    bool in_dquote = false;
    for (std::size_t i = open_start + 5; i < source.size(); ++i) {
      const char c = source[i];
      if (in_squote) {
        if (c == '\'') in_squote = false;
        continue;
      }
      if (in_dquote) {
        if (c == '"') in_dquote = false;
        continue;
      }
      if (c == '\'') {
        in_squote = true;
        continue;
      }
      if (c == '"') {
        in_dquote = true;
        continue;
      }
      if (c == '>') {
        close_gt = i;
        break;
      }
    }
  }
  // EN: An unterminated opening tag (no closing '>' anywhere in the rest of the source) is
  //     malformed input -- out of docs/uix-dom.md's scope (section 3's own "a document missing
  //     <body> is out of this format's scope" precedent for malformed input). Fail toward
  //     ABSENT rather than guessing.
  // PT: Uma tag de abertura não-terminada (nenhum '>' de fechamento no resto da fonte) é input
  //     malformado -- fora do escopo do docs/uix-dom.md (mesmo precedente da própria seção 3,
  //     "um documento sem <body> está fora do escopo deste formato", pra input malformado).
  //     Falha pro lado de ABSENT em vez de arriscar um chute.
  if (close_gt == std::string::npos) return HeadScanResult{false, ""};

  // EN: 3) Self-closing? Walk back over trailing whitespace before the '>'; if the preceding
  //     non-space byte is '/', this is `<head .../>` with no content and no `</head>` to find.
  // PT: 3) Auto-fechada? Anda pra trás sobre whitespace final antes do '>'; se o byte
  //     não-espaço anterior for '/', é um `<head .../>` sem conteúdo e sem `</head>` pra achar.
  {
    std::size_t j = close_gt;
    while (j > open_start && is_ws(source[j - 1])) --j;
    if (j > open_start && source[j - 1] == '/') {
      return HeadScanResult{true, ""};
    }
  }

  // EN: 4) Content starts right after the opening tag's '>'. `</head>` cannot nest (RML/XML
  //     grammar forbids <head> inside <head>), so a plain, non-quote-aware substring search for
  //     the literal text "</head" is exactly what section 4's own wording asks for ("up to (not
  //     including) the `<` that opens `</head>`") -- see this file's header comment for the
  //     documented limitation this implies (a coincidental "</head" inside e.g. a CSS comment
  //     would end the scan early; not corpus-exercised).
  // PT: 4) O conteúdo começa logo após o '>' da tag de abertura. `</head>` não pode aninhar
  //     (gramática RML/XML proíbe <head> dentro de <head>), então uma busca de substring simples
  //     (sem consciência de aspas) pelo texto literal "</head" é exatamente o que a própria
  //     seção 4 pede ("até (sem incluir) o `<` que abre `</head>`") -- ver o comentário de
  //     cabeçalho deste arquivo pra limitação documentada que isso implica (um "</head"
  //     coincidente dentro de, por ex., um comentário CSS terminaria o scan cedo demais; não
  //     exercitado pelo corpus).
  const std::size_t content_start = close_gt + 1;
  const std::size_t close_tag_pos = ifind(source, "</head", content_start);
  if (close_tag_pos == std::string::npos) {
    // EN: Opening tag found but no matching close -- malformed. Return everything to end of
    //     source rather than silently dropping content; still marked PRESENT since a real
    //     `<head>` open tag genuinely was found.
    // PT: Tag de abertura achada mas sem fechamento correspondente -- malformado. Devolve tudo
    //     até o fim da fonte em vez de derrubar conteúdo em silêncio; ainda marcado PRESENT já
    //     que uma tag de abertura `<head>` real de fato foi achada.
    return HeadScanResult{true, source.substr(content_start)};
  }
  return HeadScanResult{true, source.substr(content_start, close_tag_pos - content_start)};
}

namespace {

// EN: Sorted (name, decoded-string-value) pairs for `el`'s generic ATTR block -- "id"/"class"
//     excluded (section 7: they have dedicated ID/CLASS lines), sorted by NAME ascending
//     byte-wise (same `std::string operator<` reasoning as split_dedup_sorted() above).
//     `Rml::ElementAttributes` (`Dictionary` = `SmallUnorderedMap<String, Variant>`) is NOT
//     contractually ordered -- this sort is the reason dom_dump_determinism_sanity.cpp's F6
//     case (same attributes, different SOURCE order) proves byte-identical output.
// PT: Pares (nome, valor-string-decodificado) ordenados pro bloco ATTR genérico de `el` --
//     "id"/"class" excluídos (seção 7: têm linhas ID/CLASS dedicadas), ordenados por NOME
//     ascendente byte-a-byte (mesmo raciocínio de `std::string operator<` do
//     split_dedup_sorted() acima). `Rml::ElementAttributes` (`Dictionary` =
//     `SmallUnorderedMap<String, Variant>`) NÃO é ordenado por contrato -- este sort é o motivo
//     do caso F6 do dom_dump_determinism_sanity.cpp (mesmos atributos, ordem-fonte diferente)
//     provar saída byte-idêntica.
std::vector<std::pair<std::string, std::string>> sorted_generic_attrs(const Rml::Element* el) {
  std::vector<std::pair<std::string, std::string>> attrs;
  for (const auto& kv : el->GetAttributes()) {
    const std::string& name = kv.first;
    if (name == "id" || name == "class") continue;
    attrs.emplace_back(std::string(name), kv.second.Get<Rml::String>());
  }
  std::sort(attrs.begin(), attrs.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  return attrs;
}

// EN: `path` is the CALLER's own path (already includes this element's own segment) -- pure
//     text-emission function, no traversal decision made here (traversal/indexing lives in
//     dump_node below).
// PT: `path` é o próprio caminho do CHAMADOR (já inclui o segmento deste elemento) -- função
//     puramente de emissão de texto, nenhuma decisão de travessia é tomada aqui
//     (travessia/indexação mora no dump_node abaixo).
void emit_element_header(const Rml::Element* el, const std::string& path, std::string& out) {
  out += path;
  out += " ELEM ";
  out += std::string(el->GetTagName());
  out += "\n";

  const std::string id(el->GetId());
  if (!id.empty()) {
    out += path;
    out += " ID ";
    out += dom_dump_escape(id);
    out += "\n";
  }

  // EN: Raw `class` attribute value -- see dom_dump.hpp's "SPEC ADDENDUM" for why this is
  //     GetAttribute(), never GetClassNames().
  // PT: Valor cru do atributo `class` -- ver o "ADENDO DE SPEC" de dom_dump.hpp pro motivo
  //     disto ser GetAttribute(), nunca GetClassNames().
  const std::string raw_class = el->GetAttribute<Rml::String>("class", Rml::String());
  const std::set<std::string> classes = split_dedup_sorted(raw_class);
  if (!classes.empty()) {
    out += path;
    out += " CLASS";
    for (const std::string& c : classes) {
      out += " ";
      out += dom_dump_escape(c);
    }
    out += "\n";
  }

  for (const auto& [name, value] : sorted_generic_attrs(el)) {
    out += path;
    out += " ATTR ";
    out += dom_dump_escape(name);
    out += "=";
    out += dom_dump_escape(value);
    out += "\n";
  }
}

// EN: Pre-order depth-first, one node (element OR text) per call -- see docs/uix-dom.md
//     section 5 for the two record shapes this dispatches between. `include_non_dom_elements =
//     false` (GetNumChildren/GetChild's own default) on purpose: excludes internally-generated
//     pseudo-elements (e.g. scrollbar widgets) RmlUi may attach to certain elements, which are
//     NOT part of the authored source tree this format describes -- see dom_dump.hpp's header
//     comment for why this is a documented, deliberate reading of section 5 ("the body
//     subtree"), not a silent omission.
// PT: Pré-ordem profundidade-primeiro, um nó (elemento OU texto) por chamada -- ver a seção 5
//     do docs/uix-dom.md pras duas formas de registro entre as quais isto despacha.
//     `include_non_dom_elements = false` (o próprio default de GetNumChildren/GetChild) de
//     propósito: exclui pseudo-elementos gerados internamente (ex.: widgets de scrollbar) que o
//     RmlUi pode anexar a certos elementos, que NÃO fazem parte da árvore-fonte autorada que
//     este formato descreve -- ver o comentário de cabeçalho de dom_dump.hpp pro motivo disto
//     ser uma leitura documentada e deliberada da seção 5 ("a subárvore body"), não uma omissão
//     silenciosa.
void dump_node(const Rml::Element* el, const std::string& path, std::string& out) {
  if (el->GetTagName() == "#text") {
    const auto* text_el = dynamic_cast<const Rml::ElementText*>(el);
    // EN: Guarded, not asserted -- "#text" is Factory's own tag+instancer name for
    //     Rml::ElementText (Factory.cpp:330-341/394), so this should never fail for a document
    //     RmlUi itself produced; a defensive empty TEXT line (rather than a null-deref) is the
    //     fail-safe choice if that invariant is ever violated by a future RmlUi version.
    // PT: Guardado, não assertado -- "#text" é o próprio nome de tag+instancer do Factory pro
    //     Rml::ElementText (Factory.cpp:330-341/394), então isto nunca deveria falhar pra um
    //     documento que o próprio RmlUi produziu; uma linha TEXT vazia defensiva (em vez de um
    //     null-deref) é a escolha à prova de falha se essa invariante for violada por uma
    //     futura versão do RmlUi.
    out += path;
    out += " TEXT ";
    out += dom_dump_escape(text_el ? std::string(text_el->GetText()) : std::string());
    out += "\n";
    return;
  }

  emit_element_header(el, path, out);
  const int n = el->GetNumChildren(false);
  out += path;
  out += " CHILDREN ";
  out += std::to_string(n);
  out += "\n";
  for (int i = 0; i < n; ++i) {
    dump_node(el->GetChild(i), path + "/" + std::to_string(i), out);
  }
}

} // namespace

std::string dom_dump_document(Rml::ElementDocument* doc, const std::string& source) {
  assert(doc != nullptr &&
         "dom_dump_document: doc must not be null (docs/uix-dom.md section 3: a document "
         "missing <body> is out of this format's scope; a null doc is a strictly worse caller "
         "error, checked by the call site, not this function)");

  std::string out;
  const HeadScanResult head = dom_dump_scan_head(source);
  if (head.present) {
    out += "HEAD PRESENT ";
    out += dom_dump_escape(head.raw);
    out += "\n";
  } else {
    out += "HEAD ABSENT\n";
  }

  // EN: `doc` (an Rml::ElementDocument*) IS the `<body>` element itself upstream -- docs/uix-
  //     dom.md section 3, evidenced by XMLNodeHandlerBody::ElementStart applying <body>'s own
  //     attributes directly to the ElementDocument when `document == element`
  //     (examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20) -- so dumping it through the
  //     SAME dump_node() every other element goes through (no root-specific branch) is exactly
  //     section 5's own "uniform record shape for every node, root included" design, not a
  //     coincidence that happens to produce the right tag name.
  // PT: `doc` (um Rml::ElementDocument*) É o próprio elemento `<body>` upstream -- seção 3 do
  //     docs/uix-dom.md, evidenciado por XMLNodeHandlerBody::ElementStart aplicar os próprios
  //     atributos de `<body>` direto no ElementDocument quando `document == element`
  //     (examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20) -- então dumpar ele pelo MESMO
  //     dump_node() por que todo outro elemento passa (sem ramo específico-de-raiz) é
  //     exatamente o próprio design "forma de registro uniforme pra todo nó, raiz incluída" da
  //     seção 5, não uma coincidência que por acaso produz o nome de tag certo.
  dump_node(doc, "body", out);
  return out;
}

} // namespace glintfx
