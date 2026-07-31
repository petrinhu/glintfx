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
#   1. The (resolved) sandbox directory is inside this repository.
#      O diretório do sandbox (resolvido) fica dentro deste repositório.
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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

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

# EN: validate_sandbox_path -- shared by create/run/destroy. Resolves the candidate
#     (lexically, via `realpath -m`, so it works even before the directory exists) and
#     applies mandatory refusals 1+2. Prints the resolved absolute path on success.
# PT: validate_sandbox_path -- compartilhada por create/run/destroy. Resolve o candidato
#     (lexicamente, via `realpath -m`, funciona mesmo antes do diretório existir) e
#     aplica as recusas obrigatórias 1+2. Imprime o caminho absoluto resolvido no
#     sucesso.
validate_sandbox_path() {
  local candidate="$1"
  local resolved
  resolved="$(realpath -m -- "${candidate}")"

  case "${resolved}" in
    "${REPO_ROOT}"|"${REPO_ROOT}"/*)
      die "refusing: '${resolved}' resolves INSIDE the repository (${REPO_ROOT}) -- a mutation sandbox must live outside the tree it mutates, or it recreates the exact shared-build-dir contamination BUILDDIR-MUTACAO exists to kill."
      ;;
  esac

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

  # EN: mandatory refusal 3 -- <sha> must resolve to a commit ALREADY in this repo.
  #     Mutating uncommitted work is already forbidden by house rule; this mechanizes
  #     it -- there is no path from an arbitrary working-tree state into a sandbox.
  # PT: recusa obrigatória 3 -- <sha> tem que resolver a um commit JÁ neste repo. Mutar
  #     trabalho não commitado já é proibido pela casa; isto mecaniza a regra -- não
  #     existe caminho de um estado arbitrário de working tree pra um sandbox.
  local resolved_sha
  if ! resolved_sha="$(git -C "${REPO_ROOT}" rev-parse --verify --quiet "${sha}^{commit}" 2>/dev/null)"; then
    die "refusing: '${sha}' does not resolve to a commit in this repo -- mutation sandboxes are built from COMMITTED history only (mutating uncommitted work is already forbidden by house rule). Commit first, then 'create' from the resulting sha."
  fi

  local sandbox_dir
  sandbox_dir="$(validate_sandbox_path "/var/tmp/glx-mutsbx-${name}.$$")"
  [[ -e "${sandbox_dir}" ]] && die "internal: '${sandbox_dir}' already exists (pid collision?) -- retry"

  advise_disk_space

  mkdir -p "${sandbox_dir}"
  info "materializing ${resolved_sha} (from '${sha}') into ${sandbox_dir}"
  git -C "${REPO_ROOT}" archive "${resolved_sha}" | tar -x -C "${sandbox_dir}"

  export TMPDIR=/var/tmp

  local -a cmake_extra_args=()
  local rmlui_cache="${REPO_ROOT}/glintfx/build/_deps/rmlui-src"
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
