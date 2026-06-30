// SPDX-License-Identifier: MPL-2.0
// EN: PPM pixel-capture helper for golden-image tests.
//     Must be included AFTER GL/gl3w.h is initialised (App ctor does this via gl3wInit).
// PT: Helper de captura PPM para testes de golden-image.
//     Deve ser incluído APÓS GL/gl3w.h estar inicializado (ctor do App faz via gl3wInit).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <GL/gl3w.h>
#include <cstdio>
#include <vector>
inline bool save_ppm(const char* path,int w,int h){
  std::vector<unsigned char> px(w*h*3);
  // EN: RenderInterface_GL3 uses internal FBOs for effects (filters, backdrop-blur).
  //     After EndFrame+swap those FBOs may still be bound; rebind the window framebuffer (0)
  //     explicitly so glReadPixels reads from the actual window back-buffer.
  //     Mesa/llvmpipe under Xvfb does a copy-swap (not a flip), so GL_BACK retains the
  //     last rendered frame after glfwSwapBuffers.
  // PT: RenderInterface_GL3 usa FBOs internos para efeitos (filtros, backdrop-blur).
  //     Após EndFrame+swap esses FBOs podem estar vinculados; rebinda o framebuffer da
  //     janela (0) explicitamente para que glReadPixels leia do back-buffer real.
  //     Mesa/llvmpipe sob Xvfb faz copy-swap (não flip), então GL_BACK retém o último frame.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,px.data());
  FILE* f=fopen(path,"wb"); if(!f) return false;
  fprintf(f,"P6\n%d %d\n255\n",w,h); fwrite(px.data(),1,px.size(),f); fclose(f); return true;
}
inline double ppm_diff(const char* a,const char* b); // impl no .cpp do teste
