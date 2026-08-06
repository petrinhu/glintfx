// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S1 -- tokenizer (lexer) for the RML subset frozen by docs/rmlx-subset.md. This is
//     the FIRST brick of glintfx's own DOM: a mechanical byte-stream-to-token-stream pass, with
//     NO tree, NO attribute/id/class semantics, NO <head>-opacity special-casing. Those are S2
//     (node tree), S3 (recursive-descent parser that consumes this token stream and builds the
//     tree), and later waves' jobs -- see this header's "Deliberately NOT this module's job"
//     section below for the exact boundary and why each item is deferred, not merely postponed
//     by omission.
//
//     SCOPE, taken from the RMLX-1/S1 brief and docs/rmlx-subset.md, corpus-driven (glintfx's own
//     44 `.rml` fixtures under glintfx/tests/ + glintfx/demos/, none over ~8 KB, longest single
//     line 230 bytes):
//       - opening tag start (`<name`), attribute (`name="value"` / `name='value'`, both quote
//         styles -- explicitly named in the brief, "aspas simples e duplas"), opening-tag close
//         (`>`), self-close (`/>`), closing tag (`</name>`).
//       - text runs between tags (byte-verbatim, see "Entity decoding" below for what this means
//         for `&nbsp;`/`&amp;`, the only two entities the corpus uses).
//       - comments (`<!-- ... -->`), tokenized and handed to the caller, but NOT part of the DOM
//         this arc builds (real RmlUi's own XML parser has no comment-to-Element path either --
//         see docs/rmlx-subset.md's own `<head>` opacity evidence for the sibling case of "this
//         markup construct never becomes a live node"). S3 is expected to discard them.
//     Every one of these is corpus-measured: `git grep`/`awk` runs against every `.rml` fixture
//     in this repo are cited inline below at each design decision this module made because of
//     them, per this brief's own "o corpus decide" clause -- so a reader two waves from now can
//     re-run the same one-liner instead of trusting a paraphrase.
//
//     DELIBERATELY NOT THIS MODULE'S JOB (S2/S3/RMLX-2+ territory, not merely "later" -- each one
//     is a *specific*, named reason this layer must not guess):
//       - Entity decoding (`&nbsp;` -> U+00A0, `&amp;` -> `&`). docs/rmlx-subset.md ("uix-dom.md"
//         hereafter) section 6(c) says decoding happens "at text-node-construction time" -- that
//         is node/tree work, not byte-stream tokenization. This lexer's `Text`/`Attr`-value
//         payloads are the RAW source bytes, entities un-decoded, exactly like the `HEAD PRESENT`
//         payload in uix-dom.md section 4 (which is explicitly exempt from decoding for the same
//         "this is not node construction yet" reason). Decoding a FIXED table (`amp`/`nbsp`, the
//         only two the corpus uses per `grep -oh '&[a-zA-Z#0-9]*;' glintfx/tests/*.rml
//         glintfx/demos/**/*.rml | sort -u` => exactly `&amp;` and `&nbsp;`, nothing else, no
//         numeric character references) INSIDE the lexer would be the smaller, cheaper
//         implementation, but it would also silently commit S1 to a decoding policy (what does an
//         UNRECOGNISED entity do -- reject the byte stream? pass it through raw? half-decode?)
//         that uix-dom.md assigns to a LATER construction step for a documented reason. Left as
//         an explicit open point for S2/S3 below.
//       - The whitespace-only-text EXISTENCE filter (uix-dom.md section 6(a): a text node made of
//         nothing but the 4-char whitespace set is never created, full stop, and never occupies a
//         child-index slot). This lexer emits EVERY text run, including pure-whitespace ones --
//         applying the filter here would require this layer to already know about "child index
//         slots", which only exist once there is a tree (S2). S3 applies uix-dom.md 6(a) when it
//         turns this token stream into nodes.
//       - `<head>` content opacity (uix-dom.md section 4: `<head>`'s children are RAW, UNPARSED,
//         UN-ENTITY-DECODED bytes from just after `<head>`'s `>` to just before `</head>`'s `<`,
//         never walked as tags). This lexer does NOT special-case the tag name `head` -- it
//         tokenizes `<head>...</head>` exactly like any other element, recursing into `<style>`/
//         `<link/>`/`<title>` as ordinary nested tags. The decision of WHETHER to consume this
//         lexer's normal token stream inside `<head>`, or instead bypass it and slice the raw
//         source directly (which is what uix-dom.md's `HEAD PRESENT` payload actually needs) is
//         S3's to make -- `parser.cpp`'s `handle_head()`/`find_head_close()` took route (b) (see
//         parser.hpp's own header comment), UNCHANGED by the paragraph below. Do not confuse this
//         (head-BLOB opacity, still not this lexer's concern) with `<style>`/`<script>`-CONTENT
//         opacity, which this lexer DOES now own -- see "RESOLVED (UIX-LEXER-OPACO)" further down
//         this file for that narrower, tag-name-scoped, orthogonal decision.
//       - DOCTYPE / XML declaration (`<?xml ... ?>`) / CDATA (`<![CDATA[ ... ]]>`). Zero
//         occurrences in the corpus (`grep -l '<!\[CDATA\|<!DOCTYPE\|<?xml' glintfx/tests/*.rml
//         glintfx/demos/**/*.rml` => no matches). Encountering `<!` NOT followed by `--`, or `<?`,
//         is an `Error` token (fail-high, see "Hardening" below) -- this is the concrete
//         mechanism of docs/rmlx-subset.md's own header clause ("pare e reporte -- nunca
//         implemente por conta própria"): a real fixture that needs one of these stops the S1
//         lexer cold with a diagnosable token instead of silently mis-tokenizing.
//       - Bare/unclosed void elements (HTML5's `<br>`/`<input>` without a trailing `/`). Zero
//         occurrences: every void-shaped element in the corpus is self-closed explicitly
//         (`grep -on '<br[^/]' ...` / `grep -on '<link[^>]*[^/]>' ...` both empty; `<br/>`,
//         `<input .../>`, `<link .../>` are the only forms present). This lexer therefore has NO
//         void-element name list at all (that would be exactly the "let's build HTML5" scope-
//         creep this arc's own governing ADR-0020 names as the risk `RMLX-3` most needs guarding
//         against) -- self-closing syntax alone (`/>`) decides self-closure, uniformly, for every
//         tag name.
//       - Namespaced names (`xml:lang`, `xlink:href`). Zero occurrences of `:` inside a tag or
//         attribute name anywhere in the corpus. Not part of this lexer's identifier grammar (see
//         `is_name_start`/`is_name_char` in lexer.cpp) -- a `:` inside what looks like a name
//         simply ends the name early and the following byte is scanned as whatever it turns out
//         to be (almost certainly a hostile/malformed-input `Error`).
//       - Tag-name matching between an opening and its closing tag (`<div>...</span>`). Requires
//         a stack, which requires a tree -- S2/S3's job. This lexer accepts a mismatched
//         `TagClose` name as a perfectly ordinary token; see lexer_tokens_sanity.cpp's
//         `test_mismatched_close_name_not_validated` for a locked-in proof of this exact
//         boundary.
//
//     TOKEN MODEL: one `Token` per logical fact, atomic per `next()` call -- notably `Attr` bundles
//     BOTH the attribute name and its (already quote-stripped) value into ONE token instead of
//     splitting them across two `next()` calls the way a maximally-minimal state machine could.
//     This is a deliberate ergonomics choice for S3 (the consumer): an attribute name is never
//     meaningful without its value in this grammar (RML/RCSS has no valueless/boolean attribute
//     anywhere in the corpus -- `grep` for a bare `<tag attr>` with no `=` inside any `.rml`
//     fixture found zero), so splitting them into two tokens would only add inter-call state
//     ("did I just see a name, am I now expecting `=value`?") that both this lexer AND every
//     consumer would have to track redundantly. All `Token` fields reference the ORIGINAL source
//     buffer via `std::string_view` (zero-copy) -- the caller must keep that buffer alive for the
//     lifetime of every `Token` it reads.
//
//     UTF-8: this lexer operates byte-wise and never decodes UTF-8 -- every multi-byte UTF-8
//     sequence's continuation bytes (0x80-0xBF) and lead bytes (0xC0-0xFF) are numerically
//     disjoint from every ASCII delimiter this grammar recognises (`<`, `>`, `/`, `=`, `"`, `'`,
//     `!`, `-`, and the 4-char whitespace set), so scanning raw bytes is safe without a UTF-8
//     decoder -- the exact same reasoning uix-dom.md section 2 gives for why its own escaping
//     rule ("every UTF-8 multi-byte sequence... passes through unchanged") needs no Unicode
//     awareness either.
//
//     HARDENING (fail-high, same discipline as `decorator_polygon.cpp`'s `lados ∈ [3,1024]` /
//     `log.hpp`'s `kLogMaxMessageBytes` -- see this project's "input validation em borda"
//     convention): a parser is attacker-facing surface even before there is a tree to attack.
//     Two independently-derived, named ceilings:
//       - `kMaxInputBytes` (1 MiB) -- the WHOLE source buffer. Derived, not invented: the
//         corpus's largest real fixture is 7957 bytes (`fonteng_colr_scene.rml`, measured via
//         `find glintfx/tests glintfx/demos -name '*.rml' -exec wc -c {} \; | sort -n | tail -1`)
//         -- 1 MiB is ~132x that, and GusWorld's own `battle_cockpit_rml.cpp` (the "representative
//         screen", uix-dom.md/docs/rmlx-subset.md section 2 -- ~1.6x the runner-up's markup, C++
//         raw-string-embedded, not directly measurable from this repo) is expected to stay well
//         inside that same order of magnitude. Checked ONCE at construction (`Lexer::Lexer`);
//         exceeding it makes the FIRST `next()` call return `Error` immediately, offset 0.
//       - `kMaxTokenBytes` (64 KiB) -- any SINGLE token's payload (tag/attr name, attr value, text
//         run, comment body). ~285x the corpus's longest observed single line (230 bytes,
//         `fonteng_sup_scene.rml`, `awk '{print length}' ... | sort -rn | head -1`). This is
//         belt-and-suspenders distinct from `kMaxInputBytes`: it bounds the cost of a single
//         malformed/hostile token (an unterminated quote, an unterminated comment) to a fixed
//         scan length instead of running to the end of the (already-capped) buffer before
//         reporting failure. A token that WOULD exceed this ceiling is rejected outright (`Error`)
//         -- NEVER split across multiple `Text`/other tokens to work around it (a split would
//         force every consumer to know "two adjacent same-kind tokens with nothing between them
//         means merge them", an inter-token merge signal this lexer's contract does not offer).
//     Both are FAIL-HIGH: reject and emit a diagnosable `Error` token, never truncate-and-
//     continue (a truncated tag name or attribute value that silently "still parses" would produce
//     a tree that looks plausible but is WRONG -- worse than an obvious, loud failure).
//
//     ERROR STICKINESS: once `next()` returns `Error` (whatever the cause), EVERY subsequent call
//     returns the SAME `Error` token (identical `text`/`offset`/`length`) forever -- this lexer
//     never attempts to "resync" and keep producing tokens after malformed input. A resyncing
//     lexer that guesses where the NEXT well-formed construct starts can silently skip real
//     content and hand S3 a tree that is subtly wrong instead of a document that visibly failed to
//     parse -- the same "fail-high, not fail-plausible" reasoning this file's Hardening paragraph
//     already applies to the two byte ceilings, applied here to error recovery itself. Same
//     sticky contract for `EndOfFile`: every `next()` call past the end of input returns the same
//     `EndOfFile` token.
//
//     PERMISSIVE, DOCUMENTED CHARACTERISTIC -- no whitespace required between two adjacent
//     attributes (`<div id="x"class="y">` tokenizes as two ordinary `Attr` tokens, not an
//     `Error`). Real strict-XML grammar requires a separating whitespace before every attribute;
//     this lexer does not enforce it because (a) the corpus never needs the rejection, and (b) the
//     leniency introduces no parsing AMBIGUITY -- the closing quote of the previous attribute's
//     value unambiguously ends it, so the next identifier character can only start a new
//     attribute name, never anything else. See lexer_hardening_sanity.cpp's
//     `test_permissive_no_whitespace_between_attrs` for the locked-in proof. If a future fixture
//     needs the strict rejection, it is a small, local addition (track whether at least one
//     whitespace byte was skipped since the last significant token) -- deliberately not added
//     pre-emptively, per this file's own "the corpus decides" discipline.
//
//     HEAD-BLOB OPEN POINT, RESOLVED BY S3 (kept for history): `<head>` opacity (uix-dom.md
//     section 4) needs RAW, UN-TOKENIZED byte access from just after `<head>`'s `>` to just before
//     `</head>`'s `<`. This lexer's public surface is a plain `next()` token stream with no such
//     "give me the raw span instead" escape hatch -- S3 took route (b) below: `parser.cpp`'s
//     `handle_head()` constructs a SEPARATE, throwaway substring scan (`find_head_close()`) over
//     the original source buffer using the `TagOpenStart{text="head"}` token's own `offset`/
//     `length` fields as a starting point, bypassing this `Lexer` instance entirely for that one
//     span, then REBUILDS a fresh `Lexer` over the substring past `</head>`'s `>` (see parser.hpp's
//     own header comment). This header exposes `offset`/`length` on every `Token` specifically so
//     that route stays possible without modifying this file.
//
//     RESOLVED (UIX-LEXER-OPACO): `<style>`/`<script>` ARE CDATA-LIKE AT THIS TOKENIZER, MIRRORING
//     UPSTREAM -- a DIFFERENT, narrower, orthogonal decision from the head-BLOB one just above (do
//     not conflate the two: this one is tag-name-scoped and fires WHEREVER `<style>`/`<script>`
//     appears, inside `<head>` or not; the one above is `<head>`-scoped and knows nothing about
//     tag names inside it). The question this item was raised to settle -- "whose responsibility:
//     a tolerant lexer, or a parser that assumes an opaque scan?" -- has a third, simpler answer,
//     found by reading real upstream RmlUi instead of picking between the two framed options:
//     upstream's OWN tokenizer decides this, not a caller reaching in above it. `Factory.cpp:255-
//     257` registers exactly two tag names, "script" and "style", as persistent CDATA tags
//     (`XMLParser::RegisterPersistentCDATATag`); `BaseXMLParser.cpp`'s own tokenizer
//     (`ReadElement`, :243-262) checks `IsCDATATag` right after the opening tag's `>` and, if it
//     matches, calls `ReadCDATA` (:349-390) instead of resuming ordinary tag scanning -- the SAME
//     layer that recognises `<`/`>` delimiters in the first place switches its OWN mode, on a
//     specific tag name, without any escape hatch or bypass mechanism needed on its public
//     surface. This lexer now does the identical thing: `scan_in_tag()`'s `TagOpenEnd` branch
//     checks whether the tag name captured at the matching `TagOpenStart` (`pending_tag_name_`)
//     ASCII-case-folds to "style" or "script"; if so, `state_` becomes `RawCData` (not `Text`),
//     and the NEXT `next()` call runs `scan_raw_cdata()`: a byte-wise forward scan (same shape as
//     `scan_comment()`'s own `while (!starts_with_at(...))` loop, same `kMaxTokenBytes` ceiling
//     applied to the accumulated span) that treats every byte as literal content until it finds
//     the case-folded literal `"</style"`/`"</script"`, then hands control BACK to the ordinary
//     tokenizer at that exact byte (`state_ = State::Text`) -- the closing tag itself is then
//     tokenized completely normally by the EXISTING, UNMODIFIED `scan_tag_close()` (whitespace and
//     `>` still validated, still fail-high on a malformed closer). This fix touches nothing about
//     HOW a tag is recognised; it only changes WHEN raw bytes are handed out as one `Text` token
//     instead of being re-entered as tag grammar.
//
//     Why the LEXER and not a caller-side raw scan (the shape this repo's own twin fix,
//     `glintfx/src/rml/dom_dump.cpp`'s `locate_head_tag_span()`, and this repo's own
//     `parser.cpp`'s `handle_head()`/`find_head_close()`, both use): `<style>`/`<script>` opacity
//     is not scoped to `<head>` at all -- upstream's CDATA-tag registration is UNCONDITIONAL,
//     wherever either tag appears -- so a lexer that only tolerated it inside `<head>` would still
//     choke on a hypothetical (today, zero-occurrence at the top level) `<style>` inside `<body>`.
//     Putting the mode switch at the layer that already owns `<`/`>` recognition needs no new
//     escape hatch on this class's public surface (no `raw_until()` method, contra the OLD "Open
//     point for S2/S3" text this paragraph replaces) and composes for free with every EXISTING
//     caller of this `Lexer` -- including `parser.cpp`'s own post-`</head>` lexer rebuild, which
//     re-enters ordinary `State::Text` scanning on a substring that could itself contain a
//     `<style>`/`<script>` tag at the top level.
//
//     DECLARED, DELIBERATE SCOPE TETO (mirrors `dom_dump.cpp`'s own `locate_head_tag_span()`
//     header comment's teto for the identical upstream mechanism -- kept coherent with that
//     already-shipped precedent in this same repo, not re-derived independently):
//       - Exactly two tag names enter raw mode, "style" and "script" -- the exact two, and ONLY
//         those two, upstream's own `Factory.cpp:255-257` registers. A third name needs its own
//         corpus-measured justification, the same "the corpus decides" discipline this header
//         already applies to the entity table, the void-element list, and the namespace grammar.
//       - The tag-NAME match that decides whether to enter raw mode is ASCII case-folded
//         (`<STYLE>`/`<Script>` also qualify) -- matching upstream's own
//         `StringUtilities::ToLower(tag_name)` before its `IsCDATATag` lookup
//         (`BaseXMLParser.cpp:246-248`), and matching this repo's OWN `parser.cpp`'s existing
//         `to_ascii_lower` fold for "head"/"body" tag-name comparisons.
//       - The CLOSING search is a PLAIN, LITERAL, case-folded substring search for
//         `"</style"`/`"</script"` -- NOT the tag-name-VERIFIED re-scan upstream's own `ReadCDATA`
//         performs (`BaseXMLParser.cpp:365-388`: a `<...>` sequence found inside CDATA content is
//         only accepted as the terminator if its OWN tag name, after stripping a leading `/`,
//         lowercases to the registered CDATA tag name; anything else -- including an unrelated
//         closing tag that happens to start with the same 7/8 bytes, e.g. a hypothetical
//         `</styleFoo>` -- is folded back into the accumulated data and scanning continues). This
//         is the EXACT SAME simplification `dom_dump.cpp`'s `locate_head_tag_span()` already
//         declared for the identical upstream mechanism -- converged on for consistency with an
//         already-shipped precedent in this same repo, not re-derived independently. Zero corpus
//         fixture (60 measured 2026-08-06, see lexer_corpus_sanity.cpp) exercises the divergence
//         this simplification would mis-handle.
//       - `<![CDATA[ ... ]]>` sections remain OUT of this grammar entirely (unchanged -- see the
//         DOCTYPE/XML-decl/CDATA bullet above), same "zero corpus occurrences, the frozen subset
//         does not name it either" reasoning `dom_dump.cpp`'s own teto already gives for the
//         identical exclusion.
//       - A self-closed `<style/>`/`<script/>` (`TagSelfClose`, not `TagOpenEnd`) never enters raw
//         mode -- there is no body to protect, mirroring upstream's own `ReadElement` only calling
//         `ReadCDATA` when `section_opened` is true (`BaseXMLParser.cpp:200-208` vs. `:243-262`).
//       - `<head>`-BLOB opacity (the paragraph above this one) is UNCHANGED -- this fix is
//         narrower and orthogonal, protecting `<style>`/`<script>` content WHEREVER either tag
//         appears, without this lexer ever becoming aware of the tag name `head`. `parser.cpp`'s
//         `handle_head()` continues to bypass this Lexer's normal token stream entirely for
//         `<head>`'s interior via its own `find_head_close()` raw scan, UNTOUCHED by this fix, and
//         the dump format this repo's differential oracle checks byte-for-byte is likewise
//         unchanged by it (`ctest -R differential` proven green both before and after, same SCOPE
//         line: 16 fixtures compared, 0 divergent).
// PT: RMLX-1/S1 -- tokenizador (lexer) para o subconjunto de RML congelado pelo
//     docs/rmlx-subset.md. Este é o PRIMEIRO tijolo do DOM próprio da glintfx: uma passada
//     mecânica de fluxo-de-bytes pra fluxo-de-tokens, SEM árvore, SEM semântica de atributo/id/
//     class, SEM tratamento especial de opacidade de `<head>`. Isso é trabalho da S2 (árvore de
//     nós), da S3 (parser recursivo-descendente que consome este fluxo de tokens e constrói a
//     árvore), e de ondas posteriores -- ver a seção "Deliberadamente NÃO é trabalho deste módulo"
//     abaixo pra fronteira exata e o porquê de cada item ser adiado de propósito, não só omitido.
//
//     ESCOPO, tirado do briefing da RMLX-1/S1 e do docs/rmlx-subset.md, guiado por corpus (as 44
//     fixtures `.rml` próprias da glintfx sob glintfx/tests/ + glintfx/demos/, nenhuma acima de
//     ~8 KB, maior linha única 230 bytes):
//       - início de tag de abertura (`<nome`), atributo (`nome="valor"` / `nome='valor'`, os dois
//         estilos de aspas -- citado explicitamente no briefing, "aspas simples e duplas"),
//         fechamento de tag de abertura (`>`), auto-fechamento (`/>`), tag de fechamento
//         (`</nome>`).
//       - trechos de texto entre tags (byte-verbatim, ver "Decodificação de entidade" abaixo pro
//         que isto significa pra `&nbsp;`/`&amp;`, as duas únicas entidades que o corpus usa).
//       - comentários (`<!-- ... -->`), tokenizados e entregues ao chamador, mas NÃO fazem parte
//         do DOM que este arco constrói (o próprio parser XML real do RmlUi não tem caminho
//         comentário-pra-Element nenhum -- ver a própria evidência de opacidade de `<head>` do
//         docs/rmlx-subset.md pro caso irmão de "esta construção de markup nunca vira nó vivo").
//         Espera-se que a S3 os descarte.
//     Cada um destes é medido por corpus: rodadas de `git grep`/`awk` contra toda fixture `.rml`
//     deste repo são citadas inline abaixo em cada decisão de desenho que este módulo tomou por
//     causa delas, conforme a própria cláusula "o corpus decide" deste briefing -- pra um leitor
//     duas ondas à frente conseguir rodar de novo o mesmo one-liner em vez de confiar numa
//     paráfrase.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE MÓDULO (território S2/S3/RMLX-2+, não só "depois" --
//     cada um é um motivo *específico* e nomeado de esta camada não poder chutar):
//       - Decodificação de entidade (`&nbsp;` -> U+00A0, `&amp;` -> `&`). O docs/rmlx-subset.md
//         ("uix-dom.md" daqui em diante) seção 6(c) diz que a decodificação acontece "no momento
//         de construção do nó de texto" -- isso é trabalho de nó/árvore, não tokenização de
//         fluxo-de-bytes. Os payloads `Text`/valor-de-`Attr` deste lexer são os bytes CRUS da
//         fonte, entidades não-decodificadas, exatamente como o payload `HEAD PRESENT` da seção 4
//         do uix-dom.md (que é explicitamente isento de decodificação pelo mesmo motivo "isto
//         ainda não é construção de nó"). Decodificar uma tabela FIXA (`amp`/`nbsp`, as duas
//         únicas que o corpus usa por `grep -oh '&[a-zA-Z#0-9]*;' glintfx/tests/*.rml
//         glintfx/demos/**/*.rml | sort -u` => exatamente `&amp;` e `&nbsp;`, nada mais, nenhuma
//         referência numérica de caractere) DENTRO do lexer seria a implementação menor e mais
//         barata, mas também comprometeria em silêncio a S1 com uma política de decodificação (o
//         que uma entidade NÃO-RECONHECIDA faz -- rejeita o fluxo de bytes? passa cru? decodifica
//         pela metade?) que o uix-dom.md atribui a um passo de construção POSTERIOR por um motivo
//         documentado. Deixado como ponto aberto explícito pra S2/S3 abaixo.
//       - O filtro de EXISTÊNCIA de texto só-whitespace (uix-dom.md seção 6(a): um nó de texto
//         feito só do conjunto de 4 caracteres de whitespace nunca é criado, ponto final, e nunca
//         ocupa slot de índice de filho). Este lexer emite TODO trecho de texto, inclusive os
//         só-whitespace -- aplicar o filtro aqui exigiria esta camada já saber sobre "slots de
//         índice de filho", que só existem quando há árvore (S2). A S3 aplica o uix-dom.md 6(a)
//         quando transforma este fluxo de tokens em nós.
//       - Opacidade de conteúdo de `<head>` (uix-dom.md seção 4: os filhos de `<head>` são bytes
//         CRUS, NÃO-PARSEADOS, NÃO-ENTITY-DECODIFICADOS desde logo após o `>` de `<head>` até
//         logo antes do `<` de `</head>`, nunca percorridos como tags). Este lexer NÃO trata
//         especialmente o nome de tag `head` -- ele tokeniza `<head>...</head>` exatamente como
//         qualquer outro elemento, recursando em `<style>`/`<link/>`/`<title>` como tags aninhadas
//         comuns. A decisão de SE consumir o fluxo de tokens normal deste lexer dentro de
//         `<head>`, ou em vez disso contornar e fatiar a fonte crua direto (que é o que o payload
//         `HEAD PRESENT` do uix-dom.md de fato precisa) é da S3 tomar -- o `handle_head()`/
//         `find_head_close()` do `parser.cpp` tomou a rota (b) (ver o próprio comentário de
//         cabeçalho do parser.hpp), INALTERADO pelo parágrafo abaixo. Não confundir isto
//         (opacidade de BLOB de `<head>`, ainda não é preocupação deste lexer) com a opacidade de
//         CONTEÚDO de `<style>`/`<script>`, que este lexer AGORA sim possui -- ver "RESOLVED
//         (UIX-LEXER-OPACO)" mais adiante neste arquivo pra essa decisão mais estreita, escopada
//         por nome-de-tag, ortogonal.
//       - DOCTYPE / declaração XML (`<?xml ... ?>`) / CDATA (`<![CDATA[ ... ]]>`). Zero
//         ocorrências no corpus (`grep -l '<!\[CDATA\|<!DOCTYPE\|<?xml' glintfx/tests/*.rml
//         glintfx/demos/**/*.rml` => nenhum casamento). Encontrar `<!` NÃO seguido de `--`, ou
//         `<?`, é um token `Error` (fail-high, ver "Hardening" abaixo) -- este é o mecanismo
//         concreto da própria cláusula de cabeçalho do docs/rmlx-subset.md ("pare e reporte --
//         nunca implemente por conta própria"): uma fixture real que precise de um destes para o
//         lexer S1 na hora com um token diagnosticável em vez de tokenizar errado em silêncio.
//       - Elementos vazios sem fechamento (o `<br>`/`<input>` sem `/` final do HTML5). Zero
//         ocorrências: todo elemento de forma-vazia no corpus é auto-fechado explicitamente
//         (`grep -on '<br[^/]' ...` / `grep -on '<link[^>]*[^/]>' ...` os dois vazios; `<br/>`,
//         `<input .../>`, `<link .../>` são as únicas formas presentes). Este lexer portanto NÃO
//         TEM lista de nome de elemento-vazio nenhuma (isso seria exatamente o scope-creep
//         "vamos fazer HTML5" que o próprio ADR-0020 que rege este arco nomeia como o risco que a
//         `RMLX-3` mais precisa se defender) -- só a sintaxe de auto-fechamento (`/>`) decide
//         auto-fechamento, uniformemente, pra todo nome de tag.
//       - Nomes com namespace (`xml:lang`, `xlink:href`). Zero ocorrências de `:` dentro de um
//         nome de tag ou atributo em lugar nenhum do corpus. Não faz parte da gramática de
//         identificador deste lexer (ver `is_name_start`/`is_name_char` no lexer.cpp) -- um `:`
//         dentro do que parece um nome simplesmente encerra o nome cedo e o byte seguinte é
//         escaneado como o que ele acabar sendo (quase certamente um `Error` de input
//         hostil/malformado).
//       - Casamento de nome entre tag de abertura e sua tag de fechamento (`<div>...</span>`).
//         Exige uma pilha, que exige uma árvore -- trabalho da S2/S3. Este lexer aceita um nome
//         de `TagClose` descasado como um token perfeitamente comum; ver o
//         `test_mismatched_close_name_not_validated` do lexer_tokens_sanity.cpp pra prova travada
//         desta fronteira exata.
//
//     MODELO DE TOKEN: um `Token` por fato lógico, atômico por chamada de `next()` -- notavelmente
//     `Attr` empacota TANTO o nome do atributo QUANTO seu valor (já sem aspas) num ÚNICO token em
//     vez de dividi-los entre duas chamadas de `next()` como uma máquina de estado minimamente
//     mínima poderia fazer. É uma escolha deliberada de ergonomia pra S3 (o consumidor): um nome
//     de atributo nunca é significativo sem seu valor nesta gramática (RML/RCSS não tem atributo
//     sem-valor/booleano em lugar nenhum do corpus -- `grep` por um `<tag attr>` cru sem `=`
//     dentro de qualquer fixture `.rml` achou zero), então dividir em dois tokens só somaria
//     estado inter-chamada ("acabei de ver um nome, agora espero `=valor`?") que TANTO este lexer
//     QUANTO todo consumidor teriam que rastrear redundantemente. Todo campo de `Token` referencia
//     o buffer-fonte ORIGINAL via `std::string_view` (zero-cópia) -- o chamador precisa manter
//     esse buffer vivo pela duração de vida de todo `Token` que ler.
//
//     UTF-8: este lexer opera byte-a-byte e nunca decodifica UTF-8 -- todo byte de continuação de
//     sequência multi-byte UTF-8 (0x80-0xBF) e byte-líder (0xC0-0xFF) é numericamente disjunto de
//     todo delimitador ASCII que esta gramática reconhece (`<`, `>`, `/`, `=`, `"`, `'`, `!`, `-`,
//     e o conjunto de 4 caracteres de whitespace), então escanear bytes crus é seguro sem
//     decodificador UTF-8 -- exatamente o mesmo raciocínio que a seção 2 do uix-dom.md dá pra
//     regra de escape dela própria ("toda sequência multi-byte UTF-8... passa sem mudança") não
//     precisar de consciência de Unicode nenhuma tampouco.
//
//     HARDENING (fail-high, mesma disciplina do `lados ∈ [3,1024]` do `decorator_polygon.cpp` /
//     `kLogMaxMessageBytes` do `log.hpp` -- ver a convenção "input validation em borda" deste
//     projeto): um parser é superfície voltada-a-atacante mesmo antes de existir árvore pra
//     atacar. Dois tetos derivados de forma independente e nomeados:
//       - `kMaxInputBytes` (1 MiB) -- o buffer-fonte INTEIRO. Derivado, não inventado: a maior
//         fixture real do corpus são 7957 bytes (`fonteng_colr_scene.rml`, medido via
//         `find glintfx/tests glintfx/demos -name '*.rml' -exec wc -c {} \; | sort -n | tail -1`)
//         -- 1 MiB é ~132x isso, e o próprio `battle_cockpit_rml.cpp` do GusWorld (a "tela
//         representativa", seção 2 do uix-dom.md/docs/rmlx-subset.md -- ~1,6x o markup da segunda
//         colocada, embutido em raw-string C++, não medível diretamente a partir deste repo) tem
//         expectativa de ficar bem dentro da mesma ordem de grandeza. Checado UMA VEZ na
//         construção (`Lexer::Lexer`); excedê-lo faz a PRIMEIRA chamada de `next()` retornar
//         `Error` imediatamente, offset 0.
//       - `kMaxTokenBytes` (64 KiB) -- o payload de um ÚNICO token qualquer (nome de tag/atributo,
//         valor de atributo, trecho de texto, corpo de comentário). ~285x a maior linha única
//         observada no corpus (230 bytes, `fonteng_sup_scene.rml`,
//         `awk '{print length}' ... | sort -rn | head -1`). Isto é belt-and-suspenders distinto
//         do `kMaxInputBytes`: limita o custo de um único token malformado/hostil (uma aspa não
//         fechada, um comentário não fechado) a um comprimento de scan fixo em vez de correr até
//         o fim do buffer (já com teto) antes de reportar falha. Um token que EXCEDERIA este teto
//         é rejeitado por inteiro (`Error`) -- NUNCA dividido entre vários tokens `Text`/outro pra
//         contornar (dividir forçaria todo consumidor a saber "dois tokens adjacentes do mesmo
//         tipo sem nada entre eles significa fundir", um sinal de fusão-entre-tokens que o
//         contrato deste lexer não oferece).
//     Os dois são FAIL-HIGH: rejeitam e emitem um token `Error` diagnosticável, nunca
//     truncam-e-continuam (um nome de tag ou valor de atributo truncado que silenciosamente "ainda
//     parseia" produziria uma árvore que parece plausível mas está ERRADA -- pior que uma falha
//     óbvia e ruidosa).
//
//     PEGAJOSIDADE DE ERRO: uma vez que `next()` retorna `Error` (qualquer que seja a causa), TODA
//     chamada subsequente retorna o MESMO token `Error` (`text`/`offset`/`length` idênticos) pra
//     sempre -- este lexer nunca tenta "ressincronizar" e continuar produzindo tokens depois de
//     input malformado. Um lexer que ressincroniza chutando onde a PRÓXIMA construção bem-formada
//     começa pode silenciosamente pular conteúdo real e entregar à S3 uma árvore sutilmente
//     errada em vez de um documento que visivelmente falhou ao parsear -- o mesmo raciocínio
//     "fail-high, não fail-plausível" que o parágrafo Hardening deste arquivo já aplica aos dois
//     tetos de byte, aplicado aqui à própria recuperação de erro. Mesmo contrato pegajoso pro
//     `EndOfFile`: toda chamada de `next()` além do fim do input retorna o mesmo token
//     `EndOfFile`.
//
//     CARACTERÍSTICA PERMISSIVA E DOCUMENTADA -- nenhum whitespace exigido entre dois atributos
//     adjacentes (`<div id="x"class="y">` tokeniza como dois tokens `Attr` comuns, não um
//     `Error`). A gramática XML estrita de verdade exige um whitespace separador antes de todo
//     atributo; este lexer não a impõe porque (a) o corpus nunca precisa da rejeição, e (b) a
//     leniência não introduz AMBIGUIDADE de parse nenhuma -- a aspa de fechamento do valor do
//     atributo anterior o encerra sem ambiguidade, então o próximo caractere de identificador só
//     pode começar um novo nome de atributo, nunca outra coisa. Ver o
//     `test_permissive_no_whitespace_between_attrs` do lexer_hardening_sanity.cpp pra prova
//     travada. Se uma fixture futura precisar da rejeição estrita, é uma adição pequena e local
//     (rastrear se pelo menos um byte de whitespace foi pulado desde o último token
//     significativo) -- deliberadamente não somada preventivamente, pela própria disciplina "o
//     corpus decide" deste arquivo.
//
//     PONTO ABERTO DE BLOB-DE-HEAD, RESOLVIDO PELA S3 (mantido por histórico): a opacidade de
//     `<head>` (uix-dom.md seção 4) precisa de acesso a bytes CRUS, NÃO-TOKENIZADOS, desde logo
//     após o `>` de `<head>` até logo antes do `<` de `</head>`. A superfície pública deste lexer é
//     um fluxo de tokens `next()` simples, sem escotilha "me dê o trecho cru em vez disso" nenhuma
//     -- a S3 tomou a rota (b) abaixo: o `handle_head()` do `parser.cpp` constrói um scan de
//     substring SEPARADO e descartável (`find_head_close()`) sobre o buffer-fonte original usando
//     os próprios campos `offset`/`length` do token `TagOpenStart{text="head"}` como ponto de
//     partida, contornando esta instância de `Lexer` por inteiro pra aquele trecho, e depois
//     RECONSTRÓI um `Lexer` novo sobre a substring depois do `>` de `</head>` (ver o próprio
//     comentário de cabeçalho do parser.hpp). Este header expõe `offset`/`length` em todo `Token`
//     especificamente pra essa rota continuar possível sem modificar este arquivo.
//
//     RESOLVED (UIX-LEXER-OPACO): `<style>`/`<script>` SÃO TIPO-CDATA NESTE TOKENIZADOR,
//     ESPELHANDO O UPSTREAM -- uma decisão DIFERENTE, mais estreita e ortogonal à de blob-de-`<head>`
//     logo acima (não confundir as duas: esta é escopada por nome-de-tag e dispara ONDE QUER que
//     `<style>`/`<script>` apareça, dentro de `<head>` ou não; a de cima é escopada por `<head>` e
//     não sabe nada sobre nomes de tag dentro dele). A pergunta que este item foi levantado pra
//     resolver -- "de quem é a responsabilidade: um lexer tolerante, ou um parser que assume um
//     scan opaco?" -- tem uma terceira resposta, mais simples, achada lendo o RmlUi upstream real
//     em vez de escolher entre as duas opções enquadradas: o PRÓPRIO tokenizador do upstream decide
//     isto, não um chamador contornando por cima dele. `Factory.cpp:255-257` registra exatamente
//     dois nomes de tag, "script" e "style", como tags CDATA persistentes
//     (`XMLParser::RegisterPersistentCDATATag`); o próprio tokenizador do `BaseXMLParser.cpp`
//     (`ReadElement`, :243-262) checa `IsCDATATag` logo após o `>` da tag de abertura e, se casar,
//     chama `ReadCDATA` (:349-390) em vez de retomar o scan de tag comum -- a MESMA camada que
//     reconhece delimitadores `<`/`>` em primeiro lugar troca o PRÓPRIO modo, num nome de tag
//     específico, sem escotilha ou mecanismo de contorno nenhum precisar existir na superfície
//     pública dela. Este lexer agora faz a coisa idêntica: o ramo `TagOpenEnd` do `scan_in_tag()`
//     checa se o nome de tag capturado no `TagOpenStart` que casa (`pending_tag_name_`) dobra por
//     ASCII pra "style" ou "script"; se sim, `state_` vira `RawCData` (não `Text`), e a PRÓXIMA
//     chamada de `next()` roda `scan_raw_cdata()`: um scan pra frente byte-a-byte (mesma forma do
//     próprio laço `while (!starts_with_at(...))` do `scan_comment()`, mesmo teto `kMaxTokenBytes`
//     aplicado ao trecho acumulado) que trata todo byte como conteúdo literal até achar o literal
//     dobrado-por-caixa `"</style"`/`"</script"`, e então devolve o controle AO tokenizador comum
//     naquele byte exato (`state_ = State::Text`) -- a própria tag de fechamento é então tokenizada
//     completamente normal pelo `scan_tag_close()` EXISTENTE, NÃO-MODIFICADO (whitespace e `>`
//     ainda validados, ainda fail-high num fechamento malformado). Este conserto não toca em nada
//     sobre COMO uma tag é reconhecida; só muda QUANDO bytes crus são entregues como um único token
//     `Text` em vez de serem re-entrados como gramática de tag.
//
//     Por que o LEXER e não um scan cru do lado do chamador (a forma que o próprio conserto gêmeo
//     deste repo, o `locate_head_tag_span()` do `glintfx/src/rml/dom_dump.cpp`, e o próprio
//     `handle_head()`/`find_head_close()` do `parser.cpp` deste repo, os dois usam): a opacidade de
//     `<style>`/`<script>` não é escopada a `<head>` nenhuma -- o registro de tag-CDATA do upstream
//     é INCONDICIONAL, onde quer que qualquer uma das duas tags apareça -- então um lexer que só
//     tolerasse isso dentro de `<head>` ainda engasgaria num `<style>` hipotético (hoje, zero
//     ocorrências no nível de topo) dentro de `<body>`. Pôr a troca de modo na camada que já possui
//     o reconhecimento de `<`/`>` não precisa de escotilha nova na superfície pública desta classe
//     (nenhum método `raw_until()`, contra o texto ANTIGO "Ponto aberto pra S2/S3" que este
//     parágrafo substitui) e compõe de graça com todo chamador EXISTENTE deste `Lexer` -- inclusive
//     a própria reconstrução de lexer pós-`</head>` do `parser.cpp`, que reentra em scan comum de
//     `State::Text` sobre uma substring que poderia ela mesma conter uma tag `<style>`/`<script>`
//     no nível de topo.
//
//     TETO DE ESCOPO DECLARADO, DELIBERADO (espelha o próprio teto do comentário de cabeçalho do
//     `locate_head_tag_span()` do `dom_dump.cpp` pro mecanismo upstream idêntico -- mantido
//     coerente com esse precedente já-entregue neste mesmo repo, não re-derivado
//     independentemente):
//       - Exatamente dois nomes de tag entram em modo cru, "style" e "script" -- os exatos dois, e
//         SÓ esses dois, que o próprio `Factory.cpp:255-257` do upstream registra. Um terceiro nome
//         precisa da própria justificativa medida-por-corpus, a mesma disciplina "o corpus decide"
//         que este cabeçalho já aplica à tabela de entidade, à lista de elemento-vazio, e à
//         gramática de namespace.
//       - O casamento de NOME-de-tag que decide se entra em modo cru é dobrado por ASCII
//         (`<STYLE>`/`<Script>` também qualificam) -- casando com o próprio
//         `StringUtilities::ToLower(tag_name)` do upstream antes do lookup `IsCDATATag`
//         (`BaseXMLParser.cpp:246-248`), e casando com a dobra `to_ascii_lower` já existente do
//         PRÓPRIO `parser.cpp` deste repo pras comparações de nome-de-tag "head"/"body".
//       - A busca de FECHAMENTO é uma busca de substring PLANA, LITERAL, dobrada-por-caixa por
//         `"</style"`/`"</script"` -- NÃO o re-scan com VERIFICAÇÃO-de-nome que o próprio
//         `ReadCDATA` do upstream faz (`BaseXMLParser.cpp:365-388`: uma sequência `<...>` achada
//         dentro do conteúdo CDATA só é aceita como terminador se o PRÓPRIO nome de tag dela,
//         depois de tirar uma `/` inicial, minusculizar pra o nome de tag CDATA registrado;
//         qualquer outra coisa -- inclusive uma tag de fechamento não-relacionada que por acaso
//         começa com os mesmos 7/8 bytes, ex. um `</styleFoo>` hipotético -- é dobrada de volta pro
//         dado acumulado e o scan continua). Esta é a MESMA simplificação exata que o
//         `locate_head_tag_span()` do `dom_dump.cpp` já declarou pro mecanismo upstream idêntico --
//         convergida por consistência com um precedente já-entregue neste mesmo repo, não
//         re-derivada independentemente. Zero fixture de corpus (60 medidas 2026-08-06, ver
//         lexer_corpus_sanity.cpp) exercita a divergência que esta simplificação trataria mal.
//       - Seções `<![CDATA[ ... ]]>` seguem FORA desta gramática por inteiro (inalterado -- ver o
//         item DOCTYPE/decl-XML/CDATA acima), mesmo raciocínio "zero ocorrências no corpus, o
//         subconjunto congelado também não nomeia" que o próprio teto do `dom_dump.cpp` já dá pra
//         exclusão idêntica.
//       - Uma `<style/>`/`<script/>` auto-fechada (`TagSelfClose`, não `TagOpenEnd`) nunca entra em
//         modo cru -- não há corpo pra proteger, espelhando o próprio `ReadElement` do upstream só
//         chamar `ReadCDATA` quando `section_opened` é verdadeiro (`BaseXMLParser.cpp:200-208` vs.
//         `:243-262`).
//       - A opacidade de BLOB de `<head>` (o parágrafo logo acima deste) é INALTERADA -- este
//         conserto é mais estreito e ortogonal, protegendo conteúdo de `<style>`/`<script>` ONDE
//         QUER que qualquer uma das duas tags apareça, sem este lexer nunca ficar ciente do nome de
//         tag `head`. O `handle_head()` do `parser.cpp` continua contornando o fluxo de tokens
//         normal deste Lexer por inteiro pro interior de `<head>` via o próprio scan cru
//         `find_head_close()`, INTOCADO por este conserto, e o formato de dump que o oráculo
//         diferencial deste repo confere byte-a-byte também é inalterado por ele (`ctest -R
//         differential` provado verde antes E depois, mesma linha SCOPE: 16 fixtures comparadas, 0
//         divergentes).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <string_view>

