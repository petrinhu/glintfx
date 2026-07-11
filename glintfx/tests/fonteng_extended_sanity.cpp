// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_extended_sanity -- L1.20-FONTFLIP (FT-F4) sub-phase 2.1 regression net for the
//     WIDENED bake set of glintfx's own SOV-SFNT/SOV-RAST font engine.
//
//     WHAT IT PINS. Sub-phase 2.1 widened FontEngineOwn::BuildBakeSet() (src/font_engine_own.cpp)
//     from "printable ASCII + a hand-picked pt-br accent list" to "printable ASCII + the FULL
//     printable Latin-1 Supplement (U+00A0..U+00FF) + common typographic punctuation". A visual A/B
//     had shown î (U+00EE) and the em-dash (U+2014) SILENTLY VANISHING under the own engine while
//     FreeType drew them -- they simply were not in the fixed set. This test renders, through the
//     OWN engine, a scene (fonteng_extended_scene.rml) whose non-control spans contain ONLY the
//     newly-covered codepoints -- î, em-dash, en-dash, curved single+double quotes, ellipsis,
//     bullet -- with NO ASCII filler, and asserts each span draws real ink. If any target glyph
//     regresses out of the bake set, its span renders zero ink and the matching check fails
//     mechanically instead of by eye.
//
//     WHY OWN-ENGINE, EMBED PATH. The own engine is the DEFAULT in a GLINTFX_OWN_FONT_ENGINE=ON
//     build (Bootstrap::init() installs FontEngineOwn unless the own_font_engine_ab_bypass() test
//     hook is flipped true). This test forces bypass=false explicitly for hygiene (so a shared
//     process that left the flag set cannot silently measure FreeType instead) and runs a single
//     UiLayer (embed) session -- registered OUTSIDE if(GLINTFX_BACKEND_GLFW) via ${_embed_dep}, the
//     same shape as fonteng_ab_compare / fonteng_own_embed_sanity, so it builds and runs in BOTH
//     the GLFW-hosted-App CI config and GusWorld's own embed-only (UiLayer) config.
//
//     METHOD. One hidden GLFW host window provides the GL context. Render two warm-up frames (suite
//     convention -- frame 1 settles layout, frame 2 is stable), glReadPixels the back buffer, flip
//     it top-down to agree with get_element_box()'s top-left/y-down geometry, then for each span
//     count "ink" (near-white) pixels inside its bounding box. A control ASCII span ("Axg") must
//     draw ink too: if IT is blank the whole pipeline broke (not merely these glyphs), which is
//     reported distinctly so a failure is not misread as a bake-set regression.
//
//     Deterministic (no flakiness): get_element_box() is pure layout math; plain un-blurred text
//     ink counts are stable under llvmpipe -- the same statistical-oracle basis fonteng_ab_compare
//     / fonteng_own_embed_sanity already rely on. No sleeps, no retries.
// PT: fonteng_extended_sanity -- rede de regressão da sub-fase 2.1 do L1.20-FONTFLIP (FT-F4) para o
//     conjunto empacotado AMPLIADO do motor de fonte próprio SOV-SFNT/SOV-RAST do glintfx.
//
//     O QUE FIXA. A sub-fase 2.1 ampliou o FontEngineOwn::BuildBakeSet() (src/font_engine_own.cpp)
//     de "ASCII imprimível + uma lista pt-br de acentos escolhida a dedo" para "ASCII imprimível +
//     o Latin-1 Supplement imprimível COMPLETO (U+00A0..U+00FF) + pontuação tipográfica comum". Um
//     A/B visual mostrara î (U+00EE) e o em-dash (U+2014) SUMINDO EM SILÊNCIO sob o motor próprio
//     enquanto o FreeType os desenhava -- simplesmente não estavam no conjunto fixo. Este teste
//     renderiza, pelo motor PRÓPRIO, uma cena (fonteng_extended_scene.rml) cujos spans não-controle
//     contêm SÓ os codepoints recém-cobertos -- î, em-dash, en-dash, aspas curvas simples+duplas,
//     reticências, bullet -- SEM enchimento ASCII, e verifica que cada span desenha tinta real. Se
//     algum glyph alvo regredir pra fora do conjunto, seu span renderiza zero tinta e o cheque
//     correspondente falha mecanicamente em vez de a olho.
//
//     POR QUE MOTOR PRÓPRIO, CAMINHO EMBED. O motor próprio é o DEFAULT num build
//     GLINTFX_OWN_FONT_ENGINE=ON (o Bootstrap::init() instala o FontEngineOwn a menos que o hook de
//     teste own_font_engine_ab_bypass() seja virado true). Este teste força bypass=false
//     explicitamente por higiene (para que um processo compartilhado que deixou o flag setado não
//     meça o FreeType em silêncio) e roda uma única sessão UiLayer (embed) -- registrado FORA de
//     if(GLINTFX_BACKEND_GLFW) via ${_embed_dep}, mesmo formato de fonteng_ab_compare /
//     fonteng_own_embed_sanity, para buildar e rodar tanto no config de CI hospedado-por-GLFW-App
//     quanto no config embed-only (UiLayer) do próprio GusWorld.
//
//     MÉTODO. Uma única janela host GLFW oculta provê o contexto GL. Renderiza dois frames de
//     aquecimento (convenção da suíte -- frame 1 estabiliza layout, frame 2 estável), glReadPixels
//     do back buffer, inverte-o top-down para concordar com a geometria top-left/y-para-baixo do
//     get_element_box(), então pra cada span conta pixels "tinta" (quase-branco) dentro da bounding
//     box. Um span ASCII de controle ("Axg") também precisa desenhar tinta: se ELE ficar em branco
//     o pipeline inteiro quebrou (não só esses glyphs), o que é reportado distintamente para uma
//     falha não ser lida como regressão de conjunto.
//
//     Determinístico (sem flakiness): get_element_box() é pura matemática de layout; contagens de
//     tinta de texto simples sem blur são estáveis sob llvmpipe -- a mesma base de oráculo
//     estatístico que fonteng_ab_compare / fonteng_own_embed_sanity já usam. Sem sleeps, sem retries.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// EN: A/B engine-selection hook, forward-declared locally (see fonteng_ab_compare.cpp for the full
//     rationale of why the header cannot be #include-d from a test target). Forced false below so
//     this test unambiguously measures the OWN engine.
// PT: Hook de seleção de motor A/B, forward-declarado localmente (ver fonteng_ab_compare.cpp pro
//     racional completo de por que o header não pode ser #include-ado de um alvo de teste). Forçado
//     false abaixo para este teste medir inequivocamente o motor PRÓPRIO.
namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

