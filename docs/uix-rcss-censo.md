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

⚠️ **Anchored by section + literal snippet, not by line number -- see section 0b below for why.** The
first version of this table cited `uix-rcss.md:<line>` for all 17; every one of those 20 line numbers
(17 citations, `uix-rcss.md`+`rmlx-subset.md` together, some cited twice across EN/PT) went stale the
same week, when `UIX-RCSS-ERRATA-2` (`bdf3f45`) inserted +693 lines into `uix-rcss.md`. A line number is
kept below **only** as a courtesy, explicitly labeled "valid at `<sha>`" -- never the primary anchor.

| # | Doc | Section | Anchor snippet (search for this, not a line number) | Language | Claim (paraphrased) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | `uix-rcss.md` | §4 | `:hover`: 53-37 uses depending on which repository is counted, always in a composite selector | EN | `:hover` 53-37 uses depending on repo, always composite; `:focus` 3, `:active` 2 |
| 2 | `uix-rcss.md` | §4 (PT) | mirror of #1's own sentence, translated | PT | mirror of #1 |
| 3 | `uix-rcss.md` | §5 | a `%` value means three geometrically unrelated things depending on which property carries it, evidenced by the census | EN | `%` means 3 unrelated things per property (families a/b/c), evidenced by census §5.1 |
| 4 | `uix-rcss.md` | §5 (PT) | mirror of #3 | PT | mirror of #3 |
| 5 | `uix-rcss.md` | §6 | this registry is built **exclusively** from names the census measured at least once | EN | the 72-entry property registry is built exclusively from names the census measured (§3) |
| 6 | `uix-rcss.md` | §6 (PT) | mirror of #5 | PT | mirror of #5 |
| 7 | `uix-rcss.md` | §6.1 | the **only** 2 of the 72 longhand entries with **zero** measured occurrences anywhere in this document's own corpus | EN | `max-height`/`max-width` are the only 2 of 72 registry entries with zero corpus occurrences |
| 8 | `uix-rcss.md` | §6.1 (PT) | mirror of #7, adds "não alcançáveis por nenhum dos 13 shorthands" | PT | mirror of #7, adds "not reachable through any of the 13 shorthands" |
| 9 | `uix-rcss.md` | §7.1 | authorized by the census's own measured 4 hex forms | EN | 4 hex color forms measured (`#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`), corrects a stale comment |
| 10 | `uix-rcss.md` | §7.1 (PT) | mirror of #9 | PT | mirror of #9 |
| 11 | `uix-rcss.md` | §8 | Every measured length in the corpus sits in the 0-3000 range | EN | every measured length sits in the 0-3000 range; largest single value `-228dp` -- ⚠️ **already corrected in-place by `UIX-RCSS-ERRATA-2` before this census's own commit landed, see section 6.1 below** |
| 12 | `uix-rcss.md` | §8 (PT) | mirror of #11 | PT | mirror of #11 |
| 13 | `uix-rcss.md` | §8.2 | 100% of the corpus's angle usage ... is already authored in degrees | EN | angle usage: 24 instances, all `deg`, 0 `rad` |
| 14 | `uix-rcss.md` | §8.2 (PT) | mirror of #13 | PT | mirror of #13 |
| 15 | `uix-rcss.md` | §9.2 | In-scope functions, per the census's own measured decorator sub-languages | EN | in-scope decorator/filter/mask-image function set, per census §9's measured sub-languages -- ⚠️ **`vertical-gradient` gap already closed by `UIX-RCSS-ERRATA-2`, see section 6.2 below** |
| -- | `uix-rcss.md` | §9.2 (PT, deferred) | *(PT section explicitly says "mesmo conteúdo técnico" instead of re-citing -- no independent PT anchor exists for #15)* | | |
| 16 | `rmlx-subset.md` | §6 | an independent measurement (`qa-engineer`, read-only, no repo writes) covering 62 source files, 866 style blocks, 3424 declarations | EN | header line: 62 source files, 866 style blocks, 3424 declarations |
| 17 | `rmlx-subset.md` | §6 (PT) | mirror of #16 | PT | mirror of #16 |

⚠️ **The task brief said "17 citações" and named `docs/uix-rcss.md` as the document to read them from.**
Counted literally at the commit this census was first written against (`1173ae3`), `docs/uix-rcss.md`
alone had **15** occurrences of the census path, not 17. The other 2 lived in `docs/rmlx-subset.md`,
which `uix-rcss.md`'s own cross-ref block names as part of the same contract. 17 is correct **across the
two documents together**; this is recorded as a measured correction to the brief's own count, not
silently absorbed, per this task's own standing rule (divergence is reported, never quietly fixed).

### 0b. Citation anchoring in a living document: line number rots, content doesn't / Ancoragem de citação em documento vivo: número de linha apodrece, conteúdo não

**EN:** This section exists because this census's own first commit (`0de859f`) cited all 17 sources by
`file:line`, and every one of those 20 line-number citations (17 sources, some cited on 2 lines across
EN/PT) went stale **the same week** -- `UIX-RCSS-ERRATA-1` and `UIX-RCSS-ERRATA-2` (`f747ae8`, `bdf3f45`)
inserted hundreds of lines into `uix-rcss.md` fixing 7 BLOQUEIA + 3 PROVÁVEL findings from an independent
ambiguity audit, and `uix-rcss.md:209` stopped being the `:hover` sentence and started being unrelated
prose about a different finding entirely. **The rule, going forward, for this document and any future
one that cites a spec under active errata churn:** cite **section number + a short literal snippet**
(quoted verbatim, searchable), never a bare line number as the primary anchor. A line number may be kept
as a reader convenience **only if labeled `valid at <sha>`** -- unlabeled, it is a reference that lies
silently the moment the cited document gains or loses a single line above the citation. The snippet
anchor has a second, deliberate property: when the quoted text **stops existing** in the cited document,
that is not a bug in this census, it is a **signal** that the underlying claim itself changed --
information worth having, not noise to route around.

**PT:** Esta seção existe porque o primeiro commit deste censo (`0de859f`) citou as 17 fontes por
`arquivo:linha`, e cada uma dessas 20 citações de linha (17 fontes, algumas citadas em 2 linhas entre
EN/PT) apodreceu **na mesma semana** -- a `UIX-RCSS-ERRATA-1` e a `UIX-RCSS-ERRATA-2` (`f747ae8`,
`bdf3f45`) inseriram centenas de linhas no `uix-rcss.md` consertando 7 achados BLOQUEIA + 3 PROVÁVEL de
uma auditoria de ambiguidade independente, e `uix-rcss.md:209` parou de ser a frase do `:hover` e virou
prosa sem relação nenhuma sobre um achado diferente. **A regra, daqui em diante, pra este documento e
qualquer futuro que cite uma spec sob errata ativa:** citar **número de seção + um trecho literal curto**
(citado ao pé da letra, buscável), nunca um número de linha cru como âncora primária. Um número de linha
pode ficar como conveniência de leitor **só se marcado `válido em <sha>`** -- sem marca, é uma referência
que mente em silêncio no instante em que o documento citado ganha ou perde uma linha só acima da
citação. A âncora de trecho tem uma segunda propriedade, deliberada: quando o texto citado **deixa de
existir** no documento-alvo, isso não é bug deste censo, é **sinal** de que a própria afirmação mudou --
informação que vale a pena ter, não ruído pra contornar.

**Other line-number citations to `uix-rcss.md`/`uix-dom.md` found while checking, NOT fixed here (not
this census's document to edit):** `docs/uix-rcss-ambiguidades.md:284` and `:716` cite
`` `docs/uix-dom.md:71-72` ``; `TODO.md`'s `RMLX1-MAPA-FATIAS` entry cites `` `uix-dom.md:729` ``. Both
reported to the coordinator; per this task's own rule ("não saia consertando documento de outro dono"),
left for their owners to decide. / **Outras citações por número de linha a `uix-rcss.md`/`uix-dom.md`
achadas ao checar, NÃO consertadas aqui (não é documento deste censo pra editar):** `docs/uix-rcss-
ambiguidades.md:284` e `:716` citam `` `docs/uix-dom.md:71-72` ``; o item `RMLX1-MAPA-FATIAS` do
`TODO.md` cita `` `uix-dom.md:729` ``. Ambas reportadas ao coordenador; pela própria regra desta tarefa
("não saia consertando documento de outro dono"), deixadas para os donos decidirem.

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

**What `uix-rcss.md` §4's "53-37... `:focus`: 3, `:active`: 2" sentence actually is:** these four numbers
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

**O que a frase "53-37... `:focus`: 3, `:active`: 2" da §4 do `uix-rcss.md` realmente é:** esses quatro
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
`uix-rcss.md` §13's own exclusion list -- anchor snippet: "measured **zero** in the census despite `font-family`" -- without a `/var/tmp` path on that specific sentence but backed by the same census).

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
documento, seção 13 -- trecho-âncora: "medidas **zero** no censo apesar de" -- sem caminho `/var/tmp` nesse trecho específico mas
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

**Verdict / Veredito: PARTIAL DIVERGE -- already closed by `UIX-RCSS-ERRATA-2`.** The range claim
(0-3000, 3-digit-max order of magnitude, hence the precision argument) is correct; the specific cited
example (`-228dp` as "the largest single value observed") was not the largest by either sign -- `999dp`
and `-410dp` both exceed it. Reported as a finding, the underlying spec conclusion (4 decimal digits is
enough precision) is unaffected because it only depends on the *order of magnitude* staying 3-digit,
which both the cited and the actual record share. ✅ **`uix-rcss.md` §8 was corrected in-place by
`UIX-RCSS-ERRATA-2` (`bdf3f45`, 2026-08-06), citing `tools/rcss_census.py` by name and this census's own
`999dp`/`-410dp` numbers verbatim** -- the tech-lead independently re-verified both before applying the
fix (own words: "verified directly by the `tech-lead`, not merely trusted from the report that raised
them"). This is the loop this census exists to close: measurement → reported divergence → verified →
fixed in the document that mattered, not left as a standing correction only this census knows about.
/ **Veredito: DIVERGÊNCIA PARCIAL -- já fechada pela `UIX-RCSS-ERRATA-2`.** ✅ A §8 do `uix-rcss.md` foi
corrigida no lugar pela `UIX-RCSS-ERRATA-2` (`bdf3f45`, 2026-08-06), citando o `tools/rcss_census.py`
pelo nome e os números `999dp`/`-410dp` deste censo ao pé da letra -- o tech-lead reverificou os dois de
forma independente antes de aplicar o conserto. É o laço que este censo existe pra fechar: medição →
divergência reportada → verificada → corrigida no documento que importava, não deixada como correção
paralela que só este censo conhece.

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

**Verdict / Veredito: DIVERGE (significant) -- already closed by `UIX-RCSS-ERRATA-2`, and named by the
errata itself as "the single most consequential fix in this entire errata pass."** ✅ **`uix-rcss.md`
§9.2 gained a `vertical-gradient` row (`UIX-RCSS-ERRATA-2`, `bdf3f45`)**, citing 107 corpus occurrences
across 16 files -- this census's own number, reproduced exactly, plus the errata's own consequence
analysis this census did not attempt: combined with a separate fix to the fail-high rule (an unknown
decorator function previously, wrongly, dropped only itself; corrected to drop the **whole property**),
the pre-fix table would have made **both** independent oracle sides silently agree on the same wrong
empty output for every one of those 107 declarations -- "a green oracle for the wrong reason," in the
errata's own words. `docs/effects.md` (out of this census's read scope, not excluded by the brief, just
not read to stay focused on the corpus) is not what closed the gap; the corpus measurement is what did.
/ **Veredito: DIVERGÊNCIA (significativa) -- já fechada pela `UIX-RCSS-ERRATA-2`, e nomeada pela própria
errata como "o conserto mais consequente de toda esta passada de errata."** ✅ A §9.2 do `uix-rcss.md`
ganhou uma linha `vertical-gradient` (`UIX-RCSS-ERRATA-2`, `bdf3f45`), citando 107 ocorrências de corpus
em 16 arquivos -- o número deste censo, reproduzido exato, mais a análise de consequência da própria
errata que este censo não tentou: combinado com um conserto separado na regra fail-high (uma função de
decorator desconhecida antes, errado, derrubava só a si mesma; corrigido pra derrubar a propriedade
**inteira**), a tabela pré-conserto teria feito os **dois** lados independentes do oráculo concordarem em
silêncio na mesma saída vazia errada em cada uma dessas 107 declarações -- "um oráculo verde pelo motivo
errado," nas palavras da própria errata.

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

**Verdict / Veredito: DIVERGE (significant, high-value finding) -- already closed by
`UIX-RCSS-ERRATA-2`.** This is exactly the class of claim the task brief calls out as most valuable to
catch -- an **absence/frequency** claim nobody could re-verify without the report, now measured directly
against the surviving corpus and found reversed. Does not change section 9.1's own *algorithm* (spread
still defaults to `0.0000px` when omitted, that mechanism is a code fact this census does not
adjudicate) -- only the corpus-frequency claim used to justify why the default matters. ✅
**`uix-rcss.md` §9.1 was corrected in-place (`UIX-RCSS-ERRATA-2`, `bdf3f45`)**, citing an independent
re-measurement ("script + the tech-lead's own direct declaration-by-declaration walk") that landed at
**123 of 124** single-layer declarations specifying `spread` explicitly, only 1 omitting it -- same
finding, same direction, a **small residual against this census's own 127/128** (the errata's own text
does not explain the 4-declaration gap, and this census does not either; both numbers agree on what
matters -- the ratio is reversed from the original "124 of 135 omit" claim, not merely off by a little
-- and the residual is noted here rather than silently reconciled to either number). / **Veredito:
DIVERGÊNCIA (significativa, achado de alto valor) -- já fechada pela `UIX-RCSS-ERRATA-2`.** ✅ A §9.1 do
`uix-rcss.md` foi corrigida no lugar (`UIX-RCSS-ERRATA-2`, `bdf3f45`), citando uma remedição
independente ("script + a própria varredura declaração-por-declaração direta do tech-lead") que chegou
em **123 de 124** declarações single-layer especificando `spread` explicitamente, só 1 omitindo -- mesmo
achado, mesma direção, um **pequeno resíduo contra o 127/128 próprio deste censo** (o texto da própria
errata não explica a lacuna de 4 declarações, e este censo também não -- os dois números concordam no
que importa: a proporção está invertida contra a afirmação original "124 de 135 omitem", não só um pouco
fora; o resíduo fica registrado aqui em vez de reconciliado em silêncio com qualquer um dos dois
números).

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

