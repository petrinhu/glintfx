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

# EN: click_scene.rml/.rcss (F1, v0.2.5) -- shared by click_callback_sanity (embed, both
#     configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) and app_click_callback_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- addressed there via "tests/" prefix).
#     Copied once here, next to min.rml, so both consumers find the same single copy.
# PT: click_scene.rml/.rcss (F1, v0.2.5) -- compartilhado por click_callback_sanity (embed,
#     2 configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) e app_click_callback_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- endereçado lá via prefixo "tests/").
#     Copiado uma vez aqui, junto do min.rml, para que os dois consumidores achem a mesma cópia.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/click_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/click_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/click_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/click_scene.rcss COPYONLY)

# EN: element_box_scene.rml/.rcss (F2, v0.2.5) -- shared by element_box_sanity (embed, both
#     configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) and app_element_box_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- addressed there via "tests/" prefix).
#     Copied once here, next to click_scene.*, so both consumers find the same single copy.
# PT: element_box_scene.rml/.rcss (F2, v0.2.5) -- compartilhado por element_box_sanity (embed,
#     2 configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) e app_element_box_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- endereçado lá via prefixo "tests/").
#     Copiado uma vez aqui, junto do click_scene.*, para que os dois consumidores achem a
#     mesma cópia.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/element_box_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/element_box_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/element_box_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/element_box_scene.rcss COPYONLY)

# EN: viewport_origin_scene.rml/.rcss (F3, v0.2.5) -- used only by viewport_origin_sanity
#     (embed, both configs). Copied once here, next to element_box_scene.*, so the test finds
#     it regardless of WORKING_DIRECTORY (CMAKE_CURRENT_BINARY_DIR, same pattern as
#     click_callback_sanity/element_box_sanity below).
# PT: viewport_origin_scene.rml/.rcss (F3, v0.2.5) -- usado só por viewport_origin_sanity
#     (embed, 2 configs). Copiado uma vez aqui, junto de element_box_scene.*, para que o
#     teste o encontre independente do WORKING_DIRECTORY (CMAKE_CURRENT_BINARY_DIR, mesmo
#     padrão de click_callback_sanity/element_box_sanity abaixo).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/viewport_origin_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/viewport_origin_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/viewport_origin_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/viewport_origin_scene.rcss COPYONLY)

# EN: scroll_scene.rml/.rcss (GLINTFX-SCROLL-1, v0.4.0) -- shared by scroll_sanity (embed,
#     both configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) and app_scroll_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- addressed there via "tests/" prefix).
#     Copied once here, next to viewport_origin_scene.*, so both consumers find the same
#     single copy -- same reasoning as element_box_scene.* above.
# PT: scroll_scene.rml/.rcss (GLINTFX-SCROLL-1, v0.4.0) -- compartilhado por scroll_sanity
#     (embed, 2 configs, WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR) e app_scroll_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- endereçado lá via prefixo "tests/").
#     Copiado uma vez aqui, junto do viewport_origin_scene.*, para que os dois consumidores
#     achem a mesma cópia -- mesma razão do element_box_scene.* acima.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scroll_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/scroll_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scroll_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/scroll_scene.rcss COPYONLY)

