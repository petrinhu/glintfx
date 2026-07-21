# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 0.14.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)   # piso C++17; alvo 23
set(CMAKE_CXX_EXTENSIONS OFF)

# EN: GLINTFX_SANITIZE (TST-L1-MEM, TODO.md LW-TST) — AddressSanitizer + LeakSanitizer +
#     UndefinedBehaviorSanitizer for a nightly memory-safety gate. OFF by default: never
#     pollutes the normal build (dev, PR-hot CI) — the instrumented build is significantly
#     slower and is meant to run as a separate/nightly job (see .github/workflows/nightly.yml
#     and .forgejo/workflows/nightly.yml).
#     Applied via add_compile_options/add_link_options (directory-scope, set BEFORE
#     FetchContent_MakeAvailable(RmlUi) and the INTERFACE target glloader declared below)
#     so the flags reach every target compiled in this build: glintfx itself, vendored
#     gl_loader/stb_image sources, the fetched RmlUi sources, and the whole tests/ suite.
#     Prebuilt system libraries
#     linked in (GLFW, Mesa/libGL, FreeType, libstdc++) are NOT recompiled here and therefore
#     stay uninstrumented — any noise ASan/LSan/UBSan raise against them (false leaks reported
#     against driver-internal allocations, etc.) is filtered at runtime via
#     tests/lsan_suppressions.txt (LSAN_OPTIONS), not by trying to instrument those libraries.
#     -fno-sanitize-recover=all: UBSan aborts immediately on the first UB hit instead of
#     printing and continuing — this is meant to be a hard gate, not a report-only run.
#     -fno-omit-frame-pointer + -g: readable stack traces (unwind quality matters more than
#     binary size in this build).
# PT: GLINTFX_SANITIZE (TST-L1-MEM, TODO.md LW-TST) — AddressSanitizer + LeakSanitizer +
#     UndefinedBehaviorSanitizer para um gate noturno de memory-safety. OFF por padrão: nunca
#     polui o build normal (dev, CI quente de PR) — o build instrumentado é significativamente
#     mais lento e é feito para rodar como job separado/noturno (ver .github/workflows/nightly.yml
#     e .forgejo/workflows/nightly.yml).
#     Aplicado via add_compile_options/add_link_options (escopo de diretório, definido ANTES
#     de FetchContent_MakeAvailable(RmlUi) e da declaração do target INTERFACE glloader
#     abaixo) para que as flags alcancem todo target compilado neste build: a própria
#     glintfx, os fontes vendorizados gl_loader/stb_image, os fontes fetchados do RmlUi,
#     e toda a suíte tests/.
#     Bibliotecas de sistema pré-compiladas linkadas (GLFW, Mesa/libGL, FreeType, libstdc++)
#     NÃO são recompiladas aqui e portanto ficam não-instrumentadas — qualquer ruído que
#     ASan/LSan/UBSan levantem contra elas (falsos leaks reportados contra alocações internas
#     de driver, etc.) é filtrado em runtime via tests/lsan_suppressions.txt (LSAN_OPTIONS), e
#     não por tentar instrumentar essas bibliotecas.
#     -fno-sanitize-recover=all: UBSan aborta imediatamente no primeiro UB em vez de imprimir
#     e continuar — a intenção é um gate duro, não uma execução apenas-relatório.
#     -fno-omit-frame-pointer + -g: stacks legíveis (qualidade do unwind importa mais que
#     tamanho do binário neste build).
option(GLINTFX_SANITIZE
  "Build glintfx + tests with ASan/LSan/UBSan (nightly memory-safety gate, TST-L1-MEM)"
  OFF)
if(GLINTFX_SANITIZE)
  set(_glintfx_sanitize_flags
    -fsanitize=address,undefined
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
    -g)
  add_compile_options(${_glintfx_sanitize_flags})
  add_link_options(${_glintfx_sanitize_flags})
endif()

# EN: GLINTFX_COVERAGE (L1.18-COVCI, TODO.md LW10) -- Clang source-based code coverage
#     (llvm-cov), self-hosted per the líder's 2026-07-04 decision (NOT Codecov or any
#     third-party service -- zero data leaves the repo/CI runner). OFF by default: an
#     instrumented build never pollutes the normal/dev/PR-hot build; it is meant to run
#     as its own CI job (see the `coverage` job in .github/workflows/ci.yml and
#     .forgejo/workflows/ci.yml) or via tools/coverage_report.sh locally.
#
#     Clang-only, hard gate: this project's CI has built glintfx with clang since v0.5.0
#     (README.md declares "Compiler: clang"), and Clang's source-based coverage
#     (-fprofile-instr-generate -fcoverage-mapping + llvm-profdata/llvm-cov) is a
#     different, incompatible instrumentation model from GCC's --coverage/gcov. No
#     gcov/lcov fallback path is implemented -- GLINTFX_COVERAGE=ON with a non-Clang
#     compiler is a hard configure-time error, not a silent no-op or a degraded mode.
#
#     Deliberately NOT applied via directory-wide add_compile_options/add_link_options
#     (unlike GLINTFX_SANITIZE above, which intentionally instruments RmlUi too --
#     memory-safety bugs in linked third-party code are still our problem at runtime).
#     Coverage is a *report of our own authored code*, so instrumentation is applied
#     per-target via the glintfx_instrument_coverage() function below, called explicitly
#     on the glintfx library target (right after add_library() further down this file)
#     and, from a single loop, on every test/demo-helper target defined in tests/
#     (tests/CMakeLists.txt, end of file). This keeps ~200 RmlUi (FetchContent) and
#     glloader (INTERFACE) translation units uninstrumented -- instrumenting RmlUi would
#     balloon .profraw size and build time for a signal that is out of scope (RmlUi is
#     third-party; tools/coverage_report.sh's -ignore-filename-regex mirrors this same
#     scope boundary on the reporting side).
#
#     Consequence for demos: an instrumented glintfx static library's object files
#     reference the Clang profiling runtime's symbols (__llvm_profile_*). Any executable
#     that links glintfx WITHOUT itself passing -fprofile-instr-generate at link time
#     (e.g. the showcase demo, if GLINTFX_BUILD_DEMOS=ON) will fail to link with
#     undefined-reference errors. The coverage CI job and tools/coverage_report.sh both
#     configure with -DGLINTFX_BUILD_DEMOS=OFF for this reason -- if a future job needs
#     both, either instrument the demo target too (call glintfx_instrument_coverage on
#     it) or keep demos OFF in that build.
# PT: GLINTFX_COVERAGE (L1.18-COVCI, TODO.md LW10) -- cobertura de código source-based
#     do Clang (llvm-cov), self-hosted conforme decisão do líder de 2026-07-04 (NÃO
#     Codecov nem nenhum serviço de terceiro -- zero dado sai do repo/runner de CI). OFF
#     por padrão: um build instrumentado nunca polui o build normal/dev/CI quente de PR;
#     é feito para rodar como job próprio de CI (ver o job `coverage` em
#     .github/workflows/ci.yml e .forgejo/workflows/ci.yml) ou via
#     tools/coverage_report.sh localmente.
#
#     Só Clang, gate duro: o CI deste projeto builda a glintfx com clang desde a v0.5.0
#     (README.md declara "Compiler: clang"), e a cobertura source-based do Clang
#     (-fprofile-instr-generate -fcoverage-mapping + llvm-profdata/llvm-cov) é um modelo
#     de instrumentação diferente e incompatível do --coverage/gcov do GCC. Nenhum
#     caminho de fallback gcov/lcov é implementado -- GLINTFX_COVERAGE=ON com compilador
#     não-Clang é erro duro em tempo de configure, não um no-op silencioso nem um modo
#     degradado.
#
#     Deliberadamente NÃO aplicado via add_compile_options/add_link_options de escopo de
#     diretório (diferente do GLINTFX_SANITIZE acima, que instrumenta o RmlUi de
#     propósito -- bug de memory-safety em código de terceiro linkado ainda é problema
#     nosso em runtime). Cobertura é um *relatório do nosso próprio código autoral*,
#     então a instrumentação é aplicada por-alvo via a função glintfx_instrument_coverage()
#     abaixo, chamada explicitamente no alvo da biblioteca glintfx (logo após
#     add_library() mais adiante neste arquivo) e, a partir de um único loop, em todo
#     alvo de teste/helper de demo definido em tests/ (tests/CMakeLists.txt, fim do
#     arquivo). Isso mantém as ~200 unidades de tradução do RmlUi (FetchContent) e do
#     glloader (INTERFACE) não-instrumentadas -- instrumentar o RmlUi infuncionaria o
#     tamanho do .profraw e o tempo de build por um sinal fora de escopo (RmlUi é
#     terceiro; o -ignore-filename-regex do tools/coverage_report.sh espelha essa mesma
#     fronteira de escopo do lado do relatório).
#
#     Consequência para os demos: os arquivos-objeto de uma biblioteca glintfx
#     instrumentada referenciam símbolos do runtime de profiling do Clang
#     (__llvm_profile_*). Qualquer executável que linke glintfx SEM ele mesmo passar
#     -fprofile-instr-generate no link (ex.: o demo showcase, se GLINTFX_BUILD_DEMOS=ON)
#     falhará o link com erros de referência indefinida. Tanto o job de CI de cobertura
#     quanto o tools/coverage_report.sh configuram com -DGLINTFX_BUILD_DEMOS=OFF por
#     este motivo -- se um job futuro precisar dos dois, ou instrumenta o alvo do demo
#     também (chamando glintfx_instrument_coverage nele) ou mantém demos OFF nesse build.
option(GLINTFX_COVERAGE
  "Instrument glintfx + its test suite with Clang source-based code coverage (llvm-cov, L1.18-COVCI)"
  OFF)
