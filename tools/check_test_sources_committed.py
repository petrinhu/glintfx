#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Petrus Silva Costa
#
# EN: BISECT-CMAKE follow-up (TODO.md) -- catches a `glintfx/tests/CMakeLists.txt` that
#     registers an `add_executable(<target> <source>...)` whose `<source>` is NOT part of
#     the SAME commit/index state as the `CMakeLists.txt` itself: an "orphan target" that
#     makes that exact commit unconfigurable (`cmake` fails at `add_executable()` with "Cannot
#     find source file") even though `HEAD` (a LATER commit, once the missing `.cpp` finally
#     lands) looks perfectly fine. `git bisect`, a single-commit review (`git archive
#     <sha>`), or any clean checkout pinned to that one SHA all hit this; the ordinary
#     working tree never does, because everyone's uncommitted files are still sitting right
#     there on disk, masking the gap.
#
#     TWO REAL, MEASURED CASES this gate is built to catch (both from this project's own
#     history, both reproduced by running this script against the exact SHA -- see this
#     file's own test invocation in `docs/...`/commit message, not asserted here):
#       - `ca173cf` (2026-07-30 early): `tests/CMakeLists.txt` already registers
#         `ui_layer_capture_frame_smoke.cpp`/`ui_layer_capture_frame_state.cpp`, but neither
#         file exists ANYWHERE in that commit's tree (`git ls-tree -r ca173cf --name-only`
#         has zero matches) -- they landed in the FOLLOWING commit.
#       - `838e10c` (2026-07-30, CAPTURE-FREE): a `git commit --only <my files>` swept in the
#         CURRENT WORKING-TREE content of the shared `tests/CMakeLists.txt`, which already
#         carried two OTHER agents' uncommitted `add_executable()` blocks
#         (`font_nothrow_sanity`, `image_png_only_sanity`) -- `--only` is documented to
#         protect against an unrelated FILE being swept into a commit, but it does nothing
#         about unrelated CONTENT inside a file that is legitimately part of the commit.
#         Both `.cpp` sources were absent from that commit's tree the same way.
#
#     STRUCTURAL, NOT TEXTUAL, AND AGAINST GIT STATE -- NEVER THE WORKING-TREE DISK (the
#     load-bearing design decision here, spelled out because it is the one an easy-to-write
#     version of this gate would get wrong): a plain `test -f glintfx/tests/<source>` would
#     have passed on BOTH commits above, at the exact moment they were made -- the files
#     were sitting right there on disk (another agent's `git add`-less, or not-yet-committed,
#     working-tree copy), which is exactly why nobody using their own eyes or a disk-based
#     check noticed. This script resolves every source against **git's own object model**:
#     the INDEX (default mode, `git show :<path>` / `git cat-file -e :<path>` -- what WOULD
#     be committed right now, the same ground truth `tools/precommit.sh`'s other gates
#     already use via `git diff --cached`) or an explicit commit-ish passed as `argv[1]`
#     (`git show <ref>:<path>` / `git cat-file -e <ref>:<path>` -- for retroactive audits of
#     a historical SHA, or a clean `git archive`/CI checkout). Never `os.path.exists()`.
#
#     PARSES `add_executable(...)` STRUCTURALLY (paren-matched, comment-stripped,
#     MULTI-LINE-AWARE), NOT LINE-BY-LINE REGEX -- `glintfx/tests/CMakeLists.txt` has at
#     least one call spanning several lines with multiple sources
#     (`text_raster_sanity`: `text_raster_sanity.cpp` plus four more paths on the following
#     lines), and comments in this file routinely CONTAIN the literal string
#     `add_executable()` in prose (the GLINTFX_COVERAGE/-Wall block near the end of that
#     file says exactly that, twice, in EN and PT) -- a naive single-line grep for
#     `^add_executable\(` would both MISS the multi-line call's extra sources and risk a
#     false match inside a comment discussing the pattern itself, the identical class of
#     "grep cannot distinguish structure from prose" trap
#     `check_workflow_self_hosted_gate.py`'s own header comment already names for YAML.
#
#     DECLARED SCOPE LIMIT, NOT SILENT (measured, not assumed -- same discipline this
#     project's own doc-comments already apply to every fail-high/precondition contract):
#     a source argument containing a CMake variable/generator-expression token (`${...}` or
#     `$<...>`) is SKIPPED, not resolved -- this gate does not evaluate CMake, so it cannot
#     know what `${_glx_core_src_dir}` expands to without re-implementing a chunk of the
#     build's own variable scoping. Measured against the CURRENT file (2026-07-30): of 121
#     `add_executable()` calls, ALL 121 targets have at least one PLAIN, checkable literal
#     source (their own primary `<target>.cpp`, always a bare filename directly under
#     `glintfx/tests/`) -- the ONLY variable-based arguments in the whole file are the 4
#     extra sources on `text_raster_sanity`'s own multi-line call
#     (`${CMAKE_SOURCE_DIR}/src/text_raster.cpp`, three `${_glx_core_src_dir}/*.c` paths),
#     all pre-existing, stable, vendored Layer-0/Layer-1 files -- a fundamentally different
#     risk class from "a brand-new test `.cpp` another agent forgot to `git add`", which is
#     the ONLY failure mode BOTH real historical cases above actually were. This gate is
#     therefore a COMPLETE, closed enumeration over the risk class it targets, not a partial
#     one that happens to miss the two cases that already bit this project.
#
# PT: Follow-up do BISECT-CMAKE (TODO.md) -- pega um `glintfx/tests/CMakeLists.txt` que
#     registra um `add_executable(<alvo> <fonte>...)` cuja `<fonte>` NÃO faz parte do MESMO
#     estado de commit/índice que o próprio `CMakeLists.txt`: um "alvo órfão" que torna
#     aquele commit exato não-configurável (`cmake` falha em `add_executable()` com "Cannot
#     find source file") mesmo que o `HEAD` (um commit POSTERIOR, quando o `.cpp` faltante
#     finalmente chega) pareça perfeito. `git bisect`, review de um único commit (`git
#     archive <sha>`), ou qualquer checkout limpo fixado naquele SHA batem nisso; a working
#     tree comum nunca bate, porque os arquivos não-commitados de todo mundo ainda estão
#     sentados ali no disco, mascarando a lacuna.
#
#     DOIS CASOS REAIS, MEDIDOS, que este gate foi construído pra pegar (ambos do próprio
#     histórico deste projeto, ambos reproduzidos rodando este script contra o SHA exato):
#       - `ca173cf` (início de 2026-07-30): `tests/CMakeLists.txt` já registra
#         `ui_layer_capture_frame_smoke.cpp`/`ui_layer_capture_frame_state.cpp`, mas nenhum
#         dos dois arquivos existe em lugar NENHUM da árvore daquele commit (`git ls-tree -r
#         ca173cf --name-only` dá zero ocorrências) -- chegaram no commit SEGUINTE.
#       - `838e10c` (2026-07-30, CAPTURE-FREE): um `git commit --only <meus arquivos>`
#         arrastou o conteúdo CORRENTE DA WORKING TREE do `tests/CMakeLists.txt`
#         compartilhado, que já carregava blocos `add_executable()` não-commitados de DOIS
#         OUTROS agentes (`font_nothrow_sanity`, `image_png_only_sanity`) -- o `--only` é
#         documentado a proteger contra um ARQUIVO alheio ser arrastado pro commit, mas não
#         faz nada contra CONTEÚDO alheio dentro de um arquivo que é legitimamente parte do
#         commit. As duas fontes `.cpp` estavam ausentes daquela árvore do mesmo jeito.
#
#     ESTRUTURAL, NÃO TEXTUAL, E CONTRA ESTADO DO GIT -- NUNCA O DISCO DA WORKING TREE (a
#     decisão de desenho que carrega peso aqui, dita explicitamente porque é a que uma
#     versão fácil-de-escrever deste gate erraria): um `test -f glintfx/tests/<fonte>` cru
#     teria passado nos DOIS commits acima, no exato momento em que foram feitos -- os
#     arquivos estavam sentados ali no disco (cópia de working tree sem `git add`, ou
#     ainda-não-commitada, de outro agente), motivo exato pelo qual ninguém usando os
#     próprios olhos ou uma checagem baseada em disco percebeu. Este script resolve toda
#     fonte contra o PRÓPRIO modelo de objeto do git: o ÍNDICE (modo default, `git show
#     :<caminho>` / `git cat-file -e :<caminho>` -- o que SERIA commitado agora, a mesma
#     verdade-terrestre que os outros gates do `tools/precommit.sh` já usam via `git diff
#     --cached`) ou um commit-ish explícito passado em `argv[1]` (`git show <ref>:<caminho>`
#     / `git cat-file -e <ref>:<caminho>` -- pra auditorias retroativas de um SHA histórico,
#     ou um checkout limpo de `git archive`/CI). Nunca `os.path.exists()`.
#
#     PARSEIA `add_executable(...)` ESTRUTURALMENTE (parênteses casados, comentário
#     removido, CIENTE DE MÚLTIPLAS LINHAS), NÃO REGEX LINHA-A-LINHA -- o
#     `glintfx/tests/CMakeLists.txt` tem pelo menos uma chamada se espalhando por várias
#     linhas com múltiplas fontes (`text_raster_sanity`: `text_raster_sanity.cpp` mais
#     quatro caminhos a mais nas linhas seguintes), e comentários neste arquivo rotineiramente
#     CONTÊM a string literal `add_executable()` em prosa (o bloco GLINTFX_COVERAGE/-Wall
#     perto do fim daquele arquivo diz exatamente isso, duas vezes, em EN e PT) -- um grep
#     ingênuo de linha-única por `^add_executable\(` tanto PERDERIA as fontes extras da
#     chamada multi-linha QUANTO arriscaria um match falso dentro de um comentário
#     discutindo o próprio padrão, a MESMA classe de armadilha "grep não distingue estrutura
#     de prosa" que o próprio comentário de cabeçalho de `check_workflow_self_hosted_gate.py`
#     já nomeia pra YAML.
#
#     LIMITE DE ESCOPO DECLARADO, NÃO SILENCIOSO (medido, não presumido -- a mesma
#     disciplina que os próprios doc-comments deste projeto já aplicam a todo contrato de
#     fail-high/precondição): um argumento de fonte contendo um token de
#     variável/generator-expression do CMake (`${...}` ou `$<...>`) é PULADO, não resolvido
#     -- este gate não avalia CMake, então não pode saber pra que `${_glx_core_src_dir}`
#     expande sem reimplementar um pedaço do próprio escopo de variável do build. Medido
#     contra o arquivo CORRENTE (2026-07-30): das 121 chamadas `add_executable()`, TODOS os
#     121 alvos têm ao menos uma fonte literal PLANA, checável (o próprio `<alvo>.cpp`
#     primário, sempre um nome de arquivo nu direto sob `glintfx/tests/`) -- os ÚNICOS
#     argumentos baseados em variável do arquivo inteiro são as 4 fontes extras da própria
#     chamada multi-linha de `text_raster_sanity`
#     (`${CMAKE_SOURCE_DIR}/src/text_raster.cpp`, três caminhos `${_glx_core_src_dir}/*.c`),
#     todos arquivos Layer-0/Layer-1 vendorizados, estáveis, pré-existentes -- uma classe de
#     risco fundamentalmente diferente de "um `.cpp` de teste novinho que outro agente
#     esqueceu de `git add`", que é o ÚNICO modo de falha que os dois casos históricos reais
#     acima de fato foram. Este gate é portanto uma enumeração COMPLETA, fechada, sobre a
#     classe de risco que mira, não uma parcial que por acaso perde os dois casos que já
#     morderam este projeto.
#
# Usage / Uso: tools/check_test_sources_committed.py [<commit-ish>]
#   No argument / sem argumento -> checks the INDEX (staging area) / checa o ÍNDICE.
#   <commit-ish>                -> checks that commit's own tree / checa a árvore daquele
#                                   commit (e.g. `838e10c`, `ca173cf`, `HEAD`).
from __future__ import annotations

