# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Estado em 2026-07-10: o produto ativo deste repositório é o **glintfx** (lib C++ RmlUi+GL3), lançado e taggeado até **`v0.9.1`** (Codeberg + GitHub, CI dual verde + nightly sanitizer, suíte de 39 testes GLFW=ON / 22 embed; a `v0.9.1` é patch -- fix de UB de teardown do RmlUi (consumer-driven), motor de fonte próprio com kerning opt-in (`GLINTFX_OWN_FONT_ENGINE=OFF` por padrão), CI job de gate + mitigação `SYS_futex`; a `v0.9.0` trouxe DOM read/write por id + read-back do data-model (`L1.16-DOMRW`: `set_text`/`add_class`/`remove_class`/`set_property` + `get_number/string/bool`, `set_text` escapa RML anti-injeção) + 5 callbacks de evento observável (`L1.15-FORMEV`: `set_change/submit/focus/blur/hover_callback` em `App`/`UiLayer`) + cobertura de código self-hosted no CI (`L1.18-COVCI`: llvm-cov, badge, gate de piso 75%, sem uploader de terceiro) -- minor/drop-in; a `v0.8.0` trouxe loader GL 3.3 core-profile próprio **clean-room** (`L1.14-GLLOADER`, gerado do registro público Khronos `gl.xml`, elimina o `gl3w` vendorizado -- 1º marco da trilha de internalização) + `set_focus(id)`/`clear_focus()` programáticos em `App`/`UiLayer` (`L1.17-FOCUS`); o CI builda com clang). A Camada 0 (C+ASM puro, zero libc) tem a **implementação COMPLETA**: bootstrap I/O (`_start`, wrappers de syscall, `exit`/`write`/`read`) → harness próprio de teste (`C1`, habilita TDD) → libc-núcleo (memória, string, conversão int↔string) → mini-printf → alocador bump via `mmap`, tudo zero libc e sob TDD + review adversarial (itens em `🔍`, aguardando as ondas `TST-*`/`F1`/`AUD-*`/`REL-TAG` `core-v0.1.0` pro `✅`). A meta plena de internalização clean-room (RmlUi/FreeType/GLFW) segue distante, sem prazo. Leia a seção "glintfx" (produto ativo) e depois a "Camada 0" abaixo.

## Duas camadas neste repo

- **Camada 1 -- glintfx (ATIVA, é o produto):** biblioteca C++ que une RmlUi (UI HTML/CSS) ao renderer de efeitos GL3. Pasta `glintfx/`. Ver seção dedicada abaixo.
- **Camada 0 -- `loucura_c_asm` (implementação COMPLETA, aguardando auditoria; trajetória de longo prazo):** o runtime freestanding C+ASM, zero libc, descrito no resto deste documento. Pastas `src/`, `include/` na raiz. É o **alvo de internalização** clean-room de longo prazo da Camada 1 (a "loucura" do nome do repo) -- não há pressa nem prazo.

Decisão de arquitetura: [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md) (as duas camadas; a fronteira era o processo) — **sucedida em parte pelo [ADR-0009](docs/adr/0009-internalization-boundary.md)** (2026-07-09, decisão do líder): o endgame de internalização adota **link estático incremental** (`libcore.a`/prefixo `glx_`, padrão "casca-hosted + carne-C via `extern C`"), estreitando pontualmente a cláusula "não se linkam" — a Camada 0 segue freestanding na própria compilação; só o `.a` cruza.

## glintfx -- o produto ativo (Camada 1)

