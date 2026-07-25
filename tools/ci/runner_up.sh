#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Host-side wrapper that starts ONE ephemeral, containerized GitHub Actions runner
#     for glintfx's heavy checks. It mints a fresh registration token with the `gh` CLI
#     (already authenticated on this machine, token in the login keyring) and feeds it to
#     the container over stdin. Nothing is stored: there is no Personal Access Token in
#     any file, and the minted token expires in about an hour.
#
#     Run it under the supervisor, not by hand:
#       systemctl --user enable --now glintfx-heavy-runner
#     By hand is still fine for debugging (it stays in the foreground, Ctrl-C stops it):
#       tools/ci/runner_up.sh
#
#     Resource caps, and WHY (these are the lead's numbers, do not raise them casually):
#       --memory=8g --memory-swap=8g : 8 GiB ceiling AND zero swap for the container.
#           Setting --memory-swap equal to --memory is what disables swap; without it the
#           container can push the lead's RAM into swap, and swap pressure is what
#           contributed to a real OOM on this machine. The machine has 31 GiB, but the
#           lead WORKS on it while CI runs, so the ceiling protects his session, not the
#           job.
#       --cpus=4 : of 16 cores. Same reason. The workflow additionally pins every build
#           to `-j2` (a literal, never `$(nproc)`: inside a container nproc still reports
#           the host's 16 cores and would spawn 16 compilers under an 8 GiB cap, which is
#           precisely the OOM recipe).
#
#     Isolation, and WHY (the repository is PUBLIC): no docker socket, no host $HOME, no
#     bind mounts at all, no secrets, capabilities dropped, no-new-privileges. Combined
#     with heavy.yml having NO pull_request trigger and the runner being --ephemeral,
#     a fork cannot reach this machine. See tools/ci/Containerfile.runner's header.
#
# PT: Wrapper do lado do host que sobe UM runner do GitHub Actions conteinerizado e
#     efêmero para os checks pesados da glintfx. Ele minta um token de registro novo com
#     a CLI `gh` (já autenticada nesta máquina, token no keyring de login) e o entrega ao
#     container pelo stdin. Nada fica armazenado: não há Personal Access Token em arquivo
#     nenhum, e o token mintado expira em cerca de uma hora.
#
#     Rode sob o supervisor, não à mão:
#       systemctl --user enable --now glintfx-heavy-runner
#     À mão segue válido pra depurar (fica em primeiro plano, Ctrl-C encerra):
#       tools/ci/runner_up.sh
#
#     Tetos de recurso, e POR QUÊ (são os números do líder, não suba por conta própria):
#       --memory=8g --memory-swap=8g : teto de 8 GiB E swap zerado pro container. Igualar
#           --memory-swap a --memory é o que desliga o swap; sem isso o container pode
#           empurrar a RAM do líder pro swap, e a pressão de swap foi o que contribuiu
#           pra um OOM real nesta máquina. A máquina tem 31 GiB, mas o líder TRABALHA
#           nela enquanto o CI roda, então o teto protege a sessão dele, não o job.
#       --cpus=4 : de 16 núcleos. Mesmo motivo. O workflow ainda fixa todo build em `-j2`
#           (literal, nunca `$(nproc)`: dentro do container o nproc continua reportando
#           os 16 núcleos do host e dispararia 16 compiladores sob um teto de 8 GiB, que
#           é exatamente a receita do OOM).
#
#     Isolamento, e POR QUÊ (o repositório é PÚBLICO): sem socket do docker, sem $HOME do
#     host, sem bind mount nenhum, sem segredos, capabilities derrubadas,
#     no-new-privileges. Somado ao heavy.yml NÃO ter gatilho de pull_request e ao runner
#     ser --ephemeral, um fork não alcança esta máquina. Ver o cabeçalho de
#     tools/ci/Containerfile.runner.
#
set -euo pipefail

repo_slug="${GLINTFX_RUNNER_REPO:-petrinhu/glintfx}"
image="${GLINTFX_RUNNER_IMAGE:-localhost/glintfx-runner:f42}"
container="${GLINTFX_RUNNER_CONTAINER:-glintfx-heavy-runner}"
labels="${GLINTFX_RUNNER_LABELS:-self-hosted,linux,x64,glintfx-heavy}"
memory="${GLINTFX_RUNNER_MEMORY:-8g}"
cpus="${GLINTFX_RUNNER_CPUS:-4}"

die() { echo "runner_up: $*" >&2; exit 1; }

command -v gh     >/dev/null || die "gh CLI not found in PATH"
command -v docker >/dev/null || die "docker not found in PATH"
docker image inspect "${image}" >/dev/null 2>&1 \
  || die "image ${image} not found -- run tools/ci/build_runner_image.sh first"

# EN: A crashed predecessor can leave the fixed name taken; --rm usually handles it, this
#     covers the case where it did not.
# PT: Um antecessor que quebrou pode deixar o nome fixo ocupado; o --rm normalmente
#     resolve, isto cobre o caso em que não resolveu.
docker rm -f "${container}" >/dev/null 2>&1 || true

# EN: Fresh registration token, minted now, used once. Requires admin on the repo, which
#     the keyring-stored `gh` login has (`repo` scope, repository owner).
# PT: Token de registro novo, mintado agora, usado uma vez. Exige admin no repositório, o
#     que o login `gh` guardado no keyring tem (escopo `repo`, dono do repositório).
token="$(gh api -X POST "repos/${repo_slug}/actions/runners/registration-token" --jq .token)" \
  || die "could not mint a registration token (is gh authenticated? is the keyring unlocked?)"
[ -n "${token}" ] || die "gh returned an empty registration token"

echo "==> starting ephemeral runner ${container} (${image}) for ${repo_slug}"
echo "    caps: memory=${memory} swap=0 cpus=${cpus} labels=${labels}"

printf '%s\n' "${token}" | docker run \
  --rm \
  --interactive \
  --name "${container}" \
  --memory="${memory}" \
  --memory-swap="${memory}" \
  --cpus="${cpus}" \
  --pids-limit=4096 \
  --cap-drop=ALL \
  --security-opt=no-new-privileges \
  --env "GLINTFX_RUNNER_REPO_URL=https://github.com/${repo_slug}" \
  --env "GLINTFX_RUNNER_LABELS=${labels}" \
  "${image}"

exit "${PIPESTATUS[1]}"
