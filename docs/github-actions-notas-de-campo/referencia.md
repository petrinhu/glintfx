# GitHub Actions -- Reference Section (workflow syntax, contexts, expressions, variables, events)

> STATUS: IN PROGRESS -- gravação incremental (página lida = página registrada). Consolidação final em `docs/github-actions.md` fica para depois, sob coordenação do orquestrador. Autor: `devops-sre`, fatia `DOC-GHA-CANONICO`.
>
> Escopo desta seção (atribuído pelo orquestrador): sintaxe de workflow (`on:`, `jobs:`, `steps:`, `strategy`, `permissions`, `concurrency`, `defaults`, `env`), contextos, expressões e funções, variáveis, referência de eventos que disparam fluxos.
>
> Denominador de páginas lidas nesta seção (atualizado ao final): ver rodapé.

---

## 1. Workflow syntax (`on:`, `jobs:`, `steps:`, `strategy`, `permissions`, `concurrency`, `defaults`, `env`)

**Source / Fonte:** https://docs.github.com/pt/actions/reference/workflows-and-actions/workflow-syntax (fetched via WebFetch tool; the tool's extraction below reflects the page's normative content -- see caveat in section 6 about WebFetch reliability).

### EN

#### Top-level keys

- **`name`** -- workflow display name in the Actions tab. If omitted, GitHub shows the file path relative to repo root.
- **`run-name`** -- display name for an individual *run* (not the workflow itself). Supports expressions with the `github` and `inputs` contexts. Example: `run-name: Deploy to ${{ inputs.deploy_target }} by @${{ github.actor }}`.
- **`on`** -- the event(s) that trigger the workflow. Accepts a single event (`on: push`), a list (`on: [push, pull_request]`), or a map with per-event configuration (activity `types`, filters, schedule, `workflow_call`/`workflow_dispatch` inputs). See section 5 (Events) for the full reference.
- **`permissions`** -- controls the `GITHUB_TOKEN`'s scopes, at workflow level (applies to every job unless overridden) or job level.
- **`env`** -- environment variables available to every job/step in the workflow. **Cannot reference other variables defined in the same map** (no `${{ env.A }}` inside the definition of `env.B` in the same block).
- **`defaults` / `defaults.run`** -- default `shell` and `working-directory` for every `run:` step in the workflow; job-level `defaults` overrides workflow-level.
- **`concurrency`** -- see subsection below.
- **`jobs`** -- the container for one or more jobs; jobs run in parallel by default, `needs:` creates dependency order.

#### `permissions` -- available scopes

| Scope | Purpose |
|---|---|
| `actions` | Manage GitHub Actions (workflow runs, artifacts) |
| `artifact-metadata` | Work with artifact metadata |
| `attestations` | Create artifact attestations |
| `checks` | Read/write check runs |
| `code-quality` | Upload code quality reports |
| `contents` | Access repository content |
| `deployments` | Manage deployments |
| `discussions` | Manage discussions |
| `id-token` | Retrieve OIDC tokens (**write-only** -- there is no meaningful `read`) |
| `issues` | Read/write issues |
| `packages` | Publish packages |
| `pages` | Manage GitHub Pages |
| `pull-requests` | Read/write pull requests |
| `security-events` | Read/write code scanning alerts |
| `statuses` | Read commit statuses |
| `vulnerability-alerts` | Read Dependabot alerts (read-only) |

Access levels: `read`, `write`, `none`. Shorthands: `permissions: read-all`, `permissions: write-all`, `permissions: {}` (nothing). Least-privilege pattern: declare only the scopes a job actually needs, at job level, not workflow level, when jobs differ in what they touch.

```yaml
permissions:
  contents: read
  pull-requests: write
  issues: none
```

#### `concurrency`

Ensures only one run/job per *concurrency group* proceeds at a time; useful to cancel a stale run when a newer commit lands on the same branch/PR.

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

- `group` -- string (supports expressions with `github`, `inputs`, `vars` contexts); group name is case-insensitive.
- `cancel-in-progress` -- boolean, default `false`. `true` cancels any run/job already in progress in the same group.
- `queue` -- `single` (default, one pending job per group) or `max` (up to 100 pending). **`cancel-in-progress: true` and `queue: max` are mutually exclusive.**

#### `defaults.run`

```yaml
defaults:
  run:
    shell: bash
    working-directory: ./scripts
```

Available `shell` values: `bash` (Linux/macOS default), `pwsh`, `python`, `sh` (Linux/macOS fallback), `cmd` (Windows), `powershell` (PowerShell Desktop, Windows).

#### `jobs.<job_id>`

Key fields, each overriding the workflow-level equivalent when present:

- **`runs-on`** -- string or array. GitHub-hosted labels (`ubuntu-latest`, `windows-latest`, `macos-latest`, pinned versions like `ubuntu-22.04`) or self-hosted labels (`self-hosted`, plus custom labels as an array, e.g. `[self-hosted, linux, x64]`).
- **`needs`** -- job id or array of job ids this job waits for (must complete successfully unless `if:` explicitly checks failure).
- **`if`** -- conditional expression; job only runs if it evaluates truthy.
- **`permissions`** -- job-level override of `GITHUB_TOKEN` scopes.
- **`environment`** -- deployment environment name (+ optional `url`), ties into environment protection rules and environment-scoped secrets.
- **`concurrency`** -- job-level concurrency group, same syntax as workflow-level.
- **`outputs`** -- map exposing step outputs to downstream jobs (`needs:`-linked), read via `${{ jobs.<job_id>.outputs.<name> }}`.
- **`env`** -- job-level environment variables, override workflow-level.
- **`defaults`** -- job-level `run` defaults, override workflow-level.
- **`steps`** -- ordered list of tasks; each step is either `run:` (shell) or `uses:` (action).
- **`timeout-minutes`** -- job wall-clock cap; **default 360 minutes (6 hours)** if unset.
- **`strategy`** -- see below.
- **`continue-on-error`** -- job reports failure but does not fail the overall run.
- **`container`** -- run the job's steps inside a Docker container (`image`, `options`, `env`, `ports`, `volumes`).
- **`services`** -- sidecar containers (databases, etc.) reachable by the job's steps via hostname = service id.

#### `strategy` (matrix)

```yaml
strategy:
  fail-fast: false
  max-parallel: 2
  matrix:
    os: [ubuntu-latest, windows-latest]
    node-version: [16, 18, 20]
    include:
      - os: macos-latest
        node-version: 18
    exclude:
      - os: windows-latest
        node-version: 16
```

- `matrix` -- cross-product of the listed axes generates one job instance per combination (`${{ matrix.os }}`, `${{ matrix.node-version }}` accessible inside that job). `include`/`exclude` adjust the generated set.
- `fail-fast` -- boolean, **default `true`**: any matrix-job failure cancels all still-running/pending matrix jobs. Setting it `false` lets every leg run to completion independently.
- `max-parallel` -- caps how many matrix jobs run simultaneously.

#### `jobs.<job_id>.steps[*]`

- **`id`** -- step identifier, used to reference its `outputs` from later steps (`${{ steps.<id>.outputs.<name> }}`) or check its `outcome`/`conclusion`.
- **`if`** -- conditional; supports the status-check functions `success()`, `failure()`, `cancelled()`, `always()` in addition to boolean expressions.
- **`name`** -- display name in the UI.
- **`uses`** -- run an action, referenced as `{owner}/{repo}@{ref}` or `{owner}/{repo}/{path}@{ref}`; `ref` can be a tag, branch, or full commit SHA; `uses: ./local-action` for a same-repo action; `uses: docker://image` for a raw container.
- **`run`** -- shell command(s); multi-line via YAML block scalar (`|`).
- **`working-directory`** -- overrides the step's CWD.
- **`shell`** -- overrides the effective shell for this `run:` step only.
- **`with`** -- input parameters passed to the action referenced by `uses:`.
- **`env`** -- step-level environment variables, override job/workflow-level for this step only.
- **`continue-on-error`** -- step reports failure but the job keeps going.
- **`timeout-minutes`** -- per-step wall-clock cap.

