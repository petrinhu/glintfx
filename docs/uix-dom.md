# UIX DOM dump format / Formato de dump do DOM próprio

> **EN:** The canonical, byte-exact text serialization of a parsed RML document, used as the
> **sole contract** between the two independent differential-oracle dumpers `RMLX-1/S6a` (walks
> the real `Rml::ElementDocument`, confined to `glintfx/src/rml/`) and `RMLX-1/S6b` (walks
> `glintfx`'s own DOM, `src/uix/dom/`), plus the harness that diffs their output (`S7`). Diátaxis
> type: **reference**. Audience: the implementer of `S6a`, the implementer of `S6b` (each reads
> **only** this document, never the other's source -- that separation is deliberate, see
> "Why this document exists" below), and whoever implements `S7`'s diff harness. Owner:
> `software-architect`; written **2026-08-05** against `main` at `84baf37`, before any of
> `RMLX-1`'s slices (parser, tree, `S6a`, `S6b`, `S7`; see
> [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md) for the canonical map) exist -- this
> document is the **first** artifact of the `RMLX-1` wave, not a description of code already
> written.
> **PT:** A serialização textual canônica, byte-exata, de um documento RML já interpretado, usada
> como **único contrato** entre os dois dumpers independentes de oráculo diferencial `RMLX-1/S6a`
> (percorre o `Rml::ElementDocument` real, confinado a `glintfx/src/rml/`) e `RMLX-1/S6b` (percorre
> o DOM próprio da glintfx, `src/uix/dom/`), mais o harness que faz o diff dos dois (`S7`). Tipo
> Diátaxis: **reference**. Audiência: quem implementa a `S6a`, quem implementa a `S6b` (cada um lê
> **só** este documento, nunca o fonte um do outro -- essa separação é deliberada, ver "Por que
> este documento existe" abaixo), e quem implementar o harness de diff da `S7`. Owner:
> `software-architect`; escrito em **2026-08-05** contra `main` em `84baf37`, antes de qualquer uma
> das fatias da `RMLX-1` (parser, árvore, `S6a`, `S6b`, `S7`; ver
> [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md) pro mapa canônico) existir -- este documento
> é o **primeiro** artefato da onda `RMLX-1`, não a descrição de um código já escrito.

**Cross-ref:** [`docs/rmlx-subset.md`](rmlx-subset.md) (o subconjunto congelado que este documento
serve -- este dump nunca captura nada fora dele: sem valores computados, sem geometria, sem
foco/hover, ver seção "Fora do dump" abaixo), [`docs/adr/0020-rml-anticorruption-layer.md`](adr/0020-rml-anticorruption-layer.md)
(a decisão que criou a fronteira RmlUi-vs-glintfx que este dump precisa respeitar), `TODO.md`
linha `RMLX-1` (escopo/aceite da onda).

---

## 🔴 Why this document exists / Por que este documento existe

**EN:** `RMLX-1`'s acceptance is "parity with the real RmlUi tree, proven by a differential
oracle" -- but "parity" is not checkable unless both sides of the comparison speak the exact same
serialization, down to the byte. `S6a` and `S6b` are deliberately written by **different agents**,
each reading **only this document** and never the other implementation: if the same agent wrote
both dumpers, a private assumption shared by both (a whitespace rule invented on the spot, an
attribute order nobody wrote down) would make the oracle agree with itself, not with reality --
green for the wrong reason. Every ambiguity left in this document becomes a **spurious**
divergence at `S7` time, indistinguishable from a real parser bug until someone burns an hour
reading both dumpers' source to find out which one "guessed" differently. This document exists so
that hour is never spent: every field, every ordering, every edge case a real fixture is known to
exercise is decided **here**, in writing, before either dumper is coded.

**PT:** O aceite da `RMLX-1` é "paridade com a árvore real do RmlUi, provada por um oráculo
diferencial" -- mas "paridade" não é checável a menos que os dois lados da comparação falem
exatamente a mesma serialização, até o byte. `S6a` e `S6b` são escritas de propósito por
**agentes diferentes**, cada um lendo **só este documento** e nunca a implementação do outro: se o
mesmo agente escrevesse os dois dumpers, uma suposição privada compartilhada pelos dois (uma regra
de whitespace inventada na hora, uma ordem de atributo que ninguém registrou) faria o oráculo
concordar consigo mesmo, não com a realidade -- verde pelo motivo errado. Toda ambiguidade deixada
neste documento vira uma divergência **espúria** na hora da `S7`, indistinguível de um bug real de
parser até alguém queimar uma hora lendo o fonte dos dois dumpers pra descobrir qual "chutou"
diferente. Este documento existe pra essa hora nunca ser gasta: todo campo, toda ordenação, todo
caso de borda que um fixture real é conhecido por exercitar é decidido **aqui**, por escrito, antes
de qualquer um dos dois dumpers ser codificado.

---

## English

### 1. Format overview

Textual, UTF-8, one **fact** per line, every line self-describing (prefixed by the full path of
the node it describes -- no indentation-carries-meaning, no implicit nesting). A dump is compared
to another dump with a plain string `diff` (`diff a.dump b.dump`); there is no dump parser required
to *use* the oracle, only to *produce* one side of it. The file always ends with a trailing
newline (avoids a spurious "no newline at end of file" diff line). No preamble: the first line is
always the `HEAD` record (section 4), the rest is the `body` subtree in pre-order depth-first
traversal (section 5).

**Why full-path-per-line instead of indentation-nested blocks:** a path-prefixed line is
independently identifiable out of context (`grep '^body/0/2 ' dump.txt` finds every fact about
exactly one node, from any tool, without parsing the file) and a change to one node's own fields
never perturbs a sibling's lines. The cost, accepted knowingly: inserting or removing a sibling
**before** other siblings in the same parent re-numbers every following sibling's whole subtree
path, so their blocks show as fully "changed" in a diff even though their content did not change.
This is judged to be the right trade-off here (positional addressing was the explicit ask, and a
stable synthetic per-node id would be one more thing both dumpers must invent identically, i.e.
one more place for exactly the "private assumption" problem section above is about) -- but it is
recorded as a known cost, not hidden.

### 2. Escaping

One universal rule, applied to every value-bearing field (`HEAD` payload, `ID` value, each `CLASS`
token, each `ATTR` value, `TEXT` content) before it is written to a line -- in this exact order, so
the escape marker itself is never ambiguous:

```
1. '\'  (0x5C)  ->  "\\"   (two chars: backslash, backslash)
2. '\n' (0x0A)  ->  "\n"   (two chars: backslash, n)
3. '\r' (0x0D)  ->  "\r"   (two chars: backslash, r)
4. '\t' (0x09)  ->  "\t"   (two chars: backslash, t)
```

Every other byte, **including the literal ASCII space and every UTF-8 multi-byte sequence**, passes
through unchanged. Space is deliberately never escaped: section 6's whole point is that runs of
space are meaningful data, and escaping them would defeat it. This is the same 4-character set
RmlUi itself treats as whitespace (`StringUtilities::IsWhitespace`, `examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64`
-- `\r`, `\n`, space, `\t`) -- not a coincidence, see section 6.

Unescaping (inverse of the above) is only needed by a human-facing pretty-printer, never by the
oracle itself (which byte-compares two already-escaped dumps). A conforming dumper is not required
to ship one, but if it does, it must implement the exact inverse of the table above and nothing
cleverer.

### 3. Node addressing

The dump has **exactly two top-level records**: `HEAD` (a single opaque line, section 4) and the
`body` subtree (section 5), always in that order (mirrors source order: `<head>` always precedes
`<body>` in every fixture in the corpus). The wrapping `<rml>` tag itself is **not** part of the
dump and has **no path** -- see section 4 for why, evidenced against real RmlUi source.

`body` is the literal root path (no numeric suffix -- it is the one and only node at that
position, not one of several siblings). Every descendant path is `body` followed by one `/<n>`
segment per level, `n` being the node's **0-based position among its parent's surviving children**
-- "surviving" because whitespace-only text nodes are filtered out **before** indices are assigned
(section 6); a filtered node never occupies a slot, so indices are always dense, never sparse.
Example shape (from `TODO.md`'s own framing of this spec): `body/0/2/1` addresses the node that is
the 2nd child of the node that is the 3rd child of `body`'s 1st child.

A document missing `<body>` entirely is a parse error, out of this format's scope (every fixture in
the corpus has one; `XMLNodeHandlerBody::ElementStart`, `examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20`,
shows `<body>`'s attributes are applied directly to the `ElementDocument` when `document == element`
-- i.e. upstream RmlUi's own document root **is** the `<body>` element, not a wrapper around it,
which is exactly why this format roots the tree at `body` and not at some synthetic "document"
node one level further up).

### 4. `HEAD`: a single opaque record, not a subtree -- and why

`<head>`'s children in real RmlUi **never become queryable `Rml::Element` objects at all**. This is
not a simplification this document is choosing for convenience -- it is what the upstream parser
actually does, and matters enormously for `S6a` (which can only dump what it can query from the
real tree it walks):

- `XMLNodeHandlerHead::ElementStart` (`examples/RmlUi/Source/Core/XMLNodeHandlerHead.cpp:43-96`)
  returns `nullptr` for **every** tag name it sees while inside `<head>` -- `head` itself, `link`,
  `script`, and (falling through the `if`/`else if` chain with no matching branch) `style` and
  `title` too. A handler returning `nullptr` from `ElementStart` means the XML parser's frame stack
  entry for that tag carries no new `Element` (`XMLParser::HandleElementStart`,
  `examples/RmlUi/Source/Core/XMLParser.cpp:146,158`: `element = node_handler->ElementStart(...)`,
  then `frame.element = (element ? element : stack.top().element)`).
- All of `<head>`'s actual content -- the document title, the list of external/inline RCSS
  resources, external/inline scripts, template resources -- is instead captured into a
  `DocumentHeader` side-record (`ElementStart`/`ElementData`, same file, lines 47-49, 62-64, 68-70,
  86-91, 119-136) and only reaches the document via `ElementDocument::ProcessHeader` when `</head>`
  closes (`ElementEnd`, lines 98-112). None of it is ever an `Element` a tree-walk can see.
- The wrapping `<rml>` tag fares the same: it matches no registered node handler
  (`XMLParser::RegisterNodeHandler` calls in `examples/RmlUi/Source/Core/Factory.cpp:260-268`
  register only `""` (default), `body`, `head`, `template`, `tabset`, `textarea`, `select` -- never
  `rml`), and the very first call to `HandleElementStart` runs before `active_handler` is ever set
  (`examples/RmlUi/Source/Core/XMLParser.cpp:44` sets it to `nullptr` in the constructor), so
  `node_handler` is `nullptr`, `ElementStart` is never called, and `frame.element` falls back to
  the root element the parser was constructed with (`examples/RmlUi/Source/Core/XMLParser.cpp:158`)
  -- i.e. `<rml>` produces **no** `Element`, exactly like every tag inside `<head>`.

**Consequence for this format:** since `S6a`'s dumper structurally cannot produce per-tag `ELEM`
records for anything inside `<head>` (there is nothing there to walk), this format does not ask it
to. `<head>`'s entire inner content is captured as **one** opaque, unparsed, un-entity-decoded
string -- raw source bytes exactly as written, from immediately after the `>` that closes the
`<head>` open tag, up to (not including) the `<` that opens `</head>`:

```
HEAD ABSENT
HEAD PRESENT <escaped raw source, verbatim, entities NOT decoded>
```

`HEAD ABSENT` is emitted, verbatim, when the source document has no `<head>` element at all (never
omit the `HEAD` line itself in that case -- an absent line proves nothing; an explicit `ABSENT`
proves someone looked, same reasoning as `tools/check_rml_whitelist.sh`'s report-only check always
printing `divida opaca: N arquivos` even at `N=0`, `docs/rmlx-subset.md` section 4). If `<head>`
carries its own attributes (unused anywhere in the corpus today), they are **not** captured --
consistent with `<head>` never being a live `Element` upstream, an implementer who finds a real
fixture that needs them stops and edits this spec first (`docs/rmlx-subset.md`'s own header
clause, section 8 below). Because this payload is not a `TEXT` node in the DOM sense, it is exempt
from section 6's entity-decoding rule -- CSS has no entity syntax, decoding it would corrupt it.

### 5. Per-node record shape (the `body` subtree)

Traversal order for the whole file is **pre-order depth-first**: a node's own facts are emitted in
full before any child's facts, and children are emitted in source order, one child's whole subtree
at a time. Two node shapes exist -- element and text -- each a **contiguous** block of lines, all
sharing the same path prefix.

**Element node**, fields in this fixed order (mirrors the order the brief that requested this spec
named the fields in):

```
<path> ELEM <tag>
<path> ID <id>                          -- OMITTED if id is empty or absent (section 7)
<path> CLASS <c1> <c2> ... <cN>         -- OMITTED if the class set is empty (section 7)
<path> ATTR <name>=<value>              -- one line per attribute, 0..N lines (section 7)
<path> CHILDREN <n>                     -- ALWAYS present, even at n=0
```
...followed by each child's own block (element or text), `n` of them, in order.

**Text node**, exactly one line, no children possible:

```
<path> TEXT <escaped text>
```

`<tag>` is not escaped (RML tag names cannot contain the escaped characters -- they are
angle-bracket-delimited identifiers). `<id>`, each `<c>`, `<name>`, `<value>` and the `TEXT`
payload are all escaped per section 2.

The `body` root itself gets the full element block too (`body ELEM body`, then its own `ID`/
`CLASS`/`ATTR` lines if any, then `CHILDREN`) -- there is no special-cased "root has no header"
shortcut. This is deliberate: a uniform record shape for every node, root included, means neither
dumper implementation needs a root-specific branch, which is one less place for the two
implementations to accidentally diverge.

### 6. 🔴 Whitespace and text-node existence policy (the load-bearing decision)

Two **separate** rules, easy to conflate, evidenced independently against upstream RmlUi:

**(a) Existence filter -- a text node made of nothing but whitespace is never created, full stop.**
`Factory::InstanceElementText` (`examples/RmlUi/Source/Core/Factory.cpp:330-341`): *"If this text
node only contains white-space we don't want to construct it"* -- `only_white_space =
std::all_of(text.begin(), text.end(), &StringUtilities::IsWhitespace)`, and if true, the function
returns without instancing anything. This is not a rendering-time collapse; the node **never
exists** in the live `Rml::Element` tree `S6a` walks. Both dumpers must apply exactly this filter,
using exactly the 4-character `IsWhitespace` set (space, `\t`, `\n`, `\r` -- section 2's escape set,
same characters, same reason) -- **before** assigning child indices, so a run of pure
whitespace between two real siblings simply does not occupy a slot (section 3). Skipping this rule
would not be "more faithful" to the source text; it would make **every single fixture with
pretty-printed indentation** (i.e. all of them) disagree between the two dumpers on child count and
every subsequent index, which is exactly the systemic-divergence failure mode section 9's
escalation threshold exists to catch -- this is the concrete reason that threshold exists.

**(b) Content policy -- once a text node passes the filter, its content is stored, and dumped,
byte-verbatim.** No leading/trailing trim, no internal-run collapse (`"  Hello   world  "` between
two tags is one `TEXT` node whose content is exactly `  Hello   world  `, all spaces intact).
`Factory::InstanceElementText` performs no such collapse either (only the `only_white_space`
short-circuit above touches whitespace; a text node that mixes real content with padding is
instanced with `text` as received from `system_interface->TranslateString`, itself a pass-through
copy on the default `SystemInterface`, not a whitespace transform). This is the CTO's "preserve
verbatim, let layout collapse it" recommendation, and it turns out to already be **exactly**
upstream RmlUi's own behaviour, not just a design preference this document is asserting on its own
authority -- `RMLX-3` (layout) is the wave that may collapse runs of whitespace for *rendering*
purposes; the DOM never does, and this dump never pretends it does.

**(c) Entity decoding is a content-policy detail, not an existence-filter detail.** 🔴
**CORRECTED 2026-08-05, `S6a` finding (verified against `examples/RmlUi/Source/Core/
StringUtilities.cpp:128-218`, `StringUtilities::DecodeRml`, AND against a live run of `S6a`
against the ORIGINAL version of this example, which failed):** RmlUi's `DecodeRml` recognizes
exactly FIVE entity forms -- `&lt;`, `&gt;`, `&amp;`, `&quot;`, and a numeric character reference,
either decimal (`&#160;`) or hex (`&#xA0;`). **`&nbsp;` is NOT one of them** -- RML is XML-like,
not HTML, and XML's own predefined-entity set is `lt`/`gt`/`amp`/`quot`/`apos` (RmlUi implements
the first four; `DecodeRml` has no `apos` branch either, so a literal `&apos;` also passes through
undecoded, though no fixture in this wave's corpus exercises that). A named HTML entity like
`&nbsp;` that is not one of RmlUi's four literal forms falls through `DecodeRml`'s own `& ->
copy-one-byte-and-continue` default case (`StringUtilities.cpp:214-215`) and survives into the
text node **completely undecoded, as its own six literal ASCII bytes** `&nbsp;` -- NOT as U+00A0.
An earlier version of this document's own worked example (section 11) asserted the opposite and
was **wrong**; this correction (and section 11's matching fix) is the artifact of that bug being
caught. The DECODED-byte/escaping mechanics this subsection otherwise describes are correct and
still apply in full to whichever entity form actually decodes -- e.g. a numeric reference like
`&#160;` (U+00A0, non-breaking space) decodes to the two raw UTF-8 bytes `C2 A0` at
text-node-construction time (standard XML character-data handling, upstream of the whitespace
filter above), and THOSE decoded bytes are what gets dumped, escaped per section 2 like any other
content. ⚠️ **U+00A0 (UTF-8 `C2 A0`) is NOT in section 2's escape set** (it is not `\`, `\n`, `\r`
or `\t`) and is **visually indistinguishable from a plain space** in almost every editor, terminal
and `diff` pager. A `TEXT` line that "looks unchanged" in a diff view may still differ at the byte
level if a plain space on one side was an nbsp on the other -- when a `TEXT` line's diff looks like
a no-op, re-check with a byte-exact tool (`diff <(xxd a) <(xxd b)`, `cmp`) before trusting the eye.
This exact class of bug is why the house's own conventions distrust text-extraction/eyeball
verification in general; nbsp-vs-space is the DOM-dump-shaped instance of it -- and, per the
correction above, `&nbsp;`-vs-`&#160;` is now a SECOND instance of the same family: two source
spellings that look interchangeable to a human author and are not.

### 7. Attribute and class ordering (why sorted, why byte-wise)

**Classes are a set, not a list -- `docs/rmlx-subset.md`'s own framing.** The source `class="wide
highlighted"` and `class="highlighted wide"` describe the identical set of two classes; if this
format dumped them in source order, two semantically identical nodes would produce different dump
lines and the oracle would report a divergence that is not one. **Decision:** split the `class`
attribute's raw value on runs of the same 4 whitespace characters as section 6 (space, `\t`, `\n`,
`\r`), drop empty tokens, **deduplicate** (repeated tokens collapse to one -- it is a set), then
**sort ascending, byte-wise** (`std::string::operator<` / `strcmp` over the raw UTF-8 bytes -- no
locale, no Unicode collation table, no case-folding beyond what this section already states).
Empty result (no `class` attribute, or a `class` attribute with only whitespace) omits the `CLASS`
line entirely (section 5).

