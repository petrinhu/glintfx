// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S2 -- glintfx's own DOM tree model: `Node`/`Element`/`Text`/`Document`. This is the
//     SECOND brick (after S1's lexer.hpp/.cpp): a plain, general-purpose ownership tree with the
//     minimum semantics `S3` (recursive-descent parser, consumes S1's token stream and builds
//     this tree) and `S4` (public DOM read/write API) need. NO parsing, NO RmlUi/GLFW/GL, NO
//     RCSS/selector/cascade, NO layout/geometry, NO events/focus/hover, NO data-binding
//     evaluation -- see this header's "Deliberately NOT this module's job" section below for the
//     exact boundary, mirroring lexer.hpp's own section of the same name.
//
//     TWO OPEN POINTS S1 EXPLICITLY LEFT FOR THIS SLICE TO RESOLVE (docs/rmlx-subset.md's sibling
//     doc, docs/uix-dom.md -- "uix-dom.md" hereafter -- section 6a and TODO.md's RMLX-1 line):
//
//     (1) THE WHITESPACE-ONLY-TEXT EXISTENCE FILTER (uix-dom.md 6a) IS RESOLVED **HERE**, IN
//         `Element::append_child`, NOT IN S3. Argument: this is fundamentally a question of "which
//         nodes occupy a child-index slot", and child indexing is this tree's OWN concern (it is
//         what `children()`/`append_child` exist to define) -- not a parsing decision. Baking the
//         filter into `append_child` makes it a TREE INVARIANT: "an `Element`'s children never
//         include a whitespace-only `Text` node" holds for EVERY caller (`S3`'s parser today; a
//         future `S4` read/write API that programmatically inserts/replaces text; a hand-built
//         test tree) with zero chance of one of them forgetting to re-check, which is exactly the
//         "private assumption invented independently by two callers" failure mode uix-dom.md's own
//         "Why this document exists" section warns about for the `S6a`/`S6b` split -- the same
//         reasoning applies one layer down, between `S3` and a future `S4`. Concretely:
//         `append_child` uses the IDENTICAL 4-character whitespace set and the IDENTICAL
//         `std::all_of`-over-empty-range-is-vacuously-true semantics `Factory::InstanceElementText`
//         uses (`examples/RmlUi/Source/Core/Factory.cpp:330-341`, cited already in lexer.hpp) --
//         which also means an EMPTY `Text("")` is filtered too, matching upstream exactly (empty
//         range -> `all_of` is trivially true). `S3` therefore does not need to pre-check anything
//         before calling `append_child` with every `Text` node it constructs, whitespace-only or
//         not -- it just tries the append and reads back `AppendOutcome`.
//     (2) OPENING/CLOSING TAG NAME MATCHING (`<div>...</span>`) NEEDS NO NEW CODE HERE AND STAYS
//         `S3`'s JOB, AS S1 SAID -- but this header states explicitly WHY the model already
//         supports the construction S3 will do: `Element::append_child` accepts any
//         `std::unique_ptr<Node>` unconditionally (module modulo the two outcomes above); it has
//         NO notion of "the closing tag name that will eventually arrive" and never needed one.
//         `S3`'s own parse stack (push an `Element` on `TagOpenStart`+attrs+`TagOpenEnd`, pop on
//         `TagClose` -- matched name or not, per lexer.hpp's own
//         `test_mismatched_close_name_not_validated` boundary) drives `append_child` calls exactly
//         like any other tree-building code would; there is nothing in this file to change or add
//         for that to work.
//
//     DELIBERATELY NOT THIS MODULE'S JOB (S3/S4/RMLX-2+ territory):
//       - Building a tree FROM RML source text at all (S3's recursive-descent parser). This module
//         has no notion of "source bytes", `offset`/`length`, or the lexer's `Token` type.
//       - The public, ergonomic DOM read/write API glintfx's facade (`App`/`UiLayer`) will
//         eventually expose (S4) beyond the two primitives added below. S3 (an append-only,
//         top-down, stack-driven builder) never needs anything but `append_child`; this slice's
//         own brief lists "operações de classe (add/remove/has)" as in-scope but said nothing
//         about generic child removal, so `remove_child`/`clear_children` were deliberately left
//         OUT of the original S2 slice -- until S4 (`dom_api.cpp`'s `set_text`, `f15f1f8`) hit
//         exactly the wall this bullet predicted: `Rml::Element::SetInnerRML` clears every
//         existing child unconditionally before writing, and there was no way to replicate that.
//         UIX-REMOVE-CHILD (RMLX-1, 2026-08-05) closes that gap HERE, in `Element` -- child-list
//         mutation is `Element`'s own concern, exactly like `append_child` already is, not
//         `dom_api.cpp`'s (see `remove_child`/`clear_children`'s own doc-comments below for the
//         two primitives added and why their return types differ). What is STILL genuinely out of
//         scope, because nothing has needed it yet: insert-AT-POSITION (as opposed to
//         append-at-end or remove-by-identity), and a single combinator that both replaces content
//         AND validates/normalizes markup in one call (that is `RMLX-2`+ territory, once
//         RCSS/attributes interact with child replacement in ways this plain tree does not model).
//       - Any validation of tag/attribute-NAME grammar (`[A-Za-z_][A-Za-z0-9_-]*`, no `:`, etc.).
//         That grammar is S1's border (`is_name_start`/`is_name_char` in lexer.cpp) -- by the time
//         a `tag`/attribute `name` string reaches this module via `S3`, it has already passed
//         through a successfully-tokenized `TagOpenStart`/`Attr` token, so re-validating it here
//         would be redundant defense of the same border twice. The ONE exception, deliberately
//         narrower, is `Element::add_class`'s single-token check below -- that is not a grammar
//         check, it is a TREE-STRUCTURE invariant (a class SET's members must each be a single,
//         already-split token; see uix-dom.md section 7).
//       - RCSS/selectors/cascade/computed style (`RMLX-2`), layout/geometry (`RMLX-3`),
//         events/focus/hover (`RMLX-5`), data-binding evaluation (`RMLX-6` -- `data-*` attributes
//         are stored here as perfectly ordinary, INERT generic attributes, zero special-casing,
//         zero evaluation).
//       - `<head>`'s raw-byte extraction FROM SOURCE (S3's job, per lexer.hpp's own "Open point
//         for S2/S3"). This module only provides WHERE that already-extracted raw string is
//         stored once S3 has it (`Document::set_head`/`HeadContent`) -- see point (3) below for why
//         that storage belongs here and not deferred further.
//
//     (3) WHY `Document` EXISTS IN THIS SLICE EVEN THOUGH THE BRIEF NAMES ONLY "Node / Element /
//         Text": uix-dom.md's own contract (sections 3-4) is structured around exactly two
//         top-level facts a parsed document carries -- the `HEAD` record (opaque, present/absent)
//         and the `body` subtree -- and `S6b` (the dumper that will eventually walk THIS module's
//         tree) needs a single object holding both to produce that dump. Without an ownership root
//         for "the whole parsed thing", `S3` would have no natural return type and `S4` would have
//         no natural top-level object for its API. This is read as squarely inside the brief's own
//         "o mínimo que a S3/S4 precisam" clause, not scope creep: `Document` adds nothing beyond
//         (a) sole ownership of the `body` root `Element` (always present, tag `"body"`, matching
//         uix-dom.md section 5's "the `body` root itself gets the full element block too, no
//         special-cased root"), and (b) `HeadContent`, a two-field struct mirroring uix-dom.md
//         section 4's `HEAD ABSENT` / `HEAD PRESENT <raw>` shape 1:1. No querySelector, no
//         observers, no document-wide id index (see point (4)) -- nothing a browser DOM has that
//         this corpus does not need.
//     (4) NO DOCUMENT-WIDE ID INDEX (`std::unordered_map<id, Element*>`): `Element::find_by_id`
//         is a plain pre-order DFS, `O(n)` per call. This is a deliberate non-optimization, not an
//         oversight -- this slice's own brief says explicitly "otimização é proibida nesta onda; a
//         RMLX-3 é quem dirá se o modelo precisa mudar, e otimizar antes de medir é apostar", and
//         the corpus's largest fixture is under 8 KB (dozens of elements, not thousands) --
//         `RMLX-3`'s own trigger, per that same brief, is a MEASURED reason to change this, not a
//         guess made now. `find_by_id` is iterative (an explicit stack, not recursion) even though
//         it would be trivially safe recursively at `kMaxElementDepth`'s ceiling -- kept iterative
//         anyway as the same "árvore é onde recursão vira stack overflow" discipline the depth
//         ceiling below already applies to construction, applied here to traversal too, for a
//         second, independent line of defense.
//
//     LOOKUP POLICY, STATED EXPLICITLY BECAUSE THE CORPUS ALREADY EXERCISES THE AMBIGUOUS CASE:
//     `find_by_id` returns the FIRST match in pre-order (self, then children in source order,
//     depth-first), and duplicate ids are NOT rejected or deduplicated anywhere in this model --
//     this is not a hypothetical edge case: the real corpus already has duplicate `id` values
//     today (`grep -oh 'id="[^"]*"' glintfx/tests/*.rml glintfx/demos/**/*.rml | sort | uniq -c |
//     sort -rn`, e.g. `id="ctrl_ascii"` ×4, `id="scroller"` ×3, `id="line1"` ×2, several more) --
//     so "first pre-order match, no uniqueness enforced" is the policy this corpus already lives
//     with, not a guess about a case nobody hits. `find_by_id("")` always returns `nullptr`
//     immediately, never matching an id-less element: an empty id and an absent id are the SAME
//     state (uix-dom.md section 7's own empty-value asymmetry, "an empty id and no id both fail
//     every `#foo` lookup upstream, there is no behavioural difference to preserve") -- looking up
//     "the element with no id" is not a meaningful query and this model never answers it as if it
//     were one.
//
//     ID/CLASS/ATTRIBUTE STORAGE ALREADY SATISFIES uix-dom.md SECTION 7'S ORDERING "FOR FREE":
//     `Element::classes()` is a `std::set<std::string, std::less<>>` (sorted ascending, byte-wise,
//     deduplicated BY CONSTRUCTION -- exactly section 7's "split, drop empty, deduplicate, sort
//     ascending byte-wise" policy, with zero extra sort/dedupe code needed anywhere, including in
//     the future `S6b` dumper, which only needs to iterate); `Element::attributes()` is a
//     `std::map<std::string, std::string, std::less<>>`, same byte-wise ordering guarantee for the
//     same reason (`std::less<>` on `std::string`/`std::string_view` is `operator<`, which for
//     `std::string` is defined via `char_traits<char>::compare`, itself specified to compare as if
//     `unsigned char` -- i.e. exactly the byte-wise, non-locale-aware order section 7 requires,
//     same "no `std::locale`" reasoning that document already gives for why this is the right
//     choice, not an accident of what `std::sort` happens to do). `id`/`class` are RESERVED names
//     in the generic attribute map -- `set_attribute("id", ...)`/`set_attribute("class", ...)`
//     both return `false` and store nothing -- so there is exactly ONE place in this model that can
//     ever hold a given element's id, and exactly one for its class set; a future `S6b` cannot
//     accidentally read a stale/duplicate copy from the wrong place, and `has_attribute("id")` is
//     always `false` by construction (never special-cased explicitly, it falls out of the
//     reservation above for free) -- matching section 7's "a conforming dumper never emits `ATTR
//     id=...` nor `ATTR class=...`" from the storage layer up, not just at dump time.
//
//     HARDENING -- NESTING-DEPTH CEILING (fail-high, same discipline as lexer.hpp's
//     `kMaxInputBytes`/`kMaxTokenBytes`, this project's "input validation em borda" convention
//     applied one layer further in, since a tree is exactly where unbounded recursion becomes a
//     stack-overflow crash): `kMaxElementDepth` (256) bounds how deep `Element::append_child` will
//     let the tree nest (root `body` is depth 1; a child of `body` is depth 2; ...). Derived, not
//     invented: the deepest real nesting measured across all 40 corpus fixtures today is **5**
//     (a stack-based tag-nesting scan over `glintfx/tests/*.rml` + `glintfx/demos/**/*.rml` --
//     `glintfx/tests/fonteng_perf_scene.rml` the deepest single fixture -- this counts the `<rml>`/
//     `<head>` wrapper tags too, so the real `body`-subtree-only depth this module will ever see
//     from `S3` is smaller still). GusWorld's own representative screen
//     (`battle_cockpit_rml.cpp`, docs/rmlx-subset.md section 2) is not directly measurable from
//     this repo, so -- mirroring lexer.hpp's own margin reasoning for its two byte ceilings rather
//     than guessing a precise number for an unmeasured consumer -- `kMaxElementDepth` is set with a
//     generous ~50x margin over the measured corpus maximum (256 vs. 5), not a tight fit:
//     comfortably enough for GusWorld's deepest composed screens without ever being reachable by
//     realistic markup, while still being a HARD, FINITE ceiling a hostile/malformed input cannot
//     exceed. 256 levels of recursive `unique_ptr<Node>` destructor chaining (this class's own
//     destructor relies on the DEFAULT, recursive one -- see the ceiling's role in keeping that
//     default safe) is trivially within any reasonable thread's stack budget (a few hundred bytes
//     per frame at worst, tens of KB total, against a default Linux thread stack in the MiB range)
//     -- this is FAIL-HIGH like lexer.hpp's ceilings: `append_child` REJECTS (returns
//     `AppendOutcome::RejectedDepthCeiling`, does not adopt the child, does not
//     truncate/reparent/silently drop a level) rather than ever building a tree deep enough to
//     make its own destructor unsafe.
//
//     NO LOGGING IN THIS MODULE (deliberate, not an oversight): `append_child`'s two non-`Appended`
//     outcomes are signalled purely as DATA (`AppendOutcome`), never printed/logged here. This
//     module stays link-dependency-free (no RmlUi/GLFW/GL, per this arc's own governing brief and
//     `tools/check_rml_whitelist.sh`'s gate) -- pulling in `glintfx::log` (`glintfx/include/
//     glintfx/log.hpp`) would tie this standalone CMake target to the rest of the Camada 1 build
//     graph for a single `fprintf`, which is exactly the build-cost property `glintfx/src/uix/dom/
//     CMakeLists.txt`'s own header comment says this module exists to avoid. The caller (`S3`
//     today; whichever wave links `glintfx::log` in the future) decides whether/how to surface a
//     `RejectedDepthCeiling`/`FilteredWhitespaceText` outcome -- the same "this layer emits
//     diagnosable data, the NEXT layer decides what to do with it" contract lexer.hpp's own sticky
//     `Error` token already establishes one layer down.
// PT: RMLX-1/S2 -- modelo de árvore DOM próprio da glintfx: `Node`/`Element`/`Text`/`Document`.
//     Este é o SEGUNDO tijolo (depois do lexer.hpp/.cpp da S1): uma árvore de posse plana e de
//     propósito geral, com a semântica mínima que a `S3` (parser recursivo-descendente, consome o
//     fluxo de tokens da S1 e constrói esta árvore) e a `S4` (API pública de leitura/escrita do
//     DOM) precisam. SEM parsing, SEM RmlUi/GLFW/GL, SEM RCSS/seletor/cascata, SEM layout/
//     geometria, SEM eventos/foco/hover, SEM avaliação de data-binding -- ver a seção
//     "Deliberadamente NÃO é trabalho deste módulo" abaixo pra fronteira exata, espelhando a seção
//     de mesmo nome do próprio lexer.hpp.
//
//     DOIS PONTOS ABERTOS QUE A S1 DEIXOU EXPLICITAMENTE PRA ESTA FATIA RESOLVER (documento irmão
//     do docs/rmlx-subset.md, docs/uix-dom.md -- "uix-dom.md" daqui em diante -- seção 6a e a linha
//     RMLX-1 do TODO.md):
//
//     (1) O FILTRO DE EXISTÊNCIA DE TEXTO SÓ-WHITESPACE (uix-dom.md 6a) É RESOLVIDO **AQUI**, no
//         `Element::append_child`, NÃO na S3. Argumento: isto é fundamentalmente uma pergunta de
//         "quais nós ocupam um slot de índice de filho", e indexação de filho é preocupação DESTA
//         árvore mesma (é o que `children()`/`append_child` existem pra definir) -- não uma decisão
//         de parsing. Embutir o filtro no `append_child` torna isto um INVARIANTE DE ÁRVORE: "os
//         filhos de um `Element` nunca incluem um nó `Text` só-whitespace" vale pra TODO chamador
//         (o parser da `S3` hoje; uma futura API de leitura/escrita da `S4` que insere/substitui
//         texto programaticamente; uma árvore de teste feita à mão) sem chance nenhuma de um deles
//         esquecer de re-checar, que é exatamente o modo de falha "suposição privada inventada de
//         forma independente por dois chamadores" que a própria seção "Por que este documento
//         existe" do uix-dom.md avisa pra divisão `S6a`/`S6b` -- o mesmo raciocínio vale uma camada
//         abaixo, entre `S3` e uma futura `S4`. Concretamente: `append_child` usa o MESMO conjunto
//         de 4 caracteres de whitespace e a MESMA semântica de `std::all_of`-sobre-range-vazio-é-
//         vacuamente-verdadeiro que `Factory::InstanceElementText` usa
//         (`examples/RmlUi/Source/Core/Factory.cpp:330-341`, já citado no lexer.hpp) -- o que
//         também significa que um `Text("")` VAZIO também é filtrado, batendo com o upstream
//         exatamente (range vazio -> `all_of` é trivialmente verdadeiro). A `S3` portanto não
//         precisa pré-checar nada antes de chamar `append_child` com todo `Text` que constrói,
//         só-whitespace ou não -- só tenta o append e lê o `AppendOutcome` de volta.
//     (2) O CASAMENTO DE NOME ENTRE ABERTURA E FECHAMENTO (`<div>...</span>`) NÃO PRECISA DE CÓDIGO
//         NOVO AQUI E CONTINUA TRABALHO DA `S3`, COMO A S1 DISSE -- mas este cabeçalho declara
//         explicitamente POR QUÊ o modelo já suporta a construção que a S3 vai fazer:
//         `Element::append_child` aceita qualquer `std::unique_ptr<Node>` incondicionalmente
//         (módulo os dois resultados acima); ele NÃO TEM noção nenhuma de "o nome da tag de
//         fechamento que eventualmente vai chegar" e nunca precisou de uma. A própria pilha de
//         parse da `S3` (empilha um `Element` no `TagOpenStart`+atributos+`TagOpenEnd`, desempilha
//         no `TagClose` -- nome casando ou não, pela própria fronteira
//         `test_mismatched_close_name_not_validated` do lexer.hpp) dirige chamadas de
//         `append_child` exatamente como qualquer outro código de construção de árvore faria; não
//         há nada neste arquivo pra mudar ou somar pra isso funcionar.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE MÓDULO (território S3/S4/RMLX-2+):
//       - Construir árvore nenhuma A PARTIR de texto-fonte RML (parser recursivo-descendente da
//         S3). Este módulo não tem noção nenhuma de "bytes-fonte", `offset`/`length`, nem do tipo
//         `Token` do lexer.
//       - A API pública de leitura/escrita de DOM que a fachada da glintfx (`App`/`UiLayer`)
//         eventualmente vai expor (S4) além dos dois primitivos somados abaixo. A S3 (um
//         construtor append-only, de cima pra baixo, dirigido por pilha) nunca precisa de nada
//         além de `append_child`; o próprio briefing desta fatia lista "operações de classe
//         (add/remove/has)" como no escopo mas não dizia nada sobre remoção genérica de filho,
//         então `remove_child`/`clear_children` ficaram deliberadamente DE FORA da fatia S2
//         original -- até a S4 (`set_text` do `dom_api.cpp`, `f15f1f8`) bater exatamente na parede
//         que este item já previa: o próprio `Rml::Element::SetInnerRML` limpa todo filho
//         existente incondicionalmente antes de escrever, e não havia jeito de replicar isso. A
//         UIX-REMOVE-CHILD (RMLX-1, 2026-08-05) fecha essa lacuna AQUI, no `Element` -- mutação de
//         lista de filho é preocupação do próprio `Element`, exatamente como `append_child` já é,
//         não do `dom_api.cpp` (ver os próprios doc-comments de `remove_child`/`clear_children`
//         abaixo pros dois primitivos somados e por que os tipos de retorno deles diferem). O que
//         AINDA está genuinamente fora do escopo, porque nada precisou disso ainda: inserção EM
//         POSIÇÃO (diferente de somar-no-fim ou remover-por-identidade), e um único combinador que
//         tanto substitui conteúdo QUANTO valida/normaliza markup numa única chamada (isso é
//         território da `RMLX-2`+, quando RCSS/atributo interagirem com substituição de filho de
//         um jeito que esta árvore simples não modela).
//       - Validação nenhuma da gramática de NOME de tag/atributo (`[A-Za-z_][A-Za-z0-9_-]*`, sem
//         `:`, etc.). Essa gramática é fronteira da S1 (`is_name_start`/`is_name_char` no
//         lexer.cpp) -- no momento em que uma string `tag`/nome de atributo chega neste módulo via
//         `S3`, ela já passou por um token `TagOpenStart`/`Attr` tokenizado com sucesso, então
//         revalidar aqui seria defender a mesma fronteira duas vezes de forma redundante. A ÚNICA
//         exceção, deliberadamente mais estreita, é a checagem de token-único do
//         `Element::add_class` abaixo -- essa não é uma checagem de gramática, é um INVARIANTE DE
//         ESTRUTURA-DE-ÁRVORE (os membros de um conjunto de classe precisam ser cada um um token
//         único, já separado; ver seção 7 do uix-dom.md).
//       - RCSS/seletores/cascata/estilo computado (`RMLX-2`), layout/geometria (`RMLX-3`),
//         eventos/foco/hover (`RMLX-5`), avaliação de data-binding (`RMLX-6` -- atributos `data-*`
//         são guardados aqui como atributos genéricos perfeitamente comuns, INERTES, zero
//         tratamento especial, zero avaliação).
//       - Extração de bytes crus do `<head>` A PARTIR DA FONTE (trabalho da S3, pelo próprio "Ponto
//         aberto pra S2/S3" do lexer.hpp). Este módulo só provê ONDE aquela string já-extraída fica
//         guardada uma vez que a S3 a tem (`Document::set_head`/`HeadContent`) -- ver o ponto (3)
//         abaixo pra por que esse armazenamento pertence aqui e não fica adiado mais ainda.
//
//     (3) POR QUE `Document` EXISTE NESTA FATIA MESMO O BRIEFING NOMEANDO SÓ "Node / Element /
//         Text": o próprio contrato do uix-dom.md (seções 3-4) é estruturado em torno de
//         exatamente dois fatos de topo que um documento parseado carrega -- o registro `HEAD`
//         (opaco, presente/ausente) e a subárvore `body` -- e a `S6b` (o dumper que eventualmente
//         vai percorrer a árvore DESTE módulo) precisa de um objeto único guardando os dois pra
//         produzir aquele dump. Sem uma raiz de posse pra "a coisa inteira parseada", a `S3` não
//         teria tipo de retorno natural nenhum e a `S4` não teria objeto de topo natural nenhum pra
//         sua API. Isto é lido como estando dentro da própria cláusula do briefing "o mínimo que a
//         S3/S4 precisam", não scope creep: `Document` soma nada além de (a) posse única da raiz
//         `Element` `body` (sempre presente, tag `"body"`, batendo com a própria seção 5 do
//         uix-dom.md, "a própria raiz body recebe o bloco de elemento completo também, sem atalho
//         especial de raiz"), e (b) `HeadContent`, uma struct de dois campos que espelha 1:1 a
//         forma `HEAD ABSENT` / `HEAD PRESENT <cru>` da própria seção 4 do uix-dom.md. Sem
//         querySelector, sem observers, sem índice de id documento-inteiro (ver ponto (4)) -- nada
//         que um DOM de navegador tem e que este corpus não precisa.
//     (4) SEM ÍNDICE DE ID DOCUMENTO-INTEIRO (`std::unordered_map<id, Element*>`):
//         `Element::find_by_id` é uma DFS pré-ordem simples, `O(n)` por chamada. Esta é uma
//         não-otimização deliberada, não um descuido -- o próprio briefing desta fatia diz
//         explicitamente "otimização é proibida nesta onda; a RMLX-3 é quem dirá se o modelo
//         precisa mudar, e otimizar antes de medir é apostar", e a maior fixture do corpus tem
//         menos de 8 KB (dezenas de elementos, não milhares) -- o próprio gatilho da `RMLX-3`, pelo
//         mesmo briefing, é um motivo MEDIDO pra mudar isto, não um chute feito agora.
//         `find_by_id` é iterativa (pilha explícita, não recursão) mesmo sendo trivialmente segura
//         de forma recursiva até o teto de `kMaxElementDepth` -- mantida iterativa mesmo assim como
//         a mesma disciplina "árvore é onde recursão vira stack overflow" que o teto de
//         profundidade abaixo já aplica à construção, aplicada aqui à travessia também, como uma
//         segunda linha de defesa independente.
//
//     POLÍTICA DE LOOKUP, DECLARADA EXPLICITAMENTE PORQUE O CORPUS JÁ EXERCITA O CASO AMBÍGUO:
//     `find_by_id` retorna o PRIMEIRO casamento em pré-ordem (o próprio nó, depois os filhos em
//     ordem-fonte, profundidade primeiro), e ids duplicados NÃO são rejeitados nem deduplicados em
//     lugar nenhum deste modelo -- isto não é um caso de borda hipotético: o corpus real já tem
//     valores de `id` duplicados hoje (`grep -oh 'id="[^"]*"' glintfx/tests/*.rml
//     glintfx/demos/**/*.rml | sort | uniq -c | sort -rn`, ex.: `id="ctrl_ascii"` ×4,
//     `id="scroller"` ×3, `id="line1"` ×2, mais várias) -- então "primeiro casamento em pré-ordem,
//     unicidade não imposta" é a política com que este corpus já convive, não um chute sobre um
//     caso que ninguém bate. `find_by_id("")` sempre retorna `nullptr` de imediato, nunca casando
//     um elemento sem id: um id vazio e um id ausente são o MESMO estado (a própria assimetria de
//     valor-vazio da seção 7 do uix-dom.md, "um id vazio e sem id falham igualmente toda busca
//     `#foo` upstream, não há diferença comportamental a preservar") -- buscar "o elemento sem id"
//     não é uma consulta com sentido e este modelo nunca responde isso como se fosse.
//
//     ARMAZENAMENTO DE ID/CLASS/ATRIBUTO JÁ SATISFAZ A ORDENAÇÃO DA SEÇÃO 7 DO uix-dom.md "DE
//     GRAÇA": `Element::classes()` é um `std::set<std::string, std::less<>>` (ordenado ascendente,
//     byte-a-byte, deduplicado POR CONSTRUÇÃO -- exatamente a política "divide, descarta vazio,
//     deduplica, ordena ascendente byte-a-byte" da seção 7, com zero código extra de ordenar/
//     deduplicar em lugar nenhum, incluindo no futuro dumper `S6b`, que só precisa iterar);
//     `Element::attributes()` é um `std::map<std::string, std::string, std::less<>>`, mesma
//     garantia de ordenação byte-a-byte pelo mesmo motivo (`std::less<>` sobre `std::string`/
//     `std::string_view` é `operator<`, que pra `std::string` é definido via
//     `char_traits<char>::compare`, ele mesmo especificado pra comparar como se fosse `unsigned
//     char` -- ou seja, exatamente a ordem byte-a-byte, não-sensível-a-locale que a seção 7 exige,
//     mesmo raciocínio "sem `std::locale`" que aquele documento já dá pra por que essa é a escolha
//     certa, não um acidente do que `std::sort` por acaso faz). `id`/`class` são nomes RESERVADOS
//     no mapa genérico de atributo -- `set_attribute("id", ...)`/`set_attribute("class", ...)` os
//     dois retornam `false` e não guardam nada -- então existe exatamente UM lugar neste modelo que
//     pode guardar o id de um elemento, e exatamente um pro conjunto de classe dele; uma futura
//     `S6b` não pode acidentalmente ler uma cópia obsoleta/duplicada do lugar errado, e
//     `has_attribute("id")` é sempre `false` por construção (nunca casada especialmente de forma
//     explícita, cai de graça da reserva acima) -- batendo com "um dumper conforme nunca emite
//     `ATTR id=...` nem `ATTR class=...`" da seção 7 desde a camada de armazenamento pra cima, não
//     só na hora do dump.
//
//     HARDENING -- TETO DE PROFUNDIDADE DE ANINHAMENTO (fail-high, mesma disciplina do
//     `kMaxInputBytes`/`kMaxTokenBytes` do lexer.hpp, a convenção "input validation em borda" deste
//     projeto aplicada uma camada mais pra dentro, já que uma árvore é exatamente onde recursão
//     sem limite vira um crash de stack overflow): `kMaxElementDepth` (256) limita quão fundo o
//     `Element::append_child` deixa a árvore aninhar (a raiz `body` é profundidade 1; um filho de
//     `body` é profundidade 2; ...). Derivado, não inventado: a maior profundidade de aninhamento
//     real medida nas 40 fixtures do corpus hoje é **5** (scan baseado em pilha de aninhamento de
//     tag sobre `glintfx/tests/*.rml` + `glintfx/demos/**/*.rml`, `glintfx/tests/
//     fonteng_perf_scene.rml` a fixture mais funda -- isto conta as tags de embrulho `<rml>`/
//     `<head>` também, então a profundidade real só-da-subárvore-`body` que este módulo vai ver da
//     `S3` é menor ainda). A tela representativa do próprio GusWorld (`battle_cockpit_rml.cpp`,
//     seção 2 do docs/rmlx-subset.md) não é medível diretamente a partir deste repo, então --
//     espelhando o próprio raciocínio de margem do lexer.hpp pros dois tetos de byte dele em vez de
//     chutar um número preciso pra um consumidor não-medido -- `kMaxElementDepth` é setado com uma
//     margem generosa de ~50x sobre o máximo medido do corpus (256 vs. 5), não um ajuste apertado:
//     confortavelmente o bastante pras telas compostas mais fundas do GusWorld sem nunca ser
//     alcançável por markup realista, mantendo ao mesmo tempo um teto DURO e FINITO que um input
//     hostil/malformado não consegue exceder. 256 níveis de encadeamento de destrutor recursivo de
//     `unique_ptr<Node>` (o destrutor desta classe conta com o DEFAULT, recursivo -- ver o papel do
//     teto em manter esse default seguro) fica trivialmente dentro do orçamento de stack de
//     qualquer thread razoável (algumas centenas de bytes por frame no pior caso, dezenas de KB no
//     total, contra uma stack default de thread Linux na ordem de MiB) -- isto é FAIL-HIGH como os
//     tetos do lexer.hpp: `append_child` REJEITA (retorna `AppendOutcome::RejectedDepthCeiling`,
//     não adota o filho, não trunca/reparenta/derruba um nível em silêncio) em vez de algum dia
//     construir uma árvore funda o bastante pra tornar o próprio destrutor inseguro.
//
//     SEM LOGGING NESTE MÓDULO (deliberado, não descuido): os dois resultados não-`Appended` do
//     `append_child` são sinalizados puramente como DADO (`AppendOutcome`), nunca impressos/logados
//     aqui. Este módulo fica livre-de-dependência-de-link (sem RmlUi/GLFW/GL, pelo próprio
//     briefing que rege este arco e pelo gate do `tools/check_rml_whitelist.sh`) -- trazer o
//     `glintfx::log` (`glintfx/include/glintfx/log.hpp`) amarraria este alvo CMake standalone ao
//     resto do grafo de build da Camada 1 por um único `fprintf`, que é exatamente a propriedade de
//     custo-de-build que o próprio comentário de cabeçalho do `glintfx/src/uix/dom/CMakeLists.txt`
//     diz que este módulo existe pra evitar. O chamador (`S3` hoje; qualquer onda futura que linkar
//     `glintfx::log`) decide se/como expor um resultado `RejectedDepthCeiling`/
//     `FilteredWhitespaceText` -- o mesmo contrato "esta camada emite dado diagnosticável, a
//     PRÓXIMA camada decide o que fazer com ele" que o próprio token pegajoso `Error` do lexer.hpp
//     já estabelece uma camada abaixo.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace glintfx::uix {

