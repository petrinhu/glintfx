# TODO — loucura_c_asm

Tabela de pendências e planejamento. **As linhas estão na ordem de execução recomendada** (topological + WSJF, minimiza retrabalho). A coluna **Onda** marca passos paralelizáveis (mesma onda = sem dependência mútua, podem rodar juntos). Porte: early com eixo técnico elevado (cenário B bigtech), cerimônia leve.

> Convenção de frescor: implementação entregue → `🔍 Pendente verificação` (nunca `✅` direto). `✅ Concluído` só após a onda `TST-*`/`AUD-*` correspondente. Citar o ID do item na mensagem do commit. Trabalho novo no meio do caminho → anexar na INBOX (fim do arquivo), não reordenar na hora.

| ID | Onda | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1 | W0 | Arquitetura | ADR — camada de syscall: wrappers genéricos `syscall0..6`. Decidido → [ADR-0001](docs/adr/0001-syscall-layer-style.md). **OWD** | Alta | — | Baixa | 💡 Decisão tomada | — |
| A2 | W0 | Arquitetura | ADR — contrato de erro: retorno `-errno` cru. Decidido → [ADR-0002](docs/adr/0002-error-contract.md). **OWD** | Alta | — | Baixa | 💡 Decisão tomada | — |
| A3 | W0 | Arquitetura | ADR — ABI interna: System V AMD64 estrita; `_start` alinha stack 16B. Decidido → [ADR-0003](docs/adr/0003-internal-abi.md). **OWD** | Alta | — | Baixa | 💡 Decisão tomada | — |
| A4 | W0 | Arquitetura | ADR — alocador: bump sobre `mmap`, assinatura `malloc/free/realloc` final. Decidido → [ADR-0004](docs/adr/0004-allocator-strategy.md). **OWD** | Alta | — | Média | 💡 Decisão tomada | — |
| A5 | W0 | Arquitetura | ADR — layout ELF: estático no-PIE, entry `_start`. Decidido → [ADR-0005](docs/adr/0005-elf-layout.md). **OWD** | Alta | — | Baixa | 💡 Decisão tomada | — |
| A6 | W1 | Build | Build system (`Makefile`): regras `clang -ffreestanding -nostdlib` + `nasm -f elf64` + `ld -nostdlib -static -no-pie -e _start`; alvos build/test/clean/run | Alta | A5 | Média | ⏳ Pendente | — |
| B1 | W1 | Bootstrap | `types.h` próprio (sem stdint/stddef): `size_t`, `ssize_t`, `uintptr_t`, `NULL`, `bool` | Alta | — | Baixa | ⏳ Pendente | — |
| B2 | W1 | Bootstrap | Constantes de syscall (números) e flags (open/mmap/etc.) para x86-64 | Alta | — | Baixa | ⏳ Pendente | — |
| STD-SPDX | W1 | Convenções | Header SPDX `SPDX-License-Identifier: MPL-2.0` + copyright no topo de TODO arquivo de código (C `//`, NASM `;`, Makefile `#`), aplicado desde a criação (DoD herdado). Definir snippet padrão no `CLAUDE.md` | Média | — | Baixa | ⏳ Pendente | — |
| B3 | W2 | Bootstrap | Camada de syscall: wrappers `syscall0..6` em ASM/inline (rax + rdi/rsi/rdx/r10/r8/r9) | Alta | A1, A3, B2 | Média | ⏳ Pendente | — |
| B4 | W3 | Bootstrap | `_start` em ASM: alinhar stack, ler argc/argv/envp do stack inicial, chamar `main`, capturar retorno. **Fundação** | Alta | B3, A3, A5 | Alta | ⏳ Pendente | — |
| B5 | W4 | Bootstrap | `sys_exit` + binário mínimo `exit(42)`. **GATE: valida pipeline build→link→run** | Alta | B4, A6 | Baixa | ⏳ Pendente | — |
| B6 | W5 | Bootstrap | `sys_write` + "hello world" 100% puro (valida saída por syscall) | Alta | B5 | Baixa | ⏳ Pendente | — |
| B7 | W6 | Bootstrap | `sys_read` (entrada) | Média | B6 | Baixa | ⏳ Pendente | — |
| C1 | W6 | Testes | Harness de teste próprio (sem libc): `assert` próprio + runner que reporta via `write` e exit code (habilita TDD do resto) | Alta | B6, B1 | Média | ⏳ Pendente | — |
| D1 | W7 | Libc-Núcleo | Primitivas de memória: `memcpy/memset/memmove/memcmp` (atenção: `-fno-builtin` p/ clang não reintroduzir) | Alta | C1 | Média | ⏳ Pendente | — |
| D2 | W8 | Libc-Núcleo | Operações de string: `strlen/strcmp/strncmp/strcpy/strncpy/strcat/strchr` | Alta | D1 | Média | ⏳ Pendente | — |
| D3 | W9 | Libc-Núcleo | Conversões `int↔string`: `itoa/utoa` (base 10 e 16), `atoi/atou` | Alta | D2 | Média | ⏳ Pendente | — |
| E1 | W10 | Libc-Avançada | Alocador próprio `malloc/free/realloc` via `mmap`/`brk` (bump → free-list). **OWD** | Alta | A4, D1 | Alta | ⏳ Pendente | — |
| E2 | W10 | Libc-Avançada | `mini-printf` sobre `sys_write`: `%d %u %x %s %c %%` | Alta | D3 | Média | ⏳ Pendente | — |
| TST-INT | W11 | Testes | Testes de integração/E2E: builda e roda cada binário/demo, valida exit code e stdout. Ver `TESTES.md` | Alta | E1, E2 | Média | ⏳ Pendente | — |
| TST-STATIC | W11 | Testes | Análise estática (clang static analyzer / `scan-build` / cppcheck freestanding) + verificar presença do header SPDX (STD-SPDX) em todo arquivo. Ver `TESTES.md` | Média | D3, E2, STD-SPDX | Baixa | ⏳ Pendente | — |
| TST-MEM | W11 | Testes | Validação de memória do alocador: leak, double-free, alinhamento (testes dedicados + valgrind no estático se viável). Ver `TESTES.md` | Alta | E1 | Média | ⏳ Pendente | — |
| F1 | W12 | Infra | CI freestanding (Woodpecker LOCAL preferido, ou Forgejo Actions): build + roda toda a suíte de testes | Alta | TST-INT, TST-STATIC, TST-MEM | Média | ⏳ Pendente | — |
| AUD-ABI | W12 | Auditoria | Auditoria de conformidade ABI/ASM: convenção de chamada, preservação de registradores callee-saved, alinhamento de stack 16B. Ver `AUDITORIAS.md` | Alta | B3, B4, D1, TST-INT | Média | ⏳ Pendente | — |
| AUD-SEC | W12 | Auditoria | Auditoria de segurança: validação de args de syscall, bounds em mem/string, overflow inteiro em conv/alloc, ausência de UB. Ver `AUDITORIAS.md` | Alta | D3, E1, TST-INT | Média | ⏳ Pendente | — |
| REL-TAG | W13 | Release | Tag de versão **`core-v0.1.0`** (DISTINTA do glintfx `v0.1.0`): primeiro runtime funcional + libc própria mínima | Média | F1, AUD-ABI, AUD-SEC | Baixa | ⏳ Pendente | — |
| DOC-WIKI | W14 | Docs | Wiki do repo + doc `.md` extensa para INICIANTE (bilíngue en→pt), via `technical-writer`. Pré-req: tag de versão | Média | REL-TAG | Média | ⏳ Pendente | — |

