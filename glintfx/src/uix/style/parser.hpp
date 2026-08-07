// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SHEET-PARSER -- recursive-descent parser: consumes lexer.hpp's `Lexer` token stream and
//     builds a `StyleSheet` tree (rules + `@font-face`/`@keyframes` structural records). This is
//     the FOURTH brick of glintfx's own style engine, the RCSS mirror of
//     glintfx/src/uix/dom/parser.hpp (the DOM module's own S3) -- that file's header comment
//     establishes several conventions this one inherits without re-deriving them: the single
//     free-function public surface (`parse_stylesheet`, mirroring `parse_document`), the
//     `Parser`-class-as-.cpp-local-detail shape, and the `line`/`column`/`offset` triple for a
//     located error. Read that file's own header FIRST.
//
//     WHAT THIS SLICE OWNS, per this task's own brief ("prelúdio virando seletor estruturado,
//     declarações virando pares (nome, valor cru) com expansão de shorthand pelo registro, e os
//     at-rules virando registros estruturais"):
//       - SELECTOR STRUCTURING: turns a `Prelude` token's raw bytes into a `SelectorList` (a
//         comma-separated list of `Selector` chains, each chain a sequence of `CompoundSelector`
//         nodes joined by `Combinator::Descendant`/`Combinator::Child`). The exact 8 forms this
//         parser recognises, and the 4 it fail-highs, are `docs/rmlx-subset.md` section 6.2's own
//         closed, corpus-counted table -- restated once, at `parse_compound`'s own definition in
//         parser.cpp, not paraphrased here. This is NOT selector *matching* -- there is no DOM here,
//         no specificity computation, no `:hover`-state evaluation; a `Selector` is a pure syntax
//         tree, exactly as `docs/rmlx-subset.md`'s own header clause names this item's boundary
//         ("casamento de seletor... fatias seguintes").
//       - DECLARATION EXPANSION: for every `Declaration` token inside an ordinary rule's or a
//         `@keyframes` block's body, this parser (a) looks it up in `property_registry.hpp`'s
//         closed table directly (a longhand -- kept as-is), (b) failing that, tries
//         `shorthand.hpp`'s `expand_shorthand` (a shorthand -- replaced by its longhand
//         constituents, per docs/uix-rcss.md section 6.2's own algorithm table `shorthand.cpp`
//         already implements), or (c) failing both, drops the declaration and records a
//         `ParseDiagnostic` (docs/uix-rcss.md section 11's own "unknown property name... the
//         declaration is dropped; every OTHER declaration in the same rule block still applies").
//         This parser never inspects or transforms a VALUE string itself -- see
//         property_registry.hpp's and shorthand.hpp's own "not a value parser" boundary, which
//         applies identically here: quantization, color canonicalization, and the composite
//         (`box-shadow`/`decorator`/...) sub-grammars are explicitly a LATER slice's job, per this
//         item's own brief ("Não compute valor").
//         🔴 EXCEPTION, DELIBERATE, DOCUMENTED: `@font-face`'s own attribute set (`font-family`,
//         `src`, `-rmlui-fallback-face`) is captured RAW, with NEITHER the registry lookup NOR the
//         shorthand expansion applied -- docs/uix-rcss.md section 10's own explicit statement that
//         these live in a "structurally separate ID space" from the per-node `PropertyId`/`PROP`
//         registry this file's sibling implements (a real upstream citation that document's own §10
//         paragraph already gives, not re-derived here). Running `find_property("font-family")`
//         against a `@font-face` attribute would be a CATEGORY ERROR: it would find the *element*
//         `font-family` row (§6.1, `inherited: true`, `ValueDomain::String`) and silently treat a
//         font-RESOURCE name as an element style value, and `src`/`-rmlui-fallback-face` are not in
//         that table AT ALL and would be wrongly dropped as "unknown property" -- see
//         `FontFaceRule`'s own doc-comment below for the concrete consequence this avoids.
//       - AT-RULE STRUCTURING: `@font-face` becomes a `FontFaceRule` (flat attribute list, see
//         above); `@keyframes <name> { <percentage-or-from/to> { <declarations> } ... }` becomes a
//         `KeyframesRule` (a name plus an ordered list of `KeyframeBlock`s, each one a raw selector
//         text -- `"0%"`/`"50%"`/`"from"`/`"to"`, UNRESOLVED, per docs/uix-rcss.md section 10's own
//         "fourth, distinct meaning of `%`... this dump does not attempt to resolve or interpolate"
//         -- plus that block's own declarations, expanded exactly like an ordinary rule's, because a
//         keyframe body's properties (`transform`, `opacity`, ...) ARE real, registry-known element
//         properties, just scoped to an animation frame instead of the cascade). Any OTHER at-rule
//         name (case-sensitive; `@media`/`@import`/`@charset`/`@supports`/a typo/anything the census
//         never measured) is a KNOWN, NAMED limitation, not a silent guess: its `{...}` body is
//         located via a token-level brace-depth count (`skip_to_matching_brace_close`, works
//         identically whether the lexer opened a flat Declaration region for it or -- impossible for
//         any name but the literal "keyframes", per lexer.hpp's own "At-rule mode table" -- a nested
//         Structural one) and discarded whole, with one `ParseDiagnostic` naming the raw at-rule
//         word and its byte offset. This mirrors `docs/rmlx-subset.md` section 13's own real-zero
//         exclusion for these at-rules and lexer.hpp's own "stop, add a name to this table,
//         corpus-justified" discipline for ever widening the set of two names this parser knows.
//
//     RECOVERY IS AN ACCEPTANCE CRITERION, NOT A NICETY (this task's own brief, verbatim: "uma
//     regra malformada não pode derrubar a folha"): THREE independent, DIFFERENTLY-SCOPED recovery
//     mechanisms, each proven by its own dedicated test in
//     tests/uix_style/parser_atrule_sanity.cpp, never conflated:
//       (a) DECLARATION-scoped: dropping ONE `PropertyDeclaration` never touches its siblings in the
//           same rule/keyframe-block body (see "Declaration expansion" above) -- no skip needed, a
//           `Declaration` token is already an atomic unit by the time this parser sees it (S1's own
//           contract).
//       (b) RULE-scoped: a `Prelude` this parser cannot turn into a valid `SelectorList` (an
//           unsupported selector FORM -- `*`, `[attr]`, `+`/`~`, `:not(...)`, `nth-child(...)`, or a
//           genuinely malformed fragment, e.g. a stray trailing comma) fails the WHOLE rule to
//           register (docs/uix-rcss.md section 11's own "the whole rule (not just one selector in a
//           comma-list) fails to register") -- one `ParseDiagnostic` naming the raw, UNPARSED
//           selector text, then `skip_to_matching_brace_close()` discards that rule's `{...}` body
//           token-by-token (comment/declaration-shaped or not, it does not matter -- brace depth is
//           the only signal this recovery needs) and the OUTER loop resumes at the very next
//           top-level construct, completely unaffected.
//       (c) AT-RULE-scoped: an unrecognised at-rule NAME (see "At-rule structuring" above) discards
//           its entire `{...}` body the same way, one `ParseDiagnostic`, sheet parsing continues.
//     NONE of these three ever touches `fatal_`/`ParseError` -- that field is reserved EXCLUSIVELY
//     for a genuine LEXER-level sticky `Error` (a `kMaxInputBytes`/`kMaxTokenBytes`/`kMaxNestDepth`
//     ceiling, an unterminated comment/quoted-value -- see lexer.hpp's own "Hardening" and "EOF
//     permissiveness" paragraphs for the closed list), because ONLY that layer has no resync
//     primitive at all: `Lexer::next()`'s own sticky-`Error` contract means every subsequent call
//     returns the identical `Error` token forever, so there is no byte range left to skip past --
//     unlike a malformed selector or an unrecognised at-rule name, where the TOKEN STREAM remains
//     perfectly well-formed (balanced braces, atomic declarations) and only this parser's OWN
//     semantic classification of that stream failed. See `ParseError` vs. `ParseDiagnostic`'s own
//     doc-comments below for exactly which failures land in which bucket.
//
//     ZERO-COPY, UNLIKE THE DOM SIBLING -- A DELIBERATE, NAMED DIVERGENCE: `glintfx::uix::parser.hpp`
//     (the DOM module) copies every string into its `Document` because entity decoding (`&amp;` ->
//     `&`) produces bytes that are not a substring of the original source. THIS module's grammar has
//     no entity syntax anywhere in scope (RCSS values are, per this module's own upstream-derived
//     boundary, raw source text -- see lexer.hpp header's own "Token model" paragraph) -- so every
//     `std::string_view` this file's tree holds is EITHER a zero-copy slice of the CALLER's own
//     `source` buffer (matching `Lexer`'s own zero-copy `Token` contract this file consumes) OR,
//     for a `Flex` shorthand's own omitted-value defaults, a view into a `shorthand.cpp`-owned
//     literal of static storage duration (see shorthand.hpp's own `LonghandValue` doc-comment) --
//     NEVER a view into memory this parser itself allocated and could free. Consequence, stated so a
//     caller does not have to discover it by crashing: `source` MUST outlive every `StyleSheet` this
//     function returns, identically to `Lexer`'s own contract, NOT the DOM sibling's "only needs to
//     stay alive for the duration of this call" contract -- the two modules genuinely differ here,
//     not by oversight.
//
//     NAMESPACE: `glintfx::uix::style`, same module as lexer.hpp/property_registry.hpp/
//     shorthand.hpp -- see lexer.hpp's own header for why this is NOT the DOM sibling's flat
//     `glintfx::uix`.
// PT: UIX-SHEET-PARSER -- parser recursivo-descendente: consome o fluxo de tokens do `Lexer` do
//     lexer.hpp e constrói uma árvore `StyleSheet` (regras + registros estruturais `@font-face`/
//     `@keyframes`). Este é o QUARTO tijolo do motor de estilo próprio da glintfx, o espelho em RCSS
//     do glintfx/src/uix/dom/parser.hpp (a própria S3 do módulo DOM) -- o comentário de cabeçalho
//     daquele arquivo estabelece várias convenções que este herda sem re-derivá-las: a superfície
//     pública de função-livre única (`parse_stylesheet`, espelhando `parse_document`), a forma
//     classe-`Parser`-como-detalhe-local-do-.cpp, e o trio `line`/`column`/`offset` pra um erro
//     localizado. Leia PRIMEIRO o próprio cabeçalho daquele arquivo.
//
//     O QUE ESTA FATIA POSSUI, per o próprio briefing desta tarefa ("prelúdio virando seletor
//     estruturado, declarações virando pares (nome, valor cru) com expansão de shorthand pelo
//     registro, e os at-rules virando registros estruturais"):
//       - ESTRUTURAÇÃO DE SELETOR: transforma os bytes crus de um token `Prelude` numa
//         `SelectorList` (uma lista separada-por-vírgula de cadeias `Selector`, cada cadeia uma
//         sequência de nós `CompoundSelector` unidos por `Combinator::Descendant`/
//         `Combinator::Child`). As exatas 8 formas que este parser reconhece, e as 4 que ele
//         fail-higha, são a própria tabela fechada, contada-por-corpus, da seção 6.2 do
//         `docs/rmlx-subset.md` -- restatada uma vez, na própria definição de `parse_compound` no
//         parser.cpp, não parafraseada aqui. Isto NÃO é *casamento* de seletor -- não há DOM aqui,
//         nenhum cálculo de especificidade, nenhuma avaliação de estado-`:hover`; um `Selector` é
//         uma árvore de sintaxe pura, exatamente como a própria cláusula de cabeçalho do
//         `docs/rmlx-subset.md` nomeia a fronteira deste item ("casamento de seletor... fatias
//         seguintes").
//       - EXPANSÃO DE DECLARAÇÃO: pra todo token `Declaration` dentro do corpo de uma regra comum
//         ou de um bloco `@keyframes`, este parser (a) busca no próprio registro fechado do
//         property_registry.hpp direto (um longhand -- mantido como está), (b) falhando isso,
//         tenta o `expand_shorthand` do shorthand.hpp (um shorthand -- substituído pelos próprios
//         constituintes longhand, per a própria tabela de algoritmo da seção 6.2 do
//         docs/uix-rcss.md que o shorthand.cpp já implementa), ou (c) falhando os dois, descarta a
//         declaração e registra um `ParseDiagnostic` (a própria "propriedade desconhecida... a
//         declaração é descartada; toda OUTRA declaração no mesmo bloco de regra ainda se aplica"
//         da seção 11 do docs/uix-rcss.md). Este parser nunca inspeciona ou transforma um texto de
//         VALOR ele próprio -- ver a própria fronteira "não é parser de valor" do
//         property_registry.hpp e do shorthand.hpp, que se aplica identicamente aqui: quantização,
//         canonicalização de cor, e as sub-gramáticas compostas (`box-shadow`/`decorator`/...) são
//         explicitamente trabalho de uma fatia POSTERIOR, per o próprio briefing deste item ("Não
//         compute valor").
//         🔴 EXCEÇÃO, DELIBERADA, DOCUMENTADA: o próprio conjunto de atributos de `@font-face`
//         (`font-family`, `src`, `-rmlui-fallback-face`) é capturado CRU, NEM a busca de registro
//         NEM a expansão de shorthand aplicadas -- a própria declaração explícita da seção 10 do
//         docs/uix-rcss.md de que estes moram num "espaço de ID estruturalmente separado" do
//         registro `PropertyId`/`PROP` por-nó que o irmão deste arquivo implementa (uma citação real
//         de upstream que o próprio parágrafo §10 daquele documento já dá, não re-derivada aqui).
//         Rodar `find_property("font-family")` contra um atributo de `@font-face` seria um ERRO DE
//         CATEGORIA: acharia a própria linha `font-family` do *elemento* (§6.1, `inherited: true`,
//         `ValueDomain::String`) e trataria em silêncio um nome de RECURSO de fonte como um valor de
//         estilo de elemento, e `src`/`-rmlui-fallback-face` não estão naquela tabela DE JEITO
//         NENHUM e seriam descartados errado como "propriedade desconhecida" -- ver o próprio
//         comentário de doc do `FontFaceRule` abaixo pra consequência concreta que isto evita.
//       - ESTRUTURAÇÃO DE AT-RULE: `@font-face` vira um `FontFaceRule` (lista plana de atributos,
//         ver acima); `@keyframes <nome> { <porcentagem-ou-from/to> { <declarações> } ... }` vira um
//         `KeyframesRule` (um nome mais uma lista ordenada de `KeyframeBlock`s, cada um um texto de
//         seletor cru -- `"0%"`/`"50%"`/`"100%"`/`"from"`/`"to"`, NÃO-RESOLVIDO, per a própria
//         "quarta semântica distinta de `%`... este dump não tenta resolver ou interpolar" da seção
//         10 do docs/uix-rcss.md -- mais as declarações daquele bloco, expandidas exatamente como as
//         de uma regra comum, porque as propriedades de um corpo de keyframe (`transform`,
//         `opacity`, ...) SÃO propriedades de elemento reais, conhecidas do registro, só escopadas a
//         um quadro de animação em vez da cascata). Todo OUTRO nome de at-rule (case-sensitive;
//         `@media`/`@import`/`@charset`/`@supports`/um typo/qualquer coisa que o censo nunca mediu)
//         é uma limitação CONHECIDA, NOMEADA, não um chute em silêncio: o corpo `{...}` dele é
//         localizado via uma contagem de profundidade-de-chave a nível de token
//         (`skip_to_matching_brace_close`, funciona identicamente se o lexer abriu uma região
//         Declaration plana pra ele ou -- impossível pra qualquer nome que não o literal
//         "keyframes", per a própria "Tabela de modo de at-rule" do lexer.hpp -- uma Estrutural
//         aninhada) e descartado inteiro, com um `ParseDiagnostic` nomeando a própria palavra crua
//         de at-rule e o offset de byte dela. Isto espelha a própria exclusão real-zero da seção 13
//         do `docs/rmlx-subset.md` pra estes at-rules e a própria disciplina "para, soma um nome
//         nesta tabela, justificado por corpus" do lexer.hpp pra alargar os dois nomes que este
//         parser conhece.
//
//     RECUPERAÇÃO É CRITÉRIO DE ACEITE, NÃO UM CAPRICHO (o próprio briefing desta tarefa, verbatim:
//     "uma regra malformada não pode derrubar a folha"): TRÊS mecanismos de recuperação
//     independentes, ESCOPADOS DIFERENTEMENTE, cada um provado pelo próprio teste dedicado em
//     tests/uix_style/parser_atrule_sanity.cpp, nunca confundidos:
//       (a) Escopo de DECLARAÇÃO: descartar UMA `PropertyDeclaration` nunca toca as irmãs dela no
//           mesmo corpo de regra/bloco-de-keyframe (ver "Expansão de declaração" acima) -- nenhum
//           skip necessário, um token `Declaration` já é uma unidade atômica no momento em que este
//           parser o vê (o próprio contrato da S1).
//       (b) Escopo de REGRA: um `Prelude` que este parser não consegue transformar numa
//           `SelectorList` válida (uma FORMA de seletor não-suportada -- `*`, `[attr]`, `+`/`~`,
//           `:not(...)`, `nth-child(...)`, ou um trecho genuinamente malformado, ex. uma vírgula
//           final solta) reprova a regra INTEIRA de registrar (a própria "a regra inteira (não só
//           um seletor numa lista-vírgula) reprova de registrar" da seção 11 do docs/uix-rcss.md) --
//           um `ParseDiagnostic` nomeando o texto de seletor cru, NÃO-PARSEADO, depois o
//           `skip_to_matching_brace_close()` descarta o corpo `{...}` daquela regra token-por-token
//           (tem forma de comentário/declaração ou não, não importa -- profundidade de chave é o
//           único sinal que esta recuperação precisa) e o laço EXTERNO retoma na PRÓXIMA construção
//           de topo-de-nível, completamente ileso.
//       (c) Escopo de AT-RULE: um NOME de at-rule não-reconhecido (ver "Estruturação de at-rule"
//           acima) descarta o corpo `{...}` inteiro do mesmo jeito, um `ParseDiagnostic`, o parse da
//           folha continua.
//     NENHUM dos três nunca toca `fatal_`/`ParseError` -- esse campo é reservado EXCLUSIVAMENTE pra
//     um `Error` pegajoso genuíno de nível-LEXER (um teto de `kMaxInputBytes`/`kMaxTokenBytes`/
//     `kMaxNestDepth`, um comentário/valor-entre-aspas não-terminado -- ver os próprios parágrafos
//     "Hardening" e "Permissividade de EOF" do lexer.hpp pra lista fechada), porque SÓ aquela camada
//     não tem primitiva de ressincronização nenhuma: o próprio contrato de `Error` pegajoso do
//     `Lexer::next()` significa que toda chamada subsequente retorna o MESMO token `Error` pra
//     sempre, então não sobra faixa de byte nenhuma pra pular -- diferente de um seletor malformado
//     ou de um nome de at-rule não-reconhecido, onde o PRÓPRIO FLUXO DE TOKEN continua perfeitamente
//     bem-formado (chaves balanceadas, declarações atômicas) e só a classificação semântica PRÓPRIA
//     deste parser daquele fluxo falhou. Ver os próprios comentários de doc de `ParseError` vs.
//     `ParseDiagnostic` abaixo pra exatamente qual falha cai em qual balde.
//
//     ZERO-CÓPIA, DIFERENTE DO IRMÃO DOM -- UMA DIVERGÊNCIA DELIBERADA, NOMEADA: o
//     `glintfx::uix::parser.hpp` (o módulo DOM) copia toda string na `Document` dele porque
//     decodificação de entidade (`&amp;` -> `&`) produz bytes que não são uma substring da fonte
//     original. A gramática DESTE módulo não tem sintaxe de entidade em escopo nenhum (valores RCSS
//     são, per a própria fronteira derivada-de-upstream deste módulo, texto-fonte cru -- ver o
//     próprio parágrafo "Modelo de token" do cabeçalho do lexer.hpp) -- então todo `std::string_view`
//     que a árvore deste arquivo carrega é OU uma fatia zero-cópia do próprio buffer `source` do
//     CHAMADOR (casando com o próprio contrato zero-cópia de `Token` do `Lexer` que este arquivo
//     consome) OU, pros próprios defaults de valor-omitido de um shorthand `Flex`, uma view sobre
//     um literal de posse do shorthand.cpp de duração de armazenamento estática (ver o próprio
//     comentário de doc do `LonghandValue` do shorthand.hpp) -- NUNCA uma view sobre memória que este
//     próprio parser alocou e poderia liberar. Consequência, declarada pra um chamador não precisar
//     descobrir travando: `source` PRECISA sobreviver a todo `StyleSheet` que esta função retornar,
//     identicamente ao próprio contrato do `Lexer`, NÃO o contrato "só precisa ficar viva pela
//     duração desta chamada" do irmão DOM -- os dois módulos genuinamente divergem aqui, não por
//     descuido.
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo do lexer.hpp/property_registry.hpp/
//     shorthand.hpp -- ver o próprio cabeçalho do lexer.hpp pro porquê disto NÃO ser o
//     `glintfx::uix` plano do irmão DOM.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace glintfx::uix::style {

