# ONDA 4 -- Draw2D transforms + Camera2d (D2D-2) / transforms + Camera2d do Draw2D (D2D-2)

> EN first, PT below (house convention). Plan author: Caetano (CTO). Baseline: glintfx
> **v0.20.0** on `main` (`4e7bd5f`). Target tag: **v0.21.0**. Governing architecture:
> [ADR-0015](../../../../docs/adr/0015-framework-2d-atomized-architecture.md) and
> [ADR-0017](../../../../docs/adr/0017-draw2d-module.md) (the Draw2D atom; this wave ADDS an
> addendum to it -- section 2 -- recorded by `software-architect` AFTER the leader decides).
> Roadmap authority: `TODO.md` item `D2D-BLOCO` (leader decision 2026-07-23: the two-axis
> ruler) and `D2D-GAPS` (the consumer's measured feedback). Program:
> [2026-07-22-programa-completo.md](2026-07-22-programa-completo.md) (D2D-2 scope line).
> Previous wave (normative decisions D1-D11 this wave builds on, NOT re-decided here):
> [2026-07-23-onda3-draw2d-d2d1.md](2026-07-23-onda3-draw2d-d2d1.md).
> Wave item ID prefix for commits: `D2D-2A` / `D2D-2B` / `DOC-D2D2` / `ADR17-ADD`.
> STATUS: PLANNED -- awaiting (1) leader decision on section 2, (2) addendum recording,
> (3) orchestrator fan-out.

---

## Part A -- EN

### 0. What this wave is and the two central risks

D2D-2 gives the Draw2D atom the thing that makes it adoptable as a GAME renderer: per-sprite
transforms (pivot, rotation, scale, flip H/V) and a world camera (`Camera2d`: position, zoom,
rotation; `set_camera`; pure `world_to_screen`/`screen_to_world` helpers). After this wave the
framework's claim "you can draw a simple 2D game with glintfx alone" is honest (program
section 3).

**Risk 1, named up front: this wave wants to become "implement GusWorld's renderer".** The
consumer measured 19 uses of `begin_frame(camera_world, w, h)` and declared camera a
precondition of any adoption -- that sets the PRIORITY (axis 1 of `D2D-BLOCO`). The SCOPE
(axis 2) is general distribution: this plan ships a generic value-type camera and generic
sprite transforms, NOT a mirror of their `IRenderer`. Concretely: no `begin_frame(world_rect)`
API shape, no per-consumer conveniences beyond one pure, generic rect-fit helper (D17, the
single cuttable piece), and the mapping from THEIR rect-camera to OUR camera goes in the
consumer relay block at wave close, not into the API.

**Risk 2, named up front: world<->screen projection IS a conversion formula**, and "a
conversion formula without a computed-value delta test" is this house's named silent-bug
class (it already cost us a Critical once). Every formula in section 3 is therefore stated
literally in this plan, implemented ONCE in a pure header (single source for both the draw
path and the public helpers -- they cannot diverge), and pinned by computed-expected-value
unit tests plus a round-trip property test (section 5).

### 1. Scope (what ships, what explicitly does not)

**Ships (1 addendum + 2 code slices + 1 doc slice):**

- **ADR17-ADD** -- addendum to ADR-0017 recording the leader's section-2 decision (coordinate
  space model + camera anchor). Recorded by `software-architect` AFTER the decision, same
  house precedent as ADR-0008's "adendo F3" (the v0.2.5 viewport addendum). Not part of the
  implementation fan-out.
- **D2D-2A** -- pure math + public types: new internal pure header `src/transform2d.hpp`
  (world->screen / screen->world, sprite corner generation, all validation), public additions
  to `include/glintfx/draw2d.hpp` (`Vec2F`, `Camera2d`, `SpriteTransform`, free helper
  declarations, new `Draw2d` method declarations -- the LITERAL contract of section 4), pure
  unit suite `tests/transform2d_sanity.cpp`. Zero GL anywhere in this slice.
- **D2D-2B** -- plumbing: `src/sprite_batch.hpp` gains the transformed-quad entry (pure,
  still zero GL), `src/draw2d.cpp` implements `set_camera`/`reset_camera` + the transform
  overload + the free helpers (delegating to `transform2d.hpp`), readback tests
  (`draw2d_camera_render_sanity`) + hostile additions (`draw2d_hostile_sanity.cpp`), version
  bump `project(VERSION 0.21.0)`.
- **DOC-D2D2** -- `docs/draw2d.md` gains the camera/transform sections and REFRAMES the D5
  coordinate contract as "the no-camera case" (see compatibility, section 2); CHANGELOG
  v0.21.0 draft; cross-refs; `tools/check_doc_line_refs.sh` re-run AFTER the code commits
  (the header edits WILL shift the `draw2d.hpp:NNN` citations the doc already carries).

