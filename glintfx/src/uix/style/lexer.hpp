// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-LEXER -- tokenizer (lexer) for the RCSS subset frozen by docs/rmlx-subset.md and
//     narrowed by docs/uix-rcss.md. This is the FIRST brick of glintfx's own style engine, the
//     RCSS mirror of glintfx/src/uix/dom/lexer.hpp (RML's own S1): a mechanical byte-stream-to-
//     token-stream pass, with NO tree, NO cascade, NO selector matching, NO property registry.
//     Read glintfx/src/uix/dom/lexer.hpp's own header comment FIRST -- this file assumes that
//     one's "Token model"/"Hardening"/"Error stickiness" conventions as already-established house
//     style and only re-derives what genuinely differs for RCSS's own grammar.
//
//     SCOPE, corpus-driven per this task's own "o corpus decide" clause
//     (`/var/tmp/censo-rcss-qa1/censo.md`, 62 source files, 866 style blocks, 3424 declarations)
//     and real-upstream-RmlUi-source-driven (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`,
//     read in full for this slice -- every design decision below cites the exact function/line
//     range it is mirroring, not a paraphrase):
//       - a STRUCTURAL layer recognising exactly 3 delimiters -- '{', '}', '@' -- with everything
//         else swept into a raw `Prelude` token (selector text OR an at-rule's own name+params,
//         UNPARSED -- this lexer does not know ".foo"/"#bar"/">"/","/":hover" mean anything;
//         that is selector-matching, explicitly out of this item's scope). Mirrors upstream's own
//         `StyleSheetParser::Parse`'s `FindAnyToken(pre_token_str, "{@}")` loop (:709-920) exactly
//         at this granularity.
//       - a DECLARATION layer, entered only via a `{` that upstream's own `ReadProperties`
//         (:969-1071) would also enter with (see "At-rule mode table" below for the one place this
//         lexer needs a fact upstream's OWN tokenizer also needs), producing one `Declaration`
//         token per `name: value` pair -- bundled into ONE token, mirroring the DOM sibling's own
//         `Attr` bundling reasoning (that file's "Token model" paragraph): a property name is
//         never meaningful without its value in this grammar either.
//       - comments (`/* ... */`), tokenized and handed to the caller as their own `Comment` token
//         -- a DELIBERATE, DECLARED divergence from upstream's own character-level elision, see
//         "Comment handling" below.
//
//     DELIBERATELY NOT THIS MODULE'S JOB (a future RMLX-2 parser/cascade/selector-matcher/
//     property-registry slice's territory, not merely "later" -- each one is a *specific*, named
//     reason this layer must not guess, same discipline the DOM sibling's header already applies):
//       - Selector parsing/matching (`.foo`, `#bar`, descendant/child combinators, `:hover`, the
//         comma-separated list form the líder authorized into the RMLX-2 subset -- TODO.md
//         `RMLX-2` decision 1, censo.md section 1.2's own 15 measured instances). This lexer's
//         `Prelude` token carries the RAW SOURCE BYTES, unparsed, exactly like the DOM sibling's
//         `TagOpenStart`'s tag-name payload carries a raw identifier without validating it means
//         anything to a stylesheet.
//       - Property-value parsing/validation (colors, lengths, the 3-families-of-`%` distinction
//         docs/uix-rcss.md section 5 spells out, `box-shadow`'s own color-first/comma-layered
//         grammar, `decorator`'s 5 sub-languages). This lexer's `Declaration.value` is the RAW,
//         whitespace-stripped-only source text -- see "Não pode engasgar" below for the concrete
//         proof this still works for the corpus's richest, most nested values.
//       - Property registry / cascade / specificity / inheritance -- none of this lexer's concern;
//         it does not even know "color" and "colour-typo" are different in kind.
//       - `@media`/`@import`/`@charset`/`@supports` real semantics -- zero-measured in the census
//         (section 0) and docs/rmlx-subset.md section 13's own real-zero exclusion; a `{` after
//         any at-rule OTHER than the case-sensitive literal "keyframes" defaults to Declaration-
//         mode scanning, the SAME default an ordinary rule gets -- see "At-rule mode table" below
//         for the full, DECLARED (not silent) teto and its consequence for a hypothetical `@media`
//         body with nested rules (would tokenize WRONG, not crash -- documented, not guessed).
//
//     TOKEN MODEL: one `Token` per logical fact, atomic per `next()` call. `Declaration` bundles
//     BOTH the property name and its (whitespace-stripped, quotes-NOT-stripped) value into ONE
//     token -- see this paragraph's own opening citation of the DOM sibling's identical reasoning
//     for `Attr`. Quotes are deliberately NOT stripped from `Declaration.value` (unlike the DOM
//     sibling's `Attr.value`): RCSS quoting is only meaningful for `string`-domain properties
//     (`font-family`, `cursor`, `-rmlui-fallback-face`'s sibling `src`) and a future parser slice
//     needs to be able to tell "declaration written with quotes" from "declaration written bare"
//     to apply the right value-domain grammar (docs/uix-rcss.md section 7's own `string` row: "no
//     surrounding quotes even if the RCSS source quoted it (quoting is source syntax, not part of
//     the computed string value)" -- that stripping is EXPLICITLY a later, semantic step this
//     lexer does not own, matching the same "not this layer's job" reasoning the entity-decoding
//     paragraph of the DOM sibling's own header already established for RML). All `Token` fields
//     reference the ORIGINAL source buffer via `std::string_view` (zero-copy) -- the caller must
//     keep that buffer alive for the lifetime of every `Token` it reads.
//
//     NAMESPACE: `glintfx::uix::style`, NOT the flat `glintfx::uix` the DOM sibling uses.
//     Deliberate: `Lexer`/`Token`/`TokenKind` are the SAME identifiers the DOM module already
//     declares in `glintfx::uix` -- flattening this module into the same namespace would compile
//     fine TODAY (the two are separate standalone CMake projects, never linked into one binary
//     yet) but would collide the moment a later wave (RMLX-10/11, per both modules' own
//     CMakeLists.txt header comments) folds both into the main `glintfx` static target. Nesting
//     under `style` now, matching this module's own directory name (`glintfx/src/uix/style/`),
//     costs nothing today and avoids a forced rename later.
//
//     UTF-8: byte-wise, never decodes UTF-8 -- same "every multi-byte sequence is numerically
//     disjoint from every ASCII delimiter this grammar recognises" reasoning the DOM sibling's own
//     header gives (`{`, `}`, `@`, `:`, `;`, `"`, `/`, `*` are all < 0x80, disjoint from every
//     UTF-8 continuation/lead byte, 0x80-0xFF).
//
//     HARDENING (fail-high, same discipline as the DOM sibling's own two ceilings): two
//     independently-derived, named ceilings, re-derived from the RCSS-specific corpus rather than
//     reused blindly from the DOM sibling's own (different-grammar) measurements:
//       - `kMaxInputBytes` (1 MiB) -- the whole source buffer handed to one `Lexer`. Derived: the
//         corpus's largest real single `<style>` payload is 30626 bytes
//         (`system_menu__config_categorias.rml`, measured via a Python sweep of every
//         `<style>...</style>` span under `glintfx/src/uix/dom/test_fixtures/`+`glintfx/tests/`+
//         `glintfx/tests/ui/`), and the largest standalone `.rcss` file is 7632 bytes
//         (`glintfx/tests/echo_scene.rcss`, `wc -c`) -- 1 MiB is ~34x the largest real payload
//         measured. Checked ONCE at construction, same sticky-`Error`-at-offset-0 contract as the
//         DOM sibling.
//       - `kMaxTokenBytes` (64 KiB) -- any single `Prelude`/`Declaration`-value/`Comment` payload.
//         The corpus's longest single line inside any measured `<style>` block is 147 bytes
//         (`system_menu__config_audio_sliders.rml`) -- 64 KiB is ~445x that. Same
//         belt-and-suspenders-distinct-from-`kMaxInputBytes` reasoning as the DOM sibling: bounds
//         the cost of one hostile/malformed run (an unterminated quote, an unterminated comment)
//         to a fixed scan length.
//       - `kMaxNestDepth` (64) -- a ceiling THIS module's own grammar needs that the DOM sibling's
//         does not: the mode stack (see "Keyframes nesting" below) can only grow via the ONE
//         mechanism a `@keyframes`-named at-rule's `{` triggers, but nothing stops hostile input
//         from nesting `@keyframes` inside `@keyframes` bodies arbitrarily deep. The real corpus
//         never needs more than 2 levels (`gusworld_battle_cockpit.rml:214`,
//         `@keyframes spin { from {...} to {...} }`) -- 64 is a 32x margin, fail-high (sticky
//         `Error`) beyond it, same "reject, never silently truncate/resync" discipline as the two
//         byte ceilings. Proven by `lexer_hardening_sanity.cpp`'s own
//         `test_nest_depth_ceiling`.
//     All three are FAIL-HIGH: reject and emit a diagnosable `Error` token, never truncate-and-
//     continue, same reasoning the DOM sibling's own "Hardening" paragraph already gives.
//
//     ERROR STICKINESS / EOF STICKINESS: identical contract to the DOM sibling -- once `next()`
//     returns `Error`, every subsequent call returns the SAME `Error` token forever; every call
//     past the end of input returns the same `EndOfFile` token forever. This lexer never attempts
//     to resync after malformed input, same "fail-high, not fail-plausible" reasoning.
//
//     EOF PERMISSIVENESS, AND WHERE THIS MODULE IS STRICTER THAN UPSTREAM: real upstream RmlUi's
//     own `ReadProperties` (`StyleSheetParser.cpp`:1058-1070) is lenient about a file that ends
//     mid-declaration with no trailing `;` -- its own post-loop code
//     (`if (state == VALUE && !name.empty() && !value.empty())`) still parses the dangling last
//     declaration. This module makes TWO DIFFERENT, DELIBERATE choices, stated so a reader does
//     not have to reverse-engineer them from test behaviour:
//       (a) An UNCLOSED top-level block (a `{` that is never matched by a `}` anywhere before EOF,
//           but every declaration INSIDE it was itself well-formed) is NOT an error -- this
//           module's own "permissive, documented characteristic", the SAME kind of leniency
//           precedent the DOM sibling's own header names (there: no-whitespace-between-attributes;
//           here: a different leniency, but the SAME discipline of "the corpus never needs the
//           rejection, and the leniency introduces no parsing ambiguity" -- an unclosed block at
//           EOF is unambiguously "everything up to here was fine, then the file just stopped").
//           Proven by `lexer_hardening_sanity.cpp`'s own `test_unclosed_block_at_eof_is_not_an_error`.
//       (b) A declaration whose OWN name or value scan genuinely never finds its own terminator
//           before EOF (no `:` ever found for a name; an opened `"` never closed; a value that
//           never reaches `;`/`}`) IS a fail-high `Error` -- STRICTER than upstream's own
//           leniency, a deliberate divergence (not a silent gap): the real corpus is exclusively
//           well-formed, complete files, so this stricter rule costs nothing measured today, and
//           it keeps this lexer's own general "unterminated construct = Error" discipline (already
//           applied to comments, matching the DOM sibling's own unterminated-comment case)
//           internally consistent rather than carving out an upstream-shaped exception. Proven by
//           `lexer_hardening_sanity.cpp`'s own `test_unterminated_quoted_value_is_error` and
//           `test_declaration_name_runs_off_eof_without_colon_is_error`.
//
//     COMMENT HANDLING -- A REPORTED, NOT SILENTLY DECIDED, DIVERGENCE FROM UPSTREAM'S OWN
//     CHARACTER-LEVEL ELISION: real upstream's own `ReadCharacter` (`StyleSheetParser.cpp`:
//     1218-1289) strips `/* ... */` at the LOWEST possible layer, one character at a time, BELOW
//     every other state machine in the file (`FindAnyToken`, `ReadProperties`) -- the comment
//     literally never exists as bytes by the time either of those functions sees them. This has a
//     genuinely surprising consequence: a comment can SPLICE two adjacent fragments together with
//     ZERO bytes between them (`wid/*x*/th` tokenizes as the single identifier "width", not two
//     fragments with a gap). This module makes a DIFFERENT choice: `/* ... */` is its OWN,
//     diagnosable `Comment` token, mirroring the DOM sibling's own `<!-- -->` `Comment`-token
//     design rather than upstream's byte-splicing. Two consequences, both DECLARED here rather
//     than discovered by a future reader diffing behaviour:
//       (1) This lexer is scoped NARROWER than upstream in one specific, corpus-unverified way: a
//           comment is only recognised as its own token at a "fresh scan start" -- immediately
//           after a structural delimiter (`{`/`}`/`@`), immediately after a completed `Declaration`
//           (right where the next NAME would start), or at the very start of a `Prelude`/`Comment`
//           dispatch. A `/*...*/` appearing MID-RUN, after this lexer has already started
//           accumulating a `Prelude`/name/value run, is NOT specially recognised -- its bytes
//           ('/','*',...,'*','/') simply become part of that run's raw text, UNLIKE upstream (which
//           would elide it regardless of position, even inside what looks like an identifier or a
//           quoted value). ZERO corpus evidence exists either way (no real fixture places a comment
//           mid-identifier or mid-value) -- this is the FIRST of the ambiguities this task's own
//           brief invites reporting rather than guessing silently.
//       (2) A byte-exact reconstruction of "what upstream's own elided prelude/value string would
//           have been" is NOT this lexer's job -- every `Token`'s `offset`/`length` are exposed
//           (same "S3 does its own raw slicing" precedent the DOM sibling's own head-blob-opacity
//           paragraph already establishes) specifically so a future parser slice CAN reconstruct
//           it directly from the source buffer if upstream's exact splicing behaviour ever turns
//           out to matter for a real fixture.
//
//     AT-RULE MODE TABLE -- THE ONE PLACE THIS LEXER NEEDS A FACT UPSTREAM'S OWN TOKENIZER ALSO
//     NEEDS ("keyframes nesting"): a `{` can open EITHER a flat Declaration block (the
//     overwhelming majority: ordinary rules, `@font-face`, and every unmeasured at-rule this table
//     does not name) OR a SECOND, nested Structural (Prelude-scanning) region whose OWN inner `{`s
//     are what actually open Declaration blocks -- and telling the two apart REQUIRES knowing the
//     at-rule identifier, a fact bracket-depth ALONE cannot supply. This is not invented: reading
//     upstream directly (`StyleSheetParser::Parse`, :773-920) shows its OWN tokenizer making the
//     IDENTICAL decision at the IDENTICAL layer -- `State::AtRuleIdentifier`'s own branch
//     (`if (at_rule_identifier == "keyframes") state = State::KeyframeBlock;`, :786-789) is a
//     literal, CASE-SENSITIVE string compare (no `ToLower` call, unlike RML's own tag-name folding
//     the DOM sibling's `UIX-LEXER-OPACO` paragraph cites) against the FIRST whitespace-delimited
//     word of the at-rule's own prelude text (`pre_token_str.substr(0, pre_token_str.find(' '))`,
//     :783 -- note: splits on a LITERAL SPACE byte 0x20 only, not the general whitespace set; a tab
//     after `@keyframes` would NOT match upstream's own check either, matched here for fidelity).
//     Concretely proven load-bearing, not a nicety: WITHOUT this special case, this exact real
//     corpus line (`gusworld_battle_cockpit.rml:214`,
//     `@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }`)
//     cannot be tokenized without desyncing brace balance -- the outer `{` would be mis-scanned as
//     opening a Declaration block, `0% {`/`from {` would corrupt the declaration-NAME scan (upstream's
//     own NAME state has no `{`-terminator either, so it would just become part of an ever-growing,
//     garbage "name"), and the FIRST inner `}` would prematurely end the whole scan while the outer
//     `}` is left dangling. See `lexer_hardening_sanity.cpp`'s own
//     `test_keyframes_special_case_is_load_bearing` for the end-to-end, exact-token-count proof.
//
//       Mode selected by a `{`, keyed by the case-sensitive first word of the immediately-preceding
//       `Prelude` token IF that `Prelude` immediately followed an `At` token (cleared/consumed
//       exactly once per `{`, mirroring the DOM sibling's own `pending_tag_name_` "read back by the
//       VERY NEXT" precedent):
//         | First word after `@`         | Mode this `{` opens                                    |
//         | :---------------------------- | :------------------------------------------------------ |
//         | `keyframes` (exact case)      | Structural (nested) -- another Prelude/BraceOpen/At/BraceClose region, matching upstream's own `State::KeyframeBlock` |
//         | anything else, OR no `@` at all (ordinary rule) | Declaration -- flat `name: value;` scanning, matching upstream's own `ReadProperties` default |
//       DECLARED, DELIBERATE TETO (mirrors the DOM sibling's own `UIX-LEXER-OPACO` "exactly two tag
//       names, a third needs its own corpus-measured justification" discipline, narrowed to ONE
//       name here because the census measures exactly one at-rule needing this treatment):
//         - Only "keyframes" enters nested-Structural mode. `@font-face` (20 measured instances,
//           census section 7) needs NO special casing -- it already gets the Declaration-mode
//           default, matching upstream's own direct `ReadProperties` call for it (:855-863).
//         - `@media`/`@import`/`@charset`/`@supports` are ZERO-measured (census section 0,
//           docs/rmlx-subset.md section 13's own real-zero exclusion) and get the SAME
//           Declaration-mode default as any unrecognised at-rule -- this is a KNOWN, NAMED
//           limitation, not silently guessed: a hypothetical `@media { .foo { color: red; } }`
//           would mis-tokenize its nested rule as if it were a single flat declaration (the same
//           failure mode analysed two paragraphs above for what would happen to `@keyframes`
//           without its own special case). The fix, if a real fixture ever needs it, is the exact
//           same move `UIX-LEXER-OPACO` already established for the DOM sibling: "stop, add a name
//           to this table, corpus-justified" -- never silently widen the default.
//         - This is the SECOND of the ambiguities this task's own brief invites reporting: whether
//           at-rule-name-driven mode dispatch belongs in THIS lexer at all (a lexical, bytes-only
//           decision, matching upstream's own tokenizer-level placement) or should instead be
//           deferred to a future parser slice (the position this item's own brief takes for
//           selector matching and property registration) is a real, structural question this
//           document resolves ONE way (lexer-level, mirroring upstream's own placement and the DOM
//           sibling's `UIX-LEXER-OPACO` precedent) but does not claim is the only defensible
//           answer -- flagged for the líder/reviewer rather than assumed uncontroversial.
// PT: UIX-RCSS-LEXER -- tokenizador (lexer) para o subconjunto de RCSS congelado pelo
//     docs/rmlx-subset.md e estreitado pelo docs/uix-rcss.md. Este é o PRIMEIRO tijolo do motor de
//     estilo próprio da glintfx, o espelho em RCSS do glintfx/src/uix/dom/lexer.hpp (a própria S1
//     do RML): uma passada mecânica de fluxo-de-bytes pra fluxo-de-tokens, SEM árvore, SEM
//     cascata, SEM casamento de seletor, SEM registro de propriedade. Leia PRIMEIRO o próprio
//     comentário de cabeçalho do glintfx/src/uix/dom/lexer.hpp -- este arquivo assume as
//     convenções "Modelo de token"/"Hardening"/"Pegajosidade de erro" daquele como estilo da casa
//     já estabelecido e só re-deriva o que genuinamente diverge pra gramática própria do RCSS.
//
//     ESCOPO, guiado por corpus conforme a própria cláusula "o corpus decide" desta tarefa
//     (`/var/tmp/censo-rcss-qa1/censo.md`, 62 arquivos-fonte, 866 blocos de estilo, 3424
//     declarações) e guiado pelo fonte real do RmlUi upstream
//     (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`, lido por inteiro pra esta fatia -- toda
//     decisão de desenho abaixo cita o intervalo exato de função/linha que está espelhando, não
//     uma paráfrase):
//       - uma camada ESTRUTURAL reconhecendo exatamente 3 delimitadores -- '{', '}', '@' -- com
//         todo o resto varrido pra um token `Prelude` cru (texto de seletor OU o próprio
//         nome+parâmetros de um at-rule, NÃO-PARSEADO -- este lexer não sabe que ".foo"/"#bar"/
//         ">"/","/":hover" significam algo; isso é casamento-de-seletor, explicitamente fora do
//         escopo deste item). Espelha o próprio laço `FindAnyToken(pre_token_str, "{@}")`
//         (:709-920) do `StyleSheetParser::Parse` do upstream exatamente nesta granularidade.
//       - uma camada de DECLARAÇÃO, entrada só via um `{` que o próprio `ReadProperties` (:969-1071)
//         do upstream TAMBÉM entraria (ver "Tabela de modo de at-rule" abaixo pro único lugar em
//         que este lexer precisa de um fato que o PRÓPRIO tokenizador do upstream também precisa),
//         produzindo um token `Declaration` por par `nome: valor` -- empacotado num ÚNICO token,
//         espelhando o próprio raciocínio de empacotamento do `Attr` do irmão DOM (parágrafo
//         "Modelo de token" daquele arquivo): um nome de propriedade também nunca é significativo
//         sem seu valor nesta gramática.
//       - comentários (`/* ... */`), tokenizados e entregues ao chamador como o PRÓPRIO token
//         `Comment` -- uma divergência DELIBERADA, DECLARADA da própria elisão de nível-caractere
//         do upstream, ver "Trato de comentário" abaixo.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE MÓDULO (território de uma futura fatia
//     parser/cascata/casador-de-seletor/registro-de-propriedade da RMLX-2, não só "depois" -- cada
//     um é um motivo *específico*, nomeado, de esta camada não poder chutar, mesma disciplina que
//     o cabeçalho do irmão DOM já aplica):
//       - Parse/casamento de seletor (`.foo`, `#bar`, combinadores descendente/filho, `:hover`, a
//         forma de lista separada-por-vírgula que o líder autorizou no subconjunto da RMLX-2 --
//         decisão 1 da entrada `RMLX-2` do TODO.md, as próprias 15 instâncias medidas da seção 1.2
//         do censo.md). O token `Prelude` deste lexer carrega os BYTES-FONTE CRUS, não-parseados,
//         exatamente como o payload de nome-de-tag do `TagOpenStart` do irmão DOM carrega um
//         identificador cru sem validar que significa algo pra uma folha de estilo.
//       - Parse/validação de valor de propriedade (cores, comprimentos, a distinção de 3 famílias
//         de `%` que a seção 5 do docs/uix-rcss.md descreve, a própria gramática cor-primeiro/
//         camadas-por-vírgula do `box-shadow`, as 5 sub-linguagens do `decorator`). O
//         `Declaration.value` deste lexer é o texto-fonte CRU, só whitespace-stripado -- ver "Não
//         pode engasgar" abaixo pra prova concreta de que isto ainda funciona pros valores mais
//         ricos e aninhados do corpus.
//       - Registro de propriedade / cascata / especificidade / herança -- nada disso é preocupação
//         deste lexer; ele nem sabe que "color" e "colour-typo" são diferentes em espécie.
//       - Semântica real de `@media`/`@import`/`@charset`/`@supports` -- zero-medidos no censo
//         (seção 0) e a própria exclusão real-zero da seção 13 do docs/rmlx-subset.md; um `{` depois
//         de qualquer at-rule QUE NÃO o literal case-sensitive "keyframes" cai no default de scan
//         em modo Declaration, o MESMO default que uma regra comum recebe -- ver "Tabela de modo de
//         at-rule" abaixo pro teto completo, DECLARADO (não silencioso), e sua consequência pra um
//         `@media` hipotético com regras aninhadas (tokenizaria ERRADO, não travaria -- documentado,
//         não chutado).
//
//     MODELO DE TOKEN: um `Token` por fato lógico, atômico por chamada de `next()`. `Declaration`
//     empacota TANTO o nome da propriedade QUANTO seu valor (whitespace-stripado, aspas NÃO
//     removidas) num ÚNICO token -- ver a citação de abertura deste parágrafo do mesmo raciocínio
//     do irmão DOM pro `Attr`. Aspas são deliberadamente NÃO removidas de `Declaration.value`
//     (diferente do `Attr.value` do irmão DOM): quoting em RCSS só é significativo pra propriedades
//     de domínio `string` (`font-family`, `cursor`, o irmão `src` do `-rmlui-fallback-face`) e uma
//     futura fatia de parser precisa conseguir distinguir "declaração escrita com aspas" de
//     "declaração escrita crua" pra aplicar a gramática de domínio-de-valor certa (a própria linha
//     `string` da seção 7 do docs/uix-rcss.md: "sem aspas ao redor mesmo se a fonte RCSS tiver
//     citado (quoting é sintaxe-fonte, não parte do valor de string computado)" -- essa remoção é
//     EXPLICITAMENTE um passo semântico, posterior, que este lexer não possui, casando com o mesmo
//     raciocínio "não é trabalho desta camada" que o parágrafo de decodificação-de-entidade do
//     próprio cabeçalho do irmão DOM já estabeleceu pro RML). Todo campo de `Token` referencia o
//     buffer-fonte ORIGINAL via `std::string_view` (zero-cópia) -- o chamador precisa manter esse
//     buffer vivo pela duração de vida de todo `Token` que ler.
//
//     NAMESPACE: `glintfx::uix::style`, NÃO o `glintfx::uix` plano que o irmão DOM usa.
//     Deliberado: `Lexer`/`Token`/`TokenKind` são os MESMOS identificadores que o módulo DOM já
//     declara em `glintfx::uix` -- achatar este módulo no mesmo namespace compilaria bem HOJE (os
//     dois são projetos CMake standalone separados, nunca linkados num binário só ainda) mas
//     colidiria no momento em que uma onda posterior (RMLX-10/11, pelo próprio comentário de
//     cabeçalho do CMakeLists.txt dos dois módulos) dobrar os dois no alvo static `glintfx`
//     principal. Aninhar sob `style` agora, casando com o próprio nome de diretório deste módulo
//     (`glintfx/src/uix/style/`), não custa nada hoje e evita um rename forçado depois.
//
//     UTF-8: byte-a-byte, nunca decodifica UTF-8 -- mesmo raciocínio "toda sequência multi-byte é
//     numericamente disjunta de todo delimitador ASCII que esta gramática reconhece" que o próprio
//     cabeçalho do irmão DOM dá (`{`, `}`, `@`, `:`, `;`, `"`, `/`, `*` são todos < 0x80,
//     disjuntos de todo byte de continuação/líder UTF-8, 0x80-0xFF).
//
//     HARDENING (fail-high, mesma disciplina dos dois tetos do próprio irmão DOM): dois tetos
//     derivados de forma independente e nomeados, re-derivados do corpus específico de RCSS em vez
//     de reusados às cegas das medições (de gramática diferente) do irmão DOM:
//       - `kMaxInputBytes` (1 MiB) -- o buffer-fonte inteiro entregue a um `Lexer`. Derivado: o
//         maior payload real de `<style>` único do corpus são 30626 bytes
//         (`system_menu__config_categorias.rml`, medido por uma varredura Python de todo trecho
//         `<style>...</style>` sob `glintfx/src/uix/dom/test_fixtures/`+`glintfx/tests/`+
//         `glintfx/tests/ui/`), e o maior arquivo `.rcss` standalone são 7632 bytes
//         (`glintfx/tests/echo_scene.rcss`, `wc -c`) -- 1 MiB é ~34x o maior payload real medido.
//         Checado UMA VEZ na construção, mesmo contrato de `Error` pegajoso no offset 0 do irmão
//         DOM.
//       - `kMaxTokenBytes` (64 KiB) -- qualquer payload único de `Prelude`/valor-de-`Declaration`/
//         `Comment`. A maior linha única dentro de qualquer bloco `<style>` medido do corpus são
//         147 bytes (`system_menu__config_audio_sliders.rml`) -- 64 KiB é ~445x isso. Mesmo
//         raciocínio belt-and-suspenders-distinto-do-`kMaxInputBytes` do irmão DOM: limita o custo
//         de um único trecho hostil/malformado (uma aspa não-fechada, um comentário não-fechado) a
//         um comprimento de scan fixo.
//       - `kMaxNestDepth` (64) -- um teto que a PRÓPRIA gramática deste módulo precisa e o irmão
//         DOM não: a pilha de modo (ver "Aninhamento de keyframes" abaixo) só consegue crescer
//         pelo ÚNICO mecanismo que um at-rule nomeado `@keyframes` dispara no próprio `{`, mas nada
//         impede input hostil de aninhar `@keyframes` dentro de corpos de `@keyframes`
//         arbitrariamente fundo. O corpus real nunca precisa de mais que 2 níveis
//         (`gusworld_battle_cockpit.rml:214`, `@keyframes spin { from {...} to {...} }`) -- 64 é
//         uma margem de 32x, fail-high (`Error` pegajoso) além disso, mesma disciplina
//         "rejeita, nunca trunca/ressincroniza em silêncio" dos dois tetos de byte. Provado pelo
//         próprio `test_nest_depth_ceiling` do lexer_hardening_sanity.cpp.
//     Os três são FAIL-HIGH: rejeitam e emitem um token `Error` diagnosticável, nunca
//     truncam-e-continuam, mesmo raciocínio que o próprio parágrafo "Hardening" do irmão DOM já dá.
//
//     PEGAJOSIDADE DE ERRO / PEGAJOSIDADE DE EOF: contrato idêntico ao irmão DOM -- uma vez que
//     `next()` retorna `Error`, toda chamada subsequente retorna o MESMO token `Error` pra sempre;
//     toda chamada além do fim do input retorna o mesmo token `EndOfFile` pra sempre. Este lexer
//     nunca tenta ressincronizar depois de input malformado, mesmo raciocínio "fail-high, não
//     fail-plausível".
//
//     PERMISSIVIDADE DE EOF, E ONDE ESTE MÓDULO É MAIS ESTRITO QUE O UPSTREAM: o próprio
//     `ReadProperties` (`StyleSheetParser.cpp`:1058-1070) do RmlUi upstream real é leniente com um
//     arquivo que termina no meio de uma declaração sem `;` final -- o próprio código pós-laço dele
//     (`if (state == VALUE && !name.empty() && !value.empty())`) ainda parseia a última declaração
//     pendurada. Este módulo faz DUAS escolhas DIFERENTES, deliberadas, declaradas pra um leitor
//     não precisar fazer engenharia reversa a partir do comportamento do teste:
//       (a) Um bloco de topo-de-nível NÃO-FECHADO (um `{` nunca casado por um `}` em lugar nenhum
//           antes do EOF, mas toda declaração DENTRO dele era ela própria bem-formada) NÃO é erro
//           -- a própria "característica permissiva e documentada" deste módulo, o MESMO tipo de
//           precedente de leniência que o próprio cabeçalho do irmão DOM nomeia (lá:
//           sem-whitespace-entre-atributos; aqui: uma leniência diferente, mas a MESMA disciplina
//           de "o corpus nunca precisa da rejeição, e a leniência não introduz ambiguidade de
//           parse" -- um bloco não-fechado no EOF é inequivocamente "tudo até aqui estava bem,
//           depois o arquivo simplesmente parou"). Provado pelo próprio
//           `test_unclosed_block_at_eof_is_not_an_error` do lexer_hardening_sanity.cpp.
//       (b) Uma declaração cujo PRÓPRIO scan de nome ou valor genuinamente nunca acha o PRÓPRIO
//           terminador antes do EOF (nenhum `:` nunca achado pro nome; uma `"` aberta nunca
//           fechada; um valor que nunca chega em `;`/`}`) É um `Error` fail-high -- MAIS ESTRITO
//           que a própria leniência do upstream, uma divergência deliberada (não uma lacuna
//           silenciosa): o corpus real é exclusivamente de arquivos completos, bem-formados,
//           então esta regra mais estrita não custa nada medido hoje, e mantém a disciplina geral
//           "construção não-terminada = Error" deste lexer (já aplicada a comentários, casando com
//           o próprio caso de comentário não-terminado do irmão DOM) internamente consistente em
//           vez de abrir uma exceção no formato do upstream. Provado pelo próprio
//           `test_unterminated_quoted_value_is_error` e
//           `test_declaration_name_runs_off_eof_without_colon_is_error` do
//           lexer_hardening_sanity.cpp.
//
//     TRATO DE COMENTÁRIO -- UMA DIVERGÊNCIA REPORTADA, NÃO DECIDIDA EM SILÊNCIO, DA PRÓPRIA
//     ELISÃO DE NÍVEL-CARACTERE DO UPSTREAM: o próprio `ReadCharacter` (`StyleSheetParser.cpp`:
//     1218-1289) do upstream real remove `/* ... */` na camada mais baixa possível, um caractere
//     por vez, ABAIXO de toda outra máquina de estado do arquivo (`FindAnyToken`, `ReadProperties`)
//     -- o comentário literalmente nunca existe como bytes no momento em que qualquer uma das duas
//     funções o vê. Isto tem uma consequência genuinamente surpreendente: um comentário pode
//     EMENDAR dois trechos adjacentes com ZERO bytes entre eles (`wid/*x*/th` tokeniza como o
//     identificador único "width", não dois trechos com um vão). Este módulo faz uma escolha
//     DIFERENTE: `/* ... */` é o PRÓPRIO token `Comment`, diagnosticável, espelhando o próprio
//     desenho de token-`Comment` do `<!-- -->` do irmão DOM em vez da emenda-por-byte do upstream.
//     Duas consequências, as duas DECLARADAS aqui em vez de descobertas por um leitor futuro
//     comparando comportamentos:
//       (1) Este lexer é escopado MAIS ESTREITO que o upstream de um jeito específico,
//           não-verificado-por-corpus: um comentário só é reconhecido como o próprio token num
//           "início de scan fresco" -- logo depois de um delimitador estrutural (`{`/`}`/`@`), logo
//           depois de uma `Declaration` completa (bem onde o próximo NOME começaria), ou bem no
//           início de um dispatch de `Prelude`/`Comment`. Um `/*...*/` que aparece NO MEIO de um
//           trecho, depois de este lexer já ter começado a acumular um trecho de `Prelude`/
//           nome/valor, NÃO é especialmente reconhecido -- os bytes dele ('/','*',...,'*','/')
//           simplesmente viram parte do texto cru daquele trecho, DIFERENTE do upstream (que
//           elidiria independente de posição, mesmo dentro do que parece um identificador ou um
//           valor entre aspas). ZERO evidência de corpus existe dos dois lados (nenhuma fixture
//           real põe um comentário no meio de um identificador ou de um valor) -- esta é a
//           PRIMEIRA das ambiguidades que o próprio briefing desta tarefa convida a reportar em vez
//           de chutar em silêncio.
//       (2) Uma reconstrução byte-exata de "qual teria sido a string de prelúdio/valor elidida do
//           próprio upstream" NÃO é trabalho deste lexer -- todo `offset`/`length` de `Token` são
//           expostos (mesmo precedente "a S3 faz a própria fatia crua" que o próprio parágrafo de
//           opacidade-de-blob-de-head do irmão DOM já estabelece) especificamente pra uma futura
//           fatia de parser CONSEGUIR reconstruir isso direto do buffer-fonte se o comportamento
//           exato de emenda do upstream algum dia se provar importar pra uma fixture real.
//
//     TABELA DE MODO DE AT-RULE -- O ÚNICO LUGAR EM QUE ESTE LEXER PRECISA DE UM FATO QUE O
//     PRÓPRIO TOKENIZADOR DO UPSTREAM TAMBÉM PRECISA ("aninhamento de keyframes"): um `{` pode
//     abrir OU UM bloco de Declaration plano (a esmagadora maioria: regras comuns, `@font-face`, e
//     todo at-rule não-medido que esta tabela não nomeia) OU uma SEGUNDA região Estrutural
//     (scanning de Prelude) aninhada cujos `{`s PRÓPRIOS de dentro é que de fato abrem blocos de
//     Declaration -- e distinguir os dois EXIGE saber o identificador do at-rule, um fato que
//     profundidade-de-chave SOZINHA não consegue fornecer. Isto não é inventado: ler o upstream
//     direto (`StyleSheetParser::Parse`, :773-920) mostra o PRÓPRIO tokenizador dele tomando a
//     decisão IDÊNTICA na camada IDÊNTICA -- o próprio ramo do `State::AtRuleIdentifier`
//     (`if (at_rule_identifier == "keyframes") state = State::KeyframeBlock;`, :786-789) é uma
//     comparação de string literal, CASE-SENSITIVE (sem chamada `ToLower`, diferente da própria
//     dobra de nome-de-tag do RML que o parágrafo `UIX-LEXER-OPACO` do irmão DOM cita) contra a
//     PRIMEIRA palavra delimitada-por-espaço do próprio texto de prelúdio do at-rule
//     (`pre_token_str.substr(0, pre_token_str.find(' '))`, :783 -- nota: divide num byte de ESPAÇO
//     LITERAL 0x20 só, não no conjunto geral de whitespace; uma tabulação depois de `@keyframes`
//     também NÃO casaria com o próprio check do upstream, casado aqui por fidelidade). Provado
//     concretamente como portante, não capricho: SEM este caso especial, esta linha exata de
//     corpus real (`gusworld_battle_cockpit.rml:214`,
//     `@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }`)
//     não consegue ser tokenizada sem desincronizar o balanço de chaves -- o `{` externo seria
//     scaneado errado como abrindo um bloco de Declaration, `from {`/o `{` de `to` corromperiam o
//     scan de NOME-de-declaração (o próprio estado NAME do upstream também não tem terminador `{`,
//     então simplesmente viraria parte de um "nome" lixo, sempre crescendo), e o PRIMEIRO `}`
//     interno terminaria o scan inteiro prematuramente enquanto o `}` externo ficaria pendurado.
//     Ver o próprio `test_keyframes_special_case_is_load_bearing` do lexer_hardening_sanity.cpp
//     pra prova ponta-a-ponta, com contagem exata de token.
//
//       Modo selecionado por um `{`, chaveado pela primeira palavra case-sensitive do token
//       `Prelude` imediatamente anterior SE aquele `Prelude` seguiu imediatamente um token `At`
//       (limpo/consumido exatamente uma vez por `{`, espelhando o próprio precedente "lido de volta
//       só pelo PRÓXIMO imediato" do `pending_tag_name_` do irmão DOM):
//         | Primeira palavra depois de `@`  | Modo que este `{` abre                                |
//         | :------------------------------- | :------------------------------------------------------ |
//         | `keyframes` (caixa exata)         | Estrutural (aninhado) -- outra região Prelude/BraceOpen/At/BraceClose, casando com o próprio `State::KeyframeBlock` do upstream |
//         | qualquer outra coisa, OU nenhum `@` (regra comum) | Declaration -- scan plano `nome: valor;`, casando com o próprio default do `ReadProperties` do upstream |
//       TETO DECLARADO, DELIBERADO (espelha a própria disciplina "exatamente dois nomes de tag, um
//       terceiro precisa da própria justificativa medida-por-corpus" do `UIX-LEXER-OPACO` do irmão
//       DOM, estreitada pra UM nome aqui porque o censo mede exatamente um at-rule precisando deste
//       tratamento):
//         - Só "keyframes" entra em modo Estrutural aninhado. `@font-face` (20 instâncias medidas,
//           seção 7 do censo) NÃO precisa de tratamento especial nenhum -- já recebe o default de
//           modo Declaration, casando com a própria chamada direta de `ReadProperties` do upstream
//           pra ele (:855-863).
//         - `@media`/`@import`/`@charset`/`@supports` são ZERO-medidos (seção 0 do censo, a própria
//           exclusão real-zero da seção 13 do docs/rmlx-subset.md) e recebem o MESMO default de
//           modo Declaration que qualquer at-rule não-reconhecido -- isto é uma limitação
//           CONHECIDA, NOMEADA, não chutada em silêncio: um `@media { .foo { color: red; } }`
//           hipotético tokenizaria a regra aninhada dele errado, como se fosse uma declaração plana
//           única (o mesmo modo de falha analisado dois parágrafos acima pro que aconteceria com
//           `@keyframes` sem o próprio caso especial). O conserto, se uma fixture real algum dia
//           precisar, é o EXATO MESMO movimento que o `UIX-LEXER-OPACO` já estabeleceu pro irmão
//           DOM: "para, soma um nome nesta tabela, justificado por corpus" -- nunca alargar o
//           default em silêncio.
//         - Esta é a SEGUNDA das ambiguidades que o próprio briefing desta tarefa convida a
//           reportar: se o dispatch de modo guiado por nome-de-at-rule pertence a ESTE lexer (uma
//           decisão lexical, só-de-bytes, casando com o próprio posicionamento de nível-tokenizador
//           do upstream) ou deveria em vez disso ser adiado pra uma futura fatia de parser (a
//           posição que o próprio briefing deste item toma pra casamento de seletor e registro de
//           propriedade) é uma pergunta real, estrutural, que este documento resolve de UM jeito
//           (nível-lexer, espelhando o próprio posicionamento do upstream e o precedente
//           `UIX-LEXER-OPACO` do irmão DOM) mas não afirma ser a única resposta defensável --
//           sinalizado pro líder/revisor em vez de assumido incontroverso.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace glintfx::uix::style {

