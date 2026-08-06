// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SHEET-PARSER -- implementation. See parser.hpp's own header comment for the full scope/
//     boundary/recovery rationale this file holds itself to; the comments here are about HOW, not
//     WHY (the WHY already lives in the header, once, not duplicated per function -- same
//     convention lexer.cpp already states up front).
// PT: UIX-SHEET-PARSER -- implementação. Ver o próprio comentário de cabeçalho do parser.hpp pro
//     escopo/fronteira/racional-de-recuperação completos a que este arquivo se prende; os
//     comentários aqui são sobre COMO, não PORQUÊ (o PORQUÊ já mora no header, uma vez só, não
//     duplicado por função -- mesma convenção que o próprio lexer.cpp já declara de saída).
// Copyright (c) 2026 Petrus Silva Costa
#include "parser.hpp"

#include "lexer.hpp"
#include "property_registry.hpp"
#include "shorthand.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace glintfx::uix::style {

namespace {

// EN: Same 4-character whitespace set lexer.cpp's own is_whitespace uses -- duplicated here
//     (rather than exposed from lexer.hpp) matching the "each .cpp restates the tiny local helper
//     it needs" convention property_registry.cpp/shorthand.cpp already follow in this module.
// PT: Mesmo conjunto de 4 caracteres de whitespace que o próprio is_whitespace do lexer.cpp usa --
//     duplicado aqui (em vez de exposto pelo lexer.hpp) casando com a convenção "cada .cpp restata
//     o pequeno helper local que precisa" que o property_registry.cpp/shorthand.cpp já seguem
//     neste módulo.
constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view strip_whitespace(std::string_view s) {
  std::size_t begin = 0;
  while (begin < s.size() && is_whitespace(s[begin])) {
    ++begin;
  }
  std::size_t end = s.size();
  while (end > begin && is_whitespace(s[end - 1])) {
    --end;
  }
  return s.substr(begin, end - begin);
}

constexpr bool is_ident_start(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr bool is_ident_char(char c) {
  return is_ident_start(c) || (c >= '0' && c <= '9') || c == '-';
}

// EN: 1-based line/column for `offset` into `source` -- same O(offset)-once-per-parse-failure
//     shape glintfx/src/uix/dom/parser.cpp's own compute_line_col already uses, restated here (not
//     shared, matching this module's own per-.cpp-local-helper convention above).
// PT: Linha/coluna 1-based pra `offset` dentro de `source` -- mesma forma
//     O(offset)-uma-vez-por-falha-de-parse que o próprio compute_line_col do
//     glintfx/src/uix/dom/parser.cpp já usa, restatada aqui (não compartilhada, casando com a
//     própria convenção de helper-local-por-.cpp deste módulo acima).
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

// EN: Splits `raw` at the FIRST literal space (0x20) byte -- deliberately NOT the general
//     whitespace set, matching lexer.cpp's own `pending_at_rule_word_` computation exactly (see
//     lexer.hpp header, "At-rule mode table": upstream's own `pre_token_str.find(' ')` is a
//     literal-space-only split, and this parser's OWN at-rule-name classification stays consistent
//     with whatever mode decision the lexer already baked into the token stream by using the same
//     split point). Returns {name, rest} -- `rest` is everything after that one space byte
//     (unstripped; callers that need it trimmed, e.g. `@keyframes`'s own animation name, call
//     strip_whitespace themselves).
// PT: Divide `raw` no PRIMEIRO byte de espaço literal (0x20) -- deliberadamente NÃO o conjunto
//     geral de whitespace, casando exatamente com a própria computação de `pending_at_rule_word_`
//     do lexer.cpp (ver o cabeçalho do lexer.hpp, "Tabela de modo de at-rule": o próprio
//     `pre_token_str.find(' ')` do upstream é uma divisão só-por-espaço-literal, e a própria
//     classificação de nome-de-at-rule deste parser fica consistente com qualquer decisão de modo
//     que o lexer já embutiu no fluxo de token usando o mesmo ponto de divisão). Retorna
//     {nome, resto} -- `resto` é tudo depois daquele um byte de espaço (não-stripado; chamadores
//     que precisam dele aparado, ex. o próprio nome de animação de `@keyframes`, chamam
//     strip_whitespace eles mesmos).
std::pair<std::string_view, std::string_view> split_first_word(std::string_view raw) {
  const std::size_t space = raw.find(' ');
  if (space == std::string_view::npos) {
    return {raw, {}};
  }
  return {raw.substr(0, space), raw.substr(space + 1)};
}

// EN: Appends `name`/`value` to `*out` per parser.hpp's own "Declaration expansion" paragraph: a
//     registered longhand is kept as-is; a registered shorthand is replaced by its own expanded
//     constituents (`shorthand.hpp`'s own `expand_shorthand`); anything else is DROPPED and a
//     `ParseDiagnostic` is appended to `*diagnostics` (docs/uix-rcss.md section 11's own "unknown
//     property name... the declaration is dropped"). `source`/`offset` feed `compute_line_col` for
//     that diagnostic only -- see parser.hpp header, recovery scope (a).
// PT: Acrescenta `name`/`value` em `*out` per o próprio parágrafo "Expansão de declaração" do
//     parser.hpp: um longhand registrado é mantido como está; um shorthand registrado é substituído
//     pelos próprios constituintes expandidos (o próprio `expand_shorthand` do shorthand.hpp);
//     qualquer outra coisa é DESCARTADA e um `ParseDiagnostic` é acrescentado em `*diagnostics` (a
//     própria "propriedade desconhecida... a declaração é descartada" da seção 11 do
//     docs/uix-rcss.md). `source`/`offset` alimentam o `compute_line_col` só pra aquele
//     diagnóstico -- ver o cabeçalho do parser.hpp, escopo de recuperação (a).
void apply_declaration(std::string_view name, std::string_view value,
                       std::vector<PropertyDeclaration>* out,
                       std::vector<ParseDiagnostic>* diagnostics, std::string_view source,
                       std::size_t offset) {
  if (find_property(name) != nullptr) {
    out->push_back(PropertyDeclaration{name, value});
    return;
  }
  if (is_shorthand(name)) {
    std::vector<LonghandValue> expanded;
    const ShorthandExpandStatus status = expand_shorthand(name, value, &expanded);
    if (status == ShorthandExpandStatus::Ok) {
      std::transform(expanded.begin(), expanded.end(), std::back_inserter(*out),
                     [](const LonghandValue& lv) {
                       return PropertyDeclaration{lv.name, lv.value};
                     });
      return;
    }
    auto [line, col] = compute_line_col(source, offset);
    diagnostics->push_back(ParseDiagnostic{
        "malformed value for shorthand '" + std::string(name) + "': '" + std::string(value) + "'",
        line, col, offset});
    return;
  }
  auto [line, col] = compute_line_col(source, offset);
  diagnostics->push_back(
      ParseDiagnostic{"unknown property name '" + std::string(name) + "'", line, col, offset});
}

// EN: Parses ONE compound (a tag-name?/id?/classes*/hover? run glued with no combinator) starting
//     at `text[*pos]`, advancing `*pos` past it. Returns false (leaving `*pos` wherever it stopped)
//     on ANY of the fail-high forms docs/rmlx-subset.md section 6.2 names as zero-measured:
//       - a leading char that is neither an identifier-start NOR '.'/'#'/':' (catches universal
//         `*`, attribute `[x]`, and a stray leading combinator like `+`/`~` -- none of those chars
//         are ident-start or a marker, so `any` below stays false and this function refuses).
//       - a marker ('.'/'#'/':') with nothing after it (a trailing '.'/'#'/':' with no identifier).
//       - a ':' pseudo-class whose name is anything OTHER than the literal "hover" (catches
//         `:not(...)`, `:nth-child(...)`, `:focus`, `:active`, ... -- only `:hover` is authorized).
//       - trailing bytes after the compound that are neither whitespace, '>', nor ',' (catches,
//         e.g., an attribute selector glued onto a tag: `a[href]`).
// PT: Parseia UM compound (uma sequência tag?/id?/classes*/hover? colada sem combinador) começando
//     em `text[*pos]`, avançando `*pos` além dele. Retorna false (deixando `*pos` onde parou) em
//     QUALQUER uma das formas fail-high que a seção 6.2 do docs/rmlx-subset.md nomeia como
//     zero-medidas:
//       - um char inicial que não é nem início-de-identificador NEM '.'/'#'/':' (pega universal
//         `*`, atributo `[x]`, e um combinador inicial solto tipo `+`/`~` -- nenhum desses chars é
//         início-de-identificador ou marcador, então `any` abaixo fica falso e esta função recusa).
//       - um marcador ('.'/'#'/':') sem nada depois (um '.'/'#'/':' final sem identificador).
//       - uma pseudo-classe ':' cujo nome é qualquer coisa QUE NÃO o literal "hover" (pega
//         `:not(...)`, `:nth-child(...)`, `:focus`, `:active`, ... -- só `:hover` é autorizado).
//       - bytes finais depois do compound que não são whitespace, '>', nem ',' (pega, ex., um
//         seletor de atributo colado numa tag: `a[href]`).
bool parse_compound(std::string_view text, std::size_t* pos, CompoundSelector* out) {
  bool any = false;
  if (*pos < text.size() && is_ident_start(text[*pos])) {
    const std::size_t start = *pos;
    while (*pos < text.size() && is_ident_char(text[*pos])) {
      ++*pos;
    }
    out->tag = text.substr(start, *pos - start);
    any = true;
  }
  while (*pos < text.size() &&
         (text[*pos] == '.' || text[*pos] == '#' || text[*pos] == ':')) {
    const char marker = text[*pos];
    ++*pos;
    const std::size_t start = *pos;
    while (*pos < text.size() && is_ident_char(text[*pos])) {
      ++*pos;
    }
    if (*pos == start) {
      return false; // e.g. trailing '.'/'#'/':' with nothing after
    }
    const std::string_view ident = text.substr(start, *pos - start);
    if (marker == '.') {
      out->classes.push_back(ident);
    } else if (marker == '#') {
      out->id = ident;
    } else {
      if (ident != "hover") {
        return false; // unsupported pseudo-class -- fail-high per docs/rmlx-subset.md section 6.2
      }
      out->pseudo_hover = true;
    }
    any = true;
  }
  if (!any) {
    return false; // e.g. leading '*', '[', '+', '~'
  }
  if (*pos < text.size() && !is_whitespace(text[*pos]) && text[*pos] != '>') {
    return false; // trailing garbage glued onto the compound, e.g. an attribute selector 'a[href]'
  }
  return true;
}

// EN: Parses one comma-separated fragment (already trimmed, non-empty) into a `Selector` chain:
//     compounds joined by `Combinator::Child` (explicit '>') or `Combinator::Descendant` (implicit
//     whitespace-only join). See parser.hpp header's own `Selector` doc-comment for the invariant
//     this maintains (`combinators.size() == compounds.size() - 1`).
// PT: Parseia um trecho separado-por-vírgula (já aparado, não-vazio) numa cadeia `Selector`:
//     compounds unidos por `Combinator::Child` ('>' explícito) ou `Combinator::Descendant`
//     (junção implícita, só-whitespace). Ver o próprio comentário de doc de `Selector` do
//     cabeçalho do parser.hpp pro invariante que isto mantém
//     (`combinators.size() == compounds.size() - 1`).
bool parse_selector_chain(std::string_view text, Selector* out) {
  std::size_t pos = 0;
  while (pos < text.size()) {
    while (pos < text.size() && is_whitespace(text[pos])) {
      ++pos;
    }
    if (pos >= text.size()) {
      break;
    }
    if (text[pos] == '>') {
      if (out->compounds.empty()) {
        return false; // leading '>' -- no left-hand compound to combine
      }
      out->combinators.push_back(Combinator::Child);
      ++pos;
      while (pos < text.size() && is_whitespace(text[pos])) {
        ++pos;
      }
      if (pos >= text.size()) {
        return false; // dangling combinator, nothing after it
      }
    } else if (!out->compounds.empty()) {
      out->combinators.push_back(Combinator::Descendant);
    }
    CompoundSelector compound;
    if (!parse_compound(text, &pos, &compound)) {
      return false;
    }
    out->compounds.push_back(std::move(compound));
  }
  return !out->compounds.empty();
}

// EN: Splits `raw` (a `Prelude` token's own text, starting at absolute byte `raw_offset` in
//     `source`) on top-level ',' -- safe with a dumb scan because none of this subset's authorized
//     selector forms contain a comma inside themselves (no `:nth-child(2n+1)`-style
//     function-argument commas -- that whole pseudo-class is fail-high per `parse_compound` above
//     before a comma inside its parens would ever matter).
//     🔴 UIX-RCSS-ERRATA-2 (orchestrator relay, 2026-08-06 -- an independent audit of
//     docs/uix-rcss.md's 68 normative rules against real RmlUi found 7 byte-exact divergences; this
//     is the one this parser owns): an INVALID fragment (unsupported selector form, or an empty
//     fragment from a stray comma) drops ONLY THAT ONE ENTRY, appending a `ParseDiagnostic` and
//     moving on -- docs/uix-rcss.md section 11's own "the whole rule (not just one selector in a
//     comma-list) fails to register" phrasing does NOT match real RmlUi's own per-entry recovery
//     (the errata's own citation), and docs/rmlx-subset.md's own header clause is explicit that
//     where the two disagree, RmlUi wins. This is NOT a hypothetical: this repo's OWN
//     `glintfx/src/ua_stylesheet.hpp` opens with a 16-tag comma-list rule
//     (docs/rmlx-subset.md section 6.1) that sets `display: block` on every structural element of
//     every document glintfx renders -- one bad tag anywhere in a future edit of that list must
//     not silently disable `display: block` for the other 15. A `Rule` with ZERO surviving entries
//     (every fragment invalid) still cannot register at all -- that is simply what "zero valid
//     selectors" already means, not a separate rule this function encodes.
// PT: Divide `raw` (o próprio texto de um token `Prelude`, começando no byte absoluto `raw_offset`
//     em `source`) por ',' de topo-de-nível -- seguro com uma varredura boba porque nenhuma das
//     formas de seletor autorizadas por este subconjunto contém vírgula dentro de si mesma
//     (nenhuma vírgula de argumento-de-função estilo `:nth-child(2n+1)` -- aquela pseudo-classe
//     inteira já é fail-high pelo próprio `parse_compound` acima antes de uma vírgula dentro dos
//     parênteses dela algum dia importar).
//     🔴 UIX-RCSS-ERRATA-2 (relay do orquestrador, 2026-08-06 -- uma auditoria independente das 68
//     regras normativas do docs/uix-rcss.md contra o RmlUi real achou 7 divergências byte-exatas;
//     esta é a que este parser possui): um trecho INVÁLIDO (forma de seletor não-suportada, ou um
//     trecho vazio de uma vírgula solta) derruba SÓ AQUELA UMA ENTRADA, acrescentando um
//     `ParseDiagnostic` e seguindo -- a própria frase "a regra inteira (não só um seletor numa
//     lista-vírgula) reprova de registrar" da seção 11 do docs/uix-rcss.md NÃO bate com a própria
//     recuperação por-entrada do RmlUi real (a própria citação da errata), e a própria cláusula de
//     cabeçalho do docs/rmlx-subset.md é explícita que, onde os dois discordam, quem manda é o
//     RmlUi. Isto NÃO é hipotético: o PRÓPRIO `glintfx/src/ua_stylesheet.hpp` deste repo abre com
//     uma regra de lista-vírgula de 16 tags (seção 6.1 do docs/rmlx-subset.md) que dá `display:
//     block` a todo elemento estrutural de todo documento que a glintfx renderiza -- uma tag ruim
//     em qualquer edição futura daquela lista não pode desligar `display: block` em silêncio pras
//     outras 15. Uma `Rule` com ZERO entradas sobreviventes (todo trecho inválido) ainda assim não
//     consegue registrar de jeito nenhum -- isso é simplesmente o que "zero seletores válidos" já
//     significa, não uma regra separada que esta função codifica.
void parse_selector_list(std::string_view raw, std::size_t raw_offset, SelectorList* out,
                         std::vector<ParseDiagnostic>* diagnostics, std::string_view source) {
  std::size_t start = 0;
  for (;;) {
    const std::size_t comma = raw.find(',', start);
    const std::string_view fragment =
        (comma == std::string_view::npos) ? raw.substr(start) : raw.substr(start, comma - start);

    std::size_t local_begin = 0;
    while (local_begin < fragment.size() && is_whitespace(fragment[local_begin])) {
      ++local_begin;
    }
    std::size_t local_end = fragment.size();
    while (local_end > local_begin && is_whitespace(fragment[local_end - 1])) {
      --local_end;
    }
    const std::string_view trimmed = fragment.substr(local_begin, local_end - local_begin);
    const std::size_t fragment_offset = raw_offset + start + local_begin;

    if (trimmed.empty()) {
      auto [line, col] = compute_line_col(source, fragment_offset);
      diagnostics->push_back(
          ParseDiagnostic{"empty selector in comma-list (stray comma)", line, col, fragment_offset});
    } else {
      Selector sel;
      if (parse_selector_chain(trimmed, &sel)) {
        out->push_back(std::move(sel));
      } else {
        auto [line, col] = compute_line_col(source, fragment_offset);
        diagnostics->push_back(ParseDiagnostic{
            "unsupported or malformed selector: '" + std::string(trimmed) + "'", line, col,
            fragment_offset});
      }
    }

    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }
}

// EN: The .cpp-local Parser class -- see parser.hpp header's own "single free-function public
//     surface" paragraph for why this stays a local implementation detail. Drives an internal
//     `Lexer`, one token of lookahead (`cur_`), collects `StyleSheet`/`diagnostics_` as it goes,
//     and sets `fatal_` the moment `cur_.kind == TokenKind::Error` is observed ANYWHERE -- every
//     loop in this class checks `fatal_` (directly or via `cur_.kind == Error`) before doing
//     further work, so a sticky lexer Error unwinds this class's own call stack cleanly rather than
//     spinning (`Lexer::next()`'s own sticky contract means `cur_` would otherwise never change).
// PT: A classe Parser local ao .cpp -- ver o próprio parágrafo "superfície pública de função-livre
//     única" do cabeçalho do parser.hpp pro porquê disto ficar um detalhe de implementação local.
//     Dirige um `Lexer` interno, um token de lookahead (`cur_`), coleta `StyleSheet`/`diagnostics_`
//     conforme avança, e seta `fatal_` no momento em que `cur_.kind == TokenKind::Error` é
//     observado EM QUALQUER LUGAR -- todo laço nesta classe checa `fatal_` (direto ou via
//     `cur_.kind == Error`) antes de fazer mais trabalho, então um `Error` pegajoso de lexer
//     desenrola a PRÓPRIA pilha de chamada desta classe de forma limpa em vez de girar (o próprio
//     contrato pegajoso do `Lexer::next()` significa que `cur_` nunca mudaria de outro jeito).
class Parser {
public:
  explicit Parser(std::string_view source) : source_(source), lexer_(source) { advance(); }

  SheetParseResult run() {
    SheetParseResult result;
    result.sheet = std::make_unique<StyleSheet>();
    sheet_ = result.sheet.get();

    while (!fatal_ && cur_.kind != TokenKind::EndOfFile) {
      step_top_level();
    }

    if (fatal_) {
      result.sheet.reset();
      sheet_ = nullptr;
      auto [line, col] = compute_line_col(source_, fatal_offset_);
      result.error = ParseError{fatal_message_, line, col, fatal_offset_};
      result.diagnostics.clear(); // no partial tree -- diagnostics about a discarded tree are moot
      return result;
    }

    result.diagnostics = std::move(diagnostics_);
    return result;
  }

private:
  void advance() { cur_ = lexer_.next(); }

  void note_fatal() {
    fatal_ = true;
    fatal_message_ = std::string(cur_.text);
    fatal_offset_ = cur_.offset;
  }

  void diagnose(const std::string& message, std::size_t offset) {
    auto [line, col] = compute_line_col(source_, offset);
    diagnostics_.push_back(ParseDiagnostic{message, line, col, offset});
  }

  // EN: One iteration of the top-level (Structural-mode) loop: dispatches on `cur_.kind`. See
  //     parser.hpp header's own "At-rule structuring"/"Selector structuring" paragraphs for what
  //     each branch below builds.
  // PT: Uma iteração do laço de topo-de-nível (modo Estrutural): despacha por `cur_.kind`. Ver os
  //     próprios parágrafos "Estruturação de at-rule"/"Estruturação de seletor" do cabeçalho do
  //     parser.hpp pro que cada ramo abaixo constrói.
  void step_top_level() {
    switch (cur_.kind) {
      case TokenKind::Error:
        note_fatal();
        return;
      case TokenKind::EndOfFile:
        return;
      case TokenKind::Comment:
        advance();
        return;
      case TokenKind::BraceClose:
        // EN: A stray top-level '}' -- the lexer's own mode-stack floor (lexer.hpp header, "At-rule
        //     mode table" struct comment) means this token is emitted, never silently dropped, even
        //     though nothing is open to close. Recovery scope (b)-adjacent: nothing to skip (no
        //     block was ever opened), just log and move on.
        // PT: Um '}' de topo-de-nível solto -- o próprio piso da pilha-de-modo do lexer (comentário
        //     de struct "Tabela de modo de at-rule" do lexer.hpp) significa que este token é
        //     emitido, nunca descartado em silêncio, mesmo sem nada aberto pra fechar. Adjacente ao
        //     escopo de recuperação (b): nada pra pular (nenhum bloco foi aberto nunca), só loga e
        //     segue.
        diagnose("stray '}' with no matching open block at top level", cur_.offset);
        advance();
        return;
      case TokenKind::BraceOpen:
        // EN: A '{' with no preceding selector/at-rule text (e.g. source starting with "{ ... }")
        //     -- an anonymous, malformed rule. Recovery scope (b): log, then skip its own body.
        // PT: Um '{' sem texto de seletor/at-rule precedente (ex. fonte começando com "{ ... }") --
        //     uma regra anônima, malformada. Escopo de recuperação (b): loga, depois pula o próprio
        //     corpo dele.
        diagnose("'{' with no preceding selector or at-rule text", cur_.offset);
        advance();
        skip_to_matching_brace_close();
        return;
      case TokenKind::At:
        parse_at_rule_or_unknown();
        return;
      case TokenKind::Prelude:
        parse_ordinary_rule();
        return;
      case TokenKind::Declaration:
        // EN: Structurally unreachable per lexer.hpp's own contract (Declaration tokens only come
        //     from Mode::Declaration, never Mode::Structural/top-level) -- defensive-only, matches
        //     this project's own "log, never silently ignore" discipline for a case that SHOULD be
        //     impossible rather than assuming it.
        // PT: Estruturalmente inalcançável pelo próprio contrato do lexer.hpp (tokens Declaration
        //     só vêm de Mode::Declaration, nunca de Mode::Structural/topo-de-nível) --
        //     só-defensivo, casa com a própria disciplina deste projeto de "loga, nunca ignora em
        //     silêncio" pra um caso que DEVERIA ser impossível em vez de assumir isso.
        diagnose("unexpected Declaration token at top level (lexer contract violation)", cur_.offset);
        advance();
        return;
    }
  }

  // EN: Consumes tokens until the '}' matching an ALREADY-CONSUMED '{' (depth starts at 1) --
  //     works identically for a flat Declaration-mode body or a nested Structural one (a
  //     `@keyframes`-opened region), because it only ever inspects `TokenKind::BraceOpen`/
  //     `BraceClose`, never the semantic content between them. Stops at a sticky lexer `Error`
  //     (sets `fatal_`) or at `EndOfFile` (the lexer's own "unclosed block at EOF is not an error"
  //     leniency, lexer.hpp header -- this recovery simply has nothing left to skip and returns).
  //     See parser.hpp header, recovery scopes (b)/(c).
  // PT: Consome tokens até o '}' que casa com um '{' JÁ-CONSUMIDO (profundidade começa em 1) --
  //     funciona identicamente pra um corpo em modo Declaration plano ou um Estrutural aninhado
  //     (uma região aberta por `@keyframes`), porque só inspeciona `TokenKind::BraceOpen`/
  //     `BraceClose`, nunca o conteúdo semântico entre eles. Para num `Error` pegajoso de lexer
  //     (seta `fatal_`) ou em `EndOfFile` (a própria leniência "bloco não-fechado no EOF não é
  //     erro" do lexer, cabeçalho do lexer.hpp -- esta recuperação simplesmente não tem mais nada
  //     pra pular e retorna). Ver o cabeçalho do parser.hpp, escopos de recuperação (b)/(c).
  void skip_to_matching_brace_close() {
    std::size_t depth = 1;
    while (true) {
      if (cur_.kind == TokenKind::Error) {
        note_fatal();
        return;
      }
      if (cur_.kind == TokenKind::EndOfFile) {
        return;
      }
      if (cur_.kind == TokenKind::BraceOpen) {
        ++depth;
        advance();
        continue;
      }
      if (cur_.kind == TokenKind::BraceClose) {
        --depth;
        advance();
        if (depth == 0) {
          return;
        }
        continue;
      }
      advance();
    }
  }

  // EN: Collects `Declaration` tokens (via `apply_declaration`, see this file's own top-level
  //     comment) until the matching `BraceClose`, which IS consumed here (unlike
  //     `skip_to_matching_brace_close`'s own depth-counting -- this function only ever runs at
  //     depth 1, a rule/keyframe-block body never nests another '{' inside itself per this
  //     subset's own grammar, so a `BraceOpen` seen here would itself be a lexer-contract
  //     violation, logged defensively rather than assumed). Stops at a sticky `Error` (`fatal_`) or
  //     `EndOfFile` (lenient stop, same reasoning as `skip_to_matching_brace_close`). If
  //     `raw_no_registry` is true, `apply_declaration`'s own registry/shorthand routing is
  //     bypassed entirely -- see parser.hpp header's own 🔴 `@font-face` exception paragraph.
  // PT: Coleta tokens `Declaration` (via `apply_declaration`, ver o próprio comentário de
  //     topo-de-nível deste arquivo) até o `BraceClose` correspondente, que É consumido aqui
  //     (diferente da própria contagem-de-profundidade do `skip_to_matching_brace_close` -- esta
  //     função só roda em profundidade 1, um corpo de regra/bloco-de-keyframe nunca aninha outro
  //     '{' dentro de si mesmo pela própria gramática deste subconjunto, então um `BraceOpen` visto
  //     aqui seria ele próprio uma violação de contrato de lexer, logada defensivamente em vez de
  //     assumida). Para num `Error` pegajoso (`fatal_`) ou `EndOfFile` (parada leniente, mesmo
  //     raciocínio do `skip_to_matching_brace_close`). Se `raw_no_registry` for verdadeiro, o
  //     próprio roteamento de registro/shorthand do `apply_declaration` é inteiramente contornado
  //     -- ver o próprio parágrafo de exceção 🔴 `@font-face` do cabeçalho do parser.hpp.
  void collect_declarations(std::vector<PropertyDeclaration>* out, bool raw_no_registry) {
    while (true) {
      if (cur_.kind == TokenKind::Error) {
        note_fatal();
        return;
      }
      if (cur_.kind == TokenKind::EndOfFile) {
        return;
      }
      if (cur_.kind == TokenKind::Comment) {
        advance();
        continue;
      }
      if (cur_.kind == TokenKind::BraceClose) {
        advance();
        return;
      }
      if (cur_.kind == TokenKind::Declaration) {
        if (raw_no_registry) {
          out->push_back(PropertyDeclaration{cur_.text, cur_.value});
        } else {
          apply_declaration(cur_.text, cur_.value, out, &diagnostics_, source_, cur_.offset);
        }
        advance();
        continue;
      }
      // EN: BraceOpen/At/Prelude here would be a lexer-contract violation for a Declaration-mode
      //     body (this subset's grammar never nests a rule inside a rule) -- defensive log, same
      //     "should be impossible, never assumed" discipline as step_top_level's own Declaration
      //     case.
      // PT: BraceOpen/At/Prelude aqui seria uma violação de contrato de lexer pra um corpo em modo
      //     Declaration (a gramática deste subconjunto nunca aninha uma regra dentro de outra) --
      //     log defensivo, mesma disciplina "deveria ser impossível, nunca assumido" do próprio
      //     caso Declaration do step_top_level.
      diagnose("unexpected token inside a declaration block (lexer contract violation)", cur_.offset);
      advance();
    }
  }

  // EN: See parser.hpp header's own "Recovery is an acceptance criterion" for the whitespace-only-
  //     Prelude no-op below and UIX-RCSS-ERRATA-2's own per-entry comma-list recovery this function
  //     now applies via `parse_selector_list` (see that free function's own header comment).
  // PT: Ver o próprio "Recuperação é critério de aceite" do cabeçalho do parser.hpp pro no-op de
  //     Prelude só-whitespace abaixo e a própria recuperação por-entrada de lista-vírgula da
  //     UIX-RCSS-ERRATA-2 que esta função agora aplica via `parse_selector_list` (ver o próprio
  //     comentário de cabeçalho daquela função-livre).
  void parse_ordinary_rule() {
    const Token prelude_tok = cur_;

    // EN: A `Prelude` whose entire text is whitespace can ONLY arise from inter-token noise (blank
    //     lines between a closed rule's own '}' and a following comment/at-rule/EOF -- lexer.hpp's
    //     own byte-verbatim Prelude scan has no reason to skip it) -- never from an author writing
    //     an actual (even malformed) selector. Silently absorbed, matching this repo's own DOM
    //     sibling's "whitespace-only Text does not exist as content" tree invariant, restated here
    //     at the token layer instead of the tree layer (this module has no tree node for it to
    //     filter).
    // PT: Um `Prelude` cujo texto inteiro é whitespace só pode surgir de ruído entre-token (linhas
    //     em branco entre o próprio '}' de uma regra fechada e um comentário/at-rule/EOF seguinte --
    //     o próprio scan byte-verbatim de Prelude do lexer.hpp não tem motivo pra pular isso) --
    //     nunca de um autor escrevendo um seletor de verdade (mesmo malformado). Absorvido em
    //     silêncio, casando com o próprio invariante de árvore "Text só-whitespace não existe como
    //     conteúdo" do irmão DOM deste repo, restatado aqui na camada de token em vez da camada de
    //     árvore (este módulo não tem nó de árvore pra filtrar isso).
    if (strip_whitespace(prelude_tok.text).empty()) {
      advance();
      return;
    }

    advance();
    if (cur_.kind != TokenKind::BraceOpen) {
      diagnose("selector prelude '" + std::string(prelude_tok.text) + "' has no '{' body",
               prelude_tok.offset);
      return; // nothing opened -- nothing to skip; resume at whatever cur_ already is
    }
    advance(); // consume '{'

    SelectorList selectors;
    parse_selector_list(prelude_tok.text, prelude_tok.offset, &selectors, &diagnostics_, source_);
    if (selectors.empty()) {
      // EN: Every entry was invalid (or the prelude was a single, malformed selector, the
      //     non-comma-list case) -- per-entry diagnostics were already appended by
      //     parse_selector_list above; nothing left to register.
      // PT: Toda entrada era inválida (ou o prelúdio era um único seletor malformado, o caso
      //     não-lista-vírgula) -- diagnósticos por-entrada já foram acrescentados pelo
      //     parse_selector_list acima; nada sobrou pra registrar.
      skip_to_matching_brace_close();
      return;
    }

    Rule rule;
    rule.selectors = std::move(selectors);
    collect_declarations(&rule.declarations, /*raw_no_registry=*/false);
    if (fatal_) {
      return;
    }
    if (sheet_ != nullptr) {
      sheet_->rules.push_back(std::move(rule));
    }
  }

  void parse_at_rule_or_unknown() {
    advance(); // consume '@'
    std::string_view name;
    std::string_view rest;
    std::size_t at_offset = cur_.offset; // best-effort location if there is no Prelude at all
    if (cur_.kind == TokenKind::Prelude) {
      const Token prelude_tok = cur_;
      at_offset = prelude_tok.offset;
      std::tie(name, rest) = split_first_word(prelude_tok.text);
      // EN: `strip_whitespace` the extracted word -- fixes a real corpus shape
      //     (`glintfx/demos/showcase/showcase.rcss:10`, `@font-face\n{`, a newline instead of a
      //     literal space before '{') where `split_first_word`'s own literal-space-only split
      //     (needed VERBATIM for `@keyframes`'s own lexer-mode-sync, see that function's own header
      //     comment) finds no space and returns the WHOLE raw prelude, trailing newline included,
      //     as `name`. Safe for `@keyframes` too: if the source has no literal space anywhere
      //     before `spin`'s own name either, the lexer's OWN `pending_at_rule_word_` computation
      //     (lexer.cpp, identical `find(' ')` call) would ALSO fail to match the literal
      //     "keyframes", so this parser's classification staying in lockstep (both sides fail to
      //     recognise it) is exactly the consistency this split point exists to preserve --
      //     stripping trailing whitespace off ONE segment can never flip that agreement.
      // PT: `strip_whitespace` na palavra extraída -- conserta uma forma real de corpus
      //     (`glintfx/demos/showcase/showcase.rcss:10`, `@font-face\n{`, uma quebra de linha em vez
      //     de um espaço literal antes do '{') onde a própria divisão só-por-espaço-literal do
      //     `split_first_word` (necessária VERBATIM pro próprio sync-de-modo-de-lexer de
      //     `@keyframes`, ver o próprio comentário de cabeçalho daquela função) não acha espaço
      //     nenhum e retorna o PRÓPRIO prelúdio cru inteiro, quebra de linha final incluída, como
      //     `name`. Seguro pro `@keyframes` também: se a fonte não tem espaço literal nenhum antes
      //     do próprio nome de `spin` também, a própria computação de `pending_at_rule_word_` do
      //     lexer (lexer.cpp, mesma chamada `find(' ')`) TAMBÉM falharia em casar o literal
      //     "keyframes", então esta classificação deste parser ficar em lockstep (os dois lados
      //     falham em reconhecer) é exatamente a consistência que este ponto de divisão existe pra
      //     preservar -- stripar whitespace final de UM segmento nunca consegue inverter esse
      //     acordo.
      name = strip_whitespace(name);
      advance();
    }

    if (cur_.kind != TokenKind::BraceOpen) {
      diagnose("at-rule '@" + std::string(name) + "' has no '{' body", at_offset);
      return; // nothing opened -- nothing to skip
    }
    advance(); // consume '{'

    if (name == "font-face") {
      FontFaceRule ff;
      collect_declarations(&ff.attributes, /*raw_no_registry=*/true);
      if (fatal_) {
        return;
      }
      if (sheet_ != nullptr) {
        sheet_->font_faces.push_back(std::move(ff));
      }
      return;
    }

    if (name == "keyframes") {
      KeyframesRule kr;
      kr.name = strip_whitespace(rest);
      parse_keyframes_body(&kr);
      if (fatal_) {
        return;
      }
      if (sheet_ != nullptr) {
        sheet_->keyframes.push_back(std::move(kr));
      }
      return;
    }

    diagnose("unknown at-rule '@" + std::string(name) + "' -- body skipped", at_offset);
    skip_to_matching_brace_close();
  }

  // EN: We are positioned right after `@keyframes`'s own already-consumed '{' -- the lexer opened
  //     a NESTED Structural region here (lexer.hpp header, "At-rule mode table"), so `cur_` cycles
  //     through Comment/Prelude(keyframe selector text)/BraceOpen/.../BraceClose(closes THIS
  //     keyframe's own body)/... until the outer BraceClose (closes the whole `@keyframes` block,
  //     consumed here). Each keyframe selector's OWN body is a flat Declaration-mode region (only
  //     the literal "keyframes" word gets nested-Structural treatment, never a keyframe selector
  //     itself) -- collected via the SAME `collect_declarations` an ordinary rule uses.
  // PT: Estamos posicionados logo depois do próprio '{' já-consumido de `@keyframes` -- o lexer
  //     abriu uma região Estrutural ANINHADA aqui (cabeçalho do lexer.hpp, "Tabela de modo de
  //     at-rule"), então `cur_` cicla por Comment/Prelude(texto de seletor de keyframe)/
  //     BraceOpen/.../BraceClose(fecha o corpo DESTE keyframe)/... até o BraceClose externo (fecha
  //     o bloco `@keyframes` inteiro, consumido aqui). O PRÓPRIO corpo de cada seletor de keyframe
  //     é uma região em modo Declaration plana (só a palavra literal "keyframes" recebe tratamento
  //     Estrutural-aninhado, nunca um seletor de keyframe em si) -- coletado via o MESMO
  //     `collect_declarations` que uma regra comum usa.
  void parse_keyframes_body(KeyframesRule* out) {
    while (true) {
      if (cur_.kind == TokenKind::Error) {
        note_fatal();
        return;
      }
      if (cur_.kind == TokenKind::EndOfFile) {
        return;
      }
      if (cur_.kind == TokenKind::Comment) {
        advance();
        continue;
      }
      if (cur_.kind == TokenKind::BraceClose) {
        advance();
        return;
      }
      if (cur_.kind == TokenKind::Prelude) {
        const Token sel_tok = cur_;
        // EN: Same whitespace-only-Prelude no-op as parse_ordinary_rule's own -- inter-token blank
        //     lines between two keyframe blocks (or before the closing '}'), never an author's own
        //     content. See parse_ordinary_rule's own comment for the full reasoning.
        // PT: Mesmo no-op de Prelude só-whitespace do próprio parse_ordinary_rule -- linhas em
        //     branco entre-token entre dois blocos de keyframe (ou antes do '}' de fechamento),
        //     nunca conteúdo do autor. Ver o próprio comentário do parse_ordinary_rule pro
        //     raciocínio completo.
        if (strip_whitespace(sel_tok.text).empty()) {
          advance();
          continue;
        }
        advance();
        if (cur_.kind != TokenKind::BraceOpen) {
          diagnose("keyframe selector '" + std::string(sel_tok.text) + "' has no '{' body",
                   sel_tok.offset);
          continue; // best-effort resync at whatever cur_ already is
        }
        advance(); // consume this keyframe's own '{'
        KeyframeBlock block;
        block.selector_text = strip_whitespace(sel_tok.text);
        collect_declarations(&block.declarations, /*raw_no_registry=*/false);
        if (fatal_) {
          return;
        }
        out->blocks.push_back(std::move(block));
        continue;
      }
      diagnose("unexpected token inside '@keyframes' body (lexer contract violation)", cur_.offset);
      advance();
    }
  }

  std::string_view source_;
  Lexer lexer_;
  Token cur_{};
  StyleSheet* sheet_ = nullptr;
  std::vector<ParseDiagnostic> diagnostics_;
  bool fatal_ = false;
  std::string fatal_message_;
  std::size_t fatal_offset_ = 0;
};

} // namespace

SheetParseResult parse_stylesheet(std::string_view source) {
  Parser parser(source);
  return parser.run();
}

} // namespace glintfx::uix::style
