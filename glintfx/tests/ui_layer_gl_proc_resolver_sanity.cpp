// SPDX-License-Identifier: Apache-2.0
// EN: TDD sanity for `GLLOADER-HOST` (2026-08-04) -- `UiLayerConfig::gl_proc_resolver`, the
//     host-supplied GL function-pointer resolver. See `UiLayerConfig::gl_proc_resolver`'s own
//     doc-comment (ui_layer.hpp) for the full contract this test proves; summary of the four
//     checks below:
//       [1] POSITIVE: a resolver that wraps `glfwGetProcAddress` and COUNTS its own calls
//           populates the table -- `ok() == true` AND the counter is > 0, proving the table
//           came from the HOST's resolver, not from glintfx's own glX/EGL/dlsym chain.
//       [2] NEGATIVE: a resolver that always returns `nullptr` fails cleanly -- `ok() == false`,
//           no crash.
//       [3] DEFAULT: `gl_proc_resolver == nullptr` leaves the pre-existing `glx_gl_load()` path
//           untouched -- `ok() == true` via the ORIGINAL loader, no resolver involved.
//       [4] CONTRADICTION: a resolver set together with `assume_gl_loaded = true` is IGNORED
//           (never called -- counter stays 0), not a crash, not silently honoured either.
//     DELIBERATELY DOES NOT USE `_glintfx_test_ctx`/`WindowGlfw` (same reasoning as
//     `render_loader_guard_sanity.cpp`): `WindowGlfw::create()` calls `glx_gl_load()` as a side
//     effect of creating the GL context, which would pre-populate the very table this test
//     needs to observe as coming from ITS OWN resolver, not a side channel. Each check gets a
//     fresh raw GLFW window/context (own process either way -- ctest runs one executable per
//     test -- but the checks below still share ONE process, so each explicitly re-populates the
//     table it needs rather than assuming a clean slate from a prior check).
// PT: TDD de sanidade do `GLLOADER-HOST` (2026-08-04) -- `UiLayerConfig::gl_proc_resolver`, o
//     resolvedor de ponteiro de função GL fornecido pelo host. Ver o próprio doc-comment de
//     `UiLayerConfig::gl_proc_resolver` (ui_layer.hpp) pro contrato completo que este teste
//     prova; resumo dos quatro cheques abaixo:
//       [1] POSITIVO: um resolvedor que embrulha `glfwGetProcAddress` e CONTA as próprias
//           chamadas popula a tabela -- `ok() == true` E o contador é > 0, provando que a
//           tabela veio do resolvedor do HOST, não da própria cadeia glX/EGL/dlsym da glintfx.
//       [2] NEGATIVO: um resolvedor que sempre devolve `nullptr` falha de forma limpa --
//           `ok() == false`, sem crash.
//       [3] DEFAULT: `gl_proc_resolver == nullptr` deixa o caminho pré-existente de
//           `glx_gl_load()` intocado -- `ok() == true` via o loader ORIGINAL, nenhum resolvedor
//           envolvido.
//       [4] CONTRADIÇÃO: um resolvedor definido junto com `assume_gl_loaded = true` é IGNORADO
//           (nunca chamado -- o contador fica em 0), nem crash, nem honrado em silêncio.
//     DELIBERADAMENTE NÃO USA `_glintfx_test_ctx`/`WindowGlfw` (mesmo racional de
//     `render_loader_guard_sanity.cpp`): o `WindowGlfw::create()` chama `glx_gl_load()` como
//     efeito colateral de criar o contexto GL, o que pré-popularia exatamente a tabela que este
//     teste precisa observar como vinda do PRÓPRIO resolvedor, não de um canal lateral. Cada
//     cheque ganha uma janela/contexto GLFW cru fresco (mesmo processo entre os cheques -- o
//     ctest roda um executável por teste -- mas os cheques abaixo compartilham UM processo,
//     então cada um repopula explicitamente a tabela de que precisa, em vez de assumir um
//     estado limpo herdado de um cheque anterior).
// Copyright (c) 2026 Petrus Silva Costa
#include <GLFW/glfw3.h>
#include <glintfx/glintfx.hpp>
#include <cstdio>