namespace glintfx::uix {

// EN: See this file's header comment, "Hardening" paragraph, for the derivation of both values.
// PT: Ver o parágrafo "Hardening" do comentário de cabeçalho deste arquivo pra derivação dos
//     dois valores.
inline constexpr std::size_t kMaxInputBytes = 1024 * 1024; // 1 MiB
inline constexpr std::size_t kMaxTokenBytes = 64 * 1024;   // 64 KiB

// EN: See this file's header comment, "Token model" paragraph. `EndOfFile`/`TagOpenEnd`/
//     `TagSelfClose` carry no payload (`text`/`value` are empty); every other kind uses `text`
//     for its single payload except `Attr`, which uses both `text` (the attribute NAME) and
//     `value` (the attribute VALUE, quotes already stripped).
// PT: Ver o parágrafo "Modelo de token" do comentário de cabeçalho deste arquivo. `EndOfFile`/
//     `TagOpenEnd`/`TagSelfClose` não carregam payload (`text`/`value` vazios); todo outro tipo
//     usa `text` pro único payload, exceto `Attr`, que usa `text` (o NOME do atributo) E `value`
//     (o VALOR do atributo, aspas já removidas).
enum class TokenKind {
  TagOpenStart, // '<' name          -- text = tag name
  Attr,         // name="value"      -- text = attr name, value = attr value (quotes stripped)
  TagOpenEnd,   // '>'               -- closes a non-self-closing opening tag
  TagSelfClose, // '/>'              -- closes a self-closing tag
  TagClose,     // '</' name '>'     -- text = tag name
  Text,         // raw text run      -- text = content, byte-verbatim, entities NOT decoded
  Comment,      // '<!--' ... '-->'  -- text = raw content between the delimiters
  EndOfFile,    // sticky terminal state, see header comment "Error stickiness"
  Error,        // sticky terminal state -- text = human-readable diagnostic
};

struct Token {
  TokenKind kind = TokenKind::EndOfFile;
  std::string_view text;  // see TokenKind's own comments above for what this holds per kind
  std::string_view value; // Attr only; empty for every other kind
  std::size_t offset = 0; // byte offset into the source buffer where this token begins
  std::size_t length = 0; // byte length of this token's full source span (including delimiters,
                          // e.g. the quotes of an Attr value, the '<!--'/'-->' of a Comment)
};

// EN: Stateful, single-pass tokenizer over a caller-owned `std::string_view`. `source` must
//     outlive every `Token` this instance returns (all `Token` fields are views into it, zero-
//     copy -- see this file's header comment). Not copyable/movable-safe across concurrent use;
//     this project's library-wide single-threaded-no-internal-lock convention applies (see
//     glintfx/include/glintfx/log.hpp's own THREADING note for the precedent this follows) --
//     one `Lexer` instance is driven by exactly one thread.
// PT: Tokenizador com estado, passada única, sobre um `std::string_view` de posse do chamador.
//     `source` precisa sobreviver a todo `Token` que esta instância retornar (todo campo de
//     `Token` é uma view sobre ele, zero-cópia -- ver o comentário de cabeçalho deste arquivo).
//     Não é seguro copiar/mover sob uso concorrente; a convenção da biblioteca inteira de
//     single-threaded-sem-lock-interno se aplica (ver a própria nota THREADING do
//     glintfx/include/glintfx/log.hpp pro precedente que isto segue) -- uma instância de `Lexer`
//     é dirigida por exatamente uma thread.
class Lexer {
public:
  // EN: If `source.size() > kMaxInputBytes`, this constructor does NOT throw or abort -- it
  //     records the failure internally so the FIRST `next()` call returns `Error` (offset 0,
  //     `text` naming the ceiling that was exceeded), consistent with this file's fail-high
  //     discipline (a byte-ceiling violation is diagnosable input, not a programming-error-grade
  //     precondition failure that would warrant an exception/abort).
  // PT: Se `source.size() > kMaxInputBytes`, este construtor NÃO lança nem aborta -- registra a
  //     falha internamente pra que a PRIMEIRA chamada de `next()` retorne `Error` (offset 0,
  //     `text` nomeando o teto excedido), consistente com a disciplina fail-high deste arquivo
  //     (uma violação de teto de byte é input diagnosticável, não uma falha de pré-condição
  //     grau-erro-de-programação que justificaria exceção/abort).
  explicit Lexer(std::string_view source);

