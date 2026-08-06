# RMLX subset spec (what the RmlUi-elimination arc may build)

> **EN:** The frozen boundary for `RMLX-1..11` (`TODO.md`): exactly what RmlUi surface glintfx's own DOM/RCSS/layout/data-binding/effects/animation/widgets engine is allowed to reimplement, backed by two independent censuses (glintfx's own contact with RmlUi, and GusWorld's measured RCSS usage) plus the four value-type decisions the anticorruption layer already ships. Diátaxis type: **reference**. Audience: the implementer of any `RMLX-*` wave, and whoever reviews it. Owner: `software-architect`; written **2026-08-04** against `main` at `83b0649`, one wave (`RMLX-0`) fully delivered (`F1`-`F4`). **Amended 2026-08-06 (§6, EN / §6, PT):** the líder's two `RMLX-2` decisions on comma-list selectors and unit parity, plus the selector-form census that closes this document's own naming gap -- see `TODO.md`'s `RMLX-2` entry ("TRÊS DECISÕES DO LÍDER 2026-08-06") for the source record.
> **PT:** A fronteira congelada para `RMLX-1..11` (`TODO.md`): exatamente qual superfície do RmlUi o motor próprio de DOM/RCSS/layout/data-binding/efeitos/animação/widgets da glintfx tem permissão de reimplementar, apoiada em dois censos independentes (o contato da própria glintfx com o RmlUi, e o uso de RCSS medido no GusWorld) mais as quatro decisões de tipo-valor que a camada anticorrupção já entrega. Tipo Diátaxis: **reference**. Audiência: quem implementa qualquer onda `RMLX-*`, e quem a revisa. Owner: `software-architect`; escrito em **2026-08-04** contra `main` em `83b0649`, com uma onda (`RMLX-0`) inteiramente entregue (`F1`-`F4`). **Emendado em 2026-08-06 (§6 EN / §6 PT):** as duas decisões do líder na `RMLX-2` sobre seletor com lista de vírgula e paridade de unidades, mais o censo de formas de seletor que fecha a lacuna de nomeação do próprio documento -- ver o item `RMLX-2` do `TODO.md` ("TRÊS DECISÕES DO LÍDER 2026-08-06") pelo registro-fonte.

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

### 6. Amendment 2026-08-06 -- comma-list selectors, full unit parity, and the closed selector-form census

