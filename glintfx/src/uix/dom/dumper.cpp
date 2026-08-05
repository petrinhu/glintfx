// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6b -- implementation. See dumper.hpp's own header comment for the full scope/
//     boundary/design-decision rationale this file holds itself to; the comments here are about
//     HOW, not WHY (the WHY already lives in the header, once, not duplicated per function --
//     same discipline dom_tree.cpp/lexer.cpp/parser.cpp already follow for their own headers).
// PT: RMLX-1/S6b -- implementação. Ver o próprio comentário de cabeçalho do dumper.hpp pro
//     escopo/fronteira/racional-de-decisão-de-desenho completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função -- mesma disciplina que dom_tree.cpp/lexer.cpp/parser.cpp já seguem
//     pros próprios headers).
// Copyright (c) 2026 Petrus Silva Costa
#include "dumper.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace glintfx::uix {

std::string escape(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
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
        out += c;
        break;
    }
  }
  return out;
}

namespace {

// EN: `path` is the caller-computed, already-`escape`-free node address (uix-dom.md section 3:
//     "n being the node's 0-based position among its parent's surviving children" -- the path
//     ITSELF is never escaped, it is a structural coordinate, not a value-bearing field section
//     2's table applies to). `out` accumulates the whole file; appended to, never rebuilt, so the
//     total cost of one `dump_document` call is linear in the size of the tree, not quadratic.
// PT: `path` é o endereço de nó já computado por quem chama, já livre de `escape` (seção 3 do
//     uix-dom.md: "n sendo a posição 0-based do nó entre os filhos sobreviventes do pai" -- o
//     PRÓPRIO caminho nunca é escapado, é uma coordenada estrutural, não um campo que carrega
//     valor a que a tabela da seção 2 se aplica). `out` acumula o arquivo inteiro; recebe
//     append, nunca é reconstruído, então o custo total de uma chamada a `dump_document` é
//     linear no tamanho da árvore, não quadrático.
void dump_node(const Node& node, const std::string& path, std::string& out);

// EN: uix-dom.md section 5's fixed field order: ELEM, ID (omitted if empty/absent), CLASS
//     (omitted if the set is empty), ATTR (0..N lines), CHILDREN (always) -- then each child's
//     own block, in source order (`Element::children()`'s own vector order, which S3 populates
//     in source order and this dumper never reorders).
// PT: A ordem de campo fixa da seção 5 do uix-dom.md: ELEM, ID (omitido se vazio/ausente), CLASS
//     (omitido se o conjunto for vazio), ATTR (0..N linhas), CHILDREN (sempre) -- depois o
//     próprio bloco de cada filho, em ordem-fonte (a própria ordem de vetor de
//     `Element::children()`, que a S3 popula em ordem-fonte e este dumper nunca reordena).
void dump_element(const Element& element, const std::string& path, std::string& out) {
  out += path;
  out += " ELEM ";
  // EN: tag is NOT escaped (uix-dom.md section 5: "<tag> is not escaped -- RML tag names cannot
  //     contain the escaped characters") and NOT case-folded by this dumper (section 8: any
  //     tag-name transform is upstream-parser territory, never this dumper's own initiative).
  // PT: tag NÃO é escapada (seção 5 do uix-dom.md: "<tag> não é escapado -- nomes de tag RML não
  //     podem conter os caracteres escapados") e NÃO é dobrada de caixa por este dumper (seção 8:
  //     qualquer transformação de nome de tag é território do parser upstream, nunca iniciativa
  //     própria deste dumper).
  out += element.tag();
  out += '\n';

  // EN: uix-dom.md section 7's empty-value asymmetry for `id` specifically: an empty id and an
  //     absent id are the SAME state, `has_id()` is exactly that gate (see dom_tree.hpp's own
  //     `Element::id()` comment: "empty string means no id, identical to an explicit id=\"\"").
  // PT: a assimetria de valor-vazio da seção 7 do uix-dom.md pra `id` especificamente: um id
  //     vazio e um id ausente são o MESMO estado, `has_id()` é exatamente esse portão (ver o
  //     próprio comentário de `Element::id()` do dom_tree.hpp: "string vazia significa sem id,
  //     idêntico a um id=\"\" explícito").
  if (element.has_id()) {
    out += path;
    out += " ID ";
    out += escape(element.id());
    out += '\n';
  }

  // EN: `Element::classes()` is already sorted ascending byte-wise and deduplicated (a
  //     `std::set<std::string, std::less<>>`, per dom_tree.hpp's own header comment) -- this
  //     loop only needs to iterate and escape each token, never sort/dedupe anything itself.
  //     uix-dom.md section 5: the whole CLASS line, not just each token, is OMITTED when the set
  //     is empty.
  // PT: `Element::classes()` já vem ordenado ascendente byte-a-byte e deduplicado (um
  //     `std::set<std::string, std::less<>>`, pelo próprio comentário de cabeçalho do
  //     dom_tree.hpp) -- este laço só precisa iterar e escapar cada token, nunca ordenar/
  //     deduplicar nada sozinho. Seção 5 do uix-dom.md: a linha CLASS inteira, não só cada token,
  //     é OMITIDA quando o conjunto é vazio.
  if (!element.classes().empty()) {
    out += path;
    out += " CLASS";
    for (const std::string& token : element.classes()) {
      out += ' ';
      out += escape(token);
    }
    out += '\n';
  }

  // EN: `Element::attributes()` is already sorted by name, ascending byte-wise (a
  //     `std::map<std::string, std::string, std::less<>>`) and NEVER contains an `"id"`/
  //     `"class"` key (those two names are reserved in `Element::set_attribute`, per
  //     dom_tree.hpp's own header comment) -- this loop needs no exclusion logic of its own for
  //     that reason. Attribute NAME is printed raw, per uix-dom.md section 2's own list of
  //     escaped fields naming "each ATTR value" and conspicuously not the name (see this file's
  //     own dumper_sanity.cpp companion test, test_attribute_name_never_escaped, for the pinned
  //     behaviour).
  // PT: `Element::attributes()` já vem ordenado por nome, ascendente byte-a-byte (um
  //     `std::map<std::string, std::string, std::less<>>`) e NUNCA contém chave `"id"`/
  //     `"class"` (esses dois nomes são reservados no `Element::set_attribute`, pelo próprio
  //     comentário de cabeçalho do dom_tree.hpp) -- este laço não precisa de lógica de exclusão
  //     nenhuma própria por esse motivo. Nome de ATRIBUTO é impresso cru, pela própria lista de
  //     campos escapados da seção 2 do uix-dom.md nomeando "cada valor de ATTR" e visivelmente
  //     não o nome (ver o próprio teste companheiro dumper_sanity.cpp deste arquivo,
  //     test_attribute_name_never_escaped, pro comportamento fixado).
  for (const auto& [name, value] : element.attributes()) {
    out += path;
    out += " ATTR ";
    out += name;
    out += '=';
    out += escape(value);
    out += '\n';
  }

  // EN: uix-dom.md section 5: "CHILDREN <n> -- ALWAYS present, even at n=0". `child_count()` IS
  //     the "surviving children" count section 3 requires -- see this file's dumper.hpp header
  //     comment, "What this slice deliberately does not re-do", for why no whitespace-filtering
  //     happens here.
  // PT: seção 5 do uix-dom.md: "CHILDREN <n> -- SEMPRE presente, mesmo em n=0". `child_count()`
  //     JÁ É a contagem de "filhos sobreviventes" que a seção 3 exige -- ver o próprio comentário
  //     de cabeçalho do dumper.hpp deste arquivo, "O que esta fatia deliberadamente não refaz",
  //     pra por que nenhuma filtragem de whitespace acontece aqui.
  out += path;
  out += " CHILDREN ";
  out += std::to_string(element.child_count());
  out += '\n';

  std::size_t index = 0;
  for (const auto& child : element.children()) {
    std::string child_path = path;
    child_path += '/';
    child_path += std::to_string(index);
    dump_node(*child, child_path, out);
    ++index;
  }
}

void dump_node(const Node& node, const std::string& path, std::string& out) {
  if (const Element* element = as_element(&node)) {
    dump_element(*element, path, out);
    return;
  }
  if (const Text* text = as_text(&node)) {
    // EN: uix-dom.md section 5: text node, exactly one line, `<escaped text>` per section 2.
    // PT: seção 5 do uix-dom.md: nó texto, exatamente uma linha, `<texto escapado>` pela seção 2.
    out += path;
    out += " TEXT ";
    out += escape(text->content());
    out += '\n';
    return;
  }
  // EN: unreachable -- `NodeKind` has exactly two values (Element, Text) and `as_element`/
  //     `as_text` between them cover every live `Node` this tree can ever hold (dom_tree.hpp's
  //     own `NodeKind` enum). Left as a silent no-op rather than an assert: this module has zero
  //     link dependency on purpose (see dumper.hpp's own header comment, "no logging in this
  //     module" reasoning mirrored from dom_tree.hpp) and a `Node` reaching here with neither
  //     kind would already be undefined behaviour one layer down (dom_tree.hpp construction
  //     invariants), not something this function can meaningfully diagnose or recover from.
  // PT: inalcançável -- `NodeKind` tem exatamente dois valores (Element, Text) e `as_element`/
  //     `as_text` entre os dois cobrem todo `Node` vivo que esta árvore consegue guardar (o
  //     próprio enum `NodeKind` do dom_tree.hpp). Deixado como no-op silencioso em vez de assert:
  //     este módulo tem zero dependência de link de propósito (ver o próprio comentário de
  //     cabeçalho do dumper.hpp, raciocínio "sem logging neste módulo" espelhado do
  //     dom_tree.hpp) e um `Node` chegando aqui sem nenhum dos dois tipos já seria comportamento
  //     indefinido uma camada abaixo (invariantes de construção do dom_tree.hpp), não algo que
  //     esta função consiga diagnosticar ou recuperar de forma significativa.
}

} // namespace

