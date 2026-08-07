# ADR-0022 -- Full parity with the engine being replaced, not the measured minimum / Paridade total com o motor substituído, não o mínimo medido

- **Status:** Accepted (2026-08-07)
- **Deciders:** petrus (líder) -- verbatim order below
- **Tags:** scope, anti-scope-creep, architecture, process, one-way-door-adjacent
- **Builds on:** [[0020-rml-anticorruption-layer]] (the physical + documentary anticorruption layer this ADR does not revoke -- it widens the boundary that layer enforces, it does not remove the layer). [[0009-internalization-boundary]] and [[0011-soft-font-flip]] (the same house doctrine this ADR restates for a new axis: internalize incrementally, behind gates, without inventing a *general* solution the líder never ordered).
- **Cross-ref:** `docs/rmlx-subset.md` (the frozen subset spec this ADR widens the default of, §6.2/§6.3 amended in the sibling fatia of this same wave), `docs/uix-rcss.md` §13 (amended in the sibling fatia), `docs/uix-rcss-censo.md` (banner added by this fatia), `docs/uix-rcss-ambiguidades.md:555` (note added by this fatia), `TODO.md` items `ESC-0` through `ESC-29` (wave `WR2R`), `feedback_gusworld_nao_define_prioridade`, `feedback_escopo_distribuicao_geral`, `feedback_dependencia_externa_e_para_substituir`.

## Context / Contexto

**EN:** Between `RMLX-0` (2026-08-04) and this ADR (2026-08-07), the `RMLX-1..11` RmlUi-elimination
program's own governing document adopted a scoping rule that was never ordered: **cut what the
corpus does not measure**, on the reasoning that a property, selector form, colour name, or unit
absent from glintfx's own scenes and from GusWorld's corpus is safe to drop. `docs/rmlx-subset.md`
records this in its own words -- section 2: *"`nth-child`, `:not(`, and `z-index` all measure zero.
Cutting them is a **real-zero loss**, not an 'acceptable' one -- no wave needs to plan a fallback
for them"* (line 78); section 6.2's table rejects the universal selector `*`, all 7 attribute-selector
operators, and both sibling combinators the same way, each row reading *"no -- fail-high"* under a
column literally named "Authorized" (lines 168-171). `docs/uix-rcss.md` §13 encodes the identical
doctrine under the phrase *"real zero is a real cut."* This is not a rounding error or an isolated
oversight -- it is a **doctrine, written down, cited by its own name, applied consistently across
two governing documents**, which is exactly why it required a líder-level correction rather than a
quiet edit.

**The líder's verbatim order that ends this doctrine, 2026-08-07:**

> "ESTE PROJETO É PARA DISTRIBUICAO. GUSWORLD É UM consumidor apenas! NAO ESTREITE O ESCOPO POR
> GUSWORLD, NEM DECIDA NADA PELO QUE ELE PRECISA OU NÃO PRECISA!"

**The measured cost of the doctrine, verified against the pinned upstream source
(`glintfx/build-preci-glfw-off/_deps/rmlui-src`, the actual fetched dependency, not the
`examples/RmlUi` study clone), not re-typed from an earlier estimate:**

