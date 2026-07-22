#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Doc `file:line` citation gate (DOC-HOSTIN follow-up). Kills the CLASS of bug found
#     by the qa-engineer review of DOC-HOSTIN (2026-07-22): a doc-comment citation like
#     `` `is_key_down(Key k)` (`app.hpp:325`) `` silently rots the moment the cited header
#     gains or loses a line above that point -- reviewed-by-eye citations are exactly the
#     kind of claim this house's own "review adversarial que executa" discipline distrusts.
#     This script re-derives each citation mechanically: open the cited file, read the
#     cited line(s), and fail if the symbol the citation claims to document is not
#     actually there.
#
#     SCOPE, DELIBERATELY NARROW (read this before adding a citation format and wondering
#     why it is not caught). Only two citation SHAPES are parsed, because they are the only
#     ones a plain-text heuristic can extract a symbol from without guessing at prose:
#       (a) single:  `SYMBOL` (`path/to/file.ext:N`)             or `:N-M` (a range)
#       (b) double:  `SYMBOL1` / `SYMBOL2` (`file.ext:N`/`M`)    -- the shorthand this repo's
#           own docs use for two sibling methods sharing one citation group (see
#           docs/embed-integration.md section 22).
#     In both shapes the symbol backtick-span must be IMMEDIATELY adjacent to the opening
#     paren of the citation (only whitespace, or " / " for the double shape, in between).
#     A citation of the form `` `Engine::render_standalone`'s doc-comment (`engine.hpp:90-125`) ``
#     -- symbol separated from the citation by prose ("'s doc-comment") -- is NOT parsed:
#     which backtick span upstream of a citation is "the" documented symbol becomes a guess
#     once prose sits in between, and a verifier that guesses produces exactly the kind of
#     false positive/negative noise that gets a gate muted and ignored. Those citations are
#     silently skipped, not flagged as OK and not flagged as FAIL -- an honest, narrow
#     verifier beats a broad one that lies. Symbol extraction is a NAME only (identifier
#     before the first `(`, after the last `::`/`.`, after the last space if any) checked as
#     a case-sensitive SUBSTRING of the cited line(s) -- deliberately weak (it cannot tell
#     "used" from "declared") but robust against reformatting (return type churn, added
#     `const`, reordered parameters) that would break an exact-signature match and turn the
#     gate into a maintenance burden of its own.
#
#     FILE RESOLUTION: a citation path containing '/' is resolved relative to the repo
#     root; a bare filename (`app.hpp:325`) is located by `find` under the repo root,
#     excluding build directories (`*/build*/*`) and `.git`. A citation whose file cannot be
#     located at all (e.g. a vendored/FetchContent'd third-party path not present before the
#     first `cmake` configure -- `StyleSheetSpecification.cpp` is RmlUi's, not ours) is
#     SKIPPED with a note, not failed: this gate can only assert on what this checkout
#     actually has on disk.
#
#     SCANNED DOCS: docs/*.md, docs/adr/*.md, docs/wiki/*.md -- stable, LIVE reference
#     documentation meant to track today's line numbers. TWO directories are DELIBERATELY
#     EXCLUDED, both for the same reason (a citation there is correct FOR A POINT IN TIME,
#     not meant to be re-verified against HEAD forever):
#       - docs/superpowers/{plans,specs}/*.md: living planning documents (a plan may cite a
#         line that is correct only "once implemented", or describe a future state).
#       - docs/auditoria/*.md: dated audit reports, each with an explicit `Date` and
#         `Target: glintfx vX.Y.Z` header pinning it to a past tagged version -- their own
#         prose already marks line citations as approximate (`~L957-1075`) for exactly this
#         reason. Confirmed empirically while building this gate: AUD-L1-MEM.md/
#         AUD-L1-SUPPLY.md cite pre-FX-CARVE-1 line numbers in `render_gl3.cpp`/
#         `CMakeLists.txt` that a real refactor since v0.11.0 moved or renumbered -- gating
#         a dated audit snapshot on HEAD's current line numbers would be exactly the kind of
#         false failure this script's own design principle (narrow > noisy) rejects. This is
#         a genuine, pre-existing documentation-drift finding, reported upstream instead of
#         silently patched here (out of this gate's job: re-verifying a past audit's
#         technical claims against the current architecture is engineering review, not a
#         line-number bookkeeping fix).
#
# PT: Gate de citação `arquivo:linha` em doc (follow-up do DOC-HOSTIN). Mata a CLASSE do bug
#     achado pelo review do qa-engineer no DOC-HOSTIN (2026-07-22): uma citação de
#     doc-comment tipo `` `is_key_down(Key k)` (`app.hpp:325`) `` apodrece silenciosamente no
#     instante em que o header citado ganha ou perde uma linha acima daquele ponto --
#     citações revisadas a olho são exatamente o tipo de alegação que a própria disciplina
#     "review adversarial que executa" desta casa desconfia. Este script re-deriva cada
#     citação mecanicamente: abre o arquivo citado, lê a(s) linha(s) citada(s), e falha se o
#     símbolo que a citação afirma documentar não estiver realmente lá.
#
#     ESCOPO, DELIBERADAMENTE ESTREITO (leia isto antes de adicionar um formato de citação e
#     se perguntar por que não foi pego). Só duas FORMAS de citação são parseadas, porque são
#     as únicas das quais uma heurística de texto puro consegue extrair um símbolo sem
#     adivinhar prosa:
#       (a) simples: `SÍMBOLO` (`caminho/do/arquivo.ext:N`)        ou `:N-M` (faixa)
#       (b) dupla:   `SÍMBOLO1` / `SÍMBOLO2` (`arquivo.ext:N`/`M`) -- o atalho que os
#           próprios docs deste repo usam pra dois métodos irmãos compartilhando um grupo
#           de citação (ver docs/embed-integration.md seção 22).
#     Nas duas formas, o trecho entre crases do símbolo precisa estar IMEDIATAMENTE ao lado
#     do parêntese de abertura da citação (só espaço em branco, ou " / " na forma dupla,
#     entre eles). Uma citação tipo
#     `` `Engine::render_standalone`'s doc-comment (`engine.hpp:90-125`) `` -- símbolo
#     separado da citação por prosa ("'s doc-comment") -- NÃO é parseada: qual trecho entre
#     crases anterior a uma citação é "o" símbolo documentado vira um chute assim que há
#     prosa no meio, e um verificador que chuta produz exatamente o tipo de ruído de
#     falso-positivo/negativo que faz um gate ser silenciado e ignorado. Essas citações são
#     silenciosamente puladas, não marcadas como OK nem como FALHA -- um verificador honesto
#     e estreito vale mais que um abrangente que mente. A extração de símbolo é só um NOME
#     (identificador antes do primeiro `(`, depois do último `::`/`.`, depois do último
#     espaço se houver) checado como SUBSTRING (sensível a maiúsculas) da(s) linha(s)
#     citada(s) -- deliberadamente fraco (não distingue "usado" de "declarado") mas robusto
#     contra reformatação (troca de tipo de retorno, `const` adicionado, parâmetros
#     reordenados) que quebraria um match de assinatura exata e transformaria o gate num
#     fardo de manutenção próprio.
#
#     RESOLUÇÃO DE ARQUIVO: uma citação com caminho contendo '/' é resolvida relativo à raiz
#     do repo; um nome de arquivo nu (`app.hpp:325`) é localizado por `find` a partir da raiz
#     do repo, excluindo diretórios de build (`*/build*/*`) e `.git`. Uma citação cujo
#     arquivo não é localizável de jeito nenhum (ex.: um caminho de terceiro
#     vendorizado/FetchContent ainda não presente antes do primeiro configure do `cmake` --
#     `StyleSheetSpecification.cpp` é do RmlUi, não nosso) é PULADA com uma nota, não
#     falhada: este gate só consegue afirmar sobre o que este checkout de fato tem em disco.
#
#     DOCS VARRIDOS: docs/*.md, docs/adr/*.md, docs/wiki/*.md -- documentação estável e VIVA,
#     feita pra acompanhar os números de linha de hoje. DOIS diretórios são DELIBERADAMENTE
#     EXCLUÍDOS, pelo mesmo motivo (uma citação lá é correta PRA UM MOMENTO no tempo, não
#     feita pra ser reverificada contra o HEAD pra sempre):
#       - docs/superpowers/{plans,specs}/*.md: documentos de planejamento vivos (um plano
#         pode citar uma linha que só é correta "uma vez implementado", ou descrever um
#         estado futuro).
#       - docs/auditoria/*.md: relatórios de auditoria datados, cada um com cabeçalho
#         explícito `Date` e `Target: glintfx vX.Y.Z` fixando-o numa versão taggeada
#         passada -- a própria prosa deles já marca citações de linha como aproximadas
#         (`~L957-1075`) exatamente por este motivo. Confirmado empiricamente ao construir
#         este gate: AUD-L1-MEM.md/AUD-L1-SUPPLY.md citam números de linha pré-FX-CARVE-1 em
#         `render_gl3.cpp`/`CMakeLists.txt` que um refactor real desde a v0.11.0 moveu ou
#         renumerou -- gatear um snapshot de auditoria datado pelos números de linha atuais
#         do HEAD seria exatamente o tipo de falha falsa que o próprio princípio de design
#         deste script (estreito > ruidoso) rejeita. Este é um achado genuíno e
#         pré-existente de drift de documentação, relatado adiante em vez de silenciosamente
#         remendado aqui (fora do trabalho deste gate: reverificar as alegações técnicas de
#         uma auditoria passada contra a arquitetura atual é review de engenharia, não um
#         fix de contabilidade de número de linha).
#
# Usage / Uso: tools/check_doc_line_refs.sh [docs-dir]  (default: docs, relative to repo root)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCS_DIR_ARG="${1:-docs}"