#### Specificity hierarchy

Step-level settings (`env`, `shell`, `working-directory`, `defaults` do not exist at step level but its equivalents do) override job-level, which override workflow-level. Same pattern for `permissions`, `env`, `defaults`, `concurrency`, `timeout-minutes`.

### PT

#### Chaves de nível superior

- **`name`** -- nome de exibição do workflow na aba Actions. Se omitido, o GitHub mostra o caminho do arquivo relativo à raiz do repo.
- **`run-name`** -- nome de exibição de uma *execução individual* (não do workflow em si). Aceita expressões com os contextos `github` e `inputs`. Exemplo: `run-name: Deploy to ${{ inputs.deploy_target }} by @${{ github.actor }}`.
- **`on`** -- o(s) evento(s) que dispara(m) o workflow. Aceita um evento único (`on: push`), uma lista (`on: [push, pull_request]`), ou um mapa com configuração por evento (`types` de atividade, filtros, agendamento, entradas de `workflow_call`/`workflow_dispatch`). Ver seção 5 (Eventos) para a referência completa.
- **`permissions`** -- controla os escopos do `GITHUB_TOKEN`, em nível de workflow (vale para todo job salvo sobrescrita) ou de job.
- **`env`** -- variáveis de ambiente disponíveis para todo job/step do workflow. **Não pode referenciar outra variável definida no mesmo mapa** (nada de `${{ env.A }}` dentro da definição de `env.B` no mesmo bloco).
- **`defaults` / `defaults.run`** -- `shell` e `working-directory` padrão para todo step `run:` do workflow; `defaults` de nível de job sobrescreve o de nível de workflow.
- **`concurrency`** -- ver subseção abaixo.
- **`jobs`** -- o container de um ou mais jobs; jobs rodam em paralelo por padrão, `needs:` cria ordem de dependência.

#### `permissions` -- escopos disponíveis

| Escopo | Finalidade |
|---|---|
| `actions` | Gerenciar GitHub Actions (execuções de workflow, artefatos) |
| `artifact-metadata` | Trabalhar com metadados de artefato |
| `attestations` | Criar atestados de artefato |
| `checks` | Ler/escrever check runs |
| `code-quality` | Publicar relatórios de qualidade de código |
| `contents` | Acessar conteúdo do repositório |
| `deployments` | Gerenciar deployments |
| `discussions` | Gerenciar discussões |
| `id-token` | Obter tokens OIDC (**só write** -- não existe `read` significativo) |
| `issues` | Ler/escrever issues |
| `packages` | Publicar pacotes |
| `pages` | Gerenciar GitHub Pages |
| `pull-requests` | Ler/escrever pull requests |
| `security-events` | Ler/escrever alertas de code scanning |
| `statuses` | Ler status de commit |
| `vulnerability-alerts` | Ler alertas do Dependabot (só leitura) |

Níveis de acesso: `read`, `write`, `none`. Atalhos: `permissions: read-all`, `permissions: write-all`, `permissions: {}` (nada). Padrão de menor privilégio: declarar só os escopos que aquele job de fato usa, em nível de JOB, não de workflow, quando os jobs diferem no que tocam.

```yaml
permissions:
  contents: read
  pull-requests: write
  issues: none
```

#### `concurrency`

Garante que só uma execução/job por *grupo de concorrência* prossiga por vez; útil para cancelar uma execução obsoleta quando um commit mais novo chega no mesmo branch/PR.

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

- `group` -- string (aceita expressões com os contextos `github`, `inputs`, `vars`); o nome do grupo não diferencia maiúsculas/minúsculas.
- `cancel-in-progress` -- booleano, padrão `false`. `true` cancela qualquer execução/job já em andamento no mesmo grupo.
- `queue` -- `single` (padrão, um job pendente por grupo) ou `max` (até 100 pendentes). **`cancel-in-progress: true` e `queue: max` são mutuamente exclusivos.**

#### `defaults.run`

```yaml
defaults:
  run:
    shell: bash
    working-directory: ./scripts
```

Valores de `shell` disponíveis: `bash` (padrão Linux/macOS), `pwsh`, `python`, `sh` (fallback Linux/macOS), `cmd` (Windows), `powershell` (PowerShell Desktop, Windows).

#### `jobs.<job_id>`

Campos principais, cada um sobrescrevendo o equivalente de nível de workflow quando presente:

- **`runs-on`** -- string ou array. Labels hospedados pelo GitHub (`ubuntu-latest`, `windows-latest`, `macos-latest`, versões pinadas como `ubuntu-22.04`) ou self-hosted (`self-hosted`, mais labels customizadas como array, ex.: `[self-hosted, linux, x64]`).
- **`needs`** -- id de job ou array de ids que este job espera (precisam concluir com sucesso, salvo `if:` que checa explicitamente falha).
- **`if`** -- expressão condicional; o job só roda se avaliar verdadeiro.
- **`permissions`** -- sobrescrita de nível de job dos escopos do `GITHUB_TOKEN`.
- **`environment`** -- nome do ambiente de deployment (+ `url` opcional), amarra a regras de proteção de ambiente e secrets escopados por ambiente.
- **`concurrency`** -- grupo de concorrência de nível de job, mesma sintaxe do nível de workflow.
- **`outputs`** -- mapa que expõe saídas de step para jobs downstream (ligados via `needs:`), lidos via `${{ jobs.<job_id>.outputs.<nome> }}`.
- **`env`** -- variáveis de ambiente de nível de job, sobrescrevem as de nível de workflow.
- **`defaults`** -- padrões de `run` de nível de job, sobrescrevem os de nível de workflow.
- **`steps`** -- lista ordenada de tarefas; cada step é `run:` (shell) ou `uses:` (action).
- **`timeout-minutes`** -- teto de tempo-relógio do job; **padrão 360 minutos (6 horas)** se não definido.
- **`strategy`** -- ver abaixo.
- **`continue-on-error`** -- o job reporta falha mas não derruba a execução geral.
- **`container`** -- roda os steps do job dentro de um container Docker (`image`, `options`, `env`, `ports`, `volumes`).
- **`services`** -- containers auxiliares (bancos de dados, etc.) alcançáveis pelos steps do job pelo hostname = id do serviço.

#### `strategy` (matrix)

```yaml
strategy:
  fail-fast: false
  max-parallel: 2
  matrix:
    os: [ubuntu-latest, windows-latest]
    node-version: [16, 18, 20]
    include:
      - os: macos-latest
        node-version: 18
    exclude:
      - os: windows-latest
        node-version: 16
```

- `matrix` -- o produto cartesiano dos eixos listados gera uma instância de job por combinação (`${{ matrix.os }}`, `${{ matrix.node-version }}` acessíveis dentro daquele job). `include`/`exclude` ajustam o conjunto gerado.
- `fail-fast` -- booleano, **padrão `true`**: qualquer falha de job da matriz cancela todos os jobs da matriz ainda em andamento/pendentes. Colocar `false` deixa cada perna rodar até o fim de forma independente.
- `max-parallel` -- limita quantos jobs da matriz rodam simultaneamente.

#### `jobs.<job_id>.steps[*]`

- **`id`** -- identificador do step, usado para referenciar seus `outputs` de steps posteriores (`${{ steps.<id>.outputs.<nome> }}`) ou checar seu `outcome`/`conclusion`.
- **`if`** -- condicional; aceita as funções de status `success()`, `failure()`, `cancelled()`, `always()` além de expressões booleanas.
- **`name`** -- nome de exibição na UI.
- **`uses`** -- roda uma action, referenciada como `{owner}/{repo}@{ref}` ou `{owner}/{repo}/{path}@{ref}`; `ref` pode ser tag, branch, ou SHA de commit completo; `uses: ./local-action` para action no mesmo repo; `uses: docker://imagem` para container cru.
- **`run`** -- comando(s) de shell; múltiplas linhas via bloco YAML (`|`).
- **`working-directory`** -- sobrescreve o CWD do step.
- **`shell`** -- sobrescreve o shell efetivo só para este step `run:`.
- **`with`** -- parâmetros de entrada passados à action referenciada por `uses:`.
- **`env`** -- variáveis de ambiente de nível de step, sobrescrevem job/workflow só para este step.
- **`continue-on-error`** -- o step reporta falha mas o job continua.
- **`timeout-minutes`** -- teto de tempo-relógio por step.

