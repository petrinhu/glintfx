# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Estado em 2026-07-01: o produto ativo deste repositório é o **glintfx** (lib C++ RmlUi+GL3), lançado e taggeado até **`v0.2.4`** (Codeberg + GitHub, CI verde, suíte de 18 testes). A Camada 0 (C+ASM puro, zero libc) descrita mais abaixo segue como decisões tomadas (ADR 0001–0005) mas é uma **trilha soberana dormente**: ainda não há implementação (W1+ do `TODO.md` pendente). Leia primeiro a seção "glintfx" abaixo.

## Duas camadas neste repo

- **Camada 1 -- glintfx (ATIVA, é o produto):** biblioteca C++ que une RmlUi (UI HTML/CSS) ao renderer de efeitos GL3. Pasta `glintfx/`. Ver seção dedicada abaixo.
- **Camada 0 -- `loucura_c_asm` (DORMENTE, trajetória de longo prazo):** o runtime freestanding C+ASM, zero libc, descrito no resto deste documento. Pastas `src/`, `include/` na raiz. É o **alvo de internalização** clean-room de longo prazo da Camada 1 (a "loucura" do nome do repo) -- não há pressa nem prazo.

Decisão de arquitetura: [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md) (as duas camadas não se linkam; a fronteira é o processo, não o linker).

## glintfx -- o produto ativo (Camada 1)

