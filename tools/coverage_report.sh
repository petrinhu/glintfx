#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Local-first code coverage report for glintfx (L1.18-COVCI, TST-L1-COV).
#     Self-hosted per the líder's 2026-07-04 decision -- llvm-cov (Clang source-based
#     coverage), NOT Codecov or any third-party uploader; zero data leaves this machine
#     (or the CI runner, when the CI `coverage` job calls this SAME script -- see
#     .github/workflows/ci.yml and .forgejo/workflows/ci.yml). Local-first per the
#     "gêmeo local que o CI chama" preference (CLAUDE.md global,
#     feedback_execucao_local_cross_platform): this script IS the mechanism, the CI job
#     is a thin wrapper around it.
#
#     Pipeline: configure+build with -DGLINTFX_COVERAGE=ON (Clang-only, see CMakeLists.txt)
#     -> ctest with LLVM_PROFILE_FILE writing one *.profraw per (binary, pid) ->
#     llvm-profdata merge -> llvm-cov show (HTML report) + llvm-cov export -summary-only
#     (machine-readable %) -> optionally write the shields.io endpoint JSON badge.
#
#     Scope boundary: -ignore-filename-regex excludes _deps/ (FetchContent RmlUi),
#     third_party/ (vendored stb_image/Khronos headers), tests/ and demos/ (test/demo
#     code itself, not the library under test) -- mirrors the CMakeLists.txt
#     GLINTFX_COVERAGE comment's scope boundary on the instrumentation side. Only
#     glintfx's own src/ + public headers are reported on.
#
# PT: Relatório de cobertura de código local-first para glintfx (L1.18-COVCI, TST-L1-COV).
#     Self-hosted conforme decisão do líder de 2026-07-04 -- llvm-cov (cobertura
#     source-based do Clang), NÃO Codecov nem nenhum uploader de terceiro; zero dado sai
#     desta máquina (ou do runner de CI, quando o job `coverage` do CI chama este MESMO
#     script -- ver .github/workflows/ci.yml e .forgejo/workflows/ci.yml). Local-first
#     conforme a preferência "gêmeo local que o CI chama" (CLAUDE.md global,
#     feedback_execucao_local_cross_platform): este script É o mecanismo, o job de CI é
#     um wrapper fino em torno dele.
#
#     Pipeline: configure+build com -DGLINTFX_COVERAGE=ON (só Clang, ver CMakeLists.txt)
#     -> ctest com LLVM_PROFILE_FILE escrevendo um *.profraw por (binário, pid) ->
#     llvm-profdata merge -> llvm-cov show (relatório HTML) + llvm-cov export
#     -summary-only (% legível por máquina) -> opcionalmente escreve o JSON do badge
#     endpoint shields.io.
#
#     Fronteira de escopo: -ignore-filename-regex exclui _deps/ (RmlUi via FetchContent),
#     third_party/ (stb_image/headers Khronos vendorizados), tests/ e demos/ (código de
#     teste/demo em si, não a biblioteca sob teste) -- espelha a fronteira de escopo do
#     comentário GLINTFX_COVERAGE do CMakeLists.txt do lado da instrumentação. Só o
#     src/ + headers públicos da própria glintfx entram no relatório.
#
# Usage / Uso:
#   tools/coverage_report.sh [--build-dir DIR] [--update-badge] [--floor PCT]
#
#   --build-dir DIR    Build directory (default: glintfx/build-cov).
#   --update-badge     Write glintfx/coverage-badge.json (shields.io endpoint format).
#                       CI must NOT pass this flag for the commit-back step -- the CI
#                       job only validates/reports (see TODO.md L1.18-COVCI); only a
#                       human/local run updates the committed badge JSON.
#   --floor PCT        If set, exit 1 when total line coverage % < PCT (CI gate mode).
#
# Requires: cmake, ninja/make, clang, clang++, llvm-profdata, llvm-cov (or a versioned
#           variant, e.g. llvm-cov-18 -- resolved automatically, see resolve_llvm_tool()),
#           python3 (stdlib json only), xvfb (ctest runs the suite under Xvfb).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
GLINTFX_DIR="${REPO_ROOT}/glintfx"

