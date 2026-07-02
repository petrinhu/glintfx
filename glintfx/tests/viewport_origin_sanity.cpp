// SPDX-License-Identifier: MPL-2.0
// EN: Integration proof for the F3 coordinate contract -- exercises F1 (click) + F2
//     (get_element_box) + F3 (set_viewport with origin) together, because they share ONE
//     window-space coordinate system by design (see Architecture in the v0.2.5 plan).
//
//     (1) Backward-compat: legacy set_viewport(w,h) (2-arg) still works -- click + box query
//         at offset (0,0) behave exactly as before F3.
//     (2) Letterbox: after set_viewport(x=100,y=80,w=200,h=150,target_h=300), get_element_box
//         returns WINDOW-space coordinates shifted by exactly the offset (deterministic --
//         no pixel reading, no flakiness).
//     (3) Click-through-offset (the CENTREPIECE): a MouseMove+MouseButton at WINDOW
//         coordinates equal to (old local target centre + offset) still hits "target" --
//         proves mouse translation accounts for the offset. A click at the OLD
//         (pre-offset) window coordinates must NOT hit anything any more -- proves the
//         translation is not just "sometimes right".
//     (4) Pixel column-range shift (X axis only -- unambiguous regardless of glReadPixels'
//         row-origin convention): the rendered marker's bright-pixel column range shifts by
//         exactly offset_x -- proves the ACTUAL renderer wiring (RenderInterface_GL3::
//         SetViewport's offset_x/offset_y parameters) works, not just this test's own
//         bookkeeping.
//     (5) Resize while in letterbox mode: an injected UiEvent::Type::Resize changes content
//         h -- get_element_box must still report the SAME x offset afterwards (x does not
//         depend on h) and must not crash (gl_offset_y recompute path, see process_event's
//         Resize case in ui_layer.cpp).
//
// PT: Prova de integração do contrato de coordenadas da F3 -- exercita F1 (clique) + F2
//     (get_element_box) + F3 (set_viewport com origem) juntas, porque compartilham UM único
//     sistema de coordenadas espaço-janela por design (ver Architecture no plano da v0.2.5).
//
//     (1) Compat retroativa: set_viewport(w,h) legado (2 args) continua funcionando -- clique
//         + query de box em offset (0,0) se comportam exatamente como antes da F3.
//     (2) Letterbox: após set_viewport(x=100,y=80,w=200,h=150,target_h=300), get_element_box
//         retorna coordenadas ESPAÇO-JANELA deslocadas exatamente pelo offset (determinístico
//         -- sem leitura de pixel, sem flakiness).
//     (3) Clique-através-do-offset (o CENTRO da prova): um MouseMove+MouseButton em
//         coordenadas de JANELA iguais a (centro do alvo local antigo + offset) ainda acerta
//         "target" -- prova que a tradução de mouse contabiliza o offset. Um clique nas
//         coordenadas de janela ANTIGAS (pré-offset) não deve mais acertar nada -- prova que
//         a tradução não é só "às vezes certa".
//     (4) Deslocamento da faixa de coluna de pixel (só eixo X -- inequívoco independente da
//         convenção de origem de linha do glReadPixels): a faixa de colunas de pixel
//         brilhante do marcador renderizado desloca exatamente offset_x -- prova que a
//         fiação REAL do renderer (parâmetros offset_x/offset_y de RenderInterface_GL3::
//         SetViewport) funciona, não só a contabilidade deste teste.
//     (5) Resize em modo letterbox: um UiEvent::Type::Resize injetado muda o h de conteúdo --
//         get_element_box deve continuar reportando o MESMO offset x depois (x não depende de
//         h) e não deve crashar (caminho de recálculo de gl_offset_y, ver o case Resize de
//         process_event em ui_layer.cpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

static bool approx(float a, float b, float tol = 1.0f) { return std::fabs(a - b) <= tol; }

static void click_at(glintfx::UiLayer& ui, float x, float y) {
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = x, .y = y });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = true });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = false });
}

// EN: min/max BRIGHT pixel COLUMN (unambiguous -- no row-origin flip involved).
// PT: coluna mínima/máxima de pixel BRILHANTE (inequívoco -- sem flip de origem de linha).
static void bright_col_range(const std::vector<unsigned char>& px, int w, int h,
                              int& min_col, int& max_col) {
  min_col = -1; max_col = -1;
  for (int row = 0; row < h; ++row)
    for (int col = 0; col < w; ++col) {
      const unsigned char* p = &px[(size_t)(row * w + col) * 3];
      if (p[0] > 200 && p[1] > 200 && p[2] > 200) {
        if (min_col < 0 || col < min_col) min_col = col;
        if (col > max_col) max_col = col;
      }
    }
}

