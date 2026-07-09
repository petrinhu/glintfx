// SPDX-License-Identifier: MPL-2.0
// EN: input_hardening_sanity -- proves the three input-surface hardening fixes from the v0.3.0
//     security audit reject invalid input fail-high, WITHOUT breaking valid use. Runs in BOTH
//     build configs (embed-mode, links ${_embed_dep}). Reuses dp_ratio_scene.rml/.rcss (a single
//     white 100dp x 100dp box on black) as a deterministic oracle: any layout degeneration (from
//     a NaN dp ratio, a zero viewport, etc.) collapses the box and drops the bright-pixel count
//     to ~0, so "the box is still there at ~its expected size" is the proof the bad input was
//     ignored. Each check has a "tooth": removing the corresponding guard flips the assertion.
//
//     Fix 1 (CRITICAL) -- Bootstrap::load(nullptr): a null path would build std::string(nullptr)
//       -> std::logic_error -> std::terminate() -> SIGABRT, killing the whole host process.
//       Tested by calling load(nullptr) and asserting it returns false. TOOTH: with the guard
//       removed the process aborts here and the test never reaches PASS (non-zero exit).
//     Fix 2 (IMPORTANT) -- Engine::set_dp_ratio: 0/negative collapse dp-sized elements; NaN/inf
//       poison layout. Tested by applying each bad ratio and asserting the box stays ~its
//       baseline size (bad input ignored, previous ratio kept), then a legitimate 2.0 still
//       scales it up ~4x (happy path intact).
//     Fix 3 (IMPORTANT) -- UiLayer::set_viewport: w/h <= 0 degenerates the layout. Tested by
//       applying each bad (w,h) and asserting the box stays ~baseline (previous viewport kept),
//       then a legitimate viewport still renders.
//     Fix 4 (AUD-TEC-2) -- UiLayer::process_event: non-finite ev.x/ev.y in MouseMove would hit
//       UB on static_cast<int>(NaN); in MouseWheel it would poison RmlUi's scroll offset with
//       NaN. Tested by sending NaN/+inf/-inf coords through both event types and asserting no
//       crash, then a legitimate MouseMove + MouseWheel{0,-1} still renders (happy path intact).
//
// PT: input_hardening_sanity -- prova que os tres fixes de hardening da superficie de entrada da
//     auditoria de seguranca v0.3.0 rejeitam entrada invalida fail-high, SEM quebrar uso valido.
//     Roda em AMBOS os configs (embed-mode, linka ${_embed_dep}). Reusa dp_ratio_scene.rml/.rcss
//     (um unico box branco 100dp x 100dp em preto) como oraculo deterministico: qualquer
//     degeneracao de layout (de um dp ratio NaN, um viewport zero, etc.) colapsa o box e derruba
//     a contagem de pixels brilhantes pra ~0, entao "o box ainda esta la em ~seu tamanho
//     esperado" e a prova de que a entrada ruim foi ignorada. Cada check tem um "dente": remover
//     a guarda correspondente inverte a asseracao.
//
//     Fix 1 (CRITICAL) -- Bootstrap::load(nullptr): um path nulo construiria std::string(nullptr)
//       -> std::logic_error -> std::terminate() -> SIGABRT, matando o processo host inteiro.
//       Testado chamando load(nullptr) e verificando que retorna false. DENTE: com a guarda
//       removida o processo aborta aqui e o teste nunca chega ao PASS (exit != 0).
//     Fix 2 (IMPORTANT) -- Engine::set_dp_ratio: 0/negativo colapsa elementos dp; NaN/inf
//       envenena o layout. Testado aplicando cada ratio ruim e verificando que o box permanece
//       ~seu tamanho baseline (entrada ruim ignorada, ratio anterior mantido), depois um 2.0
//       legitimo ainda o escala ~4x (caminho feliz intacto).
//     Fix 3 (IMPORTANT) -- UiLayer::set_viewport: w/h <= 0 degenera o layout. Testado aplicando
//       cada (w,h) ruim e verificando que o box permanece ~baseline (viewport anterior mantido),
//       depois um viewport legitimo ainda renderiza.
//     Fix 4 (AUD-TEC-2) -- UiLayer::process_event: ev.x/ev.y não-finitos em MouseMove bateriam
//       em UB no static_cast<int>(NaN); em MouseWheel envenenariam o offset de scroll do RmlUi
//       com NaN. Testado enviando coords NaN/+inf/-inf pelos dois tipos de evento e verificando
//       ausência de crash, depois um MouseMove + MouseWheel{0,-1} legítimo ainda renderiza
//       (caminho feliz intacto).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                           // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

// EN: Count pure-white pixels (all channels > 200) -- the #box against a black body.
// PT: Conta pixels branco-puro (todos os canais > 200) -- o #box contra um body preto.
static size_t count_bright(const std::vector<unsigned char>& px, int w, int h) {
  size_t n = 0;
  const size_t total = static_cast<size_t>(w) * h;
  for (size_t i = 0; i < total; ++i)
    if (px[i*3+0] > 200 && px[i*3+1] > 200 && px[i*3+2] > 200) ++n;
  return n;
}

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