// EN: How two adjacent `CompoundSelector`s in a `Selector` chain relate. `Descendant` is an
//     implicit whitespace-only join (`#panel span`, 131 measured); `Child` is an explicit `>`
//     (`#scroller > div`, 2 measured) -- see docs/rmlx-subset.md section 6.2's own count table.
// PT: Como dois `CompoundSelector` adjacentes numa cadeia `Selector` se relacionam. `Descendant` é
//     uma junção implícita, só-whitespace (`#panel span`, 131 medidos); `Child` é um `>` explícito
//     (`#scroller > div`, 2 medidos) -- ver a própria tabela de contagem da seção 6.2 do
//     docs/rmlx-subset.md.
enum class Combinator {
  Descendant,
  Child,
};

// EN: One "word" of a selector chain -- everything glued together with no combinator between it:
//     an optional tag name, an optional `#id`, zero-or-more `.class` tokens (source order,
//     duplicates preserved -- this parser does not dedupe, that is a semantic decision this item's
//     brief does not own), and the ONE pseudo-class this subset authorizes, `:hover` (37 measured,
//     always attached to a `.class` or `#id` compound in the real corpus, but this parser does not
//     require that -- a bare `:hover` alone, zero-measured, still structurally parses). Every field
//     empty/false is legal in principle -- `parse_compound` in parser.cpp requires at least ONE of
//     tag/id/class/hover to be present, see that function's own comment.
// PT: Uma "palavra" de uma cadeia de seletor -- tudo colado sem combinador nenhum entre si: um nome
//     de tag opcional, um `#id` opcional, zero-ou-mais tokens `.classe` (ordem-fonte, duplicatas
//     preservadas -- este parser não deduplica, é uma decisão semântica que o briefing deste item
//     não possui), e a ÚNICA pseudo-classe que este subconjunto autoriza, `:hover` (37 medidos,
//     sempre grudado num compound `.classe` ou `#id` no corpus real, mas este parser não exige isso
//     -- um `:hover` cru sozinho, zero-medido, ainda parseia estruturalmente). Todo campo
//     vazio/falso é legal em princípio -- `parse_compound` no parser.cpp exige que ao menos UM de
//     tag/id/classe/hover esteja presente, ver o próprio comentário daquela função.
struct CompoundSelector {
  std::string_view tag;                  // e.g. "div" -- empty if this compound has no tag
  std::string_view id;                   // e.g. "panel" (the '#' is NOT part of this view) -- empty if none
  std::vector<std::string_view> classes; // e.g. {"wide"} for ".wide" -- '.' NOT part of each view
  bool pseudo_hover = false;             // ":hover" was present on this compound
};

