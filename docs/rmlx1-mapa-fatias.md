# RMLX-1 slice map / Mapa de fatias da RMLX-1

> **EN:** The canonical registry of what each `RMLX-1` slice (`S0`..`S7`) delivers, which
> `S`-numbers were retired and never assigned, and the rule that replaces the `S`-numbering scheme
> going forward. Diátaxis type: **reference**. Audience: anyone reading `docs/uix-dom.md` or
> `TODO.md`'s `RMLX-1` row and needing to know what a slice ID refers to; whoever plans or reviews
> the remainder of the wave. Owner: `software-architect` (decision), written by `technical-writer`
> **2026-08-05** against `main` at `41814f1`. Last reviewed: **2026-08-05**.
> **PT:** O registro canônico do que cada fatia da `RMLX-1` (`S0`..`S7`) entrega, quais números `S`
> foram aposentados e nunca atribuídos, e a regra que substitui o esquema de numeração `S` daqui
> pra frente. Tipo Diátaxis: **reference**. Audiência: quem lê `docs/uix-dom.md` ou a linha
> `RMLX-1` do `TODO.md` e precisa saber a que um ID de fatia se refere; quem planeja ou revisa o
> resto da onda. Owner: `software-architect` (decisão), escrito por `technical-writer` em
> **2026-08-05** contra `main` em `41814f1`. Última revisão: **2026-08-05**.

**Cross-ref:** [`docs/uix-dom.md`](uix-dom.md) (o contrato de formato que `S6a`/`S6b`/`S7`
implementam), [`docs/rmlx-subset.md`](rmlx-subset.md) (a fronteira congelada de toda a arco
`RMLX-1..11`), `TODO.md` linha `RMLX-1` (escopo/aceite da onda) e linha `RMLX1-MAPA-FATIAS` (o
item que pediu este documento).

---

## Why this document exists / Por que este documento existe

**EN:** The `RMLX-1` slice plan was never written down anywhere durable. It lived only in one
session's context. As a consequence, `TODO.md` accumulated `S0`/`S1`/`S2` mentions (written when
each slice closed) and `docs/uix-dom.md` referenced `S6a`/`S6b`/`S7`/`S1-S9` in passing -- but no
single artifact ever said what the full set of slices was or what each one delivers. This is the
exact failure mode this house already has a name for: *hand-written documentation is
indistinguishable from correct documentation until someone runs the command.* This document is
that missing artifact, and every row below was checked with `git show --stat <sha>` against the
actual repository before being written, not copied from memory.

**PT:** O plano de fatias da `RMLX-1` nunca foi escrito em lugar nenhum durável. Ele existia só no
contexto de uma sessão. Como consequência, o `TODO.md` acumulou menções a `S0`/`S1`/`S2` (escritas
ao fechar cada fatia) e a `docs/uix-dom.md` referenciou `S6a`/`S6b`/`S7`/`S1-S9` de passagem --
mas nenhum artefato único jamais disse qual era o conjunto completo de fatias nem o que cada uma
entrega. Este é exatamente o padrão de falha que esta casa já nomeou: *documentação escrita de
cabeça é indistinguível de documentação correta, até alguém rodar o comando.* Este documento é o
artefato que faltava, e cada linha abaixo foi conferida com `git show --stat <sha>` contra o
repositório real antes de ser escrita, não copiada de memória.

## English

### The decision (CTO, not to be reopened here)

**`S5`, `S8`, `S9` and `S10` do not exist. They are retired, not redefined.**

The only mention of that range anywhere in the repository is the preamble of
`docs/uix-dom.md`, which reads *"before any of `S1-S9` ... exist"* -- written on the day the
document itself was authored, as an author's **estimate of reach**, not a definition of slices.
No commit, brief or `TODO.md` row ever assigned content to `S5`, `S8`, `S9` or `S10`. Redefining
them now would be inventing a past with the authority of a plan.

**Rule going forward: new work gets a named ID, never an `S`-number.** This is what
`UIX-ENTITY-PARIDADE`, `UIX-CLASS-SPLIT-SPEC`, `UIX-REMOVE-CHILD` and `UIX-LEXER-OPACO` already
do below -- it eliminates this entire class of process bug, because a named ID cannot be confused
with a slot in a numbering scheme that was never fully specified.

### The slice table

