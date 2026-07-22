# AGENTS.md

> **EN:** Instructions for AI agents working in this repository. Read this before touching code.
> **PT:** Instruções para agentes de IA que trabalham neste repositório. Leia antes de tocar no código.

This repository hosts two independent tracks. Know which one you are in before editing.

| Layer / Camada | Path | Status | Build |
| :--- | :--- | :--- | :--- |
| **Layer 1: glintfx** (the released library, **active product**) | `glintfx/`, `consumer-example/` | **v0.11.2**, released, two consumption modes (`App` + `UiLayer`) | CMake |
| **Layer 0: `loucura_c_asm`** (sovereign runtime, **implementation complete and audited**) | `src/`, `include/` | freestanding pipeline + syscalls + `exit`/`write`/`read` -> own test harness (`C1`) -> core libc (memory/string/int-float-string conversion) -> mini-printf -> free-list allocator over `mmap` -> clean-room `SOV-SFNT`/`SOV-RAST`/`SOV-HINT` font engine, zero libc, under TDD, tag `core-v0.4.0` | `Makefile` (root) |

Layer 1 (glintfx) is C++ linking real libraries; it is the repository's **active product**. Layer 0 is pure C + Assembly with **zero libc**, talking to the kernel only via syscalls; its implementation is **complete and audited** (freestanding `_start` + raw syscall wrappers + `exit`/`write`/`read` helpers, a hand-rolled test harness enabling TDD, a small core libc for memory/string/int-float-string-conversion, a mini-printf, a free-list allocator over `mmap`, and the `SOV-SFNT`/`SOV-RAST`/`SOV-HINT` font engine that backs glintfx's own font rasteriser), delivered under TDD and adversarial review. All `A*`-`E*`/`AUD-*` items are `✅ Concluído`, tag `core-v0.4.0`: the original `AUD-ABI`/`AUD-SEC` audits (`core-v0.1.0`) plus a delta re-audit wave (`AUD-ABI-Δ`/`AUD-SEC-Δ`, covering everything landed since) both closed CONFORME-COM-RESSALVAS (compliant with caveats), 0 CRITICAL. It remains a **long-term internalization target** -- full clean-room internalization of RmlUi/FreeType/GLFW is still years away (two pieces are already internalized: the GL loader, `L1.14-GLLOADER` -- glintfx's own clean-room GL 3.3 core loader, `glintfx/src/gl_loader.{h,c}`, generated from the public Khronos gl.xml registry -- and the font engine, `L1.20-FONTFLIP`, glintfx's default text rasteriser since v0.10.0). They do **not** link to each other; the boundary is the process, not the linker. See [ADR-0006](docs/adr/0006-layered-hybrid-architecture.md).

---

## English

### Build and test (glintfx, Layer 1)

System dependencies (Fedora): `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`.

```sh
# configure + build (RmlUi 6.3 fetched automatically; glintfx's own GL loader is vendored)
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j

# run the showcase demo (all five effects; needs a real GPU for mask)
./glintfx/build/demos/showcase/glintfx_showcase

# run the test suite (headless, under Xvfb)
ctest --test-dir glintfx/build --output-on-failure
```

**`GLINTFX_BACKEND_GLFW`** (CMake option, default `ON`) controls how much of the library is compiled:

