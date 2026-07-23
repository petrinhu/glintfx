#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# EN: Cheap pre-commit gate for glintfx (GATE-PRECOMMIT, TODO.md). Shift-left companion
#     to TST-L1-PRECI (tools/preci.sh, hooked at pre-push): where preci.sh is the wide
#     "before it becomes visible on a remote" net (full build+ctest, ~2min warm),
#     THIS script is the narrow "before it becomes a commit" net -- no build, no test
#     run, only checks cheap enough to pay on every single commit. Born from Onda 3's
#     own retrospective (see the GATE-PRECOMMIT line in TODO.md for the measured data):
#     three rounds of avoidable rework this onda -- a formatting commit that shifted doc
#     line citations, a test commit that skipped clang-format-diff, and a null-deref
#     (draw2d.cpp:378) that only surfaced in CI ~3min later -- all three are catchable in
#     well under a second locally. Invoked by .githooks/pre-commit, or run directly by
#     hand any time (`tools/precommit.sh`).
#
#     ACTIVATION -- TWO VALID ROUTES, pick the one that matches your git config (found
#     the hard way, 2026-07-23 -- read this before assuming the pre-push comment's
#     `git config core.hooksPath .githooks` is universally correct, it is NOT on every
#     machine):
#       (a) Clone with NO `core.hooksPath` set anywhere (git's out-of-the-box default,
#           `.git/hooks/` only): `git config core.hooksPath .githooks` -- git then reads
#           BOTH .githooks/pre-commit and .githooks/pre-push directly. This is what the
#           .githooks/pre-push header describes, and it is correct for a plain clone.
#       (b) Clone where a GLOBAL `core.hooksPath` is already claimed by something else
#           (e.g. this house's own `~/.claude/githooks` -- the TODO.md freshness
#           post-commit hook, see its README.md's "Coexistência com hooks locais"): do
#           NOT override `core.hooksPath` locally -- that would REPLACE the global value
#           wholesale (git has exactly one `core.hooksPath`, it does not chain) and
#           silently kill the global hook's frescor-tracking for this repo. Instead,
#           symlink (or copy) the hook straight into the repo's OWN git-dir hooks
#           folder: `ln -s ../../.githooks/pre-commit .git/hooks/pre-commit`. The global
#           hook's own shim (`~/.claude/githooks/_chain.sh`) already does exactly this
#           delegation for you on every OTHER hook name (`pre-commit`, `pre-push`, ...):
#           it execs `$(git rev-parse --git-common-dir)/hooks/<name>` if that local file
#           is executable, so the two systems COEXIST -- the global frescor hook still
#           fires, AND it hands off to this repo's own pre-commit gate right after. This
#           is the route in use on the líder's machine today (2026-07-23): route (b),
#           `.git/hooks/pre-commit` symlinked, `core.hooksPath` left untouched at its
#           global value; `.git/hooks/pre-push` is NOT yet symlinked the same way, so
#           TST-L1-PRECI is not actually wired on this exact clone until someone
#           symlinks it too -- a pre-existing gap, not something this gate introduced.
#
#     Three gates, STAGED FILES ONLY (never the whole tree -- keeps the cost proportional
#     to what is actually being committed, not to repo size):
#       1) cppcheck on staged glintfx/src/*.cpp|*.hpp -- same flag set as CI's
#          TST-L1-STATIC (3/3) job. Prefers glintfx/build-lint/compile_commands.json
#          when it exists AND lists the file (`--project`, real compile flags, most
#          accurate); falls back to a flag-less per-file invocation (-I glintfx/include
#          -I glintfx/src, missingInclude suppressed) for files the project db does not
#          know about yet -- a brand new staged file, or no configured build at all. This
#          split is deliberate: the hook must never depend on a build existing (that is
#          exactly the moment it is needed most -- a fresh clone, first commit), but
#          should use the accurate mode when it is already sitting there for free.
#          HEADERS (`.hpp`/`.h`) never appear as their own "file" entry in
#          compile_commands.json (they are never their own translation unit) -- routed to
#          `--project` mode too whenever the db exists at all (matching CI's own strategy,
#          which has no such split), NOT to the flag-less fallback, because the flag-less
#          per-file pass analyses a header in ISOLATION and cannot see how the REST of the
#          project (some `.cpp` TU that `#include`s it) actually uses its members --
#          `unusedStructMember` false-flagged exactly this way on `sprite_batch.hpp`'s
#          `Flush::vertices` (read only from `draw2d.cpp`'s `drain_ready()`, a DIFFERENT
#          TU) the day D2D-2B first staged that file since GATE-PRECOMMIT went in,
#          confirmed reproducible in isolation and confirmed CI's own `--project`-only
#          invocation does NOT hit it. `.cpp` files keep the exact-match rule (a header is
#          virtually always reachable via inclusion from an existing TU already in the
#          db; a brand-new, not-yet-configured `.cpp` genuinely has no TU to analyse it
#          through, so it still needs the flag-less fallback for at least some coverage).
#       2) clang-format-diff on the STAGED diff (`git diff --cached`, not origin/main --
#          see rationale below) of touched glintfx/*.cpp|*.hpp|*.h, third_party/ and the
#          generated gl_loader.{h,c} excluded (same exclusions as TST-L1-FORMAT in
#          preci.sh). A staged brand-new file already shows up as a full-file addition
#          in `git diff --cached`, so unlike preci.sh's format gate there is no separate
#          "untracked whole-file" branch needed here -- everything under review here IS
#          staged, by construction.
#       3) tools/check_doc_line_refs.sh, but ONLY when the staged set is plausibly
#          relevant to it (a doc file itself is staged, or some staged file's basename
#          is cited by a doc under docs/) -- see the run_doc_refs_gate() comment for why
#          this one gate, unlike the other two, is worth gating at all: it is the
#          slowest of the three (measured ~1.6s over the whole docs/ tree, independent
#          of which file changed) yet the vast majority of commits touch nothing any doc
#          cites, so skipping it in the common case is what keeps the hook's typical
#          cost near the cppcheck+format budget instead of always paying the full 1.6s.
#
#     WHY `git diff --cached` and not `origin/main` (unlike preci.sh's pre-push gate):
#     pre-push's question is "what is about to become visible on a remote" -> diff
#     against the last shared point (origin/main). pre-commit's question is "what is
#     about to become a commit" -> diff against the index's parent, HEAD -- i.e. exactly
#     the staged diff, `git diff --cached`. Using origin/main here would flag lines this
#     commit does not even touch (already-committed-but-unpushed drift), which is not
#     this commit's fault to fix and would make the gate noisy for a chain of small
#     commits ahead of origin/main.
#
#     POLICY (same as preci.sh / .githooks/pre-push): a MISSING optional tool
#     (cppcheck, clang-format, clang-format-diff) is a WARNING and a skip of that one
#     gate, never a hard failure -- a hook that blocks commits for people without the
#     tool installed is a hook that gets `--no-verify`d into irrelevance, or removed.
#
# PT: Gate barato de pre-commit para o glintfx (GATE-PRECOMMIT, TODO.md). Companheiro
#     shift-left do TST-L1-PRECI (tools/preci.sh, enganchado no pre-push): onde o
#     preci.sh é a rede larga "antes de ficar visível num remoto" (build+ctest
#     completo, ~2min warm), ESTE script é a rede estreita "antes de virar commit" --
#     sem build, sem rodar teste, só checks baratos o bastante pra pagar em TODO commit.
#     Nasceu da própria retrospectiva da Onda 3 (ver a linha GATE-PRECOMMIT no TODO.md
#     pro dado medido): três rodadas de retrabalho evitável nesta onda -- um commit de
#     formatação que deslocou citações de linha de doc, um commit de teste que pulou o
#     clang-format-diff, e um null-deref (draw2d.cpp:378) que só apareceu no CI ~3min
#     depois -- as três são pegáveis localmente em bem menos de um segundo. Invocado por
#     .githooks/pre-commit, ou rodado direto à mão (`tools/precommit.sh`).
#
#     ATIVAÇÃO -- DUAS ROTAS VÁLIDAS, escolha a que bate com a sua config git (achado
#     na marra, 2026-07-23 -- leia isto antes de assumir que o `git config
#     core.hooksPath .githooks` do comentário do pre-push vale universalmente, NÃO vale
#     em toda máquina):
#       (a) Clone SEM `core.hooksPath` setado em lugar nenhum (o default de fábrica do
#           git, só `.git/hooks/`): `git config core.hooksPath .githooks` -- o git então
#           lê DIRETO .githooks/pre-commit e .githooks/pre-push. É o que o cabeçalho do
#           .githooks/pre-push descreve, e está correto pra um clone simples.
#       (b) Clone onde um `core.hooksPath` GLOBAL já está reivindicado por outra coisa
#           (ex.: o `~/.claude/githooks` desta casa -- o hook post-commit de frescor do
#           TODO.md, ver "Coexistência com hooks locais" no README.md dele): NÃO
#           sobrescreva `core.hooksPath` localmente -- isso SUBSTITUIRIA o valor global
#           por inteiro (o git tem exatamente um `core.hooksPath`, não encadeia) e mataria
#           silenciosamente o rastreio de frescor do hook global pra este repo. Em vez
#           disso, symlink (ou copie) o hook direto pro diretório PRÓPRIO de hooks do
#           git-dir do repo: `ln -s ../../.githooks/pre-commit .git/hooks/pre-commit`. O
#           próprio shim do hook global (`~/.claude/githooks/_chain.sh`) já faz
#           exatamente essa delegação pra você em todo OUTRO nome de hook (`pre-commit`,
#           `pre-push`, ...): ele executa `$(git rev-parse --git-common-dir)/hooks/<nome>`
#           se esse arquivo local for executável, então os dois sistemas COEXISTEM -- o
#           hook global de frescor continua disparando, E ele repassa pro gate de
#           pre-commit deste repo logo em seguida. Esta é a rota em uso na máquina do
#           líder hoje (2026-07-23): rota (b), `.git/hooks/pre-commit` symlinkado,
#           `core.hooksPath` intocado no valor global; `.git/hooks/pre-push` AINDA NÃO
#           está symlinkado do mesmo jeito, então o TST-L1-PRECI não está de fato
#           enganchado nesta clone exata até alguém symlinkar também -- lacuna
#           pré-existente, não algo que este gate introduziu.
#
#     Três gates, SÓ ARQUIVOS STAGED (nunca a árvore inteira -- mantém o custo
#     proporcional ao que de fato está sendo commitado, não ao tamanho do repo):
#       1) cppcheck nos glintfx/src/*.cpp|*.hpp staged -- mesmo conjunto de flags do job
#          TST-L1-STATIC (3/3) do CI. Prefere
#          glintfx/build-lint/compile_commands.json quando existe E lista o arquivo
#          (`--project`, flags de compilação reais, mais preciso); cai pra uma invocação
#          por-arquivo sem flags de projeto (-I glintfx/include -I glintfx/src,
#          missingInclude suprimido) pros arquivos que o project db ainda não conhece --
#          um arquivo staged novinho, ou nenhum build configurado. Essa divisão é
#          deliberada: o hook nunca pode depender de um build existir (é exatamente o
#          momento em que ele mais faz falta -- clone novo, primeiro commit), mas deve
#          usar o modo preciso quando ele já está ali de graça.
#          HEADERS (`.hpp`/`.h`) nunca aparecem como a própria entrada "file" no
#          compile_commands.json (nunca são a própria unidade de tradução) -- roteados
#          pro modo `--project` também sempre que o db existir (batendo com a própria
#          estratégia do CI, que não tem essa divisão), NÃO pro fallback sem flags,
#          porque o passe por-arquivo sem flags analisa um header ISOLADO e não consegue
#          ver como o RESTO do projeto (algum `.cpp` TU que faz `#include` dele) de fato
#          usa os membros dele -- `unusedStructMember` deu falso-positivo exatamente
#          assim em `Flush::vertices` de `sprite_batch.hpp` (lido só pelo
#          `drain_ready()` de `draw2d.cpp`, uma TU DIFERENTE) no dia em que o D2D-2B
#          staged esse arquivo pela primeira vez desde que o GATE-PRECOMMIT entrou,
#          confirmado reproduzível isolado e confirmado que a própria invocação
#          `--project`-só do CI NÃO bate nisso. Arquivos `.cpp` mantêm a regra de match
#          exato (um header é quase sempre alcançável via inclusão de uma TU já
#          existente no db; um `.cpp` novinho, ainda não configurado, de fato não tem TU
#          nenhuma pra analisá-lo através dela, então ainda precisa do fallback sem
#          flags pra ao menos alguma cobertura).
#       2) clang-format-diff no diff STAGED (`git diff --cached`, não origin/main -- ver
#          racional abaixo) dos glintfx/*.cpp|*.hpp|*.h tocados, excluindo third_party/
#          e o gl_loader.{h,c} gerado (mesmas exclusões do TST-L1-FORMAT no preci.sh).
#          Um arquivo novo staged já aparece como adição de arquivo inteiro no
#          `git diff --cached`, então, diferente do gate de formatação do preci.sh, não
#          precisa de um branch separado "arquivo inteiro não rastreado" aqui -- tudo em
#          revisão aqui JÁ está staged, por construção.
#       3) tools/check_doc_line_refs.sh, mas SÓ quando o conjunto staged é plausivelmente
#          relevante pra ele (um arquivo de doc em si está staged, ou o basename de algum
#          arquivo staged é citado por uma doc sob docs/) -- ver o comentário de
#          run_doc_refs_gate() pro porquê ESTE gate, diferente dos outros dois, vale a
#          pena gatear: é o mais lento dos três (medido ~1,6s sobre a árvore docs/
#          inteira, independente de qual arquivo mudou), mas a grande maioria dos commits
#          não toca nada que alguma doc cite, então pular no caso comum é o que mantém o
#          custo típico do hook perto do orçamento cppcheck+format em vez de sempre pagar
#          o 1,6s inteiro.
#
#     POR QUE `git diff --cached` e não `origin/main` (diferente do gate de pre-push do
#     preci.sh): a pergunta do pre-push é "o que está prestes a ficar visível num
#     remoto" -> diff contra o último ponto compartilhado (origin/main). A pergunta do
#     pre-commit é "o que está prestes a virar commit" -> diff contra o pai do índice,
#     HEAD -- ou seja, exatamente o diff staged, `git diff --cached`. Usar origin/main
#     aqui sinalizaria linhas que este commit nem toca (deriva já commitada mas ainda
#     não pushada), o que não é responsabilidade deste commit consertar e deixaria o
#     gate ruidoso numa cadeia de commits pequenos à frente do origin/main.
#
#     POLÍTICA (igual preci.sh / .githooks/pre-push): uma ferramenta opcional AUSENTE
#     (cppcheck, clang-format, clang-format-diff) é um AVISO e um skip daquele gate,
#     nunca uma falha dura -- um hook que bloqueia commit de quem não tem a ferramenta
#     instalada é um hook que vira `--no-verify` de hábito, ou é removido.
#
# Usage / Uso: tools/precommit.sh [--help]
#
# Exit status: non-zero if ANY gate found a real problem in the staged files (blocks the
#              pre-commit hook). A missing optional tool prints a warning and exits 0 for
#              that gate. Escape hatch for a single commit: `git commit --no-verify` --
#              this is an anti-pattern in this house (see CLAUDE.md), not a habit; use it
#              only for a genuine one-off, never as a way to routinely skip the gate.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  grep -E '^# ' "${BASH_SOURCE[0]}" | sed -E 's/^# ?//'
  exit 0
