# ADR-0016 — Gamepad ships as its own atom (`GLINTFX_MODULE_GAMEPAD`), `input` stays reserved / Gamepad entregue como átomo próprio (`GLINTFX_MODULE_GAMEPAD`), `input` segue reservado

- **Status:** Accepted (2026-07-21) — originally an autonomous engineering decision by Caetano/CTO within the A2-GAMEPAD execution plan (open question #2); **retroactively confirmed by the leader (petrus) on 2026-07-21**, alongside the embedded SDL-database decision (D6), clearing the way for the v0.15.0 release. / originalmente uma decisão de engenharia autônoma de Caetano/CTO dentro do plano de execução A2-GAMEPAD (pergunta aberta #2); **confirmada retroativamente pelo líder (petrus) em 2026-07-21**, junto da decisão do banco SDL embutido (D6), liberando o caminho para a release v0.15.0.
- **Deciders:** Caetano (CTO, plan author); petrus (líder, retroactive confirmation / confirmação retroativa).
- **Tags:** architecture, build-system, framework-mode, modularity, naming, additive, two-way-door
- **Relates to:** [[0015-framework-2d-atomized-architecture]] (b) (the module map this ADR refines — `input` was originally described as ONE module covering both the keyboard/mouse backend and the gamepad backend), `docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md` (the plan whose decision D1 this ADR formalizes), `TODO.md` item `A2-GAMEPAD`.

## Context / Contexto

**EN:** [ADR-0015](0015-framework-2d-atomized-architecture.md) (b) lists `input` as a single
module whose public boundary is "`UiEvent`/`Key` (neutral format) + polling/callbacks for the
game; keyboard/mouse backend + gamepad backend (evdev/udev, hotplug)", and explicitly flags the
split as "honest": the keyboard/mouse backend depends on **window** (its events are born in the
window's event loop, A1-INPUT already delivers this half), while the gamepad backend does
**not** — it talks to `/dev/input/event*` directly via evdev, with zero dependency on any
window or GL context existing at all. ADR-0015's own consequences section already names
"gamepad-only" as a composition with standalone value: "a raw X11 host that wants hotplug
without SDL".

When A2-GAMEPAD's execution plan came to actually wire the CMake option and the OBJECT-library
target for this capability, the question ADR-0015 left open became concrete: is gamepad support
gated by a **new**, dedicated `GLINTFX_MODULE_GAMEPAD` flag, or does it fold into (and finally
activate) the `GLINTFX_MODULE_INPUT` flag that ADR-0015 already named, alongside the
keyboard/mouse backend once that lands? The two backends genuinely never need each other and, in
the specific composition ADR-0015 itself calls out as valuable ("gamepad-only"), the
keyboard/mouse half is not even present. Naming the flag `GLINTFX_MODULE_INPUT` while it gates
*only* gamepad code would be a misleading name for exactly that composition's binary.

**PT:** O [ADR-0015](0015-framework-2d-atomized-architecture.md) (b) lista `input` como UM
módulo só, cuja fronteira pública é "`UiEvent`/`Key` (formato neutro) + polling/callbacks pro
jogo; backend teclado/mouse + backend gamepad (evdev/udev, hotplug)", e já sinaliza
explicitamente o split como "honesto": o backend teclado/mouse depende de **window** (os
eventos dele nascem no loop de eventos da janela, o A1-INPUT já entrega essa metade), enquanto o
backend gamepad **não** depende — ele fala direto com `/dev/input/event*` via evdev, zero
dependência de qualquer janela ou contexto GL sequer existir. A própria seção de consequências
do ADR-0015 já cita "gamepad-only" como composição de valor próprio: "um host X11 cru que quer
hotplug sem SDL".

Quando o plano de execução do A2-GAMEPAD chegou na hora de de fato ligar a option do CMake e o
alvo OBJECT-library dessa capacidade, a pergunta que o ADR-0015 tinha deixado aberta virou
concreta: o suporte a gamepad é gateado por uma flag **nova**, dedicada,
`GLINTFX_MODULE_GAMEPAD`, ou dobra dentro (e finalmente ativa) a flag `GLINTFX_MODULE_INPUT` que
o ADR-0015 já nomeou, junto do backend teclado/mouse quando ele aterrissar? Os dois backends
genuinamente nunca precisam um do outro e, na composição específica que o próprio ADR-0015 cita
como valiosa ("gamepad-only"), a metade teclado/mouse nem sequer está presente. Nomear a flag
`GLINTFX_MODULE_INPUT` enquanto ela gateia *só* código de gamepad seria um nome enganoso
exatamente para o binário dessa composição.

## Decision / Decisão

**EN:**

**(a) Gamepad ships as its own atom, `GLINTFX_MODULE_GAMEPAD`** — mirror of `audio`/`i18n`:
depends on `core` ONLY, zero GL/window/RmlUi/third-party type in its public header
(`glintfx/include/glintfx/gamepad.hpp` + `gamepad_types.hpp`), OFF removes code and symbols
(`nm`-provable), tests are pure (no Xvfb). CMake option
`GLINTFX_MODULE_GAMEPAD` (default `ON`), forced `OFF` on any non-`Linux`
`CMAKE_SYSTEM_NAME` with a `STATUS` message — evdev is a Linux kernel interface, so this is an
honest platform gate, not an artificial restriction.

**(b) `GLINTFX_MODULE_INPUT` stays reserved, exactly as ADR-0015 (b) already named it**, for the
day the keyboard/mouse backend graduates from A1-INPUT's current home (fused into `App`/window)
into its own explicit module. This ADR does **not** revoke or rename that reservation — it only
narrows what "gamepad" answers to in the meantime. When `GLINTFX_MODULE_INPUT` is built, ADR-0015
(d)(4)'s aliasing rule (flag names honored as aliases even if the mechanics evolve) is the
template this ADR follows for gamepad too: `GLINTFX_MODULE_GAMEPAD` is not expected to be folded
away or renamed later — the "gamepad-only" composition ADR-0015 already values depends on it
staying its own, addressable atom.

**(c) The module list in ADR-0015 (b) is amended, not superseded**: the `input` row's
description is refined to read "the keyboard/mouse backend (depends on **window**); the gamepad
backend ships separately as its own atom, see ADR-0016" — every other clause of ADR-0015 (the
`ui`/`fx` split, the dependency DAG rules, the per-atom quality dossier, the ordering principle)
is untouched.

**PT:**

**(a) Gamepad é entregue como átomo próprio, `GLINTFX_MODULE_GAMEPAD`** — espelho de
`audio`/`i18n`: depende SÓ de `core`, zero tipo GL/janela/RmlUi/terceiro no header público
(`glintfx/include/glintfx/gamepad.hpp` + `gamepad_types.hpp`), OFF remove código e símbolos
(provável por `nm`), testes são puros (sem Xvfb). Option do CMake `GLINTFX_MODULE_GAMEPAD`
(default `ON`), forçada `OFF` em qualquer `CMAKE_SYSTEM_NAME` não-`Linux` com mensagem
`STATUS` — evdev é interface de kernel Linux, então isto é um gate honesto de plataforma, não
uma restrição artificial.

**(b) `GLINTFX_MODULE_INPUT` segue reservado, exatamente como o ADR-0015 (b) já o nomeou**, pro
dia em que o backend teclado/mouse se formar do lar atual do A1-INPUT (fundido no `App`/window)
pro próprio módulo explícito. Este ADR NÃO revoga nem renomeia essa reserva — só estreita o que
"gamepad" responde por enquanto. Quando `GLINTFX_MODULE_INPUT` for construído, a regra de alias
do ADR-0015 (d)(4) (nomes de flag honrados como aliases mesmo que a mecânica evolua) é o
template que este ADR segue pro gamepad também: `GLINTFX_MODULE_GAMEPAD` não é esperado a ser
dobrado de volta ou renomeado depois — a composição "gamepad-only" que o ADR-0015 já valoriza
depende dele continuar sendo o próprio átomo, endereçável.

**(c) A lista de módulos do ADR-0015 (b) é emendada, não substituída**: a descrição da linha
`input` é refinada pra ler "o backend teclado/mouse (depende de **window**); o backend gamepad é
entregue separadamente como átomo próprio, ver ADR-0016" — toda outra cláusula do ADR-0015 (o
split `ui`/`fx`, as regras do DAG de dependência, o dossiê de qualidade por-átomo, o princípio de
ordenação) fica intocada.

