# Beginner's guide to this repository / Guia para iniciantes deste repositório

> **Diátaxis type:** this document is an intentional hybrid (**tutorial** + **explanation**, with a **reference map** at the end) - a deliberate exception to "one document, one type." It exists as a single front door for someone who does not yet know what a tutorial, how-to, reference, or explanation even is, or how to navigate a software repository at all. Every section links out to the properly single-typed documents that already exist in `docs/`; this guide does not duplicate their content, it introduces you to them.
> **Tipo Diátaxis:** este documento é um híbrido intencional (**tutorial** + **explicação**, com um **mapa de referência** ao fim) - uma exceção deliberada à regra "um documento, um tipo". Ele existe como porta de entrada única para quem ainda não sabe o que é um tutorial, um how-to, uma referência ou uma explicação, nem como navegar um repositório de software. Cada seção linka para os documentos já existentes em `docs/` (esses sim de tipo único); este guia não duplica o conteúdo deles, ele te apresenta a eles.

- **Audience / Audiência:** someone with **little or no prior computing knowledge** - you do not need to know what "compiling" or "a library" means before starting. / Alguém com **pouco ou nenhum conhecimento prévio de computação** - você não precisa saber o que é "compilar" ou "uma biblioteca" antes de começar.
- **Owner:** `technical-writer` agent (loucura_c_asm project) - maintained alongside the codebase.
- **Last reviewed / Última revisão:** 2026-07-16, against `glintfx v0.11.0` and Layer 0 tag `core-v0.3.0`.
- **Found something wrong? / Achou algo errado?** Open an issue on the repository (Codeberg or GitHub, links below) describing what you read and what actually happened on your machine. / Abra uma issue no repositório (Codeberg ou GitHub, links abaixo) descrevendo o que você leu e o que de fato aconteceu na sua máquina.

---

## English

### 0. Before you start: two extra jargon words you'll see immediately

You'll meet most jargon in context below (each term is explained the first time it appears, and again in the [Glossary](#glossary) at the end). Just two words up front, because you'll see them in literally the first paragraph:

- **Repository ("repo"):** a folder whose history of changes is tracked by a tool called **git**. Every file you'll read about lives inside one repository - this one.
- **Codeberg / GitHub:** two websites that host git repositories "in the cloud" so people can download (`clone`) and collaborate on them. This project publishes to both:
  - Codeberg (primary): https://codeberg.org/petrinhu/glintfx
  - GitHub (mirror): https://github.com/petrinhu/glintfx

### 1. What even is this project? (the two-floor-building analogy)

This single repository actually contains **two very different pieces of software**, called "layers" in this project's own vocabulary. Think of it as a building with two floors, built by the same person, for two different reasons:

```
┌───────────────────────────────────────────────────┐
│  FLOOR 1 (Layer 1) - glintfx                       │
│  A furnished, livable apartment.                   │
│  You move in today, use the front door (its public │
│  API), and don't worry about the plumbing behind    │
│  the walls (OpenGL, RmlUi, GLFW).                   │
│  → THIS IS THE ACTIVE, RELEASED PRODUCT.            │
├───────────────────────────────────────────────────┤
│  FLOOR 0 (Layer 0) - loucura_c_asm                 │
│  Raw concrete, poured and shaped by hand, one bag   │
│  at a time, with no pre-fabricated construction     │
│  materials at all. Nobody lives here yet - it's a   │
│  from-scratch craftsmanship project, built to learn │
│  and, eventually, to pour the floor above onto it.  │
│  → THIS IS THE LONG-TERM, EXPERIMENTAL TRACK.       │
└───────────────────────────────────────────────────┘
```

