// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S3 -- recursive-descent parser: consumes S1's `Lexer` token stream and builds S2's
//     `Document`/`Element`/`Text` tree. This is the THIRD brick of glintfx's own DOM. Public
//     surface is deliberately ONE function, `parse_document` -- the `Parser` class that does the
//     work is a `parser.cpp`-local implementation detail, not exposed here, so nothing outside
//     this module can depend on its internal recursive-descent shape.
//
//     WHAT THIS SLICE RESOLVES (the two points lexer.hpp explicitly left open, plus the ones this
//     slice's own brief names):
//       - Entity decoding (docs/uix-dom.md, "uix-dom.md" hereafter, section 6c): a FIXED table --
//         the 5 standard XML named entities (`amp`/`lt`/`gt`/`quot`/`apos`) plus `nbsp` (the one
//         extra entity the real corpus uses), and numeric character references (`&#38;` decimal,
//         `&#x26;` hex) decoded to their UTF-8 bytes. An entity this table does not recognise, or
//         a bare '&' not closed by a terminating ';' within a short bounded scan, is FAIL-HIGH: a
//         `ParseError`, never silently dropped or passed through raw. Applies to BOTH `Text`
//         content and attribute VALUES (standard XML character-data handling covers both) -- NOT
//         to `<head>`'s opaque payload, which uix-dom.md section 4's own last paragraph exempts
//         explicitly (CSS has no entity syntax; decoding it would corrupt it).
//       - `<head>` opacity (uix-dom.md section 4): captured via a RAW BYTE SCAN over the original
//         source buffer for the literal `</head` (case-folded) + whitespace* + `>` sequence --
//         deliberately NOT by reusing this module's own tag-aware skip logic on `<head>`'s
//         interior. A generic "skip nested tags with the shared Lexer" approach would make
//         `<head>`'s opacity contingent on its content happening to tokenize as valid tag/text
//         grammar (true for every fixture in today's corpus, per lexer_corpus_sanity, but NOT
//         guaranteed -- a literal, un-escaped '<' inside real CSS/JS content, e.g. an attribute
//         selector `a[href^="<"]`, would derail a tag-aware skip and turn "opaque payload" into a
//         spurious lexer `Error`). The raw scan has no such dependency: it is a dumb byte search,
//         exactly matching uix-dom.md section 4's own framing of the payload as "raw source bytes
//         exactly as written", genuinely un-parsed. Consequence: this module's `Lexer` instance is
//         DISCARDED and REBUILT once, over the substring starting right after `</head>`'s closing
//         '>', the moment a real `<head>` is found (see parser.cpp's own header comment for how
//         absolute-offset error reporting survives that rebuild).
//       - Opening/closing tag NAME matching (`<div>...</span>`): a simple stack, one entry per
//         currently-open element, implicit in this parser's own C++ call stack (recursive-descent
//         -- each nested element is one more stack frame) rather than an explicit
//         `std::vector<std::string>`. A `TagClose` whose (case-folded) name does not match the
//         (case-folded) name of the element currently being closed is a `ParseError` --
//         `lexer_tokens_sanity.cpp`'s own `test_mismatched_close_name_not_validated` proved the
//         LEXER accepts this; this module is where it stops being accepted.
//       - Tag-name case folding (uix-dom.md section 8, evidenced against
//         `XMLParser::HandleElementStart`/`XMLParser.cpp:136,167`): BOTH the opening and the
//         closing tag name are folded ASCII-lowercase before being stored (`Element::tag()`) or
//         compared -- this is real upstream RmlUi tree-construction-time behaviour, not a
//         dump-time cosmetic, so `<DIV>...</div>` genuinely closes (both fold to `"div"`) and the
//         resulting `Element::tag()` is `"div"`, never `"DIV"`. Attribute names, `id` values and
//         `class` tokens are NOT folded (same section, no equivalent evidence found for them).
//       - Whitespace-only-text existence filtering: NOT this module's job -- S2's own
//         `Element::append_child` already applies uix-dom.md section 6(a) as a TREE INVARIANT (see
//         `dom_tree.hpp`'s header comment, point (1)). This parser constructs a `Text` node for
//         EVERY `Text` token it decodes (whitespace-only or not) and simply calls `append_child`,
//         trusting the tree to decide whether it survives -- exactly the contract `dom_tree.hpp`
//         documents for its own callers.
//
//     TOP-LEVEL DOCUMENT GRAMMAR THIS PARSER ENFORCES (100% corpus-backed -- every one of the 44
//     real `.rml` fixtures under `glintfx/tests/`+`glintfx/demos/`, plus the GusWorld
//     `battle_cockpit_rml.cpp` fixture this slice adds, wraps in `<rml>`):
//       document := (Comment | whitespace-only Text)*
//                   '<rml' attrs (TagSelfClose | TagOpenEnd rml-body '</rml>')
//                   (Comment | whitespace-only Text)* EndOfFile
//       rml-body := (Comment | whitespace-only Text | head-once | body-once)*
//       (head, if present, MUST precede body; at most one of each; nothing else -- no stray
//       element, no non-whitespace text -- is a direct child of `<rml>`; a document with no
//       `<body>` at all is a `ParseError`, matching uix-dom.md section 3's own "a document missing
//       `<body>` entirely is a parse error, out of this format's scope".)
//     This is DELIBERATELY STRICTER than it needs to be to merely build a tree -- every one of
//     these rules is fail-high per this project's own "reject, report, never guess a transform the
//     corpus never asked for" discipline (see this file's OWN "Rejected constructions" list in
//     parser.cpp's header, and `docs/rmlx-subset.md`'s own frozen-boundary clause: a real fixture
//     that needs something outside this grammar stops the parser cold with a diagnosable
//     `ParseError`, never a silent guess).
//
//     ATTRIBUTE APPLICATION -- duplicate-attribute policy (NOT corpus-exercised, no fixture has
//     two `id=`/two `class=` on the same tag; this is this slice's own explicit, documented
//     choice, not a guess left implicit): the LAST occurrence wins for `id` and for `class` (the
//     class attribute's WHOLE VALUE is replaced by the last occurrence, tokens from an earlier
//     `class=` on the same tag are NOT merged in) -- ordinary DOM/attribute-map semantics, one
//     name maps to exactly one value, source-order-last is authoritative. Every OTHER attribute
//     name already gets this for free from `Element::set_attribute`'s own `std::map` overwrite
//     semantics when this parser applies attributes in source order.
// PT: RMLX-1/S3 -- parser recursivo-descendente: consome o fluxo de tokens do `Lexer` da S1 e
//     constrói a árvore `Document`/`Element`/`Text` da S2. Este é o TERCEIRO tijolo do DOM próprio
//     da glintfx. Superfície pública deliberadamente UMA função só, `parse_document` -- a classe
//     `Parser` que faz o trabalho é detalhe de implementação local ao `parser.cpp`, não exposta
//     aqui, pra nada fora deste módulo poder depender da forma interna do recursivo-descendente.
//
//     O QUE ESTA FATIA RESOLVE (os dois pontos que o lexer.hpp deixou explicitamente abertos, mais
//     os que o próprio briefing desta fatia nomeia):
//       - Decodificação de entidade (docs/uix-dom.md, "uix-dom.md" daqui em diante, seção 6c): uma
//         tabela FIXA -- as 5 entidades XML nomeadas padrão (`amp`/`lt`/`gt`/`quot`/`apos`) mais
//         `nbsp` (a única entidade extra que o corpus real usa), e referências numéricas de
//         caractere (`&#38;` decimal, `&#x26;` hex) decodificadas pros bytes UTF-8 delas. Uma
//         entidade que esta tabela não reconhece, ou um '&' cru não fechado por um ';' terminador
//         dentro de um scan curto e limitado, é FAIL-HIGH: um `ParseError`, nunca derrubado em
//         silêncio nem passado cru adiante. Vale TANTO pra conteúdo `Text` QUANTO pra VALORES de
//         atributo (tratamento padrão de character-data XML cobre os dois) -- NÃO pro payload
//         opaco do `<head>`, que o próprio último parágrafo da seção 4 do uix-dom.md isenta
//         explicitamente (CSS não tem sintaxe de entidade; decodificar corromperia).
//       - Opacidade de `<head>` (uix-dom.md seção 4): capturada via um SCAN DE BYTE CRU sobre o
//         buffer-fonte original procurando a sequência literal `</head` (caixa dobrada) +
//         whitespace* + `>` -- deliberadamente NÃO reusando a própria lógica de skip
//         ciente-de-tag deste módulo sobre o interior do `<head>`. Uma abordagem genérica de
//         "pular tags aninhadas com o Lexer compartilhado" tornaria a opacidade do `<head>`
//         contingente ao conteúdo dele por acaso tokenizar como gramática válida de tag/texto
//         (verdade pra toda fixture do corpus de hoje, pelo lexer_corpus_sanity, mas NÃO
//         garantido -- um '<' literal, não-escapado, dentro de CSS/JS real, ex. um seletor de
//         atributo `a[href^="<"]`, descarrilaria um skip ciente-de-tag e transformaria "payload
//         opaco" num `Error` espúrio de lexer). O scan cru não tem essa dependência: é uma busca
//         burra de byte, batendo exatamente com a própria formulação da seção 4 do uix-dom.md do
//         payload como "bytes-fonte exatamente como escritos", genuinamente não-parseado.
//         Consequência: a instância `Lexer` deste módulo é DESCARTADA e RECONSTRUÍDA uma vez,
//         sobre a substring começando logo após o `>` de fechamento do `</head>`, no momento em
//         que um `<head>` real é achado (ver o próprio comentário de cabeçalho do parser.cpp pra
//         como o relato de offset-absoluto de erro sobrevive a essa reconstrução).
//       - Casamento de NOME entre tag de abertura e fechamento (`<div>...</span>`): uma pilha
//         simples, uma entrada por elemento atualmente aberto, implícita na própria pilha de
//         chamada C++ deste parser (recursivo-descendente -- cada elemento aninhado é mais um
//         frame de pilha) em vez de um `std::vector<std::string>` explícito. Um `TagClose` cujo
//         nome (caixa dobrada) não casa com o nome (caixa dobrada) do elemento sendo fechado
//         agora é um `ParseError` -- o próprio `test_mismatched_close_name_not_validated` do
//         `lexer_tokens_sanity.cpp` provou que o LEXER aceita isto; este módulo é onde para de
//         ser aceito.
//       - Dobra de caixa de nome de tag (uix-dom.md seção 8, evidenciada contra
//         `XMLParser::HandleElementStart`/`XMLParser.cpp:136,167`): TANTO o nome de abertura
//         QUANTO o de fechamento são dobrados pra ASCII-minúsculo antes de guardados
//         (`Element::tag()`) ou comparados -- isto é comportamento real de tempo-de-construção-
//         de-árvore do RmlUi upstream, não um cosmético de tempo-de-dump, então `<DIV>...</div>`
//         genuinamente fecha (os dois dobram pra `"div"`) e o `Element::tag()` resultante é
//         `"div"`, nunca `"DIV"`. Nomes de atributo, valores de `id` e tokens de `class` NÃO são
//         dobrados (mesma seção, nenhuma evidência equivalente achada pra eles).
//       - Filtragem de existência de texto só-whitespace: NÃO é trabalho deste módulo -- o próprio
//         `Element::append_child` da S2 já aplica a seção 6(a) do uix-dom.md como INVARIANTE DE
//         ÁRVORE (ver o comentário de cabeçalho do `dom_tree.hpp`, ponto (1)). Este parser
//         constrói um nó `Text` pra TODO token `Text` que decodifica (só-whitespace ou não) e só
//         chama `append_child`, confiando na árvore pra decidir se sobrevive -- exatamente o
//         contrato que o `dom_tree.hpp` documenta pros próprios chamadores.
//
//     GRAMÁTICA DE TOPO DE DOCUMENTO QUE ESTE PARSER APLICA (100% apoiada em corpus -- toda uma
//     das 44 fixtures `.rml` reais sob `glintfx/tests/`+`glintfx/demos/`, mais a fixture
//     `battle_cockpit_rml.cpp` do GusWorld que esta fatia soma, embrulha em `<rml>`):
//       documento := (Comentario | Texto-só-whitespace)*
//                    '<rml' atributos (TagSelfClose | TagOpenEnd corpo-rml '</rml>')
//                    (Comentario | Texto-só-whitespace)* FimDeArquivo
//       corpo-rml := (Comentario | Texto-só-whitespace | head-uma-vez | body-uma-vez)*
//       (head, se presente, PRECISA preceder body; no máximo um de cada; nada mais -- nenhum
//       elemento perdido, nenhum texto não-whitespace -- é filho direto de `<rml>`; um documento
//       sem `<body>` nenhum é `ParseError`, batendo com a própria "um documento sem `<body>`
//       nenhum é erro de parse, fora do escopo deste formato" da seção 3 do uix-dom.md.)
//     Isto é DELIBERADAMENTE MAIS ESTRITO do que precisaria ser só pra construir uma árvore -- toda
//     uma destas regras é fail-high pela própria disciplina "rejeita, reporta, nunca chuta uma
//     transformação que o corpus nunca pediu" deste projeto (ver a própria lista "Construções
//     rejeitadas" deste arquivo no comentário de cabeçalho do parser.cpp, e a própria cláusula de
//     fronteira-congelada do `docs/rmlx-subset.md`: uma fixture real que precise de algo fora
//     desta gramática para o parser na hora com um `ParseError` diagnosticável, nunca um chute em
//     silêncio).
//
//     APLICAÇÃO DE ATRIBUTO -- política de atributo duplicado (NÃO exercitada por corpus, nenhuma
//     fixture tem dois `id=`/dois `class=` na mesma tag; esta é escolha própria e explícita desta
//     fatia, documentada, não um chute deixado implícito): a ÚLTIMA ocorrência vence pra `id` e
//     pra `class` (o VALOR INTEIRO do atributo class é substituído pela última ocorrência, tokens
//     de um `class=` anterior na mesma tag NÃO são mesclados) -- semântica comum de DOM/mapa-de-
//     atributo, um nome mapeia pra exatamente um valor, última-na-ordem-fonte é autoritativa. Todo
//     OUTRO nome de atributo já ganha isto de graça da própria semântica de sobrescrita de
//     `std::map` do `Element::set_attribute` quando este parser aplica atributos em ordem-fonte.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "uix/dom/dom_tree.hpp"