// EN: See this file's header comment, "Hardening" paragraph, for the derivation of all three
//     values.
// PT: Ver o parágrafo "Hardening" do comentário de cabeçalho deste arquivo pra derivação dos três
//     valores.
inline constexpr std::size_t kMaxInputBytes = 1024 * 1024; // 1 MiB
inline constexpr std::size_t kMaxTokenBytes = 64 * 1024;   // 64 KiB
inline constexpr std::size_t kMaxNestDepth = 64;

// EN: See this file's header comment, "Token model" paragraph. `At`/`BraceOpen`/`BraceClose` carry
//     no payload (`text`/`value` empty); `Prelude`/`Comment`/`Error` use `text` for their single
//     payload; `Declaration` uses both `text` (the property NAME, whitespace-stripped) and `value`
//     (the property VALUE, whitespace-stripped, quotes NOT stripped -- see header for why).
// PT: Ver o parágrafo "Modelo de token" do comentário de cabeçalho deste arquivo. `At`/
//     `BraceOpen`/`BraceClose` não carregam payload (`text`/`value` vazios); `Prelude`/`Comment`/
//     `Error` usam `text` pro único payload; `Declaration` usa tanto `text` (o NOME da propriedade,
//     whitespace-stripado) quanto `value` (o VALOR da propriedade, whitespace-stripado, aspas NÃO
//     removidas -- ver cabeçalho pro porquê).
enum class TokenKind {
  At,          // '@'              -- marks the start of an at-rule's own prelude
  Prelude,     // raw text run     -- text = selector list OR at-rule name+params, byte-verbatim
  BraceOpen,   // '{'              -- opens a Declaration block or a nested Structural region
  BraceClose,  // '}'              -- closes whichever region was open
  Declaration, // name: value;     -- text = property name, value = property value (both stripped)
  Comment,     // '/*' ... '*/'    -- text = raw content between the delimiters
  EndOfFile,   // sticky terminal state, see header comment "Error stickiness / EOF stickiness"
  Error,       // sticky terminal state -- text = human-readable diagnostic
};