fi

t_start_ns="$(date +%s%N)"

section() {
  echo ""
  echo "== precommit: $1 =="
}

gate_failed=0

# -----------------------------------------------------------------------------
# EN: staged files this commit is about (Added/Copied/Modified/Renamed -- Deleted
#     excluded, nothing left on disk to check). --name-only on a rename shows only the
#     new path, which is what every gate below wants.
# PT: arquivos staged deste commit (Added/Copied/Modified/Renamed -- Deleted excluído,
#     nada em disco pra checar). --name-only numa renomeação mostra só o caminho novo,
#     que é o que todo gate abaixo quer.
# -----------------------------------------------------------------------------
mapfile -t staged_files < <(git diff --cached --name-only --diff-filter=ACMR -- .)

if [[ "${#staged_files[@]}" -eq 0 ]]; then
  echo "precommit: nothing staged -- nothing to gate."
  exit 0
fi

# -----------------------------------------------------------------------------
# EN: 1) cppcheck on staged glintfx/src/*.cpp|*.hpp -- same flags as CI's
#     TST-L1-STATIC (3/3).
# PT: 1) cppcheck nos glintfx/src/*.cpp|*.hpp staged -- mesmas flags do
#     TST-L1-STATIC (3/3) do CI.
# -----------------------------------------------------------------------------
CPPCHECK_COMMON_ARGS=(
  --enable=warning,style,performance,portability
  --std=c++20 --language=c++ --platform=unix64
  --suppress=missingIncludeSystem
  --suppress='*:*/third_party/*'
  --suppress='*:*/_deps/*'
  --error-exitcode=1
)