  // EN: Returns the next token in the stream. See this file's header comment, "Error stickiness"
  //     paragraph, for the sticky-`Error`/sticky-`EndOfFile` contract.
  // PT: Retorna o próximo token do fluxo. Ver o parágrafo "Pegajosidade de erro" do comentário de
  //     cabeçalho deste arquivo pro contrato de `Error`/`EndOfFile` pegajosos.
  Token next();

private:
  // EN: `RawCData` -- see this file's header comment, "RESOLVED (UIX-LEXER-OPACO)" paragraph.
  //     Entered by `scan_in_tag()`'s `TagOpenEnd` branch when the just-closed opening tag's name
  //     (case-folded) is "style"/"script"; left by `scan_raw_cdata()` the moment it locates the
  //     literal closer, handing control back to ordinary `State::Text` scanning.
  // PT: `RawCData` -- ver o parágrafo "RESOLVED (UIX-LEXER-OPACO)" do comentário de cabeçalho
  //     deste arquivo. Entrado pelo ramo `TagOpenEnd` do `scan_in_tag()` quando o nome da tag de
  //     abertura recém-fechada (dobrado por caixa) é "style"/"script"; deixado pelo
  //     `scan_raw_cdata()` no momento em que localiza o fechamento literal, devolvendo o controle
  //     ao scan comum de `State::Text`.
  enum class State { Text,
                     InTag,
                     RawCData,
                     Done };