BUILD_DIR="${GLINTFX_DIR}/build-cov"
UPDATE_BADGE=0
FLOOR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$(cd "$(dirname "$2")" 2>/dev/null && pwd)/$(basename "$2")" 2>/dev/null || BUILD_DIR="$2"
      shift 2
      ;;
    --update-badge)
      UPDATE_BADGE=1
      shift
      ;;
    --floor)
      FLOOR="$2"
      shift 2
      ;;
    -h|--help)
      grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
      exit 0
      ;;
    *)
      echo "error: unknown argument '$1'" >&2
      exit 2
      ;;
  esac
done

# EN: resolve_llvm_tool <name> -- prints the path of <name>, or the highest-versioned
#     <name>-NN variant found in PATH (Ubuntu/Debian ship llvm-cov-18 etc. without
#     always symlinking the unversioned name). Hard-fails if neither is found.
# PT: resolve_llvm_tool <nome> -- imprime o caminho de <nome>, ou a variante <nome>-NN
#     de maior versão encontrada no PATH (Ubuntu/Debian empacotam llvm-cov-18 etc. sem
#     sempre symlinkar o nome não-versionado). Falha duro se nenhum for encontrado.
resolve_llvm_tool() {
  local name="$1"
  if command -v "${name}" >/dev/null 2>&1; then
    command -v "${name}"
    return 0
  fi
  local candidate
  candidate="$(compgen -c "${name}-" 2>/dev/null | sort -V | tail -n1 || true)"
  if [[ -n "${candidate}" ]] && command -v "${candidate}" >/dev/null 2>&1; then
    command -v "${candidate}"
    return 0
  fi
  echo "error: '${name}' (nor a versioned '${name}-NN' variant) found in PATH -- install LLVM/Clang tools" >&2
  exit 1
}

LLVM_PROFDATA="$(resolve_llvm_tool llvm-profdata)"
LLVM_COV="$(resolve_llvm_tool llvm-cov)"

echo "== glintfx coverage report (L1.18-COVCI) =="
echo "   build dir:    ${BUILD_DIR}"
echo "   llvm-profdata: ${LLVM_PROFDATA}"
echo "   llvm-cov:      ${LLVM_COV}"

# ---------------------------------------------------------------------------
# EN: 1) Configure + build. GLINTFX_BUILD_DEMOS=OFF is mandatory (see CMakeLists.txt
#     GLINTFX_COVERAGE comment -- an uninstrumented demo linking the instrumented
#     library fails to link). Single config (GLFW=ON): runs the full test suite
#     (superset of the embed-only suite), matching the CI coverage job's scope.
# PT: 1) Configure + build. GLINTFX_BUILD_DEMOS=OFF é obrigatório (ver comentário
#     GLINTFX_COVERAGE do CMakeLists.txt -- um demo não-instrumentado linkando a
#     biblioteca instrumentada falha o link). Config única (GLFW=ON): roda a suíte
#     completa (superconjunto da suíte embed-only), no mesmo escopo do job de CI.
# ---------------------------------------------------------------------------
cmake -S "${GLINTFX_DIR}" -B "${BUILD_DIR}" \
  -DGLINTFX_BUILD_TESTS=ON \
  -DGLINTFX_BUILD_DEMOS=OFF \
  -DGLINTFX_BACKEND_GLFW=ON \
  -DGLINTFX_COVERAGE=ON \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build "${BUILD_DIR}" -j"$(nproc)"