- `ON` (default): compiles the standalone `glintfx::App` facade, `window_glfw.cpp`, and the RmlUi GLFW platform adapter; links `glfw`. The suite has **31 tests** under Xvfb: `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, `app_dp_ratio_smoke`, `app_click_callback_smoke`, `app_element_box_smoke`, `render_sanity` (structural pixel-statistics, tolerant of llvmpipe non-determinism), `data_model_smoke`, `data_model_scalar`, `data_model_list`, `texture_png_alpha`, `app_input_hardening_smoke` (GLFW-only parity for the v0.3.0 API guards), `data_model_rebind_uaf` (GLFW-only, `App`-based duplicate-key `bind_*` regression, v0.3.1), `app_scroll_smoke` (GLFW-only, `App` parity for the programmatic scroll methods -- `App` has no `process_event`, so wheel forwarding itself is `UiLayer`-only, v0.4.0), plus the 16 embed tests below.
- `OFF` (embed-only): only `Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock` are compiled, `glfw` is **not** linked into the library (test fixtures still use GLFW via a test-only helper target). Runs the 16 embed tests: `ui_layer_attach`, `ui_layer_compose`, `gl_state_guard`, `ui_layer_events`, `ui_layer_sanity`, `dp_ratio_sanity`, `base_url_sanity`, `ua_stylesheet_sanity`, `click_callback_sanity`, `element_box_sanity`, `viewport_origin_sanity`, `polygon_sanity`, `input_hardening_sanity`, `polygon_gradient_sanity` (v0.3.1), `document_reload_leak` (v0.3.1), `scroll_sanity` (wheel forwarding + programmatic scroll, v0.4.0). This is the build mode SDL3/X11 hosts (e.g. GusWorld) consume -- see [ADR-0008](docs/adr/0008-embed-guest-mode.md). Mouse coordinates and `get_element_box` results are in window-space physical pixels; `set_viewport(x, y, w, h, target_h)` converts them to OpenGL's bottom-up viewport internally -- see [`docs/embed-integration.md`](docs/embed-integration.md) section 10 for the full coordinate contract.

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BACKEND_GLFW=OFF -DGLINTFX_BUILD_TESTS=ON   # embed-only
```

The pixel-exact `golden_test` is **opt-in** and flaky under software GL (requires `GLINTFX_BACKEND_GLFW=ON`):

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_GOLDEN_TEST=ON   # real GPU only
```

To verify drop-in consumption end to end, build `consumer-example/`, which pulls glintfx via `FetchContent` (`SOURCE_DIR`) and links `glintfx::glintfx` with no GL/GLFW/RmlUi references.

### Local gate before pushing (opt-in pre-push hook)

`tools/preci.sh` (TST-L1-PRECI) mirrors the fast day-to-day slice of CI **before** code becomes visible on a remote. Activate once per clone with `git config core.hooksPath .githooks`; `.githooks/pre-push` then runs it in fast mode on every `git push` and aborts the push on any failure. Fast mode detects which layer(s) were actually touched (`origin/main...HEAD` union the working tree) and runs only the matching build+test -- Layer 0 (`make build && make test`) and/or Layer 1 (`glintfx/build-preci`, `GLFW=ON` config, `ctest` under Xvfb). `tools/preci.sh --full` runs the wide net (both glintfx configs, sequentially -- one build in memory at a time -- plus `check_encapsulation.sh` and `gitleaks` if installed) for occasional manual runs, e.g. before tagging a release. See [`TESTES.md`](TESTES.md#tst-l1-preci).

### CI policy: gate order and where each check runs

Three CI surfaces exist; this repo has **no way to aggregate them cross-platform** (GitHub Actions and Forgejo Actions do not share a status API), so the gate order below is an **operational convention the orchestrator enforces by hand**, not a config setting.

**Gate order: `github` > `claudio` (self-hosted, local) > `codeberg` -- one green is enough.**

- **`github`** (`.github/workflows/ci.yml`, `ubuntu-latest`) is the **primary gate**. Runs on push to `main`, on tags, and on PRs; matrix over `GLINTFX_BACKEND_GLFW` (ON/OFF). A green run here is sufficient to merge or tag.
- **`claudio`** (`.forgejo/workflows/heavy.yml`, `runs-on: docker`, `container: fedora:42`) is the **local self-hosted Forgejo runner**. It carries the **heavy, blocking gates** -- ASan (`sanitize`) and the own-font-engine build (`fonteng`) -- on PR-to-`main`, tags, and manual dispatch. Reproduce a `claudio` failure locally in the same `fedora:42` container (`tools/ci/Containerfile.f42`) **before** iterating by push; this caught two real class-of-bug misses that only reproduced in that base image (missing `libasan`/`libubsan`, and a `SYS_futex` gap under clang's sanitizer runtime).
- **`codeberg-medium`** (`.forgejo/workflows/ci.yml`) now carries a **single minimal-parity job only** -- it does **not** block. A job stuck in `waiting` on Codeberg's shared runner queue never holds up a merge, tag, or release: `github` or `claudio` green is enough by itself.
- **Nightly ASan** (`.github/workflows/nightly.yml`, GitHub cron) stays as a **safety net**, independent of the blocking PR/tag gate. The Forgejo-side nightly (`nightly.forgejo`) was removed -- `claudio`'s `sanitize` job now covers that ground synchronously, on every PR/tag rather than once a day.

Practical rule for agents: reproduce first (`tools/ci/Containerfile.f42` for anything that fails only on `claudio`), iterate locally, and only then push. Do not chase a `waiting` Codeberg job when `github` or `claudio` is already green.

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
                          stb_image_impl.cpp, gl_state.hpp, system_clock.cpp,
                          gl_loader.h/.c (L1.14-GLLOADER, generated -- see tools/gen_glloader.py;
                          every exported pointer is `glx_<cmd>`-prefixed, ADR-0013/AUD-L1-GLSYM)
  demos/showcase/        showcase.rml/.rcss + glintfx_showcase + glintfx_capture
  tests/                 ctest smokes + sanity tests (31 with GLFW=ON, 16 embed-only) + opt-in
                          golden_test (Xvfb)
  third_party/khronos/   gl.xml (Apache-2.0, input to tools/gen_glloader.py) + Khronos headers
                          glcorearb.h/khrplatform.h (MIT)
  third_party/stb/       vendored stb_image.h (public domain / MIT) for PNG/JPG decode
  CMakeLists.txt
consumer-example/        drop-in proof: consumes glintfx via FetchContent
src/, include/           Layer 0: C/ASM runtime (implementation complete, pending audit), built via
                          the root Makefile (`make build`/`test`/`run`/`clean`), distinct from
                          glintfx's CMake build
docs/adr/                ADRs 0001-0005 (Layer 0) + 0006-0008 (layers, license, embed mode)
docs/embed-integration.md  host integration contract for UiLayer (frame lifecycle, GL state,
                          dp_ratio, base URL, data model, textures) -- authoritative reference
docs/superpowers/        design specs + plans for glintfx v1 and v2
TODO.md                  pendings + planning for both layers and v2 (+ INBOX)
NOTICE, LICENSE          MPL-2.0 + third-party attributions
```

