# SPDX-License-Identifier: MPL-2.0
# EN: Link glintfx (already contains window_glfw.cpp) — eliminates duplicate compilation.
# PT: Linka glintfx (já contém window_glfw.cpp) — elimina compilação duplicada.
add_executable(window_smoke window_smoke.cpp)
target_link_libraries(window_smoke PRIVATE glintfx)
add_test(NAME window_smoke COMMAND ${CMAKE_COMMAND}
  -DEXE=$<TARGET_FILE:window_smoke> -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake)

# EN: render_smoke — verify RenderGl3 init + single frame under a live GL context.
# PT: render_smoke — verifica init do RenderGl3 + um frame sob contexto GL ativo.
add_executable(render_smoke render_smoke.cpp)
target_link_libraries(render_smoke PRIVATE glintfx)
add_test(NAME render_smoke COMMAND ${CMAKE_COMMAND}
  -DEXE=$<TARGET_FILE:render_smoke> -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake)

# EN: bootstrap_smoke — verify RmlUi init, Rml::Context creation, and document load.
# PT: bootstrap_smoke — verifica init do RmlUi, criação de Rml::Context e carregamento de documento.
add_executable(bootstrap_smoke bootstrap_smoke.cpp)
target_link_libraries(bootstrap_smoke PRIVATE glintfx)
# EN: Copy the min.rml fixture to the test working directory (build/tests/min.rml).
#     The test binary looks for "tests/min.rml" relative to the ctest CWD (build/).
# PT: Copia a fixture min.rml para o dir de trabalho do teste (build/tests/min.rml).
#     O binário procura "tests/min.rml" relativo ao CWD do ctest (build/).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/min.rml
               ${CMAKE_CURRENT_BINARY_DIR}/min.rml COPYONLY)
# EN: WORKING_DIRECTORY ensures "tests/min.rml" resolves to build/tests/min.rml regardless
#     of where ctest is invoked from (ctest --test-dir does not set subprocess CWD).
# PT: WORKING_DIRECTORY garante que "tests/min.rml" resolva para build/tests/min.rml
#     independente de onde ctest é invocado (--test-dir não define CWD do subprocesso).
add_test(NAME bootstrap_smoke
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:bootstrap_smoke>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: app_smoke — verify the public App facade: open, load, 1 frame headless.
# PT: app_smoke — verifica a fachada pública App: abre, carrega, 1 frame headless.
add_executable(app_smoke app_smoke.cpp)
target_link_libraries(app_smoke PRIVATE glintfx)
add_test(NAME app_smoke
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:app_smoke>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: golden_test — render the showcase scene and compare pixels against a stored PPM reference.
#     First run creates the reference (exits 0 with a notice); subsequent runs compare via MSE.
# PT: golden_test — renderiza o showcase e compara pixels com referência PPM armazenada.
#     1ª execução cria a referência (sai 0 com aviso); execuções seguintes comparam via MSE.
add_executable(golden_test golden_test.cpp)
# EN: capture.hpp includes GL/gl3w.h directly — link gl3w (INTERFACE target) so its include path
#     is available here. gl3w is a PRIVATE dep of glintfx and therefore not propagated to callers.
# PT: capture.hpp inclui GL/gl3w.h diretamente — linka gl3w (target INTERFACE) para que o include
#     path esteja disponível aqui. gl3w é dep PRIVATE da glintfx e portanto não é propagado.
target_link_libraries(golden_test PRIVATE glintfx gl3w)

# EN: Copy showcase assets (RML + RCSS + font dir) to the test binary dir so the binary
#     can find them at runtime when WORKING_DIRECTORY is CMAKE_CURRENT_BINARY_DIR.
# PT: Copia assets do showcase (RML + RCSS + dir da fonte) para o dir do binário de teste
#     para que o binário os encontre em runtime com WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR.
file(COPY
  ${CMAKE_SOURCE_DIR}/demos/showcase/showcase.rml
  ${CMAKE_SOURCE_DIR}/demos/showcase/showcase.rcss
  ${CMAKE_SOURCE_DIR}/demos/showcase/assets
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# EN: Ensure the golden/ subdir exists so the first run can write the reference there.
# PT: Garante que golden/ exista para que a 1ª execução possa escrever a referência.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/golden)

# EN: If a committed reference exists in the source tree, copy it to the build dir.
# PT: Se houver referência comitada no source tree, copia para o build dir.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/golden/showcase_reference.ppm)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/golden/showcase_reference.ppm
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/golden)
endif()

add_test(NAME golden_test
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:golden_test>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