## 5. The third origin: RCSS embedded in C++ (production + test/demo) / A terceira origem: RCSS embutida em C++ (produção + teste/demo)

**EN:** This section did not exist in this census's first commit (`0de859f`). The coordinator corrected
it: RCSS in this repository lives in **three** places, not two --- `.rcss` files, `<style>` blocks
inside `.rml` (both section 2 already covers), and **C++ string/raw-string literals** in `.cpp`/`.hpp`
test fixtures and one production file. Excluding the third origin was a scope decision this census made
explicitly (section 6 of the first commit said so), but the exclusion had a real, measured cost: 3 of
the original census's own 15 comma-list-selector instances live **only** in the third origin
(`glintfx/src/ua_stylesheet.hpp`), including the single most important example -- the 16-tag rule that
is the **first rule in the file** and sets `display: block` on every structural element of every
document glintfx renders. A census that excludes that file cannot reproduce the evidence that justified
authorizing comma-list selectors into the `RMLX-2` subset in the first place.

**PT:** Esta seção não existia no primeiro commit deste censo (`0de859f`). O coordenador corrigiu: RCSS
neste repositório mora em **três** lugares, não dois -- arquivos `.rcss`, blocos `<style>` dentro de
`.rml` (os dois já cobertos pela seção 2), e **literais de string/raw-string de C++** em fixtures de
teste/demo `.cpp`/`.hpp` e um arquivo de produção. Excluir a terceira origem foi uma decisão de escopo
que este censo tomou explicitamente (a seção 6 do primeiro commit dizia isso), mas a exclusão teve um
custo real, medido: 3 das próprias 15 instâncias de seletor comma-list do censo original só vivem na
terceira origem (`glintfx/src/ua_stylesheet.hpp`), incluindo o exemplo mais importante sozinho -- a
regra de 16 tags que é a **primeira regra do arquivo** e põe `display: block` em todo elemento
estrutural de todo documento que a glintfx renderiza. Um censo que exclui esse arquivo não consegue
reproduzir a evidência que justificou autorizar seletor comma-list no subconjunto da `RMLX-2` pra
começo de conversa.

