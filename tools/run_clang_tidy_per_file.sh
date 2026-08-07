#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: TST-L1-STATIC (clang-tidy leg) -- runs clang-tidy ONE FILE AT A TIME instead of a
#     single multi-file invocation (CI-TIDY-CRASH, líder's decision 2, 2026-08-06).
#
#     Why: `clang-tidy -p <build-dir> src/**/*.cpp` dies on the FIRST crash and silently
#     drops every file after it from the report -- lost coverage, not a red gate, and
#     nobody can tell how many of the 51 files were actually analyzed vs skipped. A
#     single crash killing the whole job is a KNOWN issue on this repo: the checked-in
#     word "nós" (UTF-8, glintfx/src/rml/rcss_dump_corpus_sanity.cpp) trips a clang-tidy
#     18 bug (llvm/llvm-project#169198, fixed by #169215 -- modernize-use-std-print's
#     FormatStringConverter treats a UTF-8 continuation byte as a NEGATIVE signed char,
#     passes the `< 32` control-char test, then indexes llvm::hexdigit() out of bounds).
#     Running one file per invocation turns any FUTURE crash like this into "file X died,
#     the other 50 got analyzed" instead of "the whole job died, coverage unknown".
#
#     This script does NOT decide whether a crash is acceptable -- it just makes sure a
#     crash on file N does not hide files N+1..last, and that the summary always states
#     how many files were actually analyzed (even when that number is zero), because a
#     declared zero is how you tell "nothing broke" from "nobody looked" (repo-wide
#     scope-line convention, see e.g. rcss_dump_corpus_sanity.cpp's own scope line).
#
# PT: TST-L1-STATIC (perna clang-tidy) -- roda o clang-tidy UM ARQUIVO POR VEZ em vez de
#     uma única chamada multi-arquivo (CI-TIDY-CRASH, decisão 2 do líder, 2026-08-06).
#
#     Por quê: `clang-tidy -p <build-dir> src/**/*.cpp` morre no PRIMEIRO crash e derruba
#     em silêncio todo arquivo depois dele do relatório -- cobertura perdida, não um gate
#     vermelho, e ninguém sabe quantos dos 51 arquivos foram de fato analisados versus
#     pulados. Um crash único matando o job inteiro é um problema CONHECIDO neste repo: a
#     palavra "nós" (UTF-8, glintfx/src/rml/rcss_dump_corpus_sanity.cpp) dispara um bug do
#     clang-tidy 18 (llvm/llvm-project#169198, corrigido pelo #169215 -- o
#     FormatStringConverter do modernize-use-std-print trata um byte de continuação UTF-8
#     como char assinado NEGATIVO, passa no teste `< 32` de caractere-de-controle, e então
#     indexa llvm::hexdigit() fora dos limites). Rodar um arquivo por chamada transforma
#     qualquer crash FUTURO parecido em "o arquivo X morreu, os outros 50 foram
#     analisados" em vez de "o job inteiro morreu, cobertura desconhecida".
#
#     Este script NÃO decide se um crash é aceitável -- só garante que um crash no
#     arquivo N não esconde os arquivos N+1..último, e que o resumo sempre declara
#     quantos arquivos foram de fato analisados (mesmo quando esse número é zero), porque
#     um zero declarado é o que distingue "nada quebrou" de "ninguém olhou" (convenção da
#     linha de escopo usada no repo inteiro, ver a própria linha de escopo de
#     rcss_dump_corpus_sanity.cpp).
#
# Usage: tools/run_clang_tidy_per_file.sh [build-dir] [src-root]
#   build-dir  default: glintfx/build-lint  (must contain compile_commands.json)
#   src-root   default: glintfx/src         (recursed with `find -name '*.cpp'`)
#
# Exit code: 0 if every file was analyzed with exit 0; 1 if any file crashed and/or
#            reported a lint violation and/or the analyzed count didn't match the
#            number of files found (that last case is itself a bug in this script, not
#            in clang-tidy, and must never happen silently).
set -uo pipefail # EN: no -e -- we must survive a per-file crash to reach the summary.
                 # PT: sem -e -- precisamos sobreviver a um crash por-arquivo para chegar no resumo.

