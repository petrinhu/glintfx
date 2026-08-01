#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
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
#     `command[0]` alone is `/usr/bin/cmake` for those and useless), then fingerprint each
#     one -- see fingerprint_executables()'s own comment and the SECOND ADVERSARIAL REVIEW
#     ROUND block below for what exactly that fingerprint is made of and why (it changed
#     twice more after this paragraph was first written).
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
#     fingerprinta cada um -- ver o comentário próprio de fingerprint_executables() e o
#     bloco SEGUNDA RODADA DE REVIEW ADVERSARIAL abaixo pro que exatamente esse
#     fingerprint contém e por quê (mudou mais duas vezes depois deste parágrafo ter sido
#     escrito pela primeira vez).
#
#     SECOND ADVERSARIAL REVIEW ROUND (2026-07-31, team-lead review against 6c6c941/
#     cb57372, three findings, all MEASURED):
#       1. EMPTY TEST LIST: `ctest --show-only=json-v1 -R <filter-matching-nothing>`
#          returns exit=0 with ZERO tests (measured on the real build-preci). sha256sum of
#          empty input is a fixed constant, so an empty fingerprinted set makes
#          before==after TRIVIALLY on every run -- the guard would report OK while
#          fingerprinting NOTHING. Fixed by refusing (exit 2) when list_test_executables()
#          returns nothing; see that refusal's own comment near where it is checked.
#       2. MTIME RESOLUTION -- AND CONTENT HASH ALONE IS ALSO WRONG: `stat`'s `%Y` is
#          SECONDS only, and two genuine rebuilds were measured completing in ~1.16s
#          total, so a mutate/rebuild/restore/rebuild cycle can land both writes in the
#          SAME wall-clock second and collide, silently passing a contaminated run. The
#          team-lead's initial fix proposal -- switch fully to a content hash -- was
#          independently found HERE to be its own regression before it shipped: this
#          toolchain compiles DETERMINISTICALLY (verified live: two builds from
#          byte-identical source produce a byte-identical executable, confirmed via
#          sha256sum, no embedded build-id/timestamp), so a mutation that fully
#          round-trips back to the original source -- the EXACT transient-mutation
#          scenario this whole file exists to catch -- also round-trips back to the
#          original CONTENT. A content-hash-ONLY fingerprint would silently miss that
#          case, the same failure mode as the mtime collision, just via a different
#          mechanism, and it would have been a regression against the drill this file was
#          already passing. Fixed by combining BOTH signals instead of swapping one for
#          the other: nanosecond-resolution mtime (`stat --format '%.9Y'`, confirmed
#          available on this machine -- fixes the collision without losing the "any write
#          moves the fingerprint" property that a fully-reverted content round-trip
#          depends on) AND the content hash (catches a deliberate metadata spoof, e.g.
#          `touch -r`, that mtime alone -- at any resolution -- cannot). See
#          fingerprint_executables() for the combined implementation and the exact
#          measurement that justified departing from the team-lead's literal instruction.
#       3. REPO_ROOT ANCHOR: SCRIPT_DIR/REPO_ROOT used to derive from the script's OWN
#          file location (`dirname "${BASH_SOURCE[0]}"`) -- the exact bug class this
#          house's "verify the committed blob outside the tree" protocol already broke
#          once in tools/mutation_sandbox.sh. Copy this script to a scratch dir to review
#          it and REPO_ROOT silently becomes wherever the copy sits; `cd` there then
#          breaks every git command with `fatal: not a git repository` (measured: falls
#          through to an unhandled `git diff --no-index`, exit 129, outside this script's
#          own documented contract). Fixed by porting mutation_sandbox.sh's
#          find_git_toplevel() (the same battle-tested, thrice-reviewed algorithm -- see
#          that function's own header there for the full three-round history) anchored on
#          the resolved <build-dir> argument instead of the script's own location:
#          <build-dir> necessarily lives inside the real repo being tested, so anchoring
#          there is robust to the script itself being copied anywhere.
#
#     SEGUNDA RODADA DE REVIEW ADVERSARIAL (2026-07-31, review do team-lead contra
#     6c6c941/cb57372, três achados, todos MEDIDOS):
#       1. LISTA DE TESTES VAZIA: `ctest --show-only=json-v1 -R <filtro-que-nao-casa-nada>`
#          retorna exit=0 com ZERO testes (medido no build-preci real). sha256sum de
#          entrada vazia é uma constante fixa, então um conjunto fingerprintado vazio faz
#          before==after TRIVIALMENTE em toda rodada -- o guard reportaria OK
#          fingerprintando NADA. Corrigido recusando (exit 2) quando
#          list_test_executables() não retorna nada; ver o comentário próprio dessa
#          recusa perto de onde é checada.
#       2. RESOLUÇÃO DE MTIME -- E HASH DE CONTEÚDO SOZINHO TAMBÉM ESTÁ ERRADO: o `%Y` do
#          `stat` é só SEGUNDOS, e dois rebuilds genuínos foram medidos completando em
#          ~1,16s no total, então um ciclo mutar/rebuildar/restaurar/rebuildar pode
#          encaixar as duas escritas no MESMO segundo de relógio e colidir, aprovando em
#          silêncio uma rodada contaminada. A proposta de conserto inicial do team-lead --
#          trocar inteiramente pra hash de conteúdo -- foi encontrada AQUI, de forma
#          independente, como sendo sua PRÓPRIA regressão antes de sequer ir pro ar: esta
#          toolchain compila DETERMINISTICAMENTE (verificado ao vivo: dois builds a partir
#          de fonte byte-idêntico produzem um executável byte-idêntico, confirmado via
#          sha256sum, sem build-id/timestamp embutido), então uma mutação que faz
#          round-trip completo de volta ao fonte original -- o cenário EXATO de mutação
#          transitória que este arquivo inteiro existe pra pegar -- também faz round-trip
#          de volta ao CONTEÚDO original. Um fingerprint só de hash de conteúdo perderia
#          esse caso em silêncio, o mesmo modo de falha da colisão de mtime, só que por um
#          mecanismo diferente, e teria sido uma regressão contra o drill que este arquivo
#          já estava passando. Corrigido combinando OS DOIS sinais em vez de trocar um
#          pelo outro: mtime com resolução de nanossegundo (`stat --format '%.9Y'`,
#          confirmado disponível nesta máquina -- conserta a colisão sem perder a
#          propriedade "qualquer escrita move o fingerprint" da qual um round-trip de
#          conteúdo totalmente revertido depende) E o hash de conteúdo (pega um spoof
#          deliberado de metadado, ex. `touch -r`, que mtime sozinho -- em qualquer
#          resolução -- não pega). Ver fingerprint_executables() pra implementação
#          combinada e a medição exata que justificou se afastar da instrução literal do
#          team-lead.
#       3. ÂNCORA DO REPO_ROOT: SCRIPT_DIR/REPO_ROOT costumavam derivar da PRÓPRIA
#          localização do arquivo do script (`dirname "${BASH_SOURCE[0]}"`) -- a mesma
#          classe de bug que o protocolo desta casa de "verificar o blob commitado fora
#          da árvore" já quebrou uma vez no tools/mutation_sandbox.sh. Copie este script
#          pra um dir de scratch pra revisar e o REPO_ROOT silenciosamente vira onde a
#          cópia está; o `cd` pra lá então quebra todo comando git com `fatal: not a git
#          repository` (medido: cai num `git diff --no-index` sem tratamento, exit 129,
#          fora do contrato que o próprio --help deste script documenta). Corrigido
#          portando o find_git_toplevel() do mutation_sandbox.sh (o mesmo algoritmo
#          batalhado e revisado três vezes -- ver o comentário de cabeçalho próprio
#          daquela função lá pra história completa das três rodadas) ancorado no
#          argumento <build-dir> resolvido em vez da localização do próprio script:
#          <build-dir> necessariamente vive dentro do repo real sob teste, então ancorar
#          ali é robusto a este script ser copiado pra qualquer lugar.
#
# Usage / Uso:
#   tools/ctest_guarded.sh <build-dir> [ctest-args...]
#   tools/ctest_guarded.sh --help
#
#   Example / Exemplo:
#     tools/ctest_guarded.sh glintfx/build-preci
#     tools/ctest_guarded.sh glintfx/build-preci -R render_sanity
#
#   SCOPE OF THE ARTIFACT FINGERPRINT, HONESTLY STATED: [ctest-args...] (any -R/-E/-L
#   selection you pass) is forwarded to the `--show-only=json-v1` call that builds the
#   fingerprinted executable list, so the fingerprint always covers EXACTLY the binaries
#   THIS invocation is about to exercise -- not the whole build dir's test set. Run
#   `tools/ctest_guarded.sh glintfx/build-preci -R render_sanity` and only the ~5
#   executables matching `render_sanity` are fingerprinted; a concurrent rebuild of an
#   unrelated test binary would NOT be caught by that narrower invocation (it is outside
#   what was tested, so it is honestly outside what is guarded). Escopo do fingerprint,
#   declarado com honestidade: os args de seleção do ctest (-R/-E/-L) são repassados pra
#   chamada `--show-only=json-v1`, então o fingerprint cobre SEMPRE exatamente os
#   binários que ESTA invocação vai exercitar -- não o conjunto de teste inteiro do
#   build dir. Rebuild concorrente de um binário não relacionado a uma rodada filtrada
#   não é pego por ela (está fora do que foi testado, então honestamente está fora do
#   que é guardado).
#
# Exit status:
#   0   -- clean run: tracked tree AND fingerprinted test-executable set unchanged, ctest
#          passed.
#   1   -- CONTAMINATED run (BUILDDIR-MUTACAO): either the tracked tree under glintfx/
#          (minus build*/) moved between the pre-build and post-ctest tree snapshots, OR
#          one of the fingerprinted test executables' content hash + nanosecond mtime
#          changed between the post-build and post-ctest artifact fingerprints (a
#          concurrent rebuild, even a transient one that restores identical source and
#          identical content -- see the SECOND ADVERSARIAL REVIEW ROUND block above for
#          why both signals are combined). The matching canonical message is printed
#          verbatim on stderr (both print if both triggered); re-run once the tree/build
#          dir are quiet. This exit code takes priority over ctest's own exit code -- a
#          contaminated run is invalid REGARDLESS of whether ctest itself reported pass or
#          fail.
#   N>0 -- ctest's own exit code, propagated as-is, when NEITHER contamination check
#          fired (a genuine red suite).
#   2   -- usage/environment error: missing/invalid <build-dir>, not a configured CMake
#          build dir, a required tool (jq) missing, <build-dir> not inside any git working
#          tree (see REPO_ROOT ANCHOR above), or the given <build-dir>/selection args
#          matched ZERO tests (see EMPTY TEST LIST above -- refusing to fingerprint
#          nothing and call it a guarded run).
#
# Requires: git, cmake, ctest, xvfb-run, jq, coreutils (realpath/stat/sha256sum -- already
#           assumed elsewhere in this repo's tooling). Assumes <build-dir> was already
#           configured (has a CMakeCache.txt) -- this script builds+tests, it does not
#           configure.
set -euo pipefail

