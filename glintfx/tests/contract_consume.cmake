# SPDX-License-Identifier: MPL-2.0
# EN: contract_consume driver (TST-L1-CONTRACT) -- proves glintfx's on-disk INSTALL +
#     find_package(glintfx) + link + run contract end-to-end, without re-building RmlUi
#     (anti-OOM: RmlUi is already built once, in the ENCLOSING ctest run's own build tree
#     ${GLINTFX_BUILD_DIR}; this script only installs those ALREADY-BUILT archives to a
#     throwaway staging prefix, then configures + builds the SEPARATE
#     consumer-example-embed/ project against that staging prefix via CMAKE_PREFIX_PATH -- a
#     single small translation unit, no RmlUi recompilation).
#
#     Steps:
#       1. cmake --install ${GLINTFX_BUILD_DIR} --prefix <staging>
#       2. cmake -S ${CONSUMER_SRC_DIR} -B <staging_build> -DCMAKE_PREFIX_PATH=<staging>
#          (exercises find_package(glintfx CONFIG) -- the installed-tree path, NOT
#          FetchContent -- see consumer-example-embed/CMakeLists.txt's dual-path comment;
#          CMAKE_PREFIX_PATH is what forces the find_package() branch there to succeed instead
#          of falling back to FetchContent)
#       3. cmake --build <staging_build>
#       4. run the resulting consumer_embed binary under Xvfb; its own exit code AND stdout
#          (must contain the literal line "contract_consume: PASS") are the pass/fail signal
#
#     Any failure at any step is FATAL_ERROR (ctest reports it as a failed test with the
#     underlying stdout/stderr attached) -- a gap in the install (e.g. a header the Config
#     forgot to co-install, a missing find_dependency) surfaces HERE, not as a silent skip.
#
#     Reused across BOTH backend legs of the CI matrix (GLINTFX_BACKEND_GLFW=ON and OFF,
#     .github/workflows/ci.yml's `matrix.backend`): each leg's ${GLINTFX_BUILD_DIR} was
#     configured with that leg's backend value, so step 1 installs (and step 2's
#     find_package(glintfx CONFIG) picks up) whichever @GLINTFX_BACKEND_GLFW@ was baked into
#     THAT leg's glintfxConfig.cmake (see glintfx/cmake/glintfxConfig.cmake.in) -- the OFF leg
#     is what proves the embed-only install path end-to-end; the ON leg proves the same
#     machinery still works when glfw3 IS a transitive find_dependency. consumer_embed itself
#     always finds its OWN glfw3 independently either way (see main.cpp's top-of-file
#     comment) -- it does not rely on glintfx's Config to supply GLFW.
#
# PT: driver do contract_consume (TST-L1-CONTRACT) -- prova o contrato de INSTALL em disco +
#     find_package(glintfx) + link + run ponta-a-ponta, sem re-buildar o RmlUi (anti-OOM: o
#     RmlUi já foi buildado uma vez, na própria árvore de build da execução ctest ENVOLVENTE
#     ${GLINTFX_BUILD_DIR}; este script só instala esses arquivos JÁ BUILDADOS num prefixo de
#     staging descartável, depois configura + builda o projeto SEPARADO
#     consumer-example-embed/ contra esse prefixo via CMAKE_PREFIX_PATH -- uma única
#     translation unit pequena, sem recompilar o RmlUi).
#
#     Passos:
#       1. cmake --install ${GLINTFX_BUILD_DIR} --prefix <staging>
#       2. cmake -S ${CONSUMER_SRC_DIR} -B <staging_build> -DCMAKE_PREFIX_PATH=<staging>
#          (exercita find_package(glintfx CONFIG) -- o caminho de árvore instalada, NÃO
#          FetchContent -- ver o comentário de caminho-duplo em
#          consumer-example-embed/CMakeLists.txt; CMAKE_PREFIX_PATH é o que força o ramo
#          find_package() lá a ter sucesso em vez de cair no fallback FetchContent)
#       3. cmake --build <staging_build>
#       4. roda o binário consumer_embed resultante sob Xvfb; seu próprio exit code E o
#          stdout (deve conter a linha literal "contract_consume: PASS") são o sinal de
#          pass/fail
#
#     Qualquer falha em qualquer passo é FATAL_ERROR (o ctest reporta como teste falho com o
#     stdout/stderr subjacente anexado) -- um gap no install (ex.: um header que o Config
#     esqueceu de co-instalar, um find_dependency faltando) aparece AQUI, não como um skip
#     silencioso.
#
#     Reusado nas DUAS pernas de backend da matrix de CI (GLINTFX_BACKEND_GLFW=ON e OFF,
#     `matrix.backend` de .github/workflows/ci.yml): o ${GLINTFX_BUILD_DIR} de cada perna foi
#     configurado com o valor de backend daquela perna, então o passo 1 instala (e o
#     find_package(glintfx CONFIG) do passo 2 pega) o que quer que @GLINTFX_BACKEND_GLFW@
#     tenha sido embarcado no glintfxConfig.cmake DAQUELA perna (ver
#     glintfx/cmake/glintfxConfig.cmake.in) -- a perna OFF é o que prova o caminho de install
#     embed-only ponta-a-ponta; a perna ON prova que a mesma maquinaria continua funcionando
#     quando glfw3 É um find_dependency transitivo. O próprio consumer_embed sempre acha o
#     PRÓPRIO glfw3 independentemente de qualquer forma (ver o comentário no topo de main.cpp)
#     -- não depende do Config da glintfx para suprir GLFW.

