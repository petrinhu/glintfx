#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Local pre-CI gate (TST-L1-PRECI, TESTES.md). Mirrors the fast day-to-day slice of
#     CI BEFORE code becomes visible on a remote (líder's decision 2026-07-10: hooked at
#     pre-push, not pre-commit -- the glintfx suite takes ~2min warm, which would break
#     the flow of small frequent commits; pre-push is "before it becomes visible").
#     Invoked by .githooks/pre-push (opt-in via `git config core.hooksPath .githooks`),
#     or run directly by hand any time.
#
#     Two modes:
#       - Fast (default): detects which layer(s) were actually touched (working tree +
#         commits ahead of origin/main) and runs ONLY the matching build+test. This is
#         the fast day-to-day gate -- keep it under ~2min warm so pre-push stays cheap.
#       - --full: the wide net. Both glintfx CMake configs (GLFW ON and OFF) +
#         TST-L1-STATIC's encapsulation sub-check (tools/check_encapsulation.sh) +
#         gitleaks (TST-L1-SECRETS) if installed. Intended for occasional manual runs
#         (e.g. before tagging a release), not every push -- it is slower by design.
#         Local-first: everything here runs offline except the one-time RmlUi
#         FetchContent fetch on a clean glintfx build directory.
#       Both modes also always run the cheap, no-build gates: the .clang-format
#       lines-changed check and tools/check_doc_line_refs.sh (DOC-HOSTIN follow-up --
#       doc `` `symbol` (`file:line`) `` citations still point at the right symbol).
#
# PT: Gate de pré-CI local (TST-L1-PRECI, TESTES.md). Espelha a fatia rápida do dia a dia
#     do CI ANTES do código ficar visível num remoto (decisão do líder 2026-07-10:
#     enganchado no pre-push, não pre-commit -- a suíte glintfx leva ~2min warm, o que
#     quebraria o fluxo de commits pequenos e frequentes; pre-push é "antes de ficar
#     visível"). Invocado por .githooks/pre-push (opt-in via
#     `git config core.hooksPath .githooks`), ou rodado direto à mão a qualquer momento.
#
#     Dois modos:
#       - Rápido (default): detecta qual(is) camada(s) foram de fato tocadas (working
#         tree + commits à frente de origin/main) e roda SÓ o build+teste
#         correspondente. É o gate rápido do dia a dia -- manter sob ~2min warm pra o
#         pre-push continuar barato.
#       - --full: a rede larga. As duas configs CMake do glintfx (GLFW ON e OFF) + o
#         sub-check de encapsulamento do TST-L1-STATIC (tools/check_encapsulation.sh) +
#         gitleaks (TST-L1-SECRETS) se instalado. Pensado para rodadas manuais
#         ocasionais (ex.: antes de taggear um release), não a cada push -- é mais lento
#         por design. Local-first: tudo aqui roda offline exceto o fetch único do RmlUi
#         via FetchContent num diretório de build limpo do glintfx.
#       Os dois modos também sempre rodam os gates baratos, sem build: o check de
#       .clang-format por linhas alteradas e o tools/check_doc_line_refs.sh (follow-up
#       do DOC-HOSTIN -- citações de doc `` `símbolo` (`arquivo:linha`) `` ainda
#       apontam pro símbolo certo).
#
# Usage / Uso:
#   tools/preci.sh              # fast mode: only the layer(s) actually touched
#   tools/preci.sh --full       # full mode: both glintfx configs + static + secrets
#   tools/preci.sh --help       # this text
#
# Exit status: non-zero on ANY failure (blocks the pre-push hook). Warnings (e.g. a
# missing optional tool in --full mode) print to stderr but do not fail the run.
#
# Requires: git, make (Layer 0), cmake + clang/clang++ + xvfb-run (glintfx). gitleaks is
#           OPTIONAL in --full mode -- its absence is a warning, not a failure (do not
#           block the gate on a missing scanner the user has not installed yet).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

MODE="fast"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --full)
      MODE="full"
      shift
      ;;
    -h|--help)
      grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
      exit 0
      ;;
    *)
      echo "error: unknown argument '$1' (see --help)" >&2
      exit 2
      ;;
  esac
done

# -----------------------------------------------------------------------------
# EN: helpers -- section banner + timer, shared by both modes.
# PT: helpers -- cabeçalho de seção + cronômetro, compartilhados pelos dois modos.
# -----------------------------------------------------------------------------
section() {
  echo ""
  echo "== preci: $1 =="
}

ran_anything=0