struct Token {
  TokenKind kind = TokenKind::EndOfFile;
  std::string_view text;  // see TokenKind's own comments above for what this holds per kind
  std::string_view value; // Declaration only; empty for every other kind
  std::size_t offset = 0; // byte offset into the source buffer where this token begins
  std::size_t length = 0; // byte length of this token's full source span
};

// EN: Stateful, single-pass tokenizer over a caller-owned `std::string_view`. `source` must
//     outlive every `Token` this instance returns (all `Token` fields are views into it, zero-
//     copy -- see this file's header comment). Not safe under concurrent use; this project's
//     library-wide single-threaded-no-internal-lock convention applies (see the DOM sibling's own
//     citation of glintfx/include/glintfx/log.hpp's THREADING note for the precedent) -- one
//     `Lexer` instance is driven by exactly one thread.
// PT: Tokenizador com estado, passada única, sobre um `std::string_view` de posse do chamador.
//     `source` precisa sobreviver a todo `Token` que esta instância retornar (todo campo de
//     `Token` é uma view sobre ele, zero-cópia -- ver o comentário de cabeçalho deste arquivo).
//     Não é seguro sob uso concorrente; a convenção da biblioteca inteira de
//     single-threaded-sem-lock-interno se aplica (ver a própria citação do irmão DOM da nota
//     THREADING do glintfx/include/glintfx/log.hpp pro precedente) -- uma instância de `Lexer` é
//     dirigida por exatamente uma thread.
class Lexer {
public:
  // EN: If `source.size() > kMaxInputBytes`, this constructor does NOT throw or abort -- it
  //     records the failure internally so the FIRST `next()` call returns `Error` (offset 0),
  //     same fail-high discipline as the DOM sibling's own `Lexer::Lexer`.
  // PT: Se `source.size() > kMaxInputBytes`, este construtor NÃO lança nem aborta -- registra a
  //     falha internamente pra que a PRIMEIRA chamada de `next()` retorne `Error` (offset 0),
  //     mesma disciplina fail-high do próprio `Lexer::Lexer` do irmão DOM.
  explicit Lexer(std::string_view source);

