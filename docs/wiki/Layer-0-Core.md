# Layer-0-Core

## English

**`loucura_c_asm`** ("madness in C and Assembly", the project's own self-aware name for itself) is an experimental, sovereign runtime for Linux x86-64, written in pure C and Assembly, with a single non-negotiable rule: **zero libraries, not even the C standard library (libc)**. There is no `printf`, no `malloc`, no `strlen` from anywhere else; whatever is needed is built from scratch, and the only connection to the operating system is the raw Linux kernel **syscall** interface.

**Status:** implementation complete and audited (bootstrap, its own test harness, a small core "libc" for memory/strings/number conversion, a mini-printf, a free-list memory allocator over `mmap`, plus a clean-room `SOV-SFNT` TrueType/OpenType parser and a `SOV-RAST` anti-aliased rasterizer -- the foundation of a FreeType internalization), tagged `core-v0.4.0`. Two audit rounds -- the original `core-v0.1.0` pass, then a `core-v0.4.0` delta re-audit covering everything built since -- both closed "compliant with caveats", zero critical findings; one of the caveats was a real bug (the text-to-number conversion function, `atof`, could mis-compute very long decimal numbers) that got fixed and re-verified. This is a **long-term, no-deadline track**, not a product with users yet.

### Why this exists

This is the long-term foundation for the project's "internalization" ambition: over years, pieces of [Layer 1 (glintfx)](Layer-1-glintfx)'s third-party dependencies (RmlUi, FreeType, GLFW) may get rebuilt clean-room on top of this sovereign runtime, wherever it is worth the effort. Two pieces have already made that journey: glintfx's own OpenGL loader (`L1.14-GLLOADER`), and -- as of `core-v0.3.0` -- the `SOV-SFNT`/`SOV-RAST`/`SOV-HINT` font engine that has been glintfx's **default** text rasteriser since `v0.10.0` (see [Layer-1-glintfx](Layer-1-glintfx)'s "What's new" section and [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md)). See [ADR-0009](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0009-internalization-boundary.md) for the exact, honest boundary of what will (and will not) ever be internalized: the GPU driver and the kernel itself are explicitly out of scope, forever.

### Building and testing it

No system packages are needed, that is the entire point:

```sh
make build          # compile + link everything (src/*.c + src/*.asm, plus every tests/*.c program)
make test           # run every test program, check exit code (and sometimes exact output) against a recorded expectation
make check-static   # cppcheck + clang --analyze, static analysis without running anything
make test-mem       # valgrind memcheck over the memory allocator
```

### Where to go

| Need | Link |
| :--- | :--- |
| Total-beginner walkthrough | [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md), section 5 |
| Exact build flags and why each one exists | [`CLAUDE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CLAUDE.md), section "Camada 0" |
| Design decisions (syscall style, error contract, ABI, allocator strategy, ELF layout) | [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md), ADR-0001 through ADR-0005 |
| The live task-tracking table for this track | [`TODO.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/TODO.md), the Camada 0 (`A*`/`B*`/...) rows |
| Testing philosophy for this track | [`TESTES.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/TESTES.md) |

---

## Português

O **`loucura_c_asm`** ("loucura em C e Assembly", o próprio nome autoconsciente que o projeto se dá) é um runtime soberano e experimental para Linux x86-64, escrito em C e Assembly puros, com uma única regra inegociável: **zero bibliotecas, nem mesmo a biblioteca padrão de C (libc)**. Não há `printf`, `malloc`, `strlen` vindo de lugar nenhum externo; tudo que é preciso é construído do zero, e a única conexão com o sistema operacional é a interface crua de **syscall** do kernel Linux.

**Status:** implementação completa e auditada (bootstrap, harness de teste próprio, uma "libc"-núcleo pequena de memória/string/conversão de número, um mini-printf, um alocador free-list de memória via `mmap`, mais um parser TrueType/OpenType `SOV-SFNT` clean-room e um rasterizador anti-aliased `SOV-RAST` -- a fundação de uma internalização do FreeType), taggeada `core-v0.4.0`. Duas rodadas de auditoria -- a passagem original da `core-v0.1.0`, depois uma re-auditoria delta da `core-v0.4.0` cobrindo tudo que foi construído desde então -- fecharam as duas em "conforme com ressalvas", zero achado crítico; uma das ressalvas era um bug real (a função de conversão texto-pra-número, `atof`, podia calcular errado números decimais bem longos) que foi corrigido e re-verificado. Esta é uma **trilha de longo prazo, sem prazo**, não um produto com usuários ainda.

### Por que existe

Esta é a fundação de longo prazo da ambição de "internalização" do projeto: ao longo de anos, pedaços das dependências de terceiros da [Camada 1 (glintfx)](Layer-1-glintfx) (RmlUi, FreeType, GLFW) podem ser reconstruídos clean-room em cima deste runtime soberano, onde valer o esforço. Duas peças já fizeram essa jornada: o loader próprio de OpenGL do glintfx (`L1.14-GLLOADER`), e -- desde `core-v0.3.0` -- o motor de fonte `SOV-SFNT`/`SOV-RAST`/`SOV-HINT` que é o rasterizador de texto **default** da glintfx desde a `v0.10.0` (ver a seção "O que há de novo" de [Layer-1-glintfx](Layer-1-glintfx) e a [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md)). Ver [ADR-0009](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0009-internalization-boundary.md) pra fronteira exata e honesta do que será (e não será) internalizado algum dia: o driver de GPU e o próprio kernel estão explicitamente fora de escopo, pra sempre.

### Buildando e testando

Nenhum pacote de sistema é necessário, esse é o ponto inteiro:

```sh
make build          # compila + linka tudo (src/*.c + src/*.asm, mais todo programa tests/*.c)
make test           # roda todo programa de teste, confere o código de saída (e às vezes a saída exata) contra uma expectativa gravada
make check-static   # cppcheck + clang --analyze, análise estática sem rodar nada
make test-mem       # valgrind memcheck sobre o alocador de memória
```

### Pra onde ir

| Preciso de | Link |
| :--- | :--- |
| Passo a passo pra iniciante total | [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md), seção 5 |
| Flags exatas de build e por que cada uma existe | [`CLAUDE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CLAUDE.md), seção "Camada 0" |
| Decisões de design (estilo de syscall, contrato de erro, ABI, estratégia do alocador, layout ELF) | [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md), ADR-0001 a ADR-0005 |
| A tabela viva de rastreio de tarefas desta trilha | [`TODO.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/TODO.md), as linhas da Camada 0 (`A*`/`B*`/...) |
| Filosofia de teste desta trilha | [`TESTES.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/TESTES.md) |
