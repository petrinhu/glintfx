// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_sup_sanity -- L1.20-FONTFLIP (FT-F4) validation of the superscript-via-RCSS
//     workaround the líder asked this session to validate FOR the GusWorld dev's "a²+b²=c²"
//     problem: instead of needing the real superscript-2 glyph U+00B2 (which "Primary" --
//     assets/PixelOperatorMono.ttf, the GusWorld consumer's OWN font -- is confirmed by cmap probe
//     to LACK), the plain ASCII digit '2' (which PixelOperatorMono DOES have) is styled with RCSS
//     `vertical-align: super` + a smaller `font-size` (see tests/fonteng_sup_scene.rml's own
//     header). NO fallback face is registered in this document at all -- this validation is
//     independent of/unrelated to the fallback feature fonteng_fallback_sanity.cpp gates; it is a
//     pure RCSS-layout claim about vertical-align/font-size on an inline-block span.
//
//     THE TWO CLAIMS THIS FILE GATES (per the líder's brief, "prove get_element_box do span está
//     acima da baseline do texto base (y menor) e menor"):
//       [1] ELEVATED: each `.sup` span's box.y is MEASURABLY smaller (higher on screen -- top-left,
//           y-down convention, same as every get_element_box() oracle in this suite) than its
//           neighbouring baseline-level `base_*` span's box.y, by at least a small, explicit floor
//           (not merely `<`, which a 1-pixel float-rounding fluke could satisfy vacuously).
//       [2] SMALLER: each `.sup` span's box.h is MEASURABLY smaller than BOTH (a) its neighbouring
//           `base_*` span's own box.h (different glyph shape, same font-size -- 'a'/'2' have
//           different natural bbox heights even at equal font-size, so this alone is a weaker
//           signal) and (b) `ref2_full`'s box.h -- the SAME digit '2', SAME font-family, but at the
//           FULL 64px body size on its own line, an apples-to-apples height reference isolating
//           the `font-size: 0.7em` shrink specifically from any glyph-shape confound.
//     Both checks run independently for all three formula instances (sup1/base_a, sup2/base_b,
//     sup3/base_c) -- not just one -- so a single-instance fluke (e.g. a layout edge effect at the
//     line's start) cannot pass this gate alone.
//
//     ARTIFACT (líder's own request: "gere um PNG/PPM ampliado dessa cena"): writes a binary PPM
//     (P6, zero library dependency -- same technique fonteng_ab_visual.cpp already established for
//     this suite) of `formula_line`'s own layout box (padded, then nearest-neighbour upscaled 6x
//     for legibility at this repo's small-body-text scale) to OUTDIR/sup_formula.ppm -- OUTDIR is
//     argv[1] (default "ab_visual" under the CWD, this suite's own diagnostic-artifact directory
//     name, auto-created via std::filesystem if missing so this file is safe to run standalone or
//     under ctest without a manual `mkdir` step first).
// PT: fonteng_sup_sanity -- validação do L1.20-FONTFLIP (FT-F4) do workaround de superíndice via
//     RCSS que o líder pediu nesta sessão para validar PRO problema "a²+b²=c²" do dev do GusWorld:
//     em vez de precisar do glyph real de superscript-2 U+00B2 (que a "Primary" --
//     assets/PixelOperatorMono.ttf, a PRÓPRIA fonte do consumidor GusWorld -- é confirmada por
//     sonda de cmap como SEM), o dígito ASCII plano '2' (que a PixelOperatorMono TEM) é estilizado
//     com RCSS `vertical-align: super` + um `font-size` menor (ver o cabeçalho próprio de
//     tests/fonteng_sup_scene.rml). NENHUMA face de fallback é registrada neste documento -- esta
//     validação é independente/não-relacionada à feature de fallback que o fonteng_fallback_sanity.cpp
//     gateia; é uma alegação pura de layout RCSS sobre vertical-align/font-size num span
//     inline-block.
//
//     AS DUAS ALEGAÇÕES QUE ESTE ARQUIVO GATEIA (conforme a tarefa do líder, "prove que o
//     get_element_box do span está acima da baseline do texto base (y menor) e menor"):
//       [1] ELEVADO: o box.y de cada span `.sup` é MENSURAVELMENTE menor (mais acima na tela --
//           convenção top-left, y-para-baixo, a MESMA de todo oráculo get_element_box() desta
//           suíte) que o box.y do span `base_*` de nível-de-baseline vizinho, por pelo menos um
//           piso explícito pequeno (não meramente `<`, que um capricho de arredondamento float de
//           1 pixel poderia satisfazer vazio).
//       [2] MENOR: o box.h de cada span `.sup` é MENSURAVELMENTE menor que TANTO (a) o próprio
//           box.h do span `base_*` vizinho (forma de glyph diferente, mesmo font-size -- 'a'/'2'
//           têm alturas de bbox naturais diferentes mesmo a font-size igual, então isto sozinho é
//           um sinal mais fraco) QUANTO (b) o box.h do `ref2_full` -- o MESMO dígito '2', MESMA
//           família de fonte, mas no corpo CHEIO de 64px na própria linha, uma referência de altura
//           maçã-com-maçã isolando o encolhimento do `font-size: 0.7em` especificamente de qualquer
//           confusão de forma-de-glyph.
//     Os dois cheques rodam independentemente pras três instâncias da fórmula (sup1/base_a,
//     sup2/base_b, sup3/base_c) -- não só uma -- então um capricho de instância única (ex.: um
//     efeito de borda de layout no início da linha) não passa este gate sozinho.
//
//     ARTEFATO (pedido do próprio líder: "gere um PNG/PPM ampliado dessa cena"): grava um PPM
//     binário (P6, zero dependência de biblioteca -- mesma técnica que o fonteng_ab_visual.cpp já
//     estabeleceu pra esta suíte) da própria caixa de layout de `formula_line` (com padding, depois
//     ampliada por vizinho-mais-próximo 6x pra legibilidade na escala de texto pequeno deste repo)
//     em OUTDIR/sup_formula.ppm -- OUTDIR é argv[1] (default "ab_visual" sob o CWD, o próprio nome
//     de diretório de artefato-diagnóstico desta suíte, auto-criado via std::filesystem se ausente
//     pra este arquivo ser seguro de rodar standalone ou sob ctest sem um passo manual de `mkdir`
//     antes).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr int W = 900, H = 260;
constexpr int kUpscale = 6;

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

