#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: REL-TAGCONSIST-GATE self-test (TODO.md). Pure-bash, zero network, zero build:
#     sources .githooks/pre-push (safe -- see that file's main()/BASH_SOURCE[0]==$0
#     guard, the same shape tools/tests/pre_push_gate_test.sh already relies on for
#     CI-TAGFAST) and exercises classify_push_refs()/check_tag_release_consistency()
#     directly, against a THROWAWAY git repo built under /var/tmp (never /tmp -- this
#     machine's /tmp is tmpfs=RAM, house rule) with a handful of hand-crafted commits
#     that vary glintfx/CMakeLists.txt's project(VERSION ...) and CHANGELOG.md's
#     release-section heading independently of each other.
#
#     Why a real throwaway git repo instead of mocking `git show`: the two functions
#     under test (extract_cmake_version/changelog_has_release_section, both defined in
#     .githooks/pre-push) exist SPECIFICALLY to read a file's content AT A GIVEN
#     COMMIT rather than off the working tree -- the one thing worth proving is that
#     `git show <sha>:<path>` really does pin to that commit's content, which a mock
#     cannot demonstrate. tag_commit_is_verified() (the network-touching function) is
#     NOT exercised here -- that stays pre_push_gate_test.sh's job; this script only
#     drives check_tag_release_consistency(), which never calls it.
#
#     The 6 scenarios below are the ones REL-TAGCONSIST-GATE's own brief asked for,
#     verbatim: (a) consistent tag -> passes; (b) version mismatch -> fails;
#     (c) CHANGELOG missing the release section -> fails; (d) branch-only push (no
#     tag ref at all) -> does not interfere; (e) mixed branch+tag push -> the tag is
#     still checked (both a passing and a failing tag are exercised mixed with a
#     branch ref, so passing does not mean "the branch masked the check"); (f) an
#     unreadable/absent project(VERSION ...) -> fails (fail-closed).
#
# PT: self-test do REL-TAGCONSIST-GATE (TODO.md). Puro-bash, zero rede, zero build:
#     sourceia o .githooks/pre-push (seguro -- ver o guard main()/BASH_SOURCE[0]==$0
#     daquele arquivo, a mesma forma que o tools/tests/pre_push_gate_test.sh já usa
#     pro CI-TAGFAST) e exercita classify_push_refs()/check_tag_release_consistency()
#     direto, contra um repo git DESCARTÁVEL construído sob /var/tmp (nunca /tmp --
#     o /tmp desta máquina é tmpfs=RAM, regra da casa) com um punhado de commits
#     feitos à mão que variam o project(VERSION ...) do glintfx/CMakeLists.txt e o
#     heading de seção de release do CHANGELOG.md independentemente um do outro.
#
#     Por que um repo git descartável de verdade em vez de mockar `git show`: as duas
#     funções sob teste (extract_cmake_version/changelog_has_release_section, as duas
#     definidas em .githooks/pre-push) existem ESPECIFICAMENTE pra ler o conteúdo de
#     um arquivo NUM COMMIT DADO em vez de na working tree -- a única coisa que vale a
#     pena provar é que `git show <sha>:<path>` de fato pina no conteúdo daquele
#     commit, o que um mock não consegue demonstrar. O tag_commit_is_verified()
#     (a função que toca rede) NÃO é exercitado aqui -- isso continua sendo trabalho
#     do pre_push_gate_test.sh; este script só aciona o
#     check_tag_release_consistency(), que nunca o chama.
#
#     Os 6 cenários abaixo são exatamente os que o próprio brief do REL-TAGCONSIST-GATE
#     pediu: (a) tag consistente -> passa; (b) versão divergente -> reprova;
#     (c) CHANGELOG sem a seção de release -> reprova; (d) push só de branch (nenhuma
#     ref de tag) -> não interfere; (e) push misto branch+tag -> a tag continua sendo
#     checada (uma tag que passa E uma que reprova são exercitadas misturadas com uma
#     ref de branch, então passar não significa "a branch mascarou a checagem");
#     (f) um project(VERSION ...) ilegível/ausente -> reprova (fail-closed).
#
# Usage / Uso: tools/tests/tag_release_consistency_test.sh   (exit 0 = tudo passou)
#
# EN: file-wide shellcheck disable -- SC1091, same known false positive
#     tools/tests/pre_push_gate_test.sh already disables for its own sibling `source`.
# PT: disable de shellcheck pro arquivo inteiro -- SC1091, o mesmo falso positivo
#     conhecido que o tools/tests/pre_push_gate_test.sh já desabilita pro `source`
#     irmão dele.
# shellcheck disable=SC1091
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

source "${REPO_ROOT}/.githooks/pre-push"

