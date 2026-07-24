# ADR-0018 — `draw_text` in the Draw2D atom: sovereign-C-core glyph source, top-left anchor, word-wrap/alignment in scope / `draw_text` no átomo Draw2D: fonte de glifo no núcleo C soberano, âncora topo-esquerdo, word-wrap/alinhamento no escopo

- **Status:** Accepted (2026-07-24) — leader decision (petrus) on the three questions (Q-A, Q-B,
  Q-C) CTO Caetano presented as options in the Onda 6 plan; Q-B additionally EXPANDED by the
  leader beyond the CTO's proposed cut (word-wrap and alignment, including justify, are IN for
  v0.23.0, not seeds). / decisão do líder (petrus) sobre as três perguntas (Q-A, Q-B, Q-C) que o
  CTO Caetano apresentou como opções no plano da Onda 6; a Q-B foi adicionalmente EXPANDIDA pelo
  líder além do corte proposto pelo CTO (word-wrap e alinhamento, inclusive justify, ENTRAM na
  v0.23.0, não ficam sementes).
- **Deciders:** petrus (líder, decisões Q-A/Q-B/Q-C incl. a expansão de escopo); Caetano (CTO,
  autor do plano, opções e recomendações nos três eixos); software-architect (registro do ADR).
- **Tags:** architecture, product-direction, dependency-profile, framework-mode, font-engine,
  text-rendering, one-way-door, additive