## Legenda

- **Onda**: leva de execução. Mesma onda = paralelizável. `‖` itens independentes.
- **OWD** (one-way-door): decisão de reverter caro → no topo, exige o líder.
- **Status**: ✅ Concluído · 🔄 Em andamento · 🟡 Parcial · ⏳ Pendente · 💡 Decisão tomada · 🎨 Pendente design · 🔍 Pendente verificação.
- **Estado Auditado**: `—` não auditado · `✓` aprovado · `⚠` com ressalvas.

## Caminho crítico

`A1/A3 → B3 → B4 → B5 → B6 → C1 → D1 → D2 → D3 → E2` (E1 pendurado em A4 + D1). Tudo a jusante depende de fechar o **W0 (as 5 decisões de arquitetura)**.

## Camada 1 — C++23 (RmlUi + GL3) — ver ADR-0006

Trilha da biblioteca C++23 (compat C++17→23) que une RmlUi (UI) + renderer GL3 (efeitos). Independente da Camada 0 (W0–W14 acima). O detalhamento de **features** depende do brainstorm bigtech (`L1-BRAINSTORM`). Ondas `LW*` são internas a esta trilha.

> **Estado:** v1 ENTREGUE e mergeada (glintfx `v0.1.0` → `v0.2.4`, 6 tags) — ondas LW1–LW4 ✅. Pendências pós-v1: **LW5** = bugfix/hardening/doc rápido paralelizável (⏳); **LW6** = CI + DX de distribuição (WIP, 3 itens `🔍` — mais adiantada que LW5); **LW-TST/LW-AUD** = rede de qualidade transversal (catálogos em [`TESTES.md`](TESTES.md) / [`AUDITORIAS.md`](AUDITORIAS.md)); **LW7** = backends adiados; **LW8** = internalização clean-room (endgame).
>
> **Ordem recomendada (Kanban, finish-before-start):** verificar/fechar o WIP de **LW6** (os 3 `🔍`) antes de abrir trabalho novo → drenar **LW5** (barato). A rede **LW-TST/LW-AUD** já tem todos os pré-requisitos de código satisfeitos (features v0.2.x + embed shipados), então entra **cedo e pull-based**, em paralelo/logo após LW5/LW6 (não espera LW7/LW8) — parte dela inclusive é o que verifica os `🔍` de LW6 (`TST-L1-CONTRACT` exercita `L1.8-FINDPKG`; o gate de CI verde fecha `L1.2-CI`). **LW7/LW8 = keep-back.** A **v2** roda em modo **pull incremental** (backlog, não agendada — ver seção v2).

