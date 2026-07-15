# glintfx/vendor/core — vendored Layer 0 sovereign font engine "meat" (EN)

This directory is a **byte-identical vendored copy** of a small, self-contained slice of
`loucura_c_asm`'s Layer 0 (the freestanding, zero-libc C+ASM core of this monorepo): the
sovereign font-engine "meat" that glintfx's own font engine (`src/font_engine_own.{hpp,cpp}`,
L1.19-FONTENG / ADR-0009) links directly as `extern "C"` sources.

## Why this exists (L1.20-FONTFLIP Phase 1)

Before this vendoring, `GLINTFX_OWN_FONT_ENGINE=ON` read its C sources straight from
`../src/{sfnt,raster,hint}.c` and `../include/core/{sfnt,raster,hint}.h` — paths that only
resolve when glintfx is built **from inside the full `loucura_c_asm` monorepo** (siblings of
this `glintfx/` directory). A consumer that `FetchContent`s or copies only the `glintfx/`
subtree in isolation (the common case for an external drop-in consumer) would hit a hard
`FATAL_ERROR` at configure time.

`glintfx`'s own font engine is becoming the **default** (L1.20-FONTFLIP's "flip suave" —
FreeType stays linked and selectable, nothing is dropped). A default that only works inside
one specific monorepo checkout is not a real default for an external consumer. Vendoring this
minimal C source set makes the own-engine path self-contained: any copy of the `glintfx/`
subtree — monorepo or standalone — carries everything `GLINTFX_OWN_FONT_ENGINE=ON` needs.

## What is vendored (the minimal transitive closure)

Only 3 `.c` + 3 `.h` files — the exact set `src/font_engine_own.hpp` and its 3 `.c` files
`#include`, and nothing more (no `types.h`, no `alloc.h`/`mem.h`, no dynamic allocation — see
each file's own header comment for why: SOV-SFNT/SOV-RAST/SOV-HINT are deliberately
self-contained, freestanding-safe, allocation-free):

```
vendor/core/src/sfnt.c            <- ../../../src/sfnt.c    (SOV-SFNT, FT-F1)
vendor/core/src/raster.c          <- ../../../src/raster.c  (SOV-RAST, FT-F2)
vendor/core/src/hint.c            <- ../../../src/hint.c    (SOV-HINT, FT-F4)
vendor/core/include/core/sfnt.h   <- ../../../include/core/sfnt.h
vendor/core/include/core/raster.h <- ../../../include/core/raster.h
vendor/core/include/core/hint.h   <- ../../../include/core/hint.h
```

## Provenance and licensing

This is **not** third-party code. `sfnt.c`/`raster.c`/`hint.c` and their headers are Layer 0's
own clean-room implementation (MPL-2.0, same license as glintfx itself — see each file's
`SPDX-License-Identifier: MPL-2.0` header and their own file-header comments for the
clean-room rationale). Because this is our own code, no entry is added to `NOTICE` (that file
tracks third-party dependencies only).

## Re-syncing (keeping the vendored copy in sync with the canonical source)

The **canonical source of truth** is Layer 0: `loucura_c_asm/src/{sfnt,raster,hint}.c` and
`loucura_c_asm/include/core/{sfnt,raster,hint}.h`. This vendored copy must be a byte-identical
mirror. To re-sync after a Layer 0 change:

```sh
cp -p src/sfnt.c src/raster.c src/hint.c glintfx/vendor/core/src/
cp -p include/core/sfnt.h include/core/raster.h include/core/hint.h glintfx/vendor/core/include/core/
diff -q src/sfnt.c glintfx/vendor/core/src/sfnt.c   # ... and so on, confirm byte-identical
```

Do not hand-edit files under `vendor/core/` — patch Layer 0 (`../src`, `../include/core`) and
re-copy. `glintfx/CMakeLists.txt`'s `GLINTFX_OWN_FONT_ENGINE` block compiles exclusively from
`vendor/core/`; it never reads `../src` or `../include/core`.

---

# glintfx/vendor/core — cópia vendorizada da "carne" do motor de fonte soberano da Camada 0 (PT)

