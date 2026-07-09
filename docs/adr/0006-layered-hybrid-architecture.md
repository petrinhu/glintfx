# ADR-0006 — Layered hybrid architecture / Arquitetura híbrida em camadas

- **Status:** Accepted (2026-06-28)
- **Deciders:** petrus (líder), software-architect
- **Tags:** scope, architecture, one-way-door
- **Supersedes scope of:** the original "zero libs" framing is now scoped to Layer 0 only.

## Context / Contexto

**EN:** The project goal expanded: build a **C++23-compatible library** (compatible down to C++17, RmlUi's floor) that **merges RmlUi** (HTML/CSS UI + layout) with the **GL3 renderer** and exposes all of its visual effects (glow/drop-shadow, gradient, blur, mask, backdrop-filter — confirmed in `RmlUi_Renderer_GL3.cpp`, 2171 lines of GLSL-backed effects). These require `libstdc++`, `libGL`, a window/context system (GLFW/SDL/X11) and `libc` — **fundamentally incompatible** with the zero-libs, freestanding C/ASM core defined in ADR 0001–0005.

**PT:** O objetivo do projeto se expandiu: construir uma **biblioteca compatível com C++23** (compatível até C++17, piso do RmlUi) que **una o RmlUi** (UI HTML/CSS + layout) ao **renderer GL3** e exponha todos os seus efeitos visuais (glow/drop-shadow, degradê, blur, mask, backdrop-filter — confirmados em `RmlUi_Renderer_GL3.cpp`, 2171 linhas de efeitos via GLSL). Eles exigem `libstdc++`, `libGL`, sistema de janela/contexto (GLFW/SDL/X11) e `libc` — **fundamentalmente incompatíveis** com o núcleo C/ASM freestanding zero-libs dos ADR 0001–0005.

## Decision / Decisão

**EN:** Adopt a **two-layer architecture** instead of reverting/destroying the existing core:

- **Layer 0 — sovereign core (unchanged):** the current C + ASM freestanding runtime, zero libc, direct syscalls, static no-PIE. ADR 0001–0005 remain fully valid here. It is the long-term *internalization target*.
- **Layer 1 — C++23 library (the deliverable):** starts by **linking the real RmlUi + gl3w + libGL + a window/context backend**, exposing a **unified UI+GL3 API** with all available effects. Compatible C++17→C++23. Build via **CMake** (integrates RmlUi); the core keeps its hand-written Makefile. Window backends: **GLFW (primary), then SDL and X11** (goal: all). Hand-written **ASM only in hot paths**.
- **Trajectory (the "loucura"):** over time, **internalize** pieces of Layer 1 with clean-room reimplementations built on Layer 0, where it is worth it.

**PT:** Adotar uma **arquitetura em duas camadas** em vez de reverter/destruir o núcleo existente:

- **Camada 0 — núcleo soberano (intacto):** o runtime C + ASM freestanding atual, zero libc, syscalls diretas, estático no-PIE. Os ADR 0001–0005 seguem válidos aqui. É o *alvo de internalização* de longo prazo.
- **Camada 1 — biblioteca C++23 (a entrega):** começa **linkando o RmlUi + gl3w + libGL + um backend de janela/contexto reais**, expondo uma **API unificada UI+GL3** com todos os efeitos. Compatível C++17→C++23. Build via **CMake** (integra o RmlUi); o núcleo mantém seu Makefile artesanal. Backends de janela: **GLFW (primário), depois SDL e X11** (meta: todos). **ASM à mão só em hot paths**.
- **Trajetória (a "loucura"):** com o tempo, **internalizar** peças da Camada 1 com reimplementações clean-room sobre a Camada 0, onde valer a pena.

## Consequences / Consequências

**EN:** The project as a whole is **no longer "zero libs"** — only Layer 0 is. Two build systems coexist (Makefile + CMake). **Licensing:** RmlUi is MIT, gl3w is public domain; both live in `examples/` (gitignored) and any internalization must be **clean-room** to keep our MPL-2.0 base uncontaminated. The original purity is preserved as a direction (Layer 1 → Layer 0), not a present-day constraint on the whole repo.

**PT:** O projeto como um todo **deixa de ser "zero libs"** — só a Camada 0 é. Dois build systems coexistem (Makefile + CMake). **Licenças:** RmlUi é MIT, gl3w é domínio público; ambos vivem em `examples/` (gitignored) e qualquer internalização deve ser **clean-room** para não contaminar nossa base MPL-2.0. A pureza original é preservada como direção (Camada 1 → Camada 0), não como restrição presente no repo inteiro.

## Options considered / Opções consideradas

- **Layered hybrid (chosen / escolhida)** — avança já com efeitos reais e mantém a loucura como trajetória.
- Full pivot to a normal C++23 lib / Pivô total — entregável, mas mata o núcleo. Rejected.
- Pure clean-room from scratch / Clean-room puro — fiel, mas anos. Rejected.
- Two separate projects / Dois projetos — preserva o puro, mas o líder quis repurposar este. Rejected.

Cross-ref: [[0001-syscall-layer-style]], [[0005-elf-layout]].

## Addendum (2026-07-09)

**EN:** [[0009-internalization-boundary]] **supersedes in part** the "no linking between layers" framing above. As internalized Layer-0 pieces land (starting with the gl3w pilot), Layer 1 statically links a `libcore.a` archive (symbols prefixed `glx_*`) exposing them — via the "hosted shell + C meat" pattern: the C++ class implementing an RmlUi interface stays ordinary hosted C++, only its algorithmic core is sovereign C from Layer 0. This ADR's core two-layer split (Layer 0 remains freestanding, zero-libc, its own build system; Layer 1 remains the C++23 deliverable) is **unchanged** — only the *mechanism* by which a finished Layer-0 artifact reaches a Layer-1 binary moves from "process boundary" to "static link boundary". See ADR-0009 for the gates, the heap-ownership rule, and why Option B (IPC) was rejected for the GL/font hot path.

**PT:** O [[0009-internalization-boundary]] **substitui em parte** o enquadramento "sem link entre camadas" acima. À medida que peças internalizadas da Camada 0 são entregues (a começar pelo piloto gl3w), a Camada 1 linka estaticamente um archive `libcore.a` (símbolos prefixados `glx_*`) que as expõe — via o padrão "casca-hosted + carne-C": a classe C++ que implementa uma interface do RmlUi permanece C++ hosted comum, só seu núcleo algorítmico é C soberano da Camada 0. A separação em duas camadas deste ADR (Camada 0 permanece freestanding, zero-libc, com build system próprio; Camada 1 permanece a entrega C++23) **não muda** — só o *mecanismo* pelo qual um artefato pronto da Camada 0 chega a um binário da Camada 1 passa de "fronteira de processo" para "fronteira de link estático". Ver ADR-0009 pros gates, a regra de ownership de heap, e por que a Opção B (IPC) foi rejeitada pro hot path de GL/fonte.