| Surface | Ours | Upstream (RmlUi 6.3, pinned) | Delta | Evidence |
| :--- | ---: | ---: | ---: | :--- |
| Properties | 72 | 99 (`StyleSheetSpecification.cpp:248-438`, `RegisterDefaultProperties`, 99×`RegisterProperty(`) + 8 glintfx-native (ripple×5, image-tint×3) = 107 | 35 | counted directly against the pinned function body |
| Shorthands | 13 | 20 (`RegisterDefaultProperties`, 20×`RegisterShorthand(`) | 7 | same function, same count pass |
| Named colours | 3 (white, black, transparent) | 19 (`PropertyParserColour.cpp:117-135`, `html_colours` map, counted entry by entry) | 16 | direct read of the map literal |
| Colour functional forms | -- | 8: `rgb`, `rgba`, `hsl`, `hsla`, `lab`, `lch`, `oklab`, `oklch` (`PropertyParserColour.cpp:177-193` -- `rgb`/`hsl` prefix-match also accepts the `-a` suffix form, one parser each) | -- | traced the dispatch `if/else if` chain |
| Transform functions | 3 | 21 (`PropertyParserTransform.cpp:51-136`, every `TransformFunctionName`/token counted, no duplicates) | 18 | direct enumeration |
| Units | ~6 (px, dp, em only in the font-size chain, deg, rad, %, unitless) | 16, including the empty-string unitless entry (`PropertyParserNumber.cpp:7-24`, `unit_string_map`) | 10 | direct read of the map literal |
| Nativa decorators | 5 (glintfx's own) | 16 (`Factory.cpp:196-213`, every `RegisterDecoratorInstancer(` call, gradient aliases counted once per registered name since each is independently reachable from RCSS) | -- | direct enumeration |
| Nativa filters | 0 | 10 (`Factory.cpp:216-226`) | -- | direct enumeration |
| Nativa font effects | 0 | 4 (`Factory.cpp:229-232`) | -- | direct enumeration |
| Selector forms | 9 authorized | universal `*`, 7 attribute operators, 2 sibling combinators, 13 structural forms all rejected "fail-high" by `docs/rmlx-subset.md` §6.2's own table | -- | `docs/rmlx-subset.md:168-171` (cited, not re-measured here -- owned by the sibling fatia) |
| Pseudo-class model | 1 boolean (`:hover` only) | `SetPseudoClass(name, bool)` -- public API, arbitrary named pseudo-classes | -- | `ESC-29`, open implementability question, not a scope cut |

🔴 **The fact that best carries this decision is a self-contradiction inside `docs/rmlx-subset.md`
itself, not an external critique of it.** Its own §6.2 table rejects four selector forms by the
column "Authorized: no -- fail-high", each justified by an instance count of zero. **The very next
paragraph, in the same section, reads:** *"Zero in this corpus is not an argument to ban. glintfx's
target is broad distribution, not GusWorld-plus-glintfx's-own-test-corpus -- 'zero in these two
repositories' is a statement about two repositories, never about the world"* (`docs/rmlx-subset.md`
line 175, unchanged by this ADR -- the sibling fatia amends this same document under the same
governing sentence). **The document states the correct principle and applies its opposite four rows
above it, in the same breath.** That is not a drafting slip to patch line by line; it is what
happens when a scoping method (measure the corpus, cut what it does not show) is applied
consistently and honestly enough to eventually collide with its own stated caveat. The fix has to be
the rule the caveat was already gesturing at, generalised -- not another exception bolted onto the
same method.

**This is not the first time this project traded the líder's actual target for a consumer-shaped
proxy.** `feedback_gusworld_nao_define_prioridade` (2026-07-29) and
`feedback_escopo_distribuicao_geral` name the same failure mode at the level of wave *sequencing*
and *quality bar*; `feedback_dependencia_externa_e_para_substituir` (2026-08-04) names it at the
level of *whether to eliminate a dependency at all* (offering "bump the pin or keep the patch" as if
those were the two live options, when the project's entire reason to exist is to stop depending on
the pin). This ADR is the same correction applied to a third axis: **what surface the replacement
engine is allowed to cover**. The precedent already existed in miniature: the líder's 2026-08-06
units decision (`docs/rmlx-subset.md` §6.3) chose "full clean-room parity with every unit RmlUi
itself accepts, not the 8-unit measured floor" -- but that decision was never generalised past
units. Properties, colours, transforms, decorators, filters, font effects, and every selector form
kept the old rule. This ADR is the generalisation that was owed after §6.3, three days late.

