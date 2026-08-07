# GitHub Actions -- Operation Section (managing runs, monitoring, troubleshooting)

> STATUS: IN PROGRESS -- incremental write (page read = page recorded). Final consolidation into
> `docs/github-actions.md` happens later, coordinated by the orchestrator. Author: `devops-sre`,
> slice `DOC-GHA-CANONICO`, sub-section "GERENCIAR EXECUÇÕES, MONITORAR E DIAGNOSTICAR".
>
> Scope assigned by the orchestrator: re-run/cancel/approve runs, `concurrency`, queue and time
> limits (`timeout-minutes`); history, per-job/per-step status, status badges, notifications; log
> reading, diagnostic logging (`ACTIONS_STEP_DEBUG`/`ACTIONS_RUNNER_DEBUG`), error/warning
> annotations, workflow commands (`::error`, `::warning`, `::group`, `::notice`), job summaries
> (`$GITHUB_STEP_SUMMARY`); quota/billing/usage limits where they affect whether a workflow runs
> at all; and what GitHub documents about service outages.
>
> Explicitly left to other sections (do not duplicate here): full syntax reference/contexts
> (`secao-referencia.md`), authoring workflows and matrices (`secao-escrever.md`), security and
> self-hosted runners (`secao-seguranca.md`). "Migrating to GitHub Actions" from another CI was
> declared out of scope by the brief and was **not read**.
>
> Page-read denominator: see footer.

---

## 0. Why this section exists: the 2026-08-06 outage

This section was written the same day the project spent roughly four hours diagnosing a real
GitHub Actions incident across its own six workflows (`ci.yml`, `core-ci.yml`, `heavy.yml`,
`distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`). Every topic below is tied, where a link
exists, to one of the seven measured lessons from that day. This is deliberate: a page that only
restates the official docs is not worth the space it occupies (see `CONTEXTO.md`).

Esta seção foi escrita no mesmo dia em que o projeto passou cerca de quatro horas diagnosticando
uma pane real do GitHub Actions atravessando seus seis fluxos (`ci.yml`, `core-ci.yml`,
`heavy.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`). Cada tópico abaixo é
amarrado, onde existe ligação, a uma das sete lições medidas naquele dia. Isso é proposital: uma
página que só reconta a documentação oficial não vale o espaço que ocupa (ver `CONTEXTO.md`).

---

## 1. Managing runs -- manual dispatch, re-run, cancel, approve, concurrency

### 1.1 Manual dispatch (`workflow_dispatch`)

**Source:** https://docs.github.com/pt/actions/how-tos/manage-workflow-runs/manually-run-a-workflow

#### EN

For a workflow to be dispatchable by hand, it must declare the `workflow_dispatch` event in its
`on:` block, and the workflow file must live on the repository's **default branch** for the
"Run workflow" button/API to see it. Three ways to trigger it:

1. **Web UI** -- Actions tab -> select the workflow -> "Run workflow" button. Only workflows with
   `workflow_dispatch` show this button.
2. **`gh` CLI** -- `gh workflow run WORKFLOW` (accepts a workflow name, numeric ID, or filename).
3. **REST API** -- `POST` with `inputs` and `ref` in the body.

`workflow_dispatch` can declare up to **25 inputs**, each with `type` (`string`, `boolean`,
`choice`, `environment`), `default`, and `required`. Via `gh`, pass them with `-f key=value`
(string) or `-F key=value` (typed/file), or pipe JSON to stdin.

#### PT

Para um fluxo poder ser disparado manualmente, ele precisa declarar o evento `workflow_dispatch`
no bloco `on:`, e o arquivo do fluxo precisa estar no **branch padrão** do repositório para que o
botão "Executar fluxo de trabalho"/a API o enxerguem. Três formas de disparar:

1. **Interface web** -- aba Actions -> selecionar o fluxo -> botão "Executar fluxo de trabalho".
   Só fluxos com `workflow_dispatch` mostram esse botão.
2. **CLI `gh`** -- `gh workflow run WORKFLOW` (aceita nome, ID numérico ou nome de arquivo).
3. **API REST** -- `POST` com `inputs` e `ref` no corpo.

`workflow_dispatch` pode declarar até **25 inputs**, cada um com `type` (`string`, `boolean`,
`choice`, `environment`), `default` e `required`. Via `gh`, passe com `-f key=value` (texto) ou
`-F key=value` (tipado/arquivo), ou envie JSON pela entrada padrão.

#### Tie-in / Amarração -- `CI-TIDY-CRASH... não, CI-DISPATCH-MANUAL` (lição 1)

**EN.** During the ~4h GitHub Actions outage, `push` events stopped creating workflow runs, but
manual dispatch kept working. `ci.yml`, `core-ci.yml`, and `windows-atoms.yml` had **no**
`workflow_dispatch` trigger at the time, so attempting to dispatch them by hand failed with
`HTTP 422` (the API correctly refuses to dispatch a workflow event the file doesn't declare --
this matches the doc's "only workflows using the `workflow_dispatch` trigger have the option").
As of this writing **all six** workflows in this repo declare `workflow_dispatch` (verified:
`grep -l workflow_dispatch .github/workflows/*.yml` returns all six files). **Resilience of a CI
setup is only provable when the normal path is down** -- a workflow without a manual trigger is
indistinguishable from one with, as long as `push` never fails.

**PT.** Durante a pane de ~4h do GitHub Actions, eventos de `push` pararam de criar execuções, mas
o disparo manual continuou funcionando. `ci.yml`, `core-ci.yml` e `windows-atoms.yml` não tinham
`workflow_dispatch` na época, então tentar disparar à mão falhava com `HTTP 422` (a API
corretamente recusa disparar um evento que o arquivo não declara -- bate com "somente fluxos que
usam o gatilho `workflow_dispatch` têm a opção" da documentação). Hoje os **seis** fluxos deste
repo declaram `workflow_dispatch` (verificado: `grep -l workflow_dispatch .github/workflows/*.yml`
retorna os seis arquivos). **A resiliência de um setup de CI só se prova quando o caminho normal
cai** -- um fluxo sem gatilho manual é indistinguível de um com, enquanto o `push` nunca falha.

---

### 1.2 Re-running workflows and jobs

**Source:** https://docs.github.com/pt/actions/how-tos/manage-workflow-runs/re-run-workflows-and-jobs

#### EN

Three granularities, each available both via the web UI and via `gh`:

| What | Web UI | `gh` CLI |
|---|---|---|
| Whole workflow (all jobs) | Run page -> sync icon -> "Re-run all jobs" | `gh run rerun RUN_ID` |
| Only failed jobs | Run page -> sync icon -> "Re-run failed jobs" | `gh run rerun RUN_ID --failed` |
| One specific job | Job sidebar -> sync icon next to the job | `gh run rerun --job JOB_ID` |

Append `--debug` to any `gh run rerun` to also turn on debug logging for that re-run (equivalent
to the UI's "Enable debug logging" checkbox).

**Limits (documented, literal):** a run can be re-run **up to 30 days** after its original
execution, and **at most 50 times** total per run (full and partial re-runs count together
against the same 50).

#### PT

Três granularidades, cada uma disponível pela interface web e pelo `gh`:

| O quê | Interface web | CLI `gh` |
|---|---|---|
| Fluxo inteiro (todos os jobs) | Página da execução -> ícone de sincronizar -> "Executar novamente todos os trabalhos" | `gh run rerun RUN_ID` |
| Só os jobs que falharam | Página da execução -> ícone de sincronizar -> "Reexecutar trabalhos com falha" | `gh run rerun RUN_ID --failed` |
| Um job específico | Barra lateral de trabalhos -> ícone de sincronizar ao lado do job | `gh run rerun --job JOB_ID` |

Acrescente `--debug` a qualquer `gh run rerun` para ligar também o log de depuração nessa
re-execução (equivale à caixa "Enable debug logging" da UI).

**Limites (documentados, literais):** uma execução pode ser re-executada até **30 dias** após a
execução original, e **no máximo 50 vezes** ao todo por execução (re-execuções completas e
parciais contam juntas para o mesmo teto de 50).

#### Tie-in / Amarração -- lição 4 (aplica a re-execução)

**EN.** `gh run rerun --failed` is **refused while the run is still in progress** -- measured
directly during today's incident (the CLI errors out; it will not queue a re-run of a run that
hasn't reached a terminal state yet). Practical consequence for a diagnostic procedure: check
`status` (not just `conclusion`) before attempting a re-run -- see step 4 of the diagnostic
procedure in section 3.5.

**PT.** `gh run rerun --failed` é **recusado enquanto a execução ainda está em andamento** --
medido diretamente durante o incidente de hoje (a CLI dá erro; não enfileira re-execução de uma
run que ainda não chegou a um estado terminal). Consequência prática para um procedimento de
diagnóstico: checar o `status` (não só `conclusion`) antes de tentar re-executar -- ver passo 4 do
procedimento de diagnóstico na seção 3.5.

---

### 1.3 Cancelling a run

**Sources:**
https://docs.github.com/pt/actions/how-tos/manage-workflow-runs/cancel-a-workflow-run ·
https://docs.github.com/pt/actions/reference/workflows-and-actions/workflow-cancellation

#### EN

Cancel from the run page ("queued" or "in progress" runs only) -> "Cancel workflow" button, top
right. Cancelling stops **all jobs and steps** of that run, not just one job.

The signal sequence GitHub sends to the running process, literal from the reference page:

1. `SIGINT`/`Ctrl-C` to the step's entry process (`node` for JS actions, `docker` for container
   actions, `bash`/`cmd`/`pwsh` for `run:` steps).
2. If the process hasn't exited after **7,500 ms**, the runner escalates to `SIGTERM`/`Ctrl-Break`
   and waits another **2,500 ms**.
3. If it's still alive, the runner force-kills the whole process tree.
4. Independently, there's a global **5-minute cancellation timeout**: after that window, the
   server force-terminates any job/step still marked for cancellation.

The doc does not detail matrix-job-specific cancellation semantics or resource-release mechanics
beyond "may help release related resources" -- **not confirmed in the documentation** what exactly
gets released or when.

#### PT

Cancele pela página da execução (só runs "na fila" ou "em andamento") -> botão "Cancelar fluxo de
trabalho", canto superior direito. Cancelar interrompe **todos os jobs e steps** daquela execução,
não só um job.

A sequência de sinais que o GitHub envia ao processo em execução, literal da página de referência:

1. `SIGINT`/`Ctrl-C` para o processo de entrada do step (`node` para ações JS, `docker` para ações
   de contêiner, `bash`/`cmd`/`pwsh` para steps `run:`).
2. Se o processo não sair em **7.500 ms**, o executor escala para `SIGTERM`/`Ctrl-Break` e espera
   mais **2.500 ms**.
3. Se ainda estiver vivo, o executor mata à força a árvore de processos inteira.
4. Independente disso, existe um **timeout global de cancelamento de 5 minutos**: passada essa
   janela, o servidor força o encerramento de qualquer job/step ainda marcado para cancelamento.

A documentação não detalha a semântica de cancelamento específica de jobs em matriz nem os
mecanismos de liberação de recursos além de "pode ajudar a liberar recursos relacionados" --
**não confirmado na documentação** o que exatamente é liberado ou quando.

#### Tie-in / Amarração

**EN.** No direct lesson from today ties to manual cancellation specifically, but the 7,500 ms +
2,500 ms grace window matters for any process in this repo's heavy jobs (`sanitize`, `fonteng` in
`heavy.yml`) that installs signal handlers or writes state on exit -- a cancelled run gives roughly
10 seconds of graceful-shutdown budget before SIGKILL, not zero.

**PT.** Nenhuma lição de hoje amarra diretamente a cancelamento manual, mas a janela de graça de
7.500 ms + 2.500 ms importa para qualquer processo dos jobs pesados deste repo (`sanitize`,
`fonteng` em `heavy.yml`) que instale handler de sinal ou grave estado ao sair -- uma execução
cancelada dá cerca de 10 segundos de orçamento de desligamento gracioso antes do SIGKILL, não zero.

---

### 1.4 Approving runs from forks

**Source:** https://docs.github.com/pt/actions/how-tos/manage-workflow-runs/approve-runs-from-forks

#### EN

Only a maintainer with **write access** can approve a workflow run triggered by a fork's pull
request. This exists because a `pull_request` from an external contributor may carry hostile code,
and the risk is especially acute for **any proposed change under `.github/workflows/`** itself.
Approve from the PR page -> "Waiting for approval" -> "Approve and run workflows". Runs awaiting
approval for more than **30 days** are automatically deleted.

**Not relevant to this repo today** (no forks send PRs to `petrinhu/glintfx` currently), but
directly relevant to the self-hosted runner used by `heavy.yml` -- see the cross-reference in
lesson 5 below, and the full treatment in `secao-seguranca.md`.

#### PT

Só um mantenedor com **acesso de gravação** pode aprovar uma execução disparada por pull request
de um fork. Existe porque um `pull_request` de colaborador externo pode carregar código hostil, e
o risco é especialmente alto para **qualquer alteração proposta em `.github/workflows/`**. Aprova
pela página da PR -> "Aguardando aprovação" -> "Aprovar e executar fluxos de trabalho". Execuções
aguardando aprovação por mais de **30 dias** são apagadas automaticamente.

**Não é relevante para este repo hoje** (nenhum fork manda PR para `petrinhu/glintfx` atualmente),
mas é diretamente relevante ao executor self-hosted usado por `heavy.yml` -- ver a referência
cruzada com a lição 5 abaixo, e o tratamento completo em `secao-seguranca.md`.

#### Tie-in / Amarração -- lição 5 (fronteira com segurança)

**EN.** Lesson 5 states self-hosted runners in a public repo must not have a `pull_request`
trigger -- already this repo's practice (`heavy.yml` triggers only on `push` to `main`, filtered
by path, plus `workflow_dispatch`; **zero** `pull_request`). The approval mechanism above is the
documented alternative GitHub offers for repos that *do* want forks to trigger workflows (with
**GitHub-hosted** runners); it does not make `pull_request` safe on a **self-hosted** runner --
that is out of this section's scope and belongs to `secao-seguranca.md`. Flagging the boundary
here so the reader isn't tempted to treat "approve runs from forks" as a substitute for the
no-`pull_request`-on-self-hosted rule.