// EN: One selector out of a possibly comma-separated list -- `compounds.size() >= 1`,
//     `combinators.size() == compounds.size() - 1` (`combinators[i]` joins `compounds[i]` and
//     `compounds[i+1]`), enforced by construction (`parse_selector_chain` in parser.cpp never
//     returns a `Selector` violating this).
// PT: Um seletor de uma lista possivelmente separada-por-vírgula -- `compounds.size() >= 1`,
//     `combinators.size() == compounds.size() - 1` (`combinators[i]` une `compounds[i]` e
//     `compounds[i+1]`), garantido por construção (`parse_selector_chain` no parser.cpp nunca
//     retorna um `Selector` violando isto).
struct Selector {
  std::vector<CompoundSelector> compounds;
  std::vector<Combinator> combinators;
};

// EN: `size() >= 1` always (a `Rule` with zero selectors never registers as a `Rule` at all -- see
//     `parse_selector_list` in parser.cpp). `size() > 1` iff the source used the comma-list form
//     (15 measured, docs/rmlx-subset.md section 6.1) -- every selector in the list applies the SAME
//     declaration set, matching ordinary CSS/RCSS comma-list semantics (this parser does not
//     re-derive or question that semantics, only the SYNTAX tree that carries it forward).
// PT: `size() >= 1` sempre (uma `Rule` com zero seletores nunca chega a registrar como `Rule`
//     nenhuma -- ver `parse_selector_list` no parser.cpp). `size() > 1` sse a fonte usou a forma de
//     lista-vírgula (15 medidos, seção 6.1 do docs/rmlx-subset.md) -- todo seletor da lista aplica o
//     MESMO conjunto de declaração, casando com a semântica comum de lista-vírgula do CSS/RCSS
//     (este parser não re-deriva nem questiona essa semântica, só a árvore de SINTAXE que a carrega
//     adiante).
using SelectorList = std::vector<Selector>;

