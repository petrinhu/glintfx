# AGENTS.md

> **EN:** Instructions for AI agents working in this repository. Read this before touching code.
> **PT:** Instruções para agentes de IA que trabalham neste repositório. Leia antes de tocar no código.

This repository hosts two independent tracks. Know which one you are in before editing.

| Layer / Camada | Path | Status | Build |
| :--- | :--- | :--- | :--- |
| **Layer 1: glintfx** (the released library, **active product**) | `glintfx/`, `consumer-example/` | **v0.2.3**, released, two consumption modes (`App` + `UiLayer`) | CMake |
| **Layer 0: `loucura_c_asm`** (sovereign runtime, **dormant track**) | `src/`, `include/` | not implemented (ADRs only) | Makefile (future) |

Layer 1 (glintfx) is C++ linking real libraries; it is the repository's **active product**. Layer 0 is pure C + Assembly with **zero libc**, talking to the kernel only via syscalls; it is a **dormant, long-term internalization target** (decisions only, no implementation yet). They do **not** link to each other; the boundary is the process, not the linker. See [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md).

---

## English

### Build and test (glintfx, Layer 1)

System dependencies (Fedora): `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`.

```sh
# configure + build (RmlUi 6.3 fetched automatically; gl3w is vendored)
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j

# run the showcase demo (all five effects; needs a real GPU for mask)
./glintfx/build/demos/showcase/glintfx_showcase

# run the test suite (headless, under Xvfb)
ctest --test-dir glintfx/build --output-on-failure
```

**`GLINTFX_BACKEND_GLFW`** (CMake option, default `ON`) controls how much of the library is compiled:

- `ON` (default): compiles the standalone `glintfx::App` facade, `window_glfw.cpp`, and the RmlUi GLFW platform adapter; links `glfw`. The suite has **17 tests** under Xvfb: `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, `app_dp_ratio_smoke`, `render_sanity` (structural pixel-statistics, tolerant of llvmpipe non-determinism), `data_model_smoke`, `data_model_scalar`, `data_model_list`, `texture_png_alpha`, plus the 7 embed tests below.
- `OFF` (embed-only): only `Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock` are compiled, `glfw` is **not** linked into the library (test fixtures still use GLFW via a test-only helper target). Runs the 7 embed tests: `ui_layer_attach`, `ui_layer_compose`, `gl_state_guard`, `ui_layer_events`, `ui_layer_sanity`, `dp_ratio_sanity`, `base_url_sanity`. This is the build mode SDL3/X11 hosts (e.g. GusWorld) consume -- see [ADR-0008](docs/adr/0008-embed-guest-mode.md).

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BACKEND_GLFW=OFF -DGLINTFX_BUILD_TESTS=ON   # embed-only
```

The pixel-exact `golden_test` is **opt-in** and flaky under software GL (requires `GLINTFX_BACKEND_GLFW=ON`):

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
glintfx/                 Layer 1: the C++ library (active product)
  include/glintfx/       public headers (app.hpp, ui_layer.hpp, ui_event.hpp, glintfx.hpp,
                          config.hpp.in), no third-party types
  src/                   bootstrap.cpp, window_glfw.cpp, render_gl3.cpp, app.cpp, engine.cpp,
                          ui_layer.cpp, data_binder.cpp, base_url_file_interface.hpp,
                          stb_image_impl.cpp, gl_state.hpp, system_clock.cpp
  demos/showcase/        showcase.rml/.rcss + glintfx_showcase + glintfx_capture
  tests/                 ctest smokes + sanity tests (17 with GLFW=ON, 7 embed-only) + opt-in
                          golden_test (Xvfb)
  third_party/gl3w/      vendored gl3w loader (public domain) + Khronos headers (MIT)
  third_party/stb/       vendored stb_image.h (public domain / MIT) for PNG/JPG decode
  CMakeLists.txt
consumer-example/        drop-in proof: consumes glintfx via FetchContent
src/, include/           Layer 0: C/ASM runtime (scaffold only, not implemented, dormant)
docs/adr/                ADRs 0001-0005 (Layer 0) + 0006-0008 (layers, license, embed mode)
docs/embed-integration.md  host integration contract for UiLayer (frame lifecycle, GL state,
                          dp_ratio, base URL, data model, textures) -- authoritative reference