**PT:** Entre a `RMLX-0` (2026-08-04) e este ADR (2026-08-07), o próprio documento que governa o
programa de eliminação do RmlUi (`RMLX-1..11`) adotou uma regra de escopo que nunca foi ordenada:
**cortar o que o corpus não mede**, com o raciocínio de que uma propriedade, forma de seletor, nome
de cor ou unidade ausente das cenas da própria glintfx e do corpus do GusWorld é seguro de
descartar. O `docs/rmlx-subset.md` registra isso com as próprias palavras -- seção 2: *"`nth-child`,
`:not(` e `z-index` dão zero os três. Cortá-los é perda real zero, não 'aceitável' -- nenhuma onda
precisa planejar fallback pra eles"* (linha 253); a tabela da seção 6.2 rejeita o seletor universal
`*`, os 7 operadores de seletor de atributo e os dois combinadores de irmão da mesma forma, cada
linha lendo "não -- fail-high" sob uma coluna literalmente chamada "Autorizado" (linhas 343-346).
O `docs/uix-rcss.md` §13 codifica a mesma doutrina sob a frase "zero real é corte real". Isto não é
um erro de arredondamento nem um descuido isolado -- é uma **doutrina, escrita, citada pelo próprio
nome, aplicada de forma consistente em dois documentos governantes**, e é exatamente por isso que
exigiu correção em nível de líder, não uma edição silenciosa.

**A ordem verbatim do líder que encerra essa doutrina, 2026-08-07:**

> "ESTE PROJETO É PARA DISTRIBUICAO. GUSWORLD É UM consumidor apenas! NAO ESTREITE O ESCOPO POR
> GUSWORLD, NEM DECIDA NADA PELO QUE ELE PRECISA OU NÃO PRECISA!"

**O custo medido da doutrina, verificado contra o fonte upstream PINADO
(`glintfx/build-preci-glfw-off/_deps/rmlui-src`, a dependência de fato buscada, não o clone de
estudo `examples/RmlUi`), não retranscrito de uma estimativa anterior:** ver a tabela em inglês
acima -- os mesmos números, a mesma evidência `arquivo:linha`, medidos uma única vez.

🔴 **O fato que melhor sustenta esta decisão é uma autocontradição dentro do próprio
`docs/rmlx-subset.md`, não uma crítica externa a ele.** A própria tabela da §6.2 rejeita quatro
formas de seletor pela coluna "Autorizado: não -- fail-high", cada uma justificada por uma contagem
de instância zero. **O parágrafo imediatamente seguinte, na mesma seção, diz:** *"Zero neste corpus
não é argumento para banir. O alvo da glintfx é distribuição ampla, não GusWorld-mais-o-próprio-
corpus-de-teste-da-glintfx -- 'zero nestes dois repositórios' é uma afirmação sobre dois
repositórios, jamais sobre o mundo"* (`docs/rmlx-subset.md` linha 175, inalterada por este ADR -- a
fatia irmã emenda este mesmo documento sob esta mesma frase governante). **O documento enuncia o
princípio certo e aplica o oposto quatro linhas acima, no mesmo fôlego.** Isso não é um deslize de
redação para remendar linha a linha; é o que acontece quando um método de escopo (medir o corpus,
cortar o que ele não mostra) é aplicado de forma consistente e honesta o bastante para eventualmente
colidir com sua própria ressalva. O conserto tem que ser a regra que a ressalva já apontava,
generalizada -- não mais uma exceção colada ao mesmo método.

