#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: CI-TAGFAST self-test (TODO.md). Pure-bash, zero network, zero build: sources
#     .githooks/pre-push (safe -- see that file's main()/BASH_SOURCE[0]==$0 guard,
#     the same shape tools/preci.sh already uses) and exercises
#     classify_push_refs()/decide_action() directly, feeding synthetic
#     "<local ref> <local sha> <remote ref> <remote sha>" lines exactly as git would
#     put them on the hook's stdin.
#
#     tag_commit_is_verified() is the ONE function in the hook that touches the
#     network (git ls-remote); it is redefined here (plain function redefinition
#     after `source`, no framework) to a test double keyed on the sha it is asked
#     about, so every scenario below runs in milliseconds with no remote at all.
#
#     What this covers (and does NOT cover): the decision logic only -- push_saw_ref/
#     push_only_tags/push_tag_shas classification and the full-vs-skip verdict. It
#     does NOT exercise the real tag_commit_is_verified() (real `git ls-remote` +
#     `git merge-base --is-ancestor`), nor main()'s stdin plumbing end to end, nor
#     tools/preci.sh itself. Those are the aceite scenarios run by hand against a real
#     push (documented in the CI-TAGFAST commit/PR, per this house's own
#     "implementer's own claims are not proof" discipline) -- this script is the fast
#     regression net that guards the classification/decision code from silently
#     regressing back to "always full" or, worse, "skip when it should not".
#
# PT: self-test do CI-TAGFAST (TODO.md). Puro-bash, zero rede, zero build: sourceia o
#     .githooks/pre-push (seguro -- ver o guard main()/BASH_SOURCE[0]==$0 daquele
#     arquivo, a mesma forma que o tools/preci.sh já usa) e exercita
#     classify_push_refs()/decide_action() direto, alimentando linhas sintéticas
#     "<local ref> <local sha> <remote ref> <remote sha>" exatamente como o git as
#     colocaria no stdin do hook.
#
#     tag_commit_is_verified() é a ÚNICA função do hook que toca a rede (git
#     ls-remote); ela é redefinida aqui (redefinição de função pura depois de
#     `source`, sem framework) pra um dublê de teste indexado pelo sha perguntado,
#     então todo cenário abaixo roda em milissegundos sem remoto nenhum.
#
#     O que isto cobre (e o que NÃO cobre): só a lógica de DECISÃO -- a classificação
#     push_saw_ref/push_only_tags/push_tag_shas e o veredito full-vs-skip. NÃO
#     exercita o tag_commit_is_verified() de verdade (git ls-remote +
#     git merge-base --is-ancestor reais), nem a mecânica de stdin do main() fim-a-fim,
#     nem o próprio tools/preci.sh. Esses são os cenários de aceite rodados à mão
#     contra um push real (documentados no commit/PR do CI-TAGFAST, pela disciplina da
#     casa "relatório de agente não é prova") -- este script é a rede de regressão
#     rápida que protege o código de classificação/decisão de regredir em silêncio de
#     volta pra "sempre full" ou, pior, "pula quando não devia".
#
# Usage / Uso: tools/tests/pre_push_gate_test.sh   (exit 0 = all assertions passed)
#
# EN: file-wide shellcheck disable -- SC1091 (can't statically follow the dynamic
#     `source "${REPO_ROOT}/.githooks/pre-push"` path below), the same known false
#     positive tools/tests/preci_detect_test.sh already disables for its own sibling
#     `source`.
# PT: disable de shellcheck pro arquivo inteiro -- SC1091 (não dá pra seguir
#     estaticamente o caminho dinâmico do `source "${REPO_ROOT}/.githooks/pre-push"`
#     abaixo), o mesmo falso positivo conhecido que o
#     tools/tests/preci_detect_test.sh já desabilita pro próprio `source` irmão.
# shellcheck disable=SC1091
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

source "${REPO_ROOT}/.githooks/pre-push"

pass=0
fail=0

# -----------------------------------------------------------------------------
# EN: test double for tag_commit_is_verified() -- keyed on sha, not on remote (no
#     scenario below needs more than one remote name at a time). VERIFIED_SHAS is a
#     space-separated allowlist set per-test right before calling decide_action();
#     any sha NOT in it is treated as unverified (mirrors the real function's
#     fail-toward-full posture on doubt).
# PT: dublê de teste do tag_commit_is_verified() -- indexado por sha, não por remote
#     (nenhum cenário abaixo precisa de mais de um nome de remote por vez).
#     VERIFIED_SHAS é uma allowlist separada por espaço, setada por teste logo antes
#     de chamar decide_action(); qualquer sha NÃO nela é tratado como não-verificado
#     (espelha a postura de falhar-em-direção-ao-full do real, na dúvida).
# -----------------------------------------------------------------------------
VERIFIED_SHAS=""

tag_commit_is_verified() {
  local sha="$1"
  case " ${VERIFIED_SHAS} " in
    *" ${sha} "*) return 0 ;;
    *) return 1 ;;
  esac
}

