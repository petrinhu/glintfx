// SPDX-License-Identifier: Apache-2.0
// EN: capture_framebuffer_loader_guard (CAPTURE-FREE, W22 S8, 2026-07-30) -- proves the ONE
//     guard glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp) has
//     that its two instance-method siblings (App::capture_frame()/UiLayer::capture_frame())
//     do not need: calling it BEFORE glx_gl_load() has EVER run in this process returns a
//     clean `ok == false`, not a crash.
//
//     WHY THIS TEST CAN EXIST AT ALL (a capability its own closest precedent explicitly could
//     NOT achieve): gl_proc_address_embed_sanity.cpp's own header comment states it "cannot
//     isolate" the pre-glx_gl_load() state, because ui_layer.hpp/app.hpp's own test fixtures
//     (WindowGlfw::create()) call glx_gl_load() as a side effect of creating ANY GL context in
//     the first place -- there is no same-process way to observe the "before" state without
//     also never creating a context, at which point there is nothing left to test resolution
//     against. THIS test has no such problem: capture_framebuffer() needs no live GL context
//     of ITS OWN construction step (it has none) to be called -- it is the FIRST and ONLY
//     glintfx call this process ever makes, so the "before glx_gl_load()" state genuinely
//     holds at the point it is called, and stays observable exactly the way this file's own
//     assertions below check.
//
//     WHAT THIS DOES NOT PROVE: a REAL, successful capture once the loader HAS run -- that is
//     capture_framebuffer_smoke.cpp's/capture_framebuffer_state.cpp's own job, both of which
//     create a real WindowGlfw host first (which itself calls glx_gl_load() as a side effect,
//     src/window_glfw.cpp) before calling capture_framebuffer() for real.
//
//     Links `glintfx` directly (NOT ${_embed_dep}) and registers WITHOUT the run_xvfb.cmake
//     wrapper (NOT a GL test -- same "pure unit test, no display needed" shape as
//     image_png_only_sanity.cpp, tests/CMakeLists.txt): no window, no GLFW, no display server
//     of any kind is touched by this executable.
// PT: capture_framebuffer_loader_guard (CAPTURE-FREE, W22 S8, 2026-07-30) -- prova a ÚNICA
//     guarda que glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp)
//     tem e que as duas irmãs de método de instância dela (App::capture_frame()/
//     UiLayer::capture_frame()) não precisam: chamá-la ANTES do glx_gl_load() ter rodado
//     alguma vez neste processo devolve um `ok == false` limpo, não um crash.
//
//     POR QUE ESTE TESTE CONSEGUE EXISTIR (uma capacidade que o próprio precedente mais
//     próximo dele explicitamente NÃO conseguiu alcançar): o próprio comentário de cabeçalho
//     de gl_proc_address_embed_sanity.cpp declara que "não consegue isolar" o estado
//     pré-glx_gl_load(), porque as próprias fixtures de teste de ui_layer.hpp/app.hpp
//     (WindowGlfw::create()) chamam glx_gl_load() como efeito colateral de criar QUALQUER
//     contexto GL, pra começo de conversa -- não há forma no mesmo processo de observar o
//     estado "antes" sem também nunca criar contexto nenhum, ponto em que não sobra nada pra
//     testar a resolução contra. ESTE teste não tem esse problema: capture_framebuffer() não
//     precisa de contexto GL vivo de nenhum passo de construção PRÓPRIO dele (não tem nenhum)
//     pra ser chamada -- ela é a PRIMEIRA e ÚNICA chamada da glintfx que este processo faz,
//     então o estado "antes do glx_gl_load()" de fato vale no ponto em que é chamada, e
//     permanece observável exatamente do jeito que as próprias asserções deste arquivo abaixo
//     checam.
//
//     O QUE ISTO NÃO PROVA: uma captura REAL, com sucesso, uma vez que o loader TENHA rodado
//     -- isso é o próprio trabalho de capture_framebuffer_smoke.cpp/
//     capture_framebuffer_state.cpp, que criam um host WindowGlfw real primeiro (que por si só
//     chama glx_gl_load() como efeito colateral, src/window_glfw.cpp) antes de chamar
//     capture_framebuffer() de verdade.
//
//     Linka `glintfx` diretamente (NÃO ${_embed_dep}) e é registrado SEM o wrapper
//     run_xvfb.cmake (NÃO é um teste GL -- mesma forma "teste unit puro, sem display
//     necessário" de image_png_only_sanity.cpp, tests/CMakeLists.txt): nenhuma janela, nenhum
//     GLFW, nenhum servidor de display de qualquer tipo é tocado por este executável.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  int failures = 0;

  // EN: The FIRST glintfx call this process ever makes -- no App, no UiLayer, no Draw2d, no
  //     WindowGlfw anywhere above this line. glx_gl_load() has therefore NEVER run in this
  //     process yet.
  // PT: A PRIMEIRA chamada da glintfx que este processo faz -- nenhum App, nenhum UiLayer,
  //     nenhum Draw2d, nenhum WindowGlfw em lugar nenhum acima desta linha. O glx_gl_load()
  //     portanto NUNCA rodou neste processo ainda.
  const glintfx::CapturedFramebuffer frame = glintfx::capture_framebuffer(0, 0, 16, 16);

  if (frame.ok) {
    std::puts(
        "FAIL: capture_framebuffer() returned ok == true before glx_gl_load() ever ran -- "
        "either the loader-not-ready guard did not fire, or something upstream of this test "
        "silently initialised the GL loader");
    ++failures;
  }
  if (frame.width != 0 || frame.height != 0) {
    std::fprintf(stderr, "FAIL: width/height not zeroed: %dx%d\n", frame.width, frame.height);
    ++failures;
  }
  if (frame.pixels != nullptr) {
    std::puts("FAIL: pixels non-null despite ok == false");
    ++failures;
  }
  if (frame.byte_count != 0) {
    std::fprintf(stderr, "FAIL: byte_count=%zu, expected 0\n", frame.byte_count);
    ++failures;
  }

  // EN: Guard order sanity: the w<=0/h<=0 fail-high guard must ALSO still hold, even though
  //     the loader has still not run -- both guards are independent, neither masks the other.
  // PT: Sanidade de ordem de guarda: a guarda fail-high w<=0/h<=0 TAMBÉM precisa continuar
  //     valendo, mesmo com o loader ainda não tendo rodado -- as duas guardas são
  //     independentes, nenhuma mascara a outra.
  const glintfx::CapturedFramebuffer degenerate = glintfx::capture_framebuffer(0, 0, 0, 16);
  if (degenerate.ok) {
    std::puts("FAIL: capture_framebuffer() with w=0 returned ok == true");
    ++failures;
  }

  if (failures == 0) {
    std::puts(
        "capture_framebuffer_loader_guard: PASS (crashed-would-be call degraded to a clean "
        "ok == false, never touching a null GL function pointer)");
    return 0;
  }
  std::fprintf(stderr, "capture_framebuffer_loader_guard: %d check(s) FAILED\n", failures);
  return failures;
}