// EN: See this file's header comment, "Hardening" paragraph, for the derivation of this value.
// PT: Ver o parágrafo "Hardening" do comentário de cabeçalho deste arquivo pra derivação deste
//     valor.
inline constexpr std::size_t kMaxElementDepth = 256;

enum class NodeKind { Element,
                      Text };

class Element;

// EN: Common base of `Element` and `Text`. Not constructible on its own (protected constructor) --
//     see each derived class for what it adds. Non-copyable/non-movable: every `Node` this tree
//     ever holds is owned exactly once, by exactly one `std::unique_ptr` inside its parent's
//     `children_` vector (or by `Document::body_` for the root) -- see this file's header comment,
//     "Ownership" is the whole point of this module, and a copy/move constructor would either have
//     to deep-clone a subtree (an operation nobody in `S3`/`S4` asked for) or silently steal
//     ownership out from under a `parent_`/`depth_` this class itself maintains -- both are
//     footguns this module simply does not offer a way to reach for.
// PT: Base comum de `Element` e `Text`. Não construível sozinha (construtor protected) -- ver cada
//     classe derivada pro que ela soma. Não-copiável/não-movível: todo `Node` que esta árvore
//     algum dia guarda tem posse exatamente uma vez, por exatamente um `std::unique_ptr` dentro do
//     vetor `children_` do pai dele (ou por `Document::body_` pra raiz) -- ver o comentário de
//     cabeçalho deste arquivo, "posse" é o ponto inteiro deste módulo, e um construtor de cópia/
//     move teria ou que clonar em profundidade uma subárvore (operação que ninguém na `S3`/`S4`
//     pediu) ou roubar posse em silêncio de debaixo de um `parent_`/`depth_` que esta própria
//     classe mantém -- os dois são armadilhas que este módulo simplesmente não dá jeito de
//     alcançar.
class Node {
public:
  virtual ~Node() = default;

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(Node&&) = delete;
  Node& operator=(Node&&) = delete;