  // EN: Returns the next token in the stream. See this file's header comment, "Error stickiness /
  //     EOF stickiness" paragraph.
  // PT: Retorna o próximo token do fluxo. Ver o parágrafo "Pegajosidade de erro / Pegajosidade de
  //     EOF" do comentário de cabeçalho deste arquivo.
  Token next();

private:
  // EN: See this file's header comment, "At-rule mode table" paragraph. `Structural` is the
  //     Prelude/BraceOpen/At/BraceClose scanning region (top-level, and the SECOND, nested region
  //     a `@keyframes`-named at-rule's own `{` opens); `Declaration` is the flat `name: value;`
  //     scanning region. `mode_stack_` always starts with exactly one `Structural` entry (the
  //     top-level/root region) and is never popped below that one entry.
  // PT: Ver o parágrafo "Tabela de modo de at-rule" do comentário de cabeçalho deste arquivo.
  //     `Structural` é a região de scan de Prelude/BraceOpen/At/BraceClose (topo-de-nível, e a
  //     SEGUNDA região, aninhada, que o próprio `{` de um at-rule nomeado `@keyframes` abre);
  //     `Declaration` é a região de scan plano `nome: valor;`. `mode_stack_` sempre começa com
  //     exatamente uma entrada `Structural` (a região topo-de-nível/raiz) e nunca é esvaziada
  //     abaixo dessa entrada.
  enum class Mode { Structural,
                    Declaration };

