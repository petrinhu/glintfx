# SPDX-License-Identifier: Apache-2.0
#
# EN: UIX-RCSS-LEXER -- standalone CMake entry point for glintfx's own RCSS tokenizer
#     (glintfx::uix::style::Lexer), the first brick of the style engine this repo is building to
#     eventually replace RmlUi's own stylesheet parser (docs/rmlx-subset.md, ADR-0020). Mirrors
#     glintfx/src/uix/dom/CMakeLists.txt's own shape and reasoning EXACTLY, restated briefly (see
#     that file's own header comment for the full rationale, not duplicated here per this
#     project's own "one explanation, not a paraphrase per sibling" convention):
#       - Deliberately its OWN `project()`, NOT wired into glintfx/CMakeLists.txt's build graph
#         (no `add_subdirectory(src/uix)` there) -- this module links NEITHER RmlUi, NOR GLFW, NOR
#         GL, so it configures and builds in a fraction of a second instead of paying the
#         ~200-translation-unit RmlUi FetchContent cost that graph pays unconditionally.
#       - Own test subdirectory, `glintfx/tests/uix_style/` -- deliberately NOT
#         `glintfx/tests/uix/` (the DOM module's own test directory, already wired to
#         `glintfx::uix_dom` and actively edited by sibling RMLX-1/RMLX-2 slices at the time this
#         module was written) -- a brand-new directory this module owns outright avoids editing a
#         shared CMakeLists.txt file another agent may be mid-edit on, per this task's own
#         "não toque... território de outras fatias" boundary and this project's shared-git-index
#         discipline.
#     Usage:
#       cmake -S glintfx/src/uix/style -B <build-dir> -DGLINTFX_UIX_STYLE_BUILD_TESTS=ON
#       cmake --build <build-dir> -j2
#       ctest --test-dir <build-dir> --output-on-failure
#
#     Public surface: the `glintfx::uix_style` ALIAS target, namespace `glintfx::uix::style`
#     (NOT the flat `glintfx::uix` the DOM module uses -- see lexer.hpp's own header comment for
#     why: `Lexer`/`Token`/`TokenKind` would collide with the DOM module's own identically-named
#     types the moment both modules are eventually linked into one binary, e.g. a future
#     RMLX-10/11 wave that folds both into the main `glintfx` static target).
# PT: UIX-RCSS-LEXER -- ponto de entrada CMake standalone pro tokenizador de RCSS próprio da
#     glintfx (glintfx::uix::style::Lexer), o primeiro tijolo do motor de estilo que este repo
#     está construindo pra eventualmente substituir o parser de folha de estilo do RmlUi
#     (docs/rmlx-subset.md, ADR-0020). Espelha a FORMA e o racional do próprio
#     glintfx/src/uix/dom/CMakeLists.txt EXATAMENTE, restatado brevemente (ver o próprio
#     comentário de cabeçalho daquele arquivo pro racional completo, não duplicado aqui pela
#     própria convenção deste projeto de "uma explicação, não uma paráfrase por irmão"):
#       - Deliberadamente o PRÓPRIO `project()`, NÃO amarrado no grafo de build do
#         glintfx/CMakeLists.txt (nenhum `add_subdirectory(src/uix)` lá) -- este módulo não linka
#         NEM RmlUi, NEM GLFW, NEM GL, então configura e builda numa fração de segundo em vez de
#         pagar o custo de ~200 unidades-de-tradução do FetchContent do RmlUi que aquele grafo paga
#         incondicionalmente.
#       - Subdiretório de teste próprio, `glintfx/tests/uix_style/` -- deliberadamente NÃO
#         `glintfx/tests/uix/` (o subdiretório de teste do próprio módulo DOM, já amarrado ao
#         `glintfx::uix_dom` e ativamente editado por fatias irmãs da RMLX-1/RMLX-2 no momento em
#         que este módulo foi escrito) -- um diretório novinho que este módulo possui por inteiro
#         evita editar um CMakeLists.txt compartilhado que outro agente pode estar editando no
#         meio, conforme a própria fronteira "não toque... território de outras fatias" desta
#         tarefa e a disciplina de índice-git-compartilhado deste projeto.
#     Uso:
#       cmake -S glintfx/src/uix/style -B <build-dir> -DGLINTFX_UIX_STYLE_BUILD_TESTS=ON
#       cmake --build <build-dir> -j2
#       ctest --test-dir <build-dir> --output-on-failure
#
#     Superfície pública: o alvo ALIAS `glintfx::uix_style`, namespace `glintfx::uix::style`
#     (NÃO o `glintfx::uix` plano que o módulo DOM usa -- ver o próprio comentário de cabeçalho do
#     lexer.hpp pro porquê: `Lexer`/`Token`/`TokenKind` colidiriam com os tipos de mesmo nome do
#     módulo DOM no momento em que os dois módulos forem eventualmente linkados num binário só,
#     ex. uma futura onda RMLX-10/11 que dobra os dois no alvo static `glintfx` principal).
# Copyright (c) 2026 Petrus Silva Costa

