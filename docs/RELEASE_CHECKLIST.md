# Release checklist / Checklist de release

> **EN:** Short, pre-`git tag vX.Y.Z` checklist. Bilingual: English first, then Português.
> **PT:** Checklist curto, pré-`git tag vX.Y.Z`. Bilíngue: inglês primeiro, depois português.

---

## English

### Why this exists

Three real drift cases already happened in this repository, each caught late by an unrelated audit instead of at release time:

1. **`SECURITY.md`'s "Supported versions" table** stayed locked to `0.4.x` for several minor releases while the project had already reached `0.11.0` — caught by `AUD-L1-PARSE` (2026-07-16), the same drift class as the historical `AUD-TEC-8` finding. `SECURITY.md` now carries a process note asking future releases not to repeat this.
2. **`AUDITORIAS.md`/`TODO.md`'s own catalog prose** kept citing "gl3w vendorizado" as a live dependency for three releases after gl3w was actually removed (`v0.8.0`, `L1.14-GLLOADER`, replaced by glintfx's own clean-room GL loader) — independently flagged by both `AUD-L1-SUPPLY` and `AUD-L1-LICENSE` (2026-07-16), each noting it was outside that deliverable's own write scope to fix.
3. **`CONTRIBUTING.md`'s "default suite has 5 tests"** line is a relic from `v0.1.x` (`window_smoke`/`render_smoke`/`bootstrap_smoke`/`app_smoke`/`render_sanity`) — the suite has grown to 52+ tests since, and the sentence was never revisited across ~10 releases.

None of these were security-critical, but each one is exactly the kind of thing an external auditor (or a new contributor) hits first and loses trust over. This checklist is the cheap, five-item gate that keeps it from recurring — run it once, right before tagging, not as an afterthought a later audit catches.

### The checklist

Run all five before `git tag vX.Y.Z`. Do not tag if (a) or (e) fails.

**(a) Full suite green on GitHub.** `.github/workflows/ci.yml` green on the commit being tagged: that is the release gate. The deep ASan+UBSan and own-font-engine legs run on `.github/workflows/heavy.yml`, a self-hosted containerized/ephemeral runner (8 GiB memory cap, swap off, `push`-to-`main` + `workflow_dispatch` triggers only, no `pull_request`); confirm the latest run on `main` is green before tagging (`gh run list --workflow=heavy.yml` or the Actions tab). If you need to reproduce a failure by hand: `tools/ci/build_image.sh`, then the sanitize build inside `docker run` against `localhost/glintfx-ci:f42` (see `AGENTS.md`'s CI-policy section).

**(b) `CHANGELOG.md` has a `[vX.Y.Z]` entry.** Dated, bilingual (EN then PT), following the existing entries' shape (a one-paragraph summary plus an "Added/Changed/Fixed" breakdown). `[Unreleased]` gets renamed to the tag, not left dangling.

**(c) Version/test-count prose is current in every place it lives.** Grep for the *previous* version string and a stale test count before tagging, not after:
   - 🔴 **`glintfx/CMakeLists.txt` — the `project(glintfx ... VERSION X.Y.Z)` line. THIS IS THE ONE THAT MATTERS MOST, and it was missing from this list until `v0.30.0`.** `config.hpp.in` injects `@PROJECT_VERSION@`, so a stale `project()` makes the *generated* `config.hpp` — and therefore `glintfx::version()` — **lie in the published, immutable tag**. It shipped wrong twice: in `v0.2.5` and again in `v0.30.0`, the second time caught by a consumer, not by us. A gate now enforces this (`REL-TAGCONSIST-GATE` in `.githooks/pre-push`), but the gate is the net, not the practice. **Verify the GENERATED header, not the declaration** — the consumer who found it only did so because their own bump ritual checks the generated artifact; ours checked the declaration, which was syntactically perfect and semantically wrong.
   - 🔴 **The `GIT_TAG vX.Y.Z)` pins inside the copy-pasteable `FetchContent` examples** — `README.md` (2 sites) and `docs/getting-started.md` (2 sites). This is **not cosmetic like a badge**: it is the snippet a new consumer copies to obtain the library. At `v0.30.0` the README was pinned to `v0.27.0` and `getting-started.md` to `v0.23.0` — seven releases stale, and the two install documents **disagreeing with each other**.
   - `SECURITY.md` — the "Supported versions" table's version line (both EN and PT tables).
   - `CONTRIBUTING.md` — the "default suite has N tests" sentence (both EN and PT); list the test names too if any changed.
   - `README.md` — the version badge (top of file), the "Current release: vX.Y.Z" line, the "Delivered (v0.2.x–vX.Y.Z)" range, the per-release history bullet (**EN and PT — this pair is where the fix goes half-done**), the "Layer 1 = glintfx ... (tag vX.Y.Z)" line, and the GLFW=ON/embed test counts mentioned in the CI bullet under "Known limitations".
   - `docs/getting-started.md` and `docs/wiki/Layer-1-glintfx.md` — version prose.
   - `docs/wiki/Home.md` badges (version + any test-count claim) — the wiki is a separate git repo (`glintfx.wiki.git` on GitHub); re-sync per `docs/wiki/README.md` if this file changed.

   ⚠️ **Do NOT grep blindly for the previous version string and replace every hit.** Some occurrences are *permanent historical markers* and must NOT move: `v0.28.0` in `CLAUDE.md`/`CONTRIBUTING.md` (the MPL-2.0 cut tag), `v0.29.0` in the ADR-0019 prose, and the entire per-release history list in `README.md`. The distinction is **"current release" vs "the release where X happened"**, and no regex separates the two — this is the reason the version gate is deliberately narrow.

**(d) `NOTICE` matches the real dependency graph.** Diff `NOTICE`'s entries against `glintfx/CMakeLists.txt`'s actual `FetchContent`/`find_package`/`target_link_libraries` blocks and `glintfx/third_party/`'s actual file tree. This is the artifact that matters for license compliance (not `AUDITORIAS.md`/`TODO.md`'s catalog prose, which is planning text, not a legal notice) — if a dependency was added, removed, or its FTL/MIT/Apache-2.0 status changed, `NOTICE` must reflect it before the tag, not after.