  Token make_error(std::size_t offset, std::string_view message);

  // EN: Scans one Structural-region token: Comment, At, BraceOpen (pushes a child Mode -- see
  //     "At-rule mode table"), BraceClose (pops, floor at mode_stack_.size()==1), or a Prelude raw
  //     run (stopping before '{', '}', '@', or the start of "/*").
  // PT: Escaneia um token de região Estrutural: Comment, At, BraceOpen (empilha um Mode filho --
  //     ver "Tabela de modo de at-rule"), BraceClose (desempilha, piso em
  //     mode_stack_.size()==1), ou um trecho cru de Prelude (parando antes de '{', '}', '@', ou o
  //     início de "/*").
  Token scan_structural();

  // EN: Scans one Declaration-region token: Comment, BraceClose (pops), or a Declaration (name-
  //     run to ':', then quote-aware value-run to ';'/'}') -- loops internally (never recurses) to
  //     silently skip empty declarations (stray ';'/';;'), matching upstream's own NAME-state
  //     empty-name handling. See "EOF permissiveness" for the two EOF policies this function
  //     applies.
  // PT: Escaneia um token de região Declaration: Comment, BraceClose (desempilha), ou uma
  //     Declaration (trecho-de-nome até ':', depois trecho-de-valor ciente-de-aspas até ';'/'}')
  //     -- itera internamente (nunca recursa) pra pular declarações vazias em silêncio (';'/';;'
  //     soltos), casando com o próprio tratamento de nome-vazio do estado NAME do upstream. Ver
  //     "Permissividade de EOF" pras duas políticas de EOF que esta função aplica.
  Token scan_declaration();

