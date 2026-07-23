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

- `Draw2d()` (`glintfx/include/glintfx/draw2d.hpp:331`) does not touch GL -- construction is
  always cheap and side-effect-free, same discipline as `Audio()`/`App()`/`UiLayer()`.
- `init()` (`glintfx/include/glintfx/draw2d.hpp:353`) requires a CURRENT GL 3.3 core context.
  It calls `glx_gl_load()` itself -- see "GL loader idempotency" below for why that is safe even
  when a cohabiting `App`/`UiLayer` already loaded the same table. Returns `false` on any
  GL/shader failure, or when the instance is already initialized (call `shutdown()` first to
  re-init).
- `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:365`) is idempotent -- releases every live
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

### Coordinate contract (D5)

Screen-space pixels, **top-left origin, y-down** -- the SAME space as `UiEvent` mouse
coordinates and `UiLayer::get_element_box` (see `docs/embed-integration.md` section 10): one
coordinate story for the whole library. `src_px` is a sub-rectangle of the texture in **texel**
coordinates (top-left origin, y-down, matching image memory row order); `RectF{}` (all-zero) is
the documented sentinel for "whole texture". Camera/world-space transforms are explicitly OUT of
this slice (`D2D-2`, next wave) -- see "Limits declared" below.

### API reference

