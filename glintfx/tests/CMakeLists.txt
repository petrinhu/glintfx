# SPDX-License-Identifier: MPL-2.0

# EN: Warn on incompatible flag combination: GLINTFX_GOLDEN_TEST=ON requires App::snapshot()
#     which is compiled only with the GLFW backend (GLINTFX_BACKEND_GLFW=ON). When the
#     user sets both GLINTFX_GOLDEN_TEST=ON and GLINTFX_BACKEND_GLFW=OFF the golden test
#     is silently skipped because it lives inside if(GLINTFX_BACKEND_GLFW). Emit a warning
#     so the combination is never accepted without feedback.
# PT: Avisa sobre combinacao de flags incompativel: GLINTFX_GOLDEN_TEST=ON requer App::snapshot()
#     que so e compilado com o backend GLFW (GLINTFX_BACKEND_GLFW=ON). Quando o usuario
#     define GLINTFX_GOLDEN_TEST=ON com GLINTFX_BACKEND_GLFW=OFF o golden test e ignorado
#     silenciosamente por estar dentro de if(GLINTFX_BACKEND_GLFW). Emite aviso para que a
#     combinacao nunca seja aceita sem feedback.
if(GLINTFX_GOLDEN_TEST AND NOT GLINTFX_BACKEND_GLFW)
  message(WARNING
    "glintfx: GLINTFX_GOLDEN_TEST=ON has no effect when GLINTFX_BACKEND_GLFW=OFF -- "
    "the golden test depends on App::snapshot() which requires the GLFW backend. "
    "The golden_test target will not be registered in this build.")
endif()

# EN: min.rml is required by engine_smoke (GLFW-backend only) AND by the embed tests
#     (ui_layer_attach, ui_layer_events) which run in both build configs. Copy it
#     unconditionally so it is always present in the build/tests/ directory.
# PT: min.rml é necessário pelo engine_smoke (só backend GLFW) E pelos testes embed
#     (ui_layer_attach, ui_layer_events) que rodam em ambas as configs. Copiar
#     incondicionalmente para que esteja sempre em build/tests/.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/min.rml
               ${CMAKE_CURRENT_BINARY_DIR}/min.rml COPYONLY)