pass=0
fail=0

# -----------------------------------------------------------------------------
# EN: throwaway fixture repo, built once under /var/tmp (disk, not tmpfs) and torn
#     down on exit via `trap ... EXIT` regardless of pass/fail -- no leftover scratch.
# PT: repo descartável de fixture, construído uma vez sob /var/tmp (disco, não
#     tmpfs) e desmontado na saída via `trap ... EXIT` independente de passar/falhar
#     -- sem sobra de scratch.
# -----------------------------------------------------------------------------
FIXTURE_DIR="$(mktemp -d /var/tmp/tag-release-consistency-fixture.XXXXXX)"
cleanup() { rm -rf "${FIXTURE_DIR}"; }
trap cleanup EXIT

git init -q "${FIXTURE_DIR}"
git -C "${FIXTURE_DIR}" config user.email "tag-release-consistency-test@example.invalid"
git -C "${FIXTURE_DIR}" config user.name "tag-release-consistency-test"
mkdir -p "${FIXTURE_DIR}/glintfx"

# -----------------------------------------------------------------------------
# EN: commit_fixture CMAKE_BODY CHANGELOG_BODY -- writes both files, commits, prints
#     the new commit's SHA on stdout. Content is caller-controlled so each scenario
#     below can vary the CMakeLists version and/or the CHANGELOG heading independently.
# PT: commit_fixture CMAKE_BODY CHANGELOG_BODY -- escreve os dois arquivos, commita,
#     imprime o SHA do novo commit no stdout. Conteúdo é controlado por quem chama,
#     então cada cenário abaixo pode variar a versão do CMakeLists e/ou o heading do
#     CHANGELOG independentemente.
# -----------------------------------------------------------------------------
commit_fixture() {
  local cmake_body="$1"
  local changelog_body="$2"

  printf '%s' "${cmake_body}" >"${FIXTURE_DIR}/glintfx/CMakeLists.txt"
  printf '%s' "${changelog_body}" >"${FIXTURE_DIR}/CHANGELOG.md"
  git -C "${FIXTURE_DIR}" add -A
  git -C "${FIXTURE_DIR}" commit -q -m "fixture commit"
  git -C "${FIXTURE_DIR}" rev-parse HEAD
}

# EN: (a)/(b) shared CHANGELOG body -- has the "1.2.3" section, used by both the
#     consistent-tag and the version-mismatch scenarios (the CHANGELOG side is fine
#     in both; only the CMakeLists version differs).
# PT: corpo de CHANGELOG compartilhado por (a)/(b) -- tem a seção "1.2.3", usado
#     tanto no cenário de tag consistente quanto no de versão divergente (o lado do
#     CHANGELOG está ok nos dois; só a versão do CMakeLists difere).
CHANGELOG_HAS_123="## [1.2.3] - 2026-01-01

### Added

- fixture entry.
"

SHA_GOOD="$(commit_fixture \
  'cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 1.2.3)
' \
  "${CHANGELOG_HAS_123}")"

SHA_BAD_VERSION="$(commit_fixture \
  'cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 1.2.9)
' \
  "${CHANGELOG_HAS_123}")"

SHA_BAD_CHANGELOG="$(commit_fixture \
  'cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 1.2.3)
' \
  "## [1.2.2] - 2025-12-31

### Added

- an older section, not 1.2.3.
")"

SHA_BAD_PROJECT="$(commit_fixture \
  'cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX)
' \
  "## [1.4.0] - 2026-02-01

### Added

- fixture entry.
")"

# -----------------------------------------------------------------------------
# EN: assert_check DESC EXPECT LINE... -- feeds LINE... to classify_push_refs()
#     (same "<local ref> <local sha> <remote ref> <remote sha>" shape git puts on
#     pre-push's stdin), then runs check_tag_release_consistency() with CWD inside
#     FIXTURE_DIR (a subshell -- `( cd ... && fn )` -- inherits the caller's already-
#     defined function AND the push_tag_shas/push_tag_refs arrays classify_push_refs
#     just set, since a `(...)` subshell is a fork, not a fresh interpreter). EXPECT
#     is "pass" or "fail".
# PT: assert_check DESC EXPECT LINE... -- alimenta LINE... pro classify_push_refs()
#     (mesma forma "<local ref> <local sha> <remote ref> <remote sha>" que o git bota
#     no stdin do pre-push), então roda o check_tag_release_consistency() com o CWD
#     dentro do FIXTURE_DIR (um subshell -- `( cd ... && fn )` -- herda a função já
#     definida por quem chamou E os arrays push_tag_shas/push_tag_refs que o
#     classify_push_refs acabou de setar, já que um subshell `(...)` é um fork, não
#     um interpretador novo). EXPECT é "pass" ou "fail".
# -----------------------------------------------------------------------------
assert_check() {
  local desc="$1"
  local expect="$2"
  shift 2

  classify_push_refs "$@"

  local output rc
  if output="$(cd "${FIXTURE_DIR}" && check_tag_release_consistency 2>&1)"; then
    rc=0
  else
    rc=1
  fi

  local got="fail"
  [[ "${rc}" -eq 0 ]] && got="pass"

  if [[ "${got}" == "${expect}" ]]; then
    echo "ok - ${desc}"
    pass=$((pass + 1))
  else
    echo "not ok - ${desc} (want '${expect}', got '${got}')" >&2
    [[ -n "${output}" ]] && echo "    output: ${output}" >&2
    fail=$((fail + 1))
  fi
}

