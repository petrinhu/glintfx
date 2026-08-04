#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: Doc `file:line` citation gate (DOC-HOSTIN follow-up). Kills the CLASS of bug found
#     by the qa-engineer review of DOC-HOSTIN (2026-07-22): a doc-comment citation like
#     `` `is_key_down(Key k)` (`app.hpp:325`) `` silently rots the moment the cited header
#     gains or loses a line above that point -- reviewed-by-eye citations are exactly the
#     kind of claim this house's own "review adversarial que executa" discipline distrusts.
#     This script re-derives each citation mechanically: open the cited file, read the
#     cited line(s), and fail if the symbol the citation claims to document is not
#     actually there.
#
#     TOLERANT BY DEFAULT (DOCREFS-RIGIDEZ, W25): a citation whose symbol is NOT on the
#     cited line(s) anymore is no longer an automatic FAIL. Inserting one line above a
#     header used to reprove every citation below it, even commits that never touched
#     those sections -- and that pressure once bent an actual architecture decision (see
#     TODO.md's DOCREFS-RIGIDEZ item: FRAMEGRAB-EMBED's implementer kept a duplicated
#     struct instead of a shared header specifically to avoid shifting 10 gated
#     citations). The gate's JOB is catching rot (symbol genuinely gone), not policing
#     line-number arithmetic. So on a miss this script now searches the WHOLE cited file
#     for the same symbol before giving up:
#       - found elsewhere  -> WARN (drift, not rot), suggests the corrected citation in
#         the shape `file:OLD -> file:NEW`, and exits 0. If the symbol is not unique in
#         the file (a real risk for a short/common name like `ok` or `run`), ALL other
#         occurrences are listed too, closest-to-the-old-line wins as "the" suggestion but
#         nothing is silently discarded.
#       - found nowhere     -> FAIL, exit 1, unchanged from before: the symbol is actually
#         gone, this is rot, and rot is exactly what this gate exists to catch.
#     Pass `--strict` to recover the OLD, line-exact behaviour (a miss is FAIL regardless
#     of whether the symbol still exists elsewhere in the file) -- useful for a one-off
#     "how much drift do we actually have" audit without WARN noise, or for a caller that
#     wants the pre-W25 hard gate back.
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
#     gate into a maintenance burden of its own. The SAME weak substring match is what
#     powers the whole-file fallback above, with the SAME known cost: a short/common name
#     (`ok`, `run`, `x`) can have many candidate lines, which is exactly why the
#     multi-candidate case is reported explicitly instead of picked silently.
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
#         line-number bookkeeping fix). NOTE this directory scope is untouched by
#         DOCREFS-RIGIDEZ (W25) -- see SEED-DOCREFS-SCOPE (W31) in TODO.md for the separate,
#         still-open question of whether/how to cover it.
#
#     NON-GOAL: this script does not, and will not, auto-rewrite the docs it checks. A WARN
#     prints the suggested fix as text for a human (or another agent) to apply; nothing here
#     ever opens a doc file for writing. A `--fix` mode that edits citations in place was
#     considered and explicitly rejected for this fatia (DOCREFS-RIGIDEZ, W25) -- it is a
#     different, riskier tool (editing prose mechanically) and is out of scope here.
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
#     TOLERANTE POR PADRÃO (DOCREFS-RIGIDEZ, W25): uma citação cujo símbolo NÃO está mais
#     na(s) linha(s) citada(s) deixou de ser FAIL automático. Inserir uma linha acima de um
#     header reprovava toda citação abaixo dela, inclusive commits que nunca tocaram
#     naquelas seções -- e essa pressão chegou a curvar uma decisão de arquitetura de
#     verdade (ver o item DOCREFS-RIGIDEZ no TODO.md: o implementer do FRAMEGRAB-EMBED
#     manteve uma struct duplicada em vez de um header compartilhado especificamente pra
#     não deslocar 10 citações gateadas). O TRABALHO do gate é pegar apodrecimento (símbolo
#     de fato sumiu), não policiar aritmética de número de linha. Então, num miss, este
#     script agora procura o MESMO símbolo no arquivo INTEIRO antes de desistir:
#       - achou em outro lugar -> WARN (drift, não rot), sugere a citação corrigida no
#         formato `arquivo:ANTIGA -> arquivo:NOVA`, e sai com 0. Se o símbolo não for único
#         no arquivo (risco real pra nome curto/comum tipo `ok` ou `run`), TODAS as outras
#         ocorrências também são listadas -- a mais próxima da linha antiga vence como "a"
#         sugestão, mas nada é descartado em silêncio.
#       - não achou em lugar nenhum -> FAIL, exit 1, sem mudança: o símbolo de fato sumiu,
#         isto é apodrecimento, e apodrecimento é exatamente o que este gate existe pra
#         pegar.
#     Passe `--strict` pra recuperar o comportamento ANTIGO, exato por linha (um miss é
#     FAIL independente de o símbolo ainda existir em outro lugar do arquivo) -- útil pra
#     uma auditoria pontual de "quanto drift a gente realmente tem" sem ruído de WARN, ou
#     pra quem quer de volta o gate duro pré-W25.
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
#     fardo de manutenção próprio. O MESMO match fraco de substring é o que alimenta o
#     fallback de arquivo inteiro acima, com o MESMO custo conhecido: um nome curto/comum
#     (`ok`, `run`, `x`) pode ter muitas linhas candidatas, e é exatamente por isso que o
#     caso multi-candidata é reportado explicitamente em vez de escolhido em silêncio.
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
#         fix de contabilidade de número de linha). NOTA: este escopo de diretório NÃO é
#         tocado pelo DOCREFS-RIGIDEZ (W25) -- ver SEED-DOCREFS-SCOPE (W31) no TODO.md pra
#         a questão separada, ainda aberta, de se/como cobrir aquele diretório.
#
#     NÃO-OBJETIVO: este script não reescreve, e não vai reescrever, os docs que verifica.
#     Um WARN imprime a correção sugerida como texto pra um humano (ou outro agente)
#     aplicar; nada aqui abre um arquivo de doc pra escrita. Um modo `--fix` que editasse
#     citações no lugar foi considerado e explicitamente rejeitado nesta fatia
#     (DOCREFS-RIGIDEZ, W25) -- é uma ferramenta diferente e mais arriscada (editar prosa
#     mecanicamente), fora de escopo aqui.
#
# Usage / Uso: tools/check_doc_line_refs.sh [--strict] [docs-dir]  (default docs-dir: docs,
#              relative to repo root; --strict recovers the pre-W25 line-exact gate)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# EN: Argument parsing -- `--strict` may appear anywhere; at most one positional arg (the
#     docs dir) is accepted, matching the pre-W25 interface exactly when called with none
#     (the shape tools/precommit.sh uses). PT: parse de argumento -- `--strict` pode
#     aparecer em qualquer posição; no máximo um argumento posicional (o diretório de docs)
#     é aceito, casando a interface pré-W25 exatamente quando chamado sem nenhum (a forma
#     que tools/precommit.sh usa).
STRICT=0
DOCS_DIR_ARG="docs"
docs_dir_set=0
for arg in "$@"; do
  case "${arg}" in
    --strict)
      STRICT=1
      ;;
    -h|--help)
      echo "Usage: $0 [--strict] [docs-dir]" >&2
      exit 0
      ;;
    --*)
      echo "error: unknown flag '${arg}' (supported: --strict)" >&2
      exit 2
      ;;
    *)
      if [[ "${docs_dir_set}" -eq 1 ]]; then
        echo "error: unexpected extra argument '${arg}' (only one docs-dir is accepted)" >&2
        exit 2
      fi
      DOCS_DIR_ARG="${arg}"
      docs_dir_set=1
      ;;
  esac