**PT.** A lição 5 diz que executor self-hosted em repo público não pode ter gatilho
`pull_request` -- já é prática deste repo (`heavy.yml` dispara só em `push` na `main`, filtrado por
caminho, mais `workflow_dispatch`; **zero** `pull_request`). O mecanismo de aprovação acima é a
alternativa documentada que o GitHub oferece para repos que **querem** que forks disparem fluxos
(com executores **hospedados pelo GitHub**); não torna `pull_request` seguro num executor
**self-hosted** -- isso está fora do escopo desta seção e pertence a `secao-seguranca.md`. Marcando
a fronteira aqui para o leitor não tratar "aprovar execuções de forks" como substituto da regra de
nunca-`pull_request`-em-self-hosted.

---

### 1.5 Concurrency (`concurrency:` group, cancel-in-progress, queue)

**Sources:**
https://docs.github.com/pt/actions/concepts/workflows-and-actions/concurrency ·
https://docs.github.com/pt/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency

#### EN

By default GitHub Actions runs everything concurrently -- multiple runs of the same workflow, or
multiple jobs, can execute at once. `concurrency:` (at workflow or job level) puts runs into a
named **group**; by default only one run per group proceeds, and additional pending runs in the
same group queue up (`queue: single`, the default) unless `cancel-in-progress: true` is set, in
which case a new run in the group **cancels** whatever was already running/pending in it.

```yaml
concurrency:
  group: string-or-expression
  cancel-in-progress: boolean-or-expression
  queue: single | max      # max allows up to 100 pending runs in the group
```

`group` accepts expressions using the `github`, `inputs`, `vars`, `needs`, `strategy`, and
`matrix` contexts.

Common patterns:

```yaml
# Cancel stale PR runs when a new commit lands
concurrency:
  group: ${{ github.head_ref || github.run_id }}
  cancel-in-progress: true

# Serialize production deploys, never cancel one mid-flight
concurrency:
  group: production-deploy
  queue: max
```

#### PT

Por padrão o GitHub Actions roda tudo concorrentemente -- várias execuções do mesmo fluxo, ou
vários jobs, podem rodar ao mesmo tempo. `concurrency:` (em nível de fluxo ou de job) agrupa
execuções num **grupo** nomeado; por padrão só uma execução por grupo avança, e execuções
pendentes adicionais no mesmo grupo entram na fila (`queue: single`, o padrão), a menos que
`cancel-in-progress: true` esteja definido, caso em que uma nova execução no grupo **cancela** o
que já estava rodando/pendente nele.

```yaml
concurrency:
  group: string-ou-expressão
  cancel-in-progress: boolean-ou-expressão
  queue: single | max      # max permite até 100 execuções pendentes no grupo
```

`group` aceita expressões usando os contextos `github`, `inputs`, `vars`, `needs`, `strategy` e
`matrix`.

Padrões comuns:

```yaml
# Cancelar execuções obsoletas de PR quando um commit novo chega
concurrency:
  group: ${{ github.head_ref || github.run_id }}
  cancel-in-progress: true

# Serializar deploys de produção, nunca cancelar um em andamento
concurrency:
  group: production-deploy
  queue: max
```

#### Tie-in / Amarração -- resource used once, not everywhere