int main() {
  constexpr int WIN_W = 400, WIN_H = 300;
  glintfx::WindowGlfw host;
  if (!host.create("viewport_origin_host", WIN_W, WIN_H)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 200, .logical_height = 150, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  std::vector<std::string> hits;
  ui.set_click_callback([&hits](const char* id) { hits.emplace_back(id); });

  if (!ui.load("viewport_origin_scene.rml")) { std::puts("FAIL: load"); return 3; }

  // ---------------------------------------------------------------------------
  // (1) Backward-compat -- legacy 2-arg set_viewport, offset (0,0).
  // ---------------------------------------------------------------------------
  ui.set_viewport(200, 150);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  ui.update(); ui.render();

  auto box0 = ui.get_element_box("target");
  if (!box0.found || !approx(box0.x, 20.f) || !approx(box0.y, 15.f)) {
    std::fprintf(stderr, "FAIL: (1) box=(%.1f,%.1f) expected ~(20,15)\n", box0.x, box0.y);
    return 4;
  }
  click_at(ui, 20.f + 30.f, 15.f + 20.f);  // centre of the 60x40 box, no offset
  if (hits.size() != 1 || hits[0] != "target") {
    std::fprintf(stderr, "FAIL: (1) click at legacy coords did not hit 'target'\n");
    return 5;
  }

  std::vector<unsigned char> px((size_t)WIN_W * WIN_H * 3);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, WIN_W, WIN_H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  int min_col0 = -1, max_col0 = -1;
  bright_col_range(px, WIN_W, WIN_H, min_col0, max_col0);
  if (min_col0 < 0 || !approx((float)min_col0, 20.f, 3.f)) {
    std::fprintf(stderr, "FAIL: (1) min_col=%d expected ~20\n", min_col0);
    return 6;
  }

  // ---------------------------------------------------------------------------
  // (2) Letterbox -- set_viewport(x=100,y=80,w=200,h=150,target_h=300).
  // ---------------------------------------------------------------------------
  hits.clear();
  ui.set_viewport(100, 80, 200, 150, 300);
  glClear(GL_COLOR_BUFFER_BIT);  // host re-clears the FULL window (render() never clears).
  ui.update(); ui.render();

  auto box1 = ui.get_element_box("target");
  if (!box1.found || !approx(box1.x, 120.f) || !approx(box1.y, 95.f)) {
    std::fprintf(stderr, "FAIL: (2) box=(%.1f,%.1f) expected ~(120,95)\n", box1.x, box1.y);
    return 7;
  }

  // ---------------------------------------------------------------------------
  // (3) Click-through-offset -- the centrepiece assertion.
  // ---------------------------------------------------------------------------
  click_at(ui, 100.f + 20.f + 30.f, 80.f + 15.f + 20.f);  // offset + old local centre
  if (hits.size() != 1 || hits[0] != "target") {
    std::fprintf(stderr, "FAIL: (3) click at offset-translated window coords did not hit 'target' (got %zu hits)\n", hits.size());
    return 8;
  }
  click_at(ui, 20.f + 30.f, 15.f + 20.f);  // OLD pre-offset coords -- must miss now
  if (hits.size() != 1) {
    std::fprintf(stderr, "FAIL: (3) click at STALE pre-offset coords incorrectly hit an element\n");
    return 9;
  }

  // ---------------------------------------------------------------------------
  // (4) Pixel column-range shift -- proves the actual GL offset_x wiring.
  // ---------------------------------------------------------------------------
  glReadPixels(0, 0, WIN_W, WIN_H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  int min_col1 = -1, max_col1 = -1;
  bright_col_range(px, WIN_W, WIN_H, min_col1, max_col1);
  if (min_col1 < 0 || !approx((float)min_col1, (float)(min_col0 + 100), 3.f)) {
    std::fprintf(stderr, "FAIL: (4) min_col shifted to %d, expected ~%d (= %d + 100)\n",
                 min_col1, min_col0 + 100, min_col0);
    return 10;
  }

  // ---------------------------------------------------------------------------
  // (5) Resize while in letterbox mode -- x offset must stay intact, no crash.
  // ---------------------------------------------------------------------------
  ui.process_event({ .type = glintfx::UiEvent::Type::Resize, .width = 220, .height = 160 });
  ui.update(); ui.render();
  if (!ui.ok()) { std::puts("FAIL: (5) ok() false after Resize in letterbox mode"); return 11; }
  auto box2 = ui.get_element_box("target");
  if (!box2.found || !approx(box2.x, 120.f)) {
    std::fprintf(stderr, "FAIL: (5) box.x=%.1f expected ~120 (unchanged by Resize)\n", box2.x);
    return 12;
  }

  std::puts("viewport_origin_sanity: PASS");
  return 0;
}