**Other attributes are sorted by name, byte-wise, same rule, for the same reason** -- an
implementer typing attributes in whatever order feels natural in the markup must not create a
spurious divergence. `id` and `class` are **excluded** from the generic `ATTR` block (they already
have dedicated `ID`/`CLASS` lines) -- a conforming dumper never emits `ATTR id=...` or `ATTR
class=...`.

**Byte-wise, not locale-aware, and this is a deliberate, named choice, not a default left
unexamined:** a locale-sensitive collation (`std::locale`-aware sort, ICU collation) can order the
same bytes differently depending on the machine or environment variables (`LC_COLLATE`) the two
independent dumpers happen to run under -- exactly the kind of environment-dependent divergence
this whole document exists to prevent. Byte-wise ascending is what `std::sort` on `std::string`
does with zero extra code in both implementations, is 100% reproducible across machines, and needs
no argument about which locale is "correct" for the corpus (which is mixed pt-br UI copy and
en-intl identifiers -- there is no single correct locale to pick).

**Empty-value asymmetry, stated explicitly so nobody "fixes" it later:** `id=""` and `id` absent
are treated identically -- no `ID` line either way (an empty id and no id both fail every `#foo`
lookup upstream, there is no behavioural difference to preserve). A **generic** attribute with an
empty value (`data-if=""`) is **not** the same as that attribute being absent -- it still gets an
`ATTR` line with an empty value (`<path> ATTR data-if=`), because for a generic attribute,
"present with empty value" and "absent" are genuinely different, queryable states (`Element::HasAttribute`
returns differently), unlike `id`/`class`.