`glintfx` é uma **biblioteca C++ drop-in para Linux x86-64** (piso C++17, alvo C++23), licença MPL-2.0, que funde [RmlUi 6.3](https://github.com/mikke89/RmlUi) (UI HTML/CSS + layout) com um **renderer de efeitos GL3** (glow, degradê, backdrop-blur, drop-shadow, mask), tudo declarado em `.rcss` -- sem API imperativa de efeito.

- **Lançada:** v0.1.0 → **v0.2.4** (2026-07-01). Histórico completo em [`CHANGELOG.md`](CHANGELOG.md).
- **Dois modos de consumo:**
  - **`glintfx::App`** ([`glintfx/include/glintfx/app.hpp`](glintfx/include/glintfx/app.hpp)) -- standalone, dono da janela GLFW e do loop de frame (`poll → update → render → swap`).
  - **`glintfx::UiLayer`** ([`glintfx/include/glintfx/ui_layer.hpp`](glintfx/include/glintfx/ui_layer.hpp)) -- embed/guest mode: anexa ao contexto GL de um host (não cria janela), render **compose-only** (sem `glClear`, sem swap -- o host é dono dos dois), eventos injetados (`UiEvent` + `Key`), `GlStateGuard` salva e restaura o estado GL tocado. Ver [ADR-0008](docs/adr/0008-embed-guest-mode.md) (decisão) e [`docs/embed-integration.md`](docs/embed-integration.md) (contrato de integração, fonte de verdade para hosts).
- **Capacidades entregues até v0.2.4:**
  - **UA-stylesheet embutida** (v0.2.4): defaults de `display: block` para elementos estruturais (`div`, `p`, `h1..h6`, `ul`, `ol`, `li`, `section`, `article`, `header`, `footer`, `nav`, `main`), mescladas em todo documento via `StyleSheetContainer::CombineStyleSheetContainer()` no `Bootstrap::load()` (base de baixa especificidade -- regra do autor sobrepõe). Em `App` e `UiLayer`. Verificada por `ua_stylesheet_sanity`.
  - Efeitos GL3 data-driven via RCSS (glow/box-shadow, drop-shadow, degradê, backdrop-blur, blur, mask) -- sintaxe em [`docs/effects.md`](docs/effects.md).
  - **Data-model binding** (v0.2.3): `create_data_model(name)` → `bind_number/string/bool/list(key)` → `load()` → `set_number/string/bool/list(key, ...)`. Ordem obrigatória, enforçada pelo engine (`bind_*` após `load()` retorna `false`). Listas iteram no RML via `data-for`. Disponível em `App` e `UiLayer`. Ver `docs/embed-integration.md` seção 6.
  - **Texturas PNG/JPG/TGA** (v0.2.3): decode via stb_image (vendorizado), com **premultiply de alpha in-place** antes do upload (necessário pro blend `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` do renderer). TGA cai no loader upstream do RmlUi se o stb_image falhar. Ver `docs/embed-integration.md` seção 7.
  - **dp_ratio** (v0.2.2): `set_dp_ratio(float)` em `App`/`UiLayer` escala a unidade RCSS `dp` para pixels físicos (`px` não é afetado).
  - **`set_asset_base_url`** (v0.2.2): prefixa caminhos relativos de asset, útil quando o CWD do processo não é o diretório do documento.
  - Navegação por `Key` (gamepad/teclado) injetada via `process_event`; ainda **sem** `set_focus(id)` programático (roadmap -- INBOX `GAP-4` no `TODO.md`).
- **1º consumidor real:** **GusWorld** (jogo SDL3), embed mode no cockpit (ADR-010 no repo do GusWorld).
- **v2 (component library, Atomic Design):** PLANEJADA e PAUSADA -- spec aprovada em `docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`, branch `feat/v2-f2-components` (não iniciada). Ver `TODO.md` seção v2.

### Build e teste (glintfx)

```sh
# deps de sistema (Fedora)
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# configure + build (RmlUi 6.3 fetchado em configure; gl3w vendorizado offline)
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j

# suíte completa (17 testes, sob Xvfb / Mesa llvmpipe)
ctest --test-dir glintfx/build --output-on-failure
```

- **Option `GLINTFX_BACKEND_GLFW`** (CMake, default `ON`): `ON` compila o `glintfx::App` standalone + `window_glfw.cpp` + adapter GLFW do RmlUi, e linka `glfw`. `OFF` = build **embed-only**: só `Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock`, sem GLFW como dep transitiva da lib -- é o alvo de hosts SDL3/X11 como o GusWorld. Ver [ADR-0008](docs/adr/0008-embed-guest-mode.md).
- **Consumo:** `FetchContent`/`add_subdirectory` (recomendado, build do fonte) ou `find_package(glintfx)` (árvore instalada via `cmake --install`) → alvo único `glintfx::glintfx`.
- O card `mask` exige GPU real (Mesa/llvmpipe crasha); o `golden_test` pixel-exato é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`, só GPU real); o gate padrão de CI usa `render_sanity` (estatístico).

### Arquitetura

Fachada fina sobre módulos internos (M0 Bootstrap RmlUi, M1 Platform GLFW+FreeType, M2 Render GL3, M3 Facade `App`) mais `Engine`/`UiLayer`/`DataBinder` para o embed mode. Nenhum tipo de terceiro (GL/GLFW/RmlUi) cruza a fronteira pública (`glintfx/include/glintfx/`). Detalhe: [`README.md`](README.md#architecture), [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md) (as duas camadas), [ADR-0007](docs/adr/0007-license-mpl-2.0.md) (licença), [ADR-0008](docs/adr/0008-embed-guest-mode.md) (embed mode), spec de design em `docs/superpowers/specs/`.

### Gotchas críticos do renderer

Leia antes de mexer em `render_gl3.cpp`/`ui_layer.cpp`: **premultiplied alpha + composição sempre no FBO 0** (origem do viewport hardcoded em `(0,0)`); **MSAA desligado** (`RMLUI_NUM_MSAA_SAMPLES=0`, necessário sob Mesa/llvmpipe); **ordem fixa do data-model** (`create_data_model → bind_* → load() → set_*`); **gate de encapsulamento via grep include-based**, não nome cru de macro (`GLINTFX_BACKEND_GLFW` casa a substring "GLFW" mas não é leak de tipo de terceiro). Lista completa, com referências de linha: [`AGENTS.md`](AGENTS.md) seção "Gotchas críticos".

## Camada 0 -- núcleo soberano dormente (C + ASM puro)

`loucura_c_asm` é o nome do repositório inteiro e também o nome desta trilha **dormente**: um projeto **sério em C + Assembly puros**, com uma restrição central e inegociável:

- **ZERO bibliotecas. Sem libc, sem stdio, sem nada.** Nem `printf`, nem `malloc`, nem string ops prontas.
- O que faltar, **construímos do zero** (esta é a "loucura" do nome). A nossa própria substituta da libc vive no repo.
- Quando precisarmos de algo conhecido, **pesquisar na web sob demanda** libs públicas e papers → baixar → estudar → fazer RE (reverse engineering) → **reimplementar a nossa própria versão**. Não linkar a externa.

Consequência prática: a única ponte com o mundo é a **interface de syscalls do kernel Linux**. Toda I/O, alocação de memória e tempo passam por `syscall` direto. É o **alvo de internalização** de longo prazo da Camada 1 (glintfx): com o tempo, peças do glintfx (RmlUi, gl3w, FreeType, GLFW) podem ser reimplementadas clean-room sobre este núcleo, onde valer a pena.

- **Estudo/RE:** upstreams clonados em `examples/` (gitignored): `examples/RmlUi` (MIT), `examples/gl3w` (domínio público). Não copiar código -- reimplementar clean-room (base é MPL-2.0).

### Stack e alvo (decidido, não negociável sem o líder)

| Item | Escolha |
|---|---|
| Plataforma | Linux **x86-64**, ELF nativo. Roda sobre o kernel (não é bare-metal). |
| Entry point | `_start` próprio. **Não** há `crt0`/`main` da libc. `exit` é via syscall. |
| I/O / serviços | `syscall` direta (`write`=1, `read`=0, `exit`=60, `mmap`=9, ...). ABI System V AMD64. |
| Assembler | **NASM**, sintaxe **Intel**, arquivos `.asm` (`-f elf64`). |
| Compilador C | **clang**, `-std=c23 -ffreestanding -nostdlib`. |
| Linker | **ld** direto (GNU ld), `-nostdlib -static -no-pie`. |
| Arquitetura | só **x86-64** por ora. |

### Como buildar (sem libc)

Ainda não há `Makefile` -- quando o primeiro código surgir, consolidar estes comandos nele. Comandos canônicos:

```sh
# Compilar C freestanding (sem libc, sem PIC, sem stack protector)
clang -std=c23 -ffreestanding -nostdlib -fno-pic -fno-stack-protector \
      -fno-builtin -fno-asynchronous-unwind-tables -m64 -Wall -Wextra \
      -Iinclude -c src/arquivo.c -o build/arquivo.o

# Montar Assembly (NASM, Intel, ELF64)
nasm -f elf64 src/arquivo.asm -o build/arquivo.o

# Linkar sem libc, entry _start, binario estatico nao-PIE
ld -nostdlib -static -no-pie -e _start -o build/programa build/*.o

# Rodar e ver o codigo de saida (vem da syscall exit)
./build/programa; echo "exit=$?"
```

Por que essas flags importam (sem libc):
- `-nostdlib` / `ld -nostdlib`: não linka libc nem `crt*.o` de startup → por isso precisamos do nosso `_start`.
- `-ffreestanding`: diz ao clang que não há ambiente hospedado; ele não pode assumir que funções da libc existem.
- `-fno-builtin`: impede o clang de trocar, p.ex., um laço por uma chamada a `memcpy`/`memset` da libc (que não temos). Se quisermos `memcpy`, é a NOSSA.
- `-no-pie` / `-fno-pic`: binário em endereço fixo simplifica o início; PIE exigiria um loader dinâmico.

### Inspeção / debug (read-only, sempre úteis)

```sh
objdump -d -M intel build/programa     # desmontar (Intel, combina com NASM)
readelf -a build/programa              # cabecalhos ELF, secoes, simbolos
gdb build/programa                     # 'layout asm', 'starti', 'info registers'
strace ./build/programa                # ver as syscalls de verdade (auditar a nossa "libc")
```

### Estrutura de pastas

```
glintfx/  Camada 1 (ATIVA): a lib C++. include/glintfx/ (headers públicos), src/, demos/,
          tests/, third_party/ (gl3w, stb_image vendorizados), CMakeLists.txt.
src/      Camada 0 (dormente): Codigo-fonte (.c e .asm). Nossa runtime/substituta-de-libc.
include/  Camada 0 (dormente): Headers da API que criarmos (.h e .inc do NASM).
tests/    Camada 0 (dormente): Testes (harness ainda a definir -- provavelmente runner proprio).
docs/     Notas de RE, papers, decisoes (ADR), specs/plans, referencias de syscall/ABI -- das DUAS camadas.
tools/    Scripts auxiliares (build, dump, automacao de RE).
build/    Artefatos (.o, binarios) da Camada 0. Ignorado pelo git.
```

## Convenções

Regras de idioma (decididas pelo líder, valem para o repo inteiro):

- **Identificadores de código** (funções, variáveis, macros, labels NASM, structs): **apenas en-intl** (inglês internacional). Nada de pt-br em nome de símbolo.
- **Documentação** (arquivos em `docs/`, README, ADRs, e doc-comments / cabeçalhos de arquivo e de função): **bilíngue no MESMO arquivo -- en-intl primeiro, depois pt-br.**
- **CLAUDE.md e memórias** (instrução operacional para o agente/líder) seguem em **pt-br** -- não são "docs do produto".
- **Comunicação no chat** com o líder: **pt-br**.

Outras:

- Commits em **Conventional Commits**; mensagem em pt-br. Citar o ID do item do `TODO.md` (ex.: `L1-API`, `A1`) no corpo do commit ao fechar/avançar um item, e tocar o `Status` no mesmo commit.
- Assembly sempre em sintaxe **Intel** (consistência com NASM e com `objdump -M intel`).
- SPDX header em todo arquivo de código (`SPDX-License-Identifier: MPL-2.0`); não em `.md`.

## Regras de trabalho (deste projeto)

- **Camada 0: não introduzir nenhuma dependência externa nem chamada à libc.** Se a tentação aparecer, a resposta é implementar do zero. Em caso de dúvida real, perguntar ao líder.
- **Camada 1 (glintfx): pode linkar as libs listadas em `NOTICE`** (RmlUi, gl3w, FreeType, libGL, GLFW). Novas deps externas ainda exigem decisão do líder.
- Decisões de arquitetura/stack são do líder (petrus): apresentar 2–3 opções com prós/contras antes de decidir (ver memória do projeto).
- **Toda alteração de código/produto é feita por um agente especialista da constelação bigtech**, nunca inline pelo orquestrador (ver `AGENTS.md` seção "Governança").
- Calibre de comunicação ao explicar: C/ASM com mais contexto; arquitetura, hardware e lógica direto ao ponto (ver memória `feedback-nivel-comunicacao`).

## Licença

**MPL-2.0** (Mozilla Public License 2.0). Texto em `LICENSE`; atribuições das deps linkadas em `NOTICE`. © 2026 Petrus Silva Costa. SPDX: `MPL-2.0`. Copyleft **fraco por-arquivo** (modificações nos NOSSOS arquivos voltam abertas; apps de terceiros -- inclusive proprietários -- podem linkar a lib livremente) + grant de patente. Aplicada ao projeto inteiro; **relicenciada de AGPL-3.0 em 2026-06-28** (cliente da Camada 1 é adoção externa -- ver ADR-0007). Ao reimplementar/RE de libs externas, **não copiar código** -- reimplementar clean-room (a partir do entendimento).

## Pendências

A tabela de pendências e planejamento está em `TODO.md` na raiz (ordenada por execução; coluna Onda marca passos paralelizáveis), com seções separadas para Camada 0 (`A*`/`B*`/...), Camada 1/glintfx (`L1-*`, ondas `LW*`) e v2 (`L2-*`, ondas `V2W*`), mais a **INBOX** (descobertas não priorizadas). Decisões de arquitetura em `docs/adr/`. Manuais de teste e auditoria em `TESTES.md` e `AUDITORIAS.md`. Governança/porte em `.bigtech-porte`; a definição completa da constelação (agents, RACI, pipelines) está no plugin `bigtech_plugin` ([Codeberg](https://codeberg.org/petrinhu/bigtech_plugin) · [GitHub](https://github.com/petrinhu/bigtech_plugin)).

## Ambiente (Fedora 44)

Toolchain Camada 0 instalado e verificado: **nasm 3.01**, **clang 22**, GNU **ld/objdump/readelf 2.46**, **gdb 17**, **make 4.4**. `qemu` não está instalado e não é necessário (alvo é Linux nativo, não bare-metal).

Toolchain glintfx (Camada 1): **CMake >= 3.16**, clang, `glfw-devel`, `freetype-devel`, `mesa-libGL-devel` (Fedora). CI roda sob Xvfb/Mesa llvmpipe (GitHub Actions + Codeberg Forgejo Actions, runner `codeberg-medium` + container `catthehacker`); validar localmente com `forgejo-runner exec` antes de mexer nos workflows.
