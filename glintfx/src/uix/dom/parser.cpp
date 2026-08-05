// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S3 -- implementation. See parser.hpp's own header comment for the full scope/
//     boundary/design-decision rationale this file holds itself to; the comments here are about
//     HOW, not WHY (the WHY already lives in the header, once, not duplicated per function --
//     same discipline lexer.cpp/dom_tree.cpp already follow for their own headers).
//
//     ABSOLUTE-OFFSET SURVIVAL ACROSS THE <head> LEXER REBUILD: `source_` (this file's `Parser`
//     class) always holds the ORIGINAL, full, offset-0 source buffer -- never a substring -- for
//     the entire lifetime of a parse. Two independent mechanisms keep every reported
//     `ParseError::offset` absolute against THAT buffer, even after `lexer_` is rebuilt once (over
//     a substring starting past `</head>`) per parser.hpp's own header comment:
//       (1) Errors derived from a TOKEN's numeric `.offset` field (which IS relative to whatever
//           `std::string_view` the CURRENT `lexer_` was constructed over) go through `abs_offset_`
//           -- 0 before any rebuild, set once to the rebuild's resume point after it. Every call
//           site that reports a raw `Token::offset` adds `abs_offset_`.
//       (2) Errors derived from a TOKEN's `.text`/`.value` SPAN (e.g. non-whitespace text
//           directly inside `<rml>`, `abs_offset_of_span(tok.text)` below) bypass `abs_offset_`
//           entirely and use POINTER ARITHMETIC against `source_` instead
//           (`span.data() - source_.data()`) -- correct regardless of which `lexer_` generation
//           produced the span, because `std::string_view::substr` never copies: every `Token`'s
//           `text`/`value`, from either lexer generation, is still a view into the SAME
//           underlying character array `source_` itself points at. UIX-ENTITY-PARIDADE (2026-08)
//           removed this mechanism's ORIGINAL motivating case -- entity-decoding failures --
//           because decode_entities() can no longer fail (see its own header comment); the
//           mechanism itself stays, for the other span-relative error this file still reports.
//
//     🔴 KNOWN GAP (found while corpus-testing this slice, NOT fixed here -- see
//     parser_hardening_sanity.cpp's own `test_known_gap_lexer_cannot_tokenize_head_style_with_
//     stray_lt` for the explicit, always-run pin, and parser_corpus_sanity.cpp's header for why
//     the fixture that exposed it lives outside lexer_corpus_sanity.cpp's scan): the shared
//     `Lexer` (lexer.cpp, S1's file, not this slice's to edit), run STANDALONE over a whole
//     document, cannot tokenize `<style>`/`<script>` content containing a literal, un-escaped run
//     of two or more '<' -- real upstream RmlUi tolerates this via a TOKENIZER-level CDATA-tag
//     concept for exactly those two tag names (`Factory.cpp:256-257`,
//     `BaseXMLParser::ReadCDATA`) that `lexer.cpp` does not implement (by design -- it has no
//     tag-name awareness at all). THIS module is NOT affected in practice: every real fixture's
//     `<style>` sits inside `<head>`, and `handle_head()` below never re-tokenizes `<head>`'s
//     interior with the Lexer at all (see the "<head> opacity" paragraph in parser.hpp's header).
//     A hypothetical `<style>`/`<script>` OUTSIDE `<head>` (zero corpus occurrences) would still
//     hit this gap through THIS parser too, since `parse_element_contents` has no CDATA-tag
//     handling of its own -- flagged here, not guessed at, per this project's "the corpus decides"
//     discipline.
// PT: RMLX-1/S3 -- implementação. Ver o próprio comentário de cabeçalho do parser.hpp pro
//     escopo/fronteira/racional-de-decisão-de-desenho completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função -- mesma disciplina que lexer.cpp/dom_tree.cpp já seguem pros próprios
//     headers deles).
//
//     SOBREVIVÊNCIA DE OFFSET-ABSOLUTO ATRAVÉS DA RECONSTRUÇÃO DE LEXER DO <head>: `source_`
//     (classe `Parser` deste arquivo) sempre guarda o buffer-fonte ORIGINAL, completo, offset-0 --
//     nunca uma substring -- pela vida inteira de um parse. Dois mecanismos independentes mantêm
//     todo `ParseError::offset` relatado absoluto contra AQUELE buffer, mesmo depois do `lexer_`
//     ser reconstruído uma vez (sobre uma substring começando depois do `</head>`) pelo próprio
//     comentário de cabeçalho do parser.hpp:
//       (1) Erros derivados do campo numérico `.offset` de um TOKEN (que É relativo a qualquer
//           `std::string_view` sobre o qual o `lexer_` ATUAL foi construído) passam por
//           `abs_offset_` -- 0 antes de qualquer reconstrução, setado uma vez pro ponto de retomada
//           da reconstrução depois dela. Todo call site que relata um `Token::offset` cru soma
//           `abs_offset_`.
//       (2) Erros derivados do SPAN `.text`/`.value` de um TOKEN (ex.: texto não-whitespace
//           direto dentro de `<rml>`, `abs_offset_of_span(tok.text)` abaixo) contornam
//           `abs_offset_` por completo e usam ARITMÉTICA DE PONTEIRO contra `source_` em vez
//           disso (`span.data() - source_.data()`) -- correto independente de qual geração de
//           `lexer_` produziu o span, porque `std::string_view::substr` nunca copia: todo
//           `text`/`value` de `Token`, de qualquer geração de lexer, ainda é uma view sobre o
//           MESMO array de caractere subjacente que o próprio `source_` aponta.
//           UIX-ENTITY-PARIDADE (2026-08) removeu o caso motivador ORIGINAL deste mecanismo --
//           falhas de decodificação de entidade -- porque o decode_entities() não pode mais
//           falhar (ver o próprio comentário de cabeçalho dela); o mecanismo em si fica, pro
//           outro erro relativo-a-span que este arquivo ainda relata.
//
//     🔴 LACUNA CONHECIDA (achada testando corpus nesta fatia, NÃO consertada aqui -- ver o
//     próprio `test_known_gap_lexer_cannot_tokenize_head_style_with_stray_lt` do
//     parser_hardening_sanity.cpp pro pin explícito, sempre-rodado, e o cabeçalho do
//     parser_corpus_sanity.cpp pro porquê da fixture que a expôs morar fora do scan do
//     lexer_corpus_sanity.cpp): o `Lexer` compartilhado (lexer.cpp, arquivo da S1, não desta
//     fatia pra editar), rodando STANDALONE sobre um documento inteiro, não consegue tokenizar
//     conteúdo de `<style>`/`<script>` contendo um trecho literal, não-escapado, de dois ou mais
//     '<' -- o RmlUi upstream real tolera isto via um conceito de tag-CDATA em nível de
//     TOKENIZADOR pra exatamente esses dois nomes de tag (`Factory.cpp:256-257`,
//     `BaseXMLParser::ReadCDATA`) que o `lexer.cpp` não implementa (por desenho -- não tem
//     consciência de nome-de-tag nenhuma). ESTE módulo NÃO é afetado na prática: o `<style>` de
//     toda fixture real fica dentro de `<head>`, e o `handle_head()` abaixo nunca retokeniza o
//     interior de `<head>` com o Lexer nenhuma vez (ver o parágrafo "opacidade de <head>" do
//     cabeçalho do parser.hpp). Um `<style>`/`<script>` hipotético FORA de `<head>` (zero
//     ocorrências no corpus) ainda bateria nesta lacuna através DESTE parser também, já que
//     `parse_element_contents` não tem tratamento de tag-CDATA próprio nenhum -- sinalizado aqui,
//     não chutado, pela própria disciplina "o corpus decide" deste projeto.
//
//     REJECTED CONSTRUCTIONS, and how each manifests (full list for this slice's own report):
//       - DOCTYPE / XML declaration / CDATA -- lexer `Error` ("malformed '<'..."), propagated.
//       - Namespaced names (`xml:lang`) -- lexer `Error` (':' not in the name-char class, breaks
//         attribute-grammar scanning), propagated.
//       - Unquoted attribute value -- lexer `Error` ("expected a quoted value"), propagated.
//       - Literal '<' inside a quoted attribute value -- lexer `Error`, propagated.
//       - Unterminated quote/comment/tag, and either byte ceiling (`kMaxInputBytes`,
//         `kMaxTokenBytes`) -- lexer `Error`, propagated.
//       - Mismatched opening/closing tag NAME (`<div>...</span>`) -- THIS file's own
//         `ParseError` (the lexer accepts the token; this parser is where matching happens).
//       - Nesting past `kMaxElementDepth` (256) -- THIS file's own `ParseError` (checked BEFORE
//         recursing one level deeper, so a hostile 1M-deep input never grows this parser's own
//         C++ call stack past the ceiling either).
//       - Unclosed element / EOF mid-document (any open element, or an unterminated `<head>` with
//         no `</head>` anywhere in the rest of the source) -- THIS file's own `ParseError`.
//       - Missing top-level `<rml>` wrapper (any other top-level tag name, or no tag at all) --
//         THIS file's own `ParseError`.
//       - Missing `<body>` anywhere inside `<rml>` -- THIS file's own `ParseError` (uix-dom.md
//         section 3's own "a document missing `<body>` entirely is a parse error").
//       - Duplicate `<head>`, duplicate `<body>`, `<head>` appearing after `<body>`, `<head>`
//         nested anywhere other than a direct child of `<rml>` -- THIS file's own `ParseError`.
//       - Any element other than `<head>`/`<body>` as a direct child of `<rml>`, or non-
//         whitespace text directly inside `<rml>` -- THIS file's own `ParseError`.
//       - Trailing content (anything but whitespace/comments) after `</rml>` -- THIS file's own
//         `ParseError`.
//       - 🔴 REMOVED (UIX-ENTITY-PARIDADE, 2026-08): entity syntax used to be here -- an
//         unrecognised named entity, a bare '&' never closed by a terminating ';', or an
//         out-of-range numeric character reference. NONE of these are rejected anymore;
//         decode_entities() (below) can no longer fail at all, matching upstream's own
//         `StringUtilities::DecodeRml`, which never fails either -- see that function's own
//         header comment for the full construction-by-construction proof. A real fixture
//         (`test_fixtures/system_menu__config_controles_tabela.rml`, a bare literal '&' in a
//         label) is what exposed this parser rejecting a document RmlUi loads fine.
//       - `<style>`/`<script>` content with a literal, un-escaped run of two or more '<' -- see
//         the "known gap" paragraph above (only reachable outside `<head>`, zero corpus
//         occurrences; inside `<head>`, this parser is immune).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/dom/parser.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "uix/dom/lexer.hpp"