#### Hierarquia de especificidade

Configuração de nível de step (`env`, `shell`, `working-directory`) sobrescreve a de nível de job, que sobrescreve a de nível de workflow. Mesmo padrão para `permissions`, `env`, `defaults`, `concurrency`, `timeout-minutes`.

---

## 2. Events that trigger workflows / Eventos que disparam workflows

**Source / Fonte:** https://docs.github.com/pt/actions/reference/workflows-and-actions/events-that-trigger-workflows (fetched via WebFetch).

### EN

- **`workflow_dispatch`** -- manual trigger (UI, `gh` CLI, or REST API). Max 25 top-level `inputs`, max 65 535 characters of total input content. **"This event will only trigger a workflow run if the workflow file exists on the default branch"** -- i.e. the `workflow_dispatch:` trigger itself must already be present on the branch GitHub reads as default (here, `main`) before `gh workflow run` (or the API) can create a run for it, though the *run* itself can target a different `ref`.
- **`push`** -- fires on commit push, tag push, or repo creation from a template. **Not created above 5 000 branches pushed simultaneously; not created for tags when more than 3 tags are pushed at once.** Supports `branches`/`branches-ignore`, `tags`/`tags-ignore`, `paths`/`paths-ignore` filters (see workflow-syntax page, section 1).
- **`pull_request`** -- **`GITHUB_TOKEN` has read-only permissions on pull requests from forked repositories.** Does not fire on the fork itself; events are delivered to the *base* repository. Does not fire while the PR has a merge conflict. Payload is empty for merged PRs and for PRs from forked repos. First-time contributors on a public repo may need a maintainer with write access to approve the run.
- **`pull_request_target`** -- runs **in the context of the base repository's default branch**, not the merge commit, unlike `pull_request`. Documented rationale: **"this prevents unsafe execution of pull request code that could alter your repository or steal secrets."** Explicit warning: **"running untrusted code with the `pull_request_target` trigger can lead to security vulnerabilities. These vulnerabilities include cache poisoning and unintended access to secrets or write privileges."**
- Secrets other than `GITHUB_TOKEN` **are not passed to the runner when a workflow is triggered from a forked repository.** Dependabot PRs are treated as fork PRs for this purpose too.
- **`schedule`** -- POSIX cron, 5-field syntax, optional IANA `timezone`. Minimum interval **5 minutes**. **On public repositories, "scheduled workflows are automatically disabled when no repository activity has occurred in 60 days."** Re-enabled automatically if a user with write permission commits a change to the cron syntax. Can be delayed under high GitHub Actions load. DST edge case: a schedule that would fall in a "spring forward" gap runs at the next valid time instead.
- **`workflow_call`** -- marks a workflow as reusable, callable from another workflow; the called workflow receives the same event payload as the calling workflow.
- Full event list documented on the page: `branch_protection_rule`, `check_run`, `check_suite`, `create`, `delete`, `deployment`, `deployment_status`, `discussion`, `discussion_comment`, `fork`, `gollum`, `image_version`, `issue_comment`, `issues`, `label`, `merge_group`, `milestone`, `page_build`, `public`, `pull_request`, `pull_request_review`, `pull_request_review_comment`, `pull_request_target`, `push`, `registry_package`, `release`, `repository_dispatch`, `schedule`, `status`, `watch`, `workflow_call`, `workflow_dispatch`, `workflow_run`.
- **Não confirmado nesta página especificamente:** o texto explícito "self-hosted runner + `pull_request` de fork = execução de código arbitrário no host" **não apareceu** na extração desta página de eventos -- esse aviso vive na seção de **Segurança** da documentação (`reference/security`), fora do escopo desta seção (ver nota do orquestrador; outro agente cobre segurança). Fica registrado aqui como lacuna de cobertura, não como afirmação inventada.

### PT

- **`workflow_dispatch`** -- disparo manual (UI, CLI `gh`, ou API REST). Máx. 25 `inputs` de nível superior, máx. 65.535 caracteres de conteúdo total de entrada. **"Esse evento vai disparar apenas um fluxo de trabalho executado se o arquivo de fluxo de trabalho existe no branch padrão"** -- ou seja, o próprio gatilho `workflow_dispatch:` precisa já existir no branch que o GitHub lê como padrão (aqui, `main`) antes que `gh workflow run` (ou a API) consiga criar uma execução para ele, embora a *execução* em si possa mirar outra `ref`.
- **`push`** -- dispara em push de commit, push de tag, ou criação de repo a partir de template. **Não é criado acima de 5.000 branches enviadas simultaneamente; não é criado para tags quando mais de 3 tags são enviadas de uma vez.** Aceita filtros `branches`/`branches-ignore`, `tags`/`tags-ignore`, `paths`/`paths-ignore` (ver página de sintaxe, seção 1).
- **`pull_request`** -- **o `GITHUB_TOKEN` tem permissões só-leitura em pull requests de repositórios com fork.** Não dispara no fork em si; os eventos são entregues ao repositório BASE. Não dispara enquanto o PR tem conflito de merge. A carga vem vazia para PRs mesclados e PRs de fork. Contribuidor de primeira vez num repo público pode precisar de aprovação de um mantenedor com write.
- **`pull_request_target`** -- roda **no contexto do branch padrão do repositório base**, não do commit de merge, diferente do `pull_request`. Racional documentado: **"isso impede a execução insegura de código de pull request que poderia alterar seu repositório ou roubar segredos."** Aviso explícito: **"executar código não confiável com o gatilho `pull_request_target` pode levar a vulnerabilidades de segurança. Essas vulnerabilidades incluem envenenamento de cache e acesso não intencional a segredos ou privilégios de escrita."**
- Segredos além do `GITHUB_TOKEN` **não são passados ao runner quando um workflow é disparado por um repositório com fork.** PRs do Dependabot são tratados como PR de fork para este efeito também.
- **`schedule`** -- cron POSIX, sintaxe de 5 campos, `timezone` IANA opcional. Intervalo mínimo **5 minutos**. **Em repositórios públicos, "fluxos de trabalho agendados são automaticamente desabilitados quando nenhuma atividade do repositório ocorreu em 60 dias."** Reativado automaticamente se um usuário com permissão de escrita commitar mudança na sintaxe cron. Pode atrasar sob carga alta do GitHub Actions. Caso de borda de horário de verão: agendamento que cairia num intervalo "adiantado" roda no próximo horário válido.
- **`workflow_call`** -- marca um workflow como reutilizável, chamável por outro workflow; o workflow chamado recebe a mesma carga de evento do workflow chamador.
- Lista completa de eventos documentada na página: `branch_protection_rule`, `check_run`, `check_suite`, `create`, `delete`, `deployment`, `deployment_status`, `discussion`, `discussion_comment`, `fork`, `gollum`, `image_version`, `issue_comment`, `issues`, `label`, `merge_group`, `milestone`, `page_build`, `public`, `pull_request`, `pull_request_review`, `pull_request_review_comment`, `pull_request_target`, `push`, `registry_package`, `release`, `repository_dispatch`, `schedule`, `status`, `watch`, `workflow_call`, `workflow_dispatch`, `workflow_run`.
- **Não confirmado nesta página especificamente:** o texto explícito "runner self-hosted + `pull_request` de fork = execução de código arbitrário no host" **não apareceu** na extração desta página de eventos -- esse aviso vive na seção de **Segurança** da documentação (`reference/security`), fora do escopo desta seção (ver nota do orquestrador; outro agente cobre segurança). Fica registrado aqui como lacuna de cobertura, não como afirmação inventada.

### Como isto se aplica aqui / How this applies here

