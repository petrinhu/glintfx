# Draw2D (D2D-1 reference and how-to)

> **EN:** `glintfx::Draw2d` API reference, coordinate/premultiply/tint contracts, the GL-state
> ownership contract (the wave's central risk), and App/embed composition recipes. Diátaxis
> type: **how-to / reference**. Audience: a developer drawing a game world (sprites) alongside
> or without the `.rcss`-declared UI.
> **PT:** Referência da API `glintfx::Draw2d`, contratos de coordenadas/premultiply/tint, o
> contrato de posse de estado GL (o risco central da onda), e receitas de composição App/embed.
> Tipo Diátaxis: **how-to / reference**. Audiência: dev desenhando o mundo de um jogo (sprites)
> ao lado da, ou sem a, UI declarada em `.rcss`.

`glintfx::Draw2d` (`glintfx/include/glintfx/draw2d.hpp`) is the sprite-drawing facade for the
`GLINTFX_MODULE_DRAW2D` atom, added by [ADR-0017](adr/0017-draw2d-module.md) (framework-2D
architecture, [ADR-0015](adr/0015-framework-2d-atomized-architecture.md)). It depends on `core`
plus the internal image-decode seam and the internal `glx_`-prefixed GL loader ONLY -- no
`window`, no `ui`, no `fx` -- so it works in an embed host with no GLFW linked, and compiles in
the same `GLFW=OFF` leg the MSVC job builds.

---

## English

### Lifecycle

```cpp
glintfx::Draw2d draw2d;                                // touches nothing yet
draw2d.init();                                          // requires a CURRENT GL 3.3 core context
glintfx::Texture2d hero = draw2d.load_texture("assets/hero.png");

draw2d.begin(target_w, target_h);                        // screen px, top-left, y-down (D5)
draw2d.draw_sprite(hero, {100.f, 100.f, 64.f, 64.f});     // dst only -- whole texture, white tint
draw2d.end();                                             // flushes whatever is pending

draw2d.destroy_texture(hero);
draw2d.shutdown();                                        // or just let `draw2d` go out of scope
```

- `Draw2d()` (`glintfx/include/glintfx/draw2d.hpp:477`) does not touch GL -- construction is
  always cheap and side-effect-free, same discipline as `Audio()`/`App()`/`UiLayer()`.
- `init()` (`glintfx/include/glintfx/draw2d.hpp:457`) requires a CURRENT GL 3.3 core context.
  It calls `glx_gl_load()` itself -- see "GL loader idempotency" below for why that is safe even
  when a cohabiting `App`/`UiLayer` already loaded the same table. Returns `false` on any
  GL/shader failure, or when the instance is already initialized (call `shutdown()` first to
  re-init).
- `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:511`) is idempotent -- releases every live
  texture, the VBO/VAO, and the shader program, ONLY if `init()` ever succeeded, and discards
  any open/partial `begin()`/`end()` bracket. The destructor calls it.
- **Moved-from state is a safe no-op, deliberately stricter than `App`.** Every method on a
  moved-from `Draw2d` behaves exactly like a never-initialized instance (`ok()` returns `false`)
  -- stricter than `App`'s own "UB on anything but `ok()`/the destructor after a move" contract
  (`app.hpp`), a deliberate choice that costs nothing here and matches this module's own
  fail-high discipline (`glintfx/include/glintfx/draw2d.hpp:181-185`).
- **Lifetime requires a current context (D11).** `shutdown()`/the destructor release GL objects
  only if `init()` ever succeeded, and require that same context (or an equivalent one on the
  same share-group) to still be current -- same honesty this doc's own `UiLayer` contract
  already demands (see `docs/embed-integration.md` section 0).

### Coordinate contract (D5, REFRAMED by D2D-2)

Screen-space pixels, **top-left origin, y-down** -- the SAME space as `UiEvent` mouse
coordinates and `UiLayer::get_element_box` (see `docs/embed-integration.md` section 10): one
coordinate story for the whole library. `src_px` is a sub-rectangle of the texture in **texel**
coordinates (top-left origin, y-down, matching image memory row order); `RectF{}` (all-zero) is
the documented sentinel for "whole texture".