# EN: resize_scene.rml/.rcss (AUD-PUB-1, v0.5.0) -- used only by app_resize_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- addressed there via "tests/" prefix,
#     same as element_box_scene.*/scroll_scene.* above). Copied unconditionally here so it
#     is always present, same convention as the rest of this shared-asset block.
# PT: resize_scene.rml/.rcss (AUD-PUB-1, v0.5.0) -- usado só por app_resize_smoke
#     (GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR -- endereçado lá via prefixo "tests/",
#     mesmo padrão de element_box_scene.*/scroll_scene.* acima). Copiado incondicionalmente
#     aqui, mesma convenção do resto deste bloco de assets compartilhados.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resize_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/resize_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resize_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/resize_scene.rcss COPYONLY)

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

  # ---------------------------------------------------------------------------
  # EN: app_click_callback_smoke -- App parity smoke for set_click_callback() (F1, v0.2.5).
  #     No pixel/hit assertion (that is click_callback_sanity's job via UiLayer, same Engine
  #     path); this only proves the surface is wired on App and ok() stays true after render.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention); click_scene.rml
  #     is copied once to CMAKE_CURRENT_BINARY_DIR above, addressed here as "tests/click_scene.rml".
  # PT: app_click_callback_smoke -- smoke de paridade do App para set_click_callback() (F1, v0.2.5).
  #     Sem asserção de pixel/hit (isso é papel de click_callback_sanity via UiLayer, mesmo
  #     caminho de Engine); só prova que a superfície está conectada no App e ok() permanece
  #     true após o render. WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em
  #     todo o repo); click_scene.rml é copiado uma vez para CMAKE_CURRENT_BINARY_DIR acima,
  #     endereçado aqui como "tests/click_scene.rml".
  # ---------------------------------------------------------------------------
  add_executable(app_click_callback_smoke app_click_callback_smoke.cpp)
  target_link_libraries(app_click_callback_smoke PRIVATE glintfx)
  add_test(NAME app_click_callback_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_click_callback_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_element_box_smoke -- App parity smoke for get_element_box() (F2, v0.2.5).
  #     No pixel assertion (that is element_box_sanity's job via UiLayer, same Engine path);
  #     this only proves the surface is wired on App and returns found=true for a known id.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention);
  #     element_box_scene.rml is copied once to CMAKE_CURRENT_BINARY_DIR above, addressed
  #     here as "tests/element_box_scene.rml".
  # PT: app_element_box_smoke -- smoke de paridade do App para get_element_box() (F2, v0.2.5).
  #     Sem asserção de pixel (isso é papel de element_box_sanity via UiLayer, mesmo caminho
  #     de Engine); só prova que a superfície está conectada no App e retorna found=true para
  #     um id conhecido. WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo
  #     o repo); element_box_scene.rml é copiado uma vez para CMAKE_CURRENT_BINARY_DIR acima,
  #     endereçado aqui como "tests/element_box_scene.rml".
  # ---------------------------------------------------------------------------
  add_executable(app_element_box_smoke app_element_box_smoke.cpp)
  target_link_libraries(app_element_box_smoke PRIVATE glintfx)
  add_test(NAME app_element_box_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_element_box_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_scroll_smoke -- App parity smoke for scroll_element_into_view()/
  #     get_element_scroll_top()/set_element_scroll_top() (GLINTFX-SCROLL-1, v0.4.0). No wheel
  #     assertion (App has no process_event/input-dispatch surface at all -- App::run()'s only
  #     input path is poll_events() -> glfwPollEvents(), with no GLFW callbacks wired to RmlUi;
  #     wheel forwarding is embed-only, proven in scroll_sanity.cpp via UiLayer). This only
  #     proves the three scroll methods are wired on App and return the expected values for a
  #     known id (same Engine path scroll_sanity.cpp exercises via UiLayer). WORKING_DIRECTORY =
  #     CMAKE_BINARY_DIR (repo-wide GLFW-block convention); scroll_scene.rml is copied once to
  #     CMAKE_CURRENT_BINARY_DIR by the scroll_sanity block below, addressed here as
  #     "tests/scroll_scene.rml".
  # PT: app_scroll_smoke -- smoke de paridade do App para scroll_element_into_view()/
  #     get_element_scroll_top()/set_element_scroll_top() (GLINTFX-SCROLL-1, v0.4.0). Sem
  #     asserção de wheel (o App não tem NENHUMA superfície de process_event/dispatch de input
  #     -- o único caminho de input do App::run() é poll_events() -> glfwPollEvents(), sem
  #     callbacks GLFW conectados ao RmlUi; o encaminhamento de wheel é exclusivo do embed mode,
  #     provado em scroll_sanity.cpp via UiLayer). Só prova que os três métodos de rolagem estão
  #     conectados no App e retornam os valores esperados para um id conhecido (mesmo caminho de
  #     Engine que scroll_sanity.cpp exercita via UiLayer). WORKING_DIRECTORY = CMAKE_BINARY_DIR
  #     (convenção do bloco GLFW em todo o repo); scroll_scene.rml é copiado uma vez para
  #     CMAKE_CURRENT_BINARY_DIR pelo bloco scroll_sanity abaixo, endereçado aqui como
  #     "tests/scroll_scene.rml".
  # ---------------------------------------------------------------------------
  add_executable(app_scroll_smoke app_scroll_smoke.cpp)
  target_link_libraries(app_scroll_smoke PRIVATE glintfx)
  add_test(NAME app_scroll_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_scroll_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_input_hardening_smoke -- App-path parity smoke for the v0.3.0 input-hardening audit
  #     fixes (GLFW backend only): App::load(nullptr) returns false without aborting the process
  #     (Bootstrap::load null-guard, CRITICAL), and App::set_dp_ratio(0/-1/NaN/inf) does not crash
  #     and keeps ok() true. The pixel-level proof lives in input_hardening_sanity (embed path,
  #     both configs); this only proves the App surface shares the same guards. WORKING_DIRECTORY
  #     = CMAKE_BINARY_DIR so "tests/min.rml" resolves (copied unconditionally at the top).
  # PT: app_input_hardening_smoke -- smoke de paridade do caminho App para os fixes da auditoria de
  #     hardening de entrada v0.3.0 (só backend GLFW): App::load(nullptr) retorna false sem abortar
  #     o processo (null-guard de Bootstrap::load, CRITICAL), e App::set_dp_ratio(0/-1/NaN/inf) não
  #     crasha e mantém ok() true. A prova a nível de pixel está em input_hardening_sanity (caminho
  #     embed, 2 configs); este só prova que a superfície do App compartilha as mesmas guardas.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/min.rml" resolva (copiado no topo).
  # ---------------------------------------------------------------------------
  add_executable(app_input_hardening_smoke app_input_hardening_smoke.cpp)
  target_link_libraries(app_input_hardening_smoke PRIVATE glintfx)
  add_test(NAME app_input_hardening_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_input_hardening_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_resize_smoke -- regression test for App standalone not re-flowing RmlUi layout
  #     on window resize (AUD-PUB-1, v0.5.0). App::render() must now cache the last known
  #     window size and call Engine::set_viewport() (-> Rml::Context::SetDimensions) when it
  #     changes between frames, before Engine::render_standalone(). See app_resize_smoke.cpp
  #     for the full bug/fix/oracle writeup. GLFW-only: forces a real OS-level
  #     glfwSetWindowSize() via glfwGetCurrentContext(), so it links `glfw` directly for the
  #     header/symbols (glintfx links glfw PRIVATE, not propagated to consumers -- same
  #     rationale as golden_test linking glloader directly below). WORKING_DIRECTORY =
  #     CMAKE_BINARY_DIR (repo-wide GLFW-block convention); resize_scene.rml is copied once
  #     to CMAKE_CURRENT_BINARY_DIR above, addressed here as "tests/resize_scene.rml".
  # PT: app_resize_smoke -- teste de regressão para o App standalone não refazer o layout do
  #     RmlUi em resize de janela (AUD-PUB-1, v0.5.0). App::render() agora precisa cachear o
  #     último tamanho de janela conhecido e chamar Engine::set_viewport() (->
  #     Rml::Context::SetDimensions) quando ele mudar entre frames, antes de
  #     Engine::render_standalone(). Ver app_resize_smoke.cpp para o relato completo de
  #     bug/fix/oráculo. Só GLFW: força um glfwSetWindowSize() real no nível do SO via
  #     glfwGetCurrentContext(), então linka `glfw` diretamente pelo header/símbolos
  #     (glintfx linka glfw PRIVATE, não propagado a consumidores -- mesma racional de
  #     golden_test linkando glloader diretamente abaixo). WORKING_DIRECTORY = CMAKE_BINARY_DIR
  #     (convenção do bloco GLFW em todo o repo); resize_scene.rml é copiado uma vez para
  #     CMAKE_CURRENT_BINARY_DIR acima, endereçado aqui como "tests/resize_scene.rml".
  # ---------------------------------------------------------------------------
  add_executable(app_resize_smoke app_resize_smoke.cpp)
  target_link_libraries(app_resize_smoke PRIVATE glintfx glfw)
  add_test(NAME app_resize_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_resize_smoke>
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
  # EN: capture.hpp includes gl_loader.h directly — link glloader (INTERFACE target) so its include path
  #     is available here. glloader is a PRIVATE dep of glintfx and therefore not propagated to callers.
  # PT: capture.hpp inclui gl_loader.h diretamente — linka glloader (target INTERFACE) para que o include
  #     path esteja disponível aqui. glloader é dep PRIVATE da glintfx e portanto não é propagado.
  target_link_libraries(golden_test PRIVATE glintfx glloader)

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

  # ---------------------------------------------------------------------------
  # EN: data_model_rebind_uaf — regression test for the duplicate-key
  #     heap-use-after-free in DataBinder::bind_* (audit finding, 2026-07-04).
  #     Duplicate bind_number/bind_string/bind_bool/bind_list on the SAME key must
  #     return false as a pure no-op (guard checks map membership BEFORE touching
  #     storage); the surviving (first) cell must stay fully functional through
  #     load() + set_*() + render, proven via the same {{label}}/{{hp}} views as
  #     data_model_scalar (tests/ui/scalar.rml, already copied above — chosen over
  #     hud.rml because it @font-face-loads OpenSans and actually rasterises glyphs
  #     under Xvfb/llvmpipe, per data_model_scalar's own proof).
  #     Under GLINTFX_SANITIZE=ON, ASan must stay silent — without the guard, the
  #     duplicate call would free the cell the pre-existing Bind() binding still
  #     points to, and this test's render pass would dereference it (UAF abort).
  # PT: data_model_rebind_uaf — teste de regressão para o heap-use-after-free de
  #     chave duplicada em DataBinder::bind_* (achado de auditoria, 2026-07-04).
  #     bind_number/bind_string/bind_bool/bind_list duplicado na MESMA chave deve
  #     retornar false como no-op puro (guard checa presença no mapa ANTES de tocar
  #     o storage); a célula sobrevivente (primeira) deve continuar totalmente
  #     funcional através de load() + set_*() + render, provado via as mesmas views
  #     {{label}}/{{hp}} do data_model_scalar (tests/ui/scalar.rml, já copiado acima —
  #     escolhido em vez de hud.rml porque carrega OpenSans via @font-face e de fato
  #     rasteriza glifos sob Xvfb/llvmpipe, conforme a própria prova do data_model_scalar).
  #     Sob GLINTFX_SANITIZE=ON, o ASan deve ficar em silêncio — sem o guard, a
  #     chamada duplicada liberaria a célula que o Bind() pré-existente ainda
  #     aponta, e o passe de render deste teste a desreferenciaria (abort UAF).
  # ---------------------------------------------------------------------------
  add_executable(data_model_rebind_uaf data_model_rebind_uaf.cpp)
  target_link_libraries(data_model_rebind_uaf PRIVATE glintfx)
  add_test(NAME data_model_rebind_uaf
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:data_model_rebind_uaf>
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
  # EN: glloader provides <gl_loader.h> needed by window_glfw.cpp. glfw provides <GLFW/glfw3.h>.
  #     Both are PUBLIC so the link propagates to consuming test executables.
  # PT: glloader fornece <gl_loader.h> necessário por window_glfw.cpp. glfw fornece <GLFW/glfw3.h>.
  #     Ambos são PUBLIC para propagar ao link dos executáveis de teste que consomem.
  target_link_libraries(_glintfx_test_ctx PUBLIC glintfx glfw glloader)
  # EN: _embed_dep: used below as the single link target for all embed tests.
  #     In OFF builds it is the test helper (which pulls glintfx + glfw + glloader);
  #     in ON builds it is glintfx directly (window_glfw already compiled in).
  # PT: _embed_dep: usado abaixo como alvo único de link para todos os testes embed.
  #     Em builds OFF é o helper de teste (que puxa glintfx + glfw + glloader);
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
#     Links glloader explicitly: offscreen.hpp calls GL functions directly and glloader is
#     a PRIVATE dep of glintfx (not propagated to callers).
# PT: ui_layer_compose — render compose-only preserva a cor âncora do FBO do host.
#     Linka glloader explicitamente: offscreen.hpp chama funções GL diretamente e glloader é
#     dep PRIVATE da glintfx (não propagada aos callers).
add_executable(ui_layer_compose ui_layer_compose.cpp)
target_link_libraries(ui_layer_compose PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/min_partial.rml
               ${CMAKE_CURRENT_BINARY_DIR}/min_partial.rml COPYONLY)
add_test(NAME ui_layer_compose
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ui_layer_compose>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: gl_state_guard — GL state captured before and after UiLayer::render() must be identical.
#     Links glloader explicitly: the test creates dummy GL objects (program, VAO) directly.
# PT: gl_state_guard — estado GL capturado antes e depois de UiLayer::render() deve ser idêntico.
#     Linka glloader explicitamente: o teste cria objetos GL dummy (programa, VAO) diretamente.
add_executable(gl_state_guard gl_state_guard.cpp)
target_link_libraries(gl_state_guard PRIVATE ${_embed_dep} glloader)
add_test(NAME gl_state_guard
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:gl_state_guard>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# EN: gl_loader_sanity (L1.14-GLLOADER) — glx_gl_load() returns 0 and a representative
#     sample of core function pointers (glGetString, glCreateShader, glBindFramebuffer)
#     is resolved and callable against a real offscreen GL context. Links glloader
#     explicitly: the test calls glx_gl_load() and raw GL functions directly.
# PT: gl_loader_sanity (L1.14-GLLOADER) — glx_gl_load() retorna 0 e uma amostra
#     representativa de ponteiros de função core (glGetString, glCreateShader,
#     glBindFramebuffer) está resolvida e chamável contra um contexto GL offscreen real.
#     Linka glloader explicitamente: o teste chama glx_gl_load() e funções GL cruas
#     diretamente.
add_executable(gl_loader_sanity gl_loader_sanity.cpp)
target_link_libraries(gl_loader_sanity PRIVATE ${_embed_dep} glloader)
add_test(NAME gl_loader_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:gl_loader_sanity>
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
#     Links glloader explicitly: offscreen.hpp includes gl_loader.h directly and glloader is a
#     PRIVATE dep of glintfx (not propagated to callers).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) so embed_scene.rml and
#     embed_scene.rcss resolve relative to the binary dir where they are copied.
# PT: ui_layer_sanity — gate de renderização do embed mode: UiLayer anexa a janela oculta 900×600,
#     renderiza embed_scene.rml (body escuro + linear-gradient + card glow) compose-only via FBO 0,
#     e verifica três estatísticas de pixel agregadas tolerantes ao rendering de tiles do llvmpipe:
#       mean_brightness > 5  (renderizou algo), dark_count >= 10% (background do body presente),
#       bright_count >= 1%   (gradiente / glow presente).
#     Linka glloader explicitamente: offscreen.hpp inclui gl_loader.h diretamente e glloader é
#     dep PRIVATE da glintfx (não propagado aos callers).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) para que embed_scene.rml e
#     embed_scene.rcss resolvam relativos ao dir binário onde são copiados.
add_executable(ui_layer_sanity ui_layer_sanity.cpp)
target_link_libraries(ui_layer_sanity PRIVATE ${_embed_dep} glloader)
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
#     Links glloader explicitly: offscreen.hpp calls GL functions directly.
# PT: dp_ratio_sanity — verifica UiLayerConfig::dp_ratio (caminho Config) e
#     set_dp_ratio() (caminho setter runtime). Renderiza box branco 100dp×100dp em
#     fundo preto; em dp_ratio=2 o box cobre ~4× mais pixels que em dp_ratio=1.
#     Verifica bright_count(ratio=2) >= 3 * bright_count(ratio=1).
#     Linka glloader explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(dp_ratio_sanity dp_ratio_sanity.cpp)
target_link_libraries(dp_ratio_sanity PRIVATE ${_embed_dep} glloader)
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
#     Links glloader explicitly: offscreen.hpp calls GL functions directly.
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
#     Linka glloader explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(base_url_sanity base_url_sanity.cpp)
target_link_libraries(base_url_sanity PRIVATE ${_embed_dep} glloader)
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
#     Links glloader explicitly: offscreen.hpp calls GL functions directly.
# PT: ua_stylesheet_sanity — verifica a UA-stylesheet embutida (v0.2.4 T1): defaults de
#     'display: block' para elementos estruturais (div, p, h1..h6, ul, ol, li, section,
#     article, header, footer, nav, main), aplicada por Bootstrap::load() a todo
#     documento via StyleSheetContainer::CombineStyleSheetContainer.
#     A cena tem dois <div>s SEM regra explícita 'display: block'; se empilham (block,
#     correto) as faixas de linha de pixel são disjuntas, se compartilham line box
#     (inline, bug) as faixas se sobrepõem.
#     Linka glloader explicitamente: offscreen.hpp chama funções GL diretamente.
# ---------------------------------------------------------------------------
add_executable(ua_stylesheet_sanity ua_stylesheet_sanity.cpp)
target_link_libraries(ua_stylesheet_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_stylesheet_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/ua_stylesheet_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_stylesheet_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/ua_stylesheet_scene.rcss COPYONLY)
add_test(NAME ua_stylesheet_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ua_stylesheet_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: click_callback_sanity -- verifies UiLayer::set_click_callback() (F1, v0.2.5): direct id,
#     ancestor-walk fallback for a child without id, "" fallback for a subtree with no id
#     anywhere, and no callback invocation for a click outside any element. Runs in BOTH
#     build configs (embed-mode test, links ${_embed_dep} like the other embed tests above).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as
#     ua_stylesheet_sanity/dp_ratio_sanity/ui_layer_sanity (scene-specific rml/rcss pair
#     copied next to min.rml at the top of this file, addressed here by bare filename).
# PT: click_callback_sanity -- verifica UiLayer::set_click_callback() (F1, v0.2.5): id direto,
#     fallback de subida de ancestral para filho sem id, fallback "" para subárvore sem id em
#     lugar nenhum, e nenhuma invocação do callback para clique fora de qualquer elemento.
#     Roda em AMBAS as configs (teste de embed mode, linka ${_embed_dep} como os outros
#     testes embed acima). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) --
#     mesmo padrão de ua_stylesheet_sanity/dp_ratio_sanity/ui_layer_sanity (par rml/rcss
#     específico da cena, copiado junto do min.rml no topo deste arquivo, endereçado aqui
#     por nome de arquivo simples).
# ---------------------------------------------------------------------------
add_executable(click_callback_sanity click_callback_sanity.cpp)
target_link_libraries(click_callback_sanity PRIVATE ${_embed_dep})
add_test(NAME click_callback_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:click_callback_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: element_box_sanity -- verifies UiLayer::get_element_box() (F2, v0.2.5): exact RCSS-
#     authored border-box geometry for a known id, found=false + zeroed fields for an unknown
#     id, and found=false when no document is loaded yet. Runs in BOTH build configs
#     (embed-mode test, links ${_embed_dep} like the other embed tests above).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as
#     click_callback_sanity/ua_stylesheet_sanity (scene-specific rml/rcss pair copied next to
#     min.rml at the top of this file, addressed here by bare filename).
# PT: element_box_sanity -- verifica UiLayer::get_element_box() (F2, v0.2.5): geometria
#     border-box exata autorada no RCSS para um id conhecido, found=false + campos zerados
#     para id desconhecido, e found=false quando nenhum documento está carregado ainda. Roda
#     em AMBAS as configs (teste de embed mode, linka ${_embed_dep} como os outros testes
#     embed acima). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo
#     padrão de click_callback_sanity/ua_stylesheet_sanity (par rml/rcss específico da cena,
#     copiado junto do min.rml no topo deste arquivo, endereçado aqui por nome de arquivo
#     simples).
# ---------------------------------------------------------------------------
add_executable(element_box_sanity element_box_sanity.cpp)
target_link_libraries(element_box_sanity PRIVATE ${_embed_dep})
add_test(NAME element_box_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:element_box_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: scroll_sanity -- verifies GLINTFX-SCROLL-1 (v0.4.0, consumer-driven by GusWorld: a
#     30-item overflow-y:auto menu list could not scroll in embed mode): wheel-delta
#     forwarding (UiEvent::Type::MouseWheel -> Rml::Context::ProcessMouseWheel, with a
#     direction "tooth" -- opposite-sign delta scrolls the other way) and the three
#     programmatic scroll methods (scroll_element_into_view, get/set_element_scroll_top),
#     plus null-id/unknown-id/no-document-yet/non-finite-input hardening on all three. Runs in
#     BOTH build configs (embed-mode test, links ${_embed_dep} like the other embed tests
#     above). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as
#     click_callback_sanity/element_box_sanity (scene-specific rml/rcss pair copied here,
#     addressed by bare filename).
# PT: scroll_sanity -- verifica GLINTFX-SCROLL-1 (v0.4.0, consumer-driven pelo GusWorld: uma
#     lista de menu de 30 itens overflow-y:auto não conseguia rolar em embed mode):
#     encaminhamento de delta de wheel (UiEvent::Type::MouseWheel -> Rml::Context::
#     ProcessMouseWheel, com um "dente" de direção -- delta de sinal oposto rola pro outro
#     lado) e os três métodos de rolagem programática (scroll_element_into_view,
#     get/set_element_scroll_top), mais hardening de id nulo/id desconhecido/nenhum-documento-
#     ainda/entrada não-finita nos três. Roda em AMBAS as configs (teste de embed mode, linka
#     ${_embed_dep} como os outros testes embed acima). WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo padrão de click_callback_sanity/
#     element_box_sanity (par rml/rcss específico da cena copiado aqui, endereçado por nome de
#     arquivo simples).
# ---------------------------------------------------------------------------
add_executable(scroll_sanity scroll_sanity.cpp)
target_link_libraries(scroll_sanity PRIVATE ${_embed_dep})
# EN: scroll_scene.rml/.rcss already copied to CMAKE_CURRENT_BINARY_DIR unconditionally at the
#     top of this file (shared with app_scroll_smoke).
# PT: scroll_scene.rml/.rcss já copiado para CMAKE_CURRENT_BINARY_DIR incondicionalmente no
#     topo deste arquivo (compartilhado com app_scroll_smoke).
add_test(NAME scroll_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:scroll_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: viewport_origin_sanity -- integration proof for UiLayer::set_viewport(x,y,w,h,target_h)
#     (F3, v0.2.5): backward-compat of the legacy 2-arg overload, letterbox offset applied to
#     get_element_box()'s window-space return value, click-through-offset (mouse translation),
#     the actual GL offset_x wiring (pixel column-range shift, glReadPixels-based), and a
#     Resize while in letterbox mode. Exercises F1 (set_click_callback) + F2
#     (get_element_box) as deterministic oracles -- see the file header for the full
#     assertion list. Runs in BOTH build configs (embed-mode test, links ${_embed_dep} like
#     the other embed tests above). Links glloader explicitly: offscreen.hpp includes gl_loader.h
#     directly and glloader is a PRIVATE dep of glintfx (not propagated to callers).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as
#     click_callback_sanity/element_box_sanity (scene-specific rml/rcss pair copied next to
#     min.rml at the top of this file, addressed here by bare filename).
# PT: viewport_origin_sanity -- prova de integração de UiLayer::set_viewport(x,y,w,h,target_h)
#     (F3, v0.2.5): compat retroativa da sobrecarga legada de 2 args, offset de letterbox
#     aplicado ao retorno espaço-janela de get_element_box(), clique-através-do-offset
#     (tradução de mouse), a fiação REAL do offset_x do GL (deslocamento de faixa de coluna
#     de pixel, via glReadPixels), e um Resize em modo letterbox. Exercita F1
#     (set_click_callback) + F2 (get_element_box) como oráculos determinísticos -- ver o
#     cabeçalho do arquivo pra lista completa de asserções. Roda em AMBAS as configs (teste de
#     embed mode, linka ${_embed_dep} como os outros testes embed acima). Linka glloader
#     explicitamente: offscreen.hpp inclui gl_loader.h diretamente e glloader é dep PRIVATE da
#     glintfx (não propagado aos callers). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (=
#     build/tests/) -- mesmo padrão de click_callback_sanity/element_box_sanity (par rml/rcss
#     específico da cena, copiado junto do min.rml no topo deste arquivo, endereçado aqui por
#     nome de arquivo simples).
# ---------------------------------------------------------------------------
add_executable(viewport_origin_sanity viewport_origin_sanity.cpp)
target_link_libraries(viewport_origin_sanity PRIVATE ${_embed_dep} glloader)
add_test(NAME viewport_origin_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:viewport_origin_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: polygon_sanity -- verifies the "polygon(<sides>, <color>[, <rotation>])" decorator
#     (v0.2.6, decorator_polygon.{hpp,cpp}, consumer-driven by GusWorld's hex slider node):
#     alpha-shape proof (box centre opaque, box corner transparent -- distinguishes a polygon
#     from a filled rectangle) for a hexagon (#hex, sides=6) and an octagon (#oct, sides=8),
#     plus a shape-changes-with-`sides` proof (an east-of-centre sample point is background
#     for the hexagon but opaque for the octagon -- see the file header for the full geometry).
#     Runs in BOTH build configs (embed-mode test, links ${_embed_dep} like the other embed
#     tests above; the decorator is backend-agnostic, registered by Bootstrap which is always
#     compiled). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern
#     as ua_stylesheet_sanity/dp_ratio_sanity (scene-specific rml/rcss pair copied here).
# PT: polygon_sanity -- verifica o decorator "polygon(<lados>, <cor>[, <rotacao>])" (v0.2.6,
#     decorator_polygon.{hpp,cpp}, consumer-driven pelo nó hex slider do GusWorld): prova de
#     alpha-shape (centro da caixa opaco, canto da caixa transparente -- distingue um polígono
#     de um retângulo preenchido) para um hexágono (#hex, sides=6) e um octógono (#oct,
#     sides=8), mais uma prova de que a forma muda com `sides` (um ponto amostrado a leste do
#     centro é background para o hexágono mas opaco para o octógono -- ver o cabeçalho do
#     arquivo para a geometria completa). Roda em AMBAS as configs (teste de embed mode, linka
#     ${_embed_dep} como os outros testes embed acima; o decorator é backend-agnóstico,
#     registrado pelo Bootstrap que é sempre compilado). WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo padrão de
#     ua_stylesheet_sanity/dp_ratio_sanity (par rml/rcss específico da cena copiado aqui).
# ---------------------------------------------------------------------------
add_executable(polygon_sanity polygon_sanity.cpp)
target_link_libraries(polygon_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/polygon_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/polygon_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/polygon_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/polygon_scene.rcss COPYONLY)
add_test(NAME polygon_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:polygon_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: polygon_gradient_sanity -- verifies the "polygon(<sides>, radial-gradient(...) /
#     linear-gradient(...))" fill extension (v0.3.1, decorator_polygon.{hpp,cpp}, consumer-driven
#     by GusWorld's brass hexagon corner nodes needing centre-clear -> edge-dark screw-head
#     volume): radial ramp direction, explicit stop-percentage placement, linear angle-to-direction
#     mapping, the pre-existing SOLID fill left untouched, and fill-syntax/`sides` hardening
#     (malformed fill or sides<3 -> decorator ignored, fail-high). Companion to polygon_sanity.cpp
#     (geometry + `sides` hardening only, unmodified, still passing). Same build-config/
#     WORKING_DIRECTORY pattern as polygon_sanity above.
# PT: polygon_gradient_sanity -- verifica a extensão de preenchimento "polygon(<lados>,
#     radial-gradient(...) / linear-gradient(...))" (v0.3.1, decorator_polygon.{hpp,cpp},
#     consumer-driven pelos nós hexagonais de latão dos cantos do GusWorld que precisavam do
#     volume de cabeça-de-parafuso centro-claro -> borda-escura): direção da rampa radial,
#     posicionamento explícito de porcentagem de stop, mapeamento ângulo-para-direção do linear, o
#     preenchimento SOLID pré-existente intocado, e hardening de sintaxe de preenchimento/`sides`
#     (preenchimento malformado ou sides<3 -> decorator ignorado, fail-high). Companheiro de
#     polygon_sanity.cpp (só geometria + hardening de `sides`, sem modificação, continua passando).
#     Mesmo padrão de config-de-build/WORKING_DIRECTORY do polygon_sanity acima.
# ---------------------------------------------------------------------------
add_executable(polygon_gradient_sanity polygon_gradient_sanity.cpp)
target_link_libraries(polygon_gradient_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/polygon_gradient_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/polygon_gradient_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/polygon_gradient_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/polygon_gradient_scene.rcss COPYONLY)
add_test(NAME polygon_gradient_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:polygon_gradient_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: image_tint_sanity -- verifies the "image-tint(<url>)" luminance-key tint decorator
#     (GLINTFX-TINT-1/2, decorator_image_tint.{hpp,cpp} + the "glintfx-tint" shader injection in
#     render_gl3.cpp, ADR-0010, consumer-driven by GusWorld: one neutral base texture, N domain
#     colours at runtime): the load-bearing 3-zone luminance-key selectivity proof
#     (light+neutral strongly tinted, light+saturated barely tinted, dark untouched), multiply
#     mode's uniform (non-selective) tint as contrast, mode=None's true no-op parity with the
#     native image() decorator, fail-high hardening (unknown keyword / out-of-range threshold /
#     missing texture / empty src -- no crash), CSS opacity composition, the fresh-per-frame
#     property read (image-tint-color driven live via a data-model + data-style-image-tint-color
#     binding, NOT baked at GenerateElementData time), and the ResetProgram() state-restore
#     discipline (a polygon()-gradient / tint / polygon()-gradient triple in the SAME frame,
#     asserting the LAST gradient renders exactly like the FIRST). Same build-config/
#     WORKING_DIRECTORY pattern as polygon_sanity/polygon_gradient_sanity above (runs in BOTH
#     GLINTFX_BACKEND_GLFW configs -- the decorator + shader injection are backend-agnostic).
# PT: image_tint_sanity -- verifica o decorator de tingimento luminance-key
#     "image-tint(<url>)" (GLINTFX-TINT-1/2, decorator_image_tint.{hpp,cpp} + a injeção de shader
#     "glintfx-tint" em render_gl3.cpp, ADR-0010, consumer-driven pelo GusWorld: uma textura base
#     neutra, N cores de domínio em runtime): a prova de carga de seletividade luminance-key em 3
#     zonas (claro+neutro fortemente tingido, claro+saturado levemente tingido, escuro intocado),
#     o tingimento uniforme (não-seletivo) do modo multiply como contraste, o no-op verdadeiro de
#     mode=None com paridade ao decorator image() nativo, hardening fail-high (keyword
#     desconhecida / threshold fora de faixa / textura ausente / src vazio -- sem crash),
#     composição de opacity do CSS, a leitura fresca-por-frame de propriedade (image-tint-color
#     dirigido ao vivo via um binding data-model + data-style-image-tint-color, NÃO embutido no
#     momento de GenerateElementData), e a disciplina de restauração de estado de ResetProgram()
#     (uma tripla polygon()-gradiente / tint / polygon()-gradiente no MESMO frame, afirmando que
#     o ÚLTIMO gradiente renderiza exatamente como o PRIMEIRO). Mesmo padrão de
#     config-de-build/WORKING_DIRECTORY de polygon_sanity/polygon_gradient_sanity acima (roda em
#     AMBAS as configs de GLINTFX_BACKEND_GLFW -- o decorator + a injeção de shader são
#     backend-agnósticos).
# ---------------------------------------------------------------------------
add_executable(image_tint_sanity image_tint_sanity.cpp)
target_link_libraries(image_tint_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/image_tint_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/image_tint_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/image_tint_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/image_tint_scene.rcss COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/image_tint_fixture.png
               ${CMAKE_CURRENT_BINARY_DIR}/image_tint_fixture.png COPYONLY)
add_test(NAME image_tint_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:image_tint_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: input_hardening_sanity -- proves the three v0.3.0 input-surface hardening fixes (security
#     audit) reject invalid input fail-high without breaking valid use, all via the UiLayer/Engine
#     path shared with App: Bootstrap::load(nullptr) -> false with no SIGABRT [CRITICAL];
#     Engine::set_dp_ratio(0/-1/NaN/inf) ignored (layout not degenerated), set_dp_ratio(2.0) still
#     scales; UiLayer::set_viewport(w/h<=0) no-op (both overloads), valid viewport still renders.
#     Reuses dp_ratio_scene.rml/.rcss (copied above for dp_ratio_sanity) as a deterministic
#     bright-pixel oracle. Runs in BOTH build configs (embed-mode, links ${_embed_dep} + glloader for
#     the direct GL readback in offscreen.hpp). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR
#     (= build/tests/) so dp_ratio_scene.rml resolves by bare name.
# PT: input_hardening_sanity -- prova que os tres fixes de hardening da superficie de entrada
#     v0.3.0 (auditoria de seguranca) rejeitam entrada invalida fail-high sem quebrar uso valido,
#     todos via o caminho UiLayer/Engine compartilhado com o App: Bootstrap::load(nullptr) ->
#     false sem SIGABRT [CRITICAL]; Engine::set_dp_ratio(0/-1/NaN/inf) ignorado (layout nao
#     degenerado), set_dp_ratio(2.0) ainda escala; UiLayer::set_viewport(w/h<=0) no-op (as duas
#     sobrecargas), viewport valido ainda renderiza. Reusa dp_ratio_scene.rml/.rcss (copiado acima
#     para dp_ratio_sanity) como oraculo deterministico de pixels brilhantes. Roda em AMBAS as
#     configs (embed-mode, linka ${_embed_dep} + glloader para o readback GL direto em offscreen.hpp).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) para dp_ratio_scene.rml
#     resolver por nome simples.
# ---------------------------------------------------------------------------
add_executable(input_hardening_sanity input_hardening_sanity.cpp)
target_link_libraries(input_hardening_sanity PRIVATE ${_embed_dep} glloader)
add_test(NAME input_hardening_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:input_hardening_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: data_model_embed_sanity -- embed-path (UiLayer) end-to-end proof for the data-model API
#     (AUD-TEC-7, 2026-07-08 audit finding): create_data_model/bind_number/bind_string/
#     bind_bool/bind_list/load/set_number/set_string/set_bool/set_list, all via UiLayer, never
#     exercised before this file (the 4 pre-existing data_model_* tests are App-only, GLFW-only
#     block). Also covers set_bool (previously zero tests anywhere) and the second-
#     create_data_model()-returns-false limit. Geometry-only oracle via get_element_box() (no
#     font/pixel readback) so it renders deterministically in BOTH build configs -- same
#     reasoning as click_callback_sanity/element_box_sanity/scroll_sanity above. Runs in BOTH
#     build configs (links ${_embed_dep}, no glloader needed -- get_element_box() only).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as
#     click_callback_sanity/element_box_sanity (scene-specific rml/rcss pair copied here).
# PT: data_model_embed_sanity -- prova ponta-a-ponta do caminho embed (UiLayer) para a API de
#     data-model (achado de auditoria AUD-TEC-7, 2026-07-08): create_data_model/bind_number/
#     bind_string/bind_bool/bind_list/load/set_number/set_string/set_bool/set_list, todos via
#     UiLayer, nunca exercitados antes deste arquivo (os 4 testes data_model_* pré-existentes
#     são só-App, bloco só-GLFW). Também cobre set_bool (zero testes em qualquer lugar antes) e
#     o limite de 2ª create_data_model()-retorna-false. Oráculo só-de-geometria via
#     get_element_box() (sem fonte/readback de pixel) então renderiza deterministicamente nas
#     DUAS configs de build -- mesmo raciocínio de click_callback_sanity/element_box_sanity/
#     scroll_sanity acima. Roda em AMBAS as configs de build (linka ${_embed_dep}, sem
#     necessidade de glloader -- só get_element_box()). WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo padrão de click_callback_sanity/
#     element_box_sanity (par rml/rcss específico da cena copiado aqui).
# ---------------------------------------------------------------------------
add_executable(data_model_embed_sanity data_model_embed_sanity.cpp)
target_link_libraries(data_model_embed_sanity PRIVATE ${_embed_dep})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data_model_embed_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/data_model_embed_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data_model_embed_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/data_model_embed_scene.rcss COPYONLY)
add_test(NAME data_model_embed_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:data_model_embed_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: document_reload_leak -- regression test for the GusWorld-reported bug (2026-07-04):
#     Bootstrap::load() used to stack documents in the same Rml::Context without closing the
#     previous one on every reload (memory leak + "ghost document" rendering underneath).
#     Drives Engine::load()/Engine::update() directly (internal test, links RmlUi::Core
#     explicitly for Rml::Context::GetNumDocuments() -- same pattern as engine_smoke.cpp) and
#     asserts the document count stays at 1 across a menu-navigation sequence (A->B->A) and
#     across 5x repeated reload of the same path. Runs in BOTH build configs (links
#     ${_embed_dep}, same GL-context-fixture pattern as the other embed tests above) since the
#     fixed code (bootstrap.cpp/engine.cpp) is always compiled regardless of
#     GLINTFX_BACKEND_GLFW. min.rml (top of this file) and min_partial.rml (copied above for
#     ui_layer_compose) are reused as two distinct documents -- no new scene assets needed.
#     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/min.rml"/"tests/min_partial.rml" resolve to
#     build/tests/*.rml regardless of where ctest is invoked from.
# PT: document_reload_leak -- teste de regressão para o bug relatado pelo GusWorld (2026-07-04):
#     Bootstrap::load() empilhava documentos no mesmo Rml::Context sem fechar o anterior a cada
#     reload (vazamento de memória + "documento fantasma" renderizando por baixo). Dispara
#     Engine::load()/Engine::update() diretamente (teste interno, linka RmlUi::Core
#     explicitamente para Rml::Context::GetNumDocuments() -- mesmo padrão de engine_smoke.cpp)
#     e verifica que a contagem de documentos permanece em 1 ao longo de uma sequência de
#     navegação de menu (A->B->A) e ao longo de 5x reload repetido do mesmo path. Roda em AMBAS
#     as configs de build (linka ${_embed_dep}, mesmo padrão de fixture de contexto GL dos
#     outros testes embed acima) já que o código corrigido (bootstrap.cpp/engine.cpp) é sempre
#     compilado independente de GLINTFX_BACKEND_GLFW. min.rml (topo deste arquivo) e
#     min_partial.rml (copiado acima para ui_layer_compose) são reutilizados como dois
#     documentos distintos -- nenhum asset de cena novo é necessário. WORKING_DIRECTORY =
#     CMAKE_BINARY_DIR para que "tests/min.rml"/"tests/min_partial.rml" resolvam para
#     build/tests/*.rml independente de onde o ctest é invocado.
# ---------------------------------------------------------------------------
add_executable(document_reload_leak document_reload_leak.cpp)
target_link_libraries(document_reload_leak PRIVATE ${_embed_dep} RmlUi::Core)
add_test(NAME document_reload_leak
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:document_reload_leak>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: focus_sanity -- verifies Engine::set_focus()/clear_focus() (L1.17-FOCUS, consumes the
#     GAP-4 seed from docs/embed-integration.md section 5), the exact code both
#     glintfx::UiLayer::set_focus/clear_focus and glintfx::App::set_focus/clear_focus delegate
#     to with zero extra logic -- so this single test covers both public consumption paths,
#     same precedent as document_reload_leak.cpp above ("drive Engine directly for the internal
#     oracle"). Needs Rml::Context::GetFocusElement()/Rml::Element::GetId() as the discriminating
#     oracle (not reachable through UiLayer's public, RmlUi-type-free header), so it links
#     RmlUi::Core explicitly, same reasoning as document_reload_leak.cpp/engine_smoke.cpp. Runs
#     in BOTH build configs (links ${_embed_dep}, same GL-context-fixture pattern as the other
#     embed tests above) since the tested code (bootstrap.cpp/engine.cpp) is always compiled
#     regardless of GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (=
#     build/tests/) -- same pattern as click_callback_sanity/element_box_sanity/scroll_sanity
#     (scene-specific rml/rcss pair copied here, addressed by bare filename).
# PT: focus_sanity -- verifica Engine::set_focus()/clear_focus() (L1.17-FOCUS, consome a semente
#     GAP-4 de docs/embed-integration.md seção 5), o código EXATO para o qual
#     glintfx::UiLayer::set_focus/clear_focus e glintfx::App::set_focus/clear_focus encaminham
#     sem lógica extra -- então este único teste cobre os dois caminhos públicos de consumo,
#     mesmo precedente do document_reload_leak.cpp acima ("dirige o Engine diretamente pelo
#     oráculo interno"). Precisa de Rml::Context::GetFocusElement()/Rml::Element::GetId() como
#     oráculo discriminante (não alcançável pelo header público do UiLayer, livre de tipos
#     RmlUi), então linka RmlUi::Core explicitamente, mesmo raciocínio de
#     document_reload_leak.cpp/engine_smoke.cpp. Roda em AMBAS as configs (linka ${_embed_dep},
#     mesmo padrão de fixture de contexto GL dos outros testes embed acima) já que o código
#     testado (bootstrap.cpp/engine.cpp) é sempre compilado independente de
#     GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) --
#     mesmo padrão de click_callback_sanity/element_box_sanity/scroll_sanity (par rml/rcss
#     específico da cena copiado aqui, endereçado por nome de arquivo simples).
# ---------------------------------------------------------------------------
add_executable(focus_sanity focus_sanity.cpp)
target_link_libraries(focus_sanity PRIVATE ${_embed_dep} RmlUi::Core)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/focus_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/focus_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/focus_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/focus_scene.rcss COPYONLY)
add_test(NAME focus_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:focus_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: domrw_sanity -- verifies Engine::set_text/add_class/remove_class/set_property (DOM write
#     by id) and Engine::get_number/get_string/get_bool (data-model read-back) (L1.16-DOMRW,
#     consumes AUD-PUB-6(d)+(e)), the exact code both glintfx::UiLayer and glintfx::App delegate
#     to with zero extra logic -- so this single test covers both public consumption paths, same
#     precedent as focus_sanity.cpp/document_reload_leak.cpp above ("drive Engine directly for
#     the internal oracle"). Needs Rml::Element::GetInnerRML()/IsClassSet()/
#     GetProperty<float>()/Rml::Context::GetDocument() as the discriminating oracles (not
#     reachable through UiLayer's public, RmlUi-type-free header), so it links RmlUi::Core
#     explicitly, same reasoning as focus_sanity.cpp/document_reload_leak.cpp/engine_smoke.cpp.
#     Runs in BOTH build configs (links ${_embed_dep}, same GL-context-fixture pattern as the
#     other embed tests above) since the tested code (bootstrap.cpp/engine.cpp/data_binder.cpp)
#     is always compiled regardless of GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- same pattern as focus_sanity/
#     click_callback_sanity/element_box_sanity (scene-specific rml/rcss pair copied here,
#     addressed by bare filename).
# PT: domrw_sanity -- verifica Engine::set_text/add_class/remove_class/set_property (escrita de
#     DOM por id) e Engine::get_number/get_string/get_bool (leitura de volta do data-model)
#     (L1.16-DOMRW, consome AUD-PUB-6(d)+(e)), o código EXATO para o qual tanto glintfx::UiLayer
#     quanto glintfx::App encaminham sem lógica extra -- então este único teste cobre os dois
#     caminhos públicos de consumo, mesmo precedente do focus_sanity.cpp/
#     document_reload_leak.cpp acima ("dirige o Engine diretamente pelo oráculo interno").
#     Precisa de Rml::Element::GetInnerRML()/IsClassSet()/GetProperty<float>()/
#     Rml::Context::GetDocument() como oráculos discriminantes (não alcançáveis pelo header
#     público do UiLayer, livre de tipos RmlUi), então linka RmlUi::Core explicitamente, mesmo
#     raciocínio de focus_sanity.cpp/document_reload_leak.cpp/engine_smoke.cpp. Roda em AMBAS as
#     configs de build (linka ${_embed_dep}, mesmo padrão de fixture de contexto GL dos outros
#     testes embed acima) já que o código testado (bootstrap.cpp/engine.cpp/data_binder.cpp) é
#     sempre compilado independente de GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo padrão de focus_sanity/
#     click_callback_sanity/element_box_sanity (par rml/rcss específico da cena copiado aqui,
#     endereçado por nome de arquivo simples).
# ---------------------------------------------------------------------------
add_executable(domrw_sanity domrw_sanity.cpp)
target_link_libraries(domrw_sanity PRIVATE ${_embed_dep} RmlUi::Core)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/domrw_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/domrw_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/domrw_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/domrw_scene.rcss COPYONLY)
add_test(NAME domrw_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:domrw_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: form_events_sanity -- verifies the 5 L1.15-FORMEV callbacks (Engine::set_change_callback/
#     set_submit_callback/set_focus_callback/set_blur_callback/set_hover_callback), the exact
#     code both glintfx::UiLayer and glintfx::App delegate to with zero extra logic -- so this
#     single test covers both public consumption paths, same precedent as
#     focus_sanity.cpp/domrw_sanity.cpp ("drive Engine directly for the internal oracle"). Needs
#     Rml::Context::ProcessMouseMove/ProcessMouseButtonDown/ProcessMouseButtonUp/
#     GetHoverElement() as the input-simulation/oracle surface (not reachable through UiLayer's
#     public, RmlUi-type-free header, and Engine itself has no process_event() translation
#     layer -- that is UiLayer-only), so it links RmlUi::Core explicitly, same reasoning as
#     focus_sanity.cpp/domrw_sanity.cpp/document_reload_leak.cpp/engine_smoke.cpp. Runs in BOTH
#     build configs (links ${_embed_dep}, same GL-context-fixture pattern as the other embed
#     tests above) since the tested code (bootstrap.cpp/engine.cpp) is always compiled
#     regardless of GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (=
#     build/tests/) -- same pattern as focus_sanity/domrw_sanity/click_callback_sanity
#     (scene-specific rml/rcss pair copied here, addressed by bare filename).
# PT: form_events_sanity -- verifica os 5 callbacks do L1.15-FORMEV (Engine::
#     set_change_callback/set_submit_callback/set_focus_callback/set_blur_callback/
#     set_hover_callback), o código EXATO para o qual tanto glintfx::UiLayer quanto glintfx::App
#     encaminham sem lógica extra -- então este único teste cobre os dois caminhos públicos de
#     consumo, mesmo precedente de focus_sanity.cpp/domrw_sanity.cpp ("dirige o Engine
#     diretamente pelo oráculo interno"). Precisa de Rml::Context::ProcessMouseMove/
#     ProcessMouseButtonDown/ProcessMouseButtonUp/GetHoverElement() como superfície de
#     simulação-de-input/oráculo (não alcançável pelo header público do UiLayer, livre de tipos
#     RmlUi, e o próprio Engine não tem camada de tradução process_event() -- isso é exclusivo
#     do UiLayer), então linka RmlUi::Core explicitamente, mesmo raciocínio de
#     focus_sanity.cpp/domrw_sanity.cpp/document_reload_leak.cpp/engine_smoke.cpp. Roda em AMBAS
#     as configs de build (linka ${_embed_dep}, mesmo padrão de fixture de contexto GL dos
#     outros testes embed acima) já que o código testado (bootstrap.cpp/engine.cpp) é sempre
#     compilado independente de GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- mesmo padrão de
#     focus_sanity/domrw_sanity/click_callback_sanity (par rml/rcss específico da cena copiado
#     aqui, endereçado por nome de arquivo simples).
# ---------------------------------------------------------------------------
add_executable(form_events_sanity form_events_sanity.cpp)
target_link_libraries(form_events_sanity PRIVATE ${_embed_dep} RmlUi::Core)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/form_events_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/form_events_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/form_events_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/form_events_scene.rcss COPYONLY)
add_test(NAME form_events_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:form_events_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: GLINTFX_SANITIZE (TST-L1-MEM, nightly gate) — inject sanitizer runtime
#     options into every test registered so far in this directory. Applied here
#     (bottom of the file, after all add_test() calls above, GLFW=ON and OFF
#     branches included) via DIRECTORY PROPERTY TESTS rather than per-add_test,
#     so no individual test block above needs touching.
#       LSAN_OPTIONS  — suppressions=lsan_suppressions.txt: silence known-leaky
#         prebuilt system libraries (GLFW/Mesa-llvmpipe/FreeType/fontconfig/X11)
#         that are NOT recompiled under GLINTFX_SANITIZE (see CMakeLists.txt
#         top-level comment) so they cannot be fixed by us — never suppress by
#         glintfx function/file.
#       ASAN_OPTIONS  — detect_leaks=1 pinned explicitly (already the Linux
#         default, stated here so the gate does not silently regress if that
#         default ever changes upstream).
#       UBSAN_OPTIONS — print_stacktrace=1 for symbolized aborts; halt_on_error=1
#         belt-and-suspenders with -fno-sanitize-recover=all (compile-time flag,
#         set at the top of CMakeLists.txt) so a UB hit is always a hard test
#         failure, never a printed-and-ignored diagnostic.
# PT: GLINTFX_SANITIZE (TST-L1-MEM, gate noturno) — injeta opções de runtime do
#     sanitizer em todo teste já registrado neste diretório. Aplicado aqui (fim
#     do arquivo, após todas as chamadas add_test() acima, ramos GLFW=ON e OFF
#     inclusos) via DIRECTORY PROPERTY TESTS em vez de por add_test, então
#     nenhum bloco de teste acima precisa ser tocado.
#       LSAN_OPTIONS  — suppressions=lsan_suppressions.txt: silencia bibliotecas
#         de sistema pré-compiladas conhecidas por vazar (GLFW/Mesa-llvmpipe/
#         FreeType/fontconfig/X11) que NÃO são recompiladas sob GLINTFX_SANITIZE
#         (ver comentário no topo do CMakeLists.txt) e portanto não são
#         corrigíveis por nós — nunca suprimir por função/arquivo da glintfx.
#       ASAN_OPTIONS  — detect_leaks=1 fixado explicitamente (já é o default no
#         Linux; declarado aqui para o gate não regredir silenciosamente se
#         esse default mudar upstream).
#       UBSAN_OPTIONS — print_stacktrace=1 para aborts simbolizados;
#         halt_on_error=1 cinto-e-suspensório com -fno-sanitize-recover=all
#         (flag de compile-time, definida no topo do CMakeLists.txt) para que
#         um UB seja sempre falha dura de teste, nunca um diagnóstico impresso
#         e ignorado; suppressions=ubsan_suppressions.txt — mesmo espírito do
#         LSAN_OPTIONS acima, mas para UB real e rastreado em código de
#         terceiro (ver cabeçalho daquele arquivo — hoje só o vptr:ElementDocument
#         do teardown do RmlUi pinado; NUNCA um padrão que também case com
#         código autoral glintfx).
# ---------------------------------------------------------------------------
if(GLINTFX_SANITIZE)
  get_property(_glintfx_all_tests DIRECTORY PROPERTY TESTS)
  set_tests_properties(${_glintfx_all_tests} PROPERTIES ENVIRONMENT
    "LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/lsan_suppressions.txt;ASAN_OPTIONS=detect_leaks=1;UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1:suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ubsan_suppressions.txt")
endif()