**Esta não é a primeira vez que este projeto trocou o alvo real do líder por um proxy modelado pelo
consumidor.** `feedback_gusworld_nao_define_prioridade` (2026-07-29) e
`feedback_escopo_distribuicao_geral` nomeiam o mesmo modo de falha no nível de *sequenciamento* de
onda e *barra de qualidade*; `feedback_dependencia_externa_e_para_substituir` (2026-08-04) nomeia no
nível de *se eliminar uma dependência sequer* (oferecendo "bumpar o pin ou manter o patch" como se
fossem as duas opções vivas, quando a razão de existir do projeto inteiro é parar de depender do
pin). Este ADR é a mesma correção aplicada a um terceiro eixo: **que superfície o motor de
substituição tem permissão de cobrir**. O precedente já existia em miniatura: a decisão do líder de
2026-08-06 sobre unidades (`docs/rmlx-subset.md` §6.3) escolheu "paridade clean-room completa com
toda unidade que o próprio RmlUi aceita, não o piso de 8 unidades medido" -- mas essa decisão nunca
foi generalizada além de unidades. Propriedades, cores, transformações, decoradores, filtros,
efeitos de fonte e toda forma de seletor mantiveram a regra antiga. Este ADR é a generalização que
já era devida depois da §6.3, três dias atrasada.

## Decision / Decisão