// EN: One `{name, raw-value}` pair -- see this file's header comment, "Declaration expansion",
//     for exactly how `name` ends up here (a literal longhand from the source, OR one of a
//     shorthand's own expanded constituents). `value` is NEVER parsed/validated/canonicalized by
//     this module -- see property_registry.hpp's own "not a value parser" boundary, which applies
//     identically here.
// PT: Um par `{nome, valor-cru}` -- ver o parágrafo "Expansão de declaração" do comentário de
//     cabeçalho deste arquivo pra exatamente como `name` chega aqui (um longhand literal da fonte,
//     OU um dos próprios constituintes expandidos de um shorthand). `value` NUNCA é
//     parseado/validado/canonicalizado por este módulo -- ver a própria fronteira "não é parser de
//     valor" do property_registry.hpp, que se aplica identicamente aqui.
struct PropertyDeclaration {
  std::string_view name;
  std::string_view value;
};

// EN: An ordinary RCSS rule -- `selectors` per this file's own `SelectorList` doc-comment,
//     `declarations` post-shorthand-expansion (see "Declaration expansion" above; a shorthand
//     declaration in the SOURCE produces MULTIPLE entries here, one per longhand it expands to,
//     never its own entry under its own shorthand name -- there is no `Rule::declarations` entry
//     named `"margin"`, only however many of `margin-top`/`margin-right`/`margin-bottom`/
//     `margin-left` its own value actually supplied).
// PT: Uma regra RCSS comum -- `selectors` per o próprio comentário de doc de `SelectorList` deste
//     arquivo, `declarations` pós-expansão-de-shorthand (ver "Expansão de declaração" acima; uma
//     declaração shorthand na FONTE produz MÚLTIPLAS entradas aqui, uma por longhand pro qual
//     expande, nunca a própria entrada sob o próprio nome de shorthand dela -- não existe entrada
//     de `Rule::declarations` chamada `"margin"`, só quantos de `margin-top`/`margin-right`/
//     `margin-bottom`/`margin-left` o próprio valor dela de fato supriu).
struct Rule {
  SelectorList selectors;
  std::vector<PropertyDeclaration> declarations;
};

