#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Contamination-detecting ctest wrapper (BUILDDIR-MUTACAO, W23). Born from a fact
#     measured on 2026-07-30: two agents shared this working tree during W22. One did
#     mutation testing and sabotaged a TRACKED file under glintfx/ (removed a real
#     GL_PACK_ALIGNMENT fix), rebuilding+running it 7 times. A second agent, unaware, ran
#     the full ctest suite in the middle of that window and saw capture_framebuffer_smoke
#     die with `double free or corruption`. Coredumps later proved 21 crashes inside a
#     6-minute window, zero outside it.
#
#     The consequence that almost slipped through: ANY suite run inside a contaminated
#     window is INVALID -- including the tests that passed GREEN. A test that reported
#     PASS may have executed against a binary that was mutated at some other point during
#     the same run; a green result under contamination proves nothing. The natural
#     reflex is to chase only the red test; the correct one is to invalidate the entire
#     run and re-run clean.
#
#     This script makes that detection mechanical instead of relying on agents
#     coordinating with each other (they may not even know another agent is active):
#       1. snapshot the tracked+staged+unstaged state of the paths that feed the build
#          under test (see GUARD_PATHS below) BEFORE anything runs;
#       2. `cmake --build` the given build dir -- this is what makes the binary match
#          the CURRENT source, killing by construction the "source restored but binary
#          stale" case (2026-07-25 lesson: a restored .cpp with a stale .o still fails
#          "for no reason");
#       3. run `ctest` under the SAME Xvfb/XDG isolation recipe tools/preci.sh already
#          uses (QA-XVFBISO -- copied verbatim, not re-derived, per this house's own
#          "copy the whole canonical invocation, then adapt" lesson: a half-copy of it
#          already broke a KWin drill once);
#       4. snapshot again AFTER ctest finishes and compare. Any divergence -- staged,
#          unstaged, OR a diff against HEAD -- means the tracked tree moved during the
#          run, so the whole run (build + test) is declared INVALID regardless of the
#          ctest exit code, per the canonical message below.
#
#     SCOPE OF THE GUARD (deliberate, do not widen without re-reading this): only
#     glintfx/ EXCLUDING glintfx/build*/ is watched (GUARD_PATHS). Editing docs/,
#     TODO.md, README.md, tools/ scripts unrelated to the build, etc. in a parallel
#     session must NOT trip the guard -- a guard that flags legitimate parallel work
#     that does not affect the binary under test is noise, and a noisy guard gets
#     disabled, which is worse than no guard at all (it gives false confidence of
#     coverage). build*/ dirs are already untracked+gitignored (glintfx/build-*/ in
#     .gitignore) so `git status --porcelain -uno` (untracked files OFF) and `git diff
#     HEAD` (tracked files only) do not see them anyway; the explicit
#     ':(exclude)glintfx/build*' pathspec is defense in depth in case that ever changes.
#
#     WHAT THIS DOES NOT CATCH: mutation of an UNTRACKED file (a brand-new .cpp added
#     but never `git add`ed) is invisible to `git status --porcelain -uno` (which hides
#     ALL untracked files, not just build dirs) -- this is the same trade-off this
#     house's mutation-testing discipline already accepts elsewhere (mutate a committed
#     blob copied outside the tree, never an in-place untracked file). It also does not
#     catch mutation of a file OUTSIDE glintfx/ that the build still somehow depends on
#     (there should be none; glintfx/ is self-contained).
#
#     TRANSIENT-MUTATION ADDENDUM (found by adversarial review, 2026-07-31, MEASURED not
#     deduced -- reproduced against this script's own drill clone: hash before == hash
#     after, byte for byte, across a mutate/rebuild/restore/rebuild cycle applied fully
#     inside the guarded window). The tree snapshot above is a pure function of tree
#     STATE, so a mutation that is applied AND reverted inside the window is invisible to
#     it -- and reverting between cycles is exactly mutation-testing protocol (the W22
#     incident was 7 such cycles). A guarded suite that starts and ends with the source
#     restored sees a constant tree at both snapshot points and declares the run valid,
#     while the shared build dir's binaries were swapped out from under it mid-run -- the
#     guard failing permissive in precisely the scenario that motivated it.
#     Fixed by fingerprinting the ACTUAL TEST EXECUTABLES (not the whole build dir, which
#     would pick up ctest's own Testing/ writes and turn the guard noisy): right after
#     `cmake --build`, enumerate every executable ctest is about to invoke (via `ctest
#     --show-only=json-v1`, unwrapping the `-DEXE=<path>` indirection that
#     glintfx/tests/run_xvfb.cmake's per-test Xvfb wrapper introduces -- most tests run
#     through `cmake -DEXE=... -P run_xvfb.cmake`, not the binary directly, so
#     `command[0]` alone is `/usr/bin/cmake` for those and useless), then hash
#     (path, size, mtime) of each. A concurrent rebuild -- even one that lands
#     byte-identical machine code -- writes a FRESH file, so mtime moves even when
#     content does not: this catches exactly what the tree snapshot cannot.
#
# PT: Wrapper de `ctest` que detecta contaminação (BUILDDIR-MUTACAO, W23). Nasceu de um
#     fato medido em 2026-07-30: dois agentes compartilharam esta working tree durante a
#     W22. Um fazia mutation testing e sabotou um arquivo RASTREADO sob glintfx/ (removeu
#     um fix real de GL_PACK_ALIGNMENT), rebuildando+rodando 7 ciclos. Um segundo agente,
#     sem saber, rodou a suíte completa de ctest no meio dessa janela e viu
#     capture_framebuffer_smoke morrer com `double free or corruption`. Coredumps depois
#     provaram 21 crashes numa janela de 6 minutos, zero fora dela.
#
#     A consequência que quase passou batido: QUALQUER suíte medida numa janela
#     contaminada é INVÁLIDA -- inclusive os testes que passaram VERDES. Um teste que
#     reportou PASS pode ter executado contra um binário mutado em outro ponto da mesma
#     rodada; um resultado verde sob contaminação não prova nada. O reflexo natural é
#     investigar só o teste vermelho; o correto é invalidar a rodada inteira e re-rodar
#     limpa.
#
#     Este script torna essa detecção mecânica em vez de depender de agentes se
#     coordenarem entre si (podem nem saber que outro agente está ativo):
#       1. tira um snapshot do estado rastreado+staged+unstaged dos caminhos que
#          alimentam o build sob teste (ver GUARD_PATHS abaixo) ANTES de qualquer coisa
#          rodar;
#       2. `cmake --build` do diretório de build dado -- é isso que garante que o
#          binário corresponde ao fonte ATUAL, matando por construção o caso "fonte
#          restaurado mas binário stale" (lição de 2026-07-25: um .cpp restaurado com
#          .o stale ainda falha "sem motivo");
#       3. roda `ctest` sob a MESMA receita de isolamento Xvfb/XDG que o tools/preci.sh
#          já usa (QA-XVFBISO -- copiada verbatim, não re-derivada, seguindo a lição
#          desta casa de "copie a invocação canônica inteira, e só então adapte": uma
#          meia-cópia dela já quebrou um drill de KWin uma vez);
#       4. tira snapshot de novo DEPOIS do ctest terminar e compara. Qualquer
#          divergência -- staged, unstaged, OU um diff contra HEAD -- significa que a
#          árvore rastreada mudou durante a rodada, então a rodada inteira (build +
#          teste) é declarada INVÁLIDA independente do código de saída do ctest,
#          conforme a mensagem canônica abaixo.
#
#     ESCOPO DO GUARD (deliberado, não alargar sem reler isto): só glintfx/ EXCLUINDO
#     glintfx/build*/ é vigiado (GUARD_PATHS). Editar docs/, TODO.md, README.md, scripts
#     de tools/ não relacionados ao build, etc. numa sessão paralela NÃO PODE disparar o
#     guard -- um guard que acusa trabalho paralelo legítimo que não afeta o binário sob
#     teste é ruído, e guard ruidoso é desligado, o que é pior que nenhum guard (dá falsa
#     sensação de cobertura). Diretórios build*/ já são untracked+gitignored
#     (glintfx/build-*/ no .gitignore), então `git status --porcelain -uno` (untracked
#     desligado) e `git diff HEAD` (só arquivos rastreados) já não os enxergam; o
#     pathspec explícito ':(exclude)glintfx/build*' é defesa em profundidade caso isso
#     mude algum dia.
#
#     O QUE ISTO NÃO PEGA: mutação de arquivo UNTRACKED (um .cpp novo adicionado mas
#     nunca `git add`ado) é invisível a `git status --porcelain -uno` (que esconde TODO
#     untracked, não só build dirs) -- é a mesma troca que a própria disciplina de
#     mutation testing desta casa já aceita em outro lugar (mutar um blob commitado
#     copiado pra fora da árvore, nunca um untracked in-place). Também não pega mutação
#     de arquivo FORA de glintfx/ do qual o build de alguma forma ainda dependa (não
#     deveria haver nenhum; glintfx/ é autocontido).
#
#     ADENDO MUTAÇÃO TRANSITÓRIA (achado por review adversarial, 2026-07-31, MEDIDO não
#     deduzido -- reproduzido contra o próprio clone de drill deste script: hash antes ==
#     hash depois, byte a byte, num ciclo mutar/rebuildar/restaurar/rebuildar inteiro
#     dentro da janela guardada). O snapshot de árvore acima é função pura do ESTADO da
#     árvore, então uma mutação aplicada E revertida dentro da janela é invisível a ele --
#     e reverter entre ciclos é exatamente o protocolo de mutation testing (o incidente da
#     W22 foram 7 ciclos assim). Uma suíte guardada que começa e termina com o fonte
#     restaurado vê árvore constante nos dois pontos de snapshot e declara a rodada
#     válida, enquanto os binários do build dir compartilhado foram trocados por baixo no
#     meio -- o guard errando pro lado permissivo justamente no cenário que o motivou.
#     Corrigido tirando a impressão digital dos EXECUTÁVEIS DE TESTE de fato (não do
#     build dir inteiro, que pegaria as escritas do próprio ctest em Testing/ e viraria
#     ruído): logo após o `cmake --build`, enumera todo executável que o ctest está pra
#     invocar (via `ctest --show-only=json-v1`, desfazendo a indireção `-DEXE=<caminho>`
#     que o wrapper de Xvfb por-teste do glintfx/tests/run_xvfb.cmake introduz -- a
#     maioria dos testes roda via `cmake -DEXE=... -P run_xvfb.cmake`, não o binário
#     direto, então `command[0]` sozinho é `/usr/bin/cmake` pra esses e não serve), e
#     hasheia (caminho, tamanho, mtime) de cada um. Um rebuild concorrente -- mesmo um que
#     produza código de máquina byte-idêntico -- escreve um arquivo NOVO, então o mtime se
#     move mesmo quando o conteúdo não muda: isso pega exatamente o que o snapshot de
#     árvore não pega.
#
# Usage / Uso:
#   tools/ctest_guarded.sh <build-dir> [ctest-args...]
#   tools/ctest_guarded.sh --help
#
#   Example / Exemplo:
#     tools/ctest_guarded.sh glintfx/build-preci
#     tools/ctest_guarded.sh glintfx/build-preci -R render_sanity
#
# Exit status:
#   0   -- clean run: tracked tree AND test-executable fingerprints unchanged, ctest
#          passed.
#   1   -- CONTAMINATED run (BUILDDIR-MUTACAO): either the tracked tree under glintfx/
#          (minus build*/) moved between the pre-build and post-ctest tree snapshots, OR
#          one of the test executables' (path, size, mtime) changed between the
#          post-build and post-ctest artifact fingerprints (a concurrent rebuild, even a
#          transient one that restores identical source). The matching canonical message
#          is printed verbatim on stderr (both print if both triggered); re-run once the
#          tree/build dir are quiet. This exit code takes priority over ctest's own exit
#          code -- a contaminated run is invalid REGARDLESS of whether ctest itself
#          reported pass or fail.
#   N>0 -- ctest's own exit code, propagated as-is, when NEITHER contamination check
#          fired (a genuine red suite).
#   2   -- usage error (missing/invalid <build-dir>, not a configured CMake build dir, or
#          a required tool -- jq -- is missing).
#
# Requires: git, cmake, ctest, xvfb-run, jq. Assumes <build-dir> was already configured
#           (has a CMakeCache.txt) -- this script builds+tests, it does not configure.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]; then
  grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
  exit 0