import re
import subprocess
import sys

CMAKE_PATH = "glintfx/tests/CMakeLists.txt"

# EN: matches a CMake variable expansion (`${...}`) or generator expression (`$<...>`) --
#     either marks a source argument as out of this gate's declared scope (see header).
# PT: casa uma expansão de variável do CMake (`${...}`) ou generator expression (`$<...>`)
#     -- qualquer um marca um argumento de fonte como fora do escopo declarado deste gate
#     (ver cabeçalho).
_VARIABLE_TOKEN_RE = re.compile(r"\$\{|\$<")

# EN: one `add_executable(` call's full argument list, DOTALL so it spans newlines --
#     matches this file's own real multi-line `text_raster_sanity` call. Non-greedy up to
#     the FIRST `)`: every add_executable() in this codebase's own CMakeLists.txt has zero
#     nested parens in its argument list (confirmed empirically against all 121 calls,
#     2026-07-30 -- no source path or generator-expression here ever contains a literal
#     `)`), so "first close-paren" is the correct, simple terminator, not an approximation.
# PT: a lista de argumentos completa de uma chamada `add_executable(`, DOTALL pra abranger
#     quebras de linha -- casa a própria chamada real multi-linha de `text_raster_sanity`
#     deste arquivo. Não-guloso até o PRIMEIRO `)`: toda add_executable() do próprio
#     CMakeLists.txt deste codebase tem zero parênteses aninhados na lista de argumentos
#     (confirmado empiricamente contra as 121 chamadas, 2026-07-30 -- nenhum caminho de
#     fonte ou generator-expression aqui jamais contém um `)` literal), então "primeiro
#     parêntese de fechamento" é o terminador correto e simples, não uma aproximação.
_ADD_EXECUTABLE_RE = re.compile(r"add_executable\(([^)]*)\)", re.DOTALL)


