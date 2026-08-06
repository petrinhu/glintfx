// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SELECTOR-MATCH -- the selector MATCHER: given an `Element` from glintfx's own DOM
//     (`glintfx/src/uix/dom/dom_tree.hpp`) and a `Selector`/`SelectorList` already structured by
//     `parser.hpp`'s S3 (the RCSS-side sibling, "the FOURTH tijolo"), answers two questions --
//     does it match, and at what SPECIFICITY -- for exactly the 8 selector forms
//     `docs/rmlx-subset.md` section 6.2 authorizes (restated once, at `compound_matches`'s own
//     definition in the .cpp, not paraphrased here). This is the FIFTH brick of glintfx's own
//     style engine, `RMLX-2`'s own "casador de seletor" item, immediately downstream of S3/parser
//     and immediately upstream of a future cascade slice (`RMLX-2`+, not this item's job -- see
//     "Deliberately NOT this file's job" below).
//
//     WHAT THIS FILE ADDS THAT PARSER.HPP DELIBERATELY DOES NOT: parser.hpp's own header comment
//     says this in so many words -- "Isto NÃO é *casamento* de seletor -- não há DOM aqui, nenhum
//     cálculo de especificidade, nenhuma avaliação de estado-`:hover`". This file is exactly that
//     missing half: it is the first file in this module to `#include "uix/dom/dom_tree.hpp"` (a
//     new, deliberate, one-directional dependency of `glintfx::uix::style` on `glintfx::uix` --
//     see this file's own CMakeLists.txt entries for how both build graphs wire the extra
//     `dom_tree.cpp` translation unit in), and the first to compute a numeric specificity or
//     evaluate a pseudo-class against anything.
//
//     THE SPECIFICITY ALGORITHM -- A DOCUMENTED SPEC GAP THIS FILE CLOSES BY RE-STUDY, NOT BY
//     GUESSING: neither `docs/uix-rcss.md` nor `docs/rmlx-subset.md` states a specificity FORMULA
//     anywhere (both mention the *word* "specificity" only in passing, e.g. uix-rcss.md section 1's
//     "the cascade (origin, specificity, source order)" and `docs/effects.md`'s own worked
//     scrollbar-override example, which gives ROUNDED figures -- "~10000 per tag", "~20000" for a
//     two-tag compound -- evidence of the SHAPE of the algorithm, not its exact constants). Per
//     this task's own brief ("se ela não bastar... isso é defeito da spec: pare e reporte, não
//     improvise"), this is reported here and in this item's own `TODO.md` entry -- but it is NOT
//     left unresolved, because this project's own established RE-study convention (CLAUDE.md's
//     "pesquisar... reimplementar clean-room", already exercised by every sibling in this module,
//     e.g. value_compute.hpp's `examples/RmlUi/Source/Core/ComputeProperty.cpp` citations) applies
//     identically here: the EXACT upstream algorithm, read directly from the pinned RE-study clone,
//     is:
//       `examples/RmlUi/Source/Core/StyleSheetSelector.h:13-21` (`namespace SelectorSpecificity`):
//         `Tag = 10'000`, `Class = Attribute = PseudoClass = 100'000`, `ID = 1'000'000`.
//       `examples/RmlUi/Source/Core/StyleSheetNode.cpp:369-390` (`CalculateAndSetSpecificity`):
//         one compound's own specificity is `(tag.empty() ? 0 : Tag) + (id.empty() ? 0 : ID) +
//         Class*class_names.size() + Attribute*attributes.size() + PseudoClass*pseudo_class_names
//         .size()`, and a WHOLE selector chain's specificity is the SUM of every compound's own
//         specificity in the chain (upstream computes this cumulatively while building a tree of
//         `StyleSheetNode`, "then add our parent's specificity onto ours", `:387-389` -- summing
//         over a flat chain, as this module's own `Selector::compounds` already is, is arithmetically
//         identical to that cumulative parent-add, since specificity is a pure additive quantity
//         with no combinator-dependent weighting: `Combinator::Descendant` vs. `::Child` never
//         appears anywhere in `CalculateAndSetSpecificity`'s own body).
//     `kSpecificityWeightAttribute` is NOT declared below: `[attr]` is zero-measured and OUT of
//     this subset (`docs/rmlx-subset.md` section 6.2), so no compound this module's own callers can
//     construct (via `parser.hpp`'s `parse_compound`) ever has a non-empty attribute list to weigh
//     -- adding a constant for a selector form this repo's own parser cannot produce would be dead
//     code pretending to be a decision.
//     ⚠️ WHAT THIS FILE DELIBERATELY DOES **NOT** RE-DERIVE FROM UPSTREAM: the `rule_count`
//     source-order term (`StyleSheetParser.cpp:749`'s `rule_count++`, folded into the SAME integer
//     via `ImportProperties`' own `specificity + rule_specificity`, `StyleSheetNode.cpp:144`) is
//     upstream's mechanism for breaking a tie between two DIFFERENT rules of EQUAL compound
//     specificity by source order -- that is a CASCADE-WIDE decision spanning every rule in a
//     sheet (and, per `StyleSheetContainer`'s `specificity_offset` accumulation, potentially every
//     MERGED sheet), squarely the next slice's ("cascade") job, per this module's own sibling
//     parser.hpp's stated boundary ("casamento de seletor... fatias seguintes") -- this file's own
//     `Specificity` is the per-selector-CHAIN weight alone, exactly upstream's per-`StyleSheetNode`
//     `specificity` field BEFORE `ImportProperties` adds a `rule_specificity` term to it.
//
//     COMMA-LIST SEMANTICS (`match_selector_list`, the ONE place this file combines more than one
//     fact into a single verdict, and why that is still squarely THIS item's job, not the cascade's):
//     `docs/rmlx-subset.md` section 6.1 authorizes `.a, .b { }` (15 measured, 13 files); parser.hpp's
//     own `SelectorList` doc-comment states the semantics this file must honour: "every selector in
//     the list applies the SAME declaration set". Upstream's own `StyleSheetParser::ParseProperties`
//     (`StyleSheetParser.cpp:734-747`) builds ONE independent `StyleSheetNode` PATH per comma-list
//     entry, all sharing the SAME `rule_count` (source order -- incremented once AFTER the whole
//     comma-list loop, `:749`), but each entry keeps its OWN compound-derived specificity (a bare
//     `.a` and a compound `#id .a` in the same list do NOT share a weight). When an element matches
//     MORE THAN ONE entry of the same list (e.g. `.a, .b { color: red }` against an element carrying
//     BOTH classes), upstream's `PropertyDictionary::SetProperty` (`PropertyDictionary.cpp:134-142`)
//     keeps whichever entry's specificity is HIGHER for that property (`iterator->second.specificity
//     > specificity` guards an overwrite) -- since every entry in one list shares the same source
//     order, the higher COMPOUND specificity is the one and only tie-breaker between them, an
//     entirely LOCAL fact about this one `SelectorList` that does not need any OTHER rule in the
//     sheet to resolve. `match_selector_list` therefore returns the MAXIMUM specificity among every
//     matching entry -- not "the first that matches", not "the sum of every match" -- proven by a
//     dedicated, mutation-tested case in this item's own test suite (two entries of DIFFERENT
//     specificity both matching the same element).
//
//     PSEUDO-CLASS STATE (`MatchState::hover_active`): this subset authorizes exactly ONE
//     pseudo-class, `:hover` (`parser.hpp`'s own `CompoundSelector::pseudo_hover` doc-comment,
//     "the ONE pseudo-class this subset authorizes"; `:focus`/`:active` are zero-measured in
//     SELECTOR position and out of subset, `docs/rmlx-subset.md` section 2). This module has NO
//     notion of a live UI's actual mouse/focus state -- there is no such state anywhere in
//     `dom_tree.hpp`'s own `Element` (a plain, static ownership tree, per that file's own header
//     comment, "SEM eventos/foco/hover"). Matching `:hover` therefore takes a single GLOBAL boolean,
//     forced identically on EVERY element being matched -- the exact shape `docs/uix-rcss.md`
//     section 4's own STATE matrix already fixes for the DUMP this matcher eventually feeds
//     (`none` / `hover-all`, "forced globally, for every element simultaneously", never inferred
//     per-element from static markup). This file does not invent that policy; it is the one
//     pre-existing contract in this repo that already tells a caller what `:hover` should mean
//     without a live UI.
//
//     DELIBERATELY NOT THIS FILE'S JOB (cascade / `RMLX-2`+ territory, restated so a caller does not
//     have to re-derive the boundary from parser.hpp's own header a second time):
//       - Combining specificity across DIFFERENT rules/`Selector`s of the SAME `StyleSheet` (source
//         order, `!important`-equivalent -- RCSS has none, `docs/uix-rcss.md` section 12 -- multiple
//         merged sheets' own `specificity_offset`). `match_selector`/`match_selector_list` answer
//         "does THIS ONE selector-or-list match THIS ONE element", nothing about which of several
//         MATCHING rules across a whole sheet should win a given property -- that ranking is the
//         next slice's own job, consuming THIS file's `Specificity` as one of its inputs.
//       - Walking a `StyleSheet`'s `rules` vector at all. This file takes one `Selector`/
//         `SelectorList` at a time, handed to it by a future cascade slice that already knows which
//         `Rule` it came from.
//       - Resolving `@keyframes`' own `KeyframeBlock::selector_text` (`"0%"`/`"from"`/`"to"`) --
//         that is not a DOM selector at all (parser.hpp's own doc-comment, "NÃO-RESOLVIDO... a
//         quarta semântica de `%`"), and this file's `Selector`/`SelectorList` types are never the
//         type a `KeyframeBlock` carries.
//       - Attribute selectors (`[attr]`), universal (`*`), sibling combinators (`+`/`~`),
//         `:nth-child()`/`:not()` -- all zero-measured, out of subset (`docs/rmlx-subset.md` section
//         6.2); the parser this file consumes structurally CANNOT produce a `CompoundSelector` using
//         any of these (no field exists on that struct for them), so there is nothing for this file
//         to reject at its own boundary -- the fail-high already happened one slice upstream, at
//         parse time.
//
//     COMPLEXITY, STATED HONESTLY (this project's own "profile before optimizing" discipline, not
//     "lock-free wishful thinking" applied to correctness instead of concurrency): the
//     `Combinator::Descendant` ancestor search backtracks (see the .cpp's own `match_from` for the
//     exact recursion), worst case `O(depth^compound_count)` for a pathological all-descendant
//     chain against a maximally deep tree. This is NOT tightened here: `dom_tree.hpp`'s own
//     `kMaxElementDepth` (256) already bounds `depth`, and the real corpus's own measured selector
//     chains are 1-3 compounds long (`docs/rmlx-subset.md` section 6.2's own descendant/child/
//     compound counts) against a measured tree depth of 5 -- nowhere near the ceiling where this
//     algorithm's own worst case would matter. A future cascade slice that discovers a real,
//     MEASURED hot path here (not a guess) is the one licensed to memoize/index this, per this
//     project's own "otimizar antes de medir é apostar" precedent (`dom_tree.hpp`'s own point (4)).
//
//     NAMESPACE: `glintfx::uix::style`, same module as lexer.hpp/parser.hpp/property_registry.hpp/
//     shorthand.hpp/value_compute.hpp. Depends on `glintfx::uix::Element` (the flat DOM namespace,
//     `glintfx/src/uix/dom/dom_tree.hpp`) by fully-qualified name at every use, deliberately never
//     `using namespace`-imported, so a reader never has to guess which of the two sibling modules'
//     identically-shaped-but-differently-scoped types (`glintfx::uix::style::Selector` vs. anything
//     `glintfx::uix`-flat) a bare name in this file refers to.
// PT: UIX-SELECTOR-MATCH -- o CASADOR de seletor: dado um `Element` do DOM próprio da glintfx
//     (`glintfx/src/uix/dom/dom_tree.hpp`) e um `Selector`/`SelectorList` já estruturado pela S3 do
//     parser.hpp (o irmão do lado RCSS, "o QUARTO tijolo"), responde duas perguntas -- casa, e com
//     que ESPECIFICIDADE -- pras exatas 8 formas de seletor que a seção 6.2 do
//     `docs/rmlx-subset.md` autoriza (restatada uma vez, na própria definição de `compound_matches`
//     no .cpp, não parafraseada aqui). Este é o QUINTO tijolo do motor de estilo próprio da
//     glintfx, o próprio item "casador de seletor" da `RMLX-2`, imediatamente rio-abaixo da S3/
//     parser e imediatamente rio-acima de uma futura fatia de cascata (`RMLX-2`+, não trabalho
//     deste item -- ver "Deliberadamente NÃO é trabalho deste arquivo" abaixo).
//
//     O QUE ESTE ARQUIVO SOMA QUE O PARSER.HPP DELIBERADAMENTE NÃO SOMA: o próprio comentário de
//     cabeçalho do parser.hpp diz isto com todas as letras -- "Isto NÃO é *casamento* de seletor --
//     não há DOM aqui, nenhum cálculo de especificidade, nenhuma avaliação de estado-`:hover`".
//     Este arquivo é exatamente essa metade que faltava: é o primeiro arquivo deste módulo a fazer
//     `#include "uix/dom/dom_tree.hpp"` (uma dependência nova, deliberada, de mão única, do
//     `glintfx::uix::style` sobre o `glintfx::uix` -- ver as próprias entradas de CMakeLists.txt
//     deste arquivo pra como os dois grafos de build amarram a unidade de tradução extra
//     `dom_tree.cpp`), e o primeiro a computar uma especificidade numérica ou avaliar uma
//     pseudo-classe contra qualquer coisa.
//
//     O ALGORITMO DE ESPECIFICIDADE -- UMA LACUNA DE SPEC DOCUMENTADA QUE ESTE ARQUIVO FECHA POR
//     RE-ESTUDO, NÃO POR CHUTE: nem o `docs/uix-rcss.md` nem o `docs/rmlx-subset.md` declara uma
//     FÓRMULA de especificidade em lugar nenhum (os dois mencionam a *palavra* "especificidade" só
//     de passagem, ex.: o próprio "a cascata (origem, especificidade, ordem de fonte)" da seção 1
//     do uix-rcss.md e o próprio exemplo trabalhado de scrollbar do `docs/effects.md`, que dá
//     números ARREDONDADOS -- "~10000 por tag", "~20000" pra um composto de duas tags -- evidência
//     da FORMA do algoritmo, não das constantes exatas dele). Per o próprio briefing desta tarefa
//     ("se ela não bastar... isso é defeito da spec: pare e reporte, não improvise"), isto é
//     reportado aqui e no próprio item deste trabalho no `TODO.md` -- mas NÃO fica sem resolver,
//     porque a própria convenção de RE-estudo já estabelecida deste projeto (o "pesquisar...
//     reimplementar clean-room" do CLAUDE.md, já exercitada por todo irmão deste módulo, ex.: as
//     próprias citações de `examples/RmlUi/Source/Core/ComputeProperty.cpp` do value_compute.hpp)
//     se aplica identicamente aqui: o algoritmo EXATO do upstream, lido direto do clone de
//     RE-estudo pinado, é:
//       `examples/RmlUi/Source/Core/StyleSheetSelector.h:13-21` (`namespace SelectorSpecificity`):
//         `Tag = 10'000`, `Class = Attribute = PseudoClass = 100'000`, `ID = 1'000'000`.
//       `examples/RmlUi/Source/Core/StyleSheetNode.cpp:369-390` (`CalculateAndSetSpecificity`): a
//         própria especificidade de um compound é `(tag vazia ? 0 : Tag) + (id vazio ? 0 : ID) +
//         Class*tamanho(class_names) + Attribute*tamanho(attributes) +
//         PseudoClass*tamanho(pseudo_class_names)`, e a especificidade de uma cadeia de seletor
//         INTEIRA é a SOMA da própria especificidade de todo compound na cadeia (o upstream computa
//         isto cumulativamente construindo uma árvore de `StyleSheetNode`, "depois soma a
//         especificidade do nosso pai na nossa", `:387-389` -- somar sobre uma cadeia PLANA, como o
//         próprio `Selector::compounds` deste módulo já é, é aritmeticamente idêntico a esse
//         acúmulo-de-pai, já que especificidade é uma quantidade puramente aditiva sem peso nenhum
//         dependente-de-combinador: `Combinator::Descendant` vs. `::Child` nunca aparece em lugar
//         nenhum do próprio corpo de `CalculateAndSetSpecificity`).
//     `kSpecificityWeightAttribute` NÃO é declarado abaixo: `[attr]` é zero-medido e FORA deste
//     subconjunto (seção 6.2 do `docs/rmlx-subset.md`), então nenhum compound que os próprios
//     chamadores deste módulo conseguem construir (via `parse_compound` do parser.hpp) algum dia
//     tem lista de atributo não-vazia pra pesar -- somar uma constante pra uma forma de seletor que
//     o próprio parser deste repo não consegue produzir seria código morto fingindo ser decisão.
//     ⚠️ O QUE ESTE ARQUIVO DELIBERADAMENTE NÃO RE-DERIVA DO UPSTREAM: o próprio termo de
//     ordem-de-fonte `rule_count` (`StyleSheetParser.cpp:749`'s `rule_count++`, dobrado no MESMO
//     inteiro via o próprio `specificity + rule_specificity` do `ImportProperties`,
//     `StyleSheetNode.cpp:144`) é o mecanismo do upstream pra desempatar entre DUAS regras
//     DIFERENTES de especificidade-de-compound IGUAL por ordem de fonte -- isso é uma decisão
//     CASCATA-INTEIRA, atravessando toda regra de uma folha (e, pelo próprio acúmulo de
//     `specificity_offset` do `StyleSheetContainer`, potencialmente toda folha MESCLADA),
//     exatamente trabalho da próxima fatia ("cascata"), per a própria fronteira já declarada do
//     irmão parser.hpp deste módulo ("casamento de seletor... fatias seguintes") -- o próprio
//     `Specificity` deste arquivo é só o peso por-CADEIA-de-seletor, exatamente o campo
//     `specificity` por-`StyleSheetNode` do upstream ANTES do `ImportProperties` somar um termo
//     `rule_specificity` a ele.
//
//     SEMÂNTICA DE LISTA-VÍRGULA (`match_selector_list`, o ÚNICO lugar deste arquivo que combina
//     mais de um fato num veredito só, e por que isso ainda é trabalho DESTE item, não da cascata):
//     a seção 6.1 do `docs/rmlx-subset.md` autoriza `.a, .b { }` (15 medidos, 13 arquivos); o
//     próprio comentário de doc de `SelectorList` do parser.hpp declara a semântica que este
//     arquivo precisa honrar: "todo seletor da lista aplica o MESMO conjunto de declaração". O
//     próprio `StyleSheetParser::ParseProperties` do upstream (`StyleSheetParser.cpp:734-747`)
//     constrói UM caminho `StyleSheetNode` independente POR entrada de lista-vírgula, todos
//     compartilhando o MESMO `rule_count` (ordem de fonte -- incrementado uma vez DEPOIS do laço da
//     lista-vírgula inteira, `:749`), mas cada entrada mantém a PRÓPRIA especificidade derivada de
//     compound (um `.a` cru e um composto `#id .a` na mesma lista NÃO compartilham peso). Quando um
//     elemento casa com MAIS DE UMA entrada da mesma lista (ex.: `.a, .b { color: red }` contra um
//     elemento carregando AS DUAS classes), o próprio `PropertyDictionary::SetProperty` do upstream
//     (`PropertyDictionary.cpp:134-142`) mantém a entrada com especificidade MAIOR pra aquela
//     propriedade (`iterator->second.specificity > specificity` guarda uma sobrescrita) -- já que
//     toda entrada numa lista compartilha a mesma ordem de fonte, a especificidade de COMPOUND maior
//     é o único desempate entre elas, um fato inteiramente LOCAL sobre esta ÚNICA `SelectorList` que
//     não precisa de nenhuma OUTRA regra da folha pra resolver. `match_selector_list` portanto
//     retorna a especificidade MÁXIMA entre toda entrada que casa -- não "a primeira que casa", não
//     "a soma de todo casamento" -- provado por um caso dedicado, testado-por-mutação, da própria
//     suíte deste item (duas entradas de especificidade DIFERENTE, as duas casando o mesmo
//     elemento).
//
//     ESTADO DE PSEUDO-CLASSE (`MatchState::hover_active`): este subconjunto autoriza exatamente
//     UMA pseudo-classe, `:hover` (o próprio comentário de doc de `CompoundSelector::pseudo_hover`
//     do parser.hpp, "a ÚNICA pseudo-classe que este subconjunto autoriza"; `:focus`/`:active` são
//     zero-medidos em posição de SELETOR e fora do subconjunto, seção 2 do `docs/rmlx-subset.md`).
//     Este módulo NÃO TEM noção nenhuma de estado real de mouse/foco de uma UI viva -- não existe
//     estado nenhum assim em lugar nenhum do próprio `Element` do `dom_tree.hpp` (uma árvore de
//     posse plana e estática, pelo próprio comentário de cabeçalho daquele arquivo, "SEM eventos/
//     foco/hover"). Casar `:hover` portanto recebe um único booleano GLOBAL, forçado
//     identicamente em TODO elemento sendo casado -- a forma exata que a própria matriz de STATE da
//     seção 4 do `docs/uix-rcss.md` já fixa pro DUMP que este casador eventualmente alimenta (`none`
//     / `hover-all`, "forçado globalmente, pra todo elemento simultaneamente", nunca inferido
//     por-elemento a partir de markup estático). Este arquivo não inventa essa política; é o único
//     contrato pré-existente deste repo que já diz a um chamador o que `:hover` deveria significar
//     sem uma UI viva.
//
//     DELIBERADAMENTE NÃO É TRABALHO DESTE ARQUIVO (território de cascata / `RMLX-2`+, restatado
//     pra um chamador não precisar re-derivar a fronteira uma segunda vez a partir do próprio
//     cabeçalho do parser.hpp):
//       - Combinar especificidade entre REGRAS/`Selector`s DIFERENTES da MESMA `StyleSheet` (ordem
//         de fonte, equivalente-a-`!important` -- RCSS não tem, seção 12 do `docs/uix-rcss.md` --
//         o próprio `specificity_offset` de múltiplas folhas mescladas). `match_selector`/
//         `match_selector_list` respondem "esta ÚNICA lista-ou-seletor casa com ESTE ÚNICO
//         elemento", nada sobre qual de várias regras CASANDO numa folha inteira deveria vencer uma
//         dada propriedade -- esse ranqueamento é trabalho da próxima fatia, que consome o
//         `Specificity` DESTE arquivo como uma das entradas dela.
//       - Percorrer o vetor `rules` de uma `StyleSheet` de jeito nenhum. Este arquivo recebe um
//         `Selector`/`SelectorList` de cada vez, entregue por uma futura fatia de cascata que já
//         sabe de qual `Rule` ele veio.
//       - Resolver o próprio `KeyframeBlock::selector_text` de `@keyframes` (`"0%"`/`"from"`/`"to"`)
//         -- isso não é um seletor de DOM de jeito nenhum (o próprio comentário de doc do
//         parser.hpp, "NÃO-RESOLVIDO... a quarta semântica de `%`"), e os tipos `Selector`/
//         `SelectorList` deste arquivo nunca são o tipo que um `KeyframeBlock` carrega.
//       - Seletores de atributo (`[attr]`), universal (`*`), combinadores de irmão (`+`/`~`),
//         `:nth-child()`/`:not()` -- todos zero-medidos, fora do subconjunto (seção 6.2 do
//         `docs/rmlx-subset.md`); o parser que este arquivo consome NÃO CONSEGUE, estruturalmente,
//         produzir um `CompoundSelector` usando nenhum destes (não existe campo nenhum naquela
//         struct pra eles), então não há nada pra este arquivo rejeitar na própria fronteira -- o
//         fail-high já aconteceu uma fatia rio-acima, em tempo de parse.
//
//     COMPLEXIDADE, DECLARADA COM HONESTIDADE (a própria disciplina "profilear antes de otimizar"
//     deste projeto, não "lock-free wishful thinking" aplicado a corretude em vez de concorrência):
//     a busca de ancestral do `Combinator::Descendant` retrocede (backtrack) (ver o próprio
//     `match_from` do .cpp pra recursão exata), pior caso `O(profundidade^num_compounds)` pra uma
//     cadeia toda-descendente patológica contra uma árvore maximamente funda. Isto NÃO é apertado
//     aqui: o próprio `kMaxElementDepth` (256) do `dom_tree.hpp` já limita `profundidade`, e as
//     próprias cadeias de seletor medidas no corpus real têm 1-3 compounds de comprimento (as
//     próprias contagens de descendente/filho/composto da seção 6.2 do `docs/rmlx-subset.md`)
//     contra uma profundidade de árvore medida de 5 -- longe do teto onde o pior caso deste
//     algoritmo importaria. Uma futura fatia de cascata que descobrir um hot-path real, MEDIDO (não
//     um chute) aqui é a licenciada a memoizar/indexar isto, pelo próprio precedente "otimizar antes
//     de medir é apostar" deste projeto (o próprio ponto (4) do `dom_tree.hpp`).
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo do lexer.hpp/parser.hpp/
//     property_registry.hpp/shorthand.hpp/value_compute.hpp. Depende de `glintfx::uix::Element` (o
//     namespace plano do DOM, `glintfx/src/uix/dom/dom_tree.hpp`) por nome totalmente qualificado
//     em todo uso, deliberadamente nunca importado por `using namespace`, pra um leitor nunca ter
//     que adivinhar a qual dos dois tipos de forma-parecida-mas-escopo-diferente dos módulos irmãos
//     (`glintfx::uix::style::Selector` vs. qualquer coisa plana `glintfx::uix`) um nome nu neste
//     arquivo se refere.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>

