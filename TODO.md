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
| REL-TAG | W13 | Release | Tag de versão `v0.1.0` (primeiro runtime funcional + libc própria mínima) | Média | F1, AUD-ABI, AUD-SEC | Baixa | ⏳ Pendente | — |
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

| ID | Onda | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| L1-CLONE | LW1 | C++23/Setup | Clonar RmlUi + gl3w em `examples/` (gitignored) para estudo/RE | Alta | — | Baixa | ✅ Concluído | — |
| L1-BRAINSTORM | LW1 | C++23/Produto | Brainstorm bigtech: escopo/efeitos/nome. Gerou spec + plano + ADR-0006/0007; nome **glintfx**, licença MPL-2.0 | Alta | — | Média | ✅ Concluído | ✓ |
| L1-BUILD | LW2 | C++23/Build | Esqueleto CMake + FetchContent (RmlUi 6.3 + gl3w vendorizado) **pronto**; install/export + alias + consumer drop-in via FetchContent entregues (Task 8) | Alta | L1-CLONE | Média | ✅ Concluído | ✓ |
| L1-BACKEND | LW3 | C++23/Plataforma | Janela/contexto **GLFW** + wrap `RenderInterface_GL3` + bootstrap RmlUi/FreeType; fix backbuffer opaco (alpha×compositor). SDL/X11 adiados | Alta | L1-BUILD | Alta | ✅ Concluído | ✓ |
| L1-DEMO | LW4 | C++23/Demo | Showcase com os **4 efeitos** (glow, degradê, backdrop-blur, mask) — validado em GPU real (Intel+NVIDIA/Wayland) pelo líder 2026-06-29 | Alta | L1-BACKEND, L1-BRAINSTORM | Média | ✅ Concluído | ✓ |
| L1-API | LW4 | C++23/API | Fachada `glintfx::App` RAII (pImpl, headers limpos), compat C++17→23 | Alta | L1-BRAINSTORM | Alta | ✅ Concluído | ✓ |
| L1-INTERNALIZE | LW5 | C++23/Loucura | Trilha de internalização clean-room (peças da Camada 1 → reescritas sobre a Camada 0). Pós-MVP | Média | L1-DEMO | Alta | 💡 Decisão tomada | — |

## v2 — Component Library (Atomic Design) — PRÓXIMO (não iniciado)

> **Decisão do líder (2026-06-29):** o glintfx será **distribuído e reusado em vários projetos** ("todas as features") → vira **produto distribuível**, o que justifica uma **component library / design system** (Atomic Design) sobre o engine da v1. Sequência decidida: **fechar a v1 primeiro**; a v2 é a próxima fase.
>
> **Escopo/breakdown a definir pela bigtech (não autorar inline):** inventário de componentes, tokens, variantes de efeito, faseamento e modelo de distribuição da v2 serão definidos em **brainstorm próprio** por **Capitolino/CPO** (produto) + **ux-ui-designer** (design system) + frontend/qa, com spec e plano próprios — **só quando a v2 iniciar**. Avaliação inicial em `AtomicEval` (ux-ui-designer): tokens-first, faseado por demanda, anti-OE.

## INBOX (descobertas não priorizadas)

- **`App` estratégia de erro de construção** (hardening DX p/ distribuição): hoje a construção falha silenciosamente (v1 só documenta — N2). Considerar `bool ok()` e/ou exceptions opcionais.
- **CI versionado** (N5): Forgejo Actions / Woodpecker para gate automático da suíte glintfx (4 smokes + `render_sanity`) sob Xvfb. Hoje os testes existem mas nada automatiza o gate.
- **`snapshot()` flip vertical**: captura sai de cabeça pra baixo (origem do `glReadPixels`); endireitar as linhas.
- **Guards null** (T2/T5): `window_glfw` (`make_current`/`swap`/`poll` sem checar `win_`; `glfwTerminate` sem flag `glfw_inited_`) e `App` (`poll_events`/`render` sem `impl_->ok`). Não crasham (no-op downstream), só assimetria.
- **`body { width: 100% }`**: fundo do demo não cobre a janela inteira.
- **`TESTES.md`** (T7): corrigir `xvfb-run` citado na seção "GPU real" (Xvfb usa llvmpipe, não GPU real).
- **Backends SDL/X11**: adiados da v1 (só GLFW entregue).
- **`find_package(glintfx)` completo** (pós-v1): `glintfxConfig.cmake` + `find_dependency()` + `install(EXPORT)` — removido na Task 8 por atrito com export-set do CMake 4.x. Consumo hoje via FetchContent/`add_subdirectory` (drop-in provado).
- **Tag da Camada 0**: o `REL-TAG` do runtime C/ASM precisa de tag DISTINTA (ex.: `core-v0.1.0`) — o glintfx usa `v0.1.0`.