**EN:** **If the engine being replaced accepts it, ours accepts it -- parity, not the measured
minimum.** Usage counts in any repository (ours, a consumer's, or both) are **sequencing/risk data
only** ("what is used breaks early if we get it wrong"), and are **never** a scope boundary. "Zero
in these repositories" is a statement about these repositories, never about the world; glintfx
targets broad distribution.

This generalises the líder's 2026-08-06 units decision (`docs/rmlx-subset.md` §6.3, "full parity
with what the old engine accepts, not the measured minimum") from one axis (units) to every axis
`RMLX-1..11` touches: property registration, shorthand expansion, named colours and colour
functional forms, transform functions, decorator/filter/font-effect parsing, and selector forms
including the universal selector, attribute operators, sibling combinators, and the structural
pseudo-class set. **Two boundaries remain, unmoved by this ADR:**

1. **The parse/compute/serialize vs. render/apply boundary is unchanged.** Accepting a transform
   function, decorator, or filter at the syntax level is a `RMLX-2`/`ESC-*` concern; wiring it to
   the actual render matrix or GL pass stays with `RMLX-7`/`RMLX-8`, exactly as it already did for
   the 3 transform functions and 5 native decorators glintfx already implements. Parity of
   acceptance does not collapse the layering `docs/rmlx-subset.md` and `docs/uix-rcss.md` already
   established between "the parser understands this" and "the renderer does something with it."
2. **The fail-high policy is unchanged, only its default flips.** `docs/rmlx-subset.md`'s own
   words, restated exactly, now govern what "unrecognized" means going forward: an unrecognized
   *construct* -- one the old engine itself would also reject -- still fails the whole rule to
   register, logs the raw text and `file:line`, never partially matches, never silently breaks the
   stylesheet. What changes is which constructs are "recognized" by default: **before this ADR,
   default = what the corpus measures; after this ADR, default = what the pinned upstream engine
   accepts.** Something neither engine accepts (`ms` as a time unit, `nth-child` was never actually
   in this category -- it is measured zero *and* upstream-supported, so it moves to the accepted
   side) stays out on both counts.

**What this does not authorize:** inventing surface the old engine itself does not have. `RMLX-1..11`
does not become "build general CSS" -- it becomes "build what RmlUi 6.3 itself ships," which is a
closed, enumerable, already-measured set (the table above), not an open-ended one. The anticorruption
layer's mechanism ([[0020-rml-anticorruption-layer]]) is unchanged: `docs/rmlx-subset.md` remains
the single frozen source of truth a wave must consult before implementing, `tools/check_rml_whitelist.sh`
remains the physical gate, and the header clause's process (stop, edit the spec with a diff, get the
líder's sign-off, then implement) remains binding -- this ADR changes what the spec's own default
scope-line is drawn against, not who draws it or how a wave requests an exception.

**PT:** **Se o motor que está sendo substituído aceita, o nosso aceita -- paridade, não o mínimo
medido.** Contagem de uso em qualquer repositório (nosso, de um consumidor, ou os dois) é **apenas
dado de sequenciamento/risco** ("o que é usado quebra cedo se errarmos"), e **nunca** fronteira de
escopo. "Zero nestes repositórios" é afirmação sobre estes repositórios, jamais sobre o mundo; o
alvo da glintfx é distribuição ampla.

Isto generaliza a decisão do líder de 2026-08-06 sobre unidades (`docs/rmlx-subset.md` §6.3,
"paridade total com o que o motor antigo aceita, não o mínimo medido") de um eixo (unidades) para
todo eixo que a `RMLX-1..11` toca: registro de propriedade, expansão de atalho, cores nomeadas e
formas funcionais de cor, funções de transformação, parse de decorador/filtro/efeito-de-fonte, e
formas de seletor incluindo o seletor universal, operadores de atributo, combinadores de irmão e o
conjunto de pseudo-classes estruturais. **Duas fronteiras permanecem, intocadas por este ADR:**

1. **A fronteira parse/computo/serialização versus render/aplicação continua igual.** Aceitar uma
   função de transformação, decorador ou filtro no nível de sintaxe é preocupação da
   `RMLX-2`/`ESC-*`; ligar isso à matriz de render de fato ou ao passe GL continua sendo da
   `RMLX-7`/`RMLX-8`, exatamente como já era para as 3 funções de transformação e os 5 decoradores
   nativos que a glintfx já implementa. Paridade de aceitação não colapsa o camadeamento que o
   `docs/rmlx-subset.md` e o `docs/uix-rcss.md` já estabeleceram entre "o parser entende isto" e "o
   renderer faz algo com isto".
2. **A política fail-high continua igual, só o default vira.** As próprias palavras do
   `docs/rmlx-subset.md`, restatadas exatamente, agora governam o que "não-reconhecido" significa
   daqui pra frente: um *construto* não-reconhecido -- um que o próprio motor antigo também
   rejeitaria -- ainda reprova a regra inteira a registrar, loga o texto bruto e `arquivo:linha`,
   nunca casa parcialmente, nunca quebra a folha em silêncio. O que muda é qual construto é
   "reconhecido" por padrão: **antes deste ADR, default = o que o corpus mede; depois deste ADR,
   default = o que o motor upstream pinado aceita.** Algo que nenhum dos dois motores aceita (`ms`
   como unidade de tempo; `nth-child` nunca esteve de fato nesta categoria -- mede zero *e* é
   suportado upstream, então migra pro lado aceito) fica de fora nos dois critérios.

**O que isto NÃO autoriza:** inventar superfície que o motor antigo em si não tem. A `RMLX-1..11`
não vira "construir CSS geral" -- vira "construir o que o próprio RmlUi 6.3 entrega", que é um
conjunto fechado, enumerável, já medido (a tabela acima), não um conjunto aberto. O mecanismo da
camada anticorrupção ([[0020-rml-anticorruption-layer]]) continua igual: o `docs/rmlx-subset.md`
segue sendo a fonte única de verdade congelada que uma onda precisa consultar antes de implementar,
o `tools/check_rml_whitelist.sh` segue sendo o gate físico, e o processo da cláusula do cabeçalho
(parar, editar a spec com um diff, pegar o aval do líder, só então implementar) segue vinculante --
este ADR muda contra o que a própria linha-padrão de escopo da spec é traçada, não quem a traça nem
como uma onda pede exceção.

## Consequences / Consequências

**EN:** **Cost, stated plainly.** `TODO.md`'s `WR2R` wave (`ESC-1` through `ESC-29`) now scopes
~35 properties, 7 shorthands, 16 colours, 8 colour functional forms, 18 transform functions, 10
units, 16 native decorators, 10 native filters, 4 native font effects, the universal selector, 7
attribute operators, 2 sibling combinators, and 13 structural selector forms back **in** --
estimated at roughly 5-7k additional LOC across the fatias this ADR's sibling `ESC-*` items enumerate
(`TODO.md` carries the per-fatia breakdown). `RMLX-3` (layout) specifically grows from an estimated
~8-10k LOC to ~10-13k LOC, because `float`/`clear` and `display: table-*` -- previously cut on the
same real-zero-corpus reasoning this ADR retires -- return to scope (`ESC-28`, the layout-wave
registry amendment, records the sequencing: table layout lands post-checkpoint, off the go/no-go
critical path).

**Benefit, stated as the actual point of the exercise.** A stylesheet authored for RmlUi 6.3 --
one glintfx has never seen, from a consumer that does not yet exist -- now has a real chance of
rendering correctly against glintfx's own engine without silent property/selector/unit loss. That
**is** what "replacing an engine" means; a replacement that only accepts what one known consumer's
markup happens to exercise is not a replacement, it is that consumer's private fork wearing a
general-purpose name.

**What does not change.** The fail-high policy (unrecognized construct fails the rule, logs
`file:line`, never partial-matches -- unchanged, see Decision above). The anticorruption layer's
physical gate and documentary-spec mechanism (unchanged, [[0020-rml-anticorruption-layer]] is
amended, not superseded). The parse-vs-render layering (unchanged, `RMLX-7`/`RMLX-8` still own
wiring accepted syntax to an actual render effect). The `Rml::Variant`/opaque-pointer debt register
(unchanged, still retires at `RMLX-11`). This ADR is a **scope-default change**, not an architecture
change -- no file this ADR touches introduces a new module, dependency, or mechanism; it edits four
documents' stated defaults and the reasoning behind them.

**A residual open question this ADR does not resolve, left for the sibling fatia and `ESC-29`:**
whether `SetPseudoClass`'s arbitrary-name model belongs on the public `App`/`UiLayer` facade now or
waits for `RMLX-5` (where live element state is born) is an **implementability** question, not a
scope one -- this ADR settles that the surface is in-scope; it does not settle which wave exposes it
or through which API.

**PT:** **Custo, dito sem suavizar.** A onda `WR2R` do `TODO.md` (`ESC-1` até `ESC-29`) agora
escopa de volta **para dentro**: ~35 propriedades, 7 atalhos, 16 cores, 8 formas funcionais de cor,
18 funções de transformação, 10 unidades, 16 decoradores nativos, 10 filtros nativos, 4 efeitos de
fonte nativos, o seletor universal, 7 operadores de atributo, 2 combinadores de irmão e 13 formas de
seletor estrutural -- estimado em aproximadamente 5-7k LOC adicionais ao longo das fatias que os
itens `ESC-*` irmãos deste ADR enumeram (o `TODO.md` carrega o detalhamento por-fatia). A `RMLX-3`
(layout) especificamente cresce de uma estimativa de ~8-10k LOC para ~10-13k LOC, porque
`float`/`clear` e `display: table-*` -- antes cortados pelo mesmo raciocínio de zero-real-no-corpus
que este ADR aposenta -- voltam ao escopo (`ESC-28`, a emenda do registro da onda de layout, registra
o sequenciamento: layout de tabela pousa pós-checkpoint, fora do caminho crítico de go/no-go).

**Benefício, dito como o real ponto do exercício.** Uma folha de estilo escrita para o RmlUi 6.3 --
uma que a glintfx nunca viu, de um consumidor que ainda não existe -- agora tem chance real de
renderizar corretamente contra o motor próprio da glintfx sem perda silenciosa de
propriedade/seletor/unidade. **É isso** que "substituir um motor" significa; uma substituição que só
aceita o que o markup de um único consumidor conhecido exercita não é substituição, é o fork privado
daquele consumidor usando um nome de propósito geral.

**O que não muda.** A política fail-high (construto não-reconhecido reprova a regra, loga
`arquivo:linha`, nunca casa parcialmente -- inalterada, ver Decisão acima). O gate físico e o
mecanismo de spec documental da camada anticorrupção (inalterados, o [[0020-rml-anticorruption-layer]]
é emendado, não sucedido). O camadeamento parse-versus-render (inalterado, a `RMLX-7`/`RMLX-8`
continuam donas de ligar sintaxe aceita a um efeito de render de fato). O registro de dívida de
`Rml::Variant`/ponteiro opaco (inalterado, ainda se aposenta na `RMLX-11`). Este ADR é uma **mudança
de default de escopo**, não uma mudança de arquitetura -- nenhum arquivo que este ADR toca introduz
módulo, dependência ou mecanismo novo; ele edita o default declarado de quatro documentos e o
raciocínio por trás.

**Uma pergunta residual em aberto que este ADR não resolve, deixada para a fatia irmã e a `ESC-29`:**
se o modelo de nome-arbitrário do `SetPseudoClass` pertence à fachada pública `App`/`UiLayer` agora
ou espera a `RMLX-5` (onde o estado vivo de elemento nasce) é uma pergunta de
**implementabilidade**, não de escopo -- este ADR resolve que a superfície está dentro do escopo;
não resolve qual onda a expõe nem por qual API.

## Options considered / Opções consideradas

1. **Keep the measured-subset method, patch each individual contradiction as found.** Pros: least
   diff, no wave re-planning. Cons: this is the status quo this ADR exists to end -- a framework
   whose scope is defined by what one consumer's markup happens to exercise is not a framework, it
   is that consumer's private layer wearing a general-purpose name; the method would keep producing
   the same class of self-contradiction (§6.2's own caveat vs. its own table) on every future axis,
   because the caveat was always the correct principle and the table was always the mistake.
   Rejected -- this is the failure mode the líder's order names directly.
2. **Parity for properties/colours/transforms/units only, keep the fail-high corpus-measured cut
   for selector forms (the highest-implementation-cost axis: universal selector's zero-specificity
   rule, 7 attribute operators, sibling combinators touch the matcher's traversal).** Pros: caps the
   `RMLX-3`/`RMLX-4` cost increase. Cons: is exactly the kind of partial application the context
   section shows already failed once (units got parity in isolation on 2026-08-06, everything else
   didn't, and the gap sat unresolved for three days until this ADR); selector forms are precisely
   where "zero in this corpus" is least informative about "zero in the world" (a `[data-state]`
   attribute selector or a `.card:not(.disabled)` pattern is common CSS, not an exotic one).
   Rejected: it reproduces the doctrine's own failure at smaller scale.
3. **Full parity across every axis, generalizing the líder's 2026-08-06 units decision (chosen).**
   Pros: one rule, stated once, that resolves every current and future instance of this question
   without a new ADR per axis; matches the project's own stated goal (`docs/rmlx-subset.md`'s own
   caveat, `ADR-0015`'s framework2d direction, `ADR-0019`'s Apache-2.0 rotation -- all three already
   on record as targeting broad, general-purpose adoption, not the one known consumer). Cons: the
   real cost named in Consequences (~5-7k LOC, `RMLX-3` growing by ~2-3k LOC) is real and not
   waved away -- it is the correct trade for what the líder actually ordered, not a free lunch.

**PT:**

1. **Manter o método do subconjunto medido, remendar cada contradição individual conforme achada.**
   Prós: menor diff, sem replanejar onda. Contras: é o status quo que este ADR existe para encerrar
   -- um framework cujo escopo é definido pelo que o markup de um consumidor exercita não é
   framework, é a camada privada daquele consumidor usando nome de propósito geral; o método
   continuaria produzindo a mesma classe de autocontradição (a ressalva da §6.2 contra a própria
   tabela) em todo eixo futuro, porque a ressalva sempre foi o princípio certo e a tabela sempre foi
   o erro. Rejeitada -- é o modo de falha que a ordem do líder nomeia diretamente.
2. **Paridade só para propriedades/cores/transformações/unidades, manter o corte fail-high medido
   por corpus para formas de seletor (o eixo de maior custo de implementação: a regra de
   especificidade-zero do seletor universal, 7 operadores de atributo, combinadores de irmão mexem
   na travessia do casador).** Prós: limita o crescimento de custo da `RMLX-3`/`RMLX-4`. Contras: é
   exatamente o tipo de aplicação parcial que a seção de contexto já mostra ter falhado uma vez
   (unidades ganharam paridade isolada em 2026-08-06, o resto não, e a lacuna ficou sem resolver por
   três dias até este ADR); formas de seletor são precisamente onde "zero neste corpus" é menos
   informativo sobre "zero no mundo" (um seletor de atributo `[data-state]` ou um padrão
   `.card:not(.disabled)` é CSS comum, não exótico). Rejeitada: reproduz a falha da própria doutrina
   em escala menor.
3. **Paridade total em todo eixo, generalizando a decisão do líder de 2026-08-06 sobre unidades
   (escolhida).** Prós: uma regra, dita uma vez, que resolve toda instância atual e futura desta
   pergunta sem um ADR novo por eixo; casa com o objetivo declarado do próprio projeto (a própria
   ressalva do `docs/rmlx-subset.md`, a direção framework2d do ADR-0015, a rotação Apache-2.0 do
   ADR-0019 -- os três já registrados mirando adoção ampla e de propósito geral, não o único
   consumidor conhecido). Contras: o custo real nomeado em Consequências (~5-7k LOC, a `RMLX-3`
   crescendo ~2-3k LOC) é real e não é varrido pra baixo do tapete -- é a troca correta pelo que o
   líder de fato ordenou, não almoço grátis.

## Reversibility / Reversibilidade

**EN:** Two-way door for the rule itself -- a future ADR can narrow scope again with the same
process this one used (líder-level decision, not a quiet edit), and nothing this ADR changes is
irreversible at the document level (`docs/rmlx-subset.md` and `docs/uix-rcss.md` remain living
documents per [[0020-rml-anticorruption-layer]]'s own terms). **Leaning one-way in practice**,
though, for the same reason [[0020-rml-anticorruption-layer]] itself is tagged
`one-way-door-adjacent`: once `ESC-1..29`'s implementation work lands inside `RMLX-2..8`, walking
back to the narrower subset discards real, non-trivial sunk implementation (the same asymmetry named
for the 12-wave elimination commitment itself). This ADR does not reopen or re-litigate `RMLX-3`'s
sequencing, `RMLX-11`'s excision timeline, or the anticorruption layer's physical gate -- it widens
one input to a mechanism [[0020-rml-anticorruption-layer]] already built.

**PT:** Porta de mão dupla para a regra em si -- um ADR futuro pode estreitar o escopo de novo com o
mesmo processo que este usou (decisão em nível de líder, não uma edição silenciosa), e nada que este
ADR muda é irreversível no nível de documento (`docs/rmlx-subset.md` e `docs/uix-rcss.md` continuam
documentos vivos pelos próprios termos do [[0020-rml-anticorruption-layer]]). **Inclinada pra mão
única na prática**, porém, pela mesma razão que o próprio [[0020-rml-anticorruption-layer]] é
tagueado `one-way-door-adjacent`: uma vez que o trabalho de implementação da `ESC-1..29` pousar
dentro da `RMLX-2..8`, voltar atrás pro subconjunto mais estreito descarta implementação real e
não-trivial já investida (a mesma assimetria nomeada para o próprio compromisso de eliminação em 12
ondas). Este ADR não reabre nem re-litiga o sequenciamento da `RMLX-3`, o cronograma de excisão da
`RMLX-11`, nem o gate físico da camada anticorrupção -- ele alarga um insumo de um mecanismo que o
[[0020-rml-anticorruption-layer]] já construiu.