if(NOT DEFINED GLINTFX_BUILD_DIR)
  message(FATAL_ERROR "contract_consume: GLINTFX_BUILD_DIR not set")
endif()
if(NOT DEFINED CONSUMER_SRC_DIR)
  message(FATAL_ERROR "contract_consume: CONSUMER_SRC_DIR not set")
endif()
if(NOT DEFINED CMAKE_COMMAND_PATH)
  set(CMAKE_COMMAND_PATH ${CMAKE_COMMAND})
endif()

# EN: Staging root -- $ENV{TMPDIR} if the caller set one (this repo's convention: heavy work
#     goes to /var/tmp, a real-disk tmp, NEVER /tmp which is tmpfs=RAM here -- see CLAUDE.md
#     "Build pesado vai pra /var/tmp"), falling back to /var/tmp directly (never /tmp). A
#     random-suffixed leaf directory avoids collisions between concurrent ctest runs (e.g. the
#     GLFW-ON and GLFW-OFF CI matrix legs staging at the same time) and is removed at the end
#     of every code path, success or failure.
# PT: Raiz de staging -- $ENV{TMPDIR} se o chamador tiver definido (convenção deste repo:
#     trabalho pesado vai pra /var/tmp, tmp em disco real, NUNCA /tmp que aqui é tmpfs=RAM --
#     ver CLAUDE.md "Build pesado vai pra /var/tmp"), caindo para /var/tmp diretamente (nunca
#     /tmp). Um diretório-folha sufixado aleatoriamente evita colisão entre execuções
#     concorrentes de ctest (ex.: as pernas GLFW-ON e GLFW-OFF da matrix de CI fazendo staging
#     ao mesmo tempo) e é removido ao fim de todo caminho de código, sucesso ou falha.
if(DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
  set(_staging_root "$ENV{TMPDIR}")
else()
  set(_staging_root "/var/tmp")
endif()
string(RANDOM LENGTH 8 ALPHABET "0123456789abcdef" _staging_tag)
set(_staging "${_staging_root}/glx_staging_${_staging_tag}")
set(_staging_build "${_staging}_build")

file(MAKE_DIRECTORY ${_staging})

# EN: Step 1 -- install the ALREADY-BUILT glintfx tree (built by the enclosing ctest run,
#     GLINTFX_BUILD_DIR) to the staging prefix. Does not touch GLINTFX_BUILD_DIR itself
#     (--prefix overrides CMAKE_INSTALL_PREFIX for this one invocation only).
# PT: Passo 1 -- instala a árvore glintfx JÁ BUILDADA (pela execução ctest envolvente,
#     GLINTFX_BUILD_DIR) no prefixo de staging. Não toca GLINTFX_BUILD_DIR em si (--prefix
#     sobrepõe CMAKE_INSTALL_PREFIX só para esta invocação).
execute_process(
  COMMAND ${CMAKE_COMMAND_PATH} --install ${GLINTFX_BUILD_DIR} --prefix ${_staging}
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)
if(NOT _rc EQUAL 0)
  file(REMOVE_RECURSE ${_staging} ${_staging_build})
  message(FATAL_ERROR "contract_consume FAIL [install]: rc=${_rc}\n--- stdout ---\n${_out}\n--- stderr ---\n${_err}")
endif()

# EN: Step 2 -- configure consumer-example-embed/ pointed at the staging prefix.
#     CMAKE_PREFIX_PATH forces find_package(glintfx CONFIG) to succeed inside
#     consumer-example-embed/CMakeLists.txt's dual-path lookup (see that file's top-of-file
#     comment) -- the FetchContent fallback branch is NOT taken here, which is exactly the
#     "no RmlUi rebuild" property this test exists to prove. CMAKE_CXX_COMPILER pinned to
#     clang++ -- aligns with README.md's declared toolchain, same as ci.yml's own configure
#     step, rather than silently falling back to whatever cc/c++ the environment defaults to.
# PT: Passo 2 -- configura consumer-example-embed/ apontado pro prefixo de staging.
#     CMAKE_PREFIX_PATH força o find_package(glintfx CONFIG) a ter sucesso dentro da busca de
#     caminho-duplo do consumer-example-embed/CMakeLists.txt (ver o comentário no topo daquele
#     arquivo) -- o ramo de fallback FetchContent NÃO é tomado aqui, que é exatamente a
#     propriedade "sem rebuild do RmlUi" que este teste existe para provar.
#     CMAKE_CXX_COMPILER fixado em clang++ -- alinha com o toolchain declarado no README.md,
#     mesmo passo de configure do próprio ci.yml, em vez de cair silenciosamente no que quer
#     que cc/c++ o ambiente tenha como default.
execute_process(
  COMMAND ${CMAKE_COMMAND_PATH} -S ${CONSUMER_SRC_DIR} -B ${_staging_build}
          -DCMAKE_PREFIX_PATH=${_staging}
          -DCMAKE_BUILD_TYPE=Release
          -DCMAKE_CXX_COMPILER=clang++
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)
if(NOT _rc EQUAL 0)
  file(REMOVE_RECURSE ${_staging} ${_staging_build})
  message(FATAL_ERROR "contract_consume FAIL [configure]: rc=${_rc}\n--- stdout ---\n${_out}\n--- stderr ---\n${_err}")
endif()

# EN: Step 3 -- build ONLY the consumer (1 TU: main.cpp -- links the pre-built
#     libglintfx.a/librmlui.a staged above, does not recompile them).
# PT: Passo 3 -- builda SÓ o consumidor (1 TU: main.cpp -- linka o libglintfx.a/librmlui.a
#     pré-buildado e staged acima, não os recompila).
execute_process(
  COMMAND ${CMAKE_COMMAND_PATH} --build ${_staging_build} -j2
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)
if(NOT _rc EQUAL 0)
  file(REMOVE_RECURSE ${_staging} ${_staging_build})
  message(FATAL_ERROR "contract_consume FAIL [build]: rc=${_rc}\n--- stdout ---\n${_out}\n--- stderr ---\n${_err}")
endif()

# EN: Step 4 -- run the consumer under Xvfb (it opens a hidden GLFW window + a live GL
#     context, same headless requirement as the rest of this suite). unset(WAYLAND_DISPLAY)
#     mirrors run_xvfb.cmake's own defense-in-depth comment (see that file) -- kept here for
#     the same reason, not a fix by itself. WORKING_DIRECTORY = the staging build dir so
#     consumer_embed's ui.load("contract_scene.rml") resolves the scene files copied next to
#     it by consumer-example-embed/CMakeLists.txt's configure_file() calls.
# PT: Passo 4 -- roda o consumidor sob Xvfb (ele abre uma janela GLFW oculta + contexto GL
#     ativo, mesmo requisito headless do resto desta suíte). unset(WAYLAND_DISPLAY) espelha o
#     próprio comentário cinto-e-suspensório do run_xvfb.cmake (ver aquele arquivo) -- mantido
#     aqui pela mesma razão, não é uma correção por si só. WORKING_DIRECTORY = o dir de build
#     de staging para que ui.load("contract_scene.rml") do consumer_embed resolva os arquivos
#     de cena copiados junto dele pelas chamadas configure_file() do
#     consumer-example-embed/CMakeLists.txt.
unset(ENV{WAYLAND_DISPLAY})
set(_consumer_exe "${_staging_build}/consumer_embed")
execute_process(
  COMMAND xvfb-run -a ${_consumer_exe}
  WORKING_DIRECTORY ${_staging_build}
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)

file(REMOVE_RECURSE ${_staging} ${_staging_build})

message(STATUS "${_out}")
if(NOT _err STREQUAL "")
  message(STATUS "${_err}")
endif()

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "contract_consume FAIL [run]: rc=${_rc}")
endif()
if(NOT _out MATCHES "contract_consume: PASS")
  message(FATAL_ERROR "contract_consume FAIL: expected \"contract_consume: PASS\" in stdout, got:\n${_out}")
endif()

message(STATUS "contract_consume: PASS (install + find_package + link + run + pixel assert)")