### The public API surface (do not invent)

The public API lives in two headers under `glintfx/include/glintfx/`: [`app.hpp`](glintfx/include/glintfx/app.hpp) (the standalone facade `glintfx::App`, RAII move-only, plus `AppConfig` and `version()`) and [`ui_layer.hpp`](glintfx/include/glintfx/ui_layer.hpp) (the embed/guest facade `glintfx::UiLayer`, plus `UiLayerConfig`; events via [`ui_event.hpp`](glintfx/include/glintfx/ui_event.hpp)). Both expose the same data-model API (`create_data_model`/`bind_*`/`set_*`) and the same `set_dp_ratio`/`set_asset_base_url` methods -- see `docs/embed-integration.md` for the full contract. There is **no imperative effect API**: effects are declared in `.rcss`. Do not document or call methods that are not in those two headers.

### Where to run window/input verification (canonical, DOC-HOSTIN follow-up)

**WARNING, the single most important point of this section (more than the table below): bringing up the nested compositor is NOT enough. `-u WAYLAND_DISPLAY` does not protect you.** This is not theoretical -- it happened in this exact repository, in real time, while this very section was being written: a `qa-engineer` brought up nested KWin correctly, PROVED the nested display was `:1`, separate from the leader's `:0`, and its probe still opened a real window in the leader's LIVE session for 2 to 3 minutes.