# EN: --help works from ANY location (reads its own file's comments, no repo/CWD
#     dependency) -- checked BEFORE anything that needs REPO_ROOT below.
# PT: --help funciona de QUALQUER localização (lê os comentários do próprio arquivo, sem
#     depender de repo/CWD) -- checado ANTES de qualquer coisa que precise de REPO_ROOT
#     abaixo.
if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]; then
  grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
  exit 0
fi

BUILD_DIR_ARG="$1"
shift
CTEST_ARGS=("$@")

# EN: resolve <build-dir> to an absolute path BEFORE any `cd`, anchored to the CALLER's
#     CWD at invocation time (the natural interpretation of a relative path the user
#     typed). This absolute path is also the anchor used below to find REPO_ROOT -- see
#     the REPO_ROOT ANCHOR entry in the SECOND ADVERSARIAL REVIEW ROUND header block for
#     why this script no longer derives REPO_ROOT from its OWN location.
# PT: resolve <build-dir> pra um caminho absoluto ANTES de qualquer `cd`, ancorado no CWD
#     de quem chamou no momento da invocação (a interpretação natural de um caminho
#     relativo que o usuário digitou). Este caminho absoluto também é a âncora usada
#     abaixo pra achar o REPO_ROOT -- ver a entrada ÂNCORA DO REPO_ROOT no bloco de
#     cabeçalho SEGUNDA RODADA DE REVIEW ADVERSARIAL pro porquê deste script não derivar
#     mais o REPO_ROOT da PRÓPRIA localização.
BUILD_DIR="$(realpath -m -- "${BUILD_DIR_ARG}")"

