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

### Local gates (opt-in hooks): pre-commit and pre-push

Two opt-in hooks, both wrapper scripts in `.githooks/` that delegate to a `tools/*.sh` script -- neither is installed by default (git only looks in `.git/hooks/` out of the box):

- **`.githooks/pre-commit` -> `tools/precommit.sh` (GATE-PRECOMMIT).** Runs on every `git commit`, staged files only: cppcheck on staged `glintfx/src/*.cpp|*.hpp` (same flags as CI's TST-L1-STATIC), `clang-format-diff` on the staged diff, and `tools/check_doc_line_refs.sh` when (and only when) the staged set is relevant to it -- a doc file itself, or a staged file's basename cited `path:N`-shaped somewhere under `docs/`. Typical cost: tens of ms; the doc-refs sub-check only fires when relevant (~1.6s when it does -- see the script's own header for the 2026-07-23 false-positive it replaced, where a bare-substring match made touching `TODO.md` -- which nearly every commit does per this house's own "cite the ID" convention -- trigger the expensive path on almost every commit).
- **`.githooks/pre-push` -> `tools/preci.sh` (TST-L1-PRECI).** Mirrors the fast day-to-day slice of CI **before** code becomes visible on a remote. See below for what it runs.

**Activation has two valid routes** -- which one applies depends on whether `core.hooksPath` is already claimed globally by something else on this machine (found the hard way, 2026-07-23; full rationale in `tools/precommit.sh`'s header, "ACTIVATION -- TWO VALID ROUTES"):
- **(a) Plain clone, no global `core.hooksPath` set:** `git config core.hooksPath .githooks` -- git then reads both `.githooks/pre-commit` and `.githooks/pre-push` directly.
- **(b) A global `core.hooksPath` is already set** (e.g. this house's own `~/.claude/githooks`, the TODO.md freshness post-commit hook): do **not** override `core.hooksPath` locally -- git has exactly one value for it, so a local override *replaces* the global one wholesale and silently disables whatever that global hook does for this repo. Instead symlink (or copy) each hook straight into this repo's own git-dir hooks folder: `ln -s ../../.githooks/pre-commit .git/hooks/pre-commit` (and the same for `pre-push`). The global hook's own shim (`~/.claude/githooks/_chain.sh`) already delegates to `$(git rev-parse --git-common-dir)/hooks/<name>` when that local file is executable, so both systems coexist.

`tools/preci.sh` fast mode detects which layer(s) were actually touched (`origin/main...HEAD` union the working tree) and runs only the matching build+test -- Layer 0 (`make build && make test`) and/or Layer 1 (`glintfx/build-preci`, `GLFW=ON` config, `ctest` under Xvfb). `tools/preci.sh --full` runs the wide net (both glintfx configs, sequentially -- one build in memory at a time -- plus `check_encapsulation.sh` and `gitleaks` if installed) for occasional manual runs, e.g. before tagging a release. See [`TESTES.md`](TESTES.md#tst-l1-preci).

### Hosting: GitHub is the only public face

**Decision by the lead, 2026-07-25, standing until he says otherwise: the project lives on GitHub only.** Codeberg announced it no longer accepts LLM/AI-generated code, and this repository is written mostly by agents, so the lead is archiving the Codeberg repository himself.

What this means in practice:

- `https://github.com/petrinhu/glintfx` is the canonical URL for the repository, its releases, its issues, and its wiki. Every `codeberg.org` link is a future 404 and must not be reintroduced.
- The git remotes `origin` and `github` both point at GitHub. There is no "dual push" any more: one push, one remote.
- **The whole `.forgejo/` directory is gone**, deleted on 2026-07-25: the two Codeberg workflows (`ci.yml`, `core-ci.yml`, `runs-on: codeberg-medium`) and `heavy.yml` (the local `claudio` runner) alike. Do not recreate it.
- Historical prose in `CHANGELOG.md` still describes what past releases actually did, dual-remote mentions included. That is a record, not an instruction: do not rewrite it.

### CI policy: where each check runs

**All CI runs on GitHub. `.github/workflows/` is the only CI that exists.** (Lead's decision, 2026-07-25.)

- **`.github/workflows/ci.yml`** (`ubuntu-latest`) is the **gate**. Runs on push to `main`, on tags, and on PRs; matrix over `GLINTFX_BACKEND_GLFW` (ON/OFF). A green run here is what authorises a merge or a tag.
- **`.github/workflows/nightly.yml`** (GitHub cron) is the nightly ASan **safety net**, independent of the PR/tag gate.
- **The old Forgejo runner `claudio` is gone.** It was shut down and removed on 2026-07-25, and `.forgejo/` no longer exists in the repository. `forgejo-runner exec` is not a thing here: there is no Forgejo workflow left to validate with it.

**`.github/workflows/heavy.yml`** (self-hosted, containerized, ephemeral) carries the two heavy legs that used to run on `claudio`: `sanitize` (ASan/LSan/UBSan) and `fonteng` (own-font-engine, `GLINTFX_OWN_FONT_ENGINE=ON`), each as a 2-value `backend: [ON, OFF]` matrix -- four jobs total. The runner is `tools/ci/Containerfile.runner` (built on top of `glintfx-ci:f42`) supervised by a `systemd --user` unit (`glintfx-heavy-runner`) that re-registers a fresh ephemeral container after every job; hard caps of `--memory=8g --memory-swap=8g` (swap off) and `--cpus=4` (`-j2` literal in the builds, never `$(nproc)`), plus `--cap-drop=ALL` and `--security-opt=no-new-privileges`. Trigger is deliberately narrow -- `push` to `main` (path-filtered) and `workflow_dispatch` only, **zero `pull_request`** -- because this is a self-hosted runner on a public repository and GitHub warns that a fork's PR would otherwise run arbitrary code on this machine. Proven end-to-end more than once, most recently run `30368396354` (`workflow_dispatch`, 2026-07-28) with all four jobs green. The original Forgejo recipe it was ported from is archived at `~/.claude/receitas-ci/glintfx-heavy-claudio-forgejo.yml`; audit trail in `TODO.md`'s `GH-ONLY-RUNNER`/`AUD-CI-RUNNER` items.

**SEC-CI-HARDEN (2026-07-29, `AUD-CI-RUNNER` remediation, 2 of 3 IMPORTANT findings closed):** the anti-fork boundary described above used to be enforced only by the header comment on `heavy.yml`'s `on:` block -- nothing failed if a future edit ever reintroduced `pull_request` next to the `self-hosted` label. `.github/workflows/ci.yml`'s `lint-and-scan` job now runs `tools/check_workflow_self_hosted_gate.py` on every push/PR (GitHub-hosted, never on the self-hosted label itself, no secret in scope): it parses every `.github/workflows/*.yml` with PyYAML and fails if any job whose `runs-on` contains `self-hosted` also sits under a workflow triggering on `pull_request`/`pull_request_target`/`issue_comment`. It is deliberately structural, not textual -- a raw `grep pull_request heavy.yml` matches that file's own four explanatory comment lines and would give a false sense of coverage. Separately, `heavy.yml`'s four `uses:` lines (`actions/checkout`, `actions/cache`, both jobs) are now pinned to a full commit SHA with the version as a trailing comment, not the mutable `@v4` tag -- unpinned third-party actions on a self-hosted runner are a materially different risk than the same actions on a disposable GitHub-hosted VM (the risk tier `ci.yml`'s own still-unpinned `@v4` uses deliberately stay in). The third finding, `IMP-3` (no network egress restriction on the runner container -- LAN-pivot risk, only matters if the first two are ever defeated together), is tracked in the INBOX as `SEC-CI-EGRESS`, an accepted residual, not fixed here.

Two practices survive the move and both still matter:

- **Reproduce a container failure locally, by hand, before iterating by push.** `tools/ci/Containerfile.f42` and `tools/ci/build_image.sh` still work standalone: they build `localhost/glintfx-ci:f42` (fedora:42 with clang, cmake, `libasan`/`libubsan`, Xvfb and Mesa pre-installed) with a plain `docker build`, no runner involved. Run the failing command inside `docker run` against that image. This image caught two real class-of-bug misses that reproduced only on a Fedora base (missing `libasan`/`libubsan`, and a `SYS_futex` gap under clang's sanitizer runtime).
- **Run the pre-commit `ctest` locally** (`tools/preci.sh`, see the local-gates section above) rather than using CI as your first compiler. The heavy job only runs on push to `main`, so this is still the main thing standing between you and a regression before that point.

### Conventions

- **SPDX header in every code file.** First line: `SPDX-License-Identifier: Apache-2.0` (comment style per language: `//` for C/C++, `;` for NASM, `#` for CMake/Makefile/shell). Do **not** put SPDX in `.md` docs.
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
NOTICE, LICENSE          Apache-2.0 + third-party attributions
```

### The public API surface (do not invent)

The public API lives in two headers under `glintfx/include/glintfx/`: [`app.hpp`](glintfx/include/glintfx/app.hpp) (the standalone facade `glintfx::App`, RAII move-only, plus `AppConfig` and `version()`) and [`ui_layer.hpp`](glintfx/include/glintfx/ui_layer.hpp) (the embed/guest facade `glintfx::UiLayer`, plus `UiLayerConfig`; events via [`ui_event.hpp`](glintfx/include/glintfx/ui_event.hpp)). Both expose the same data-model API (`create_data_model`/`bind_*`/`set_*`) and the same `set_dp_ratio`/`set_asset_base_url` methods -- see `docs/embed-integration.md` for the full contract. There is **no imperative effect API**: effects are declared in `.rcss`. Do not document or call methods that are not in those two headers.

### API parity: declare the asymmetry, never leave it silent (API-PARITY-GATE, W22)

**Rule: every new public API declares parity between `App` and `UiLayer`, or justifies the asymmetry in its own doc-comment, citing the contract that imposes it.** The rule is NOT "every API must exist on both facades" -- real asymmetries exist and are legitimate (`UiLayer::set_viewport`'s offset/letterbox overload only makes sense for an embed host compositing into a sub-region; `App` owns the whole window, so it has no equivalent need). The rule is that the asymmetry be **stated**, never **silent**.

**Why this is a gate and not a style preference:** `GLPROC-EMBED` and `FRAMEGRAB-EMBED` were the *same process defect* twice in the same week -- a capability shipped, announced as the thing that "unlocks" an embed consumer, that in fact did not reach embed builds at all (`gl_proc_address()` was compiled out under `GLINTFX_BACKEND_GLFW=OFF`; `App::capture_frame()` shipped `App`-only while its own requesting consumer was embed-only). Both were caught only because a consumer tried to use the feature and hit a wall, not because anything in the codebase or its docs said "this exists on one facade, not the other, and here is why." A doc-comment that states the asymmetry up front turns that same gap into something a reviewer -- or the API's own author, re-reading it a week later -- catches by inspection, not by a blocked consumer.

**What "declared" looks like in practice, the two valid shapes:**
- **Symmetric:** both `App::foo()` and `UiLayer::foo()` exist, with the same contract (see `capture_frame()` itself, `FRAMEGRAB-TEX`/`FRAMEGRAB-EMBED`, which shipped `App`-only first and was deliberately extended to `UiLayer` once the gap was named).
- **Asymmetric, justified:** one facade only, with the doc-comment stating WHY the other does not need or cannot have it -- see [`glintfx/include/glintfx/frame_capture.hpp`](glintfx/include/glintfx/frame_capture.hpp)'s own top comment (`CAPTURE-FREE`, W22 S8) for a worked example: it explains, in the header itself, why the new instance-free `capture_framebuffer()` deliberately does NOT reuse `App::CapturedFrame` (does not exist in an embed-only build at all) nor `UiLayer::CapturedFrame` (would tie an instance-free function to the RmlUi-backed class, contradicting its own reason for existing) -- a third, independent type, with the asymmetry from its two closest siblings named and reasoned about, not silently duplicated.

**A `gl_proc_address()`-style function that is conceptually process-wide, not per-facade, is a THIRD legitimate shape** (see `gl_proc.hpp`'s own "WHY A FREE FUNCTION" note) -- the parity question for those is "does it work in BOTH `GLINTFX_BACKEND_GLFW=ON` and `OFF` builds", not "does it exist on both `App` and `UiLayer`".

**Enforcement today is by review, not by a script** -- add this to whatever review-adversarial checklist you are already running against a new public header (`TESTES.md`/`AUDITORIAS.md`, or an ad hoc review brief): before signing off a new symbol under `glintfx/include/glintfx/`, ask "does the sibling facade have this, and if not, does the doc-comment say why?" A missing answer is the same class of gap `GLPROC-EMBED`/`FRAMEGRAB-EMBED` both were.

### Where to run window/input verification (canonical, DOC-HOSTIN follow-up)

**WARNING, the single most important point of this section (more than the table below): bringing up the nested compositor is NOT enough. `-u WAYLAND_DISPLAY` does not protect you.** This is not theoretical -- it happened in this exact repository, in real time, while this very section was being written: a `qa-engineer` brought up nested KWin correctly, PROVED the nested display was `:1`, separate from the leader's `:0`, and its probe still opened a real window in the leader's LIVE session for 2 to 3 minutes.

- **Why:** libwayland's `wl_display_connect(NULL)` falls back to the BUILT-IN name `"wayland-0"` the moment the env var is absent, and it resolves that name INSIDE `$XDG_RUNTIME_DIR` -- which is still the real session's `/run/user/1000` unless you changed it too. Unsetting `WAYLAND_DISPLAY` only removes the override; it does not stop the connection.
- **The real protection is on the APP under test ONLY, not on the nested compositor itself.** The nested compositor must INHERIT the normal environment on purpose (it needs to find the real `wayland-0` to nest inside it); it is the app under test that gets an isolated `XDG_RUNTIME_DIR` (`chmod 700`), so that no `wayland-0` socket exists anywhere inside its own runtime dir for the fallback to find.
- **The validated invocation (this actually ran, corrected after a first draft got it wrong):**
  ```sh
  mkdir -p /var/tmp/glx-xdgrun && chmod 700 /var/tmp/glx-xdgrun
  # nested compositor: INHERITS the normal env on purpose (needs the real wayland-0 to nest into)
  # note: `--windowed` does NOT exist (confirmed via `kwin_wayland --help`) -- windowed IS the
  # default the moment you pass --wayland-display/--x11-display; `--virtual` is the headless mode
  kwin_wayland --wayland-display wayland-0 --width 640 --height 480 --xwayland --socket <unique-name>
  # find the new :N in /tmp/.X11-unix/ (X0 = the leader's session; the new one = the nested one)
  # app under test: THIS is where the isolated env goes
  env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 XDG_RUNTIME_DIR=/var/tmp/glx-xdgrun DISPLAY=:N <binary>
  # prove it BEFORE interacting with anything:
  lsof -p <pid> | grep -c libwayland   # must be 0
  ss -xp | grep <fd's inode>           # must point at @/tmp/.X11-unix/X<N>, never X0/wayland-0
  ```
- **`GLFW_PLATFORM` is not an env var** -- it is the `glfwInitHint()` constant (which does exist and works, called from application code, not from the shell). The env var GLFW actually honours during backend auto-selection is **`XDG_SESSION_TYPE=x11`** (with `WAYLAND_DISPLAY` unset), confirmed in isolation via `glfwGetPlatform()`. Also worth recording, because it saves the next person a detour: **this repo's own `App`/`WindowGlfw` calls neither `glfwInitHint` nor anything platform-selecting** (empty `grep` across `glintfx/src`/`glintfx/include`) -- so this is an invocation trap, not a product finding.
- **Prove it, do not assume it:** `lsof -p <pid>` or `/proc/<pid>/fd` showing the socket inside the isolated directory -- BEFORE launching and AGAIN after the app comes up. If the proof does not close cleanly, KILL the process immediately; do not adjust anything in the dark.
- **The parent lesson, which outlives this one case:** the house's own canonical recipe already had the antidote -- the standard ctest line is `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ...` (e.g. `docs/superpowers/plans/2026-07-19-framework2d-A1-input.md:231`), and it is the **`XDG_RUNTIME_DIR` swap that does the actual work, not the `-u`**. It got copied halfway. **When a canonical invocation already exists in the project, copy it WHOLE, then adapt** -- every piece is there because someone already got burned.

Factual record, kept sober (this is engineering documentation, not a scolding): the incident was CONTAINED -- no input was ever injected into `:0`, only read-only probes, and it was one of those very probes that revealed the leak; the process was killed and the cleanup confirmed. The value of documenting this is the trap, not whoever fell into it.

| | Xvfb (CI's own) | Xephyr | Nested KWin (the house's choice) |
| :--- | :--- | :--- | :--- |
| Display | phantom, memory-only | real window in the session | real window in the session |
| Window manager | **none** | **none** (it is just the X server) | **yes, full** |
| X button / alt-tab / minimize | do not exist | do not exist by themselves | exist |
| Installed on this machine | yes | **no** | **yes** (`/usr/bin/kwin_wayland`) |

**Canonical choice: `kwin_wayland --wayland-display wayland-0 --xwayland` (windowed is the default; see the validated invocation above -- `--windowed` is not a real flag).** Reasons, in this order: zero install (Xephyr would need two packages, because it is **not** a window manager, it is only the display server), and it is the **same compositor as the leader's own session**, hence more faithful to the real environment than a minimalist WM nobody actually uses.

**What only the nested compositor proves.** Without a window manager there is no title bar, hence no real click on the X button, alt-tab, or minimize -- so close veto (`set_close_request_callback`), window focus, and iconify only get end-to-end coverage there. Under Xvfb the honest maximum is testing the pure decision seam (e.g. `glfw_decide_window_close()`, `glintfx/src/glfw_event_translate.hpp:614` -- cite the real `file:line`, and `tools/check_doc_line_refs.sh` now exists to confirm that citation has not rotted) and DECLARING that as a downgrade, never selling it as end-to-end.

`xdotool`/XTest **works under Xvfb** for physical key and mouse input (this is how callback reentrancy was proven with a real keystroke in Onda 2, `HOSTIN-2`); what is actually missing under Xvfb is only the window manager.

**What did NOT work in the nested-compositor leg (KWin-nested-over-Wayland + its internal Xwayland): keyboard injection via `xdotool`/XTest is FLAKY on this exact stack.** Delivery is intermittent even with X11 focus confirmed, and one case produced a double toggle from a single call (suspected X11 auto-repeat, not confirmed). What DID work reliably there: window-manager actions (`windowminimize`, `windowactivate`) and an ICCCM `ClientMessage` (`WM_DELETE_WINDOW` via `python-xlib`) for the close-veto e2e. The recorded recommendation for a future keyboard-injection round is raw `uinput`, which bypasses X11/XTest entirely.

**Hard rule: never on the leader's live session.** Input injection always targets the **nested** display, **never** `:0`. The reason is on record, not hypothetical: a burst of window-mode switching once froze his touchpad until a reboot was required (see the memory `feedback_nunca_stress_janela_sessao_viva`).

**An agent does not install a system package on its own initiative** -- ask for authorization first. Reporting "not executed, needs `<package>` installed" is an honest negative result, and a better one than improvising around a missing tool.

### Mutation testing sandbox: sabotage a copy, never the shared build dir (BUILDDIR-MUTACAO, W23)

**The incident this exists to prevent (W22, `capture_framebuffer_smoke`, 2026-07-30):** two agents shared this working tree. One did mutation testing against `CAPTURE-FREE`'s `GL_PACK_ALIGNMENT=1` fix -- a real fix, in a TRACKED file -- and ran seven rebuild+execute cycles. A second agent, on an unrelated slice, ran the full `ctest` suite in the middle of that window and hit `capture_framebuffer_smoke` dying with `double free or corruption`. The orchestrator held back an unrelated commit over this and, on first read, blamed the wrong agent. The real cause was proven only by an independent source -- `coredumpctl` timestamps, not either agent's own account: 21 `capture_framebuffer_smoke` coredumps, all inside a closed 6-minute window, zero outside it, matching the mutating agent's own reported cycle count and crash-type mix.

**This is the third variant of shared-tree contamination found this wave, in order of subtlety:** (1) the INDEX -- a colliding `git add` pulls in a teammate's unrelated file; (2) the WORKING TREE -- reading a teammate's deliberate sabotage as a real defect; (3) the BUILD ARTIFACT -- the one this section is about. `git commit --only`/`git diff --cached` protect against neither of the first two, and protect against none of the third: by the time one agent runs `ctest`, the binary the other agent is mid-mutating is already compiled and on disk.

**The consequence that almost slipped through, more important than either tool below: any suite result measured inside a contaminated window is INVALID -- including the ones that passed GREEN.** A green test may have executed against a binary another agent mutated and rebuilt moments earlier; a green result under contamination proves nothing. The natural reflex is to chase the red test; the correct one is to distrust the entire run and re-run clean.

**`tools/mutation_sandbox.sh`** (`create`/`run`/`destroy`) gives every mutation-testing session an isolated, self-contained build tree in `/var/tmp`, materialized via `git archive` from a COMMITTED sha -- never the working tree -- so a reviewer sabotages a copy, never a tracked file a teammate might be reading at the same moment:

```sh
SBOX="$(tools/mutation_sandbox.sh create HEAD --name my-drill)"   # git archive + configure + build
tools/mutation_sandbox.sh run "${SBOX}" -R render_sanity          # incremental rebuild + ctest (Xvfb-isolated)
tools/mutation_sandbox.sh destroy "${SBOX}"                       # rm -rf, only if basename matches glx-mutsbx-*
```

Verified live while writing this section (`glx-mutsbx-docverify.<pid>`, from `HEAD`): `create` archived, configured and built glintfx cleanly; `run -R render_sanity` rebuilt incrementally and passed 5/5; `destroy` removed the sandbox. All three mandatory refusals were exercised live too: `run` against a path inside this repo's own `glintfx/` refuses (`"is inside a git working tree"`); `run` against `/tmp/...` refuses (`"/tmp is tmpfs (RAM) on this machine"`); `create` against a nonexistent sha refuses (`"does not resolve to a commit"`); and `destroy` against a real directory whose basename does not look like a sandbox refuses without deleting it (must match `glx-mutsbx-*`).

**`tools/ctest_guarded.sh`** wraps a `cmake --build` + `ctest` run -- same Xvfb/`XDG_RUNTIME_DIR` isolation `tools/preci.sh`'s `run_layer1_config()` already uses, copied whole -- with a before/after check: a snapshot of the tracked+staged+unstaged state of `glintfx/` (excluding `build*/`), AND a fingerprint of the actual test executables `ctest` is about to invoke (content sha256 + nanosecond mtime, both signals combined -- see why below). Either axis diverging prints the canonical `RODADA INVALIDA (BUILDDIR-MUTACAO)` message on stderr and exits non-zero, regardless of whether `ctest` itself reported pass or fail:

```sh
tools/ctest_guarded.sh glintfx/build-preci -R render_sanity
```

Verified live against the real `glintfx/build-preci`: a clean run built, ran 5/5 tests, and printed `OK (tracked tree AND test-executable fingerprints unchanged during build+test)`; an empty-selection run (`-R nome_que_nao_existe_xyz`) refused with `matched ZERO tests ... refusing to fingerprint nothing`, instead of silently reporting a valid guarded run over nothing; a run against an unconfigured directory refused with a `CMakeCache.txt not found` message. **Not re-verified for this doc slice:** the live contamination path itself (a genuine concurrent rebuild mid-run) -- reproducing it would mean sabotaging a second, real build dir while this one is running, exactly the risk this tool exists to remove from a shared tree. That behavior is documented instead from the script's own header, which records it as MEASURED (not deduced) against a disposable drill clone during `BUILDDIR-MUTACAO`'s own adversarial review -- see the commits at the end of this section.

**Why a content hash AND a nanosecond mtime, not a content hash alone:** this toolchain compiles deterministically (measured live during review: a `touch` with no content change still reproduces a byte-identical executable). A mutate/rebuild/restore/rebuild cycle that fully round-trips back to the original source therefore also round-trips back to the original CONTENT -- a content-hash-only fingerprint would see no difference at all in exactly the scenario `BUILDDIR-MUTACAO` exists to catch. Second-resolution mtime alone collides too: two genuine rebuilds were measured completing in ~1.16s total, comfortably inside one wall-clock second. Combining nanosecond mtime with a content hash closes both blind spots at once, each covering where the other is weak.

**Declared limits -- read before trusting either tool for more than it covers:**
- `ctest_guarded.sh`'s tree guard watches `glintfx/` only (`GUARD_PATHS`, excluding `build*/`) -- **Layer 0 (`src/`, `include/` at the repo root) is NOT covered.** A concurrent mutation there is invisible to this guard.
- The executable fingerprint covers exactly the binaries the invocation's own `-R`/`-E`/`-L` selection is about to exercise, not the whole build dir's test set -- a filtered run (e.g. `-R render_sanity`) does not fingerprint, and therefore does not protect, an unrelated binary rebuilt concurrently outside that filter.
- Neither tool sees mutation of an UNTRACKED file (never `git add`ed) -- the same trade-off this house's mutation-testing discipline already accepts elsewhere (sabotage a committed blob copied outside the tree, never an in-place untracked file).

**Three layers of adversarial review, each catching what the previous one missed** -- the same shape the W22 incident itself had, worth naming rather than hiding behind a clean final diff. `mutation_sandbox.sh`'s guard against a sandbox target inside any repo went through three rounds before it was airtight: round 1 (`6eabc9d`) compared the target against the SCRIPT's own location -- the wrong axis, since copying the script out of the tree to review it (exactly how this house verifies a committed blob) silently broke the guard; round 2 (`3f4a463`, `37f442f`) fixed the axis but still called `git rev-parse --show-toplevel` for discovery, which a poisoned `GIT_DIR` can fail open on -- even from inside a real repo; round 3 (`3c5516b`) found that the hand-rolled filesystem walk which replaced it was itself fail-open on an unreadable ancestor directory, the exact "failure to observe misread as a positive absence" shape as round 2's bug, via a different mechanism. `ctest_guarded.sh` went through an equivalent arc: the first version (`321227e`) snapshotted the tree only, blind to a mutation applied and reverted inside the guarded window -- the W22 incident's own 7-cycle shape; the fix for that (`6c6c941`) shipped with an empty-test-list gap; a follow-up (`cb57372`) closed a filter-scope gap; and a further round (`d65a9cc`) found and fixed three more issues together (the empty-list refusal, the mtime-resolution/content-hash combination, and the same location-anchor bug `mutation_sandbox.sh` had already hit) -- once someone went looking on purpose.

**At least one of those rounds corrected an explicit instruction with a measurement, and shipped the correction instead of the literal instruction.** The fix proposed for the mtime collision was to switch the fingerprint fully to a content hash; the agent implementing it measured that this toolchain's determinism makes a content-hash-only fingerprint blind to a fully-round-tripped transient mutation -- the exact scenario the file exists to catch -- and shipped the combined signal (content hash AND nanosecond mtime) instead, reporting the departure explicitly rather than applying the instruction as given. See commit `cb57372`'s body for the full account.

Commits: `mutation_sandbox.sh` -- `6eabc9d` (initial), `3f4a463`, `37f442f`, `3c5516b` (three review rounds). `ctest_guarded.sh` -- `321227e` (initial), `6c6c941` (transient-mutation fix), `cb57372` (filter-scope fix), `d65a9cc` (the three-finding round above).

### Vendored dependencies: upstream bug reporting policy

**Rule: before opening a bug ticket against an upstream project, check its development branches (`dev`/`master`/`next`).** If the bug is already fixed there, do NOT open a ticket -- at most, comment on an existing one. Apply the fix locally as a disclosed patch, document it in the vendor directory's own `README.md` and in `CHANGELOG.md`, and wait for the next tagged release to drop the local patch.

**The concrete case behind this rule** (recorded factually and soberly, without sourness -- the maintainer is fully within their rights): we reported a real heap-use-after-free in miniaudio (`ma_resource_manager_data_buffer_node_acquire`, the synchronous-decode-failure path), caught live by this repo's own test suite under AddressSanitizer -- [mackron/miniaudio#1141](https://github.com/mackron/miniaudio/issues/1141). The ticket was explicitly transparent: it stated the `dev` branch already avoided the bug, and argued that the 0.11.25 release and `master` still crashed with nothing tracking it. The maintainer closed it in one line: *"Please do not open tickets for bugs that have already been fixed."* Being transparent about the state of `dev` did not offset that -- the ticket itself was the problem. **The recorded decision (the leader's) is to not insist and not reply**: pushing further spends goodwill with a dependency's maintainer for zero technical gain, since the fix already exists on `dev` regardless of what happens with the ticket.

**Operational side, for whoever touches this vendor directory next:** the one-line local patch lives at `glintfx/third_party/miniaudio/miniaudio.h:70939` (the `result == MA_SUCCESS &&` guard), documented in `glintfx/third_party/miniaudio/README.md`. It must be REMOVED on the next miniaudio version bump that already carries the `dev`-branch fix -- check that first, before re-applying it blindly on a re-vendor.

**What this rule does NOT cover:** the consumer-to-library flow (a GusWorld finding becomes a fast tagged patch release) keeps applying in full -- this rule is only about opening a ticket in a third-party repository, nothing else.

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

A constelação bigtech (definição dos agents, RACI, pipelines de release) é mantida no plugin [`bigtech_plugin`](https://github.com/petrinhu/bigtech_plugin).

### Ponteiros essenciais

- [`CLAUDE.md`](CLAUDE.md): convenções do projeto, idioma, autoridade do líder, ambiente (toolchain Fedora 44), glintfx como produto ativo.
- Gates locais pre-commit/pre-push (`GATE-PRECOMMIT`/TST-L1-PRECI, `.githooks/` -> `tools/precommit.sh`/`tools/preci.sh`): duas rotas de ativação, nem sempre `git config core.hooksPath .githooks` -- ver seção "Local gates (opt-in hooks): pre-commit and pre-push" acima e [`TESTES.md`](TESTES.md#tst-l1-preci).
- [`docs/embed-integration.md`](docs/embed-integration.md): contrato de integração do `UiLayer` para hosts (fonte de verdade para frame lifecycle, GL state, dp_ratio, base URL, data model, texturas) -- leia antes de mexer no caminho embed.
- [`TODO.md`](TODO.md): tabela de pendências (ondas, IDs, pré-requisitos) das duas camadas, a **INBOX** (descobertas não priorizadas) e o escopo planejado da **v2** (component library / Atomic Design, spec em `docs/superpowers/specs/2026-06-30-glintfx-v2-design.md`, branch `feat/v2-f2-components`, pausada).
- **Sistema de memória:** memórias tipadas em `~/.claude/projects/<slug>/memory/` (índice em `MEMORY.md`), autocarregadas por sessão. Não duplicar o que o repo já registra; registrar só o não óbvio.
- [`docs/adr/`](docs/adr/README.md): decisões de arquitetura (imutáveis quando `Accepted`; para mudar, escrever novo ADR que substitua). ADR-0006 (camadas), ADR-0007 (licença), ADR-0008 (embed/guest mode).

### Paridade de API: declare a assimetria, nunca deixe silenciosa (API-PARITY-GATE, W22)

**Regra: toda API pública nova declara paridade entre `App` e `UiLayer`, ou justifica a assimetria no próprio doc-comment, citando o contrato que a impõe.** A regra NÃO é "toda API tem que existir nas duas fachadas" -- assimetrias reais existem e são legítimas (a sobrecarga de offset/letterbox de `UiLayer::set_viewport` só faz sentido pra um host embed compondo numa sub-região; o `App` é dono da janela inteira, então não tem a mesma necessidade). A regra é que a assimetria seja **declarada**, nunca **silenciosa**.

**Por que isto é um gate e não uma preferência de estilo:** o `GLPROC-EMBED` e o `FRAMEGRAB-EMBED` foram o *mesmo defeito de processo* duas vezes na mesma semana -- uma capacidade lançada, anunciada como o que "destrava" um consumidor embed, que na verdade não alcançava builds embed nenhum (`gl_proc_address()` era compilado fora sob `GLINTFX_BACKEND_GLFW=OFF`; `App::capture_frame()` saiu só-`App` enquanto o próprio consumidor que a pediu era embed-only). Os dois só foram pegos porque um consumidor tentou usar a feature e bateu numa parede, não porque algo no código ou nos docs dissesse "isto existe numa fachada, não na outra, e eis o porquê." Um doc-comment que declara a assimetria de saída transforma essa mesma lacuna em algo que um revisor -- ou o próprio autor da API, relendo uma semana depois -- pega por inspeção, não por um consumidor bloqueado.

**Como "declarado" se parece na prática, as duas formas válidas:**
- **Simétrico:** tanto `App::foo()` quanto `UiLayer::foo()` existem, com o mesmo contrato (ver o próprio `capture_frame()`, `FRAMEGRAB-TEX`/`FRAMEGRAB-EMBED`, que saiu só-`App` primeiro e foi deliberadamente estendido pro `UiLayer` assim que a lacuna foi nomeada).
- **Assimétrico, justificado:** só uma fachada, com o doc-comment declarando POR QUE a outra não precisa ou não pode ter -- ver o próprio comentário de topo de [`glintfx/include/glintfx/frame_capture.hpp`](glintfx/include/glintfx/frame_capture.hpp) (`CAPTURE-FREE`, W22 S8) pra um exemplo trabalhado: ele explica, no próprio header, por que o novo `capture_framebuffer()` sem instância deliberadamente NÃO reusa `App::CapturedFrame` (nem existe num build embed-only) nem `UiLayer::CapturedFrame` (amarraria uma função sem instância à classe baseada em RmlUi, contradizendo o próprio motivo dela existir) -- um terceiro tipo, independente, com a assimetria das duas irmãs mais próximas nomeada e justificada, não duplicada em silêncio.

**Uma função no estilo `gl_proc_address()`, conceitualmente de escopo de processo, não por-fachada, é uma TERCEIRA forma legítima** (ver a própria nota "POR QUE UMA FUNÇÃO LIVRE" de `gl_proc.hpp`) -- a pergunta de paridade pra essas é "funciona nos DOIS builds, `GLINTFX_BACKEND_GLFW=ON` e `OFF`", não "existe em `App` e `UiLayer`".

**A imposição hoje é por review, não por script** -- some isto a qualquer roteiro de review adversarial que você já esteja rodando contra um header público novo (`TESTES.md`/`AUDITORIAS.md`, ou um brief de review ad hoc): antes de aprovar um símbolo novo sob `glintfx/include/glintfx/`, pergunte "a fachada irmã tem isto, e se não tem, o doc-comment diz por quê?" Uma resposta ausente é a mesma classe de lacuna que o `GLPROC-EMBED`/`FRAMEGRAB-EMBED` foram os dois.

### Hospedagem: o GitHub é o único rosto público

**Decisão do líder, 2026-07-25, valendo até ele dizer o contrário: o projeto vive só no GitHub.** O Codeberg anunciou que não aceita mais código gerado por LLM/AI, e este repositório é escrito majoritariamente por agentes, então o líder está arquivando o repositório do Codeberg ele mesmo.

O que isso significa na prática:

- `https://github.com/petrinhu/glintfx` é a URL canônica do repositório, das releases, das issues e da wiki. Todo link `codeberg.org` é um 404 futuro e não pode ser reintroduzido.
- Os remotos git `origin` e `github` apontam os dois pro GitHub. Não existe mais "push dual": um push, um remoto.
- **O diretório `.forgejo/` inteiro sumiu**, apagado em 2026-07-25: tanto os dois workflows do Codeberg (`ci.yml`, `core-ci.yml`, `runs-on: codeberg-medium`) quanto o `heavy.yml` (o runner local `claudio`). Não recriar.
- A prosa histórica do `CHANGELOG.md` continua descrevendo o que releases passadas de fato fizeram, menções a remoto dual incluídas. Isso é registro, não instrução: não reescrever.

### Política de CI: o que roda onde

**Todo CI roda no GitHub. O `.github/workflows/` é o único CI que existe.** (Decisão do líder, 2026-07-25.)

- **`.github/workflows/ci.yml`** (`ubuntu-latest`) é o **gate**. Roda em push para `main`, em tags e em PRs; matriz sobre `GLINTFX_BACKEND_GLFW` (ON/OFF). Um run verde aqui é o que autoriza merge ou tag.
- **`.github/workflows/nightly.yml`** (cron do GitHub) é a **rede de segurança** noturna de ASan, independente do gate de PR/tag.
- **O runner Forgejo `claudio` acabou.** Foi desligado e removido em 2026-07-25, e o `.forgejo/` não existe mais no repositório. `forgejo-runner exec` não se aplica aqui: não sobrou workflow Forgejo pra validar com ele.

**`.github/workflows/heavy.yml`** (self-hosted, conteinerizado, efêmero) carrega as duas pernas pesadas que rodavam no `claudio`: `sanitize` (ASan/LSan/UBSan) e `fonteng` (motor de fonte próprio, `GLINTFX_OWN_FONT_ENGINE=ON`), cada uma numa matrix `backend: [ON, OFF]` -- quatro jobs no total. O runner é o `tools/ci/Containerfile.runner` (construído em cima da `glintfx-ci:f42`), supervisionado por uma unit `systemd --user` (`glintfx-heavy-runner`) que reregistra um container efêmero novo depois de cada job; teto rígido de `--memory=8g --memory-swap=8g` (swap desligado) e `--cpus=4` (`-j2` literal nos builds, nunca `$(nproc)`), mais `--cap-drop=ALL` e `--security-opt=no-new-privileges`. O gatilho é deliberadamente estreito -- só `push` na `main` (com filtro de caminho) e `workflow_dispatch`, **zero `pull_request`** -- porque é um runner self-hosted em repositório público, e o GitHub avisa que sem essa restrição o PR de um fork rodaria código arbitrário nesta máquina. Provado ponta a ponta mais de uma vez, mais recentemente na run `30368396354` (`workflow_dispatch`, 2026-07-28) com os quatro jobs verdes. A receita Forgejo original de onde foi portado segue arquivada em `~/.claude/receitas-ci/glintfx-heavy-claudio-forgejo.yml`; trilha de auditoria nos itens `GH-ONLY-RUNNER`/`AUD-CI-RUNNER` do `TODO.md`.

**SEC-CI-HARDEN (2026-07-29, remediação da `AUD-CI-RUNNER`, 2 dos 3 achados IMPORTANTE fechados):** a fronteira anti-fork descrita acima era imposta só pelo comentário de cabeçalho no bloco `on:` do `heavy.yml` -- nada falhava se uma edição futura reintroduzisse `pull_request` perto do label `self-hosted`. O job `lint-and-scan` de `.github/workflows/ci.yml` agora roda `tools/check_workflow_self_hosted_gate.py` em todo push/PR (GitHub-hosted, nunca no próprio label self-hosted, sem segredo no escopo): ele parseia todo `.github/workflows/*.yml` com PyYAML e falha se algum job cujo `runs-on` contenha `self-hosted` também estiver sob um workflow que dispara em `pull_request`/`pull_request_target`/`issue_comment`. É deliberadamente estrutural, não textual -- um `grep pull_request heavy.yml` cru casa as quatro linhas de comentário explicativo daquele arquivo e daria uma falsa sensação de cobertura. Separadamente, as quatro linhas `uses:` do `heavy.yml` (`actions/checkout`, `actions/cache`, nos dois jobs) agora estão fixadas por SHA de commit completo com a versão em comentário à direita, não mais na tag mutável `@v4` -- action de terceiro não pinada num runner self-hosted é um risco materialmente diferente da mesma action numa VM descartável GitHub-hosted (o nível de risco em que os `@v4` ainda não-pinados do próprio `ci.yml` ficam deliberadamente). O terceiro achado, `IMP-3` (sem restrição de egress de rede no container do runner -- risco de pivô de LAN, só importa se os dois primeiros forem derrotados juntos), está rastreado na INBOX como `SEC-CI-EGRESS`, residual aceito, não consertado aqui.

Duas práticas sobrevivem à mudança, e as duas continuam importando:

- **Reproduzir falha de container localmente, à mão, antes de iterar por push.** O `tools/ci/Containerfile.f42` e o `tools/ci/build_image.sh` continuam funcionando sozinhos: constroem a `localhost/glintfx-ci:f42` (fedora:42 com clang, cmake, `libasan`/`libubsan`, Xvfb e Mesa pré-instalados) com um `docker build` comum, sem runner nenhum envolvido. Rode o comando que falha dentro de um `docker run` contra essa imagem. Foi essa imagem que pegou dois achados reais que só reproduziam numa base Fedora (`libasan`/`libubsan` faltando, e um gap de `SYS_futex` sob o runtime de sanitizer do clang).
- **Rodar o `ctest` de pre-commit localmente** (`tools/preci.sh`, ver a seção de gates locais acima) em vez de usar o CI como primeiro compilador. O job pesado só roda em push pra `main`, então isto continua sendo a principal coisa entre você e uma regressão antes desse ponto.

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
- **Não existe mais workflow Forgejo nenhum, nem runner nesta máquina.** O `.forgejo/` inteiro (`ci.yml`, `core-ci.yml` do Codeberg e o `heavy.yml` do `claudio`) foi apagado em 2026-07-25, e o próprio `forgejo-runner` foi desinstalado da máquina (serviço, pacote RPM e binário) -- **`forgejo-runner exec` não roda mais aqui, não tente**. Ver "Hospedagem: o GitHub é o único rosto público" acima; não recriar nada disso. **O que sobreviveu e ainda serve:** a imagem `localhost/glintfx-ci:f42` (`tools/ci/Containerfile.f42` + `tools/ci/build_image.sh`, base `fedora:42` com as deps de sistema incluindo os pacotes `-dev` de GL/EGL/xkbcommon) continua sendo o jeito de reproduzir os checks pesados **localmente e à mão**, via `docker run` -- sem runner no meio. A receita das pernas `sanitize`/`fonteng` já foi portada ao GitHub -- roda hoje em `.github/workflows/heavy.yml`, no runner self-hosted conteinerizado (ver "Política de CI: o que roda onde" acima); o original que serviu de base segue arquivado fora do repo em `~/.claude/receitas-ci/glintfx-heavy-claudio-forgejo.yml`.

### Onde rodar teste de janela/input (canônico, follow-up do DOC-HOSTIN)

**ATENÇÃO, o ponto mais importante desta seção (mais que a tabela abaixo): subir o compositor aninhado NÃO basta. `-u WAYLAND_DISPLAY` não protege.** Isto não é teórico -- aconteceu de verdade neste repositório, em tempo real, enquanto esta própria seção estava sendo escrita: um `qa-engineer` subiu o KWin aninhado corretamente, PROVOU que o display aninhado era o `:1`, separado do `:0` do líder, e mesmo assim a sonda dele abriu uma janela real na sessão VIVA do líder por 2 a 3 minutos.

- **Por quê:** o `wl_display_connect(NULL)` do libwayland cai no nome EMBUTIDO `"wayland-0"` no instante em que a env var some, e resolve esse nome DENTRO do `$XDG_RUNTIME_DIR` -- que continua sendo o `/run/user/1000` da sessão real, a menos que ele também tenha sido trocado. Remover `WAYLAND_DISPLAY` só tira o *override*, não impede a conexão.
- **A proteção real é SÓ do APP sob teste, não do compositor aninhado.** O compositor aninhado precisa HERDAR o env normal de propósito (precisa achar o `wayland-0` real pra se aninhar dentro dele); é o app sob teste que recebe um `XDG_RUNTIME_DIR` isolado (`chmod 700`), de forma que não exista nenhum socket `wayland-0` dentro do próprio runtime dir dele pro fallback encontrar.
- **A invocação validada (esta de fato rodou, corrigida depois que um primeiro rascunho errou):**
  ```sh
  mkdir -p /var/tmp/glx-xdgrun && chmod 700 /var/tmp/glx-xdgrun
  # compositor aninhado: HERDA o env normal de propósito (precisa do wayland-0 real pra se aninhar)
  # nota: `--windowed` NÃO existe (confirmado por `kwin_wayland --help`) -- o modo em janela É o
  # default no instante em que se passa --wayland-display/--x11-display; `--virtual` é o headless
  kwin_wayland --wayland-display wayland-0 --width 640 --height 480 --xwayland --socket <nome-unico>
  # descobrir o novo :N em /tmp/.X11-unix/ (X0 = sessão do líder; o novo = o aninhado)
  # app sob teste: AQUI sim vai o env isolado
  env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 XDG_RUNTIME_DIR=/var/tmp/glx-xdgrun DISPLAY=:N <binário>
  # provar ANTES de interagir com qualquer coisa:
  lsof -p <pid> | grep -c libwayland   # tem que ser 0
  ss -xp | grep <inode do fd>          # tem que apontar pra @/tmp/.X11-unix/X<N>, nunca X0/wayland-0
  ```
- **`GLFW_PLATFORM` não é env var** -- é a constante de `glfwInitHint()` (que existe e funciona, chamada do código da aplicação, não do shell). A env var que o GLFW de fato honra na auto-seleção de backend é **`XDG_SESSION_TYPE=x11`** (com `WAYLAND_DISPLAY` removida), confirmado isoladamente via `glfwGetPlatform()`. Vale registrar também, porque poupa a próxima pessoa de um desvio: **o `App`/`WindowGlfw` deste repo NÃO chama `glfwInitHint` nem nada que selecione plataforma** (grep vazio em `glintfx/src`/`glintfx/include`) -- então isto é armadilha de invocação, não achado de produto.
- **Provar, não presumir:** `lsof -p <pid>` ou `/proc/<pid>/fd` mostrando o socket dentro do diretório isolado -- ANTES de lançar e DE NOVO depois de o app subir. Se a prova não fechar limpa, MATAR o processo na hora; não ajustar nada no escuro.
- **A lição mãe, que vale além deste caso:** a receita canônica da própria casa já tinha o antídoto -- a linha padrão de ctest é `env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ...` (ex.: `docs/superpowers/plans/2026-07-19-framework2d-A1-input.md:231`), e é a **troca do `XDG_RUNTIME_DIR` que faz o trabalho de verdade, não o `-u`**. Foi copiada pela metade. **Quando já existe invocação canônica no projeto, copie-a INTEIRA e só então adapte** -- cada pedaço está lá porque alguém já se queimou.

Registro factual, mantido sóbrio (é doc de engenharia, não bronca): o incidente foi CONTIDO -- nenhum input foi injetado no `:0`, só buscas read-only, e foi justamente uma dessas buscas que revelou o vazamento; o processo foi morto e a limpeza confirmada. O valor de documentar isto é a armadilha, não quem caiu nela.

| | Xvfb (o do CI) | Xephyr | KWin aninhado (o da casa) |
| :--- | :--- | :--- | :--- |
| Tela | fantasma, só memória | janela real na sessão | janela real na sessão |
| Window manager | **nenhum** | **nenhum** (é só o servidor X) | **sim, completo** |
| Botão X / alt-tab / minimizar | não existem | não existem sozinhos | existem |
| Instalado nesta máquina | sim | **não** | **sim** (`/usr/bin/kwin_wayland`) |

**Escolha canônica: `kwin_wayland --wayland-display wayland-0 --xwayland` (em janela é o default; ver a invocação validada acima -- `--windowed` não é uma flag real).** Motivos, nessa ordem: zero instalação (o Xephyr exigiria dois pacotes, porque **não** é window manager, é só a tela) e é o **mesmo compositor da sessão do líder**, logo mais fiel ao ambiente real do que um WM minimalista que ninguém usa.

**O que só o aninhado prova.** Sem window manager não há barra de título, logo não há clique real no botão X, alt-tab nem minimizar -- então o veto de close (`set_close_request_callback`), o focus e o iconify de janela só têm cobertura ponta-a-ponta lá. Sob Xvfb o máximo honesto é testar o *seam* puro de decisão (ex.: `glfw_decide_window_close()`, `glintfx/src/glfw_event_translate.hpp:614` -- cite o `arquivo:linha` real, e o `tools/check_doc_line_refs.sh` agora existe justamente pra confirmar que essa citação não apodreceu) e **declarar isso como downgrade**, nunca vender como e2e.

`xdotool`/XTest **funciona sob Xvfb** para tecla e mouse físicos (foi assim que a reentrância de callback foi provada com uma tecla real na Onda 2, `HOSTIN-2`); o que de fato falta no Xvfb é só o window manager.

**O que NÃO funcionou na perna do compositor aninhado (KWin aninhado sobre Wayland + o Xwayland interno dele): injeção de teclado via `xdotool`/XTest é FLAKY exatamente nesta pilha.** A entrega é intermitente mesmo com foco X11 confirmado, e houve um caso de toggle duplo numa única chamada (suspeita de auto-repeat do X11, não confirmada). O que FUNCIONOU de forma confiável ali: ações de window manager (`windowminimize`, `windowactivate`) e um `ClientMessage` ICCCM (`WM_DELETE_WINDOW` via `python-xlib`) para o e2e do veto de close. A recomendação registrada para uma próxima rodada de injeção de teclado é `uinput` bruto, que faz bypass do X11/XTest inteiro.

**Regra dura: nunca na sessão viva do líder.** A injeção de input mira **sempre** o display aninhado, **nunca** o `:0`. O motivo está registrado, não é hipotético: uma rajada de troca de modo de janela já travou o touchpad dele até precisar de reboot (ver a memória `feedback_nunca_stress_janela_sessao_viva`).

**Agente não instala pacote de sistema por conta própria** -- pede autorização primeiro. Reportar "não executado, precisa instalar `<pacote>`" é um resultado negativo honesto, e melhor do que improvisar em cima de uma ferramenta ausente.

### Sandbox de mutation testing: sabote uma cópia, nunca o build dir compartilhado (BUILDDIR-MUTACAO, W23)

**O incidente que isto existe pra prevenir (W22, `capture_framebuffer_smoke`, 2026-07-30):** dois agentes compartilharam esta working tree. Um fazia mutation testing contra o fix `GL_PACK_ALIGNMENT=1` do `CAPTURE-FREE` -- um fix real, num arquivo RASTREADO -- e rodou sete ciclos de rebuild+execução. Um segundo agente, numa fatia sem relação, rodou a suíte completa de `ctest` no meio dessa janela e viu `capture_framebuffer_smoke` morrer com `double free or corruption`. O orquestrador segurou um commit sem relação por causa disso e, na primeira leitura, acusou o agente errado. A causa real só foi provada por fonte independente -- carimbos do `coredumpctl`, não o relato de nenhum dos dois agentes: 21 coredumps de `capture_framebuffer_smoke`, todos dentro de uma janela fechada de 6 minutos, zero fora dela, batendo com a contagem de ciclos e a mistura de tipo de crash que o próprio agente que mutava já tinha relatado.

**É a terceira variante de contaminação de árvore compartilhada descoberta nesta onda, em ordem de sutileza:** (1) o ÍNDICE -- um `git add` colidindo arrasta arquivo alheio; (2) a WORKING TREE -- ler sabotagem deliberada de colega como defeito real; (3) o ARTEFATO DE BUILD -- a que esta seção é sobre. `git commit --only`/`git diff --cached` não protegem contra nenhuma das duas primeiras, e não protegem nada contra a terceira: quando um agente roda o `ctest`, o binário que o outro está mutando no meio já está compilado em disco.

**A consequência que quase passou batido, mais importante que qualquer uma das duas ferramentas abaixo: qualquer resultado de suíte medido dentro de uma janela contaminada é INVÁLIDO -- inclusive os que passaram VERDES.** Um teste verde pode ter executado contra um binário que outro agente mutou e reconstruiu instantes antes; um resultado verde sob contaminação não prova nada. O reflexo natural é investigar só o teste vermelho; o correto é desconfiar da rodada inteira e re-rodar limpa.

**`tools/mutation_sandbox.sh`** (`create`/`run`/`destroy`) dá a cada sessão de mutation testing uma árvore de build isolada e autocontida em `/var/tmp`, materializada via `git archive` a partir de um sha COMMITADO -- nunca a working tree -- então um revisor sabota uma cópia, nunca um arquivo rastreado que um colega possa estar lendo no mesmo instante:

```sh
SBOX="$(tools/mutation_sandbox.sh create HEAD --name my-drill)"   # git archive + configure + build
tools/mutation_sandbox.sh run "${SBOX}" -R render_sanity          # rebuild incremental + ctest (isolado sob Xvfb)
tools/mutation_sandbox.sh destroy "${SBOX}"                       # rm -rf, só se o basename casar glx-mutsbx-*
```

Verificado ao vivo enquanto esta seção era escrita (`glx-mutsbx-docverify.<pid>`, a partir de `HEAD`): `create` arquivou, configurou e buildou a glintfx limpo; `run -R render_sanity` reconstruiu incrementalmente e passou 5/5; `destroy` removeu o sandbox. As três recusas obrigatórias também foram exercitadas ao vivo: `run` contra um caminho dentro do próprio `glintfx/` deste repo recusa (`"is inside a git working tree"`); `run` contra `/tmp/...` recusa (`"/tmp is tmpfs (RAM) on this machine"`); `create` contra um sha inexistente recusa (`"does not resolve to a commit"`); e `destroy` contra um diretório real cujo basename não parece um sandbox recusa sem apagá-lo (tem que casar `glx-mutsbx-*`).

**`tools/ctest_guarded.sh`** embrulha um `cmake --build` + `ctest` -- mesmo isolamento Xvfb/`XDG_RUNTIME_DIR` que `run_layer1_config()` do `tools/preci.sh` já usa, copiado inteiro -- com uma checagem antes/depois: um snapshot do estado rastreado+staged+unstaged de `glintfx/` (excluindo `build*/`), E um fingerprint dos executáveis de teste de fato que o `ctest` está pra invocar (sha256 de conteúdo + mtime em nanossegundo, os dois sinais combinados -- ver o porquê abaixo). Qualquer um dos dois eixos divergindo imprime a mensagem canônica `RODADA INVALIDA (BUILDDIR-MUTACAO)` em stderr e sai não-zero, independente do que o próprio `ctest` tenha reportado:

```sh
tools/ctest_guarded.sh glintfx/build-preci -R render_sanity
```

Verificado ao vivo contra a `glintfx/build-preci` real: uma rodada limpa buildou, rodou 5/5 testes, e imprimiu `OK (tracked tree AND test-executable fingerprints unchanged during build+test)`; uma rodada com seleção vazia (`-R nome_que_nao_existe_xyz`) recusou com `matched ZERO tests ... refusing to fingerprint nothing`, em vez de reportar em silêncio uma rodada guardada válida sobre nada; uma rodada contra um diretório não configurado recusou com mensagem `CMakeCache.txt not found`. **Não reverificado nesta fatia de doc:** o caminho de contaminação de fato (um rebuild concorrente genuíno no meio da rodada) -- reproduzi-lo exigiria sabotar um segundo build dir real enquanto este roda, exatamente o risco que esta ferramenta existe pra tirar de uma árvore compartilhada. Esse comportamento está documentado a partir do próprio cabeçalho do script, que o registra como MEDIDO (não deduzido) contra um clone de drill descartável durante o próprio review adversarial do `BUILDDIR-MUTACAO` -- ver os commits ao fim desta seção.

**Por que hash de conteúdo E mtime em nanossegundo, não hash de conteúdo sozinho:** esta toolchain compila deterministicamente (medido ao vivo durante o review: um `touch` sem alteração de conteúdo ainda reproduz um executável byte-idêntico). Um ciclo mutar/rebuildar/restaurar/rebuildar que faz round-trip completo de volta ao fonte original portanto também faz round-trip de volta ao CONTEÚDO original -- um fingerprint só de hash de conteúdo não veria diferença nenhuma exatamente no cenário que o `BUILDDIR-MUTACAO` existe pra pegar. mtime em resolução de segundo sozinho também colide: dois rebuilds genuínos foram medidos completando em ~1,16s no total, folgadamente dentro de um segundo de relógio. Combinar mtime de nanossegundo com hash de conteúdo fecha os dois pontos cegos de uma vez, cada um cobrindo onde o outro é fraco.

**Limites declarados -- ler antes de confiar em qualquer uma das duas ferramentas além do que ela cobre:**
- O guard de árvore do `ctest_guarded.sh` vigia só `glintfx/` (`GUARD_PATHS`, excluindo `build*/`) -- **a Camada 0 (`src/`, `include/` na raiz do repo) NÃO é coberta.** Uma mutação concorrente lá é invisível a este guard.
- O fingerprint de executáveis cobre exatamente os binários que a seleção `-R`/`-E`/`-L` DAQUELA invocação está pra exercitar, não o conjunto de teste inteiro do build dir -- uma rodada filtrada (ex.: `-R render_sanity`) não fingerprinta, e portanto não protege, um binário sem relação reconstruído concorrentemente fora desse filtro.
- Nenhuma das duas ferramentas enxerga mutação de arquivo UNTRACKED (nunca `git add`ado) -- a mesma troca que a disciplina de mutation testing desta casa já aceita em outro lugar (sabotar um blob commitado copiado pra fora da árvore, nunca um untracked in-place).

**Três camadas de review adversarial, cada uma pegando o que a anterior não viu** -- a mesma forma que o próprio incidente da W22 teve, vale nomear em vez de esconder atrás de um diff final limpo. O guard de `mutation_sandbox.sh` contra um alvo de sandbox dentro de qualquer repo passou por três rodadas até ficar hermético: rodada 1 (`6eabc9d`) comparava o alvo contra a PRÓPRIA localização do script -- o eixo errado, já que copiar o script pra fora da árvore pra revisá-lo (exatamente como esta casa verifica um blob commitado) quebrava o guard em silêncio; rodada 2 (`3f4a463`, `37f442f`) consertou o eixo mas ainda chamava `git rev-parse --show-toplevel` pra descoberta, que um `GIT_DIR` envenenado consegue fazer falhar aberta -- mesmo de dentro de um repo real; rodada 3 (`3c5516b`) achou que a subida de filesystem à mão que substituiu aquilo era ELA MESMA fail-open num ancestral ilegível, exatamente a forma "falha de observação lida como ausência positiva" do bug da rodada 2, via um mecanismo diferente. O `ctest_guarded.sh` passou por um arco equivalente: a primeira versão (`321227e`) tirava snapshot só da árvore, cega a uma mutação aplicada e revertida dentro da janela guardada -- a mesma forma dos 7 ciclos do incidente da W22; o conserto disso (`6c6c941`) saiu com um buraco de lista-de-testes-vazia; um follow-up (`cb57372`) fechou um buraco de escopo de filtro; e uma rodada seguinte (`d65a9cc`) achou e consertou mais três achados juntos (a recusa de lista vazia, a combinação mtime-resolução/hash-de-conteúdo, e o mesmo bug de âncora de localização que o `mutation_sandbox.sh` já tinha batido) -- assim que alguém foi procurar de propósito.

**Pelo menos uma dessas rodadas corrigiu uma instrução explícita com uma medição, e entregou a correção em vez da instrução literal.** O conserto proposto pra colisão de mtime era trocar o fingerprint inteiramente pra hash de conteúdo; o agente que implementava mediu que o determinismo desta toolchain torna um fingerprint só de hash de conteúdo cego a uma mutação transitória com round-trip completo -- o cenário exato que o arquivo existe pra pegar -- e entregou o sinal combinado (hash de conteúdo E mtime de nanossegundo) em vez disso, reportando o desvio explicitamente em vez de aplicar a instrução como veio. Ver o corpo do commit `cb57372` pro relato completo.

Commits: `mutation_sandbox.sh` -- `6eabc9d` (inicial), `3f4a463`, `37f442f`, `3c5516b` (três rodadas de review). `ctest_guarded.sh` -- `321227e` (inicial), `6c6c941` (conserto da mutação transitória), `cb57372` (conserto do escopo de filtro), `d65a9cc` (a rodada de três achados acima).

### Dependências vendorizadas: política de reporte de bug upstream

**Regra: antes de abrir issue de bug num upstream, checar os branches de desenvolvimento (`dev`/`master`/`next`).** Se o bug já estiver corrigido lá, NÃO abrir issue -- no máximo comentar numa issue existente. Aplicar o fix localmente como patch divulgado, documentar no `README.md` do próprio diretório do vendor e no `CHANGELOG.md`, e esperar o próximo release taggeado pra descartar o patch local.

**O caso concreto por trás desta regra** (registrado factual e sóbrio, sem azedume -- o mantenedor está plenamente no direito dele): reportamos um heap-use-after-free real no miniaudio (`ma_resource_manager_data_buffer_node_acquire`, caminho de falha de decode síncrono), capturado ao vivo pela suíte de testes deste repositório sob AddressSanitizer -- [mackron/miniaudio#1141](https://github.com/mackron/miniaudio/issues/1141). A issue foi explicitamente transparente: dizia que o branch `dev` já evitava o bug, e argumentava que a release 0.11.25 e o `master` ainda crashavam sem nada rastreando isso. O mantenedor fechou em uma linha: *"Please do not open tickets for bugs that have already been fixed."* A transparência sobre o estado do `dev` não compensou -- o próprio ticket foi o problema. **A decisão registrada (do líder) é não insistir e não responder**: insistir gasta capital com o mantenedor de uma dependência sem ganho técnico nenhum, já que o fix já existe no `dev` independente do destino do ticket.

**Lado operacional, pra quem for mexer neste diretório de vendor depois:** o patch local de uma linha vive em `glintfx/third_party/miniaudio/miniaudio.h:70939` (o guard `result == MA_SUCCESS &&`), documentado em `glintfx/third_party/miniaudio/README.md`. Ele DEVE SER REMOVIDO no próximo bump de versão do miniaudio que já traga o fix do branch `dev` -- checar isso primeiro, antes de reaplicar cegamente num re-vendor.

**O que esta regra NÃO cobre:** o fluxo consumidor↔lib (um achado do GusWorld vira patch taggeado rápido) segue valendo integralmente -- esta regra é só sobre abrir ticket num repositório de terceiro, nada além disso.