- **Why:** libwayland's `wl_display_connect(NULL)` falls back to the BUILT-IN name `"wayland-0"` the moment the env var is absent, and it resolves that name INSIDE `$XDG_RUNTIME_DIR` -- which is still the real session's `/run/user/1000` unless you changed it too. Unsetting `WAYLAND_DISPLAY` only removes the override; it does not stop the connection.
- **The real protection:** your OWN `XDG_RUNTIME_DIR` (`chmod 700`), exported BEFORE bringing up the nested compositor and used by BOTH the nested compositor AND the app under test, so that no `wayland-0` socket exists anywhere for the fallback to find.
- **Prove it, do not assume it:** `lsof -p <pid>` or `/proc/<pid>/fd` showing the socket inside the isolated directory -- BEFORE launching and AGAIN after the app comes up. If the proof does not close cleanly, KILL the process immediately; do not adjust anything in the dark.
- **The parent lesson, which outlives this one case:** the house's own canonical recipe already had the antidote -- the standard ctest line is `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ...` (e.g. `docs/superpowers/plans/2026-07-19-framework2d-A1-input.md:231`), and it is the **`XDG_RUNTIME_DIR` swap that does the actual work, not the `-u`**. It got copied halfway. **When a canonical invocation already exists in the project, copy it WHOLE, then adapt** -- every piece is there because someone already got burned.

Factual record, kept sober (this is engineering documentation, not a scolding): the incident was CONTAINED -- no input was ever injected into `:0`, only read-only probes, and it was one of those very probes that revealed the leak; the process was killed and the cleanup confirmed. The value of documenting this is the trap, not whoever fell into it.

| | Xvfb (CI's own) | Xephyr | Nested KWin (the house's choice) |
| :--- | :--- | :--- | :--- |
| Display | phantom, memory-only | real window in the session | real window in the session |
| Window manager | **none** | **none** (it is just the X server) | **yes, full** |
| X button / alt-tab / minimize | do not exist | do not exist by themselves | exist |
| Installed on this machine | yes | **no** | **yes** (`/usr/bin/kwin_wayland`) |

**Canonical choice: `kwin_wayland --windowed --xwayland`.** Reasons, in this order: zero install (Xephyr would need two packages, because it is **not** a window manager, it is only the display server), and it is the **same compositor as the leader's own session**, hence more faithful to the real environment than a minimalist WM nobody actually uses.

**What only the nested compositor proves.** Without a window manager there is no title bar, hence no real click on the X button, alt-tab, or minimize -- so close veto (`set_close_request_callback`), window focus, and iconify only get end-to-end coverage there. Under Xvfb the honest maximum is testing the pure decision seam (e.g. `glfw_decide_window_close()`, `glintfx/src/glfw_event_translate.hpp:608` -- cite the real `file:line`, and `tools/check_doc_line_refs.sh` now exists to confirm that citation has not rotted) and DECLARING that as a downgrade, never selling it as end-to-end.

`xdotool`/XTest **works under Xvfb** for physical key and mouse input (this is how callback reentrancy was proven with a real keystroke in Onda 2, `HOSTIN-2`); what is actually missing under Xvfb is only the window manager.

**Hard rule: never on the leader's live session.** Input injection always targets the **nested** display, **never** `:0`. The reason is on record, not hypothetical: a burst of window-mode switching once froze his touchpad until a reboot was required (see the memory `feedback_nunca_stress_janela_sessao_viva`).

