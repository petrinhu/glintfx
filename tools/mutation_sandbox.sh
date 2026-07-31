#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: BUILDDIR-MUTACAO (W23) -- mechanizes the "sabotage a COPY outside the tree, never
#     the tracked file" protocol that fatia APP-MINIMIZED already used by hand in W22 and
#     that was the only slice of that wave to leave zero ghost coredumps. Two agents
#     sharing one working tree corrupt each other in THREE distinct ways, discovered in
#     order of subtlety: the INDEX (`git add` colliding), the WORKING TREE (reading a
#     teammate's sabotage as a real defect), and -- the one this script exists for -- the
#     BUILD ARTIFACT. `git commit --only`/`git diff --cached` protect neither of the first
#     two from the third: by the time one agent runs `ctest`, the binary the OTHER agent
#     is mid-mutating is already compiled and on disk. 21 `capture_framebuffer_smoke`
#     coredumps in a closed 6-minute window on 2026-07-30 (TODO.md, BUILDDIR-MUTACAO) were
#     traced to exactly this: one agent's mutation-testing rebuild cycle bleeding into
#     another agent's unrelated full-suite run in the SAME `glintfx/build*` directory.
#     Any full-suite result measured inside a shared-build-dir mutation window is invalid
#     -- including the GREEN ones (a test can pass over a binary another agent mutated a
#     moment earlier and rebuilt back).
#
#     This script gives every mutation-testing session its own self-contained build tree,
#     entirely outside the repo, materialized from a COMMITTED sha (never uncommitted
#     work -- mutating what is not committed is already forbidden by house rule, and
#     requiring a sha here mechanizes that rule instead of just documenting it).
#
# PT: BUILDDIR-MUTACAO (W23) -- mecaniza o protocolo "sabote uma CÓPIA fora da árvore,
#     nunca o arquivo rastreado" que a fatia APP-MINIMIZED já usou à mão na W22 e foi a
#     única daquela onda a não deixar nenhum coredump fantasma. Dois agentes
#     compartilhando uma working tree se corrompem em TRÊS variantes distintas,
#     descobertas em ordem de sutileza: o ÍNDICE (colisão de `git add`), a WORKING TREE
#     (ler sabotagem de colega como defeito real) e -- a que este script existe para
#     resolver -- o ARTEFATO DE BUILD. `git commit --only`/`git diff --cached` não
#     protegem nenhuma das duas primeiras contra a terceira: quando um agente roda o
#     `ctest`, o binário que o OUTRO agente está mutando no meio já está compilado em
#     disco. 21 coredumps de `capture_framebuffer_smoke` numa janela fechada de 6 minutos
#     em 2026-07-30 (TODO.md, BUILDDIR-MUTACAO) foram rastreados a exatamente isto: o
#     ciclo de rebuild de mutation testing de um agente vazando para a rodada de suíte
#     completa, sem relação nenhuma, de outro agente, no MESMO diretório `glintfx/build*`.
#     Qualquer resultado de suíte completa medido dentro de uma janela de mutação com
#     build dir compartilhado é inválido -- inclusive os VERDES (um teste pode passar
#     sobre um binário que outro agente mutou um instante antes e já reconstruiu).
#
#     Este script dá a cada sessão de mutation testing sua própria árvore de build
#     autocontida, inteiramente fora do repo, materializada a partir de um sha COMMITADO
#     (nunca trabalho não commitado -- mutar o que não está commitado já é proibido por
#     regra da casa, e exigir um sha aqui mecaniza essa regra em vez de só documentá-la).
#
# Usage / Uso:
#   tools/mutation_sandbox.sh create <sha> --name <slug>
#       Materializes <sha> (git archive, the committed blob -- never the working tree)
#       into /var/tmp/glx-mutsbx-<slug>.<pid>/, configures (GLINTFX_BACKEND_GLFW=ON,
#       tests ON) and builds Layer 1 (glintfx) inside it. If
#       glintfx/build/_deps/rmlui-src already exists in THIS repo's build dir, it is
#       COPIED (never referenced) into the sandbox and passed as
#       -DFETCHCONTENT_SOURCE_DIR_RMLUI=<copy>, to skip the network fetch -- the sandbox
#       stays self-contained; pointing at the repo's copy would recreate the exact shared
#       artifact contamination this tool exists to kill. Prints the sandbox dir (and
#       nothing else) to stdout on success, so it can be captured:
#           SBOX="$(tools/mutation_sandbox.sh create HEAD --name gl-pack-align)"
#
#       Materializa <sha> (git archive, o blob commitado -- nunca a working tree) em
#       /var/tmp/glx-mutsbx-<slug>.<pid>/, configura (GLINTFX_BACKEND_GLFW=ON, testes ON)
#       e builda a Camada 1 (glintfx) dentro dele. Se glintfx/build/_deps/rmlui-src já
#       existir no diretório de build DESTE repo, é COPIADO (nunca referenciado) para o
#       sandbox e passado como -DFETCHCONTENT_SOURCE_DIR_RMLUI=<cópia>, pra pular o fetch
#       de rede -- o sandbox continua autocontido; apontar pra cópia do repo recriaria
#       exatamente a contaminação de artefato compartilhado que esta ferramenta existe pra
#       matar. Imprime o diretório do sandbox (e só ele) em stdout no sucesso, pra poder
#       ser capturado:
#           SBOX="$(tools/mutation_sandbox.sh create HEAD --name gl-pack-align)"
#
#   tools/mutation_sandbox.sh run <dir> [ctest-args...]
#       Incremental rebuild + `ctest` inside <dir> (a sandbox 'create' produced), under
#       the SAME Xvfb/XDG_RUNTIME_DIR isolation recipe tools/preci.sh's
#       run_layer1_config() already uses (copied whole, then adapted -- a half-copy of a
#       canonical invocation has broken this house before, per feedback_nunca_stress_
#       janela_sessao_viva). Extra arguments are forwarded to `ctest` verbatim, e.g.:
#           tools/mutation_sandbox.sh run "${SBOX}" -R capture_framebuffer_smoke
#       Exit status is ctest's.
#
#       Rebuild incremental + `ctest` dentro de <dir> (um sandbox que o 'create'
#       produziu), sob a MESMA receita de isolamento Xvfb/XDG_RUNTIME_DIR que
#       run_layer1_config() do tools/preci.sh já usa (copiada inteira, só então adaptada
#       -- meia-cópia de invocação canônica já quebrou nesta casa, ver
#       feedback_nunca_stress_janela_sessao_viva). Argumentos extras são repassados ao
#       `ctest` tal como vieram, ex.:
#           tools/mutation_sandbox.sh run "${SBOX}" -R capture_framebuffer_smoke
#       O status de saída é o do ctest.
#
#   tools/mutation_sandbox.sh destroy <dir>
#       Removes the sandbox directory. Refuses anything whose basename does not look
#       like a sandbox this tool created (glx-mutsbx-*), so a typo cannot rm -rf an
#       unrelated /var/tmp directory.
#
#       Remove o diretório do sandbox. Recusa qualquer coisa cujo basename não pareça um
#       sandbox criado por esta ferramenta (glx-mutsbx-*), pra um typo não conseguir dar
#       rm -rf num diretório qualquer de /var/tmp.
#
#   tools/mutation_sandbox.sh --help
#       This text / este texto.
#
# Mandatory refusals (all three subcommands run the same path/sha checks) / Recusas
# obrigatórias (os três subcomandos passam pelas mesmas checagens de path/sha):
#   1. The (resolved) sandbox directory is inside ANY git working tree -- checked
#      against the TARGET (never against where this script happens to live) by
#      find_git_toplevel()'s OWN filesystem walk (never `git rev-parse`'s
#      environment-sensitive discovery, which a poisoned GIT_DIR/GIT_WORK_TREE/
#      GIT_CEILING_DIRECTORIES can fail open on). Fails CLOSED: only a positive proof
#      of reaching `/` without ever finding `.git` allows the target. See
#      find_git_toplevel()'s and validate_sandbox_path()'s header comments for the two
#      2026-07-31 incidents (axis, then fail-open) this design defends against.
#      O diretório do sandbox (resolvido) fica dentro de QUALQUER working tree do git
#      -- checado contra o ALVO (nunca contra onde este script mora) pela PRÓPRIA
#      subida de filesystem de find_git_toplevel() (nunca a descoberta sensível a
#      ambiente do `git rev-parse`, que um GIT_DIR/GIT_WORK_TREE/
#      GIT_CEILING_DIRECTORIES envenenado consegue fazer falhar aberta). Falha
#      FECHADA: só uma prova positiva de chegar em `/` sem nunca achar `.git` libera o
#      alvo. Ver os comentários de cabeçalho de find_git_toplevel() e
#      validate_sandbox_path() pros dois incidentes de 2026-07-31 (eixo, depois
#      fail-open) contra os quais este desenho se defende.
#   2. The (resolved) sandbox directory is under /tmp -- /tmp is tmpfs (RAM) on this
#      machine; a heavy build there can exhaust memory and feed an OOM, not just fail to
#      link. Use /var/tmp (disk).
#      O diretório (resolvido) fica sob /tmp -- /tmp é tmpfs (RAM) nesta máquina; um
#      build pesado ali pode esgotar memória e alimentar OOM, não só falhar o link. Use
#      /var/tmp (disco).
#   3. `create`'s <sha> does not resolve to a commit in this repo -- mutating
#      uncommitted work is already forbidden by house rule; requiring a sha mechanizes
#      that rule instead of relying on discipline.
#      O <sha> de `create` não resolve a um commit deste repo -- mutar trabalho não
#      commitado já é proibido pela casa; exigir um sha mecaniza essa regra em vez de
#      depender de disciplina.
#
# Requires: git, tar, cmake, clang/clang++, xvfb-run, ctest, nproc, realpath, mktemp.
set -euo pipefail

