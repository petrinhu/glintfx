# UIX RCSS corpus census -- in-repo, re-derivable / Censo de corpus RCSS -- no repositório, re-derivável

> **EN:** This document replaces `/var/tmp/censo-rcss-qa1/censo.md`, the scratch report `docs/uix-rcss.md`
> cites **15** times and `docs/rmlx-subset.md` cites **2** more times (**17** citations total, both
> docs, all to the same now-deleted path) as the authority behind `RMLX-2`'s property registry, unit
> parity, selector-form, and color-scope decisions -- including several **absence** claims ("zero
> occurrences", "zero-measured") that nobody could re-verify without the report. The scratch file lived
> on `/var/tmp` (tmpfs/disk-scratch, never checked in) and is gone. **The raw data survived**: every
> `.rcss` file and every `<style>` block inside every `.rml` file this census measures is git-tracked
> and still on disk. This document is that census, regenerated from the surviving corpus by a real
> parser (`tools/rcss_census.py`, cited throughout, re-runnable any time the corpus changes), bilingual,
> and checked into the repository so it cannot disappear from a scratch directory again.
> Diátaxis type: **reference**. Owner: `qa-engineer`. Written **2026-08-06** against `main`, corpus
> scope = every git-tracked `*.rcss` and `*.rml` file at that commit.
>
> **PT:** Este documento substitui o `/var/tmp/censo-rcss-qa1/censo.md`, o relatório scratch que o
> `docs/uix-rcss.md` cita **15** vezes e o `docs/rmlx-subset.md` cita mais **2** vezes (**17** citações
> no total, nos dois documentos, todas pro mesmo caminho hoje apagado) como autoridade por trás das
> decisões de registro de propriedade, paridade de unidade, forma de seletor e escopo de cor da
> `RMLX-2` -- incluindo várias afirmações de **ausência** ("zero ocorrências", "zero medição") que
> ninguém conseguia reverificar sem o relatório. O arquivo scratch morava em `/var/tmp` (tmpfs/disco
> descartável, nunca versionado) e sumiu. **O dado bruto sobreviveu**: todo arquivo `.rcss` e todo
> bloco `<style>` dentro de todo `.rml` que este censo mede está rastreado pelo git e ainda em disco.
> Este documento é esse censo, regerado do corpus sobrevivente por um parser de verdade
> (`tools/rcss_census.py`, citado o documento inteiro, re-executável sempre que o corpus mudar),
> bilíngue, e versionado no repositório pra não poder sumir de um diretório scratch de novo.
> Tipo Diátaxis: **reference**. Owner: `qa-engineer`. Escrito em **2026-08-06** contra `main`, escopo
> de corpus = todo arquivo `*.rcss` e `*.rml` rastreado pelo git naquele commit.

**Cross-ref:** [`docs/uix-rcss.md`](uix-rcss.md) (the 15 citations this document answers),
[`docs/rmlx-subset.md`](rmlx-subset.md) (the 2 more citations, and the document that already carries a
**partially corrected, durable copy** of parts of the original census -- section 6 there, cross-checked
against section 2 of this document below), `tools/rcss_census.py` (the rederivation script this
document's every number comes from -- run it again rather than hand-editing a table here).

---

## SCOPE (printed once, denominator for everything below) / ESCOPO (impresso uma vez, denominador de tudo abaixo)

```
SCOPE: 62 .rml + 23 .rcss versionados (git ls-files) = 85 arquivos-fonte
       36 dos 62 .rml carregam >=1 bloco <style> (59 fontes RCSS no total: 23 .rcss + 36 inline)
       850 blocos de estilo de topo (837 regras + 19 @font-face + 4 @keyframes),
       9 sub-blocos de @keyframes (from/to/percentagem)
       3388 declaracoes (3339 em regras + 40 em @font-face + 9 em keyframes)
       0 arquivos ilegiveis, 0 sobra nao-branco fora de bloco (leftover_nonblank vazio)
```

**EN:** Every number in this document derives from this scope, reproduced by
`python3 tools/rcss_census.py --repo-root .`. The `0 arquivos ilegíveis` / `0 sobra` lines are printed
on purpose, not omitted: they are the "zero declared, not zero looked" discipline this task's own
brief requires -- an absent line proves nothing, this line proves the scanner read every byte of every
file and had nothing left uncounted.

**PT:** Todo número deste documento deriva deste escopo, reproduzido por
`python3 tools/rcss_census.py --repo-root .`. As linhas `0 arquivos ilegíveis` / `0 sobra` são
impressas de propósito, não omitidas: são a disciplina "zero declarado, não zero olhado" que o próprio
brief desta tarefa exige -- linha ausente não prova nada, esta linha prova que o scanner leu todo byte
de todo arquivo e não sobrou nada sem contar.

---

## 0. The 17 citations, listed before measuring / As 17 citações, listadas antes de medir

**EN:** Per this task's own instruction, the citations were enumerated **before** any measurement, so
they set what this census must contain, not the other way around. 15 in `docs/uix-rcss.md` (8 distinct
claims, stated once in English and mirrored in Portuguese -- except the decorator-grammar claim, whose
Portuguese section explicitly defers to the English one, "mesmo conteúdo técnico", rather than
repeating the path a 16th time), 2 in `docs/rmlx-subset.md` (the document's own header line for its
"durable copy" sections).

**PT:** Pela própria instrução desta tarefa, as citações foram enumeradas **antes** de qualquer
medição, então são elas que ditam o que este censo precisa conter, não o contrário. 15 no
`docs/uix-rcss.md` (8 afirmações distintas, ditas uma vez em inglês e espelhadas em português -- exceto
a afirmação da gramática de decorators, cuja seção em português explicitamente defere pra em inglês,
"mesmo conteúdo técnico", em vez de repetir o caminho uma 16ª vez), 2 no `docs/rmlx-subset.md` (a
própria linha de cabeçalho do documento pras seções que ele chama de "cópia durável").

| # | Doc:line | Language | Claim (paraphrased) |
| :--- | :--- | :--- | :--- |
| 1 | `uix-rcss.md:209` | EN | `:hover` 53-37 uses depending on repo, always composite; `:focus` 3, `:active` 2 |
| 2 | `uix-rcss.md:1283` | PT | mirror of #1 |
| 3 | `uix-rcss.md:256` | EN | `%` means 3 unrelated things per property (families a/b/c), evidenced by census §5.1 |
| 4 | `uix-rcss.md:1331` | PT | mirror of #3 |
| 5 | `uix-rcss.md:319` | EN | the 72-entry property registry is built exclusively from names the census measured (§3) |
| 6 | `uix-rcss.md:1397` | PT | mirror of #5 |
| 7 | `uix-rcss.md:437` | EN | `max-height`/`max-width` are the only 2 of 72 registry entries with zero corpus occurrences |
| 8 | `uix-rcss.md:1448` | PT | mirror of #7, adds "not reachable through any of the 13 shorthands" |
| 9 | `uix-rcss.md:533` | EN | 4 hex color forms measured (`#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`), corrects a stale comment |
| 10 | `uix-rcss.md:1517` | PT | mirror of #9 |
| 11 | `uix-rcss.md:594` | EN | every measured length sits in the 0-3000 range; largest single value `-228dp` |
| 12 | `uix-rcss.md:1581` | PT | mirror of #11 |
| 13 | `uix-rcss.md:637` | EN | angle usage: 24 instances, all `deg`, 0 `rad` |
| 14 | `uix-rcss.md:1627` | PT | mirror of #13 |
| 15 | `uix-rcss.md:718` | EN | in-scope decorator/filter/mask-image function set, per census §9's measured sub-languages |
| -- | *(no PT mirror -- `uix-rcss.md:1650` explicitly defers to the EN section instead of re-citing)* | | |
| 16 | `rmlx-subset.md:126` | EN | header line: 62 source files, 866 style blocks, 3424 declarations |
| 17 | `rmlx-subset.md:301` | PT | mirror of #16 |

⚠️ **The task brief said "17 citações" and named `docs/uix-rcss.md` as the document to read them from.**
Counted literally, `docs/uix-rcss.md` alone has **15** occurrences of the census path (`grep -c
'/var/tmp/censo-rcss-qa1/censo.md' docs/uix-rcss.md` → `15`), not 17. The other 2 live in
`docs/rmlx-subset.md`, which `uix-rcss.md`'s own cross-ref block names as part of the same contract.
17 is correct **across the two documents together**; this is recorded as a measured correction to the
brief's own count, not silently absorbed, per this task's own standing rule (divergence is reported,
never quietly fixed).

---

## 1. Method / Método

**EN:** `tools/rcss_census.py` is a real tokenizer, not a line-oriented regex:

1. **Comment stripping is string-aware and blind to line boundaries.** Both `/* ... */` (does not
   require `*/` on the same line as `/*`) and `//`-to-end-of-line are blanked out character-by-character,
   tracking whether the scanner is inside a quoted string. Two real bugs were caught and fixed by this
   discipline while building the script (kept below as evidence the discipline mattered, not
   hypothetically):
   - A first pass over an early file matched `<style>` **inside an XML comment** (`difficulty_menu__
     lista_hardcore_bloqueado.rml:19`, prose documenting the RML lexer's own limitation happens to
     contain the literal substring `<style>`), then kept scanning for the *next* `</style>` and
     swallowed the real block plus ~9 lines of markup/prose as fake "RCSS text". Fixed by blanking
     `<!-- ... -->` spans before searching for `<style>` tags at all.
   - A first pass over comment syntax stripped only `/* */`, missing that this corpus's RCSS (RmlUi's
     dialect, not plain CSS) also uses `//` line comments for real
     (`save_load_menu__modo_carregar_dois_slots_ocupados.rml:121-125`, a 5-line `//` comment block
     between two real rules) -- a 3-part comma-separated **prose** sentence
     ("`"Voltar" NAO participa , de state.selected... , sempre desenhado`") was counted as a comma-list
     selector until this was fixed. This is the exact "comment counted as a selector" failure mode this
     task's own brief names, reproduced independently, one comment syntax later.
2. **Block extraction is brace-depth-counted, string-aware, and does not require a rule's `{`/`}`, or a
   comma-separated selector list, to share a source line** -- the house's own measured lesson that a
   line-oriented `grep` undercounts a selector broken across 4 lines (`glintfx/src/ua_stylesheet.hpp`'s
   own 16-tag UA-stylesheet rule) is structurally impossible to repeat here, because no step of this
   parser ever looks at "one line" as a unit.
3. **Declarations are split on top-level `;` only** (paren-depth tracked, so a function argument list
   is never mistaken for a declaration boundary -- moot for `;`-using values in this corpus since RCSS
   itself never nests `;` inside a value, but implemented correctly regardless).
4. **Selectors are split on top-level `,` only**, same paren-depth discipline.
5. **Numeric/unit tokenization is a regex, but only ever applied to an already-isolated declaration
   VALUE string** (never to raw file text, never used to decide a rule or selector boundary) -- this is
   the sub-tokenization step, not the structural parse, so it does not repeat the line-oriented mistake
   the structural parts above were built to avoid.

Full source: [`tools/rcss_census.py`](../tools/rcss_census.py). Re-run:
`python3 tools/rcss_census.py --repo-root .` (JSON to stdout); this document's tables were produced from
that JSON, not typed by hand.

**PT:** `tools/rcss_census.py` é um tokenizador de verdade, não uma regex orientada a linha:

1. **Remoção de comentário é sensível a string e cega a fronteira de linha.** Tanto `/* ... */` (não
   exige `*/` na mesma linha do `/*`) quanto `//` até fim-de-linha são apagados caractere a caractere,
   rastreando se o scanner está dentro de uma string entre aspas. Dois bugs reais foram pegos e
   corrigidos por essa disciplina construindo o script (mantidos abaixo como evidência de que a
   disciplina importou, não hipoteticamente):
   - Uma primeira passada sobre um arquivo cedo casou `<style>` **dentro de um comentário XML**
     (`difficulty_menu__lista_hardcore_bloqueado.rml:19`, prosa documentando uma limitação do próprio
     lexer de RML contém por acaso a substring literal `<style>`), e continuou procurando o *próximo*
     `</style>`, engolindo o bloco real mais ~9 linhas de markup/prosa como "texto RCSS" falso.
     Corrigido apagando trechos `<!-- ... -->` antes de sequer buscar tag `<style>`.
   - Uma primeira passada sobre sintaxe de comentário só removia `/* */`, perdendo que o RCSS deste
     corpus (dialeto do RmlUi, não CSS puro) também usa comentário `//` de linha de verdade
     (`save_load_menu__modo_carregar_dois_slots_ocupados.rml:121-125`, um bloco de comentário `//` de 5
     linhas entre duas regras reais) -- uma frase de **prosa** com 3 partes separadas por vírgula
     (`"Voltar" NAO participa , de state.selected... , sempre desenhado`) foi contada como seletor
     comma-list até isso ser corrigido. É exatamente o modo de falha "comentário contado como seletor"
     que o próprio brief desta tarefa nomeia, reproduzido de forma independente, numa sintaxe de
     comentário a mais.
2. **Extração de bloco é por contagem de profundidade de chave, sensível a string, e não exige que o
   `{`/`}` de uma regra, nem uma lista de seletor separada por vírgula, compartilhem linha de fonte** --
   a lição já medida na casa de que um `grep` orientado a linha subconta um seletor quebrado em 4 linhas
   (a própria regra de 16 tags do UA-stylesheet, `glintfx/src/ua_stylesheet.hpp`) é estruturalmente
   impossível de repetir aqui, porque nenhum passo deste parser trata "uma linha" como unidade.
3. **Declaração é cortada só por `;` de topo** (profundidade de parênteses rastreada).
4. **Seletor é cortado só por `,` de topo**, mesma disciplina de profundidade.
5. **Tokenização de número/unidade é regex, mas só é aplicada a um VALOR de declaração já isolado**
   (nunca ao texto cru do arquivo, nunca usada pra decidir fronteira de regra ou seletor) -- é o passo
   de sub-tokenização, não o parse estrutural, então não repete o erro orientado a linha que as partes
   estruturais acima foram desenhadas pra evitar.

Fonte completa: [`tools/rcss_census.py`](../tools/rcss_census.py). Re-rodar:
`python3 tools/rcss_census.py --repo-root .` (JSON no stdout); as tabelas deste documento vieram desse
JSON, não foram digitadas à mão.

---

## 2. The corpus / O corpus

**EN:** `git ls-files '*.rcss'` → 23 files. `git ls-files '*.rml'` → 62 files. 85 total, matching this
task's own stated scope exactly. Of the 62 `.rml` files, 36 contain at least one `<style>` block
(`git ls-files '*.rml' | xargs grep -lE '<style'` finds 37 -- one more than the parser's own 36, because
that grep hits the same false match inside an XML comment section 1 above describes; the parser's 36 is
the correct count, the 37 is exactly the bug this document exists to not repeat).

| Metric | Value |
| :--- | ---: |
| `.rcss` files | 23 |
| `.rml` files | 62 |
| `.rml` files with >=1 real `<style>` block | 36 |
| Total RCSS source chunks parsed (23 + 36) | 59 |
| Top-level style blocks | 850 |
| -- of which normal rule blocks | 837 |
| -- of which `@font-face` | 19 |
| -- of which `@keyframes` | 4 |
| `@keyframes` child (step) blocks | 9 |
| Declarations, total | 3388 |
| -- inside rule blocks | 3339 |
| -- inside `@font-face` blocks | 40 |
| -- inside `@keyframes` steps | 9 |
| Unreadable files | 0 |
| Non-blank leftover text outside any block | 0 |

**PT:** `git ls-files '*.rcss'` → 23 arquivos. `git ls-files '*.rml'` → 62 arquivos. 85 no total,
batendo exatamente com o escopo que a própria tarefa declarou. Dos 62 `.rml`, 36 têm pelo menos um
bloco `<style>` real (`git ls-files '*.rml' | xargs grep -lE '<style'` acha 37 -- um a mais que os 36 do
parser, porque esse grep casa o mesmo falso-positivo dentro de comentário XML que a seção 1 acima
descreve; o 36 do parser é a contagem correta, o 37 é exatamente o bug que este documento existe pra
não repetir). Tabela: ver acima (bilíngue, números idênticos).

⚠️ **A file at file #86 that this census's primary scope excludes on purpose, by the task's own
definition** — `glintfx/src/ua_stylesheet.hpp` is a `.hpp` (C++ source, not `.rcss`/`.rml`) that embeds
real, shipped RCSS as a raw string literal (`kUaStylesheetRcss`). It is glintfx's own base UA
stylesheet -- the rule that sets `display: block` on every structural element of every document glintfx
renders -- and it is real, versioned, production RCSS, just not inside either of the two file
extensions this task named as the surviving corpus. Section 5 below quantifies exactly what including
it changes (it turns out to explain several of the small numeric gaps against `docs/rmlx-subset.md`'s
own already-published numbers). / **Um arquivo #86 que o escopo primário deste censo exclui de
propósito, pela própria definição da tarefa** -- `glintfx/src/ua_stylesheet.hpp` é um `.hpp` (fonte
C++, não `.rcss`/`.rml`) que embute RCSS real, já shippado, como raw string literal
(`kUaStylesheetRcss`). É a própria folha UA-base da glintfx -- a regra que põe `display: block` em todo
elemento estrutural de todo documento que a glintfx renderiza -- e é RCSS real, versionado, de
produção, só que fora das duas extensões que esta tarefa nomeou como o corpus sobrevivente. A seção 5
abaixo quantifica exatamente o que incluí-lo muda (explica várias das pequenas lacunas numéricas contra
os números já publicados no `docs/rmlx-subset.md`).

---

## 3. Per-citation verification / Verificação citação por citação

### 3.1 Pseudo-classes: `:hover`/`:focus`/`:active` (citations #1-2)

**EN:** Measured, this repository's corpus only: **`:hover` 37**, always composite (`.foo:hover` /
`#foo:hover` -- 0 bare `:hover` instances), **`:focus` 0**, **`:active` 0**.