**An agent does not install a system package on its own initiative** -- ask for authorization first. Reporting "not executed, needs `<package>` installed" is an honest negative result, and a better one than improvising around a missing tool.

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
- Gate local pre-push: `git config core.hooksPath .githooks` ativa `tools/preci.sh` via `.githooks/pre-push` (TST-L1-PRECI) -- ver seção "Local gate before pushing" acima e [`TESTES.md`](TESTES.md#tst-l1-preci).
- [`docs/embed-integration.md`](docs/embed-integration.md): contrato de integração do `UiLayer` para hosts (fonte de verdade para frame lifecycle, GL state, dp_ratio, base URL, data model, texturas) -- leia antes de mexer no caminho embed.
- [`TODO.md`](TODO.md): tabela de pendências (ondas, IDs, pré-requisitos) das duas camadas, a **INBOX** (descobertas não priorizadas) e o escopo planejado da **v2** (component library / Atomic Design, spec em `docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`, branch `feat/v2-f2-components`, pausada).
- **Sistema de memória:** memórias tipadas em `~/.claude/projects/<slug>/memory/` (índice em `MEMORY.md`), autocarregadas por sessão. Não duplicar o que o repo já registra; registrar só o não óbvio.
- [`docs/adr/`](docs/adr/README.md): decisões de arquitetura (imutáveis quando `Accepted`; para mudar, escrever novo ADR que substitua). ADR-0006 (camadas), ADR-0007 (licença), ADR-0008 (embed/guest mode).

### Política de CI: ordem de gate e o que roda onde

Existem três superfícies de CI; este repo **não tem como agregá-las cross-plataforma** (GitHub Actions e Forgejo Actions não compartilham API de status), então a ordem abaixo é uma **convenção operacional que o orquestrador aplica manualmente**, não uma config.

**Ordem de gate: `github` > `claudio` (self-hosted local) > `codeberg` -- basta UM verde.**

- **`github`** (`.github/workflows/ci.yml`, `ubuntu-latest`) é o **gate primário**. Roda em push para `main`, em tags e em PRs; matriz sobre `GLINTFX_BACKEND_GLFW` (ON/OFF). Um run verde aqui já basta pra merge ou tag.
- **`claudio`** (`.forgejo/workflows/heavy.yml`, `runs-on: docker`, `container: fedora:42`) é o **runner self-hosted local do Forgejo**. Carrega os **gates pesados e bloqueantes** -- ASan (`sanitize`) e o build do motor de fonte próprio (`fonteng`) -- em PR→`main`, tag e dispatch manual. Reproduza uma falha do `claudio` localmente no mesmo container `fedora:42` (`tools/ci/Containerfile.f42`) **antes** de iterar por push; a política já pegou dois achados assim, que só reproduziam nessa imagem-base (`libasan`/`libubsan` faltando, e um gap de `SYS_futex` sob o runtime de sanitizer do clang).
- **`codeberg-medium`** (`.forgejo/workflows/ci.yml`) ficou só com **um job de paridade mínima** -- não bloqueia. Um job preso em `waiting` na fila do runner compartilhado do Codeberg NUNCA segura merge, tag ou release: `github` ou `claudio` verde já basta sozinho.
- **Nightly ASan** (`.github/workflows/nightly.yml`, cron do GitHub) permanece como **rede de segurança**, independente do gate bloqueante de PR/tag. O nightly do lado Forgejo (`nightly.forgejo`) foi removido -- o job `sanitize` do `claudio` já cobre esse terreno de forma síncrona, em todo PR/tag em vez de uma vez por dia.

Regra prática para agents: reproduza primeiro (`tools/ci/Containerfile.f42` para qualquer coisa que só falhe no `claudio`), itere localmente, e só então empurre. Não persiga um job `waiting` no Codeberg quando `github` ou `claudio` já estão verdes.

### Gotchas críticos (RmlUi / GL3 / embed mode): leia antes de mexer no renderer

- **Premultiplied alpha, backbuffer opaco e composição sempre no FBO 0.** O `RenderInterface_GL3` trabalha com alpha premultiplicado (`GL_ONE, GL_ONE_MINUS_SRC_ALPHA`). No modo standalone, garanta o backbuffer **opaco** (alpha=1 no compositor) ou o compositor (Wayland/X) deixa a janela translúcida -- fix já aplicado em `window_glfw.cpp`/`render_gl3.cpp`. No embed mode, `UiLayer::render()` é **compose-only** (sem `glClear`, sem swap) e compõe **incondicionalmente sobre o FBO 0** com origem de viewport hardcoded em `(0,0)` -- um FBO custom do host ligado antes de `render()` **não** é o alvo (limitação F1 documentada no [ADR-0008](docs/adr/0008-embed-guest-mode.md) e em `docs/embed-integration.md` seções 0 e 2). Texturas decodificadas por `LoadTexture` (PNG/JPG via stb_image, v0.2.3) também passam por premultiply in-place antes do upload, pelo mesmo motivo.
- **MSAA desligado no render layer.** `RMLUI_NUM_MSAA_SAMPLES=0` é necessário sob Mesa/llvmpipe (Xvfb): o resolve `glBlitFramebuffer` MSAA para não-MSAA produz silenciosamente textura de pós-processo preta. Ver comentário no `glintfx/CMakeLists.txt`.
- **Card `mask` crasha no software GL.** Sob Mesa/llvmpipe o shader BlendMask dual-sampler provoca corrupção de heap (`free(): invalid next size`), bug do Mesa SW renderer e **não** do glintfx. O CI headless usa `showcase_test.rml` (sem o card mask). O card mask só roda em **GPU real**.
- **O loader GL (`glintfx/src/gl_loader.{h,c}`) é gerado e commitado offline (L1.14-GLLOADER).** `tools/gen_glloader.py` produz os dois arquivos a partir do registro `gl.xml` da Khronos (Apache-2.0, vendorizado em `glintfx/third_party/khronos/gl.xml`) mais os headers `glcorearb.h`/`khrplatform.h` (MIT, mesmo diretório). Não dependa de rede em tempo de configure para o loader GL; só o RmlUi é fetchado. Regenerar só se o alvo de versão/profile mudar (`python3 tools/gen_glloader.py`), nunca editar `gl_loader.{h,c}` à mão.
- **Todo ponteiro exportado do loader GL usa o prefixo `glx_` (ADR-0013/AUD-L1-GLSYM, `v0.11.2`) -- nunca reintroduzir nome GL cru como símbolo global.** `tools/gen_glloader.py` emite `extern PFN<CMD>PROC glx_<cmd>;` em vez do nome cru (`glClear` etc.), mais um `#define <cmd> glx_<cmd>` no header PRIVADO (`gl_loader.h`, nunca instalado sob `glintfx/include/glintfx/`) que mantém os call sites internos (`render_gl3.cpp`, decorators) inalterados. Sem o prefixo, os 344 ponteiros viravam símbolos de dado BSS globais com nome idêntico às funções GL reais -- um host embed que chame `glClear` pelo nome cru e linke `libglintfx.a` antes do `libGL.so` resolvia pro slot BSS não-inicializado da glintfx em vez do driver, `SIGSEGV` na 1ª chamada GL (achado real via `nm`, não hipotético). Ao regenerar o loader, o prefixo é preservado automaticamente pelo gerador; o gate de regressão é `nm libglintfx.a | grep -E ' [BbDd] gl[A-Z]'` (0 matches esperados).
- **`golden_test` é flaky no llvmpipe.** O render de tiles em threads + não-associatividade de ponto flutuante geram MSE alto entre execuções idênticas. Por isso é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) e o gate padrão usa `render_sanity` (estatístico). Gere a referência em GPU real com `glintfx_capture`.
- **`snapshot()` sai com flip vertical** (origem do `glReadPixels`). Item aberto na INBOX do `TODO.md`.
- **Sintaxe RCSS do RmlUi 6.3 difere do CSS.** Cor primeiro em `box-shadow`/`drop-shadow`; gradiente via `decorator:`; hex `#rrggbbaa`. Ver `glintfx/demos/showcase/showcase.rcss` (fonte de verdade) e [`docs/effects.md`](docs/effects.md).
- **Ciclo de vida do data-model é fixo e enforçado.** `create_data_model(name) → bind_number/string/bool/list(key) → load() → set_*(key, value)`. O RmlUi compila as views de data-binding no `load()`; chamar `bind_*` depois retorna `false` e o engine loga a violação. Vale igual em `App` e `UiLayer`. Memória dos slots é do engine (`DataBinder`, um `std::unique_ptr` por chave em `std::map`) -- o consumidor passa valores por cópia, nunca guarda ponteiro cru. Detalhe: `docs/embed-integration.md` seção 6.
- **Gate de encapsulamento: grep include-based, não nome cru de macro.** Ao validar que nenhum tipo de terceiro (GL/GLFW/RmlUi) vaza no header público, faça grep por `#include` em `glintfx/include/glintfx/`, não por substring de macro -- `GLINTFX_BACKEND_GLFW` casa a string "GLFW" mas não é um leak de tipo. O teste real é "nenhum header de GL/GLFW/RmlUi incluído ou tipo deles referenciado em `app.hpp`/`ui_layer.hpp`/`ui_event.hpp`".
- **`GLINTFX_OWN_FONT_ENGINE` (`L1.19-FONTENG`) -- include path deve ser por-arquivo, não de alvo inteiro.** A raiz `include/` da Camada 0 contém um `limits.h` próprio (freestanding, só `INT_MAX`/`INT_MIN`/`UINT_MAX`) que SOMBREIA o `<limits.h>` de sistema (sem `SHRT_MAX`/`SHRT_MIN`) se virar `-I` de escopo de alvo inteiro no `glintfx` -- quebrou o `stb_image.h` vendorizado (`stb_image_impl.cpp`), unidade de tradução sem nenhuma relação com font engine. Fix: `set_source_files_properties(... PROPERTIES INCLUDE_DIRECTORIES ...)` restrito SÓ aos 3 arquivos que precisam (`font_engine_own.cpp`, `bootstrap.cpp`, `sfnt.c`/`raster.c` da Camada 0), nunca `target_include_directories()` de alvo inteiro. Ver `glintfx/CMakeLists.txt`.
- **`GLINTFX_OWN_FONT_ENGINE` -- `Rml::CallbackTexture` precisa ser liberado ANTES do `Rml::Shutdown()`.** `Rml::Shutdown()` chama `contexts.clear()` (destruindo o `RenderManager` de cada `Context`) ANTES de chamar `font_interface->Shutdown()` -- confiar só no hook `FontEngineInterface::Shutdown()` pra liberar o atlas GPU do nosso motor de fonte crasha em todo teardown ("Leaking CallbackTexture detected... will likely result in memory corruption"). `Bootstrap::shutdown()` chama `FontEngineOwn::Shutdown()` EXPLICITAMENTE, antes da própria chamada a `Rml::Shutdown()`. Ver `glintfx/src/bootstrap.cpp`/`font_engine_own.{hpp,cpp}` e `docs/embed-integration.md` seção 17.
- **CI Codeberg (Forgejo Actions) -- paridade mínima, não bloqueia.** Runner `codeberg-medium` (4 CPU/8 GB/10 min) + container `ghcr.io/catthehacker/ubuntu:act-latest`; deps de sistema instaladas a cada job (sem stack pré-instalada), incluindo os pacotes `-dev` de GL/EGL/xkbcommon. **Desde a Onda 3 da reestruturação de CI (poda, 2026-07-11)** este workflow roda **um único job** (`build & test (codeberg-medium / GLFW=ON, minimal parity)`) -- **sem matriz**, só `GLINTFX_BACKEND_GLFW=ON`, em `push` para `main` apenas -- como sinal voluntário de "ainda builda no Codeberg", **não** gate de release. A perna `GLFW=OFF`, o `lint-and-scan`, o `coverage` e o `nightly.forgejo` foram removidos deste arquivo (cobertos pelo GitHub e pelo `claudio`/`heavy.yml`); um job preso em `waiting` no `codeberg-medium` nunca segura merge/tag/release (ver "Política de CI: ordem de gate" acima -- `github > claudio > codeberg`). Validar mudanças no workflow localmente com `forgejo-runner exec` antes de empurrar -- ver o cabeçalho de `.forgejo/workflows/ci.yml`.