namespace glintfx::uix {

// EN: `line`/`column` are 1-based, computed against the ORIGINAL source buffer (never a
//     post-`<head>`-rebuild substring's own coordinates -- see parser.cpp's header comment for
//     how this is guaranteed). `offset` is the matching 0-based absolute BYTE offset, provided
//     alongside line/column for tooling that wants to slice the source directly.
// PT: `line`/`column` são 1-based, computados contra o buffer-fonte ORIGINAL (nunca as
//     coordenadas próprias de uma substring pós-reconstrução-de-`<head>` -- ver o comentário de
//     cabeçalho do parser.cpp pra como isto é garantido). `offset` é o offset absoluto de BYTE
//     0-based correspondente, fornecido junto de line/column pra ferramental que queira fatiar a
//     fonte direto.
struct ParseError {
  std::string message;
  std::size_t line = 0;
  std::size_t column = 0;
  std::size_t offset = 0;
};

// EN: `document` is non-null if and only if `error` is `std::nullopt`. On failure, whatever
//     partial tree existed at the point of failure is discarded (not returned) -- a caller that
//     gets an `error` has no tree to inspect, matching this project's fail-high discipline (a
//     PARTIAL, wrong tree that "still has something in it" is worse than an obvious, total
//     failure -- same reasoning `lexer.hpp`'s own sticky-`Error` paragraph already gives one layer
//     down).
// PT: `document` é não-nulo se e somente se `error` for `std::nullopt`. Na falha, qualquer árvore
//     parcial que existia no ponto da falha é descartada (não retornada) -- um chamador que recebe
//     um `error` não tem árvore nenhuma pra inspecionar, batendo com a disciplina fail-high deste
//     projeto (uma árvore PARCIAL e errada que "ainda tem algo dentro" é pior que uma falha óbvia
//     e total -- mesmo raciocínio que o próprio parágrafo de `Error` pegajoso do `lexer.hpp` já dá
//     uma camada abaixo).
struct ParseResult {
  std::unique_ptr<Document> document;
  std::optional<ParseError> error;
};

// EN: Parses `source` (a complete, in-memory RML document -- see this file's header comment for
//     the exact top-level grammar enforced) into a `Document`. `source` only needs to stay alive
//     for the duration of this call -- unlike `Lexer`'s own zero-copy `Token` contract, the
//     resulting `Document`'s `Element`/`Text` nodes own their OWN `std::string` copies of every
//     string they hold (tag/id/class/attribute-name/attribute-value/text-content), because entity
//     decoding (`&amp;` -> `&`) can never be a view into the original bytes -- the decoded content
//     is not a substring of the source at all once an entity has been expanded. `ParseError`,
//     on failure, is the one exception: fully self-contained (a `std::string` message, not a
//     view), so it too outlives `source` safely.
// PT: Parseia `source` (um documento RML completo, em memória -- ver o comentário de cabeçalho
//     deste arquivo pra gramática de topo exata aplicada) num `Document`. `source` só precisa
//     ficar viva pela duração desta chamada -- diferente do próprio contrato zero-cópia de
//     `Token` do `Lexer`, os nós `Element`/`Text` do `Document` resultante têm posse das PRÓPRIAS
//     cópias `std::string` de toda string que carregam (tag/id/class/nome-de-atributo/valor-de-
//     atributo/conteúdo-de-texto), porque decodificação de entidade (`&amp;` -> `&`) nunca pode
//     ser uma view sobre os bytes originais -- o conteúdo decodificado não é uma substring da
//     fonte nenhuma, uma vez que uma entidade foi expandida. `ParseError`, na falha, é a única
//     exceção: totalmente autocontido (uma mensagem `std::string`, não uma view), então também
//     sobrevive a `source` com segurança.
ParseResult parse_document(std::string_view source);

} // namespace glintfx::uix
