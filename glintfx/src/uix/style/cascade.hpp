// SPDX-License-Identifier: Apache-2.0
// EN: UIX-CASCADE -- glintfx's own style CASCADE: given our DOM (`glintfx::uix::Element`,
//     `glintfx/src/uix/dom/dom_tree.hpp`) and a parsed `StyleSheet` (`parser.hpp`'s S4), decides,
//     for every registered property (`property_registry.hpp`'s 72-entry table) and every element,
//     which declaration WINS -- specificity, then source order, per CSS/RCSS's own textbook
//     precedence rule -- falls back to inheritance for `inherited` properties with no winning
//     declaration anywhere in that element's own cascade, and falls back to the registry's own
//     initial value for everything else. This is the SIXTH brick of glintfx's own style engine,
//     immediately downstream of `selector_match.hpp`'s S5 ("casador de seletor") and immediately
//     upstream of a future value-computation-integration slice -- see "Deliberately NOT this file's
//     job" below for exactly where this file's own boundary sits and why.
//
//     WHAT "WINS" MEANS, RE-DERIVED FROM UPSTREAM BY RE-STUDY, NOT GUESSED (this task's own brief:
//     "se ela não bastar... isso é defeito da spec: pare e reporte, não improvise" -- neither
//     `docs/uix-rcss.md` nor `docs/rmlx-subset.md` names a source-order/`rule_count` formula
//     anywhere; `selector_match.hpp`'s own header states outright it deliberately does NOT
//     re-derive this term, naming it this file's own job). The exact upstream mechanism, read
//     directly from the pinned RE-study clone:
//       `examples/RmlUi/Source/Core/StyleSheetParser.cpp:690` declares `int rule_count = 0` once
//       per `Parse()` call (i.e. once per WHOLE stylesheet, not per rule, not per selector) and
//       `:749` increments it by exactly ONE after every `{...}` block -- including a
//       comma-separated block, whose `rule_name_list` loop (`:734-747`) imports EVERY entry of the
//       list under the SAME `rule_count` value, only incrementing once AFTER the whole loop. This
//       is this file's own `Rule` (`parser.hpp`), one increment per entry of `StyleSheet::rules`,
//       in that vector's own source order.
//       `StyleSheetNode.cpp:142-144` (`ImportProperties`): every property a rule sets is stamped
//       with `specificity + rule_specificity` -- the rule's own MATCHED chain specificity
//       (`selector_match.hpp`'s `Specificity`) plus that same `rule_count` value, added directly
//       into ONE integer, never kept as a separate tuple field.
//       `PropertyDictionary.cpp:134-142` (`SetProperty`): `if (existing.specificity >
//       new_specificity) return;` -- otherwise overwrite. A STRICTLY higher stamped value keeps the
//       old declaration; an EQUAL or higher one overwrites. This is `>=`-overwrites, not `>`.
//       `ElementDefinition.cpp:7-11` / `StyleSheet.cpp:214-221` (`GetElementDefinition`): the nodes
//       whose selector matched the element are merged into one `PropertyDictionary`, one property
//       at a time, via the SAME `SetProperty` comparison above -- the merge ORDER upstream chooses
//       (nodes sorted by their own bare CHAIN specificity, ties broken by raw pointer value) never
//       changes the OUTCOME, because `SetProperty`'s own comparison decides purely from the two
//       STAMPED specificity values being compared, which already have `rule_count` baked in -- see
//       "Why per-rule winner selection is provably equivalent" below for the arithmetic that makes
//       this true, not merely asserted.
//
//     THIS FILE'S OWN FORMULA, re-derived from the four citations above WITHOUT packing two
//     dimensions into one integer (an earlier revision did; see "A CEILING THIS FILE USED TO HAVE,
//     AND WHY IT DOES NOT ANY MORE" below for the bug that packing caused and why the fix REMOVES
//     the encoding instead of widening it): for each `Rule` in `sheet.rules`, in that vector's own
//     source order, a property's declaration in this rule REPLACES the current winner iff
//     `match_selector_list(rule.selectors, element, state).specificity >= <current winner's own
//     chain specificity>` -- comparing the two chain specificities DIRECTLY, matching `SetProperty`'s
//     own `>`-keeps/`>=`-overwrites direction exactly (`match_selector_list`'s own documented
//     "returns the MAXIMUM specificity among every matching entry" already reproduces `rule_count`'s
//     own "every comma-list entry shares one value" fact for the ONE rule being considered -- see
//     selector_match.hpp's own "Comma-list semantics" paragraph, which this file inherits without
//     re-deriving). Processing rules in ascending source-order position, and declarations within one
//     rule in their own source-order vector position, with this SAME `>=`-overwrites comparison at
//     every step, is what makes source-order tie-break correct WITHOUT encoding "which position was
//     this" into any number: visiting two EQUAL-specificity declarations in that fixed order and
//     always overwriting on a tie makes the LATER one win purely because this file's own single
//     ascending pass visits it later, a property of the loop shape, not of a stored integer -- no
//     `StyleSheetNode` tree, no separate merge pass, no node-processing-order question to resolve,
//     and (the point the packed encoding got wrong) no ceiling on how many rules a sheet may have.
//
//     A CEILING THIS FILE USED TO HAVE, AND WHY IT DOES NOT ANY MORE (`UIX-CASCADE-TETO-REGRAS`,
//     found by the orchestrator's own re-verification pass, not by this file's own original test
//     suite): an earlier revision stamped `combined = chain_specificity + rule_index` into ONE
//     `Specificity` integer per winning declaration, reasoning that since `kSpecificityWeightTag`
//     (10'000, `selector_match.hpp`) is the SMALLEST non-zero per-compound weight upstream defines,
//     `combined mod 10'000` / `combined div 10'000` recover both terms uniquely as long as
//     `rule_index < 10'000` -- true, but ONLY inside that bound, and the bound was never enforced
//     anywhere in code (grep confirms: zero occurrences of `10'000`/`10000` in this file's own .cpp
//     outside a comment). Outside it the failure mode is NOT upstream's own bounded ambiguity-
//     between-EQUALS (`PropertyDictionary.cpp:134-142`'s pointer-order tie-break only ever fires
//     between EQUAL stamped values, never inverts distinct ones) -- it is a genuine INVERSION: once
//     the GAP between two rules' own `rule_index` values reached or exceeded the GAP between their
//     two chain specificities, the sum let the LOWER-specificity, LATER-declared rule outrank a
//     HIGHER-specificity, earlier one (worked example: a 1-tag chain, specificity 10'000, at
//     `rule_index` 15'000 stamps `combined = 25'000`; a 2-tag chain, specificity 20'000 -- DOUBLE --
//     at `rule_index` 0 stamps `combined = 20'000`; the weaker chain wins). The bound this file's
//     own prior revision leaned on ("this repo's own corpus has 866 rules, two orders of magnitude
//     under 10'000") was an assumption about ONE repo's OWN corpus, not a ceiling this project's own
//     distribution target (a general-purpose library, not a fixed app) can assume of every consumer
//     -- and this module's own sibling `lexer.hpp`'s `kMaxInputBytes` (1 MiB) sanctions well over
//     170'000 rule blocks in a single stylesheet (a 6-byte minimal block, `a{b:c}`, repeated), 17x
//     past the 10'000 the additive encoding actually needed to stay correct: the two declared
//     ceilings were mutually incompatible, and nothing in this module cross-checked them. THE FIX:
//     remove the ceiling instead of raising it -- comparing chain specificities directly, with no
//     sum, has no analogous term to overflow or collide, and is correct for ANY rule count this
//     `std::vector<Rule>` can hold, not merely comfortable ones. `Specificity` was already
//     `std::int64_t` (selector_match.hpp) before this fix -- the earlier revision's ceiling was
//     never a matter of integer WIDTH, widening it further would not have helped; the flaw was
//     packing two independent dimensions into one comparable scalar at all, which this file's own
//     single ascending-source-order pass never actually needed (that packing was solving upstream's
//     merge-ORDER-independence problem, a problem this file's own single-pass shape does not have).
//     `tests/uix_style/cascade_sanity.cpp`'s own
//     `test_precedence_holds_beyond_former_rule_index_ceiling` pins the exact former failure point
//     (the boundary where the two rules' own `rule_index` gap first reached their specificity gap,
//     and one step past it) directly against `compute_element_style`.
//
//     INHERITANCE (`property_registry.hpp`'s own `inherited` flag): a property with no winning
//     declaration ANYWHERE in this element's own rule set (not merely no rule matched at all -- a
//     rule can match and simply not set this particular property) inherits the PARENT's own
//     already-computed value for that same property, iff `inherited == true` AND a parent exists.
//     The root element (`glintfx::uix::Element::parent() == nullptr`, `cascade_tree`'s own `root`
//     argument) has no parent to inherit from, by construction (`docs/uix-dom.md` section 3's own
//     "a document missing `<body>`... out of this format's scope" -- there is no styleable
//     ancestor of `body` this dump could reach even if it wanted to). `opacity`'s own
//     `inherited: true` row (`docs/uix-rcss.md` section 6.1, flagged ⚠️ there as a DELIBERATE
//     divergence from real CSS, where `opacity` does not inherit and instead compounds visually)
//     needs NO special-casing here: this file inherits the single cascaded NUMBER, never
//     multiplies across ancestors -- exactly what that section's own text already states is the
//     intended, simpler-than-real-CSS behaviour for this dump, not an accident this file must work
//     around.
//
//     INITIAL VALUE FALLBACK: a property with no winning declaration and either NOT `inherited` or
//     no parent to inherit from takes `property_registry.hpp`'s own `initial_value` verbatim.
//
//     OWNERSHIP -- WHY `ComputedProperty::value` IS ALWAYS AN OWNED `std::string`, NEVER A
//     `string_view`, EVEN THOUGH A DECLARED WINNER'S RAW TEXT IS A ZERO-COPY VIEW INTO THE
//     CALLER'S OWN `sheet`'s SOURCE BUFFER (`parser.hpp`'s own "zero-copy" contract) UNTIL THIS
//     FILE TOUCHES IT: inheritance can chain across an unbounded number of tree levels
//     (`dom_tree.hpp`'s own `kMaxElementDepth`, 256), and `cascade_tree`'s own recursive walk keeps
//     each level's `ComputedStyle` alive only as a LOCAL stack variable for exactly as long as that
//     level's own children are being visited (see `cascade_visit`'s own doc-comment in the .cpp) --
//     a grandchild's inherited value, if left as a `string_view` chasing back through two ancestor
//     frames, would dangle the moment the OLDER of those two frames returns, even though the
//     grandchild's own `NodeStyle` entry in the caller-visible output vector is still very much
//     alive. Copying once, at the point of inheritance (and at the point of taking a declared or
//     initial value too, for the SAME uniform reason -- no `ComputedProperty` in this file's own
//     output ever holds a view into anything this file itself does not own for the OUTPUT vector's
//     own full lifetime), removes that whole class of footgun for a cost this project's own
//     "profile before optimizing" discipline calls trivial at this scale: 72 short strings per
//     element, dozens of elements per corpus fixture (`dom_tree.hpp`'s own measured corpus
//     citations), not thousands.
//
//     DELIBERATELY NOT THIS FILE'S JOB -- A SCOPE GAP FOUND BY RE-STUDY, REPORTED HERE RATHER THAN
//     IMPROVISED (this task's own brief's own governing rule, restated): `docs/uix-rcss.md` section
//     15.1's own byte-exact worked example prints `opacity=0.5000` (quantized to 4 digits, §8) and
//     `color=#223344ff` (canonical 8-digit hex, §7.1) for a SOURCE declaration that was literally
//     `opacity: 0.5;` / `color: #223344;` -- neither of those two printed forms is the raw
//     declaration text, which is exactly what THIS file's own winner-selection produces
//     (`ComputedProperty::value` for a declared winner is the `PropertyDeclaration::value` text,
//     copied verbatim, never passed through `value_compute.hpp`'s `quantize`/`parse_color`/
//     `print_color`/etc.). `property_registry.hpp`'s own `initial_value` field is further, concrete
//     evidence this is the RIGHT boundary, not a shortcut: it stores `"black"`/`"transparent"`/
//     `"1"` -- raw, pre-canonical keyword/number text, the EXACT SAME representation tier a raw
//     declared value sits at, never the hex/quantized form `value_compute.hpp` would produce from
//     it. Running `value_compute` selectively (only on declared values, never on initial ones, since
//     the registry itself is not in that form) would produce an internally INCONSISTENT
//     `ComputedStyle` -- some entries canonical, some not, with no marker distinguishing which --
//     exactly the kind of silent, undocumented asymmetry this project's own conventions warn
//     against. This file's own pipeline position, per this task's own brief ("lexer -> parser ->
//     casador -> CASCATA -> valor computado -> dump", five arrows, six *distinct* named stages) is
//     BEFORE "valor computado", not the same stage as it -- a future slice owns dispatching each of
//     the 72 properties' own winning raw text through the correct `value_compute.hpp` function
//     (itself needing, per `property_registry.hpp`'s own `has_alternate_domain` field and
//     `shorthand.hpp`'s own `looks_like_number_token`/`looks_like_length_token` precedent, a new
//     text-shape classifier this file's own brief never asked for and this header does not invent
//     one for).
//       - Resolving `%`/layout at all (`docs/uix-rcss.md` section 1's own "before... RMLX-3" line,
//         already `selector_match.hpp`'s and `value_compute.hpp`'s own shared boundary, unchanged
//         here).
//       - `@font-face`/`@keyframes` (`parser.hpp`'s own `StyleSheet::font_faces`/`::keyframes`) --
//         this file walks `StyleSheet::rules` ONLY; those two lists are a structurally separate ID
//         space (`parser.hpp`'s own header comment, "🔴 EXCEPTION, DELIBERATE, DOCUMENTED") this
//         file never touches, matching every sibling module's own boundary at this same fork.
//       - Producing the `STATE`/`PROPS`/`PROP` TEXT FORMAT `docs/uix-rcss.md` section 3 defines --
//         that is a future dumper's own job, consuming `cascade_tree`'s own `NodeStyle` vector (path
//         addressing per `docs/uix-dom.md` section 3, escaping per its section 2) the same way a
//         future `S6b` was always going to consume a finished tree, not something this file
//         produces itself.
//
//     NAMESPACE: `glintfx::uix::style`, same module as every sibling header. Depends on
//     `glintfx::uix::Element`/`as_element` (the flat DOM namespace) by fully-qualified name at every
//     use, same discipline `selector_match.hpp` already established for this exact cross-module
//     dependency.
// PT: UIX-CASCADE -- a própria CASCATA de estilo da glintfx: dado o nosso DOM (`glintfx::uix::
//     Element`, `glintfx/src/uix/dom/dom_tree.hpp`) e um `StyleSheet` já parseado (a S4 do
//     parser.hpp), decide, pra toda propriedade registrada (a tabela de 72 entradas do
//     property_registry.hpp) e todo elemento, qual declaração VENCE -- especificidade, depois ordem
//     de origem, pela própria regra de precedência de livro-texto do CSS/RCSS -- cai pra herança nas
//     propriedades `inherited` sem declaração vencedora em lugar nenhum da própria cascata daquele
//     elemento, e cai pro próprio valor inicial do registro pra tudo mais. Este é o SEXTO tijolo do
//     motor de estilo próprio da glintfx, imediatamente rio-abaixo da S5 do selector_match.hpp ("o
//     casador de seletor") e imediatamente rio-acima de uma futura fatia de integração de
//     computação-de-valor -- ver "Deliberadamente NÃO é trabalho deste arquivo" abaixo pra
//     exatamente onde a própria fronteira deste arquivo fica e por quê.
//
//     O QUE "VENCE" SIGNIFICA, RE-DERIVADO DO UPSTREAM POR RE-ESTUDO, NÃO CHUTADO (o próprio
//     briefing desta tarefa: "se ela não bastar... isso é defeito da spec: pare e reporte, não
//     improvise" -- nem o `docs/uix-rcss.md` nem o `docs/rmlx-subset.md` nomeia uma fórmula de
//     ordem-de-fonte/`rule_count` em lugar nenhum; o próprio cabeçalho do selector_match.hpp declara
//     categoricamente que ele deliberadamente NÃO re-deriva este termo, nomeando isto como trabalho
//     PRÓPRIO deste arquivo). O mecanismo exato do upstream, lido direto do clone de RE-estudo
//     pinado:
//       `examples/RmlUi/Source/Core/StyleSheetParser.cpp:690` declara `int rule_count = 0` uma vez
//       por chamada de `Parse()` (ou seja, uma vez pra FOLHA DE ESTILO INTEIRA, não por regra, não
//       por seletor) e `:749` incrementa ele em exatamente UM depois de todo bloco `{...}` --
//       incluindo um bloco separado-por-vírgula, cujo próprio laço `rule_name_list` (`:734-747`)
//       importa TODA entrada da lista sob o MESMO valor de `rule_count`, incrementando só uma vez
//       DEPOIS do laço inteiro. Isto é a própria `Rule` deste arquivo (parser.hpp), um incremento
//       por entrada de `StyleSheet::rules`, na própria ordem-fonte daquele vetor.
//       `StyleSheetNode.cpp:142-144` (`ImportProperties`): toda propriedade que uma regra ajusta é
//       carimbada com `specificity + rule_specificity` -- a própria especificidade de cadeia CASADA
//       da regra (o `Specificity` do selector_match.hpp) mais aquele mesmo valor de `rule_count`,
//       somados diretamente num inteiro só, nunca guardados como campo de tupla separado.
//       `PropertyDictionary.cpp:134-142` (`SetProperty`): `if (existing.specificity >
//       new_specificity) return;` -- senão sobrescreve. Um valor carimbado ESTRITAMENTE maior
//       mantém a declaração velha; um IGUAL ou maior sobrescreve. Isto é `>=`-sobrescreve, não `>`.
//       `ElementDefinition.cpp:7-11` / `StyleSheet.cpp:214-221` (`GetElementDefinition`): os nós
//       cujo seletor casou com o elemento são mesclados numa `PropertyDictionary` só, uma
//       propriedade de cada vez, via a MESMA comparação `SetProperty` acima -- a ORDEM de mesclagem
//       que o upstream escolhe (nós ordenados pela própria especificidade de CADEIA nua, empates
//       desempatados por valor de ponteiro cru) nunca muda o RESULTADO, porque a própria comparação
//       do `SetProperty` decide puramente a partir dos dois valores de especificidade CARIMBADOS
//       sendo comparados, que já têm `rule_count` embutido -- ver "Por que a seleção de vencedor
//       por-regra é provavelmente equivalente" abaixo pra aritmética que torna isto verdade, não
//       meramente afirmado.
//
//     A PRÓPRIA FÓRMULA DESTE ARQUIVO, re-derivada das quatro citações acima SEM empacotar duas
//     dimensões num inteiro só (uma revisão anterior empacotava; ver "UM TETO QUE ESTE ARQUIVO
//     TINHA, E POR QUE NÃO TEM MAIS" abaixo pro defeito que aquele empacotamento causava e por que o
//     conserto REMOVE a codificação em vez de alargá-la): pra toda `Rule` em `sheet.rules`, na
//     própria ordem-fonte daquele vetor, a declaração de uma propriedade nesta regra SUBSTITUI o
//     vencedor atual sse `match_selector_list(rule.selectors, element, state).specificity >=
//     <própria especificidade de cadeia do vencedor atual>` -- comparando as duas especificidades de
//     cadeia DIRETAMENTE, batendo com a própria direção `>`-mantém/`>=`-sobrescreve do `SetProperty`
//     exatamente (o próprio "retorna a especificidade MÁXIMA entre toda entrada que casa"
//     documentado do `match_selector_list` já reproduz o próprio fato "toda entrada de
//     lista-vírgula compartilha um valor só" do `rule_count` pra ÚNICA regra sendo considerada --
//     ver o próprio parágrafo "Semântica de lista-vírgula" do selector_match.hpp, que este arquivo
//     herda sem re-derivar). Processar regras em posição ordem-fonte ascendente, e declarações
//     dentro de uma regra na própria posição-de-vetor ordem-fonte delas, com esta MESMA comparação
//     `>=`-sobrescreve a cada passo, é o que torna o desempate por ordem-fonte correto SEM
//     codificar "qual posição era essa" em número nenhum: visitar duas declarações de
//     especificidade IGUAL naquela ordem fixa e sempre sobrescrever em empate faz a POSTERIOR
//     vencer puramente porque a própria passada única, ascendente, deste arquivo a visita depois,
//     uma propriedade da FORMA do laço, não de um inteiro guardado -- nenhuma árvore
//     `StyleSheetNode`, nenhuma passada de mesclagem separada, nenhuma pergunta de ordem-de-
//     processamento-de-nó pra resolver, e (o ponto que a codificação empacotada errava) nenhum teto
//     em quantas regras uma folha pode ter.
//
//     UM TETO QUE ESTE ARQUIVO TINHA, E POR QUE NÃO TEM MAIS (`UIX-CASCADE-TETO-REGRAS`, achado pela
//     própria passada de re-verificação do orquestrador, não pela própria suíte de teste original
//     deste arquivo): uma revisão anterior carimbava `combined = especificidade_de_cadeia +
//     rule_index` num inteiro `Specificity` SÓ por declaração vencedora, raciocinando que, como
//     `kSpecificityWeightTag` (10'000, selector_match.hpp) é o MENOR peso não-zero por-compound que
//     o upstream define, `combined mod 10'000` / `combined div 10'000` recuperam os dois termos
//     unicamente contanto que `rule_index < 10'000` -- verdadeiro, mas SÓ dentro daquele limite, e o
//     limite nunca foi imposto em código nenhum (grep confirma: zero ocorrências de
//     `10'000`/`10000` no próprio .cpp deste arquivo fora de comentário). Fora dele o modo de falha
//     NÃO é a própria ambiguidade limitada-entre-IGUAIS do upstream (o próprio desempate de
//     ordem-de-ponteiro do `PropertyDictionary.cpp:134-142` só dispara entre valores carimbados
//     IGUAIS, nunca inverte distintos) -- é uma INVERSÃO genuína: uma vez que a DIFERENÇA entre os
//     próprios valores de `rule_index` de duas regras alcançava ou excedia a DIFERENÇA entre as duas
//     especificidades de cadeia delas, a soma deixava a regra de especificidade MENOR, declarada
//     DEPOIS, superar uma de especificidade MAIOR, anterior (exemplo trabalhado: uma cadeia de 1 tag,
//     especificidade 10'000, no `rule_index` 15'000 carimba `combined = 25'000`; uma cadeia de 2
//     tags, especificidade 20'000 -- O DOBRO -- no `rule_index` 0 carimba `combined = 20'000`; a
//     cadeia mais fraca vence). O limite em que a própria revisão anterior deste arquivo se apoiava
//     ("o corpus deste repo tem 866 regras, duas ordens de magnitude abaixo de 10'000") era uma
//     suposição sobre o corpus PRÓPRIO de UM repo, não um teto que o próprio alvo de distribuição
//     deste projeto (uma biblioteca de propósito geral, não um app fixo) pode supor de todo
//     consumidor -- e o próprio `kMaxInputBytes` (1 MiB) do `lexer.hpp` irmão deste módulo sanciona
//     bem mais de 170'000 blocos de regra numa folha de estilo só (um bloco mínimo de 6 bytes,
//     `a{b:c}`, repetido), 17x além dos 10'000 que a codificação aditiva de fato precisava pra
//     seguir correta: os dois tetos declarados eram mutuamente incompatíveis, e nada neste módulo os
//     cruzava. O CONSERTO: remover o teto em vez de alargá-lo -- comparar especificidades de cadeia
//     diretamente, sem soma, não tem termo análogo pra transbordar ou colidir, e é correto pra
//     QUALQUER contagem de regra que este `std::vector<Rule>` aguente, não só contagens confortáveis.
//     `Specificity` já era `std::int64_t` (selector_match.hpp) antes deste conserto -- o teto da
//     revisão anterior nunca foi questão de LARGURA de inteiro, alargar mais não teria ajudado; o
//     defeito era empacotar duas dimensões independentes num escalar comparável só, o que a própria
//     passada única, ordem-fonte ascendente, deste arquivo nunca precisou de fato (aquele
//     empacotamento resolvia o problema de independência-de-ORDEM-de-mesclagem do upstream, um
//     problema que a própria forma de passada única deste arquivo não tem). O próprio
//     `test_precedence_holds_beyond_former_rule_index_ceiling` do
//     tests/uix_style/cascade_sanity.cpp prova o próprio ponto de falha anterior exato (a fronteira
//     onde a própria diferença de `rule_index` das duas regras primeiro alcançava a diferença de
//     especificidade delas, e um passo além dela) diretamente contra o `compute_element_style`.
//
//     HERANÇA (a própria flag `inherited` do property_registry.hpp): uma propriedade sem declaração
//     vencedora em LUGAR NENHUM do próprio conjunto de regra deste elemento (não meramente "nenhuma
//     regra casou de jeito nenhum" -- uma regra pode casar e simplesmente não ajustar esta
//     propriedade específica) herda o próprio valor JÁ COMPUTADO do PAI pra essa mesma propriedade,
//     sse `inherited == true` E um pai existe. O elemento raiz (`glintfx::uix::Element::parent() ==
//     nullptr`, o próprio argumento `root` do `cascade_tree`) não tem pai nenhum pra herdar, por
//     construção (a própria "um documento sem `<body>` nenhum... fora do escopo deste formato" da
//     seção 3 do docs/uix-dom.md -- não existe ancestral estilizável de `body` que este dump
//     conseguisse alcançar mesmo que quisesse). A própria linha `inherited: true` do `opacity`
//     (seção 6.1 do docs/uix-rcss.md, marcada ⚠️ ali como uma divergência DELIBERADA do CSS real,
//     onde `opacity` não herda e em vez disso compõe visualmente) não precisa de caso especial
//     nenhum aqui: este arquivo herda o NÚMERO cascateado único, nunca multiplica através de
//     ancestrais -- exatamente o comportamento pretendido, mais simples que o CSS real, que o
//     próprio texto daquela seção já declara pra este dump, não um acidente que este arquivo precisa
//     contornar.
//
//     FALLBACK DE VALOR INICIAL: uma propriedade sem declaração vencedora e ou NÃO `inherited` ou
//     sem pai pra herdar toma o próprio `initial_value` do property_registry.hpp verbatim.
//
//     POSSE -- POR QUE `ComputedProperty::value` É SEMPRE UM `std::string` DE POSSE, NUNCA UM
//     `string_view`, MESMO O TEXTO CRU DE UM VENCEDOR DECLARADO SENDO UMA VIEW ZERO-CÓPIA SOBRE O
//     PRÓPRIO BUFFER-FONTE DO `sheet` DO CHAMADOR (o próprio contrato "zero-cópia" do parser.hpp)
//     ATÉ ESTE ARQUIVO TOCÁ-LO: herança pode encadear através de um número ilimitado de níveis de
//     árvore (o próprio `kMaxElementDepth` do dom_tree.hpp, 256), e a própria travessia recursiva do
//     `cascade_tree` mantém o `ComputedStyle` de cada nível vivo só como uma variável LOCAL de stack
//     por exatamente o tempo que os próprios filhos daquele nível estão sendo visitados (ver o
//     próprio comentário de doc do `cascade_visit` no .cpp) -- o valor herdado de um neto, se
//     deixado como `string_view` perseguindo de volta através de dois frames ancestrais, ficaria
//     pendurado no momento em que o MAIS VELHO desses dois frames retornasse, mesmo a própria
//     entrada `NodeStyle` do neto no vetor de saída visível-ao-chamador ainda estando bem viva.
//     Copiar uma vez, no ponto da herança (e no ponto de tomar um valor declarado ou inicial também,
//     pelo MESMO motivo uniforme -- nenhum `ComputedProperty` na própria saída deste arquivo algum
//     dia guarda uma view sobre algo que este próprio arquivo não possui pela própria vida inteira
//     do vetor de SAÍDA), remove essa classe inteira de armadilha por um custo que a própria
//     disciplina "profilear antes de otimizar" deste projeto chama de trivial nesta escala: 72
//     strings curtas por elemento, dezenas de elemento por fixture de corpus (as próprias citações
//     de corpus medido do dom_tree.hpp), não milhares.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE ARQUIVO -- UMA LACUNA DE ESCOPO ACHADA POR RE-ESTUDO,
//     REPORTADA AQUI EM VEZ DE IMPROVISADA (a própria regra que rege o briefing desta tarefa,
//     restatada): o próprio exemplo trabalhado byte-exato da seção 15.1 do docs/uix-rcss.md imprime
//     `opacity=0.5000` (quantizado a 4 dígitos, §8) e `color=#223344ff` (hex canônico de 8 dígitos,
//     §7.1) pra uma declaração-FONTE que era literalmente `opacity: 0.5;` / `color: #223344;` --
//     nenhuma dessas duas formas impressas é o próprio texto de declaração cru, que é exatamente o
//     que a própria seleção-de-vencedor DESTE arquivo produz (`ComputedProperty::value` pra um
//     vencedor declarado é o próprio texto `PropertyDeclaration::value`, copiado verbatim, nunca
//     passado pelo `quantize`/`parse_color`/`print_color`/etc. do value_compute.hpp). O próprio
//     campo `initial_value` do property_registry.hpp é evidência concreta adicional de que esta é a
//     fronteira CERTA, não um atalho: guarda `"black"`/`"transparent"`/`"1"` -- texto
//     palavra-chave/número cru, pré-canônico, exatamente o MESMO patamar de representação em que um
//     valor declarado cru se senta, nunca a forma hex/quantizada que o value_compute.hpp produziria
//     dele. Rodar o `value_compute` seletivamente (só em valores declarados, nunca nos iniciais, já
//     que o próprio registro não está naquela forma) produziria um `ComputedStyle` internamente
//     INCONSISTENTE -- algumas entradas canônicas, outras não, sem marcador nenhum distinguindo
//     qual -- exatamente o tipo de assimetria silenciosa, não-documentada que as próprias convenções
//     deste projeto avisam contra. A própria posição de pipeline deste arquivo, per o próprio
//     briefing desta tarefa ("lexer -> parser -> casador -> CASCATA -> valor computado -> dump",
//     cinco setas, seis estágios *distintos* nomeados) é ANTES de "valor computado", não o mesmo
//     estágio dele -- uma futura fatia possui o trabalho de despachar o próprio texto cru vencedor
//     de cada uma das 72 propriedades pela função certa do value_compute.hpp (precisando ela mesma,
//     per o próprio campo `has_alternate_domain` do property_registry.hpp e o próprio precedente
//     `looks_like_number_token`/`looks_like_length_token` do shorthand.hpp, de um classificador de
//     forma-de-texto novo que o próprio briefing deste item nunca pediu e este cabeçalho não inventa
//     um pra isso).
//       - Resolver `%`/layout de jeito nenhum (a própria linha "antes... da RMLX-3" da seção 1 do
//         docs/uix-rcss.md, já a própria fronteira compartilhada do selector_match.hpp e do
//         value_compute.hpp, inalterada aqui).
//       - `@font-face`/`@keyframes` (os próprios `StyleSheet::font_faces`/`::keyframes` do
//         parser.hpp) -- este arquivo percorre `StyleSheet::rules` SÓ; essas duas listas são um
//         espaço de ID estruturalmente separado (o próprio comentário de cabeçalho do parser.hpp,
//         "🔴 EXCEÇÃO, DELIBERADA, DOCUMENTADA") que este arquivo nunca toca, batendo com a própria
//         fronteira de todo módulo irmão nesta mesma bifurcação.
//       - Produzir o FORMATO DE TEXTO `STATE`/`PROPS`/`PROP` que a seção 3 do docs/uix-rcss.md
//         define -- isso é trabalho de um futuro dumper, consumindo o próprio vetor `NodeStyle` do
//         `cascade_tree` (endereçamento de caminho per a seção 3 do docs/uix-dom.md, escaping per a
//         seção 2 dele) do mesmo jeito que uma futura `S6b` sempre ia consumir uma árvore pronta,
//         não algo que este arquivo produz ele mesmo.
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo de todo cabeçalho irmão. Depende de
//     `glintfx::uix::Element`/`as_element` (o namespace plano do DOM) por nome totalmente
//     qualificado em todo uso, mesma disciplina que o selector_match.hpp já estabeleceu pra esta
//     exata dependência cross-módulo.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "uix/dom/dom_tree.hpp"
#include "uix/style/parser.hpp"
#include "uix/style/selector_match.hpp"

