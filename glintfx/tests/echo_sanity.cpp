// SPDX-License-Identifier: MPL-2.0
// EN: echo_sanity -- proves the "sprite-echo" pattern (L1.21-ECHO, consumer-driven by GusWorld: a
//     translucent, blue-tinted "ghost" copy of a sprite, faded in/out by the host) is achievable
//     TODAY with ZERO new glintfx C++ feature -- only RCSS (the "image-tint(<url>)" decorator,
//     GLINTFX-TINT-1/2, shipped since v0.3.x; CSS `opacity`) plus the existing public DOM API
//     (`add_class`/`remove_class`, shipped since v0.9.0's L1.16-DOMRW). This file does NOT touch
//     any glintfx core .cpp/.hpp -- it is a pure integration/acceptance test proving a USAGE
//     PATTERN, the same spirit as image_tint_sanity.cpp proving the underlying decorator itself.
//
//     Three proofs, all measured by framebuffer pixel readback (not visual inspection):
//       Proof 1 (alpha)     -- the echo has LESS colour coverage/brightness than the full sprite
//                               (opacity is actually applied, not a no-op).
//       Proof 2 (hue shift) -- the echo's dominant channel flips from the full sprite's warm
//                               RED-dominant (R>>B) to a cool BLUE-dominant (B>>R) signature (the
//                               tint is actually applied, not a no-op) -- see echo_scene.rcss for
//                               why image-tint-color:#0070ff (R=0) makes this a qualitative
//                               INVERSION rather than a fuzzy quantitative shift.
//       Proof 3 (toggle)    -- the SAME echo recipe, driven purely via
//                               add_class(id,"echo-visible")/remove_class(id,"echo-visible") (the
//                               actual host-facing API), goes background -> visible -> background
//                               again -- i.e. the host can switch a sprite-echo on and off at
//                               runtime, reversibly, with the DOM API alone.
//
//     UiLayer/WindowGlfw embed-mode harness -- same methodology as image_tint_sanity.cpp/
//     polygon_gradient_sanity.cpp (2-frames-no-clear-between capture(), FBO 0 readback,
//     content-space pixel_at() with the y-flip GL <-> RCSS convention). Runs in BOTH build
//     configs (GLINTFX_BACKEND_GLFW=ON and =OFF): image-tint + opacity + add_class/remove_class
//     are all backend-agnostic (Bootstrap-registered decorator + Engine/UiLayer DOM API, no
//     dependency on the GLFW App/window backend option).
//
// PT: echo_sanity -- prova que o padrão "sprite-eco" (L1.21-ECHO, consumer-driven pelo GusWorld:
//     uma cópia "fantasma" translúcida e tingida de azul de um sprite, com fade in/out pelo host)
//     é alcançável HOJE com ZERO feature C++ nova no glintfx -- só RCSS (o decorator
//     "image-tint(<url>)", GLINTFX-TINT-1/2, lançado desde v0.3.x; o `opacity` do CSS) mais a API
//     DOM pública já existente (`add_class`/`remove_class`, lançados desde o L1.16-DOMRW da
//     v0.9.0). Este arquivo NÃO toca nenhum .cpp/.hpp do core do glintfx -- é um teste puro de
//     integração/aceitação provando um PADRÃO DE USO, no mesmo espírito de image_tint_sanity.cpp
//     provando o próprio decorator subjacente.
//
//     Três provas, todas medidas por readback de pixel do framebuffer (não inspeção visual): ver
//     os parágrafos EN acima (espelhados aqui em conceito) -- Prova 1 (alpha: opacity realmente
//     aplicada), Prova 2 (matiz: tint realmente aplicado, inversão qualitativa R->B), Prova 3
//     (toggle: a mesma receita ligada/desligada via add_class/remove_class, reversível).
//
//     Harness embed-mode UiLayer/WindowGlfw -- mesma metodologia de image_tint_sanity.cpp/
//     polygon_gradient_sanity.cpp. Roda em AMBAS as configs de build.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"  // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                          // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Render two frames and read back FBO 0 (GL_BACK) -- same methodology as
//     image_tint_sanity.cpp/polygon_gradient_sanity.cpp.
// PT: Renderiza dois frames e lê de volta o FBO 0 (GL_BACK) -- mesma metodologia de
//     image_tint_sanity.cpp/polygon_gradient_sanity.cpp.
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