### 8. Tag-name case, and what this document deliberately does NOT decide

`XMLParser::HandleElementStart` folds every tag name to lowercase before anything else touches it
(`StringUtilities::ToLower(_name)`, `examples/RmlUi/Source/Core/XMLParser.cpp:136`, also line 167
for the closing tag) -- **`<tag>` in an `ELEM` line is always lowercase**, regardless of the case
written in the source. No equivalent lowercasing call was found for attribute **names**, `id`
**values**, `class` **tokens**, or attribute **values** anywhere in `XMLParser.cpp`/`Factory.cpp`
(only tag names and node-handler lookup keys are folded) -- so this format does **not** fold them:
they are dumped exactly as written in the source, case preserved. This is not "no decision was
made" -- absence of evidence for a transform is itself evidence against inventing one; a transform
this document does not name is a transform neither dumper may apply on its own initiative (the
same "frozen unless the spec says so" discipline `docs/rmlx-subset.md`'s own header imposes on
`RMLX-1..11` as a whole, applied here to one format detail).

### 9. Divergence ledger

**Location.** The live table is section 9's own subsection "Ledger table" below, inside this same
file. Appending a row to that table is **routine** (any `S6a`/`S6b`/`S7` implementer does it the
moment a real divergence is found) and is **not** a scope edit under `docs/rmlx-subset.md`'s
sign-off clause -- only a change to sections 1-8 above (the format contract itself) is. This split
is stated explicitly so nobody either (a) blocks on líder sign-off to log a finding, or (b)
assumes editing the ledger is free to also quietly change the format it is measuring against.

**The three classes, and what to do for each:**

- **(a) Our bug.** The dumper (either one) does not implement section 1-8 correctly. Fix the
  dumper. The fixture that exposed it becomes a permanent minimal regression fixture (added to the
  corpus, not discarded once green).
