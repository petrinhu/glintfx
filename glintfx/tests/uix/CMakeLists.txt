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

# EN: RMLX-1/S2 -- glintfx::uix::{Node,Element,Text,Document} (glintfx/src/uix/dom/dom_tree.{hpp,
#     cpp}), same "own CMake, zero RmlUi/GLFW/GL" discipline as the S1 lexer tests above.
# PT: RMLX-1/S2 -- glintfx::uix::{Node,Element,Text,Document} (glintfx/src/uix/dom/dom_tree.{hpp,
#     cpp}), mesma disciplina "CMake próprio, zero RmlUi/GLFW/GL" dos testes de lexer da S1 acima.
add_executable(uix_dom_tree_sanity dom_tree_sanity.cpp)
target_link_libraries(uix_dom_tree_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_dom_tree_sanity COMMAND uix_dom_tree_sanity)

add_executable(uix_dom_tree_hardening_sanity dom_tree_hardening_sanity.cpp)
target_link_libraries(uix_dom_tree_hardening_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_dom_tree_hardening_sanity COMMAND uix_dom_tree_hardening_sanity)

# EN: RMLX-1/S4 -- glintfx::uix::{set_text,add_class,remove_class} (glintfx/src/uix/dom/dom_api.
#     {hpp,cpp}), same "own CMake, zero RmlUi/GLFW/GL" discipline as the S2 dom_tree tests above.
# PT: RMLX-1/S4 -- glintfx::uix::{set_text,add_class,remove_class} (glintfx/src/uix/dom/dom_api.
#     {hpp,cpp}), mesma disciplina "CMake próprio, zero RmlUi/GLFW/GL" dos testes de dom_tree da
#     S2 acima.
add_executable(uix_dom_api_sanity dom_api_sanity.cpp)
target_link_libraries(uix_dom_api_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_dom_api_sanity COMMAND uix_dom_api_sanity)

# EN: RMLX-1/S3 -- glintfx::uix::parse_document (glintfx/src/uix/dom/parser.{hpp,cpp}), same
#     "own CMake, zero RmlUi/GLFW/GL" discipline as the S1/S2/S4 tests above.
# PT: RMLX-1/S3 -- glintfx::uix::parse_document (glintfx/src/uix/dom/parser.{hpp,cpp}), mesma
#     disciplina "CMake próprio, zero RmlUi/GLFW/GL" dos testes de S1/S2/S4 acima.
add_executable(uix_parser_tokens_sanity parser_tokens_sanity.cpp)
target_link_libraries(uix_parser_tokens_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_parser_tokens_sanity COMMAND uix_parser_tokens_sanity)

add_executable(uix_parser_hardening_sanity parser_hardening_sanity.cpp)
target_link_libraries(uix_parser_hardening_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_parser_hardening_sanity COMMAND uix_parser_hardening_sanity)

