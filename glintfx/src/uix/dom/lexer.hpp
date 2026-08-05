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
//         S3's to make -- this is the single biggest open point this slice hands to S3/S6b, see
//         "Open point for S2/S3" at the bottom of this file.
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
//     OPEN POINT FOR S2/S3 (flagged here so both waves hit it deliberately, not by surprise):
//     `<head>` opacity (uix-dom.md section 4) needs RAW, UN-TOKENIZED byte access from just after
//     `<head>`'s `>` to just before `</head>`'s `<`. This lexer's public surface is a plain
//     `next()` token stream with no such "give me the raw span instead" escape hatch -- S3, on
//     seeing a `TagOpenStart{text="head"}` token, will need EITHER (a) a new `Lexer` method this
//     slice does not add (e.g. `raw_until(std::string_view closing_tag)`), OR (b) to construct a
//     SEPARATE, throwaway substring scan over the original source buffer using this token's own
//     `offset`/`length` fields as a starting point, bypassing this `Lexer` instance entirely for
//     that one span. This header exposes `offset`/`length` on every `Token` specifically so route
//     (b) is possible without modifying this file -- but which route S3 actually takes is S3's
//     decision to make, not pre-empted here.
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
//         `HEAD PRESENT` do uix-dom.md de fato precisa) é da S3 tomar -- este é o maior ponto
//         aberto único que esta fatia entrega pra S3/S6b, ver "Ponto aberto pra S2/S3" no final
//         deste arquivo.
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
//     PONTO ABERTO PRA S2/S3 (sinalizado aqui pra as duas ondas baterem nele de propósito, não por
//     surpresa): a opacidade de `<head>` (uix-dom.md seção 4) precisa de acesso a bytes CRUS,
//     NÃO-TOKENIZADOS, desde logo após o `>` de `<head>` até logo antes do `<` de `</head>`. A
//     superfície pública deste lexer é um fluxo de tokens `next()` simples, sem escotilha "me dê o
//     trecho cru em vez disso" nenhuma -- a S3, ao ver um token `TagOpenStart{text="head"}`, vai
//     precisar OU (a) de um método novo do `Lexer` que esta fatia não soma (ex.:
//     `raw_until(std::string_view closing_tag)`), OU (b) de construir um scan de substring
//     SEPARADO e descartável sobre o buffer-fonte original usando os próprios campos
//     `offset`/`length` deste token como ponto de partida, contornando esta instância de `Lexer`
//     por inteiro pra aquele trecho. Este header expõe `offset`/`length` em todo `Token`
//     especificamente pra a rota (b) ser possível sem modificar este arquivo -- mas qual rota a
//     S3 de fato toma é decisão da S3, não pré-decidida aqui.
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
  enum class State { Text,
                     InTag,
                     Done };

  Token make_error(std::size_t offset, std::string_view message);
  Token scan_text();
  Token scan_comment();
  Token scan_tag_open_start();
  Token scan_tag_close();
  Token scan_in_tag(); // consumes exactly one of: Attr, TagOpenEnd, TagSelfClose, Error

  std::string_view source_;
  std::size_t pos_ = 0;
  State state_ = State::Text;
  Token sticky_{}; // valid only once state_ == Done (Error or natural EOF reached)
};

} // namespace glintfx::uix