namespace glintfx::uix {

namespace {

// EN: Same 4-character whitespace set lexer.cpp's/dom_tree.cpp's own predicates use -- see
//     dom_tree.cpp's own comment on this exact duplication for why it is re-declared here rather
//     than shared via a common header (this is now the THIRD place; still smaller than the
//     indirection a one-boolean-predicate header would add).
// PT: O MESMO conjunto de 4 caracteres de whitespace que os próprios predicados do lexer.cpp/
//     dom_tree.cpp usam -- ver o próprio comentário do dom_tree.cpp sobre esta mesma duplicação
//     pra por que é redeclarado aqui em vez de compartilhado via header comum (este é agora o
//     TERCEIRO lugar; ainda menor que a indireção que um header de um-único-predicado-booleano
//     somaria).
constexpr bool is_whitespace_char(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_whitespace_only(std::string_view s) {
  return std::all_of(s.begin(), s.end(), is_whitespace_char);
}

char ascii_lower_char(char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// EN: ASCII-only lowercase fold -- uix-dom.md section 8, evidenced against
//     `StringUtilities::ToLower`/`XMLParser.cpp:136,167` (see parser.hpp header). Safe byte-wise
//     without a UTF-8 decoder for the same reason lexer.cpp's own scanning is: every UTF-8
//     multi-byte lead/continuation byte (0x80-0xFF) is numerically disjoint from 'A'-'Z'
//     (0x41-0x5A).
// PT: Dobra pra minúscula, só-ASCII -- uix-dom.md seção 8, evidenciada contra
//     `StringUtilities::ToLower`/`XMLParser.cpp:136,167` (ver cabeçalho do parser.hpp). Seguro
//     byte-a-byte sem decodificador UTF-8 pelo mesmo motivo que o próprio scanning do lexer.cpp é:
//     todo byte líder/continuação multi-byte UTF-8 (0x80-0xFF) é numericamente disjunto de 'A'-'Z'
//     (0x41-0x5A).
std::string to_ascii_lower(std::string_view s) {
  std::string out(s);
  std::transform(out.begin(), out.end(), out.begin(), ascii_lower_char);
  return out;
}

// EN: Splits `s` on runs of the 4-char whitespace set, dropping empty tokens -- uix-dom.md
//     section 7's own class-token split, applied here at PARSE time (the tree just receives
//     already-split tokens via `Element::add_class`, see dom_tree.hpp's own header comment,
//     point on `add_class`'s single-token precondition).
// PT: Divide `s` em runs do conjunto de 4 caracteres de whitespace, descartando tokens vazios --
//     o próprio split de token de classe da seção 7 do uix-dom.md, aplicado aqui em tempo de
//     PARSE (a árvore só recebe tokens já separados via `Element::add_class`, ver o próprio
//     comentário de cabeçalho do dom_tree.hpp, ponto sobre a pré-condição de token-único do
//     `add_class`).
std::vector<std::string_view> split_whitespace(std::string_view s) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && is_whitespace_char(s[i])) {
      ++i;
    }
    std::size_t start = i;
    while (i < s.size() && !is_whitespace_char(s[i])) {
      ++i;
    }
    if (i > start) {
      out.push_back(s.substr(start, i - start));
    }
  }
  return out;
}

// EN: 1-based line/column for `offset` into `source` -- O(offset), called at most once per parse
//     (on failure only), so this is not a hot path worth indexing newlines up front for.
// PT: Linha/coluna 1-based pra `offset` dentro de `source` -- O(offset), chamado no máximo uma
//     vez por parse (só na falha), então não é um caminho quente que valha indexar quebras de
//     linha antecipadamente.
std::pair<std::size_t, std::size_t> compute_line_col(std::string_view source, std::size_t offset) {
  const std::size_t off = offset > source.size() ? source.size() : offset;
  std::size_t line = 1;
  std::size_t line_start = 0;
  for (std::size_t i = 0; i < off; ++i) {
    if (source[i] == '\n') {
      ++line;
      line_start = i + 1;
    }
  }
  return {line, off - line_start + 1};
}

// EN: UTF-8-encodes a Unicode scalar value. `std::nullopt` for NUL (disallowed XML char data), a
//     UTF-16 surrogate (0xD800-0xDFFF, never a valid standalone codepoint), or anything above the
//     Unicode maximum (0x10FFFF) -- see parser.hpp header, entity-decoding paragraph.
// PT: Codifica em UTF-8 um valor escalar Unicode. `std::nullopt` pra NUL (character-data XML não
//     permite), um substituto UTF-16 (0xD800-0xDFFF, nunca um codepoint autônomo válido), ou
//     qualquer coisa acima do máximo Unicode (0x10FFFF) -- ver cabeçalho do parser.hpp, parágrafo
//     de decodificação de entidade.
std::optional<std::string> utf8_encode(std::uint32_t cp) {
  if (cp == 0 || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
    return std::nullopt;
  }
  std::string s;
  if (cp <= 0x7F) {
    s.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
  return s;
}

// EN: The fixed named-entity table -- UIX-ENTITY-PARIDADE (2026-08, RMLX-1) narrowed this to
//     EXACTLY the 4 names upstream's own `StringUtilities::DecodeRml` recognises
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:136,142,148,154 -- four literal
//     character-by-character checks, `'l'`/`'g'`/`'a'`/`'q'` right after the '&', nothing else).
//     Both `apos` AND `nbsp` were in this table until this slice; BOTH REMOVED, because DecodeRml
//     has no branch for either:
//       - `apos`'s first character 'a' only ever matches the START of the `amp` check
//         (StringUtilities.cpp:148, `s[i+1]=='a'`), which then requires `'m'` at the next
//         position; `apos`'s second character is `'p'`, so the check fails.
//       - `nbsp`'s first character 'n' matches none of `'l'/'g'/'a'/'q'` at all.
//     Either way DecodeRml falls through to its own final `result += s[i]; i += 1;`
//     (StringUtilities.cpp:214-215) -- `&apos;`/`&nbsp;` are copied through as their own literal
//     6-byte text, never decoded to `'`/U+00A0. This module's own decode_entities() now
//     reproduces that exact fall-through (see its header comment below), so removing both names
//     from this table is enough -- no special-casing needed elsewhere.
//
//     🔴 `nbsp`'S REMOVAL HAS A HISTORY WORTH KEEPING VISIBLE, NOT JUST STATING THE OUTCOME: this
//     slice's OWN first pass argued to KEEP `nbsp` decoding -- docs/uix-dom.md section 6c (the
//     canonical spec) and glintfx/tests/uix/dumper_sanity.cpp (a concurrent agent's own file, not
//     this module's to touch) both asserted `&nbsp;` decodes to U+00A0 at the time, and flipping
//     it here alone would have silently diverged from BOTH of them mid-flight while that other
//     agent's work was still in progress -- so this slice deliberately shipped `nbsp` decoding
//     UNCHANGED and flagged the finding to the orchestrator/líder instead of resolving it
//     unilaterally. That other agent then independently re-verified section 6c against a LIVE run
//     of RmlUi (`S6a`, per that section's own 2026-08-05 correction) and confirmed `&nbsp;`
//     survives as its own six literal ASCII bytes upstream, never U+00A0 -- the spec itself was
//     wrong, not just this table. This table now matches the corrected spec. The general
//     principle this episode reinforces: an isolated-looking table entry that also lives in a
//     canonical cross-module doc, exercised by ANOTHER agent's concurrent tests, is not this
//     slice's call to flip alone even when the evidence points the same way -- coordinate first.
//
//     `std::nullopt` for anything else -- an UNRECOGNISED name is not this table's job to guess
//     at, it is decode_entities()'s own job to fall back on (no longer a rejection -- see below).
// PT: A tabela fixa de entidade nomeada -- UIX-ENTITY-PARIDADE (2026-08, RMLX-1) estreitou isto
//     pros EXATOS 4 nomes que o próprio `StringUtilities::DecodeRml` do upstream reconhece
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:136,142,148,154 -- quatro checagens de
//     caractere-literal, `'l'`/`'g'`/`'a'`/`'q'` logo após o '&', nada mais). Tanto `apos` QUANTO
//     `nbsp` estavam nesta tabela até esta fatia; AMBOS REMOVIDOS, porque o DecodeRml não tem
//     ramo nenhum pra nenhum dos dois:
//       - o primeiro caractere de `apos`, 'a', só casa o INÍCIO da checagem de `amp`
//         (StringUtilities.cpp:148, `s[i+1]=='a'`), que então exige `'m'` na posição seguinte; o
//         segundo caractere de `apos` é `'p'`, então a checagem falha.
//       - o primeiro caractere de `nbsp`, 'n', não casa nenhum de `'l'/'g'/'a'/'q'` de jeito
//         nenhum.
//     Dos dois jeitos o DecodeRml cai pro próprio `result += s[i]; i += 1;` final
//     (StringUtilities.cpp:214-215) -- `&apos;`/`&nbsp;` são copiados como o próprio texto
//     literal de 6 bytes, nunca decodificados pra `'`/U+00A0. O próprio decode_entities() deste
//     módulo agora reproduz esse mesmo fall-through (ver o comentário de cabeçalho dele abaixo),
//     então remover os dois nomes desta tabela basta -- nenhum caso especial precisa em lugar
//     nenhum.
//
//     🔴 A REMOÇÃO DE `nbsp` TEM UM HISTÓRICO QUE VALE MANTER VISÍVEL, NÃO SÓ AFIRMAR O
//     DESFECHO: a PRIMEIRA passada desta própria fatia argumentou por MANTER `nbsp`
//     decodificando -- a seção 6c do docs/uix-dom.md (a spec canônica) e o
//     glintfx/tests/uix/dumper_sanity.cpp (arquivo de um agente concorrente, não deste módulo pra
//     tocar) ambos afirmavam, na época, que `&nbsp;` decodifica pra U+00A0, e inverter isto aqui
//     sozinho teria divergido em silêncio dos DOIS no meio do trabalho ainda em curso daquele
//     outro agente -- então esta fatia deliberadamente entregou a decodificação de `nbsp`
//     INALTERADA e sinalizou o achado ao orquestrador/líder em vez de resolver sozinha. Aquele
//     outro agente então reverificou a seção 6c independentemente contra uma rodada AO VIVO do
//     RmlUi (`S6a`, pela própria correção de 2026-08-05 daquela seção) e confirmou que `&nbsp;`
//     sobrevive como os próprios seis bytes ASCII literais no upstream, nunca U+00A0 -- a própria
//     spec estava errada, não só esta tabela. Esta tabela agora bate com a spec corrigida. O
//     princípio geral que este episódio reforça: uma entrada de tabela que parece isolada mas
//     também mora numa doc canônica cross-módulo, exercitada pelos testes concorrentes de OUTRO
//     agente, não é decisão desta fatia inverter sozinha mesmo quando a evidência aponta pro
//     mesmo lugar -- coordenar primeiro.
//
//     `std::nullopt` pra qualquer outra coisa -- um nome NÃO-RECONHECIDO não é trabalho desta
//     tabela chutar, é trabalho do próprio decode_entities() cair pro fallback (não é mais uma
//     rejeição -- ver abaixo).
std::optional<std::string> decode_named_entity(std::string_view name) {
  if (name == "amp") return std::string("&");
  if (name == "lt") return std::string("<");
  if (name == "gt") return std::string(">");
  if (name == "quot") return std::string("\"");
  return std::nullopt;
}

// EN: Parses a numeric character reference BODY (the part between '#' and ';', so either
//     "38"/decimal or "x26"/"X26" hex -- the leading '#' itself already stripped by the caller).
//     Bounded digit count (kMaxDigits) avoids any need for overflow-checked bignum arithmetic:
//     0x10FFFF is 7 hex digits / 7 decimal digits, so 8 is a generous, still-tiny ceiling that
//     makes every accepted value fit comfortably in a std::uint32_t before the codepoint-range
//     check in utf8_encode() even runs.
// PT: Parseia o CORPO de uma referência numérica de caractere (a parte entre '#' e ';', ou
//     "38"/decimal ou "x26"/"X26" hex -- o '#' líder já removido por quem chama). Contagem de
//     dígito limitada (kMaxDigits) evita qualquer necessidade de aritmética bignum com checagem
//     de overflow: 0x10FFFF são 7 dígitos hex / 7 dígitos decimais, então 8 é um teto generoso e
//     ainda minúsculo que faz todo valor aceito caber confortavelmente num std::uint32_t antes da
//     checagem de faixa de codepoint no utf8_encode() sequer rodar.
std::optional<std::uint32_t> parse_numeric_ref(std::string_view body) {
  constexpr std::size_t kMaxDigits = 8;
  bool hex = false;
  std::string_view digits = body;
  if (!digits.empty() && (digits[0] == 'x' || digits[0] == 'X')) {
    hex = true;
    digits = digits.substr(1);
  }
  if (digits.empty() || digits.size() > kMaxDigits) {
    return std::nullopt;
  }
  std::uint32_t value = 0;
  for (char c : digits) {
    int digit;
    if (c >= '0' && c <= '9') {
      digit = c - '0';
    } else if (hex && c >= 'a' && c <= 'f') {
      digit = 10 + (c - 'a');
    } else if (hex && c >= 'A' && c <= 'F') {
      digit = 10 + (c - 'A');
    } else {
      return std::nullopt;
    }
    value = value * (hex ? 16u : 10u) + static_cast<std::uint32_t>(digit);
  }
  return value;
}

// EN: Entity-decodes `raw` (a lexer Text/Attr-value payload, entities NOT yet decoded) to match
//     upstream's own `StringUtilities::DecodeRml`
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:128-218) BYTE FOR BYTE for every
//     construction this module's real corpus exercises -- UIX-ENTITY-PARIDADE (2026-08, RMLX-1).
//
//     THE PARITY BREAK THIS SLICE FOUND AND CLOSES: this function used to be FAIL-HIGH (a
//     `ParseError`) for a bare '&' not closed by ';', or a "'&'...';'" span whose body did not
//     decode. Upstream's DecodeRml NEVER fails -- there is no error return at all in its own
//     signature (`String DecodeRml(const String&)`), and its four fixed literal-character checks
//     (`&lt;`/`&gt;`/`&amp;`/`&quot;`, StringUtilities.cpp:136,142,148,154) either match and
//     decode, or the WHOLE function falls through to its own final
//     `result += s[i]; i += 1;` (StringUtilities.cpp:214-215) -- copying the SINGLE '&' byte
//     literally and resuming scanning right after it, never consuming whatever text happened to
//     follow. A real fixture GusWorld measured
//     (glintfx/src/uix/dom/test_fixtures/system_menu__config_controles_tabela.rml, a rótulo
//     "Interagir & Diálogo" with a bare literal '&', no entity intended at all) loads fine in
//     RmlUi today and would have failed to load in THIS parser at the old fail-high boundary.
//     This function now reproduces that exact fall-through: a name outside {amp,lt,gt,quot} (see
//     decode_named_entity()'s own header -- `apos`/`nbsp` are BOTH gone from that table now, no
//     exceptions), a bare '&' with no ';' anywhere in the bounded scan window below, a malformed
//     numeric body, or a numeric codepoint utf8_encode() itself refuses (see that function's own
//     header) all take the SAME fallback: emit '&' alone, resume scanning at `amp_pos + 1` --
//     NEVER at the matched `;` this function's own bounded scan found (that `;`, if any, may
//     belong to an entity of its OWN starting mid-body -- e.g. `&foo&amp;bar;`: DecodeRml's
//     per-character resumption reaches the real `&amp;` and decodes it to `"&foo&bar;"`; jumping
//     past the scanned span instead would swallow it, a genuine behavioural bug this exact
//     fallback position avoids). Consequence: this function can no longer fail -- entity syntax
//     is never again a reason for `parse_document()` to reject a document RmlUi would happily
//     load (uix-dom.md's own fail-high posture still applies to every OTHER construction this
//     module rejects; entities are the one place upstream itself never rejects, so parity wins
//     here, per this slice's own brief).
//
//     🔴 `&nbsp;` WAS A KEPT DIVERGENCE IN THIS SLICE'S FIRST PASS -- NO LONGER: this function's
//     first pass deliberately did NOT narrow `nbsp` to match upstream (strict byte-parity means
//     `&nbsp;` falls through as literal text, same as `&apos;`), because at the time
//     docs/uix-dom.md section 6c (the canonical spec) and glintfx/tests/uix/dumper_sanity.cpp (a
//     concurrent agent's own file, not this module's to touch) both asserted `&nbsp;` decodes to
//     U+00A0, and flipping it here alone would have silently diverged from both mid-flight. That
//     other agent independently re-verified section 6c against a LIVE run of RmlUi and confirmed
//     the spec itself was wrong -- `&nbsp;` survives undecoded upstream, same as `&apos;`. Both
//     docs/uix-dom.md and this module now agree: `&nbsp;` is NOT special-cased, it takes the
//     exact same fallback path as any other unrecognised name. See decode_named_entity()'s own
//     header for the full history of this correction.
//
//     NOT BYTE-REPLICATED EITHER, judged acceptable, out of scope: numeric references whose
//     codepoint is a UTF-16 surrogate (0xD800-0xDFFF) or above 0x10FFFF. Upstream's own numeric
//     branch (StringUtilities.cpp:160-211) treats these as "successfully decoded" at the
//     DecodeRml layer (no fallback, `i` advances past the whole reference) and defers the actual
//     byte production to `ToUTF8()` (StringUtilities.cpp:497-538): for a surrogate, ToUTF8 has NO
//     range check at all and emits a real (if technically invalid per RFC 3629) 3-byte UTF-8
//     sequence for it (the unconditional `c < 0x10000` branch); for >0x10FFFF, ToUTF8 sets
//     `invalid_character` and appends NOTHING -- the reference silently vanishes, zero bytes,
//     behind a warning log nothing downstream of DecodeRml's return value can see. (Codepoint 0
//     is NOT part of this divergence -- DecodeRml's own `code_point != 0` guard,
//     StringUtilities.cpp:180/204, routes it to the SAME fall-through this function already
//     replicates, so codepoint 0 IS byte-parity.) This function's own `utf8_encode()` refuses all
//     three (`std::nullopt` for 0, a surrogate, or >0x10FFFF -- see its own header), which this
//     function's generic fallback then renders as the READABLE LITERAL TEXT of the reference
//     (`&#xD800;` stays `&#xD800;`) instead of either upstream outcome. Judged acceptable, NOT
//     upstream-identical, for two reasons: (1) zero corpus occurrence across all 44+15 real
//     fixtures -- nobody hand-authors a raw UTF-16 surrogate or an out-of-Unicode-range numeric
//     reference in game UI markup; (2) the two upstream outcomes this function declines to
//     replicate are THEMSELVES a genuinely invalid UTF-8 byte sequence and a silent content-loss,
//     respectively -- matching them byte-for-byte would mean this module deliberately
//     manufacturing invalid UTF-8 inside `Document`/`Element`/`Text`, an invariant no other part
//     of this DOM violates on purpose. The LOAD-SUCCEEDS/LOAD-FAILS parity this slice's brief
//     cares about most is still preserved either way -- a document with one of these two numeric
//     forms loads in BOTH engines, just with different (and, for this module, always
//     well-formed) bytes for that one reference.
//
//     kMaxEntityScan=32 (the bounded-scan window that finds a candidate `;`) is generously larger
//     than DecodeRml's own effective lookahead (at most ~10 characters: `#` + up to 8 hex digits
//     + `;`, or 6 for the `quot` check) -- this is fine, not a divergence: whenever a candidate
//     body is found but does not decode, OR no `;` exists in the window at all, this function's
//     fallback is identical either way (emit '&' alone), so a wider window than upstream's
//     effective one never changes the observable outcome, it just costs a few more comparisons on
//     a hostile input before reaching the same fallback DecodeRml would have reached much sooner.
// PT: Decodifica entidade em `raw` (um payload Text/valor-de-Attr do lexer, entidades AINDA NÃO
//     decodificadas) pra bater com o próprio `StringUtilities::DecodeRml` do upstream
//     (examples/RmlUi/Source/Core/StringUtilities.cpp:128-218) BYTE A BYTE pra toda construção
//     que o corpus real deste módulo exercita -- UIX-ENTITY-PARIDADE (2026-08, RMLX-1).
//
//     A QUEBRA DE PARIDADE QUE ESTA FATIA ACHOU E FECHA: esta função costumava ser FAIL-HIGH (um
//     `ParseError`) pra um '&' cru não fechado por ';', ou um span "'&'...';'" cujo corpo não
//     decodificava. O DecodeRml do upstream NUNCA falha -- não existe retorno de erro nenhum na
//     própria assinatura dele (`String DecodeRml(const String&)`), e as quatro checagens de
//     caractere-literal fixas dele (`&lt;`/`&gt;`/`&amp;`/`&quot;`,
//     StringUtilities.cpp:136,142,148,154) ou casam e decodificam, ou a função INTEIRA cai pro
//     próprio `result += s[i]; i += 1;` final (StringUtilities.cpp:214-215) -- copiando o byte
//     '&' SOLITÁRIO literalmente e retomando o scan logo depois dele, nunca consumindo o texto
//     que por acaso vinha depois. Uma fixture real que o GusWorld mediu
//     (glintfx/src/uix/dom/test_fixtures/system_menu__config_controles_tabela.rml, um rótulo
//     "Interagir & Diálogo" com um '&' literal cru, entidade nenhuma pretendida) carrega normal
//     no RmlUi hoje e teria falhado ao carregar NESTE parser na antiga fronteira fail-high. Esta
//     função agora reproduz esse mesmo fall-through: um nome fora de {amp,lt,gt,quot} (ver o
//     próprio cabeçalho do decode_named_entity() -- `apos`/`nbsp` estão AMBOS fora dessa tabela
//     agora, nenhuma exceção), um '&' cru sem ';' nenhum na janela de scan limitado abaixo, um
//     corpo numérico malformado, ou um codepoint numérico que o próprio utf8_encode() recusa (ver
//     o cabeçalho dele) tomam TODOS o mesmo fallback: emite só '&', retoma o scan em
//     `amp_pos + 1` -- NUNCA no `;` que o próprio scan limitado desta função achou (aquele `;`,
//     se existir, pode pertencer a uma entidade PRÓPRIA começando no meio do corpo -- ex.:
//     `&foo&amp;bar;`: a retomada por-caractere do DecodeRml alcança o `&amp;` real e decodifica
//     pra `"&foo&bar;"`; pular o span escaneado inteiro em vez disso engoliria ele, um bug de
//     comportamento genuíno que esta posição exata de fallback evita). Consequência: esta função
//     não pode mais falhar -- sintaxe de entidade nunca mais é motivo pro `parse_document()`
//     rejeitar um documento que o RmlUi carregaria feliz (a própria postura fail-high do
//     uix-dom.md segue valendo pra toda OUTRA construção que este módulo rejeita; entidade é o
//     único lugar onde o próprio upstream nunca rejeita, então paridade vence aqui, pelo próprio
//     briefing desta fatia).
//
//     🔴 `&nbsp;` ERA UMA DIVERGÊNCIA MANTIDA NA PRIMEIRA PASSADA DESTA FATIA -- NÃO MAIS: a
//     primeira passada desta função deliberadamente NÃO estreitou `nbsp` pro upstream (paridade
//     estrita de byte significa `&nbsp;` cair como texto literal, igual `&apos;`), porque na
//     época a seção 6c do docs/uix-dom.md (a spec canônica) e o
//     glintfx/tests/uix/dumper_sanity.cpp (arquivo de um agente concorrente, não deste módulo pra
//     tocar) ambos afirmavam que `&nbsp;` decodifica pra U+00A0, e inverter isto aqui sozinho
//     teria divergido em silêncio dos dois no meio do trabalho. Aquele outro agente reverificou a
//     seção 6c independentemente contra uma rodada AO VIVO do RmlUi e confirmou que a própria
//     spec estava errada -- `&nbsp;` sobrevive não-decodificado no upstream, igual `&apos;`.
//     Tanto o docs/uix-dom.md quanto este módulo agora concordam: `&nbsp;` NÃO é caso especial,
//     toma o mesmo caminho de fallback que qualquer outro nome não-reconhecido. Ver o próprio
//     cabeçalho do decode_named_entity() pro histórico completo desta correção.
//
//     TAMBÉM NÃO REPLICADO EM BYTE, julgado aceitável, fora de escopo: referências numéricas cujo
//     codepoint é um substituto UTF-16 (0xD800-0xDFFF) ou acima de 0x10FFFF. O próprio ramo
//     numérico do upstream (StringUtilities.cpp:160-211) trata estes como "decodificados com
//     sucesso" na camada do DecodeRml (sem fallback, `i` avança pela referência inteira) e adia a
//     produção de byte de verdade pro `ToUTF8()` (StringUtilities.cpp:497-538): pra um
//     substituto, o ToUTF8 NÃO TEM checagem de faixa nenhuma e emite uma sequência UTF-8 real (se
//     tecnicamente inválida pela RFC 3629) de 3 bytes pra ele (o ramo incondicional `c <
//     0x10000`); pra >0x10FFFF, o ToUTF8 seta `invalid_character` e não anexa NADA -- a
//     referência desaparece em silêncio, zero bytes, atrás de um log de aviso que nada depois do
//     valor de retorno do DecodeRml consegue ver. (Codepoint 0 NÃO faz parte desta divergência --
//     o próprio guard `code_point != 0` do DecodeRml, StringUtilities.cpp:180/204, roteia ele pro
//     MESMO fall-through que esta função já replica, então codepoint 0 É paridade de byte.) O
//     próprio `utf8_encode()` desta função recusa os três (`std::nullopt` pra 0, um substituto,
//     ou >0x10FFFF -- ver o cabeçalho dele), que o fallback genérico desta função então
//     renderiza como o TEXTO LITERAL LEGÍVEL da referência (`&#xD800;` fica `&#xD800;`) em vez de
//     qualquer um dos dois desfechos do upstream. Julgado aceitável, NÃO idêntico-ao-upstream,
//     por duas razões: (1) zero ocorrência de corpus nas 44+15 fixtures reais -- ninguém escreve
//     à mão um substituto UTF-16 cru ou uma referência numérica fora do range Unicode em markup
//     de UI de jogo; (2) os dois desfechos do upstream que esta função recusa replicar são ELES
//     MESMOS uma sequência UTF-8 genuinamente inválida e uma perda-de-conteúdo silenciosa,
//     respectivamente -- casar eles byte a byte significaria este módulo fabricando UTF-8
//     inválido de propósito dentro de `Document`/`Element`/`Text`, um invariante que nenhuma
//     outra parte deste DOM viola de propósito. A paridade CARREGA/NÃO-CARREGA que o briefing
//     desta fatia mais se importa segue preservada dos dois jeitos -- um documento com uma destas
//     duas formas numéricas carrega nos DOIS motores, só com bytes diferentes (e, pra este
//     módulo, sempre bem-formados) pra aquela referência.
//
//     kMaxEntityScan=32 (a janela de scan limitado que acha um `;` candidato) é generosamente
//     maior que a própria lookahead efetiva do DecodeRml (no máximo ~10 caracteres: `#` + até 8
//     dígitos hex + `;`, ou 6 pra checagem de `quot`) -- isto está OK, não é divergência: sempre
//     que um corpo candidato é achado mas não decodifica, OU nenhum `;` existe na janela nenhuma,
//     o fallback desta função é idêntico dos dois jeitos (emite só '&'), então uma janela mais
//     larga que a efetiva do upstream nunca muda o desfecho observável, só custa umas
//     comparações a mais num input hostil antes de chegar no mesmo fallback que o DecodeRml
//     alcançaria bem antes.
std::string decode_entities(std::string_view raw) {
  constexpr std::size_t kMaxEntityScan = 32;
  std::string out;
  out.reserve(raw.size());
  std::size_t i = 0;
  while (i < raw.size()) {
    if (raw[i] != '&') {
      out.push_back(raw[i]);
      ++i;
      continue;
    }
    const std::size_t amp_pos = i;
    const std::size_t search_end =
        (raw.size() - i > kMaxEntityScan) ? (i + kMaxEntityScan) : raw.size();
    const std::string_view scan_window = raw.substr(i + 1, search_end - (i + 1));
    const std::size_t semi_in_window = scan_window.find(';');
    const std::size_t semi =
        (semi_in_window == std::string_view::npos) ? std::string_view::npos
                                                   : (i + 1 + semi_in_window);
    std::optional<std::string> decoded;
    if (semi != std::string_view::npos) {
      const std::string_view body = raw.substr(i + 1, semi - i - 1);
      if (!body.empty() && body[0] == '#') {
        const std::optional<std::uint32_t> cp = parse_numeric_ref(body.substr(1));
        if (cp.has_value()) {
          decoded = utf8_encode(*cp);
        }
      } else {
        decoded = decode_named_entity(body);
      }
    }
    if (decoded.has_value()) {
      out += *decoded;
      i = semi + 1;
    } else {
      // EN: Fallback matching DecodeRml's own final `result += s[i]; i += 1;`
      //     (StringUtilities.cpp:214-215) -- copy the lone '&' and resume scanning right after it
      //     (`amp_pos + 1`), NOT after whatever ';' the bounded scan above found -- see this
      //     function's own header comment for why that distinction matters (the `&foo&amp;bar;`
      //     case).
      // PT: Fallback batendo com o próprio `result += s[i]; i += 1;` final do DecodeRml
      //     (StringUtilities.cpp:214-215) -- copia o '&' solitário e retoma o scan logo depois
      //     dele (`amp_pos + 1`), NÃO depois de qualquer ';' que o scan limitado acima tenha
      //     achado -- ver o próprio comentário de cabeçalho desta função pro motivo desta
      //     distinção importar (o caso `&foo&amp;bar;`).
      out.push_back('&');
      i = amp_pos + 1;
    }
  }
  return out;
}

// EN: Raw-scans `source` starting at `start` for the literal `</head` (case-folded) + whitespace*
//     + `>` sequence -- see parser.hpp header's own "<head> opacity" paragraph for why this is a
//     DUMB byte search rather than a tag-aware skip. Returns {content_end, resume_offset}:
//     `content_end` is the absolute offset of the '<' that opens the matching `</head>`
//     (uix-dom.md section 4's own exact boundary, "up to (not including) the < that opens
//     </head>"); `resume_offset` is the absolute offset right after that closing tag's '>'.
//     `std::nullopt` if no such sequence exists anywhere in the rest of `source` (unterminated
//     `<head>`).
// PT: Scan cru de byte em `source` começando em `start` procurando a sequência literal `</head`
//     (caixa dobrada) + whitespace* + `>` -- ver o próprio parágrafo "opacidade de <head>" do
//     cabeçalho do parser.hpp pra por que isto é uma busca BURRA de byte em vez de um skip
//     ciente-de-tag. Retorna {content_end, resume_offset}: `content_end` é o offset absoluto do
//     '<' que abre o `</head>` que casa (a própria fronteira exata da seção 4 do uix-dom.md, "até
//     (sem incluir) o < que abre </head>"); `resume_offset` é o offset absoluto logo após o '>'
//     daquela tag de fechamento. `std::nullopt` se nenhuma sequência dessas existir em lugar
//     nenhum do resto de `source` (`<head>` não-terminado).
std::optional<std::pair<std::size_t, std::size_t>> find_head_close(std::string_view source,
                                                                   std::size_t start) {
  constexpr std::string_view kHeadName = "head";
  std::size_t i = start;
  while (i < source.size()) {
    if (source[i] != '<') {
      ++i;
      continue;
    }
    if (i + 1 >= source.size() || source[i + 1] != '/') {
      ++i;
      continue;
    }
    std::size_t j = i + 2;
    bool name_matches = true;
    for (char want : kHeadName) {
      if (j >= source.size() || ascii_lower_char(source[j]) != want) {
        name_matches = false;
        break;
      }
      ++j;
    }
    if (name_matches) {
      std::size_t k = j;
      while (k < source.size() && is_whitespace_char(source[k])) {
        ++k;
      }
      if (k < source.size() && source[k] == '>') {
        return std::make_pair(i, k + 1);
      }
    }
    ++i;
  }
  return std::nullopt;
}

const char* token_kind_name(TokenKind k) {
  switch (k) {
    case TokenKind::TagOpenStart:
      return "an opening tag";
    case TokenKind::Attr:
      return "an attribute";
    case TokenKind::TagOpenEnd:
      return "'>'";
    case TokenKind::TagSelfClose:
      return "'/>'";
    case TokenKind::TagClose:
      return "a closing tag";
    case TokenKind::Text:
      return "text";
    case TokenKind::Comment:
      return "a comment";
    case TokenKind::EndOfFile:
      return "end of file";
    case TokenKind::Error:
      return "a lexer error";
  }
  return "an unknown token";
}

// EN: The recursive-descent implementation itself -- see parser.hpp's header comment for the
//     grammar this class enforces; this class only exists to carry the (small) per-parse state
//     (`source_`, `lexer_`, `abs_offset_`) across the recursive helper methods below without
//     threading them through every call's parameter list.
// PT: A própria implementação recursivo-descendente -- ver o comentário de cabeçalho do
//     parser.hpp pra gramática que esta classe aplica; esta classe só existe pra carregar o
//     estado (pequeno) por-parse (`source_`, `lexer_`, `abs_offset_`) através dos métodos
//     auxiliares recursivos abaixo sem passá-los pela lista de parâmetro de toda chamada.
class Parser {
public:
  explicit Parser(std::string_view source) : source_(source), lexer_(source) {}