### Onde rodar teste de janela/input (canônico, follow-up do DOC-HOSTIN)

**ATENÇÃO, o ponto mais importante desta seção (mais que a tabela abaixo): subir o compositor aninhado NÃO basta. `-u WAYLAND_DISPLAY` não protege.** Isto não é teórico -- aconteceu de verdade neste repositório, em tempo real, enquanto esta própria seção estava sendo escrita: um `qa-engineer` subiu o KWin aninhado corretamente, PROVOU que o display aninhado era o `:1`, separado do `:0` do líder, e mesmo assim a sonda dele abriu uma janela real na sessão VIVA do líder por 2 a 3 minutos.

- **Por quê:** o `wl_display_connect(NULL)` do libwayland cai no nome EMBUTIDO `"wayland-0"` no instante em que a env var some, e resolve esse nome DENTRO do `$XDG_RUNTIME_DIR` -- que continua sendo o `/run/user/1000` da sessão real, a menos que ele também tenha sido trocado. Remover `WAYLAND_DISPLAY` só tira o *override*, não impede a conexão.
- **A proteção real:** um `XDG_RUNTIME_DIR` PRÓPRIO (`chmod 700`), exportado ANTES de subir o compositor aninhado e usado tanto pelo compositor aninhado QUANTO pelo app sob teste, de forma que não exista nenhum socket `wayland-0` em lugar nenhum pro fallback encontrar.
- **Provar, não presumir:** `lsof -p <pid>` ou `/proc/<pid>/fd` mostrando o socket dentro do diretório isolado -- ANTES de lançar e DE NOVO depois de o app subir. Se a prova não fechar limpa, MATAR o processo na hora; não ajustar nada no escuro.
- **A lição mãe, que vale além deste caso:** a receita canônica da própria casa já tinha o antídoto -- a linha padrão de ctest é `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ...` (ex.: `docs/superpowers/plans/2026-07-19-framework2d-A1-input.md:231`), e é a **troca do `XDG_RUNTIME_DIR` que faz o trabalho de verdade, não o `-u`**. Foi copiada pela metade. **Quando já existe invocação canônica no projeto, copie-a INTEIRA e só então adapte** -- cada pedaço está lá porque alguém já se queimou.

