#!/usr/bin/env sh
# SPDX-License-Identifier: MPL-2.0
# EN: SOV-LIBCORE (ADR-0009, FT-F0) gate 4: verifies `build/libcore.a`'s EXPORTED (global,
#     defined) symbol surface is EXACTLY what the physical internalization boundary promises --
#     a POSITIVE WHITELIST, fail-secure by construction: every defined global symbol in the
#     archive must be explicitly allowed, or this script FAILS, naming the offender. Run by
#     `make libcore-test`, after `build/libcore.a` exists.
#
#     SECURITY-ENGINEER FINDING (post-decb2fb review, CRITICAL, this script is the fix): the
#     PRIOR version of this script was a BLACKLIST of 7 fixed names
#     (malloc/free/realloc/memcpy/memset/memmove/memcmp) -- it reported "OK" even though
#     src/str.c/src/conv.c/src/printf.c's libc-shaped names (`strlen`/`strcmp`/.../`atoi`/`atof`/
#     .../`mini_printf`/`mini_vsnprintf`) were exported BARE in the archive (the Makefile's
#     `wildcard src/*.c` archives every src/*.c file, but the old `$(CORE_RENAME_FLAGS)` only
#     renamed 7 of the many libc-shaped names actually present) -- a hosted C++ consumer linking
#     `libcore.a` would silently hijack glibc's own `strlen`/`atoi`/etc. for the WHOLE process,
#     proven live. A blacklist is structurally the wrong shape for this gate: it only catches
#     collisions someone thought to list in advance, and silently PASSES ("OK") on anything new
#     (a symbol added to src/*.c tomorrow and forgotten here). This whitelist inverts that: it
#     enumerates every symbol name this archive is ALLOWED to export (the `glx_*` front door +
#     the renamed `glx_core_*_impl` internals + the small ASM/wrapper set that was never
#     libc-shaped to begin with), and treats ANY OTHER defined global symbol as a failure --
#     including a str/conv/printf-shaped name someone forgets to add to
#     `$(CORE_RENAME_FLAGS)` in the future. Fail-secure: deny by default, allow explicitly.
# PT: SOV-LIBCORE (ADR-0009, FT-F0) gate 4: verifica que a superfície de símbolo EXPORTADA
#     (global, definida) da `build/libcore.a` é EXATAMENTE o que a fronteira física de
#     internalização promete -- uma WHITELIST POSITIVA, fail-secure por construção: todo símbolo
#     global definido no archive precisa estar explicitamente permitido, ou este script FALHA,
#     nomeando o infrator. Rodado pelo `make libcore-test`, depois que `build/libcore.a` existe.
#
#     ACHADO DO SECURITY-ENGINEER (revisão pós-decb2fb, CRÍTICO, este script É o fix): a versão
#     ANTERIOR deste script era uma LISTA NEGRA de 7 nomes fixos
#     (malloc/free/realloc/memcpy/memset/memmove/memcmp) -- reportava "OK" mesmo com os nomes no
#     formato de libc do src/str.c/src/conv.c/src/printf.c (`strlen`/`strcmp`/.../`atoi`/`atof`/
#     .../`mini_printf`/`mini_vsnprintf`) exportados CRUS no archive (o `wildcard src/*.c` do
#     Makefile arquiva todo src/*.c, mas o `$(CORE_RENAME_FLAGS)` antigo só renomeava 7 dos muitos
#     nomes no formato de libc de fato presentes) -- um consumidor C++ hosted linkando
#     `libcore.a` sequestraria em silêncio o `strlen`/`atoi`/etc. PRÓPRIO da glibc pro processo
#     INTEIRO, provado ao vivo. Uma lista negra é estruturalmente a forma errada pra este gate:
#     só pega colisões que alguém pensou em listar de antemão, e PASSA em silêncio ("OK") em
#     qualquer coisa nova (um símbolo acrescentado ao src/*.c amanhã e esquecido aqui). Esta
#     whitelist inverte isso: enumera todo nome de símbolo que este archive tem PERMISSÃO de
#     exportar (a porta-da-frente `glx_*` + os internos renomeados `glx_core_*_impl` + o pequeno
#     conjunto ASM/wrapper que nunca teve formato de libc pra começo de conversa), e trata
#     QUALQUER OUTRO símbolo global definido como falha -- inclusive um nome no formato de
#     str/conv/printf que alguém esqueça de acrescentar ao `$(CORE_RENAME_FLAGS)` no futuro.
#     Fail-secure: nega por padrão, permite explicitamente.
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

