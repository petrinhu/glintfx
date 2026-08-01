#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Petrus Silva Costa
#
# EN: SEC-CI-HARDEN (IMP-2) -- enforced anti-fork gate for self-hosted CI. AUD-CI-RUNNER
#     found that the single most important control on the self-hosted runner's whole
#     surface -- "no workflow with a `self-hosted` job may also trigger on
#     `pull_request`/`pull_request_target`/`issue_comment`" -- existed only as a header
#     comment in .github/workflows/heavy.yml, not as anything CI itself checks. A future
#     edit (an honest mistake, a bad merge, or a malicious PR a reviewer approves without
#     noticing a trigger change buried in a large diff) could silently reopen the exact
#     attack GitHub warns about for self-hosted runners on public repositories, and
#     nothing would fail red to catch it before merge. This script turns that promise
#     into a mechanically-checked invariant.
#
#     Runs on a GitHub-hosted runner (see the "TST-CI-SELFHOSTED-GATE" step in
#     .github/workflows/ci.yml's lint-and-scan job) -- never on the self-hosted label
#     itself, and on every PR (including fork PRs), precisely because it needs no
#     runner privilege and touches no secret to do its job: it only reads the workflow
#     YAML files already checked out.
#
#     STRUCTURAL, NOT TEXTUAL. The audit's own gotcha: a raw `grep pull_request
#     heavy.yml` matches the FOUR prose comment lines in that file that explain why
#     pull_request must never be added, giving a false sense that "grep would have
#     caught it" -- it would not, because grep cannot distinguish a YAML key from a
#     word inside a comment. This script parses each workflow with PyYAML and inspects
#     the actual document structure: the top-level `on:` trigger block and each job's
#     `runs-on:` field. It also knows the classic PyYAML/YAML-1.1 trap that a bare,
#     unquoted `on:` key is resolved to the Python boolean True (not the string "on")
#     by the default resolver -- every one of this repo's five workflow files hits
#     this, so a gate that only checked `doc.get("on")` would silently see nothing and
#     always pass. See the two-direction proof this file's own header explains is
#     required (a fixture workflow that trips the gate, and a clean run against the
#     real repo) in the SEC-CI-HARDEN work item / TODO.md and the docstring below.
#
# PT: SEC-CI-HARDEN (IMP-2) -- gate anti-fork imposto para o CI self-hosted. A
#     AUD-CI-RUNNER achou que o controle mais importante de toda a superfície do
#     runner self-hosted -- "nenhum workflow com job `self-hosted` pode também
#     disparar em `pull_request`/`pull_request_target`/`issue_comment`" -- existia só
#     como comentário de cabeçalho em .github/workflows/heavy.yml, não como algo que o
#     próprio CI checa. Uma edição futura (um engano honesto, um merge ruim, ou um PR
#     malicioso que um revisor aprova sem notar uma mudança de gatilho enterrada num
#     diff grande) poderia reabrir em silêncio exatamente o ataque que o GitHub avisa
#     pra runner self-hosted em repositório público, e nada falharia em vermelho pra
#     pegar isso antes do merge. Este script transforma essa promessa num invariante
#     checado mecanicamente.
#
#     Roda num runner GitHub-hosted (ver o passo "TST-CI-SELFHOSTED-GATE" no job
#     lint-and-scan de .github/workflows/ci.yml) -- nunca no label self-hosted em si,
#     e em todo PR (inclusive PR de fork), exatamente porque não precisa de
#     privilégio de runner nem toca segredo nenhum pra fazer o trabalho: só lê os
#     arquivos YAML de workflow já presentes no checkout.
#
#     ESTRUTURAL, NÃO TEXTUAL. A própria armadilha que a auditoria apontou: um `grep
#     pull_request heavy.yml` cru casa as QUATRO linhas de comentário em prosa daquele
#     arquivo que explicam por que pull_request nunca deve ser adicionado, dando a
#     falsa sensação de que "grep teria pego" -- não teria, porque grep não distingue
#     uma chave YAML de uma palavra dentro de um comentário. Este script parseia cada
#     workflow com PyYAML e inspeciona a estrutura real do documento: o bloco de
#     gatilho `on:` de topo e o campo `runs-on:` de cada job. Também conhece a
#     armadilha clássica do PyYAML/YAML-1.1 de que uma chave `on:` nua, sem aspas, é
#     resolvida pro booleano Python True (não a string "on") pelo resolver default --
#     todos os cinco arquivos de workflow deste repo caem nisso, então um gate que só
#     checasse `doc.get("on")` não veria nada em silêncio e sempre passaria. Ver a
#     prova nos dois sentidos que o cabeçalho deste arquivo explica ser exigida (um
#     workflow-fixture que dispara o gate, e um run limpo contra o repo real) no item
#     de trabalho SEC-CI-HARDEN / TODO.md e no docstring abaixo.

