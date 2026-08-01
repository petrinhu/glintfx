#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: Builds/refreshes localhost/glintfx-ci:f42, the local image used to reproduce
#     glintfx's heavy checks (ASan/LSan/UBSan, own-font-engine build) on a Fedora 42
#     base BY HAND. There is no CI runner on this machine any more -- the self-hosted
#     `claudio` Forgejo runner was shut down on 2026-07-25 and `.forgejo/` was deleted
#     from the repository -- so this script and a plain `docker run` are the whole
#     local-reproduction story now. Uses `docker` (not `podman`) so the image lands in
#     docker's local image store under the `localhost/` prefix. Re-run whenever
#     tools/ci/Containerfile.f42 or its package list changes; the docker build cache
#     makes a no-op re-run near-instant.
#
#     Typical use after building:
#       docker run --rm -v "$PWD":/src -w /src localhost/glintfx-ci:f42 bash -c '<cmd>'
#
# PT: Builda/atualiza a localhost/glintfx-ci:f42, a imagem local usada pra reproduzir
#     À MÃO os checks pesados da glintfx (ASan/LSan/UBSan, build do motor de fonte
#     próprio) sobre uma base Fedora 42. Não existe mais runner de CI nesta máquina --
#     o runner self-hosted `claudio` do Forgejo foi desligado em 2026-07-25 e o
#     `.forgejo/` foi apagado do repositório -- então este script mais um `docker run`
#     comum são toda a história de reprodução local agora. Usa `docker` (não `podman`)
#     pra imagem cair no armazenamento local de imagens do docker sob o prefixo
#     `localhost/`. Re-execute sempre que tools/ci/Containerfile.f42 ou sua lista de
#     pacotes mudar; o cache de build do docker deixa uma re-execução no-op quase
#     instantânea.
#
#     Uso típico depois de buildar:
#       docker run --rm -v "$PWD":/src -w /src localhost/glintfx-ci:f42 bash -c '<cmd>'
#
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
tag="localhost/glintfx-ci:f42"

echo "==> building ${tag} from tools/ci/Containerfile.f42"
docker build -f "${repo_root}/tools/ci/Containerfile.f42" -t "${tag}" "${repo_root}"

echo "==> sanity check"
docker run --rm "${tag}" cat /etc/glintfx-ci-image
docker run --rm "${tag}" bash -c "node --version && clang --version | head -1 && cmake --version | head -1 && nasm -v"

echo "==> done: ${tag}"
