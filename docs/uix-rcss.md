# UIX RCSS computed-value dump format / Formato de dump de valor computado RCSS

> **EN:** The canonical, byte-exact text serialization of a parsed-and-cascaded RCSS stylesheet
> applied to a parsed document, used as the **sole contract** between the two independent
> differential-oracle dumpers for `RMLX-2` -- side **A** (walks real RmlUi `Style::ComputedValues`,
> confined to `glintfx/src/rml/`) and side **B** (walks glintfx's own RCSS engine, `src/uix/rcss/`),
> plus the harness that diffs their output. Diátaxis type: **reference**. Audience: the implementer
> of side A, the implementer of side B (each reads **only** this document, never the other's source
> -- that separation is deliberate, see "Why this document exists"), and whoever implements the
> diff harness. Owner: `software-architect`; written **2026-08-06** against `main` at `1173ae3`,
> before any `RMLX-2` slice (parser, cascade, side A, side B, harness) exists -- this document is
> the **first** artifact of the `RMLX-2` wave, not a description of code already written.
> **PT:** A serialização textual canônica, byte-exata, de uma folha RCSS já parseada e cascateada,
> aplicada a um documento já parseado, usada como **único contrato** entre os dois dumpers
> independentes de oráculo diferencial da `RMLX-2` -- o lado **A** (percorre `Style::ComputedValues`
> real do RmlUi, confinado a `glintfx/src/rml/`) e o lado **B** (percorre o motor RCSS próprio da
> glintfx, `src/uix/rcss/`), mais o harness que faz o diff dos dois. Tipo Diátaxis: **reference**.
> Audiência: quem implementa o lado A, quem implementa o lado B (cada um lê **só** este documento,
> nunca o fonte um do outro -- separação deliberada, ver "Por que este documento existe"), e quem
> implementar o harness de diff. Owner: `software-architect`; escrito em **2026-08-06** contra
> `main` em `1173ae3`, antes de qualquer fatia da `RMLX-2` (parser, cascata, lado A, lado B,
> harness) existir -- este documento é o **primeiro** artefato da onda `RMLX-2`, não a descrição de
> um código já escrito.

**Cross-ref:** [`docs/uix-dom.md`](uix-dom.md) (o gêmeo desta spec para a `RMLX-1`; esta spec reusa
o endereçamento de nó dele por completo -- ver seção 2 -- e segue a mesma disciplina de escaping,
ordenação byte-wise, e ledger de divergências de três classes), [`docs/rmlx-subset.md`](rmlx-subset.md)
(o subconjunto congelado que esta spec serve -- toda propriedade/seletor/unidade/função de decorator
que este documento não nomeia está fora de escopo, ver seção "A cláusula" abaixo),
[`docs/effects.md`](effects.md) (referência de sintaxe how-to dos decorators autorais da glintfx --
`polygon()`, `image-tint()`, `ripple()` -- que seção 9 desta spec formaliza em grade de serialização),
[`docs/adr/0020-rml-anticorruption-layer.md`](adr/0020-rml-anticorruption-layer.md) (a decisão que
criou a fronteira RmlUi-vs-glintfx), `TODO.md` linha `RMLX-2` (escopo/aceite/as três decisões do
líder que este documento implementa), `TODO.md` linha `UIX-HEAD-PREFIXO-CEGO` (o ponto cego medido
na `RMLX-1` que esta spec foi escrita para não repetir -- ver a seção seguinte).

---

## 🔴 Why this document exists, and the lesson it must not repeat / Por que este documento existe, e a lição que não pode repetir

**EN:** `RMLX-2`'s acceptance is "computed values identical to RmlUi's, proven by a differential
oracle" -- exactly the same shape of claim `RMLX-1` made, and `RMLX-1`'s own oracle just proved
that shape of claim is not enough on its own: `UIX-HEAD-PREFIXO-CEGO` found that **both** `RMLX-1`
dumpers, written by different agents reading only `docs/uix-dom.md`, independently discarded the
exact same byte range of the `HEAD` payload -- because that spec's own wording admitted two
readings, and both readers picked the same one. The oracle diffed two dumps that agreed with each
other and were **both wrong** against the spec's own stated intent. Author independence protects
against the same *misreading*; it does not protect against two authors reading an ambiguous
sentence the same correct-sounding way. This document's working discipline, applied line by line
below: **for every rule where a second implementer could plausibly land on a different byte, this
document gives the input and the exact expected output, not just the prose rule.** Where that
discipline could not be fully closed -- and there are places below where it could not -- this
document says so explicitly rather than papering over the gap, per this task's own instruction that
an unresolved ambiguity is the most valuable thing to report, not a weakness to hide.

**PT:** O aceite da `RMLX-2` é "valores computados idênticos aos do RmlUi, provados por um oráculo
diferencial" -- exatamente a mesma forma de afirmação que a `RMLX-1` fez, e o próprio oráculo da
`RMLX-1` acabou de provar que essa forma de afirmação não basta sozinha: a `UIX-HEAD-PREFIXO-CEGO`
achou que **os dois** dumpers da `RMLX-1`, escritos por agentes diferentes lendo só o
`docs/uix-dom.md`, descartaram de forma independente exatamente o mesmo trecho de bytes do payload
do `HEAD` -- porque a própria formulação daquela spec admitia duas leituras, e os dois leitores
escolheram a mesma. O oráculo comparou dois dumps que concordavam entre si e **os dois estavam
errados** contra a própria intenção declarada da spec. A independência de autor protege contra a
mesma *má leitura*; não protege contra dois autores lendo uma frase ambígua do mesmo jeito que soa
correto. A disciplina de trabalho deste documento, aplicada linha a linha abaixo: **para toda regra
em que um segundo implementer poderia plausivelmente chegar a um byte diferente, este documento dá
a entrada e a saída esperada exata, não só a regra em prosa.** Onde essa disciplina não pôde ser
fechada por completo -- e há pontos abaixo em que não pôde -- este documento diz isso explicitamente
em vez de disfarçar a lacuna, seguindo a própria instrução desta tarefa de que uma ambiguidade não
resolvida é o item mais valioso a reportar, não uma fraqueza a esconder.

---

## 🟡 Errata (`UIX-RCSS-ERRATA-1`, 2026-08-06) / Errata (`UIX-RCSS-ERRATA-1`, 2026-08-06)

**EN:** This document originally shipped (2026-08-06, `1173ae3`) with one **false statement**, found
and fixed the same day by the `tech-lead`'s own pre-flight review before the two `RMLX-2` dumpers
were dispatched -- exactly the failure mode section "Why this document exists" above warns about,
caught before either independent author read it, not after. **What was wrong:** section 6.2's own
table said the `FallThrough` algorithm for `border-top`/`-right`/`-bottom`/`-left` is *"order-
independent between the two"* (width vs. color). **It is not**, for a 2-item/2-token chain --
already proven by measurement in `UIX-PROP-REGISTRY` (`381624c`) before this errata: the reversed
order (color first, e.g. `#7A5A2E 1dp`) leaves the item cursor exhausted with the second token still
unclaimed, and upstream's own post-loop guard aborts the **whole shorthand**, not a partial result --
traced iteration-by-iteration against `examples/RmlUi/Source/Core/PropertySpecification.cpp:429-471`
(read directly, not paraphrased) and pinned by
`glintfx/tests/uix_style/shorthand_expansion_sanity.cpp`'s own
`test_border_top_fallthrough_order_is_load_bearing`. The **code and the test were already correct**
and already cited this false sentence as wrong (`glintfx/src/uix/style/shorthand.hpp:38`,
`shorthand.cpp:30`) -- only this document had not been corrected to match. **What changed:** section
6.2's table cell now states the real, order-sensitive rule; section 11 gained an explicit fail-high
case for a malformed shorthand value (the reversed-order declaration is dropped whole, same as any
other fail-high construct); section 15 gained a byte-exact worked example showing both the correct
order and the reversed-order failure side by side. **Also closed in this same pass, per the
`tech-lead`'s own brief:** the corpus-unjustified-but-intentional `max-height`/`max-width` registry
entries (section 6.1), an explicit non-merge decision for percentage families (b)/(c) (section 5), a
newly-found gap in the angle print form (section 8.2), a byte-count typo in section 9.2.1's own
worked example, and the three ambiguities reported by earlier `RMLX-2` slices (section 16).

**PT:** Este documento foi entregue originalmente (2026-08-06, `1173ae3`) com uma **afirmação
falsa**, achada e corrigida no mesmo dia pela própria revisão pré-voo do `tech-lead` antes dos dois
dumpers da `RMLX-2` serem despachados -- exatamente o modo de falha que a seção "Por que este
documento existe" acima avisa, pego antes de qualquer autor independente ler, não depois. **O que
estava errado:** a própria tabela da seção 6.2 dizia que o algoritmo `FallThrough` de
`border-top`/`-right`/`-bottom`/`-left` é *"independente de ordem entre os dois"* (width vs. color).
**Não é**, pra uma cadeia de 2-itens/2-tokens -- já provado por medição na `UIX-PROP-REGISTRY`
(`381624c`) antes desta errata: a ordem revertida (cor primeiro, ex. `#7A5A2E 1dp`) deixa o cursor de
item esgotado com o segundo token ainda não-reivindicado, e a própria guarda pós-laço do upstream
aborta o shorthand **inteiro**, não um resultado parcial -- rastreada iteração-por-iteração contra o
`examples/RmlUi/Source/Core/PropertySpecification.cpp:429-471` (lido direto, não parafraseado) e
pinada pelo próprio `test_border_top_fallthrough_order_is_load_bearing` do
`glintfx/tests/uix_style/shorthand_expansion_sanity.cpp`. O **código e o teste já estavam corretos**
e já citavam esta frase falsa como errada (`glintfx/src/uix/style/shorthand.hpp:38`,
`shorthand.cpp:30`) -- só este documento não tinha sido corrigido pra bater. **O que mudou:** a
célula da tabela da seção 6.2 agora declara a regra real, sensível-a-ordem; a seção 11 ganhou um caso
fail-high explícito pra valor de shorthand malformado (a declaração de ordem revertida é descartada
inteira, igual a qualquer outra construção fail-high); a seção 15 ganhou um exemplo trabalhado
byte-exato mostrando a ordem correta e a falha de ordem revertida lado a lado. **Também fechado nesta
mesma passada, per o próprio briefing do `tech-lead`:** as entradas de registro
`max-height`/`max-width` sem justificativa de corpus mas intencionais (seção 6.1), uma decisão
explícita de não-fusão pras famílias de porcentagem (b)/(c) (seção 5), uma lacuna nova achada na
forma de impressão de ângulo (seção 8.2), um erro de contagem de byte num exemplo trabalhado da
seção 9.2.1, e as três ambiguidades reportadas por fatias anteriores da `RMLX-2` (seção 16).

---

## 🟠 Errata (`UIX-RCSS-ERRATA-2`, 2026-08-06) / Errata (`UIX-RCSS-ERRATA-2`, 2026-08-06)

**EN:** An independent ambiguity audit (`UIX-RCSS-AMBIGUIDADE`, commit `4c5ce78`,
`docs/uix-rcss-ambiguidades.md`) enumerated all 68 normative rules this document states and found 7
**BLOQUEIA** (certain byte divergence against real upstream RmlUi), 3 **PROVÁVEL**, 1 **COSMÉTICO**,
and 4 non-determinable-without-the-lost-census claims. Before applying any of it, the `tech-lead`
**reverified every BLOQUEIA and PROVÁVEL finding directly against the upstream clone**
(`examples/RmlUi/`, cited `file:line`, never trusted from the audit's own paraphrase) -- the same
discipline this document's own header section demands of itself, applied one level up, to the audit
that checks it. All 7 BLOQUEIA and all 3 PROVÁVEL findings confirmed exactly as reported; nothing was
found to be a false positive; nothing required stopping short of application. **Two of the seven
(Findings A and B) corrected an exemplo trabalhado this same document had already published as
"byte-exact proof"** -- section 6.2's `UIX-RCSS-ERRATA-1` correction itself (Finding A) and section
9.1's original worked line (Finding B) -- meaning a Side A/Side B pair built against the
pre-errata-2 text would have silently agreed with each other on wrong bytes, exactly the failure mode
this document's own header warns about.

**What changed, section by section (all in this same pass):**
- **§3** (new): explicit "textual, UTF-8" format-overview sentence, and the file's trailing-newline
  terminator, previously only stated by the sibling `docs/uix-dom.md` and never inherited by name
  here (`Finding D`).
- **§4** (new): `STATE` block file order (`none` before `hover-all`) stated as a fixed, prose-declared
  sequence, not governed by the byte-wise sort rule used elsewhere (`Finding E`).
- **§6.2 / §11** (corrected): a rejected `FallThrough`/`RecursiveRepeat` shorthand does **not** revert
  every targeted longhand -- only the longhand that never matched a token falls back; one that matched
  before the loop's later failure keeps the source value, because upstream mutates the dictionary
  in-place with no rollback (`Finding A`; also corrects `UIX-RCSS-ERRATA-1`'s own still-wrong
  consequence text).
- **§7 / §7.1** (corrected + new): `box-shadow` layer colors and gradient-stop colors are
  **premultiplied**, not straight-alpha like every other color field -- a real behaviour difference
  for `alpha<255` (`Finding B`, **see the flagged decision note below**); citation line fixed
  `:69`→`:72` (`Finding K`); string-domain empty-value print convention and structural-identifier
  non-escaping stated explicitly (inheritance sweep, see below); string values do not escape this
  document's own `|`/`;`/`:` separators (`Finding H`).
- **§8** (new): `quantize()` is undefined for non-finite input; treat as any other fail-high
  computation error (`Finding J`).
- **§9.1** (corrected + new): worked example's layer-2 color fixed to the premultiplied byte value;
  a malformed single shadow layer aborts the whole property, not just that layer (`Finding I`).
- **§9.2** (corrected): `filter`/`backdrop-filter` split their source function list on **space**, not
  comma like `decorator`/`mask-image` -- a different parser class entirely (`Finding F`).
- **§9.3** (new): a malformed single `<single-animation-value>` aborts the whole `animation` property
  (`Finding I`).
- **§11** (corrected): a malformed decorator/filter entry drops the **entire property**, not just that
  entry -- the original text had this backwards (`Finding C`); an unrecognized selector in a
  comma-list drops **only that selector**, not the whole rule -- the original text had this backwards
  too, and it directly touches the corpus's own 16-tag UA-stylesheet rule (`Finding G`).
- **§15.2** (corrected): worked example's `body/1` line fixed -- `border-top-color` does not revert.

**Flagged for the líder's explicit attention, not silently decided:** Finding B (§7.1) names two
readings -- print the stored premultiplied bytes as-is (changing this document's own straight-alpha
contract for two fields), or un-premultiply before printing (keeping one uniform rule, at the cost of
an undefined `alpha=0` case neither engine needs to invent otherwise). This errata **applies the
first reading** and states why (§7.1), but the audit itself named this "a decision for the líder, not
something either dumper author should pick independently" -- the `tech-lead` closed it with a rule,
per this task's own instruction that an ambiguity must close, not merely be explained, but this one
specific closure is a design choice, not a fact-correction, and is called out here so it can be
overridden in one place if the líder disagrees.

**Inheritance sweep, requested by the orquestrador mid-task:** every form decision `docs/uix-dom.md`
§1-8 states was checked against whether this document inherited the equivalent. `SCOPE [herança do
documento-irmão]: 17 decisões de forma enumeradas no uix-dom.md, 8 já fechadas no uix-rcss.md, 5
lacunas fechadas por esta errata, 4 não-aplicáveis (com o motivo)`. The 5 gaps closed: UTF-8
format-overview statement, trailing newline (`Finding D`, already counted above), `STATE`-block file
order (`Finding E`, already counted above), structural-identifier (property-name) non-escaping, and
the string-domain empty-computed-value print convention (§7). The 4 non-applicable, with reason: `HEAD`
single-opaque-record (no equivalent concept -- this dump has no head/body split of that kind);
whitespace/text-node existence filter + entity decoding (no text nodes or XML entities in a
computed-value dump); tag-name lowercase-folding (property names in a `PROP` line are always
dumper-selected from the closed §6.1 registry, never echoed from arbitrary source casing the way a
tag name is, so casing is structurally moot here); the bare line-terminator byte (`\n` vs `\r\n`
between records, distinct from the trailing-newline-at-EOF question) -- `docs/uix-dom.md` itself never
states this either, so there is nothing to inherit; flagged as a shared gap in both siblings, out of
this specific sweep's scope, not fixed here.

**Addendum, mid-task, from a regenerated corpus census (`tools/rcss_census.py`,
`docs/uix-rcss-censo.md`, replacing the scratch `/var/tmp/censo-rcss-qa1/censo.md` this document's
older citations still point to -- re-sourcing every one of those older citations across the whole
document is a separate, larger pass, out of this fatia's scope, flagged here rather than attempted):
three more corrections, one of them the single most consequential fix in this entire errata pass.**
(1) **`vertical-gradient` was entirely missing from §9.2's function table** -- 107 corpus occurrences
across 16 files, the single most-used decorator function in the corpus, more than `polygon`. Combined
with `Finding C`'s corrected consequence (a malformed/unknown decorator entry drops the whole
property), the pre-fix table would have silently dropped `decorator`/`mask-image` to `none` on every
one of those 107 declarations, and both oracle sides would have agreed on the same wrong empty
output -- a green oracle for the wrong reason. Closed by full enumeration of the corpus's own
decorator-function vocabulary, not a spot-check; see §9.2's own `SCOPE [funções de decorator]` line.
(2) **§9.1's `box-shadow` spread-omission ratio was inverted** -- published as "124 of 135 omit
`spread`", independently re-measured (script + the `tech-lead`'s own direct declaration-by-declaration
walk) as roughly the opposite: 123 of 124 single-layer declarations *specify* `spread` explicitly,
only 1 omits it. The rule itself (`spread` defaults to `0px` when omitted) is unaffected; only the
*how common* corpus claim was backwards. (3) **§8's "largest length observed" example, `-228dp`, was
not actually the largest** -- `999dp` (`border-radius`, the corpus's own "fully rounded" idiom) and
`-410dp` (`margin-left`) are both larger in magnitude; cosmetic, the broader 0-3000-range conclusion
is unaffected (`999dp` is still 3 digits before the point). All three verified directly by the
`tech-lead`, not merely trusted from the report that raised them.

**PT:** Uma auditoria independente de ambiguidade (`UIX-RCSS-AMBIGUIDADE`, commit `4c5ce78`,
`docs/uix-rcss-ambiguidades.md`) enumerou as 68 regras normativas deste documento e achou 7
**BLOQUEIA** (divergência de byte certa contra o RmlUi upstream real), 3 **PROVÁVEL**, 1
**COSMÉTICO**, e 4 reivindicações não-determináveis sem o censo perdido. Antes de aplicar qualquer
coisa, o `tech-lead` **reverificou todo achado BLOQUEIA e PROVÁVEL direto contra o clone upstream**
(`examples/RmlUi/`, citado `arquivo:linha`, nunca confiado na própria paráfrase da auditoria) -- a
mesma disciplina que a própria seção de cabeçalho deste documento exige de si mesmo, aplicada um nível
acima, à auditoria que o verifica. Todos os 7 BLOQUEIA e os 3 PROVÁVEL confirmados exatamente como
reportados; nenhum se revelou falso positivo; nenhum exigiu parar sem aplicar. **Dois dos sete
(Achados A e B) corrigiram um exemplo trabalhado que este mesmo documento já publicava como "prova
byte-exata"** -- a própria correção da seção 6.2 da `UIX-RCSS-ERRATA-1` (Achado A) e a linha
trabalhada original da seção 9.1 (Achado B) -- significando que um par lado A/lado B construído
contra o texto pré-errata-2 teria concordado em silêncio um com o outro sobre bytes errados,
exatamente o modo de falha que a própria seção de cabeçalho deste documento avisa.

**O que mudou, seção por seção (tudo nesta mesma passada):**
- **§3** (novo): frase explícita de visão geral de formato "textual, UTF-8", e o terminador de
  newline final do arquivo, antes só declarado pelo irmão `docs/uix-dom.md` e nunca herdado por nome
  aqui (`Achado D`).
- **§4** (novo): a ordem de arquivo dos blocos `STATE` (`none` antes de `hover-all`) declarada como
  sequência fixa, prosa-declarada, não governada pela regra de ordenação byte-wise usada em outro
  lugar (`Achado E`).
- **§6.2 / §11** (corrigido): um shorthand `FallThrough`/`RecursiveRepeat` rejeitado **não** reverte
  todo longhand alvejado -- só o longhand que nunca casou nenhum token cai pro fallback; um que casou
  antes da falha posterior do laço mantém o valor da fonte, porque o upstream muta o dicionário in
  place sem rollback (`Achado A`; também corrige o próprio texto de consequência da
  `UIX-RCSS-ERRATA-1`, que ainda estava errado).
- **§7 / §7.1** (corrigido + novo): cores de camada de `box-shadow` e de stop de gradiente são
  **pré-multiplicadas**, não straight-alpha como todo outro campo de cor -- uma diferença de
  comportamento real pra `alpha<255` (`Achado B`, **ver a nota de decisão sinalizada abaixo**);
  citação de linha corrigida `:69`→`:72` (`Achado K`); convenção de impressão de valor-computado-vazio
  do domínio string e não-escape de identificador estrutural declarados explicitamente (varredura de
  herança, ver abaixo); valores string não escapam os separadores próprios `|`/`;`/`:` deste documento
  (`Achado H`).
- **§8** (novo): `quantize()` é indefinido pra entrada não-finita; tratar como qualquer outro erro de
  computação fail-high (`Achado J`).
- **§9.1** (corrigido + novo): cor da camada 2 do exemplo trabalhado corrigida pro valor de byte
  pré-multiplicado; uma camada de shadow malformada única derruba a propriedade inteira, não só
  aquela camada (`Achado I`).
- **§9.2** (corrigido): `filter`/`backdrop-filter` dividem a própria lista de função da fonte por
  **espaço**, não vírgula como `decorator`/`mask-image` -- uma classe de parser inteiramente separada
  (`Achado F`).
- **§9.3** (novo): um `<single-animation-value>` malformado único derruba a propriedade `animation`
  inteira (`Achado I`).
- **§11** (corrigido): uma entrada de decorator/filter malformada derruba a **propriedade inteira**,
  não só aquela entrada -- o texto original tinha isso invertido (`Achado C`); um seletor
  não-reconhecido numa lista-vírgula derruba **só aquele seletor**, não a regra inteira -- o texto
  original tinha isso invertido também, e toca direto a própria regra de UA-stylesheet de 16-tags do
  corpus (`Achado G`).
- **§15.2** (corrigido): linha `body/1` do exemplo trabalhado corrigida -- `border-top-color` não
  reverte.

**Sinalizado pra atenção explícita do líder, não decidido em silêncio:** o Achado B (§7.1) nomeia
duas leituras -- imprimir os bytes pré-multiplicados armazenados como estão (mudando o próprio
contrato straight-alpha deste documento pra dois campos), ou despré-multiplicar antes de imprimir
(mantendo uma regra uniforme única, ao custo de um caso `alpha=0` indefinido que nenhum motor
precisaria inventar de outro jeito). Esta errata **aplica a primeira leitura** e declara por quê
(§7.1), mas a própria auditoria nomeou isto "uma decisão do líder, não algo que qualquer autor de
dumper deveria escolher sozinho" -- o `tech-lead` fechou com uma regra, pela própria instrução desta
tarefa de que uma ambiguidade precisa fechar, não só ser explicada, mas este fechamento específico é
uma escolha de design, não uma correção de fato, e está sinalizado aqui pra poder ser revertido num
lugar só se o líder discordar.

**Varredura de herança, pedida pelo orquestrador no meio da tarefa:** toda decisão de forma que a
`docs/uix-dom.md` §1-8 declara foi checada quanto a se este documento herdou o equivalente. `SCOPE
[herança do documento-irmão]: 17 decisões de forma enumeradas no uix-dom.md, 8 já fechadas no
uix-rcss.md, 5 lacunas fechadas por esta errata, 4 não-aplicáveis (com o motivo)`. As 5 lacunas
fechadas: declaração de visão geral de formato UTF-8, newline final (`Achado D`, já contado acima),
ordem de arquivo dos blocos `STATE` (`Achado E`, já contado acima), não-escape de identificador
estrutural (nome de propriedade), e a convenção de impressão de valor-computado-vazio do domínio
string (§7). As 4 não-aplicáveis, com motivo: registro único opaco `HEAD` (sem conceito equivalente --
este dump não tem divisão head/body desse tipo); filtro de existência de nó de texto + decodificação
de entidade (sem nós de texto ou entidades XML num dump de valor computado); folding de caixa de nome
de tag pra minúscula (nomes de propriedade numa linha `PROP` são sempre escolhidos pelo dumper a
partir do registro fechado da seção 6.1, nunca ecoados de caixa arbitrária da fonte do jeito que um
nome de tag é, então caixa é estruturalmente irrelevante aqui); o byte de terminador de linha em si
(`\n` vs `\r\n` entre registros, distinto da questão de newline-final-no-EOF) -- o próprio
`docs/uix-dom.md` nunca declara isso também, então não há nada pra herdar; sinalizado como lacuna
compartilhada nos dois irmãos, fora do escopo desta varredura específica, não consertado aqui.

**Adendo, no meio da tarefa, de um censo de corpus regenerado (`tools/rcss_census.py`,
`docs/uix-rcss-censo.md`, substituindo o scratch `/var/tmp/censo-rcss-qa1/censo.md` pro qual as
citações mais antigas deste documento ainda apontam -- re-referenciar cada uma dessas citações mais
antigas no documento inteiro é uma passada separada e maior, fora do escopo desta fatia, sinalizada
aqui em vez de tentada): três correções a mais, uma delas o conserto mais consequente de toda esta
passada de errata.** (1) **`vertical-gradient` estava inteiramente ausente da tabela de funções da
seção 9.2** -- 107 ocorrências de corpus em 16 arquivos, a função de decorator mais usada do corpus,
mais que `polygon`. Combinado com a consequência corrigida do Achado C (uma entrada de decorator
malformada/desconhecida derruba a propriedade inteira), a tabela pré-conserto teria derrubado
`decorator`/`mask-image` pra `none` em silêncio em cada uma dessas 107 declarações, e os dois lados
do oráculo teriam concordado na mesma saída vazia errada -- um oráculo verde pelo motivo errado.
Fechado por enumeração completa do próprio vocabulário de função-de-decorator do corpus, não um
spot-check; ver a própria linha `SCOPE [funções de decorator]` da seção 9.2. (2) **A taxa de omissão
de `spread` do `box-shadow` da seção 9.1 estava invertida** -- publicada como "124 de 135 omitem
`spread`", remedida de forma independente (script + a própria varredura declaração-por-declaração
direta do `tech-lead`) como aproximadamente o oposto: 123 de 124 declarações single-layer
*especificam* `spread` explicitamente, só 1 omite. A própria regra (`spread` tem default `0px`
quando omitido) não é afetada; só a reivindicação de corpus de *quão comum* estava invertida. (3) **O
exemplo de "maior comprimento observado" da seção 8, `-228dp`, não era de fato o maior** -- `999dp`
(`border-radius`, o próprio idioma "totalmente arredondado" do corpus) e `-410dp` (`margin-left`) são
os dois maiores em magnitude; cosmético, a conclusão mais ampla da faixa 0-3000 não é afetada
(`999dp` ainda é 3 dígitos antes do ponto). Os três verificados diretamente pelo `tech-lead`, não só
confiados do relatório que os levantou.

---

## 🟢 Errata (`UIX-RCSS-ERRATA-3`, 2026-08-06) / Errata (`UIX-RCSS-ERRATA-3`, 2026-08-06)

**EN:** Found by the implementer of oracle side A (`UIX-RCSS-DUMP-A`) while building the fixture
§15.4's own table claims to anchor; reverified independently by the `tech-lead`, arithmetic below,
before applying this correction -- and before treating the finding as settled, every OTHER worked
example in this document (§9.1, §9.2.1, §15.1-§15.4, six total, the complete labelled set) was
re-enumerated against the same question, per this task's own house rule ("enumerate the small space,
don't search inside it," §4's own governing principle applied to this document about itself): *does
the entry's chosen input actually reach the condition it claims to exercise?*

**What was wrong:** §15.4's rows 1 and 3 (`1.234450`/`-1.234450`) claim an "exact tie" -- but the
*decimal* literal `1.234450` is not the *binary32* value `quantize()` actually receives. Widened once
to `double` (`quantize()`'s own first step), `1.234450f` is `1.2344499826431274`, so `scaled = x *
10000` is `12344.499826431274` -- short of the half-integer `12344.5` by ~1.7e-4, not equal to it;
`1.234449f` widens to `1.234449028968811` (`scaled = 12344.49028968811`), also not a tie (rows 2 and
4, "one step below," were never wrong on their own terms -- a value below a boundary is still below a
boundary even if the boundary itself was misdrawn, exactly why this bug hid through two prior errata
passes: the wrong rows *looked* internally consistent with the correct ones). The cause is structural,
not an unlucky pick: `N.5 / 10000` is representable in `float32` only when `x`'s own reduced fraction
has a power-of-two denominator, and a literal built from 4-6 decimal digits (`10000 = 2^4 * 5^4`)
essentially never reduces that way -- a value *chosen to look like a tie in decimal* is, almost
always, not one in binary.

**Why this is worse than merely imprecise:** the whole reason a byte-exact worked example exists in
this document is that prose admits two readings and a golden does not (§15's own header). A golden
that never reaches the condition it claims to pin fails silently in the one place independence is
supposed to catch it: both oracle sides run the identical `quantize()` algorithm from §8 against an
input that isn't the boundary, both print the same non-tie answer, both agree -- and the tie-breaking
rule this table exists to prove is never exercised by either side. This already happened for real,
isolated by construction: `UIX-RCSS-DUMP-A`'s author correctly detected the divergence while wiring
the fixture, reported it faithfully in code rather than silently matching the spec's wrong printed
value (`glintfx/tests/uix_style/value_compute_sanity.cpp`,
`test_worked_example_15_4_row_1_and_3_do_not_reproduce_float32_representability`), and added a
*separate*, hand-picked genuine tie (`1.21875f`) to actually prove the property elsewhere in the same
file -- the correct response to a wrong gabarito, per this project's own house rule. But
`UIX-RCSS-DUMP-B`'s author, who by this architecture's own non-cross-contamination rule may **only**
read this spec, never `DUMP-A`'s source, would have hit the identical dead end with no such escape
hatch visible to them -- the divergence `DUMP-A` quietly worked around stayed live in the one document
a second, independent implementer is required to trust.

**The fix:** rows 1/3 replaced with `1.21875`/`-1.21875` -- `1.21875 = 39/32`, reduced denominator
`2^5`, a power of two, exactly representable in `float32` (`(double)(float)1.21875 == 1.21875`, no
widening error at all), `scaled = 1.21875 * 10000 = 12187.5` exactly, a genuine tie. Rows 2/4 replaced
with `1.21874`/`-1.21874` (kept in the same decimal neighborhood as the new tie, not left orphaned
pointing at the old one) -- widens to `1.2187399864196777`, `scaled = 12187.399864196777`, below the
half-integer, correctly rounds toward zero. The chosen tie value matches `UIX-RCSS-DUMP-A`'s own
already-shipped `test_quantize_genuine_float32_exact_tie` literal exactly, closing the spec-vs-code
divergence rather than introducing a third, unrelated pair.

**Consequence for `UIX-RCSS-DUMP-A`, flagged not fixed here (out of this fatia's scope, this document
only):** the existing test suite's rows-2/4 assertion (`quantize(1.234449f) == "1.2344"`) is still a
true statement about `quantize()`, but is no longer anchored to a §15.4 table row once this correction
lands -- a future pass of `UIX-RCSS-DUMP-A` (or its review) should add `quantize(1.21874f) ==
"1.2187"` to stay byte-locked to the corrected table, the same discipline that test file's own header
comment already states for itself.

**Enumeration of the other five labelled worked examples in this document, same question applied to
each (`SCOPE [exemplos trabalhados]: 6 enumerados, 3 exercitam de fato a condição declarada, 1 inerte
(corrigido nesta errata, §15.4), 2 inertes (declarados, não corrigidos, motivo abaixo)`):**
- **§15.1** (`:hover`, two states) -- exercises the claim: the state matrix is closed at 2 members
  (`none`/`hover-all`, §4's own scoped table) and the example enumerates both in full; only `color`
  (the one property `.btn:hover` touches) differs between them, proving override isolation, not
  leaking into `width`/`opacity`. Not inert.
- **§15.2** (`border-top` shorthand order) -- exercises the claim: both token orders are shown side by
  side, `#a` (canonical) and `#b` (reversed), and the asymmetric fallback (`-color` matched and kept,
  `-width` never matched and reverted) is exactly what a naive "whole declaration reverts" reading
  would get wrong. Already reverified byte-by-byte against upstream by `UIX-RCSS-ERRATA-2`. Not inert.
- **§15.3** (three `%` families) -- exercises the claim that families (b) and (c) do not merge: the
  worked `radial-gradient(...)` puts family (c) (`35%`/`30%`, the `circle at` center) and family (b)
  (`0%`/`55%`/`100%`, the stop positions) inside the **same** function call, different argument slots,
  closing §5's own reported merge risk with a byte anchor rather than only the prose decision. Not
  inert.
- **§9.1** (`box-shadow`) -- **partially inert, flagged not fixed here.** The paragraph immediately
  preceding the worked example states two non-trivial facts the example does not exercise: (1)
  `inset`/color are assigned by length-parse-success order, not list position, demonstrated in prose
  only by an abstract pair (`inset #ff0000 0 0 10px` vs. reversed) -- the real corpus fixture used for
  the worked example places color first and `inset` last in **both** layers, the canonical order a
  naive positional parser would also get right; (2) `spread` defaults to `0.0000px` when the source
  omits it (the subject of this same errata pass's own corpus-ratio correction, `123 of 124 specify it
  explicitly, only 1 omits it`) -- but both layers of the chosen fixture specify all 4 length fields
  explicitly, so the default path is never printed. What the example does correctly exercise: the
  premultiplied-alpha correction (`UIX-RCSS-ERRATA-2`, `Finding B` -- this is where that real bug was
  actually found) and the `|`-joined, source-order multi-layer grammar. Not fixed in this pass:
  constructing a new byte-exact anchor for the two untested facts needs a corpus fixture verified
  against real RmlUi parsing behaviour (the one candidate the errata-2 corpus census already names,
  `gusworld_battle_cockpit.rml`'s `#22D3EE 0dp 0dp 8dp`, for the spread-omission case) -- that is
  `UIX-RCSS-DUMP-A`/census territory, not this fatia's.
- **§9.2.1** (gradient auto-spacing algorithm) -- **partially inert, flagged not fixed here.** The
  algorithm has three non-trivial rules beyond the trivial "explicit position kept": rule 2 (first
  stop, unpositioned, → `0%`), rule 3 (last stop, unpositioned, → `100%`), rule 4 (a run of K≥1
  consecutive unpositioned stops interpolated evenly between their neighbors). The worked example
  (reused verbatim by §15.3) only has one unpositioned stop, the first one -- rule 2 fires, rules 3
  and 4 never do. Rule 4, the only one with an actual formula (`P_before + i * (P_after - P_before) /
  (K + 1)`), is the likeliest place for an off-by-one, and it is untested by this document's own
  "byte-exact, not just prose" anchor. Same scoping reason as §9.1: a new fixture needs corpus
  verification, out of this fatia.

**Rule for reuse, stated once so it outlives this specific table:** when a literal exists to pin a
floating-point rounding boundary, verify the value **is** the boundary in the type's own binary
representation -- widen the literal to `double` and compare `scaled` to the nearest half-integer
directly -- never that it *looks like* the boundary in decimal. Decimal and binary disagree exactly at
the edges a boundary test exists to exercise; that disagreement is the whole reason `float32` needs a
boundary test in the first place, and it is precisely what "eyeballing the decimal digits" cannot see.

**PT:** Achado pelo implementador do lado A do oráculo (`UIX-RCSS-DUMP-A`) ao montar a fixture que a
própria tabela da seção 15.4 diz ancorar; reverificado de forma independente pelo `tech-lead`,
aritmética abaixo, antes de aplicar esta correção -- e antes de tratar o achado como resolvido, todo
OUTRO exemplo trabalhado deste documento (§9.1, §9.2.1, §15.1-§15.4, seis no total, o conjunto
rotulado completo) foi reenumerado contra a mesma pergunta, pela própria regra da casa desta tarefa
("enumere o espaço pequeno, não busque dentro dele", o próprio princípio-guia da seção 4 aplicado a
este documento sobre si mesmo): *a entrada escolhida por este exemplo realmente alcança a condição que
ele afirma exercitar?*

**O que estava errado:** as linhas 1 e 3 da seção 15.4 (`1.234450`/`-1.234450`) alegam "empate exato"
-- mas o literal *decimal* `1.234450` não é o valor *binary32* que o `quantize()` de fato recebe.
Ampliado uma vez pra `double` (o próprio primeiro passo do `quantize()`), `1.234450f` é
`1.2344499826431274`, então `scaled = x * 10000` é `12344.499826431274` -- abaixo do meio-inteiro
`12344.5` por ~1,7e-4, não igual a ele; `1.234449f` amplia pra `1.234449028968811` (`scaled =
12344.49028968811`), também não é empate (as linhas 2 e 4, "um passo abaixo", nunca estavam erradas
nos próprios termos -- um valor abaixo de uma fronteira continua abaixo mesmo se a própria fronteira
foi mal-desenhada, e é exatamente por isso que este bug ficou escondido por duas passadas de errata
anteriores: as linhas erradas *pareciam* internamente consistentes com as corretas). A causa é
estrutural, não azar de escolha: `N.5 / 10000` só é representável em `float32` quando a fração
reduzida de `x` já tem denominador potência de dois, e um literal construído com 4-6 casas decimais
(`10000 = 2^4 * 5^4`) quase nunca reduz desse jeito -- um valor *escolhido pra parecer empate em
decimal* quase sempre não é um em binário.

**Por que isto é pior que mera imprecisão:** a razão inteira de um exemplo trabalhado byte-exato
existir neste documento é que prosa admite duas leituras e um golden não (cabeçalho da própria seção
15). Um golden que nunca alcança a condição que afirma fixar falha em silêncio bem no ponto em que a
independência deveria pegá-lo: os dois lados do oráculo rodam o mesmo algoritmo `quantize()` da seção
8 contra uma entrada que não é a fronteira, os dois imprimem a mesma resposta não-empate, os dois
concordam -- e a regra de desempate que esta tabela existe para provar nunca é exercitada por nenhum
dos dois lados. Isto já aconteceu de fato, isolado por construção: o autor da `UIX-RCSS-DUMP-A`
detectou corretamente a divergência ao montar a fixture, reportou com fidelidade em código em vez de
casar em silêncio com o valor errado impresso da spec (`glintfx/tests/uix_style/value_compute_sanity.cpp`,
`test_worked_example_15_4_row_1_and_3_do_not_reproduce_float32_representability`), e acrescentou um
empate genuíno *separado*, escolhido à mão (`1.21875f`), pra de fato provar a propriedade em outro
lugar do mesmo arquivo -- a resposta correta pra um gabarito errado, pela própria regra da casa deste
projeto. Mas o autor da `UIX-RCSS-DUMP-B`, que pela própria regra de não-contaminação-cruzada desta
arquitetura só PODE ler esta spec, nunca o fonte da `DUMP-A`, teria batido no mesmo beco sem saída
idêntico, sem essa saída de emergência visível pra ele -- a divergência que a `DUMP-A` contornou em
silêncio continuava viva no único documento que um segundo implementador independente é obrigado a
confiar.

**O conserto:** linhas 1/3 substituídas por `1.21875`/`-1.21875` -- `1.21875 = 39/32`, denominador
reduzido `2^5`, potência de dois, exatamente representável em `float32` (`(double)(float)1.21875 ==
1.21875`, zero erro de ampliação), `scaled = 1.21875 * 10000 = 12187.5` exatamente, um empate genuíno.
Linhas 2/4 substituídas por `1.21874`/`-1.21874` (mantidas na mesma vizinhança decimal do novo empate,
não deixadas órfãs apontando pro empate antigo) -- amplia pra `1.2187399864196777`, `scaled =
12187.399864196777`, abaixo do meio-inteiro, arredonda corretamente pra zero. O valor de empate
escolhido bate exatamente com o literal já entregue do próprio `test_quantize_genuine_float32_exact_tie`
da `UIX-RCSS-DUMP-A`, fechando a divergência spec-vs-código em vez de introduzir um terceiro par sem
relação.

**Consequência pra `UIX-RCSS-DUMP-A`, sinalizada e não consertada aqui (fora do escopo desta fatia, só
este documento):** a asserção existente das linhas 2/4 da suíte de teste (`quantize(1.234449f) ==
"1.2344"`) continua sendo uma afirmação verdadeira sobre `quantize()`, mas deixa de estar ancorada
numa linha da tabela da 15.4 assim que esta correção entrar -- uma passada futura da `UIX-RCSS-DUMP-A`
(ou sua revisão) deveria acrescentar `quantize(1.21874f) == "1.2187"` pra ficar byte-travada na tabela
corrigida, a mesma disciplina que o próprio comentário de cabeçalho daquele arquivo de teste já declara
pra si mesmo.

**Enumeração dos outros cinco exemplos trabalhados rotulados deste documento, mesma pergunta aplicada
a cada um (`SCOPE [exemplos trabalhados]: 6 enumerados, 3 exercitam de fato a condição declarada, 1
inerte (corrigido nesta errata, §15.4), 2 inertes (declarados, não corrigidos, motivo abaixo)`):**
- **§15.1** (`:hover`, dois estados) -- exercita a afirmação: a matriz de estado é fechada em 2
  membros (`none`/`hover-all`, a própria tabela de escopo da seção 4) e o exemplo enumera os dois por
  completo; só `color` (a única propriedade que `.btn:hover` toca) difere entre eles, provando
  isolamento de override, sem vazar pra `width`/`opacity`. Não inerte.
- **§15.2** (ordem do shorthand `border-top`) -- exercita a afirmação: as duas ordens de token são
  mostradas lado a lado, `#a` (canônica) e `#b` (revertida), e o fallback assimétrico (`-color` casou
  e ficou, `-width` nunca casou e reverteu) é exatamente o que uma leitura ingênua "a declaração
  inteira reverte" erraria. Já reverificado byte a byte contra o upstream pela `UIX-RCSS-ERRATA-2`.
  Não inerte.
- **§15.3** (três famílias de `%`) -- exercita a afirmação de que as famílias (b) e (c) não se fundem:
  o `radial-gradient(...)` trabalhado coloca a família (c) (`35%`/`30%`, o centro do `circle at`) e a
  família (b) (`0%`/`55%`/`100%`, as posições de stop) dentro da MESMA chamada de função, em slots de
  argumento diferentes, fechando o risco de fusão da própria seção 5 com uma âncora de byte, não só a
  decisão em prosa. Não inerte.
- **§9.1** (`box-shadow`) -- **parcialmente inerte, sinalizado e não consertado aqui.** O parágrafo
  logo antes do exemplo trabalhado declara dois fatos não-triviais que o exemplo não exercita: (1)
  `inset`/cor são atribuídos por ordem de sucesso-de-parse-de-comprimento, não posição na lista,
  demonstrado em prosa só por um par abstrato (`inset #ff0000 0 0 10px` vs. revertido) -- a fixture
  real de corpus usada no exemplo trabalhado coloca a cor primeiro e o `inset` por último nas DUAS
  camadas, a ordem canônica que um parser posicional ingênuo também acertaria; (2) `spread` tem
  default `0.0000px` quando a fonte omite -- assunto da própria correção de razão de corpus desta
  mesma passada de errata (`123 de 124 especificam explicitamente, só 1 omite`) -- mas as duas camadas
  da fixture escolhida especificam os 4 campos de comprimento explicitamente, então o caminho de
  default nunca é impresso. O que o exemplo exercita corretamente: a correção de alfa pré-multiplicado
  (`UIX-RCSS-ERRATA-2`, Achado B -- é onde aquele bug real foi de fato achado) e a gramática de
  múltiplas camadas unidas por `|` em ordem de fonte. Não consertado nesta passada: construir uma nova
  âncora byte-exata pros dois fatos não-testados precisa de uma fixture de corpus verificada contra o
  comportamento real de parse do RmlUi (o candidato que o próprio censo da errata-2 já nomeia,
  `#22D3EE 0dp 0dp 8dp` do `gusworld_battle_cockpit.rml`, pro caso de omissão de spread) -- é
  território da `UIX-RCSS-DUMP-A`/censo, não desta fatia.
- **§9.2.1** (algoritmo de auto-espaçamento de gradiente) -- **parcialmente inerte, sinalizado e não
  consertado aqui.** O algoritmo tem três regras não-triviais além do trivial "posição explícita
  fica": regra 2 (primeiro stop, sem posição, → `0%`), regra 3 (último stop, sem posição, → `100%`),
  regra 4 (uma sequência de K≥1 stops consecutivos sem posição interpolados uniformemente entre os
  vizinhos). O exemplo trabalhado (reusado literalmente pela §15.3) só tem um stop sem posição, o
  primeiro -- a regra 2 dispara, as regras 3 e 4 nunca. A regra 4, a única com fórmula de fato
  (`P_before + i * (P_after - P_before) / (K + 1)`), é o lugar mais provável de um off-by-one, e não é
  testada pela própria âncora "byte-exata, não só prosa" deste documento. Mesmo motivo de escopo do
  §9.1: uma fixture nova precisa de verificação de corpus, fora desta fatia.

**Regra pra reuso, declarada uma vez pra sobreviver a esta tabela específica:** ao escolher um literal
pra fixar uma fronteira de arredondamento de ponto flutuante, verifique que o valor É a fronteira na
própria representação binária do tipo -- amplie o literal pra `double` e compare `scaled` com o
meio-inteiro mais próximo diretamente -- nunca que ele PARECE a fronteira em decimal. Decimal e
binário discordam exatamente nas bordas que um teste de fronteira existe para exercitar; essa
discordância é a razão inteira de `float32` precisar de um teste de fronteira, e é exatamente o que
"olhar os dígitos decimais" não consegue enxergar.

---

## 🔴 Errata (`UIX-RCSS-ERRATA-4`, 2026-08-06) / Errata (`UIX-RCSS-ERRATA-4`, 2026-08-06)

**EN:** `UIX-RCSS-ERRATA-2`'s own §7.1 decision -- print `box-shadow` layer colors and gradient-stop
colors as the **stored premultiplied bytes, as-is** -- is **reverted** here, decided today by the
**líder himself**, with the correction and the new evidence below on the table, not closed
unilaterally by the `tech-lead` the way `ERRATA-2` closed it. **Both fields go back to straight
alpha**, matching every other color-typed field this document names (§7.1's own general rule),
obtained by applying `ToNonPremultiplied()` to the value the parser actually stored -- not the
premultiplied bytes echoed as-is.

**`ERRATA-2`'s argument (b) is false, and this is measured, not asserted:** it justified printing
the premultiplied bytes partly because "un-premultiplying is undefined at `alpha=0`". It is not.
`examples/RmlUi/Include/RmlUi/Core/Colour.h:105-107`, `ToNonPremultiplied()`'s own body:
`ColourType(alpha > 0 ? (red * 255) / alpha : 0)` (same guard on the green/blue channels) -- an
explicit `alpha > 0` guard, total for every input, `0` for the fully-transparent case. There is no
division by zero to avoid; the alternative `ERRATA-2` rejected as "undefined" is defined by the same
upstream this whole document takes as its oracle.

**New evidence `ERRATA-2` did not have: upstream's own answer to "what text represents this color"
is straight alpha, for exactly these two fields.** When real RmlUi itself converts a `ColorStopList`
or a `BoxShadowList` to `String` -- the same question this document's `PROP` line answers, just for
a different consumer -- it un-premultiplies first:
`examples/RmlUi/Source/Core/TypeConverter.cpp:223` (`ColorStopList` → `String`):
`dest += ToString(stop.color.ToNonPremultiplied());`; `TypeConverter.cpp:256` (`BoxShadowList` →
`String`): `dest += ToString(shadow.color.ToNonPremultiplied()) + temp;`. `ERRATA-2`'s own principle
-- "report whatever the pipeline actually produced" (§7.1, reasoning (1)) -- is not overturned, it is
**re-applied to a fact `ERRATA-2` did not have**: the pipeline's own textual serialization step
un-premultiplies. Printing the premultiplied bytes as-is was not "staying mechanical", it was
skipping a step upstream itself takes.

**🔴 The point most likely to cause a byte divergence, and the one nobody had measured before now:
the round trip is lossy, in both directions, and the printed value equals neither the value authored
in the sheet nor the value stored after parsing.** `(channel * 255) / alpha` is integer division,
truncating, same as the premultiply step's own `(channel * alpha) / 255` -- two truncations in
series do not invert each other. Measured directly (`(channel*alpha)/255` then `(result*255)/alpha`,
both truncating):

| written in RCSS | stored (premultiplied) | printed (straight alpha) |
| :--- | :--- | :--- |
| `#22d3ee80` | `#11697780` | **`#21d1ed80`** |
| `#c9a24b40` | `#32281240` | **`#c79f4740`** |
| `#22d3ee00` | `#00000000` | **`#00000000`** |

Three distinct values for `alpha<255`, not two. An implementer expecting the printed byte to equal
the authored byte -- a reasonable expectation for every *other* color field this document defines --
will diverge silently the first time a fixture uses `alpha<255` on `box-shadow`/gradient-stop. **The
normative formula, stated explicitly so a second implementer does not have to reconstruct it: `printed
= alpha > 0 ? (stored_channel * 255) / alpha : 0`, integer truncation, the exact guard and the exact
arithmetic `ToNonPremultiplied()` uses (`Colour.h:105-107`) -- never `round()`, never a floating-point
division.**

**§9.1's worked example is corrected to match (below, in the English section) -- layer 2's printed
color changes a *third* time across this document's history: `#22d3ee26` (`ERRATA-1`-era text,
naively echoed the source literal) → `#051f2326` (`ERRATA-2`, premultiplied bytes as-is) →
`#21d0ea26` (`ERRATA-4`, straight alpha via `ToNonPremultiplied()` -- and notably **not** the same
byte as the original `#22d3ee26`, exactly the lossy round-trip measured above).** §9.1's own
`ERRATA-3`-flagged inertness gap (the fixture's `inset`/color order is canonical, not
position-independent; both layers specify `spread` explicitly, so its default path is never printed)
is **untouched by this errata and remains open** -- closing it needs a corpus-verified fixture
(`ERRATA-3` already names the candidate, `gusworld_battle_cockpit.rml`'s `#22D3EE 0dp 0dp 8dp`), which
is `UIX-RCSS-DUMP-A`/census territory, not a claim this errata can verify without inventing a fixture
of its own. Redeclared here rather than left to be rediscovered.

**Reversibility: not zero-cost -- `UIX-RCSS-DUMP-A` already shipped against `ERRATA-2`'s reading, and
this document's own header claim ("no `RMLX-2` dumper exists yet") is stale as of this errata.**
`UIX-RCSS-DUMP-A`'s own `TODO.md` entry states it explicitly: color is printed "as está, sem
despremultiplizar" -- exactly the decision this errata reverts. Reverting the *document* is two-way
and free; the *code* now diverges from the spec it is supposed to implement, for every `alpha<255`
`box-shadow`/gradient-stop fixture, until a follow-up patch applies `ToNonPremultiplied()` in
`glintfx/src/rml/rcss_dump.{hpp,cpp}` (out of scope here -- forbidden path for this fatia). `Side B`
(`UIX-RCSS-DUMP-B`, gated behind the still-in-progress `UIX-VALUE-COMPUTE`) has not shipped yet, so it
costs nothing and needs no correction. Flagged in `TODO.md` as a follow-up so it is not lost.

**PT:** A própria decisão da seção 7.1 da `UIX-RCSS-ERRATA-2` -- imprimir cores de camada de
`box-shadow` e cores de stop de gradiente como os **bytes pré-multiplicados armazenados, como
estão** -- é **revertida** aqui, decidida hoje pelo **próprio líder**, com a correção e a evidência
nova abaixo na mesa, não fechada unilateralmente pelo `tech-lead` do jeito que a `ERRATA-2` fechou.
**Os dois campos voltam pra alfa direto (straight alpha)**, batendo com todo outro campo tipo-cor que
este documento nomeia (a própria regra geral da seção 7.1), obtido aplicando `ToNonPremultiplied()`
ao valor que o parser de fato armazenou -- não os bytes pré-multiplicados ecoados como estão.

**O argumento (b) da `ERRATA-2` é falso, e isto é medido, não afirmado:** ele justificava imprimir os
bytes pré-multiplicados em parte porque "despré-multiplicar é indefinido em `alpha=0`". Não é.
`examples/RmlUi/Include/RmlUi/Core/Colour.h:105-107`, o próprio corpo de `ToNonPremultiplied()`:
`ColourType(alpha > 0 ? (red * 255) / alpha : 0)` (mesma guarda nos canais verde/azul) -- uma guarda
explícita `alpha > 0`, total pra toda entrada, `0` pro caso totalmente transparente. Não há divisão
por zero a evitar; a alternativa que a `ERRATA-2` rejeitou como "indefinida" é definida pelo mesmo
upstream que este documento inteiro toma como oráculo.

**Evidência nova que a `ERRATA-2` não tinha: a resposta do próprio upstream pra "que texto representa
esta cor" é alfa direto, exatamente pra esses dois campos.** Quando o RmlUi real converte um
`ColorStopList` ou um `BoxShadowList` pra `String` -- a mesma pergunta que a linha `PROP` deste
documento responde, só que pra um consumidor diferente -- ele despré-multiplica primeiro:
`examples/RmlUi/Source/Core/TypeConverter.cpp:223` (`ColorStopList` → `String`):
`dest += ToString(stop.color.ToNonPremultiplied());`; `TypeConverter.cpp:256` (`BoxShadowList` →
`String`): `dest += ToString(shadow.color.ToNonPremultiplied()) + temp;`. O próprio princípio da
`ERRATA-2` -- "reportar o que quer que o pipeline de fato produziu" (seção 7.1, raciocínio (1)) -- não
é derrubado, é **reaplicado a um fato que a `ERRATA-2` não tinha**: o próprio passo de serialização
textual do pipeline despré-multiplica. Imprimir os bytes pré-multiplicados como estão não era "ficar
mecânico", era pular um passo que o próprio upstream dá.

**🔴 O ponto que mais provavelmente causaria divergência de byte, e que ninguém tinha medido até
agora: o round trip é lossy, nos dois sentidos, e o valor impresso não é igual nem ao valor escrito na
folha nem ao valor armazenado após o parse.** `(canal * 255) / alpha` é divisão inteira, truncante,
igual ao próprio passo de pré-multiplicação `(canal * alpha) / 255` -- duas truncagens em série não se
invertem uma à outra. Medido diretamente (`(canal*alpha)/255` depois `(resultado*255)/alpha`, as duas
truncantes):

| escrito no RCSS | guardado (pré-mult.) | impresso (alfa direto) |
| :--- | :--- | :--- |
| `#22d3ee80` | `#11697780` | **`#21d1ed80`** |
| `#c9a24b40` | `#32281240` | **`#c79f4740`** |
| `#22d3ee00` | `#00000000` | **`#00000000`** |

Três valores distintos pra `alpha<255`, não dois. Um implementer esperando que o byte impresso seja
igual ao byte autoral -- uma expectativa razoável pra todo *outro* campo de cor que este documento
define -- vai divergir em silêncio na primeira fixture que usar `alpha<255` em
`box-shadow`/stop-de-gradiente. **A fórmula normativa, declarada explicitamente pra um segundo
implementer não ter de reconstruí-la: `impresso = alpha > 0 ? (canal_armazenado * 255) / alpha : 0`,
truncamento inteiro, exatamente a guarda e a aritmética que `ToNonPremultiplied()` usa
(`Colour.h:105-107`) -- nunca `round()`, nunca divisão de ponto flutuante.**

**O exemplo trabalhado da seção 9.1 é corrigido pra bater (abaixo, na seção em inglês) -- a cor
impressa da camada 2 muda uma *terceira* vez ao longo da história deste documento: `#22d3ee26` (texto
da era `ERRATA-1`, ecoava o literal-fonte ingenuamente) → `#051f2326` (`ERRATA-2`, bytes
pré-multiplicados como estão) → `#21d0ea26` (`ERRATA-4`, alfa direto via `ToNonPremultiplied()` -- e
note que **não** é o mesmo byte do `#22d3ee26` original, exatamente o round-trip lossy medido acima).**
A própria lacuna de inércia da seção 9.1 sinalizada pela `ERRATA-3` (a ordem `inset`/cor da fixture é
canônica, não independente de posição; as duas camadas especificam `spread` explicitamente, então o
caminho de default nunca é impresso) **permanece intocada por esta errata e continua aberta** --
fechá-la precisa de uma fixture verificada por corpus (a `ERRATA-3` já nomeia a candidata,
`#22D3EE 0dp 0dp 8dp` do `gusworld_battle_cockpit.rml`), que é território da
`UIX-RCSS-DUMP-A`/censo, não uma afirmação que esta errata pode verificar sem inventar uma fixture
própria. Redeclarada aqui em vez de deixada pra ser redescoberta.

**Reversibilidade: não é custo zero -- a `UIX-RCSS-DUMP-A` já foi entregue contra a leitura da
`ERRATA-2`, e a própria afirmação de cabeçalho deste documento ("nenhum dumper da `RMLX-2` existe
ainda") está desatualizada a partir desta errata.** O próprio item da `UIX-RCSS-DUMP-A` no `TODO.md`
declara isso explicitamente: a cor é impressa "como está, sem despremultiplizar" -- exatamente a
decisão que esta errata reverte. Reverter o *documento* é two-way e grátis; o *código* passa a
divergir da spec que deveria implementar, pra toda fixture `alpha<255` de `box-shadow`/stop-de-gradiente,
até um patch de acompanhamento aplicar `ToNonPremultiplied()` em `glintfx/src/rml/rcss_dump.{hpp,cpp}`
(fora de escopo aqui -- caminho proibido pra esta fatia). O `Lado B` (`UIX-RCSS-DUMP-B`, atrás da
`UIX-VALUE-COMPUTE` ainda em andamento) não foi entregue ainda, então não custa nada e não precisa de
correção. Sinalizado no `TODO.md` como acompanhamento pra não se perder.

---

## 🔵 Errata (`UIX-RCSS-ERRATA-5`, 2026-08-06) / Errata (`UIX-RCSS-ERRATA-5`, 2026-08-06)

**EN:** Two líder decisions, made the same day on two unrelated `RMLX-2` findings, both land in this
same document -- one author, to avoid two agents colliding on the same file.

**Decision 1 -- the reversed-order `border-top` shorthand (`UIX-SHORTHAND-PARCIAL`/`UIX-RCSS-DUMP-B`,
`TODO.md`): our own engine's atomic-discard behaviour is correct and STAYS**; upstream's own
in-loop, no-rollback partial write (`ERRATA-2`'s own second correction, §6.2 above) is confirmed as
a real upstream quirk that **Side A alone** reproduces (because Side A *is* upstream code) and
**Side B deliberately does not** -- the líder judged upstream's own behaviour here a
dictionary-mutation artifact (no staging buffer, no rollback), not an intentional CSS semantic worth
bug-for-bug replicating in a clean-room engine. **§6.2's own two corrections above, and §11's own
malformed-shorthand bullet, describe upstream/Side A's own real behaviour and are UNCHANGED and still
correct as a trace of what upstream does** -- what changes is that they no longer describe Side B by
implication. §15.2's worked example is split below into a Side A line and a Side B line, no longer a
single shared answer. This is the **first entry** of a brand-new section, §14.1 below,
**"Deliberate divergences"** -- a permanent, by-design Side A ≠ Side B byte mismatch for one named
input, tracked separately from §14's own divergence ledger (whose three classes all assume the two
sides are *expected to eventually agree*; this is the opposite kind of fact, and conflating the two
would corrupt §14's own escalation-threshold signal).

**Decision 2 -- unrecognized keyword value, the sixth fail-high case (`UIX-RCSS-DUMP-B`, `TODO.md`):**
§11's original five fail-high cases never named "a well-formed identifier that is not a legal member
of *this* property's own accepted keyword set" (`display: blocc;` being the reported example) --
`value_compute.hpp` offers no such check today, so Side B currently prints the raw text verbatim
instead of reverting, a genuine fail-high gap, correctly self-reported rather than silently patched
with an invented table. **Closed here:** a sixth §11 bullet, and §6.1's own registry table gains the
enumerated legal keyword set for the 11 rows that, until this errata, carried the bare word
"keyword" with no member list (the other 18 keyword-domain rows already had theirs named inline).
Unlike Decision 1, **this is not a Side A/Side B divergence** -- both sides reject the same bad token
the same way, so it needed no §14.1 entry, only a completed rule.

**PT:** Duas decisões do líder, tomadas no mesmo dia sobre dois achados não-relacionados da `RMLX-2`,
as duas pousam neste mesmo documento -- um autor só, pra evitar dois agentes colidindo no mesmo
arquivo.

**Decisão 1 -- o shorthand `border-top` de ordem revertida (`UIX-SHORTHAND-PARCIAL`/
`UIX-RCSS-DUMP-B`, `TODO.md`): o comportamento de descarte atômico do nosso próprio motor está
correto e FICA**; a própria escrita parcial do upstream, dentro do laço, sem rollback (a própria
segunda correção da `ERRATA-2`, seção 6.2 acima) é confirmada como uma peculiaridade real do
upstream que **só o lado A** reproduz (porque o lado A *é* código upstream) e **o lado B
deliberadamente não reproduz** -- o líder julgou esse comportamento do upstream um artefato de
mutação de dicionário (sem buffer de staging, sem rollback), não uma semântica CSS intencional que
valha a pena replicar bug-por-bug num motor clean-room. **As duas correções da seção 6.2 acima, e o
próprio bullet de shorthand malformado da seção 11, descrevem o comportamento real do
upstream/lado A e continuam INALTERADAS e corretas como rastro do que o upstream faz** -- o que muda
é que elas não descrevem mais o lado B por implicação. O exemplo trabalhado da seção 15.2 é dividido
abaixo numa linha do lado A e numa linha do lado B, não mais uma resposta compartilhada única. Esta
é a **primeira entrada** de uma seção inteiramente nova, a seção 14.1 abaixo,
**"Divergências deliberadas"** -- uma discrepância de byte lado A ≠ lado B permanente, por design,
pra uma entrada nomeada, rastreada separadamente do próprio registro de divergências da seção 14
(cujas três classes assumem todas que os dois lados **devem eventualmente concordar**; isto é o fato
oposto, e misturar os dois corromperia o próprio sinal de limiar de escalonamento da seção 14).

**Decisão 2 -- valor de palavra-chave não-reconhecido, o sexto caso fail-high
(`UIX-RCSS-DUMP-B`, `TODO.md`):** os cinco casos fail-high originais da seção 11 nunca nomeavam
"um identificador bem-formado que não é membro legal do próprio conjunto de palavras-chave aceitas
*daquela* propriedade" (`display: blocc;` sendo o exemplo reportado) -- o `value_compute.hpp` não
oferece essa checagem hoje, então o lado B atualmente imprime o texto cru verbatim em vez de
reverter, uma lacuna fail-high genuína, corretamente auto-reportada em vez de remendada em silêncio
com uma tabela inventada. **Fechado aqui:** um sexto bullet na seção 11, e a própria tabela de
registro da seção 6.1 ganha o conjunto de palavras-chave legal enumerado pras 11 linhas que, até esta
errata, carregavam a palavra nua "keyword" sem lista de membros (as outras 18 linhas de domínio
palavra-chave já tinham a própria lista nomeada inline). Diferente da Decisão 1, **esta NÃO é uma
divergência lado A/lado B** -- os dois lados rejeitam o mesmo token ruim do mesmo jeito, então não
precisou de entrada na seção 14.1, só de uma regra completada.

---

## 🟤 Errata (`UIX-RCSS-ERRATA-6`, 2026-08-07) / Errata (`UIX-RCSS-ERRATA-6`, 2026-08-07)

**EN:** ⚠️ **Reported by the orchestrator as an autonomous-mode decision, pending the líder's
retroactive confirmation** (this repo's own autonomous-mode convention): this document never named
the `style="..."` attribute anywhere, and Side B never read it -- a real, corpus-measured gap,
closed here by amending the spec FIRST, then implementing (`UIX-INLINE-STYLE`, `TODO.md`). Measured
by an independent QA pass enumerating the 4 unique inline-style-value forms this repo's own corpus
actually contains (`decorator: image(...)`, a percentage `width`/`left`, `font-size: <N>dp`), all 4
failing 100% before this errata: Side A (real RmlUi) honoured the attribute, Side B silently fell
back to the registry's own initial value, with **zero diagnosable trace** -- the differential
oracle's own `fonteng_ab_visual_scene.rml`/`fonteng_perf_scene.rml` (`font-size`),
`system_menu__config_audio_sliders.rml` (`width`/`left`), and `npc_dialogue__*` (`decorator`)
divergences are this exact gap, not a coincidence.

**What was missing, closed here:**

1. **The attribute IS read, and IS in scope.** `glintfx::uix::Element::attribute("style")`
   (`glintfx/src/uix/dom/dom_tree.hpp`) already stored it as an ordinary, inert generic attribute
   (that module's own header comment, "perfectly ordinary, INERT generic attributes, zero
   special-casing") -- this errata is the cascade's own missing consumer, not a DOM-layer change.
2. **Grammar and recovery: identical to a rule body's own declaration list, never a separate
   dialect.** A `style="..."` value is a flat `name: value;` run with no selector and no enclosing
   `{`/`}` of its own -- parsed by a NEW, dedicated entry point,
   `glintfx::uix::style::parse_inline_style` (`parser.hpp`/`.cpp`), which reuses the SAME
   registry/shorthand `apply_declaration` machinery §11 below already governs for an ordinary rule
   body, UNCHANGED -- an unknown property name or malformed shorthand value is dropped with a
   `ParseDiagnostic` naming the raw offending text, every OTHER declaration in the same attribute
   still applies, matching real upstream's own `Element.cpp:1753-1772` (`attribute == "style"`
   branch calling `StyleSheetParser::ParseProperties` directly, a SEPARATE entry point from
   `Parse()`, never a synthetic selector wrap).
3. **Precedence: unconditional, never a specificity comparison.** A `style="..."` declaration wins
   over EVERY `StyleSheet` rule matching the element, REGARDLESS of that rule's own specificity --
   not "a very high `Specificity` number" (this module defines no such reserved value), a
   genuinely SEPARATE mechanism, matching real upstream's own `ElementStyle::GetLocalProperty`
   (`examples/RmlUi/Source/Core/ElementStyle.cpp:48-60`): `inline_properties` is checked FIRST,
   unconditionally, before `definition` (the cascade-matched winner) is ever consulted. §1's own
   "the cascade (origin, specificity, source order)" framing gains a fourth, higher-precedence
   ORIGIN this errata names for the first time: the element's own inline declaration, evaluated
   before -- never inside -- the specificity/source-order ranking §1 already describes for
   `StyleSheet` rules.
4. **Dump appearance: no new syntax, the SAME `PROP` line an ordinary rule's own winning
   declaration already produces.** An inline-declared property's raw text goes through the exact
   same downstream value-computation pipeline §7/§8/§9 already describe (quantization, canonical
   color/composite forms) -- there is no `PROP` marker distinguishing "this value came from
   `style=\"...\"`" from "this value came from a matched rule"; the byte-exact worked example below
   is this errata's own concrete proof.
5. **Worked example (`npc_dialogue__no_com_3_escolhas.rml`, the real corpus fixture that measured
   this gap):** `<div id="npcdlg-portrait" style="decorator: image( retrato_seu_bertoldo_caim.png
   cover );"></div>`, no matching `StyleSheet` rule for `#npcdlg-portrait` at all. Side A prints
   `PROP decorator=image(retrato_seu_bertoldo_caim.png)`; Side B, before this errata, printed the
   registry's own initial value, `PROP decorator=none`. ⚠️ **A residual divergence survives this
   errata for this SAME worked example, deliberately out of `UIX-INLINE-STYLE`'s own scope:** Side
   A drops the `cover` fit keyword from its own printed `image(...)` form while Side B currently
   keeps it (`image(retrato_seu_bertoldo_caim.png cover)`) -- a PRE-EXISTING, SEPARATE divergence,
   reproduced identically by an ordinary `<style>` rule using `decorator: image(... cover)`
   (`gusworld_battle_cockpit.rml:59,229`) with no inline attribute involved at all, so it is not
   this errata's own defect to fix -- it belongs to whichever slice owns `value_compute.hpp`'s own
   `image()` composite-value printing, tracked separately, not by `UIX-INLINE-STYLE`.

**PT:** ⚠️ **Reportado pelo orquestrador como decisão de modo autônomo, pendente de confirmação
retroativa do líder** (a própria convenção de modo autônomo deste repo): este documento nunca
nomeou o atributo `style="..."` em lugar nenhum, e o lado B nunca o lia -- uma lacuna real, medida
por corpus, fechada aqui emendando a spec PRIMEIRO, depois implementando (`UIX-INLINE-STYLE`,
`TODO.md`). Medida por uma passada de QA independente que enumerou as 4 formas únicas de valor de
estilo em linha que o corpus deste repo de fato contém (`decorator: image(...)`, um `width`/`left`
percentual, `font-size: <N>dp`), as 4 falhando 100% antes desta errata: o lado A (RmlUi real)
honrava o atributo, o lado B caía em silêncio pro próprio valor inicial do registro, com **zero
rastro diagnosticável** -- as próprias divergências do oráculo diferencial em
`fonteng_ab_visual_scene.rml`/`fonteng_perf_scene.rml` (`font-size`),
`system_menu__config_audio_sliders.rml` (`width`/`left`), e `npc_dialogue__*` (`decorator`) são
esta exata lacuna, não coincidência.

**O que faltava, fechado aqui:**

1. **O atributo É lido, e ESTÁ em escopo.** O próprio `glintfx::uix::Element::attribute("style")`
   (`glintfx/src/uix/dom/dom_tree.hpp`) já o guardava como um atributo genérico, comum, inerte
   (o próprio comentário de cabeçalho daquele módulo, "atributos genéricos perfeitamente comuns,
   INERTES, zero caso-especial") -- esta errata é o próprio consumidor faltante da cascata, não uma
   mudança de camada-DOM.
2. **Gramática e recuperação: idênticas à própria lista de declaração de um corpo de regra, nunca
   um dialeto separado.** Um valor `style="..."` é um trecho plano `nome: valor;` sem seletor
   nenhum e sem `{`/`}` envolvente próprio nenhum -- parseado por um NOVO ponto de entrada
   dedicado, `glintfx::uix::style::parse_inline_style` (`parser.hpp`/`.cpp`), que reusa a MESMA
   maquinaria `apply_declaration` de registro/shorthand que a seção 11 abaixo já governa pra um
   corpo de regra comum, SEM MUDANÇA -- um nome de propriedade desconhecido ou um valor de
   shorthand malformado é descartado com um `ParseDiagnostic` nomeando o texto cru ofensor, toda
   OUTRA declaração no mesmo atributo ainda se aplica, casando com o próprio
   `Element.cpp:1753-1772` do upstream real (o ramo `attribute == "style"` chamando o
   `StyleSheetParser::ParseProperties` direto, um ponto de entrada SEPARADO do `Parse()`, nunca um
   envelope de seletor sintético).
3. **Precedência: incondicional, nunca uma comparação de especificidade.** Uma declaração
   `style="..."` vence QUALQUER regra de `StyleSheet` que case com o elemento, INDEPENDENTE da
   própria especificidade daquela regra -- não "um número de `Specificity` bem alto" (este módulo
   não define valor reservado nenhum assim), um mecanismo genuinamente SEPARADO, casando com o
   próprio `ElementStyle::GetLocalProperty` do upstream real (`examples/RmlUi/Source/Core/
   ElementStyle.cpp:48-60`): `inline_properties` é checado PRIMEIRO, incondicionalmente, antes do
   `definition` (o vencedor casado-pela-cascata) sequer ser consultado. A própria formulação "a
   cascata (origem, especificidade, ordem de fonte)" da seção 1 ganha uma quarta ORIGEM, de
   precedência MAIOR, que esta errata nomeia pela primeira vez: a própria declaração inline do
   elemento, avaliada antes -- nunca dentro -- do ranqueamento por especificidade/ordem-de-fonte
   que a seção 1 já descreve pras regras de `StyleSheet`.
4. **Aparência no dump: nenhuma sintaxe nova, a MESMA linha `PROP` que a própria declaração
   vencedora de uma regra comum já produz.** O texto cru de uma propriedade declarada em linha
   passa pelo exato mesmo pipeline de computação-de-valor rio-abaixo que as seções 7/8/9 já
   descrevem (quantização, formas canônicas de cor/composto) -- não existe marcador `PROP`
   distinguindo "este valor veio de `style=\"...\"`" de "este valor veio de uma regra casada"; o
   exemplo trabalhado byte-exato abaixo é a própria prova concreta desta errata.
5. **Exemplo trabalhado (`npc_dialogue__no_com_3_escolhas.rml`, a própria fixture real de corpus
   que mediu esta lacuna):** `<div id="npcdlg-portrait" style="decorator: image(
   retrato_seu_bertoldo_caim.png cover );"></div>`, nenhuma regra de `StyleSheet` casando
   `#npcdlg-portrait` de jeito nenhum. O lado A imprime
   `PROP decorator=image(retrato_seu_bertoldo_caim.png)`; o lado B, antes desta errata, imprimia o
   próprio valor inicial do registro, `PROP decorator=none`. ⚠️ **Uma divergência residual
   sobrevive a esta errata pra este MESMO exemplo trabalhado, deliberadamente fora do próprio
   escopo da `UIX-INLINE-STYLE`:** o lado A descarta a própria palavra-chave de encaixe `cover` da
   própria forma impressa de `image(...)` enquanto o lado B atualmente a mantém
   (`image(retrato_seu_bertoldo_caim.png cover)`) -- uma divergência PRÉ-EXISTENTE, SEPARADA,
   reproduzida identicamente por uma regra `<style>` comum usando `decorator: image(... cover)`
   (`gusworld_battle_cockpit.rml:59,229`) sem atributo inline nenhum envolvido, então não é defeito
   próprio desta errata pra consertar -- pertence a qualquer fatia que possua a própria impressão
   de valor-composto `image()` do `value_compute.hpp`, rastreada separadamente, não pela
   `UIX-INLINE-STYLE`.

---

## ⚫ Errata (`UIX-RCSS-ERRATA-7`, 2026-08-07) / Errata (`UIX-RCSS-ERRATA-7`, 2026-08-07)

**EN:** A generalisation this document's own §7/§7.1 has carried since `UIX-RCSS-ERRATA-2`/
`ERRATA-4` -- "box-shadow layer colors and gradient-stop colors are premultiplied" (Finding B,
§7.1) -- names the RIGHT fact for `linear-gradient`, `radial-gradient` and `box-shadow` (verified
again, independently, below), but its own wording ("gradient-stop colors") is ambiguous enough to
also read as covering `horizontal-gradient`/`vertical-gradient`'s own two-color `start`/`stop`
arguments -- colloquially also "gradient stop" colors, even though upstream parses them through a
STRUCTURALLY DIFFERENT class. That reading propagated into this repo's own `value_compute.cpp`
(side B) and its own module comment, which applied the SAME premultiply/un-premultiply round trip
to `horizontal-gradient`/`vertical-gradient` too, corrupting the printed RGB channels at low alpha
(`UIX-GRADIENT-ALFA`, commit `3256683`, `TODO.md`) -- a real bug, now fixed, whose root cause was
this document's own unclosed ambiguity, not an implementation slip independent of the spec.

**What was actually verified, file:line, by two independent readers (the `UIX-GRADIENT-ALFA`
implementer, and re-verified here by this fatia against the same vendored upstream clone):**

- **`horizontal-gradient`/`vertical-gradient` are NOT premultiplied at parse time.**
  `examples/RmlUi/Source/Core/DecoratorGradient.h:25` (`bool Initialise(Direction direction, Colourb
  start, Colourb stop)`) and `:34` (`Colourb start, stop;`) -- the class's own stored fields are
  plain `Colourb`, never `ColourbPremultiplied`. The only two calls to `ToPremultiplied()` anywhere
  in `DecoratorGradient.cpp` for this decorator are at `:145-146`, inside
  `DecoratorStraightGradient::GenerateElementData()`, with `opacity`
  (`element->GetComputedValues().opacity()`) baked into the call -- a RENDER-time transform (mesh
  generation, `MeshUtilities::GenerateBackground`), not a PARSE/STORAGE-time one. Storing plain and
  premultiplying-with-opacity at draw time is a DIFFERENT operation from storing premultiplied, and
  only the second one round-trips lossily the way §7.1's own normative formula describes.
- **`linear-gradient`/`radial-gradient`/`box-shadow` ARE genuinely premultiplied at parse time --
  §7.1's own claim stands correct for these three, do not confuse this errata with reopening it.**
  `PropertyParserColorStopList.cpp:47`: `color_stop.color = p_color.Get<Colourb>().ToPremultiplied();`
  -- a real parse-time, storage-time call, matching `PropertyParserBoxShadow.cpp:72` already cited
  in `ERRATA-4` above. **This is the legitimate case §7.1 always described correctly.**

**The mechanism the ambiguity hid: two RCSS syntaxes reaching two upstream parser CLASSES that
share a colloquial name ("gradient stop colors") but not a premultiply behaviour.**
`horizontal-gradient(<start> <stop>)`/`vertical-gradient(<start> <stop>)` are a native
`DecoratorStraightGradientInstancer` (2-color shorthand, §6.1's own registry,
`DecoratorGradient.cpp:186-221`); `linear-gradient(...)`/`radial-gradient(...)`'s own N-stop lists
go through `PropertyParserColorStopList` instead -- a function-name dispatch upstream itself makes,
not a distinction §7's own prose drew explicitly before this errata. Reading "gradient-stop colors"
as one uniform rule for "every color argument any gradient-shaped decorator accepts" was the exact
false generalisation that shipped as a real bug.

**Closed here without silently editing §7's own prose above (this document's own house rule -- a
correction is added, not smuggled into the text it corrects):** the boundary, stated explicitly so
a third implementer does not have to re-derive it, is `linear-gradient`/`radial-gradient`/
`box-shadow` premultiplied (side A parses via `PropertyParserColorStopList`/
`PropertyParserBoxShadow`, §7.1's own formula applies) versus `horizontal-gradient`/
`vertical-gradient` straight (side A parses via `DecoratorStraightGradientInstancer`, no round
trip, plain `Colourb` printed as-is, the same straight-alpha rule every OTHER scalar color field
already follows). Side B's own fix (`UIX-GRADIENT-ALFA`, `3256683`) removed the round trip
specifically from the two-stop straight-gradient computation path, leaving the round trip untouched
for the three properties that genuinely need it.

**Almost repeated by the very fatia that found it -- the trap is reusable.** The
`UIX-GRADIENT-ALFA` implementer's own report (`TODO.md`) records almost reporting the SAME
confusion in reverse -- seeing an untouched `ToPremultiplied()` call inside `parse_gradient_stop()`
and almost flagging it as an "incomplete fix", when in fact `linear-gradient`/`radial-gradient` are
exactly the two properties that correctly KEEP the round trip. Two different RCSS syntaxes falling
into two different upstream parsers is invisible from a function's own NAME alone; only reading the
upstream source settles which side of the boundary a given decorator sits on.

**PT:** Uma generalização que a própria seção 7/7.1 deste documento carrega desde a
`UIX-RCSS-ERRATA-2`/`ERRATA-4` -- "cores de camada de box-shadow e cores de stop de gradiente são
pré-multiplicadas" (Achado B, seção 7.1) -- nomeia o fato CERTO pro `linear-gradient`,
`radial-gradient` e `box-shadow` (verificado de novo, independentemente, abaixo), mas a própria
redação ("cores de stop de gradiente") é ambígua o bastante pra também ser lida como cobrindo os
próprios argumentos `start`/`stop` de 2 cores do `horizontal-gradient`/`vertical-gradient` --
coloquialmente também cores de "stop de gradiente", ainda que o upstream os parseie por uma classe
ESTRUTURALMENTE DIFERENTE. Essa leitura se propagou pro próprio `value_compute.cpp` deste repo (lado
B) e pro próprio comentário do módulo, que aplicava o MESMO round-trip de pré-multiplicar/
despré-multiplicar também ao `horizontal-gradient`/`vertical-gradient`, corrompendo os canais RGB
impressos em alfa baixo (`UIX-GRADIENT-ALFA`, commit `3256683`, `TODO.md`) -- um bug real, agora
consertado, cuja causa raiz era a própria ambiguidade não-fechada deste documento, não um deslize de
implementação independente da spec.

**O que foi de fato verificado, arquivo:linha, por dois leitores independentes (o implementer da
`UIX-GRADIENT-ALFA`, e reverificado aqui por esta fatia contra o mesmo clone upstream vendorizado):**

- **`horizontal-gradient`/`vertical-gradient` NÃO são pré-multiplicados em tempo de parse.**
  `examples/RmlUi/Source/Core/DecoratorGradient.h:25` (`bool Initialise(Direction direction, Colourb
  start, Colourb stop)`) e `:34` (`Colourb start, stop;`) -- os próprios campos armazenados da
  classe são `Colourb` plano, nunca `ColourbPremultiplied`. As únicas duas chamadas a
  `ToPremultiplied()` em qualquer lugar de `DecoratorGradient.cpp` pra este decorator estão nas
  linhas `145-146`, dentro de `DecoratorStraightGradient::GenerateElementData()`, com `opacity`
  (`element->GetComputedValues().opacity()`) embutida na chamada -- uma transformação de tempo-de-
  RENDERIZAÇÃO (geração de malha, `MeshUtilities::GenerateBackground`), não de tempo-de-
  PARSE/ARMAZENAMENTO. Guardar plano e pré-multiplicar-com-opacity ao desenhar é uma operação
  DIFERENTE de guardar pré-multiplicado, e só a segunda faz o round-trip lossy do jeito que a
  própria fórmula normativa da seção 7.1 descreve.
- **`linear-gradient`/`radial-gradient`/`box-shadow` SÃO genuinamente pré-multiplicados em tempo de
  parse -- a própria alegação da seção 7.1 continua correta pra esses três, não confunda esta
  errata com reabri-la.** `PropertyParserColorStopList.cpp:47`:
  `color_stop.color = p_color.Get<Colourb>().ToPremultiplied();` -- uma chamada real de tempo-de-
  parse, tempo-de-armazenamento, batendo com o próprio `PropertyParserBoxShadow.cpp:72` já citado na
  `ERRATA-4` acima. **Este é o caso legítimo que a seção 7.1 sempre descreveu corretamente.**

**O mecanismo que a ambiguidade escondia: duas sintaxes RCSS chegando a duas CLASSES de parser
upstream que compartilham um nome coloquial ("cores de stop de gradiente") mas não um comportamento
de pré-multiplicação.** `horizontal-gradient(<start> <stop>)`/`vertical-gradient(<start> <stop>)`
são um `DecoratorStraightGradientInstancer` nativo (shorthand de 2 cores, o próprio registro da
seção 6.1, `DecoratorGradient.cpp:186-221`); as próprias listas de N stops do `linear-gradient(...)`/
`radial-gradient(...)` passam pelo `PropertyParserColorStopList` em vez disso -- um despacho por
nome-de-função que o próprio upstream faz, não uma distinção que a própria prosa da seção 7 tinha
desenhado explicitamente antes desta errata. Ler "cores de stop de gradiente" como uma regra
uniforme única pra "todo argumento de cor que qualquer decorator em formato de gradiente aceita" foi
exatamente a falsa generalização que virou bug real.

**Fechado aqui sem editar em silêncio a própria prosa da seção 7 acima (a própria regra da casa
deste documento -- uma correção é somada, não contrabandeada pro texto que ela corrige):** a
fronteira, declarada explicitamente pra um terceiro implementer não ter de rederivá-la, é
`linear-gradient`/`radial-gradient`/`box-shadow` pré-multiplicados (o lado A parseia via
`PropertyParserColorStopList`/`PropertyParserBoxShadow`, a própria fórmula da seção 7.1 se aplica)
contra `horizontal-gradient`/`vertical-gradient` diretos (o lado A parseia via
`DecoratorStraightGradientInstancer`, sem round-trip, `Colourb` plano impresso como está, a mesma
regra straight-alpha que todo OUTRO campo de cor escalar já segue). O próprio conserto do lado B
(`UIX-GRADIENT-ALFA`, `3256683`) removeu o round-trip especificamente do caminho de computação do
gradiente reto de 2 stops, deixando o round-trip intocado pras três propriedades que genuinamente
precisam dele.

**Quase repetido pela própria fatia que o achou -- a armadilha é reutilizável.** O próprio relatório
do implementer da `UIX-GRADIENT-ALFA` (`TODO.md`) registra que quase reportou a MESMA confusão ao
contrário -- viu uma chamada intocada de `ToPremultiplied()` dentro de `parse_gradient_stop()` e
quase sinalizou como um "conserto incompleto", quando na verdade `linear-gradient`/`radial-gradient`
são exatamente as duas propriedades que corretamente MANTÊM o round-trip. Duas sintaxes RCSS
diferentes caindo em dois parsers upstream diferentes é invisível só pelo próprio NOME da função; só
ler o fonte do upstream resolve de que lado da fronteira um decorator dado está.

---

## English

### 1. Scope of this dump: computed values, not used values

This dump reports **CSS computed values** -- the value each property has *after* the cascade
(origin, specificity, source order) and *after* resolving every unit that does not require box
geometry (absolute lengths, `em`/`rem` against the font-size chain, `dp` against `dp_ratio`,
viewport units against the viewport, angles, numbers) -- but **before** anything that requires
`RMLX-3` (layout): a box-relative percentage is not resolved to pixels here, because doing so needs
a containing-block size this wave does not have. This is not a simplification invented for
convenience; it mirrors real RmlUi's own architecture, evidenced directly:
`ComputeLength(NumericValue value, float font_size, float document_font_size, float dp_ratio,
Vector2f vp_dimensions)` (`examples/RmlUi/Source/Core/ComputeProperty.cpp:52-69`) takes a font-size
chain, a `dp_ratio`, and viewport dimensions -- **it does not take a containing-block size at all**.
Percentages on box-relative properties (`width`, `margin-top`, `top`, ...) stay `Unit::PERCENT` all
the way through `ComputeProperty`; they are only resolved to pixels later, during layout, via a
separately-threaded `base_value` parameter this wave's `ComputeLength` signature has no room for.
Section 5.1 names the three families a `%` can belong to and states, for each, whether this dump
resolves it now or keeps it symbolic.

**Consequence for the oracle:** side A and side B both cascade + resolve absolute units, both leave
box-relative/gradient-stop/radial-center percentages as symbolic `<number>%` tokens, and neither
attempts geometry it structurally cannot have yet. A future `RMLX-3` dump (box-relative % resolved
against real layout) is **not** an extension of this one by default -- same discipline
`docs/uix-dom.md` section 10 states for its own out-of-wave boundary.

### 2. Node addressing (reused, unchanged, from `docs/uix-dom.md`)

This dump walks the **same tree**, addressed the **same way**, as `docs/uix-dom.md` section 3:
`body` is the literal root, every descendant path is `body` followed by one `/<n>` segment per
level, `n` the node's 0-based position among its parent's **surviving** children (whitespace-only
text nodes excluded, per `docs/uix-dom.md` section 6a -- unchanged here, this wave does not
redefine tree shape). `<head>`'s own opacity (`docs/uix-dom.md` section 4) is also unchanged: `head`
carries no styleable elements and gets no `PROP` records at all.

**Only element nodes carry `PROP` records.** Text nodes (`TEXT` in `docs/uix-dom.md`'s vocabulary)
have no RCSS properties of their own -- style is computed per-`Element`, and a text node is not an
`Rml::Element`. A path that is a text node's path in the `RMLX-1` dump simply does not appear in
this dump at all; there is no `PROP` block for it, and a conforming dumper must not synthesize one
(e.g. by copying the parent's computed values onto the text node's own path -- that would silently
invent a fact this format does not define).

### 3. File shape: `STATE` blocks, then per-node `PROP` enumeration

**Format overview (`UIX-RCSS-ERRATA-2`, stated explicitly here for the first time -- the sibling
`docs/uix-dom.md` §1 opens with this same sentence for its own format, this document never had the
equivalent, and an audit found the gap):** textual, UTF-8, one fact per `PROP`/`PROPS`/`STATE` line.
The byte-`==` comparator §8 defines is this format's own chosen comparison mechanism -- a deliberate
departure from `docs/uix-dom.md`'s plain-string-`diff`, not an oversight; §8 already states why (the
quantization step is where this format's "forgiveness" lives, so the comparator itself can stay a
strict `==`).

**File terminator (`UIX-RCSS-ERRATA-2`, closing a gap `docs/uix-dom.md` §1 already closed for its own
format and this document had not yet inherited by name):** the dump file always ends with a single
trailing newline after the very last `PROP` line of the very last `STATE` block -- same convention
and same justification as `docs/uix-dom.md`'s own file-terminator clause (avoids a spurious "no
newline at end of file" diff line, load-bearing for the byte-`==` comparator §8 defines: a dump
missing this trailing byte and one that has it are *different files* even when every printed line is
identical). No blank line between `STATE` blocks, none at the very start of the file.

The dump has **N top-level `STATE` blocks** (section 4 below defines exactly which states, and
why exactly that many, not more, not fewer), each a **complete, independent, full-tree property
enumeration**, back to back in one file:

```
STATE <name>
<path> PROPS <n>
<path> PROP <property-name>=<canonical-value>
... (n of these, in ascending byte-wise order by <property-name>, one per registry entry
     regardless of whether that node's rule set actually declared it -- section 6)
... (repeated for every node in the tree, in the exact same pre-order depth-first order
     docs/uix-dom.md section 5 defines, root `body` included)
STATE <name>
... (the whole walk again, under the next forced state)
```

`<path> PROPS <n>` is **always** present for every element node, `n` **always** equal to the fixed
registry size (section 6) -- never fewer, because this dump enumerates the **whole registry per
node, not just what was set** (the brief's own instruction, and the concrete lesson of
`docs/uix-dom.md`'s own worked evidence: a dump-only-the-set format cannot distinguish "this
property was never touched, initial value applies" from "this property was set to a value that
happens to equal the initial value" from "the engine has a bug and silently dropped the
declaration" -- all three produce identical *output* under dump-only-the-set, and only the first
two are supposed to). A node with **zero** author-facing declarations anywhere in its cascade chain
still emits the full `n`-line block, entirely initial-or-inherited values -- this is not treated as
"nothing to report" the way `docs/uix-dom.md`'s `CHILDREN 0` still gets its own line (same
reasoning: an absent block proves nothing, a full block of initial values proves someone looked).

**Escaping.** Every `<canonical-value>` is escaped exactly per `docs/uix-dom.md` section 2's
four-rule table (`\`, `\n`, `\r`, `\t` -- literal space and every UTF-8 multi-byte sequence pass
through unchanged), reused verbatim, not reinvented, for the same reason section 2 of that document
gives: a second escape convention invented here would be one more place for two independent
implementers to diverge on something that carries zero new information.

### 4. State matrix: enumerate the small space, don't search inside it

**Decision, and the reasoning behind it (the house's own "enumerate the whole small space instead
of searching inside it" rule, cited by the brief itself):** `RMLX-2`'s corpus census
(`UIX-RCSS-CENSUS`, 62 files, `/var/tmp/censo-rcss-qa1/censo.md` section 2, also
`docs/rmlx-subset.md` section 2's independently-verified 12 numbers) measures **`:hover`: 53-37
uses depending on which repository is counted, always in a composite selector** (`.foo:hover`,
never bare) and **`:focus`: 3, `:active`: 2** -- both real but two orders of magnitude rarer than
`:hover`. Rather than trying to guess *which specific elements* a fixture author intended to be
hovered/focused/active (a search inside an unbounded space -- there is no reliable way to infer
"the mouse is over this button" from static markup), this dump forces each boolean pseudo-class
**globally, for every element simultaneously**, and enumerates that small, closed, 2-member state
space in full:

| State name | What it forces | Corpus justification |
| :--- | :--- | :--- |
| `none` | No pseudo-class forced on any element (RmlUi/glintfx default: nothing hovered, focused, or active) | The baseline every fixture is authored against |
| `hover-all` | `:hover` forced **true** on every element in the tree, simultaneously | 37-53 real uses, always composite, this wave's stated "produto principal" |

**File order of `STATE` blocks (`UIX-RCSS-ERRATA-2`, found by an independent audit, `Finding E`):
the table's own row order above, `none` first, is a fixed, prose-declared sequence -- it was
previously only ever shown (this table, §15.1's worked example), never stated as a rule.** This is
**not** resolved by the byte-wise sort rule §3/§6 use for property names and other tokens elsewhere
in this document -- `"hover-all"` sorts before `"none"` byte-wise (`'h'` < `'n'`), which is the
opposite order the table and every worked example already use, so that rule must **not** be assumed
to also govern `STATE` order without this sentence saying so. A future `focus-all`/`active-all`
addition appends to the end of this same fixed sequence, in the order the table above gains the new
rows, never by re-sorting the existing ones.

`:focus` and `:active` are **not** separate rows in this wave's matrix -- 3 and 2 measured uses
respectively is real, non-zero usage (unlike `nth-child`/`:not`/`z-index`'s measured zero in
`docs/rmlx-subset.md` section 2, which that document correctly treats as a real-zero cut), so the
**pseudo-class matching mechanism itself must be generic** (any of `:hover`/`:focus`/`:active` must
be a selectable, settable boolean flag per element in both engines' selector matcher -- not a
`:hover`-hardcoded special case), but this wave's **oracle matrix** exercises only `hover-all` by
name. Adding `focus-all` and `active-all` rows later is a **one-line addition to this table**, not
a redesign, precisely because the matching mechanism underneath was never hover-specific to begin
with. This is recorded as a deliberate, bounded scope decision, not a silent omission -- if a future
consumer's corpus grows `:focus`/`:active` usage meaningfully, the fix is "add a row here", per
this document's own header clause, not "discover it fixture by fixture" the way
`UIX-HEAD-PREFIXO-CEGO` had to be discovered by a live probe rather than being visible in writing.

**Why `hover-all` (every element at once) instead of one dump per hoverable element:** the
alternative -- finding every element any `:hover`-suffixed rule could match and dumping one state
per such element -- turns a fixed, small, enumerable state space into a search whose size depends
on the fixture (exactly the anti-pattern the brief's own worked example warns against). Forcing
`:hover` true everywhere at once is a **single, deterministic, corpus-independent** state that
still exercises every `:hover` rule in the stylesheet in one pass (an element with no `:hover` rule
targeting it computes identically in both states, which is itself useful signal -- a divergence
appearing only in `hover-all` for a node with no hover rule at all would point at a bug in the
matcher's global-scope handling, not at the rule itself). The cost, stated plainly: `hover-all` is
not a state any real user session produces (only a subset of elements is ever hovered at once) --
this dump proves the cascade *computes* `:hover` styling correctly, not that the *event pipeline*
that decides which single element is hovered at runtime is correct (that is `RMLX-5`'s job, out of
this dump's scope, restated in section 12).

### 5. Percentages: three resolution families, one symbolic print form

**The líder's own framing, verbatim from `TODO.md`'s `RMLX-2` entry, and why a single
`resolve_percent()` is wrong by construction:** a `%` value means three geometrically unrelated
things depending on which property carries it, evidenced by the census
(`/var/tmp/censo-rcss-qa1/censo.md` section 5.1) and by `ComputeProperty.cpp`'s own function
signatures (section 1 above):

| Family | Properties (measured) | Resolves against | Resolved by this dump? |
| :--- | :--- | :--- | :--- |
| (a) Box-relative | `width`, `height`, `top`, `right`, `bottom`, `left` (also, unmeasured but structurally identical: `margin-*`, `padding-*`, `min/max-width/height`, `flex-basis`) | The **containing block**'s content-box dimension -- a layout fact | **No.** Stays a symbolic `<number>%` token (section 1's own boundary) |
| (b) Gradient-stop position | `<position%>` inside `linear-gradient(...)`/`radial-gradient(...)` (`decorator`, `mask-image`, and the `<fill>` argument of `polygon(...)`) | The gradient's **own axis** (0% = first point of the axis, 100% = last), independent of the element's box size | **No.** Stays symbolic (section 9's own composite grammar) -- see the note below on *auto-spacing*, which is resolved despite the position itself staying symbolic |
| (c) Radial-center coordinate | `<x%> <y%>` in `radial-gradient(circle at <x%> <y%>, ...)` | The gradient's **own 2D local space** (its own inscribed circle / axis box, per `docs/effects.md`'s `polygon()`+`radial-gradient` grammar) -- also not the element's border-box in the general CSS sense, and specifically **not** the same axis family (b) resolves against | **No.** Stays symbolic, same reasoning as (b) |

**Why families (b) and (c) don't need `RMLX-3` even though they stay symbolic:** unlike family (a),
families (b) and (c) never resolve against the *element's* containing block at all -- they resolve
against the decorator's own local coordinate space, which is itself only fully known at render time
(the element's own box, which for family (b)/(c)'s specific case is available slightly earlier than
family (a)'s containing-block dependency, but *this wave draws the line at "no box geometry of any
kind"*, so (b)/(c) are deferred for the same reason (a) is, not because they are harder). The point
of naming three families instead of writing `resolve_percent()` once is **not** "some resolve now
and some resolve later" -- in this wave, **none of the three resolve now** -- it is that a future
implementer wiring up `RMLX-3`/`RMLX-9` must dispatch each `%` occurrence to the *correct* one of
three different base quantities, and conflating them (e.g. accidentally feeding a gradient-stop
percentage into the containing-block-width resolver) is exactly the class of bug a single generic
function invites and this table exists to prevent.

**Gradient stop auto-spacing IS resolved by this dump, and this is not a contradiction of the
above.** `docs/effects.md` (section "How-to: a polygon with a gradient fill") documents, as already
shipped glintfx behaviour: *"Stop `<position%>` is optional and auto-spaced like CSS when omitted"*.
Auto-spacing is a pure function of **stop index and total stop count within the same gradient
function** -- it needs zero box geometry, zero layout, nothing this wave lacks -- so this dump
resolves every **omitted** stop position to its explicit CSS-standard auto-spaced percentage before
printing it (section 9.2 gives the exact algorithm and a worked example). This is the one place in
this document where a percentage that started symbolic in the source becomes a concrete printed
number: the *distribution* is resolved (deterministic, geometry-free), the *base* it is a
percentage *of* (the gradient axis, family (b)) is not.

**Decision closing a reported ambiguity: families (b) and (c) are never merged, even though both
print as a bare `<number>%` inside the same gradient functions.** `UIX-RCSS-SPEC`'s own delivery
report flagged this as an open reading: "a second implementer reading fast could fuse (b) and (c)
because they are syntactically similar" -- both are `%`, both appear only inside
`linear-gradient(...)`/`radial-gradient(...)`, and nothing in the raw RCSS token stream marks one
differently from the other (family (b)'s `<position%>` and family (c)'s `<x%> <y%>` are both just
`Declaration`/`Prelude`-adjacent numeric-percent text by the time `UIX-RCSS-LEXER`'s own tokens reach
a future parser). **Argument for merging into one "gradient `%`" family:** it is less code -- one
resolver function, parameterized by "1D axis" vs. "2D local space", could serve both, and CSS itself
does not name them as different *kinds* of percentage the way this document's table does. **Argument
against, and the decision:** merging would be exactly the class of bug section 5's own opening
paragraph names for conflating any two of the three families -- family (b) resolves against the
gradient's own 1D axis (0% = first point, 100% = last, a **length along a line**), family (c)
resolves against the gradient's own 2D local coordinate space (the inscribed circle / axis box, **an
`(x, y)` pair, not a scalar offset**) -- these are not the same *quantity*, only the same *print
form*, and a resolver written to "handle a gradient percentage" generically invites exactly the
accidental cross-feed this document's families table exists to prevent (§5's own worked instance:
feeding a gradient-stop percentage into the wrong resolver). **They stay two distinct families,
never one.** This is not merely restated prose -- section 15.3 below gives the byte-exact worked
form of all three families side by side, specifically so a second implementer cannot land on the
merged reading even skimming fast: the two families never share an argument position within one
function call (family (c) is always the first 1-2 arguments of `radial-gradient`, before its own
`circle at` keyword is consumed; family (b) is always inside a `<stop>` entry, after the color), so
a worked example that shows both in the same function call closes the ambiguity a prose-only rule
cannot.

### 6. The property registry (the fixed, closed list every `PROPS` block enumerates)

**Scope discipline, stated once so it needn't be repeated per row:** `TODO.md`'s own `RMLX-2` scope
text says *"só as propriedades medidas em uso"* -- this registry is built **exclusively** from
names the census (`/var/tmp/censo-rcss-qa1/censo.md` section 3) measured at least once, expanded
through shorthand definitions (a shorthand like `margin` is not itself a registry entry -- see
section 6.2 -- but its constituent longhands are, whether or not that specific longhand name was
*also* measured written directly). A property this table does not list is **out of subset by
default**, per `docs/rmlx-subset.md`'s own clause -- section 13 below names every measured-zero or
at-rule-only exclusion explicitly, so a future implementer who finds one in a real fixture stops and
edits this spec rather than guessing.

**Evidentiary source for every default value and every `inherited` flag in the table below** (cited
once, not per row, to keep the table readable): `examples/RmlUi/Source/Core/
StyleSheetSpecification.cpp:262-433` (native RmlUi properties, function signature
`RegisterProperty(id, name, default_value, inherited, forces_layout)` -- **note the 4th positional
argument is `inherited`, not `forces_layout`; a reader skimming the call sites must not swap
these**, which is exactly the kind of misreading this document exists to pre-empt by stating the
parsed result directly rather than making an implementer re-derive it from an unfamiliar call
signature); `glintfx/src/rml/decorator_ripple.cpp:332-336` and
`glintfx/src/rml/decorator_image_tint.cpp:409-411` (the glintfx-authored custom properties).

**Sort order for the `PROP` lines within a `PROPS` block: ascending, byte-wise, by property name --
`std::string::operator<` over raw UTF-8 bytes, no locale, same rule and same justification as
`docs/uix-dom.md` section 7's `CLASS`/`ATTR` ordering** (reproducible across machines, no argument
about which locale is "correct"). This is **not** the property's internal `PropertyId` enum order
in either engine -- an internal enum's numeric order is an implementation detail neither side should
have to replicate identically, exactly the shared-private-assumption risk `docs/uix-dom.md`'s own
"why this document exists" section warns against.

#### 6.1 Registry table (72 longhand entries, alphabetical -- the dump's own required order)

| Property | Initial value | Inherited | Value domain (section 7) |
| :--- | :--- | :---: | :--- |
| `align-items` | `stretch` | no | keyword(`flex-start`,`flex-end`,`center`,`baseline`,`stretch`) |
| `animation` | `none` | no | composite (§9.3) |
| `backdrop-filter` | *(empty)* | no | composite filter-list (§9.2) |
| `background-color` | `transparent` | no | color |
| `border-bottom-color` | `black` | no | color |
| `border-bottom-left-radius` | `0px` | no | length |
| `border-bottom-right-radius` | `0px` | no | length |
| `border-bottom-width` | `0px` | no | length |
| `border-left-color` | `black` | no | color |
| `border-left-width` | `0px` | no | length |
| `border-right-color` | `black` | no | color |
| `border-right-width` | `0px` | no | length |
| `border-top-color` | `black` | no | color |
| `border-top-left-radius` | `0px` | no | length |
| `border-top-right-radius` | `0px` | no | length |
| `border-top-width` | `0px` | no | length |
| `bottom` | `auto` | no | keyword(`auto`) or length-percent (§5, family a) |
| `box-shadow` | `none` | no | composite shadow-list (§9.1) |
| `box-sizing` | `content-box` | no | keyword(`content-box`,`border-box`) |
| `color` | `white` | **yes** | color |
| `column-gap` | `0px` | no | length |
| `cursor` | *(empty)* | **yes** | string |
| `decorator` | *(empty)* | no | composite decorator-list (§9.2) |
| `display` | `inline` | no | keyword(`none`,`block`,`inline`,`inline-block`,`flow-root`,`flex`,`inline-flex`,`table`,`inline-table`,`table-row`,`table-row-group`,`table-column`,`table-column-group`,`table-cell`) |
| `filter` | *(empty)* | no | composite filter-list (§9.2) |
| `flex-basis` | `auto` | no | keyword(`auto`) or length-percent (family a) |
| `flex-grow` | `0` | no | number |
| `flex-shrink` | `1` | no | number |
| `focus` | `auto` | **yes** ⚠️ | keyword(`none`,`auto`) |
| `font-family` | *(empty)* | **yes** | string |
| `font-size` | `12px` | **yes** | length (relative to itself is disallowed; resolves via §8 `em`/`rem` rules) |
| `height` | `auto` | no | keyword(`auto`) or length-percent (family a) |
| `image-tint-color` | `white` | no | color |
| `image-tint-mode` | `none` | no | keyword(`none`,`multiply`,`luminance-multiply`,`screen`) |
| `image-tint-threshold` | `0.55` | no | number, clamped `[0, 0.999]` |
| `justify-content` | `flex-start` | no | keyword(`flex-start`,`flex-end`,`center`,`space-between`,`space-around`,`space-evenly`) |
| `left` | `auto` | no | keyword(`auto`) or length-percent (family a) |
| `letter-spacing` | `normal` | **yes** | keyword(`normal`) or length |
| `line-height` | `1.2` | **yes** | number or length-percent (relative to `font-size`) |
| `margin-bottom` | `0px` | no | keyword(`auto`) or length-percent (family a) |
| `margin-left` | `0px` | no | keyword(`auto`) or length-percent (family a) |
| `margin-right` | `0px` | no | keyword(`auto`) or length-percent (family a) |
| `margin-top` | `0px` | no | keyword(`auto`) or length-percent (family a) |
| `mask-image` | *(empty)* | no | composite decorator-list (§9.2) |
| `max-height` | `none` | no | keyword(`none`) or length-percent (family a) |
| `max-width` | `none` | no | keyword(`none`) or length-percent (family a) |
| `min-height` | `0px` | no | length-percent (family a) |
| `min-width` | `0px` | no | length-percent (family a) |
| `opacity` | `1` | **yes** ⚠️ | number, clamped `[0, 1]` |
| `overflow-x` | `visible` | no | keyword(`visible`,`hidden`,`auto`,`scroll`) |
| `overflow-y` | `visible` | no | keyword(`visible`,`hidden`,`auto`,`scroll`) |
| `padding-bottom` | `0px` | no | length-percent (family a) |
| `padding-left` | `0px` | no | length-percent (family a) |
| `padding-right` | `0px` | no | length-percent (family a) |
| `padding-top` | `0px` | no | length-percent (family a) |
| `position` | `static` | no | keyword(`static`,`relative`,`absolute`,`fixed`) |
| `right` | `auto` | no | keyword(`auto`) or length-percent (family a) |
| `ripple-origin-x` | `0` | no | number (px, custom glintfx) |
| `ripple-origin-y` | `0` | no | number (px, custom glintfx) |
| `ripple-phase` | `0` | no | number (custom glintfx) |
| `ripple-strength` | `0` | no | number (px, custom glintfx) |
| `ripple-width` | `48` | no | number (px, custom glintfx) |
| `row-gap` | `0px` | no | length |
| `tab-index` | `none` | no | keyword(`none`,`auto`) |
| `text-align` | `left` | **yes** | keyword(`left`,`right`,`center`,`justify`) |
| `text-overflow` | `clip` | no | keyword(`clip`,`ellipsis`) or string |
| `text-transform` | `none` | **yes** | keyword(`none`,`capitalize`,`uppercase`,`lowercase`) |
| `top` | `auto` | no | keyword(`auto`) or length-percent (family a) |
| `transform` | `none` | no | composite transform-list (§9.4) |
| `vertical-align` | `baseline` | no | keyword(`baseline`,`middle`,`sub`,`super`,`text-top`,`text-bottom`,`top`,`center`,`bottom`) or length-percent (relative to `line-height`) |
| `white-space` | `normal` | **yes** | keyword(`normal`,`pre`,`nowrap`,`pre-wrap`,`pre-line`) |
| `width` | `auto` | no | keyword(`auto`) or length-percent (family a) |

**⚠️ Two entries that read as surprising and are correct as measured, flagged so nobody "fixes"
them later:** `focus` is `inherited: true` in upstream RmlUi despite controlling something
(whether `Element::Focus()` can succeed) that has no intuitive notion of "inheriting" -- confirmed
directly at the registration call site, not inferred. `opacity` is also `inherited: true`, which is
**not** how CSS's own `opacity` behaves (real CSS `opacity` does not inherit; it visually compounds
through stacking contexts instead) -- RmlUi's own model is a genuinely different mechanism (each
descendant's own opacity, if unset, cascades from its ancestor's *value*, and RmlUi additionally
**multiplies** opacities down the render tree at draw time, `docs/embed-integration.md` is silent on
this and it is out of this dump's own scope since it is a render-time compounding, not a
cascade-time computed value -- the computed `opacity` value itself, which this dump reports, is
exactly the CSS-inheritance-style single cascaded number, not the compounded product).

**⚠️ `max-height`/`max-width`: the one registry entry with zero corpus justification, kept on
purpose, not a bug to fix later.** `UIX-PROP-REGISTRY`'s own delivery closed the 64-vs-72 accounting
(section 6 above) and found these two are the **only** 2 of the 72 longhand entries with **zero**
measured occurrences anywhere in this document's own corpus (`/var/tmp/censo-rcss-qa1/censo.md`) --
not written directly, and not reachable through any of the 13 shorthands section 6.2 defines (no
shorthand expands into `max-height`/`max-width`; they are plain, unexpanded RmlUi native properties).
Section 6's own scope discipline states this registry is built "exclusively" from measured names --
by that rule alone these two do not belong. **They stay in the registry anyway**, for two reasons
stated once here so a future reader does not re-litigate them fixture by fixture: (1) this document's
own table (section 6.1 above) already listed them before the corpus-exclusivity discipline was
written down, and the spec is the contract two independent dumper authors build against -- removing
an already-published registry entry needs the same "stop, edit this spec with a diff, líder sign-off"
discipline section 13 requires for *adding* an out-of-subset item, not a silent drop; (2) the more
durable reason, restated from this project's own standing rule: the glintfx target is **broad
distribution**, and "zero occurrences in this repo's two-project corpus" is a true statement about
two repositories, never a true statement about the world -- a consumer this document has never seen
may genuinely author `max-height: 200px;` tomorrow. **The teto this decision is bounded by:** these
two entries are pinned exactly as they are today -- `keyword(none)` or length-percent (family a),
same domain and print form as every other box-relative property in the table -- by
`glintfx/tests/uix_style/property_registry_sanity.cpp`'s own
`test_max_height_max_width_are_the_one_known_unexplained_gap`; a future census that measures a real
use of either is a **confirmation**, not a discovery, and changes nothing about this decision; a
future census that finds a *third* zero-corpus-but-listed entry is a **new** anomaly and must be
reported the same way this one was, not silently folded into this same justification.

**🔵 Enumerated keyword sets, added by `UIX-RCSS-ERRATA-5`, 2026-08-06 -- 29 of these 72 rows are
`Keyword` domain (either the sole domain, or the primary domain of a two-domain row); zero rows have
`Keyword` only as the *alternate* domain. Of the 29, 18 already carried their own legal member set
named inline before this errata** (`bottom`, `flex-basis`, `focus`, `height`, `image-tint-mode`,
`left`, `letter-spacing`, `margin-bottom`, `margin-left`, `margin-right`, `margin-top`, `max-height`,
`max-width`, `right`, `tab-index`, `text-overflow`, `top`, `width` -- each a single word, `auto`/
`none`, or a short enumerated pair/quad) -- **the remaining 11 carried only the bare word "keyword",
with no member list, until this errata: `align-items`, `box-sizing`, `display`, `justify-content`,
`overflow-x`, `overflow-y`, `position`, `text-align`, `text-transform`, `vertical-align`,
`white-space`.** All 11 are closed above, in the table's own cells, the same convention the other 18
already used -- this table remains the single source of truth for every Keyword-domain property's
own legal set; a future implementer does not consult a second document or the upstream source
directly for this list, only cite it here.

**Source and a judgment call, named so it can be overridden in one place:** all 11 sets are the
**full upstream RmlUi enumeration**, verbatim from each property's own `RegisterProperty(...)
.AddParser("keyword", "...")` call site (`examples/RmlUi/Source/Core/StyleSheetSpecification.cpp`):
`align-items` `:417`, `box-sizing` `:318`, `display` `:306-307`, `justify-content` `:426`,
`overflow-x`/`overflow-y` `:335-336`, `position` `:308`, `text-align` `:361`, `text-transform` `:363`,
`vertical-align` `:331-333`, `white-space` `:364` -- `image-tint-mode`'s own 4-member set is
glintfx-authored, not upstream, already correctly enumerated before this errata
(`glintfx/src/rml/decorator_image_tint.cpp:409-411`), unaffected by this pass, restated here only for
the denominator's own completeness. **Why full upstream, not a corpus-narrowed subset** (the same
"real zero is a real cut" discipline section 13 below applies to *unmeasured properties and value
syntax forms*): accepting a keyword string is a zero-cost set-membership check, not new
parser/serializer machinery the way accepting a functional color form (`rgb()`, section 13) or a new
property (`transition`, section 13) would be -- there is nothing here analogous to "implementing a
feature nobody asked for". This dump's own scope is computed **values**, never layout (§12); printing
`display=table-row` as a legal computed value commits this project to nothing about ever laying out a
table, exactly as printing an out-of-viewport length commits it to nothing about ever rendering
off-screen. The closest existing precedent is section 6.3's own `Box`-algorithm 3-value row: real,
unmeasured, upstream, cheap to include faithfully, kept **because** it costs nothing beyond honestly
implementing an algorithm this table already requires for the measured cases. **Flagged for the
líder's attention regardless, not silently decided** (the same courtesy `ERRATA-2`'s premultiply
call and `ERRATA-4`'s reversal of it both extended): `display`'s own 14-member upstream list is the
one most likely to deserve a second look, since several of its members name layout modes
(`table-row`, `flow-root`, ...) this codebase has no plan to ever lay out -- if the líder judges that
a corpus-narrowed `display` set (measured: `none`, `block`, `inline-block`, plus `flex` per
`docs/rmlx-subset.md` section 2's own consumer census, 10 instances) is the right call instead, that
is a one-cell edit here, not a design change anywhere else in this document.

#### 6.2 Shorthand-to-longhand expansion (no separate registry slot; feeds the longhand entries above)

Evidence: `examples/RmlUi/Source/Core/StyleSheetSpecification.cpp` `RegisterShorthand` calls +
`PropertySpecification.cpp:311-472` (the four expansion algorithms, cited per-type below).

| Shorthand (raw name measured) | Expands to | Algorithm | Corpus-observed value-count distribution (census §4) |
| :--- | :--- | :--- | :--- |
| `margin` | `margin-top/-right/-bottom/-left` | **Box** (§6.3) | 1-value: 44, 2-value: 16, 4-value: 39 (3-value: 0 measured, still valid per §6.3) |
| `padding` | `padding-top/-right/-bottom/-left` | **Box** | 1-value: 29, 2-value: 59, 4-value: 18 (3-value: 0 measured) |
| `border-radius` | the 4 `border-*-radius` corners | **Box** | 100% 1-value (2/3/4-value: 0 measured, still valid per §6.3) |
| `border-color` | the 4 `border-*-color` | **Box** | 100% 1-value |
| `border-top`/`-right`/`-bottom`/`-left` | that side's `-width` + `-color` (no `-style` -- **RmlUi has no border-style property at all**, confirmed by its absence from every `Register(Shorthand\|Property)` call touching `border`) | **FallThrough** (each token routes to whichever of `-width`/`-color` its own shape matches first; **NOT** order-independent -- see the errata block above this document's header and the note directly below this table) | -- |
| `border` | the 4 `border-top/-right/-bottom/-left` shorthands above | **RecursiveRepeat** (the same 2-token value string is fed to all 4 side-shorthands verbatim) | 100% 2-part (width + color, never a 3rd token) |
| `background` | `background-color` only | **FallThrough**, 1 item | 100% solid-color value (§4.2 of the census: `docs/effects.md`'s own documented restriction -- gradients go through `decorator`, never `background`) |
| `gap` | `row-gap` + `column-gap` | **Replicate** (1 value sets both; 2 values set each independently) | -- |
| `overflow` | `overflow-x` + `overflow-y` | **Replicate** | -- |
| `flex` | `flex-grow`, `flex-shrink`, `flex-basis` | **Flex** (special-cased: the bare keyword `none` expands to `0 0 auto`; otherwise omitted trailing values default to `1`/`1`/`0`, **not** each property's own normal initial value -- `PropertySpecification.cpp:311-334`, cited because this is exactly the kind of "an ordinary reader would guess wrong" default a second implementer could plausibly miss) | -- |

**Correction to the `border-top`/`-right`/`-bottom`/`-left` row above, dated 2026-08-06 (see the
errata block at this document's own header for the full account):** "order-independent between the
two" was **false** for a 2-item/2-token `FallThrough` chain. The real rule, traced
iteration-by-iteration against upstream's own real loop
(`examples/RmlUi/Source/Core/PropertySpecification.cpp:429-471`, read directly): upstream always
advances the **item** cursor every iteration (match or not), and only advances the **token** cursor
on a match -- so a token that fails item 0's own domain and only matches item 1 (the reversed,
color-then-width order) gets claimed by item 1, leaving the item cursor exhausted with the other
token still unclaimed, and upstream's own post-loop guard (`value_index < property_values.size() &&
property_index >= items.size()`) aborts the **entire shorthand** -- not a partial result. Concretely,
for `border-top`: `1dp #7A5A2E` (width-then-color, the corpus's own 100%-measured order) succeeds;
`#7A5A2E 1dp` (color-then-width) is `MalformedValue`, and `ParseShorthandDeclaration` returns
`false` for the whole `border-top` declaration -- **but see the `UIX-RCSS-ERRATA-2` correction
directly below the table before trusting what that `false` return implies about which longhands
actually end up holding.** **What "order-independent" IS true for:** which *domain* a token routes to is content-driven
(a token that looks like a length routes to `-width` regardless of which position it appears in) --
that part of the original sentence was not wrong. **What it is not true for:** that an arbitrary
token *order* always succeeds for a 2-item/2-token chain. It does not. Section 15.2 below gives the
byte-exact dump for both orders side by side. Proof, not merely asserted: pinned by
`glintfx/tests/uix_style/shorthand_expansion_sanity.cpp`'s own
`test_border_top_fallthrough_order_is_load_bearing`, and already correctly stated in
`glintfx/src/uix/style/shorthand.hpp:38`/`shorthand.cpp:30-35` before this document was corrected to
match.

**Second correction (`UIX-RCSS-ERRATA-2`, 2026-08-06): the paragraph above is itself half wrong about
the *consequence* of that `false` return, found by an independent audit (`UIX-RCSS-AMBIGUIDADE`,
`docs/uix-rcss-ambiguidades.md` Finding A) and reverified line-by-line by the `tech-lead` before this
correction was written.** It is **not true** that "both `border-top-width` and `border-top-color`
keep whatever the cascade's next-lower-specificity rule provides" for the reversed-order case.
Upstream's `FallThrough`/`Box` loop (`PropertySpecification.cpp:433-472`) has **no staging buffer** --
`dictionary.SetProperty(...)` (`:461`) fires **inside the loop, the moment any item's `ParseValue`
succeeds**, before the post-loop guard (`:469-471`) ever runs. Tracing `#b { border-top: #7A5A2E 1dp;
}` token by token: iteration 1, item 0 (`-width`) tries `"#7A5A2E"`, fails to parse as a length, and
(being `FallThrough` with a next item available) `continue`s -- the `for`-loop's own increment
advances `property_index` to 1, `value_index` stays `0` (a `continue` in C++ does not skip the
`for`-header's own increment clause, it only skips the loop body's remaining statements).
Iteration 2, item 1 (`-color`) tries the **same, still-unclaimed** `"#7A5A2E"`, succeeds, and
`dictionary.SetProperty(BorderTopColor, ...)` fires **right there** -- `border-top-color` is set from
the source token. Only *after* the loop does the post-loop guard see `value_index(1) <
property_values.size()(2)` and `property_index(2) >= items.size()(2)`, and return `false`. Nothing
downstream of that `false` undoes the `SetProperty` call that already happened --
`StyleSheetParser::ReadProperties` (`StyleSheetParser.cpp:1023`) only logs a warning on `false`, and
`PropertyDictionary::SetProperty` (`PropertyDictionary.cpp:8`) is a bare `properties[id] = property;`
with no transactional layer to roll back. **The corrected consequence for `body/1` (`#b`):
`border-top-color=#7a5a2eff` (set from the source token, item 1 matched before the failure surfaced)
, `border-top-width=0.0000px` (item 0 never matched anything in this call, falls back to its §6.1
registry initial) -- only `-width` reverts, `-color` does not.** Section 15.2 below is corrected to
match. The same mechanism applies to `border`'s own `RecursiveRepeat` (see the table row above and
the matching correction in section 11 below): each of the 4 side-shorthand sub-calls runs to
completion independently (`result &= ParseShorthandDeclaration(...)`, `PropertySpecification.cpp:379`,
no early exit across the loop at `:375-388`), so a reversed-order `border: #7A5A2E 1dp;` partial-writes
all 4 `-color` longhands from the source token (each side's own `FallThrough` sub-call hits the same
bug independently) while leaving all 4 `-width` longhands untouched, *before* the aggregate
`result=false` makes the outer call return failure.

**🔵 Scope of the paragraph above, narrowed by `UIX-RCSS-ERRATA-5`, 2026-08-06: this is a trace of
upstream's own real behaviour, true of Side A (which *is* upstream code) and, unless a fixture proves
otherwise, of `border`'s own `RecursiveRepeat` case too -- it is no longer, by the líder's own
decision, a description of Side B.** Side B's own clean-room engine (`shorthand.cpp`/`parser.cpp`)
discards a `FallThrough`/`RecursiveRepeat` declaration **atomically** on the same reversed-order
input -- no longhand keeps a partial match, every one of them falls back to its own §6.1 registry
initial (or inherited value) -- and that atomic-discard behaviour is the one that **stays**, kept
deliberately rather than bug-for-bug matched to upstream's own no-rollback dictionary mutation. See
§14.1 below, "Deliberate divergences", for the full account, the byte-exact pair, and the fixture
obligation this decision carries. §15.2's own worked example is split accordingly.

##### 6.3 The `Box` algorithm (verbatim from `PropertySpecification.cpp:336-370`, standard CSS box-model expansion)

| Values given | top | right | bottom | left |
| :---: | :---: | :---: | :---: | :---: |
| 1 | v0 | v0 | v0 | v0 |
| 2 | v0 | v1 | v0 | v1 |
| 3 | v0 | v1 | v2 | v1 |
| 4 | v0 | v1 | v2 | v3 |

The 3-value row is **not** measured anywhere in the census's corpus but is a real, reachable
upstream behaviour (same `Box`-type shorthand engine `margin`/`padding` already use for their
measured 1/2/4-value forms) -- included here for full parity because it costs nothing beyond
faithfully implementing the one algorithm already required for the measured cases, not a
speculative extension.

### 7. Value-domain canonical print forms

Every `<canonical-value>` in a `PROP` line is produced by exactly one of these rules, keyed by the
property's domain in section 6.1's table:

| Domain | Canonical print rule |
| :--- | :--- |
| `keyword` | The exact registered keyword string (lowercase, matching `StyleSheetSpecification.cpp`'s own `AddParser("keyword", "...")` lists verbatim -- e.g. `display` prints one of `none`/`block`/`inline`/... exactly as spelled there), never re-cased |
| `number` | The quantized decimal number (§8), no suffix, no sign for a positive value, `-` prefix for negative |
| `length` (resolved) | The quantized decimal number (§8) **plus literal suffix `px`**, always `px` regardless of the unit the source used -- see §8.1 for why the printed unit is always `px` |
| `length-percent` (family a) | **Either** the resolved-length form above **or** `<quantized-number>%` (§5's symbolic form) -- never both; whichever the cascade's winning declaration specified |
| `color` | 8-digit lowercase hex `#rrggbbaa`, straight (non-premultiplied) alpha -- §7.1 |
| `string` | The raw string content, escaped per §3's escaping rule, **no surrounding quotes** even if the RCSS source quoted it (quoting is source syntax, not part of the computed string value) |
| composite (shadow-list / decorator-list / filter-list / transform-list / animation) | §9's own per-domain grammar |

**String-domain, empty computed value (`UIX-RCSS-ERRATA-2`, closing a gap the sibling document
`docs/uix-dom.md` §7 already closed for its own `ATTR data-if=` case but this document had not yet
stated for its own analogous case):** `cursor` and `font-family` both register with `*(empty)*` as
their §6.1 initial value. A `PROP` line for either, when the computed value is the empty string,
still prints -- `<path> PROP cursor=` (nothing after `=`), never omitted -- the same convention
`docs/uix-dom.md` §7 states by name for `ATTR data-if=` (present-with-empty-value and absent are
different, both queryable, states; a `string`-domain `PROP` line always exists per §3's own "the
whole registry, every node" rule, so "present with empty value" is the only reachable state here --
there is no "absent" state to distinguish it from, unlike a DOM attribute).

**String-domain values do not escape this document's own composite separators (`UIX-RCSS-ERRATA-2`,
closing `Finding H`):** §3's four-rule escape table (`\`, `\n`, `\r`, `\t`, inherited from
`docs/uix-dom.md` §2) is the **only** escaping a `string`-domain value gets. It does **not** cover
`\|`, `;`, or `:` -- this document's own composite-list, argument, and stop separators (§9), chosen
specifically so a *composite* dump line is never ambiguous about which comma-role a byte played in
the source. A `font-family`, `cursor`, or `text-overflow` string value containing a literal `;` or
`:` (both legal RCSS string content) prints byte-for-byte unescaped at the top level -- this is a
deliberate scope limit, not an oversight: §9's "never ambiguous" goal was stated for *composite*-value
grammar, and is not retroactively extended to a **plain, non-composite** `PROP` line's string value. A
`PROP` line is therefore only safely re-splittable on `=` (once, at the first occurrence) plus
whatever domain-specific grammar §9 defines for that specific property -- it is not a general-purpose
delimited record.

**Structural identifiers are never escaped (`UIX-RCSS-ERRATA-2`, mirroring `docs/uix-dom.md` §8's
identical treatment of `<tag>`):** the `<property-name>` half of a `PROP` line's `name=value` pair is
never escaped per §2's table, for the same reason `docs/uix-dom.md` §5 gives for `<tag>` -- it is
always one of the 72 fixed, closed, ASCII kebab-case identifiers §6.1's own table names (chosen by
the *dumper*, iterating its own registry, never echoed back from arbitrary source-authored casing or
content the way an attribute value is), and none of those 72 strings can structurally contain any of
the 4 escape characters (`\`, `\n`, `\r`, `\t`). Escaping is therefore never *reachable* for this
field, and this document names that explicitly rather than leaving a second implementer to wonder
whether it was forgotten.

#### 7.1 Color: canonical form, and the exact scope authorized today

**In scope, authorized by the census's own measured 4 hex forms** (`/var/tmp/censo-rcss-qa1/censo.md`
section 6, cross-checked against `examples/RmlUi/Source/Core/PropertyParserColour.cpp:211-237`'s
`switch (value.size())`): `#rgb` (3 hex digits), `#rgba` (4), `#rrggbb` (6), `#rrggbbaa` (8) --
**all four**, per the census's own correction of `showcase.rcss:8`'s stale comment (the code and
real usage both support all four; the comment did not). All four normalize to the same canonical
8-digit output form: each single-digit channel doubles (`f` → `ff`, matching upstream's own
fallthrough `case 5 → case 4`/`case 9 → case 7` duplication), a missing alpha channel defaults to
`ff` (fully opaque, matching upstream's `hex_values[3] = {'f','f'}` pre-fill default), then all four
channels print as two lowercase hex digits each, in `rgba` order, prefixed by `#`.

**Out of scope, requires the líder's sign-off before implementation (§13):** the two named colors
the census actually measured (`transparent` → `#00000000`, `white` → `#ffffffff`) are in scope
because they are measured; the **rest** of RmlUi's 19-entry named-color table (`black`, `red`,
`blue`, ... -- `PropertyParserColour.cpp:117-136`) and every functional color form (`rgb()`,
`rgba()`, `hsl()`, `lab()`, `oklab()`, `lch()`, `oklch()`) are **zero-measured** in the census
(section 0: *"0 `rgb()`/`rgba()` funcional"*) and are **not** authorized by the líder's units-parity
decision, which named units specifically, not color syntax. A conforming dumper encountering any of
these must **fail-high** (§11), not silently support them by accident because the parser happened
to be easy to extend.

**Colors are dumped straight-alpha for scalar color-typed properties -- `background-color`,
`border-*-color`, `color`, `image-tint-color`.** `Style::ComputedValues`/`Property::Get<Colourb>`
store straight alpha for these; there is no premultiply call anywhere in their parse path.

**Corrected, `UIX-RCSS-ERRATA-2` (2026-08-06): `box-shadow` layer colors and every gradient-stop
color are the exception, and are premultiplied, not straight -- found by an independent audit
(`UIX-RCSS-AMBIGUIDADE`, Finding B) and reverified directly by the `tech-lead`.** The original text
above claimed these two fields were straight-alpha "including" for `box-shadow`/gradient-stop colors
-- **false**. `PropertyParserBoxShadow.cpp:72`: `shadow.color = prop.Get<Colourb>().ToPremultiplied();`
(the citation drifted to `:69` in an earlier draft -- `:72` is the verified line in the checked-out
upstream clone). `PropertyParserColorStopList.cpp:47` (the parser behind every gradient stop in
`linear-gradient`/`radial-gradient`, both used by `decorator`/`mask-image`/`filter`/
`backdrop-filter`): `color_stop.color = p_color.Get<Colourb>().ToPremultiplied();` -- same pattern.
Both calls fire **at parse time**, before the value ever reaches `Style::ComputedValues`, and the
**struct field's own type** is `ColourbPremultiplied` (`Include/RmlUi/Core/DecorationTypes.h:9`,
`:22`), a type distinct from the straight `Colourb` every scalar color-typed field uses -- there is
no straight-alpha representation of these two fields anywhere downstream of parsing, the type system
enforces it structurally.

**Decision superseded, `UIX-RCSS-ERRATA-4` (2026-08-06), reverted by the líder himself: print
`ToNonPremultiplied()` of the stored bytes -- straight alpha, not the premultiplied bytes as-is.**
`ERRATA-2`'s reasoning above is kept in place rather than deleted (this document's own house rule:
a wrong decision that gets erased comes back), but two of its three legs do not hold. Reasoning (2)
-- "un-premultiplying is undefined at `alpha=0`" -- is **false**, measured against
`examples/RmlUi/Include/RmlUi/Core/Colour.h:105-107`: `ToNonPremultiplied()`'s own body is
`ColourType(alpha > 0 ? (red * 255) / alpha : 0)` (same guard on green/blue), an explicit,
total-for-every-input guard -- there is no undefined case to avoid. And there is evidence `ERRATA-2`
did not have that undercuts reasoning (1) -- "report whatever the pipeline actually produced": when
real RmlUi's own pipeline converts these exact two composite types to text, it un-premultiplies
first, `TypeConverter.cpp:223` (`ColorStopList` → `String`) and `TypeConverter.cpp:256`
(`BoxShadowList` → `String`), both calling `ToNonPremultiplied()`. Upstream's own answer to "what
text represents this color" is straight alpha for these two fields, same as every other -- printing
the raw premultiplied bytes was not staying mechanical, it was skipping a step upstream itself takes.
Reasoning (3) survives in spirit but not in conclusion: Side A stays a faithful reader of what is
stored, and *also* applies the one mechanical, upstream-defined transform (`ToNonPremultiplied()`)
that upstream's own text-conversion path applies at exactly this boundary -- not an invented
transform, a borrowed one. **Consequence: a conforming Side A and Side B both print straight
`Colourb`-shaped bytes for every color-typed field without exception, `box-shadow`/gradient-stop
included -- §7.1's general rule now covers these two fields too, no exception clause needed.
🔴 This conversion is lossy in both directions (two truncating integer divisions in series do not
invert each other) -- the printed byte equals neither the value authored in the source nor the value
`Style::ComputedValues` stores after parsing; see `UIX-RCSS-ERRATA-4` above for the measured table and
the normative formula.** Section 9.1's own worked example below is corrected to match, a third time.

### 8. Numeric quantization: the rule, chosen and justified

**The problem this rule solves.** Side A and side B are two independently-written engines doing
logically equivalent but not bit-identical floating-point arithmetic (different operation order,
different compiler, possibly FMA on one side and not the other). A byte-exact `==` comparison
between two floats computed by different code paths will occasionally disagree in the last one or
two bits of a `float32` mantissa **even when both engines are correct** -- that is ULP noise, not a
bug. The oracle needs a single rule, fixed **before** either side is coded, that a real bug (wrong
formula, wrong base, wrong sign, off by a percent, off by a whole unit) reliably fails and that ULP
noise reliably passes -- and it must print identically for two independent implementations of "the
same rule", which rules out anything that depends on library-internal rounding-mode state a second
implementer would have to divine rather than read.

**The rule, stated as an explicit algorithm (not "use `%.4f`", which is under-specified -- see
below for why):**

```
quantize(x: float32) -> string:
    d := (double)x                      // widen once, to compute the decimal digits precisely
    scaled := d * 10000.0
    rounded := trunc(scaled + copysign(0.5, scaled))   // round-half-away-from-zero, explicit
    q := rounded / 10000.0
    if q == 0.0: q := 0.0                // canonicalize -0.0 to 0.0 (no "-0.0000" ever printed)
    return fixed-point decimal string of q, exactly 4 digits after '.', no exponent,
           '-' prefix iff q < 0, no '+' prefix, '.' as the decimal point always (never locale-dependent)
```

Applied to **every** float-valued field this dump prints: resolved lengths (px), angles (degrees,
§8.2), unitless numbers, opacity, symbolic percentages' own numeric part, and every numeric argument
inside a composite value (§9). Colors are integers already (§7.1) and are exempt -- they need no
quantization, only the canonical hex form.

**`quantize()` is defined only for finite `x` (`UIX-RCSS-ERRATA-2`, closing `Finding J`):** a
non-finite computed value (`NaN`, `+Inf`, `-Inf`) is treated the same as any other
internally-detected computation error: it is never printed via this algorithm -- log it and fall back
to the property's own §6.1 initial value (or its inherited value), the same consequence §11 already
defines for a rejected declaration. This is this document's own general fail-high policy applied to a
numeric *computation* result rather than a *parse* result -- named explicitly because this project's
own review culture practises adversarial/mutation testing that manufactures inputs a corpus never
would, and the algorithm above (`trunc`, `copysign`, fixed-point formatting) has no defined behaviour
for a non-finite input otherwise.

**Why 4 decimal digits, chosen and not merely defaulted to:** `float32` carries roughly 7 significant
decimal digits. Every measured length in the corpus sits in the 0-3000 range (**corrected,
`UIX-RCSS-ERRATA-2`, cosmetic -- the largest single value cited here was previously `-228dp`; a
regenerated census (`tools/rcss_census.py`) measured `999dp` (`border-radius`, the "fully rounded"
idiom used throughout the corpus) and `-410dp` (`margin-left`) as larger in magnitude; the corrected
largest-observed example is `999dp`, still 3 digits before the point**); at that magnitude, 4 digits
after the point is **more** precision than `float32` can even
represent meaningfully (roughly 3-4 significant digits remain below the noise floor at that
magnitude) -- so genuine ULP-level disagreement between two correct engines is rounded away by this
step for the entire realistic value range, while any real bug (a wrong unit conversion is off by a
multiplicative factor, a wrong percentage base is off by whatever the actual base is, a sign flip
is off by 2×the value) produces a difference many orders of magnitude larger than `0.0001` and
survives quantization untouched. This mirrors the líder's own stated preference over the CTO's
tolerance-inside-the-harness proposal: the comparison itself stays a pure byte `==`, and the
"forgiveness" lives entirely in one shared, written-down, pre-registered rounding step both sides
run **before** printing -- never as runtime tolerance logic the harness applies after the fact.

**Why round-half-away-from-zero, explicitly, instead of "whatever `printf("%.4f", ...)` does":**
libc's `%f` rounding is well-defined per the *current* value (correctly-rounded per IEEE 754 in
every libc this project targets), so `%.4f` would in practice agree with the explicit algorithm
above for the overwhelming majority of inputs -- but "agree in practice, for now, on these
platforms" is precisely the kind of unstated-but-usually-true assumption `docs/uix-dom.md`'s own
"why this document exists" section warns produces spurious divergences the moment two dumpers run
under different libcs (glibc vs. musl, for instance) or different locale settings that quietly
change decimal-point rendering. Stating the algorithm in this document, independent of any specific
C standard library's formatting function, means a conforming dumper in any language, on any libc,
produces the identical string -- the same reasoning `docs/uix-dom.md` section 7 gives for choosing
byte-wise sort over locale-aware collation.

#### 8.1 Why the printed unit for every resolved length is always `px`, never the source unit

A length declared `2dp`, resolved through `dp_ratio`, and a length declared `16px` both describe
the same *kind* of computed fact (a physical pixel count) once resolved -- printing the original
source unit (`dp` vs. `px`) alongside the resolved number would let two semantically identical
computed values (say, `2dp` at `dp_ratio=8` and `16px` written directly) produce different dump
lines, and the oracle would report a divergence that is not one, exactly the failure mode
`docs/uix-dom.md` section 7 names for why `class` tokens are dumped as a sorted set rather than in
source order. `px` is the fixed, single output unit for every member of the `LENGTH` unit family
(§9 of `examples/RmlUi/Include/RmlUi/Core/Unit.h`: `PX | DP | VW | VH | EM | REM | PPI_UNIT`) after
resolution.

#### 8.2 Angles: canonicalized to degrees, always

`transform`/gradient angles accept both `deg` and `rad` as input units (§9 below, full unit
parity); this dump always prints the resolved angle in **degrees** (`RAD` values converted via
`degrees = radians * (180 / π)` before quantization), for the same "one canonical output form"
reasoning as §8.1 -- `deg` is chosen over `rad` because 100% of the corpus's angle usage
(`/var/tmp/censo-rcss-qa1/censo.md` section 5, 24 instances, all `deg`, 0 `rad`) is already
authored in degrees, and `π`'s irrationality means converting a `deg` value to `rad` and back
introduces its own small rounding noise that converting the rarer `rad` input to `deg` avoids for
the overwhelmingly common case.

**Printed form, a gap this document did not previously state and closes here:** an angle prints as
`quantize()`'s own bare output (section 8), **with no unit suffix** -- unlike `length`, which gets an
explicit, additional `px` suffix layered on top of `quantize()` by section 8.1's own stated override.
This section's own title only ever said angles are *canonicalized* to degrees (which unit the number
means); it never said whether the printed string carries that unit's own name. Left unstated, a
second implementer has two equally-plausible readings -- copy the `px`-suffix precedent and print
`90.0000deg`, or trust `quantize()`'s own literal contract ("no exponent... no unit" is never
overridden for angles the way it explicitly is for length) and print the bare `90.0000`. This
document resolves it as the bare form, because that is what section 8's own algorithm already
produces without any angle-specific override existing anywhere in this document -- `length` is the
one domain given an explicit suffix rule; angle has none, so it falls through to `quantize()`'s own
unmodified output. Section 15.3 below prints an angle inside a worked `linear-gradient(...)` example
using this rule, so a second implementer has a byte-exact anchor rather than only this prose.

### 9. Composite value serialization grammar

Every composite-domain property (§6.1: `box-shadow`, `decorator`, `mask-image`, `filter`,
`backdrop-filter`, `transform`, `animation`) is a **list** of one or more function-shaped items.
**Outer list separator: `|`. Inner argument separator inside one function: `;`.** Both are chosen
deliberately distinct from `,` (the separator RCSS source itself uses for both purposes, per
`StringUtilities::ExpandString(..., ',', '(', ')')` in every parser cited below) so that a
canonical dump line is never ambiguous about which comma-role a given byte played in the source --
a concern that does not arise for `,` in source RCSS (parenthesis-aware splitting resolves it there)
but would force every *consumer* of a dump line to reimplement that same parenthesis-aware split
for no reason, when a dump-only separator avoids the need entirely. An empty list (`none`, or an
empty string for the decorator-family properties whose initial value is `""`) prints as the literal
string `none` -- chosen over an empty string so that `<path> PROP box-shadow=` (a property that
genuinely resolved to an empty string, distinguishable from "line absent") never has to be
disambiguated from "the dumper had nothing to print"; there is no domain in this registry where an
empty-string *value* and *absence of a value* are both reachable states that need distinguishing
the way `docs/uix-dom.md` section 7 needs `ATTR data-if=` vs. attribute-absent -- every composite
property's own initial value already collapses to `none`/no-decorator, so `none` is unambiguous.

#### 9.1 `box-shadow`

Evidence: `examples/RmlUi/Source/Core/PropertyParserBoxShadow.cpp:12-83` (full source read; already
partially cited in `docs/effects.md`). **Key fact a second implementer could plausibly get wrong:**
the parser does **not** assign tokens to `offset_x`/`offset_y`/`blur`/`spread` by *position in the
whole argument list* -- it assigns them by **order of appearance among only the tokens that parse
successfully as a length**, skipping over `inset` and the color token wherever either appears in the
list (`length_argument_index` only increments on a successful length parse, `PropertyParserBoxShadow.cpp:52-66`).
Concretely: `box-shadow: inset #ff0000 0 0 10px;` and `box-shadow: #ff0000 0 0 10px inset;` are
**the same value** (`inset=true`, `offset_x=0`, `offset_y=0`, `blur=10px`, `color=#ff0000ff`) --
`inset` and the color argument may appear **anywhere** in the token list; only the length-typed
tokens are positional, and only relative to each other, not to the whole list. A dumper that
naively takes "argument N" by list position, rather than by length-parse-success order, will silently
misassign `offset_x`/`offset_y`/`blur`/`spread` the moment `inset` or the color token appears
anywhere but last, and this is exactly the kind of divergence a diff would surface as "different
number" rather than obviously flag as a parsing-order bug.

**Layer grammar:** `<color>;<offset_x>;<offset_y>;<blur>;<spread>;<inset>` -- 6 fields **always**
present (never fewer, regardless of how many the source actually wrote): `spread` defaults to
`0.0000px` when the source omits it. **Corrected, `UIX-RCSS-ERRATA-2` (2026-08-06): the omission
ratio published here was inverted.** The original text claimed "124 of 135 single-layer declarations
omit `spread`" -- a regenerated, versioned corpus census (`tools/rcss_census.py`, `docs/uix-rcss-censo.md`,
replacing the scratch `/var/tmp/censo-rcss-qa1/censo.md` this document's earlier citations point to,
which no longer exists on this machine) measured close to the **opposite**: of the corpus's
single-layer `box-shadow` declarations, only **1 omits `spread`**, the other **123 specify all 4
length fields explicitly** (independently re-derived by the `tech-lead`, walking every single-layer
declaration directly rather than trusting either count at face value: 124 total single-layer
declarations, 120 with 4 explicit length tokens, 3 more with 4 fields written as bare unitless `0`
offsets that a naive unit-tagged count missed -- `123` in total -- and exactly `1` with only 3 length
tokens, `gusworld_battle_cockpit.rml`'s `#22D3EE 0dp 0dp 8dp`). The **rule itself is unaffected** --
`spread` still defaults to `0.0000px` on the rare declaration that omits it -- only the *how common is
this* corpus statistic was backwards; a reader who took the original "124 of 135 omit" claim as
license to skip testing the 4-field explicit form against a real fixture would have been testing the
rare case and skipping the common one. `<color>` per §7.1, the four
numeric fields per §7/§8 (resolved length, `px` suffix, quantized). Multiple layers join with `|`,
**in source (author) order** -- unlike `class` tokens, box-shadow layers are an ordered rendering
stack (later layers paint on top), not a set, so this dump does **not** sort them; sorting would
destroy real, order-dependent information.

Worked example: source `box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;`
(`difficulty_menu__lista_hardcore_bloqueado.rml:62`, at `dp_ratio=1.0` for this example) dumps as
(single `PROP` line, wrapped here only for readability -- the real line has no internal newline):

```
box-shadow=#22d3eeff;0.0000px;0.0000px;0.0000px;1.0000px;true|#21d0ea26;0.0000px;0.0000px;16.0000px;0.0000px;false
```

**Corrected (`UIX-RCSS-ERRATA-2`, 2026-08-06): layer 2's color was published as `#22d3ee26`
(straight, unchanged from source) -- wrong, per §7.1's own correction above.** `#22D3EE26` = R`0x22`
(34) G`0xD3`(211) B`0xEE`(238) A`0x26`(38); premultiplied (`channel*38/255`, integer division,
truncating): R=`5`(`0x05`), G=`31`(`0x1f`), B=`35`(`0x23`), A unchanged (`38`/`0x26`) ->
`#051f2326`, the value printed by `ERRATA-2`. Layer 1's color (`#22D3EE`, implicit alpha `ff`=255)
premultiplies to itself (`channel*255/255=channel`) -- `#22d3eeff` was already correct either way,
which is why this bug did not show up in this worked example's first layer.

**Reverted (`UIX-RCSS-ERRATA-4`, 2026-08-06): layer 2's printed color is `#21d0ea26`, a third,
distinct value -- neither `ERRATA-1`'s naive `#22d3ee26` nor `ERRATA-2`'s premultiplied
`#051f2326`.** §7.1's `ERRATA-4` decision prints `ToNonPremultiplied()` of the *stored* premultiplied
bytes, not the original source literal echoed back. Applying the normative formula
(`alpha > 0 ? (channel * 255) / alpha : 0`, truncating) to the stored `#051f2326`: R=`5*255/38=33`
(`0x21`), G=`31*255/38=208` (`0xd0`), B=`35*255/38=234` (`0xea`), A unchanged (`38`/`0x26`) ->
`#21d0ea26` -- the value now printed above, and **not** the same byte as the original `#22D3EE26`
authored in the source. This is the lossy round trip §7.1/`ERRATA-4` measures generally, exercised
concretely: `#22D3EE26` (authored) → `#051f2326` (stored, premultiply truncates) → `#21d0ea26`
(printed, un-premultiply truncates again) -- three different byte sequences for one declared color,
and a conforming dumper must reproduce the middle-then-last hop, not shortcut straight from authored
to printed. Layer 1 stays `#22d3eeff` unaffected either way, `alpha=255` has no truncation to lose
in either direction (`channel*255/255=channel` exactly, both ways).

**Malformed single shadow layer (`UIX-RCSS-ERRATA-2`, closing `Finding I`, reverified directly):** a
malformed single layer inside a comma-separated `box-shadow` list aborts the **entire property**, not
just that layer. `PropertyParserBoxShadow::ParseValue` (`PropertyParserBoxShadow.cpp:12-83`) `return
false`s on the first invalid layer -- an empty argument list (`:39`), an unrecognized token that is
neither a valid length, `inset`, nor a valid color (`:78`), or a layer with fewer than 2 length
arguments (`:81`) -- and `property.value`/`property.unit` (`Unit::BOXSHADOWLIST`) are only ever
assigned at the very end of the function, after every layer parsed successfully. There is no partial
`BoxShadowList` a failure leaves behind; the whole `box-shadow` declaration reverts to its cascade/
registry-initial value (`none`), the same consequence §11 states for `decorator`/`filter` below.

#### 9.2 `decorator` / `mask-image` / `filter` / `backdrop-filter`

**Corrected (`UIX-RCSS-ERRATA-2`, 2026-08-06, `Finding F`, reverified directly): the four properties
do NOT all split their source function-list on the same character.** `decorator`/`mask-image` split
on **comma**, parenthesis-aware (`PropertyParserDecorator.cpp:55`:
`StringUtilities::ExpandString(decorator_string_list, decorator_string_value, ',', '(', ')');`).
`filter`/`backdrop-filter` are parsed by a **separate class**, `PropertyParserFilter`
(`StyleSheetSpecification.cpp:407-408`), whose own `ParseValue` splits on **space**, parenthesis-aware
(`PropertyParserFilter.cpp:32`: `StringUtilities::ExpandString(filter_string_list,
filter_string_value, ' ', '(', ')', true);`) -- matching real CSS `filter` syntax (`filter: blur(4px)
brightness(1.2);`), and the function's own preceding comment states this explicitly ("Filters are
declared as `filter: <filter-value>[ <filter-value> ...]`"). A conforming Side B dispatcher must pick
`,` vs. ` ` based on **which of the four properties it is parsing**, not assume one shape for all
four. What the four properties **do** share, unchanged: each function serializes as `name(<args>)`,
args joined by `;`, multiple functions in the list joined by `|` in **source order** in the printed
dump (decorators paint in list order, same "it's a stack, not a set" reasoning as §9.1) -- only the
*source-string* split character differs, never the *dump-format* output separators.

**In-scope functions, per the corpus's own measured decorator sub-languages and `docs/effects.md`'s
own shipped grammar -- re-enumerated in full for this errata, not merely spot-checked (`UIX-RCSS-
ERRATA-2`, see below for why):**

| Function | Args (in order) | Notes |
| :--- | :--- | :--- |
| `image(<url>)` | `<url>` (escaped string, §3) | -- |
| `linear-gradient(<angle>;<stop>;<stop>;...)` | angle per §8.2, then ≥2 stops | Angle is **mandatory** (no CSS `to <side>` form), per `docs/effects.md` |
| `radial-gradient(<cx%>;<cy%>;<stop>;<stop>;...)` | center X/Y per §5 family (c) (quantized `<number>%`, default `50.0000%;50.0000%` when the source omits `circle at ...`), then ≥2 stops | `ellipse` is not supported upstream in this grammar (`docs/effects.md`: "only `circle` is supported") -- a source using `ellipse` is a fail-high case (§11), not silently coerced to `circle` |
| `polygon(<sides>;<fill>;<rotation>)` | `sides` (integer, printed as a `number` per §8 with 4 zero decimals, e.g. `6.0000`), `fill` (either a `<color>` per §7.1, or a **nested** `linear-gradient(...)`/`radial-gradient(...)` using this same grammar recursively), `rotation` (degrees, §8.2) | Validation range `[3, 1024]` and fail-high behaviour already shipped, per `docs/effects.md` -- this dump reports the value **as validated**; an out-of-range `sides` means the decorator did not apply at all (§11), so there is no `polygon(...)` function in the list to print for that declaration |
| `image-tint(<url>)` | `<url>` | The tint itself is 3 **separate** standalone properties (`image-tint-color`/`-mode`/`-threshold`, §6.1), not decorator arguments -- do not fold them into this function's args |
| `ripple(<max-radius>)` | one number, px, §8 (default `0.0000` = auto) | The five `ripple-*` effect parameters are standalone properties, same non-folding rule as `image-tint` |
| `horizontal-gradient(<color>;<color>)` | exactly 2 colors (`mask-image`'s own 2-stop shorthand form, per `docs/effects.md`) | Registered by the same native `DecoratorStraightGradientInstancer` as `vertical-gradient` below, dispatched on the decorator's own `name` (`DecoratorGradient.cpp:186-221`) |
| **`vertical-gradient(<color>;<color>)`** | exactly 2 colors, same grammar as `horizontal-gradient` immediately above | **Added, `UIX-RCSS-ERRATA-2` -- this row was entirely missing until this errata; see the correction note below the table.** Same `DecoratorStraightGradientInstancer`, same 2-color shorthand (`direction, start-color, stop-color`, `DecoratorGradient.cpp:191`), only `name == "vertical-gradient"` at `:203` picks the vertical axis instead of horizontal |
| `blur(<radius>)` | one resolved length, px | Used by both `filter` and `backdrop-filter` |
| `drop-shadow(<color>;<x>;<y>;<blur>)` | color first (matches `box-shadow`'s own color-first convention), then 3 resolved lengths -- **no spread, no inset**, unlike `box-shadow` | Used by `filter` only, per `docs/effects.md` |

**`vertical-gradient` was missing (`UIX-RCSS-ERRATA-2`, 2026-08-06) -- found by a regenerated corpus
census, and this is the single most consequential correction in this errata pass, worse than any of
the seven findings above by volume.** `git grep -c "vertical-gradient" -- '*.rml' '*.rcss' '*.hpp'`
against the corpus: **107 occurrences across 16 files** -- the single most-used decorator function in
the entire corpus, ahead of `polygon` (47) and every gradient function. Combined with §11's own
corrected fail-high rule (`Finding C`, above: a malformed/unrecognized decorator entry drops the
**entire property**), an implementation built against the pre-errata-2 table would have silently
dropped the `decorator`/`mask-image` property to `none` on **every one** of those 107 declarations,
in 16 corpus files -- and because both Side A and Side B would independently hit the exact same
"unknown function name" fail-high path, the oracle would have diffed two equally-empty outputs and
reported **green**, for the wrong reason. This is exactly the shared-private-misreading failure mode
this document's own header section names, except the miswritten shared assumption originates in the
spec itself, not in two independent readings of it.

**Full enumeration, not a spot-check (per the orquestrador's own instruction: search the closed set,
don't search inside it for what you already suspect):** every distinct function name the corpus uses
inside a `decorator`/`mask-image`/`filter`/`backdrop-filter` value was counted (`tools/rcss_census.py`'s
own `decorator_func_counts`, which folds `transform`'s own function names into the same counter --
`rotate` appears there twice, but that is `transform: rotate(...)` from §9.4, already in scope, not a
decorator/filter function, and is excluded from the tally below). `SCOPE [funções de decorator]: 10
distintas no corpus, 9 na tabela 9.2 (antes desta errata), 1 acrescentada por esta errata.` The 10:
`vertical-gradient`(107), `polygon`(47), `drop-shadow`(23), `linear-gradient`(22), `radial-gradient`(21),
`image-tint`(13), `horizontal-gradient`(6), `image`(6), `ripple`(2), `blur`(1) -- every one of the
other 9 was already in the table above; `vertical-gradient` was the only gap, and it is now closed.

##### 9.2.1 Gradient stop grammar and the auto-spacing algorithm (§5's own resolved exception)

A stop is `<color>` alone or `<color>:<position%>` -- **`:` as the color/position separator within
one stop**, distinct from the `;` that separates whole stops, chosen because a stop's own two parts
are a tighter unit than the stop-to-stop relationship (mirrors the `body/0` vs. `body/0/0` nesting
intuition `docs/uix-dom.md` uses `/` for). `<position%>` is **always** printed, even when the source
omitted it, per this algorithm (CSS Images Module Level 3's own standard auto-spacing rule, already
the documented glintfx behaviour per `docs/effects.md`, restated here as the exact algorithm a
second implementer needs, not just the one-line prose the how-to guide gives):

```
1. Any stop with an explicit <position%> keeps it (quantized per §8, printed as <number>%).
2. The first stop, if it has no explicit position, is assigned 0%.
3. The last stop, if it has no explicit position, is assigned 100%.
4. Every remaining explicit-position-less stop is assigned a position evenly spaced between its
   nearest preceding stop's position (explicit or already-assigned-by-this-algorithm) and its
   nearest following stop's position (same), such that a run of K consecutive unpositioned stops
   between a stop at position P_before and one at position P_after receives, in order,
   P_before + i * (P_after - P_before) / (K + 1) for i = 1..K.
```

Worked example (`npc_dialogue__no_com_3_escolhas.rml:56`, from the census):
source `radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%)` dumps as:

```
decorator=radial-gradient(35.0000%;30.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%)
```

**Byte-count fix (2026-08-06, part of the same errata pass as the header block above):** this line
previously printed the last color as `#7a5a2effff` -- 10 hex digits, one channel too many. `#7A5A2E`
is a 6-digit `#rrggbb` source form (three already-2-digit channels, R=`7a`, G=`5a`, B=`2e`); section
7.1's own rule only *doubles* a single-digit channel (the `#rgb`/`#rgba` forms) and *defaults* a
missing alpha channel to `ff` -- neither rule applies twice here. Correct output is 8 hex digits:
`#7a5a2eff`. Left uncorrected, this line would have been a wrong golden value for whichever `RMLX-2`
slice eventually turns this worked example into a literal test fixture.

(the first stop, `#F0D98C`, has no explicit position in the source and is the *first* stop, so
step 2 assigns it `0.0000%` -- this is the one line in this worked example where the printed value
is **not** copied from the source text, and is exactly the kind of line a second implementer might
print as `#f0d98cff:auto` or omit the position entirely if this algorithm were only described in
prose rather than given byte-exact here).

#### 9.3 `animation`

Evidence: `examples/RmlUi/Source/Core/PropertyParserAnimation.cpp:118-204`. Grammar:
`name(;<duration>;<timing>;<iterations>;<alternate>;<paused>)` -- wait, corrected: no leading `;`,
the function name itself is the fixed literal `animation` wrapping the parsed fields, to stay
consistent with every other composite's `name(args)` shape even though the source's own `animation:`
value has no function-call syntax of its own:

```
animation(<keyframes-name>;<duration>;<timing-keyword>;<iterations>;<alternate>;<paused>)
```

`<duration>` is a resolved-to-seconds number (§8, no unit suffix in the printed form here since
`animation`'s own domain is always seconds -- confirmed by `PropertyParserAnimation.cpp:164`'s
`sscanf(argument.c_str(), "%fs%n", ...)`, which recognizes **only** a literal `s` suffix; **`ms` is
not a recognized duration unit in upstream RmlUi at all**, matching the census's own zero-`ms`
finding -- this is not a corpus-coverage gap, it is upstream's actual accepted grammar). `<iterations>`
is either a resolved integer or the literal string `infinite`. `<alternate>`/`<paused>` are literal
`true`/`false`. Multiple simultaneous animations (a comma-list in the source) join with `|` in
source order (same ordered-stack reasoning as §9.1/§9.2).

**Malformed single-animation-value (`UIX-RCSS-ERRATA-2`, closing `Finding I`, reverified directly):**
same consequence as a malformed `box-shadow` layer above -- a malformed
`<single-animation-value>` inside a comma-separated `animation` list aborts the **entire property**.
`PropertyParserAnimation::ParseAnimation` (`PropertyParserAnimation.cpp:111-206`) `return false`s
immediately on validation failure for one instance (`animation.name.empty() ||
animation.duration <= 0.0f || (animation.num_iterations < -1 || animation.num_iterations == 0)`,
`:204`), and `property`'s own `AnimationList` value is only ever assigned after every instance in the
list parses successfully. No partial list survives; the property reverts to its cascade/registry-
initial (`none`).

#### 9.4 `transform`

**Scope, stated explicitly because this is the one composite this document intentionally leaves
thinnest:** `RMLX-2`'s own scope is the cascade's computed *value* of `transform` -- the parsed list
of transform functions -- not applying that value to produce a render matrix (that is `RMLX-8`'s
job, per `docs/rmlx-subset.md` section 2's own framing: *"`transform` (19) é ~3× mais usado que
`@keyframes`... `RMLX-8`... desenhada em torno de `transform` 2D primeiro"*). The census's own
corpus shows exactly **2** `transform` instances, both `rotate(0deg)`/`rotate(360deg)` inside one
`@keyframes` block. This dump's grammar covers exactly the 2D subset that instance needs plus its
two obvious siblings, and states plainly that anything beyond this is unverified against the corpus:

```
translate(<x>;<y>) | scale(<x>;<y>) | rotate(<angle>)
```

`<x>`/`<y>` for `translate`/`scale` per §7/§8 (resolved length or plain number respectively);
`<angle>` per §8.2. **This grammar is not verified against any measured 3D or matrix transform
function** (`translate3d`, `matrix`, `perspective`, ...) -- if a real fixture is ever found using
one, per this document's own header clause, **stop and edit this section with a diff** before
implementing it; do not silently extend the grammar to fit.

### 10. `@font-face` and `@keyframes`: structural registries, not element properties

**`@font-face`** carries its own attribute set (`font-family`, `src`, and the glintfx-relevant
`-rmlui-fallback-face`) that is **not** part of the per-node `PROP` registry above -- these
attributes describe a font resource, never an `Rml::Element`, and have no cascade, no inheritance,
no specificity (`examples/RmlUi/Source/Core/StyleSheetParser.cpp:306`, `-rmlui-fallback-face`'s own
registration, is a `FontFaceId`-scoped property, a structurally separate ID space from the
element-cascade `PropertyId` space this document's §6 registry lives in). This dump does not emit
`PROP` lines for `@font-face` blocks at all; a future `RMLX-2`-adjacent font-resource dump (if one
is ever needed) is, per this document's own section-1 discipline, a new decision, not a silent
extension of this format.

**`@keyframes`** blocks are parsed and their existence/structure is in scope (the animation's own
`animation` property, §9.3, references a keyframes name), but the **keyframe selector percentage**
(`0%`, `50%`, `100%`, `from`/`to`) is a **fourth, distinct meaning of `%`** beyond §5's three
families -- a *time offset* along an animation's own timeline, not a spatial/gradient quantity at
all. **This dump does not attempt to resolve or interpolate keyframe values** -- doing so requires
an active animation instance with a running clock, which is `RMLX-8`'s runtime concern, not this
wave's static-cascade one. `from`/`to` are equivalent to `0%`/`100%` (per CSS's own convention, not
independently re-derived here) and are recorded for completeness, not exercised by this wave's
oracle.

### 11. Fail-high policy (unknown property, unknown selector form, unknown decorator function, out-of-range value)

**One rule, applied uniformly, already the canonized pattern for `polygon()`'s own `sides` range
(`docs/effects.md`):** an unrecognized or invalid construct is **logged and ignored**, never a hard
parse failure that poisons the rest of the stylesheet, never a silent guess. Concretely:

- **Unknown property name** (not in §6.1's registry): the declaration is dropped; every *other*
  declaration in the same rule block still applies.
- **Unknown selector form** (anything `docs/rmlx-subset.md` section... -- restated here for this
  wave's own selector scope: `class`/`id`/descendant/child (`>`)/tag/compound/comma-list/`:hover`
  per the líder's decision 1, `TODO.md`'s `RMLX-2` entry -- `nth-child`, `:not(`, attribute
  selectors, sibling combinators, and every pseudo-class beyond `:hover`/`:focus`/`:active` are all
  **out of subset**, matching `docs/rmlx-subset.md` section 2's own real-zero cuts): **corrected,
  `UIX-RCSS-ERRATA-2` (`Finding G`, reverified directly) -- only the individual selector that fails
  to register is dropped, not the whole rule.** `StyleSheetParser::ConstructNodes`
  (`StyleSheetParser.cpp:947-967`) resolves each comma-separated selector independently
  (`for (const String& selector : selector_list) { ... if (!leaf_node) Log::Message(...); else ...
  leaf_nodes.push_back(...); }`) -- a failed selector logs a warning and the loop **continues**;
  it never returns early, never discards `leaf_nodes` already collected, never touches the rule's own
  property declarations. The rule's declarations apply to every selector that **did** resolve,
  regardless of any sibling selector in the same comma-list that failed. "The whole rule fails" is
  only true for a **non-comma-list** selector that is itself invalid (nothing else in the list to
  fall back to). This directly touches the líder's own comma-list-selector evidence (15 corpus
  instances, including the 16-tag UA-stylesheet base rule, `docs/rmlx-subset.md` §6.1) -- a single
  typo'd tag name in that rule loses only that one tag under real RmlUi, not the whole rule.
- **Unknown decorator/filter function name**, or a known function given the wrong argument shape:
  **corrected, `UIX-RCSS-ERRATA-2` (`Finding C`, reverified directly) -- the *entire property*
  reverts, not just that one entry.** `PropertyParserDecorator::ParseValue`
  (`PropertyParserDecorator.cpp:63-131`) and `PropertyParserFilter::ParseValue`
  (`PropertyParserFilter.cpp:29-90`) both loop over their own split function-value list (§9.2 -- comma
  for `decorator`/`mask-image`, space for `filter`/`backdrop-filter`) and, on the **first** invalid
  keyword, unknown function name, or shorthand-parse failure inside one entry, `return false`
  **immediately** -- `property.value`/`property.unit` are never assigned. The caller only calls
  `dictionary.SetProperty` **after** a successful `ParseValue`, so on failure nothing is written at
  all and the property falls back, in full, to its cascade/registry-initial value (`none`). There is
  no partial list surviving -- **every** entry in that list is discarded, including the ones that
  individually would have parsed fine. This is the **same** consequence already correctly stated
  below for a malformed shorthand; the two bullets now use the same wording, not opposite ones.
- **Out-of-range numeric value** for a property/argument with a declared range (`polygon()`'s
  `sides ∈ [3, 1024]`, `image-tint-threshold ∈ [0, 0.999]`, `opacity ∈ [0, 1]`): the value clamps to
  the nearest bound **only where §6.1/§9 states a clamp**; where no clamp is stated, the whole
  declaration/decorator-entry is dropped per the two rules above, never silently clamped by
  invention.
- **Malformed shorthand value** (a recognised shorthand name, §6.2, whose raw value's own token
  count or shape does not fit any of that shorthand's accepted forms -- e.g. `border-top`'s own
  2-token chain given in the reversed, color-then-width order, §6.2's own errata note): the same
  consequence as an unknown property, in the sense that `ParseShorthandDeclaration` **returns**
  `false` and the declaration is logged as rejected -- but **corrected, `UIX-RCSS-ERRATA-2`
  (`Finding A`, reverified directly): the return value does not mean every targeted longhand reverts.**
  Upstream's `SetProperty` calls happen **inside** the parsing loop, in place, with no staging buffer
  and no rollback on the loop's later failure (§6.2's own second correction has the full byte-by-byte
  trace) -- whichever longhand had **already matched successfully before the loop's post-condition
  failure fires** keeps that matched value from the source; only the longhand that was **never**
  matched in that call falls back to cascade/registry-initial. For a shorthand whose own algorithm is
  itself composed of sub-shorthands (`border`'s `RecursiveRepeat`, §6.2): each of the 4
  side-shorthand sub-calls runs to completion independently (`result &= ...`,
  `PropertySpecification.cpp:375-388`, no early exit across the loop) **before** the aggregate
  `result=false` surfaces -- so each side's own partial-write (per the paragraph above) already
  happened for all 4 sides by the time the outer call reports failure. The **logged, reported**
  outcome is still "declaration rejected" for both cases -- what changed is which longhands the
  dictionary actually ends up holding when that rejection is reported, which is the fact this dump's
  own `PROP` lines make visible and byte-comparable. **🔵 Scope, narrowed by `UIX-RCSS-ERRATA-5`,
  2026-08-06: the partial-write consequence above is Side A's own (real upstream code) and is no
  longer, by the líder's own decision, Side B's -- Side B discards the whole declaration atomically
  instead. See §14.1, "Deliberate divergences", for the byte-exact pair and the fixture obligation
  this permanent Side A ≠ Side B mismatch carries.**
- **Unrecognized keyword value** for a property whose value domain (§6.1, either the sole domain or
  a two-domain row's own keyword alternative) is `Keyword` -- a syntactically well-formed identifier
  token that is not a member of *that specific property's own* accepted set (`display: blocc;`, one
  character off `block`, is the reported example): **added, `UIX-RCSS-ERRATA-5`, 2026-08-06, closing
  a gap the original five cases above never named.** The declaration is dropped in full -- the same
  consequence already stated for an unknown property name, the first bullet above -- **not** a
  variant of the malformed-shorthand partial-write case two bullets above, because it is a different
  upstream code path entirely, with no in-loop mutation to guard against:
  `PropertyParserKeyword::ParseValue`
  (`examples/RmlUi/Source/Core/PropertyParserKeyword.cpp:9-19`) returns `false` on a
  lookup miss against its own `ParameterMap`; `PropertyDefinition::ParseValue`
  (`PropertyDefinition.cpp:75-89`) loops every parser registered for that property (the keyword
  parser, plus, for a two-domain row, the length-percent/length/string parser) and only returns
  `false` once **none** of them accept the token; the single-property caller,
  `PropertySpecification::ParsePropertyDeclaration` (`PropertySpecification.cpp:255-282` -- the
  *non*-shorthand path, contrast `ParseShorthandDeclaration`'s own loop the bullet above describes),
  calls `dictionary.SetProperty` **only after** a successful `ParseValue` (`:277-280`) -- on failure,
  `SetProperty` is never reached, so there is no partial dictionary mutation to reason about here.
  The property computes to its inherited value (if `inherited: true` and an ancestor provides one) or
  its §6.1 registry initial value -- the same outcome this section's own closing paragraph already
  states for every fail-high case; this is the sixth trigger for it, not a seventh new consequence.
  **Where the accepted set comes from:** §6.1's own registry table is the single source of truth for
  every `Keyword`-domain property's own legal member set, named inline in that table's own "Value
  domain" cell (`keyword(`a`,`b`,...)`) -- the convention already used, before this errata, for the
  18 rows whose keyword alternative was a single word or a short enumerated set; this errata only
  extends that same convention to the 11 rows that had carried the bare word "keyword" with no member
  list (§6.1's own new paragraph above names all 11 and their upstream citation). **This case is
  Side-A/Side-B-symmetric, unlike the bullet above** -- both dumpers reject the same unrecognized
  token the same way, so §14.1's own ledger gets no entry for it.
- **Logging format, minimum content:** the raw text of the rejected construct, and its file/line if
  available -- never a bare "invalid RCSS" with no locatable cause, the same standard
  `check_rml_whitelist.sh`'s own `file:line` reporting sets for this codebase (`docs/rmlx-subset.md`
  section 4).

This dump format has **no special record** for a fail-high event beyond its ordinary consequence:
the property in question simply prints its registry-declared initial value (§6.1) or its inherited
value, because that **is** the correct computed value once the invalid declaration is discarded --
a dumper does not need a distinct "REJECTED" marker line, because the initial/inherited value at
that path already proves, structurally, that nothing else won the cascade for that property.

### 12. Out of this dump / not this wave's job

Restated so a reader of only this file has the complete boundary: **layout-resolved box-relative
percentages, box geometry, real (non-forced) focus/hover/active state driven by actual input
events, keyframe timeline interpolation, and transform-to-render-matrix composition** are all out
of `RMLX-2`'s scope. They belong to `RMLX-3` (layout), `RMLX-5` (events/focus), `RMLX-8`
(animation), and `RMLX-8`/`RMLX-9` (transform application) respectively -- none of them are cascade
facts, and a future dump format for any of them is a new decision, made the same way this one was
(spec first, líder sign-off, then code), never a silent extension of this file, mirroring
`docs/uix-dom.md` section 10's identical clause for its own boundary.

### 13. Explicitly out-of-subset today -- requires the líder's sign-off before any implementation

Per this document's own header clause and `docs/rmlx-subset.md`'s: **stop, edit this spec with a
diff, get the líder's sign-off, only then implement.** Named here so a future implementer does not
have to rediscover each one fixture by fixture:

- **Color:** every RmlUi named color beyond `transparent`/`white` (17 more entries,
  `PropertyParserColour.cpp:117-136`), and every functional color form (`rgb()`, `rgba()`, `hsl()`,
  `lab()`, `lch()`, `oklab()`, `oklch()`) -- zero-measured, and the líder's units-parity decision
  named units, not color syntax, so this was never implicitly authorized.
- **`transition`, `font-effect`:** both are real, registered RmlUi properties
  (`StyleSheetSpecification.cpp:399`, `:405`) but **zero-measured** in the census -- excluded from
  §6.1's registry on the same "real zero is a real cut" basis `docs/rmlx-subset.md` section 2 uses
  for `nth-child`/`:not`/`z-index`, **not** silently folded in because they happen to share
  machinery with `animation`/`filter`.
- **`z-index`, `nth-child`, `:not(`, attribute selectors (`[x]`), sibling combinators (`+`/`~`),
  universal selector (`*`):** already named as real-zero cuts by `docs/rmlx-subset.md` itself;
  restated here so this document alone (per its own header promise) states the complete selector
  boundary without requiring a reader to also have that document open.
- **3D/matrix `transform` functions** (`translate3d`, `matrix`, `perspective`, ...): §9.4's own
  stated limit -- the 2D subset is verified against the corpus's 2 instances, nothing beyond it is.
- **`radial-gradient`'s `ellipse` shape:** only `circle` is in scope per `docs/effects.md`'s own
  already-shipped grammar; an `ellipse` fixture would need this document's own sign-off before a
  dumper may accept it, not merely a code change to `docs/effects.md`.
- **Unmeasured RmlUi native properties not in §6.1** (a non-exhaustive sample, named because they
  are the ones a reader familiar with full upstream RmlUi might expect and reasonably assume are
  included): `font-style`, `font-weight` (measured **zero** in the census despite `font-family`/
  `font-size` being heavily used -- flagged explicitly since their absence is easy to miss),
  `caret-color`, `image-color`, `visibility`, `clip`, `float`, `clear`, `drag`, `nav-*`,
  `pointer-events`, `perspective*`, `transform-origin*`, `-rmlui-language`, `-rmlui-direction`,
  `word-break`, `text-decoration`, `font-kerning`, `scrollbar-margin`, `overscroll-behavior`.

### 14. Divergence ledger

**Same three-class scheme as `docs/uix-dom.md` section 9, same escalation threshold (~10 class-(b)
rows means re-read sections 1-13, not keep patching), same split between "routine to append" (any
implementer, the moment a real divergence is found) and "requires líder sign-off" (only a change to
sections 1-13 above, the format contract itself).** Restated briefly rather than copied in full --
`docs/uix-dom.md` section 9 is the canonical description of the three classes (a: our bug, fix the
dumper; b: expected RmlUi/glintfx normalization, log it regardless of corpus coverage; c: out-of-subset
construct, STOP) and this ledger inherits that text by reference, not by duplication, to avoid the
two documents drifting apart on what "class b" means.

| Date | Class | Description | Fixture | Resolution |
| :--- | :---: | :--- | :--- | :--- |
| 2026-08-07 | b | Every `body`/root element prints `position=absolute` on side A regardless of any RCSS -- upstream RmlUi's `ElementDocument` constructor (`examples/RmlUi/Source/Core/ElementDocument.cpp:137`, `SetProperty(PropertyId::Position, Property(Style::Position::Absolute))`) force-sets this as an INSTANCE property, bypassing the cascade entirely (it is not achievable through any stylesheet, UA or author). Side B has no equivalent special-case for the DOM root, so it always prints the registry's own `static` initial. Measured via `rcss_dump_differential_oracle` (`UIX-RCSS-ORACULO`) on ALL 33 in-scope corpus fixtures -- 100% reproduction rate, `STATE none` and `STATE hover-all` alike. | Reproduces on literally every fixture this oracle compares (e.g. `glintfx/tests/min.rml`, `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml`) | Closed 2026-08-07 -- filed as a §14.1 deliberate divergence (row 1 below): Side B keeps the registry's own `static` initial for the tree root; RmlUi's own forced `absolute` instance-property is NOT replicated (a UA-stylesheet special-case would be incorrect, not just ugly -- an author rule can override a UA rule but never RmlUi's own instance property, so the bytes would diverge again the moment a fixture sets `position` on the root; real CSS agrees with Side B, the root is `static` by default and the initial containing block is a layout-time decision, not a computed-value one; verified, not assumed, that nothing in `glintfx/src/uix/` consumes this today -- no layout module exists yet, `RMLX-3`). Decision, autonomous mode: Caetano/CTO 2026-08-07, pending the líder's retroactive confirmation (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`). |
| 2026-08-07 | a | An RCSS `/* ... */` comment preceded by ANY whitespace (i.e. on its own line, the overwhelmingly common real-world shape) is not recognised as a comment inside a declaration block and gets merged into the NEXT real declaration's own name run, corrupting or fully dropping that declaration. Root cause, traced to two exact lines: `glintfx/src/uix/style/lexer.cpp`'s `try_scan_comment()` (line 78-79) requires `starts_with_at(source_, pos_, "/*")` to hold with ZERO tolerance for leading whitespace, and `scan_declaration()`'s own loop (line 199-238) calls `try_scan_comment()` at the top of every iteration WITHOUT first skipping whitespace -- so a comment glued directly after the previous `;`/`{` (no intervening bytes) IS recognised, but the same comment on its own indented line (any real author's actual style) is not, and its bytes plus everything up to the following `:` become one malformed "property name". Measured directly: `parse_stylesheet()` on `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml`'s own real RCSS produces 12 `ParseDiagnostic`s of the exact form `unknown property name '/* <comment text> */\n  <next-real-property-name>'`, and the corresponding real declaration (`width`, `box-shadow`, `border`, ...) is silently absent from that `Rule`'s own `declarations`, which side B's dumper then reports at its own registry initial value -- explaining most of this fixture's own 102 divergent `PROP` lines against side A (which has no such gap; RmlUi's real lexer strips comments independent of surrounding whitespace). | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` (12 corpus-real reproductions; also present in most other `RMLX-1` corpus fixtures that use inline RCSS comments, per this document's own corpus census) | Closed by commit `0d5f475` (`UIX-LEXER-COMENT-ESPACO`, 2026-08-07) -- a new, private `Lexer::try_scan_comment_at_fresh_start()` peeks past leading whitespace only to look for a comment; if found, the whitespace is never materialised into any token; if not found, `pos_` rolls back so the caller's own subsequent scan starts at exactly the same byte as before -- zero behaviour change for every other token shape. Measured before/after on this same oracle: `gusworld_battle_cockpit.rml` 102 -> 12 diverging lines; the 33-fixture total 414 -> 232 lines (-182, -44%). The 12 lines still remaining on that fixture are the three OTHER known classes (root `position`, `animation`, `image()` missing `cover`), zero comment-related. |
| 2026-08-07 | b | The `polygon()` decorator's own `fill:` argument accepts a nested `radial-gradient(...)`/`linear-gradient(...)` composite value (a glintfx-authored decorator, not part of upstream RmlUi at all) -- side A's own dumper pipeline (`glintfx/src/rml/rcss_dump.cpp:670-711`) drives RmlUi's real property-parsing machinery, which has no grammar for a function call nested as a sub-value inside another custom decorator's own argument list; the whole `decorator:` declaration is REJECTED at parse time with RmlUi's own fail-high warning `decorator entry type 'polygon' dropped (fail-high, section 11)`, and side A's dumper then prints the registry's own `none` initial for that property. Side B's own clean-room parser carries no such limitation -- it recognises the nested gradient function and prints the full composite value. Measured via `rcss_dump_differential_oracle` (`UIX-RCSS-ORACULO`) on the 12 in-scope corpus fixtures that carry this exact declaration (`polygon(6, radial-gradient(...))`, byte-identical copy across all 12) -- 100% reproduction rate on every one, `STATE none` and `STATE hover-all` alike, 96 diverging lines total (8 per fixture). Byte-exact example (`body/1/0`): side A `decorator=none`, side B `decorator=polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%);0.0000)`. Reported in the commit that shipped this oracle (`354e81d`, achado 3) but never persisted anywhere in this document until now -- closing that gap is this fatia's (`UIX-ORACLE-CANON`) own obligation. | `difficulty_menu__lista_hardcore_bloqueado.rml`, `difficulty_menu__splash_confirmacao.rml`, `save_load_menu__modo_carregar_dois_slots_ocupados.rml`, `save_load_menu__modo_salvar_com_autosave.rml`, `save_load_menu__modo_salvar_todos_vazios.rml`, `system_menu__config_audio_sliders.rml`, `system_menu__config_categorias.rml`, `system_menu__config_controles_tabela.rml`, `system_menu__confirmacao_menu_inicial.rml`, `system_menu__pause_raiz.rml`, `title_menu__com_save.rml`, `title_menu__sem_save.rml` | Closed 2026-08-07 -- filed as a §14.1 deliberate divergence (row 3 below): the gap is side A's own, permanent as long as side A wraps the RmlUi dependency this repo is retiring, self-documented by its own fail-high log line; side B stays the complete side, not downgraded to match. Decision, autonomous mode: Caetano/CTO 2026-08-07, pending the líder's retroactive confirmation (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`). |

#### 14.1 🔵 Deliberate divergences (Side A ≠ Side B, permanent, by líder decision -- added `UIX-RCSS-ERRATA-5`, 2026-08-06)

**What this section is, and why it is not the ledger above.** Section 14's own three classes (a/b/c)
all share one assumption: Side A and Side B are *expected to eventually agree* on every fixture --
class (a) is fixed by patching the dumper that is wrong, class (b) is fixed by teaching both sides the
same real-upstream normalization, class (c) stops implementation until scope is settled, precisely
because agreement is still the goal once the líder signs off. **This section tracks the opposite
kind of fact: a byte-level mismatch the líder has decided to KEEP, permanently, for a named input --
Side B is not wrong, Side A is not wrong, they are correctly implementing two different rules on
purpose.** Filing an entry like this into section 14's own ledger would corrupt that ledger's own
escalation signal (the "~10 class-(b) rows means something is systemically wrong" threshold, §14
above) -- a permanent, intended mismatch is evidence of nothing going wrong, and must not count
toward a signal that exists to detect exactly that.

**Deliberate divergences: 4.** (Count kept at the top of this section, in both languages, precisely
so growth here is visible and uncomfortable -- see the next paragraph for why that discomfort is the
point.)

**⚠️ Why this list must stay short, and why every row is dangerous by construction.** Until today,
this document's own working rule was simple: any Side A/Side B byte difference is a defect,
somewhere, full stop. The líder's own decision on `border-top` (§6.2, §11, §15.2) opens the first
exception to that rule -- and an exception list that grows without discipline **eats the oracle's own
value whole**: every future implementer who hits a real, undecided divergence now has a tempting
third option beyond "fix Side A" or "fix Side B" -- "add a row here and call it deliberate" -- and if
that door opens without friction, the oracle stops meaning anything. The four requirements below are
the friction, and all four are mandatory, not aspirational:

1. **Single source, read not embedded.** This table is the *only* place a Side A/Side B exception is
   declared, for the entire `RMLX-2` oracle. **Normative requirement for the future
   `UIX-RCSS-ORACULO` fatia (not yet a `TODO.md` table item as of this errata):** the differential
   harness that diffs Side A's dump against Side B's dump MUST, for every byte-level mismatch at a
   given `(node path, property, STATE)` triple, look that triple up against this table's own rows
   *before* reporting a failure -- a mismatch matching a row here is expected and MUST NOT fail the
   harness; a mismatch matching no row here MUST fail the harness. **The harness owns zero exception
   logic of its own** -- no `if` statement inside the harness naming `border-top` or any other
   property, no hardcoded skip-list, no comment saying "known difference, ignore". An exception
   embedded in the harness's own code is indistinguishable from a bug the harness happens not to
   catch; an exception that requires editing *this* document (with the same "stop, diff, líder
   sign-off" discipline every other contract change in this document already carries, per this
   document's own header) is auditable the same way every other rule here is.
2. **Four required fields per row**, no row may omit any of them: **(a)** the exact case that
   diverges (source RCSS + which node/state, precise enough that a second implementer can reconstruct
   the exact fixture without guessing); **(b)** the exact byte Side A prints; **(c)** the exact byte
   Side B prints; **(d)** why the two sides deliberately disagree, **dated and attributed to the
   líder by name of decision date** -- never "the tech-lead decided" for a Side A/Side B behavioral
   split, because that is a call this document's own header reserves for the líder, the same
   authority line every other design judgment call in this document (the premultiply flag in
   `ERRATA-2`, its own reversal in `ERRATA-4`) already follows.
3. **🔴 Every exception MUST be exercised by at least one fixture, and the oracle MUST fail if any
   exception is never exercised.** This is not paperwork. `TODO.md`'s own `SEED-GOLDEN-INERTE`
   finding, the same day this errata was written, measured **three** golden fixtures in this same
   `RMLX-2` wave that did not reach the condition they claimed to cover -- a golden that never
   exercises what it says it exercises is worse than no golden at all, because green looks like proof
   and is not. An unexercised *exception* is the identical failure mode one level up: it would let a
   real regression hide behind a row that no longer describes any code path anyone runs. Concretely,
   for the oracle harness's own test suite (`UIX-RCSS-ORACULO`, when that fatia is opened): (i) every
   row in this table needs a named fixture, in the Fixture column below, that feeds the row's own
   exact case through both dumpers; (ii) the harness's own meta-test suite needs a mutation-testing
   pass per row -- neutralize this table's own lookup for that one row (so the harness would, for that
   row alone, fall back to plain byte comparison) and assert the fixture's own test **goes red** --
   proving the fixture actually lands on the code path this row excuses, not merely that the fixture
   exists; a row whose neutralization does not turn its own fixture red is a build error for that
   fatia, not a silent pass, the identical mutation-testing discipline this repo's own house rule
   already applies to every other guard (`feedback_mutante_em_arquivo_nao_commitado`,
   `SEED-GOLDEN-INERTE`, both `TODO.md`).
4. **This section's own count line, above, is not decorative** -- it is the visible cost of adding a
   row. A future implementer proposing a second entry should feel the same friction section 14's own
   escalation-threshold paragraph describes for class-(b) rows: if this table ever approaches even a
   handful of entries, that is a signal the líder-decision process itself, not this document's
   format, needs re-examining -- a long list of "the two engines just disagree here, permanently" is
   not a healthy end state for an oracle whose entire purpose is proving the two engines agree.

**Ledger table.** Same column discipline as section 14's own table above (Fixture is a path relative
to `glintfx/tests/`, or `none yet -- <how/when a fixture will exist>` until the fatia that owns it
ships), plus the two byte columns unique to this table.

| Date | Case | Side A byte | Side B byte | Why (líder decision) | Fixture |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 2026-08-06 | `#b { border-top: #7A5A2E 1dp; }` (reversed-order `border-top` shorthand, `FallThrough`, §6.2) -- `body/1 PROP border-top-color`, `STATE none` | `border-top-color=#7a5a2eff` (upstream's own in-loop, no-rollback partial `SetProperty`, §6.2's second correction) | `border-top-color=#000000ff` (Side B's own atomic-discard: the whole declaration is dropped, both longhands fall back to the §6.1 registry initial, `black`, straight-alpha default `ff` per §7.1) | Decided 2026-08-06, líder: Side B's atomic-discard is judged the *correct* behaviour and stays; upstream's own partial write is a no-rollback dictionary-mutation artifact, not an intentional CSS semantic worth bug-for-bug replication in a clean-room engine. Origin: `UIX-SHORTHAND-PARCIAL`/`UIX-RCSS-DUMP-B` (`TODO.md`). `border-top-width` is **not** part of this exception -- both sides print `border-top-width=0.0000px` for `body/1`, in agreement, for unrelated reasons on each side (§15.2). | **Per-side fixtures exist and exercise this exact row (`UIX-RCSS-CONFORMIDADE`, 2026-08-06)** -- `glintfx/src/rml/rcss_dump_worked_examples.cpp::test_15_2_shorthand_order_border_top` (Side A, real RmlUi pipeline, asserts `#7a5a2eff`) and `glintfx/tests/uix_style/dumper_sanity.cpp::test_worked_example_15_2_shorthand_order` (Side B, real lexer→parser→cascade pipeline, asserts `#000000ff`) each independently reach and print this row's own byte. **Still missing:** the *differential* harness itself (`UIX-RCSS-ORACULO`, not yet a `TODO.md` item) that would look this row up and suppress the mismatch automatically -- today the two fixtures above prove each side's own byte in isolation, not that a diff-and-consult-§14.1 harness exists yet |
| 2026-08-07 | Any `body`/root element of any in-scope corpus fixture -- `position`, `STATE none` and `STATE hover-all` alike (upstream's own `ElementDocument` constructor, `examples/RmlUi/Source/Core/ElementDocument.cpp:137`, forces this as an INSTANCE property outside the cascade -- see §14's own class-(b) row above for the full mechanism) | `position=absolute` | `position=static` | Decided 2026-08-07, autonomous mode (Caetano/CTO), pending the líder's retroactive confirmation (`PLANO-RMLX2-FECHO`, `TODO.md`) -- three reasons, in order of force, all measured not assumed: **(1)** replicating via a UA-stylesheet rule would be INCORRECT, not just ugly -- a UA rule is overridable by an author rule, RmlUi's own instance property is not, so a future fixture with `body { position: relative; }` would still diverge (side A keeps printing `absolute` regardless, a UA-ruled side B would print `relative`); the only byte-faithful replication would be an instance special-case for the tree root baked into the clean-room cascade, carrying an internal quirk of the very dependency this repo is retiring. **(2)** Real CSS agrees with side B -- the root is `static` by default and the initial containing block comes from the viewport, a LAYOUT decision, not a computed-value one; when `RMLX-3` builds the engine's own layout, the root fills the viewport by the layout engine's own rule, no cascade lie required. **(3)** Verified, not assumed: `glintfx/src/uix/` has no layout module today (`RMLX-3` not shipped), and a grep over `src/uix/**` for `position` hits only registry/shorthand/gradient-stop code -- zero layout consumer exists that this decision could regress. | Corpus-wide -- reproduces on all 33 in-scope fixtures (same list as §14's own row above); canonical byte-exact instance: `glintfx/tests/min.rml`, `body` line |
| 2026-08-07 | `decorator: polygon(6, fill: radial-gradient(...))` -- a nested gradient composite as the `polygon()` decorator's own fill argument, any node/state declaring the exact corpus copy (see §14's own row above for the full mechanism) | `decorator=none` | `decorator=polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%);0.0000)` | Decided 2026-08-07, autonomous mode (Caetano/CTO), pending the líder's retroactive confirmation (`PLANO-RMLX2-FECHO`, `TODO.md`) -- the gap is side A's OWN, permanent for as long as side A wraps a dependency this repo is retiring, and self-documented (its own fail-high log line names it every single time it fires). Closing it on side A would mean building a second composite-value parser inside a dumper wrapping a library on its way out -- work against this repo's own `R1` exit strategy. Downgrading side B to match would fake the oracle in the opposite direction. Excluding these 12 fixtures from the oracle's own scope was considered and rejected -- it would hide the hundreds of OTHER `PROP` lines those same fixtures carry. | `difficulty_menu__lista_hardcore_bloqueado.rml`, `difficulty_menu__splash_confirmacao.rml`, `save_load_menu__modo_carregar_dois_slots_ocupados.rml`, `save_load_menu__modo_salvar_com_autosave.rml`, `save_load_menu__modo_salvar_todos_vazios.rml`, `system_menu__config_audio_sliders.rml`, `system_menu__config_categorias.rml`, `system_menu__config_controles_tabela.rml`, `system_menu__confirmacao_menu_inicial.rml`, `system_menu__pause_raiz.rml`, `title_menu__com_save.rml`, `title_menu__sem_save.rml` |
| 2026-08-07 | `decorator: image(<url> cover)` -- the `cover` fit keyword in an `image()` decorator's own printed composite form, any node/state carrying it (5 unique nodes across 4 fixtures, measured `UIX-ORACLE-REMEDICAO`/`relatorio-B2.md`) | `decorator=image(retrato_seu_bertoldo_caim.png)` (side A's own `image()` serializer drops the `cover` fit keyword from its printed form; example node `body/1/4`) | `decorator=image(retrato_seu_bertoldo_caim.png cover)` (side B keeps the raw parsed `cover` token in its own printed form) | Decided 2026-08-07, autonomous mode (Caetano/CTO), pending the líder's retroactive confirmation (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`) -- pre-existed on `gusworld_battle_cockpit.rml` alone (2 nodes) as a MASKED residue, and was UNMASKED, not introduced, when `UIX-INLINE-STYLE` (`72f832c`) taught the cascade to read the `style="..."` attribute for the first time, exposing the same pre-existing gap on 3 more `npc_dialogue__*` fixtures that had been carrying it silently as `decorator=none` (the declaration was discarded before it ever reached the point where `cover` mattered; `UIX-ORACLE-REMEDICAO`, `TODO.md`). Filed here as PERMANENT for the oracle's own purposes only because no wave currently owns the fix -- `UIX-IMAGE-COVER-AUSENTE` is an un-pulled INBOX seed, not a scheduled wave, so it cannot take a §14.2 "scheduled gap" row (which requires an owning wave). Should a future fatia pull `UIX-IMAGE-COVER-AUSENTE` and make side B match side A's own truncation (or vice-versa), this row is removed in the SAME commit that closes it, per this section's own discipline. | `gusworld_battle_cockpit.rml` (2 nodes, `body/0/0/0/3/0` and `body/0/1/0/0/0`, pre-existing), `npc_dialogue__no_com_3_escolhas.rml` (`body/1/4`), `npc_dialogue__no_linear_fala_curta.rml` (`body/1/4`), `npc_dialogue__no_linear_fala_longa.rml` (`body/1/4`) |

#### 14.2 🟣 Scheduled gaps: pinned until an owning wave ships (added `UIX-ORACLE-CANON`, 2026-08-07)

**What this section is, and why it is not §14.1.** §14.1 above is, by its own design, ONLY for
permanent, deliberate divergence -- a byte mismatch nobody intends to close. This section tracks the
opposite kind of incompleteness: side B's own dumper is missing a computation it WILL eventually
have, because the property it would compute is explicitly out of THIS wave's own scope (`RMLX-2`),
not because either side is behaving correctly on purpose. Filing a temporary gap into §14.1's own
ledger would poison both: a §14.1 entry that later needs REMOVING (once the owning wave ships)
contradicts §14.1's own "permanent, by líder decision" contract, and letting §14.1 carry temporary
rows would blunt the discomfort §14.1's own count line is built to cause (this document's own §14.1,
requirement 4). **Expiration here is AUTOMATIC, not a chore for a human to remember:** the oracle's
own stale-pin check (already built for §14.1) applies identically to this table -- the day the
owning wave ships the missing computation, side B starts printing the real value, the pinned byte
here stops matching, and the oracle FAILS HIGH demanding the row be removed in the SAME commit. No
implementer needs to remember to clean this table up; the mechanism forces it.

**Scheduled gaps: 3.** (Count kept at the top of this section, in both languages, same discipline as
§14.1's own count line above.)

**Table.** Same four required fields §14.1's own requirement 2 lists (case, side A byte, side B
byte, why), with the "why" column's own fourth sub-field replaced: instead of §14.1's own
"attributed to the líder by name" clause, every row here names the fence already drawn by
`docs/rmlx-subset.md` and the owning wave/item that debits this gap -- a scheduled gap is not a
design judgment call, so the "why" here is a fence-plus-owner, not a líder decision date.

| Date | Case | Side A byte | Side B byte | Why (fence + owning wave) | Fixture |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 2026-08-07 | `body/0/0/0/1` `animation` declaration, `STATE none` (`L369`) and `STATE hover-all` (`L4385`) alike | `animation=animation(linear;18.0000;linear-out;infinite;false;false)` | `animation=none` | Side B's own dumper has no `compute_animation` (`glintfx/src/uix/dom/dumper.cpp` lines 49-55/121-128 -- the declaration falls back to the registry's own `none` initial); `docs/rmlx-subset.md` freezes `animation`/`@keyframes` scope at `RMLX-8`, and implementing it now would breach that fence. Not a defect, not permanent -- SCHEDULED incompleteness, owned by `RMLX-8` (`TODO.md`). | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |
| 2026-08-07 | `body/0/0/3/0` `animation` declaration, `STATE none` (`L880`) and `STATE hover-all` (`L4896`) alike | `animation=animation(step-start;1.1000;linear-out;infinite;false;false)` | `animation=none` | Same as the row above -- `compute_animation` not implemented, frozen by `docs/rmlx-subset.md` at `RMLX-8`. | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |
| 2026-08-07 | `body/0/1/0/0` `animation` declaration, `STATE none` (`L1099`) and `STATE hover-all` (`L5115`) alike | `animation=animation(infinite-alternate;2.4000;cubic-in-out;1;false;false)` | `animation=none` | Same as the row above -- `compute_animation` not implemented, frozen by `docs/rmlx-subset.md` at `RMLX-8`. | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |

3 unique nodes, 2 `STATE`s each = 6 diverging lines, all on the single fixture in this corpus that
uses `animation` -- measured (`UIX-ORACLE-MEDICAO`/`UIX-ORACLE-REMEDICAO`, `TODO.md`), not assumed;
an earlier planning draft (`PLANO-RMLX2-FECHO`, `/var/tmp/plano-rmlx2-fecho/PLANO.md`) named 4
nodes/8 lines before the measurement existed -- the measurement corrected it to 3/6 and this table
follows the measurement, not the draft.

⚠️ **`RMLX-8`'s own `TODO.md` entry already carries the removal instruction: remove these 3 rows in
the SAME commit that implements `compute_animation`, and lower this section's own count line in the
same commit -- the stale-pin check will refuse to pass otherwise.**

### 15. Worked examples (byte-exact)

**Four independent examples below (15.1-15.4), each anchoring a place this document's own prose
alone left room for two readers to land on different bytes -- per this section's own governing
principle, restated from the header: two independent implementers can agree on the same
wrong-sounding-correct reading of a rule in prose; they cannot both reproduce the same byte-exact
worked answer while disagreeing about what it means.**

#### 15.1 Two states, one node (`:hover`)

Source fragment (`.btn` styled, `.btn:hover` overrides `color`; `dp_ratio = 1.0` for this example;
only the fields relevant to the point are shown -- a real dump still emits all 72 `PROP` lines per
node per §3, elided here with `...` where a line is identical to its own registry initial value and
does not illustrate anything new):

```rcss
.btn {
    display: block;
    width: 50%;
    color: #223344;
    opacity: 0.5;
}
.btn:hover {
    color: #ff0000;
}
```

```rml
<body><div id="root"><button class="btn">Go</button></div></body>
```

Relevant lines from the full dump (node path `body/0/0` is the `<button>`; `...` elides the
remaining ~68 unaffected `PROP` lines each state block still emits in full per §3):

```
STATE none
...
body/0/0 PROPS 72
body/0/0 PROP color=#223344ff
...
body/0/0 PROP display=block
...
body/0/0 PROP opacity=0.5000
...
body/0/0 PROP width=50.0000%
...
STATE hover-all
...
body/0/0 PROPS 72
body/0/0 PROP color=#ff0000ff
...
body/0/0 PROP display=block
...
body/0/0 PROP opacity=0.5000
...
body/0/0 PROP width=50.0000%
...
```

Notes tying each line back to the sections above:

- `color` is the **only** line that differs between the two `STATE` blocks -- exactly the point of
  the `hover-all` state (§4): every property `.btn:hover` does not touch computes identically in
  both states, which is itself the useful signal a diff surfaces (a divergence appearing on `width`
  or `opacity` between `none` and `hover-all` for this node would point at a matcher bug that leaks
  `:hover`'s specificity/selection into unrelated properties, not at `color`'s own rule).
- `width=50.0000%`, unresolved, in **both** states (§1, §5 family a) -- `:hover` does not change
  which family a percentage belongs to, so this line is identical in both blocks for a reason
  unrelated to `:hover` at all; it stays symbolic because `RMLX-2` never resolves family-(a)
  percentages, regardless of pseudo-class state.
- `opacity=0.5000`, quantized to 4 decimal digits per §8 even though `0.5` needed none of that
  precision -- the dump always prints the full 4-digit form, never a shorter one when trailing
  zeros would do, because a variable-length printed form is one more place for two independent
  printers to diverge on formatting rather than on the value itself.
- `color=#223344ff` / `#ff0000ff`: both straight-alpha, alpha defaulted to `ff` per §7.1 since
  neither source value specified one.

#### 15.2 Shorthand order is load-bearing (`border-top`), and Side A/Side B permanently disagree on `#b`

This is the errata's own worked anchor -- section 6.2's corrected table row, byte-exact. `body/0`
(the canonical, width-then-color order) is identical on both sides and shown once. `body/1` (the
reversed, color-then-width order) is **not** identical on both sides as of `UIX-RCSS-ERRATA-5`,
2026-08-06 -- shown as two separate blocks, Side A then Side B, this worked example's own
`§14.1`-tracked deliberate divergence. `dp_ratio = 1.0` throughout.

```rcss
#a { border-top: 1dp #7A5A2E; }
#b { border-top: #7A5A2E 1dp; }
```

```rml
<body><div id="a"></div><div id="b"></div></body>
```

`body/0` (`#a`, width-then-color, the corpus's own 100%-measured real order) -- both longhands set
from the declaration, identical on both sides:

```
body/0 PROP border-top-color=#7a5a2eff
body/0 PROP border-top-width=1.0000px
```

`border-top-width=1.0000px` (`1dp` resolved through `dp_ratio=1.0`, `px` suffix per §8.1),
`border-top-color=#7a5a2eff` (§7.1, alpha defaulted to `ff`).

`body/1` (`#b`, color-then-width, the reversed order) -- **Side A** (real upstream RmlUi,
`UIX-RCSS-ERRATA-2`'s own `Finding A`, byte-by-byte trace in §6.2's second correction): only
`border-top-width` reverts, `border-top-color` does not.

```
body/1 PROP border-top-color=#7a5a2eff    # Side A only
body/1 PROP border-top-width=0.0000px
```

`ParseShorthandDeclaration` still returns `false` and the declaration is still logged as rejected
per §11's corrected malformed-shorthand bullet -- but upstream's own loop already called
`dictionary.SetProperty(BorderTopColor, ...)` from the source token `"#7A5A2E"` (item 1, `-color`,
matched it) **before** the post-loop guard ever fires; item 0 (`-width`) never matched anything in
this call. Printed result: `border-top-color=#7a5a2eff` (**set from the source token, identical to
`body/0`'s own value** -- not reverted), `border-top-width=0.0000px` (`0px`, quantized, the §6.1
registry initial -- this one **did** revert). This is the line a naive "whole declaration reverts"
reading gets wrong **for Side A**: it is not that `#b`'s two longhands print identically to
`body/0`'s *or* both revert to black/`0px` -- one of each: the matched longhand keeps the source
value, only the never-matched one falls back.

`body/1` -- **Side B** (`UIX-RCSS-ERRATA-5`, 2026-08-06, the líder's own decision, §14.1's own first
ledger entry): the **entire** declaration is discarded atomically, no partial match survives, both
longhands fall back to their own §6.1 registry initial:

```
body/1 PROP border-top-color=#000000ff    # Side B only
body/1 PROP border-top-width=0.0000px
```

`border-top-color=#000000ff` (`black`, the §6.1 registry initial, straight-alpha default `ff` per
§7.1 -- **not** `#7a5a2eff`), `border-top-width=0.0000px` (same initial value as Side A prints for
this longhand, by coincidence of both longhands sharing the same fallback here, not because the two
sides agree on the mechanism). **This byte pair -- Side A's `border-top-color=#7a5a2eff` against
Side B's `border-top-color=#000000ff`, both for the identical `#b` input -- is a real, permanent,
by-design oracle mismatch, not a bug in either dumper.** An oracle harness comparing the two blindly
byte-for-byte on this fixture reports a false failure unless it consults §14.1 first. An earlier
version of this worked example (before `UIX-RCSS-ERRATA-2`) published `border-top-color=#000000ff`
for a single, shared `body/1` line covering both sides -- that was wrong *for what was then a
shared answer*; it is coincidentally the correct Side B byte today, for an unrelated reason (Side B
no longer claims to match Side A on this input at all).

#### 15.3 The three `%` families, side by side

One node, three properties, `dp_ratio = 1.0` -- chosen specifically so families (b) and (c) appear
inside sibling arguments of gradient functions, closing §5's own reported merge risk with a byte
anchor rather than only the prose decision above.

```rcss
#c {
    width: 50%;
    decorator: linear-gradient(90deg, #FF0000 20%, #00FF00 80%), radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%);
}
```

```rml
<body><div id="c"></div></body>
```

```
body/0 PROP decorator=linear-gradient(90.0000;#ff0000ff:20.0000%;#00ff00ff:80.0000%)|radial-gradient(35.0000%;30.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%)
body/0 PROP width=50.0000%
```

- **Family (a)**, `width=50.0000%` -- box-relative, stays symbolic, unrelated to either gradient
  function; the only one of the three families that is not even inside a `decorator` value.
- **Family (b)**, the two `<position%>` tokens after `:` inside each `linear-gradient(...)` stop
  (`20.0000%`, `80.0000%`) -- each resolves against **that gradient's own 1D axis** (a length along
  the 90-degree line the angle argument describes), never against the element's box, never against
  the radial gradient's own center. Note the angle argument itself, `90.0000` -- bare, no `deg`
  suffix, per §8.2's newly-closed print-form rule.
- **Family (c)**, the two `<x%> <y%>` arguments right after `radial-gradient(`'s own opening
  (`35.0000%`, `30.0000%`) -- resolve against **that gradient's own 2D local coordinate space** (an
  `(x, y)` pair, the inscribed circle's own center), never against the 1D axis family (b) uses one
  function over. Syntactically both families are a bare `<number>%`; the fact that closes the
  ambiguity is **argument position**: family (c) is always the first 1-2 arguments immediately after
  `radial-gradient(`'s own `circle at` clause, consumed before any stop is read at all; family (b) is
  always inside a `<stop>` entry, after that stop's own color and its own `:` separator. The two
  never occupy the same slot in the same function call, in either gradient kind.
- The radial-gradient's own first stop, `#F0D98C`, has no explicit position and is first, so
  §9.2.1's auto-spacing algorithm assigns it `0.0000%` -- the one number on this line that is not
  copied from the source text, same as the section 9.2.1 worked example this one reuses.

#### 15.4 Quantization boundary: exact tie and one step outside

**Corrected (`UIX-RCSS-ERRATA-3`, 2026-08-06): the original four literals below never reached the
boundary they exist to pin -- see the errata block at the top of this document for the full finding,
why it stayed inert through two prior errata passes, and the consequence for `UIX-RCSS-DUMP-A`'s own
test suite. Short version, arithmetic inline in the table below:** `1.234450f`/`1.234449f` (and their
negations) widen to `1.2344499826431274`/`1.234449028968811` in `double` -- neither is an exact `N.5`
once scaled by `10000`, so no row of the original table actually reached a tie. The replacement pair,
`1.21875f`/`1.21874f`, is chosen because `1.21875 = 39/32` has a power-of-two reduced denominator
(`2^5`) and is therefore *exactly* representable in `float32` -- the structural condition a decimal
literal must meet to land on a real binary tie at all (§8's own `quantize()` scales by `10000 = 2^4 *
5^4`; a tie needs `x`'s own fraction to already be a power of two, or the multiplication cannot
produce an exact `.5`).

Per this project's own house rule that testing a boundary's exact edge is not sufficient on its own
(a widened tolerance still contains its own edge) -- these four abstract inputs to `quantize()` (§8)
pin the exact rounding cut point, not merely that *some* rounding happens near it. All four apply
`quantize()` directly (no property, no cascade -- this is the algorithm itself, in isolation):

| Input `x` | `x` widened `float32`→`double` | `scaled = x * 10000` | Tie? | `quantize(x)` | What this proves |
| :--- | :--- | :---: | :---: | :--- | :--- |
| `1.21875` | `1.21875` (exact -- `39/32`, power-of-two denominator, no widening error) | `12187.5` | **exact tie** | `1.2188` | Rounds **away from zero** at the exact half -- not merely "rounds up" |
| `1.21874` | `1.2187399864196777` | `12187.399864196777` | one step below | `1.2187` | Below the tie, rounds **toward zero**, not toward the tie's own outcome |
| `-1.21875` | `-1.21875` (exact) | `-12187.5` | **exact tie**, negative | `-1.2188` | Proves "away from zero" is not a euphemism for "toward positive infinity" -- the negative tie also grows in magnitude |
| `-1.21874` | `-1.2187399864196777` | `-12187.399864196777` | one step below, negative | `-1.2187` | Mirrors the positive case: one step short of the tie stays at the smaller magnitude on both signs |

**Why the tie alone would not have been enough:** an implementer whose rounding only fires for
`scaled` *strictly greater than* the half-integer (a common off-by-one when translating "round half
away from zero" into `>` instead of `>=`) produces the **same** output as the correct algorithm for
every value that is not exactly a tie, and only diverges exactly at rows 1 and 3 above -- both of
which sit precisely at a 4th-decimal-digit boundary a real computed length is unlikely to hit by
accident, but a hostile or adversarial-review-generated input can hit deliberately. Rows 2 and 4
exist for the opposite failure: an implementer whose rounding is unconditional (always rounds the
4th digit up regardless of the 5th digit's own value, a plausible misreading of "round... away from
zero" as "always round away from zero") would wrongly report `1.2188`/`-1.2188` for rows 2/4 instead
of the correct `1.2187`/`-1.2187`. Only having all four -- tie and one-step-outside, both signs --
distinguishes the correct algorithm from both wrong ones.

**The rule this near-miss teaches, for reuse beyond this one table:** when choosing a literal to pin
a floating-point rounding boundary, verify that the value **is** the boundary in the type's own
binary representation, not that it *looks like* the boundary in decimal -- widen the literal to
`double` and compare `scaled` to the nearest half-integer directly (as done in the table above), never
by eyeballing the decimal digits, which is exactly how the original pair passed a decimal read while
failing the binary test it existed to enforce.

### 16. Contract decisions closing ambiguities reported by earlier `RMLX-2` slices

**Scope of this section, stated once:** `UIX-RCSS-LEXER`'s own header comment
(`glintfx/src/uix/style/lexer.hpp`) reported two ambiguities it deliberately did not resolve alone,
each affecting how a future parser slice built on top of that lexer's token stream must behave when
producing the computed values this document's own dump format reports -- so, per this task's own
instruction that an unresolved ambiguity is not this project's to leave open once found, they are
closed here, with both sides argued, as this document's own contract decisions. (The third ambiguity
this task named -- percentage families (b) and (c) -- is already closed in section 5 above; it is
not repeated here, only cross-referenced, so the two documents that state it do not drift.)

#### 16.1 Comments inside a mid-run token: diagnosable `Comment` token vs. upstream's own byte-splice

**The lexer's own choice (already shipped, `UIX-RCSS-LEXER`):** `/* ... */` is its own `Comment`
token, recognised only at a fresh-scan start -- immediately after a structural delimiter, immediately
after a completed `Declaration`, or at the very start of a `Prelude`/`Comment` dispatch. Real upstream
RmlUi elides comments at the *character* level, below every state machine, so a comment can *splice*
two adjacent fragments with zero bytes between them (`wid/*x*/th` tokenizes as the identifier
`"width"`) regardless of where it appears -- mid-identifier, mid-value, anywhere.

**Argument for upstream's byte-splice behaviour:** it is what `Style::ComputedValues` (side A of the
`RMLX-2` oracle) actually does, because side A *is* real RmlUi code -- matching it byte-for-byte
closes a divergence source before it can ever appear. **Argument for the lexer's own, narrower
choice (the decision):** a diagnosable `Comment` token is strictly more useful for every corpus
fixture that exists today (comments only ever appear at fresh-scan-start positions in all 62 census
files -- zero counter-examples), keeps this module's own error-reporting/hardening machinery uniform
with the DOM sibling's identical `<!-- -->` design, and -- the reason this stays the contract instead
of being reopened -- **every `Token`'s own `offset`/`length` fields are preserved exactly so a future
parser slice can still reconstruct upstream's byte-spliced reading directly from the source buffer**,
if a real fixture is ever found where it matters (section 1's own header clause: stop, diff, líder
sign-off, only then implement). **Decision:** kept as shipped. **Consequence for this document's own
divergence ledger (section 14):** a future fixture with a comment appearing *mid-run* (inside an
already-started `Prelude`/name/value scan, not at a fresh-scan boundary) is **pre-registered here as
class (b)** (expected RmlUi/glintfx normalization, not a bug) the moment it is found -- an
implementer who hits it does not need to first debate whether it is class (a) or (b), this document
already answers that.

#### 16.2 At-rule-name-driven mode dispatch: lexer-level vs. deferred to a future parser slice

**The lexer's own choice (already shipped):** the decision of whether a `{` opens a flat Declaration
block or a second, nested Structural region (needed for exactly one case measured in the corpus,
`@keyframes`) is made **inside the lexer itself**, keyed by the case-sensitive first word of the
at-rule's own preceding `Prelude`. Every *other* semantic question this same lexer touches --
selector matching, property-value validation, property registration -- is explicitly deferred to a
future parser slice; `@keyframes` recognition is the one exception, and `UIX-RCSS-LEXER`'s own header
flagged it as a real, structural, two-sided question rather than an obviously-settled one.

**Argument for deferring to a future parser (the position taken everywhere else in this same
lexer):** keeps the lexer a purely mechanical, context-free byte-to-token pass with zero knowledge of
what any at-rule *means* -- the same boundary drawn for `.foo`/`#bar`/`:hover` (carried as raw,
unparsed `Prelude` bytes) and for every property name/value (carried as raw `Declaration` text).
Recognising the literal string `"keyframes"` is, on its face, exactly the kind of semantic knowledge
this module's own header says repeatedly is "not this layer's job". **Argument for keeping it at the
lexer level (the decision):** this is not a semantic-validity question, it is a **syntactic
necessity for producing a correct token stream at all** -- a different category from every deferred
case. Selector/property semantics being deferred costs nothing structural: an unparsed `Prelude`
token is still a complete, correctly-bounded token whether or not anything downstream understands
`:hover`. At-rule-name dispatch is not like that: without knowing the at-rule is `@keyframes`, the
lexer cannot tell a nested `{`/`}` pair apart from a flat one, and gets the **brace balance itself
wrong** -- proven, not asserted, by `lexer_hardening_sanity.cpp`'s own
`test_keyframes_special_case_is_load_bearing`: without the special case, `@keyframes spin { from {
transform: rotate(0deg); } to { transform: rotate(360deg); } }` desyncs, the first inner `}`
prematurely ends the whole scan, and the outer `}` is left dangling -- not a wrong *interpretation* of
otherwise-valid tokens, a **wrong token stream**, which no downstream parser slice could recover a
correct tree from no matter how it is designed. Real upstream RmlUi's own tokenizer needs and makes
this identical decision at the identical layer (`StyleSheetParser::Parse`'s own
`State::AtRuleIdentifier` branch, `:786-789`) -- this is not a boundary this module invented, it is
the one place upstream's own tokenizer-vs-parser split already draws the line the same way.
**Decision:** stays lexer-level, permanently, not merely "for now" -- the teto is already declared
(only the literal, case-sensitive `"keyframes"`; `docs/rmlx-subset.md` section 13's own real-zero
exclusions for `@media`/`@import`/`@charset`/`@supports` get the ordinary Declaration-mode default,
and a real fixture needing one of those is the same "stop, add a name to this table" move, never a
silent default-widening).

---

## Português

### 1. Escopo deste dump: valores computados, não valores usados

Este dump reporta **valores computados** no sentido CSS -- o valor que cada propriedade tem *depois*
da cascata (origem, especificidade, ordem de fonte) e *depois* de resolver toda unidade que não
precisa de geometria de caixa (comprimentos absolutos, `em`/`rem` contra a cadeia de font-size, `dp`
contra `dp_ratio`, unidades de viewport contra o viewport, ângulos, números) -- mas **antes** de
qualquer coisa que precise da `RMLX-3` (layout): uma porcentagem box-relativa não é resolvida a
pixels aqui, porque isso precisaria de um tamanho de containing-block que esta onda não tem. Isto
não é uma simplificação inventada por conveniência; espelha a própria arquitetura do RmlUi real,
evidenciada diretamente: `ComputeLength(NumericValue value, float font_size, float
document_font_size, float dp_ratio, Vector2f vp_dimensions)`
(`examples/RmlUi/Source/Core/ComputeProperty.cpp:52-69`) recebe uma cadeia de font-size, um
`dp_ratio` e dimensões de viewport -- **não recebe tamanho de containing-block nenhum**.
Porcentagens em propriedades box-relativas (`width`, `margin-top`, `top`, ...) ficam
`Unit::PERCENT` do início ao fim do `ComputeProperty`; só são resolvidas a pixels depois, durante o
layout, via um parâmetro `base_value` passado separadamente que a assinatura do `ComputeLength`
desta onda não tem espaço para. A seção 5.1 nomeia as três famílias que um `%` pode pertencer e
declara, para cada uma, se este dump resolve agora ou fica simbólico.

**Consequência para o oráculo:** o lado A e o lado B cascateiam + resolvem unidades absolutas os
dois, os dois deixam porcentagens box-relativas/stop-de-gradiente/centro-radial como tokens
simbólicos `<número>%`, e nenhum dos dois tenta geometria que estruturalmente ainda não tem. Um
futuro dump da `RMLX-3` (% box-relativa resolvida contra layout real) **não** é uma extensão deste
por padrão -- mesma disciplina que a seção 10 do `docs/uix-dom.md` declara pra própria fronteira
fora-de-onda.

### 2. Endereçamento de nó (reusado, sem mudança, do `docs/uix-dom.md`)

Este dump percorre a **mesma árvore**, endereçada da **mesma forma**, que a seção 3 do
`docs/uix-dom.md`: `body` é a raiz literal, todo caminho descendente é `body` seguido de um segmento
`/<n>` por nível, `n` a posição 0-based do nó entre os filhos **sobreviventes** do pai (nós de texto
só-whitespace excluídos, pela seção 6a do `docs/uix-dom.md` -- sem mudança aqui, esta onda não
redefine a forma da árvore). A opacidade do próprio `<head>` (seção 4 do `docs/uix-dom.md`) também
não muda: `head` não carrega elemento estilizável nenhum e não ganha registro `PROP` algum.

**Só nós elemento carregam registros `PROP`.** Nós de texto (`TEXT` no vocabulário do
`docs/uix-dom.md`) não têm propriedade RCSS própria nenhuma -- estilo é computado por-`Element`, e
um nó de texto não é um `Rml::Element`. Um caminho que é caminho de nó de texto no dump da `RMLX-1`
simplesmente não aparece neste dump de jeito nenhum; não existe bloco `PROP` pra ele, e um dumper
conforme não pode sintetizar um (ex.: copiando os valores computados do pai pro próprio caminho do
nó de texto -- isso inventaria em silêncio um fato que este formato não define).

### 3. Forma do arquivo: blocos `STATE`, depois enumeração `PROP` por nó

**Visão geral de formato (`UIX-RCSS-ERRATA-2`, declarada explicitamente aqui pela primeira vez -- o
irmão `docs/uix-dom.md` §1 abre com esta mesma frase pro próprio formato, este documento nunca tinha
o equivalente, e uma auditoria achou a lacuna):** textual, UTF-8, um fato por linha
`PROP`/`PROPS`/`STATE`. O comparador byte-`==` que a seção 8 define é o próprio mecanismo de
comparação escolhido por este formato -- um afastamento deliberado do `diff` de string cru do
`docs/uix-dom.md`, não um esquecimento; a seção 8 já declara por quê (o passo de quantização é onde
a "clemência" deste formato mora, então o próprio comparador pode ficar um `==` estrito).

**Terminador de arquivo (`UIX-RCSS-ERRATA-2`, fechando uma lacuna que o `docs/uix-dom.md` §1 já
fechava pro próprio formato e este documento não tinha herdado por nome):** o arquivo de dump sempre
termina com uma newline final única depois da própria última linha `PROP` do próprio último bloco
`STATE` -- mesma convenção e mesma justificativa da própria cláusula de terminador de arquivo do
`docs/uix-dom.md` (evita uma linha de diff espúria de "sem newline no fim do arquivo", load-bearing
pro comparador byte-`==` que a seção 8 define: um dump sem esse byte final e um que o tem são
*arquivos diferentes* mesmo com toda linha impressa idêntica). Nenhuma linha em branco entre blocos
`STATE`, nenhuma no exato início do arquivo.

O dump tem **N blocos `STATE`** de topo (a seção 4 abaixo define exatamente quantos estados, e por
que exatamente esse número, nem mais nem menos), cada um uma **enumeração de propriedade da árvore
inteira, completa e independente**, um atrás do outro num arquivo só:

```
STATE <nome>
<caminho> PROPS <n>
<caminho> PROP <nome-propriedade>=<valor-canônico>
... (n destas, em ordem ascendente byte-a-byte por <nome-propriedade>, uma por entrada do
     registro independente de o rule-set daquele nó ter de fato declarado ou não -- seção 6)
... (repetido pra todo nó da árvore, na mesma ordem pré-ordem profundidade-primeiro que a
     seção 5 do docs/uix-dom.md define, raiz body incluída)
STATE <nome>
... (a travessia inteira de novo, sob o próximo estado forçado)
```

`<caminho> PROPS <n>` está **sempre** presente pra todo nó elemento, `n` **sempre** igual ao tamanho
fixo do registro (seção 6) -- nunca menos, porque este dump enumera o **registro inteiro por nó, não
só o que foi setado** (a própria instrução do brief, e a evidência concreta do próprio
`docs/uix-dom.md`: um formato dump-só-do-setado não consegue distinguir "esta propriedade nunca foi
tocada, o valor inicial se aplica" de "esta propriedade foi setada pra um valor que por acaso é
igual ao inicial" de "o motor tem um bug e descartou a declaração em silêncio" -- as três produzem
*saída* idêntica sob dump-só-do-setado, e só as duas primeiras deveriam). Um nó com **zero**
declarações autorais em qualquer ponto da cadeia de cascata ainda emite o bloco de `n` linhas
inteiro, todo valores iniciais-ou-herdados -- isto não é tratado como "nada a reportar" do mesmo
jeito que o `CHILDREN 0` do `docs/uix-dom.md` ainda ganha linha própria (mesmo raciocínio: um bloco
ausente não prova nada, um bloco cheio de valores iniciais prova que alguém olhou).

**Escaping.** Todo `<valor-canônico>` é escapado exatamente pela tabela de 4 regras da seção 2 do
`docs/uix-dom.md` (`\`, `\n`, `\r`, `\t` -- espaço literal e toda sequência multi-byte UTF-8 passam
sem mudança), reusada verbatim, não reinventada, pelo mesmo motivo que a seção 2 daquele documento
dá: uma segunda convenção de escape inventada aqui seria só mais um lugar pra dois implementers
independentes divergirem em algo que carrega zero informação nova.

### 4. Matriz de estados: enumere o espaço pequeno, não busque dentro dele

**Decisão, e o raciocínio atrás dela (a própria regra da casa de "enumere o espaço pequeno inteiro
em vez de buscar dentro dele", citada pelo próprio brief):** o censo de corpus da `RMLX-2`
(`UIX-RCSS-CENSUS`, 62 arquivos, `/var/tmp/censo-rcss-qa1/censo.md` seção 2, também os 12 números
verificados de forma independente da seção 2 do `docs/rmlx-subset.md`) mede **`:hover`: 53-37 usos
dependendo de qual repositório é contado, sempre em seletor composto** (`.foo:hover`, nunca puro) e
**`:focus`: 3, `:active`: 2** -- os dois reais mas duas ordens de grandeza mais raros que `:hover`.
Em vez de tentar adivinhar *quais elementos específicos* um autor de fixture pretendia que
estivessem hovered/focused/active (uma busca dentro de um espaço não-limitado -- não há forma
confiável de inferir "o mouse está sobre este botão" a partir de markup estático), este dump força
cada pseudo-classe booleana **globalmente, em todo elemento simultaneamente**, e enumera esse espaço
de estado pequeno, fechado, de 2 membros, por completo:

| Nome do estado | O que força | Justificativa de corpus |
| :--- | :--- | :--- |
| `none` | Nenhuma pseudo-classe forçada em elemento nenhum (default RmlUi/glintfx: nada hovered, focused ou active) | O baseline contra o qual todo fixture é autorado |
| `hover-all` | `:hover` forçado **verdadeiro** em todo elemento da árvore, simultaneamente | 37-53 usos reais, sempre composto, "produto principal" declarado desta onda |

**Ordem de arquivo dos blocos `STATE` (`UIX-RCSS-ERRATA-2`, achado por auditoria independente,
`Achado E`): a própria ordem de linha da tabela acima, `none` primeiro, é uma sequência fixa,
prosa-declarada -- antes só era mostrada (esta tabela, o exemplo trabalhado da seção 15.1), nunca
declarada como regra.** Isto **não** é resolvido pela regra de ordenação byte-wise que as seções 3/6
usam pra nomes de propriedade e outros tokens em outro lugar deste documento --
`"hover-all"` ordena antes de `"none"` byte-wise (`'h'` < `'n'`), que é a ordem oposta que a tabela e
todo exemplo trabalhado já usam, então essa regra não pode ser assumida como governando também a
ordem de `STATE` sem esta frase dizer isso. Uma futura adição `focus-all`/`active-all` se acrescenta
ao fim desta mesma sequência fixa, na ordem em que a tabela acima ganha as novas linhas, nunca
reordenando as existentes.

`:focus` e `:active` **não** são linhas separadas na matriz desta onda -- 3 e 2 usos medidos
respectivamente é uso real, não-zero (diferente do zero medido de `nth-child`/`:not`/`z-index` da
seção 2 do `docs/rmlx-subset.md`, que aquele documento corretamente trata como corte de zero real),
então **o próprio mecanismo de casamento de pseudo-classe tem de ser genérico** (qualquer uma de
`:hover`/`:focus`/`:active` tem de ser uma flag booleana selecionável e setável por elemento no
matcher de seletor dos dois motores -- não um caso especial hardcoded pra `:hover`), mas a **matriz
do oráculo desta onda** exercita só `hover-all` por nome. Somar linhas `focus-all` e `active-all`
depois é **uma linha a mais nesta tabela**, não um redesenho, precisamente porque o mecanismo de
casamento por baixo nunca foi específico de hover pra começo de conversa. Isto fica registrado como
uma decisão de escopo deliberada e limitada, não uma omissão silenciosa -- se um censo futuro de
consumidor crescer uso significativo de `:focus`/`:active`, o conserto é "somar uma linha aqui", pela
própria cláusula de cabeçalho deste documento, não "descobrir fixture por fixture" do jeito que a
`UIX-HEAD-PREFIXO-CEGO` teve de ser descoberta por sonda ao vivo em vez de estar visível por escrito.

**Por que `hover-all` (todo elemento de uma vez) em vez de um dump por elemento hoverable:** a
alternativa -- achar todo elemento que alguma regra `:hover`-sufixada poderia casar e dumpar um
estado por elemento desses -- transforma um espaço de estado fixo, pequeno, enumerável, numa busca
cujo tamanho depende do fixture (exatamente o anti-padrão que o próprio exemplo do brief avisa
contra). Forçar `:hover` verdadeiro em tudo de uma vez é um estado **único, determinístico,
independente-de-fixture** que ainda exercita toda regra `:hover` da folha numa passada só (um
elemento sem regra `:hover` que o alveje computa identicamente nos dois estados, o que já é sinal
útil -- uma divergência aparecendo só em `hover-all` num nó sem regra de hover nenhuma apontaria pra
um bug no tratamento de escopo global do matcher, não pra própria regra). O custo, declarado sem
rodeio: `hover-all` não é um estado que sessão real nenhuma de usuário produz (só um subconjunto de
elementos é hovered de cada vez) -- este dump prova que a cascata *computa* estilização de `:hover`
corretamente, não que o *pipeline de eventos* que decide qual único elemento está hovered em runtime
está correto (isso é trabalho da `RMLX-5`, fora do escopo deste dump, restated na seção 12).

### 5. Porcentagens: três famílias de resolução, uma forma de impressão simbólica

**A própria formulação do líder, ao pé da letra do `TODO.md`, e por que um `resolve_percent()`
único está errado por construção:** um valor `%` significa três coisas geometricamente não
relacionadas dependendo de qual propriedade o carrega, evidenciado pelo censo
(`/var/tmp/censo-rcss-qa1/censo.md` seção 5.1) e pelas próprias assinaturas de função do
`ComputeProperty.cpp` (seção 1 acima):

| Família | Propriedades (medidas) | Resolve contra | Este dump resolve? |
| :--- | :--- | :--- | :--- |
| (a) Box-relativa | `width`, `height`, `top`, `right`, `bottom`, `left` (também, não-medidas mas estruturalmente idênticas: `margin-*`, `padding-*`, `min/max-width/height`, `flex-basis`) | A dimensão de content-box do **containing block** -- um fato de layout | **Não.** Fica um token simbólico `<número>%` (fronteira da própria seção 1) |
| (b) Posição de stop de gradiente | `<posição%>` dentro de `linear-gradient(...)`/`radial-gradient(...)` (`decorator`, `mask-image`, e o argumento `<preenchimento>` de `polygon(...)`) | O **próprio eixo** do gradiente (0% = primeiro ponto do eixo, 100% = último), independente do tamanho da caixa do elemento | **Não.** Fica simbólico (gramática composta da própria seção 9) -- ver a nota abaixo sobre *auto-espaçamento*, que É resolvido apesar da posição em si ficar simbólica |
| (c) Coordenada de centro radial | `<x%> <y%>` em `radial-gradient(circle at <x%> <y%>, ...)` | O **próprio espaço local 2D** do gradiente (o próprio círculo inscrito / caixa de eixo, pela gramática `polygon()`+`radial-gradient` do `docs/effects.md`) -- também não é o border-box do elemento no sentido CSS geral, e especificamente **não** é a mesma família de eixo que (b) resolve contra | **Não.** Fica simbólico, mesmo raciocínio de (b) |

**Por que as famílias (b) e (c) não precisam da `RMLX-3` mesmo ficando simbólicas:** diferente da
família (a), as famílias (b) e (c) nunca resolvem contra o containing block *do elemento* de jeito
nenhum -- resolvem contra o espaço de coordenada local do próprio decorator, que por si só só é
totalmente conhecido em tempo de render (a própria caixa do elemento, que pro caso específico de
(b)/(c) fica disponível um pouco mais cedo que a dependência de containing-block da família (a), mas
*esta onda traça a linha em "nenhuma geometria de caixa, de espécie nenhuma"*, então (b)/(c) são
adiadas pelo mesmo motivo que (a), não porque são mais difíceis). O ponto de nomear três famílias em
vez de escrever `resolve_percent()` uma vez **não é** "algumas resolvem agora e outras depois" --
nesta onda, **nenhuma das três resolve agora** -- é que um futuro implementer ligando a `RMLX-3`/
`RMLX-9` precisa despachar cada ocorrência de `%` pra correta entre três quantidades-base
diferentes, e confundir elas (ex.: por acidente alimentar uma porcentagem de stop de gradiente no
resolvedor de largura-de-containing-block) é exatamente a classe de bug que uma função genérica
única convida, e esta tabela existe pra prevenir.

**O auto-espaçamento de stop de gradiente É resolvido por este dump, e isto não é contradição do
acima.** O `docs/effects.md` (seção "How-to: um polígono com preenchimento em gradiente")
documenta, como comportamento glintfx já entregue: *"A `<posição%>` do stop é opcional e
auto-espaçada como no CSS quando omitida"*. Auto-espaçamento é uma função pura de **índice do stop e
contagem total de stops dentro da mesma função de gradiente** -- precisa zero geometria de caixa,
zero layout, nada que esta onda não tenha -- então este dump resolve toda posição de stop
**omitida** pra sua porcentagem auto-espaçada explícita padrão-CSS antes de imprimir (seção 9.2 dá
o algoritmo exato e um exemplo trabalhado). Este é o único ponto deste documento em que uma
porcentagem que começou simbólica na fonte vira um número concreto impresso: a *distribuição* é
resolvida (determinística, sem geometria), a *base* da qual ela é porcentagem (o eixo do gradiente,
família (b)) não é.

**Decisão que fecha uma ambiguidade reportada: as famílias (b) e (c) nunca se fundem, mesmo as duas
imprimindo como um `<número>%` cru dentro das mesmas funções de gradiente.** O próprio relatório de
entrega da `UIX-RCSS-SPEC` sinalizou isto como uma leitura em aberto: "um segundo implementer lendo
rápido poderia fundir (b) e (c) por serem sintaticamente parecidas" -- as duas são `%`, as duas só
aparecem dentro de `linear-gradient(...)`/`radial-gradient(...)`, e nada no fluxo de token RCSS cru
marca uma diferente da outra (o `<posição%>` da família (b) e o `<x%> <y%>` da família (c) são só
texto numérico-percentual `Declaration`/`Prelude`-adjacente pro próprio lexer da `UIX-RCSS-LEXER` até
uma futura fatia de parser). **Argumento pra fundir numa família "% de gradiente" só:** é menos
código -- um resolvedor só, parametrizado por "eixo 1D" vs. "espaço local 2D", poderia servir os
dois, e o próprio CSS não os nomeia como *tipos* diferentes de porcentagem do jeito que a tabela
deste documento faz. **Argumento contra, e a decisão:** fundir seria exatamente a classe de bug que
o próprio parágrafo de abertura da seção 5 nomeia pra confundir quaisquer duas das três famílias -- a
família (b) resolve contra o **próprio eixo 1D** do gradiente (0% = primeiro ponto, 100% = último,
um **comprimento ao longo de uma linha**), a família (c) resolve contra o **próprio espaço de
coordenada local 2D** do gradiente (um par `(x, y)`, não um offset escalar) -- não são a mesma
*quantidade*, só a mesma *forma de impressão*, e um resolvedor escrito pra "tratar uma porcentagem de
gradiente" de forma genérica convida exatamente o cross-feed acidental que a tabela de famílias deste
documento existe pra prevenir (o próprio exemplo trabalhado da seção 5: alimentar uma porcentagem de
stop de gradiente no resolvedor errado). **Ficam duas famílias distintas, nunca uma.** Isto não é só
prosa restated -- a seção 15.3 dá a forma trabalhada byte-exata das três famílias lado a lado,
especificamente pra um segundo implementer não conseguir chegar na leitura fundida nem passando o
olho rápido: as duas famílias nunca compartilham posição de argumento dentro de uma mesma chamada de
função (a família (c) é sempre o(s) primeiro(s) 1-2 argumento(s) de `radial-gradient`, antes da
própria cláusula `circle at` ser consumida; a família (b) é sempre dentro de uma entrada `<stop>`,
depois da cor) -- um exemplo trabalhado que mostra as duas na mesma chamada de função fecha a
ambiguidade que uma regra só-em-prosa não consegue.

### 6. O registro de propriedades (a lista fixa e fechada que todo bloco `PROPS` enumera)

**Disciplina de escopo, declarada uma vez pra não repetir por linha:** o próprio texto de escopo da
`RMLX-2` no `TODO.md` diz *"só as propriedades medidas em uso"* -- este registro é construído
**exclusivamente** de nomes que o censo (`/var/tmp/censo-rcss-qa1/censo.md` seção 3) mediu pelo
menos uma vez, expandido através das definições de shorthand (um shorthand como `margin` não é ele
mesmo uma entrada de registro -- ver seção 6.2 -- mas seus longhands constituintes são, tenha esse
longhand específico sido *também* medido escrito direto ou não). Uma propriedade que esta tabela não
lista está **fora de escopo por padrão**, pela própria cláusula do `docs/rmlx-subset.md` -- a seção
13 abaixo nomeia toda exclusão de zero-medido ou só-at-rule explicitamente, pra um futuro
implementer que achar uma numa fixture real parar e editar esta spec em vez de adivinhar.

**Fonte de evidência pra todo valor inicial e toda flag `inherited` da tabela abaixo** (citada uma
vez, não por linha, pra manter a tabela legível): `examples/RmlUi/Source/Core/
StyleSheetSpecification.cpp:262-433` (propriedades nativas do RmlUi, assinatura de função
`RegisterProperty(id, name, default_value, inherited, forces_layout)` -- **note que o 4º argumento
posicional é `inherited`, não `forces_layout`; um leitor passando o olho pelos call sites não pode
trocar os dois**, que é exatamente o tipo de má leitura que este documento existe pra prevenir
declarando o resultado já parseado direto, em vez de fazer um implementer re-derivar de uma
assinatura de chamada não-familiar); `glintfx/src/rml/decorator_ripple.cpp:332-336` e
`glintfx/src/rml/decorator_image_tint.cpp:409-411` (as propriedades customizadas autorais da
glintfx).

**Ordem de classificação das linhas `PROP` dentro de um bloco `PROPS`: ascendente, byte-a-byte, por
nome de propriedade -- `std::string::operator<` sobre bytes UTF-8 crus, sem locale, mesma regra e
mesma justificativa da ordenação `CLASS`/`ATTR` da seção 7 do `docs/uix-dom.md`** (reproduzível
entre máquinas, sem argumento sobre qual locale é "correto"). Isto **não** é a ordem do enum interno
`PropertyId` de nenhum dos dois motores -- a ordem numérica de um enum interno é detalhe de
implementação que nenhum dos dois lados deveria ter de replicar identicamente, exatamente o risco de
suposição-privada-compartilhada que a própria seção "por que este documento existe" do
`docs/uix-dom.md` avisa.

#### 6.1 Tabela de registro (72 entradas longhand, alfabética -- a própria ordem exigida do dump)

*(mesma tabela da seção 6.1 em inglês acima -- valores, nomes de propriedade, e domínios de valor
não são traduzidos, são identificadores de código en-intl por convenção do próprio `CLAUDE.md` do
projeto.)*

**⚠️ Duas entradas que soam surpreendentes e estão corretas como medidas, sinalizadas pra ninguém
"consertar" depois:** `focus` é `inherited: true` no RmlUi upstream apesar de controlar algo (se
`Element::Focus()` pode ter sucesso) que não tem noção intuitiva de "herdar" -- confirmado direto no
call site de registro, não inferido. `opacity` também é `inherited: true`, o que **não** é como o
próprio `opacity` do CSS se comporta (o `opacity` real do CSS não herda; compõe visualmente através
de stacking contexts em vez disso) -- o próprio modelo do RmlUi é um mecanismo genuinamente
diferente (a própria opacidade de cada descendente, se não setada, cascateia do *valor* do
ancestral, e o RmlUi adicionalmente **multiplica** opacidades pela árvore de render em tempo de
desenho, algo que `docs/embed-integration.md` não menciona e está fora do escopo deste dump por ser
uma composição em tempo-de-render, não um valor computado em tempo-de-cascata -- o próprio valor
computado de `opacity`, que este dump reporta, é exatamente o número único cascateado ao estilo de
herança CSS, não o produto composto).

**⚠️ `max-height`/`max-width`: a única entrada de registro com zero justificativa de corpus, mantida
de propósito, não um bug pra consertar depois.** A própria entrega da `UIX-PROP-REGISTRY` fechou a
conta 64-vs-72 (seção 6 acima) e achou que essas duas são as **únicas** 2 das 72 entradas longhand
com **zero** ocorrência medida em lugar nenhum do corpus deste documento
(`/var/tmp/censo-rcss-qa1/censo.md`) -- não escritas direto, e não alcançáveis por nenhum dos 13
shorthands da seção 6.2 (nenhum shorthand expande em `max-height`/`max-width`; são propriedades
nativas do RmlUi, planas, sem expansão). A própria disciplina de escopo da seção 6 declara que este
registro é construído "exclusivamente" de nomes medidos -- por essa regra sozinha, essas duas não
pertenceriam. **Ficam no registro mesmo assim**, por dois motivos declarados aqui uma vez pra um
futuro leitor não reabrir a discussão fixture por fixture: (1) a própria tabela deste documento
(seção 6.1 acima) já as listava antes da disciplina de exclusividade-de-corpus ser escrita -- e a
spec é o contrato que os dois autores independentes de dumper constroem contra; remover uma entrada
de registro já publicada exige a mesma disciplina "parar, editar esta spec com um diff, aval do
líder" que a seção 13 exige pra *somar* um item fora-de-subconjunto, não um descarte silencioso; (2)
o motivo mais duradouro, restated da própria regra permanente deste projeto: o alvo da glintfx é
**distribuição ampla**, e "zero ocorrência no corpus de dois projetos" é uma afirmação verdadeira
sobre dois repositórios, nunca uma afirmação verdadeira sobre o mundo -- um consumidor que este
documento nunca viu pode genuinamente autorar `max-height: 200px;` amanhã. **O teto a que esta
decisão fica limitada:** essas duas entradas ficam pinadas exatamente como estão hoje --
`keyword(none)` ou length-percent (família a), mesmo domínio e forma de impressão que toda outra
propriedade box-relativa da tabela -- pelo próprio
`test_max_height_max_width_are_the_one_known_unexplained_gap` do
`glintfx/tests/uix_style/property_registry_sanity.cpp`; um futuro censo que medir um uso real de
qualquer uma delas é uma **confirmação**, não uma descoberta, e não muda nada nesta decisão; um
futuro censo que achar uma **terceira** entrada zero-corpus-mas-listada é uma anomalia **nova** e
precisa ser reportada do mesmo jeito que esta foi, não dobrada em silêncio nesta mesma justificativa.

**🔵 Conjuntos de palavras-chave enumerados, acrescentados pela `UIX-RCSS-ERRATA-5`, 2026-08-06 -- 29
destas 72 linhas são de domínio `Keyword` (o domínio único, ou o domínio primário de uma linha de
dois domínios); zero linhas têm `Keyword` só como domínio *alternativo*. Das 29, 18 já carregavam o
próprio conjunto de membros legal nomeado inline antes desta errata** (`bottom`, `flex-basis`,
`focus`, `height`, `image-tint-mode`, `left`, `letter-spacing`, `margin-bottom`, `margin-left`,
`margin-right`, `margin-top`, `max-height`, `max-width`, `right`, `tab-index`, `text-overflow`,
`top`, `width` -- cada uma uma palavra só, `auto`/`none`, ou um par/quádrupla curto enumerado) --
**as 11 restantes carregavam só a palavra nua "keyword", sem lista de membros, até esta errata:
`align-items`, `box-sizing`, `display`, `justify-content`, `overflow-x`, `overflow-y`, `position`,
`text-align`, `text-transform`, `vertical-align`, `white-space`.** As 11 são fechadas acima, nas
próprias células da tabela, a mesma convenção que as outras 18 já usavam -- esta tabela permanece a
fonte única de verdade pro próprio conjunto legal de toda propriedade de domínio palavra-chave; um
futuro implementer não consulta um segundo documento nem o próprio fonte upstream direto pra esta
lista, só cita aqui.

**Fonte e uma decisão de julgamento, nomeada pra poder ser revertida num lugar só:** os 11 conjuntos
são a **enumeração upstream RmlUi completa**, verbatim do próprio call site
`RegisterProperty(...).AddParser("keyword", "...")` de cada propriedade
(`examples/RmlUi/Source/Core/StyleSheetSpecification.cpp`): `align-items` `:417`, `box-sizing`
`:318`, `display` `:306-307`, `justify-content` `:426`, `overflow-x`/`overflow-y` `:335-336`,
`position` `:308`, `text-align` `:361`, `text-transform` `:363`, `vertical-align` `:331-333`,
`white-space` `:364` -- o próprio conjunto de 4 membros de `image-tint-mode` é autorado pela glintfx,
não upstream, já corretamente enumerado antes desta errata
(`glintfx/src/rml/decorator_image_tint.cpp:409-411`), não afetado por esta passada, restatado aqui só
pela completude do próprio denominador. **Por que upstream completo, não um subconjunto
corpus-estreitado** (a mesma disciplina "zero real é corte real" que a seção 13 abaixo aplica a
*propriedades e formas de sintaxe de valor não-medidas*): aceitar uma string de palavra-chave é uma
checagem de pertencimento-a-conjunto de custo zero, não maquinaria nova de parser/serializador do
jeito que aceitar uma forma funcional de cor (`rgb()`, seção 13) ou uma propriedade nova
(`transition`, seção 13) seria -- não há nada aqui análogo a "implementar uma feature que ninguém
pediu". O próprio escopo deste dump é valores **computados**, nunca layout (§12); imprimir
`display=table-row` como um valor computado legal não compromete este projeto a nada sobre algum dia
fazer layout de tabela, exatamente como imprimir um comprimento fora-do-viewport não compromete a
nada sobre algum dia renderizar fora-da-tela. O precedente existente mais próximo é a própria linha
de 3 valores do algoritmo `Box` da seção 6.3: real, não-medida, upstream, barata de incluir
fielmente, mantida **porque** não custa nada além de implementar honestamente um algoritmo que esta
tabela já exige pros casos medidos. **Sinalizado pra atenção do líder de qualquer jeito, não decidido
em silêncio** (a mesma cortesia que a própria chamada de pré-multiplicação da `ERRATA-2` e a própria
reversão dela pela `ERRATA-4` estenderam): a própria lista de 14 membros upstream do `display` é a
que mais provavelmente merece um segundo olhar, já que vários dos próprios membros nomeiam modos de
layout (`table-row`, `flow-root`, ...) que este código-base não tem plano nenhum de algum dia
fazer layout. Se o líder julgar que um conjunto `display` corpus-estreitado (medido: `none`, `block`,
`inline-block`, mais `flex` pelo próprio censo do consumidor da seção 2 do
`docs/rmlx-subset.md`, 10 instâncias) é a decisão certa em vez disso, isso é uma edição de célula
aqui, não uma mudança de design em lugar nenhum mais deste documento.

#### 6.2 Expansão shorthand-pra-longhand (sem slot próprio de registro; alimenta as entradas longhand acima)

*(mesma tabela da seção 6.2 em inglês -- nomes de propriedade e algoritmos não traduzidos. A linha de
`border-top`/`-right`/`-bottom`/`-left` foi corrigida em 2026-08-06 -- ver a nota a seguir.)*

**Correção à linha de `border-top`/`-right`/`-bottom`/`-left` da tabela acima, datada 2026-08-06 (ver
o bloco de errata no cabeçalho deste documento pro relato completo):** "independente de ordem entre
os dois" era **falso** pra uma cadeia `FallThrough` de 2-itens/2-tokens. A regra real, rastreada
iteração-por-iteração contra o próprio laço real do upstream
(`examples/RmlUi/Source/Core/PropertySpecification.cpp:429-471`, lido direto): o upstream sempre
avança o cursor de **item** a cada iteração (casamento ou não), e só avança o cursor de **token** num
casamento -- então um token que falha o próprio domínio do item 0 e só casa com o item 1 (a ordem
revertida, cor-depois-width) é reivindicado pelo item 1, deixando o cursor de item esgotado com o
outro token ainda não-reivindicado, e a própria guarda pós-laço do upstream (`value_index <
property_values.size() && property_index >= items.size()`) aborta o shorthand **inteiro** -- não um
resultado parcial. Concretamente, pra `border-top`: `1dp #7A5A2E` (width-depois-color, a própria
ordem 100%-medida do corpus) tem sucesso; `#7A5A2E 1dp` (color-depois-width) é `MalformedValue`, e
`ParseShorthandDeclaration` retorna `false` pra declaração `border-top` inteira -- **mas ver a
correção da `UIX-RCSS-ERRATA-2` logo abaixo da tabela antes de confiar no que aquele `false` implica
sobre quais longhands de fato ficam segurando o quê.** **O que "independente de
ordem" É verdade:** qual *domínio* um token roteia é guiado por conteúdo (um token com forma de
comprimento roteia pra `-width` independente da posição em que aparece) -- essa parte da frase
original não estava errada. **O que não é verdade:** que uma *ordem* de token arbitrária sempre tem
sucesso pra uma cadeia de 2-itens/2-tokens. Não tem. A seção 15.2 abaixo dá o dump byte-exato das
duas ordens lado a lado. Prova, não só afirmação: pinado pelo próprio
`test_border_top_fallthrough_order_is_load_bearing` do
`glintfx/tests/uix_style/shorthand_expansion_sanity.cpp`, e já declarado corretamente no próprio
`glintfx/src/uix/style/shorthand.hpp:38`/`shorthand.cpp:30-35` antes deste documento ser corrigido
pra bater.

**Segunda correção (`UIX-RCSS-ERRATA-2`, 2026-08-06): o parágrafo acima está ele mesmo pela metade
errado sobre a *consequência* daquele retorno `false`, achado por uma auditoria independente
(`UIX-RCSS-AMBIGUIDADE`, `docs/uix-rcss-ambiguidades.md`, Achado A) e reverificado linha-por-linha
pelo `tech-lead` antes desta correção ser escrita.** **Não é verdade** que "tanto
`border-top-width` quanto `border-top-color` ficam com o que a regra de próxima-especificidade-menor
da cascata fornecer" pro caso de ordem revertida. O laço `FallThrough`/`Box` do upstream
(`PropertySpecification.cpp:433-472`) **não tem buffer de staging** -- `dictionary.SetProperty(...)`
(`:461`) dispara **dentro do laço, no momento em que o `ParseValue` de qualquer item tem sucesso**,
antes da guarda pós-laço (`:469-471`) sequer rodar. Rastreando `#b { border-top: #7A5A2E 1dp; }`
token por token: iteração 1, item 0 (`-width`) tenta `"#7A5A2E"`, falha ao parsear como comprimento,
e (sendo `FallThrough` com um próximo item disponível) dá `continue` -- o próprio incremento do
cabeçalho do laço `for` avança `property_index` pra 1, `value_index` fica em `0` (um `continue` em
C++ não pula a própria cláusula de incremento do cabeçalho `for`, só pula o resto das instruções do
corpo do laço). Iteração 2, item 1 (`-color`) tenta o **mesmo `"#7A5A2E"` ainda não-reivindicado**,
tem sucesso, e `dictionary.SetProperty(BorderTopColor, ...)` dispara **ali mesmo** --
`border-top-color` é setado a partir do token da fonte. Só *depois* do laço a guarda pós-laço vê
`value_index(1) < property_values.size()(2)` e `property_index(2) >= items.size()(2)`, e retorna
`false`. Nada rio-abaixo daquele `false` desfaz a chamada `SetProperty` que já aconteceu --
`StyleSheetParser::ReadProperties` (`StyleSheetParser.cpp:1023`) só loga um aviso no `false`, e
`PropertyDictionary::SetProperty` (`PropertyDictionary.cpp:8`) é um `properties[id] = property;` cru,
sem camada transacional pra desfazer. **A consequência corrigida pro `body/1` (`#b`):
`border-top-color=#7a5a2eff` (setado a partir do token da fonte, item 1 casou antes da falha
aparecer), `border-top-width=0.0000px` (item 0 nunca casou nada nesta chamada, cai pro próprio valor
inicial de registro da seção 6.1) -- só `-width` reverte, `-color` não.** A seção 15.2 abaixo é
corrigida pra bater. O mesmo mecanismo se aplica ao próprio `RecursiveRepeat` de `border` (ver a
linha da tabela acima e a correção correspondente na seção 11 abaixo): cada uma das 4 sub-chamadas
de side-shorthand roda até o fim de forma independente (`result &= ParseShorthandDeclaration(...)`,
`PropertySpecification.cpp:379`, sem saída antecipada), então um `border: #7A5A2E 1dp;` revertido
escreve parcialmente os 4 longhands `-color` a partir do token da fonte (cada sub-chamada
`FallThrough` de cada lado bate no mesmo bug de forma independente) enquanto deixa os 4 longhands
`-width` intocados, *antes* do `result=false` agregado fazer a chamada externa retornar falha.

**🔵 Escopo do parágrafo acima, estreitado pela `UIX-RCSS-ERRATA-5`, 2026-08-06: isto é um rastro do
próprio comportamento real do upstream, verdadeiro pro lado A (que *é* código upstream) e, a menos
que uma fixture prove o contrário, pro próprio caso `RecursiveRepeat` do `border` também -- não é
mais, pela própria decisão do líder, uma descrição do lado B.** O próprio motor clean-room do lado B
(`shorthand.cpp`/`parser.cpp`) descarta uma declaração `FallThrough`/`RecursiveRepeat`
**atomicamente** na mesma entrada de ordem revertida -- nenhum longhand segura um casamento parcial,
cada um deles cai pro próprio valor inicial de registro da seção 6.1 (ou valor herdado) -- e esse
comportamento de descarte atômico é o que **fica**, mantido deliberadamente em vez de casado
bug-por-bug com a própria mutação de dicionário sem rollback do upstream. Ver a seção 14.1 abaixo,
"Divergências deliberadas", pro relato completo, o par byte-exato, e a obrigação de fixture que esta
decisão carrega. O próprio exemplo trabalhado da seção 15.2 é dividido de acordo.

##### 6.3 O algoritmo `Box` (verbatim de `PropertySpecification.cpp:336-370`, expansão de box-model CSS padrão)

*(mesma tabela da seção 6.3 em inglês.)* A linha de 3 valores **não** é medida em fixture nenhuma do
corpus do censo mas é comportamento real e alcançável upstream (o mesmo motor de shorthand tipo
`Box` que `margin`/`padding` já usam pras próprias formas medidas de 1/2/4 valores) -- incluída aqui
pra paridade completa porque não custa nada além de implementar fielmente o único algoritmo já
exigido pelos casos medidos, não uma extensão especulativa.

### 7. Formas canônicas de impressão por domínio de valor

*(mesma tabela em inglês -- os nomes de domínio (`keyword`, `number`, `length`, etc.) são
identificadores técnicos, não traduzidos.)*

**Valor computado vazio no domínio string (`UIX-RCSS-ERRATA-2`, fechando uma lacuna que o documento
irmão `docs/uix-dom.md` §7 já fechava pro próprio caso `ATTR data-if=` mas este documento ainda não
tinha declarado pro caso análogo próprio):** `cursor` e `font-family` registram `*(vazio)*` como o
próprio valor inicial da seção 6.1. Uma linha `PROP` pra qualquer uma das duas, quando o valor
computado é a string vazia, ainda imprime -- `<caminho> PROP cursor=` (nada depois do `=`), nunca
omitida -- a mesma convenção que o `docs/uix-dom.md` §7 declara por nome pro próprio `ATTR
data-if=` (presente-com-valor-vazio e ausente são estados diferentes, os dois consultáveis; uma
linha `PROP` de domínio `string` sempre existe pela própria regra "o registro inteiro, todo nó" da
seção 3, então "presente com valor vazio" é o único estado alcançável aqui -- não existe estado
"ausente" pra distinguir dele, diferente de um atributo do DOM).

**Identificadores estruturais nunca são escapados (`UIX-RCSS-ERRATA-2`, espelhando o tratamento
idêntico que o `docs/uix-dom.md` §8 dá pro próprio `<tag>`):** a metade `<nome-propriedade>` do par
`nome=valor` de uma linha `PROP` nunca é escapada pela tabela da seção 2, pelo mesmo motivo que o
`docs/uix-dom.md` §5 dá pro próprio `<tag>` -- é sempre um dos 72 identificadores fixos, fechados,
ASCII kebab-case que a própria tabela da seção 6.1 nomeia (escolhido pelo *dumper*, iterando o
próprio registro, nunca ecoado de volta de caixa ou conteúdo arbitrário autorado na fonte do jeito
que um valor de atributo é), e nenhuma dessas 72 strings pode estruturalmente conter nenhum dos 4
caracteres de escape (`\`, `\n`, `\r`, `\t`). Escapar é portanto nunca *alcançável* pra este campo, e
este documento declara isso explicitamente em vez de deixar um segundo implementer imaginar se foi
esquecido.

**Valores de domínio string não escapam os próprios separadores compostos deste documento
(`UIX-RCSS-ERRATA-2`, fechando o Achado H):** a tabela de 4 regras da seção 3 (`\`, `\n`, `\r`, `\t`,
herdada do `docs/uix-dom.md` §2) é o **único** escape que um valor de domínio `string` recebe. Ela
**não** cobre `\|`, `;`, ou `:` -- os próprios separadores de lista-composta, argumento e stop deste
documento (seção 9), escolhidos especificamente pra que uma linha de dump *composta* nunca fique
ambígua sobre qual papel-de-vírgula um byte tinha na fonte. Um valor string de `font-family`,
`cursor`, ou `text-overflow` contendo um `;` ou `:` literal (os dois conteúdo string RCSS legal)
imprime byte-por-byte não-escapado no nível superior -- isto é um limite de escopo deliberado, não um
esquecimento: o objetivo "nunca ambíguo" da seção 9 foi declarado pra gramática de valor
*composto*, e não é estendido retroativamente pro valor string de uma linha `PROP` **plana,
não-composta**. Uma linha `PROP` é portanto só re-divisível com segurança em `=` (uma vez, na
primeira ocorrência) mais a gramática específica-de-domínio que a seção 9 define pra aquela
propriedade específica -- não é um registro delimitado de propósito geral.

#### 7.1 Cor: forma canônica, e o escopo exato autorizado hoje

**No escopo, autorizado pelas próprias 4 formas hex medidas pelo censo**
(`/var/tmp/censo-rcss-qa1/censo.md` seção 6, cruzado contra
`examples/RmlUi/Source/Core/PropertyParserColour.cpp:211-237`'s `switch (value.size())`): `#rgb` (3
dígitos hex), `#rgba` (4), `#rrggbb` (6), `#rrggbbaa` (8) -- **as quatro**, pela própria correção do
censo do comentário desatualizado de `showcase.rcss:8` (o código e o uso real suportam as quatro; o
comentário não). As quatro normalizam pra mesma forma canônica de 8 dígitos: cada canal de um dígito
dobra (`f` → `ff`, batendo com o próprio fallthrough `case 5 → case 4`/`case 9 → case 7` do
upstream), um canal alpha ausente tem default `ff` (totalmente opaco, batendo com o
pré-preenchimento default `hex_values[3] = {'f','f'}` do upstream), depois os quatro canais imprimem
como dois dígitos hex minúsculos cada, em ordem `rgba`, prefixados por `#`.

**Fora de escopo, exige aval do líder antes de qualquer implementação (seção 13):** as duas cores
nomeadas que o censo de fato mediu (`transparent` → `#00000000`, `white` → `#ffffffff`) estão no
escopo por serem medidas; o **resto** da tabela de 19 cores nomeadas do RmlUi (`black`, `red`,
`blue`, ... -- `PropertyParserColour.cpp:117-136`) e toda forma funcional de cor (`rgb()`, `rgba()`,
`hsl()`, `lab()`, `lch()`, `oklab()`, `oklch()`) estão com **zero medição** no censo (seção 0: *"0
`rgb()`/`rgba()` funcional"*) e **não** estão autorizadas pela decisão de paridade-de-unidades do
líder, que nomeou unidades, não sintaxe de cor. Um dumper conforme encontrando qualquer uma delas
tem de **fail-high** (seção 11), não suportar em silêncio por acaso porque o parser era fácil de
estender.

**Cores são dumpadas straight-alpha pra propriedades tipo-cor escalares -- `background-color`,
`border-*-color`, `color`, `image-tint-color`.** `Style::ComputedValues`/`Property::Get<Colourb>`
guardam alpha straight pra essas; não existe chamada de pré-multiplicação em lugar nenhum do próprio
caminho de parse delas.

**Corrigido, `UIX-RCSS-ERRATA-2` (2026-08-06): cores de camada de `box-shadow` e todo stop de
gradiente são a exceção, e são pré-multiplicadas, não straight -- achado por uma auditoria
independente (`UIX-RCSS-AMBIGUIDADE`, Achado B) e reverificado direto pelo `tech-lead`.** O texto
original acima afirmava que esses dois campos eram straight-alpha "inclusive" pras cores de
`box-shadow`/stop-de-gradiente -- **falso**. `PropertyParserBoxShadow.cpp:72`: `shadow.color =
prop.Get<Colourb>().ToPremultiplied();` (a citação tinha deriva pra `:69` num rascunho anterior --
`:72` é a linha verificada no clone upstream). `PropertyParserColorStopList.cpp:47` (o parser por
trás de todo stop de gradiente em `linear-gradient`/`radial-gradient`, os dois usados por
`decorator`/`mask-image`/`filter`/`backdrop-filter`): `color_stop.color =
p_color.Get<Colourb>().ToPremultiplied();` -- mesmo padrão. As duas chamadas disparam **em tempo de
parse**, antes do valor sequer chegar ao `Style::ComputedValues`, e o **próprio tipo do campo do
struct** é `ColourbPremultiplied` (`Include/RmlUi/Core/DecorationTypes.h:9`, `:22`), um tipo distinto
do `Colourb` straight que todo campo tipo-cor escalar usa -- não existe representação straight-alpha
desses dois campos em lugar nenhum rio-abaixo do parse, o sistema de tipos garante isso
estruturalmente.

**Decisão revertida, `UIX-RCSS-ERRATA-4` (2026-08-06), pelo próprio líder: imprimir
`ToNonPremultiplied()` do byte armazenado -- alfa direto, não os bytes pré-multiplicados como estão.**
O raciocínio da `ERRATA-2` acima é mantido no lugar em vez de apagado (a própria regra da casa deste
documento: decisão errada apagada volta), mas duas de suas três pernas não se sustentam. O raciocínio
(2) -- "despré-multiplicar é indefinido em `alpha=0`" -- é **falso**, medido contra
`examples/RmlUi/Include/RmlUi/Core/Colour.h:105-107`: o próprio corpo de `ToNonPremultiplied()` é
`ColourType(alpha > 0 ? (red * 255) / alpha : 0)` (mesma guarda em verde/azul), uma guarda explícita,
total pra toda entrada -- não há caso indefinido a evitar. E há evidência que a `ERRATA-2` não tinha e
que mina o raciocínio (1) -- "reportar o que quer que o pipeline de fato produziu": quando o próprio
pipeline do RmlUi real converte esses exatos dois tipos compostos pra texto, ele despré-multiplica
primeiro, `TypeConverter.cpp:223` (`ColorStopList` → `String`) e `TypeConverter.cpp:256`
(`BoxShadowList` → `String`), os dois chamando `ToNonPremultiplied()`. A resposta do próprio upstream
pra "que texto representa esta cor" é alfa direto pra esses dois campos, igual a todo outro --
imprimir os bytes pré-multiplicados crus não era ficar mecânico, era pular um passo que o próprio
upstream dá. O raciocínio (3) sobrevive em espírito mas não na conclusão: o lado A segue sendo um
leitor fiel do que está armazenado, e *também* aplica a única transformação mecânica, definida pelo
upstream, que o próprio caminho de conversão-pra-texto do upstream aplica exatamente nesta fronteira
-- não uma transformação inventada, uma emprestada. **Consequência: um par lado A/lado B conforme os
dois imprimem bytes no formato `Colourb` straight pra todo campo tipo-cor sem exceção,
`box-shadow`/stop-de-gradiente incluídos -- a regra geral da seção 7.1 agora cobre esses dois campos
também, sem cláusula de exceção. 🔴 Esta conversão é lossy nos dois sentidos (duas divisões inteiras
truncantes em série não se invertem uma à outra) -- o byte impresso não é igual nem ao valor escrito
na fonte nem ao valor que o `Style::ComputedValues` armazena depois do parse; ver a `UIX-RCSS-ERRATA-4`
acima pra tabela medida e fórmula normativa.** O próprio exemplo trabalhado da seção 9.1 é corrigido
pra bater, pela terceira vez.

### 8. Quantização numérica: a regra, escolhida e justificada

**O problema que esta regra resolve.** O lado A e o lado B são dois motores escritos de forma
independente fazendo aritmética de ponto flutuante logicamente equivalente mas não bit-idêntica
(ordem de operação diferente, compilador diferente, possivelmente FMA de um lado e não do outro).
Uma comparação `==` byte-exata entre dois floats computados por caminhos de código diferentes vai
ocasionalmente discordar no último bit ou dois da mantissa de um `float32` **mesmo quando os dois
motores estão corretos** -- isso é ruído de ULP, não bug. O oráculo precisa de uma regra única, fixa
**antes** de qualquer um dos lados ser codificado, que um bug real (fórmula errada, base errada,
sinal errado, errado por uma porcentagem, errado por uma unidade inteira) reprove de forma confiável
e que ruído de ULP passe de forma confiável -- e tem que imprimir de forma idêntica pras duas
implementações independentes de "a mesma regra", o que descarta qualquer coisa que dependa de estado
de arredondamento interno de biblioteca que um segundo implementer teria de adivinhar em vez de ler.

**A regra, declarada como algoritmo explícito (não "use `%.4f`", que é sub-especificado -- ver
abaixo por quê):**

```
quantize(x: float32) -> string:
    d := (double)x                      // amplia uma vez, pra computar os dígitos decimais com precisão
    scaled := d * 10000.0
    rounded := trunc(scaled + copysign(0.5, scaled))   // arredonda-meio-pra-longe-de-zero, explícito
    q := rounded / 10000.0
    if q == 0.0: q := 0.0                // canonicaliza -0.0 pra 0.0 (nunca imprime "-0.0000")
    return string decimal ponto-fixo de q, exatamente 4 dígitos depois de '.', sem expoente,
           prefixo '-' sse q < 0, sem prefixo '+', '.' como separador decimal sempre (nunca dependente de locale)
```

Aplicada a **todo** campo de valor float que este dump imprime: comprimentos resolvidos (px),
ângulos (graus, seção 8.2), números sem unidade, opacidade, a própria parte numérica de
porcentagens simbólicas, e todo argumento numérico dentro de um valor composto (seção 9). Cores já
são inteiros (seção 7.1) e são isentas -- não precisam de quantização, só da forma hex canônica.

**`quantize()` é definido só pra `x` finito (`UIX-RCSS-ERRATA-2`, fechando o Achado J):** um valor
computado não-finito (`NaN`, `+Inf`, `-Inf`) é tratado igual a qualquer outro erro de computação
detectado internamente: nunca é impresso por este algoritmo -- loga e cai pro próprio valor inicial
da propriedade (seção 6.1) ou seu valor herdado, a mesma consequência que a seção 11 já define pra
uma declaração rejeitada. Isto é a própria política geral de fail-high deste documento aplicada a um
resultado de *computação* numérica em vez de um resultado de *parse* -- nomeado explicitamente porque
a própria cultura de revisão deste projeto pratica teste adversarial/de mutação que fabrica entradas
que um corpus nunca produziria, e o algoritmo acima (`trunc`, `copysign`, formatação ponto-fixo) não
tem comportamento definido pra entrada não-finita de outro jeito.

**Por que 4 dígitos decimais, escolhidos e não apenas herdados por default:** `float32` carrega
aproximadamente 7 dígitos decimais significativos. Todo comprimento medido no corpus fica na faixa
0-3000 (**corrigido, `UIX-RCSS-ERRATA-2`, cosmético -- o maior valor único citado aqui era antes
`-228dp`; um censo regenerado (`tools/rcss_census.py`) mediu `999dp` (`border-radius`, o idioma
"totalmente arredondado" usado por todo o corpus) e `-410dp` (`margin-left`) como maiores em
magnitude; o exemplo de maior-valor-observado corrigido é `999dp`, ainda 3 dígitos antes do ponto**);
nessa magnitude, 4 dígitos depois do ponto é **mais**
precisão do que `float32` sequer consegue representar de forma significativa (aproximadamente 3-4
dígitos significativos restam abaixo do piso de ruído nessa magnitude) -- então discordância genuína
de nível-ULP entre dois motores corretos é arredondada fora por este passo pra faixa de valor
realista inteira, enquanto qualquer bug real (uma conversão de unidade errada erra por um fator
multiplicativo, uma base de porcentagem errada erra pela base real que for, uma inversão de sinal
erra por 2×o valor) produz uma diferença muitas ordens de grandeza maior que `0.0001` e sobrevive à
quantização intocada. Isto espelha a própria preferência declarada do líder sobre a proposta do CTO
de tolerância-dentro-do-harness: a própria comparação continua um `==` byte puro, e o "perdão" mora
inteiramente num único passo de arredondamento compartilhado e escrito, que os dois lados rodam
**antes** de imprimir -- nunca como lógica de tolerância em runtime que o harness aplica depois do
fato.

**Por que arredonda-meio-pra-longe-de-zero, explicitamente, em vez de "o que quer que `printf("%.4f",
...)` faça":** o arredondamento de `%f` da libc é bem definido pro valor *atual* (corretamente
arredondado por IEEE 754 em toda libc que este projeto alveja), então `%.4f` na prática concordaria
com o algoritmo explícito acima pra maioria esmagadora dos inputs -- mas "concorda na prática, por
enquanto, nestas plataformas" é precisamente o tipo de suposição não-declarada-mas-geralmente-verdadeira
que a própria seção "por que este documento existe" do `docs/uix-dom.md` avisa que produz
divergências espúrias no momento em que dois dumpers rodam sob libcs diferentes (glibc vs. musl, por
exemplo) ou configurações de locale diferentes que mudam em silêncio a renderização do ponto
decimal. Declarar o algoritmo neste documento, independente de qualquer função de formatação
específica da biblioteca padrão C, significa que um dumper conforme em qualquer linguagem, em
qualquer libc, produz a string idêntica -- o mesmo raciocínio que a seção 7 do `docs/uix-dom.md` dá
pra escolher ordenação byte-wise em vez de colação locale-aware.

#### 8.1 Por que a unidade impressa de todo comprimento resolvido é sempre `px`, nunca a unidade-fonte

Um comprimento declarado `2dp`, resolvido através de `dp_ratio`, e um comprimento declarado `16px`
descrevem o mesmo *tipo* de fato computado (uma contagem de pixel físico) uma vez resolvidos --
imprimir a unidade-fonte original (`dp` vs. `px`) junto do número resolvido deixaria dois valores
computados semanticamente idênticos (digamos, `2dp` com `dp_ratio=8` e `16px` escrito direto)
produzirem linhas de dump diferentes, e o oráculo reportaria uma divergência que não é uma,
exatamente o modo de falha que a seção 7 do `docs/uix-dom.md` nomeia pra por que tokens de `class`
são dumpados como um conjunto ordenado em vez de na ordem-fonte. `px` é a unidade de saída fixa e
única pra todo membro da família de unidade `LENGTH` (seção 9 de
`examples/RmlUi/Include/RmlUi/Core/Unit.h`: `PX | DP | VW | VH | EM | REM | PPI_UNIT`) depois da
resolução.

#### 8.2 Ângulos: canonicalizados pra graus, sempre

`transform`/ângulos de gradiente aceitam `deg` e `rad` como unidade de entrada (seção 9 abaixo,
paridade completa de unidade); este dump sempre imprime o ângulo resolvido em **graus** (valores
`RAD` convertidos via `graus = radianos * (180 / π)` antes da quantização), pelo mesmo raciocínio de
"uma forma de saída canônica só" da seção 8.1 -- `deg` é escolhido sobre `rad` porque 100% do uso de
ângulo do corpus (`/var/tmp/censo-rcss-qa1/censo.md` seção 5, 24 instâncias, todas `deg`, 0 `rad`) já
é autorado em graus, e a irracionalidade de `π` significa que converter um valor `deg` pra `rad` e de
volta introduz seu próprio ruído pequeno de arredondamento que converter o mais raro `rad` de entrada
pra `deg` evita pro caso esmagadoramente comum.

**Forma de impressão, uma lacuna que este documento não declarava antes e fecha aqui:** um ângulo
imprime como a própria saída crua do `quantize()` (seção 8), **sem sufixo de unidade** -- diferente
de `length`, que ganha um sufixo `px` explícito, adicional, sobreposto ao `quantize()` pela própria
regra da seção 8.1. O próprio título desta seção só dizia que ângulos são *canonicalizados* pra graus
(qual unidade o número significa); nunca dizia se a string impressa carrega o próprio nome dessa
unidade. Deixado sem declarar, um segundo implementer tem duas leituras igualmente plausíveis --
copiar o precedente do sufixo `px` e imprimir `90.0000deg`, ou confiar no próprio contrato literal do
`quantize()` ("sem expoente... sem unidade" nunca é sobreposto pra ângulos do jeito que é
explicitamente pra length) e imprimir o número cru `90.0000`. Este documento resolve como a forma
crua, porque é isso que o próprio algoritmo da seção 8 já produz sem nenhuma sobreposição específica
de ângulo existindo em lugar nenhum deste documento -- `length` é o único domínio que ganha uma regra
explícita de sufixo; ângulo não tem nenhuma, então cai na própria saída não-modificada do
`quantize()`. A seção 15.3 abaixo imprime um ângulo dentro de um exemplo trabalhado de
`linear-gradient(...)`, usando esta regra, pra um segundo implementer ter uma âncora byte-exata em
vez de só esta prosa.

### 9. Gramática de serialização de valor composto

*(seções 9.1-9.4, 9.2.1 -- mesmo conteúdo técnico, tabelas e exemplos trabalhados da versão em
inglês acima; gramáticas, nomes de função e valores de exemplo são identificadores/dados técnicos,
não traduzidos.)* Todo separador de lista externo é `|`, todo separador de argumento interno de uma
função é `;`, todo separador cor/posição dentro de um stop de gradiente é `:` -- os três
deliberadamente distintos da `,` que a própria fonte RCSS usa pros dois papéis, pra uma linha de
dump canônica nunca ficar ambígua sobre qual papel-de-vírgula um byte tinha na fonte.

### 10. `@font-face` e `@keyframes`: registros estruturais, não propriedades de elemento

**`@font-face`** carrega o próprio conjunto de atributos (`font-family`, `src`, e o
`-rmlui-fallback-face` relevante pra glintfx) que **não** faz parte do registro `PROP` por-nó acima
-- esses atributos descrevem um recurso de fonte, nunca um `Rml::Element`, e não têm cascata, não
têm herança, não têm especificidade (`examples/RmlUi/Source/Core/StyleSheetParser.cpp:306`, o
próprio registro do `-rmlui-fallback-face`, é uma propriedade de escopo `FontFaceId`, um espaço de
ID estruturalmente separado do `PropertyId` de cascata-de-elemento em que o registro da seção 6
deste documento vive). Este dump não emite linha `PROP` nenhuma pra bloco `@font-face`; um futuro
dump de recurso-de-fonte adjacente-à-`RMLX-2` (se algum dia for preciso) é, pela própria disciplina
da seção 1 deste documento, uma decisão nova, não uma extensão silenciosa deste formato.

**`@keyframes`** blocos são parseados e a própria existência/estrutura está no escopo (a própria
propriedade `animation`, seção 9.3, referencia um nome de keyframes), mas a **porcentagem de
seletor de keyframe** (`0%`, `50%`, `100%`, `from`/`to`) é um **quarto significado, distinto**, de
`%` além das três famílias da seção 5 -- um *offset de tempo* ao longo da própria timeline de uma
animação, nem um pouco uma quantidade espacial/de-gradiente. **Este dump não tenta resolver nem
interpolar valores de keyframe** -- fazer isso precisa de uma instância de animação ativa com um
relógio rodando, o que é preocupação de runtime da `RMLX-8`, não desta onda de cascata estática.
`from`/`to` são equivalentes a `0%`/`100%` (pela própria convenção do CSS, não re-derivada de forma
independente aqui) e são registrados por completude, não exercitados pelo oráculo desta onda.

### 11. Política fail-high (propriedade desconhecida, forma de seletor desconhecida, função de decorator desconhecida, valor fora de faixa)

**Uma regra só, aplicada uniformemente, já o padrão canonizado pro próprio range de `sides` do
`polygon()` (`docs/effects.md`):** uma construção não-reconhecida ou inválida é **logada e
ignorada**, nunca uma falha de parse dura que envenena o resto da folha, nunca um chute silencioso.
Concretamente:

- **Nome de propriedade desconhecido** (não no registro da seção 6.1): a declaração é descartada;
  toda *outra* declaração no mesmo bloco de regra continua se aplicando.
- **Forma de seletor desconhecida** (qualquer coisa fora do escopo desta onda -- restated aqui pro
  próprio escopo de seletor desta onda: `class`/`id`/descendente/filho (`>`)/tag/composto/
  lista-vírgula/`:hover` pela decisão 1 do líder, entrada `RMLX-2` do `TODO.md` -- `nth-child`,
  `:not(`, seletores de atributo, combinadores irmão, e toda pseudo-classe além de
  `:hover`/`:focus`/`:active` estão todos **fora de escopo**, batendo com os próprios cortes de zero
  real da seção 2 do `docs/rmlx-subset.md`): **corrigido, `UIX-RCSS-ERRATA-2` (Achado G,
  reverificado direto) -- só o seletor individual que falha ao registrar é descartado, não a regra
  inteira.** `StyleSheetParser::ConstructNodes` (`StyleSheetParser.cpp:947-967`) resolve cada
  seletor separado por vírgula de forma independente (`for (const String& selector : selector_list)
  { ... if (!leaf_node) Log::Message(...); else ... leaf_nodes.push_back(...); }`) -- um seletor que
  falha loga um aviso e o laço **continua**; nunca retorna cedo, nunca descarta os `leaf_nodes` já
  coletados, nunca toca as próprias declarações de propriedade da regra. As declarações da regra se
  aplicam a todo seletor que **de fato** resolveu, independente de qualquer seletor irmão da mesma
  lista-vírgula que falhou. "A regra inteira falha" só é verdade pra um seletor **sem** lista-vírgula
  que é ele mesmo inválido (nada mais na lista pra cair como fallback). Isto toca direto a própria
  evidência de lista-vírgula do líder (15 instâncias de corpus, incluindo a regra base de 16-tags da
  UA-stylesheet, `docs/rmlx-subset.md` §6.1) -- um único nome de tag com erro de digitação naquela
  regra perde só aquela tag sob o RmlUi real, não a regra inteira.
- **Nome de função de decorator/filter desconhecido**, ou uma função conhecida com a forma errada
  de argumento: **corrigido, `UIX-RCSS-ERRATA-2` (Achado C, reverificado direto) -- a *propriedade
  inteira* reverte, não só aquela entrada.** `PropertyParserDecorator::ParseValue`
  (`PropertyParserDecorator.cpp:63-131`) e `PropertyParserFilter::ParseValue`
  (`PropertyParserFilter.cpp:29-90`) os dois iteram a própria lista de valor-de-função dividida
  (seção 9.2 -- vírgula pra `decorator`/`mask-image`, espaço pra `filter`/`backdrop-filter`) e, na
  **primeira** keyword inválida, nome de função desconhecido, ou falha de parse de shorthand dentro
  de uma entrada, `return false` **imediatamente** -- `property.value`/`property.unit` nunca são
  atribuídos. O chamador só chama `dictionary.SetProperty` **depois** de um `ParseValue`
  bem-sucedido, então na falha nada é escrito e a propriedade cai, por completo, pro próprio valor
  inicial de registro/cascata (`none`). Não sobra lista parcial nenhuma -- **toda** entrada daquela
  lista é descartada, inclusive as que individualmente teriam parseado bem. Esta é a **mesma**
  consequência já declarada corretamente abaixo pra um shorthand malformado; os dois bullets agora
  usam a mesma redação, não redações opostas.
- **Valor numérico fora de faixa** pra propriedade/argumento com faixa declarada (`sides ∈ [3,
  1024]` do `polygon()`, `image-tint-threshold ∈ [0, 0.999]`, `opacity ∈ [0, 1]`): o valor clampa
  pro limite mais próximo **só onde a seção 6.1/9 declara um clamp**; onde nenhum clamp é declarado,
  a declaração/entrada-de-decorator inteira é descartada pelas duas regras acima, nunca clampada em
  silêncio por invenção.
- **Valor de shorthand malformado** (um nome de shorthand reconhecido, seção 6.2, cujo próprio valor
  cru tem contagem ou forma de token que não cabe em nenhuma das formas aceitas daquele shorthand --
  ex.: a própria cadeia de 2 tokens de `border-top` dada na ordem revertida, color-depois-width, per
  a nota de errata da própria seção 6.2): a mesma consequência de uma propriedade desconhecida, no
  sentido de que `ParseShorthandDeclaration` **retorna** `false` e a declaração é logada como
  rejeitada -- mas **corrigido, `UIX-RCSS-ERRATA-2` (Achado A, reverificado direto): o valor de
  retorno não significa que todo longhand alvejado reverte.** As chamadas `SetProperty` do upstream
  acontecem **dentro** do laço de parse, in place, sem buffer de staging e sem rollback na falha
  posterior do laço (a própria segunda correção da seção 6.2 tem o rastro byte-por-byte completo) --
  qualquer longhand que **já tinha casado com sucesso antes da falha da pós-condição do laço
  disparar** mantém aquele valor casado da fonte; só o longhand que **nunca** casou naquela chamada
  cai pro fallback de cascata/registro-inicial. Pra um shorthand cujo próprio algoritmo é ele mesmo
  composto de sub-shorthands (o `RecursiveRepeat` de `border`, seção 6.2): cada uma das 4
  sub-chamadas de side-shorthand roda até o fim de forma independente (`result &= ...`,
  `PropertySpecification.cpp:375-388`, sem saída antecipada no laço) **antes** do `result=false`
  agregado aparecer -- então a própria escrita-parcial de cada lado (pelo parágrafo acima) já
  aconteceu pros 4 lados no momento em que a chamada externa reporta falha. O resultado **logado,
  reportado** continua sendo "declaração rejeitada" pros dois casos -- o que mudou é quais longhands
  o dicionário de fato termina segurando quando aquela rejeição é reportada, que é o fato que as
  próprias linhas `PROP` deste dump tornam visível e byte-comparável. **🔵 Escopo, estreitado pela
  `UIX-RCSS-ERRATA-5`, 2026-08-06: a consequência de escrita-parcial acima é do próprio lado A
  (código upstream real) e não é mais, pela própria decisão do líder, do lado B -- o lado B descarta
  a declaração inteira, atomicamente, em vez disso. Ver a seção 14.1, "Divergências deliberadas",
  pro par byte-exato e a obrigação de fixture que esta discrepância permanente lado A ≠ lado B
  carrega.**
- **Valor de palavra-chave não-reconhecido** pra uma propriedade cujo domínio de valor (seção 6.1, o
  domínio único ou a própria alternativa palavra-chave de uma linha de dois domínios) é `Keyword` --
  um token identificador bem-formado sintaticamente que não é membro do conjunto aceito *daquela
  propriedade específica* (`display: blocc;`, um caractere de diferença de `block`, é o exemplo
  reportado): **acrescentado, `UIX-RCSS-ERRATA-5`, 2026-08-06, fechando uma lacuna que os cinco casos
  originais acima nunca nomearam.** A declaração é descartada por completo -- a mesma consequência já
  declarada pra um nome de propriedade desconhecido, o primeiro bullet acima -- **não** uma variante
  do caso de escrita-parcial de shorthand malformado dois bullets acima, porque é um caminho de
  código upstream inteiramente diferente, sem mutação dentro de laço nenhuma pra se preocupar:
  `PropertyParserKeyword::ParseValue`
  (`examples/RmlUi/Source/Core/PropertyParserKeyword.cpp:9-19`) retorna `false` num miss de busca
  contra o próprio `ParameterMap`; `PropertyDefinition::ParseValue` (`PropertyDefinition.cpp:75-89`)
  itera todo parser registrado pra aquela propriedade (o parser de keyword, mais, pra uma linha de
  dois domínios, o parser de length-percent/length/string) e só retorna `false` quando **nenhum**
  deles aceita o token; o chamador de propriedade única,
  `PropertySpecification::ParsePropertyDeclaration`
  (`PropertySpecification.cpp:255-282` -- o caminho *não*-shorthand,
  contraste com o próprio laço de `ParseShorthandDeclaration` que o bullet acima descreve), chama
  `dictionary.SetProperty` **só depois** de um `ParseValue` bem-sucedido (`:277-280`) -- na falha,
  `SetProperty` nunca é alcançado, então não há mutação parcial de dicionário nenhuma a considerar
  aqui. A propriedade computa pro próprio valor herdado (se `inherited: true` e um ancestral fornecer
  um) ou pro próprio valor inicial de registro da seção 6.1 -- o mesmo resultado que o próprio
  parágrafo de fechamento desta seção já declara pra todo caso fail-high; este é o sexto gatilho pra
  isso, não uma sétima consequência nova. **De onde vem o conjunto aceito:** a própria tabela de
  registro da seção 6.1 é a fonte única de verdade pro próprio conjunto de membros legal de toda
  propriedade de domínio `Keyword`, nomeado inline na própria célula "Value domain" daquela tabela
  (`keyword(`a`,`b`,...)`) -- a convenção já usada, antes desta errata, pras 18 linhas cuja
  alternativa palavra-chave era uma palavra só ou um conjunto curto enumerado; esta errata só estende
  essa mesma convenção pras 11 linhas que carregavam a palavra nua "keyword" sem lista de membros (o
  próprio parágrafo novo da seção 6.1 acima nomeia as 11 e a própria citação upstream). **Este caso é
  simétrico lado-A/lado-B, diferente do bullet acima** -- os dois dumpers rejeitam o mesmo token
  não-reconhecido do mesmo jeito, então o próprio ledger da seção 14.1 não ganha entrada nenhuma por
  causa dele.
- **Formato de log, conteúdo mínimo:** o texto cru da construção rejeitada, e o arquivo/linha se
  disponível -- nunca um "RCSS inválido" nu sem causa localizável, o mesmo padrão que o próprio
  relatório `file:line` do `check_rml_whitelist.sh` fixa pra este código-base (seção 4 do
  `docs/rmlx-subset.md`).

Este formato de dump **não tem registro especial** pra um evento fail-high além da própria
consequência ordinária: a propriedade em questão simplesmente imprime seu valor inicial declarado
no registro (seção 6.1) ou seu valor herdado, porque esse **é** o valor computado correto uma vez
que a declaração inválida é descartada -- um dumper não precisa de uma linha marcadora distinta
"REJEITADO", porque o valor inicial/herdado naquele caminho já prova, estruturalmente, que nada mais
venceu a cascata pra aquela propriedade.

### 12. Fora deste dump / não é trabalho desta onda

Restated pra quem lê só este arquivo ter a fronteira completa: **porcentagens box-relativas
resolvidas por layout, geometria de caixa, estado real (não-forçado) de foco/hover/active dirigido
por eventos de input de verdade, interpolação de timeline de keyframe, e composição de
transform-pra-matriz-de-render** estão todos fora do escopo da `RMLX-2`. Pertencem à `RMLX-3`
(layout), `RMLX-5` (eventos/foco), `RMLX-8` (animação), e `RMLX-8`/`RMLX-9` (aplicação de transform)
respectivamente -- nenhum deles é fato de cascata, e um futuro formato de dump pra qualquer um deles
é uma decisão nova, tomada do mesmo jeito que esta foi (spec primeiro, aval do líder, só então
código), nunca uma extensão silenciosa deste arquivo, espelhando a cláusula idêntica da seção 10 do
`docs/uix-dom.md` pra própria fronteira.

### 13. Explicitamente fora de escopo hoje -- exige aval do líder antes de qualquer implementação

Pela própria cláusula de cabeçalho deste documento e a do `docs/rmlx-subset.md`: **parar, editar
esta spec com um diff, pegar o aval do líder, só então implementar.** Nomeado aqui pra um futuro
implementer não ter de redescobrir cada um fixture por fixture:

- **Cor:** toda cor nomeada do RmlUi além de `transparent`/`white` (mais 17 entradas,
  `PropertyParserColour.cpp:117-136`), e toda forma funcional de cor (`rgb()`, `rgba()`, `hsl()`,
  `lab()`, `lch()`, `oklab()`, `oklch()`) -- zero-medida, e a decisão de paridade-de-unidades do
  líder nomeou unidades, não sintaxe de cor, então isso nunca foi autorizado implicitamente.
- **`transition`, `font-effect`:** as duas são propriedades reais e registradas do RmlUi
  (`StyleSheetSpecification.cpp:399`, `:405`) mas com **zero medição** no censo -- excluídas do
  registro da seção 6.1 na mesma base de "zero real é corte real" que a seção 2 do
  `docs/rmlx-subset.md` usa pra `nth-child`/`:not`/`z-index`, **não** dobradas em silêncio só porque
  por acaso compartilham maquinaria com `animation`/`filter`.
- **`z-index`, `nth-child`, `:not(`, seletores de atributo (`[x]`), combinadores irmão (`+`/`~`),
  seletor universal (`*`):** já nomeados como cortes de zero real pelo próprio `docs/rmlx-subset.md`;
  restated aqui pra este documento sozinho (pela própria promessa do cabeçalho dele) declarar a
  fronteira de seletor completa sem exigir que um leitor também tenha aquele documento aberto.
- **Funções de `transform` 3D/matriz** (`translate3d`, `matrix`, `perspective`, ...): o próprio
  limite declarado da seção 9.4 -- o subconjunto 2D é verificado contra as 2 instâncias do corpus,
  nada além disso.
- **A forma `ellipse` do `radial-gradient`:** só `circle` está no escopo pela própria gramática já
  entregue do `docs/effects.md`; um fixture com `ellipse` precisaria do aval deste documento antes
  de um dumper poder aceitar, não meramente uma mudança de código no `docs/effects.md`.
- **Propriedades nativas do RmlUi não-medidas fora da seção 6.1** (uma amostra não-exaustiva,
  nomeada porque são as que um leitor familiarizado com o RmlUi upstream completo poderia esperar e
  razoavelmente supor incluídas): `font-style`, `font-weight` (medidas **zero** no censo apesar de
  `font-family`/`font-size` serem muito usadas -- sinalizadas explicitamente já que a ausência delas
  é fácil de passar batido), `caret-color`, `image-color`, `visibility`, `clip`, `float`, `clear`,
  `drag`, `nav-*`, `pointer-events`, `perspective*`, `transform-origin*`, `-rmlui-language`,
  `-rmlui-direction`, `word-break`, `text-decoration`, `font-kerning`, `scrollbar-margin`,
  `overscroll-behavior`.

### 14. Registro de divergências (divergence ledger)

**Mesmo esquema de três classes da seção 9 do `docs/uix-dom.md`, mesmo limiar de escalonamento
(~10 linhas classe-(b) significa reler as seções 1-13, não continuar remendando), mesma divisão
entre "rotina de somar" (qualquer implementer, no momento em que acha uma divergência real) e "exige
aval do líder" (só uma mudança nas seções 1-13 acima, o próprio contrato de formato).** Restated de
forma breve em vez de copiado por completo -- a seção 9 do `docs/uix-dom.md` é a descrição canônica
das três classes (a: bug nosso, conserta o dumper; b: normalização esperada do RmlUi/glintfx, loga
independente de cobertura de corpus; c: construção fora-de-escopo, PARAR) e este ledger herda esse
texto por referência, não por duplicação, pra evitar os dois documentos divergirem sobre o que
"classe b" significa.

| Data | Classe | Descrição | Fixture | Resolução |
| :--- | :---: | :--- | :--- | :--- |
| 2026-08-07 | b | Todo elemento `body`/raiz imprime `position=absolute` no lado A independente de qualquer RCSS -- o próprio construtor `ElementDocument` do RmlUi upstream (`examples/RmlUi/Source/Core/ElementDocument.cpp:137`, `SetProperty(PropertyId::Position, Property(Style::Position::Absolute))`) força isto como propriedade de INSTÂNCIA, contornando a cascata por inteiro (não é alcançável por folha de estilo nenhuma, UA ou autor). O lado B não tem caso-especial equivalente pra raiz do DOM, então sempre imprime o próprio inicial de registro `static`. Medido via rcss_dump_differential_oracle (`UIX-RCSS-ORACULO`) nas 33 fixtures de corpus dentro do escopo -- taxa de reprodução de 100%, `STATE none` e `STATE hover-all` igual. | Reproduz em literalmente toda fixture que este oráculo compara (ex. `glintfx/tests/min.rml`, `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml`) | Fechado em 2026-08-07 -- registrado como divergência deliberada da §14.1 (linha 1 abaixo): o lado B mantém o próprio inicial de registro `static` pra raiz da árvore; o `absolute` forçado como propriedade de instância do RmlUi NÃO é replicado (um caso-especial via UA-stylesheet seria incorreto, não só feio -- uma regra de autor pode sobrepor uma regra UA mas nunca a própria propriedade de instância do RmlUi, então os bytes divergiriam de novo assim que uma fixture setasse `position` na raiz; CSS real concorda com o lado B, a raiz é `static` por default e o initial containing block é decisão de tempo-de-layout, não de valor computado; verificado, não suposto, que nada em `glintfx/src/uix/` consome isto hoje -- nenhum módulo de layout existe ainda, `RMLX-3`). Decisão, modo autônomo: Caetano/CTO 2026-08-07, pendente de confirmação retroativa do líder (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`). |
| 2026-08-07 | a | Um comentário RCSS `/* ... */` precedido de QUALQUER whitespace (isto é, na própria linha, a forma de-longe mais comum no mundo real) não é reconhecido como comentário dentro de um bloco de declaração e é mesclado ao próprio trecho de NOME da PRÓXIMA declaração real, corrompendo ou descartando por inteiro aquela declaração. Causa raiz, rastreada a duas linhas exatas: o próprio `try_scan_comment()` do `glintfx/src/uix/style/lexer.cpp` (linha 78-79) exige que `starts_with_at(source_, pos_, "/*")` valha com ZERO tolerância a whitespace à esquerda, e o próprio laço do `scan_declaration()` (linha 199-238) chama `try_scan_comment()` no topo de toda iteração SEM antes pular whitespace -- então um comentário colado direto depois do `;`/`{` anterior (zero bytes entre eles) É reconhecido, mas o mesmo comentário na própria linha indentada (a forma real de todo autor de verdade) não é, e os bytes dele mais tudo até o `:` seguinte viram um "nome de propriedade" malformado só. Medido direto: `parse_stylesheet()` sobre o próprio RCSS real do `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` produz 12 `ParseDiagnostic`s da forma exata `unknown property name '/* <texto do comentário> */\n  <próximo-nome-de-propriedade-real>'`, e a declaração real correspondente (`width`, `box-shadow`, `border`, ...) fica silenciosamente ausente das próprias `declarations` daquela `Rule`, que o dumper do lado B então reporta no próprio valor inicial de registro dela -- explicando a maioria das próprias 102 linhas `PROP` divergentes desta fixture contra o lado A (que não tem lacuna nenhuma dessas; o próprio lexer real do RmlUi remove comentário independente do whitespace ao redor). | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` (12 reproduções reais-de-corpus; também presente na maioria das outras fixtures de corpus da `RMLX-1` que usam comentário RCSS inline, pelo próprio censo de corpus deste documento) | Fechado pelo commit `0d5f475` (`UIX-LEXER-COMENT-ESPACO`, 2026-08-07) -- um novo `Lexer::try_scan_comment_at_fresh_start()`, privado, espia passando por whitespace à esquerda só pra procurar um comentário; se achado, o whitespace nunca é materializado em token nenhum; se não achado, `pos_` reverte pra que o próprio scan subsequente do chamador comece exatamente no mesmo byte de antes -- zero mudança de comportamento pra qualquer outro formato de token. Medido antes/depois no mesmo oráculo: `gusworld_battle_cockpit.rml` 102 -> 12 linhas divergentes; o total das 33 fixtures 414 -> 232 linhas (-182, -44%). As 12 linhas que sobram nessa fixture são as três OUTRAS classes já conhecidas (`position` na raiz, `animation`, `image()` sem `cover`), zero relacionada a comentário. |
| 2026-08-07 | b | O próprio argumento `fill:` do decorator `polygon()` aceita um valor composto aninhado `radial-gradient(...)`/`linear-gradient(...)` (um decorator autoral do glintfx, que não faz parte nenhuma do RmlUi upstream) -- o próprio pipeline de dump do lado A (`glintfx/src/rml/rcss_dump.cpp:670-711`) dirige a maquinaria real de parsing de propriedade do RmlUi, que não tem gramática nenhuma pra uma chamada de função aninhada como sub-valor dentro da própria lista de argumentos de OUTRO decorator custom; a declaração `decorator:` inteira é REJEITADA em tempo de parse com o próprio aviso fail-high do RmlUi, `decorator entry type 'polygon' dropped (fail-high, section 11)`, e o dumper do lado A então imprime o próprio inicial de registro `none` pra aquela propriedade. O próprio parser clean-room do lado B não carrega limitação nenhuma dessas -- reconhece a função de gradiente aninhada e imprime o valor composto por inteiro. Medido via `rcss_dump_differential_oracle` (`UIX-RCSS-ORACULO`) nas 12 fixtures de corpus dentro do escopo que carregam esta declaração exata (`polygon(6, radial-gradient(...))`, cópia byte-idêntica nas 12) -- taxa de reprodução de 100% em cada uma, `STATE none` e `STATE hover-all` igual, 96 linhas divergentes no total (8 por fixture). Exemplo byte-exato (`body/1/0`): lado A `decorator=none`, lado B `decorator=polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%);0.0000)`. Reportado no próprio commit que entregou este oráculo (`354e81d`, achado 3), mas nunca persistido em lugar nenhum deste documento até agora -- fechar essa lacuna é a própria obrigação desta fatia (`UIX-ORACLE-CANON`). | `difficulty_menu__lista_hardcore_bloqueado.rml`, `difficulty_menu__splash_confirmacao.rml`, `save_load_menu__modo_carregar_dois_slots_ocupados.rml`, `save_load_menu__modo_salvar_com_autosave.rml`, `save_load_menu__modo_salvar_todos_vazios.rml`, `system_menu__config_audio_sliders.rml`, `system_menu__config_categorias.rml`, `system_menu__config_controles_tabela.rml`, `system_menu__confirmacao_menu_inicial.rml`, `system_menu__pause_raiz.rml`, `title_menu__com_save.rml`, `title_menu__sem_save.rml` | Fechado em 2026-08-07 -- registrado como divergência deliberada da §14.1 (linha 3 abaixo): a lacuna é do próprio lado A, permanente enquanto o lado A envolver a dependência RmlUi que este repo está aposentando, auto-documentada pela própria linha de log fail-high dela; o lado B permanece o lado completo, não rebaixado pra igualar. Decisão, modo autônomo: Caetano/CTO 2026-08-07, pendente de confirmação retroativa do líder (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`). |

#### 14.1 🔵 Divergências deliberadas (lado A ≠ lado B, permanente, por decisão do líder -- acrescentada pela `UIX-RCSS-ERRATA-5`, 2026-08-06)

**O que esta seção é, e por que não é o ledger acima.** As próprias três classes (a/b/c) da seção 14
compartilham uma suposição: espera-se que o lado A e o lado B **eventualmente concordem** em toda
fixture -- a classe (a) é consertada corrigindo o dumper errado, a classe (b) é consertada ensinando
aos dois lados a mesma normalização real-do-upstream, a classe (c) para a implementação até o escopo
ser resolvido, precisamente porque concordância continua sendo o objetivo assim que o líder dá aval.
**Esta seção rastreia o fato oposto: uma discrepância de byte que o líder decidiu MANTER,
permanentemente, pra uma entrada nomeada -- o lado B não está errado, o lado A não está errado, os
dois estão implementando corretamente duas regras diferentes de propósito.** Registrar uma entrada
dessas no próprio ledger da seção 14 corromperia o próprio sinal de escalonamento daquele ledger (o
limiar "~10 linhas classe-(b) significa que algo está sistemicamente errado", seção 14 acima) -- uma
discrepância permanente e intencional é evidência de que nada está dando errado, e não pode contar
pra um sinal que existe pra detectar exatamente isso.

**Divergências deliberadas: 4.** (Contagem mantida no topo desta seção, nas duas línguas,
precisamente pra que o crescimento aqui seja visível e incômodo -- ver o próximo parágrafo pro
porquê desse incômodo ser o ponto.)

**⚠️ Por que esta lista precisa ficar curta, e por que toda linha é perigosa por construção.** Até
hoje, a própria regra de trabalho deste documento era simples: qualquer diferença de byte lado
A/lado B é um defeito, em algum lugar, ponto final. A própria decisão do líder sobre `border-top`
(§6.2, §11, §15.2) abre a primeira exceção a essa regra -- e uma lista de exceção que cresce sem
disciplina **engole o próprio valor do oráculo inteiro**: todo futuro implementer que bater numa
divergência real e não-decidida agora tem uma terceira opção tentadora além de "consertar o lado A"
ou "consertar o lado B" -- "somar uma linha aqui e chamar de deliberada" -- e se essa porta abrir sem
atrito, o oráculo para de significar qualquer coisa. Os quatro requisitos abaixo são o atrito, e os
quatro são obrigatórios, não aspiracionais:

1. **Fonte única, lida, não embutida.** Esta tabela é o *único* lugar onde uma exceção lado A/lado B
   é declarada, pro oráculo inteiro da `RMLX-2`. **Requisito normativo pra futura fatia
   `UIX-RCSS-ORACULO` (ainda não é um item de tabela do `TODO.md` nesta errata):** o harness
   diferencial que faz o diff do dump do lado A contra o dump do lado B DEVE, pra toda discrepância de
   byte numa tripla `(caminho de nó, propriedade, STATE)` dada, consultar essa tripla contra as
   próprias linhas desta tabela *antes* de reportar uma falha -- uma discrepância que casa com uma
   linha aqui é esperada e NÃO DEVE falhar o harness; uma discrepância que não casa com linha nenhuma
   aqui DEVE falhar o harness. **O harness não é dono de lógica de exceção nenhuma própria** --
   nenhum `if` dentro do harness nomeando `border-top` ou qualquer outra propriedade, nenhuma
   skip-list hardcoded, nenhum comentário dizendo "diferença conhecida, ignorar". Uma exceção
   embutida no próprio código do harness é indistinguível de um bug que o harness por acaso não
   pega; uma exceção que exige editar *este* documento (com a mesma disciplina "parar, diff, aval do
   líder" que toda outra mudança de contrato deste documento já carrega, pela própria cláusula de
   cabeçalho deste documento) é auditável do mesmo jeito que toda outra regra aqui é.
2. **Quatro campos obrigatórios por linha**, nenhuma linha pode omitir nenhum deles: **(a)** o caso
   exato que diverge (RCSS-fonte + qual nó/estado, preciso o bastante pra um segundo implementer
   reconstruir a fixture exata sem chutar); **(b)** o byte exato que o lado A imprime; **(c)** o byte
   exato que o lado B imprime; **(d)** por que os dois lados deliberadamente discordam, **datado e
   atribuído ao líder por nome de data de decisão** -- nunca "o tech-lead decidiu" pra uma cisão de
   comportamento lado A/lado B, porque essa é uma chamada que o próprio cabeçalho deste documento
   reserva pro líder, a mesma linha de autoridade que toda outra decisão de julgamento de design
   deste documento (a sinalização de pré-multiplicação da `ERRATA-2`, a própria reversão dela na
   `ERRATA-4`) já segue.
3. **🔴 Toda exceção DEVE ser exercitada por pelo menos uma fixture, e o oráculo DEVE falhar se
   alguma exceção nunca for exercitada.** Isto não é burocracia. O próprio achado `SEED-GOLDEN-INERTE`
   do `TODO.md`, no mesmo dia em que esta errata foi escrita, mediu **três** fixtures golden nesta
   mesma onda `RMLX-2` que não alcançavam a condição que afirmavam cobrir -- um golden que nunca
   exercita o que diz exercitar é pior que golden nenhum, porque verde parece prova e não é. Uma
   *exceção* não-exercitada é o mesmo modo de falha um nível acima: deixaria uma regressão real se
   esconder atrás de uma linha que não descreve mais nenhum caminho de código que alguém roda.
   Concretamente, pra própria suíte de teste do harness do oráculo (`UIX-RCSS-ORACULO`, quando essa
   fatia for aberta): (i) toda linha desta tabela precisa de uma fixture nomeada, na coluna Fixture
   abaixo, que alimenta o caso exato daquela linha pelos dois dumpers; (ii) a própria suíte de
   meta-teste do harness precisa de uma passada de mutation testing por linha -- neutralizar a
   própria busca desta tabela pra aquela linha (de modo que o harness, só pra aquela linha, caia pra
   comparação de byte simples) e assertar que o próprio teste da fixture **fica vermelho** -- provando
   que a fixture de fato alcança o caminho de código que aquela linha desculpa, não só que a fixture
   existe; uma linha cuja neutralização não vira o próprio vermelho da fixture é um erro de build
   pra aquela fatia, não uma passagem silenciosa, a mesma disciplina de mutation testing que a própria
   regra da casa já aplica a todo outro guarda (`feedback_mutante_em_arquivo_nao_commitado`,
   `SEED-GOLDEN-INERTE`, os dois no `TODO.md`).
4. **A própria linha de contagem desta seção, acima, não é decorativa** -- é o custo visível de somar
   uma linha. Um futuro implementer propondo uma segunda entrada deveria sentir o mesmo atrito que o
   próprio parágrafo de limiar-de-escalonamento da seção 14 descreve pras linhas classe-(b): se esta
   tabela algum dia se aproximar até de um punhado de entradas, isso é um sinal de que o próprio
   processo de decisão-do-líder, não o formato deste documento, precisa de reexame -- uma lista longa
   de "os dois motores só discordam aqui, permanentemente" não é um estado final saudável pra um
   oráculo cujo propósito inteiro é provar que os dois motores concordam.

**Tabela do ledger.** Mesma disciplina de coluna da própria tabela da seção 14 acima (Fixture é um
caminho relativo a `glintfx/tests/`, ou `nenhuma ainda -- <como/quando uma fixture vai existir>` até
a fatia dona dela ser entregue), mais as duas colunas de byte próprias desta tabela.

| Data | Caso | Byte lado A | Byte lado B | Por quê (decisão do líder) | Fixture |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 2026-08-06 | `#b { border-top: #7A5A2E 1dp; }` (shorthand `border-top` de ordem revertida, `FallThrough`, §6.2) -- `body/1 PROP border-top-color`, `STATE none` | `border-top-color=#7a5a2eff` (a própria escrita parcial do upstream, dentro do laço, sem rollback, segunda correção da §6.2) | `border-top-color=#000000ff` (o próprio descarte atômico do lado B: a declaração inteira é derrubada, os dois longhands caem pro próprio valor inicial de registro da seção 6.1, `black`, default straight-alpha `ff` pela seção 7.1) | Decidido em 2026-08-06, líder: o descarte atômico do lado B é julgado o comportamento *correto* e fica; a própria escrita parcial do upstream é um artefato de mutação de dicionário sem rollback, não uma semântica CSS intencional que valha a pena replicar bug-por-bug num motor clean-room. Origem: `UIX-SHORTHAND-PARCIAL`/`UIX-RCSS-DUMP-B` (`TODO.md`). `border-top-width` **não** faz parte desta exceção -- os dois lados imprimem `border-top-width=0.0000px` pro `body/1`, em concordância, por motivos não-relacionados de cada lado (§15.2). | **Fixtures por lado existem e exercitam esta linha exata (`UIX-RCSS-CONFORMIDADE`, 2026-08-06)** -- `glintfx/src/rml/rcss_dump_worked_examples.cpp::test_15_2_shorthand_order_border_top` (lado A, pipeline real do RmlUi, assere `#7a5a2eff`) e `glintfx/tests/uix_style/dumper_sanity.cpp::test_worked_example_15_2_shorthand_order` (lado B, pipeline real lexer→parser→cascata, assere `#000000ff`) cada um alcança e imprime, independentemente, o próprio byte desta linha. **Ainda faltando:** o próprio harness DIFERENCIAL (`UIX-RCSS-ORACULO`, ainda não é item do `TODO.md`) que consultaria esta linha e suprimiria o mismatch automaticamente -- hoje as duas fixtures acima provam o próprio byte de cada lado isoladamente, não que um harness de diff-e-consulta-§14.1 já existe |
| 2026-08-07 | Qualquer elemento `body`/raiz de qualquer fixture de corpus dentro do escopo -- `position`, `STATE none` e `STATE hover-all` igual (o próprio construtor `ElementDocument` do upstream, `examples/RmlUi/Source/Core/ElementDocument.cpp:137`, força isto como propriedade de INSTÂNCIA fora da cascata -- ver a própria linha classe-(b) da seção 14 acima pro mecanismo completo) | `position=absolute` | `position=static` | Decidido em 2026-08-07, modo autônomo (Caetano/CTO), pendente de confirmação retroativa do líder (`PLANO-RMLX2-FECHO`, `TODO.md`) -- três razões, em ordem de força, todas medidas, não supostas: **(1)** replicar via uma regra UA-stylesheet seria INCORRETO, não só feio -- uma regra UA é sobrepujável por regra de autor, a própria propriedade de instância do RmlUi não é, então uma fixture futura com `body { position: relative; }` ainda divergiria (o lado A segue imprimindo `absolute` de qualquer forma, um lado B com regra UA imprimiria `relative`); a única replicação byte-fiel seria um caso-especial de instância pra raiz da árvore carimbado na cascata clean-room, carregando uma peculiaridade interna da própria dependência que este repo está aposentando. **(2)** CSS real concorda com o lado B -- a raiz é `static` por default e o initial containing block vem do viewport, decisão de LAYOUT, não de valor computado; quando a `RMLX-3` construir o próprio layout do motor, a raiz preenche o viewport pela própria regra do motor de layout, sem precisar de mentira nenhuma na cascata. **(3)** Verificado, não suposto: `glintfx/src/uix/` não tem módulo de layout nenhum hoje (`RMLX-3` não entregue), e um grep sobre `src/uix/**` por `position` acha só código de registry/shorthand/gradient-stop -- zero consumidor de layout existe que esta decisão poderia regredir. | Corpus-wide -- reproduz nas 33 fixtures dentro do escopo (mesma lista da própria linha da seção 14 acima); instância byte-exata canônica: `glintfx/tests/min.rml`, linha do `body` |
| 2026-08-07 | `decorator: polygon(6, fill: radial-gradient(...))` -- um composto de gradiente aninhado como o próprio argumento de preenchimento do decorator `polygon()`, qualquer nó/estado declarando a cópia exata de corpus (ver a própria linha da seção 14 acima pro mecanismo completo) | `decorator=none` | `decorator=polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%);0.0000)` | Decidido em 2026-08-07, modo autônomo (Caetano/CTO), pendente de confirmação retroativa do líder (`PLANO-RMLX2-FECHO`, `TODO.md`) -- a lacuna é do próprio lado A, permanente enquanto o lado A envolver uma dependência que este repo está aposentando, e auto-documentada (a própria linha de log fail-high dela nomeia o achado toda vez que dispara). Fechar isto no lado A significaria construir um segundo parser de valor composto dentro de um dumper que envolve uma biblioteca em rota de saída -- trabalho contra a própria estratégia de saída `R1` deste repo. Rebaixar o lado B pra igualar fraudaria o oráculo na direção inversa. Excluir estas 12 fixtures do próprio escopo do oráculo foi considerado e rejeitado -- esconderia as centenas de OUTRAS linhas `PROP` que essas mesmas fixtures carregam. | `difficulty_menu__lista_hardcore_bloqueado.rml`, `difficulty_menu__splash_confirmacao.rml`, `save_load_menu__modo_carregar_dois_slots_ocupados.rml`, `save_load_menu__modo_salvar_com_autosave.rml`, `save_load_menu__modo_salvar_todos_vazios.rml`, `system_menu__config_audio_sliders.rml`, `system_menu__config_categorias.rml`, `system_menu__config_controles_tabela.rml`, `system_menu__confirmacao_menu_inicial.rml`, `system_menu__pause_raiz.rml`, `title_menu__com_save.rml`, `title_menu__sem_save.rml` |
| 2026-08-07 | `decorator: image(<url> cover)` -- a palavra-chave de encaixe `cover` na própria forma composta impressa de um decorator `image()`, qualquer nó/estado que a carregue (5 nós únicos em 4 fixtures, medido pela `UIX-ORACLE-REMEDICAO`/`relatorio-B2.md`) | `decorator=image(retrato_seu_bertoldo_caim.png)` (o próprio serializador de `image()` do lado A descarta a palavra-chave de encaixe `cover` da própria forma impressa; nó de exemplo `body/1/4`) | `decorator=image(retrato_seu_bertoldo_caim.png cover)` (o lado B mantém o próprio token `cover` cru parseado na própria forma impressa) | Decidido em 2026-08-07, modo autônomo (Caetano/CTO), pendente de confirmação retroativa do líder (`PLANO-RMLX2-FECHO`/`UIX-ORACLE-CANON`, `TODO.md`) -- pré-existia só no `gusworld_battle_cockpit.rml` (2 nós) como resíduo MASCARADO, e foi DESMASCARADO, não introduzido, quando a `UIX-INLINE-STYLE` (`72f832c`) ensinou a cascata a ler o atributo `style="..."` pela primeira vez, expondo a mesma lacuna pré-existente em mais 3 fixtures `npc_dialogue__*` que a carregavam em silêncio como `decorator=none` (a declaração era descartada antes de sequer chegar no ponto onde `cover` importava; `UIX-ORACLE-REMEDICAO`, `TODO.md`). Registrada aqui como PERMANENTE só pros próprios propósitos do oráculo porque nenhuma onda possui o conserto hoje -- `UIX-IMAGE-COVER-AUSENTE` é uma semente de INBOX não-puxada, não uma onda agendada, então não pode levar uma linha "scheduled gap" da §14.2 (que exige onda dona). Se uma fatia futura puxar a `UIX-IMAGE-COVER-AUSENTE` e fizer o lado B igualar o próprio truncamento do lado A (ou vice-versa), esta linha é removida no MESMO commit que a fecha, pela própria disciplina desta seção. | `gusworld_battle_cockpit.rml` (2 nós, `body/0/0/0/3/0` e `body/0/1/0/0/0`, pré-existente), `npc_dialogue__no_com_3_escolhas.rml` (`body/1/4`), `npc_dialogue__no_linear_fala_curta.rml` (`body/1/4`), `npc_dialogue__no_linear_fala_longa.rml` (`body/1/4`) |

#### 14.2 🟣 Lacunas agendadas: pinadas até uma onda dona entregar (acrescentada pela `UIX-ORACLE-CANON`, 2026-08-07)

**O que esta seção é, e por que não é a §14.1.** A §14.1 acima é, por desenho próprio, SÓ pra
divergência permanente e deliberada -- uma discrepância de byte que ninguém pretende fechar. Esta
seção rastreia o tipo oposto de incompletude: o próprio dumper do lado B está sem uma computação que
VAI ter eventualmente, porque a propriedade que ela computaria está explicitamente fora do próprio
escopo DESTA onda (`RMLX-2`), não porque nenhum dos dois lados está se comportando corretamente de
propósito. Registrar uma lacuna temporária no próprio ledger da §14.1 envenenaria os dois: uma
entrada da §14.1 que depois precisa ser REMOVIDA (assim que a onda dona entregar) contradiz o próprio
contrato "permanente, por decisão do líder" da §14.1, e deixar a §14.1 carregar linhas temporárias
embotaria o desconforto que a própria linha de contagem da §14.1 é construída pra causar (a própria
§14.1 deste documento, requisito 4). **A expiração aqui é AUTOMÁTICA, não é tarefa de gente
lembrar:** a própria checagem de pin obsoleto do oráculo (já construída pra §14.1) se aplica de
forma idêntica a esta tabela -- no dia em que a onda dona entregar a computação faltante, o lado B
passa a imprimir o valor real, o byte pinado aqui para de casar, e o oráculo FALHA ALTO exigindo que
a linha seja removida no MESMO commit. Nenhum implementer precisa lembrar de limpar esta tabela; o
mecanismo força.

**Lacunas agendadas: 3.** (Contagem mantida no topo desta seção, nas duas línguas, mesma disciplina
da própria linha de contagem da §14.1 acima.)

**Tabela.** Os mesmos quatro campos obrigatórios que o próprio requisito 2 da §14.1 lista (caso,
byte lado A, byte lado B, por quê), com o próprio quarto subcampo do "por quê" substituído: no
lugar da própria cláusula "atribuído ao líder por nome" da §14.1, toda linha aqui nomeia a cerca já
desenhada pelo `docs/rmlx-subset.md` e a onda/item dona que deve esta lacuna -- uma lacuna agendada
não é uma chamada de julgamento de design, então o "por quê" aqui é cerca-mais-dono, não uma data de
decisão do líder.

| Data | Caso | Byte lado A | Byte lado B | Por quê (cerca + onda dona) | Fixture |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 2026-08-07 | Declaração `animation` de `body/0/0/0/1`, `STATE none` (`L369`) e `STATE hover-all` (`L4385`) igual | `animation=animation(linear;18.0000;linear-out;infinite;false;false)` | `animation=none` | O próprio dumper do lado B não tem `compute_animation` (`glintfx/src/uix/dom/dumper.cpp` linhas 49-55/121-128 -- a declaração cai pro próprio inicial de registro `none`); o `docs/rmlx-subset.md` congela o escopo de `animation`/`@keyframes` na `RMLX-8`, e implementar agora violaria essa cerca. Não é defeito, não é permanente -- incompletude AGENDADA, dona `RMLX-8` (`TODO.md`). | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |
| 2026-08-07 | Declaração `animation` de `body/0/0/3/0`, `STATE none` (`L880`) e `STATE hover-all` (`L4896`) igual | `animation=animation(step-start;1.1000;linear-out;infinite;false;false)` | `animation=none` | Igual à linha acima -- `compute_animation` não implementado, congelado pelo `docs/rmlx-subset.md` na `RMLX-8`. | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |
| 2026-08-07 | Declaração `animation` de `body/0/1/0/0`, `STATE none` (`L1099`) e `STATE hover-all` (`L5115`) igual | `animation=animation(infinite-alternate;2.4000;cubic-in-out;1;false;false)` | `animation=none` | Igual à linha acima -- `compute_animation` não implementado, congelado pelo `docs/rmlx-subset.md` na `RMLX-8`. | `glintfx/src/uix/dom/test_fixtures/gusworld_battle_cockpit.rml` |

3 nós únicos, 2 `STATE`s cada = 6 linhas divergentes, todas na única fixture deste corpus que usa
`animation` -- medido (`UIX-ORACLE-MEDICAO`/`UIX-ORACLE-REMEDICAO`, `TODO.md`), não suposto; um
rascunho de planejamento anterior (`PLANO-RMLX2-FECHO`, `/var/tmp/plano-rmlx2-fecho/PLANO.md`)
nomeava 4 nós/8 linhas antes da medição existir -- a medição corrigiu pra 3/6 e esta tabela segue a
medição, não o rascunho.

⚠️ **O próprio item `RMLX-8` do `TODO.md` já carrega a instrução de remoção: remova estas 3 linhas
no MESMO commit que implementar `compute_animation`, e baixe a própria linha de contagem desta seção
no mesmo commit -- a checagem de pin obsoleto vai se recusar a passar de outro jeito.**

### 15. Exemplos trabalhados (byte-exato)

**Quatro exemplos independentes abaixo (15.1-15.4), cada um ancorando um ponto em que a própria
prosa deste documento sozinha deixava espaço pra dois leitores caírem em bytes diferentes -- pelo
próprio princípio-guia desta seção, restated do cabeçalho: dois implementers independentes podem
concordar na mesma leitura que soa-correta-mas-é-errada de uma regra em prosa; não conseguem os dois
reproduzir a mesma resposta trabalhada byte-exata discordando sobre o que ela significa.**

#### 15.1 Dois estados, um nó (`:hover`)

*(mesmo exemplo em inglês acima -- fonte RCSS/RML, linhas de dump e as notas técnicas que as amarram
de volta às seções não são traduzidas; identificadores de campo, valores e nomes de arquivo são
dados técnicos.)*

#### 15.2 A ordem do shorthand é load-bearing (`border-top`), e lado A/lado B discordam permanentemente em `#b`

*(mesmo exemplo em inglês acima -- a própria âncora trabalhada da errata, seção 6.2, as duas ordens
lado a lado; identificadores de campo, valores e nomes de arquivo são dados técnicos, não
traduzidos.)* **🔵 Atualizado pela `UIX-RCSS-ERRATA-5`, 2026-08-06: a linha `body/1` deixou de ser
uma resposta única compartilhada entre os dois lados do oráculo -- ver a versão em inglês acima pro
par byte-exato completo, lado A (`border-top-color=#7a5a2eff`) contra lado B
(`border-top-color=#000000ff`), e a própria seção 14.1 abaixo pro registro normativo desta
divergência deliberada.**

#### 15.3 As três famílias de `%`, lado a lado

*(mesmo exemplo em inglês acima -- fonte RCSS/RML e linhas de dump não traduzidas; fecha a
ambiguidade de fusão (b)/(c) da seção 5 com uma âncora byte-exata, não só a decisão em prosa.)*

#### 15.4 Fronteira da quantização: empate exato e um passo pra fora

*(mesma tabela em inglês acima -- os quatro inputs abstratos ao `quantize()` (empate exato + um passo
pra fora, os dois sinais) não são traduzidos, são dados técnicos; ver a própria regra da casa citada
no cabeçalho da tabela: testar só o limite exato não basta, um limite alargado ainda contém a própria
borda.)*

### 16. Decisões de contrato que fecham ambiguidades reportadas por fatias anteriores da `RMLX-2`

**Escopo desta seção, declarado uma vez:** o próprio comentário de cabeçalho da `UIX-RCSS-LEXER`
(`glintfx/src/uix/style/lexer.hpp`) reportou duas ambiguidades que deliberadamente não resolveu
sozinha, cada uma afetando como uma futura fatia de parser construída sobre o fluxo de token daquele
lexer precisa se comportar ao produzir os valores computados que o formato de dump deste documento
reporta -- então, pela própria instrução desta tarefa de que uma ambiguidade não-resolvida não é
deste projeto pra deixar em aberto uma vez achada, são fechadas aqui, com os dois lados argumentados,
como decisões de contrato deste documento. (A terceira ambiguidade que esta tarefa nomeou -- as
famílias de porcentagem (b) e (c) -- já está fechada na seção 5 acima; não é repetida aqui, só
cross-referenciada, pra os dois documentos que a declaram não divergirem.)

#### 16.1 Comentários no meio de um trecho: token `Comment` diagnosticável vs. emenda-por-byte do upstream

**A própria escolha do lexer (já entregue, `UIX-RCSS-LEXER`):** `/* ... */` é o próprio token
`Comment`, reconhecido só num início-de-scan fresco -- logo depois de um delimitador estrutural,
logo depois de uma `Declaration` completa, ou bem no início de um dispatch de `Prelude`/`Comment`. O
próprio RmlUi upstream real elide comentários no nível de *caractere*, abaixo de toda máquina de
estado, então um comentário pode *emendar* dois trechos adjacentes com zero bytes entre eles
(`wid/*x*/th` tokeniza como o identificador `"width"`) independente de onde aparece -- meio de
identificador, meio de valor, em qualquer lugar.

**Argumento pro comportamento de emenda-por-byte do upstream:** é o que `Style::ComputedValues` (o
lado A do oráculo da `RMLX-2`) de fato faz, porque o lado A *é* código real do RmlUi -- casar
byte-a-byte fecha uma fonte de divergência antes que ela possa aparecer. **Argumento pra própria
escolha, mais estreita, do lexer (a decisão):** um token `Comment` diagnosticável é estritamente mais
útil pra toda fixture do corpus que existe hoje (comentários só aparecem em posições de
início-de-scan-fresco nos 62 arquivos do censo -- zero contra-exemplo), mantém a própria maquinaria
de relato-de-erro/hardening deste módulo uniforme com o desenho idêntico `<!-- -->` do irmão DOM, e
-- o motivo que mantém isto como contrato em vez de reabrir -- **os próprios campos `offset`/`length`
de todo `Token` são preservados exatamente pra uma futura fatia de parser ainda conseguir
reconstruir a própria leitura emendada-por-byte do upstream direto do buffer-fonte**, se uma fixture
real algum dia for achada em que isso importa (a própria cláusula de cabeçalho da seção 1: parar,
diff, aval do líder, só então implementar). **Decisão:** mantida como entregue. **Consequência pro
próprio registro de divergências deste documento (seção 14):** uma futura fixture com um comentário
aparecendo *no meio* de um trecho (dentro de um scan de `Prelude`/nome/valor já iniciado, não numa
fronteira de início-de-scan-fresco) é **pré-registrada aqui como classe (b)** (normalização esperada
RmlUi/glintfx, não um bug) no momento em que for achada -- um implementer que a encontrar não precisa
primeiro debater se é classe (a) ou (b), este documento já responde isso.

#### 16.2 Dispatch de modo guiado por nome-de-at-rule: nível-lexer vs. adiado pra uma futura fatia de parser

**A própria escolha do lexer (já entregue):** a decisão de se um `{` abre um bloco Declaration plano
ou uma segunda região Estrutural aninhada (precisa pra exatamente um caso medido no corpus,
`@keyframes`) é tomada **dentro do próprio lexer**, chaveada pela primeira palavra, case-sensitive,
do próprio `Prelude` que precede o at-rule. Toda *outra* questão semântica que este mesmo lexer toca
-- casamento de seletor, validação de valor de propriedade, registro de propriedade -- é
explicitamente adiada pra uma futura fatia de parser; o reconhecimento de `@keyframes` é a única
exceção, e o próprio cabeçalho da `UIX-RCSS-LEXER` sinalizou isso como uma questão real, estrutural,
de dois lados, em vez de obviamente-já-resolvida.

**Argumento pra adiar pra um futuro parser (a posição tomada em todo o resto deste mesmo lexer):**
mantém o lexer uma passada byte-pra-token puramente mecânica, livre-de-contexto, com zero
conhecimento do que qualquer at-rule *significa* -- a mesma fronteira traçada pra `.foo`/`#bar`/
`:hover` (carregados como bytes `Prelude` crus, não-parseados) e pra todo nome/valor de propriedade
(carregados como texto `Declaration` cru). Reconhecer a string literal `"keyframes"` é, à primeira
vista, exatamente o tipo de conhecimento semântico que o próprio cabeçalho deste módulo repete
várias vezes "não é trabalho desta camada". **Argumento pra manter no nível do lexer (a decisão):**
isto não é uma questão de validade-semântica, é uma **necessidade sintática pra sequer produzir um
fluxo de token correto** -- uma categoria diferente de todo caso adiado. Semântica de
seletor/propriedade sendo adiada não custa nada estrutural: um token `Prelude` não-parseado ainda é
um token completo, corretamente delimitado, independente de algo rio-abaixo entender `:hover` ou
não. Dispatch por nome-de-at-rule não é assim: sem saber que o at-rule é `@keyframes`, o lexer não
consegue distinguir um par `{`/`}` aninhado de um plano, e erra o **próprio balanço de chave** --
provado, não afirmado, pelo próprio `test_keyframes_special_case_is_load_bearing` do
`lexer_hardening_sanity.cpp`: sem o caso especial, `@keyframes spin { from { transform: rotate(0deg);
} to { transform: rotate(360deg); } } }` desincroniza, o primeiro `}` interno termina o scan inteiro
prematuramente, e o `}` externo fica pendurado -- não uma *interpretação* errada de tokens por outro
lado válidos, um **fluxo de token errado**, do qual nenhuma fatia de parser rio-abaixo conseguiria
recuperar uma árvore correta não importa como for desenhada. O próprio tokenizador do RmlUi upstream
real precisa e toma esta decisão idêntica na camada idêntica (o próprio ramo `State::AtRuleIdentifier`
do `StyleSheetParser::Parse`, `:786-789`) -- esta não é uma fronteira que este módulo inventou, é o
único lugar em que a própria divisão tokenizador-vs-parser do upstream já traça a linha do mesmo
jeito. **Decisão:** fica no nível do lexer, permanentemente, não só "por enquanto" -- o teto já está
declarado (só o literal, case-sensitive, `"keyframes"`; as próprias exclusões de zero-real da seção
13 do `docs/rmlx-subset.md` pra `@media`/`@import`/`@charset`/`@supports` recebem o default comum de
modo Declaration, e uma fixture real precisando de uma delas é o mesmo movimento "parar, somar um
nome nesta tabela", nunca um alargamento silencioso do default).