**Floor 1 - `glintfx`** is a **library** (a piece of reusable code that other programs can plug into - like a power tool you buy instead of forging yourself) written in the **C++** programming language. It lets a programmer build a graphical user interface - buttons, panels, menus - described in files that look like web pages (**HTML**/**CSS**), while getting nice visual effects (glow, blur, shadows, gradients) computed by the computer's graphics card (the **GPU**) for free. It is **released** (people can already use it) and is the project's **active product**.

**Floor 0 - `loucura_c_asm`** ("`loucura`" is Portuguese for "madness" - the project's own self-aware name for itself) is an experiment in building a tiny operating-system-adjacent runtime **from absolutely nothing**: no libraries, not even the most basic one every C program normally relies on (called the **libc**, short for "C standard library" - see the glossary). Every single piece of it - how to print text, how to read keyboard input, how to allocate memory - is written by hand in **C** and **Assembly** (the lowest-level human-readable programming language, one step above the raw binary numbers the processor executes). Its only connection to the outside world is the Linux kernel's raw **syscall** interface (see glossary). This track has **no deadline**; it is a long-term craftsmanship/learning project. Its own implementation is now **complete and awaiting a final audit**, but it is not yet a released product like glintfx - nothing depends on it in production.

**Why do both live in one building?** Floor 0 is the long-term "internalize everything, trust nothing you didn't build" ambition. Floor 1 is what pays the bills today (or would, if this were commercial): a real, usable library, built pragmatically on top of well-established third-party pieces (RmlUi, GLFW, FreeType - real, mature open-source libraries). Over years, pieces of Floor 1's foundations may get rebuilt on top of Floor 0, one piece at a time, wherever it's worth the effort. One piece already made that journey: glintfx's own OpenGL "loader" (a small piece of plumbing explained below) was rewritten from scratch, replacing a third-party one. See [`docs/adr/0009-internalization-boundary.md`](adr/0009-internalization-boundary.md) for the precise (and honest) technical boundary between the two floors.

**If you only care about using a graphics library today:** skip straight to [section 4](#4-first-contact-building-and-running-glintfx-floor-1).
**If the "build everything from zero" idea is what excites you:** skip to [section 5](#5-first-contact-building-and-testing-layer-0-floor-0).

### 2. Words you'll need before touching a keyboard

These are explained in full, alphabetically, in the [Glossary](#glossary) - but here are the five you'll hit immediately:

| Word | Plain-language meaning |
| :--- | :--- |
| **Compile** | Translate human-readable source code (text files you or someone else wrote) into a form the processor can actually execute. Like translating a recipe written in English into a sequence of exact hand motions. |
| **Library** | Pre-written code that solves a problem once, so other programs can reuse it instead of rewriting it. `glintfx` is a library. |
| **Link (linking)** | The step, after compiling, that stitches multiple compiled pieces (your code + the libraries it uses) into one finished, runnable program. |
| **Terminal / shell** | A text-based window where you type commands instead of clicking icons. Every command in this guide is meant to be typed into one. On Linux, common ones are called `bash` or `zsh`. |
| **CLI (command)** | A line of text you type into a terminal and press Enter to run - e.g. `cmake --version`. |

### 3. Map of the repository

```
loucura_c_asm/            <- the repo root (this file's folder's parent)
├── glintfx/               Floor 1 - the C++ library itself
│   ├── include/glintfx/   its PUBLIC headers (the only files a consumer #includes)
│   ├── src/                its internal implementation (.cpp files)
│   ├── demos/showcase/    a runnable demo program exercising every effect
│   ├── tests/              its automated test suite (ctest)
│   └── CMakeLists.txt     its build recipe (see "What is CMake?" in the Glossary)
├── src/                   Floor 0 - the C+ASM runtime's source code (.c and .asm files)
├── include/               Floor 0 - its header files (.h and .inc)
├── tests/                 Floor 0 - its own test programs and expected-output files
├── docs/                  documentation for BOTH floors
│   ├── adr/                Architecture Decision Records - WHY a design was chosen (see Glossary: ADR)
│   ├── getting-started.md  developer tutorial for glintfx (assumes you already know C++/CMake)
│   ├── effects.md          how-to + reference for glintfx's visual effects (RCSS syntax)
│   ├── embed-integration.md  reference for the advanced "embed mode" of glintfx
│   ├── wiki/                source files for this project's Codeberg/Forgejo Wiki (see below)
│   └── GUIA_INICIANTE.md   <- you are reading this file right now
├── Makefile                the build recipe for Floor 0 (see section 5)
├── README.md               the project's front page (bilingual, both floors)
├── CHANGELOG.md            a dated history of every released glintfx version
├── TODO.md                 the project's live task-tracking table ("what's being worked on")
└── LICENSE / NOTICE        the legal terms (MPL-2.0) and third-party credits
```

### 4. First contact: building and running glintfx (Floor 1)

This walks you through the exact commands from this repository's own build instructions (they are the source of truth: [`CLAUDE.md`](../CLAUDE.md) and [`README.md`](../README.md)). If a command below ever stops matching those files, trust the other two files - they are updated by the same automated check as this one, but this guide could still lag.

**Step 1 - install the system packages glintfx needs.** These are pre-built libraries (see Glossary: *library*) glintfx relies on instead of reinventing (a windowing/input library called **GLFW**, a font-rendering library called **FreeType**, and the graphics driver interface, **OpenGL**, via **Mesa**). On Fedora Linux:

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel
```

> On a different Linux distribution, install the equivalents (e.g. on Debian/Ubuntu: `libglfw3-dev`, `libfreetype-dev`, `libgl1-mesa-dev`). This project only officially supports Linux on x86-64 processors (the kind found in almost every desktop/laptop PC, as opposed to, say, a Raspberry Pi's ARM chip).

**Step 2 - configure and build.** "Configure" means running **CMake** (see Glossary), a tool that reads `CMakeLists.txt` and generates the actual low-level build instructions for your machine; "build" means actually compiling and linking everything.

```sh
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build
```

(`-S glintfx` = "the source is in the `glintfx/` folder"; `-B glintfx/build` = "put the generated build files in a new `glintfx/build/` folder", which is disposable - deleting it and re-running the two commands above always works.)

**Step 3 - run the bundled demo.** This is a small program, already written for you, that opens a window and shows all of glintfx's visual effects at once:

```sh
./glintfx/build/demos/showcase/glintfx_showcase
```

If a window opens showing glowing, blurred, gradient-filled panels - it worked. Close the window (or `Ctrl+C` in the terminal) to exit.

**Step 4 (optional) - run the automated test suite.** This compiles a second time with testing enabled, then runs every automated check the project ships with:

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j
ctest --test-dir glintfx/build --output-on-failure
```

`ctest` will print one line per test (`Passed`/`Failed`). The suite keeps growing every release (it was 38 tests at the time this guide was first written, and had grown past 50 as of `v0.11.0` - see [`CHANGELOG.md`](../CHANGELOG.md) for the exact, always-current count of each release; there is also a smaller, alternate "embed-only" build configuration - not something you need for first contact).

**Where to go deeper from here:** [`docs/getting-started.md`](getting-started.md) walks through *writing your own* tiny glintfx program (not just running the bundled demo) - it assumes you already know some C++ and CMake, which you now have a first taste of.

### 4.5. What changed since v0.9.0 (three things worth knowing)

This guide was first written against `glintfx v0.9.0`. Three consumer-visible things changed in the three releases since then. Explained here in beginner terms; each links to the properly single-typed document that has the full technical detail.

1. **glintfx now draws its own letters by default (`v0.10.0`).** Any program that shows text (buttons, labels, a game's heads-up display) needs a piece of software called a **font rasteriser**: it takes a font file (like a `.ttf`) and figures out exactly which dots ("pixels") on your screen need to light up, and in what shade, to draw a given letter at a given size. Up through `v0.9.0`, glintfx always used a mature, well-known third-party rasteriser called **FreeType** for this. Since `v0.9.1`, glintfx also has **its own, hand-written rasteriser** - part of the long-term "build it yourself" ambition from [section 1](#1-what-even-is-this-project-the-two-floor-building-analogy) above (its lowest-level math, reading the font file's bytes and turning outlines into pixels, is written on **Floor 0**, this project's from-scratch runtime - one more piece that "moved downstairs"). Since `v0.10.0`, that own rasteriser is the **default** one glintfx uses. **Nothing changes for you if you never touch this setting** - and if your own program's text ever looks wrong after upgrading, you can switch back to the old default with a single line of code, no reinstalling or rebuilding required:
   ```cpp
   glintfx::AppConfig cfg;
   cfg.font_engine = glintfx::FontEngine::FreeType;  // rolls back to the pre-v0.10.0 default
   ```
   See [ADR-0011](adr/0011-soft-font-flip.md) for why this was done as a reversible "soft flip" rather than a one-way switch.
2. **A screen "ripple" visual effect exists now (`v0.11.0`), for programs that embed glintfx inside a bigger app (like a game).** Picture the ring of ripples that spreads outward when you drop a stone in still water - that is roughly the visual. It is written entirely in the same CSS-like stylesheet language (`.rcss`) every other glintfx effect uses, as `decorator: ripple()` - no C++ code required to draw it. It only does anything in "embed mode" (see the [Glossary](#glossary)); it is a no-op in the standalone window mode this guide's [section 4](#4-first-contact-building-and-running-glintfx-floor-1) walked you through. See [`docs/effects.md`](effects.md) for the exact syntax.
3. **glintfx took one small step toward Windows, not a full leap (`v0.9.2`).** One small internal piece (the "GL loader" - the bit of plumbing that finds the graphics card's drawing functions on whatever machine the program runs on) learned how to do that job on Windows too, and this was proven by compiling it *for* Windows *from* a Linux machine (a technique called **cross-compiling**). This is a real, useful step, but it is **not** the same as "glintfx fully supports Windows today": the project's one officially supported platform remains Linux on x86-64 processors (see the main [`README.md`](../README.md#known-limitations)'s "Known limitations"). If you are curious about a full Windows build, ask on the project's issue tracker before assuming everything works end-to-end.

### 5. First contact: building and testing Layer 0 (Floor 0)

This is the "from raw concrete" side. Nothing here needs any system package - that is the entire point: **zero libraries, not even the most basic one.**

**Step 1 - check your tools.** You need `nasm` (the assembler - a program that turns Assembly-language text into machine code), `clang` (the C compiler), and GNU `ld` (the linker). This project's own environment already has them; check yours with:

```sh
nasm -v
clang --version
ld --version
```

**Step 2 - build everything with `make`.** `make` is a tool that reads a `Makefile` (a recipe file) and runs exactly the compile/assemble/link commands needed, in the right order, without you typing each one by hand:

```sh
make build
```

This compiles every `.c` file in `src/` (the shared runtime - this project's own hand-written substitute for a libc) and every `.c` file in `tests/` (small standalone programs, each one a `main` that exercises a piece of the runtime), then links them, with **zero use of any system library**.

**Step 3 - run the test suite.** Every compiled test program is run, and its exit code (and in some cases its exact text output) is checked against a recorded expected value:

```sh
make test
```

You'll see a `PASS`/`FAIL` line per program. All of them should say `PASS` (as of this writing, this pipeline is implementation-complete and passing, though still awaiting a final external audit before being marked fully verified in `TODO.md`).

**Step 4 (optional) - the other checks the project runs on itself:**

```sh
make check-static   # static analysis (cppcheck + clang --analyze) -- finds bugs WITHOUT running the code
make test-mem        # valgrind memcheck over the memory allocator -- catches gross memory corruption
make run             # runs every test program live and prints its output + exit code
make clean           # deletes the build/ folder (safe -- just re-run `make build` afterwards)
```

**Where to go deeper from here:** the [`CLAUDE.md`](../CLAUDE.md) file's "Camada 0" section explains every compiler/linker flag used and *why* (e.g. why `-nostdlib`, why a hand-written `_start`); [`docs/adr/`](adr/README.md) records the actual design decisions (syscall style, error handling, memory allocator strategy, ELF binary layout) with their reasoning.

### 6. Where to go next (a map of the "real" documentation)

This guide is a front door, not a full house. Once you're oriented, these are the properly single-purpose documents (see [What is "Diátaxis"?](#what-is-diátaxis-the-four-kinds-of-documentation) below) to read next:

| I want to... | Read this |
| :--- | :--- |
| Write my own small glintfx program, step by step | [`docs/getting-started.md`](getting-started.md) *(tutorial)* |
| Look up the exact CSS-like syntax for a visual effect | [`docs/effects.md`](effects.md) *(how-to + reference)* |
| Embed glintfx inside a bigger application (e.g. a game) that owns its own window | [`docs/embed-integration.md`](embed-integration.md) *(reference)* |
| Install a pre-built copy of glintfx instead of building from source every time | [`docs/packaging.md`](packaging.md) *(how-to)* |
| Fix a build/integration error I'm stuck on | [`docs/troubleshooting.md`](troubleshooting.md) *(how-to)* |
| Understand *why* a design decision was made | [`docs/adr/README.md`](adr/README.md) *(explanation, one file per decision - "ADR")* |
| See the exact history of every released version | [`../CHANGELOG.md`](../CHANGELOG.md) *(reference)* |
| See what's currently being worked on, and what's planned | [`../TODO.md`](../TODO.md) *(reference - this is a live project-management table, more technical/dense than the rest)* |
| Understand the project's testing philosophy and how to run every kind of test/audit | [`../TESTES.md`](../TESTES.md) *(explanation + how-to)* |
| Contribute code myself | [`../CONTRIBUTING.md`](../CONTRIBUTING.md) |
| Report a security problem | [`../SECURITY.md`](../SECURITY.md) |

### What is "Diátaxis"? (the four kinds of documentation)

You'll see the word **Diátaxis** mentioned across this project's documentation - it's a documentation framework (a set of rules for organizing docs) this project follows deliberately. The idea: any piece of documentation is trying to do exactly one of four different jobs, and mixing jobs in one document makes it worse at all of them:

| Type | Answers... | Example in this repo |
| :--- | :--- | :--- |
| **Tutorial** | "Teach me, hand-holding, I've never done this" | `docs/getting-started.md` |
| **How-to** | "I know roughly what I'm doing, show me how to solve THIS specific problem" | `docs/effects.md`, `docs/troubleshooting.md` |
| **Reference** | "I need one exact fact - a parameter name, a function signature" | `docs/embed-integration.md`, the public headers in `glintfx/include/glintfx/` |
| **Explanation** | "I want to understand WHY, not just HOW" | `docs/adr/*.md` |

This guide you're reading is the one deliberate exception, because a total beginner doesn't yet know which of the four kinds of document they need - this document's whole job is to get you oriented enough to pick the right one next.

### About the project's Wiki

Besides the files in `docs/`, this project also publishes a lightweight **Wiki** (a separate, small collection of short, heavily cross-linked pages, hosted natively by Codeberg/GitHub rather than living as files in this repository's main history). The Wiki's *source* files are prepared in [`docs/wiki/`](wiki/) in this repository - see [`docs/wiki/README.md`](wiki/README.md) for what the Wiki is for and how it gets published.

### Glossary

Alphabetical. Every term used anywhere above is defined here again, in isolation, so you can jump straight to a word without re-reading the section it came from.

- **ADR (Architecture Decision Record):** a short document recording ONE design decision, why it was made, what alternatives were considered, and why they were rejected. This project keeps them in [`docs/adr/`](adr/README.md), numbered in the order they were written; a decision, once `Accepted`, is never edited - a change of mind gets a brand-new ADR that supersedes the old one.
- **API (Application Programming Interface):** the set of functions/types a piece of code exposes for other code to call - the "front door" of a library, as opposed to its internal plumbing.
- **ASan / UBSan (AddressSanitizer / UndefinedBehaviorSanitizer):** compiler-provided tools that make certain classes of programming bugs (writing past the end of a block of memory; using memory after it was freed; undefined/nonsensical operations) crash loudly and immediately with a precise report, instead of silently corrupting things or working "by luck." This project runs its test suite under both on a schedule ("nightly").
- **Assembly (ASM):** the lowest-level programming language a human can realistically read and write; each instruction maps almost one-to-one to a single operation the processor itself performs (move this number here, add these two numbers, jump to this instruction). This project's Floor 0 uses it, via the **NASM** assembler, for the handful of operations C cannot express directly (like the very first instruction a program runs, or the raw `syscall` instruction).
- **CI (Continuous Integration):** an automated robot that builds and tests the project every single time someone proposes a change (a `push` or a Pull Request), so mistakes get caught immediately instead of days later. This project runs CI on both GitHub Actions and Codeberg's Forgejo Actions.
- **Clean-room (reimplementation):** rewriting a piece of software from an understanding of *what it should do*, without ever looking at (or copying from) the original's source code - the opposite of copy-pasting. This project's long-term "internalization" ambition is explicitly clean-room: study how something works, then write an original implementation of the same idea.
- **CLI:** see "command" in section 2 above.
- **CMake:** a tool that generates the actual low-level build instructions (for compilers/linkers) from a higher-level recipe file (`CMakeLists.txt`), so the same recipe can produce a working build across different machines/toolchains. Used by glintfx (Floor 1); Floor 0 uses a plain `Makefile` instead, deliberately, since it has no cross-platform ambitions.
- **Compile / Compiler:** see section 2. The compiler used here is **clang**.
- **Cross-compiling:** compiling a program on one kind of machine (e.g. Linux) so it can run on a different kind (e.g. Windows), without ever running the target machine yourself during the build. Used to validate glintfx's GL loader's Windows codepath (`v0.9.2`) without owning a Windows machine.
- **Drop-in (library):** a library designed so that adding it to your project is as simple as possible - "add one line, get the feature," with no manual plumbing of its internal dependencies.
- **ELF (Executable and Linkable Format):** the file format Linux uses for compiled programs and libraries - what a `.o` or a final executable file actually looks like as bytes on disk. Floor 0's [`docs/adr/0005-elf-layout.md`](adr/0005-elf-layout.md) documents the exact layout decisions made for its own hand-linked binaries.
- **Embed mode:** a way of using glintfx where it does NOT own the application's window - instead, it attaches to a window/graphics context that a *host* application (e.g. a game) already created and owns, drawing its UI on top. Contrast with the default "standalone" mode, where glintfx creates and owns its own window. See [`docs/embed-integration.md`](embed-integration.md).
- **Font rasteriser:** the piece of software that turns a font file's mathematical letter shapes into the actual coloured dots ("pixels") drawn on screen at a given size. glintfx used only the third-party FreeType rasteriser through `v0.9.0`; since `v0.10.0` it defaults to its own, hand-written one (see [section 4.5](#45-what-changed-since-v090-three-things-worth-knowing)).
- **Freestanding:** a program built with NO assumption that an operating system's standard runtime environment (libc, `main`'s usual startup code, etc.) is available - you provide literally everything yourself, down to the program's very first instruction. Floor 0 is entirely freestanding.
- **git:** the version-control tool that tracks every change ever made to this repository's files, who made it, and why (via a short message called a "commit message"). `git clone` downloads a full copy of a repository (and its history) to your machine.
- **GPU (Graphics Processing Unit):** the specialized chip (separate from the general-purpose CPU) that computes graphics - including glintfx's visual effects - extremely fast, in parallel, because that's the one thing it's built to do.
- **Header file (`.h`/`.hpp`):** a file listing WHAT a piece of code offers (function names, types) without the actual implementation - what another file `#include`s to be able to call into it, without needing to see (or recompile) the implementation itself.
- **Kernel (Linux kernel):** the core program that manages a computer's hardware (memory, processor time, disks, the screen) and is the only piece of software allowed to talk to that hardware directly; every other program, including this one's Floor 0, asks the kernel to do things on its behalf via **syscalls**.
- **libc (C standard library):** the library almost every C program in existence relies on for basics like printing text, allocating memory, or reading a string's length. Floor 0's entire reason for existing is refusing to use it, writing an equivalent from scratch instead.
- **Library:** see section 2.
- **Link / Linker:** see section 2. The linker used here is GNU **`ld`**.
- **Mermaid (diagram):** a way of drawing a diagram (flowchart, sequence diagram) as plain text, which tools like this repository's README/wiki renderer then turn into an actual picture - kept in version control just like code, instead of a separate image file that can silently go stale.
- **OpenGL:** a standard interface for talking to a GPU to draw graphics; glintfx's rendering is built on OpenGL version 3.3.
- **RAII (Resource Acquisition Is Initialization):** a C++ pattern where an object automatically cleans up after itself (closes a file, frees memory, destroys a window) the moment it goes out of scope, so a programmer cannot forget to. glintfx's public `App`/`UiLayer` types follow this pattern.
- **Repository ("repo"):** see section 0.
- **RmlUi:** a mature, existing open-source library glintfx builds on for laying out and rendering a user interface, described in files that look like simplified HTML (`.rml`) and CSS (`.rcss`).
- **Shader:** a small program that runs directly on the GPU, written in a graphics-specific language, that computes how something should look on screen (e.g. glintfx's glow/blur effects are implemented as shaders).
- **Syscall (system call):** the one and only way a normal program is allowed to ask the Linux kernel to do something on its behalf - write text to the screen, read keyboard input, allocate memory, exit the program. Floor 0 talks to the kernel through nothing else.
- **TDD (Test-Driven Development):** a way of writing code where you write a small, currently-failing automated test FIRST (describing what the code should do), then write just enough code to make it pass ("red → green"), then clean up the result ("refactor") - used throughout this project's Floor 0.
- **Terminal / shell:** see section 2.

---

## Português

### 0. Antes de começar: duas palavras de jargão que você vai ver de imediato

Você vai encontrar a maior parte do jargão no contexto abaixo (cada termo é explicado na primeira vez que aparece, e de novo no [Glossário](#glossário) ao fim). Só duas palavras logo de cara, porque você vai vê-las literalmente no primeiro parágrafo:

- **Repositório ("repo"):** uma pasta cujo histórico de mudanças é rastreado por uma ferramenta chamada **git**. Todo arquivo sobre o qual você vai ler mora dentro de um repositório - este aqui.
- **Codeberg / GitHub:** dois sites que hospedam repositórios git "na nuvem" pra que as pessoas possam baixar (`clone`) e colaborar neles. Este projeto publica nos dois:
  - Codeberg (principal): https://codeberg.org/petrinhu/glintfx
  - GitHub (espelho): https://github.com/petrinhu/glintfx

### 1. Afinal, o que É este projeto? (a analogia do prédio de dois andares)

Este único repositório na verdade contém **dois softwares bem diferentes**, chamados de "camadas" no próprio vocabulário do projeto. Pense num prédio de dois andares, construído pela mesma pessoa, por dois motivos diferentes:

```
┌───────────────────────────────────────────────────┐
│  ANDAR 1 (Camada 1) - glintfx                      │
│  Um apartamento mobiliado e habitável.              │
│  Você se muda hoje, usa a porta da frente (a API    │
│  pública dele), e não se preocupa com o encanamento │
│  atrás das paredes (OpenGL, RmlUi, GLFW).            │
│  → ESTE É O PRODUTO ATIVO E LANÇADO.                │
├───────────────────────────────────────────────────┤
│  ANDAR 0 (Camada 0) - loucura_c_asm                 │
│  Concreto cru, moldado à mão, um saco de cimento de  │
│  cada vez, sem NENHUM material de construção         │
│  pré-fabricado. Ninguém mora aqui ainda - é um       │
│  projeto de artesania do zero, feito pra aprender e, │
│  eventualmente, despejar o andar de cima em cima.    │
│  → ESTA É A TRILHA DE LONGO PRAZO, EXPERIMENTAL.     │
└───────────────────────────────────────────────────┘
```

**Andar 1 - `glintfx`** é uma **biblioteca** (um pedaço de código reutilizável que outros programas conseguem plugar - como comprar uma ferramenta elétrica em vez de forjar a sua) escrita na linguagem de programação **C++**. Ela permite a um programador construir uma interface gráfica de usuário - botões, painéis, menus - descrita em arquivos parecidos com páginas web (**HTML**/**CSS**), ganhando de graça efeitos visuais bonitos (glow, blur, sombras, degradês) calculados pela placa de vídeo do computador (a **GPU**). Ela está **lançada** (as pessoas já podem usar) e é o **produto ativo** do projeto.