  // EN: Shared by both scan_structural() and scan_declaration(): if source_[pos_..] starts with
  //     "/*", scans to the matching "*/" (kMaxTokenBytes-bounded, EOF-before-close is Error) and
  //     returns a Comment token; otherwise returns false (caller proceeds with its own scan).
  // PT: Compartilhado por scan_structural() e scan_declaration(): se source_[pos_..] começa com
  //     "/*", escaneia até o "*/" correspondente (limitado por kMaxTokenBytes, EOF-antes-do-
  //     fechamento é Error) e retorna um token Comment; senão retorna false (o chamador segue com
  //     o próprio scan).
  bool try_scan_comment(Token* out);

  std::string_view source_;
  std::size_t pos_ = 0;
  bool done_ = false;
  Token sticky_{}; // valid only once done_ (Error or natural EOF reached)

  std::vector<Mode> mode_stack_;

  // EN: True right after an At token; consumed (cleared) by the VERY NEXT Prelude token, which
  //     computes pending_at_rule_word_ from its own raw text if this flag is set. See "At-rule
  //     mode table" for the exact algorithm (first literal-space-delimited word).
  // PT: Verdadeiro logo depois de um token At; consumido (limpo) pelo PRÓXIMO token Prelude
  //     imediato, que computa pending_at_rule_word_ a partir do próprio texto cru se esta flag
  //     estiver setada. Ver "Tabela de modo de at-rule" pro algoritmo exato (primeira palavra
  //     delimitada por espaço literal).
  bool awaiting_at_rule_word_ = false;

  // EN: The most recently captured at-rule first-word. Consumed EXACTLY ONCE: the very next
  //     BraceOpen decides its mode from this field, THEN clears it to empty (which never equals
  //     "keyframes", so it safely defaults to Declaration mode for every later, unrelated
  //     BraceOpen) -- same "read back by the VERY NEXT" one-shot precedent as the DOM sibling's
  //     own `pending_tag_name_`.
  // PT: A primeira-palavra de at-rule capturada mais recentemente. Consumida EXATAMENTE UMA VEZ: o
  //     próprio próximo BraceOpen decide o modo dele a partir deste campo, DEPOIS o limpa pra
  //     vazio (que nunca é igual a "keyframes", então cai com segurança no default de modo
  //     Declaration pra todo BraceOpen posterior, não-relacionado) -- mesmo precedente de
  //     "lido de volta só pelo PRÓXIMO imediato", uso-único, do próprio `pending_tag_name_` do
  //     irmão DOM.
  std::string_view pending_at_rule_word_;
};

} // namespace glintfx::uix::style