**Os 6 fluxos deste projeto** (`.github/workflows/`): `ci.yml` (gate de release, hosted `ubuntu-latest`), `core-ci.yml` (Camada 0, leve, hosted), `nightly.yml` (rede de segurança ASan, cron), `windows-atoms.yml` (MSVC dedicado, hosted), `distro-matrix.yml` (Arch/Fedora44/Ubuntu via `container:`, hosted), `heavy.yml` (ASan+fonteng pesado, self-hosted conteinerizado).

- **Lição CI-DISPATCH-MANUAL, amarrada à frase exata da doc:** o texto **"esse evento vai disparar apenas um fluxo de trabalho executado se o arquivo de fluxo de trabalho existe no branch padrão"** explica POR QUE o conserto (`75ad1ba`) precisou ser um `push` normal antes de `gh workflow run` funcionar nos três fluxos que ganharam o gatilho -- o `workflow_dispatch:` só passa a existir "para o GitHub" depois que o arquivo com essa chave chega ao `main`. Antes do conserto, `ci.yml` (job `build-and-test`, `.github/workflows/ci.yml:68`), `core-ci.yml` (`:93`) e `windows-atoms.yml` (`:64`) devolviam `HTTP 422` a qualquer tentativa de `gh workflow run`. Hoje os 6 arquivos têm `workflow_dispatch:` (confirmado nesta fatia por grep direto).
- **`heavy.yml` é o único a citar EXPLICITAMENTE o risco de `pull_request` + self-hosted em repositório público** -- não porque a página de eventos o diga (não diz, ver "não confirmado" acima), mas por decisão de arquitetura registrada no próprio cabeçalho do arquivo (`heavy.yml:14-19`, bilíngue): *"TRIGGERS ARE THE SECURITY BOUNDARY. push to main and workflow_dispatch ONLY. There is deliberately NO pull_request trigger (...) the repository is PUBLIC, and GitHub explicitly warns that a self-hosted runner on a public repository lets a fork's pull request execute arbitrary code on the host machine."* Essa alegação sobre o aviso do GitHub **não foi confirmada nesta seção da doc** (é a seção de Segurança, fora do meu escopo) -- registrar aqui para quem consolidar checar contra a seção de Segurança de outro agente.
- **`nightly.yml` usa `schedule: cron: "17 3 * * *"` (`nightly.yml:63`) + `workflow_dispatch` (`:66`).** A regra dos **60 dias de inatividade desabilita cron em repo público** é uma descoberta desta leitura, **não estava registrada em nenhum item do `TODO.md` verificado nesta fatia** -- é achado novo, não lição já sabida: se o repositório ficar 60 dias sem `push`/PR/etc, o `nightly` (a rede de segurança ASan) para de rodar silenciosamente, sem erro visível em lugar nenhum além de "sumiu da lista de runs". Como o projeto tem cadência de commits frequente, o risco é hoje teórico, mas é o tipo de coisa que só se percebe olhando `gh run list --workflow=nightly.yml` periodicamente -- reforça a lição já registrada (`CI-LINT-RED`) de que "a verificação que não se faz não avisa que não foi feita".
- **`ci.yml` combina `tags: ["v*"]` com `paths:` no MESMO bloco `push:` (`ci.yml:69-73`).** O comentário do próprio arquivo (`ci.yml:28-32`) afirma que isso é proposital porque *"o GitHub Actions não consegue calcular diff de uma ref recém-criada contra um estado anterior, então o filtro de paths sempre casa (...) numa tag recém-empurrada"*. **Verificado por mim nesta fatia:** a doc confirma o FATO (citação literal obtida via segunda leitura da página de sintaxe, seção EN reference/workflows-and-actions/workflow-syntax: **"Path filters are not evaluated for pushes of tags"**), mas **não confirma o MECANISMO** que o comentário do arquivo dá como explicação ("não consegue calcular diff de ref nova") -- a doc apenas declara o comportamento, sem justificá-lo por essa razão. **Contradição parcial, não do comportamento (que bate), mas da explicação causal** -- vale marcar o comentário do arquivo como "efeito confirmado, causa não confirmada" numa próxima passada de doc-comment.

### Aplicação da seção 1 (permissions / concurrency / strategy) neste projeto

- **`permissions:` só é declarado em `heavy.yml` (`:123-124`, `contents: read`).** Os outros 5 fluxos (`ci.yml`, `core-ci.yml`, `nightly.yml`, `windows-atoms.yml`, `distro-matrix.yml`) **não declaram `permissions:` nenhuma**, herdando o default do `GITHUB_TOKEN` configurado nas Settings do repositório. A doc recomenda menor privilégio POR JOB/WORKFLOW explícito (seção `permissions`, acima) -- **é uma lacuna real, não uma contradição**: os 5 fluxos sem `permissions:` explícito rodam em runner hospedado (superfície menor que self-hosted), mas ainda assim herdam qualquer escopo de escrita que o default do repo conceda hoje, sem ninguém ter de olhar o YAML pra saber qual é. Reportar, não consertar (fora do escopo desta fatia).
- **`concurrency:` só existe em `heavy.yml` (`:117-119`, `group: heavy-${{ github.ref }}`, `cancel-in-progress: true`).** Faz sentido lá -- runner único, `-j2`, 8 GiB, uma fila cara. **`ci.yml` (o gate mais disparado -- todo push+PR na `main`) não declara `concurrency:` nenhuma.** Push sucessivo rápido na `main` empilha execuções paralelas de `ci.yml` em vez de cancelar a obsoleta -- gasta minutos de runner hospedado à toa (não é um limite rígido do GitHub, é desperdício de cota/tempo). Reportar, não consertar.
- **`strategy.matrix` -- a lição `CI-VERMELHO-2X` (2026-08-06) é exatamente sobre esta chave.** Contexto: `distro-matrix.yml` tem uma perna `arch-container` com `matrix.werror=true` (única com `-DCMAKE_CXX_FLAGS=-Werror`); o commit original que tentou consertar um erro de `-Wmissing-field-initializers` sobre código vendorizado (`stb_image_write.h`, ver `CI-ARCH-WERROR-STB`) aplicou `-Wno-missing-field-initializers` **sem condicionar por plataforma** -- e quebrou as **duas pernas** do job `windows msvc build` (`ci.yml:935`, `strategy: matrix: backend: [ON, OFF]`) com `error D8021: invalid numeric argument`, porque `cl.exe` não reconhece essa grafia de flag GCC (confirmado empiricamente: `cl.exe` aceita `-w` GCC-style porque mapeia para `/w` real, mas não aceita `-Wno-missing-field-initializers` porque não existe `/W` correspondente -- ver `glintfx/CMakeLists.txt:1168-1183`, citação completa). **Conserto:** a mesma generator-expression `$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:...>` já usada nos outros 9 pontos do arquivo (`CMakeLists.txt` linha ~1105). **Lição, na forma que o líder/orquestrador registrou:** *"quando o arquivo de build é compartilhado, o conjunto de plataformas a verificar é ENUMERADO, não lembrado"* -- a doc do `strategy.matrix` não previne isso (matrix é só o mecanismo de gerar as pernas); o gap foi de processo, não de sintaxe. **A doc não documenta uma forma de "aplicar uma flag só numa perna específica da matrix" nativamente pelo YAML** -- a divisão por plataforma aqui é feita no `CMakeLists.txt` (`CXX_COMPILER_ID`), não no `strategy.matrix` do workflow; quem quisesse condicionar por matrix no próprio YAML usaria `if: matrix.os == '...'` num step (ver seção `jobs.<job_id>.steps[*].if`, tópico 1).

---

## 3. Variables reference / Referência de variáveis

**Source / Fonte:** https://docs.github.com/pt/actions/reference/workflows-and-actions/variables (fetched via WebFetch).

### EN