  ParseResult parse() {
    auto doc = std::make_unique<Document>();
    ParseError err;

    Token tok = skip_insignificant_top_level(err);
    if (err_set_) {
      return ParseResult{nullptr, err};
    }
    if (tok.kind != TokenKind::TagOpenStart || to_ascii_lower(tok.text) != "rml") {
      return ParseResult{nullptr,
                         error_at("expected a top-level <rml> element, found " +
                                      std::string(token_kind_name(tok.kind)),
                                  abs_offset(tok.offset))};
    }

    std::vector<AttrRaw> rml_attrs;
    bool rml_self_closed = false;
    Token rml_terminator;
    if (!collect_attributes(rml_attrs, rml_self_closed, rml_terminator, err)) {
      return ParseResult{nullptr, err};
    }

    bool saw_body = false;

    if (!rml_self_closed) {
      bool saw_head = false;
      if (!parse_rml_children(*doc, saw_head, saw_body, err)) {
        return ParseResult{nullptr, err};
      }
    }

    if (!saw_body) {
      return ParseResult{nullptr,
                         error_at("document has no <body> element (uix-dom.md section 3: out "
                                  "of this format's scope)",
                                  abs_offset(tok.offset))};
    }

    if (!skip_trailing_content(err)) {
      return ParseResult{nullptr, err};
    }

    return ParseResult{std::move(doc), std::nullopt};
  }

private:
  struct AttrRaw {
    std::string_view name; // view into source_ -- attribute NAMES are never decoded, section 8
    std::string value;     // entity-DECODED, owns its own storage
  };