# -----------------------------------------------------------------------------
# EN: assert_action DESC VERIFIED_SHAS EXPECT_ACTION REMOTE LINE... -- sets the test
#     double's allowlist, classifies LINE... via classify_push_refs(), calls
#     decide_action(REMOTE), and checks the result against EXPECT_ACTION ("full" or
#     "skip"). Measures decide_action()'s output via `action="$(decide_action ...)"`
#     directly -- no pipeline to `grep`/`wc`, per this house's own measured lesson
#     that piping a command into another swallows the FIRST command's exit status
#     (six wrong readings in one session before this rule was canonized).
# PT: assert_action DESC VERIFIED_SHAS EXPECT_ACTION REMOTE LINE... -- seta a
#     allowlist do dublê de teste, classifica LINE... via classify_push_refs(), chama
#     decide_action(REMOTE), e confere o resultado contra EXPECT_ACTION ("full" ou
#     "skip"). Mede a saída do decide_action() via `action="$(decide_action ...)"`
#     direto -- sem pipeline pra `grep`/`wc`, pela lição medida da própria casa de que
#     encanar um comando pra outro engole o exit status do PRIMEIRO comando (seis
#     leituras erradas numa sessão só antes desta regra ser canonizada).
# -----------------------------------------------------------------------------
assert_action() {
  local desc="$1"
  local verified="$2"
  local expect="$3"
  local remote="$4"
  shift 4

  VERIFIED_SHAS="${verified}"
  classify_push_refs "$@"

  local action
  action="$(decide_action "${remote}")"

  if [[ "${action}" == "${expect}" ]]; then
    echo "ok - ${desc}"
    pass=$((pass + 1))
  else
    echo "not ok - ${desc} (want '${expect}', got '${action}')" >&2
    fail=$((fail + 1))
  fi
}

# -----------------------------------------------------------------------------
# EN: the 5 acceptance scenarios from the CI-TAGFAST brief, verbatim.
# PT: os 5 cenários de aceite do brief do CI-TAGFAST, ao pé da letra.
# -----------------------------------------------------------------------------

# 1) só tag, commit já no remoto -> NÃO roda --full (skip)
assert_action \
  "1) tag-only, commit already on remote -> skip" \
  "aaaa000000000000000000000000000000000a" \
  "skip" "origin" \
  "refs/tags/v1.2.3 aaaa000000000000000000000000000000000a refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# 2) só tag, commit NÃO no remoto -> roda --full
assert_action \
  "2) tag-only, commit NOT on remote -> full" \
  "" \
  "full" "origin" \
  "refs/tags/v1.2.3 bbbb000000000000000000000000000000000b refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# 3) só branch -> roda --full
assert_action \
  "3) branch-only -> full" \
  "cccc000000000000000000000000000000000c" \
  "full" "origin" \
  "refs/heads/main cccc000000000000000000000000000000000c refs/heads/main dddd000000000000000000000000000000000d"

# 4) misto (branch + tag) -> roda --full, mesmo com a tag verificada
assert_action \
  "4) mixed branch+tag, tag sha verified but branch present -> full" \
  "eeee000000000000000000000000000000000e" \
  "full" "origin" \
  "refs/heads/main ffff000000000000000000000000000000000f refs/heads/main 0000000000000000000000000000000000000000" \
  "refs/tags/v1.2.3 eeee000000000000000000000000000000000e refs/tags/v1.2.3 0000000000000000000000000000000000000000"

# 5) stdin vazio -> decisão segura (full), nunca skip
assert_action \
  "5) empty stdin (no ref lines) -> full (fail-safe, never skip)" \
  "" \
  "full" "origin"

# -----------------------------------------------------------------------------
# EN: extra coverage beyond the brief's 5 -- blank-line-only stdin (git can pad with
#     a trailing newline; must classify the same as truly empty, not as "saw a ref"),
#     tag deletion (all-zero local sha -- must count as verified/no-op, not force
#     --full), and multiple tags in one push where only SOME are verified (any single
#     unverified tag must force --full for the whole batch).
# PT: cobertura extra além dos 5 do brief -- stdin só-com-linha-em-branco (o git pode
#     preencher com uma newline final; tem que classificar igual a vazio de verdade,
#     não como "viu uma ref"), deleção de tag (sha local todo-zero -- tem que contar
#     como verificado/no-op, não forçar --full), e múltiplas tags num push só onde
#     SÓ ALGUMAS estão verificadas (qualquer uma não-verificada força --full pro lote
#     inteiro).
# -----------------------------------------------------------------------------
assert_action \
  "6) blank-line-only args -> full (same as empty, not 'saw a ref')" \
  "" \
  "full" "origin" \
  ""

assert_action \
  "7) tag deletion (all-zero local sha) -> skip (nothing new becomes visible)" \
  "" \
  "skip" "origin" \
  "refs/tags/v0.9.9 0000000000000000000000000000000000000000 refs/tags/v0.9.9 1111000000000000000000000000000000000a"

assert_action \
  "8) two tags, only one verified -> full (whole batch fails toward full)" \
  "2222000000000000000000000000000000000a" \
  "full" "origin" \
  "refs/tags/v1.0.0 2222000000000000000000000000000000000a refs/tags/v1.0.0 0000000000000000000000000000000000000000" \
  "refs/tags/v1.1.0 3333000000000000000000000000000000000a refs/tags/v1.1.0 0000000000000000000000000000000000000000"

assert_action \
  "9) two tags, both verified -> skip" \
  "4444000000000000000000000000000000000a 5555000000000000000000000000000000000a" \
  "skip" "origin" \
  "refs/tags/v1.0.0 4444000000000000000000000000000000000a refs/tags/v1.0.0 0000000000000000000000000000000000000000" \
  "refs/tags/v1.1.0 5555000000000000000000000000000000000a refs/tags/v1.1.0 0000000000000000000000000000000000000000"

echo ""
echo "== pre_push_gate_test: ${pass} passed, ${fail} failed =="

if [[ "${fail}" -gt 0 ]]; then
  exit 1
fi