fi

BUILD_DIR="$1"
shift
CTEST_ARGS=("$@")

if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  echo "error: '${BUILD_DIR}/CMakeCache.txt' not found -- <build-dir> must already be configured (run cmake -S ... -B '${BUILD_DIR}' first)." >&2
  exit 2
fi

if ! command -v jq >/dev/null 2>&1; then
  echo "error: 'jq' not found in PATH -- required for the test-executable fingerprint (see the TRANSIENT-MUTATION ADDENDUM header comment). This is a house-canonical tool (TOOLING.md); install it rather than skipping the check -- a guard that silently degrades gives false confidence." >&2
  exit 2
fi

section() {
  echo ""
  echo "== ctest_guarded: $1 =="
}

# EN: paths that feed the build under test. See the "SCOPE OF THE GUARD" header comment
#     above before touching this.
# PT: caminhos que alimentam o build sob teste. Ver o comentário de cabeçalho "ESCOPO DO
#     GUARD" acima antes de mexer aqui.
GUARD_PATHS=(glintfx/ ':(exclude)glintfx/build*')

snapshot() {
  {
    git status --porcelain -uno -- "${GUARD_PATHS[@]}"
    git diff HEAD -- "${GUARD_PATHS[@]}"
  } | sha256sum | awk '{print $1}'
}