  Token next_token() { return lexer_.next(); }

  std::size_t abs_offset(std::size_t lexer_relative_offset) const {
    return lexer_relative_offset + abs_offset_;
  }

  // EN: Absolute offset of a `Token::text`/`Token::value` SPAN via pointer arithmetic against
  //     the TRUE original `source_` -- see this file's header comment, mechanism (2).
  // PT: Offset absoluto de um SPAN `Token::text`/`Token::value` via aritmética de ponteiro contra
  //     o `source_` ORIGINAL verdadeiro -- ver o comentário de cabeçalho deste arquivo,
  //     mecanismo (2).
  std::size_t abs_offset_of_span(std::string_view span) const {
    return static_cast<std::size_t>(span.data() - source_.data());
  }

  ParseError error_at(std::string message, std::size_t absolute_offset) const {
    auto [line, col] = compute_line_col(source_, absolute_offset);
    return ParseError{std::move(message), line, col, absolute_offset};
  }

  // EN: `err_set_` lets the top-level `skip_insignificant_top_level` helper distinguish "found a
  //     lexer Error token" (caller must propagate `err`) from "found EndOfFile/some other
  //     non-insignificant token" (caller inspects the returned Token itself) without an
  //     `std::optional<Token>` return type.
  // PT: `err_set_` deixa o helper de topo `skip_insignificant_top_level` distinguir "achou um
  //     token Error de lexer" (chamador precisa propagar `err`) de "achou EndOfFile/algum outro
  //     token não-insignificante" (chamador inspeciona o próprio Token retornado) sem um tipo de
  //     retorno `std::optional<Token>`.
  bool err_set_ = false;

