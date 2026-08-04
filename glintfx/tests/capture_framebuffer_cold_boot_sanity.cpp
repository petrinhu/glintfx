// SPDX-License-Identifier: Apache-2.0
// EN: capture_framebuffer_cold_boot_sanity (CAPTURE-FREE-LOADER, W27, 2026-08-04) -- proves
//     the fix this task exists for: a caller with NO glintfx instance of ANY kind (no App, no
//     UiLayer, no glintfx::WindowGlfw -- this library's own GL-context test helper, whose own
//     create() itself calls glx_gl_load() as a side effect, src/window_glfw.cpp) that creates
//     a GL context through RAW GLFW calls, then calls glintfx::capture_framebuffer() as the
//     FIRST glintfx call this process ever makes, gets a REAL, non-empty capture back -- not
//     the clean-but-useless CapturedFramebuffer{} the loader-not-ready guard handed back
//     unconditionally before this fix (see frame_capture.cpp's own updated guard comment for
//     the full design rationale).
//
//     WHY THIS TEST NEEDS ITS OWN RAW GLFW CALLS, NOT glintfx::WindowGlfw (the helper most
//     other GL tests in this suite use): WindowGlfw::create() calls glx_gl_load() itself,
//     right after glfwMakeContextCurrent() (src/window_glfw.cpp) -- using it here would
//     populate the loader table BEFORE this file's own call to capture_framebuffer(), which
//     is exactly the state this test exists to rule out. Same technique, same reason, as
//     render_loader_guard_sanity.cpp's own raw-GLFW context (that file's own header comment
//     spells out the identical hazard for RenderGl3::init()'s sibling guard) -- this file
//     mirrors WindowGlfw::create()'s own hint sequence (GLFW_CONTEXT_VERSION_MAJOR/MINOR=3.3,
//     GLFW_OPENGL_PROFILE=CORE) so the context itself is equivalent, while deliberately
//     skipping the glx_gl_load() call that follows it there.
//
//     COLD-PROCESS GUARANTEE: this executable's main() makes exactly one glfw*() call
//     sequence and exactly one glintfx call (capture_framebuffer()) before the first
//     assertion -- no App, UiLayer, Engine, Draw2d, or WindowGlfw is constructed anywhere in
//     this translation unit, and this file links no other glintfx test helper that could.
//     Because the process is the unit of isolation for this claim (glx_ is a process-global
//     table -- see frame_capture.cpp's own guard comment and gl_loader.h's own doc-comment;
//     CTest runs each test binary in its own process), a single dedicated executable is both
//     necessary and sufficient -- no fixture cleanup between "cases" could undo a
//     glx_gl_load() call already made earlier in the SAME process, so this file intentionally
//     has only ONE cold-boot case, not several sharing a binary.
//
//     NEGATIVE PROOF: verified by hand during this task's own development (reverting
//     frame_capture.cpp's own lazy-init call, leaving only the ORIGINAL check-and-give-up
//     guard, turns this test's [1] check red) -- not re-encoded here as a runtime toggle, the
//     same mutation-testing discipline this codebase already applies to a guard this narrow
//     (one call, one re-check).
//
//     WHAT THIS DOES NOT PROVE: pixel-CONTENT correctness of the readback pipeline itself
//     (coordinate contract, top-down flip, PACK_ALIGNMENT) -- that is
//     capture_framebuffer_smoke.cpp's own job, which already covers it through a
//     loader-already-populated WindowGlfw host. This file's [2] check below (after the
//     cold-boot assertion) reuses the NOW-populated table to paint a known colour with raw GL
//     calls and capture it again -- a cheap, additional confidence check that the lazy
//     glx_gl_load() call populated the WHOLE table (glClear/glBindFramebuffer/etc.), not
//     merely glx_glReadPixels in isolation -- but the authoritative coordinate/flip/alignment
//     contract tests remain the smoke test's, not duplicated here.
//
//     REGISTERED UNCONDITIONALLY -- RUNS IN BOTH BUILD CONFIGS, same reasoning as
//     render_loader_guard_sanity.cpp/ui_layer_gl_proc_resolver_sanity.cpp (both link `glintfx
//     glfw` directly, not `${_embed_dep}`, for the identical "avoid WindowGlfw's own
//     glx_gl_load() side effect" reason above): by the point tests/CMakeLists.txt reaches
//     this block, the `glfw` imported target already exists in EITHER config
//     (GLINTFX_BACKEND_GLFW=ON: find_package(glfw3) at top-level CMakeLists.txt;
//     GLINTFX_BACKEND_GLFW=OFF: this file's own `_glintfx_test_ctx` block's own
//     find_package(glfw3) call) -- so this test needs no `if(GLINTFX_BACKEND_GLFW)` guard and
//     no downgrade: a genuinely cold, real GL context is available to construct in BOTH
//     configs without going through any glintfx entity.
// PT: capture_framebuffer_cold_boot_sanity (CAPTURE-FREE-LOADER, W27, 2026-08-04) -- prova o
//     conserto pelo qual esta tarefa existe: um chamador SEM instância glintfx de NENHUM tipo
//     (nenhum App, nenhum UiLayer, nenhum glintfx::WindowGlfw -- o próprio helper de teste de
//     contexto GL desta biblioteca, cujo próprio create() já chama glx_gl_load() como efeito
//     colateral, src/window_glfw.cpp) que cria um contexto GL por chamadas GLFW CRUAS, e então
//     chama glintfx::capture_framebuffer() como a PRIMEIRA chamada da glintfx que este
//     processo faz, recebe de volta uma captura REAL, não-vazia -- não o
//     CapturedFramebuffer{} limpo mas inútil que a guarda de loader-não-pronto devolvia
//     incondicionalmente antes deste conserto (ver o próprio comentário atualizado da guarda
//     em frame_capture.cpp pro racional completo de desenho).
//
//     POR QUE ESTE TESTE PRECISA DAS PRÓPRIAS CHAMADAS GLFW CRUAS, NÃO glintfx::WindowGlfw (o
//     helper que a maioria dos outros testes GL desta suíte usa): o próprio
//     WindowGlfw::create() chama glx_gl_load(), logo após glfwMakeContextCurrent()
//     (src/window_glfw.cpp) -- usá-lo aqui popularia a tabela do loader ANTES da própria
//     chamada deste arquivo a capture_framebuffer(), exatamente o estado que este teste
//     existe pra descartar. Mesma técnica, mesmo motivo, do próprio contexto GLFW cru de
//     render_loader_guard_sanity.cpp (o próprio comentário de cabeçalho daquele arquivo
//     descreve o perigo idêntico pra guarda irmã de RenderGl3::init()) -- este arquivo
//     espelha a própria sequência de hints de WindowGlfw::create()
//     (GLFW_CONTEXT_VERSION_MAJOR/MINOR=3.3, GLFW_OPENGL_PROFILE=CORE) pra que o contexto em
//     si seja equivalente, pulando deliberadamente a chamada glx_gl_load() que vem depois
//     dela lá.
//
//     GARANTIA DE PROCESSO FRIO: o main() deste executável faz exatamente uma sequência de
//     chamadas glfw*() e exatamente uma chamada da glintfx (capture_framebuffer()) antes da
//     primeira asserção -- nenhum App, UiLayer, Engine, Draw2d ou WindowGlfw é construído em
//     lugar nenhum desta unidade de tradução, e este arquivo não linka nenhum outro helper de
//     teste da glintfx que pudesse. Como o processo é a unidade de isolamento pra esta
//     alegação (glx_ é uma tabela process-global -- ver o próprio comentário de guarda de
//     frame_capture.cpp e o próprio doc-comment de gl_loader.h; o CTest roda cada binário de
//     teste no próprio processo), um único executável dedicado é necessário e suficiente --
//     nenhuma limpeza de fixture entre "casos" desfaria uma chamada glx_gl_load() já feita
//     antes no MESMO processo, então este arquivo tem de propósito só UM caso de cold-boot,
//     não vários compartilhando um binário.
//
//     PROVA NEGATIVA: verificada à mão durante o próprio desenvolvimento desta tarefa
//     (reverter a própria chamada de lazy-init de frame_capture.cpp, deixando só a guarda
//     ORIGINAL de checar-e-desistir, torna a checagem [1] deste teste vermelha) -- não
//     re-codificada aqui como toggle em tempo de execução, a mesma disciplina de
//     mutation-testing que este código-base já aplica pra uma guarda deste tamanho (uma
//     chamada, uma rechecagem).
//
//     O QUE ISTO NÃO PROVA: corretude de CONTEÚDO de pixel do próprio pipeline de readback
//     (contrato de coordenadas, inversão top-down, PACK_ALIGNMENT) -- isso é o próprio
//     trabalho de capture_framebuffer_smoke.cpp, que já cobre isso através de um host
//     WindowGlfw com o loader já populado. A checagem [2] deste arquivo abaixo (depois da
//     asserção de cold-boot) reusa a tabela AGORA populada pra pintar uma cor conhecida com
//     chamadas GL cruas e capturá-la de novo -- uma checagem de confiança adicional, barata,
//     de que a chamada lazy glx_gl_load() populou a tabela INTEIRA (glClear/
//     glBindFramebuffer/etc.), não só o glx_glReadPixels isolado -- mas os testes
//     autoritativos de contrato de coordenadas/inversão/alinhamento continuam sendo os do
//     smoke test, não duplicados aqui.
//
//     REGISTRADO INCONDICIONALMENTE -- RODA NAS DUAS CONFIGS DE BUILD, mesmo racional de
//     render_loader_guard_sanity.cpp/ui_layer_gl_proc_resolver_sanity.cpp (os dois linkam
//     `glintfx glfw` diretamente, não `${_embed_dep}`, pelo motivo idêntico "evitar o efeito
//     colateral glx_gl_load() do próprio WindowGlfw" acima): no ponto em que
//     tests/CMakeLists.txt alcança este bloco, o alvo importado `glfw` já existe em QUALQUER
//     config (GLINTFX_BACKEND_GLFW=ON: find_package(glfw3) no CMakeLists.txt de nível-topo;
//     GLINTFX_BACKEND_GLFW=OFF: o próprio bloco `_glintfx_test_ctx` deste arquivo, próprio
//     find_package(glfw3)) -- então este teste não precisa de guarda `if(GLINTFX_BACKEND_GLFW)`
//     nem de downgrade nenhum: um contexto GL genuinamente frio e real está disponível pra
//     construir nas DUAS configs sem passar por entidade glintfx nenhuma.
// Copyright (c) 2026 Petrus Silva Costa
// EN: GLFW_INCLUDE_NONE -- glfw3.h must not pull in ANY system GL header of its own (e.g.
//     GL/gl.h) before gl_loader.h/glcorearb.h below get to declare the PFN*PROC typedefs this
//     file's own [2] phase needs -- the system header's own, older/partial typedefs would
//     otherwise collide (measured: a plain, un-flagged #include order here fails to compile,
//     "unknown type name 'PFNGLDRAWARRAYSPROC'", the system glext.h's own EXT variant winning
//     the name instead). gl_loader.h is included AFTER, so it is the ONE source of truth for
//     every PFNGL*PROC typedef in this translation unit.
// PT: GLFW_INCLUDE_NONE -- o glfw3.h não pode puxar NENHUM header GL de sistema próprio (ex.:
//     GL/gl.h) antes do gl_loader.h/glcorearb.h abaixo poderem declarar os typedefs
//     PFN*PROC que a própria fase [2] deste arquivo precisa -- os typedefs mais antigos/
//     parciais do header de sistema colidiriam de outra forma (medido: a ordem de #include
//     simples, sem esta flag, aqui falha ao compilar, "unknown type name
//     'PFNGLDRAWARRAYSPROC'", com a própria variante EXT do glext.h de sistema vencendo o
//     nome). gl_loader.h é incluído DEPOIS, então é a ÚNICA fonte de verdade pra todo typedef
//     PFNGL*PROC nesta unidade de tradução.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glintfx/glintfx.hpp>
#include "gl_loader.h" // EN: only used AFTER [1]'s cold-boot call below, once the table is populated -- see this file's own top comment, check [2]. PT: só usado DEPOIS da própria chamada de cold-boot de [1] abaixo, uma vez que a tabela está populada -- ver o próprio comentário de topo deste arquivo, checagem [2].
#include <cstdio>

