# SPDX-License-Identifier: Apache-2.0
#
# EN: UIX-RCSS-LEXER -- test executables for glintfx::uix::style::Lexer. Added by
#     glintfx/src/uix/style/CMakeLists.txt's own add_subdirectory() call (see that file's header
#     comment for why this lives in its OWN directory, glintfx/tests/uix_style/, rather than the
#     DOM module's shared glintfx/tests/uix/) -- never included by glintfx/tests/CMakeLists.txt
#     (the big, RmlUi/GLFW/GL-linked suite). Every executable below links ONLY
#     glintfx::uix_style -- zero RmlUi, zero GLFW, zero GL.
# PT: UIX-RCSS-LEXER -- executáveis de teste pro glintfx::uix::style::Lexer. Somados pela própria
#     chamada add_subdirectory() do glintfx/src/uix/style/CMakeLists.txt (ver o comentário de
#     cabeçalho daquele arquivo pro porquê disto morar no PRÓPRIO diretório,
#     glintfx/tests/uix_style/, em vez do glintfx/tests/uix/ compartilhado do módulo DOM) -- nunca
#     incluído pelo glintfx/tests/CMakeLists.txt (a suíte grande, linkada a RmlUi/GLFW/GL). Todo
#     executável abaixo linka SÓ glintfx::uix_style -- zero RmlUi, zero GLFW, zero GL.
# Copyright (c) 2026 Petrus Silva Costa

add_executable(uix_style_lexer_tokens_sanity lexer_tokens_sanity.cpp)
target_link_libraries(uix_style_lexer_tokens_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_lexer_tokens_sanity COMMAND uix_style_lexer_tokens_sanity)

add_executable(uix_style_lexer_hardening_sanity lexer_hardening_sanity.cpp)
target_link_libraries(uix_style_lexer_hardening_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_lexer_hardening_sanity COMMAND uix_style_lexer_hardening_sanity)

# EN: GLINTFX_UIX_STYLE_*_DIR: absolute paths to the real corpus directories, resolved at
#     CONFIGURE time (portable regardless of ctest's CWD at RUN time -- same reasoning
#     glintfx/tests/uix/CMakeLists.txt already applies to its own GLINTFX_UIX_TESTS_DIR/
#     GLINTFX_UIX_DEMOS_DIR). GLINTFX_UIX_STYLE_UA_STYLESHEET points at the one file, not a
#     directory (its RCSS payload is a C++ raw-string literal, extracted by the test itself).
# PT: GLINTFX_UIX_STYLE_*_DIR: caminhos absolutos pros diretórios de corpus reais, resolvidos em
#     tempo de CONFIGURE (portável independente do CWD do ctest em tempo de EXECUÇÃO -- mesmo
#     raciocínio que o próprio glintfx/tests/uix/CMakeLists.txt já aplica ao próprio
#     GLINTFX_UIX_TESTS_DIR/GLINTFX_UIX_DEMOS_DIR dele). GLINTFX_UIX_STYLE_UA_STYLESHEET aponta pro
#     único arquivo, não um diretório (o payload RCSS dele é um literal de raw-string C++,
#     extraído pelo próprio teste).
add_executable(uix_style_lexer_corpus_sanity lexer_corpus_sanity.cpp)
target_link_libraries(uix_style_lexer_corpus_sanity PRIVATE glintfx::uix_style)
target_compile_definitions(uix_style_lexer_corpus_sanity PRIVATE
  GLINTFX_UIX_STYLE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_STYLE_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_STYLE_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures"
  GLINTFX_UIX_STYLE_UA_STYLESHEET="${CMAKE_CURRENT_SOURCE_DIR}/../../src/ua_stylesheet.hpp")
add_test(NAME uix_style_lexer_corpus_sanity COMMAND uix_style_lexer_corpus_sanity)

# EN: UIX-PROP-REGISTRY -- property registry table + shorthand-to-longhand expansion. Own
#     executables, same "zero RmlUi/GLFW/GL" discipline as the 3 lexer executables above.
# PT: UIX-PROP-REGISTRY -- tabela de registro de propriedades + expansão shorthand-pra-longhand.
#     Executáveis próprios, mesma disciplina "zero RmlUi/GLFW/GL" dos 3 executáveis de lexer acima.
add_executable(uix_style_property_registry_sanity property_registry_sanity.cpp)
target_link_libraries(uix_style_property_registry_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_property_registry_sanity COMMAND uix_style_property_registry_sanity)

add_executable(uix_style_shorthand_expansion_sanity shorthand_expansion_sanity.cpp)
target_link_libraries(uix_style_shorthand_expansion_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_shorthand_expansion_sanity COMMAND uix_style_shorthand_expansion_sanity)