# ---------------------------------------------------------------------------
# EN: 2) Run the suite with LLVM_PROFILE_FILE set. %m = binary signature (dedups by
#     the actual executable, not just its name), %p = pid -- together they guarantee a
#     unique .profraw per (binary, invocation), safe even if ctest ever parallelizes.
#     Each individual test ALSO wraps itself in its own nested `xvfb-run -a`
#     (tests/run_xvfb.cmake); the outer `xvfb-run -a ctest ...` here matches every
#     other ctest invocation in this repo's CI workflows (defense-in-depth DISPLAY
#     hermeticity -- see run_xvfb.cmake's header comment). Both layers inherit
#     LLVM_PROFILE_FILE like any other exported env var; no per-test wiring needed.
# PT: 2) Roda a suíte com LLVM_PROFILE_FILE definido. %m = assinatura do binário
#     (dedup pelo executável real, não só o nome), %p = pid -- juntos garantem um
#     .profraw único por (binário, invocação), seguro mesmo se o ctest algum dia
#     paralelizar. Cada teste individual TAMBÉM se envolve no próprio `xvfb-run -a`
#     aninhado (tests/run_xvfb.cmake); o `xvfb-run -a ctest ...` externo aqui casa com
#     toda outra invocação de ctest nos workflows de CI deste repo (hermeticidade de
#     DISPLAY cinto-e-suspensório -- ver o comentário de cabeçalho de run_xvfb.cmake).
#     As duas camadas herdam LLVM_PROFILE_FILE como qualquer outra env var exportada;
#     nenhuma fiação por-teste necessária.
# ---------------------------------------------------------------------------
PROFILE_DIR="${BUILD_DIR}/coverage-profiles"
rm -rf "${PROFILE_DIR}"
mkdir -p "${PROFILE_DIR}"
export LLVM_PROFILE_FILE="${PROFILE_DIR}/%m-%p.profraw"

# EN: QA-XVFBISO -- isolate WAYLAND_DISPLAY/XDG_RUNTIME_DIR AT THE POINT this script
#     launches `xvfb-run -a ctest`, instead of trusting whatever the calling shell already
#     had unset/exported (that trust is what let real windows open on the líder's live
#     Wayland desktop on 2026-07-23, via this exact same unprotected pattern in
#     tools/preci.sh -- see that script's run_layer1_config() for the full incident
#     story). Each individual test ALSO isolates itself via
#     glintfx/tests/run_xvfb.cmake (its header has the full wl_display_connect(NULL)/
#     XDG_RUNTIME_DIR mechanism) -- that protects the test binaries by construction, but
#     this outer `ctest` launch is a SEPARATE entry point and gets its own isolation too:
#     isolation belongs to whoever executes, not to whoever calls. One fake
#     XDG_RUNTIME_DIR for this whole script run (mktemp -d, mode 0700, atomic), removed
#     via the EXIT trap regardless of how the script ends. Verified by hand (2026-07-23)
#     that this specific combination -- trap set at TOP-LEVEL script scope (not inside a
#     function) on EXIT (not RETURN) -- DOES fire correctly even when the wrapped `ctest`
#     command fails under `set -e`: this is the mechanism tools/preci.sh's
#     run_layer1_config() tried FIRST (a `trap ... RETURN` set inside that function) and
#     had to abandon after proving it silently skips the cleanup on the failure path (see
#     that function's comment for the harness that caught it) -- EXIT-at-top-level does
#     not share that gap. Unrelated to PROFILE_DIR above: this directory only ever holds
#     an absent-by-design "wayland-0" socket, nothing coverage-related lives in it.
# PT: QA-XVFBISO -- isola WAYLAND_DISPLAY/XDG_RUNTIME_DIR NO PONTO em que este script
#     lança o `xvfb-run -a ctest`, em vez de confiar no que o shell chamador já tivesse
#     feito unset/export (foi exatamente essa confiança que deixou janelas reais abrirem
#     na área de trabalho Wayland ao vivo do líder em 2026-07-23, através deste mesmo
#     padrão sem proteção no tools/preci.sh -- ver o run_layer1_config() daquele script
#     pra história completa do incidente). Cada teste individual TAMBÉM se isola via
#     glintfx/tests/run_xvfb.cmake (o cabeçalho dele tem o mecanismo completo de
#     wl_display_connect(NULL)/XDG_RUNTIME_DIR) -- isso protege os binários de teste por
#     construção, mas este lançamento externo do `ctest` é um ponto de entrada SEPARADO e
#     também ganha isolamento próprio: isolamento pertence a quem executa, não a quem
#     chama. Um XDG_RUNTIME_DIR falso pra esta rodada inteira do script (mktemp -d, modo
#     0700, atômico), removido pelo trap EXIT independente de como o script termina.
#     Verificado à mão (2026-07-23) que esta combinação especifica -- trap definido no
#     escopo de TOPO do script (não dentro de uma função) sobre EXIT (não RETURN) --
#     dispara corretamente mesmo quando o `ctest` envolvido falha sob `set -e`: foi essa a
#     mecânica que o run_layer1_config() do tools/preci.sh tentou PRIMEIRO (um
#     `trap ... RETURN` definido dentro daquela função) e teve que abandonar depois de
#     provar que ela silenciosamente pula a limpeza no caminho de falha (ver o comentário
#     daquela função pro harness que pegou isso) -- EXIT-no-topo não compartilha essa
#     lacuna. Sem relação com PROFILE_DIR acima: este diretório só guarda um socket
#     "wayland-0" ausente de propósito, nada de cobertura vive nele.
GLINTFX_XVFB_FAKE_XDG_RUNTIME="$(mktemp -d "${TMPDIR:-/tmp}/glintfx-cov-xvfb-runtime.XXXXXX")"
trap 'rm -rf "${GLINTFX_XVFB_FAKE_XDG_RUNTIME}"' EXIT