constexpr int W = 640, H = 480;

// EN: Spans in fonteng_extended_scene.rml order. [0] is the ASCII rendering control; the rest each
//     contain ONLY a codepoint the sub-phase-2.1 widening added to the bake set (no ASCII filler),
//     so a blank span pinpoints that glyph regressing out of the set.
// PT: Spans na ordem de fonteng_extended_scene.rml. [0] é o controle de renderização ASCII; o resto
//     contém SÓ um codepoint que a ampliação da sub-fase 2.1 adicionou ao conjunto (sem enchimento
//     ASCII), então um span em branco aponta aquele glyph regredindo pra fora do conjunto.
struct Span {
  const char* id;
  const char* what;  // EN: human label for the failure message. PT: rótulo humano da mensagem.
  bool control;      // EN: true only for the ASCII control. PT: true só pro controle ASCII.
};
const Span kSpans[] = {
    {"ctrl_ascii",   "ASCII control 'Axg'",           true},
    {"ext_icirc",    "i-circumflex U+00EE",           false},
    {"ext_emdash",   "em-dash U+2014",                false},
    {"ext_endash",   "en-dash U+2013",                false},
    {"ext_squote",   "curved single quotes U+2018/9", false},
    {"ext_dquote",   "curved double quotes U+201C/D", false},
    {"ext_ellipsis", "ellipsis U+2026",               false},
    {"ext_bullet",   "bullet U+2022",                 false},
};
constexpr int kN = static_cast<int>(sizeof(kSpans) / sizeof(kSpans[0]));

// EN: glReadPixels' origin is bottom-left; get_element_box() is top-left, y-down -- flip in place
//     so both agree (identical to fonteng_ab_compare / fonteng_own_embed_sanity).
// PT: A origem do glReadPixels é bottom-left; get_element_box() é top-left, y-para-baixo -- inverte
//     in-place para concordarem (idêntico a fonteng_ab_compare / fonteng_own_embed_sanity).
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