# EN: The public `glx_*` front door (include/core/core.h) -- REQUIRED to be present (missing one
#     means the archive is incomplete/broken, a separate failure mode from a collision but caught
#     by the same loop below). SOV-SFNT (FT-F1, include/core/sfnt.h) adds its own front door here
#     -- `glx_sfnt_open`/`glx_sfnt_glyph_id`/`glx_sfnt_hmetrics`/`glx_sfnt_glyph_outline` are
#     ALREADY namespaced by construction (no `$(CORE_RENAME_FLAGS)` entry needed, unlike
#     malloc/memcpy/str*/conv*/printf* -- see src/sfnt.c's file header), listed here explicitly
#     (not via a `glx_sfnt_*` glob pattern) so a future accidental internal helper leaking a bare
#     `glx_sfnt_`-prefixed symbol still fails this gate instead of silently riding along.
# PT: A porta-da-frente pública `glx_*` (include/core/core.h) -- OBRIGATÓRIA de estar presente
#     (faltar uma significa que o archive está incompleto/quebrado, um modo de falha separado de
#     uma colisão mas pego pelo mesmo laço abaixo). O SOV-SFNT (FT-F1, include/core/sfnt.h)
#     acrescenta a própria porta-da-frente aqui -- `glx_sfnt_open`/`glx_sfnt_glyph_id`/
#     `glx_sfnt_hmetrics`/`glx_sfnt_glyph_outline` já são namespaced por construção (nenhuma
#     entrada em `$(CORE_RENAME_FLAGS)` necessária, diferente de malloc/memcpy/str*/conv*/printf*
#     -- ver o cabeçalho de arquivo do src/sfnt.c), listados aqui explicitamente (não via padrão
#     glob `glx_sfnt_*`) pra que um futuro helper interno acidental vazando um símbolo cru
#     prefixado com `glx_sfnt_` ainda falhe este gate em vez de viajar junto em silêncio.
# EN: SOV-RAST (FT-F2) adds its own front door here -- `glx_rasterize_outline`/
#     `glx_raster_scratch_floats` are ALREADY namespaced by construction (no `$(CORE_RENAME_FLAGS)`
#     entry needed, same reasoning as SOV-SFNT's own `glx_sfnt_*` names above -- see
#     src/raster.c's file header), listed explicitly (not via a `glx_raster_*`/`glx_rasterize_*`
#     glob) so a future accidental internal helper leaking a bare-prefixed symbol still fails this
#     gate instead of silently riding along.
# PT: O SOV-RAST (FT-F2) acrescenta a própria porta-da-frente aqui -- `glx_rasterize_outline`/
#     `glx_raster_scratch_floats` já são namespaced por construção (nenhuma entrada em
#     `$(CORE_RENAME_FLAGS)` necessária, mesmo raciocínio dos próprios nomes `glx_sfnt_*` do
#     SOV-SFNT acima -- ver o cabeçalho de arquivo do src/raster.c), listados explicitamente (não
#     via um glob `glx_raster_*`/`glx_rasterize_*`) pra que um futuro helper interno acidental
#     vazando um símbolo cru com o mesmo prefixo ainda falhe este gate em vez de viajar junto em
#     silêncio.
required="glx_malloc glx_free glx_realloc glx_memcpy glx_memset glx_sfnt_open glx_sfnt_glyph_id glx_sfnt_hmetrics glx_sfnt_glyph_outline glx_rasterize_outline glx_raster_scratch_floats"

# EN: Everything else this archive is allowed to export: the small ASM/wrapper set that was never
#     libc-shaped to begin with (_start, the syscall*N* trampolines, the sys_* one-line wrappers)
#     -- present OPTIONALLY (they are real, expected members, not merely tolerated, but this
#     script does not treat their absence as a failure the way it does for the `required` front
#     door above; a future refactor moving them around should not need to touch this list).
# PT: Tudo mais que este archive tem permissão de exportar: o pequeno conjunto ASM/wrapper que
#     nunca teve formato de libc pra começo de conversa (_start, os trampolins syscall*N*, os
#     wrappers de 1 linha sys_*) -- presentes OPCIONALMENTE (são membros reais, esperados, não só
#     tolerados, mas este script não trata a ausência deles como falha do jeito que trata a
#     porta-da-frente `required` acima; um refactor futuro que mova eles não deveria precisar
#     tocar esta lista).
allowed_extra="_start syscall1 syscall2 syscall3 syscall6 sys_exit sys_write sys_read sys_mmap sys_munmap"

is_allowed() {
  name="$1"
  # EN: The renamed-internals family -- every libc-shaped identifier in src/*.c that
  #     $(CORE_RENAME_FLAGS) (Makefile) renames to `glx_core_<name>_impl` lands here. Shell `case`
  #     glob match (POSIX, no external process) -- matches any name starting with `glx_core_` and
  #     ending with `_impl`.
  # PT: A família de internos renomeados -- todo identificador no formato de libc em src/*.c que
  #     o $(CORE_RENAME_FLAGS) (Makefile) renomeia pra `glx_core_<nome>_impl` cai aqui. Match de
  #     glob do `case` do shell (POSIX, sem processo externo) -- casa qualquer nome começando com
  #     `glx_core_` e terminando com `_impl`.
  case "$name" in
    glx_core_*_impl) return 0 ;;
  esac
  for a in $required $allowed_extra; do
    if [ "$name" = "$a" ]; then
      return 0
    fi
  done
  return 1
}

# EN: Deny-by-default pass: every defined global symbol name must be explicitly allowed above, or
#     it is a COLLISION -- this is what makes the check fail-secure (a forgotten rename, or a
#     brand-new libc-shaped name added to src/*.c tomorrow, fails loudly here instead of silently
#     riding along as it would under the old 7-name blacklist).
# PT: Passada nega-por-padrão: todo nome de símbolo global definido precisa estar explicitamente
#     permitido acima, ou é uma COLISÃO -- é isso que torna a checagem fail-secure (um rename
#     esquecido, ou um nome novo no formato de libc acrescentado ao src/*.c amanhã, falha alto
#     aqui em vez de viajar junto em silêncio como aconteceria sob a lista negra de 7 nomes
#     antiga).
for name in $defined_names; do
  if ! is_allowed "$name"; then
    echo "COLLISION: unexpected exported symbol '$name' is DEFINED by $LIB (not in the glx_*/glx_core_*_impl/ASM-wrapper whitelist)" >&2
    status=1
  fi
done

for r in $required; do
  if ! echo "$defined_names" | grep -qx "$r"; then
    echo "MISSING: public symbol '$r' is NOT defined/exported by $LIB" >&2
    status=1
  fi
done

if [ "$status" -eq 0 ]; then
  echo "check_libcore_symbols: OK (glx_* front door present, every defined symbol matches the positive whitelist -- no bare libc-shaped name exported)"
else
  echo "check_libcore_symbols: FAILED"
fi
exit "$status"
