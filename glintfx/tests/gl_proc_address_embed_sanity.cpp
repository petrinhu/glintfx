// SPDX-License-Identifier: MPL-2.0
// EN: gl_proc_address_embed_sanity (GLPROC-EMBED, 2026-07-30) -- EMBED-PATH contract test for
//     `glintfx::gl_proc_address(const char*)` (glintfx/include/glintfx/gl_proc.hpp), the sibling
//     of `gl_proc_address_sanity.cpp` (App-mode) proving the function works the same way when
//     the CURRENT GL context belongs to an embed host, not to a `glintfx::App`. Compiles and
//     runs in BOTH build configs (same `_embed_dep` pattern as `ui_layer_attach.cpp`, see
//     tests/CMakeLists.txt): in `GLINTFX_BACKEND_GLFW=OFF` this is the ONLY coverage this
//     function gets (app.cpp/gl_proc.hpp used to be compiled out there entirely, before
//     GLPROC-EMBED); in `GLINTFX_BACKEND_GLFW=ON` it re-proves the same contract through a
//     UiLayer instead of an App, so both facades stay honest about resolving against
//     "the current context", not "an App's context" specifically.
//
//     WHAT THIS PROVES: [1] the null-input guard behaves identically to the App-mode test,
//     both before and after a GL context/loader exist in this process; [2] a symbol that is a
//     REAL, always-present GL 3.3 core-profile entry point (`glClear`) resolves to a non-null
//     pointer once a `UiLayer` has attached to a HOST-owned context (`host.create()` below
//     stands in for the host's own window/context creation, e.g. SDL3 in a real embed
//     consumer -- see `ui_layer_attach.cpp`'s own header comment for the same substitution).
//
//     WHAT THIS DELIBERATELY DOES NOT PROVE (declared, not silently skipped -- same discipline
//     as `gl_proc_address_sanity.cpp`'s own header comment): the `Config::load_gl = false`
//     caveat documented on `gl_proc_address()`'s own doc-comment (a UiLayer host that skips
//     glintfx's own `glx_gl_load()` gets a reliable `nullptr` for every name). This harness
//     cannot isolate that case: `host.create()` below (glintfx::WindowGlfw, the test-only
//     context helper -- see tests/CMakeLists.txt's `_glintfx_test_ctx`) calls `glx_gl_load()`
//     itself as a side effect of establishing the GL context in the FIRST place
//     (src/window_glfw.cpp), so by the time a `UiLayer{.load_gl = false}` could be constructed
//     in this same process, the loader state the caveat is ABOUT is already populated by the
//     host window's own creation -- there is no same-process way to observe the pre-`glx_gl_load`
//     state a real `load_gl = false` host would be in without ALSO never creating a context at
//     all (at which point there is no context to test resolution against either). Asserting a
//     specific outcome here would therefore pin an artifact of this test harness, not the
//     documented contract.
// PT: gl_proc_address_embed_sanity (GLPROC-EMBED, 2026-07-30) -- teste de contrato do CAMINHO
//     EMBED para `glintfx::gl_proc_address(const char*)` (glintfx/include/glintfx/gl_proc.hpp),
//     o irmão de `gl_proc_address_sanity.cpp` (modo App) provando que a função funciona do
//     mesmo jeito quando o contexto GL CORRENTE pertence a um host embed, não a um
//     `glintfx::App`. Compila e roda nas DUAS configs de build (mesmo padrão `_embed_dep` de
//     `ui_layer_attach.cpp`, ver tests/CMakeLists.txt): em `GLINTFX_BACKEND_GLFW=OFF` esta é a
//     ÚNICA cobertura que esta função recebe (app.cpp/gl_proc.hpp costumavam ser compilados
//     fora ali inteiramente, antes do GLPROC-EMBED); em `GLINTFX_BACKEND_GLFW=ON` reprova o
//     mesmo contrato por um UiLayer em vez de um App, então as duas fachadas seguem honestas
//     sobre resolver contra "o contexto corrente", não "o contexto de um App" especificamente.
//
//     O QUE ISTO PROVA: [1] a guarda de entrada nula se comporta idêntico ao teste de modo App,
//     tanto antes quanto depois de existir um contexto GL/loader neste processo; [2] um símbolo
//     que é um entry point GL 3.3 core-profile REAL e sempre presente (`glClear`) resolve pra
//     um ponteiro não-nulo assim que um `UiLayer` anexa a um contexto de propriedade do HOST
//     (`host.create()` abaixo faz o papel da própria criação de janela/contexto do host, ex.:
//     SDL3 num consumidor embed real -- ver o próprio comentário de cabeçalho de
//     `ui_layer_attach.cpp` pra mesma substituição).
//
//     O QUE ISTO DELIBERADAMENTE NÃO PROVA (declarado, não silenciosamente pulado -- mesma
//     disciplina do próprio comentário de cabeçalho de `gl_proc_address_sanity.cpp`): a
//     ressalva `Config::load_gl = false` documentada no próprio doc-comment de
//     `gl_proc_address()` (um host UiLayer que pula o próprio `glx_gl_load()` da glintfx recebe
//     um `nullptr` confiável pra todo nome). Este harness não consegue isolar esse caso:
//     `host.create()` abaixo (glintfx::WindowGlfw, o helper de contexto só-de-teste -- ver
//     `_glintfx_test_ctx` de tests/CMakeLists.txt) chama o próprio `glx_gl_load()` como efeito
//     colateral de estabelecer o contexto GL em PRIMEIRO lugar (src/window_glfw.cpp), então no
//     momento em que um `UiLayer{.load_gl = false}` poderia ser construído neste mesmo
//     processo, o estado do loader do qual a ressalva TRATA já está populado pela própria
//     criação da janela do host -- não há forma no mesmo processo de observar o estado
//     pré-`glx_gl_load` em que um host `load_gl = false` real estaria sem TAMBÉM nunca criar
//     contexto nenhum (ponto em que não há contexto pra testar a resolução contra). Afirmar um
//     resultado específico aqui portanto fixaria um artefato deste harness de teste, não o
//     contrato documentado.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  int failures = 0;

  // ---------------------------------------------------------------------------
  // [1a] Null-input guard BEFORE any GL context/loader state exists in this process.
  // ---------------------------------------------------------------------------
  if (glintfx::gl_proc_address(nullptr) != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_embed_sanity FAIL: [1a] gl_proc_address(nullptr) != nullptr "
                 "before any GL context exists\n");
    ++failures;
  }
  if (glintfx::gl_proc_address("") != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_embed_sanity FAIL: [1a] gl_proc_address(\"\") != nullptr "
                 "before any GL context exists\n");
    ++failures;
  }

  // ---------------------------------------------------------------------------
  // Host creates the GL context (stands in for SDL3/GLX/EGL context creation in a real embed
  // consumer -- same substitution ui_layer_attach.cpp uses).
  // ---------------------------------------------------------------------------
  glintfx::WindowGlfw host;
  if (!host.create("gl_proc_address_embed_sanity host", 320, 240)) {
    std::puts("SKIP: host window create failed -- no display / GL context");
    return failures;
  }

  // ---------------------------------------------------------------------------
  // [1b] UiLayer attaches to the HOST-owned context (Config::load_gl = true default, matching
  //      ui_layer_attach.cpp -- glx_gl_load() runs against the already-current host context).
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({.logical_width = 320, .logical_height = 240, .load_gl = true});
  if (!ui.ok()) {
    std::puts("SKIP: UiLayer attach failed");
    return failures;
  }

  if (glintfx::gl_proc_address(nullptr) != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_embed_sanity FAIL: [1b] gl_proc_address(nullptr) != nullptr "
                 "after UiLayer attach\n");
    ++failures;
  }
  if (glintfx::gl_proc_address("") != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_embed_sanity FAIL: [1b] gl_proc_address(\"\") != nullptr "
                 "after UiLayer attach\n");
    ++failures;
  }

  // ---------------------------------------------------------------------------
  // [2] A real, always-present GL 3.3 core-profile entry point resolves to non-null once
  //     attach (via glx_gl_load()) has run -- proves resolution works against a HOST-owned
  //     context, not just an App-owned one.
  // ---------------------------------------------------------------------------
  if (glintfx::gl_proc_address("glClear") == nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_embed_sanity FAIL: [2] gl_proc_address(\"glClear\") == "
                 "nullptr -- a GL 3.3 core-profile entry point that must always be resolvable "
                 "once glx_gl_load() has run against a host-owned context\n");
    ++failures;
  }

  if (failures == 0) {
    std::puts(
        "gl_proc_address_embed_sanity: PASS "
        "(the Config::load_gl = false caveat is deliberately NOT exercised here -- see "
        "this file's own header comment)");
    return 0;
  }
  std::fprintf(stderr, "gl_proc_address_embed_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
