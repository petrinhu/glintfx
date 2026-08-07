# Field notes: GitHub Actions survey (`DOC-GHA-CANONICO`, 2026-08-06)

**EN:** Raw field notes from the four-agent survey of the official GitHub Actions documentation.
**These are the audit trail, not the deliverable.** The deliverable is
[`../github-actions.md`](../github-actions.md): a 52 KB synthesis that deliberately cut ~82% of
this material, keeping what is *ours* (the measurements, the incidents, the traps) and replacing
generic restatement of the official docs with links.

**Why keep the raw notes at all, then?** Because the synthesis kept **25** source links and these
notes hold **80**. If someone later needs to verify a claim against the page it came from, the
trail lives here. Cutting for readability is right; losing the evidence is not.

⚠️ **These were originally written to a temporary directory (`/var/tmp/gha-docs/`) and moved here
by the líder's instruction: knowledge does not live in a folder that a cleanup can erase.** That
correction is itself worth remembering.

**Coverage, declared:** 74 pages of `docs.github.com/pt/actions` read across four sections
(10 + 27 + 16 + 21). "Migrating to GitHub Actions" was **deliberately excluded** as inapplicable
to this project, declared so the denominator stays honest.

**Not maintained.** These are a snapshot of what the documentation said on 2026-08-06. The
synthesis is the living document; if the two disagree, the synthesis wins and these notes are
evidence of what changed.

| File | Section | Pages read |
|---|---|---|
| `00-contexto-do-levantamento.md` | The brief all four agents shared: what not to produce, the 7 measured lessons, method rules | (n/a) |
| `referencia.md` | Workflow syntax, events, variables, expressions, contexts | 10 |
| `escrever.md` | Triggers, jobs, matrix, cache, artifacts, reuse, authoring actions | 27 |
| `seguranca.md` | Hardening, `GITHUB_TOKEN` permissions, secrets, OIDC, SHA pinning, self-hosted runners | 16 |
| `operacao.md` | Managing runs, monitoring, diagnosing, quotas, service outages | 21 |

---

# Notas de campo: levantamento de GitHub Actions (`DOC-GHA-CANONICO`, 2026-08-06)

**PT:** Notas de campo brutas do levantamento da documentação oficial de GitHub Actions, feito por
quatro agentes. **Isto é o rastro de auditoria, não o entregável.** O entregável é
[`../github-actions.md`](../github-actions.md): uma síntese de 52 KB que cortou ~82% deste
material de propósito, guardando o que é *nosso* (as medições, os incidentes, as armadilhas) e
trocando a repetição genérica da documentação oficial por links.

**Então por que guardar as notas brutas?** Porque a síntese ficou com **25** links de origem e
estas notas guardam **80**. Se alguém precisar conferir uma afirmação contra a página de onde ela
veio, o rastro mora aqui. Cortar por legibilidade está certo; **perder a evidência não**.

⚠️ **Estas notas nasceram numa pasta temporária (`/var/tmp/gha-docs/`) e foram movidas para cá
por ordem do líder: conhecimento não mora em pasta que uma limpeza apaga.** A própria correção
vale como lição.

**Cobertura, declarada:** 74 páginas de `docs.github.com/pt/actions` lidas nas quatro seções
(10 + 27 + 16 + 21). A seção "Migrar para o GitHub Actions" foi **deliberadamente excluída** por
não se aplicar a este projeto, declarada para o denominador ficar honesto.

**Não são mantidas.** São um retrato do que a documentação dizia em 2026-08-06. A síntese é o
documento vivo; se as duas divergirem, a síntese vence e estas notas são evidência do que mudou.

| Arquivo | Seção | Páginas lidas |
|---|---|---|
| `00-contexto-do-levantamento.md` | O briefing comum aos quatro agentes: o que não produzir, as 7 lições medidas, regras de método | (n/a) |
| `referencia.md` | Sintaxe de fluxo, eventos, variáveis, expressões, contextos | 10 |
| `escrever.md` | Gatilhos, jobs, matriz, cache, artefatos, reuso, autoria de ações | 27 |
| `seguranca.md` | Endurecimento, permissões do `GITHUB_TOKEN`, segredos, OIDC, fixação por SHA, executores auto-hospedados | 16 |
| `operacao.md` | Gerenciar execuções, monitorar, diagnosticar, cotas, indisponibilidade do serviço | 21 |
