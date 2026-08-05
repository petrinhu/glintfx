#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# EN: RMLX-0/F4 -- RmlUi whitelist gate. Guards the confinement F1-F3 already bought:
#     with `#include <RmlUi/...>` moved into `glintfx/src/rml/` (F1) and `engine.cpp`/
#     `ui_layer.cpp` weaned off RmlUi entirely (F2/F3), nothing stops a NEW file from
#     reintroducing the include tomorrow and silently undoing three fatias of work. This
#     script is that stop.
#
#     Four checks, run in order (a/b/c blocking, d report-only):
#       (a) any `#include` of RmlUi in glintfx/src/, glintfx/include/, glintfx/demos/
#           OUTSIDE glintfx/src/rml/ -> FAIL (file:line).
#       (b) any `#include` of RmlUi in glintfx/tests/ outside the FROZEN whitelist of 4
#           files (domrw_sanity.cpp, focus_sanity.cpp, form_events_sanity.cpp,
#           document_reload_leak.cpp) -> FAIL. Caps the pre-existing test debt: a NEW
#           test that includes RmlUi directly fails the gate.
#       (c) value-type tokens (Rml::String/Vector2/Colourb/Variant/Input/Log) in CODE
#           (comments stripped) outside src/rml/ -> FAIL (file:line).
#       (d) report-only, ALWAYS printed: count of files with residual opaque `Rml::`
#           usage (forward-declared pointers -- Rml::Context*, Rml::SystemInterface* --
#           i.e. anything NOT already caught by (c)'s value-type list) outside the
#           whitelist. Format: "divida opaca: N arquivos". Printed even when N is 0 --
#           "zero declared" proves someone looked, an absent line does not.
#
#     Deliberately grep-based on `#include` DIRECTIVES for (a)/(b), never on a raw
#     substring of the whole file -- same doctrine as tools/check_encapsulation.sh
#     (GLINTFX_BACKEND_GLFW contains the substring "GLFW" but is not a GLFW header
#     include, so a naive grep must not trip on it; the analogous risk here would be a
#     macro or identifier containing "Rml" that is not an #include).
#
#     Comment stripping for (c)/(d): glintfx's public headers carry DOZENS of legitimate
#     `Rml::` mentions in doc-comments explaining what a wrapper encapsulates (measured:
#     55 in ui_layer.hpp alone) -- a raw grep for the token list would false-positive on
#     every one of them. strip_comments() below removes both `//` line comments
#     (including one that starts mid-line, after real code -- a naive `^\s*//` anchor
#     misses exactly that case) and `/* ... */` block comments (state carried across
#     lines via the RML_WL_IN_BLOCK global), with minimal string/char-literal awareness
#     so a stray `//` or `/*` inside a `"..."`/`'...'` does not truncate real code.
#
#     KNOWN, FROZEN, DOCUMENTED EXCEPTION -- the RmlUi GLFW platform-backend adapter
#     pair (glintfx/src/app.cpp, glintfx/src/system_glfw_dedup.hpp): discovered while
#     building this gate (2026-08-04), NOT called out in the F4 brief, whose own quick
#     verification (`grep -rln '^#include <RmlUi' glintfx/src/ | grep -v
#     '^glintfx/src/rml/'`) is angle-bracket-only and does not match the quoted
#     `#include "RmlUi_Platform_GLFW.h"` these two files carry. RMLX-0/F1's own census
#     counted 19 files with a real RmlUi #include pre-F1; F1 moved exactly 17 into
#     src/rml/, leaving these 2 -- App owns the live GLFWwindow* that RmlUi's upstream
#     SystemInterface_GLFW needs at construction time (only available post
#     WindowGlfw::create()), so the platform-backend header has to reach app.cpp
#     directly, and system_glfw_dedup.hpp is App-mode's thin LogMessage-override
#     subclass of that same upstream type (LOGTHR-1, TODO.md) -- hence its literal
#     `Rml::Log::Type`/`Rml::String` signature, mandated by the base class it overrides,
#     not a leak of glintfx's own choosing. Both files are EXPLICITLY exempted below
#     (RML_INCLUDE_EXCEPTIONS for check (a); RML_TOKEN_EXCEPTIONS for check (c) --
#     system_glfw_dedup.hpp only, app.cpp has zero non-comment Rml:: token use).
#     Growing either list is a decision for the líder, not something this script's
#     author gets to do unilaterally by adding a path to satisfy a red run.
#
# PT: RMLX-0/F4 -- gate de whitelist do RmlUi. Guarda o confinamento que F1-F3 já
#     compraram: com o `#include <RmlUi/...>` movido para `glintfx/src/rml/` (F1) e
#     `engine.cpp`/`ui_layer.cpp` desmamados do RmlUi por completo (F2/F3), nada impede
#     um arquivo NOVO de reintroduzir o include amanhã e desfazer em silêncio três
#     fatias de trabalho. Este script é essa trava.
#
#     Quatro checagens, nesta ordem (a/b/c bloqueantes, d report-only):
#       (a) qualquer `#include` de RmlUi em glintfx/src/, glintfx/include/,
#           glintfx/demos/ FORA de glintfx/src/rml/ -> FAIL (arquivo:linha).
#       (b) qualquer `#include` de RmlUi em glintfx/tests/ fora da whitelist
#           CONGELADA de 4 arquivos (domrw_sanity.cpp, focus_sanity.cpp,
#           form_events_sanity.cpp, document_reload_leak.cpp) -> FAIL. Trava a dívida
#           pré-existente: um teste NOVO que inclua RmlUi direto reprova o gate.
#       (c) tokens de tipo-valor (Rml::String/Vector2/Colourb/Variant/Input/Log) em
#           CÓDIGO (comentário removido) fora de src/rml/ -> FAIL (arquivo:linha).
#       (d) report-only, SEMPRE impresso: contagem de arquivos com uso opaco residual
#           de `Rml::` (ponteiro via fwd-decl -- Rml::Context*, Rml::SystemInterface* --
#           ou seja, tudo que (c) NÃO já pega pela lista de tipo-valor) fora da
#           whitelist. Formato: "divida opaca: N arquivos". Impresso mesmo quando N é 0
#           -- "zero declarado" prova que alguém olhou, linha ausente não prova nada.
#
#     Deliberadamente baseado em grep de DIRETIVAS `#include` para (a)/(b), nunca em
#     substring crua do arquivo inteiro -- mesma doutrina do tools/check_encapsulation.sh
#     (GLINTFX_BACKEND_GLFW contém a substring "GLFW" mas não é include de header GLFW,
#     então um grep ingênuo não pode disparar nela; o risco análogo aqui seria uma macro
#     ou identificador contendo "Rml" que não é um #include).
#
#     Remoção de comentário para (c)/(d): os headers públicos da glintfx carregam
#     DEZENAS de menções legítimas a `Rml::` em doc-comment explicando o que um wrapper
#     encapsula (medido: 55 só em ui_layer.hpp) -- um grep cru na lista de tokens daria
#     falso positivo em cada uma. O strip_comments() abaixo remove tanto comentário de
#     linha `//` (inclusive um que começa NO MEIO da linha, depois de código de verdade
#     -- uma âncora ingênua `^\s*//` erra exatamente esse caso) quanto comentário de
#     bloco `/* ... */` (estado carregado entre linhas via o global
#     RML_WL_IN_BLOCK), com consciência mínima de literal de string/char pra um `//` ou
#     `/*` perdido dentro de um `"..."`/`'...'` não truncar código de verdade.
#
#     EXCEÇÃO CONHECIDA, CONGELADA E DOCUMENTADA -- o par adaptador de backend GLFW do
#     RmlUi (glintfx/src/app.cpp, glintfx/src/system_glfw_dedup.hpp): descoberta ao
#     construir este gate (2026-08-04), NÃO citada no brief da F4, cuja própria
#     verificação rápida (`grep -rln '^#include <RmlUi' glintfx/src/ | grep -v
#     '^glintfx/src/rml/'`) é só-angle-bracket e não casa o `#include
#     "RmlUi_Platform_GLFW.h"` (aspas) que estes dois arquivos carregam. O próprio censo
#     da RMLX-0/F1 contou 19 arquivos com #include real de RmlUi pré-F1; F1 moveu
#     exatamente 17 para src/rml/, deixando estes 2 -- App é dona do GLFWwindow* vivo
#     que o SystemInterface_GLFW upstream do RmlUi exige na construção (só disponível
#     pós WindowGlfw::create()), então o header do backend de plataforma precisa
#     alcançar app.cpp direto, e system_glfw_dedup.hpp é a subclasse fina de override de
#     LogMessage desse mesmo tipo upstream, modo App (LOGTHR-1, TODO.md) -- daí a
#     assinatura literal `Rml::Log::Type`/`Rml::String`, exigida pela classe-base que
#     ela sobrescreve, não um vazamento de escolha própria da glintfx. Os dois arquivos
#     são EXPLICITAMENTE isentos abaixo (RML_INCLUDE_EXCEPTIONS pro check (a);
#     RML_TOKEN_EXCEPTIONS pro check (c) -- só system_glfw_dedup.hpp, app.cpp tem zero
#     uso de token Rml:: fora de comentário). Aumentar qualquer uma das duas listas é
#     decisão do líder, não algo que o autor deste script decide sozinho pra deixar um
#     run vermelho passar.
#
# Usage / Uso:
#   tools/check_rml_whitelist.sh              # selftest, then the real check (repo root CWD)
#   tools/check_rml_whitelist.sh --selftest   # selftest only (self-contained, mktemp, no repo needed)
#
# Exit status: 0 = clean; 1 = a blocking violation was found ((a)/(b)/(c), or the
# selftest itself failed to prove both sides); 2 = usage/environment error.
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"