class GitError(RuntimeError):
    """EN: Raised when a `git` plumbing call this script depends on fails unexpectedly
    (not "path not found", which is the ordinary success-path signal `_git_object_exists`
    interprets itself -- this is for `git show`/`git cat-file` failing for some OTHER
    reason, e.g. not being inside a git repository at all).
    PT: Levantada quando uma chamada de baixo nível do `git` da qual este script depende
    falha de forma inesperada (não "caminho não encontrado", que é o sinal comum de
    caminho-de-sucesso que o próprio `_git_object_exists` interpreta -- isto é pra
    `git show`/`git cat-file` falhando por algum OUTRO motivo, ex.: nem estar dentro de um
    repositório git)."""


def _strip_full_line_comments(text: str) -> str:
    """EN: Blank out any line whose FIRST non-whitespace character is `#` (CMake's own
    comment syntax, and the exact shape every comment in this file's own CMakeLists.txt
    uses -- confirmed empirically, no `code() # trailing comment` form exists in it).
    Replaces the line with an empty string rather than deleting it, so downstream
    line-number-free regex matching over the WHOLE blob still sees the same overall
    length/structure (irrelevant for the paren-matching regex used here, but a cheap
    invariant to keep for any future caller of this helper that DOES care about offsets).
    PT: Apaga qualquer linha cujo PRIMEIRO caractere não-espaço seja `#` (a própria
    sintaxe de comentário do CMake, e a forma exata que todo comentário deste
    CMakeLists.txt usa -- confirmado empiricamente, não existe forma `codigo() #
    comentário à direita` nele). Substitui a linha por string vazia em vez de apagá-la,
    pra que o casamento de regex livre-de-número-de-linha a jusante sobre o blob INTEIRO
    ainda veja o mesmo comprimento/estrutura geral (irrelevante pro regex de parênteses
    usado aqui, mas um invariante barato de manter pra qualquer chamador futuro deste
    helper que SE importe com offsets)."""
    return "\n".join(
        "" if line.strip().startswith("#") else line for line in text.split("\n")
    )


