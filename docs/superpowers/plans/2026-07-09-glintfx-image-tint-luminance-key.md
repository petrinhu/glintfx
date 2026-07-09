# glintfx `image-tint()` — luminance-key decorator — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. This plan implements [ADR-0010](../../adr/0010-image-tint-luminance-key.md) — read it first, especially the "Investigation summary" and Decision (a)-(e); this plan does not re-derive the *why*, only the *how*.

> **Status of the design:** ADR-0010 is `Proposed`, not `Accepted` — the decorator being named `image-tint()` instead of `image()` (ADR §Decision b) is a deviation from the líder's original RCSS sketch and needs explicit confirmation **before** Task 1 starts. Do not begin implementation until the líder has signed off on the ADR (or given a different naming instruction).

**Goal:** Add a new glintfx decorator, `decorator: image-tint(<url>);`, plus three standalone RCSS properties (`image-tint-color`, `image-tint-mode`, `image-tint-threshold`), that recolour only the light/neutral texels of a decorator image while preserving already-saturated and dark texels — a *luminance-key* tint, as opposed to the native `image-color`'s uniform multiply. Consumer-driven by GusWorld (one neutral base texture → N domain colours).

**Non-goals (out of scope for this plan):** gamma/sRGB correction (ADR §d flags it as an open question — implement WITHOUT it, confirm visually against the real GusWorld asset before considering adding it); tinting decorators other than a plain image quad (no `tiled-*`/ninepatch support); a `screen` mode formula beyond the standard `1-(1-a)(1-b)` (implement it, but it has not been consumer-validated — lower test priority than `luminance-multiply`).

---

## Architecture recap (see ADR-0010 for full derivation)

- **New decorator class** `glintfx::ImageTintDecorator` / `ImageTintDecoratorInstancer`, new files `glintfx/src/decorator_image_tint.{hpp,cpp}`, modelled directly on `decorator_polygon.{hpp,cpp}` (same Initialise/GenerateElementData/ReleaseElementData/RenderElement shape, same instancer-registers-properties-in-ctor idiom, same fail-high hardening philosophy).
- **Global properties** `image-tint-color` (colour, default `white`), `image-tint-mode` (keyword `none|multiply|luminance-multiply|screen`, default `none`), `image-tint-threshold` (number, default `0.55`) registered via `Rml::StyleSheetSpecification::RegisterProperty` (NOT decorator-shorthand args) — read fresh, per element, inside `RenderElement` (not cached in `GenerateElementData`, see ADR §c).
- **`Gl3RenderInterface`** (`glintfx/src/render_gl3.cpp`) gains overrides of `CompileShader`/`RenderShader`/`ReleaseShader` that recognise `name == "glintfx-tint"`, wrapping every handle (both tint and delegated-to-base) in a small tagged struct so the three methods can tell the two cases apart. The actual custom draw (own GLSL program, own VAO, own uniforms) happens **inside** `RenderShader`, where `this` is the concrete backend (`this->GetTransform()`/`this->ResetProgram()` reachable for free — see ADR §a/§1-3).
- **Texture** resolved once per decorator-instance via `DecoratorInstancerInterface::GetTexture(src)` at `InstanceDecorator` time (shared/cached by RmlUi across every element using the same rule, like the native `image` decorator).
- **`image-tint-mode: none`** (default) never touches any of the above — `RenderElement` calls plain `Geometry::Render(translation, texture)`, identical cost/behaviour to `image()`.

---

## Task 1 — Property registration + decorator skeleton (no shader yet)

**Goal of this task:** `decorator: image-tint(base.png);` parses, renders the image **unmodified** (mode defaults to `none`), and the three new properties are registered, hardened, and readable — with **zero** new GL code yet. This isolates decorator/property-registration risk from shader/VAO risk.

