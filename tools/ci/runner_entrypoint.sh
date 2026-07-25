#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: PID 1 of the glintfx heavy runner container (tools/ci/Containerfile.runner).
#     Registers ONE ephemeral GitHub Actions runner and hands over to run.sh; the
#     process exits as soon as that single job finishes, and the host-side supervisor
#     (tools/ci/glintfx-heavy-runner.service, Restart=always) starts a fresh container.
#
#     The registration token arrives on STDIN, one line, and nowhere else. Not an
#     argument to `docker run` (argv is world-readable in `ps` on the host), not an
#     environment variable (env is readable via `docker inspect`). It is short-lived
#     anyway (roughly one hour) and is minted per start by tools/ci/runner_up.sh, so no
#     long-lived Personal Access Token is ever written to disk.
#     Residual exposure, stated honestly: `config.sh` only accepts the token as a
#     command-line argument, so for the few seconds it runs the token is visible in the
#     process list INSIDE this container. The container is single-tenant and ephemeral,
#     so that is the accepted floor, not an oversight.
#
# PT: PID 1 do container do runner pesado da glintfx (tools/ci/Containerfile.runner).
#     Registra UM runner efêmero do GitHub Actions e passa a bola pro run.sh; o processo
#     sai assim que aquele job único termina, e o supervisor do lado do host
#     (tools/ci/glintfx-heavy-runner.service, Restart=always) sobe um container novo.
#
#     O token de registro chega pelo STDIN, uma linha, e por mais lugar nenhum. Não é
#     argumento do `docker run` (argv é legível por qualquer um no `ps` do host), não é
#     variável de ambiente (env é legível via `docker inspect`). Ele já é de vida curta
#     (cerca de uma hora) e é mintado a cada partida pelo tools/ci/runner_up.sh, então
#     nenhum Personal Access Token de vida longa chega a ser escrito em disco.
#     Exposição residual, dita com honestidade: o `config.sh` só aceita o token como
#     argumento de linha de comando, então durante os poucos segundos em que ele roda o
#     token fica visível na lista de processos DENTRO deste container. O container é
#     mono-inquilino e efêmero, então esse é o piso aceito, não um descuido.
set -euo pipefail

repo_url="${GLINTFX_RUNNER_REPO_URL:?GLINTFX_RUNNER_REPO_URL not set}"
labels="${GLINTFX_RUNNER_LABELS:-self-hosted,linux,x64,glintfx-heavy}"
name="${GLINTFX_RUNNER_NAME:-glintfx-heavy-$(head -c 4 /dev/urandom | od -An -tx1 | tr -d ' \n')}"

# EN: One line on stdin, with a timeout so a mis-wired supervisor fails fast instead of
#     hanging a container forever holding 8 GiB of quota.
# PT: Uma linha no stdin, com timeout pra um supervisor mal ligado falhar rápido em vez
#     de pendurar um container pra sempre segurando 8 GiB de cota.
if ! IFS= read -r -t 30 token; then
  echo "runner_entrypoint: no registration token on stdin after 30s" >&2
  exit 1
fi
if [ -z "${token}" ]; then
  echo "runner_entrypoint: empty registration token on stdin" >&2
  exit 1
fi

cd /home/runner/actions-runner

# EN: --ephemeral    one job, then exit and de-register (GitHub removes the runner).
#     --disableupdate  never self-update: the pinned version in Containerfile.runner is
#                      the single source of truth (and a self-update would be lost with
#                      the container anyway).
#     --replace        tolerate a stale registration with the same name after a crash.
#     --unattended     no prompts.
# PT: --ephemeral    um job, depois sai e se des-registra (o GitHub remove o runner).
#     --disableupdate  nunca se auto-atualiza: a versão fixada no Containerfile.runner é
#                      a fonte única de verdade (e uma auto-atualização se perderia com o
#                      container de qualquer forma).
#     --replace        tolera registro obsoleto de mesmo nome depois de um crash.
#     --unattended     sem prompts.
./config.sh \
  --unattended \
  --ephemeral \
  --disableupdate \
  --replace \
  --url "${repo_url}" \
  --token "${token}" \
  --name "${name}" \
  --labels "${labels}" \
  --work /home/runner/_work

unset token

exec ./run.sh