# -----------------------------------------------------------------------------
# EN: Config -- paths are relative to CWD, which must be the repo root (same
#     convention as tools/check_encapsulation.sh).
# PT: Config -- caminhos relativos ao CWD, que precisa ser a raiz do repo (mesma
#     convenção do tools/check_encapsulation.sh).
# -----------------------------------------------------------------------------
RML_SRC_ROOTS=(glintfx/src glintfx/include glintfx/demos)
RML_DIR="glintfx/src/rml"
RML_TESTS_DIR="glintfx/tests"

# EN: check (b) -- frozen whitelist of test FILES allowed to #include RmlUi directly.
#     Matched by full relative path (RML_TESTS_DIR-prefixed, same string `find` yields
#     for $f below) -- NOT by basename. RMLX-0 adversarial review proved the
#     basename-only match was a real gate hole: a NEW file at
#     glintfx/tests/subdir/domrw_sanity.cpp (same basename as the frozen debt entry,
#     but a different, un-audited file) with a live RmlUi #include passed the gate
#     clean. Full-path matching closes that: only the exact frozen path is exempt.
# PT: check (b) -- whitelist congelada de arquivos de teste com permissão de #include
#     direto de RmlUi. Casado pelo caminho relativo COMPLETO (prefixado por
#     RML_TESTS_DIR, a mesma string que o `find` devolve pro $f abaixo) -- NÃO pelo
#     basename. A revisão adversarial da RMLX-0 provou que o casamento só-por-basename
#     era um furo real do gate: um arquivo NOVO em
#     glintfx/tests/subdir/domrw_sanity.cpp (mesmo basename da entrada de dívida
#     congelada, mas um arquivo diferente, não auditado) com um #include de RmlUi vivo
#     passava o gate limpo. Casar pelo caminho completo fecha isso: só o caminho
#     congelado exato fica isento.
RML_TEST_WHITELIST=(
  "glintfx/tests/domrw_sanity.cpp"
  "glintfx/tests/focus_sanity.cpp"
  "glintfx/tests/form_events_sanity.cpp"
  "glintfx/tests/document_reload_leak.cpp"
)

