// SPDX-License-Identifier: Apache-2.0
// EN: render_loader_guard_sanity (GLPROC-CRASH, 2026-07-30) -- contract/regression test for
//     `RenderGl3::init()`'s loader-not-ready guard (src/render_gl3.cpp, right before
//     `RmlGL3::Initialize`/`new Impl`). This is the DIRECT regression test for a SIGSEGV
//     measured by execution (gdb) while verifying `gl_proc_address()`'s own loader-skip caveat
//     (see the doc-comment on `glintfx::gl_proc_address()`, gl_proc.hpp, and this library's own
//     CHANGELOG/commit `f0cacbb` for the full writeup): before the guard,
//     `glintfx::UiLayer{.load_gl = false}` (the ORIGINAL field name at the time this bug was
//     found), constructed as the FIRST glintfx entity in a process (so `glx_gl_load()` never ran
//     anywhere before it), crashed inside its own constructor -- `Engine::attach()` ->
//     `RenderGl3::init()` -> `new Impl` -> `RenderInterface_GL3`'s own constructor (RmlUi)
//     compiling GL3 shaders through a still-NULL `glCreateShader` (== `glx_glCreateShader`)
//     function pointer.
//
//     RENAMED, SAME CONTRACT (`SEED-LOADGL-NOME`, 2026-08-04): this file's own construction below
//     now uses `.assume_gl_loaded = true`, the canonical replacement for `.load_gl = false` (see
//     `UiLayerConfig`'s own doc-comment, `ui_layer.hpp`, for the full rename rationale and the
//     inverted-polarity warning). The regression this test proves is UNCHANGED -- the guard in
//     `RenderGl3::init()` reads a function-pointer sentinel, not either config field by name, so
//     the SAME crash-class protection applies regardless of which spelling a caller reaches it
//     through.
//
//     DELIBERATELY DOES NOT USE `_glintfx_test_ctx`/`WindowGlfw` (unlike every other embed test
//     in this suite, e.g. `ui_layer_attach.cpp`, `gl_proc_address_embed_sanity.cpp`): that
//     helper's own `create()` (src/window_glfw.cpp) calls `glx_gl_load()` unconditionally as a
//     side effect of establishing the GL context, which would populate the very loader state
//     this test needs to observe as UNPOPULATED. This test creates its own raw GLFW context
//     instead, so `glx_gl_load()` genuinely never runs anywhere in this process before the
//     `UiLayer` construction under test.
//
//     WHAT THIS PROVES: [1] `glintfx::gl_proc_address("glClear")` returns `nullptr` BEFORE any
//     glintfx entity exists (same contract `gl_proc_address_sanity.cpp`/
//     `gl_proc_address_embed_sanity.cpp` already prove, re-confirmed here against a REAL raw
//     context rather than "no context at all"); [2] constructing `glintfx::UiLayer{
//     .assume_gl_loaded = true}` as the sole/first glintfx entity in this process does NOT crash -- if the guard in
//     `RenderGl3::init()` regresses (removed, or checks the wrong sentinel), this test crashes
//     with SIGSEGV and ctest reports it as a hard failure, which IS the regression signal (no
//     softer assertion would catch this class of defect as reliably as letting the process die);
//     [3] `ui.ok()` reports `false` (not a partially-constructed, ambiguous state); [4]
//     `gl_proc_address("glClear")` STILL returns `nullptr` after the failed construction --
//     proves the guard returns early WITHOUT calling `glx_gl_load()` itself, so the loader state
//     genuinely stays unpopulated (the guard is a REFUSAL, not a silent auto-fix).
//
//     WHAT THIS DELIBERATELY DOES NOT PROVE: the POSITIVE path (loader populated,
//     `Config::load_gl = true`, shaders actually compile and render) -- that is already covered
//     by every other test in this suite that constructs a working `UiLayer`/`App`
//     (`gl_proc_address_embed_sanity.cpp` among them), and re-proving it here would duplicate
//     coverage without adding confidence; this file's whole reason to exist is the NEGATIVE
//     (loader-absent) path specifically.
// PT: render_loader_guard_sanity (GLPROC-CRASH, 2026-07-30) -- teste de contrato/regressão para
//     a guarda de loader-não-pronto de `RenderGl3::init()` (src/render_gl3.cpp, logo antes de
//     `RmlGL3::Initialize`/`new Impl`). Este é o teste de regressão DIRETO pra um SIGSEGV medido
//     por execução (gdb) ao verificar a própria ressalva de pular loader de `gl_proc_address()`
//     (ver o doc-comment de `glintfx::gl_proc_address()`, gl_proc.hpp, e o próprio
//     CHANGELOG/commit `f0cacbb` desta biblioteca pro relato completo): antes da guarda,
//     `glintfx::UiLayer{.load_gl = false}` (o NOME ORIGINAL do campo no momento em que este bug
//     foi achado), construído como a PRIMEIRA entidade glintfx de um processo (então o
//     `glx_gl_load()` nunca rodou em lugar nenhum antes dele), crashava dentro do próprio
//     construtor -- `Engine::attach()` -> `RenderGl3::init()` -> `new Impl` -> o próprio
//     construtor de `RenderInterface_GL3` (RmlUi) compilando shaders GL3 através de um ponteiro
//     de função `glCreateShader` (== `glx_glCreateShader`) ainda-NULO.
//
//     RENOMEADO, MESMO CONTRATO (`SEED-LOADGL-NOME`, 2026-08-04): a própria construção deste
//     arquivo abaixo agora usa `.assume_gl_loaded = true`, o substituto canônico de
//     `.load_gl = false` (ver o próprio doc-comment de `UiLayerConfig`, `ui_layer.hpp`, pro
//     racional completo do rename e o aviso de polaridade invertida). A regressão que este teste
//     prova fica INALTERADA -- a guarda em `RenderGl3::init()` lê uma sentinela de ponteiro de
//     função, não qualquer um dos dois campos de config por nome, então a MESMA proteção de
//     classe-de-crash se aplica independente de qual grafia um chamador usa pra alcançá-la.
//
//     DELIBERADAMENTE NÃO USA `_glintfx_test_ctx`/`WindowGlfw` (diferente de todo outro teste
//     embed desta suíte, ex.: `ui_layer_attach.cpp`, `gl_proc_address_embed_sanity.cpp`): o
//     próprio `create()` daquele helper (src/window_glfw.cpp) chama `glx_gl_load()`
//     incondicionalmente como efeito colateral de estabelecer o contexto GL, o que popularia
//     exatamente o estado de loader que este teste precisa observar como NÃO-POPULADO. Este
//     teste cria o próprio contexto GLFW cru em vez disso, então o `glx_gl_load()` genuinamente
//     nunca roda em lugar nenhum deste processo antes da construção do `UiLayer` sob teste.
//
//     O QUE ISTO PROVA: [1] `glintfx::gl_proc_address("glClear")` retorna `nullptr` ANTES de
//     qualquer entidade glintfx existir (mesmo contrato que `gl_proc_address_sanity.cpp`/
//     `gl_proc_address_embed_sanity.cpp` já provam, reconfirmado aqui contra um contexto cru
//     REAL em vez de "contexto nenhum"); [2] construir `glintfx::UiLayer{.assume_gl_loaded =
//     true}` como a única/primeira entidade glintfx deste processo NÃO crasha -- se a guarda em
//     `RenderGl3::init()` regredir (removida, ou checando o sentinela errado), este teste
//     crasha com SIGSEGV e o ctest reporta como falha dura, que É o sinal de regressão (nenhuma
//     asserção mais branda pegaria esta classe de defeito de forma tão confiável quanto deixar o
//     processo morrer); [3] `ui.ok()` reporta `false` (não um estado parcialmente construído,
//     ambíguo); [4] `gl_proc_address("glClear")` AINDA retorna `nullptr` depois da construção
//     falhada -- prova que a guarda retorna cedo SEM chamar o próprio `glx_gl_load()`, então o
//     estado do loader genuinamente permanece não-populado (a guarda é uma RECUSA, não um
//     auto-conserto silencioso).
//
//     O QUE ISTO DELIBERADAMENTE NÃO PROVA: o caminho POSITIVO (loader populado,
//     `Config::load_gl = true`, shaders de fato compilam e renderizam) -- isso já é coberto por
//     todo outro teste desta suíte que constrói um `UiLayer`/`App` funcional
//     (`gl_proc_address_embed_sanity.cpp` entre eles), e reprovar aqui duplicaria cobertura sem
//     adicionar confiança; a razão de existir inteira deste arquivo é o caminho NEGATIVO
//     (loader-ausente) especificamente.
// Copyright (c) 2026 Petrus Silva Costa
#include <GLFW/glfw3.h>
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  int failures = 0;

  if (!glfwInit()) {
    std::puts("SKIP: glfwInit failed -- no display");
    return 0;
  }
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  GLFWwindow* win = glfwCreateWindow(64, 64, "render_loader_guard_sanity", nullptr, nullptr);
  if (!win) {
    std::puts("SKIP: glfwCreateWindow failed -- no display / GL context");
    glfwTerminate();
    return 0;
  }
  glfwMakeContextCurrent(win);

  // ---------------------------------------------------------------------------
  // [1] Before any glintfx entity exists in this process, against a REAL raw GL context
  //     (deliberately never routed through glx_gl_load()).
  // ---------------------------------------------------------------------------
  if (glintfx::gl_proc_address("glClear") != nullptr) {
    std::fprintf(stderr,
                 "render_loader_guard_sanity FAIL: [1] gl_proc_address(\"glClear\") != nullptr before "
                 "any glintfx entity exists -- this test's own premise (a genuinely never-loaded "
                 "process) does not hold, results below are not meaningful\n");
    ++failures;
  } else {
    std::puts("render_loader_guard_sanity [1] PASS: loader genuinely unpopulated pre-condition confirmed");
  }

  // ---------------------------------------------------------------------------
  // [2]/[3] The regression proof: this construction must NOT crash. If RenderGl3::init()'s
  //         loader-not-ready guard regresses, this line SIGSEGVs and ctest reports the crash
  //         directly -- that IS the failure signal for this specific check.
  //
  //         ⚠ CRASH-BASED DETECTION, NOT ASSERTION-BASED -- name the difference, do not blur it
  //         (2026-07-30, mutation run confirmed this empirically: neutering the guard reproduces
  //         the ORIGINAL SIGSEGV, ctest reports `***Failed` with a nonzero signal exit, not a
  //         printed FAIL line): the failures[i] counters and stderr messages further down in
  //         this file are NORMAL assertion-based checks -- they run, print a reason, and return
  //         a failure count. This ONE check is different in kind: with the guard removed, the
  //         PROCESS DIES inside RmlUi's own constructor (RenderInterface_GL3, vendored, not this
  //         file's code) before a single line after this construction ever executes -- there is
  //         no `if`/`++failures`/`fprintf` possible for THIS specific regression, because nothing
  //         downstream of the crash runs at all. ctest's own hard-failure-on-signal behaviour IS
  //         the check; there is no softer assertion this file could add that would fire instead
  //         (an assertion needs live code after the crash point to execute it). Do not read a
  //         green run of THIS check as "the assertion passed" -- read it as "the process did not
  //         die here", a distinct and slightly weaker claim: a crash ANYWHERE else on this same
  //         call path (not just a regressed loader guard) would ALSO fail this check the same
  //         way, which is a feature for a regression test (broad net) but means a failure here
  //         needs a backtrace (gdb) to attribute to THIS specific guard, not just this test's
  //         own pass/fail text.
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({.logical_width = 64, .logical_height = 64, .assume_gl_loaded = true});
  std::puts(
      "render_loader_guard_sanity [2] PASS: UiLayer{.assume_gl_loaded=true} construction did not "
      "crash (crash-based check -- see this call site's own comment: absence of a signal IS the "
      "pass condition, not an assertion that fired)");

  if (ui.ok()) {
    std::fprintf(stderr,
                 "render_loader_guard_sanity FAIL: [3] ui.ok() == true after an assume_gl_loaded=true "
                 "construction with no prior glintfx entity in this process -- expected false (the "
                 "loader was never populated, so a working attach here would be a bigger surprise than "
                 "the crash this test guards against)\n");
    ++failures;
  } else {
    std::puts("render_loader_guard_sanity [3] PASS: ui.ok() == false, as documented");
  }

  // ---------------------------------------------------------------------------
  // [4] The guard is a REFUSAL, not a silent auto-fix: it must not itself call glx_gl_load().
  // ---------------------------------------------------------------------------
  if (glintfx::gl_proc_address("glClear") != nullptr) {
    std::fprintf(stderr,
                 "render_loader_guard_sanity FAIL: [4] gl_proc_address(\"glClear\") != nullptr after "
                 "the failed construction -- the loader-not-ready guard must not itself populate the "
                 "loader as a side effect\n");
    ++failures;
  } else {
    std::puts(
        "render_loader_guard_sanity [4] PASS: loader still unpopulated after the guard fired "
        "(refusal, not a silent auto-fix)");
  }

  glfwDestroyWindow(win);
  glfwTerminate();

  if (failures == 0) {
    std::puts("render_loader_guard_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "render_loader_guard_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