  NodeKind kind() const;

  // EN: `nullptr` for a node with no parent (the `Document::body()` root, or a freshly-constructed
  //     detached node not yet passed to `append_child`).
  // PT: `nullptr` pra um nó sem pai (a raiz `Document::body()`, ou um nó recém-construído e
  //     desanexado, ainda não passado pra `append_child`).
  Element* parent();
  const Element* parent() const;

  // EN: 1 for the `Document::body()` root and any detached node not yet parented; `parent()->
  //     depth() + 1` once `append_child` adopts it. See this file's header comment, "Hardening"
  //     paragraph, for how this bounds recursive destructor/traversal safety.
  // PT: 1 pra raiz `Document::body()` e pra qualquer nó desanexado ainda não parentado;
  //     `parent()->depth() + 1` assim que `append_child` o adota. Ver o parágrafo "Hardening" do
  //     comentário de cabeçalho deste arquivo pra como isto limita a segurança de destrutor/
  //     travessia recursivos.
  std::size_t depth() const;

protected:
  explicit Node(NodeKind kind);

private:
  friend class Element;

  NodeKind kind_;
  Element* parent_ = nullptr;
  std::size_t depth_ = 1;
};

// EN: A text-content leaf. Never has children (uix-dom.md section 5: "text node, exactly one
//     line, no children possible") -- this class simply does not declare a `children_` member,
//     which is the strongest way to enforce that ("a node type that cannot have children" beats
//     "a node type that could but we promise never populates it").
// PT: Uma folha de conteúdo textual. Nunca tem filhos (seção 5 do uix-dom.md: "nó texto,
//     exatamente uma linha, sem filhos possíveis") -- esta classe simplesmente não declara membro
//     `children_` nenhum, que é a forma mais forte de impor isso ("um tipo de nó que não PODE ter
//     filhos" vence "um tipo de nó que poderia mas prometemos nunca popular").
class Text final : public Node {
public:
  // EN: `content` is stored byte-verbatim, exactly as given -- no trim, no whitespace-run
  //     collapse, no entity decoding (uix-dom.md section 6b/6c: decoding and verbatim-preservation
  //     are the CALLER's job, upstream of this constructor -- by the time `S3` builds a `Text`,
  //     entity decoding has already happened, this class never re-interprets its own content).
  // PT: `content` é guardado byte-verbatim, exatamente como recebido -- sem trim, sem colapso de
  //     run de whitespace, sem decodificação de entidade (seção 6b/6c do uix-dom.md: decodificação
  //     e preservação-verbatim são trabalho de QUEM CHAMA, anterior a este construtor -- no
  //     momento em que a S3 constrói um `Text`, a decodificação de entidade já aconteceu, esta
  //     classe nunca reinterpreta o próprio conteúdo).
  explicit Text(std::string content);