if(GLINTFX_COVERAGE)
  if(NOT (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    message(FATAL_ERROR
      "glintfx: GLINTFX_COVERAGE=ON requires Clang (pass -DCMAKE_C_COMPILER=clang "
      "-DCMAKE_CXX_COMPILER=clang++) -- the coverage toolchain is Clang source-based "
      "coverage (llvm-cov/llvm-profdata). There is no gcov/lcov fallback; "
      "CMAKE_C_COMPILER_ID=${CMAKE_C_COMPILER_ID} CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}.")
  endif()
endif()

# EN: glintfx_instrument_coverage(<target>) -- applies Clang source-based coverage flags
#     (compile: -fprofile-instr-generate -fcoverage-mapping; link: -fprofile-instr-generate)
#     to a single target, PRIVATE scope so the flags never leak to consumers via usage
#     requirements (an external FetchContent/find_package consumer must never be forced
#     to link the profiling runtime just because glintfx's own CI built a coverage
#     variant). No-op when GLINTFX_COVERAGE=OFF -- safe to call unconditionally.
# PT: glintfx_instrument_coverage(<alvo>) -- aplica as flags de cobertura source-based do
#     Clang (compile: -fprofile-instr-generate -fcoverage-mapping; link:
#     -fprofile-instr-generate) a um único alvo, escopo PRIVATE para que as flags nunca
#     vazem a consumidores via usage requirements (um consumidor externo via
#     FetchContent/find_package nunca deve ser forçado a linkar o runtime de profiling só
#     porque o CI da própria glintfx buildou uma variante de cobertura). No-op com
#     GLINTFX_COVERAGE=OFF -- seguro chamar incondicionalmente.
function(glintfx_instrument_coverage target)
  if(GLINTFX_COVERAGE)
    target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(${target} PRIVATE -fprofile-instr-generate)
  endif()
endfunction()

include(FetchContent)

# EN: GLFW window backend option.
#     ON  (default) — compile the standalone glintfx::App facade, window_glfw.cpp and
#         the RmlUi GLFW platform adapter. find_package(glfw3) is called and glfw is
#         linked into the library.
#     OFF — embed-only build: only Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock
#         are compiled. find_package(glfw3) is NOT called; glfw is NOT linked into the
#         library. Consumers (e.g. SDL3 hosts) must not drag GLFW as a transitive dep.
# PT: Opção de backend de janela GLFW.
#     ON  (padrão) — compila a fachada standalone glintfx::App, window_glfw.cpp e o
#         adapter de plataforma GLFW do RmlUi. find_package(glfw3) é chamado e glfw é
#         linkado na biblioteca.
#     OFF — build embed-only: só Engine + UiLayer + RenderGl3 + Bootstrap + SystemClock
#         são compilados. find_package(glfw3) NÃO é chamado; glfw NÃO é linkado na lib.
#         Consumidores (ex.: hosts SDL3) não puxam GLFW como dep transitiva.
option(GLINTFX_BACKEND_GLFW
  "GLFW window backend + standalone glintfx::App (OFF = embed-only, no GLFW in lib)"
  ON)

# EN: GLINTFX_OWN_FONT_ENGINE (L1.19-FONTENG, FT-F3, ADR-0009 "hosted shell + C meat";
#     L1.20-FONTFLIP Phase 1 "soft flip" -- TODO.md) — the A/B gate between RmlUi's own
#     FreeType-backed font engine and glintfx's own SOV-SFNT/SOV-RAST/SOV-HINT font engine
#     (src/font_engine_own.{hpp,cpp}). Default is now **ON**: the own engine is compiled by
#     default and selectable at runtime. This is a "soft flip", not a drop -- FreeType stays
#     linked (RMLUI_FONT_ENGINE stays "freetype" below, see next paragraph) and the RmlUi
#     default remains fully available; nothing about FreeType support is removed.
#
#     RUNTIME switch, not a RmlUi-rebuild switch: RMLUI_FONT_ENGINE stays "freetype" below
#     regardless of this option (do NOT change that to "none") -- Rml::SetFontEngineInterface()
#     overrides RmlUi's registered engine at runtime, in Bootstrap::init() (src/bootstrap.cpp),
#     guarded by `#if GLINTFX_OWN_FONT_ENGINE` (glintfx/include/glintfx/config.hpp.in). This is
#     what lets A/B testing happen without rebuilding the ~200-translation-unit RmlUi
#     FetchContent dependency for each side of the comparison.
#
#     ADR-0009 link pattern, now VENDORED (L1.20-FONTFLIP Phase 1): this option compiles
#     src/font_engine_own.cpp (the hosted C++ "shell") PLUS Layer 0's sovereign C "meat" --
#     sfnt.c/raster.c/hint.c -- `extern "C"`-linked in directly as glintfx sources (still no
#     separate libcore.a archive; see font_engine_own.hpp's own file header for the full
#     ADR-0009 rationale). Unlike the pre-Phase-1 layout, these 3 `.c` files (plus the 3
#     `core/*.h` headers they need) are read from glintfx/vendor/core/ -- a byte-identical
#     vendored copy of Layer 0's src/{sfnt,raster,hint}.c and include/core/{sfnt,raster,hint}.h
#     (see glintfx/vendor/core/README.md) -- NOT from ../src / ../include/core. This makes the
#     own-engine default self-contained and portable: it compiles whether glintfx is built
#     inside the loucura_c_asm monorepo OR as a standalone glintfx/ subtree fetched/copied in
#     isolation by an external consumer (e.g. GusWorld) -- no monorepo-sibling requirement, no
#     configure-time FATAL_ERROR path.
# PT: GLINTFX_OWN_FONT_ENGINE (L1.19-FONTENG, FT-F3, ADR-0009 "casca-hosted + carne-C";
#     L1.20-FONTFLIP Fase 1 "flip suave" -- TODO.md) — o gate A/B entre o motor de fonte
#     próprio do RmlUi, apoiado em FreeType, e o motor de fonte próprio do glintfx,
#     SOV-SFNT/SOV-RAST/SOV-HINT (src/font_engine_own.{hpp,cpp}). O padrão agora é **ON**: o
#     motor próprio é compilado por padrão e selecionável em runtime. Isto é um "flip suave",
#     não um descarte -- o FreeType continua linkado (RMLUI_FONT_ENGINE continua "freetype"
#     abaixo, ver próximo parágrafo) e o default do RmlUi permanece totalmente disponível;
#     nada do suporte a FreeType é removido.
#
#     Chave de RUNTIME, não uma chave de rebuild do RmlUi: RMLUI_FONT_ENGINE continua
#     "freetype" abaixo independente desta option (NÃO trocar para "none") --
#     Rml::SetFontEngineInterface() sobrepõe o motor registrado do RmlUi em runtime, no
#     Bootstrap::init() (src/bootstrap.cpp), protegido por `#if GLINTFX_OWN_FONT_ENGINE`
#     (glintfx/include/glintfx/config.hpp.in). É isso que permite o teste A/B acontecer sem
#     rebuildar a dependência FetchContent do RmlUi (~200 unidades de tradução) pra cada lado
#     da comparação.
#
#     Padrão de link da ADR-0009, agora VENDORIZADO (L1.20-FONTFLIP Fase 1): esta option
#     compila src/font_engine_own.cpp (a "casca" C++ hosted) MAIS a "carne" C soberana da
#     Camada 0 -- sfnt.c/raster.c/hint.c -- linkada via `extern "C"` diretamente como sources
#     do glintfx (ainda sem archive libcore.a separado; ver o próprio cabeçalho de arquivo de
#     font_engine_own.hpp pra racional completa da ADR-0009). Diferente do layout pré-Fase-1,
#     estes 3 arquivos `.c` (mais os 3 headers `core/*.h` de que precisam) são lidos de
#     glintfx/vendor/core/ -- uma cópia vendorizada byte-idêntica de src/{sfnt,raster,hint}.c
#     e include/core/{sfnt,raster,hint}.h da Camada 0 (ver glintfx/vendor/core/README.md) --
#     NÃO de ../src / ../include/core. Isso torna o padrão do motor próprio auto-contido e
#     portável: compila tanto com o glintfx buildado dentro do monorepo loucura_c_asm QUANTO
#     como uma subárvore glintfx/ standalone fetchada/copiada isoladamente por um consumidor
#     externo (ex.: GusWorld) -- sem exigência de irmão-de-monorepo, sem caminho de
#     FATAL_ERROR em tempo de configure.
option(GLINTFX_OWN_FONT_ENGINE
  "Compile glintfx's own SOV-SFNT/SOV-RAST/SOV-HINT font engine (vendored under glintfx/vendor/core/), selectable at runtime alongside RmlUi's FreeType default via Rml::SetFontEngineInterface(), no RmlUi rebuild (L1.19-FONTENG, L1.20-FONTFLIP)"
  ON)

# ===========================================================================
# EN: ADR-0015 (framework-2D atomized architecture) -- phase A0, the
#     GLINTFX_MODULE_* flag family. A0's mandate is BUILD-SYSTEM ONLY: zero new
#     public API, zero change to any .cpp/.hpp LOGIC -- see the ADR and
#     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md. Every flag
#     below either (1) is a genuine alias/rename of a flag that ALREADY
#     compiles-out code and link deps today (window), (2) is a genuinely new
#     DAG dependency gate over sources that are already cleanly separable via
#     pure CMake list manipulation (app), or (3) is a RESERVED name for a
#     module whose real OFF path needs a source-level change this phase
#     forbids (image, text) -- declared with a FATAL_ERROR on an attempted
#     OFF instead of a silent no-op, so the gap is loud, not hidden. ui and fx
#     get NEITHER kind of flag in A0 (see the note further below, next to
#     where their sources are compiled) -- introducing a flag with nothing
#     real to gate would be exactly the "pretend the seam exists before it is
#     built" anti-pattern ADR-0015 (c) forbids for the ui/fx split, and by the
#     same principle it is not done here for ui/fx as a whole either.
# PT: ADR-0015 (arquitetura atomizada do framework 2D) -- fase A0, a família
#     de flags GLINTFX_MODULE_*. O mandato do A0 é SÓ BUILD-SYSTEM: zero API
#     pública nova, zero mudança de LÓGICA em qualquer .cpp/.hpp -- ver a ADR e
#     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md. Cada flag
#     abaixo ou (1) é um alias/rename genuíno de uma flag que JÁ tira código e
#     dep de link do build hoje (window), (2) é um gate de dependência de DAG
#     genuinamente novo sobre sources que já são limpamente separáveis via
#     manipulação pura de lista do CMake (app), ou (3) é um nome RESERVADO
#     para um módulo cujo caminho OFF real precisa de mudança de nível de
#     source que esta fase proíbe (image, text) -- declarado com um
#     FATAL_ERROR numa tentativa de OFF em vez de um no-op silencioso, para
#     que a lacuna seja alta, não escondida. ui e fx NÃO ganham flag de
#     nenhum dos dois tipos no A0 (ver a nota mais abaixo, perto de onde os
#     sources deles são compilados) -- introduzir uma flag sem nada real pra
#     gatear seria exatamente o anti-padrão "fingir que a costura existe antes
#     de ela ser construída" que a ADR-0015 (c) proíbe para o split ui/fx, e
#     pelo mesmo princípio isso também não é feito aqui para ui/fx como um todo.
# ===========================================================================

# --- GLINTFX_MODULE_WINDOW (alias of GLINTFX_BACKEND_GLFW, ADR-0015 (d)(4)) ---
# EN: The legacy flag (GLINTFX_BACKEND_GLFW, declared above) keeps working
#     unchanged for every existing consumer/CI script -- nothing about it is
#     touched. GLINTFX_MODULE_WINDOW is the new canonical ADR-0015 name for the
#     SAME on/off switch (window + GL context creation, poll, swap,
#     should_close, frame step -- ADR-0015 (b)). Whichever of the two the
#     caller sets explicitly on the configure command line wins and is
#     mirrored into the other. `DEFINED <name>` is true from the very first
#     line of this file for any `-D<name>=...` passed at the `cmake`
#     invocation (CMake pre-populates the cache before running any script
#     code), so this check reliably distinguishes "the user set it explicitly"
#     from "still at its option() default" even though both flags default ON.
# PT: A flag legada (GLINTFX_BACKEND_GLFW, declarada acima) continua
#     funcionando sem mudança para todo consumidor/script de CI existente --
#     nada nela é tocado. GLINTFX_MODULE_WINDOW é o novo nome canônico da
#     ADR-0015 para o MESMO interruptor (janela + criação de contexto GL,
#     poll, swap, should_close, passo de frame -- ADR-0015 (b)). Qualquer um
#     dos dois que o chamador definir explicitamente na linha de comando do
#     configure vence e é espelhado no outro. `DEFINED <nome>` é verdadeiro
#     desde a primeiríssima linha deste arquivo para qualquer `-D<nome>=...`
#     passado na invocação do `cmake` (o CMake pré-popula o cache antes de
#     rodar qualquer código de script), então esta checagem distingue de
#     forma confiável "o usuário definiu explicitamente" de "ainda no default
#     do option()" mesmo as duas flags tendo default ON.
if(DEFINED GLINTFX_MODULE_WINDOW)
  set(GLINTFX_BACKEND_GLFW ${GLINTFX_MODULE_WINDOW} CACHE BOOL
    "GLFW window backend + standalone glintfx::App (OFF = embed-only, no GLFW in lib) -- alias target of GLINTFX_MODULE_WINDOW"
    FORCE)
endif()
set(GLINTFX_MODULE_WINDOW ${GLINTFX_BACKEND_GLFW} CACHE BOOL
  "Window module: window + GL context creation, poll, swap, should_close, frame step (ADR-0015 (b); alias of GLINTFX_BACKEND_GLFW, ADR-0015 (d)(4))"
  FORCE)

# --- GLINTFX_MODULE_APP (new, ADR-0015 (b): the a-la-carte aggregator) ---
# EN: app.cpp (the App facade) plus the RmlUi GLFW SystemInterface glue it
#     itself #includes (${_rml_backends}/RmlUi_Platform_GLFW.cpp -- see
#     app.cpp's own file header) are the "app" atom, distinct from "window"
#     (WindowGlfw in window_glfw.cpp: the bare window+GL-context primitive App
#     composes, see the glintfx_window OBJECT library further below). Defaults
#     to GLINTFX_MODULE_WINDOW's value -- today App cannot exist without a
#     window (DAG dependency, ADR-0015 (b)/(d)(1)) -- and a FATAL_ERROR
#     enforces that dependency instead of silently producing a build that
#     configures fine and fails at link time if a caller asks for app=ON with
#     window=OFF.
# PT: app.cpp (a fachada App) mais a cola SystemInterface GLFW do RmlUi que
#     ele próprio inclui (${_rml_backends}/RmlUi_Platform_GLFW.cpp -- ver o
#     próprio cabeçalho de arquivo do app.cpp) são o átomo "app", distinto de
#     "window" (WindowGlfw em window_glfw.cpp: o primitivo nu de
#     janela+contexto-GL que a App compõe, ver a biblioteca OBJECT
#     glintfx_window mais abaixo). Tem default igual ao valor de
#     GLINTFX_MODULE_WINDOW -- hoje a App não existe sem uma janela
#     (dependência de DAG, ADR-0015 (b)/(d)(1)) -- e um FATAL_ERROR reforça
#     essa dependência em vez de produzir silenciosamente um build que
#     configura bem e falha no link se alguém pedir app=ON com window=OFF.
option(GLINTFX_MODULE_APP
  "App module: the App facade composing window(+input/audio, future)+ui into \"the game inside glintfx\" (ADR-0015 (b); requires GLINTFX_MODULE_WINDOW)"
  ${GLINTFX_MODULE_WINDOW})
if(GLINTFX_MODULE_APP AND NOT GLINTFX_MODULE_WINDOW)
  message(FATAL_ERROR
    "glintfx: GLINTFX_MODULE_APP=ON requires GLINTFX_MODULE_WINDOW=ON (ADR-0015 dependency "
    "rule (d)(1): app depends on window) -- the App facade cannot exist without the window "
    "it composes. Pass -DGLINTFX_MODULE_WINDOW=ON or -DGLINTFX_MODULE_APP=OFF.")
endif()

# --- GLINTFX_MODULE_IMAGE (reserved, ADR-0015 (b): PNG/JPG/TGA decode) ---
# EN: stb_image_impl.cpp becomes its own OBJECT-library target (glintfx_image,
#     further below) in this very phase -- real groundwork, not a stub -- but
#     the flag stays FORCED ON: render_gl3.cpp's GenerateTexture calls
#     stbi_load_from_memory/stbi_image_free UNCONDITIONALLY, so compiling
#     glintfx_image out while still compiling render_gl3.cpp (part of ui,
#     always on) would fail the LINK, not gracefully degrade. Making
#     GLINTFX_MODULE_IMAGE=OFF actually work needs a compile-time guard INSIDE
#     render_gl3.cpp's texture loader (falling back to RmlUi's own TGA-only
#     loader, which the existing code already reaches whenever stb_image
#     itself fails to decode) -- a source-level change, deferred past A0's
#     build-system-only mandate to a future phase.
# PT: stb_image_impl.cpp vira seu próprio alvo de biblioteca OBJECT
#     (glintfx_image, mais abaixo) já nesta fase -- trabalho real de base, não
#     um stub -- mas a flag fica FORÇADA ON: o GenerateTexture do
#     render_gl3.cpp chama stbi_load_from_memory/stbi_image_free
#     INCONDICIONALMENTE, então compilar glintfx_image fora enquanto ainda se
#     compila render_gl3.cpp (parte da ui, sempre ligada) falharia o LINK, não
#     degradaria graciosamente. Fazer GLINTFX_MODULE_IMAGE=OFF funcionar de
#     verdade precisa de uma guarda em tempo de compilação DENTRO do
#     carregador de textura do render_gl3.cpp (caindo para o loader TGA-only
#     do próprio RmlUi, que o código existente já alcança sempre que o
#     próprio stb_image falha o decode) -- mudança de nível de source, adiada
#     para além do mandato só-build-system do A0, pra fase futura.
option(GLINTFX_MODULE_IMAGE
  "Image module: PNG/JPG/TGA decode via vendored stb_image (ADR-0015 (b); forced ON in A0, see comment above -- OFF needs a render_gl3.cpp texture-loader guard, deferred)"
  ON)
if(NOT GLINTFX_MODULE_IMAGE)
  message(FATAL_ERROR
    "glintfx: GLINTFX_MODULE_IMAGE=OFF is not yet supported -- render_gl3.cpp's texture "
    "loader calls stb_image unconditionally; wiring a real OFF path needs a source-level "
    "guard there, deferred past ADR-0015 phase A0 (build-system skeleton only, zero "
    "source-logic changes). Tracked in docs/adr/0015-framework-2d-atomized-architecture.md.")
endif()

# --- GLINTFX_MODULE_TEXT (reserved, ADR-0015 (b): glyph raster + metrics) ---
# EN: GLINTFX_MODULE_TEXT is the ENCLOSING flag for the whole text subsystem
#     per ADR-0015 (b) -- including RmlUi's own FreeType-backed DEFAULT engine,
#     which is a hard `find_package(Freetype REQUIRED)` above (no OFF path
#     exists for it, nor could one without changing bootstrap.cpp's font-engine
#     registration) -- so it is forced ON in A0, same FATAL_ERROR-on-OFF
#     pattern as GLINTFX_MODULE_IMAGE above and for the same reason (a real
#     OFF path is a source-level change, out of A0's scope). The pre-existing,
#     genuinely-independent GLINTFX_OWN_FONT_ENGINE selector DECLARED ABOVE
#     keeps being the toggle INSIDE this module (ADR-0015 (d)(4)) -- it is
#     untouched; its own sources become the glintfx_text OBJECT-library
#     target further below, real groundwork same as glintfx_image.
# PT: GLINTFX_MODULE_TEXT é a flag ENVOLVENTE do subsistema de texto inteiro
#     conforme a ADR-0015 (b) -- incluindo o motor DEFAULT do RmlUi apoiado em
#     FreeType, que é um `find_package(Freetype REQUIRED)` duro acima (nenhum
#     caminho OFF existe para ele, nem poderia existir sem mudar o registro de
#     motor de fonte do bootstrap.cpp) -- então fica forçada ON no A0, mesmo
#     padrão de FATAL_ERROR-em-OFF do GLINTFX_MODULE_IMAGE acima e pelo mesmo
#     motivo (um caminho OFF real é mudança de nível de source, fora do escopo
#     do A0). O seletor GLINTFX_OWN_FONT_ENGINE pré-existente e genuinamente
#     independente, DECLARADO ACIMA, continua sendo o interruptor DENTRO deste
#     módulo (ADR-0015 (d)(4)) -- permanece intocado; os próprios sources dele
#     viram o alvo de biblioteca OBJECT glintfx_text mais abaixo, trabalho de
#     base real igual ao glintfx_image.
option(GLINTFX_MODULE_TEXT
  "Text module: glyph raster + metrics, encloses RmlUi's mandatory FreeType default engine plus the optional GLINTFX_OWN_FONT_ENGINE selector (ADR-0015 (b); forced ON in A0, see comment above)"
  ON)
if(NOT GLINTFX_MODULE_TEXT)
  message(FATAL_ERROR
    "glintfx: GLINTFX_MODULE_TEXT=OFF is not yet supported -- RmlUi's default font engine "
    "requires FreeType unconditionally (find_package(Freetype REQUIRED) above); wiring a "
    "real OFF path needs source-level changes to bootstrap.cpp's font-engine registration, "
    "deferred past ADR-0015 phase A0. Tracked in docs/adr/0015-framework-2d-atomized-architecture.md.")
endif()

# --- GLINTFX_MODULE_I18N (real ON/OFF, framework-2D slice i18n-1) ---
# EN: Unlike GLINTFX_MODULE_IMAGE/TEXT above (reserved, forced ON because a real OFF path needs
#     source-level guards elsewhere), i18n genuinely costs nothing to gate for real, same as
#     GLINTFX_MODULE_WINDOW's "living embryo": src/i18n.cpp has zero callers anywhere else in
#     the library (nothing in render_gl3.cpp/bootstrap.cpp/engine.cpp/ui_layer.cpp/app.cpp calls
#     into glintfx::I18n -- ADR-0015 (b): "i18n" feeds the UI only via the pre-existing
#     bind_string/set_string surface, from the CONSUMER's code, never from glintfx's own
#     sources), so compiling it out drops both the translation unit and its symbols cleanly, no
#     FATAL_ERROR guard needed. Depends on `core` only (no GL, no window, no RmlUi type) -- see
#     glintfx_i18n's own add_library() block further below, which needs NEITHER glloader NOR any
#     find_package() result, unlike every other per-module OBJECT library in this file.
# PT: Diferente de GLINTFX_MODULE_IMAGE/TEXT acima (reservadas, forçadas ON porque um caminho
#     OFF real precisa de guardas em nível de source em outro lugar), o i18n genuinamente não
#     custa nada pra gatear de verdade, igual ao "embrião vivo" do GLINTFX_MODULE_WINDOW:
#     src/i18n.cpp não tem NENHUM chamador em outro lugar da biblioteca (nada em
#     render_gl3.cpp/bootstrap.cpp/engine.cpp/ui_layer.cpp/app.cpp chama glintfx::I18n --
#     ADR-0015 (b): o "i18n" alimenta a ui só via a superfície pré-existente bind_string/
#     set_string, a partir do código do CONSUMIDOR, nunca dos próprios sources da glintfx),
#     então compilar ele fora tira tanto a unidade de tradução quanto os símbolos dela de forma
#     limpa, sem precisar de guarda FATAL_ERROR. Depende só de `core` (zero GL, zero janela,
#     zero tipo do RmlUi) -- ver o próprio bloco add_library() do glintfx_i18n mais abaixo, que
#     não precisa NEM de glloader NEM de nenhum resultado de find_package(), diferente de toda
#     outra biblioteca OBJECT por-módulo deste arquivo.
option(GLINTFX_MODULE_I18N
  "i18n module: translation catalog + locale-aware lookup + CLDR-lite plurals (ADR-0015 (b); depends on core ONLY, genuinely OFF-able)"
  ON)

# --- GLINTFX_MODULE_AUDIO (real ON/OFF, framework-2D slice A3-AUDIO) ---
# EN: Sound-effect/music playback via vendored miniaudio. Same independence class as
#     GLINTFX_MODULE_I18N above: src/audio.cpp has zero callers anywhere else in the library
#     (nothing in render_gl3.cpp/bootstrap.cpp/engine.cpp/ui_layer.cpp/app.cpp calls into
#     glintfx::Audio -- ADR-0015 (b), consumer-only). Depends on `core` ONLY (no GL, no
#     window, no RmlUi type). No `audio-only` preset yet (would need GLINTFX_MODULE_UI=OFF,
#     which does not exist as a real flag yet -- see the "ui / fx" comment further below).
# PT: Reprodução de efeito sonoro/música via miniaudio vendorizado. Mesma classe de
#     independência do GLINTFX_MODULE_I18N acima: src/audio.cpp não tem NENHUM chamador em
#     outro lugar da biblioteca (nada em render_gl3.cpp/bootstrap.cpp/engine.cpp/
#     ui_layer.cpp/app.cpp chama glintfx::Audio -- ADR-0015 (b), só-consumidor). Depende só de
#     `core` (zero GL, zero janela, zero tipo do RmlUi). Ainda sem preset `audio-only` (exigiria
#     GLINTFX_MODULE_UI=OFF, que ainda não existe como flag real -- ver o comentário "ui / fx"
#     mais abaixo).
option(GLINTFX_MODULE_AUDIO
  "Audio module: sound-effect/music playback via vendored miniaudio (ADR-0015 (b); depends on core ONLY, genuinely OFF-able)"
  ON)

# --- GLINTFX_MODULE_GAMEPAD (real ON/OFF, framework-2D slice A2-GAMEPAD; Linux-only) ---
# EN: Same independence class as GLINTFX_MODULE_I18N/AUDIO above: src/gamepad_{decode,mapping,*}.cpp
#     have zero caller anywhere else in the library (ADR-0015 (b), consumer-only -- a game reads
#     `glintfx::Gamepads` from its own frame loop, alongside `App::set_frame_callback`, A1).
#     Depends on `core` ONLY (no GL, no window, no RmlUi type). UNLIKE i18n/audio, this module is
#     Linux-only BY NATURE: raw evdev (`<linux/input.h>`, `/dev/input/eventN`) is a Linux kernel
#     interface, not a portable one -- there is no cross-platform substitute to fall back to, so
#     the option is forced OFF (gracefully, `message(STATUS ...)`, never FATAL_ERROR) on any
#     non-Linux `CMAKE_SYSTEM_NAME` immediately below, mirroring GLINTFX_MODULE_WINDOW/APP's own
#     "shadow the cache variable so every guard below sees the effective value" pattern. See
#     ADR-0016 (docs/adr/, D1: gamepad ships as its OWN atom, not folded into the reserved
#     GLINTFX_MODULE_INPUT name) and the A2-GAMEPAD plan
#     (docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md) section 2.6.
# PT: Mesma classe de independência do GLINTFX_MODULE_I18N/AUDIO acima: src/gamepad_{decode,
#     mapping,*}.cpp não têm NENHUM chamador em outro lugar da biblioteca (ADR-0015 (b),
#     só-consumidor -- um jogo lê `glintfx::Gamepads` do próprio frame loop dele, junto de
#     `App::set_frame_callback`, A1). Depende só de `core` (zero GL, zero janela, zero tipo do
#     RmlUi). DIFERENTE de i18n/audio, este módulo é Linux-only POR NATUREZA: evdev cru
#     (`<linux/input.h>`, `/dev/input/eventN`) é uma interface do kernel Linux, não portável --
#     não há substituto cross-platform pro qual cair, então a option é forçada OFF (graciosamente,
#     `message(STATUS ...)`, nunca FATAL_ERROR) em qualquer `CMAKE_SYSTEM_NAME` não-Linux logo
#     abaixo, espelhando o próprio padrão "sombreia a variável de cache pra que toda guarda abaixo
#     veja o valor efetivo" do GLINTFX_MODULE_WINDOW/APP. Ver ADR-0016 (docs/adr/, D1: gamepad
#     entregue como átomo PRÓPRIO, não dobrado no nome reservado GLINTFX_MODULE_INPUT) e o plano
#     A2-GAMEPAD (docs/superpowers/plans/2026-07-21-framework2d-A2-gamepad.md) seção 2.6.
option(GLINTFX_MODULE_GAMEPAD
  "Gamepad module: raw-evdev pads + SDL-db mapping (ADR-0015/ADR-0016; core ONLY; Linux-only)"
  ON)
if(GLINTFX_MODULE_GAMEPAD AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "glintfx: gamepad module is Linux-only (raw evdev) -- forcing GLINTFX_MODULE_GAMEPAD=OFF on ${CMAKE_SYSTEM_NAME}")
  set(GLINTFX_MODULE_GAMEPAD OFF) # EN: shadow local: every guard below sees this. PT: sombra local: toda guarda abaixo enxerga isto.
endif()

# --- GLINTFX_MODULE_FX (real ON/OFF, FX-CARVE-1) ---
# EN: Carves the fx AUTHORED by glintfx (tint + ripple GLSL programs, their CompileShader/
#     RenderShader/ReleaseShader dispatch, the backdrop-capture machinery, and the 3 decorator
#     TUs decorator_polygon/image_tint/ripple.cpp) out of the ui module into their own
#     src/fx/effects_gl3.cpp OBJECT library, reached from render_gl3.cpp through the FxHook seam
#     (src/fx_hook.hpp). OFF removes the fx TU and every fx symbol from the binary for real (`nm`
#     proof: zero "Ripple"/"ImageTint"/"PolygonDecorator"/"Gl3FxHook" matches) -- an unrecognised
#     "decorator: polygon(...)"/"image-tint(...)"/"ripple(...)" in RCSS is then just ignored by
#     RmlUi (fail-high, bootstrap.cpp).
#     HONEST BOUNDARY (declared, not disguised -- ADR-0015 (c)'s own caveat, now narrowed by
#     FX-CARVE-1 instead of removed): glow/box-shadow/drop-shadow/blur/backdrop-blur/mask and
#     gradients are native RCSS features rendered by RmlUi's OWN upstream backend
#     (Backends/RmlUi_Renderer_GL3.cpp, compiled straight from the FetchContent tree) -- this
#     flag does NOT remove those shaders from the binary; doing so would require patching a
#     third-party file, real but low-ROI work deferred to a possible future FX-CARVE-2, only if
#     a consumer asks for a genuinely minimal binary. What IS glintfx-authored -- tint, ripple,
#     and polygon()'s own decorator wiring (polygon reuses RmlUi's stock gradient shader, no
#     custom GLSL of its own) -- is carved completely by this flag.
# PT: Recorta o fx AUTORAL da glintfx (programas GLSL de tint + ripple, o despacho deles de
#     CompileShader/RenderShader/ReleaseShader, a maquinaria de captura de backdrop, e as 3 TUs
#     de decorator decorator_polygon/image_tint/ripple.cpp) pra fora do módulo ui, numa
#     biblioteca OBJECT própria src/fx/effects_gl3.cpp, alcançada de render_gl3.cpp pela costura
#     FxHook (src/fx_hook.hpp). OFF remove a TU de fx e todo símbolo de fx do binário de verdade
#     (prova via `nm`: zero ocorrências de "Ripple"/"ImageTint"/"PolygonDecorator"/"Gl3FxHook")
#     -- um "decorator: polygon(...)"/"image-tint(...)"/"ripple(...)" não reconhecido no RCSS aí
#     é só ignorado pelo RmlUi (fail-high, bootstrap.cpp).
#     FRONTEIRA HONESTA (declarada, não disfarçada -- a própria ressalva da ADR-0015 (c), agora
#     estreitada pelo FX-CARVE-1 em vez de removida): glow/box-shadow/drop-shadow/blur/
#     backdrop-blur/mask e gradientes são features RCSS nativas renderizadas pelo PRÓPRIO backend
#     upstream do RmlUi (Backends/RmlUi_Renderer_GL3.cpp, compilado direto da árvore
#     FetchContent) -- esta flag NÃO remove esses shaders do binário; fazer isso exigiria patch
#     num arquivo de terceiro, trabalho real mas de baixo ROI, adiado pra um possível FX-CARVE-2
#     futuro, só se um consumidor pedir um binário genuinamente mínimo. O que É autoral da
#     glintfx -- tint, ripple, e a fiação de decorator do próprio polygon() (o polygon reusa o
#     shader de gradiente de fábrica do RmlUi, sem GLSL customizado próprio) -- é recortado por
#     completo por esta flag.
option(GLINTFX_MODULE_FX
  "Fx module: glintfx-AUTHORED shader effects (tint/ripple GLSL + backdrop-capture + the 3 decorator TUs), genuinely OFF-able (FX-CARVE-1); RmlUi's own upstream filter/gradient shaders are NOT gated (see doc string above, FX-CARVE-2 candidate)"
  ON)

# --- GLINTFX_MODULE_FX (real ON/OFF since FX-CARVE-1 -- see the option() block below) ---
# EN: FX-CARVE-1 carved the fx gate for real: the "glintfx-tint"/"glintfx-ripple" GLSL programs +
#     their CompileShader/RenderShader/ReleaseShader dispatch + the backdrop-capture machinery
#     (previously fused inside render_gl3.cpp) now live in the new src/fx/effects_gl3.cpp,
#     reached from ui through the FxHook seam (src/fx_hook.hpp). See GLINTFX_MODULE_FX's own
#     option() block further below for the honest boundary this carve stops at (RmlUi's own
#     upstream filter/gradient shaders are NOT gated -- see that option's doc string).
# PT: O FX-CARVE-1 recortou o gate de fx de verdade: os programas GLSL "glintfx-tint"/
#     "glintfx-ripple" + o despacho deles de CompileShader/RenderShader/ReleaseShader + a
#     maquinaria de captura de backdrop (antes fundidos dentro de render_gl3.cpp) agora moram no
#     novo src/fx/effects_gl3.cpp, alcançados pela ui através da costura FxHook
#     (src/fx_hook.hpp). Ver o próprio bloco option() de GLINTFX_MODULE_FX mais abaixo pra
#     fronteira honesta onde este recorte para (os próprios shaders upstream de filtro/gradiente
#     do RmlUi NÃO são gateados -- ver a doc string daquela option).

# --- ui: NOT introduced in A0 (see the header comment at the top of this ---
# --- section for the "why", and ADR-0015 (c) for the ratified split) ---
# EN: GLINTFX_MODULE_UI would gate render_gl3.cpp/bootstrap.cpp/engine.cpp/
#     data_binder.cpp/ui_layer.cpp/RmlUi itself -- today's whole product, with
#     no composition yet that needs it OFF (that arrives with A2 gamepad-only
#     / A3 audio-only, once those modules exist to compose WITHOUT ui). The
#     name is reserved by ADR-0015 (b) and will be introduced for real the
#     moment the first module exists that must compose without ui.
# PT: GLINTFX_MODULE_UI gatearia o render_gl3.cpp/bootstrap.cpp/engine.cpp/
#     data_binder.cpp/ui_layer.cpp/o próprio RmlUi -- o produto inteiro de
#     hoje, sem nenhuma composição ainda que precise dele OFF (isso chega com
#     o A2 gamepad-only / A3 audio-only, quando esses módulos existirem para
#     compor SEM a ui). O nome é reservado pela ADR-0015 (b) e será
#     introduzido de verdade no momento em que existir o primeiro módulo que
#     precisar compor sem a ui.

if(GLINTFX_OWN_FONT_ENGINE)
  # EN: Vendored paths (L1.20-FONTFLIP Phase 1) -- always present as part of the glintfx/
  #     subtree itself (glintfx/vendor/core/), so no existence guard/FATAL_ERROR is needed
  #     here anymore (unlike the pre-Phase-1 ../src / ../include/core layout). See
  #     glintfx/vendor/core/README.md for provenance and re-sync instructions.
  # PT: Paths vendorizados (L1.20-FONTFLIP Fase 1) -- sempre presentes como parte da própria
  #     subárvore glintfx/ (glintfx/vendor/core/), então nenhum guard de existência/
  #     FATAL_ERROR é mais necessário aqui (diferente do layout pré-Fase-1 ../src /
  #     ../include/core). Ver glintfx/vendor/core/README.md pra procedência e instruções de
  #     re-sincronização.
  set(_glx_core_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/vendor/core/src)
  set(_glx_core_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/vendor/core/include)
endif()

# Deps de sistema — OpenGL e FreeType sempre presentes; GLFW só no backend ON.
# System deps — OpenGL and FreeType are always present; GLFW only when the backend is ON.
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
if(GLINTFX_BACKEND_GLFW)
  find_package(glfw3 REQUIRED)
endif()

# RmlUi (pinado) — sem samples, estático, FreeType
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)

# EN: PATCH_COMMAND — applies patches/rmlui-2cd28864-teardown-ub.patch (tracked, reviewed
#     source patch fixing 2 genuine teardown-order UBs in RmlUi itself; see
#     glintfx/patches/README.md for the full writeup) explicitly and visibly to the fetched
#     RmlUi source, instead of silently forking the pin or leaving the UBs suppressed at the
#     symptom (see the now-emptied glintfx/tests/ubsan_suppressions.txt).
#
#     IDEMPOTENCY (non-negotiable — PATCH_COMMAND re-runs on every re-populate, e.g. a clean
#     reconfigure that still finds the source already checked out at _deps/rmlui-src): a bare
#     `git apply <patch>` would fail the SECOND time with "patch does not apply" because the
#     tree already carries the change. The command below is a two-step probe instead of a
#     one-shot apply: `git apply --reverse --check` asks "would REVERSING this patch succeed
#     cleanly?" — that is only true if the patch is ALREADY applied. If the probe succeeds
#     (patch already in), skip. If the probe fails (patch not in yet), apply it forward. Both
#     branches exit 0, so CMake never sees a failing PATCH_COMMAND on a routine reconfigure.
#     `sh -c "... || ..."` is required (not a bare multi-token COMMAND list) because CMake
#     does not interpret shell operators like `||` on its own — only a real shell does; this
#     repo's build is Linux-only (CLAUDE.md "Ambiente"), so `sh` is guaranteed present.
# PT: PATCH_COMMAND — aplica patches/rmlui-2cd28864-teardown-ub.patch (patch de fonte
#     rastreado e revisado que corrige 2 UBs genuínos de ordem-de-teardown no próprio RmlUi;
#     ver glintfx/patches/README.md para o relato completo) de forma explícita e visível ao
#     source fetchado do RmlUi, em vez de fazer fork silencioso do pin ou deixar os UBs
#     suprimidos no sintoma (ver o agora-esvaziado glintfx/tests/ubsan_suppressions.txt).
#
#     IDEMPOTÊNCIA (não-negociável — o PATCH_COMMAND roda de novo a cada re-populate, ex.:
#     uma reconfiguração limpa que ainda encontra o source já feito checkout em
#     _deps/rmlui-src): um `git apply <patch>` cru falharia na SEGUNDA vez com "patch does
#     not apply" porque a árvore já carrega a mudança. O comando abaixo é uma sonda em duas
#     etapas em vez de um apply único: `git apply --reverse --check` pergunta "REVERTER este
#     patch teria sucesso de forma limpa?" — isso só é verdade se o patch JÁ está aplicado.
#     Se a sonda tem sucesso (patch já dentro), pula. Se a sonda falha (patch ainda não
#     dentro), aplica pra frente. Os dois ramos saem com 0, então o CMake nunca vê um
#     PATCH_COMMAND falhando numa reconfiguração de rotina. `sh -c "... || ..."` é necessário
#     (não uma lista de COMMAND multi-token crua) porque o CMake não interpreta operadores de
#     shell como `||` sozinho — só um shell de verdade faz isso; o build deste repositório é
#     Linux-only (CLAUDE.md "Ambiente"), então `sh` está garantidamente presente.
set(_glintfx_rmlui_patch ${CMAKE_CURRENT_SOURCE_DIR}/patches/rmlui-2cd28864-teardown-ub.patch)
FetchContent_Declare(RmlUi
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG 2cd28864ae25ed345b70598751703a5433b12356
  PATCH_COMMAND sh -c "git apply --reverse --check '${_glintfx_rmlui_patch}' >/dev/null 2>&1 || git apply '${_glintfx_rmlui_patch}'")

FetchContent_MakeAvailable(RmlUi)

# EN: glloader (L1.14-GLLOADER) — glintfx's own GL 3.3 core-profile function-pointer
#     loader, generated from the public Khronos gl.xml registry by tools/gen_glloader.py
#     and committed under src/gl_loader.{h,c} (offline/reproducible, no network dependency
#     at configure time). Replaces the previously vendored gl3w third-party loader.
#     INTERFACE library (same shape the old gl3w target had): consumers get the include
#     paths (src/ for gl_loader.h, third_party/khronos/ for the Khronos glcorearb.h/
#     khrplatform.h headers gl_loader.h depends on) and gl_loader.c compiled in-place —
#     each target that links glloader compiles its own copy of gl_loader.c (mirrors the
#     old gl3w INTERFACE-library idiom so tests/ that talk to raw GL directly, e.g. via
#     offscreen.hpp/capture.hpp, keep working unchanged beyond the rename).
# PT: glloader (L1.14-GLLOADER) — loader próprio de ponteiros de função GL 3.3 core
#     profile do glintfx, gerado a partir do registro público gl.xml da Khronos por
#     tools/gen_glloader.py e commitado em src/gl_loader.{h,c} (offline/reprodutível, sem
#     dependência de rede em configure). Substitui o loader third-party gl3w antes
#     vendorizado.
#     Biblioteca INTERFACE (mesmo formato que o antigo target gl3w tinha): consumidores
#     recebem os include paths (src/ para gl_loader.h, third_party/khronos/ para os
#     headers Khronos glcorearb.h/khrplatform.h dos quais gl_loader.h depende) e
#     gl_loader.c compilado inline — cada target que linka glloader compila sua própria
#     cópia de gl_loader.c (espelha o idioma de biblioteca INTERFACE do antigo gl3w para
#     que os tests/ que falam GL cru diretamente, ex.: via offscreen.hpp/capture.hpp,
#     continuem funcionando sem mudança além da renomeação).
add_library(glloader INTERFACE)
target_sources(glloader INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/gl_loader.c>)
target_include_directories(glloader INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/khronos>)
target_link_libraries(glloader INTERFACE OpenGL::GL ${CMAKE_DL_LIBS})

# EN: RmlUi backends are not installed — compile directly from fetched source.
# PT: Os backends do RmlUi não fazem parte da lib instalada; compilar do source fetchado.
set(_rml_backends ${rmlui_SOURCE_DIR}/Backends)

# EN: Generate config.hpp into the build tree so consumers detect build options at
#     compile time (e.g. #if GLINTFX_BACKEND_GLFW). The build-interface include dir
#     is propagated transitively via INTERFACE_INCLUDE_DIRECTORIES.
# PT: Gera config.hpp na árvore de build para que consumidores detectem opções em
#     tempo de compilação (ex.: #if GLINTFX_BACKEND_GLFW). O include dir da interface
#     de build é propagado transitivamente via INTERFACE_INCLUDE_DIRECTORIES.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glintfx/config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/glintfx/config.hpp
  @ONLY)