| ID | Onda | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| L1-CLONE | LW1 | C++23/Setup | Clonar RmlUi + gl3w em `examples/` (gitignored) para estudo/RE | Alta | — | Baixa | ✅ Concluído | — |
| L1-BRAINSTORM | LW1 | C++23/Produto | Brainstorm bigtech: escopo/efeitos/nome. Gerou spec + plano + ADR-0006/0007; nome **glintfx**, licença MPL-2.0 | Alta | — | Média | ✅ Concluído | ✓ |
| L1-BUILD | LW2 | C++23/Build | Esqueleto CMake + FetchContent (RmlUi 6.3 + gl3w vendorizado) **pronto**; install/export + alias + consumer drop-in via FetchContent entregues (Task 8) | Alta | L1-CLONE | Média | ✅ Concluído | ✓ |
| L1-BACKEND | LW3 | C++23/Plataforma | Janela/contexto **GLFW** + wrap `RenderInterface_GL3` + bootstrap RmlUi/FreeType; fix backbuffer opaco (alpha×compositor). SDL/X11 adiados | Alta | L1-BUILD | Alta | ✅ Concluído | ✓ |
| L1-DEMO | LW4 | C++23/Demo | Showcase com os **4 efeitos** (glow, degradê, backdrop-blur, mask) — validado em GPU real (Intel+NVIDIA/Wayland) pelo líder 2026-06-29 | Alta | L1-BACKEND, L1-BRAINSTORM | Média | ✅ Concluído | ✓ |
| L1-API | LW4 | C++23/API | Fachada `glintfx::App` RAII (pImpl, headers limpos), compat C++17→23 | Alta | L1-BRAINSTORM | Alta | ✅ Concluído | ✓ |
| L1-DOCS | LW4 | C++23/Docs | Docs glintfx v0.1 mergeados: README, AGENTS.md, CHANGELOG, CONTRIBUTING, SECURITY, getting-started, effects | Alta | L1-API, L1-DEMO | Média | ✅ Concluído | ✓ |
| L1.6-TESTDOC | LW5 | C++23/Docs | `TESTES.md`: corrigir `xvfb-run` na seção "GPU real" (Xvfb usa llvmpipe, não GPU real) — já entregue em `0a93b2b`, status estava desatualizado | Média | — | Baixa | ✅ Concluído | — |
| L1.3-SNAPFLIP | LW5 | C++23/Bugfix | `snapshot()` sai de cabeça pra baixo (origem do `glReadPixels`); flip vertical das linhas | Média | L1-API | Baixa | ⏳ Pendente | — |
| L1.9-VERSEMBED | LW5 | C++23/API | Mover `version()` de `app.hpp` p/ header sempre-disponível: o build embed-only (`GLINTFX_BACKEND_GLFW=OFF`, ex.: GusWorld) hoje não expõe `glintfx::version()`. Mudança de superfície pública (two-way-door). Ex-INBOX (v0.2.4 review) | Baixa | L1-API, L2-EMBED | Baixa | ⏳ Pendente | — |
| L1.10-APIDOC | LW5 | C++23/API | Contrato de API (F1 review), bundle de 2 itens: (a) anotar em `App`/`UiLayer` que métodos exceto `ok()`/dtor têm UB em objeto movido-de (convenção do projeto, igual `unique_ptr`); (b) `UiEvent{}` default cai em `MouseMove` -- considerar `Type::None` p/ default mais seguro (two-way-door). Ex-INBOX | Baixa | L1-API, L2-EMBED | Baixa | ⏳ Pendente | — |
| L1.4-GUARDS | LW5 | C++23/Hardening | Guards null: `window_glfw` (`make_current`/`swap`/`poll` sem checar `win_`; `glfwTerminate` sem flag `glfw_inited_`) e `App` (`poll_events`/`render` sem `impl_->ok`). Não crasham, só assimetria | Baixa | L1-BACKEND, L1-API | Baixa | ⏳ Pendente | — |
| L1.5-DEMOBG | LW5 | C++23/Demo | `body { width: 100% }`: fundo do demo não cobre a janela inteira | Baixa | L1-DEMO | Baixa | ⏳ Pendente | — |
| L1.2-CI | LW6 | C++23/Infra | CI versionado: **GitHub Actions** (`.github/workflows/ci.yml`, gate principal) + **Codeberg Forgejo Actions** (`.forgejo/workflows/ci.yml`, soberania) — gate automático da suíte glintfx (5 testes: 4 smokes + `render_sanity`) sob Xvfb/llvmpipe | Alta | L1-DEMO | Média | 🔍 Pendente verificação | — |
| L1.1-ERRSTRAT | LW6 | C++23/API | `App` estratégia de erro de construção (hardening DX p/ distribuição): hoje falha silenciosamente; considerar `bool ok()` e/ou exceptions opcionais. **Decisão de design → líder/CPO** | Alta | L1-API | Média | 🔍 Pendente verificação | — |
| L1.8-FINDPKG | LW6 | C++23/Build | `find_package(glintfx)` habilitado via `glintfxConfig.cmake.in` + target `glintfx::glintfx IMPORTED` (sem `install(EXPORT)`, contornando o export-set do CMake 4.x); RmlUi co-instalado. Ambos os caminhos verdes (FetchContent + find_package) | Média | L1-BUILD | Média | 🔍 Pendente verificação | — |
| TST-L1-STATIC | LW-TST | C++23/Testes | Análise estática C++17→23 (`clang-tidy` modernize/cppcoreguidelines/clang-analyzer + `cppcheck`) **+ gate de encapsulamento**: grep include-based `(Rml\|glfw\|gl3w\|GL/\|stb\|SDL)` nos headers públicos (a substring "GLFW" da macro `GLINTFX_BACKEND_GLFW` não conta). Inclui a **unificação da regex do gate** (ex-INBOX). CI-now, sempre-verde. Ver `TESTES.md` | Alta | L1-API | Média | ⏳ Pendente | — |
| TST-L1-SECRETS | LW-TST | C++23/Testes | Secret-scanning (`gitleaks`/`trufflehog`) no histórico + pre-CI do repo público dual (Codeberg+GitHub). Custo ~zero, sempre-verde. CI-now. Ver `TESTES.md` | Média | — | Baixa | ⏳ Pendente | — |
| TST-L1-DEPS | LW-TST | C++23/Testes | Scan de deps + CVE (`trivy fs`/`grype`) na árvore fetchada+vendorizada (stb_image, gl3w, RmlUi 6.3; GLFW/FreeType/libGL do sistema) + auditoria manual das versões pinadas vs advisories. T5+T12 fundidos. CI-now. Ver `TESTES.md` | Média | L1-BUILD | Baixa | ⏳ Pendente | — |
| TST-L1-MEM | LW-TST | C++23/Testes | Gate de memória: rebuild `-fsanitize=address,undefined` + LSan, `ctest` sob Xvfb; suppression-file p/ ruído GLFW/Mesa/FreeType. Ancora o bug histórico do buffer stb não-liberado em `LoadTexture` (hoje RAII). Nightly (build mais lenta que o gate quente de PR). Ver `TESTES.md` | Alta | L1-BACKEND, L2-EMBED | Média | ⏳ Pendente | — |
| TST-L1-DECODE | LW-TST | C++23/Testes | Robustez de decode de asset não-confiável (PNG/JPG/TGA malformado/truncado via stb_image + laço próprio de premultiply, bounds `w*h*4`): retorna fallback sem crash/corrupção. Corpus → graduável a libFuzzer/AFL; roda sob a build ASan de TST-L1-MEM. Pull por demanda. Ver `TESTES.md` | Média | L1-BACKEND, TST-L1-MEM | Média | ⏳ Pendente | — |
| TST-L1-GLLEAK | LW-TST | C++23/Testes | Vazamento de recurso GPU/GL (textura/FBO/buffer/VAO/programa) no ciclo de `App`/`UiLayer` e load/unload repetido -- o que ASan não vê (handle é do driver, não heap). Contexto GL debug (`GL_KHR_debug`) ou accounting `glGen`↔`glDelete` por N ciclos. **Absorve** a cobertura do `GlStateGuard` (ex-INBOX). Endurance-lite (risco no host de longa duração: GusWorld). Pull. Ver `TESTES.md` | Média | L1-API, L2-EMBED | Média | ⏳ Pendente | — |
| TST-L1-CONTRACT | LW-TST | C++23/Testes | Contrato público de embed (`docs/embed-integration.md`) exercido como host real: **só API pública** (FetchContent + `find_package`), sem includes internos, nos 2 modos (GLFW=ON e embed-only OFF); `load`+`render()` num FBO + leitura de pixel; falha se a superfície quebrar/vazar tipo de terceiro. **Entrega o `consumer-example-embed/`** e o harness `test_host.hpp` que remove os includes `"../src/..."` da suíte (ex-INBOX). Pull. Ver `TESTES.md` | Média | L1.8-FINDPKG, L2-EMBED | Média | ⏳ Pendente | — |
| TST-L1-PRECI | LW-TST | C++23/Testes | Pré-CI local (`tools/preci.sh`): espelha o gate -- 2 configs (GLFW ON/OFF) + build + `ctest` sob Xvfb + os gates hoje ausentes do CI (STATIC/MEM/SECRETS). Local-first (soberania; offline exceto o FetchContent do RmlUi). Consolida quando ≥2 gates existirem. Ver `TESTES.md` | Baixa | TST-L1-STATIC, TST-L1-SECRETS, TST-L1-MEM | Baixa | ⏳ Pendente | — |
| AUD-L1-PARSE | LW-AUD | C++23/Auditoria | **PRIORIDADE ALTA -- 2 achados reais.** Robustez de parsing de asset hostil (decode PNG/JPG/TGA em `render_gl3.cpp::LoadTexture` + laço de premultiply; `.rml`/`.rcss`). Achados: (1) **overflow inteiro** no bound `i < w*h` (`int`) no premultiply; (2) `SECURITY.md` desatualizado ("0.1.x suportada"). Fuzz (libFuzzer/AFL++), cap de dimensão (`STBI_MAX_DIMENSIONS`), DoS por alocação. Consome TST-L1-DECODE. Ver `AUDITORIAS.md` | Alta | TST-L1-DECODE, L1-BACKEND | Média | ⏳ Pendente | — |
| AUD-L1-SUPPLY | LW-AUD | C++23/Auditoria | Supply-chain/CVE: pins vs advisories (RmlUi pin SHA `2cd28864…`, stb_image v2.30 + gl3w vendorizados, FreeType/GLFW/libGL do sistema); imutabilidade dos pins, superfície mínima (`STBI_ONLY_*`), licenças rastreadas, processo de advisory no bump. `osv-scanner`/`trivy fs`. Consome TST-L1-DEPS. Ver `AUDITORIAS.md` | Média | TST-L1-DEPS, L1-BUILD | Baixa | ⏳ Pendente | — |
| AUD-L1-MEM | LW-AUD | C++23/Auditoria | Revisão de memory-safety de recurso GL/buffer manual (textura/FBO/buffer, buffer stb; ciclo `GenerateTexture`/`GlStateGuard`; UB de overflow `w*h` no premultiply). **Consome** o run ASan+UBSan de TST-L1-MEM (julgamento downstream, não re-executa o gate). Classe validada por bug real (leak stb em exceção, hoje RAII). Ver `AUDITORIAS.md` | Média | TST-L1-MEM, L2-EMBED | Baixa | ⏳ Pendente | — |
| AUD-L1-LICENSE | LW-AUD | C++23/Auditoria | Conformidade de licença pós-MPL-2.0: SPDX `MPL-2.0` em todo fonte; `NOTICE` completo (RmlUi MIT, gl3w/stb domínio público, FreeType FTL, Khronos MIT, OpenSans Apache-2.0); zero resíduo AGPL (relicenciado 2026-06-28); compat com o copyleft fraco por-arquivo. Checagem rápida (grep SPDX + diff NOTICE), não parecer jurídico. Ver `AUDITORIAS.md` | Baixa | L1-DOCS | Baixa | ⏳ Pendente | — |
| AUD-L1-BUILD | LW-AUD | C++23/Auditoria | **Deferível** (o security-engineer sugeriu dobrar em SUPPLY+MEM+PARSE no corte mínimo). Hardening de build (`-D_FORTIFY_SOURCE=3`, `-D_GLIBCXX_ASSERTIONS`, `-fstack-protector-strong`, `-Wl,-z,relro,-z,now`) + scanners como gate nos 2 CIs. Validar com `forgejo-runner exec`. Ver `AUDITORIAS.md` | Baixa | L1.2-CI, L1-BUILD | Média | ⏳ Pendente | — |
| L1.7-BACKENDS | LW7 | C++23/Plataforma | Backends SDL/X11 (adiados da v1; só GLFW entregue) | Baixa | L1-BACKEND | Alta | ⏳ Pendente | — |
| L1-INTERNALIZE | LW8 | C++23/Loucura | Trilha de internalização clean-room (peças da Camada 1 → reescritas sobre a Camada 0). Pós-MVP. **Épico de fim-de-projeto:** independência clean-room das libs userspace (RmlUi/gl3w/FreeType/GLFW); fronteira irredutível = libGL/driver/kernel | Média | L1-DEMO | Alta | 💡 Decisão tomada | — |