cd "${REPO_ROOT}"

if [[ ! -d "${DOCS_DIR_ARG}" ]]; then
  echo "error: docs dir '${DOCS_DIR_ARG}' not found (run from repo root, or pass a path)" >&2
  exit 2
fi

# EN: The scanned set -- top-level + the stable reference sub-dirs, superpowers/ and
#     auditoria/ excluded (see header comment). PT: o conjunto varrido -- topo + os
#     subdiretórios estáveis de referência, superpowers/ e auditoria/ excluídos (ver
#     comentário de cabeçalho).
mapfile -t DOC_FILES < <(
  {
    find "${DOCS_DIR_ARG}" -maxdepth 1 -name '*.md' -type f
    for sub in adr wiki; do
      [[ -d "${DOCS_DIR_ARG}/${sub}" ]] && find "${DOCS_DIR_ARG}/${sub}" -name '*.md' -type f
    done
  } | sort -u
)

# EN: SINGLE shape: `SYMBOL` (`path:N` or `path:N-M`). Capture groups (bash ERE, matched via
#     [[ =~ ]]): 1=symbol, 2=path, 3=first line, 4=(unused, the "-M" incl. dash), 5=last line.
# PT: forma SIMPLES: `SÍMBOLO` (`caminho:N` ou `caminho:N-M`). Grupos de captura (ERE do
#     bash, via [[ =~ ]]): 1=símbolo, 2=caminho, 3=primeira linha, 4=(não usado, "-M" com
#     traço), 5=última linha.
SINGLE_RE='`([^`]+)`[[:space:]]\(`([A-Za-z0-9_./-]+\.[A-Za-z0-9]+):([0-9]+)(-([0-9]+))?`\)'