if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  echo "error: '${BUILD_DIR}/CMakeCache.txt' not found -- <build-dir> ('${BUILD_DIR_ARG}', resolved to '${BUILD_DIR}') must already be configured (run cmake -S ... -B '${BUILD_DIR_ARG}' first)." >&2
  exit 2
fi

if ! command -v jq >/dev/null 2>&1; then
  echo "error: 'jq' not found in PATH -- required for the test-executable fingerprint (see the SECOND ADVERSARIAL REVIEW ROUND header comment). This is a house-canonical tool (TOOLING.md); install it rather than skipping the check -- a guard that silently degrades gives false confidence." >&2
  exit 2
fi

# EN: find_git_toplevel -- PORTED VERBATIM (same battle-tested, thrice-adversarially-
#     reviewed algorithm, not re-derived) from tools/mutation_sandbox.sh's function of the
#     same name. See that function's own header comment in that file for the full
#     three-round history of why it looks exactly like this (deliberately never consults
#     git itself -- GIT_DIR/GIT_WORK_TREE/GIT_CEILING_DIRECTORIES can poison `git
#     rev-parse --show-toplevel` into a false "not a repo" even from inside a real one --
#     and deliberately treats an unreadable ancestor as INCONCLUSIVE (return 2), not
#     "clear", because a silent permission failure reading that far up must never be
#     misread as a positive "no .git here"). Three-way return contract, caller MUST
#     handle all three:
#       0 -- found. Prints the directory containing `.git`.
#       1 -- POSITIVELY not a repo: walked all the way to `/`, every ancestor on the path
#            was genuinely searchable. Prints nothing.
#       2 -- INCONCLUSIVE: an ancestor exists but is not searchable (missing 'x'), so the
#            walk cannot see whether `.git` is inside it. Prints that ancestor's path.
# PT: find_git_toplevel -- PORTADA VERBATIM (o mesmo algoritmo batalhado e revisado três
#     vezes adversarialmente, não re-derivado) da função de mesmo nome no
#     tools/mutation_sandbox.sh. Ver o comentário de cabeçalho próprio daquela função
#     naquele arquivo pra história completa das três rodadas do porquê dela ser
#     exatamente assim (deliberadamente nunca consulta o próprio git --
#     GIT_DIR/GIT_WORK_TREE/GIT_CEILING_DIRECTORIES podem envenenar `git rev-parse
#     --show-toplevel` num falso "não é repo" mesmo de dentro de um real -- e
#     deliberadamente trata um ancestral ilegível como INCONCLUSIVO (retorna 2), não
#     "livre", porque uma falha silenciosa de permissão lendo lá em cima nunca pode ser
#     lida como um "não tem .git aqui" positivo). Contrato de retorno de três vias, quem
#     chama TEM que tratar as três:
#       0 -- achou. Imprime o diretório que contém o `.git`.
#       1 -- POSITIVAMENTE não é repo: subiu até `/`, todo ancestral no caminho era de
#            fato varrível. Não imprime nada.
#       2 -- INCONCLUSIVO: um ancestral existe mas não é varrível (falta o 'x'), então a
#            subida não consegue ver se `.git` está dentro dele. Imprime o caminho desse
#            ancestral.
find_git_toplevel() {
  local dir="$1"
  while :; do
    if [[ -d "${dir}" && ! -x "${dir}" ]]; then
      printf '%s\n' "${dir}"
      return 2
    fi

    if [[ -e "${dir}/.git" ]]; then
      printf '%s\n' "${dir}"
      return 0
    fi
    [[ "${dir}" == "/" ]] && return 1
    local parent
    parent="$(dirname -- "${dir}")"
    [[ "${parent}" == "${dir}" ]] && return 1
    dir="${parent}"
  done
}