### 5.1 Discovery method: structural, not filename guessing / Método de descoberta: estrutural, não chute de nome de arquivo

**EN:** `tools/rcss_census.py`'s `find_cpp_embedded_rcss_files()` scans every git-tracked `.cpp`/`.hpp`
(excluding the same two forbidden implementation paths as always) for at least one **self-contained,
brace-balanced fragment** matching a known RCSS property name -- built from three layers, each added
because a looser one produced a wrong answer first:

1. **Adjacent C++ string literals separated only by whitespace are merged into one run** (`merge_adjacent_
   literal_runs`) -- this is not a heuristic, it is C++'s own literal-concatenation rule (`"a" "b"` is
   byte-identical to `"ab"`), so merging loses zero information. A run broken by real code (`<< some_var
   <<`) stays broken, and is reported as a **gap**, not guessed.
2. **Raw-string literals** (`R"delim(...)delim"`) are extracted whole, never merged with a neighbour
   (already complete on their own).
3. **Each run is fed to the SAME brace/string-aware block scanner section 1 already uses**
   (`parse_stylesheet`) -- a run is only counted as a real, extractable block if the scanner finds a
   **closed** `{...}` pair; an unclosed brace (because the run was truncated by interpolation, or
   because a test deliberately never closes it -- see 5.3) is routed to `leftover`, never silently
   treated as valid content. This required a real fix to the scanner itself: a first version accepted
   `depth != 0` at end-of-text as if the block had closed, which would have **fabricated** content for
   every truncated/malformed fixture in this section; fixed to route an unclosed brace to leftover,
   tagged, before any of this section's files were parsed.

