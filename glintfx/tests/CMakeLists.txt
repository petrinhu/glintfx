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

# EN: engine_smoke — verify Engine (RmlUi+GL3) init via SystemClock + document load.
#     Replaces bootstrap_smoke: Bootstrap::init no longer accepts WindowGlfw (T1 refactor).
# PT: engine_smoke — verifica init do Engine (RmlUi+GL3) via SystemClock + carregamento de documento.
#     Substitui bootstrap_smoke: Bootstrap::init não aceita mais WindowGlfw (refator T1).
add_executable(engine_smoke engine_smoke.cpp)
# EN: system_clock.hpp inherits from Rml::SystemInterface — needs RmlUi include path.
#     RmlUi::Core is a PRIVATE dep of glintfx and not propagated; link explicitly here.
# PT: system_clock.hpp herda de Rml::SystemInterface — precisa do include path do RmlUi.
#     RmlUi::Core é dep PRIVATE da glintfx e não propagada; linka explicitamente aqui.
target_link_libraries(engine_smoke PRIVATE glintfx RmlUi::Core)
# EN: Copy the min.rml fixture to build/tests/ so the binary finds "tests/min.rml"
#     relative to the ctest CWD (build/).
# PT: Copia a fixture min.rml para build/tests/ para que o binário encontre "tests/min.rml"
#     relativo ao CWD do ctest (build/).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/min.rml
               ${CMAKE_CURRENT_BINARY_DIR}/min.rml COPYONLY)
# EN: WORKING_DIRECTORY ensures "tests/min.rml" resolves to build/tests/min.rml regardless
#     of where ctest is invoked from (ctest --test-dir does not set subprocess CWD).
# PT: WORKING_DIRECTORY garante que "tests/min.rml" resolva para build/tests/min.rml
#     independente de onde ctest é invocado (--test-dir não define CWD do subprocesso).
add_test(NAME engine_smoke
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:engine_smoke>
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

# EN: Shared assets for visual tests (render_sanity and golden_test).
#     Copy showcase RML variants, RCSS, font, and test RML variant so both tests find
#     them at runtime from WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/).
#     showcase_test.rml is the CI/headless variant: same layout as showcase.rml but
#     without the mask card (Mesa llvmpipe BlendMask dual-sampler crash, Mesa SW bug).
# PT: Assets compartilhados para testes visuais (render_sanity e golden_test).
#     Copia variantes RML do showcase, RCSS, fonte e variante RML de teste para que ambos
#     os testes as encontrem em runtime com WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/).
#     showcase_test.rml é a variante CI/headless: mesmo layout que showcase.rml mas sem o
#     card mask (crash do Mesa llvmpipe BlendMask dual-sampler, bug do Mesa SW).
file(COPY
  ${CMAKE_SOURCE_DIR}/demos/showcase/showcase.rml
  ${CMAKE_SOURCE_DIR}/demos/showcase/showcase_test.rml
  ${CMAKE_SOURCE_DIR}/demos/showcase/showcase.rcss
  ${CMAKE_SOURCE_DIR}/demos/showcase/assets
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# EN: render_sanity — structural pixel-statistics check: not black, has dark background,
#     has colourful/bright content. Tolerant of per-pixel variation from llvmpipe's threaded
#     tile rendering (blur/glow convolution order non-determinism). Always active in CI.
# PT: render_sanity — verificação estrutural de estatísticas de pixels: não preto, tem fundo
#     escuro, tem conteúdo colorido/brilhante. Tolerante a variação per-pixel do rendering de
#     tiles em threads do llvmpipe (não-determinismo na ordem de convolução de blur/glow).
#     Sempre ativo no CI.
add_executable(render_sanity render_sanity.cpp)
target_link_libraries(render_sanity PRIVATE glintfx)
add_test(NAME render_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:render_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# EN: golden_test — pixel-exact MSE comparison against a stored PPM reference.
#     FLAKY under Mesa llvmpipe / Xvfb: llvmpipe renders blur/glow convolution in parallel
#     tiles; floating-point non-associativity across thread schedules yields MSE ~3 000
#     between runs of the same scene (tolerance was 50). Root causes: threaded tile rendering
#     and floating-point accumulation order in blur/drop-shadow shader passes.
#     Gated behind GLINTFX_GOLDEN_TEST (default OFF) so it does not pollute the default CI
#     gate. Enable with -DGLINTFX_GOLDEN_TEST=ON when running on real GPU hardware alongside
#     glintfx_capture for reference generation.
# PT: golden_test — comparação MSE pixel-exata com referência PPM armazenada.
#     FLAKY sob Mesa llvmpipe / Xvfb: o llvmpipe renderiza convolução de blur/glow em tiles
#     paralelos; não-associatividade de ponto flutuante entre agendamentos de threads gera
#     MSE ~3 000 entre execuções da mesma cena (tolerância era 50). Causas raiz: rendering de
#     tiles em threads e ordem de acumulação de ponto flutuante nas passagens de shader de
#     blur/drop-shadow.
#     Bloqueado por GLINTFX_GOLDEN_TEST (default OFF) para não poluir o gate padrão de CI.
#     Ativar com -DGLINTFX_GOLDEN_TEST=ON ao rodar em GPU real com glintfx_capture para
#     geração de referência.
option(GLINTFX_GOLDEN_TEST
  "Enable pixel-exact golden-image test (requires real GPU or stable software renderer)"
  OFF)

add_executable(golden_test golden_test.cpp)
# EN: capture.hpp includes GL/gl3w.h directly — link gl3w (INTERFACE target) so its include path
#     is available here. gl3w is a PRIVATE dep of glintfx and therefore not propagated to callers.
# PT: capture.hpp inclui GL/gl3w.h diretamente — linka gl3w (target INTERFACE) para que o include
#     path esteja disponível aqui. gl3w é dep PRIVATE da glintfx e portanto não é propagado.
target_link_libraries(golden_test PRIVATE glintfx gl3w)

# EN: Ensure the golden/ subdir exists so the first run can write the reference there.
# PT: Garante que golden/ exista para que a 1ª execução possa escrever a referência.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/golden)

# EN: If a committed reference exists in the source tree, copy it to the build dir.
# PT: Se houver referência comitada no source tree, copia para o build dir.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/golden/showcase_reference.ppm)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/golden/showcase_reference.ppm
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/golden)
endif()

if(GLINTFX_GOLDEN_TEST)
  add_test(NAME golden_test
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:golden_test>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