This matches `docs/rmlx-subset.md`'s own already-published, once-corrected table (section 6.2 there:
"`:hover` is the only pseudo-class found", 37 instances) exactly -- strong cross-validation of this
census's method against a table that document had already independently corrected twice (see its own
section 6.1 "the orchestrator's own error" note).

**What `uix-rcss.md:209-212`'s "53-37... `:focus`: 3, `:active`: 2" actually is:** these four numbers
are **not all from the same corpus**. `rmlx-subset.md` section 2 states plainly that its 12-number
table (`:hover` 53, `:focus` 3, `:active` 2, among others) is **"Medido pelo consumidor"** -- GusWorld's
own repository, a different codebase this qa-engineer has no access to and this task's brief explicitly
scopes out. The 37 is this repository's own corpus (cross-checked above). `uix-rcss.md`'s own phrasing
("53-37 uses depending on which repository is counted") is technically accurate but easy to misread as
"both numbers come from the census this document cites" -- they do not; only 37 does. **This
repository's corpus alone has zero measured `:focus` and zero measured `:active`** -- a materially
different picture than "both real, two orders of magnitude rarer than `:hover`" reads on a first pass,
if a reader assumes "the census" means "this repo".

| Pseudo-class | This repo's corpus (measured here) | GusWorld's corpus (cited by `rmlx-subset.md` §2, not independently verified here) |
| :--- | ---: | ---: |
| `:hover` | **37**, 100% composite | 53 |
| `:focus` | **0** | 3 |
| `:active` | **0** | 2 |

