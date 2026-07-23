# SPDX-License-Identifier: MPL-2.0
#
# EN: unset WAYLAND_DISPLAY AND isolate XDG_RUNTIME_DIR before launching under
#     Xvfb (X11-only) -- defense-in-depth hermeticity measure (TST-L1-MEM),
#     now a COMPLETE fix (closed 2026-07-23, QA-XVFBISO -- see below for the
#     history of why it wasn't). On a dev machine running a live Wayland
#     desktop session, WAYLAND_DISPLAY leaks through ctest's inherited
#     environment into the xvfb-run child process; GLFW's platform
#     auto-detection then prefers the Wayland backend over the Xvfb DISPLAY,
#     which for a *decorated* window pulls in libdecor's GTK3
#     client-side-decoration plugin -> Pango -> Cairo -> fontconfig -> GLib
#     just to measure a title-bar label. All of that machinery leaks
#     internally at process exit (global font/type caches) and shows up as
#     noise in the GLINTFX_SANITIZE (ASan/LSan) nightly run.
#     CAVEAT, formerly unclosed (confirmed by testing 2026-07-02): unsetting
#     WAYLAND_DISPLAY alone does NOT stop GLFW from picking Wayland on a host
#     with a live desktop session, because libwayland-client's
#     wl_display_connect(NULL) falls back to the default socket name
#     "wayland-0" inside $XDG_RUNTIME_DIR when the env var is absent, and that
#     socket is still live (it belongs to the host's real desktop session).
#     WORSE than a merely incomplete fix: on 2026-07-23 this exact gap let a
#     window-mode test suite run without the caller exporting a fake
#     XDG_RUNTIME_DIR (nothing in this file enforced it) and the test windows
#     opened for real on the lead's live Wayland session (QA-XVFBISO). The
#     comment documented the caveat; it did not implement it -- a half
#     protection that reads as complete is worse than none, because nothing
#     downstream double-checks it.
#     THE FIX (2026-07-23): point XDG_RUNTIME_DIR at a fresh, empty,
#     mode-0700 directory created by THIS wrapper for every single
#     invocation (one `cmake -P run_xvfb.cmake` process per ctest test, so
#     one fake runtime dir per test -- concurrent `ctest -j` runs never share
#     one, `mktemp -d` allocates it atomically so there is no
#     create/collide race). With no "wayland-0" socket reachable inside that
#     directory, wl_display_connect(NULL) fails outright and GLFW's
#     auto-detection falls back to X11 -- exactly the Xvfb DISPLAY this
#     wrapper already sets up. This closes the gap for test fixtures WITHOUT
#     the production-behaviour change a forced
#     glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11) in src/window_glfw.cpp
#     would have been (that would have forced every glintfx::App consumer off
#     Wayland, not just test fixtures -- out of scope, still not done, still
#     would need an explicit architecture decision if ever wanted). The fake
#     directory is removed again after the test process exits, win or lose.
#     GitHub Actions / Forgejo Actions runners are headless (no desktop
#     session, no WAYLAND_DISPLAY, no wayland-0 socket to begin with) so this
#     change is inert there: XDG_RUNTIME_DIR gets pointed at an equally-empty
#     directory that no code path needed protecting against in the first
#     place.
# PT: remove WAYLAND_DISPLAY E isola XDG_RUNTIME_DIR antes de rodar sob Xvfb
#     (só X11) -- medida de hermeticidade cinto-e-suspensório (TST-L1-MEM),
#     agora uma correção COMPLETA (fechada em 2026-07-23, QA-XVFBISO -- ver
#     abaixo o histórico de por que não era). Numa máquina dev com sessão
#     Wayland ativa, WAYLAND_DISPLAY vaza pelo ambiente herdado do ctest para
#     o processo filho do xvfb-run; a auto-detecção de plataforma do GLFW
#     então prefere o backend Wayland ao DISPLAY do Xvfb, que para uma janela
#     *decorada* traz o plugin de decoração client-side GTK3 do libdecor ->
#     Pango -> Cairo -> fontconfig -> GLib só para medir um label de
#     title-bar. Toda essa maquinaria vaza internamente na saída do processo
#     (cache global de fonte/tipo) e aparece como ruído na execução noturna do
#     GLINTFX_SANITIZE (ASan/LSan).
#     RESSALVA, antes não fechada (confirmada por teste em 2026-07-02):
#     remover WAYLAND_DISPLAY sozinho NÃO impede o GLFW de escolher Wayland
#     numa máquina com sessão de desktop ativa, porque o
#     wl_display_connect(NULL) do libwayland-client cai de volta para o nome
#     de socket default "wayland-0" dentro de $XDG_RUNTIME_DIR quando a env
#     var está ausente, e esse socket continua vivo (pertence à sessão de
#     desktop real do host). PIOR que uma correção meramente incompleta: em
#     2026-07-23 essa mesma brecha deixou uma suíte de testes de window-mode
#     rodar sem o chamador exportar um XDG_RUNTIME_DIR falso (nada neste
#     arquivo exigia isso) e as janelas de teste abriram de verdade na sessão
#     Wayland ao vivo do líder (QA-XVFBISO). O comentário documentava a
#     ressalva; não a implementava -- uma proteção pela metade que parece
#     completa é pior que nenhuma, porque nada depois dela confere de novo.
#     A CORREÇÃO (2026-07-23): aponta XDG_RUNTIME_DIR para um diretório novo,
#     vazio, modo 0700, criado por ESTE wrapper a cada invocação (um processo
#     `cmake -P run_xvfb.cmake` por teste do ctest, logo um diretório de
#     runtime falso por teste -- execuções concorrentes de `ctest -j` nunca
#     compartilham um, o `mktemp -d` aloca de forma atômica então não há
#     corrida de criação/colisão). Sem nenhum socket "wayland-0" alcançável
#     dentro desse diretório, o wl_display_connect(NULL) falha de vez e a
#     auto-detecção do GLFW cai para X11 -- exatamente o DISPLAY do Xvfb que
#     este wrapper já monta. Isso fecha a brecha para os fixtures de teste SEM
#     a mudança de comportamento de produção que um
#     glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11) forçado em
#     src/window_glfw.cpp teria sido (forçaria todo consumidor de
#     glintfx::App para fora do Wayland, não só os fixtures de teste -- fora
#     de escopo, ainda não feito, ainda exigiria decisão explícita de
#     arquitetura se algum dia for desejado). O diretório falso é removido de
#     novo depois que o processo de teste sai, passe ou falhe.
#     Runners do GitHub Actions / Forgejo Actions são headless (sem sessão de
#     desktop, sem WAYLAND_DISPLAY, sem socket wayland-0 de saída) então esta
#     mudança é inócua lá: XDG_RUNTIME_DIR só passa a apontar para um
#     diretório igualmente vazio que nenhum caminho de código precisava de
#     proteção ali.
unset(ENV{WAYLAND_DISPLAY})