**(e) Zero open 🔴 CRITICAL in `docs/auditoria/`.** Check every report's verdict line and `docs/auditoria/README.md`'s findings-by-severity table. Per `AUDITORIAS.md`'s own rule, this is a hard release blocker, not a recommendation.

### After tagging

Push the tag to GitHub, confirm the CI run for the tag itself goes green, and — only then — update `docs/auditoria/README.md`'s "what changed since the last consolidation" note if this release touched anything audited.

---

## Português

### Por que isto existe

Três casos reais de drift já aconteceram neste repositório, cada um pego tarde por uma auditoria não-relacionada em vez de no momento da release:

1. **A tabela "Supported versions" do `SECURITY.md`** ficou travada em `0.4.x` por várias releases minor enquanto o projeto já tinha chegado à `0.11.0` — pego pelo `AUD-L1-PARSE` (2026-07-16), a mesma classe de drift do achado histórico `AUD-TEC-8`. O `SECURITY.md` agora carrega uma nota de processo pedindo que releases futuras não repitam isso.
2. **A prosa do próprio catálogo em `AUDITORIAS.md`/`TODO.md`** continuou citando "gl3w vendorizado" como dependência viva por três releases depois do gl3w ter sido de fato removido (`v0.8.0`, `L1.14-GLLOADER`, substituído pelo loader GL clean-room próprio da glintfx) — sinalizado de forma independente tanto pelo `AUD-L1-SUPPLY` quanto pelo `AUD-L1-LICENSE` (2026-07-16), cada um notando que estava fora do escopo de escrita daquele entregável pra corrigir.
3. **A linha "a suíte padrão tem 5 testes" do `CONTRIBUTING.md`** é uma relíquia da `v0.1.x` (`window_smoke`/`render_smoke`/`bootstrap_smoke`/`app_smoke`/`render_sanity`) — a suíte cresceu para 52+ testes desde então, e a frase nunca foi revisitada ao longo de ~10 releases.

Nenhum destes era crítico de segurança, mas cada um é exatamente o tipo de coisa que um auditor externo (ou um novo contribuidor) encontra primeiro e perde confiança por causa disso. Este checklist é o gate barato de cinco itens que evita a recorrência — rodar uma vez, bem antes de taggear, não como algo pego depois por uma auditoria futura.

### O checklist

Rodar os cinco antes do `git tag vX.Y.Z`. Não taggear se (a) ou (e) falhar.

**(a) Suíte completa verde no GitHub.** O `.github/workflows/ci.yml` verde no commit sendo taggeado: esse é o gate de release. As pernas profundas de ASan+UBSan e do motor de fonte próprio rodam no `.github/workflows/heavy.yml`, runner self-hosted conteinerizado e efêmero (teto de 8 GiB de memória, swap desligado, gatilho só `push` na `main` + `workflow_dispatch`, sem `pull_request`); confira que a última run na `main` está verde antes de taggear (`gh run list --workflow=heavy.yml` ou a aba Actions). Se precisar reproduzir uma falha à mão: `tools/ci/build_image.sh`, depois o build sanitize dentro de um `docker run` contra a `localhost/glintfx-ci:f42` (ver a seção de política de CI do `AGENTS.md`).