## v2 — Component Library (Atomic Design) — SPEC APROVADO — modo PULL INCREMENTAL (não agendada)

> **Decisão do líder (2026-06-29):** o glintfx será **distribuído e reusado em vários projetos** ("todas as features") → vira **produto distribuível**, o que justifica uma **component library / design system** (Atomic Design) sobre o engine da v1. Sequência decidida: **fechar a v1 primeiro**; a v2 é a próxima fase.
>
> **Escopo/breakdown a definir pela bigtech (não autorar inline):** inventário de componentes, tokens, variantes de efeito, faseamento e modelo de distribuição da v2 serão definidos em **brainstorm próprio** por **Capitolino/CPO** (produto) + **ux-ui-designer** (design system) + frontend/qa, com spec e plano próprios — **só quando a v2 iniciar**. Avaliação inicial em `AtomicEval` (ux-ui-designer): tokens-first, faseado por demanda, anti-OE.
>
> **Reroteamento do líder (2026-07-01, via `/bigtech`):** modo **pull incremental** — a F2 NÃO é disparada agora como projeto de 7 tarefas. As 4 releases mais úteis da F1 (v0.2.1–v0.2.4) nasceram de pull real do GusWorld sobre o *engine*; nenhuma exigiu component library, e o cockpit já funciona ponta-a-ponta sem ela. F2 vira **backlog puxado por demanda**: só inicia com um achado real (2º consumidor, ou pedido concreto do GusWorld) que a exija. Branch `feat/v2-f2-components` **parada, não abandonada** — plano pronto pra retomar quando o pull aparecer. Detalhe: evento do vault `Journal/eventos/2026-07-01-glintfx-reroteamento-lean.md` e `.bigtech-porte`.