**EN.** `grep -rn concurrency .github/workflows/*.yml` shows this repo uses `concurrency:` in
exactly **one** of six workflows -- `heavy.yml` (the self-hosted-runner job). The other five
(`ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`) have no
`concurrency:` block, meaning two pushes seconds apart to the same branch queue two full,
independent GitHub-hosted runs instead of cancelling the stale one. That is a legitimate choice
for `ci.yml` (the release gate -- you may *want* every push's result recorded), but it is a
resource the documentation offers and this repo does not use uniformly; listed in the closing
report as a candidate for `distro-matrix.yml` and `nightly.yml`, which are the two most
minute-expensive workflows (matrix across three distros; nightly sanitizer run) and the ones where
a superseded push wastes the most GitHub-hosted minutes.

**PT.** `grep -rn concurrency .github/workflows/*.yml` mostra que este repo usa `concurrency:` em
exatamente **um** dos seis fluxos -- `heavy.yml` (o job de executor self-hosted). Os outros cinco
(`ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`) não têm bloco
`concurrency:`, o que significa que dois pushes segundos apart na mesma branch enfileiram duas
execuções completas e independentes em executor hospedado pelo GitHub, em vez de cancelar a
obsoleta. É escolha legítima para `ci.yml` (o gate de release -- pode-se *querer* o resultado de
cada push registrado), mas é um recurso que a documentação oferece e este repo não usa de forma
uniforme; listado no relatório final como candidato para `distro-matrix.yml` e `nightly.yml`, os
dois fluxos mais caros em minutos (matriz de três distros; execução noturna do sanitizer) e onde
um push superado desperdiça mais minutos de executor hospedado.

---

### 1.6 `timeout-minutes` (job-level time ceiling)

**Not confirmed in a dedicated "timeout" how-to page** -- `timeout-minutes` is documented as a
job-level key in the workflow syntax reference (owned by `secao-referencia.md`); this section only
covers its *operational* effect: it caps how long a stuck job holds a runner before GitHub kills
it and marks the job `cancelled`/timed-out, which matters for diagnosis (section 3) because a
timeout looks different in the log than a crash or a real failure.

Não confirmado em página dedicada de "timeout" -- `timeout-minutes` é documentado como chave de
nível de job na referência de sintaxe de fluxo (posse de `secao-referencia.md`); esta seção só
cobre o efeito *operacional*: ele limita por quanto tempo um job travado segura um executor antes
de o GitHub matá-lo e marcar o job como `cancelled`/expirado, o que importa para diagnóstico
(seção 3) porque um timeout aparece diferente no log de um crash ou de uma falha real.

#### Tie-in / Amarração -- resource used in 2 of 6 workflows

**EN.** `grep -rn timeout-minutes .github/workflows/*.yml` finds it in exactly **two** jobs, both
in `heavy.yml`: `sanitize` and `fonteng`, each `timeout-minutes: 120`. The other four workflows
(`ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`) rely on the
platform default (**360 minutes / 6 hours** for GitHub-hosted jobs, per section 4's limits table).
For a CI gate whose expected wall-clock time is a few minutes, a 6-hour default ceiling means a
genuinely hung job (not the ~4h *outage* -- an actual stuck process) burns up to six hours of
minutes before GitHub itself intervenes; nobody watching would wait that long, but nothing stops
the meter. Candidate for the closing report's "not used" list.

**PT.** `grep -rn timeout-minutes .github/workflows/*.yml` encontra em exatamente **dois** jobs,
ambos em `heavy.yml`: `sanitize` e `fonteng`, cada um `timeout-minutes: 120`. Os outros quatro
fluxos (`ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`) contam
com o teto padrão da plataforma (**360 minutos / 6 horas** para jobs hospedados pelo GitHub,
conforme a tabela de limites da seção 4). Para um gate de CI cujo tempo esperado é poucos minutos,
um teto padrão de 6h significa que um job genuinamente travado (não a *pane* de ~4h -- um processo
realmente empacado) queima até seis horas de minutos antes de o próprio GitHub intervir; ninguém
observando esperaria tanto, mas nada trava o medidor. Candidato à lista de "não usado" do relatório
final.

---

## 2. Monitoring runs -- history, per-job/step status, badges, notifications

### 2.1 Run history

**Source:** https://docs.github.com/pt/actions/how-tos/monitor-workflows/view-workflow-run-history

#### EN

Web UI: Actions tab -> select workflow in the left sidebar -> list of runs. `gh` CLI: `gh run
list` (default limit 10 results, `-L`/`--limit` to change) and `gh run list -w WORKFLOW` /
`gh run view RUN_ID -w WORKFLOW` to filter by workflow (name, ID, or filename). The doc text
fetched does not spell out UI-side filters by branch/event/actor beyond what the run list page
visually offers -- **not confirmed in the documentation** which exact filter chips exist in the
web UI as of today; treat the sidebar filter controls as the authoritative source, not this doc.

#### PT

Interface web: aba Actions -> selecionar o fluxo na barra lateral -> lista de execuções. CLI `gh`:
`gh run list` (limite padrão de 10 resultados, `-L`/`--limit` para mudar) e `gh run list -w
WORKFLOW` / `gh run view RUN_ID -w WORKFLOW` para filtrar por fluxo (nome, ID ou arquivo). O texto
buscado não detalha filtros de UI por branch/evento/ator além do que a página de lista oferece
visualmente -- **não confirmado na documentação** quais chips de filtro exatos existem na UI hoje;
tratar os controles de filtro da barra lateral como fonte de verdade, não este doc.

---

### 2.2 Job execution time and the visualization graph

**Sources:**
https://docs.github.com/pt/actions/how-tos/monitor-workflows/view-job-execution-time ·
https://docs.github.com/pt/actions/how-tos/monitor-workflows/use-the-visualization-graph

#### EN

Each run's summary page shows total job execution time; billable-minutes detail (with
multipliers, e.g. for larger runners) lives under "Usage" in the run's sidebar. **Per-step timing
is not documented as a summary-page feature** -- the fetched page only confirms total job time;
per-step duration is visible by expanding a step in the log view itself (timestamps prefix each
log line), not as a dedicated UI element.

The **visualization graph** (shown on the run summary) draws one node per job, with an icon to the
left of the job name indicating its status and lines between jobs indicating `needs:` dependency
order. Clicking a job jumps to its logs. The fetched page does not spell out the icon/color legend
(queued vs. in-progress vs. failed) -- **not confirmed in the documentation**; verify visually
against a known-good and a known-failed run rather than assuming a color meaning.

#### PT

A página de resumo de cada execução mostra o tempo total de execução do job; o detalhe de minutos
faturáveis (com multiplicadores, ex. para executores maiores) fica em "Uso" na barra lateral da
execução. **Tempo por step não é documentado como recurso da página de resumo** -- a página
buscada só confirma o tempo total do job; a duração por step é visível expandindo um step na
própria visualização de log (cada linha de log tem timestamp), não como elemento dedicado de UI.

O **gráfico de visualização** (mostrado no resumo da execução) desenha um nó por job, com um ícone
à esquerda do nome do job indicando seu status e linhas entre jobs indicando a ordem de dependência
`needs:`. Clicar num job pula para os logs dele. A página buscada não detalha a legenda de
ícones/cores (na fila vs. em andamento vs. com falha) -- **não confirmado na documentação**;
verificar visualmente contra uma execução boa conhecida e uma com falha conhecida em vez de supor
o significado de uma cor.

#### Tie-in / Amarração -- lição 7, a armadilha do painel que mente

**EN.** This is the direct backdrop for the **measured trap** from today (see section 3.5, step
5): the run's overview graph showed **four jobs as "queued"** while the log of the self-hosted
runner's own container said one of them had been **running for ten minutes**. The graph is a
GitHub-side summary computed from run/job state; when GitHub's own job-scheduling layer is the
thing misbehaving (as during today's outage), the summary itself can be stale or wrong. **The
visualization graph is a distant description of the work, not a report from inside it** -- treat
it as a hint, and confirm against the runner's own log (`journalctl`/container log for
self-hosted, or `gh run view --json jobs` polled directly) before concluding a job is stuck.

**PT.** Este é o pano de fundo direto da **armadilha medida** de hoje (ver seção 3.5, passo 5): o
gráfico de visão geral da execução mostrava **quatro jobs "na fila"** enquanto o log do próprio
contêiner do executor self-hosted dizia que um deles estava **rodando havia dez minutos**. O
gráfico é um resumo do lado do GitHub, computado a partir do estado da run/job; quando a própria
camada de agendamento do GitHub é a que está com defeito (como na pane de hoje), o resumo em si
pode estar obsoleto ou errado. **O gráfico de visualização é uma descrição de longe do trabalho,
não um relato de dentro dele** -- trate-o como pista, e confirme contra o log do próprio executor
(`journalctl`/log do contêiner para self-hosted, ou `gh run view --json jobs` sondado
diretamente) antes de concluir que um job está travado.

---

### 2.3 Status badges

**Source:** https://docs.github.com/pt/actions/how-tos/monitor-workflows/add-a-status-badge

#### EN

```markdown
![example workflow](https://github.com/OWNER/REPOSITORY/actions/workflows/WORKFLOW-FILE/badge.svg)
```

Optional query params: `?branch=NAME` (badge for a specific branch), `?event=push` (badge for a
specific triggering event). The UI can generate this markdown for you: Actions -> select workflow
-> "..." menu -> "Create status badge". **Badges of a private repository cannot be fetched
externally** (readme renderers outside GitHub's own auth context see nothing) -- fine for
`glintfx`, which is public.

#### PT

```markdown
![example workflow](https://github.com/OWNER/REPOSITORY/actions/workflows/WORKFLOW-FILE/badge.svg)
```

Parâmetros de consulta opcionais: `?branch=NOME` (badge de um branch específico), `?event=push`
(badge de um evento disparador específico). A UI pode gerar esse markdown: Actions -> selecionar o
fluxo -> menu "..." -> "Create status badge". **Badges de repositório privado não podem ser
buscados externamente** (renderizadores de README fora do contexto de autenticação do GitHub não
veem nada) -- sem problema para o `glintfx`, que é público.

#### Tie-in / Amarração

**EN.** The project's own memory (`feedback_github_only_saida_codeberg`) flags "badge de versão
congelado no README" as a known trap when migrating hosts. Same class of staleness risk applies to
a workflow badge: it points at a `WORKFLOW-FILE` by filename, so **renaming a workflow YAML file
breaks the badge silently** (404 on the badge image, not a build failure anyone notices). Worth a
one-line check next time any `.github/workflows/*.yml` is renamed.

**PT.** A própria memória do projeto (`feedback_github_only_saida_codeberg`) já marca "badge de
versão congelado no README" como armadilha conhecida ao migrar de host. A mesma classe de risco de
obsolescência vale para um badge de fluxo: ele aponta para um `WORKFLOW-FILE` pelo nome do arquivo,
então **renomear um YAML de fluxo quebra o badge em silêncio** (404 na imagem do badge, não uma
falha de build que alguém note). Vale um check de uma linha da próxima vez que qualquer
`.github/workflows/*.yml` for renomeado.

---

### 2.4 Notifications

**Source:** https://docs.github.com/pt/actions/concepts/workflows-and-actions/notifications-for-workflow-runs

#### EN

A notification fires when any workflow you triggered completes, covering all four terminal
outcomes: success, failure, neutral, cancelled. For scheduled workflows, notifications go to
whoever authored the current cron schedule (changing the cron syntax reassigns future
notifications to whoever made that edit; re-enabling a disabled workflow sends future notifications
to whoever re-enabled it). Preferences (email/web) can be scoped down to "only notify me when a
run fails" -- relevant for a project with a nightly sanitizer job nobody wants to hear about on
every green run.

#### PT

Uma notificação dispara quando qualquer fluxo que você disparou termina, cobrindo os quatro
desfechos terminais: sucesso, falha, neutro, cancelado. Para fluxos agendados, notificações vão a
quem for o autor do agendamento cron atual (mudar a sintaxe do cron reatribui notificações futuras
a quem fez a edição; reativar um fluxo desabilitado manda notificações futuras a quem o
reativou). Preferências (email/web) podem ser restringidas a "só me notifique quando uma execução
falhar" -- relevante para um projeto com job noturno de sanitizer que ninguém quer ouvir falar a
cada execução verde.

#### Tie-in / Amarração

**EN.** `nightly.yml`'s `sanitize` job is exactly the case the "only on failure" preference exists
for. **Not confirmed in the documentation** whether this preference is per-repository or global
to the GitHub account -- worth checking in the account's own notification settings before assuming
it can be scoped to just this repo's nightly run.

**PT.** O job `sanitize` do `nightly.yml` é exatamente o caso para o qual a preferência "só em
falha" existe. **Não confirmado na documentação** se essa preferência é por repositório ou global à
conta GitHub -- vale checar nas configurações de notificação da própria conta antes de supor que dá
para restringir só à execução noturna deste repo.

---

## 3. Diagnosing failures -- logs, debug logging, workflow commands, job summaries

### 3.1 Reading run logs

**Source:** https://docs.github.com/pt/actions/how-tos/monitor-workflows/use-workflow-run-logs

#### EN

Web UI navigation: Actions -> workflow -> run -> job (from the sidebar or the visualization graph).
**Failed steps auto-expand** -- you land already looking at the failing output, no manual
scrolling needed to find it in the common case. The log view has a search box ("Search logs", top
right of the log panel), but it **only searches expanded steps** -- a collapsed successful step is
invisible to search until you expand it. Download the full log via the gear/settings icon -> "Download
log file"; if the run had re-runs, the downloaded archive **only contains the re-run's jobs, not
the original attempt's**, unless you select the specific attempt first (`-a`/`--attempt` on `gh
run view`). Log **retention period is not confirmed in the fetched documentation** -- do not
assume logs are permanent; if a log needs to survive analysis, download it.

`gh` CLI equivalents:

```sh
gh run view RUN_ID --log                    # full log, all jobs
gh run view --job JOB_ID --log              # full log, one job
gh run view --job JOB_ID --log-failed       # only the failed steps' output
gh run view --job JOB_ID --log | grep error # pipe into your own filter
```

#### PT

Navegação na UI: Actions -> fluxo -> execução -> job (pela barra lateral ou pelo gráfico de
visualização). **Steps com falha se expandem automaticamente** -- você já cai olhando a saída que
falhou, sem precisar rolar manualmente para achá-la no caso comum. A visualização de log tem caixa
de busca ("Search logs", canto superior direito do painel de log), mas ela **só busca em steps
expandidos** -- um step bem-sucedido colapsado é invisível à busca até ser expandido. Baixe o log
completo pelo ícone de engrenagem/configurações -> "Baixar arquivo de log"; se a execução teve
re-execuções, o arquivo baixado **só contém os jobs da re-execução, não da tentativa original**, a
menos que você selecione a tentativa específica primeiro (`-a`/`--attempt` em `gh run view`).
**Período de retenção de log não confirmado na documentação buscada** -- não supor que logs são
permanentes; se um log precisa sobreviver à análise, baixe-o.

Equivalentes na CLI `gh`:

```sh
gh run view RUN_ID --log                    # log completo, todos os jobs
gh run view --job JOB_ID --log              # log completo, um job
gh run view --job JOB_ID --log-failed       # só a saída dos steps com falha
gh run view --job JOB_ID --log | grep error # canalizar para seu próprio filtro
```

---

### 3.2 Enabling diagnostic logging (`ACTIONS_STEP_DEBUG`, `ACTIONS_RUNNER_DEBUG`)

**Source:** https://docs.github.com/pt/actions/how-tos/monitor-workflows/enable-debug-logging

#### EN

Two distinct mechanisms, both enabled the same way (repository secret or variable named exactly
as below, value `true`; if both a secret and a variable with the same name exist, **the secret
wins**):

- **`ACTIONS_STEP_DEBUG`** -- increases verbosity of a job's own step logs (more debug-level
  events inside each step's output). This is "our code's" debug view.
- **`ACTIONS_RUNNER_DEBUG`** -- adds extra log files about **how the runner itself executed the
  job** (runner process log: coordination/configuration of runners; worker process log: execution
  of the job). This is the runner-infrastructure debug view.

Anyone with permission to run a workflow can turn on both for a **re-run** without touching repo
secrets (the "Enable debug logging" checkbox mentioned in section 3 above, or `gh run rerun
--debug`), which is the lighter-weight path for a one-off investigation versus setting a
persistent secret/variable.

#### PT

Dois mecanismos distintos, ambos ligados da mesma forma (secret ou variável do repositório com o
nome exato abaixo, valor `true`; se existirem secret e variável com o mesmo nome, **o secret
prevalece**):

- **`ACTIONS_STEP_DEBUG`** -- aumenta a verbosidade dos próprios logs de step de um job (mais
  eventos de nível debug dentro da saída de cada step). É a visão de debug do "nosso código".
- **`ACTIONS_RUNNER_DEBUG`** -- adiciona arquivos de log extras sobre **como o próprio executor
  rodou o job** (log de processo do runner: coordenação/configuração de runners; log de processo
  do worker: execução do job). É a visão de debug da infraestrutura do executor.

Qualquer pessoa com permissão para rodar um fluxo pode ligar os dois numa **re-execução** sem
mexer em secrets do repo (a caixa "Enable debug logging" mencionada na seção 3 acima, ou `gh run
rerun --debug`), que é o caminho mais leve para uma investigação pontual versus definir um
secret/variável persistente.

#### Tie-in / Amarração -- separates the two halves of lesson 7

**EN.** `ACTIONS_STEP_DEBUG` is the tool for "did **our** step do something unexpected"
(`CI-VERMELHO-2X`-class bugs: a bad compiler flag, a condition that evaluated wrong).
`ACTIONS_RUNNER_DEBUG` is the tool for "is the **runner/infrastructure** itself behaving oddly"
(the `Set up job` class of failure, and the self-hosted-runner container issues from
`CI-HEAVY-SEM-CACHE-DE-ACAO`). Reach for the matching one first instead of turning both on
reflexively -- `ACTIONS_RUNNER_DEBUG` output is noisy and mostly irrelevant when the bug is a
missing `#ifdef _WIN32` guard in a shared CMake option.

**PT.** `ACTIONS_STEP_DEBUG` é a ferramenta para "o **nosso** step fez algo inesperado" (classe
`CI-VERMELHO-2X`: uma flag de compilador errada, uma condição que avaliou errado).
`ACTIONS_RUNNER_DEBUG` é a ferramenta para "o **executor/infraestrutura** em si está se comportando
esquisito" (a classe `Set up job`, e os problemas de contêiner do executor self-hosted de
`CI-HEAVY-SEM-CACHE-DE-ACAO`). Buscar o que combina primeiro em vez de ligar os dois por reflexo --
a saída de `ACTIONS_RUNNER_DEBUG` é barulhenta e majoritariamente irrelevante quando o bug é uma
guarda `#ifdef _WIN32` faltando numa opção de CMake compartilhada.

---

### 3.3 Job condition (`if`) evaluation logs

**Source:** https://docs.github.com/pt/actions/how-tos/monitor-workflows/view-job-condition-logs

#### EN

When a **job-level** `if:` condition is evaluated, GitHub Actions logs the evaluation to help
explain both "why a job was skipped" and "why a job ran when you expected it to be skipped."
Access: run summary -> click the job -> gear/settings icon -> "Download log file" -> unzip ->
open `JOB-NAME/system.txt`. The file shows three lines: the original expression, the expanded
version with values substituted, and the final boolean result. **Documented limitation:**
expression logs exist **only for job-level `if:`** -- step-level condition evaluation is not
covered here; use `ACTIONS_STEP_DEBUG` for that instead.

#### PT

Quando a condição `if:` de nível **job** é avaliada, o GitHub Actions registra a avaliação para
ajudar a explicar tanto "por que um job foi pulado" quanto "por que um job rodou quando eu esperava
que fosse pulado." Acesso: resumo da execução -> clicar no job -> ícone de engrenagem/configurações
-> "Baixar arquivo de log" -> descompactar -> abrir `JOB-NAME/system.txt`. O arquivo mostra três
linhas: a expressão original, a versão expandida com valores substituídos, e o resultado booleano
final. **Limitação documentada:** logs de expressão existem **só para `if:` de nível job** --
avaliação de condição de step não é coberta aqui; use `ACTIONS_STEP_DEBUG` para isso.

#### Tie-in / Amarração

**EN.** The `troubleshoot-workflows` page (section 3.4) names a real footgun that lands exactly
here: using `always()` in a job/step condition **returns `true` even on cancellation**, so a job
guarded by `if: always()` runs during a cancel when the intent was "run even on failure, but not
on cancel" (`if: always() && !cancelled()` is the fix pattern, not confirmed as GitHub's exact
recommended wording but consistent with the documented `cancelled()` function). Worth checking
`system.txt` before assuming a job "ignored" a cancellation.

