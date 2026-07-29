// SPDX-License-Identifier: MPL-2.0
// EN: gl_proc_address_sanity -- contract test for `DOC-GLCOHAB`'s (b) answer,
//     `glintfx::gl_proc_address(const char*)` (glintfx/include/glintfx/gl_proc.hpp).
//
//     WHAT THIS PROVES: [1] the null-input guard (`name == nullptr` / `name == ""`) always
//     returns `nullptr`, never crashes, both BEFORE any `App` exists (the underlying loader
//     state is unpopulated) and AFTER one does; [2] a symbol that is a REAL, always-present
//     GL 3.3 core-profile entry point (`glClear`) resolves to a non-null pointer once an `App`
//     has run `glx_gl_load()` at least once.
//
//     WHAT THIS DELIBERATELY DOES NOT PROVE (declared, not silently skipped -- see the ⚠ note
//     on gl_proc.hpp's own doc-comment): that a MADE-UP name returns `nullptr`. Measured
//     empirically while writing this contract, under this library's own Mesa/llvmpipe CI
//     environment: it does NOT -- `GLX_ARB_get_proc_address` is documented to permit an
//     implementation to hand back a non-NULL pointer for a name it does not recognise, and this
//     environment's driver does exactly that. Asserting `nullptr` for a bogus name here would
//     therefore be asserting a driver-dependent accident, not a contract this library can
//     promise -- the opposite of what this file exists to pin. This is the SAME class of
//     "declare the honest limitation instead of faking coverage" discipline
//     `app_vsync_sanity.cpp`'s own header comment uses for driver-honesty gaps.
// PT: gl_proc_address_sanity -- teste de contrato para a resposta (b) do `DOC-GLCOHAB`,
//     `glintfx::gl_proc_address(const char*)` (glintfx/include/glintfx/gl_proc.hpp).
//
//     O QUE ISTO PROVA: [1] a guarda de entrada nula (`name == nullptr` / `name == ""`) sempre
//     retorna `nullptr`, nunca crasha, tanto ANTES de qualquer `App` existir (o estado do
//     loader subjacente não está populado) quanto DEPOIS; [2] um símbolo que é um entry point
//     GL 3.3 core-profile REAL e sempre presente (`glClear`) resolve pra um ponteiro não-nulo
//     assim que um `App` rodou `glx_gl_load()` ao menos uma vez.
//
//     O QUE ISTO DELIBERADAMENTE NÃO PROVA (declarado, não silenciosamente pulado -- ver a nota
//     ⚠ no próprio doc-comment de gl_proc.hpp): que um nome INVENTADO retorna `nullptr`. Medido
//     empiricamente ao escrever este contrato, sob o próprio ambiente de CI Mesa/llvmpipe desta
//     biblioteca: NÃO retorna -- o `GLX_ARB_get_proc_address` é documentado a permitir que uma
//     implementação devolva um ponteiro não-NULO pra um nome que não reconhece, e o driver
//     deste ambiente faz exatamente isso. Afirmar `nullptr` pra um nome bogus aqui seria então
//     afirmar um acidente dependente de driver, não um contrato que esta biblioteca consegue
//     prometer -- o oposto do que este arquivo existe pra fixar. É a MESMA disciplina de
//     "declare a limitação honesta em vez de forjar cobertura" que o próprio comentário de
//     cabeçalho de `app_vsync_sanity.cpp` usa pras lacunas de honestidade de driver.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  int failures = 0;

  // ---------------------------------------------------------------------------
  // [1a] Null-input guard BEFORE any App/loader state exists in this process.
  // ---------------------------------------------------------------------------
  if (glintfx::gl_proc_address(nullptr) != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_sanity FAIL: [1a] gl_proc_address(nullptr) != nullptr "
                 "before any App exists\n");
    ++failures;
  }
  if (glintfx::gl_proc_address("") != nullptr) {
    std::fprintf(stderr,
                 "gl_proc_address_sanity FAIL: [1a] gl_proc_address(\"\") != nullptr "
                 "before any App exists\n");
    ++failures;
  }

  // ---------------------------------------------------------------------------
  // [1b]/[2] Same null-input guard AFTER App exists, plus a real, always-present GL 3.3
  //          core-profile symbol resolving to non-null.
  // ---------------------------------------------------------------------------
  {
    glintfx::AppConfig cfg;
    cfg.title = "gl_proc_address_sanity";
    cfg.width = 320;
    cfg.height = 240;
    glintfx::App app(cfg);
    if (!app.ok()) {
      std::puts("SKIP: App init failed -- no display / GL context");
      return failures;
    }

    if (glintfx::gl_proc_address(nullptr) != nullptr) {
      std::fprintf(stderr,
                   "gl_proc_address_sanity FAIL: [1b] gl_proc_address(nullptr) != nullptr "
                   "after App exists\n");
      ++failures;
    }
    if (glintfx::gl_proc_address("") != nullptr) {
      std::fprintf(stderr,
                   "gl_proc_address_sanity FAIL: [1b] gl_proc_address(\"\") != nullptr "
                   "after App exists\n");
      ++failures;
    }

    if (glintfx::gl_proc_address("glClear") == nullptr) {
      std::fprintf(stderr,
                   "gl_proc_address_sanity FAIL: [2] gl_proc_address(\"glClear\") == nullptr "
                   "-- a GL 3.3 core-profile entry point that must always be resolvable once "
                   "glx_gl_load() has run\n");
      ++failures;
    }

    std::puts(
        "gl_proc_address_sanity [2] glClear resolved (a bogus/made-up name is "
        "deliberately NOT asserted here -- see this file's own header comment)");
  }

  if (failures == 0) {
    std::puts("gl_proc_address_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "gl_proc_address_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