// EN: `@font-face`'s own attribute set -- RAW, NOT run through property_registry.hpp/shorthand.hpp
//     (see this file's header comment, the 🔴 exception paragraph, for exactly why: `font-family`
//     inside `@font-face` names a font RESOURCE, not an element's style value, and `find_property`
//     would silently conflate the two if this parser ran it here; `src`/`-rmlui-fallback-face`
//     aren't in that table at all and would be wrongly dropped as "unknown property"). No dedicated
//     name-validation lives here either -- an author-mistyped `@font-face` attribute name is simply
//     an entry with a name nothing downstream recognises, a decision this item's own brief
//     ("registros estruturais... não implemente parse de valor") leaves to whichever future slice
//     actually consumes `@font-face` records (docs/uix-rcss.md section 10's own "a future
//     RMLX-2-adjacent font-resource dump... is a new decision, not a silent extension").
// PT: O próprio conjunto de atributos de `@font-face` -- CRU, NÃO rodado pelo property_registry.hpp/
//     shorthand.hpp (ver o parágrafo de exceção 🔴 do comentário de cabeçalho deste arquivo pro
//     porquê exato: `font-family` dentro de `@font-face` nomeia um RECURSO de fonte, não um valor de
//     estilo de elemento, e o `find_property` confundiria os dois em silêncio se este parser o
//     rodasse aqui; `src`/`-rmlui-fallback-face` não estão naquela tabela de jeito nenhum e seriam
//     descartados errado como "propriedade desconhecida"). Nenhuma validação de nome dedicada mora
//     aqui também -- um nome de atributo de `@font-face` digitado errado pelo autor é simplesmente
//     uma entrada com um nome que nada rio-abaixo reconhece, uma decisão que o próprio briefing
//     deste item ("registros estruturais... não implemente parse de valor") deixa pra qualquer
//     futura fatia que de fato consumir registros `@font-face` (a própria "um futuro dump de
//     recurso-de-fonte adjacente à RMLX-2... é uma decisão nova, não uma extensão silenciosa" da
//     seção 10 do docs/uix-rcss.md).
struct FontFaceRule {
  std::vector<PropertyDeclaration> attributes;
};

