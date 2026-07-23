# ONDA 5 -- Draw2D primitives + layers + scissor + perf budget (D2D-3) / primitivas + layers + scissor + orcamento de performance do Draw2D (D2D-3)

> EN first, PT below (house convention). Plan author: Caetano (CTO). Baseline: glintfx
> **v0.21.0** on `main` (`18f28dd`). Target tag: **v0.22.0**. Governing architecture:
> [ADR-0015](../../../../docs/adr/0015-framework-2d-atomized-architecture.md) and
> [ADR-0017](../../../../docs/adr/0017-draw2d-module.md) WITH its Addendum (the camera model
> this wave extends; NO new ADR and NO addendum this wave -- the atom and its coordinate
> model already exist, this is the contracted D2D-3 growth of the same surface).
> Roadmap authority: `TODO.md` items `D2D-BLOCO` (the leader's two-axis ruler, closed, NOT
> reopened here) and `D2D-GAPS` (the consumer's measured usage counts). Program:
> [2026-07-22-programa-completo.md](2026-07-22-programa-completo.md) (D2D-3 scope line).
> Previous waves (normative decisions D1-D22, fully in force, NOT re-decided):
> [onda3](2026-07-23-onda3-draw2d-d2d1.md) (D1-D11), [onda4](2026-07-23-onda4-draw2d-camera.md)
> (D12-D22). This wave's decisions continue the numbering: **D23-D32**.
> Wave item ID prefix for commits: `D2D-3A` / `D2D-3B` / `PERF-D2D3` / `DOC-D2D3`.
> STATUS: PLANNED, autonomous mode (leader-authorized): no decision stop; the three
> design-shaped calls in section 2 are AUTONOMOUS DECISIONS, recorded with alternatives and
> flagged for retroactive confirmation by the leader at wave close.

---

## Part A -- EN

### 0. What this wave is and the three central risks

D2D-3 makes the Draw2D atom COMFORTABLE and MEASURED (program section 3's own words): the
game-world drawing that D2D-1/D2D-2 made possible gains untextured primitives (filled rect,
filled quad, line with thickness, rect outline), explicit draw-order control (layers),
rectangular clipping (scissor), the `texture_content_bbox` query, and a performance budget
test that DECLARES numbers instead of a bare pass/fail.

**Risk 1 -- becoming "GusWorld's renderer".** The consumer measured `draw_filled_rect` 29
uses, `draw_rect_outline` 19 uses (thickness in WORLD units, scaling with the camera),
`texture_content_bbox` 5 uses (born from a real playtest bug). That sets PRIORITY (axis 1 of
`D2D-BLOCO`). SCOPE (axis 2) is general distribution: every primitive here is complete and
generic (arbitrary filled quad, any-angle line, outline usable in screen OR world space),
no `IRenderer` mirroring, no consumer-shaped signature. Concretely, their
"thickness-in-world-units" need is NOT a bespoke API: it falls out of the generic rule
"primitive geometry is built in the ACTIVE space and each corner is projected" (D24) --
thickness scales with zoom because projection scales it, not because we special-cased them.

**Risk 2 -- layers vs batching is where performance can degrade.** A sorted-layer system
that buffers and reorders draws changes the memory profile and the flush timing of the whole
module. Antidote, twofold: (a) layers are OPT-IN with default OFF -- a bracket that never
calls `set_layer` runs the LITERAL v0.21.0 streaming path, unchanged (the same Q1 (c) idiom
the camera used, third use in this atom's history: absence of a diff, not a proof of
equivalence); (b) the layered path's cost is MEASURED by this same wave's budget test
(streaming vs layered comparative gate, D30) -- the risk is not argued away, it is priced.

**Risk 3 -- two new conversion formulas.** "A conversion formula without a computed-value
delta test" is this house's named silent-bug class. This wave adds exactly two: the line's
perpendicular-offset geometry (D25) and the scissor rect's top-left-y-down to GL-bottom-up
mapping (D28, `gl_y = target_h - (y + h)` -- the same class of flip `UiLayer::set_viewport`
already documents). Both are stated literally in this plan, both get hand-computed
expected-value tests, and both are named mutation targets in the QA brief. The onda-4
lesson also applies: a test that cannot distinguish the mutation does not exist -- every
formula test below includes at least one ASYMMETRIC case where the inverted/mirrored
formula fails loudly (e.g. the scissor readback uses an off-center rect precisely so a
y-flip error moves pixels to the WRONG half and the region assert fails).

### 1. Scope (what ships, what explicitly does not)

**Ships (3 code slices + 1 perf slice + 1 doc slice):**

- **D2D-3A** -- pure geometry + public surface: NEW internal pure header
  `src/primitives2d.hpp` (line-quad corners, outline 4-strip decomposition, validation
  predicates), the pure `compute_content_bbox()` addition to `src/image_decode.hpp`,
  public additions to `include/glintfx/draw2d.hpp` (the LITERAL contract of section 4),
  NEW pure suite `tests/primitives2d_sanity.cpp`. Zero GL in this slice.
- **D2D-3B** -- plumbing: NEW internal pure header `src/layer_queue.hpp` (buffered-command
  queue: layer tag, stable order, scissor snapshot, cap) + NEW `tests/layer_queue_sanity.cpp`;
  `src/sprite_batch.hpp` gains ONE additive method (`flush_pending()`, D31 -- nothing else
  in that file changes); `src/draw2d.cpp` implements the white texture (D23), the five
  primitive methods, `set_layer`, `set_scissor`/`reset_scissor`, `texture_content_bbox`,
  the buffered replay at `end()`, and the one declared diff in `set_gl_state_for_draw`
  (D28); NEW readback suite `tests/draw2d_primitives_render_sanity.cpp`; extended
  `tests/draw2d_hostile_sanity.cpp`; version bump `project(VERSION 0.22.0)`.
- **PERF-D2D3** -- `performance-engineer` (a DIFFERENT role from the adversarial
  `qa-engineer`): NEW `tests/draw2d_perf_budget.cpp` (declared-number budget, D30), ctest
  wiring, the strict-gate env line in the `claudio` heavy workflow, and the measured
  numbers handed to the doc slice.
- **DOC-D2D3** -- `docs/draw2d.md` gains the primitives/layers/scissor/bbox sections, the
  UPDATED D9 touched-state list (scissor is now conditionally ENABLED at flush -- a
  contract-visible change, documented loudly for embed hosts), the declared perf numbers,
  and the refreshed limits; `CHANGELOG.md` v0.22.0 draft; `tools/check_doc_line_refs.sh`
  re-run after the code commits (the header edits WILL shift cited line numbers).

**Does NOT ship (recorded, with trigger):**

- `draw_text` -- **D2D-TEXT**, its own wave, REQUIRES its own ADR first (font-engine
  tension; `D2D-BLOCO`'s closed order).
- Circle/ellipse/regular-polygon primitives -- seed `SEED-D2D-SHAPES` (trigger: consumer
  ask; the ui atom's `polygon()` decorator already covers the RCSS side).
- Line caps and joins (round/miter), polylines -- seed `SEED-D2D-LINECAPS`; this wave's
  line is butt-capped, documented.
- Secondary sort by texture WITHIN a layer (fewer flushes in layered mode) -- seed
  `SEED-D2D-LAYER-TEXSORT` (trigger: the budget test showing a flush-bound consumer).
- World-space clipping (a rotated clip region under a camera) -- seed `SEED-D2D-WORLDCLIP`;
  it is a mask/stencil-class feature, not a scissor. Scissor here is screen-space only (D28).
- An alpha THRESHOLD parameter for `texture_content_bbox` -- seed `SEED-D2D-BBOX-THRESH`;
  this wave's definition is `alpha > 0` (D29), documented.
- Unifying the old axis-aligned vertex path with the quad path -- still the onda-4 seed
  (soak first); this wave deliberately does not touch it.

### 2. Autonomous decisions (would have been AskUserQuestion; flagged for retroactive confirmation)

The leader authorized autonomous execution for this wave. These three calls are
design-shaped enough that under normal cadence each would have gone to the leader with
options. Each is recorded here with the alternatives and my recommendation APPLIED; the
wave-close report flags them for retroactive confirmation.

#### A1 -- how untextured primitives render

| Option | What it means | Cost |
|---|---|---|
| **(a) CHOSEN -- internal 1x1 white premultiplied texture in a normal registry slot** (created inside `init()`, id 1, handle NEVER handed out) | Primitives are quads through the EXISTING batcher and shader; the D8 tint formula on a white premultiplied texel `(1,1,1,1)` yields exactly the premultiplied fill colour | Zero new shader, zero new GL state class, zero diff in `sprite_batch.hpp`'s existing paths AND in `drain_ready()` (the slot resolves like any other texture); one texture switch when primitives interleave with sprites -- same cost as any multi-texture scene, measured by D30 |
| (b) Second colour-only shader program | Saves a 1x1 texture sample per fragment | A whole second GL pipeline to carry under D9 (two programs, two flush classes), real new bug surface of the exact class `GlStateGuard` exists for, for a per-fragment saving that is noise on both llvmpipe and real GPUs at 2D-game scale |
| (c) Branch/uniform in the existing shader | One program | A per-draw uniform breaks batching per primitive (uniform changes force flushes) or a per-fragment branch taxes every sprite ever drawn -- both worse than (a) on the axis they claim to help |

Rejection of (b)/(c) is revisitable with a trigger: the D30 budget test showing the white
texture switch dominating a real consumer's frame.

#### A2 -- the draw-order model

| Option | What it means | Cost |
|---|---|---|
| (a) No layer API; order = call order forever | Zero work, matches ADR-0017 axis 3 | Refuses the program's scope line; consumers whose iteration order is not depth order (entity lists) must sort on their side every frame |
| **(b) CHOSEN -- `set_layer(int)`, OPT-IN buffered mode, default OFF** | A bracket that never calls `set_layer` runs the LITERAL v0.21.0 streaming path (zero diff). Any `set_layer` call arms THAT bracket: draws are buffered (post-validation, post-projection screen-space corners + scissor snapshot), stable-sorted by layer at `end()` (equal layer = submission order), replayed through the untouched batcher | Two code paths in `Draw2d` (streaming + buffered) -- accepted; compatibility is provable by absence of diff, and the buffered path's cost is priced by D30. Memory: buffering holds the bracket's commands until `end()`, hard-capped (D27) |
| (c) Nested layer brackets (`begin_layer`/`end_layer`) | Explicit scoping | Doubles the bracket discipline the consumer must get right, solves nothing (b) does not, and pollutes the one-bracket-per-pass story D4 established |

Layer scope note, against creep: layers order draws WITHIN one Draw2D bracket only.
Composition with the UI and with the host's own GL stays "order = call order" (ADR-0017
axis 3, untouched).

#### A3 -- the scissor's coordinate space

| Option | What it means | Cost |
|---|---|---|
| **(a) CHOSEN -- ALWAYS screen px** (top-left, y-down, the same space as `begin()`'s target size), camera-independent | Scissor is a screen-space GL concept; "clip the minimap panel" is a screen-space need | A consumer wanting a WORLD-rect clip under rotation cannot have it -- correctly so: that is a mask-class feature (seed `SEED-D2D-WORLDCLIP`), and silently axis-aligning a rotated world rect would be a lie |
| (b) Active-space rect (world units with a camera, projected to an axis-aligned bbox) | One space story for everything | Surprising under rotation (the "scissor" would clip a DIFFERENT region than the world rect names), and couples a GL raster state to the camera -- the exact coupling D13 kept out |

### 3. Design decisions (D23-D32) -- normative

Numbering continues onda 3's D1-D11 and onda 4's D12-D22, all fully in force (fail-high
D10/D20, GL-state D9, batching D4, coordinates D5 as the no-camera case, CPU-side camera
D13, null-safe-on-moved-from for EVERY public method, D15's idiom for every setter).

**D23 -- The white-texture substrate (A1 (a)), lifecycle.** `init()` creates a 1x1
RGBA8 premultiplied WHITE texture through the same `upload_gl_texture` path and registers
it as a NORMAL registry slot (it becomes id 1; the public handle is never constructed, so
no consumer can draw with it directly or destroy it -- `Texture2d`'s fields are private
and only `load_texture` mints handles). Creation failure fails `init()` (fail-high: a
`Draw2d` that cannot draw primitives it advertises does not half-initialize).
`shutdown()` releases it with every other slot (existing loop, zero diff). Consequence,
stated: the first consumer `load_texture` now yields id 2 -- unobservable through the
public API (ids are opaque and private), asserted harmless by the existing suite staying
green.

**D24 -- Primitive semantics: geometry in the ACTIVE space, corners projected, one
composition story.** Every primitive (filled rect, filled quad, line, outline) builds its
quad corners in whichever space is active (world units with a camera set, screen px
otherwise -- exactly D18's rule for sprites), then projects each corner through
`world_to_screen` when a camera is set, then passes the post-math D20 finiteness guard,
then enters the batcher as a `draw_quad` against the white texture with the primitive's
colour as the D8 tint. Consequences: (1) thickness (line, outline) is in ACTIVE-space
units and scales with `cam.zoom` BY PROJECTION, no separate conversion formula to get
wrong -- but the scaling is still pinned by a computed-value test (zoom 3 on a
thickness-2 line yields a 6 px screen band, D30-class delta test); (2) the D8 tint
formula on the white texel makes the output exactly the premultiplied fill colour --
formula restated in the doc, pinned by a statistical readback (mid-alpha grey, region
mean); (3) every primitive shares the sprite path's fail-high template: any non-finite
input member -> skip + dedup'd log BEFORE any arithmetic; degenerate size (rect
`w <= 0 || h <= 0`, line `a == b` or `thickness <= 0`, outline `thickness <= 0`) ->
SILENT legal no-op (the D10 idiom); the post-math corner guard covers overflow (huge
zoom times huge coordinate), same as sprites (D20's second guard, now shared).

**D25 -- Line geometry, literal (single source: `src/primitives2d.hpp`).** With
`d = b - a`, `len = sqrt(d.x^2 + d.y^2)`, unit `u = d / len`, normal `n = (-u.y, u.x)`,
`h = thickness / 2`:

```
corners (TL,TR,BR,BL order, matching emit_quad_corners' slot-UV pairing):
  tl = a + n*h,  tr = b + n*h,  br = b - n*h,  bl = a - n*h
```

Guard order: all of `a`/`b`/`thickness`/`color` finite BEFORE any arithmetic (the sqrt
included); `len == 0` (a == b exactly) or `thickness <= 0` -> silent no-op BEFORE the
division (no divide-by-zero path exists). Butt caps (the quad ends exactly at `a` and
`b`), documented; caps/joins are seed `SEED-D2D-LINECAPS`. Mutation targets, named: swap
`n`'s components or drop the minus sign (the computed-value test on a horizontal line
with an asymmetric endpoint pair must fail); swap `h` for `thickness` (band width test
fails at exactly 2x).

**D26 -- Rect outline, literal: 4 NON-OVERLAPPING strips INSIDE the rect.** With
`t = min(thickness, min(w, h) / 2)` (the collapse clamp):

```
top    = {x,       y,        w,      t}
bottom = {x,       y+h-t,    w,      t}
left   = {x,       y+t,      t,      h-2t}
right  = {x+w-t,   y+t,      t,      h-2t}
```

Non-overlap is a CORRECTNESS property, not a micro-optimization: with a translucent
outline colour, overlapping strips double-blend at the corners (visibly darker
squares) -- pinned by a readback test drawing an `alpha 0.5` outline and asserting the
corner region's mean equals the edge region's mean within tolerance. When
`2*thickness >= min(w, h)` the outline degenerates to ONE filled rect covering `dst`
(documented; the clamp above makes the four strips tile exactly in that case -- the test
pins the boundary value). Thickness is active-space units (D24): the consumer's measured
"outline thickness in world units, scaling with the camera" need is THIS, with zero
consumer-shaped API. Fail-high: non-finite `dst`/`thickness`/`color` -> skip + dedup'd
log; `thickness <= 0` or `dst.w <= 0 || dst.h <= 0` -> silent no-op.

**D27 -- Layers: opt-in buffered mode (A2 (b)), literal rules.**

- `set_layer(int layer)`: any `int` is legal (negative = below default 0; no float, no
  NaN surface). Sticky WITHIN the bracket; the CURRENT layer tags every subsequent draw
  (sprites AND primitives).
- Arming: the first `set_layer` call arms buffered mode for the CURRENT bracket (or the
  NEXT one when called outside a bracket). A bracket never armed runs the literal
  v0.21.0 streaming path -- zero diff, the Q1 (c) idiom.
- Arming MID-bracket with draws already streamed: the already-flushed draws are under
  the bridge (they rendered first, effectively below every layer); `set_layer` finalizes
  and drains pending streaming state first, then arms. Deterministic, documented, with
  the recommendation "call set_layer before the first draw of the bracket".
- Buffered command = post-validation, post-projection state: final screen-space corners,
  texture id (white for primitives), resolved-space src_px, clamped tint, scissor
  snapshot (D28), layer tag. The camera is therefore sampled AT CALL TIME, exactly like
  streaming mode -- switching cameras mid-bracket keeps per-call semantics in both modes
  (D13 untouched).
- At `end()`: stable sort by layer (equal layer = submission order, guaranteed), replay
  through the UNTOUCHED batcher grouping consecutive same-scissor commands (D28), drain
  per group. `end()` then RESETS layer state: next bracket starts disarmed at layer 0.
  Rationale for not being sticky across brackets like the camera: the camera never
  changes flush behaviour (D13); layers do (buffering), so one experimental `set_layer`
  must not permanently change the module's flush timing.
- Memory: buffering holds one bracket's commands until `end()` -- hard cap 262144
  commands per bracket (~24 MiB at ~96 B/command), beyond which draws are dropped
  fail-high (logged once, dedup'd). The cap is a MEMORY bound against a hostile stream
  (same class as D4's 4096-quad flush bound), not a performance claim -- D30 measures.
- `shutdown()` discards any buffer (D32). Null-safe on moved-from, like every method.

**D28 -- Scissor: screen-space (A3 (a)), sticky state, flush-boundary semantics,
literal GL mapping.**

- `set_scissor(const RectF& rect_px)`: rect in SCREEN px, top-left origin, y-down, the
  same space as `begin()`'s target size; camera-independent (A3). Sticky across draws
  AND brackets (like the camera) until `reset_scissor()`/`shutdown()`.
- Validation (the D15 idiom): any non-finite member -> reject, PREVIOUS state kept, one
  dedup'd log. `w <= 0 || h <= 0` is LEGAL and means "clip everything" (GL semantics,
  documented -- an empty scissor is a valid tool, e.g. collapsing a reveal animation).
  Negative `x`/`y` legal (clamping is the GPU's job).
- Mid-bracket change in STREAMING mode forces a run boundary: pending run finalized and
  drained under the OLD scissor, then the state updates. Named contrast with the camera
  (D13: camera never forces a flush; scissor DOES -- it is per-flush GL state, not
  per-vertex data). In BUFFERED mode the snapshot travels with each command and replay
  groups consecutive same-scissor commands -- same observable semantics.
- GL mapping at flush, literal (the y-flip conversion formula, risk 3):
  `glScissor(x, target_h - (y + h), w, h)` with the rect first clamped to non-negative
  integer extents. Pinned by a hand-computed unit test AND an asymmetric readback (an
  off-center rect: a y-flip error moves the clipped region to the wrong half and the
  region assert fails loudly).
- The ONE declared diff in existing code: `set_gl_state_for_draw()` (`draw2d.cpp:284`)
  currently does `glDisable(GL_SCISSOR_TEST)` unconditionally; it becomes "if a scissor
  is set -> `glEnable` + `glScissor(mapped rect)`, else `glDisable`" -- still asserting
  EVERYTHING it depends on at every flush, per D9's own no-caching rule. With no scissor
  ever set the emitted GL calls are IDENTICAL to v0.21.0.
- D9 contract update, documented LOUDLY: Draw2D may now leave `GL_SCISSOR_TEST` ENABLED
  behind (it still restores nothing, by contract). In `App`, the frame hook's
  `GlStateGuard` already captures and restores scissor box+enable (`gl_state.hpp:68`,
  `:72`, `:129`, `:131`) -- verified, not assumed. In embed mode this is the sharpest
  edge of the whole wave for hosts: a leftover scissor clips the host's NEXT pass
  including its clear -- `docs/draw2d.md`'s touched-state list gains the row plus an
  explicit "reset your scissor or call reset_scissor() before handing the frame on"
  recipe line, and `draw2d_ui_coexist_sanity` gains a case proving `UiLayer::render()`
  is unaffected (its own `GlStateGuard` already re-establishes state).

**D29 -- `texture_content_bbox`: computed at load, cached, house-precedent return
struct.** The decoded pixels exist on the CPU exactly once, inside `load_texture()`
(`image_decode.hpp`'s `DecodedImage`, discarded after GL upload) -- so the bbox is
computed THERE, once, by a pure function, and cached in the registry slot (16 bytes).
Alternatives rejected: retaining the RGBA buffer (up to 256 MiB-class memory for a
5-use query) and `glGetTexImage` readback on demand (a GL round-trip, and a
GLES/core-profile portability trap). Definition: the smallest rect containing every
texel with `alpha > 0` (semi-transparent counts; threshold parameterization is seed
`SEED-D2D-BBOX-THRESH`). Premultiply does NOT disturb this: the alpha channel is
unchanged by premultiplication (`image_decode.hpp`'s loop multiplies RGB only), stated
so the reviewer does not have to re-derive it. Pure function
`compute_content_bbox(const unsigned char* rgba, int w, int h)` lives in
`src/image_decode.hpp` (the image atom's seam, where the SVG insertion also plugs);
crafted-array unit tests pin single-pixel corners at `(0,0)` and `(w-1, h-1)` (the
off-by-one mutation targets), a fully-transparent image, and a fully-opaque one.

Public API (house precedent: `ElementBox{found, x, y, w, h}` from `get_element_box`):

```
struct TextureBbox { bool found; int x, y, w, h; };  // texel coords, top-left, y-down
TextureBbox Draw2d::texture_content_bbox(const Texture2d& tex);
```

`found == false` (all fields zero) on: invalid/stale/foreign/never-loaded handle (the
full D7 validation chain, dedup'd log -- same rejection story as `draw_sprite`) OR a
fully-transparent texture (documented: distinct from a 1x1 opaque texture, which returns
`{true, 0, 0, 1, 1}`). Coordinates are in the SAME texel space as `src_px` (top-left,
y-down, matching image memory row order, D5) -- so the consumer's hitbox-from-bbox
pattern composes with `draw_sprite`'s `src_px` directly.

**D30 -- The performance budget: declared numbers, honest gates, honest downgrades.**
Design (the `performance-engineer`'s slice, distinct from the adversarial QA):

- **What is measured (all headless):** (1) pure batcher throughput -- 100k quads through
  `SpriteBatch` (CPU only, deterministic machine work, no display); (2) layered-mode
  overhead -- the SAME 100k-command stream through the buffered path (buffer + stable
  sort + replay), reported as a RATIO to (1); (3) end-to-end under Xvfb/llvmpipe -- a
  10k-sprite bracket plus a 1k-primitive bracket, median frame time over 30 frames.
- **What the numbers MEAN, declared:** llvmpipe rasterizes on the CPU -- (3) is a
  CPU-raster regression guard, NOT a GPU truth; no GPU throughput, no memory-bandwidth,
  no vsync pacing claim is made (downgrades section). (1) and (2) are real CPU numbers.
- **The gate, two-tier (a noisy-runner gate that still bites):** every metric prints a
  machine-readable line `GLINTFX_PERF <key>=<value>`. On the `claudio` self-hosted
  runner (stable hardware -- the house's heavy-CI leg), the workflow sets
  `GLINTFX_PERF_STRICT=1` and the test FAILS if a metric exceeds its recorded baseline
  by more than 50% (baselines are constants in the test source, measured on claudio
  during this wave, dated in a comment). Everywhere else (GitHub's variable runners,
  dev machines) only a 10x sanity ceiling fails the test -- the number is still printed
  and visible in the log. The layered/streaming RATIO gate (metric 2 <= 2x metric 1)
  is machine-relative, so it is STRICT everywhere -- it is the direct answer to risk 2.
- **The declared claim** (goes to `docs/draw2d.md` and the CHANGELOG, with the machine
  and date): "N quads/s through the batcher, layered mode <= 2x streaming, M ms median
  10k-sprite frame under llvmpipe on <machine>" -- real numbers filled in by
  `PERF-D2D3` at execution time, never invented in this plan.
- Historical tracking across releases is seed `SEED-PERF-TRACKDB` (trigger: second wave
  that wants to compare against more than one baseline).

**D31 -- Compatibility mechanics: where diffs are allowed, provable by `git diff`.**
The zero-diff set (the review MEASURES this): `sprite_batch.hpp`'s `draw_sprite()`,
`draw_quad()`, `emit_quad()`, `emit_quad_corners()`, `resolve_src()` and every existing
member -- the ONLY change in that file is the ADDITIVE `flush_pending()` method (public,
~4 lines: finalize current run, keep the bracket open -- what `set_scissor`'s
run-boundary and `set_layer`'s arming drain need); `draw2d.cpp`'s two existing
`draw_sprite` overloads, `begin()`, `end()`'s existing lines, `drain_ready()`,
`load_texture()`'s existing flow (it GAINS the bbox computation + slot store, declared),
`set_camera`/`reset_camera`. The ONE behavioural diff in existing code is
`set_gl_state_for_draw`'s scissor line (D28, declared above). Existing tests
(`draw2d_render_sanity`, `draw2d_camera_render_sanity`, `draw2d_ui_coexist_sanity`,
`draw2d_ripple_coexist_sanity`, `draw2d_hostile_sanity`, `sprite_batch_sanity`,
`transform2d_sanity`) run UNTOUCHED and green -- the published-contract guard. No new
CMake flag (same `GLINTFX_MODULE_DRAW2D` atom, contracted D2D-3 growth); with the flag
OFF, `nm` shows zero draw2d symbols INCLUDING every new method (nothing new is
header-inline that carries code; `primitives2d.hpp`/`layer_queue.hpp` are internal
headers compiled only into the module's TU). MSVC: new code is
`<cmath>`/`<algorithm>`/`<vector>` only, no Unix headers, no raw `min`/`max` issue
(NOMINMAX is directory-scope), compiles in the embed job as before. `project(VERSION
0.22.0)`.

**D32 -- Lifetime/state summary.** New `Draw2d` state and its resets: scissor -- sticky,
reset by `reset_scissor()` and `shutdown()`; layer/buffered-mode -- bracket-scoped,
reset by `end()` (and discarded by `shutdown()` mid-bracket); white texture -- created
by `init()` (failure fails init), released by `shutdown()`; bbox cache -- lives and dies
with its texture slot. Move transfers everything with the pImpl; every new public method
is a safe no-op on a never-init/moved-from/post-shutdown instance (the null-safe
contract, cppcheck-checked -- the onda-3 lesson applies to EVERY new method, eleven of
them this wave, it is the QA's checklist item, not a hope).

### 4. Public API contract (literal -- additions to `include/glintfx/draw2d.hpp`)

```cpp
// All in namespace glintfx; EN/PT doc-comments per house style; zero third-party types.

// D29 -- texel coords, top-left origin, y-down (the same space as src_px).
// found == false (all zeros): invalid handle OR fully-transparent texture.
struct TextureBbox {
  bool found = false;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

class Draw2d {
  // ... everything shipped in v0.21.0, unchanged ...

  // D23/D24 -- untextured primitives. Geometry in the ACTIVE space (world units with a
  // camera set, screen px otherwise); colour is straight RGBA, clamped on intake (D8).
  void draw_filled_rect(const RectF& dst, const ColorF& color);
  void draw_filled_rect(const RectF& dst, const ColorF& color,
                        const SpriteTransform& transform);         // rotated/scaled rect
  void draw_filled_quad(Vec2F tl, Vec2F tr, Vec2F br, Vec2F bl,
                        const ColorF& color);                      // arbitrary quad
  void draw_line(Vec2F a, Vec2F b, float thickness, const ColorF& color);  // D25
  void draw_rect_outline(const RectF& dst, float thickness, const ColorF& color); // D26

  // D27 -- draw order. Opt-in: a bracket with no set_layer call runs the literal
  // v0.21.0 streaming path. Any int is legal; equal layers keep submission order.
  // end() resets to layer 0 / streaming for the next bracket.
  void set_layer(int layer);

  // D28 -- ALWAYS screen px (top-left, y-down, begin()'s target space), camera-
  // independent. Sticky until reset_scissor()/shutdown(). Fail-high on NaN/Inf
  // (previous state kept); w<=0 || h<=0 is a LEGAL "clip everything".
  void set_scissor(const RectF& rect_px);
  void reset_scissor();

  // D29 -- cached at load_texture() time; full D7 handle validation.
  TextureBbox texture_content_bbox(const Texture2d& tex);
};
```

No method shipped in v0.21.0 changes signature or behaviour (D31's zero-diff set).
`draw_filled_quad`'s corners follow the TL,TR,BR,BL convention (D5/D19); a degenerate or
self-crossing (bowtie) quad is LEGAL and draws exactly the two triangles the corners
describe (the GPU's business, documented), with the D20 post-math guard still applying.

### 5. Testing (headless-testable vs not, downgrades declared)

**Canonical headless line (copy WHOLE, house rule):**
`env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest --test-dir <build> --output-on-failure -j1`

1. **Pure units -- NEW `primitives2d_sanity` (D2D-3A):**
   - D25 computed values: horizontal line `(0,0)-(10,0)` thickness 4 -> exact corner
     values; a DIAGONAL asymmetric case (the normal-swap mutation killer); thickness/2
     asymmetry (an `h`-vs-`thickness` mutation doubles the band and fails).
   - D25 guards: `a == b` and `thickness <= 0` silent no-op BEFORE any division; NaN in
     every field rejected before math; astronomically long line + huge thickness ->
     post-math guard path.
   - D26 computed values: the 4 strips for a hand-picked rect/thickness; NON-OVERLAP and
     EXACT COVERAGE asserts (union area == 2t(w+h) - 4t^2, pairwise intersection empty);
     the collapse boundary `2t == min(w,h)` tiles exactly; `2t > min(w,h)` yields the
     single filled rect.
   - D29 crafted arrays: single opaque pixel at `(0,0)` -> `{true,0,0,1,1}`; at
     `(w-1,h-1)` -> exact; two-pixel diagonal -> spanning bbox; all-transparent ->
     `found == false`; all-opaque -> full rect; a semi-transparent (`alpha == 1`) pixel
     COUNTS (the `alpha > 0` definition pin).
   - D28 mapping unit test: hand-computed `glScissor` args for an off-center rect on a
     known target height (the y-flip killer), including the clamp of negative extents.
2. **Pure units -- NEW `layer_queue_sanity` (D2D-3B):** stable order within a layer
   (equal-layer commands keep submission order -- the stable-sort mutation killer);
   negative and interleaved layers sort ascending; scissor grouping splits runs exactly
   at snapshot changes; the 262144 cap drops and reports (a 300k hostile stream stays
   bounded); camera-at-call-time semantics (commands store final corners).
3. **Pixel-readback under Xvfb/llvmpipe (statistical, region means, NEVER exact) --
   NEW `draw2d_primitives_render_sanity` (D2D-3B):**
   - Filled rect: region mean equals the premultiplied colour within tolerance (the D8
     white-texel formula pin); outside region untouched.
   - Alpha-0.5 outline: corner-region mean == edge-region mean (the D26 double-blend
     killer); hole region untouched.
   - Line at zoom 1 vs camera zoom 3: band width triples (the thickness-scales-by-
     projection delta test, risk 3).
   - Rotated filled rect (transform overload) under camera rotation pi/2: quadrant
     permutation, same style as the onda-4 camera readback.
   - Scissor: off-center rect -- inside painted, outside untouched, on BOTH y-halves
     (the y-flip killer); empty scissor (`w == 0`) clips everything; scissor + `UiLayer`
     coexistence case added to `draw2d_ui_coexist_sanity` (UI unaffected).
   - Layers: two overlapping translucent quads submitted in REVERSE layer order render
     back-to-front (region mean proves which is on top); same test WITHOUT set_layer
     proves call order (the arming pin); sprites and primitives interleaved across
     layers from ONE sprite texture + white (texture-change flushes survive sorting).
   - v0.21.0 regression: every existing draw2d/sprite/camera/coexist/hostile test runs
     UNTOUCHED and green.
4. **Hostile -- extended `draw2d_hostile_sanity` (D2D-3B):** every new method on
   never-init/post-shutdown/moved-from instances (eleven methods, the full sweep --
   cppcheck run included); NaN/Inf in every field of every primitive; `set_scissor`
   hostile keeps previous state (draw before/after lands identically); `set_layer`
   during a hostile/degenerate bracket; 300k-draw stream in buffered mode (cap holds,
   memory bounded); `texture_content_bbox` with stale/foreign/destroyed handles;
   `destroy_texture` then `texture_content_bbox` on the zeroed handle. ASan leg.
5. **Perf -- NEW `draw2d_perf_budget` (PERF-D2D3, D30):** the three metrics, the
   two-tier gate, the strict ratio gate, machine-readable output lines.
6. **Declared downgrades (limits stated instead of faked):**
   - llvmpipe = CPU raster: end-to-end perf numbers are regression guards, not GPU
     truth; no GPU throughput/bandwidth/pacing claim.
   - No pixel-exact GPU goldens (unchanged; exactness lives in the pure CPU tests only).
   - MSVC = compile+link gate only (embed job); no Windows runtime execution in CI.
   - Scissor+layers+camera COMBINED in one bracket is verified by the pure grouping
     tests plus single readbacks; the full combinatorial visual matrix is not
     headless-provable and is not claimed.
   - The strict perf baseline is claudio-hardware-specific; other machines get the
     sanity ceiling plus printed numbers, not the 50% gate.

### 6. House gates (all mandatory)

- Build dirs in **`/var/tmp`** (`/var/tmp/glx-onda5*`) with `export TMPDIR=/var/tmp`;
  **one heavy build at a time** (OOM is real on this machine); ASan leg before review
  sign-off.
- Suite green in BOTH configs (GLFW=ON full; GLFW=OFF embed), canonical headless line
  WHOLE, serial (`-j1` -- Xvfb contention under `-j$(nproc)` is a known false-failure).
- `tools/preci.sh` green; **`tools/check_doc_line_refs.sh` re-run AFTER any commit that
  shifts lines** -- `docs/draw2d.md` cites `draw2d.hpp:NNN`/`draw2d.cpp:NNN` numbers the
  D2D-3A/3B edits WILL displace.
- Symbol gates: ADR-0013 `nm` (zero raw `gl[A-Z]` B/D symbols) unchanged; with
  `GLINTFX_MODULE_DRAW2D=OFF`, `nm` shows zero draw2d symbols INCLUDING every new
  method (the reviewer runs both).
- MSVC-safe: no raw `-Wall`/`-Wextra`, no unguarded `min`/`max`, no
  `filesystem::path::c_str()`, no Unix-only header -- all new/touched files compile in
  the MSVC embed job.
- Implementer != reviewer != orchestrator; adversarial review EXECUTES (mutation on the
  canonical CMake build, NEVER mixing ASan/non-ASan objects); the orchestrator
  re-verifies the build and spot-checks claims before accepting any report.
- Push / merge / tag: autonomous mode authorizes push at wave close and the tag on
  green CI per the leader's standing authorization for THIS run; a red CI BLOCKS the
  tag unconditionally. Commits cite the slice ID and touch `TODO.md` status in the same
  commit (`🔍` on delivery, never `✅`).
- Wave close: `project(VERSION 0.22.0)`, CHANGELOG entry, consumer relay block for the
  leader (their `draw_filled_rect`/`draw_rect_outline`/`texture_content_bbox` map
  one-to-one; the outline's world-unit thickness is the default semantics, no adapter
  needed), the retroactive-confirmation list (section 2's A1/A2/A3).

### 7. Agent map (4 in the implementation fan-out; QA adversarial last)

| # | Agent | Slice | Owns (files) | Runs |
|---|---|---|---|---|
| 1 | `backend-engineer` (implementer A) | D2D-3A | NEW `glintfx/src/primitives2d.hpp`, `glintfx/src/image_decode.hpp` (compute_content_bbox addition), `glintfx/include/glintfx/draw2d.hpp` (section-4 additions, LITERAL), NEW `glintfx/tests/primitives2d_sanity.cpp`, `glintfx/tests/CMakeLists.txt` (their block) | first |
| 2 | `backend-engineer` (implementer B, DIFFERENT agent) | D2D-3B | NEW `glintfx/src/layer_queue.hpp`, `glintfx/src/sprite_batch.hpp` (flush_pending ONLY), `glintfx/src/draw2d.cpp`, NEW `glintfx/tests/layer_queue_sanity.cpp`, NEW `glintfx/tests/draw2d_primitives_render_sanity.cpp`, `glintfx/tests/draw2d_hostile_sanity.cpp` (extend), `glintfx/tests/draw2d_ui_coexist_sanity.cpp` (one scissor case), `glintfx/tests/CMakeLists.txt` (their block), `glintfx/CMakeLists.txt` (version bump + source list) | after A (header + pure geometry dependency; `tests/CMakeLists.txt` collision) |
| 3 | `performance-engineer` | PERF-D2D3 | NEW `glintfx/tests/draw2d_perf_budget.cpp`, `glintfx/tests/CMakeLists.txt` (their block), `.forgejo/workflows/heavy.yml` (the one GLINTFX_PERF_STRICT env line) | after B (needs the built module; serialize the heavy build) |
| 4 | `technical-writer` | DOC-D2D3 | `docs/draw2d.md` (primitives/layers/scissor/bbox + D9 touched-state UPDATE + perf numbers + limits), `CHANGELOG.md` (v0.22.0 draft), `docs/embed-integration.md` (scissor warning cross-ref line) | after B; may run in PARALLEL with #3 (file-disjoint, no build needed) but takes the perf NUMBERS from #3 before its final commit |
| 5 | `qa-engineer` (adversarial reviewer, EXECUTES) | whole wave | read+run everything; section 8.5 brief | last |

Orchestrator (main thread) re-verifies: clean build from scratch in `/var/tmp` (both
configs, sequentially), spot-check of claims (file:line), `git log` for unauthorized
pushes, the two `nm` gates, suite green, doc-line-ref gate, the D31 zero-diff `git diff`
check, then closes the wave (push/tag per the autonomous-mode authorization, CI green
mandatory).

### 8. Briefs (ready to paste)

#### 8.1 Brief -- implementer A (`backend-engineer`) -- D2D-3A

```
Voce e backend-engineer no glintfx (repo loucura_c_asm, pasta glintfx/). NAO faca push nem
merge. Commits locais Conventional pt-br citando D2D-3A + Status do TODO.md (-> 🔍) no
mesmo commit.

Leia ANTES: o plano glintfx/docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md
(secoes 3-4: D23-D32 e o contrato da secao 4 sao LITERAIS, nao invente assinatura);
docs/adr/0017-draw2d-module.md (corpo + adendo); glintfx/include/glintfx/draw2d.hpp,
glintfx/src/transform2d.hpp e glintfx/src/image_decode.hpp INTEIROS (o estilo de
doc-comment bilingue e a disciplina D10/D20 que voce estende); AGENTS.md (gotchas).

Entregue sob TDD (red/green/refactor), ZERO GL nesta fatia:
1. NEW glintfx/src/primitives2d.hpp: header interno PURO com a geometria de linha D25 AO
   PE DA LETRA (normal via (-u.y, u.x), butt caps, guardas ANTES da divisao: a==b ou
   thickness<=0 -> no-op ANTES do sqrt/divide; todo campo finito antes de qualquer
   conta) e a decomposicao de contorno D26 (4 faixas SEM sobreposicao, dentro do rect,
   clamp de colapso t = min(thickness, min(w,h)/2), 2t >= min(w,h) -> um rect cheio).
   Funcoes puras devolvendo cantos TL,TR,BR,BL (mesma ordem do emit_quad_corners) ou a
   lista de rects. SPDX + doc bilingue.
2. glintfx/src/image_decode.hpp: NEW funcao pura compute_content_bbox(rgba, w, h) (D29:
   alpha > 0, top-left y-down, mesmo espaco do src_px; all-transparent -> found false).
   NAO toque em decode_premultiplied_rgba (zero mudanca de comportamento).
3. include/glintfx/draw2d.hpp: EXATAMENTE a secao 4 do plano (TextureBbox, 5 metodos de
   primitiva + overload, set_layer, set_scissor/reset_scissor, texture_content_bbox
   declarados). Doc-comments bilingues completos (espaco ATIVO, fail-high, D27 arming,
   D28 screen-px + aviso embed). NAO implemente nada de .cpp (fatia do B).
4. NEW glintfx/tests/primitives2d_sanity.cpp: TODOS os itens 1 da secao 5 do plano --
   valores computados a mao (linha horizontal E diagonal assimetrica; contorno com
   nao-sobreposicao + cobertura exata + fronteira de colapso; bbox com pixel unico em
   (0,0) e (w-1,h-1), diagonal, all-transparent, all-opaque, alpha==1 conta; mapeamento
   glScissor D28 com rect fora do centro). Bloco proprio em tests/CMakeLists.txt.
Gates: build em /var/tmp/glx-onda5 com TMPDIR=/var/tmp (1 build pesado por vez); as DUAS
configs verdes com a linha headless canonica INTEIRA
(env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ... -j1);
tools/preci.sh limpo; MSVC-safe. Seus edits no draw2d.hpp DESLOCAM linhas citadas por
docs/draw2d.md -- NAO conserte o doc (fatia do technical-writer), mas LISTE no relatorio
quais citacoes quebraram (tools/check_doc_line_refs.sh te diz). Relatorio final com
file:line + saida do ctest.
```

#### 8.2 Brief -- implementer B (`backend-engineer`, agente DIFERENTE do A) -- D2D-3B

```
Voce e backend-engineer no glintfx. NAO faca push/merge. Commits locais Conventional
pt-br citando D2D-3B + Status do TODO.md (-> 🔍). Rode APOS o implementer A (voce depende
de primitives2d.hpp, do compute_content_bbox e do header publico novo; colide em
tests/CMakeLists.txt).

Leia ANTES: o plano (secoes 3-6; D23-D32 normativos); glintfx/src/primitives2d.hpp e o
compute_content_bbox (entregues pelo A -- consuma as funcoes DELE, nao reimplemente
NENHUMA formula); glintfx/src/draw2d.cpp, glintfx/src/sprite_batch.hpp e
glintfx/src/transform2d.hpp inteiros; glintfx/src/gl_state.hpp (o GlStateGuard que
restaura scissor no App -- gl_state.hpp:68/72/129/131); AGENTS.md.

REGRA DE COMPATIBILIDADE (D31, o review vai medir por git diff): em sprite_batch.hpp o
UNICO diff permitido e o metodo ADITIVO flush_pending() (finaliza a corrida corrente,
mantem o bracket aberto); em draw2d.cpp os overloads de draw_sprite existentes, begin(),
drain_ready(), set_camera/reset_camera ficam byte-identicos; o UNICO diff comportamental
em codigo existente e a linha de scissor do set_gl_state_for_draw (D28: condicional
enable+glScissor(x, target_h-(y+h), w, h) / disable -- sem scissor setado os GL calls
emitidos sao IDENTICOS aos da v0.21.0).

Entregue sob TDD, na ordem:
1. NEW glintfx/src/layer_queue.hpp (PURO, zero GL): comando bufferizado (cantos finais
   pos-projecao, texture id, src_px, tint clampado, snapshot de scissor, layer int),
   stable sort por layer no dreno (layer igual = ordem de submissao), agrupamento por
   scissor consecutivo, cap 262144 comandos fail-high. NEW
   glintfx/tests/layer_queue_sanity.cpp (itens 2 da secao 5).
2. glintfx/src/draw2d.cpp: textura branca 1x1 premultiplicada no init() como slot NORMAL
   do registry (id 1, handle nunca emitido; falha de criacao -> init() false, D23);
   os 5 metodos de primitiva (geometria via primitives2d.hpp do A, espaco ativo ->
   projecao por canto quando ha camera -> guarda pos-conta D20 -> batch.draw_quad com a
   textura branca e a cor como tint, D24); set_layer (D27: arma modo bufferizado do
   bracket corrente/proximo; armar no meio do bracket drena o streaming pendente antes;
   end() reseta pra streaming/layer 0); set_scissor/reset_scissor (D28: validacao D15,
   w/h<=0 legal "clipa tudo", mudanca em streaming forca flush_pending()+drain com o
   scissor VELHO); texture_content_bbox (D29: computado no load_texture via
   compute_content_bbox do A, cacheado no TextureSlot, validacao D7 completa no getter);
   replay bufferizado no end() (grupos por scissor, drain por grupo). TODO metodo novo
   null-safe em movido-de (a licao do cppcheck da onda 3 -- rode cppcheck).
3. Testes: NEW glintfx/tests/draw2d_primitives_render_sanity.cpp (itens 3 da secao 5 --
   tudo ESTATISTICO, regiao-media com tolerancia, NUNCA pixel-exato; inclui o rect fora
   do centro do scissor nas DUAS metades y, o contorno alpha 0.5 sem double-blend nos
   cantos, a linha com zoom 3 triplicando a banda, e as camadas com dois quads
   translucidos em ordem invertida); EXTEND draw2d_hostile_sanity.cpp (itens 4);
   +1 caso de scissor em draw2d_ui_coexist_sanity.cpp (UiLayer nao afetado).
4. glintfx/CMakeLists.txt: project(VERSION 0.22.0) + headers novos na lista de fontes.
Gates: build em /var/tmp/glx-onda5 (TMPDIR=/var/tmp, 1 pesado por vez); DUAS configs
verdes + leg ASan; linha headless canonica INTEIRA serial; preci.sh +
check_encapsulation.sh limpos; os DOIS nm (ADR-0013 zero gl cru;
GLINTFX_MODULE_DRAW2D=OFF remove TODOS os simbolos novos -- rode e cole no relatorio);
MSVC-safe. Os testes EXISTENTES do draw2d ficam INTOCADOS e verdes. Relatorio final com
file:line + ctest + os dois nm + o git diff do sprite_batch.hpp (provando so o
flush_pending).
```

#### 8.3 Brief -- `performance-engineer` -- PERF-D2D3

```
Voce e performance-engineer no glintfx (papel DISTINTO do qa-engineer adversarial). NAO
faca push/merge. Commit local Conventional pt-br citando PERF-D2D3 + Status do TODO.md
(-> 🔍). Rode APOS o implementer B (precisa do modulo construido; serialize o build
pesado -- 1 por vez nesta maquina).

Leia ANTES: o plano (D30 e a secao 5 item 5); glintfx/src/sprite_batch.hpp e
glintfx/src/layer_queue.hpp (o que voce mede); a politica de CI (AGENTS.md -- o runner
claudio e a perna estavel de hardware).

Entregue:
1. NEW glintfx/tests/draw2d_perf_budget.cpp com TRES metricas, cada uma imprimindo
   linha machine-readable "GLINTFX_PERF <chave>=<valor>":
   (a) batcher puro: 100k quads pelo SpriteBatch (CPU, sem display) -> quads/s;
   (b) overhead do modo layered: o MESMO stream de 100k comandos pelo caminho
       bufferizado (buffer + stable sort + replay) -> RATIO sobre (a);
   (c) end-to-end sob Xvfb/llvmpipe: bracket de 10k sprites + bracket de 1k primitivas,
       mediana de frame em 30 frames -> ms.
2. Gate em dois niveis (D30): com GLINTFX_PERF_STRICT=1 (so no runner claudio), falha
   se qualquer metrica passar de 1.5x a baseline gravada como constante no fonte
   (MEDIDA POR VOCE no claudio nesta onda, com data e maquina em comentario -- nunca
   inventada); sem a env, so o teto de sanidade 10x falha (numero sempre impresso).
   O gate de RATIO (b) <= 2x (a) e relativo a maquina, entao e ESTRITO em toda parte --
   e a resposta direta ao risco layers-vs-batching.
3. Wiring: bloco proprio em tests/CMakeLists.txt; a UNICA linha de env
   GLINTFX_PERF_STRICT=1 no job do .forgejo/workflows/heavy.yml (runner claudio). NAO
   mexa em mais nada dos workflows (politica de CI canonica).
4. Entregue os numeros medidos (as tres metricas + maquina + data) no seu relatorio --
   o technical-writer os grava no docs/draw2d.md e no CHANGELOG.
Declare os downgrades no proprio teste (comentario): llvmpipe = raster de CPU, guarda de
regressao, NAO verdade de GPU; baseline estrita e especifica do hardware do claudio.
Gates: build em /var/tmp/glx-onda5-perf (TMPDIR=/var/tmp); linha headless canonica
INTEIRA serial; MSVC-safe (o teste compila no job embed; as medicoes rodam no CI Linux).
Relatorio com file:line + as linhas GLINTFX_PERF coladas.
```

#### 8.4 Brief -- `technical-writer` -- DOC-D2D3

```
Voce e technical-writer no glintfx. NAO faca push/merge. Commit local citando DOC-D2D3.
Rode apos o implementer B (precisa das assinaturas e linhas reais); pegue os numeros de
performance do relatorio do performance-engineer ANTES do commit final. Docs bilingues
(EN primeiro). Sem em-dash em prosa nova.
1. docs/draw2d.md: (a) secao nova de primitivas (espaco ATIVO, D24; formula D8 sobre o
   texel branco = cor premultiplicada; linha D25 com butt caps; contorno D26 com as 4
   faixas sem sobreposicao, o clamp de colapso e a espessura que ESCALA com o zoom por
   projecao -- o caso de uso do consumidor sai daqui, sem API dedicada); (b) secao de
   layers (D27: opt-in, default streaming v0.21.0 literal, stable sort, cap 262144,
   armar no meio do bracket, reset no end(); layers ordenam SO dentro do bracket --
   composicao com UI/host continua ordem-de-chamada, ADR-0017 eixo 3); (c) secao de
   scissor (D28: SEMPRE px de tela, formula gl_y = target_h-(y+h) verbatim, w/h<=0
   legal, muda-no-meio forca flush em contraste com a camera D13, e o AVISO ALTO de
   embed: o Draw2D pode deixar GL_SCISSOR_TEST LIGADO -- no App o GlStateGuard restaura
   (gl_state.hpp), no embed o host precisa resetar ou chamar reset_scissor()); a LISTA
   DE ESTADO TOCADO do D9 ganha a linha do scissor (contrato publicado muda -- destaque
   de changelog); (d) secao do texture_content_bbox (D29: alpha>0, espaco do src_px,
   found==false nos dois casos, computado no load -- custo zero por consulta); (e)
   secao de performance com os numeros DECLARADOS do performance-engineer (maquina +
   data + o que llvmpipe significa); (f) tabela de API com as ~10 linhas novas; (g)
   limites declarados atualizados (sem draw_text ate D2D-TEXT; caps/joins, shapes,
   world-clip, threshold de bbox = seeds); (h) TODAS as citacoes arquivo:linha
   deslocadas -- rode tools/check_doc_line_refs.sh ate 0 falha ANTES e DEPOIS.
2. CHANGELOG.md: rascunho v0.22.0 (Added: D2D-3A/D2D-3B/PERF-D2D3; Changed: a linha de
   scissor no contrato D9; downgrades declarados juntos).
3. docs/embed-integration.md: uma linha de cross-ref na secao Draw2D existente sobre o
   aviso de scissor em embed.
Nada de claim nao verificavel; cite file:line real.
```

#### 8.5 Brief -- `qa-engineer` (review adversarial, EXECUTA) -- onda inteira

```
Voce e qa-engineer, revisor ADVERSARIAL da onda D2D-3A/D2D-3B/PERF-D2D3/DOC-D2D3 do
glintfx. Voce NAO implementou nada disto (implementer != reviewer). NAO faca push/merge.
Metodo: voce EXECUTA, nao so le. Build limpo do zero em /var/tmp/glx-onda5-review
(TMPDIR=/var/tmp, 1 build pesado por vez), DUAS configs + leg ASan -- mutation SEMPRE no
build canonico do CMake, NUNCA misturando objetos ASan/nao-ASan.
1. Mutation dirigido as FORMULAS (a classe Critical da casa; teste que nao distingue a
   mutacao NAO existe):
   - D25: troque a normal (-u.y,u.x) por (u.y,-u.x) e por (u.x,u.y) (o caso DIAGONAL
     pega?); troque h por thickness (a banda dobra -- pega?);
   - D26: faca as faixas sobrepor (top/bottom largura cheia + left/right altura cheia --
     o readback de contorno alpha 0.5 pega o double-blend?); quebre o clamp de colapso;
   - D28: troque target_h-(y+h) por target_h-y e por y (o readback assimetrico nas duas
     metades pega?); remova o glDisable do ramo sem-scissor (o coexist pega?);
   - D27: troque stable_sort por sort instavel com comparador que empata (o teste de
     ordem-de-submissao pega?); inverta a ordem de layer; remova o cap (o stream de
     300k estoura memoria? rode com limite);
   - D29: introduza off-by-one nos dois extremos do bbox (os testes de pixel em (0,0) e
     (w-1,h-1) pegam?); conte alpha==0 como conteudo (o all-transparent pega?);
   - D23: mude o branco pra (0.5,0.5,0.5,1) (o readback de cor de rect pega?).
2. Hostil: os 11 metodos novos em never-init/pos-shutdown/movido-de (rode cppcheck --
   a licao do null-deref da onda 3); NaN/Inf em todo campo de toda primitiva ANTES de
   qualquer conta (strace do log dedup'd); set_scissor hostil mantem estado (desenhe
   antes/depois e compare); linha com a==b e thickness<=0 em silencio (SEM log);
   overflow zoom*coordenada nas primitivas cai na guarda pos-conta; stream 300k em modo
   layered limitado; scissor deixado LIGADO + UiLayer::render depois (coexist).
3. Compatibilidade (D31, o risco central): git diff de sprite_batch.hpp mostra SO o
   flush_pending aditivo; os overloads de draw_sprite/begin/drain_ready/set_camera de
   draw2d.cpp byte-identicos; a unica mudanca comportamental e a linha de scissor do
   set_gl_state_for_draw; TODOS os testes existentes do draw2d INTOCADOS e verdes; sem
   scissor setado os GL calls do flush sao os da v0.21.0 (leia o codigo E rode).
4. Domino: todo guard novo tem gemeo ou analise-de-gemeo nos 5 metodos de primitiva
   (NaN checado em a/b/thickness/color/dst/transform? o mesmo conjunto do draw_sprite?);
   os DOIS nm (ADR-0013; flag OFF remove todos os simbolos novos -- rode voce mesmo);
   MSVC-safe nos arquivos tocados.
5. Perf: rode o draw2d_perf_budget SEM a env estrita (teto de sanidade + numeros
   impressos) e confira que o gate de RATIO (layered <= 2x streaming) roda estrito;
   confira que a baseline no fonte tem data/maquina e bate com o relatorio do
   performance-engineer (numeros inventados = Critical).
6. Docs: formulas do draw2d.md batem VERBATIM com primitives2d.hpp/draw2d.cpp? A lista
   de estado tocado D9 tem a linha do scissor? check_doc_line_refs.sh 0 falha? Cada
   claim verificavel?
Relatorio: Critical/Major/Minor com file:line + comando de repro. Critical/Major
bloqueiam a tag. O orquestrador vai re-verificar suas claims -- seja preciso.
```

### 9. TODO.md rows (for the orchestrator to insert)

| ID | Item | Onda | Esforço | Status |
|---|---|---|---|---|
| D2D-3A | Geometria pura D2D-3: primitives2d.hpp (linha D25 + contorno D26, fonte única) + compute_content_bbox no image_decode.hpp (D29) + contrato público (TextureBbox + 10 declarações) + primitives2d_sanity | 5 | M | ⬜ |
| D2D-3B | Plumbing D2D-3: textura branca (D23), 5 primitivas (D24), set_layer bufferizado (D27), set_scissor/reset_scissor (D28), texture_content_bbox (D29), layer_queue.hpp + sanity, readback + hostil, VERSION 0.22.0 | 5 | G | ⬜ |
| PERF-D2D3 | Orçamento de performance (D30): draw2d_perf_budget com 3 métricas declaradas, gate 2 níveis (estrito no claudio), ratio layered<=2x estrito em toda parte, números pro doc | 5 | M | ⬜ |
| DOC-D2D3 | docs/draw2d.md (primitivas/layers/scissor/bbox + lista D9 atualizada + números de perf) + CHANGELOG v0.22.0 + line-refs re-verificados | 5 | P/M | ⬜ |

INBOX seeds to add: `SEED-D2D-SHAPES` (circle/ellipse/polygon primitives; trigger:
consumer ask), `SEED-D2D-LINECAPS` (caps/joins/polyline; trigger: consumer ask),
`SEED-D2D-LAYER-TEXSORT` (texture sort within layer; trigger: budget shows flush-bound
frame), `SEED-D2D-WORLDCLIP` (world-space clip, mask-class; trigger: consumer ask),
`SEED-D2D-BBOX-THRESH` (alpha threshold param; trigger: consumer ask),
`SEED-PERF-TRACKDB` (historical perf tracking; trigger: second baseline consumer).

### 10. CTO counter-arguments (honest record)

1. **Is D2D-3 as scoped one wave, or too big?** It is the largest wave since D2D-1
   (program priced it M/G; this plan lands at G for the B slice). I considered cutting
   it in two (primitives+scissor+bbox / layers+perf). I kept ONE wave because: the
   slices are file-disjoint and sequenced; the perf budget only makes sense WITH layers
   in the same tag (risk 2 is priced the moment it ships, not a release later); and the
   opt-in design (A2) makes layers REMOVABLE at review time without dragging the tag --
   if QA finds a Critical in the buffered path, the cut line is exactly `set_layer` +
   `layer_queue.hpp`, and primitives/scissor/bbox/perf still tag as a coherent v0.22.0.
   That is the stop-anywhere property the program demands, preserved inside the wave.
2. **Why not a second colour-only shader for primitives?** Because the 1x1 white
   texture rides EVERY existing discipline for free (batcher, D8 tint, D9 state list,
   nm gate) and the per-fragment cost it "wastes" is noise at 2D scale -- while a second
   GL pipeline is a permanent new bug surface of the exact class this house already
   guards with test-pinned state contracts. The budget test is the revisit trigger.
3. **Layers add a second execution path (streaming + buffered).** Deliberate, same
   trade as onda 4's two vertex paths: compatibility provable by ABSENCE of diff beats
   compatibility provable by test, and we keep both nets anyway (untouched streaming
   path AND the readback pins). Unification is a seed after a soak wave, same policy.
4. **Scissor changes a PUBLISHED contract line** (the D9 touched-state list said
   scissor is always disabled). True, and it cannot be helped -- a scissor feature that
   never enables scissor does not exist. The change is additive-in-spirit (no scissor
   set = identical GL stream), loudly documented, and the embed hazard (leftover
   scissor clipping the host's clear) gets its own doc recipe + coexist test. This is
   the one item I would have taken to the leader outside autonomous mode; it is flagged
   in section 2's spirit even though it is a consequence, not an option.
5. **The perf numbers are llvmpipe numbers.** Declared, not hidden: the end-to-end
   metric guards regression of OUR CPU-side work (batching, buffering, sort, state
   churn) plus a stable software raster -- which is exactly the part of the stack this
   wave changes. GPU truth would need real-GPU CI this house does not have; faking it
   with an untracked local run would be worse than declaring the limit.
6. **`texture_content_bbox` computes on every load even for consumers that never ask.**
   One linear pass over pixels already in cache from the premultiply loop, at asset-load
   time (not per-frame), to avoid a 256 MiB-class retention or a GL readback. If a
   profile ever shows it mattering, a lazy flag is a two-line change behind the same
   API.

---

## Part B -- PT

### 0. O que é a onda e os três riscos centrais

O D2D-3 torna o átomo Draw2D CONFORTÁVEL e MEDIDO: primitivas sem textura (rect cheio,
quad cheio, linha com espessura, contorno de rect), controle explícito de ordem
(layers), recorte retangular (scissor), a consulta `texture_content_bbox`, e um
orçamento de performance que DECLARA números. **Risco 1, nomeado:** virar "o renderer
do GusWorld" -- os 29+19+5 usos medidos deles definem a PRIORIDADE (eixo 1 do
`D2D-BLOCO`), mas o ESCOPO é distribuição geral (eixo 2): cada primitiva sai completa e
genérica; a "espessura em unidade de mundo" deles NÃO vira API dedicada -- cai da regra
geral "geometria no espaço ATIVO, cantos projetados" (D24). **Risco 2:** layers vs
batching é onde a perf pode degradar -- antídoto duplo: layers OPT-IN com default
DESLIGADO (bracket sem `set_layer` roda o caminho streaming v0.21.0 LITERAL, o idioma
Q1 (c) pela terceira vez) e o custo do caminho bufferizado é MEDIDO pela própria onda
(gate de ratio no D30). **Risco 3:** duas fórmulas de conversão novas (a normal da
linha D25 e o y-flip do scissor D28, `gl_y = target_h - (y + h)`) -- ambas literais no
plano, com teste de valor computado à mão e caso ASSIMÉTRICO que faz a mutação falhar
alto (lição da onda 4: teste que não distingue a mutação não existe).

### 1. Escopo

**Entra:** D2D-3A (geometria pura: primitives2d.hpp com linha D25 + contorno D26;
compute_content_bbox no image_decode.hpp; contrato público literal; suíte pura);
D2D-3B (textura branca D23, 5 primitivas D24, layers D27 com layer_queue.hpp puro,
scissor D28, bbox D29, readback + hostil, bump 0.22.0); PERF-D2D3
(`performance-engineer`: orçamento D30 com números declarados); DOC-D2D3 (doc +
CHANGELOG + lista D9 atualizada + line-refs).

**NÃO entra (com gatilho):** `draw_text` (D2D-TEXT, exige ADR); círculo/elipse
(`SEED-D2D-SHAPES`); caps/joins de linha (`SEED-D2D-LINECAPS`); sort por textura dentro
do layer (`SEED-D2D-LAYER-TEXSORT`); clip em espaço de mundo (`SEED-D2D-WORLDCLIP`);
threshold de alpha no bbox (`SEED-D2D-BBOX-THRESH`); unificação dos dois caminhos de
vértice (seed da onda 4, soak primeiro).

### 2. Decisões autônomas (confirmar retroativamente com o líder)

- **A1 -- primitivas via textura BRANCA 1x1** premultiplicada em slot normal do
  registry (id 1, handle nunca emitido; criada no init(), falha -> init() false):
  zero shader novo, zero estado GL novo, zero diff no batcher; a fórmula D8 sobre o
  texel branco entrega exatamente a cor premultiplicada. Rejeitadas: segundo shader
  color-only (segundo pipeline inteiro sob o D9 por economia de ruído) e branch/uniform
  no shader existente (quebra batching ou taxa todo sprite).
- **A2 -- layers = `set_layer(int)` opt-in, default OFF**: bracket sem set_layer roda o
  streaming v0.21.0 literal; qualquer set_layer arma O bracket (comandos bufferizados
  pós-validação/pós-projeção, stable sort por layer no end(), replay pelo batcher
  intocado, cap 262144 fail-high; end() reseta -- layers não são sticky entre brackets
  porque, ao contrário da câmera D13, mudam o timing de flush). Rejeitadas: não fazer
  (recusa a linha de escopo) e brackets aninhados de layer (dobra a disciplina sem
  ganho). Layers ordenam SÓ dentro do bracket; composição com UI/host segue
  ordem-de-chamada (ADR-0017 eixo 3 intocado).
- **A3 -- scissor SEMPRE em px de TELA** (top-left, y-down, o espaço do begin()),
  independente de câmera. Rejeitada: rect em espaço ativo (sob rotação "clipparia" uma
  região diferente da nomeada -- mentira silenciosa; clip de mundo é classe
  mask/stencil, seed).

### 3. Decisões de desenho (resumo -- normativo completo na Parte A, D23-D32)

- **D23**: textura branca no init() como slot normal; primeiro load_texture do
  consumidor vira id 2 (inobservável, ids são opacos); shutdown() libera no loop
  existente.
- **D24**: TODA primitiva constrói cantos no espaço ATIVO -> projeta por canto com
  câmera -> guarda pós-conta D20 -> draw_quad com a branca e a cor como tint. Espessura
  escala com o zoom POR PROJEÇÃO (sem fórmula extra), fixada por teste de delta
  (zoom 3 -> banda 3x). Fail-high do template do sprite: NaN -> skip + log dedup'd
  ANTES de qualquer conta; degenerado (w/h<=0, a==b, thickness<=0) -> no-op SILENCIOSO.
- **D25**: linha literal -- `u = (b-a)/len`, `n = (-u.y, u.x)`, cantos `a±n·t/2`,
  `b±n·t/2` (TL,TR,BR,BL); butt caps; guardas antes da divisão.
- **D26**: contorno = 4 faixas SEM sobreposição por dentro do rect (double-blend com
  alpha translúcido é bug de correção, fixado por readback); clamp de colapso
  `t = min(thickness, min(w,h)/2)`; `2t >= min(w,h)` -> um rect cheio.
- **D27**: modelo de layers da A2 (regras literais na Parte A: arming, meio-de-bracket,
  comando bufferizado com câmera amostrada NA chamada, cap, reset no end()).
- **D28**: scissor da A3 -- sticky (como a câmera) até reset_scissor()/shutdown();
  validação D15 (NaN -> mantém estado); w/h<=0 legal ("clipa tudo"); mudança no meio do
  bracket FORÇA flush (contraste nomeado com a câmera D13); mapeamento GL literal
  `glScissor(x, target_h-(y+h), w, h)`; o ÚNICO diff em código existente é a linha do
  set_gl_state_for_draw (sem scissor setado, GL calls idênticos à v0.21.0); a lista de
  estado tocado do D9 ganha a linha e o doc avisa ALTO o host embed (scissor esquecido
  ligado clipa o clear do host; no App o GlStateGuard já restaura --
  gl_state.hpp:68/72/129/131, verificado).
- **D29**: bbox computado UMA vez no load_texture (o pixel já está na CPU ali; premult
  não toca o canal alpha), cacheado no slot (16 B); `TextureBbox{found,x,y,w,h}`
  (precedente ElementBox); `alpha > 0`; all-transparent -> found false; validação D7
  completa; função pura no image_decode.hpp (a costura do átomo image).
- **D30**: orçamento com números DECLARADOS -- 3 métricas (batcher puro 100k quads;
  ratio layered/streaming; end-to-end llvmpipe 10k sprites) impressas machine-readable;
  gate estrito 1.5x da baseline SÓ no runner claudio (hardware estável,
  GLINTFX_PERF_STRICT=1 no heavy.yml); teto de sanidade 10x no resto; o gate de RATIO
  (<= 2x) é relativo à máquina, estrito em toda parte. Downgrade declarado: llvmpipe é
  raster de CPU -- guarda de regressão, não verdade de GPU.
- **D31**: compat provável por git diff -- sprite_batch.hpp só ganha o flush_pending()
  aditivo; os caminhos existentes do draw2d.cpp byte-idênticos; testes existentes
  intocados e verdes; sem flag CMake nova; nm com a flag OFF remove tudo; MSVC-safe;
  VERSION 0.22.0.
- **D32**: resets -- scissor no reset_scissor()/shutdown(); layers no end(); branca no
  init()/shutdown(); bbox com o slot; TODO método novo null-safe em movido-de (11
  métodos, checklist do QA com cppcheck).

### 4. Testes, gates, agentes

- **Headless-testável (resposta exata):** unidades puras (linha D25 com caso diagonal
  assimétrico; contorno D26 com não-sobreposição/cobertura/fronteira de colapso; bbox
  D29 com pixels únicos nos dois extremos; mapeamento D28 à mão; layer_queue com stable
  sort e cap) + readback ESTATÍSTICO sob Xvfb/llvmpipe (rect cheio com média da cor
  premultiplicada; contorno alpha 0.5 sem double-blend; banda da linha 3x sob zoom 3;
  scissor assimétrico nas duas metades; layers com quads translúcidos em ordem
  invertida; coexistência scissor+UiLayer) + hostil (11 métodos em
  never-init/movido-de, NaN em tudo, 300k no cap, scissor mantém estado) + perf (D30).
  **NÃO testável headless, declarado:** GPU real (llvmpipe = CPU raster); execução
  Windows (MSVC só compila); matriz combinatória scissor×layers×câmera visual (coberta
  pela pura + readbacks pontuais, não alegada).
- **Gates:** `/var/tmp/glx-onda5*` + `TMPDIR=/var/tmp`, 1 build pesado por vez, duas
  configs + ASan, linha headless canônica INTEIRA e serial, preci.sh,
  check_doc_line_refs.sh re-rodado após todo commit que desloca linha, os DOIS nm,
  MSVC-safe, implementer != reviewer != orquestrador, review que EXECUTA (mutations
  nomeadas no brief 8.5), CI vermelho BLOQUEIA a tag; push/tag ao fim da onda pela
  autorização de modo autônomo vigente.
- **Agentes (4 no fan-out; QA por último):** implementer A (`backend-engineer`,
  D2D-3A: geometria pura + header público + bbox puro + unitários) -> implementer B
  (`backend-engineer` DIFERENTE, D2D-3B: branca/primitivas/layers/scissor/bbox/
  readback/hostil/bump; depende do A, colide em tests/CMakeLists.txt) ->
  `performance-engineer` (PERF-D2D3, após B, serializa build) em PARALELO com
  `technical-writer` (DOC-D2D3, após B, sem build; pega os números do perf antes do
  commit final) -> `qa-engineer` adversarial que EXECUTA. Orquestrador re-verifica
  build limpo, claims file:line, git log, os dois nm, o git diff do D31.

### 5. Contra-argumentos do CTO (registro honesto)

1. **Onda grande demais?** É a maior desde o D2D-1 (M/G no programa; G na fatia B).
   Mantive UMA onda porque as fatias são disjuntas e sequenciadas, o orçamento de perf
   só faz sentido NA MESMA tag que os layers (o risco 2 é precificado no embarque), e o
   design opt-in torna os layers REMOVÍVEIS no review sem derrubar a tag -- a linha de
   corte é exatamente `set_layer` + `layer_queue.hpp`, e primitivas/scissor/bbox/perf
   ainda fecham uma v0.22.0 coerente (stop-anywhere preservado DENTRO da onda).
2. **Por que não um shader color-only?** A branca 1x1 herda TODA disciplina existente
   de graça; um segundo pipeline GL é superfície de bug permanente por uma economia que
   é ruído em escala 2D. Gatilho de revisita: o próprio orçamento D30.
3. **Dois caminhos de execução (streaming + bufferizado).** Deliberado, mesmo trade da
   onda 4: compat por AUSÊNCIA de diff vence compat por teste; unificação é seed
   pós-soak.
4. **O scissor muda uma linha de contrato PUBLICADO** (a lista D9 dizia scissor sempre
   OFF). Inevitável -- scissor que nunca liga scissor não existe. Sem scissor setado o
   stream GL é idêntico; o perigo embed (scissor esquecido clipando o clear do host)
   ganha receita no doc + teste de coexistência. É o item que iria ao líder fora do
   modo autônomo -- flagado pra confirmação retroativa junto com A1-A3.
5. **Números de perf são de llvmpipe.** Declarado: guardam regressão do NOSSO trabalho
   CPU-side (batching, buffer, sort, estado) -- exatamente a parte que esta onda muda.
   Verdade de GPU exigiria CI com GPU real, que a casa não tem; declarar o limite vence
   fingir o número.
6. **Bbox computado em todo load mesmo sem consulta.** Um passe linear sobre pixels já
   quentes do loop de premultiply, em tempo de load de asset (não por-frame), pra
   evitar retenção de 256 MiB ou readback GL. Se um profile mostrar custo, um lazy flag
   é mudança de duas linhas atrás da mesma API.