**(b) `CHANGELOG.md` tem uma entrada `[vX.Y.Z]`.** Datada, bilíngue (EN depois PT), seguindo a forma das entradas existentes (um resumo de um parágrafo mais uma repartição "Added/Changed/Fixed"). `[Unreleased]` é renomeado para a tag, não deixado pendurado.

**(c) Prosa de versão/contagem-de-teste está corrente em TODOS os lugares onde vive.** Fazer grep pela string de versão *anterior* e por uma contagem de teste obsoleta antes de taggear, não depois:
   - 🔴 **`glintfx/CMakeLists.txt` — a linha `project(glintfx ... VERSION X.Y.Z)`. É A QUE MAIS IMPORTA, e faltava desta lista até a `v0.30.0`.** O `config.hpp.in` injeta `@PROJECT_VERSION@`, então um `project()` obsoleto faz o `config.hpp` **gerado** — e portanto o `glintfx::version()` — **mentir na tag publicada, que é imutável**. Saiu errado duas vezes: na `v0.2.5` e de novo na `v0.30.0`, a segunda pega por um consumidor, não por nós. Hoje há gate (`REL-TAGCONSIST-GATE`, em `.githooks/pre-push`), mas o gate é a rede, não a prática. **Conferir o header GERADO, não a declaração** — o consumidor só achou porque o ritual de bump dele confere o artefato gerado; o nosso conferia a declaração, que estava sintaticamente perfeita e semanticamente errada.
   - 🔴 **Os pins `GIT_TAG vX.Y.Z)` dentro dos exemplos copiáveis de `FetchContent`** — `README.md` (2 sítios) e `docs/getting-started.md` (2 sítios). Isto **não é cosmético como um badge**: é o snippet que um consumidor novo copia para obter a biblioteca. Na `v0.30.0` o README estava pinado em `v0.27.0` e o `getting-started.md` em `v0.23.0` — sete releases atrás, e os dois documentos de instalação **divergindo entre si**.
   - `SECURITY.md` — a linha de versão da tabela "Supported versions"/"Versões suportadas" (as duas tabelas, EN e PT).
   - `CONTRIBUTING.md` — a frase "a suíte padrão tem N testes" (EN e PT); listar os nomes dos testes também se algum mudou.
   - `README.md` — o badge de versão (topo do arquivo), a linha "Current release: vX.Y.Z", a faixa "Entregue (v0.2.x–vX.Y.Z)", o bullet de histórico por-release (**EN e PT — este par é onde o conserto fica pela metade**), a linha "Camada 1 = glintfx ... (tag vX.Y.Z)", e as contagens de teste GLFW=ON/embed citadas no bullet de CI sob "Known limitations".
   - `docs/getting-started.md` e `docs/wiki/Layer-1-glintfx.md` — prosa de versão.
   - badges do `docs/wiki/Home.md` (versão + qualquer alegação de contagem de teste) — a wiki é um repo git separado (`glintfx.wiki.git` no GitHub); re-sincronizar conforme `docs/wiki/README.md` se este arquivo mudou.

   ⚠️ **NÃO fazer grep cego pela versão anterior e substituir toda ocorrência.** Algumas são *marcos históricos permanentes* e **não** podem se mover: `v0.28.0` no `CLAUDE.md`/`CONTRIBUTING.md` (a tag de corte da MPL-2.0), `v0.29.0` na prosa do ADR-0019, e a lista inteira de histórico por-release do `README.md`. A distinção é **"release atual" vs "a release em que X aconteceu"**, e nenhum regex separa as duas — é exatamente por isso que o gate de versão é deliberadamente estreito.

**(d) `NOTICE` bate com o grafo de dependências real.** Diff das entradas do `NOTICE` contra os blocos reais `FetchContent`/`find_package`/`target_link_libraries` do `glintfx/CMakeLists.txt` e a árvore de arquivos real de `glintfx/third_party/`. Este é o artefato que importa pra conformidade de licença (não a prosa de catálogo do `AUDITORIAS.md`/`TODO.md`, que é texto de planejamento, não um aviso legal) — se uma dependência foi adicionada, removida, ou o status FTL/MIT/Apache-2.0 dela mudou, o `NOTICE` precisa refletir isso antes da tag, não depois.

**(e) Zero 🔴 CRÍTICO aberto em `docs/auditoria/`.** Checar a linha de veredito de todo relatório e a tabela de achados-por-severidade do `docs/auditoria/README.md`. Conforme a própria regra do `AUDITORIAS.md`, isto é um bloqueio duro de release, não uma recomendação.

### Depois de taggear

Empurrar a tag pro GitHub, confirmar que o run de CI da própria tag fica verde, e — só então — atualizar a nota "o que mudou desde a última consolidação" do `docs/auditoria/README.md` se esta release tocou algo auditado.