## Consequences / Consequências

**EN:**

**Positive:**
- The "gamepad-only" composition ADR-0015 already names as valuable (a raw X11 host wanting
  hotplug without SDL) becomes buildable and addressable TODAY, without waiting for
  `GLINTFX_MODULE_INPUT`'s keyboard/mouse half to land.
- No naming lie: a flag named `GLINTFX_MODULE_GAMEPAD` that gates only gamepad code is honest;
  a flag named `GLINTFX_MODULE_INPUT` doing the same would not be, for exactly that composition.
- Zero rework risk later: when the keyboard/mouse backend graduates into
  `GLINTFX_MODULE_INPUT`, gamepad does not need renaming, merging, or a deprecation cycle — the
  two atoms simply coexist, exactly like `audio` and `i18n` coexist as siblings today.

**Negative / accepted as cost:**
- ADR-0015's original module table technically undersold the granularity by one row (it
  described "input" as if it might ship as a single flag) — this ADR is the acknowledgment and
  correction, adding a small amount of ADR-reading overhead (two documents instead of one to
  understand the `input` family's final shape) for anyone auditing the module map.

**Risks / points of attention:**
- This started as an autonomous engineering decision (per the plan's own convention for calls
  made without a synchronous `AskUserQuestion` to the leader, since the plan's open questions
  explicitly did not block starting A2-GAMEPAD). **Retroactively confirmed by petrus on
  2026-07-21** — the risk this section originally tracked (the leader preferring a folded,
  single future `GLINTFX_MODULE_INPUT`) did not materialize; the own-atom shape is now the
  leader-endorsed one, not a provisional one awaiting review. Should a future need ever justify
  revisiting this shape, the project's ADR immutability rule still applies: a new ADR
  supersedes this one, it is not hand-edited.

**PT:**

**Positivas:**
- A composição "gamepad-only" que o ADR-0015 já cita como valiosa (um host X11 cru que quer
  hotplug sem SDL) vira buildável e endereçável HOJE, sem esperar a metade teclado/mouse do
  `GLINTFX_MODULE_INPUT` aterrissar.
- Sem mentira de nomenclatura: uma flag chamada `GLINTFX_MODULE_GAMEPAD` que gateia só código de
  gamepad é honesta; uma flag chamada `GLINTFX_MODULE_INPUT` fazendo o mesmo não seria, para
  exatamente essa composição.
- Zero risco de retrabalho depois: quando o backend teclado/mouse se formar em
  `GLINTFX_MODULE_INPUT`, o gamepad não precisa de renomeação, fusão, ou ciclo de depreciação —
  os dois átomos simplesmente coexistem, exatamente como `audio` e `i18n` coexistem como irmãos
  hoje.

**Negativas / aceitas como custo:**
- A tabela de módulos original do ADR-0015 tecnicamente subestimou a granularidade em uma linha
  (descreveu "input" como se pudesse sair numa flag só) — este ADR é o reconhecimento e a
  correção, somando um pouco de overhead de leitura de ADR (dois documentos em vez de um pra
  entender a forma final da família `input`) pra quem audita o mapa de módulos.

**Riscos / pontos de atenção:**
- Isto começou como uma decisão de engenharia autônoma (conforme a convenção do próprio plano
  pra chamadas feitas sem um `AskUserQuestion` síncrono ao líder, já que as perguntas abertas do
  plano explicitamente não bloqueavam o início do A2-GAMEPAD). **Confirmada retroativamente por
  petrus em 2026-07-21** — o risco que esta seção originalmente rastreava (o líder preferir um
  `GLINTFX_MODULE_INPUT` futuro único, dobrado) não se materializou; a forma de átomo próprio
  agora é a endossada pelo líder, não uma provisória aguardando revisão. Caso uma necessidade
  futura algum dia justifique revisitar esta forma, a regra de imutabilidade de ADR do projeto
  continua valendo: um ADR novo substitui este, não é editado à mão.

## Options considered / Opções consideradas

**EN:**
1. **Fold gamepad into a `GLINTFX_MODULE_INPUT` that ships partial (gamepad now, keyboard/mouse
   later).** Rejected: a flag whose name promises "input" but whose binary, until A1's
   keyboard/mouse backend graduates into it, only ever contains gamepad code is misleading —
   and worse, it would force a breaking rename or a confusing "input-gamepad-only" sub-flag once
   the keyboard/mouse half does land.
2. **Wait for `GLINTFX_MODULE_INPUT`'s full scope (both backends) before shipping any gamepad
   code.** Rejected: this defers the ADR-0015-endorsed "gamepad-only" composition for no
   architectural reason — the two backends have zero shared implementation and zero shared
   dependency, so there is nothing to gain from a synchronized ship date.
3. **A single `GLINTFX_MODULE_INPUT` NOW, with gamepad as its first (and for a while, only)
   backend.** Rejected for the same misleading-name reason as option 1, plus it would make the
   dependency-DAG story (gamepad depends on core only; keyboard/mouse depends on window) harder
   to express as ADR-0015 (d)(1)'s DAG rule expects — one flag, two genuinely-independent
   dependency profiles, is the shape ADR-0015 itself already flagged as needing an honest split.

**PT:**
1. **Dobrar o gamepad num `GLINTFX_MODULE_INPUT` que sai parcial (gamepad agora, teclado/mouse
   depois).** Rejeitada: uma flag cujo nome promete "input" mas cujo binário, até o backend
   teclado/mouse do A1 se formar nele, só contém código de gamepad é enganosa — e pior, forçaria
   uma renomeação quebrando compatibilidade ou uma sub-flag confusa "input-gamepad-only" quando a
   metade teclado/mouse enfim aterrissar.
2. **Esperar o escopo completo do `GLINTFX_MODULE_INPUT` (os dois backends) antes de entregar
   qualquer código de gamepad.** Rejeitada: isso adia a composição "gamepad-only" que o próprio
   ADR-0015 endossa sem nenhuma razão arquitetural — os dois backends têm zero implementação
   compartilhada e zero dependência compartilhada, então não há nada a ganhar com uma data de
   entrega sincronizada.
3. **Um `GLINTFX_MODULE_INPUT` único JÁ, com gamepad como primeiro (e por um tempo, único)
   backend.** Rejeitada pelo mesmo motivo de nome enganoso da opção 1, mais o fato de que
   dificultaria contar a história do DAG de dependência (gamepad depende só de core;
   teclado/mouse depende de window) do jeito que a regra de DAG do ADR-0015 (d)(1) espera — uma
   flag, dois perfis de dependência genuinamente independentes, é exatamente a forma que o
   próprio ADR-0015 já sinalizou precisar de um split honesto.

## Reversibility / Reversibilidade

**EN:** Two-way door. This is a naming/flag-boundary decision with no runtime behavior of its
own — reverting means introducing `GLINTFX_MODULE_INPUT` later and either (a) keeping
`GLINTFX_MODULE_GAMEPAD` as a permanent sibling atom (the expected path, per (b) above,
requiring zero change to already-shipped consumer code) or (b) a future ADR that supersedes this
one and folds the two, which WOULD be a breaking flag rename for any consumer that pinned
`GLINTFX_MODULE_GAMEPAD=OFF` explicitly. Low cost either way: no public API shape depends on the
flag's name, only the build-system surface does.

**PT:** Two-way door. É uma decisão de nomenclatura/fronteira-de-flag sem comportamento de
runtime próprio — reverter significa introduzir `GLINTFX_MODULE_INPUT` depois e ou (a) manter
`GLINTFX_MODULE_GAMEPAD` como átomo irmão permanente (o caminho esperado, conforme (b) acima,
exigindo zero mudança em código de consumidor já lançado) ou (b) um ADR futuro que substitui este
e dobra os dois, o que SERIA uma renomeação de flag quebrando compatibilidade pra qualquer
consumidor que tenha pinado `GLINTFX_MODULE_GAMEPAD=OFF` explicitamente. Custo baixo de qualquer
jeito: nenhuma forma de API pública depende do nome da flag, só a superfície do build-system
depende.