**Classification (production vs test/demo) is by CMake registration, not filename pattern.** A file is
"test/demo" if its path is under `glintfx/tests/`/`glintfx/demos/`, OR its basename is a source argument
of an `add_executable(...)` call in some `glintfx/tests/**/CMakeLists.txt` -- checked by parsing that
call's own argument list, not a substring search of the whole CMake file (a substring search would have
misclassified `ua_stylesheet.hpp` as test/demo, because its basename appears inside a
`target_compile_definitions(...)` string in `glintfx/tests/uix_style/CMakeLists.txt` -- a path handed to
a test at runtime, not a source the test is built from). **A first version of this rule guessed by
filename suffix** (`*_sanity.cpp`/`*_smoke.cpp`/etc.) and silently misclassified
`glintfx/src/rml/dom_dump_spec_conformance.cpp` as production -- its own header doc-comment says
"byte-exact oracle", plainly a test, but its name ends in `_conformance`, not one of the guessed
suffixes; `grep -l dom_dump_spec_conformance glintfx/tests/CMakeLists.txt` finds it registered there
directly, alongside its sibling `dom_dump_determinism_sanity.cpp`, even though both `.cpp` files live
under `glintfx/src/rml/` by this repo's own convention (the RMLX-1 DOM-dump oracle tests are co-located
with the implementation they test). The CMake-registration check replaced the filename guess before this
section's numbers were produced.

**PT:** O `find_cpp_embedded_rcss_files()` do `tools/rcss_census.py` varre todo `.cpp`/`.hpp` rastreado
pelo git (excluindo os mesmos dois caminhos de implementação proibidos de sempre) atrás de pelo menos um
**fragmento auto-contido, balanceado em chave** casando um nome de propriedade RCSS conhecido -- montado
em três camadas, cada uma acrescentada porque uma mais frouxa deu resposta errada primeiro: ver os 3
itens acima (bilíngue). **A classificação (produção vs teste/demo) é por registro no CMake, não padrão
de nome de arquivo.** Ver parágrafo acima (bilíngue) -- uma primeira versão chutava por sufixo de nome e
classificou em silêncio `glintfx/src/rml/dom_dump_spec_conformance.cpp` como produção; a checagem de
registro no CMake substituiu o chute antes dos números desta seção serem produzidos.