  std::string_view content() const;
  void set_content(std::string new_content);

private:
  std::string content_;
};

// EN: The three possible outcomes of `Element::append_child` -- see this file's header comment,
//     points (1) and "Hardening", for why these two non-`Appended` cases exist and why neither one
//     is signalled by exception/abort/log.
// PT: Os três resultados possíveis de `Element::append_child` -- ver o comentário de cabeçalho
//     deste arquivo, pontos (1) e "Hardening", pra por que estes dois casos não-`Appended` existem
//     e por que nenhum dos dois é sinalizado por exceção/abort/log.
enum class AppendOutcome {
  Appended,
  FilteredWhitespaceText,
  RejectedDepthCeiling,
};

struct AppendResult {
  // EN: Non-null iff `outcome == Appended`; observer pointer into the child now owned by the
  //     parent's `children_` (never by the caller -- ownership of the argument transfers into this
  //     call unconditionally, adopted on success, destroyed on either rejection, exactly like
  //     `std::vector::push_back` never hands anything back on success or failure).
  // PT: Não-nulo sse `outcome == Appended`; ponteiro observador pro filho agora de posse do
  //     `children_` do pai (nunca de quem chamou -- a posse do argumento transfere pra esta
  //     chamada incondicionalmente, adotado no sucesso, destruído nas duas rejeições, exatamente
  //     como `std::vector::push_back` nunca devolve nada nem no sucesso nem na falha).
  Node* node = nullptr;
  AppendOutcome outcome = AppendOutcome::Appended;
};

