# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 0.9.1)

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

# EN: Core sources — always compiled regardless of GLFW backend option.
#     Engine + UiLayer + RenderGl3 (plus its GL3 backend) + Bootstrap + SystemClock + version().
#     version.cpp (L1.9-VERSEMBED) must stay here (not in the GLFW-only block below) so
#     glintfx::version() is available in embed-only builds (GLINTFX_BACKEND_GLFW=OFF).
# PT: Sources principais — sempre compilados independente da opção do backend GLFW.
#     Engine + UiLayer + RenderGl3 (mais o backend GL3 dele) + Bootstrap + SystemClock + version().
#     version.cpp (L1.9-VERSEMBED) deve ficar aqui (não no bloco GLFW-only abaixo) para que
#     glintfx::version() esteja disponível em builds embed-only (GLINTFX_BACKEND_GLFW=OFF).
set(_glintfx_sources
  src/render_gl3.cpp
  src/stb_image_impl.cpp
  src/bootstrap.cpp
  src/data_binder.cpp
  src/decorator_polygon.cpp
  src/decorator_image_tint.cpp
  src/engine.cpp
  src/system_clock.cpp
  src/ui_layer.cpp
  src/version.cpp
  ${_rml_backends}/RmlUi_Renderer_GL3.cpp)

if(GLINTFX_BACKEND_GLFW)
  # EN: GLFW-backend sources — standalone App + GLFW window + RmlUi GLFW platform adapter.
  #     Only compiled when GLINTFX_BACKEND_GLFW=ON.
  # PT: Sources do backend GLFW — App standalone + janela GLFW + adapter de plataforma GLFW do RmlUi.
  #     Só compilados com GLINTFX_BACKEND_GLFW=ON.
  list(APPEND _glintfx_sources
    src/app.cpp
    src/window_glfw.cpp
    ${_rml_backends}/RmlUi_Platform_GLFW.cpp)
endif()

# EN: L1.19-FONTENG (FT-F3, L1.20-FONTFLIP Phase 1) — the "hosted shell"
#     (src/font_engine_own.cpp) plus Layer 0's sovereign C "meat", read from the vendored
#     copy (${_glx_core_src_dir}/sfnt.c, raster.c, hint.c -- glintfx/vendor/core/src/, NOT
#     ../src), compiled directly as glintfx sources per ADR-0009's "hosted shell + C meat"
#     pattern -- always core (not GLFW-only), so the own font engine is available in
#     embed-only builds too. Only added when GLINTFX_OWN_FONT_ENGINE=ON (default ON since
#     L1.20-FONTFLIP Phase 1; the vendored sources are always present, no existence guard
#     needed).
# PT: L1.19-FONTENG (FT-F3, L1.20-FONTFLIP Fase 1) — a "casca hosted"
#     (src/font_engine_own.cpp) mais a "carne" C soberana da Camada 0, lida da cópia
#     vendorizada (${_glx_core_src_dir}/sfnt.c, raster.c, hint.c -- glintfx/vendor/core/src/,
#     NÃO ../src), compiladas diretamente como sources do glintfx conforme o padrão
#     "casca-hosted + carne-C" da ADR-0009 -- sempre core (não só-GLFW), para que o motor de
#     fonte próprio esteja disponível também em builds embed-only. Só adicionado quando
#     GLINTFX_OWN_FONT_ENGINE=ON (padrão ON desde o L1.20-FONTFLIP Fase 1; os sources
#     vendorizados estão sempre presentes, nenhum guard de existência é necessário).
if(GLINTFX_OWN_FONT_ENGINE)
  list(APPEND _glintfx_sources
    src/font_engine_own.cpp
    ${_glx_core_src_dir}/sfnt.c
    ${_glx_core_src_dir}/raster.c
    ${_glx_core_src_dir}/hint.c)  # EN/PT: SOV-HINT (L1.20-FONTFLIP FT-F4) Y grid-fitter, wired in sub-phase 1.4.
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
