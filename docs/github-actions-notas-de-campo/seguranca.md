# Segurança e executores auto-hospedados (GitHub Actions)

> EN: Security section of the canonical GitHub Actions doc for this repo. Written by the
> `security-engineer`, from the official documentation (`docs.github.com`), tied to the 7
> measured lessons of this project (see `/var/tmp/gha-docs/CONTEXTO.md`) and to a real
> conformance audit of our 6 workflow files, run on 2026-08-06. This is not a generic
> tutorial: every topic either anchors a real incident here, or exists because our own CI
> configuration made the question concrete (self-hosted runner in a public repo).
>
> PT: Seção de segurança da doc canônica de GitHub Actions deste repo. Escrita pelo
> `security-engineer`, a partir da documentação oficial (`docs.github.com`), amarrada às 7
> lições medidas deste projeto (ver `/var/tmp/gha-docs/CONTEXTO.md`) e a uma auditoria de
> conformidade real dos nossos 6 arquivos de workflow, rodada em 2026-08-06. Isto não é
> tutorial genérico: todo tópico ou ancora um incidente real daqui, ou existe porque a nossa
> própria configuração de CI tornou a pergunta concreta (runner self-hosted em repo público).

## Método e denominador / Method and denominator

EN: 16 documentation URLs fetched with content extraction on 2026-08-06 (4 were navigation
index pages that returned mostly link lists; 12 returned substantive page content, quoted
below with literal excerpts where useful). All 6 of this repo's workflow files
(`ci.yml`, `core-ci.yml`, `heavy.yml`, `distro-matrix.yml`, `nightly.yml`,
`windows-atoms.yml`) were read in full before writing the audit table — not sampled.

PT: 16 URLs de documentação lidas com extração de conteúdo em 2026-08-06 (4 eram páginas de
índice de navegação que devolveram principalmente listas de links; 12 devolveram conteúdo
substantivo de página, citado abaixo com trechos literais quando útil). Os 6 arquivos de
workflow deste repo (`ci.yml`, `core-ci.yml`, `heavy.yml`, `distro-matrix.yml`,
`nightly.yml`, `windows-atoms.yml`) foram lidos por inteiro antes de montar a tabela de
auditoria -- não amostrados.

Páginas lidas / Pages read (16):

1. `docs.github.com/pt/actions` (entrada da ordem do líder / nav)
2. `docs.github.com/pt/actions/how-tos/secure-your-work` (nav, thin)
3. `docs.github.com/en/actions/concepts/security` (nav index)
4. `docs.github.com/en/actions/reference/security` (nav index)
5. `docs.github.com/en/actions/reference/security/secure-use` -- **guia núcleo de hardening**
6. `docs.github.com/en/actions/reference/security/securely-using-pull_request_target`
7. `docs.github.com/en/actions/concepts/security/github_token`
8. `docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax` (seção `permissions:`)
9. `docs.github.com/en/actions/concepts/security/compromised-runners`
10. `docs.github.com/en/actions/concepts/runners/self-hosted-runners`
11. `docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/manage-access` -- **achado mais valioso, ver §6**
12. `docs.github.com/en/actions/concepts/security/secrets`
13. `docs.github.com/en/actions/concepts/security/openid-connect`
14. `docs.github.com/en/actions/concepts/security/script-injections`
15. `docs.github.com/en/actions/reference/security/secrets`
16. `docs.github.com/en/actions/reference/security/oidc`

**Fora de escopo, declarado / Out of scope, declared** (não lido, com motivo):

- 9 páginas de OIDC por provedor de nuvem (`oidc-in-aws`, `-azure`, `-google-cloud-platform`,
  `-hashicorp-vault`, `-jfrog`, `-octopus-deploy`, `-pypi`, `-cloud-providers`,
  `-with-reusable-workflows`) -- nenhum dos nossos 6 fluxos faz deploy nem usa OIDC hoje;
  não aplicável.
- 6 páginas de artifact attestations (`use-artifact-attestations`, `increase-security-rating`,
  `enforce-artifact-attestations`, `verify-attestations-offline`, `manage-attestations`,
  `concepts/security/artifact-attestations`) -- feature de proveniência SLSA que nenhum dos
  nossos fluxos usa hoje; adjacente a "artefatos como vetor" (§8) mas é tópico distinto --
  registrado como candidato de fatia futura, não aprofundado.
- `concepts/security/kubernetes-admissions-controller` -- não aplicável (sem k8s no projeto).
- Subpáginas de "como configurar/rodar/rotular/monitorar um runner" (`add-runners`,
  `run-scripts`, `customize-containers`, `configure-the-application`, `apply-labels`,
  `use-in-a-workflow`, `monitor-and-troubleshoot`, `remove-runners`,
  `reference/runners/self-hosted-runners`) -- território declarado da seção "Gerenciar
  execuções, monitorar e diagnosticar" (`secao-operacao.md`), não desta.
- "Migrar para o GitHub Actions" -- excluído por ordem do `CONTEXTO.md`.
- Referência de sintaxe/contextos além da seção `permissions:` (usada aqui só para fechar a
  pergunta de herança de `GITHUB_TOKEN`) -- território da seção "Referência"
  (`secao-referencia.md`).

---

## 1. Endurecimento de segurança -- o guia oficial / Security hardening -- the official guide

EN: The current canonical page is **"Security hardening for GitHub Actions"**
(`docs.github.com/en/actions/reference/security/secure-use`) -- this is the page GitHub's own
navigation now files under "reference/security/secure-use" (an older URL,
`security-guides/security-hardening-for-github-actions`, has been restructured away; do not
link the old path). Read it end to end before touching any of our 6 workflows' `on:`,
`permissions:`, or `run:` blocks. Its own summary line: **"credentials being used within
workflows have the least privileges required and that appropriate audit logging is enabled."**
That sentence is the organizing principle behind every subsection below.

PT: A página canônica atual é **"Security hardening for GitHub Actions"**
(`docs.github.com/en/actions/reference/security/secure-use`) -- é a página que a própria
navegação do GitHub arquiva hoje sob "reference/security/secure-use" (uma URL mais antiga,
`security-guides/security-hardening-for-github-actions`, foi reestruturada; não linkar o
caminho velho). Leia-a de ponta a ponta antes de mexer em `on:`, `permissions:` ou `run:` de
qualquer um dos nossos 6 workflows. A linha-resumo dela: **"credentials being used within
workflows have the least privileges required and that appropriate audit logging is
enabled."** Essa frase é o princípio organizador de cada subseção abaixo.