# -----------------------------------------------------------------------------
# EN: 1) Layer detection (fast mode only). Compares against origin/main (commits
#     ahead) UNION the working tree (staged + unstaged + untracked, so uncommitted
#     work-in-progress is caught too, not just what is already committed). Falls back
#     to "diff against the empty tree" if origin/main is unreachable (e.g. offline,
#     fresh clone without a fetch yet) -- fast mode then just runs on whatever changed
#     locally rather than hard-failing on a missing remote-tracking ref.
# PT: 1) Detecção de camada (só modo rápido). Compara contra origin/main (commits à
#     frente) UNIÃO a working tree (staged + unstaged + untracked, pra pegar também
#     trabalho não commitado, não só o que já foi commitado). Cai para "diff contra a
#     árvore vazia" se origin/main não for alcançável (ex.: offline, clone novo sem
#     fetch ainda) -- o modo rápido então só roda sobre o que mudou localmente em vez
#     de falhar duro por ref remota ausente.
# -----------------------------------------------------------------------------
detect_touched_files() {
  local base="origin/main"
  if ! git rev-parse --verify --quiet "${base}" >/dev/null; then
    echo "warning: 'origin/main' not found locally (offline/fresh clone?) -- fast mode will diff against the empty tree instead" >&2
    base="$(git hash-object -t tree /dev/null)"
  fi

  {
    git diff --name-only "${base}...HEAD" -- . 2>/dev/null || true
    git diff --name-only HEAD -- . 2>/dev/null || true
    git diff --name-only --cached -- . 2>/dev/null || true
    git ls-files --others --exclude-standard -- . 2>/dev/null || true
  } | sort -u
}

layer0_touched=0
layer1_touched=0