run_cppcheck_gate() {
  if ! command -v cppcheck >/dev/null 2>&1; then
    echo "warning: 'cppcheck' not found in PATH -- skipping the cppcheck gate (install it, see TOOLING.md)" >&2
    return 0
  fi

  local -a targets=()
  local f
  for f in "${staged_files[@]}"; do
    case "${f}" in
      glintfx/src/*.cpp|glintfx/src/*.hpp) ;;
      *) continue ;;
    esac
    [[ -f "${f}" ]] || continue
    targets+=("${f}")
  done

  [[ "${#targets[@]}" -eq 0 ]] && return 0

  section "cppcheck -- glintfx/src/ (${#targets[@]} file(s) staged)"

  local compiledb="glintfx/build-lint/compile_commands.json"
  local -a in_db=() not_in_db=()
  if [[ -f "${compiledb}" ]]; then
    for f in "${targets[@]}"; do
      case "${f}" in
        # EN: A header (.hpp/.h) is never its own "file" entry (never its own TU) --
        #     route it through --project mode too whenever the db exists at all, same
        #     as CI's own strategy (no such split there). The flag-less per-file
        #     fallback analyses a header in ISOLATION and cannot see cross-TU usage of
        #     its members from whichever .cpp #includes it -- confirmed to false-flag
        #     unusedStructMember this exact way (D2D-2B, sprite_batch.hpp).
        # PT: Um header (.hpp/.h) nunca é a própria entrada "file" (nunca é a própria
        #     TU) -- roteado pro modo --project também sempre que o db existir, mesma
        #     estratégia do CI (sem essa divisão lá). O fallback sem flags analisa um
        #     header ISOLADO e não enxerga uso cross-TU dos membros dele pelo .cpp que
        #     faz #include -- confirmado dar falso-positivo de unusedStructMember
        #     exatamente assim (D2D-2B, sprite_batch.hpp).
        *.hpp|*.h) in_db+=("${f}") ;;
        *)
          if grep -qF "${f}\"" "${compiledb}" 2>/dev/null; then
            in_db+=("${f}")
          else
            not_in_db+=("${f}")
          fi
          ;;
      esac
    done
  else
    not_in_db=("${targets[@]}")
  fi

  local failed=0

  if [[ "${#in_db[@]}" -gt 0 ]]; then
    local -a filter_args=()
    for f in "${in_db[@]}"; do
      filter_args+=(--file-filter="*${f}")
    done
    cppcheck "${CPPCHECK_COMMON_ARGS[@]}" --project="${compiledb}" "${filter_args[@]}" || failed=1
  fi

  if [[ "${#not_in_db[@]}" -gt 0 ]]; then
    echo "precommit: ${#not_in_db[@]} file(s) not in ${compiledb} (new file, or no configured build) -- falling back to a flag-less per-file check" >&2
    cppcheck "${CPPCHECK_COMMON_ARGS[@]}" --suppress=missingInclude \
      -I glintfx/include -I glintfx/src \
      "${not_in_db[@]}" || failed=1
  fi

  return "${failed}"
}

if ! run_cppcheck_gate; then
  gate_failed=1
fi

# -----------------------------------------------------------------------------
# EN: 2) clang-format-diff on the staged diff of touched glintfx C++ files.
# PT: 2) clang-format-diff no diff staged dos arquivos C++ do glintfx tocados.
# -----------------------------------------------------------------------------
run_format_gate() {
  if ! command -v clang-format >/dev/null 2>&1 || ! command -v clang-format-diff >/dev/null 2>&1; then
    echo "warning: 'clang-format'/'clang-format-diff' not found in PATH -- skipping the format gate (install clang-tools, see TOOLING.md)" >&2
    return 0
  fi

  local -a targets=()
  local f
  for f in "${staged_files[@]}"; do
    case "${f}" in
      glintfx/third_party/*) continue ;;
      glintfx/src/gl_loader.h|glintfx/src/gl_loader.c) continue ;;
      glintfx/*.cpp|glintfx/*.hpp|glintfx/*.h) ;;
      *) continue ;;
    esac
    [[ -f "${f}" ]] || continue
    targets+=("${f}")
  done

  [[ "${#targets[@]}" -eq 0 ]] && return 0

  section "clang-format-diff -- staged lines (${#targets[@]} file(s))"

  local diff_out
  diff_out="$(git diff --cached -U0 -- "${targets[@]}" | clang-format-diff -p1 -style=file || true)"
  if [[ -n "${diff_out}" ]]; then
    echo "${diff_out}"
    echo "error: clang-format-diff flagged formatting drift in staged lines of: ${targets[*]}" >&2
    return 1
  fi

  return 0
}

if ! run_format_gate; then
  gate_failed=1
fi

# -----------------------------------------------------------------------------
# EN: 3) check_doc_line_refs.sh, gated on relevance. See the header comment above for
#     why this one is conditional and the other two are not: it scans ALL of docs/
#     regardless of which file changed (~1.6s measured), so the win is skipping the
#     whole run, not narrowing its input. "Relevant" = a doc file itself is staged, OR
#     some staged file's basename is cited somewhere under docs/ AS AN ACTUAL
#     `path:N`-SHAPED CITATION -- a cheap grep, not the full symbol-aware verifier, but
#     matching the SAME SHAPE check_doc_line_refs.sh itself parses (basename immediately
#     followed by `:<digits>`), not a bare substring mention.
#
#     PLAIN-SUBSTRING FALSE-POSITIVE, FOUND AND FIXED (2026-07-23, live test by the
#     team-lead): the first cut of this gate grepped for the basename ALONE, so staging
#     `TODO.md` (which this house's own convention says to touch on nearly every commit
#     that closes a TODO item -- see CLAUDE.md's "citar o ID nos commits") matched every
#     one of the many *plain mentions* of "TODO.md" across docs/ (`docs/draw2d.md`,
#     `docs/audio.md`, `docs/adr/0009-...`, `docs/wiki/Layer-0-Core.md`, ...) even though
#     NONE of them is a `TODO.md:N` line citation -- confirmed empirically: zero hits for
#     `grep -E 'TODO\.md:[0-9]'` across the whole docs/ tree. The bare-substring version
#     turned the common case (touch TODO.md + one .cpp) into the expensive path
#     (~4.1s measured), exactly backwards from the intent. Requiring `:<digits>`
#     immediately after the basename is the fix: it is the one syntactic marker that
#     distinguishes "this doc is CITING a file:line" from "this doc merely MENTIONS a
#     filename", and it costs nothing extra (still one grep pass, same doc_files list).
# PT: 3) check_doc_line_refs.sh, gateado por relevância. Ver o comentário de cabeçalho
#     acima pro porquê este é condicional e os outros dois não: ele varre TODO o docs/
#     independente de qual arquivo mudou (~1,6s medido), então o ganho é pular a rodada
#     inteira, não estreitar a entrada dela. "Relevante" = um arquivo de doc em si está
#     staged, OU o basename de algum arquivo staged é citado em algum lugar sob docs/
#     COMO UMA CITAÇÃO DE VERDADE no formato `caminho:N` -- um grep barato, não o
#     verificador completo ciente de símbolo, mas casando a MESMA FORMA que o próprio
#     check_doc_line_refs.sh parseia (basename imediatamente seguido de `:<dígitos>`),
#     não uma menção crua de substring.
#
#     FALSO-POSITIVO DE SUBSTRING CRUA, ACHADO E CORRIGIDO (2026-07-23, teste ao vivo do
#     líder de equipe): a primeira versão deste gate grepava só o basename SOZINHO,
#     então staged `TODO.md` (que a própria convenção da casa manda tocar em quase todo
#     commit que fecha um item do TODO -- ver "citar o ID nos commits" do CLAUDE.md)
#     casava com toda MENÇÃO crua de "TODO.md" pelo docs/ (`docs/draw2d.md`,
#     `docs/audio.md`, `docs/adr/0009-...`, `docs/wiki/Layer-0-Core.md`, ...) mesmo
#     nenhuma delas sendo uma citação `TODO.md:N` -- confirmado empiricamente: zero
#     ocorrências de `grep -E 'TODO\.md:[0-9]'` na árvore docs/ inteira. A versão de
#     substring crua transformou o caso comum (tocar TODO.md + um .cpp) no caminho caro
#     (~4,1s medido), exatamente o oposto da intenção. Exigir `:<dígitos>` logo após o
#     basename é o conserto: é o único marcador sintático que distingue "esta doc está
#     CITANDO um arquivo:linha" de "esta doc só MENCIONA um nome de arquivo", e não
#     custa nada a mais (ainda um único passe de grep, mesma lista doc_files).
# -----------------------------------------------------------------------------
run_doc_refs_gate() {
  local f relevant=0

  for f in "${staged_files[@]}"; do
    case "${f}" in
      docs/*.md|docs/adr/*.md|docs/wiki/*.md) relevant=1; break ;;
    esac
  done

  if [[ "${relevant}" -eq 0 ]]; then
    local -a doc_files=()
    mapfile -t doc_files < <(
      {
        find docs -maxdepth 1 -name '*.md' -type f 2>/dev/null
        find docs/adr docs/wiki -maxdepth 1 -name '*.md' -type f 2>/dev/null
      } | sort -u
    )
    if [[ "${#doc_files[@]}" -gt 0 ]]; then
      for f in "${staged_files[@]}"; do
        local base base_re
        base="$(basename "${f}")"
        # EN: escape ERE metacharacters in the basename (mainly '.') before using it in
        #     grep -E -- a filename like "app.hpp" must match the LITERAL dot, not "any
        #     char". PT: escapa metacaracteres de ERE no basename (principalmente '.')
        #     antes de usar no grep -E -- um nome de arquivo tipo "app.hpp" precisa
        #     casar o ponto LITERAL, não "qualquer caractere".
        base_re="$(printf '%s' "${base}" | sed -e 's/[.[\^$*+?(){}|\\]/\\&/g')"
        if grep -qlE -- "${base_re}:[0-9]" "${doc_files[@]}" 2>/dev/null; then
          relevant=1
          break
        fi
      done
    fi
  fi

  [[ "${relevant}" -eq 0 ]] && return 0

  section "check_doc_line_refs.sh (staged set touches a doc-cited file)"
  "${SCRIPT_DIR}/check_doc_line_refs.sh" || return 1
  return 0
}

if ! run_doc_refs_gate; then
  gate_failed=1
fi

# -----------------------------------------------------------------------------
# EN: report + exit.
# PT: relatório + saída.
# -----------------------------------------------------------------------------
t_end_ns="$(date +%s%N)"
elapsed_ms=$(( (t_end_ns - t_start_ns) / 1000000 ))

echo ""
if [[ "${gate_failed}" -eq 1 ]]; then
  echo "== precommit: FAILED (${elapsed_ms}ms) -- fix the above, or 'git commit --no-verify' for a genuine one-off (not a habit) =="
  exit 1
fi

echo "== precommit: OK (${elapsed_ms}ms) =="