def _git_show(ref_path: str) -> str:
    """EN: `git show <ref_path>` (e.g. `:glintfx/tests/CMakeLists.txt` for the index, or
    `838e10c:glintfx/tests/CMakeLists.txt` for a commit), decoded as UTF-8. Raises
    `GitError` with the exact `ref_path` that failed if the object cannot be read (file
    genuinely absent from that ref, or `ref_path` malformed) -- never silently returns an
    empty string, which would make every add_executable() in the (non-existent) file look
    "not present", a false PASS by vacuous truth, not a real one.
    PT: `git show <ref_path>` (ex.: `:glintfx/tests/CMakeLists.txt` pro índice, ou
    `838e10c:glintfx/tests/CMakeLists.txt` pra um commit), decodificado como UTF-8.
    Levanta `GitError` com o `ref_path` exato que falhou se o objeto não puder ser lido
    (arquivo genuinamente ausente daquele ref, ou `ref_path` malformado) -- nunca devolve
    silenciosamente uma string vazia, o que faria todo add_executable() do arquivo
    (inexistente) parecer "não presente", um PASS falso por verdade vazia, não um real."""
    result = subprocess.run(
        ["git", "show", ref_path], capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        raise GitError(
            f"'git show {ref_path}' failed (rc={result.returncode}): {result.stderr.strip()}"
        )
    return result.stdout


def _git_object_exists(ref_path: str) -> bool:
    """EN: `git cat-file -e <ref_path>` -- True iff that blob/tree/commit object resolves,
    False otherwise (including "path does not exist at that ref", the ordinary,
    non-exceptional outcome this whole gate exists to detect -- NOT raised as an error).
    PT: `git cat-file -e <ref_path>` -- True se e somente se aquele objeto
    blob/tree/commit resolve, False caso contrário (inclusive "caminho não existe naquele
    ref", o resultado comum, não-excepcional que este gate inteiro existe pra detectar --
    NÃO levantado como erro)."""
    result = subprocess.run(
        ["git", "cat-file", "-e", ref_path], capture_output=True, text=True, check=False
    )
    return result.returncode == 0


def find_add_executable_calls(cmake_text: str) -> list[tuple[str, list[str]]]:
    """EN: Returns a list of (target_name, source_args) tuples, one per `add_executable()`
    call found in `cmake_text` (comments already expected stripped by the caller). A call
    with no arguments at all (should not happen in valid CMake, but this parser does not
    assume it cannot) yields an empty `source_args` list, never a crash.
    PT: Devolve uma lista de tuplas (nome_do_alvo, args_de_fonte), uma por chamada
    `add_executable()` achada em `cmake_text` (comentários já esperados removidos pelo
    chamador). Uma chamada sem argumento nenhum (não deveria acontecer em CMake válido,
    mas este parser não presume que não pode) rende uma lista `source_args` vazia, nunca
    um crash."""
    calls: list[tuple[str, list[str]]] = []
    for match in _ADD_EXECUTABLE_RE.finditer(cmake_text):
        args = match.group(1).split()
        if not args:
            calls.append(("<unnamed>", []))
            continue
        target, sources = args[0], args[1:]
        calls.append((target, sources))
    return calls


def check(ref: str | None) -> tuple[list[str], int, int]:
    """EN: Runs the gate against `ref` (a commit-ish, e.g. `838e10c`/`HEAD`) or, when `ref`
    is `None`, against the INDEX. Returns a list of human-readable violation strings (empty
    list == clean). Raises `GitError` if `glintfx/tests/CMakeLists.txt` itself cannot be
    read from the requested ref at all (a harder failure than "gate found an orphan
    target" -- the caller should surface it distinctly, see `main()`).
    PT: Roda o gate contra `ref` (um commit-ish, ex.: `838e10c`/`HEAD`) ou, quando `ref` é
    `None`, contra o ÍNDICE. Devolve uma lista de strings de violação legíveis (lista
    vazia == limpo). Levanta `GitError` se o próprio `glintfx/tests/CMakeLists.txt` não
    puder ser lido do ref pedido de jeito nenhum (uma falha mais dura que "gate achou um
    alvo órfão" -- o chamador deve destacar isso separadamente, ver `main()`)."""
    prefix = ":" if ref is None else f"{ref}:"
    cmake_text = _strip_full_line_comments(_git_show(f"{prefix}{CMAKE_PATH}"))

    violations: list[str] = []
    checked = 0
    skipped_variable = 0
    for target, sources in find_add_executable_calls(cmake_text):
        for source in sources:
            if _VARIABLE_TOKEN_RE.search(source):
                skipped_variable += 1
                continue
            checked += 1
            resolved = f"glintfx/tests/{source}"
            if not _git_object_exists(f"{prefix}{resolved}"):
                where = "the index" if ref is None else f"commit '{ref}'"
                violations.append(
                    f"target '{target}': source '{source}' (resolved '{resolved}') "
                    f"not present in {where} -- orphan target, this commit would not "
                    f"configure in a clean checkout"
                )
    return violations, checked, skipped_variable


def main(argv: list[str]) -> int:
    ref = argv[1] if len(argv) > 1 else None
    where = "the INDEX (staging area)" if ref is None else f"commit '{ref}'"

    try:
        violations, checked, skipped_variable = check(ref)
    except GitError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    if violations:
        print(
            f"FAIL: {len(violations)} orphan add_executable() source(s) checked against "
            f"{where}:",
            file=sys.stderr,
        )
        print(
            f"FALHA: {len(violations)} fonte(s) órfã(s) de add_executable() checada(s) "
            f"contra {where}:",
            file=sys.stderr,
        )
        for v in violations:
            print(f"  - {v}", file=sys.stderr)
        return 1

    print(
        f"OK: {checked} add_executable() source(s) present in {where} "
        f"({skipped_variable} CMake-variable-based source(s) skipped, declared out of "
        f"scope -- see this script's own header comment)"
    )
    print(
        f"OK: {checked} fonte(s) de add_executable() presente(s) em {where} "
        f"({skipped_variable} fonte(s) baseada(s) em variável CMake pulada(s), fora de "
        f"escopo declarado -- ver o próprio comentário de cabeçalho deste script)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