| ID | Onda | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| L2-BRAINSTORM | V2W1 | v2/Produto | **Porta de entrada da v2 — CONCLUÍDA.** Brainstorm bigtech (CPO+ux-ui-designer) + verdito de integração GusWorld (software-architect) + [ADR-0008](docs/adr/0008-embed-guest-mode.md) (embed mode) + [spec da v2](docs/superpowers/specs/2026-06-30-glintfx-v2-design.md) (aprovado pelo líder) | Alta | v1 fechada | Média | ✅ Concluído | ✓ |
| L2-EMBED | V2W2 | v2/Arquitetura | **Keystone (F1) — ENTREGUE:** embed/guest mode `UiLayer` (anexa ao contexto GL do host, compose-only, eventos injetados+gamepad, `GlStateGuard`). SDD T1–T6, suíte 10/10, review final internal-auditor PRONTO; mergeada na main, **tag `v0.2.0`**. **OWD** | Alta | L2-BRAINSTORM | Alta | ✅ Concluído | ✓ |
| L2-COMPONENTS | V2W3 | v2/UI | **F2 (pull incremental, 2026-07-01):** tokens-first + component library de UI de jogo (button/panel/dialog/menu/label, efeitos como modificadores) + `glintfx::ui` opt-in. Plano pronto em `docs/superpowers/plans/2026-06-30-glintfx-v2-f2-components.md`, branch `feat/v2-f2-components` parada — só inicia com demanda real confirmada (não agendada) | Média | L2-EMBED | Alta | ⏳ Pendente | — |
| L2-GUSWORLD | V2W4 | v2/Integração | **F3:** GusWorld adota o glintfx via embed (ADR-010) no cockpit — **substancialmente feito na prática** (4 releases consumer-driven fecharam o arco ponta-a-ponta, cockpit funcional sem depender de L2-COMPONENTS). Retirada formal do HUD RmlUi 6.2 vendorizado (branch `wip/rmlui-gl3-cockpit` no repo GusWorld) ainda não confirmada desta sessão — checar no próximo relay. Esforço no repo GusWorld | Média | L2-EMBED | Alta | 🔍 Pendente verificação | — |