# EN: Core ui sources — always compiled regardless of the window/app modules. Engine + UiLayer +
#     RenderGl3 (plus its GL3 backend) + Bootstrap. The 3 fx decorator TUs (decorator_polygon/
#     image_tint/ripple.cpp) moved OUT of this flat list, FX-CARVE-1: they are now the
#     glintfx_fx OBJECT-library target (further below, linked in only when GLINTFX_MODULE_FX=ON)
#     alongside the new src/fx/effects_gl3.cpp. version.cpp and system_clock.cpp
#     (L1.9-VERSEMBED) used to live in this list so glintfx::version()/SystemClock stayed
#     available in embed-only builds (GLINTFX_BACKEND_GLFW=OFF); as of ADR-0015 phase A0 they are
#     the glintfx_core OBJECT-library target instead (see further below, unconditionally linked
#     into glintfx), which keeps the exact same guarantee -- glintfx_core has no dependency on
#     any module flag. stb_image_impl.cpp moved the same way, into the glintfx_image OBJECT
#     library (also further below, unconditionally linked; GLINTFX_MODULE_IMAGE is forced ON in
#     A0, see its option() block above).
# PT: Sources principais de ui -- sempre compilados independente dos módulos window/app. Engine +
#     UiLayer + RenderGl3 (mais o backend GL3 dele) + Bootstrap. As 3 TUs de decorator de fx
#     (decorator_polygon/image_tint/ripple.cpp) SAÍRAM desta lista plana, FX-CARVE-1: agora são o
#     alvo de biblioteca OBJECT glintfx_fx (mais abaixo, linkado só quando GLINTFX_MODULE_FX=ON)
#     junto do novo src/fx/effects_gl3.cpp. version.cpp e system_clock.cpp (L1.9-VERSEMBED)
#     costumavam ficar nesta lista para que glintfx::version()/SystemClock continuassem
#     disponíveis em builds embed-only (GLINTFX_BACKEND_GLFW=OFF); a partir da fase A0 da
#     ADR-0015 eles são o alvo de biblioteca OBJECT glintfx_core (ver mais abaixo, linkado
#     incondicionalmente na glintfx), o que mantém exatamente a mesma garantia -- glintfx_core
#     não depende de nenhuma flag de módulo. stb_image_impl.cpp migrou do mesmo jeito, para a
#     biblioteca OBJECT glintfx_image (também mais abaixo, linkada incondicionalmente;
#     GLINTFX_MODULE_IMAGE fica forçada ON no A0, ver o bloco option() acima).
set(_glintfx_sources
  src/render_gl3.cpp
  src/bootstrap.cpp
  src/data_binder.cpp
  src/engine.cpp
  src/ui_layer.cpp
  ${_rml_backends}/RmlUi_Renderer_GL3.cpp)