**Decided by:** the líder, 2026-08-06, on the evidence of `UIX-RCSS-CENSUS` -- an independent measurement (`qa-engineer`, read-only, no repo writes) covering **62 source files**, **866 style blocks**, **3424 declarations**; report at `/var/tmp/censo-rcss-qa1/censo.md` (scratch, not checked into the repo -- the tables below are this section's durable copy of the parts that bind a `RMLX-*` wave). Recorded verbatim in `TODO.md`'s `RMLX-2` entry ("TRÊS DECISÕES DO LÍDER 2026-08-06"). This section is the diff the header clause requires before any `RMLX-2` slice may use either construct -- `docs/uix-rcss.md` (written the same day, before this amendment landed) already assumed both decisions and cited `TODO.md` directly for the selector scope this section now states as this document's own; going forward, cite this section instead.

#### 6.1 Decision 1 -- comma-list selectors (`.a, .b { }`) are authorized

Not a corner case: the census found **15 instances across 8 source files**, 3 of them in glintfx's own `glintfx/src/ua_stylesheet.hpp` -- including a **16-tag list in a single rule** (`div, p, h1, h2, h3, h4, h5, h6, ul, ol, li, section, article, header, footer, nav, main`, lines 99-102) that is the **first rule in the file** and sets `display: block` on every structural element of every document glintfx renders. Without this selector form, the new engine cannot apply its own base stylesheet, and every document renders wrong from the first frame. The full 15, exhaustive:

| # | File:line | Full selector |
| :--- | :--- | :--- |
| 1 | `glintfx/tests/data_model_embed_scene.rcss:81` | `#hpbox.wide, #namebox.wide, #flagwide.wide` |
| 2 | `glintfx/tests/form_events_scene.rcss:26` | `div, span, input` |
| 3 | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml:104` | `.apnum, .mananum` |
| 4 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_carregar_dois_slots_ocupados.rml:62` | `#slmenu-list scrollbarvertical sliderarrowdec, #slmenu-list scrollbarvertical sliderarrowinc` |
| 5 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_salvar_com_autosave.rml:61` | same pair, sibling screen-state fixture |
| 6 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_salvar_todos_vazios.rml:61` | same pair |
| 7 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_audio_sliders.rml:362` | `#ctrl-list scrollbarvertical sliderarrowdec, #ctrl-list scrollbarvertical sliderarrowinc` |
| 8 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_categorias.rml:363` | same pair |
| 9 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_controles_tabela.rml:363` | same pair |
| 10 | `glintfx/src/uix/dom/test_fixtures/system_menu__confirmacao_menu_inicial.rml:363` | same pair |
| 11 | `glintfx/src/uix/dom/test_fixtures/system_menu__pause_raiz.rml:362` | same pair |
| 12 | `glintfx/tests/fonteng_colr_scene.rml:100` | `#two_layer, #two_layer_vs16, #fg_sentinel` |
| 13 | `glintfx/src/ua_stylesheet.hpp:99-102` | 16-tag list (above) -- the UA-stylesheet base rule |
| 14 | `glintfx/src/ua_stylesheet.hpp:120-121` | `scrollbarvertical sliderarrowdec, scrollbarvertical sliderarrowinc` |
| 15 | `glintfx/src/ua_stylesheet.hpp:140-141` | `scrollbarhorizontal sliderarrowdec, scrollbarhorizontal sliderarrowinc` |

**Correction on record, the orchestrator's own error:** an earlier pass reported **1** occurrence, wrong twice in opposite directions -- first counting a comment as a match, then requiring the comma and the `{` on the same source line (the UA-stylesheet's own rule wraps across 4 lines). The census number above is the one that governs (`TODO.md`'s `RMLX-2` entry; see also `feedback_controle_positivo_e_ciclo_do_bus` for the full account).

**A second, smaller discrepancy found while writing this amendment, left on record rather than silently reconciled:** the census's own prose (its section 1.1) states "15 usos em 8 arquivos-fonte distintos," and `TODO.md`'s `RMLX-2` entry repeats "8 arquivos" verbatim -- but the census's own exhaustive table (its section 1.2, reproduced whole above) lists **13 distinct file paths**, not 8 (`data_model_embed_scene.rcss`, `form_events_scene.rcss`, `gusworld_battle_cockpit.rml`, 3 distinct `save_load_menu__*.rml` fixtures, 5 distinct `system_menu__*.rml` fixtures, `fonteng_colr_scene.rml`, and `ua_stylesheet.hpp` counted once despite holding 3 of the 15 rows). Counted by hand, twice, against the table above: 15 rows, 13 unique paths. This does not change decision 1 -- 13 files is, if anything, stronger evidence for authorizing the form than 8 -- but it means the "8" figure is the census's own summary undercounting its own table, not a fact this amendment re-derives. Whoever needs the precise file count in the future should count the table in 6.1 directly, the same discipline section 1.3 already applies to a different pair of numbers in this document.

#### 6.2 Closing the gap decision 1 exposes -- every selector form this document authorizes, by count

Section 2's real-zero cuts (`nth-child`, `:not(`, `z-index`) name individual pseudo-classes and one property, never a selector *form* -- so before this amendment, the header clause's own rule ("every selector this document does not name is out of scope by default") left **every** selector form out of scope by omission, including the ones already shipping in glintfx's own UA-stylesheet. This table closes that gap with `UIX-RCSS-CENSUS` section 1's full result -- **1073 selector instances across 319 distinct selectors, in 842 rules**, covering both zero and non-zero forms alike:

| Form | Instances | Authorized | Example `file:line` |
| :--- | ---: | :--- | :--- |
| `.class` | 590 | **yes** | `glintfx/demos/showcase/showcase.rcss:40` -- `.section-dark` |
| `#id` | 183 | **yes** | `glintfx/tests/app_process_event_scene.rcss:23` -- `#btn_a` |
| descendant (space) | 131 | **yes** | `glintfx/tests/click_scene.rcss:12` -- `#panel span` |
| tag | 110 | **yes** | `glintfx/demos/showcase/showcase.rcss:17` -- `body` |
| pseudo-class, composite (`.foo:hover`/`#foo:hover`; `:hover` is the only pseudo-class found) | 37 | **yes**, `:hover` only | `glintfx/src/uix/dom/test_fixtures/difficulty_menu__lista_hardcore_bloqueado.rml:69` -- `.difficulty-item:hover` |
| comma-list (multiple selectors, one rule) | 15 | **yes -- this amendment** | `glintfx/src/ua_stylesheet.hpp:99-102` |
| compound, no combinator (`tag.class`/`tag#id`) | 5 | **yes** | `glintfx/tests/fonteng_ab_visual_scene.rml:63` -- `div.row` |
| child (`>`) | 2 | **yes** | `glintfx/tests/app_process_event_scene.rcss:55` -- `#scroller > div` |
| universal (`*`) | **0** | **no -- fail-high** | -- |
| attribute (`[x]`) | **0** | **no -- fail-high** | -- |
| adjacent/general sibling (`+`/`~`) | **0** | **no -- fail-high** | -- |
| `nth-child`, `:not(` | **0** (already named §2) | **no -- fail-high** | -- |

Every measured selector instance in the census matches an authorized form -- the corpus does not already contain an unauthorized construct in real use. This is now `docs/rmlx-subset.md`'s own selector-form boundary; `docs/uix-rcss.md` section 11's own restatement of it should cite this section going forward, rather than `TODO.md`'s `RMLX-2` entry directly.

⚠️ **Zero in this corpus is not an argument to ban.** glintfx's target is broad distribution, not GusWorld-plus-glintfx's-own-test-corpus -- "zero in these two repositories" is a statement about two repositories, never about the world (see `feedback_gusworld_nao_define_prioridade`, `feedback_escopo_distribuicao_geral`). What stays out today stays out **under the fail-high policy** already canonized by `polygon()` and restated by `docs/uix-rcss.md` section 11: an unrecognized selector form fails the *whole rule* to register (never the whole stylesheet, never a partial/guessed match), logged naming the raw selector text and `file:line` -- never a silent break. Authorizing more selector forms later follows the header clause exactly as this amendment did: stop, edit this spec with a diff, get the líder's sign-off, then implement.

#### 6.3 Decision 2 -- units: full parity with what the old engine accepts, not the measured minimum

The census measured 8 units in real use (`dp` 2237, `px` 334, `%` 207, unitless 171, `auto` 33, `deg` 24, `s` 4, `em` 1 -- zero `rem`/`vw`/`vh`/`ms`). The líder chose **full clean-room parity with every unit RmlUi itself accepts**, not the 8-unit measured floor, trading the larger implementation effort for compatibility with whoever migrates a stylesheet glintfx has never seen. This is binding for `RMLX-2`'s parser and cascade regardless of which units this corpus happens to exercise today -- the measured 8 are the **evidence that informed weighing the decision**, not the resulting scope.

🔴 **Independent of the unit list, this document records what the líder's decision text itself requires stated explicitly: `%` is three distinct semantics, not one.** A single generic `resolve_percent()` is wrong by construction, and two independent implementers (side A vs. side B of `RMLX-2`'s differential oracle, `docs/uix-rcss.md`) would diverge exactly there if this were left implicit:

1. **Box-relative** (`width`/`height`/`left`/`top`, 103 of the corpus's 207 `%` instances) -- resolves against the containing block's own dimension, the standard CSS model; requires the container's box resolved **before** the child's `%`.
2. **Gradient stop position** (`decorator: linear-gradient()`/`radial-gradient()`, the majority of the 104 `%` instances counted under `decorator`) -- `0%..100%` along the gradient's own axis, unrelated to the element's box.
3. **Radial-gradient center coordinate** (`radial-gradient(circle at 35% 30%, ...)`) -- the two percentages are X/Y coordinates inside the decorator's own 2D box, a third semantics, neither the gradient axis nor layout's containing block.

`docs/uix-rcss.md` section 5 already implements this three-way split in the dump format that ships `RMLX-2`'s oracle; this amendment is what makes that split a decision **this** document -- the one with sign-off authority under the header clause -- also records, not only an implementer's private inference from the dump spec alone.

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

### 6. Emenda 2026-08-06 -- seletor com lista de vírgula, paridade completa de unidades, e o censo de seletor que fecha a lacuna

**Decidido por:** o líder, em 2026-08-06, com a evidência do `UIX-RCSS-CENSUS` -- medição independente (`qa-engineer`, só-leitura, zero escrita no repo) cobrindo **62 arquivos-fonte**, **866 blocos de estilo**, **3424 declarações**; relatório em `/var/tmp/censo-rcss-qa1/censo.md` (scratch, não versionado -- as tabelas abaixo são a cópia durável desta seção das partes que vinculam uma onda `RMLX-*`). Registrado ao pé da letra no item `RMLX-2` do `TODO.md` ("TRÊS DECISÕES DO LÍDER 2026-08-06"). Esta seção é o diff que a cláusula do cabeçalho exige antes de qualquer fatia da `RMLX-2` poder usar qualquer uma das duas construções -- o `docs/uix-rcss.md` (escrito no mesmo dia, antes desta emenda existir) já assumia as duas decisões e citava o `TODO.md` direto pro escopo de seletor que esta seção agora registra como próprio deste documento; daqui em diante, citar esta seção.

#### 6.1 Decisão 1 -- seletor com lista de vírgula (`.a, .b { }`) está autorizado

Não é caso de canto: o censo achou **15 ocorrências em 8 arquivos-fonte**, 3 delas na própria `glintfx/src/ua_stylesheet.hpp` -- incluindo uma **lista de 16 tags numa regra só** (`div, p, h1, h2, h3, h4, h5, h6, ul, ol, li, section, article, header, footer, nav, main`, linhas 99-102) que é a **primeira regra do arquivo** e dá `display: block` a todo elemento estrutural de todo documento que a glintfx renderiza. Sem essa forma de seletor, o motor novo não aplica a própria folha base, e todo documento renderiza errado desde o primeiro frame. As 15, exaustivas:

| # | Arquivo:linha | Seletor completo |
| :--- | :--- | :--- |
| 1 | `glintfx/tests/data_model_embed_scene.rcss:81` | `#hpbox.wide, #namebox.wide, #flagwide.wide` |
| 2 | `glintfx/tests/form_events_scene.rcss:26` | `div, span, input` |
| 3 | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml:104` | `.apnum, .mananum` |
| 4 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_carregar_dois_slots_ocupados.rml:62` | `#slmenu-list scrollbarvertical sliderarrowdec, #slmenu-list scrollbarvertical sliderarrowinc` |
| 5 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_salvar_com_autosave.rml:61` | mesmo par, fixture irmã de screen-state |
| 6 | `glintfx/src/uix/dom/test_fixtures/save_load_menu__modo_salvar_todos_vazios.rml:61` | mesmo par |
| 7 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_audio_sliders.rml:362` | `#ctrl-list scrollbarvertical sliderarrowdec, #ctrl-list scrollbarvertical sliderarrowinc` |
| 8 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_categorias.rml:363` | mesmo par |
| 9 | `glintfx/src/uix/dom/test_fixtures/system_menu__config_controles_tabela.rml:363` | mesmo par |
| 10 | `glintfx/src/uix/dom/test_fixtures/system_menu__confirmacao_menu_inicial.rml:363` | mesmo par |
| 11 | `glintfx/src/uix/dom/test_fixtures/system_menu__pause_raiz.rml:362` | mesmo par |
| 12 | `glintfx/tests/fonteng_colr_scene.rml:100` | `#two_layer, #two_layer_vs16, #fg_sentinel` |
| 13 | `glintfx/src/ua_stylesheet.hpp:99-102` | lista de 16 tags (acima) -- a regra base da UA-stylesheet |
| 14 | `glintfx/src/ua_stylesheet.hpp:120-121` | `scrollbarvertical sliderarrowdec, scrollbarvertical sliderarrowinc` |
| 15 | `glintfx/src/ua_stylesheet.hpp:140-141` | `scrollbarhorizontal sliderarrowdec, scrollbarhorizontal sliderarrowinc` |

**Correção registrada, erro do próprio orquestrador:** uma passada anterior reportou **1** ocorrência, errando duas vezes em direções opostas -- primeiro contando um comentário como se batesse, depois exigindo a vírgula e o `{` na mesma linha-fonte (a própria regra da UA-stylesheet quebra em 4 linhas). O número do censo acima é o que vale (item `RMLX-2` do `TODO.md`; ver também `feedback_controle_positivo_e_ciclo_do_bus` pro relato completo).

**Uma segunda divergência, menor, achada ao escrever esta emenda, registrada em vez de reconciliada em silêncio:** a própria prosa do censo (seção 1.1 dele) diz "15 usos em 8 arquivos-fonte distintos", e o item `RMLX-2` do `TODO.md` repete "8 arquivos" ao pé da letra -- mas a própria tabela exaustiva do censo (seção 1.2 dele, reproduzida inteira acima) lista **13 caminhos de arquivo distintos**, não 8 (`data_model_embed_scene.rcss`, `form_events_scene.rcss`, `gusworld_battle_cockpit.rml`, 3 fixtures `save_load_menu__*.rml` distintas, 5 fixtures `system_menu__*.rml` distintas, `fonteng_colr_scene.rml`, e `ua_stylesheet.hpp` contado uma vez apesar de carregar 3 das 15 linhas). Contado à mão, duas vezes, contra a tabela acima: 15 linhas, 13 caminhos únicos. Isso não muda a decisão 1 -- 13 arquivos é, se algo, evidência mais forte pra autorizar a forma do que 8 -- mas significa que a cifra "8" é o próprio resumo do censo subcontando a própria tabela dele, não um fato que esta emenda re-deriva. Quem precisar da contagem exata de arquivo no futuro deve contar a tabela da seção 6.1 diretamente, a mesma disciplina que a seção 1.3 já aplica a outro par de números deste documento.

#### 6.2 Fechando a lacuna que a decisão 1 expõe -- toda forma de seletor que este documento autoriza, com contagem

Os cortes de zero real da seção 2 (`nth-child`, `:not(`, `z-index`) nomeiam pseudo-classes individuais e uma propriedade, nunca uma *forma* de seletor -- então, antes desta emenda, a própria regra da cláusula do cabeçalho ("todo seletor que este documento não nomeia está fora de escopo por padrão") deixava **toda** forma de seletor fora de escopo por omissão, incluindo as que já rodam na própria UA-stylesheet da glintfx. Esta tabela fecha essa lacuna com o resultado completo da seção 1 do `UIX-RCSS-CENSUS` -- **1073 instâncias de seletor em 319 seletores distintos, dentro de 842 regras**, cobrindo formas com zero e com uso real igualmente:

| Forma | Instâncias | Autorizada | Exemplo `arquivo:linha` |
| :--- | ---: | :--- | :--- |
| `.classe` | 590 | **sim** | `glintfx/demos/showcase/showcase.rcss:40` -- `.section-dark` |
| `#id` | 183 | **sim** | `glintfx/tests/app_process_event_scene.rcss:23` -- `#btn_a` |
| descendente (espaço) | 131 | **sim** | `glintfx/tests/click_scene.rcss:12` -- `#panel span` |
| tag | 110 | **sim** | `glintfx/demos/showcase/showcase.rcss:17` -- `body` |
| pseudo-classe, composto (`.foo:hover`/`#foo:hover`; `:hover` é a única pseudo-classe encontrada) | 37 | **sim**, só `:hover` | `glintfx/src/uix/dom/test_fixtures/difficulty_menu__lista_hardcore_bloqueado.rml:69` -- `.difficulty-item:hover` |
| lista com vírgula (vários seletores, uma regra) | 15 | **sim -- esta emenda** | `glintfx/src/ua_stylesheet.hpp:99-102` |
| composto, sem combinador (`tag.classe`/`tag#id`) | 5 | **sim** | `glintfx/tests/fonteng_ab_visual_scene.rml:63` -- `div.row` |
| filho (`>`) | 2 | **sim** | `glintfx/tests/app_process_event_scene.rcss:55` -- `#scroller > div` |
| universal (`*`) | **0** | **não -- fail-high** | -- |
| atributo (`[x]`) | **0** | **não -- fail-high** | -- |
| irmão adjacente/geral (`+`/`~`) | **0** | **não -- fail-high** | -- |
| `nth-child`, `:not(` | **0** (já nomeados na §2) | **não -- fail-high** | -- |

Toda instância de seletor medida no censo casa com uma forma autorizada -- o corpus não contém, hoje, nenhuma construção não-autorizada já em uso real. Esta é agora a fronteira de forma-de-seletor do próprio `docs/rmlx-subset.md`; a seção 11 do `docs/uix-rcss.md`, ao restatar isso, deve citar esta seção daqui em diante, em vez do item `RMLX-2` do `TODO.md` direto.

⚠️ **Zero neste corpus não é argumento para banir.** O alvo da glintfx é distribuição ampla, não GusWorld-mais-o-corpus-de-teste-da-própria-glintfx -- "zero nestes dois repositórios" é uma afirmação sobre dois repositórios, nunca sobre o mundo (ver `feedback_gusworld_nao_define_prioridade`, `feedback_escopo_distribuicao_geral`). O que fica fora hoje fica fora **sob a política fail-high** já canonizada pelo `polygon()` e restatada pela seção 11 do `docs/uix-rcss.md`: uma forma de seletor não-reconhecida faz **a regra inteira** falhar ao registrar (nunca a folha inteira, nunca um casamento parcial/chutado), logada nomeando o texto cru do seletor e `arquivo:linha` -- nunca quebra em silêncio. Autorizar mais formas de seletor depois segue a cláusula do cabeçalho exatamente como esta emenda seguiu: parar, editar esta spec com um diff, pegar o aval do líder, só então implementar.

#### 6.3 Decisão 2 -- unidades: paridade completa com o que o motor antigo aceita, não o mínimo medido

O censo mediu 8 unidades em uso real (`dp` 2237, `px` 334, `%` 207, sem-unidade 171, `auto` 33, `deg` 24, `s` 4, `em` 1 -- zero `rem`/`vw`/`vh`/`ms`). O líder escolheu **paridade clean-room completa com toda unidade que o próprio RmlUi aceita**, não o piso de 8 unidades medidas, trocando o esforço maior de implementação pela compatibilidade de quem migra uma folha de estilo que a glintfx nunca viu. Isto é vinculante para o parser e a cascata da `RMLX-2` independentemente de quais unidades este corpus exercita hoje -- as 8 medidas são a **evidência que informou o peso da decisão**, não o escopo resultante.

🔴 **Independente da lista de unidades, este documento registra o que o próprio texto da decisão do líder exige que fique explícito: `%` são três semânticas distintas, não uma.** Uma `resolve_percent()` genérica única é errada por construção, e dois implementadores independentes (lado A vs. lado B do oráculo diferencial da `RMLX-2`, `docs/uix-rcss.md`) divergiriam exatamente aí se isto ficasse implícito:

1. **Relativo à caixa** (`width`/`height`/`left`/`top`, 103 das 207 instâncias de `%` no corpus) -- resolve contra a dimensão do próprio containing block, o modelo CSS padrão; exige a caixa do container resolvida **antes** do `%` do filho.
2. **Posição de parada de gradiente** (`decorator: linear-gradient()`/`radial-gradient()`, a maioria das 104 instâncias de `%` contadas em `decorator`) -- `0%..100%` ao longo do próprio eixo do gradiente, sem relação com a caixa do elemento.
3. **Coordenada do centro do gradiente radial** (`radial-gradient(circle at 35% 30%, ...)`) -- as duas porcentagens são coordenadas X/Y dentro da própria caixa 2D do decorator, uma terceira semântica, nem eixo de gradiente nem containing block de layout.

O `docs/uix-rcss.md` seção 5 já implementa essa separação em três no formato de dump que serve o oráculo da `RMLX-2`; esta emenda é o que faz essa separação virar uma decisão que **este** documento -- o que tem autoridade de aval sob a cláusula do cabeçalho -- também registra, não só uma inferência privada do implementer a partir da spec de dump sozinha.