| Slice | Commit | What it delivers |
| :--- | :--- | :--- |
| `S0` | `7f84d30` | `docs/uix-dom.md` -- the spec of the canonical dump format, the shared contract between `S6a` and `S6b` |
| `S1` | `9ff2f78` | Lexer (`glintfx/src/uix/dom/lexer.*`) -- standalone tokenizer of the frozen RML subset, no tree, no parser |
| `S2` | `dfb9fd9` | Tree model (`dom_tree.*`) -- `Node`/`Text`/`Element`/`Document`, `find_by_id`, the whitespace-existence filter |
| `S3` | `6d6aebd` | Recursive-descent parser (`parser.*`) -- consumes `S1`'s tokens, builds `S2`'s tree |
| `S4` | `f15f1f8` | DOM API by id (`set_text`/`add_class`/`remove_class`) over `S2`'s tree |
| `S6b` | `273ec30` | Dumper over glintfx's own tree (`S2`), emitting the `S0` format |
| `S6a` | `3d13512` | Dumper over the real `Rml::ElementDocument`, confined to `glintfx/src/rml/`, emitting the `S0` format |
| `S7` | `e321f2a` | The differential-diff harness (the oracle) -- **delivered**. Measured 16 fixtures: **15 divergent, 0 parse errors, 1 defect**, all one mechanism, all on the `HEAD` line. Every other line is byte-identical in all 16 -- the two independently-written trees agree completely on structure, traversal order, child count, escaping, classes, attributes and text. The defect is in `S6a`'s own `dom_dump_scan_head()` (comment-unaware raw scan grabs the first literal `<head` in the file, which in 15 fixtures is prose inside the provenance comment, 819-1069 source-bytes before the real element). Control that proves it: `gusworld_battle_cockpit.rml`, the only fixture whose comment has no literal `<head`, is byte-IDENTICAL -- correlation 16/16. **Deliberately not fixed** (fixing a dumper to flatten what the oracle just found is defrauding the oracle); pinned by mechanism so it fails in BOTH directions, and the fix is its own item (`UIX-HEAD-COMENTARIO`) |
| `S5`, `S8`, `S9`, `S10` | -- | 🔴 **Never assigned -- retired.** Do not look for content; none was ever defined. See "The decision" above. |

Every SHA above was verified with `git show --stat <sha>` against `main` immediately before this
document was written; each commit's subject line matches the slice label it is listed under.

### Named items of the wave (not `S`-numbered by design)

- **`UIX-ENTITY-PARIDADE`** (`591d7b5`) -- entity-decoding parity fix
  against RmlUi's `DecodeRml`, found via the consumer's real corpus.
- **`UIX-CLASS-SPLIT-SPEC`** -- a resolution shared by both dumpers landed in one dumper's header
  instead of the spec; open, tracked in `TODO.md`.
- **`UIX-REMOVE-CHILD`** -- the own tree's `Element` has no removal primitive, found by `S4`; open,
  tracked in `TODO.md`.
- **`UIX-LEXER-OPACO`** -- the lexer stops on content the parser would treat as opaque (`<head>`
  contents), found by a real consumer fixture; open, tracked in `TODO.md`, in progress.
- **`RMLX1-CORPUS`** -- ⚠️ **not found anywhere in this repository** (`TODO.md`, `git log --all`,
  and every `docs/*.md` file were searched). It is named in this document's originating brief as
  one of the wave's named items, but no commit, `TODO.md` row or document defines or references
  it. This is reported here rather than guessed at or silently omitted -- if `RMLX1-CORPUS` is a
  real planned item, it needs a `TODO.md` row written by whoever owns that decision; this document
  does not invent one.

### Why `S6a` and `S6b` are two separate slices, not one

