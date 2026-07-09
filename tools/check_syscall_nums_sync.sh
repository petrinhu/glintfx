#!/usr/bin/env sh
# SPDX-License-Identifier: MPL-2.0
# EN: TST-STATIC sub-a (TODO.md, W11 / AUDIT_FIND.md AUD-C0-*): grep-diffs the two hand-kept
#     mirrors of syscall number constants -- include/syscall_nums.h (C `#define NAME VALUE`)
#     and include/syscall_nums.inc (NASM `%define NAME VALUE`) -- and FAILS if a constant
#     present in BOTH files has a DIFFERENT value (real drift: someone edited one copy and
#     forgot the other). A constant present in only ONE file is NOT by itself a failure: the
#     .h intentionally carries SYS_mmap while the .inc does not (see both files' own header
#     comments -- no NASM source calls mmap, E1's allocator is pure C over sys_mmap.c), so
#     asymmetric coverage is a documented, deliberate YAGNI choice, not a bug. What this script
#     actually guards against is the value itself silently drifting apart for a name that both
#     files agree should exist -- e.g. if `SYS_write` became `#define SYS_write 1` in one file
#     and `%define SYS_write 2` in the other, every write() call in ASM-linked code would use
#     the wrong syscall number with zero compiler diagnostic (the two languages never
#     cross-check each other's constants at build time).
# PT: TST-STATIC sub-a (TODO.md, W11 / AUDIT_FIND.md AUD-C0-*): faz grep-diff dos dois espelhos
#     mantidos a mão das constantes de número de syscall -- include/syscall_nums.h (`#define
#     NOME VALOR` em C) e include/syscall_nums.inc (`%define NOME VALOR` em NASM) -- e FALHA se
#     uma constante presente nos DOIS arquivos tiver um valor DIFERENTE (drift real: alguém
#     editou uma cópia e esqueceu a outra). Uma constante presente em só UM dos arquivos NÃO é,
#     por si só, uma falha: o `.h` carrega `SYS_mmap` de propósito enquanto o `.inc` não (ver os
#     próprios comentários de cabeçalho dos dois arquivos -- nenhuma fonte NASM chama mmap, o
#     alocador do E1 é C puro sobre sys_mmap.c), então cobertura assimétrica é uma escolha YAGNI
#     documentada e deliberada, não um bug. O que este script realmente guarda é o VALOR em si
#     driftando em silêncio pra um nome que os dois arquivos concordam que deveria existir --
#     ex.: se `SYS_write` virasse `#define SYS_write 1` num arquivo e `%define SYS_write 2` no
#     outro, toda chamada write() em código linkado com ASM usaria o número de syscall errado
#     com zero diagnóstico do compilador (as duas linguagens nunca conferem as constantes uma da
#     outra em tempo de build).
# Copyright (c) 2026 Petrus Silva Costa
set -eu

H_FILE="include/syscall_nums.h"
INC_FILE="include/syscall_nums.inc"

for f in "$H_FILE" "$INC_FILE"; do
  if [ ! -f "$f" ]; then
    echo "check_syscall_nums_sync: ERROR - required file missing: $f" >&2
    exit 1
  fi
done

# EN: Extract "NAME VALUE" pairs, one per line, sorted by name. `#define NAME VALUE` (C) and
#     `%define NAME VALUE` (NASM) share the same "<directive> NAME VALUE" shape after the sigil,
#     so a single sed pattern per file suffices; trailing comments/whitespace are trimmed.
# PT: Extrai pares "NOME VALOR", um por linha, ordenados por nome. `#define NOME VALOR` (C) e
#     `%define NOME VALOR` (NASM) compartilham o mesmo formato "<diretiva> NOME VALOR" depois do
#     sinal, então um único padrão sed por arquivo basta; comentários/espaços finais são
#     cortados.
h_pairs=$(sed -n 's/^#define[[:space:]]\+\([A-Za-z_][A-Za-z0-9_]*\)[[:space:]]\+\([0-9][0-9]*\).*/\1 \2/p' "$H_FILE" | sort)
inc_pairs=$(sed -n 's/^%define[[:space:]]\+\([A-Za-z_][A-Za-z0-9_]*\)[[:space:]]\+\([0-9][0-9]*\).*/\1 \2/p' "$INC_FILE" | sort)

if [ -z "$h_pairs" ]; then
  echo "check_syscall_nums_sync: ERROR - zero '#define NAME VALUE' pairs parsed from $H_FILE (refusing to pass vacuously)" >&2
  exit 1
fi
if [ -z "$inc_pairs" ]; then
  echo "check_syscall_nums_sync: ERROR - zero '%define NAME VALUE' pairs parsed from $INC_FILE (refusing to pass vacuously)" >&2
  exit 1
fi

drift=0
# EN: For every name the .inc declares, it MUST exist in the .h with the SAME value -- the .inc
#     is the smaller, intentionally-partial mirror (see file header), so it can never legitimately
#     have a name the .h lacks, and every shared name must agree numerically.
# PT: Pra todo nome que o `.inc` declara, ele PRECISA existir no `.h` com o MESMO valor -- o
#     `.inc` é o espelho menor, intencionalmente parcial (ver cabeçalho do arquivo), então nunca
#     pode legitimamente ter um nome que o `.h` não tem, e todo nome compartilhado precisa
#     concordar numericamente.
echo "$inc_pairs" | while IFS=' ' read -r name inc_val; do
  h_val=$(echo "$h_pairs" | awk -v n="$name" '$1==n{print $2}')
  if [ -z "$h_val" ]; then
    echo "DRIFT: $name defined in $INC_FILE (=$inc_val) but MISSING from $H_FILE" >&2
    exit 1
  fi
  if [ "$h_val" != "$inc_val" ]; then
    echo "DRIFT: $name = $h_val in $H_FILE but = $inc_val in $INC_FILE" >&2
    exit 1
  fi
done || drift=1

if [ "$drift" -ne 0 ]; then
  echo "check_syscall_nums_sync: FAILED"
  exit 1
fi

echo "check_syscall_nums_sync: OK ($(echo "$inc_pairs" | wc -l | tr -d ' ') shared constant(s) verified, $H_FILE is the superset)"