// EN: One `<percentage-or-from/to> { <declarations> }` body inside a `@keyframes` block.
//     `selector_text` is the RAW, whitespace-stripped-once text of the keyframe selector (`"0%"`,
//     `"50%"`, `"100%"`, `"from"`, `"to"`) -- see this file's header comment, "At-rule structuring",
//     for why this is deliberately UNRESOLVED (the 4th, timeline-offset meaning of `%`,
//     docs/uix-rcss.md section 10, out of this wave's scope). `declarations` go through the SAME
//     registry/shorthand expansion an ordinary `Rule`'s do (see that same paragraph for why: a
//     keyframe body's own properties are real, cascade-known element properties).
// PT: Um corpo `<porcentagem-ou-from/to> { <declarações> }` dentro de um bloco `@keyframes`.
//     `selector_text` é o texto CRU, whitespace-stripado uma vez, do seletor de keyframe (`"0%"`,
//     `"50%"`, `"100%"`, `"from"`, `"to"`) -- ver o parágrafo "Estruturação de at-rule" do
//     comentário de cabeçalho deste arquivo pro porquê disto ser deliberadamente NÃO-RESOLVIDO (a
//     4ª semântica de `%`, offset-de-timeline, seção 10 do docs/uix-rcss.md, fora do escopo desta
//     onda). `declarations` passa pela MESMA expansão de registro/shorthand que as de uma `Rule`
//     comum (ver o mesmo parágrafo pro porquê: as próprias propriedades de um corpo de keyframe são
//     propriedades de elemento reais, conhecidas da cascata).
struct KeyframeBlock {
  std::string_view selector_text;
  std::vector<PropertyDeclaration> declarations;
};

// EN: `name` is the animation name (`@keyframes spin { ... }` -> `"spin"`), the RAW text between
//     the at-rule's own first word ("keyframes") and its own `{` -- whitespace-stripped-once, not
//     otherwise validated (an author-mistyped or empty name is simply recorded as such; the
//     `animation` property's own cross-reference to it, docs/uix-rcss.md section 9.3, is a later
//     slice's concern). `blocks` preserves SOURCE ORDER (not sorted by percentage -- an animation
//     runtime, RMLX-8's own territory, needs the timeline in source order to resolve `from`/`to`
//     against whichever percentages the author actually wrote around them).
// PT: `name` é o nome da animação (`@keyframes spin { ... }` -> `"spin"`), o texto CRU entre a
//     própria primeira palavra do at-rule ("keyframes") e o próprio `{` dele -- whitespace-stripado
//     uma vez, não validado de outro jeito (um nome digitado errado ou vazio pelo autor é
//     simplesmente registrado como tal; a própria referência cruzada da propriedade `animation` a
//     ele, seção 9.3 do docs/uix-rcss.md, é preocupação de uma fatia posterior). `blocks` preserva a
//     ORDEM-FONTE (não ordenado por porcentagem -- um runtime de animação, território da própria
//     RMLX-8, precisa da timeline em ordem-fonte pra resolver `from`/`to` contra quaisquer
//     porcentagens que o autor de fato escreveu ao redor deles).
struct KeyframesRule {
  std::string_view name;
  std::vector<KeyframeBlock> blocks;
};

// EN: The whole parsed stylesheet. Three independent lists, NOT interleaved/merged into one --
//     `rules`/`font_faces`/`keyframes` each preserve THEIR OWN source order, but the relative order
//     ACROSS the three lists (e.g. "was this `@keyframes` block written before or after that
//     ordinary rule") is NOT recorded -- no consumer measured by this item's own corpus needs
//     cross-kind interleaving (RCSS has no scoping construct where at-rule/rule ORDER relative to
//     each OTHER's own kind matters the way `@import` order matters in full CSS, and `@import` is
//     itself zero-measured/out-of-subset, docs/rmlx-subset.md section 13).
// PT: A folha de estilo inteira, parseada. Três listas independentes, NÃO intercaladas/mescladas
//     numa só -- `rules`/`font_faces`/`keyframes` cada uma preserva a PRÓPRIA ordem-fonte, mas a
//     ordem relativa ENTRE as três listas (ex. "este bloco `@keyframes` foi escrito antes ou depois
//     daquela regra comum") NÃO é registrada -- nenhum consumidor medido pelo próprio corpus deste
//     item precisa de intercalação entre-espécies (RCSS não tem construção de escopo onde a ORDEM
//     de at-rule/regra relativa à PRÓPRIA espécie da outra importa do jeito que a ordem de
//     `@import` importa no CSS completo, e `@import` é ele próprio zero-medido/fora-de-subconjunto,
//     seção 13 do docs/rmlx-subset.md).
struct StyleSheet {
  std::vector<Rule> rules;
  std::vector<FontFaceRule> font_faces;
  std::vector<KeyframesRule> keyframes;
};

