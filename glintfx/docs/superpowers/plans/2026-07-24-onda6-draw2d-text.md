# Onda 6 -- D2D-TEXT: `draw_text` in the Draw2D atom / `draw_text` no átomo Draw2D (alvo v0.23.0)

- **Author / Autor:** Caetano (CTO). Planning only -- the orchestrator does the fan-out.
- **Status:** DECIDED BY THE LEADER (petrus, 2026-07-24) -- **Q-A = Option A** (sovereign C
  core), **Q-C = top-left**, **Q-B = EXPANDED beyond the CTO's proposed line**: word-wrap and
  alignment (left/center/right/justify) are IN for v0.23.0, by leader decision (scope is his
  call; the CTO's original cut had them as seeds). Section 2 stays as the option record for
  ADR-0018; sections 3-5 are updated to the decided, expanded scope. / DECIDIDO PELO LÍDER
  (petrus, 2026-07-24) -- **Q-A = Opção A** (núcleo C soberano), **Q-C = topo-esquerdo**,
  **Q-B = EXPANDIDA além da linha proposta pelo CTO**: word-wrap e alinhamento
  (left/center/right/justify) ENTRAM na v0.23.0, por decisão do líder (escopo é dele; o corte
  original do CTO os tinha como sementes). A seção 2 fica como registro de opções pro ADR-0018;
  as seções 3-5 estão atualizadas pro escopo decidido e expandido.
- **Inputs read:** `TODO.md` (`D2D-BLOCO`, `D2D-GAPS`), ADR-0011 (soft font flip), ADR-0015
  (atoms), ADR-0017 + Addendum (Draw2D), `glintfx/include/glintfx/draw2d.hpp` (the real public
  surface as of v0.22.0), `glintfx/src/font_engine_own.hpp` (the ui-side own-engine shell),
  `glintfx/vendor/core/include/core/{sfnt,raster,hint}.h` (the sovereign C font core),
  `glintfx/CMakeLists.txt` (module flags, vendored-core wiring), the program plan
  (`2026-07-22-programa-completo.md`).

---

## 1. Why this wave is the heaviest slice / Por que esta onda é a fatia mais pesada

**EN:** `draw_text` is the single largest measured gap at the consumer (GusWorld: 44 uses, the
biggest number in their table, `D2D-GAPS`), and simultaneously the scope decision `D2D-BLOCO`
itself flags as the hardest: WHERE the text rasterization for Draw2D lives crosses three things
that already exist in this repo and must not collide:

1. **The clean-room own font engine** (`L1.20-FONTFLIP`, default since v0.10.0): the sovereign C
   core `glx_sfnt`/`glx_raster`/`glx_hint`, vendored byte-identical at `glintfx/vendor/core/`
   (sfnt parser with cmap 4/12 + `kern` format 0 + COLR v0, analytic AA rasterizer, pen-snap/vsnap
   hinting), plus the hosted C++ shell `FontEngineOwn` that adapts it to
   `Rml::FontEngineInterface` (ui-side, RmlUi-fused, 2k lines).
2. **The ui's text**: RmlUi renders HTML/CSS text through either FreeType or the own engine,
   selected at runtime (`FontEngine::Own` default, ADR-0011).
3. **The draw2d atom** (ADR-0017): depends ONLY on `core` + the image seam + the internal
   `glx_` GL loader. It must NOT drag `ui` -- a sprites+text game must not carry RmlUi.

The leader's ruler (`D2D-BLOCO`, verbatim): *"o que gusworld pede é prioridade, MAS NÃO é pra
fazer APENAS o que ele pede"* -- and, specifically for text: *"não é o atlas ASCII 32..126 deles
-- é o que um framework distribuído precisa (UTF-8, o motor de fonte próprio clean-room que já
temos da Camada 0/L1.20-FONTFLIP, kerning, e a decisão de como conviver com o texto da ui --
exige ADR)"*. The two named risks of this wave are over-engineering (building a shaping engine)
AND under-delivery (shipping their ASCII subset). Section 2 draws the line explicitly.

**PT:** `draw_text` é a maior lacuna medida no consumidor (GusWorld: 44 usos, o maior número da
tabela deles, `D2D-GAPS`) e ao mesmo tempo a decisão de escopo que o próprio `D2D-BLOCO` marca
como a mais difícil: ONDE a rasterização de texto do Draw2D vive cruza três coisas que já existem
no repo e não podem colidir: (1) o motor de fonte próprio clean-room (núcleo C soberano
`glx_sfnt`/`glx_raster`/`glx_hint` vendorizado em `glintfx/vendor/core/`, mais a casca hosted
`FontEngineOwn` fundida ao RmlUi); (2) o texto da `ui` (RmlUi, FreeType ou motor próprio em
runtime, ADR-0011); (3) o átomo `draw2d`, que não pode arrastar a `ui` (ADR-0017). A régua do
líder está citada acima; os dois riscos nomeados são over-engineering (virar motor de shaping) E
subentrega (entregar só o subset ASCII deles). A seção 2 traça a linha.

---

## 2. LEADER DECISION -- the ADR-0018 options / DECISÃO DO LÍDER -- as opções do ADR-0018

**EN:** `D2D-BLOCO` mandates an ADR of its own before any line of code. The next free number is
**ADR-0018** (`docs/adr/0018-draw2d-text.md`), to be recorded by `software-architect`, same
collaborative pattern as ADR-0017. Three questions; Q-A is the heavy one. **All three are now
DECIDED (2026-07-24, see Status above)** -- this section stays as the option record the ADR
transcribes, with each decision marked inline.

**PT:** O `D2D-BLOCO` exige ADR próprio antes de qualquer linha. O próximo número livre é
**ADR-0018** (`docs/adr/0018-draw2d-text.md`), gravado pelo `software-architect`, mesmo padrão
colaborativo do ADR-0017. Três perguntas; a Q-A é a pesada. **As três estão DECIDIDAS
(2026-07-24, ver Status acima)** -- esta seção fica como o registro de opções que o ADR
transcreve, com cada decisão marcada inline.

### Q-A -- Where the glyph comes from / De onde vem o glifo rasterizado -- **DECIDED: Option A / DECIDIDA: Opção A (líder, 2026-07-24)**

#### Option A (RECOMMENDED / RECOMENDADA) -- the sovereign C core, direct / o núcleo C soberano, direto

**EN:** Draw2D's text machinery is a NEW thin internal seam (`src/text_raster.hpp/.cpp`, the
`image_decode.hpp` pattern) built DIRECTLY on the already-vendored sovereign C core:
`glx_sfnt_open` (hostile-input-hardened parser), `glx_sfnt_glyph_id` (cmap 4/12),
`glx_sfnt_hmetrics`, `glx_sfnt_glyph_outline`, `glx_sfnt_kern` (binary search),
`glx_rasterize_outline` (analytic AA), `glx_hint` (pen-snap/vsnap, the small-body sharpness work
GusWorld's 9-13dp HUD already validated). The hosted shell (UTF-8 decode, glyph atlas,
shelf-pack, lazy bake, Font2d registry) is written fresh for Draw2D -- it does NOT reuse
`FontEngineOwn`'s shell, which is fused to `Rml::FontEngineInterface` and cannot be carved out
without touching the production ui font path (same "the adaptation layer would be most of the
work anyway" reasoning ADR-0017 axis 2 used to reject RenderInterface_GL3 reuse). One CMake
change: the vendored `vendor/core/{sfnt,raster,hint}.c` objects compile when
`GLINTFX_OWN_FONT_ENGINE OR GLINTFX_MODULE_DRAW2D` (today they are gated on the former alone,
`CMakeLists.txt` block at the `if(GLINTFX_OWN_FONT_ENGINE)` around line 664) --
`GLINTFX_OWN_FONT_ENGINE` remains strictly the UI-engine selector, untouched.

- **Pros:** ONE source of truth for rasterization math in the whole library (the C core; ui and
  game text share it -- with the default `FontEngine::Own`, both render through literally the
  same rasterizer, visually consistent on the same screen). ZERO new dependency edge: the core is
  already in-tree, pure C, no libc beyond hosted-compile basics, already compiles in the MSVC job
  (own engine is default ON). The hostile-font surface (malformed/truncated/adversarial TTF) is
  exactly what the clean-room parser was built and reviewed for. Kerning, COLR v0 (future emoji),
  and the hinting work come along at zero marginal cost. Aligns with the internalization
  trajectory (ADR-0009/0011) instead of working against it.
- **Cons:** Draw2D's text quality ceiling is the core's ceiling: no GSUB/GPOS (no complex
  shaping, no ligatures, and no kerning for fonts that ship kerning ONLY as GPOS -- a real
  subset of modern fonts; the ui's own engine already lives with this since v0.10.0, so the bar
  is already accepted house-wide, but it becomes true of game text too). Real shell work is
  duplicated relative to `FontEngineOwn` (atlas/bake orchestration, ~medium), though not the
  heavy math. The draw2d atom's declared dependency profile gains "+ vendored core" -- a
  distribution-visible change to ADR-0017 (e)'s table row (amendment needed).
- **Effort:** M/G. **Risk:** medium (new shell, proven core).
- **Reversibility:** the public API (`Font2d`/`draw_text`/`measure_text`) leaks neither the core
  nor any third-party type, so the backend is technically swappable behind the pImpl (two-way in
  API terms). BUT in practice this is the **ONE-WAY-DOOR of the wave**: (1) the atom's dependency
  profile is something consumers pin on (the exact caveat ADR-0016/0017's reversibility sections
  name); (2) pixel output becomes observed contract (backend swap = visible rendering change,
  the same reason v0.10.0 was MINOR); (3) it commits the sovereign core as a load-bearing
  production path for game text for years. **Confirmed: this goes to the leader.**

**PT:** A maquinaria de texto do Draw2D é uma costura interna NOVA e fina
(`src/text_raster.hpp/.cpp`, padrão `image_decode.hpp`) construída DIRETO sobre o núcleo C
soberano já vendorizado (`glx_sfnt_open`/`glyph_id`/`hmetrics`/`glyph_outline`/`kern`/
`glx_rasterize_outline`/`glx_hint`). A casca hosted (decode UTF-8, atlas, shelf-pack, lazy bake,
registry `Font2d`) é escrita nova para o Draw2D -- NÃO reusa a casca do `FontEngineOwn`, fundida
ao `Rml::FontEngineInterface` (carvá-la mexeria no caminho de fonte da ui em produção; mesmo
raciocínio do eixo 2 do ADR-0017). Uma mudança de CMake: os objetos de `vendor/core/*.c`
compilam quando `GLINTFX_OWN_FONT_ENGINE OR GLINTFX_MODULE_DRAW2D` (hoje só sob o primeiro,
bloco `if(GLINTFX_OWN_FONT_ENGINE)` por volta da linha 664); `GLINTFX_OWN_FONT_ENGINE` segue
sendo estritamente o seletor de motor da UI, intocado.

- **Prós:** UMA fonte de verdade de rasterização na biblioteca inteira (com `FontEngine::Own`
  default, texto da ui e do jogo saem do MESMO rasterizador, consistentes na mesma tela); ZERO
  aresta de dependência nova (o core já está in-tree, C puro, já compila no job MSVC); a
  superfície de fonte hostil é exatamente para o que o parser clean-room foi construído e
  revisado; kerning, COLR v0 (emoji futuro) e o hinting de corpo pequeno vêm de graça; alinha
  com a trilha de internalização (ADR-0009/0011) em vez de andar contra.
- **Contras:** o teto de qualidade do texto do Draw2D vira o teto do core: sem GSUB/GPOS (sem
  shaping complexo, sem ligaduras, e sem kerning de fontes que só trazem kerning em GPOS -- a ui
  já vive com isso desde a v0.10.0, barra já aceita na casa, mas passa a valer pro texto de jogo
  também); trabalho real de casca duplicado em relação ao `FontEngineOwn` (orquestração de
  atlas/bake, médio -- não a matemática pesada); o perfil de dependência declarado do átomo
  draw2d ganha "+ core vendorizado" (emenda à tabela do ADR-0017 (e)).
- **Esforço:** M/G. **Risco:** médio.
- **Reversibilidade:** a API pública não vaza o motor -- tecnicamente two-way atrás do pImpl.
  MAS na prática é a **ONE-WAY-DOOR da onda**: perfil de dependência do átomo é coisa que
  consumidor pina (a ressalva das seções de reversibilidade dos ADR-0016/0017); saída de pixel
  vira contrato observado; e compromete o core soberano como caminho de produção do texto de
  jogo por anos. **Confirmado: vai ao líder.**

#### Option B -- own atlas over FreeType (the GusWorld path) / atlas próprio sobre FreeType (o caminho do GusWorld)

**EN:** Draw2D bakes its own glyph atlas through FreeType (what GusWorld's `font_atlas.hpp` does
today, 32..126 grayscale).

- **Pros:** FreeType covers everything (mature hinting, every cmap, GPOS-only-kerning fonts,
  battle-tested robustness); less in-house raster shell; the consumer already trusts this path.
- **Cons:** creates a NEW third-party dependency edge in an atom that today has none beyond GL
  -- FreeType currently enters only via `ui`/RmlUi; a future draw2d-without-ui composition would
  link FreeType solely for game text. Two rasterization truths on one screen (ui text on the own
  engine by default, game text on FreeType -- subtly different rendering of the same font).
  Works AGAINST the internalization trajectory: when ADR-0011's explicitly-left-open "full flip"
  (dropping FreeType entirely) is someday proposed, Draw2D becomes the anchor that blocks it.
- **Effort:** M. **Risk:** low-medium technically, HIGH strategically.
- **Reversibility:** the WORSE one-way-door: an added dependency edge is far harder to remove
  later (consumers build/package against it) than an internal backend swap. Also goes to the
  leader if chosen.

**PT:** O Draw2D assa o próprio atlas via FreeType (o que o `font_atlas.hpp` do GusWorld faz
hoje). **Prós:** FreeType cobre tudo (hinting maduro, todo cmap, fontes kerning-só-GPOS,
robustez batalha-testada); menos casca própria; caminho que o consumidor já confia.
**Contras:** cria aresta de dependência NOVA num átomo que hoje não tem nenhuma além do GL
(FreeType hoje só entra via `ui`/RmlUi; uma composição draw2d-sem-ui passaria a linkar FreeType
só pro texto de jogo); duas verdades de rasterização na mesma tela (ui no motor próprio por
default, jogo no FreeType); anda CONTRA a internalização -- quando o "full flip" deixado aberto
pelo ADR-0011 for proposto, o Draw2D vira a âncora que o impede. **Esforço:** M. **Risco:**
baixo-médio técnico, ALTO estratégico. **Reversibilidade:** a one-way-door PIOR: aresta de
dependência adicionada é muito mais difícil de remover depois que troca interna de backend.

#### Option C -- reuse RmlUi's text path / reusar o caminho de texto do RmlUi

**EN:** Route Draw2D text through RmlUi's font/render machinery. **Listed for completeness,
recommended for rejection on sight:** it drags `Rml::Initialise` and the whole `ui` stack into
the atom, violating ADR-0017 (a) and ADR-0015's boundary ("a sprites-only game must not carry
HTML/CSS"). Every argument ADR-0017 axis 2 used against RenderInterface_GL3 reuse applies
verbatim, plus the module-boundary violation on top.

**PT:** Rotear o texto do Draw2D pela maquinaria de fonte/render do RmlUi. **Listada por
completude, recomendada para rejeição de ofício:** arrasta `Rml::Initialise` e a pilha `ui`
inteira pro átomo, violando ADR-0017 (a) e a fronteira do ADR-0015. Todo argumento do eixo 2 do
ADR-0017 contra reusar o RenderInterface_GL3 vale verbatim, mais a violação de fronteira.

#### CTO recommendation on Q-A / Recomendação do CTO na Q-A

**EN:** **Option A.** It is the only option that is simultaneously: zero new dependency, one
rasterization truth, aligned with the internalization trajectory the leader has been funding
since L1.19, and already MSVC-proven. Its real cost (no GPOS) is a bar the house already
accepted for the ui's default engine. Option B would be choosing, for the atom's permanent
dependency profile, the exact library the own engine exists to replace.

**PT:** **Opção A.** É a única simultaneamente: zero dependência nova, uma verdade de
rasterização só, alinhada com a trilha de internalização que o líder financia desde o L1.19, e
já provada no MSVC. O custo real (sem GPOS) é barra que a casa já aceitou pro motor default da
ui. A Opção B seria escolher, pro perfil de dependência permanente do átomo, exatamente a lib
que o motor próprio existe pra substituir.

### Q-B -- The scope line for v0.23.0 / A linha de escopo da v0.23.0 -- **DECIDED, EXPANDED BY THE LEADER / DECIDIDA, EXPANDIDA PELO LÍDER (2026-07-24)**

**EN:** Applying `D2D-BLOCO`'s axis 2 (general-distribution scope, not the GusWorld subset)
without tipping into shaping-engine territory. The CTO proposed items 1-8 below; **the leader
expanded the scope with items 9-10 (word-wrap and alignment, including justify)** -- originally
marked as seeds by the CTO, now IN by leader decision. **IN for v0.23.0 (decided):**

1. **Full UTF-8 input** -- decode with the standard replacement policy (invalid/overlong/
   truncated sequence -> U+FFFD, never a crash, never silent truncation); any codepoint the
   loaded face's cmap (format 4/12) covers renders; a codepoint the face lacks renders `.notdef`
   (tofu) -- visible, honest, per TrueType's own convention.
2. **Kerning** via the `kern` table (`glx_sfnt_kern`, gid-keyed, binary search) -- applied
   identically in `draw_text` and `measure_text` (single source, mutation-testable).
3. **Multi-line `\n`** with the face's own line-height metric (ascent - descent + line-gap).
   Cheap, natural, and its absence would be an immediate consumer papercut.
4. **Metrics:** `measure_text` returning width/height + ascent/line-height (layout without
   drawing -- HUD anchoring, centering; the consumer's own layout code needs it on day one).
5. **LTR only**, no BiDi, no complex shaping -- DECLARED limit (see OUT list), honest because
   the core has no GSUB/GPOS; pretending otherwise would be the dishonesty ADR-0015 (c) forbids.
6. **Active-space rule** (the house rule since D18): text draws in whichever space is active
   (world units under a camera, screen px otherwise); size scales with `cam.zoom` by projection
   alone, like every primitive. Layer (D27) and scissor (D28) tag text exactly like sprites.
7. **Small-body sharpness in screen space**: pen-snap/vsnap hinting applied when text renders
   unscaled at integer pixel sizes (the `glx_hint` work, reused).
8. **Fail-high, central** (section 4 details the full hostile matrix).
9. **Word-wrap (leader-added):** automatic line breaking against a caller-supplied
   `max_width` in ACTIVE-space units. Breaks at spaces (U+0020); a single word wider than
   `max_width` force-breaks at the last glyph that fits (fail-safe: text never overflows the
   requested width, never disappears); explicit `\n` is always honoured. No hyphenation
   (declared limit, see OUT). `max_width <= 0` is the documented "no wrap" sentinel (the
   `RectF{}` idiom).
10. **Alignment (leader-added):** `left` / `center` / `right` / `justify`, applied per line
    within the reference width (`max_width` when wrapping; otherwise the widest line of the
    block defines the box, so centered multi-line `\n` text works with no wrap). Justify is
    **word-spacing only** (the extra space of each line is distributed equally across its
    space gaps, deterministic remainder rule); per typographic convention, the LAST line of the
    block and any line ended by an explicit `\n` are NOT justified (they fall back to left).
    Justify with `max_width <= 0` degrades to left (documented -- justify needs a target
    width to mean anything).

**OUT (decided), each a named INBOX seed:** multi-face fallback chain
(`SEED-D2D-TEXT-FALLBACK`; the ui has it, game text starts single-face), COLR v0 color emoji in
draw2d (`SEED-D2D-TEXT-COLR`; core supports it, atlas format complicates, no consumer demand
yet), hyphenation + letter-spacing justify + rich text (`SEED-D2D-TEXT-TYPESETTING`; the
typographic-quality tier ABOVE this wave's word-spacing justify -- language-aware hyphenation
is dictionary-class work), RTL/BiDi/complex shaping (`SEED-D2D-TEXT-SHAPING`; requires
GSUB/GPOS-class work, a Layer-0-track decision, not a wave), GPOS kerning
(`SEED-CORE-GPOS-KERN`; a CORE seed -- if it ever lands in the sovereign C, both ui and draw2d
text inherit it for free, which is Option A paying out), system-font discovery (explicit path
only, same as the ui), a `SpriteTransform` overload for text (`SEED-D2D-TEXT-TRANSFORM`).

**CTO cost note on justify, on the record (duty to name, not to re-litigate):** justify is the
thorniest of the four modes -- variable word spacing interacts with the advance/kern loop, and
its tests only count if the distributed-space arithmetic is hand-computed (section 4's gate).
The saving grace: WITHOUT complex shaping (this wave is LTR, no GSUB/GPOS), justify degrades to
pure word-spacing arithmetic over the already-computed wrapped lines -- moderate cost, not
HarfBuzz-class. What this wave's justify deliberately is NOT: letter-spacing distribution or
hyphenation (both in `SEED-D2D-TEXT-TYPESETTING`). Shipped with that limit declared loudly in
`docs/draw2d.md`. If mid-wave evidence shows justify alone endangering the release, the
fallback proposal to the leader is: cut ONLY justify to the seed, keep left/center/right --
pre-registered here so it is a named contingency, not a scope surprise.

This line is strictly more than GusWorld's 32..126 (their subset renders as a trivial special
case) and strictly less than a shaping engine.

**PT:** Aplicando o eixo 2 do `D2D-BLOCO` sem cair em motor de shaping. O CTO propôs os itens
1-8; **o líder expandiu o escopo com os itens 9-10 (word-wrap e alinhamento, inclusive
justify)** -- originalmente sementes no corte do CTO, agora IN por decisão dele. **IN decidido
para a v0.23.0:** (1) entrada UTF-8 completa (sequência inválida -> U+FFFD, nunca crash nem truncamento
silencioso; qualquer codepoint coberto pelo cmap 4/12 da face renderiza; ausente = `.notdef`/
tofu, visível e honesto); (2) kerning via tabela `kern` (idêntico em `draw_text` e
`measure_text`, fonte única, mutation-testável); (3) multi-linha `\n` com line-height da face;
(4) métricas `measure_text` (largura/altura + ascent/line-height -- layout sem desenhar);
(5) SÓ LTR, sem BiDi, sem shaping complexo -- limite DECLARADO (honesto: o core não tem
GSUB/GPOS); (6) regra do espaço ativo (mundo sob câmera, px de tela sem; tamanho escala com
`cam.zoom` só por projeção; layer e scissor marcam texto como sprites); (7) nitidez de corpo
pequeno em espaço de tela (pen-snap/vsnap reusado); (8) fail-high central (seção 4);
(9) **word-wrap (adição do líder):** quebra automática contra `max_width` em unidades do espaço
ativo; quebra em espaços (U+0020); palavra maior que `max_width` quebra forçada no último glifo
que cabe (fail-safe: o texto nunca estoura a largura pedida, nunca some); `\n` explícito sempre
respeitado; sem hifenização (limite declarado); `max_width <= 0` = sentinela documentada de
"sem wrap" (idioma do `RectF{}`); (10) **alinhamento (adição do líder):** `left`/`center`/
`right`/`justify` por linha dentro da largura de referência (`max_width` com wrap; sem wrap, a
linha mais larga do bloco define a caixa -- centralizar multi-linha `\n` funciona sem wrap);
justify é **só word-spacing** (o espaço extra de cada linha distribuído igualmente entre os
gaps de espaço, regra determinística de resto); pela convenção tipográfica, a ÚLTIMA linha do
bloco e linhas terminadas por `\n` explícito NÃO justificam (caem pra left); justify com
`max_width <= 0` degrada pra left (documentado -- justify precisa de largura-alvo pra
significar algo).
**OUT (decidido) com semente nomeada:** fallback multi-face (`SEED-D2D-TEXT-FALLBACK`), emoji
COLR (`SEED-D2D-TEXT-COLR`), hifenização + justify por letter-spacing + rich text
(`SEED-D2D-TEXT-TYPESETTING` -- o tier tipográfico ACIMA do justify word-spacing desta onda;
hifenização consciente de idioma é trabalho classe dicionário), RTL/BiDi/shaping
(`SEED-D2D-TEXT-SHAPING`), kerning GPOS (`SEED-CORE-GPOS-KERN` -- semente do CORE: se um dia
entrar no C soberano, ui e draw2d herdam de graça, que é a Opção A pagando), descoberta de
fonte do sistema (só caminho explícito), overload de `SpriteTransform` pra texto
(`SEED-D2D-TEXT-TRANSFORM`).
**Nota de custo do CTO sobre o justify, em registro (dever de nomear, não de reabrir):**
justify é o mais espinhoso dos quatro modos -- word spacing variável interage com o laço de
avanço/kern, e seus testes só contam se a aritmética de distribuição de espaço for computada à
mão (gate da seção 4). O que salva: SEM shaping complexo (esta onda é LTR, sem GSUB/GPOS),
justify degrada pra aritmética pura de word-spacing sobre as linhas já quebradas -- custo
moderado, não classe HarfBuzz. O que o justify desta onda deliberadamente NÃO é: distribuição
por letter-spacing nem hifenização (ambos em `SEED-D2D-TEXT-TYPESETTING`), limite declarado em
voz alta no `docs/draw2d.md`. Se evidência no meio da onda mostrar o justify sozinho ameaçando
a release, a proposta de contingência ao líder é: cortar SÓ o justify pra semente, mantendo
left/center/right -- pré-registrada aqui pra ser contingência nomeada, não surpresa de escopo.
Esta linha é estritamente mais que o 32..126 do GusWorld e estritamente menos que um motor de
shaping.

### Q-C -- position anchor of `draw_text` / âncora de posição do `draw_text` -- **DECIDED: top-left / DECIDIDA: topo-esquerdo (líder, 2026-07-24)**

**EN:** Is `pos` the TOP-LEFT of the first line's box, or the BASELINE origin?
**Recommendation: top-left** -- consistent with `dst` of every sprite/primitive (y-down,
top-left, D5's one-coordinate-story), what 2D-framework consumers universally expect, and what
GusWorld's atlas path does; `measure_text` exposes ascent so baseline-precise consumers can
convert with one addition. Baseline-origin (the typographic answer) is rejected because it makes
the simple case ("put text at x,y") require metrics knowledge on day one. Two-way-door in
principle, but it is API contract consumers lay out against -- worth 10 seconds of the same
leader batch rather than a silent CTO default (the house rule: design decisions come as
options).

**PT:** `pos` é o TOPO-ESQUERDO da caixa da primeira linha, ou a origem da BASELINE?
**Recomendação: topo-esquerdo** -- consistente com o `dst` de todo sprite/primitiva (y-down,
top-left, a história única de coordenadas do D5), o que consumidor de framework 2D espera, e o
que o caminho de atlas do GusWorld faz; `measure_text` expõe o ascent pra quem precisa de
baseline converter com uma soma. Baseline (a resposta tipográfica) rejeitada porque obriga o
caso simples a conhecer métrica no dia um. Two-way em princípio, mas é contrato de layout do
consumidor -- vale os 10 segundos no mesmo batch do líder em vez de default silencioso do CTO.

---

## 3. Normative design (Q-A = A, decided) / Design normativo (Q-A = A, decidida)

**EN (PT below):** Numbered TX1..TX17 (continuing the house D-series convention with a text
prefix to avoid colliding with D1-D30 from waves 3-5). TX15-TX17 are the leader's scope
expansion (wrap + align).

- **TX1 -- public surface, on `Draw2d` itself** (text participates in bracket/batcher/camera/
  layer/scissor state, so it cannot live anywhere else):

  ```cpp
  struct Font2d { bool ok() const; /* + private id/generation/owner, Texture2d idiom */ };
  enum class TextAlign { Left, Center, Right, Justify };   // own enum, zero third-party type
  struct TextOptions {
    float max_width = 0.f;                 // <= 0 = no wrap (the RectF{} sentinel idiom)
    TextAlign align = TextAlign::Left;     // default = the simple case, zero surprise
  };
  struct TextMetrics {
    bool ok = false;
    float width = 0, height = 0, ascent = 0, line_height = 0;
    int line_count = 0;                    // lines AFTER wrap + explicit \n
  };

  Font2d load_font(const char* path);
  void destroy_font(Font2d& font);
  void draw_text(const Font2d& font, const char* utf8, Vec2F pos, float size,
                 const ColorF& color = ColorF{});
  void draw_text(const Font2d& font, const char* utf8, Vec2F pos, float size,
                 const ColorF& color, const TextOptions& options);
  TextMetrics measure_text(const Font2d& font, const char* utf8, float size);
  TextMetrics measure_text(const Font2d& font, const char* utf8, float size,
                           const TextOptions& options);
  ```

  **Why an options struct + overload, not tail default args and not separate methods:** (1)
  the aggregate-with-defaults struct is the house idiom (`SpriteTransform` is the exact
  precedent, and `TextOptions{}` == the no-options overload by construction -- pinned by a
  unit test); (2) a growing tail of positional defaults (`max_width`, then `align`, then
  whatever a future consumer needs) is the call-site trap the house avoids; (3) a separate
  `draw_text_wrapped` would fork the text surface the same way `draw_sprite_world` was
  rejected in the ADR-0017 Addendum (Q1 option (b)). The 5-arg form stays the untouched simple
  case (absence-of-diff idiom). Zero third-party or core type crosses the header (golden
  boundary). NO `SpriteTransform` overload for text in v0.23.0 (`SEED-D2D-TEXT-TRANSFORM`).
  `size` and `max_width` are in ACTIVE-space units (px without camera, world units with --
  scale by projection, TX7).
- **TX2 -- the internal seam, TWO pure headers:** (a) `src/text_raster.hpp/.cpp` -- pure CPU
  (zero GL, zero RmlUi): UTF-8 decode (-> U+FFFD policy), face lifecycle over `glx_sfnt_open`
  (owns the file blob for the face's lifetime -- the core reads borrowed bytes), glyph bake
  (`glyph_outline` -> `glx_hint` when snapping applies -> `glx_rasterize_outline` into caller
  scratch), per-glyph advance/kern. (b) `src/text_layout.hpp/.cpp` -- pure POSITIONING math
  (zero GL, zero raster: consumes only an advance-measuring interface fed by (a)): explicit
  `\n` breaking, word-wrap (TX15), per-line alignment incl. justify space distribution (TX16),
  emitting `PlacedGlyph{gid, pen_x, pen_y}` runs plus the block metrics. Same single-source
  class as `transform2d.hpp`/`primitives2d.hpp` -- `measure_text` and `draw_text` consume THE
  SAME layout output (a mutation in advance/wrap/align math must break both).
- **TX3 -- atlas:** per-`Font2d`, per rounded-integer pixel size instance; shelf-pack; lazy bake
  on first sight of a (gid, size) pair; **GL_R8 texture + core-profile texture swizzle
  (RRRR)** so a coverage texel reads as premultiplied-white RGBA in the EXISTING shader -- zero
  shader change, the D8 tint formula colors text for free, 1 byte/texel. Glyph quads flow
  through the SAME batcher as sprites (a glyph run over one atlas texture batches exactly like
  a same-texture sprite run; D4 untouched).
- **TX4 -- atlas exhaustion is a NAMED hostile vector:** a hostile string streaming thousands of
  distinct codepoints grows the atlas; hard cap (atlas pages per font-size instance bounded;
  recommend 4096x4096 max, then oldest-page eviction is NOT built (over-engineering) -- past the
  cap, new glyphs draw as `.notdef` + one dedup'd log). Memory bound, not a performance claim
  (same D-series honesty as the 4096-quad and 262144-command caps).
- **TX5 -- bake size policy:** bake keyed by `round(size_at_screen)` clamped to [1, 512];
  fractional/zoomed sizes render the nearest baked size scaled by geometry (documented: text is
  pixel-crisp at integer screen sizes with no camera; under camera zoom it scales like every
  sprite). No per-frame rebake on zoom (atlas churn is the trap this policy exists to avoid).
- **TX6 -- fail-high matrix (central, not appendix):** `load_font(nullptr)`/open-fail/blob cap
  (64 MiB)/`glx_sfnt_open` reject -> `Font2d{ok()==false}`, never a crash (the parser is the
  hostile-input wall, but the shell still validates before and after); invalid/stale/foreign
  `Font2d` in any call -> rejected, dedup'd log (full D7 chain, same as `Texture2d`);
  `utf8 == nullptr` -> silent legal no-op for draw, `TextMetrics{}` for measure; per-call input
  cap 1 MiB of bytes per `draw_text`/`measure_text` (past it: processed up to the cap + one
  dedup'd log -- prevents a single hostile call from freezing a frame); non-finite
  `pos`/`size`/`color` member -> skip, dedup'd log, BEFORE any arithmetic (D10);
  `size <= 0` -> silent legal no-op (degenerate, the `dst.w <= 0` idiom); `TextOptions`
  validated BEFORE any layout arithmetic: non-finite `max_width` -> skip the call, dedup'd log
  (the D10 idiom; `max_width <= 0` itself is LEGAL, it is the no-wrap sentinel), an `align`
  value outside the enum's range (hostile cast) -> treated as `Left`, dedup'd log; post-math
  guard on projected glyph corners (D20's second guard, shared); every call null-safe on
  never-init/moved-from/post-shutdown (the module contract).
- **TX7 -- space/camera:** glyph quads are laid out in the active space and each corner
  projected via the existing `world_to_screen` path exactly like sprite corners (one
  composition story, no special case). Snapping (TX8) applies only when no camera is set.
- **TX8 -- hinting/snap:** pen-snap X + vsnap Y (the `glx_hint` defaults the ui already
  matched against FreeType's auto-hinter) apply in the no-camera, integer-size path; under a
  camera they are bypassed (snapping world-space text would make it swim -- documented).
- **TX9 -- CMake:** vendored core objects compile under
  `GLINTFX_OWN_FONT_ENGINE OR GLINTFX_MODULE_DRAW2D`; no new module flag, no sub-flag
  (`draw_text` is part of the draw2d atom; a sprites-only consumer pays ~3 C files of compile,
  declared as accepted cost in ADR-0018 -- a `GLINTFX_DRAW2D_TEXT` sub-flag is rejected as flag
  proliferation, revisitable if a real consumer asks). `GLINTFX_MODULE_TEXT` (reserved, ui-side
  enclosing flag) is UNTOUCHED. The `nm` gate extends: `GLINTFX_MODULE_DRAW2D=OFF` removes all
  text symbols too.
- **TX10 -- dependency-table amendment:** ADR-0018 amends ADR-0017 (e)'s draw2d row: depends on
  `core` + `image` seam + internal gl-loader **+ vendored font core (glx_sfnt/raster/hint)**.
  Stated loudly, not slipped in.
- **TX11 -- ui coexistence (the "how it lives with ui text" answer):** they share the C core
  and NOTHING else -- separate shells, separate atlases, separate GL textures, zero shared
  mutable state (the core is stateless by design, "no global/static mutable"). A document
  reload in the ui cannot invalidate a `Font2d`, and `Draw2d::shutdown()` cannot touch the ui's
  faces. This is the collision-avoidance story the leader asked the ADR to settle.
- **TX12 -- what GusWorld gets:** their 32..126 atlas becomes `load_font` + `draw_text` calls;
  their `font_atlas.hpp` becomes deletable; their world-space HUD text scales with their camera
  via TX7 with no extra code.
- **TX13 -- perf:** text throughput micro-bench added (informational); the EXISTING sprite perf
  budget test must not regress (gating, the wave-5 gate reused); measured with the LITERAL CI
  line (clang, no `CMAKE_BUILD_TYPE`).
- **TX14 -- versioning:** additive API only -> **v0.23.0 MINOR**; `version()` bump in CMake
  (the v0.3.0-era lesson).
- **TX15 -- word-wrap (leader-added scope), normative:** greedy line breaking against
  `max_width` in active-space units, measured with the SAME advance+kern accumulation as
  everything else (single source, TX2 (b)). Break opportunities are spaces (U+0020) only;
  consecutive spaces collapse at a break point (the space that caused the break is consumed,
  not rendered at the line edge); a single word wider than `max_width` force-breaks at the
  last glyph that fits, minimum one glyph per line (fail-safe: never an infinite loop, never
  overflow past `max_width` by more than that one mandatory glyph, never vanishing text);
  explicit `\n` always breaks. No hyphenation, no locale-aware break classes (U+00A0
  no-break-space is honoured as non-breaking -- one line of code, prevents a silly wrong
  break; anything smarter is `SEED-D2D-TEXT-TYPESETTING`).
- **TX16 -- alignment (leader-added scope), normative:** applied per line AFTER wrap, within
  the reference width (`max_width` when `> 0`, else the block's widest line): `Left` offset 0;
  `Center` offset `(ref_w - line_w) / 2`; `Right` offset `ref_w - line_w`. `Justify`
  distributes `extra = ref_w - line_w` equally across the line's N space gaps
  (`extra / N` per gap, exact float arithmetic, deterministic); lines with zero gaps, the
  block's last line, and lines ended by an explicit `\n` fall back to `Left` (typographic
  convention, documented); `Justify` with `max_width <= 0` degrades to `Left` (documented).
  Word-spacing ONLY -- no letter-spacing distribution (`SEED-D2D-TEXT-TYPESETTING`).
- **TX17 -- hand-computed layout gate (the mutation lesson, applied to TX15/TX16):** the unit
  tests of `text_layout` use the monospace fixture where every advance is a known constant, so
  expected line breaks and offsets are literal numbers: e.g. with advance `a` and
  `max_width = 5a`, the string `"aaa bb cc"` MUST break as `["aaa bb" is too wide -> "aaa",
  "bb cc"]`-style cases computed by hand in the test source; center offset of a 3-glyph line
  in a 5-glyph box MUST equal exactly `a`; justify with 2 gaps and `extra = 10` px MUST place
  the middle words at hand-computed x positions (+5 per gap). A test that a sign-flip or a
  swapped `Center`/`Right` formula survives is rejected in review.

**PT:** TX1 superfície pública em `Draw2d` (o texto participa de bracket/batcher/câmera/layer/
scissor, não pode morar em outro lugar): `Font2d` (idioma id/geração/dono do `Texture2d`),
`enum class TextAlign { Left, Center, Right, Justify }` (enum próprio, zero tipo de terceiro),
`TextOptions{max_width = 0 (<= 0 = sem wrap, idioma RectF{}), align = Left}`,
`load_font`/`destroy_font`, `draw_text(font, utf8, pos, size, color)` + overload com
`TextOptions`, `measure_text -> TextMetrics{ok,width,height,ascent,line_height,line_count}` +
overload com `TextOptions`. **Por que struct de opções + overload, e não default args de cauda
nem métodos separados:** o agregado-com-defaults é o idioma da casa (`SpriteTransform` é o
precedente exato, e `TextOptions{}` == overload sem opções, fixado por teste); cauda de
defaults posicionais é a armadilha de call-site que a casa evita; `draw_text_wrapped` separado
bifurcaria a superfície como o `draw_sprite_world` rejeitado no Adendo da ADR-0017 (Q1 (b)). O
overload de 5 args fica o caso simples intocado (idioma ausência-de-diff). Zero tipo de
terceiro/core no header; SEM overload de `SpriteTransform` na v0.23.0
(`SEED-D2D-TEXT-TRANSFORM`); `size` e `max_width` em unidades do espaço ativo. TX2 costura
interna em DOIS headers puros: (a) `src/text_raster.hpp/.cpp` (decode UTF-8, ciclo de vida da
face sobre `glx_sfnt_open` com posse do blob, bake via outline->hint->rasterize, avanço/kern
por glifo); (b) `src/text_layout.hpp/.cpp` -- matemática PURA de posicionamento (zero GL, zero
raster; consome só a interface de medição de avanço de (a)): quebra em `\n`, word-wrap (TX15),
alinhamento por linha inclusive distribuição de espaço do justify (TX16), emitindo runs de
`PlacedGlyph{gid, pen_x, pen_y}` + métricas do bloco; mesma classe fonte-única de
`transform2d.hpp`/`primitives2d.hpp` -- `measure_text` e `draw_text` consomem o MESMO layout
(mutação em avanço/wrap/align quebra os dois). TX3 atlas por
(fonte, tamanho-px inteiro), shelf-pack, lazy bake, **GL_R8 + swizzle RRRR** (texel de cobertura
vira RGBA branco-premultiplicado no shader EXISTENTE -- zero mudança de shader, o tint D8 colore
de graça, 1 byte/texel); quads de glifo passam pelo MESMO batcher dos sprites. TX4 exaustão de
atlas é vetor hostil NOMEADO: teto duro (4096x4096 por instância fonte-tamanho; além disso,
glifo novo desenha `.notdef` + log dedup'd; eviction NÃO é construída -- over-engineering).
TX5 política de bake: chave `round(size)` clampada [1, 512]; tamanho fracionário/zoom usa o bake
inteiro mais próximo escalado por geometria (nítido em tamanho inteiro sem câmera; sob zoom
escala como sprite; sem rebake por frame). TX6 matriz fail-high central: `load_font` nullptr/
falha de open/teto de blob 64 MiB/rejeição do parser -> `ok()==false` nunca crash; handle
inválido/obsoleto/estrangeiro -> cadeia D7 completa; `utf8 == nullptr` -> no-op legal / métricas
vazias; teto por chamada de 1 MiB (processa até o teto + log dedup'd -- uma chamada hostil não
congela um frame); membro não-finito -> pula ANTES de qualquer conta (D10); `size <= 0` -> no-op
silencioso; `TextOptions` validada ANTES de qualquer conta de layout (`max_width` não-finito ->
pula com log dedup'd; `max_width <= 0` é LEGAL, sentinela de sem-wrap; `align` fora do range do
enum por cast hostil -> tratado como `Left` com log dedup'd); guarda pós-conta nos cantos
projetados (D20); tudo null-safe. TX7 espaço/câmera:
cantos de glifo projetados pelo `world_to_screen` existente como sprites. TX8 pen-snap/vsnap só
no caminho sem câmera em tamanho inteiro (snap em espaço de mundo faria o texto "nadar" --
documentado). TX9 CMake: objetos do core vendorizado compilam sob `GLINTFX_OWN_FONT_ENGINE OR
GLINTFX_MODULE_DRAW2D`; sem flag nova nem sub-flag (custo de ~3 arquivos C pro consumidor
só-sprites, declarado no ADR-0018); `GLINTFX_MODULE_TEXT` (reservada, ui) INTOCADA; gate `nm`
estendido. TX10 emenda da tabela de dependência do ADR-0017 (e), dita em voz alta. TX11
convivência com a ui: compartilham o núcleo C e NADA mais (cascas, atlases e texturas GL
separados; o core é stateless por design; reload de documento na ui não invalida `Font2d`;
`Draw2d::shutdown()` não toca as faces da ui) -- é a resposta de colisão que o líder pediu ao
ADR. TX12 o que o GusWorld ganha: `font_atlas.hpp` deles vira deletável; texto de HUD em mundo
escala com a câmera sem código extra. TX13 perf: micro-bench de texto informacional + gate
existente de sprite não regride (linha LITERAL do CI: clang, sem `CMAKE_BUILD_TYPE`). TX14
aditivo -> **v0.23.0 MINOR**; bump do `version()` no CMake. TX15 word-wrap (escopo adicionado
pelo líder), normativo: quebra gulosa contra `max_width` em unidades do espaço ativo, medida
com a MESMA acumulação avanço+kern de tudo (fonte única, TX2 (b)); oportunidade de quebra = só
espaço (U+0020); o espaço que causou a quebra é consumido (não renderiza na borda); palavra
maior que `max_width` quebra forçada no último glifo que cabe, mínimo um glifo por linha
(fail-safe: nunca loop infinito, nunca texto sumindo); `\n` sempre quebra; U+00A0 honrado como
não-quebrável (uma linha de código); sem hifenização nem classes de quebra por locale
(`SEED-D2D-TEXT-TYPESETTING`). TX16 alinhamento (escopo adicionado pelo líder), normativo:
por linha, APÓS o wrap, dentro da largura de referência (`max_width` quando `> 0`, senão a
linha mais larga do bloco): `Left` offset 0; `Center` `(ref_w - line_w) / 2`; `Right`
`ref_w - line_w`; `Justify` distribui `extra = ref_w - line_w` igualmente entre os N gaps de
espaço da linha (`extra / N` por gap, aritmética float exata, determinística); linha sem gap, a
última linha do bloco e linha terminada por `\n` explícito caem pra `Left` (convenção
tipográfica, documentado); `Justify` com `max_width <= 0` degrada pra `Left`; SÓ word-spacing.
TX17 gate de layout à mão (a lição da mutação aplicada a TX15/TX16): os testes de
`text_layout` usam a fixture monoespaçada onde todo avanço é constante conhecida -- quebras e
offsets esperados são números LITERAIS no fonte do teste (com avanço `a` e `max_width = 5a`,
os casos de quebra computados à mão; offset de center de linha de 3 glifos em caixa de 5 ==
exatamente `a`; justify com 2 gaps e `extra = 10` px posiciona as palavras do meio em x
computado à mão, +5 por gap). Teste que sobrevive a um sinal trocado ou a `Center`/`Right`
permutados é rejeitado no review.

---

## 4. Execution map / Mapa de execução (max 4 sonnet, implementer != reviewer)

**EN (PT below):** Sequenced; one heavy build at a time (`TMPDIR=/var/tmp`, btrfs check via
`btrfs filesystem usage /`). Every brief: commit early and often (two agents crashed mid-wave-5;
the disk saved the work), cite the item ID (`D2D-TEXT`) in every commit, NO push (orchestrator
verifies `git log` itself), no `file:line` claims in docs without running
`tools/check_doc_line_refs.sh`.

| Slice | Agent | Depends on | Scope | Size |
|---|---|---|---|---|
| **T0** | `software-architect` | leader's decisions (recorded above) | Record ADR-0018 (options verbatim from section 2, decisions incl. the leader's scope expansion, reversibility, TX10 amendment) | P |
| **T1a** | `backend-engineer` #1 | T0 | The raster seam: `src/text_raster.hpp/.cpp` (TX2 (a), TX4 CPU-side, TX6 parse-side) + TX9 CMake decoupling + the tiny frozen advance-interface header T1b codes against (FIRST commit of the slice) + pure unit tests with HAND-COMPUTED values (TX17 gate, raster half) | M |
| **T1b** | `backend-engineer` #2 (distinct agent, disjoint files) | T0 + T1a's interface commit | The layout seam: `src/text_layout.hpp/.cpp` (TX2 (b), TX15 wrap, TX16 align/justify) -- PURE positioning math, zero GL, zero raster; unit tests with HAND-COMPUTED breaks/offsets (TX17 gate, layout half: the word-overflow break case, center-vs-left offset, justify gap distribution, all literal numbers) | M |
| **T2** | `engine-graphics-programmer` | T1a + T1b | `Draw2d` integration: `Font2d` registry (D7 idiom), R8+swizzle atlas upload, `draw_text`/`measure_text` (both overloads) through the batcher, TX7/TX8 camera/snap, TX6 draw-side guards incl. `TextOptions` validation, `app_draw2d_smoke` extension | M/G |
| **T3** | `qa-engineer` (adversarial, EXECUTES) | T2 | Hostile matrix end-to-end: malformed/truncated/bit-flipped TTF corpus vs `load_font`; invalid UTF-8 corpus; 1 MiB cap; atlas-exhaustion stream; foreign/stale handles; hostile `TextOptions` (NaN `max_width`, out-of-range align cast, `max_width` smaller than one glyph -- the force-break floor); pixel-readback under Xvfb (text run changes pixels inside the expected bbox, background elsewhere; a centered run's bbox visibly offset from a left run's); MUTATION passes on advance/kern/line-height AND wrap/align math (flip a sign, swap `Center`/`Right`, off-by-one the gap count in justify -- a test that survives the mutant is rejected); no-camera vs camera equivalence checks | M |
| **T4** | `technical-writer` | T2 (real signatures) | `docs/draw2d.md` Text section (contracts TX1-TX8 + TX15-TX16 verbatim, declared OUT list + seeds incl. the justify limits), `docs/embed-integration.md` cross-ref, `CHANGELOG.md` `[Unreleased]`, TODO.md status touches; `check_doc_line_refs.sh` before AND after | P/M |

**Parallelism note:** T1a and T1b are DISJOINT files by design (`text_raster.*` vs
`text_layout.*`, distinct agents, implementer != implementer) -- T1b starts as soon as T1a's
first commit freezes the advance interface, so the two pure halves overlap; everything else is
sequential. Never more than one heavy build at a time regardless.

**Hand-computed-value gate (the repeated lesson, TX17):** T1a/T1b's unit tests derive expected
advances/kern/line-height by parsing the fixture font's `hmtx`/`kern`/`hhea` tables
INDEPENDENTLY in the test (or from precomputed constants checked against a table dump), never
by calling the code-under-test twice. `measure_text("AB") == advance(A) + kern(A,B) +
advance(B)` with literal numbers; wrap/align cases per TX17 (the word-overflow break, the
center offset, the justify gap distribution -- all literal numbers on the monospace fixture).
Fixture fonts: `demos/showcase/assets/OpenSans-Regular.ttf` (proportional, has kern) and
`PixelOperatorMono.ttf` (monospace: `measure_text(n chars) == n * advance`, the cheapest
mutation-killer there is).

**Review:** implementer != reviewer != orchestrator. T1 reviewed by the T2 agent's counterpart
plus the adversarial T3 pass; T2 reviewed by an agent that did not write it (`tech-lead` class),
review EXECUTES the suite and at least one mutation by hand. The orchestrator re-verifies:
clean build on the LITERAL CI line, spot-checks file:line claims, runs the new tests, checks
`nm` gate on the OFF build, confirms no push happened. CI red BLOCKS the tag.

**Gates before tag `v0.23.0`:** full suite both legs (GLFW=ON / embed) + MSVC job green +
`nm`/encapsulation/doc-refs gates + sprite perf budget unregressed + CHANGELOG/version bump.
Gate order per house policy: github > claudio > codeberg, one green suffices.

**PT:** Sequenciado; 1 build pesado por vez (`TMPDIR=/var/tmp`; disco por `btrfs filesystem
usage /`). Todo brief: commitar cedo e sempre (2 agents caíram na onda 5; o disco salvou o
trabalho), citar `D2D-TEXT` nos commits, NÃO pushar (o orquestrador confere `git log` ele
mesmo), citação `arquivo:linha` em doc só com `tools/check_doc_line_refs.sh` rodado.
**Fatias:** T0 `software-architect` grava o ADR-0018 com as decisões (incl. a expansão de
escopo do líder) (P); T1a `backend-engineer` #1 faz a costura de raster (`text_raster.*`, TX2
(a)) + CMake + a interface de avanço congelada no PRIMEIRO commit + testes com VALORES À MÃO
(M); T1b `backend-engineer` #2 (agente distinto, arquivos disjuntos) faz a costura de layout
(`text_layout.*`, TX2 (b): wrap TX15 + align/justify TX16), matemática PURA de posicionamento,
testes com quebras/offsets LITERAIS (TX17) (M) -- T1b começa quando o commit de interface do
T1a congela, as duas metades puras rodam em paralelo; T2 `engine-graphics-programmer` integra
no `Draw2d` (registry, atlas R8+swizzle, os dois overloads no batcher, câmera/snap, guards
incl. validação de `TextOptions`) (M/G); T3 `qa-engineer` adversarial que EXECUTA (corpus de
TTF malformada, UTF-8 hostil, teto 1 MiB, exaustão de atlas, handles estrangeiros,
`TextOptions` hostil -- NaN em `max_width`, cast fora do enum, `max_width` menor que um glifo
(o piso da quebra forçada) --, pixel-readback sob Xvfb inclusive bbox de run centralizado
visivelmente deslocado do left, MUTAÇÃO nas fórmulas de avanço/kern/line-height E de
wrap/align -- sinal trocado, `Center`/`Right` permutados, off-by-one na contagem de gaps do
justify; teste que sobrevive ao mutante é rejeitado) (M); T4 `technical-writer` docs+changelog
incl. os limites do justify (P/M). **Gate de valor-à-mão (TX17):** o teste deriva o esperado
lendo `hmtx`/`kern`/`hhea` da fixture por caminho INDEPENDENTE (nunca chamando o código sob
teste duas vezes); wrap/align com números literais na fixture monoespaçada; fixtures
`OpenSans-Regular.ttf` (proporcional, tem kern) e `PixelOperatorMono.ttf` (monoespaçada:
`measure_text(n) == n * advance`, o mata-mutante mais barato que existe). **Review:** implementer != reviewer != orquestrador; review EXECUTA a suíte
e pelo menos uma mutação à mão; o orquestrador re-verifica build na linha LITERAL do CI,
spot-check de claims, testes novos, gate `nm` no build OFF, e confere que ninguém pushou. CI
vermelho BLOQUEIA a tag. **Gates pré-tag `v0.23.0`:** suíte completa nas duas pernas + MSVC
verde + gates nm/encapsulamento/doc-refs + perf de sprite sem regressão + CHANGELOG/version.

---

## 5. Counter-argument: does full `draw_text` fit one wave? / Contra-argumento: cabe numa onda?

**EN:** It was already a G-sized wave -- the biggest since D2D-1 -- and the leader's scope
expansion (wrap + align + justify) makes it heavier. Two split questions, answered separately:

1. **Split across RELEASES (6a ASCII / 6b UTF-8)?** Still NO: the core's cmap 4/12 lookup
   already handles any codepoint (ASCII-only removes no code, it adds an artificial filter),
   lazy bake makes Unicode coverage a non-cost until a glyph is requested, and kerning is one
   existing binary-search call. And the leader wants wrap+align in THIS release, so any split
   is intra-wave by his own decision.
2. **Split WITHIN the wave?** YES, and the expansion is exactly why the map now has T1a/T1b:
   wrap+align is pure positioning math over the metrics interface -- it parallelizes cleanly
   against the raster seam on disjoint files, so the added scope widens the wave without
   lengthening its critical path (T2 starts when both pure halves are unit-green). The wave is
   a heavy G, controlled, not two waves in a trench coat.

**Justify cost, named once more (duty, not re-litigation):** the one genuinely thorny addition
is justify; it is moderate here only because this wave has no complex shaping (word-spacing
arithmetic over already-wrapped lines). The pre-registered contingency stands: if mid-wave
evidence shows justify alone endangering v0.23.0, the proposal to the leader is cutting ONLY
justify to `SEED-D2D-TEXT-TYPESETTING`, keeping left/center/right -- his call, prepared in
advance.

**PT:** Já era uma onda G -- a maior desde o D2D-1 -- e a expansão de escopo do líder
(wrap + align + justify) a engorda. Duas perguntas de fatiamento, respondidas separadas:

1. **Fatiar entre RELEASES (6a ASCII / 6b UTF-8)?** Continua NÃO: o lookup cmap 4/12 já trata
   qualquer codepoint (ASCII-only não remove código, adiciona filtro artificial), o lazy bake
   torna Unicode custo-zero até um glifo ser pedido, e o kerning é uma busca binária que já
   existe. E o líder quer wrap+align NESTA release -- qualquer fatiamento é intra-onda por
   decisão dele.
2. **Fatiar DENTRO da onda?** SIM, e a expansão é exatamente por que o mapa agora tem T1a/T1b:
   wrap+align é matemática pura de posicionamento sobre a interface de métricas -- paraleliza
   limpo contra a costura de raster em arquivos disjuntos, então o escopo adicionado ALARGA a
   onda sem esticar o caminho crítico (o T2 começa quando as duas metades puras estão verdes
   no unit). A onda é um G pesado, controlado -- não duas ondas disfarçadas.

**Custo do justify, nomeado mais uma vez (dever, não reabertura):** a única adição espinhosa de
verdade é o justify; aqui é moderado só porque esta onda não tem shaping complexo (aritmética
de word-spacing sobre linhas já quebradas). A contingência pré-registrada vale: se evidência no
meio da onda mostrar o justify sozinho ameaçando a v0.23.0, a proposta ao líder é cortar SÓ o
justify pra `SEED-D2D-TEXT-TYPESETTING`, mantendo left/center/right -- decisão dele, preparada
de antemão.

---

## 6. Traps carried forward / Armadilhas herdadas (não repetir)

**EN/PT:**
1. Measure with the LITERAL CI line: clang, no `CMAKE_BUILD_TYPE` / Medir com a linha LITERAL
   do CI: clang, sem `CMAKE_BUILD_TYPE` (enganou 3x na onda anterior).
2. `/tmp` is tmpfs; heavy builds in `/var/tmp` with `TMPDIR=/var/tmp`; disk truth is
   `btrfs filesystem usage /` / build pesado em `/var/tmp`; verdade do disco é btrfs, não `df`.
3. One heavy build at a time (real OOM) / 1 build pesado por vez (OOM real).
4. Commit early; uncommitted work is fragile (2 agent crashes in wave 5) / commitar cedo;
   trabalho não commitado é frágil.
5. `file:line` citations rot; run `tools/check_doc_line_refs.sh` / citação `arquivo:linha`
   apodrece; rodar o gate.
6. NO push by any agent; orchestrator verifies remotes itself / NENHUM agent pusha; o
   orquestrador confere os remotos ele mesmo.
7. A test that cannot distinguish the mutation does not exist / teste que não distingue a
   mutação não existe (o gate de valor-à-mão da seção 4 existe por isso).
