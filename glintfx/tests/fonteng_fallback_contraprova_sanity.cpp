// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_fallback_contraprova_sanity -- CONTRA-PROVA half of the L1.20-FONTFLIP (FT-F4)
//     per-glyph fallback gate (see fonteng_fallback_sanity.cpp's own file header, criterion
//     [1-contra], for the full rationale). Loads tests/fonteng_fallback_contraprova_scene.rml --
//     "Primary" (assets/PixelOperatorMono.ttf) ONLY, NO -rmlui-fallback-face block anywhere -- in
//     its OWN fresh process (FontEngineOwn::fallback_faces_ is a flat, process-lifetime vector, so
//     "no fallback face was ever registered" cannot be isolated by loading a second document into
//     fonteng_fallback_sanity's SAME UiLayer session; it requires a fresh process). Same four span
//     ids as fonteng_fallback_scene.rml (ctrl_ascii, greek_omega, greek_alpha_lo, sup2):
//     ctrl_ascii MUST still render ink (pipeline sanity -- Primary alone covers plain ASCII), the
//     other three MUST render ZERO ink (Primary genuinely lacks Ω/α/², confirmed absent from its
//     cmap by a throwaway probe run during this review; with no fallback registered at all,
//     BakeGlyph() has nothing else to try -- negative-cache, exactly the pre-fallback-feature
//     behaviour). This is the differential half of the proof: fonteng_fallback_sanity.cpp's own
//     non-blank result for these EXACT three glyphs is attributable to the fallback mechanism
//     specifically, not some unrelated change (e.g. a font-asset swap, a rendering-pipeline fix).
// PT: fonteng_fallback_contraprova_sanity -- metade CONTRA-PROVA do gate de fallback por-glyph do
//     L1.20-FONTFLIP (FT-F4) (ver o cabeçalho próprio de fonteng_fallback_sanity.cpp, critério
//     [1-contra], pro racional completo). Carrega
//     tests/fonteng_fallback_contraprova_scene.rml -- só "Primary"
//     (assets/PixelOperatorMono.ttf), SEM bloco -rmlui-fallback-face em lugar nenhum -- no PRÓPRIO
//     processo novo (FontEngineOwn::fallback_faces_ é um vetor plano, de vida-de-processo, então
//     "nenhuma face de fallback jamais registrada" não pode ser isolada carregando um 2º documento
//     na MESMA sessão UiLayer do fonteng_fallback_sanity; exige um processo novo). Os mesmos
//     quatro ids de span de fonteng_fallback_scene.rml (ctrl_ascii, greek_omega, greek_alpha_lo,
//     sup2): ctrl_ascii PRECISA renderizar tinta mesmo assim (sanidade de pipeline -- Primary
//     sozinha cobre ASCII plano), os outros três PRECISAM renderizar tinta ZERO (Primary de fato
//     não tem Ω/α/², confirmado ausente do cmap dela por uma sonda descartável rodada durante este
//     review; sem fallback nenhum registrado, o BakeGlyph() não tem mais nada a tentar -- cache
//     negativo, exatamente o comportamento pré-feature-de-fallback). Esta é a metade diferencial da
//     prova: o resultado não-branco do próprio fonteng_fallback_sanity.cpp pra estes EXATOS três
//     glyphs é atribuível ao mecanismo de fallback especificamente, não a alguma mudança
//     não-relacionada (ex.: uma troca de asset de fonte, um fix no pipeline de render).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

constexpr int W = 320, H = 260;

void flip_to_top_down(std::vector<unsigned char>& px, int w, int h) {
  const size_t row_bytes = static_cast<size_t>(w) * 3;
  std::vector<unsigned char> tmp(row_bytes);
  for (int top = 0, bottom = h - 1; top < bottom; ++top, --bottom) {
    unsigned char* rt = px.data() + static_cast<size_t>(top) * row_bytes;
    unsigned char* rb = px.data() + static_cast<size_t>(bottom) * row_bytes;
    std::copy(rt, rt + row_bytes, tmp.data());
    std::copy(rb, rb + row_bytes, rt);
    std::copy(tmp.data(), tmp.data() + row_bytes, rb);
  }
}

