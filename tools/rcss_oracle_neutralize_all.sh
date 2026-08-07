#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: `UIX-ORACLE-MECANISMO` -- drives `rcss_dump_differential_oracle`'s own per-pin meta-test
#     (`docs/uix-rcss.md` section 14.1's own normative requirement 3: "every exception MUST be
#     exercised by at least one fixture, and the oracle MUST fail if any exception is never
#     exercised"). Discovers the pin count itself via `${EXE} --list-pins` (no display needed for
#     that one call), then re-runs `${EXE}` once per pin `k` with `GLINTFX_ORACLE_NEUTRALIZE_PIN=k`
#     set -- each such run makes row `k` invisible to `matches()`'s own caller, so every diff line
#     that row would otherwise have swallowed surfaces as a brand-new UNKNOWN divergence and the
#     process exits non-zero. A pin whose neutralized run exits ZERO means that pin's own named
#     fixture is not actually reaching the code path the row excuses -- a build error for this
#     fatia, not a silent pass (same discipline `feedback_mutante_em_arquivo_nao_commitado`/
#     `SEED-GOLDEN-INERTE`, both `TODO.md`, already apply to every other guard in this repo).
#
#     One process per `k` (never one long-lived process iterating an in-process loop) so one pin's
#     own neutralization can never bleed into another's -- same "process isolation over shared
#     state" reasoning `run_xvfb.cmake`'s own `-DARGS=<mode>` precedent (`capture_framebuffer_
#     packskip_regression`) already established for this repo's own test infrastructure.
#
#     Own Xvfb/XDG_RUNTIME_DIR isolation recipe, copied from `run_xvfb.cmake`'s own header comment
#     and adapted (one fake runtime dir PER neutralization run, same "one `mktemp -d` per invocation,
#     no create/collide race under `ctest -j`" property) -- NOT delegated to `run_xvfb.cmake` itself,
#     because that wrapper treats a non-zero exit as ITS OWN test failure (`message(FATAL_ERROR ...)`
#     on `rc != 0`), the exact OPPOSITE of what every individual neutralized run in this script is
#     supposed to do.
#
# PT: `UIX-ORACLE-MECANISMO` -- conduz o meta-teste por-pin do PRÓPRIO rcss_dump_differential_oracle
#     (o próprio requisito normativo 3 da seção 14.1 do docs/uix-rcss.md: "toda exceção DEVE ser
#     exercitada por pelo menos uma fixture, e o oráculo DEVE falhar se alguma exceção nunca for
#     exercitada"). Descobre a própria contagem de pin via `${EXE} --list-pins` (sem display nenhum
#     precisado pra essa chamada), depois roda `${EXE}` de novo, uma vez por pin `k`, com
#     `GLINTFX_ORACLE_NEUTRALIZE_PIN=k` setado -- cada rodada dessas torna a linha `k` invisível pro
#     PRÓPRIO chamador do matches(), então toda linha de diff que aquela linha teria engolido de
#     outro jeito aparece como divergência UNKNOWN nova e o processo sai não-zero. Um pin cuja
#     rodada neutralizada sai ZERO significa que a própria fixture nomeada daquele pin não está de
#     fato alcançando o caminho de código que a linha desculpa -- um erro de build pra esta fatia,
#     não uma passagem silenciosa (a mesma disciplina que feedback_mutante_em_arquivo_nao_commitado/
#     SEED-GOLDEN-INERTE, os dois no TODO.md, já aplicam a todo outro guarda deste repo).
#
#     Um processo por `k` (nunca um processo de vida longa iterando um laço in-process) pra que a
#     própria neutralização de um pin nunca vaze pra outro -- mesmo racional "isolamento de processo
#     acima de estado compartilhado" que o próprio precedente `-DARGS=<mode>` do run_xvfb.cmake
#     (capture_framebuffer_packskip_regression) já estabeleceu pra própria infraestrutura de teste
#     deste repo.
#
#     Receita própria de isolamento Xvfb/XDG_RUNTIME_DIR, copiada do próprio comentário de cabeçalho
#     do run_xvfb.cmake e adaptada (um diretório de runtime falso POR rodada de neutralização, a
#     mesma propriedade "um mktemp -d por invocação, sem corrida de criação/colisão sob ctest -j")
#     -- NÃO delegada ao próprio run_xvfb.cmake, porque aquele wrapper trata saída não-zero como
#     falha DO PRÓPRIO teste dele (message(FATAL_ERROR ...) em rc != 0), o OPOSTO exato do que toda
#     rodada neutralizada individual deste script tem que fazer.
#
# Usage / Uso:
#   tools/rcss_oracle_neutralize_all.sh <path-to-rcss_dump_differential_oracle-binary>
set -euo pipefail

die() {
  echo "error: $*" >&2
  exit 1
}

[[ $# -ge 1 ]] || die "usage: $(basename -- "${BASH_SOURCE[0]}") <path-to-rcss_dump_differential_oracle-binary>"
EXE="$1"
[[ -x "${EXE}" ]] || die "'${EXE}' is not an executable file"

# EN: --list-pins needs no display -- fail loud if it ever does (a future change to main()'s own
#     argv handling that moves window creation before the --list-pins check would silently break
#     this, and this check turns that silent break into a clear message here instead).
# PT: --list-pins não precisa de display nenhum -- falha alto se algum dia precisar (uma mudança
#     futura no próprio tratamento de argv do main() que mova a criação de janela pra antes da
#     checagem de --list-pins quebraria isto em silêncio, e esta checagem transforma essa quebra
#     silenciosa numa mensagem clara aqui).
N="$("${EXE}" --list-pins)" || die "'${EXE} --list-pins' exited non-zero"
[[ "${N}" =~ ^[0-9]+$ ]] || die "'${EXE} --list-pins' printed '${N}', not a plain non-negative integer"

echo "rcss_oracle_neutralize_all: ${N} pin(s) declared by kKnownDivergences -- neutralizing each in turn" >&2

if [[ "${N}" -eq 0 ]]; then
  die "0 pins declared -- this meta-test would vacuously pass with nothing exercised; treat an empty kKnownDivergences as a failure of THIS test, not a green light"
fi

tmp_base="${TMPDIR:-/tmp}"
fail_count=0

for ((k = 0; k < N; ++k)); do
  fake_xdg_runtime="$(mktemp -d "${tmp_base}/glintfx-oracle-neutralize-xvfb.XXXXXX")"
  rc=0
  out="$(env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR="${fake_xdg_runtime}" GLINTFX_ORACLE_NEUTRALIZE_PIN="${k}" \
    xvfb-run -a "${EXE}" 2>&1)" || rc=$?
  rm -rf "${fake_xdg_runtime}"

  if [[ "${rc}" -eq 0 ]]; then
    echo "FAIL: pin ${k} of ${N} did NOT turn the run red when neutralized (exit 0) -- its own fixture is not actually exercising the code path this pin excuses" >&2
    echo "${out}" >&2
    fail_count=$((fail_count + 1))
  else
    echo "ok: pin ${k} of ${N} correctly failed the run when neutralized (exit ${rc})" >&2
  fi
done

if [[ "${fail_count}" -gt 0 ]]; then
  echo "rcss_oracle_neutralize_all: ${fail_count} of ${N} pin(s) did NOT go red when neutralized -- see FAIL lines above" >&2
  exit 1
fi

echo "rcss_oracle_neutralize_all: all ${N} pin(s) correctly went red when neutralized -- every kKnownDivergences row is exercised" >&2
exit 0
