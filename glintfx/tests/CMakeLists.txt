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

# EN: app_process_event_scene.rml/.rcss (A1, framework-2D) -- shared by app_process_event_smoke
#     AND app_frame_callback_smoke (both GLFW-only, WORKING_DIRECTORY=CMAKE_BINARY_DIR,
#     addressed there via "tests/" prefix, same convention as resize_scene.*/element_box_scene.*
#     above). Deliberately NOT click_scene.rml/.rcss -- see the scene's own header comment for
#     why (tab-index:auto needed for the focus+Enter leg, which click_scene's #btn_a lacks).
# PT: app_process_event_scene.rml/.rcss (A1, framework-2D) -- compartilhado por
#     app_process_event_smoke E app_frame_callback_smoke (ambos GLFW-only, WORKING_DIRECTORY=
#     CMAKE_BINARY_DIR, endereçados lá via prefixo "tests/", mesma convenção de
#     resize_scene.*/element_box_scene.* acima). Deliberadamente NÃO click_scene.rml/.rcss --
#     ver o comentário de cabeçalho da própria cena pra entender por quê (tab-index:auto
#     necessário pra perna focus+Enter, que o #btn_a de click_scene não tem).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app_process_event_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/app_process_event_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app_process_event_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/app_process_event_scene.rcss COPYONLY)

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

  # ---------------------------------------------------------------------------
  # EN: app_process_event_smoke (A1, framework-2D) -- verifies App::process_event(const
  #     UiEvent&), the synthetic/replay channel sharing Engine::process_event with the GLFW
  #     physical-input callbacks (window_glfw.cpp) and with UiLayer::process_event. See the
  #     test file's own header for the full oracle (synthetic click, focus+Enter emulated
  #     click, Resize documented no-op). WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide
  #     GLFW-block convention); app_process_event_scene.rml is copied once to
  #     CMAKE_CURRENT_BINARY_DIR above, addressed here as "tests/app_process_event_scene.rml".
  # PT: app_process_event_smoke (A1, framework-2D) -- verifica App::process_event(const
  #     UiEvent&), o canal sintético/replay que compartilha Engine::process_event com os
  #     callbacks físicos GLFW (window_glfw.cpp) e com UiLayer::process_event. Ver o cabeçalho
  #     do próprio arquivo de teste pro oráculo completo (clique sintético, clique emulado por
  #     focus+Enter, Resize no-op documentado). WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção
  #     do bloco GLFW em todo o repo); app_process_event_scene.rml é copiado uma vez para
  #     CMAKE_CURRENT_BINARY_DIR acima, endereçado aqui como
  #     "tests/app_process_event_scene.rml".
  # ---------------------------------------------------------------------------
  add_executable(app_process_event_smoke app_process_event_smoke.cpp)
  target_link_libraries(app_process_event_smoke PRIVATE glintfx)
  add_test(NAME app_process_event_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_process_event_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_frame_callback_smoke (A1, framework-2D) -- verifies App::set_frame_callback(): N
  #     frames of counter/dt sanity, plus the pixel-proof leg (paint inside the hook, snapshot,
  #     confirm the UI's own opaque content painted OVER the hook's colour -- proves both
  #     "scene composes under the UI" and "GL context was current inside the hook"). See the
  #     test file's own header for the full oracle derivation. Links glloader explicitly for
  #     gl_loader.h (glScissor/glClearColor/glClear calls inside the test's own hook lambda);
  #     glloader is a PRIVATE dep of glintfx, not propagated to callers (same rationale as
  #     golden_test/gl_state_guard above). WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide
  #     GLFW-block convention); reuses app_process_event_scene.rml copied above.
  # PT: app_frame_callback_smoke (A1, framework-2D) -- verifica App::set_frame_callback(): N
  #     frames de sanidade de contador/dt, mais a perna de prova por pixel (pinta dentro do
  #     hook, tira snapshot, confirma que o próprio conteúdo opaco da UI pintou POR CIMA da cor
  #     do hook -- prova tanto "a cena compõe sob a UI" quanto "o contexto GL estava corrente
  #     dentro do hook"). Ver o cabeçalho do próprio arquivo de teste pra derivação completa do
  #     oráculo. Linka glloader explicitamente para gl_loader.h (chamadas
  #     glScissor/glClearColor/glClear dentro da própria lambda de hook do teste); glloader é
  #     dep PRIVATE da glintfx, não propagada a callers (mesma racional de
  #     golden_test/gl_state_guard acima). WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do
  #     bloco GLFW em todo o repo); reusa app_process_event_scene.rml copiado acima.
  # ---------------------------------------------------------------------------
  add_executable(app_frame_callback_smoke app_frame_callback_smoke.cpp)
  target_link_libraries(app_frame_callback_smoke PRIVATE glintfx glloader)
  add_test(NAME app_frame_callback_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_frame_callback_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: app_window_mode_smoke (A4-WINMODES, v0.16.0) -- ONE binary, parameterized by the
  #     GLINTFX_TEST_WINDOW_MODE environment variable, registered 5x below (App's N3 contract
  #     forbids 4+ App instances in one process, so per-mode isolation happens at the ctest
  #     level, not inside main() -- see the test file's own header for the full rationale).
  #     Each of the 5 runs constructs ONE App in that mode, proves ok()==true, renders 2 frames,
  #     asserts get_window_size() is sane and snapshot()'s own PPM header dims match it, and
  #     checks the LIVE App::window_mode() query -- exact match for windowed/fullscreen_*,
  #     tolerant of either Maximized or Windowed for "maximized" (Xvfb has no WM to honour the
  #     request -- D4, NOT machine-verifiable, see docs/window-modes.md's manual smoke).
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention, "tests/min.rml").
  # PT: app_window_mode_smoke (A4-WINMODES, v0.16.0) -- UM binário, parametrizado pela variável
  #     de ambiente GLINTFX_TEST_WINDOW_MODE, registrado 5x abaixo (o contrato N3 do App proíbe
  #     4+ instâncias de App num processo, então o isolamento por-modo acontece no nível do
  #     ctest, não dentro do main() -- ver o cabeçalho do próprio arquivo de teste pra racional
  #     completa). Cada uma das 5 corridas constrói UM App naquele modo, prova ok()==true,
  #     renderiza 2 frames, afirma que get_window_size() é são e que as dims do próprio
  #     cabeçalho PPM do snapshot() batem com ele, e checa a query VIVA App::window_mode() --
  #     match exato pra windowed/fullscreen_*, tolerante a Maximized OU Windowed pra "maximized"
  #     (Xvfb não tem WM pra honrar o pedido -- D4, NÃO verificável por máquina, ver a seção de
  #     smoke manual de docs/window-modes.md). WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção
  #     do bloco GLFW em todo o repo, "tests/min.rml").
  # ---------------------------------------------------------------------------
  add_executable(app_window_mode_smoke app_window_mode_smoke.cpp)
  target_link_libraries(app_window_mode_smoke PRIVATE glintfx)
  foreach(_glintfx_wm_case windowed maximized fullscreen_desktop fullscreen_exclusive invalid_config)
    add_test(NAME app_window_mode_smoke_${_glintfx_wm_case}
      COMMAND ${CMAKE_COMMAND}
        -DEXE=$<TARGET_FILE:app_window_mode_smoke>
        -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(app_window_mode_smoke_${_glintfx_wm_case} PROPERTIES
      ENVIRONMENT "GLINTFX_TEST_WINDOW_MODE=${_glintfx_wm_case}")
  endforeach()

  # ---------------------------------------------------------------------------
  # EN: app_window_mode_transitions_smoke (A4-WINMODES, v0.16.0) -- ONE App instance (N3),
  #     cycles App::set_window_mode() through every pairwise transition class D7/D8
  #     (window_glfw.cpp) implements a distinct code path for (Windowed<->Maximized,
  #     Windowed<->Fullscreen, Fullscreen->Maximized's D8-mandatory restore-then-maximize
  #     order), asserting get_window_size() stays sane between EVERY step; then a same-mode
  #     re-request no-op, and 2 out-of-range-enum hardening calls that must leave the live mode
  #     untouched (D5). See the test file's own header for the exact sequence and rationale.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention, "tests/min.rml").
  # PT: app_window_mode_transitions_smoke (A4-WINMODES, v0.16.0) -- UMA instância de App (N3),
  #     cicla App::set_window_mode() por toda classe de transição par-a-par que D7/D8
  #     (window_glfw.cpp) implementa um caminho de código distinto (Windowed<->Maximized,
  #     Windowed<->Fullscreen, a ordem restaurar-depois-maximizar OBRIGATÓRIA pelo D8 de
  #     Fullscreen->Maximized), afirmando que get_window_size() continua são entre TODO passo;
  #     depois um no-op de repedido-do-mesmo-modo, e 2 chamadas de hardening com enum fora de
  #     faixa que precisam deixar o modo vivo intocado (D5). Ver o cabeçalho do próprio arquivo
  #     de teste pra sequência exata e racional. WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção
  #     do bloco GLFW em todo o repo, "tests/min.rml").
  # ---------------------------------------------------------------------------
  add_executable(app_window_mode_transitions_smoke app_window_mode_transitions_smoke.cpp)
  target_link_libraries(app_window_mode_transitions_smoke PRIVATE glintfx)
  add_test(NAME app_window_mode_transitions_smoke
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:app_window_mode_transitions_smoke>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # ---------------------------------------------------------------------------
  # EN: glfw_event_translate_sanity (A1, framework-2D) -- PURE unit test for
  #     glfw_event_translate.hpp: no window, no GL context, no glfwInit() call anywhere in the
  #     test (only GLFW #define constants are used). Runs the executable DIRECTLY, not through
  #     run_xvfb.cmake, unlike every other test in this file -- there is nothing display-
  #     related to isolate here. Links `glfw` for the GLFW header/constants (the imported
  #     target is header-only at this call site: no glfwInit()/window/context is ever created)
  #     and `glintfx` for glintfx/ui_event.hpp's public Key/UiEvent/Mod types. See the test
  #     file's own header for the full "why this file must exist" rationale (the "fórmula de
  #     conversão sem teste de delta" bug class) and the mutation-testability note for the
  #     adversarial reviewer.
  # PT: glfw_event_translate_sanity (A1, framework-2D) -- teste unit PURO para
  #     glfw_event_translate.hpp: sem janela, sem contexto GL, sem nenhuma chamada a
  #     glfwInit() no teste (só constantes #define do GLFW são usadas). Roda o executável
  #     DIRETO, não através de run_xvfb.cmake, diferente de todo outro teste deste arquivo --
  #     não há nada relacionado a display pra isolar aqui. Linka `glfw` pelo header/constantes
  #     GLFW (o alvo importado é header-only neste call site: nenhum glfwInit()/janela/contexto
  #     é criado nunca) e `glintfx` pelos tipos públicos Key/UiEvent/Mod de
  #     glintfx/ui_event.hpp. Ver o cabeçalho do próprio arquivo de teste pra racional completa
  #     de "por que este arquivo precisa existir" (a classe de bug "fórmula de conversão sem
  #     teste de delta") e a nota de mutation-testability pro reviewer adversarial.
  # ---------------------------------------------------------------------------
  add_executable(glfw_event_translate_sanity glfw_event_translate_sanity.cpp)
  target_link_libraries(glfw_event_translate_sanity PRIVATE glintfx glfw)
  add_test(NAME glfw_event_translate_sanity COMMAND glfw_event_translate_sanity)

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

  # ---------------------------------------------------------------------------
  # EN: fonteng_own_sanity (L1.19-FONTENG, FT-F3) — minimal, engine-agnostic visual oracle:
  #     renders tests/fonteng_scene.rml (3 static text lines, incl. pt-br-accented Latin-1
  #     Supplement) and checks coarse pixel statistics prove legible GLYPHS rendered (not
  #     nothing, not a filled block), each line's own band included. Runs UNCONDITIONALLY
  #     (not gated behind GLINTFX_OWN_FONT_ENGINE) -- the oracle is deliberately symmetric:
  #     it must pass whichever font engine is active (RmlUi's FreeType default, OFF; or
  #     glintfx's own SOV-SFNT/SOV-RAST engine, ON), proving the A/B gate is truly a drop-in
  #     swap. See fonteng_own_sanity.cpp's own header for the full check-by-check rationale.
  #     Reuses tests/assets/OpenSans-Regular.ttf, already copied by the showcase block above.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention, "tests/" prefix).
  # PT: fonteng_own_sanity (L1.19-FONTENG, FT-F3) — oráculo visual mínimo, agnóstico de motor:
  #     renderiza tests/fonteng_scene.rml (3 linhas de texto estático, incl. Latin-1
  #     Supplement acentuado pt-br) e checa estatísticas grosseiras de pixel que provam GLYPHS
  #     legíveis renderizados (nem nada, nem um bloco preenchido), com a banda de cada linha
  #     incluída. Roda INCONDICIONALMENTE (não bloqueado atrás de GLINTFX_OWN_FONT_ENGINE) --
  #     o oráculo é deliberadamente simétrico: precisa passar com qualquer motor de fonte
  #     ativo (default FreeType do RmlUi, OFF; ou o motor próprio SOV-SFNT/SOV-RAST do
  #     glintfx, ON), provando que o gate A/B é de fato uma troca drop-in. Ver o cabeçalho
  #     próprio de fonteng_own_sanity.cpp pra racional completa cheque-a-cheque. Reusa
  #     tests/assets/OpenSans-Regular.ttf, já copiado pelo bloco showcase acima.
  #     WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo, prefixo
  #     "tests/").
  # ---------------------------------------------------------------------------
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_scene.rml COPYONLY)

  add_executable(fonteng_own_sanity fonteng_own_sanity.cpp)
  target_link_libraries(fonteng_own_sanity PRIVATE glintfx)
  add_test(NAME fonteng_own_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_own_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

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
# EN: fonteng_own_embed_sanity (L1.19-FONTENG, FT-F3, review IMPORTANT #1) -- the UiLayer/
#     embed-mode analogue of fonteng_own_sanity (GLFW-only, glintfx::App path, registered inside
#     if(GLINTFX_BACKEND_GLFW) far above). GusWorld -- the ONLY real consumer of glintfx --
#     attaches via UiLayer in an embed-only build (GLINTFX_BACKEND_GLFW=OFF); before this test
#     NOTHING in the tracked suite exercised the font engine's glyph-rasterisation pipeline
#     (LoadFontFace -> GetFontFaceHandle -> GenerateString) on that specific path (reviewer's ad
#     hoc probe confirmed it works, but ad hoc probes are not CI regression protection).
#     Registered here (OUTSIDE if(GLINTFX_BACKEND_GLFW), AFTER ${_embed_dep} is defined above by
#     the "Embed-mode tests" section) so it builds and runs in BOTH configs: GLFW=ON (glintfx
#     already contains window_glfw.cpp) and embed-only GLFW=OFF (the _glintfx_test_ctx helper
#     provides the GL-context test fixture, same as every other embed test in this file).
#     Gated behind GLINTFX_OWN_FONT_ENGINE (unlike fonteng_own_sanity, which is deliberately
#     engine-agnostic and unconditional): this test's whole purpose is proving glintfx's own
#     SOV-SFNT/SOV-RAST engine specifically THROUGH the UiLayer path -- a FreeType-through-
#     UiLayer variant is out of this ticket's scope.
#     Font asset: demos/showcase/assets/ (contains OpenSans-Regular.ttf) is copied to
#     ${CMAKE_CURRENT_BINARY_DIR} (= build/tests/) HERE, unconditionally with respect to
#     GLINTFX_BACKEND_GLFW, whenever GLINTFX_OWN_FONT_ENGINE=ON -- review finding: until this
#     block, ONLY the showcase file(COPY) inside if(GLINTFX_BACKEND_GLFW) (line ~308 above) ever
#     placed that font in the build tree, so an embed-only build (GLFW=OFF) never had it
#     available. file(COPY) is idempotent, so when GLFW=ON this simply re-copies the same
#     already-present files (no conflict with the showcase block above).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- repo-wide embed-test
#     convention (same as ui_layer_sanity/dp_ratio_sanity/data_model_embed_sanity below/above);
#     fonteng_embed_scene.rml's @font-face src is "assets/OpenSans-Regular.ttf", relative to
#     that same directory. Links glloader explicitly: the test calls glReadPixels/glClear/
#     glBindFramebuffer directly (same rationale as ui_layer_sanity above).
# PT: fonteng_own_embed_sanity (L1.19-FONTENG, FT-F3, IMPORTANTE #1 do review) -- o análogo em
#     modo UiLayer/embed do fonteng_own_sanity (só-GLFW, caminho glintfx::App, registrado dentro
#     de if(GLINTFX_BACKEND_GLFW) bem acima). O GusWorld -- ÚNICO consumidor real do glintfx --
#     anexa via UiLayer num build embed-only (GLINTFX_BACKEND_GLFW=OFF); antes deste teste NADA
#     na suíte rastreada exercitava o pipeline de rasterização de glyph do motor de fonte
#     (LoadFontFace -> GetFontFaceHandle -> GenerateString) nesse caminho específico (a sonda ad
#     hoc do reviewer confirmou que funciona, mas sondas ad hoc não são proteção de regressão de
#     CI).
#     Registrado aqui (FORA de if(GLINTFX_BACKEND_GLFW), DEPOIS de ${_embed_dep} ser definido
#     acima pela seção "Embed-mode tests") para buildar e rodar nas DUAS configs: GLFW=ON
#     (glintfx já contém window_glfw.cpp) e embed-only GLFW=OFF (o helper _glintfx_test_ctx
#     provê o fixture de contexto GL, mesmo padrão de todo outro teste embed deste arquivo).
#     Bloqueado atrás de GLINTFX_OWN_FONT_ENGINE (diferente de fonteng_own_sanity, que é
#     deliberadamente agnóstico de motor e incondicional): o propósito inteiro deste teste é
#     provar especificamente o motor próprio SOV-SFNT/SOV-RAST do glintfx ATRAVÉS do caminho
#     UiLayer -- uma variante FreeType-via-UiLayer é fora do escopo desta tarefa.
#     Asset de fonte: demos/showcase/assets/ (contém OpenSans-Regular.ttf) é copiado para
#     ${CMAKE_CURRENT_BINARY_DIR} (= build/tests/) AQUI, incondicional em relação a
#     GLINTFX_BACKEND_GLFW, sempre que GLINTFX_OWN_FONT_ENGINE=ON -- achado do review: até este
#     bloco, SÓ o file(COPY) do showcase dentro de if(GLINTFX_BACKEND_GLFW) (linha ~308 acima)
#     colocava essa fonte na árvore de build, então um build embed-only (GLFW=OFF) nunca a tinha
#     disponível. file(COPY) é idempotente, então com GLFW=ON isto apenas recopia os mesmos
#     arquivos já presentes (sem conflito com o bloco showcase acima).
#     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (= build/tests/) -- convenção repo-wide de
#     teste embed (mesma de ui_layer_sanity/dp_ratio_sanity/data_model_embed_sanity abaixo/
#     acima); o src de @font-face do fonteng_embed_scene.rml é "assets/OpenSans-Regular.ttf",
#     relativo a esse mesmo diretório. Linka glloader explicitamente: o teste chama
#     glReadPixels/glClear/glBindFramebuffer diretamente (mesmo racional de ui_layer_sanity
#     acima).
# ---------------------------------------------------------------------------
if(GLINTFX_OWN_FONT_ENGINE)
  file(COPY ${CMAKE_SOURCE_DIR}/demos/showcase/assets
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  add_executable(fonteng_own_embed_sanity fonteng_own_embed_sanity.cpp)
  target_link_libraries(fonteng_own_embed_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_embed_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_embed_scene.rml COPYONLY)
  add_test(NAME fonteng_own_embed_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_own_embed_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_own_kern_sanity (L1.19-FONTENG/L1.20-FONTFLIP, kerning-application amadurecimento)
  #     -- NUMERIC width oracle (not visual/pixel) proving FontEngineOwn::IterateGlyphs()
  #     actually APPLIES SOV-SFNT's glx_sfnt_kern() adjustment to the pen, via
  #     UiLayer::get_element_box(id).w on tests/fonteng_kern_scene.rml's `display: inline-block`
  #     spans. Same registration pattern as fonteng_own_embed_sanity right above (OUTSIDE
  #     if(GLINTFX_BACKEND_GLFW), via ${_embed_dep}) so it builds and runs identically whichever
  #     way GLINTFX_BACKEND_GLFW is set -- proving the kern fold on both the GLFW-hosted-App CI
  #     path and GusWorld's own embed-only (UiLayer, no glintfx::App) consumption path. See the
  #     .cpp's own file header for the full derivation of both assertions (kerned pair "AV" must
  #     measure >= 2px tighter than the sum of its parts; un-kerned control pair "HH" must match
  #     the sum of its parts within a documented +-1px rounding-carry tolerance).
  # PT: fonteng_own_kern_sanity (amadurecimento de aplicação-de-kerning do L1.19-FONTENG/
  #     L1.20-FONTFLIP) -- oráculo NUMÉRICO de largura (não visual/de-pixel) provando que
  #     FontEngineOwn::IterateGlyphs() de fato APLICA o ajuste glx_sfnt_kern() do SOV-SFNT na
  #     pena, via UiLayer::get_element_box(id).w nos spans `display: inline-block` de
  #     tests/fonteng_kern_scene.rml. Mesmo padrão de registro de fonteng_own_embed_sanity logo
  #     acima (FORA de if(GLINTFX_BACKEND_GLFW), via ${_embed_dep}) para buildar e rodar
  #     identicamente com qualquer valor de GLINTFX_BACKEND_GLFW -- provando a dobra de kern
  #     tanto no caminho de CI hospedado-por-GLFW-App quanto no próprio caminho de consumo
  #     embed-only do GusWorld (UiLayer, sem glintfx::App). Ver o cabeçalho próprio do .cpp pra
  #     derivação completa das duas asserções (o par com kern "AV" precisa medir >= 2px mais
  #     apertado que a soma das partes; o par de controle sem kern "HH" precisa bater com a soma
  #     das partes dentro de uma tolerância documentada de +-1px de carregamento de
  #     arredondamento).
  # -------------------------------------------------------------------------
  add_executable(fonteng_own_kern_sanity fonteng_own_kern_sanity.cpp)
  target_link_libraries(fonteng_own_kern_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_kern_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_kern_scene.rml COPYONLY)
  add_test(NAME fonteng_own_kern_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_own_kern_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_ab_compare (L1.20-FONTFLIP, FT-F4) -- the A/B SAFETY NET for "the flip": renders
  #     the SAME document (fonteng_ab_scene.rml) through BOTH the own SOV-SFNT/SOV-RAST engine and
  #     RmlUi's FreeType engine, in ONE GLINTFX_OWN_FONT_ENGINE=ON build, and compares per-span
  #     advance width + ink coverage numerically -- flagging GROSS divergence (missing glyph,
  #     2x-wrong advance, broken pt-br composite-accent outline) before the own engine becomes the
  #     default. Runtime engine selection is via the src-internal own_font_engine_ab_bypass() hook
  #     (font_engine_own.hpp), read once per session by Bootstrap::init(). Same registration
  #     pattern as fonteng_own_kern_sanity / fonteng_own_embed_sanity right above (OUTSIDE
  #     if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader) so it builds and runs identically
  #     with GLINTFX_BACKEND_GLFW ON or OFF -- exercising the A/B on both the GLFW-hosted-App CI
  #     path and GusWorld's own embed-only (UiLayer) consumption path. The assets/ font dir is
  #     already copied to CMAKE_CURRENT_BINARY_DIR by the fonteng_own_embed_sanity block above; the
  #     scene's @font-face src ("assets/OpenSans-Regular.ttf") resolves against the shared
  #     WORKING_DIRECTORY (= build/tests/). See fonteng_ab_compare.cpp's own header for the full
  #     metric/tolerance derivation.
  # PT: fonteng_ab_compare (L1.20-FONTFLIP, FT-F4) -- a REDE DE SEGURANÇA A/B para "o flip":
  #     renderiza o MESMO documento (fonteng_ab_scene.rml) através TANTO do motor próprio
  #     SOV-SFNT/SOV-RAST QUANTO do motor FreeType do RmlUi, num único build
  #     GLINTFX_OWN_FONT_ENGINE=ON, e compara largura de avanço + cobertura de tinta por-span
  #     numericamente -- flagrando divergência GROSSEIRA (glyph faltando, avanço 2x errado, outline
  #     de acento composto pt-br quebrado) antes de o motor próprio virar o default. A seleção de
  #     motor em runtime é via o hook src-interno own_font_engine_ab_bypass() (font_engine_own.hpp),
  #     lido uma vez por sessão pelo Bootstrap::init(). Mesmo padrão de registro de
  #     fonteng_own_kern_sanity / fonteng_own_embed_sanity logo acima (FORA de
  #     if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader) para buildar e rodar identicamente
  #     com GLINTFX_BACKEND_GLFW ON ou OFF -- exercitando o A/B tanto no caminho de CI
  #     hospedado-por-GLFW-App quanto no próprio caminho de consumo embed-only (UiLayer) do
  #     GusWorld. O dir de fonte assets/ já é copiado pra CMAKE_CURRENT_BINARY_DIR pelo bloco
  #     fonteng_own_embed_sanity acima; o src de @font-face da cena ("assets/OpenSans-Regular.ttf")
  #     resolve contra o WORKING_DIRECTORY compartilhado (= build/tests/). Ver o cabeçalho próprio
  #     de fonteng_ab_compare.cpp pra derivação completa de métrica/tolerância.
  # -------------------------------------------------------------------------
  add_executable(fonteng_ab_compare fonteng_ab_compare.cpp)
  target_link_libraries(fonteng_ab_compare PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_ab_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_ab_scene.rml COPYONLY)
  add_test(NAME fonteng_ab_compare
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_ab_compare>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_engine_select_sanity (L1.20-FONTFLIP Phase 2/3, FT-F4 QA gate) -- proves the
  #     PUBLIC UiLayerConfig::font_engine field (glintfx/include/glintfx/font_engine.hpp) itself
  #     selects the engine, end-to-end -- unlike fonteng_ab_compare right above (which drives the
  #     engine via the src-internal own_font_engine_ab_bypass() test hook), this file uses ONLY
  #     the field a real consumer sets. Reuses fonteng_ab_scene.rml (already copied to
  #     CMAKE_CURRENT_BINARY_DIR by the fonteng_ab_compare block above) and the same
  #     hidden-GLFW-host-window fixture. Same registration pattern (OUTSIDE
  #     if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader) so it builds and runs identically
  #     with GLINTFX_BACKEND_GLFW ON or OFF. See fonteng_engine_select_sanity.cpp's own header
  #     for the full two-proof (rollback-renders / field-is-not-a-no-op) rationale.
  # PT: fonteng_engine_select_sanity (L1.20-FONTFLIP Fase 2/3, gate de QA do FT-F4) -- prova que o
  #     campo PÚBLICO UiLayerConfig::font_engine (glintfx/include/glintfx/font_engine.hpp) em si
  #     seleciona o motor, ponta-a-ponta -- diferente do fonteng_ab_compare logo acima (que dirige
  #     o motor pelo hook src-interno só-de-teste own_font_engine_ab_bypass()), este arquivo usa
  #     SÓ o campo que um consumidor real define. Reusa fonteng_ab_scene.rml (já copiado para
  #     CMAKE_CURRENT_BINARY_DIR pelo bloco fonteng_ab_compare acima) e o mesmo fixture de janela-
  #     host-GLFW-oculta. Mesmo padrão de registro (FORA de if(GLINTFX_BACKEND_GLFW), via
  #     ${_embed_dep} + glloader) para buildar e rodar identicamente com GLINTFX_BACKEND_GLFW ON
  #     ou OFF. Ver o cabeçalho próprio de fonteng_engine_select_sanity.cpp pra racional completa
  #     das duas provas (rollback-renderiza / campo-não-é-no-op).
  # -------------------------------------------------------------------------
  add_executable(fonteng_engine_select_sanity fonteng_engine_select_sanity.cpp)
  target_link_libraries(fonteng_engine_select_sanity PRIVATE ${_embed_dep} glloader)
  add_test(NAME fonteng_engine_select_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_engine_select_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_ab_visual (L1.20-FONTFLIP, FT-F4) -- the VISUAL sibling of fonteng_ab_compare: a
  #     DIAGNOSTIC harness (NOT a ctest gate -- deliberately NO add_test) that renders the SAME
  #     phrase through BOTH the own SOV-SFNT/SOV-RAST engine and RmlUi's FreeType engine, at four
  #     small body sizes (11/13/16/24 dp) and two dp_ratios (1.0 low-DPI, 2.0 HiDPI), and dumps
  #     aligned per-line PPM crops so a human can make the perceptual sharpness call the numeric
  #     fonteng_ab_compare cannot. Same registration shape as fonteng_ab_compare right above
  #     (add_executable + the scene copied to CMAKE_CURRENT_BINARY_DIR; the assets/ font dir is
  #     already copied there by the fonteng_own_embed_sanity block). It is built but NOT run by
  #     ctest -- run it by hand under xvfb-run, passing an output directory as argv[1]. See
  #     fonteng_ab_visual.cpp's own header for the full rationale.
  # PT: fonteng_ab_visual (L1.20-FONTFLIP, FT-F4) -- o irmão VISUAL do fonteng_ab_compare: um
  #     harness de DIAGNÓSTICO (NÃO um gate do ctest -- de propósito SEM add_test) que renderiza a
  #     MESMA frase através TANTO do motor próprio SOV-SFNT/SOV-RAST QUANTO do motor FreeType do
  #     RmlUi, em quatro tamanhos de corpo pequenos (11/13/16/24 dp) e dois dp_ratios (1.0 baixa-DPI,
  #     2.0 HiDPI), e despeja recortes PPM por-linha alinhados para que um humano faça a decisão
  #     perceptual de nitidez que o numérico fonteng_ab_compare não faz. Mesmo formato de registro do
  #     fonteng_ab_compare logo acima (add_executable + a cena copiada para CMAKE_CURRENT_BINARY_DIR;
  #     o dir de fonte assets/ já é copiado lá pelo bloco fonteng_own_embed_sanity). É buildado mas
  #     NÃO rodado pelo ctest -- rode-o à mão sob xvfb-run, passando um diretório de saída como
  #     argv[1]. Ver o cabeçalho próprio de fonteng_ab_visual.cpp para a justificativa completa.
  # -------------------------------------------------------------------------
  add_executable(fonteng_ab_visual fonteng_ab_visual.cpp)
  target_link_libraries(fonteng_ab_visual PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_ab_visual_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_ab_visual_scene.rml COPYONLY)

  # -------------------------------------------------------------------------
  # EN: fonteng_perf_bench (L1.20-PERFBENCH, FT-F4 perf gate) -- the PERFORMANCE sibling of
  #     fonteng_ab_visual right above: a DIAGNOSTIC harness (NOT a ctest gate -- deliberately NO
  #     add_test, same convention as fonteng_ab_visual) that measures the own SOV-SFNT/SOV-RAST
  #     engine's CPU/memory cost against RmlUi's FreeType engine across four axes (bake/layout/
  #     frame/mem), consumer-driven by the GusWorld's weak-hardware perf condition for flipping
  #     GLINTFX_OWN_FONT_ENGINE's default. Run by hand under xvfb-run, one
  #     --engine/--axis/--font/--dp combination per process -- see fonteng_perf_bench.cpp's own
  #     header for the full per-axis method. Same registration shape as fonteng_ab_visual (OUTSIDE
  #     if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader); the assets/ font dir (both
  #     OpenSans-Regular.ttf AND PixelOperatorMono.ttf) is already copied to
  #     CMAKE_CURRENT_BINARY_DIR by the fonteng_own_embed_sanity block at the top of this
  #     GLINTFX_OWN_FONT_ENGINE block.
  # PT: fonteng_perf_bench (L1.20-PERFBENCH, gate de perf do FT-F4) -- o irmão de PERFORMANCE do
  #     fonteng_ab_visual logo acima: um harness de DIAGNÓSTICO (NÃO um gate do ctest --
  #     propositalmente SEM add_test, mesma convenção do fonteng_ab_visual) que mede o custo de
  #     CPU/memória do motor próprio SOV-SFNT/SOV-RAST contra o motor FreeType do RmlUi em quatro
  #     eixos (bake/layout/frame/mem), consumer-driven pela condição de perf em hardware fraco do
  #     GusWorld para virar o default do GLINTFX_OWN_FONT_ENGINE. Rode à mão sob xvfb-run, uma
  #     combinação --engine/--axis/--font/--dp por processo -- ver o cabeçalho próprio de
  #     fonteng_perf_bench.cpp pro método completo por-eixo. Mesmo formato de registro do
  #     fonteng_ab_visual (FORA de if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader); o dir
  #     de fonte assets/ (TANTO OpenSans-Regular.ttf QUANTO PixelOperatorMono.ttf) já é copiado
  #     pra CMAKE_CURRENT_BINARY_DIR pelo bloco fonteng_own_embed_sanity no topo deste bloco
  #     GLINTFX_OWN_FONT_ENGINE.
  # -------------------------------------------------------------------------
  add_executable(fonteng_perf_bench fonteng_perf_bench.cpp)
  target_link_libraries(fonteng_perf_bench PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_perf_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_perf_scene.rml COPYONLY)

  # -------------------------------------------------------------------------
  # EN: fonteng_extended_sanity (L1.20-FONTFLIP, FT-F4, sub-phase 2.1) -- regression net for the
  #     WIDENED bake set. Renders, through the OWN engine, spans containing ONLY the codepoints
  #     sub-phase 2.1 added to FontEngineOwn::BuildBakeSet() (î U+00EE, em/en-dash, curved quotes,
  #     ellipsis, bullet) -- no ASCII filler -- and asserts each draws real ink, so a glyph
  #     regressing out of the set is caught mechanically. Same registration shape as
  #     fonteng_ab_compare right above (OUTSIDE if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} +
  #     glloader), so it builds and runs with GLINTFX_BACKEND_GLFW ON or OFF; the assets/ font dir
  #     is already copied to CMAKE_CURRENT_BINARY_DIR by the top of this GLINTFX_OWN_FONT_ENGINE
  #     block, and the scene's @font-face src resolves against the shared WORKING_DIRECTORY
  #     (= build/tests/). See fonteng_extended_sanity.cpp's own header for the full method.
  # PT: fonteng_extended_sanity (L1.20-FONTFLIP, FT-F4, sub-fase 2.1) -- rede de regressão do
  #     conjunto empacotado AMPLIADO. Renderiza, pelo motor PRÓPRIO, spans contendo SÓ os codepoints
  #     que a sub-fase 2.1 adicionou ao FontEngineOwn::BuildBakeSet() (î U+00EE, em/en-dash, aspas
  #     curvas, reticências, bullet) -- sem enchimento ASCII -- e verifica que cada um desenha tinta
  #     real, então um glyph regredindo pra fora do conjunto é pego mecanicamente. Mesmo formato de
  #     registro de fonteng_ab_compare logo acima (FORA de if(GLINTFX_BACKEND_GLFW), via
  #     ${_embed_dep} + glloader), para buildar e rodar com GLINTFX_BACKEND_GLFW ON ou OFF; o dir de
  #     fonte assets/ já é copiado pra CMAKE_CURRENT_BINARY_DIR pelo topo deste bloco
  #     GLINTFX_OWN_FONT_ENGINE, e o src de @font-face da cena resolve contra o WORKING_DIRECTORY
  #     compartilhado (= build/tests/). Ver o cabeçalho próprio de fonteng_extended_sanity.cpp pro
  #     método completo.
  # -------------------------------------------------------------------------
  add_executable(fonteng_extended_sanity fonteng_extended_sanity.cpp)
  target_link_libraries(fonteng_extended_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_extended_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_extended_scene.rml COPYONLY)
  add_test(NAME fonteng_extended_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_extended_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_lazy_sanity (L1.20-FONTFLIP, FT-F4, sub-phase 2B) -- INDEPENDENT ADVERSARIAL GATE
  #     for the lazy on-demand bake (FontEngineOwn::EnsureGlyphs()/BakeGlyph()/GrowAtlas(), commit
  #     7f119f1), written by the QA reviewer (not the implementer). Renders Greek/Cyrillic
  #     codepoints OUTSIDE the eager warm set to prove non-tofu lazy bake, forces a GUARANTEED
  #     GrowAtlas() call via a runtime set_text() top-up and re-measures two canary glyphs baked
  #     BEFORE that growth to prove no corruption, checks a lazy multi-glyph string's layout width
  #     against its own rendered mesh extent, and exercises a codepoint the face lacks (CJK) through
  #     the negative-cache path without crashing. Same registration shape as fonteng_extended_sanity
  #     right above (OUTSIDE if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader), so it builds
  #     and runs with GLINTFX_BACKEND_GLFW ON or OFF; the assets/ font dir is already copied to
  #     CMAKE_CURRENT_BINARY_DIR by the top of this GLINTFX_OWN_FONT_ENGINE block. See
  #     fonteng_lazy_sanity.cpp's own header for the full six-criterion method.
  # PT: fonteng_lazy_sanity (L1.20-FONTFLIP, FT-F4, sub-fase 2B) -- GATE ADVERSARIAL INDEPENDENTE
  #     pro empacotamento preguiçoso sob demanda (FontEngineOwn::EnsureGlyphs()/BakeGlyph()/
  #     GrowAtlas(), commit 7f119f1), escrito pelo revisor de QA (não o implementador). Renderiza
  #     codepoints Grego/Cirílico FORA do conjunto ávido pra provar empacotamento preguiçoso
  #     não-tofu, força uma chamada GARANTIDA de GrowAtlas() via um top-up set_text() em runtime e
  #     remede dois glyphs canário empacotados ANTES daquele crescimento pra provar ausência de
  #     corrupção, checa a largura de layout de uma string preguiçosa multi-glyph contra a própria
  #     extensão renderizada do mesh, e exercita um codepoint que a face não tem (CJK) pelo caminho
  #     de cache negativo sem crashar. Mesmo formato de registro de fonteng_extended_sanity logo
  #     acima (FORA de if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader), para buildar e rodar
  #     com GLINTFX_BACKEND_GLFW ON ou OFF; o dir de fonte assets/ já é copiado pra
  #     CMAKE_CURRENT_BINARY_DIR pelo topo deste bloco GLINTFX_OWN_FONT_ENGINE. Ver o cabeçalho
  #     próprio de fonteng_lazy_sanity.cpp pro método completo dos seis critérios.
  # -------------------------------------------------------------------------
  add_executable(fonteng_lazy_sanity fonteng_lazy_sanity.cpp)
  target_link_libraries(fonteng_lazy_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_lazy_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_lazy_scene.rml COPYONLY)
  add_test(NAME fonteng_lazy_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_lazy_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_dedup_sanity (L1.20-FONTFLIP, FT-F4, pre-flip FIX) -- regression net for the
  #     RegisterFace() face-registration LEAK confirmed by adversarial review and fixed the same
  #     session (src/font_engine_own.cpp/.hpp): reloading the SAME document N times used to push N
  #     duplicate LoadedFace blobs (~214KB/reload for Open Sans, unbounded over a session) with no
  #     dedup; RegisterFace() now dedups by (family,style,weight,blob-content) before allocating,
  #     mirroring the pinned RmlUi source's own FontProvider::AddFace()/FontFamily::AddFace()
  #     "Duplicate" outcome. This test reloads fonteng_embed_scene.rml (already copied above by the
  #     fonteng_own_embed_sanity block; its assets/ font dir too) 50 times in one UiLayer session
  #     and asserts the src-internal DEBUG hook own_font_engine_debug_registered_face_count()
  #     (font_engine_own.hpp/.cpp) is EXACTLY 1 after the first load and stays EXACTLY 1 after all
  #     50 (an exact-integer oracle, not a tolerance -- the pre-fix code would have measured 50),
  #     plus an ink check on the final reload proving the deduped face still renders. Same
  #     registration shape as fonteng_lazy_sanity right above (OUTSIDE if(GLINTFX_BACKEND_GLFW),
  #     via ${_embed_dep} + glloader). See fonteng_dedup_sanity.cpp's own header for the full method.
  # PT: fonteng_dedup_sanity (L1.20-FONTFLIP, FT-F4, FIX pré-flip) -- rede de regressão do
  #     VAZAMENTO de registro de face do RegisterFace(), confirmado por review adversarial e
  #     corrigido na mesma sessão (src/font_engine_own.cpp/.hpp): recarregar o MESMO documento N
  #     vezes empilhava N blobs LoadedFace duplicados (~214KB/reload pra Open Sans, sem limite ao
  #     longo de uma sessão) sem dedup; o RegisterFace() agora deduplica por
  #     (family,style,weight,conteúdo-do-blob) antes de alocar, espelhando o próprio resultado
  #     "Duplicate" do FontProvider::AddFace()/FontFamily::AddFace() do source pinado do RmlUi.
  #     Este teste recarrega fonteng_embed_scene.rml (já copiado acima pelo bloco
  #     fonteng_own_embed_sanity; o dir de fonte assets/ também) 50 vezes numa única sessão UiLayer
  #     e verifica que o hook de DEBUG src-interno
  #     own_font_engine_debug_registered_face_count() (font_engine_own.hpp/.cpp) é EXATAMENTE 1
  #     após a 1ª carga e permanece EXATAMENTE 1 após as 50 (oráculo de inteiro exato, não
  #     tolerância -- o código pré-fix teria medido 50), mais um cheque de tinta no reload final
  #     provando que a face deduplicada ainda renderiza. Mesmo formato de registro de
  #     fonteng_lazy_sanity logo acima (FORA de if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} +
  #     glloader). Ver o cabeçalho próprio de fonteng_dedup_sanity.cpp pro método completo.
  # -------------------------------------------------------------------------
  add_executable(fonteng_dedup_sanity fonteng_dedup_sanity.cpp)
  target_link_libraries(fonteng_dedup_sanity PRIVATE ${_embed_dep} glloader)
  # EN: fonteng_embed_scene.rml + assets/ already copied to CMAKE_CURRENT_BINARY_DIR by the
  #     fonteng_own_embed_sanity block above -- no further configure_file()/file(COPY) needed.
  # PT: fonteng_embed_scene.rml + assets/ já copiados pra CMAKE_CURRENT_BINARY_DIR pelo bloco
  #     fonteng_own_embed_sanity acima -- sem configure_file()/file(COPY) adicional necessário.
  add_test(NAME fonteng_dedup_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_dedup_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_fallback_sanity (L1.20-FONTFLIP, FT-F4, per-glyph fallback) -- INDEPENDENT
  #     ADVERSARIAL GATE for FontEngineOwn::fallback_faces_/BakeGlyph()/RasterizeGlyph()'s `src`
  #     parameter (commit 957505b), written by the QA reviewer (not the implementer). Renders
  #     Greek + superscript-2 codepoints "Primary" (PixelOperatorMono) genuinely lacks through a
  #     TWO-candidate fallback list (an incapable candidate registered first, the capable Open Sans
  #     one second) to prove non-tofu resolution + walk-past-a-non-resolving-candidate; compares a
  #     fallback-resolved glyph's metrics against the SAME glyph baked directly on its own source
  #     face to catch the units_per_em-mismatch CRITICAL bug class the review brief names; checks
  #     width==mesh for a mixed Primary+fallback string (cross-face kern must be a no-op) and for a
  #     same-face pair reached via the fallback family directly (kern DOES apply, mesh must still
  #     match); exercises the CJK negative-cache path; and reloads the document under this SAME
  #     UiLayer session to exercise RegisterFace()'s dedup + BakeGlyph()'s fallback path repeatedly
  #     (meaningful under -DGLINTFX_SANITIZE=ON). Same registration shape as fonteng_dedup_sanity
  #     right above (OUTSIDE if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader); the assets/
  #     font dir is already copied to CMAKE_CURRENT_BINARY_DIR by the top of this
  #     GLINTFX_OWN_FONT_ENGINE block (includes assets/PixelOperatorMono.ttf). See
  #     fonteng_fallback_sanity.cpp's own header for the full seven-criterion method.
  # PT: fonteng_fallback_sanity (L1.20-FONTFLIP, FT-F4, fallback por-glyph) -- GATE ADVERSARIAL
  #     INDEPENDENTE pro FontEngineOwn::fallback_faces_/BakeGlyph()/o parâmetro `src` do
  #     RasterizeGlyph() (commit 957505b), escrito pelo revisor de QA (não o implementador).
  #     Renderiza codepoints Grego + superscript-2 que a "Primary" (PixelOperatorMono) de fato não
  #     tem através de uma lista de DOIS candidatos de fallback (um incapaz registrado primeiro, a
  #     Open Sans capaz em segundo) pra provar resolução não-tofu + caminhada-além-de-um-candidato-
  #     que-não-resolve; compara as métricas de um glyph resolvido por fallback contra o MESMO glyph
  #     empacotado direto na própria face-fonte pra pegar a classe de bug CRÍTICA de mismatch-de-
  #     units_per_em que a tarefa de review nomeia; checa largura==mesh pra uma string mista
  #     Primary+fallback (kern entre faces precisa ser no-op) e pra um par da mesma face alcançado
  #     pela família de fallback direto (kern SE aplica, mesh ainda precisa bater); exercita o
  #     caminho de cache-negativo CJK; e recarrega o documento nesta MESMA sessão UiLayer pra
  #     exercitar o dedup do RegisterFace() + o caminho de fallback do BakeGlyph() repetidamente
  #     (relevante sob -DGLINTFX_SANITIZE=ON). Mesmo formato de registro de fonteng_dedup_sanity
  #     logo acima (FORA de if(GLINTFX_BACKEND_GLFW), via ${_embed_dep} + glloader); o dir de fonte
  #     assets/ já é copiado pra CMAKE_CURRENT_BINARY_DIR pelo topo deste bloco
  #     GLINTFX_OWN_FONT_ENGINE (inclui assets/PixelOperatorMono.ttf). Ver o cabeçalho próprio de
  #     fonteng_fallback_sanity.cpp pro método completo dos sete critérios.
  # -------------------------------------------------------------------------
  add_executable(fonteng_fallback_sanity fonteng_fallback_sanity.cpp)
  target_link_libraries(fonteng_fallback_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_fallback_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_fallback_scene.rml COPYONLY)
  add_test(NAME fonteng_fallback_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_fallback_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_fallback_contraprova_sanity (L1.20-FONTFLIP, FT-F4, per-glyph fallback) -- the
  #     CONTRA-PROVA half of the gate above: a SEPARATE process/document declaring "Primary"
  #     (PixelOperatorMono) ONLY, with NO -rmlui-fallback-face block anywhere, asserting the same
  #     Greek/superscript-2 spans render BLANK (not the non-blank result fonteng_fallback_sanity
  #     measures) -- proving that non-blank result is attributable to the fallback mechanism
  #     specifically. Needs its OWN process because FontEngineOwn::fallback_faces_ is a flat,
  #     process-lifetime vector shared by every document a UiLayer session ever loads (see
  #     fonteng_fallback_contraprova_sanity.cpp's own header). Same registration shape as
  #     fonteng_fallback_sanity right above.
  # PT: fonteng_fallback_contraprova_sanity (L1.20-FONTFLIP, FT-F4, fallback por-glyph) -- a metade
  #     CONTRA-PROVA do gate acima: um processo/documento SEPARADO declarando só "Primary"
  #     (PixelOperatorMono), SEM bloco -rmlui-fallback-face em lugar nenhum, verificando que os
  #     mesmos spans Grego/superscript-2 renderizam EM BRANCO (não o resultado não-branco que o
  #     fonteng_fallback_sanity mede) -- provando que aquele resultado não-branco é atribuível ao
  #     mecanismo de fallback especificamente. Precisa do PRÓPRIO processo porque
  #     FontEngineOwn::fallback_faces_ é um vetor plano, de vida-de-processo, compartilhado por todo
  #     documento que uma sessão UiLayer algum dia carrega (ver o cabeçalho próprio de
  #     fonteng_fallback_contraprova_sanity.cpp). Mesmo formato de registro de
  #     fonteng_fallback_sanity logo acima.
  # -------------------------------------------------------------------------
  add_executable(fonteng_fallback_contraprova_sanity fonteng_fallback_contraprova_sanity.cpp)
  target_link_libraries(fonteng_fallback_contraprova_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_fallback_contraprova_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_fallback_contraprova_scene.rml COPYONLY)
  add_test(NAME fonteng_fallback_contraprova_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_fallback_contraprova_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  # -------------------------------------------------------------------------
  # EN: fonteng_sup_sanity (L1.20-FONTFLIP, FT-F4) -- validates the RCSS `vertical-align: super` +
  #     smaller `font-size` workaround for the GusWorld dev's "a²+b²=c²" problem (Primary/
  #     PixelOperatorMono lacks the real superscript-2 glyph; the plain digit '2' it DOES have is
  #     styled instead). Asserts each `.sup` span is measurably ELEVATED (smaller box.y) and
  #     measurably SMALLER (box.h) than its neighbouring baseline text AND an apples-to-apples
  #     full-size reference digit, for all three formula instances. Also writes an upscaled PPM
  #     artifact of the whole formula line to OUTDIR/sup_formula.ppm (OUTDIR = argv[1], defaults to
  #     "ab_visual" under WORKING_DIRECTORY when ctest invokes it with no extra args) for the líder
  #     to inspect visually -- see fonteng_sup_sanity.cpp's own header for the full method.
  # PT: fonteng_sup_sanity (L1.20-FONTFLIP, FT-F4) -- valida o workaround RCSS
  #     `vertical-align: super` + `font-size` menor pro problema "a²+b²=c²" do dev do GusWorld
  #     (Primary/PixelOperatorMono não tem o glyph real de superscript-2; o dígito plano '2' que ela
  #     TEM é estilizado no lugar). Verifica que cada span `.sup` está mensuravelmente ELEVADO
  #     (box.y menor) e mensuravelmente MENOR (box.h) que o texto de baseline vizinho E uma
  #     referência de dígito em tamanho-cheio maçã-com-maçã, pras três instâncias da fórmula. Também
  #     grava um artefato PPM ampliado da linha da fórmula inteira em OUTDIR/sup_formula.ppm
  #     (OUTDIR = argv[1], default "ab_visual" sob WORKING_DIRECTORY quando o ctest o invoca sem
  #     args extras) pro líder inspecionar visualmente -- ver o cabeçalho próprio de
  #     fonteng_sup_sanity.cpp pro método completo.
  # -------------------------------------------------------------------------
  add_executable(fonteng_sup_sanity fonteng_sup_sanity.cpp)
  target_link_libraries(fonteng_sup_sanity PRIVATE ${_embed_dep} glloader)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fonteng_sup_scene.rml
                 ${CMAKE_CURRENT_BINARY_DIR}/fonteng_sup_scene.rml COPYONLY)
  add_test(NAME fonteng_sup_sanity
    COMMAND ${CMAKE_COMMAND}
      -DEXE=$<TARGET_FILE:fonteng_sup_sanity>
      -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

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
# EN: FX-CARVE-1 -- polygon_sanity/polygon_gradient_sanity/image_tint_sanity/echo_sanity
#     all exercise glintfx-AUTHORED fx (the polygon()/image-tint() decorators and their
#     registration, now inside the GLINTFX_MODULE_FX-gated glintfx_fx OBJECT library) --
#     gated here so a lean-ui (GLINTFX_MODULE_FX=OFF) build's test suite does not try to
#     link against a decorator surface that was never compiled in (an unrecognised
#     "decorator: polygon(...)"/"image-tint(...)" is ignored by RmlUi, fail-high --
#     bootstrap.cpp). All 4 .cpp files are BYTE-UNMODIFIED by FX-CARVE-1 (only this CMake
#     gating changed) -- they remain the pixel-equivalence regression proof for the carve.
#     echo_sanity (L1.21-ECHO) is included here too even though its own header comment never
#     mentions "fx" by name: it is built ENTIRELY on top of the "image-tint(<url>)" decorator
#     (auditoria-dominó finding, FX-CARVE-1 -- confirmed by grep against echo_scene.rcss:
#     every box uses `decorator: image-tint(...)`), so it would silently render wrong (no
#     tint at all, decorator ignored fail-high) rather than fail loudly if left ungated.
# PT: FX-CARVE-1 -- polygon_sanity/polygon_gradient_sanity/image_tint_sanity/echo_sanity
#     todos exercitam fx AUTORAL da glintfx (os decorators polygon()/image-tint() e o
#     registro deles, agora dentro da biblioteca OBJECT glintfx_fx gateada por
#     GLINTFX_MODULE_FX) -- gateados aqui pra que a suíte de teste de um build lean-ui
#     (GLINTFX_MODULE_FX=OFF) não tente linkar contra uma superfície de decorator que nunca
#     foi compilada (um "decorator: polygon(...)"/"image-tint(...)" não reconhecido é
#     ignorado pelo RmlUi, fail-high -- bootstrap.cpp). Os 4 arquivos .cpp são
#     BYTE-INALTERADOS pelo FX-CARVE-1 (só este gating de CMake mudou) -- permanecem a prova
#     de regressão de equivalência-de-pixel da carve. O echo_sanity (L1.21-ECHO) entra aqui
#     também mesmo o próprio comentário de cabeçalho dele nunca citando "fx" pelo nome: ele é
#     construído INTEIRAMENTE em cima do decorator "image-tint(<url>)" (achado de
#     auditoria-dominó, FX-CARVE-1 -- confirmado via grep contra echo_scene.rcss: toda caixa
#     usa `decorator: image-tint(...)`), então renderizaria errado silenciosamente (nenhum
#     tingimento, decorator ignorado fail-high) em vez de falhar alto se deixado sem gate.
# ---------------------------------------------------------------------------
if(GLINTFX_MODULE_FX)
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
# EN: echo_sanity (L1.21-ECHO) -- proves the "sprite-echo" RCSS pattern (consumer-driven by
#     GusWorld: a translucent, blue-tinted "ghost" copy of a sprite, faded in/out by the host) is
#     achievable with ZERO new glintfx C++ feature -- only the existing "image-tint(<url>)"
#     decorator (GLINTFX-TINT-1/2) + CSS opacity + the existing add_class/remove_class DOM API
#     (L1.16-DOMRW). Reuses the circle_rgba.png fixture already vendored for texture_png_alpha.cpp
#     (copied again here, flat, next to echo_scene.rml/.rcss, so this embed-mode test's
#     WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR resolves "circle_rgba.png" the same way
#     image_tint_sanity resolves "image_tint_fixture.png"). Same build-config/WORKING_DIRECTORY
#     pattern as image_tint_sanity/polygon_gradient_sanity above (runs in BOTH GLINTFX_BACKEND_GLFW
#     configs -- the decorator + opacity + DOM API are all backend-agnostic).
# PT: echo_sanity (L1.21-ECHO) -- prova que o padrão RCSS "sprite-eco" (consumer-driven pelo
#     GusWorld: uma cópia "fantasma" translúcida e tingida de azul de um sprite, com fade in/out
#     pelo host) é alcançável com ZERO feature C++ nova no glintfx -- só o decorator
#     "image-tint(<url>)" já existente (GLINTFX-TINT-1/2) + o opacity do CSS + a API DOM
#     add_class/remove_class já existente (L1.16-DOMRW). Reusa a fixture circle_rgba.png já
#     vendorizada para texture_png_alpha.cpp (copiada de novo aqui, plana, junto de
#     echo_scene.rml/.rcss, para que este teste embed com
#     WORKING_DIRECTORY=CMAKE_CURRENT_BINARY_DIR resolva "circle_rgba.png" do mesmo jeito que
#     image_tint_sanity resolve "image_tint_fixture.png"). Mesmo padrão de
#     config-de-build/WORKING_DIRECTORY de image_tint_sanity/polygon_gradient_sanity acima (roda em
#     AMBAS as configs de GLINTFX_BACKEND_GLFW -- decorator + opacity + API DOM são todos
#     backend-agnósticos).
# ---------------------------------------------------------------------------
add_executable(echo_sanity echo_sanity.cpp)
target_link_libraries(echo_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/echo_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/echo_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/echo_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/echo_scene.rcss COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/circle_rgba.png
               ${CMAKE_CURRENT_BINARY_DIR}/circle_rgba.png COPYONLY)
add_test(NAME echo_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:echo_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()  # GLINTFX_MODULE_FX

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
# EN: asset_decode_hostile_sanity (AUD-L1-PARSE / TST-L1-DECODE, LW-AUD wave) -- fixed 5-case
#     corpus test for Gl3RenderInterface::LoadTexture (render_gl3.cpp) against malformed/hostile
#     asset files: PNG truncated mid-IDAT, pure garbage bytes, a PNG with absurd declared
#     dimensions (100000x100000), a 0-byte file, and a sparse file one byte past the 256 MiB
#     file-size cap the same audit wave added. Oracle: each hostile document still LOADS (broken
#     texture degrades the <img>, not the document), no crash/UB/OOM, and the engine remains
#     fully usable afterward (a real PNG decode still succeeds post-corpus). See
#     asset_decode_hostile_sanity.cpp's own top comment for the full per-case rationale and why
#     this is NOT a fuzz campaign (stb_image is already one of the world's most fuzzed OSS
#     targets; glintfx's own code around it is ~6 lines). Runs in BOTH build configs (embed-mode,
#     links ${_embed_dep} glloader -- same pair as input_hardening_sanity above; glloader is
#     needed for offscreen.hpp's own <gl_loader.h> include even though this test has no
#     GL-entry-point spy, unlike ripple_sanity). Depends on circle_rgba.png (flat, at
#     CMAKE_CURRENT_BINARY_DIR/circle_rgba.png -- see asset_decode_hostile_sanity.cpp's own final
#     "still usable" check, which reuses circle_rgba.png + texture_png_alpha.cpp's own red-pixel
#     oracle, NOT image_tint_fixture.png as a since-corrected comment here used to claim; the
#     flat copy previously came from the echo_sanity block below, FX-CARVE-1 fixed the
#     resulting dangling dependency, see that fix's own comment right below). WORKING_DIRECTORY =
#     CMAKE_CURRENT_BINARY_DIR (repo-wide embed-test convention) so "circle_rgba.png" resolves by
#     bare relative name; hostile fixtures themselves are written at runtime under /var/tmp (NOT
#     /tmp -- tmpfs on the dev machine, see the project's own CLAUDE.md gotcha), not checked into
#     the build tree.
# PT: asset_decode_hostile_sanity (AUD-L1-PARSE / TST-L1-DECODE, onda LW-AUD) -- teste de corpus
#     fixo de 5 casos para Gl3RenderInterface::LoadTexture (render_gl3.cpp) contra arquivos de
#     asset malformados/hostis: PNG truncado no meio do IDAT, bytes de lixo puro, um PNG com
#     dimensões declaradas absurdas (100000x100000), um arquivo 0-byte, e um arquivo esparso um
#     byte acima do teto de 256 MiB que a mesma onda de auditoria adicionou. Oráculo: cada
#     documento hostil ainda CARREGA (textura quebrada degrada o <img>, não o documento), sem
#     crash/UB/OOM, e o engine permanece totalmente usável depois (um decode de PNG real ainda
#     funciona pós-corpus). Ver o próprio comentário do topo de asset_decode_hostile_sanity.cpp
#     pro racional completo por-caso e por que isto NÃO é uma campanha de fuzz (stb_image já é um
#     dos alvos OSS mais fuzzados do mundo; o código próprio da glintfx ao redor dele tem ~6
#     linhas). Roda em AMBAS as configs de build (embed-mode, linka ${_embed_dep} glloader --
#     mesmo par de input_hardening_sanity acima; glloader é necessário pro próprio
#     <gl_loader.h> de offscreen.hpp mesmo este teste não tendo espião de GL-entry-point,
#     diferente do ripple_sanity). Depende de circle_rgba.png (plano, em
#     CMAKE_CURRENT_BINARY_DIR/circle_rgba.png -- ver o próprio check final "ainda usável" de
#     asset_decode_hostile_sanity.cpp, que reusa circle_rgba.png + o mesmo oráculo de pixel
#     vermelho de texture_png_alpha.cpp, NÃO image_tint_fixture.png como um comentário aqui,
#     agora corrigido, afirmava antes; a cópia plana vinha antes do bloco echo_sanity abaixo, o
#     FX-CARVE-1 corrigiu a dependência pendurada resultante, ver o próprio comentário deste fix
#     logo abaixo). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (convenção repo-wide de teste
#     embed) para "circle_rgba.png" resolver por nome relativo simples; as fixtures hostis em si
#     são escritas em runtime sob /var/tmp (NÃO /tmp -- tmpfs na máquina de dev, ver o gotcha do
#     CLAUDE.md deste projeto), não versionadas na árvore de build.
# ---------------------------------------------------------------------------
# EN: FX-CARVE-1 auditoria-dominó fix -- asset_decode_hostile_sanity is NOT an fx test (it
#     exercises LoadTexture, always compiled), but its own final "still usable" real-asset check
#     (asset_decode_hostile_sanity.cpp) resolves "circle_rgba.png" by bare relative name against
#     WORKING_DIRECTORY, relying on the FLAT copy the (now fx-gated) echo_sanity block below used
#     to provide unconditionally -- CONFIRMED BY RUNNING THE SUITE, not just by reading: this
#     exact gap made the test fail for real in a first `lean-ui` run (`red_count=0 < 500`, no
#     crash, silently wrong texture pipeline state) before this fix landed. Without an
#     UNCONDITIONAL copy here too, this test fails to find its fixture in a `lean-ui`
#     (GLINTFX_MODULE_FX=OFF) build -- a real regression the fx gate would otherwise introduce
#     for an fx-unrelated test. Harmless to duplicate against texture_png_alpha's own copy above
#     (different destination, `ui/circle_rgba.png` vs. this flat one) and against echo_sanity's
#     own copy when fx=ON (same COPYONLY destination, configure-time, no conflict).
# PT: fix de auditoria-dominó do FX-CARVE-1 -- asset_decode_hostile_sanity NÃO é um teste de fx
#     (exercita o LoadTexture, sempre compilado), mas o próprio check final "ainda usável" de
#     asset real dele (asset_decode_hostile_sanity.cpp) resolve "circle_rgba.png" por nome
#     relativo simples contra o WORKING_DIRECTORY, dependendo da cópia PLANA que o bloco
#     echo_sanity (agora gateado por fx) abaixo fornecia incondicionalmente -- CONFIRMADO
#     RODANDO A SUÍTE, não só lendo: essa lacuna exata fez o teste falhar de verdade numa
#     primeira rodada `lean-ui` (`red_count=0 < 500`, sem crash, estado da pipeline de textura
#     silenciosamente errado) antes deste fix pousar. Sem uma cópia INCONDICIONAL aqui também,
#     este teste falha em achar a própria fixture num build `lean-ui` (GLINTFX_MODULE_FX=OFF) --
#     uma regressão real que o gate de fx introduziria de outra forma pra um teste sem relação
#     com fx. Inofensivo duplicar contra a própria cópia do texture_png_alpha acima (destino
#     diferente, `ui/circle_rgba.png` vs. esta plana) e contra a própria cópia do echo_sanity
#     quando fx=ON (mesmo destino COPYONLY, em tempo de configure, sem conflito).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ui/circle_rgba.png
               ${CMAKE_CURRENT_BINARY_DIR}/circle_rgba.png COPYONLY)
add_executable(asset_decode_hostile_sanity asset_decode_hostile_sanity.cpp)
target_link_libraries(asset_decode_hostile_sanity PRIVATE ${_embed_dep} glloader)
add_test(NAME asset_decode_hostile_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:asset_decode_hostile_sanity>
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
# EN: gl_resource_leak (TST-L1-GLLEAK / GLLEAK-2) -- endurance-lite proof that no GL driver
#     HANDLE (texture/FBO/renderbuffer/buffer/VAO/program/shader) leaks across 20 reload-on-
#     navigate cycles (alternating min.rml/min_partial.rml, the same document pair
#     document_reload_leak.cpp reuses above) plus 1 full UiLayer destroy+recreate cycle.
#     Complements document_reload_leak.cpp: that test proves heap/document-count hygiene via
#     ASan/LSan (TST-L1-MEM), which is BLIND to GL object names living in the driver's own
#     namespace. Uses ONLY the public UiLayer API + raw GL queries (glIsTexture et al.) via
#     glloader, no RmlUi:: internals needed -- unlike document_reload_leak.cpp/focus_sanity.cpp
#     above, so it does NOT link RmlUi::Core. Links glloader explicitly: the test calls
#     glIsTexture/glIsFramebuffer/glIsRenderbuffer/glIsBuffer/glIsVertexArray/glIsProgram/
#     glIsShader directly, same reasoning as gl_state_guard.cpp/ui_layer_compose.cpp above.
#     Runs in BOTH build configs (links ${_embed_dep}) since the code under test
#     (engine.cpp/bootstrap.cpp/render_gl3.cpp) is always compiled regardless of
#     GLINTFX_BACKEND_GLFW. WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/min.rml"/
#     "tests/min_partial.rml" resolve to build/tests/*.rml regardless of where ctest is
#     invoked from -- same convention as document_reload_leak.cpp immediately above.
# PT: gl_resource_leak (TST-L1-GLLEAK / GLLEAK-2) -- prova endurance-lite de que nenhum HANDLE
#     de driver GL (textura/FBO/renderbuffer/buffer/VAO/programa/shader) vaza ao longo de 20
#     ciclos de reload-on-navigate (alternando min.rml/min_partial.rml, o mesmo par de
#     documentos que document_reload_leak.cpp reutiliza acima) mais 1 ciclo completo de
#     destroy+recreate do UiLayer. Complementa document_reload_leak.cpp: aquele teste prova
#     higiene de heap/contagem-de-documento via ASan/LSan (TST-L1-MEM), que é CEGO a nomes de
#     objeto GL que vivem no namespace do próprio driver. Usa SÓ a API pública do UiLayer +
#     queries GL cruas (glIsTexture etc.) via glloader, sem precisar de internals do RmlUi:: --
#     diferente de document_reload_leak.cpp/focus_sanity.cpp acima, então NÃO linka
#     RmlUi::Core. Linka glloader explicitamente: o teste chama glIsTexture/glIsFramebuffer/
#     glIsRenderbuffer/glIsBuffer/glIsVertexArray/glIsProgram/glIsShader diretamente, mesmo
#     raciocínio de gl_state_guard.cpp/ui_layer_compose.cpp acima. Roda em AMBAS as configs de
#     build (linka ${_embed_dep}) já que o código sob teste (engine.cpp/bootstrap.cpp/
#     render_gl3.cpp) é sempre compilado independente de GLINTFX_BACKEND_GLFW.
#     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/min.rml"/"tests/min_partial.rml"
#     resolvam para build/tests/*.rml independente de onde o ctest é invocado -- mesma
#     convenção do document_reload_leak.cpp logo acima.
# ---------------------------------------------------------------------------
add_executable(gl_resource_leak gl_resource_leak.cpp)
target_link_libraries(gl_resource_leak PRIVATE ${_embed_dep} glloader)
add_test(NAME gl_resource_leak
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:gl_resource_leak>
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
# EN: FX-CARVE-1 -- ripple_sanity/ripple_stress_sanity exercise the glintfx-AUTHORED
#     "ripple()" decorator + its backdrop-capture machinery (both now inside the
#     GLINTFX_MODULE_FX-gated glintfx_fx OBJECT library, src/fx/effects_gl3.cpp) -- gated
#     here so a lean-ui (GLINTFX_MODULE_FX=OFF) build's test suite does not try to link
#     against a decorator/shader-injection surface that was never compiled in. Both .cpp
#     files are BYTE-UNMODIFIED by FX-CARVE-1 (only this CMake gating changed) -- they remain
#     the pixel-equivalence regression proof for the carve.
# PT: FX-CARVE-1 -- ripple_sanity/ripple_stress_sanity exercitam o decorator "ripple()"
#     AUTORAL da glintfx + a própria maquinaria de captura de backdrop (ambos agora dentro
#     da biblioteca OBJECT glintfx_fx gateada por GLINTFX_MODULE_FX, src/fx/effects_gl3.cpp)
#     -- gateados aqui pra que a suíte de teste de um build lean-ui (GLINTFX_MODULE_FX=OFF)
#     não tente linkar contra uma superfície de decorator/injeção-de-shader que nunca foi
#     compilada. Os dois arquivos .cpp são BYTE-INALTERADOS pelo FX-CARVE-1 (só este gating
#     de CMake mudou) -- permanecem a prova de regressão de equivalência-de-pixel da carve.
# ---------------------------------------------------------------------------
if(GLINTFX_MODULE_FX)
# ---------------------------------------------------------------------------
# EN: ripple_sanity -- ADVERSARIAL GATE for the "ripple([<max-radius>])" FBO-0 backdrop-refraction
#     decorator (L1.22-WAVE, decorator_ripple.{hpp,cpp}) and its capture (L1.22-CAPTURE,
#     Gl3RenderInterface::CaptureBackdrop, render_gl3.cpp). Proves (1) capture+refraction are
#     pixel-correct on BOTH axes (horizontal AND vertical -- the vertical proof specifically
#     targets the commit's own backdrop-sample Y-flip fix), (2) the cost-zero-when-inactive gate
#     via a GL-entry-point SPY on glCopyTexSubImage2D (a pixel-only check cannot falsify this gate
#     -- see ripple_sanity.cpp's own top comment), (3) GlStateGuard's read-framebuffer restore.
#     Links glloader explicitly (like golden_test/ui_layer_compose/gl_state_guard above): the spy
#     needs the SAME glCopyTexSubImage2D global CaptureBackdrop calls (see the linkage-reasoning
#     comment in ripple_sanity.cpp for why `glintfx glloader` together, not `glintfx` alone,
#     guarantees this). Runs in BOTH build configs (embed-mode test, links ${_embed_dep} like the
#     other embed tests above) -- the decorator + shader injection are backend-agnostic.
# PT: ripple_sanity -- GATE ADVERSARIAL para o decorator de refração de backdrop do FBO-0
#     "ripple([<raio-max>])" (L1.22-WAVE) e sua captura (L1.22-CAPTURE). Prova (1) captura+
#     refração corretas em pixel em AMBOS os eixos (horizontal E vertical -- a prova vertical mira
#     especificamente o fix de flip em Y do próprio commit), (2) o gate de custo-zero-quando-
#     inativo via um ESPIÃO de entry point GL sobre glCopyTexSubImage2D (um check só-de-pixel não
#     consegue falsificar este gate -- ver o próprio comentário do topo de ripple_sanity.cpp),
#     (3) a restauração do read-framebuffer do GlStateGuard. Linka glloader explicitamente (como
#     golden_test/ui_layer_compose/gl_state_guard acima). Roda em AMBAS as configs de build (teste
#     de embed mode, linka ${_embed_dep} como os outros testes embed acima) -- o decorator + a
#     injeção de shader são backend-agnósticos.
# ---------------------------------------------------------------------------
add_executable(ripple_sanity ripple_sanity.cpp)
target_link_libraries(ripple_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ripple_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/ripple_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ripple_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/ripple_scene.rcss COPYONLY)
add_test(NAME ripple_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ripple_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# ---------------------------------------------------------------------------
# EN: ripple_stress_sanity -- INDEPENDENT adversarial gate for commit 43cdcfa (the L1.22-CAPTURE
#     cold-start fix's own review: EnsureBackdropCaptured now runs mid-Context::Render(), not
#     before it). Differential proof (NOT hand-derived math): renders the SAME document twice
#     (ripple decorator inactive, then active) and diffs FOUR "after" elements -- each a different
#     RenderInterface_GL3 draw path, each painted LATER in the SAME tree walk than the ripple
#     element -- bit-exact between the two runs, plus a host-GL-state sentinel on a non-zero
#     texture unit. See ripple_stress_sanity.cpp's own top comment for the full methodology. Links
#     `glintfx glloader` like ripple_sanity above (same glCopyTexSubImage2D spy idiom, same
#     linkage-safety reasoning).
# PT: ripple_stress_sanity -- GATE ADVERSARIAL INDEPENDENTE pro commit 43cdcfa (review do próprio
#     fix de cold-start do L1.22-CAPTURE). Prova DIFERENCIAL (NÃO matemática derivada à mão):
#     renderiza o MESMO documento duas vezes (decorator ripple inativo, depois ativo) e compara
#     QUATRO elementos "after" -- cada um um caminho de draw diferente do RenderInterface_GL3,
#     cada um pintado DEPOIS do elemento ripple na MESMA caminhada de árvore -- bit-exatos entre as
#     duas rodadas, mais um sentinela de estado GL do host numa unidade de textura não-zero. Ver o
#     próprio comentário do topo de ripple_stress_sanity.cpp pra metodologia completa.
# ---------------------------------------------------------------------------
add_executable(ripple_stress_sanity ripple_stress_sanity.cpp)
target_link_libraries(ripple_stress_sanity PRIVATE ${_embed_dep} glloader)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ripple_stress_scene.rml
               ${CMAKE_CURRENT_BINARY_DIR}/ripple_stress_scene.rml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ripple_stress_scene.rcss
               ${CMAKE_CURRENT_BINARY_DIR}/ripple_stress_scene.rcss COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/image_tint_fixture.png
               ${CMAKE_CURRENT_BINARY_DIR}/image_tint_fixture.png COPYONLY)
add_test(NAME ripple_stress_sanity
  COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:ripple_stress_sanity>
    -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()  # GLINTFX_MODULE_FX

# ---------------------------------------------------------------------------
# EN: contract_consume (TST-L1-CONTRACT) — proves the consumer-side contract end-to-end from
#     OUTSIDE the glintfx source tree: cmake --install this build tree to a throwaway staging
#     prefix, configure + build the SEPARATE consumer-example-embed/ project against that
#     prefix via find_package(glintfx CONFIG) + CMAKE_PREFIX_PATH (NOT FetchContent -- see
#     contract_consume.cmake's own top-of-file comment for why that distinction matters: it is
#     what avoids re-building RmlUi, a genuine anti-OOM constraint on this repo's dev/CI
#     hardware), then run the resulting consumer_embed binary and check it renders a
#     non-degenerate frame. All 4 sub-steps (install / configure / build / run) happen inside
#     a single CMake -P script driver (contract_consume.cmake) so ctest reports ONE test with
#     the full stdout/stderr of whichever step failed attached -- a gap anywhere in the public
#     install (a header the Config forgot to co-install, a missing find_dependency, ...)
#     surfaces here as a normal ctest failure, not a silent skip.
#
#     Gated behind GLINTFX_CONTRACT_TEST (default OFF): the extra `cmake --install` +
#     `cmake --build` round-trip this test performs is real (if modest) wall-clock cost every
#     `ctest` invocation would otherwise always pay in the local dev loop; CI turns it ON
#     explicitly (mirrors GLINTFX_GOLDEN_TEST's OFF-by-default, opt-in-in-CI shape a few
#     hundred lines up in this same file). Also gated `AND NOT GLINTFX_SANITIZE`: a
#     GLINTFX_SANITIZE=ON build's installed libglintfx.a is ASan/UBSan-instrumented, but
#     consumer-example-embed/CMakeLists.txt does not (and, being a wholly separate project
#     that any THIRD-PARTY consumer would build unmodified, structurally should not) apply
#     those same instrumentation flags to its own consumer_embed executable -- linking an
#     instrumented static archive into an uninstrumented final binary is a known-broken
#     combination (missing __asan_*/__ubsan_* runtime symbols), a different, not-yet-solved
#     problem out of scope for this contract test; the nightly GLINTFX_SANITIZE gate already
#     covers glintfx's OWN memory-safety in-tree, which is what it exists for.
#     Also only registered when glintfx is the TOP-LEVEL project (mirrors the
#     `if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)` guard around the install() rules
#     themselves, further up in CMakeLists.txt) -- running an install-and-reconsume round-trip
#     makes sense only for glintfx's own build, never when glintfx is pulled in as a
#     FetchContent/add_subdirectory dependency of another project's build (e.g. GusWorld).
# PT: contract_consume (TST-L1-CONTRACT) — prova o contrato do lado consumidor ponta-a-ponta
#     de FORA da árvore de fonte da glintfx: cmake --install nesta árvore de build para um
#     prefixo de staging descartável, configura + builda o projeto SEPARADO
#     consumer-example-embed/ contra esse prefixo via find_package(glintfx CONFIG) +
#     CMAKE_PREFIX_PATH (NÃO FetchContent -- ver o comentário no topo do próprio
#     contract_consume.cmake para o porquê dessa distinção importar: é o que evita re-buildar
#     o RmlUi, uma restrição anti-OOM real no hardware de dev/CI deste repositório), depois
#     roda o binário consumer_embed resultante e verifica que ele renderiza um frame
#     não-degenerado. Todos os 4 sub-passos (install / configure / build / run) acontecem
#     dentro de um único script driver CMake -P (contract_consume.cmake) para que o ctest
#     reporte UM teste com o stdout/stderr completo de qualquer passo que falhou anexado -- um
#     gap em qualquer lugar do install público (um header que o Config esqueceu de
#     co-instalar, um find_dependency faltando, ...) aparece aqui como uma falha normal de
#     ctest, não um skip silencioso.
#
#     Bloqueado por GLINTFX_CONTRACT_TEST (default OFF): o ciclo extra de `cmake --install` +
#     `cmake --build` que este teste executa é um custo real (ainda que modesto) de
#     wall-clock que toda invocação de `ctest` de outra forma sempre pagaria no loop de dev
#     local; o CI liga explicitamente (espelha o formato OFF-por-padrão,
#     opt-in-no-CI do GLINTFX_GOLDEN_TEST algumas centenas de linhas acima neste mesmo
#     arquivo). Também bloqueado `AND NOT GLINTFX_SANITIZE`: o libglintfx.a instalado de um
#     build GLINTFX_SANITIZE=ON é instrumentado com ASan/UBSan, mas
#     consumer-example-embed/CMakeLists.txt não aplica (e, sendo um projeto inteiramente
#     separado que qualquer consumidor TERCEIRO buildaria sem modificação, estruturalmente não
#     deveria aplicar) essas mesmas flags de instrumentação ao próprio executável
#     consumer_embed -- linkar um archive estático instrumentado num binário final não
#     instrumentado é uma combinação conhecidamente quebrada (símbolos de runtime
#     __asan_*/__ubsan_* faltando), um problema diferente e ainda não resolvido, fora do
#     escopo deste teste de contrato; o gate noturno GLINTFX_SANITIZE já cobre a segurança de
#     memória da PRÓPRIA glintfx in-tree, que é para o que ele existe.
#     Também só registrado quando a glintfx é o projeto TOP-LEVEL (espelha o guard
#     `if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)` em torno das próprias regras install(),
#     mais acima no CMakeLists.txt) -- um round-trip de install-e-reconsumo só faz sentido
#     para o próprio build da glintfx, nunca quando a glintfx é puxada como dependência
#     FetchContent/add_subdirectory do build de outro projeto (ex.: GusWorld).
# ---------------------------------------------------------------------------
option(GLINTFX_CONTRACT_TEST
  "Enable the contract_consume ctest: install + find_package(glintfx) + link + run round-trip (TST-L1-CONTRACT)"
  OFF)

if(GLINTFX_CONTRACT_TEST AND NOT GLINTFX_SANITIZE AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_test(NAME contract_consume
    COMMAND ${CMAKE_COMMAND}
      -DGLINTFX_BUILD_DIR=${CMAKE_BINARY_DIR}
      -DCONSUMER_SRC_DIR=${CMAKE_SOURCE_DIR}/../consumer-example-embed
      -P ${CMAKE_SOURCE_DIR}/tests/contract_consume.cmake)
endif()

# ---------------------------------------------------------------------------
# EN: i18n-1 (framework-2D, ADR-0015 module "i18n") -- glintfx_i18n's own test suite.
#     Deliberately OUTSIDE both the `if(GLINTFX_BACKEND_GLFW)` block above and any Xvfb wrapper
#     (run_xvfb.cmake): glintfx::I18n has zero GL/window/RmlUi dependency (ADR-0015 (b), "i18n
#     depends on core ONLY"), so these are PURE unit tests, run directly as plain executables --
#     same class of test as glfw_event_translate_sanity (pure formula/unit, no display to
#     isolate), except this one does not even need to link `glfw` for header constants, since it
#     touches nothing GLFW-shaped at all. Runs identically in BOTH GLINTFX_BACKEND_GLFW configs
#     (ON/OFF) and proves the module builds+passes isolated per ADR-0015 (e)'s "each module ships
#     with its own test suite that passes with all other optional modules OFF" -- there is no
#     other optional module glintfx_i18n could possibly depend on to turn OFF here (see the
#     zero-link-deps comment on glintfx_i18n's own add_library() block in CMakeLists.txt).
#     Guarded on GLINTFX_MODULE_I18N (default ON) so a consumer that explicitly configures
#     -DGLINTFX_MODULE_I18N=OFF does not fail the test build over a module it opted out of.
# PT: i18n-1 (framework-2D, módulo "i18n" da ADR-0015) -- suíte de testes própria do
#     glintfx_i18n. Deliberadamente FORA tanto do bloco `if(GLINTFX_BACKEND_GLFW)` acima quanto
#     de qualquer wrapper Xvfb (run_xvfb.cmake): glintfx::I18n não tem NENHUMA dependência de
#     GL/janela/RmlUi (ADR-0015 (b), "i18n depende SÓ de core"), então estes são testes unit
#     PUROS, rodados direto como executáveis simples -- mesma classe de teste do
#     glfw_event_translate_sanity (unit/fórmula puro, sem nada de display pra isolar), exceto
#     que este nem precisa linkar `glfw` pelas constantes de header, já que não toca em nada com
#     formato GLFW. Roda identicamente nas DUAS configs de GLINTFX_BACKEND_GLFW (ON/OFF) e prova
#     que o módulo builda+passa isolado conforme a ADR-0015 (e), "cada módulo entrega a própria
#     suíte de testes, que passa com todos os outros módulos opcionais OFF" -- não há nenhum
#     outro módulo opcional do qual o glintfx_i18n pudesse depender pra desligar aqui (ver o
#     comentário de zero-deps-de-link no próprio bloco add_library() do glintfx_i18n no
#     CMakeLists.txt). Bloqueado por GLINTFX_MODULE_I18N (padrão ON) pra que um consumidor que
#     configure explicitamente -DGLINTFX_MODULE_I18N=OFF não falhe o build de testes por causa
#     de um módulo do qual ele optou sair.
# ---------------------------------------------------------------------------
if(GLINTFX_MODULE_I18N)
  # EN: i18n_catalog_sanity -- catalog parsing: well-formed multi-locale load, malformed-line
  #     robustness (lines_skipped > 0, never a crash/abort), key=value-before-any-[locale]
  #     rejection, last-write-wins on a repeated key, and load_catalog_string(nullptr)/
  #     load_catalog_file(missing path) hardening (ok=false, no touch to the catalog).
  # PT: i18n_catalog_sanity -- parsing de catálogo: carga multi-locale bem-formada, robustez a
  #     linha malformada (lines_skipped > 0, nunca crash/abort), rejeição de key=value antes de
  #     qualquer [locale], último-escreve-vence numa chave repetida, e hardening de
  #     load_catalog_string(nullptr)/load_catalog_file(caminho inexistente) (ok=false, catálogo
  #     intocado).
  add_executable(i18n_catalog_sanity i18n_catalog_sanity.cpp)
  target_link_libraries(i18n_catalog_sanity PRIVATE glintfx)
  add_test(NAME i18n_catalog_sanity COMMAND i18n_catalog_sanity)

  # EN: i18n_lookup_fallback_sanity -- tr()/tr_plural() RESOLUTION ORDER (i18n.hpp): exact-locale
  #     hit, region->base-language fallback (pt-BR miss -> pt hit), explicit fallback_locale
  #     (+ ITS base-language step), and the final bare-key last resort when nothing resolves
  #     anywhere in the chain -- plus runtime locale switching (T1: same key, two different
  #     set_locale() calls, two different resolved strings, no reload/re-create needed).
  # PT: i18n_lookup_fallback_sanity -- ORDEM DE RESOLUÇÃO de tr()/tr_plural() (i18n.hpp): acerto
  #     de locale exato, fallback região->idioma-base (miss em pt-BR -> acerto em pt), fallback
  #     explícito fallback_locale (+ o passo de idioma-base DELE), e o último recurso de chave
  #     crua quando nada resolve em nenhum ponto da cadeia -- mais troca de locale em runtime
  #     (T1: mesma chave, duas chamadas set_locale() diferentes, duas strings resolvidas
  #     diferentes, sem precisar recarregar/recriar nada).
  add_executable(i18n_lookup_fallback_sanity i18n_lookup_fallback_sanity.cpp)
  target_link_libraries(i18n_lookup_fallback_sanity PRIVATE glintfx)
  add_test(NAME i18n_lookup_fallback_sanity COMMAND i18n_lookup_fallback_sanity)

  # EN: i18n_plural_sanity -- CLDR-lite plural category selection for en/pt (i18n.cpp's cited
  #     CLDR v48 chart rules), asserted by COUNT, not just presence: en one={1}/other={0,2,3,...};
  #     pt one={0,1}/other={2,3,...} (the actual divergence between the two locales' rules, and
  #     therefore the case most likely to silently break if the rule table is ever "simplified"
  #     back to a single shared formula -- the exact "fórmula de conversão sem teste de delta"
  #     bug class the glfw_event_translate_sanity header comment names). Also asserts the '#'
  #     count-placeholder substitution (both single and multiple '#' occurrences) and negative-n
  #     sign handling.
  # PT: i18n_plural_sanity -- seleção de categoria de plural CLDR-lite para en/pt (regras do
  #     gráfico CLDR v48 citado em i18n.cpp), verificada por CONTAGEM, não só presença: en
  #     one={1}/other={0,2,3,...}; pt one={0,1}/other={2,3,...} (a divergência real entre as
  #     regras dos dois locales, e portanto o caso mais provável de quebrar silenciosamente se a
  #     tabela de regra algum dia for "simplificada" de volta pra uma fórmula única compartilhada
  #     -- exatamente a classe de bug "fórmula de conversão sem teste de delta" que o comentário
  #     de cabeçalho do glfw_event_translate_sanity nomeia). Também verifica a substituição do
  #     placeholder de contagem '#' (tanto uma ocorrência única quanto múltiplas) e o tratamento
  #     de sinal para n negativo.
  add_executable(i18n_plural_sanity i18n_plural_sanity.cpp)
  target_link_libraries(i18n_plural_sanity PRIVATE glintfx)
  add_test(NAME i18n_plural_sanity COMMAND i18n_plural_sanity)
endif()

# EN: A3-AUDIO (framework-2D, ADR-0015 module "audio") -- glintfx_audio's own test suite.
#     Deliberately outside both the `if(GLINTFX_BACKEND_GLFW)` block above and the Xvfb
#     wrapper (run_xvfb.cmake): glintfx::Audio has zero GL/window/RmlUi dependency (ADR-0015
#     (b), "audio depends on core ONLY"), same independence class as glintfx_i18n's own suite
#     immediately above -- these are pure unit tests, no display server needed. Uses
#     AudioConfig::null_backend=true throughout (see docs/audio.md), so no ALSA/PulseAudio
#     dependency either -- these run in any CI container, headless.
# PT: A3-AUDIO (framework-2D, módulo "audio" da ADR-0015) -- suíte de testes própria do
#     glintfx_audio. Deliberadamente FORA tanto do bloco `if(GLINTFX_BACKEND_GLFW)` acima
#     quanto do wrapper Xvfb (run_xvfb.cmake): glintfx::Audio tem zero dependência de
#     GL/janela/RmlUi (ADR-0015 (b), "audio depende SÓ de core"), mesma classe de
#     independência da própria suíte do glintfx_i18n logo acima -- são testes unit puros,
#     sem servidor de display necessário. Usa AudioConfig::null_backend=true o tempo todo
#     (ver docs/audio.md), então também sem dependência de ALSA/PulseAudio -- rodam em
#     qualquer container de CI, headless.
if(GLINTFX_MODULE_AUDIO)
  # EN: audio_lifecycle_sanity -- init/shutdown/is_initialized state transitions, shutdown()
  #     idempotency (2x + via dtor), init() called twice without an intervening shutdown(),
  #     every op before init()/after shutdown() returning false/0, re-init after a clean
  #     shutdown().
  # PT: audio_lifecycle_sanity -- transições de estado init/shutdown/is_initialized,
  #     idempotência do shutdown() (2x + via dtor), init() chamado duas vezes sem shutdown()
  #     no meio, toda op antes de init()/depois de shutdown() retornando false/0, re-init
  #     depois de um shutdown() limpo.
  add_executable(audio_lifecycle_sanity audio_lifecycle_sanity.cpp)
  target_link_libraries(audio_lifecycle_sanity PRIVATE glintfx)
  add_test(NAME audio_lifecycle_sanity COMMAND audio_lifecycle_sanity)

  # EN: audio_playback_sanity -- a minimal PCM16 WAV generated IN CODE (no binary fixture in
  #     the repo) to a temp path; load -> play -> stop -> set_volume on a real sound; volume
  #     hardening (NaN/Inf/negative rejected); unknown SoundId failing every per-sound op;
  #     load_sound() of a missing path returning 0.
  # PT: audio_playback_sanity -- um WAV PCM16 mínimo gerado EM CÓDIGO (sem fixture binária no
  #     repo) num caminho temporário; load -> play -> stop -> set_volume num som real;
  #     hardening de volume (NaN/Inf/negativo rejeitados); SoundId desconhecido falhando toda
  #     operação por-som; load_sound() de um caminho ausente retornando 0.
  add_executable(audio_playback_sanity audio_playback_sanity.cpp)
  target_link_libraries(audio_playback_sanity PRIVATE glintfx)
  add_test(NAME audio_playback_sanity COMMAND audio_playback_sanity)

  # EN: audio_hostile_sanity -- WAV truncated mid-header/mid-data, random garbage bytes, an
  #     empty file, a WAV lying about its data-chunk size, load_sound(nullptr) -- every case
  #     must return 0/false and never crash (auditoria-dominó: same robustness class as
  #     i18n_catalog_sanity/decorator_polygon).
  # PT: audio_hostile_sanity -- WAV truncado no meio do header/data, bytes de lixo aleatório,
  #     arquivo vazio, um WAV mentindo sobre o tamanho do chunk de data, load_sound(nullptr)
  #     -- todo caso precisa retornar 0/false e nunca crashar (auditoria-dominó: mesma classe
  #     de robustez do i18n_catalog_sanity/decorator_polygon).
  add_executable(audio_hostile_sanity audio_hostile_sanity.cpp)
  target_link_libraries(audio_hostile_sanity PRIVATE glintfx)
  add_test(NAME audio_hostile_sanity COMMAND audio_hostile_sanity)
endif()

# ---------------------------------------------------------------------------
# EN: A2-GAMEPAD (framework-2D, ADR-0015/ADR-0016 module "gamepad") -- Wave 1 (Agent A) test
#     suite: the decode core (src/gamepad_decode.{hpp,cpp}) and the mapping module
#     (src/gamepad_mapping.{hpp,cpp}). Deliberately outside both the `if(GLINTFX_BACKEND_GLFW)`
#     block above and any Xvfb wrapper (run_xvfb.cmake): both are fed synthetic byte buffers/
#     capability bitmaps only -- zero `fd`, zero `ioctl`, zero real `/dev/input` device, zero
#     GL/window/RmlUi dependency -- same independence class as glintfx_i18n/glintfx_audio's own
#     suites immediately above. Guarded on GLINTFX_MODULE_GAMEPAD (default ON on Linux, forced
#     OFF elsewhere by the option's own platform gate in CMakeLists.txt) so a consumer that
#     explicitly configures -DGLINTFX_MODULE_GAMEPAD=OFF (or builds on a non-Linux host) does not
#     fail the test build over a module it opted out of / cannot have. Targets link `glintfx`
#     (for the public `<glintfx/gamepad_types.hpp>` GamepadButton/GamepadAxis enums) plus the
#     internal `src/` include dir (for `gamepad_decode.hpp`/`gamepad_mapping.hpp`, never
#     installed, per AGENTS.md's encapsulation rule -- these tests deliberately reach past the
#     public API to unit-test the pure decode/mapping internals directly, same posture as
#     glfw_event_translate_sanity reaching into glfw_event_translate.hpp).
# PT: A2-GAMEPAD (framework-2D, módulo "gamepad" da ADR-0015/ADR-0016) -- suíte de teste da Onda
#     1 (Agente A): o núcleo de decode (src/gamepad_decode.{hpp,cpp}) e o módulo de mapeamento
#     (src/gamepad_mapping.{hpp,cpp}). Deliberadamente fora tanto do bloco
#     `if(GLINTFX_BACKEND_GLFW)` acima quanto de qualquer wrapper Xvfb (run_xvfb.cmake): ambos só
#     são alimentados com buffers de bytes sintéticos/bitmaps de capacidade -- zero `fd`, zero
#     `ioctl`, zero device `/dev/input` real, zero dependência de GL/janela/RmlUi -- mesma classe
#     de independência das próprias suítes do glintfx_i18n/glintfx_audio logo acima. Bloqueado
#     por GLINTFX_MODULE_GAMEPAD (padrão ON no Linux, forçado OFF em outro lugar pelo próprio
#     gate de plataforma da option no CMakeLists.txt) pra que um consumidor que configure
#     explicitamente -DGLINTFX_MODULE_GAMEPAD=OFF (ou builde num host não-Linux) não falhe o
#     build de testes por causa de um módulo do qual ele optou sair / não pode ter. Os alvos
#     linkam `glintfx` (pelos enums públicos GamepadButton/GamepadAxis de
#     `<glintfx/gamepad_types.hpp>`) mais o include dir interno de `src/` (por
#     `gamepad_decode.hpp`/`gamepad_mapping.hpp`, nunca instalados, conforme a regra de
#     encapsulamento do AGENTS.md -- estes testes deliberadamente alcançam além da API pública
#     pra unit-testar os internos puros de decode/mapping diretamente, mesma postura do
#     glfw_event_translate_sanity alcançando dentro de glfw_event_translate.hpp).
# ---------------------------------------------------------------------------
if(GLINTFX_MODULE_GAMEPAD)
  # EN: gamepad_decode_sanity -- well-formed decode: button press/release, autorepeat (value=2)
  #     no-op, stick axis normalization (-32768..32767 real range), trigger axis normalization
  #     (0..255 real range), D-pad hat (-1/0/+1), SYN_DROPPED sticky flag, and chunking
  #     independence (1-byte-at-a-time vs 7+17-byte pieces vs a single call all decode
  #     identically).
  # PT: gamepad_decode_sanity -- decode bem-formado: press/release de botão, autorepeat
  #     (value=2) no-op, normalização de eixo stick (faixa real -32768..32767), normalização de
  #     eixo gatilho (faixa real 0..255), hat do D-pad (-1/0/+1), flag pegajosa de SYN_DROPPED, e
  #     independência de fatiamento (1 byte por vez vs pedaços de 7+17 bytes vs uma única
  #     chamada decodificam identicamente).
  add_executable(gamepad_decode_sanity gamepad_decode_sanity.cpp)
  target_link_libraries(gamepad_decode_sanity PRIVATE glintfx)
  target_include_directories(gamepad_decode_sanity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  add_test(NAME gamepad_decode_sanity COMMAND gamepad_decode_sanity)

  # EN: gamepad_decode_hostile -- random garbage bytes (several seeds), an event truncated at
  #     the end of the buffer, type/code far out of range (0xFFFF), value=INT32_MIN/MAX for both
  #     EV_KEY and EV_ABS, and degenerate absinfo ranges (min==max, min>max) -- never a crash,
  #     never an out-of-bounds read, a degenerate axis normalizes to exactly 0.0f.
  # PT: gamepad_decode_hostile -- bytes de lixo aleatório (várias seeds), um evento truncado no
  #     fim do buffer, type/code bem fora da faixa (0xFFFF), value=INT32_MIN/MAX tanto pra
  #     EV_KEY quanto pra EV_ABS, e faixas absinfo degeneradas (min==max, min>max) -- nunca
  #     crash, nunca leitura fora dos limites, um eixo degenerado normaliza pra exatamente 0.0f.
  add_executable(gamepad_decode_hostile gamepad_decode_hostile.cpp)
  target_link_libraries(gamepad_decode_hostile PRIVATE glintfx)
  target_include_directories(gamepad_decode_hostile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  add_test(NAME gamepad_decode_hostile COMMAND gamepad_decode_hostile)

  # EN: gamepad_mapping_sanity -- build_guid() against a known real input_id (Xbox 360 USB) plus
  #     the GUID "2 attempts" match (exact, then CRC-zeroed) MappingDb::find() performs;
  #     derive_layout() against a synthetic capability bitmap, asserting the exact
  #     SDL-compatible enumeration ORDER; MappingDb::parse_text() fed a gamecontrollerdb.txt-
  #     format line for the same pad, resolved end to end through a synthetic evdev event;
  #     default_kernel_spec_mapping() (D5) for a pad no DB entry matches.
  # PT: gamepad_mapping_sanity -- build_guid() contra um input_id real conhecido (Xbox 360 USB)
  #     mais o casamento de "2 tentativas" de GUID (exato, depois CRC-zerado) que
  #     MappingDb::find() faz; derive_layout() contra um bitmap de capacidade sintético,
  #     verificando a ORDEM exata de enumeração compatível-SDL; MappingDb::parse_text()
  #     alimentado com uma linha em formato gamecontrollerdb.txt pro mesmo pad, resolvida ponta a
  #     ponta através de um evento evdev sintético; default_kernel_spec_mapping() (D5) pra um pad
  #     que nenhuma entrada de DB casa.
  add_executable(gamepad_mapping_sanity gamepad_mapping_sanity.cpp)
  target_link_libraries(gamepad_mapping_sanity PRIVATE glintfx)
  target_include_directories(gamepad_mapping_sanity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  add_test(NAME gamepad_mapping_sanity COMMAND gamepad_mapping_sanity)

  # EN: gamepad_mapping_hostile -- a line with no commas, GUID short/odd-length/non-hex, fields
  #     b99999/a-1/h9.99, a ~1 MB single line, text with no trailing '\n', parse_text(nullptr),
  #     and a mix of well-formed/malformed/wrong-platform lines -- 0 entries or a per-field skip,
  #     never a crash.
  # PT: gamepad_mapping_hostile -- uma linha sem vírgulas, GUID curto/tamanho ímpar/não-hex,
  #     campos b99999/a-1/h9.99, uma única linha de ~1 MB, texto sem '\n' final,
  #     parse_text(nullptr), e uma mistura de linhas bem-formadas/malformadas/plataforma-errada
  #     -- 0 entradas ou um skip por-campo, nunca crash.
  add_executable(gamepad_mapping_hostile gamepad_mapping_hostile.cpp)
  target_link_libraries(gamepad_mapping_hostile PRIVATE glintfx)
  target_include_directories(gamepad_mapping_hostile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  add_test(NAME gamepad_mapping_hostile COMMAND gamepad_mapping_hostile)

  # EN: Wave 2 (Agent B) test suite: the device layer (src/gamepad.cpp), reached only through the
  #     public glintfx::Gamepads facade (glintfx/gamepad.hpp) -- no internal src/ include needed
  #     for these three, unlike the Wave 1 tests above. `dev_dir` is always overridden to a
  #     throwaway temp directory (GamepadsConfig::dev_dir, see gamepad.hpp's own doc-comment) --
  #     these tests never touch the host's real /dev/input devices, so they are just as
  #     Xvfb-free/headless-safe as the Wave 1 pair above.
  # PT: Suíte de teste da Onda 2 (Agente B): a camada de device (src/gamepad.cpp), alcançada só
  #     pela fachada pública glintfx::Gamepads (glintfx/gamepad.hpp) -- nenhum include interno de
  #     src/ é necessário pra estes três, diferente dos testes da Onda 1 acima. `dev_dir` é
  #     sempre sobrescrito pra um diretório temporário descartável (GamepadsConfig::dev_dir, ver o
  #     próprio doc-comment do gamepad.hpp) -- estes testes nunca tocam nos devices reais de
  #     /dev/input do host, então são tão livres de Xvfb/seguros pra headless quanto o par da
  #     Onda 1 acima.
  add_executable(gamepad_device_sanity gamepad_device_sanity.cpp)
  target_link_libraries(gamepad_device_sanity PRIVATE glintfx)
  add_test(NAME gamepad_device_sanity COMMAND gamepad_device_sanity)

  add_executable(gamepad_hotplug_smoke gamepad_hotplug_smoke.cpp)
  target_link_libraries(gamepad_hotplug_smoke PRIVATE glintfx)
  add_test(NAME gamepad_hotplug_smoke COMMAND gamepad_hotplug_smoke)

  add_executable(gamepad_api_hardening gamepad_api_hardening.cpp)
  target_link_libraries(gamepad_api_hardening PRIVATE glintfx)
  add_test(NAME gamepad_api_hardening COMMAND gamepad_api_hardening)
endif()

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
#
#     A4-WINMODES DOMINO FIX (v0.16.0): this used to be a plain (non-APPEND)
#     set_tests_properties(... PROPERTIES ENVIRONMENT ...) call, which REPLACES a test's
#     entire ENVIRONMENT property rather than merging into it -- silently wiping out the
#     GLINTFX_TEST_WINDOW_MODE=<mode> variable app_window_mode_smoke_* sets earlier in this
#     file (its ENVIRONMENT is registered per-test, ABOVE, before this DIRECTORY-wide block
#     runs). Under a GLINTFX_SANITIZE=ON build every one of those 5 tests would have failed
#     at startup ("GLINTFX_TEST_WINDOW_MODE not set") the moment the nightly ASan gate ran --
#     found by auditing every existing consumer of ENVIRONMENT in this file, not just the new
#     one, per the house's "auditoria-dominó" discipline. Fixed by switching to
#     set_property(... APPEND PROPERTY ENVIRONMENT ...), which appends to whatever
#     per-test ENVIRONMENT entries already exist instead of clobbering them (a no-op append
#     for the many tests here with no prior ENVIRONMENT set).
# PT: FIX DE DOMINÓ A4-WINMODES (v0.16.0): isto costumava ser uma chamada simples (sem APPEND)
#     de set_tests_properties(... PROPERTIES ENVIRONMENT ...), que SUBSTITUI a propriedade
#     ENVIRONMENT inteira de um teste em vez de mesclar nela -- apagando silenciosamente a
#     variável GLINTFX_TEST_WINDOW_MODE=<modo> que app_window_mode_smoke_* define mais acima
#     neste arquivo (o ENVIRONMENT dele é registrado por-teste, ACIMA, antes deste bloco
#     de escopo de DIRECTORY rodar). Sob um build GLINTFX_SANITIZE=ON, cada um daqueles 5
#     testes teria falhado na largada ("GLINTFX_TEST_WINDOW_MODE not set") assim que o gate
#     noturno de ASan rodasse -- achado auditando TODO consumidor existente de ENVIRONMENT
#     neste arquivo, não só o novo, conforme a disciplina da casa de "auditoria-dominó".
#     Corrigido trocando para set_property(... APPEND PROPERTY ENVIRONMENT ...), que anexa às
#     entradas ENVIRONMENT por-teste que já existirem em vez de sobrescrevê-las (um append
#     no-op pros muitos testes aqui que não têm ENVIRONMENT prévio nenhum).
# ---------------------------------------------------------------------------
if(GLINTFX_SANITIZE)
  get_property(_glintfx_all_tests DIRECTORY PROPERTY TESTS)
  set_property(TEST ${_glintfx_all_tests} APPEND PROPERTY ENVIRONMENT
    "LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/lsan_suppressions.txt;ASAN_OPTIONS=detect_leaks=1;UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1:suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ubsan_suppressions.txt")
endif()

# ---------------------------------------------------------------------------
# EN: GLINTFX_COVERAGE (L1.18-COVCI, TST-L1-COV) -- instrument every executable/
#     static-library target defined in THIS directory (every test binary above, plus
#     the embed-only _glintfx_test_ctx GL-context helper) with Clang source-based
#     coverage flags. Applied here (end of file, after every add_executable() above,
#     both the GLINTFX_BACKEND_GLFW branch and the embed-mode branch) via a single loop
#     over this DIRECTORY's BUILDSYSTEM_TARGETS property -- mirrors the GLINTFX_SANITIZE
#     DIRECTORY-property idiom immediately above (same rationale: no individual
#     add_executable() block above needs to be touched to add/remove this gate).
#     Restricted to EXECUTABLE and STATIC_LIBRARY target types: INTERFACE libraries
#     (glloader) have no object file of their own to instrument -- their single source
#     is compiled inline into whichever target links them, which is already covered by
#     this same loop. No-op when GLINTFX_COVERAGE=OFF (glintfx_instrument_coverage()
#     itself is a no-op then; see CMakeLists.txt top-level for the function + full
#     per-target-vs-directory-wide rationale, including why RmlUi/_deps must NOT be
#     instrumented).
# PT: GLINTFX_COVERAGE (L1.18-COVCI, TST-L1-COV) -- instrumenta todo alvo executável/
#     biblioteca estática definido NESTE diretório (todo binário de teste acima, mais o
#     helper de contexto GL _glintfx_test_ctx do embed-only) com as flags de cobertura
#     source-based do Clang. Aplicado aqui (fim do arquivo, após todo add_executable()
#     acima, tanto o ramo GLINTFX_BACKEND_GLFW quanto o ramo embed mode) via um único
#     loop sobre a propriedade BUILDSYSTEM_TARGETS deste DIRECTORY -- espelha o idioma
#     de DIRECTORY property do GLINTFX_SANITIZE logo acima (mesma racional: nenhum bloco
#     add_executable() individual acima precisa ser tocado pra adicionar/remover este
#     gate). Restrito a tipos de alvo EXECUTABLE e STATIC_LIBRARY: bibliotecas INTERFACE
#     (glloader) não têm objeto próprio pra instrumentar -- seu único source é compilado
#     inline em qualquer alvo que as linka, já coberto por este mesmo loop. No-op com
#     GLINTFX_COVERAGE=OFF (a própria glintfx_instrument_coverage() já é no-op nesse
#     caso; ver CMakeLists.txt top-level para a função + a racional completa
#     por-alvo-vs-diretório, incluindo por que RmlUi/_deps NÃO devem ser instrumentados).
# ---------------------------------------------------------------------------
if(GLINTFX_COVERAGE)
  get_property(_glintfx_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_glintfx_cov_target ${_glintfx_test_targets})
    get_target_property(_glintfx_cov_target_type ${_glintfx_cov_target} TYPE)
    if(_glintfx_cov_target_type STREQUAL "EXECUTABLE" OR _glintfx_cov_target_type STREQUAL "STATIC_LIBRARY")
      glintfx_instrument_coverage(${_glintfx_cov_target})
    endif()
  endforeach()
endif()

# EN: -Wall -Wextra on every authored test/demo-helper target in this directory
#     (AUD-L1-QUALITY Onda 2). BUILDSYSTEM_TARGETS (CMake 3.7+) picks up all targets
#     defined above + any future one, so a new test never silently escapes the gate.
#     No -Werror (mirrors the glintfx lib target, commit 45209e5). Third-party/fetched
#     TUs are not defined here, so they are untouched.
# PT: -Wall -Wextra em todo alvo autoral de teste/demo-helper deste diretório
#     (AUD-L1-QUALITY Onda 2). BUILDSYSTEM_TARGETS (CMake 3.7+) pega todos os alvos
#     acima + qualquer futuro, então um teste novo nunca escapa do gate em silêncio.
#     Sem -Werror (espelha o alvo glintfx, 45209e5). TUs de terceiros/fetchados não
#     são definidos aqui, então ficam intocados.
get_property(_glintfx_test_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY BUILDSYSTEM_TARGETS)
foreach(_t IN LISTS _glintfx_test_targets)
  target_compile_options(${_t} PRIVATE -Wall -Wextra)
endforeach()
