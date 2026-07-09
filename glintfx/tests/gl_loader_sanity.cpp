// SPDX-License-Identifier: MPL-2.0
// EN: TDD sanity for L1.14-GLLOADER -- glintfx's own GL 3.3 core-profile function-pointer
//     loader (glintfx/src/gl_loader.h/.c), replacing the previously vendored gl3w.
//     Proves the loader's public contract in isolation, against a real offscreen GL
//     context (a hidden GLFW window, same test-only-GLFW-context-fixture pattern as the
//     other embed tests in this directory -- see the "GL context fixture" comment at the
//     top of tests/CMakeLists.txt):
//       1. glx_gl_load() returns 0 (success) -- same polarity as the gl3wInit() call
//          sites it replaces.
//       2. A representative sample of core function pointers is non-null after loading:
//          glGetString (GL 1.0), glCreateShader (GL 2.0 -- shader pipeline), and
//          glBindFramebuffer (GL 3.0 -- FBO/render-target path this renderer depends on).
//       3. glGetString(GL_VERSION) actually returns a non-null string, proving the
//          resolved glGetString pointer is not just non-null but *callable* against a
//          real context.
//     Runs in BOTH build configs (GLINTFX_BACKEND_GLFW=ON and OFF) -- see
//     tests/CMakeLists.txt "Embed-mode tests" section, right after gl_state_guard.
// PT: TDD de sanidade do L1.14-GLLOADER -- loader próprio de ponteiros de função GL 3.3
//     core profile do glintfx (glintfx/src/gl_loader.h/.c), substituindo o gl3w antes
//     vendorizado.
//     Prova o contrato público do loader isoladamente, contra um contexto GL offscreen
//     real (janela GLFW oculta, mesmo padrão de fixture de contexto GL só-de-teste dos
//     outros testes embed deste diretório -- ver o comentário "GL context fixture" no
//     topo de tests/CMakeLists.txt):
//       1. glx_gl_load() retorna 0 (sucesso) -- mesma polaridade dos call-sites de
//          gl3wInit() que substitui.
//       2. Uma amostra representativa de ponteiros de função core está não-nula após o
//          load: glGetString (GL 1.0), glCreateShader (GL 2.0 -- pipeline de shader) e
//          glBindFramebuffer (GL 3.0 -- caminho FBO/render-target do qual este renderer
//          depende).
//       3. glGetString(GL_VERSION) de fato retorna uma string não-nula, provando que o
//          ponteiro glGetString resolvido não está só não-nulo, mas *chamável* contra um
//          contexto real.
//     Roda em AMBAS as configurações de build (GLINTFX_BACKEND_GLFW=ON e OFF) -- ver a
//     seção "Embed-mode tests" de tests/CMakeLists.txt, logo após gl_state_guard.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "gl_loader.h"
#include <cstdio>

int main() {
  // EN: Host creates a hidden window + current GL 3.3 core context. WindowGlfw::create()
  //     already calls glx_gl_load() internally and fails if it returns non-zero -- a
  //     failure here would already prove the loader broken, but we re-assert the direct
  //     contract below regardless (glx_gl_load() is idempotent, see gl_loader.h).
  // PT: Host cria janela oculta + contexto GL 3.3 core corrente. WindowGlfw::create() já
  //     chama glx_gl_load() internamente e falha se retornar não-zero -- uma falha aqui já
  //     provaria o loader quebrado, mas reafirmamos o contrato direto abaixo mesmo assim
  //     (glx_gl_load() é idempotente, ver gl_loader.h).
  glintfx::WindowGlfw win;
  if (!win.create("gl_loader_sanity", 64, 64)) {
    std::puts("FAIL: window/context create failed");
    return 1;
  }

  if (glx_gl_load() != 0) {
    std::puts("FAIL: glx_gl_load() != 0");
    return 2;
  }
  if (!glGetString) {
    std::puts("FAIL: glGetString pointer is null after glx_gl_load()");
    return 3;
  }
  if (!glCreateShader) {
    std::puts("FAIL: glCreateShader pointer is null after glx_gl_load()");
    return 4;
  }
  if (!glBindFramebuffer) {
    std::puts("FAIL: glBindFramebuffer pointer is null after glx_gl_load()");
    return 5;
  }

  const unsigned char* version = glGetString(GL_VERSION);
  if (!version) {
    std::puts("FAIL: glGetString(GL_VERSION) returned null");
    return 6;
  }

  std::printf("OK: glx_gl_load()==0, core pointers resolved, GL_VERSION=%s\n", version);
  return 0;
}