- **Two families:** *default environment variables* (`GITHUB_*`/`RUNNER_*`, set automatically by GitHub for every step, e.g. `GITHUB_SHA`, `GITHUB_REF`, `GITHUB_WORKSPACE`, `GITHUB_JOB`, `GITHUB_RUN_ID`, `GITHUB_RUN_NUMBER`, `RUNNER_OS`, `RUNNER_ARCH`, `RUNNER_NAME`, `RUNNER_TEMP`, `RUNNER_TOOL_CACHE`, plus `CI`, `GITHUB_ACTIONS`, `GITHUB_ACTOR`, `GITHUB_REPOSITORY`, and others -- **the extraction did not capture the full table verbatim; treat the list above as a representative subset, not exhaustive**) versus *custom configuration variables* (`vars` context, Settings > Variables, org/repo/environment scoped).
- **Precedence when the same name exists at multiple scopes: the lowest-scope variable wins** -- explicit order documented: environment-level > repository-level > organization-level. **Environment-level variables are only available on the runner after the job starts** (they gate on the deployment `environment:` protection flow).
- **Naming:** alphanumeric + underscore only, cannot start with a digit, **does not need** (i.e. is not required, but also must not collide with) the `GITHUB_`/`RUNNER_` prefixes reserved for default variables; not case-sensitive when referenced; must be unique within its creation scope.
- **Limits:** single variable max **48 KB**; combined org+repo variables per workflow run max **256 KB**. Counts: up to 1 000 variables per organization, 500 per repository, 100 per environment; a workflow run can pull up to 500 repo vars / 1 000 org vars provided the combined 256 KB cap holds.
- **Access:** inside expressions (`if:`, `env:` values, `with:` values) use `${{ env.X }}` / `${{ vars.X }}` / `${{ github.X }}`; inside a `run:` shell block, plain shell interpolation (`$X` on bash/sh, `$env:X` on pwsh) reads the process environment directly -- **no `${{ }}` needed there**, and in fact interpolating untrusted context data with `${{ }}` directly into a `run:` script body is the classic script-injection vector (see contexts section 5 below for the security note).
- **Critical distinction:** the *default* `GITHUB_*` variables are **not** exposed under the `env` context (`${{ env.GITHUB_SHA }}` does not work) -- each has an equivalent property under the `github`/`runner` context instead (`${{ github.sha }}`, `${{ runner.os }}`, etc). Custom `env:` entries and `vars` **are** exposed under their own contexts (`env.*`, `vars.*`).

### PT

- **Duas famílias:** *variáveis de ambiente padrão* (`GITHUB_*`/`RUNNER_*`, definidas automaticamente pelo GitHub em todo step, ex.: `GITHUB_SHA`, `GITHUB_REF`, `GITHUB_WORKSPACE`, `GITHUB_JOB`, `GITHUB_RUN_ID`, `GITHUB_RUN_NUMBER`, `RUNNER_OS`, `RUNNER_ARCH`, `RUNNER_NAME`, `RUNNER_TEMP`, `RUNNER_TOOL_CACHE`, mais `CI`, `GITHUB_ACTIONS`, `GITHUB_ACTOR`, `GITHUB_REPOSITORY`, entre outras -- **a extração não capturou a tabela completa literalmente; tratar a lista acima como subconjunto representativo, não exaustivo**) versus *variáveis de configuração customizadas* (contexto `vars`, Settings > Variables, escopo org/repo/environment).
- **Precedência quando o mesmo nome existe em múltiplos escopos: a variável do escopo MAIS BAIXO ganha** -- ordem explícita documentada: nível de environment > nível de repositório > nível de organização. **Variáveis de nível de environment só ficam disponíveis no runner depois que o job começa a rodar** (dependem do fluxo de proteção do `environment:` de deployment).
- **Nomenclatura:** só alfanumérico + underscore, não pode começar com dígito, **não precisa** (isto é, não é exigido, mas também não pode colidir com) os prefixos `GITHUB_`/`RUNNER_` reservados pras variáveis padrão; não diferencia maiúsculas/minúsculas quando referenciada; precisa ser única dentro do escopo de criação.
- **Limites:** variável individual máx. **48 KB**; combinado org+repo por execução de workflow máx. **256 KB**. Contagens: até 1.000 variáveis por organização, 500 por repositório, 100 por environment; uma execução pode puxar até 500 vars de repo / 1.000 de org desde que o teto combinado de 256 KB seja respeitado.
- **Acesso:** dentro de expressões (`if:`, valores de `env:`, valores de `with:`) usa-se `${{ env.X }}` / `${{ vars.X }}` / `${{ github.X }}`; dentro de um bloco shell `run:`, interpolação de shell pura (`$X` em bash/sh, `$env:X` em pwsh) lê o ambiente do processo diretamente -- **não precisa de `${{ }}` ali**, e de fato interpolar dado de contexto não confiável via `${{ }}` diretamente no CORPO de um script `run:` é o vetor clássico de injeção de script (ver a nota de segurança na seção 5, Contextos, abaixo).
- **Distinção crítica:** as variáveis *padrão* `GITHUB_*` **não** ficam expostas sob o contexto `env` (`${{ env.GITHUB_SHA }}` não funciona) -- cada uma tem uma propriedade equivalente sob o contexto `github`/`runner` em vez disso (`${{ github.sha }}`, `${{ runner.os }}`, etc). Entradas customizadas de `env:` e `vars` **ficam** expostas sob os próprios contextos (`env.*`, `vars.*`).

### Aplicação neste projeto / Application in this project

- **Nenhum dos 6 fluxos usa o contexto `vars` (`${{ vars.* }}`) nem `secrets.*` custom** -- confirmado por grep (`grep -rn "secrets\." .github/workflows/` = zero ocorrências). O único crédito usado é o `GITHUB_TOKEN` implícito, e só em `heavy.yml` de forma explícita (`permissions: contents: read`, que restringe a leitura sem precisar declarar `secrets.GITHUB_TOKEN`). Não é lacuna -- é a superfície mínima possível (nenhum job publica nada, todos só fazem checkout+build+teste).
- **`RUNNER_TEMP`/`GITHUB_PATH` em uso real:** `ci.yml:629`/`:645` fazem `echo "$HOME/.local/bin" >> "$GITHUB_PATH"` para adicionar um diretório instalado dentro do job ao `PATH` das etapas seguintes -- é exatamente o padrão "workflow command via arquivo apontado por variável de ambiente" que a doc de variáveis referencia (o arquivo em si é descrito com mais detalhe na página de *workflow commands*, fora do meu escopo desta fatia).
- **`${{ runner.os }}` em toda chave de cache** (`ci.yml:158`, `:285`, `:399`, `:723`, `:988`; `nightly.yml:94`) -- uso canônico e correto do contexto `runner` (não `env`) para compor uma chave de cache que precisa variar por SO, exatamente como a distinção "padrão não fica em `env`, fica em contexto próprio" prevê.

---

## 4. Expressions / Expressões

**Source / Fonte:** https://docs.github.com/pt/actions/reference/workflows-and-actions/expressions (fetched via WebFetch).

### EN

- **Syntax:** `${{ <expression> }}`. Inside `if:` the wrapper is implicit -- `if: <expression>` and `if: ${{ <expression> }}` are equivalent.
- **Operators:** `()` grouping, `[]` indexing, `.` property access, `!` (NOT), `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&` (AND), `||` (OR).
- **Type coercion on comparison (loose equality):** `null`→`0`; `true`→`1`, `false`→`0`; strings parse as JSON numbers when possible, empty string→`0`; arrays/objects→`NaN`. String comparisons are case-insensitive. Relational comparisons involving `NaN` are always `false`. Objects/arrays are only equal to themselves (same instance), never structurally.
- **Built-in functions** (non-exhaustive on this page, but this is the operationally relevant subset): `contains(search, item)`, `startsWith(searchString, searchValue)`, `endsWith(searchString, searchValue)` (all case-insensitive); `format(string, ...values)` (positional `{0}`, `{1}`..., `{{`/`}}` to escape literal braces); `join(array, optionalSeparator)` (default separator `,`); `toJSON(value)` (debug-oriented JSON dump); `fromJSON(value)` (parses a JSON string into a real object/array/bool/number -- the standard way to turn a `strategy.matrix` axis or a step output into a structured value); `hashFiles(path, ...)` (SHA-256 over files matching the glob(s), empty string if nothing matches -- the standard cache-key ingredient).
- **Status-check functions**, implicitly `success()` when `if:` has none of them: `success()` (default -- true if every prior step in the job succeeded), `always()` (always true -- runs the step regardless of prior failure/cancellation; the page notes this should be used carefully, not for steps whose own failure would be dangerous to ignore), `cancelled()` (true if the workflow run was cancelled), `failure()` (true if any prior step -- or, across `needs:`, any ancestor job -- failed). **The implicit-`success()` default is the single most consequential fact on this page for CI design: `if: <condition>` silently means `if: success() && <condition>`, so a step meant to run "whenever the condition holds, even after an earlier failure" needs `if: always() && <condition>` explicitly.**
- **Literals:** `true`/`false` (boolean), `null`, numbers (JSON-valid: decimal, exponential, hex), strings (single-quoted, `''` escapes a literal single quote; no `${{ }}` wrapper strictly required for a bare literal but conventionally still used inside `if:`/`env:` for clarity). Falsy set: `false`, `0`, `-0`, `""`, `''`, `null`; everything else is truthy.
- **Não confirmado nesta leitura:** precedência formal de operadores (tabela explícita) e limite de tamanho/profundidade de expressão **não apareceram no conteúdo extraído** desta página nesta sessão -- não afirmar um valor específico sem reconfirmar.