# -----------------------------------------------------------------------------
# EN: the 6 acceptance scenarios from the REL-TAGCONSIST-GATE brief, verbatim.
# PT: os 6 cenários de aceite do brief do REL-TAGCONSIST-GATE, ao pé da letra.
# -----------------------------------------------------------------------------

# (a) tag consistente -> passa
assert_check \
  "(a) consistent tag (CMakeLists 1.2.3, CHANGELOG has [1.2.3], tag v1.2.3) -> pass" \
  "pass" \
  "refs/tags/v1.2.3 ${SHA_GOOD} refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# (b) versão divergente -> reprova
assert_check \
  "(b) version mismatch (CMakeLists 1.2.9, tag v1.2.3) -> fail" \
  "fail" \
  "refs/tags/v1.2.3 ${SHA_BAD_VERSION} refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# (c) CHANGELOG sem a seção -> reprova
assert_check \
  "(c) CHANGELOG missing the release section (CMakeLists 1.2.3, no [1.2.3] heading, tag v1.2.3) -> fail" \
  "fail" \
  "refs/tags/v1.2.3 ${SHA_BAD_CHANGELOG} refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# (d) push só de branch, sem tag -> não interfere
assert_check \
  "(d) branch-only push (no tag ref at all) -> pass (does not interfere)" \
  "pass" \
  "refs/heads/main ${SHA_GOOD} refs/heads/main 1111111111111111111111111111111111111a"

# (e) push misto branch+tag -> checa a tag (duas variantes: tag boa e tag ruim, as duas com branch junto)
assert_check \
  "(e1) mixed branch+GOOD tag -> pass (tag still checked, not skipped because of the branch)" \
  "pass" \
  "refs/heads/main ${SHA_GOOD} refs/heads/main 1111111111111111111111111111111111111a" \
  "refs/tags/v1.2.3 ${SHA_GOOD} refs/tags/v1.2.3 0000000000000000000000000000000000000000"

assert_check \
  "(e2) mixed branch+BAD tag -> fail (the branch ref does not mask a real inconsistency)" \
  "fail" \
  "refs/heads/main ${SHA_GOOD} refs/heads/main 1111111111111111111111111111111111111a" \
  "refs/tags/v1.2.3 ${SHA_BAD_VERSION} refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# (f) project() ilegível/ausente -> reprova (fail-closed)
assert_check \
  "(f) project(VERSION ...) unreadable (VERSION field absent from project()) -> fail (fail-closed)" \
  "fail" \
  "refs/tags/v1.4.0 ${SHA_BAD_PROJECT} refs/tags/v1.4.0 0000000000000000000000000000000000000000"

# -----------------------------------------------------------------------------
# EN: extra coverage beyond the brief's 6 -- a tag ref NOT shaped like vX.Y.Z is out
#     of scope (REL_TAG_VERSION_RE) and must not be checked at all (even against the
#     BAD_VERSION commit, which would fail if it WERE checked -- this proves the
#     out-of-scope skip is real, not an accidental pass).
# PT: cobertura extra além dos 6 do brief -- uma ref de tag que NÃO tem a forma
#     vX.Y.Z está fora de escopo (REL_TAG_VERSION_RE) e não deve ser checada de jeito
#     nenhum (mesmo contra o commit BAD_VERSION, que falharia SE fosse checado --
#     isto prova que o skip de fora-de-escopo é real, não um passar por acidente).
# -----------------------------------------------------------------------------
assert_check \
  "(g) non-release-shaped tag name (refs/tags/experimental) against an inconsistent commit -> pass (out of scope, not checked)" \
  "pass" \
  "refs/tags/experimental ${SHA_BAD_VERSION} refs/tags/experimental 0000000000000000000000000000000000000000"

echo ""
echo "== tag_release_consistency_test: ${pass} passed, ${fail} failed =="

if [[ "${fail}" -gt 0 ]]; then
  exit 1
fi