// EN: An element node: tag name, optional id, class set, generic attribute map, ordered children.
//     See this file's header comment for the load-bearing design decisions (whitespace-existence
//     filter, id/class reservation, depth ceiling, no document-wide id index).
// PT: Um nó elemento: nome de tag, id opcional, conjunto de classe, mapa de atributo genérico,
//     filhos ordenados. Ver o comentário de cabeçalho deste arquivo pras decisões de desenho que
//     carregam peso (filtro de existência de whitespace, reserva de id/class, teto de profundidade,
//     sem índice de id documento-inteiro).
class Element final : public Node {
public:
  // EN: `tag` is stored exactly as given, case preserved -- uix-dom.md section 8: tag-name
  //     lowercasing (if any) is upstream `XMLParser`/`S3` territory, this constructor never folds
  //     case on its own initiative.
  // PT: `tag` é guardado exatamente como recebido, caixa preservada -- seção 8 do uix-dom.md:
  //     dobrar caixa de nome de tag (se houver) é território do `XMLParser`/`S3` upstream, este
  //     construtor nunca dobra caixa por iniciativa própria.
  explicit Element(std::string tag);

  std::string_view tag() const;

  // EN: Empty string means "no id", identical to an explicit `id=""` -- uix-dom.md section 7's own
  //     stated empty-value asymmetry for `id` specifically (unlike a generic attribute, where
  //     empty-value and absent are genuinely different states).
  // PT: String vazia significa "sem id", idêntico a um `id=""` explícito -- a própria assimetria
  //     de valor-vazio da seção 7 do uix-dom.md pra `id` especificamente (diferente de um atributo
  //     genérico, onde valor-vazio e ausente são estados genuinamente diferentes).
  std::string_view id() const;
  bool has_id() const;
  void set_id(std::string new_id);

