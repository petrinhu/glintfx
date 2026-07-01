// SPDX-License-Identifier: MPL-2.0
// EN: Scans the framebuffer for pure-red and pure-blue pixels and records their row
//     range. If the two divs are inline (today's bug, no UA-stylesheet), they share the
//     same line box -- their row ranges OVERLAP. If they are block (fixed by T1), they
//     stack -- their row ranges are DISJOINT. Order-agnostic (doesn't assert which one
//     is "on top"): disjointness alone is the proof, robust to either glReadPixels row
//     convention.
// PT: Varre o framebuffer por pixels vermelho/azul puros e registra a faixa de linhas.
//     Se os 2 divs são inline (bug de hoje, sem UA-stylesheet), compartilham a mesma
//     line box -- as faixas de linha SE SOBREPÕEM. Se são block (corrigido por T1),
//     empilham -- as faixas ficam DISJUNTAS. Não assume qual fica "em cima": a
//     disjunção sozinha é a prova, robusta a qualquer convenção de linha do glReadPixels.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes GL/gl3w.h (gl3w already loaded by UiLayer ctor).
                           // PT: inclui GL/gl3w.h (gl3w já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Row range where a predicate colour was found, plus a hit count for the
//     "did it even render" sanity guard.
// PT: Faixa de linhas onde uma cor-predicado foi encontrada, mais uma contagem de
//     hits para o guard de sanidade "isso sequer renderizou".
// ---------------------------------------------------------------------------
struct RowRange { int min_row = -1, max_row = -1; size_t count = 0; };

static bool is_red (const unsigned char* p) { return p[0] > 150 && p[1] < 80  && p[2] < 80;  }
static bool is_blue(const unsigned char* p) { return p[2] > 150 && p[0] < 80  && p[1] < 80;  }

static RowRange scan(const std::vector<unsigned char>& px, int w, int h,
                      bool (*pred)(const unsigned char*)) {
  RowRange r;
  for (int row = 0; row < h; ++row)
    for (int col = 0; col < w; ++col) {
      const unsigned char* p = &px[(size_t)(row * w + col) * 3];
      if (pred(p)) { if (r.min_row < 0) r.min_row = row; r.max_row = row; ++r.count; }
    }
  return r;
}

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) -- same methodology as
//     dp_ratio_sanity.cpp's capture().
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK) -- mesma metodologia
//     do capture() de dp_ratio_sanity.cpp.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> capture(glintfx::UiLayer& ui, int w, int h) {
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

int main() {
  constexpr int W = 200, H = 160;

  glintfx::WindowGlfw host;
  if (!host.create("ua_stylesheet_sanity_host", W, H)) {
    std::puts("ua_stylesheet_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("ua_stylesheet_sanity FAIL: ui attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.load("ua_stylesheet_scene.rml");

  const auto px  = capture(ui, W, H);
  const auto red  = scan(px, W, H, is_red);
  const auto blue = scan(px, W, H, is_blue);

  std::printf("ua_stylesheet_sanity: red rows=[%d,%d] count=%zu  blue rows=[%d,%d] count=%zu\n",
              red.min_row, red.max_row, red.count, blue.min_row, blue.max_row, blue.count);

  if (red.count < 500 || blue.count < 500) {
    std::fprintf(stderr,
        "ua_stylesheet_sanity FAIL: div não renderizou (red=%zu, blue=%zu, mínimo 500)\n",
        red.count, blue.count);
    return 3;
  }

  const bool disjoint = (red.max_row < blue.min_row) || (blue.max_row < red.min_row);
  if (!disjoint) {
    std::fprintf(stderr,
        "ua_stylesheet_sanity FAIL: faixas de linha se sobrepõem -- divs NÃO empilharam "
        "(red=[%d,%d], blue=[%d,%d])\n",
        red.min_row, red.max_row, blue.min_row, blue.max_row);
    return 4;
  }

  std::puts("ua_stylesheet_sanity: PASS");
  return 0;
}
