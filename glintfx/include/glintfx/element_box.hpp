// SPDX-License-Identifier: MPL-2.0
// EN: Plain result struct for get_element_box() -- window-space physical-pixel geometry of a
//     document element (F2, v0.2.5). Coordinate space: window/render-target physical pixels,
//     top-left origin, y-down -- the SAME space as UiEvent's mouse coordinates (see
//     docs/embed-integration.md section 10). Border-box (Rml::BoxArea::Border): includes
//     border, excludes margin. found=false means the id was not found in the currently loaded
//     document (or no document is loaded yet) -- x/y/w/h stay at their zero defaults.
// PT: Struct de resultado simples para get_element_box() -- geometria em pixels físicos,
//     espaço-janela, de um elemento do documento (F2, v0.2.5). Espaço de coordenadas: pixels
//     físicos do render-target/janela, origem superior-esquerda, y pra baixo -- o MESMO espaço
//     das coordenadas de mouse do UiEvent (ver docs/embed-integration.md seção 10). Border-box
//     (Rml::BoxArea::Border): inclui borda, exclui margem. found=false significa que o id não
//     foi encontrado no documento carregado atualmente (ou nenhum documento carregado ainda) --
//     x/y/w/h ficam nos defaults zero.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

struct ElementBox {
  bool  found = false;
  float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
};

} // namespace glintfx
