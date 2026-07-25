#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Builds localhost/glintfx-runner:f42 -- the containerized GitHub Actions
#     self-hosted runner that hosts glintfx's heavy checks (`sanitize` and `fonteng` in
#     .github/workflows/heavy.yml). It is localhost/glintfx-ci:f42 plus the pinned
#     actions/runner, so this script builds the base image first (that build is a
#     near-instant no-op when nothing changed).
#
#     Run this whenever tools/ci/Containerfile.runner, tools/ci/Containerfile.f42 or
#     tools/ci/runner_entrypoint.sh changes, then restart the service so the new image
#     is picked up:
#
#       tools/ci/build_runner_image.sh
#       systemctl --user restart glintfx-heavy-runner
#
# PT: Builda a localhost/glintfx-runner:f42 -- o runner self-hosted do GitHub Actions
#     conteinerizado que hospeda os checks pesados da glintfx (`sanitize` e `fonteng` no
#     .github/workflows/heavy.yml). Ela é a localhost/glintfx-ci:f42 mais o
#     actions/runner fixado, então este script builda a imagem base antes (esse build é
#     um no-op quase instantâneo quando nada mudou).
#
#     Rode sempre que tools/ci/Containerfile.runner, tools/ci/Containerfile.f42 ou
#     tools/ci/runner_entrypoint.sh mudar, e então reinicie o serviço pra nova imagem ser
#     usada:
#
#       tools/ci/build_runner_image.sh
#       systemctl --user restart glintfx-heavy-runner
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tag="localhost/glintfx-runner:f42"

echo "==> building the base image first (localhost/glintfx-ci:f42)"
"${here}/build_image.sh"

# EN: Build context is tools/ci/ itself -- the only file COPYed in is
#     runner_entrypoint.sh, so there is no reason to hand docker the whole repository.
# PT: O contexto de build é o próprio tools/ci/ -- o único arquivo COPYado é o
#     runner_entrypoint.sh, então não há motivo pra entregar o repositório inteiro ao
#     docker.
echo "==> building ${tag} from tools/ci/Containerfile.runner"
docker build -f "${here}/Containerfile.runner" -t "${tag}" "${here}"

echo "==> sanity check"
docker run --rm --entrypoint cat "${tag}" /etc/glintfx-runner-image
docker run --rm --entrypoint bash "${tag}" -c \
  'id -un && ls config.sh run.sh >/dev/null && echo "runner payload present"'

echo "==> done: ${tag}"
