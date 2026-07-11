#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Builds/refreshes the local CI image used by the self-hosted `claudio` Forgejo
#     runner (.forgejo/workflows/heavy.yml). Uses `docker` deliberately, NOT `podman`
#     -- the runner's label (`docker:docker://node:lts`, config.yaml on this machine)
#     drives jobs through the docker socket, so the image must land in docker's local
#     image store (`localhost/glintfx-ci:f42`) for `container.force_pull: false` (the
#     forgejo-runner default -- confirmed in this machine's config, no override
#     present) to find it without trying to pull from a registry that does not have it.
#     Re-run this script whenever tools/ci/Containerfile.f42 or its package list
#     changes; the docker build cache makes a no-op re-run near-instant.
# PT: Builda/atualiza a imagem local de CI usada pelo runner self-hosted `claudio` do
#     Forgejo (.forgejo/workflows/heavy.yml). Usa `docker` deliberadamente, NÃO
#     `podman` -- o label do runner (`docker:docker://node:lts`, config.yaml nesta
#     máquina) leva os jobs pelo socket do docker, então a imagem precisa cair no
#     armazenamento local de imagens do docker (`localhost/glintfx-ci:f42`) para o
#     `container.force_pull: false` (default do forgejo-runner -- confirmado no
#     config.yaml desta máquina, sem override presente) achá-la sem tentar puxar de um
#     registry que não a tem. Re-execute este script sempre que
#     tools/ci/Containerfile.f42 ou sua lista de pacotes mudar; o cache de build do
#     docker deixa uma re-execução no-op quase instantânea.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
tag="localhost/glintfx-ci:f42"

echo "==> building ${tag} from tools/ci/Containerfile.f42"
docker build -f "${repo_root}/tools/ci/Containerfile.f42" -t "${tag}" "${repo_root}"

echo "==> sanity check"
docker run --rm "${tag}" cat /etc/glintfx-ci-image
docker run --rm "${tag}" bash -c "node --version && clang --version | head -1 && cmake --version | head -1 && nasm -v"

echo "==> done: ${tag}"
