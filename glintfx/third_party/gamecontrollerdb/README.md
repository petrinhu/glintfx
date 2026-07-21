# glintfx/third_party/gamecontrollerdb -- vendored Linux subset of SDL_GameControllerDB (EN)

This directory vendors a **filtered, Linux-only subset** of the community-maintained
[SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) mapping database, as
**DATA**, for the `GLINTFX_MODULE_GAMEPAD` atom (`A2-GAMEPAD`,
[ADR-0015](../../../docs/adr/0015-framework-2d-atomized-architecture.md) (b) "gamepad",
[ADR-0016](../../../docs/adr/0016-gamepad-atom.md)). No SDL code is compiled, linked, or read
here -- the file is treated purely as text: `guid,name,field:value,...,platform:Linux` lines,
consumed by glintfx's own clean-room parser (`glintfx/src/gamepad_mapping.cpp`).

## Why data, not a dependency

The gamepad atom's whole point is to talk to `/dev/input/event*` directly via raw evdev --
**zero new dependency** (no `libSDL`, no `libudev`, no `libevdev`; leader's decision, see the
A2-GAMEPAD plan). But the *mapping* problem it solves (which raw button/axis code means "South
button" / "left stick X" on a given controller) is exactly what SDL's community-maintained
database already answers for tens of thousands of real devices. Vendoring that text -- and
parsing it ourselves -- gets the mapping coverage without the SDL dependency. This is the same
posture as `glintfx/third_party/khronos/gl.xml` (Apache-2.0 data input to
`tools/gen_glloader.py`, not a linked library): a public data file feeding a from-scratch
generator/parser glintfx owns end to end.

## Pinned snapshot

| Field | Value |
|---|---|
| Upstream repository | `https://github.com/mdqinc/SDL_GameControllerDB` (renamed from `gabomdq/SDL_GameControllerDB`; the old URL redirects here) |
| Source file | `gamecontrollerdb.txt` |
| Pinned commit | [`8d9fefd7b810f2541f78cc7a8ccbd185bc84c7a5`](https://github.com/mdqinc/SDL_GameControllerDB/commit/8d9fefd7b810f2541f78cc7a8ccbd185bc84c7a5) (2026-07-15T21:29:38Z, "Add Xbox Series X Controller mapping for Linux (#966)") |
| Fetched on | 2026-07-21 |
| Full upstream file SHA-256 (pre-filter) | `dd4dd9dcb458aa4fbfd9b37ccdd4884b1e2e258edf8a16c3c4df3e77ac5174a0` |
| Upstream entries (total / `platform:Linux`) | 2244 platform-tagged / **728** |
| `gamecontrollerdb_linux.txt` (this dir) | 728 data lines + provenance header comment, 743 lines total |

Fetched via the exact pinned commit's raw URL (not `master`), byte-verified identical to a
same-day `master` fetch (no intervening commit touched the file between fetch and pin).

## What is vendored, and how

`gamecontrollerdb_linux.txt` is **NOT** a straight copy of upstream: it is a one-time,
by-hand-documented filter pass over the pinned upstream file (`curl` the exact commit, `grep`
lines whose trailing field is `platform:Linux,` -- 728 of them -- prepend a provenance comment
block: pinned commit, fetch date, upstream SHA-256, filter rule; see "Re-syncing" below for the
exact recipe), analogous to how `glintfx/third_party/khronos/gl.xml` is fetched by hand (see
`tools/gen_glloader.py`'s own header) rather than by its generator script. `tools/gen_gamepad_db.py`
then reads this already-vendored, already-filtered `.txt` -- its ONLY input -- and emits
`gamecontrollerdb_linux.inc`, a generated (do-not-hand-edit) C byte array
(`glintfx_gamecontrollerdb_linux[]` + `_len`) that `glintfx::Gamepads` embeds when
`GamepadsConfig::use_builtin_db` is `true` (the default) -- see `glintfx/docs/gamepad.md`. The
generator does not fetch, filter, or touch the network at all -- same posture as
`gen_glloader.py` reading the already-vendored `gl.xml`.

## Re-syncing (bumping the pin)

Two separate steps -- re-vendoring the filtered `.txt` (manual, by hand, like `gl.xml`) and
regenerating the `.inc` (scripted, `tools/gen_gamepad_db.py`):

1. Pick a new commit on `https://github.com/mdqinc/SDL_GameControllerDB` (prefer one that has
   settled -- avoid pinning mid-review PR commits).
2. `curl -sSL -o /tmp/gcdb.txt "https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/<new-commit-sha>/gamecontrollerdb.txt"`
3. `sha256sum /tmp/gcdb.txt` -- record the new hash.
4. Re-filter: `grep ',platform:Linux,' /tmp/gcdb.txt` -- overwrite `gamecontrollerdb_linux.txt`
   with a fresh provenance comment block (new commit SHA, date, upstream SHA-256, entry counts)
   followed by the filtered lines. Never hand-edit the data lines themselves.
5. Regenerate the `.inc` from the new `.txt`: `python3 tools/gen_gamepad_db.py` (reads
   `gamecontrollerdb_linux.txt`, writes `gamecontrollerdb_linux.inc` -- deterministic, same
   input byte-for-byte produces the same output).
6. Update the pinned-snapshot table above (commit, date, SHA-256, entry counts) and this
   README's mentions of the old commit.
7. Re-check the LICENSE file below hasn't changed (it has not, historically -- zlib since the
   project's inception); if it has, review the new terms before committing.
8. Diff-review the regenerated `.txt`/`.inc` before committing -- confirm the entry-count delta
   looks like real upstream additions/fixes, not an accidental full-DB (non-Linux) leak from a
   broken filter.

## Why data, not dependency (recap for the license/audit trail)

Nothing in `gamecontrollerdb_linux.txt`/`.inc` is executable code, and no upstream SDL header or
source file is read, compiled, or linked anywhere in this repository. The parser that
interprets this text (`glintfx/src/gamepad_mapping.cpp`) is glintfx's own clean-room
implementation of the well-documented, informally-specified `gamecontrollerdb.txt` line format
(`guid,name,bN/aN/hH.M/...,platform:Linux,`) -- written from understanding the format, not by
reading SDL's own parser source.

## License

**zlib License** (see `LICENSE` in this directory, fetched from the same pinned commit,
byte-identical to a same-day fetch of upstream's `LICENSE` at the repository root). Permits use,
modification, and redistribution, including as vendored, filtered data inside a differently
(MPL-2.0) licensed project, with attribution (this README + the repository's `NOTICE`) and
without misrepresenting the filtered subset as glintfx's own original mapping data. Listed in
the repository's `NOTICE`.

## What links against it

Nothing links against this directory in the compiled sense -- it feeds
`tools/gen_gamepad_db.py` (offline, at development time, not at build/configure time) which
emits `gamecontrollerdb_linux.inc`, `#include`d by `glintfx/src/gamepad.cpp` (Agent B's device
layer) under `#if GLINTFX_MODULE_GAMEPAD` and gated further at runtime by
`GamepadsConfig::use_builtin_db`. `OFF` on the module removes both the generated array and its
symbol from the archive, same discipline as every other atom's vendored/generated data (compare
`glintfx/third_party/khronos/gl.xml` -> `tools/gen_glloader.py` -> `gl_loader.{h,c}`).

---

# glintfx/third_party/gamecontrollerdb -- subconjunto Linux vendorizado do SDL_GameControllerDB (PT)

Este diretório vendoriza um **subconjunto filtrado, só-Linux**, do banco de dados de mapeamento
mantido pela comunidade [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB),
como **DADO**, para o átomo `GLINTFX_MODULE_GAMEPAD` (`A2-GAMEPAD`,
[ADR-0015](../../../docs/adr/0015-framework-2d-atomized-architecture.md) (b) "gamepad",
[ADR-0016](../../../docs/adr/0016-gamepad-atom.md)). Nenhum código SDL é compilado, linkado ou
lido aqui -- o arquivo é tratado puramente como texto: linhas
`guid,name,campo:valor,...,platform:Linux`, consumidas pelo parser clean-room próprio do glintfx
(`glintfx/src/gamepad_mapping.cpp`).

## Por que dado, não dependência

O ponto inteiro do átomo gamepad é falar com `/dev/input/event*` diretamente via evdev cru --
**zero dependência nova** (sem `libSDL`, sem `libudev`, sem `libevdev`; decisão do líder, ver o
plano A2-GAMEPAD). Mas o problema de *mapeamento* que ele resolve (qual código cru de
botão/eixo significa "botão South" / "eixo X do stick esquerdo" num controle específico) é
exatamente o que o banco mantido pela comunidade do SDL já responde para dezenas de milhares de
devices reais. Vendorizar esse texto -- e parsear por conta própria -- ganha a cobertura de
mapeamento sem a dependência do SDL. É a mesma postura de
`glintfx/third_party/khronos/gl.xml` (entrada de dado Apache-2.0 pro `tools/gen_glloader.py`,
não uma biblioteca linkada): um arquivo de dado público alimentando um gerador/parser do zero
que o glintfx possui ponta a ponta.

## Snapshot pinado

| Campo | Valor |
|---|---|
| Repositório upstream | `https://github.com/mdqinc/SDL_GameControllerDB` (renomeado de `gabomdq/SDL_GameControllerDB`; a URL antiga redireciona pra cá) |
| Arquivo-fonte | `gamecontrollerdb.txt` |
| Commit pinado | [`8d9fefd7b810f2541f78cc7a8ccbd185bc84c7a5`](https://github.com/mdqinc/SDL_GameControllerDB/commit/8d9fefd7b810f2541f78cc7a8ccbd185bc84c7a5) (2026-07-15T21:29:38Z, "Add Xbox Series X Controller mapping for Linux (#966)") |
| Baixado em | 2026-07-21 |
| SHA-256 do arquivo upstream completo (pré-filtro) | `dd4dd9dcb458aa4fbfd9b37ccdd4884b1e2e258edf8a16c3c4df3e77ac5174a0` |
| Entradas upstream (total / `platform:Linux`) | 2244 com tag de plataforma / **728** |
| `gamecontrollerdb_linux.txt` (este dir) | 728 linhas de dado + cabeçalho de comentário de procedência, 743 linhas no total |

Baixado via a URL raw do commit exato pinado (não `master`), verificado byte-idêntico a um
fetch de `master` no mesmo dia (nenhum commit intermediário tocou o arquivo entre o fetch e o
pin).

## O que é vendorizado, e como

`gamecontrollerdb_linux.txt` **NÃO** é uma cópia direta do upstream: é o resultado de uma
passada de filtro única, documentada à mão (`curl` no commit exato pinado, `grep` das linhas
cujo campo final é `platform:Linux,` -- 728 delas -- prependendo um bloco de comentário de
procedência: commit pinado, data do fetch, SHA-256 do upstream, regra de filtro; ver
"Re-sincronizando" abaixo pela receita exata), análogo a como
`glintfx/third_party/khronos/gl.xml` é baixado à mão (ver o próprio cabeçalho do
`tools/gen_glloader.py`) em vez de pelo script gerador dele. O `tools/gen_gamepad_db.py` então
lê esse `.txt` já vendorizado, já filtrado -- sua ÚNICA entrada -- e emite
`gamecontrollerdb_linux.inc`, um array de bytes C gerado (não editar à mão)
(`glintfx_gamecontrollerdb_linux[]` + `_len`) que o `glintfx::Gamepads` embute quando
`GamepadsConfig::use_builtin_db` é `true` (o default) -- ver `glintfx/docs/gamepad.md`. O
gerador não busca, filtra nem toca a rede em momento nenhum -- mesma postura do
`gen_glloader.py` lendo o `gl.xml` já vendorizado.

## Re-sincronizando (atualizando o pin)

Dois passos separados -- re-vendorizar o `.txt` filtrado (manual, à mão, como o `gl.xml`) e
regenerar o `.inc` (via script, `tools/gen_gamepad_db.py`):

1. Escolha um commit novo em `https://github.com/mdqinc/SDL_GameControllerDB` (prefira um que já
   assentou -- evite pinar commits de PR em revisão).
2. `curl -sSL -o /tmp/gcdb.txt "https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/<novo-commit-sha>/gamecontrollerdb.txt"`
3. `sha256sum /tmp/gcdb.txt` -- registre o novo hash.
4. Refiltre: `grep ',platform:Linux,' /tmp/gcdb.txt` -- sobrescreva `gamecontrollerdb_linux.txt`
   com um bloco de comentário de procedência novo (SHA do commit novo, data, SHA-256 do
   upstream, contagens de entrada) seguido das linhas filtradas. Nunca edite as linhas de dado
   em si à mão.
5. Regenere o `.inc` a partir do `.txt` novo: `python3 tools/gen_gamepad_db.py` (lê
   `gamecontrollerdb_linux.txt`, escreve `gamecontrollerdb_linux.inc` -- determinístico, a
   mesma entrada byte-a-byte produz a mesma saída).
6. Atualize a tabela de snapshot pinado acima (commit, data, SHA-256, contagens de entrada) e as
   menções deste README ao commit antigo.
7. Reverifique que o arquivo LICENSE abaixo não mudou (historicamente não mudou -- zlib desde a
   origem do projeto); se mudou, revise os novos termos antes de commitar.
8. Faça diff-review do `.txt`/`.inc` regenerados antes de commitar -- confirme que o delta de
   contagem de entradas parece adições/correções reais do upstream, não um vazamento acidental
   do banco INTEIRO (não-Linux) por um filtro quebrado.

## Por que dado, não dependência (recapitulando pro rastro de licença/auditoria)

Nada em `gamecontrollerdb_linux.txt`/`.inc` é código executável, e nenhum header ou fonte SDL do
upstream é lido, compilado ou linkado em lugar nenhum deste repositório. O parser que interpreta
esse texto (`glintfx/src/gamepad_mapping.cpp`) é implementação clean-room própria do glintfx do
formato de linha bem documentado (ainda que informalmente especificado) do
`gamecontrollerdb.txt` (`guid,name,bN/aN/hH.M/...,platform:Linux,`) -- escrita a partir do
entendimento do formato, não lendo o parser do próprio SDL.

## Licença

**Licença zlib** (ver `LICENSE` neste diretório, baixado do mesmo commit pinado, byte-idêntico
a um fetch no mesmo dia do `LICENSE` do upstream na raiz do repositório). Permite uso,
modificação e redistribuição, inclusive como dado vendorizado e filtrado dentro de um projeto
licenciado diferente (MPL-2.0), com atribuição (este README + o `NOTICE` do repositório) e sem
apresentar o subconjunto filtrado como dado de mapeamento original do glintfx. Listado no
`NOTICE` do repositório.

## O que linka contra ele

Nada linka contra este diretório no sentido compilado -- ele alimenta o
`tools/gen_gamepad_db.py` (offline, em tempo de desenvolvimento, não em tempo de
build/configure) que emite `gamecontrollerdb_linux.inc`, incluído (`#include`) por
`glintfx/src/gamepad.cpp` (camada de device do Agente B) sob `#if GLINTFX_MODULE_GAMEPAD` e
gateado ainda em runtime por `GamepadsConfig::use_builtin_db`. `OFF` no módulo remove tanto o
array gerado quanto o símbolo dele do archive, a mesma disciplina de todo outro dado
vendorizado/gerado de outro átomo (compare `glintfx/third_party/khronos/gl.xml` ->
`tools/gen_glloader.py` -> `gl_loader.{h,c}`).