// EN: Sample at RCSS content-space coordinates (top-left origin, y-down) -- glReadPixels' row 0
//     is the window BOTTOM, so the row is flipped; column (x) needs no flip. Same convention as
//     image_tint_sanity.cpp/polygon_gradient_sanity.cpp.
// PT: Amostra em coordenadas de espaço-conteúdo do RCSS (origem topo-esquerda, y pra baixo) --
//     mesma convenção de image_tint_sanity.cpp/polygon_gradient_sanity.cpp.
static const unsigned char* pixel_at(const std::vector<unsigned char>& px, int w, int h, int content_x, int content_y) {
  const int row = h - 1 - content_y;
  const int col = content_x;
  return &px[(static_cast<size_t>(row) * w + col) * 3];
}

int main() {
  constexpr int W = 400, H = 300;

  glintfx::WindowGlfw host;
  if (!host.create("echo_sanity_host", W, H)) {
    std::puts("echo_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("echo_sanity FAIL: ui attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  const bool ok_load = ui.load("echo_scene.rml");
  if (!ok_load) {
    std::printf("echo_sanity FAIL: setup -- load=%d\n", ok_load);
    return 3;
  }

  int failures = 0;

  // ===========================================================================================
  // Proofs 1+2 -- static recipe: #sprite_full=(20,20,64,64) centre=(52,52);
  //   #sprite_echo=(120,20,64,64) centre=(152,52). See echo_scene.rcss for the hand-derived
  //   expected values (full~=(220,80,80) idempotent-opaque; echo~=(0,20,46) 2-frame-accumulated).
  // ===========================================================================================
  const auto px = capture(ui, W, H);
  {
    const auto* full = pixel_at(px, W, H, 52, 52);
    const auto* echo = pixel_at(px, W, H, 152, 52);
    std::printf("echo_sanity [static]: full=(%d,%d,%d) echo=(%d,%d,%d)\n",
        full[0], full[1], full[2], echo[0], echo[1], echo[2]);

    // Control: the full sprite must actually be the opaque warm-red circle centre (sanity check
    // on the fixture/geometry itself -- if this fails, the proofs below are meaningless).
    if (!(full[0] > 190 && full[1] < 110 && full[2] < 110 && full[0] - full[2] > 100)) {
      std::puts("echo_sanity FAIL [control]: #sprite_full centre is not the expected opaque "
                 "warm-red circle texel -- fixture/geometry broken, cannot validate the "
                 "sprite-echo proofs below");
      ++failures;
    }

    // Proof 1 -- alpha/coverage: the echo must be MUCH dimmer overall than the full sprite
    // (opacity:0.35 is actually applied, not ignored). Compare total channel brightness.
    const int full_sum = full[0] + full[1] + full[2];
    const int echo_sum = echo[0] + echo[1] + echo[2];
    if (!(echo_sum < full_sum / 2)) {
      std::printf("echo_sanity FAIL [1]: echo not clearly less bright than the full sprite "
                  "(full_sum=%d echo_sum=%d) -- opacity on the echo does not appear to be "
                  "applied\n", full_sum, echo_sum);
      ++failures;
    }
    // Also must not be fully invisible (echo_sum==0 would mean the decorator/opacity combo
    // suppressed the draw entirely rather than dimming it -- a different, wrong failure mode).
    if (!(echo_sum > 15)) {
      std::printf("echo_sanity FAIL [1b]: echo is essentially invisible (echo_sum=%d) -- "
                  "opacity:0.35 should still leave a measurable, translucent draw, not nothing\n",
                  echo_sum);
      ++failures;
    }

    // Proof 2 -- hue shift: the full sprite is RED-dominant (R clearly > B, its native warm
    // colour); the echo must be BLUE-dominant (B clearly > R) -- a qualitative inversion proving
    // image-tint-color:#0070ff/multiply is actually recolouring the sprite, not a no-op.
    if (!(full[0] > full[2] + 60)) {
      std::printf("echo_sanity FAIL [2-control]: #sprite_full is not clearly red-dominant "
                  "(R=%d B=%d) -- control broken\n", full[0], full[2]);
      ++failures;
    }
    if (!(echo[2] > echo[0] + 15)) {
      std::printf("echo_sanity FAIL [2]: echo did not flip to blue-dominant (R=%d B=%d) -- "
                  "image-tint-color/multiply does not appear to be recolouring the echo\n",
                  echo[0], echo[2]);
      ++failures;
    }
  }

  // ===========================================================================================
  // Proof 3 -- toggle via the DOM API: #toggle_echo=(20,120,64,64) centre=(52,152). Off by
  //   default (opacity:0) -> add_class("echo-visible") -> on (same recipe as #sprite_echo) ->
  //   remove_class("echo-visible") -> off again. Each state is captured after an explicit
  //   glClear() so the three readings are independent (not accumulated with each other's
  //   residue) -- see echo_scene.rcss's comment for why.
  // ===========================================================================================
  {
    const auto* off1 = pixel_at(px, W, H, 52, 152);
    std::printf("echo_sanity [toggle before add_class]: (%d,%d,%d)\n", off1[0], off1[1], off1[2]);
    if (!(off1[0] < 10 && off1[1] < 10 && off1[2] < 10)) {
      std::printf("echo_sanity FAIL [3a]: #toggle_echo is not background-black before "
                  "add_class(\"echo-visible\") (got (%d,%d,%d)) -- opacity:0 default not applied\n",
                  off1[0], off1[1], off1[2]);
      ++failures;
    }

    const bool ok_add = ui.add_class("toggle_echo", "echo-visible");
    if (!ok_add) {
      std::puts("echo_sanity FAIL [3-api]: add_class(\"toggle_echo\",\"echo-visible\") returned false");
      ++failures;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    const auto px_on = capture(ui, W, H);
    const auto* on = pixel_at(px_on, W, H, 52, 152);
    std::printf("echo_sanity [toggle after add_class]: (%d,%d,%d)\n", on[0], on[1], on[2]);
    if (!(on[2] > on[0] + 15 && (on[0] + on[1] + on[2]) > 15)) {
      std::printf("echo_sanity FAIL [3b]: #toggle_echo did not turn into the expected translucent "
                  "blue-tinted echo after add_class (got (%d,%d,%d)) -- the host-facing toggle API "
                  "does not drive the sprite-echo recipe on\n", on[0], on[1], on[2]);
      ++failures;
    }

    const bool ok_remove = ui.remove_class("toggle_echo", "echo-visible");
    if (!ok_remove) {
      std::puts("echo_sanity FAIL [3-api]: remove_class(\"toggle_echo\",\"echo-visible\") returned false");
      ++failures;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    const auto px_off2 = capture(ui, W, H);
    const auto* off2 = pixel_at(px_off2, W, H, 52, 152);
    std::printf("echo_sanity [toggle after remove_class]: (%d,%d,%d)\n", off2[0], off2[1], off2[2]);
    if (!(off2[0] < 10 && off2[1] < 10 && off2[2] < 10)) {
      std::printf("echo_sanity FAIL [3c]: #toggle_echo did not return to background-black after "
                  "remove_class(\"echo-visible\") (got (%d,%d,%d)) -- the toggle is not reversible\n",
                  off2[0], off2[1], off2[2]);
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("echo_sanity: PASS");
    return 0;
  }
  std::printf("echo_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
