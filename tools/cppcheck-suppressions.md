# `tools/cppcheck-suppressions.txt` -- rationale / racional

> **EN:** This is the prose sibling of `tools/cppcheck-suppressions.txt`. The `.txt` file
> itself must stay **DATA-ONLY** -- see the ⚠️ note below for why -- so this doc carries
> everything that used to live as `#`-comments inside it.
> **PT:** Este é o irmão em prosa do `tools/cppcheck-suppressions.txt`. O próprio arquivo
> `.txt` precisa ficar **SÓ-DADO** -- ver a nota ⚠️ abaixo do porquê -- então este doc
> carrega tudo que costumava viver como comentário `#` dentro dele.

## ⚠️ Why the `.txt` has zero `#` comments / Por que o `.txt` tem zero comentário `#`

**EN:** `cppcheck 2.13.0` -- the exact version `ci.yml`'s `ubuntu-latest` runner installs
via `apt-get` -- **rejects `#`-led comment lines inside a `--suppressions-list` file**,
failing hard with `cppcheck: error: Failed to add suppression. No id.` (exit 1, the whole
gate goes red). `cppcheck 2.21.1` -- this repo's own Fedora 44 dev machine's package --
**accepts** the exact same comment lines silently. Measured 2026-08-07
(`CI-CPPCHECK-DIVERGENCIA`), in a `ubuntu:noble` container running the real `2.13.0`
package, after an earlier version of this file (with comments) shipped and broke `ci.yml`
again, one commit after the original divergence was fixed. **This is the same underlying
class of bug the item exists to close**: a local environment (2.21.1) silently accepting
something a CI environment (2.13.0) rejects, discovered only because someone looked at
BOTH, not just the newer one. Do **not** add `#` comments back to the `.txt` file -- if
this repo's cppcheck version floor ever moves past whatever version started accepting
comments in suppression files, this note (and the whole reason this `.md` exists) can be
revisited, but only after re-measuring against whatever `ci.yml` actually installs at that
time, not against this dev machine's own version.

**PT:** O `cppcheck 2.13.0` -- a versão EXATA que o runner `ubuntu-latest` do `ci.yml`
instala via `apt-get` -- **rejeita linhas de comentário `#` dentro de um arquivo
`--suppressions-list`**, falhando duro com `cppcheck: error: Failed to add suppression. No
id.` (exit 1, o gate inteiro fica vermelho). O `cppcheck 2.21.1` -- o pacote da própria
máquina de dev Fedora 44 deste repo -- **aceita** as mesmas linhas de comentário em
silêncio. Medido em 2026-08-07 (`CI-CPPCHECK-DIVERGENCIA`), num container `ubuntu:noble`
rodando o pacote `2.13.0` real, depois que uma versão anterior deste arquivo (com
comentários) embarcou e quebrou o `ci.yml` de novo, um commit depois da divergência
original ter sido consertada. **É a MESMA classe de bug que este item existe pra fechar**:
um ambiente local (2.21.1) aceitando em silêncio algo que um ambiente de CI (2.13.0)
rejeita, descoberto só porque alguém olhou os DOIS, não só o mais novo. **Não** volte a
somar comentário `#` no arquivo `.txt` -- se o piso de versão do cppcheck deste repo um dia
passar da versão que passou a aceitar comentário em arquivo de supressão, esta nota (e a
própria razão deste `.md` existir) pode ser revisitada, mas só depois de re-medir contra o
que o `ci.yml` de fato instala naquele momento, não contra a versão desta máquina de dev.

## What lives in `tools/cppcheck-suppressions.txt` / O que mora no `tools/cppcheck-suppressions.txt`

**EN:** Single source of truth for the cppcheck suppressions shared between the CI gate
(`.github/workflows/ci.yml`, step "TST-L1-STATIC -- cppcheck") and the local pre-commit
gate's `--project`-mode branch (`tools/precommit.sh`, `run_cppcheck_gate()`, the `in_db`
branch). Both invoke cppcheck with `--suppressions-list=<that file>` instead of
maintaining two hand-written `--suppress=` lists (`CI-CPPCHECK-DIVERGENCIA`, 2026-08-07):
the two lists silently drifted apart for 8 CI runs straight (2026-08-06 05:18 through
2026-08-07 11:24) after two file-scoped suppressions were added ONLY to `precommit.sh`'s
own `CPPCHECK_COMMON_ARGS`, on the false premise that CI's `--project` mode "PERMANENTLY"
never looks at `glintfx/src/uix/` -- see the `CHANGELOG.md` entry and `TODO.md` item
`CI-CPPCHECK-DIVERGENCIA` for the full story. Format: one `[error id]:[filename-glob]` per
line (cppcheck's own `--suppress` spec format, verified identical between
`--suppress=<spec>` and `--suppressions-list=<file>` for DATA lines on both cppcheck
2.13.0 and 2.21.1 -- only comment-line handling differs between the two, see above).

