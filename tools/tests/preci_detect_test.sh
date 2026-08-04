#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: SEED-GATE-NAO-GUARDA-SI-MESMO self-test (TODO.md, W27). Pure-bash, zero git, zero
#     build: sources tools/preci.sh (safe -- see that file's `main()`/BASH_SOURCE[0]==$0
#     guard, added specifically to make this sourcing safe) and asserts
#     classify_touched_files() against synthetic touched-file lists standing in for the
#     output a real `git diff --name-only` would produce. Runs in seconds.
#
#     What this covers (and does NOT cover): this is a unit test of the CLASSIFICATION
#     logic only -- it does not exercise detect_touched_files()'s actual git plumbing,
#     and it does not run any of the run_layer*/run_full_extras gates. The end-to-end
#     proof that `bash tools/preci.sh` on a real scratch branch reacts correctly to a
#     tools/ctest_guarded.sh touch (and stays fast on a tools/check_doc_line_refs.sh
#     touch) is the aceite scenarios run by hand against the real repo -- this script is
#     the fast regression net that runs on every CI pass instead, so the allowlist in
#     classify_touched_files() cannot silently rot back to "tools/* matches nothing"
#     the way it did before this seed was filed.
#
#     The one deliberate irony this repo can produce: this file itself lives under
#     tools/tests/, which is NOT in the classify_touched_files() allowlist (editing a
#     TEST does not need to force a full two-layer suite run) -- so committing changes
#     to this test does not, by itself, trigger the gate it verifies. That is
#     intentional and matches the seed's own scoping call ("NAO entra: alargar para
#     tools/ inteiro"); it is the SIBLING scripts (preci.sh, ctest_guarded.sh,
#     mutation_sandbox.sh, check_encapsulation.sh) that must, and do.
#
# PT: self-test do SEED-GATE-NAO-GUARDA-SI-MESMO (TODO.md, W27). Puro-bash, zero git,
#     zero build: sourceia o tools/preci.sh (seguro -- ver o `main()`/guard
#     BASH_SOURCE[0]==$0 daquele arquivo, adicionado especificamente pra tornar este
#     sourcing seguro) e afirma classify_touched_files() contra listas sintéticas de
#     arquivo-tocado que representam o que um `git diff --name-only` de verdade
#     produziria. Roda em segundos.
#
#     O que isto cobre (e o que NÃO cobre): isto é um teste unitário só da lógica de
#     CLASSIFICAÇÃO -- não exercita a mecânica git de verdade do detect_touched_files(),
#     nem roda nenhum dos gates run_layer*/run_full_extras. A prova fim-a-fim de que
#     `bash tools/preci.sh` numa branch scratch de verdade reage certo a um toque em
#     tools/ctest_guarded.sh (e continua rápido num toque em
#     tools/check_doc_line_refs.sh) são os cenários de aceite rodados à mão contra o
#     repo real -- este script é a rede de regressão rápida que roda em toda passada de
#     CI no lugar disso, então a allowlist em classify_touched_files() não pode
#     apodrecer de volta pra "tools/* não casa com nada" do jeito que apodreceu antes
#     desta semente ser registrada.
#
#     A única ironia deliberada que este repo consegue produzir: este próprio arquivo
#     mora sob tools/tests/, que NÃO está na allowlist do classify_touched_files()
#     (editar um TESTE não precisa forçar uma rodada de suíte completa nas duas
#     camadas) -- então commitar mudanças neste teste não, por si só, dispara o gate
#     que ele verifica. Isso é intencional e casa com o próprio recorte de escopo da
#     semente ("NAO entra: alargar para tools/ inteiro"); são os scripts IRMÃOS
#     (preci.sh, ctest_guarded.sh, mutation_sandbox.sh, check_encapsulation.sh) que
#     precisam, e conseguem.
#
# Usage / Uso: tools/tests/preci_detect_test.sh   (exit 0 = all assertions passed)
#
# EN: file-wide shellcheck disables -- SC1091 (can't statically follow the dynamic
#     `source "${REPO_ROOT}/tools/preci.sh"` path below) and SC2154
#     (layer0_touched/layer1_touched "referenced but not assigned" -- shellcheck does
#     not see that classify_touched_files(), defined in the sourced preci.sh, assigns
#     both globals before assert_layers() ever reads them). Both are known false
#     positives inherent to sourcing a sibling script for testing.
# PT: disables de shellcheck pro arquivo inteiro -- SC1091 (não dá pra seguir
#     estaticamente o caminho dinâmico do `source "${REPO_ROOT}/tools/preci.sh"`
#     abaixo) e SC2154 (layer0_touched/layer1_touched "referenciada mas não
#     atribuída" -- o shellcheck não vê que classify_touched_files(), definida no
#     preci.sh sourceado, atribui os dois globais antes de assert_layers() lê-los).
#     Os dois são falsos positivos conhecidos, inerentes a sourcear um script irmão
#     pra teste.
# shellcheck disable=SC1091,SC2154
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

source "${REPO_ROOT}/tools/preci.sh"

pass=0
fail=0