  Token skip_insignificant_top_level(ParseError& err) {
    err_set_ = false;
    for (;;) {
      Token tok = next_token();
      if (tok.kind == TokenKind::Comment) {
        continue;
      }
      if (tok.kind == TokenKind::Text && is_whitespace_only(tok.text)) {
        continue;
      }
      if (tok.kind == TokenKind::Error) {
        err = error_at(std::string(tok.text), abs_offset(tok.offset));
        err_set_ = true;
      }
      return tok;
    }
  }

  bool skip_trailing_content(ParseError& err) {
    for (;;) {
      Token tok = next_token();
      switch (tok.kind) {
        case TokenKind::EndOfFile:
          return true;
        case TokenKind::Comment:
          continue;
        case TokenKind::Text:
          if (is_whitespace_only(tok.text)) {
            continue;
          }
          err = error_at("unexpected content after </rml>", abs_offset(tok.offset));
          return false;
        case TokenKind::Error:
          err = error_at(std::string(tok.text), abs_offset(tok.offset));
          return false;
        default:
          err = error_at("unexpected content after </rml>", abs_offset(tok.offset));
          return false;
      }
    }
  }

  // EN: Collects Attr tokens after an already-consumed TagOpenStart, entity-decoding each value
  //     as it goes. UIX-ENTITY-PARIDADE (2026-08): decode_entities() can no longer fail (see its
  //     own header comment) -- this loop no longer has an entity-decode error path, only the
  //     lexer-Error/unexpected-token ones below. `terminator_out` is the TagOpenEnd/TagSelfClose
  //     that ended the scan -- callers need its offset+length to compute where the tag's `>` sits.
  // PT: Coleta tokens Attr depois de um TagOpenStart já consumido, decodificando entidade de cada
  //     valor à medida que avança. UIX-ENTITY-PARIDADE (2026-08): decode_entities() não pode mais
  //     falhar (ver o próprio comentário de cabeçalho dela) -- este laço não tem mais caminho de
  //     erro de decodificação de entidade, só os de lexer-Error/token-inesperado abaixo.
  //     `terminator_out` é o TagOpenEnd/TagSelfClose que encerrou o scan -- chamadores precisam do
  //     offset+length dele pra computar onde o '>' da tag fica.
  bool collect_attributes(std::vector<AttrRaw>& attrs_out, bool& self_closing_out,
                          Token& terminator_out, ParseError& err) {
    attrs_out.clear();
    for (;;) {
      Token tok = next_token();
      switch (tok.kind) {
        case TokenKind::Attr: {
          attrs_out.push_back(AttrRaw{tok.text, decode_entities(tok.value)});
          continue;
        }
        case TokenKind::TagOpenEnd:
          self_closing_out = false;
          terminator_out = tok;
          return true;
        case TokenKind::TagSelfClose:
          self_closing_out = true;
          terminator_out = tok;
          return true;
        case TokenKind::Error:
          err = error_at(std::string(tok.text), abs_offset(tok.offset));
          return false;
        default:
          // EN: Unreachable given the lexer's own scan_in_tag() state machine (it only ever
          //     emits Attr/TagOpenEnd/TagSelfClose/Error after a TagOpenStart) -- kept as a
          //     defensive fail-high branch anyway, same "a bug in THIS lexer is not a reason to
          //     trust it blindly" discipline lexer_corpus_sanity.cpp's own comment documents.
          // PT: Inalcançável dada a própria máquina de estado scan_in_tag() do lexer (ela só
          //     emite Attr/TagOpenEnd/TagSelfClose/Error depois de um TagOpenStart) -- mantido
          //     como ramo fail-high defensivo mesmo assim, mesma disciplina "um bug NESTE lexer
          //     não é motivo pra confiar nele cegamente" que o próprio comentário do
          //     lexer_corpus_sanity.cpp documenta.
          err = error_at("unexpected token while scanning tag attributes",
                         abs_offset(tok.offset));
          return false;
      }
    }
  }

