# SPDX-License-Identifier: MPL-2.0
# consumer-example-embed — drop-in proof: consumes glintfx's EMBED contract
# (GLINTFX_BACKEND_GLFW=OFF) from outside the source tree. Only <glintfx/ui_layer.hpp> is
# referenced by main.cpp -- no direct references to GL / RmlUi (GLFW IS referenced directly,
# but by THIS project's own CMakeLists.txt below, never as a transitive dep pulled in by
# glintfx -- see main.cpp's top-of-file comment for the full rationale).
# Prova de drop-in: consome o contrato EMBED da glintfx (GLINTFX_BACKEND_GLFW=OFF) de fora da
# árvore de fonte. main.cpp só referencia <glintfx/ui_layer.hpp> -- sem referências diretas a
# GL / RmlUi (GLFW É referenciado diretamente, mas pelo CMakeLists.txt deste PRÓPRIO projeto
# abaixo, nunca como dep transitiva puxada pela glintfx -- ver o comentário no topo de
# main.cpp para a racional completa).
# Copyright (c) 2026 Petrus Silva Costa
cmake_minimum_required(VERSION 3.16)
project(consumer_embed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# EN: TWO ways to obtain glintfx -- both intentionally supported by this ONE CMakeLists.txt,
#     tried in this order:
#       1. find_package(glintfx CONFIG) -- a PRE-BUILT, installed tree (point
#          CMAKE_PREFIX_PATH at the install prefix). This is the path TST-L1-CONTRACT's
#          `contract_consume` ctest (glintfx/tests/CMakeLists.txt +
#          glintfx/tests/contract_consume.cmake) exercises: it installs glintfx ONCE
#          (cmake --install) into a throwaway staging prefix and configures THIS project
#          against that prefix, so building this consumer does NOT re-build RmlUi (anti-OOM
#          -- see contract_consume.cmake's own top-of-file comment).
#       2. FetchContent from the sibling glintfx/ source tree -- the fallback used for a
#          standalone developer build (`cmake -S consumer-example-embed -B build`, no
#          CMAKE_PREFIX_PATH set), mirroring ../consumer-example/CMakeLists.txt's own
#          FetchContent pattern. Builds glintfx (embed-only) from source.
#     GLINTFX_BACKEND_GLFW=OFF applies to path 2 only (path 1 reads whatever the staged
#     install was built with -- see glintfxConfig.cmake.in's @GLINTFX_BACKEND_GLFW@
#     substitution). Either way, this consumer proves the EMBED contract: the library must
#     compile with no App/GLFW inside it, which main.cpp's <glintfx/ui_layer.hpp>-only
#     #include enforces at compile time regardless of which path supplied the library.
# PT: DUAS formas de obter a glintfx -- ambas suportadas de propósito por este ÚNICO
#     CMakeLists.txt, tentadas nesta ordem:
#       1. find_package(glintfx CONFIG) -- uma árvore PRÉ-BUILDADA e instalada (aponte
#          CMAKE_PREFIX_PATH pro prefixo de instalação). É o caminho que o ctest
#          `contract_consume` de TST-L1-CONTRACT (glintfx/tests/CMakeLists.txt +
#          glintfx/tests/contract_consume.cmake) exercita: instala a glintfx UMA VEZ
#          (cmake --install) num prefixo de staging descartável e configura ESTE projeto
#          contra esse prefixo, então buildar este consumidor NÃO re-builda o RmlUi
#          (anti-OOM -- ver o comentário no topo do próprio contract_consume.cmake).
#       2. FetchContent do diretório-irmão glintfx/ -- o fallback usado para um build
#          standalone de desenvolvedor (`cmake -S consumer-example-embed -B build`, sem
#          CMAKE_PREFIX_PATH definido), espelhando o próprio padrão FetchContent de
#          ../consumer-example/CMakeLists.txt. Builda a glintfx (embed-only) do fonte.
#     GLINTFX_BACKEND_GLFW=OFF se aplica só ao caminho 2 (o caminho 1 lê o que quer que o
#     install staged tenha sido buildado -- ver a substituição @GLINTFX_BACKEND_GLFW@ de
#     glintfxConfig.cmake.in). De qualquer forma, este consumidor prova o contrato EMBED: a
#     lib precisa compilar sem App/GLFW dentro dela, o que o #include só-de
#     <glintfx/ui_layer.hpp> em main.cpp garante em tempo de compilação independentemente de
#     qual caminho supriu a lib.
find_package(glintfx CONFIG QUIET)
if(NOT glintfx_FOUND)
  message(STATUS "consumer-example-embed: glintfx not found via find_package -- falling back to FetchContent from ../glintfx")
  set(GLINTFX_BACKEND_GLFW OFF CACHE BOOL "" FORCE)
  set(GLINTFX_BUILD_DEMOS  OFF CACHE BOOL "" FORCE)
  set(GLINTFX_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
  include(FetchContent)
  FetchContent_Declare(glintfx SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../glintfx)
  FetchContent_MakeAvailable(glintfx)
else()
  message(STATUS "consumer-example-embed: glintfx found via find_package (installed tree)")
endif()

# EN: GLFW is THIS consumer's own choice of GL-context provider -- see main.cpp's top-of-file
#     comment. Not required by glintfx::glintfx itself in embed-only mode (glintfxConfig.cmake
#     only calls find_dependency(glfw3) when the staged install was built with
#     GLINTFX_BACKEND_GLFW=ON -- see glintfxConfig.cmake.in).
# PT: GLFW é a escolha PRÓPRIA deste consumidor de provedor de contexto GL -- ver o comentário
#     no topo de main.cpp. Não é exigido pela glintfx::glintfx em si em modo embed-only
#     (glintfxConfig.cmake só chama find_dependency(glfw3) quando o install staged foi
#     buildado com GLINTFX_BACKEND_GLFW=ON -- ver glintfxConfig.cmake.in).
find_package(glfw3 REQUIRED)

add_executable(consumer_embed main.cpp)
target_link_libraries(consumer_embed PRIVATE glintfx::glintfx glfw)

# EN: Scene assets read by main.cpp's ui.load("contract_scene.rml") at runtime (relative to
#     the process CWD). The contract_consume ctest driver runs the binary from
#     CMAKE_CURRENT_BINARY_DIR (see contract_consume.cmake's execute_process WORKING_DIRECTORY)
#     -- consumer_embed's default RUNTIME_OUTPUT_DIRECTORY, since no override is set below --
#     so a plain configure_file() to CMAKE_CURRENT_BINARY_DIR (same pattern as every scene
#     asset in glintfx/tests/CMakeLists.txt) lands the two files next to the binary for both a
#     standalone developer build (path 2 above) and the ctest-driven install-tree build
#     (path 1). NOTE: configure_file()'s DESTINATION does NOT support generator expressions
#     (it runs at configure time, before the generate step) -- CMAKE_CURRENT_BINARY_DIR is the
#     correct plain-variable target here, not $<TARGET_FILE_DIR:...>.
# PT: Assets de cena lidos por ui.load("contract_scene.rml") de main.cpp em runtime
#     (relativos ao CWD do processo). O driver ctest de contract_consume roda o binário a
#     partir de CMAKE_CURRENT_BINARY_DIR (ver o WORKING_DIRECTORY do execute_process em
#     contract_consume.cmake) -- o RUNTIME_OUTPUT_DIRECTORY default de consumer_embed, já que
#     nenhum override é definido abaixo -- então um configure_file() simples para
#     CMAKE_CURRENT_BINARY_DIR (mesmo padrão de todo asset de cena em
#     glintfx/tests/CMakeLists.txt) coloca os dois arquivos junto do binário tanto para um
#     build standalone de desenvolvedor (caminho 2 acima) quanto para o build da árvore
#     instalada dirigido por ctest (caminho 1). NOTA: o DESTINATION de configure_file() NÃO
#     suporta generator expressions (roda em configure time, antes do generate step) --
#     CMAKE_CURRENT_BINARY_DIR é o alvo de variável simples correto aqui, não
#     $<TARGET_FILE_DIR:...>.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contract_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/contract_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/contract_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/contract_scene.rcss COPYONLY)