# EN: check (a) -- see the header comment above ("KNOWN, FROZEN, DOCUMENTED
#     EXCEPTION") for why these two, and only these two, are here.
# PT: check (a) -- ver o comentário de cabeçalho acima ("EXCEÇÃO CONHECIDA,
#     CONGELADA E DOCUMENTADA") pro motivo de estes dois, e só estes dois, estarem aqui.
RML_INCLUDE_EXCEPTIONS=(
  "glintfx/src/app.cpp"
  "glintfx/src/system_glfw_dedup.hpp"
)

# EN: check (c) -- same rationale, narrower: only system_glfw_dedup.hpp has real
#     (non-comment) Rml::Log/Rml::String token use; app.cpp's Rml:: mentions are 100%
#     comment.
# PT: check (c) -- mesma racional, mais estreita: só system_glfw_dedup.hpp tem uso
#     real (fora de comentário) de token Rml::Log/Rml::String; as menções a Rml:: em
#     app.cpp são 100% comentário.
RML_TOKEN_EXCEPTIONS=(
  "glintfx/src/system_glfw_dedup.hpp"
)

# EN: Matches an #include directive whose argument contains "Rml" -- catches both
#     angle-bracket Core headers (<RmlUi/Core/Types.h>) and the quoted GLFW
#     platform-backend header ("RmlUi_Platform_GLFW.h"). Anchored to the directive
#     itself (line-start, only whitespace before `#`), not free text in the file --
#     see check_encapsulation.sh's own header for why that anchoring matters.
# PT: Casa uma diretiva #include cujo argumento contém "Rml" -- pega tanto headers Core
#     de angle-bracket (<RmlUi/Core/Types.h>) quanto o header de backend de plataforma
#     GLFW entre aspas ("RmlUi_Platform_GLFW.h"). Ancorado na própria diretiva (início
#     de linha, só espaço antes do `#`), não em texto livre do arquivo -- ver o
#     cabeçalho do próprio check_encapsulation.sh pro motivo de a ancoragem importar.
RML_INCLUDE_PATTERN='^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^>"]*Rml[^>"]*[>"]'