# ---------------------------------------------------------------------------
# EN: GLFW-backend-only tests: window_smoke, render_smoke, engine_smoke, app_smoke,
#     render_sanity. These depend on glintfx::App / glintfx::WindowGlfw which are
#     only compiled when GLINTFX_BACKEND_GLFW=ON.
# PT: Testes exclusivos do backend GLFW: window_smoke, render_smoke, engine_smoke,
#     app_smoke, render_sanity. Dependem de glintfx::App / glintfx::WindowGlfw que
#     só são compilados com GLINTFX_BACKEND_GLFW=ON.
# ---------------------------------------------------------------------------
if(GLINTFX_BACKEND_GLFW)

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

  # ---------------------------------------------------------------------------
  # EN: app_dp_ratio_smoke — smoke test for App::set_dp_ratio(), App::set_asset_base_url(),
  #     and AppConfig::dp_ratio (v0.2.2 additions). No pixel comparison; proves the App
  #     API surface is wired and ok() stays true after the setter calls.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/min.rml" resolves to build/tests/min.rml
  #     (min.rml is copied unconditionally at the top of this file).
  # PT: app_dp_ratio_smoke — teste de fumaça para App::set_dp_ratio(), App::set_asset_base_url(),
  #     e AppConfig::dp_ratio (adicoes da v0.2.2). Sem comparação de pixels; prova que a
  #     superficie de API do App está conectada e ok() permanece true após as chamadas de setter.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/min.rml" resolva para build/tests/min.rml
  #     (min.rml é copiado incondicionalmente no topo deste arquivo).
  # ---------------------------------------------------------------------------
  add_executable(app_dp_ratio_smoke app_dp_ratio_smoke.cpp)
  target_link_libraries(app_dp_ratio_smoke PRIVATE glintfx)
  add_test(NAME app_dp_ratio_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_dp_ratio_smoke>
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

  # ---------------------------------------------------------------------------
  # EN: data_model_smoke — lifecycle test for the data-model API (T1, v0.2.3).
  #     Exercises create_data_model -> bind_string/bind_number -> load ->
  #     update + render without crash; asserts bind_* after load() returns false.
  #     Requires App (GLFW backend). WORKING_DIRECTORY = CMAKE_BINARY_DIR so
  #     "tests/ui/hud.rml" resolves to build/tests/ui/hud.rml.
  # PT: data_model_smoke — teste de ciclo de vida para a API de data-model (T1, v0.2.3).
  #     Exercita create_data_model -> bind_string/bind_number -> load ->
  #     update + render sem crash; verifica que bind_* após load() retorna false.
  #     Requer App (backend GLFW). WORKING_DIRECTORY = CMAKE_BINARY_DIR para que
  #     "tests/ui/hud.rml" resolva para build/tests/ui/hud.rml.
  # ---------------------------------------------------------------------------
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ui)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/hud.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/ui/hud.rml COPYONLY)

  add_executable(data_model_smoke data_model_smoke.cpp)
  target_link_libraries(data_model_smoke PRIVATE glintfx)
  add_test(NAME data_model_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:data_model_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: data_model_scalar — live-scalar propagation test (T2, v0.2.3).
  #     Verifies that set_string() / set_number() followed by update() reflects
  #     the new values in the rendered frame via two snapshot pairs:
  #       (1) String path:  bright_B (after set_string "HELLO HP 99") > bright_A (baseline).
  #       (2) Number path:  bright_C (after set_number 99.0) > bright_B.
  #     Pixel statistic: count of pixels with max(R,G,B) > 100 (bright threshold).
  #     Window: 320×160. Document: tests/ui/scalar.rml (data-model "hud"; OpenSans font).
  #     Font is resolved from WORKING_DIRECTORY (CMAKE_BINARY_DIR = build/) via path
  #     "tests/assets/OpenSans-Regular.ttf" — available because the showcase CMake block
  #     copies demos/showcase/assets/ to build/tests/assets/ (same GLFW block).
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/ui/scalar.rml" resolves to
  #     build/tests/ui/scalar.rml (copied by configure_file below).
  # PT: data_model_scalar — teste de propagação de escalares vivos (T2, v0.2.3).
  #     Verifica que set_string() / set_number() seguido de update() reflete os novos
  #     valores no frame renderizado via dois pares de snapshots:
  #       (1) Caminho string:  bright_B (após set_string "HELLO HP 99") > bright_A (baseline).
  #       (2) Caminho numérico: bright_C (após set_number 99.0) > bright_B.
  #     Estatística de pixel: contagem de pixels com max(R,G,B) > 100 (limiar de brilho).
  #     Janela: 320×160. Documento: tests/ui/scalar.rml (data-model "hud"; fonte OpenSans).
  #     A fonte é resolvida do WORKING_DIRECTORY (CMAKE_BINARY_DIR = build/) via path
  #     "tests/assets/OpenSans-Regular.ttf" — disponível pois o bloco CMake do showcase
  #     copia demos/showcase/assets/ para build/tests/assets/ (mesmo bloco GLFW).
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/ui/scalar.rml" resolva para
  #     build/tests/ui/scalar.rml (copiado pelo configure_file abaixo).
  # ---------------------------------------------------------------------------
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/scalar.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/ui/scalar.rml COPYONLY)

  add_executable(data_model_scalar data_model_scalar.cpp)
  target_link_libraries(data_model_scalar PRIVATE glintfx)
  add_test(NAME data_model_scalar
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:data_model_scalar>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: data_model_list — live-list propagation test (T3, v0.2.3).
  #     Verifies that bind_list() + set_list() wires a string-list variable to a
  #     data-for loop and that mutations propagate to the render:
  #       Snapshot A: log list empty  → near-zero bright pixels.
  #       Snapshot B: 3 items pushed  → bright_B > bright_A (text lines appeared).
  #     Pixel statistic: count of pixels with max(R,G,B) > 100 (bright threshold).
  #     Window: 400x300. Document: tests/ui/log.rml (data-model "hud"; iterates "log").
  #     Font resolved from WORKING_DIRECTORY (CMAKE_BINARY_DIR = build/) via path
  #     "tests/assets/OpenSans-Regular.ttf" — available via the showcase CMake block.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/ui/log.rml" resolves to
  #     build/tests/ui/log.rml (copied by configure_file below).
  # PT: data_model_list — teste de propagação de lista viva (T3, v0.2.3).
  #     Verifica que bind_list() + set_list() liga uma variável de lista de strings a
  #     um laço data-for e que mutações propagam para o render:
  #       Snapshot A: lista log vazia  → pixels claros quase nulos.
  #       Snapshot B: 3 itens pushados → bright_B > bright_A (linhas aparecem).
  #     Estatística de pixel: contagem de pixels com max(R,G,B) > 100 (limiar de brilho).
  #     Janela: 400x300. Documento: tests/ui/log.rml (data-model "hud"; itera "log").
  #     Fonte resolvida do WORKING_DIRECTORY (CMAKE_BINARY_DIR = build/) via path
  #     "tests/assets/OpenSans-Regular.ttf" — disponível via bloco CMake do showcase.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/ui/log.rml" resolva para
  #     build/tests/ui/log.rml (copiado pelo configure_file abaixo).
  # ---------------------------------------------------------------------------
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/log.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/ui/log.rml COPYONLY)

  add_executable(data_model_list data_model_list.cpp)
  target_link_libraries(data_model_list PRIVATE glintfx)
  add_test(NAME data_model_list
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:data_model_list>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: texture_png_alpha — PNG/alpha decode test (T4, v0.2.3).
  #     Verifies three properties of the Gl3RenderInterface LoadTexture override:
  #       (A) PNG decoded (not white fallback): red_count > 500
  #       (B) Not fallback white: white_count < 2 000
  #       (C) No halo (premultiplied alpha): reddish_count < 3 500
  #     Assets (tex.rml + circle_rgba.png) are copied to build/tests/ui/ below.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/ui/tex.rml" and the PNG
  #     referenced therein resolve correctly via RmlUi's FileInterface.
  # PT: texture_png_alpha — teste de decode PNG/alpha (T4, v0.2.3).
  #     Verifica três propriedades do override de LoadTexture em Gl3RenderInterface:
  #       (A) PNG decodificado (não fallback branco): red_count > 500
  #       (B) Não é fallback branco: white_count < 2 000
  #       (C) Sem halo (alpha premultiplicado): reddish_count < 3 500
  #     Assets (tex.rml + circle_rgba.png) são copiados para build/tests/ui/ abaixo.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/ui/tex.rml" e o PNG
  #     referenciado nele resolvam corretamente via FileInterface do RmlUi.
  # ---------------------------------------------------------------------------
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/tex.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/ui/tex.rml COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/circle_rgba.png
                 ${CMAKE_CURRENT_BINARY_DIR}/ui/circle_rgba.png COPYONLY)

  add_executable(texture_png_alpha texture_png_alpha.cpp)
  target_link_libraries(texture_png_alpha PRIVATE glintfx)
  add_test(NAME texture_png_alpha
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:texture_png_alpha>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

endif() # GLINTFX_BACKEND_GLFW

# ---------------------------------------------------------------------------
# EN: Embed-mode tests — ui_layer_attach, ui_layer_compose, gl_state_guard,
#     ui_layer_events, ui_layer_sanity.
#
#     These tests run in BOTH build configurations (GLFW backend ON and OFF).
#     They validate glintfx::UiLayer, which is always compiled.
#
#     GL context fixture: the tests create a hidden GLFW window as a test fixture
#     to provide a current GL context for UiLayer (standing in for an SDL3 window
#     in production). This is a TEST-ONLY GLFW dependency.
#
#     When GLINTFX_BACKEND_GLFW=ON:  window_glfw.cpp is already compiled into the
#     glintfx static library, so the tests pick up WindowGlfw transitively.
#
#     When GLINTFX_BACKEND_GLFW=OFF: the library does NOT contain window_glfw.cpp
#     and does NOT link glfw. A test-only helper target (_glintfx_test_ctx) is
#     created here that compiles window_glfw.cpp and links glfw — the lib itself
#     remains free of the GLFW dependency.
#
# PT: Testes do embed mode — ui_layer_attach, ui_layer_compose, gl_state_guard,
#     ui_layer_events, ui_layer_sanity.
#
#     Estes testes rodam em AMBAS as configurações (backend GLFW ON e OFF).
#     Validam glintfx::UiLayer, que é sempre compilado.
#
#     Fixture de contexto GL: os testes criam uma janela GLFW oculta como fixture
#     para fornecer um contexto GL corrente ao UiLayer (representando a janela SDL3
#     em produção). Isso é uma dependência GLFW SÓ-DE-TESTE.
#
#     Com GLINTFX_BACKEND_GLFW=ON:  window_glfw.cpp já está compilado na lib estática
#     glintfx, então os testes obtêm WindowGlfw transitivamente.
#
#     Com GLINTFX_BACKEND_GLFW=OFF: a lib NÃO contém window_glfw.cpp e NÃO linka
#     glfw. Um target helper só-de-teste (_glintfx_test_ctx) é criado aqui compilando
#     window_glfw.cpp e linkando glfw — a lib permanece livre da dep GLFW.
# ---------------------------------------------------------------------------

if(NOT GLINTFX_BACKEND_GLFW)
  # EN: Test-only GLFW context helper for embed tests (embed-only build).
  #     Compiles window_glfw.cpp and links glfw so the embed tests can create a GL
  #     context fixture without requiring glfw to be a dependency of the lib itself.
  # PT: Helper de contexto GLFW só-de-teste para os testes embed (build embed-only).
  #     Compila window_glfw.cpp e linka glfw para que os testes embed possam criar um
  #     fixture de contexto GL sem exigir que glfw seja dep da lib.
  find_package(glfw3 REQUIRED)
  add_library(_glintfx_test_ctx STATIC
    ${CMAKE_SOURCE_DIR}/src/window_glfw.cpp)
  target_include_directories(_glintfx_test_ctx PUBLIC
    ${CMAKE_SOURCE_DIR}/src)
  # EN: gl3w provides <GL/gl3w.h> needed by window_glfw.cpp. glfw provides <GLFW/glfw3.h>.
  #     Both are PUBLIC so the link propagates to consuming test executables.
  # PT: gl3w fornece <GL/gl3w.h> necessário por window_glfw.cpp. glfw fornece <GLFW/glfw3.h>.
  #     Ambos são PUBLIC para propagar ao link dos executáveis de teste que consomem.
  target_link_libraries(_glintfx_test_ctx PUBLIC glintfx glfw gl3w)
  # EN: _embed_dep: used below as the single link target for all embed tests.
  #     In OFF builds it is the test helper (which pulls glintfx + glfw + gl3w);
  #     in ON builds it is glintfx directly (window_glfw already compiled in).
  # PT: _embed_dep: usado abaixo como alvo único de link para todos os testes embed.
  #     Em builds OFF é o helper de teste (que puxa glintfx + glfw + gl3w);
  #     em builds ON é o glintfx diretamente (window_glfw já compilado nele).
  set(_embed_dep _glintfx_test_ctx)
else()
  set(_embed_dep glintfx)
endif()

# EN: ui_layer_attach — verify UiLayer embed attach: host creates GL context (hidden GLFW window),
#     UiLayer attaches to it, load() + set_viewport() must not crash and ok() must be true.
# PT: ui_layer_attach — verifica attach do UiLayer embed: host cria contexto GL (janela GLFW oculta),
#     UiLayer anexa a ele, load() + set_viewport() não devem crashar e ok() deve ser true.
add_executable(ui_layer_attach ui_layer_attach.cpp)
target_link_libraries(ui_layer_attach PRIVATE ${_embed_dep})
# EN: min.rml already copied to build/tests/ unconditionally at the top of this file.
# PT: min.rml já copiado para build/tests/ incondicionalmente no topo deste arquivo.
add_test(NAME ui_layer_attach
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ui_layer_attach>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: ui_layer_compose — compose-only render preserves the host's FBO anchor colour.
#     Links gl3w explicitly: offscreen.hpp calls GL functions directly and gl3w is
#     a PRIVATE dep of glintfx (not propagated to callers).
# PT: ui_layer_compose — render compose-only preserva a cor âncora do FBO do host.
#     Linka gl3w explicitamente: offscreen.hpp chama funções GL diretamente e gl3w é
#     dep PRIVATE da glintfx (não propagada aos callers).
add_executable(ui_layer_compose ui_layer_compose.cpp)
target_link_libraries(ui_layer_compose PRIVATE ${_embed_dep} gl3w)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/min_partial.rml
               ${CMAKE_CURRENT_BINARY_DIR}/min_partial.rml COPYONLY)
add_test(NAME ui_layer_compose
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ui_layer_compose>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: gl_state_guard — GL state captured before and after UiLayer::render() must be identical.
#     Links gl3w explicitly: the test creates dummy GL objects (program, VAO) directly.
# PT: gl_state_guard — estado GL capturado antes e depois de UiLayer::render() deve ser idêntico.
#     Linka gl3w explicitamente: o teste cria objetos GL dummy (programa, VAO) diretamente.
add_executable(gl_state_guard gl_state_guard.cpp)
target_link_libraries(gl_state_guard PRIVATE ${_embed_dep} gl3w)
add_test(NAME gl_state_guard
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:gl_state_guard>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: ui_layer_events — inject representative UiEvent sequence (mouse, key nav, resize) and
#     verify no crash + ok() stays true after the sequence (T4 — embed mode input).
# PT: ui_layer_events — injeta sequência representativa de UiEvent (mouse, nav por tecla, resize)
#     e verifica sem crash + ok() permanece true após a sequência (T4 — input embed mode).
add_executable(ui_layer_events ui_layer_events.cpp)
target_link_libraries(ui_layer_events PRIVATE ${_embed_dep})
# EN: min.rml already copied to build/tests/ unconditionally at the top of this file.
# PT: min.rml já copiado para build/tests/ incondicionalmente no topo deste arquivo.
add_test(NAME ui_layer_events
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ui_layer_events>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: ui_layer_sanity — embed-mode render gate: UiLayer attaches to a hidden 900×600 window,
#     renders embed_scene.rml (dark body + linear-gradient + glow card) compose-only via FBO 0,
#     and verifies three aggregate pixel statistics tolerant of llvmpipe threaded-tile rendering:
#       mean_brightness > 5  (rendered something), dark_count >= 10% (body background present),
#       bright_count >= 1%   (gradient / glow present).
#     Links gl3w explicitly: offscreen.hpp includes GL/gl3w.h directly and gl3w is a
#     PRIVATE dep of glintfx (not propagated to callers).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) so embed_scene.rml and
#     embed_scene.rcss resolve relative to the binary dir where they are copied.
# PT: ui_layer_sanity — gate de renderização do embed mode: UiLayer anexa a janela oculta 900×600,
#     renderiza embed_scene.rml (body escuro + linear-gradient + card glow) compose-only via FBO 0,
#     e verifica três estatísticas de pixel agregadas tolerantes ao rendering de tiles do llvmpipe:
#       mean_brightness > 5  (renderizou algo), dark_count >= 10% (background do body presente),
#       bright_count >= 1%   (gradiente / glow presente).
#     Linka gl3w explicitamente: offscreen.hpp inclui GL/gl3w.h diretamente e gl3w é
#     dep PRIVATE da glintfx (não propagado aos callers).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) para que embed_scene.rml e
#     embed_scene.rcss resolvam relativos ao dir binário onde são copiados.
add_executable(ui_layer_sanity ui_layer_sanity.cpp)
target_link_libraries(ui_layer_sanity PRIVATE ${_embed_dep} gl3w)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/embed_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/embed_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/embed_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/embed_scene.rcss COPYONLY)
add_test(NAME ui_layer_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ui_layer_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: dp_ratio_sanity — verifies UiLayerConfig::dp_ratio (Config path) and
#     set_dp_ratio() (runtime setter path). Renders a 100dp×100dp white box on
#     a black background; at dp_ratio=2 the box covers ~4× more pixels than at
#     dp_ratio=1. Assert bright_count(ratio=2) >= 3 * bright_count(ratio=1).
#     Links gl3w explicitly: offscreen.hpp calls GL functions directly.
# PT: dp_ratio_sanity — verifica UiLayerConfig::dp_ratio (caminho Config) e
#     set_dp_ratio() (caminho setter runtime). Renderiza box branco 100dp×100dp em
#     fundo preto; em dp_ratio=2 o box cobre ~4× mais pixels que em dp_ratio=1.
#     Verifica bright_count(ratio=2) >= 3 * bright_count(ratio=1).
#     Linka gl3w explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(dp_ratio_sanity dp_ratio_sanity.cpp)
target_link_libraries(dp_ratio_sanity PRIVATE ${_embed_dep} gl3w)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dp_ratio_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/dp_ratio_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dp_ratio_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/dp_ratio_scene.rcss COPYONLY)
add_test(NAME dp_ratio_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:dp_ratio_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: base_url_sanity — verifies set_asset_base_url() on UiLayer.
#     Proves two things:
#       [Negative] load("bu_scene.rml") WITHOUT base URL silently fails because
#       bu_scene.rml/.rcss are placed ONLY in base_url_assets/, never in the test
#       CWD (= build/tests/). A render after the failed load is all-black.
#       [Positive] After set_asset_base_url("base_url_assets") the same path resolves
#       and the scene renders; three aggregate pixel statistics are checked.
#     Using distinct filenames (bu_scene vs embed_scene) guarantees the negative assert
#     stays valid: embed_scene.rml lives in CWD (for ui_layer_sanity) but bu_scene.rml
#     does not — there is no accidental resolution path.
#     Links gl3w explicitly: offscreen.hpp calls GL functions directly.
# PT: base_url_sanity — verifica set_asset_base_url() no UiLayer.
#     Prova duas coisas:
#       [Negativo] load("bu_scene.rml") SEM base URL falha silenciosamente porque
#       bu_scene.rml/.rcss são colocados SOMENTE em base_url_assets/, nunca no CWD
#       do teste (= build/tests/). Um render após a carga falha é todo-preto.
#       [Positivo] Após set_asset_base_url("base_url_assets") o mesmo path resolve
#       e a cena renderiza; três estatísticas de pixel agregadas são verificadas.
#     Usar nomes distintos (bu_scene vs embed_scene) garante que o assert negativo
#     seja válido: embed_scene.rml está no CWD (para ui_layer_sanity) mas bu_scene.rml
#     não está — não há caminho de resolução acidental.
#     Linka gl3w explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(base_url_sanity base_url_sanity.cpp)
target_link_libraries(base_url_sanity PRIVATE ${_embed_dep} gl3w)
# EN: Create base_url_assets/ and copy bu_scene files ONLY there — NOT to the CWD root.
#     The test CWD is CMAKE_CURRENT_BINARY_DIR (= build/tests/). bu_scene.rml MUST NOT
#     exist there so the negative assert (mean_brightness < 5 without base URL) holds.
# PT: Cria base_url_assets/ e copia arquivos bu_scene SOMENTE lá — NÃO para a raiz do CWD.
#     O CWD do teste é CMAKE_CURRENT_BINARY_DIR (= build/tests/). bu_scene.rml NAO DEVE
#     existir lá para que o assert negativo (mean_brightness < 5 sem base URL) seja válido.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/base_url_assets)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bu_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/base_url_assets/bu_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bu_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/base_url_assets/bu_scene.rcss COPYONLY)
add_test(NAME base_url_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:base_url_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: ua_stylesheet_sanity — verifies the embedded UA-stylesheet (v0.2.4 T1):
#     'display: block' defaults for structural elements (div, p, h1..h6, ul, ol, li,
#     section, article, header, footer, nav, main), applied by Bootstrap::load() to
#     every document via StyleSheetContainer::CombineStyleSheetContainer.
#     Scene has two <div>s WITHOUT an explicit 'display: block' rule; if they stack
#     (block, correct) their pixel row ranges are disjoint, if they share a line box
#     (inline, bug) the ranges overlap.
#     Links gl3w explicitly: offscreen.hpp calls GL functions directly.
# PT: ua_stylesheet_sanity — verifica a UA-stylesheet embutida (v0.2.4 T1): defaults de
#     'display: block' para elementos estruturais (div, p, h1..h6, ul, ol, li, section,
#     article, header, footer, nav, main), aplicada por Bootstrap::load() a todo
#     documento via StyleSheetContainer::CombineStyleSheetContainer.
#     A cena tem dois <div>s SEM regra explícita 'display: block'; se empilham (block,
#     correto) as faixas de linha de pixel são disjuntas, se compartilham line box
#     (inline, bug) as faixas se sobrepõem.
#     Linka gl3w explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(ua_stylesheet_sanity ua_stylesheet_sanity.cpp)
target_link_libraries(ua_stylesheet_sanity PRIVATE ${_embed_dep} gl3w)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_stylesheet_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/ua_stylesheet_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_stylesheet_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/ua_stylesheet_scene.rcss COPYONLY)
add_test(NAME ua_stylesheet_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ua_stylesheet_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