done

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
warn_count=0
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

  local range_suffix=""
  [[ "${last}" != "${first}" ]] && range_suffix="-${last}"

  if [[ "${first}" -le "${total_lines}" && "${last}" -le "${total_lines}" ]] \
     && name_in_range "${resolved}" "${first}" "${last}" "${name}"; then
    pass_count=$((pass_count + 1))
    return
  fi

  # EN: Miss -- either past end-of-file, or present-but-not-matching in [first,last].
  # PT: miss -- ou passou do fim do arquivo, ou está presente mas não bate em [first,last].
  if [[ "${STRICT}" -eq 1 ]]; then
    if [[ "${first}" -gt "${total_lines}" || "${last}" -gt "${total_lines}" ]]; then
      echo "FAIL  ${doc_file}:${doc_line}  ${cited_path}:${first}${range_suffix} is past end-of-file (${resolved} has ${total_lines} lines) -- symbol '${name}'"
    else
      local actual
      actual="$(sed -n "${first}p" "${resolved}")"
      echo "FAIL  ${doc_file}:${doc_line}"
      echo "      cited:  ${cited_path}:${first}${range_suffix}  (resolved: ${resolved})"
      echo "      symbol: '${name}' not found on that line/range"
      echo "      actual line ${first}: ${actual}"
    fi
    fail_count=$((fail_count + 1))
    return
  fi

  # EN: TOLERANT (default) -- the cited line moved or is gone. Search the WHOLE file
  #     before giving up: found elsewhere is drift (WARN), found nowhere is rot (FAIL).
  # PT: TOLERANTE (padrão) -- a linha citada se moveu ou sumiu. Procura o arquivo INTEIRO
  #     antes de desistir: achou em outro lugar é drift (WARN), não achou em lugar nenhum é
  #     apodrecimento (FAIL).
  local -a match_lines=()
  local ln
  while IFS=: read -r ln _; do
    [[ -n "${ln}" ]] && match_lines+=("${ln}")
  done < <(grep -nF -- "${name}" "${resolved}" || true)

  if [[ "${#match_lines[@]}" -eq 0 ]]; then
    echo "FAIL  ${doc_file}:${doc_line}"
    echo "      cited:  ${cited_path}:${first}${range_suffix}  (resolved: ${resolved})"
    echo "      symbol: '${name}' not found ANYWHERE in ${cited_path} (${total_lines} lines) -- looks like real drift/rot, not a line shift"
    fail_count=$((fail_count + 1))
    return
  fi

  local nearest_line="" nearest_diff="" diff
  for ln in "${match_lines[@]}"; do
    diff=$(( ln > first ? ln - first : first - ln ))
    if [[ -z "${nearest_diff}" || "${diff}" -lt "${nearest_diff}" ]]; then
      nearest_line="${ln}"
      nearest_diff="${diff}"
    fi
  done

  local -a other_lines=()
  for ln in "${match_lines[@]}"; do
    [[ "${ln}" == "${nearest_line}" ]] && continue
    other_lines+=("${ln}")
  done

  echo "WARN  ${doc_file}:${doc_line}"
  echo "      cited:  ${cited_path}:${first}${range_suffix}  (resolved: ${resolved})"
  echo "      symbol: '${name}' not on the cited line(s) anymore -- found elsewhere, looks like drift (a line shifted), not rot"
  echo "      suggested fix: ${cited_path}:${first} → ${cited_path}:${nearest_line}"
  if [[ "${#other_lines[@]}" -gt 0 ]]; then
    local other_ln joined=""
    for other_ln in "${other_lines[@]}"; do
      joined+="${cited_path}:${other_ln}, "
    done
    joined="${joined%, }"
    echo "      NOTE: '${name}' is not unique in this file -- nearest occurrence was picked, but it also appears at: ${joined} (verify by eye for a short/common symbol name)"
  fi
  warn_count=$((warn_count + 1))
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
echo "check_doc_line_refs: ${pass_count} ok, ${warn_count} warned (drift, not rot), ${fail_count} failed, ${skip_count} skipped (unsupported shape or file not in this checkout)"

if [[ "${fail_count}" -gt 0 ]]; then
  echo "FAIL: one or more doc file:line citations do not contain the symbol they claim to document -- see above" >&2
  echo "FALHA: uma ou mais citações arquivo:linha de doc não contêm o símbolo que afirmam documentar -- ver acima" >&2
  exit 1
fi

if [[ "${warn_count}" -gt 0 ]]; then
  echo "NOTE: ${warn_count} citation(s) drifted (line shifted, symbol still present) -- not blocking, but consider updating them when convenient. Re-run with --strict to treat drift as a hard failure."
  echo "NOTA: ${warn_count} citação(ões) sofreram drift (linha deslocou, símbolo continua presente) -- não bloqueante, mas considere atualizá-las quando convier. Rode de novo com --strict pra tratar drift como falha dura."
fi

echo "OK: every checkable doc file:line citation matches its cited source (or has drifted only, see WARN above)"
echo "OK: toda citação arquivo:linha de doc verificável bate com sua fonte citada (ou só sofreu drift, ver WARN acima)"
