#!/usr/bin/env sh
# SPDX-License-Identifier: MPL-2.0
# EN: Verifies every code file (.c, .h, .asm, .inc, .sh, Makefile) under src/, include/,
#     tests/, tools/, and the top-level Makefile carries the SPDX header on one of its first
#     5 lines. Scoped to Layer 0 (loucura_c_asm) only -- does NOT walk glintfx/ (Layer 1 has
#     its own tooling and convention, already enforced). Exits non-zero and lists offenders if
#     any file is missing it. Reused later, unmodified, by TST-STATIC (TODO.md, W11).
#
#     TST-STATIC hardening (sub-b, TODO.md W11): the ORIGINAL version of this script treated
#     "zero files found" as a PASS ("check_spdx: OK (0 files found)", exit 0). That is "green by
#     vacuity" (feedback_auditoria_domino): if src/include/tests/tools were ever deleted, moved,
#     or a typo crept into `find`'s path list, the gate would keep reporting OK forever -- silent
#     total loss of coverage, indistinguishable from "everything already has SPDX". Two
#     independent checks now guard against that: (1) every expected directory must actually
#     exist BEFORE `find` runs (a missing directory is a hard FAIL, not a shrinking scan), and
#     (2) the scan must find at least one file, or it is also a hard FAIL. `set -eu` was already
#     present; it alone does not catch this class of bug (a `find` over existing-but-empty
#     directories exits 0 with empty output -- `-eu` has nothing to trip on).
# PT: Verifica se todo arquivo de código (.c, .h, .asm, .inc, .sh, Makefile) sob src/,
#     include/, tests/, tools/ e o Makefile de topo carrega o header SPDX numa das primeiras
#     5 linhas. Escopo restrito à Camada 0 (loucura_c_asm) -- NÃO varre glintfx/ (a Camada 1
#     tem tooling e convenção próprios, já aplicados). Sai com código não-zero e lista os
#     infratores se algum arquivo estiver sem. Reusado depois, sem mudança, pelo TST-STATIC
#     (TODO.md, W11).
#
#     Endurecimento do TST-STATIC (sub-b, TODO.md W11): a versão ORIGINAL deste script tratava
#     "zero arquivos encontrados" como PASSOU ("check_spdx: OK (0 files found)", exit 0). Isso é
#     "verde por vacuidade" (feedback_auditoria_domino): se src/include/tests/tools fossem
#     deletadas, movidas, ou um typo entrasse na lista de caminhos do `find`, o gate continuaria
#     reportando OK pra sempre -- perda total de cobertura em silêncio, indistinguível de "tudo
#     já tem SPDX". Duas checagens independentes agora guardam contra isso: (1) toda pasta
#     esperada precisa realmente existir ANTES do `find` rodar (pasta ausente é FALHA dura, não
#     um scan encolhido), e (2) o scan precisa achar pelo menos um arquivo, senão também é FALHA
#     dura. O `set -eu` já existia; sozinho ele não pega essa classe de bug (um `find` sobre
#     pastas existentes-mas-vazias sai 0 com saída vazia -- `-eu` não tem em que tropeçar).
# Copyright (c) 2026 Petrus Silva Costa
set -eu

SCAN_DIRS="src include tests tools"

for d in $SCAN_DIRS; do
  if [ ! -d "$d" ]; then
    echo "check_spdx: FAILED (expected directory missing: $d -- refusing to scan a shrunk tree)" >&2
    exit 1
  fi
done

missing=0
files=$(find $SCAN_DIRS -type f \
        \( -name '*.c' -o -name '*.h' -o -name '*.asm' -o -name '*.inc' -o -name '*.sh' \))
[ -f Makefile ] && files="$files Makefile"

if [ -z "$files" ]; then
  echo "check_spdx: FAILED (0 files found -- refusing to pass vacuously, see file header)" >&2
  exit 1
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