**Andar 0 - `loucura_c_asm`** (o próprio nome que o projeto se dá, de forma bem-humorada e autoconsciente) é um experimento de construir um pequeno runtime, próximo de um sistema operacional, **a partir de absolutamente nada**: nenhuma biblioteca, nem mesmo a mais básica que todo programa C normalmente usa (chamada de **libc**, "biblioteca padrão de C" - ver glossário). Cada peça - como imprimir texto, como ler entrada do teclado, como alocar memória - é escrita à mão em **C** e **Assembly** (a linguagem de programação de mais baixo nível ainda legível por humanos, um degrau acima dos números binários crus que o processador executa). A única conexão dela com o mundo externo é a interface crua de **syscall** do kernel Linux (ver glossário). Esta trilha **não tem prazo**; é um projeto de artesania/aprendizado de longo prazo. A implementação dela já está **completa e aguardando uma auditoria final**, mas ainda não é um produto lançado como o glintfx - nada depende dela em produção.

**Por que os dois moram no mesmo prédio?** O Andar 0 é a ambição de longo prazo de "internalizar tudo, não confiar em nada que você mesmo não construiu". O Andar 1 é o que paga as contas hoje (ou pagaria, se isso fosse comercial): uma biblioteca real e usável, construída de forma pragmática em cima de peças de terceiros bem estabelecidas (RmlUi, GLFW, FreeType - bibliotecas open-source reais e maduras). Ao longo de anos, pedaços das fundações do Andar 1 podem ser reconstruídos em cima do Andar 0, uma peça de cada vez, onde valer o esforço. Uma peça já fez essa jornada: o "loader" de OpenGL do próprio glintfx (um pedaço pequeno de encanamento, explicado abaixo) foi reescrito do zero, substituindo um de terceiro. Ver [`docs/adr/0009-internalization-boundary.md`](adr/0009-internalization-boundary.md) pra fronteira técnica precisa (e honesta) entre os dois andares.