  // EN: Which literal closer `scan_raw_cdata()` is hunting for -- set by `scan_in_tag()`'s
  //     `TagOpenEnd` branch alongside `state_ = State::RawCData`, consumed (and reset to `None`)
  //     by `scan_raw_cdata()` itself.
  // PT: Qual fechamento literal o `scan_raw_cdata()` está caçando -- setado pelo ramo `TagOpenEnd`
  //     do `scan_in_tag()` junto de `state_ = State::RawCData`, consumido (e resetado pra `None`)
  //     pelo próprio `scan_raw_cdata()`.
  enum class CDataKind { None,
                         Style,
                         Script };

  Token make_error(std::size_t offset, std::string_view message);
  Token scan_text();
  Token scan_comment();
  Token scan_tag_open_start();
  Token scan_tag_close();
  Token scan_in_tag();    // consumes exactly one of: Attr, TagOpenEnd, TagSelfClose, Error
  Token scan_raw_cdata(); // consumes the CDATA-like body of a <style>/<script> tag, see State::RawCData

  std::string_view source_;
  std::size_t pos_ = 0;
  State state_ = State::Text;
  Token sticky_{};                       // valid only once state_ == Done (Error or natural EOF reached)
  std::string_view pending_tag_name_;    // the most recent TagOpenStart's name, see scan_tag_open_start()
  CDataKind raw_kind_ = CDataKind::None; // valid only while state_ == State::RawCData
};

} // namespace glintfx::uix