# EN: DOUBLE shape: `SYM1` / `SYM2` (`path:N`/`M`). Groups: 1=sym1, 2=sym2, 3=path, 4=line1,
#     5=line2. PT: forma DUPLA: `SÍM1` / `SÍM2` (`caminho:N`/`M`). Grupos: 1=sym1, 2=sym2,
#     3=caminho, 4=linha1, 5=linha2.
DOUBLE_RE='`([^`]+)`[[:space:]]*/[[:space:]]*`([^`]+)`[[:space:]]\(`([A-Za-z0-9_./-]+\.[A-Za-z0-9]+):([0-9]+)`/`([0-9]+)`\)'

pass_count=0
fail_count=0
skip_count=0

# EN: Extract a bare identifier NAME out of a raw backtick-span (see the header comment's
#     "symbol extraction" rule). PT: extrai um NOME de identificador nu de um trecho cru
#     entre crases (ver a regra de "extração de símbolo" no comentário de cabeçalho).
extract_name() {
  local raw="$1"
  raw="${raw%%(*}"
  raw="${raw%%=*}"
  raw="${raw%%<*}"
  raw="$(printf '%s' "${raw}" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
  [[ "${raw}" == *"::"* ]] && raw="${raw##*::}"
  [[ "${raw}" == *"."* ]] && raw="${raw##*.}"
  [[ "${raw}" == *" "* ]] && raw="${raw##* }"
  raw="$(printf '%s' "${raw}" | sed -e 's/[^A-Za-z0-9_].*$//')"
  printf '%s' "${raw}"
}

