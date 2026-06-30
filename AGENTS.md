# AGENTS.md

> **EN:** Instructions for AI agents working in this repository. Read this before touching code.
> **PT:** Instruções para agentes de IA que trabalham neste repositório. Leia antes de tocar no código.

This repository hosts two independent tracks. Know which one you are in before editing.

| Layer / Camada | Path | Status | Build |
| :--- | :--- | :--- | :--- |
| **Layer 1: glintfx** (the released library) | `glintfx/`, `consumer-example/` | v0.1.0, ready | CMake |
| **Layer 0: `loucura_c_asm`** (sovereign runtime) | `src/`, `include/` | not implemented (ADRs only) | Makefile (future) |

Layer 1 (glintfx) is C++ linking real libraries. Layer 0 is pure C + Assembly with **zero libc**, talking to the kernel only via syscalls. They do **not** link to each other; the boundary is the process, not the linker. See [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md).

---

## English

### Build and test (glintfx, Layer 1)

System dependencies (Fedora): `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`.

```sh
# configure + build (RmlUi 6.3 fetched automatically; gl3w is vendored)
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build

# run the showcase demo (all five effects; needs a real GPU for mask)
./glintfx/build/demos/showcase/glintfx_showcase

# run the test suite (headless, under Xvfb)
ctest --test-dir glintfx/build --output-on-failure
```

The suite has 5 tests, all wrapped to run under Xvfb: `window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke`, and `render_sanity` (structural pixel-statistics, tolerant of llvmpipe non-determinism). The pixel-exact `golden_test` is **opt-in** and flaky under software GL:

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_GOLDEN_TEST=ON   # real GPU only
```

To verify drop-in consumption end to end, build `consumer-example/`, which pulls glintfx via `FetchContent` (`SOURCE_DIR`) and links `glintfx::glintfx` with no GL/GLFW/RmlUi references.

### Conventions

- **SPDX header in every code file.** First line: `SPDX-License-Identifier: MPL-2.0` (comment style per language: `//` for C/C++, `;` for NASM, `#` for CMake/Makefile/shell). Do **not** put SPDX in `.md` docs.
- **Identifiers in English only** (functions, variables, macros, NASM labels, structs). No pt-br in symbol names.
- **Docs are bilingual, en first then pt**, in the **same file** (this applies to `docs/`, README, ADRs, and doc-comments / file and function headers). CLAUDE.md, AGENTS.md operational notes, and chat stay in pt-br.
- **Public headers expose no third-party types.** Nothing from GL, GLFW, or RmlUi may appear in `glintfx/include/glintfx/`. The facade uses pImpl (`struct Impl;` + `std::unique_ptr`). This is the "golden boundary" that keeps future internalization possible.
- **Conventional Commits**, message in pt-br. Cite the `TODO.md` item ID (e.g. `L1-API`) in the commit body when closing or advancing an item, and touch its `Status` in the same commit (delivered work goes to `🔍 Pendente verificação`, never straight to `✅`).
- **Assembly is always Intel syntax** (Layer 0), consistent with NASM and `objdump -M intel`.
- **No new external dependency in Layer 0.** If something is missing there, it is implemented from scratch (the "loucura"). Layer 1 may link the libraries listed in `NOTICE`.

### Repository structure

```
glintfx/                 Layer 1: the C++ library
  include/glintfx/       public headers (app.hpp, glintfx.hpp), no third-party types
  src/                   M0-M3: bootstrap.cpp, window_glfw.cpp, render_gl3.cpp, app.cpp
  demos/showcase/        showcase.rml/.rcss + glintfx_showcase + glintfx_capture
  tests/                 ctest smokes + render_sanity + opt-in golden_test (Xvfb)
  third_party/gl3w/      vendored gl3w loader (public domain) + Khronos headers (MIT)
  CMakeLists.txt
consumer-example/        drop-in proof: consumes glintfx via FetchContent
src/, include/           Layer 0: C/ASM runtime (scaffold only, not implemented)
docs/adr/                ADRs 0001-0005 (Layer 0) + 0006-0007 (layers, license)
docs/superpowers/        design spec + plan for glintfx v1
TODO.md                  pendings + planning for both layers (+ INBOX)
NOTICE, LICENSE          MPL-2.0 + third-party attributions
```

### The public API surface (do not invent)