Registro factual, mantido sóbrio (é doc de engenharia, não bronca): o incidente foi CONTIDO -- nenhum input foi injetado no `:0`, só buscas read-only, e foi justamente uma dessas buscas que revelou o vazamento; o processo foi morto e a limpeza confirmada. O valor de documentar isto é a armadilha, não quem caiu nela.

| | Xvfb (o do CI) | Xephyr | KWin aninhado (o da casa) |
| :--- | :--- | :--- | :--- |
| Tela | fantasma, só memória | janela real na sessão | janela real na sessão |
| Window manager | **nenhum** | **nenhum** (é só o servidor X) | **sim, completo** |
| Botão X / alt-tab / minimizar | não existem | não existem sozinhos | existem |
| Instalado nesta máquina | sim | **não** | **sim** (`/usr/bin/kwin_wayland`) |

**Escolha canônica: `kwin_wayland --windowed --xwayland`.** Motivos, nessa ordem: zero instalação (o Xephyr exigiria dois pacotes, porque **não** é window manager, é só a tela) e é o **mesmo compositor da sessão do líder**, logo mais fiel ao ambiente real do que um WM minimalista que ninguém usa.

**O que só o aninhado prova.** Sem window manager não há barra de título, logo não há clique real no botão X, alt-tab nem minimizar -- então o veto de close (`set_close_request_callback`), o focus e o iconify de janela só têm cobertura ponta-a-ponta lá. Sob Xvfb o máximo honesto é testar o *seam* puro de decisão (ex.: `glfw_decide_window_close()`, `glintfx/src/glfw_event_translate.hpp:608` -- cite o `arquivo:linha` real, e o `tools/check_doc_line_refs.sh` agora existe justamente pra confirmar que essa citação não apodreceu) e **declarar isso como downgrade**, nunca vender como e2e.

`xdotool`/XTest **funciona sob Xvfb** para tecla e mouse físicos (foi assim que a reentrância de callback foi provada com uma tecla real na Onda 2, `HOSTIN-2`); o que de fato falta no Xvfb é só o window manager.

**Regra dura: nunca na sessão viva do líder.** A injeção de input mira **sempre** o display aninhado, **nunca** o `:0`. O motivo está registrado, não é hipotético: uma rajada de troca de modo de janela já travou o touchpad dele até precisar de reboot (ver a memória `feedback_nunca_stress_janela_sessao_viva`).

**Agente não instala pacote de sistema por conta própria** -- pede autorização primeiro. Reportar "não executado, precisa instalar `<pacote>`" é um resultado negativo honesto, e melhor do que improvisar em cima de uma ferramenta ausente.