std::string dump_document(const Document& document) {
  std::string out;

  // EN: uix-dom.md section 1: "the first line is always the HEAD record (section 4)". Section 4:
  //     "HEAD ABSENT is emitted, verbatim, when the source document has no <head> element at all
  //     -- never omit the HEAD line itself in that case".
  // PT: seção 1 do uix-dom.md: "a primeira linha é sempre o registro HEAD (seção 4)". Seção 4:
  //     "HEAD ABSENT é emitido, verbatim, quando o documento-fonte não tem <head> nenhum -- nunca
  //     omitir a própria linha HEAD nesse caso".
  const HeadContent& head = document.head();
  if (head.present) {
    out += "HEAD PRESENT ";
    out += escape(head.raw);
    out += '\n';
  } else {
    out += "HEAD ABSENT\n";
  }

  // EN: uix-dom.md section 3: "body is the literal root path (no numeric suffix)"; section 5:
  //     "the body root itself gets the full element block too -- there is no special-cased 'root
  //     has no header' shortcut". `document.body()` is always a valid, fully-constructed
  //     `Element` (see `Document`'s own constructor comment in dom_tree.hpp: there is no
  //     "document with no body" state this class can represent), so this call needs no null
  //     check.
  // PT: seção 3 do uix-dom.md: "body é o caminho-raiz literal (sem sufixo numérico)"; seção 5: "a
  //     própria raiz body recebe o bloco de elemento completo também -- não existe atalho
  //     especial de 'raiz não tem cabeçalho'". `document.body()` é sempre um `Element` válido,
  //     totalmente construído (ver o próprio comentário do construtor de `Document` no
  //     dom_tree.hpp: não existe estado "documento sem body" que esta classe consiga
  //     representar), então esta chamada não precisa de checagem de nulo nenhuma.
  dump_element(document.body(), "body", out);

  return out;
}

} // namespace glintfx::uix