# -----------------------------------------------------------------------------
# EN: helpers
# PT: auxiliares
# -----------------------------------------------------------------------------
usage() {
  grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
}

die() {
  echo "error: $*" >&2
  exit 1
}

info() {
  echo "mutation_sandbox: $*" >&2
}

# EN: find_git_toplevel -- walks up from <start-dir> looking for a `.git` entry
#     (directory OR file -- a FILE covers worktrees/submodules, whose `.git` is a
#     `gitdir: <path>` pointer) at each ancestor up to `/`. Prints the directory that
#     holds it and returns 0; prints nothing and returns 1 if the walk reaches `/`
#     without finding one.
#
#     BUILDDIR-MUTACAO fix (2nd round, found by adversarial review, 2026-07-31): this
#     deliberately does NOT call `git rev-parse --show-toplevel`/`git -C <dir> ...` for
#     the discovery. That command's own discovery is environment-sensitive --
#     GIT_DIR/GIT_WORK_TREE/GIT_CEILING_DIRECTORIES/GIT_DISCOVERY_ACROSS_FILESYSTEM can
#     redirect or truncate it -- and the FIRST round of this fix (still calling
#     `git -C "${probe}" rev-parse --show-toplevel 2>/dev/null`) was FAIL-OPEN on that:
#     a poisoned `GIT_DIR` (e.g. inherited from a prior git operation -- git exports
#     GIT_DIR to child processes, and this repo's own hooks receive it) made the
#     command exit non-zero even INSIDE a real working tree, and "git failed" was
#     silently read as "not a repo, proceed" with no warning. Reimplementing the walk
#     by hand sidesteps the entire class: no git environment variable can influence a
#     plain `[[ -e "${dir}/.git" ]]` filesystem check. The walk also FAILS CLOSED by
#     construction -- there is no path through this function that returns "not a repo"
#     except genuinely reaching `/` without ever finding `.git`; that is a POSITIVE
#     proof of absence, not an absence of proof, which is the property refusal 1 needs
#     (see validate_sandbox_path()'s own header for how the caller enforces this).
# PT: find_git_toplevel -- sobe de <dir-inicial> procurando uma entrada `.git`
#     (diretório OU arquivo -- um ARQUIVO cobre worktree/submódulo, cujo `.git` é um
#     ponteiro `gitdir: <path>`) em cada ancestral até `/`. Imprime o diretório que a
#     contém e retorna 0; não imprime nada e retorna 1 se a subida chegar em `/` sem
#     achar nenhuma.
#
#     Conserto do BUILDDIR-MUTACAO (2ª rodada, achado por review adversarial,
#     2026-07-31): isto deliberadamente NÃO chama `git rev-parse --show-toplevel`/
#     `git -C <dir> ...` pra descoberta. A descoberta desse comando é sensível a
#     ambiente -- GIT_DIR/GIT_WORK_TREE/GIT_CEILING_DIRECTORIES/
#     GIT_DISCOVERY_ACROSS_FILESYSTEM podem redirecionar ou truncar -- e a PRIMEIRA
#     rodada deste conserto (ainda chamando
#     `git -C "${probe}" rev-parse --show-toplevel 2>/dev/null`) era FAIL-OPEN nisso:
#     um `GIT_DIR` envenenado (ex.: herdado de uma operação git anterior -- o git
#     exporta GIT_DIR pra processos filhos, e os próprios hooks deste repo o recebem)
#     fazia o comando sair não-zero mesmo DENTRO de uma working tree real, e "o git
#     falhou" era lido em silêncio como "não é repo, segue" sem aviso nenhum.
#     Reimplementar a subida à mão contorna a classe inteira: nenhuma variável de
#     ambiente do git consegue influenciar um `[[ -e "${dir}/.git" ]]` puro de
#     filesystem. A subida também FALHA FECHADA por construção -- não existe caminho
#     nesta função que retorne "não é repo" exceto chegar de fato em `/` sem nunca
#     achar `.git`; isso é uma PROVA POSITIVA de ausência, não ausência de prova, que é
#     a propriedade que a recusa 1 precisa (ver o próprio cabeçalho de
#     validate_sandbox_path() pra como o chamador aplica isto).
find_git_toplevel() {
  local dir="$1"
  while :; do
    if [[ -e "${dir}/.git" ]]; then
      printf '%s\n' "${dir}"
      return 0
    fi
    [[ "${dir}" == "/" ]] && return 1
    local parent
    parent="$(dirname -- "${dir}")"
    # EN: dirname stopped progressing (should only happen at "/", already handled
    #     above, but guards against any platform quirk) -- treat as inconclusive,
    #     which for THIS function means "no .git found", so the CALLER (which fails
    #     closed on refusal 1, see validate_sandbox_path) is what actually protects.
    # PT: dirname parou de progredir (só deveria acontecer em "/", já tratado acima,
    #     mas protege contra qualquer peculiaridade de plataforma) -- trata como
    #     inconclusivo, que pra ESTA função significa "nenhum .git achado", então o
    #     CHAMADOR (que falha fechado na recusa 1, ver validate_sandbox_path) é quem
    #     de fato protege.
    [[ "${parent}" == "${dir}" ]] && return 1
    dir="${parent}"
  done
}