**PT:** Fonte única de verdade das supressões de cppcheck compartilhadas entre o gate do
CI (`.github/workflows/ci.yml`, passo "TST-L1-STATIC -- cppcheck") e o ramo de modo
`--project` do gate local de pre-commit (`tools/precommit.sh`, `run_cppcheck_gate()`, o
ramo `in_db`). Os dois invocam o cppcheck com `--suppressions-list=<aquele arquivo>` em
vez de manter duas listas `--suppress=` escritas à mão (`CI-CPPCHECK-DIVERGENCIA`,
2026-08-07): as duas listas divergiram em silêncio por 8 execuções seguidas do CI
(2026-08-06 05:18 até 2026-08-07 11:24) depois que duas supressões restritas a arquivo
foram somadas SÓ ao próprio `CPPCHECK_COMMON_ARGS` do `precommit.sh`, sob a premissa falsa
de que o modo `--project` do CI "PERMANENTEMENTE" nunca olha pra `glintfx/src/uix/` -- ver
a entrada do `CHANGELOG.md` e o item `CI-CPPCHECK-DIVERGENCIA` do `TODO.md` pra história
completa. Formato: um `[id do erro]:[glob de arquivo]` por linha (o próprio formato de
spec do `--suppress` do cppcheck, verificado idêntico entre `--suppress=<spec>` e
`--suppressions-list=<arquivo>` para linhas de DADO nos dois cppcheck 2.13.0 e 2.21.1 --
só o tratamento de linha-comentário difere entre os dois, ver acima).

## Per-entry rationale / Racional por entrada

**EN:**
- `missingIncludeSystem` -- suppresses cppcheck's own note about not being able to find
  system headers (no `-I` for the full system include path is passed; not a real defect).
- `*:*/third_party/*` -- vendored code (`glintfx/third_party/`) is not this project's own
  authorship and out of scope for this gate.
- `*:*/_deps/*` -- fetched dependencies (RmlUi via `FetchContent`, under
  `glintfx/build-lint/_deps/`) -- same rationale as `third_party/`.
- `constParameterCallback:*/image_encode.cpp` (`IMG-ENCODE`, W21) -- `write_cb()`'s
  `void* data` parameter must stay non-const to match stb_image_write's own
  `stbi_write_func` typedef EXACTLY (`void (*)(void*, void*, int)`); constifying it would
  require a `reinterpret_cast<stbi_write_func*>` at every `_to_func` call site just to
  silence a style note about a signature this file does not own -- see
  `image_encode.cpp`'s own doc-comment right above `write_cb()` for the full rationale.

**PT:**
- `missingIncludeSystem` -- suprime a própria nota do cppcheck sobre não conseguir achar
  headers de sistema (nenhum `-I` do caminho de include de sistema completo é passado; não
  é defeito real).
- `*:*/third_party/*` -- código vendorizado (`glintfx/third_party/`) não é autoria própria
  deste projeto e está fora do escopo deste gate.
- `*:*/_deps/*` -- dependências fetchadas (RmlUi via `FetchContent`, sob
  `glintfx/build-lint/_deps/`) -- mesmo racional do `third_party/`.
- `constParameterCallback:*/image_encode.cpp` (`IMG-ENCODE`, W21) -- o parâmetro
  `void* data` de `write_cb()` precisa ficar não-const pra bater EXATAMENTE com o próprio
  typedef `stbi_write_func` do stb_image_write (`void (*)(void*, void*, int)`); tornar
  const forçaria um `reinterpret_cast<stbi_write_func*>` em todo sítio de chamada
  `_to_func` só pra calar uma nota de estilo sobre uma assinatura que este arquivo não
  possui -- ver o próprio comentário de `image_encode.cpp` logo acima de `write_cb()` pro
  racional completo.

## LOCAL-ONLY suppressions that deliberately do NOT belong in the shared `.txt` / Supressões SÓ-LOCAIS que de propósito NÃO entram no `.txt` compartilhado

**EN:** `unusedStructMember:*glintfx/src/uix/*` and `missingInclude` are structurally
specific to `precommit.sh`'s own flag-less, per-file FALLBACK invocation (the `not_in_db`
branch, used only for a brand-new file with no `compile_commands.json` entry yet). CI's
`build-lint` always configures a full `compile_commands.json` before cppcheck runs, so CI
never takes that code path and never needs those two. Kept as direct `--suppress=` flags
on `precommit.sh`'s own fallback invocation (`CPPCHECK_LOCAL_FALLBACK_SUPPRESS`), not in
this shared file. Re-verified live 2026-08-07: `unusedStructMember` is NOT stale -- a
throwaway probe header with no consuming `.cpp` yet still reproduces the exact finding
under the flag-less invocation, protecting a real, recurring editing-order case (header
written before its `.cpp`, this repo's own TDD red/green workflow).

**PT:** `unusedStructMember:*glintfx/src/uix/*` e `missingInclude` são estruturalmente
específicas da invocação de FALLBACK sem flags, por-arquivo, do próprio `precommit.sh` (o
ramo `not_in_db`, usado só pra um arquivo novinho sem entrada em `compile_commands.json`
ainda). O `build-lint` do CI sempre configura um `compile_commands.json` completo antes do
cppcheck rodar, então o CI nunca passa por esse caminho de código e nunca precisa dessas
duas. Mantidas como flags `--suppress=` diretas na própria invocação de fallback do
`precommit.sh` (`CPPCHECK_LOCAL_FALLBACK_SUPPRESS`), não neste arquivo compartilhado.
Reverificada ao vivo em 2026-08-07: `unusedStructMember` NÃO está obsoleta -- um header-
sonda descartável sem `.cpp` consumidor ainda reproduz o achado exato sob a invocação sem
flags, protegendo um caso real e recorrente de ordem de edição (header escrito antes do
`.cpp`, o próprio fluxo TDD red/green desta casa).
