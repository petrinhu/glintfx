# ADR-0020 — RmlUi anticorruption layer / Camada anticorrupção do RmlUi

- **Status:** Accepted (2026-08-04)
- **Deciders:** petrus (líder), caetano-cto, software-architect
- **Tags:** architecture, internalization, anti-scope-creep, one-way-door-adjacent
- **Builds on:** [[0009-internalization-boundary]] (the hosted-shell + C-meat pattern for
  Layer-0-into-Layer-1 static linking -- a **different mechanism**, not reused here directly, but
  the same internalization *doctrine*: sovereignty is spent where it has value, incrementally,
  behind gates, with a rollback path), [[0011-soft-font-flip]] (the **soft flip** precedent this
  ADR's own `RMLX-10` wave explicitly copies: flip the default, keep the old dependency linked and
  selectable, no config-line-away safety net removed until a separately-decided, later ADR).
- **Cross-ref:** `docs/rmlx-subset.md` (the frozen subset spec this ADR's decision produces and
  depends on -- read together, not a substitute for one another), `TODO.md` (`RMLX-0..11`, `WR0..WR11`).

> **EN -- Amendment 2026-08-07:** [[0022-paridade-total-com-o-motor-substituido]] **widens**, and does not
> revoke, the default this ADR's `docs/rmlx-subset.md` draws its boundary against. The two-part
> anticorruption mechanism described below -- physical confinement + frozen documentary spec, gated
> by the header clause's "stop, edit the spec, get sign-off, then implement" process -- is unchanged
> and remains binding. What changed is one input to that mechanism: `docs/rmlx-subset.md`'s default
> for what counts as "in scope" moved from **what the corpus measures** to **what the pinned
> upstream RmlUi 6.3 itself accepts** (líder's order, 2026-08-07: "NAO ESTREITE O ESCOPO POR
> GUSWORLD"). Read ADR-0022 for the full reasoning; this paragraph exists only so a reader of this
> ADR does not mistake the frozen-subset mechanism for a frozen-subset *content* that never changes.
>
> **PT -- Emenda 2026-08-07:** o [[0022-paridade-total-com-o-motor-substituido]] **alarga**, e não revoga,
> o default contra o qual o `docs/rmlx-subset.md` deste ADR traça sua fronteira. O mecanismo
> anticorrupção em duas partes descrito abaixo -- confinamento físico + spec documental congelada,
> com gate pelo processo da cláusula do cabeçalho ("parar, editar a spec, pegar aval, só então
> implementar") -- continua inalterado e vinculante. O que mudou foi um insumo desse mecanismo: o
> default do `docs/rmlx-subset.md` pro que conta como "dentro do escopo" saiu de **o que o corpus
> mede** para **o que o RmlUi 6.3 upstream pinado em si aceita** (ordem do líder, 2026-08-07: "NAO
> ESTREITE O ESCOPO POR GUSWORLD"). Leia o ADR-0022 pelo raciocínio completo; este parágrafo existe
> só para quem lê este ADR não confundir o mecanismo de subconjunto congelado com um *conteúdo* de
> subconjunto congelado que nunca muda.

## Context / Contexto

**EN:** The líder ordered the **elimination of RmlUi** as glintfx's UI/RCSS engine, over 12 waves
(`RMLX-0..11`, `TODO.md`), estimated at 2-3 months of continuous work, half the project's risk
concentrated in one wave (`RMLX-3`, layout). This is qualitatively different from ADR-0009's
internalization track: that ADR moves **Layer-0 sovereign C** into Layer-1 binaries via static
linking (`libcore.a`, `glx_*`); this one replaces a **linked third-party C++ library**
(RmlUi 6.3, MIT) with glintfx's own clean-room C++ implementation of the same contract, entirely
within Layer 1. Both are "internalization" in spirit -- reducing dependence on external code
the project does not control -- but the mechanism, the risk profile, and the failure mode if
scope is mismanaged are different enough that this needed its own decision, not a paragraph added
to ADR-0009.

The concrete risk this ADR exists to name: RmlUi 6.3's own surface is large -- a general-purpose
HTML/CSS-like UI toolkit, complete with things glintfx's actual two consumers (its own demos and
GusWorld) do not use at all (`nth-child`, `:not()`, table layout, complex-script text shaping).
Without a frozen boundary, the highest-risk wave (`RMLX-3`, layout) has every incentive to solve
the *general* problem ("let's build CSS") instead of the *measured* one, because "what does RCSS
even mean" has no natural stopping point without an artifact that names one. `RMLX-0`
(`WR0`, this ADR's own precondition) spent one wave doing exactly the measurement work needed to
draw that boundary: a census of glintfx's own contact with RmlUi (19 files with a real `#include`,
1174 lines, 94 distinct `Rml::` symbols, `git grep`-reproducible against commit `1a67dea`), a
census of the consumer's actual RCSS usage (12 numbers, re-measured and corrected twice by the
consumer and re-verified independently here), and a working type-boundary between glintfx's own
value types and RmlUi's (`glintfx/src/rml/type_bridge.hpp`). The open question this ADR answers:
**how does that measurement work become binding on the 11 waves that follow it**, rather than a
one-time report that a wave under schedule pressure quietly ignores?

**PT:** O líder ordenou a **eliminação do RmlUi** como motor de UI/RCSS da glintfx, em 12 ondas
(`RMLX-0..11`, `TODO.md`), estimadas em 2-3 meses de trabalho contínuo, com metade do risco do
projeto concentrada numa única onda (`RMLX-3`, layout). Isto é qualitativamente diferente da
trilha de internalização do ADR-0009: aquele ADR move **C soberano da Camada 0** pra binários da
Camada 1 via link estático (`libcore.a`, `glx_*`); este substitui uma **biblioteca C++ de
terceiro linkada** (RmlUi 6.3, MIT) pela própria implementação C++ clean-room da glintfx do mesmo
contrato, inteiramente dentro da Camada 1. Os dois são "internalização" em espírito -- reduzir
dependência de código externo que o projeto não controla -- mas o mecanismo, o perfil de risco, e
o modo de falha se o escopo for mal gerido são diferentes o bastante pra isto merecer decisão
própria, não um parágrafo somado ao ADR-0009.

O risco concreto que este ADR existe pra nomear: a própria superfície do RmlUi 6.3 é grande -- um
toolkit de UI genérico, estilo HTML/CSS, completo com coisas que os dois consumidores reais da
glintfx (as próprias demos e o GusWorld) não usam de jeito nenhum (`nth-child`, `:not()`, layout de
tabela, shaping de texto de script complexo). Sem uma fronteira congelada, a onda de maior risco
(`RMLX-3`, layout) tem todo incentivo pra resolver o problema *geral* ("vamos fazer CSS") em vez
do *medido*, porque "o que RCSS sequer significa" não tem parada natural sem um artefato que
nomeie uma. A `RMLX-0` (`WR0`, pré-condição deste próprio ADR) gastou uma onda inteira fazendo
exatamente o trabalho de medição necessário pra traçar essa fronteira: um censo do contato da
própria glintfx com o RmlUi (19 arquivos com `#include` real, 1174 linhas, 94 símbolos `Rml::`
distintos, reproduzível por `git grep` contra o commit `1a67dea`), um censo do uso real de RCSS do
consumidor (12 números, remedidos e corrigidos duas vezes pelo consumidor e reverificados aqui de
forma independente), e uma fronteira de tipo funcionando entre os tipos-valor próprios da glintfx
e os do RmlUi (`glintfx/src/rml/type_bridge.hpp`). A pergunta aberta que este ADR responde:
**como esse trabalho de medição vira vinculante pras 11 ondas que o seguem**, em vez de um
relatório único que uma onda sob pressão de cronograma ignora em silêncio?

## Decision / Decisão

**EN:** Adopt a **two-part anticorruption layer**: a physical/mechanical half (already shipped,
`RMLX-0/F1-F4`) and a documentary/governance half (`docs/rmlx-subset.md`, shipped alongside this
ADR). Neither half is sufficient alone; together they make the frozen boundary self-enforcing.

- **Physical confinement.** Every real `#include` of RmlUi, and every RmlUi value-type token
  (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`), is confined to
  `glintfx/src/rml/`, with exactly 2 frozen, documented exceptions (`app.cpp`,
  `system_glfw_dedup.hpp` -- the GLFW↔RmlUi platform-backend bridge, which retires with GLFW, not
  with `RMLX-*`). `tools/check_rml_whitelist.sh` enforces this mechanically -- 3 blocking checks
  plus 1 report-only debt counter, embedded self-test that proves the gate has teeth on both
  sides before every real run, wired into `tools/preci.sh` (pre-commit) and CI
  (`.github/workflows/ci.yml:449`). This makes the boundary **impossible to regress silently**: a
  future file that reintroduces an RmlUi `#include` outside the whitelist fails the build, it
  does not wait for a code reviewer to notice.
- **Frozen subset spec.** `docs/rmlx-subset.md` is the single source of truth for what
  `RMLX-1..11` may implement: the two censuses (glintfx's own RmlUi contact, and the consumer's
  12 measured RCSS numbers), the 4 value-type decisions with `file:line` evidence, the gate's
  exact contract, and a debt register naming which wave retires each of the 4 remaining opaque
  pointer types (`Rml::Context*`, `Rml::SystemInterface*`, `Rml::RenderInterface*`). Its own
  header states the enforcement rule in plain terms: **a wave may only implement what is in the
  frozen subset; anything believed missing requires editing the spec first, with the líder's
  sign-off, before implementation** -- never the reverse order.
- **Type-boundary pattern.** `glintfx/src/rml/type_bridge.hpp`: no alias for `Rml::String` (it
  already is `std::string`), reuse of glintfx's own `Vec2F`/`ColorF` for `Rml::Vector2f`/`Colourb`
  (standard-layout, field-copy safe, guarded by `sizeof`+`is_standard_layout` `static_assert`s
  that fail the build on a future RmlUi layout change rather than silently miscompiling), and
  explicit non-replication of `Rml::Variant` (non-trivial, SSO-backed -- a bridge for it would be
  a second implementation to maintain forever). This pattern is **binding**, not advisory: any
  `RMLX-1..11` wave that introduces a new glintfx-vs-RmlUi type boundary routes through this one
  file, so a future swap-out touches one file's conversion logic, not N call sites.

**The elimination itself is phased and reversible until `RMLX-11`.** `RMLX-10` ("the flip") is
explicitly modelled on ADR-0011's **soft** flip: `GLINTFX_OWN_UI_ENGINE=ON` becomes the default,
RmlUi becomes a runtime-selectable rollback, exactly as `FontEngine::FreeType` remained selectable
after the font-engine flip. Only `RMLX-11` ("excision") removes RmlUi from the repo entirely
(`FetchContent`, the teardown-UB patch, the `NOTICE` entry) -- **1-2 releases after the flip, and
only with the consumer confirmed on the bus.** This means the 4 opaque-pointer debt files
(`data_binder.hpp`, `engine.hpp`/`.cpp`, `render_gl3.hpp`) and the 4 tests with a deliberate RmlUi
include all retire at `RMLX-11`, not earlier -- a soft flip by definition keeps the old
dependency's types alive as the rollback path.

**PT:** Adotar uma **camada anticorrupção em duas partes**: uma metade física/mecânica (já
entregue, `RMLX-0/F1-F4`) e uma metade documental/de governança (`docs/rmlx-subset.md`, entregue
junto com este ADR). Nenhuma das duas metades basta sozinha; juntas, tornam a fronteira congelada
auto-aplicável.

- **Confinamento físico.** Todo `#include` real de RmlUi, e todo token de tipo-valor do RmlUi
  (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`), fica confinado a
  `glintfx/src/rml/`, com exatamente 2 exceções congeladas e documentadas (`app.cpp`,
  `system_glfw_dedup.hpp` -- a ponte de backend de plataforma GLFW↔RmlUi, que se aposenta com o
  GLFW, não com as `RMLX-*`). O `tools/check_rml_whitelist.sh` aplica isso mecanicamente -- 3
  checks bloqueantes mais 1 contador de dívida só-relatório, autoteste embutido que prova que o
  gate tem dente dos dois lados antes de todo run real, amarrado no `tools/preci.sh` (pre-commit)
  e no CI (`.github/workflows/ci.yml:449`). Isso torna a fronteira **impossível de regredir em
  silêncio**: um arquivo futuro que reintroduza um `#include` de RmlUi fora da whitelist reprova o
  build, não espera um revisor de código perceber.
- **Spec de subconjunto congelada.** `docs/rmlx-subset.md` é a fonte única de verdade pro que a
  `RMLX-1..11` tem permissão de implementar: os dois censos (contato da própria glintfx com o
  RmlUi, e os 12 números de RCSS medidos pelo consumidor), as 4 decisões de tipo-valor com
  evidência `arquivo:linha`, o contrato exato do gate, e um registro de dívida nomeando qual onda
  aposenta cada um dos 4 tipos-ponteiro opacos restantes (`Rml::Context*`,
  `Rml::SystemInterface*`, `Rml::RenderInterface*`). O próprio cabeçalho dela declara a regra de
  aplicação em termos simples: **uma onda só pode implementar o que está no subconjunto
  congelado; qualquer coisa que pareça faltar exige editar a spec primeiro, com o aval do líder,
  antes de implementar** -- nunca a ordem inversa.
- **Padrão de fronteira de tipo.** `glintfx/src/rml/type_bridge.hpp`: nenhum alias pra
  `Rml::String` (já é `std::string`), reuso dos próprios `Vec2F`/`ColorF` da glintfx pra
  `Rml::Vector2f`/`Colourb` (standard-layout, seguro pra cópia de campo, guardado por
  `static_assert`s de `sizeof`+`is_standard_layout` que derrubam o build numa futura mudança de
  layout do RmlUi em vez de miscompilar em silêncio), e não-replicação explícita de
  `Rml::Variant` (não-trivial, apoiado em SSO -- uma ponte pra ele seria uma segunda implementação
  pra manter pra sempre). Este padrão é **vinculante**, não uma sugestão: qualquer onda
  `RMLX-1..11` que introduzir uma fronteira de tipo nova glintfx-vs-RmlUi passa por este único
  arquivo, então uma futura troca toca a lógica de conversão de um arquivo só, não N sítios de
  chamada.

**A eliminação em si é faseada e reversível até a `RMLX-11`.** A `RMLX-10` ("o flip") é
explicitamente modelada no flip **suave** do ADR-0011: `GLINTFX_OWN_UI_ENGINE=ON` vira o default,
o RmlUi vira rollback selecionável em runtime, exatamente como o `FontEngine::FreeType` continuou
selecionável depois do flip do motor de fonte. Só a `RMLX-11` ("excisão") remove o RmlUi do repo
por completo (`FetchContent`, o patch de teardown-UB, a entrada no `NOTICE`) -- **1-2 releases
depois do flip, e só com o consumidor confirmado no bus.** Isso significa que os 4 arquivos de
dívida de ponteiro opaco (`data_binder.hpp`, `engine.hpp`/`.cpp`, `render_gl3.hpp`) e os 4 testes
com include deliberado de RmlUi todos se aposentam na `RMLX-11`, não antes -- um flip suave, por
definição, mantém os tipos da dependência antiga vivos como caminho de rollback.

## Consequences / Consequências

**EN:** Every `RMLX-1..11` wave now has a concrete, checkable acceptance gate for scope, not just
a functional one: does the wave's diff stay inside `docs/rmlx-subset.md`'s frozen boundary, and
does `tools/check_rml_whitelist.sh` still report `OK`/`divida opaca: 4 arquivos` (shrinking only
as waves retire the named debt, never growing)? A wave that wants to go beyond the spec has an
explicit, named process (edit the spec, get sign-off, then implement) instead of an implicit one
(quietly build it, hope nobody asks). `docs/rmlx-subset.md` becomes a **living document with a
non-trivial update cost by design**: each edit is itself a scope decision, and the spec's own
header makes clear who owns approving one. The debt register's wave assignments (`RMLX-11` for
all 4 opaque-pointer files) create an explicit dependency: no opaque `Rml::` pointer type can be
fully retired before `RMLX-11`, which is now a documented, cross-referenced constraint rather
than something a late wave discovers by surprise. This ADR is tagged `one-way-door-adjacent`, not
a full one-way door: the physical gate (`check_rml_whitelist.sh`) is trivially reversible (delete
the script, nobody enforces the boundary anymore), but the **12-wave elimination commitment
itself** -- once `RMLX-3` (layout) is underway, walking back to "keep RmlUi forever" abandons
real, non-trivial sunk work, which is the property a one-way door has. Divergences found while
writing `docs/rmlx-subset.md` (the brief's "17+4=21" contact-point arithmetic vs. the
re-measured 19/22/41, and the unreconciled 41-vs-56 file-count split against `TODO.md`'s own
historical note) are recorded in the spec itself rather than silently corrected -- a documented
discrepancy is more useful to the next wave than a quietly "fixed" number nobody can audit
against its source.

**PT:** Toda onda `RMLX-1..11` agora tem um gate de aceite de escopo concreto e checável, não só
funcional: o diff da onda fica dentro da fronteira congelada do `docs/rmlx-subset.md`, e o
`tools/check_rml_whitelist.sh` continua reportando `OK`/`divida opaca: 4 arquivos` (encolhendo só
à medida que ondas aposentam a dívida nomeada, nunca crescendo)? Uma onda que quer ir além da spec
tem um processo explícito e nomeado (editar a spec, pegar aval, só então implementar) em vez de
um implícito (construir em silêncio, torcer pra ninguém perguntar). O `docs/rmlx-subset.md` vira
um **documento vivo com custo de atualização não-trivial de propósito**: cada edição é ela mesma
uma decisão de escopo, e o próprio cabeçalho da spec deixa claro quem é dono de aprová-la. As
atribuições de onda do registro de dívida (`RMLX-11` pros 4 arquivos de ponteiro opaco) criam uma
dependência explícita: nenhum tipo-ponteiro opaco `Rml::` pode se aposentar por completo antes da
`RMLX-11`, o que agora é uma restrição documentada e referenciada em vez de algo que uma onda
tardia descobre de surpresa. Este ADR é tagueado `one-way-door-adjacent`, não uma porta de mão
única completa: o gate físico (`check_rml_whitelist.sh`) é trivialmente reversível (apaga o
script, ninguém mais aplica a fronteira), mas o **compromisso de eliminação em 12 ondas em si** --
uma vez que a `RMLX-3` (layout) estiver em andamento, voltar atrás pra "manter o RmlUi pra
sempre" abandona trabalho real e não-trivial já investido, que é a propriedade que uma porta de
mão única tem. Divergências achadas ao escrever o `docs/rmlx-subset.md` (a aritmética
"17+4=21" de pontos de contato do brief vs. a remedição 19/22/41, e o recorte de contagem de
arquivo 41-vs-56 não-reconciliado contra a própria nota histórica do `TODO.md`) ficam registradas
na própria spec em vez de silenciosamente corrigidas -- uma discrepância documentada é mais útil
pra próxima onda do que um número "consertado" em silêncio que ninguém consegue auditar contra a
fonte.

## Options considered / Opções consideradas

- **Chosen -- physical gate + frozen documentary spec, both mandatory.** Pros: the boundary is
  enforced two ways that fail differently (a gate catches a mechanical regression instantly; a
  spec catches a scope decision a gate cannot express, like "should `RMLX-4` implement
  `nth-child`"); the spec is grep-able evidence for every future wave, not tribal knowledge;
  divergences between the original brief and re-measurement are preserved rather than silently
  smoothed over. Cons: real, ongoing maintenance cost -- every wave that touches the RmlUi
  boundary must first read a non-trivial document, and the spec itself needs updating (with
  sign-off) if real evidence outgrows it, which is friction by design, not an oversight.
- **Documentary spec only, no physical gate.** Pros: cheaper to produce; no CI/pre-commit
  maintenance burden. Cons: nothing stops a wave under schedule pressure from ignoring the spec
  and reintroducing an RmlUi include outside `glintfx/src/rml/` -- exactly the risk `RMLX-0`'s own
  brief called "the anti-scope-creep of every other wave"; a code-review-only enforcement model
  is precisely what this house's own doctrine ("relatório de agente não é prova", "implementer ≠
  reviewer") distrusts for anything checkable mechanically. Rejected.
- **Physical gate only, no frozen spec.** Pros: catches the mechanical regression (a stray
  `#include`) just as well. Cons: does **not** solve the actual risk named in the context -- a
  gate that only checks "is RmlUi confined to `glintfx/src/rml/`" says nothing about whether the
  *replacement* code inside `RMLX-1..11` is reimplementing the measured subset or drifting into
  general-purpose CSS; the layout wave's derailment risk is a **scope** problem, not an
  **encapsulation** problem, and only a named, evidence-backed subset addresses scope. Rejected.
- **No anticorruption layer -- let each `RMLX-*` wave self-scope from its own reading of the
  consumer's `.rml`/`.rcss` files.** Pros: zero upfront cost, maximally flexible per wave. Cons:
  exactly the failure mode this ADR exists to prevent -- 12 waves, each independently
  re-deriving "what does RCSS mean here", with no shared, checked artifact to catch drift between
  them, and no mechanism to stop the highest-risk wave from silently expanding scope under its
  own schedule pressure. Rejected outright, this is the status quo ante the ADR is meant to fix.

## Reversibility / Reversibilidade

**EN:** Hybrid, leaning one-way-door for the elimination commitment. The physical gate
(`check_rml_whitelist.sh`) and the spec document are themselves two-way doors -- either can be
edited, relaxed, or deleted with a normal commit, no líder sign-off structurally required by the
mechanism itself (though the spec's own header asks for one before scope grows). What is
effectively one-way is the **12-wave elimination program this ADR formalises the guardrails
for**: past `RMLX-3` (layout, the highest-risk wave, ~half the project's total risk per the
líder's own framing), reverting to "keep RmlUi as the permanent engine" discards real,
non-trivial sunk implementation work, the same asymmetry ADR-0009 names for its own static-link
boundary. This ADR required the líder's ordering of the 12-wave arc as its precondition; it does
not itself re-litigate that call, only the mechanism that keeps the waves honest to it.

**PT:** Híbrida, inclinada pra porta de mão única no que toca o compromisso de eliminação. O
próprio gate físico (`check_rml_whitelist.sh`) e o documento de spec são portas de mão dupla --
qualquer um pode ser editado, relaxado ou apagado com um commit normal, sem aval do líder
estruturalmente exigido pelo mecanismo em si (embora o próprio cabeçalho da spec peça um antes do
escopo crescer). O que é efetivamente mão-única é o **programa de eliminação em 12 ondas que este
ADR formaliza as guarda-corpos de**: passada a `RMLX-3` (layout, a onda de maior risco, ~metade do
risco total do projeto pelo próprio enquadramento do líder), reverter pra "manter o RmlUi como
motor permanente" descarta trabalho de implementação real e não-trivial já investido, a mesma
assimetria que o ADR-0009 nomeia pra própria fronteira de link estático. Este ADR exigiu a ordem
do líder pro arco de 12 ondas como pré-condição; ele mesmo não re-litiga essa decisão, só o
mecanismo que mantém as ondas honestas a ela.
