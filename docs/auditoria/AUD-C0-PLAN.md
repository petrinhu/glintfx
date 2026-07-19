# AUD-C0-PLAN — Layer 0 delta re-audit scope / Escopo da re-auditoria delta da Camada 0

- **Date / Data:** 2026-07-19
- **Author / Autor:** Caetano (CTO) — planning only, read-only on code / só planejamento, read-only no código
- **Wave / Onda:** Onda 4 (pós-`LW-AUD`) — Layer 0 / Camada 0
- **Status:** PLAN — awaiting orchestrator fan-out / PLANO — aguardando fan-out do orquestrador

## 0. Premise correction / Correção de premissa

**EN:** The wave brief assumed "Layer 0 implementation complete, all items `🔍` awaiting audit". That state is **stale**: the `core-v0.1.0` wave (W11–W13, 2026-07-09) already ran `TST-INT`/`TST-STATIC`/`TST-MEM`, `F1` (core-ci), **`AUD-ABI` and `AUD-SEC`** (both **CONFORME**, reports in this folder), and tagged `core-v0.1.0` — every `A*`→`E*` item is already `✅`/`✓` in `TODO.md`. What the dossier does **not** cover is everything that landed **after** those two audits: the surface roughly **doubled** (free-list allocator, float conversion, `libcore.a` boundary, SFNT parser, rasterizer, hinter — tags `core-v0.2.0`/`core-v0.3.0` plus 4 untagged commits). Each new piece had a per-wave adversarial review, but the **formal dossier audits were never re-issued** — `docs/auditoria/README.md` even labels Layer 0 as `core-v0.3.0` while `AUD-ABI.md`/`AUD-SEC.md` are dated 2026-07-09 and enumerate only the v0.1.0 files. **Onda 4 is therefore a DELTA re-audit + dossier consolidation, not a first audit.**

**PT:** O brief da onda assumia "implementação da Camada 0 completa, todos os itens `🔍` aguardando auditoria". Esse estado está **defasado**: a onda do `core-v0.1.0` (W11–W13, 2026-07-09) já rodou `TST-INT`/`TST-STATIC`/`TST-MEM`, `F1` (core-ci), **`AUD-ABI` e `AUD-SEC`** (ambos **CONFORME**, relatórios nesta pasta) e taggeou `core-v0.1.0` — todo item `A*`→`E*` já está `✅`/`✓` no `TODO.md`. O que o dossiê **não** cobre é tudo que pousou **depois** dessas duas auditorias: a superfície praticamente **dobrou** (alocador free-list, conversão float, fronteira `libcore.a`, parser SFNT, rasterizador, hinter — tags `core-v0.2.0`/`core-v0.3.0` mais 4 commits sem tag). Cada peça nova teve review adversarial por-onda, mas as **auditorias formais do dossiê nunca foram re-emitidas** — o `docs/auditoria/README.md` inclusive rotula a Camada 0 como `core-v0.3.0` enquanto `AUD-ABI.md`/`AUD-SEC.md` datam de 2026-07-09 e enumeram só os arquivos da v0.1.0. **A Onda 4 é portanto uma re-auditoria DELTA + consolidação de dossiê, não uma primeira auditoria.**

## 1. Audit surface / Superfície de auditoria

**EN:** Complete Layer 0 inventory (root `src/`+`include/`, `make` build). Column "Last formal audit" = coverage by `AUD-ABI.md`/`AUD-SEC.md` as written; "Δ" marks what those reports never saw. Line counts at HEAD (`f461703`).
**PT:** Inventário completo da Camada 0 (`src/`+`include/` da raiz, build via `make`). Coluna "Último audit formal" = cobertura de `AUD-ABI.md`/`AUD-SEC.md` como escritos; "Δ" marca o que esses relatórios nunca viram. Contagem de linhas no HEAD (`f461703`).