Este diretório é uma **cópia vendorizada byte-idêntica** de uma fatia pequena e
auto-contida da Camada 0 do `loucura_c_asm` (o núcleo C+ASM freestanding, zero-libc, deste
monorepo): a "carne" do motor de fonte soberano que o motor de fonte próprio do glintfx
(`src/font_engine_own.{hpp,cpp}`, L1.19-FONTENG / ADR-0009) linka diretamente como sources
`extern "C"`.

## Por que isto existe (L1.20-FONTFLIP Fase 1)

Antes desta vendorização, `GLINTFX_OWN_FONT_ENGINE=ON` lia seus fontes C direto de
`../src/{sfnt,raster,hint}.c` e `../include/core/{sfnt,raster,hint}.h` — paths que só
resolvem quando o glintfx é buildado **de dentro do monorepo `loucura_c_asm` completo**
(irmãos deste diretório `glintfx/`). Um consumidor que faz `FetchContent` ou copia só a
subárvore `glintfx/` isolada (o caso comum de um consumidor externo drop-in) bateria num
`FATAL_ERROR` duro em tempo de configure.

O motor de fonte próprio do glintfx está virando o **padrão** ("flip suave" do
L1.20-FONTFLIP — o FreeType continua linkado e selecionável, nada é descartado). Um padrão
que só funciona dentro de um checkout específico de monorepo não é um padrão de verdade para
um consumidor externo. Vendorizar este conjunto mínimo de fontes C torna o caminho do motor
próprio auto-contido: qualquer cópia da subárvore `glintfx/` — monorepo ou standalone — carrega
tudo que `GLINTFX_OWN_FONT_ENGINE=ON` precisa.

## O que é vendorizado (o fecho transitivo mínimo)

Só 3 arquivos `.c` + 3 `.h` — o conjunto exato que `src/font_engine_own.hpp` e seus 3 `.c`
`#include`am, nada mais (sem `types.h`, sem `alloc.h`/`mem.h`, sem alocação dinâmica — ver o
comentário de cabeçalho de cada arquivo pro motivo: SOV-SFNT/SOV-RAST/SOV-HINT são
deliberadamente auto-contidos, seguros em freestanding, sem alocação):

```
vendor/core/src/sfnt.c            <- ../../../src/sfnt.c    (SOV-SFNT, FT-F1)
vendor/core/src/raster.c          <- ../../../src/raster.c  (SOV-RAST, FT-F2)
vendor/core/src/hint.c            <- ../../../src/hint.c    (SOV-HINT, FT-F4)
vendor/core/include/core/sfnt.h   <- ../../../include/core/sfnt.h
vendor/core/include/core/raster.h <- ../../../include/core/raster.h
vendor/core/include/core/hint.h   <- ../../../include/core/hint.h
```

## Procedência e licenciamento

Isto **não** é código de terceiro. `sfnt.c`/`raster.c`/`hint.c` e seus headers são
implementação clean-room própria da Camada 0 (MPL-2.0, mesma licença do glintfx — ver o
cabeçalho `SPDX-License-Identifier: MPL-2.0` de cada arquivo e os próprios comentários de
cabeçalho-de-arquivo para a racional clean-room). Por ser código nosso, nenhuma entrada é
adicionada ao `NOTICE` (aquele arquivo rastreia só dependências de terceiro).

## Re-sincronização (mantendo a cópia vendorizada em dia com o fonte canônico)

A **fonte canônica de verdade** é a Camada 0: `loucura_c_asm/src/{sfnt,raster,hint}.c` e
`loucura_c_asm/include/core/{sfnt,raster,hint}.h`. Esta cópia vendorizada deve ser um espelho
byte-idêntico. Para re-sincronizar após uma mudança na Camada 0:

```sh
cp -p src/sfnt.c src/raster.c src/hint.c glintfx/vendor/core/src/
cp -p include/core/sfnt.h include/core/raster.h include/core/hint.h glintfx/vendor/core/include/core/
diff -q src/sfnt.c glintfx/vendor/core/src/sfnt.c   # ... e assim por diante, confirmar byte-idêntico
```

Não edite os arquivos em `vendor/core/` à mão — corrija a Camada 0 (`../src`,
`../include/core`) e recopie. O bloco `GLINTFX_OWN_FONT_ENGINE` do `glintfx/CMakeLists.txt`
compila exclusivamente a partir de `vendor/core/`; ele nunca lê `../src` nem
`../include/core`.