if [[ "${MODE}" == "fast" ]]; then
  mapfile -t touched < <(detect_touched_files)

  for f in "${touched[@]}"; do
    [[ -z "${f}" ]] && continue
    case "${f}" in
      glintfx/*)
        layer1_touched=1
        ;;
      src/*|include/*|Makefile|tests/*)
        # NB: tests/ here is Layer 0's root tests/ (exit42/hello/echo_stdin), distinct
        #     from glintfx/tests/ which is already matched by the glintfx/* case above.
        layer0_touched=1
        ;;
    esac
  done

  if [[ "${layer0_touched}" -eq 0 && "${layer1_touched}" -eq 0 ]]; then
    echo "preci: no Layer 0 (src/include/Makefile/tests) or Layer 1 (glintfx/) changes detected vs origin/main + working tree -- nothing to gate, passing fast."
    exit 0
  fi
fi

# -----------------------------------------------------------------------------
# EN: 2) Layer 0 gate: `make build && make test` (root Makefile, zero-libc suite).
# PT: 2) Gate da Camada 0: `make build && make test` (Makefile raiz, suíte zero-libc).
# -----------------------------------------------------------------------------
run_layer0() {
  section "Layer 0 (C+ASM, zero libc) -- make build && make test"
  make build
  make test
  ran_anything=1
}

# -----------------------------------------------------------------------------
# EN: 3) Layer 1 (glintfx) gate: configure (incremental, reuses the build dir across
#     runs) + build + `ctest` under Xvfb. Fast mode uses ONE config (GLFW=ON, the
#     superset suite); --full uses BOTH configs, one at a time (see run_layer1_full,
#     OOM guardrail from the brief: only one glintfx build in memory at once).
# PT: 3) Gate da Camada 1 (glintfx): configure (incremental, reaproveita o diretório de
#     build entre rodadas) + build + `ctest` sob Xvfb. Modo rápido usa UMA config
#     (GLFW=ON, a suíte superconjunto); --full usa AS DUAS configs, uma de cada vez
#     (ver run_layer1_full, guardrail de OOM do brief: só um build glintfx na memória
#     por vez).
# -----------------------------------------------------------------------------
run_layer1_config() {
  local backend_glfw="$1"
  local build_dir="$2"
  section "Layer 1 (glintfx) -- configure+build+ctest, GLINTFX_BACKEND_GLFW=${backend_glfw} (${build_dir})"
  cmake -S glintfx -B "${build_dir}" \
    -DGLINTFX_BUILD_TESTS=ON \
    -DGLINTFX_BACKEND_GLFW="${backend_glfw}" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
  cmake --build "${build_dir}" -j"$(nproc)"

  # EN: QA-XVFBISO -- isolate WAYLAND_DISPLAY/XDG_RUNTIME_DIR AT THE POINT THIS SCRIPT
  #     LAUNCHES `xvfb-run -a ctest`, instead of trusting whatever the calling shell
  #     already had unset/exported. That trust is exactly what failed on 2026-07-23: this
  #     line used to be a bare `xvfb-run -a ctest ...` with no isolation of its own, so
  #     running `bash tools/preci.sh` from a shell with a live Wayland session inherited
  #     straight through into every one of the ~90 tests, opening real windows on the
  #     líder's desktop (25-30 of them create a window). Each individual test ALSO
  #     isolates itself via glintfx/tests/run_xvfb.cmake (see its header for the full
  #     wl_display_connect(NULL)/XDG_RUNTIME_DIR mechanism) -- that fix alone protects the
  #     test binaries by construction, but this outer `ctest` launch is a SEPARATE entry
  #     point and gets its own isolation too: isolation belongs to whoever executes, not
  #     to whoever calls. One fake XDG_RUNTIME_DIR per run_layer1_config() invocation
  #     (mktemp -d, mode 0700, atomic).
  #     Cleanup is an explicit capture-rc/rm/return, NOT a `trap ... RETURN` -- verified by
  #     hand (harness script, 2026-07-23) that a RETURN trap set inside a function does
  #     NOT reliably fire when `set -e` aborts on that function's own failing command: the
  #     shell unwinds straight past the function scope instead of "returning" from it
  #     normally, so the trap body silently never runs and the fake dir is orphaned on
  #     every ctest FAILURE -- exactly the case that most needs cleanup, since it is also
  #     the case run daily. The explicit form below has no such gap: it is correct on
  #     both the pass and the fail path, and cannot regress if `set -e` semantics differ
  #     across bash versions run by the líder's shell vs. this repo's CI containers.
  # PT: QA-XVFBISO -- isola WAYLAND_DISPLAY/XDG_RUNTIME_DIR NO PONTO em que este script
  #     lança o `xvfb-run -a ctest`, em vez de confiar no que o shell chamador já tivesse
  #     feito unset/export. Foi exatamente essa confiança que falhou em 2026-07-23: esta
  #     linha era um `xvfb-run -a ctest ...` cru, sem isolamento próprio, então rodar
  #     `bash tools/preci.sh` de um shell com sessão Wayland viva vazava direto pros ~90
  #     testes, abrindo janelas reais na área de trabalho do líder (25-30 deles criam
  #     janela). Cada teste individual TAMBÉM se isola via glintfx/tests/run_xvfb.cmake
  #     (ver o cabeçalho dele pro mecanismo completo de
  #     wl_display_connect(NULL)/XDG_RUNTIME_DIR) -- esse fix sozinho já protege os
  #     binários de teste por construção, mas este lançamento externo do `ctest` é um
  #     ponto de entrada SEPARADO e também ganha isolamento próprio: isolamento pertence
  #     a quem executa, não a quem chama. Um XDG_RUNTIME_DIR falso por invocação de
  #     run_layer1_config() (mktemp -d, modo 0700, atômico).
  #     A limpeza é um capture-rc/rm/return explícito, NÃO um `trap ... RETURN` --
  #     verificado à mão (script de harness, 2026-07-23) que um trap RETURN definido
  #     dentro de uma função NÃO dispara de forma confiável quando o `set -e` aborta por
  #     causa do próprio comando que falhou naquela função: o shell desenrola direto,
  #     pulando o "retorno" normal da função, então o corpo do trap silenciosamente nunca
  #     roda e o diretório falso fica órfão em toda FALHA de ctest -- exatamente o caso
  #     que mais precisa de limpeza, já que também é o caso rodado todo dia. A forma
  #     explícita abaixo não tem essa lacuna: está correta tanto no caminho de sucesso
  #     quanto no de falha, e não regride se a semântica do `set -e` variar entre a versão
  #     do bash no shell do líder e a dos containers de CI deste repo.
  local fake_xdg_runtime
  fake_xdg_runtime="$(mktemp -d "${TMPDIR:-/tmp}/glintfx-preci-xvfb-runtime.XXXXXX")"

  local xvfb_rc=0
  env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR="${fake_xdg_runtime}" \
    xvfb-run -a ctest --test-dir "${build_dir}" --output-on-failure || xvfb_rc=$?
  rm -rf "${fake_xdg_runtime}"
  if [[ "${xvfb_rc}" -ne 0 ]]; then
    return "${xvfb_rc}"
  fi

  ran_anything=1
}

run_layer1_fast() {
  run_layer1_config ON "glintfx/build-preci"
}

run_layer1_full() {
  # EN: sequential, not parallel -- one glintfx build in memory at a time (OOM
  #     guardrail; the FetchContent'd RmlUi build is heavy).
  # PT: sequencial, não paralelo -- um build glintfx na memória por vez (guardrail de
  #     OOM; o build do RmlUi via FetchContent é pesado).
  run_layer1_config ON "glintfx/build-preci-glfw-on"
  run_layer1_config OFF "glintfx/build-preci-glfw-off"
}

# -----------------------------------------------------------------------------
# EN: 3.5) TST-L1-FORMAT -- .clang-format gate, lines-changed-only (AUD-L1-QUALITY
#     Onda 2). Runs `clang-format-diff` restricted to the LINES actually touched in
#     each C++ file under review -- NOT whole-file -- because retrofitting
#     glintfx/.clang-format onto this tree leaves an 11% residual of pre-existing
#     formatting drift (measured empirically by the CTO before adopting the config);
#     a whole-file gate would fail on unrelated pre-existing lines and block every
#     push. Runs in BOTH fast and full mode (cheap: only touched files, no build).
#     Excludes glintfx/third_party/ (vendorized) and glintfx/src/gl_loader.{h,c}
#     (generated -- reformatting it would fight the generator). New (untracked)
#     C++ files get a stricter whole-file check via `clang-format --dry-run
#     --Werror`, since there is no "pre-existing" content to be lenient about.
#     Zero touched C++ files -> silent skip (not a failure). Missing
#     clang-format/clang-format-diff in PATH -> warning + skip, same policy as
#     gitleaks below (do not block the gate on a missing optional tool).
# PT: 3.5) Gate TST-L1-FORMAT -- .clang-format, só linhas mudadas (AUD-L1-QUALITY
#     Onda 2). Roda `clang-format-diff` restrito às LINHAS de fato tocadas em cada
#     arquivo C++ em revisão -- NÃO o arquivo inteiro -- porque retrofit do
#     glintfx/.clang-format nesta árvore deixa um residual de 11% de deriva de
#     formatação pré-existente (medido empiricamente pelo CTO antes de adotar a
#     config); um gate de arquivo inteiro falharia em linhas pré-existentes não
#     relacionadas e bloquearia todo push. Roda nos DOIS modos, fast e full (barato:
#     só arquivos tocados, sem build). Exclui glintfx/third_party/ (vendorizado) e
#     glintfx/src/gl_loader.{h,c} (gerado -- reformatar brigaria com o gerador).
#     Arquivos C++ novos (untracked) recebem checagem mais estrita de arquivo
#     inteiro via `clang-format --dry-run --Werror`, já que não há conteúdo
#     "pré-existente" a tolerar. Zero arquivo C++ tocado -> skip silencioso (não
#     falha). clang-format/clang-format-diff ausentes no PATH -> aviso + skip,
#     mesma política do gitleaks abaixo (não bloqueia o gate por ferramenta
#     opcional ausente).
# -----------------------------------------------------------------------------
run_format_gate() {
  if ! command -v clang-format >/dev/null 2>&1 || ! command -v clang-format-diff >/dev/null 2>&1; then
    echo "warning: 'clang-format'/'clang-format-diff' not found in PATH -- skipping TST-L1-FORMAT (install clang-tools, see TOOLING.md)" >&2
    return 0
  fi

  local base="origin/main"
  if ! git rev-parse --verify --quiet "${base}" >/dev/null; then
    base="$(git hash-object -t tree /dev/null)"
  fi

  local -a touched_fmt
  mapfile -t touched_fmt < <(detect_touched_files)

  local -a tracked_cpp=()
  local -a untracked_cpp=()
  local f
  for f in "${touched_fmt[@]}"; do
    [[ -z "${f}" ]] && continue
    case "${f}" in
      glintfx/third_party/*) continue ;;
      glintfx/src/gl_loader.h|glintfx/src/gl_loader.c) continue ;;
      glintfx/*.cpp|glintfx/*.hpp|glintfx/*.h) ;;
      *) continue ;;
    esac
    # EN: skip deleted files -- nothing left on disk to format-check.
    # PT: pula arquivos deletados -- nada em disco pra checar formatação.
    [[ -f "${f}" ]] || continue
    if git ls-files --error-unmatch -- "${f}" >/dev/null 2>&1; then
      tracked_cpp+=("${f}")
    else
      untracked_cpp+=("${f}")
    fi
  done

  if [[ "${#tracked_cpp[@]}" -eq 0 && "${#untracked_cpp[@]}" -eq 0 ]]; then
    return 0
  fi

  section "TST-L1-FORMAT -- .clang-format gate (lines-changed, glintfx/.clang-format)"

  local gate_failed=0

  if [[ "${#tracked_cpp[@]}" -gt 0 ]]; then
    local diff_out
    diff_out="$(git diff -U0 "${base}" -- "${tracked_cpp[@]}" 2>/dev/null | clang-format-diff -p1 -style=file || true)"
    if [[ -n "${diff_out}" ]]; then
      echo "${diff_out}"
      echo "error: clang-format-diff flagged formatting drift in touched lines of: ${tracked_cpp[*]}" >&2
      gate_failed=1
    fi
  fi

  if [[ "${#untracked_cpp[@]}" -gt 0 ]]; then
    local uf
    for uf in "${untracked_cpp[@]}"; do
      if ! clang-format --style=file --dry-run --Werror "${uf}" 2>&1; then
        echo "error: '${uf}' is a new file and fails the whole-file clang-format check." >&2
        gate_failed=1
      fi
    done
  fi

  if [[ "${gate_failed}" -eq 1 ]]; then
    return 1
  fi

  ran_anything=1
}

# -----------------------------------------------------------------------------
# EN: 3.6) DOC-HOSTIN follow-up -- tools/check_doc_line_refs.sh. Cheap (no build, no
#     network), doc-only gate: fails if a `` `SYMBOL` (`file:line`) `` citation in
#     docs/*.md (+ adr/, wiki/ -- see the script's own header comment for the excluded
#     dirs and why) no longer contains the symbol it claims to document. Runs in BOTH
#     fast and full mode, same rationale as the format gate above (cheap enough to
#     never be worth gating behind --full).
# PT: 3.6) Follow-up do DOC-HOSTIN -- tools/check_doc_line_refs.sh. Gate barato (sem
#     build, sem rede), só de doc: falha se uma citação `` `SÍMBOLO` (`arquivo:linha`) ``
#     em docs/*.md (+ adr/, wiki/ -- ver o próprio comentário de cabeçalho do script pros
#     dirs excluídos e o porquê) não contiver mais o símbolo que afirma documentar. Roda
#     nos DOIS modos, rápido e --full, mesmo racional do gate de formatação acima
#     (barato demais pra valer a pena gatear atrás de --full).
# -----------------------------------------------------------------------------
run_doc_line_refs_gate() {
  section "DOC-HOSTIN follow-up -- tools/check_doc_line_refs.sh"
  "${SCRIPT_DIR}/check_doc_line_refs.sh"
  ran_anything=1
}

# -----------------------------------------------------------------------------
# EN: 4) --full extras: TST-L1-STATIC's encapsulation sub-check + TST-L1-SECRETS
#     (gitleaks). Both are cheap and always-green per TESTES.md's adoption order, so
#     they only run in --full (fast mode stays focused on build+test of the touched
#     layer). gitleaks is optional: skip with a warning if not installed, do NOT fail
#     the gate on a missing scanner.
# PT: 4) Extras do --full: sub-check de encapsulamento do TST-L1-STATIC + TST-L1-SECRETS
#     (gitleaks). Ambos são baratos e sempre-verdes pela ordem de adoção do TESTES.md,
#     então só rodam no --full (o modo rápido fica focado em build+teste da camada
#     tocada). gitleaks é opcional: pula com aviso se não instalado, NÃO falha o gate
#     por um scanner ausente.
# -----------------------------------------------------------------------------
run_full_extras() {
  section "TST-L1-STATIC (sub-check) -- tools/check_encapsulation.sh"
  "${SCRIPT_DIR}/check_encapsulation.sh"
  ran_anything=1

  section "TST-L1-SECRETS -- gitleaks"
  if command -v gitleaks >/dev/null 2>&1; then
    gitleaks detect --source "${REPO_ROOT}" --no-banner
    ran_anything=1
  else
    echo "warning: 'gitleaks' not found in PATH -- skipping TST-L1-SECRETS (install it to enable this check, see TOOLING.md)" >&2
  fi
}

# -----------------------------------------------------------------------------
# EN: 5) Dispatch.
# PT: 5) Despacho.
# -----------------------------------------------------------------------------
if [[ "${MODE}" == "fast" ]]; then
  [[ "${layer0_touched}" -eq 1 ]] && run_layer0
  [[ "${layer1_touched}" -eq 1 ]] && run_layer1_fast
  run_format_gate
  run_doc_line_refs_gate
else
  run_layer0
  run_layer1_full
  run_full_extras
  run_format_gate
  run_doc_line_refs_gate
fi

if [[ "${ran_anything}" -eq 0 ]]; then
  echo "preci: nothing ran (unexpected -- please report)." >&2
  exit 1
fi

echo ""
echo "== preci: OK (mode=${MODE}) =="