| Componente | Arquivo(s) | Teste (freestanding, `make test`) | Sanitizer/dinâmico | Último audit formal |
| :--- | :--- | :--- | :--- | :--- |
| Bootstrap `_start` | `src/start.asm` (70L) | via todo binário | — | `AUD-ABI` ✓ (inalterado) |
| Wrappers syscall | `src/syscall.asm` (62L) | via todo binário | `strace` | `AUD-ABI` ✓ **parcial — Δ: `syscall2` readicionada no SOV-ALLOC** |
| Helpers C↔kernel | `src/sys_{exit,write,read,mmap}.c` + **Δ `src/sys_munmap.c`** | TST-INT | `strace` | `AUD-ABI`/`AUD-SEC` ✓; **`sys_munmap` = Δ (não listado em nenhum dos dois)** |
| Harness C1 | `include/test.h`, `tests/selftest.c`, `tests/negative_probe.c` | `make test-negative` + golden stderr | — | coberto indiretamente (AUD-C0-5.3) |
| Memória | `src/mem.c` (154L) | `tests/test_mem.c` | valgrind (`make test-mem`) | `AUD-SEC` ✓ (inalterado) |
| String | `src/str.c` (179L) | `tests/test_str.c` | — | `AUD-SEC` ✓ (inalterado) |
| Conversão int↔str | `src/conv.c` §int | `tests/test_conv.c` | — | `AUD-SEC` ✓ |
| **Δ Conversão float↔str** | `src/conv.c` §`ftoa`/`atof` (711L total) | `tests/test_conv.c` | — | **nunca (só review adversarial `qa-engineer`, SOV-FCONV)** |
| mini-printf | `src/printf.c` (346L; **Δ `%f`**) | `tests/test_printf.c`, `printf_e2e` | — | `AUD-SEC` ✓ **parcial — `%f` = Δ** |
| **Δ Alocador free-list** | `src/alloc.c` (799L — **REESCRITO**: era bump na época do `AUD-SEC`; hoje free-list first-fit + boundary tags + coalescência + `munmap` de arena) | `tests/test_alloc.c`, `test_alloc_free.c` | valgrind (`make test-mem`) | **nunca como free-list (só review adversarial `security-engineer`, 1 CRÍTICO fechado — `216ce7d`)** |
| **Δ Fronteira `libcore.a`** | `src/core_api.c` (201L), `include/core/core.h`, prefixo `glx_`, ADR-0009 | `make libcore-test` (`tests/libcore_consumer.cpp` hosted C++) + gate `nm -u` | — | **nunca (review SOV-LIBCORE fechou symbol-hijack CRÍTICO — `40a74bc`)** |
| **Δ Parser SFNT** | `src/sfnt.c` (1344L, **input hostil por natureza**; pós-v0.3.0: tabela `kern` `7d3d6d4` + fix aggregate-init `1432ac2` + **bsearch `baeb15b` SEM review adversarial próprio — commit de perf**) | `tests/test_sfnt.c` | `make sanitize-sfnt` (hosted-companion ASan+UBSan) | **nunca (review SOV-SFNT fechou UB float→int16 CRÍTICO — `ecf74b8`)** |
| **Δ Rasterizador** | `src/raster.c` (816L) | `tests/test_raster.c` (oráculo analítico) | `make sanitize-raster` | **nunca (review SOV-RAST; mina cosmética `iabs32(INT32_MIN)` registrada na INBOX)** |
| **Δ Grid-fitter (hinter)** | `src/hint.c` (348L, pós-v0.3.0, `df7f3c7`) | `tests/test_hint.c` (355L) | **NENHUM — gap: não existe `sanitize-hint` irmão** | **nunca** |

**EN:** Summary: ~3 900 of 5 171 lines of `src/` were never seen by a formal audit report. The two hottest spots: `src/sfnt.c` post-`core-v0.3.0` additions (kern + bsearch, the bsearch being a perf commit without its own adversarial review, over a **hostile-input** table) and `src/hint.c` (newest TU, tested but with **no hosted-companion sanitizer target**, unlike its sfnt/raster siblings — a domino-pattern gap).
**PT:** Resumo: ~3 900 de 5 171 linhas de `src/` nunca foram vistas por relatório formal de auditoria. Os dois pontos mais quentes: os acréscimos pós-`core-v0.3.0` de `src/sfnt.c` (kern + bsearch, sendo o bsearch um commit de perf sem review adversarial próprio, sobre tabela de **input hostil**) e `src/hint.c` (TU mais nova, testada mas **sem alvo sanitizer hosted-companion**, ao contrário dos irmãos sfnt/raster — gap do padrão dominó).

## 2. Applicable chapters / Capítulos aplicáveis

**EN:** The Layer-0 catalog in `AUDITORIAS.md` has exactly two chapters (`AUD-ABI`, `AUD-SEC`) — deliberately pruned. Onda 4 keeps them (as delta re-issues) and adds one thin consolidation gate. **Three chapters, not six** — anti-over-engineering for an early-tier, 5.2 kL surface.
**PT:** O catálogo da Camada 0 no `AUDITORIAS.md` tem exatamente dois capítulos (`AUD-ABI`, `AUD-SEC`) — podados de propósito. A Onda 4 os mantém (como re-emissões delta) e adiciona um gate fino de consolidação. **Três capítulos, não seis** — anti-over-engineering para porte early e superfície de 5,2 kL.

