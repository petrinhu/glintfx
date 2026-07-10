# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 0.9.0)

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
FetchContent_Declare(RmlUi
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG 2cd28864ae25ed345b70598751703a5433b12356)

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
