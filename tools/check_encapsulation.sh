#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Encapsulation gate for glintfx public headers (TST-L1-STATIC, sub-check).
#     Fails if any third-party type/header (RmlUi, GLFW, gl3w, raw GL/, stb, SDL) leaks
#     into the public API surface (glintfx/include/glintfx/*.hpp). Consumers must be
#     able to include glintfx headers without pulling in third-party types.
#
#     Deliberately grep-based on `#include` DIRECTIVES, not on raw substrings in the
#     whole file: the macro GLINTFX_BACKEND_GLFW (config.hpp / glintfx.hpp) contains
#     the substring "GLFW" but is not an include of a GLFW header, so it must NOT
#     trigger this gate. See CLAUDE.md / AGENTS.md "Gotchas críticos -- gate de
#     encapsulamento: grep include-based, não nome cru de macro".
#
# PT: Gate de encapsulamento para os headers públicos do glintfx (TST-L1-STATIC,
#     sub-check). Falha se algum header/tipo de terceiro (RmlUi, GLFW, gl3w, GL/ cru,
#     stb, SDL) vazar para a superfície pública (glintfx/include/glintfx/*.hpp).
#     Consumidores precisam poder incluir os headers do glintfx sem puxar tipos de
#     terceiro junto.
#
#     Deliberadamente baseado em grep de DIRETIVAS `#include`, não em substring crua
#     do arquivo inteiro: a macro GLINTFX_BACKEND_GLFW (config.hpp / glintfx.hpp) contém
#     a substring "GLFW" mas não é um include de header GLFW, então NÃO deve disparar
#     este gate. Ver CLAUDE.md / AGENTS.md "Gotchas críticos -- gate de encapsulamento:
#     grep include-based, não nome cru de macro".
#
# Usage: tools/check_encapsulation.sh [path-to-public-header-dir]
#        (default: glintfx/include/glintfx, relative to repo root / CWD)
set -euo pipefail

PUBLIC_HEADER_DIR="${1:-glintfx/include/glintfx}"

# EN: Matches an #include directive whose argument contains a forbidden third-party
#     token. Anchored to the include directive itself (not free text in the file).
# PT: Casa uma diretiva #include cujo argumento contém um token proibido de terceiro.
#     Ancorado na própria diretiva de include (não em texto livre do arquivo).
FORBIDDEN_PATTERN='^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^>"]*(Rml|glfw|GLFW|gl3w|GL/|stb|SDL)[^>"]*[>"]'

if [[ ! -d "$PUBLIC_HEADER_DIR" ]]; then
  echo "error: public header dir '$PUBLIC_HEADER_DIR' not found (run from repo root)" >&2
  exit 2
fi

violations_found=0

while IFS= read -r -d '' header; do
  matches="$(grep -nE "$FORBIDDEN_PATTERN" "$header" || true)"
  if [[ -n "$matches" ]]; then
    echo "encapsulation violation in $header:" >&2
    echo "$matches" >&2
    violations_found=1
  fi
done < <(find "$PUBLIC_HEADER_DIR" -type f -name '*.hpp' -print0 | sort -z)

if [[ "$violations_found" -ne 0 ]]; then
  echo "FAIL: third-party header leaked into glintfx public API ($PUBLIC_HEADER_DIR) -- see violations above" >&2
  echo "FAIL: header de terceiro vazou na API pública do glintfx ($PUBLIC_HEADER_DIR) -- ver violações acima" >&2
  exit 1
fi

echo "OK: no third-party #include leaked into $PUBLIC_HEADER_DIR"
echo "OK: nenhum #include de terceiro vazou em $PUBLIC_HEADER_DIR"
