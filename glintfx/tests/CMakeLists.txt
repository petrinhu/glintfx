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