cmake_minimum_required(VERSION 3.16)
project(glintfx_uix_style LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# EN: UIX-SELECTOR-MATCH -- selector_match.cpp is this module's FIRST source to depend on the DOM
#     sibling module (glintfx::uix::Element, glintfx/src/uix/dom/dom_tree.hpp) -- a new, deliberate,
#     one-directional dependency (style -> dom, never the reverse), per selector_match.hpp's own
#     header comment. `../dom/dom_tree.cpp` is listed DIRECTLY here (not add_subdirectory(../dom),
#     for the identical "own standalone CMake project, would drag a second test tree in" reasoning
#     this file's own header comment already states for staying out of glintfx/CMakeLists.txt's
#     graph) -- mirrors the exact "list sources directly" discipline
#     glintfx/tests/CMakeLists.txt's own dom_dump_differential_oracle target already established
#     for the identical situation (one executable needing sources from two standalone modules).
#     dom_tree.cpp alone (not lexer/parser/dumper.cpp): this module only ever consumes a
#     already-built `Element` tree (via match_selector's own `const Element&` parameter), it never
#     parses RML text itself.
# PT: UIX-SELECTOR-MATCH -- o selector_match.cpp é a PRIMEIRA fonte deste módulo a depender do
#     módulo irmão DOM (glintfx::uix::Element, glintfx/src/uix/dom/dom_tree.hpp) -- uma dependência
#     nova, deliberada, de mão única (style -> dom, nunca o contrário), pelo próprio comentário de
#     cabeçalho do selector_match.hpp. `../dom/dom_tree.cpp` é listado DIRETAMENTE aqui (não
#     add_subdirectory(../dom), pelo raciocínio idêntico "próprio projeto CMake standalone,
#     arrastaria uma segunda árvore de teste" que o próprio comentário de cabeçalho deste arquivo já
#     declara pra ficar fora do grafo do glintfx/CMakeLists.txt) -- espelha exatamente a disciplina
#     "lista fontes diretamente" que o próprio alvo dom_dump_differential_oracle do
#     glintfx/tests/CMakeLists.txt já estabeleceu pra situação idêntica (um executável precisando de
#     fontes de dois módulos standalone). Só dom_tree.cpp (não lexer/parser/dumper.cpp): este módulo
#     só consome uma árvore `Element` JÁ CONSTRUÍDA (via o próprio parâmetro `const Element&` do
#     match_selector), nunca parseia texto RML ele próprio.
# EN: UIX-CASCADE -- cascade.cpp added here (2026-08-06), no new source dependency (it only
#     includes selector_match.hpp/parser.hpp/dom_tree.hpp, all three already compiled into this
#     library above).
# PT: UIX-CASCADE -- cascade.cpp somado aqui (2026-08-06), nenhuma dependência de fonte nova (só
#     inclui selector_match.hpp/parser.hpp/dom_tree.hpp, os três já compilados nesta biblioteca
#     acima).
# EN: UIX-RCSS-DUMP-B -- dumper.cpp added here (2026-08-06), no new source dependency (it only
#     includes dom_tree.hpp/cascade.hpp/parser.hpp, all already compiled into this library above).
# PT: UIX-RCSS-DUMP-B -- dumper.cpp somado aqui (2026-08-06), nenhuma dependência de fonte nova (só
#     inclui dom_tree.hpp/cascade.hpp/parser.hpp, os três já compilados nesta biblioteca acima).
add_library(glintfx_uix_style STATIC
  lexer.cpp property_registry.cpp shorthand.cpp parser.cpp value_compute.cpp selector_match.cpp
  cascade.cpp dumper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../dom/dom_tree.cpp)
target_include_directories(glintfx_uix_style PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# EN: -Wall -Wextra -Wpedantic, GCC/Clang only -- same portability guard shape as
#     glintfx/src/uix/dom/CMakeLists.txt's own (which cites glintfx/CMakeLists.txt's own MSVC
#     handling as the root precedent).
# PT: -Wall -Wextra -Wpedantic, só GCC/Clang -- mesma forma de guarda de portabilidade do próprio
#     glintfx/src/uix/dom/CMakeLists.txt (que cita o próprio glintfx/CMakeLists.txt como precedente
#     raiz pro trato de MSVC).
target_compile_options(glintfx_uix_style PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

add_library(glintfx::uix_style ALIAS glintfx_uix_style)

option(GLINTFX_UIX_STYLE_BUILD_TESTS "Build glintfx_uix_style's own standalone unit tests" ON)

if(GLINTFX_UIX_STYLE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../tests/uix_style
                    ${CMAKE_CURRENT_BINARY_DIR}/tests_uix_style)
endif()
