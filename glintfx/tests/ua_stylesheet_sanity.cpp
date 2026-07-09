// SPDX-License-Identifier: MPL-2.0
// EN: Scans the framebuffer for pure-red and pure-blue pixels and records their row
//     range. If the two divs are inline (today's bug, no UA-stylesheet), they share the
//     same line box -- their row ranges OVERLAP. If they are block (fixed by T1), they
//     stack -- their row ranges are DISJOINT. Order-agnostic (doesn't assert which one
//     is "on top"): disjointness alone is the proof, robust to either glReadPixels row
//     convention.
//     v0.6.0: also scans #scroller's rightmost strip for the UA-stylesheet's scrollbar
//     defaults (scrollbarvertical/slidertrack/sliderbar/sliderarrowdec/sliderarrowinc) --
//     WITHOUT them the scrollbar is zero-size (see ua_stylesheet.hpp's doc-comment), so this
//     is a real regression guard, not a decorative addition.
// PT: Varre o framebuffer por pixels vermelho/azul puros e registra a faixa de linhas.
//     Se os 2 divs são inline (bug de hoje, sem UA-stylesheet), compartilham a mesma
//     line box -- as faixas de linha SE SOBREPÕEM. Se são block (corrigido por T1),
//     empilham -- as faixas ficam DISJUNTAS. Não assume qual fica "em cima": a
//     disjunção sozinha é a prova, robusta a qualquer convenção de linha do glReadPixels.
//     v0.6.0: também varre a faixa mais à direita do #scroller pelos defaults de scrollbar
//     da UA-stylesheet (scrollbarvertical/slidertrack/sliderbar/sliderarrowdec/
//     sliderarrowinc) -- SEM eles a scrollbar tem tamanho zero (ver o doc-comment de
//     ua_stylesheet.hpp), então isto é um guard de regressão de verdade, não um adorno.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                           // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
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

// EN: v0.6.0 -- scrollbar-default coverage. #scroller's own background-color is #101010 (~16,16,
//     16, same as body); the UA-stylesheet's scrollbar rules (scrollbarvertical/slidertrack/
//     sliderbar/sliderarrowdec/sliderarrowinc) blend translucent WHITE (alpha 0x20/0x40/0x80)
//     over that base -- the darkest of the three (slidertrack, alpha 0x20 = 12.5%) already
//     resolves to ~46 per channel (16*0.875 + 255*0.125), well above any anti-aliasing noise on
//     the plain #101010 background. Without the UA scrollbar defaults (WidgetScroll zero-sizes
//     the track/arrows when no RCSS-authored height/width resolves -- see ua_stylesheet.hpp's
//     doc-comment), NO pixel in this strip would ever cross this threshold.
// PT: v0.6.0 -- cobertura dos defaults de scrollbar. O background-color próprio do #scroller é
//     #101010 (~16,16,16, igual ao body); as regras de scrollbar da UA-stylesheet
//     (scrollbarvertical/slidertrack/sliderbar/sliderarrowdec/sliderarrowinc) misturam BRANCO
//     translúcido (alpha 0x20/0x40/0x80) sobre essa base -- a mais escura das três (slidertrack,
//     alpha 0x20 = 12,5%) já resolve para ~46 por canal (16*0,875 + 255*0,125), bem acima de
//     qualquer ruído de anti-aliasing sobre o fundo #101010 puro. Sem os defaults de scrollbar da
//     UA (o WidgetScroll zera o tamanho de track/setas quando nenhuma altura/largura autorada em
//     RCSS resolve -- ver o doc-comment de ua_stylesheet.hpp), NENHUM pixel nesta faixa
//     cruzaria este limiar.
static bool is_scrollbar_tint(const unsigned char* p) {
  return p[0] > 35 && p[1] > 35 && p[2] > 35;
}

static size_t scan_col_strip(const std::vector<unsigned char>& px, int w, int h,
                              int x0, int x1, int y0, int y1,
                              bool (*pred)(const unsigned char*)) {
  size_t count = 0;
  for (int row = y0; row < y1 && row < h; ++row)
    for (int col = x0; col < x1 && col < w; ++col) {
      const unsigned char* p = &px[(size_t)(row * w + col) * 3];
      if (pred(p)) ++count;
    }
  return count;
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

  // ---------------------------------------------------------------------------
  // v0.6.0 -- scrollbar-default coverage: #scroller is authored at document-space (top-down)
  // border-box (20,90)-(120,140) (see ua_stylesheet_scene.rcss). glReadPixels' row 0 is the
  // window BOTTOM (well-defined convention, same as glViewport -- confirmed and exercised
  // already by viewport_origin_sanity.cpp), so document-space rows [90,140) map to GL-space rows
  // [H-140, H-90) = [20,70). The scrollbar (UA default width 10dp) occupies the rightmost 10
  // document-space columns of the border-box, [110,120) -- columns need no flip (unambiguous,
  // same reasoning as viewport_origin_sanity.cpp's column scan).
  // ---------------------------------------------------------------------------
  const int scrollbar_x0 = 108, scrollbar_x1 = 121;
  const int scrollbar_y0 = H - 140, scrollbar_y1 = H - 90;
  const size_t scrollbar_tint_count =
      scan_col_strip(px, W, H, scrollbar_x0, scrollbar_x1, scrollbar_y0, scrollbar_y1,
                     is_scrollbar_tint);
  std::printf("ua_stylesheet_sanity: scrollbar tint pixels in strip=%zu (min 150)\n",
              scrollbar_tint_count);
  if (scrollbar_tint_count < 150) {
    std::fprintf(stderr,
        "ua_stylesheet_sanity FAIL: #scroller's vertical scrollbar did not render "
        "(tint pixels=%zu, mínimo 150) -- UA-stylesheet scrollbar defaults missing/broken\n",
        scrollbar_tint_count);
    return 5;
  }

  std::puts("ua_stylesheet_sanity: PASS");
  return 0;
}