---

## 2. Permissões do `GITHUB_TOKEN` / `GITHUB_TOKEN` permissions

EN: The token is created automatically per workflow run, scoped to the repository that
contains the workflow. Two facts matter most:

- **Without an explicit `permissions:` block anywhere in a workflow, the token's permissions
  fall back to "the default setting for the enterprise, organization, or repository."**
  ("The permissions for the `GITHUB_TOKEN` are initially set to the default setting for the
  enterprise, organization, or repository.") That default is a repo/org SETTING, not a
  property of the workflow file -- it can be changed later by anyone with admin access,
  silently, without touching a single `.yml`.
- `permissions:` can be declared at the **workflow level** (applies to every job) or
  **narrowed per job** (`jobs.<job_id>.permissions`). The full scope list: `actions`,
  `artifact-metadata`, `attestations`, `checks`, `code-quality`, `contents`, `deployments`,
  `discussions`, `id-token`, `issues`, `packages`, `pages`, `pull-requests`,
  `security-events`, `statuses`, `vulnerability-alerts` -- each independently `read`/`write`/
  `none`. Shorthand `read-all`/`write-all` exists but defeats the point of least privilege.
- The official hardening guide's own recommendation: **"set the default permission for the
  `GITHUB_TOKEN` to read access only for repository contents"** and elevate per-job only
  where a job actually needs to write (e.g. pushing a tag, commenting on a PR, publishing a
  package) -- because "any user with write access to the repository, or a fork, gets read
  access to the secrets."

PT: O token é criado automaticamente por execução de workflow, escopado ao repositório que
contém o workflow. Dois fatos importam mais:

- **Sem um bloco `permissions:` explícito em lugar nenhum do workflow, as permissões do
  token caem para "o ajuste default da enterprise, organização ou repositório."** Esse
  default é um AJUSTE do repo/org, não uma propriedade do arquivo de workflow -- pode ser
  mudado depois por qualquer um com acesso de admin, em silêncio, sem tocar um `.yml`
  sequer.
- `permissions:` pode ser declarado no **nível do workflow** (vale para todo job) ou
  **estreitado por job** (`jobs.<job_id>.permissions`). Lista completa de escopos:
  `actions`, `artifact-metadata`, `attestations`, `checks`, `code-quality`, `contents`,
  `deployments`, `discussions`, `id-token`, `issues`, `packages`, `pages`, `pull-requests`,
  `security-events`, `statuses`, `vulnerability-alerts` -- cada um independentemente
  `read`/`write`/`none`. O atalho `read-all`/`write-all` existe mas derrota o propósito de
  menor privilégio.
- A recomendação do próprio guia de hardening: **"set the default permission for the
  `GITHUB_TOKEN` to read access only for repository contents"** e elevar por job só onde o
  job de fato precisa escrever (ex.: empurrar tag, comentar em PR, publicar pacote) --
  porque "any user with write access to the repository, or a fork, gets read access to the
  secrets."

**Amarração com este repo / Tied to this repo:** verificado via `gh api
repos/petrinhu/glintfx/actions/permissions/workflow` -- `default_workflow_permissions` está
em **`"read"`** neste repositório (bom, é o ajuste recomendado). Mas isso é uma configuração
de repositório, não algo que os 6 arquivos de workflow declaram por si -- ver §9 para o
número exato de quantos declaram `permissions:` explicitamente. A defesa em profundidade
pede o explícito no arquivo além do default correto no repo, porque o ajuste de repo pode
mudar sem que ninguém revise um diff de `.yml`.

---

## 3. Segredos / Secrets

EN: Three scopes -- **organization**, **repository**, **environment** -- with **environment
> repository > organization** precedence when names collide ("the secret at the lowest level
takes precedence"). Technical limits: secrets are **limited to 48 KB**; up to 1000
organization secrets, 100 repository secrets, 100 environment secrets per workflow run.
Naming: alphanumeric/underscore only, cannot start with `GITHUB_` or a digit, case-insensitive
on reference.

Encryption: secrets use **libsodium sealed boxes**, encrypted client-side before reaching
GitHub. Masking in logs: GitHub **"automatically redacts the content of any GitHub secrets
that are printed to the workflow log"**, plus a fixed list of recognized sensitive-data
formats (Azure keys, JWTs, GitHub v2 tokens, etc.) even when not stored as a secret. But this
is explicitly **not guaranteed**: **"this redaction is not guaranteed"** because there are
"multiple ways a secret value could be transformed" (base64, split across lines, printed
character-by-character), and "the runner can only mask secrets that are used within the
current job" -- a secret exposed in job A's log is not retroactively masked when job B logs
the same value differently. **If an unredacted secret reaches a log, the guide's own
remediation is: delete the log AND rotate the secret** -- deleting the log alone is
insufficient (the secret is compromised, not just "displayed").

What is **not** automatically protected: data that is sensitive but not stored as a GitHub
secret (a password embedded in a config file, a token pasted into a commit message) gets none
of this masking.

Reusable workflows / environments: environment-scoped secrets can require **"required
reviewers"** before a job that references that environment runs -- a manual approval gate
between "workflow triggered" and "secret becomes readable."

PT: Três escopos -- **organização**, **repositório**, **ambiente** -- com precedência
**ambiente > repositório > organização** quando nomes colidem ("the secret at the lowest
level takes precedence"). Limites técnicos: segredos **limitados a 48 KB**; até 1000
segredos de organização, 100 de repositório, 100 de ambiente por execução de workflow. Nome:
só alfanumérico/underscore, não pode começar com `GITHUB_` nem dígito, case-insensitive na
referência.

Criptografia: segredos usam **libsodium sealed boxes**, criptografados no lado do cliente
antes de chegar ao GitHub. Mascaramento em log: o GitHub **"automatically redacts the
content of any GitHub secrets that are printed to the workflow log"**, mais uma lista fixa de
formatos de dado sensível reconhecidos (chaves Azure, JWTs, tokens GitHub v2, etc.) mesmo
quando não armazenados como segredo. Mas isto é explicitamente **não garantido**: **"this
redaction is not guaranteed"** porque existem "multiple ways a secret value could be
transformed" (base64, quebrado entre linhas, impresso caractere-a-caractere), e "the runner
can only mask secrets that are used within the current job" -- um segredo exposto no log do
job A não é retroativamente mascarado quando o job B loga o mesmo valor de outro jeito.
**Se um segredo não-mascarado chega a um log, o próprio remédio do guia é: apagar o log E
rotacionar o segredo** -- apagar só o log não basta (o segredo está comprometido, não só
"exibido").

O que **não** é protegido automaticamente: dado sensível mas não armazenado como segredo do
GitHub (uma senha embutida num arquivo de config, um token colado numa mensagem de commit)
não recebe mascaramento nenhum.

Workflows reutilizáveis / ambientes: segredos escopados a ambiente podem exigir
**"required reviewers"** antes de um job que referencia aquele ambiente rodar -- um gate de
aprovação manual entre "workflow disparado" e "segredo fica legível."

**Amarração com este repo / Tied to this repo:** confirmado por `grep -rn "secrets\."` nos 6
arquivos -- **nenhum uso de `secrets.*`** em lugar nenhum dos 6 workflows. Isso reduz
drasticamente a superfície deste tópico específico aqui e hoje (não há segredo de
organização/repositório sendo consumido por nenhum dos fluxos), mas **não elimina o risco de
vazamento por log** -- se um passo de `run:` algum dia imprimir uma variável de ambiente
sensível (token de terceiro, chave de API futura), a regra "apagar log + rotacionar" vale
igual, e nada nos nossos gates hoje detecta isso automaticamente em tempo real (o
`gitleaks` do `lint-and-scan` varre o histórico do git, não os logs de execução do Actions).

---

## 4. OIDC e credenciais de curta duração / OIDC and short-lived credentials

EN: The problem OIDC solves: **"using hardcoded secrets requires you to create credentials in
the cloud provider and then duplicate them in GitHub as a secret"** -- a long-lived credential
sitting in GitHub Secrets forever, valid until manually rotated. OIDC replaces that with a
per-job, auto-expiring token: GitHub's OIDC provider issues a JWT for the run, the cloud
provider validates its claims against a pre-configured trust relationship, and issues **"a
short-lived access token that is only valid for a single job, and then automatically
expires."**

Required permission: **`id-token: write`**, declared at workflow or job level. **Without it,
"the OIDC JWT ID token cannot be requested"** at all -- the step requesting it simply fails.
JWT claims include `sub`, `aud` (defaults to the repository owner's URL), `iss`
(`https://token.actions.githubusercontent.com`), plus GitHub-specific claims:
`repository`, `repository_id`, `repository_owner`, `ref`, `ref_type`, `workflow`,
`workflow_ref`, `workflow_sha`, `actor`, `actor_id`, `environment`. The cloud-side trust
policy **must** condition on at least one of these (e.g. `repository:petrinhu/glintfx:ref:
refs/heads/main`) -- an unconditioned trust policy would let any workflow in any repository
that can reach GitHub's OIDC issuer mint a token, which defeats the whole mechanism.

PT: O problema que o OIDC resolve: **"using hardcoded secrets requires you to create
credentials in the cloud provider and then duplicate them in GitHub as a secret"** -- uma
credencial de longa duração parada no GitHub Secrets pra sempre, válida até rotação manual. O
OIDC substitui isso por um token por-job, que expira sozinho: o provedor OIDC do GitHub emite
um JWT pra execução, o provedor de nuvem valida as claims contra uma relação de confiança
pré-configurada, e emite **"a short-lived access token that is only valid for a single job,
and then automatically expires."**

Permissão exigida: **`id-token: write`**, declarada no nível de workflow ou de job. **Sem
ela, "the OIDC JWT ID token cannot be requested"** -- o passo que a pede simplesmente falha.
Claims do JWT incluem `sub`, `aud` (default = URL do dono do repositório), `iss`
(`https://token.actions.githubusercontent.com`), mais claims específicas do GitHub:
`repository`, `repository_id`, `repository_owner`, `ref`, `ref_type`, `workflow`,
`workflow_ref`, `workflow_sha`, `actor`, `actor_id`, `environment`. A política de confiança
do lado da nuvem **precisa** condicionar em pelo menos uma delas (ex.:
`repository:petrinhu/glintfx:ref:refs/heads/main`) -- uma política de confiança sem condição
deixaria qualquer workflow de qualquer repositório que alcance o emissor OIDC do GitHub
gerar um token, o que derrota o mecanismo inteiro.

**Amarração com este repo / Tied to this repo:** **não aplicável hoje** -- nenhum dos 6
fluxos publica em nuvem, registry de container, ou qualquer serviço externo que aceite OIDC.
Registrado aqui para quando essa necessidade aparecer (ex.: publicar imagem do runner
self-hosted num registry, ou publicar release assinada em algum serviço) -- a resposta
default deveria ser OIDC, não um novo secret de longa duração.

---

## 5. Fixar ação por SHA versus por tag / Pin actions to a SHA vs. a tag

EN: The guide's own words: **"Pinning to a particular SHA helps mitigate the risk of a bad
actor adding a backdoor to the action's repository, as they would need to generate a SHA-1
collision"** [note: GitHub Actions SHA pinning uses the full 40-character Git object hash;
Git's transition away from SHA-1 collision-prone hashing does not change the operational
guidance here]. A tag, in contrast, **"can be moved or deleted if a bad actor gains access to
the repository storing the action"** -- pinning `@v4` trusts that `v4` will always point to
the code you reviewed; pinning `@11d5960a...` trusts nothing beyond the bytes you already
audited. This is the exact **tj-actions/changed-files 2025 pattern** our own `heavy.yml`
comment cites: a compromised maintainer account repoints a mutable tag, and every consumer
pinned to that tag pulls the new, malicious code on their next run without any diff to
review.

### Amarração obrigatória -- lição 4 / Mandatory tie-in -- lesson 4

**This is the CORRECT security choice, and it created real operational fragility, measured in
this repo.** Both are true; do not treat the fragility as a defect to be quietly patched away.
`heavy.yml`'s two jobs (`sanitize`, `fonteng`) pin `actions/checkout` and `actions/cache` by
full commit SHA -- because they run on this repo's own self-hosted runner (see §6), a
materially different risk tier than the same actions on a disposable GitHub-hosted VM. **Three
steps failed with a 100-second timeout against `codeload.github.com`** because the ephemeral,
`--cap-drop=ALL` container has no persistent action cache across runs and re-downloads the
pinned action archive fresh every single job. `ci.yml`, `core-ci.yml`, `distro-matrix.yml`,
`nightly.yml` and `windows-atoms.yml` deliberately stay on the mutable `@v4` tag -- they run
on GitHub-hosted, ephemeral VMs, which the security guide itself calls the
**"well-understood, generally-accepted risk"** tier: the blast radius of a compromised `@v4`
there is a throwaway VM, not the lead's own machine. **Record this trade-off as a conscious
choice in code comments (as `heavy.yml` already does), not as something a future edit should
"fix" by dropping the pin for convenience.**

PT: A frase do próprio guia: **"Pinning to a particular SHA helps mitigate the risk of a bad
actor adding a backdoor to the action's repository, as they would need to generate a SHA-1
collision"** [nota: o pin por SHA do GitHub Actions usa o hash Git completo de 40 caracteres;
a transição do Git pra longe do SHA-1 propenso a colisão não muda a orientação operacional
aqui]. Uma tag, em contraste, **"can be moved or deleted if a bad actor gains access to the
repository storing the action"** -- pinar `@v4` confia que `v4` sempre vai apontar pro código
que você revisou; pinar `@11d5960a...` não confia em nada além dos bytes que você já
auditou. Este é exatamente o **padrão tj-actions/changed-files de 2025** que o próprio
comentário do nosso `heavy.yml` cita: uma conta de mantenedor comprometida repontea uma tag
mutável, e todo consumidor pinado naquela tag puxa o código novo e malicioso na próxima
execução sem diff nenhum pra revisar.

### Amarração obrigatória -- lição 4 / Mandatory tie-in -- lesson 4

**Esta é a escolha CERTA de segurança, e criou fragilidade operacional real, medida neste
repo.** As duas coisas são verdade; não trate a fragilidade como defeito a ser silenciosamente
remendado. Os dois jobs do `heavy.yml` (`sanitize`, `fonteng`) fixam `actions/checkout` e
`actions/cache` por SHA de commit completo -- porque rodam no próprio runner self-hosted deste
repo (ver §6), um nível de risco materialmente diferente das mesmas actions numa VM
GitHub-hosted descartável. **Três etapas falharam com timeout de 100 segundos contra
`codeload.github.com`** porque o container efêmero, `--cap-drop=ALL`, não tem cache
persistente de action entre execuções e rebaixa o arquivo pinado da action do zero a cada
job. `ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml` e `windows-atoms.yml` ficam
deliberadamente na tag mutável `@v4` -- rodam em VMs GitHub-hosted, efêmeras, que o próprio
guia de segurança chama de nível **"well-understood, generally-accepted risk"**: o raio de
explosão de um `@v4` comprometido ali é uma VM descartável, não a máquina do líder.
**Registre esta troca como escolha consciente em comentário de código (como o `heavy.yml` já
faz), não como algo que uma edição futura deveria "consertar" tirando o pin por
conveniência.**

---

## 6. Executores auto-hospedados em repositório público / Self-hosted runners in a public repo

### Amarração obrigatória -- lição 5, e o achado mais valioso desta fatia / Mandatory tie-in -- lesson 5, and the most valuable finding of this slice

EN: The security hardening guide's own words, quoted directly: **"self-hosted runners should
almost never be used for public repositories on GitHub, because any user can open pull
requests against the repository and compromise the environment."** It adds that self-hosted
runners **"do not have guarantees around running in ephemeral clean virtual machines, and can
be persistently compromised by untrusted code in a workflow."**

⚠️ **This is more restrictive than our own practice, and that is the finding this whole slice
exists to surface.** Our stated policy (`heavy.yml`'s own header comment) is narrower and more
permissive than GitHub's blanket recommendation: we run a self-hosted runner on a public repo,
mitigated by removing the `pull_request` trigger entirely (only `push` to `main` and
`workflow_dispatch` reach it). GitHub's own official guidance for **runner group access
policy** goes further still -- fetched from
`docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/manage-access`,
literal quote: **"We recommend that you only use self-hosted runners with private
repositories. This is because forks of your public repository can potentially run dangerous
code on your self-hosted runner machine."** The same page states the runner group's **default
access policy already restricts self-hosted runners to private repositories only** -- meaning
an organization has to take an explicit, opt-in action to let a self-hosted runner attach to
ANY public repository at all, regardless of trigger.

**What this means concretely:** removing `pull_request` (our mitigation) closes the specific,
well-documented "pwn request" attack path -- a fork's PR can no longer execute code on the
runner. It does **not** address the two other things GitHub's blanket recommendation is
actually about: (a) any user with normal write access to the repo (not just fork PRs) can
still push directly to `main` and trigger `heavy.yml`'s self-hosted jobs -- for a
single-maintainer repo this is the same trust boundary as "the lead's own machine," so it
is not a new risk, but it is worth stating explicitly rather than silently assuming; (b)
GitHub's language about runners not being guaranteed "ephemeral clean virtual machines" is
addressed here by our OWN engineering (see below), not by anything GitHub enforces --
GitHub's platform-level guarantee stops at "we recommend against this," the rest is on us.

**What we do that GitHub's guide does not require, but that closes gap (b) above** --
verified directly against `tools/ci/runner_up.sh` and `heavy.yml`, not just their comments:
`--ephemeral` runner (one job, then destroyed), `--memory=8g --memory-swap=8g` (hard memory
ceiling, zero swap), `--cpus=4`, `--pids-limit=4096`, **`--cap-drop=ALL`**,
**`--security-opt=no-new-privileges`**, no `container:` key inside the workflow (the runner
itself IS the sandboxed container, so there is no docker-in-docker escape hatch), a
**mechanically-checked anti-fork gate** (`tools/check_workflow_self_hosted_gate.py`, run on
every PR on a GitHub-hosted runner, structurally parses every workflow's YAML with PyYAML --
not a `grep`, because a raw `grep pull_request heavy.yml` would match that file's own 4
explanatory comment lines and give a false sense of coverage) that fails the build if any
workflow with a `self-hosted` job ever also declares `pull_request`/`pull_request_target`/
`issue_comment`, and a **memory gate** (`tools/ci/wait_for_memory.sh`) that waits for host
headroom before letting a build compete with the lead's own interactive session. None of this
is documented by GitHub as a requirement -- it is this project's own defense-in-depth answer
to running a self-hosted runner at all on a public repo, which GitHub's own guidance says to
avoid outright.

**The honest conclusion to bring to the lead:** our practice is a deliberate, narrower
exception to GitHub's blanket "don't" -- justified by (1) removing the specific fork-PR
attack path via the trigger gate, (2) hardening the container itself well beyond GitHub's
platform guarantees, and (3) the practical reality that the only alternative GitHub actually
recommends (private repo, or no self-hosted runner) is incompatible with this project's other
constraint of being public + needing an ASan/UBSan + Fedora-class runner GitHub does not sell
as a hosted option. This is a risk-accepted deviation, not an oversight -- but it should be
named as a deviation from official guidance, in writing, to the lead, not silently assumed
"fine" because our own mitigations are strong.

PT: A frase do próprio guia de hardening de segurança, citada direto: **"self-hosted runners
should almost never be used for public repositories on GitHub, because any user can open pull
requests against the repository and compromise the environment."** Acrescenta que runners
self-hosted **"do not have guarantees around running in ephemeral clean virtual machines, and
can be persistently compromised by untrusted code in a workflow."**

⚠️ **Isso é mais restritivo que a nossa própria prática, e esse é o achado que esta fatia
inteira existe para expor.** A nossa política declarada (comentário de cabeçalho do próprio
`heavy.yml`) é mais estreita e mais permissiva que a recomendação geral do GitHub: rodamos
um runner self-hosted num repo público, mitigado removendo o gatilho `pull_request` por
completo (só `push` na `main` e `workflow_dispatch` alcançam ele). A própria orientação
oficial do GitHub para **política de acesso de grupo de runner** vai ainda mais longe --
extraída de
`docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/manage-access`,
citação literal: **"We recommend that you only use self-hosted runners with private
repositories. This is because forks of your public repository can potentially run dangerous
code on your self-hosted runner machine."** A mesma página afirma que a **política default de
acesso do grupo de runner já restringe runners self-hosted a repositórios privados apenas**
-- ou seja, uma organização precisa de uma ação explícita, opt-in, para deixar um runner
self-hosted anexar em QUALQUER repositório público, independente de gatilho.

**O que isso significa concretamente:** remover `pull_request` (a nossa mitigação) fecha o
caminho de ataque específico e bem documentado de "pwn request" -- um PR de fork não
consegue mais executar código no runner. Isso **não** endereça as outras duas coisas que a
recomendação geral do GitHub de fato trata: (a) qualquer usuário com acesso normal de escrita
ao repo (não só PR de fork) ainda consegue empurrar direto pra `main` e disparar os jobs
self-hosted do `heavy.yml` -- para um repo de mantenedor único isso é a mesma fronteira de
confiança que "a própria máquina do líder", então não é um risco novo, mas vale dizer
explicitamente em vez de assumir em silêncio; (b) a linguagem do GitHub sobre runners não
terem garantia de "ephemeral clean virtual machines" é endereçada aqui pela NOSSA própria
engenharia (ver abaixo), não por nada que o GitHub garanta -- a garantia de plataforma do
GitHub para em "recomendamos contra isto", o resto é conosco.

**O que fazemos que o guia do GitHub não exige, mas que fecha a lacuna (b) acima** --
verificado diretamente contra `tools/ci/runner_up.sh` e `heavy.yml`, não só os comentários
deles: runner **`--ephemeral`** (um job, depois destruído), **`--memory=8g
--memory-swap=8g`** (teto duro de memória, swap zerado), **`--cpus=4`**,
**`--pids-limit=4096`**, **`--cap-drop=ALL`**, **`--security-opt=no-new-privileges`**,
nenhuma chave `container:` dentro do workflow (o próprio runner É o container sandboxado,
então não existe saída via docker-in-docker), um **gate anti-fork checado
mecanicamente** (`tools/check_workflow_self_hosted_gate.py`, roda em todo PR num runner
GitHub-hosted, parseia estruturalmente o YAML de todo workflow com PyYAML -- não um `grep`,
porque um `grep pull_request heavy.yml` cru casaria as 4 linhas de comentário explicativo
daquele arquivo e daria falsa sensação de cobertura) que falha o build se algum workflow com
job `self-hosted` também declarar `pull_request`/`pull_request_target`/`issue_comment`, e um
**gate de memória** (`tools/ci/wait_for_memory.sh`) que espera folga no host antes de deixar
um build competir com a própria sessão interativa do líder. Nada disso é exigido pelo GitHub
como requisito -- é a resposta de defesa em profundidade deste projeto para rodar um runner
self-hosted de qualquer jeito num repo público, o que a própria orientação do GitHub diz para
evitar de saída.

**A conclusão honesta a levar ao líder:** a nossa prática é uma exceção deliberada e mais
estreita ao "não faça" geral do GitHub -- justificada por (1) remover o caminho de ataque
específico de PR de fork via o gate de gatilho, (2) endurecer o próprio container muito além
das garantias de plataforma do GitHub, e (3) a realidade prática de que a única alternativa
que o GitHub de fato recomenda (repo privado, ou nenhum runner self-hosted) é incompatível
com a outra restrição deste projeto de ser público + precisar de um runner classe
ASan/UBSan+Fedora que o GitHub não vende como opção hospedada. Este é um desvio de risco
aceito, não um descuido -- mas precisa ser nomeado como desvio da orientação oficial, por
escrito, ao líder, não assumido "de boa" em silêncio só porque as nossas próprias mitigações
são fortes.

---

## 7. Injeção de script, `pull_request_target`, e o que fazer em vez disso / Script injection, `pull_request_target`, and what to do instead

EN: **Untrusted input contexts** (CWE-78/CWE-94 class): the guide flags any context field
ending in `body`, `default_branch`, `email`, `head_ref`, `label`, `message`, `name`,
`page_name`, `ref`, or `title` as attacker-controlled when it originates from a fork or an
issue/PR a stranger can open -- e.g. `github.event.issue.title`,
`github.event.pull_request.body`. Less obvious sources exist too: **branch names and email
addresses**, with the documentation's own literal exploit example being a branch named
`zzz";echo${IFS}"hello";#`. The canonical exploit: a `run:` step that inline-interpolates
`${{ github.event.pull_request.title }}` into a shell string
(`title="${{ github.event.pull_request.title }}"`) lets an attacker open a PR titled
`a"; ls $GITHUB_WORKSPACE"` -- the closing quote breaks out of the variable assignment and
executes arbitrary shell.

**Mitigation the guide recommends:** never interpolate untrusted context directly into
`run:`. Pass it through an **intermediate environment variable** first:

```yaml
env:
  TITLE: ${{ github.event.pull_request.title }}
run: |
  if [[ "$TITLE" =~ ^octocat ]]; then
```

This works because "the context value is not used to generate a shell script, but is instead
passed to the action as an argument" -- the shell's own variable-expansion rules, not string
interpolation into the script text, handle the value.

**`pull_request_target`**: fetched in full (§ dedicated). It runs with the BASE repository's
elevated `GITHUB_TOKEN` (read/write, secrets available) while still being triggerable by a
fork's PR. The dangerous combination -- called a **"pwn request"** in the doc's own words --
is checking out the fork's code (`actions/checkout` with the PR's head/merge ref) and then
building/testing/running it: **"Checking out a pull request's head or merge commit in
actions/checkout and then building, testing, or otherwise executing the result"** runs
attacker-controlled `Makefile`/build-script/dependency/config content with the base repo's
secrets. Execution does not need an explicit build step -- `npm install`, `npm run build`,
and any dependency resolution the code triggers, can all run attacker code. **Primary
alternative: use `pull_request` if elevated secret access is not needed** -- it gets a
read-only token and no secrets by default for fork PRs. If `pull_request_target` genuinely is
needed, the guide's own required mitigations are: least-privilege `permissions:`, never
checking out untrusted ref content (or if unavoidable, treating it strictly as inert data,
never executed), read-only cache access is already the platform default for this trigger, and
(as of `actions/checkout` v7+) an explicit opt-out flag (`allow-unsafe-pr-checkout: true`) is
required to even attempt a fork checkout under this trigger -- a deliberate speed bump.

### Amarração com este repo / Tied to this repo

Confirmed by direct `grep` across all 6 workflow files: **zero occurrences of
`pull_request_target`**, and **zero interpolation of `github.event.*`/`github.head_ref` inside
any `run:` block** (grep for `github.event` and `head_ref` across the whole
`.github/workflows/` tree returned nothing). This repo has no PR-title/branch-name-driven
automation at all today, so this specific class of finding is clean by absence of the feature,
not by a deliberate mitigation being exercised -- worth remembering if a future slice adds,
say, an auto-labeling or PR-comment-triggered workflow: the intermediate-env-var pattern above
is the one to reach for on day one, not to retrofit after an incident.

PT: **Contextos de entrada não confiável** (classe CWE-78/CWE-94): o guia sinaliza qualquer
campo de contexto terminado em `body`, `default_branch`, `email`, `head_ref`, `label`,
`message`, `name`, `page_name`, `ref` ou `title` como controlado por atacante quando se
origina de um fork ou de uma issue/PR que um estranho pode abrir -- ex.:
`github.event.issue.title`, `github.event.pull_request.body`. Existem fontes menos óbvias
também: **nomes de branch e endereços de email**, com o exemplo literal de exploração da
própria documentação sendo um nome de branch `zzz";echo${IFS}"hello";#`. O exploit canônico:
um passo `run:` que interpola inline `${{ github.event.pull_request.title }}` numa string de
shell (`title="${{ github.event.pull_request.title }}"`) deixa um atacante abrir um PR com
título `a"; ls $GITHUB_WORKSPACE"` -- a aspa de fechamento quebra a atribuição de variável e
executa shell arbitrário.

**Mitigação que o guia recomenda:** nunca interpolar contexto não confiável direto em
`run:`. Passá-lo por uma **variável de ambiente intermediária** primeiro:

```yaml
env:
  TITLE: ${{ github.event.pull_request.title }}
run: |
  if [[ "$TITLE" =~ ^octocat ]]; then
```

Isso funciona porque "the context value is not used to generate a shell script, but is
instead passed to the action as an argument" -- as regras de expansão de variável do próprio
shell, não interpolação de string no texto do script, tratam o valor.

**`pull_request_target`**: lido por inteiro (seção dedicada). Roda com o `GITHUB_TOKEN`
elevado do repositório BASE (leitura/escrita, segredos disponíveis) mesmo sendo disparável
por PR de fork. A combinação perigosa -- chamada de **"pwn request"** nas próprias palavras
da doc -- é fazer checkout do código do fork (`actions/checkout` com o ref de head/merge do
PR) e depois buildar/testar/rodar: **"Checking out a pull request's head or merge commit in
actions/checkout and then building, testing, or otherwise executing the result"** roda
conteúdo de `Makefile`/script de build/dependência/config controlado por atacante com os
segredos do repo base. Execução não precisa de passo de build explícito -- `npm install`,
`npm run build`, e qualquer resolução de dependência que o código dispare, todos podem rodar
código do atacante. **Alternativa primária: usar `pull_request` se acesso elevado a segredo
não for necessário** -- ele recebe token só-leitura e nenhum segredo por default para PR de
fork. Se `pull_request_target` for de fato necessário, as mitigações exigidas pelo próprio
guia são: `permissions:` de menor privilégio, nunca fazer checkout de conteúdo de ref não
confiável (ou, se inevitável, tratá-lo estritamente como dado inerte, nunca executado), acesso
de cache já é read-only por default de plataforma para este gatilho, e (a partir do
`actions/checkout` v7+) uma flag explícita de opt-out (`allow-unsafe-pr-checkout: true`) é
exigida para sequer tentar um checkout de fork sob este gatilho -- um freio deliberado.

### Amarração com este repo / Tied to this repo

Confirmado por `grep` direto nos 6 arquivos de workflow: **zero ocorrências de
`pull_request_target`**, e **zero interpolação de `github.event.*`/`github.head_ref` dentro
de qualquer bloco `run:`** (grep por `github.event` e `head_ref` na árvore inteira de
`.github/workflows/` não devolveu nada). Este repo não tem automação nenhuma hoje guiada por
título-de-PR/nome-de-branch, então este achado específico está limpo por ausência da
feature, não por uma mitigação deliberada sendo exercitada -- vale lembrar se uma fatia
futura acrescentar, digamos, auto-rotulagem ou um workflow disparado por comentário de PR: o
padrão de variável-de-ambiente-intermediária acima é o que se busca desde o dia um, não algo
pra remendar depois de um incidente.

---

## 8. Artefatos e logs como vetor de vazamento / Artifacts and logs as a leakage vector

EN: The security guide flags two distinct leakage paths beyond secret masking itself:

- **Artifacts**: **"Workflows triggered on `workflow_run` should treat artifacts uploaded
  from other workflows with caution"** -- an artifact produced by one workflow and consumed
  by another (especially across a trust boundary, e.g. a `pull_request` build artifact later
  consumed by a privileged `workflow_run`) is attacker-influenced data if the producing
  workflow could be triggered by an untrusted actor. **On a PUBLIC repository, any artifact
  uploaded via `actions/upload-artifact` is downloadable by anyone with read access to the
  repository** (i.e. anyone, for a public repo) unless the repository/org restricts artifact
  and log access -- this is a platform behavior, not something a workflow author opts into
  per-artifact.
- **Logs**: **"It's not always obvious how a command or tool you're invoking will send errors
  to `STDOUT` and `STDERR`, and secrets might subsequently end up in error logs."** The
  guide's own recommendation is to manually review logs after testing a step with both valid
  AND invalid inputs -- error paths are exactly where an unmasked credential tends to surface
  (a failed `curl` printing its own `Authorization:` header, a stack trace embedding a
  connection string), because masking only catches values GitHub already knows are secrets.

PT: O guia de segurança sinaliza dois caminhos distintos de vazamento além do próprio
mascaramento de segredo:

- **Artefatos**: **"Workflows triggered on `workflow_run` should treat artifacts uploaded
  from other workflows with caution"** -- um artefato produzido por um workflow e consumido
  por outro (especialmente atravessando uma fronteira de confiança, ex.: um artefato de build
  de `pull_request` depois consumido por um `workflow_run` privilegiado) é dado influenciado
  por atacante se o workflow produtor pudesse ser disparado por um ator não confiável. **Em
  um repositório PÚBLICO, todo artefato subido via `actions/upload-artifact` é baixável por
  qualquer um com acesso de leitura ao repositório** (ou seja, qualquer pessoa, num repo
  público) a menos que o repositório/organização restrinja acesso a artefato e log -- isto é
  comportamento de plataforma, não algo que o autor do workflow escolhe por artefato.
- **Logs**: **"It's not always obvious how a command or tool you're invoking will send errors
  to `STDOUT` and `STDERR`, and secrets might subsequently end up in error logs."** A própria
  recomendação do guia é revisar logs manualmente depois de testar um passo com entrada
  válida E inválida -- caminhos de erro são exatamente onde uma credencial não-mascarada
  tende a aparecer (um `curl` que falhou imprimindo o próprio header `Authorization:`, um
  stack trace embutindo string de conexão), porque o mascaramento só pega valores que o
  GitHub já sabe serem segredos.

### Amarração com este repo / Tied to this repo

EN: `ci.yml`'s `coverage` job is the **only** point across all 6 workflows that uploads an
artifact (`actions/upload-artifact@v4`, the HTML coverage report, `retention-days: 30`). It
carries source-code coverage annotations, not secrets -- low individual risk, but because this
repo is public, that artifact (and every job's raw log output) is world-readable by
construction, which is exactly why lesson §3's "zero `secrets.*` usage across all 6 workflows"
finding matters as a structural mitigation, not a coincidence to be relaxed casually if a
future job needs a credential.

PT: O job `coverage` do `ci.yml` é o **único** ponto entre os 6 workflows que sobe artefato
(`actions/upload-artifact@v4`, o relatório HTML de cobertura, `retention-days: 30`). Carrega
anotação de cobertura de código-fonte, não segredo -- risco individual baixo, mas como este
repo é público, aquele artefato (e o log bruto de todo job) é legível pelo mundo por
construção, que é exatamente por que o achado da §3 ("zero uso de `secrets.*` em todos os 6
workflows") importa como mitigação estrutural, não coincidência a ser relaxada casualmente se
um job futuro precisar de uma credencial.

---

## 9. Auditoria de conformidade dos 6 fluxos / Conformance audit of our 6 workflows

EN: Every finding below was verified directly against the checked-out `.yml` files and, where
noted, against `gh api`/`tools/ci/*` -- not inferred from comments. Denominator declared for
every row: **6 workflows** unless stated otherwise.

PT: Todo achado abaixo foi verificado diretamente contra os arquivos `.yml` do checkout e,
quando anotado, contra `gh api`/`tools/ci/*` -- não inferido de comentário. Denominador
declarado em toda linha: **6 workflows**, salvo quando dito o contrário.

| Pergunta | Resposta medida (denominador) | Evidência |
|---|---|---|
| Todo fluxo declara `permissions:` explicitamente? | **1 de 6** (`heavy.yml`, `contents: read`) — os outros 5 (`ci.yml`, `core-ci.yml`, `distro-matrix.yml`, `nightly.yml`, `windows-atoms.yml`) não declaram, e herdam o default do repositório | `grep -n "^permissions:" *.yml` -- 1 match |
| Default do `GITHUB_TOKEN` a nível de repositório | `"read"` (correto/least-privilege) | `gh api repos/petrinhu/glintfx/actions/permissions/workflow` → `default_workflow_permissions: "read"` |
| Algum fluxo usa `pull_request_target`? | **0 de 6** | `grep -rn "pull_request_target" .` -- vazio |
| Alguma entrada não confiável (`github.event.*`, `head_ref`) interpolada em `run:`? | **0 de 6** — nenhuma ocorrência de `github.event` nem `head_ref` em nenhum arquivo | `grep -rn "github.event\|head_ref" .` -- vazio |
| Uso de `secrets.*` | **0 de 6** | `grep -rn "secrets\." .` -- vazio |
| Uso de `environment:` (GitHub Environments) | **0 de 6** | `grep -rn "environment:" .` -- vazio |
| Pontos de ação de terceiro por SHA vs por tag | **17 por tag, 4 por SHA (21 pontos no total)** — ⚠️ **diverge do número enunciado no briefing ("16 por tag, 4 por SHA" = 20). Contei por conta própria e o total real é 21, com 17 por tag, não 16.** As 4 fixadas por SHA são as duas (`checkout`+`cache`) usadas 2× em `heavy.yml` (jobs `sanitize` e `fonteng`) | `grep -rhoE "uses: [a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+@[a-f0-9]{40}"` = 4; `...@v[0-9]` = 17; soma manual das linhas listadas = 21 |
| O fluxo do runner self-hosted (`heavy.yml`) tem gatilho `pull_request`? | **Não.** Só `push: branches: [main]` (filtrado por paths) + `workflow_dispatch` | Leitura completa do bloco `on:` de `heavy.yml`, linhas 102-115 |
| Gate mecânico que impede `pull_request` chegar a um job `self-hosted` | **Sim, existe e roda em todo PR** — `tools/check_workflow_self_hosted_gate.py`, passo `SEC-CI-HARDEN` no job `lint-and-scan` de `ci.yml` | Leitura de `ci.yml` linhas 452-489 + cabeçalho do próprio script |
| Endurecimento de container do runner self-hosted (além do que o GitHub exige) | `--ephemeral`, `--memory=8g --memory-swap=8g`, `--cpus=4`, `--pids-limit=4096`, `--cap-drop=ALL`, `--security-opt=no-new-privileges`, sem `container:` na composição do workflow | `grep` em `tools/ci/runner_up.sh` |
| Artefatos publicados (repo público → legível por qualquer um) | **1 de 6** — `coverage` job do `ci.yml`, `actions/upload-artifact@v4`, `retention-days: 30` | `grep -rn "upload-artifact" .` |
| Uso de `concurrency:` (mitiga corrida/DoS de fila) | **1 de 6** — `heavy.yml`, `group: heavy-${{ github.ref }}, cancel-in-progress: true` | `grep -rn "^concurrency:" .` |

---

## Relatório para o orquestrador / Report to the orchestrator

EN:

- **Pages read**: 16 URLs fetched with content extraction (4 nav-index pages returned mostly
  link lists, 12 returned substantive quoted content). Full list and declared out-of-scope
  exclusions in the "Método e denominador" section above.
- **Topics the documentation covers that we do NOT apply**:
  1. **Explicit `permissions:` in 5 of 6 workflows** — currently protected only by the
     repo-level default (`"read"`, confirmed correct), not by the workflow file itself. This
     is the guide's own top recommendation and our weakest point of the whole audit.
  2. **OIDC** — not used anywhere, but also not needed anywhere today (no cloud deploy step
     exists). Flagging as a "when it appears" note, not a gap.
  3. **Artifact attestations / SLSA provenance** — a real GitHub feature for supply-chain
     trust, entirely unused; out of this slice's depth but worth a future look given this
     project already cares about supply chain (pinned-by-SHA reasoning, `grype`/`gitleaks`
     gates).
- **The single most important contradiction found**: **our self-hosted runner practice is a
  narrower, deliberate exception to a GitHub recommendation that is stricter than "no
  `pull_request` trigger" — GitHub's default runner-group policy restricts self-hosted
  runners to private repositories entirely, and its guidance says public-repo self-hosted
  runners "should almost never be used," full stop, independent of trigger.** Our own
  container hardening (ephemeral, capped, `cap-drop=ALL`, no-new-privileges, mechanically
  gated against `pull_request` ever reaching it) is real and substantial, but it is *our*
  compensating control, not something GitHub's platform guarantees or endorses for this
  configuration. This should be named to the lead explicitly as an accepted, documented
  deviation — not silently treated as "compliant" because the specific fork-PR path is
  closed.
- **Secondary, smaller finding**: the number given in the brief ("16 by tag, 4 by SHA") does
  not match an independent recount of the same 6 files — the real total is **17 by tag, 4 by
  SHA (21 points)**. Reported per the project's own house rule that a given number must be
  re-verified, not trusted.
- **Not conserted, only reported**, per the restrictions in `CONTEXTO.md`: no workflow file
  was touched.

PT:

- **Páginas lidas**: 16 URLs com extração de conteúdo (4 páginas de índice de navegação
  devolveram principalmente listas de link, 12 devolveram conteúdo substantivo citado). Lista
  completa e exclusões declaradas de fora-de-escopo na seção "Método e denominador" acima.
- **Tópicos que a documentação cobre e que NÃO aplicamos**:
  1. **`permissions:` explícito em só 1 de 6 workflows** — hoje protegido só pelo default de
     nível de repositório (`"read"`, confirmado correto), não pelo próprio arquivo de
     workflow. É a recomendação número um do próprio guia, e o ponto mais fraco de toda a
     auditoria.
  2. **OIDC** — não usado em lugar nenhum, mas também não necessário em lugar nenhum hoje
     (não existe passo de deploy em nuvem). Sinalizado como nota "para quando aparecer", não
     como lacuna.
  3. **Artifact attestations / proveniência SLSA** — feature real do GitHub para confiança de
     supply-chain, totalmente não usada; fora da profundidade desta fatia, mas vale um olhar
     futuro dado que este projeto já se importa com supply-chain (racional de pin-por-SHA,
     gates de `grype`/`gitleaks`).
- **A contradição mais importante achada**: **a nossa prática de runner self-hosted é uma
  exceção mais estreita e deliberada a uma recomendação do GitHub que é mais restritiva que
  "sem gatilho `pull_request`" — a política default de grupo de runner do GitHub restringe
  runner self-hosted a repositório privado por completo, e a orientação diz que runner
  self-hosted em repo público "should almost never be used", ponto final, independente de
  gatilho.** O nosso próprio endurecimento de container (efêmero, com teto, `cap-drop=ALL`,
  no-new-privileges, gateado mecanicamente contra `pull_request` nunca chegar lá) é real e
  substancial, mas é controle COMPENSATÓRIO nosso, não algo que a plataforma do GitHub
  garante ou endossa para esta configuração. Isto deveria ser nomeado ao líder
  explicitamente como desvio aceito e documentado — não tratado em silêncio como "conforme"
  só porque o caminho específico de PR de fork está fechado.
- **Achado secundário, menor**: o número dado no briefing ("16 por tag, 4 por SHA") não bate
  com uma recontagem independente dos mesmos 6 arquivos — o total real é **17 por tag, 4 por
  SHA (21 pontos)**. Reportado conforme a regra da casa deste projeto de que um número dado
  precisa ser re-verificado, não confiado de cabeça.
- **Nada consertado, só reportado**, conforme as restrições do `CONTEXTO.md`: nenhum arquivo
  de workflow foi tocado.