### 5.2 File inventory: 1 production + 15 test/demo / Inventário de arquivos: 1 produção + 15 teste/demo

**EN:** Reproducible via `python3 tools/rcss_census.py --repo-root . --list-cpp-embedded`. "Blocks"/
"declarations" are what the scanner **closed and extracted with full confidence**; "truncated runs" are
runs the scanner refused to guess at (count and byte-length shown, never fabricated):

| File | Category | Closed blocks | Declarations | Truncated runs |
| :--- | :--- | ---: | ---: | ---: |
| `glintfx/src/ua_stylesheet.hpp` | **production** | 9 | 13 | 1 (1 byte, trivial) |
| `glintfx/src/rml/dom_dump_determinism_sanity.cpp` | test/demo | 4 | 4 | 4 |
| `glintfx/src/rml/dom_dump_spec_conformance.cpp` | test/demo | 2 | 2 | 2 |
| `glintfx/tests/app_draw2d_smoke.cpp` | test/demo | 1 | 3 | 1 |
| `glintfx/tests/asset_decode_hostile_sanity.cpp` | test/demo | 2 | 6 | 3 |
| `glintfx/tests/draw2d_ripple_coexist_sanity.cpp` | test/demo | 3 | 13 | 1 (1 byte, trivial) |
| `glintfx/tests/draw2d_ui_coexist_sanity.cpp` | test/demo | 1 | 2 | 1 |
| `glintfx/tests/uix/dom_tree_sanity.cpp` | test/demo | 2 | 2 | 2 |
| `glintfx/tests/uix/dumper_determinism_sanity.cpp` | test/demo | 1 | 1 | 1 |
| `glintfx/tests/uix/dumper_sanity.cpp` | test/demo | 2 | 2 | 2 |
| `glintfx/tests/uix/lexer_tokens_sanity.cpp` | test/demo | 5 | 5 | 5 |
| `glintfx/tests/uix/parser_hardening_sanity.cpp` | test/demo | 1 | 1 | 1 |
| `glintfx/tests/uix/parser_tokens_sanity.cpp` | test/demo | 2 | 2 | 2 |
| `glintfx/tests/uix/spec_conformance_sanity.cpp` | test/demo | 2 | 2 | 2 |
| `glintfx/tests/uix_style/lexer_hardening_sanity.cpp` | test/demo | 4 | 6 | 8 |
| `glintfx/tests/uix_style/lexer_tokens_sanity.cpp` | test/demo | 14 | 22 | 14 |
| **Total** | **1 production + 15 test/demo = 16** | **55** | **85** | **50** |

**PT:** Reproduzível via `python3 tools/rcss_census.py --repo-root . --list-cpp-embedded`. "Blocos"/
"declarações" são o que o scanner **fechou e extraiu com confiança total**; "runs truncados" são trechos
que o scanner recusou a chutar (contagem e tamanho em bytes mostrados, nunca fabricados): ver tabela
acima (bilíngue).

### 5.3 What did NOT get extracted, and why / O que NÃO foi extraído, e por quê

**EN:** Per this task's own instruction ("declare not-extracted with reason instead of guessing"), three
distinct reasons account for every truncated run, none silently padded with a guessed value:

1. **Runtime interpolation (`<<`)** -- `app_draw2d_smoke.cpp`, `asset_decode_hostile_sanity.cpp`, and
   `draw2d_ui_coexist_sanity.cpp` each build one rule (`#box { ... width: <<box_size<<px; ... }` or
   `img { ... width: <<dp_size<<dp; ... }`) whose numeric value is a C++ **variable**, not a literal --
   there is no static text that says what number ships at test time, so this census does not print one.
   The surrounding **static** text (e.g. `app_draw2d_smoke.cpp`'s own `body { margin: 0; padding: 0;
   background-color: #0a1428; }`, a complete, separate, self-closing rule with no interpolation inside
   it) **is** extracted, because it is not truncated -- only the one interpolated rule per file is
   skipped, not the whole file.
2. **Deliberately malformed fixtures** -- `glintfx/tests/uix_style/lexer_hardening_sanity.cpp` embeds
   `a { font-family: "never closed` (an intentionally unterminated string, testing the lexer's own
   hardening against exactly this input) and `a{;;color:red;;}` (empty declarations between semicolons).
   These are real embedded text, extracted where brace-balanced (`a{;;color:red;;}` closes and is
   counted), left as truncated leftover where they do not (`"never closed` never closes, correctly never
   counted as a valid declaration).
3. **`glintfx/tests/uix_style/lexer_corpus_sanity.cpp` embeds no independent text at all** -- found by
   the `R"rcss(` marker in this census's very first pass, its own header doc-comment says it "extracts
   the RCSS payload embedded in `ua_stylesheet.hpp`'s own raw string" **at test runtime**, reading the
   production file directly rather than duplicating its content -- the file's own raw-string literal is
   a 5-character placeholder (`" ... "`), not real corpus text. Correctly excluded from the 16-file
   inventory above (0 closed blocks, 0 truncated bytes -- it does not appear in section 5.2's table
   because it produced neither).

