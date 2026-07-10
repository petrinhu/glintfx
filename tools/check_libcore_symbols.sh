#!/usr/bin/env sh
# SPDX-License-Identifier: MPL-2.0
# EN: SOV-LIBCORE (ADR-0009, FT-F0) gate 4: verifies `build/libcore.a`'s EXPORTED (global,
#     defined) symbol surface is exactly what the physical internalization boundary promises --
#     the `glx_*` front door present, and NONE of the bare libc-shaped names
#     (`malloc`/`free`/`realloc`/`memcpy`/`memset`/`memmove`/`memcmp`) that src/core_api.c's
#     `#define`-rename mechanism (see that file's header) is supposed to have eliminated from
#     every archived object. A collision here would mean a hosted consumer statically linking
#     `libcore.a` risks silently overriding glibc's own allocator/memory-primitive symbols for
#     the WHOLE process (ADR-0009's own words: "the `glx_` prefix handles [symbol collision]
#     trivially" -- this script is what actually PROVES that, rather than merely asserting it).
#     Run by `make libcore-test`, after `build/libcore.a` exists.
# PT: SOV-LIBCORE (ADR-0009, FT-F0) gate 4: verifica que a superfície de símbolo EXPORTADA
#     (global, definida) da `build/libcore.a` é exatamente o que a fronteira física de
#     internalização promete -- a porta-da-frente `glx_*` presente, e NENHUM dos nomes crus no
#     formato da libc (`malloc`/`free`/`realloc`/`memcpy`/`memset`/`memmove`/`memcmp`) que o
#     mecanismo de rename via `#define` do src/core_api.c (ver o cabeçalho daquele arquivo)
#     deveria ter eliminado de todo objeto archived. Uma colisão aqui significaria que um
#     consumidor hosted linkando estaticamente a `libcore.a` arrisca sobrepor em silêncio os
#     símbolos de alocador/primitiva-de-memória PRÓPRIOS da glibc pro processo INTEIRO (nas
#     palavras da própria ADR-0009: "o prefixo `glx_` resolve [colisão de símbolo] trivialmente"
#     -- este script é o que de fato PROVA isso, em vez de só afirmar). Rodado pelo `make
#     libcore-test`, depois que `build/libcore.a` existe.
# Copyright (c) 2026 Petrus Silva Costa
set -eu

LIB="build/libcore.a"

if [ ! -f "$LIB" ]; then
  echo "check_libcore_symbols: FAILED (missing $LIB -- run 'make libcore' first)" >&2
  exit 1
fi

# EN: Defined (T/D/B/R/W...) global symbol NAMES across every member of the archive -- one per
#     line. `nm -g --defined-only` output lines are "<addr> <type> <name>"; member-header lines
#     ("foo.o:") and blank separator lines have a different field count and are dropped by the
#     `NF==3` filter.
# PT: NOMES de símbolo global DEFINIDO (T/D/B/R/W...) em todo membro do archive -- um por linha.
#     As linhas de saída do `nm -g --defined-only` são "<endereço> <tipo> <nome>"; linhas de
#     cabeçalho de membro ("foo.o:") e linhas separadoras em branco têm contagem de campo
#     diferente e são descartadas pelo filtro `NF==3`.
defined_names=$(nm -g --defined-only "$LIB" | awk 'NF==3{print $3}')

if [ -z "$defined_names" ]; then
  echo "check_libcore_symbols: FAILED (0 defined global symbols found in $LIB -- refusing to pass vacuously)" >&2
  exit 1
fi

status=0

for forbidden in malloc free realloc memcpy memset memmove memcmp; do
  if echo "$defined_names" | grep -qx "$forbidden"; then
    echo "COLLISION: bare libc-shaped symbol '$forbidden' is DEFINED and exported by $LIB" >&2
    status=1
  fi
done

for required in glx_malloc glx_free glx_realloc glx_memcpy glx_memset; do
  if ! echo "$defined_names" | grep -qx "$required"; then
    echo "MISSING: public symbol '$required' is NOT defined/exported by $LIB" >&2
    status=1
  fi
done

if [ "$status" -eq 0 ]; then
  echo "check_libcore_symbols: OK (glx_* front door present, no bare malloc/free/realloc/memcpy/memset/memmove/memcmp exported)"
else
  echo "check_libcore_symbols: FAILED"
fi
exit "$status"
