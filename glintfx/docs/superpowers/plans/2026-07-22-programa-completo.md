# PROGRAMA-COMPLETO — INBOX drain + SVG images + 2D draw layer / esvaziamento da INBOX + imagens SVG + camada de desenho 2D

> EN first, PT below (house convention). Plan author: Caetano (CTO). Baseline: glintfx
> **v0.18.0** on `main`. Governing architecture: [ADR-0015](../../../../docs/adr/0015-framework-2d-atomized-architecture.md)
> (atomized 2D framework, ROI-first ordering). This is a PROGRAM roadmap (waves of slices),
> not a single-slice plan. Item ID for commits: `PROG-1` (per-slice IDs below).
> STATUS: ACTIVE — Wave 0 DECIDED by the leader on 2026-07-22 (section 6 is a closed record,
> do not reopen). Wave 1 briefs issued 2026-07-22.

---

## Part A — EN

### 0. Program shape and non-negotiable operating constraints

Three blocks, ordered into waves where **every wave closes something coherent and taggeable**
(the leader can stop after ANY wave with zero half-done work):

- **Block 1** — drain the entire INBOX (every item gets a disposition: do / verify-and-close /
  defer-with-trigger; nothing is silently ignored).
- **Block 2** — SVG image support (NEW; dependency decision belongs to the leader, section 6).
- **Block 3** — the 2D draw layer (the strategic gap: the public surface has zero
  sprite/quad/camera/tilemap/blit; a game today draws its world with raw GL).

Constraints that bind EVERY slice (recorded once, valid throughout):

1. Small workflow: **< 5 agents per slice**; implementer ≠ reviewer; adversarial review that
   EXECUTES (mutation testing, sanitizers); orchestrator re-verifies build + claims.
2. **One heavy build at a time** (RAM ~18 Gi, swap pressured); heavy/ASan builds in
   `/var/tmp` with `TMPDIR=/var/tmp` (never tmpfs `/tmp`).