### 2.1 `AUD-ABI-Δ` — conformidade ABI/ASM do delta

- `syscall2` readicionada em `src/syscall.asm` + fronteira nova `src/sys_munmap.c` (o relatório de 2026-07-09 verificou `syscall0/1/3/6` e cita explicitamente o reshuffle `r10←r8`; a `syscall2` entrou horas depois).
- **Fronteira mista do ADR-0009**: `libcore.a` (freestanding, `-fno-pic`) linkado num consumidor C++ **hosted** (`tests/libcore_consumer.cpp`) — alinhamento de stack 16B nos dois mundos, red zone, ausência de reloc incompatível, prefixo `glx_` estanque (nenhum símbolo não-prefixado exportado além do contrato), gate `nm -u` (zero undefined = zero libc).
- Evidência exigida: `objdump -d -M intel` / `readelf` sobre binários **rebuiltados do zero** (`make clean && make build && make libcore`), não sobre leitura de source; `strace` inventariando as syscalls reais da suíte (esperado: só `mmap/munmap/write/read/execve/exit`).

### 2.2 `AUD-SEC-Δ` — memory-safety / UB / input hostil do delta

- **Alocador free-list** (`src/alloc.c`): boundary tags forjáveis via `free()` de ponteiro adverso, coalescência nas bordas de arena (`[base,end)`), double-free (contrato `sys_exit(111)` documentado), overflow de aritmética de tamanho (re-verificar o fix `216ce7d` + procurar gêmeos), alinhamento 16B pós-reuso.
- **`ftoa`/`atof` + `%f`** (`src/conv.c`, `src/printf.c`): strings hostis longas (os 2 caps anti-DoS documentados), fronteiras `±0.0`/inf/nan/denormal, o edge conhecido `1e303+`→`+Inf` (documentado — conferir que segue honesto e testado), validar-antes-do-cast em todo site float→int.
- **SFNT pós-v0.3.0** (`src/sfnt.c`): a tabela `kern` com bsearch (`baeb15b`) sobre dados hostis — **precondição de ordenação**: fonte adversarial com tabela kern não-ordenada/truncada não pode causar OOB nem loop; conferir bounds do par (índices `l`/`r`), e o fix de aggregate-init (`1432ac2`).
- **`src/hint.c`**: passada completa (nunca auditado) + **dominó da mina `iabs32(INT32_MIN)`** do raster — procurar o padrão gêmeo (negação/abs sem guarda em fixed-point) no hinter; aritmética 26.6 (shift de negativo, overflow de acumulador).
- Substituto do ASan em freestanding (canônico, `TESTES.md`): padrão **hosted-companion** — `make sanitize-sfnt`/`sanitize-raster` já existem; **entregável desta auditoria: criar o `sanitize-hint` irmão** (a TU de `hint.c` só usa `<stdint.h>`-likes, deve ser hosted-compilável como as irmãs — confirmar; se não for, documentar o porquê e cair pra análise manual + fronteiras). Sondas descartáveis em scratchpad (precedente do `AUD-SEC` original: 51 checks hosted ASan+UBSan) + `valgrind` via `make test-mem`.
- Toda alegação de fix re-verificada por **prova negativa executada** (reverter o fix localmente → o gate/sanitizer tem que acusar; restaurar).

### 2.3 `AUD-C0-GATE` — invariante zero-libc + frescor de suíte + consolidação do dossiê

- `nm -u` limpo em **todos** os binários (incl. `libcore_consumer`), `make test && make test-negative && make check-static && make test-mem && make sanitize-sfnt && make sanitize-raster` verdes no HEAD, golden files íntegros.
- Consolidação: seções "Delta re-audit 2026-07-19" **apensadas** a `AUD-ABI.md`/`AUD-SEC.md` (relatório individual = fonte de verdade), índice `docs/auditoria/README.md` atualizado (datas, estado real da Camada 0, achados novos por severidade), linhas correspondentes do `TODO.md` tocadas.

### Not applicable (and why) / Não aplicáveis (e por quê)

**EN/PT:** Sem paralelo com a Camada 1 — a Camada 0 não tem: **supply-chain/CVE** (zero dependência — é o ponto do projeto; o gate `nm -u` É a auditoria de dependência), **licença de terceiro** (100% autoral MPL-2.0, clean-room; SPDX já gateado em `make check-static`), **rede/authN/cripto/PII/DAST** (mesma poda do `AUD-L1`), **parsing de asset da Camada 1** (o análogo — SFNT — está DENTRO do `AUD-SEC-Δ`), **fuzzing contínuo** (gatilho já registrado na INBOX pro SFNT; fica como semente pull, não gate desta onda — as sondas adversariais dirigidas + sanitizers cobrem o porte atual).