The entire public API is in [`glintfx/include/glintfx/app.hpp`](glintfx/include/glintfx/app.hpp). It is the facade `glintfx::App` (RAII, move-only) plus `AppConfig` and `version()`. There is **no imperative effect API**: effects are declared in `.rcss`. Do not document or call methods that are not in that header.

---

## Para o Claude Code (português)

> Esta seção é a governança operacional deste repo. As regras acima (build, convenções) valem; abaixo está **como** o trabalho deve ser conduzido.

### Governança: constelação bigtech (execução por agente especialista)

Todo **código, produto, review e planejamento** deste repositório é executado por um **agente especialista** da constelação bigtech, nunca inline pelo orquestrador. O orquestrador apenas **coordena, faz git e pergunta ao líder**; ele não escreve produto.

- Porte/backend C++ ou C/ASM: `backend-engineer`.
- Arquitetura e ADRs: `software-architect`.
- Teste/QA: `qa-engineer`.
- Documentação: `technical-writer` / `ux-writer`.
- Produto/escopo (ex.: a v2 component library): `Capitolino/CPO` + `ux-ui-designer`.
- Auditoria: `internal-auditor`.

Quando vários agents são necessários, um **C-level** orquestra. Decisões de arquitetura, stack, escopo, licença e qualquer porta de mão única (one-way door) são **sempre do líder (petrus)**: apresentar 2-3 opções com prós/contras via `AskUserQuestion`, não decidir sozinho. Detalhe em [`CLAUDE.md`](CLAUDE.md) (seção de governança e autoridade suprema).

### Ponteiros essenciais

- [`CLAUDE.md`](CLAUDE.md): convenções do projeto, idioma, autoridade do líder, ambiente (toolchain Fedora 44).
- [`TODO.md`](TODO.md): tabela de pendências (ondas, IDs, pré-requisitos) das duas camadas, a **INBOX** (descobertas não priorizadas) e o escopo planejado da **v2** (component library / Atomic Design).
- **Sistema de memória:** memórias tipadas em `~/.claude/projects/<slug>/memory/` (índice em `MEMORY.md`), autocarregadas por sessão. Não duplicar o que o repo já registra; registrar só o não óbvio.
- [`docs/adr/`](docs/adr/README.md): decisões de arquitetura (imutáveis quando `Accepted`; para mudar, escrever novo ADR que substitua).

### Gotchas críticos (RmlUi / GL3): leia antes de mexer no renderer

- **Premultiplied alpha e backbuffer opaco.** O `RenderInterface_GL3` trabalha com alpha premultiplicado. Garanta o backbuffer **opaco** (alpha=1 no compositor) ou o compositor (Wayland/X) deixa a janela translúcida. Esse fix já está aplicado em `window_glfw.cpp`/`render_gl3.cpp`.
- **MSAA desligado no render layer.** `RMLUI_NUM_MSAA_SAMPLES=0` é necessário sob Mesa/llvmpipe (Xvfb): o resolve `glBlitFramebuffer` MSAA para não-MSAA produz silenciosamente textura de pós-processo preta. Ver comentário no `glintfx/CMakeLists.txt`.
- **Card `mask` crasha no software GL.** Sob Mesa/llvmpipe o shader BlendMask dual-sampler provoca corrupção de heap (`free(): invalid next size`), bug do Mesa SW renderer e **não** do glintfx. O CI headless usa `showcase_test.rml` (sem o card mask). O card mask só roda em **GPU real**.
- **gl3w é vendorizado e offline.** Os arquivos pré-gerados vivem em `glintfx/third_party/gl3w/` (domínio público) com headers Khronos (MIT). Não dependa de rede em tempo de configure para o gl3w; só o RmlUi é fetchado.
- **`golden_test` é flaky no llvmpipe.** O render de tiles em threads + não-associatividade de ponto flutuante geram MSE alto entre execuções idênticas. Por isso é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) e o gate padrão usa `render_sanity` (estatístico). Gere a referência em GPU real com `glintfx_capture`.
- **`snapshot()` sai com flip vertical** (origem do `glReadPixels`). Item aberto na INBOX do `TODO.md`.
- **Sintaxe RCSS do RmlUi 6.3 difere do CSS.** Cor primeiro em `box-shadow`/`drop-shadow`; gradiente via `decorator:`; hex `#rrggbbaa`. Ver `glintfx/demos/showcase/showcase.rcss` (fonte de verdade) e [`docs/effects.md`](docs/effects.md).