#include "uix/dom/dom_tree.hpp"
#include "uix/style/parser.hpp"

namespace glintfx::uix::style {

// EN: Per-compound specificity weights -- see this file's header comment, "The specificity
//     algorithm", for the exact upstream citation (`StyleSheetSelector.h:13-21`). `std::int64_t`,
//     not upstream's plain `int`: this module's own SUM over a whole chain (not just one compound)
//     has no ceiling upstream's per-node `int` needed to respect (upstream never sums a WHOLE chain
//     into one variable the way `selector_specificity` below does -- it accumulates one node's
//     weight, then relies on the TREE's own parent pointers for the rest) -- a wider type costs
//     nothing and removes any theoretical overflow question for a hostile, adversarially long
//     compound chain before it can ever be asked.
// PT: Pesos de especificidade por-compound -- ver o parágrafo "O algoritmo de especificidade" do
//     comentário de cabeçalho deste arquivo pra citação exata do upstream
//     (`StyleSheetSelector.h:13-21`). `std::int64_t`, não o `int` puro do upstream: a própria SOMA
//     deste módulo sobre uma cadeia INTEIRA (não só um compound) não tem o teto que o `int`
//     por-nó do upstream precisava respeitar (o upstream nunca soma uma cadeia INTEIRA numa
//     variável só do jeito que o `selector_specificity` abaixo faz -- ele acumula o peso de um nó,
//     depois conta com os próprios ponteiros-de-pai da ÁRVORE pro resto) -- um tipo mais largo não
//     custa nada e remove qualquer questão teórica de overflow pra uma cadeia de compound hostil,
//     adversarialmente longa, antes mesmo dela poder ser perguntada.
using Specificity = std::int64_t;

inline constexpr Specificity kSpecificityWeightTag = 10'000;
inline constexpr Specificity kSpecificityWeightClassOrPseudo = 100'000;
inline constexpr Specificity kSpecificityWeightId = 1'000'000;

// EN: The one boolean this subset's ONE pseudo-class needs -- see this file's header comment,
//     "Pseudo-class state", for why this is global (not per-`Element`) and why `:focus`/`:active`
//     have no field here (out of subset).
// PT: O único booleano que a ÚNICA pseudo-classe deste subconjunto precisa -- ver o parágrafo
//     "Estado de pseudo-classe" do comentário de cabeçalho deste arquivo pra por que isto é global
//     (não por-`Element`) e por que `:focus`/`:active` não têm campo nenhum aqui (fora do
//     subconjunto).
struct MatchState {
  bool hover_active = false;
};

// EN: `matched == false` always carries `specificity == 0` (never a partial/stale weight from a
//     failed attempt) -- a caller can read `specificity` unconditionally without checking `matched`
//     first and never observe a non-zero value for a non-match. `matched == true` always carries
//     `specificity >= kSpecificityWeightTag` (10'000): every `CompoundSelector` this module's own
//     parser can construct has AT LEAST one of tag/id/class/hover present (parser.hpp's own
//     `CompoundSelector` doc-comment, "parse_compound... exige que ao menos UM... esteja presente"),
//     so a matched chain's specificity is never zero by construction, not by this file's own
//     defensive coding.
// PT: `matched == false` sempre carrega `specificity == 0` (nunca um peso parcial/obsoleto de uma
//     tentativa falha) -- um chamador pode ler `specificity` incondicionalmente sem checar
//     `matched` antes e nunca observar um valor não-zero pra um não-casamento. `matched == true`
//     sempre carrega `specificity >= kSpecificityWeightTag` (10'000): todo `CompoundSelector` que o
//     próprio parser deste módulo consegue construir tem AO MENOS um de tag/id/classe/hover
//     presente (o próprio comentário de doc de `CompoundSelector` do parser.hpp, "parse_compound...
//     exige que ao menos UM... esteja presente"), então a especificidade de uma cadeia casada nunca
//     é zero por construção, não por codificação defensiva própria deste arquivo.
struct MatchResult {
  bool matched = false;
  Specificity specificity = 0;
};

// EN: The specificity of ONE `CompoundSelector`, in isolation -- see this file's header comment for
//     the exact formula and its upstream citation. Exposed publicly (not `.cpp`-local) because a
//     future cascade slice, or a test proving a specific weight boundary, has a legitimate reason to
//     ask this question about a single compound without building a whole `Selector` chain around it.
// PT: A especificidade de UM `CompoundSelector`, isoladamente -- ver o comentário de cabeçalho
//     deste arquivo pra fórmula exata e a citação de upstream dela. Exposta publicamente (não
//     local-do-.cpp) porque uma futura fatia de cascata, ou um teste provando um limite de peso
//     específico, tem motivo legítimo pra fazer esta pergunta sobre um compound isolado sem
//     construir uma cadeia `Selector` inteira em volta dele.
Specificity compound_specificity(const CompoundSelector& compound);

// EN: The specificity of a WHOLE selector chain -- the sum of `compound_specificity` over every
//     `CompoundSelector` in `selector.compounds`, per this file's own header comment ("summing over
//     a flat chain... is arithmetically identical to upstream's cumulative parent-add"). Does NOT
//     depend on whether `selector` actually matches anything -- this is a pure syntactic weight,
//     computable from the `Selector` alone, exactly like upstream's own `CalculateAndSetSpecificity`
//     runs once at PARSE time, before any element is ever matched against it.
// PT: A especificidade de uma cadeia de seletor INTEIRA -- a soma de `compound_specificity` sobre
//     todo `CompoundSelector` em `selector.compounds`, pelo próprio comentário de cabeçalho deste
//     arquivo ("somar sobre uma cadeia plana... é aritmeticamente idêntico ao acúmulo-de-pai
//     cumulativo do upstream"). NÃO depende de `selector` de fato casar com algo -- isto é um peso
//     puramente sintático, computável só a partir do `Selector`, exatamente como o próprio
//     `CalculateAndSetSpecificity` do upstream roda uma vez em tempo de PARSE, antes de qualquer
//     elemento algum dia ser casado contra ele.
Specificity selector_specificity(const Selector& selector);

// EN: Does `selector` (ONE chain -- not a comma-list, see `match_selector_list` below for that)
//     match `element`? `element`'s ANCESTOR CHAIN (`glintfx::uix::Element::parent()`) is walked for
//     `Combinator::Descendant`/`::Child` -- this function never synthesizes an ancestor `element`
//     does not structurally have; a root element (`parent() == nullptr`) can only ever satisfy a
//     single-compound `selector` (`selector.compounds.size() == 1`). See this file's header comment,
//     "Complexity, stated honestly", for the worst-case shape of the ancestor search.
//
//     TRUST BOUNDARY: `selector.compounds.size() >= 1` and `combinators.size() ==
//     compounds.size() - 1` are construction invariants `parser.hpp`'s own `Selector` doc-comment
//     already guarantees for every `Selector` this repo's own `parse_stylesheet` can produce --
//     this function still handles an (unreachable-by-construction) empty `compounds` defensively,
//     returning a clean not-matched `MatchResult{}`, rather than trusting that invariant blindly
//     into an out-of-bounds access; belt-and-suspenders against a FUTURE caller that hand-builds a
//     `Selector` outside `parse_stylesheet` (this module's own test suite does exactly that, per
//     this task's own brief), not a defence against hostile external input (a `Selector` never
//     comes from outside this repo's own source tree).
// PT: `selector` (UMA cadeia -- não uma lista-vírgula, ver `match_selector_list` abaixo pra isso)
//     casa com `element`? A CADEIA DE ANCESTRAL de `element` (`glintfx::uix::Element::parent()`) é
//     percorrida pra `Combinator::Descendant`/`::Child` -- esta função nunca sintetiza um ancestral
//     que `element` não tem estruturalmente; um elemento raiz (`parent() == nullptr`) só consegue
//     satisfazer um `selector` de um-compound-só (`selector.compounds.size() == 1`). Ver o
//     parágrafo "Complexidade, declarada com honestidade" do comentário de cabeçalho deste arquivo
//     pra forma de pior-caso da busca de ancestral.
//
//     FRONTEIRA DE CONFIANÇA: `selector.compounds.size() >= 1` e `combinators.size() ==
//     compounds.size() - 1` são invariantes de construção que o próprio comentário de doc de
//     `Selector` do parser.hpp já garante pra todo `Selector` que o próprio `parse_stylesheet` deste
//     repo consegue produzir -- esta função mesmo assim trata um `compounds` vazio
//     (inalcançável-por-construção) defensivamente, retornando um `MatchResult{}` limpo de
//     não-casou, em vez de confiar naquele invariante cegamente até um acesso fora-de-limite;
//     cinto-e-suspensório contra um FUTURO chamador que constrói um `Selector` à mão fora do
//     `parse_stylesheet` (a própria suíte de teste deste módulo faz exatamente isso, per o próprio
//     briefing desta tarefa), não uma defesa contra input externo hostil (um `Selector` nunca vem
//     de fora da própria árvore-fonte deste repo).
MatchResult match_selector(const Selector& selector, const glintfx::uix::Element& element,
                           const MatchState& state);

// EN: Does ANY entry inside `list` match `element`? See this file's header comment, "Comma-list
//     semantics", for why the answer, on a match, is the MAXIMUM specificity among every matching
//     entry -- not the first, not a sum. `list.empty()` (unreachable by construction, `Rule.selectors
//     .size() >= 1` per parser.hpp's own `Rule` doc-comment) returns a clean not-matched
//     `MatchResult{}`, same defensive-not-hostile reasoning as `match_selector`'s own trust-boundary
//     paragraph.
// PT: ALGUMA entrada dentro de `list` casa com `element`? Ver o parágrafo "Semântica de
//     lista-vírgula" do comentário de cabeçalho deste arquivo pra por que a resposta, num
//     casamento, é a especificidade MÁXIMA entre toda entrada que casa -- não a primeira, não uma
//     soma. `list.empty()` (inalcançável por construção, `Rule.selectors.size() >= 1` pelo próprio
//     comentário de doc de `Rule` do parser.hpp) retorna um `MatchResult{}` limpo de não-casou,
//     mesmo raciocínio defensivo-não-hostil do próprio parágrafo de fronteira-de-confiança do
//     `match_selector`.
MatchResult match_selector_list(const SelectorList& list, const glintfx::uix::Element& element,
                                const MatchState& state);

} // namespace glintfx::uix::style
