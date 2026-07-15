# ADR-0011 — Soft font-engine flip / Flip suave de motor de fonte

- **Status:** Accepted (2026-07-15)
- **Deciders:** petrus (líder), caetano-cto
- **Tags:** font-engine, internalization, reversible
- **Builds on:** [[0009-internalization-boundary]] (hosted-shell + C-meat pattern -- the own font
  engine's algorithmic core, `glx_sfnt`/`glx_raster`/`glx_hint`, is exactly this pattern's first
  heap-touching, glyph-shaped instance).

## Context / Contexto

**EN:** `L1.19-FONTENG` (v0.9.1) landed glintfx's own clean-room font engine
(SOV-SFNT/SOV-RAST/SOV-HINT) as an **opt-in** build (`GLINTFX_OWN_FONT_ENGINE=OFF` by default --
zero behaviour change for existing consumers). `L1.20-FONTFLIP` then closed the gaps that stood
between "it builds and passes tests" and "it's good enough to be the default a real UI ships
with": nitidez at small body sizes (pen-snap X + `vsnap` Y, matched against FreeType's own
auto-hinter at the 9-15dp range GusWorld's HUD actually uses -- see
`feedback_gusworld_corpo_pequeno_fonte.md`), Unicode coverage beyond the initial warm set (lazy
bake), per-glyph font fallback for codepoints the primary face lacks, kerning fast enough for a
frame budget (O(log n) binary search, not O(n) linear scan), and a face-registration leak on
document reload. With those closed, the question became: **flip the default, or keep asking every
consumer to opt in by hand?**

**PT:** O `L1.19-FONTENG` (v0.9.1) entregou o motor de fonte próprio clean-room do glintfx
(SOV-SFNT/SOV-RAST/SOV-HINT) como build **opt-in** (`GLINTFX_OWN_FONT_ENGINE=OFF` por padrão --
zero mudança de comportamento pra consumidor existente). O `L1.20-FONTFLIP` então fechou os gaps
entre "builda e passa nos testes" e "é bom o bastante pra ser o default que uma UI de verdade
embarca": nitidez em corpo pequeno (pen-snap X + `vsnap` Y, empatado contra o auto-hinter do
próprio FreeType na faixa 9-15dp que o HUD do GusWorld de fato usa -- ver
`feedback_gusworld_corpo_pequeno_fonte.md`), cobertura Unicode além do warm set inicial (lazy
bake), fallback de fonte por-glifo pra codepoints que a face primária não tem, kerning rápido o
bastante pro orçamento de um frame (busca binária O(log n), não varredura linear O(n)), e um
vazamento de registro de face em reload de documento. Com isso fechado, a pergunta virou: **fazer
o flip do default, ou continuar pedindo pra cada consumidor optar manualmente?**

## Decision / Decisão