# EN: list_test_executables -- the exact set of files ctest is about to invoke for
#     "${BUILD_DIR}", one absolute path per line, deduplicated. See the
#     TRANSIENT-MUTATION ADDENDUM header comment for why command[0] alone is not enough
#     (most tests here are wrapped by glintfx/tests/run_xvfb.cmake, so command[0] is
#     `/usr/bin/cmake` and the real binary is the value of a `-DEXE=<path>` argument).
# PT: list_test_executables -- o conjunto exato de arquivos que o ctest está pra invocar
#     em "${BUILD_DIR}", um caminho absoluto por linha, deduplicado. Ver o comentário de
#     cabeçalho ADENDO MUTAÇÃO TRANSITÓRIA pro porquê de command[0] sozinho não bastar
#     (a maioria dos testes aqui é embrulhada por glintfx/tests/run_xvfb.cmake, então
#     command[0] é `/usr/bin/cmake` e o binário real é o valor de um argumento
#     `-DEXE=<caminho>`).
list_test_executables() {
  local build_dir="$1"
  # EN: NB the `(.[0] // "")` (not `.[0] // empty`): on an empty array `.[0]` is `null`,
  #     and `null // empty` makes jq's `as $exe_arg` binding produce ZERO outputs for that
  #     input -- silently DROPPING the whole test entry, not binding an empty string. This
  #     was measured live: it silently dropped every test WITHOUT a `-DEXE=` wrapper (the
  #     common case), including audio_hostile_sanity in the very drill that was supposed
  #     to prove this function works, before the fix below.
  # PT: NB o `(.[0] // "")` (não `.[0] // empty`): num array vazio `.[0]` é `null`, e
  #     `null // empty` faz o binding `as $exe_arg` do jq produzir ZERO saídas pra aquele
  #     input -- DESCARTANDO silenciosamente a entrada de teste inteira, não vinculando
  #     string vazia. Isto foi medido ao vivo: descartava silenciosamente todo teste SEM
  #     wrapper `-DEXE=` (o caso comum), inclusive audio_hostile_sanity no próprio drill
  #     que deveria provar que esta função funciona, antes do conserto abaixo.
  ctest --show-only=json-v1 --test-dir "${build_dir}" | jq -r '
    .tests[] |
    ( .command | map(select(startswith("-DEXE="))) | (.[0] // "") ) as $exe_arg |
    if $exe_arg != "" then
      ($exe_arg | sub("^-DEXE="; ""))
    else
      .command[0]
    end
  ' | sort -u
}

# EN: fingerprint_executables -- reads paths from stdin (one per line, as produced by
#     list_test_executables) and hashes (path, size, mtime) of each. A path that does not
#     exist is fingerprinted as literally "MISSING" instead of being skipped, so a binary
#     that got deleted out from under the run also counts as a mismatch.
# PT: fingerprint_executables -- lê caminhos da stdin (um por linha, como
#     list_test_executables produz) e hasheia (caminho, tamanho, mtime) de cada um. Um
#     caminho que não existe é registrado literalmente como "MISSING" em vez de pulado,
#     então um binário apagado por baixo da rodada também conta como divergência.
fingerprint_executables() {
  local f
  while IFS= read -r f; do
    [[ -z "${f}" ]] && continue
    if [[ -f "${f}" ]]; then
      stat --format '%n %s %Y' -- "${f}"
    else
      printf '%s MISSING\n' "${f}"
    fi
  done | sha256sum | awk '{print $1}'
}

before_hash="$(snapshot)"

section "cmake --build ${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"

section "fingerprint test executables (TRANSIENT-MUTATION ADDENDUM)"
test_exe_list="$(list_test_executables "${BUILD_DIR}")"
artifact_before="$(printf '%s\n' "${test_exe_list}" | fingerprint_executables)"

# EN: QA-XVFBISO isolation, copied VERBATIM from tools/preci.sh's run_layer1_config() --
#     see that function's own header comment for the full rationale (2026-07-23 incident:
#     a bare `xvfb-run -a ctest` without this isolation leaked into the live desktop).
# PT: isolamento QA-XVFBISO, copiado VERBATIM de run_layer1_config() no tools/preci.sh --
#     ver o comentário de cabeçalho daquela função pro racional completo (incidente
#     2026-07-23: um `xvfb-run -a ctest` cru sem este isolamento vazava pra área de
#     trabalho viva).
section "ctest --test-dir ${BUILD_DIR} (Xvfb/XDG isolated)"
fake_xdg_runtime="$(mktemp -d "${TMPDIR:-/tmp}/glintfx-ctest-guarded-xvfb-runtime.XXXXXX")"

ctest_rc=0
env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR="${fake_xdg_runtime}" \
  xvfb-run -a ctest --test-dir "${BUILD_DIR}" --output-on-failure "${CTEST_ARGS[@]}" || ctest_rc=$?
rm -rf "${fake_xdg_runtime}"

artifact_after="$(printf '%s\n' "${test_exe_list}" | fingerprint_executables)"
after_hash="$(snapshot)"

contaminated=0

if [[ "${before_hash}" != "${after_hash}" ]]; then
  echo "" >&2
  echo "RODADA INVALIDA (BUILDDIR-MUTACAO): a arvore rastreada mudou durante a suite -- TODOS os resultados desta rodada estao invalidados, INCLUSIVE OS VERDES. Confira coredumpctl e re-rode." >&2
  contaminated=1
fi

if [[ "${artifact_before}" != "${artifact_after}" ]]; then
  echo "" >&2
  echo "RODADA INVALIDA (BUILDDIR-MUTACAO): os binarios de teste no build dir mudaram durante a suite (provavel rebuild concorrente, inclusive transitorio com fonte restaurado ao final) -- TODOS os resultados desta rodada estao invalidados, INCLUSIVE OS VERDES. Confira coredumpctl e re-rode." >&2
  contaminated=1
fi

if [[ "${contaminated}" -eq 1 ]]; then
  exit 1
fi

if [[ "${ctest_rc}" -ne 0 ]]; then
  exit "${ctest_rc}"
fi

echo ""
echo "== ctest_guarded: OK (tracked tree AND test-executable fingerprints unchanged during build+test) =="