  // EN: `cls` must be a single, already-split token -- returns `false` (no-op) for:
  //       (1) an EMPTY token (`cls.empty()`) -- there is no such thing as "the empty class".
  //       (2) a token containing a literal SPACE (`' '`, 0x20) ANYWHERE -- space is upstream
  //           RmlUi's sole class-list delimiter (`ElementStyle::SetClassNames`'s
  //           `StringUtilities::ExpandString(classes, class_names, ' ')`,
  //           `examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`; see uix-dom.md section 7,
  //           revised 2026-08-05, `UIX-CLASS-SPLIT-SPEC`). A token that still contains a space
  //           means the CALLER (`S3`) failed to split on it -- a caller bug this invariant
  //           catches, not a legitimate RmlUi class name.
  //       (3) a token made ENTIRELY of the other 3 whitespace bytes (`\t`/`\n`/`\r`, no space
  //           per (2), and nothing else) -- upstream's `ExpandString` never actually captures a
  //           run of ONLY those 3 bytes as a token: its scan only advances `start_ptr`/`end_ptr`
  //           on a NON-whitespace byte, so a segment with nothing else degenerates to "no token
  //           captured there" (an empty/skipped delimiter run upstream, per uix-dom.md section
  //           7's ledger row dated 2026-08-05) -- never a real single-token class this tree
  //           should hold. Equivalent to `std::all_of(cls.begin(), cls.end(),
  //           is_whitespace_char)` reusing the SAME 4-char predicate as (2)/section 6, since (2)
  //           already ruled out the space case, only `\t`/`\n`/`\r` are left to be "entirely".
  //     Accepted (unlike the pre-`UIX-CLASS-SPLIT-2` version of this check): a token with
  //     `\t`/`\n`/`\r` EMBEDDED between two non-whitespace bytes, e.g. `"a\tb"` -- upstream's
  //     `ExpandString` DOES capture that whole span verbatim (the tab sits between two bytes its
  //     scan already captured, so the emitted token is a contiguous slice of the original string
  //     including the tab -- see uix-dom.md section 7's own "`a\ta` survives, `\tb` becomes `b`"
  //     worked distinction). Rejecting `"a\tb"` would have been THIS tree's own divergence from
  //     RmlUi, not a rule RmlUi itself enforces -- `UIX-CLASS-SPLIT-2` (RMLX-1, 2026-08-05) closed
  //     that gap. The CALLER (`S3`) is still the one that cuts a raw `class="a b"` attribute value
  //     into individual segments before calling this (parser.cpp's own `split_class_tokens`,
  //     replicating `ExpandString(raw, ' ')` byte for byte) -- this single check remains a
  //     TREE-STRUCTURE invariant ("every member of this element's class SET is a single,
  //     already-split token"), not a grammar re-validation; see this file's header comment for why
  //     that distinction matters.
  // PT: `cls` precisa ser um único token já-separado -- retorna `false` (no-op) pra:
  //       (1) um token VAZIO (`cls.empty()`) -- não existe "a classe vazia".
  //       (2) um token contendo um ESPAÇO literal (`' '`, 0x20) EM QUALQUER POSIÇÃO -- espaço é o
  //           único delimitador de lista-de-classe do RmlUi upstream
  //           (`ElementStyle::SetClassNames`'s `StringUtilities::ExpandString(classes,
  //           class_names, ' ')`, `examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`; ver
  //           seção 7 do uix-dom.md, revisada 2026-08-05, `UIX-CLASS-SPLIT-SPEC`). Um token que
  //           ainda contém espaço significa que QUEM CHAMA (a `S3`) falhou em separar por ele --
  //           um bug de chamador que este invariante pega, não um nome de classe legítimo do
  //           RmlUi.
  //       (3) um token feito INTEIRAMENTE dos outros 3 bytes de whitespace (`\t`/`\n`/`\r`, sem
  //           espaço pelo item (2), e nada mais) -- o `ExpandString` upstream nunca captura de
  //           fato um run de SÓ esses 3 bytes como token: o scan dele só avança
  //           `start_ptr`/`end_ptr` num byte NÃO-whitespace, então um segmento sem mais nada
  //           degenera pra "nenhum token capturado ali" (um run de delimitador vazio/pulado
  //           upstream, pela própria linha do ledger da seção 7 do uix-dom.md datada de
  //           2026-08-05) -- nunca uma classe de token-único de verdade que esta árvore deveria
  //           guardar. Equivalente a `std::all_of(cls.begin(), cls.end(), is_whitespace_char)`
  //           reaproveitando o MESMO predicado de 4 caracteres do item (2)/seção 6, já que o item
  //           (2) já descartou o caso de espaço, só sobram `\t`/`\n`/`\r` pra serem "inteiramente".
  //     Aceito (diferente da versão pré-`UIX-CLASS-SPLIT-2` desta checagem): um token com
  //     `\t`/`\n`/`\r` EMBUTIDO entre dois bytes não-whitespace, ex.: `"a\tb"` -- o `ExpandString`
  //     upstream CAPTURA esse span inteiro verbatim (o tab fica entre dois bytes que o scan já
  //     capturou, então o token emitido é uma fatia contígua da string original incluindo o tab --
  //     ver a própria distinção trabalhada "`a\ta` sobrevive, `\tb` vira `b`" da seção 7 do
  //     uix-dom.md). Rejeitar `"a\tb"` teria sido uma divergência DESTA árvore em relação ao
  //     RmlUi, não uma regra que o próprio RmlUi impõe -- a `UIX-CLASS-SPLIT-2` (RMLX-1,
  //     2026-08-05) fechou essa lacuna. QUEM CHAMA (a `S3`) continua sendo quem corta um valor de
  //     atributo `class="a b"` cru em segmentos individuais antes de chamar isto (o próprio
  //     `split_class_tokens` do parser.cpp, replicando `ExpandString(raw, ' ')` byte a byte) --
  //     esta única checagem continua sendo um invariante de ESTRUTURA-DE-ÁRVORE ("todo membro do
  //     conjunto de classe deste elemento é um token único, já separado"), não uma revalidação de
  //     gramática; ver o comentário de cabeçalho deste arquivo pra por que essa distinção importa.
  bool has_class(std::string_view cls) const;
  bool add_class(std::string_view cls);
  bool remove_class(std::string_view cls);