# EN: validate_sandbox_path -- shared by create/run/destroy. Resolves the candidate
#     (lexically, via `realpath -m`, so it works even before the directory exists) and
#     applies mandatory refusals 1+2. Prints the resolved absolute path on success.
#
#     BUILDDIR-MUTACAO fix (found by adversarial review, 2026-07-31): refusal 1 used to
#     compare the resolved target against ${REPO_ROOT}, which this script derives from
#     ITS OWN location (`dirname "${BASH_SOURCE[0]}"`). That is the wrong axis -- it
#     answers "is the target under where I happen to live?" instead of "does the target
#     belong to ANY git working tree?". Copy this very script outside the repo (exactly
#     what a reviewer verifying the committed blob does, per house protocol) and
#     ${REPO_ROOT} silently becomes wherever the COPY sits, so the guard stops matching
#     the real repo -- `run <repo-root>` from that copy walked straight into rebuilding
#     the shared build dir, the exact contamination this tool exists to kill. Fixed by
#     asking git about the TARGET instead: `git -C <target-or-nearest-existing-ancestor>
#     rev-parse --show-toplevel`. This is also strictly wider than the old check -- it
#     refuses a target inside ANY git repo on the machine, not just this one.
# PT: validate_sandbox_path -- compartilhada por create/run/destroy. Resolve o candidato
#     (lexicamente, via `realpath -m`, funciona mesmo antes do diretório existir) e
#     aplica as recusas obrigatórias 1+2. Imprime o caminho absoluto resolvido no
#     sucesso.
#
#     Conserto do BUILDDIR-MUTACAO (achado por review adversarial, 2026-07-31): a
#     recusa 1 comparava o alvo resolvido contra ${REPO_ROOT}, que este script deriva
#     da PRÓPRIA localização (`dirname "${BASH_SOURCE[0]}"`). Esse é o eixo errado --
#     responde "o alvo está debaixo de onde eu moro?" em vez de "o alvo pertence a
#     QUALQUER working tree do git?". Copie este próprio script pra fora do repo
#     (exatamente o que um revisor verificando o blob commitado faz, pelo protocolo da
#     casa) e ${REPO_ROOT} silenciosamente vira onde a CÓPIA está, então a guarda para
#     de casar com o repo real -- `run <raiz-do-repo>` a partir daquela cópia foi direto
#     rebuildar o build dir compartilhado, a contaminação exata que esta ferramenta
#     existe pra matar. Corrigido perguntando ao git sobre o ALVO em vez disso.
#
#     2ª rodada (achado por review adversarial, 2026-07-31): a 1ª correção acima ainda
#     chamava `git -C "${probe}" rev-parse --show-toplevel`, cuja descoberta é
#     sensível a GIT_DIR/GIT_WORK_TREE/GIT_CEILING_DIRECTORIES -- um `GIT_DIR`
#     envenenado no ambiente (facilmente herdado: git exporta pra filhos, hooks
#     recebem) fazia o comando falhar mesmo DENTRO de um repo real, e a falha era lida
#     como "não é repo, segue" -- fail-open exatamente na recusa que existe pra nunca
#     falhar aberta. Trocado por find_git_toplevel() (ver o cabeçalho dela), que
#     caminha o filesystem à mão sem NUNCA consultar variável de ambiente do git, e só
#     libera quando prova positivamente que chegou em `/` sem achar `.git` nenhum.
validate_sandbox_path() {
  local candidate="$1"
  local resolved
  resolved="$(realpath -m -- "${candidate}")"

  # EN: walk up to the nearest EXISTING ancestor -- 'create' validates a path that does
  #     not exist yet, and find_git_toplevel() needs a real starting directory.
  # PT: sobe até o ancestral EXISTENTE mais próximo -- 'create' valida um caminho que
  #     ainda não existe, e find_git_toplevel() precisa de um diretório real de partida.
  local probe="${resolved}"
  while [[ ! -d "${probe}" && "${probe}" != "/" ]]; do
    probe="$(dirname -- "${probe}")"
  done

  local found_git_at
  if found_git_at="$(find_git_toplevel "${probe}")"; then
    die "refusing: '${resolved}' is inside a git working tree ('.git' found walking up from '${probe}', at '${found_git_at}') -- a mutation sandbox must live outside any repo it might contaminate, or it recreates the exact shared-build-dir contamination BUILDDIR-MUTACAO exists to kill."
  fi

  case "${resolved}" in
    /tmp|/tmp/*)
      die "refusing: '${resolved}' is under /tmp -- /tmp is tmpfs (RAM) on this machine; a heavy glintfx build there can exhaust memory and feed an OOM. Use /var/tmp (disk) instead."
      ;;
  esac

  printf '%s\n' "${resolved}"
}

# EN: best-effort, non-fatal disk advisory -- df lies on btrfs (hides Device
#     unallocated); a missing/unprivileged btrfs command must never block the tool.
# PT: aviso de disco best-effort, não fatal -- df mente em btrfs (esconde o Device
#     unallocated); comando btrfs ausente/sem privilégio nunca deve bloquear a
#     ferramenta.
advise_disk_space() {
  command -v btrfs >/dev/null 2>&1 || return 0
  local usage_out
  usage_out="$(btrfs filesystem usage -m /var/tmp 2>/dev/null)" || return 0
  local min_free_mib
  min_free_mib="$(printf '%s\n' "${usage_out}" | grep -oE 'min:[[:space:]]*[0-9]+\.[0-9]+MiB' | grep -oE '[0-9]+\.[0-9]+' || true)"
  [[ -n "${min_free_mib}" ]] || return 0
  # EN: each sandbox costs ~0.9-1.5 GiB per the brief; warn under 2048 MiB of headroom.
  # PT: cada sandbox custa ~0,9-1,5 GiB pelo brief; avisa abaixo de 2048 MiB de folga.
  if awk -v v="${min_free_mib}" 'BEGIN{exit !(v<2048)}'; then
    echo "warning: btrfs 'Free (estimated) min' is only ~${min_free_mib} MiB -- a sandbox costs ~0.9-1.5 GiB; consider 'destroy'-ing old sandboxes first (see 'btrfs filesystem usage /')." >&2
  fi
}

# -----------------------------------------------------------------------------
# EN: create <sha> --name <slug>
# PT: create <sha> --name <slug>
# -----------------------------------------------------------------------------
cmd_create() {
  local sha="" name=""
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --name)
        [[ $# -ge 2 ]] || die "create: --name requires a value"
        name="$2"
        shift 2
        ;;
      --name=*)
        name="${1#--name=}"
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      -*)
        die "create: unknown option '$1' (see --help)"
        ;;
      *)
        if [[ -z "${sha}" ]]; then
          sha="$1"
          shift
        else
          die "create: unexpected extra argument '$1'"
        fi
        ;;
    esac
  done

  [[ -n "${sha}" ]] || die "create: missing <sha>"
  [[ -n "${name}" ]] || die "create: missing --name <slug>"
  # EN: name is a plain slug (no '/', no leading '.') -- keeps the sandbox path a leaf
  #     under /var/tmp by construction, so it cannot itself be a traversal vector; the
  #     three mandatory refusals are demonstrated via 'run'/'destroy' with a direct bad
  #     <dir> instead (simpler, does not depend on string-concatenation arithmetic).
  # PT: name é um slug simples (sem '/', sem '.' líder) -- mantém o caminho do sandbox
  #     como folha sob /var/tmp por construção, então não pode ser vetor de travessia
  #     sozinho; as três recusas obrigatórias são demonstradas via 'run'/'destroy' com um
  #     <dir> ruim direto (mais simples, não depende de aritmética de concatenação de
  #     string).
  [[ "${name}" =~ ^[A-Za-z0-9][A-Za-z0-9_-]*$ ]] \
    || die "create: --name '${name}' must match ^[A-Za-z0-9][A-Za-z0-9_-]*\$ (no '/', no leading '.')"

  # EN: source repo -- derived from the EXECUTION CONTEXT (current working directory),
  #     via find_git_toplevel() (the same env-var-immune, fail-closed filesystem walk
  #     validate_sandbox_path() uses -- see its own header), NOT from where this script
  #     happens to be stored. Fixed as part of the same 2026-07-31 review that found
  #     refusal 1's fail-open: 'create' used to resolve the repo it archives FROM via
  #     `dirname "${BASH_SOURCE[0]}"`, the exact wrong axis already fixed for the
  #     sandbox-destination check -- surviving here meant 'create' could not be run
  #     from a copy of the script extracted outside the tree, which is precisely how a
  #     reviewer verifies a committed blob in this house. CWD-derivation makes that
  #     work: run this script's WORKING-TREE copy (or any copy) from inside the repo
  #     you want to archive from, regardless of where the script file itself sits.
  # PT: repo-fonte -- derivado do CONTEXTO DE EXECUÇÃO (diretório de trabalho atual),
  #     via find_git_toplevel() (a mesma subida de filesystem imune a variável de
  #     ambiente e fail-closed que validate_sandbox_path() usa -- ver o cabeçalho
  #     dela), NÃO de onde este script está armazenado. Corrigido como parte da mesma
  #     revisão de 2026-07-31 que achou o fail-open da recusa 1: o 'create' resolvia o
  #     repo de onde arquiva via `dirname "${BASH_SOURCE[0]}"`, o mesmo eixo errado já
  #     consertado pra checagem do destino -- sobreviver aqui significava que o
  #     'create' não podia rodar a partir de uma cópia do script extraída pra fora da
  #     árvore, que é exatamente como um revisor verifica um blob commitado nesta
  #     casa. Derivar do CWD resolve isso: rode a cópia da WORKING TREE deste script
  #     (ou qualquer cópia) de dentro do repo de onde quer arquivar, independente de
  #     onde o arquivo do script está.
  local source_repo_root
  if ! source_repo_root="$(find_git_toplevel "$(pwd -P)")"; then
    die "create: current directory ($(pwd -P)) is not inside a git working tree -- 'create' archives from a repo found by walking up from the CWD, not from where this script file happens to live. Run it from inside the repo you want to archive from."
  fi

  # EN: mandatory refusal 3 -- <sha> must resolve to a commit ALREADY in the source
  #     repo. Mutating uncommitted work is already forbidden by house rule; this
  #     mechanizes it -- there is no path from an arbitrary working-tree state into a
  #     sandbox.
  # PT: recusa obrigatória 3 -- <sha> tem que resolver a um commit JÁ no repo-fonte.
  #     Mutar trabalho não commitado já é proibido pela casa; isto mecaniza a regra --
  #     não existe caminho de um estado arbitrário de working tree pra um sandbox.
  local resolved_sha
  if ! resolved_sha="$(git -C "${source_repo_root}" rev-parse --verify --quiet "${sha}^{commit}" 2>/dev/null)"; then
    die "refusing: '${sha}' does not resolve to a commit in ${source_repo_root} -- mutation sandboxes are built from COMMITTED history only (mutating uncommitted work is already forbidden by house rule). Commit first, then 'create' from the resulting sha."
  fi

  local sandbox_dir
  sandbox_dir="$(validate_sandbox_path "/var/tmp/glx-mutsbx-${name}.$$")"
  [[ -e "${sandbox_dir}" ]] && die "internal: '${sandbox_dir}' already exists (pid collision?) -- retry"

  advise_disk_space

  mkdir -p "${sandbox_dir}"
  info "materializing ${resolved_sha} (from '${sha}') into ${sandbox_dir}"
  git -C "${source_repo_root}" archive "${resolved_sha}" | tar -x -C "${sandbox_dir}"

  export TMPDIR=/var/tmp

  local -a cmake_extra_args=()
  local rmlui_cache="${source_repo_root}/glintfx/build/_deps/rmlui-src"
  if [[ -d "${rmlui_cache}" ]]; then
    local rmlui_copy="${sandbox_dir}/_rmlui-src-cache"
    info "copying cached RmlUi FetchContent source (${rmlui_cache}) into the sandbox -- self-contained, not referenced"
    cp -a "${rmlui_cache}" "${rmlui_copy}"
    cmake_extra_args+=("-DFETCHCONTENT_SOURCE_DIR_RMLUI=${rmlui_copy}")
  else
    echo "warning: no cached RmlUi source at ${rmlui_cache} -- 'create' will fetch it over the network" >&2
  fi

  local build_dir="${sandbox_dir}/glintfx/build"
  info "configuring (GLINTFX_BACKEND_GLFW=ON, GLINTFX_BUILD_TESTS=ON)"
  cmake -S "${sandbox_dir}/glintfx" -B "${build_dir}" \
    -DGLINTFX_BUILD_TESTS=ON \
    -DGLINTFX_BACKEND_GLFW=ON \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    "${cmake_extra_args[@]}"

  info "building (-j$(nproc))"
  cmake --build "${build_dir}" -j"$(nproc)"

  info "sandbox ready: ${sandbox_dir}"
  printf '%s\n' "${sandbox_dir}"
}

# -----------------------------------------------------------------------------
# EN: run <dir> [ctest-args...]
# PT: run <dir> [args do ctest...]
# -----------------------------------------------------------------------------
cmd_run() {
  [[ $# -ge 1 ]] || die "run: missing <dir>"
  local dir="$1"
  shift

  local resolved
  resolved="$(validate_sandbox_path "${dir}")"
  [[ -d "${resolved}" ]] || die "run: '${resolved}' is not a directory (did 'create' run first?)"

  local build_dir="${resolved}/glintfx/build"
  [[ -d "${build_dir}" ]] || die "run: no build directory at ${build_dir} -- 'create' must run first"

  export TMPDIR=/var/tmp

  info "rebuilding (incremental, -j$(nproc)) ${build_dir}"
  cmake --build "${build_dir}" -j"$(nproc)"

  # EN: QA-XVFBISO recipe, copied WHOLE from tools/preci.sh's run_layer1_config() and
  #     only then adapted (fake_xdg_runtime under TMPDIR, which this tool already forces
  #     to /var/tmp -- so this is strictly better than preci.sh's own
  #     ${TMPDIR:-/tmp} fallback, never worse). Isolation belongs to whoever executes,
  #     not to whoever calls -- see preci.sh's own header comment for the full 2026-07-23
  #     incident this defends against. Cleanup is explicit capture-rc/rm, NOT a
  #     `trap ... RETURN` (verified in that same incident that RETURN traps do not fire
  #     reliably when `set -e` aborts inside the function).
  # PT: receita QA-XVFBISO, copiada INTEIRA de run_layer1_config() do tools/preci.sh e só
  #     então adaptada (fake_xdg_runtime sob TMPDIR, que esta ferramenta já força pra
  #     /var/tmp -- portanto estritamente melhor que o fallback próprio do preci.sh
  #     ${TMPDIR:-/tmp}, nunca pior). Isolamento pertence a quem executa, não a quem
  #     chama -- ver o próprio comentário de cabeçalho do preci.sh pro incidente completo
  #     de 2026-07-23 contra o qual isto se defende. A limpeza é um capture-rc/rm
  #     explícito, NÃO um `trap ... RETURN` (verificado no mesmo incidente que traps
  #     RETURN não disparam de forma confiável quando o `set -e` aborta dentro da
  #     função).
  local fake_xdg_runtime
  fake_xdg_runtime="$(mktemp -d "${TMPDIR}/glintfx-mutsbx-xvfb-runtime.XXXXXX")"

  local xvfb_rc=0
  env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR="${fake_xdg_runtime}" \
    xvfb-run -a ctest --test-dir "${build_dir}" --output-on-failure "$@" || xvfb_rc=$?
  rm -rf "${fake_xdg_runtime}"

  exit "${xvfb_rc}"
}

# -----------------------------------------------------------------------------
# EN: destroy <dir>
# PT: destroy <dir>
# -----------------------------------------------------------------------------
cmd_destroy() {
  [[ $# -ge 1 ]] || die "destroy: missing <dir>"
  local dir="$1"

  local resolved
  resolved="$(validate_sandbox_path "${dir}")"

  if [[ ! -e "${resolved}" ]]; then
    info "'${resolved}' does not exist -- nothing to destroy"
    return 0
  fi

  # EN: extra guard beyond the 3 mandatory refusals -- only remove directories whose
  #     basename looks like something THIS tool created, so a typo'd <dir> cannot
  #     rm -rf an unrelated /var/tmp directory.
  # PT: guarda extra além das 3 recusas obrigatórias -- só remove diretórios cujo
  #     basename parece algo que ESTA ferramenta criou, pra um <dir> com typo não dar
  #     rm -rf num diretório qualquer de /var/tmp.
  case "$(basename -- "${resolved}")" in
    glx-mutsbx-*) ;;
    *)
      die "refusing: '${resolved}' does not look like a mutation sandbox (expected basename 'glx-mutsbx-*') -- pass the exact directory 'create' printed"
      ;;
  esac

  rm -rf -- "${resolved}"
  info "destroyed ${resolved}"
}

# -----------------------------------------------------------------------------
# EN: dispatch
# PT: despacho
# -----------------------------------------------------------------------------
main() {
  if [[ $# -eq 0 ]]; then
    usage
    exit 2
  fi

  local cmd="$1"
  shift

  case "${cmd}" in
    create)
      cmd_create "$@"
      ;;
    run)
      cmd_run "$@"
      ;;
    destroy)
      cmd_destroy "$@"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown subcommand '${cmd}' (see --help)" >&2
      exit 2
      ;;
  esac
}

main "$@"