# EN: Isolated, per-invocation fake XDG_RUNTIME_DIR (see rationale above).
#     mktemp -d creates it atomically with mode 0700 -- no separate chmod
#     step needed, no window for another process to race into it. Honours
#     $TMPDIR when the caller has set one (e.g. the project's own
#     `export TMPDIR=/var/tmp` convention for heavy builds), falls back to
#     /tmp otherwise -- either is fine here since this directory holds no
#     socket files of consequence, just an empty jail for wl_display_connect
#     to fail against.
# PT: XDG_RUNTIME_DIR falso, isolado por invocação (ver racional acima).
#     mktemp -d cria de forma atômica com modo 0700 -- sem passo de chmod
#     separado, sem janela pra outro processo entrar na corrida. Respeita
#     $TMPDIR quando o chamador já definiu um (ex.: a convenção do próprio
#     projeto de `export TMPDIR=/var/tmp` para builds pesados), cai para /tmp
#     senão -- tanto faz aqui, já que este diretório não guarda nenhum
#     arquivo de socket relevante, só uma cela vazia pro
#     wl_display_connect falhar contra ela.
if(DEFINED ENV{TMPDIR})
  set(_glintfx_xvfb_tmp_base "$ENV{TMPDIR}")
else()
  set(_glintfx_xvfb_tmp_base "/tmp")
endif()

execute_process(
  COMMAND mktemp -d "${_glintfx_xvfb_tmp_base}/glintfx-xvfb-runtime.XXXXXX"
  OUTPUT_VARIABLE _glintfx_fake_xdg_runtime
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE _glintfx_mktemp_rc)
if(NOT _glintfx_mktemp_rc EQUAL 0)
  message(FATAL_ERROR "run_xvfb.cmake: mktemp -d failed rc=${_glintfx_mktemp_rc} (could not build isolated XDG_RUNTIME_DIR)")
endif()

set(ENV{XDG_RUNTIME_DIR} "${_glintfx_fake_xdg_runtime}")

execute_process(COMMAND xvfb-run -a ${EXE} RESULT_VARIABLE rc)

# EN: clean up the fake runtime dir regardless of pass/fail, then surface the
#     test's own result -- REMOVE_RECURSE never itself fails the test.
# PT: limpa o diretório de runtime falso independente de passar/falhar, e só
#     então propaga o resultado do próprio teste -- REMOVE_RECURSE nunca faz
#     o teste falhar por conta própria.
file(REMOVE_RECURSE "${_glintfx_fake_xdg_runtime}")

if(NOT rc EQUAL 0)
  message(FATAL_ERROR "test failed rc=${rc}")
endif()