REPO_ROOT=""
git_toplevel_rc=0
REPO_ROOT="$(find_git_toplevel "${BUILD_DIR}")" || git_toplevel_rc=$?
case "${git_toplevel_rc}" in
  0)
    ;;
  1)
    echo "error: '${BUILD_DIR}' is not inside any git working tree (walked up to '/' without finding '.git') -- this script must run against a real checkout of the repo containing <build-dir>, not a copy of the script alone. If you copied ctest_guarded.sh out of the tree to verify the committed blob per house protocol, point <build-dir> at the real repo's build dir -- do not run the extracted script against a scratch copy of the whole tree." >&2
    exit 2
    ;;
  2)
    echo "error: cannot determine the git repo root for '${BUILD_DIR}' -- ancestor '${REPO_ROOT}' exists but is not searchable (missing the 'x' permission bit), so walking up cannot see whether '.git' is inside it." >&2
    exit 2
    ;;
  *)
    echo "error: find_git_toplevel returned unexpected exit code ${git_toplevel_rc}" >&2
    exit 2
    ;;
esac

cd "${REPO_ROOT}"

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
#     "${BUILD_DIR}" GIVEN THE SAME SELECTION ARGS as the real run (CTEST_ARGS, e.g. -R/
#     -E/-L), one absolute path per line, deduplicated. `--show-only=json-v1` honors those
#     filters exactly like a normal run does (verified: `-R render_sanity` on the real
#     build-preci narrows 125 tests to 5), so passing CTEST_ARGS here is required --
#     without it this function fingerprints the WHOLE suite while the actual invocation
#     below only exercises a filtered subset, and the guard would be fingerprinting a
#     different set than it tests (found by team-lead review, 2026-07-31).
#     See the TRANSIENT-MUTATION ADDENDUM header comment for why command[0] alone is not
#     enough (most tests here are wrapped by glintfx/tests/run_xvfb.cmake, so command[0]
#     is `/usr/bin/cmake` and the real binary is the value of a `-DEXE=<path>` argument).
# PT: list_test_executables -- o conjunto exato de arquivos que o ctest está pra invocar
#     em "${BUILD_DIR}" DADOS OS MESMOS ARGS DE SELEÇÃO da rodada real (CTEST_ARGS, ex.
#     -R/-E/-L), um caminho absoluto por linha, deduplicado. `--show-only=json-v1`
#     respeita esses filtros igual a uma rodada normal (verificado: `-R render_sanity` no
#     build-preci real estreita 125 testes pra 5), então passar CTEST_ARGS aqui é
#     obrigatório -- sem isso esta função fingerprinta a SUÍTE INTEIRA enquanto a
#     invocação real abaixo só exercita um subconjunto filtrado, e o guard estaria
#     fingerprintando um conjunto diferente do que testa (achado por review do
#     team-lead, 2026-07-31).
#     Ver o comentário de cabeçalho ADENDO MUTAÇÃO TRANSITÓRIA pro porquê de command[0]
#     sozinho não bastar (a maioria dos testes aqui é embrulhada por
#     glintfx/tests/run_xvfb.cmake, então command[0] é `/usr/bin/cmake` e o binário real
#     é o valor de um argumento `-DEXE=<caminho>`).
list_test_executables() {
  local build_dir="$1"
  shift
  local -a select_args=("$@")
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
  ctest --show-only=json-v1 --test-dir "${build_dir}" "${select_args[@]}" | jq -r '
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
#     list_test_executables) and returns a single hash covering, for each existing file,
#     BOTH its content (sha256) AND its mtime at nanosecond resolution (`stat --format
#     '%.9Y'`). A path that does not exist is fingerprinted as literally "MISSING" instead
#     of being skipped, so a binary deleted out from under the run also counts as a
#     mismatch. Bulk `stat`/`sha256sum` calls (one invocation over the whole file list,
#     not one process per file) -- this is what keeps the cost the team-lead measured
#     (0.58s over 120 executables / 521 MB) actually representative.
#
#     WHY BOTH SIGNALS, NOT JUST CONTENT HASH (see the MTIME RESOLUTION entry of the
#     SECOND ADVERSARIAL REVIEW ROUND header block for the full account): content hash
#     alone is the team-lead's literal instruction, and it is the right fix for a
#     DELIBERATE metadata spoof (`touch -r` restoring old mtime+size over genuinely
#     different content) -- but it is ALSO, independently, a silent regression for the
#     transient-mutation scenario this whole file exists to catch. This toolchain
#     compiles deterministically (measured: `touch src/audio.cpp && cmake --build
#     --target audio_hostile_sanity` from otherwise-unchanged source reproduces a
#     byte-identical executable, confirmed via sha256sum) -- so a mutate/rebuild/restore/
#     rebuild cycle that fully round-trips back to the original SOURCE also round-trips
#     back to the original CONTENT, and a content-hash-only fingerprint would see no
#     difference at all, same failure mode as the mtime-collision bug, via a different
#     mechanism. Nanosecond mtime alone would fix the collision (the actual bug measured)
#     but stays vulnerable to a deliberate spoof; content hash alone would fix the spoof
#     but reintroduces the transient-mutation blind spot. Combining both keeps each
#     signal's strength where the other is blind, at negligible extra cost (`stat` is a
#     rounding error next to `sha256sum` over the same file set).
# PT: fingerprint_executables -- lê caminhos da stdin (um por linha, como
#     list_test_executables produz) e retorna um único hash cobrindo, pra cada arquivo
#     existente, TANTO o conteúdo (sha256) QUANTO o mtime em resolução de nanossegundo
#     (`stat --format '%.9Y'`). Um caminho que não existe é registrado literalmente como
#     "MISSING" em vez de pulado, então um binário apagado por baixo da rodada também
#     conta como divergência. Chamadas de `stat`/`sha256sum` em lote (uma invocação sobre
#     a lista inteira de arquivos, não um processo por arquivo) -- é isso que mantém o
#     custo que o team-lead mediu (0,58s sobre 120 executáveis / 521 MB) de fato
#     representativo.
#
#     POR QUE OS DOIS SINAIS, NÃO SÓ HASH DE CONTEÚDO (ver a entrada RESOLUÇÃO DE MTIME
#     do bloco de cabeçalho SEGUNDA RODADA DE REVIEW ADVERSARIAL pro relato completo):
#     hash de conteúdo sozinho é a instrução literal do team-lead, e é o conserto certo
#     pra um spoof DELIBERADO de metadado (`touch -r` restaurando mtime+tamanho antigos
#     sobre um conteúdo genuinamente diferente) -- mas é TAMBÉM, independentemente, uma
#     regressão silenciosa pro cenário de mutação transitória que este arquivo inteiro
#     existe pra pegar. Esta toolchain compila deterministicamente (medido: `touch
#     src/audio.cpp && cmake --build --target audio_hostile_sanity`, sem NENHUMA
#     alteração de conteúdo além do touch, reproduz um executável byte-idêntico,
#     confirmado via sha256sum) -- então um ciclo mutar/rebuildar/restaurar/rebuildar que
#     faz round-trip
#     completo de volta ao FONTE original também faz round-trip de volta ao CONTEÚDO
#     original, e um fingerprint só de hash de conteúdo não veria diferença nenhuma,
#     mesmo modo de falha do bug de colisão de mtime, via mecanismo diferente. mtime em
#     nanossegundo sozinho consertaria a colisão (o bug de fato medido) mas continua
#     vulnerável a um spoof deliberado; hash de conteúdo sozinho consertaria o spoof mas
#     reintroduz o ponto cego de mutação transitória. Combinar os dois mantém a força de
#     cada sinal onde o outro é cego, a custo extra desprezível (`stat` é erro de
#     arredondamento perto de `sha256sum` sobre o mesmo conjunto de arquivos).
fingerprint_executables() {
  local -a existing=()
  local -a missing=()
  local f
  while IFS= read -r f; do
    [[ -z "${f}" ]] && continue
    if [[ -f "${f}" ]]; then
      existing+=("${f}")
    else
      missing+=("${f}")
    fi
  done

  {
    if [[ "${#existing[@]}" -gt 0 ]]; then
      stat --format '%n %.9Y' -- "${existing[@]}"
      sha256sum -- "${existing[@]}"
    fi
    local m
    for m in "${missing[@]}"; do
      printf '%s MISSING\n' "${m}"
    done
  } | sha256sum | awk '{print $1}'
}

before_hash="$(snapshot)"

section "cmake --build ${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"

section "fingerprint test executables (TRANSIENT-MUTATION ADDENDUM)"
test_exe_list="$(list_test_executables "${BUILD_DIR}" "${CTEST_ARGS[@]}")"

# EN: EMPTY-TEST-LIST refusal (found by adversarial review, 2026-07-31, MEASURED: `ctest
#     --show-only=json-v1 -R nome_que_nao_existe_xyz` on the real build-preci returns
#     exit=0 with ZERO tests). sha256sum of empty input is a fixed constant, so an empty
#     list would make artifact_before == artifact_after TRIVIALLY on every run, regardless
#     of what happened to the build dir -- the guard would report OK while fingerprinting
#     NOTHING. The realistic trigger is not an attacker: GLINTFX_BUILD_TESTS accidentally
#     OFF at configure time, a half-finished configure, or <build-dir>/CTEST_ARGS pointing
#     at the wrong place. Nobody runs this guard on purpose over zero tests; if that
#     happens, it is an operator error and deserves to stop, not a silent pass.
# PT: recusa de LISTA VAZIA (achado por review adversarial, 2026-07-31, MEDIDO: `ctest
#     --show-only=json-v1 -R nome_que_nao_existe_xyz` no build-preci real retorna exit=0
#     com ZERO testes). sha256sum de entrada vazia é uma constante fixa, então uma lista
#     vazia faria artifact_before == artifact_after TRIVIALMENTE em toda rodada,
#     independente do que aconteceu com o build dir -- o guard reportaria OK
#     fingerprintando NADA. O gatilho realista não é um atacante: GLINTFX_BUILD_TESTS
#     acidentalmente OFF no configure, um configure pela metade, ou <build-dir>/
#     CTEST_ARGS apontando pro lugar errado. Ninguém roda este guard de propósito sobre
#     zero testes; se acontecer, é erro de operação e merece parar, não passar em
#     silêncio.
if [[ -z "${test_exe_list}" ]]; then
  echo "error: 'ctest --show-only=json-v1' matched ZERO tests for '${BUILD_DIR}' with the given selection args -- refusing to fingerprint nothing and call it a guarded run. Check GLINTFX_BUILD_TESTS at configure time, <build-dir>, and any -R/-E/-L filter." >&2
  exit 2
fi

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
