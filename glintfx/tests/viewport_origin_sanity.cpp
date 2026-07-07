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
//     (4) Pixel column-range shift (X axis, absolute value) AND row-range shift (Y axis,
//         DELTA between the two configs already rendered in (1)/(2)): the rendered marker's
//         bright-pixel column range shifts by exactly offset_x, and its row range shifts by
//         exactly (gl_offset_y at (2)) - (gl_offset_y at (1)) = 70 - 0 = 70 rows -- proves the
//         ACTUAL renderer wiring (RenderInterface_GL3::SetViewport's offset_x/offset_y
//         parameters, gl_offset_y = target_h - y - h) works, not just this test's own
//         bookkeeping. The row check was DROPPED in the first cut of this test on the theory
//         that glReadPixels' row-origin convention made an absolute-row assertion ambiguous;
//         that theory was wrong -- glReadPixels has a well-defined bottom-origin convention
//         (row 0 = window bottom, same as glViewport) -- but rather than hardcode the
//         absolute row (which WOULD couple the test to that convention), this test asserts
//         only the SHIFT between two readbacks, which is convention-agnostic AND still fails
//         hard if gl_offset_y's formula regresses (e.g. gl_offset_y = y instead of
//         target_h - y - h: verified by injecting that exact bug and confirming this
//         assertion fails -- see commit message for the before/after numbers).
//     (5) Resize while in letterbox mode: an injected UiEvent::Type::Resize changes content
//         h -- get_element_box must still report the SAME x offset afterwards (x does not
//         depend on h) and must not crash (gl_offset_y recompute path, see process_event's
//         Resize case in ui_layer.cpp).
//     (6) AUD-TEC-4: set_viewport(0, INT_MAX, 100, INT_MAX, INT_MAX) -- adversarial y/h/target_h
//         near INT_MAX must be a no-op (previous letterbox viewport from (5) kept, no crash, no
//         UBSan signed-overflow hit on `target_h - y - h`), proving the kMaxViewportDim ceiling
//         added to both set_viewport overloads.
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
//     (4) Deslocamento da faixa de coluna de pixel (eixo X, valor absoluto) E da faixa de
//         linha (eixo Y, DELTA entre as duas configs já renderizadas em (1)/(2)): a faixa de
//         colunas de pixel brilhante do marcador renderizado desloca exatamente offset_x, e
//         sua faixa de linhas desloca exatamente (gl_offset_y em (2)) - (gl_offset_y em (1))
//         = 70 - 0 = 70 linhas -- prova que a fiação REAL do renderer (parâmetros
//         offset_x/offset_y de RenderInterface_GL3::SetViewport, gl_offset_y =
//         target_h - y - h) funciona, não só a contabilidade deste teste. A checagem de linha
//         foi CORTADA no primeiro corte deste teste sob a teoria de que a convenção de
//         origem de linha do glReadPixels tornava uma asserção de linha absoluta ambígua;
//         essa teoria estava errada -- o glReadPixels tem convenção bem definida de origem na
//         base (linha 0 = base da janela, igual ao glViewport) -- mas em vez de fixar a linha
//         absoluta (o que ACOPLARIA o teste a essa convenção), este teste assere só o
//         DESLOCAMENTO entre duas leituras, o que é agnóstico à convenção E ainda assim falha
//         forte se a fórmula do gl_offset_y regredir (ex.: gl_offset_y = y em vez de
//         target_h - y - h: verificado injetando exatamente esse bug e confirmando que esta
//         asserção falha -- ver mensagem do commit pros números antes/depois).
//     (5) Resize em modo letterbox: um UiEvent::Type::Resize injetado muda o h de conteúdo --
//         get_element_box deve continuar reportando o MESMO offset x depois (x não depende de
//         h) e não deve crashar (caminho de recálculo de gl_offset_y, ver o case Resize de
//         process_event em ui_layer.cpp).
//     (6) AUD-TEC-4: set_viewport(0, INT_MAX, 100, INT_MAX, INT_MAX) -- y/h/target_h
//         adversariais perto de INT_MAX devem ser no-op (viewport letterbox anterior de (5)
//         mantido, sem crash, sem overflow com sinal no UBSan em `target_h - y - h`), provando
//         o teto kMaxViewportDim adicionado às duas sobrecargas de set_viewport.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"
#include <climits>
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