3. Local static gate before ANY push: clang-tidy + cppcheck + gitleaks (`tools/preci.sh`).
4. **The `windows msvc build` job runs on PRs** — MSVC portability is a gate now: no raw
   `-Wall`/`-Wextra` (use the generator-expression guard), beware `min`/`max`
   (`NOMINMAX` is directory-scope but don't regress), beware `filesystem::path::c_str()`
   (`wchar_t` on Windows).
5. **Window/input stress runs in a NESTED compositor, never in the leader's live session.**
6. Push, merge to `main`, tags: only with explicit leader authorization in-context (or under an
   explicitly invoked autonomous mode, which allows tag-by-judgment at wave close, CI green).
7. Release hygiene: bump `glintfx/CMakeLists.txt` `project(VERSION)` before every tag
   (`docs/RELEASE_CHECKLIST.md`); commits cite the slice ID; TODO.md status touched in the
   same commit (`🔍` on delivery, `✅` only post-verification).

Version numbers below are indicative; actual tags are cut at wave close per checklist.

### 1. Inventory — Block 1: the INBOX, item by item (nothing skipped)

Legend: **DO** = scheduled in a wave here · **VERIFY** = stale/likely-closed, verify and strike ·
**DEFER** = stays in INBOX with explicit trigger (recommendation, leader confirms in section 6) ·
**EXT** = external trigger owns it (not our work to start).

#### 1.1 Actionable in this program (DO)

| ID | INBOX item | Effort | Risk | Wave |
|---|---|---|---|---|
| QW-GUARDLOG | Inconsistent guard logging (`set_dp_ratio` warns; `load(nullptr)`/`set_viewport` silent) → standardize `LT_WARNING` on all 3 | P | low | 1 |
| QW-RIPPLEAPP | Ripple silently inert in standalone `App` (no backdrop by design) → warning log + public doc note | P | low | 1 |
| QW-PROOF4 | `ripple_sanity` Proof 4 blind spot → adopt the non-zero sentinel-FBO idiom from `ripple_stress_sanity` | P | low | 1 |
| QW-IABS32 | Camada 0 `iabs32(INT32_MIN)` defensive guard (1 line + C1 test) | P | low | 1 |
| QW-XKB | `ci.yml`: add `libxkbcommon-dev` (parity with Forgejo workflow) | P | low | 1 |
| QW-EMDASH | Em-dashes in `ui_layer.hpp` doc-comments → `--` | P | low | 1 |
| QW-COLRDOC | Refine COLR bisection doc-comment (per-field vs per-record corruption; MINOR from A4-EMOJI S3) | P | low | 1 |
| QW-MKDEP | `-MMD`/`-MP` in the Camada 0 Makefile (header-dependency tracking) | P | low | 1 |
| QW-D1TEST | D1 follow-up tests: `memmove(dst==src)` / `memmove(n==0)` in `test_mem.c` (promised "next touch") | P | low | 1 |
| QW-FONT9DOC | FONTENG 9px: document the honest limit in `docs/` (own engine ~0.42 vs FT 0.85 x-height fidelity at 9px@dp1.0; recommend ≥10px or FreeType runtime flip) — the CODE fix stays deferred (see pits, 5.4) | P | low | 1 |
| GP-DEADZONE | Gamepad deadzone helper: free function `glintfx::apply_deadzone(x, y, threshold)` (radial), stateless, OUTSIDE `Gamepads`, raw path untouched (D9 stance preserved: opt-in) | P | low | 5 |
| GP-APPCFG | Gamepad `App` integration: `AppConfig::enable_gamepads` + `App::gamepads()` (auto-owned instance, `poll()` inside the frame step); manual path stays supported | P/M | low | 5 |
| GP-RUMBLE | Rumble/force-feedback `EV_FF`: fd `O_RDWR`, `EVIOCSFF`/`EVIOCRMFF`, `play_rumble(pad, strong, weak, duration_s)`; NEW permission surface (RW on device) → Narciso/CISO review in-slice; graceful degrade when read-only | M | med | 5 |
| WM-MULTIMON | Multi-monitor: enumeration API (`monitor_count()`, id/name/geometry, no third-party types) + `monitor_index` on `AppConfig`/`set_window_mode`; fail-high on invalid index | M | med | 6 |
| WM-EXTRAS | `resizable` (config+runtime), borderless-windowed, min/max size, mode-change callback (WM-initiated changes observable), vsync control | M | med | 6 |
| AS-STREAM | `AUDIO-STREAM`: `load_music()`/flag using `MA_SOUND_FLAG_STREAM` (decode-on-demand for long tracks); loop/fade/is_playing parity; NOT mixed into `play_oneshot` copies (streams are single-instance, documented) | P/M | low–med | 7 |
| CI-MSVCGLFW | MSVC job covers GLFW/`glintfx_window`: `vcpkg install glfw3` + drop `GLINTFX_BACKEND_GLFW=OFF` → 9/9 targets under MSVC | P/M | low (CI-only) | 7 |
| EM-CBDT | Emoji CBDT/CBLC (bitmap strikes → covers modern Noto): SFNT tables parsed in Camada 0 (`src/sfnt.c`, `glx_sfnt_cbdt_*`, hostile-hardened like `kern`/COLR), PNG payload decoded HOSTED-side via vendored stb (same C0/hosted split as COLRv0+CPAL), bake into the existing RGBA8 atlas; strike selection by nearest ppem | M/G | med | 9 (gated: leader go) |
| C0-PICVAR | `libcore_pic.a` (`-fpic` variant) + PIE consumer-harness leg | P/M | low | 10 (optional; recommend DEFER, see 5.5) |

#### 1.2 Stale — verify and strike (VERIFY)

| ID | Item | Why stale |
|---|---|---|
| VF-APPWHEEL | "App has NO `MouseWheel` path" seed | A1-INPUT (v0.12.0) delivered physical scroll capture (`glfw_event_translate.hpp` scroll translation with sign inversion) + `Engine::process_event` `MouseWheel` case shared by `App::process_event`. Task: prove by test (synthetic + translate unit already exist — extend `app_process_event_smoke` with a wheel event if uncovered), then strike from INBOX. Wave 1, effort P. |
| VF-WIDGETSCROLL | 2nd UBSan cause (`WidgetScroll`) unreproduced | Largely obsoleted by the root-cause RmlUi teardown patch (v0.9.1, suppressions REMOVED). Task: check latest `nightly-sanitize`/heavy runs for any vptr finding; if clean, strike. Wave 1, effort P. |

#### 1.3 Deferred with explicit trigger (DEFER — leader confirms in section 6)

| Item | Trigger that revives it | Rationale |
|---|---|---|
| Text-input forwarding (`process_text_input`, IME) | GusWorld starts SAVE-NOMEAR (their PI10) | The INBOX itself records the consumer's contract and warns: "do NOT spec in the air, it is retrabalho recipe". Spec only closes against the real field. **Recommend: keep gated even though the program says "whole INBOX" — this is the one item where doing it now is provably worse.** |
| `L1.14-GLLOADER-WIN` 🔍→✅ | Consumer bumps pin + revalidates their full cross-compile | Verification is theirs; our side shipped and was adversarially gated. |
| RmlUi #968 pin bump | Upstream merges | Then: bump pin, drop `PATCH_COMMAND`, notify GusWorld (standing combinado). |
| Codeberg `waiting` runner | Pattern is accepted policy (github > claudio > codeberg, 1 green suffices) | No work. |
| `SYS_futex` CXXFLAGS mitigation | Runner toolchain update | Watch-only; flag becomes inert then removable. |
| ~3px scroll jitter (upstream RmlUi) | A consumer reports confusion | Doc-only then. |
| GAP-2b (host FBO ≠ 0) | Real consumer demand | Hardcoded FBO 0 is upstream-bound; no demand. |
| AUD-PUB-6 remainder (unload/hide/show, multi-document, `load_font` runtime, cursor cb, clipboard cb) | 2nd real consumer | Explicit pull seeds; GusWorld reads SDL direct. |
| `image-tint` hard saturation cut | 2nd consumer with problematic gold | Threshold dial sufficed (ADR-0010 addendum). |
| UA-stylesheet redundant recompile | none (inherent, no payoff) | Recorded as accepted. |
| C0 seeds: harness stdin fixture, `mini_printf` short-write retry, `%g` | Real consumer need (each documented YAGNI) | Keep. |
| `AUD-TEC-6` breaking API consolidation | v1.0 program (leader-deferred) | Out of this program by prior leader decision. |
| FX-CARVE-2 | (i) lean-ui consumer with numeric footprint req; (ii) upstream minimal-build option | Decided-deferred, ADR-0015 addendum. |
| FONTENG 9px CODE fix (curve-top hinting) | A consumer complains about 9px specifically | Pit — see 5.4. Doc slice QW-FONT9DOC closes the INBOX presence honestly. |
| COLRv1 · ZWJ/GSUB shaping | See pits 5.2/5.3 | 10–20× efforts, deferred twice already; CBDT covers Noto raster need at a fraction of the cost. |
| `out.ppm` housekeeping | Leader one-liner (`gio trash out.ppm`) | Untracked; not agent work (tmpfs/deletion protections). |

### 2. Inventory — Block 2: SVG image support (leader decision required)

**Insertion point (technical recommendation, independent of the library choice):** the **image
atom** — SVG is "one more decode" in the texture-loading path (`BaseUrlFileInterface` →
decode → premultiply → GL upload), exactly where stb_image lives. That makes SVG work
automatically in `<img>`, in every decorator that takes an image, AND in future Draw2D
textures. First slice rasters at intrinsic SVG size × `dp_ratio` (crisp at declared size);
re-raster-on-element-resize is a recorded seed, not slice 1. No new module: a feature inside
`GLINTFX_MODULE_IMAGE` (same pattern as "emoji inside text", ADR-0015 (b)).

**Options for the dependency (the leader's call — house precedent: vendor with pin + NOTICE,
as stb_image/miniaudio/gamecontrollerdb):**

| Option | License | Size/shape | Coverage | Maintenance | Risk | Verdict |
|---|---|---|---|---|---|---|
| **(A) nanosvg** | zlib | 2 header files, tiny | paths, basic gradients; no text | low activity; known fuzz-fragility history in float/path parsing | hostile-input surface with weak upstream hardening → OUR corpus must carry it | acceptable minimum |
| **(B) lunasvg** (+plutovg) | MIT | mid-size C++ lib | static SVG incl. gradients, clips | active; it is what RmlUi's own SVG plugin uses | moderate | **RECOMMENDED** |
| **(C) RmlUi SVG plugin** (enables upstream plugin, feeds it lunasvg) | MIT | build-config + dep B | `<svg>` element in RML, re-rasters on size | upstream-owned | couples to RmlUi build; serves UI ONLY (not Draw2D textures) | optional LATER, on top of B (same vendored lib, two consumers) |
| **(D) clean-room SVG** | — | months of work | whatever we build | ours forever | bottomless pit (see 5.1) | REJECT now; long-term internalization clause applies as with the rest of the pile |

**Recommendation: (B) lunasvg, vendored + pinned + NOTICE entry, wired into the image atom;
record in a short ADR-0018 (dependency + boundary + insertion point). Option (C) can be added
later reusing the same vendored lib if RML-native `<svg>` elements get real demand.**
Hostile-input discipline regardless of option: SVG corpus tests (truncated, recursive,
billion-laughs-style entity/def abuse, huge dimensions vs the 256 MiB asset cap, malformed
floats), same class as `asset_decode_hostile_sanity`.

### 3. Inventory — Block 3: the 2D draw layer (`GLINTFX_MODULE_DRAW2D`)

**Verified fact:** the public surface has nothing to draw a game world. The declared goal of
ADR-0015 ("the single piece for a SIMPLE 2D game") does not close for a new adopter without it.

**Needs an ADR — yes, mandatorily:** ADR-0015 (b) states the module list changes only by a new
ADR. **ADR-0017** (new atom `GLINTFX_MODULE_DRAW2D`) written in collaborative mode (options to
the leader BEFORE recording, per `software-architect` house rule).

Architectural stance to propose in ADR-0017 (summary):

- **New atom** `GLINTFX_MODULE_DRAW2D`, depends on **core + image + gl-loader (internal)**.
  NOT on ui, NOT on window: it requires only a current GL context — provided by `App`
  (standalone, inside `set_frame_callback`, already guarded by `GlStateGuard`) or by the host
  (embed, same contract the GusWorld already honors drawing raw GL under the `UiLayer`).
  Draw order story: world via Draw2D first, UI composited on top — exactly today's contract.
- **Reuses the house renderer substrate**: own GL loader (`glx_`-prefixed, Windows-capable
  since v0.9.2), premultiplied-alpha convention, `GlStateGuard` discipline, FBO 0 composition.
  It does NOT reuse RmlUi's render interface (that is ui's substrate; Draw2D must not drag
  RmlUi in — a game with fx OFF and ui OFF can still draw sprites).