if(GLINTFX_MODULE_APP)
  # EN: App-atom sources (ADR-0015 (b)) -- the standalone App facade plus the
  #     RmlUi GLFW SystemInterface glue app.cpp itself #includes. Gated on
  #     GLINTFX_MODULE_APP (not directly on GLINTFX_BACKEND_GLFW anymore --
  #     the two stay in lockstep by default, see the GLINTFX_MODULE_APP
  #     option() block above). window_glfw.cpp is NOT here: it is the separate
  #     "window" atom, compiled into the glintfx_window OBJECT library further
  #     below and linked in whenever GLINTFX_MODULE_WINDOW=ON (a dependency
  #     GLINTFX_MODULE_APP enforces via its own FATAL_ERROR guard above).
  # PT: Sources do átomo app (ADR-0015 (b)) -- a fachada App standalone mais a
  #     cola SystemInterface GLFW do RmlUi que o próprio app.cpp inclui.
  #     Gateado em GLINTFX_MODULE_APP (não mais diretamente em
  #     GLINTFX_BACKEND_GLFW -- os dois ficam em lockstep por padrão, ver o
  #     bloco option() de GLINTFX_MODULE_APP acima). window_glfw.cpp NÃO está
  #     aqui: é o átomo separado "window", compilado na biblioteca OBJECT
  #     glintfx_window mais abaixo e linkado sempre que GLINTFX_MODULE_WINDOW=ON
  #     (dependência que GLINTFX_MODULE_APP reforça via sua própria guarda
  #     FATAL_ERROR acima).
  list(APPEND _glintfx_sources
    src/app.cpp
    ${_rml_backends}/RmlUi_Platform_GLFW.cpp)