**EN:** Flip the default -- but **softly**, not completely. `GLINTFX_OWN_FONT_ENGINE=ON` becomes
the CMake default (own engine is compiled in unless a consumer opts out at build time), and
`FontEngine::Own` becomes the runtime default on both `AppConfig` and `UiLayerConfig`. This is
explicitly distinguished from a **full/irreversible flip** (dropping FreeType as a linked
dependency entirely, i.e. `RMLUI_FONT_ENGINE` no longer `freetype`, `NOTICE`'s FTL entry removed)
-- that is a **future, separately-decided** milestone, not scoped here.

Two properties make this flip "soft":

1. **FreeType stays fully linked and supported, either way.** `GLINTFX_OWN_FONT_ENGINE=OFF` at
   build time is still a supported configuration (own engine simply isn't compiled in -- the
   pre-flip default, still exercised by CI). `GLINTFX_OWN_FONT_ENGINE=ON` (the new default) also
   keeps FreeType linked; it just adds the own engine alongside it.
2. **A consumer that hits a regression rolls back with a one-line config change, no rebuild.**
   `font_engine = FontEngine::FreeType` on `AppConfig`/`UiLayerConfig` selects RmlUi's
   built-in, FreeType-backed engine at construction time -- the escape hatch is a source-level
   flag flip in the *consumer's* code, not a glintfx rebuild, not a glintfx patch release. If the
   library was itself built with `GLINTFX_OWN_FONT_ENGINE=OFF`, requesting `Own` degrades to a
   logged warning and a FreeType fallback -- never a crash.

This scope is deliberately narrower than ADR-0009's `libcore.a`/`glx_*` static-link boundary,
which this ADR **builds on rather than re-decides**: the own font engine's algorithmic core
(`glx_sfnt`/`glx_raster`/`glx_hint`) already follows ADR-0009's hosted-shell + C-meat pattern (the
`FontEngineOwn` C++ class implementing `Rml::FontEngineInterface` is ordinary hosted C++; only the
SFNT/rasterization/hinting math is sovereign C, vendored under `glintfx/vendor/core/` for this
release specifically so a plain `glintfx/` checkout can build it standalone). What's new *here* is
strictly the **default value** of an already-shipped, already-tested selector -- not a new
internalization mechanism.

**PT:** Fazer o flip do default -- mas de forma **suave**, não completa. `GLINTFX_OWN_FONT_ENGINE=ON`
vira o default do CMake (o motor próprio é compilado a menos que um consumidor opte por sair em
tempo de build), e `FontEngine::Own` vira o default de runtime em `AppConfig` e `UiLayerConfig`.
Isso é explicitamente distinto de um **flip completo/irreversível** (largar o FreeType como
dependência linkada por completo, ou seja `RMLUI_FONT_ENGINE` deixar de ser `freetype`, a entrada
FTL do `NOTICE` removida) -- esse é um marco **futuro, decidido separadamente**, fora de escopo
aqui.

Duas propriedades tornam este flip "suave":

1. **O FreeType continua plenamente linkado e suportado, de qualquer forma.**
   `GLINTFX_OWN_FONT_ENGINE=OFF` em tempo de build continua sendo uma configuração suportada (o
   motor próprio simplesmente não é compilado -- o default pré-flip, ainda exercitado pelo CI).
   `GLINTFX_OWN_FONT_ENGINE=ON` (o novo default) também mantém o FreeType linkado; só adiciona o
   motor próprio ao lado dele.
2. **Um consumidor que encontrar uma regressão reverte com uma mudança de config de uma linha,
   sem rebuild.** `font_engine = FontEngine::FreeType` em `AppConfig`/`UiLayerConfig` seleciona o
   motor embutido do RmlUi, apoiado em FreeType, em tempo de construção -- a saída de emergência é
   um flip de flag em nível de fonte no código do *consumidor*, não um rebuild do glintfx, não um
   patch release do glintfx. Se a própria lib foi buildada com `GLINTFX_OWN_FONT_ENGINE=OFF`, pedir
   `Own` degrada pra um aviso logado e fallback pro FreeType -- nunca um crash.

Este escopo é deliberadamente mais estreito que a fronteira `libcore.a`/`glx_*` de link estático do
ADR-0009, que este ADR **usa como base em vez de redecidir**: o núcleo algorítmico do motor de
fonte próprio (`glx_sfnt`/`glx_raster`/`glx_hint`) já segue o padrão casca-hosted + carne-C do
ADR-0009 (a classe C++ `FontEngineOwn` que implementa `Rml::FontEngineInterface` é C++ hosted
comum; só a matemática de SFNT/rasterização/hinting é C soberano, vendorizada em
`glintfx/vendor/core/` nesta release especificamente pra que um checkout puro de `glintfx/` consiga
buildá-la standalone). O que é novo *aqui* é estritamente o **valor default** de um seletor já
entregue e já testado -- não um mecanismo novo de internalização.

## Consequences / Consequências