// EN: Writes a PADDED, nearest-neighbour-UPSCALED crop of the top-down RGB frame `src` (W wide) as
//     a binary PPM (P6). Zero library dependency -- same technique fonteng_ab_visual.cpp already
//     established for this suite (write_ppm()), extended with an integer upscale pass so this
//     repo's small-body-text scale (this scene's own formula is ~64px tall) is legible without an
//     external viewer doing its own zoom.
// PT: Grava um recorte com PADDING, AMPLIADO por vizinho-mais-próximo, do frame RGB top-down `src`
//     (largura W) como PPM binário (P6). Zero dependência de biblioteca -- mesma técnica que o
//     fonteng_ab_visual.cpp já estabeleceu pra esta suíte (write_ppm()), estendida com um passe de
//     ampliação inteira pra a escala de texto-de-corpo-pequeno deste repo (a própria fórmula desta
//     cena tem ~64px de altura) ser legível sem um visualizador externo fazendo o próprio zoom.
bool write_ppm_upscaled(const std::string& path, const std::vector<unsigned char>& src, int fw, int fh,
                         int x0, int y0, int cw, int ch, int upscale) {
  x0 = std::max(0, x0);
  y0 = std::max(0, y0);
  cw = std::min(cw, fw - x0);
  ch = std::min(ch, fh - y0);
  if (cw <= 0 || ch <= 0) return false;

  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) { std::fprintf(stderr, "fonteng_sup_sanity: cannot open %s\n", path.c_str()); return false; }
  const int ow = cw * upscale, oh = ch * upscale;
  std::fprintf(f, "P6\n%d %d\n255\n", ow, oh);
  std::vector<unsigned char> row(static_cast<size_t>(ow) * 3);
  for (int oy = 0; oy < oh; ++oy) {
    const int sy = y0 + oy / upscale;
    const unsigned char* srow = src.data() + (static_cast<size_t>(sy) * fw + x0) * 3;
    for (int ox = 0; ox < ow; ++ox) {
      const int sx = ox / upscale;
      std::memcpy(&row[static_cast<size_t>(ox) * 3], srow + static_cast<size_t>(sx) * 3, 3);
    }
    std::fwrite(row.data(), 1, row.size(), f);
  }
  std::fclose(f);
  return true;
}

} // namespace