# EN: Resolve a citation path to an actual file under REPO_ROOT. Echoes the resolved path,
#     or nothing (and returns 1) if it cannot be found. PT: resolve um caminho de citação pra
#     um arquivo real sob REPO_ROOT. Ecoa o caminho resolvido, ou nada (retorna 1) se não
#     achar.
resolve_path() {
  local cited="$1"
  if [[ "${cited}" == */* ]]; then
    if [[ -f "${REPO_ROOT}/${cited}" ]]; then
      printf '%s' "${REPO_ROOT}/${cited}"
      return 0
    fi
    return 1
  fi
  local found
  found="$(find "${REPO_ROOT}" -type f -name "${cited}" \
            -not -path '*/build*/*' -not -path '*/.git/*' 2>/dev/null | head -n1 || true)"
  if [[ -n "${found}" ]]; then
    printf '%s' "${found}"
    return 0
  fi
  return 1
}

# EN: Check whether `name` appears as a substring on any line in [first, last] of `file`.
# PT: checa se `name` aparece como substring em alguma linha em [first, last] de `file`.
name_in_range() {
  local file="$1" first="$2" last="$3" name="$4"
  sed -n "${first},${last}p" "${file}" | grep -qF -- "${name}"
}

# EN: Verify one citation match. Args: doc file, doc line number (for reporting), resolved
#     symbol name, cited path token, first line, last line. PT: verifica uma citação.
#     Args: arquivo de doc, número de linha do doc (pra relatar), nome de símbolo resolvido,
#     token de caminho citado, primeira linha, última linha.
verify_one() {
  local doc_file="$1" doc_line="$2" name="$3" cited_path="$4" first="$5" last="$6"

  if [[ -z "${name}" ]]; then
    echo "SKIP  ${doc_file}:${doc_line}  (could not extract a plain identifier from the citation's symbol span -- not a supported shape)"
    skip_count=$((skip_count + 1))
    return
  fi

  local resolved
  if ! resolved="$(resolve_path "${cited_path}")"; then
    echo "SKIP  ${doc_file}:${doc_line}  ${cited_path} not found under ${REPO_ROOT} (vendored/build artifact not present in this checkout?) -- symbol '${name}' unverified"
    skip_count=$((skip_count + 1))
    return
  fi

  local total_lines
  total_lines="$(wc -l < "${resolved}")"
  if [[ "${first}" -gt "${total_lines}" || "${last}" -gt "${total_lines}" ]]; then
    echo "FAIL  ${doc_file}:${doc_line}  ${cited_path}:${first}${first:+}$([[ "${last}" != "${first}" ]] && echo "-${last}") is past end-of-file (${resolved} has ${total_lines} lines) -- symbol '${name}'"
    fail_count=$((fail_count + 1))
    return
  fi

  if name_in_range "${resolved}" "${first}" "${last}" "${name}"; then
    pass_count=$((pass_count + 1))
  else
    local actual
    actual="$(sed -n "${first}p" "${resolved}")"
    echo "FAIL  ${doc_file}:${doc_line}"
    echo "      cited:  ${cited_path}:${first}$([[ "${last}" != "${first}" ]] && echo "-${last}")  (resolved: ${resolved})"
    echo "      symbol: '${name}' not found on that line/range"
    echo "      actual line ${first}: ${actual}"
    fail_count=$((fail_count + 1))
  fi
}

for doc_file in "${DOC_FILES[@]}"; do
  [[ -z "${doc_file}" ]] && continue
  line_no=0
  while IFS= read -r line || [[ -n "${line}" ]]; do
    line_no=$((line_no + 1))
    local_line="${line}"

    # EN: DOUBLE shape first (its citation text would otherwise partially confuse the
        # single-shape matcher on the trailing "`/`M`)" tail). PT: forma DUPLA primeiro (o
        # texto da citação dela senão confundiria em parte o matcher da forma simples na
        # cauda "`/`M`)").
    while [[ "${local_line}" =~ ${DOUBLE_RE} ]]; do
      whole="${BASH_REMATCH[0]}"
      sym1="${BASH_REMATCH[1]}"
      sym2="${BASH_REMATCH[2]}"
      path="${BASH_REMATCH[3]}"
      l1="${BASH_REMATCH[4]}"
      l2="${BASH_REMATCH[5]}"
      verify_one "${doc_file}" "${line_no}" "$(extract_name "${sym1}")" "${path}" "${l1}" "${l1}"
      verify_one "${doc_file}" "${line_no}" "$(extract_name "${sym2}")" "${path}" "${l2}" "${l2}"
      local_line="${local_line/"${whole}"/}"
    done

    while [[ "${local_line}" =~ ${SINGLE_RE} ]]; do
      whole="${BASH_REMATCH[0]}"
      sym="${BASH_REMATCH[1]}"
      path="${BASH_REMATCH[2]}"
      l1="${BASH_REMATCH[3]}"
      l2="${BASH_REMATCH[5]:-${BASH_REMATCH[3]}}"
      verify_one "${doc_file}" "${line_no}" "$(extract_name "${sym}")" "${path}" "${l1}" "${l2}"
      local_line="${local_line/"${whole}"/}"
    done
  done < "${doc_file}"
done

echo ""
echo "check_doc_line_refs: ${pass_count} ok, ${fail_count} failed, ${skip_count} skipped (unsupported shape or file not in this checkout)"

if [[ "${fail_count}" -gt 0 ]]; then
  echo "FAIL: one or more doc file:line citations do not contain the symbol they claim to document -- see above" >&2
  echo "FALHA: uma ou mais citações arquivo:linha de doc não contêm o símbolo que afirmam documentar -- ver acima" >&2
  exit 1
fi

echo "OK: every checkable doc file:line citation matches its cited source"
echo "OK: toda citação arquivo:linha de doc verificável bate com sua fonte citada"
