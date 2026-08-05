# RMLX subset spec (what the RmlUi-elimination arc may build)

> **EN:** The frozen boundary for `RMLX-1..11` (`TODO.md`): exactly what RmlUi surface glintfx's own DOM/RCSS/layout/data-binding/effects/animation/widgets engine is allowed to reimplement, backed by two independent censuses (glintfx's own contact with RmlUi, and GusWorld's measured RCSS usage) plus the four value-type decisions the anticorruption layer already ships. Diátaxis type: **reference**. Audience: the implementer of any `RMLX-*` wave, and whoever reviews it. Owner: `software-architect`; written **2026-08-04** against `main` at `83b0649`, one wave (`RMLX-0`) fully delivered (`F1`-`F4`).
> **PT:** A fronteira congelada para `RMLX-1..11` (`TODO.md`): exatamente qual superfície do RmlUi o motor próprio de DOM/RCSS/layout/data-binding/efeitos/animação/widgets da glintfx tem permissão de reimplementar, apoiada em dois censos independentes (o contato da própria glintfx com o RmlUi, e o uso de RCSS medido no GusWorld) mais as quatro decisões de tipo-valor que a camada anticorrupção já entrega. Tipo Diátaxis: **reference**. Audiência: quem implementa qualquer onda `RMLX-*`, e quem a revisa. Owner: `software-architect`; escrito em **2026-08-04** contra `main` em `83b0649`, com uma onda (`RMLX-0`) inteiramente entregue (`F1`-`F4`).

---

## 🔴 The clause that gives this document power / A cláusula que dá poder a este documento

**EN:** **The waves `RMLX-1..11` may only implement what is in the subset frozen by this document (sections 1-5 below).** If a wave's implementer believes real corpus evidence (glintfx's own scenes, or GusWorld's measured usage) requires something outside this subset, the move is: **stop, edit this spec with a diff, get the líder's sign-off, then implement** -- never implement first and update the spec after the fact. This is the concrete mechanism that stops the layout wave (`RMLX-3`) from quietly turning into "let's build CSS": every property, selector and pseudo-class this document does not name is **out of scope by default**, not "maybe later".

**PT:** **As ondas `RMLX-1..11` só podem implementar o que está no subconjunto congelado por este documento (seções 1-5 abaixo).** Se o implementer de uma onda achar que evidência real de corpus (as cenas da própria glintfx, ou o uso medido do GusWorld) exige algo fora deste subconjunto, o movimento é: **parar, editar esta spec com um diff, pegar o aval do líder, só então implementar** -- nunca implementar primeiro e atualizar a spec depois. Este é o mecanismo concreto que impede a onda de layout (`RMLX-3`) de silenciosamente virar "vamos fazer CSS": toda propriedade, seletor e pseudo-classe que este documento não nomeia está **fora de escopo por padrão**, não "talvez depois".

---

## English

### 1. Our census -- glintfx's own contact with RmlUi

#### 1.1 Historical state (pre-`RMLX-0`, commit `1a67dea`)