- **(b) RmlUi normalization** -- whitespace handling, entity decoding, an injected node, `<head>`
  treatment, or any other upstream behaviour this document did not anticipate. **Expected, not a
  bug** -- the pre-parse markup and the post-parse tree are never byte-identical for any real HTML/
  XML-like format, RmlUi included. Action: if the corpus actually exercises it, add a **named**
  ledger row with the fixture that proves it, and both dumpers replicate the behaviour (updating
  section 1-8 of this document too, if the behaviour was not already covered here -- this is
  covered by the "routine" carve-out above, not the sign-off clause, because it documents a fact
  about RmlUi, it does not change what this format is trying to represent).
- **(c) Out-of-subset feature.** The divergence traces to a selector/property/markup construct
  `docs/rmlx-subset.md` does not name. **STOP.** This is that document's own clause: edit the spec
  with a diff, get the líder's sign-off, only then implement. Never implement first.

⚠️ **Escalation signal, to be read literally, not rhetorically:** if the ledger's class-(b) rows
pass roughly **10 entries**, treat that as evidence this format document itself is wrong somewhere
-- not as a growing list of quirks to keep patching around. A healthy ledger has a handful of named,
understood RmlUi normalizations (whitespace-only filtering already priced in at section 6, entity
decoding already priced in, `<head>`'s opacity already priced in); a ledger that keeps growing past
that means some *other* systemic gap in sections 1-8 is being rediscovered fixture by fixture
instead of fixed once at the source. Stop adding rows, re-read sections 1-8 against the fixtures
that produced the last several rows, and revise the format -- the same failure mode section 6's
whitespace rule exists specifically to have pre-empted.

**Ledger table.** Empty at the time this document is written (`RMLX-1`'s slices do not exist yet --
see [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md)). Columns:
date found, class (a/b/c), one-line description, fixture that proves it (path, relative to
`glintfx/tests/` or the GusWorld corpus copy), resolution (commit or cross-ref).

| Date | Class | Description | Fixture | Resolution |
| :--- | :---: | :--- | :--- | :--- |
| *(none yet)* | | | | |

### 10. Out of the dump this wave / not this document's job

Restated from the brief, so a reader of only this file has the full boundary without needing
`docs/rmlx-subset.md` open side-by-side: computed style values, layout boxes/geometry, focus state,
hover state. These belong to `RMLX-2` (RCSS/cascade), `RMLX-3` (layout), and `RMLX-5` (events/
focus) respectively -- none of them are DOM facts, and none of them are in scope for `RMLX-1`'s
parser-and-tree work (see [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md) for the slice map).
A future dump format for those waves is **not** an extension of this one by
default; it is a new decision, made the same way this one was (spec first, sign-off, then code),
not silently bolted onto this file.

### 11. Worked example (byte-exact)

🔴 **CORRECTED 2026-08-05 (`S6a` finding):** this example originally used the named HTML entity
`&nbsp;` where it now uses the numeric character reference `&#160;`. Section 6c explains why:
RmlUi's `DecodeRml` does not recognize `&nbsp;` at all (it is not one of the 4 literal forms
`DecodeRml` implements, nor a numeric reference) -- the ORIGINAL version of this example asserted
`&nbsp;` decodes to U+00A0, which a live run of `S6a` against real RmlUi disproved (the text
survived as its own 6 literal ASCII bytes `&nbsp;`, not U+00A0). `&#160;` is the numeric spelling
of the exact same code point and DOES decode correctly -- it preserves this example's whole
teaching point (an invisible byte a `diff` viewer conflates with plain space) without asserting a
falsehood about upstream behaviour.

Source fragment (indentation and line breaks below are the literal, exact source bytes -- this is
the whole point of the example):

```
<rml>
<head>
<style>body{color:white}</style>
</head>
<body>
<div id="panel" class="wide highlighted" data-if="flag" title="Panel">
  <span class="highlighted wide">Hi&#160;there</span>
</div>
</body>
</rml>
```

Full, complete dump this source produces (every line; nothing elided):

```
HEAD PRESENT \n<style>body{color:white}</style>\n
body ELEM body
body CHILDREN 1
body/0 ELEM div
body/0 ID panel
body/0 CLASS highlighted wide
body/0 ATTR data-if=flag
body/0 ATTR title=Panel
body/0 CHILDREN 1
body/0/0 ELEM span
body/0/0 CLASS highlighted wide
body/0/0 CHILDREN 1
body/0/0/0 TEXT Hi<NBSP>there
```

Notes tying each line back to the sections above (delete none of this reasoning when copying the
example elsewhere -- the annotations are the point, not the 13 lines on their own):