static size_t bright_now(glintfx::UiLayer& ui, int w, int h) {
  return count_bright(capture(ui, w, h), w, h);
}

int main() {
  constexpr int W = 900, H = 600;

  glintfx::WindowGlfw host;
  if (!host.create("input_hardening_host", W, H)) {
    std::puts("input_hardening_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true, .dp_ratio = 1.0f });
  if (!ui.ok()) {
    std::puts("input_hardening_sanity FAIL: ui attach failed");
    return 2;
  }

  // ---------------------------------------------------------------------------
  // EN: Fix 1 -- load(nullptr) must return false and NOT abort the process.
  //     TOOTH: without the guard, std::string(nullptr) -> terminate() -> SIGABRT here.
  // PT: Fix 1 -- load(nullptr) deve retornar false e NÃO abortar o processo.
  //     DENTE: sem a guarda, std::string(nullptr) -> terminate() -> SIGABRT aqui.
  // ---------------------------------------------------------------------------
  if (ui.load(nullptr)) {
    std::puts("input_hardening_sanity FAIL: load(nullptr) returned true (expected false)");
    return 3;
  }
  std::puts("input_hardening_sanity: load(nullptr) -> false (no abort) OK");

  // ---------------------------------------------------------------------------
  // EN: Load the real scene and establish the baseline bright-pixel count at dp_ratio=1.0.
  // PT: Carrega a cena real e estabelece a contagem de pixels brilhantes baseline em dp_ratio=1.0.
  // ---------------------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  if (!ui.load("dp_ratio_scene.rml")) {
    std::puts("input_hardening_sanity FAIL: load(dp_ratio_scene.rml) returned false");
    return 4;
  }
  const size_t base = bright_now(ui, W, H);
  std::printf("input_hardening_sanity: baseline bright=%zu (100dp box @ dp_ratio=1.0)\n", base);
  if (base < 2500) {   // EN: expect ~10 000 (100x100); conservative floor. PT: espera ~10 000; piso conservador.
    std::puts("input_hardening_sanity FAIL: baseline too low -- scene did not render");
    return 5;
  }

  // EN: "unchanged" tolerance band: bad input must keep the box within +/-25% of baseline.
  //     A collapsed layout (guard removed) would drop to ~0, far below the lower bound.
  // PT: faixa de tolerância "inalterado": entrada ruim deve manter o box dentro de +/-25% do
  //     baseline. Um layout colapsado (guarda removida) cairia pra ~0, bem abaixo do limite.
  const size_t lo = base - base / 4;
  const size_t hi = base + base / 4;

  // ---------------------------------------------------------------------------
  // EN: Fix 2 -- set_dp_ratio invalid values must be ignored (box stays ~baseline).
  // PT: Fix 2 -- valores inválidos de set_dp_ratio devem ser ignorados (box fica ~baseline).
  // ---------------------------------------------------------------------------
  const float bad_ratios[] = { 0.0f, -1.0f,
                               std::nanf(""), std::numeric_limits<float>::infinity() };
  const char*  bad_names[]  = { "0", "-1", "NaN", "inf" };
  for (int i = 0; i < 4; ++i) {
    ui.set_dp_ratio(bad_ratios[i]);
    const size_t b = bright_now(ui, W, H);
    std::printf("input_hardening_sanity: set_dp_ratio(%s) -> bright=%zu (baseline=%zu)\n",
                bad_names[i], b, base);
    if (b < lo || b > hi) {
      std::printf("input_hardening_sanity FAIL: set_dp_ratio(%s) changed the layout "
                  "(bright=%zu outside [%zu,%zu]) -- invalid dp ratio was not rejected\n",
                  bad_names[i], b, lo, hi);
      return 6;
    }
  }

  // ---------------------------------------------------------------------------
  // EN: Fix 2 happy path -- a legitimate 2.0 ratio still scales the box up (~4x area).
  //     Then reset to 1.0 so the viewport checks below start from the known baseline.
  // PT: Fix 2 caminho feliz -- um ratio 2.0 legítimo ainda escala o box (~4x área).
  //     Depois reseta pra 1.0 para os checks de viewport abaixo partirem do baseline conhecido.
  // ---------------------------------------------------------------------------
  ui.set_dp_ratio(2.0f);
  const size_t big = bright_now(ui, W, H);
  std::printf("input_hardening_sanity: set_dp_ratio(2.0) -> bright=%zu (expected ~4x baseline)\n", big);
  if (big < base * 3) {
    std::puts("input_hardening_sanity FAIL: legitimate set_dp_ratio(2.0) did not scale up "
              "(happy path broken by the guard)");
    return 7;
  }
  ui.set_dp_ratio(1.0f);
  const size_t back = bright_now(ui, W, H);
  if (back < lo || back > hi) {
    std::puts("input_hardening_sanity FAIL: set_dp_ratio(1.0) did not restore baseline");
    return 8;
  }

  // ---------------------------------------------------------------------------
  // EN: Fix 3 -- set_viewport with w<=0 or h<=0 must be a no-op (previous viewport kept).
  //     Covers both overloads: (w,h) and (x,y,w,h,target_h).
  // PT: Fix 3 -- set_viewport com w<=0 ou h<=0 deve ser no-op (viewport anterior mantido).
  //     Cobre as duas sobrecargas: (w,h) e (x,y,w,h,target_h).
  // ---------------------------------------------------------------------------
  struct BadVp { int w, h; const char* name; };
  const BadVp bad_vps[] = { {0, H, "(0,H)"}, {W, 0, "(W,0)"}, {-1, H, "(-1,H)"}, {W, -5, "(W,-5)"} };
  for (const auto& v : bad_vps) {
    ui.set_viewport(v.w, v.h);                    // 2-arg overload
    size_t b = bright_now(ui, W, H);
    if (b < lo || b > hi) {
      std::printf("input_hardening_sanity FAIL: set_viewport%s [2-arg] degenerated the layout "
                  "(bright=%zu outside [%zu,%zu])\n", v.name, b, lo, hi);
      return 9;
    }
    ui.set_viewport(0, 0, v.w, v.h, H);           // 5-arg overload
    b = bright_now(ui, W, H);
    if (b < lo || b > hi) {
      std::printf("input_hardening_sanity FAIL: set_viewport%s [5-arg] degenerated the layout "
                  "(bright=%zu outside [%zu,%zu])\n", v.name, b, lo, hi);
      return 10;
    }
  }
  std::puts("input_hardening_sanity: invalid set_viewport (both overloads) -> no-op OK");

  // ---------------------------------------------------------------------------
  // EN: Fix 3 happy path -- a legitimate viewport still applies and renders the box.
  // PT: Fix 3 caminho feliz -- um viewport legítimo ainda aplica e renderiza o box.
  // ---------------------------------------------------------------------------
  ui.set_viewport(W, H);
  const size_t valid = bright_now(ui, W, H);
  std::printf("input_hardening_sanity: set_viewport(W,H) -> bright=%zu (baseline=%zu)\n", valid, base);
  if (valid < lo || valid > hi) {
    std::puts("input_hardening_sanity FAIL: legitimate set_viewport(W,H) did not render the box "
              "(happy path broken by the guard)");
    return 11;
  }

  // ---------------------------------------------------------------------------
  // EN: Fix 4 (AUD-TEC-2) -- process_event() must reject non-finite ev.x/ev.y instead of
  //     UB-casting them (MouseMove: static_cast<int>(NaN) is UB per [conv.fpint]) or poisoning
  //     RmlUi's scroll offset with NaN (MouseWheel). No pixel oracle needed here -- the tooth
  //     is a crash/UB, not a layout change: with the guard removed this loop may crash, hang,
  //     or silently corrupt scroll state depending on the implementation.
  // PT: Fix 4 (AUD-TEC-2) -- process_event() deve rejeitar ev.x/ev.y não-finitos em vez de
  //     castar via UB (MouseMove: static_cast<int>(NaN) é UB por [conv.fpint]) ou envenenar o
  //     offset de scroll do RmlUi com NaN (MouseWheel). Sem oráculo de pixel aqui -- o dente é
  //     um crash/UB, não mudança de layout: com a guarda removida este laço pode crashar,
  //     travar, ou corromper o estado de scroll silenciosamente dependendo da implementação.
  // ---------------------------------------------------------------------------
  const float bad_coords[] = { std::nanf(""), std::numeric_limits<float>::infinity(),
                                -std::numeric_limits<float>::infinity() };
  for (float bc : bad_coords) {
    ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = bc, .y = bc });
    ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = bc });
    ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = bc, .y = 0.f });
  }
  std::puts("input_hardening_sanity: MouseMove/MouseWheel non-finite coords -> no crash OK");

  // EN: happy path intact -- a legitimate MouseMove + MouseWheel{0,-1} still work (no crash,
  //     scene still renders at ~baseline; dp_ratio_scene.rml has no scrollable content so this
  //     is a render/crash proof, not a scroll-amount proof -- scroll behavior itself is covered
  //     by scroll_sanity.cpp).
  // PT: caminho feliz intacto -- MouseMove + MouseWheel{0,-1} legítimos ainda funcionam (sem
  //     crash, cena ainda renderiza em ~baseline; dp_ratio_scene.rml não tem conteúdo rolável
  //     então isto é prova de render/crash, não de quantidade de rolagem -- o comportamento de
  //     scroll em si é coberto por scroll_sanity.cpp).
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 50.f, .y = 50.f });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = -1.f });
  const size_t after_wheel = bright_now(ui, W, H);
  if (after_wheel < lo || after_wheel > hi) {
    std::puts("input_hardening_sanity FAIL: legitimate MouseMove/MouseWheel broke the layout "
              "(happy path broken by the guard)");
    return 12;
  }

  std::puts("input_hardening_sanity: PASS");
  return 0;
}