**PT.** A página `troubleshoot-workflows` (seção 3.4) nomeia uma armadilha real que cai exatamente
aqui: usar `always()` numa condição de job/step **retorna `true` mesmo em cancelamento**, então um
job protegido por `if: always()` roda durante um cancelamento quando a intenção era "rodar mesmo em
falha, mas não em cancelamento" (`if: always() && !cancelled()` é o padrão de correção, não
confirmado como a redação exata recomendada pelo GitHub mas consistente com a função `cancelled()`
documentada). Vale checar o `system.txt` antes de supor que um job "ignorou" um cancelamento.

---

### 3.4 The troubleshooting page proper

**Source:** https://docs.github.com/pt/actions/how-tos/troubleshoot-workflows

#### EN

Tools it points to: GitHub Copilot chat (opens with suggested next steps for the specific failure
being viewed), the run logs themselves, debug logging (section 3.2), and tool-specific verbosity
flags for the software *inside* your steps (`npm install --verbose`, `GIT_TRACE=1` for git -- i.e.
once you've confirmed the GitHub-side machinery is fine, turn up verbosity in the tool that's
actually failing).

Documented common trigger problems: a manually disabled workflow won't respond to events; some
events only fire on the **default branch** (`issues`, `schedule`); `pull_request` workflows
**won't run if the PR has a merge conflict**; commit messages with skip-CI annotations skip runs;
scheduled (`cron`) runs "can suffer delays during periods of high load."