**PT:** Pela própria instrução desta tarefa ("declare não-extraído com o motivo em vez de adivinhar"),
três motivos distintos respondem por todo run truncado, nenhum preenchido em silêncio com valor chutado:
ver os 3 itens acima (bilíngue) -- interpolação em runtime (`<<`), fixture deliberadamente malformada, e
o caso do `lexer_corpus_sanity.cpp` que não embute texto independente nenhum (lê `ua_stylesheet.hpp`
direto em runtime, o próprio raw-string dele é só um placeholder de 5 caracteres).

### 5.4 Validation: this section reproduces `rmlx-subset.md`'s own comma-list numbers exactly / Validação: esta seção reproduz exatos os próprios números de comma-list do `rmlx-subset.md`

**EN:** `docs/rmlx-subset.md` section 6.1 already states, from the original (now-lost) census: **"15
instâncias across 8 arquivos-fonte"** in the census's own prose, but **"15 rows, 13 unique paths"**
counted by hand against that same report's own exhaustive table -- and names 3 of those 15 rows as
living in `glintfx/src/ua_stylesheet.hpp`. Folding **only the production file** into this census (85
files + `ua_stylesheet.hpp` = 86) reproduces **both** numbers exactly:

```
python3 tools/rcss_census.py --repo-root . --include-hpp glintfx/src/ua_stylesheet.hpp
  -> comma_list_rules = 15
  -> len(files_with_comma_list) = 13
```

This is the strongest cross-validation in this entire census: two independently-arrived-at headline
numbers (rows and distinct files), from a report that no longer exists, reproduced byte-for-byte by a
different tool measuring a different (but overlapping) file set. Folding in the **full** third origin
(85 + all 16 C++-embedded files = 101) moves the number to **16** comma-list rules -- one more than the
original census's own 15 -- because `glintfx/tests/uix_style/lexer_tokens_sanity.cpp` embeds a **verbatim
copy** of `data_model_embed_scene.rcss`'s own comma-list selector (`#hpbox.wide, #namebox.wide,
#flagwide.wide { display: block; }`, an existing `.rcss` fixture's rule reused as a lexer unit-test
input) -- real text, correctly extracted, but **not new corpus evidence**, an echo of a rule this census
already counted once in section 2. This is why production and test/demo are never summed into one
number without the breakdown: production is new evidence, test/demo can include verbatim echoes of
evidence counted elsewhere.

**PT:** A seção 6.1 do `docs/rmlx-subset.md` já declara, do censo original (hoje perdido): **"15
instâncias em 8 arquivos-fonte"** na prosa do próprio censo, mas **"15 linhas, 13 caminhos únicos"**
contados à mão contra a tabela exaustiva do mesmo relatório -- e nomeia 3 dessas 15 linhas como vivendo
em `glintfx/src/ua_stylesheet.hpp`. Incluir **só o arquivo de produção** neste censo (85 arquivos +
`ua_stylesheet.hpp` = 86) reproduz **os dois** números exatos: ver comando acima (bilíngue) -- resultado
`comma_list_rules = 15`, `13` arquivos distintos.

É a validação cruzada mais forte deste censo inteiro: dois números-manchete chegados de forma
independente (linhas e arquivos distintos), de um relatório que não existe mais, reproduzidos byte a
byte por uma ferramenta diferente medindo um conjunto de arquivo diferente (mas sobreposto). Incluir a
terceira origem **inteira** (85 + os 16 arquivos com C++ embutido = 101) move o número pra **16** regras
comma-list -- uma a mais que as 15 próprias do censo original -- porque
`glintfx/tests/uix_style/lexer_tokens_sanity.cpp` embute uma **cópia verbatim** do próprio seletor
comma-list de `data_model_embed_scene.rcss` (`#hpbox.wide, #namebox.wide, #flagwide.wide { display:
block; }`, a regra de uma fixture `.rcss` já existente reusada como entrada de teste unitário do lexer)
-- texto real, extraído corretamente, mas **não é evidência nova de corpus**, é um eco de uma regra que
este censo já contou uma vez na seção 2. É por isso que produção e teste/demo nunca são somados num
número só sem a decomposição: produção é evidência nova, teste/demo pode incluir eco verbatim de
evidência já contada em outro lugar.

### 5.5 Side-by-side numbers, all three tiers / Números lado a lado, os três níveis

**EN:** Reproducible: `--out primary.json` (no flag), `--include-hpp glintfx/src/ua_stylesheet.hpp`
(+production), `--cpp-embedded` (+production +test/demo, full 101-file scope):