### PT

- **Sintaxe:** `${{ <expressão> }}`. Dentro de `if:` o envelope é implícito -- `if: <expressão>` e `if: ${{ <expressão> }}` são equivalentes.
- **Operadores:** `()` agrupamento, `[]` indexação, `.` acesso a propriedade, `!` (NOT), `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&` (AND), `||` (OR).
- **Coerção de tipo na comparação (igualdade frouxa):** `null`→`0`; `true`→`1`, `false`→`0`; strings são parseadas como número JSON quando possível, string vazia→`0`; arrays/objetos→`NaN`. Comparação de string não diferencia maiúsculas/minúsculas. Comparação relacional envolvendo `NaN` é sempre `false`. Objetos/arrays só são iguais a si mesmos (mesma instância), nunca estruturalmente.
- **Funções embutidas** (não-exaustivo nesta página, mas é o subconjunto operacionalmente relevante): `contains(search, item)`, `startsWith(searchString, searchValue)`, `endsWith(searchString, searchValue)` (todas insensíveis a maiúsculas/minúsculas); `format(string, ...valores)` (posicional `{0}`, `{1}`..., `{{`/`}}` escapa chaves literais); `join(array, separadorOpcional)` (separador padrão `,`); `toJSON(valor)` (dump JSON voltado a debug); `fromJSON(valor)` (parseia uma string JSON num objeto/array/bool/número real -- a forma padrão de transformar um eixo de `strategy.matrix` ou uma saída de step num valor estruturado); `hashFiles(path, ...)` (SHA-256 sobre arquivos que casam o(s) glob(s), string vazia se nada casar -- o ingrediente padrão de chave de cache).
- **Funções de status**, implicitamente `success()` quando `if:` não usa nenhuma delas: `success()` (padrão -- verdadeiro se todo step anterior do job teve sucesso), `always()` (sempre verdadeiro -- roda o step independente de falha/cancelamento anterior; a página nota que deve ser usada com cuidado, não pra steps cuja própria falha seria perigoso ignorar), `cancelled()` (verdadeiro se a execução foi cancelada), `failure()` (verdadeiro se algum step anterior -- ou, através de `needs:`, algum job ancestral -- falhou). **O padrão implícito de `success()` é o fato mais consequente desta página pro desenho de CI: `if: <condição>` silenciosamente significa `if: success() && <condição>`, então um step que deveria rodar "sempre que a condição valer, mesmo depois de uma falha anterior" precisa de `if: always() && <condição>` explícito.**
- **Literais:** `true`/`false` (booleano), `null`, números (válidos em JSON: decimal, exponencial, hex), strings (aspas simples, `''` escapa uma aspa simples literal; o envelope `${{ }}` não é estritamente exigido pra um literal cru, mas convencionalmente ainda é usado dentro de `if:`/`env:` por clareza). Conjunto falsy: `false`, `0`, `-0`, `""`, `''`, `null`; tudo o mais é truthy.
- **Não confirmado nesta leitura:** precedência formal de operadores (tabela explícita) e limite de tamanho/profundidade de expressão **não apareceram no conteúdo extraído** desta página nesta sessão -- não afirmar um valor específico sem reconfirmar.

### Aplicação neste projeto / Application in this project

- **`hashFiles('glintfx/CMakeLists.txt')` é o ingrediente de TODAS as chaves de cache do RmlUi** (`ci.yml:158`,`:285`,`:399`,`:723`; `nightly.yml:94`; `heavy.yml` -- comentário `:201-211` explica que ele reaproveita as mesmas chaves `rmlui-${{ runner.os }}-...` dos jobs `ubuntu-latest`). Uso canônico: o `CMakeLists.txt` é onde a versão pinada do RmlUi (`FetchContent`) está declarada, então uma mudança nele invalida corretamente o cache.
- **Nenhum uso de `fromJSON`/`toJSON`/`format`/`join` nos 6 fluxos** (confirmado por grep) -- os workflows deste repo não fazem parsing de JSON dinâmico nem formatação complexa de string; toda lógica condicional usa `matrix.*`/`github.*` diretamente. Não é lacuna, é simplicidade genuína (nenhum job gera matriz dinâmica via `fromJSON` a partir de uma etapa anterior).
- **`if: matrix.backend == 'ON'` (`ci.yml:1012`)** é o único `if:` condicionado por matrix encontrado nos 6 fluxos -- exemplo direto do padrão "implicit `success()`" descrito acima: como não usa `always()`/`failure()`, esse step só roda se TODOS os steps anteriores do job tiverem sucesso E `matrix.backend == 'ON'`, comportamento correto para o caso (não teria sentido rodar um passo específico do backend ON depois de uma falha anterior no mesmo job).
- **Este projeto não usa (visto nesta fatia) `always()`/`failure()`/`cancelled()` em NENHUM dos 6 fluxos** (confirmado por grep). Isso significa: hoje, se um step intermediário falhar, todo step seguinte no mesmo job é pulado por padrão -- inclusive qualquer step de limpeza/relato que dependesse de rodar mesmo com falha anterior. **Não é contradição com a doc (o padrão é documentado e é o esperado), mas é uma lacuna a observar** se algum job futuro precisar de "sempre rode este passo de diagnóstico, mesmo se o build falhou antes" (ex.: publicar log de crash do `clang-tidy`/sanitizer). Reportar, não consertar.

---

## 5. Contexts / Contextos

**Source / Fonte:** https://docs.github.com/pt/actions/reference/workflows-and-actions/contexts (fetched via WebFetch, mais uma segunda leitura direcionada da página em inglês para confirmar literalmente `steps.<id>.outcome` vs `steps.<id>.conclusion`).

### EN