**The infra-vs-user split the doc itself draws:**

| Infrastructure / runner-side | Workflow logic / our side |
|---|---|
| Network issues: DNS, firewalls, proxies, certificates | Misevaluated job conditions (verifiable via `system.txt`) |
| Misconfigured runner labels | Wrong filter syntax |
| IP addresses flagged by security scanners | Bad trigger configuration |
| Self-hosted runner problems | Conditional logic that blocks intended cancellation (the `always()` trap) |

#### PT

Ferramentas que a página aponta: chat do GitHub Copilot (abre com passos sugeridos para a falha
específica sendo vista), os próprios logs da execução, log de depuração (seção 3.2), e flags de
verbosidade específicas da ferramenta *dentro* dos seus steps (`npm install --verbose`,
`GIT_TRACE=1` para git -- ou seja, depois de confirmar que a maquinaria do lado do GitHub está bem,
aumente a verbosidade na ferramenta que está de fato falhando).

Problemas comuns de gatilho documentados: um fluxo desabilitado manualmente não responde a
eventos; alguns eventos só disparam no **branch padrão** (`issues`, `schedule`); fluxos de
`pull_request` **não rodam se a PR tiver conflito de merge**; mensagens de commit com anotação de
pular CI pulam execuções; execuções agendadas (`cron`) "podem sofrer atrasos em períodos de alta
carga."

**A divisão infra-vs-nosso que a própria doc traça:**

| Infraestrutura / lado do executor | Lógica do fluxo / nosso lado |
|---|---|
| Problemas de rede: DNS, firewalls, proxies, certificados | Condições de job mal avaliadas (verificável via `system.txt`) |
| Rótulos de executor mal configurados | Sintaxe de filtro errada |
| Endereços IP marcados por scanners de segurança | Configuração de gatilho inadequada |
| Problemas de executor self-hosted | Lógica condicional que bloqueia cancelamento pretendido (a armadilha `always()`) |

#### Tie-in / Amarração -- this table IS lesson 7's documentation-side confirmation

**EN.** This is the closest the official documentation comes to naming the exact distinction
lesson 7 is built on. It confirms the split is real and documented, but it does **not** give the
procedural "how do I tell which bucket I'm in without reading the whole log" answer -- that
procedure (using `Set up job` as the marker, `gh run view --json jobs`, exit-code ranges) is this
project's own contribution, not something copied from GitHub's docs. Section 3.5 below is that
procedure.

**PT.** Isto é o mais perto que a documentação oficial chega de nomear a distinção exata sobre a
qual a lição 7 é construída. Ela confirma que a divisão é real e documentada, mas **não** dá a
resposta procedimental de "como eu sei em qual balde estou sem ler o log inteiro" -- esse
procedimento (usar `Set up job` como marcador, `gh run view --json jobs`, faixas de código de
saída) é contribuição própria deste projeto, não algo copiado da documentação do GitHub. A seção
3.5 abaixo é esse procedimento.

---

### 3.5 The diagnostic procedure -- `Set up job` vs. a step of ours (lesson 7)

**EN.** This is the project's own procedure, assembled from the documentation above plus what was
measured live during the 2026-08-06 incident. It is deliberately written as an ordered checklist,
not prose, because that's how it was actually used under time pressure.

**PT.** Este é o procedimento próprio do projeto, montado a partir da documentação acima mais o que
foi medido ao vivo durante o incidente de 2026-08-06. Está escrito deliberadamente como checklist
ordenado, não em prosa, porque foi assim que foi de fato usado sob pressão de tempo.

#### Step 1 / Passo 1 -- find which step failed without reading the whole log

**EN.**

```sh
gh run view <run-id> --json jobs \
  --jq '.jobs[] | {name, status, conclusion, steps: [.steps[] | select(.conclusion=="failure") | {name, number, conclusion}]}'
```

This prints, per job: its own `status`/`conclusion`, and **only the steps whose `conclusion` is
`failure`** -- no log text, just names and step numbers. Cross-check with the web UI's
auto-expand behavior from section 3.1 (failed steps expand by themselves) as a second, independent
signal. If the failed step's `name` is literally **`Set up job`** (GitHub's own runner-provisioning
step, which appears before any step you wrote), stop here -- see step 4. Any other step name is
one of ours.

**PT.**

```sh
gh run view <run-id> --json jobs \
  --jq '.jobs[] | {name, status, conclusion, steps: [.steps[] | select(.conclusion=="failure") | {name, number, conclusion}]}'
```

Isso imprime, por job: o próprio `status`/`conclusion`, e **só os steps cuja `conclusion` é
`failure`** -- sem texto de log, só nomes e números de step. Cruze com o comportamento de
auto-expandir da UI web da seção 3.1 (steps com falha se expandem sozinhos) como segundo sinal
independente. Se o nome do step que falhou for literalmente **`Set up job`** (o próprio step de
provisionamento do executor pelo GitHub, que aparece antes de qualquer step que você escreveu),
pare aqui -- ver passo 4. Qualquer outro nome de step é nosso.

#### Step 2 / Passo 2 -- tool crash (exit >= 128) vs. real failure (exit 1)

**EN.** A Unix process killed by signal N reports exit code `128 + N` to its parent (POSIX/shell
convention, not a GitHub-specific rule -- **verify with `echo $?` locally if in doubt**, this is
general process-exit-code behavior, not documented on the GitHub Actions pages fetched for this
section). Two concrete examples measured this same day, in this same repo:

- **Exit 139** (`128 + 11`, `SIGSEGV`) -- `clang-tidy` 18.1.3 (from `ubuntu-latest`'s `apt`)
  segfaulting on a UTF-8 continuation byte inside a printf format string
  (`glintfx/src/rml/rcss_dump_corpus_sanity.cpp`, the word "nós"), a real upstream bug
  (`llvm/llvm-project#169198`). **This is a tool crash: the tool told us nothing about our code.**
  Action: work around the trigger (the per-file runner in `tools/run_clang_tidy_per_file.sh`
  isolates it to one file instead of losing the other 50), and/or track the upstream fix -- do
  **not** try to "fix" the source for a tool bug.
- **Exit 1** -- the same clang-tidy run, on other files, found 5 files that **genuinely** violate
  a configured check. **This is a real reprovation: the tool told us something true about our
  code.** Action: fix the code, or adjust the check configuration in `.clang-tidy` if the check
  itself is wrong for this codebase -- but that is a decision, not a given.

The two require **opposite actions** -- retrying/working around a crash makes sense; retrying a
real exit-1 failure without changing anything does not. Read the exit code before deciding which
branch you're in.

**PT.** Um processo Unix morto por um sinal N reporta código de saída `128 + N` ao processo-pai
(convenção POSIX/shell, não regra específica do GitHub -- **verifique com `echo $?` localmente se
tiver dúvida**, isso é comportamento geral de código de saída de processo, não documentado nas
páginas do GitHub Actions buscadas para esta seção). Dois exemplos concretos medidos neste mesmo
dia, neste mesmo repo:

- **Exit 139** (`128 + 11`, `SIGSEGV`) -- `clang-tidy` 18.1.3 (do `apt` do `ubuntu-latest`)
  segfaultando num byte de continuação UTF-8 dentro de uma string de formato printf
  (`glintfx/src/rml/rcss_dump_corpus_sanity.cpp`, a palavra "nós"), um bug real do upstream
  (`llvm/llvm-project#169198`). **Isto é crash de ferramenta: a ferramenta não nos disse nada sobre
  o nosso código.** Ação: contornar o gatilho (o runner por-arquivo em
  `tools/run_clang_tidy_per_file.sh` isola a um arquivo em vez de perder os outros 50), e/ou
  acompanhar o conserto do upstream -- **não** tentar "consertar" o fonte por um bug da ferramenta.
- **Exit 1** -- a mesma execução de clang-tidy, em outros arquivos, achou 5 arquivos que **de
  fato** violam um check configurado. **Isto é reprovação real: a ferramenta nos disse algo
  verdadeiro sobre o nosso código.** Ação: consertar o código, ou ajustar a configuração do check
  em `.clang-tidy` se o check em si estiver errado para esta base de código -- mas isso é decisão,
  não dado.

As duas exigem **ações opostas** -- retentar/contornar um crash faz sentido; retentar uma falha
real de exit 1 sem mudar nada não. Leia o código de saída antes de decidir em qual ramo você está.

#### Step 3 / Passo 3 -- did *any* of our steps run at all? (the pane signature)

**EN.** When the failure is `Set up job` itself, no step of ours ran, so the log contains **zero**
occurrences of any tool we invoke. This is exactly how today's outage was confirmed as
not-our-fault:

```sh
gh run view <run-id> --log | grep -ciE 'cmake|ctest|nasm|dnf|apt-get'
```

**Zero** hits means the run never got past provisioning -- there is nothing in our own pipeline to
fix, because our pipeline never started. A nonzero count means at least one of our steps executed,
so whatever failed did so *after* our own tooling had a chance to run, and the failure is more
likely ours (or an environmental issue reachable only once our steps are running, e.g. a flaky
network call inside a `cmake` step).

**PT.** Quando a falha é o próprio `Set up job`, nenhum step nosso rodou, então o log contém
**zero** ocorrências de qualquer ferramenta que invocamos. Foi exatamente assim que a pane de hoje
foi confirmada como não sendo nosso defeito:

```sh
gh run view <run-id> --log | grep -ciE 'cmake|ctest|nasm|dnf|apt-get'
```

**Zero** ocorrências significa que a execução nunca passou do provisionamento -- não há nada no
nosso próprio pipeline para consertar, porque o nosso pipeline nunca começou. Uma contagem
diferente de zero significa que pelo menos um step nosso executou, então o que falhou fez isso
*depois* de a nossa própria ferramentaria ter tido chance de rodar, e a falha é mais provavelmente
nossa (ou um problema ambiental só alcançável uma vez que nossos steps estejam rodando, ex. uma
chamada de rede instável dentro de um step de `cmake`).

#### Step 4 / Passo 4 -- when to re-run, and when it's pointless

**EN.**

| Situation | Re-run? |
|---|---|
| `Set up job` failed, zero of our tools ran (step 3 confirms) | **Yes** -- this is infrastructure; the same push will very likely succeed once GitHub's side recovers. Re-run the whole run (section 1.2) once the status page (section 5) shows the incident resolved, or via `workflow_dispatch` if `push` itself isn't creating runs (lesson 1). |
| Our step failed with exit 1 (real reprovation, step 2) | **No** -- re-running reproduces the identical failure every time (deterministic build/tests). Fix the code first. |
| Our step failed with exit >= 128 (tool crash, step 2), and the crash is data-dependent (e.g. one specific input file) | **Maybe, narrowly** -- re-running the exact same input reproduces the exact same crash (it's not usually flaky); what's worth re-running is a *fixed* invocation (workaround applied) against the same inputs, not a blind retry. |
| `gh run rerun --failed` returns an error | **Check `status` first** -- refused while the run is still `in_progress` (measured, lesson 4); wait for a terminal `status` (`completed`) before retrying the rerun command itself. |
| The run is stuck as `queued`/`in_progress` far past its usual wall-clock time | **Do not immediately assume "stuck"** -- see step 5 before concluding anything, then re-run only after cancelling (section 1.3) if truly wedged. |

**PT.**

| Situação | Re-executar? |
|---|---|
| `Set up job` falhou, zero das nossas ferramentas rodou (passo 3 confirma) | **Sim** -- é infraestrutura; o mesmo push muito provavelmente vai passar assim que o lado do GitHub se recuperar. Re-execute a execução inteira (seção 1.2) depois que a página de status (seção 5) mostrar o incidente resolvido, ou via `workflow_dispatch` se o próprio `push` não estiver criando execuções (lição 1). |
| Nosso step falhou com exit 1 (reprovação real, passo 2) | **Não** -- re-executar reproduz a falha idêntica sempre (build/testes determinísticos). Conserte o código primeiro. |
| Nosso step falhou com exit >= 128 (crash de ferramenta, passo 2), e o crash depende de dado (ex. um arquivo de entrada específico) | **Talvez, restrito** -- re-executar a mesma entrada exata reproduz o mesmo crash exato (geralmente não é instável); o que vale re-executar é uma invocação *consertada* (contorno aplicado) contra as mesmas entradas, não uma retentativa cega. |
| `gh run rerun --failed` retorna erro | **Cheque o `status` primeiro** -- é recusado enquanto a execução ainda está `in_progress` (medido, lição 4); espere um `status` terminal (`completed`) antes de retentar o comando de rerun em si. |
| A execução fica travada em `queued`/`in_progress` bem além do tempo de parede usual | **Não suponha "travado" de cara** -- ver passo 5 antes de concluir qualquer coisa, e só re-executar depois de cancelar (seção 1.3) se de fato estiver empacada. |

#### Step 5 / Passo 5 -- ⚠️ the panel lies; the thing doing the work does not

**EN.** Measured today: the GitHub run overview showed **four jobs as "queued"** for several
minutes. Reading only that panel, the natural conclusion was "the self-hosted runner is wedged."
The self-hosted runner's own **container log** (`journalctl --user -u <service>` /
`docker logs`/`podman logs` on the box hosting the ephemeral runner container -- see
`reference_ci_runner_local.md` for exact unit names in this project) told a different story: one
of those "queued" jobs had in fact been **running for ten minutes**. GitHub's own scheduling
summary was stale during the incident.

**Rule:** the source that describes the work *from a distance* (run overview graph, run history
list, `gh run list`, the status badge) can lag or misreport, especially during a platform-side
incident. The source that is *inside* the work (the runner's own process/container log for
self-hosted; `ACTIONS_RUNNER_DEBUG` output for GitHub-hosted, per section 3.2) is closer to ground
truth. **Where to look, by scenario:**

- **GitHub-hosted runner, suspected stuck job:** re-run with `ACTIONS_RUNNER_DEBUG` (section 3.2)
  to get the runner's own process/worker logs, rather than trusting the summary graph alone.
- **Self-hosted runner (this repo's `heavy.yml`):** go straight to the runner's own service/
  container log on the host machine -- it is the only source that can distinguish "the runner
  process is dead" from "the runner process is alive and working, but GitHub's dashboard hasn't
  caught up."
- **Either case:** poll `gh run view <run-id> --json jobs` a second time after a short wait and
  compare -- if the reported `status` hasn't moved at all across two polls spaced minutes apart
  *and* the runner's own log shows no activity either, that's a real stuck state, not a display
  lag.

**PT.** Medido hoje: a visão geral da execução no GitHub mostrava **quatro jobs "na fila"** por
vários minutos. Lendo só aquele painel, a conclusão natural era "o executor self-hosted está
empacado." O próprio **log do contêiner** do executor self-hosted (`journalctl --user -u <serviço>`
/ `docker logs`/`podman logs` na máquina que hospeda o contêiner efêmero do executor -- ver
`reference_ci_runner_local.md` para os nomes exatos de unit neste projeto) contava uma história
diferente: um daqueles jobs "na fila" na verdade estava **rodando havia dez minutos**. O próprio
resumo de agendamento do GitHub estava obsoleto durante o incidente.

**Regra:** a fonte que descreve o trabalho *de longe* (gráfico de visão geral da execução, lista de
histórico, `gh run list`, o badge de status) pode atrasar ou informar errado, especialmente durante
um incidente do lado da plataforma. A fonte que está *dentro* do trabalho (o log do próprio
processo/contêiner do executor para self-hosted; a saída de `ACTIONS_RUNNER_DEBUG` para executor
hospedado pelo GitHub, conforme seção 3.2) está mais perto da verdade de fato. **Onde olhar, por
cenário:**

- **Executor hospedado pelo GitHub, suspeita de job travado:** re-execute com
  `ACTIONS_RUNNER_DEBUG` (seção 3.2) para pegar os próprios logs de processo/worker do executor, em
  vez de confiar só no gráfico de resumo.
- **Executor self-hosted (o `heavy.yml` deste repo):** vá direto ao log do próprio
  serviço/contêiner do executor na máquina hospedeira -- é a única fonte capaz de distinguir "o
  processo do executor morreu" de "o processo do executor está vivo e trabalhando, mas o painel do
  GitHub não acompanhou."
- **Em qualquer caso:** sonde `gh run view <run-id> --json jobs` uma segunda vez depois de uma
  espera curta e compare -- se o `status` reportado não se moveu nada entre duas sondagens
  espaçadas por minutos **e** o próprio log do executor não mostra atividade, isso é um estado
  travado real, não um atraso de exibição.

---

### 3.6 Workflow commands (`::error`, `::warning`, `::notice`, `::group`, `::debug`) and `$GITHUB_STEP_SUMMARY`

**Source:** https://docs.github.com/pt/actions/reference/workflows-and-actions/workflow-commands

> Filed here (not in `secao-referencia.md`) because the orchestrator's brief for this section
> explicitly lists workflow commands and job summaries as part of "diagnosticar." Flagging the
> overlap with the reference section's normal ownership so the consolidator doesn't duplicate it.
>
> Registrado aqui (não em `secao-referencia.md`) porque o briefing do orquestrador para esta seção
> lista explicitamente comandos de fluxo e resumos de job como parte de "diagnosticar." Marcando a
> sobreposição com a posse normal da seção de referência para o consolidador não duplicar.

#### EN

```sh
echo "::error file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::warning file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::notice file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::group::{title}"
# ... output ...
echo "::endgroup::"
echo "::debug::{message}"   # only visible when ACTIONS_STEP_DEBUG is on (section 3.2)
```

`::error`/`::warning`/`::notice` all accept the same location parameters (`file`, `line`,
`endLine`, `col`, `endColumn`, `title`, all optional) and surface as **annotations** on the run
summary page and inline on the diff for the relevant file when the run belongs to a PR -- these
are what put a red squiggly-style marker directly at a file/line instead of forcing a log dive.
`::group`/`::endgroup` fold a range of log lines into a collapsible section in the raw log view.

`$GITHUB_STEP_SUMMARY` is a file path (env var), append GitHub-flavored Markdown to it and that
Markdown renders on the run summary page, per step:

```sh
echo "### Hello world :rocket:" >> "$GITHUB_STEP_SUMMARY"
echo "- bullet point" >> "$GITHUB_STEP_SUMMARY"
```

Limit: **1 MiB per step**; secrets are automatically masked in the rendered output.

#### PT

```sh
echo "::error file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::warning file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::notice file={name},line={line},endLine={endLine},col={col},endColumn={endColumn},title={title}::{message}"
echo "::group::{title}"
# ... saída ...
echo "::endgroup::"
echo "::debug::{message}"   # só visível com ACTIONS_STEP_DEBUG ligado (seção 3.2)
```

`::error`/`::warning`/`::notice` aceitam todos os mesmos parâmetros de localização (`file`,
`line`, `endLine`, `col`, `endColumn`, `title`, todos opcionais) e aparecem como **anotações** na
página de resumo da execução e inline no diff do arquivo relevante quando a execução pertence a uma
PR -- é isso que coloca um marcador tipo sublinhado vermelho direto num arquivo/linha em vez de
forçar um mergulho no log. `::group`/`::endgroup` dobram um intervalo de linhas de log numa seção
recolhível na visualização de log bruto.

`$GITHUB_STEP_SUMMARY` é um caminho de arquivo (variável de ambiente); anexe Markdown no sabor
GitHub e ele renderiza na página de resumo da execução, por step:

```sh
echo "### Olá mundo :rocket:" >> "$GITHUB_STEP_SUMMARY"
echo "- item de lista" >> "$GITHUB_STEP_SUMMARY"
```

Limite: **1 MiB por step**; secrets são automaticamente mascarados na saída renderizada.

#### Tie-in / Amarração -- lesson 2, and a concrete gap found while writing this doc

**EN.** `tools/run_clang_tidy_per_file.sh` (the fix for `CI-TIDY-CRASH`) already does the hard
part right: it separates `found`/`analyzed`/`failed` counts, distinguishes crash (exit >= 128)
from real violation (exit 1) per file, and **prints the summary even when the failure count is
zero** (`OK: $analyzed/$found_count arquivos analisados, 0 falhas`) -- exactly the discipline
lesson 2 demanded. **But it prints all of that with plain `echo` to stdout**, buried inside the
regular job log. `grep -rln GITHUB_STEP_SUMMARY .github/workflows/*.yml` and the same for
`::error`/`::warning`/`::group` inside the workflows and this script both return **nothing** --
this repo has never used a workflow command or a job summary, in any of the six workflows.
Concretely, this script's own summary block would be a clean fit for `$GITHUB_STEP_SUMMARY` (the
found/analyzed/failed table renders as a Markdown table on the run page, visible without opening
the log at all) and its per-crash and per-violation lines would be clean fits for `::error
file=...::` (jumps straight to an annotation instead of a log line). **Flagged as a concrete,
non-hypothetical recommendation for whoever next touches `tools/run_clang_tidy_per_file.sh` or
the `lint-and-scan` job in `ci.yml`** -- not implemented here, this section is documentation-only
per the brief's hard restriction ("não altere nenhum fluxo").

