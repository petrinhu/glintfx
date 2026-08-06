# UIX-RCSS-AMBIGUIDADE -- ambiguity audit of `docs/uix-rcss.md` / auditoria de ambiguidade de `docs/uix-rcss.md`

> **EN:** Independent ambiguity audit of `docs/uix-rcss.md` (the `RMLX-2` computed-value dump
> contract), performed by the `tech-lead` **without reading either oracle side's implementation**
> (`glintfx/src/uix/style/**`, `glintfx/src/rml/rcss_dump*`, or their tests) -- only the spec itself,
> `docs/rmlx-subset.md`, and the upstream RmlUi clone at `examples/RmlUi/` (read directly, cited
> `file:line`, never paraphrased from the spec's own citations). Diátaxis type: **reference**
> (a ledger of findings against a fixed target). Audience: `software-architect` (owns
> `docs/uix-rcss.md`), the líder (sign-off authority per that document's own header clause), and
> whoever implements side A or side B of the `RMLX-2` oracle next. Owner: `tech-lead`; written
> **2026-08-06** against `docs/uix-rcss.md` as it stood after its own `UIX-RCSS-ERRATA-1` pass, same
> day, same commit range.
> **PT:** Auditoria independente de ambiguidade do `docs/uix-rcss.md` (o contrato de dump de valor
> computado da `RMLX-2`), feita pelo `tech-lead` **sem ler a implementação de nenhum dos dois lados
> do oráculo** (`glintfx/src/uix/style/**`, `glintfx/src/rml/rcss_dump*`, ou os testes deles) -- só a
> própria spec, o `docs/rmlx-subset.md`, e o clone upstream do RmlUi em `examples/RmlUi/` (lido
> direto, citado `arquivo:linha`, nunca parafraseado das próprias citações da spec). Tipo Diátaxis:
> **reference** (um ledger de achados contra um alvo fixo). Audiência: `software-architect` (dono do
> `docs/uix-rcss.md`), o líder (autoridade de aval pela própria cláusula de cabeçalho daquele
> documento), e quem implementar o lado A ou o lado B do oráculo da `RMLX-2` a seguir. Owner:
> `tech-lead`; escrito **2026-08-06** contra o `docs/uix-rcss.md` como estava logo após a própria
> passada `UIX-RCSS-ERRATA-1`, mesmo dia, mesma faixa de commit.

**SCOPE: 68 regras normativas enumeradas, 7 BLOQUEIA, 3 PROVÁVEL, 1 COSMÉTICO, 4 não-determináveis
contra o upstream (números de corpus cujo arquivo-fonte de censo, `/var/tmp/censo-rcss-qa1/censo.md`,
já não existe nesta máquina -- era scratch, não versionado).**

🔴 **Há fatia BLOQUEANTE em voo.** Sete achados abaixo (A-G) produzem divergência de byte com
certeza contra o comportamento real do RmlUi upstream, rastreado linha a linha no próprio código-fonte
citado. Dois deles (A, B) invalidam exemplos trabalhados que o próprio `docs/uix-rcss.md` já publica
como "âncora byte-exata" depois de uma passada de errata que existia especificamente para fechar esta
classe de problema. Nenhum dos dois lados do oráculo (`RMLX-2`) deveria começar a implementar contra
a spec como está hoje sem que estes sete pontos sejam resolvidos pelo `software-architect` + aval do
líder.

---

## English

### Method

Every normative sentence in `docs/uix-rcss.md` (rules that tell an implementer to do something --
format, order, rounding, unit, inheritance, escape, separator, terminator, what to omit, what to
include) was enumerated section by section (table below), then tested against: *"do two competent
implementers, reading only this sentence, produce the same bytes?"* Where the spec cites upstream
RmlUi evidence, that citation was read directly in `examples/RmlUi/` -- not trusted from the spec's
own paraphrase -- and where the spec's own worked examples give byte-exact output, that output was
independently recomputed from the traced upstream algorithm. Two of the seven BLOQUEIA findings were
found exactly this way: by recomputing a worked example the spec already publishes as its own proof
and getting a different answer.

### Enumerated rules and verdicts

| # | Section | Rule (paraphrased) | Verdict | Note |
| :--- | :--- | :--- | :--- | :--- |
| R1 | 1 | Dump reports post-cascade, pre-layout computed values; box-relative `%` not resolved | PASS | Verified: `ComputeLength(...)` (`ComputeProperty.cpp:52-69`) takes no containing-block param, confirmed by direct read |
| R2 | 1 | `dp`/`px`/`vw`/`vh`/`em`/`rem`/angle/number resolved now | PASS | -- |
| R3 | 2 | Reuses `uix-dom.md` §3 node addressing verbatim | PASS | Inherited, out of this doc's own primary claim surface |
| R4 | 2 | `head` gets no `PROP` records | PASS | -- |
| R5 | 2 | Only element nodes carry `PROP`; text nodes get none, never synthesized | PASS | Clear, unambiguous as stated |
| R6 | 3 | N `STATE` blocks, each independent full-tree enumeration, back to back | PASS | See R11 for the one open edge |
| R7 | 3 | `PROPS <n>` always present, `n`=72 fixed, even for zero-declaration nodes | PASS | -- |
| R8 | 3 | `PROP` sort order: ascending byte-wise by property name | PASS | -- |
| R9 | 3 | Traversal = pre-order depth-first per `uix-dom.md` §5, repeated per `STATE` | PASS | -- |
| R10 | 3 | Escaping = `uix-dom.md` §2's 4-rule table | PASS* | *See Finding H -- the table doesn't cover this doc's *own new* separators |
| **R11** | 3 | File-level trailing newline after the final line | **BLOQUEIA** | **Finding D** |
| R12 | 4 | Exactly 2 states enumerated: `none`, `hover-all` | PASS | Set membership is clear |
| **R13** | 4 | File order of the `STATE` blocks (`none` before `hover-all`) | **BLOQUEIA** | **Finding E** |
| R14 | 4 | `hover-all` forces `:hover` true globally, simultaneously, not per-element | PASS | -- |
| R15 | 4 | `:focus`/`:active` not separate matrix rows this wave | PASS | -- |
| R16 | 5 | Family (a) box-relative -- stays symbolic | PASS | -- |
| R17 | 5 | Family (b) gradient-stop position -- stays symbolic | PASS | -- |
| R18 | 5 | Family (c) radial-center coordinate -- stays symbolic | PASS | -- |
| R19 | 5 | Families (b)/(c) never merge | PASS | Byte-anchored by §15.3 |
| R20 | 5 | Gradient-stop auto-spacing IS resolved (the one concrete-from-symbolic exception) | PASS | glintfx's own decorator grammar, not upstream-checkable |
| R21 | 6 | Registry built exclusively from measured names + shorthand-expanded longhands | PASS (rule clear) | Corpus counts themselves: see não-determináveis |
| R22 | 6 | Evidentiary citation: `RegisterProperty`'s 4th positional arg is `inherited`, not `forces_layout` | PASS | Verified: `Include/RmlUi/Core/StyleSheetSpecification.h:84` |
| R23 | 6 | Sort order ascending byte-wise, no locale (restated) | PASS | Duplicate of R8, consistent |
| R24 | 6.1 | 72-entry registry table (values, `inherited` flags) | PASS (spot-checked) | Verified `border-top-width`(`0px`,no), `border-top-color`(`black`,no), `opacity`(`1`,**yes**), `focus`(`auto`,**yes**) against `StyleSheetSpecification.cpp:282-376` directly; remaining ~66 entries **not individually re-traced** in this audit -- limitation, not a finding |
| R25 | 6.1 | `focus`/`opacity` both `inherited: true`, flagged as surprising-but-correct | PASS | Verified at the exact call sites (`:351`, `:376`) |
| R26 | 6.1 | `max-height`/`max-width` kept despite zero corpus, teto declared | PASS (rule clear) | Corpus number itself unverifiable, census gone |
| R27 | 6.2 | 13-shorthand table, algorithm type per shorthand (Box/FallThrough/RecursiveRepeat/Replicate/Flex) | PASS (spot-checked) | Verified `border-top`/`-right`/`-bottom`/`-left` = FallThrough, `border` = RecursiveRepeat, `margin`/`padding`/`border-radius`/`border-color` registration lines read directly |
| R28 | 6.2 | `flex` shorthand: omitted trailing values default to `1`/`1`/`0`, not each property's own initial | PASS | Verified verbatim: `default_omitted_values[] = {"1","1","0"}`, `PropertySpecification.cpp:322` |
| **R29** | 6.2 | Errata: `border-top` FallThrough order-sensitivity, and its **consequence** | **BLOQUEIA (half right)** | **Finding A** -- order-sensitivity itself is correctly re-derived; the *consequence* ("both longhands revert") is wrong |
| **R30** | 6.2 | `border`'s `RecursiveRepeat`: "if any of the 4 sub-expansions fails, the *whole* declaration is dropped, not just the failing side" | **BLOQUEIA** | **Finding A** (same root cause, extended to `border`) |
| R31 | 6.3 | `Box` algorithm 1/2/3/4-value table | PASS | Verified verbatim against `PropertySpecification.cpp:336-370` |
| R32 | 7 | `keyword` domain: exact registered string, lowercase, never re-cased | PASS | -- |
| R33 | 7 | `number` domain: quantized, no suffix, sign rules | PASS | See R41-42 for `quantize()` itself |
| R34 | 7 | `length` domain: always `px` suffix | PASS | Verified via `Unit.h` `LENGTH` family |
| R35 | 7 | `length-percent`: either resolved-length or symbolic `%`, never both | PASS | -- |
| R36 | 7 | `color` domain: 8-digit lowercase hex, straight alpha | PASS (as a *form* rule) | See Finding B for where it's factually false as a *value* rule |
| R37 | 7 | `string` domain: raw content, §3-escaped, no quotes | PASS (as stated) | See Finding H for the escape-table gap this rule inherits |
| R38 | 7.1 | 4 in-scope hex forms (`#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`), doubling + default-alpha normalization | PASS (structurally verified) | `PropertyParserColour.cpp:211-237`'s `switch(value.size())` shape confirmed present; digit-by-digit doubling logic not exhaustively re-traced |
| R39 | 7.1 | Out-of-scope named colors/functional forms require fail-high | PASS | Clear rule |
| **R40** | 7.1 | "Colors are dumped straight-alpha... **including** colors nested inside a box-shadow layer or a gradient stop -- none of those get premultiplied before printing either" | **BLOQUEIA** | **Finding B** |
| R41 | 8 | `quantize()` algorithm: widen to double, round-half-away-from-zero explicit, canonicalize `-0.0` | PASS | Algorithm is literal and unambiguous as written |
| R42 | 8 | 4 decimal digits, applied to all float fields, colors exempt | PASS | -- |
| **R43** | 8 | `quantize()` behaviour for NaN/±Infinity input | **PROVÁVEL** | **Finding J** |
| R44 | 8.1 | Printed length unit always `px`, never source unit | PASS | Verified via `Unit.h:58` `LENGTH = PX \| DP \| VW \| VH \| EM \| REM \| PPI_UNIT` |
| R45 | 8.2 | Angles canonicalized to degrees always, `rad`→`deg` formula | PASS | -- |
| R46 | 8.2 | Angle print form = bare `quantize()` output, no unit suffix (doc's own self-closed gap) | PASS (as now stated) | Already closed by the doc's own errata; no residual ambiguity for a fresh reader |
| R47 | 9 | Outer separator `\|`, inner separator `;`, both distinct from source's `,` | PASS (as a design choice) | Interacts with Finding H |
| R48 | 9 | Empty composite list / initial value prints literal `none` | PASS | -- |
| R49 | 9.1 | box-shadow tokens assigned by *order of successful length parse*, not list position; `inset`/color skip | PASS | Verified verbatim against `PropertyParserBoxShadow.cpp`'s `length_argument_index` logic |
| R50 | 9.1 | 6-field layer grammar, `spread` defaults when omitted, `inset` always literal `true`/`false` | PASS (form) | Omission-ratio corpus number unverifiable |
| R51 | 9.1 | Multiple `box-shadow` layers join with `\|` in **source order**, never sorted | PASS | Clear, well-justified (ordered stack) |
| **R52** | 9.1 | §9.1's own worked example (`box-shadow: ... inset, #22D3EE26 ...` → `...#22d3ee26;...`) | **BLOQUEIA** | **Finding B** (this exact worked example is byte-wrong; recomputed below) |
| **R53** | 9.2 | "All four [`decorator`/`mask-image`/`filter`/`backdrop-filter`] share [...] identical comma-list-of-functions shape" | **BLOQUEIA** | **Finding F** |
| R54 | 9.2 | Function argument-order table (`image`/`linear-gradient`/`radial-gradient`/`polygon`/`image-tint`/`ripple`/`horizontal-gradient`/`blur`/`drop-shadow`) | PASS (not independently re-verifiable for the glintfx-custom functions) | `linear-gradient`/`radial-gradient`/`image`/`blur`/`drop-shadow` are native RmlUi decorators, structurally plausible; `polygon`/`image-tint`/`ripple`/`horizontal-gradient` are glintfx-authored, outside `examples/RmlUi/`'s authority to verify |
| **R55** | 9.2 | "Unknown decorator/filter function name [...]: that single decorator entry is dropped from its property's list (the *rest* [...] still applies)" | **BLOQUEIA** | **Finding C** |
| R56 | 9.2.1 | Stop grammar `<color>` or `<color>:<position%>`, `:` as intra-stop separator | PASS | This is the dump format's own new grammar, not upstream-derived |
| R57 | 9.2.1 | 4-step auto-spacing algorithm | PASS (internally consistent) | glintfx's own decorator behaviour (per `docs/effects.md`), not an upstream RmlUi mechanism to cross-check |
| R58 | 9.2.1 | §9.2.1's own worked example (`radial-gradient(circle at 35% 30%, #F0D98C, ...)`) | PASS (byte-correct) | All colors in this example are fully opaque (default alpha `ff`) so Finding B's premultiply bug is a no-op here **by coincidence** -- flagged so nobody mistakes this example's correctness for evidence against Finding B |
| R59 | 9.3 | `animation(<name>;<duration>;<timing>;<iterations>;<alternate>;<paused>)` grammar, function-wrap convention | PASS | Dump-format design decision |
| R60 | 9.3 | Duration in bare seconds, no suffix; `ms` not a recognized upstream duration unit | PASS (mostly) | Verified `sscanf(...,"%fs%n",...)` pattern; see Finding I footnote on the actual (surprising) fallback behaviour for a hypothetical `ms` token, informational only, zero-corpus |
| R61 | 9.3 | Iterations: integer or literal `infinite`; `alternate`/`paused`: literal `true`/`false` | PASS | Matches `KeywordType::Infinite` / `animation.alternate = true` structure read directly |
| R62 | 9.3 | Multiple animations join with `\|` in source order | PASS | -- |
| **R63** | 9.3/11 | Malformed single-`<single-animation-value>` in a comma-list: per-entry drop or whole-property abort? | **PROVÁVEL** | **Finding I** |
| R64 | 9.4 | `transform` scope explicitly thin (2 corpus instances), grammar limited to `translate`/`scale`/`rotate` 2D | PASS | Honest, self-limiting; not independently checked against upstream transform parser (out of the doc's own stated verification, appropriately flagged by the doc itself) |
| R65 | 10 | `@font-face` gets no `PROP` lines | PASS | -- |
| R66 | 10 | `@keyframes` existence in scope; keyframe-selector `%` unresolved this wave; `from`/`to` = `0%`/`100%` | PASS | -- |
| R67 | 11 | Unknown property name: declaration dropped, other declarations in the same rule block still apply | PASS | Verified against `ReadProperties`'s per-declaration loop: a name miss never enters a mutation path |
| **R68** | 11 | "Unknown selector form [...]: the whole rule (not just one selector in a comma-list) fails to register" | **BLOQUEIA** | **Finding G** |

*(R11-R68 above already fold in the 7 BLOQUEIA, 3 PROVÁVEL and 1 COSMÉTICO findings by cross-reference;
Finding K, the citation-line drift, is noted separately below since it is not itself a normative-rule
row but a footnote to R40/R52.)*

### Findings, in full (BLOQUEIA)

#### Finding A -- a shorthand's own partial dictionary write survives its final rejection

**Section and citation:** §6.2's errata block and §11's "Malformed shorthand value" bullet, both
claiming: *"the **entire shorthand declaration** is dropped, every longhand it would have targeted
keeps whatever the cascade's next-lower-specificity rule provides, or its §6.1 registry initial value
if none"* -- and §15.2's worked example, printing `body/1 PROP border-top-color=#000000ff` (i.e.
reverted to `black`) for the reversed-order case `#b { border-top: #7A5A2E 1dp; }`.

**The two readings, in bytes:**
- **Reading the doc gives (what it says happens):** `border-top-color=#000000ff`,
  `border-top-width=0.0000px` -- both longhands revert to their registry initial value.
- **What upstream's own code actually does:** `border-top-color=#7a5a2eff`,
  `border-top-width=0.0000px` -- only `-width` reverts; `-color` is **set from the source value**.

**Traced against upstream, not paraphrased:** `PropertySpecification.cpp:429-471`'s `FallThrough`
loop calls `dictionary.SetProperty(items[property_index].property_id, new_property)` **directly on
the caller's dictionary, inside the loop, the moment any item matches** -- there is no staging buffer,
no "commit only if the whole shorthand parses". Tracing `border-top: #7A5A2E 1dp;` (`items[0]` =
`-width`, `items[1]` = `-color`, confirmed at `StyleSheetSpecification.cpp:294`): iteration 1,
`items[0]` (`-width`) fails to parse `"#7A5A2E"` as a length, `FallThrough` continues (`property_index`
advances via the `for`-loop's own increment, which a `continue` does **not** skip in C++ -- `value_index`
stays put). Iteration 2, `items[1]` (`-color`) succeeds parsing the **same still-unclaimed**
`"#7A5A2E"` -- `dictionary.SetProperty(BorderTopColor, ...)` fires **here**, before any failure is
detected. Only *after* the loop does the post-loop guard (`value_index < property_values.size() &&
property_index >= items.size()`) fire and make `ParseShorthandDeclaration` `return false`. The caller
(`StyleSheetParser::ReadProperties`, `:1023`/`:1062`) does nothing on `false` beyond
`Log::Message(Log::LT_WARNING, "Syntax error...")` -- it does **not** roll back the dictionary; it
just moves on to the next declaration. `PropertyDictionary::SetProperty` (`PropertyDictionary.cpp:8`)
is a bare `properties[id] = property;` -- there is no transactional layer anywhere in this call chain
to undo it. **The same bug applies to `border`'s `RecursiveRepeat`** (`:369-380`): the loop iterates
all 4 side-shorthand sub-calls unconditionally (`result &= ParseShorthandDeclaration(...)`, no early
exit), so a reversed-order `border: #7A5A2E 1dp;` sets **all four** `-color` longhands from the source
value while leaving all four `-width` longhands untouched, before the accumulated `result=false` makes
the outer call return `false` -- contradicting §11's own "the whole `border` declaration is dropped"
bullet in the same way.

**Severity: BLOQUEIA.** Side A (real RmlUi) will produce `#7a5a2eff`; a Side B built to the letter of
the spec's current text will produce `#000000ff`. Guaranteed byte divergence, no ULP noise involved.

**Proposed errata text (not applied to `docs/uix-rcss.md` -- for the líder's sign-off):**
> Correct §6.2/§11/§15.2: a rejected `FallThrough`/`Box`/`RecursiveRepeat` shorthand does **not**
> revert every targeted longhand. Upstream's `PropertyDictionary::SetProperty` mutates in place with
> no rollback; every longhand that **already matched successfully before the loop's post-condition
> failure fires** keeps that matched value. Concretely for `border-top`: a token that fails item 0's
> domain and matches item 1 (the reversed order) **sets item 1's longhand from the source**, and only
> the longhand that was never matched (item 0, `-width`) falls back to cascade/initial. Section
> 15.2's worked line for `body/1` must read `border-top-color=#7a5a2eff` /
> `border-top-width=0.0000px`, not `#000000ff` / `0.0000px`. The same correction applies to `border`'s
> `RecursiveRepeat`: each of the 4 side sub-calls independently partial-writes per this same rule
> before the aggregate `result=false` surfaces.

#### Finding B -- box-shadow and gradient-stop colors are structurally premultiplied, not straight

**Section and citation:** §7.1: *"Colors are dumped straight-alpha, not premultiplied [...] this dump
reports the cascade-domain value, so straight alpha is correct and consistent for every color-typed
field, **including** colors nested inside a box-shadow layer or a gradient stop (§9.1/§9.2) -- none of
those get premultiplied before printing either."*

**Traced against upstream, not paraphrased:** `PropertyParserBoxShadow.cpp:72`:
`shadow.color = prop.Get<Colourb>().ToPremultiplied();` -- called at **parse time**, before the value
ever reaches `Style::ComputedValues`. `PropertyParserColorStopList.cpp:47` (the parser behind every
gradient stop in `linear-gradient`/`radial-gradient`, both native RmlUi decorators used by
`decorator`/`mask-image`/`filter`/`backdrop-filter`): `color_stop.color =
p_color.Get<Colourb>().ToPremultiplied();` -- same pattern. This is not merely an incidental call that
a future re-read could reverse: the **struct field's own type** is `ColourbPremultiplied`
(`Include/RmlUi/Core/DecorationTypes.h:9`, `:22`; `using ColourbPremultiplied = Colour<byte, 255,
true>;`, `Types.h:36`). There is no straight-alpha representation of these two fields anywhere
downstream of parsing -- the type system enforces it. `ToPremultiplied()`'s own formula
(`Colour.h:76-82`, no-opacity overload): `new_channel = ColourType((channel * alpha) / 255)` (byte
arithmetic, integer division, truncating), alpha unchanged.

**Byte-exact recomputation of the doc's OWN §9.1 worked example:** source
`box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;`. Layer 1's color
(`#22D3EE`, implicit alpha `ff`=255) premultiplies to itself (`channel*255/255=channel`) -- this is
why the doc's own example doesn't reveal the bug for layer 1 (`#22d3eeff` is correct either way).
Layer 2's color, `#22D3EE26` = R`0x22`(34) G`0xD3`(211) B`0xEE`(238) A`0x26`(38):
- **Reading the doc gives:** `#22d3ee26` (straight, unchanged from source).
- **What upstream's own code actually stores and Side A will read:**
  R=`(34*38)/255=5`(`0x05`), G=`(211*38)/255=31`(`0x1f`), B=`(238*38)/255=35`(`0x23`), A=`38`(`0x26`)
  → **`#051f2326`**.

So the doc's own §9.1 worked example line should read
`box-shadow=#22d3eeff;0.0000px;0.0000px;0.0000px;1.0000px;true|#051f2326;0.0000px;0.0000px;16.0000px;0.0000px;false`,
not the currently-published `...|#22d3ee26;...`. The §9.2.1 gradient-stop worked example
(§15.3/§9.2.1, `radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%)`) happens to be
byte-correct **only because every color in it is fully opaque** (alpha `ff`, where premultiply is a
no-op) -- see R58 in the table above; this is a coincidence of the chosen example, not evidence the
rule is right.

**Severity: BLOQUEIA.** Any corpus fixture with a semi-transparent `box-shadow` color or a
semi-transparent gradient stop (both plausible -- the corpus's own measured 8-digit `#rrggbbaa` hex
form exists per §7.1's own text) will diverge between a spec-faithful Side B (straight) and real
RmlUi's Side A (premultiplied).

**Proposed errata text:**
> Correct §7.1: colors are dumped straight-alpha for **scalar** color-typed properties
> (`background-color`, `border-*-color`, `color`, `image-tint-color`) only. `box-shadow` layer colors
> and every gradient-stop color reached through a native RmlUi decorator (`linear-gradient`,
> `radial-gradient`) are premultiplied **at parse time** by upstream (`PropertyParserBoxShadow.cpp:72`,
> `PropertyParserColorStopList.cpp:47`) and stored as `ColourbPremultiplied` -- a type distinct from
> the straight `Colourb` every other color-typed field uses. A conforming Side A dumper reading
> `Style::ComputedValues` for these two fields must either (a) print the premultiplied bytes as-is
> and this document's contract changes to "straight for scalar colors, premultiplied for box-shadow/
> gradient-stop colors", or (b) un-premultiply (`straight = premultiplied * 255 / alpha`, clamped/
> guarded for `alpha=0`) before printing to keep the uniform straight-alpha rule -- **this is a decision
> for the líder**, not something either dumper author should pick independently, because whichever is
> chosen changes real printed bytes for any fixture with `alpha<255` in these two composite domains.
> Section 9.1's worked example must be corrected to `#051f2326` for layer 2's color if (a) is chosen.

#### Finding C -- a malformed decorator/filter entry drops the *whole* property, not just that entry

**Section and citation:** §11: *"Unknown decorator/filter function name, or a known function given
the wrong argument shape: that single decorator entry is dropped from its property's list (the *rest*
of a comma-separated decorator list, if any, still applies)."*

**Traced against upstream, not paraphrased:** `PropertyParserDecorator::ParseValue`
(`PropertyParserDecorator.cpp:63-131`) loops over the comma-separated decorator-value list
(`decorator_string_list`, split at `:55`) and, on the **first** invalid keyword
(`return false;` at the "Bail out if we have an invalid keyword" comment), unknown decorator type
(`Log::Message(...); return false;` at "Decorator type not found"), or a shorthand-property parse
failure inside one entry (`return false;`), **exits the entire function immediately** -- `property.value`/
`property.unit` are never assigned. The caller (`ParsePropertyDeclaration` for a plain property, which
`decorator`/`mask-image`/`filter`/`backdrop-filter` all are, not shorthands) calls
`dictionary.SetProperty` **only after** a successful `ParseValue` -- so on failure, nothing is written
at all, and the property falls back, in full, to whatever the cascade's next rule (or the §6.1 initial
`none`) provides. There is no partial list surviving; **every** entry in that comma-list is discarded,
including the ones that individually would have parsed fine.

**The two readings, in bytes** (hypothetical fixture `decorator: linear-gradient(90deg, #FF0000 20%,
#00FF00 80%), not-a-real-function(1);` -- second entry unknown):
- **Reading the doc gives:** `decorator=linear-gradient(90.0000;#ff0000ff:20.0000%;#00ff00ff:80.0000%)`
  (the valid `linear-gradient` entry survives, the bad entry alone is dropped).
- **What upstream's own code actually does:** `decorator=none` (the entire property reverts, the valid
  `linear-gradient` entry is discarded along with the invalid one).

**Severity: BLOQUEIA.** This is not a corner case invented for this audit -- it is the literal fail-high
behaviour named in §11 as a canonized, load-bearing policy ("mirrors `polygon()`'s existing... rule
extended to whole *entry*"), and it is backwards.

**Proposed errata text:**
> Correct §11's "Unknown decorator/filter function name" bullet: a malformed entry inside a
> `decorator`/`mask-image`/`filter`/`backdrop-filter` comma-separated (or, for `filter`/
> `backdrop-filter`, space-separated -- see Finding F) function list drops the **entire property**, not
> the single entry -- `PropertyParserDecorator`/`PropertyParserFilter` both return `false` on the first
> bad entry without ever assigning `property.value`, so nothing partial survives. The property's
> printed value is its full §6.1 initial (`none`) or whatever the cascade's next-lower-specificity rule
> provides, exactly the same consequence already correctly stated for a malformed shorthand (§11's
> other bullet) -- these two bullets should use the same wording, not opposite ones.

#### Finding D -- the dump file's own trailing-newline convention is unstated

**Section and citation:** Nowhere in `docs/uix-rcss.md` §3 (file shape) is there a sentence
equivalent to `docs/uix-dom.md`'s own: *"The file always ends with a trailing newline (avoids a
spurious 'no newline at end of file' diff line)"* (`docs/uix-dom.md:71-72`). §3 says escaping is
"reused verbatim, not reinvented" from that sibling document, but the trailing-newline clause lives in
that sibling's own **introductory** material (its section on general file conventions), not inside the
4-rule escaping table §3 explicitly cites -- so it is never actually inherited by name.

**The two readings, in bytes:** for a one-node, one-state toy dump ending in
`body/0 PROP width=auto`, reading A ends the file `...width=auto\n` (EOF), reading B ends it
`...width=auto` (no trailing `\n`, e.g. built via `"\n".join(lines)`). A byte-exact `==` comparator
(the harness's own stated comparison mechanism, §8) sees these as **different files** even when every
printed line is identical.

**What upstream does:** not applicable -- this is a property of *this dump format's own file writer*,
not of RmlUi; there is no upstream citation that resolves it. The only textual anchor is the sibling
document making exactly this choice explicit for a near-identical byte-exact-diff use case.

**Severity: BLOQUEIA.** High-probability divergence: it is the single most common byte-level
disagreement between two independently-written line-oriented file writers in any language (`print`
per line vs. `join` with a separator), and the spec gives literally zero guidance either way.

**Proposed errata text:**
> Add to §3, immediately after the escaping paragraph: "File terminator: the dump file always ends
> with a single trailing newline after the very last `PROP` line of the very last `STATE` block --
> same convention and same justification as `docs/uix-dom.md`'s own file-terminator clause (avoids a
> spurious 'no newline at end of file' diff line). No blank line between `STATE` blocks, none at the
> very start of the file."

#### Finding E -- the `STATE` blocks' own file order is only ever shown, never stated as a rule

**Section and citation:** §4's table lists `none` before `hover-all`, and §15.1's worked example
prints `STATE none` before `STATE hover-all` -- but no sentence in §3 or §4 says *"the file always
emits `STATE none` first"*. The document's own repeatedly-invoked tie-break principle elsewhere
(§3, §6: *"ascending, byte-wise... no locale... reproducible across machines, no argument about which
locale is 'correct'"*) is never invoked for `STATE` order itself.

**The two readings, in bytes:** Reading A (copy the example/table order): file opens `STATE none`.
Reading B (apply the document's own established byte-wise-sort default to this new small enumerable
set, since nothing overrides it here): `"hover-all"` < `"none"` byte-wise (`'h'`(0x68) < `'n'`(0x6e)),
so the file would open `STATE hover-all`. Both readings are internally consistent with something the
document says elsewhere; neither is contradicted by §4's own prose, which frames the table as an
enumeration of a *set* ("enumerate that small, closed, 2-member state space in full"), not an ordered
sequence.

**What upstream does:** not applicable -- state forcing is this dump format's own invention (§4's own
text: "this dump forces each boolean pseudo-class... and enumerates that... space"), not a native
RmlUi concept, so there is no upstream citation to settle it either way.

**Severity: BLOQUEIA.** Two implementers who each internalize the document's own stated preference for
"reproducible, no argument about which convention is right" byte-wise ordering could reasonably land
on opposite `STATE`-block orders precisely *because* they took that preference seriously and applied
it somewhere the document forgot to say it also applies.

**Proposed errata text:**
> Add to §4, after the state-matrix table: "File order of `STATE` blocks is the table's own row
> order above, `none` first: this is a **fixed, prose-declared sequence**, not resolved by the byte-wise
> sort rule §3/§6 use for property and token ordering elsewhere in this document -- that rule applies
> only where explicitly invoked. A future `focus-all`/`active-all` addition appends to the end of this
> same fixed sequence, in the order the table gains the new rows, not by re-sorting the existing ones."

#### Finding F -- `filter`/`backdrop-filter` are space-separated, not comma-separated like `decorator`

**Section and citation:** §9.2's opening sentence: *"All four share `Unit::DECORATOR` or
`Unit::FILTER`'s **identical comma-list-of-functions shape** (`examples/RmlUi/Source/Core/
PropertyParserDecorator.cpp:55`, `StringUtilities::ExpandString(..., ',', '(', ')')` -- the same
parenthesis-aware split `box-shadow` uses)."*

**Traced against upstream, not paraphrased:** `PropertyParserDecorator.cpp:55` (backing `decorator`/
`mask-image`) does indeed split on `,`:
`StringUtilities::ExpandString(decorator_string_list, decorator_string_value, ',', '(', ')');`. But
`filter`/`backdrop-filter` use a **separate class**, `PropertyParserFilter`
(`StyleSheetSpecification.cpp:407-408`: `RegisterProperty(PropertyId::Filter, "filter", ...).AddParser("filter", "filter")`),
whose own `ParseValue` (`PropertyParserFilter.cpp:29-31`) splits on **space**:
`StringUtilities::ExpandString(filter_string_list, filter_string_value, ' ', '(', ')', true);` -- the
function's own preceding comment states this explicitly: *"Filters are declared as `filter:
<filter-value>[ <filter-value> ...]`"* (space between entries, not comma).

**The two readings, in bytes** (fixture `filter: blur(4px) drop-shadow(#000000ff;0;0;4px);`):
- **Reading the doc gives (comma-split, "identical" to decorator):** the whole string has no comma, so
  a Side B built to the letter of §9.2 sees **one** entry, `"blur(4px) drop-shadow(...)"`, fails to
  find a matching decorator/filter type named that whole string, and the entire `filter` property
  fails to parse (per Finding C's real consequence: reverts to `none`).
- **What upstream's own code actually does:** splits on the space between `)` and `drop-shadow`,
  correctly yielding two filter entries, both parsed, both printed: `filter=blur(4.0000px)|drop-shadow(#000000ff;0.0000px;0.0000px;4.0000px)`.

**Severity: BLOQUEIA.** This is not a subtle rounding disagreement -- it is a different split
character, guaranteed to produce a different *count* of parsed entries for any multi-function `filter`/
`backdrop-filter` value, the moment one exists in a real fixture.

**Proposed errata text:**
> Correct §9.2's opening sentence: `decorator`/`mask-image` split their comma-separated function list
> via `PropertyParserDecorator.cpp:55` (`,` delimiter). `filter`/`backdrop-filter` are a **separate**
> parser, `PropertyParserFilter` (`PropertyParserFilter.cpp:29`), which splits on **space**, matching
> real CSS `filter` syntax (`filter: blur(4px) brightness(1.2);`). The two pairs share the same
> per-function `name(args)` shape and the same dump-format output separators (`\|` between functions,
> `;` between args) -- that part of §9.2 is correct -- but they do **not** share the same source-string
> split character, and a Side B parser must dispatch on which of the four properties it is parsing
> before choosing `,` vs. ` ` as the split delimiter.

#### Finding G -- a bad selector in a comma-list drops only itself, not the whole rule

**Section and citation:** §11: *"Unknown selector form [...]: **the whole rule (not just one selector
in a comma-list) fails to register**, and is logged naming the raw selector text and the file/line."*

**Traced against upstream, not paraphrased:** `StyleSheetParser::ConstructNodes`
(`StyleSheetParser.cpp:946-965`) splits a rule's prelude into its comma-separated selector list
(`StringUtilities::ExpandString(selector_list, selectors, ',', '(', ')')`) and then, **per selector**:
```
for (const String& selector : selector_list)
{
    StyleSheetNode* leaf_node = ImportProperties(&root_node, selector, empty_properties, 0);
    if (!leaf_node)
        Log::Message(Log::LT_WARNING, "Invalid selector '%s' encountered.", selector.c_str());
    else if (leaf_node != &root_node)
        leaf_nodes.push_back(leaf_node);
}
```
A failure for one selector (`ImportProperties` returning `nullptr`) logs a warning for **that
selector alone** and the loop **continues** to the next selector in the same comma-list -- it never
returns early, never discards `leaf_nodes` already collected, never touches the rule's own property
declarations. The rule's properties are then applied to every selector that **did** resolve to a leaf
node, regardless of any sibling selector in the same comma-list that failed.

**The two readings, in bytes** (fixture `.valid, [attr-form-not-authorized] { color: red; }` --
`[attr]` selectors are explicitly out-of-subset per `docs/rmlx-subset.md` §6.2):
- **Reading the doc gives:** the whole rule fails to register; no element anywhere gets `color=red`
  from this rule (neither `.valid`-matching elements nor anything else).
- **What upstream's own code actually does:** `.valid`-matching elements **do** get `color=#ff0000ff`
  from this rule; only the `[attr-form-not-authorized]` half of the comma-list is skipped (logged,
  dropped), the other half still registers and cascades normally.

**Severity: BLOQUEIA.** This directly touches the líder's own flagship decision-1 evidence (comma-list
selectors, 15 corpus instances, including the 16-tag UA-stylesheet base rule) -- any future fixture
that mixes an authorized and an unauthorized selector form in the same comma-list will diverge exactly
here, and the UA-stylesheet's own 16-tag rule is precisely the kind of long comma-list where a single
typo'd tag name would silently only lose that one tag under real RmlUi, not the whole rule.

**Proposed errata text:**
> Correct §11's "Unknown selector form" bullet: only the **individual selector** that fails to
> register is dropped -- `StyleSheetParser::ConstructNodes` (`StyleSheetParser.cpp:946-965`) resolves
> each comma-separated selector independently and applies the rule's declarations to every selector
> that *did* resolve, logging (not failing) the ones that didn't. "The whole rule fails" is only true
> for a **non-comma-list** selector that is itself invalid (there is nothing else in the list to fall
> back to) -- for a comma-list, replace "the whole rule (not just one selector in a comma-list) fails
> to register" with "each selector in the list is resolved independently; an unrecognized one is
> logged and dropped, the rest of the comma-list's selectors still receive the rule's declarations."

### Findings, in full (PROVÁVEL)

#### Finding H -- string-domain values can collide with this document's own new structural separators

**Section and citation:** §7's `string` domain rule: *"The raw string content, escaped per §3's
escaping rule, no surrounding quotes"* -- and §3's escaping table is the 4-character set inherited
from `docs/uix-dom.md` (`\`, `\n`, `\r`, `\t`). §9's own opening paragraph justifies choosing `\|`/`;`/
`:` as this document's *new* structural separators specifically *"so that a canonical dump line is
never ambiguous about which comma-role a given byte played in the source."*

**The gap:** `font-family`, `cursor`, and `text-overflow`'s string form are all `string`-domain
top-level `PROP` values (§6.1). None of §3's four escaped characters includes `\|`, `;`, or `:` -- the
three characters this same document introduces as its own structural separators one section later.
A `text-overflow: "a|b";` or a `font-family` string containing a literal `;` or `:` (both legal RCSS
string content) prints byte-for-byte unescaped at the top level, indistinguishable from where the
document's *own* stated goal ("never ambiguous about which comma-role a byte played") would want it
escaped -- because that goal was stated for composite-domain separators, and never explicitly extended
to whether a **plain, non-composite** `PROP` line's string value needs the same defensive escaping.

**Two plausible readings:** Implementer A follows §3's escape table to the letter (only 4 characters,
literal `\|`/`;`/`:` pass through unescaped in `string`-domain output, since §7 never says otherwise).
Implementer B, having read §9's stated *intent*, defensively extends escaping to `\|`/`;`/`:` for any
`string`-domain value, to actually deliver the "never ambiguous" property the document claims for the
format as a whole.

**Severity: PROVÁVEL** (diverges only if a fixture's string content contains one of these three bytes
-- plausible for `font-family` and `text-overflow`, not observed as certain).

**Proposed errata text:**
> Add to §7's `string` domain row: "String-domain values are printed byte-for-byte per §3's 4-rule
> table only -- `\|`, `;`, and `:` (this document's own composite-list, argument, and stop separators)
> are **not** escaped in a plain `PROP` line's string value, even though they are structural inside a
> composite value. A `PROP` line is therefore only safely re-splittable on `=` (once, at the first
> occurrence) plus whatever domain-specific grammar §9 defines for that specific property -- it is not
> a general-purpose delimited record."

#### Finding I -- malformed single-entry list failure for `animation`/`box-shadow` is unstated (and, per Finding C's mechanism, is whole-property-abort)

**Section and citation:** §9.1 and §9.3 never state what happens when one `<single-animation-value>`
in a comma-separated `animation` list, or one shadow layer in a comma-separated `box-shadow` list, is
individually malformed while its siblings are well-formed. §11 only makes an explicit (and, per
Finding C, wrong) claim for `decorator`/`filter`.

**Traced against upstream:** `PropertyParserAnimation::ParseAnimation`
(`PropertyParserAnimation.cpp:111-206`) loops per comma-separated `single_animation_value` and, on
validation failure for one instance (`if (animation.name.empty() || animation.duration <= 0.0f ||
...) return false;`, `:204`), **returns `false` immediately** -- same whole-function-abort pattern as
Finding C, not a per-entry skip. `PropertyParserBoxShadow::ParseValue` similarly `return false`s
on the first malformed `shadow_str` (`:39`, `:78`) without ever reaching `property.unit =
Unit::BOXSHADOWLIST;`. Since these two are the *same* family of failure as Finding C
(single bad function/decorator inside `decorator`/`filter`), this is very likely the same-severity
issue, but is listed as PROVÁVEL rather than BLOQUEIA because, unlike Finding C, the document never
asserts a *specific, contradicted* rule here -- it is silent, and a second implementer could correctly
guess whole-property-abort by generalizing §11's opening sentence ("an unrecognized or invalid
construct is logged and ignored") rather than its one wrong concrete example.

**Severity: PROVÁVEL.**

**Proposed errata text:**
> Add to §9.1 and §9.3: "A malformed single shadow layer / single-animation-value inside a
> comma-separated list aborts the **entire property**, per the same mechanism as a malformed
> shorthand (§11) -- `PropertyParserBoxShadow`/`PropertyParserAnimation` both `return false` on the
> first invalid entry without ever assigning the property's `Variant`, so no partial list is possible.
> This is the *opposite* consequence from what §11 (pre-errata-2) states for `decorator`/`filter` --
> see Finding C's own correction, which brings `decorator`/`filter` into agreement with this rule
> rather than the reverse."

#### Finding J -- `quantize()`'s NaN/±Infinity behaviour is unstated

**Section and citation:** §8's `quantize()` algorithm assumes a finite `float32` input throughout
(`trunc`, `copysign`, fixed-point formatting) and never names what a conforming dumper must print for
a non-finite input.

**Why this matters here:** the document itself repeatedly names hostile/adversarial input as a real
concern for this exact function (§15.4: *"a hostile or adversarial-review-generated input can hit
[the quantization boundary] deliberately"*), and this project's own review culture explicitly practises
mutation/adversarial testing that manufactures edge cases a corpus never would. A plausible source of
a non-finite value in a future wave (not this one, but the function itself has no wave-scoping guard):
an angle conversion or a `line-height`-relative resolution dividing by a zero `font-size` if a future
fixture author sets `font-size: 0px` (currently unmeasured but not out-of-subset, and `font-size` is
explicitly *not* clamped away from zero anywhere in §6.1's table).

**Severity: PROVÁVEL** (not concretely demonstrated as reachable within `RMLX-2`'s own current scope,
but the function's own contract has no textual guard against it, and the two natural implementations
-- printing whatever the platform's fixed-point formatter does with NaN/Inf (typically `"nan"`/`"inf"`,
neither matching `quantize()`'s own stated `[-]D.DDDD` shape) vs. crashing an assertion -- visibly
diverge).

**Proposed errata text:**
> Add to §8, after the algorithm block: "`quantize()` is defined only for finite `x`. A conforming
> dumper must treat a non-finite computed value (`NaN`, `+Inf`, `-Inf`) the same as any other
> internally-detected computation error: do not print it via this algorithm; log it and fall back to
> the property's own initial value, the same consequence §11 already defines for a rejected
> declaration -- this is this document's own general fail-high policy applied to a numeric result
> rather than a parse result."

### COSMÉTICO

#### Finding K -- citation-line drift on the exact line Finding B hinges on

**Section and citation:** §6's own evidentiary-source paragraph cites
`glintfx/src/rml/decorator_ripple.cpp:332-336` and `glintfx/src/rml/decorator_image_tint.cpp:409-411`
(not re-verified in this audit -- those two files are glintfx-authored product code, off-limits per
this task's own independence rule, not upstream). Separately, §7.1's premultiply discussion cites
`PropertyParserBoxShadow.cpp:69` for the `.ToPremultiplied()` call. In the upstream clone checked out
at `examples/RmlUi/` today, that call is at **line 72**, not 69 (a 90-line file; the call sits inside
`else if (parser_color->ParseValue(prop, argument, empty_parameter_map))`).

**Severity: COSMÉTICO** -- the citation still resolves to the right function and the right
statement of fact once read; it does not change any byte output. Worth fixing only because this
document explicitly prizes "read directly, not paraphrased" line-exact citation as part of its own
credibility mechanism (the errata block's own opening claim), and this is the exact line Finding B's
entire argument depends on.

**Proposed correction:** `PropertyParserBoxShadow.cpp:72` (not `:69`); no textual change to the rule
itself, only to the line number.

### Não-determináveis (corpus counts, not upstream behaviour)

The following claims are about `/var/tmp/censo-rcss-qa1/censo.md`, the `UIX-RCSS-CENSUS` scratch
report -- explicitly not checked into the repo, and no longer present on this machine at the time of
this audit (`ls` confirms the directory does not exist). They are **not** ambiguities in the prose
(the sentences are clear) -- they are factual claims this audit has no way to re-verify without either
re-running the census (out of this fatia's scope) or reading implementation source (forbidden by this
task's own independence rule):

1. §6.1's "`max-height`/`max-width` are the only 2 of 72 zero-measured entries" (exact count).
2. §6.2's "`border-top`'s width-then-color order is the corpus's own 100%-measured order" (exact
   ratio/count).
3. §9.1's "124 of 135 single-layer `box-shadow` declarations omit `spread`" (exact ratio).
4. `docs/rmlx-subset.md` §6.1's "15 comma-list selector instances in 8 (or 13) files" -- already
   flagged as internally-inconsistent by that document's own §6.1 correction paragraph; not
   re-measured here.

None of these affect the BLOQUEIA/PROVÁVEL findings above, which rest on upstream *code* behaviour,
not corpus statistics.

---

## Português

### Método

Toda frase normativa do `docs/uix-rcss.md` (regra que manda um implementer fazer algo -- formato,
ordem, arredondamento, unidade, herança, escape, separador, terminador, o que omitir, o que incluir)
foi enumerada seção por seção (tabela acima -- não re-traduzida, é dado técnico), depois testada
contra: *"dois implementers competentes, lendo só esta frase, produzem os mesmos bytes?"* Onde a spec
cita evidência upstream do RmlUi, essa citação foi lida direto em `examples/RmlUi/` -- não confiada na
própria paráfrase da spec -- e onde os próprios exemplos trabalhados da spec dão saída byte-exata, essa
saída foi recomputada de forma independente a partir do algoritmo upstream rastreado. Dois dos sete
achados BLOQUEIA foram achados exatamente assim: recomputando um exemplo trabalhado que a própria spec
já publica como prova própria e chegando numa resposta diferente.

### Regras enumeradas e veredito

A tabela é a mesma acima (seção em inglês) -- identificadores de regra (`R1`-`R68`), citações de
seção, e vereditos são dados técnicos, não traduzidos por convenção do próprio `CLAUDE.md` deste
projeto ("identificadores de código apenas en-intl").

### Achados, na íntegra (BLOQUEIA)

#### Achado A -- a própria escrita parcial de um shorthand no dicionário sobrevive à própria rejeição final

**Seção e citação:** o bloco de errata da seção 6.2 e o bullet "Malformed shorthand value" da seção
11, os dois afirmando: *"a **declaração de shorthand inteira** é descartada, todo longhand que ela
alvejaria fica com o que a regra de próxima-especificidade-menor da cascata fornecer, ou o próprio
valor inicial de registro da seção 6.1 se nenhuma"* -- e o exemplo trabalhado da seção 15.2, imprimindo
`body/1 PROP border-top-color=#000000ff` (revertido pra `black`) pro caso de ordem revertida
`#b { border-top: #7A5A2E 1dp; }`.

**As duas leituras, em bytes:**
- **O que a spec diz que acontece:** `border-top-color=#000000ff`, `border-top-width=0.0000px` -- os
  dois longhands revertem pro próprio valor inicial de registro.
- **O que o próprio código upstream de fato faz:** `border-top-color=#7a5a2eff`,
  `border-top-width=0.0000px` -- só `-width` reverte; `-color` **é setado a partir do valor da fonte**.

**Rastreado contra o upstream, não parafraseado:** o laço `FallThrough` do
`PropertySpecification.cpp:429-471` chama `dictionary.SetProperty(items[property_index].property_id,
new_property)` **direto no dicionário do chamador, dentro do laço, no exato momento em que qualquer
item casa** -- não existe buffer de staging, não existe "commit só se o shorthand inteiro parsear".
Rastreando `border-top: #7A5A2E 1dp;` (`items[0]` = `-width`, `items[1]` = `-color`, confirmado em
`StyleSheetSpecification.cpp:294`): iteração 1, `items[0]` (`-width`) falha ao parsear `"#7A5A2E"`
como comprimento, `FallThrough` continua (`property_index` avança pelo próprio incremento do laço
`for`, que um `continue` em C++ **não** pula -- `value_index` fica parado). Iteração 2, `items[1]`
(`-color`) tem sucesso parseando o **mesmo `"#7A5A2E"` ainda não-reivindicado** -- `dictionary.
SetProperty(BorderTopColor, ...)` dispara **aqui**, antes de qualquer falha ser detectada. Só *depois*
do laço a guarda pós-laço (`value_index < property_values.size() && property_index >= items.size()`)
dispara e faz `ParseShorthandDeclaration` retornar `false`. O chamador
(`StyleSheetParser::ReadProperties`, `:1023`/`:1062`) não faz nada no `false` além de
`Log::Message(Log::LT_WARNING, "Syntax error...")` -- **não** reverte o dicionário; só segue pra
próxima declaração. `PropertyDictionary::SetProperty` (`PropertyDictionary.cpp:8`) é um
`properties[id] = property;` cru -- não existe camada transacional em lugar nenhum desta cadeia de
chamada pra desfazer isso. **O mesmo bug se aplica ao `RecursiveRepeat` de `border`** (`:369-380`): o
laço itera as 4 sub-chamadas de side-shorthand incondicionalmente (`result &=
ParseShorthandDeclaration(...)`, sem saída antecipada), então um `border: #7A5A2E 1dp;` revertido seta
os **quatro** longhands `-color` a partir do valor-fonte enquanto deixa os quatro longhands `-width`
intocados, antes que o `result=false` acumulado faça a chamada externa retornar `false` -- contradizendo
o próprio bullet "a declaração border inteira é descartada" da seção 11 do mesmo jeito.

**Severidade: BLOQUEIA.** O lado A (RmlUi real) vai produzir `#7a5a2eff`; um lado B construído à letra
do texto atual da spec vai produzir `#000000ff`. Divergência de byte garantida, sem ruído de ULP
envolvido.

**Proposta de errata (não aplicada ao `docs/uix-rcss.md` -- pro aval do líder):** ver o texto em inglês
acima (proposta idêntica; identificadores técnicos não traduzidos).

#### Achado B -- cores de box-shadow e de stop de gradiente são estruturalmente pré-multiplicadas, não straight

**Seção e citação:** seção 7.1: *"Cores são dumpadas straight-alpha, não pré-multiplicadas [...] este
dump reporta o valor de domínio-de-cascata, então alpha straight é correto e consistente pra todo
campo tipo-cor, **inclusive** cores aninhadas dentro de uma camada de box-shadow ou um stop de
gradiente (seção 9.1/9.2) -- nenhuma delas é pré-multiplicada antes de imprimir também."*

**Rastreado contra o upstream, não parafraseado:** `PropertyParserBoxShadow.cpp:72`: `shadow.color =
prop.Get<Colourb>().ToPremultiplied();` -- chamado em **tempo de parse**, antes do valor sequer chegar
ao `Style::ComputedValues`. `PropertyParserColorStopList.cpp:47` (o parser por trás de todo stop de
gradiente em `linear-gradient`/`radial-gradient`, os dois decorators nativos do RmlUi usados por
`decorator`/`mask-image`/`filter`/`backdrop-filter`): `color_stop.color =
p_color.Get<Colourb>().ToPremultiplied();` -- mesmo padrão. Isto não é só uma chamada incidental que
uma releitura futura poderia reverter: o **próprio tipo do campo do struct** é `ColourbPremultiplied`
(`Include/RmlUi/Core/DecorationTypes.h:9`, `:22`; `using ColourbPremultiplied = Colour<byte, 255,
true>;`, `Types.h:36`). Não existe representação straight-alpha desses dois campos em lugar nenhum
rio-abaixo do parse -- o sistema de tipos garante isso. A própria fórmula do `ToPremultiplied()`
(`Colour.h:76-82`, overload sem opacidade): `novo_canal = ColourType((canal * alpha) / 255)`
(aritmética byte, divisão inteira, truncando), alpha inalterado.

**Recomputação byte-exata do próprio exemplo trabalhado da seção 9.1 da spec:** fonte
`box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;`. A cor da camada 1
(`#22D3EE`, alpha implícito `ff`=255) pré-multiplica pra ela mesma (`canal*255/255=canal`) -- por isso
o próprio exemplo da spec não revela o bug pra camada 1 (`#22d3eeff` está correto dos dois jeitos). A
cor da camada 2, `#22D3EE26` = R`0x22`(34) G`0xD3`(211) B`0xEE`(238) A`0x26`(38):
- **O que a spec diz:** `#22d3ee26` (straight, sem mudança da fonte).
- **O que o próprio código upstream de fato guarda e o lado A vai ler:**
  R=`(34*38)/255=5`(`0x05`), G=`(211*38)/255=31`(`0x1f`), B=`(238*38)/255=35`(`0x23`), A=`38`(`0x26`)
  → **`#051f2326`**.

Então a própria linha do exemplo trabalhado da seção 9.1 deveria ser
`box-shadow=#22d3eeff;0.0000px;0.0000px;0.0000px;1.0000px;true|#051f2326;0.0000px;0.0000px;16.0000px;0.0000px;false`,
não a atualmente publicada `...|#22d3ee26;...`. O exemplo trabalhado de stop-de-gradiente (seção
15.3/9.2.1, `radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%)`) por acaso é
byte-correto **só porque toda cor nele é totalmente opaca** (alpha `ff`, onde pré-multiplicar é
no-op) -- ver R58 na tabela acima; isso é coincidência do exemplo escolhido, não evidência de que a
regra está certa.

**Severidade: BLOQUEIA.** Qualquer fixture do corpus com uma cor de `box-shadow` semi-transparente ou
um stop de gradiente semi-transparente (os dois plausíveis -- a própria forma hex `#rrggbbaa` de 8
dígitos medida pelo censo existe pelo próprio texto da seção 7.1) vai divergir entre um lado B fiel à
spec (straight) e o lado A do RmlUi real (pré-multiplicado).

**Proposta de errata:** ver o texto em inglês acima -- a decisão entre (a) imprimir os bytes
pré-multiplicados como estão e mudar o contrato do documento, ou (b) despré-multiplicar antes de
imprimir pra manter a regra straight-alpha uniforme, **é decisão do líder**, não algo que qualquer
autor de dumper deveria escolher sozinho, porque qualquer uma das duas muda bytes impressos reais pra
qualquer fixture com `alpha<255` nesses dois domínios compostos.

#### Achado C -- uma entrada malformada de decorator/filter derruba a propriedade INTEIRA, não só aquela entrada

**Seção e citação:** seção 11: *"Nome de função de decorator/filter desconhecido, ou uma função
conhecida com a forma errada de argumento: aquela entrada de decorator específica é descartada da
própria lista da propriedade (o *resto* de uma lista de decorator separada por vírgula, se houver,
continua se aplicando)."*

**Rastreado contra o upstream, não parafraseado:** `PropertyParserDecorator::ParseValue`
(`PropertyParserDecorator.cpp:63-131`) itera a lista de valores-de-decorator separada por vírgula
(`decorator_string_list`, dividida na `:55`) e, na **primeira** keyword inválida (`return false;` no
comentário "Bail out if we have an invalid keyword"), tipo de decorator desconhecido
(`Log::Message(...); return false;` em "Decorator type not found"), ou falha de parse de propriedade
shorthand dentro de uma entrada (`return false;`), **sai da função inteira imediatamente** --
`property.value`/`property.unit` nunca são atribuídos. O chamador (`ParsePropertyDeclaration` pra uma
propriedade plana, que `decorator`/`mask-image`/`filter`/`backdrop-filter` são todas, não shorthands)
só chama `dictionary.SetProperty` **depois** de um `ParseValue` bem-sucedido -- então na falha, nada é
escrito, e a propriedade cai, por completo, pro que a próxima regra da cascata (ou o `none` inicial da
seção 6.1) fornecer. Não sobra lista parcial nenhuma; **toda** entrada daquela lista-vírgula é
descartada, inclusive as que individualmente teriam parseado bem.

**Severidade: BLOQUEIA.** Isto não é um caso de canto inventado pra esta auditoria -- é literalmente o
comportamento fail-high nomeado na seção 11 como política canonizada e load-bearing ("espelha a regra
já existente... do `polygon()`... estendida pra 'entrada inteira'"), e está invertida.

**Proposta de errata:** ver texto em inglês acima -- corrigir o bullet pra dizer que a propriedade
**inteira** é descartada, não a entrada, com o mesmo texto que já está correto no bullet vizinho de
shorthand malformado.

#### Achado D -- a própria convenção de newline final do arquivo de dump está sem declarar

**Seção e citação:** em lugar nenhum da seção 3 do `docs/uix-rcss.md` (forma do arquivo) existe frase
equivalente à do próprio `docs/uix-dom.md`: *"O arquivo sempre termina com newline final (evita uma
linha de diff espúria de 'sem newline no fim do arquivo')"* (`docs/uix-dom.md:71-72`, versão PT
`:534`). A seção 3 diz que o escape é "reusado verbatim, não reinventado" desse irmão, mas a cláusula
de newline-final mora no material **introdutório** daquele irmão (a seção geral de convenções de
arquivo dele), não dentro da tabela de 4 regras de escape que a seção 3 cita explicitamente -- então
nunca é de fato herdada por nome.

**Severidade: BLOQUEIA.** Divergência de alta probabilidade: é o desacordo de byte mais comum entre
dois escritores de arquivo linha-a-linha escritos de forma independente em qualquer linguagem
(`print` por linha vs. `join` com separador), e a spec não dá orientação nenhuma pra nenhum dos dois
lados.

**Proposta de errata:** ver texto em inglês acima.

#### Achado E -- a própria ordem-de-arquivo dos blocos `STATE` só é mostrada, nunca declarada como regra

**Seção e citação:** a tabela da seção 4 lista `none` antes de `hover-all`, e o exemplo trabalhado da
seção 15.1 imprime `STATE none` antes de `STATE hover-all` -- mas nenhuma frase na seção 3 ou 4 diz
*"o arquivo sempre emite `STATE none` primeiro"*. O próprio princípio de desempate repetidamente
invocado pelo documento em outros lugares (seções 3, 6: *"ascendente, byte-a-byte... sem locale...
reproduzível entre máquinas, sem argumento sobre qual locale é 'correto'"*) nunca é invocado pra
própria ordem de `STATE`.

**As duas leituras, em bytes:** Leitura A (copiar a ordem do exemplo/tabela): o arquivo abre com
`STATE none`. Leitura B (aplicar o próprio default de ordenação byte-wise já estabelecido pelo
documento a este novo conjunto pequeno enumerável, já que nada o sobrepõe aqui): `"hover-all"` <
`"none"` byte-wise (`'h'`(0x68) < `'n'`(0x6e)), então o arquivo abriria com `STATE hover-all`. As duas
leituras são internamente consistentes com algo que o documento diz em outro lugar; nenhuma é
contradita pela própria prosa da seção 4, que enquadra a tabela como enumeração de um *conjunto*
("enumere esse espaço de estado pequeno, fechado, de 2 membros, por completo"), não uma sequência
ordenada.

**Severidade: BLOQUEIA.**

**Proposta de errata:** ver texto em inglês acima.

#### Achado F -- `filter`/`backdrop-filter` são separados por espaço, não por vírgula como `decorator`

**Seção e citação:** frase de abertura da seção 9.2: *"As quatro compartilham a forma idêntica de
lista-separada-por-vírgula-de-funções do `Unit::DECORATOR` ou `Unit::FILTER`
(`examples/RmlUi/Source/Core/PropertyParserDecorator.cpp:55`, `StringUtilities::ExpandString(...,
',', '(', ')')` -- o mesmo split parênteses-aware que `box-shadow` usa)."*

**Rastreado contra o upstream, não parafraseado:** `PropertyParserDecorator.cpp:55` (por trás de
`decorator`/`mask-image`) de fato divide por `,`. Mas `filter`/`backdrop-filter` usam uma **classe
separada**, `PropertyParserFilter` (`StyleSheetSpecification.cpp:407-408`), cujo próprio `ParseValue`
(`PropertyParserFilter.cpp:29-31`) divide por **espaço**:
`StringUtilities::ExpandString(filter_string_list, filter_string_value, ' ', '(', ')', true);` -- o
próprio comentário da função declara isso explicitamente: *"Filters are declared as `filter:
<filter-value>[ <filter-value> ...]`"* (espaço entre entradas, não vírgula).

**Severidade: BLOQUEIA.** Não é uma discordância sutil de arredondamento -- é um caractere de split
diferente, garantido a produzir uma *contagem* diferente de entradas parseadas pra qualquer valor
multi-função de `filter`/`backdrop-filter`, no momento em que uma existir numa fixture real.

**Proposta de errata:** ver texto em inglês acima.

#### Achado G -- um seletor ruim numa lista-vírgula derruba só ele mesmo, não a regra inteira

**Seção e citação:** seção 11: *"Forma de seletor desconhecida [...]: **a regra inteira (não só um
seletor de uma lista-vírgula) falha ao registrar**, e é logada nomeando o texto cru do seletor e o
arquivo/linha."*

**Rastreado contra o upstream, não parafraseado:** `StyleSheetParser::ConstructNodes`
(`StyleSheetParser.cpp:946-965`) divide o prelude de uma regra na própria lista de seletores separada
por vírgula e depois, **por seletor**: uma falha pra um seletor loga um aviso pra **aquele seletor
sozinho** e o laço **continua** pro próximo seletor da mesma lista-vírgula -- nunca retorna cedo, nunca
descarta os `leaf_nodes` já coletados, nunca toca as próprias declarações de propriedade da regra. As
propriedades da regra são então aplicadas a todo seletor que **de fato** resolveu num nó-folha,
independente de qualquer seletor irmão da mesma lista-vírgula que falhou.

**Severidade: BLOQUEIA.** Isto toca direto na própria evidência da decisão-1 principal do líder
(seletores com lista-vírgula, 15 instâncias de corpus, incluindo a regra base de 16-tags da
UA-stylesheet) -- qualquer fixture futura que misture uma forma de seletor autorizada e uma não-autorizada
na mesma lista-vírgula vai divergir exatamente aqui, e a própria regra de 16-tags da UA-stylesheet é
exatamente o tipo de lista-vírgula longa onde um único nome de tag com erro de digitação perderia, em
silêncio, só aquela tag sob o RmlUi real, não a regra inteira.

**Proposta de errata:** ver texto em inglês acima.

### Achados, na íntegra (PROVÁVEL)

Ver Achados H (valores string colidem com os próprios separadores estruturais novos deste documento),
I (falha de entrada única malformada em `animation`/`box-shadow` não-declarada, e pelo mecanismo do
Achado C, é aborto-da-propriedade-inteira) e J (comportamento de `quantize()` pra NaN/±Infinity
não-declarado) -- texto completo na seção em inglês acima, propostas de errata idênticas,
identificadores técnicos não traduzidos.

### COSMÉTICO

Ver Achado K (deriva de linha de citação: `PropertyParserBoxShadow.cpp:69` citado, linha real é `72`)
-- texto completo na seção em inglês acima.

### Não-determináveis (contagens de corpus, não comportamento upstream)

As quatro reivindicações listadas na seção em inglês acima (entradas zero-corpus de
`max-height`/`max-width`; ordem 100%-medida de `border-top`; razão de `spread` omitido em
`box-shadow`; contagem de arquivo de seletor-lista-vírgula) dependem do
`/var/tmp/censo-rcss-qa1/censo.md`, que era scratch e não existe mais nesta máquina no momento desta
auditoria (`ls` confirma). Não são ambiguidades de prosa (as frases são claras) -- são reivindicações
factuais que esta auditoria não tem como reverificar sem rerodar o censo (fora do escopo desta fatia)
ou ler código de implementação (proibido pela própria regra de independência desta tarefa). Nenhuma
delas afeta os achados BLOQUEIA/PROVÁVEL acima, que se apoiam em comportamento de *código* upstream,
não em estatística de corpus.
