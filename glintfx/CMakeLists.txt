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

# gl3w (pinado) — gera o loader
FetchContent_Declare(gl3w
  GIT_REPOSITORY https://github.com/skaslev/gl3w.git
  GIT_TAG d5ba9340cdeb9154323817f5c87e5a5c377fdef7)

FetchContent_MakeAvailable(RmlUi gl3w)

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
  PRIVATE ${_rml_backends} ${gl3w_BINARY_DIR}/include)
target_compile_definitions(glintfx PRIVATE RMLUI_GL3_CUSTOM_LOADER=<GL/gl3w.h>)
target_link_libraries(glintfx PRIVATE RmlUi::Core glfw OpenGL::GL Freetype::Freetype gl3w)
target_compile_features(glintfx PUBLIC cxx_std_17)

option(GLINTFX_BUILD_DEMOS "Build demos" ON)
option(GLINTFX_BUILD_TESTS "Build tests" ON)
if(GLINTFX_BUILD_DEMOS)
  add_subdirectory(demos/showcase)
endif()
if(GLINTFX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