**PT.** `tools/run_clang_tidy_per_file.sh` (o conserto de `CI-TIDY-CRASH`) já acerta a parte
difícil: separa as contagens `found`/`analyzed`/`failed`, distingue crash (exit >= 128) de
violação real (exit 1) por arquivo, e **imprime o resumo mesmo quando a contagem de falhas é zero**
(`OK: $analyzed/$found_count arquivos analisados, 0 falhas`) -- exatamente a disciplina que a lição
2 exigiu. **Mas imprime tudo isso com `echo` simples para stdout**, enterrado dentro do log normal
do job. `grep -rln GITHUB_STEP_SUMMARY .github/workflows/*.yml` e o mesmo para
`::error`/`::warning`/`::group` dentro dos fluxos e deste script retornam **nada** -- este repo
nunca usou um comando de fluxo nem um resumo de job, em nenhum dos seis fluxos. Concretamente, o
próprio bloco de resumo deste script seria um encaixe limpo para `$GITHUB_STEP_SUMMARY` (a tabela
found/analyzed/failed renderiza como tabela Markdown na página da execução, visível sem abrir o log
de jeito nenhum) e suas linhas de crash e de violação seriam encaixes limpos para `::error
file=...::` (pula direto para uma anotação em vez de uma linha de log). **Sinalizado como
recomendação concreta, não hipotética, para quem mexer a seguir em
`tools/run_clang_tidy_per_file.sh` ou no job `lint-and-scan` de `ci.yml`** -- não implementado
aqui, esta seção é só documentação pela restrição dura do briefing ("não altere nenhum fluxo").

---

## 4. Quota, billing, and usage limits (where they affect whether a run happens at all)

**Sources:**
https://docs.github.com/pt/actions/concepts/billing-and-usage ·
https://docs.github.com/pt/actions/reference/limits ·
https://docs.github.com/pt/actions/how-tos/administer/view-metrics

#### EN

Every GitHub account gets a free quota of minutes and storage for GitHub-hosted runners,
according to the account's plan; usage beyond the included amount is billed. **The fetched
billing-and-usage page does not itself state the exact free-minute figures per plan or confirm
whether workflows simply stop running once the quota is exhausted** -- not confirmed in that page.
The **limits reference page** does give literal numbers relevant to "why didn't my workflow even
start":

| Limit | Value |
|---|---|
| Matrix jobs per run | 256 max |
| Concurrency-group queue (`queue: max`) | up to 100 pending runs |
| Queued runs | 500 per 10 seconds |
| Workflow run duration | 35 days max |
| Job duration, GitHub-hosted | 6 hours (360 min) max |
| Job duration, self-hosted | 5 days max |
| Self-hosted job queue before auto-cancel | 24 hours |
| Storage (Actions artifacts) | Free: 500 MB -- Enterprise: 50 GB (varies by plan) |
| Cache, per repository | 10 GB |
| Included minutes/month | Free: 2,000 -- Enterprise: 50,000 (varies by plan) |
| API rate limit, unauthenticated | 60/hour |
| API rate limit, `GITHUB_TOKEN` | 1,000/hour (15,000 on GHEC) |
| API rate limit, authenticated user | 5,000/hour (15,000 on GHEC) |
| Re-runs per workflow run | 50 max |
| Workflow trigger events | 1,500 per 10 seconds per repository |

`how-tos/administer/view-metrics` documents an organization/repository usage-metrics dashboard
(workflow/job counts and performance over configurable windows: weekly, monthly, 30/90 days,
annual, custom). **Not confirmed as restricted to large organizations** -- the how-to gives
instructions for both org-level and individual-repository scope, but in practice a project this
size (six workflows, one maintainer) is unlikely to hit any of the rate/queue limits above except
possibly the **6-hour job ceiling** if a job genuinely hangs (see section 1.6) or the
**re-run/attempt** ceilings during a prolonged incident like today's.

#### PT

Toda conta GitHub recebe uma cota gratuita de minutos e armazenamento para executores hospedados
pelo GitHub, de acordo com o plano da conta; uso além do incluído é cobrado. **A página de
billing-and-usage buscada não afirma em si os números exatos de minutos gratuitos por plano nem
confirma se os fluxos simplesmente param de rodar quando a cota se esgota** -- não confirmado
naquela página. A **página de referência de limites** dá números literais relevantes para "por que
meu fluxo nem começou":

| Limite | Valor |
|---|---|
| Jobs de matriz por execução | 256 no máximo |
| Fila de grupo de concorrência (`queue: max`) | até 100 execuções pendentes |
| Execuções enfileiradas | 500 a cada 10 segundos |
| Duração de execução de fluxo | 35 dias no máximo |
| Duração de job, hospedado pelo GitHub | 6 horas (360 min) no máximo |
| Duração de job, self-hosted | 5 dias no máximo |
| Fila de job self-hosted antes de auto-cancelar | 24 horas |
| Armazenamento (artefatos do Actions) | Free: 500 MB -- Enterprise: 50 GB (varia por plano) |
| Cache, por repositório | 10 GB |
| Minutos incluídos/mês | Free: 2.000 -- Enterprise: 50.000 (varia por plano) |
| Limite de taxa de API, não autenticado | 60/hora |
| Limite de taxa de API, `GITHUB_TOKEN` | 1.000/hora (15.000 no GHEC) |
| Limite de taxa de API, usuário autenticado | 5.000/hora (15.000 no GHEC) |
| Re-execuções por execução de fluxo | 50 no máximo |
| Eventos de gatilho de fluxo | 1.500 a cada 10 segundos por repositório |

`how-tos/administer/view-metrics` documenta um painel de métricas de uso em nível de
organização/repositório (contagens de fluxo/job e desempenho em janelas configuráveis: semanal,
mensal, 30/90 dias, anual, personalizada). **Não confirmado como restrito a organizações grandes**
-- o how-to dá instruções tanto para escopo de organização quanto de repositório individual, mas na
prática um projeto deste porte (seis fluxos, um mantenedor) dificilmente vai bater em qualquer um
dos limites de taxa/fila acima, exceto possivelmente o **teto de 6 horas por job** se um job
travar de verdade (ver seção 1.6) ou os tetos de **re-execução/tentativa** durante um incidente
prolongado como o de hoje.

#### Tie-in / Amarração -- lesson 4, why quota is a "did it even run" concern