long count_ink_rect(const std::vector<unsigned char>& px, int fw, int fh,
                    float rx, float ry, float rw, float rh, int threshold = 100) {
  int x0 = std::max(0, static_cast<int>(std::floor(rx)));
  int y0 = std::max(0, static_cast<int>(std::floor(ry)));
  int x1 = std::min(fw, static_cast<int>(std::ceil(rx + rw)));
  int y1 = std::min(fh, static_cast<int>(std::ceil(ry + rh)));
  long count = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      if (px[i] > threshold || px[i + 1] > threshold || px[i + 2] > threshold) ++count;
    }
  }
  return count;
}

struct Box { glintfx::ElementBox box; long ink; };

} // namespace

int main() {
  glintfx::own_font_engine_ab_bypass() = false;

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_fallback_contraprova_sanity_host", W, H)) {
    std::puts("fonteng_fallback_contraprova_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_fallback_contraprova_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_fallback_contraprova_scene.rml")) {
    std::puts("fonteng_fallback_contraprova_sanity FAIL: load() returned false");
    return 3;
  }

  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  auto measure = [&](const char* id) -> Box {
    const glintfx::ElementBox b = ui.get_element_box(id);
    const long ink = b.found ? count_ink_rect(px, W, H, b.x, b.y, b.w, b.h) : 0;
    return { b, ink };
  };

  const Box ctrl    = measure("ctrl_ascii");
  const Box omega   = measure("greek_omega");
  const Box alpha_lo= measure("greek_alpha_lo");
  const Box sup2    = measure("sup2");

  std::printf("fonteng_fallback_contraprova_sanity: %dx%d\n", W, H);
  auto print_box = [&](const char* id, const Box& b) {
    std::printf("  %-14s %6s %8.2f %6ld\n", id, b.box.found ? "yes" : "no", b.box.w, b.ink);
  };
  print_box("ctrl_ascii",     ctrl);
  print_box("greek_omega",    omega);
  print_box("greek_alpha_lo", alpha_lo);
  print_box("sup2",           sup2);

  int failures = 0;
  constexpr long kMinInk = 15;

  if (!ctrl.box.found || ctrl.ink < kMinInk) {
    std::fprintf(stderr,
        "fonteng_fallback_contraprova_sanity FAIL: ctrl_ascii rendered no ink (found=%d ink=%ld) -- "
        "the whole render pipeline is broken, not merely the (absent) fallback path\n",
        ctrl.box.found, ctrl.ink);
    ++failures;
  }

  auto check_blank = [&](const Box& b, const char* id) {
    if (!b.box.found) {
      std::fprintf(stderr, "fonteng_fallback_contraprova_sanity FAIL: span '%s' not found in layout\n", id);
      ++failures;
      return;
    }
    if (b.ink != 0) {
      std::fprintf(stderr,
          "fonteng_fallback_contraprova_sanity FAIL: '%s' rendered ink=%ld with NO fallback face "
          "registered -- Primary (PixelOperatorMono) is not expected to contain this glyph; a "
          "non-zero ink count here means either PixelOperatorMono unexpectedly DOES have this "
          "glyph (fixture assumption wrong) or something OTHER than the fallback mechanism is "
          "supplying it, which would undermine fonteng_fallback_sanity.cpp's own non-blank result "
          "as proof of the fallback feature specifically\n", id, b.ink);
      ++failures;
    }
  };
  check_blank(omega,    "greek_omega");
  check_blank(alpha_lo, "greek_alpha_lo");
  check_blank(sup2,     "sup2");

  if (!ui.ok()) {
    std::fputs("fonteng_fallback_contraprova_sanity FAIL: ok() false after render sequence\n", stderr);
    ++failures;
  }

  glintfx::own_font_engine_ab_bypass() = false;

  if (failures == 0) {
    std::puts("fonteng_fallback_contraprova_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_fallback_contraprova_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
