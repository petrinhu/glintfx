# Effects (RCSS reference and how-to)

> **EN:** How to apply each visual effect, plus a reference for the RmlUi 6.3 RCSS syntax glintfx uses. Diátaxis type: **how-to / reference**. Audience: a developer styling a glintfx UI.
> **PT:** Como aplicar cada efeito visual, mais uma referência da sintaxe RCSS do RmlUi 6.3 que o glintfx usa. Tipo Diátaxis: **how-to / reference**. Audiência: dev estilizando uma UI glintfx.

Effects in glintfx are **data-driven**: you declare them in `.rcss`, there is no imperative effect API in C++. The canonical, working example is [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss); this guide explains it.

---

## English

### RCSS is not exactly CSS

RmlUi 6.3 styling looks like CSS but differs in important ways. Keep these rules in mind or effects will silently not apply:

- **Color comes first** in `box-shadow` and `drop-shadow` (CSS puts it last).
- **Gradients use `decorator:`**, not `background:`.
- **Colors are 8-digit hex** `#rrggbbaa`; the last two digits are alpha. The 4-digit short form (e.g. `#000f`) works for the simple cases used in masks, but prefer 8-digit elsewhere.
- **Every element defaults to `display: inline`** (unlike HTML where `div` is block). Set `display: block;` so boxes stack and size as expected.

### Effect reference

| Effect | RCSS property | Argument order | Example |
| :--- | :--- | :--- | :--- |
| Box shadow / outer glow | `box-shadow` | `COLOR x y blur spread` | `box-shadow: #5fd0ff 0 0 32px 8px;` |
| Drop shadow (alpha-shaped) | `filter: drop-shadow(...)` | `COLOR x y blur` | `filter: drop-shadow(#5fd0ff80 0 0 20px);` |
| Linear gradient | `decorator: linear-gradient(...)` | `angle, color, color, ...` | `decorator: linear-gradient(45deg, #ff6a00, #ee0979);` |
| Regular polygon fill | `decorator: polygon(...)` | `sides, fill[, rotation]` (`fill` = color, `radial-gradient(...)`, or `linear-gradient(...)`) | `decorator: polygon(6, radial-gradient(#fff, #C9A24B 55%, #7A5A2E));` |
| Backdrop blur | `backdrop-filter` | `blur(Npx)` | `backdrop-filter: blur(8px);` |
| Blur filter | `filter` | `blur(Npx)` | `filter: blur(4px);` |
| Mask | `mask-image` | `horizontal-gradient(COLOR COLOR)` | `mask-image: horizontal-gradient(#000f #0000);` |
| Uniform image tint | `image-color` | `COLOR` | `image-color: #ffcc66;` |
| Luminance-key image tint | `decorator: image-tint(url)` + `image-tint-color`/`-mode`/`-threshold` | `url` / `COLOR` / `none\|multiply\|luminance-multiply\|screen` / number | `decorator: image-tint(runes-base.png); image-tint-mode: luminance-multiply;` |

`box-shadow` and `filter: drop-shadow` differ: `box-shadow` is a rectangular (raster-box) shadow, while `drop-shadow` follows the element's alpha shape (e.g. rounded corners). They are independent and can be combined.

### How-to: a cyan glow

Use `box-shadow` for a prominent rectangular halo, optionally combined with `drop-shadow` for a shape-accurate glow. A dark background maximises contrast.

```css
.glow {
    background-color: #1e2a50;
    box-shadow: #5fd0ff 0 0 32px 8px;            /* color first; big blur + spread = wide halo */
    filter: drop-shadow(#5fd0ff80 0 0 20px);     /* follows the rounded shape; 80 = 50% alpha */
}
```

### How-to: a gradient fill

Gradients are decorators, not backgrounds. The angle comes first, then two or more color stops.

```css
.grad {
    decorator: linear-gradient(45deg, #ff6a00, #ee0979);
}
```

You can also use a gradient on a section to give backdrop-blur rich content to smear:

```css
.section-gradient {
    decorator: linear-gradient(135deg, #6a0ddc, #e02828, #0f8a4a);
}
```

### How-to: a regular polygon (e.g. a hexagon node)

`decorator: polygon(<sides>, <color>[, <rotation>])` fills the element's box with a solid-color regular N-gon (a triangle-fan inscribed in a circle of radius = half the SHORTER box dimension, centred in the box). `sides` must be an integer in the range **`[3, 1024]`**; `rotation` is optional (degrees, any finite angle including negatives), and its default orientation points the first vertex straight up. There is **no separate polygon-glow or polygon-clip API** — reuse the existing `filter: drop-shadow(...)` (follows the element's alpha shape, so it outlines the polygon exactly) and `mask-image: polygon(...)` (any decorator, including `polygon`, can be used as a mask source) instead of adding new surface area.

**Invalid input is rejected, not guessed.** Because RCSS can come from a less-trusted source (a theme, a mod, dynamically generated content), the `sides` argument is validated defensively: an empty `polygon()`, or a `sides` value outside `[3, 1024]` (including negatives, `polygon(2)`, or a hostile `polygon(100000)` / `polygon(1e30)` that would otherwise tessellate a huge mesh), makes the decorator **ignored** — the element renders with no polygon (its own `background-color`, if any, shows through) and a warning is logged naming the received value and the valid range. Correct usage (`polygon(6, #5fd0ff)`) is never affected. `rotation` is not range-checked (any finite angle is valid; a non-finite angle falls back to no rotation).

```css
.hex {
    decorator: polygon(6, #5fd0ff);           /* hexagon, vertex pointing up, sides>=3 required */
}
.hex-glow {
    decorator: polygon(6, #5fd0ff);
    filter: drop-shadow(#5fd0ff80 0 0 20px);  /* glow that hugs the hexagon outline -- zero new code */
}
.hex-clip {
    background-color: #ffffff;
    mask-image: polygon(6, #000f);            /* clips background-color to the hexagon shape */
}
```

For a non-square element the polygon does **not** stretch to fill the longer axis — it stays a regular (equilateral) shape, centred, with empty box margin on the longer axis (same idea as a circular `border-radius` on a rectangle). Size a square element if you want the polygon to touch every edge.

### How-to: a polygon with a gradient fill (e.g. a metallic badge)