**Does NOT ship (recorded, with trigger -- all confirmed by `D2D-BLOCO`'s closed order):**

- `draw_text` -- **D2D-TEXT**, its own wave, REQUIRES its own ADR first (font-engine tension).
- Filled rect / outline / line / layers / scissor / perf budget -- **D2D-3** (promoted, next).
- `texture_content_bbox` -- **D2D-BBOX** (after D2D-3, per the consumer's declared order).
- Tilemap/atlas helpers, text-in-world, parallax layers, camera shake/smoothing/bounds
  (lerp-follow, deadzone) -- gameplay-side conveniences, seeds with consumer trigger. Camera
  ANIMATION is the consumer's game loop's job; we ship the projection, not the cinematics.
- Camera getters on `Draw2d` (`camera()`/`has_camera()`) -- seed `SEED-D2D-CAMGETTER`; the
  consumer owns their `Camera2d` value and passes it to the pure helpers; a getter has zero
  demand today.
- A `begin(w, h, camera)` overload -- rejected: one way to set the camera (`set_camera`),
  not two.
- No measured performance number for the transform path (D2D-3's budget test covers it).

### 2. The leader's decision (options, not a fait accompli)

Two questions. My recommendation on each is explicit and argued; the decision is the
leader's; `software-architect` records the addendum only after. **Compatibility constraint
that governs both:** `docs/draw2d.md` (v0.20.0, published) promises "screen px, top-left,
y-down, the SAME space as `UiEvent`/`get_element_box`" for `draw_sprite`. Whatever wins must
keep every existing v0.20.0 call site compiling AND rendering identically.

#### Q1 -- how world space and screen space coexist in one API

| Option | What it means | Compatibility with v0.20.0 | Pros | Cons |
|---|---|---|---|---|
| **(a) `set_camera` switches the space of ALL subsequent draws; camera is a plain value from day one** | Sticky mode; `Camera2d{}` default would need to BE identity | Preserved only if the default-constructed camera is exactly identity, which forces the anchor answer (Q2) to top-left | One concept, no mode flag | Couples Q1 to Q2; a center-anchored camera (the friendlier one) has NO all-zero identity value, so "default value = old behaviour" becomes a lie waiting to happen |
| **(b) Two explicit spaces: `draw_sprite` (screen) + `draw_sprite_world` (world), or a per-draw flag** | Fully preserved (old method untouched) | Impossible to accidentally draw in the wrong space | DOUBLES the draw surface now and again at D2D-3 (every future primitive x2: `draw_rect_world`, `draw_line_world`...); the per-draw flag variant pollutes every signature; the consumer's own stated pain was "two draw paths in one frame is complexity without gain" -- this bakes the two paths into OUR api forever |
| **(c) RECOMMENDED -- optional camera, default OFF: the no-camera state IS screen space (the untouched v0.20.0 code path); `set_camera(Camera2d)` switches subsequent draws to world space; `reset_camera()` returns** | Strongest: with no camera set, the 4-arg `draw_sprite` executes the LITERAL v0.20.0 path (not "an identity transform that happens to be exact" -- the same code); pinned additionally by an exact-equivalence unit test | One draw surface; screen space is the honest default (nothing changes until you opt in); mid-frame HUD switch is `reset_camera()` + a second bracket, or just `set_camera` again -- and because the transform is CPU-side (D13), switching camera NEVER forces a GL flush and never breaks batching; frees Q2 to pick the friendlier anchor | The camera is stateful on `Draw2d` (sticky until reset) -- mitigated by fail-high validation at `set_camera` (D15) and by the state being trivially observable in tests |

**Recommendation: (c).** It is (a)'s single-concept simplicity plus (b)'s guarantee that
old code cannot change behaviour, without either's cost. The published D5 contract is then
REFRAMED, not broken: "screen px, top-left, y-down" becomes the documented no-camera case,
true by construction forever.

#### Q2 -- what `Camera2d.x/y` means (the anchor)

| Option | What it means | Pros | Cons |
|---|---|---|---|
| **(2a) RECOMMENDED -- center anchor: `(x, y)` is the world point at the CENTER of the viewport** | "Point the camera at the player" is `cam.x = player.x; cam.y = player.y` -- the natural follow-cam line of code; zoom and rotation pivot about the view center, which is what every consumer expects visually | Under Q1 (c) there is no need for `Camera2d{}` to be identity, so the friendlier anchor costs nothing | `Camera2d{}` (all defaults) is NOT the identity mapping (it centers world origin on screen) -- harmless under Q1 (c) because the default STATE is no-camera, but it must be documented loudly |
| **(2b) Top-left anchor: `(x, y)` is the world point at the viewport's top-left** | `Camera2d{}` is exactly identity (screen == world), enabling Q1 (a) | Every follow-cam consumer writes `cam.x = player.x - (w/2)/zoom` forever -- a permanent foot-gun; zoom anchored at a corner is visually surprising |

**Recommendation: (2a) center anchor.** The only argument for (2b) is enabling Q1 (a), and
Q1 (c) is better than (a) anyway.

**Not presented as options** (absolute or CTO-level, listed in section 3): radians for every
rotation (D14); zero third-party types on the public boundary (house absolute); the exact
projection formulas (D12 -- they follow mechanically from Q1+Q2 and are pinned by tests);
`camera_from_world_rect` as a generic convenience (D17, cuttable).

### 3. Design decisions (D12-D22) -- normative, conditional on section 2 = (c)+(2a)

Numbering continues onda 3's D1-D11, which remain fully in force (fail-high D10, GL-state
D9, batching D4, coordinates D5 -- now "the no-camera case" -- etc.).

**D12 -- The projection formulas, literal (single source: `src/transform2d.hpp`).**
With viewport `W x H`, viewport center `c = (W/2, H/2)`, camera `cam = {x, y, zoom, rotation}`
(center anchor, Q2 (2a)), and `rot(v, t) = (v.x*cos(t) - v.y*sin(t), v.x*sin(t) + v.y*cos(t))`
(in y-down screen space a positive `t` rotates clockwise on screen):

```
world_to_screen(cam, W, H, p) = rot((p - (cam.x, cam.y)) * cam.zoom, -cam.rotation) + c
screen_to_world(cam, W, H, s) = rot(s - c, +cam.rotation) / cam.zoom + (cam.x, cam.y)
```

Semantics: `cam.rotation` is the camera's OWN orientation -- a positive rotation turns the
camera clockwise, so the world appears to rotate counter-clockwise on screen. `cam.zoom` is
screen px per world unit (`zoom 2` = magnified 2x). Both functions are implemented ONCE and
consumed by BOTH the draw path and the public helpers -- divergence is structurally
impossible, and the formulas above go verbatim into `docs/draw2d.md` AND into
computed-expected-value tests (risk 2 of section 0).

**D13 -- Camera and transforms are CPU-side, pure, pre-batcher. The GL layer is untouched.**
The existing shader (`uTargetSize` ortho, y-flip, D8 tint) does not change; no new uniform,
no new GL state (D9's touched-state list in `docs/draw2d.md` stays exactly as published).
`transform2d.hpp` computes the four screen-space corner points on the CPU; the batcher
receives corners and emits vertices. Consequences, both deliberate: (1) changing the camera
or per-sprite transform mid-bracket NEVER forces a flush -- batching stays purely
per-texture (D4 untouched); (2) the entire new surface is headless-unit-testable, which is
what the program's "math is pure/unit-able" line demanded.

**D14 -- Rotation is radians, everywhere.** `Camera2d.rotation` and
`SpriteTransform.rotation` both. Matches `<cmath>`, matches every C++ math convention;
degrees are a documentation footnote (`deg * pi / 180`), not an API variant. Any finite
value is legal -- `rotation > 2*pi` wraps naturally through `sin`/`cos` (pinned by test:
`t` and `t + 2*pi` produce the same mapping within tolerance).

**D15 -- `set_camera` validation (fail-high, the set_dp_ratio idiom).** Any non-finite
member, or `zoom <= 0`, rejects the call: previous camera state (including "no camera")
is KEPT, one dedup'd log line. `zoom < 0` is NOT a mirror idiom (flip flags exist for
that); `zoom == 0` would collapse the world and make `screen_to_world` divide by zero.
Rotation: any finite value accepted (D14). `reset_camera()` always succeeds, is idempotent,
and both methods are safe no-ops on a moved-from/uninitialized `Draw2d` (the null-safe
contract that already burned us once -- cppcheck nullPointer, onda 3 -- applies to every
new public method).

**D16 -- Pure helper totality.** `world_to_screen`/`screen_to_world` are pure free
functions: they cannot log. On invalid input (any non-finite member of `cam` or the point,
`cam.zoom <= 0`, `viewport_w <= 0 || viewport_h <= 0`) they return the INPUT POINT
UNCHANGED -- a deterministic, documented, testable fail-high fallback (never NaN out, never
UB). The doc says so explicitly; the hostile unit tests pin it.

**D17 -- `camera_from_world_rect` (generic convenience, THE cuttable piece).**
`Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h)`:
returns the camera that shows `world_rect` fitted by WIDTH (`zoom = viewport_w /
world_rect.w`, center = rect center); vertical coverage follows the viewport's aspect
(documented -- callers wanting letterbox-exact fit compose with `UiLayer::set_viewport`,
which already exists). Fail-high: non-finite members, `world_rect.w <= 0 || world_rect.h
<= 0`, or non-positive viewport return `Camera2d{}` (documented). Rationale: "fit the view
to a world rect" is a textbook camera constructor every rect-thinking consumer needs (ours
maps from it in one line), it is pure/testable, and it is NOT consumer-shaped API (no
`begin_frame` mirror). If the leader wants this wave leaner, this is the one piece that
cuts without damaging the wave's claim.

**D18 -- `SpriteTransform` semantics (per-sprite, orthogonal to the camera, applies in
BOTH spaces).** Fields: `origin_x/origin_y` (pivot, NORMALIZED within `dst`, default
`0.5/0.5` = center), `rotation` (radians about the pivot, default 0), `scale_x/scale_y`
(about the pivot, default 1; negative scale is legal -- it mirrors geometry; winding is
irrelevant because D9 explicitly disables cull), `flip_h/flip_v` (bools, default false --
implemented as UV swaps on the RESOLVED `src_px` sub-rect, so an atlas frame flips within
itself, never bleeding neighbouring frames). Corner math, literal order:

```
pivot P = (dst.x + origin_x*dst.w, dst.y + origin_y*dst.h)
corner' = P + rot((corner - P) * (scale_x, scale_y), rotation)     for each of TL,TR,BR,BL
```

With a camera set, `dst` and the resulting corners are WORLD units and each corner then
passes through D12's `world_to_screen`; with no camera they are screen px directly. One
composition story, no special cases.

**D19 -- Batcher entry for transformed quads; the v0.20.0 path stays verbatim.**
`SpriteBatch` gains `draw_quad(texture_id, tex_w, tex_h, corners[4], src_px, tint)`
(pure): same bracket/capacity/texture-change policy (D4 untouched), same src_px
sentinel+clamp (D5), same tint clamp (D8), vertices carry the caller's corners instead of
axis-aligned expansion. The EXISTING `draw_sprite` batcher path is NOT rewritten: the
public 4-arg `Draw2d::draw_sprite` with no camera set keeps calling it unchanged (Q1 (c)'s
compatibility guarantee is the absence of a diff, not a proof of equivalence). The overlap
between the two paths is pinned by an EXACT pure unit test: identity transform + no camera
through the quad path produces bit-identical vertices to the old path (exact float
equality is legitimate here -- CPU-deterministic, and identity ops `*1.0f`, `+0.0f`,
`cos(0)=1`, `sin(0)=0` are IEEE-exact).

**D20 -- Fail-high input surface of the new API (D10 extended, domino rule up front).**
Validated BEFORE any arithmetic, in this order: `SpriteTransform` members all finite
(NaN/Inf anywhere -> skip + dedup'd log, same `NonFinite` class as dst/src/tint);
`dst.w <= 0 || dst.h <= 0` stays a silent legal no-op (unchanged). AFTER the corner math,
one more guard: any non-finite computed corner (overflow -- e.g. huge zoom times huge
coordinate multiplying to Inf) -> skip + dedup'd log. That post-math check is NEW and has
no v0.20.0 twin because the old path cannot overflow (no multiplication); the reviewer's
domino sweep must confirm the twin ANALYSIS, not blindly demand a twin guard. Camera
validation happens once at `set_camera` (D15), not per draw -- a stored camera is valid by
construction.

**D21 -- Module/CI mechanics unchanged.** No new CMake flag, no new module: this is the
same `GLINTFX_MODULE_DRAW2D` atom growing its contracted D2D-2 surface (ADR-0017 scope
line said so). The OFF gate now also removes the new symbols (free helpers are DECLARED in
`draw2d.hpp`, DEFINED in `draw2d.cpp` -- deliberately not header-inline, so `nm` proof of
removal keeps working); ADR-0013's raw-GL symbol gate unchanged. MSVC: new code is
`<cmath>`-only, no Unix headers, no `min`/`max` collisions (NOMINMAX is directory-scope),
compiles in the embed job as before.

**D22 -- Lifetime/state.** The camera is part of `Draw2d`'s state: `shutdown()` resets it
to no-camera (a re-`init()`ed instance starts in screen space, matching a fresh one);
move transfers it with the pImpl. Documented in the header.

### 4. Public API contract (literal -- additions to `include/glintfx/draw2d.hpp`)

```cpp
// All in namespace glintfx; EN/PT doc-comments per house style; zero third-party types.

struct Vec2F { float x = 0.f; float y = 0.f; };

struct Camera2d {              // world camera, CENTER anchor (Q2 (2a))
  float x = 0.f;               // world point shown at the viewport center
  float y = 0.f;
  float zoom = 1.f;            // screen px per world unit; must be > 0 (D15)
  float rotation = 0.f;        // radians; camera's own orientation (D12/D14)
};

struct SpriteTransform {       // per-sprite, applies in BOTH spaces (D18)
  float origin_x = 0.5f;       // pivot, normalized within dst (0.5,0.5 = center)
  float origin_y = 0.5f;
  float rotation = 0.f;        // radians about the pivot
  float scale_x = 1.f;         // about the pivot; negative = mirror (finite, any sign)
  float scale_y = 1.f;
  bool flip_h = false;         // UV swap on the resolved src_px sub-rect
  bool flip_v = false;
};

// Pure, total helpers (D12/D16) -- defined in draw2d.cpp, removed when the module is OFF.
Vec2F world_to_screen(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F world);
Vec2F screen_to_world(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F screen);
Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h); // D17

class Draw2d {
  // ... everything shipped in v0.20.0, unchanged ...

  // Q1 (c): default state is NO camera = screen space = the untouched v0.20.0 behaviour.
  void set_camera(const Camera2d& camera);   // sticky; fail-high keeps previous (D15)
  void reset_camera();                        // back to screen space; idempotent

  // D18 overload -- transforms; dst is world units when a camera is set, screen px otherwise.
  void draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px,
                   const ColorF& tint, const SpriteTransform& transform);
};
```

No method shipped in v0.20.0 changes signature or behaviour-without-camera. `glintfx.hpp`'s
guarded include is already in place (no change).

### 5. Testing (headless-testable vs not, downgrades declared)

**Canonical headless line (copy WHOLE, house rule):**
`env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest --test-dir <build> --output-on-failure -j1`

1. **Pure units (deterministic, no display, the bulk) -- NEW `transform2d_sanity`:**
   - D12 computed-value tests: hand-computed expected screen points for cameras with
     offset-only, zoom-only (2x and 0.5x), rotation-only (pi/2 exact values), and a
     combined case -- the conversion-formula delta tests (risk 2).
   - Round-trip property: `screen_to_world(world_to_screen(p)) == p` within tolerance over
     a grid of points x a grid of cameras (offsets, zooms 0.1..10, rotations incl. > 2*pi).
   - Rotation wrap: `t` vs `t + 2*pi` equal within tolerance (D14).
   - D16 totality: zoom 0, zoom negative, NaN/Inf camera members, NaN point, non-positive
     viewport -- helpers return the input unchanged, never NaN out.
   - D17: rect-fit zoom/center computed values; degenerate/hostile rects return `Camera2d{}`.
   - D18 corner math: rotate pi/2 about center permutes corners exactly; pivot (0,0) vs
     (1,1) computed values; scale about pivot; negative scale mirrors; flip_h/flip_v swap
     the RESOLVED atlas-frame UVs (not the whole texture's).
   - D19 exact equivalence: identity transform + no camera through the quad path ==
     bit-identical vertices to the v0.20.0 path (the compatibility pin).
   - D20: NaN/Inf in every `SpriteTransform` field rejected before math; overflow camera
     (zoom 1e30 x coord 1e30) rejected by the post-math corner guard.
2. **Pixel-readback under Xvfb/llvmpipe (statistical, `render_sanity` class -- NEVER exact)
   -- NEW `draw2d_camera_render_sanity`:**
   - Camera offset: sprite drawn at a world position lands in the expected screen region
     (region-mean within tolerance) for a camera panned away and back.
   - Zoom 2x: a quadrant of a 2x2 checker texture covers the expected doubled region.
   - Rotation pi/2 with an asymmetric two-colour texture: dominant colours of the four
     screen quadrants permute as computed.
   - `SpriteTransform` rotation pi/2 (no camera): same permutation check in screen space.
   - Mid-bracket `set_camera` switch (world sprites then `reset_camera()` + HUD sprite):
     both land where computed, and the test asserts the flush count story indirectly by
     drawing both from ONE texture (same-texture batching must survive the switch, D13).
   - v0.20.0 regression: the EXISTING `draw2d_render_sanity` and `draw2d_ui_coexist_sanity`
     run UNTOUCHED -- their staying green is the published-contract guard.
3. **Hostile -- extended `draw2d_hostile_sanity`:** `set_camera` with each invalid member
   (state provably kept: draw before/after lands identically); `set_camera`/`reset_camera`
   on never-init / after-shutdown / moved-from instances (safe no-op, D15/D22); the
   transform overload with hostile fields; shutdown-then-init camera reset (D22). ASan leg.
4. **Declared downgrades (limits stated instead of faked):**
   - No pixel-exact GPU goldens (llvmpipe non-determinism; `golden_test` stays opt-in).
     Exactness claims live ONLY in the pure CPU tests, where they are legitimate.
   - MSVC = compile+link gate only (embed job); no Windows runtime execution in CI.
   - No performance measurement of the transform path (D2D-3's budget test, leader-closed).
   - Camera rotation composed with per-sprite rotation is verified by pure math tests +
     one statistical readback; exhaustive visual verification of arbitrary angle
     combinations is not headless-provable and is not claimed.

### 6. House gates (all mandatory)

- Build dirs in **`/var/tmp`** (`/var/tmp/glx-onda4*`) with `export TMPDIR=/var/tmp`;
  **one heavy build at a time**; ASan leg before review sign-off.
- Suite green in BOTH configs (GLFW=ON full; GLFW=OFF embed), canonical headless line
  WHOLE, serial (`-j1`).
- `tools/preci.sh` green; **`tools/check_doc_line_refs.sh` re-run AFTER any commit that
  shifts lines** -- `docs/draw2d.md` already cites `draw2d.hpp:NNN` line numbers that the
  D2D-2A header edits WILL displace (the onda-3 postmortem named this exact rot).
- Symbol gates: ADR-0013 `nm` (zero raw `gl[A-Z]` B/D symbols) unchanged; with
  `GLINTFX_MODULE_DRAW2D=OFF`, `nm` shows zero draw2d symbols INCLUDING the new free
  helpers (D21 -- the reviewer runs both).
- MSVC-safe: no raw `-Wall`/`-Wextra`, no unguarded `min`/`max`, no
  `filesystem::path::c_str()`, no Unix-only header -- all new/touched files compile in the
  MSVC embed job.
- Implementer != reviewer; adversarial review EXECUTES (mutation on the canonical CMake
  build, NEVER mixing ASan/non-ASan objects).
- Push / merge / tag: **only with explicit leader authorization in-context.** Commits cite
  the slice ID and touch `TODO.md` status in the same commit (`🔍` on delivery, never `✅`).
- Wave close: `project(VERSION 0.21.0)`, CHANGELOG entry, consumer relay block for the
  leader (the GusWorld mapping recipe: their `begin_frame(camera_world, w, h)` becomes
  `d2d.set_camera(camera_from_world_rect(camera_world, w, h)); d2d.begin(w, h);` -- one
  line, no API mirroring needed).

### 7. Agent map (4 in the implementation fan-out; addendum precedes it)

| # | Agent | Slice | Owns (files) | Runs |
|---|---|---|---|---|
| 0 | `software-architect` | ADR17-ADD | `docs/adr/0017-draw2d-module.md` (addendum section: Q1/Q2 decision + D12-D22 summary, links this plan) | first, AFTER the leader decides section 2 |
| 1 | `backend-engineer` (implementer A) | D2D-2A | NEW `glintfx/src/transform2d.hpp`, `glintfx/include/glintfx/draw2d.hpp` (section-4 additions, LITERAL), NEW `glintfx/tests/transform2d_sanity.cpp`, `glintfx/tests/CMakeLists.txt` (their block) | after addendum |
| 2 | `backend-engineer` (implementer B, DIFFERENT agent) | D2D-2B | `glintfx/src/sprite_batch.hpp` (draw_quad entry), `glintfx/src/draw2d.cpp` (set_camera/reset_camera/overload/helpers), NEW `glintfx/tests/draw2d_camera_render_sanity.cpp`, `glintfx/tests/draw2d_hostile_sanity.cpp` (extend), `glintfx/tests/CMakeLists.txt` (their block), `glintfx/CMakeLists.txt` (version bump + source list) | after A (depends on header + pure math; `tests/CMakeLists.txt` collision) |
| 3 | `technical-writer` | DOC-D2D2 | `docs/draw2d.md` (camera/transform sections + D5 reframe + shifted line-refs), `CHANGELOG.md` (v0.21.0 draft), `docs/embed-integration.md` (one cross-ref line if needed) | after B (needs real signatures/lines) |
| 4 | `qa-engineer` (adversarial reviewer, EXECUTES) | whole wave | read+run everything; section 8.4 brief | last |

Orchestrator (main thread) re-verifies: clean build from scratch in `/var/tmp` (both
configs, sequentially), spot-check of claims (file:line), `git log` for unauthorized
pushes, the two `nm` gates, suite green, doc-line-ref gate, then takes the wave to the
leader.

### 8. Briefs (ready to paste)

#### 8.1 Brief -- `software-architect` -- ADR17-ADD

```
Você é software-architect no glintfx (repo loucura_c_asm). NÃO faça push/merge. Commit local
Conventional pt-br citando ADR17-ADD. O líder JÁ DECIDIU a seção 2 do plano
glintfx/docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md (o orquestrador te informa
a decisão Q1/Q2). Grave um ADENDO ao docs/adr/0017-draw2d-module.md (precedente da casa:
ADR-0008 adendo F3) -- seção nova "Addendum (2026-07-23): D2D-2 coordinate model", bilíngue
EN→PT, curta: a decisão Q1 (modelo de espaço) e Q2 (âncora), as alternativas rejeitadas em
1 linha cada, e um resumo de 5 linhas das decisões D12-D22 com link pro plano. NÃO
reescreva o corpo do ADR; só o adendo + uma linha no Relates-to. Sem em-dash.
```

#### 8.2 Brief -- implementer A (`backend-engineer`) -- D2D-2A

```
Você é backend-engineer no glintfx (repo loucura_c_asm, pasta glintfx/). NÃO faça push nem
merge. Commits locais Conventional pt-br citando D2D-2A + Status do TODO.md (→ 🔍) no mesmo
commit.

Leia ANTES: o plano glintfx/docs/superpowers/plans/2026-07-23-onda4-draw2d-camera.md
(seções 3-4: D12-D22 e o contrato da seção 4 são LITERAIS, não invente assinatura);
docs/adr/0017-draw2d-module.md (com o adendo novo); glintfx/include/glintfx/draw2d.hpp e
glintfx/src/sprite_batch.hpp INTEIROS (o estilo de doc-comment bilíngue e a disciplina D10
que você vai estender); AGENTS.md (gotchas).

Entregue sob TDD (red/green/refactor), ZERO GL nesta fatia:
1. NEW glintfx/src/transform2d.hpp: header interno PURO com as fórmulas D12 AO PÉ DA LETRA
   (world_to_screen/screen_to_world, fonte ÚNICA -- o draw path e os helpers públicos vão
   consumir as MESMAS funções), a geração de cantos D18 (pivô normalizado, scale→rotate
   sobre o pivô, ordem literal do plano), validação D20 (todo campo finito ANTES de
   qualquer conta; guard pós-conta de canto não-finito p/ overflow), totalidade D16
   (input inválido → retorna o ponto de entrada inalterado) e D17
   (camera_from_world_rect, fit por largura, hostil → Camera2d{}). SPDX + doc bilíngue.
2. include/glintfx/draw2d.hpp: EXATAMENTE a seção 4 do plano (Vec2F, Camera2d,
   SpriteTransform, 3 free functions declaradas, set_camera/reset_camera/overload de
   draw_sprite declarados). REFRAME do comentário D5 do cabeçalho: "px de tela" vira "o
   caso sem câmera" (Q1 (c)). NÃO implemente nada de .cpp (fatia do implementer B).
3. NEW glintfx/tests/transform2d_sanity.cpp: TODOS os itens 1 da seção 5 do plano --
   valores computados à mão das fórmulas D12 (offset/zoom/rotação pi/2/combinado),
   round-trip world→screen→world em grade de pontos × câmeras, wrap 2π, totalidade D16
   (zoom 0/negativo/NaN/Inf/viewport ≤0), D17, matemática de cantos D18 (rotação pi/2
   permuta cantos, pivôs, scale negativo espelha, flips trocam UV do sub-rect RESOLVIDO),
   equivalência EXATA D19 (identidade == caminho v0.20.0, igualdade de float exata -- é
   legítima aqui), overflow D20. Bloco próprio em tests/CMakeLists.txt.
Gates: build em /var/tmp/glx-onda4 com TMPDIR=/var/tmp (1 build pesado por vez); as DUAS
configs verdes com a linha headless canônica INTEIRA
(env -u WAYLAND_DISPLAY XDG_RUNTIME_DIR=/var/tmp/fake_xdg_runtime xvfb-run -a ctest ... -j1);
tools/preci.sh limpo; MSVC-safe (só <cmath>, nada Unix-only, nada de min/max cru). Seus
edits no draw2d.hpp DESLOCAM linhas citadas por docs/draw2d.md -- NÃO conserte o doc (fatia
do technical-writer), mas LISTE no seu relatório quais citações quebraram
(tools/check_doc_line_refs.sh te diz). Relatório final com file:line + saída do ctest.
```

#### 8.3 Brief -- implementer B (`backend-engineer`, agente DIFERENTE do A) -- D2D-2B

```
Você é backend-engineer no glintfx. NÃO faça push/merge. Commits locais Conventional pt-br
citando D2D-2B + Status do TODO.md (→ 🔍). Rode APÓS o implementer A (você depende de
transform2d.hpp e do header público novo; colide em tests/CMakeLists.txt).

Leia ANTES: o plano (seções 3-6; D12-D22 normativos); glintfx/src/transform2d.hpp (entregue
pelo A -- consuma as funções DELE, não reimplemente NENHUMA fórmula); glintfx/src/draw2d.cpp
e glintfx/src/sprite_batch.hpp inteiros; docs/adr/0017 (adendo); AGENTS.md (gotchas:
premultiplied, FBO 0, MSAA off, llvmpipe).

Entregue sob TDD, na ordem:
1. glintfx/src/sprite_batch.hpp: entrada draw_quad(texture_id, tex_w, tex_h, corners[4],
   src_px, tint) PURA (D19): mesma política de bracket/capacidade/troca-de-textura D4,
   mesmo sentinel+clamp de src_px D5, mesmo clamp de tint D8; vértices carregam os cantos
   do chamador. O caminho draw_sprite EXISTENTE do batcher NÃO muda (a garantia de
   compatibilidade Q1 (c) é a AUSÊNCIA de diff nesse caminho).
2. glintfx/src/draw2d.cpp: set_camera (validação D15: membro não-finito ou zoom<=0 →
   MANTÉM o estado anterior + log dedup'd; null-safe em movido-de -- a lição do cppcheck
   da onda 3 vale pra TODO método novo), reset_camera (idempotente), estado de câmera no
   Impl (shutdown() reseta pra sem-câmera, D22), overload de draw_sprite com
   SpriteTransform (valida D20 ANTES de qualquer conta; cantos via transform2d.hpp do A;
   com câmera → world_to_screen POR CANTO; guard pós-conta de canto não-finito; entrega ao
   draw_quad do batcher), e as 3 free functions públicas DEFINIDAS aqui delegando ao
   transform2d.hpp (D21: fora do header, pra prova de remoção por nm continuar valendo).
   O caminho de 4 args SEM câmera continua chamando o caminho antigo do batcher, verbatim.
3. Testes: NEW glintfx/tests/draw2d_camera_render_sanity.cpp (itens 2 da seção 5:
   offset/zoom 2x/rotação pi/2 com textura assimétrica/transform pi/2 sem câmera/troca de
   câmera no MEIO do bracket com UMA textura só -- tudo ESTATÍSTICO, região-média com
   tolerância, NUNCA pixel-exato); EXTEND glintfx/tests/draw2d_hostile_sanity.cpp (itens 3
   da seção 5: set_camera hostil com estado provadamente mantido, chamadas em
   never-init/pós-shutdown/movido-de, overload com campos hostis, reset pós-shutdown).
4. glintfx/CMakeLists.txt: project(VERSION 0.21.0) + transform2d.hpp na lista de fontes do
   módulo (se a convenção da lista pedir headers).
Gates: build em /var/tmp/glx-onda4 (TMPDIR=/var/tmp, 1 pesado por vez); DUAS configs verdes
+ leg ASan; linha headless canônica INTEIRA; preci.sh + check_encapsulation.sh limpos; os
DOIS nm (ADR-0013 zero gl cru; GLINTFX_MODULE_DRAW2D=OFF remove TODOS os símbolos novos,
inclusive as free functions -- rode e cole no relatório); MSVC-safe. Os testes EXISTENTES
draw2d_render_sanity/draw2d_ui_coexist_sanity ficam INTOCADOS e verdes (guarda do contrato
publicado). Relatório final com file:line + ctest + os dois nm.
```

#### 8.4 Brief -- `technical-writer` -- DOC-D2D2

```
Você é technical-writer no glintfx. NÃO faça push/merge. Commit local citando DOC-D2D2.
Rode após o implementer B (precisa das assinaturas e linhas reais). Docs bilíngues (EN
primeiro). Sem em-dash em prosa nova.
1. docs/draw2d.md: (a) REFRAME da seção de coordenadas D5 -- "px de tela, top-left, y-down"
   agora é o CASO SEM CÂMERA (o default, comportamento v0.20.0 intocado; Q1 (c) do plano
   onda4); (b) seção nova de câmera: Camera2d (âncora CENTRO, zoom = px por unidade de
   mundo, rotação em RADIANOS, orientação da própria câmera), as fórmulas D12 VERBATIM do
   plano, set_camera/reset_camera (validação D15, estado mantido em input hostil, câmera é
   estado sticky, trocar no meio do bracket NÃO quebra batching -- D13), helpers puros
   world_to_screen/screen_to_world (totalidade D16: input inválido devolve o ponto
   inalterado) e camera_from_world_rect (fit por largura, D17); (c) seção nova de
   SpriteTransform (pivô normalizado default centro, ordem scale→rotate do D18, flips como
   troca de UV do sub-rect resolvido, scale negativo legal, vale nos DOIS espaços); (d)
   tabela de API atualizada com as linhas novas; (e) limites declarados atualizados (sem
   texto até D2D-TEXT, sem primitivas/perf até D2D-3, chega de câmera: shake/smoothing/
   bounds são do consumidor); (f) TODAS as citações draw2d.hpp:NNN deslocadas pelos edits
   -- rode tools/check_doc_line_refs.sh até 0 falha ANTES do commit e de novo DEPOIS.
2. CHANGELOG.md: rascunho v0.21.0 (Added: D2D-2A/D2D-2B/ADR17-ADD; downgrades declarados).
3. docs/embed-integration.md: linha de cross-ref na seção Draw2D existente, se couber.
Nada de claim não verificável; cite file:line real.
```

#### 8.5 Brief -- `qa-engineer` (review adversarial, EXECUTA) -- onda inteira

```
Você é qa-engineer, revisor ADVERSARIAL da onda D2D-2A/D2D-2B/DOC-D2D2 do glintfx. Você
NÃO implementou nada disto (implementer ≠ reviewer). NÃO faça push/merge.
Método: você EXECUTA, não só lê. Build limpo do zero em /var/tmp/glx-onda4-review
(TMPDIR=/var/tmp, 1 build pesado por vez), DUAS configs + leg ASan -- mutation SEMPRE no
build canônico do CMake, NUNCA misturando objetos ASan/não-ASan.
1. Mutation dirigido às FÓRMULAS (a classe de bug Critical da casa): troque o sinal do
   sin na rotação (o teste de valor computado pega?); troque -cam.rotation por
   +cam.rotation no world_to_screen (o round-trip pega? e o readback de rotação?); troque
   a ordem scale→rotate do D18 (pega?); quebre o wrap 2π; troque zoom por 1/zoom no
   screen_to_world; inverta um flip; quebre a equivalência D19 (mude 1 float do caminho
   novo -- o teste EXATO pega?); remova o guard pós-conta de canto não-finito (o teste de
   overflow pega?).
2. Hostil: set_camera com NaN/Inf/zoom 0/zoom negativo → prove que o estado anterior FOI
   mantido (desenhe antes/depois e compare); helpers com câmera hostil devolvem o ponto
   inalterado; overload com todo campo hostil; moved-from em TODO método novo (a lição do
   null-deref que passou pelo review da onda 3 -- rode cppcheck também); stream de 100k
   draws COM transform (memória segue limitada?); câmera trocada 1000× num bracket (não
   pode gerar flush por troca -- D13: conte os flushes via o batcher puro).
3. Compatibilidade (o risco central): draw2d_render_sanity e draw2d_ui_coexist_sanity da
   v0.20.0 INTOCADOS e verdes; git diff do caminho 4-args sem câmera == vazio em
   sprite_batch.hpp (o caminho antigo não pode ter mudado); o teste de equivalência D19
   passa com igualdade EXATA.
4. Dominó: todo guard novo tem gêmeo ou análise-de-gêmeo (D20 nomeia o caso do guard
   pós-conta SEM gêmeo no caminho velho -- confirme a análise em vez de exigir o guard
   cego)? NaN checado em Camera2d E SpriteTransform E nos helpers E no rect do D17? Os
   DOIS nm (ADR-0013; flag OFF remove free functions novas -- rode você mesmo). MSVC-safe
   nos arquivos tocados.
5. Docs: fórmulas do draw2d.md batem VERBATIM com transform2d.hpp? check_doc_line_refs.sh
   0 falha? Cada claim verificável?
Relatório: Critical/Major/Minor com file:line + comando de repro. Critical/Major bloqueiam
a tag. O orquestrador vai re-verificar suas claims -- seja preciso.
```

### 9. TODO.md rows (for the orchestrator to insert)

| ID | Item | Onda | Esforço | Status |
|---|---|---|---|---|
| ADR17-ADD | Adendo ao ADR-0017: modelo de espaço da câmera (Q1/Q2, decisão do líder → gravação pelo software-architect) | 4 | P | ⬜ |
| D2D-2A | Matemática pura D2D-2: transform2d.hpp (fórmulas D12/D18, fonte única) + tipos públicos (Vec2F/Camera2d/SpriteTransform/helpers) + transform2d_sanity | 4 | P/M | ⬜ |
| D2D-2B | Plumbing D2D-2: draw_quad no batcher, set_camera/reset_camera/overload, free helpers, readback estatístico + hostil, VERSION 0.21.0 | 4 | M | ⬜ |
| DOC-D2D2 | docs/draw2d.md (reframe D5 + câmera + transforms) + CHANGELOG v0.21.0 + line-refs re-verificados | 4 | P | ⬜ |

INBOX seeds to add: `SEED-D2D-CAMGETTER` (camera()/has_camera() getters; trigger: consumer
ask), `SEED-D2D-CAMANIM` (camera smoothing/shake/bounds helpers; trigger: consumer ask --
recorded as gameplay-side by default), `SEED-D2D-LETTERBOX` (aspect-exact rect fit beyond
D17's width-fit; trigger: consumer ask).

### 10. CTO counter-arguments (honest record)

1. **Is D2D-2 as scoped here still M? Yes.** The GL layer does not change at all (D13) --
   the wave is pure math + plumbing + tests, exactly what the program priced as "low-med,
   math is pure/unit-able". The one piece I added beyond the program's scope line is D17
   (`camera_from_world_rect`), it is P-sized, generic, and explicitly cuttable.
2. **Why not give the consumer their `begin_frame(world_rect, w, h)` directly?** Because
   that is their renderer's shape, not a camera model -- axis 2 of `D2D-BLOCO`. The generic
   camera plus the D17 helper reaches the same one-line adoption without freezing their
   API shape into ours.
3. **The published D5 contract is reframed, not broken -- but it IS a semantic evolution
   of a published doc,** which is why Q1 goes to the leader instead of being decided here.
   Under (c) the guarantee is structural: no camera set means the v0.20.0 code path runs
   unchanged, so old consumers cannot observe this wave at all.
4. **Two vertex paths (old axis-aligned + new quad) is a real divergence risk.** Accepted
   deliberately: rewriting the old path through the new math would trade a
   provable-by-absence-of-diff compatibility guarantee for a provable-by-test one. We keep
   BOTH nets anyway (untouched old path AND the D19 exact-equivalence test); D2D-3 may
   unify the paths once the transform path has a shipped wave of soak behind it.
5. **The post-math corner-finiteness guard (D20) has no v0.20.0 twin** -- the domino sweep
   will flag that asymmetry; the plan pre-answers it (the old path has no multiplication
   that can overflow) so the reviewer confirms the analysis instead of filing a false
   positive.

---

## Part B -- PT

### 0. O que é a onda e os dois riscos centrais

O D2D-2 dá ao átomo Draw2D o que o torna adotável como renderer de JOGO: transforms
por-sprite (pivô, rotação, escala, flip H/V) e câmera de mundo (`Camera2d`; `set_camera`;
helpers puros `world_to_screen`/`screen_to_world`). **Risco 1, nomeado:** a onda querer
virar "implementar o renderer do GusWorld" -- os 19 usos de `begin_frame(camera_world,...)`
deles definem a PRIORIDADE (eixo 1 do `D2D-BLOCO`), mas o ESCOPO é distribuição geral
(eixo 2): câmera genérica como value type, nada de espelhar a `IRenderer` deles; o mapeamento
pro modelo deles vira UMA linha no bloco de relay (seção 6), não API. **Risco 2, nomeado:**
projeção mundo↔tela É fórmula de conversão -- a classe de bug silencioso que já nos custou
um Critical. Antídoto: fórmulas literais no plano (D12), implementadas UMA vez num header
puro (fonte única pro draw path e pros helpers -- divergência estruturalmente impossível),
fixadas por teste de valor computado + round-trip.

### 1. Escopo

**Entra:** ADR17-ADD (adendo ao ADR-0017 gravado pelo `software-architect` APÓS a decisão do
líder na seção 2; precedente: adendo F3 do ADR-0008); D2D-2A (matemática pura + tipos
públicos + suíte unitária, zero GL); D2D-2B (entrada de quad no batcher, plumbing,
readback estatístico, hostil, bump 0.21.0); DOC-D2D2 (doc + CHANGELOG + line-refs).

**NÃO entra (com gatilho):** `draw_text` (D2D-TEXT, exige ADR próprio); primitivas/layers/
scissor/perf (D2D-3, promovida, é a próxima); `texture_content_bbox` (D2D-BBOX);
suavização/shake/limites de câmera (é do game loop do consumidor -- seed); getters de câmera
(seed); overload `begin(w,h,camera)` (rejeitado: um jeito só de setar câmera).

### 2. A decisão do líder (resumo; tabelas completas na Parte A)

Restrição que governa tudo: o `docs/draw2d.md` da v0.20.0 PUBLICOU "px de tela, top-left,
y-down, mesmo espaço do UiEvent" -- o que vencer aqui precisa manter todo call site v0.20.0
compilando E renderizando idêntico.

- **Q1 -- como mundo e tela convivem:** (a) `set_camera` muda o espaço de todos os draws,
  câmera-valor desde o início (obriga o default `Camera2d{}` a ser identidade → amarra a
  Q2 no pior âncora) · (b) dois métodos/flag por-draw (dobra a superfície agora e de novo
  no D2D-3; o próprio consumidor chamou dois caminhos num frame de "complexidade sem
  ganho") · **(c) RECOMENDO: câmera OPCIONAL, default DESLIGADA -- o estado sem-câmera É o
  espaço de tela, rodando o caminho v0.20.0 LITERALMENTE intocado; `set_camera` liga o
  mundo, `reset_camera()` volta.** Compatibilidade mais forte possível (ausência de diff,
  não prova de equivalência -- e o teste exato D19 ainda fixa por cima); um caminho de draw
  só; trocar câmera no meio do frame não quebra batching (transform é CPU-side, D13).
- **Q2 -- âncora do Camera2d:** **(2a) RECOMENDO: CENTRO** (câmera "olha para" `(x,y)`;
  follow-cam vira `cam.x = player.x`, zoom/rotação pivotam no centro da vista -- o que todo
  consumidor espera; sob a Q1 (c) o default não precisar ser identidade não custa nada) ·
  (2b) topo-esquerda (identidade com zeros, mas condena todo follow-cam a
  `player.x - (w/2)/zoom` pra sempre).

### 3. Decisões de desenho (resumo -- normativo completo na Parte A, D12-D22)

- **D12**: fórmulas literais (âncora centro, `rot` em espaço y-down, `-cam.rotation` no
  mundo→tela; zoom = px por unidade de mundo; rotação = orientação da própria câmera).
  Fonte ÚNICA em `src/transform2d.hpp`, verbatim no doc E nos testes de valor computado.
- **D13**: câmera e transforms são CPU-side, puros, ANTES do batcher -- o shader e a lista
  de estado GL D9 publicada NÃO mudam; trocar câmera nunca força flush.
- **D14**: radianos em toda rotação; valor finito qualquer é legal, wrap por sin/cos
  (testado t vs t+2π).
- **D15**: `set_camera` fail-high (não-finito ou zoom≤0 → MANTÉM estado anterior + log
  dedup'd, idioma do set_dp_ratio); todo método novo null-safe em movido-de (lição do
  cppcheck da onda 3).
- **D16**: helpers puros são TOTAIS -- input inválido devolve o ponto de entrada inalterado
  (determinístico, documentado, testado; nunca NaN pra fora).
- **D17**: `camera_from_world_rect` (fit por largura, hostil → `Camera2d{}`) -- a peça
  CORTÁVEL; genérica, P, e é o que torna a adoção do consumidor uma linha.
- **D18**: `SpriteTransform` (pivô normalizado default centro; ordem scale→rotate sobre o
  pivô; flips = troca de UV do sub-rect resolvido; scale negativo legal, cull já OFF por
  D9); vale nos DOIS espaços; com câmera, cada canto passa pelo D12.
- **D19**: batcher ganha `draw_quad` puro (mesma política D4/D5/D8); o caminho antigo NÃO é
  reescrito (compat por ausência de diff) + teste de equivalência EXATA identidade == v0.20.0.
- **D20**: superfície nova fail-high ANTES de qualquer conta + guard PÓS-conta de canto
  não-finito (overflow de zoom×coordenada) -- sem gêmeo no caminho velho por análise (lá
  não há multiplicação), pré-respondido pro dominó do review.
- **D21**: sem flag nova (mesmo átomo); free functions declaradas no header e DEFINIDAS no
  .cpp (prova de remoção por `nm` com a flag OFF continua valendo); MSVC-safe (<cmath> só).
- **D22**: câmera é estado do Draw2d; `shutdown()` reseta pra sem-câmera; move transfere.

### 4. Testes, gates, agentes

- **Headless-testável (resposta exata):** (1) unidades puras -- fórmulas D12 com valores
  computados à mão, round-trip em grade, wrap 2π, totalidade D16, D17, cantos D18
  (permutação em π/2, pivôs, espelho, flips), equivalência EXATA D19 (float igual --
  legítimo: CPU determinístico e identidade é IEEE-exata), overflow D20; (2) readback
  ESTATÍSTICO sob Xvfb/llvmpipe -- offset/zoom 2×/rotação π/2 com textura assimétrica/
  transform sem câmera/troca de câmera no meio do bracket com UMA textura (batching
  sobrevive); os testes v0.20.0 existentes rodam INTOCADOS (guarda do contrato publicado).
  **NÃO testável headless, declarado:** pixel-exato em GPU (llvmpipe não-determinístico;
  exatidão SÓ nos testes puros de CPU); execução Windows (MSVC compila, não roda);
  performance do caminho de transform (D2D-3); combinação exaustiva de ângulos
  câmera×sprite (matemática pura cobre, visual não é provável headless -- não é alegado).
- **Gates:** `/var/tmp/glx-onda4*` + `TMPDIR=/var/tmp`, 1 build pesado por vez, duas
  configs + ASan, linha headless canônica INTEIRA e serial, `preci.sh`,
  **`check_doc_line_refs.sh` re-rodado APÓS todo commit que desloca linha** (o draw2d.md já
  cita `draw2d.hpp:NNN` que os edits do A VÃO deslocar), os DOIS `nm` (ADR-0013; flag OFF
  remove inclusive as free functions novas), MSVC-safe, implementer ≠ reviewer, review que
  EXECUTA (mutation dirigido às fórmulas: sinal do sin, ±rotation, ordem scale/rotate,
  1/zoom, equivalência D19), push/tag SÓ com autorização explícita do líder, bump
  `project(VERSION 0.21.0)` + CHANGELOG + bloco de relay pro consumidor
  (`set_camera(camera_from_world_rect(rect, w, h))` -- uma linha, sem espelhar API).
- **Agentes (4 no fan-out; adendo antes):** `software-architect` grava o adendo pós-decisão
  → implementer A (`backend-engineer`, D2D-2A: matemática pura + header público + unitários)
  → implementer B (`backend-engineer` DIFERENTE, D2D-2B: batcher/plumbing/readback/hostil/
  bump; depende do A e colide em `tests/CMakeLists.txt`) → `technical-writer` (DOC-D2D2,
  após B) → `qa-engineer` adversarial que EXECUTA. Orquestrador re-verifica build limpo,
  claims file:line, `git log` e os dois `nm`.

### 5. Contra-argumentos do CTO (registro honesto)

1. **Continua M? Sim** -- a camada GL não muda nada (D13); a onda é matemática pura +
   plumbing + testes, exatamente o "low-med, pure/unit-able" que o programa precificou. O
   único acréscimo ao escopo do programa é o D17 (P, genérico, cortável).
2. **Por que não dar o `begin_frame(world_rect)` deles?** Porque é a forma do renderer
   DELES, não um modelo de câmera -- eixo 2 do `D2D-BLOCO`. Câmera genérica + D17 chega à
   mesma adoção-em-uma-linha sem congelar a forma da API deles na nossa.
3. **O contrato D5 publicado é REENQUADRADO, não quebrado -- mas É evolução semântica de
   doc publicado**, por isso a Q1 vai ao líder em vez de ser decidida aqui. Sob (c) a
   garantia é estrutural: sem câmera, o caminho v0.20.0 roda intocado.
4. **Dois caminhos de vértice (velho + quad) é risco real de divergência.** Aceito de
   propósito: reescrever o velho trocaria compat provada-por-ausência-de-diff por
   provada-por-teste. Ficam as DUAS redes (caminho velho intocado + equivalência exata
   D19); o D2D-3 pode unificar depois de uma onda de soak.
5. **O guard pós-conta do D20 não tem gêmeo no caminho velho** -- o dominó do review vai
   apontar; o plano pré-responde (lá não há multiplicação que estoure), então o reviewer
   confirma a análise em vez de abrir falso positivo.
