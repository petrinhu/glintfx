# SPDX-License-Identifier: Apache-2.0
#
# EN: RMLX-1/S1 -- standalone CMake entry point for glintfx's own RML lexer
#     (glintfx::uix::Lexer), the first brick of the DOM this arc (RMLX-1..11, docs/rmlx-subset.md,
#     ADR-0020) is building to eventually replace RmlUi. Deliberately its OWN `project()`,
#     NOT wired into glintfx/CMakeLists.txt's build graph (no `add_subdirectory(src/uix)` there,
#     no entry in glintfx/tests/CMakeLists.txt) -- this is the slice's own explicit brief, not an
#     oversight:
#       - glintfx/CMakeLists.txt's FetchContent_MakeAvailable(RmlUi) runs UNCONDITIONALLY at
#         configure time (line ~755, before `add_subdirectory(tests)`), so ANY target added to
#         that graph pays the ~200-translation-unit RmlUi build cost on a fresh configure,
#         regardless of whether that target itself links RmlUi. This module links NEITHER RmlUi,
#         NOR GLFW, NOR GL (this file's own add_library() call below has zero find_package()
#         result on its target_link_libraries) -- so it configures and builds in seconds, and
#         staying standalone is how it KEEPS that property instead of inheriting the big graph's
#         cost by association.
#       - This is also why `.claude/tdd-guard.json`'s `fast_command`/`test_command` build/test
#         THIS directory as a SEPARATE chained step (own build dir, /var/tmp/glintfx-uix-dom-tdd-
#         build) rather than folding it into the existing `cmake --build glintfx/build` step --
#         see that file's own `_comment_9` for the full rationale.
#     Usage (also the exact invocation the TDD guard's own fast_command uses):
#       cmake -S glintfx/src/uix/dom -B <build-dir> -DGLINTFX_UIX_DOM_BUILD_TESTS=ON
#       cmake --build <build-dir> -j
#       ctest --test-dir <build-dir> --output-on-failure
#
#     Public surface: the `glintfx::uix_dom` ALIAS target. `target_include_directories(...
#     PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)` exposes `glintfx/src/` as the include root, so a
#     consumer (this module's own tests today; a future glintfx/src/engine.cpp or glintfx/src/
#     uix/dom_tree.cpp once S2/S3 exist) writes `#include "uix/dom/lexer.hpp"` -- the SAME quoted,
#     flat-relative-to-glintfx/src convention every other internal (non-public-API) glintfx module
#     already uses (e.g. glintfx/src/engine.cpp's own `#include "rml/bootstrap.hpp"`). This is
#     deliberate: when a LATER wave (RMLX-10/11) wires this module into the main `glintfx`
#     STATIC target for real, the #include lines in this module and in ITS consumers need ZERO
#     changes -- only the CMake wiring (an `add_subdirectory`/direct source-list entry replacing
#     this file's own `project()` call) changes.
# PT: RMLX-1/S1 -- ponto de entrada CMake standalone pro lexer de RML próprio da glintfx
#     (glintfx::uix::Lexer), o primeiro tijolo do DOM que este arco (RMLX-1..11,
#     docs/rmlx-subset.md, ADR-0020) está construindo pra eventualmente substituir o RmlUi.
#     Deliberadamente o PRÓPRIO `project()`, NÃO amarrado no grafo de build do
#     glintfx/CMakeLists.txt (nenhum `add_subdirectory(src/uix)` lá, nenhuma entrada no
#     glintfx/tests/CMakeLists.txt) -- isto é o próprio briefing explícito desta fatia, não um
#     descuido:
#       - O FetchContent_MakeAvailable(RmlUi) do glintfx/CMakeLists.txt roda INCONDICIONALMENTE em
#         tempo de configure (linha ~755, antes do `add_subdirectory(tests)`), então QUALQUER alvo
#         somado àquele grafo paga o custo de build de ~200 unidades-de-tradução do RmlUi numa
#         configuração nova, independente de aquele alvo em si linkar RmlUi. Este módulo não linka
#         NEM RmlUi, NEM GLFW, NEM GL (a própria chamada add_library() deste arquivo abaixo não tem
#         resultado de find_package() nenhum no target_link_libraries) -- então configura e builda
#         em segundos, e ficar standalone é como ele MANTÉM essa propriedade em vez de herdar o
#         custo do grafo grande por associação.
#       - É também por isso que o `fast_command`/`test_command` do `.claude/tdd-guard.json`
#         buildam/testam ESTE diretório como um passo ENCADEADO SEPARADO (build dir próprio,
#         /var/tmp/glintfx-uix-dom-tdd-build) em vez de dobrar no passo `cmake --build
#         glintfx/build` já existente -- ver o próprio `_comment_9` daquele arquivo pro racional
#         completo.
#     Uso (também a invocação exata que o próprio fast_command do TDD guard usa):
#       cmake -S glintfx/src/uix/dom -B <build-dir> -DGLINTFX_UIX_DOM_BUILD_TESTS=ON
#       cmake --build <build-dir> -j
#       ctest --test-dir <build-dir> --output-on-failure
#
#     Superfície pública: o alvo ALIAS `glintfx::uix_dom`. `target_include_directories(...
#     PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)` expõe `glintfx/src/` como raiz de include, então
#     um consumidor (os próprios testes deste módulo hoje; um futuro glintfx/src/engine.cpp ou
#     glintfx/src/uix/dom_tree.cpp quando S2/S3 existirem) escreve `#include "uix/dom/lexer.hpp"`
#     -- a MESMA convenção entre-aspas, plana-relativa-a-glintfx/src que todo outro módulo interno
#     (não-API-pública) da glintfx já usa (ex.: o próprio `#include "rml/bootstrap.hpp"` do
#     glintfx/src/engine.cpp). Isto é deliberado: quando uma onda POSTERIOR (RMLX-10/11) amarrar
#     este módulo no alvo STATIC `glintfx` principal de verdade, as linhas #include neste módulo e
#     nos SEUS consumidores precisam de ZERO mudança -- só a fiação CMake muda (um
#     `add_subdirectory`/entrada direta de lista-de-fonte substituindo a própria chamada
#     `project()` deste arquivo).
# Copyright (c) 2026 Petrus Silva Costa