**PT:** Medido, só o corpus deste repositório: **`:hover` 37**, sempre composto (`.foo:hover` /
`#foo:hover` -- 0 instâncias de `:hover` isolado), **`:focus` 0**, **`:active` 0**.

Bate exatamente com a tabela já publicada (e já corrigida uma vez) do `docs/rmlx-subset.md` (seção 6.2:
"`:hover` é a única pseudo-classe achada", 37 instâncias) -- validação cruzada forte do método deste
censo contra uma tabela que aquele documento já tinha corrigido de forma independente duas vezes (ver a
própria nota "erro do orquestrador" na seção 6.1 de lá).

**O que "53-37... `:focus`: 3, `:active`: 2" do `uix-rcss.md:209-212` realmente é:** esses quatro
números **não vêm todos do mesmo corpus**. A seção 2 do `rmlx-subset.md` diz claramente que sua tabela
de 12 números (`:hover` 53, `:focus` 3, `:active` 2, entre outros) foi **"medido pelo consumidor"** -- o
próprio repositório do GusWorld, um código-base diferente ao qual este qa-engineer não tem acesso e que
o brief desta tarefa explicitamente deixa fora de escopo. O 37 é o corpus deste repositório (validado
cruzado acima). A própria formulação do `uix-rcss.md` ("53-37 usos dependendo de qual repositório se
conta") é tecnicamente correta mas fácil de ler errado como "os dois números vêm do censo que este
documento cita" -- não vêm; só o 37 vem. **O corpus deste repositório sozinho tem zero `:focus` medido e
zero `:active` medido** -- um quadro bem diferente de "os dois reais, duas ordens de grandeza mais raros
que `:hover`" numa primeira leitura, se o leitor assumir que "o censo" quer dizer "este repo". Tabela:
ver acima (bilíngue).

**Verdict / Veredito: MATCH for `:hover`** (37, exact cross-check against `rmlx-subset.md`); **clarifying
finding, not a contradiction, for `:focus`/`:active`** -- the doc's own numbers are correct but come
from a corpus this census does not and should not reproduce; the doc's phrasing invites a scope
misreading this section corrects.

### 3.2 Percentage families (citations #3-4)

**EN:** `docs/rmlx-subset.md` section 6.3 already states, from the same original census: 207 total `%`
instances, 103 of them "family (a)" (box-relative: `width`/`height`/`left`/`top`, plus the
structurally-identical `margin-*`/`padding-*`/`min/max-*`/`flex-basis`, none of which this corpus
happens to use with `%`). This census's own independent count, restricted to the 4 family-(a)
properties actually used with `%` in this corpus:

| Property | `%` instances |
| :--- | ---: |
| `width` | 46 |
| `height` | 40 |
| `left` | 15 |
| `top` | 4 |
| **Family (a) total** | **105** |
| **All `%` instances (any property/value)** | **209** |

**Close, not exact, against the already-published 103/207:** +2 on the family-(a) subtotal, +2 on the
overall total. Not reconciled to zero; recorded as a small open gap (see section 6, "unreconciled
residuals").

**PT:** A seção 6.3 do `docs/rmlx-subset.md` já declara, do mesmo censo original: 207 instâncias totais
de `%`, 103 delas "família (a)" (relativa à caixa: `width`/`height`/`left`/`top`, mais os
estruturalmente-idênticos `margin-*`/`padding-*`/`min/max-*`/`flex-basis`, nenhum dos quais este corpus
usa com `%`). A contagem independente deste censo, restrita às 4 propriedades da família (a) de fato
usadas com `%` neste corpus: ver tabela acima (bilíngue).

**Próximo, não exato, contra o 103/207 já publicado:** +2 no subtotal da família (a), +2 no total geral.
Não reconciliado a zero; registrado como uma pequena lacuna aberta (ver seção 6, "resíduos não
reconciliados").

**Verdict / Veredito: CLOSE** (within ~2%, methodology sound, small unreconciled residual).

### 3.3 Property registry: exclusivity, `max-height`/`max-width` (citations #5-8)

**EN:** Directly-written property names measured in this corpus, alphabetical, with per-name count
(includes both RmlUi-native and glintfx-custom `ripple-*`/`image-tint-*` properties; **excludes**
longhands only reachable through shorthand expansion, e.g. `border-left`/`border-right`/`padding-
bottom`/`padding-left` never appear written directly in this corpus, only via `border`/`padding` --
expected and consistent with section 6's own scope discipline, not a gap):

```
-rmlui-fallback-face:2  align-items:26        animation:4            backdrop-filter:1
background:32           background-color:128  border:178             border-bottom:5
border-color:19         border-radius:115     border-top:9            bottom:54
box-shadow:135          box-sizing:57         color:322               cursor:3
decorator:203           display:121           filter:23               flex:19
focus:1                 font-family:56        font-size:193           gap:5
height:188              image-tint-color:8    image-tint-mode:10      image-tint-threshold:5
justify-content:13      left:146              letter-spacing:80       line-height:47
margin:95               margin-bottom:35      margin-left:49          margin-right:15
margin-top:69           mask-image:1          min-height:11           min-width:5
opacity:20              overflow:2            overflow-x:10           overflow-y:11
padding:102             padding-right:8       padding-top:17          position:137
right:45                ripple-origin-x:2     ripple-origin-y:2       ripple-phase:2
ripple-strength:2       ripple-width:2        src:19                  tab-index:2
text-align:89           text-overflow:1       text-transform:10       top:153
transform:2             vertical-align:1      white-space:14          width:247
```

**`max-height` and `max-width`: 0 occurrences, confirmed** -- neither name appears anywhere in this
corpus's directly-written declarations, matching the doc's claim exactly. **`font-style` and
`font-weight`: 0 occurrences, also confirmed** (the doc's own section 13 exclusion list, cited at
`uix-rcss.md:1757/1773`, without a `/var/tmp` path on that specific line but backed by the same census).

**On "not reachable through any of the 13 shorthands" (citation #8):** section 6.2 of `uix-rcss.md`
itself lists exactly 13 shorthand names (`margin`, `padding`, `border-radius`, `border-color`,
`border-top`, `border-right`, `border-bottom`, `border-left`, `border`, `background`, `gap`,
`overflow`, `flex`) -- self-consistent with the "13" figure; this census does not verify RmlUi's own
shorthand-expansion source code (out of scope by this task's own exclusion of the implementation
directories), only that `max-height`/`max-width` are absent from this corpus's *written* RCSS, direct or
shorthand-syntax alike.

**PT:** Nomes de propriedade escritos diretamente, medidos neste corpus, alfabético, com contagem por
nome (inclui nativas do RmlUi e as custom da glintfx `ripple-*`/`image-tint-*`; **exclui** longhands só
alcançáveis via expansão de shorthand, ex. `border-left`/`border-right`/`padding-bottom`/`padding-left`
nunca aparecem escritas direto neste corpus, só via `border`/`padding` -- esperado e consistente com a
própria disciplina de escopo da seção 6, não uma lacuna): ver bloco acima (bilíngue, mesmos números).

**`max-height` e `max-width`: 0 ocorrências, confirmado** -- nenhum dos dois nomes aparece em nenhuma
declaração escrita direto neste corpus, batendo exatamente com a afirmação do documento. **`font-style`
e `font-weight`: 0 ocorrências, também confirmado** (a própria lista de exclusão da seção 13 do
documento, citada em `uix-rcss.md:1757/1773`, sem caminho `/var/tmp` nessa linha específica mas
apoiada no mesmo censo).

**Sobre "não alcançáveis por nenhum dos 13 shorthands" (citação #8):** a própria seção 6.2 do
`uix-rcss.md` lista exatamente 13 nomes de shorthand (`margin`, `padding`, `border-radius`,
`border-color`, `border-top`, `border-right`, `border-bottom`, `border-left`, `border`, `background`,
`gap`, `overflow`, `flex`) -- auto-consistente com a cifra "13"; este censo não verifica o código-fonte
de expansão de shorthand do próprio RmlUi (fora de escopo pela própria exclusão desta tarefa dos
diretórios de implementação), só que `max-height`/`max-width` estão ausentes da RCSS *escrita* deste
corpus, direta ou em sintaxe de shorthand.

**Verdict / Veredito: MATCH** (zero-corpus claim for `max-height`/`max-width` and for `font-style`/
`font-weight` both confirmed exactly; the "13 shorthands" figure is self-consistent within the doc, not
independently re-derivable from this corpus alone).

### 3.4 Hex color forms (citations #9-10)

**EN:** All 4 hex forms are present, confirmed exactly:

| Form | Hex digits | Instances | Example |
| :--- | :---: | ---: | :--- |
| `#rgb` | 3 | 1 | `glintfx/demos/showcase/showcase.rcss` -- `color: #fff` |
| `#rgba` | 4 | 2 | `glintfx/demos/showcase/showcase.rcss` -- `mask-image: ...#000f`/`#0000` |
| `#rrggbb` | 6 | 1044 | `consumer-example-embed/contract_scene.rcss` -- `background-color: #ff3366` |
| `#rrggbbaa` | 8 | 167 | `glintfx/demos/showcase/showcase.rcss` -- `filter: ...#5fd0ff80` |

Named colors: **exactly 2 distinct names measured, both cited by the doc** -- `transparent` (16
instances) and `white` (2 instances). Color functions (`rgb()`/`rgba()`/`hsl()`/`hsla()`/`lab()`/
`lch()`/`oklab()`/`oklch()`): **0 instances of every one**, matching "0 `rgb()`/`rgba()` funcional".

**PT:** As 4 formas hex estão presentes, confirmado exatamente: ver tabela acima (bilíngue).

Cores nomeadas: **exatamente 2 nomes distintos medidos, os dois citados pelo documento** --
`transparent` (16 instâncias) e `white` (2 instâncias). Funções de cor (`rgb()`/`rgba()`/`hsl()`/
`hsla()`/`lab()`/`lch()`/`oklab()`/`oklch()`): **0 instâncias de cada uma**, batendo com "0
`rgb()`/`rgba()` funcional".

**Verdict / Veredito: MATCH exact** on all three sub-claims (4 hex forms, 2 named colors, 0 color
functions).

### 3.5 Length range and the "-228dp" record claim (citations #11-12)

**EN:** The broader claim -- "every measured length sits in the 0-3000 range" -- **holds**: the largest
magnitude found in this corpus is **999** (`border-radius: 999dp`, the "pill" corner-radius idiom, used
dozens of times across the `system_menu__*`/`save_load_menu__*`/`title_menu__*`/`npc_dialogue__*`
fixtures) and **-410** (`margin-left: -410dp`, `npc_dialogue__no_com_3_escolhas.rml:41` and 2 sibling
fixtures) -- both still comfortably 3-digit, well inside 0-3000, and the doc's own precision argument
(4 decimal digits is more than `float32` can meaningfully carry at this magnitude) is unaffected either
way.

**The specific number cited as the record, however, is not the record.** `-228dp` is real
(`margin-left: -220dp; margin-top: -228dp;` in `difficulty_menu__lista_hardcore_bloqueado.rml:37` and
one sibling fixture) but is neither the largest positive value (999 > 228) nor the largest-magnitude
negative value (`-410` and `-310` both exceed 228 in magnitude, found in
`npc_dialogue__no_com_3_escolhas.rml:41`/`system_menu__config_audio_sliders.rml:296` and 4 sibling
fixtures each). Top-5 by absolute magnitude, this corpus:

| Value | Property | `file:line` |
| :--- | :--- | :--- |
| `999dp` | `border-radius` | `difficulty_menu__lista_hardcore_bloqueado.rml:73` (and 30+ more sites) |
| `-410dp` | `margin-left` | `npc_dialogue__no_com_3_escolhas.rml:41` (and 4 sibling fixtures) |
| `-310dp` | `margin-left` | `system_menu__config_audio_sliders.rml:296` (and 4 sibling fixtures) |
| `900px` | `width` | `glintfx/tests/ripple_scene.rcss` |
| `-230dp` | (measured; not individually traced here) | -- |

**PT:** A afirmação mais ampla -- "todo comprimento medido fica na faixa 0-3000" -- **se sustenta**: a
maior magnitude achada neste corpus é **999** (`border-radius: 999dp`, o idioma de canto "pílula", usado
dezenas de vezes pelas fixtures `system_menu__*`/`save_load_menu__*`/`title_menu__*`/
`npc_dialogue__*`) e **-410** (`margin-left: -410dp`, `npc_dialogue__no_com_3_escolhas.rml:41` e 2
fixtures irmãs) -- os dois ainda confortavelmente 3 dígitos, bem dentro de 0-3000, e o próprio argumento
de precisão do documento (4 casas decimais é mais do que `float32` consegue carregar com sentido nessa
magnitude) não muda de nenhum jeito.

**O número específico citado como recorde, porém, não é o recorde.** `-228dp` é real
(`margin-left: -220dp; margin-top: -228dp;` em `difficulty_menu__lista_hardcore_bloqueado.rml:37` e uma
fixture irmã) mas não é nem o maior valor positivo (999 > 228) nem a maior magnitude negativa (`-410` e
`-310` excedem 228 em magnitude, achados em
`npc_dialogue__no_com_3_escolhas.rml:41`/`system_menu__config_audio_sliders.rml:296` e mais 4 fixtures
irmãs cada). Top-5 por magnitude absoluta: ver tabela acima (bilíngue).

**Verdict / Veredito: PARTIAL DIVERGE.** The range claim (0-3000, 3-digit-max order of magnitude, hence
the precision argument) is correct; the specific cited example (`-228dp` as "the largest single value
observed") is not the largest by either sign -- `999dp` and `-410dp` both exceed it. Reported as a
finding, the underlying spec conclusion (4 decimal digits is enough precision) is unaffected because it
only depends on the *order of magnitude* staying 3-digit, which both the cited and the actual record
share.

### 3.6 Angle units (citations #13-14)

**EN:** **Exact match**: `deg` 24 instances, `rad` 0 instances. Largest angle value: `360deg`
(`transform: rotate(360deg)`, `gusworld_battle_cockpit.rml`). All 24 `deg` instances occur inside
`decorator: linear-gradient(...)`/`radial-gradient(...)` angle arguments or `transform: rotate(...)`.

**PT:** **Bate exato**: `deg` 24 instâncias, `rad` 0 instâncias. Maior valor de ângulo: `360deg`
(`transform: rotate(360deg)`, `gusworld_battle_cockpit.rml`). As 24 instâncias de `deg` ocorrem dentro
de argumentos de ângulo de `decorator: linear-gradient(...)`/`radial-gradient(...)` ou de
`transform: rotate(...)`.

**Verdict / Veredito: MATCH exact.**

### 3.7 In-scope decorator/filter/mask-image functions (citation #15)

**EN:** Every function name found as the head of a `<function>(...)` call inside `decorator`/
`mask-image`/`filter`/`backdrop-filter`/`transform`/`animation` values, with instance counts:

| Function | Instances | In `docs/uix-rcss.md` §9.2's table? |
| :--- | ---: | :--- |
| `vertical-gradient` | 107 | **No** -- not listed in §9.2's function table at all |
| `polygon` | 47 | Yes |
| `drop-shadow` | 23 | Yes |
| `linear-gradient` | 22 | Yes |
| `radial-gradient` | 21 | Yes |
| `image-tint` | 13 | Yes |
| `horizontal-gradient` | 6 | Yes |
| `image` | 6 | Yes |
| `ripple` | 2 | Yes |
| `rotate` | 2 | (a `transform` function, out of §9.2's own list by design -- §9.2 covers `decorator`/`mask-image`/`filter`/`backdrop-filter` only, `transform` has its own grammar elsewhere in the doc) |
| `blur` | 1 | Yes |

**Finding: `vertical-gradient` is this corpus's single most-used decorator function (107 instances,
more than `polygon` and `linear-gradient`/`radial-gradient` combined) and does not appear in
`docs/uix-rcss.md` section 9.2's own function table.** `horizontal-gradient` (6 instances) is listed
there as "`mask-image`'s own 2-stop shorthand form", explicitly named; `vertical-gradient` is not named
anywhere in the sections of `uix-rcss.md` this qa-engineer read. This is the single largest gap between
what section 9.2 authorizes and what this corpus actually ships.

**PT:** Todo nome de função achado como cabeça de uma chamada `<função>(...)` dentro de valores de
`decorator`/`mask-image`/`filter`/`backdrop-filter`/`transform`/`animation`, com contagem de instância:
ver tabela acima (bilíngue).

**Achado: `vertical-gradient` é a função de decorator mais usada deste corpus (107 instâncias, mais que
`polygon` e `linear-gradient`/`radial-gradient` somados) e não aparece na tabela de função da própria
seção 9.2 do `docs/uix-rcss.md`.** `horizontal-gradient` (6 instâncias) está listada lá como "a própria
forma-atalho de 2-stops do `mask-image`", explicitamente nomeada; `vertical-gradient` não é nomeada em
nenhuma das seções do `uix-rcss.md` que este qa-engineer leu. É a maior lacuna sozinha entre o que a
seção 9.2 autoriza e o que este corpus de fato usa em produção.

**Verdict / Veredito: DIVERGE (significant).** `docs/effects.md` is out of this census's read scope (not
excluded by the brief, simply not read to keep this document focused on the corpus, not the shipped
grammar reference) -- it is plausible `vertical-gradient` is already documented there and simply missing
from `uix-rcss.md` section 9.2's own table; either way, section 9.2's claim that its function list is
"per the census's own measured decorator sub-languages" does not match this census's own measurement of
the same corpus for this one function, and `RMLX-2`'s own header clause ("stop, edit this spec with a
diff, get the líder's sign-off") applies squarely here before any dumper implements `decorator`.

### 3.8 Overall unit distribution (citations #16-17, `rmlx-subset.md`)

**EN:** `docs/rmlx-subset.md` section 6.3 states 8 units measured: `dp` 2237, `px` 334, `%` 207,
unitless 171, `auto` 33, `deg` 24, `s` 4, `em` 1, zero `rem`/`vw`/`vh`/`ms`. This census's own count,
same 85-file scope:

| Unit | This census (85 files) | This census + `ua_stylesheet.hpp` (86 files) | `rmlx-subset.md` §6.3 |
| :--- | ---: | ---: | ---: |
| `dp` | 2231 | **2237** | 2237 |
| `px` | 330 | 330 | 334 |
| `%` | 209 | 209 | 207 |
| unitless | 163 | 163 | 171 |
| `auto` (family-a only, see note) | 31 | -- | 33 |
| `deg` | 24 | 24 | 24 |
| `s` | 4 | 4 | 4 |
| `em` | 1 | 1 | 1 |
| `rem`/`vw`/`vh`/`ms` | 0 | 0 | 0 |

**`dp` matches exactly (2237) once `glintfx/src/ua_stylesheet.hpp` is folded in** -- strong,
specific evidence that the original census's own corpus scope included this `.hpp`-embedded stylesheet,
outside the two file extensions this task defines as "the surviving corpus". `deg`/`s`/`em`/the four
zeros match exactly regardless. `px`/`%`/unitless remain a small, unreconciled residual (1-5%) even
after folding `ua_stylesheet.hpp` in -- plausibly explained by a third RCSS-bearing source
(`glintfx/tests/uix_style/lexer_corpus_sanity.cpp` also embeds an `R"rcss(...)rcss"` raw string, found
by `git ls-files '*.hpp' '*.cpp' | xargs grep -l 'R"rcss('`, but this census deliberately does not read
or fold it in -- it lives inside the test suite for the parser implementation this task explicitly
scopes out, and folding a *test fixture* into a *corpus* census would blur exactly the "measure the
corpus, not the implementation" line this task draws) -- left open, not guessed at further.

**`auto`:** counted only for the length-percent family-(a) properties (`width`/`height`/`margin*`/
`padding*`/`top`/`right`/`bottom`/`left`/`flex-basis`/`min-max-*`), since `auto` is also a valid keyword
for unrelated domains this corpus uses it in (`overflow-y: auto`, 11 instances; `tab-index: auto`, 2
instances) that are not part of the same "unit" being measured. Restricted this way: **31**, close to
the doc's 33.

**PT:** A seção 6.3 do `docs/rmlx-subset.md` declara 8 unidades medidas: `dp` 2237, `px` 334, `%` 207,
sem-unidade 171, `auto` 33, `deg` 24, `s` 4, `em` 1, zero `rem`/`vw`/`vh`/`ms`. A contagem deste censo,
mesmo escopo de 85 arquivos: ver tabela acima (bilíngue).

**`dp` bate exato (2237) assim que `glintfx/src/ua_stylesheet.hpp` entra na conta** -- evidência forte e
específica de que o escopo de corpus do censo original incluía essa folha embutida em `.hpp`, fora das
duas extensões que esta tarefa define como "o corpus sobrevivente". `deg`/`s`/`em`/os quatro zeros batem
exato de qualquer jeito. `px`/`%`/sem-unidade seguem um resíduo pequeno, não reconciliado (1-5%) mesmo
depois de incluir o `ua_stylesheet.hpp` -- plausivelmente explicado por uma terceira fonte com RCSS
embutido (`glintfx/tests/uix_style/lexer_corpus_sanity.cpp` também embute um raw string
`R"rcss(...)rcss"`, achado por `git ls-files '*.hpp' '*.cpp' | xargs grep -l 'R"rcss('`, mas este censo
deliberadamente não lê nem inclui esse arquivo -- ele mora dentro da suíte de teste da implementação do
parser que esta tarefa explicitamente deixa fora de escopo, e incluir uma *fixture de teste* num censo
de *corpus* borraria exatamente a linha "meça o corpus, não a implementação" que esta tarefa traça) --
deixado em aberto, sem mais chute.

**`auto`:** contado só pras propriedades da família (a) relativa-à-caixa (`width`/`height`/`margin*`/
`padding*`/`top`/`right`/`bottom`/`left`/`flex-basis`/`min-max-*`), já que `auto` também é palavra-chave
válida pra domínios não relacionados que este corpus usa (`overflow-y: auto`, 11 instâncias;
`tab-index: auto`, 2 instâncias) que não fazem parte da mesma "unidade" sendo medida. Restrito assim:
**31**, próximo do 33 do documento.

**Verdict / Veredito: MATCH exact for `dp`/`deg`/`s`/`em`/the-four-zeros; CLOSE (small unreconciled
residual) for `px`/`%`/unitless/`auto`.**

---

## 4. Findings not directly cited, corpus-adjacent / Achados não citados diretamente, adjacentes ao corpus

### 4.1 `box-shadow` spread-omission: the doc's claim is inverted against this corpus

**EN:** `docs/uix-rcss.md` section 9.1 states: *"`spread` defaults to `0.0000px` when the source
omitted it (per the census, 124 of 135 single-layer declarations do)"* -- i.e. the claim is that **most**
single-layer `box-shadow` declarations omit the 4th length (spread), relying on the default.

Measured, this corpus: **135 total `box-shadow` declarations** (matching the doc's own "135" exactly),
of which 128 are single-layer and 7 are multi-layer. Of the 128 single-layer declarations: **127
explicitly specify all 4 length fields (offset-x, offset-y, blur, spread)**, and **only 1 omits spread**
(3 length fields). This is the **near-opposite** of "124 of 135 omit it" -- in this corpus, 127 of 128
single-layer declarations **specify** spread explicitly; omission is the rare case, not the common one.

| | Doc's claim | Measured, this corpus |
| :--- | :--- | :--- |
| Total single-layer declarations | 135 (doc's own denominator) | 128 |
| Spread omitted (3 lengths) | **124** | **1** |
| Spread specified (4 lengths) | 11 (implied) | **127** |

The `135` figure is real and matches exactly (total `box-shadow` declarations, not exclusively
single-layer -- the doc's own sentence conflates "135" with "single-layer" when 7 of the 135 are
multi-layer); the `124` figure appears to have survived the loss of the scratch report with its
direction flipped -- a plausible, not provable, hypothesis is that "124 specify it explicitly" became
"124 omit it" somewhere between measurement and the sentence that shipped, since 124 (this census's own
127, off by 3 accounted for below) is close to this census's own **explicit** count, not its omitted
count.

**PT:** A seção 9.1 do `docs/uix-rcss.md` declara: *"`spread` assume `0.0000px` por padrão quando a
fonte omitiu (pelo censo, 124 de 135 declarações single-layer omitem)"* -- ou seja, a afirmação é que a
**maioria** das declarações `box-shadow` de camada única omite o 4º comprimento (spread), contando com o
padrão.

Medido, este corpus: **135 declarações `box-shadow` no total** (batendo exato com o "135" do próprio
documento), das quais 128 são de camada única e 7 são multi-camada. Das 128 de camada única: **127
especificam explicitamente os 4 campos de comprimento** (offset-x, offset-y, blur, spread), e **só 1
omite spread** (3 campos). É o **quase-oposto** de "124 de 135 omitem" -- neste corpus, 127 de 128
declarações de camada única **especificam** spread explicitamente; omitir é o caso raro, não o comum.
Tabela: ver acima (bilíngue).

A cifra `135` é real e bate exata (total de declarações `box-shadow`, não exclusivamente camada única --
a própria frase do documento confunde "135" com "camada única" quando 7 das 135 são multi-camada); a
cifra `124` parece ter sobrevivido à perda do relatório scratch com a direção invertida -- uma hipótese
plausível, não provável, é que "124 especificam explicitamente" virou "124 omitem" em algum ponto entre
a medição e a frase que foi publicada, já que 124 (contra o 127 próprio deste censo, diferença de 3
explicada abaixo) é próximo da contagem **explícita** deste censo, não da contagem de omissão.

⚠️ **3 single-layer declarations use unitless `0` for offset-x/offset-y instead of `0dp`/`0px`**
(`showcase.rcss`, `bu_scene.rcss`, `embed_scene.rcss`, all the identical
`box-shadow: #5fd0ff 0 0 32px 8px;`) -- correctly counted as "4 length fields present" once unitless
zero is accepted as a valid length token (RmlUi's own parser accepts a bare `0` for any length-typed
argument), bringing single-layer totals to 128 = 127 + 1. / **3 declarações de camada única usam `0`
sem unidade em vez de `0dp`/`0px`** para offset-x/offset-y (`showcase.rcss`, `bu_scene.rcss`,
`embed_scene.rcss`, todas o mesmo `box-shadow: #5fd0ff 0 0 32px 8px;`) -- contadas corretamente como "4
campos de comprimento presentes" uma vez que zero sem unidade é aceito como token de comprimento válido
(o próprio parser do RmlUi aceita `0` cru pra qualquer argumento tipo-comprimento), levando o total de
camada única a 128 = 127 + 1.

**Verdict / Veredito: DIVERGE (significant, high-value finding).** This is exactly the class of claim
the task brief calls out as most valuable to catch -- an **absence/frequency** claim nobody could
re-verify without the report, now measured directly against the surviving corpus and found reversed.
Does not change section 9.1's own *algorithm* (spread still defaults to `0.0000px` when omitted, that
mechanism is a code fact this census does not adjudicate) -- only the corpus-frequency claim used to
justify why the default matters.

### 4.2 Shorthand value-count distributions (`docs/uix-rcss.md` §6.2 table)

**EN:** Value-count histograms for the 4 `Box`-algorithm shorthands (`margin`, `padding`,
`border-radius`, `border-color`) plus `border`/`border-<side>`/`background`:

| Shorthand | 1-value | 2-value | 3-value | 4-value | Doc's claim |
| :--- | ---: | ---: | ---: | ---: | :--- |
| `margin` | 40 | 16 | 0 | 39 | 44/16/39 -- 2-value and 4-value MATCH exactly, 1-value off by 4 |
| `padding` | 25 | 59 | 0 | 18 | 29/59/18 -- 2-value and 4-value MATCH exactly, 1-value off by 4 |
| `border-radius` | 115 | 0 | 0 | 0 | "100% 1-value" -- **MATCH exact** |
| `border-color` | 19 | -- | -- | -- | "100% 1-value" -- **MATCH exact** |
| `border` (recursive, 4 sides fed the same 2-token value) | 178 rules × 2-token | -- | -- | -- | "100% 2-part" -- **MATCH exact** |
| `border-top`/`-right`/`-bottom`/`-left` (FallThrough) | -- | 14 × 2-token | -- | -- | (no value-count claim, only order-sensitivity, out of this census's scope) |
| `background` | 32 × 1-token | -- | -- | -- | "100% solid-color value" -- **MATCH exact** |

The two 1-value gaps (`margin` 40 vs 44, `padding` 25 vs 29, both -4) are the same order of magnitude as
the other small residuals in section 3.8/3.2 above, plausibly the same unreconciled scope difference,
not investigated further per this task's "don't chase every last delta" cost/value line.

**PT:** Histogramas de contagem de valor pros 4 shorthands do algoritmo `Box` (`margin`, `padding`,
`border-radius`, `border-color`) mais `border`/`border-<lado>`/`background`: ver tabela acima
(bilíngue).

Os dois vazios de 1-valor (`margin` 40 vs 44, `padding` 25 vs 29, ambos -4) são da mesma ordem de
grandeza dos outros resíduos pequenos das seções 3.8/3.2 acima, plausivelmente a mesma diferença de
escopo não reconciliada, não investigada mais a fundo pela própria linha de custo/valor desta tarefa
("não persiga todo delta até o fim").

**Verdict / Veredito: MATCH** on shape/form for all 6 shorthands (`border-radius`/`border-color`/
`border`/`background` exact; `margin`/`padding` exact on the 2-value and 4-value rows, small residual
on 1-value only).

---

## 5. Verdict summary / Resumo de veredito

| # | Claim | Verdict |
| :--- | :--- | :--- |
| 1-2 | `:hover` 53-37, `:focus` 3, `:active` 2 | **MATCH** for `:hover`=37 (this repo); clarified scope (GusWorld vs this repo) for `:focus`/`:active` |
| 3-4 | 3 percentage families, family (a) 103/207 | **CLOSE** (105/209 measured, +2/+2) |
| 5-6 | Registry built exclusively from measured names | **MATCH** (methodology confirmed, `max-height`/`max-width`/`font-style`/`font-weight` all 0 as claimed) |
| 7-8 | `max-height`/`max-width` zero-corpus, not shorthand-reachable | **MATCH** (zero confirmed; shorthand-reachability is a code fact, not independently re-derived here) |
| 9-10 | 4 hex forms, corrects stale comment | **MATCH exact** |
| 11-12 | Length range 0-3000, largest `-228dp` | **PARTIAL DIVERGE** (range holds; `-228dp` is not the actual max -- `999dp`/`-410dp` are larger) |
| 13-14 | Angle: 24 instances, all `deg`, 0 `rad` | **MATCH exact** |
| 15 | In-scope decorator function set | **DIVERGE** (`vertical-gradient`, 107 instances, the single most-used decorator function, is absent from §9.2's own table) |
| 16-17 | 62 files / 866 blocks / 3424 declarations | **CLOSE** (85-file primary scope: 850/3388; 86-file scope incl. `ua_stylesheet.hpp`: 859/3401 -- neither matches exactly, but the `dp`-unit exact match in §3.8 is strong evidence the original scope included the `.hpp`) |
| bonus | `box-shadow` spread omitted 124/135 | **DIVERGE (significant)** -- measured near-inverse: 127/128 single-layer declarations specify spread explicitly, only 1 omits it |
| bonus | shorthand value-count shapes | **MATCH** (6/6 shorthands, small residual on `margin`/`padding` 1-value row only) |

---

## 6. Known gaps / unreconciled residuals / Lacunas conhecidas / resíduos não reconciliados

**EN:**
- **Scope boundary, `.hpp`-embedded RCSS:** `glintfx/src/ua_stylesheet.hpp` (production, shipped) is
  folded in only for the section 3.8/comma-list cross-checks against `rmlx-subset.md`, never for this
  document's own primary numbers (section 2's table), per this task's explicit "62 `.rml` + 23 `.rcss`"
  scope. `glintfx/tests/uix_style/lexer_corpus_sanity.cpp` (test fixture, adjacent to the
  implementation this task excludes) is not read or folded in anywhere, on purpose.
- **Small unreconciled residuals** (all 1-5%, none investigated to zero): `px`/`%`/unitless/`auto` unit
  counts (§3.8), `%` family-(a) subtotal (§3.2), `margin`/`padding` 1-value shorthand counts (§4.2).
  Each is reported with its exact measured number rather than adjusted to match the doc, per this
  task's own "report divergence, don't silently reconcile" instruction.
- **This census does not verify:** anything inside `glintfx/src/uix/style/**` or
  `glintfx/src/rml/rcss_dump*` (excluded by brief, to protect the differential oracle's author
  independence), the shorthand-expansion source in RmlUi itself (out of this census's read scope), or
  GusWorld's own repository (the source of the `:hover`=53/`:focus`=3/`:active`=2 figures in
  `rmlx-subset.md` §2, not accessible from here).

**PT:**
- **Fronteira de escopo, RCSS embutida em `.hpp`:** `glintfx/src/ua_stylesheet.hpp` (produção,
  shippado) só entra nas checagens cruzadas da seção 3.8/comma-list contra o `rmlx-subset.md`, nunca nos
  números primários deste documento (tabela da seção 2), pelo escopo explícito "62 `.rml` + 23 `.rcss`"
  desta tarefa. `glintfx/tests/uix_style/lexer_corpus_sanity.cpp` (fixture de teste, adjacente à
  implementação que esta tarefa exclui) não é lido nem incluído em lugar nenhum, de propósito.
- **Pequenos resíduos não reconciliados** (todos 1-5%, nenhum investigado até zero): contagens de
  unidade `px`/`%`/sem-unidade/`auto` (§3.8), subtotal `%` da família (a) (§3.2), contagens de shorthand
  1-valor de `margin`/`padding` (§4.2). Cada um reportado com seu número medido exato em vez de ajustado
  pra bater com o documento, pela própria instrução desta tarefa de "reporte divergência, não reconcilie
  em silêncio".
- **Este censo não verifica:** nada dentro de `glintfx/src/uix/style/**` ou `glintfx/src/rml/rcss_dump*`
  (excluído pelo brief, pra proteger a independência de autor do oráculo diferencial), o código-fonte de
  expansão de shorthand do próprio RmlUi (fora do escopo de leitura deste censo), nem o repositório
  próprio do GusWorld (a fonte das cifras `:hover`=53/`:focus`=3/`:active`=2 no `rmlx-subset.md` §2, não
  acessível daqui).