  // EN: Applies id/class/generic attributes to `element` -- see parser.hpp header's own
  //     "duplicate-attribute policy" paragraph for the last-wins/whole-value-replace rules this
  //     implements.
  // PT: Aplica atributos id/class/genéricos em `element` -- ver o próprio parágrafo "política de
  //     atributo duplicado" do cabeçalho do parser.hpp pras regras de última-vence/substituição-
  //     de-valor-inteiro que isto implementa.
  static void apply_attributes(Element& element, const std::vector<AttrRaw>& attrs) {
    const AttrRaw* id_attr = nullptr;
    const AttrRaw* class_attr = nullptr;
    for (const AttrRaw& a : attrs) {
      if (a.name == "id") {
        id_attr = &a;
      } else if (a.name == "class") {
        class_attr = &a;
      }
    }
    if (id_attr != nullptr) {
      element.set_id(id_attr->value);
    }
    if (class_attr != nullptr) {
      for (std::string_view token : split_whitespace(class_attr->value)) {
        element.add_class(token);
      }
    }
    for (const AttrRaw& a : attrs) {
      if (a.name == "id" || a.name == "class") {
        continue;
      }
      element.set_attribute(std::string(a.name), a.value);
    }
  }

  // EN: Parses <rml>'s direct children (Comment | whitespace Text | one <head> | one <body>)
  //     until the matching </rml>. See parser.hpp header's own top-level grammar.
  // PT: Parseia os filhos diretos de <rml> (Comentário | Texto whitespace | um <head> | um
  //     <body>) até o </rml> que casa. Ver a própria gramática de topo do cabeçalho do
  //     parser.hpp.
  bool parse_rml_children(Document& doc, bool& saw_head, bool& saw_body, ParseError& err) {
    for (;;) {
      Token tok = next_token();
      switch (tok.kind) {
        case TokenKind::EndOfFile:
          err = error_at("unexpected end of file: unclosed <rml>", abs_offset(tok.offset));
          return false;
        case TokenKind::Error:
          err = error_at(std::string(tok.text), abs_offset(tok.offset));
          return false;
        case TokenKind::Comment:
          continue;
        case TokenKind::Text:
          if (!is_whitespace_only(tok.text)) {
            err = error_at(
                "unexpected non-whitespace text directly inside <rml> (expected "
                "only <head>/<body>)",
                abs_offset_of_span(tok.text));
            return false;
          }
          continue;
        case TokenKind::TagClose:
          if (to_ascii_lower(tok.text) == "rml") {
            return true;
          }
          err = error_at("unexpected closing tag at <rml> level: </" +
                             to_ascii_lower(tok.text) + ">",
                         abs_offset(tok.offset));
          return false;
        case TokenKind::TagOpenStart: {
          const std::string name_lower = to_ascii_lower(tok.text);
          if (name_lower == "head") {
            if (saw_head) {
              err = error_at("duplicate <head>", abs_offset(tok.offset));
              return false;
            }
            if (saw_body) {
              err = error_at("<head> must precede <body>", abs_offset(tok.offset));
              return false;
            }
            if (!handle_head(doc, err)) {
              return false;
            }
            saw_head = true;
            continue;
          }
          if (name_lower == "body") {
            if (saw_body) {
              err = error_at("duplicate <body>", abs_offset(tok.offset));
              return false;
            }
            std::vector<AttrRaw> attrs;
            bool self_closing = false;
            Token terminator;
            if (!collect_attributes(attrs, self_closing, terminator, err)) {
              return false;
            }
            apply_attributes(doc.body(), attrs);
            if (!self_closing) {
              if (!parse_element_contents(doc.body(), "body", /*depth=*/1, err)) {
                return false;
              }
            }
            saw_body = true;
            continue;
          }
          err = error_at("unexpected element <" + name_lower +
                             "> directly inside <rml> (expected only <head>/<body>)",
                         abs_offset(tok.offset));
          return false;
        }
        default:
          err = error_at("unexpected token inside <rml>", abs_offset(tok.offset));
          return false;
      }
    }
  }