// EN: Counts "ink" pixels (any channel > threshold) inside [x,x+w) x [y,y+h), clamped -- px is
//     TOP-DOWN RGB (flip already applied).
// PT: Conta pixels "tinta" (algum canal > threshold) em [x,x+w) x [y,y+h), recortado -- px é RGB
//     TOP-DOWN (flip já aplicado).
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

} // namespace

int main() {
  // EN: Force the OWN engine (see file header). A create() failure is a FAIL, not a SKIP -- Xvfb
  //     provides a real (software) GL context in CI, the repo-wide embed-test convention.
  // PT: Força o motor PRÓPRIO (ver cabeçalho). Um create() falho é FAIL, não SKIP -- o Xvfb provê
  //     um contexto GL real (por software) em CI, convenção repo-wide dos testes embed.
  glintfx::own_font_engine_ab_bypass() = false;

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_extended_sanity_host", W, H)) {
    std::puts("fonteng_extended_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_extended_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  // EN: Deterministic black baseline before load()/render() (same as fonteng_own_embed_sanity).
  // PT: Baseline preto determinístico antes de load()/render() (igual fonteng_own_embed_sanity).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_extended_scene.rml")) {
    std::puts("fonteng_extended_sanity FAIL: load() returned false");
    return 3;
  }

  // EN: Two warm-up frames -- suite convention (frame 1 settles layout, frame 2 is stable).
  // PT: Dois frames de aquecimento -- convenção da suíte (frame 1 estabiliza layout, frame 2
  //     estável).
  ui.update(); ui.render();
  ui.update(); ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  // EN: Minimum ink per span. Deliberately small -- even the thinnest target (the en-dash, a short
  //     mid-line horizontal bar at 48px) covers well over this; the point is "not blank", not a
  //     tight coverage match. A missing glyph yields exactly 0, far below this floor.
  // PT: Tinta mínima por span. Deliberadamente pequeno -- até o alvo mais fino (o en-dash, uma
  //     barra horizontal curta no meio da linha a 48px) cobre bem mais que isto; o ponto é "não em
  //     branco", não um casamento apertado de cobertura. Um glyph faltando dá exatamente 0, bem
  //     abaixo deste piso.
  constexpr long kMinInk = 15;

  std::printf("fonteng_extended_sanity: %dx%d  own-engine per-span ink (widened bake set)\n", W, H);
  std::printf("  %-13s %10s %10s   %s\n", "id", "found", "ink", "what");

  int failures = 0;
  for (int i = 0; i < kN; ++i) {
    const glintfx::ElementBox box = ui.get_element_box(kSpans[i].id);
    const long ink = box.found ? count_ink_rect(px, W, H, box.x, box.y, box.w, box.h) : 0;

    std::printf("  %-13s %10s %10ld   %s\n", kSpans[i].id, box.found ? "yes" : "no", ink,
                kSpans[i].what);

    if (!box.found) {
      std::fprintf(stderr, "fonteng_extended_sanity FAIL: span '%s' (%s) not found in layout\n",
                   kSpans[i].id, kSpans[i].what);
      ++failures;
      continue;
    }
    if (ink < kMinInk) {
      if (kSpans[i].control) {
        std::fprintf(stderr,
            "fonteng_extended_sanity FAIL: ASCII CONTROL '%s' rendered no ink (ink=%ld < %ld) -- "
            "the whole render pipeline is broken, NOT merely the widened glyphs\n",
            kSpans[i].id, ink, kMinInk);
      } else {
        std::fprintf(stderr,
            "fonteng_extended_sanity FAIL: '%s' (%s) rendered no ink (ink=%ld < %ld) -- this "
            "codepoint regressed out of the own engine's widened bake set (BuildBakeSet())\n",
            kSpans[i].id, kSpans[i].what, ink, kMinInk);
      }
      ++failures;
    }
  }

  // EN: Restore the default (hygiene -- no shared process today, but be consistent with
  //     fonteng_ab_compare).
  // PT: Restaura o default (higiene -- sem processo compartilhado hoje, mas consistente com
  //     fonteng_ab_compare).
  glintfx::own_font_engine_ab_bypass() = false;

  if (failures == 0) {
    std::puts("fonteng_extended_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_extended_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