int main() {
  int failures = 0;

  if (!glfwInit()) {
    std::puts("SKIP: glfwInit() failed -- no display available");
    return 0;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  const int W = 32, H = 32;
  GLFWwindow* win =
      glfwCreateWindow(W, H, "capture_framebuffer_cold_boot_sanity", nullptr, nullptr);
  if (!win) {
    std::puts("SKIP: glfwCreateWindow() failed -- no GL context available");
    glfwTerminate();
    return 0;
  }
  glfwMakeContextCurrent(win);

  // ---------------------------------------------------------------------------
  // [1] THE cold-boot moment: the FIRST glintfx call this process ever makes, with glx_
  //     never having been populated by anything (no glx_gl_load() call anywhere above this
  //     line, in this file or any callee it invoked).
  // ---------------------------------------------------------------------------
  const glintfx::CapturedFramebuffer cold = glintfx::capture_framebuffer(0, 0, W, H);

  if (!cold.ok) {
    std::puts(
        "FAIL: [1] capture_framebuffer() returned ok == false on cold boot -- the lazy-init "
        "guard did not populate the GL loader table (CAPTURE-FREE-LOADER regression)");
    ++failures;
  }
  if (cold.width != W || cold.height != H) {
    std::fprintf(stderr, "FAIL: [1] cold boot size %dx%d, expected %dx%d\n", cold.width,
                 cold.height, W, H);
    ++failures;
  }
  if (cold.pixels == nullptr) {
    std::puts("FAIL: [1] cold boot pixels == nullptr despite ok == true");
    ++failures;
  }
  const std::size_t expected_bytes =
      static_cast<std::size_t>(W) * static_cast<std::size_t>(H) * 4;
  if (cold.byte_count != expected_bytes) {
    std::fprintf(stderr, "FAIL: [1] cold boot byte_count=%zu, expected %zu\n", cold.byte_count,
                 expected_bytes);
    ++failures;
  }

  if (failures != 0) {
    // EN: [1] itself failed -- [2] below assumes the table IS populated (it is not, on this
    //     path), so running it would just add noise on top of an already-diagnosed failure.
    // PT: A própria [1] falhou -- a [2] abaixo presume que a tabela ESTÁ populada (não está,
    //     neste caminho), então rodá-la só acrescentaria ruído por cima de uma falha já
    //     diagnosticada.
    std::fprintf(stderr, "capture_framebuffer_cold_boot_sanity: %d check(s) FAILED\n", failures);
    glfwDestroyWindow(win);
    glfwTerminate();
    return failures;
  }
  std::puts("capture_framebuffer_cold_boot_sanity: [1] PASS (cold-boot lazy-init worked)");

  // ---------------------------------------------------------------------------
  // [2] The lazy call above must have populated the WHOLE table, not merely
  //     glx_glReadPixels: paint a known colour with raw GL calls (safe now -- the table is
  //     populated) and capture it again, proving glClear/glBindFramebuffer/glReadPixels all
  //     resolved together.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(10.f / 255.f, 200.f / 255.f, 90.f / 255.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  const glintfx::CapturedFramebuffer painted = glintfx::capture_framebuffer(0, 0, W, H);
  if (!painted.ok || !painted.pixels) {
    std::puts("FAIL: [2] post-lazy-init capture_framebuffer() ok == false or pixels null");
    ++failures;
  } else {
    const unsigned char r = painted.pixels[0];
    const unsigned char g = painted.pixels[1];
    const unsigned char b = painted.pixels[2];
    auto close = [](int a, int b2, int tol = 6) { return (a - b2 >= -tol) && (a - b2 <= tol); };
    if (!close(r, 10) || !close(g, 200) || !close(b, 90)) {
      std::fprintf(stderr,
                   "FAIL: [2] pixel (%d,%d,%d) does not match painted colour (10,200,90) -- "
                   "lazy-init populated glx_glReadPixels but not the rest of the table\n",
                   r, g, b);
      ++failures;
    } else {
      std::puts(
          "capture_framebuffer_cold_boot_sanity: [2] PASS (whole table resolved by lazy-init, "
          "not just glReadPixels)");
    }
  }

  glfwDestroyWindow(win);
  glfwTerminate();

  if (failures == 0) {
    std::puts("capture_framebuffer_cold_boot_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "capture_framebuffer_cold_boot_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