- **Relates to:** [[0017-draw2d-module]] (this ADR amends (e)'s `draw2d` dependency-profile row,
  same amendment pattern this ADR itself follows), [[0015-framework-2d-atomized-architecture]]
  (the module map/table both [[0016-gamepad-atom]] and [[0017-draw2d-module]] already amended
  textually rather than hand-edited — this ADR follows the identical convention), [[0011-soft-font-flip]]
  (the sovereign C core and the `FontEngine::Own` default this ADR reuses, and the house
  precedent for accepting "no GSUB/GPOS" as the engine's honest ceiling), [[0009-internalization-boundary]]
  (the internalization trajectory this decision aligns with rather than works against),
  `docs/superpowers/plans/2026-07-24-onda6-draw2d-text.md` (the plan whose section 2 this ADR
  records verbatim; sections 3-5 are the normative design that follows FROM this decision),
  `TODO.md` items `D2D-TEXT`, `PROG-1`.

## Context / Contexto

**EN:** `draw_text` is the single largest measured gap at the consumer (GusWorld: 44 uses, the
biggest number in their gap table, `D2D-GAPS`) and, at the same time, the hardest scope decision
the program plan (`D2D-BLOCO`) itself flags: WHERE the text rasterization for Draw2D lives
crosses three things that already exist in this repo and must not collide.

1. **The clean-room own font engine** (`L1.20-FONTFLIP`, default since v0.10.0): the sovereign C
   core `glx_sfnt`/`glx_raster`/`glx_hint`, vendored byte-identical at `glintfx/vendor/core/`
   (sfnt parser with cmap 4/12 + `kern` format 0 + COLR v0, analytic AA rasterizer, pen-snap/vsnap
   hinting) — `glx_sfnt_open` (`glintfx/vendor/core/include/core/sfnt.h:423`), `glx_sfnt_glyph_id`
   (`glintfx/vendor/core/include/core/sfnt.h:442`), `glx_sfnt_hmetrics`
   (`glintfx/vendor/core/include/core/sfnt.h:457`), `glx_sfnt_glyph_outline`
   (`glintfx/vendor/core/include/core/sfnt.h:536`), `glx_sfnt_kern`
   (`glintfx/vendor/core/include/core/sfnt.h:618`), `glx_rasterize_outline`
   (`glintfx/vendor/core/include/core/raster.h:300`), `glx_hint_outline`
   (`glintfx/vendor/core/include/core/hint.h:207`) — plus the hosted C++ shell `FontEngineOwn`
   (`glintfx/src/font_engine_own.hpp:218`) that adapts it to `Rml::FontEngineInterface` (ui-side,
   RmlUi-fused, ~2000 lines in `glintfx/src/font_engine_own.cpp`).
2. **The ui's text:** RmlUi renders HTML/CSS text through either FreeType or the own engine,
   selected at runtime (`FontEngine::Own` default, [[0011-soft-font-flip]]).
3. **The draw2d atom** ([[0017-draw2d-module]]): depends ONLY on `core` + the image seam + the
   internal `glx_`-prefixed GL loader — confirmed still true today (`glintfx_draw2d` links only
   `glloader`, no font engine of any kind, `glintfx/CMakeLists.txt:1264-1274`). It must NOT drag
   `ui` — a sprites-only game must not carry RmlUi.

The leader's ruler (`D2D-BLOCO`, verbatim): *"o que gusworld pede é prioridade, MAS NÃO é pra
fazer APENAS o que ele pede"* — and specifically for text: *"não é o atlas ASCII 32..126 deles —
é o que um framework distribuído precisa (UTF-8, o motor de fonte próprio clean-room que já temos
da Camada 0/L1.20-FONTFLIP, kerning, e a decisão de como conviver com o texto da ui — exige
ADR)"*. The two named risks are over-engineering (building a shaping engine) and under-delivery
(shipping the consumer's ASCII subset verbatim).

**PT:** `draw_text` é a maior lacuna medida no consumidor (GusWorld: 44 usos, o maior número da
tabela de lacunas deles, `D2D-GAPS`) e, ao mesmo tempo, a decisão de escopo mais difícil que o
próprio plano do programa (`D2D-BLOCO`) marca: ONDE a rasterização de texto do Draw2D mora cruza
três coisas que já existem neste repo e não podem colidir.

1. **O motor de fonte próprio clean-room** (`L1.20-FONTFLIP`, default desde a v0.10.0): o núcleo C
   soberano `glx_sfnt`/`glx_raster`/`glx_hint`, vendorizado byte-idêntico em
   `glintfx/vendor/core/` (parser sfnt com cmap 4/12 + `kern` formato 0 + COLR v0, rasterizador
   analítico AA, hinting pen-snap/vsnap) — `glx_sfnt_open`, `glx_sfnt_glyph_id`,
   `glx_sfnt_hmetrics`, `glx_sfnt_glyph_outline`, `glx_sfnt_kern`, `glx_rasterize_outline`,
   `glx_hint_outline` (linhas citadas acima) — mais a casca hosted C++ `FontEngineOwn`
   (`glintfx/src/font_engine_own.hpp:218`) que a adapta pro `Rml::FontEngineInterface` (lado ui,
   fundida ao RmlUi, ~2000 linhas em `glintfx/src/font_engine_own.cpp`).
2. **O texto da ui:** o RmlUi renderiza texto HTML/CSS via FreeType ou o motor próprio, escolhido
   em runtime (`FontEngine::Own` default, [[0011-soft-font-flip]]).
3. **O átomo draw2d** ([[0017-draw2d-module]]): depende SÓ de `core` + a costura image + o loader
   GL interno prefixado `glx_` — confirmado ainda verdadeiro hoje (`glintfx_draw2d` linka só
   `glloader`, nenhum motor de fonte de espécie nenhuma, `glintfx/CMakeLists.txt:1264-1274`). NÃO
   pode arrastar a `ui` — um jogo só-sprites não pode carregar RmlUi.

A régua do líder (`D2D-BLOCO`, verbatim): *"o que gusworld pede é prioridade, MAS NÃO é pra fazer
APENAS o que ele pede"* — e, especificamente pro texto: *"não é o atlas ASCII 32..126 deles — é o
que um framework distribuído precisa (UTF-8, o motor de fonte próprio clean-room que já temos da
Camada 0/L1.20-FONTFLIP, kerning, e a decisão de como conviver com o texto da ui — exige ADR)"*.
Os dois riscos nomeados são over-engineering (virar motor de shaping) e subentrega (entregar só o
subset ASCII do consumidor, verbatim).

## Decision / Decisão

**EN:**

**(a) Q-A — the glyph source is the sovereign C core, directly; NOT a reuse of `FontEngineOwn`'s
shell.** Draw2D's text machinery is a NEW, thin internal seam (`src/text_raster.hpp/.cpp`, the
`image_decode.hpp` pattern) built DIRECTLY on the already-vendored core:
`glx_sfnt_open`/`glyph_id`/`hmetrics`/`glyph_outline`/`kern`, `glx_rasterize_outline`,
`glx_hint_outline`. The hosted shell (UTF-8 decode, glyph atlas, shelf-pack, lazy bake, `Font2d`
registry) is written FRESH for Draw2D — it does not reuse `FontEngineOwn`'s shell, which is fused
to `Rml::FontEngineInterface` and cannot be carved out without touching the production ui font
path (the same "the adaptation layer would be most of the work anyway" reasoning
[[0017-draw2d-module]] axis 2 used to reject `RenderInterface_GL3` reuse). Rejected: (b) a
Draw2D-own atlas baked through FreeType (the path GusWorld's `font_atlas.hpp` runs today) — see
Options considered; (c) routing Draw2D text through RmlUi's text path — rejected on sight, listed
for completeness only.

**(b) Q-B — the v0.23.0 scope line, EXPANDED by the leader.** The CTO's cut (items 1-8) plus two
items the leader moved from seed to IN (items 9-10):

1. Full UTF-8 input (invalid/overlong/truncated sequence → U+FFFD, never a crash, never silent
   truncation); any codepoint the loaded face's cmap (format 4/12) covers renders; an absent
   codepoint renders `.notdef` (tofu) — visible, honest.
2. Kerning via the `kern` table (`glx_sfnt_kern`), applied identically in `draw_text` and
   `measure_text` (single source).
3. Multi-line `\n` with the face's own line-height metric (ascent − descent + line-gap).
4. Metrics: `measure_text` returning width/height + ascent/line-height.
5. LTR only, no BiDi, no complex shaping — DECLARED limit (the core has no GSUB/GPOS).
6. Active-space rule: text draws in whichever space is active (world under a camera, screen px
   otherwise), scaling with `cam.zoom` by projection alone, like every primitive.
7. Small-body sharpness in screen space: pen-snap X + vsnap Y (`glx_hint_outline`) applied in the
   no-camera, integer-size path.
8. Fail-high, central (full hostile matrix in the plan's section 3, TX6).
9. **Word-wrap (leader-added):** greedy line breaking against a caller-supplied `max_width` in
   active-space units; breaks at spaces (U+0020, U+00A0 non-breaking honoured); a single word
   wider than `max_width` force-breaks at the last glyph that fits (fail-safe: text never
   overflows past that one mandatory glyph, never vanishes); explicit `\n` always honoured; no
   hyphenation; `max_width <= 0` is the documented "no wrap" sentinel (the `RectF{}` idiom).
10. **Alignment (leader-added):** `Left`/`Center`/`Right`/`Justify`, applied per line within the
    reference width (`max_width` when wrapping, else the block's widest line). `Justify` is
    word-spacing only (the extra space of a line distributed equally across its space gaps,
    deterministic remainder); the block's last line and any line ended by explicit `\n` are NOT
    justified (fall back to `Left`, typographic convention); `Justify` with `max_width <= 0`
    degrades to `Left`.

**OUT (decided), each a named INBOX seed:** multi-face fallback (`SEED-D2D-TEXT-FALLBACK`), COLR
v0 color emoji in draw2d (`SEED-D2D-TEXT-COLR`), hyphenation + letter-spacing justify + rich text
(`SEED-D2D-TEXT-TYPESETTING`), RTL/BiDi/complex shaping (`SEED-D2D-TEXT-SHAPING`), GPOS kerning
(`SEED-CORE-GPOS-KERN` — a CORE seed: if it ever lands in the sovereign C, both `ui` and `draw2d`
inherit it for free, which is (a)'s bet paying out), system-font discovery (explicit path only,
same as the ui), a `SpriteTransform` overload for text (`SEED-D2D-TEXT-TRANSFORM`). This line is
strictly more than the consumer's 32..126 ASCII subset (renders as a trivial special case of it)
and strictly less than a shaping engine.

**(c) Q-C — the position anchor of `draw_text` is TOP-LEFT, not baseline.** Consistent with `dst`
of every sprite/primitive (y-down, top-left, [[0017-draw2d-module]] Addendum D5's
one-coordinate-story) and what 2D-framework consumers universally expect; `measure_text` exposes
`ascent` so baseline-precise consumers convert with one addition. Baseline-origin (the
typographic answer) is rejected because it forces the simple case ("put text at x,y") to require
metrics knowledge on day one.

**(d) The `draw2d` dependency-profile row ([[0017-draw2d-module]] (e)) is amended, not
superseded** — the same amendment pattern [[0016-gamepad-atom]] (c) used for the `input` row and
[[0017-draw2d-module]] (e) itself used for [[0015-framework-2d-atomized-architecture]] (b). The
row's "depends on" is refined to read: **`core` + `image` (via the D2D-1A seam) + the internal
gl-loader + the vendored sovereign font core (`glx_sfnt`/`glx_raster`/`glx_hint`, read-only
shared with the ui's own font engine, [[0011-soft-font-flip]])**. The row also gains, in
"exposes": `Font2d`/`draw_text`/`measure_text`. "Real independence" stays high (zero `ui`, zero
`fx`, zero `window`) but the dependency SURFACE widens by one vendored C library — stated loudly,
not slipped in, per this ADR's own consequences section below. Every other clause of
[[0017-draw2d-module]] (axes 1-3, the GL pipeline, the composition-order decision) is untouched.

**(e) CMake gate decoupling (mechanical, follows FROM (a)+(d)).** Today the vendored
`vendor/core/{sfnt,raster,hint}.c` objects compile ONLY under `GLINTFX_OWN_FONT_ENGINE`
(`glintfx/CMakeLists.txt:688` sets the vendored paths; the `glintfx_text` object library at
`glintfx/CMakeLists.txt:1051-1056` is the sole consumer). This ADR requires the gate to widen to
`GLINTFX_OWN_FONT_ENGINE OR GLINTFX_MODULE_DRAW2D` so the core objects compile for Draw2D too
when the ui's own font engine is OFF. `GLINTFX_OWN_FONT_ENGINE` itself remains STRICTLY the
ui-side engine selector (`FontEngine::Own` vs. FreeType, [[0011-soft-font-flip]]) — untouched,
not repurposed. No new `GLINTFX_MODULE_*` sub-flag for text-in-draw2d is introduced: `draw_text`
ships as part of the `draw2d` atom itself (a sprites-only consumer accepts ~3 extra C
translation units of compile as the accepted cost of (d), not flag proliferation).

**(f) UI coexistence: the core is shared, nothing else is.** Draw2D's text and the ui's text
share the C core and NOTHING else — separate hosted shells (`text_raster`/`text_layout` vs.
`FontEngineOwn`), separate glyph atlases, separate GL textures, zero shared mutable state (the
core is stateless by design — no global/static mutable). An RmlUi document reload cannot
invalidate a Draw2D `Font2d`, and `Draw2d::shutdown()` cannot touch the ui's loaded faces. This
is the collision-avoidance answer `D2D-BLOCO` required before any code.

**PT:**

**(a) Q-A — a fonte do glifo é o núcleo C soberano, direto; NÃO reuso da casca do
`FontEngineOwn`.** A maquinaria de texto do Draw2D é uma costura interna NOVA e fina
(`src/text_raster.hpp/.cpp`, padrão `image_decode.hpp`) construída DIRETO sobre o núcleo já
vendorizado: `glx_sfnt_open`/`glyph_id`/`hmetrics`/`glyph_outline`/`kern`,
`glx_rasterize_outline`, `glx_hint_outline`. A casca hosted (decode UTF-8, atlas de glifo,
shelf-pack, lazy bake, registry `Font2d`) é escrita NOVA pro Draw2D — não reusa a casca do
`FontEngineOwn`, fundida ao `Rml::FontEngineInterface` e não carvável sem mexer no caminho de
fonte da ui em produção (mesmo raciocínio "a camada de adaptação seria a maior parte do trabalho
de qualquer jeito" que o eixo 2 do [[0017-draw2d-module]] usou pra rejeitar o reuso do
`RenderInterface_GL3`). Rejeitadas: (b) atlas próprio do Draw2D assado via FreeType (o caminho
que o `font_atlas.hpp` do GusWorld roda hoje) — ver Opções consideradas; (c) rotear o texto do
Draw2D pelo caminho de texto do RmlUi — rejeitada de ofício, listada só por completude.

**(b) Q-B — a linha de escopo da v0.23.0, EXPANDIDA pelo líder.** O corte do CTO (itens 1-8) mais
dois itens que o líder moveu de semente pra IN (itens 9-10): (1) entrada UTF-8 completa (sequência
inválida → U+FFFD, nunca crash, nunca truncamento silencioso; qualquer codepoint coberto pelo
cmap 4/12 da face renderiza; ausente → `.notdef`/tofu, visível e honesto); (2) kerning via tabela
`kern` (`glx_sfnt_kern`), idêntico em `draw_text` e `measure_text`, fonte única; (3) multi-linha
`\n` com line-height da própria face; (4) métricas `measure_text` (largura/altura +
ascent/line-height); (5) SÓ LTR, sem BiDi, sem shaping complexo — limite DECLARADO (o core não
tem GSUB/GPOS); (6) regra do espaço ativo (mundo sob câmera, px de tela sem; escala com
`cam.zoom` só por projeção, como toda primitiva); (7) nitidez de corpo pequeno em espaço de tela
(pen-snap/vsnap via `glx_hint_outline`, caminho sem câmera em tamanho inteiro); (8) fail-high
central (matriz hostil completa na seção 3 do plano, TX6); (9) **word-wrap (adição do líder):**
quebra gulosa contra `max_width` em unidades do espaço ativo; quebra em espaços (U+0020, U+00A0
não-quebrável honrado); palavra maior que `max_width` quebra forçada no último glifo que cabe
(fail-safe: nunca estoura além desse um glifo obrigatório, nunca some); `\n` explícito sempre
respeitado; sem hifenização; `max_width <= 0` = sentinela documentada de "sem wrap" (idioma
`RectF{}`); (10) **alinhamento (adição do líder):** `Left`/`Center`/`Right`/`Justify` por linha
dentro da largura de referência (`max_width` com wrap, senão a linha mais larga do bloco);
`Justify` é só word-spacing (espaço extra distribuído igualmente entre os gaps, resto
determinístico); última linha do bloco e linha terminada por `\n` explícito NÃO justificam (caem
pra `Left`, convenção tipográfica); `Justify` com `max_width <= 0` degrada pra `Left`.
**OUT (decidido), cada um com semente nomeada:** fallback multi-face
(`SEED-D2D-TEXT-FALLBACK`), emoji COLR v0 (`SEED-D2D-TEXT-COLR`), hifenização + justify por
letter-spacing + rich text (`SEED-D2D-TEXT-TYPESETTING`), RTL/BiDi/shaping complexo
(`SEED-D2D-TEXT-SHAPING`), kerning GPOS (`SEED-CORE-GPOS-KERN` — semente do CORE: se um dia
entrar no C soberano, `ui` e `draw2d` herdam de graça, que é a aposta da (a) pagando),
descoberta de fonte do sistema (só caminho explícito, como a ui), overload de `SpriteTransform`
pra texto (`SEED-D2D-TEXT-TRANSFORM`). Esta linha é estritamente mais que o subset ASCII 32..126
do consumidor (renderiza como caso especial trivial dela) e estritamente menos que um motor de
shaping.

**(c) Q-C — a âncora de posição do `draw_text` é TOPO-ESQUERDO, não baseline.** Consistente com o
`dst` de todo sprite/primitiva (y-down, topo-esquerdo, a história única de coordenadas do D5 do
Adendo do [[0017-draw2d-module]]) e o que consumidor de framework 2D espera universalmente;
`measure_text` expõe `ascent` pra quem precisa de baseline converter com uma soma. Baseline (a
resposta tipográfica) rejeitada porque obriga o caso simples ("põe texto em x,y") a exigir
conhecimento de métrica no dia um.

**(d) A linha de perfil de dependência do `draw2d` ([[0017-draw2d-module]] (e)) é emendada, não
substituída** — o mesmo padrão de emenda que o [[0016-gamepad-atom]] (c) usou pra linha `input` e
que o próprio [[0017-draw2d-module]] (e) usou pro [[0015-framework-2d-atomized-architecture]] (b).
O "depende de" da linha é refinado pra ler: **`core` + `image` (via a costura D2D-1A) + o
gl-loader interno + o núcleo C soberano de fonte vendorizado (`glx_sfnt`/`glx_raster`/`glx_hint`,
compartilhado só-leitura com o motor de fonte próprio da ui, [[0011-soft-font-flip]])**. A linha
também ganha, em "expõe": `Font2d`/`draw_text`/`measure_text`. A "independência real" segue alta
(zero `ui`, zero `fx`, zero `window`) mas a SUPERFÍCIE de dependência alarga em uma lib C
vendorizada — dita em voz alta, não escorregada pra dentro, conforme a seção de consequências
deste ADR abaixo. Toda outra cláusula do [[0017-draw2d-module]] (eixos 1-3, o pipeline GL, a
decisão de ordem de composição) fica intocada.

**(e) Desacoplamento do gate CMake (mecânico, decorre de (a)+(d)).** Hoje os objetos vendorizados
`vendor/core/{sfnt,raster,hint}.c` compilam SÓ sob `GLINTFX_OWN_FONT_ENGINE`
(`glintfx/CMakeLists.txt:688` seta os paths vendorizados; a biblioteca-objeto `glintfx_text` em
`glintfx/CMakeLists.txt:1051-1056` é o único consumidor). Este ADR exige alargar o gate pra
`GLINTFX_OWN_FONT_ENGINE OR GLINTFX_MODULE_DRAW2D`, pra que os objetos do core compilem pro
Draw2D também quando o motor próprio da ui estiver OFF. O `GLINTFX_OWN_FONT_ENGINE` em si segue
sendo ESTRITAMENTE o seletor de motor do lado ui (`FontEngine::Own` vs. FreeType,
[[0011-soft-font-flip]]) — intocado, não redestinado. Nenhuma sub-flag `GLINTFX_MODULE_*` nova
pra texto-no-draw2d é introduzida: o `draw_text` sai como parte do próprio átomo `draw2d` (um
consumidor só-sprites aceita ~3 unidades de tradução C a mais de compile como custo aceito da
(d), não proliferação de flag).

**(f) Convivência com a ui: o core é compartilhado, nada mais é.** O texto do Draw2D e o texto da
ui compartilham o núcleo C e NADA mais — cascas hosted separadas (`text_raster`/`text_layout`
contra `FontEngineOwn`), atlases de glifo separados, texturas GL separadas, zero estado mutável
compartilhado (o core é stateless por design — sem global/static mutável). Um reload de documento
no RmlUi não pode invalidar um `Font2d` do Draw2D, e `Draw2d::shutdown()` não pode tocar as faces
carregadas da ui. Esta é a resposta de evitar-colisão que o `D2D-BLOCO` exigiu antes de qualquer
código.

## Consequences / Consequências

**EN:**

**Positive:**
- ONE source of truth for rasterization math in the whole library: with the default
  `FontEngine::Own`, ui text and game text render through the literally same rasterizer,
  visually consistent on the same screen.
- ZERO new dependency edge: the core is already in-tree, pure C, no libc beyond hosted-compile
  basics, and already compiles in the MSVC job (own engine default ON) — the leader's decision
  costs the project no new supply-chain surface.
- The hostile-font surface (malformed/truncated/adversarial TTF) is exactly what the clean-room
  parser was built and reviewed for — Draw2D inherits a proven wall for free.
- Kerning, COLR v0 (future emoji), and the hinting work come along at zero marginal cost.
- Aligns with the internalization trajectory ([[0009-internalization-boundary]],
  [[0011-soft-font-flip]]) instead of anchoring against it — unlike option (b), this decision
  never becomes the thing that blocks ADR-0011's explicitly-left-open "full flip" (dropping
  FreeType entirely) from someday being proposed.
- Word-wrap and alignment (the leader's scope expansion) parallelize cleanly against the raster
  seam on disjoint files (pure positioning math over the metrics interface), so the wider scope
  does not lengthen the wave's critical path.

**Negative / accepted as cost:**
- Draw2D's text quality ceiling IS the core's ceiling: no GSUB/GPOS — no complex shaping, no
  ligatures, and no kerning for fonts that ship kerning ONLY as GPOS (a real subset of modern
  fonts). The ui's own engine has lived with this since v0.10.0 ([[0011-soft-font-flip]]), so the
  bar is already accepted house-wide; this decision extends it to game text too.
- Real shell work is duplicated relative to `FontEngineOwn` (atlas/bake orchestration, medium
  cost) — the heavy math (parsing, rasterization, hinting) is NOT duplicated, only the hosted
  orchestration around it.
- The `draw2d` atom's declared dependency profile gains "+ vendored font core" — a
  distribution-visible change to [[0017-draw2d-module]] (e)'s table row, amended by decision (d)
  above, not silently absorbed.
- A sprites-only consumer that never calls `draw_text` still pays the compile cost of ~3 extra C
  translation units (decision (e)) — accepted, not flag-proliferated away.

**Risks / points of attention:**
- Justify (the leader-added alignment mode) is the thorniest of the four alignment modes named in
  the plan's cost note: variable word spacing interacts with the advance/kern loop, and its tests
  only count if the distributed-space arithmetic is hand-computed. Pre-registered contingency (not
  a scope surprise): if mid-wave evidence shows justify alone endangering v0.23.0, the proposal to
  the leader is cutting ONLY justify to `SEED-D2D-TEXT-TYPESETTING`, keeping
  left/center/right.
- The CMake decoupling in decision (e) is unimplemented as of this ADR (verified: today's gate is
  still `GLINTFX_OWN_FONT_ENGINE` alone, `glintfx/CMakeLists.txt:1051-1056`) — a real,
  sequenced prerequisite for the module itself, not a free byproduct, carrying its own regression
  risk against the existing `GLINTFX_OWN_FONT_ENGINE`-only build (mitigated by the existing font
  test suite acting as the guard, same convention [[0017-draw2d-module]] (d) used for the
  image-decode carve).
- Two atlas/shell implementations (ui's `FontEngineOwn` and Draw2D's new `text_raster`/
  `text_layout`) both consuming the same core is a real duplication surface for future bugs (a
  fix to one shell's atlas-eviction or bake policy does not automatically apply to the other) —
  named here as an accepted cost, not a solved problem; decision (f)'s "share the core and
  nothing else" is the explicit boundary that keeps the duplication legible rather than a hidden
  coupling.

**PT:**

**Positivas:**
- UMA fonte de verdade de matemática de rasterização na biblioteca inteira: com `FontEngine::Own`
  default, texto da ui e texto de jogo saem do MESMO rasterizador, literalmente, consistentes na
  mesma tela.
- ZERO aresta de dependência nova: o core já está in-tree, C puro, sem libc além do básico de
  compile hosted, e já compila no job MSVC (motor próprio default ON) — a decisão do líder não
  custa ao projeto superfície de supply-chain nova nenhuma.
- A superfície de fonte hostil (TTF malformada/truncada/adversarial) é exatamente pra que o
  parser clean-room foi construído e revisado — o Draw2D herda uma muralha provada de graça.
- Kerning, COLR v0 (emoji futuro) e o trabalho de hinting vêm de graça marginal.
- Alinha com a trilha de internalização ([[0009-internalization-boundary]],
  [[0011-soft-font-flip]]) em vez de ancorar contra ela — diferente da opção (b), esta decisão
  nunca vira o que impede o "full flip" (tirar o FreeType de vez) deixado explicitamente em
  aberto pelo ADR-0011 de um dia ser proposto.
- Word-wrap e alinhamento (a expansão de escopo do líder) paralelizam limpo contra a costura de
  raster em arquivos disjuntos (matemática pura de posicionamento sobre a interface de métrica),
  então o escopo mais largo não estica o caminho crítico da onda.

**Negativas / aceitas como custo:**
- O teto de qualidade do texto do Draw2D É o teto do core: sem GSUB/GPOS — sem shaping complexo,
  sem ligaduras, sem kerning de fontes que só trazem kerning em GPOS (subset real de fontes
  modernas). O motor próprio da ui já vive com isso desde a v0.10.0 ([[0011-soft-font-flip]]),
  barra já aceita na casa inteira; esta decisão estende isso pro texto de jogo também.
- Trabalho real de casca é duplicado em relação ao `FontEngineOwn` (orquestração de atlas/bake,
  custo médio) — a matemática pesada (parsing, rasterização, hinting) NÃO é duplicada, só a
  orquestração hosted em volta dela.
- O perfil de dependência declarado do átomo `draw2d` ganha "+ núcleo de fonte vendorizado" —
  mudança visível-pra-distribuição na linha da tabela do [[0017-draw2d-module]] (e), emendada
  pela decisão (d) acima, não absorvida em silêncio.
- Um consumidor só-sprites que nunca chama `draw_text` ainda paga o custo de compile de ~3
  unidades de tradução C a mais (decisão (e)) — aceito, não afastado por proliferação de flag.

**Riscos / pontos de atenção:**
- O justify (o modo de alinhamento adicionado pelo líder) é o mais espinhoso dos quatro modos
  nomeados na nota de custo do plano: word spacing variável interage com o laço de avanço/kern, e
  os testes dele só contam se a aritmética de distribuição de espaço for computada à mão.
  Contingência pré-registrada (não surpresa de escopo): se evidência no meio da onda mostrar o
  justify sozinho ameaçando a v0.23.0, a proposta ao líder é cortar SÓ o justify pra
  `SEED-D2D-TEXT-TYPESETTING`, mantendo left/center/right.
- O desacoplamento de CMake da decisão (e) segue não-implementado no momento deste ADR
  (verificado: o gate de hoje segue só `GLINTFX_OWN_FONT_ENGINE`, `glintfx/CMakeLists.txt:1051-1056`)
  — um pré-requisito real, sequenciado, pro módulo em si, não um subproduto de graça, carregando
  risco de regressão próprio contra o build hoje só-`GLINTFX_OWN_FONT_ENGINE` (mitigado pela
  suíte de testes de fonte existente agindo como guarda, mesma convenção que a (d) do
  [[0017-draw2d-module]] usou pro carve de decode de imagem).
- Duas implementações de casca/atlas (a `FontEngineOwn` da ui e o `text_raster`/`text_layout`
  novo do Draw2D) consumindo o mesmo core é superfície real de duplicação pra bug futuro (um fix
  na política de eviction/bake de atlas de uma casca não se aplica automaticamente à outra) —
  nomeado aqui como custo aceito, não problema resolvido; o "compartilha o core e nada mais" da
  decisão (f) é a fronteira explícita que mantém a duplicação legível em vez de acoplamento
  escondido.

## Options considered / Opções consideradas

**EN:**

*Q-A — where the glyph comes from:*
1. **The sovereign C core, direct (chosen).** See decision (a) above. Pros: one rasterization
   truth, zero new dependency, hostile-input wall already reviewed, aligned with internalization.
   Cons: the core's ceiling (no GSUB/GPOS) becomes the game-text ceiling too; shell work
   duplicated relative to `FontEngineOwn`; widens the `draw2d` dependency profile.
2. **Own atlas over FreeType (the GusWorld path).** Rejected: creates a NEW third-party
   dependency edge in an atom that today has none beyond GL — FreeType currently enters only via
   `ui`/RmlUi; a future draw2d-without-ui composition would link FreeType solely for game text.
   Two rasterization truths on one screen (ui on the own engine by default, game text on
   FreeType — subtly different rendering of the same font). Works AGAINST the internalization
   trajectory: when ADR-0011's "full flip" is someday proposed, Draw2D becomes the anchor
   blocking it. The WORSE one-way-door of the two: an added dependency edge is far harder to
   remove later than an internal backend swap.
3. **Reuse RmlUi's text path.** Rejected on sight: drags `Rml::Initialise` and the whole `ui`
   stack into the atom, violating [[0017-draw2d-module]] (a) and
   [[0015-framework-2d-atomized-architecture]]'s boundary. Listed for completeness only.

*Q-C — position anchor:*
1. **Top-left (chosen).** See decision (c) above.
2. **Baseline origin.** Rejected: the typographic answer, but forces the simple case ("put text
   at x,y") to require metrics knowledge on day one.

**PT:**

*Q-A — de onde vem o glifo:*
1. **O núcleo C soberano, direto (escolhida).** Ver decisão (a) acima. Prós: uma verdade de
   rasterização só, zero dependência nova, muralha de input hostil já revisada, alinhada com a
   internalização. Contras: o teto do core (sem GSUB/GPOS) vira o teto do texto de jogo também;
   trabalho de casca duplicado em relação ao `FontEngineOwn`; alarga o perfil de dependência do
   `draw2d`.
2. **Atlas próprio sobre FreeType (o caminho do GusWorld).** Rejeitada: cria aresta de
   dependência de TERCEIRO nova num átomo que hoje não tem nenhuma além de GL — o FreeType hoje
   só entra via `ui`/RmlUi; uma composição futura draw2d-sem-ui passaria a linkar FreeType só pro
   texto de jogo. Duas verdades de rasterização na mesma tela (ui no motor próprio por default,
   jogo no FreeType — renderização sutilmente diferente da mesma fonte). Anda CONTRA a
   internalização: quando o "full flip" do ADR-0011 for proposto um dia, o Draw2D vira a âncora
   que o impede. A one-way-door PIOR das duas: aresta de dependência adicionada é muito mais
   difícil de remover depois que troca interna de backend.
3. **Reusar o caminho de texto do RmlUi.** Rejeitada de ofício: arrasta `Rml::Initialise` e a
   pilha `ui` inteira pro átomo, violando o [[0017-draw2d-module]] (a) e a fronteira do
   [[0015-framework-2d-atomized-architecture]]. Listada só por completude.

*Q-C — âncora de posição:*
1. **Topo-esquerdo (escolhida).** Ver decisão (c) acima.
2. **Origem na baseline.** Rejeitada: é a resposta tipográfica, mas obriga o caso simples ("põe
   texto em x,y") a exigir conhecimento de métrica no dia um.

## Reversibility / Reversibilidade

**EN:** The public API (`Font2d`/`draw_text`/`measure_text`) leaks neither the core nor any
third-party type, so the backend is technically swappable behind the pImpl — two-way in API
terms, same shape [[0016-gamepad-atom]] and [[0017-draw2d-module]] already established for their
own placement/pipeline choices. BUT in practice Q-A is the ONE-WAY-DOOR of this ADR: (1) the
atom's dependency profile is something consumers pin on, the exact caveat
[[0016-gamepad-atom]]/[[0017-draw2d-module]]'s reversibility sections already name; (2) pixel
output becomes observed contract — a backend swap is a visible rendering change, the same reason
v0.10.0 shipped MINOR ([[0011-soft-font-flip]]); (3) it commits the sovereign core as a
load-bearing production path for game text for years, deepening the internalization bet
([[0009-internalization-boundary]]) rather than merely trying it. This is why Q-A went to the
leader rather than being decided by the CTO alone. Q-C (top-left anchor) is comparatively cheap
to reverse in isolation (an API-shape change, not a dependency commitment) but was batched into
the same leader decision because it is API contract consumers lay out against, per the house rule
that design decisions come as options, not silent defaults. Q-B (the scope line) is the cheapest
of the three: every OUT item is a named seed, addable later without reworking what ships in
v0.23.0.

**PT:** A API pública (`Font2d`/`draw_text`/`measure_text`) não vaza nem o core nem nenhum tipo
de terceiro, então o backend é tecnicamente trocável atrás do pImpl — two-way em termos de API, a
mesma forma que [[0016-gamepad-atom]] e [[0017-draw2d-module]] já estabeleceram pras próprias
escolhas de posição/pipeline. MAS na prática a Q-A é a ONE-WAY-DOOR deste ADR: (1) o perfil de
dependência do átomo é coisa que consumidor pina, a ressalva exata que as seções de
reversibilidade do [[0016-gamepad-atom]]/[[0017-draw2d-module]] já nomeiam; (2) a saída de pixel
vira contrato observado — trocar de backend é mudança visível de renderização, o mesmo motivo
pelo qual a v0.10.0 saiu MINOR ([[0011-soft-font-flip]]); (3) compromete o core soberano como
caminho de produção do texto de jogo por anos, aprofundando a aposta de internalização
([[0009-internalization-boundary]]) em vez de só experimentá-la. É por isso que a Q-A foi ao
líder em vez de decidida só pelo CTO. A Q-C (âncora topo-esquerdo) é comparativamente barata de
reverter isolada (mudança de forma de API, não compromisso de dependência), mas entrou no mesmo
lote de decisão do líder porque é contrato de layout que consumidor usa — a regra da casa de que
decisão de design vem como opções, não default silencioso. A Q-B (a linha de escopo) é a mais
barata das três: todo item OUT é semente nomeada, adicionável depois sem retrabalhar o que sai na
v0.23.0.