  // EN: Sorted ascending byte-wise, deduplicated -- uix-dom.md section 7, satisfied "for free" by
  //     `std::set`'s own invariant, see this file's header comment.
  // PT: Ordenado ascendente byte-a-byte, deduplicado -- seção 7 do uix-dom.md, satisfeita "de
  //     graça" pelo próprio invariante do `std::set`, ver o comentário de cabeçalho deste arquivo.
  const std::set<std::string, std::less<>>& classes() const;

  // EN: `name == "id"` or `name == "class"` is REJECTED (returns `false`, stores nothing) -- those
  //     two names are reserved for `set_id`/`add_class` exclusively, see this file's header
  //     comment for why this keeps exactly one storage location per concept.
  // PT: `name == "id"` ou `name == "class"` é REJEITADO (retorna `false`, não guarda nada) -- esses
  //     dois nomes são reservados exclusivamente pra `set_id`/`add_class`, ver o comentário de
  //     cabeçalho deste arquivo pra por que isto mantém exatamente um lugar de armazenamento por
  //     conceito.
  bool set_attribute(std::string name, std::string value);
  bool has_attribute(std::string_view name) const;
  std::optional<std::string_view> attribute(std::string_view name) const;
  bool remove_attribute(std::string_view name);

  // EN: Sorted by name, ascending byte-wise -- uix-dom.md section 7, satisfied "for free" by
  //     `std::map`'s own invariant. Never contains an `"id"`/`"class"` key -- see `set_attribute`.
  // PT: Ordenado por nome, ascendente byte-a-byte -- seção 7 do uix-dom.md, satisfeita "de graça"
  //     pelo próprio invariante do `std::map`. Nunca contém chave `"id"`/`"class"` -- ver
  //     `set_attribute`.
  const std::map<std::string, std::string, std::less<>>& attributes() const;

  // EN: Takes ownership of `child` unconditionally (see `AppendResult`'s own comment for the
  //     precise contract on rejection). `child` must be non-null -- a caller passing a null
  //     `unique_ptr` is a programming-error-grade precondition violation (this constructor's
  //     border is trusted-internal-caller data, see this file's header comment on why grammar/
  //     null-checks this deep are the wrong layer), not diagnosable input this class defends
  //     against.
  // PT: Toma posse de `child` incondicionalmente (ver o próprio comentário de `AppendResult` pro
  //     contrato exato na rejeição). `child` precisa ser não-nulo -- um chamador passando um
  //     `unique_ptr` nulo é violação de pré-condição grau-erro-de-programação (a fronteira deste
  //     construtor é dado de chamador-interno-confiável, ver o comentário de cabeçalho deste
  //     arquivo pra por que checagem de gramática/nulo nesta profundidade é a camada errada), não
  //     input diagnosticável contra o qual esta classe se defende.
  AppendResult append_child(std::unique_ptr<Node> child);

  // EN: UIX-REMOVE-CHILD (RMLX-1, 2026-08-05). Removes `child` from this element's children,
  //     destroying it (the `unique_ptr` owning it is erased from `children_`, running `child`'s
  //     destructor -- including any of ITS OWN descendants, recursively, exactly like the rest of
  //     this tree's ownership model). Returns `true` iff `child` was found (by pointer identity,
  //     among THIS element's DIRECT children only -- not a recursive search) and removed; `false`
  //     for `child == nullptr` or a `child` that is not a direct child of `this` (a grandchild, a
  //     child of a DIFFERENT element, or already removed) -- a no-op, tree untouched.
  //
  //     WHY `bool`, NOT AN `AppendResult`-SHAPED STRUCT: matches this class's own existing
  //     "found it/didn't find it" convention for `remove_class`/`remove_attribute` above, not
  //     `append_child`'s richer `AppendResult` -- unlike `append_child`, there is no MEANINGFUL
  //     distinction to report beyond "removed" vs. "nothing to remove". Removal has no analogue to
  //     `FilteredWhitespaceText` (nothing here decides "should this count as a child", that
  //     question was already answered when the child was appended) nor to `RejectedDepthCeiling`
  //     (removing a node can only ever REDUCE the depth other nodes sit at, never grow it, so
  //     there is no ceiling to hit on the way out). `bool` says everything a caller needs; a struct
  //     would be ceremony without content.
  // PT: UIX-REMOVE-CHILD (RMLX-1, 2026-08-05). Remove `child` dos filhos deste elemento,
  //     destruindo-o (o `unique_ptr` que o possui é apagado de `children_`, rodando o destrutor de
  //     `child` -- incluindo os PRÓPRIOS descendentes dele, recursivamente, exatamente como o
  //     resto do modelo de posse desta árvore). Retorna `true` sse `child` foi achado (por
  //     identidade de ponteiro, só entre os filhos DIRETOS deste elemento -- não é busca
  //     recursiva) e removido; `false` pra `child == nullptr` ou um `child` que não é filho direto
  //     de `this` (um neto, um filho de um elemento DIFERENTE, ou já removido) -- um no-op, árvore
  //     intocada.
  //
  //     POR QUE `bool`, NÃO UMA STRUCT NO MOLDE DO `AppendResult`: bate com a própria convenção já
  //     existente desta classe "achou/não achou" de `remove_class`/`remove_attribute` acima, não
  //     com o `AppendResult` mais rico do `append_child` -- diferente do `append_child`, não existe
  //     distinção SIGNIFICATIVA pra reportar além de "removeu" vs. "nada pra remover". Remoção não
  //     tem análogo a `FilteredWhitespaceText` (nada aqui decide "isto deveria contar como filho",
  //     essa pergunta já foi respondida quando o filho foi somado) nem a `RejectedDepthCeiling`
  //     (remover um nó só consegue REDUZIR a profundidade em que outros nós estão, nunca aumentar,
  //     então não há teto pra bater na saída). `bool` diz tudo que um chamador precisa; uma struct
  //     seria cerimônia sem conteúdo.
  bool remove_child(const Node* child);