// EN: A RECOVERED-FROM issue -- sheet parsing CONTINUES past every one of these (see this file's
//     header comment, "Recovery is an acceptance criterion", for the three scopes this covers).
//     `message` names BOTH the raw, offending text AND why it was rejected (docs/uix-rcss.md
//     section 11's own minimum logging-content requirement: "the raw text of the rejected
//     construct... never a bare 'invalid RCSS' with no locatable cause"). `line`/`column`/`offset`
//     mirror the DOM sibling's own `ParseError` triple, computed the same way (see parser.cpp's own
//     `compute_line_col`).
// PT: Um problema RECUPERADO -- o parse da folha CONTINUA além de cada um destes (ver o parágrafo
//     "Recuperação é critério de aceite" do comentário de cabeçalho deste arquivo pros três escopos
//     que isto cobre). `message` nomeia TANTO o texto cru, ofensor, QUANTO o porquê da rejeição (a
//     própria exigência mínima de conteúdo de log da seção 11 do docs/uix-rcss.md: "o texto cru da
//     construção rejeitada... nunca um 'RCSS inválido' pelado sem causa localizável").
//     `line`/`column`/`offset` espelham o próprio trio `ParseError` do irmão DOM, computado do mesmo
//     jeito (ver o próprio `compute_line_col` do parser.cpp).
struct ParseDiagnostic {
  std::string message;
  std::size_t line = 0;
  std::size_t column = 0;
  std::size_t offset = 0;
};

// EN: A FATAL, unrecoverable failure -- see this file's header comment, "Recovery is an acceptance
//     criterion", last paragraph, for exactly which failures land here (a lexer-level sticky
//     `Error` ONLY) versus `ParseDiagnostic` above. Mirrors the DOM sibling's own `ParseError`
//     shape exactly, same field meanings.
// PT: Uma falha FATAL, irrecuperável -- ver o último parágrafo de "Recuperação é critério de
//     aceite" do comentário de cabeçalho deste arquivo pra exatamente quais falhas caem aqui (SÓ um
//     `Error` pegajoso de nível-lexer) versus o `ParseDiagnostic` acima. Espelha exatamente a
//     própria forma de `ParseError` do irmão DOM, mesmos significados de campo.
struct ParseError {
  std::string message;
  std::size_t line = 0;
  std::size_t column = 0;
  std::size_t offset = 0;
};

// EN: `sheet` is non-null if and only if `error` is `std::nullopt` -- same "no partial tree on
//     failure" discipline the DOM sibling's own `ParseResult` states (a fatal lexer-level `Error`
//     discards whatever partial `StyleSheet` existed, matching lexer.hpp's own sticky-`Error`
//     paragraph one layer down). `diagnostics` may be non-empty even when `sheet` is non-null (and
//     WILL be, for any real-world stylesheet exercising one of this parser's own three recovery
//     paths) -- a non-null `sheet` with non-empty `diagnostics` is this module's OWN definition of
//     "parsed successfully, with some constructs rejected", not a partial failure.
// PT: `sheet` é não-nulo se e somente se `error` for `std::nullopt` -- mesma disciplina "nenhuma
//     árvore parcial na falha" que o próprio `ParseResult` do irmão DOM declara (um `Error` fatal de
//     nível-lexer descarta qualquer `StyleSheet` parcial que existia, casando com o próprio
//     parágrafo de `Error` pegajoso do lexer.hpp uma camada abaixo). `diagnostics` pode ser
//     não-vazio mesmo quando `sheet` é não-nulo (e VAI ser, pra qualquer folha de estilo do mundo
//     real que exercitar um dos próprios três caminhos de recuperação deste parser) -- um `sheet`
//     não-nulo com `diagnostics` não-vazio é a PRÓPRIA definição deste módulo de "parseou com
//     sucesso, com algumas construções rejeitadas", não uma falha parcial.
struct SheetParseResult {
  std::unique_ptr<StyleSheet> sheet;
  std::vector<ParseDiagnostic> diagnostics;
  std::optional<ParseError> error;
};

// EN: Parses `source` (a complete, in-memory RCSS stylesheet -- a standalone `.rcss` file's own
//     content, OR the raw text glintfx/src/uix/dom/'s own `<head>`/`<style>` extraction hands this
//     module, per docs/uix-rcss.md section 1's own "computed values" framing) into a `StyleSheet`.
//     ⚠️ `source` MUST outlive every `StyleSheet` this function returns -- see this file's header
//     comment, "Zero-copy, unlike the DOM sibling", for why this differs from
//     glintfx::uix::parse_document's own weaker "only needs to stay alive for the duration of this
//     call" contract.
// PT: Parseia `source` (uma folha de estilo RCSS completa, em memória -- o próprio conteúdo de um
//     arquivo `.rcss` standalone, OU o texto cru que a própria extração de `<head>`/`<style>` do
//     glintfx/src/uix/dom/ entrega a este módulo, per a própria formulação "valores computados" da
//     seção 1 do docs/uix-rcss.md) num `StyleSheet`. ⚠️ `source` PRECISA sobreviver a todo
//     `StyleSheet` que esta função retornar -- ver o parágrafo "Zero-cópia, diferente do irmão DOM"
//     do comentário de cabeçalho deste arquivo pro porquê disto divergir do próprio contrato mais
//     fraco "só precisa ficar viva pela duração desta chamada" do glintfx::uix::parse_document.
SheetParseResult parse_stylesheet(std::string_view source);

