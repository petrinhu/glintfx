# SPDX-License-Identifier: MPL-2.0
#
# EN: unset WAYLAND_DISPLAY before launching under Xvfb (X11-only) —
#     defense-in-depth hermeticity measure (TST-L1-MEM), NOT a complete fix by
#     itself (verified empirically 2026-07-02 — see below). On a dev machine
#     running a live Wayland desktop session, WAYLAND_DISPLAY leaks through
#     ctest's inherited environment into the xvfb-run child process; GLFW's
#     platform auto-detection then prefers the Wayland backend over the Xvfb
#     DISPLAY, which for a *decorated* window pulls in libdecor's GTK3
#     client-side-decoration plugin -> Pango -> Cairo -> fontconfig -> GLib
#     just to measure a title-bar label. All of that machinery leaks
#     internally at process exit (global font/type caches) and shows up as
#     noise in the GLINTFX_SANITIZE (ASan/LSan) nightly run.
#     CAVEAT (confirmed by testing): unsetting WAYLAND_DISPLAY alone does NOT
#     stop GLFW from picking Wayland on this host, because libwayland-client's
#     wl_display_connect(NULL) falls back to the default socket name
#     "wayland-0" inside $XDG_RUNTIME_DIR when the env var is absent, and that
#     socket is still live (it belongs to the host's real desktop session).
#     Forcing X11 unconditionally would require glfwInitHint(GLFW_PLATFORM,
#     GLFW_PLATFORM_X11) in src/window_glfw.cpp — a production behaviour
#     change out of scope for this gate (would force every glintfx::App
#     consumer off Wayland, not just test fixtures; needs an explicit
#     architecture decision). The actual gate-hermeticity guarantee for this
#     noise cluster is therefore tests/lsan_suppressions.txt, not this unset —
#     kept here anyway as a correct, harmless step for any host where it DOES
#     fully work (e.g. no stray XDG_RUNTIME_DIR wayland socket).
#     GitHub Actions / Forgejo Actions runners are headless (no desktop
#     session, no WAYLAND_DISPLAY, no wayland-0 socket) so none of this noise
#     cluster is reachable there in the first place.
# PT: remove WAYLAND_DISPLAY antes de rodar sob Xvfb (só X11) — medida de
#     hermeticidade cinto-e-suspensório (TST-L1-MEM), NÃO uma correção
#     completa por si só (verificado empiricamente em 2026-07-02 — ver
#     abaixo). Numa máquina dev com sessão Wayland ativa, WAYLAND_DISPLAY
#     vaza pelo ambiente herdado do ctest para o processo filho do xvfb-run; a
#     auto-detecção de plataforma do GLFW então prefere o backend Wayland ao
#     DISPLAY do Xvfb, que para uma janela *decorada* traz o plugin de
#     decoração client-side GTK3 do libdecor -> Pango -> Cairo -> fontconfig
#     -> GLib só para medir um label de title-bar. Toda essa maquinaria vaza
#     internamente na saída do processo (cache global de fonte/tipo) e
#     aparece como ruído na execução noturna do GLINTFX_SANITIZE (ASan/LSan).
#     RESSALVA (confirmada por teste): remover WAYLAND_DISPLAY sozinho NÃO
#     impede o GLFW de escolher Wayland nesta máquina, porque o
#     wl_display_connect(NULL) do libwayland-client cai de volta para o nome
#     de socket default "wayland-0" dentro de $XDG_RUNTIME_DIR quando a env
#     var está ausente, e esse socket continua vivo (pertence à sessão de
#     desktop real do host). Forçar X11 incondicionalmente exigiria
#     glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11) em src/window_glfw.cpp —
#     uma mudança de comportamento de produção fora do escopo deste gate
#     (forçaria todo consumidor de glintfx::App para fora do Wayland, não só
#     os fixtures de teste; exige decisão explícita de arquitetura). A
#     garantia real de hermeticidade do gate para este cluster de ruído é
#     portanto tests/lsan_suppressions.txt, não este unset — mantido aqui
#     mesmo assim como passo correto e inofensivo para qualquer host onde ELE
#     funcione por completo (ex.: sem socket wayland avulso em
#     XDG_RUNTIME_DIR). Runners do GitHub Actions / Forgejo Actions são
#     headless (sem sessão de desktop, sem WAYLAND_DISPLAY, sem socket
#     wayland-0) então nada deste cluster de ruído é alcançável lá de saída.
unset(ENV{WAYLAND_DISPLAY})

execute_process(COMMAND xvfb-run -a ${EXE} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "test failed rc=${rc}")
endif()