env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR="${GLINTFX_XVFB_FAKE_XDG_RUNTIME}" \
  xvfb-run -a ctest --test-dir "${BUILD_DIR}" --output-on-failure

shopt -s nullglob
profraws=("${PROFILE_DIR}"/*.profraw)
shopt -u nullglob
if [[ ${#profraws[@]} -eq 0 ]]; then
  echo "error: no .profraw files written to ${PROFILE_DIR} -- did the test suite actually run instrumented binaries?" >&2
  exit 1
fi
echo "   collected ${#profraws[@]} .profraw file(s)"

# ---------------------------------------------------------------------------
# EN: 3) Merge raw profiles into one indexed .profdata.
# PT: 3) Mescla os profiles crus num único .profdata indexado.
# ---------------------------------------------------------------------------
PROFDATA="${BUILD_DIR}/coverage.profdata"
"${LLVM_PROFDATA}" merge -sparse "${profraws[@]}" -o "${PROFDATA}"

# ---------------------------------------------------------------------------
# EN: 4) Discover every test binary via `ctest --show-only=json-v1` (the -DEXE=<path>
#     argument to run_xvfb.cmake, per tests/CMakeLists.txt's add_test() COMMAND shape)
#     instead of hardcoding a list -- stays correct as tests are added/removed.
#     Deduplicated (a binary may back more than one ctest NAME in theory).
# PT: 4) Descobre todo binário de teste via `ctest --show-only=json-v1` (o argumento
#     -DEXE=<path> pra run_xvfb.cmake, conforme o formato do COMMAND de add_test() em
#     tests/CMakeLists.txt) em vez de fixar uma lista -- continua correto conforme
#     testes são adicionados/removidos. Deduplicado (um binário pode em tese respaldar
#     mais de um NAME de ctest).
# ---------------------------------------------------------------------------
mapfile -t test_binaries < <(
  ctest --test-dir "${BUILD_DIR}" --show-only=json-v1 | python3 -c '
import json, sys
data = json.load(sys.stdin)
seen = set()
for test in data.get("tests", []):
    for arg in test.get("command", []):
        if arg.startswith("-DEXE="):
            path = arg[len("-DEXE="):]
            if path not in seen:
                seen.add(path)
                print(path)
'
)

if [[ ${#test_binaries[@]} -eq 0 ]]; then
  echo "error: could not discover any test binary from 'ctest --show-only=json-v1' (-DEXE= argument not found)" >&2
  exit 1
fi
echo "   discovered ${#test_binaries[@]} test binary/binaries for llvm-cov -object"

llvm_cov_objects=("${test_binaries[0]}")
for bin in "${test_binaries[@]:1}"; do
  llvm_cov_objects+=(-object "${bin}")
done

IGNORE_REGEX='(_deps|third_party|tests|demos)/'

# ---------------------------------------------------------------------------
# EN: 5) HTML report (human-facing artifact, uploaded by CI; browsable locally).
# PT: 5) Relatório HTML (artefato pra humano, enviado como artifact pelo CI;
#     navegável localmente).
# ---------------------------------------------------------------------------
HTML_DIR="${BUILD_DIR}/coverage-html"
"${LLVM_COV}" show "${llvm_cov_objects[@]}" \
  -instr-profile="${PROFDATA}" \
  -ignore-filename-regex="${IGNORE_REGEX}" \
  -format=html \
  -output-dir="${HTML_DIR}"
echo "   HTML report: ${HTML_DIR}/index.html"

# ---------------------------------------------------------------------------
# EN: 6) Machine-readable summary -> extract total line coverage %.
# PT: 6) Resumo legível por máquina -> extrai a % total de cobertura de linha.
# ---------------------------------------------------------------------------
SUMMARY_JSON="${BUILD_DIR}/coverage-summary.json"
"${LLVM_COV}" export "${llvm_cov_objects[@]}" \
  -instr-profile="${PROFDATA}" \
  -ignore-filename-regex="${IGNORE_REGEX}" \
  -summary-only > "${SUMMARY_JSON}"

LINE_PCT="$(python3 -c '
import json, sys
with open(sys.argv[1]) as f:
    data = json.load(f)
lines_totals = data["data"][0]["totals"]["lines"]
pct = lines_totals["percent"]
print(f"{pct:.1f}")
' "${SUMMARY_JSON}")"

echo "== line coverage: ${LINE_PCT}% (glintfx src/ + public headers only) =="

# ---------------------------------------------------------------------------
# EN: 7) --floor PCT -- CI gate mode: fail if below the fixed floor.
# PT: 7) --floor PCT -- modo gate de CI: falha se abaixo do piso fixado.
# ---------------------------------------------------------------------------
if [[ -n "${FLOOR}" ]]; then
  below="$(python3 -c "print(1 if float(\"${LINE_PCT}\") < float(\"${FLOOR}\") else 0)")"
  if [[ "${below}" == "1" ]]; then
    echo "FAIL: line coverage ${LINE_PCT}% is below the floor ${FLOOR}% (L1.18-COVCI gate)" >&2
    exit 1
  fi
  echo "OK: line coverage ${LINE_PCT}% >= floor ${FLOOR}%"
fi

# ---------------------------------------------------------------------------
# EN: 8) --update-badge -- write the shields.io endpoint JSON. Colour ramp:
#     <50 red, <70 yellow, <85 yellowgreen, else brightgreen. This is a LOCAL/human
#     step only -- the CI `coverage` job validates the % against --floor but does NOT
#     commit/push this file back (see TODO.md L1.18-COVCI + this script's own header).
# PT: 8) --update-badge -- escreve o JSON endpoint do shields.io. Rampa de cor:
#     <50 red, <70 yellow, <85 yellowgreen, senão brightgreen. Este é um passo
#     LOCAL/humano apenas -- o job `coverage` do CI valida a % contra --floor mas NÃO
#     commita/pusha este arquivo de volta (ver TODO.md L1.18-COVCI + o cabeçalho deste
#     script).
# ---------------------------------------------------------------------------
if [[ "${UPDATE_BADGE}" -eq 1 ]]; then
  BADGE_JSON="${GLINTFX_DIR}/coverage-badge.json"
  python3 -c '
import json, sys

pct = float(sys.argv[1])
if pct < 50:
    color = "red"
elif pct < 70:
    color = "yellow"
elif pct < 85:
    color = "yellowgreen"
else:
    color = "brightgreen"

badge = {
    "schemaVersion": 1,
    "label": "coverage",
    "message": f"{pct:.1f}%",
    "color": color,
}
with open(sys.argv[2], "w") as f:
    json.dump(badge, f, indent=2)
    f.write("\n")
' "${LINE_PCT}" "${BADGE_JSON}"
  echo "   badge JSON updated: ${BADGE_JSON}"
fi

echo "== done =="
