# SPDX-License-Identifier: Apache-2.0
#
# EN: RMLX-1/S1 -- test executables for glintfx::uix::Lexer. Added by
#     glintfx/src/uix/dom/CMakeLists.txt's own add_subdirectory() call (its own comment explains
#     why this lives HERE, glintfx/tests/uix/, and not next to the module) -- never included
#     directly by glintfx/tests/CMakeLists.txt (the big, RmlUi/GLFW/GL-linked suite). Every
#     executable below links ONLY glintfx::uix_dom -- zero RmlUi, zero GLFW, zero GL, per this
#     slice's own brief.
# PT: RMLX-1/S1 -- executáveis de teste pro glintfx::uix::Lexer. Somados pela própria chamada
#     add_subdirectory() do glintfx/src/uix/dom/CMakeLists.txt (o próprio comentário dele explica
#     o porquê disto morar AQUI, glintfx/tests/uix/, e não junto do módulo) -- nunca incluído
#     direto pelo glintfx/tests/CMakeLists.txt (a suíte grande, linkada a RmlUi/GLFW/GL). Todo
#     executável abaixo linka SÓ glintfx::uix_dom -- zero RmlUi, zero GLFW, zero GL, conforme o
#     próprio briefing desta fatia.
# Copyright (c) 2026 Petrus Silva Costa

add_executable(uix_lexer_tokens_sanity lexer_tokens_sanity.cpp)
target_link_libraries(uix_lexer_tokens_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_lexer_tokens_sanity COMMAND uix_lexer_tokens_sanity)

add_executable(uix_lexer_hardening_sanity lexer_hardening_sanity.cpp)
target_link_libraries(uix_lexer_hardening_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_lexer_hardening_sanity COMMAND uix_lexer_hardening_sanity)

# EN: GLINTFX_UIX_TESTS_DIR/GLINTFX_UIX_DEMOS_DIR: absolute paths to the two real corpus
#     directories, resolved at CONFIGURE time (portable regardless of ctest's CWD at RUN time,
#     same reasoning the rest of this repo's tests already apply to WORKING_DIRECTORY handling).
#     GLINTFX_UIX_DEMOS_DIR points at demos/showcase specifically (the only .rml-bearing demo
#     directory today; a future demo directory with its own .rml would need its own define here,
#     not a silent glob-widening of this one).
# PT: GLINTFX_UIX_TESTS_DIR/GLINTFX_UIX_DEMOS_DIR: caminhos absolutos pros dois diretórios de
#     corpus reais, resolvidos em tempo de CONFIGURE (portável independente do CWD do ctest em
#     tempo de EXECUÇÃO, mesmo raciocínio que o resto dos testes deste repo já aplica ao trato de
#     WORKING_DIRECTORY). GLINTFX_UIX_DEMOS_DIR aponta pro demos/showcase especificamente (o único
#     diretório de demo com `.rml` hoje; um futuro diretório de demo com `.rml` próprio precisaria
#     de seu próprio define aqui, não um alargamento silencioso de glob deste).
add_executable(uix_lexer_corpus_sanity lexer_corpus_sanity.cpp)
target_link_libraries(uix_lexer_corpus_sanity PRIVATE glintfx::uix_dom)
target_compile_definitions(uix_lexer_corpus_sanity PRIVATE
  GLINTFX_UIX_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase")
add_test(NAME uix_lexer_corpus_sanity COMMAND uix_lexer_corpus_sanity)