BUILD_DIR="${1:-glintfx/build-lint}"
SRC_ROOT="${2:-glintfx/src}"

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "error: '$BUILD_DIR/compile_commands.json' not found -- configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first" >&2
  exit 2
fi
if [[ ! -d "$SRC_ROOT" ]]; then
  echo "error: src root '$SRC_ROOT' not found (run from repo root)" >&2
  exit 2
fi

# EN: `find` recurses correctly into any depth (src/fx/*.cpp, src/rml/*.cpp, ...) --
#     the exact bug class (flat `src/*.cpp` glob silently skipping nested dirs) that bit
#     FX-CARVE-1's CI integration is what motivated the recursive `src/**/*.cpp` glob
#     this script replaces; `find` gets the same recursion without depending on the
#     shell's `globstar` option being set by the caller.
# PT: `find` recursa corretamente em qualquer profundidade (src/fx/*.cpp, src/rml/*.cpp,
#     ...) -- a mesma classe de bug (glob plano `src/*.cpp` pulando dirs aninhados em
#     silêncio) que pegou a integração de CI do FX-CARVE-1 foi o que motivou o glob
#     recursivo `src/**/*.cpp` que este script substitui; `find` obtém a mesma recursão
#     sem depender da opção `globstar` do shell estar ligada por quem chama.
mapfile -t files < <(find "$SRC_ROOT" -type f -name '*.cpp' | sort)
found_count="${#files[@]}"

analyzed=0
declare -a crashed=()
declare -a violated=()
declare -a other_failed=()

for f in "${files[@]}"; do
  echo "--- clang-tidy: $f ---"
  clang-tidy -p "$BUILD_DIR" "$f"
  rc=$?
  analyzed=$((analyzed + 1))
  if [[ "$rc" -ge 128 ]]; then
    crashed+=("$f (exit=$rc)")
  elif [[ "$rc" -eq 1 ]]; then
    violated+=("$f (exit=$rc)")
  elif [[ "$rc" -ne 0 ]]; then
    other_failed+=("$f (exit=$rc)")
  fi
done

failed_count=$(( ${#crashed[@]} + ${#violated[@]} + ${#other_failed[@]} ))

# EN: The scope line -- printed even when every count is zero (repo-wide convention,
#     see rcss_dump_corpus_sanity.cpp's own comment on this).
# PT: A linha de escopo -- impressa mesmo quando todas as contagens são zero (convenção
#     do repo inteiro, ver o próprio comentário disso em rcss_dump_corpus_sanity.cpp).
echo
echo "=== TST-L1-STATIC clang-tidy (per-file) resumo/summary ==="
echo "encontrados/found: $found_count"
echo "analisados/analyzed: $analyzed"
echo "falharam/failed: $failed_count"

if [[ "${#crashed[@]}" -gt 0 ]]; then
  echo "  crashes (exit >= 128, ferramenta/tool):"
  printf '    %s\n' "${crashed[@]}"
fi
if [[ "${#violated[@]}" -gt 0 ]]; then
  echo "  violações de lint (exit == 1, nosso código/our code):"
  printf '    %s\n' "${violated[@]}"
fi
if [[ "${#other_failed[@]}" -gt 0 ]]; then
  echo "  outras falhas (exit != 0/1/>=128):"
  printf '    %s\n' "${other_failed[@]}"
fi

# EN: This is a bug in THIS script (or in `find`/the filesystem), never expected to
#     trigger from clang-tidy's own behaviour -- fail loud instead of reporting a
#     mismatched "analyzed" count as if it were trustworthy.
# PT: Isto é um bug DESTE script (ou do `find`/sistema de arquivos), nunca esperado a
#     partir do comportamento do próprio clang-tidy -- falhar alto em vez de reportar
#     uma contagem "analisados" divergente como se fosse confiável.
if [[ "$analyzed" -ne "$found_count" ]]; then
  echo "FAIL: analisados ($analyzed) != encontrados ($found_count) -- bug no laço, não no clang-tidy" >&2
  exit 1
fi

if [[ "$failed_count" -gt 0 ]]; then
  echo "FAIL: $failed_count de $analyzed arquivo(s) falharam -- ver lista acima" >&2
  exit 1
fi

echo "OK: $analyzed/$found_count arquivos analisados, 0 falhas"
exit 0