## 3. Method per chapter / Método por capítulo

| Capítulo | Agente | Evidência mínima exigida |
| :--- | :--- | :--- |
| `AUD-ABI-Δ` + `AUD-C0-GATE` | `embedded-firmware-engineer` (auditor do `AUD-ABI` original — continuidade de método; `qa-engineer` seria aceitável, mas a fronteira mista `.a`↔hosted pede olho de ABI) | `objdump`/`readelf` do binário real rebuiltado; `strace` da suíte; `nm -u` de todos os binários; gates `make` todos verdes com saída colada no relatório |
| `AUD-SEC-Δ` | `security-engineer` (instância nova — o reviewer do SOV-ALLOC também foi `security-engineer`, mas em sessão distinta; implementer ≠ reviewer ≠ auditor preservado) | Sondas hosted ASan+UBSan **executadas** (não lidas) pra cada classe da §2.2; prova negativa de ≥1 fix por componente; `sanitize-hint` novo rodando limpo (ou justificativa técnica da impossibilidade); fonte kern adversarial sintética exercitando o bsearch |

**EN:** Report-of-agent is not proof: the orchestrator re-verifies (clean rebuild + spot-check of file:line claims) before accepting, per house rule.
**PT:** Relatório de agent não é prova: o orquestrador re-verifica (rebuild limpo + spot-check das claims arquivo:linha) antes de aceitar, regra da casa.

## 4. Expected findings & promotion gate / Achados esperados e gate de promoção

**EN:** Severity per `AUDITORIAS.md` (🔴 CRÍTICO blocks tag · 🟠 IMPORTANTE fix-before-tag · 🟡 COSMÉTICO records). Concrete candidates: kern-bsearch precondition on hostile table (🟠 if reachable), missing `sanitize-hint` (🟡→closed in-wave), `iabs32` twin in `hint.c` (🟡 unless reachable), forged-boundary-tag behaviour under `free()` of non-heap pointer (likely documented trade-off, 🟡), dossier freshness mismatch (🟡, closed by §2.3).
**PT:** Severidade conforme `AUDITORIAS.md` (🔴 CRÍTICO bloqueia tag · 🟠 IMPORTANTE corrige antes da tag · 🟡 COSMÉTICO registra). Candidatos concretos: precondição do bsearch do kern sob tabela hostil (🟠 se alcançável), ausência do `sanitize-hint` (🟡→fechado na própria onda), gêmeo do `iabs32` em `hint.c` (🟡 salvo se alcançável), comportamento de boundary-tag forjada sob `free()` de ponteiro não-heap (provável trade-off documentado, 🟡), descompasso de frescor do dossiê (🟡, fechado pela §2.3).

**Gate / Gate:**

1. Zero 🔴 aberto; todo 🟠 corrigido **e re-verificado por execução** dentro da onda.
2. Todos os gates `make` da §2.3 verdes no HEAD + `core-ci` verde.
3. Dossiê consolidado (README + relatórios com seção delta + `TODO.md` sincronizado).
4. → **`REL-TAG core-v0.4.0`** (o delta pós-v0.3.0 — kern, bsearch, hinter — está sem tag; a tag em si é **decisão do líder**, como todo push/tag). Como os itens já estão `✅`/`✓`, o gate desta onda promove o **dossiê** (frescor Camada 0 = HEAD), não itens `🔍`.

## 5. Fan-out plan / Plano de fan-out

**EN:** **2 sonnet agents in PARALLEL, then one short serial consolidation step.** Parallelism is safe: both are read-only on product code and write to **disjoint report files**. The only code-adjacent write (new `tests/sanitize_hint.c` + Makefile target, Agent A) touches files Agent B never writes.
**PT:** **2 agentes sonnet em PARALELO, depois um passo serial curto de consolidação.** O paralelismo é seguro: ambos são read-only no código de produto e escrevem em **relatórios disjuntos**. A única escrita perto de código (novo `tests/sanitize_hint.c` + alvo no Makefile, Agente A) toca arquivos que o Agente B nunca escreve.

### Agente A — `security-engineer` (sonnet) → `AUD-SEC-Δ`