## INBOX (descobertas não priorizadas)

**Ativos (com gatilho real):**

- **`glintfxConfig.cmake.in`** (Minor · gatilho: próximo toque no arquivo / follow-up de `L1.8-FINDPKG`): `unset()` sem `CACHE` não limpa entradas de cache de `find_library` (funcional pelo guard `if(NOT TARGET)`, mas semanticamente incorreto).
- **`.github/workflows/ci.yml`** (Minor · gatilho: próxima varredura de robustez de CI): `libxkbcommon-dev` ausente (presente no Forgejo; implícito por transitividade no `ubuntu-latest`) -- adicionar p/ consistência/robustez.
- **Em-dashes pré-existentes** em doc-comments de `ui_layer.hpp` (gatilho: qualquer varredura de doc; **candidato a varrer junto de `L1.10-APIDOC`**, mesmo header): trocar por `--`.
- **GAP-2 FBO custom + origem de viewport** (feature p/ F2/F3, pedido GusWorld · **ADIADO, sem demanda confirmada**): permitir compor num FBO do host (≠ FBO 0) e viewport com origem ≠ (0,0). Hoje o backend RmlUi hardcoda FBO 0 + origem (0,0). GusWorld compõe no FBO 0.
- **GAP-4 `set_focus(id)` no `UiLayer`** (feature p/ v2 · **semente, sem demanda, NÃO urgente**): expor `set_focus(const char* id)`/`focus_element(id)` para hosts cujo MODELO é dono da seleção (ex.: GusWorld, canon 1B). **Contornado** no GusWorld (foco glintfx inerte, sem `:focus`; destaque 100% por data-binding `data-class-sel`). Boa candidata pra v2 (componentes focáveis).
- **UA-stylesheet: recompile redundante por load** (Minor · **sem payoff -- inerente à mesclagem via API pública pós-load**): quando o documento tem `<link>`/`<style>` próprio, a stylesheet original é compilada no `LoadDocument` e descartada quando o `Bootstrap::load()` a substitui pela mesclada.

