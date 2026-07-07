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

### Juntando tudo

O showcase usa duas seções: uma quase-preta para os cards glow e gradiente (fundo escuro torna o halo inconfundível) e uma seção de gradiente colorido para os cards blur e mask (para o backdrop-blur ter o que desfocar e a mask fazer fade sobre conteúdo rico). Leia [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss) e [`showcase.rml`](../glintfx/demos/showcase/showcase.rml) para o layout completo e funcional.

### Relacionados

- Tutorial: [`getting-started.md`](getting-started.md).
- Fonte de verdade: [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss).
- Docs upstream do RmlUi: https://mikke89.github.io/RmlUiDoc/