int main(int argc, char** argv) {
  const std::string outdir = (argc > 1) ? argv[1] : "ab_visual";

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_sup_sanity_host", W, H)) {
    std::puts("fonteng_sup_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_sup_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_sup_scene.rml")) {
    std::puts("fonteng_sup_sanity FAIL: load() returned false");
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

  const Box base_a = measure("base_a");
  const Box sup1    = measure("sup1");
  const Box base_b = measure("base_b");
  const Box sup2    = measure("sup2");
  const Box base_c = measure("base_c");
  const Box sup3    = measure("sup3");
  const Box ref2    = measure("ref2_full");
  const Box line    = measure("formula_line");

  std::printf("fonteng_sup_sanity: %dx%d\n", W, H);
  auto print_box = [&](const char* id, const Box& b) {
    std::printf("  %-14s %6s x=%7.2f y=%7.2f w=%7.2f h=%7.2f  ink=%6ld\n",
                id, b.box.found ? "yes" : "no", b.box.x, b.box.y, b.box.w, b.box.h, b.ink);
  };
  print_box("base_a", base_a);
  print_box("sup1",   sup1);
  print_box("base_b", base_b);
  print_box("sup2",   sup2);
  print_box("base_c", base_c);
  print_box("sup3",   sup3);
  print_box("ref2_full", ref2);
  print_box("formula_line", line);

  int failures = 0;
  constexpr long kMinInk = 10; // sup glyphs are ~45px, still comfortably above single-glyph noise floor.

  auto check_ink = [&](const Box& b, const char* id) {
    if (!b.box.found) {
      std::fprintf(stderr, "fonteng_sup_sanity FAIL: span '%s' not found in layout\n", id);
      ++failures;
      return false;
    }
    if (b.ink < kMinInk) {
      std::fprintf(stderr, "fonteng_sup_sanity FAIL: '%s' rendered no ink (ink=%ld < %ld)\n", id, b.ink, kMinInk);
      ++failures;
      return false;
    }
    return true;
  };
  const bool ok_a  = check_ink(base_a, "base_a");
  const bool ok_s1 = check_ink(sup1,   "sup1");
  const bool ok_b  = check_ink(base_b, "base_b");
  const bool ok_s2 = check_ink(sup2,   "sup2");
  const bool ok_c  = check_ink(base_c, "base_c");
  const bool ok_s3 = check_ink(sup3,   "sup3");
  const bool ok_r2 = check_ink(ref2,   "ref2_full");

  // EN: [1] ELEVATED -- box.y strictly, MEASURABLY smaller (>= 1px floor -- not a vacuous `<`).
  // PT: [1] ELEVADO -- box.y estrita, MENSURAVELMENTE menor (piso >= 1px -- não um `<` vazio).
  auto check_elevated = [&](const Box& sup, const Box& base, const char* sup_id, const char* base_id) {
    const float rise = base.box.y - sup.box.y;
    std::printf("fonteng_sup_sanity: [1] %s.y=%.2f  %s.y=%.2f  rise=%.2f (floor >= 1.0)\n",
                sup_id, sup.box.y, base_id, base.box.y, rise);
    if (rise < 1.0f) {
      std::fprintf(stderr,
          "fonteng_sup_sanity FAIL [1] '%s' (y=%.2f) is not measurably ABOVE '%s' (y=%.2f) -- "
          "rise=%.2f < 1.0px; vertical-align:super does not look applied\n",
          sup_id, sup.box.y, base_id, base.box.y, rise);
      ++failures;
    }
  };
  if (ok_s1 && ok_a) check_elevated(sup1, base_a, "sup1", "base_a");
  if (ok_s2 && ok_b) check_elevated(sup2, base_b, "sup2", "base_b");
  if (ok_s3 && ok_c) check_elevated(sup3, base_c, "sup3", "base_c");

  // EN: [2] SMALLER -- box.h measurably smaller than BOTH the neighbouring base_* box.h AND the
  //     apples-to-apples ref2_full box.h (same glyph, same family, full size).
  // PT: [2] MENOR -- box.h mensuravelmente menor que TANTO o box.h do base_* vizinho QUANTO o
  //     box.h maçã-com-maçã do ref2_full (mesmo glyph, mesma família, tamanho cheio).
  auto check_smaller = [&](const Box& sup, const Box& base, const char* sup_id, const char* base_id) {
    std::printf("fonteng_sup_sanity: [2] %s.h=%.2f  %s.h=%.2f  ref2_full.h=%.2f (sup must be < both)\n",
                sup_id, sup.box.h, base_id, base.box.h, ref2.box.h);
    if (sup.box.h >= base.box.h) {
      std::fprintf(stderr,
          "fonteng_sup_sanity FAIL [2] '%s' (h=%.2f) is not smaller than '%s' (h=%.2f)\n",
          sup_id, sup.box.h, base_id, base.box.h);
      ++failures;
    }
    if (sup.box.h >= ref2.box.h) {
      std::fprintf(stderr,
          "fonteng_sup_sanity FAIL [2] '%s' (h=%.2f) is not smaller than ref2_full (h=%.2f, the "
          "SAME digit at full 64px body size) -- font-size:0.7em does not look applied\n",
          sup_id, sup.box.h, ref2.box.h);
      ++failures;
    }
  };
  if (ok_s1 && ok_a && ok_r2) check_smaller(sup1, base_a, "sup1", "base_a");
  if (ok_s2 && ok_b && ok_r2) check_smaller(sup2, base_b, "sup2", "base_b");
  if (ok_s3 && ok_c && ok_r2) check_smaller(sup3, base_c, "sup3", "base_c");

  // --- artifact: upscaled PPM of the whole formula line, for the líder to look at ----------------
  if (!line.box.found) {
    std::fprintf(stderr, "fonteng_sup_sanity FAIL: span 'formula_line' not found -- cannot write artifact\n");
    ++failures;
  } else {
    std::error_code ec;
    std::filesystem::create_directories(outdir, ec);
    constexpr int kPad = 12;
    const int x0 = static_cast<int>(std::floor(line.box.x)) - kPad;
    const int y0 = static_cast<int>(std::floor(line.box.y)) - kPad;
    const int cw = static_cast<int>(std::ceil(line.box.w)) + 2 * kPad;
    const int ch = static_cast<int>(std::ceil(line.box.h)) + 2 * kPad;
    const std::string path = outdir + "/sup_formula.ppm";
    if (!write_ppm_upscaled(path, px, W, H, x0, y0, cw, ch, kUpscale)) {
      std::fprintf(stderr, "fonteng_sup_sanity: WARNING could not write artifact %s (not fatal)\n", path.c_str());
    } else {
      std::printf("fonteng_sup_sanity: wrote artifact %s (%dx%d crop, %dx upscale)\n",
                  path.c_str(), cw, ch, kUpscale);
    }
  }

  if (!ui.ok()) {
    std::fputs("fonteng_sup_sanity FAIL: ok() false after render sequence\n", stderr);
    ++failures;
  }

  if (failures == 0) {
    std::puts("fonteng_sup_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_sup_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