| Metric | Primary (85 files) | +production (86 files) | +production+test/demo (101 files) | `rmlx-subset.md` §6 claim |
| :--- | ---: | ---: | ---: | ---: |
| Top-level style blocks | 850 | 859 | 905 | 866 |
| Declarations | 3388 | 3401 | 3469 | 3424 |
| Comma-list rules | 12 | **15** | 16 | **15** |
| Distinct files with a comma-list rule | 12 | **13** | 14 | **13** |
| `:hover` (composite, this repo's corpus) | 37 | 37 | 38 | 37 (`rmlx-subset.md` §6.2's own table) |
| `dp` unit instances | 2231 | **2237** | 2248 | **2237** |
| `px` unit instances | 330 | 330 | 338 | 334 |
| `%` unit instances | 209 | 209 | 209 | 207 |
| unitless numeric instances | 163 | 163 | 181 | 171 |

**Reading this table:** the **+production** column (86 files) is the closest reconstruction of what the
original, now-lost census actually measured -- it reproduces the comma-list headline numbers (15/13)
exactly and the `dp` unit count (2237) exactly, strong evidence the original census's own file scope
included `ua_stylesheet.hpp` even though it is not a `.rml`/`.rcss` file. The **+production+test/demo**
column (101 files, the full third origin) is this census's own, more complete measurement -- useful for
completeness, but includes at least one verbatim echo (section 5.4) and is not expected to match the
original census's own numbers, because there is no evidence the original census's scope extended to test
fixtures at all. `top_level_blocks`/`declarations` still fall short of `rmlx-subset.md`'s own 866/3424
even at the full 101-file scope (859/3401 at +production, 905/3469 at full scope) -- a residual this
census does not close, plausibly a fourth source this census's search did not find, or a different
counting convention (e.g. whether a `@keyframes` step counts as its own "block"); left open rather than
padded to match.

**PT:** Reproduzível: `--out primary.json` (sem flag), `--include-hpp glintfx/src/ua_stylesheet.hpp`
(+produção), `--cpp-embedded` (+produção +teste/demo, escopo completo de 101 arquivos): ver tabela acima
(bilíngue).

**Lendo esta tabela:** a coluna **+produção** (86 arquivos) é a reconstrução mais próxima do que o censo
original, hoje perdido, de fato mediu -- reproduz os números-manchete de comma-list (15/13) exatos e a
contagem de unidade `dp` (2237) exata, evidência forte de que o escopo de arquivo do censo original
incluía `ua_stylesheet.hpp` mesmo não sendo um arquivo `.rml`/`.rcss`. A coluna **+produção+teste/demo**
(101 arquivos, a terceira origem inteira) é a medição própria, mais completa, deste censo -- útil pra
completude, mas inclui pelo menos um eco verbatim (seção 5.4) e não se espera que bata com os números
próprios do censo original, porque não há evidência de que o escopo do censo original se estendesse a
fixture de teste. `top_level_blocks`/`declarações` ainda ficam abaixo dos próprios 866/3424 do
`rmlx-subset.md` mesmo no escopo completo de 101 arquivos (859/3401 em +produção, 905/3469 no escopo
completo) -- um resíduo que este censo não fecha, plausivelmente uma quarta fonte que a busca deste
censo não achou, ou uma convenção de contagem diferente (ex.: se um passo de `@keyframes` conta como
"bloco" próprio); deixado em aberto em vez de forçado a bater.

---

## 6. Verdict summary / Resumo de veredito