**Promovidos/absorvidos na reconciliação (2026-07-01 · não re-adicionar):**

- Gate de encapsulamento (unificar a regex `(Rml|glfw|gl3w|GL/|stb|SDL)`) → sub-tarefa de **`TST-L1-STATIC`** (LW-TST).
- `version()` no build embed-only → **`L1.9-VERSEMBED`** (LW5).
- Doc de moved-from + `UiEvent::Type` default → **`L1.10-APIDOC`** (LW5).
- `consumer-example-embed/` → entregue por **`TST-L1-CONTRACT`** (LW-TST).
- Includes relativos `"../src/..."` na suíte → harness `test_host.hpp` de **`TST-L1-CONTRACT`** (LW-TST).
- Cobertura do `GlStateGuard` → **`TST-L1-GLLEAK`** (LW-TST) + auditoria **`AUD-L1-MEM`** (LW-AUD).

**Concluídos (histórico):**

- ~~GAP-1 `dp_ratio`~~ ✅ **entregue na v0.2.2** (`set_dp_ratio` + `dp_ratio` no config, UiLayer+App).
- ~~GAP-3 base-url override~~ ✅ **entregue na v0.2.2** (`set_asset_base_url` via `BaseUrlFileInterface`, UiLayer+App).
- ~~DOC-README (achado `/bigtech` 2026-07-01, Minor): README/AGENTS/getting-started apontavam `v0.2.3`/`v0.1.0`~~ ✅ **resolvido em c5a2072** -- sincronizados p/ `v0.2.4` (badge, `GIT_TAG` EN+PT, "produto ativo", status do AGENTS, FetchContent do tutorial); varredura do repo concluída. Follow-ups só-versão sinalizados (fora do escopo estrito): a data `2026-06-30` de "Current release" (a v0.2.4 saiu 2026-07-01), a linha "v0.1.0 is honest" no README e o snapshot datado do `CLAUDE.md` (versão soldada a data/contagem de testes).