namespace glintfx::uix::style {

// EN: One computed property. `name` is a stable `string_view` into `property_registry.cpp`'s own
//     static table (static storage duration, always valid) -- see this file's header comment,
//     "Ownership", for why `value` is always an OWNED copy instead.
// PT: Uma propriedade computada. `name` é um `string_view` estável sobre a própria tabela estática
//     do property_registry.cpp (duração de armazenamento estática, sempre válida) -- ver o parágrafo
//     "Posse" do comentário de cabeçalho deste arquivo pro porquê de `value` ser sempre uma cópia DE
//     POSSE em vez disso.
struct ComputedProperty {
  std::string_view name;
  std::string value;
};

// EN: Always exactly `all_properties().size()` (72) entries, in the EXACT SAME order
//     `property_registry.hpp`'s `all_properties()` declares them (ascending, byte-wise, by name) --
//     `compute_element_style`'s own loop below builds this by walking that span index-by-index, so
//     this ordering is a structural guarantee, not a happenstance of insertion order, and never
//     leaks any internal map/set iteration order (this file uses none).
// PT: Sempre exatamente `all_properties().size()` (72) entradas, na EXATA MESMA ordem que
//     `all_properties()` do property_registry.hpp as declara (ascendente, byte-a-byte, por nome) --
//     o próprio laço do `compute_element_style` abaixo constrói isto percorrendo aquele span
//     índice-por-índice, então esta ordenação é uma garantia estrutural, não um acaso de ordem de
//     inserção, e nunca vaza ordem de iteração de mapa/set interno nenhuma (este arquivo não usa
//     nenhum).
using ComputedStyle = std::vector<ComputedProperty>;

// EN: Computes the cascaded style of ONE element -- see this file's header comment for the full
//     algorithm (specificity + source order, inheritance, initial-value fallback). `parent_style`
//     is `nullptr` for the root element (no ancestor to inherit from) or a non-null pointer to the
//     PARENT's own already-computed `ComputedStyle` (same 72-entry, same-order invariant this
//     function itself produces) -- `cascade_tree` below is the caller that keeps this contract
//     (parent computed before any child) for a whole tree; a caller computing a single detached
//     element's style with no real parent passes `nullptr` deliberately, exactly like the root case.
// PT: Computa o estilo cascateado de UM elemento -- ver o comentário de cabeçalho deste arquivo pro
//     algoritmo completo (especificidade + ordem de origem, herança, fallback de valor inicial).
//     `parent_style` é `nullptr` pro elemento raiz (nenhum ancestral pra herdar) ou um ponteiro
//     não-nulo pro próprio `ComputedStyle` JÁ COMPUTADO do PAI (mesmo invariante de 72-entradas,
//     mesma-ordem que esta própria função produz) -- o `cascade_tree` abaixo é o chamador que
//     mantém este contrato (pai computado antes de qualquer filho) pra uma árvore inteira; um
//     chamador computando o estilo de um único elemento desanexado sem pai de verdade passa
//     `nullptr` deliberadamente, exatamente como o caso raiz.
ComputedStyle compute_element_style(const StyleSheet& sheet, const glintfx::uix::Element& element,
                                    const MatchState& state, const ComputedStyle* parent_style);

// EN: One tree node's own computed style, paired with an observer pointer to the `Element` it
//     belongs to (owned by the DOM tree the caller passed to `cascade_tree`, never by this struct --
//     same "observer, not owner" convention `dom_tree.hpp`'s own `AppendResult::node` already
//     establishes).
// PT: O próprio estilo computado de um nó de árvore, emparelhado com um ponteiro observador pro
//     `Element` a que pertence (de posse da árvore DOM que o chamador passou pro `cascade_tree`,
//     nunca deste struct -- mesma convenção "observador, não dono" que o próprio
//     `AppendResult::node` do dom_tree.hpp já estabelece).
struct NodeStyle {
  const glintfx::uix::Element* element = nullptr;
  ComputedStyle style;
};

// EN: Walks `root`'s own subtree (root included), pre-order depth-first, EXCLUDING text nodes --
//     `docs/uix-rcss.md` section 2's own "only element nodes carry `PROP` records" -- computing each
//     element's style with its own already-computed parent supplied for inheritance. This is the
//     SAME traversal order `docs/uix-dom.md` section 5 fixes for this DOM model (and the exact order
//     a future dumper needs to address nodes by path, per that same section). `root`'s own recursion
//     depth is bounded by the SAME `kMaxElementDepth` (256) ceiling `dom_tree.hpp`'s own
//     `Element::append_child` already enforces when the tree was built -- this function introduces
//     no NEW unbounded recursion of its own, it only ever recurses as deep as a tree `append_child`
//     already agreed to build.
// PT: Percorre a própria subárvore de `root` (raiz incluída), pré-ordem profundidade-primeiro,
//     EXCLUINDO nós de texto -- o próprio "só nós elemento carregam registros `PROP`" da seção 2 do
//     docs/uix-rcss.md -- computando o estilo de cada elemento com o próprio pai já-computado dele
//     suprido pra herança. Esta é a MESMA ordem de travessia que a seção 5 do docs/uix-dom.md fixa
//     pra este modelo de DOM (e a ordem exata que um futuro dumper precisa pra endereçar nó por
//     caminho, per aquela mesma seção). A própria profundidade de recursão de `root` é limitada pelo
//     MESMO teto `kMaxElementDepth` (256) que o próprio `Element::append_child` do dom_tree.hpp já
//     impõe quando a árvore foi construída -- esta função não introduz recursão NOVA sem limite
//     própria nenhuma, ela só recursa tão fundo quanto uma árvore que o `append_child` já concordou
//     em construir.
std::vector<NodeStyle> cascade_tree(const StyleSheet& sheet, const glintfx::uix::Element& root,
                                    const MatchState& state);

} // namespace glintfx::uix::style