// EN: min/max BRIGHT pixel ROW. glReadPixels' row 0 is the window BOTTOM (same convention as
//     glViewport's y), so this is well-defined -- but section (4) below deliberately does NOT
//     assert an absolute row value; it asserts the DELTA between two readbacks (offset_y=0 vs
//     offset_y=70), which is agnostic to which end row 0 represents and still catches a wrong
//     gl_offset_y formula (see section (4)'s comment above main()).
// PT: linha mínima/máxima de pixel BRILHANTE. A linha 0 do glReadPixels é a BASE da janela
//     (mesma convenção do y do glViewport), então é bem definido -- mas a seção (4) abaixo
//     deliberadamente NÃO assere um valor de linha absoluto; ela assere o DELTA entre duas
//     leituras (offset_y=0 vs offset_y=70), o que é agnóstico a qual extremidade a linha 0
//     representa e ainda assim pega uma fórmula errada de gl_offset_y (ver o comentário da
//     seção (4) acima de main()).
static void bright_row_range(const std::vector<unsigned char>& px, int w, int h,
                              int& min_row, int& max_row) {
  min_row = -1; max_row = -1;
  for (int row = 0; row < h; ++row)
    for (int col = 0; col < w; ++col) {
      const unsigned char* p = &px[(size_t)(row * w + col) * 3];
      if (p[0] > 200 && p[1] > 200 && p[2] > 200) {
        if (min_row < 0 || row < min_row) min_row = row;
        if (row > max_row) max_row = row;
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
  // EN: baseline row range at gl_offset_y=0 (legacy 2-arg set_viewport above never touches
  //     gl_offset_y, see UiLayer::set_viewport(w,h)) -- saved for the DELTA assertion in
  //     section (4) below, which is what actually exercises the gl_offset_y formula.
  // PT: faixa de linha baseline em gl_offset_y=0 (o set_viewport(w,h) legado de 2 args acima
  //     nunca toca gl_offset_y, ver UiLayer::set_viewport(w,h)) -- guardada pra asserção de
  //     DELTA na seção (4) abaixo, que é o que de fato exercita a fórmula do gl_offset_y.
  int min_row0 = -1, max_row0 = -1;
  bright_row_range(px, WIN_W, WIN_H, min_row0, max_row0);
  if (min_row0 < 0) {
    std::fprintf(stderr, "FAIL: (1) no bright pixel rows found (min_row0)\n");
    return 13;
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
  //     (4b) below: row-range DELTA -- proves the actual GL offset_y (gl_offset_y) wiring.
  // ---------------------------------------------------------------------------
  glReadPixels(0, 0, WIN_W, WIN_H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  int min_col1 = -1, max_col1 = -1;
  bright_col_range(px, WIN_W, WIN_H, min_col1, max_col1);
  if (min_col1 < 0 || !approx((float)min_col1, (float)(min_col0 + 100), 3.f)) {
    std::fprintf(stderr, "FAIL: (4) min_col shifted to %d, expected ~%d (= %d + 100)\n",
                 min_col1, min_col0 + 100, min_col0);
    return 10;
  }

  // EN: (4b) Row-range DELTA -- the assertion this task exists to add. gl_offset_y at (1) is
  //     0 (legacy 2-arg set_viewport); gl_offset_y at (2) is target_h - y - h = 300-80-150 =
  //     70 (see UiLayer::set_viewport(x,y,w,h,target_h) in ui_layer.cpp). Content w/h is
  //     identical in both configs (200x150), so the ONLY thing that can move the bright rows
  //     is that glViewport(offset_x, offset_y, w, h) call inside EndFrame() -- i.e. this is
  //     the actual renderer wiring, not this test's bookkeeping. A wrong formula --
  //     e.g. gl_offset_y = y (= 80) instead of target_h - y - h (= 70) -- shifts by a
  //     different amount and fails this assertion (verified by injecting exactly that bug;
  //     see the commit message for the observed before/after shift values).
  // PT: (4b) DELTA da faixa de linha -- a asserção que esta tarefa existe pra adicionar.
  //     gl_offset_y em (1) é 0 (set_viewport legado de 2 args); gl_offset_y em (2) é
  //     target_h - y - h = 300-80-150 = 70 (ver UiLayer::set_viewport(x,y,w,h,target_h) em
  //     ui_layer.cpp). O w/h do conteúdo é idêntico nas duas configs (200x150), então a ÚNICA
  //     coisa que pode mover as linhas brilhantes é aquela chamada
  //     glViewport(offset_x, offset_y, w, h) dentro de EndFrame() -- ou seja, é a fiação REAL
  //     do renderer, não a contabilidade deste teste. Uma fórmula errada -- ex.:
  //     gl_offset_y = y (= 80) em vez de target_h - y - h (= 70) -- desloca por uma
  //     quantidade diferente e falha esta asserção (verificado injetando exatamente esse bug;
  //     ver a mensagem do commit pros valores de deslocamento observados antes/depois).
  int min_row1 = -1, max_row1 = -1;
  bright_row_range(px, WIN_W, WIN_H, min_row1, max_row1);
  const float expected_row_shift = 70.f;  // target_h(300) - y(80) - h(150), minus gl_offset_y(1)=0
  const float row_shift_min = (float)(min_row1 - min_row0);
  const float row_shift_max = (float)(max_row1 - max_row0);
  if (min_row1 < 0 || !approx(row_shift_min, expected_row_shift, 3.f)) {
    std::fprintf(stderr, "FAIL: (4b) min_row shifted by %.0f, expected ~%.0f "
                 "(gl_offset_y delta = target_h-y-h - 0 = 300-80-150 - 0)\n",
                 row_shift_min, expected_row_shift);
    return 14;
  }
  if (max_row1 < 0 || !approx(row_shift_max, expected_row_shift, 3.f)) {
    std::fprintf(stderr, "FAIL: (4b) max_row shifted by %.0f, expected ~%.0f\n",
                 row_shift_max, expected_row_shift);
    return 15;
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

  // ---------------------------------------------------------------------------
  // (6) AUD-TEC-4 -- adversarial y/h/target_h near INT_MAX must be a no-op: previous letterbox
  //     viewport (from (5), x=120) kept, no crash, no UBSan signed-overflow hit inside
  //     `target_h - y - h`.
  // ---------------------------------------------------------------------------
  ui.set_viewport(0, INT_MAX, 100, INT_MAX, INT_MAX);
  if (!ui.ok()) { std::puts("FAIL: (6) ok() false after adversarial set_viewport"); return 16; }
  ui.update(); ui.render();
  if (!ui.ok()) { std::puts("FAIL: (6) ok() false after render() post adversarial set_viewport"); return 17; }
  auto box3 = ui.get_element_box("target");
  if (!box3.found || !approx(box3.x, 120.f)) {
    std::fprintf(stderr, "FAIL: (6) box.x=%.1f expected ~120 (adversarial set_viewport must be "
                 "a no-op)\n", box3.x);
    return 18;
  }

  std::puts("viewport_origin_sanity: PASS");
  return 0;
}
