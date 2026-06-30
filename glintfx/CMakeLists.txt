# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
project(glintfx LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)   # piso C++17; alvo 23
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Deps de sistema
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)

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

add_library(glintfx STATIC
  src/app.cpp
  src/window_glfw.cpp
  src/render_gl3.cpp
  src/bootstrap.cpp
  src/engine.cpp
  src/system_clock.cpp
  ${_rml_backends}/RmlUi_Renderer_GL3.cpp
  ${_rml_backends}/RmlUi_Platform_GLFW.cpp)
target_include_directories(glintfx
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${_rml_backends})
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
target_link_libraries(glintfx PRIVATE RmlUi::Core glfw OpenGL::GL Freetype::Freetype gl3w)
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
if(GLINTFX_BUILD_DEMOS)
  add_subdirectory(demos/showcase)
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

  # EN: Public headers.
  # PT: Headers públicos.
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  # EN: CMake package config — enables find_package(glintfx REQUIRED).
  #     PATH_VARS exposes PACKAGE_CMAKE_INSTALL_LIBDIR / PACKAGE_CMAKE_INSTALL_INCLUDEDIR
  #     as relocated absolute paths inside the installed Config.cmake.
  # PT: Config do pacote CMake — habilita find_package(glintfx REQUIRED).
  #     PATH_VARS expõe PACKAGE_CMAKE_INSTALL_LIBDIR / PACKAGE_CMAKE_INSTALL_INCLUDEDIR
  #     como paths absolutos relocados dentro do Config.cmake instalado.
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
