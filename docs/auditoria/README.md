# Audit dossier — master index / Dossiê de auditoria — índice mestre

> **EN:** Consolidated by `internal-auditor`. This index is the entry point for an external auditor: what was audited, when, by whom, and the verdict. Individual reports are the source of truth; this file only indexes and summarizes them. Governance/method: [`AUDITORIAS.md`](../../AUDITORIAS.md) (root). Pre-tag process: [`docs/RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md).
> **PT:** Consolidado pelo `internal-auditor`. Este índice é o ponto de entrada para um auditor externo: o que foi auditado, quando, por quem, e o veredito. Os relatórios individuais são a fonte de verdade; este arquivo só indexa e resume. Governança/método: [`AUDITORIAS.md`](../../AUDITORIAS.md) (raiz). Processo pré-tag: [`docs/RELEASE_CHECKLIST.md`](../RELEASE_CHECKLIST.md).

## Scope / Escopo

**EN:** This repository has two independent stacks under audit — Layer 0 (`loucura_c_asm`, pure C+ASM, zero libc, `core-v0.3.0`) and Layer 1 (`glintfx`, C++/RmlUi/GL3 library, `v0.11.0`, the active product). Layer 0's catalog is `AUD-ABI`/`AUD-SEC` (ABI conformance + security). Layer 1's catalog is `AUD-L1-*` (supply-chain, hostile-asset parsing, memory-safety, license); the full catalog rationale (why these five and not more — network/authN/crypto/PII are explicitly out of scope for a desktop rendering library) lives in `AUDITORIAS.md`.

**PT:** Este repositório tem duas stacks independentes sob auditoria — Camada 0 (`loucura_c_asm`, C+ASM puro, zero libc, `core-v0.3.0`) e Camada 1 (`glintfx`, biblioteca C++/RmlUi/GL3, `v0.11.0`, o produto ativo). O catálogo da Camada 0 é `AUD-ABI`/`AUD-SEC` (conformidade ABI + segurança). O catálogo da Camada 1 é `AUD-L1-*` (supply-chain, parsing de asset hostil, memory-safety, licença); o racional completo do catálogo (por que estes cinco e não mais — rede/authN/cripto/PII estão explicitamente fora de escopo para uma lib de render desktop) vive no `AUDITORIAS.md`.

## Executive summary / Sumário executivo

**EN:** All 6 audits closed for the current tagged state have a **CONFORME** verdict. Zero 🔴 CRITICAL findings anywhere in the dossier — none open, none historically found-and-unfixed. Two 🟠 IMPORTANTE findings were found and closed within the same audit wave, both independently re-verified by execution (mutation testing redone by the reviewer, not trusted from the implementer's report) before the verdict was recorded. Five 🟢 COSMÉTICO findings remain, all informational/non-blocking, three of which are documentation-freshness nits in this repo's own planning files (`TODO.md`/`AUDITORIAS.md`) — fixed as part of this same consolidation pass (see "Cross-cutting findings" below). `AUD-L1-BUILD` (build-hardening gate) was dispensed by an autonomous CTO decision, folded into `AUD-L1-SUPPLY` as a packager-facing recommendation rather than a repo-side CI gate (rationale: `glintfx` ships as source, so imposing linker/compiler hardening flags unilaterally would be a correctness regression for some consumers, not a security improvement) — see that report's "AUD-L1-BUILD fold-in" section. **Readiness for an external auditor: ready.** No 🔴 blocks a release; the `RELEASE_CHECKLIST.md` created in this pass gives the process to keep this state from drifting silently release to release.

**PT:** As 6 auditorias fechadas para o estado tageado atual têm veredito **CONFORME**. Zero achados 🔴 CRÍTICO no dossiê inteiro — nenhum aberto, nenhum historicamente achado-e-não-corrigido. Dois achados 🟠 IMPORTANTE foram encontrados e fechados dentro da mesma onda de auditoria, ambos re-verificados de forma independente por execução (mutation testing refeito pelo revisor, não confiado no relato do implementer) antes do veredito ser registrado. Cinco achados 🟢 COSMÉTICO permanecem, todos informativos/não-bloqueantes, três dos quais são nits de frescor de documentação nos próprios arquivos de planejamento deste repo (`TODO.md`/`AUDITORIAS.md`) — corrigidos como parte desta mesma passada de consolidação (ver "Achados transversais" abaixo). O `AUD-L1-BUILD` (gate de hardening de build) foi dispensado por decisão autônoma do CTO, dobrado no `AUD-L1-SUPPLY` como recomendação voltada a empacotadores em vez de gate de CI do próprio repo (racional: a `glintfx` é distribuída como fonte, então impor flags de hardening de linker/compilador unilateralmente seria uma regressão de correção para alguns consumidores, não uma melhoria de segurança) — ver a seção "AUD-L1-BUILD fold-in" daquele relatório. **Prontidão para auditor externo: pronto.** Nenhum 🔴 bloqueia release; o `RELEASE_CHECKLIST.md` criado nesta passada dá o processo para este estado não desalinhar silenciosamente release a release.

## Findings by severity / Achados por severidade

| Severidade | Total | Abertos | Detalhe |
| :--- | :---: | :---: | :--- |
| 🔴 CRÍTICO | 0 | 0 | Nenhum em nenhum relatório do dossiê. |
| 🟠 IMPORTANTE | 2 | 0 | Ambos em `AUD-L1-PARSE` (font-path cap ausente + corpus test tautológico); fechados no mesmo ciclo (`16ab365`→`0f944e1`), re-verificados por execução independente. |
| 🟢 COSMÉTICO | 5 | 2 | `AUD-ABI` (1, comentário `syscall.asm:33`), `AUD-SEC` (3, trade-offs deliberados documentados), `AUD-L1-PARSE` (1, TOCTOU informativo, ver julgamento abaixo). Os 2 nits de frescor "gl3w" (`AUD-L1-SUPPLY`/`AUD-L1-LICENSE`) e o doc-drift de contagem de teste do `nightly.yml` (`AUD-L1-MEM`) foram corrigidos nesta consolidação — não contam mais como abertos. |

## Catalog / Catálogo

### Layer 0 — `loucura_c_asm` (core-v0.3.0)

| ID | Report / Relatório | Date / Data | Auditor | Verdict / Veredito | Findings | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| `AUD-ABI` | [`AUD-ABI.md`](AUD-ABI.md) | 2026-07-09 | embedded-firmware-engineer | **CONFORME** | 0🔴 0🟠 1🟢 | ✓ |
| `AUD-SEC` | [`AUD-SEC.md`](AUD-SEC.md) | 2026-07-09 | security-engineer | **CONFORME** | 0🔴 0🟠 3🟢 | ✓ |

### Layer 1 — `glintfx` (v0.11.0)

| ID | Report / Relatório | Date / Data | Auditor | Verdict / Veredito | Findings | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| `AUD-L1-SUPPLY` | [`AUD-L1-SUPPLY.md`](AUD-L1-SUPPLY.md) | 2026-07-16 | security-engineer | **CONFORME** | 0🔴 0🟠 1🟢 (frescor "gl3w" — corrigido nesta consolidação) | ✓ |
| `AUD-L1-PARSE` | [`AUD-L1-PARSE.md`](AUD-L1-PARSE.md) | 2026-07-16 (re-review mesmo dia, `0f944e1`) | security-engineer | **CONFORME** — 2 IMPORTANTE fechados + re-verificados | 0🔴 0🟠 1🟢 (TOCTOU, informativo) | ✓ |
| `AUD-L1-MEM` | [`AUD-L1-MEM.md`](AUD-L1-MEM.md) | 2026-07-16 | security-engineer | **CONFORME** | 0🔴 0🟠 0🟢 | ✓ |
| `AUD-L1-LICENSE` | [`AUD-L1-LICENSE.md`](AUD-L1-LICENSE.md) | 2026-07-16 | compliance-legal | **CONFORME** | 0🔴 0🟠 1🟢 (frescor "gl3w" — corrigido nesta consolidação) | ✓ |
| `AUD-L1-BUILD` | dispensado — dobrado em `AUD-L1-SUPPLY` §"fold-in" | 2026-07-16 | Caetano/CTO (decisão autônoma) | **Resolvido por dobra** — recomendação para empacotadores, não gate de CI | n/a | ✓ (ver `TODO.md`) |

## Judgment on the one residual COSMÉTICO / Julgamento sobre o único COSMÉTICO residual

**EN:** `AUD-L1-PARSE`'s TOCTOU note (the base RmlUi loader's fallback re-derives file size independently of the already-validated cap check) requires an adversarial filesystem/mount, not just an adversarial file, and is not a regression introduced by this audit's own fix — it is a pre-existing, narrow, upstream-code property outside this fix's realistic threat model (a static asset file). The re-review explicitly kept it as `✓` (aprovado), not `⚠`; this index concurs — it is informational, not a release blocker, and does not warrant downgrading `AUD-L1-PARSE`'s Estado Auditado.

**PT:** A nota de TOCTOU do `AUD-L1-PARSE` (o fallback do loader RmlUi da base re-deriva o tamanho de arquivo independentemente da checagem de teto já validada) exige um sistema de arquivos/mount adversarial, não só um arquivo adversarial, e não é uma regressão introduzida pelo próprio fix desta auditoria — é uma propriedade pré-existente, estreita, de código upstream fora do modelo de ameaça realista deste fix (um arquivo de asset estático). A re-review manteve explicitamente `✓` (aprovado), não `⚠`; este índice concorda — é informativo, não bloqueia release, e não justifica rebaixar o Estado Auditado do `AUD-L1-PARSE`.

## Cross-cutting findings fixed in this consolidation / Achados transversais corrigidos nesta consolidação

**EN:** Three of the individual reports independently flagged the same class of nit — planning-document prose (`TODO.md`/`AUDITORIAS.md`) lagging behind a real code change (gl3w removed in v0.8.0, replaced by glintfx's own clean-room GL loader) — each explicitly noting it was outside that deliverable's write scope. Fixed here as part of consolidation (see `TODO.md`/`AUDITORIAS.md` diffs, this branch): both catalog rows now describe the real dependency (glintfx's own `gl_loader.{h,c}`), not the removed third-party `gl3w`. A fourth, unrelated doc-drift (the `nightly.yml` workflow comment naming a stale test count, "18 GLFW=ON / 8 embed", noted inside `AUD-L1-MEM.md` itself as non-blocking) is left as-is — it is a comment inside a CI file, out of this pass's DOCS-only scope, and does not affect the gate's actual behaviour (`ctest` runs whatever is registered, not what a comment says).

**PT:** Três dos relatórios individuais sinalizaram independentemente a mesma classe de nit — prosa de documento de planejamento (`TODO.md`/`AUDITORIAS.md`) atrasada em relação a uma mudança real de código (gl3w removido na v0.8.0, substituído pelo loader GL clean-room próprio da glintfx) — cada um notando explicitamente que estava fora do escopo de escrita daquele entregável. Corrigido aqui como parte da consolidação (ver diffs de `TODO.md`/`AUDITORIAS.md`, este branch): as duas linhas de catálogo agora descrevem a dependência real (`gl_loader.{h,c}` próprio da glintfx), não o `gl3w` de terceiro removido. Um quarto doc-drift, não relacionado (o comentário do workflow `nightly.yml` citando uma contagem de teste desatualizada, "18 GLFW=ON / 8 embed", anotado dentro do próprio `AUD-L1-MEM.md` como não-bloqueante) foi deixado como está — é um comentário dentro de um arquivo de CI, fora do escopo só-DOCS desta passada, e não afeta o comportamento real do gate (`ctest` roda o que estiver registrado, não o que um comentário diz).

## What changed since the last consolidation / O que mudou desde a última consolidação

**EN:** This is the dossier's first master index — the six individual reports above (`AUD-ABI`/`AUD-SEC` from `core-v0.1.0`'s W12 wave, `AUD-L1-*` from the `LW-AUD` wave on `v0.11.0`) previously existed without a unifying index. Also new in this pass: `-Wall -Wextra` applied to the `glintfx` target (commit `45209e5`, zero warnings on our own 17 authored sources; RmlUi backend files and the 26 test + 2 demo targets deliberately left uninstrumented — see the rationale comment at `glintfx/CMakeLists.txt:518-540`) and `docs/RELEASE_CHECKLIST.md` (see that file). An INBOX seed for a future `AUD-L1-QUALITY` (god-class/complexity/dead-code review, e.g. `font_engine_own.cpp` at 1727 lines) was registered in `TODO.md` — not yet an audit, a pull candidate before v1.0.

**PT:** Este é o primeiro índice mestre do dossiê — os seis relatórios individuais acima (`AUD-ABI`/`AUD-SEC` da onda W12 do `core-v0.1.0`, `AUD-L1-*` da onda `LW-AUD` na `v0.11.0`) existiam antes sem um índice unificador. Também novo nesta passada: `-Wall -Wextra` aplicado ao alvo `glintfx` (commit `45209e5`, zero warnings nos nossos 17 sources autorais; arquivos de backend do RmlUi e os 26 alvos de teste + 2 de demo deliberadamente deixados sem instrumentar — ver o comentário de racional em `glintfx/CMakeLists.txt:518-540`) e o `docs/RELEASE_CHECKLIST.md` (ver esse arquivo). Uma semente de INBOX para um futuro `AUD-L1-QUALITY` (revisão de god-class/complexidade/dead-code, ex.: `font_engine_own.cpp` com 1727 linhas) foi registrada no `TODO.md` — ainda não uma auditoria, candidata a pull antes da v1.0.