# EN: GLINTFX_UIX_S3_FIXTURES_DIR: a THIRD corpus directory, this slice's own
#     (glintfx/src/uix/dom/test_fixtures/). Originally NOT one of the two
#     GLINTFX_UIX_TESTS_DIR/GLINTFX_UIX_DEMOS_DIR directories the S1 lexer_corpus_sanity target
#     below scans -- see parser_corpus_sanity.cpp's own header comment for the full history (the
#     fixtures there used to exercise a real Lexer-level gap, pinned explicitly by
#     parser_hardening_sanity.cpp). `UIX-LEXER-OPACO` (2026-08-06) fixed that gap in lexer.cpp
#     itself, so lexer_corpus_sanity's own target FURTHER DOWN this file now ALSO defines this
#     same variable and sweeps this directory -- the directory split stays (this target and
#     dumper_corpus_sanity/dumper_determinism_sanity below still need their own Rml::-linked
#     fixture discovery), it just no longer means "the standalone Lexer can't handle these".
# PT: GLINTFX_UIX_S3_FIXTURES_DIR: um TERCEIRO diretório de corpus, próprio desta fatia
#     (glintfx/src/uix/dom/test_fixtures/). Originalmente NÃO um dos dois diretórios
#     GLINTFX_UIX_TESTS_DIR/GLINTFX_UIX_DEMOS_DIR que o alvo S1 lexer_corpus_sanity abaixo varre --
#     ver o próprio comentário de cabeçalho do parser_corpus_sanity.cpp pro histórico completo (as
#     fixtures lá costumavam exercitar uma lacuna real de nível-Lexer, presa explicitamente pelo
#     parser_hardening_sanity.cpp). `UIX-LEXER-OPACO` (2026-08-06) consertou essa lacuna no próprio
#     lexer.cpp, então o alvo próprio do lexer_corpus_sanity MAIS ADIANTE neste arquivo agora
#     TAMBÉM define esta mesma variável e varre este diretório -- a divisão de diretório fica (este
#     alvo e o dumper_corpus_sanity/dumper_determinism_sanity abaixo ainda precisam da própria
#     descoberta de fixture linkada a Rml::), só não significa mais "o Lexer standalone não
#     consegue tratar estas".
add_executable(uix_parser_corpus_sanity parser_corpus_sanity.cpp)
target_link_libraries(uix_parser_corpus_sanity PRIVATE glintfx::uix_dom)
target_compile_definitions(uix_parser_corpus_sanity PRIVATE
  GLINTFX_UIX_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures")
add_test(NAME uix_parser_corpus_sanity COMMAND uix_parser_corpus_sanity)

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
# EN: `UIX-LEXER-OPACO` (2026-08-06) -- this target now ALSO defines GLINTFX_UIX_S3_FIXTURES_DIR
#     and sweeps it (see lexer_corpus_sanity.cpp's own header comment for why): the standalone
#     Lexer no longer chokes on the `<style>` stray-'<' content that directory's fixtures exercise.
# PT: `UIX-LEXER-OPACO` (2026-08-06) -- este alvo agora TAMBÉM define GLINTFX_UIX_S3_FIXTURES_DIR
#     e o varre (ver o próprio comentário de cabeçalho do lexer_corpus_sanity.cpp pro porquê): o
#     Lexer standalone não engasga mais no conteúdo `<style>` com '<' perdido que as fixtures
#     daquele diretório exercitam.
add_executable(uix_lexer_corpus_sanity lexer_corpus_sanity.cpp)
target_link_libraries(uix_lexer_corpus_sanity PRIVATE glintfx::uix_dom)
target_compile_definitions(uix_lexer_corpus_sanity PRIVATE
  GLINTFX_UIX_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures")
add_test(NAME uix_lexer_corpus_sanity COMMAND uix_lexer_corpus_sanity)

# EN: RMLX-1/S6b -- glintfx::uix::dump_document (glintfx/src/uix/dom/dumper.{hpp,cpp}), the
#     dumper that walks THIS module's own DOM tree and serializes it into docs/uix-dom.md's
#     canonical format -- one half of the S6a/S6b/S7 differential oracle (dumper.hpp's own header
#     comment). Same "own CMake, zero RmlUi/GLFW/GL" discipline as every S1-S4 test above.
#     dumper_sanity.cpp pins field-level byte-exactness (including uix-dom.md section 11's own
#     worked example); dumper_determinism_sanity.cpp proves byte-exact reproducibility is not an
#     accident of a single run; dumper_corpus_sanity.cpp proves the format comes out well-shaped
#     for the whole real corpus. The determinism/corpus suites also link parser.{hpp,cpp} (S3) to
#     get real trees from real fixtures -- still zero RmlUi/GLFW/GL, both are part of the same
#     glintfx::uix_dom standalone target.
# PT: RMLX-1/S6b -- glintfx::uix::dump_document (glintfx/src/uix/dom/dumper.{hpp,cpp}), o dumper
#     que percorre a árvore DOM PRÓPRIA deste módulo e a serializa no formato canônico do
#     docs/uix-dom.md -- uma metade do oráculo diferencial S6a/S6b/S7 (o próprio comentário de
#     cabeçalho do dumper.hpp). Mesma disciplina "CMake próprio, zero RmlUi/GLFW/GL" de todo teste
#     S1-S4 acima. O dumper_sanity.cpp fixa exatidão-de-byte em nível de campo (inclusive o
#     próprio exemplo trabalhado da seção 11 do uix-dom.md); o dumper_determinism_sanity.cpp prova
#     que a reprodutibilidade byte-exata não é acidente de uma única execução; o
#     dumper_corpus_sanity.cpp prova que o formato sai bem-formado pro corpus real inteiro. As
#     suítes de determinismo/corpus também linkam parser.{hpp,cpp} (S3) pra pegar árvores reais de
#     fixtures reais -- ainda zero RmlUi/GLFW/GL, os dois fazem parte do mesmo alvo standalone
#     glintfx::uix_dom.
add_executable(uix_dumper_sanity dumper_sanity.cpp)
target_link_libraries(uix_dumper_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_dumper_sanity COMMAND uix_dumper_sanity)

add_executable(uix_dumper_determinism_sanity dumper_determinism_sanity.cpp)
target_link_libraries(uix_dumper_determinism_sanity PRIVATE glintfx::uix_dom)
target_compile_definitions(uix_dumper_determinism_sanity PRIVATE
  GLINTFX_UIX_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures")
add_test(NAME uix_dumper_determinism_sanity COMMAND uix_dumper_determinism_sanity)

add_executable(uix_dumper_corpus_sanity dumper_corpus_sanity.cpp)
target_link_libraries(uix_dumper_corpus_sanity PRIVATE glintfx::uix_dom)
target_compile_definitions(uix_dumper_corpus_sanity PRIVATE
  GLINTFX_UIX_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
  GLINTFX_UIX_DEMOS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../demos/showcase"
  GLINTFX_UIX_S3_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../src/uix/dom/test_fixtures")
add_test(NAME uix_dumper_corpus_sanity COMMAND uix_dumper_corpus_sanity)

# EN: `UIX-CONFORMIDADE-SPEC` -- S6b's half of the THIRD oracle type (alongside, never instead of,
#     dom_dump_differential_oracle.cpp's S7 byte-diff): tests parse_document()+dump_document()
#     against docs/uix-dom.md's own text, in isolation, never comparing against S6a's output. Same
#     "own CMake, zero RmlUi/GLFW/GL" discipline as every test above.
# PT: `UIX-CONFORMIDADE-SPEC` -- a metade da S6b do TERCEIRO tipo de oráculo (ao lado, nunca no
#     lugar, do diff byte-a-byte S7 do dom_dump_differential_oracle.cpp): testa
#     parse_document()+dump_document() contra o próprio texto do docs/uix-dom.md, isoladamente,
#     nunca comparando contra a saída da S6a. Mesma disciplina "CMake próprio, zero RmlUi/GLFW/GL"
#     de todo teste acima.
add_executable(uix_spec_conformance_sanity spec_conformance_sanity.cpp)
target_link_libraries(uix_spec_conformance_sanity PRIVATE glintfx::uix_dom)
add_test(NAME uix_spec_conformance_sanity COMMAND uix_spec_conformance_sanity)