  // EN: UIX-REMOVE-CHILD (RMLX-1, 2026-08-05). Removes ALL of this element's children
  //     unconditionally, destroying each one (same recursive-destructor discipline as
  //     `remove_child` above). Returns the NUMBER of children removed (`0` if already childless --
  //     idempotent in both directions: calling this on an empty element is a safe no-op that
  //     reports exactly that, `0`, not an error).
  //
  //     WHY `std::size_t`, NOT `bool` (unlike `remove_child` above): the richer signal is
  //     essentially free here -- `children_.size()` is already known before the clear -- and
  //     genuinely useful to a caller doing a bulk replace. `dom_api.cpp`'s `set_text` (the exact
  //     call site this primitive was added for) uses it only for its side effect and discards the
  //     count, but a future caller diffing "did I just destroy N nodes" should not have to
  //     re-derive `N` from a `child_count()` captured separately before the call.
  // PT: UIX-REMOVE-CHILD (RMLX-1, 2026-08-05). Remove TODOS os filhos deste elemento
  //     incondicionalmente, destruindo cada um (mesma disciplina de destrutor recursivo do
  //     `remove_child` acima). Retorna o NÚMERO de filhos removidos (`0` se já sem filhos --
  //     idempotente nas duas direções: chamar isto num elemento vazio é um no-op seguro que
  //     reporta exatamente isso, `0`, não um erro).
  //
  //     POR QUE `std::size_t`, NÃO `bool` (diferente do `remove_child` acima): o sinal mais rico é
  //     essencialmente de graça aqui -- `children_.size()` já é conhecido antes do clear -- e
  //     genuinamente útil pra um chamador fazendo uma substituição em massa. O `set_text` do
  //     `dom_api.cpp` (o próprio ponto de chamada pro qual este primitivo foi somado) usa só pelo
  //     efeito colateral e descarta a contagem, mas um chamador futuro comparando "acabei de
  //     destruir N nós" não deveria ter que re-derivar `N` de um `child_count()` capturado à parte
  //     antes da chamada.
  std::size_t clear_children();

  const std::vector<std::unique_ptr<Node>>& children() const;
  std::size_t child_count() const;

  // EN: Pre-order DFS starting at (and including) `this`; first match wins; `nullptr` if none.
  //     `id` empty always returns `nullptr` immediately. See this file's header comment, "Lookup
  //     policy", for why duplicate ids are a real, corpus-measured case this deliberately does not
  //     reject.
  // PT: DFS pré-ordem começando em (e incluindo) `this`; primeiro casamento vence; `nullptr` se
  //     nenhum. `id` vazio sempre retorna `nullptr` de imediato. Ver o comentário de cabeçalho
  //     deste arquivo, "Política de lookup", pra por que ids duplicados são um caso real, medido no
  //     corpus, que isto deliberadamente não rejeita.
  Element* find_by_id(std::string_view id);
  const Element* find_by_id(std::string_view id) const;

private:
  std::string tag_;
  std::string id_;
  std::set<std::string, std::less<>> classes_;
  std::map<std::string, std::string, std::less<>> attributes_;
  std::vector<std::unique_ptr<Node>> children_;
};

// EN: Mirrors uix-dom.md section 4's `HEAD ABSENT` / `HEAD PRESENT <raw>` shape 1:1.
//     `present == false` (the default) is `HEAD ABSENT`; `raw` holds the exact source bytes from
//     just after `<head>`'s closing `>` to just before `</head>`'s opening `<`, UN-escaped (section
//     2's escaping is a DUMP-TIME concern, `S6b`'s job, not stored here) and UN-entity-decoded
//     (section 4's own last paragraph: this payload is exempt from entity decoding entirely, CSS
//     has no entity syntax).
// PT: Espelha 1:1 a forma `HEAD ABSENT` / `HEAD PRESENT <cru>` da seção 4 do uix-dom.md.
//     `present == false` (o default) é `HEAD ABSENT`; `raw` guarda os bytes-fonte exatos desde logo
//     após o `>` de fechamento de `<head>` até logo antes do `<` de abertura de `</head>`,
//     NÃO-escapado (o escaping da seção 2 é preocupação de TEMPO-DE-DUMP, trabalho da `S6b`, não
//     guardado aqui) e NÃO-entity-decodificado (o próprio último parágrafo da seção 4: este payload
//     é isento de decodificação de entidade por completo, CSS não tem sintaxe de entidade).
struct HeadContent {
  bool present = false;
  std::string raw;
};

// EN: Sole ownership root of a parsed document -- see this file's header comment, point (3), for
//     why this exists in an "S2" slice whose brief names only `Node`/`Element`/`Text`.
// PT: Raiz de posse única de um documento parseado -- ver o comentário de cabeçalho deste arquivo,
//     ponto (3), pra por que isto existe numa fatia "S2" cujo briefing nomeia só `Node`/`Element`/
//     `Text`.
class Document {
public:
  // EN: Constructs `body()` immediately -- an `Element` with tag `"body"`, depth 1, no parent,
  //     empty id/classes/attributes/children. There is no "document with no body" state this class
  //     can represent (uix-dom.md section 3: "a document missing `<body>` entirely is a parse
  //     error, out of this format's scope" -- if `S3` cannot find a `<body>`, it simply never
  //     produces a `Document` at all, rather than producing one this class would have to represent
  //     as headless).
  // PT: Constrói `body()` de imediato -- um `Element` com tag `"body"`, profundidade 1, sem pai,
  //     id/classes/atributos/filhos vazios. Não existe estado "documento sem body" que esta classe
  //     consiga representar (seção 3 do uix-dom.md: "um documento sem `<body>` nenhum é erro de
  //     parse, fora do escopo deste formato" -- se a `S3` não conseguir achar um `<body>`, ela
  //     simplesmente nunca produz `Document` nenhum, em vez de produzir um que esta classe teria
  //     que representar como sem-cabeça).
  Document();

  Element& body();
  const Element& body() const;

  const HeadContent& head() const;
  void set_head(std::string raw);
  void clear_head();

private:
  std::unique_ptr<Element> body_;
  HeadContent head_;
};

// EN: `nullptr` if `node` is `nullptr` or `node->kind()` does not match -- convenience wrappers
//     around a `kind()`-checked `static_cast`, sparing every consumer (`S3`, a future `S6b`) from
//     writing the same check inline at every downcast site.
// PT: `nullptr` se `node` for `nullptr` ou `node->kind()` não casar -- wrappers de conveniência em
//     volta de um `static_cast` checado por `kind()`, poupando todo consumidor (`S3`, uma futura
//     `S6b`) de escrever a mesma checagem inline em todo lugar que faz downcast.
Element* as_element(Node* node);
const Element* as_element(const Node* node);
Text* as_text(Node* node);
const Text* as_text(const Node* node);

} // namespace glintfx::uix