**EN:** Every new consumer gets glintfx's own font engine by default, without asking -- this is
the intended effect (it is the internalization track's first user-visible payoff). Existing
consumers that never touch `font_engine` inherit the new default on their next glintfx upgrade;
this is a **behavioural** change even though no signature is removed (`v0.10.0` is MINOR, not
PATCH, precisely because rendering output can differ pixel-for-pixel even where the API is
unchanged). The rollback path (`FontEngine::FreeType`, no rebuild) is the safety valve that makes
shipping this as a default -- rather than requiring every consumer to discover and opt in --
acceptable risk. The **full/irreversible flip** (dropping FreeType entirely) remains explicitly
open and unscheduled; when it is proposed, it needs its own ADR and its own líder sign-off,
because unlike this one it removes a rollback path rather than adding a default.

**PT:** Todo consumidor novo ganha o motor de fonte próprio do glintfx por padrão, sem pedir --
esse é o efeito pretendido (é o primeiro payoff visível-ao-usuário da trilha de internalização).
Consumidores existentes que nunca tocam `font_engine` herdam o novo default no próximo upgrade do
glintfx; isso é uma mudança **comportamental** mesmo sem nenhuma assinatura removida (a `v0.10.0`
é MINOR, não PATCH, precisamente porque a saída de renderização pode diferir pixel-a-pixel mesmo
com a API inalterada). O caminho de rollback (`FontEngine::FreeType`, sem rebuild) é a válvula de
segurança que torna aceitável embarcar isso como default -- em vez de exigir que cada consumidor
descubra e opte manualmente. O **flip completo/irreversível** (largar o FreeType por completo)
segue explicitamente aberto e sem agenda; quando for proposto, precisa do próprio ADR e do próprio
aval do líder, porque, diferente deste, remove um caminho de rollback em vez de adicionar um
default.

## Options considered / Opções consideradas

- **Soft flip -- default ON, FreeType stays selectable (chosen / escolhida).** Flips the default
  value of an already-shipped selector; keeps FreeType fully linked and one config-line away.
  **Pros:** near-zero regression risk (instant, no-rebuild rollback); every consumer benefits from
  the internalization work without opting in by hand; does not foreclose the full flip later.
  **Cons:** the binary still links both engines (larger binary, `libcore.a` present alongside
  FreeType), so it doesn't yet realize the long-term "independent of third-party font code" goal
  -- it's a step, not the destination.
- **Full/irreversible flip -- drop FreeType entirely.** Remove `RMLUI_FONT_ENGINE=freetype`,
  `libfreetype` linkage, and the `NOTICE` FTL entry; own engine becomes the only engine.
  **Pros:** realizes the full internalization goal for fonts; smaller binary; no dual-engine
  maintenance burden. **Cons:** no config-line rollback for a consumer that hits an edge case the
  own engine doesn't yet cover (CJK shaping beyond the fallback path, complex script layout,
  color-emoji fonts, etc. are not proven equivalents); removes a safety net before the own engine
  has enough field mileage across consumers other than GusWorld to justify it. Rejected **for
  now** -- explicitly left open as a future, separately-decided ADR once the own engine has
  broader production exposure.
- **Stay opt-in (do nothing).** Keep `GLINTFX_OWN_FONT_ENGINE=OFF` as the default indefinitely,
  requiring every consumer to discover and flip it manually. **Pros:** zero behavioural change
  risk for anyone. **Cons:** the internalization work sits unused by default indefinitely, which
  defeats its purpose; GusWorld already validated the own engine in production-adjacent testing
  through v0.9.1/v0.9.2, so the risk this option avoids is already largely retired. Rejected.

Cross-ref: [[0009-internalization-boundary]] (`libcore.a`/`glx_*`, hosted-shell + C-meat pattern
this ADR builds on), [[0006-layered-hybrid-architecture]] (the two-layer split, unaffected by this
ADR).