The `<fill>` argument of `polygon(<sides>, <fill>[, <rotation>])` accepts a `radial-gradient(...)` or `linear-gradient(...)` in place of a solid color — this gives the polygon a real per-pixel color ramp (the same fragment shader RmlUi's own `radial-gradient`/`linear-gradient` decorators use), not a faceted approximation:

```css
.badge {
    /* Radial: bright highlight in the middle, dark toward the rim -- classic embossed-disc volume.
       "at 40% 35%" moves the highlight up-left for an off-center light source; the last stop (t=1)
       always lands exactly on the polygon's own inscribed-circle edge. */
    decorator: polygon(6, radial-gradient(circle at 40% 35%, #F0D98C, #C9A24B 55%, #7A5A2E 100%));
}
.diagonal-hex {
    /* Linear: angle is mandatory (no CSS "to <side>" keyword support), 0deg = up, clockwise. */
    decorator: polygon(6, linear-gradient(135deg, #6a0ddc, #0f8a4a));
}
```

Syntax notes (see `decorator_polygon.cpp`'s `ParseFill()` for the exact grammar):

- **Radial:** `radial-gradient([circle [at <x%> <y%>],] <color> <position%>?, <color> <position%>?, ...)`. The `circle at ...` prefix is optional (default centre `50% 50%`); only `circle` is supported (no `ellipse` — the polygon's fill shape is always its own inscribed circle). Stop `<position%>` is optional and auto-spaced like CSS when omitted; the **last** stop always lands on the polygon's own inscribed-circle radius (not CSS's farthest-corner default), which is what makes the outer ring line up with the shape's own edge.
- **Linear:** `linear-gradient(<angle>, <color> <position%>?, <color> <position%>?, ...)`. The angle is **mandatory** here — this implementation does not support the CSS `to <side>` keyword form. The gradient line spans exactly the polygon's inscribed-circle diameter, centred on the polygon, independent of `rotation` (rotation only turns the N-gon's vertices, never the fill axis).
- Stop colors accept the same syntax as a plain `<color>` (hex, `rgb()`/`rgba()`, named colors). Stop positions must be a percentage (or omitted for auto-spacing) — lengths (`px`, `em`, ...) are not supported for a polygon fill stop and are rejected.
- Solid-color fill (`polygon(6, #C9A24B)`) is unchanged and unaffected by any of the above.
- A malformed fill (bad color, bad stop, missing linear angle, empty gradient) makes the **whole decorator** ignored, fail-high, exactly like an out-of-range `sides` — never a partial or garbage render.
- **Stop count limit:** up to **16 stops** per gradient — inherited from RmlUi's own gradient shader (`MAX_NUM_STOPS`, shared with the native `radial-gradient`/`linear-gradient` decorators, not something this feature introduces). Extra stops beyond the 16th are silently clamped, not rejected.

### How-to: backdrop blur

`backdrop-filter: blur(...)` blurs whatever is **behind** a semi-transparent element. It only looks like anything if there is content behind it, so place the card over a gradient or busy background.

```css
.blur {
    background-color: #ffffff20;   /* semi-transparent white (alpha = 20) */
    backdrop-filter: blur(8px);
}
```

### How-to: a fading mask

`mask-image` fades an element using a gradient as the alpha channel. A horizontal gradient from opaque to transparent fades the element left-to-right.

```css
.mask {
    background-color: #00d18f;                      /* solid teal */
    mask-image: horizontal-gradient(#000f #0000);   /* opaque -> transparent */
}
```

> **Important:** the `mask` effect requires a **real GPU**. Under Mesa/llvmpipe (the software renderer used in headless CI) the dual-sampler mask shader crashes (`free(): invalid next size`). This is a Mesa bug, not a glintfx bug. The headless test variant omits the mask card.

### How-to: style scrollbars

RmlUi generates scrollbars automatically whenever an element's content overflows its box and the relevant `overflow` property computes to `auto` or `scroll`. There is **no separate C++ API for scrollbar appearance** — you style the elements RmlUi creates internally, the same way you style anything else, with ordinary RCSS selectors. No new glintfx code is involved.

**Enabling scroll:**

| Property | Effect |
| :--- | :--- |
| `overflow-y: auto;` | Vertical scrollbar only |
| `overflow-x: auto;` | Horizontal scrollbar only |
| `overflow: auto;` | Both axes (shorthand for `overflow-x` + `overflow-y`) |

Accepted keywords: `visible` (default — no clipping, no scrollbar), `hidden` (clips content, no scrollbar), `auto` (scrollbar appears only when content actually overflows the box), `scroll` (scrollbar is always shown, even when content fits).

```css
.menu-list {
    display: block;
    height: 320dp;      /* a bounded box -- overflow only triggers past this size */
    overflow-y: auto;   /* vertical scrollbar when content is taller than the box */
}
```

**The scrollbar elements.** RmlUi appends non-DOM child elements to the scrolling element; reach them with an ordinary descendant selector off your own class:

| Element (RCSS tag) | Role |
| :--- | :--- |
| `scrollbarvertical` | The whole vertical scrollbar assembly (track + thumb + 2 arrows) |
| `scrollbarhorizontal` | The whole horizontal scrollbar assembly |
| `slidertrack` | The channel the thumb slides along |
| `sliderbar` | The thumb — the part you drag |
| `sliderarrowdec` | The "decrement" arrow button (scrolls up for vertical, left for horizontal) |
| `sliderarrowinc` | The "increment" arrow button (scrolls down for vertical, right for horizontal) |
| `scrollbarcorner` | The small square where a vertical and a horizontal scrollbar meet (only relevant when both axes overflow at once) |

> **Specificity gotcha (overriding glintfx's built-in scrollbar defaults):** glintfx ships minimal UA-stylesheet defaults for these elements (see `ua_stylesheet.hpp`) so a scrollbar is visible and usable even with zero authored RCSS. Those defaults use **two-tag compound selectors** (e.g. `scrollbarvertical sliderbar`), which RmlUi scores at roughly **20000 specificity** (~10000 per tag in the selector chain). A single-tag override written in isolation, like `sliderbar { background-color: red; }` (~10000), is **lower** specificity and will **not** override the default — RmlUi resolves conflicting declarations by specificity score, not by "author vs. built-in". To win, either match the same compound-tag shape (`scrollbarvertical sliderbar { ... }`, as every example on this page already does) or add a class/id ancestor to unambiguously outrank it (`.scrollable scrollbarvertical sliderbar { ... }` or `#my-list scrollbarvertical sliderbar { ... }`).

**Bar width (vertical) / height (horizontal) — the scrollbar's own thickness:**

```css
.menu-list scrollbarvertical   { width: 14dp; }   /* thickness of a vertical bar   */
.menu-list scrollbarhorizontal { height: 14dp; }  /* thickness of a horizontal bar */
```

**Thumb color:** `background-color` (or `background`/`decorator` for a gradient or an image) on `sliderbar`. Pseudo-classes work exactly like on any other element.

```css
.menu-list scrollbarvertical sliderbar {
    background-color: #5fd0ffcc;
    border-radius: 4dp;
}
.menu-list scrollbarvertical sliderbar:hover {
    background-color: #5fd0ffff;
}
```

**Arrow colors:** `background-color`/`decorator` on `sliderarrowdec` and `sliderarrowinc`. They're two independent elements — one at each end of the track — so style them separately if you want, say, different icons.

```css
.menu-list scrollbarvertical sliderarrowdec,
.menu-list scrollbarvertical sliderarrowinc {
    height: 14dp;
    background-color: #5fd0ff40;
}
```

**Fixed-size thumb:** set an explicit `height` on `sliderbar` for a vertical bar (`width` for a horizontal bar). Any explicit length opts the thumb **out** of RmlUi's automatic proportional sizing — it stays exactly that size no matter how much content overflows.

```css
.fixed-thumb scrollbarvertical sliderbar {
    height: 40dp;   /* always 40dp tall, regardless of content length */
}
```

**Min/max thumb size (still proportional, just clamped):** `min-height`/`max-height` on `sliderbar` (vertical) or `min-width`/`max-width` (horizontal) bound the *proportional* size RmlUi computes — the thumb keeps shrinking and growing with content length, just never below `min-*` or above `max-*`. This only has an effect while `height`/`width` is left at its default (`auto`) — do not combine with an explicit fixed size above.

```css
.menu-list scrollbarvertical sliderbar {
    min-height: 24dp;   /* stays big enough to grab even in a very long list */
    max-height: 120dp;  /* never balloons to dominate a short list */
}
```

**Proportional auto-size (the default — nothing to configure):** leave `height`/`width` and `min-*`/`max-*` unset on `sliderbar` and RmlUi automatically sizes the thumb proportional to `visible content / total content` (the same math a browser scrollbar uses), recomputed every time the content or box size changes. This is what you get for free the moment you enable a scrollbar with `overflow`, with zero extra RCSS.

**Track:** `background-color`/`decorator`/`border` on `slidertrack`.

```css
.menu-list scrollbarvertical slidertrack {
    background-color: #1a1a2e80;
    border-radius: 4dp;
}
```

**Consolidated example — a game menu scrollbar:**

```css
.menu-list {
    display: block;
    height: 320dp;
    overflow-y: auto;
}

.menu-list scrollbarvertical {
    width: 14dp;
}

.menu-list scrollbarvertical slidertrack {
    background-color: #1a1a2e80;
    border-radius: 4dp;
}

.menu-list scrollbarvertical sliderbar {
    background-color: #5fd0ffcc;   /* proportional thumb: no height set -> auto */
    border-radius: 4dp;
    min-height: 24dp;              /* clamp: never too small to grab      */
    max-height: 120dp;             /* clamp: never dominates a short list */
}
.menu-list scrollbarvertical sliderbar:hover {
    background-color: #5fd0ffff;
}

.menu-list scrollbarvertical sliderarrowdec,
.menu-list scrollbarvertical sliderarrowinc {
    height: 14dp;
    background-color: #5fd0ff40;
}
.menu-list scrollbarvertical sliderarrowdec:hover,
.menu-list scrollbarvertical sliderarrowinc:hover {
    background-color: #5fd0ff80;
}
```

Horizontal scrollbars follow the exact same pattern under `scrollbarhorizontal`, swapping `height`⇄`width` on `sliderbar` (the thumb's "length" axis matches the scroll axis: `height`/`min-height`/`max-height` for vertical, `width`/`min-width`/`max-width` for horizontal).

> Scroll **sound** (the audio cue played when the user scrolls) is a host-side concern set via `set_scroll_callback` (v0.6.0) — it is not part of RCSS styling; see the CHANGELOG for that API.

### How-to: tint / recolor a decorator image (`image-color`)

`image-color` is a native RmlUi 6.3 RCSS property (`StyleSheetSpecification.cpp:350`) that tints the texture sampled by an `image`/`tiled-*`/ninepatch decorator — glintfx does not add any code for this, it inherits the capability by using stock RmlUi decorators.

```css
.rune-frame {
    decorator: image( runes-base.png );
    image-color: var(--domain-color, #ffffff);   /* defaults to no tint (white = identity) */
    transition: image-color 0.3s;                 /* animates smoothly on any state change */
}
.rune-frame.domain-fire   { --domain-color: #ff6a3c; }
.rune-frame.domain-frost  { --domain-color: #6ac8ff; }

/* Or drive it from a @keyframes pulse instead of a state class: */
@keyframes pulse-glow {
    0%   { image-color: #ffffff; }
    50%  { image-color: #ffcc66; }
    100% { image-color: #ffffff; }
}
.rune-frame.charging { animation: pulse-glow 1.2s infinite; }
```

**What it does.** The renderer multiplies the sampled texel by `image-color` in premultiplied space (`DecoratorTiled.cpp:80`, `quad_colour = computed.image_color().ToPremultiplied(opacity)`, then `MeshUtilities::GenerateQuad(...)` bakes that color into the quad's vertex color): `out.rgb = texel.rgb × tint.rgb`, `out.a = texel.a × tint.a`. White texels become the tint color exactly; black/dark texels stay dark (multiplying by anything leaves near-zero near-zero). It is a genuine multiply, not an overlay or a hue-replace — it is the same mechanism CSS `filter` tint hacks approximate, done natively per-pixel.

**Real use case: one neutral base texture, N domains.** Author a single grayscale/neutral `runes-base.png` — white or light-gray runes on a dark stone base. Ship it once. Recolor per-domain purely through RCSS by setting `image-color` (a CSS custom property per domain, or driven by a data-model binding), with zero extra textures and zero C++ code. Because `image-color` is a `color`-typed property, it is **interpolable**: `transition`/`@keyframes` animate it exactly like `background-color`, so a domain swap or a "charging" pulse can fade smoothly instead of popping.

`var()`/custom properties resolve normally here too (`ElementStyle::ResolveVariables`, the same substitution path every other RCSS property uses) — `image-color: var(--domain-color);` works, including inside `@keyframes`.

**Honest limit: it is a uniform multiply, not a luminance-key.** `image-color` tints the *entire* texture — including texels that already carry their own color. If `runes-base.png` had gold-colored detail pixels mixed with the white/neutral runes, `image-color` would multiply those gold pixels too (shifting their hue, not just the neutral ones), because the shader has no concept of "which pixels are neutral" — it is one scalar multiply applied uniformly. Preserving pre-colored detail while tinting only the neutral/light pixels needs a different tool: `image-tint(luminance-multiply)`, below (v0.7.0).

### How-to: luminance-key tint (`image-tint()`)

`image-color` (above) tints everything uniformly. `image-tint(luminance-multiply)` tints only the light+neutral texels of a texture, leaving already-saturated (e.g. gold trim) and dark (e.g. stone) texels alone — a genuinely new glintfx decorator + fragment shader, not a native RmlUi capability. See [ADR-0010](adr/0010-image-tint-luminance-key.md) for the full design rationale.

**Syntax.** The decorator takes a single positional argument, the texture URL; the tint itself is controlled by three standalone, globally-registered, independently-animatable RCSS properties (not decorator arguments):

| Property | Values | Default |
| :--- | :--- | :--- |
| `decorator: image-tint( url )` | a texture path | -- |
| `image-tint-color` | a `color` (accepts `var()`) | `white` |
| `image-tint-mode` | `none` \| `multiply` \| `luminance-multiply` \| `screen` | `none` |
| `image-tint-threshold` | a number, clamped to `[0, 0.999]` | `0.55` |

**The four modes.**
- `none` — true no-op. Identical cost/behavior to a plain `image()` decorator; no shader is even compiled.
- `multiply` — same math as native `image-color` (`straight × tint_color`), but routed through this decorator instead. Use this only if you also need `image-tint-mode` to be switchable at runtime alongside `luminance-multiply`.
- `luminance-multiply` — the luminance-key tint: `w = smoothstep(threshold, 1, luminance) × (1 − saturation)`, then `tinted = mix(straight, straight × tint_color, w)`. Light+neutral texels (high luminance, low saturation) get tinted; dark or already-saturated texels are weighted toward zero.
- `screen` — `1 − (1 − straight) × (1 − tint_color)`, a brightening blend (useful for glow-style tints on dark bases, unlike `multiply`'s darkening behavior).

**Recoloring one base texture into N domains, with animation.**

```css
.card {
    decorator: image-tint( runes-base.png );
    image-tint-mode: luminance-multiply;
    image-tint-color: var(--domain-color, #ffffff);
    image-tint-threshold: 0.55;
    transition: image-tint-color 0.3s;   /* animates smoothly, same as image-color */
}
.card.domain-fire  { --domain-color: #ff6a3c; }
.card.domain-frost { --domain-color: #6ac8ff; }

/* Or drive it from a @keyframes pulse instead of a state class: */
@keyframes tint-pulse {
    0%   { image-tint-color: #ffffff; }
    50%  { image-tint-color: #ffcc66; }
    100% { image-tint-color: #ffffff; }
}
.card.charging { animation: tint-pulse 1.2s infinite; }
```

One `runes-base.png` (white runes + navy stone + gold trim), N domain colors at runtime, zero extra textures shipped, zero C++ code — this is the GusWorld use case `image-tint()` was built for.

**Honest fidelity note: "preserves saturated texels" is approximate, not exact.** The `(1 − saturation)` weighting is *linear*, not a hard cutoff — a texel that is highly-but-not-fully saturated (e.g. a gold trim with `saturation ≈ 0.69`) still receives a non-trivial fraction of the tint at the default `threshold: 0.55`, roughly 13-16% of a full tint in that case. This is not a bug; it is exactly what the formula computes for any texel whose luminance clears the threshold.

**Tuning `image-tint-threshold` per asset.** To preserve a specific saturated color near-totally, raise `image-tint-threshold` above *that color's own luminance* — this pushes its `smoothstep` weight toward zero. **Trade-off:** raising the threshold also proportionally suppresses tinting of the neutral zone the effect exists to recolor, so keep `threshold` comfortably below the luminance of the texels you actually want tinted. There is no universal default that "just works" for every asset — treat `image-tint-threshold` as a per-asset dial, tuned against your own texture's luminance histogram, not a constant to copy-paste. `0.55` is a reasonable starting point for a base shaped like the example above, not a guarantee.

### Putting it together

The showcase uses two sections: a near-black one for the glow and gradient cards (dark background makes the halo unmistakable), and a colourful gradient section for the blur and mask cards (so backdrop-blur has something to smear and the mask fades over rich content). Read [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss) and [`showcase.rml`](../glintfx/demos/showcase/showcase.rml) for the full, working layout.

### Related

- Tutorial: [`getting-started.md`](getting-started.md).
- Source of truth: [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss).
- Upstream RmlUi docs: https://mikke89.github.io/RmlUiDoc/

---

## Português

### RCSS não é exatamente CSS

A estilização do RmlUi 6.3 parece CSS mas difere de formas importantes. Lembre destas regras ou os efeitos simplesmente não se aplicam:

- **A cor vem primeiro** em `box-shadow` e `drop-shadow` (o CSS a coloca por último).
- **Gradientes usam `decorator:`**, não `background:`.
- **As cores são hex de 8 dígitos** `#rrggbbaa`; os dois últimos dígitos são o alpha. A forma curta de 4 dígitos (ex.: `#000f`) funciona para os casos simples usados em máscaras, mas prefira 8 dígitos no resto.
- **Todo elemento usa `display: inline` por padrão** (diferente do HTML, onde `div` é block). Defina `display: block;` para as caixas empilharem e dimensionarem como esperado.

### Referência de efeitos

| Efeito | Propriedade RCSS | Ordem dos argumentos | Exemplo |
| :--- | :--- | :--- | :--- |
| Box shadow / glow externo | `box-shadow` | `COR x y blur spread` | `box-shadow: #5fd0ff 0 0 32px 8px;` |
| Drop shadow (segue o alpha) | `filter: drop-shadow(...)` | `COR x y blur` | `filter: drop-shadow(#5fd0ff80 0 0 20px);` |
| Gradiente linear | `decorator: linear-gradient(...)` | `ângulo, cor, cor, ...` | `decorator: linear-gradient(45deg, #ff6a00, #ee0979);` |
| Preenchimento de polígono regular | `decorator: polygon(...)` | `lados, preenchimento[, rotação]` (`preenchimento` = cor, `radial-gradient(...)`, ou `linear-gradient(...)`) | `decorator: polygon(6, radial-gradient(#fff, #C9A24B 55%, #7A5A2E));` |
| Backdrop blur | `backdrop-filter` | `blur(Npx)` | `backdrop-filter: blur(8px);` |
| Filtro blur | `filter` | `blur(Npx)` | `filter: blur(4px);` |
| Mask | `mask-image` | `horizontal-gradient(COR COR)` | `mask-image: horizontal-gradient(#000f #0000);` |
| Tingimento uniforme de imagem | `image-color` | `COR` | `image-color: #ffcc66;` |
| Tingimento de imagem por luminance-key | `decorator: image-tint(url)` + `image-tint-color`/`-mode`/`-threshold` | `url` / `COR` / `none\|multiply\|luminance-multiply\|screen` / número | `decorator: image-tint(runes-base.png); image-tint-mode: luminance-multiply;` |

`box-shadow` e `filter: drop-shadow` diferem: `box-shadow` é uma sombra retangular (box-raster), enquanto `drop-shadow` segue a forma do alpha do elemento (ex.: cantos arredondados). São independentes e podem ser combinados.

### How-to: um glow ciano

Use `box-shadow` para um halo retangular proeminente, opcionalmente combinado com `drop-shadow` para um glow preciso por forma. Um fundo escuro maximiza o contraste.

```css
.glow {
    background-color: #1e2a50;
    box-shadow: #5fd0ff 0 0 32px 8px;            /* cor primeiro; blur + spread grandes = halo largo */
    filter: drop-shadow(#5fd0ff80 0 0 20px);     /* segue a forma arredondada; 80 = 50% de alpha */
}
```

### How-to: um preenchimento com gradiente

Gradientes são decorators, não backgrounds. O ângulo vem primeiro, depois dois ou mais stops de cor.

```css
.grad {
    decorator: linear-gradient(45deg, #ff6a00, #ee0979);
}
```

Você também pode usar um gradiente numa seção para dar ao backdrop-blur conteúdo rico para desfocar:

```css
.section-gradient {
    decorator: linear-gradient(135deg, #6a0ddc, #e02828, #0f8a4a);
}
```

### How-to: um polígono regular (ex.: um nó hexagonal)

`decorator: polygon(<lados>, <cor>[, <rotação>])` preenche a caixa do elemento com um N-ágono regular de cor sólida (um triangle-fan inscrito num círculo de raio = metade da dimensão MENOR da caixa, centrado na caixa). `lados` deve ser um inteiro no range **`[3, 1024]`**; `rotação` é opcional (graus, qualquer ângulo finito incluindo negativos), e a orientação padrão aponta o primeiro vértice reto pra cima. **Não há API separada de glow-de-polígono ou clip-de-polígono** — reuse o `filter: drop-shadow(...)` já existente (segue a forma do alpha do elemento, então contorna o polígono exatamente) e `mask-image: polygon(...)` (qualquer decorator, incluindo `polygon`, pode ser usado como fonte de máscara) em vez de adicionar superfície nova.

**Entrada inválida é rejeitada, não adivinhada.** Como o RCSS pode vir de uma fonte menos confiável (um tema, um mod, conteúdo gerado dinamicamente), o argumento `lados` é validado defensivamente: um `polygon()` vazio, ou um valor de `lados` fora de `[3, 1024]` (incluindo negativos, `polygon(2)`, ou um `polygon(100000)` / `polygon(1e30)` hostil que senão tesselaria uma mesh gigante), faz o decorator ser **ignorado** — o elemento renderiza sem polígono (o próprio `background-color`, se houver, aparece) e um aviso é logado nomeando o valor recebido e o range válido. O uso correto (`polygon(6, #5fd0ff)`) nunca é afetado. `rotação` não é validada por range (qualquer ângulo finito é válido; um ângulo não-finito cai para sem rotação).

```css
.hex {
    decorator: polygon(6, #5fd0ff);           /* hexágono, vértice apontando pra cima, sides>=3 obrigatório */
}
.hex-glow {
    decorator: polygon(6, #5fd0ff);
    filter: drop-shadow(#5fd0ff80 0 0 20px);  /* glow que abraça o contorno do hexágono -- zero código novo */
}
.hex-clip {
    background-color: #ffffff;
    mask-image: polygon(6, #000f);            /* recorta o background-color na forma do hexágono */
}
```

Para um elemento não-quadrado o polígono **não** estica para preencher o eixo mais longo — permanece uma forma regular (equilátera), centrada, com margem vazia de caixa no eixo mais longo (mesma ideia de um `border-radius` circular num retângulo). Dimensione um elemento quadrado se quiser que o polígono toque todas as bordas.

### How-to: um polígono com preenchimento em gradiente (ex.: uma cabeça de parafuso de latão)

O argumento `<preenchimento>` de `polygon(<lados>, <preenchimento>[, <rotação>])` aceita um `radial-gradient(...)` ou `linear-gradient(...)` no lugar de uma cor sólida — isso dá ao polígono uma rampa de cor real por-pixel (o mesmo shader de fragmento que os próprios decorators `radial-gradient`/`linear-gradient` do RmlUi usam), não uma aproximação facetada:

```css
.cabeca-parafuso {
    /* Radial: highlight claro no meio, escuro em direção à borda -- volume clássico de "cabeça de
       parafuso". "at 40% 35%" move o highlight pra cima-esquerda, simulando uma fonte de luz
       fora do centro; o último stop (t=1) sempre cai exatamente na borda do círculo inscrito do
       próprio polígono. */
    decorator: polygon(6, radial-gradient(circle at 40% 35%, #F0D98C, #C9A24B 55%, #7A5A2E 100%));
}
.hex-diagonal {
    /* Linear: o ângulo é obrigatório (sem suporte à palavra-chave "to <side>" do CSS), 0deg = pra
       cima, sentido horário. */
    decorator: polygon(6, linear-gradient(135deg, #6a0ddc, #0f8a4a));
}
```

Notas de sintaxe (ver `ParseFill()` em `decorator_polygon.cpp` para a gramática exata):

- **Radial:** `radial-gradient([circle [at <x%> <y%>],] <cor> <posição%>?, <cor> <posição%>?, ...)`. O prefixo `circle at ...` é opcional (centro default `50% 50%`); só `circle` é suportado (sem `ellipse` — a forma de preenchimento do polígono é sempre o próprio círculo inscrito). A `<posição%>` do stop é opcional e auto-espaçada como no CSS quando omitida; o **último** stop sempre cai no raio do círculo inscrito do próprio polígono (não no default farthest-corner do CSS), o que faz o anel externo alinhar com a borda da própria forma.
- **Linear:** `linear-gradient(<ângulo>, <cor> <posição%>?, <cor> <posição%>?, ...)`. O ângulo é **obrigatório** aqui -- esta implementação não suporta a forma de palavra-chave `to <side>` do CSS. A linha do gradiente cobre exatamente o diâmetro do círculo inscrito do polígono, centrada no polígono, independente de `rotação` (rotação só gira os vértices do N-ágono, nunca o eixo do preenchimento).
- Cores de stop aceitam a mesma sintaxe de uma `<cor>` simples (hex, `rgb()`/`rgba()`, cores nomeadas). Posições de stop devem ser uma porcentagem (ou omitidas para auto-espaçamento) — comprimentos (`px`, `em`, ...) não são suportados para um stop de preenchimento de polígono e são rejeitados.
- Preenchimento de cor sólida (`polygon(6, #C9A24B)`) continua inalterado e não é afetado por nada disso.
- Um preenchimento malformado (cor inválida, stop inválido, ângulo do linear ausente, gradiente vazio) faz o **decorator inteiro** ser ignorado, fail-high, exatamente como um `lados` fora do range -- nunca um render parcial ou lixo.
- **Limite de stops:** até **16 stops** por gradiente — herdado do próprio shader de gradiente do RmlUi (`MAX_NUM_STOPS`, compartilhado com os decorators nativos `radial-gradient`/`linear-gradient`, não algo introduzido por esta feature). Stops além do 16º são silenciosamente clampados, não rejeitados.

### How-to: backdrop blur

`backdrop-filter: blur(...)` desfoca o que está **atrás** de um elemento semi-transparente. Só aparece algo se houver conteúdo atrás, então coloque o card sobre um gradiente ou fundo movimentado.

```css
.blur {
    background-color: #ffffff20;   /* branco semi-transparente (alpha = 20) */
    backdrop-filter: blur(8px);
}
```

### How-to: uma máscara com fade

`mask-image` faz o fade de um elemento usando um gradiente como canal alpha. Um gradiente horizontal de opaco a transparente faz o fade da esquerda para a direita.

```css
.mask {
    background-color: #00d18f;                      /* teal sólido */
    mask-image: horizontal-gradient(#000f #0000);   /* opaco -> transparente */
}
```

> **Importante:** o efeito `mask` exige uma **GPU real**. Sob Mesa/llvmpipe (o renderer de software usado em CI headless) o shader de mask dual-sampler crasha (`free(): invalid next size`). Isso é um bug do Mesa, não do glintfx. A variante de teste headless omite o card mask.

### How-to: estilizar barras de rolagem (scrollbars)

O RmlUi gera barras de rolagem automaticamente sempre que o conteúdo de um elemento excede sua caixa e a propriedade `overflow` relevante computa para `auto` ou `scroll`. **Não há API C++ separada para a aparência da scrollbar** — você estiliza os elementos que o RmlUi cria internamente, do mesmo jeito que estiliza qualquer outra coisa, com seletores RCSS comuns. Nenhum código novo do glintfx está envolvido.

**Habilitando a rolagem:**

| Propriedade | Efeito |
| :--- | :--- |
| `overflow-y: auto;` | Só barra vertical |
| `overflow-x: auto;` | Só barra horizontal |
| `overflow: auto;` | Ambos os eixos (shorthand para `overflow-x` + `overflow-y`) |

Palavras-chave aceitas: `visible` (padrão -- sem clip, sem barra), `hidden` (recorta o conteúdo, sem barra), `auto` (a barra aparece só quando o conteúdo realmente excede a caixa), `scroll` (a barra é sempre exibida, mesmo quando o conteúdo cabe).

```css
.menu-list {
    display: block;
    height: 320dp;      /* uma caixa com altura fixa -- o overflow só dispara acima disso */
    overflow-y: auto;   /* barra vertical quando o conteúdo for mais alto que a caixa */
}
```

**Os elementos da scrollbar.** O RmlUi anexa elementos filhos não-DOM ao elemento rolável; alcance-os com um seletor descendente comum a partir da sua própria classe:

| Elemento (tag RCSS) | Papel |
| :--- | :--- |
| `scrollbarvertical` | O conjunto inteiro da barra vertical (trilho + thumb + 2 setas) |
| `scrollbarhorizontal` | O conjunto inteiro da barra horizontal |
| `slidertrack` | O canal por onde o thumb desliza |
| `sliderbar` | O thumb -- a parte que você arrasta |
| `sliderarrowdec` | O botão de seta "decremento" (rola pra cima na vertical, pra esquerda na horizontal) |
| `sliderarrowinc` | O botão de seta "incremento" (rola pra baixo na vertical, pra direita na horizontal) |
| `scrollbarcorner` | O quadradinho onde uma barra vertical e uma horizontal se encontram (só relevante quando os dois eixos excedem ao mesmo tempo) |

> **Pegadinha de especificidade (sobrepondo os defaults de scrollbar embutidos da glintfx):** a glintfx embarca defaults mínimos de UA-stylesheet para esses elementos (ver `ua_stylesheet.hpp`) para que uma scrollbar fique visível e usável mesmo com zero RCSS autorado. Esses defaults usam **seletores compostos de duas tags** (ex.: `scrollbarvertical sliderbar`), que o RmlUi pontua em aproximadamente **20000 de especificidade** (~10000 por tag na cadeia do seletor). Uma sobreposição de uma tag só, escrita isoladamente, como `sliderbar { background-color: red; }` (~10000), tem especificidade **menor** e **não** sobrepõe o default -- o RmlUi resolve declarações conflitantes por pontuação de especificidade, não por "autor vs. embutido". Para vencer, ou iguale o mesmo formato composto de tags (`scrollbarvertical sliderbar { ... }`, como todo exemplo desta página já faz) ou adicione um ancestral classe/id para vencer sem ambiguidade (`.scrollable scrollbarvertical sliderbar { ... }` ou `#minha-lista scrollbarvertical sliderbar { ... }`).

**Largura da barra (vertical) / altura da barra (horizontal) -- a espessura da própria scrollbar:**

```css
.menu-list scrollbarvertical   { width: 14dp; }   /* espessura de uma barra vertical   */
.menu-list scrollbarhorizontal { height: 14dp; }  /* espessura de uma barra horizontal */
```

**Cor da barra (o thumb):** `background-color` (ou `background`/`decorator` para gradiente ou imagem) no `sliderbar`. Pseudo-classes funcionam exatamente como em qualquer outro elemento.

```css
.menu-list scrollbarvertical sliderbar {
    background-color: #5fd0ffcc;
    border-radius: 4dp;
}
.menu-list scrollbarvertical sliderbar:hover {
    background-color: #5fd0ffff;
}
```

**Cor das pontas (as setas):** `background-color`/`decorator` em `sliderarrowdec` e `sliderarrowinc`. São dois elementos independentes -- um em cada ponta do trilho -- então estilize-os separadamente se quiser, por exemplo, ícones diferentes.

```css
.menu-list scrollbarvertical sliderarrowdec,
.menu-list scrollbarvertical sliderarrowinc {
    height: 14dp;
    background-color: #5fd0ff40;
}
```

**Tamanho fixo do thumb:** defina um `height` explícito no `sliderbar` para uma barra vertical (`width` para uma barra horizontal). Qualquer comprimento explícito tira o thumb do dimensionamento proporcional automático do RmlUi -- ele fica exatamente naquele tamanho, não importa quanto o conteúdo exceda.

```css
.fixed-thumb scrollbarvertical sliderbar {
    height: 40dp;   /* sempre 40dp de altura, independente do tamanho do conteúdo */
}
```

**Tamanho mín/máx do thumb (ainda proporcional, só limitado):** `min-height`/`max-height` no `sliderbar` (vertical) ou `min-width`/`max-width` (horizontal) limitam o tamanho *proporcional* que o RmlUi calcula -- o thumb continua encolhendo e crescendo com o tamanho do conteúdo, só nunca abaixo de `min-*` nem acima de `max-*`. Isso só tem efeito enquanto `height`/`width` fica no padrão (`auto`) -- não combine com um tamanho fixo explícito acima.

```css
.menu-list scrollbarvertical sliderbar {
    min-height: 24dp;   /* fica grande o bastante pra agarrar mesmo numa lista bem longa */
    max-height: 120dp;  /* nunca infla a ponto de dominar uma lista curta */
}
```

**Tamanho proporcional automático (o padrão -- nada para configurar):** deixe `height`/`width` e `min-*`/`max-*` sem definir no `sliderbar` e o RmlUi dimensiona o thumb automaticamente proporcional a `conteúdo visível / conteúdo total` (a mesma matemática de uma scrollbar de navegador), recalculado toda vez que o conteúdo ou o tamanho da caixa mudam. É isso que você ganha de graça no momento em que habilita uma scrollbar com `overflow`, sem RCSS extra nenhum.

**Trilho:** `background-color`/`decorator`/`border` no `slidertrack`.

```css
.menu-list scrollbarvertical slidertrack {
    background-color: #1a1a2e80;
    border-radius: 4dp;
}
```

**Exemplo consolidado -- uma scrollbar de menu de jogo:**

```css
.menu-list {
    display: block;
    height: 320dp;
    overflow-y: auto;
}

.menu-list scrollbarvertical {
    width: 14dp;
}

.menu-list scrollbarvertical slidertrack {
    background-color: #1a1a2e80;
    border-radius: 4dp;
}

.menu-list scrollbarvertical sliderbar {
    background-color: #5fd0ffcc;   /* thumb proporcional: sem height definido -> auto */
    border-radius: 4dp;
    min-height: 24dp;              /* limite: nunca pequeno demais pra agarrar   */
    max-height: 120dp;             /* limite: nunca domina uma lista curta       */
}
.menu-list scrollbarvertical sliderbar:hover {
    background-color: #5fd0ffff;
}

.menu-list scrollbarvertical sliderarrowdec,
.menu-list scrollbarvertical sliderarrowinc {
    height: 14dp;
    background-color: #5fd0ff40;
}
.menu-list scrollbarvertical sliderarrowdec:hover,
.menu-list scrollbarvertical sliderarrowinc:hover {
    background-color: #5fd0ff80;
}
```

Barras horizontais seguem exatamente o mesmo padrão sob `scrollbarhorizontal`, trocando `height`⇄`width` no `sliderbar` (o eixo de "comprimento" do thumb acompanha o eixo de rolagem: `height`/`min-height`/`max-height` na vertical, `width`/`min-width`/`max-width` na horizontal).

> O **som** da rolagem (o efeito sonoro tocado quando o usuário rola) é responsabilidade do host, configurado via `set_scroll_callback` (v0.6.0) -- não faz parte da estilização RCSS; ver o CHANGELOG para essa API.

### How-to: tingir / recolorir a imagem de um decorator (`image-color`)

`image-color` é uma propriedade RCSS nativa do RmlUi 6.3 (`StyleSheetSpecification.cpp:350`) que tinge a textura amostrada por um decorator `image`/`tiled-*`/ninepatch -- a glintfx não adiciona nenhum código pra isso, herda a capacidade por usar decorators nativos do RmlUi.

```css
.moldura-runas {
    decorator: image( runes-base.png );
    image-color: var(--cor-dominio, #ffffff);   /* padrão sem tingimento (branco = identidade) */
    transition: image-color 0.3s;                /* anima suavemente em qualquer troca de estado */
}
.moldura-runas.dominio-fogo   { --cor-dominio: #ff6a3c; }
.moldura-runas.dominio-gelo   { --cor-dominio: #6ac8ff; }

/* Ou dirija a partir de um @keyframes de pulso em vez de uma classe de estado: */
@keyframes pulso-glow {
    0%   { image-color: #ffffff; }
    50%  { image-color: #ffcc66; }
    100% { image-color: #ffffff; }
}
.moldura-runas.carregando { animation: pulso-glow 1.2s infinite; }
```

**O que faz.** O renderer multiplica o texel amostrado pelo `image-color` em espaço premultiplicado (`DecoratorTiled.cpp:80`, `quad_colour = computed.image_color().ToPremultiplied(opacity)`, e depois `MeshUtilities::GenerateQuad(...)` embute essa cor na cor de vértice do quad): `out.rgb = texel.rgb × tint.rgb`, `out.a = texel.a × tint.a`. Texels brancos viram exatamente a cor do tingimento; texels pretos/escuros continuam escuros (multiplicar qualquer coisa por perto-de-zero permanece perto-de-zero). É um multiply de verdade, não um overlay ou uma troca de matiz -- é o mesmo mecanismo que hacks de tingimento via `filter` do CSS aproximam, feito nativamente por-pixel aqui.

**Caso de uso real: uma textura base neutra, N domínios.** Autore uma única textura em escala de cinza/neutra `runes-base.png` -- runas brancas ou cinza-claras sobre uma base de pedra escura. Distribua-a uma vez só. Recolora por domínio puramente via RCSS definindo `image-color` (uma custom property CSS por domínio, ou dirigida por um binding de data-model), sem textura extra nenhuma e sem código C++ novo. Como `image-color` é uma propriedade do tipo `color`, ela é **interpolável**: `transition`/`@keyframes` a animam exatamente como `background-color`, então uma troca de domínio ou um pulso de "carregando" pode fazer fade suave em vez de trocar abruptamente.

`var()`/custom properties resolvem normalmente aqui também (`ElementStyle::ResolveVariables`, o mesmo caminho de substituição que qualquer outra propriedade RCSS usa) -- `image-color: var(--cor-dominio);` funciona, inclusive dentro de `@keyframes`.

**Limite honesto: é um multiply uniforme, não um luminance-key.** `image-color` tinge a textura *inteira* -- inclusive texels que já carregam cor própria. Se `runes-base.png` tivesse pixels de detalhe dourados misturados com as runas brancas/neutras, `image-color` multiplicaria esses pixels dourados também (deslocando o matiz deles, não só o dos neutros), porque o shader não tem noção de "quais pixels são neutros" -- é um multiply escalar único aplicado uniformemente. Preservar detalhe pré-colorido enquanto tinge só os pixels claros/neutros precisa de outra ferramenta: `image-tint(luminance-multiply)`, abaixo (v0.7.0).

### How-to: tingimento por luminance-key (`image-tint()`)

`image-color` (acima) tinge tudo uniformemente. `image-tint(luminance-multiply)` tinge só os texels claros+neutros de uma textura, preservando texels já saturados (ex.: trim ouro) e escuros (ex.: pedra) -- um decorator + fragment shader genuinamente novos da glintfx, não uma capacidade nativa do RmlUi. Ver [ADR-0010](adr/0010-image-tint-luminance-key.md) para a racional de design completa.

**Sintaxe.** O decorator recebe um único argumento posicional, a URL da textura; o tingimento em si é controlado por três propriedades RCSS standalone, registradas globalmente, animáveis independentemente (não são argumentos de decorator):

| Propriedade | Valores | Default |
| :--- | :--- | :--- |
| `decorator: image-tint( url )` | um caminho de textura | -- |
| `image-tint-color` | uma `color` (aceita `var()`) | `white` |
| `image-tint-mode` | `none` \| `multiply` \| `luminance-multiply` \| `screen` | `none` |
| `image-tint-threshold` | um número, clampado a `[0, 0.999]` | `0.55` |

**Os quatro modos.**
- `none` -- no-op de verdade. Custo/comportamento idêntico a um decorator `image()` simples; nenhum shader sequer é compilado.
- `multiply` -- mesma matemática do `image-color` nativo (`straight × tint_color`), mas roteada por este decorator. Use só se também precisar que `image-tint-mode` seja trocável em runtime junto de `luminance-multiply`.
- `luminance-multiply` -- o tingimento luminance-key: `w = smoothstep(threshold, 1, luminância) × (1 − saturação)`, depois `tinted = mix(straight, straight × tint_color, w)`. Texels claros+neutros (luminância alta, saturação baixa) são tingidos; texels escuros ou já saturados pesam pra perto de zero.
- `screen` -- `1 − (1 − straight) × (1 − tint_color)`, um blend de clareamento (útil pra tingimentos estilo-glow sobre bases escuras, ao contrário do comportamento de escurecimento do `multiply`).

**Recolorindo uma textura base em N domínios, com animação.**

```css
.card {
    decorator: image-tint( runes-base.png );
    image-tint-mode: luminance-multiply;
    image-tint-color: var(--cor-dominio, #ffffff);
    image-tint-threshold: 0.55;
    transition: image-tint-color 0.3s;   /* anima suavemente, igual ao image-color */
}
.card.dominio-fogo   { --cor-dominio: #ff6a3c; }
.card.dominio-gelo   { --cor-dominio: #6ac8ff; }

/* Ou dirija por um pulso @keyframes em vez de uma classe de estado: */
@keyframes tint-pulse {
    0%   { image-tint-color: #ffffff; }
    50%  { image-tint-color: #ffcc66; }
    100% { image-tint-color: #ffffff; }
}
.card.carregando { animation: tint-pulse 1.2s infinite; }
```

Uma única `runes-base.png` (runas brancas + pedra navy + trim ouro), N cores de domínio em runtime, zero textura extra distribuída, zero código C++ -- este é o caso de uso do GusWorld pra qual o `image-tint()` foi construído.

**Nota honesta de fidelidade: "preserva texels saturados" é aproximado, não exato.** A ponderação `(1 − saturação)` é *linear*, não um corte duro -- um texel altamente-mas-não-totalmente-saturado (ex.: um trim ouro com `saturação ≈ 0.69`) ainda recebe uma fração não-trivial do tingimento no `threshold: 0.55` default, cerca de 13-16% de um tingimento pleno nesse caso. Isso não é um bug; é exatamente o que a fórmula calcula pra qualquer texel cuja luminância ultrapasse o threshold.

**Tunando `image-tint-threshold` por asset.** Pra preservar uma cor saturada específica quase-totalmente, suba `image-tint-threshold` acima da *luminância própria daquela cor* -- isso empurra o peso `smoothstep` dela pra perto de zero. **Troca:** subir o threshold também suprime proporcionalmente o tingimento da zona neutra que o efeito existe pra recolorir, então mantenha `threshold` confortavelmente abaixo da luminância dos texels que você de fato quer tingidos. Não existe um default universal que "simplesmente funciona" pra todo asset -- trate `image-tint-threshold` como um dial por-asset, tunado contra o histograma de luminância da sua própria textura, não uma constante pra copiar-e-colar. `0.55` é um ponto de partida razoável pra uma base no formato do exemplo acima, não uma garantia.

### Juntando tudo

O showcase usa duas seções: uma quase-preta para os cards glow e gradiente (fundo escuro torna o halo inconfundível) e uma seção de gradiente colorido para os cards blur e mask (para o backdrop-blur ter o que desfocar e a mask fazer fade sobre conteúdo rico). Leia [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss) e [`showcase.rml`](../glintfx/demos/showcase/showcase.rml) para o layout completo e funcional.

### Relacionados

- Tutorial: [`getting-started.md`](getting-started.md).
- Fonte de verdade: [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss).
- Docs upstream do RmlUi: https://mikke89.github.io/RmlUiDoc/