**EN.** During today's outage, the concern was never "did we exhaust our minutes" -- it was "did
the platform accept the trigger at all" (`HTTP 422` from a missing `workflow_dispatch`, lesson 1).
The quota/limits material is filed under this diagnostic section specifically because a workflow
that silently never runs can have **either** cause (quota exhausted, or trigger rejected/platform
incident), and they require checking different places: billing usage page for quota, run history +
`gh run list` for "did a run even get created" (if no run object exists at all, it's not a job
failure to diagnose with section 3 -- it's a dispatch problem, back to section 1.1).

**PT.** Durante a pane de hoje, a preocupação nunca foi "esgotamos nossos minutos" -- foi "a
plataforma sequer aceitou o gatilho" (`HTTP 422` por `workflow_dispatch` faltando, lição 1). O
material de cota/limites está arquivado nesta seção de diagnóstico especificamente porque um fluxo
que silenciosamente nunca roda pode ter **qualquer uma** das duas causas (cota esgotada, ou gatilho
rejeitado/incidente de plataforma), e elas exigem checar lugares diferentes: página de uso de
cobrança para cota, histórico de execuções + `gh run list` para "uma execução sequer foi criada"
(se nenhum objeto de execução existe, não é falha de job para diagnosticar com a seção 3 -- é
problema de disparo, volta para a seção 1.1).

---

## 5. Service outages -- what GitHub documents, and what's left to the user

#### EN

**Not confirmed in `docs.github.com/actions`** -- the fetched navigation tree for this whole
section returned no page under `/pt/actions/` about platform-wide outages or how to distinguish
"my workflow is broken" from "GitHub itself is down." The documented answer lives **outside** the
Actions docs entirely: **https://www.githubstatus.com/**, GitHub's independent status page.

What that page offers (fetched live, 2026-08-06 -- coincidentally during this repo's own
`workflow_dispatch`-flagged incident window): per-component status across 11 tracked services
including **Actions** specifically; 90-day uptime history per component; an incident history
archive; and subscription channels (email, SMS, Slack integration, webhooks, Atom/RSS) so an
incident's start/resolution can be watched without polling the page by hand.

**What is left to the user, per everything else in this section:** GitHub does not document an
in-product way to tell "my run failed because of an outage" from "my run failed because of my own
code" -- that determination is exactly the procedure in section 3.5, built from evidence internal
to the run (`Set up job` failing, zero of our tools executing) rather than from an outage
announcement. The status page confirms a *suspicion*; it does not replace the diagnostic.

#### PT

**Não confirmado em `docs.github.com/actions`** -- a árvore de navegação buscada para esta seção
inteira não retornou nenhuma página sob `/pt/actions/` sobre indisponibilidade de plataforma ou
como distinguir "meu fluxo está quebrado" de "o próprio GitHub está fora do ar." A resposta
documentada mora **fora** da documentação do Actions por completo:
**https://www.githubstatus.com/**, a página de status independente do GitHub.

O que aquela página oferece (buscada ao vivo, 2026-08-06 -- coincidentemente durante a própria
janela de incidente sinalizada por `workflow_dispatch` deste repo): status por componente entre 11
serviços monitorados incluindo **Actions** especificamente; histórico de uptime de 90 dias por
componente; um arquivo de histórico de incidentes; e canais de assinatura (email, SMS, integração
com Slack, webhooks, Atom/RSS) para acompanhar início/resolução de um incidente sem ficar
atualizando a página manualmente.

**O que fica por conta do usuário, conforme tudo mais nesta seção:** o GitHub não documenta uma
forma dentro do próprio produto de distinguir "minha execução falhou por causa de um incidente" de
"minha execução falhou por causa do meu próprio código" -- essa determinação é exatamente o
procedimento da seção 3.5, construído a partir de evidência interna à execução (`Set up job`
falhando, zero das nossas ferramentas executando) em vez de a partir de um anúncio de incidente. A
página de status confirma uma *suspeita*; não substitui o diagnóstico.

---

## 6. Closing report (for the orchestrator / consolidator)

### 6.1 Pages actually read

**EN.** Counting only `docs.github.com/pt/actions/...` pages fetched in full (not the two external
references below):

1. `/pt/actions` (landing/nav page, used to enumerate this section's tree)
2. `how-tos/manage-workflow-runs/manually-run-a-workflow`
3. `how-tos/manage-workflow-runs/re-run-workflows-and-jobs`
4. `how-tos/manage-workflow-runs/cancel-a-workflow-run`
5. `how-tos/manage-workflow-runs/approve-runs-from-forks`
6. `reference/workflows-and-actions/workflow-cancellation`
7. `concepts/workflows-and-actions/concurrency`
8. `how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency`
9. `how-tos/monitor-workflows/view-workflow-run-history`
10. `how-tos/monitor-workflows/view-job-execution-time`
11. `how-tos/monitor-workflows/use-the-visualization-graph`
12. `how-tos/monitor-workflows/add-a-status-badge`
13. `concepts/workflows-and-actions/notifications-for-workflow-runs`
14. `how-tos/monitor-workflows/use-workflow-run-logs`
15. `how-tos/monitor-workflows/enable-debug-logging`
16. `how-tos/monitor-workflows/view-job-condition-logs`
17. `how-tos/troubleshoot-workflows`
18. `reference/workflows-and-actions/workflow-commands` (technically reference-section territory;
    read and used here because the brief explicitly assigned workflow commands + job summaries to
    this section -- flagged to the consolidator to avoid duplicating with `secao-referencia.md`)
19. `concepts/billing-and-usage`
20. `reference/limits`
21. `how-tos/administer/view-metrics`

**Total: 21 pages** under `docs.github.com/pt/actions/`, plus 2 external references read to
complete the diagnostic procedure and the outage section: `cli.github.com/manual/gh_run_view`
(GitHub CLI manual, needed for the exact `--json` field list used in section 3.5) and
`www.githubstatus.com` (GitHub's independent status page, section 5).

**Deliberately not read:** anything under "Migrating to GitHub Actions" (declared out of scope by
`CONTEXTO.md`), and anything in the reference/syntax, authoring/matrix, or security/self-hosted
trees that this brief explicitly assigns to the other three agents -- when a fetched page's
navigation surfaced one of those (e.g. `secao-referencia.md`'s territory), it was named but not
read in full here.

**PT.** Contando só páginas `docs.github.com/pt/actions/...` buscadas por completo (sem as duas
referências externas abaixo): **21 páginas** (lista idêntica à de cima), mais 2 referências
externas: o manual do `gh run view` na CLI do GitHub (necessário para a lista exata de campos
`--json` usada na seção 3.5) e a página de status independente do GitHub (seção 5).

**Deliberadamente não lido:** qualquer coisa sob "Migrar para o GitHub Actions" (declarado fora de
escopo por `CONTEXTO.md`), e qualquer coisa nas árvores de referência/sintaxe, autoria/matriz, ou
segurança/self-hosted que este briefing atribui explicitamente aos outros três agentes -- quando
uma página buscada revelou algo desse território (ex.: território de `secao-referencia.md`), foi
nomeado mas não lido por completo aqui.

### 6.2 Resources the documentation offers that this repo does NOT use

**EN.**

- **`$GITHUB_STEP_SUMMARY`** -- zero uses across all six workflows and all tooling scripts
  (verified: `grep -rln GITHUB_STEP_SUMMARY .github/workflows/*.yml` returns nothing). Concrete,
  non-hypothetical gap: `tools/run_clang_tidy_per_file.sh` already computes exactly the
  found/analyzed/failed summary this feature is built for, and prints it to a buried log instead
  (section 3.6).
- **`::error`/`::warning`/`::notice`/`::group`** workflow commands -- zero uses, same verification.
  Every diagnostic signal in this repo's CI today lives in plain log text, one log-scroll away
  from the automatic annotation surface GitHub already provides for free.
- **`concurrency:`** -- used in exactly 1 of 6 workflows (`heavy.yml`). `distro-matrix.yml` and
  `nightly.yml` are the two most minute-expensive workflows without it (section 1.5).
- **`timeout-minutes`** -- used in exactly 2 of 6 jobs (both in `heavy.yml`, both 120 min). The
  other four workflows run under the platform default of 360 minutes, ~3x longer than any of them
  should ever need (section 1.6).
- **Notification scoping ("only on failure")** -- not verified as configured for this account;
  the `nightly.yml` `sanitize` job is the textbook use case (section 2.4).
- **Org/repo usage-metrics dashboard** (`how-tos/administer/view-metrics`) -- not checked as part
  of this task; plausibly low-value at this project's current scale, but not verified either way.

**PT.**

- **`$GITHUB_STEP_SUMMARY`** -- zero usos em todos os seis fluxos e scripts de ferramentaria
  (verificado: `grep -rln GITHUB_STEP_SUMMARY .github/workflows/*.yml` não retorna nada). Lacuna
  concreta, não hipotética: `tools/run_clang_tidy_per_file.sh` já computa exatamente o resumo
  found/analyzed/failed para o qual este recurso foi feito, e imprime num log enterrado em vez
  disso (seção 3.6).
- **Comandos de fluxo `::error`/`::warning`/`::notice`/`::group`** -- zero usos, mesma verificação.
  Todo sinal de diagnóstico no CI deste repo hoje mora em texto de log puro, a uma rolagem de
  distância da superfície de anotação automática que o GitHub já oferece de graça.
- **`concurrency:`** -- usado em exatamente 1 dos 6 fluxos (`heavy.yml`). `distro-matrix.yml` e
  `nightly.yml` são os dois fluxos mais caros em minutos sem ele (seção 1.5).
- **`timeout-minutes`** -- usado em exatamente 2 dos 6 jobs (ambos em `heavy.yml`, ambos 120 min).
  Os outros quatro fluxos rodam sob o padrão da plataforma de 360 minutos, ~3x mais do que
  qualquer um deles deveria precisar (seção 1.6).
- **Restrição de notificação ("só em falha")** -- não verificado se está configurado para esta
  conta; o job `sanitize` do `nightly.yml` é o caso de livro-texto (seção 2.4).
- **Painel de métricas de uso org/repo** (`how-tos/administer/view-metrics`) -- não checado como
  parte desta tarefa; plausivelmente de baixo valor na escala atual do projeto, mas não verificado
  em nenhuma direção.

### 6.3 Contradictions between documentation and this repo's practice

**EN.** None found that rise to the level of "documentation says X, we measured not-X." The one
genuine gap is the inverse: the documentation describes the infra-vs-user failure split (section
3.4's table) but does **not** give a procedural answer for telling them apart quickly -- this
project had to build that procedure itself (section 3.5) from the pieces the docs do offer
(`gh run view --json jobs`, exit-code conventions that are general Unix behavior, not
GitHub-specific).

One near-contradiction worth flagging precisely as a **near**-contradiction, not a real one: the
run overview graph (section 2.2) is presented by the docs as the way to watch a run's progress,
but this project measured it showing stale "queued" state for a job that a lower-level log proved
was already running (section 3.5, step 5). This is not documentation being wrong -- nothing in the
docs claims the graph is real-time-guaranteed during a platform incident -- but it is a real trap
for anyone who takes "visualization graph" at face value as ground truth, and is exactly the kind
of thing `CONTEXTO.md` asked to be surfaced even without a clean textual contradiction to point at.

**PT.** Nenhuma que chegue ao nível de "a documentação diz X, medimos não-X." A única lacuna
genuína é o inverso: a documentação descreve a divisão de falha infra-vs-usuário (tabela da seção
3.4) mas **não** dá uma resposta procedimental para separá-las rápido -- este projeto teve que
construir esse procedimento sozinho (seção 3.5) a partir das peças que a doc de fato oferece
(`gh run view --json jobs`, convenções de código de saída que são comportamento Unix geral, não
específico do GitHub).

Uma quase-contradição vale sinalizar precisamente como **quase**-contradição, não uma real: o
gráfico de visão geral da execução (seção 2.2) é apresentado pela doc como a forma de acompanhar o
progresso de uma execução, mas este projeto mediu ele mostrando estado "na fila" obsoleto para um
job que um log de nível mais baixo provou já estar rodando (seção 3.5, passo 5). Isso não é a
documentação estar errada -- nada na doc afirma que o gráfico é garantidamente tempo-real durante um
incidente de plataforma -- mas é uma armadilha real para quem toma o "gráfico de visualização" pelo
valor de face como verdade de fato, e é exatamente o tipo de coisa que `CONTEXTO.md` pediu para
trazer à tona mesmo sem uma contradição textual limpa para apontar.