Both dumpers walk a tree and emit the same textual format (`docs/uix-dom.md`), which could look
like duplicated work. It is not: they are a **differential oracle**, and an oracle built by one
author checking their own work is not an oracle -- it is a mirror. `S6a` (walks the real
`Rml::ElementDocument`) and `S6b` (walks glintfx's own tree) were deliberately written by
**different agents, each reading only the spec, never the other's source**. The same author on
both slices would share the same blind spot, and the diff in `S7` would come back green for the
wrong reason -- a false pass, not a real one.

**Corollary:** any divergence `S7` finds between the two dumpers' output is a **result** of the
wave, not a failure of it. Adjusting one dumper to match the other without going through the spec
(`docs/uix-dom.md`) would be defeating the oracle. This is why `UIX-CLASS-SPLIT-SPEC` above is
flagged as a process problem even though its technical resolution was correct: the fix belonged in
the spec, not in one dumper's header where the other author was never meant to read it.

## Português

### A decisão (do CTO, não reabrir aqui)

**`S5`, `S8`, `S9` e `S10` não existem. Estão aposentadas, não redefinidas.**

A única menção a esse alcance em todo o repositório é o preâmbulo da `docs/uix-dom.md`, que diz
*"antes de qualquer uma das `S1-S9` ... existir"* -- escrito no dia em que o próprio documento foi
escrito, como uma **estimativa de alcance** do autor, não uma definição de fatias. Nenhum commit,
brief ou linha do `TODO.md` jamais atribuiu conteúdo a `S5`, `S8`, `S9` ou `S10`. Redefini-las
agora seria inventar um passado com autoridade de plano.

**Regra daqui pra frente: trabalho novo ganha ID nomeado, nunca número `S`.** É o que
`UIX-ENTITY-PARIDADE`, `UIX-CLASS-SPLIT-SPEC`, `UIX-REMOVE-CHILD` e `UIX-LEXER-OPACO` já fazem
abaixo -- isso elimina essa classe de bug de processo inteira, porque um ID nomeado não pode ser
confundido com um slot num esquema de numeração que nunca foi totalmente especificado.

### A tabela de fatias

| Fatia | Commit | O que entrega |
| :--- | :--- | :--- |
| `S0` | `7f84d30` | `docs/uix-dom.md` -- a spec do formato canônico de dump, o contrato compartilhado entre `S6a` e `S6b` |
| `S1` | `9ff2f78` | Lexer (`glintfx/src/uix/dom/lexer.*`) -- tokenizer standalone do subconjunto de RML congelado, sem árvore, sem parser |
| `S2` | `dfb9fd9` | Modelo de árvore (`dom_tree.*`) -- `Node`/`Text`/`Element`/`Document`, `find_by_id`, o filtro de existência de whitespace |
| `S3` | `6d6aebd` | Parser recursivo-descendente (`parser.*`) -- consome os tokens da `S1`, constrói a árvore da `S2` |
| `S4` | `f15f1f8` | API DOM por id (`set_text`/`add_class`/`remove_class`) sobre a árvore da `S2` |
| `S6b` | `273ec30` | Dumper sobre a árvore própria da glintfx (`S2`), emitindo o formato da `S0` |
| `S6a` | `3d13512` | Dumper sobre o `Rml::ElementDocument` real, confinado a `glintfx/src/rml/`, emitindo o formato da `S0` |
| `S7` | `e321f2a` | O harness de diff diferencial (o oráculo) -- **entregue**. Mediu 16 fixtures: **15 divergentes, 0 erros de parse, 1 defeito**, um mecanismo só, todas na linha `HEAD`. Toda linha restante é byte-idêntica nas 16 -- as duas árvores, escritas por agentes diferentes, concordam por completo em estrutura, ordem de travessia, contagem de filhos, escape, classes, atributos e texto. O defeito é do próprio `dom_dump_scan_head()` da `S6a` (busca de byte cru sem consciência de comentário pega o primeiro `<head` literal do arquivo, que em 15 fixtures é prosa dentro do comentário de proveniência, 819-1069 bytes-fonte antes do elemento real). Controle que prova: `gusworld_battle_cockpit.rml`, a única fixture cujo comentário não tem `<head` literal, é byte-IDÊNTICA -- correlação 16/16. **Deliberadamente não consertado** (ajustar um dumper pra achatar o que o oráculo acabou de achar é fraudar o oráculo); pinado por mecanismo, reprovando nos DOIS sentidos, e o conserto é item próprio (`UIX-HEAD-COMENTARIO`). Verificado pelo orquestrador: árvore idêntica ao commit, teste verde, e **mutação independente** (remoção do `<head` do comentário de uma fixture) reprovou pelas duas faces simultaneamente -- divergência nova desconhecida E conhecida-que-sumiu, `EXIT=10` |
| `S5`, `S8`, `S9`, `S10` | -- | 🔴 **Nunca atribuídas -- aposentadas.** Não procure conteúdo; nenhum jamais foi definido. Ver "A decisão" acima. |

Cada SHA acima foi verificado com `git show --stat <sha>` contra `main` imediatamente antes deste
documento ser escrito; a linha de assunto de cada commit bate com a fatia sob a qual está listado.

### Itens nomeados da onda (deliberadamente sem número `S`)

- **`UIX-ENTITY-PARIDADE`** (`591d7b5`) -- correção de paridade de
  decodificação de entidade contra o `DecodeRml` do RmlUi, achada via corpus real do consumidor.
- **`UIX-CLASS-SPLIT-SPEC`** -- uma resolução compartilhada pelos dois dumpers ficou no header de
  um deles em vez de ir para a spec; aberto, rastreado no `TODO.md`.
- **`UIX-REMOVE-CHILD`** -- o `Element` da árvore própria não tem primitivo de remoção, achado
  pela `S4`; aberto, rastreado no `TODO.md`.
- **`UIX-LEXER-OPACO`** -- o lexer trava em conteúdo que o parser trataria como opaco (conteúdo do
  `<head>`), achado por uma fixture real do consumidor; aberto, rastreado no `TODO.md`, em
  andamento.
- **`RMLX1-CORPUS`** -- ⚠️ **não encontrado em lugar nenhum deste repositório** (`TODO.md`,
  `git log --all` e todo `docs/*.md` foram varridos). É citado no brief que originou este
  documento como um dos itens nomeados da onda, mas nenhum commit, linha do `TODO.md` ou documento
  o define ou referencia. Isto é reportado aqui em vez de adivinhado ou silenciosamente omitido --
  se `RMLX1-CORPUS` é um item real planejado, precisa de uma linha no `TODO.md` escrita por quem
  é dono dessa decisão; este documento não inventa uma.

### Por que `S6a` e `S6b` são duas fatias separadas, não uma

Os dois dumpers percorrem uma árvore e emitem o mesmo formato textual (`docs/uix-dom.md`), o que
poderia parecer trabalho duplicado. Não é: são um **oráculo diferencial**, e um oráculo construído
por um único autor conferindo o próprio trabalho não é um oráculo -- é um espelho. A `S6a`
(percorre o `Rml::ElementDocument` real) e a `S6b` (percorre a árvore própria da glintfx) foram
deliberadamente escritas por **agentes diferentes, cada um lendo só a spec, nunca o fonte um do
outro**. O mesmo autor nas duas fatias compartilharia o mesmo ponto cego, e o diff da `S7`
voltaria verde pela razão errada -- um passe falso, não um passe real.

**Corolário:** qualquer divergência que a `S7` encontrar entre a saída dos dois dumpers é um
**resultado** da onda, não uma falha dela. Ajustar um dumper para bater com o outro sem passar
pela spec (`docs/uix-dom.md`) seria fraudar o oráculo. É por isso que a `UIX-CLASS-SPLIT-SPEC`
acima está sinalizada como problema de processo mesmo com a resolução técnica dela estando
correta: o conserto pertencia à spec, não ao header de um dumper onde o outro autor nunca deveria
ler.

---

## Appendix: migrated session briefs / Apêndice: briefs de sessão migrados

**EN:** The three files below were the actual dispatch briefs and plan for slices `S6a`, `S6b`
and `S7`, written during the session that produced `S6a`/`S6b` but never committed -- they lived
only in that session's scratch directory and would have been lost when the session ended. They
are migrated here **verbatim except for machine-specific absolute paths**, which were replaced
with repository-relative references. They are kept in the **original pt-br**, not translated to
English: they are session/process evidence (what was asked of each agent, including the explicit
prohibition on reading the sibling dumper's code), not user-facing reference documentation, and
inventing an English translation of a historical instruction would misrepresent what was actually
said at the time. This is a deliberate exception to this repository's usual bilingual-documentation
rule, scoped to this appendix only.

**PT:** Os três arquivos abaixo eram os briefs de despacho e o plano reais das fatias `S6a`,
`S6b` e `S7`, escritos durante a sessão que produziu `S6a`/`S6b` mas nunca commitados -- viviam só
no diretório de scratch daquela sessão e teriam sido perdidos ao ela terminar. Estão migrados aqui
**verbatim, exceto pelos caminhos absolutos específicos da máquina**, substituídos por referências
relativas ao repositório. Ficam no **pt-br original**, não traduzidos pro inglês: são evidência de
sessão/processo (o que foi pedido a cada agente, inclusive a proibição explícita de ler o código
do dumper irmão), não documentação de referência voltada ao usuário, e inventar uma tradução
inglesa de uma instrução histórica desrepresentaria o que de fato foi dito na hora. É uma exceção
deliberada à regra usual de documentação bilíngue deste repositório, restrita a este apêndice.

### A.1 -- Briefing `S6a` (dumper sobre a árvore do RmlUi)

# Briefing RMLX-1 / S6a — o dumper da árvore do RmlUi

> Agente: `backend-engineer`, model=sonnet.
> 🔴 **SLOT EXCLUSIVO.** Esta é a primeira fatia da `RMLX-1` que exige o **build pesado** do
> glintfx. Enquanto ela roda: nenhum outro build, nenhuma outra fatia pesada, **nenhum push**
> (o `pre-push` chama `preci.sh`, que builda). Dois builds simultâneos deste projeto já
> derrubaram esta máquina por OOM.
>
> ⚠️ **Não contar a este agente o que a S6b fez, e não deixá-lo ler o código dela.** A
> independência entre os dois dumpers é o desenho: `docs/uix-dom.md` manda que sejam escritos por
> agentes diferentes vendo só a spec, porque o mesmo autor nos dois compartilharia o mesmo ponto
> cego e o oráculo daria **falso-verde**.

---

Repo: raiz deste repositório, branch `main`. Chat e commits em pt-br; identificadores em inglês; docs bilíngues (EN primeiro, PT depois).

## O que você faz

Você é a fatia **S6a da onda RMLX-1**: escreve o dumper que percorre o **`Rml::ElementDocument` real** — a árvore do RmlUi — e emite o formato canônico de dump.

**`docs/uix-dom.md` é o seu único contrato.** É referência normativa, não sugestão. Se algo estiver ambíguo, **a correção vai para a spec**, nunca para o seu dumper: ajuste privado vira contrato tácito que ninguém mais conhece.

Existe um dumper irmão que percorre uma árvore diferente e emite **o mesmo formato**; uma terceira fatia (`S7`) vai diffar as duas saídas sobre o corpus. Você **não** vai ver o código dele e **não** deve procurá-lo.

## 🔴 A restrição estrutural: você toca RmlUi, logo mora atrás da cerca

A `RMLX-0` confinou **todo** contato com o RmlUi em `glintfx/src/rml/`, e o gate
`tools/check_rml_whitelist.sh` reprova qualquer `#include` de RmlUi fora dali.

- **Seu arquivo mora em `glintfx/src/rml/`.** Não tente colocá-lo em `src/uix/` — lá é o
  território do motor próprio, que por definição não linka RmlUi.
- **Se o gate reprovar, o arquivo está no lugar errado, não o gate.** Ele foi furado 12 vezes ao
  longo da `RMLX-0` e cada furo foi fechado com prova; ele **falha fechado** de propósito
  (include por macro reprova, raw string mal formada reprova). Não o contorne, não peça exceção
  sem argumento forte — exceção nova é dívida que alguma onda futura terá de quitar.
- Rode `tools/check_rml_whitelist.sh` e **meça o código de saída sozinho** (`EXIT=$?` numa linha
  própria, nunca através de pipeline) antes de considerar a fatia pronta.

## O que ler para saber o que o RmlUi realmente faz

Não presuma o comportamento dele: **leia o fonte pinado**. Está em `examples/RmlUi/` (clone de
estudo, gitignored). Duas descobertas já feitas por outras fatias, que estão na spec e que você
vai reencontrar:

- **O `<head>` não é subárvore.** `XMLNodeHandlerHead.cpp` faz `ElementStart` devolver `nullptr`
  para toda tag ali dentro — `title`/`link`/`style`/`script` **nunca** viram `Rml::Element`; vão
  para um `DocumentHeader` paralelo. E `<rml>` não tem handler registrado. Por isso a spec ancora
  o dump em `body` e trata o head como registro opaco.
- **Nó de texto só-whitespace nunca é criado** (`Factory::InstanceElementText`).

A spec já incorpora as duas. Se você encontrar uma terceira coisa que a spec não previu, **isso é
achado**: reporte, não remende.

## Aceite

- Emite o formato de `docs/uix-dom.md` para o corpus de fixtures da onda.
- **Determinístico**: mesma árvore, mesma saída byte a byte, em qualquer ambiente. A spec fixa
  ordenação **byte-wise sem locale** justamente porque os dois dumpers podem rodar em máquinas
  diferentes — nada sensível a locale (comparador localizado, `toupper`/`tolower` de locale,
  formatação numérica localizada).
- **Teste que prova o determinismo**, não só que a saída "parece certa".
- TDD, como as fatias anteriores.

## Fronteira de arquivo — há outros agentes na mesma árvore

- **SEUS**: o dumper em `glintfx/src/rml/` e os testes dele.
- **NÃO TOQUE** em `glintfx/src/uix/**` (é o motor próprio, de outras fatias) nem nos arquivos
  já confinados em `glintfx/src/rml/` que não sejam seus.
- ⚠️ **`glintfx/src/uix/dom/CMakeLists.txt` tem UMA LINHA ÚNICA** com lista de fontes, editada
  por vários agentes — mas ela **não é sua**; você registra no CMake do glintfx propriamente.
  Se precisar editar qualquer arquivo compartilhado, **releia imediatamente antes** e ancore no
  estado atual; se a edição falhar por texto não bater, releia e refaça. **Nunca** reescreva com
  o conteúdo lido antes, nunca reescreva o arquivo inteiro.
- ⚠️ **O ÍNDICE DO GIT É COMPARTILHADO.** **Sempre** `git commit -- <seus caminhos>` com pathspec
  explícito; **nunca** `git commit -m` puro, `git add -A` ou `git commit -a`. `git show --stat`
  depois, conferindo que só os seus arquivos aparecem. Se aparecer arquivo alheio, **pare e me
  avise** — não tente desfazer sozinho.

## Restrições de build (esta fatia é a exceção pesada)

- Você **pode** buildar o glintfx, porque precisa. Mas **um build por vez**: antes de disparar,
  confirme que não há outro rodando (`pgrep -f 'preci\.sh'` — ⚠️ **não** use `ld` no padrão do
  `pgrep`, casa dentro de `firewalld`, `ydotoold`, `kthrotld`).
- **Build em `/var/tmp/`** (disco real), com `export TMPDIR=/var/tmp`. **Nunca `/tmp`** — é
  **tmpfs (RAM)** nesta máquina, e o link do glintfx com deps via FetchContent enche o tmpfs e
  falha com `no space on device`.
- Antes e depois, meça o disco com **`btrfs filesystem usage /`** (sem sudo). ⚠️ **`df` engana em
  btrfs**: o teto real é o `Device unallocated`. Se ele cair abaixo de ~10 GiB, **pare e reporte**.
- **NÃO faça push.** Commits locais, Conventional Commits em pt-br citando `RMLX-1`/S6a.
- Nada de `git reset --hard`, `checkout -f`, `clean -fd`, `stash`.

## Como medir sem se enganar

- **Nunca leia código de saída através de pipeline** (`cmd | grep x; echo $?` devolve o status do
  `grep`). Rode sozinho, capture `$?`, depois filtre.
- **Nunca suprima stderr** e leia o silêncio como aprovação.
- `grep` cru casa substring — `-w`/`\b` para alvos curtos.
- `git diff --cached --stat` antes de commitar, `git show --stat` depois.

## Relatório

SHA; **toda ambiguidade da spec que você encontrou** (mesmo as resolvidas com confiança — são
candidatas a correção da spec, que vai para os dois lados); as decisões de ordenação e por quê;
o resultado do `check_rml_whitelist.sh` com o código de saída medido; o `Device unallocated`
antes e depois; e qualquer comportamento do RmlUi que a spec não previu.

### A.2 -- Briefing `S6b` (dumper sobre a árvore própria)

# Briefing RMLX-1 / S6b — o dumper da NOSSA árvore

> Agente: `backend-engineer`, model=sonnet. Fatia leve (módulo standalone, build ~0,03 s).
> Depende só da S2, que já está pronta (`dfb9fd9`).
>
> ⚠️ **NÃO revisar este brief para "ajudar" com o que a S6a fizer.** A independência entre os
> dois dumpers é o desenho, não um detalhe: `docs/uix-dom.md` diz que serão escritos por agentes
> diferentes vendo só a spec, porque o mesmo autor nos dois compartilharia o mesmo ponto cego e
> o oráculo daria **falso-verde**. Este brief descreve a spec e o alvo, e deixa o dumper
> descobrir o resto. Antecipar o achado é plantar o achado.

---

Repo: raiz deste repositório, branch `main`. Chat e commits em pt-br; identificadores em inglês; docs bilíngues (EN primeiro, PT depois).

## O que você faz

Você é a fatia **S6b da onda RMLX-1**: escreve o dumper que percorre a **árvore DOM própria da glintfx** (`glintfx/src/uix/dom/`) e emite o formato canônico de dump.

**`docs/uix-dom.md` é o seu único contrato.** Ele é a referência normativa do formato — não é sugestão, não é ponto de partida. Se algo nele estiver ambíguo, **a correção vai para a spec**, não para o seu dumper: um ajuste privado no dumper vira contrato tácito que ninguém mais conhece.

Existe um dumper irmão (`S6a`) que percorre uma árvore diferente e emite **o mesmo formato**. Uma terceira fatia (`S7`) vai diffar as duas saídas sobre o corpus inteiro. Você **não** vai ver o código dele e **não** deve procurá-lo — se as duas implementações compartilharem o mesmo mal-entendido da spec, o diff dá verde e não prova nada.

## O que já existe (leia do BLOB commitado)

`git show dfb9fd9:glintfx/src/uix/dom/dom_tree.hpp` — o cabeçalho bilíngue documenta as decisões da S2. **Não leia a working tree para julgar**: outros agentes trabalham nela em paralelo, e o que está lá pode ser trabalho em voo.

Resumo da API: `Node`/`Text`/`Element`/`Document`, `NodeKind`, `find_by_id`, `child_count`, `tag`/`id`/`classes`/`attributes`, `HeadContent` + `set_head`/`clear_head`. `classes()` é `std::set` e `attributes()` é `std::map` com `std::less<>` — a S2 escolheu assim de propósito, e o cabeçalho dela explica por quê em relação à seção 7 da spec. Leia esse trecho antes de escrever qualquer ordenação sua.

## Onde mora, e por quê

`glintfx/src/uix/dom/` (ou `glintfx/tests/uix/`, se você concluir que é ferramenta de teste — decida e justifique). O módulo tem **CMake próprio fora do grafo** de `glintfx/CMakeLists.txt`, que dispara `FetchContent(RmlUi)` incondicional.

⚠️ **Você NÃO pode linkar RmlUi, GLFW ou GL.** Essa é a razão de o módulo existir. Se você achar que precisa de algo do RmlUi para produzir o dump, **pare e reporte** — significa que a spec ou o modelo está errado, e isso é informação valiosa, não um obstáculo a contornar.

## Aceite

- Emite o formato de `docs/uix-dom.md` para todo o corpus de fixtures que a onda já reúne.
- **Determinístico**: a mesma árvore produz byte a byte a mesma saída, em qualquer ambiente. A spec fixa ordenação byte-wise **sem locale** justamente porque os dois dumpers podem rodar em máquinas diferentes — não use nada sensível a locale (`std::sort` com comparador de locale, `toupper`/`tolower` dependentes de locale, formatação numérica localizada).
- **Teste que prova o determinismo**, não só que a saída "parece certa": dumpar duas vezes e comparar, e dumpar árvores construídas em ordens diferentes que devem render a mesma saída.
- TDD: teste antes da implementação, como as fatias anteriores.
- Se a spec não cobrir um caso que você encontrar, **reporte a lacuna** com o caso concreto. Não improvise a regra: a regra improvisada por você não chega ao outro dumper, e a divergência aparece na S7 sem causa aparente.

## Fronteira de arquivo — há outros agentes na mesma árvore

- **SEUS**: os arquivos do dumper (`dumper_uix.*` ou nome equivalente que você escolher) e os testes dele.
- **NÃO TOQUE**: `dom_tree.*`, `lexer.*`, `parser.*`, `dom_api.*`. Se precisar de algo que a árvore não expõe, **não altere: reporte** — é a evidência mais valiosa que a onda produz, e a `RMLX-1` ainda é barata de corrigir.
- ⚠️ **`glintfx/src/uix/dom/CMakeLists.txt` tem UMA LINHA ÚNICA** com a lista de fontes (`add_library(glintfx_uix_dom STATIC ...)`), editada por vários agentes. **Releia imediatamente antes de editar** e ancore no estado atual; se a edição falhar por texto não bater, releia e refaça. **Nunca** reescreva a linha com o conteúdo lido antes, **nunca** reescreva o arquivo inteiro — apagar a fonte de outra fatia quebra o build dela em silêncio, e o sintoma aparece como erro de link sem relação com a causa.
- ⚠️ **O ÍNDICE DO GIT É COMPARTILHADO** entre os agentes ativos. **Sempre** `git commit -- <seus caminhos>` com pathspec explícito; **nunca** `git commit -m` puro, `git add -A` ou `git commit -a`. Depois, `git show --stat` conferindo que só os seus arquivos aparecem. Se aparecer arquivo alheio, **pare e me avise** — não tente desfazer sozinho.

## Restrições

- **NÃO faça push.** Commits locais, Conventional Commits em pt-br citando `RMLX-1`/S6b.
- Nada de `git reset --hard`, `checkout -f`, `clean -fd`, `stash`.
- Scratch em `/var/tmp/`, nunca `/tmp` (tmpfs=RAM aqui). Reporte o que deixou lá; não force flag de deleção.

## Como medir sem se enganar

- **Nunca leia código de saída através de pipeline** (`cmd | grep x; echo $?` devolve o status do `grep`).
- **Nunca suprima stderr** e leia o silêncio como aprovação.
- `grep` cru casa substring — `-w`/`\b` para alvos curtos.
- `git diff --cached --stat` antes de commitar, `git show --stat` depois.

## Relatório

SHA; **toda ambiguidade da spec que você encontrou** (mesmo as que resolveu com confiança — são candidatas a correção da spec, e a spec corrigida vai para os dois lados); as decisões de ordenação e por quê; e qualquer ponto em que a árvore da S2 não deu o que a spec pede.

### A.3 -- Plano das fatias `S6a`/`S6b`/`S7` (o oráculo diferencial)

# Plano das fatias S6a / S6b / S7 — o oráculo diferencial

> Preparado enquanto S3 e S4 rodam. **Ainda não despachado.**

## A regra que não pode ser quebrada, e o motivo dela

`docs/uix-dom.md` é o **único contrato** entre os dois dumpers. Está escrito lá, e é decisão
deliberada da S0:

> os dois dumpers serão escritos por **agentes DIFERENTES vendo só esta spec** — mesmo autor nos
> dois compartilharia o mesmo ponto cego e o oráculo daria **falso-verde**.

⚠️ **Consequência operacional que eu tenho de respeitar como orquestrador:**

1. **Não contar a um o que o outro fez.** Nada de "a S6b resolveu X assim, faça igual" — isso
   destrói a independência que é a razão de existir do desenho.
2. **Não deixar um ler o código do outro.** No brief, proibição explícita de abrir
   `dumper_rml.*` (S6a) ou `dumper_uix.*` (S6b), conforme o lado.
3. **Se um deles achar ambiguidade na spec, a correção vai para a SPEC**, e a spec corrigida é
   entregue aos dois — nunca um ajuste privado no dumper. Foi assim que a S0 resolveu o `<head>`
   e o filtro de whitespace, e é o que impede o remendo de virar contrato tácito.
4. **Divergência encontrada pela S7 é RESULTADO, não falha.** O ledger de divergências é o
   produto da onda. Achatar divergência ajustando um dumper para bater com o outro é fraudar o
   oráculo.

## Dependências e o gargalo real

| Fatia | O que percorre | Depende de | Build |
|---|---|---|---|
| **S6a** | o `Rml::ElementDocument` real, **confinado a `glintfx/src/rml/`** | RmlUi linkado | 🔴 **PESADO** |
| **S6b** | a nossa árvore (`src/uix/dom/`) | S2 (pronta, `dfb9fd9`) | leve (~0,03 s) |
| **S7** | diffa a saída dos dois + o corpus | S6a **e** S6b | 🔴 pesado (precisa das duas) |

🔴 **A S6a é a primeira fatia da RMLX-1 que exige o build pesado do glintfx** — todas as
anteriores viveram no módulo standalone de propósito. Isso muda a régua de concorrência:

- **Um build pesado por vez.** Dois builds do glintfx simultâneos já derrubaram esta máquina
  por OOM. Enquanto a S6a estiver compilando, nenhuma outra fatia pesada roda, e **nenhum push**
  (o `pre-push` chama `preci.sh`, que builda).
- A S6a tem de respeitar o **gate de confinamento** (`tools/check_rml_whitelist.sh`): ela toca
  RmlUi, logo o arquivo dela **precisa** morar em `glintfx/src/rml/` ou entrar na whitelist com
  justificativa. Esse gate foi furado 12 vezes; não o contorne — se ele reprovar, o arquivo está
  no lugar errado.

## Ordem que eu pretendo seguir

1. **S6b sozinha** (leve) — pode correr em paralelo com o que sobrar de S3/S4.
2. **S6a sozinha**, com o slot pesado exclusivo, sem nenhum outro build e sem push concorrente.
3. **S7** depois das duas, com a árvore estável.

Alternativa considerada e **descartada**: rodar S6a e S6b juntas para ganhar tempo. O ganho é
pequeno (a S6b leva minutos) e o risco é o de sempre — dois agentes, um deles compilando pesado,
e o `ctest_guarded` invalidando a rodada inteira por mutação da árvore, **inclusive os verdes**.

## O que o brief de cada dumper NÃO pode conter

Releitura de R3 aplicada a este caso: o brief não pode dizer **como o outro lado resolveu**, nem
antecipar "você vai encontrar divergência em X". Antecipar o achado é plantar o achado. O brief
descreve **a spec e o alvo**, e deixa o dumper descobrir.