// EN: `UIX-INLINE-STYLE` -- `source` is a `style="..."` attribute's own raw VALUE (never the whole
//     `style="..."` token, never wrapped in a synthetic selector) -- the entry point a `style`
//     attribute needs, mirroring real upstream's own SEPARATE `StyleSheetParser::ParseProperties`
//     (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`, called directly from
//     `Element::OnAttributeChange`'s own `"style"` branch, never through `Parse()`/a synthetic
//     rule). See `docs/uix-rcss.md`'s `UIX-RCSS-ERRATA-6` for the full contract this closes
//     (precedence, malformed-declaration handling, byte-exact dump appearance).
//
//     Reuses this file's OWN registry/shorthand declaration-expansion machinery
//     (`apply_declaration`, this file's "Declaration expansion" paragraph) UNCHANGED -- a
//     `style="..."` attribute's own grammar is a flat `name: value;` run, exactly what
//     `Lexer(source, /*start_in_declaration_mode=*/true)` (lexer.hpp) scans directly, with NO
//     enclosing `{`/`}` of its own -- so this function is NOT `parse_stylesheet` wrapping `source`
//     in a synthetic rule (which would shift every diagnostic's own `offset`/`line`/`column` by the
//     wrapper's own length, and would need an always-valid dummy selector this module has no
//     principled way to invent); it drives the SAME lexer directly, one token at a time, the same
//     shape `parser.cpp`'s own `.cpp`-local `collect_declarations` already has for an ordinary
//     rule/keyframe-block body, restated here because that method's own postcondition (consuming a
//     terminating `BraceClose`) does not fit a buffer that never has one.
//
//     RECOVERY: identical discipline to `parse_stylesheet`'s own declaration-scoped recovery (this
//     file's header, scope (a)) -- an unknown property name or a malformed shorthand value is
//     dropped, with a `ParseDiagnostic` naming the raw offending text, and EVERY OTHER declaration
//     in this SAME attribute still applies; a genuinely malformed *lexical* construct (an
//     unterminated quoted value, matching `Lexer`'s own sticky-`Error` contract) is the only path
//     that reaches `error` (`sheet`-less `declarations`, matching `SheetParseResult`'s own "no
//     partial tree on failure" discipline). A stray top-level `}` (nothing meaningful can open one
//     inside a bare declaration list) is diagnosed and skipped, the SAME declaration-scoped
//     recovery, never fatal.
//
//     ⚠️ `source` MUST outlive every entry of the returned `declarations` -- identical zero-copy
//     contract to `parse_stylesheet`'s own (this file's header, "Zero-copy, unlike the DOM
//     sibling"), because `PropertyDeclaration::name`/`::value` are `std::string_view`s into it.
// PT: `UIX-INLINE-STYLE` -- `source` é o próprio VALOR cru de um atributo `style="..."` (nunca o
//     token `style="..."` inteiro, nunca envolvido num seletor sintético) -- o ponto de entrada que
//     um atributo `style` precisa, espelhando o próprio `StyleSheetParser::ParseProperties`
//     SEPARADO do upstream real (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`, chamado direto
//     do próprio ramo `"style"` do `Element::OnAttributeChange`, nunca através do `Parse()`/uma
//     regra sintética). Ver a `UIX-RCSS-ERRATA-6` do `docs/uix-rcss.md` pro contrato completo que
//     isto fecha (precedência, tratamento de declaração malformada, aparência byte-exata no dump).
//
//     Reusa a PRÓPRIA maquinaria de expansão-de-declaração por registro/shorthand deste arquivo
//     (`apply_declaration`, o próprio parágrafo "Expansão de declaração" deste arquivo) SEM
//     mudança -- a própria gramática de um atributo `style="..."` é um trecho plano `nome: valor;`,
//     exatamente o que `Lexer(source, /*start_in_declaration_mode=*/true)` (lexer.hpp) escaneia
//     direto, SEM `{`/`}` envolvente nenhum próprio -- então esta função NÃO é o `parse_stylesheet`
//     envolvendo `source` numa regra sintética (o que deslocaria o próprio `offset`/`line`/`column`
//     de todo diagnóstico pelo próprio comprimento do envelope, e precisaria de um seletor-dummy
//     sempre-válido que este módulo não tem jeito principiado de inventar); ela dirige o MESMO
//     lexer direto, um token de cada vez, a mesma forma que o próprio `collect_declarations` local-
//     do-.cpp do parser.cpp já tem pro corpo de uma regra/bloco-de-keyframe comum, restatada aqui
//     porque a própria pós-condição daquele método (consumir um `BraceClose` terminador) não cabe
//     num buffer que nunca tem um.
//
//     RECUPERAÇÃO: disciplina idêntica à própria recuperação escopada-por-declaração do
//     `parse_stylesheet` (o cabeçalho deste arquivo, escopo (a)) -- um nome de propriedade
//     desconhecido ou um valor de shorthand malformado é descartado, com um `ParseDiagnostic`
//     nomeando o texto cru ofensor, e TODA OUTRA declaração NESTE MESMO atributo ainda se aplica;
//     uma construção genuinamente malformada no nível LÉXICO (um valor entre aspas não-terminado,
//     casando com o próprio contrato de `Error` pegajoso do `Lexer`) é o único caminho que alcança
//     `error` (`declarations` sem `sheet`, casando com a própria disciplina "nenhuma árvore parcial
//     na falha" do `SheetParseResult`). Um `}` de topo-de-nível solto (nada com sentido consegue
//     abrir um dentro de uma lista de declaração nua) é diagnosticado e pulado, a MESMA recuperação
//     escopada-por-declaração, nunca fatal.
//
//     ⚠️ `source` PRECISA sobreviver a toda entrada de `declarations` retornado -- contrato
//     zero-cópia idêntico ao próprio `parse_stylesheet` (o cabeçalho deste arquivo, "Zero-cópia,
//     diferente do irmão DOM"), porque `PropertyDeclaration::name`/`::value` são `std::string_view`s
//     sobre ele.
struct InlineStyleParseResult {
  std::vector<PropertyDeclaration> declarations;
  std::vector<ParseDiagnostic> diagnostics;
  std::optional<ParseError> error;
};

InlineStyleParseResult parse_inline_style(std::string_view source);

} // namespace glintfx::uix::style