docs/superpowers/        design specs + plans for glintfx v1 and v2
TODO.md                  pendings + planning for both layers and v2 (+ INBOX)
NOTICE, LICENSE          MPL-2.0 + third-party attributions
```

### The public API surface (do not invent)

The public API lives in two headers under `glintfx/include/glintfx/`: [`app.hpp`](glintfx/include/glintfx/app.hpp) (the standalone facade `glintfx::App`, RAII move-only, plus `AppConfig` and `version()`) and [`ui_layer.hpp`](glintfx/include/glintfx/ui_layer.hpp) (the embed/guest facade `glintfx::UiLayer`, plus `UiLayerConfig`; events via [`ui_event.hpp`](glintfx/include/glintfx/ui_event.hpp)). Both expose the same data-model API (`create_data_model`/`bind_*`/`set_*`) and the same `set_dp_ratio`/`set_asset_base_url` methods -- see `docs/embed-integration.md` for the full contract. There is **no imperative effect API**: effects are declared in `.rcss`. Do not document or call methods that are not in those two headers.

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

A constelação bigtech (definição dos agents, RACI, pipelines de release) é mantida no plugin `bigtech_plugin` ([Codeberg](https://codeberg.org/petrinhu/bigtech_plugin) · [GitHub](https://github.com/petrinhu/bigtech_plugin)).

### Ponteiros essenciais

- [`CLAUDE.md`](CLAUDE.md): convenções do projeto, idioma, autoridade do líder, ambiente (toolchain Fedora 44), glintfx como produto ativo.
- [`docs/embed-integration.md`](docs/embed-integration.md): contrato de integração do `UiLayer` para hosts (fonte de verdade para frame lifecycle, GL state, dp_ratio, base URL, data model, texturas) -- leia antes de mexer no caminho embed.
- [`TODO.md`](TODO.md): tabela de pendências (ondas, IDs, pré-requisitos) das duas camadas, a **INBOX** (descobertas não priorizadas) e o escopo planejado da **v2** (component library / Atomic Design, spec em `docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`, branch `feat/v2-f2-components`, pausada).
- **Sistema de memória:** memórias tipadas em `~/.claude/projects/<slug>/memory/` (índice em `MEMORY.md`), autocarregadas por sessão. Não duplicar o que o repo já registra; registrar só o não óbvio.
- [`docs/adr/`](docs/adr/README.md): decisões de arquitetura (imutáveis quando `Accepted`; para mudar, escrever novo ADR que substitua). ADR-0006 (camadas), ADR-0007 (licença), ADR-0008 (embed/guest mode).

### Gotchas críticos (RmlUi / GL3 / embed mode): leia antes de mexer no renderer

- **Premultiplied alpha, backbuffer opaco e composição sempre no FBO 0.** O `RenderInterface_GL3` trabalha com alpha premultiplicado (`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`). No modo standalone, garanta o backbuffer **opaco** (alpha=1 no compositor) ou o compositor (Wayland/X) deixa a janela translúcida -- fix já aplicado em `window_glfw.cpp`/`render_gl3.cpp`. No embed mode, `UiLayer::render()` é **compose-only** (sem `glClear`, sem swap) e compõe **incondicionalmente sobre o FBO 0** com origem de viewport hardcoded em `(0,0)` -- um FBO custom do host ligado antes de `render()` **não** é o alvo (limitação F1 documentada no [ADR-0008](docs/adr/0008-embed-guest-mode.md) e em `docs/embed-integration.md` seções 0 e 2). Texturas decodificadas por `LoadTexture` (PNG/JPG via stb_image, v0.2.3) também passam por premultiply in-place antes do upload, pelo mesmo motivo.
- **MSAA desligado no render layer.** `RMLUI_NUM_MSAA_SAMPLES=0` é necessário sob Mesa/llvmpipe (Xvfb): o resolve `glBlitFramebuffer` MSAA para não-MSAA produz silenciosamente textura de pós-processo preta. Ver comentário no `glintfx/CMakeLists.txt`.
- **Card `mask` crasha no software GL.** Sob Mesa/llvmpipe o shader BlendMask dual-sampler provoca corrupção de heap (`free(): invalid next size`), bug do Mesa SW renderer e **não** do glintfx. O CI headless usa `showcase_test.rml` (sem o card mask). O card mask só roda em **GPU real**.
- **gl3w é vendorizado e offline.** Os arquivos pré-gerados vivem em `glintfx/third_party/gl3w/` (domínio público) com headers Khronos (MIT). Não dependa de rede em tempo de configure para o gl3w; só o RmlUi é fetchado.
- **`golden_test` é flaky no llvmpipe.** O render de tiles em threads + não-associatividade de ponto flutuante geram MSE alto entre execuções idênticas. Por isso é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) e o gate padrão usa `render_sanity` (estatístico). Gere a referência em GPU real com `glintfx_capture`.
- **`snapshot()` sai com flip vertical** (origem do `glReadPixels`). Item aberto na INBOX do `TODO.md`.
- **Sintaxe RCSS do RmlUi 6.3 difere do CSS.** Cor primeiro em `box-shadow`/`drop-shadow`; gradiente via `decorator:`; hex `#rrggbbaa`. Ver `glintfx/demos/showcase/showcase.rcss` (fonte de verdade) e [`docs/effects.md`](docs/effects.md).
- **Ciclo de vida do data-model é fixo e enforçado.** `create_data_model(name) → bind_number/string/bool/list(key) → load() → set_*(key, value)`. O RmlUi compila as views de data-binding no `load()`; chamar `bind_*` depois retorna `false` e o engine loga a violação. Vale igual em `App` e `UiLayer`. Memória dos slots é do engine (`DataBinder`, um `std::unique_ptr` por chave em `std::map`) -- o consumidor passa valores por cópia, nunca guarda ponteiro cru. Detalhe: `docs/embed-integration.md` seção 6.
- **Gate de encapsulamento: grep include-based, não nome cru de macro.** Ao validar que nenhum tipo de terceiro (GL/GLFW/RmlUi) vaza no header público, faça grep por `#include` em `glintfx/include/glintfx/`, não por substring de macro -- `GLINTFX_BACKEND_GLFW` casa a string "GLFW" mas não é um leak de tipo. O teste real é "nenhum header de GL/GLFW/RmlUi incluído ou tipo deles referenciado em `app.hpp`/`ui_layer.hpp`/`ui_event.hpp`".
- **CI Codeberg (Forgejo Actions).** Runner `codeberg-medium` (4 CPU/8 GB/10 min) + container `ghcr.io/catthehacker/ubuntu:act-latest`; deps de sistema instaladas a cada job (sem stack pré-instalada), incluindo os pacotes `-dev` de GL/EGL/xkbcommon. Matriz roda os dois valores de `GLINTFX_BACKEND_GLFW` (ON/OFF). Validar mudanças no workflow localmente com `forgejo-runner exec` antes de empurrar -- ver `.forgejo/workflows/ci.yml`.