- **Public boundary with zero third-party types**: own `Texture2d` handle (loaded through the
  image atom, premultiplied), `Sprite`/draw params as plain structs, `Camera2d` as a value type.
- Batching by texture internally; screen-space pixel coords by default; camera provides
  world→screen.

**Slices (each taggeable, each M or G max — the pit is controlled by slicing, see 5.6):**

| Slice | Delivers | Out of scope (recorded seeds) | Effort | Risk |
|---|---|---|---|---|
| **D2D-0** (part of Wave 0) | ADR-0017 options + API sketch to the leader; decision recorded | — | P | low |
| **D2D-1** | Texture load (image atom → GL, premultiplied) + `draw_sprite(tex, dst, src_rect, tint)` + internal per-texture batching + works in `App` frame hook AND embed; pixel-readback tests under Xvfb (statistical, `render_sanity` class); MSVC-compiled (loader is win-capable) | camera, rotation, primitives | **G** | med — new GL path; mitigation: llvmpipe CI + adversarial pixel proofs |
| **D2D-2** | Transforms: origin/rotation/scale/flip + `Camera2d` (pos/zoom/rotation, `set_camera`, `screen_to_world` helper) | tilemap, text-in-world, z/layers | M | low–med (math + tests are pure/unit-able) |
| **D2D-3** | Draw-order control (explicit layers), untextured primitives (filled rect/quad, line), scissor, batching perf pass + **performance budget test** (`performance-engineer` class) | particles, shaders-on-sprites (fx stays ui-side) | M/G | med |
| Seeds (post-program, pull) | Tilemap/atlas helper (GusWorld pull), text-in-world (needs text-atom raster → texture bridge), sprite fx | — | — | — |