endif()

add_library(glintfx STATIC ${_glintfx_sources})
target_include_directories(glintfx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  PRIVATE
    ${_rml_backends}
    # EN: stb_image header-only library (PNG/JPG/TGA decode) — vendored under third_party/stb/.
    # PT: Biblioteca header-only stb_image (decode PNG/JPG/TGA) — vendorizada em third_party/stb/.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
  # EN: glloader include path comes transitively from linking PRIVATE glloader (INTERFACE target).
  # PT: Include path do glloader vem transitivamente do link PRIVATE glloader (alvo INTERFACE).

# EN: L1.19-FONTENG (FT-F3, L1.20-FONTFLIP Phase 1) — the vendored core/ include root
#     (glintfx/vendor/core/include, NOT Layer 0's full ../include/ anymore -- needed so
#     font_engine_own.hpp's `#include "core/sfnt.h"` / `"core/raster.h"` / `"core/hint.h"`
#     resolve, matching those headers' own documented consumption contract -- see
#     include/core/sfnt.h's file header, "meant to be consumable from a future HOSTED C++
#     shell the same way glx_malloc/glx_sfnt_open already are"). Applied via
#     set_source_files_properties(... INCLUDE_DIRECTORIES ...) to ONLY the files that need it
#     (font_engine_own.cpp, bootstrap.cpp, sfnt.c, raster.c, hint.c), NOT target-wide
#     target_include_directories() -- kept scoped even though the historical collision that
#     motivated this (Layer 0's full include/limits.h, a minimal freestanding
#     <limits.h>-equivalent with only INT_MAX/INT_MIN/UINT_MAX, SHADOWING the system
#     <limits.h> for every OTHER translation unit once the whole ../include/ root was on the
#     search path -- broke src/stb_image_impl.cpp's vendored stb_image.h with "use of
#     undeclared identifier 'SHRT_MAX'" the first time this was wired target-wide) cannot
#     reproduce against the vendored path: glintfx/vendor/core/include/ contains ONLY
#     core/{sfnt,raster,hint}.h (see glintfx/vendor/core/README.md), not Layer 0's full
#     include/ root -- no limits.h, no types.h, nothing else to shadow. Scoping is retained
#     anyway as defensive/minimal-blast-radius practice, not because the specific collision
#     still applies.
# PT: L1.19-FONTENG (FT-F3, L1.20-FONTFLIP Fase 1) — a raiz include core/ vendorizada
#     (glintfx/vendor/core/include, NÃO mais o ../include/ completo da Camada 0 -- necessária
#     para que os `#include "core/sfnt.h"` / `"core/raster.h"` / `"core/hint.h"` do
#     font_engine_own.hpp resolvam, batendo com o próprio contrato de consumo documentado
#     daqueles headers -- ver o cabeçalho de arquivo do include/core/sfnt.h, "pensado pra ser
#     consumível de uma futura casca C++ HOSTED do mesmo jeito que glx_malloc/glx_sfnt_open já
#     são"). Aplicada via set_source_files_properties(... INCLUDE_DIRECTORIES ...) SÓ nos
#     arquivos que precisam dela (font_engine_own.cpp, bootstrap.cpp, sfnt.c, raster.c,
#     hint.c), NÃO via target_include_directories() de escopo de alvo inteiro -- mantido
#     escopado mesmo que a colisão histórica que motivou isto (o include/limits.h COMPLETO da
#     Camada 0, um equivalente freestanding mínimo a <limits.h> só com INT_MAX/INT_MIN/
#     UINT_MAX, SOMBREANDO o <limits.h> de sistema para toda OUTRA unidade de tradução uma vez
#     que a raiz ../include/ inteira entrava no path de busca -- quebrou o stb_image.h
#     vendorizado do src/stb_image_impl.cpp com "use of undeclared identifier 'SHRT_MAX'" na
#     1ª vez que isto foi conectado de escopo de alvo inteiro) não reproduz mais contra o path
#     vendorizado: glintfx/vendor/core/include/ contém SÓ core/{sfnt,raster,hint}.h (ver
#     glintfx/vendor/core/README.md), não a raiz include/ completa da Camada 0 -- sem
#     limits.h, sem types.h, nada mais pra sombrear. O escopo é mantido mesmo assim como
#     prática defensiva/de raio-de-explosão mínimo, não porque a colisão específica ainda se
#     aplique.
if(GLINTFX_OWN_FONT_ENGINE)
  # EN: src/bootstrap.cpp is included here too -- it `#include "font_engine_own.hpp"` itself
  #     (guarded by `#if GLINTFX_OWN_FONT_ENGINE`, src/bootstrap.cpp) to hold/register the
  #     glintfx::FontEngineOwn instance; that header transitively needs "core/sfnt.h" /
  #     "core/raster.h" resolvable from bootstrap.cpp's own translation unit too. Still confirmed
  #     collision-free: bootstrap.cpp's OWN #include list (grep-checked) never names <limits.h>
  #     or any other standard-library header name Layer 0's include/ root also happens to
  #     provide.
  # PT: src/bootstrap.cpp também entra aqui -- ele próprio faz `#include "font_engine_own.hpp"`
  #     (protegido por `#if GLINTFX_OWN_FONT_ENGINE`, src/bootstrap.cpp) para guardar/registrar
  #     a instância de glintfx::FontEngineOwn; aquele header precisa transitivamente de
  #     "core/sfnt.h" / "core/raster.h" resolvíveis também a partir da própria unidade de
  #     tradução do bootstrap.cpp. Ainda confirmado livre de colisão: a PRÓPRIA lista de
  #     #include do bootstrap.cpp (checada via grep) nunca nomeia <limits.h> nem nenhum outro
  #     nome de header de biblioteca padrão que a raiz include/ da Camada 0 por acaso também
  #     forneça.
  set_source_files_properties(
    src/bootstrap.cpp
    src/font_engine_own.cpp
    ${_glx_core_src_dir}/sfnt.c
    ${_glx_core_src_dir}/raster.c
    ${_glx_core_src_dir}/hint.c
    PROPERTIES INCLUDE_DIRECTORIES ${_glx_core_include_dir})
endif()

# ===========================================================================
# EN: ADR-0015 phase A0 -- per-module OBJECT-library TARGETS (the CMake
#     mechanics of (d)(5)): one target per module that is cleanly separable
#     today (core, image, text, window), plus the `glintfx::<module>` alias
#     each, mirroring the existing `glintfx::glintfx` alias further below.
#     Positioned here (after config.hpp/RmlUi/glloader/the include-dir
#     override above, all of which some of these targets need) rather than
#     earlier next to the flag declarations, which only need the flag VALUES,
#     not the targets these sources compile against.
#
#     Every one of these OBJECT libraries is linked PRIVATE into the
#     aggregate `glintfx` STATIC target right below via target_link_libraries
#     (not the older $<TARGET_OBJECTS:...> idiom) specifically so their own
#     usage requirements (include dirs, and — for glintfx_window — the
#     transitive glfw/glloader link deps) are inherited automatically; see
#     CMake's own Object Libraries docs. install(TARGETS glintfx ARCHIVE ...)
#     further below folds their .o files into libglintfx.a exactly as it did
#     before this refactor moved them out of the flat `_glintfx_sources` list
#     -- no install() changes needed for any of the four.
#
#     ui and fx are deliberately NOT object libraries here: see the "ui / fx:
#     NOT introduced in A0" note next to the GLINTFX_MODULE_TEXT option()
#     block above. Their sources stay directly in `_glintfx_sources` and
#     compile straight into the `glintfx` target, unchanged from before this
#     refactor.
# PT: ADR-0015 fase A0 -- ALVOS de biblioteca OBJECT por módulo (a mecânica
#     CMake do (d)(5)): um alvo por módulo que é limpamente separável hoje
#     (core, image, text, window), mais o alias `glintfx::<módulo>` de cada
#     um, espelhando o alias `glintfx::glintfx` já existente mais abaixo.
#     Posicionado aqui (depois do config.hpp/RmlUi/glloader/da sobrescrita de
#     include-dir acima, dos quais alguns destes alvos precisam) em vez de
#     mais cedo, junto às declarações de flag, que só precisam dos VALORES das
#     flags, não dos alvos contra os quais estes sources compilam.
#
#     Cada uma destas bibliotecas OBJECT é linkada PRIVATE no alvo agregado
#     `glintfx` STATIC logo abaixo via target_link_libraries (não o idioma
#     mais antigo $<TARGET_OBJECTS:...>) especificamente para que os próprios
#     usage requirements delas (include dirs e, para o glintfx_window, as deps
#     de link transitivas glfw/glloader) sejam herdados automaticamente; ver a
#     própria documentação de Object Libraries do CMake. O
#     install(TARGETS glintfx ARCHIVE ...) mais abaixo dobra os arquivos .o
#     delas em libglintfx.a exatamente como fazia antes deste refactor tirá-los
#     da lista plana `_glintfx_sources` -- nenhuma mudança de install()
#     necessária para nenhum dos quatro.
#
#     ui e fx deliberadamente NÃO são bibliotecas OBJECT aqui: ver a nota "ui /
#     fx: NOT introduced in A0" junto ao bloco option() de GLINTFX_MODULE_TEXT
#     acima. Os sources deles continuam direto na `_glintfx_sources` e
#     compilam direto pro alvo `glintfx`, sem mudança em relação a antes deste
#     refactor.
# ===========================================================================

# --- glintfx_core (always ON, "root" per ADR-0015 (b); no flag needed) ---
# EN: system_clock.hpp (SystemClock : Rml::SystemInterface) needs
#     <RmlUi/Core/SystemInterface.h> -- RmlUi::Core is linked PRIVATE here for
#     that reason (found only once this was actually built: a first attempt
#     without it failed with "'RmlUi/Core/SystemInterface.h' file not found",
#     confirming SystemClock is not as RmlUi-independent as the ADR-0015 (b)
#     module table's own "core ... depends on nothing" entry implies -- the
#     table describes the ADR's target-state boundary, not every current
#     implementation detail; A0 does not change SystemClock's actual
#     dependency, only documents it accurately here).
# PT: system_clock.hpp (SystemClock : Rml::SystemInterface) precisa de
#     <RmlUi/Core/SystemInterface.h> -- RmlUi::Core é linkado PRIVATE aqui por
#     isso (achado só ao efetivamente buildar isto: uma primeira tentativa sem
#     ele falhou com "'RmlUi/Core/SystemInterface.h' file not found",
#     confirmando que SystemClock não é tão independente do RmlUi quanto a
#     própria entrada "core ... depende de nada" da tabela de módulos da
#     ADR-0015 (b) sugere -- a tabela descreve a fronteira de estado-alvo da
#     ADR, não todo detalhe de implementação atual; o A0 não muda a
#     dependência real do SystemClock, só a documenta com exatidão aqui).
add_library(glintfx_core OBJECT src/system_clock.cpp src/version.cpp)
target_include_directories(glintfx_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_link_libraries(glintfx_core PRIVATE RmlUi::Core)
target_compile_options(glintfx_core PRIVATE -Wall -Wextra)
glintfx_instrument_coverage(glintfx_core)
add_library(glintfx::core ALIAS glintfx_core)

# --- glintfx_image (GLINTFX_MODULE_IMAGE forced ON in A0, see option() above) ---
add_library(glintfx_image OBJECT src/stb_image_impl.cpp)
target_include_directories(glintfx_image PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
target_compile_options(glintfx_image PRIVATE -Wall -Wextra)
glintfx_instrument_coverage(glintfx_image)
add_library(glintfx::image ALIAS glintfx_image)

# --- glintfx_text (GLINTFX_MODULE_TEXT forced ON in A0; sources only exist ---
# --- when the pre-existing GLINTFX_OWN_FONT_ENGINE selector is ON, unchanged ---
if(GLINTFX_OWN_FONT_ENGINE)
  add_library(glintfx_text OBJECT
    src/font_engine_own.cpp
    ${_glx_core_src_dir}/sfnt.c
    ${_glx_core_src_dir}/raster.c
    ${_glx_core_src_dir}/hint.c)  # EN/PT: SOV-HINT (L1.20-FONTFLIP FT-F4) Y grid-fitter, wired in sub-phase 1.4.
  target_include_directories(glintfx_text PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
  # EN: RmlUi::Core for <RmlUi/Core/*.h> (font_engine_own.hpp); the vendored
  #     core/{sfnt,raster,hint}.h include dir is already applied to these exact
  #     source files via the set_source_files_properties() call right above,
  #     which is file-scoped (not target-scoped) and therefore still applies
  #     here unchanged.
  # PT: RmlUi::Core para <RmlUi/Core/*.h> (font_engine_own.hpp); o include dir
  #     vendorizado core/{sfnt,raster,hint}.h já é aplicado a estes exatos
  #     arquivos-fonte via a chamada set_source_files_properties() logo acima,
  #     que é escopada por arquivo (não por alvo) e por isso continua se
  #     aplicando aqui sem mudança.
  target_link_libraries(glintfx_text PRIVATE RmlUi::Core)
  target_compile_options(glintfx_text PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_text)
  add_library(glintfx::text ALIAS glintfx_text)
endif()

# --- glintfx_window (GLINTFX_MODULE_WINDOW, genuinely OFF-able today -- the ---
# --- "living embryo" ADR-0015 (a) cites, unchanged since v0.2.1) ---
if(GLINTFX_MODULE_WINDOW)
  add_library(glintfx_window OBJECT src/window_glfw.cpp)
  # EN: gl_loader.h comes transitively from linking glloader (INTERFACE); GLFW's
  #     own headers come transitively from linking the glfw imported target.
  #     window_glfw.hpp now includes ONE glintfx PUBLIC header, <glintfx/ui_event.hpp>
  #     (A1, framework-2D: WindowGlfw::set_event_sink/dispatch construct and forward UiEvent
  #     instances -- docs/superpowers/plans/2026-07-19-framework2d-A1-input.md section 2.2), so
  #     this target now needs the public include dir explicitly (it previously did not --
  #     window_glfw.hpp used to be fully self-contained). PRIVATE is enough: only this OBJECT
  #     library's own compilation resolves the include; downstream consumers of the aggregate
  #     `glintfx` target already get the public include dir from ITS OWN
  #     target_include_directories() call, unrelated to this one.
  # PT: gl_loader.h vem transitivamente de linkar glloader (INTERFACE); os
  #     próprios headers do GLFW vêm transitivamente de linkar o alvo
  #     importado glfw. window_glfw.hpp agora inclui UM header PÚBLICO da glintfx,
  #     <glintfx/ui_event.hpp> (A1, framework-2D: WindowGlfw::set_event_sink/dispatch
  #     constroem e repassam instâncias de UiEvent --
  #     docs/superpowers/plans/2026-07-19-framework2d-A1-input.md seção 2.2), então este alvo
  #     agora precisa do include dir público explicitamente (antes não precisava --
  #     window_glfw.hpp era totalmente auto-contido). PRIVATE basta: só a compilação desta
  #     biblioteca OBJECT resolve o include; consumidores do alvo agregado `glintfx` já
  #     recebem o include dir público da PRÓPRIA chamada target_include_directories() dele,
  #     sem relação com esta.
  target_include_directories(glintfx_window PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
  target_link_libraries(glintfx_window PRIVATE glloader glfw)
  target_compile_options(glintfx_window PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_window)
  add_library(glintfx::window ALIAS glintfx_window)
endif()

# --- glintfx_i18n (GLINTFX_MODULE_I18N, real ON/OFF -- see its option() block above) ---
# EN: The leanest OBJECT library in this file: one source, one PUBLIC include dir (for
#     <glintfx/i18n.hpp> itself), no PRIVATE link deps at all -- i18n.cpp #includes only its own
#     public header plus <cstdlib>/<fstream>/<sstream> from the C++ standard library (confirmed
#     by reading the file: no RmlUi, no GL, no glloader, no third-party header of any kind).
#     This is the concrete proof of ADR-0015 (b)'s "i18n depends on core ONLY" claim, not just a
#     table entry -- unlike glintfx_core (needs RmlUi::Core for SystemClock) or glintfx_text
#     (needs RmlUi::Core + the vendored core/ include dir), glintfx_i18n links against nothing.
# PT: A biblioteca OBJECT mais enxuta deste arquivo: um source, um include dir PUBLIC (pro
#     próprio <glintfx/i18n.hpp>), nenhuma dep de link PRIVATE — i18n.cpp inclui só o próprio
#     header público mais <cstdlib>/<fstream>/<sstream> da biblioteca padrão C++ (confirmado
#     lendo o arquivo: sem RmlUi, sem GL, sem glloader, sem header de terceiro de nenhum tipo).
#     Esta é a prova concreta da alegação "i18n depende SÓ de core" da ADR-0015 (b), não só uma
#     entrada de tabela -- diferente do glintfx_core (precisa de RmlUi::Core pro SystemClock) ou
#     do glintfx_text (precisa de RmlUi::Core + o include dir vendorizado core/), o glintfx_i18n
#     não linka contra nada.
if(GLINTFX_MODULE_I18N)
  add_library(glintfx_i18n OBJECT src/i18n.cpp)
  target_include_directories(glintfx_i18n PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
  target_compile_options(glintfx_i18n PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_i18n)
  add_library(glintfx::i18n ALIAS glintfx_i18n)
endif()

# --- glintfx_fx (GLINTFX_MODULE_FX, real ON/OFF, FX-CARVE-1 -- see its option() block above) ---
# EN: The glintfx-AUTHORED shader effects: src/fx/effects_gl3.cpp (tint + ripple GLSL programs,
#     their CompileShader/RenderShader/ReleaseShader dispatch, the backdrop-capture machinery --
#     moved verbatim out of render_gl3.cpp) plus the 3 decorator TUs (polygon/image_tint/ripple)
#     that were previously flat in `_glintfx_sources`. Needs RmlUi::Core (Element/Geometry/
#     Property/RenderManager/StyleSheetSpecification/... headers, all 3 decorators) and glloader
#     (gl_loader.h + the Khronos include dir it carries transitively, every source in this
#     target). The ${_rml_backends} include dir resolves effects_gl3.cpp's own
#     "RmlUi_Renderer_GL3.h" include (needed for the full RenderInterface_GL3 definition the
#     qualified `ri.RenderInterface_GL3::...` calls require, ADR-0010 Decision (a)) -- the same
#     path render_gl3.cpp itself already uses via its own PRIVATE target_include_directories()
#     call further below. Public+binary include dirs mirror every sibling per-module OBJECT
#     library in this file even though nothing in this module currently #includes a public
#     <glintfx/*.hpp> header -- kept for consistency/forward-compat with glintfx_window/
#     glintfx_text's own shape, cheap to carry.
# PT: Os efeitos de shader AUTORAIS da glintfx: src/fx/effects_gl3.cpp (programas GLSL de tint +
#     ripple, o despacho deles de CompileShader/RenderShader/ReleaseShader, a maquinaria de
#     captura de backdrop -- movidos literalmente pra fora de render_gl3.cpp) mais as 3 TUs de
#     decorator (polygon/image_tint/ripple) que antes ficavam planas em `_glintfx_sources`.
#     Precisa de RmlUi::Core (headers Element/Geometry/Property/RenderManager/
#     StyleSheetSpecification/..., todos os 3 decorators) e glloader (gl_loader.h + o include dir
#     Khronos que ele carrega transitivamente, todo source deste alvo). O include dir
#     ${_rml_backends} resolve o próprio #include "RmlUi_Renderer_GL3.h" de effects_gl3.cpp
#     (necessário pra definição completa de RenderInterface_GL3 que as chamadas qualificadas
#     `ri.RenderInterface_GL3::...` exigem, Decisão (a) do ADR-0010) -- o mesmo path que o próprio
#     render_gl3.cpp já usa via sua própria chamada target_include_directories() PRIVATE mais
#     abaixo. Include dirs public+binary espelham toda biblioteca OBJECT por-módulo irmã deste
#     arquivo mesmo que nada neste módulo hoje inclua um header público <glintfx/*.hpp> -- mantido
#     por consistência/forward-compat com a própria forma do glintfx_window/glintfx_text, barato
#     de carregar.
if(GLINTFX_MODULE_FX)
  add_library(glintfx_fx OBJECT
    src/fx/effects_gl3.cpp
    src/decorator_polygon.cpp
    src/decorator_image_tint.cpp
    src/decorator_ripple.cpp)
  target_include_directories(glintfx_fx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
    ${_rml_backends})
  target_link_libraries(glintfx_fx PRIVATE RmlUi::Core glloader)
  target_compile_options(glintfx_fx PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_fx)
  add_library(glintfx::fx ALIAS glintfx_fx)
endif()

# --- glintfx_audio (GLINTFX_MODULE_AUDIO, real ON/OFF -- see its option() block above) ---
# EN: src/audio.cpp (ours, -Wall -Wextra) + src/miniaudio_impl.c (third-party, -w below).
#     Threads::Threads + CMAKE_DL_LIBS + m: miniaudio dlopen's ALSA/PulseAudio/etc at
#     runtime -- zero system audio link dependency, same "no hard link dep, discover at
#     runtime" spirit as glloader's own OpenGL::GL + CMAKE_DL_LIBS pattern above.
# PT: src/audio.cpp (nosso, -Wall -Wextra) + src/miniaudio_impl.c (terceiro, -w abaixo).
#     Threads::Threads + CMAKE_DL_LIBS + m: o miniaudio faz dlopen de ALSA/PulseAudio/etc em
#     runtime -- zero dependência de link de sistema de áudio, mesmo espírito "sem dep de
#     link rígida, descoberta em runtime" do próprio padrão OpenGL::GL + CMAKE_DL_LIBS do
#     glloader acima.
if(GLINTFX_MODULE_AUDIO)
  find_package(Threads REQUIRED)
  set_source_files_properties(src/miniaudio_impl.c PROPERTIES COMPILE_OPTIONS "-w")
  add_library(glintfx_audio OBJECT src/audio.cpp src/miniaudio_impl.c)
  target_include_directories(glintfx_audio PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio)
  target_link_libraries(glintfx_audio PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
  target_compile_options(glintfx_audio PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_audio)
  add_library(glintfx::audio ALIAS glintfx_audio)
endif()

# --- glintfx_gamepad (GLINTFX_MODULE_GAMEPAD, real ON/OFF -- see its option() block above) ---
# EN: Wave 1 (Agent A) compiles only the decode+mapping pair below -- src/gamepad_decode.cpp
#     (byte-stream reassembly + state) and src/gamepad_mapping.cpp (GUID/layout/DB-parser/
#     default-kernel-spec). Wave 2 (Agent B) appends src/gamepad.cpp (the device layer: fd scan,
#     ioctl, inotify hotplug) to THIS SAME add_library() source list -- do not create a second
#     OBJECT library for it. No PRIVATE link deps at all (same "core ONLY" shape as glintfx_i18n
#     above): evdev/inotify are kernel UAPI headers + syscalls, not a linkable system library.
# PT: A onda 1 (Agente A) compila só o par decode+mapping abaixo -- src/gamepad_decode.cpp
#     (reagrupamento de byte-stream + estado) e src/gamepad_mapping.cpp (GUID/layout/parser de
#     DB/default kernel-spec). A onda 2 (Agente B) acrescenta src/gamepad.cpp (a camada de
#     device: varredura de fd, ioctl, hotplug via inotify) a ESTA MESMA lista de sources do
#     add_library() -- não cria uma segunda biblioteca OBJECT pra ele. Nenhuma dep de link
#     PRIVATE (mesma forma "só core" do glintfx_i18n acima): evdev/inotify são headers UAPI do
#     kernel + syscalls, não uma biblioteca de sistema linkável.
if(GLINTFX_MODULE_GAMEPAD)
  add_library(glintfx_gamepad OBJECT src/gamepad_decode.cpp src/gamepad_mapping.cpp)
  target_include_directories(glintfx_gamepad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gamecontrollerdb)
  target_compile_options(glintfx_gamepad PRIVATE -Wall -Wextra)
  glintfx_instrument_coverage(glintfx_gamepad)
  add_library(glintfx::gamepad ALIAS glintfx_gamepad)
endif()

# EN: Fold every module OBJECT library into the aggregate `glintfx` STATIC
#     target. glintfx_core/glintfx_image are unconditional (their flags are
#     forced ON in A0); glintfx_text/glintfx_window/glintfx_i18n/glintfx_fx/glintfx_audio/
#     glintfx_gamepad mirror the same guard their own add_library() call above is under.
# PT: Dobra cada biblioteca OBJECT de módulo no alvo agregado `glintfx`
#     STATIC. glintfx_core/glintfx_image são incondicionais (as flags deles
#     são forçadas ON no A0); glintfx_text/glintfx_window/glintfx_i18n/glintfx_fx/
#     glintfx_audio/glintfx_gamepad espelham a mesma guarda sob a qual a própria chamada
#     add_library() delas está acima.
target_link_libraries(glintfx PRIVATE glintfx_core glintfx_image)
if(GLINTFX_OWN_FONT_ENGINE)
  target_link_libraries(glintfx PRIVATE glintfx_text)
endif()
if(GLINTFX_MODULE_WINDOW)
  target_link_libraries(glintfx PRIVATE glintfx_window)
endif()
if(GLINTFX_MODULE_I18N)
  target_link_libraries(glintfx PRIVATE glintfx_i18n)
endif()
if(GLINTFX_MODULE_FX)
  target_link_libraries(glintfx PRIVATE glintfx_fx)
endif()
if(GLINTFX_MODULE_AUDIO)
  target_link_libraries(glintfx PRIVATE glintfx_audio)
endif()
if(GLINTFX_MODULE_GAMEPAD)
  target_link_libraries(glintfx PRIVATE glintfx_gamepad)
endif()

target_compile_definitions(glintfx PRIVATE
  RMLUI_GL3_CUSTOM_LOADER=<gl_loader.h>
  # EN: Disable MSAA for the render layer — required under Mesa/llvmpipe (Xvfb) where
  #     glBlitFramebuffer MSAA→non-MSAA resolve silently produces a black postprocess texture.
  #     With samples=0 the render layer uses a plain texture FBO (no MSAA renderbuffer),
  #     making the resolve blit a simple copy that works correctly under software GL.
  # PT: Desabilita MSAA no render layer — necessário sob Mesa/llvmpipe (Xvfb) onde
  #     glBlitFramebuffer MSAA→não-MSAA produz silenciosamente textura de pós-processo preta.
  #     Com samples=0 o render layer usa FBO de textura simples (sem renderbuffer MSAA),
  #     tornando o blit de resolução uma cópia simples que funciona no GL software.
  RMLUI_NUM_MSAA_SAMPLES=0
)

# EN: Core link deps (always): RmlUi, OpenGL, FreeType, glloader.
#     glfw is only linked when the GLFW window backend is enabled.
# PT: Deps de link principais (sempre): RmlUi, OpenGL, FreeType, glloader.
#     glfw só é linkado quando o backend de janela GLFW está habilitado.
target_link_libraries(glintfx PRIVATE RmlUi::Core OpenGL::GL Freetype::Freetype glloader)
if(GLINTFX_BACKEND_GLFW)
  target_link_libraries(glintfx PRIVATE glfw)
endif()
target_compile_features(glintfx PUBLIC cxx_std_17)

# EN: -Wall -Wextra (CONTRACT §10.4 build-hygiene gap, chore/lw-aud) — applied via
#     target_compile_options(glintfx PRIVATE ...), i.e. scoped to THIS single target only,
#     never via directory-wide add_compile_options(). PRIVATE also means the flags never leak
#     to consumers via usage requirements (a FetchContent/find_package consumer must not be
#     forced to build under our warning flags). Deliberately does NOT touch RmlUi (~200 fetched
#     translation units), glloader (INTERFACE, gl_loader.c compiled per-consumer) or the
#     tests/demos subdirectories (26 test targets + 2 demo targets, out of scope for this
#     pass) -- instrumenting those would drown our own signal in third-party/generated-code
#     noise we do not control. No -Werror yet: 87 clang-tidy findings are already tracked as
#     out-of-scope debt (see AUDITORIAS.md); promoting -Wall/-Wextra to a hard gate is a
#     separate future decision, not bundled here.
# PT: -Wall -Wextra (gap de higiene de build do CONTRACT §10.4, chore/lw-aud) — aplicado via
#     target_compile_options(glintfx PRIVATE ...), ou seja, escopado só a ESTE alvo, nunca via
#     add_compile_options() de diretório inteiro. PRIVATE também significa que as flags nunca
#     vazam a consumidores via usage requirements (um consumidor via FetchContent/find_package
#     não deve ser forçado a buildar sob nossas flags de warning). Deliberadamente NÃO toca no
#     RmlUi (~200 unidades de tradução fetchadas), no glloader (INTERFACE, gl_loader.c
#     compilado por-consumidor) nem nos subdiretórios tests/demos (26 alvos de teste + 2 de
#     demo, fora de escopo nesta passada) -- instrumentar esses afogaria nosso próprio sinal em
#     ruído de código de terceiro/gerado que não controlamos. Ainda sem -Werror: 87 achados de
#     clang-tidy já são débito rastreado fora de escopo (ver AUDITORIAS.md); promover
#     -Wall/-Wextra a gate duro é uma decisão futura separada, não incluída aqui.
target_compile_options(glintfx PRIVATE -Wall -Wextra)

# EN: The two RmlUi_*.cpp Backends files (${_rml_backends}/RmlUi_Renderer_GL3.cpp always,
#     RmlUi_Platform_GLFW.cpp only when GLINTFX_BACKEND_GLFW=ON) are compiled directly as
#     glintfx sources but are NOT ours -- they are RmlUi's own upstream backend source,
#     referenced straight out of the FetchContent tree (${rmlui_SOURCE_DIR}/Backends), not
#     even a vendored copy we could patch. -Wall -Wextra above is target-wide (target_compile_
#     options has no per-source-subset form), so without this override these two files would
#     leak third-party warning noise into our signal (confirmed: RmlUi_Renderer_GL3.cpp emits
#     -Wunused-parameter at line 2142, code we do not author and would not "fix" upstream from
#     here). -w on just these two source files silences that noise at its origin instead of
#     polluting the target's warning output; it does not affect any of our own 17 authored
#     sources in _glintfx_sources. Harmless to list RmlUi_Platform_GLFW.cpp unconditionally --
#     the property is a no-op if the file is never added to the target's source list
#     (GLINTFX_BACKEND_GLFW=OFF).
# PT: Os dois arquivos RmlUi_*.cpp de Backends (${_rml_backends}/RmlUi_Renderer_GL3.cpp
#     sempre, RmlUi_Platform_GLFW.cpp só com GLINTFX_BACKEND_GLFW=ON) são compilados
#     diretamente como sources da glintfx mas NÃO são nossos -- são o próprio backend source
#     upstream do RmlUi, referenciado direto da árvore FetchContent (${rmlui_SOURCE_DIR}/
#     Backends), nem sequer uma cópia vendorizada que poderíamos remendar. O -Wall -Wextra
#     acima é de escopo de alvo inteiro (target_compile_options não tem forma por-subconjunto-
#     de-source), então sem este override esses dois arquivos vazariam ruído de warning de
#     terceiro pro nosso sinal (confirmado: RmlUi_Renderer_GL3.cpp emite -Wunused-parameter na
#     linha 2142, código que não autoramos e não "consertaríamos" upstream daqui). -w só
#     nesses dois source files silencia o ruído na origem em vez de poluir a saída de warning
#     do alvo; não afeta nenhum dos nossos 17 sources autorais em _glintfx_sources. Inofensivo
#     listar RmlUi_Platform_GLFW.cpp incondicionalmente -- a property é no-op se o arquivo
#     nunca entrar na lista de sources do alvo (GLINTFX_BACKEND_GLFW=OFF).
set_source_files_properties(
  ${_rml_backends}/RmlUi_Renderer_GL3.cpp
  ${_rml_backends}/RmlUi_Platform_GLFW.cpp
  PROPERTIES COMPILE_OPTIONS "-w")

# EN: L1.18-COVCI -- instrument the glintfx library itself (no-op when GLINTFX_COVERAGE=OFF).
#     See the GLINTFX_COVERAGE option block above for the full rationale (per-target, not
#     directory-wide; why RmlUi/_deps stay uninstrumented; the demos/GLINTFX_BUILD_DEMOS caveat).
# PT: L1.18-COVCI -- instrumenta a própria biblioteca glintfx (no-op com GLINTFX_COVERAGE=OFF).
#     Ver o bloco da option GLINTFX_COVERAGE acima para a racional completa (por-alvo, não
#     de diretório; por que RmlUi/_deps ficam não-instrumentados; a ressalva de demos/GLINTFX_BUILD_DEMOS).
glintfx_instrument_coverage(glintfx)

# EN: Alias so FetchContent consumers can write target_link_libraries(... glintfx::glintfx).
#     Placed here (before DEMOS/TESTS subdirs) so the alias is always available regardless
#     of whether demos or tests are enabled.
# PT: Alias para que consumidores via FetchContent escrevam target_link_libraries(... glintfx::glintfx).
#     Posicionado aqui (antes dos subdirs de DEMOS/TESTS) para que o alias esteja sempre
#     disponível independentemente de demos ou testes estarem habilitados.
add_library(glintfx::glintfx ALIAS glintfx)

# EN: Default demos and tests ON only when glintfx is the top-level project.
#     When consumed via FetchContent/add_subdirectory the default is OFF so that
#     a consumer's build is not polluted with glintfx's own demos and tests.
# PT: Demos e testes ativados por padrão apenas quando glintfx é o projeto top-level.
#     Quando consumido via FetchContent/add_subdirectory o padrão é OFF para que
#     o build do consumidor não seja poluído com os demos/testes do glintfx.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(_glintfx_top ON)
else()
  set(_glintfx_top OFF)
endif()
option(GLINTFX_BUILD_DEMOS "Build demos" ${_glintfx_top})
option(GLINTFX_BUILD_TESTS "Build tests" ${_glintfx_top})

# EN: Demos require the GLFW window backend (the showcase uses glintfx::App).
# PT: Demos exigem o backend de janela GLFW (o showcase usa glintfx::App).
if(GLINTFX_BUILD_DEMOS)
  if(GLINTFX_BACKEND_GLFW)
    if(GLINTFX_COVERAGE)
      message(WARNING
        "glintfx: GLINTFX_COVERAGE=ON with GLINTFX_BUILD_DEMOS=ON -- the showcase demo "
        "links the instrumented glintfx library but is not itself instrumented and will "
        "fail to link (undefined reference to __llvm_profile_* symbols). Configure with "
        "-DGLINTFX_BUILD_DEMOS=OFF for coverage builds (see the GLINTFX_COVERAGE option "
        "block above).")
    endif()
    add_subdirectory(demos/showcase)
  else()
    message(STATUS "glintfx: GLINTFX_BUILD_DEMOS=ON but GLINTFX_BACKEND_GLFW=OFF — demos skipped (require App/GLFW).")
  endif()
endif()
if(GLINTFX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# EN: Install rules — active only when glintfx is the top-level project (not when
#     consumed via FetchContent/add_subdirectory).
#
#     find_package(glintfx) strategy (L1.8-FINDPKG):
#       - install(EXPORT) is intentionally NOT used. CMake 4.x requires all PRIVATE
#         transitive deps of exported targets to be in the same export set; rmlui_core
#         and glloader are FetchContent/vendored targets that cannot be installed as
#         proper CMake exported targets.
#       - glloader is an INTERFACE library: its single source (gl_loader.c) is compiled
#         directly into libglintfx.a, so no separate archive is needed.
#       - rmlui_core IS a real archive (librmlui.a, 33 MB). It is co-installed under
#         <libdir>/glintfx/ (private subdir) so it does not pollute the root lib dir
#         and does not conflict with a consumer-side RmlUi installation.
#       - glintfxConfig.cmake.in creates the glintfx::glintfx IMPORTED target
#         manually, referencing the co-installed rmlui archive + system deps via
#         find_dependency(OpenGL / glfw3 / Freetype).
#       - FetchContent drop-in (add_library alias above) remains the recommended path
#         for consumers that build from source. find_package is the pre-built path.
#
# PT: Regras de install — ativas apenas quando glintfx é o projeto top-level.
#
#     Estratégia find_package(glintfx) (L1.8-FINDPKG):
#       - install(EXPORT) é intencionalmente OMITIDO. O CMake 4.x exige que todas
#         as deps PRIVATE transitivas de targets exportados estejam no mesmo export
#         set; rmlui_core e glloader são targets FetchContent/vendorizados que não
#         podem ser instalados como targets CMake exportados.
#       - glloader é uma biblioteca INTERFACE: seu único fonte (gl_loader.c) é
#         compilado diretamente em libglintfx.a, então nenhum arquivo separado é
#         necessário.
#       - rmlui_core É um arquivo real (librmlui.a, 33 MB). É co-instalado em
#         <libdir>/glintfx/ (subdiretório privado) para não poluir o diretório raiz
#         de libs e não conflitar com uma instalação de RmlUi pelo consumidor.
#       - glintfxConfig.cmake.in cria o target IMPORTED glintfx::glintfx
#         manualmente, referenciando o arquivo rmlui co-instalado + deps de sistema
#         via find_dependency(OpenGL / glfw3 / Freetype).
#       - O drop-in FetchContent (alias add_library acima) permanece o caminho
#         recomendado para consumidores que buildam do fonte. find_package é o
#         caminho para binários pré-compilados.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # EN: Main static archive.
  # PT: Arquivo estático principal.
  install(TARGETS glintfx
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  # EN: Co-install rmlui_core archive under <libdir>/glintfx/ (private subdir).
  #     $<TARGET_FILE:rmlui_core> evaluates at generate time to the build-tree path
  #     of librmlui.a.
  # PT: Co-instala o arquivo rmlui_core em <libdir>/glintfx/ (subdiretório privado).
  #     $<TARGET_FILE:rmlui_core> avalia em tempo de geração para o path de build
  #     de librmlui.a.
  install(FILES $<TARGET_FILE:rmlui_core>
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/glintfx)

  # EN: Public headers — install the source-tree include/ directory and the
  #     generated config.hpp (which lives in the build tree, not the source tree).
  # PT: Headers públicos — instala o diretório include/ da árvore de fonte e o
  #     config.hpp gerado (que fica na árvore de build, não na de fonte).
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          PATTERN "*.in" EXCLUDE)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/glintfx/config.hpp
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/glintfx)

  # EN: CMake package config — enables find_package(glintfx REQUIRED).
  #     PATH_VARS exposes PACKAGE_CMAKE_INSTALL_LIBDIR / PACKAGE_CMAKE_INSTALL_INCLUDEDIR
  #     as relocated absolute paths inside the installed Config.cmake.
  #     GLINTFX_BACKEND_GLFW is embedded in the generated config via @GLINTFX_BACKEND_GLFW@
  #     so the installed config conditionally pulls in find_dependency(glfw3).
  # PT: Config do pacote CMake — habilita find_package(glintfx REQUIRED).
  #     PATH_VARS expõe PACKAGE_CMAKE_INSTALL_LIBDIR / PACKAGE_CMAKE_INSTALL_INCLUDEDIR
  #     como paths absolutos relocados dentro do Config.cmake instalado.
  #     GLINTFX_BACKEND_GLFW é embarcado no config gerado via @GLINTFX_BACKEND_GLFW@
  #     para que o config instalado chame find_dependency(glfw3) condicionalmente.
  configure_package_config_file(
    cmake/glintfxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/glintfxConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glintfx
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/glintfxConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/glintfxConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/glintfxConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/glintfx)
endif()
