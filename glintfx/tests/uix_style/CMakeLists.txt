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

# EN: UIX-SHEET-PARSER -- recursive-descent parser (selector structuring, declaration expansion,
#     at-rule structural records). Own executables, same "zero RmlUi/GLFW/GL" discipline as every
#     executable above.
# PT: UIX-SHEET-PARSER -- parser recursivo-descendente (estruturação de seletor, expansão de
#     declaração, registros estruturais de at-rule). Executáveis próprios, mesma disciplina "zero
#     RmlUi/GLFW/GL" de todo executável acima.
add_executable(uix_style_parser_selector_sanity parser_selector_sanity.cpp)
target_link_libraries(uix_style_parser_selector_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_parser_selector_sanity COMMAND uix_style_parser_selector_sanity)

add_executable(uix_style_parser_atrule_sanity parser_atrule_sanity.cpp)
target_link_libraries(uix_style_parser_atrule_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_parser_atrule_sanity COMMAND uix_style_parser_atrule_sanity)

add_executable(uix_style_parser_corpus_sanity parser_corpus_sanity.cpp)
target_link_libraries(uix_style_parser_corpus_sanity PRIVATE glintfx::uix_style)
target_compile_definitions(uix_style_parser_corpus_sanity PRIVATE
  GLINTFX_UIX_STYLE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_STYLE_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_STYLE_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures"
  GLINTFX_UIX_STYLE_UA_STYLESHEET="${CMAKE_CURRENT_SOURCE_DIR}/../../src/ua_stylesheet.hpp")
add_test(NAME uix_style_parser_corpus_sanity COMMAND uix_style_parser_corpus_sanity)

# EN: UIX-VALUE-COMPUTE -- pure value-computation functions (quantization, per-domain print
#     forms, composite grammars). Own executable, same "zero RmlUi/GLFW/GL" discipline as every
#     executable above.
# PT: UIX-VALUE-COMPUTE -- funções puras de computação de valor (quantização, formas de impressão
#     por-domínio, gramáticas compostas). Executável próprio, mesma disciplina "zero RmlUi/GLFW/GL"
#     de todo executável acima.
add_executable(uix_style_value_compute_sanity value_compute_sanity.cpp)
target_link_libraries(uix_style_value_compute_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_value_compute_sanity COMMAND uix_style_value_compute_sanity)

# EN: UIX-SELECTOR-MATCH -- selector matcher against glintfx's own DOM (Element/dom_tree.hpp).
#     Links glintfx::uix_style (which now also compiles ../dom/dom_tree.cpp, a new, deliberate,
#     one-directional dependency -- see selector_match.hpp's own header comment and this module's
#     own CMakeLists.txt for why).
# PT: UIX-SELECTOR-MATCH -- casador de seletor contra o DOM próprio da glintfx
#     (Element/dom_tree.hpp). Linka glintfx::uix_style (que agora também compila
#     ../dom/dom_tree.cpp, uma dependência nova, deliberada, de mão única -- ver o próprio
#     comentário de cabeçalho do selector_match.hpp e o próprio CMakeLists.txt deste módulo pro
#     porquê).
add_executable(uix_style_selector_match_sanity selector_match_sanity.cpp)
target_link_libraries(uix_style_selector_match_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_selector_match_sanity COMMAND uix_style_selector_match_sanity)

# EN: UIX-SELECTOR-MATCH -- corpus oracle: real selectors from this repo's own versioned RCSS
#     against real DOM trees parsed from those same files' own markup (glintfx::uix::parse_document,
#     the DOM module's own S3). Needs lexer.cpp/parser.cpp from ../dom/ in ADDITION to what
#     glintfx::uix_style already links (dom_tree.cpp) -- deliberately NOT dom_tree.cpp again here
#     (already compiled into glintfx_uix_style, a second copy in this executable would be a
#     duplicate-symbol link error); deliberately NOT dumper.cpp either (this test only PARSES and
#     WALKS the tree, it never dumps it).
# PT: UIX-SELECTOR-MATCH -- oráculo de corpus: seletores reais dos próprios RCSS versionados deste
#     repo contra árvores DOM reais parseadas do próprio markup desses mesmos arquivos
#     (glintfx::uix::parse_document, a própria S3 do módulo DOM). Precisa de lexer.cpp/parser.cpp
#     de ../dom/ ALÉM do que o glintfx::uix_style já linka (dom_tree.cpp) -- deliberadamente SEM
#     dom_tree.cpp de novo aqui (já compilado no glintfx_uix_style, uma segunda cópia neste
#     executável seria erro de link de símbolo duplicado); deliberadamente SEM dumper.cpp também
#     (este teste só PARSEIA e PERCORRE a árvore, nunca a dumpa).
add_executable(uix_style_selector_match_corpus_sanity
  selector_match_corpus_sanity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/lexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/parser.cpp)
target_link_libraries(uix_style_selector_match_corpus_sanity PRIVATE glintfx::uix_style)
target_compile_definitions(uix_style_selector_match_corpus_sanity PRIVATE
  GLINTFX_UIX_STYLE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_STYLE_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_STYLE_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures"
  GLINTFX_UIX_STYLE_UA_STYLESHEET="${CMAKE_CURRENT_SOURCE_DIR}/../../src/ua_stylesheet.hpp")
add_test(NAME uix_style_selector_match_corpus_sanity COMMAND uix_style_selector_match_corpus_sanity)

# EN: UIX-CASCADE -- cascade.cpp is this module's SECOND source to depend on the DOM sibling module
#     (already pulled in by selector_match.cpp above, `glintfx::uix::Element`) -- no NEW
#     cross-module dependency, cascade.hpp itself includes selector_match.hpp and dom_tree.hpp, both
#     already wired into this library. Own executables, same "zero RmlUi/GLFW/GL" discipline as
#     every executable above.
# PT: UIX-CASCADE -- o cascade.cpp é a SEGUNDA fonte deste módulo a depender do módulo irmão DOM (já
#     trazido pelo selector_match.cpp acima, `glintfx::uix::Element`) -- nenhuma dependência
#     cross-módulo NOVA, o próprio cascade.hpp inclui selector_match.hpp e dom_tree.hpp, os dois já
#     amarrados nesta biblioteca. Executáveis próprios, mesma disciplina "zero RmlUi/GLFW/GL" de todo
#     executável acima.
add_executable(uix_style_cascade_sanity cascade_sanity.cpp)
target_link_libraries(uix_style_cascade_sanity PRIVATE glintfx::uix_style)
add_test(NAME uix_style_cascade_sanity COMMAND uix_style_cascade_sanity)

add_executable(uix_style_cascade_corpus_sanity
  cascade_corpus_sanity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/lexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/parser.cpp)
target_link_libraries(uix_style_cascade_corpus_sanity PRIVATE glintfx::uix_style)
target_compile_definitions(uix_style_cascade_corpus_sanity PRIVATE
  GLINTFX_UIX_STYLE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_STYLE_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_STYLE_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures"
  GLINTFX_UIX_STYLE_UA_STYLESHEET="${CMAKE_CURRENT_SOURCE_DIR}/../../src/ua_stylesheet.hpp")
add_test(NAME uix_style_cascade_corpus_sanity COMMAND uix_style_cascade_corpus_sanity)
