# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)   # piso C++17; alvo 23
set(CMAKE_CXX_EXTENSIONS OFF)

# EN: GLINTFX_SANITIZE (TST-L1-MEM, TODO.md LW-TST) — AddressSanitizer + LeakSanitizer +
#     UndefinedBehaviorSanitizer for a nightly memory-safety gate. OFF by default: never
#     pollutes the normal build (dev, PR-hot CI) — the instrumented build is significantly
#     slower and is meant to run as a separate/nightly job (see .github/workflows/nightly.yml
#     and .forgejo/workflows/nightly.yml).
#     Applied via add_compile_options/add_link_options (directory-scope, set BEFORE
#     FetchContent_MakeAvailable(RmlUi) and add_subdirectory(third_party/gl3w) below) so the
#     flags reach every target compiled in this build: glintfx itself, vendored gl3w/stb_image
#     sources, the fetched RmlUi sources, and the whole tests/ suite. Prebuilt system libraries
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
#     de FetchContent_MakeAvailable(RmlUi) e add_subdirectory(third_party/gl3w) abaixo) para que
#     as flags alcancem todo target compilado neste build: a própria glintfx, os fontes
#     vendorizados gl3w/stb_image, os fontes fetchados do RmlUi, e toda a suíte tests/.
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

# EN: gl3w — vendored (pre-generated) to avoid network dependency at configure time.
#     Files generated from gl3w_gen.py + GL 4.6 spec are committed under third_party/gl3w/.
# PT: gl3w — vendorizado (pré-gerado) para evitar dependência de rede em configure.
#     Arquivos gerados com gl3w_gen.py + spec GL 4.6 estão em third_party/gl3w/.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w
                 ${CMAKE_CURRENT_BINARY_DIR}/gl3w-build)

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
  # EN: gl3w include path comes transitively from linking PRIVATE gl3w (INTERFACE target).
  # PT: Include path do gl3w vem transitivamente do link PRIVATE gl3w (alvo INTERFACE).
target_compile_definitions(glintfx PRIVATE
  RMLUI_GL3_CUSTOM_LOADER=<GL/gl3w.h>
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

# EN: Core link deps (always): RmlUi, OpenGL, FreeType, gl3w.
#     glfw is only linked when the GLFW window backend is enabled.
# PT: Deps de link principais (sempre): RmlUi, OpenGL, FreeType, gl3w.
#     glfw só é linkado quando o backend de janela GLFW está habilitado.
target_link_libraries(glintfx PRIVATE RmlUi::Core OpenGL::GL Freetype::Freetype gl3w)
if(GLINTFX_BACKEND_GLFW)
  target_link_libraries(glintfx PRIVATE glfw)
endif()
target_compile_features(glintfx PUBLIC cxx_std_17)

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
#         and gl3w are FetchContent/vendored targets that cannot be installed as proper
#         CMake exported targets.
#       - gl3w is an INTERFACE library: its single source (gl3w.c) is compiled directly
#         into libglintfx.a, so no separate archive is needed.
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
#         set; rmlui_core e gl3w são targets FetchContent/vendorizados que não podem
#         ser instalados como targets CMake exportados.
#       - gl3w é uma biblioteca INTERFACE: seu único fonte (gl3w.c) é compilado
#         diretamente em libglintfx.a, então nenhum arquivo separado é necessário.
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