- `HEAD PRESENT ...` (section 4): the payload is everything between `<head>`'s closing `>` and
  `</head>`'s opening `<`, i.e. `\n<style>body{color:white}</style>\n` -- literally the two
  characters `\` `n` at each end once escaped (section 2), because the source has a real line break
  right after `<head>` and right before `</head>`. Nothing inside is entity-decoded or otherwise
  touched (section 4's last paragraph) -- the CSS's own `{`/`}`/`:` pass through untouched, not
  because they happen to not collide with the escape set, but because this payload is exempt from
  entity decoding entirely, unlike a real `TEXT` node.
- `body CHILDREN 1`, not 3: the source has three children of `<body>` positionally (the whitespace
  before `<div>`, the `<div>` itself, the whitespace after `</div>`), but the two whitespace-only
  runs (`"\n"` each) are filtered by section 6(a) before indices are assigned -- so the div is
  `body/0`, not `body/1`, and it is the **only** entry. The same filtering removes `<div>`'s own
  leading `"\n  "` and trailing `"\n"` text children, which is why `body/0 CHILDREN 1` too (only the
  `<span>` survives, as `body/0/0`).
- `body/0 CLASS highlighted wide`: source order was `"wide highlighted"`; the dump is sorted
  ascending byte-wise (section 7) regardless -- `h` (0x68) sorts before `w` (0x77).
  `body/0/0 CLASS highlighted wide`: the `<span>`'s source order was already
  `"highlighted wide"` -- same sorted output either way, which is exactly the point of sorting (the
  two nodes' `CLASS` lines are compared by content, not accidentally by whichever source order the
  markup author happened to type).
- `body/0 ATTR data-if=flag` before `body/0 ATTR title=Panel`: sorted by attribute name,
  `data-if` < `title` byte-wise (section 7); `id`/`class` do not reappear here (excluded from the
  generic `ATTR` block, section 7).
- `body/0/0/0 TEXT Hi<NBSP>there`: `&#160;` decodes to U+00A0 (section 6c) -- `&nbsp;` would NOT
  (see this section's own 2026-08-05 correction note above). The literal dump line contains the
  real UTF-8 bytes `C2 A0` at that position, not the four ASCII characters `<`, `N`, `B`, `S`,
  `P`, `>` -- `<NBSP>` here is this document's typographic placeholder for "the invisible byte",
  used because U+00A0 cannot be typed unambiguously in Markdown prose without a reader mistaking
  it for a regular space (the exact confusion section 6c warns a `diff` viewer will also fall
  into). A conforming dumper must never emit the literal string `<NBSP>` -- that would be
  **wrong**, indistinguishable from a literal 6-character attribute glitch; it must emit the two
  raw bytes `0xC2 0xA0`.

---

## Português

### 1. Visão geral do formato

Textual, UTF-8, um **fato** por linha, toda linha se auto-descreve (prefixada pelo caminho
completo do nó que ela descreve -- sem indentação carregando significado, sem aninhamento
implícito). Um dump se compara a outro com um `diff` de string puro (`diff a.dump b.dump`); não
precisa de um parser de dump pra *usar* o oráculo, só pra *produzir* um dos lados dele. O arquivo
sempre termina com newline final (evita uma linha de diff espúria de "sem newline no fim do
arquivo"). Sem preâmbulo: a primeira linha é sempre o registro `HEAD` (seção 4), o resto é a
subárvore de `body` em travessia pré-ordem (seção 5).

**Por que caminho-completo-por-linha em vez de blocos aninhados por indentação:** uma linha
prefixada por caminho é identificável de forma independente fora de contexto
(`grep '^body/0/2 ' dump.txt` acha todo fato sobre exatamente um nó, de qualquer ferramenta, sem
parsear o arquivo) e uma mudança nos próprios campos de um nó nunca perturba as linhas de um irmão.
O custo, aceito conscientemente: inserir ou remover um irmão **antes** de outros irmãos no mesmo
pai renumera o caminho da subárvore inteira de todo irmão seguinte, então os blocos deles aparecem
como totalmente "mudados" num diff mesmo que o conteúdo deles não tenha mudado. Julga-se esse o
trade-off certo aqui (endereçamento posicional foi o pedido explícito, e um id sintético estável
por nó seria só mais uma coisa que os dois dumpers precisariam inventar de forma idêntica, ou seja,
mais um lugar exato pro problema de "suposição privada" da seção acima) -- mas fica registrado como
custo conhecido, não escondido.

### 2. Escaping

Uma regra universal, aplicada a todo campo que carrega valor (payload do `HEAD`, valor do `ID`,
cada token de `CLASS`, cada valor de `ATTR`, conteúdo de `TEXT`) antes de ser escrito numa linha --
nesta ordem exata, pra o próprio marcador de escape nunca ficar ambíguo:

```
1. '\'  (0x5C)  ->  "\\"   (dois chars: barra invertida, barra invertida)
2. '\n' (0x0A)  ->  "\n"   (dois chars: barra invertida, n)
3. '\r' (0x0D)  ->  "\r"   (dois chars: barra invertida, r)
4. '\t' (0x09)  ->  "\t"   (dois chars: barra invertida, t)
```

Todo outro byte, **incluindo o espaço ASCII literal e toda sequência multi-byte UTF-8**, passa sem
mudança. Espaço deliberadamente nunca é escapado: o ponto inteiro da seção 6 é que runs de espaço
são dado significativo, e escapá-los derrotaria isso. É o mesmo conjunto de 4 caracteres que o
próprio RmlUi trata como whitespace (`StringUtilities::IsWhitespace`,
`examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64` -- `\r`, `\n`, espaço, `\t`) -- não é
coincidência, ver seção 6.

Desescapar (o inverso da tabela acima) só é necessário pra um pretty-printer voltado a humano,
nunca pro próprio oráculo (que compara byte-a-byte dois dumps já escapados). Um dumper conforme não
é obrigado a entregar um, mas se entregar, tem que implementar o inverso exato da tabela acima e
nada mais esperto que isso.

### 3. Endereçamento de nó

O dump tem **exatamente dois registros de topo**: `HEAD` (uma linha opaca só, seção 4) e a
subárvore `body` (seção 5), sempre nessa ordem (espelha a ordem da fonte: `<head>` sempre precede
`<body>` em todo fixture do corpus). A própria tag de embrulho `<rml>` **não** faz parte do dump e
**não tem caminho** -- ver seção 4 pro porquê, evidenciado contra o fonte real do RmlUi.

`body` é o caminho-raiz literal (sem sufixo numérico -- é o único nó naquela posição, não um entre
vários irmãos). Todo caminho descendente é `body` seguido de um segmento `/<n>` por nível, `n`
sendo a **posição 0-based do nó entre os filhos sobreviventes do pai** -- "sobreviventes" porque
nós de texto só-whitespace são filtrados **antes** dos índices serem atribuídos (seção 6); um nó
filtrado nunca ocupa um slot, então os índices são sempre densos, nunca esparsos. Formato de
exemplo (na própria formulação do `TODO.md` para esta spec): `body/0/2/1` endereça o nó que é o
2º filho do nó que é o 3º filho do 1º filho de `body`.

Um documento sem `<body>` nenhum é erro de parse, fora do escopo deste formato (todo fixture do
corpus tem um; `XMLNodeHandlerBody::ElementStart`,
`examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20`, mostra que os atributos de `<body>` são
aplicados direto ao `ElementDocument` quando `document == element` -- ou seja, a própria raiz de
documento do RmlUi upstream **é** o elemento `<body>`, não um embrulho ao redor dele, que é
exatamente por que este formato ancora a árvore em `body` e não num nó "documento" sintético um
nível acima).

### 4. `HEAD`: um registro opaco só, não uma subárvore -- e por quê

Os filhos de `<head>` no RmlUi real **nunca viram objetos `Rml::Element` consultáveis**. Isto não é
uma simplificação que este documento escolhe por conveniência -- é o que o parser upstream de fato
faz, e importa muito pra `S6a` (que só consegue dumpar o que consegue consultar da árvore real que
percorre):

- `XMLNodeHandlerHead::ElementStart` (`examples/RmlUi/Source/Core/XMLNodeHandlerHead.cpp:43-96`)
  retorna `nullptr` pra **todo** nome de tag que vê dentro de `<head>` -- o próprio `head`, `link`,
  `script`, e (caindo pela cadeia `if`/`else if` sem ramo casando) `style` e `title` também. Um
  handler que retorna `nullptr` de `ElementStart` significa que a entrada da pilha de frames do
  parser XML pra aquela tag não carrega `Element` novo nenhum
  (`XMLParser::HandleElementStart`, `examples/RmlUi/Source/Core/XMLParser.cpp:146,158`:
  `element = node_handler->ElementStart(...)`, depois
  `frame.element = (element ? element : stack.top().element)`).
- Todo o conteúdo real de `<head>` -- o título do documento, a lista de recursos RCSS externos/
  inline, scripts externos/inline, recursos de template -- é capturado num registro paralelo
  `DocumentHeader` (`ElementStart`/`ElementData`, mesmo arquivo, linhas 47-49, 62-64, 68-70, 86-91,
  119-136) e só chega ao documento via `ElementDocument::ProcessHeader` quando `</head>` fecha
  (`ElementEnd`, linhas 98-112). Nada disso é `Element` nenhum que uma travessia de árvore consiga
  ver.
- A tag de embrulho `<rml>` corre a mesma sorte: não casa com handler de nó nenhum registrado
  (as chamadas de `XMLParser::RegisterNodeHandler` em
  `examples/RmlUi/Source/Core/Factory.cpp:260-268` registram só `""` (default), `body`, `head`,
  `template`, `tabset`, `textarea`, `select` -- nunca `rml`), e a primeira chamada a
  `HandleElementStart` roda antes de `active_handler` sequer ser setado
  (`examples/RmlUi/Source/Core/XMLParser.cpp:44` seta pra `nullptr` no construtor), então
  `node_handler` é `nullptr`, `ElementStart` nunca é chamado, e `frame.element` cai de volta pro
  elemento raiz com que o parser foi construído (`examples/RmlUi/Source/Core/XMLParser.cpp:158`) --
  ou seja, `<rml>` não produz `Element` nenhum, exatamente como toda tag dentro de `<head>`.

**Consequência pra este formato:** já que o dumper da `S6a` estruturalmente não consegue produzir
registros `ELEM` por-tag pra nada dentro de `<head>` (não tem nada lá pra percorrer), este formato
não pede isso dele. Todo o conteúdo interno de `<head>` é capturado como **uma** string opaca,
não-parseada, não-entity-decodificada -- bytes crus da fonte exatamente como escritos, desde
imediatamente após o `>` que fecha a tag de abertura `<head>`, até (sem incluir) o `<` que abre
`</head>`:

```
HEAD ABSENT
HEAD PRESENT <fonte crua escapada, verbatim, entidades NÃO decodificadas>
```

`HEAD ABSENT` é emitido, verbatim, quando o documento-fonte não tem `<head>` nenhum (nunca omitir a
própria linha `HEAD` nesse caso -- linha ausente não prova nada; um `ABSENT` explícito prova que
alguém olhou, mesma lógica do check só-relatório do `tools/check_rml_whitelist.sh` sempre imprimir
`divida opaca: N arquivos` mesmo com `N=0`, `docs/rmlx-subset.md` seção 4). Se `<head>` carrega
atributos próprios (não usado em fixture nenhum do corpus hoje), eles **não** são capturados --
consistente com `<head>` nunca ser `Element` vivo upstream; um implementer que achar um fixture
real que precise deles para, e edita esta spec primeiro (cláusula do próprio cabeçalho do
`docs/rmlx-subset.md`, seção 8 abaixo). Como este payload não é um nó `TEXT` no sentido do DOM, ele
fica isento da regra de decodificação de entidade da seção 6 -- CSS não tem sintaxe de entidade,
decodificar corromperia.

### 5. Formato de registro por nó (a subárvore de `body`)

A ordem de travessia do arquivo inteiro é **pré-ordem, profundidade primeiro**: os próprios fatos
de um nó são emitidos por completo antes de qualquer fato de filho, e os filhos são emitidos na
ordem da fonte, uma subárvore de filho inteira por vez. Existem duas formas de nó -- elemento e
texto -- cada uma um bloco **contíguo** de linhas, todas compartilhando o mesmo prefixo de caminho.

**Nó elemento**, campos nesta ordem fixa (espelha a ordem em que o brief que pediu esta spec nomeou
os campos):

```
<caminho> ELEM <tag>
<caminho> ID <id>                       -- OMITIDO se id vazio ou ausente (seção 7)
<caminho> CLASS <c1> <c2> ... <cN>      -- OMITIDO se o conjunto de classes for vazio (seção 7)
<caminho> ATTR <nome>=<valor>           -- uma linha por atributo, 0..N linhas (seção 7)
<caminho> CHILDREN <n>                  -- SEMPRE presente, mesmo em n=0
```
...seguido do próprio bloco de cada filho (elemento ou texto), `n` deles, em ordem.

**Nó texto**, exatamente uma linha, não pode ter filhos:

```
<caminho> TEXT <texto escapado>
```

`<tag>` não é escapado (nomes de tag RML não podem conter os caracteres escapados -- são
identificadores delimitados por colchete angular). `<id>`, cada `<c>`, `<nome>`, `<valor>` e o
payload de `TEXT` são todos escapados pela seção 2.

A própria raiz `body` recebe o bloco de elemento completo também (`body ELEM body`, depois as
próprias linhas `ID`/`CLASS`/`ATTR` se houver, depois `CHILDREN`) -- não existe atalho especial de
"raiz não tem cabeçalho". Isto é deliberado: uma forma de registro uniforme pra todo nó, raiz
incluída, significa que nenhuma das duas implementações precisa de um ramo específico-de-raiz, que
é um lugar a menos pras duas implementações divergirem sem querer.

### 6. 🔴 Política de whitespace e existência de nó de texto (a decisão que carrega peso)

Duas regras **separadas**, fáceis de confundir, evidenciadas independentemente contra o RmlUi
upstream:

**(a) Filtro de existência -- um nó de texto feito só de whitespace nunca é criado, ponto final.**
`Factory::InstanceElementText` (`examples/RmlUi/Source/Core/Factory.cpp:330-341`): *"If this text
node only contains white-space we don't want to construct it"* -- `only_white_space =
std::all_of(text.begin(), text.end(), &StringUtilities::IsWhitespace)`, e se verdadeiro, a função
retorna sem instanciar nada. Isto não é um colapso em tempo-de-render; o nó **nunca existe** na
árvore `Rml::Element` viva que a `S6a` percorre. Os dois dumpers precisam aplicar exatamente este
filtro, usando exatamente o conjunto de 4 caracteres de `IsWhitespace` (espaço, `\t`, `\n`, `\r` --
o mesmo conjunto de escape da seção 2, mesmos caracteres, mesmo motivo) -- **antes** de atribuir
índices de filho, então um trecho de whitespace puro entre dois irmãos reais simplesmente não ocupa
um slot (seção 3). Pular esta regra não seria "mais fiel" ao texto-fonte; faria **todo fixture com
indentação pretty-printed** (ou seja, todos eles) discordar entre os dois dumpers em contagem de
filhos e todo índice seguinte, que é exatamente o modo de falha de divergência-sistêmica que o
limiar de escalonamento da seção 9 existe pra pegar -- este é o motivo concreto de esse limiar
existir.

**(b) Política de conteúdo -- uma vez que um nó de texto passa pelo filtro, seu conteúdo é
armazenado, e dumpado, byte-verbatim.** Sem trim de início/fim, sem colapso de run interno
(`"  Hello   world  "` entre duas tags é UM nó `TEXT` cujo conteúdo é exatamente
`  Hello   world  `, todos os espaços intactos). `Factory::InstanceElementText` também não faz esse
colapso (só o atalho `only_white_space` acima toca whitespace; um nó de texto que mistura conteúdo
real com padding é instanciado com `text` como recebido de `system_interface->TranslateString`, ele
mesmo uma cópia pass-through no `SystemInterface` default, não uma transformação de whitespace).
Esta é a recomendação do CTO de "preservar verbatim, deixar o layout colapsar" -- e acontece de já
ser **exatamente** o comportamento do próprio RmlUi upstream, não só uma preferência de design que
este documento afirma por autoridade própria -- a `RMLX-3` (layout) é a onda que pode colapsar runs
de whitespace pra fins de *renderização*; o DOM nunca faz isso, e este dump nunca finge que faz.

**(c) Decodificação de entidade é um detalhe de política de conteúdo, não de filtro de
existência.** 🔴 **CORRIGIDO em 2026-08-05, achado da `S6a` (verificado contra
`examples/RmlUi/Source/Core/StringUtilities.cpp:128-218`, `StringUtilities::DecodeRml`, E contra
uma rodada ao vivo da `S6a` contra a versão ORIGINAL deste exemplo, que falhou):** o `DecodeRml`
do RmlUi reconhece exatamente CINCO formas de entidade -- `&lt;`, `&gt;`, `&amp;`, `&quot;`, e uma
referência numérica de caractere, decimal (`&#160;`) ou hex (`&#xA0;`). **`&nbsp;` NÃO é uma
delas** -- o RML é estilo-XML, não HTML, e o próprio conjunto de entidades predefinidas do XML é
`lt`/`gt`/`amp`/`quot`/`apos` (o RmlUi implementa as quatro primeiras; o `DecodeRml` também não
tem ramo pra `apos`, então um `&apos;` literal também passa sem decodificar, embora nenhuma
fixture do corpus desta onda exercite isso). Uma entidade HTML nomeada como `&nbsp;`, que não é
uma das quatro formas literais do RmlUi, cai no caso default do próprio `DecodeRml` (`& ->
copia-um-byte-e-continua`, `StringUtilities.cpp:214-215`) e sobrevive no nó de texto
**completamente não-decodificada, como seus próprios seis bytes ASCII literais** `&nbsp;` -- NÃO
como U+00A0. Uma versão anterior do próprio exemplo trabalhado deste documento (seção 11) afirmava
o oposto e estava **errada**; esta correção (e o conserto correspondente da seção 11) é o
artefato desse bug tendo sido pego. A mecânica de byte-decodificado/escape que esta subseção
descreve fora isso está correta e continua valendo por completo pra qualquer forma de entidade que
de fato decodifique -- ex.: uma referência numérica como `&#160;` (U+00A0, espaço
não-quebrável) decodifica pros dois bytes UTF-8 crus `C2 A0` no momento de construção do nó de
texto (tratamento padrão de character-data XML, anterior ao filtro de whitespace acima), e ESSES
bytes decodificados são o que vai pro dump, escapado pela seção 2 como qualquer outro conteúdo.
⚠️ **U+00A0 (UTF-8 `C2 A0`) NÃO está no conjunto de escape da seção 2** (não é `\`, `\n`, `\r`
nem `\t`) e é **visualmente indistinguível de um espaço comum** em quase todo editor, terminal e
pager de `diff`. Uma linha `TEXT` que "parece sem mudança" numa view de diff ainda pode diferir no
nível de byte se um espaço comum de um lado era um nbsp do outro -- quando o diff de uma linha
`TEXT` parece um no-op, reconferir com ferramenta byte-exata (`diff <(xxd a) <(xxd b)`, `cmp`)
antes de confiar no olho. Esta classe exata de bug é por que as convenções da própria casa
desconfiam de extração-de-texto/verificação a olho em geral; nbsp-vs-espaço é a instância
no-formato-de-dump-de-DOM disso -- e, pela correção acima, `&nbsp;`-vs-`&#160;` agora é uma
SEGUNDA instância da mesma família: duas grafias-fonte que parecem intercambiáveis pra um autor
humano e não são.

### 7. Ordenação de atributo e classe (por que ordenado, por que byte-a-byte)

**Classes são um conjunto, não uma lista -- a própria formulação do `docs/rmlx-subset.md`.** A
fonte `class="wide highlighted"` e `class="highlighted wide"` descrevem o mesmo conjunto idêntico
de duas classes; se este formato dumpasse na ordem da fonte, dois nós semanticamente idênticos
produziriam linhas de dump diferentes e o oráculo reportaria uma divergência que não é uma.
**Decisão:** dividir o valor cru do atributo `class` em runs dos mesmos 4 caracteres de whitespace
da seção 6 (espaço, `\t`, `\n`, `\r`), descartar tokens vazios, **deduplicar** (tokens repetidos
colapsam pra um só -- é um conjunto), depois **ordenar ascendente, byte-a-byte**
(`std::string::operator<` / `strcmp` sobre os bytes UTF-8 crus -- sem locale, sem tabela de
colação Unicode, sem case-folding além do que esta seção já declara). Resultado vazio (sem
atributo `class`, ou um `class` só com whitespace) omite a linha `CLASS` inteira (seção 5).

**Outros atributos são ordenados por nome, byte-a-byte, mesma regra, pelo mesmo motivo** -- um
implementer digitando atributos em qualquer ordem que pareça natural no markup não pode criar uma
divergência espúria. `id` e `class` são **excluídos** do bloco genérico `ATTR` (já têm linhas
`ID`/`CLASS` dedicadas) -- um dumper conforme nunca emite `ATTR id=...` nem `ATTR class=...`.

**Byte-a-byte, não sensível a locale, e isto é escolha deliberada e nomeada, não um default
deixado sem exame:** uma colação sensível a locale (sort `std::locale`-aware, colação ICU) pode
ordenar os mesmos bytes de forma diferente dependendo da máquina ou das variáveis de ambiente
(`LC_COLLATE`) sob as quais os dois dumpers independentes rodarem -- exatamente o tipo de
divergência dependente-de-ambiente que este documento inteiro existe pra prevenir. Ascendente
byte-a-byte é o que `std::sort` sobre `std::string` faz com zero código extra nas duas
implementações, é 100% reproduzível entre máquinas, e não precisa de argumento sobre qual locale é
"correto" pro corpus (que é texto de UI pt-br misturado com identificadores en-intl -- não há um
único locale correto pra escolher).

**Assimetria de valor-vazio, declarada explicitamente pra ninguém "consertar" depois:** `id=""` e
`id` ausente são tratados de forma idêntica -- sem linha `ID` de nenhum jeito (um id vazio e sem id
falham igualmente toda busca `#foo` upstream, não há diferença comportamental a preservar). Um
atributo **genérico** com valor vazio (`data-if=""`) **não é** o mesmo que esse atributo estar
ausente -- ainda ganha uma linha `ATTR` com valor vazio (`<caminho> ATTR data-if=`), porque pra um
atributo genérico, "presente com valor vazio" e "ausente" são estados genuinamente diferentes e
consultáveis (`Element::HasAttribute` retorna diferente), diferente de `id`/`class`.

### 8. Caixa de nome de tag, e o que este documento deliberadamente NÃO decide

`XMLParser::HandleElementStart` dobra todo nome de tag pra minúsculas antes de qualquer outra coisa
tocá-lo (`StringUtilities::ToLower(_name)`, `examples/RmlUi/Source/Core/XMLParser.cpp:136`,
também linha 167 pra tag de fechamento) -- **`<tag>` numa linha `ELEM` é sempre minúscula**,
independente da caixa escrita na fonte. Nenhuma chamada equivalente de minúsculas foi achada pra
**nomes** de atributo, **valores** de `id`, tokens de `class`, ou **valores** de atributo em lugar
nenhum de `XMLParser.cpp`/`Factory.cpp` (só nomes de tag e chaves de lookup de node-handler são
dobrados) -- então este formato **não** os dobra: são dumpados exatamente como escritos na fonte,
caixa preservada. Isto não é "nenhuma decisão foi tomada" -- ausência de evidência pra uma
transformação é ela mesma evidência CONTRA inventar uma; uma transformação que este documento não
nomeia é uma transformação que nenhum dos dois dumpers pode aplicar por iniciativa própria (a mesma
disciplina de "congelado a menos que a spec diga" que o próprio cabeçalho do `docs/rmlx-subset.md`
impõe pra `RMLX-1..11` como um todo, aplicada aqui a um detalhe de formato).

### 9. Registro de divergências (divergence ledger)

**Localização.** A tabela viva é a própria subseção "Tabela do ledger" da seção 9, abaixo, dentro
deste mesmo arquivo. Somar uma linha nessa tabela é **rotina** (qualquer implementer de
`S6a`/`S6b`/`S7` faz isso no momento em que acha uma divergência real) e **não é** uma edição de
escopo sob a cláusula de aval do `docs/rmlx-subset.md` -- só uma mudança nas seções 1-8 acima (o
próprio contrato de formato) é. Esta separação é declarada explicitamente pra ninguém (a) travar
esperando aval do líder pra registrar um achado, nem (b) supor que editar o ledger é livre pra
também mudar em silêncio o formato que ele está medindo.

**As três classes, e o que fazer em cada uma:**

- **(a) Bug nosso.** O dumper (qualquer um dos dois) não implementa as seções 1-8 corretamente.
  Conserta o dumper. O fixture que expôs vira fixture de regressão mínima permanente (somado ao
  corpus, não descartado assim que fica verde).
- **(b) Normalização do RmlUi** -- tratamento de whitespace, decodificação de entidade, um nó
  injetado, tratamento de `<head>`, ou qualquer outro comportamento upstream que este documento não
  antecipou. **Esperado, não é bug** -- o markup pré-parse e a árvore pós-parse nunca são
  byte-idênticos pra formato real nenhum estilo HTML/XML, RmlUi incluído. Ação: se o corpus de fato
  exercita, somar uma linha **nomeada** no ledger com o fixture que prova, e os dois dumpers
  replicam o comportamento (atualizando as seções 1-8 deste documento também, se o comportamento
  ainda não estava coberto aqui -- isso está coberto pelo carve-out de "rotina" acima, não pela
  cláusula de aval, porque documenta um fato sobre o RmlUi, não muda o que este formato está
  tentando representar).
- **(c) Recurso fora do subconjunto.** A divergência remonta a um seletor/propriedade/construção de
  markup que o `docs/rmlx-subset.md` não nomeia. **PARAR.** Esta é a própria cláusula daquele
  documento: editar a spec com um diff, pegar o aval do líder, só então implementar. Nunca
  implementar primeiro.

⚠️ **Sinal de escalonamento, pra ser lido literalmente, não retoricamente:** se as linhas classe-(b)
do ledger passarem de aproximadamente **10 entradas**, tratar isso como evidência de que este
próprio documento de formato está errado em algum lugar -- não como uma lista crescente de
peculiaridades pra ficar remendando. Um ledger saudável tem um punhado de normalizações do RmlUi
nomeadas e entendidas (filtro só-whitespace já precificado na seção 6, decodificação de entidade já
precificada, opacidade do `<head>` já precificada); um ledger que continua crescendo além disso
significa que alguma *outra* lacuna sistêmica nas seções 1-8 está sendo redescoberta fixture por
fixture em vez de consertada uma vez na fonte. Parar de somar linhas, reler as seções 1-8 contra os
fixtures que produziram as últimas entradas, e revisar o formato -- o mesmo modo de falha que a
regra de whitespace da seção 6 existe especificamente pra ter prevenido.

**Tabela do ledger.** Vazia no momento em que este documento é escrito (as fatias da `RMLX-1`
ainda não existem -- ver [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md)).
Colunas: data do achado, classe (a/b/c), descrição de uma linha, fixture que prova (caminho,
relativo a `glintfx/tests/` ou à cópia do corpus GusWorld), resolução (commit ou cross-ref).

| Data | Classe | Descrição | Fixture | Resolução |
| :--- | :---: | :--- | :--- | :--- |
| *(nenhuma ainda)* | | | | |

### 10. Fora do dump nesta onda / não é trabalho deste documento

Restated do brief, pra quem lê só este arquivo ter a fronteira completa sem precisar abrir o
`docs/rmlx-subset.md` ao lado: valores computados de estilo, caixas/geometria de layout, estado de
foco, estado de hover. Pertencem à `RMLX-2` (RCSS/cascata), `RMLX-3` (layout) e `RMLX-5`
(eventos/foco) respectivamente -- nenhum deles é fato de DOM, e nenhum está no escopo do trabalho
de parser-e-árvore da `RMLX-1` (ver [`docs/rmlx1-mapa-fatias.md`](rmlx1-mapa-fatias.md) pro mapa
de fatias). Um futuro formato de dump pra essas ondas **não** é uma extensão
deste por padrão; é uma decisão nova, tomada do mesmo jeito que esta foi (spec primeiro, aval,
só então código), não aparafusada em silêncio neste arquivo.

### 11. Exemplo trabalhado (byte-exato)

🔴 **CORRIGIDO em 2026-08-05 (achado da `S6a`):** este exemplo originalmente usava a entidade
HTML nomeada `&nbsp;` onde agora usa a referência numérica de caractere `&#160;`. A seção 6c
explica o motivo: o `DecodeRml` do RmlUi não reconhece `&nbsp;` de jeito nenhum (não é uma das 4
formas literais que o `DecodeRml` implementa, nem uma referência numérica) -- a versão ORIGINAL
deste exemplo afirmava que `&nbsp;` decodifica pra U+00A0, o que uma rodada ao vivo da `S6a`
contra o RmlUi real desmentiu (o texto sobreviveu como seus próprios 6 bytes ASCII literais
`&nbsp;`, não U+00A0). `&#160;` é a grafia numérica do EXATO mesmo ponto de código e DECODIFICA
corretamente -- preserva o ponto pedagógico inteiro deste exemplo (um byte invisível que um
visualizador de `diff` confunde com espaço comum) sem afirmar uma falsidade sobre o comportamento
upstream.

Fragmento-fonte (a indentação e as quebras de linha abaixo são os bytes-fonte literais, exatos --
este é o ponto inteiro do exemplo):

```
<rml>
<head>
<style>body{color:white}</style>
</head>
<body>
<div id="panel" class="wide highlighted" data-if="flag" title="Panel">
  <span class="highlighted wide">Hi&#160;there</span>
</div>
</body>
</rml>
```

Dump completo que esta fonte produz (toda linha; nada omitido):

```
HEAD PRESENT \n<style>body{color:white}</style>\n
body ELEM body
body CHILDREN 1
body/0 ELEM div
body/0 ID panel
body/0 CLASS highlighted wide
body/0 ATTR data-if=flag
body/0 ATTR title=Panel
body/0 CHILDREN 1
body/0/0 ELEM span
body/0/0 CLASS highlighted wide
body/0/0 CHILDREN 1
body/0/0/0 TEXT Hi<NBSP>there
```

Notas amarrando cada linha de volta às seções acima (não apagar este raciocínio ao copiar o
exemplo pra outro lugar -- as anotações são o ponto, não as 13 linhas sozinhas):

- `HEAD PRESENT ...` (seção 4): o payload é tudo entre o `>` de fechamento de `<head>` e o `<` de
  abertura de `</head>`, ou seja `\n<style>body{color:white}</style>\n` -- literalmente os dois
  caracteres `\` `n` em cada ponta uma vez escapado (seção 2), porque a fonte tem uma quebra de
  linha real logo após `<head>` e logo antes de `</head>`. Nada dentro é entity-decodificado ou
  tocado de outra forma (último parágrafo da seção 4) -- o `{`/`}`/`:` do próprio CSS passam
  intocados, não porque por acaso não colidem com o conjunto de escape, mas porque este payload é
  isento de decodificação de entidade por completo, diferente de um nó `TEXT` de verdade.
- `body CHILDREN 1`, não 3: a fonte tem três filhos de `<body>` posicionalmente (o whitespace antes
  de `<div>`, o `<div>` em si, o whitespace depois de `</div>`), mas os dois runs só-whitespace
  (`"\n"` cada) são filtrados pela seção 6(a) antes dos índices serem atribuídos -- então o div é
  `body/0`, não `body/1`, e é a **única** entrada. O mesmo filtro remove os próprios filhos de
  texto inicial `"\n  "` e final `"\n"` de `<div>`, por isso `body/0 CHILDREN 1` também (só o
  `<span>` sobrevive, como `body/0/0`).
- `body/0 CLASS highlighted wide`: a ordem-fonte era `"wide highlighted"`; o dump é ordenado
  ascendente byte-a-byte (seção 7) independente disso -- `h` (0x68) ordena antes de `w` (0x77).
  `body/0/0 CLASS highlighted wide`: a ordem-fonte do `<span>` já era `"highlighted wide"` -- mesma
  saída ordenada dos dois jeitos, que é exatamente o ponto de ordenar (as linhas `CLASS` dos dois
  nós são comparadas por conteúdo, não acidentalmente por qual ordem-fonte o autor do markup por
  acaso digitou).
- `body/0 ATTR data-if=flag` antes de `body/0 ATTR title=Panel`: ordenado por nome de atributo,
  `data-if` < `title` byte-a-byte (seção 7); `id`/`class` não reaparecem aqui (excluídos do bloco
  genérico `ATTR`, seção 7).
- `body/0/0/0 TEXT Hi<NBSP>there`: `&#160;` decodifica pra U+00A0 (seção 6c) -- `&nbsp;` NÃO
  decodificaria (ver a nota de correção de 2026-08-05 desta seção acima). A linha de dump
  literal contém os bytes UTF-8 reais `C2 A0` naquela posição, não os seis caracteres ASCII `<`,
  `N`, `B`, `S`, `P`, `>` -- `<NBSP>` aqui é o placeholder tipográfico deste documento pro "byte
  invisível", usado porque U+00A0 não pode ser digitado sem ambiguidade em prosa Markdown sem um
  leitor confundir com um espaço comum (a mesma confusão que a seção 6c avisa que um visualizador
  de `diff` também vai cair). Um dumper conforme nunca deve emitir a string literal `<NBSP>` -- isso
  seria **errado**, indistinguível de um glitch literal de atributo de 6 caracteres; tem que emitir
  os dois bytes crus `0xC2 0xA0`.