After D2D-2 the framework honestly claims "you can draw a simple 2D game with glintfx alone";
D2D-3 makes it comfortable and measured.

### 4. The waves (order + why + what each one tags)

Ordering criteria (leader-canonized): value early · easiest/best ROI first · dependencies
respected · **stop-anywhere** (every wave = coherent, taggeable close). Draw2D outranks the
capability leftovers because it is the declared strategic gap; quick wins ship first while the
Wave-0 decisions are with the leader. Waves 4–7 are mutually independent (permutable at the
leader's preference); the listed order is my ROI reading.

| Wave | Name | Content (slice IDs) | Closes/tags | Effort |
|---|---|---|---|---|
| **0** | Decisions (no code) | One AskUserQuestion batch: SVG option (section 2), Draw2D scope + ADR-0017 draft options (D2D-0), CBDT go/no-go, defer-list confirmation (1.3), C0-PICVAR stance; then ADR-0017 (+ADR-0018 if SVG=vendor) recorded | ADRs merged; nothing else | P |
| **1** | INBOX quick sweep | QW-* (11 items, all P, file-disjoint) + VF-APPWHEEL + VF-WIDGETSCROLL | INBOX visibly drained of every small item; tag **v0.18.1** (hygiene/patch) unless review upgrades it | P (batch) |
| **2** | Draw2D core | D2D-1 | **v0.19.0** — sprites on screen, standalone + embed | G |
| **3** | Draw2D camera | D2D-2 | **v0.20.0** — "simple 2D game" claim honest | M |
| **4** | SVG | SVG slice per Wave-0 decision (vendor + image-atom decode + hostile corpus + NOTICE/pin) | **v0.21.0** — `.svg` usable everywhere an image is | M |
| **5** | Gamepad finish | GP-DEADZONE + GP-APPCFG + GP-RUMBLE (CISO look on the RW surface) — input stress in nested compositor | **v0.22.0** — gamepad atom feature-complete for real pads | M |
| **6** | Window-modes finish | WM-MULTIMON + WM-EXTRAS — manual smoke in nested compositor, NEVER the live session | **v0.23.0** — window atom feature-complete | M |
| **7** | Audio stream + CI gap | AS-STREAM + CI-MSVCGLFW | **v0.24.0** — music without full-RAM cost; 9/9 MSVC targets | P/M |
| **8** | Draw2D comfort | D2D-3 | **v0.25.0** — layers, primitives, perf budget | M/G |
| **9** | Emoji Noto (gated) | EM-CBDT — only if Wave 0 said go | **v0.26.0** — modern Noto emoji renders | M/G |
| ~~10~~ | ~~Optional completeness~~ | **DROPPED — leader upheld the defer of C0-PICVAR (2026-07-22, section 6)** | — | — |
| **11** | Docs/wiki ressync (fixed end-of-table item, house rule) | `technical-writer`/`ux-writer`: GUIA_INICIANTE, wiki (Codeberg+GitHub), README/docs freshness across everything the program shipped | doc release | M |

### 5. Bottomless pits — named, with the honest number

1. **SVG clean-room (D):** the SVG spec (paths, transforms, gradients, clips, text, CSS subset,
   filters) is a multi-month renderer project on its own. **Vendor now (B); clean-room only
   under the long-term internalization clause, no date, like RmlUi/FreeType.**
2. **ZWJ/skin-tone/flags shaping:** requires GSUB (lookup parsing, ligature substitution,
   cmap14 + sequence logic) — a real shaping engine's first third. Deferred 2× already as
   10–20× the slice effort; that reading stands. **No cheap defensible slice exists** (a
   "ZWJ-only" hack still needs GSUB machinery). Recommend: DEFER with trigger (a consumer
   actually rendering flags/skin-tones), revisit as its own program.
3. **COLRv1:** paint graph + gradients + transforms = a mini vector rasterizer inside the font
   engine (G/GG). **The defensible move is EM-CBDT instead** — bitmap strikes cover the modern
   Noto need at a fraction of the cost; COLRv1 deferred with the same trigger discipline.
4. **FONTENG 9px x-height:** the spike PROVED it is not rounding — it is curve-top overshoot
   suppression, i.e., real outline hinting of round tops, the grid-fit class already rejected
   twice as expensive/risky. **No cheap fix exists.** QW-FONT9DOC (Wave 1) documents the limit
   and the workarounds (≥10px, or runtime FreeType flip which exists precisely for this);
   the code fix stays dormant with its trigger.
5. **C0-PICVAR:** cheap (P/M) but has **zero consumers** — ADR-0009 already recorded the
   accepted cost, and building a second archive variant against hypothetical demand is scope
   creep by our own canon (`feedback_cadencia_releases_consumidor`). **My recommendation as CTO:
   do NOT do it in this program**; it is in Wave 10 only as a leader override, and it is the one
   Block-1 item where I push back on "everything".
6. **Draw2D as a whole is G by nature** — controlled by slicing: D2D-1 is the only G slice, and
   it is still one coherent, taggeable capability (sprites). If D2D-1 slips, nothing else in the
   program is blocked (waves 4–7 are independent).
7. **Also flagged:** WM-EXTRAS is five small features wearing one coat — if review pressure
   grows, split `vsync` + `mode-change callback` into a follow-up patch tag rather than let the
   wave sprawl.

### 6. Wave-0 decisions — DECIDED BY THE LEADER 2026-07-22 (closed record, do not reopen)

1. **SVG = option (B), vendored lunasvg** (pin + NOTICE, inside the image atom; ADR-0018
   recorded in Wave 4). — **Leader decision, 2026-07-22.**
2. **Draw2D = D2D-1 + D2D-2** (sprites/quad + batching, then transform + `Camera2d`) — that is
   where the leader considers the framework goal met. **D2D-3 stays in Wave 8** as planned.
   **ADR-0017 confirmed as prerequisite of D2D-1.** — **Leader decision, 2026-07-22.**
3. **Deferrals CONFIRMED in full** (section 1.3), including the two the CTO contra-argued and
   the leader upheld: **text-input/IME deferred** (trigger: GusWorld starts SAVE-NOMEAR) and
   **C0-PICVAR will NOT be done** (Wave 10 dropped). Also confirmed: ZWJ/GSUB and COLRv1
   deferred; **EM-CBDT stays in Wave 9** (the move that covers modern Noto); FONTENG 9px
   closes by documentation (QW-FONT9DOC). — **Leader decision, 2026-07-22.**
4. **Wave order 4–7: keep as listed** (permutable, no leader preference expressed).
   — **Leader decision, 2026-07-22.**

---

## Part B — PT

### 0. Forma do programa e restrições operacionais inegociáveis

Três blocos, ordenados em ondas onde **toda onda fecha algo coeso e taggeável** (o líder pode
parar depois de QUALQUER onda sem trabalho pela metade):

- **Bloco 1** — esvaziar a INBOX inteira (todo item ganha disposição: fazer /
  verificar-e-baixar / adiar-com-gatilho; nada é ignorado em silêncio).
- **Bloco 2** — suporte a imagens SVG (NOVO; decisão de dependência é do líder, seção 6).
- **Bloco 3** — camada de desenho 2D (a lacuna estratégica: a superfície pública tem zero
  sprite/quad/câmera/tilemap/blit; hoje o jogo desenha o mundo com GL cru).

Restrições que valem pra TODA fatia:

1. Workflow pequeno: **< 5 agentes por fatia**; implementer ≠ reviewer; review adversarial que
   EXECUTA (mutation testing, sanitizers); orquestrador re-verifica build + claims.
2. **1 build pesado por vez** (RAM ~18 Gi, swap pressionado); builds pesados/ASan em
   `/var/tmp` com `TMPDIR=/var/tmp` (nunca no tmpfs `/tmp`).
3. Gate estático local antes de qualquer push: clang-tidy + cppcheck + gitleaks (`tools/preci.sh`).
4. **O job `windows msvc build` roda no PR** — portabilidade MSVC é gate: nada de `-Wall` cru
   (usar o guard de generator expression), cuidado com `min`/`max` (`NOMINMAX` já é
   directory-scope, não regredir) e com `filesystem::path::c_str()` (`wchar_t` no Windows).
5. **Stress de janela/input roda em compositor ANINHADO, nunca na sessão viva do líder.**
6. Push, merge em `main` e tag: só com autorização explícita do líder naquele contexto (ou sob
   modo autônomo explicitamente invocado — tag por julgamento ao fim da onda, CI verde).
7. Higiene de release: bump do `project(VERSION)` antes de toda tag
   (`docs/RELEASE_CHECKLIST.md`); commits citam o ID da fatia; status do TODO.md tocado no
   mesmo commit (`🔍` na entrega; `✅` só pós-verificação).

Números de versão abaixo são indicativos; a tag real sai no fechamento da onda pelo checklist.

### 1. Inventário — Bloco 1: a INBOX, item a item (nada pulado)

Legenda: **FAZER** = agendado numa onda · **VERIFICAR** = defasado/provavelmente fechado,
provar e baixar · **ADIAR** = fica na INBOX com gatilho explícito (recomendação; o líder
confirma na seção 6) · **EXT** = gatilho externo é o dono.

#### 1.1 Acionáveis neste programa (FAZER)

Mesma tabela da Parte A (IDs `QW-*`, `GP-*`, `WM-*`, `AS-STREAM`, `CI-MSVCGLFW`, `EM-CBDT`,
`C0-PICVAR`), com esforço P/M/G, risco e onda. Destaques:

- **Onda 1 (varredura rápida, tudo P, arquivos disjuntos):** logs dos guards padronizados
  (`LT_WARNING` nos 3), aviso+doc do ripple inerte em `App` standalone, reforço do Proof 4 com
  FBO sentinela, guarda do `iabs32`, `libxkbcommon-dev` no `ci.yml`, em-dashes do
  `ui_layer.hpp`, doc-comment da bisecção COLR, `-MMD`/`-MP` no Makefile da Camada 0, testes
  `memmove(dst==src)`/`n==0` prometidos no D1, e a documentação honesta do limite de 9px do
  motor de fonte (o FIX de código segue adiado — poço 5.4).
- **Onda 5 (gamepad fecha):** helper de deadzone (função livre opt-in, caminho cru intocado),
  integração `AppConfig::enable_gamepads`/`App::gamepads()`, e rumble `EV_FF` — superfície nova
  de permissão RW no device → olhar do Narciso/CISO dentro da fatia; degradação graciosa.
- **Onda 6 (window-modes fecha):** enumeração de monitores + `monitor_index` (fail-high em
  índice inválido) e o pacote resizable/borderless/min-max/callback de mudança de modo/vsync.
- **Onda 7:** streaming de áudio (`MA_SOUND_FLAG_STREAM`, single-instance documentado, sem
  mistura com `play_oneshot`) + cobertura GLFW no job MSVC (`vcpkg install glfw3`, 9/9 alvos).
- **Onda 9 (gated):** emoji **CBDT/CBLC** — parser das tabelas na Camada 0 zero-libc
  (`glx_sfnt_cbdt_*`, mesma postura hostil do `kern`/COLR), decode do PNG embutido no lado
  hosted via stb já vendorizado (mesmo split C0/hosted do COLRv0+CPAL), bake no atlas RGBA8.

#### 1.2 Defasados — verificar e baixar (VERIFICAR)

- **VF-APPWHEEL:** a semente "App sem caminho de `MouseWheel`" está DESATUALIZADA — o
  A1-INPUT (v0.12.0) entregou a captura física de scroll (`glfw_event_translate.hpp`, inversão
  de sinal) + o caso `MouseWheel` do `Engine::process_event` compartilhado pelo
  `App::process_event`. Provar por teste (estender `app_process_event_smoke` se faltar
  cobertura) e riscar. Onda 1, P.
- **VF-WIDGETSCROLL:** a 2ª causa UBSan foi em grande parte obsoletada pelo patch de raiz do
  teardown do RmlUi (v0.9.1, supressões REMOVIDAS). Checar os runs recentes de sanitizer; se
  limpo, riscar. Onda 1, P.

#### 1.3 Adiados com gatilho explícito (ADIAR — líder confirma na seção 6)

Mesma tabela da Parte A. Os que merecem destaque em pt:

- **Text-input/IME:** gatilho = GusWorld iniciar o SAVE-NOMEAR. A própria INBOX avisa: "NÃO
  especificar no ar, é receita de retrabalho" — a spec só fecha contra o campo real.
  **Recomendo manter gated mesmo com a ordem de "INBOX inteira": é o único item onde fazer
  agora é comprovadamente pior.**
- **EXT/watch sem trabalho nosso:** `L1.14-GLLOADER-WIN` (🔍 aguarda revalidação do
  consumidor), bump do pin quando o RmlUi #968 merjar (+ avisar o GusWorld), Codeberg
  `waiting` (política canônica cobre), `SYS_futex` (watch), jitter de 3px (upstream).
- **Sementes pull confirmadas:** GAP-2b, restante do AUD-PUB-6, saturação dura do
  `image-tint`, recompile da UA-stylesheet, sementes YAGNI da Camada 0 (stdin fixture,
  short-write, `%g`), `AUD-TEC-6` (candidato v1.0, decisão anterior do líder), FX-CARVE-2
  (adiada por ADR), fix de CÓDIGO do FONTENG 9px, COLRv1 e shaping ZWJ/GSUB (poços 5.2/5.3),
  `out.ppm` (one-liner do líder).

### 2. Inventário — Bloco 2: SVG (decisão do líder)

**Ponto de inserção (recomendação técnica, independe da lib):** o **átomo image** — SVG é
"mais um decode" no caminho de textura (`BaseUrlFileInterface` → decode → premultiply →
upload GL), onde o stb_image vive. Com isso o SVG funciona de graça em `<img>`, em todo
decorator que aceita imagem, E nas texturas do futuro Draw2D. Fatia 1 rasteriza no tamanho
intrínseco × `dp_ratio`; re-raster no resize do elemento é semente registrada. Sem módulo
novo: feature DENTRO de `GLINTFX_MODULE_IMAGE` (mesmo padrão "emoji dentro do text").

**Opções** (tabela completa na Parte A): **(A)** nanosvg (zlib, minúsculo, histórico frágil a
input hostil, atividade baixa) · **(B) lunasvg (MIT, ativo, é o que o plugin SVG oficial do
RmlUi usa) — RECOMENDADA** · **(C)** = B + habilitar o plugin `<svg>` do RmlUi depois (mesma
lib vendorizada, 2 consumidores) · **(D)** clean-room — REJEITAR agora (poço 5.1; cláusula de
internalização de longo prazo se aplica como ao resto). Qualquer opção: vendorizar com
pin + NOTICE (precedente da casa) + **ADR-0018** curto + corpus hostil de SVG (truncado,
recursão/defs abusivos, dimensões gigantes vs teto de 256 MiB, floats malformados), mesma
classe do `asset_decode_hostile_sanity`.

### 3. Inventário — Bloco 3: camada de desenho 2D (`GLINTFX_MODULE_DRAW2D`)

**Fato verificado:** não há NADA na superfície pública pra desenhar o jogo. A meta do ADR-0015
("a única peça pra um jogo 2D SIMPLES") não fecha pra um adotante novo sem isso.

**Precisa de ADR — sim, obrigatoriamente:** o ADR-0015 (b) diz que a lista de módulos só muda
por ADR novo. **ADR-0017** (átomo novo `GLINTFX_MODULE_DRAW2D`), em modo colaborativo (opções
ao líder ANTES de gravar).

Postura arquitetural a propor (resumo; detalhe na Parte A):

- Átomo novo dependendo de **core + image + gl-loader (interno)** — NÃO de ui, NÃO de window:
  só exige contexto GL corrente, dado pelo `App` (standalone, dentro do `set_frame_callback`,
  já guardado por `GlStateGuard`) ou pelo host (embed — mesmo contrato que o GusWorld já honra
  desenhando GL cru sob o `UiLayer`). Ordem de desenho: mundo via Draw2D, UI por cima.
- **Reusa o substrato da casa**: loader GL próprio (`glx_`, Windows-capaz desde a v0.9.2),
  premultiplied alpha, disciplina `GlStateGuard`, composição no FBO 0. NÃO arrasta o RmlUi
  (jogo com ui OFF ainda desenha sprite).
- **Fronteira pública sem tipo de terceiro**: `Texture2d` próprio (carregado pelo átomo image,
  premultiplicado), structs simples de sprite, `Camera2d` como value type. Batching por
  textura interno; coordenadas de tela em pixel por default; câmera dá mundo→tela.

**Fatias** (cada uma taggeável; o poço é controlado pelo fatiamento — 5.6):

- **D2D-0** (na Onda 0): opções de ADR/API ao líder. P.
- **D2D-1**: textura + `draw_sprite(tex, dst, src_rect, tint)` + batching + funciona em
  `App` E embed; provas por pixel readback sob Xvfb; compila no MSVC. **G**, risco médio.
- **D2D-2**: origin/rotação/escala/flip + `Camera2d` (pos/zoom/rotação, `screen_to_world`).
  M. Depois dela, a alegação "jogo 2D simples só com glintfx" fica honesta.
- **D2D-3**: layers/ordem explícita, primitivas sem textura (rect/quad/linha), scissor, passe
  de perf com **budget medido** (`performance-engineer`). M/G.
- **Sementes pós-programa (pull):** tilemap/atlas helper, texto-no-mundo (ponte
  text-atom→textura), fx de sprite.

### 4. As ondas (ordem + porquê + o que cada uma tagueia)

Critérios (canonizados pelo líder): valor cedo · mais-fácil/melhor-ROI primeiro · dependências
respeitadas · **parar-em-qualquer-onda** (toda onda fecha coeso e taggeável). O Draw2D passa na
frente das sobras de capability por ser a lacuna estratégica declarada; os quick wins saem
primeiro enquanto as decisões da Onda 0 estão com o líder. Ondas 4–7 são independentes entre
si (permutáveis a gosto do líder).

| Onda | Nome | Conteúdo | Fecha/tag | Esforço |
|---|---|---|---|---|
| **0** | Decisões (sem código) | 1 lote de AskUserQuestion: SVG, escopo Draw2D + opções do ADR-0017, go/no-go CBDT, confirmação da lista de adiamentos, postura do C0-PICVAR; grava ADR-0017 (+0018) | ADRs | P |
| **1** | Varredura INBOX | 11 `QW-*` + 2 `VF-*` (tudo P, disjunto) | **v0.18.1** | P |
| **2** | Draw2D núcleo | D2D-1 | **v0.19.0** — sprite na tela | G |
| **3** | Draw2D câmera | D2D-2 | **v0.20.0** — "jogo 2D simples" honesto | M |
| **4** | SVG | fatia conforme decisão | **v0.21.0** | M |
| **5** | Gamepad fecha | deadzone + App/cfg + rumble (CISO na superfície RW; stress em compositor aninhado) | **v0.22.0** | M |
| **6** | Window-modes fecha | multi-monitor + extras (smoke em compositor aninhado) | **v0.23.0** | M |
| **7** | Audio stream + CI | AS-STREAM + CI-MSVCGLFW | **v0.24.0** | P/M |
| **8** | Draw2D conforto | D2D-3 | **v0.25.0** | M/G |
| **9** | Emoji Noto (gated) | EM-CBDT (se o líder aprovar) | **v0.26.0** | M/G |
| ~~10~~ | ~~Completude opcional~~ | **CAIU — o líder manteve o adiamento do C0-PICVAR (2026-07-22, seção 6)** | — | — |
| **11** | Ressync docs/wiki (item fixo de fim-de-tabela) | `technical-writer`/`ux-writer`: GUIA_INICIANTE, wikis, README | release de doc | M |

### 5. Poços-sem-fundo — nomeados, com o número honesto

1. **SVG clean-room:** projeto de meses (spec enorme). Vendorizar agora; clean-room só sob a
   cláusula de internalização de longo prazo, sem data.
2. **Shaping ZWJ/tom-de-pele/bandeiras:** exige GSUB (o primeiro terço de um shaping engine).
   Já adiado 2× como 10–20× o esforço — a leitura se mantém. **Não existe fatia mínima barata
   defensável** (um hack "só ZWJ" ainda precisa da maquinaria GSUB). ADIAR com gatilho
   (consumidor renderizando bandeira/tom-de-pele de verdade); revisitar como programa próprio.
3. **COLRv1:** paint graph + gradientes + transforms = mini-rasterizador vetorial dentro do
   motor de fonte (G/GG). **A jogada defensável é o EM-CBDT** — strikes bitmap cobrem a Noto
   moderna por fração do custo; COLRv1 adiado com a mesma disciplina de gatilho.
4. **FONTENG 9px:** o spike PROVOU que não é arredondamento — é supressão de overshoot de topo
   curvo, ou seja hinting real de outline, a classe já rejeitada 2× por cara/arriscada. **Não
   há fix barato.** O QW-FONT9DOC (Onda 1) documenta o limite e as saídas (≥10px, ou o flip
   runtime pro FreeType que existe exatamente pra isso); o fix de código segue dormante.
5. **C0-PICVAR:** barato (P/M) mas com **zero consumidores** — o ADR-0009 já registrou o custo
   aceito; construir 2ª variante de archive contra demanda hipotética é scope creep pelo nosso
   próprio cânone. **Minha recomendação como CTO: NÃO fazer neste programa** — está na Onda 10
   só como override do líder, e é o único item do Bloco 1 onde contra-argumento o "tudo".
6. **Draw2D é G por natureza** — controlado pelo fatiamento: só o D2D-1 é G, e ainda assim é
   UMA capability coesa e taggeável (sprites). Se atrasar, nada mais bloqueia (ondas 4–7
   independem).
7. **Também sinalizado:** o WM-EXTRAS são 5 features pequenas num casaco só — se o review
   apertar, `vsync` + callback de modo saem num patch tag de follow-up em vez de esticar a onda.

### 6. Decisões da Onda 0 — DECIDIDAS PELO LÍDER 2026-07-22 (registro fechado, não reabrir)

1. **SVG = opção (B), lunasvg vendorizada** (pin + NOTICE, no átomo image; ADR-0018 gravado na
   Onda 4). — **Decisão do líder, 2026-07-22.**
2. **Draw2D = D2D-1 + D2D-2** (sprite/quad + batching, depois transform + `Camera2d`) — é onde
   o líder considera a meta cumprida. **D2D-3 fica na Onda 8** como planejado. **ADR-0017
   confirmado como pré-requisito do D2D-1.** — **Decisão do líder, 2026-07-22.**
3. **Adiamentos CONFIRMADOS integralmente** (seção 1.3), incluindo os 2 em que o CTO
   contra-argumentou e o líder manteve: **text-input/IME adiado** (gatilho: GusWorld começar o
   SAVE-NOMEAR) e **C0-PICVAR NÃO será feito** (Onda 10 cai). Também confirmados: ZWJ/GSUB e
   COLRv1 adiados; **EM-CBDT segue na Onda 9** (a jogada que cobre a Noto moderna); FONTENG
   9px fecha por documentação (QW-FONT9DOC). — **Decisão do líder, 2026-07-22.**
4. **Ordem das ondas 4–7: mantida como listada** (permutável, sem preferência do líder).
   — **Decisão do líder, 2026-07-22.**