| # | Claim | Verdict |
| :--- | :--- | :--- |
| 1-2 | `:hover` 53-37, `:focus` 3, `:active` 2 | **MATCH** for `:hover`=37 (this repo); clarified scope (GusWorld vs this repo) for `:focus`/`:active` |
| 3-4 | 3 percentage families, family (a) 103/207 | **CLOSE** (105/209 measured, +2/+2) |
| 5-6 | Registry built exclusively from measured names | **MATCH** (methodology confirmed, `max-height`/`max-width`/`font-style`/`font-weight` all 0 as claimed) |
| 7-8 | `max-height`/`max-width` zero-corpus, not shorthand-reachable | **MATCH** (zero confirmed; shorthand-reachability is a code fact, not independently re-derived here) |
| 9-10 | 4 hex forms, corrects stale comment | **MATCH exact** |
| 11-12 | Length range 0-3000, largest `-228dp` | **PARTIAL DIVERGE, ✅ fixed** -- range holds; `-228dp` was not the actual max (`999dp`/`-410dp` larger); corrected in-place by `UIX-RCSS-ERRATA-2` (`bdf3f45`), citing this census's own numbers |
| 13-14 | Angle: 24 instances, all `deg`, 0 `rad` | **MATCH exact** |
| 15 | In-scope decorator function set | **DIVERGE, ✅ fixed** -- `vertical-gradient` (107 instances, single most-used decorator function) was absent from §9.2's own table; `UIX-RCSS-ERRATA-2` added the row, citing this census's number exactly, and named it "the single most consequential fix in this entire errata pass" |
| 16-17 | 62 files / 866 blocks / 3424 declarations | **CLOSE** (85-file primary scope: 850/3388; +production, 86 files: 859/3401; +production+test/demo, 101 files: 905/3469 -- none matches exactly, but the `dp`-unit exact match at +production, and the comma-list 15/13 exact match at +production, are strong evidence the original scope included `ua_stylesheet.hpp` -- section 5) |
| bonus | `box-shadow` spread omitted 124/135 | **DIVERGE (significant), ✅ fixed** -- measured near-inverse: 127/128 single-layer declarations specify spread explicitly, only 1 omits it; `UIX-RCSS-ERRATA-2` corrected the ratio (independently re-measured at 123/124, a small residual against this census's own 127/128, same direction/conclusion) |
| bonus | shorthand value-count shapes | **MATCH** (6/6 shorthands, small residual on `margin`/`padding` 1-value row only) |
| bonus | third origin: comma-list 15 rows/13 files (`rmlx-subset.md` §6.1) | **MATCH exact** at +production scope (86 files) -- section 5.4, the strongest cross-validation in this census |

**3 of 8 primary citations (#11-15, plus the box-shadow bonus finding) were already corrected in the
cited document by `UIX-RCSS-ERRATA-2` (`bdf3f45`, 2026-08-06) before this update to this census landed** --
all three citing this census's own tooling (`tools/rcss_census.py`) or its own numbers by name, and all
three independently re-verified by the `tech-lead` rather than trusted from this report alone. / **3 das
8 citações primárias (#11-15, mais o achado bônus do box-shadow) já foram corrigidas no documento citado
pela `UIX-RCSS-ERRATA-2` (`bdf3f45`, 2026-08-06) antes desta atualização deste censo chegar** -- as três
citando a própria ferramentagem deste censo (`tools/rcss_census.py`) ou seus próprios números pelo nome,
e as três reverificadas de forma independente pelo `tech-lead`, não confiadas só neste relatório.

---

## 7. Known gaps / unreconciled residuals / Lacunas conhecidas / resíduos não reconciliados

**EN:**
- **Scope boundary, `.hpp`-embedded RCSS -- CLOSED, section 5.** This was an open gap in this census's
  first commit (`0de859f`); the coordinator's correction added the discovery+extraction machinery
  (`--include-hpp`, `--cpp-embedded`, `--list-cpp-embedded`) and section 5 above presents the full
  three-tier numbers. `glintfx/tests/uix_style/lexer_corpus_sanity.cpp` remains correctly excluded --
  not by scope decision this time, but because it was measured (section 5.3) to embed no independent
  text at all (it reads `ua_stylesheet.hpp` at test runtime; its own raw string is a 5-character
  placeholder).
- **Small unreconciled residuals** (all 1-5%, none investigated to zero): `px`/`%`/unitless/`auto` unit
  counts (§3.8), `%` family-(a) subtotal (§3.2), `margin`/`padding` 1-value shorthand counts (§4.2), and
  the `top_level_blocks`/`declarations` gap against `rmlx-subset.md`'s own 866/3424 even at full
  101-file scope (§5.5). Each is reported with its exact measured number rather than adjusted to match
  the doc, per this task's own "report divergence, don't silently reconcile" instruction.
- **This census does not verify:** anything inside `glintfx/src/uix/style/**` or
  `glintfx/src/rml/rcss_dump*` (excluded by brief, to protect the differential oracle's author
  independence), the shorthand-expansion source in RmlUi itself (out of this census's read scope), or
  GusWorld's own repository (the source of the `:hover`=53/`:focus`=3/`:active`=2 figures in
  `rmlx-subset.md` §2, not accessible from here).

**PT:**
- **Fronteira de escopo, RCSS embutida em `.hpp` -- FECHADA, seção 5.** Era uma lacuna aberta no primeiro
  commit deste censo (`0de859f`); a correção do coordenador acrescentou a maquinaria de
  descoberta+extração (`--include-hpp`, `--cpp-embedded`, `--list-cpp-embedded`) e a seção 5 acima
  apresenta os números completos em três níveis. `glintfx/tests/uix_style/lexer_corpus_sanity.cpp`
  segue corretamente excluído -- não mais por decisão de escopo, mas porque foi medido (seção 5.3) que
  não embute texto independente nenhum (lê `ua_stylesheet.hpp` em runtime de teste; o próprio raw string
  dele é um placeholder de 5 caracteres).
- **Pequenos resíduos não reconciliados** (todos 1-5%, nenhum investigado até zero): contagens de
  unidade `px`/`%`/sem-unidade/`auto` (§3.8), subtotal `%` da família (a) (§3.2), contagens de shorthand
  1-valor de `margin`/`padding` (§4.2), e a lacuna de `top_level_blocks`/`declarações` contra os próprios
  866/3424 do `rmlx-subset.md` mesmo no escopo completo de 101 arquivos (§5.5). Cada um reportado com
  seu número medido exato em vez de ajustado pra bater com o documento, pela própria instrução desta
  tarefa de "reporte divergência, não reconcilie em silêncio".
- **Este censo não verifica:** nada dentro de `glintfx/src/uix/style/**` ou `glintfx/src/rml/rcss_dump*`
  (excluído pelo brief, pra proteger a independência de autor do oráculo diferencial), o código-fonte de
  expansão de shorthand do próprio RmlUi (fora do escopo de leitura deste censo), nem o repositório
  próprio do GusWorld (a fonte das cifras `:hover`=53/`:focus`=3/`:active`=2 no `rmlx-subset.md` §2, não
  acessível daqui).