`glintfx` é uma **biblioteca C++ drop-in para Linux x86-64** (piso C++17, alvo C++23), licença MPL-2.0, que funde [RmlUi 6.3](https://github.com/mikke89/RmlUi) (UI HTML/CSS + layout) com um **renderer de efeitos GL3** (glow, degradê, backdrop-blur, drop-shadow, mask), tudo declarado em `.rcss` -- sem API imperativa de efeito.

- **Lançada:** v0.1.0 → **v0.9.1** (2026-07-10). Histórico completo em [`CHANGELOG.md`](CHANGELOG.md).
- **Dois modos de consumo:**
  - **`glintfx::App`** ([`glintfx/include/glintfx/app.hpp`](glintfx/include/glintfx/app.hpp)) -- standalone, dono da janela GLFW e do loop de frame (`poll → update → render → swap`).
  - **`glintfx::UiLayer`** ([`glintfx/include/glintfx/ui_layer.hpp`](glintfx/include/glintfx/ui_layer.hpp)) -- embed/guest mode: anexa ao contexto GL de um host (não cria janela), render **compose-only** (sem `glClear`, sem swap -- o host é dono dos dois), eventos injetados (`UiEvent` + `Key`), `GlStateGuard` salva e restaura o estado GL tocado. Ver [ADR-0008](docs/adr/0008-embed-guest-mode.md) (decisão) e [`docs/embed-integration.md`](docs/embed-integration.md) (contrato de integração, fonte de verdade para hosts).
- **Capacidades entregues até v0.4.0:**
  - **Rolagem em embed mode** (v0.4.0, `GLINTFX-SCROLL-1`): `UiEvent::Type::MouseWheel` (rola o ancestral rolável mais próximo do elemento em HOVER, não foco -- confirmado comportalmente por review adversarial) + 5 métodos programáticos em `UiLayer`/`App` (`scroll_element_into_view`, trio de métricas `get/set_element_scroll_top`+`get_element_scroll_height`+`get_element_client_height`). `App` não encaminha wheel (não tem `process_event` nenhum, lacuna pré-existente) -- só `UiLayer`. Ver `docs/embed-integration.md`.
  - **Preenchimento em gradiente no `polygon()`** (v0.3.1): o preenchimento do `polygon()` aceita `radial-gradient(...)`/`linear-gradient(...)` além de cor sólida -- rampa de cor real por-pixel via o próprio shader de gradiente do RmlUi (não facetada), consumer-driven pelo nó "cabeça de parafuso" do GusWorld. Ver `docs/effects.md`.
  - **Dois fixes de segurança de memória** (v0.3.1, achados via auditoria): `load()` não vaza mais o documento anterior (fechava sem `Close()`, causando leak + "fantasma" renderizando por baixo a cada reload); `bind_number/string/bool/list` não corrompe mais memória quando chamado 2x com a mesma chave antes do `load()` (era um heap-use-after-free real, capturado ao vivo pelo ASan). Ver `CHANGELOG.md`.
  - **Decorator `polygon()` (shape primitive)** (v0.3.0): `decorator: polygon(<lados>, <cor>[, <rotação>])` preenche a caixa do elemento com um N-ágono regular (triangle-fan), `lados ∈ [3, 1024]` com fail-high (ignora e loga) em input inválido/hostil. Glow via `filter: drop-shadow` e clip via `mask-image` funcionam de graça (qualquer decorator serve de alpha-shape/máscara). Ver `docs/effects.md`.
  - **Hardening de superfície de entrada da API** (v0.3.0): `load(nullptr)` retorna `false` em vez de abortar o host (`std::terminate()`); `set_dp_ratio` rejeita `!isfinite`/`≤0` mantendo o valor anterior; `set_viewport` rejeita `w/h≤0` como no-op. `version()` corrigido -- estava travado em `"0.2.4"` desde a v0.2.5 por bump esquecido no CMake, agora reflete a tag corretamente.
  - **Hit-test / geometria para o host** (v0.2.5): `set_click_callback(fn(const char* id))` reporta o id do elemento clicado (hit-test interno do RmlUi devolvido ao host; sobe ancestrais até achar id, `""` se nenhum); `get_element_box(id) → ElementBox{found,x,y,w,h}` dá a geometria border-box em px físicos do viewport. Ambos em `App` e `UiLayer`. Mata o padrão de espelhar geometria de RCSS à mão no host. Ver `docs/embed-integration.md` seção 10.
  - **Viewport com origem / letterbox** (v0.2.5): `UiLayer::set_viewport(x,y,w,h,target_h)` compõe a UI numa sub-região do FBO 0 (só `UiLayer`; `App` é dono da janela). **Contrato de coordenadas:** mouse (`UiEvent`) e `get_element_box` sempre em espaço-janela (top-left, y-down); o `UiLayer` traduz pro offset GL bottom-up via `target_h` (altura total da janela), `gl_offset_y = target_h - y - h`. Composição segue no FBO 0; só a origem muda. `set_viewport(w,h)` legado = `(0,0,w,h)`. ADR-0008 (adendo F3), `docs/embed-integration.md` seção 10.
  - **UA-stylesheet embutida** (v0.2.4): defaults de `display: block` para elementos estruturais (`div`, `p`, `h1..h6`, `ul`, `ol`, `li`, `section`, `article`, `header`, `footer`, `nav`, `main`), mescladas em todo documento via `StyleSheetContainer::CombineStyleSheetContainer()` no `Bootstrap::load()` (base de baixa especificidade -- regra do autor sobrepõe). Em `App` e `UiLayer`. Verificada por `ua_stylesheet_sanity`.
  - Efeitos GL3 data-driven via RCSS (glow/box-shadow, drop-shadow, degradê, backdrop-blur, blur, mask) -- sintaxe em [`docs/effects.md`](docs/effects.md).
  - **Data-model binding** (v0.2.3): `create_data_model(name)` → `bind_number/string/bool/list(key)` → `load()` → `set_number/string/bool/list(key, ...)`. Ordem obrigatória, enforçada pelo engine (`bind_*` após `load()` retorna `false`). Listas iteram no RML via `data-for`. Disponível em `App` e `UiLayer`. Ver `docs/embed-integration.md` seção 6.
  - **Texturas PNG/JPG/TGA** (v0.2.3): decode via stb_image (vendorizado), com **premultiply de alpha in-place** antes do upload (necessário pro blend `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` do renderer). TGA cai no loader upstream do RmlUi se o stb_image falhar. Ver `docs/embed-integration.md` seção 7.
  - **dp_ratio** (v0.2.2): `set_dp_ratio(float)` em `App`/`UiLayer` escala a unidade RCSS `dp` para pixels físicos (`px` não é afetado).
  - **`set_asset_base_url`** (v0.2.2): prefixa caminhos relativos de asset, útil quando o CWD do processo não é o diretório do documento.
  - Navegação por `Key` (gamepad/teclado) injetada via `process_event`; ainda **sem** `set_focus(id)` programático (roadmap -- INBOX `GAP-4` no `TODO.md`).
- **1º consumidor real:** **GusWorld** (jogo SDL3), embed mode no cockpit (ADR-010 no repo do GusWorld).
- **v2 (component library, Atomic Design):** modo **PULL INCREMENTAL** (decisão do líder 2026-07-01) -- não agendada como projeto; backlog puxado por demanda real. Spec aprovada em `docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`, branch `feat/v2-f2-components` (parada, não abandonada). Nenhuma das releases pós-v1 precisou de componentes. Ver `TODO.md` seção v2.

### Build e teste (glintfx)

```sh
# deps de sistema (Fedora)
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# configure + build (RmlUi 6.3 fetchado em configure; gl3w vendorizado offline)
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j

# suíte completa (31 testes GLFW=ON / 16 embed GLFW=OFF, sob Xvfb / Mesa llvmpipe)
ctest --test-dir glintfx/build --output-on-failure
```

- **Option `GLINTFX_BACKEND_GLFW`** (CMake, default `ON`): `ON` compila o `glintfx::App` standalone + `window_glfw.cpp` + adapter GLFW do RmlUi, e linka `glfw`. `OFF` = build **embed-only**: só `Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock`, sem GLFW como dep transitiva da lib -- é o alvo de hosts SDL3/X11 como o GusWorld. Ver [ADR-0008](docs/adr/0008-embed-guest-mode.md).
- **Consumo:** `FetchContent`/`add_subdirectory` (recomendado, build do fonte) ou `find_package(glintfx)` (árvore instalada via `cmake --install`) → alvo único `glintfx::glintfx`.
- O card `mask` exige GPU real (Mesa/llvmpipe crasha); o `golden_test` pixel-exato é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`, só GPU real); o gate padrão de CI usa `render_sanity` (estatístico).

### Arquitetura

Fachada fina sobre módulos internos (M0 Bootstrap RmlUi, M1 Platform GLFW+FreeType, M2 Render GL3, M3 Facade `App`) mais `Engine`/`UiLayer`/`DataBinder` para o embed mode. Nenhum tipo de terceiro (GL/GLFW/RmlUi) cruza a fronteira pública (`glintfx/include/glintfx/`). Detalhe: [`README.md`](README.md#architecture), [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md) (as duas camadas), [ADR-0007](docs/adr/0007-license-mpl-2.0.md) (licença), [ADR-0008](docs/adr/0008-embed-guest-mode.md) (embed mode), spec de design em `docs/superpowers/specs/`.

### Gotchas críticos do renderer

Leia antes de mexer em `render_gl3.cpp`/`ui_layer.cpp`: **premultiplied alpha + composição sempre no FBO 0** (origem do viewport hardcoded em `(0,0)`); **MSAA desligado** (`RMLUI_NUM_MSAA_SAMPLES=0`, necessário sob Mesa/llvmpipe); **ordem fixa do data-model** (`create_data_model → bind_* → load() → set_*`); **gate de encapsulamento via grep include-based**, não nome cru de macro (`GLINTFX_BACKEND_GLFW` casa a substring "GLFW" mas não é leak de tipo de terceiro). Lista completa, com referências de linha: [`AGENTS.md`](AGENTS.md) seção "Gotchas críticos".

## Camada 0 -- núcleo soberano (C + ASM puro)

> **Estado (2026-07-04):** IMPLEMENTAÇÃO COMPLETA (aguardando auditoria). Pipeline entregue ponta-a-ponta, tudo zero libc e sob TDD (red/green/refactor) + review adversarial: bootstrap I/O (`_start`, wrappers `syscall0..6`, helpers `exit`/`write`/`read`) → harness de teste próprio (`C1`, habilita TDD) → libc-núcleo (`D1` memória: `memcpy`/`memset`/`memmove`/`memcmp`; `D2` string: `strlen`/`strcmp`/`strncmp`/`strcpy`/`strncpy`/`strcat`/`strchr`; `D3` conversão int↔string) → `E2` mini-printf → `E1` alocador bump via `mmap`. Itens em `🔍 Pendente verificação`, aguardando as ondas `TST-*`/`F1`/`AUD-*`/`REL-TAG` (`core-v0.1.0`) pro `✅`. A meta plena (internalizar RmlUi/gl3w/FreeType/GLFW clean-room) segue a anos — sem pressa nem prazo.

`loucura_c_asm` é o nome do repositório inteiro e também o nome desta trilha: um projeto **sério em C + Assembly puros**, com uma restrição central e inegociável:

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

O `Makefile` na raiz (entregue em `A6`) já consolida estes comandos — use `make build` / `make test` / `make run` / `make clean` no dia a dia. Os comandos canônicos abaixo são a referência do que o Makefile faz (e úteis pra invocação manual):

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
src/      Camada 0 (ATIVA): Codigo-fonte (.c e .asm). Runtime/substituta-de-libc: syscall.asm, start.asm, sys_{exit,write,read}.c.
include/  Camada 0 (ATIVA): Headers (.h e .inc do NASM): types.h, syscall_nums.h/.inc, syscall.h, sys_{exit,write,read}.h.
tests/    Camada 0 (ATIVA): exit42/hello/echo_stdin + expected_exit.txt (harness do Makefile; runner proprio C1 e futuro).
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

**Política de CI (canônica desde 2026-07-10/11):** gate de ordem `github` > `claudio` (self-hosted local, `.forgejo/workflows/heavy.yml`, container `fedora:42`, roda ASan+fonteng) > `codeberg` (só paridade mínima, não bloqueia) -- basta UM verde; um job `waiting` no Codeberg nunca segura release. Reproduzir falha no container `fedora:42` local (`tools/ci/Containerfile.f42`) antes de iterar por push. Detalhe operacional completo: [`AGENTS.md`](AGENTS.md#política-de-ci-ordem-de-gate-e-o-que-roda-onde); histórico da decisão: memória `feedback_ci_policy_canonica`.