# -----------------------------------------------------------------------------
# EN: assert_layers DESC EXPECT_LAYER0 EXPECT_LAYER1 FILE... -- calls
#     classify_touched_files() with FILE... (a synthetic diff's file list) and checks
#     the resulting layer0_touched/layer1_touched globals against the expectation.
# PT: assert_layers DESC EXPECT_LAYER0 EXPECT_LAYER1 FILE... -- chama
#     classify_touched_files() com FILE... (lista de arquivo de um diff sintético) e
#     confere os globais layer0_touched/layer1_touched resultantes contra a expectativa.
# -----------------------------------------------------------------------------
assert_layers() {
  local desc="$1"
  local expect_l0="$2"
  local expect_l1="$3"
  shift 3

  classify_touched_files "$@"

  if [[ "${layer0_touched}" -eq "${expect_l0}" && "${layer1_touched}" -eq "${expect_l1}" ]]; then
    echo "ok - ${desc}"
    pass=$((pass + 1))
  else
    echo "not ok - ${desc} (want layer0=${expect_l0} layer1=${expect_l1}, got layer0=${layer0_touched} layer1=${layer1_touched})" >&2
    fail=$((fail + 1))
  fi
}

# -----------------------------------------------------------------------------
# EN: the four allowlisted gate scripts -- each alone must force BOTH layers. This is
#     the direct regression test for SEED-GATE-NAO-GUARDA-SI-MESMO: before the
#     allowlist existed, every one of these four asserted 0/0 (nothing to gate).
# PT: os quatro scripts de gate na allowlist -- cada um sozinho tem que forçar AS DUAS
#     camadas. Este é o teste de regressão direto do SEED-GATE-NAO-GUARDA-SI-MESMO:
#     antes da allowlist existir, cada um destes quatro afirmava 0/0 (nada a gatear).
# -----------------------------------------------------------------------------
assert_layers "tools/preci.sh alone forces both layers"            1 1 "tools/preci.sh"
assert_layers "tools/ctest_guarded.sh alone forces both layers"    1 1 "tools/ctest_guarded.sh"
assert_layers "tools/mutation_sandbox.sh alone forces both layers" 1 1 "tools/mutation_sandbox.sh"
assert_layers "tools/check_encapsulation.sh alone forces both"     1 1 "tools/check_encapsulation.sh"

# -----------------------------------------------------------------------------
# EN: everything else under tools/ stays OUT of the allowlist on purpose (the seed's
#     scoping call) -- must still pass fast (0/0), same as today.
# PT: todo o resto sob tools/ fica DE FORA da allowlist de propósito (o recorte de
#     escopo da semente) -- ainda tem que passar rápido (0/0), como hoje.
# -----------------------------------------------------------------------------
assert_layers "tools/check_doc_line_refs.sh (not allowlisted) stays fast" 0 0 "tools/check_doc_line_refs.sh"
assert_layers "tools/coverage_report.sh (not allowlisted) stays fast"     0 0 "tools/coverage_report.sh"
assert_layers "tools/gen_glloader.py (not allowlisted) stays fast"        0 0 "tools/gen_glloader.py"

# -----------------------------------------------------------------------------
# EN: allowlist match must be EXACT (repo-root-relative literal path), not a prefix or
#     suffix match -- a lookalike path must NOT force the gate.
# PT: o match da allowlist tem que ser EXATO (caminho literal relativo à raiz do repo),
#     não um match de prefixo ou sufixo -- um caminho parecido NÃO pode forçar o gate.
# -----------------------------------------------------------------------------
assert_layers "tools/preci.sh.bak (suffix lookalike) does not match"    0 0 "tools/preci.sh.bak"
assert_layers "nested/tools/preci.sh (prefix lookalike) does not match" 0 0 "nested/tools/preci.sh"

# -----------------------------------------------------------------------------
# EN: pre-existing arms (unrelated to this seed) still work as before -- regression net
#     for the refactor that pulled the for-loop out into this function.
# PT: os braços preexistentes (não relacionados a esta semente) continuam funcionando
#     como antes -- rede de regressão pro refactor que tirou o for-loop pra esta função.
# -----------------------------------------------------------------------------
assert_layers "glintfx/*.cpp touches only Layer 1"       0 1 "glintfx/src/foo.cpp"
assert_layers "glintfx/tests/* touches only Layer 1"     0 1 "glintfx/tests/bar_test.cpp"
assert_layers "src/*.c touches only Layer 0"             1 0 "src/foo.c"
assert_layers "include/*.h touches only Layer 0"         1 0 "include/foo.h"
assert_layers "root Makefile touches only Layer 0"       1 0 "Makefile"
assert_layers "root tests/* touches only Layer 0"        1 0 "tests/exit42/main.c"
assert_layers "empty file list touches nothing"          0 0
assert_layers "blank line in the list is ignored"        0 0 ""

# -----------------------------------------------------------------------------
# EN: mixed lists -- a synthetic multi-file diff, closer to what a real commit looks
#     like. Only the allowlisted/glintfx/src/include/tests entries should count.
# PT: listas mistas -- um diff sintético multi-arquivo, mais perto do que um commit de
#     verdade parece. Só as entradas da allowlist/glintfx/src/include/tests devem contar.
# -----------------------------------------------------------------------------
assert_layers "doc-only tool + glintfx file -> Layer 1 only, doc tool adds nothing" \
  0 1 "tools/check_doc_line_refs.sh" "glintfx/src/foo.cpp"
assert_layers "gate script + glintfx file -> both layers (redundant with allowlist alone)" \
  1 1 "tools/ctest_guarded.sh" "glintfx/src/foo.cpp"
assert_layers "gate script + Layer 0 file -> both layers" \
  1 1 "tools/mutation_sandbox.sh" "src/foo.c"

echo ""
echo "== preci_detect_test: ${pass} passed, ${fail} failed =="

if [[ "${fail}" -gt 0 ]]; then
  exit 1
fi