# EN: check (c)'s blocking value-type token list.
# PT: lista bloqueante de tokens de tipo-valor do check (c).
RML_TOKEN_PATTERN='Rml::(String|Vector2|Colourb|Variant|Input|Log)'

# EN: check (d)'s generic opaque-usage pattern (any Rml::Identifier).
# PT: padrão genérico de uso opaco do check (d) (qualquer Rml::Identificador).
RML_ANY_TOKEN_PATTERN='Rml::[A-Za-z_][A-Za-z0-9_]*'

# -----------------------------------------------------------------------------
# EN: strip_comments FILE -- prints FILE with `//` line comments and `/* ... */` block
#     comments removed, one output line per input line (so line numbers used by
#     downstream grep -n stay correct). Minimal string/char-literal awareness: a `/`
#     inside `"..."`/`'...'` does not start a comment. Not a full C++ tokenizer
#     (raw-string literals R"(...)" are not special-cased -- none of the files this
#     gate scans use them; if one ever does, that is a false-negative risk, not a
#     false-positive one, and the doctrine here (grep is a hint, not a compiler) is
#     that a miss is far cheaper than a spurious block on unrelated work).
# PT: strip_comments ARQUIVO -- imprime ARQUIVO com comentário de linha `//` e de
#     bloco `/* ... */` removidos, uma linha de saída por linha de entrada (pra os
#     números de linha usados pelo grep -n rio abaixo continuarem corretos). Consciência
#     mínima de literal string/char: um `/` dentro de `"..."`/`'...'` não abre
#     comentário. Não é um tokenizer C++ completo (raw-string R"(...)" não tem
#     tratamento especial -- nenhum arquivo que este gate varre usa isso; se algum dia
#     usar, o risco é de falso-negativo, não falso-positivo, e a doutrina aqui (grep é
#     pista, não compilador) é que um erro-por-omissão é bem mais barato que travar
#     trabalho não relacionado à toa).
# -----------------------------------------------------------------------------
strip_comments() {
  awk '
    BEGIN { in_block = 0 }
    {
      line = $0
      n = length(line)
      out = ""
      in_str = 0
      in_chr = 0
      i = 1
      while (i <= n) {
        c = substr(line, i, 1)
        if (in_block) {
          if (c == "*" && substr(line, i + 1, 1) == "/") {
            in_block = 0
            i += 2
            continue
          }
          i += 1
          continue
        }
        if (in_str) {
          out = out c
          if (c == "\\") { out = out substr(line, i + 1, 1); i += 2; continue }
          if (c == "\"") { in_str = 0 }
          i += 1
          continue
        }
        if (in_chr) {
          out = out c
          if (c == "\\") { out = out substr(line, i + 1, 1); i += 2; continue }
          if (c == "'"'"'") { in_chr = 0 }
          i += 1
          continue
        }
        if (c == "\"") { in_str = 1; out = out c; i += 1; continue }
        if (c == "'"'"'") { in_chr = 1; out = out c; i += 1; continue }
        if (c == "/" && substr(line, i + 1, 1) == "/") { break }
        if (c == "/" && substr(line, i + 1, 1) == "*") { in_block = 1; i += 2; continue }
        out = out c
        i += 1
      }
      print out
    }
  ' "$1"
}

# EN: True if NEEDLE equals one of the remaining args.
# PT: Verdadeiro se NEEDLE é igual a um dos argumentos restantes.
rml_array_contains() {
  local needle="$1"
  shift
  local x
  for x in "$@"; do
    [[ "$x" == "$needle" ]] && return 0
  done
  return 1
}

# -----------------------------------------------------------------------------
# EN: check_include_gate SRC_ROOT_1 [SRC_ROOT_2 ...] -- shared engine for (a): finds
#     every *.c/*.cpp/*.h/*.hpp under the given roots, skips anything under
#     EXCL_DIR (global, set by caller) and anything in EXCL_FILES (global array),
#     and reports arquivo:linha for any #include matching RML_INCLUDE_PATTERN.
#     EXCL_DIR/EXCL_FILES are read as globals (not params) to keep call sites in
#     check (a)/selftest terse -- same "unqualified globals read by convention"
#     pattern tools/preci.sh documents for its own classify_touched_files().
# PT: check_include_gate RAIZ_1 [RAIZ_2 ...] -- motor compartilhado do (a): acha todo
#     *.c/*.cpp/*.h/*.hpp sob as raízes dadas, pula tudo sob EXCL_DIR (global, setado
#     pelo chamador) e tudo em EXCL_FILES (array global), e reporta arquivo:linha pra
#     qualquer #include que case RML_INCLUDE_PATTERN. EXCL_DIR/EXCL_FILES são lidos
#     como globais (não parâmetros) pra manter os pontos de chamada de (a)/selftest
#     enxutos -- mesmo padrão de "globais sem qualificador lidos por convenção" que o
#     tools/preci.sh documenta pra sua própria classify_touched_files().
# -----------------------------------------------------------------------------
check_include_gate() {
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    if rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      continue
    fi
    matches="$(grep -nE "$RML_INCLUDE_PATTERN" "$f" || true)"
    if [[ -n "$matches" ]]; then
      echo "RmlUi #include fora da whitelist em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(find "$@" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) -print0 2>/dev/null | sort -z)
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: check_test_whitelist_gate TESTS_DIR -- (b): every RmlUi #include in TESTS_DIR
#     must be in a file whose FULL RELATIVE PATH (as `find` yields it -- i.e.
#     TESTS_DIR-prefixed, not basename) is in EXCL_FILES (global, caller-set to the
#     frozen 4). Basename-only matching used to let a NEW file at
#     TESTS_DIR/subdir/<frozen-basename> ride through on the debt entry's name alone;
#     matching the full path closes that hole.
# PT: check_test_whitelist_gate DIR_TESTES -- (b): todo #include de RmlUi em
#     DIR_TESTES precisa estar num arquivo cujo CAMINHO RELATIVO COMPLETO (como o
#     `find` devolve -- prefixado por DIR_TESTES, não basename) está em EXCL_FILES
#     (global, setado pelo chamador pros 4 congelados). Casar só pelo basename deixava
#     um arquivo NOVO em DIR_TESTES/subdir/<basename-congelado> passar de carona no
#     nome da entrada de dívida; casar pelo caminho completo fecha esse furo.
# -----------------------------------------------------------------------------
check_test_whitelist_gate() {
  local tests_dir="$1"
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    if rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      continue
    fi
    matches="$(grep -nE "$RML_INCLUDE_PATTERN" "$f" || true)"
    if [[ -n "$matches" ]]; then
      echo "RmlUi #include em teste NOVO fora da whitelist congelada de 4, em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(find "$tests_dir" -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 2>/dev/null | sort -z)
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: check_token_gate SRC_ROOT_1 [...] -- (c): comment-stripped scan for
#     RML_TOKEN_PATTERN outside EXCL_DIR, skipping EXCL_FILES (global, caller-set to
#     the frozen exception).
# PT: check_token_gate RAIZ_1 [...] -- (c): varredura com comentário removido por
#     RML_TOKEN_PATTERN fora de EXCL_DIR, pulando EXCL_FILES (global, setado pelo
#     chamador pra exceção congelada).
# -----------------------------------------------------------------------------
check_token_gate() {
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    if rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      continue
    fi
    matches="$(strip_comments "$f" | grep -nE "$RML_TOKEN_PATTERN" || true)"
    if [[ -n "$matches" ]]; then
      echo "token de tipo-valor RmlUi fora de $RML_DIR em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(find "$@" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) -print0 2>/dev/null | sort -z)
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: opaque_debt_count SRC_ROOT_1 [...] -- (d): counts files (not occurrences) with
#     comment-stripped `Rml::Identifier` usage outside EXCL_DIR whose matching lines
#     are NOT already fully accounted for by RML_TOKEN_PATTERN. Report-only, no
#     concept of EXCL_FILES here on purpose -- (d) is meant to surface ALL residual
#     opaque debt, including inside the (a)/(c) frozen exceptions, not hide it.
# PT: opaque_debt_count RAIZ_1 [...] -- (d): conta arquivos (não ocorrências) com uso
#     `Rml::Identificador` (comentário removido) fora de EXCL_DIR cujas linhas casadas
#     NÃO já são totalmente cobertas por RML_TOKEN_PATTERN. Só relatório, sem conceito
#     de EXCL_FILES aqui de propósito -- (d) existe pra expor TODA a dívida opaca
#     residual, inclusive dentro das exceções congeladas de (a)/(c), não escondê-la.
# -----------------------------------------------------------------------------
opaque_debt_count() {
  local count=0
  local f stripped residual
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    stripped="$(strip_comments "$f")"
    residual="$(printf '%s\n' "$stripped" | grep -E "$RML_ANY_TOKEN_PATTERN" | grep -vE "$RML_TOKEN_PATTERN" || true)"
    if [[ -n "$residual" ]]; then
      count=$((count + 1))
    fi
  done < <(find "$@" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) -print0 2>/dev/null | sort -z)
  echo "$count"
}

# -----------------------------------------------------------------------------
# EN: run_real_check -- the 4 checks against the actual repo tree (CWD = repo root).
# PT: run_real_check -- as 4 checagens contra a árvore real do repo (CWD = raiz do repo).
# -----------------------------------------------------------------------------
run_real_check() {
  for d in "${RML_SRC_ROOTS[@]}" "$RML_DIR" "$RML_TESTS_DIR"; do
    if [[ ! -d "$d" ]]; then
      echo "error: '$d' not found (run from repo root)" >&2
      return 2
    fi
  done

  local overall=0

  EXCL_DIR="$RML_DIR"
  EXCL_FILES=("${RML_INCLUDE_EXCEPTIONS[@]}")
  if ! check_include_gate "${RML_SRC_ROOTS[@]}"; then
    overall=1
  fi

  EXCL_FILES=("${RML_TEST_WHITELIST[@]}")
  if ! check_test_whitelist_gate "$RML_TESTS_DIR"; then
    overall=1
  fi

  EXCL_DIR="$RML_DIR"
  EXCL_FILES=("${RML_TOKEN_EXCEPTIONS[@]}")
  if ! check_token_gate "${RML_SRC_ROOTS[@]}"; then
    overall=1
  fi

  EXCL_DIR="$RML_DIR"
  local debt
  debt="$(opaque_debt_count "${RML_SRC_ROOTS[@]}")"
  echo "divida opaca: ${debt} arquivos"

  if [[ "$overall" -ne 0 ]]; then
    echo "FAIL: RmlUi vazou da whitelist -- ver violações acima" >&2
    echo "FAIL: RmlUi leaked out of the whitelist -- see violations above" >&2
    return 1
  fi

  echo "OK: nenhum #include/token de RmlUi fora da whitelist (glintfx/src/rml/ + exceções congeladas)"
  echo "OK: no RmlUi #include/token outside the whitelist (glintfx/src/rml/ + frozen exceptions)"
  return 0
}

# -----------------------------------------------------------------------------
# EN: run_selftest -- SEED-GATE-NAO-GUARDA-SI-MESMO discipline: builds two throwaway
#     fixture trees under mktemp (never touches the real repo), and proves the gate
#     has teeth on BOTH sides -- a clean tree passes, a tree with a planted violation
#     in each of (a)/(b)/(c) fails. If any of the 4 assertions below is wrong, this
#     function itself fails loudly, and the caller (main, below) aborts BEFORE running
#     the real check -- a toothless gate must not silently report "OK" on the real
#     tree.
# PT: run_selftest -- disciplina SEED-GATE-NAO-GUARDA-SI-MESMO: monta duas árvores de
#     fixture descartáveis sob mktemp (nunca toca o repo real), e prova que o gate tem
#     dente dos DOIS lados -- árvore limpa passa, árvore com violação plantada em cada
#     um de (a)/(b)/(c) falha. Se qualquer uma das 4 asserções abaixo estiver errada,
#     esta função falha alto, e quem chama (main, abaixo) aborta ANTES de rodar o check
#     real -- um gate sem dente não pode reportar "OK" em silêncio na árvore real.
# -----------------------------------------------------------------------------
run_selftest() {
  local tmp
  tmp="$(mktemp -d "${TMPDIR:-/tmp}/rml-whitelist-selftest.XXXXXX")"
  # EN: No early `return` between here and the cleanup `rm -rf "$tmp"` below by
  #     construction, so a plain call-at-the-end is enough -- deliberately NOT
  #     `trap ... RETURN`: that trap fires again when the OUTER caller (main) returns
  #     too, by which point `tmp` is out of scope and `set -u` turns the stale
  #     `rm -rf "$tmp"` into an "unbound variable" abort (measured live while writing
  #     this script). This is throwaway scratch, not repo state, so the plain `rm`
  #     here is fine (it is entirely inside our own mktemp dir, never the shared
  #     working tree the CLAUDE.md "never rm scratch" rule guards).
  # PT: Nenhum `return` antecipado entre aqui e o `rm -rf "$tmp"` de limpeza abaixo,
  #     por construção, então uma chamada só no fim basta -- deliberadamente NÃO
  #     `trap ... RETURN`: essa trap dispara de novo quando o chamador EXTERNO (main)
  #     retorna também, e nesse ponto `tmp` já saiu de escopo e o `set -u` transforma o
  #     `rm -rf "$tmp"` obsoleto num abort de "variável não associada" (medido ao vivo
  #     escrevendo este script). É scratch descartável, não estado do repo, então o
  #     `rm` cru aqui é seguro (está inteiramente dentro do nosso próprio dir de
  #     mktemp, nunca a working tree compartilhada que a regra "nunca rm scratch" do
  #     CLAUDE.md protege).

  local ok=1

  # --- fixture 1: clean tree -- MUST pass all 3 blocking checks. ---
  local clean="$tmp/clean"
  mkdir -p "$clean/src/rml" "$clean/include/glintfx" "$clean/demos" "$clean/tests"
  cat > "$clean/src/rml/bootstrap.hpp" <<'EOF'
#pragma once
#include <RmlUi/Core/Context.h>
namespace demo { inline Rml::Context* ctx() { return nullptr; } }
EOF
  cat > "$clean/src/widget.cpp" <<'EOF'
// EN: no RmlUi contact here. Mentions "RmlUi" only in a comment, never in an #include.
#include <string>
namespace demo { std::string name() { return "widget"; } }
EOF
  cat > "$clean/include/glintfx/widget.hpp" <<'EOF'
#pragma once
// EN: dozens-of-comment-mentions stress case: "//" trailing after real code below,
// which strip_comments() must still strip.
int widget_id(); // wraps Rml::Element::GetId() upstream (comment-only, not code)
EOF
  cat > "$clean/tests/widget_sanity.cpp" <<'EOF'
#include "widget.hpp"
int main() { return widget_id() == 0 ? 0 : 1; }
EOF

  EXCL_DIR="$clean/src/rml"
  EXCL_FILES=()
  if ! check_include_gate "$clean/src" "$clean/include" "$clean/demos"; then
    echo "selftest FAIL: check (a) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi
  EXCL_FILES=()
  if ! check_test_whitelist_gate "$clean/tests"; then
    echo "selftest FAIL: check (b) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi
  EXCL_DIR="$clean/src/rml"
  EXCL_FILES=()
  if ! check_token_gate "$clean/src" "$clean/include" "$clean/demos"; then
    echo "selftest FAIL: check (c) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi

  # --- fixture 2: dirty tree -- MUST fail (a), (b) and (c) independently. ---
  local dirty="$tmp/dirty"
  mkdir -p "$dirty/src/rml" "$dirty/include/glintfx" "$dirty/demos" "$dirty/tests"
  cp "$clean/src/rml/bootstrap.hpp" "$dirty/src/rml/bootstrap.hpp"

  # (a): RmlUi include planted OUTSIDE src/rml/.
  cat > "$dirty/src/leaky.cpp" <<'EOF'
#include <RmlUi/Core/Element.h>
void f() {}
EOF

  # (b): RmlUi include in a test NOT in the frozen whitelist.
  cat > "$dirty/tests/new_widget_sanity.cpp" <<'EOF'
#include <RmlUi/Core/Context.h>
int main() { return 0; }
EOF

  # (c): value-type token in real code outside src/rml/ (no #include needed to trip
  #      this check on its own -- proves (c) is independent of (a)).
  cat > "$dirty/src/typeleak.cpp" <<'EOF'
// EN: no #include here on purpose -- (c) must catch the token even without (a) firing.
void g(const Rml::String& s) { (void)s; }
EOF

  EXCL_DIR="$dirty/src/rml"
  EXCL_FILES=()
  if check_include_gate "$dirty/src" "$dirty/include" "$dirty/demos"; then
    echo "selftest FAIL: check (a) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi
  EXCL_FILES=()
  if check_test_whitelist_gate "$dirty/tests"; then
    echo "selftest FAIL: check (b) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi
  EXCL_DIR="$dirty/src/rml"
  EXCL_FILES=()
  if check_token_gate "$dirty/src" "$dirty/include" "$dirty/demos"; then
    echo "selftest FAIL: check (c) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi

  # --- fixture 3: RMLX-0 basename-collision exploit -- proves check (b) now matches
  #     by FULL PATH, not basename. Adversarial review of the pre-fix gate proved a
  #     real hole: a NEW file whose basename collided with a frozen whitelist entry
  #     (but lived at a DIFFERENT path, e.g. a subdir) rode the gate clean because
  #     only the basename was compared. Two independent trees, each isolated from any
  #     other violation, so a failure here can only mean the collision itself: (3a)
  #     the legitimately whitelisted file at its exact frozen path must still pass;
  #     (3b) a same-basename file at a DIFFERENT, un-whitelisted path must still fail.
  # PT: fixture 3 -- exploit de colisão de basename da RMLX-0 -- prova que o check (b)
  #     agora casa por CAMINHO COMPLETO, não por basename. A revisão adversarial do
  #     gate pré-conserto provou um furo real: um arquivo NOVO cujo basename colidia
  #     com uma entrada congelada da whitelist (mas vivia em um caminho DIFERENTE, ex.
  #     um subdir) passava o gate limpo porque só o basename era comparado. Duas
  #     árvores independentes, cada uma isolada de qualquer outra violação, pra uma
  #     falha aqui só poder significar a própria colisão: (3a) o arquivo
  #     legitimamente whitelisted no seu caminho congelado exato ainda tem de passar;
  #     (3b) um arquivo de mesmo basename num caminho DIFERENTE, não-whitelisted,
  #     ainda tem de falhar.
  local collision_ok="$tmp/collision_ok"
  mkdir -p "$collision_ok/tests"
  cat > "$collision_ok/tests/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
int main() { return 0; }
EOF
  EXCL_FILES=("$collision_ok/tests/domrw_sanity.cpp")
  if ! check_test_whitelist_gate "$collision_ok/tests"; then
    echo "selftest FAIL: check (b) reprovou o arquivo whitelisted legitimo no caminho completo exato (falso positivo)" >&2
    ok=0
  fi

  local collision_bad="$tmp/collision_bad"
  mkdir -p "$collision_bad/tests/subdir"
  cat > "$collision_bad/tests/subdir/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
int main() { return 0; }
EOF
  EXCL_FILES=("$collision_bad/tests/domrw_sanity.cpp")
  if check_test_whitelist_gate "$collision_bad/tests"; then
    echo "selftest FAIL: check (b) NAO reprovou colisao de basename em subdir nao-whitelisted (falso negativo -- o furo RMLX-0 que a revisao adversarial provou)" >&2
    ok=0
  fi

  rm -rf "$tmp"

  if [[ "$ok" -ne 1 ]]; then
    return 1
  fi
  echo "selftest OK: fixture limpa passa nos 3 checks bloqueantes, fixture suja falha nos 3 (independentemente), e a colisao de basename em subdir da RMLX-0 continua bloqueada"
  return 0
}

# -----------------------------------------------------------------------------
# EN: main.
# PT: main.
# -----------------------------------------------------------------------------
main() {
  case "${1:-}" in
    --selftest)
      run_selftest
      return $?
      ;;
    -h|--help)
      cat <<EOF
Usage: ${SCRIPT_NAME} [--selftest]
  (no args)    selftest, then the real check against the repo tree in CWD.
  --selftest   selftest only, self-contained (mktemp), no repo needed.
EOF
      return 0
      ;;
    "")
      ;;
    *)
      echo "error: unknown argument '$1' (see --help)" >&2
      return 2
      ;;
  esac

  echo "-- RMLX-0/F4 selftest (SEED-GATE-NAO-GUARDA-SI-MESMO) --"
  if ! run_selftest; then
    echo "FAIL: o proprio gate esta sem dente -- abortando ANTES do check real (nao confiar num gate que nao passou no proprio selftest)" >&2
    return 1
  fi

  echo "-- RMLX-0/F4 check real --"
  run_real_check
}

main "$@"