from __future__ import annotations

import glob
import sys
from pathlib import Path
from typing import Any

import yaml

# EN: The three GitHub Actions events that hand control of the workflow run to
#     whoever opened the PR / issue comment -- the exact class of trigger the
#     self-hosted-on-public-repo warning is about. `pull_request_target` and
#     `issue_comment` are included even though this repo does not use them today:
#     they are the other two well-known variants of the same attack shape (a
#     pull_request_target checks out the BASE ref but still runs with the target
#     repo's token and secrets against attacker-controlled workflow content in some
#     configurations; issue_comment lets any commenter re-trigger a run). The gate
#     should fail on all three, not just the one currently absent.
# PT: Os três eventos do GitHub Actions que entregam o controle do run do workflow
#     pra quem abriu o PR / comentário de issue -- exatamente a classe de gatilho de
#     que o aviso self-hosted-em-repo-público trata. `pull_request_target` e
#     `issue_comment` entram mesmo este repo não os usando hoje: são as outras duas
#     variantes bem conhecidas do mesmo formato de ataque (pull_request_target faz
#     checkout do ref BASE mas ainda roda com o token/segredos do repo alvo contra
#     conteúdo de workflow controlado pelo atacante em algumas configurações;
#     issue_comment deixa qualquer comentarista re-disparar um run). O gate deve
#     falhar nos três, não só no que está ausente hoje.
DENYLISTED_TRIGGERS = frozenset({"pull_request", "pull_request_target", "issue_comment"})

SELF_HOSTED_LABEL = "self-hosted"


class GateViolation(Exception):
    """EN: Raised with a human-readable reason when a workflow trips the gate.
    PT: Levantada com um motivo legível quando um workflow dispara o gate."""


def _trigger_names(doc: dict[str, Any]) -> set[str]:
    """EN: Return the set of top-level trigger event names from a parsed workflow
    document, handling the PyYAML "bare `on:` becomes the boolean True key" trap
    (YAML 1.1's implicit boolean resolver, still PyYAML's default with
    yaml.safe_load) as well as every shape GitHub Actions accepts for the `on:`
    value: a bare string (`on: push`), a list (`on: [push, pull_request]`), or a
    mapping (`on: {push: {...}, pull_request: {...}}`).
    PT: Devolve o conjunto de nomes de evento de gatilho de topo de um documento de
    workflow parseado, lidando com a armadilha do PyYAML de "`on:` nua vira a chave
    booleana True" (o resolver implícito de booleano do YAML 1.1, ainda default do
    PyYAML com yaml.safe_load), além de todo formato que o GitHub Actions aceita
    pro valor de `on:`: string nua (`on: push`), lista (`on: [push, pull_request]`),
    ou mapa (`on: {push: {...}, pull_request: {...}}`)."""
    # EN: Prefer the literal string key "on" if present (some YAML front-matter
    #     tools or a future PyYAML version might not trip the boolean trap), but
    #     fall back to the boolean True key, which is what every workflow in THIS
    #     repo actually parses to today -- verified empirically, not assumed.
    # PT: Prefere a chave string literal "on" se presente (alguma ferramenta de
    #     front-matter YAML ou uma versão futura do PyYAML pode não cair na
    #     armadilha do booleano), mas cai pra chave booleana True, que é pro que
    #     todo workflow DESTE repo de fato parseia hoje -- verificado empiricamente,
    #     não assumido.
    triggers = doc.get("on", doc.get(True))
    if triggers is None:
        return set()
    if isinstance(triggers, str):
        return {triggers}
    if isinstance(triggers, list):
        return {str(item) for item in triggers}
    if isinstance(triggers, dict):
        return {str(key) for key in triggers}
    return set()