namespace {

GLFWwindow* make_hidden_context(const char* title) {
  if (!glfwInit()) return nullptr;
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  GLFWwindow* win = glfwCreateWindow(64, 64, title, nullptr, nullptr);
  if (!win) return nullptr;
  glfwMakeContextCurrent(win);
  return win;
}

// EN: Stateful resolver, host-side static (per GlProcResolver's own doc-comment: no `void*
//     user` parameter, so a counting/caching resolver carries its state this way -- exactly
//     what a real host with a cached loader handle would do).
// PT: Resolvedor com estado, static do lado do host (conforme o próprio doc-comment de
//     GlProcResolver: sem parâmetro `void* user`, então um resolvedor que conta/cacheia carrega
//     o estado assim -- exatamente o que um host real com handle de loader cacheado faria).
int g_counting_calls = 0;
void* counting_resolver(const char* name) {
  ++g_counting_calls;
  return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

int g_null_calls = 0;
void* null_resolver(const char*) {
  ++g_null_calls;
  return nullptr;
}

int g_contradiction_calls = 0;
void* contradiction_resolver(const char*) {
  ++g_contradiction_calls;
  return nullptr;
}

} // namespace

int main() {
  int failures = 0;

  // ---------------------------------------------------------------------------
  // [1] POSITIVE -- resolver populates the table; ok()==true AND counter>0.
  // ---------------------------------------------------------------------------
  {
    GLFWwindow* win = make_hidden_context("ui_layer_gl_proc_resolver_sanity_1");
    if (!win) {
      std::puts("SKIP: no display -- cannot create GL context for check [1]");
    } else {
      g_counting_calls = 0;
      glintfx::UiLayer ui({.logical_width = 64,
                           .logical_height = 64,
                           .gl_proc_resolver = counting_resolver});
      if (!ui.ok()) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [1] ok() == false with a working "
                     "counting resolver wrapping glfwGetProcAddress\n");
        ++failures;
      } else if (g_counting_calls <= 0) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [1] ok() == true but the resolver "
                     "was never called (counter == %d) -- the table did not come from it\n",
                     g_counting_calls);
        ++failures;
      } else {
        std::printf(
            "ui_layer_gl_proc_resolver_sanity [1] PASS: ok()==true, resolver called %d times "
            "(table populated via the HOST's own resolver)\n",
            g_counting_calls);
      }
      glfwDestroyWindow(win);
      glfwTerminate();
    }
  }

  // ---------------------------------------------------------------------------
  // [2] NEGATIVE -- resolver always returns nullptr; ok()==false, no crash.
  // ---------------------------------------------------------------------------
  {
    GLFWwindow* win = make_hidden_context("ui_layer_gl_proc_resolver_sanity_2");
    if (!win) {
      std::puts("SKIP: no display -- cannot create GL context for check [2]");
    } else {
      g_null_calls = 0;
      glintfx::UiLayer ui(
          {.logical_width = 64, .logical_height = 64, .gl_proc_resolver = null_resolver});
      if (ui.ok()) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [2] ok() == true with a resolver "
                     "that always returns nullptr -- expected a clean false\n");
        ++failures;
      } else if (g_null_calls <= 0) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [2] resolver was never invoked "
                     "(counter == %d) -- the resolver path was not taken\n",
                     g_null_calls);
        ++failures;
      } else {
        std::printf(
            "ui_layer_gl_proc_resolver_sanity [2] PASS: ok()==false (no crash), resolver called "
            "%d times, all NULL\n",
            g_null_calls);
      }
      glfwDestroyWindow(win);
      glfwTerminate();
    }
  }

  // ---------------------------------------------------------------------------
  // [3] DEFAULT -- gl_proc_resolver == nullptr leaves the pre-existing path unaffected.
  // ---------------------------------------------------------------------------
  {
    GLFWwindow* win = make_hidden_context("ui_layer_gl_proc_resolver_sanity_3");
    if (!win) {
      std::puts("SKIP: no display -- cannot create GL context for check [3]");
    } else {
      glintfx::UiLayer ui({.logical_width = 64, .logical_height = 64});
      if (!ui.ok()) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [3] ok() == false with the default "
                     "config (gl_proc_resolver == nullptr) -- the pre-existing glx_gl_load() "
                     "path regressed\n");
        ++failures;
      } else {
        std::puts(
            "ui_layer_gl_proc_resolver_sanity [3] PASS: default config (no resolver) still "
            "works via glintfx's own glx_gl_load()");
      }
      glfwDestroyWindow(win);
      glfwTerminate();
    }
  }

  // ---------------------------------------------------------------------------
  // [4] CONTRADICTION -- resolver set together with assume_gl_loaded=true is ignored, never
  //     called, no crash (fail-high, not a silent honour of the resolver).
  // ---------------------------------------------------------------------------
  {
    GLFWwindow* win = make_hidden_context("ui_layer_gl_proc_resolver_sanity_4");
    if (!win) {
      std::puts("SKIP: no display -- cannot create GL context for check [4]");
    } else {
      g_contradiction_calls = 0;
      glintfx::UiLayer ui({.logical_width = 64,
                           .logical_height = 64,
                           .assume_gl_loaded = true,
                           .gl_proc_resolver = contradiction_resolver});
      // EN: No crash is the primary claim here (construction above already proved that by
      //     returning). The resolver must never have been called -- assume_gl_loaded=true wins,
      //     the resolver is ignored, not silently honoured.
      // PT: Nenhum crash é a alegação primária aqui (a construção acima já provou isso ao
      //     retornar). O resolvedor nunca deve ter sido chamado -- assume_gl_loaded=true vence,
      //     o resolvedor é ignorado, não honrado em silêncio.
      if (g_contradiction_calls != 0) {
        std::fprintf(stderr,
                     "ui_layer_gl_proc_resolver_sanity FAIL: [4] resolver was called %d time(s) "
                     "despite assume_gl_loaded=true -- the contradiction must ignore the "
                     "resolver entirely, not call it\n",
                     g_contradiction_calls);
        ++failures;
      } else {
        std::printf(
            "ui_layer_gl_proc_resolver_sanity [4] PASS: resolver never called (assume_gl_loaded="
            "true wins, contradiction logged, no crash); ui.ok()=%s\n",
            ui.ok() ? "true" : "false");
      }
      glfwDestroyWindow(win);
      glfwTerminate();
    }
  }

  if (failures == 0) {
    std::puts("ui_layer_gl_proc_resolver_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "ui_layer_gl_proc_resolver_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