cmake_minimum_required(VERSION 3.16)
project(glintfx_uix_dom LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_library(glintfx_uix_dom STATIC lexer.cpp dom_tree.cpp dom_api.cpp parser.cpp dumper.cpp)
target_include_directories(glintfx_uix_dom PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# EN: -Wall -Wextra -Wpedantic, GCC/Clang only -- same portability guard pattern as
#     glintfx/CMakeLists.txt's own MSVC handling (see glintfx/include/glintfx/log.hpp's
#     GLINTFX_PRINTF_LIKE macro comment for the sibling precedent of this exact guard shape).
# PT: -Wall -Wextra -Wpedantic, só GCC/Clang -- mesmo padrão de guarda de portabilidade do
#     próprio glintfx/CMakeLists.txt pro MSVC (ver o comentário da macro GLINTFX_PRINTF_LIKE do
#     glintfx/include/glintfx/log.hpp pro precedente irmão desta mesma forma de guarda).
target_compile_options(glintfx_uix_dom PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

add_library(glintfx::uix_dom ALIAS glintfx_uix_dom)

# EN: Default ON: this standalone project's ONLY reason to be configured directly (as opposed to
#     a future add_subdirectory() from the main glintfx/CMakeLists.txt) is to build and run its
#     own tests -- see this file's header comment.
# PT: Default ON: a ÚNICA razão deste projeto standalone ser configurado diretamente (em oposição
#     a um futuro add_subdirectory() a partir do glintfx/CMakeLists.txt principal) é buildar e
#     rodar os próprios testes -- ver o comentário de cabeçalho deste arquivo.
option(GLINTFX_UIX_DOM_BUILD_TESTS "Build glintfx_uix_dom's own standalone unit tests" ON)

if(GLINTFX_UIX_DOM_BUILD_TESTS)
  enable_testing()
  # EN: Tests live at glintfx/tests/uix/ (this repo's own test-subdirectory convention -- see
  #     glintfx/tests/CMakeLists.txt for the SAME "tests mirror the module's own scene/fixture
  #     naming" pattern applied project-wide), NOT next to this CMakeLists.txt -- a source
  #     directory outside this project's own tree needs an explicit binary-dir argument (2nd
  #     add_subdirectory() argument) since CMake cannot derive one automatically for a path that
  #     is not a descendant of CMAKE_CURRENT_SOURCE_DIR.
  # PT: Testes moram em glintfx/tests/uix/ (a própria convenção de subdiretório-de-teste deste
  #     repo -- ver glintfx/tests/CMakeLists.txt pro MESMO padrão "testes espelham o nome de
  #     cena/fixture do próprio módulo" aplicado no projeto inteiro), NÃO junto deste
  #     CMakeLists.txt -- um diretório-fonte fora da própria árvore deste projeto precisa de um
  #     argumento explícito de binary-dir (2º argumento do add_subdirectory()) já que o CMake não
  #     consegue derivar um automaticamente pra um caminho que não é descendente do
  #     CMAKE_CURRENT_SOURCE_DIR.
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../tests/uix
                    ${CMAKE_CURRENT_BINARY_DIR}/tests_uix)
endif()