**Se você só quer usar uma biblioteca gráfica hoje:** vá direto pra [seção 4](#4-primeiro-contato-buildando-e-rodando-o-glintfx-andar-1).
**Se a ideia de "construir tudo do zero" é o que te empolga:** vá pra [seção 5](#5-primeiro-contato-buildando-e-testando-a-camada-0-andar-0).

### 2. Palavras que você vai precisar antes de tocar no teclado

Estão explicadas por completo, em ordem alfabética, no [Glossário](#glossário) - mas aqui vão as cinco que você vai encontrar de imediato:

| Palavra | Significado em linguagem simples |
| :--- | :--- |
| **Compilar (compile)** | Traduzir código-fonte legível por humanos (arquivos de texto que você ou outra pessoa escreveu) pra uma forma que o processador consegue de fato executar. Como traduzir uma receita escrita em português pra uma sequência de movimentos exatos de mão. |
| **Biblioteca (library)** | Código já escrito que resolve um problema uma vez, pra outros programas reusarem em vez de reescrever. `glintfx` é uma biblioteca. |
| **Linkar (linking)** | O passo, depois de compilar, que costura vários pedaços compilados (seu código + as bibliotecas que ele usa) num único programa pronto e executável. |
| **Terminal / shell** | Uma janela baseada em texto onde você digita comandos em vez de clicar em ícones. Todo comando deste guia deve ser digitado ali. No Linux, os mais comuns se chamam `bash` ou `zsh`. |
| **Comando (CLI)** | Uma linha de texto que você digita num terminal e aperta Enter pra rodar - ex.: `cmake --version`. |

### 3. Mapa do repositório

```
loucura_c_asm/            <- a raiz do repo (o pai da pasta deste arquivo)
├── glintfx/               Andar 1 - a biblioteca C++ em si
│   ├── include/glintfx/   os headers PÚBLICOS dela (os únicos arquivos que quem consome faz #include)
│   ├── src/                a implementação interna dela (arquivos .cpp)
│   ├── demos/showcase/    um programa demo executável exercitando todos os efeitos
│   ├── tests/              a suíte de testes automatizados dela (ctest)
│   └── CMakeLists.txt     a receita de build dela (ver "O que é CMake?" no Glossário)
├── src/                   Andar 0 - o código-fonte do runtime C+ASM (arquivos .c e .asm)
├── include/               Andar 0 - os headers dele (.h e .inc)
├── tests/                 Andar 0 - os programas de teste dele e arquivos de saída esperada
├── docs/                  documentação das DUAS camadas
│   ├── adr/                Architecture Decision Records - POR QUE um design foi escolhido (ver Glossário: ADR)
│   ├── getting-started.md  tutorial de dev para o glintfx (assume que você já sabe C++/CMake)
│   ├── effects.md          how-to + referência dos efeitos visuais do glintfx (sintaxe RCSS)
│   ├── embed-integration.md  referência do modo avançado "embed" do glintfx
│   ├── wiki/                arquivos-fonte da Wiki deste projeto no Codeberg/Forgejo (ver abaixo)
│   └── GUIA_INICIANTE.md   <- você está lendo este arquivo agora
├── Makefile                a receita de build da Camada 0 (ver seção 5)
├── README.md               a página de rosto do projeto (bilíngue, as duas camadas)
├── CHANGELOG.md            um histórico datado de toda versão lançada do glintfx
├── TODO.md                 a tabela viva de rastreio de tarefas do projeto ("o que está sendo feito")
└── LICENSE / NOTICE        os termos legais (MPL-2.0) e créditos de terceiros
```

### 4. Primeiro contato: buildando e rodando o glintfx (Andar 1)

Isso te leva pelos comandos exatos das próprias instruções de build deste repositório (elas são a fonte de verdade: [`CLAUDE.md`](../CLAUDE.md) e [`README.md`](../README.md)). Se algum comando abaixo parar de bater com esses arquivos, confie nos outros dois - eles são atualizados junto com a mesma checagem automatizada deste guia, mas este guia pode ficar atrasado.

**Passo 1 - instale os pacotes de sistema que o glintfx precisa.** São bibliotecas já prontas (ver Glossário: *biblioteca*) que o glintfx usa em vez de reinventar (uma biblioteca de janela/entrada chamada **GLFW**, uma biblioteca de renderização de fonte chamada **FreeType**, e a interface do driver gráfico, **OpenGL**, via **Mesa**). No Fedora Linux:

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel
```

> Numa distribuição Linux diferente, instale os equivalentes (ex.: no Debian/Ubuntu: `libglfw3-dev`, `libfreetype-dev`, `libgl1-mesa-dev`). Este projeto só suporta oficialmente Linux em processadores x86-64 (o tipo encontrado em quase todo PC desktop/notebook, ao contrário, por exemplo, do chip ARM de um Raspberry Pi).

**Passo 2 - configure e builde.** "Configurar" significa rodar o **CMake** (ver Glossário), uma ferramenta que lê o `CMakeLists.txt` e gera as instruções de build de baixo nível de fato pra sua máquina; "buildar" significa efetivamente compilar e linkar tudo.

```sh
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build
```

(`-S glintfx` = "o fonte está na pasta `glintfx/`"; `-B glintfx/build` = "coloque os arquivos de build gerados numa pasta nova `glintfx/build/`", que é descartável - apagá-la e rerodar os dois comandos acima sempre funciona.)

**Passo 3 - rode o demo embutido.** É um programinha, já escrito pra você, que abre uma janela e mostra todos os efeitos visuais do glintfx de uma vez:

```sh
./glintfx/build/demos/showcase/glintfx_showcase
```

Se uma janela abrir mostrando painéis com glow, blur e preenchimento em degradê - funcionou. Feche a janela (ou `Ctrl+C` no terminal) pra sair.

**Passo 4 (opcional) - rode a suíte de testes automatizados.** Isso compila uma segunda vez com os testes habilitados, e então roda toda checagem automatizada que o projeto vem com:

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j
ctest --test-dir glintfx/build --output-on-failure
```

O `ctest` vai imprimir uma linha por teste (`Passed`/`Failed`). A suíte cresce a cada release (eram 38 testes quando este guia foi escrito pela primeira vez, e já passava de 50 na `v0.11.0` - ver [`CHANGELOG.md`](../CHANGELOG.md) pra contagem exata e sempre atual de cada release; existe também uma configuração alternativa "embed-only", menor - não necessária pro primeiro contato).

**Pra ir mais fundo a partir daqui:** [`docs/getting-started.md`](getting-started.md) percorre *escrever seu próprio* programinha glintfx (não só rodar o demo embutido) - ele assume que você já sabe um pouco de C++ e CMake, do qual você agora já teve um primeiro gostinho.

### 4.5. O que mudou desde a v0.9.0 (três coisas que vale saber)

Este guia foi escrito pela primeira vez contra a `glintfx v0.9.0`. Três coisas visíveis pro consumidor mudaram nas três releases desde então. Explicadas aqui em termos de iniciante; cada uma linka pro documento de tipo único correspondente, com o detalhe técnico completo.

1. **A glintfx agora desenha as próprias letras por padrão (`v0.10.0`).** Qualquer programa que mostra texto (botões, rótulos, o HUD de um jogo) precisa de um pedaço de software chamado **rasterizador de fonte**: ele pega um arquivo de fonte (tipo um `.ttf`) e decide exatamente quais pontos ("pixels") da sua tela precisam acender, e em que tom, pra desenhar uma letra num tamanho dado. Até a `v0.9.0`, a glintfx sempre usava um rasterizador de terceiro, maduro e bem conhecido, chamado **FreeType**, pra isso. Desde a `v0.9.1`, a glintfx também tem **o próprio rasterizador, escrito à mão** - parte da ambição de longo prazo "construir tudo você mesmo" da [seção 1](#1-afinal-o-que-é-este-projeto-a-analogia-do-prédio-de-dois-andares) acima (a matemática de mais baixo nível dele, ler os bytes do arquivo de fonte e transformar contornos em pixels, é escrita no **Andar 0**, o runtime deste projeto feito do zero - mais uma peça que "desceu de andar"). Desde a `v0.10.0`, esse rasterizador próprio é o **padrão** que a glintfx usa. **Nada muda pra você se você nunca tocar nessa configuração** - e se o texto do seu próprio programa parecer errado depois de um upgrade, você consegue voltar pro default antigo com uma única linha de código, sem reinstalar nem rebuildar nada:
   ```cpp
   glintfx::AppConfig cfg;
   cfg.font_engine = glintfx::FontEngine::FreeType;  // volta pro default pré-v0.10.0
   ```
   Ver [ADR-0011](adr/0011-soft-font-flip.md) pra entender por que isso foi feito como um "flip suave" reversível, e não uma troca sem volta.
2. **Existe um efeito visual de "ondulação" ("ripple") de tela agora (`v0.11.0`), pra programas que embutem a glintfx dentro de um app maior (tipo um jogo).** Pense no anel de ondas que se espalha quando você joga uma pedra num lago parado - é mais ou menos esse o visual. Ele é escrito inteiramente na mesma linguagem de folha de estilo tipo-CSS (`.rcss`) que todo outro efeito da glintfx usa, como `decorator: ripple()` - nenhum código C++ é necessário pra desenhá-lo. Ele só faz alguma coisa em "modo embed" (ver o [Glossário](#glossário)); é um no-op no modo janela standalone que a [seção 4](#4-primeiro-contato-buildando-e-rodando-o-glintfx-andar-1) deste guia te mostrou. Ver [`docs/effects.md`](effects.md) pra sintaxe exata.
3. **A glintfx deu um passo pequeno rumo ao Windows, não um salto completo (`v0.9.2`).** Uma peça pequena, interna (o "loader GL" - o pedaço de encanamento que encontra as funções de desenho da placa de vídeo na máquina onde o programa roda) aprendeu a fazer esse trabalho também no Windows, e isso foi comprovado compilando-a *para* Windows *a partir de* uma máquina Linux (uma técnica chamada **cross-compiling**). É um passo real e útil, mas **não** é o mesmo que "a glintfx suporta Windows por completo hoje": a única plataforma oficialmente suportada do projeto continua sendo Linux em processadores x86-64 (ver a seção "Limitações conhecidas" do [`README.md`](../README.md#limitações-conhecidas) principal). Se você tem curiosidade sobre um build Windows completo, pergunte no issue tracker do projeto antes de assumir que tudo funciona de ponta a ponta.

### 5. Primeiro contato: buildando e testando a Camada 0 (Andar 0)

Este é o lado "do concreto cru". Nada aqui precisa de nenhum pacote de sistema - esse é o ponto inteiro: **zero bibliotecas, nem mesmo a mais básica.**

**Passo 1 - confira suas ferramentas.** Você precisa do `nasm` (o assembler - um programa que transforma texto em Assembly em código de máquina), do `clang` (o compilador C), e do `ld` da GNU (o linker). O próprio ambiente deste projeto já os tem; confira o seu com:

```sh
nasm -v
clang --version
ld --version
```

**Passo 2 - builde tudo com `make`.** `make` é uma ferramenta que lê um `Makefile` (um arquivo de receita) e roda exatamente os comandos de compilar/montar/linkar necessários, na ordem certa, sem você ter que digitar cada um à mão:

```sh
make build
```

Isso compila todo arquivo `.c` em `src/` (a runtime compartilhada - a substituta de libc escrita à mão por este projeto) e todo arquivo `.c` em `tests/` (programinhas independentes, cada um com seu próprio `main` que exercita uma peça da runtime), e então os linka, com **zero uso de qualquer biblioteca de sistema**.

**Passo 3 - rode a suíte de testes.** Todo programa de teste compilado é rodado, e seu código de saída (e em alguns casos seu texto de saída exato) é conferido contra um valor esperado gravado:

```sh
make test
```

Você vai ver uma linha `PASS`/`FAIL` por programa. Todos deveriam dizer `PASS` (no momento em que este guia foi escrito, este pipeline está com a implementação completa e passando, ainda que aguardando uma auditoria externa final antes de ser marcado como totalmente verificado no `TODO.md`).

**Passo 4 (opcional) - as outras checagens que o projeto roda sobre si mesmo:**

```sh
make check-static   # análise estática (cppcheck + clang --analyze) -- acha bugs SEM rodar o código
make test-mem        # valgrind memcheck sobre o alocador de memória -- pega corrupção grosseira de memória
make run             # roda todo programa de teste ao vivo e imprime a saída + código de saída
make clean           # apaga a pasta build/ (seguro -- basta rerodar `make build` depois)
```

**Pra ir mais fundo a partir daqui:** a seção "Camada 0" do [`CLAUDE.md`](../CLAUDE.md) explica toda flag de compilador/linker usada e *por quê* (ex.: por que `-nostdlib`, por que um `_start` escrito à mão); [`docs/adr/`](adr/README.md) registra as decisões de design de fato (estilo de syscall, tratamento de erro, estratégia do alocador de memória, layout do binário ELF) com o racional delas.

### 6. Pra onde ir depois (um mapa da documentação "de verdade")

Este guia é uma porta de entrada, não a casa inteira. Uma vez orientado, estes são os documentos de propósito único de fato (ver [O que é "Diátaxis"?](#o-que-é-diátaxis-os-quatro-tipos-de-documentação) abaixo) pra ler em seguida:

| Eu quero... | Leia isto |
| :--- | :--- |
| Escrever meu próprio programinha glintfx, passo a passo | [`docs/getting-started.md`](getting-started.md) *(tutorial)* |
| Consultar a sintaxe exata, tipo-CSS, de um efeito visual | [`docs/effects.md`](effects.md) *(how-to + referência)* |
| Embutir o glintfx dentro de uma aplicação maior (ex.: um jogo) que é dona da própria janela | [`docs/embed-integration.md`](embed-integration.md) *(referência)* |
| Instalar uma cópia pré-buildada do glintfx em vez de compilar do fonte toda vez | [`docs/packaging.md`](packaging.md) *(how-to)* |
| Resolver um erro de build/integração em que estou travado | [`docs/troubleshooting.md`](troubleshooting.md) *(how-to)* |
| Entender *por que* uma decisão de design foi tomada | [`docs/adr/README.md`](adr/README.md) *(explicação, um arquivo por decisão - "ADR")* |
| Ver o histórico exato de toda versão lançada | [`../CHANGELOG.md`](../CHANGELOG.md) *(referência)* |
| Ver o que está sendo trabalhado agora, e o que está planejado | [`../TODO.md`](../TODO.md) *(referência - é uma tabela viva de gestão de projeto, mais técnica/densa que o resto)* |
| Entender a filosofia de teste do projeto e como rodar cada tipo de teste/auditoria | [`../TESTES.md`](../TESTES.md) *(explicação + how-to)* |
| Contribuir com código eu mesmo | [`../CONTRIBUTING.md`](../CONTRIBUTING.md) |
| Reportar um problema de segurança | [`../SECURITY.md`](../SECURITY.md) |

### O que é "Diátaxis"? (os quatro tipos de documentação)

Você vai ver a palavra **Diátaxis** mencionada pela documentação deste projeto - é um framework de documentação (um conjunto de regras pra organizar docs) que este projeto segue deliberadamente. A ideia: todo pedaço de documentação está tentando fazer exatamente um de quatro trabalhos diferentes, e misturar trabalhos num único documento piora todos eles:

| Tipo | Responde... | Exemplo neste repo |
| :--- | :--- | :--- |
| **Tutorial** | "Me ensine, de mãozinha, eu nunca fiz isso" | `docs/getting-started.md` |
| **How-to** | "Eu já sei mais ou menos o que estou fazendo, me mostre como resolver ESTE problema específico" | `docs/effects.md`, `docs/troubleshooting.md` |
| **Referência** | "Eu preciso de um fato exato - um nome de parâmetro, uma assinatura de função" | `docs/embed-integration.md`, os headers públicos em `glintfx/include/glintfx/` |
| **Explicação** | "Eu quero entender POR QUE, não só COMO" | `docs/adr/*.md` |

Este guia que você está lendo é a exceção deliberada, porque um iniciante total ainda não sabe qual dos quatro tipos de documento precisa - o trabalho inteiro deste documento é te orientar o suficiente pra escolher o certo em seguida.

### Sobre a Wiki do projeto

Além dos arquivos em `docs/`, este projeto também publica uma **Wiki** leve (uma coleção separada e pequena de páginas curtas e bastante interligadas, hospedada nativamente pelo Codeberg/GitHub em vez de viver como arquivos no histórico principal deste repositório). Os arquivos-*fonte* da Wiki estão preparados em [`docs/wiki/`](wiki/) neste repositório - ver [`docs/wiki/README.md`](wiki/README.md) pra que serve a Wiki e como ela é publicada.

### Glossário

Em ordem alfabética. Todo termo usado em qualquer lugar acima é definido aqui de novo, isolado, pra você poder pular direto pra uma palavra sem reler a seção de onde ela veio.

- **ADR (Architecture Decision Record):** um documento curto registrando UMA decisão de design, por que foi tomada, quais alternativas foram consideradas e por que foram rejeitadas. Este projeto os mantém em [`docs/adr/`](adr/README.md), numerados na ordem em que foram escritos; uma decisão, uma vez `Accepted`, nunca é editada - uma mudança de ideia ganha um ADR novo que substitui o antigo.
- **API (Application Programming Interface):** o conjunto de funções/tipos que um pedaço de código expõe pra outro código chamar - a "porta da frente" de uma biblioteca, em contraste com seu encanamento interno.
- **ASan / UBSan (AddressSanitizer / UndefinedBehaviorSanitizer):** ferramentas fornecidas pelo compilador que fazem certas classes de bug de programação (escrever além do fim de um bloco de memória; usar memória depois de liberada; operações indefinidas/sem sentido) crasharem alto e imediatamente com um relatório preciso, em vez de corromper coisas em silêncio ou "funcionar por sorte". Este projeto roda sua suíte de testes sob os dois numa agenda ("nightly").
- **Assembly (ASM):** a linguagem de programação de mais baixo nível que um humano consegue realisticamente ler e escrever; cada instrução mapeia quase um-pra-um numa única operação que o próprio processador executa (mova este número pra cá, some estes dois números, pule pra esta instrução). A Camada 0 deste projeto a usa, via o assembler **NASM**, pro punhado de operações que C não consegue expressar diretamente (como a primeiríssima instrução que um programa roda, ou a instrução crua de `syscall`).
- **Biblioteca (library):** ver seção 2.
- **CI (Continuous Integration, Integração Contínua):** um robô automatizado que builda e testa o projeto toda vez que alguém propõe uma mudança (um `push` ou um Pull Request), pra erros serem pegos imediatamente em vez de dias depois. Este projeto roda CI tanto no GitHub Actions quanto no Forgejo Actions do Codeberg.
- **Clean-room (reimplementação):** reescrever um pedaço de software a partir do entendimento de *o que ele deveria fazer*, sem nunca olhar pro (ou copiar do) código-fonte do original - o oposto de copiar-e-colar. A ambição de longo prazo de "internalização" deste projeto é explicitamente clean-room: estudar como algo funciona, depois escrever uma implementação original da mesma ideia.
- **CLI:** ver "comando" na seção 2 acima.
- **CMake:** uma ferramenta que gera as instruções de build de baixo nível de fato (para compiladores/linkers) a partir de um arquivo de receita de mais alto nível (`CMakeLists.txt`), pra que a mesma receita produza um build funcionando em máquinas/toolchains diferentes. Usado pelo glintfx (Andar 1); a Camada 0 usa um `Makefile` simples em vez disso, deliberadamente, já que não tem ambição multiplataforma.
- **Compilar / Compilador:** ver seção 2. O compilador usado aqui é o **clang**.
- **Cross-compiling (compilação cruzada):** compilar um programa numa máquina de um tipo (ex.: Linux) pra ele rodar num tipo diferente (ex.: Windows), sem nunca rodar a máquina-alvo durante o build. Usado pra validar o codepath Windows do loader GL da glintfx (`v0.9.2`) sem ter uma máquina Windows.
- **Drop-in (biblioteca):** uma biblioteca desenhada pra que adicioná-la ao seu projeto seja o mais simples possível - "adicione uma linha, ganhe a feature", sem encanamento manual das dependências internas dela.
- **ELF (Executable and Linkable Format):** o formato de arquivo que o Linux usa pra programas e bibliotecas compilados - como um `.o` ou um executável final de fato parecem em bytes no disco. O [`docs/adr/0005-elf-layout.md`](adr/0005-elf-layout.md) da Camada 0 documenta as decisões exatas de layout tomadas para os binários linkados à mão dela.
- **Embed mode (modo embutido):** um jeito de usar o glintfx onde ele NÃO é dono da janela da aplicação - em vez disso, ele se anexa a uma janela/contexto gráfico que uma aplicação *host* (ex.: um jogo) já criou e é dona, desenhando a UI dele por cima. Em contraste com o modo padrão "standalone", onde o glintfx cria e é dono da própria janela. Ver [`docs/embed-integration.md`](embed-integration.md).
- **Freestanding:** um programa construído SEM nenhuma suposição de que o ambiente de runtime padrão de um sistema operacional (libc, o código de inicialização usual de `main`, etc.) está disponível - você fornece literalmente tudo você mesmo, até a primeiríssima instrução do programa. A Camada 0 é inteiramente freestanding.
- **git:** a ferramenta de controle de versão que rastreia toda mudança já feita nos arquivos deste repositório, quem a fez e por quê (via uma mensagem curta chamada "commit message"). `git clone` baixa uma cópia completa de um repositório (e seu histórico) pra sua máquina.
- **GPU (Graphics Processing Unit, unidade de processamento gráfico):** o chip especializado (separado do processador de propósito geral, o CPU) que computa gráficos - incluindo os efeitos visuais do glintfx - extremamente rápido, em paralelo, porque é a única coisa pra qual foi construído.
- **Header (arquivo `.h`/`.hpp`):** um arquivo listando O QUE um pedaço de código oferece (nomes de função, tipos) sem a implementação de fato - o que outro arquivo faz `#include` pra conseguir chamar, sem precisar ver (ou recompilar) a implementação em si.
- **Kernel (kernel Linux):** o programa central que gerencia o hardware de um computador (memória, tempo de processador, discos, a tela) e é o único software autorizado a falar diretamente com esse hardware; todo outro programa, incluindo a Camada 0 deste, pede ao kernel pra fazer coisas em seu nome via **syscalls**.
- **libc (biblioteca padrão de C):** a biblioteca da qual quase todo programa C existente depende para o básico, como imprimir texto, alocar memória, ou ler o tamanho de uma string. O motivo inteiro de existir da Camada 0 é recusar usá-la, escrevendo um equivalente do zero.
- **Linkar / Linker:** ver seção 2. O linker usado aqui é o **`ld`** da GNU.
- **Mermaid (diagrama):** um jeito de desenhar um diagrama (fluxograma, diagrama de sequência) como texto puro, que ferramentas como o renderizador do README/wiki deste repositório transformam numa imagem de fato - mantido em controle de versão igual código, em vez de um arquivo de imagem separado que pode ficar desatualizado em silêncio.
- **OpenGL:** uma interface padrão pra falar com uma GPU e desenhar gráficos; a renderização do glintfx é construída sobre OpenGL versão 3.3.
- **RAII (Resource Acquisition Is Initialization):** um padrão de C++ onde um objeto se limpa automaticamente (fecha um arquivo, libera memória, destrói uma janela) no momento em que sai de escopo, pra um programador não conseguir esquecer. Os tipos públicos `App`/`UiLayer` do glintfx seguem este padrão.
- **Rasterizador de fonte (font rasteriser):** o pedaço de software que transforma o desenho matemático das letras de um arquivo de fonte nos pontos coloridos ("pixels") de fato desenhados na tela, num tamanho dado. A glintfx só usava o rasterizador de terceiro FreeType até a `v0.9.0`; desde a `v0.10.0` ela usa por padrão o próprio, escrito à mão (ver a [seção 4.5](#45-o-que-mudou-desde-a-v090-três-coisas-que-vale-saber)).
- **Repositório ("repo"):** ver seção 0.
- **RmlUi:** uma biblioteca open-source madura e já existente sobre a qual o glintfx é construído pra montar e renderizar uma interface de usuário, descrita em arquivos parecidos com HTML simplificado (`.rml`) e CSS (`.rcss`).
- **Shader:** um programinha que roda direto na GPU, escrito numa linguagem específica de gráficos, que computa como algo deve aparecer na tela (ex.: os efeitos de glow/blur do glintfx são implementados como shaders).
- **Syscall (chamada de sistema):** o único jeito que um programa comum tem permissão de pedir ao kernel Linux pra fazer algo em seu nome - escrever texto na tela, ler entrada de teclado, alocar memória, encerrar o programa. A Camada 0 fala com o kernel por nada mais além disso.
- **TDD (Test-Driven Development, desenvolvimento guiado por teste):** um jeito de escrever código onde você escreve um teste automatizado pequeno e atualmente-falhando PRIMEIRO (descrevendo o que o código deveria fazer), depois escreve só o suficiente de código pra passar ("vermelho → verde"), depois limpa o resultado ("refatorar") - usado ao longo da Camada 0 deste projeto.
- **Terminal / shell:** ver seção 2.