Brief-esqueleto: (1) ler `AUDITORIAS.md`, `AUD-SEC.md` (formato + o que JÁ foi coberto — não re-auditar `mem.c`/`str.c`/int-conv, só delta), `TESTES.md` §hosted-companion, ADR-0004/0009; (2) auditar `src/alloc.c`, `src/conv.c` §float, `src/printf.c` §`%f`, `src/sfnt.c` §kern/bsearch + fixes pós-v0.3.0, `src/raster.c` (dominó leve), `src/hint.c` (passada completa); (3) sondas hosted ASan+UBSan por classe (scratchpad, nunca commitadas) + fonte kern adversarial sintética; (4) **criar `make sanitize-hint`** (padrão idêntico ao `sanitize-raster`; única escrita de produto permitida: `tests/sanitize_hint.c` + bloco no `Makefile`); (5) prova negativa de ≥1 fix histórico por componente; (6) apensar seção "Delta re-audit 2026-07-19" bilíngue em `AUD-SEC.md` com achados por severidade. **NÃO push. NÃO tocar em nenhum outro arquivo de produto.** Commits locais citando `AUD-SEC` no corpo.

### Agente B — `embedded-firmware-engineer` (sonnet) → `AUD-ABI-Δ` + `AUD-C0-GATE`

Brief-esqueleto: (1) ler `AUD-ABI.md` (formato + cobertura prévia), ADR-0001/0003/0005/0009; (2) auditar `src/syscall.asm` (foco `syscall2`), `src/sys_munmap.c`, `src/core_api.c` + fronteira `libcore.a`↔`libcore_consumer.cpp` (alinhamento, red zone, estanqueidade `glx_`); (3) `make clean && make build && make libcore && make libcore-test` + `objdump -d -M intel`/`readelf`/`nm -u` sobre os artefatos reais; (4) `strace` da suíte completa (inventário de syscalls colado no relatório); (5) rodar TODOS os gates da §2.3 e registrar saída; (6) apensar seção "Delta re-audit 2026-07-19" bilíngue em `AUD-ABI.md`. **NÃO push. Escrita permitida: só `AUD-ABI.md`.** Commits locais citando `AUD-ABI`.

### Passo serial final — consolidação (curto)

`internal-auditor` (ou o orquestrador re-verificando e delegando a edição): atualizar `docs/auditoria/README.md` (datas, estado Camada 0 = HEAD/`core-v0.4.0`-candidato, achados novos na tabela de severidade) + linhas do `TODO.md`. Serial porque toca os mesmos arquivos-índice que qualquer paralelismo colidiria; depende dos dois relatórios prontos e **re-verificados pelo orquestrador**.

### Ordem e porquê / Order & why

A ‖ B (superfícies e relatórios disjuntos; builds leves — a Camada 0 compila em segundos, sem risco de OOM; único cuidado: os dois rodam `make` na mesma árvore `build/` — cada agente deve usar `make -C` na árvore raiz normalmente, o build é determinístico e barato, e re-rodar `make build` após o outro é custo desprezível; se o orquestrador preferir zero interferência, B roda `make clean && make build` no início do seu turno de gates e A usa sondas hosted que não dependem de `build/`) → consolidação serial.

## 6. Risks & gotchas / Riscos e gotchas

- **ASan/UBSan não rodam no binário freestanding** — substituto canônico é o hosted-companion (`TESTES.md`); não inventar runtime de sanitizer nem dar exceção ao `-ffreestanding`.
- **valgrind é cego dentro da arena** (`0 allocs` — bump/free-list não passam pela libc): a cobertura real de invariante é `test_alloc*/test_alloc_free` + sondas hosted; limitação já documentada em `TESTES.md`, não re-descobrir.
- Build via **`make`** (não cmake); toolchain Fedora 44: nasm 3.01, clang 22, ld 2.46. Camada 0 é leve — sem risco de OOM/tmpfs; sondas descartáveis no **scratchpad da sessão** (nunca commitadas, precedente do AUD-SEC original).
- **NÃO push / NÃO tag** sem autorização explícita do líder nesta onda; commits locais são livres.
- `tests/sanitize_*.c` ficam **fora** de `$(PROGRAMS)`/`make test` (wildcard filter no `Makefile:74`) — o `sanitize-hint` novo segue o mesmo filtro.
- cppcheck: escopo restrito a `src/` de propósito; falso-positivo só com suppress cirúrgico + rationale (regra do `TESTES.md`).
- Fuzzing do SFNT: **fora do gate** desta onda (semente INBOX); se o Agente A o achar barato demais pra não fazer, registrar como recomendação, não expandir escopo sem o líder (regra de expansão via AskUserQuestion).