| Method | Returns | Notes |
| :--- | :--- | :--- |
| `init()` (`glintfx/include/glintfx/draw2d.hpp:353`) | `bool` | Requires a current GL 3.3 core context. `false` on GL/shader failure or if already initialized. |
| `ok()` (`glintfx/include/glintfx/draw2d.hpp:356`) | `bool` | `true` between a successful `init()` and the next `shutdown()`/destruction. |
| `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:365`) | `void` | Idempotent GL teardown. Safe on a never-initialized instance. |
| `load_texture(const char* path)` (`glintfx/include/glintfx/draw2d.hpp:395`) | `Texture2d` | PNG/JPG/TGA/BMP (whatever `image_decode.hpp`'s stb_image-backed decode recognises), premultiplied on upload. `path` read via a plain `const char*` ifstream overload (D7, MSVC-safe), 256 MiB cap. `ok() == false` on `nullptr`, open failure, the cap, or a decode failure -- never a crash. |
| `destroy_texture(Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:412`) | `void` | Releases the GL texture if any, ALWAYS zeroes `tex` on return. Fail-high (never UB) on already-destroyed, tampered, or foreign handles. |
| `begin(int target_width, int target_height)` (`glintfx/include/glintfx/draw2d.hpp:431`) | `void` | Opens a batching bracket (D4). `target_width`/`target_height` are the viewport size in screen px this bracket's sprites project against -- the SAME size the host's own GL viewport is set to for this pass. Nested `begin()` implicitly ends the previous bracket first (flushing it, logged once). |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px = RectF{}, const ColorF& tint = ColorF{})` (`glintfx/include/glintfx/draw2d.hpp:465-466`) | `void` | Queues one textured quad, internally batched by texture. See "Batching" and "Fail-high input surface" below for the full guard list. |
| `end()` (`glintfx/include/glintfx/draw2d.hpp:533`) | `void` | Closes the bracket opened by `begin()`, flushing whatever is still pending -- unconditionally, even an empty bracket. Safe without a matching `begin()`. |
| `Texture2d::ok()` (`glintfx/include/glintfx/draw2d.hpp:273`) | `bool` | `false` when never loaded, when `load_texture()` failed, or after `destroy_texture()` zeroed this handle. |
| `Texture2d::width()`/`height()` (`glintfx/include/glintfx/draw2d.hpp:276-277`) | `int` | Pixel dimensions of the decoded image. `0` when `!ok()`. |

### Premultiply and the tint formula (D8)

`load_texture()` decodes and **alpha-premultiplies** the image on upload
(`glintfx/src/image_decode.hpp:150`, `R = R*A/255`, `G = G*A/255`, `B = B*A/255`, `A` unchanged)
to match this module's blend, `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`
(`glintfx/src/draw2d.cpp:269`). `ColorF` (`glintfx/include/glintfx/draw2d.hpp:121`) is
**straight** (non-premultiplied) RGBA in `[0, 1]`, each channel clamped on intake -- default is
opaque white (identity: an unmodified sample). The fragment shader
(`glintfx/src/draw2d.cpp:90-99`) applies the exact modulation:

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
(`set_gl_state_for_draw()`, `glintfx/src/draw2d.cpp:262-276`) and **assumes nothing** left behind
by a cohabiting renderer -- the RmlUi GL3 backend shares this same GL context. Enumerated list
of touched state (same style as `gl_state.hpp`'s captured list, `glintfx/src/gl_state.hpp:2-6`):

- Shader program (`glUseProgram`)
- Vertex array object (`glBindVertexArray`)
- Depth test (explicitly `glDisable(GL_DEPTH_TEST)`)
- Face culling (explicitly `glDisable(GL_CULL_FACE)`)
- Scissor test (explicitly `glDisable(GL_SCISSOR_TEST)`)
- Blend enable (`glEnable(GL_BLEND)`)
- Blend func, both RGB and alpha (`glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`)
- Blend equation (`glBlendEquation(GL_FUNC_ADD)`)
- Active texture unit (`glActiveTexture(GL_TEXTURE0)`)
- 2D texture binding on that unit (`glBindTexture(GL_TEXTURE_2D, ...)`)
- The `uTex`/`uTargetSize` shader uniforms

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

`Texture2d` (`glintfx/include/glintfx/draw2d.hpp:266`) is a plain, copyable value handle -- NOT
an RAII owner (`destroy_texture()` is the explicit release, same idiom as `Audio::SoundId`).
Internally it carries an opaque id, a generation counter, and an **owner tag** (the issuing
`Draw2d`'s own identity): a `Draw2d` instance validates every handle against its own internal
registry before use.

**A handle is NOT interchangeable between `Draw2d` instances.** `load_texture()` stamps the
returned handle with the emitting instance's identity (`glintfx/src/draw2d.cpp:476`); both
`draw_sprite()` / `destroy_texture()` (`glintfx/src/draw2d.cpp:535`/`480`) reject a handle whose
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

### GL loader idempotency

`Draw2d::init()` calls `glx_gl_load()` itself
(`glintfx/include/glintfx/draw2d.hpp:213-215`) -- necessary because with `ui`/`window` OFF nobody
else would load the internal GL function-pointer table. This is safe even when a cohabiting
`App`/`UiLayer` already loaded the same table: `glx_gl_load()` unconditionally resolves and
overwrites every symbol slot on each call, on both the Linux (`glintfx/src/gl_loader.c:897`) and
Windows (`glintfx/src/gl_loader.c:830`) code paths -- there is no internal "already loaded" guard
to interact with, so calling it twice is a redundant, harmless re-resolve, not a state hazard. A
host does not need to order Draw2D's `init()` relative to `App`'s/`UiLayer`'s own initialization.

### Limits declared

Stated instead of faked, per the plan's own testing-downgrade discipline:

- **No camera, rotation, scale, flip, or `screen_to_world`** in this slice -- `D2D-2`, next wave
  (v0.21.0).
- **No layers, explicit draw order, untextured primitives, scissor, or measured performance
  budget** in this slice -- `D2D-3`, a later wave. The 4096-quad capacity flush is a memory
  bound, not a throughput claim.
- **MSVC is a compile+link gate only.** The embed (`GLFW=OFF`) job builds this module on
  Windows; there is no Windows RUNTIME execution in CI.
- **Pixel-readback tests under Xvfb/llvmpipe are statistical, never pixel-exact** -- tile
  threading on llvmpipe is non-deterministic (the same reason the `golden_test` remains opt-in
  for real GPUs).
- **`load_texture()` does NOT honour `set_asset_base_url`.** Draw2D reads the given path as-is
  (`glintfx/src/draw2d.cpp:380`) -- that facility is `ui`-side today. Seeded follow-up:
  `SEED-D2D-BASEURL` (`TODO.md` INBOX), trigger: a consumer ask.
- **No post-UI `App` frame hook** -- sprites cannot be drawn OVER the UI in standalone mode until
  that seed lands (`SEED-D2D-POSTUI`, trigger: a real consumer ask). In embed, the host already
  controls order freely (see "Composition recipes" above).
- **Texture filtering is fixed at `GL_LINEAR`** -- no nearest-neighbour option per texture in
  this slice. Seeded follow-up: `SEED-D2D-TEXFILTER`, trigger: a consumer ask (e.g. pixel art).

### Related

- `glintfx/include/glintfx/draw2d.hpp` -- full doc-commented API contract (source of truth).
- [ADR-0017](adr/0017-draw2d-module.md) -- the architectural decision this module implements
  (module placement, own GL pipeline, order-by-call-order composition).
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

- `Draw2d()` (`glintfx/include/glintfx/draw2d.hpp:331`) não toca em GL -- a construção é sempre
  barata e sem efeito colateral, mesma disciplina de `Audio()`/`App()`/`UiLayer()`.
- `init()` (`glintfx/include/glintfx/draw2d.hpp:353`) exige um contexto GL 3.3 core CORRENTE.
  Chama o próprio `glx_gl_load()` -- ver "Idempotência do loader GL" abaixo pro porquê disso ser
  seguro mesmo quando um `App`/`UiLayer` coabitante já carregou a mesma tabela. Retorna `false`
  em qualquer falha de GL/shader, ou quando a instância já está inicializada (chame `shutdown()`
  primeiro pra re-inicializar).
- `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:365`) é idempotente -- libera toda textura
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

### Contrato de coordenadas (D5)

Pixels em espaço de tela, **origem superior-esquerda, y pra baixo** -- o MESMO espaço das
coordenadas de mouse do `UiEvent` e de `UiLayer::get_element_box` (ver
`docs/embed-integration.md` seção 10): uma história de coordenadas só pra biblioteca inteira.
`src_px` é uma sub-retângulo da textura em coordenadas de **texel** (origem superior-esquerda, y
pra baixo, batendo com a ordem de linha da memória de imagem); `RectF{}` (tudo zero) é a
sentinela documentada de "textura inteira". Transformações de câmera/espaço-de-mundo estão
explicitamente FORA desta fatia (`D2D-2`, próxima onda) -- ver "Limites declarados" abaixo.

### Referência de API

| Método | Retorna | Notas |
| :--- | :--- | :--- |
| `init()` (`glintfx/include/glintfx/draw2d.hpp:353`) | `bool` | Exige um contexto GL 3.3 core corrente. `false` em falha de GL/shader ou se já inicializado. |
| `ok()` (`glintfx/include/glintfx/draw2d.hpp:356`) | `bool` | `true` entre um `init()` bem-sucedido e o próximo `shutdown()`/destruição. |
| `shutdown()` (`glintfx/include/glintfx/draw2d.hpp:365`) | `void` | Teardown GL idempotente. Seguro numa instância nunca inicializada. |
| `load_texture(const char* path)` (`glintfx/include/glintfx/draw2d.hpp:395`) | `Texture2d` | PNG/JPG/TGA/BMP (o que o decode apoiado em stb_image do `image_decode.hpp` reconhecer), premultiplicado no upload. `path` lido via overload de ifstream de `const char*` puro (D7, MSVC-safe), teto de 256 MiB. `ok() == false` em `nullptr`, falha ao abrir, o teto, ou falha de decode -- nunca um crash. |
| `destroy_texture(Texture2d& tex)` (`glintfx/include/glintfx/draw2d.hpp:412`) | `void` | Libera a textura GL se houver, SEMPRE zera `tex` ao retornar. Fail-high (nunca UB) em handle já-destruído, adulterado, ou estrangeiro. |
| `begin(int target_width, int target_height)` (`glintfx/include/glintfx/draw2d.hpp:431`) | `void` | Abre um bracket de batching (D4). `target_width`/`target_height` são o tamanho de viewport em px de tela contra o qual os sprites deste bracket projetam -- o MESMO tamanho pro qual o próprio viewport GL do host está definido neste passe. `begin()` aninhado encerra o bracket anterior implicitamente primeiro (fazendo flush dele, logado uma vez). |
| `draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px = RectF{}, const ColorF& tint = ColorF{})` (`glintfx/include/glintfx/draw2d.hpp:465-466`) | `void` | Enfileira um quad texturizado, batchado internamente por textura. Ver "Batching" e "Superfície de entrada fail-high" abaixo pra lista completa de guardas. |
| `end()` (`glintfx/include/glintfx/draw2d.hpp:533`) | `void` | Fecha o bracket aberto por `begin()`, fazendo flush do que ainda estiver pendente -- incondicionalmente, mesmo um bracket vazio. Seguro sem um `begin()` correspondente. |
| `Texture2d::ok()` (`glintfx/include/glintfx/draw2d.hpp:273`) | `bool` | `false` quando nunca carregado, quando `load_texture()` falhou, ou depois de `destroy_texture()` zerar este handle. |
| `Texture2d::width()`/`height()` (`glintfx/include/glintfx/draw2d.hpp:276-277`) | `int` | Dimensões em pixel da imagem decodificada. `0` quando `!ok()`. |

### Premultiply e a fórmula do tint (D8)

`load_texture()` decodifica e **premultiplica o alpha** da imagem no upload
(`glintfx/src/image_decode.hpp:150`, `R = R*A/255`, `G = G*A/255`, `B = B*A/255`, `A` inalterado)
pra bater com o blend deste módulo, `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`
(`glintfx/src/draw2d.cpp:269`). `ColorF` (`glintfx/include/glintfx/draw2d.hpp:121`) é RGBA
**straight** (não-premultiplicado) em `[0, 1]`, cada canal clampado na entrada -- o padrão é
branco opaco (identidade: uma amostra sem modificação). O estágio de fragmento
(`glintfx/src/draw2d.cpp:90-99`) aplica a modulação exata:

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
(`set_gl_state_for_draw()`, `glintfx/src/draw2d.cpp:262-276`) e **não assume nada** deixado por
um renderer coabitante -- o backend GL3 do RmlUi compartilha este mesmo contexto GL. Lista
enumerada do estado tocado (mesmo estilo da lista capturada em `gl_state.hpp`,
`glintfx/src/gl_state.hpp:2-6`):

- Programa de shader (`glUseProgram`)
- Vertex array object (`glBindVertexArray`)
- Depth test (explicitamente `glDisable(GL_DEPTH_TEST)`)
- Face culling (explicitamente `glDisable(GL_CULL_FACE)`)
- Scissor test (explicitamente `glDisable(GL_SCISSOR_TEST)`)
- Blend enable (`glEnable(GL_BLEND)`)
- Blend func, RGB e alpha (`glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA)`)
- Blend equation (`glBlendEquation(GL_FUNC_ADD)`)
- Unidade de textura ativa (`glActiveTexture(GL_TEXTURE0)`)
- Binding de textura 2D naquela unidade (`glBindTexture(GL_TEXTURE_2D, ...)`)
- Os uniforms de shader `uTex`/`uTargetSize`

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

`Texture2d` (`glintfx/include/glintfx/draw2d.hpp:266`) é um handle-valor simples, copiável --
NÃO é dono RAII (`destroy_texture()` é a liberação explícita, mesmo idioma do `Audio::SoundId`).
Internamente carrega um id opaco, um contador de geração, e uma **tag de dono** (a identidade
própria do `Draw2d` emissor): uma instância `Draw2d` valida todo handle contra o próprio registry
interno antes de usar.

**Um handle NÃO é intercambiável entre instâncias `Draw2d`.** `load_texture()` carimba o handle
retornado com a identidade da instância emissora (`glintfx/src/draw2d.cpp:476`); tanto
`draw_sprite()` / `destroy_texture()` (`glintfx/src/draw2d.cpp:535`/`480`) rejeitam um handle cuja
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

### Limites declarados

Declarados em vez de fingidos, conforme a própria disciplina de downgrade-de-teste do plano:

- **Sem câmera, rotação, escala, flip, ou `screen_to_world`** nesta fatia -- `D2D-2`, próxima
  onda (v0.21.0).
- **Sem layers, ordem explícita de desenho, primitivas não-texturizadas, scissor, ou perf budget
  medido** nesta fatia -- `D2D-3`, onda futura. O flush em 4096 quads é um limite de memória, não
  um claim de throughput.
- **MSVC é só um gate de compilação+link.** O job embed (`GLFW=OFF`) compila este módulo no
  Windows; não há execução RUNTIME Windows no CI.
- **Testes de pixel-readback sob Xvfb/llvmpipe são estatísticos, nunca pixel-exatos** --
  tile-threading no llvmpipe é não-determinístico (o mesmo motivo do `golden_test` seguir
  opt-in pra GPU real).
- **`load_texture()` NÃO honra `set_asset_base_url`.** O Draw2D lê o caminho recebido como está
  (`glintfx/src/draw2d.cpp:380`) -- essa facilidade é do lado `ui` hoje. Desdobramento semeado:
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
- [ADR-0017](adr/0017-draw2d-module.md) -- a decisão arquitetural que este módulo implementa
  (posição do módulo, pipeline GL próprio, composição por ordem-de-chamada).
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