- **12 contexts**, availability varies by phase: `github` (available throughout), `env` (workflow/job/step-defined vars), `vars` (org/repo/environment config vars), `job` (current job info, **only defined during step execution**), `jobs` (**only for reusable workflows**, aggregates outputs), `steps` (executed steps of the current job), `runner` (executor machine details), `secrets` (secret names/values available to this run), `strategy` (matrix execution strategy info), `matrix` (current job's matrix property values), `needs` (outputs of declared dependency jobs), `inputs` (inputs for reusable or manually-dispatched workflows). Rule of thumb confirmed by the page: most contexts resolve early (before the runner picks up the job); `job`, `runner`, `steps` require the runner to actually be executing.
- **`github` context, key properties:** `github.event` (full webhook payload that triggered the run), `github.event_name`, `github.sha` (commit SHA), `github.ref` (full ref, `refs/heads/<branch>` or `refs/tags/<tag>`), `github.ref_name` (short name), `github.actor` (username that triggered the run), `github.repository` (`owner/repo`), `github.run_id` (unique per run, **unchanged across re-runs**), `github.run_number` (increments per new run, **not** per re-run), `github.workspace` (checkout/working dir), `github.token` (functionally equivalent to the `GITHUB_TOKEN` secret).
- **`runner` context:** `runner.os`, `runner.temp`, `runner.tool_cache`, `runner.debug`. (The extraction did not surface full descriptions for each beyond the name -- treat as confirmed-to-exist, not confirmed-in-full-detail.)
- **`secrets` context:** contains the names/values of secrets available to the run; `${{ secrets.NAME }}`. `github.token` is explicitly documented as **"functionally equivalent to the `GITHUB_TOKEN` secret"** and is always available with no declaration required.
- **`steps` context -- `outcome` vs `conclusion` (confirmed by literal quote from the English page, since the Portuguese extraction alone was ambiguous):** *"The result of a completed step **before** `continue-on-error` is applied"* is `outcome`; *"The result of a completed step **after** `continue-on-error` is applied"* is `conclusion`. Concretely: a step that fails but has `continue-on-error: true` reports `outcome: failure`, `conclusion: success` -- `outcome` tells you what *actually happened*, `conclusion` tells you what the *job-level status calculus* will treat it as.
- **`job` context:** `job.status` -- current job's status.
- **Security warning, direct quote (translated):** *"When creating workflows and actions, always consider whether your code might execute untrusted input from possible attackers."* Named as attacker-controllable: `github.event.issue.title`, `github.head_ref` (a pull request's source branch name), and event-payload user-controlled fields generally. **Safe pattern documented:** pass the value through an intermediate `env:` variable first (`${{ env.TITLE }}` referenced from the shell as `$TITLE`, never the raw context interpolated straight into the `run:` script body), since environment-variable substitution happens at the shell level, not via GitHub's own string-templating engine that would otherwise expand attacker-controlled `${{ }}`-lookalike content inside the payload.

### PT

- **12 contextos**, disponibilidade varia por fase: `github` (disponível o tempo todo), `env` (vars definidas em workflow/job/step), `vars` (vars de configuração org/repo/environment), `job` (info do job atual, **só definido durante a execução de step**), `jobs` (**só para workflows reutilizáveis**, agrega outputs), `steps` (steps executados do job atual), `runner` (detalhes da máquina executora), `secrets` (nomes/valores de secrets disponíveis nesta execução), `strategy` (info da estratégia de execução da matrix), `matrix` (valores das propriedades de matrix do job atual), `needs` (outputs dos jobs de dependência declarados), `inputs` (entradas de workflows reutilizáveis ou disparados manualmente). Regra geral confirmada pela página: a maioria dos contextos resolve cedo (antes do runner pegar o job); `job`, `runner`, `steps` exigem que o runner já esteja executando.
- **Contexto `github`, propriedades-chave:** `github.event` (carga completa de webhook que disparou a execução), `github.event_name`, `github.sha` (SHA do commit), `github.ref` (ref completa, `refs/heads/<branch>` ou `refs/tags/<tag>`), `github.ref_name` (nome curto), `github.actor` (usuário que disparou a execução), `github.repository` (`owner/repo`), `github.run_id` (único por execução, **inalterado entre re-execuções**), `github.run_number` (incrementa por nova execução, **não** por re-execução), `github.workspace` (dir de checkout/trabalho), `github.token` (funcionalmente equivalente ao secret `GITHUB_TOKEN`).
- **Contexto `runner`:** `runner.os`, `runner.temp`, `runner.tool_cache`, `runner.debug`. (A extração não trouxe descrições completas além do nome de cada -- tratar como confirmado-que-existe, não confirmado-em-detalhe-total.)
- **Contexto `secrets`:** contém nomes/valores dos secrets disponíveis nesta execução; `${{ secrets.NOME }}`. `github.token` é documentado explicitamente como **"funcionalmente equivalente ao secret `GITHUB_TOKEN`"** e está sempre disponível sem precisar de declaração.
- **Contexto `steps` -- `outcome` vs `conclusion` (confirmado por citação literal da página em inglês, já que a extração em português sozinha ficou ambígua):** *"O resultado de um step concluído **antes** de `continue-on-error` ser aplicado"* é o `outcome`; *"O resultado de um step concluído **depois** de `continue-on-error` ser aplicado"* é o `conclusion`. Concretamente: um step que falha mas tem `continue-on-error: true` reporta `outcome: failure`, `conclusion: success` -- `outcome` diz o que *de fato aconteceu*, `conclusion` diz como o *cálculo de status de nível de job* vai tratar aquilo.
- **Contexto `job`:** `job.status` -- status do job atual.
- **Aviso de segurança, citação direta (traduzida):** *"Ao criar workflows e actions, sempre considere se seu código pode executar entrada não confiável de possíveis atacantes."* Citados como controláveis por atacante: `github.event.issue.title`, `github.head_ref` (nome do branch de origem de um pull request), e campos de carga de evento controlados pelo usuário em geral. **Padrão seguro documentado:** passar o valor por uma variável `env:` intermediária primeiro (`${{ env.TITLE }}` referenciada do shell como `$TITLE`, nunca o contexto cru interpolado direto no CORPO do script `run:`), já que a substituição de variável de ambiente acontece no nível do shell, não pelo próprio motor de templating de string do GitHub que expandiria conteúdo controlado por atacante parecido com `${{ }}` dentro da carga.

### Aplicação neste projeto / Application in this project

- **Nenhum dos 6 fluxos interpola `github.event.*`/`github.head_ref` diretamente dentro de um `run:`** (confirmado por grep -- não há `${{ github.event` nem `${{ github.head_ref` em nenhum arquivo `.github/workflows/*.yml`). O padrão de risco de script injection descrito acima **não se aplica hoje** a este repo -- os únicos usos de `github.*` são `github.ref`/`github.workflow` (em `concurrency.group` de `heavy.yml`) e `runner.os`/`matrix.*` (em chaves de cache e condicionais), nenhum dos quais é conteúdo controlável por um atacante externo via payload de PR/issue.
- **`steps.*.outcome`/`.conclusion` não são usados em NENHUM dos 6 fluxos** (confirmado por grep) -- nenhum step deste repo lê o resultado de um step anterior via contexto `steps`. Consistente com a lição `CI-TIDY-CRASH` (fora do meu escopo direto, mas tangente): a distinção "ferramenta crashou" (exit >= 128) vs "violação real de lint" (exit == 1) que `tools/run_clang_tidy_per_file.sh` implementa é feita **em bash puro** (`$?` capturado por invocação), não via `steps.<id>.outcome`/`conclusion` do GitHub -- porque o laço roda DENTRO de um único step `run:` (múltiplas invocações do `clang-tidy`, não múltiplos steps do workflow), então o contexto `steps` do GitHub nunca entraria em jogo aqui de qualquer forma. Nenhuma contradição; só uma nota de que a solução do problema ficou num nível abaixo do que este tópico de doc cobre.
- **`runner.temp`/`runner.tool_cache` não aparecem em uso nos 6 fluxos** (confirmado por grep) -- o projeto não usa essas propriedades hoje; ferramentas extras (`clang-tidy`, `cppcheck`, `gitleaks`, `grype`) são instaladas via `apt`/download direto para `$HOME/.local/bin`, não via o padrão `runner.tool_cache`. Não é contradição, é uma escolha de instalação diferente da que a doc destaca como o local "correto" para cache de ferramentas -- reportar como observação, não como defeito.

---

## 6. Denominador de leitura, contradições e lacunas (para a consolidação)

### EN

**Pages actually read this slice (denominator declared):** 10 pages with content successfully extracted, plus 1 truncated/ambiguous fetch and 1 failed fetch (404), both discarded and not counted as coverage:

1. `docs.github.com/pt/actions` (index) -- navigation overview only, low confidence extraction (the tool reported "no explicit sidebar menu"), used only to orient, not cited as a normative source anywhere above.
2. `docs.github.com/en/actions/reference` (Reference section index) -- link list.
3. `docs.github.com/en/actions/reference/workflows-and-actions` (sub-index) -- link list, gave me the 12 sibling pages of my assigned bucket, of which I read 5.
4. `docs.github.com/pt/actions/reference/workflows-and-actions/workflow-syntax` -- **section 1** of this doc.
5. `docs.github.com/pt/actions/reference/workflows-and-actions/events-that-trigger-workflows` -- **section 2**.
6. `docs.github.com/pt/actions/reference/workflows-and-actions/variables` -- **section 3**.
7. `docs.github.com/pt/actions/reference/workflows-and-actions/expressions` -- **section 4**.
8. `docs.github.com/pt/actions/reference/workflows-and-actions/contexts` -- **section 5** (first pass).
9. `docs.github.com/en/actions/reference/workflows-and-actions/contexts` -- second pass, English, to get the literal `outcome`/`conclusion` quote the Portuguese pass left ambiguous.
10. `docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax` -- second pass, English, to get the literal "Path filters are not evaluated for pushes of tags" quote.

**Discarded (not counted as coverage):**
- `docs.github.com/pt/actions/reference/workflows-and-actions/contexts#contexto-steps` -- anchor fetch returned a truncated/cached page that did not include the `steps` section; superseded by item 9 above.
- `docs.github.com/pt/actions/writing-workflows/choosing-what-your-workflow-does/patterns-for-filtering-branches-and-tags` -- 404, page does not exist at that path in the current doc tree.

**Of the 12 pages in the "workflows-and-actions" reference bucket, 7 were NOT read in this slice** (out of scope per the orchestrator's split): `workflow-commands`, `deployments-and-environments`, `dependency-caching`, `reusing-workflow-configurations`, `metadata-syntax`, `workflow-cancellation`, `dockerfile-support`. These belong to reusability/deployment topics assigned elsewhere (per the coordinator's message) or were simply not part of my five assigned topics -- flag for the consolidator to confirm coverage, since none of the parallel agents' scope was described to me beyond "the other three sections".

**Contradictions/tensions found between documented behavior and this project's practice (none are "the docs are wrong" -- all are "our workflows chose a narrower/looser posture than the least-privilege pattern the docs describe"):**

1. **`permissions:` is declared in only 1 of 6 workflows** (`heavy.yml`); the other 5 rely on the repository's default `GITHUB_TOKEN` permissions instead of an explicit, auditable-in-the-YAML scope. Not a violation of anything mandatory, but it is the opposite of the least-privilege pattern the `permissions` reference page recommends.
2. **`concurrency:` is declared in only 1 of 6 workflows** (`heavy.yml`); `ci.yml`, the most-frequently-triggered gate, has none, so rapid successive pushes to `main` queue parallel runs instead of superseding the stale one.
3. **The `ci.yml` header comment's causal explanation for tag+paths behavior ("GitHub cannot diff a brand-new ref") is not confirmed by the docs** -- the docs confirm the *effect* ("path filters are not evaluated for pushes of tags") but not that specific *mechanism*.
4. **The `heavy.yml` header's claim that "GitHub explicitly warns that a self-hosted runner on a public repository lets a fork's pull request execute arbitrary code" was not found on the events-reference page** -- it likely lives in the Security reference section, out of this slice's scope; flagged for the consolidator to cross-check against whichever agent covered Security.
5. **New finding, not previously tracked in `TODO.md` as far as this slice searched:** scheduled workflows on public repos auto-disable after 60 days of repository inactivity (`nightly.yml`'s cron trigger is exposed to this, though the project's commit cadence makes it low-risk today).

### PT

**Páginas de fato lidas nesta fatia (denominador declarado):** 10 páginas com conteúdo extraído com sucesso, mais 1 leitura truncada/ambígua e 1 leitura que falhou (404), ambas descartadas e não contadas como cobertura:

1. `docs.github.com/pt/actions` (índice) -- só visão geral de navegação, extração de baixa confiança (a ferramenta reportou "sem menu lateral explícito"), usada só para orientação, não citada como fonte normativa em nenhum ponto acima.
2. `docs.github.com/en/actions/reference` (índice da seção Referência) -- lista de links.
3. `docs.github.com/en/actions/reference/workflows-and-actions` (subíndice) -- lista de links, deu as 12 páginas irmãs do meu bloco atribuído, das quais li 5.
4. `docs.github.com/pt/actions/reference/workflows-and-actions/workflow-syntax` -- **seção 1** deste doc.
5. `docs.github.com/pt/actions/reference/workflows-and-actions/events-that-trigger-workflows` -- **seção 2**.
6. `docs.github.com/pt/actions/reference/workflows-and-actions/variables` -- **seção 3**.
7. `docs.github.com/pt/actions/reference/workflows-and-actions/expressions` -- **seção 4**.
8. `docs.github.com/pt/actions/reference/workflows-and-actions/contexts` -- **seção 5** (primeira passada).
9. `docs.github.com/en/actions/reference/workflows-and-actions/contexts` -- segunda passada, em inglês, pra pegar a citação literal `outcome`/`conclusion` que a passada em português deixou ambígua.
10. `docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax` -- segunda passada, em inglês, pra pegar a citação literal "Path filters are not evaluated for pushes of tags".

**Descartadas (não contadas como cobertura):**
- `docs.github.com/pt/actions/reference/workflows-and-actions/contexts#contexto-steps` -- fetch por âncora devolveu página truncada/em cache que não incluía a seção `steps`; superada pelo item 9 acima.
- `docs.github.com/pt/actions/writing-workflows/choosing-what-your-workflow-does/patterns-for-filtering-branches-and-tags` -- 404, a página não existe nesse caminho na árvore de doc atual.

**Das 12 páginas do bloco de referência "workflows-and-actions", 7 NÃO foram lidas nesta fatia** (fora do escopo pela divisão do orquestrador): `workflow-commands`, `deployments-and-environments`, `dependency-caching`, `reusing-workflow-configurations`, `metadata-syntax`, `workflow-cancellation`, `dockerfile-support`. Pertencem a tópicos de reutilização/deployment atribuídos a outra parte (pela mensagem do coordenador) ou simplesmente não estavam entre os meus cinco tópicos designados -- sinalizar para quem consolidar confirmar a cobertura, já que o escopo dos outros agentes paralelos não me foi descrito além de "as outras três seções".

**Contradições/tensões achadas entre o comportamento documentado e a prática deste projeto (nenhuma é "a doc está errada" -- todas são "nossos workflows escolheram uma postura mais estreita/frouxa do que o padrão de menor privilégio que a doc descreve"):**

1. **`permissions:` só é declarado em 1 dos 6 workflows** (`heavy.yml`); os outros 5 dependem do padrão do `GITHUB_TOKEN` do repositório em vez de um escopo explícito e auditável no próprio YAML. Não é violação de nada obrigatório, mas é o oposto do padrão de menor privilégio que a página de referência de `permissions` recomenda.
2. **`concurrency:` só é declarado em 1 dos 6 workflows** (`heavy.yml`); o `ci.yml`, o gate mais disparado, não tem nenhuma, então pushes sucessivos rápidos na `main` empilham execuções paralelas em vez de suplantar a obsoleta.
3. **A explicação causal do comentário de cabeçalho do `ci.yml` para o comportamento tag+paths ("o GitHub não consegue calcular diff de uma ref recém-criada") não é confirmada pela doc** -- a doc confirma o *efeito* ("filtros de path não são avaliados para pushes de tag") mas não esse *mecanismo* específico.
4. **A alegação do cabeçalho do `heavy.yml` de que "o GitHub avisa explicitamente que um runner self-hosted em repositório público deixa o pull request de um fork executar código arbitrário" não foi encontrada na página de referência de eventos** -- provavelmente vive na seção de Referência de Segurança, fora do escopo desta fatia; sinalizado para quem consolidar cruzar com quem cobriu Segurança.
5. **Achado novo, não rastreado (até onde esta fatia buscou) em nenhum item do `TODO.md`:** workflows agendados em repositórios públicos se auto-desabilitam após 60 dias de inatividade do repositório (o gatilho cron do `nightly.yml` está exposto a isso, embora a cadência de commits do projeto torne isso baixo-risco hoje).

---

*Fim da seção Referência. Aguardando sinal do orquestrador para consolidar junto com as outras 3 seções em `docs/github-actions.md`.*
