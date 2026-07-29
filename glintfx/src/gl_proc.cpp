// SPDX-License-Identifier: MPL-2.0
// EN: DOC-GLCOHAB -- one-line delegation from the public glintfx::gl_proc_address() (declared
//     in glintfx/include/glintfx/gl_proc.hpp, see that header for the full contract and design
//     rationale) to the private glx_gl_get_proc_address() C helper (gl_loader.h/.c). Kept in its
//     own translation unit (not folded into window_glfw.cpp/app.cpp) so this one-symbol public
//     surface has a single, easy-to-audit call site independent of App's own construction/
//     lifecycle code -- grepping this file alone proves nothing beyond the delegation itself
//     runs here.
// PT: DOC-GLCOHAB -- delegação de uma linha do glintfx::gl_proc_address() público (declarado em
//     glintfx/include/glintfx/gl_proc.hpp, ver aquele header pro contrato completo e o racional
//     de desenho) pro helper C privado glx_gl_get_proc_address() (gl_loader.h/.c). Mantido na
//     própria unidade de tradução (não dobrado dentro de window_glfw.cpp/app.cpp) para que esta
//     superfície pública de um único símbolo tenha um único call site fácil de auditar,
//     independente do código de construção/ciclo-de-vida do próprio App -- grepar só este
//     arquivo prova que a delegação roda aqui, nada além disso.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/gl_proc.hpp>
#include "gl_loader.h"
namespace glintfx {

void* gl_proc_address(const char* name) {
  return glx_gl_get_proc_address(name);
}

} // namespace glintfx