- [ ] **1.1 — New files `glintfx/src/decorator_image_tint.hpp` / `.cpp`.**
  - `ImageTintDecorator : public Rml::Decorator` — members: `Rml::Texture texture_;` (baked at `Initialise` time, see 1.3), per-element data struct `ImageTintElementData { Rml::Geometry geometry; }` (the *vanilla*-path geometry, built via `render_manager->MakeGeometry(quad_mesh)` sized to the element's fill box — same box-geometry convention `decorator_polygon.cpp`'s `GenerateElementData` already uses: `element->GetRenderBox(paint_area)`, `GetFillOffset()`/`GetFillSize()`). Reuse this same mesh/geometry in Task 2 for the tinted path too (do not duplicate).
  - `ImageTintDecoratorInstancer : public Rml::DecoratorInstancer` — registers ONE positional argument, `"src"`, parser `"string"` (passthrough, same pattern `decorator_polygon.cpp` uses for `fill` — see that file's "FILL PARSING" doc-comment block for why passthrough-then-manually-validate is the house style here), shorthand `"decorator": "src"` (single positional item — `Rml::ShorthandType::FallThrough` is sufficient for one argument, no need for `RecursiveCommaSeparated`'s comma-splitting since there's nothing to split).
  - `InstanceDecorator`: `Rml::Texture texture = instancer_interface.GetTexture(src_raw);` — hardening: `if (!texture) { Rml::Log::Message(LT_WARNING, "decorator: image-tint() could not load texture '%s' -- decorator ignored.", src_raw.c_str()); return nullptr; }` (fail-high, mirrors `polygon()`'s `sides<3`/invalid-fill rejection and the `load(nullptr)` precedent). On success: `auto decorator = Rml::MakeShared<ImageTintDecorator>(); decorator->Initialise(std::move(texture)); return decorator;`.
  - `GenerateElementData`: same box-size/offset computation as `decorator_polygon.cpp` (`element->GetRenderManager()`, `element->GetRenderBox(paint_area)`, reject `size.x<=0.f || size.y<=0.f` → `INVALID_DECORATORDATAHANDLE`, matching precedent), build a 4-vertex quad `Rml::Mesh` (`position` = box corners in border-box-relative space per `decorator_polygon.cpp`'s "translation happens in `RenderElement`" convention — i.e. author the mesh at `offset..offset+size`, translated later by `element->GetAbsoluteOffset(Rml::BoxArea::Border)` exactly like `PolygonDecorator::RenderElement` does; `tex_coord` = `(0,0)..(1,1)`), `vertex color` = opaque premultiplied white (texture-only quad, no vertex tint baked in — tint is applied in the shader in Task 2, not via vertex colour, since it must be re-evaluatable every frame). `render_manager->MakeGeometry(std::move(mesh))` → store in `ImageTintElementData`.
  - `RenderElement` (Task 1 version — no tint yet): `data->geometry.Render(element->GetAbsoluteOffset(Rml::BoxArea::Border), texture_);` — plain textured quad, no shader argument (falls into `Geometry::Render`'s default-`shader` branch, `RenderGeometry`/`ProgramId::Texture`, exactly the native `image()` decorator's own rendering cost).
  - `ReleaseElementData`: `delete reinterpret_cast<ImageTintElementData*>(element_data);` (RAII via `Rml::Geometry`'s own `UniqueRenderResource`, same as `PolygonElementData`).

- [ ] **1.2 — Register the three global properties.** In `Rml::StyleSheetSpecification::RegisterProperty` calls (same registry-timing constraint as `PolygonDecoratorInstancer` — see `bootstrap.cpp`'s `Impl::polygon_instancer` doc-comment: must happen **after** `Rml::Initialise()` returns, before `Factory::Initialise()` runs; do this registration from inside `ImageTintDecoratorInstancer`'s own constructor, exactly mirroring how `PolygonDecoratorInstancer`'s ctor registers `sides`/`fill`/`rotation` — same lazy-allocation-in-`Bootstrap::init()` lifetime, see Task 3):
  - `image-tint-color`: `RegisterProperty("image-tint-color", "white", /*inherited=*/false).AddParser("color");`
  - `image-tint-mode`: `RegisterProperty("image-tint-mode", "none", /*inherited=*/false).AddParser("keyword", "none, multiply, luminance-multiply, screen");` (confirm the exact 2-arg `AddParser(parser_name, parser_parameters)` signature against `Include/RmlUi/Core/PropertyDefinition.h:20` before writing this — verified present during investigation, but double-check the keyword-list string format against an existing RmlUi-native keyword property, e.g. how `StyleSheetSpecification.cpp` registers `display` or `position`, for exact comma/space convention).
  - `image-tint-threshold`: `RegisterProperty("image-tint-threshold", "0.55", /*inherited=*/false).AddParser("number");`
  - These are property IDs global to the whole application (not scoped to `ImageTintDecoratorInstancer`'s own `PropertyIds`), read later via `element->GetProperty("image-tint-color")` etc — confirm `Rml::Element::GetProperty(const String&)` (`Include/RmlUi/Core/Element.h:193`) is the right call (returns `const Property*`, `nullptr` if never set/no default — should never be `nullptr` once registered with a default value, but guard anyway).

- [ ] **1.3 — Read + harden the three properties (still Task 1 — hardening logic first, wired to the vanilla-only render path).** Add a small free function (or static method) `ResolveTintState(Rml::Element*, TintState& out)` in `decorator_image_tint.cpp` that:
  - Reads `image-tint-mode` (`Rml::Property::Get<Rml::String>()` or however the keyword parser stores it — confirm: keyword properties typically resolve to an `int`/`Unit::KEYWORD` matching the ordinal position in the `AddParser` list, NOT a string — verify against an existing RmlUi keyword-property read site, e.g. `computed.display()` in `ComputedValues.h`, before assuming `Get<Rml::String>()` works). Maps to an `enum class TintMode { None, Multiply, LuminanceMultiply, Screen };`. **Defense-in-depth default case** (per ADR §"hardening specifics"): any value not matching one of the 4 known ordinals → `TintMode::None` (belt-and-suspenders; the `keyword` parser should already gate this at RCSS-parse time, but mirrors `decorator_polygon.cpp`'s "RCSS parser is a transport, not a gate" philosophy).
  - Reads `image-tint-color` → `Rml::Colourb`, no extra validation needed (any resolved colour is valid, per ADR).
  - Reads `image-tint-threshold` → `float`; hardening **exactly** mirrors `decorator_polygon.cpp`'s `sides` float-cast-overflow guard: `if (!std::isfinite(t)) t = 0.55f;` (NaN/Inf → default) THEN `t = std::clamp(t, 0.0f, 1.0f);` (out-of-range but finite → clamp, per the líder's explicit ask — note this is a *different* hardening shape than `sides`, which rejects the whole decorator on out-of-range; `threshold` clamps instead of rejecting, since an out-of-range threshold is not destructive/DoS-shaped the way a billion-sided polygon is — document this distinction inline, don't silently copy `sides`' reject-the-whole-thing behaviour without justifying why threshold differs).
  - Task 1 wiring: `RenderElement` calls `ResolveTintState` but only uses it to decide **whether to log a one-time "not yet implemented" warning** if mode != None (or simply ignore it — Task 2 makes it functional). Do not half-wire the shader path in Task 1.

- [ ] **1.4 — Tests (Task 1 scope): decorator parses, renders unmodified, properties round-trip.**
  - New test file `glintfx/tests/image_tint_sanity.cpp` (mirror `polygon_sanity.cpp`'s structure/harness — GLFW=ON test, offscreen render + pixel readback via whatever helper `render_sanity`/`polygon_sanity` already use).
  - Test: `decorator: image-tint(<fixture>.png);` with no `image-tint-*` set renders **pixel-identical** (or within the existing statistical tolerance `render_sanity` uses) to `decorator: image(<fixture>.png);` on the same element — proves the vanilla/no-op path in 1.1 is truly equivalent to the native decorator.
  - Test: missing/garbage `src` (`image-tint(does-not-exist.png)`) → decorator ignored, no crash, warning logged (mirror `polygon_sanity`'s invalid-input test style).
  - Test: `image-tint-threshold: 5.0;` (out of range) does not crash / does not assert; `image-tint-threshold: nan;` (if RCSS can even produce this — check) does not corrupt state.
  - Test: `image-tint-mode: bogus-value;` — confirm RmlUi's own `keyword` parser rejects it at property-set time (falls back to `none`) — if it does NOT reject cleanly (i.e. if `GetProperty` somehow returns something unexpected), the defense-in-depth `default:` case in 1.3 must catch it; write the test to exercise BOTH layers if feasible (a raw `Rml::Property` with an out-of-range int, constructed directly bypassing the parser, fed straight to the hardening function) — this is exactly the kind of "auditoria dominó" check the project's `feedback_auditoria_domino` memory calls for (an input-hardening gap found once, checked for elsewhere).
  - Register the new test in `glintfx/tests/CMakeLists.txt` (mirror the existing `polygon_sanity` entry).

- [ ] **1.5 — Commit Task 1.** Conventional commit, cite the ID `GLINTFX-TINT-1` (see Task 5 for `TODO.md` wiring — this ID should be added to `TODO.md`'s INBOX-seed line as an active item before or alongside this commit, so `todo_sync.py` has something to match against). **Local commit only, no push** (repo-wide standing rule).

---

## Task 2 — Shader injection: `Gl3RenderInterface` overrides + GLSL

**Goal of this task:** `image-tint-mode: multiply | luminance-multiply | screen` actually recolour the texture, per ADR-0010 §a/§c/§d.

- [ ] **2.1 — GLSL source (own vertex + fragment shader, own uniform names — do NOT reuse RmlUi's `shader_vert_main`/`_translate`/`_transform` uniform NAMES verbatim in a way that could collide if ever linked into the same program object; separate program object, names can safely match RmlUi's convention for consistency but must live in glintfx's own compiled `GLuint` program, never touching RmlUi's `program_data`).**

  Vertex shader (2 attributes: position, texcoord — no vertex colour needed, unlike RmlUi's `inColor0`, since tint/opacity are uniforms here, re-read fresh every frame per ADR §c):
  ```glsl
  #version 330
  uniform vec2 _translate;
  uniform mat4 _transform;   // pre-combined projection * element-transform, from this->GetTransform()
  in vec2 inPosition;
  in vec2 inTexCoord0;
  out vec2 fragTexCoord;
  void main() {
    fragTexCoord = inTexCoord0;
    vec2 translatedPos = inPosition + _translate;
    gl_Position = _transform * vec4(translatedPos, 0.0, 1.0);
  }
  ```

  Fragment shader — **note the un-premultiply/re-premultiply required by ADR §d** (the consumer's original GLSL sketch operates directly on `texture(uTex,uv).rgb`, which is WRONG here because glintfx's textures are pre-multiplied on load — this corrected version fixes that):
  ```glsl
  #version 330
  #define MODE_NONE 0
  #define MODE_MULTIPLY 1
  #define MODE_LUMINANCE_MULTIPLY 2
  #define MODE_SCREEN 3

  uniform sampler2D _tex;
  uniform vec4 _tint_color;      // straight (non-premultiplied) RGBA, from image-tint-color
  uniform int  _mode;
  uniform float _threshold;
  uniform float _opacity;        // element's CSS opacity, read fresh (see ADR "opacity staleness" risk)

  in vec2 fragTexCoord;
  out vec4 finalColor;

  void main() {
    vec4 texel = texture(_tex, fragTexCoord);          // PREMULTIPLIED (rgb already *= a) -- see Gl3RenderInterface::LoadTexture
    vec3 straight = texel.rgb / max(texel.a, 0.0001);   // un-premultiply, div-by-zero guarded

    vec3 tinted = straight;
    if (_mode == MODE_MULTIPLY) {
      tinted = straight * _tint_color.rgb;
    } else if (_mode == MODE_LUMINANCE_MULTIPLY) {
      float L   = dot(straight, vec3(0.299, 0.587, 0.114));
      float mx  = max(max(straight.r, straight.g), straight.b);
      float mn  = min(min(straight.r, straight.g), straight.b);
      float sat = mx - mn;
      float w   = smoothstep(_threshold, 1.0, L) * (1.0 - sat);
      tinted = mix(straight, straight * _tint_color.rgb, w);
    } else if (_mode == MODE_SCREEN) {
      tinted = vec3(1.0) - (vec3(1.0) - straight) * (vec3(1.0) - _tint_color.rgb);
    }
    // MODE_NONE: tinted == straight (identity) -- note this branch should be UNREACHABLE in
    // practice, since RenderElement takes the plain Geometry::Render(translation, texture) path
    // (no shader at all) when mode==None -- kept here only as a defensive fallback, never the
    // primary dispatch for "none".

    float outAlpha = texel.a * _tint_color.a * _opacity;
    finalColor = vec4(tinted * outAlpha, outAlpha);      // re-premultiply
  }
  ```
  **Implementer must validate:** `mix(straight, straight*_tint_color.rgb, w)` can push `tinted` values above 1.0 if `_tint_color.rgb` has channels > 1 (RCSS colours don't allow that, so should be moot) — no clamp needed under normal RCSS input, but note it. Also validate the `smoothstep(_threshold,1.0,L)` — if `_threshold` is ever `>= 1.0` (clamped max), `smoothstep(1,1,L)` is a documented edge case (GLSL spec: undefined if edge0>=edge1; here edge0==edge1==1.0, both same value is well-defined per spec — `0.0` for `L<1`, `1.0` at `L==1` — confirm this on the actual target GL driver (Mesa/llvmpipe) since `render_sanity` already runs under it).

- [ ] **2.2 — Program compile helper.** In `render_gl3.cpp` (or a small anonymous-namespace helper if the file grows unwieldy — implementer's call), a lazy-initialised static/member `GLuint tint_program_ = 0` on (or reachable from) `Gl3RenderInterface`, compiled on first use inside `CompileShader`'s `"glintfx-tint"` branch (glCreateShader/glShaderSource/glCompileShader with `glGetShaderiv(GL_COMPILE_STATUS)` + `glGetShaderInfoLog` error logging on failure — mirror whatever minimal GL-error-checking convention `Gfx::CheckGLError` in the vendored backend uses, for consistency, though that helper itself is not reachable/reusable from glintfx code — write a small equivalent). On compile failure: log + treat as if `CompileShader` returned an invalid handle (fail-high, no crash) — `RenderElement`/`Geometry::Render` must tolerate a falsy `Rml::CompiledShader` gracefully (confirm this is safe by reading `RenderManager::Render`'s dispatch — a `CompiledShader` that is falsy takes the `RenderGeometry` branch per the existing `PolygonDecorator` Solid-fill precedent, so a `CompileShader` failure should make `ImageTintDecorator::RenderElement` degrade to the vanilla textured-quad path rather than propagate an invalid shader forward — implement this fallback explicitly, don't assume it happens automatically, since here (unlike polygon) the DECORATOR itself decides whether to call `CompileShader` at all).

- [ ] **2.3 — `GlintfxShaderHandle` wrapper + `CompileShader`/`RenderShader`/`ReleaseShader` overrides.** In `Gl3RenderInterface` (`render_gl3.cpp`):
  ```cpp
  struct GlintfxTintShaderData {
    GLuint vao = 0;
    int    index_count = 0;
    Rml::Colourf tint_color{1,1,1,1};   // straight, from image-tint-color
    int    mode = 0;                    // TintMode ordinal
    float  threshold = 0.55f;
    float  opacity = 1.0f;
  };
  struct GlintfxShaderHandle {
    bool is_tint;
    Rml::CompiledShaderHandle base_handle;   // valid only if !is_tint
    GlintfxTintShaderData tint_data;         // valid only if is_tint
  };
  ```
  - `CompileShader(name, params) override`: `if (name == "glintfx-tint") { build GlintfxTintShaderData from params (vao/index_count/tint_color/mode/threshold/opacity, all via Rml::Get(params, key, default)); return reinterpret_cast<Rml::CompiledShaderHandle>(new GlintfxShaderHandle{true, {}, data}); } auto base = RenderInterface_GL3::CompileShader(name, params); if (!base) return {}; return reinterpret_cast<Rml::CompiledShaderHandle>(new GlintfxShaderHandle{false, base, {}});` — confirm `Rml::Get(const Dictionary&, key, default)` free function signature against `decorator_polygon.cpp`'s own imports / RmlUi's `Backends/RmlUi_Renderer_GL3.cpp` usage (`Rml::Get(parameters, "repeating", false)` already seen — same helper, public, `Include/RmlUi/Core/Dictionary.h` or similar, confirm exact header).
  - `RenderShader(shader_handle, geometry_handle, translation, texture) override`: unwrap `GlintfxShaderHandle`. If `!wrapper->is_tint`: `RenderInterface_GL3::RenderShader(wrapper->base_handle, geometry_handle, translation, texture); return;` (pure passthrough, geometry_handle untouched, exactly the pre-existing gradient/creation behaviour). If `is_tint`: **never touch `geometry_handle`** — `glUseProgram(tint_program_); glUniform2f(loc(_translate), translation.x, translation.y); glUniformMatrix4fv(loc(_transform), 1, GL_FALSE, this->GetTransform().data()); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, (GLuint)texture); glUniform1i(loc(_tex), 0); glUniform4f(loc(_tint_color), ...); glUniform1i(loc(_mode), ...); glUniform1f(loc(_threshold), ...); glUniform1f(loc(_opacity), ...); glBindVertexArray(wrapper->tint_data.vao); glDrawElements(GL_TRIANGLES, wrapper->tint_data.index_count, GL_UNSIGNED_INT, (const GLvoid*)0); glBindVertexArray(0); this->ResetProgram();` — confirm `Rml::Matrix4f` exposes a raw-float accessor (`.data()`/similar) for `glUniformMatrix4fv`, check `Include/RmlUi/Core/Matrix4.h`.
  - `ReleaseShader(shader_handle) override`: unwrap; if `!is_tint`, delegate `RenderInterface_GL3::ReleaseShader(wrapper->base_handle)`; either way `delete wrapper;` — **do not** `glDeleteVertexArrays` on `tint_data.vao` here (VAO lifetime belongs to `ImageTintElementData`/`ReleaseElementData`, not to the per-call `CompiledShader`, per ADR §c — get this ownership split right, it's the one place a double-free/use-after-free could sneak in).

- [ ] **2.4 — `ImageTintDecorator` changes to actually use the shader.**
  - `GenerateElementData` (extend Task 1's version): additionally allocate a raw `GLuint vao_, vbo_, ebo_` via gl3w calls (`glGenVertexArrays`/`glGenBuffers`, `glBufferData` with the SAME 4-vertex/6-index quad data already computed for the `Rml::Mesh` in Task 1 — factor a small shared helper that builds the vertex/index arrays once, feeds both the `Rml::Mesh` (vanilla path) and the raw VBO/EBO (tint path), rather than duplicating the box-corner math). Store `vao_`/`index_count=6` in `ImageTintElementData` alongside the existing `Rml::Geometry`. Release both in `ReleaseElementData` (`glDeleteVertexArrays(1,&vao_); glDeleteBuffers(...)` + the existing `Rml::Geometry` RAII).
  - `RenderElement` (final version): `TintState state; ResolveTintState(element, state); const float opacity = element->GetComputedValues().opacity();` (read FRESH every call — see ADR §c risk note on opacity staleness) `if (state.mode == TintMode::None) { data->geometry.Render(translation, texture_); return; } Rml::CompiledShader shader = render_manager->CompileShader("glintfx-tint", {{"vao", Rml::Variant((int)data->vao)}, {"index_count", Rml::Variant((int)data->index_count)}, {"tint_color", Rml::Variant(ToColourf(state.color))}, {"mode", Rml::Variant((int)state.mode)}, {"threshold", Rml::Variant(state.threshold)}, {"opacity", Rml::Variant(opacity)}}); if (!shader) { data->geometry.Render(translation, texture_); return; } data->geometry.Render(translation, texture_, shader);` — the fallback-to-vanilla-on-`!shader` path is the explicit degrade-gracefully behaviour flagged in 2.2.

- [ ] **2.5 — Tests (Task 2 scope): the actual tint math.**
  - **Synthetic 3-zone fixture texture** (implementer generates a small PNG, e.g. 3×1 or a 3-region grid — coordinate with `qa-engineer`/whoever owns test fixtures on exact format/tooling used elsewhere in `glintfx/tests/`): zone A = light+neutral (e.g. `#f0f0f0`, simulates a "rune"), zone B = light+saturated (e.g. `#f0c040`, simulates "gold trim"), zone C = dark (e.g. `#202020`, simulates "stone"). Render with `image-tint-mode: luminance-multiply; image-tint-color: #ff0000; image-tint-threshold: 0.5;` and assert (pixel readback, same technique as `render_sanity`/`polygon_sanity`): zone A shifts strongly toward red, zone B stays close to its original gold (low tint weight, saturated), zone C stays close to its original dark grey (low tint weight, luminance below threshold). This is the load-bearing test for the entire feature — it is the only test that actually proves the luminance-key selectivity works, not just "some shader ran."
  - Test `image-tint-mode: multiply;` on the same fixture: ALL three zones shift toward the tint colour (unlike luminance-multiply) — proves `multiply` still behaves like uniform tint (parity with `image-color`'s existing documented behaviour, different code path).
  - Test `image-tint-mode: none;` (or unset): pixel-identical to `image()` (already covered in Task 1's 1.4, re-assert here doesn't hurt as a regression guard once Task 2's code exists alongside it).
  - Test **interpolation**: `transition: image-tint-color 0.2s;` (or driving via `Context::Update` across simulated time steps, matching however other transition tests in this codebase are structured — search for an existing transition/animation test, e.g. anything exercising `image-color`'s own documented `transition` example in `docs/effects.md`, to mirror the harness) — assert an intermediate frame's pixel colour is between the start and end tint colours, not a hard cut — this is what proves the "per-frame fresh read, not baked at `GenerateElementData`" design in ADR §c actually avoids the staleness bug it was designed to avoid. **This test is the second load-bearing one** — if it fails (colour snaps instead of interpolating), the bug is almost certainly a `GenerateElementData`-time bake creeping back in.
  - Test **opacity interaction**: an ancestor/self `opacity: 0.5;` on a `luminance-multiply`-tinted element composites correctly (not fully opaque, not double-applied) — regression guard for the "opacity read fresh" requirement (ADR risk note).
  - Test **state-restore / no cross-contamination**: render, in the same frame, a `luminance-multiply`-tinted element followed by a `polygon()` gradient element (or plain text) on the same document — assert the polygon/text renders correctly (not corrupted/blank/wrong-shader) — this is the direct test for the `ResetProgram()` discipline risk flagged in ADR-0010.
  - Test **hardening**: `image-tint-threshold: 1.0;` (clamped max, exercises the `smoothstep(edge0==edge1)` GLSL edge case flagged in 2.1) does not crash/produce NaN pixels; a texture load failure inside `image-tint()` behaves like Task 1's 1.4 test (decorator ignored).
  - Register all new tests in `glintfx/tests/CMakeLists.txt`.

- [ ] **2.6 — Commit Task 2.** Conventional commit, cite `GLINTFX-TINT-2`. **Local commit only, no push.**

---

## Task 3 — Wiring into `bootstrap.cpp` (App + UiLayer parity)

- [ ] **3.1** Add `Rml::UniqueRenderResource... Rml::UniquePtr<glintfx::ImageTintDecoratorInstancer> tint_instancer;` to `Bootstrap::Impl`, mirroring `polygon_instancer`'s exact lifetime discipline (default-null, allocated lazily **after** `Rml::Initialise()` succeeds, `Rml::Factory::RegisterDecoratorInstancer("image-tint", impl_->tint_instancer.get())`, outlives `Rml::Shutdown()` — copy the doc-comment's reasoning verbatim/adapted, it applies identically here since the SAME `StyleSheetSpecification`/`Factory` initialisation-order constraint governs both).
- [ ] **3.2** Confirm both `App` and `UiLayer` get this decorator "for free" (both already route through the same `Bootstrap`/`Engine` — verify by reading how `polygon()` became available in both without any `App`/`UiLayer`-specific code, and confirm the same holds here; if it does not, that is a real finding to report back, not something to silently work around).
- [ ] **3.3** Update `docs/effects.md` (bilingual, EN then PT, matching the existing `image-color` how-to section's structure exactly) with a new "How-to: luminance-key tint (`image-tint`)" section — include the RCSS example from ADR-0010 §b, the 4 mode values, the threshold semantics, and an explicit cross-reference/contrast against the existing `image-color` section's "Honest limit" paragraph (which currently says luminance-key "is not supported natively... treat as roadmap candidate" — that sentence needs updating once this ships).
- [ ] **3.4** Commit, cite `GLINTFX-TINT-3`. **No push.**

---

## Task 4 — Review + release prep (not this agent's job to execute, but sequence it)

- [ ] **4.1** Whole-feature adversarial review by a DIFFERENT agent than the implementer (per the project's implementer≠reviewer rule) — review must EXECUTE the tests (mutation-testing spirit, "com dente"), not just read the diff, per the project's `feedback_auditoria_domino` memory. Specifically re-probe: (a) the `ResetProgram()` state-restore test from 2.5 under a MORE adversarial ordering (tint element, then MULTIPLE different native shader types in the same frame — gradient AND creation AND plain text); (b) whether `image-tint-mode` on an element with a `transform: rotate(...)` renders the tinted quad rotated correctly (this exercises `this->GetTransform()` — not explicitly listed as a Task 2 test above, add if missing); (c) whether a document with `set_viewport(x,y,w,h,target_h)` (embed-mode letterbox, ADR-0008 F3) renders a tinted element in the correct place (exercises the `translation`/`_transform` uniform math under a non-identity viewport offset).
- [ ] **4.2** `TODO.md` update: promote the INBOX seed line (currently reads "sem demanda confirmada") to an active item citing `GLINTFX-TINT-1/2/3`, status `🔍 Pendente verificação` until 4.1's review passes (never `✅` directly, per the project's tabela-pendencias-frescor rule).
- [ ] **4.3** Version bump + `CHANGELOG.md` entry (minor, drop-in — new decorator + new properties, no existing API changes) — release-prep agent's job, not this plan's.

---

## Open questions for the líder (do not resolve unilaterally)

1. **Decorator name `image-tint()` vs. the originally-sketched `image()` + separate props (ADR-0010 §b).** Confirm before Task 1.
2. **Gamma/sRGB correction (ADR-0010 §d).** Confirmed OFF for this plan; revisit only if the real `base.png` asset looks wrong once tested.
3. **`screen` mode's actual usefulness** — not consumer-validated; keep it (cheap to implement, symmetric with `multiply`) but do not over-invest test time relative to `luminance-multiply`.