Re-measured independently against the pre-`RMLX-0` commit (`1a67dea`, the last commit before `F1`'s `git mv`), scope `glintfx/src/` + `glintfx/include/` only, reproducible with `git grep -l/-c 'Rml::' 1a67dea -- glintfx/src glintfx/include`:

| Metric | Measured value |
| :--- | :--- |
| Files with a real `#include` of RmlUi (angle- or quote-bracket) | **19** |
| Files that mention the `Rml::` token at all (code or comment) but carry **no own `#include`** | **22** |
| Total files with any `Rml::` contact | **41** (19 + 22) |
| Lines matching `Rml::` (`git grep -c`, summed) | **1174** |
| Distinct `Rml::`-prefixed symbols (`git grep -oh 'Rml::[A-Za-z_][A-Za-z0-9_]*' \| sort -u \| wc -l`) | **94** |

Of the 19 files with a real `#include`, `RMLX-0/F1` moved **17** into `glintfx/src/rml/` via `git mv` (verified: `git show e7506a2 --stat`), leaving exactly **2** behind on purpose -- see 1.2.

#### 1.2 Current state (post-`F1`-`F3`, gate-verified)

`tools/check_rml_whitelist.sh` (run against `HEAD`, `83b0649`) reports **`OK`** on all three blocking checks and prints **`divida opaca: 4 arquivos`** for the report-only check. Concretely:

- Every real `#include` of RmlUi outside `glintfx/src/rml/` is confined to **2 frozen exceptions**: `glintfx/src/app.cpp` and `glintfx/src/system_glfw_dedup.hpp` (see section 4).
- Every value-type token (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`) outside `glintfx/src/rml/` is confined to those same 2 files (only `system_glfw_dedup.hpp` has a real, non-comment token; `app.cpp`'s `Rml::` mentions are 100% comment).
- **4 files** carry residual opaque `Rml::` usage (forward-declared pointers, not value types) outside `glintfx/src/rml/` and the 2 exceptions -- see section 5 for the exact list and which wave retires each.

#### 1.3 Divergence report (measured vs. the brief that requested this spec)

Two of the brief's own section-1 numbers do not survive re-measurement, and neither matches `TODO.md`'s own historical note either -- both are recorded here rather than silently copied:

- **"17 arquivos com `#include` direto"** -- the brief's own number is the count of files `F1` **moved** into `glintfx/src/rml/`, not the total that had a real include pre-`F1`. The actual pre-`F1` total is **19** (confirmed twice: my own `git grep` against `1a67dea`, and `TODO.md` line 327's own "Medido 2026-08-04: 19 arquivos"). The other 2 of the 19 are the frozen GLFW-bridge exception (section 1.2), not a gap -- 17 + 2 = 19.
- **"+4 que usam `Rml::` ... → 21 pontos de contato"** -- does not match either measurement. My reproducible count is **22** token-only files (not 4), for a total of **41** points of contact (not 21). `TODO.md`'s own line 327 states "56 mencionam `Rml::`, mas 37 são só comentário" (implying 19 real + 37 comment-only = 56) -- that 56/37 split is **also** not reproducible by the `git grep -l 'Rml::' 1a67dea -- glintfx/src glintfx/include` methodology used here (41 total, not 56); the two numbers this document's methodology **does** reproduce exactly, and which match both the brief and `TODO.md` verbatim, are **1174 lines** and **94 distinct symbols** -- those are trustworthy. The file-count split (19/22/41 vs. 19/37/56) is left as an open, unreconciled discrepancy rather than guessed at; whoever needs a precise file-contact count in the future should re-run the one-liner above against the commit in question rather than trust either historical figure.

### 2. The consumer's census -- GusWorld, 12 numbers (verbatim, re-verified)

Measured by the consumer, cross-checked here against `TODO.md` (the consumer's own remeasurement is quoted there verbatim, lines 328-337) -- all 12 numbers below match `TODO.md` exactly:

| Selector / property | Count |
| :--- | ---: |
| `:hover` | **53** |
| `:focus` | **3** |
| `:active` | **2** |
| `display: flex` | **10** |
| `data-if` | **23** |
| `data-for` | **8** |
| `data-model` | **24** |
| `@keyframes` | **6** |
| `transform` | **19** |
| `overflow` | **42** |
| `nth-child` | **0** |
| `:not(` | **0** |
| `z-index` | **0** |

Scope declared by the consumer: `GusEngine/app` in full, `.cpp`/`.hpp`/`.rml`, including tests and probes.

**Consequences this spec records, so a wave planner doesn't have to re-derive them:**

- **6 screens carry markup, not 7.** They are `battle_cockpit_rml.cpp`, `system_menu_rml.cpp`, `save_load_menu_rml.cpp`, `difficulty_menu_rml.cpp`, `title_menu_rml.cpp`, `npc_dialogue_rml.cpp`.
- 🎯 **The representative screen is `battle_cockpit_rml.cpp`** -- roughly 1.6× the markup of the runner-up, the only one with a composed HUD (portraits, card frames, simultaneous HP/AP/mana bars), and the one that most exercises nested layout. *"If wave 4's work passes on it, it passes on the other five."*
- ⚠️ **Do not use `appmode_spike/main.rml`** -- untracked scratch in the consumer's repo, may disappear at any time; its markup is copied as a corpus **fixture**, never referenced by path.
- `overflow` is the **2nd most-used property** in the consumer (after `:hover`), ahead of every layout property except `:hover` itself -- `RMLX-4` (flex + scroll + z-index) is sized around it, not around `flex`.
- `transform` (19) is **~3× more used than `@keyframes`** (6) -- `RMLX-8` (animation) is designed around 2D `transform` first, `@keyframes`/`animation` second.
- `nth-child`, `:not(`, and `z-index` all measure **zero**. Cutting them is a **real-zero loss**, not an "acceptable" one -- no wave needs to plan a fallback for them.

### 3. Four type decisions (with evidence)

Already shipped in `glintfx/src/rml/type_bridge.hpp` (`RMLX-0/F1`), and binding for every `RMLX-1..11` wave that touches these types:

| RmlUi type | Decision | Evidence (`file:line`) | glintfx equivalent |
| :--- | :--- | :--- | :--- |
| `Rml::String` | **No alias created.** It already **is** `std::string`; an alias would be pure noise. | `using String = std::string;` (`examples/RmlUi/Include/RmlUi/Config/Config.h:108`) | `std::string`, used directly |
| `Rml::Vector2f` | **Reuse `glintfx::Vec2F`** (already existed). Standard-layout, 2 floats, 8 bytes -- a field-copy conversion is safe. | `using Vector2f = Vector2<float>;` (`examples/RmlUi/Include/RmlUi/Core/Types.h:38`); fields `Type x; Type y;` (`examples/RmlUi/Include/RmlUi/Core/Vector2.h:124-125`) | `glintfx::Vec2F { float x, y; }` (`glintfx/include/glintfx/draw2d.hpp:158`) |
| `Rml::Colourb` | **Reuse `glintfx::ColorF`**, with `/255.f` (RmlUi→glintfx) / `std::clamp`+`std::lround` (glintfx→RmlUi) conversion in the bridge. 4 bytes, byte-packed vs. glintfx's 4-float straight-alpha. | `using Colourb = Colour<byte, 255, false>;` (`examples/RmlUi/Include/RmlUi/Core/Types.h:35`, also `Math.h:11`) | `glintfx::ColorF { float r,g,b,a; }` (`glintfx/include/glintfx/draw2d.hpp:142`) |
| `Rml::Variant` | **Not replicated, not wrapped.** Non-trivial class (user-declared copy/move ctor + dtor) with small-object-optimisation storage (`LOCAL_DATA_SIZE` local buffer). Stays entirely confined to `glintfx/src/rml/` callers that already include RmlUi headers directly -- a value-type bridge for a non-trivial class would just be a second `Variant` to keep in sync forever. | Ctors/dtor: `examples/RmlUi/Include/RmlUi/Core/Variant.h:49-54`; SSO buffer: `Variant.h:137` (`alignas(TransitionList) char data[LOCAL_DATA_SIZE];`) | *(none -- confined)* |

**The static_asserts are the actual point of `type_bridge.hpp`, more than the two conversion functions:** `sizeof(Rml::Vector2f) == 8` + `std::is_standard_layout_v<Rml::Vector2f>` (and the `Colourb` equivalent, `sizeof == 4`) are a **layout tripwire** against a future RmlUi version bump -- if upstream ever changes field count/type/order, the build fails at the exact place that assumed the old layout, instead of silently miscompiling a coordinate or a channel swap that only shows up as a visual bug at runtime.

`examples/RmlUi/` is the gitignored upstream clone used for this study (per `CLAUDE.md`'s RE-study convention); citations against it are for **evidence at the time of writing**, not a live-verified reference in every checkout (a fresh clone without `examples/RmlUi/` will not have that path on disk -- `tools/check_doc_line_refs.sh` skips, not fails, an unresolvable path, by design).

### 4. The whitelist and the gate contract

**Location.** `glintfx/src/rml/` is the only directory allowed to `#include` RmlUi directly, with the 2 frozen exceptions below. Everything a `RMLX-1..11` wave writes to *replace* RmlUi surface lives **outside** `glintfx/src/rml/` -- that directory is where the RmlUi-**dependent** code shrinks to, not where the new engine grows.

**The gate, `tools/check_rml_whitelist.sh`.** Four checks, run in this order (a/b/c blocking, d report-only), wired into `tools/preci.sh` (pre-commit) and `.github/workflows/ci.yml:449` (CI):

- **(a)** Any `#include` of RmlUi in `glintfx/src/`, `glintfx/include/`, `glintfx/demos/` outside `glintfx/src/rml/` → **FAIL** (`file:line`).
- **(b)** Any `#include` of RmlUi in `glintfx/tests/` outside a **frozen whitelist of 4 files** → **FAIL**. This caps the pre-existing test debt at exactly these 4, matched by basename: `domrw_sanity.cpp`, `focus_sanity.cpp`, `form_events_sanity.cpp`, `document_reload_leak.cpp` (all 4 confirmed present on disk). A new test that includes RmlUi directly fails the gate -- it does not silently grow the whitelist.
- **(c)** Value-type tokens (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`) in code (comments stripped) outside `glintfx/src/rml/` → **FAIL** (`file:line`).
- **(d)** Report-only, always printed: count of files with residual **opaque** `Rml::` usage (forward-declared pointers, i.e. anything not already caught by (c)'s value-type list) outside the whitelist. Format: `"divida opaca: N arquivos"` -- printed even when `N` is 0, because "zero declared" proves someone looked; an absent line proves nothing. Currently **N = 4** (section 5).

**Self-defending.** Every invocation runs an embedded `--selftest` first (two throwaway fixture trees under `mktemp`, never touching the real repo): a clean fixture must pass all 3 blocking checks, a fixture with one violation planted in each of (a)/(b)/(c) must fail all 3, independently. If the selftest itself fails, the gate **aborts before running the real check** -- a toothless gate must never silently report "OK" on the real tree. `tools/check_rml_whitelist.sh --selftest` runs the selftest alone, self-contained, no repo checkout needed.

**The 2 frozen exceptions -- the GLFW↔RmlUi bridge.** `glintfx/src/app.cpp` and `glintfx/src/system_glfw_dedup.hpp` `#include "RmlUi_Platform_GLFW.h"` **by quotes**, discovered while building this gate (`SEED-PONTE-GLFW-RMLUI`, `TODO.md`). They are exempt from checks (a) and (c) (`RML_INCLUDE_EXCEPTIONS` / `RML_TOKEN_EXCEPTIONS` in the script) because `App` owns the live `GLFWwindow*` that RmlUi's upstream `SystemInterface_GLFW` needs at construction time (only available post-`WindowGlfw::create()`), and `system_glfw_dedup.hpp` is `App`-mode's thin `LogMessage`-override subclass of that same upstream type (`LOGTHR-1`) -- its `Rml::Log::Type`/`Rml::String` signature is mandated by the base class it overrides, not a leak of glintfx's own choosing. **These two do not retire with `RMLX-1..11`** -- they retire when GLFW itself is replaced, which the líder has already named as the next dependency after this arc. Growing either exception list is a decision for the líder, not something a wave's implementer gets to do unilaterally to make a red gate pass.

### 5. Debt register -- 4 opaque-pointer files, and which wave quits each

The gate's check (d) currently reports exactly these 4 files, all holding a forward-declared RmlUi pointer type rather than a value type:

| File | Opaque type held | Retires at |
| :--- | :--- | :--- |
| `glintfx/src/data_binder.hpp` | `Rml::Context*` (parameter of `create()`) | `RMLX-11` |
| `glintfx/src/engine.hpp` / `engine.cpp` | `Rml::Context*` (the live document/DOM tree), `Rml::SystemInterface*` (the platform interface `attach()` takes) | `RMLX-11` |
| `glintfx/src/render_gl3.hpp` | `Rml::RenderInterface*` (the render callback contract RmlUi's `Context::Render()` calls into) | `RMLX-11` |

**Why all 4 retire at `RMLX-11` (excision) and not earlier, including not at `RMLX-10` (the flip):** `RMLX-10`'s own scope, quoting `TODO.md`, is explicitly modelled on `ADR-0011`'s **soft** flip -- `GLINTFX_OWN_UI_ENGINE=ON` becomes the *default*, "o RmlUi vira rollback selecionável em runtime, exatamente como o FreeType virou no ADR-0011". A soft flip, by that ADR's own precedent, keeps the old dependency **fully linked and its types alive** as the rollback path; only the *default* changes. `Rml::Context`/`Rml::SystemInterface`/`Rml::RenderInterface` therefore stay real, live types through `RMLX-10` regardless of which engine is selected at runtime. They can only actually disappear from these 4 signatures at `RMLX-11` -- "o RmlUi sai do repo" (`FetchContent` removed, patch removed, `NOTICE` updated) -- which is precisely when there is no longer a rollback to keep a type alive for. Assigning an earlier wave to any of the 4 would require guessing that a specific wave coincidentally stops needing the pointer while RmlUi is still linked as fallback, which is not evidence this document has; `RMLX-11` is the only wave whose own stated scope structurally guarantees it.

**The 4 tests with a deliberate RmlUi include** (`domrw_sanity.cpp`, `focus_sanity.cpp`, `form_events_sanity.cpp`, `document_reload_leak.cpp`, section 4's frozen test whitelist) are, by the same reasoning, differential/oracle tests against the *real* RmlUi -- they retire at `RMLX-11` too, or are rewritten earlier as A/B oracles once the relevant wave (`RMLX-1` DOM, `RMLX-5` events, `RMLX-1` DOM-reload) has a native engine to compare against, at the discretion of that wave's implementer, subject to the same "edit this spec first" clause in the header.

---

## Português

### 1. Nosso censo -- o contato da própria glintfx com o RmlUi

#### 1.1 Estado histórico (pré-`RMLX-0`, commit `1a67dea`)

Remedido de forma independente contra o commit pré-`RMLX-0` (`1a67dea`, o último antes do `git mv` da `F1`), escopo só `glintfx/src/` + `glintfx/include/`, reproduzível com `git grep -l/-c 'Rml::' 1a67dea -- glintfx/src glintfx/include`:

| Métrica | Valor medido |
| :--- | :--- |
| Arquivos com `#include` real de RmlUi (colchete angular ou aspas) | **19** |
| Arquivos que mencionam o token `Rml::` (código ou comentário) mas **sem `#include` próprio** | **22** |
| Total de arquivos com algum contato com `Rml::` | **41** (19 + 22) |
| Linhas casando `Rml::` (`git grep -c`, somado) | **1174** |
| Símbolos distintos prefixados por `Rml::` (`git grep -oh 'Rml::[A-Za-z_][A-Za-z0-9_]*' \| sort -u \| wc -l`) | **94** |

Dos 19 arquivos com `#include` real, a `RMLX-0/F1` moveu **17** para `glintfx/src/rml/` via `git mv` (verificado: `git show e7506a2 --stat`), deixando exatamente **2** de propósito -- ver 1.2.

#### 1.2 Estado atual (pós-`F1`-`F3`, verificado pelo gate)

O `tools/check_rml_whitelist.sh` (rodado contra `HEAD`, `83b0649`) reporta **`OK`** nos três checks bloqueantes e imprime **`divida opaca: 4 arquivos`** no check só-relatório. Concretamente:

- Todo `#include` real de RmlUi fora de `glintfx/src/rml/` está confinado a **2 exceções congeladas**: `glintfx/src/app.cpp` e `glintfx/src/system_glfw_dedup.hpp` (ver seção 4).
- Todo token de tipo-valor (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`) fora de `glintfx/src/rml/` está confinado aos mesmos 2 arquivos (só `system_glfw_dedup.hpp` tem token real, fora de comentário; as menções a `Rml::` de `app.cpp` são 100% comentário).
- **4 arquivos** carregam uso opaco residual de `Rml::` (ponteiro fwd-declarado, não tipo-valor) fora de `glintfx/src/rml/` e das 2 exceções -- ver seção 5 pra lista exata e qual onda quita cada um.

#### 1.3 Relatório de divergência (medido vs. o brief que pediu esta spec)

Dois dos próprios números da seção 1 do brief não sobrevivem à remedição, e nenhum bate com a nota histórica do próprio `TODO.md` -- os dois ficam registrados aqui em vez de copiados em silêncio:

- **"17 arquivos com `#include` direto"** -- o número do brief é a contagem de arquivos que a `F1` **moveu** pra `glintfx/src/rml/`, não o total com include real pré-`F1`. O total real pré-`F1` é **19** (confirmado duas vezes: meu próprio `git grep` contra `1a67dea`, e a linha 327 do `TODO.md`, "Medido 2026-08-04: 19 arquivos"). Os outros 2 dos 19 são a exceção congelada da ponte GLFW (seção 1.2), não uma lacuna -- 17 + 2 = 19.
- **"+4 que usam `Rml::` ... → 21 pontos de contato"** -- não bate com nenhuma das duas medições. Minha contagem reproduzível é **22** arquivos só-token (não 4), pra um total de **41** pontos de contato (não 21). A própria linha 327 do `TODO.md` diz "56 mencionam `Rml::`, mas 37 são só comentário" (implicando 19 reais + 37 só-comentário = 56) -- esse recorte 56/37 **também** não é reproduzível pela metodologia `git grep -l 'Rml::' 1a67dea -- glintfx/src glintfx/include` usada aqui (41 no total, não 56); os dois números que a metodologia deste documento **reproduz** exatamente, e que batem com o brief E com o `TODO.md` ao pé da letra, são **1174 linhas** e **94 símbolos distintos** -- esses são confiáveis. O recorte de contagem de arquivo (19/22/41 vs. 19/37/56) fica registrado como discrepância aberta e não-reconciliada, em vez de chutado; quem precisar de uma contagem precisa de pontos de contato no futuro deve rodar de novo o one-liner acima contra o commit em questão, em vez de confiar em qualquer uma das duas cifras históricas.

### 2. O censo do consumidor -- GusWorld, 12 números (verbatim, reverificado)

Medido pelo consumidor, cruzado aqui contra o `TODO.md` (a remedição do próprio consumidor está citada lá ao pé da letra, linhas 328-337) -- os 12 números abaixo batem com o `TODO.md` exatamente:

| Seletor / propriedade | Contagem |
| :--- | ---: |
| `:hover` | **53** |
| `:focus` | **3** |
| `:active` | **2** |
| `display: flex` | **10** |
| `data-if` | **23** |
| `data-for` | **8** |
| `data-model` | **24** |
| `@keyframes` | **6** |
| `transform` | **19** |
| `overflow` | **42** |
| `nth-child` | **0** |
| `:not(` | **0** |
| `z-index` | **0** |

Escopo declarado pelo consumidor: `GusEngine/app` inteiro, `.cpp`/`.hpp`/`.rml`, incluindo testes e sondas.

**Consequências que esta spec registra, pra quem planeja uma onda não precisar re-derivar:**

- **6 telas carregam markup, não 7.** São `battle_cockpit_rml.cpp`, `system_menu_rml.cpp`, `save_load_menu_rml.cpp`, `difficulty_menu_rml.cpp`, `title_menu_rml.cpp`, `npc_dialogue_rml.cpp`.
- 🎯 **A tela representativa é `battle_cockpit_rml.cpp`** -- ~1,6× o markup da segunda colocada, única com HUD composto (retratos, molduras de carta, barras de HP/AP/mana simultâneas), e a que mais exercita layout aninhado. *"Se o trabalho da onda 4 passar nela, passa nas outras cinco."*
- ⚠️ **Não usar `appmode_spike/main.rml`** -- scratch untracked no repo do consumidor, pode sumir a qualquer momento; o markup dele é copiado como fixture do corpus, nunca referenciado por caminho.
- `overflow` é a **2ª propriedade mais usada** no consumidor (depois de `:hover`), à frente de toda propriedade de layout menos o próprio `:hover` -- a `RMLX-4` (flex + scroll + z-index) é dimensionada em torno dela, não em torno de `flex`.
- `transform` (19) é **~3× mais usado que `@keyframes`** (6) -- a `RMLX-8` (animação) é desenhada em torno de `transform` 2D primeiro, `@keyframes`/`animation` depois.
- `nth-child`, `:not(` e `z-index` dão **zero** os três. Cortá-los é **perda real zero**, não "aceitável" -- nenhuma onda precisa planejar fallback pra eles.

### 3. As quatro decisões de tipo (com evidência)

Já entregues em `glintfx/src/rml/type_bridge.hpp` (`RMLX-0/F1`), e vinculantes pra toda onda `RMLX-1..11` que tocar nesses tipos:

| Tipo do RmlUi | Decisão | Evidência (`arquivo:linha`) | Equivalente na glintfx |
| :--- | :--- | :--- | :--- |
| `Rml::String` | **Nenhum alias criado.** Já **é** `std::string`; um alias seria ruído puro. | `using String = std::string;` (`examples/RmlUi/Include/RmlUi/Config/Config.h:108`) | `std::string`, usado direto |
| `Rml::Vector2f` | **Reusa `glintfx::Vec2F`** (já existia). Standard-layout, 2 floats, 8 bytes -- uma conversão por cópia de campo é segura. | `using Vector2f = Vector2<float>;` (`examples/RmlUi/Include/RmlUi/Core/Types.h:38`); campos `Type x; Type y;` (`examples/RmlUi/Include/RmlUi/Core/Vector2.h:124-125`) | `glintfx::Vec2F { float x, y; }` (`glintfx/include/glintfx/draw2d.hpp:158`) |
| `Rml::Colourb` | **Reusa `glintfx::ColorF`**, com conversão `/255.f` (RmlUi→glintfx) / `std::clamp`+`std::lround` (glintfx→RmlUi) na ponte. 4 bytes, empacotado em byte vs. os 4 floats straight-alpha da glintfx. | `using Colourb = Colour<byte, 255, false>;` (`examples/RmlUi/Include/RmlUi/Core/Types.h:35`, também `Math.h:11`) | `glintfx::ColorF { float r,g,b,a; }` (`glintfx/include/glintfx/draw2d.hpp:142`) |
| `Rml::Variant` | **Não replicado, não envolvido.** Classe não-trivial (ctor de cópia/move + dtor declarados pelo usuário) com armazenamento small-object-optimisation (buffer local `LOCAL_DATA_SIZE`). Fica inteiramente confinado aos chamadores de `glintfx/src/rml/` que já incluem os headers do RmlUi direto -- uma ponte de tipo-valor pra uma classe não-trivial seria só um segundo `Variant` pra manter sincronizado pra sempre. | Ctors/dtor: `examples/RmlUi/Include/RmlUi/Core/Variant.h:49-54`; buffer SSO: `Variant.h:137` (`alignas(TransitionList) char data[LOCAL_DATA_SIZE];`) | *(nenhum -- confinado)* |

**Os static_asserts são o ponto de fato do `type_bridge.hpp`, mais até que as duas funções de conversão:** `sizeof(Rml::Vector2f) == 8` + `std::is_standard_layout_v<Rml::Vector2f>` (e o equivalente pra `Colourb`, `sizeof == 4`) são um **fio de tropeço de layout** contra um futuro bump de versão do RmlUi -- se o upstream algum dia mudar contagem/tipo/ordem de campo, o build falha exatamente no lugar que assumia o layout antigo, em vez de miscompilar em silêncio uma coordenada ou uma troca de canal que só aparece como bug visual em runtime.

`examples/RmlUi/` é o clone upstream gitignorado usado pra este estudo (pela convenção de estudo/RE do `CLAUDE.md`); as citações contra ele valem como **evidência no momento em que foram escritas**, não como referência viva verificada em todo checkout (um clone novo sem `examples/RmlUi/` não terá esse caminho em disco -- o `tools/check_doc_line_refs.sh` pula, não falha, um caminho não-resolvível, de propósito).

### 4. A whitelist e o contrato do gate

**Localização.** `glintfx/src/rml/` é o único diretório com permissão de `#include` direto de RmlUi, com as 2 exceções congeladas abaixo. Tudo que uma onda `RMLX-1..11` escrever pra *substituir* superfície do RmlUi vive **fora** de `glintfx/src/rml/` -- esse diretório é pra onde o código RmlUi-**dependente** encolhe, não onde o motor novo cresce.

**O gate, `tools/check_rml_whitelist.sh`.** Quatro checagens, nesta ordem (a/b/c bloqueantes, d só-relatório), amarradas no `tools/preci.sh` (pre-commit) e no `.github/workflows/ci.yml:449` (CI):

- **(a)** Qualquer `#include` de RmlUi em `glintfx/src/`, `glintfx/include/`, `glintfx/demos/` fora de `glintfx/src/rml/` → **FALHA** (`arquivo:linha`).
- **(b)** Qualquer `#include` de RmlUi em `glintfx/tests/` fora de uma **whitelist congelada de 4 arquivos** → **FALHA**. Trava a dívida pré-existente exatamente nestes 4, casados por basename: `domrw_sanity.cpp`, `focus_sanity.cpp`, `form_events_sanity.cpp`, `document_reload_leak.cpp` (os 4 confirmados presentes em disco). Um teste novo que inclua RmlUi direto reprova o gate -- não faz a whitelist crescer em silêncio.
- **(c)** Tokens de tipo-valor (`Rml::String`/`Vector2`/`Colourb`/`Variant`/`Input`/`Log`) em código (comentário removido) fora de `glintfx/src/rml/` → **FALHA** (`arquivo:linha`).
- **(d)** Só-relatório, sempre impresso: contagem de arquivos com uso **opaco** residual de `Rml::` (ponteiro fwd-declarado, ou seja, tudo que o (c) ainda não pega pela lista de tipo-valor) fora da whitelist. Formato: `"divida opaca: N arquivos"` -- impresso mesmo quando `N` é 0, porque "zero declarado" prova que alguém olhou; linha ausente não prova nada. Atualmente **N = 4** (seção 5).

**Autodefesa.** Toda invocação roda primeiro um `--selftest` embutido (duas árvores de fixture descartáveis sob `mktemp`, nunca tocando o repo real): uma fixture limpa precisa passar nos 3 checks bloqueantes, uma fixture com uma violação plantada em cada um de (a)/(b)/(c) precisa falhar nos 3, independentemente. Se o próprio selftest falhar, o gate **aborta antes de rodar o check real** -- um gate sem dente nunca pode reportar "OK" em silêncio na árvore real. `tools/check_rml_whitelist.sh --selftest` roda só o selftest, autocontido, sem checkout do repo.

**As 2 exceções congeladas -- a ponte GLFW↔RmlUi.** `glintfx/src/app.cpp` e `glintfx/src/system_glfw_dedup.hpp` fazem `#include "RmlUi_Platform_GLFW.h"` **por aspas**, achado ao construir este gate (`SEED-PONTE-GLFW-RMLUI`, `TODO.md`). São isentos dos checks (a) e (c) (`RML_INCLUDE_EXCEPTIONS` / `RML_TOKEN_EXCEPTIONS` no script) porque o `App` é dono do `GLFWwindow*` vivo que o `SystemInterface_GLFW` upstream do RmlUi exige na construção (só disponível pós-`WindowGlfw::create()`), e `system_glfw_dedup.hpp` é a subclasse fina de override de `LogMessage` desse mesmo tipo upstream, modo `App` (`LOGTHR-1`) -- a assinatura `Rml::Log::Type`/`Rml::String` dela é exigida pela classe-base que ela sobrescreve, não um vazamento de escolha própria da glintfx. **Estes dois não saem com as ondas `RMLX-1..11`** -- saem quando o próprio GLFW for substituído, que o líder já nomeou como a próxima dependência depois deste arco. Fazer crescer qualquer uma das duas listas de exceção é decisão do líder, não algo que o implementer de uma onda decide sozinho pra deixar um gate vermelho passar.

### 5. Registro de dívida -- 4 arquivos com ponteiro opaco, e qual onda quita cada um

O check (d) do gate reporta hoje exatamente estes 4 arquivos, todos carregando um tipo-ponteiro fwd-declarado do RmlUi, não um tipo-valor:

| Arquivo | Tipo opaco carregado | Quita em |
| :--- | :--- | :--- |
| `glintfx/src/data_binder.hpp` | `Rml::Context*` (parâmetro do `create()`) | `RMLX-11` |
| `glintfx/src/engine.hpp` / `engine.cpp` | `Rml::Context*` (a árvore DOM/documento viva), `Rml::SystemInterface*` (a interface de plataforma que `attach()` recebe) | `RMLX-11` |
| `glintfx/src/render_gl3.hpp` | `Rml::RenderInterface*` (o contrato de callback de render que o `Context::Render()` do RmlUi invoca) | `RMLX-11` |

**Por que os 4 quitam na `RMLX-11` (excisão) e não antes, nem sequer na `RMLX-10` (o flip):** o próprio escopo da `RMLX-10`, citando o `TODO.md`, é explicitamente modelado no flip **suave** do `ADR-0011` -- `GLINTFX_OWN_UI_ENGINE=ON` vira o *default*, "o RmlUi vira rollback selecionável em runtime, exatamente como o FreeType virou no ADR-0011". Um flip suave, pelo próprio precedente daquele ADR, mantém a dependência antiga **plenamente linkada e seus tipos vivos** como caminho de rollback; só o *default* muda. `Rml::Context`/`Rml::SystemInterface`/`Rml::RenderInterface` portanto seguem sendo tipos reais e vivos durante toda a `RMLX-10`, independente de qual motor está selecionado em runtime. Só podem de fato sumir dessas 4 assinaturas na `RMLX-11` -- "o RmlUi sai do repo" (`FetchContent` removido, patch removido, `NOTICE` atualizado) -- que é precisamente quando não existe mais rollback nenhum pra manter um tipo vivo. Atribuir uma onda anterior a qualquer um dos 4 exigiria chutar que uma onda específica para de precisar do ponteiro por coincidência enquanto o RmlUi ainda está linkado como fallback, o que não é evidência que este documento tem; a `RMLX-11` é a única onda cujo próprio escopo declarado garante isso estruturalmente.

**Os 4 testes com include deliberado de RmlUi** (`domrw_sanity.cpp`, `focus_sanity.cpp`, `form_events_sanity.cpp`, `document_reload_leak.cpp`, a whitelist congelada de teste da seção 4) são, pela mesma lógica, testes de oráculo/diferencial contra o RmlUi *real* -- quitam na `RMLX-11` também, ou são reescritos antes como oráculos A/B assim que a onda relevante (`RMLX-1` DOM, `RMLX-5` eventos, `RMLX-1` DOM-reload) tiver um motor nativo pra comparar, a critério do implementer daquela onda, sujeito à mesma cláusula de "edite esta spec primeiro" do cabeçalho.
