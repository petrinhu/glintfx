#!/usr/bin/env sh
# SPDX-License-Identifier: MPL-2.0
# EN: Verifies every code file (.c, .h, .asm, .inc, .sh, Makefile) under src/, include/,
#     tests/, tools/, and the top-level Makefile carries the SPDX header on one of its first
#     5 lines. Scoped to Layer 0 (loucura_c_asm) only -- does NOT walk glintfx/ (Layer 1 has
#     its own tooling and convention, already enforced). Exits non-zero and lists offenders if
#     any file is missing it. Reused later, unmodified, by TST-STATIC (TODO.md, W11).
# PT: Verifica se todo arquivo de código (.c, .h, .asm, .inc, .sh, Makefile) sob src/,
#     include/, tests/, tools/ e o Makefile de topo carrega o header SPDX numa das primeiras
#     5 linhas. Escopo restrito à Camada 0 (loucura_c_asm) -- NÃO varre glintfx/ (a Camada 1
#     tem tooling e convenção próprios, já aplicados). Sai com código não-zero e lista os
#     infratores se algum arquivo estiver sem. Reusado depois, sem mudança, pelo TST-STATIC
#     (TODO.md, W11).
# Copyright (c) 2026 Petrus Silva Costa
set -eu

missing=0
files=$(find src include tests tools -type f \
        \( -name '*.c' -o -name '*.h' -o -name '*.asm' -o -name '*.inc' -o -name '*.sh' \) \
        2>/dev/null)
[ -f Makefile ] && files="$files Makefile"

if [ -z "$files" ]; then
  echo "check_spdx: OK (0 files found)"
  exit 0
fi

for f in $files; do
  if ! head -n 5 "$f" | grep -q 'SPDX-License-Identifier: MPL-2.0'; then
    echo "MISSING SPDX: $f"
    missing=1
  fi
done

if [ "$missing" -eq 0 ]; then
  echo "check_spdx: OK"
else
  echo "check_spdx: FAILED"
fi
exit "$missing"