def _runs_on_labels(runs_on: Any) -> list[str]:
    """EN: Normalize a job's `runs-on:` value (string or list -- GitHub Actions
    accepts both) into a flat list of label strings.
    PT: Normaliza o valor `runs-on:` de um job (string ou lista -- o GitHub Actions
    aceita as duas) numa lista plana de strings de label."""
    if isinstance(runs_on, str):
        return [runs_on]
    if isinstance(runs_on, list):
        return [str(item) for item in runs_on]
    return []


def check_workflow(path: Path) -> list[str]:
    """EN: Parse one workflow file and return a list of violation strings (empty if
    clean). Never raises on a well-formed GitHub Actions workflow -- a YAML parse
    error is a real problem GitHub Actions itself would also refuse to run, so it
    propagates rather than being swallowed as "no violation found".
    PT: Parseia um arquivo de workflow e devolve uma lista de strings de violação
    (vazia se limpo). Nunca engole um erro de parse YAML -- um YAML malformado é um
    problema real que o próprio GitHub Actions também recusaria rodar, então
    propaga em vez de ser tratado como "nenhuma violação achada"."""
    with path.open("r", encoding="utf-8") as handle:
        doc = yaml.safe_load(handle)

    if not isinstance(doc, dict):
        # EN: An empty or non-mapping file is not a valid GitHub Actions workflow;
        #     nothing to check, but also nothing this gate should crash on.
        # PT: Um arquivo vazio ou que não é um mapa não é um workflow válido do
        #     GitHub Actions; nada a checar, mas também nada que este gate deva
        #     quebrar em cima.
        return []

    triggers = _trigger_names(doc)
    denylisted_hit = triggers & DENYLISTED_TRIGGERS
    if not denylisted_hit:
        return []

    jobs = doc.get("jobs", {})
    if not isinstance(jobs, dict):
        return []

    violations: list[str] = []
    for job_name, job in jobs.items():
        if not isinstance(job, dict):
            continue
        labels = _runs_on_labels(job.get("runs-on"))
        if any(label == SELF_HOSTED_LABEL for label in labels):
            violations.append(
                f"{path}: job '{job_name}' has runs-on={labels!r} (self-hosted) "
                f"AND the workflow triggers on {sorted(denylisted_hit)!r} -- "
                f"a fork PR / issue comment could execute code on the self-hosted "
                f"runner. Remove the denylisted trigger, or move this job off the "
                f"self-hosted label."
            )
    return violations


def main(argv: list[str]) -> int:
    workflow_dir = argv[1] if len(argv) > 1 else ".github/workflows"
    pattern_yml = str(Path(workflow_dir) / "*.yml")
    pattern_yaml = str(Path(workflow_dir) / "*.yaml")
    paths = sorted(Path(p) for p in (*glob.glob(pattern_yml), *glob.glob(pattern_yaml)))

    if not paths:
        print(f"error: no workflow files found under '{workflow_dir}'", file=sys.stderr)
        return 2

    all_violations: list[str] = []
    for path in paths:
        all_violations.extend(check_workflow(path))

    if all_violations:
        print("FAIL: self-hosted job(s) reachable by a fork-controlled trigger:", file=sys.stderr)
        print("FALHA: job(s) self-hosted alcançável(is) por gatilho controlado por fork:", file=sys.stderr)
        for violation in all_violations:
            print(f"  - {violation}", file=sys.stderr)
        return 1

    print(f"OK: no self-hosted job triggers on {sorted(DENYLISTED_TRIGGERS)} in {len(paths)} workflow file(s)")
    print(f"OK: nenhum job self-hosted dispara em {sorted(DENYLISTED_TRIGGERS)} em {len(paths)} arquivo(s) de workflow")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