**This is now the documented NO-CAMERA case, true by construction.** `Draw2d`'s camera is
OPTIONAL and DEFAULTS OFF (ADR-0017's [Addendum](adr/0017-draw2d-module.md#addendum-2026-07-23-d2d-2-coordinate-model), Q1 (c)): with no camera set, `draw_sprite` runs the
LITERAL v0.20.0 code path above, not a re-derivation that happens to match it -- an absence of
diff, additionally pinned by an exact-equivalence unit test (D19). Every v0.20.0 call site keeps
compiling and rendering identically; nothing described in this section changed. `set_camera(...)`
(see "Camera" below) switches subsequent draws to WORLD space -- see that section for the
projection formulas and their own coordinate story.

### Camera (D2D-2 -- optional world space)

`Camera2d` (`glintfx/include/glintfx/draw2d.hpp:191`) is an OPTIONAL world camera. Its default
STATE is "no camera" -- exactly the coordinate contract above, not "camera set to identity" (see
the anchor note below for why that distinction matters). `set_camera(const Camera2d&)`
(`glintfx/include/glintfx/draw2d.hpp:589`) switches subsequent `draw_sprite` calls, in any open
or future bracket, to WORLD space; `reset_camera()` (`glintfx/include/glintfx/draw2d.hpp:641`)
returns to screen space -- idempotent, safe to call with no camera set. `shutdown()` already
resets to no-camera (D22); a freshly-`init()`ed instance always starts in screen space.

**Anchor -- read this before reaching for `Camera2d{}` expecting "no-op" (ADR-0017 Addendum,
Q2 (2a)):** `Camera2d.x`/`Camera2d.y` is the world point shown at the viewport's **CENTER**, not
its top-left corner. `cam.x = player.x; cam.y = player.y` is the whole follow-cam line, and
`zoom`/`rotation` pivot about the view center -- what every consumer visually expects. The cost,
stated loudly on purpose: a default-constructed `Camera2d{}` is **NOT** the identity mapping --
passed to `set_camera`, it centers the world origin on screen. This is harmless under the Q1 (c)
default (the default STATE of a `Draw2d` is "no camera", never "camera set to identity"), but it
WILL surprise anyone who calls `set_camera(Camera2d{})` expecting an unchanged frame.

`zoom` is screen px per world unit (`zoom 2` = magnified 2x; must be `> 0`). `rotation` is the
camera's OWN orientation in **radians** (D14) -- a positive value turns the camera clockwise, so
the world appears to rotate counter-clockwise on screen; any finite value is legal, wrapping
naturally through `sin`/`cos` past `2*pi` (pinned by a test: `t` and `t + 2*pi` produce the same
mapping within tolerance).

**Projection formulas (D12, literal -- single source, `glintfx/src/transform2d.hpp`, shared by
the draw path and the public helpers below, so the two cannot diverge -- a conversion formula
without a computed-value delta test is this house's own named silent-bug class):**

```
world_to_screen(cam, W, H, p) = rot((p - (cam.x, cam.y)) * cam.zoom, -cam.rotation) + (W/2, H/2)
screen_to_world(cam, W, H, s) = rot(s - (W/2, H/2), +cam.rotation) / cam.zoom + (cam.x, cam.y)
```

where `rot(v, t) = (v.x*cos(t) - v.y*sin(t), v.x*sin(t) + v.y*cos(t))`
(`glintfx/src/transform2d.hpp:124`) in y-down screen space. `screen_to_world` is the exact
inverse of `world_to_screen` (`rot(rot(v,-t),t) == v`), pinned by a round-trip property test over
a grid of points x cameras (offsets, zooms `0.1..10`, rotations including `> 2*pi`). Both
functions -- `world_to_screen` (`glintfx/src/transform2d.hpp:136`) and `screen_to_world`
(`glintfx/src/transform2d.hpp:154`) -- are also exposed as pure, public free functions
(`glintfx/include/glintfx/draw2d.hpp:240-241`, each a one-line delegation to this same header,
defined `glintfx/src/draw2d.cpp:1070`/`glintfx/src/draw2d.cpp:1091`): use them to convert your own
coordinates (a mouse click, say) between spaces without duplicating the math.

**Validation (D15, the `set_dp_ratio` idiom -- `glintfx/src/draw2d.cpp:937`):** any non-finite
`camera` member, or `camera.zoom <= 0`, rejects the call -- the PREVIOUS camera state (including
"no camera") is KEPT, one dedup'd log line, never a partial/corrupt write. `zoom` must be
strictly positive: `zoom == 0` would collapse the world and make `screen_to_world` divide by
zero, and negative zoom is deliberately NOT a mirror idiom -- `SpriteTransform.flip_h`/`flip_v`
(below) exist for that. `set_camera`/`reset_camera` are safe no-ops on a
never-initialized/moved-from/post-shutdown `Draw2d`, the same null-safe contract as every method
on this class.

**Totality of the pure helpers (D16):** `world_to_screen`/`screen_to_world` are pure and
TOTAL -- they cannot log. On invalid input (any non-finite member of `cam` or the point,
`cam.zoom <= 0`, or a non-positive viewport) they return the INPUT POINT UNCHANGED, never NaN
out. This is a deterministic, tested fallback, not an error path.

**The camera is sticky, CPU-side, and cheap to switch mid-frame (D13):** it is stored `Draw2d`
state, not a per-draw parameter -- it stays active across every `draw_sprite` call until the
next `set_camera`/`reset_camera`/`shutdown()`. Camera and per-sprite transform math run entirely
on the CPU, before the batcher (`glintfx/src/draw2d.cpp:1023-1024`) -- the GL layer and its
published touched-state list (D9 below) are untouched by either, so **switching the camera in
the middle of a `begin()`/`end()` bracket never forces a GL flush**, and same-texture batching
survives the switch (D4 untouched). A common pattern -- world sprites, then a screen-space HUD
icon, in one bracket:

```cpp
draw2d.begin(w, h);
draw2d.set_camera(world_camera);
draw2d.draw_sprite(hero, world_dst);      // world space
draw2d.reset_camera();
draw2d.draw_sprite(hud_icon, screen_dst); // screen space, same bracket
draw2d.end();
```

**`camera_from_world_rect` (D17, generic, the one cuttable convenience --
`glintfx/include/glintfx/draw2d.hpp:242`, `glintfx/src/transform2d.hpp:175`):**
`Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h)`
returns the camera that shows `world_rect` fitted by WIDTH (`zoom = viewport_w / world_rect.w`,
center = the rect's own center); vertical coverage follows the viewport's own aspect ratio -- a
caller wanting a letterbox-exact fit composes this with `UiLayer::set_viewport`
(`docs/embed-integration.md` section 10), which already exists. Fail-high: non-finite
`world_rect`, `world_rect.w <= 0 || world_rect.h <= 0`, or a non-positive viewport return
`Camera2d{}` (documented, not a crash).

### SpriteTransform (D18 -- per-sprite pivot/rotation/scale/flip)

`SpriteTransform` (`glintfx/include/glintfx/draw2d.hpp:222`) is orthogonal to the camera and
applies in **BOTH** spaces -- world units when a camera is set, screen px otherwise (D5/Q1 (c))
-- via the overload `draw_sprite(tex, dst, src_px, tint, transform)`
(`glintfx/include/glintfx/draw2d.hpp:628-629`, defined `glintfx/src/draw2d.cpp:621`).
`origin_x`/`origin_y` are the pivot, NORMALIZED within `dst` (default `0.5/0.5` = center);
`rotation` is radians about the pivot (D14, independent of any camera rotation);
`scale_x`/`scale_y` are about the pivot (default `1`; negative is LEGAL -- it mirrors the
geometry, winding is irrelevant because D9 already disables backface cull); `flip_h`/`flip_v`
swap UV on the RESOLVED `src_px` sub-rect (`glintfx/src/draw2d.cpp:1050-1066`), so an atlas frame
flips within itself and never bleeds into a neighbouring frame.

**Corner math, literal order (D18, `glintfx/src/transform2d.hpp:210`, single source with the
identity that also proves D19's compatibility pin):**

```
pivot P = (dst.x + origin_x*dst.w, dst.y + origin_y*dst.h)
corner' = P + rot((corner - P) * (scale_x, scale_y), rotation)     for each of TL,TR,BR,BL
```

**Order matters: SCALE first, THEN rotate, both about the pivot.** Swapping this order gives a
visually different (and wrong) result for any non-uniform scale combined with a non-zero
rotation -- it is the first line the adversarial review is briefed to mutate. With a camera set,
`dst` and the resulting corners are WORLD units and each corner is then individually projected
through `world_to_screen` (D12); with no camera they are screen px directly -- "one composition
story, no special cases" (D18).

**Compatibility (D19): the plain 4-arg `draw_sprite` is UNCHANGED by this overload's
existence.** It keeps calling the v0.20.0 batcher path verbatim, camera or not -- that
absence-of-diff IS the Q1 (c) compatibility guarantee, additionally pinned by an exact-
equivalence unit test: an identity `SpriteTransform{}` with no camera produces BIT-IDENTICAL
vertices to the old path (legitimate exact-float equality here -- CPU-deterministic, and
identity ops `*1.0f`, `+0.0f`, `cos(0)=1`, `sin(0)=0` are IEEE-exact).

**Fail-high (D20, D10 extended):** every `transform` member finite BEFORE any arithmetic
(NaN/Inf anywhere skips the draw, dedup'd log, same `NonFinite` class as `dst`/`src_px`/`tint`);
`dst.w <= 0 || dst.h <= 0` stays the silent legal no-op the 4-arg overload already has
(unchanged). AFTER the corner math, one more guard rejects a non-finite COMPUTED corner
(overflow -- e.g. a huge `zoom` times a huge coordinate multiplying to `Inf`,
`glintfx/src/draw2d.cpp:1044-1041`). This post-math guard has NO v0.20.0 twin, by design: the old
axis-aligned path has no multiplication that can overflow, so there was nothing to guard
against there -- confirmed as an analysis, not an omission, by the adversarial review.

### Primitives (D23-D26 -- untextured drawing)

`draw_filled_rect`, `draw_filled_quad`, `draw_line`, and `draw_rect_outline`
(`glintfx/include/glintfx/draw2d.hpp:705`/`616`/`637`/`668`/`699`) draw untextured shapes
through the SAME batched pipeline as `draw_sprite` -- no second shader, no new GL-state class
(decision A1 (a), D23). `init()` creates an internal 1x1 RGBA8 premultiplied WHITE texture
(`Impl::create_white_texture()`, `glintfx/src/draw2d.cpp:405`) as a NORMAL registry slot (id 1;
the public `Texture2d` handle for it is never constructed, so a consumer can neither draw with
it nor destroy it). Creation failure fails `init()` itself (fail-high: a `Draw2d` that cannot
draw the primitives it advertises does not half-initialize). Consequence, stated: the first
consumer `load_texture()` now returns id 2, unobservable through the public API (ids are
opaque).

Every primitive shares D24's composition story with `draw_sprite`/`SpriteTransform`: geometry
is built in the ACTIVE space (world units with a camera set, screen px otherwise, D5/D18), each
corner is then projected through `world_to_screen` when a camera is set, then passes the D20
post-math finiteness guard, then enters the SAME batcher as a `draw_quad` against the white
texture with the primitive's colour as the D8 tint -- on that white texel, the D8 formula
(`out = texel_premult * vec4(tint.rgb*tint.a, tint.a)`, see "Premultiply and the tint formula"
below) yields EXACTLY `color` in premultiplied space, no separate formula. Consequence:
`draw_line`/`draw_rect_outline` thickness is in ACTIVE-space units and scales with `cam.zoom`
BY PROJECTION alone -- the consumer's measured "outline/line thickness in world units, scaling
with the camera" need falls out of this general rule, no bespoke API. Every primitive shares
the sprite fail-high template: any non-finite input member skips the draw (dedup'd log) BEFORE
any arithmetic; a degenerate size (`dst.w<=0||dst.h<=0`, a line's `a==b` or `thickness<=0`, an
outline's `thickness<=0`) is a SILENT legal no-op; the post-math corner guard (D20) covers
overflow, same as sprites. Each primitive is tagged by the CURRENT layer (D27) and travels with
the CURRENT scissor (D28) exactly like `draw_sprite`.

`draw_filled_rect` has a 4-arg overload and a `SpriteTransform` overload (same
pivot/rotation/scale/flip composition as `draw_sprite`'s own transform overload, "SpriteTransform"
above -- `flip_h`/`flip_v` are accepted but have no visual effect on a solid fill). `draw_filled_quad`
takes four caller-supplied corners in TL,TR,BR,BL order (D5/D19's convention); a degenerate or
self-crossing (bowtie) quad is LEGAL and draws exactly the two triangles the corners describe
(the GPU's own business).

**Line geometry (D25, literal -- single source `glintfx/src/primitives2d.hpp:203`,
`compute_line_corners()`):** with `d = b - a`, `len = |d|`, unit `u = d/len`, normal
`n = (-u.y, u.x)`, `h = thickness/2`:

```
tl = a + n*h,  tr = b + n*h,  br = b - n*h,  bl = a - n*h
```

Butt caps -- the quad ends exactly at `a` and `b`; round/miter caps and polylines are seed
`SEED-D2D-LINECAPS` (`TODO.md` INBOX), out of this wave. Guard order (`is_degenerate_line()`,
`glintfx/src/primitives2d.hpp:183`): `a`/`b`/`thickness`/`color` all finite BEFORE any
arithmetic (the `sqrt` included), THEN `a == b` (exactly) or `thickness <= 0` is checked -- no
divide-by-zero path exists in `compute_line_corners()` because the caller never invokes it when
degenerate.

**Rect outline (D26, literal -- single source `glintfx/src/primitives2d.hpp:247`,
`compute_outline_strips()`):** four NON-OVERLAPPING strips inside `dst`, with the collapse
clamp `t = min(thickness, min(dst.w, dst.h)/2)`:

```
top    = {x,       y,        w,      t}
bottom = {x,       y+h-t,    w,      t}
left   = {x,       y+t,      t,      h-2t}
right  = {x+w-t,   y+t,      t,      h-2t}
```

Non-overlap is a CORRECTNESS property, not a micro-optimization: with a translucent outline
colour, overlapping strips would double-blend at the corners (visibly darker squares). The SAME
clamped formula, with no special-case branch, tiles `dst` EXACTLY once
`2*thickness >= min(dst.w, dst.h)` -- the outline degenerates to ONE filled rect covering `dst`.
Each of the 4 strips is projected and submitted independently: a non-finite PROJECTED corner on
one strip skips only THAT strip (logged once, dedup'd), the others still draw.

**Not shipped, seeded:** circle/ellipse/regular-polygon primitives (`SEED-D2D-SHAPES` -- the
`ui` atom's `polygon()` RCSS decorator already covers the declarative side); secondary sort by
texture within a layer (`SEED-D2D-LAYER-TEXSORT`).

### Layers (D27 -- explicit draw order, opt-in)

`set_layer(int layer)` (`glintfx/include/glintfx/draw2d.hpp:844`) is OPT-IN, default OFF: a
bracket that never calls `set_layer` runs the LITERAL v0.21.0 streaming path unchanged -- the
SAME "absence of diff" idiom the camera (D13) and the coordinate contract (D5) already use in
this module, applied a third time. Any `int` is legal (negative sorts below the default `0`);
the CURRENT layer tags every subsequent `draw_sprite`/primitive call, sticky WITHIN the bracket.

**Arming:** the FIRST `set_layer` call arms buffered mode for the CURRENT bracket (or the NEXT
one, when called outside a bracket). If draws already streamed in the current bracket, they are
finalized and drained under the OLD streaming path FIRST (`SpriteBatch::flush_pending()`,
`glintfx/src/sprite_batch.hpp:261`, the one additive D31 method) -- they render below every
layer, effectively "under the bridge". Recommendation: call `set_layer` before the first draw
of a bracket to avoid this split.

**Buffering and replay:** a buffered command captures the FINAL state -- post-validation,
post-projection screen-space corners, texture id (white for primitives), resolved `src_px`,
clamped tint, the scissor snapshot ACTIVE at push time (D28), and the layer tag; the camera is
therefore sampled AT CALL TIME, exactly like streaming mode. At `end()`, commands are
stable-sorted by layer (`std::stable_sort`, `glintfx/src/layer_queue.hpp:206` -- equal layer
keeps submission order, GUARANTEED) and replayed through the UNTOUCHED batcher, grouped into
maximal runs of consecutive same-scissor commands (`scissor_equal()`,
`glintfx/src/layer_queue.hpp:111`); `end()` then RESETS to streaming/layer 0 for the NEXT
bracket -- layers are NOT sticky across brackets like the camera/scissor, because (unlike them)
they change the module's own flush timing.

**Layers order draws WITHIN one Draw2D bracket only.** Composition with the UI or the host's
own GL calls stays order-of-call (ADR-0017 axis 3, untouched) -- there is still no cross-module
ordering API.

**Memory:** buffering holds ONE bracket's commands until `end()`, hard-capped at 262144
(`LayerQueue::kMaxCommands`, `glintfx/src/layer_queue.hpp:166`, ~24 MiB at ~96 B/command) -- a
MEMORY bound against a hostile stream (same class as the batcher's own 4096-quad flush bound,
D4), NOT a performance claim -- see "Performance" below; past the cap, draws are dropped,
logged once, dedup'd. `shutdown()` discards any open buffer.

### Scissor (D28 -- rectangular clip)

`set_scissor(const RectF& rect_px)` / `reset_scissor()`
(`glintfx/include/glintfx/draw2d.hpp:889`/`792`) clip drawing to a rectangle, ALWAYS in SCREEN
px -- top-left origin, y-down, the SAME space as `begin()`'s target size -- camera-INDEPENDENT
(A3): a world-rect clip under rotation is a mask/stencil-class feature, not this (seed
`SEED-D2D-WORLDCLIP`). Sticky across draws AND brackets (like the camera) until
`reset_scissor()`/`shutdown()`.

**Validation (D15's idiom, `glintfx/src/draw2d.cpp:1558`):** any non-finite `rect_px` member
REJECTS the call, the PREVIOUS scissor state is KEPT, one dedup'd log.
`rect_px.w <= 0 || rect_px.h <= 0` is LEGAL and means "clip everything" (GL semantics -- e.g. a
collapsing reveal animation). Negative `x`/`y` are LEGAL (clamping is the GPU's job).

**Flush-boundary semantics, named contrast with the camera (D13):** the camera never forces a
GL flush (D13); a mid-bracket scissor change in STREAMING mode DOES -- the pending run is
finalized and drained under the OLD scissor first (`flush_pending()`+drain, same D31 method
`set_layer` already uses), then the state updates. In BUFFERED mode nothing is forced: each
command's OWN scissor snapshot travels with it (captured at push time), and replay already
groups consecutive same-scissor commands -- same observable semantics.

**GL mapping at flush, literal (the y-flip conversion -- single source, `map_scissor_to_gl()`,
`glintfx/src/primitives2d.hpp:272`):**

```
glScissor(x, target_h - (y + h), w, h)
```

with `w`/`h` (the GL "extents") clamped to non-negative BEFORE the int cast; `x`/`y` (the
origin) are cast straight through, un-clamped, per D28's own "clamping is the GPU's job" rule.

**The ONE declared behavioural diff in existing code (D31), and the D9 contract update it
forces:** `set_gl_state_for_draw()` (`glintfx/src/draw2d.cpp:458`) used to call
`glDisable(GL_SCISSOR_TEST)` unconditionally, every flush. It is now conditional
(`glintfx/src/draw2d.cpp:463-474`): if a scissor is set, `glEnable(GL_SCISSOR_TEST)` +
`glScissor(mapped rect)`; otherwise `glDisable(GL_SCISSOR_TEST)`, still asserting EVERYTHING
this state depends on at every flush, per D9's own no-caching rule, unchanged. With no scissor
EVER set, the emitted GL calls are IDENTICAL to v0.21.0. The GL-state contract (D9, below)
publishes an UPDATED touched-state line for this.

**High-severity warning for embed hosts -- read this before shipping a scissor call:** Draw2D
may now leave `GL_SCISSOR_TEST` ENABLED behind (it still restores nothing, by contract, D9). In
`App`, the frame hook's `GlStateGuard` already captures and restores the scissor box+enable
(`glintfx/src/gl_state.hpp:68`/`72`/`129`/`131`, verified, not assumed) -- covered for free.
**In EMBED MODE this is the sharpest edge of the whole D2D-3 wave for hosts:** a leftover
scissor clips the host's NEXT GL pass, INCLUDING its own clear. Reset your own scissor, or call
`reset_scissor()`, before handing the frame back to the host. `draw2d_ui_coexist_sanity.cpp`
pins that `UiLayer::render()` itself is unaffected (its own `GlStateGuard` already
re-establishes state) -- but a host doing its OWN raw GL after a Draw2D pass has no such guard
unless it writes one.

### Texture content bbox (D29)

`texture_content_bbox(const Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:929`) returns
`TextureBbox{found, x, y, w, h}` (`glintfx/include/glintfx/draw2d.hpp:340`, house precedent:
`ElementBox{found,x,y,w,h}` from `UiLayer::get_element_box`) -- the smallest rect containing
every texel with `alpha > 0` (semi-transparent counts; an alpha THRESHOLD parameter is seed
`SEED-D2D-BBOX-THRESH`). Coordinates are in the SAME texel space as `src_px` (top-left origin,
y-down, matching image memory row order, D5) -- the hitbox-from-bbox pattern composes with
`draw_sprite`'s `src_px` directly.

Computed ONCE, inside `load_texture()` (`compute_content_bbox()`, `glintfx/src/image_decode.hpp`
-- on the decoded pixels that already exist on the CPU there, discarded right after) and cached
in the texture's registry slot (16 bytes) -- NOT retained as a full RGBA buffer (a 256 MiB-class
memory cost for a rarely-used query), NOT a `glGetTexImage` readback on demand (a GL round-trip
and a GLES/core-profile portability trap). Premultiply does NOT disturb this: the alpha channel
is unchanged by premultiplication.

`found == false` (every other field zero) on: an invalid/stale/foreign/never-loaded `tex`
handle (the full D7 validation chain, dedup'd log -- same rejection story as `draw_sprite`) OR a
fully-transparent texture -- distinct from a 1x1 opaque texture, which returns
`{true, 0, 0, 1, 1}`.

### API reference

| Method | Returns | Notes |
| :--- | :--- | :--- |
| `init()` (`glintfx/include/glintfx/draw2d.hpp:457`) | `bool` | Requires a current GL 3.3 core context. `false` on GL/shader failure or if already initialized. |
| `ok()` (`glintfx/include/glintfx/draw2d.hpp:502`) | `bool` | `true` between a successful `init()` and the next `shutdown()`/destruction. |
| `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:511`) | `void` | Idempotent GL teardown. Safe on a never-initialized instance. |
| `load_texture(const char* path)` (`glintfx/include/glintfx/draw2d.hpp:541`) | `Texture2d` | PNG/JPG/TGA/BMP (whatever `image_decode.hpp`'s stb_image-backed decode recognises), premultiplied on upload. `path` read via a plain `const char*` ifstream overload (D7, MSVC-safe), 256 MiB cap. `ok() == false` on `nullptr`, open failure, the cap, or a decode failure -- never a crash. |
| `destroy_texture(Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:558`) | `void` | Releases the GL texture if any, ALWAYS zeroes `tex` on return. Fail-high (never UB) on already-destroyed, tampered, or foreign handles. |
| `begin(int target_width, int target_height)` (`glintfx/include/glintfx/draw2d.hpp:577`) | `void` | Opens a batching bracket (D4). `target_width`/`target_height` are the viewport size in screen px this bracket's sprites project against -- the SAME size the host's own GL viewport is set to for this pass. Nested `begin()` implicitly ends the previous bracket first (flushing it, logged once). |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px = RectF{}, const ColorF& tint = ColorF{})` (`glintfx/include/glintfx/draw2d.hpp:611-612`) | `void` | Queues one textured quad, internally batched by texture. See "Batching" and "Fail-high input surface" below for the full guard list. |
| `set_camera(const Camera2d& camera)` (`glintfx/include/glintfx/draw2d.hpp:631`) | `void` | Switches subsequent draws to world space (D2D-2, Q1 (c)). Fail-high (D15): keeps the previous state on a non-finite member or `zoom <= 0`. See "Camera" above. |
| `reset_camera()` (`glintfx/include/glintfx/draw2d.hpp:641`) | `void` | Returns subsequent draws to screen space (the no-camera default). Idempotent. See "Camera" above. |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px, const ColorF& tint, const SpriteTransform& transform)` (`glintfx/include/glintfx/draw2d.hpp:670-671`) | `void` | D18 overload -- pivot/rotation/scale/flip, in whichever space is active. See "SpriteTransform" above. |
| `end()` (`glintfx/include/glintfx/draw2d.hpp:679`) | `void` | Closes the bracket opened by `begin()`, flushing whatever is still pending -- unconditionally, even an empty bracket. Safe without a matching `begin()`. |
| `Texture2d::ok()` (`glintfx/include/glintfx/draw2d.hpp:294`) | `bool` | `false` when never loaded, when `load_texture()` failed, or after `destroy_texture()` zeroed this handle. |
| `Texture2d::width()`/`height()` (`glintfx/include/glintfx/draw2d.hpp:297-298`) | `int` | Pixel dimensions of the decoded image. `0` when `!ok()`. |
| `world_to_screen(const Camera2d&, int, int, Vec2F)` (`glintfx/include/glintfx/draw2d.hpp:261`) | `Vec2F` | Pure, total free function (D12/D16). See "Camera" above. |
| `screen_to_world(const Camera2d&, int, int, Vec2F)` (`glintfx/include/glintfx/draw2d.hpp:262`) | `Vec2F` | Pure, total free function (D12/D16), exact inverse of `world_to_screen`. |
| `camera_from_world_rect(const RectF&, int, int)` (`glintfx/include/glintfx/draw2d.hpp:263`) | `Camera2d` | D17, generic rect-fit convenience. Hostile input returns `Camera2d{}`. |
| `draw_filled_rect(const RectF&, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:705`) | `void` | D23/D24 -- untextured filled rectangle, ACTIVE space. See "Primitives" above. |
| `draw_filled_rect(const RectF&, const ColorF&, const SpriteTransform&)` (`glintfx/include/glintfx/draw2d.hpp:720`) | `void` | D18 overload -- pivot/rotation/scale/flip, same composition as `draw_sprite`'s own transform overload. |
| `draw_filled_quad(Vec2F, Vec2F, Vec2F, Vec2F, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:741`) | `void` | D24 -- arbitrary quad, corners in TL,TR,BR,BL order. Bowtie quads are legal. |
| `draw_line(Vec2F, Vec2F, float, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:772`) | `void` | D25 -- butt-capped line segment, `thickness` in ACTIVE-space units. See "Primitives" above. |
| `draw_rect_outline(const RectF&, float, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:803`) | `void` | D26 -- 4 non-overlapping strips, collapse clamp at `2*thickness >= min(w,h)`. See "Primitives" above. |
| `set_layer(int)` (`glintfx/include/glintfx/draw2d.hpp:844`) | `void` | D27 -- opt-in buffered draw order. `end()` resets to streaming/layer 0. See "Layers" above. |
| `set_scissor(const RectF&)` (`glintfx/include/glintfx/draw2d.hpp:889`) | `void` | D28 -- ALWAYS screen px, camera-independent. Fail-high keeps previous state on non-finite input. See "Scissor" above. |
| `reset_scissor()` (`glintfx/include/glintfx/draw2d.hpp:896`) | `void` | D28 -- idempotent, clears the scissor state set by `set_scissor`. |
| `texture_content_bbox(const Texture2d&)` (`glintfx/include/glintfx/draw2d.hpp:929`) | `TextureBbox` | D29 -- smallest `alpha>0` rect, cached at `load_texture()` time. See "Texture content bbox" above. |

### Premultiply and the tint formula (D8)

`load_texture()` decodes and **alpha-premultiplies** the image on upload
(`glintfx/src/image_decode.hpp:150`, `R = R*A/255`, `G = G*A/255`, `B = B*A/255`, `A` unchanged)
to match this module's blend, `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`
(`glintfx/src/draw2d.cpp:369`). `ColorF` (`glintfx/include/glintfx/draw2d.hpp:142`) is
**straight** (non-premultiplied) RGBA in `[0, 1]`, each channel clamped on intake -- default is
opaque white (identity: an unmodified sample). The fragment shader
(`glintfx/src/draw2d.cpp:137-146`) applies the exact modulation:

```
out = texel_premult * vec4(tint.rgb * tint.a, tint.a)
```

`texel_premult` is already alpha-premultiplied on upload, so `tint.rgb * tint.a` premultiplies
the straight tint into the same space before modulating, and `tint.a` alone modulates the
(already-premultiplied) alpha channel. `tint.a` fades the sprite as a whole; `tint.rgb` scales
the colour. This formula is also pinned by a pixel-readback test with a **computed** expected
value (`draw2d_render_sanity.cpp`, per the plan's testing rule: a formula without a delta test
that exercises it is this house's named silent-bug class).

### GL-state contract (D9 -- the anti-`GlStateGuard`-bug decision)

Draw2D **sets every piece of GL state it depends on at each internal flush**
(`set_gl_state_for_draw()`, `glintfx/src/draw2d.cpp:362-376`) and **assumes nothing** left behind
by a cohabiting renderer -- the RmlUi GL3 backend shares this same GL context. Enumerated list
of touched state (same style as `gl_state.hpp`'s captured list, `glintfx/src/gl_state.hpp:2-6`):

- Shader program (`glUseProgram`)
- Vertex array object (`glBindVertexArray`)
- Depth test (explicitly `glDisable(GL_DEPTH_TEST)`)
- Face culling (explicitly `glDisable(GL_CULL_FACE)`)
- Scissor test (CONDITIONAL since D2D-3/D28, `glintfx/src/draw2d.cpp:463-474`: `glEnable(GL_SCISSOR_TEST)` + `glScissor(...)` when a scissor is set via `set_scissor`, otherwise `glDisable(GL_SCISSOR_TEST)` as before -- see "Scissor" above for the CONTRACT CHANGE this represents)
- Blend enable (`glEnable(GL_BLEND)`)
- Blend func, both RGB and alpha (`glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`)
- Blend equation (`glBlendEquation(GL_FUNC_ADD)`)
- Active texture unit (`glActiveTexture(GL_TEXTURE0)`)
- 2D texture binding on that unit (`glBindTexture(GL_TEXTURE_2D, ...)`)
- The `uTex`/`uTargetSize` shader uniforms

**D2D-3 contract change, stated loudly:** before v0.22.0, the scissor line above was
unconditional (`glDisable(GL_SCISSOR_TEST)`, every flush, no exception) -- a scissor feature
that never enables scissor does not exist, so this line changed. With no scissor EVER set
(`set_scissor` never called) the emitted GL stream is byte-identical to v0.21.0; the moment a
host calls `set_scissor`, Draw2D can leave `GL_SCISSOR_TEST` ENABLED behind after `end()`. See
"Scissor" above for the full mapping/flush-boundary contract and the embed-mode hazard.

**Draw2D does NOT restore any of this after `end()`.** This is deliberate, not an oversight --
restoration is the enclosing contract's job, and both enclosing contracts already exist:

- **In `App`**, the frame hook's state is auto-restored before the UI render pass runs (the
  `set_frame_callback` contract, `glintfx/include/glintfx/app.hpp:506` -- see
  `docs/embed-integration.md` section 21). Draw2D calls made inside the hook are covered by that
  same restore, for free.
- **In embed mode**, `UiLayer::render()` re-establishes its own full GL state inside
  `GlStateGuard` regardless of what ran before it. What Draw2D leaves behind after the host's
  own draw pass is the host's to manage -- exactly like the host's own raw GL calls today.

This contract is pinned by a coexistence test in BOTH orders (sprite pass then UI pass, and UI
pass then sprite pass -- `draw2d_ui_coexist_sanity.cpp`), the test that guards this wave's named
central risk: a new GL path cohabiting the RmlUi GL3 renderer's context is exactly the bug class
`GlStateGuard` (`glintfx/src/gl_state.hpp`) exists to guard against.

### Composition recipes

No new ordering API -- composition order is call order (ADR-0017 axis 3). Both hosting modes
reuse an insertion point that already exists.

**`App` -- inside the frame hook, under the UI:**

```cpp
glintfx::App app;
// ... app.load(...) ...
glintfx::Draw2d draw2d;
draw2d.init();
glintfx::Texture2d hero = draw2d.load_texture("assets/hero.png");

app.set_frame_callback([&](float dt_seconds) {
  draw2d.begin(app_width, app_height);   // the SAME size the App's own viewport uses this pass
  draw2d.draw_sprite(hero, {player_x, player_y, 64.f, 64.f});
  draw2d.end();
});
app.run();
```

The frame hook runs after the frame clear and before the UI render pass
(`glintfx/include/glintfx/app.hpp:506`), so sprites drawn here compose **under** the UI -- there
is no post-UI insertion point in this slice (see "Limits declared" below).

**Embed -- before or after `ui_layer.render()`, host's choice:**

```cpp
// world under the UI (the GusWorld production pattern):
draw2d.begin(viewport_w, viewport_h);
draw2d.draw_sprite(hero, dst);
draw2d.end();
ui_layer.render();

// -- OR, for an overlay drawn ON TOP of the UI --
ui_layer.render();
draw2d.begin(viewport_w, viewport_h);
draw2d.draw_sprite(hud_icon, dst);
draw2d.end();
```

Either order is legal; the host controls it exactly as it already controls any of its own raw GL
calls (D9 above covers who restores what).

### Texture handles (D7)

`Texture2d` (`glintfx/include/glintfx/draw2d.hpp:287`) is a plain, copyable value handle -- NOT
an RAII owner (`destroy_texture()` is the explicit release, same idiom as `Audio::SoundId`).
Internally it carries an opaque id, a generation counter, and an **owner tag** (the issuing
`Draw2d`'s own identity): a `Draw2d` instance validates every handle against its own internal
registry before use.

**A handle is NOT interchangeable between `Draw2d` instances.** `load_texture()` stamps the
returned handle with the emitting instance's identity (`glintfx/src/draw2d.cpp:1073`); both
`draw_sprite()` / `destroy_texture()` (`glintfx/src/draw2d.cpp:1144`/`728`) reject a handle whose
`owner_` tag does not match `this`, logged once and treated exactly like any other
unknown/stale handle -- never dereferenced as a raw GL name.
This is a guarantee by construction, not a numeric coincidence: id and generation alone cannot
tell "id 3, generation 1 in THIS instance's registry" apart from the same pair in a *different*
instance's independent registry.

### Batching (D4)

`begin(target_w, target_h)` ... N x `draw_sprite(...)` ... `end()`. The batcher
(`glintfx::draw2d_detail::SpriteBatch`, `glintfx/src/sprite_batch.hpp:75`) accumulates quads
while the texture stays the same as the previous call and flushes on: a texture change, `end()`,
or reaching an internal **4096-quad capacity**
(`glintfx/src/sprite_batch.hpp:151-159`) -- a MEMORY bound against a hostile/degenerate call
stream, transparent to the caller. **This is a memory bound, not a performance claim** -- see
"Limits declared" below for D2D-3.

- `draw_sprite()` outside a `begin()`/`end()` bracket: fail-high (logged once, dedup'd), no-op.
- Nested `begin()`: the previous bracket is implicitly ended (flushing whatever it had
  accumulated), logged once, dedup'd -- pending draws are never silently dropped.

All batching DECISIONS live in the pure `sprite_batch.hpp` (zero GL calls: it takes draw commands
and emits vertex data + flush ranges, unit-testable headless); `draw2d.cpp` owns only the GL
execution of what the pure batcher decided.

### Fail-high input surface (D10)

Every guard below is checked BEFORE any cast or arithmetic touches the input
(`sprite_batch.hpp:132-134`):

- Any NaN/Inf member in `dst`/`src_px`/`tint` -> skip the draw (logged once, dedup'd).
- `dst.w <= 0 || dst.h <= 0` -> silent no-op (a degenerate sprite, not an error -- not logged).
- Negative `dst.x`/`dst.y` -> **legal** (off-screen clipping is the GPU's job).
- `src_px` outside the texture bounds -> clamped, not rejected (documented: a half-off-atlas
  frame under animation-math jitter is a common legitimate case,
  `glintfx/src/sprite_batch.hpp:209-214`).
- `begin(target_w, target_h)` with either `<= 0` -> the bracket is accepted (still must be closed
  by `end()`) but every `draw_sprite()` inside it is a silent no-op until then, logged ONCE at
  `begin()`, not per `draw_sprite()`.
- An unknown/stale/tampered/foreign `Texture2d` handle -> rejected (logged once, dedup'd),
  never dereferenced as a raw GL name.

The `draw_sprite` overload taking a `SpriteTransform` (D2D-2) extends this list with its own
two-stage guard (member-finite before any arithmetic, then a post-math computed-corner guard) --
see "SpriteTransform" above (D20) rather than duplicating it here.

### GL loader idempotency

`Draw2d::init()` calls `glx_gl_load()` itself
(`glintfx/include/glintfx/draw2d.hpp:213-215`) -- necessary because with `ui`/`window` OFF nobody
else would load the internal GL function-pointer table. This is safe even when a cohabiting
`App`/`UiLayer` already loaded the same table: `glx_gl_load()` unconditionally resolves and
overwrites every symbol slot on each call, on both the Linux (`glintfx/src/gl_loader.c:897`) and
Windows (`glintfx/src/gl_loader.c:830`) code paths -- there is no internal "already loaded" guard
to interact with, so calling it twice is a redundant, harmless re-resolve, not a state hazard. A
host does not need to order Draw2D's `init()` relative to `App`'s/`UiLayer`'s own initialization.

### Performance (D30 -- declared numbers, honest gates)

`glintfx/tests/draw2d_perf_budget.cpp` measures three metrics, headless, printed as
machine-readable `GLINTFX_PERF <key>=<value>` lines on every run (visible in the CI log
regardless of gate outcome):

1. **`pure_batcher_quads_per_s`** -- 100k quads through `SpriteBatch` alone (CPU, no GL/display
   involved).
2. **`layered_streaming_ratio`** -- the SAME 100k-command stream through the buffered path
   (`LayerQueue::push` -> `drain_grouped()` -> replay via `SpriteBatch::draw_quad()`, the EXACT
   sequence `Draw2d::end()`'s own replay runs), reported as `time_layered / time_streaming`
   (`>= 1.0` -- the layered path can only ADD work, it reuses the SAME batcher underneath, D31's
   zero-diff set).
3. **`e2e_10k_sprite_1k_prim_median_ms`** -- a real `Draw2d` bracket of 10k sprites (one
   texture) plus a real bracket of 1k `draw_filled_rect` primitives, under a live GL 3.3 context
   on Xvfb/llvmpipe, `glFinish()`'d before the clock stops -- median over 30 frames (first 2
   discarded as warm-up).

**Declared downgrade:** llvmpipe rasterizes on the CPU -- metric 3 is a CPU-raster REGRESSION
GUARD, not a GPU throughput/bandwidth/vsync-pacing truth. Metrics 1 and 2 ARE real CPU numbers
(no GL involved at all).

**The gate, two-tier:** with `GLINTFX_PERF_STRICT=1` in the environment (set ONLY by the
`claudio` self-hosted runner's job in `.forgejo/workflows/heavy.yml`, never by
`ci.yml`/`nightly.yml`), metrics 1 and 3 FAIL if they cross their own BASELINE constant by more
than 1.5x. Without the env var (every other runner, every dev machine), only a blunt 10x
SANITY ceiling fails the test -- the number is still printed either way. The RATIO gate (metric
2) is MACHINE-RELATIVE by construction (both arms run on the same machine, same process, same
invocation), so it is STRICT EVERYWHERE, unconditional on `GLINTFX_PERF_STRICT` -- it is the
direct, always-on answer to this wave's own risk 2 ("layers vs batching is where performance can
degrade").

**A real finding, reported not hidden:** the ratio gate measures ~3.4-3.6x on the machine that
measured this wave's baselines, consistently across repeated runs -- above the 2.0x ceiling this
wave first set. Root cause: `LayerQueue::drain_grouped()`'s `std::stable_sort`
(`glintfx/src/layer_queue.hpp:206`) moves whole ~96-byte `LayerCommand` structs to reorder by a
single `int layer` key, dominating the layered path's own cost (roughly 60% of it in a
standalone probe) -- `Draw2d::end()`'s own replay runs the EXACT same `drain_grouped()` call
this benchmark does, so this is not a benchmark artifact. A lightweight-key sort was tried and
reverted: it measured better under gcc but WORSE under clang -- the only compiler every CI
workflow in this repo actually pins (`-DCMAKE_CXX_COMPILER=clang++`). The house's own call: keep
the plain `std::stable_sort` and REVISE THE CEILING to **4.0x** instead of chasing a
compiler-specific micro-optimization that regresses on the compiler CI actually runs -- layered
mode is opt-in by construction (the untouched streaming path pays none of this), so ~3.4x
measured WITH CLANG is the honest price of that feature, not a regression to chase further.

**Declared numbers, honest provenance** (`glintfx/tests/draw2d_perf_budget.cpp`, baseline
constants dated in-file): `pure_batcher_quads_per_s` baseline = **8 500 000**,
`e2e_10k_sprite_1k_prim_median_ms` baseline = **5.0**, `layered_streaming_ratio` ceiling =
**4.0x** (revised, clang-measured) -- all measured on **2026-07-23**, on a Fedora 44 sandboxed
dev container (12th Gen Intel Core i5-12500H, 16 logical cores, 31 GiB RAM), **NOT** the literal
`claudio` self-hosted Forgejo runner named by this wave's plan (a different physical machine).
This is a real measurement, never a guess, but a STAND-IN baseline: the first `claudio` CI run
of this test prints its own `GLINTFX_PERF` lines in the job log, and per D30 these three
constants should be swapped for that run's numbers once available (a one-line diff each, this
comment's date/host updated in `draw2d_perf_budget.cpp`) -- flagged here explicitly, not silently
left as if it were the final number.

### Limits declared

Stated instead of faked, per the plan's own testing-downgrade discipline:

- **No `draw_text`** -- `D2D-TEXT`, its own wave, requires its own ADR first (font-engine
  tension: `App`'s FreeType-vs-own-engine flip, `docs/adr/0011-soft-font-flip.md`).
- **No circle/ellipse/regular-polygon primitives** -- seed `SEED-D2D-SHAPES` (the `ui` atom's
  `polygon()` RCSS decorator already covers the declarative side); trigger: a consumer ask.
- **No line caps/joins (round/miter), no polylines** -- `draw_line` is butt-capped only; seed
  `SEED-D2D-LINECAPS`, trigger: a consumer ask.
- **No secondary sort by texture within a layer** -- fewer flushes in layered mode is seed
  `SEED-D2D-LAYER-TEXSORT`, trigger: the perf budget showing a flush-bound consumer.
- **No world-space clipping** (a rotated clip region under a camera) -- scissor here is
  screen-space only (D28, A3); a world-rect clip is a mask/stencil-class feature, seed
  `SEED-D2D-WORLDCLIP`, trigger: a consumer ask.
- **No alpha THRESHOLD parameter for `texture_content_bbox`** -- this wave's definition is fixed
  at `alpha > 0`, seed `SEED-D2D-BBOX-THRESH`, trigger: a consumer ask.
- **The 4096-quad batcher capacity and the 262144-command layer cap stay MEMORY bounds, not
  throughput claims** -- see "Performance" above for the measured, declared numbers.
- **Camera shake, smoothing, and bounds/clamp are the consumer's game loop's job, not this
  module's.** Draw2D ships the projection (`Camera2d`, `set_camera`, the pure helpers); it does
  not ship the cinematics on top of it (lerp-follow, deadzone, screen shake). Seeded follow-up:
  `SEED-D2D-CAMANIM` (`TODO.md` INBOX), trigger: a consumer ask.
- **No `camera()`/`has_camera()` getters on `Draw2d`.** The consumer already owns the
  `Camera2d` value it passed to `set_camera` -- a getter has zero demand today. Seeded
  follow-up: `SEED-D2D-CAMGETTER`, trigger: a consumer ask.
- **`camera_from_world_rect` fits by WIDTH only** -- an aspect-exact letterbox fit composes it
  with `UiLayer::set_viewport` today (see "Camera" above). Seeded follow-up:
  `SEED-D2D-LETTERBOX`, trigger: a consumer ask.
- **MSVC is a compile+link gate only.** The embed (`GLFW=OFF`) job builds this module (including
  the D2D-2 camera/transform surface, `<cmath>`-only, no Unix headers) on Windows; there is no
  Windows RUNTIME execution in CI.
- **Pixel-readback tests under Xvfb/llvmpipe are statistical, never pixel-exact** -- tile
  threading on llvmpipe is non-deterministic (the same reason the `golden_test` remains opt-in
  for real GPUs). Exactness claims (D19's equivalence pin, D12's computed-value formulas) live
  ONLY in the pure CPU unit tests, where they are legitimate.
- **`load_texture()` does NOT honour `set_asset_base_url`.** Draw2D reads the given path as-is
  (`glintfx/src/draw2d.cpp:480`) -- that facility is `ui`-side today. Seeded follow-up:
  `SEED-D2D-BASEURL` (`TODO.md` INBOX), trigger: a consumer ask.
- **No post-UI `App` frame hook** -- sprites cannot be drawn OVER the UI in standalone mode until
  that seed lands (`SEED-D2D-POSTUI`, trigger: a real consumer ask). In embed, the host already
  controls order freely (see "Composition recipes" above).
- **Texture filtering is fixed at `GL_LINEAR`** -- no nearest-neighbour option per texture in
  this slice. Seeded follow-up: `SEED-D2D-TEXFILTER`, trigger: a consumer ask (e.g. pixel art).

### Related

- `glintfx/include/glintfx/draw2d.hpp` -- full doc-commented API contract (source of truth).
- `glintfx/src/transform2d.hpp` -- the pure, single-source projection/corner math (D12/D18)
  behind both the camera draw path and the public `world_to_screen`/`screen_to_world`/
  `camera_from_world_rect` helpers.
- `glintfx/src/primitives2d.hpp` -- the pure, single-source geometry (D25 line, D26 outline,
  D28 scissor-to-GL mapping) behind the primitives, headless-testable
  (`tests/primitives2d_sanity.cpp`).
- `glintfx/src/layer_queue.hpp` -- the pure, single-source buffered command queue (D27) behind
  `set_layer`, headless-testable (`tests/layer_queue_sanity.cpp`).
- [Onda 5 plan](superpowers/plans/2026-07-23-onda5-draw2d-primitives.md) -- the D2D-3 design
  decisions D23-D32 behind primitives/layers/scissor/bbox/perf (this doc's own sections above).
- [ADR-0017](adr/0017-draw2d-module.md) -- the architectural decision this module implements
  (module placement, own GL pipeline, order-by-call-order composition); its
  [Addendum (2026-07-23)](adr/0017-draw2d-module.md#addendum-2026-07-23-d2d-2-coordinate-model)
  records the leader's Q1/Q2 decision behind the camera model in this doc.
- [ADR-0015](adr/0015-framework-2d-atomized-architecture.md) -- the atomized-module architecture
  this module is a slice of.
- [ADR-0013](adr/0013-gl-symbol-boundary.md) -- the `glx_`-prefixed internal GL loader Draw2D
  reuses.
- `docs/embed-integration.md` section 10 -- the window-space coordinate contract (`UiEvent`
  mouse coordinates, `get_element_box`) that this module's own coordinate contract (D5) extends.
- `docs/embed-integration.md` section 21 -- the `App` frame-hook contract Draw2D's own App
  recipe (above) builds on.
- `glintfx/src/gl_state.hpp` -- `GlStateGuard`, the class of bug D9 exists to guard against.

---

## Português

### Ciclo de vida

```cpp
glintfx::Draw2d draw2d;                                // não toca em nada ainda
draw2d.init();                                          // exige um contexto GL 3.3 core CORRENTE
glintfx::Texture2d hero = draw2d.load_texture("assets/hero.png");

draw2d.begin(target_w, target_h);                        // px de tela, topo-esquerda, y-baixo (D5)
draw2d.draw_sprite(hero, {100.f, 100.f, 64.f, 64.f});     // só dst -- textura inteira, tint branco
draw2d.end();                                             // faz flush do que estiver pendente

draw2d.destroy_texture(hero);
draw2d.shutdown();                                        // ou simplesmente deixe `draw2d` sair de escopo
```

- `Draw2d()` (`glintfx/include/glintfx/draw2d.hpp:477`) não toca em GL -- a construção é sempre
  barata e sem efeito colateral, mesma disciplina de `Audio()`/`App()`/`UiLayer()`.
- `init()` (`glintfx/include/glintfx/draw2d.hpp:457`) exige um contexto GL 3.3 core CORRENTE.
  Chama o próprio `glx_gl_load()` -- ver "Idempotência do loader GL" abaixo pro porquê disso ser
  seguro mesmo quando um `App`/`UiLayer` coabitante já carregou a mesma tabela. Retorna `false`
  em qualquer falha de GL/shader, ou quando a instância já está inicializada (chame `shutdown()`
  primeiro pra re-inicializar).
- `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:511`) é idempotente -- libera toda textura
  viva, o VBO/VAO, e o programa de shader, SÓ SE `init()` alguma vez teve sucesso, e descarta
  qualquer bracket `begin()`/`end()` aberto/parcial. O destrutor chama.
- **Estado movido-de é um no-op seguro, deliberadamente mais estrito que o `App`.** Todo método
  num `Draw2d` movido-de se comporta exatamente como uma instância nunca-inicializada (`ok()`
  retorna `false`) -- mais estrito que o próprio contrato "UB em qualquer coisa além de
  `ok()`/o destrutor após um move" do `App` (`app.hpp`), uma escolha deliberada que não custa nada
  aqui e bate com a própria disciplina fail-high deste módulo
  (`glintfx/include/glintfx/draw2d.hpp:181-185`).
- **Ciclo de vida exige contexto corrente (D11).** `shutdown()`/o destrutor liberam objetos GL
  só se `init()` alguma vez teve sucesso, e exigem que esse mesmo contexto (ou um equivalente no
  mesmo share-group) ainda esteja corrente -- mesma honestidade que o próprio contrato do
  `UiLayer` neste doc já exige (ver `docs/embed-integration.md` seção 0).

### Contrato de coordenadas (D5, REENQUADRADO pelo D2D-2)

Pixels em espaço de tela, **origem superior-esquerda, y pra baixo** -- o MESMO espaço das
coordenadas de mouse do `UiEvent` e de `UiLayer::get_element_box` (ver
`docs/embed-integration.md` seção 10): uma história de coordenadas só pra biblioteca inteira.
`src_px` é uma sub-retângulo da textura em coordenadas de **texel** (origem superior-esquerda, y
pra baixo, batendo com a ordem de linha da memória de imagem); `RectF{}` (tudo zero) é a
sentinela documentada de "textura inteira".

**Isto agora é o caso documentado SEM CÂMERA, verdadeiro por construção.** A câmera do `Draw2d`
é OPCIONAL e DEFAULT DESLIGADA (o [Adendo](adr/0017-draw2d-module.md#addendum-2026-07-23-d2d-2-coordinate-model) da ADR-0017, Q1 (c)): sem câmera setada, `draw_sprite` roda o
caminho literal da v0.20.0 acima, não uma re-derivação que por acaso bate com ele -- uma
ausência de diff, fixada adicionalmente por um teste unitário de equivalência exata (D19). Todo
call site v0.20.0 continua compilando e renderizando idêntico; nada descrito nesta seção mudou.
`set_camera(...)` (ver "Câmera" abaixo) passa os desenhos seguintes pro espaço de MUNDO -- ver
aquela seção pras fórmulas de projeção e a própria história de coordenadas.

### Câmera (D2D-2 -- espaço de mundo opcional)

`Camera2d` (`glintfx/include/glintfx/draw2d.hpp:191`) é uma câmera de mundo OPCIONAL. O ESTADO
default dela é "sem câmera" -- exatamente o contrato de coordenadas acima, não "câmera setada
pra identidade" (ver a nota de âncora abaixo pro porquê dessa distinção importar).
`set_camera(const Camera2d&)` (`glintfx/include/glintfx/draw2d.hpp:631`) passa as chamadas
`draw_sprite` seguintes, em qualquer bracket aberto ou futuro, pro espaço de MUNDO;
`reset_camera()` (`glintfx/include/glintfx/draw2d.hpp:641`) volta pro espaço de tela --
idempotente, seguro chamar sem câmera setada. `shutdown()` já reseta pra sem-câmera (D22); uma
instância recém-`init()`ada sempre começa em espaço de tela.

**Âncora -- leia isto antes de recorrer a `Camera2d{}` esperando um "no-op" (Adendo da
ADR-0017, Q2 (2a)):** `Camera2d.x`/`Camera2d.y` é o ponto do mundo mostrado no **CENTRO** do
viewport, não no canto superior-esquerdo. `cam.x = player.x; cam.y = player.y` é a linha
inteira do follow-cam, e `zoom`/`rotação` pivotam sobre o centro da vista -- o que todo
consumidor espera visualmente. O custo, dito com destaque de propósito: um `Camera2d{}`
default-construído NÃO É a transformação identidade -- passado a `set_camera`, ele centra a
origem do mundo na tela. Isto é inofensivo sob o default da Q1 (c) (o ESTADO default de um
`Draw2d` é "sem câmera", nunca "câmera setada pra identidade"), mas VAI surpreender quem chamar
`set_camera(Camera2d{})` esperando um frame inalterado.

`zoom` é px de tela por unidade de mundo (`zoom 2` = ampliado 2x; precisa ser `> 0`). `rotation`
é a orientação da PRÓPRIA câmera em **radianos** (D14) -- um valor positivo gira a câmera no
sentido horário, então o mundo parece girar anti-horário na tela; qualquer valor finito é legal,
dando wrap naturalmente por `sin`/`cos` além de `2*pi` (fixado por teste: `t` e `t + 2*pi`
produzem o mesmo mapeamento dentro da tolerância).

**Fórmulas de projeção (D12, literais -- fonte única, `glintfx/src/transform2d.hpp`,
compartilhada pelo caminho de desenho e pelos helpers públicos abaixo, então os dois não podem
divergir -- fórmula de conversão sem teste de valor computado é a classe de bug silencioso
nomeada desta casa):**

```
world_to_screen(cam, W, H, p) = rot((p - (cam.x, cam.y)) * cam.zoom, -cam.rotation) + (W/2, H/2)
screen_to_world(cam, W, H, s) = rot(s - (W/2, H/2), +cam.rotation) / cam.zoom + (cam.x, cam.y)
```

onde `rot(v, t) = (v.x*cos(t) - v.y*sin(t), v.x*sin(t) + v.y*cos(t))`
(`glintfx/src/transform2d.hpp:124`) em espaço de tela y-baixo. `screen_to_world` é o inverso
exato de `world_to_screen` (`rot(rot(v,-t),t) == v`), fixado por um teste de propriedade de
round-trip numa grade de pontos × câmeras (offsets, zooms `0.1..10`, rotações inclusive
`> 2*pi`). As duas funções -- `world_to_screen` (`glintfx/src/transform2d.hpp:136`) e
`screen_to_world` (`glintfx/src/transform2d.hpp:154`) -- também são expostas como free functions
puras e públicas (`glintfx/include/glintfx/draw2d.hpp:240-241`, cada uma uma delegação de uma
linha pro mesmo header, definidas em `glintfx/src/draw2d.cpp:1070`/`glintfx/src/draw2d.cpp:1091`):
use-as pra converter suas próprias coordenadas (um clique de mouse, por exemplo) entre espaços
sem duplicar a matemática.

**Validação (D15, o idioma do `set_dp_ratio` -- `glintfx/src/draw2d.cpp:937`):** qualquer
membro não-finito de `camera`, ou `camera.zoom <= 0`, rejeita a chamada -- o estado ANTERIOR de
câmera (inclusive "sem câmera") é MANTIDO, uma linha de log dedup'd, nunca uma escrita
parcial/corrompida. `zoom` precisa ser estritamente positivo: `zoom == 0` colapsaria o mundo e
faria `screen_to_world` dividir por zero, e zoom negativo NÃO é um idioma de espelho
deliberadamente -- `SpriteTransform.flip_h`/`flip_v` (abaixo) existem pra isso.
`set_camera`/`reset_camera` são no-ops seguros num `Draw2d`
nunca-inicializado/movido-de/pós-shutdown, o mesmo contrato null-safe de todo método desta
classe.

**Totalidade dos helpers puros (D16):** `world_to_screen`/`screen_to_world` são puros e
TOTAIS -- não podem logar. Em input inválido (qualquer membro não-finito de `cam` ou do ponto,
`cam.zoom <= 0`, ou viewport não-positivo) devolvem o PONTO DE ENTRADA INALTERADO, nunca NaN pra
fora. Este é um fallback determinístico e testado, não um caminho de erro.

**A câmera é sticky, CPU-side, e barata de trocar no meio do frame (D13):** ela é estado
armazenado do `Draw2d`, não um parâmetro por-desenho -- fica ativa por toda chamada
`draw_sprite` até o próximo `set_camera`/`reset_camera`/`shutdown()`. A matemática de câmera e
de transform por-sprite roda inteiramente na CPU, antes do batcher
(`glintfx/src/draw2d.cpp:1023-1024`) -- a camada GL e sua lista publicada de estado tocado (D9
abaixo) ficam intocadas por qualquer um dos dois, então **trocar de câmera no meio de um bracket
`begin()`/`end()` nunca força um flush GL**, e o batching por-mesma-textura sobrevive à troca
(D4 intocado). Um padrão comum -- sprites de mundo, depois um ícone de HUD em espaço de tela,
num bracket só:

```cpp
draw2d.begin(w, h);
draw2d.set_camera(world_camera);
draw2d.draw_sprite(hero, world_dst);      // espaço de mundo
draw2d.reset_camera();
draw2d.draw_sprite(hud_icon, screen_dst); // espaço de tela, mesmo bracket
draw2d.end();
```

**`camera_from_world_rect` (D17, genérica, a única conveniência cortável --
`glintfx/include/glintfx/draw2d.hpp:242`, `glintfx/src/transform2d.hpp:175`):**
`Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h)`
devolve a câmera que mostra `world_rect` ajustado por LARGURA (`zoom = viewport_w /
world_rect.w`, centro = o próprio centro do retângulo); a cobertura vertical segue o próprio
aspecto do viewport -- um chamador querendo um fit letterbox-exato compõe isto com
`UiLayer::set_viewport` (`docs/embed-integration.md` seção 10), que já existe. Fail-high:
`world_rect` não-finito, `world_rect.w <= 0 || world_rect.h <= 0`, ou viewport não-positivo
devolvem `Camera2d{}` (documentado, não um crash).

### SpriteTransform (D18 -- pivô/rotação/escala/flip por-sprite)

`SpriteTransform` (`glintfx/include/glintfx/draw2d.hpp:222`) é ortogonal à câmera e vale nos
DOIS espaços -- unidades de mundo com câmera setada, px de tela caso contrário (D5/Q1 (c)) --
via o overload `draw_sprite(tex, dst, src_px, tint, transform)`
(`glintfx/include/glintfx/draw2d.hpp:628-629`, definido em `glintfx/src/draw2d.cpp:621`).
`origin_x`/`origin_y` são o pivô, NORMALIZADO dentro de `dst` (padrão `0.5/0.5` = centro);
`rotation` é radianos sobre o pivô (D14, independente de qualquer rotação de câmera);
`scale_x`/`scale_y` são sobre o pivô (padrão `1`; negativo é LEGAL -- espelha a geometria, o
giro é irrelevante porque o D9 já desliga o cull de backface); `flip_h`/`flip_v` trocam UV no
sub-retângulo `src_px` RESOLVIDO (`glintfx/src/draw2d.cpp:1050-1066`), então um frame de atlas
espelha dentro de si mesmo e nunca vaza pra um frame vizinho.

**Matemática de canto, ordem literal (D18, `glintfx/src/transform2d.hpp:210`, fonte única com a
identidade que também prova a fixação de compatibilidade do D19):**

```
pivô P = (dst.x + origin_x*dst.w, dst.y + origin_y*dst.h)
canto' = P + rot((canto - P) * (scale_x, scale_y), rotation)     para cada um de TL,TR,BR,BL
```

**A ordem importa: ESCALA primeiro, DEPOIS rotação, ambas sobre o pivô.** Trocar essa ordem dá
um resultado visualmente diferente (e errado) pra qualquer escala não-uniforme combinada com
rotação não-zero -- é a primeira linha que o review adversarial está orientado a mutar. Com uma
câmera setada, `dst` e os cantos resultantes são unidades de MUNDO e cada canto é então
projetado individualmente por `world_to_screen` (D12); sem câmera são px de tela diretamente --
"uma história de composição só, sem caso especial" (D18).

**Compatibilidade (D19): o `draw_sprite` de 4 args puro fica INALTERADO pela existência deste
overload.** Continua chamando o caminho v0.20.0 do batcher verbatim, com ou sem câmera -- essa
ausência-de-diff É a garantia de compatibilidade da Q1 (c), fixada adicionalmente por um teste
unitário de equivalência exata: um `SpriteTransform{}` identidade sem câmera produz vértices
BIT-IDÊNTICOS ao caminho antigo (igualdade de float exata legítima aqui -- CPU determinístico, e
operações identidade `*1.0f`, `+0.0f`, `cos(0)=1`, `sin(0)=0` são IEEE-exatas).

**Fail-high (D20, D10 estendido):** todo membro de `transform` finito ANTES de qualquer conta
(NaN/Inf em qualquer lugar pula o desenho, log dedup'd, mesma classe `NonFinite` de
`dst`/`src_px`/`tint`); `dst.w <= 0 || dst.h <= 0` continua o no-op legal silencioso que o
overload de 4 args já tem (inalterado). DEPOIS da matemática de canto, uma guarda a mais rejeita
um canto COMPUTADO não-finito (overflow -- ex.: um `zoom` enorme vezes uma coordenada enorme
multiplicando pra `Inf`, `glintfx/src/draw2d.cpp:1044-1041`). Esta guarda pós-conta NÃO tem gêmeo
na v0.20.0, por design: o caminho antigo axis-aligned não tem multiplicação que possa estourar,
então não havia nada pra guardar contra lá -- confirmado como análise, não como omissão, pelo
review adversarial.

### Primitivas (D23-D26 -- desenho não-texturizado)

`draw_filled_rect`, `draw_filled_quad`, `draw_line`, e `draw_rect_outline`
(`glintfx/include/glintfx/draw2d.hpp:705`/`616`/`637`/`668`/`699`) desenham formas
não-texturizadas pelo MESMO pipeline batchado do `draw_sprite` -- sem segundo shader, sem
classe de estado GL nova (decisão A1 (a), D23). O `init()` cria uma textura branca RGBA8
premultiplicada 1x1 interna (`Impl::create_white_texture()`, `glintfx/src/draw2d.cpp:405`) como
slot NORMAL do registry (id 1; o handle público `Texture2d` dela nunca é construído, então um
consumidor não pode nem desenhar com ela nem destruí-la). Falha de criação falha o próprio
`init()` (fail-high: um `Draw2d` que não consegue desenhar as primitivas que anuncia não
meio-inicializa). Consequência, dita: o primeiro `load_texture()` do consumidor agora devolve o
id 2, inobservável pela API pública (ids são opacos).

Toda primitiva compartilha a história de composição do D24 com `draw_sprite`/`SpriteTransform`:
a geometria é construída no espaço ATIVO (unidades de mundo com câmera setada, px de tela caso
contrário, D5/D18), cada canto é então projetado por `world_to_screen` quando há câmera, depois
passa pela guarda pós-conta de finitude do D20, depois entra no MESMO batcher como um
`draw_quad` contra a textura branca com a cor da primitiva como o tint D8 -- sobre aquele texel
branco, a fórmula D8 (`out = texel_premult * vec4(tint.rgb*tint.a, tint.a)`, ver "Premultiply e
a fórmula do tint" abaixo) produz EXATAMENTE `color` em espaço premultiplicado, sem fórmula
separada. Consequência: a espessura de `draw_line`/`draw_rect_outline` é em unidades do espaço
ATIVO e escala com `cam.zoom` SÓ POR PROJEÇÃO -- a necessidade medida do consumidor de
"espessura de contorno/linha em unidade de mundo, escalando com a câmera" sai desta regra geral,
sem API sob medida. Toda primitiva compartilha o template fail-high do sprite: um membro de
input não-finito pula o desenho (log dedup'd) ANTES de qualquer conta; um tamanho degenerado
(`dst.w<=0||dst.h<=0`, o `a==b` ou `thickness<=0` de uma linha, o `thickness<=0` de um contorno)
é um no-op legal SILENCIOSO; a guarda pós-conta de canto (D20) cobre overflow, igual aos
sprites. Cada primitiva é marcada pela camada CORRENTE (D27) e viaja com o scissor CORRENTE
(D28) exatamente como o `draw_sprite`.

`draw_filled_rect` tem um overload de 4 args e um overload de `SpriteTransform` (mesma
composição de pivô/rotação/escala/flip do próprio overload de transform do `draw_sprite`,
"SpriteTransform" acima -- `flip_h`/`flip_v` são aceitos mas não têm efeito visual sobre um
preenchimento sólido). `draw_filled_quad` recebe quatro cantos fornecidos pelo chamador na ordem
TL,TR,BR,BL (convenção D5/D19); um quad degenerado ou auto-cruzado (bowtie) é LEGAL e desenha
exatamente os dois triângulos que os cantos descrevem (negócio da própria GPU).

**Geometria de linha (D25, literal -- fonte única `glintfx/src/primitives2d.hpp:203`,
`compute_line_corners()`):** com `d = b - a`, `len = |d|`, unitário `u = d/len`, normal
`n = (-u.y, u.x)`, `h = thickness/2`:

```
tl = a + n*h,  tr = b + n*h,  br = b - n*h,  bl = a - n*h
```

Tampas butt -- o quad termina exatamente em `a` e `b`; tampas round/miter e polylines são a
semente `SEED-D2D-LINECAPS` (INBOX do `TODO.md`), fora desta onda. Ordem de guarda
(`is_degenerate_line()`, `glintfx/src/primitives2d.hpp:183`): `a`/`b`/`thickness`/`color` todos
finitos ANTES de qualquer conta (o `sqrt` incluso), DEPOIS `a == b` (exatamente) ou
`thickness <= 0` é checado -- nenhum caminho de divisão-por-zero existe em
`compute_line_corners()` porque o chamador nunca a invoca quando degenerado.

**Contorno de retângulo (D26, literal -- fonte única `glintfx/src/primitives2d.hpp:247`,
`compute_outline_strips()`):** quatro faixas SEM SOBREPOSIÇÃO dentro de `dst`, com o clamp de
colapso `t = min(thickness, min(dst.w, dst.h)/2)`:

```
top    = {x,       y,        w,      t}
bottom = {x,       y+h-t,    w,      t}
left   = {x,       y+t,      t,      h-2t}
right  = {x+w-t,   y+t,      t,      h-2t}
```

Não-sobreposição é uma propriedade de CORREÇÃO, não uma micro-otimização: com uma cor de
contorno translúcida, faixas sobrepostas dariam double-blend nos cantos (quadrados visivelmente
mais escuros). A MESMA fórmula clampada, sem ramo de caso especial, ladrilha `dst` EXATAMENTE
quando `2*thickness >= min(dst.w, dst.h)` -- o contorno degenera pra UM retângulo preenchido
cobrindo `dst`. Cada uma das 4 faixas é projetada e submetida independentemente: um canto
PROJETADO não-finito numa faixa pula só AQUELA faixa (logado uma vez, dedup'd), as outras ainda
desenham.

**Não entregue, semeado:** primitivas de círculo/elipse/polígono-regular (`SEED-D2D-SHAPES` -- o
decorator RCSS `polygon()` do átomo `ui` já cobre o lado declarativo); sort secundário por
textura dentro de uma camada (`SEED-D2D-LAYER-TEXSORT`).

### Layers (D27 -- ordem de desenho explícita, opt-in)

`set_layer(int layer)` (`glintfx/include/glintfx/draw2d.hpp:844`) é OPT-IN, default DESLIGADO:
um bracket que nunca chama `set_layer` roda o caminho streaming LITERAL da v0.21.0 inalterado --
o MESMO idioma de "ausência de diff" que a câmera (D13) e o contrato de coordenadas (D5) já usam
neste módulo, aplicado uma terceira vez. Qualquer `int` é legal (negativo ordena abaixo do `0`
default); a camada CORRENTE marca toda chamada `draw_sprite`/primitiva seguinte, sticky DENTRO
do bracket.

**Armar:** a PRIMEIRA chamada `set_layer` arma o modo bufferizado do bracket CORRENTE (ou do
PRÓXIMO, se chamada fora de um bracket). Se já havia desenhos streamados no bracket corrente,
eles são finalizados e drenados pelo caminho streaming ANTIGO PRIMEIRO
(`SpriteBatch::flush_pending()`, `glintfx/src/sprite_batch.hpp:261`, o único método aditivo do
D31) -- renderizam abaixo de toda camada, efetivamente "debaixo da ponte". Recomendação: chame
`set_layer` antes do primeiro desenho de um bracket pra evitar esse split.

**Buffering e replay:** um comando bufferizado captura o estado FINAL -- cantos em espaço de
tela pós-validação/pós-projeção, id de textura (branca pra primitivas), `src_px` resolvido, tint
clampado, o snapshot de scissor ATIVO no momento do push (D28), e a tag de camada; a câmera é
portanto amostrada NO MOMENTO DA CHAMADA, exatamente como o modo streaming. No `end()`, comandos
são ordenados de forma estável por camada (`std::stable_sort`, `glintfx/src/layer_queue.hpp:206`
-- camada igual mantém ordem de submissão, GARANTIDO) e reproduzidos pelo batcher INTOCADO,
agrupados em corridas maximais de comandos consecutivos de mesmo scissor (`scissor_equal()`,
`glintfx/src/layer_queue.hpp:111`); o `end()` então RESETA pra streaming/camada 0 pro PRÓXIMO
bracket -- camadas NÃO são sticky entre brackets como a câmera/scissor, porque (diferente
delas) mudam o próprio timing de flush do módulo.

**Camadas ordenam desenhos SÓ DENTRO de um bracket do Draw2D.** A composição com a UI ou com as
próprias chamadas GL do host continua ordem-de-chamada (eixo 3 da ADR-0017, intocado) -- ainda
não existe API de ordem cross-módulo.

**Memória:** o buffering segura os comandos de UM bracket até o `end()`, teto rígido de 262144
(`LayerQueue::kMaxCommands`, `glintfx/src/layer_queue.hpp:166`, ~24 MiB a ~96 B/comando) -- um
limite de MEMÓRIA contra um stream hostil (mesma classe do próprio limite de flush em 4096 quads
do batcher, D4), NÃO um claim de performance -- ver "Performance" abaixo; passado o teto,
desenhos são descartados, logado uma vez, dedup'd. `shutdown()` descarta qualquer buffer aberto.

### Scissor (D28 -- recorte retangular)

`set_scissor(const RectF& rect_px)` / `reset_scissor()`
(`glintfx/include/glintfx/draw2d.hpp:889`/`792`) recortam o desenho a um retângulo, SEMPRE em px
de TELA -- origem superior-esquerda, y pra baixo, o MESMO espaço do tamanho-alvo do `begin()` --
câmera-INDEPENDENTE (A3): um recorte de retângulo de mundo sob rotação é uma feature classe
máscara/stencil, não isto (semente `SEED-D2D-WORLDCLIP`). Sticky entre desenhos E brackets (como
a câmera) até `reset_scissor()`/`shutdown()`.

**Validação (idioma do D15, `glintfx/src/draw2d.cpp:1558`):** qualquer membro não-finito de
`rect_px` REJEITA a chamada, o estado ANTERIOR de scissor é MANTIDO, um log dedup'd.
`rect_px.w <= 0 || rect_px.h <= 0` é LEGAL e significa "clipa tudo" (semântica GL -- ex.: uma
animação de revelação colapsando). x/y negativos são LEGAIS (clampar é trabalho da GPU).

**Semântica de fronteira de flush, contraste nomeado com a câmera (D13):** a câmera nunca força
um flush GL (D13); uma mudança de scissor no meio do bracket em modo STREAMING FORÇA -- a
corrida pendente é finalizada e drenada sob o scissor VELHO primeiro
(`flush_pending()`+dreno, o mesmo método do D31 que o `set_layer` já usa), depois o estado
atualiza. Em modo BUFFERIZADO nada é forçado: o PRÓPRIO snapshot de scissor de cada comando
viaja com ele (capturado no momento do push), e o replay já agrupa comandos consecutivos de
mesmo scissor -- mesma semântica observável.

**Mapeamento GL no flush, literal (a conversão de y-flip -- fonte única, `map_scissor_to_gl()`,
`glintfx/src/primitives2d.hpp:272`):**

```
glScissor(x, target_h - (y + h), w, h)
```

com `w`/`h` (a "extensão" GL) clampados a não-negativo ANTES do cast pra int; `x`/`y` (a origem)
são convertidos direto, sem clamp, pela própria regra "clampar é trabalho da GPU" do D28.

**O ÚNICO diff comportamental declarado em código existente (D31), e a atualização de contrato
D9 que ele força:** o `set_gl_state_for_draw()` (`glintfx/src/draw2d.cpp:458`) costumava chamar
`glDisable(GL_SCISSOR_TEST)` incondicionalmente, a cada flush. Agora é condicional
(`glintfx/src/draw2d.cpp:463-474`): se um scissor está setado, `glEnable(GL_SCISSOR_TEST)` +
`glScissor(retângulo mapeado)`; senão `glDisable(GL_SCISSOR_TEST)`, ainda afirmando TUDO de que
este estado depende a cada flush, pela própria regra de não-cachear do D9, inalterada. Sem
scissor NUNCA setado, os GL calls emitidos são IDÊNTICOS à v0.21.0. O contrato de estado GL
(D9, abaixo) publica uma linha de estado tocado ATUALIZADA pra isto.

**Aviso de severidade alta pra hosts embed -- leia isto antes de embarcar uma chamada de
scissor:** o Draw2D agora pode deixar `GL_SCISSOR_TEST` LIGADO pra trás (continua não
restaurando nada, por contrato, D9). No `App`, o `GlStateGuard` do hook de frame já captura e
restaura a caixa+enable de scissor (`glintfx/src/gl_state.hpp:68`/`72`/`129`/`131`, verificado,
não assumido) -- coberto de graça. **EM MODO EMBED esta é a aresta mais afiada de toda a onda
D2D-3 pros hosts:** um scissor esquecido clipa o PRÓXIMO passe GL do host, INCLUSIVE o próprio
clear dele. Resete o próprio scissor, ou chame `reset_scissor()`, antes de entregar o frame de
volta ao host. `draw2d_ui_coexist_sanity.cpp` fixa que o próprio `UiLayer::render()` não é
afetado (o próprio `GlStateGuard` já reestabelece estado) -- mas um host fazendo GL cru PRÓPRIO
depois de um passe Draw2D não tem essa guarda a menos que escreva uma.

### Bbox de conteúdo de textura (D29)

`texture_content_bbox(const Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:929`) devolve
`TextureBbox{found, x, y, w, h}` (`glintfx/include/glintfx/draw2d.hpp:340`, precedente da casa:
`ElementBox{found,x,y,w,h}` de `UiLayer::get_element_box`) -- o menor retângulo contendo todo
texel com `alpha > 0` (semi-transparente conta; um parâmetro de LIMIAR de alpha é a semente
`SEED-D2D-BBOX-THRESH`). Coordenadas no MESMO espaço de texel de `src_px` (origem
superior-esquerda, y pra baixo, batendo com a ordem de linha da memória de imagem, D5) -- o
padrão hitbox-a-partir-de-bbox compõe direto com o `src_px` do `draw_sprite`.

Computado UMA vez, dentro do `load_texture()` (`compute_content_bbox()`,
`glintfx/src/image_decode.hpp` -- sobre os pixels decodificados que já existem na CPU ali,
descartados logo depois) e cacheado no slot do registry da textura (16 bytes) -- NÃO retido como
buffer RGBA completo (um custo de memória classe-256 MiB pra uma consulta pouco usada), NÃO um
readback `glGetTexImage` sob demanda (um round-trip de GL e uma armadilha de portabilidade
GLES/core-profile). O premultiply NÃO perturba isto: o canal alpha fica inalterado pela
premultiplicação.

`found == false` (todo outro campo zero) em: um handle `tex` inválido/obsoleto/estrangeiro/
nunca-carregado (a cadeia completa de validação D7, log dedup'd -- mesma história de rejeição do
`draw_sprite`) OU uma textura totalmente transparente -- distinta de uma textura opaca 1x1, que
devolve `{true, 0, 0, 1, 1}`.

### Referência de API

| Método | Retorna | Notas |
| :--- | :--- | :--- |
| `init()` (`glintfx/include/glintfx/draw2d.hpp:457`) | `bool` | Exige um contexto GL 3.3 core corrente. `false` em falha de GL/shader ou se já inicializado. |
| `ok()` (`glintfx/include/glintfx/draw2d.hpp:502`) | `bool` | `true` entre um `init()` bem-sucedido e o próximo `shutdown()`/destruição. |
| `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:511`) | `void` | Teardown GL idempotente. Seguro numa instância nunca inicializada. |
| `load_texture(const char* path)` (`glintfx/include/glintfx/draw2d.hpp:541`) | `Texture2d` | PNG/JPG/TGA/BMP (o que o decode apoiado em stb_image do `image_decode.hpp` reconhecer), premultiplicado no upload. `path` lido via overload de ifstream de `const char*` puro (D7, MSVC-safe), teto de 256 MiB. `ok() == false` em `nullptr`, falha ao abrir, o teto, ou falha de decode -- nunca um crash. |
| `destroy_texture(Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:558`) | `void` | Libera a textura GL se houver, SEMPRE zera `tex` ao retornar. Fail-high (nunca UB) em handle já-destruído, adulterado, ou estrangeiro. |
| `begin(int target_width, int target_height)` (`glintfx/include/glintfx/draw2d.hpp:577`) | `void` | Abre um bracket de batching (D4). `target_width`/`target_height` são o tamanho de viewport em px de tela contra o qual os sprites deste bracket projetam -- o MESMO tamanho pro qual o próprio viewport GL do host está definido neste passe. `begin()` aninhado encerra o bracket anterior implicitamente primeiro (fazendo flush dele, logado uma vez). |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px = RectF{}, const ColorF& tint = ColorF{})` (`glintfx/include/glintfx/draw2d.hpp:611-612`) | `void` | Enfileira um quad texturizado, batchado internamente por textura. Ver "Batching" e "Superfície de entrada fail-high" abaixo pra lista completa de guardas. |
| `set_camera(const Camera2d& camera)` (`glintfx/include/glintfx/draw2d.hpp:631`) | `void` | Passa os desenhos seguintes pro espaço de mundo (D2D-2, Q1 (c)). Fail-high (D15): mantém o estado anterior em membro não-finito ou `zoom <= 0`. Ver "Câmera" acima. |
| `reset_camera()` (`glintfx/include/glintfx/draw2d.hpp:641`) | `void` | Devolve os desenhos seguintes pro espaço de tela (o default sem câmera). Idempotente. Ver "Câmera" acima. |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px, const ColorF& tint, const SpriteTransform& transform)` (`glintfx/include/glintfx/draw2d.hpp:670-671`) | `void` | Overload D18 -- pivô/rotação/escala/flip, no espaço que estiver ativo. Ver "SpriteTransform" acima. |
| `end()` (`glintfx/include/glintfx/draw2d.hpp:679`) | `void` | Fecha o bracket aberto por `begin()`, fazendo flush do que ainda estiver pendente -- incondicionalmente, mesmo um bracket vazio. Seguro sem um `begin()` correspondente. |
| `Texture2d::ok()` (`glintfx/include/glintfx/draw2d.hpp:294`) | `bool` | `false` quando nunca carregado, quando `load_texture()` falhou, ou depois de `destroy_texture()` zerar este handle. |
| `Texture2d::width()`/`height()` (`glintfx/include/glintfx/draw2d.hpp:297-298`) | `int` | Dimensões em pixel da imagem decodificada. `0` quando `!ok()`. |
| `world_to_screen(const Camera2d&, int, int, Vec2F)` (`glintfx/include/glintfx/draw2d.hpp:261`) | `Vec2F` | Free function pura e total (D12/D16). Ver "Câmera" acima. |
| `screen_to_world(const Camera2d&, int, int, Vec2F)` (`glintfx/include/glintfx/draw2d.hpp:262`) | `Vec2F` | Free function pura e total (D12/D16), inverso exato de `world_to_screen`. |
| `camera_from_world_rect(const RectF&, int, int)` (`glintfx/include/glintfx/draw2d.hpp:263`) | `Camera2d` | D17, conveniência genérica de fit-por-retângulo. Input hostil devolve `Camera2d{}`. |
| `draw_filled_rect(const RectF&, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:705`) | `void` | D23/D24 -- retângulo preenchido não-texturizado, espaço ATIVO. Ver "Primitivas" acima. |
| `draw_filled_rect(const RectF&, const ColorF&, const SpriteTransform&)` (`glintfx/include/glintfx/draw2d.hpp:720`) | `void` | Overload D18 -- pivô/rotação/escala/flip, mesma composição do overload de transform do `draw_sprite`. |
| `draw_filled_quad(Vec2F, Vec2F, Vec2F, Vec2F, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:741`) | `void` | D24 -- quad arbitrário, cantos na ordem TL,TR,BR,BL. Quads bowtie são legais. |
| `draw_line(Vec2F, Vec2F, float, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:772`) | `void` | D25 -- segmento de linha com tampa butt, `thickness` em unidades do espaço ATIVO. Ver "Primitivas" acima. |
| `draw_rect_outline(const RectF&, float, const ColorF&)` (`glintfx/include/glintfx/draw2d.hpp:803`) | `void` | D26 -- 4 faixas sem sobreposição, clamp de colapso em `2*thickness >= min(w,h)`. Ver "Primitivas" acima. |
| `set_layer(int)` (`glintfx/include/glintfx/draw2d.hpp:844`) | `void` | D27 -- ordem de desenho bufferizada opt-in. `end()` reseta pra streaming/camada 0. Ver "Layers" acima. |
| `set_scissor(const RectF&)` (`glintfx/include/glintfx/draw2d.hpp:889`) | `void` | D28 -- SEMPRE px de tela, câmera-independente. Fail-high mantém estado anterior em input não-finito. Ver "Scissor" acima. |
| `reset_scissor()` (`glintfx/include/glintfx/draw2d.hpp:896`) | `void` | D28 -- idempotente, limpa o estado de scissor setado pelo `set_scissor`. |
| `texture_content_bbox(const Texture2d&)` (`glintfx/include/glintfx/draw2d.hpp:929`) | `TextureBbox` | D29 -- menor retângulo `alpha>0`, cacheado no momento do `load_texture()`. Ver "Bbox de conteúdo de textura" acima. |

### Premultiply e a fórmula do tint (D8)

`load_texture()` decodifica e **premultiplica o alpha** da imagem no upload
(`glintfx/src/image_decode.hpp:150`, `R = R*A/255`, `G = G*A/255`, `B = B*A/255`, `A` inalterado)
pra bater com o blend deste módulo, `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`
(`glintfx/src/draw2d.cpp:369`). `ColorF` (`glintfx/include/glintfx/draw2d.hpp:142`) é RGBA
**straight** (não-premultiplicado) em `[0, 1]`, cada canal clampado na entrada -- o padrão é
branco opaco (identidade: uma amostra sem modificação). O estágio de fragmento
(`glintfx/src/draw2d.cpp:137-146`) aplica a modulação exata:

```
out = texel_premult * vec4(tint.rgb * tint.a, tint.a)
```

`texel_premult` já está alpha-premultiplicado no upload, então `tint.rgb * tint.a` premultiplica
o tint straight pro mesmo espaço antes de modular, e `tint.a` sozinho modula o canal alpha (já
premultiplicado). `tint.a` esmaece o sprite como um todo; `tint.rgb` escala a cor. Esta fórmula
também é fixada por um teste de pixel-readback com um valor esperado **calculado**
(`draw2d_render_sanity.cpp`, conforme a regra de teste do plano: uma fórmula sem teste de delta
que a exercite é a classe de bug silencioso nomeada desta casa).

### Contrato de estado GL (D9 -- a decisão anti-bug-`GlStateGuard`)

O Draw2D **seta todo pedaço de estado GL de que depende a cada flush interno**
(`set_gl_state_for_draw()`, `glintfx/src/draw2d.cpp:362-376`) e **não assume nada** deixado por
um renderer coabitante -- o backend GL3 do RmlUi compartilha este mesmo contexto GL. Lista
enumerada do estado tocado (mesmo estilo da lista capturada em `gl_state.hpp`,
`glintfx/src/gl_state.hpp:2-6`):

- Programa de shader (`glUseProgram`)
- Vertex array object (`glBindVertexArray`)
- Depth test (explicitamente `glDisable(GL_DEPTH_TEST)`)
- Face culling (explicitamente `glDisable(GL_CULL_FACE)`)
- Scissor test (CONDICIONAL desde o D2D-3/D28, `glintfx/src/draw2d.cpp:463-474`: `glEnable(GL_SCISSOR_TEST)` + `glScissor(...)` quando um scissor está setado via `set_scissor`, senão `glDisable(GL_SCISSOR_TEST)` como antes -- ver "Scissor" acima pra MUDANÇA DE CONTRATO que isto representa)
- Blend enable (`glEnable(GL_BLEND)`)
- Blend func, RGB e alpha (`glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`)
- Blend equation (`glBlendEquation(GL_FUNC_ADD)`)
- Unidade de textura ativa (`glActiveTexture(GL_TEXTURE0)`)
- Binding de textura 2D naquela unidade (`glBindTexture(GL_TEXTURE_2D, ...)`)
- Os uniforms de shader `uTex`/`uTargetSize`

**Mudança de contrato do D2D-3, dita em voz alta:** antes da v0.22.0, a linha de scissor acima
era incondicional (`glDisable(GL_SCISSOR_TEST)`, todo flush, sem exceção) -- um scissor que
nunca liga scissor não existe, então esta linha mudou. Sem scissor NUNCA setado
(`set_scissor` nunca chamado) o stream GL emitido é byte-idêntico à v0.21.0; no momento em que
um host chama `set_scissor`, o Draw2D pode deixar `GL_SCISSOR_TEST` LIGADO pra trás depois do
`end()`. Ver "Scissor" acima pro contrato completo de mapeamento/fronteira-de-flush e o perigo
em modo embed.

**O Draw2D NÃO restaura nada disso depois do `end()`.** É deliberado, não um esquecimento --
restaurar é trabalho do contrato que envolve, e os dois contratos que envolvem já existem:

- **No `App`**, o estado do hook de frame é auto-restaurado antes do passe de render de UI rodar
  (o contrato do `set_frame_callback`, `glintfx/include/glintfx/app.hpp:506` -- ver
  `docs/embed-integration.md` seção 21). Chamadas de Draw2D dentro do hook são cobertas pela
  mesma restauração, de graça.
- **No modo embed**, `UiLayer::render()` reestabelece o próprio estado GL completo dentro do
  `GlStateGuard` independente do que rodou antes. O que o Draw2D deixa para trás após o próprio
  passe de desenho do host é responsabilidade do host gerenciar -- exatamente como as próprias
  chamadas GL cruas do host hoje.

Este contrato é fixado por um teste de coexistência nas DUAS ordens (passe de sprite depois passe
de UI, e passe de UI depois passe de sprite -- `draw2d_ui_coexist_sanity.cpp`), o teste que guarda
o risco central nomeado desta onda: um caminho GL novo coabitando o contexto do renderer GL3 do
RmlUi é exatamente a classe de bug que o `GlStateGuard`
(`glintfx/src/gl_state.hpp`) existe pra guardar contra.

### Receitas de composição

Sem API de ordem nova -- a ordem de composição é ordem de chamada (ADR-0017 eixo 3). Os dois
modos de hospedagem reusam um ponto de inserção que já existe.

**`App` -- dentro do hook de frame, sob a UI:**

```cpp
glintfx::App app;
// ... app.load(...) ...
glintfx::Draw2d draw2d;
draw2d.init();
glintfx::Texture2d hero = draw2d.load_texture("assets/hero.png");

app.set_frame_callback([&](float dt_seconds) {
  draw2d.begin(app_width, app_height);   // o MESMO tamanho que o próprio viewport do App usa neste passe
  draw2d.draw_sprite(hero, {player_x, player_y, 64.f, 64.f});
  draw2d.end();
});
app.run();
```

O hook de frame roda depois do clear do frame e antes do passe de render de UI
(`glintfx/include/glintfx/app.hpp:506`), então sprites desenhados aqui compõem **sob** a UI --
não há ponto de inserção pós-UI nesta fatia (ver "Limites declarados" abaixo).

**Embed -- antes ou depois do `ui_layer.render()`, escolha do host:**

```cpp
// mundo sob a UI (o padrão de produção do GusWorld):
draw2d.begin(viewport_w, viewport_h);
draw2d.draw_sprite(hero, dst);
draw2d.end();
ui_layer.render();

// -- OU, para um overlay desenhado POR CIMA da UI --
ui_layer.render();
draw2d.begin(viewport_w, viewport_h);
draw2d.draw_sprite(hud_icon, dst);
draw2d.end();
```

As duas ordens são legais; o host controla exatamente como já controla qualquer uma das próprias
chamadas GL cruas (o D9 acima cobre quem restaura o quê).

### Handles de textura (D7)

`Texture2d` (`glintfx/include/glintfx/draw2d.hpp:287`) é um handle-valor simples, copiável --
NÃO é dono RAII (`destroy_texture()` é a liberação explícita, mesmo idioma do `Audio::SoundId`).
Internamente carrega um id opaco, um contador de geração, e uma **tag de dono** (a identidade
própria do `Draw2d` emissor): uma instância `Draw2d` valida todo handle contra o próprio registry
interno antes de usar.

**Um handle NÃO é intercambiável entre instâncias `Draw2d`.** `load_texture()` carimba o handle
retornado com a identidade da instância emissora (`glintfx/src/draw2d.cpp:1073`); tanto
`draw_sprite()` / `destroy_texture()` (`glintfx/src/draw2d.cpp:1144`/`728`) rejeitam um handle cuja
tag `owner_` não bate com `this`, logado uma vez e tratado exatamente como qualquer outro handle
desconhecido/obsoleto -- nunca desreferenciado como nome GL cru. É uma garantia por construção,
não uma coincidência numérica:
id e geração sozinhos não conseguem distinguir "id 3, geração 1 no registry DESTA instância" do
mesmo par no registry independente de uma instância *diferente*.

### Batching (D4)

`begin(target_w, target_h)` ... N x `draw_sprite(...)` ... `end()`. O batcher
(`glintfx::draw2d_detail::SpriteBatch`, `glintfx/src/sprite_batch.hpp:75`) acumula quads enquanto
a textura permanece igual à chamada anterior e faz flush em: uma troca de textura, no `end()`, ou
ao atingir uma capacidade interna de **4096 quads**
(`glintfx/src/sprite_batch.hpp:151-159`) -- um limite de MEMÓRIA contra um stream hostil/
degenerado de chamadas, transparente ao chamador. **Isto é um limite de memória, não um claim de
performance** -- ver "Limites declarados" abaixo pro D2D-3.

- `draw_sprite()` fora de um bracket `begin()`/`end()`: fail-high (logado uma vez, dedup'd),
  no-op.
- `begin()` aninhado: o bracket anterior é encerrado implicitamente (fazendo flush do que tinha
  acumulado), logado uma vez, dedup'd -- desenhos pendentes nunca são descartados em silêncio.

Toda DECISÃO de batching mora no `sprite_batch.hpp` puro (zero chamada GL: recebe comandos de
desenho e emite dado de vértice + faixas de flush, testável headless); `draw2d.cpp` é dono só da
execução GL do que o batcher puro decidiu.

### Superfície de entrada fail-high (D10)

Toda guarda abaixo é checada ANTES de qualquer cast ou aritmética tocar a entrada
(`sprite_batch.hpp:132-134`):

- Qualquer membro NaN/Inf em `dst`/`src_px`/`tint` -> pula o desenho (logado uma vez, dedup'd).
- `dst.w <= 0 || dst.h <= 0` -> no-op silencioso (um sprite degenerado, não um erro -- não
  logado).
- `dst.x`/`dst.y` negativo -> **legal** (recorte fora-da-tela é trabalho da GPU).
- `src_px` fora dos limites da textura -> clampado, não rejeitado (documentado: um frame
  meio-fora-do-atlas sob jitter de matemática de animação é um caso legítimo comum,
  `glintfx/src/sprite_batch.hpp:209-214`).
- `begin(target_w, target_h)` com algum `<= 0` -> o bracket é aceito (ainda precisa ser fechado
  por `end()`) mas todo `draw_sprite()` dentro dele é um no-op silencioso até lá, logado UMA VEZ
  no `begin()`, não por `draw_sprite()`.
- Um handle `Texture2d` desconhecido/obsoleto/adulterado/estrangeiro -> rejeitado (logado uma
  vez, dedup'd), nunca desreferenciado como nome GL cru.

O overload de `draw_sprite` que recebe um `SpriteTransform` (D2D-2) estende esta lista com a
própria guarda em dois estágios (membro finito antes de qualquer conta, depois uma guarda
pós-conta de canto computado) -- ver "SpriteTransform" acima (D20) em vez de duplicar aqui.

### Idempotência do loader GL

`Draw2d::init()` chama o próprio `glx_gl_load()`
(`glintfx/include/glintfx/draw2d.hpp:213-215`) -- necessário porque com `ui`/`window` OFF ninguém
mais carregaria a tabela interna de ponteiros de função GL. Isso é seguro mesmo quando um
`App`/`UiLayer` coabitante já carregou a mesma tabela: o `glx_gl_load()` resolve e sobrescreve
incondicionalmente todo slot de símbolo a cada chamada, tanto no caminho Linux
(`glintfx/src/gl_loader.c:897`) quanto no Windows (`glintfx/src/gl_loader.c:830`) -- não há
guarda interna de "já carregado" pra interagir com nada, então chamar duas vezes é um
re-resolve redundante e inofensivo, não um risco de estado. Um host não precisa ordenar o
`init()` do Draw2D em relação à própria inicialização do `App`/`UiLayer`.

### Performance (D30 -- números declarados, gates honestos)

`glintfx/tests/draw2d_perf_budget.cpp` mede três métricas, headless, impressas como linhas
machine-readable `GLINTFX_PERF <chave>=<valor>` toda execução (visíveis no log de CI independente
do resultado do gate):

1. **`pure_batcher_quads_per_s`** -- 100k quads pelo `SpriteBatch` sozinho (CPU, sem GL/display
   envolvido).
2. **`layered_streaming_ratio`** -- o MESMO stream de 100k comandos pelo caminho bufferizado
   (`LayerQueue::push` -> `drain_grouped()` -> replay via `SpriteBatch::draw_quad()`, a MESMA
   sequência que o próprio replay do `Draw2d::end()` roda), reportado como
   `time_layered / time_streaming` (`>= 1.0` -- o caminho bufferizado só pode ADICIONAR trabalho,
   reusa o MESMO batcher por baixo, o conjunto zero-diff do D31).
3. **`e2e_10k_sprite_1k_prim_median_ms`** -- um bracket real de `Draw2d` de 10k sprites (uma
   textura) mais um bracket real de 1k primitivas `draw_filled_rect`, sob um contexto GL 3.3 vivo
   no Xvfb/llvmpipe, com `glFinish()` antes do cronômetro parar -- mediana sobre 30 frames (2
   primeiros descartados como aquecimento).

**Downgrade declarado:** o llvmpipe rasteriza na CPU -- a métrica 3 é uma GUARDA DE REGRESSÃO DE
RASTER DE CPU, não uma verdade de throughput/banda/pacing-de-vsync de GPU. As métricas 1 e 2 SÃO
números reais de CPU (nenhum GL envolvido).

**O gate, dois níveis:** com `GLINTFX_PERF_STRICT=1` no ambiente (setada SÓ pelo job do runner
self-hosted `claudio` em `.forgejo/workflows/heavy.yml`, nunca por `ci.yml`/`nightly.yml`), as
métricas 1 e 3 FALHAM se cruzarem a própria constante BASELINE em mais de 1,5x. Sem a env
(qualquer outro runner, qualquer máquina de dev), só um teto de SANIDADE grosseiro de 10x falha o
teste -- o número é impresso de qualquer forma. O gate de RATIO (métrica 2) é MACHINE-RELATIVE
por construção (os dois braços rodam na mesma máquina, mesmo processo, mesma invocação), então é
ESTRITO EM TODO LUGAR, incondicional a `GLINTFX_PERF_STRICT` -- é a resposta direta e sempre-ligada
ao próprio risco 2 desta onda ("layers vs batching é onde a performance pode degradar").

**Um achado real, reportado e não escondido:** o gate de razão mede ~3,4-3,6x na máquina que
mediu as baselines desta onda, de forma consistente em corridas repetidas -- acima do teto de
2,0x que esta onda setou primeiro. Causa-raiz: o `std::stable_sort` (`glintfx/src/layer_queue.hpp:206`)
de `LayerQueue::drain_grouped()` move structs `LayerCommand`
inteiras (~96 B) pra reordenar por uma chave `int layer` só, dominando o custo do caminho em
camadas (aproximadamente 60% dele numa sonda avulsa) -- o próprio replay do `Draw2d::end()` roda
a EXATA mesma `drain_grouped()` que este benchmark roda, então não é artefato de benchmark. Uma
ordenação por chave leve foi tentada e revertida: media melhor sob gcc mas PIOR sob clang -- o
único compilador que todo workflow de CI deste repo de fato pina
(`-DCMAKE_CXX_COMPILER=clang++`). A decisão da casa: manter o `std::stable_sort` simples e
REVISAR O TETO pra **4,0x** em vez de perseguir uma micro-otimização específica de compilador
que regride no compilador que o CI de fato roda -- o modo em camadas é opt-in por construção (o
caminho streaming intocado não paga nada disso), então ~3,4x medido COM CLANG é o preço honesto
dessa feature, não uma regressão a perseguir mais.

**Números declarados, proveniência honesta** (`glintfx/tests/draw2d_perf_budget.cpp`, constantes
de baseline datadas no próprio arquivo): baseline de `pure_batcher_quads_per_s` = **8 500 000**,
baseline de `e2e_10k_sprite_1k_prim_median_ms` = **5,0**, teto de `layered_streaming_ratio` =
**4,0x** (revisado, medido com clang) -- todos medidos em **2026-07-23**, num container de dev
sandboxed Fedora 44 (Intel Core i5-12500H 12ª geração, 16 núcleos lógicos, 31 GiB RAM), **NÃO** o
próprio runner Forgejo self-hosted `claudio` nomeado pelo plano desta onda (uma máquina física
diferente). Esta é uma medição real, nunca um chute, mas uma baseline PROVISÓRIA: a 1ª execução
de CI no `claudio` deste teste imprime as próprias linhas `GLINTFX_PERF` no log do job, e
conforme o D30 essas três constantes devem ser trocadas pelos números daquela execução assim que
disponíveis (um diff de uma linha cada, a data/máquina deste comentário atualizada em
`draw2d_perf_budget.cpp`) -- flagado aqui explicitamente, não deixado em silêncio como se fosse
o número final.

### Limites declarados

Declarados em vez de fingidos, conforme a própria disciplina de downgrade-de-teste do plano:

- **Sem `draw_text`** -- `D2D-TEXT`, onda própria, exige o próprio ADR primeiro (tensão de
  motor-de-fonte: o flip FreeType-vs-motor-próprio do `App`, `docs/adr/0011-soft-font-flip.md`).
- **Sem primitivas de círculo/elipse/polígono-regular** -- semente `SEED-D2D-SHAPES` (o
  decorator RCSS `polygon()` do átomo `ui` já cobre o lado declarativo); gatilho: um consumidor
  pedir.
- **Sem tampas/junções de linha (round/miter), sem polylines** -- `draw_line` é só tampa butt;
  semente `SEED-D2D-LINECAPS`, gatilho: um consumidor pedir.
- **Sem sort secundário por textura dentro de uma camada** -- menos flushes em modo layered é a
  semente `SEED-D2D-LAYER-TEXSORT`, gatilho: o perf budget mostrar um consumidor limitado por
  flush.
- **Sem clipping em espaço de mundo** (uma região de clip rotacionada sob câmera) -- scissor
  aqui é só espaço de tela (D28, A3); um clip de retângulo de mundo é uma feature classe
  máscara/stencil, semente `SEED-D2D-WORLDCLIP`, gatilho: um consumidor pedir.
- **Sem parâmetro de LIMIAR de alpha pro `texture_content_bbox`** -- a definição desta onda é
  fixa em `alpha > 0`, semente `SEED-D2D-BBOX-THRESH`, gatilho: um consumidor pedir.
- **A capacidade de 4096 quads do batcher e o teto de 262144 comandos de camada seguem limites
  de MEMÓRIA, não claims de throughput** -- ver "Performance" acima pros números medidos e
  declarados.
- **Shake, suavização e limites/clamp de câmera são trabalho do game loop do consumidor, não
  deste módulo.** O Draw2D entrega a projeção (`Camera2d`, `set_camera`, os helpers puros); não
  entrega a cinemática por cima dela (lerp-follow, deadzone, screen shake). Desdobramento
  semeado: `SEED-D2D-CAMANIM` (INBOX do `TODO.md`), gatilho: um consumidor pedir.
- **Sem getters `camera()`/`has_camera()` no `Draw2d`.** O consumidor já é dono do valor
  `Camera2d` que passou pro `set_camera` -- um getter tem demanda zero hoje. Desdobramento
  semeado: `SEED-D2D-CAMGETTER`, gatilho: um consumidor pedir.
- **`camera_from_world_rect` ajusta só por LARGURA** -- um fit letterbox-exato compõe isto com
  `UiLayer::set_viewport` hoje (ver "Câmera" acima). Desdobramento semeado:
  `SEED-D2D-LETTERBOX`, gatilho: um consumidor pedir.
- **MSVC é só um gate de compilação+link.** O job embed (`GLFW=OFF`) compila este módulo
  (inclusive a superfície de câmera/transform do D2D-2, só `<cmath>`, sem header Unix-only) no
  Windows; não há execução RUNTIME Windows no CI.
- **Testes de pixel-readback sob Xvfb/llvmpipe são estatísticos, nunca pixel-exatos** --
  tile-threading no llvmpipe é não-determinístico (o mesmo motivo do `golden_test` seguir
  opt-in pra GPU real). Claims de exatidão (a fixação de equivalência do D19, as fórmulas de
  valor computado do D12) vivem SÓ nos testes unitários de CPU, onde são legítimos.
- **`load_texture()` NÃO honra `set_asset_base_url`.** O Draw2D lê o caminho recebido como está
  (`glintfx/src/draw2d.cpp:480`) -- essa facilidade é do lado `ui` hoje. Desdobramento semeado:
  `SEED-D2D-BASEURL` (INBOX do `TODO.md`), gatilho: um consumidor pedir.
- **Sem hook de frame pós-UI no `App`** -- sprites não podem ser desenhados SOBRE a UI no
  standalone até essa semente pousar (`SEED-D2D-POSTUI`, gatilho: um consumidor real pedir). No
  embed, o host já controla a ordem livremente (ver "Receitas de composição" acima).
- **Filtro de textura fixo em `GL_LINEAR`** -- sem opção nearest-neighbour por textura nesta
  fatia. Desdobramento semeado: `SEED-D2D-TEXFILTER`, gatilho: um consumidor pedir (ex.: pixel
  art).

### Relacionados

- `glintfx/include/glintfx/draw2d.hpp` -- contrato de API completo com doc-comments (fonte da
  verdade).
- `glintfx/src/transform2d.hpp` -- a matemática pura de fonte única de projeção/canto (D12/D18)
  por trás tanto do caminho de desenho com câmera quanto dos helpers públicos
  `world_to_screen`/`screen_to_world`/`camera_from_world_rect`.
- `glintfx/src/primitives2d.hpp` -- a geometria pura de fonte única (linha D25, contorno D26,
  mapeamento scissor-pra-GL D28) por trás das primitivas, testável headless
  (`tests/primitives2d_sanity.cpp`).
- `glintfx/src/layer_queue.hpp` -- a fila de comandos bufferizada pura de fonte única (D27) por
  trás do `set_layer`, testável headless (`tests/layer_queue_sanity.cpp`).
- [Plano da Onda 5](superpowers/plans/2026-07-23-onda5-draw2d-primitives.md) -- as decisões de
  desenho D23-D32 do D2D-3 por trás de primitivas/layers/scissor/bbox/perf (as próprias seções
  deste doc acima).
- [ADR-0017](adr/0017-draw2d-module.md) -- a decisão arquitetural que este módulo implementa
  (posição do módulo, pipeline GL próprio, composição por ordem-de-chamada); o
  [Adendo (2026-07-23)](adr/0017-draw2d-module.md#addendum-2026-07-23-d2d-2-coordinate-model)
  grava a decisão Q1/Q2 do líder por trás do modelo de câmera deste doc.
- [ADR-0015](adr/0015-framework-2d-atomized-architecture.md) -- a arquitetura de módulos
  atomizados da qual este módulo é uma fatia.
- [ADR-0013](adr/0013-gl-symbol-boundary.md) -- o loader GL interno prefixado `glx_` que o
  Draw2D reusa.
- `docs/embed-integration.md` seção 10 -- o contrato de coordenadas espaço-janela (coordenadas
  de mouse do `UiEvent`, `get_element_box`) que o próprio contrato de coordenadas deste módulo
  (D5) estende.
- `docs/embed-integration.md` seção 21 -- o contrato do hook de frame do `App` sobre o qual a
  própria receita App do Draw2D (acima) se apoia.
- `glintfx/src/gl_state.hpp` -- `GlStateGuard`, a classe de bug que o D9 existe pra guardar
  contra.
