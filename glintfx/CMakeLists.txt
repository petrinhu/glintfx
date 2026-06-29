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

option(GLINTFX_BUILD_DEMOS "Build demos" ON)
option(GLINTFX_BUILD_TESTS "Build tests" ON)
if(GLINTFX_BUILD_DEMOS)
  add_subdirectory(demos/showcase)
endif()
if(GLINTFX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# EN: Install rules — archive + public headers. Only active when glintfx is the
#     top-level project (not when consumed via FetchContent/add_subdirectory).
#
#     NOTE: install(EXPORT) / glintfxConfig.cmake is a post-v1 item. CMake 4.x
#     requires all PRIVATE transitive deps of exported targets to be in an export
#     set too; since RmlUi and gl3w are not installed targets, the export would
#     fail at generate time. Consumers should use FetchContent (alias above) until
#     a proper Config.cmake with find_dependency() calls is written.
#
# PT: Regras de install — arquivo + headers públicos. Ativas apenas quando glintfx
#     é o projeto top-level (não quando consumido via FetchContent/add_subdirectory).
#
#     NOTA: install(EXPORT) / glintfxConfig.cmake é item pós-v1. O CMake 4.x
#     exige que todas as deps PRIVATE transitivas de targets exportados também
#     estejam em export sets; como RmlUi e gl3w não são targets instalados, o
#     export falharia em generate. Consumidores devem usar FetchContent (alias
#     acima) até que um Config.cmake adequado com find_dependency() seja escrito.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(GNUInstallDirs)
  install(TARGETS glintfx
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