  // EN: `<head>`'s own attributes are collected (so a malformed entity in one of THEM is still
  //     fail-high, no special-casing of validation) but discarded -- uix-dom.md section 4's own
  //     last paragraph: "if <head> carries its own attributes... they are NOT captured". Then the
  //     raw-scan (find_head_close) locates the matching </head>, the raw span becomes
  //     Document::set_head(), and `lexer_` is REBUILT over the substring past </head>'s '>' --
  //     see this file's header comment for how absolute offsets survive that rebuild.
  // PT: Os próprios atributos de `<head>` são coletados (então uma entidade malformada num
  //     DELES ainda é fail-high, sem tratamento especial de validação) mas descartados -- o
  //     próprio último parágrafo da seção 4 do uix-dom.md: "se <head> carrega atributos próprios
  //     ... eles NÃO são capturados". Depois o scan-cru (find_head_close) localiza o </head> que
  //     casa, o span cru vira Document::set_head(), e o `lexer_` é RECONSTRUÍDO sobre a substring
  //     depois do '>' do </head> -- ver o comentário de cabeçalho deste arquivo pra como offsets
  //     absolutos sobrevivem a essa reconstrução.
  bool handle_head(Document& doc, ParseError& err) {
    std::vector<AttrRaw> discard_attrs;
    bool self_closing = false;
    Token terminator;
    if (!collect_attributes(discard_attrs, self_closing, terminator, err)) {
      return false;
    }
    if (self_closing) {
      doc.set_head("");
      return true;
    }

    const std::size_t content_start = abs_offset(terminator.offset) + terminator.length;
    auto found = find_head_close(source_, content_start);
    if (!found.has_value()) {
      err = error_at("unterminated <head>: no matching </head> found before end of file",
                     content_start);
      return false;
    }
    const auto [content_end, resume_offset] = *found;
    doc.set_head(std::string(source_.substr(content_start, content_end - content_start)));

    lexer_ = Lexer(source_.substr(resume_offset));
    abs_offset_ = resume_offset;
    return true;
  }

  // EN: Parses the CHILDREN of an already-open element (attributes already applied by the
  //     caller) up to and including the matching TagClose. `tag_lower` is the already-folded
  //     name to match against; `depth` is THIS element's own depth (body=1, its children=2, ...)
  //     -- used ONLY to pre-empt this parser's own C++ recursion before it ever calls
  //     `append_child` (which has its own, redundant, belt-and-suspenders check against
  //     `kMaxElementDepth` -- see dom_tree.hpp).
  // PT: Parseia os FILHOS de um elemento já-aberto (atributos já aplicados por quem chama) até
  //     e incluindo o TagClose que casa. `tag_lower` é o nome já-dobrado pra casar contra;
  //     `depth` é a própria profundidade DESTE elemento (body=1, filhos dele=2, ...) -- usado SÓ
  //     pra preemptar a própria recursão C++ deste parser antes dela sequer chamar
  //     `append_child` (que tem a PRÓPRIA checagem redundante, belt-and-suspenders, contra
  //     `kMaxElementDepth` -- ver dom_tree.hpp).
  bool parse_element_contents(Element& element, std::string_view tag_lower, std::size_t depth,
                              ParseError& err) {
    for (;;) {
      Token tok = next_token();
      switch (tok.kind) {
        case TokenKind::EndOfFile:
          err = error_at("unexpected end of file: unclosed <" + std::string(tag_lower) + ">",
                         abs_offset(tok.offset));
          return false;
        case TokenKind::Error:
          err = error_at(std::string(tok.text), abs_offset(tok.offset));
          return false;
        case TokenKind::Comment:
          continue;
        case TokenKind::Text: {
          // EN: UIX-ENTITY-PARIDADE (2026-08): decode_entities() can no longer fail (see its own
          //     header comment) -- no entity-decode error path here anymore, only the
          //     lexer-Error/EOF/depth-ceiling ones this switch already handles.
          // PT: UIX-ENTITY-PARIDADE (2026-08): decode_entities() não pode mais falhar (ver o
          //     próprio comentário de cabeçalho dela) -- nenhum caminho de erro de decodificação
          //     de entidade aqui mais, só os de lexer-Error/EOF/teto-de-profundidade que este
          //     switch já trata.
          auto text_node = std::make_unique<Text>(decode_entities(tok.text));
          AppendResult r = element.append_child(std::move(text_node));
          if (r.outcome == AppendOutcome::RejectedDepthCeiling) {
            err = error_at("nesting exceeds kMaxElementDepth (256) ceiling",
                           abs_offset(tok.offset));
            return false;
          }
          continue;
        }
        case TokenKind::TagClose: {
          const std::string close_lower = to_ascii_lower(tok.text);
          if (close_lower != tag_lower) {
            err = error_at("mismatched closing tag: expected </" + std::string(tag_lower) +
                               ">, found </" + close_lower + ">",
                           abs_offset(tok.offset));
            return false;
          }
          return true;
        }
        case TokenKind::TagOpenStart: {
          const std::string child_lower = to_ascii_lower(tok.text);
          if (child_lower == "head") {
            err = error_at("<head> is only valid as a direct child of the top-level <rml>",
                           abs_offset(tok.offset));
            return false;
          }
          if (!parse_new_element_and_append(element, tok, child_lower, depth + 1, err)) {
            return false;
          }
          continue;
        }
        default:
          err = error_at("unexpected token while parsing children of <" + std::string(tag_lower) +
                             ">",
                         abs_offset(tok.offset));
          return false;
      }
    }
  }

  bool parse_new_element_and_append(Element& parent, const Token& open_tok, std::string_view tag_lower,
                                    std::size_t depth, ParseError& err) {
    if (depth > kMaxElementDepth) {
      err = error_at("nesting exceeds kMaxElementDepth (256) ceiling", abs_offset(open_tok.offset));
      return false;
    }

    std::vector<AttrRaw> attrs;
    bool self_closing = false;
    Token terminator;
    if (!collect_attributes(attrs, self_closing, terminator, err)) {
      return false;
    }

    auto element = std::make_unique<Element>(std::string(tag_lower));
    apply_attributes(*element, attrs);

    if (!self_closing) {
      if (!parse_element_contents(*element, tag_lower, depth, err)) {
        return false;
      }
    }

    AppendResult r = parent.append_child(std::move(element));
    if (r.outcome == AppendOutcome::RejectedDepthCeiling) {
      err = error_at("nesting exceeds kMaxElementDepth (256) ceiling", abs_offset(open_tok.offset));
      return false;
    }
    return true;
  }

  std::string_view source_;    // ALWAYS the true, original, offset-0 buffer -- never reassigned.
  Lexer lexer_;                // rebuilt at most once, by handle_head().
  std::size_t abs_offset_ = 0; // added to every lexer_-relative Token::offset -- see header.
};

} // namespace

ParseResult parse_document(std::string_view source) {
  Parser parser(source);
  return parser.parse();
}

} // namespace glintfx::uix
