// SPDX-License-Identifier: MPL-2.0
// EN: Verifies the GLINTFX-SCROLL-1 feature (v0.3.2, consumer-driven by GusWorld: a 30-item
//     overflow-y:auto menu list could not scroll in embed mode) end-to-end through UiLayer's
//     PUBLIC API only (glintfx::UiLayer::process_event/get_element_scroll_top/
//     set_element_scroll_top/scroll_element_into_view -- never Bootstrap/Engine/Rml::Context
//     directly):
//
//       (A) Wheel forwarding actually scrolls, with teeth on direction: UiEvent{MouseWheel}
//           forwarded to Rml::Context::ProcessMouseWheel scrolls the element under the
//           CURRENT hover target (a MouseMove is sent first, over #scroller -- confirmed by
//           reading Context::ProcessMouseWheel in the pinned RmlUi source: it reads the
//           `hover` member set by the most recent ProcessMouseMove/ProcessMouseButtonDown,
//           resolves Element::GetClosestScrollableContainer() from there, and is a no-op when
//           hover is null). A positive-y wheel delta increases scroll_top; a negative-y delta
//           decreases it -- the "dente" (teeth) that distinguishes real scrolling from a
//           one-way accidental side effect. RmlUi's default scroll behaviour here is a
//           SMOOTHSCROLL animation driven by real wall-clock time (ScrollController::
//           UpdateSmoothscroll, confirmed by reading ScrollController.cpp: velocity model,
//           `smoothscroll_prefer_instant` defaults false and glintfx never overrides it) --
//           NOT synchronous like ScrollIntoView/SetScrollTop below -- so this test pumps
//           update() in a poll loop with real sleeps (settle_scroll() below) instead of
//           asserting on a single frame.
//
//       (B) scroll_element_into_view()/get_element_scroll_top()/set_element_scroll_top() are
//           synchronous (RmlUi's ScrollIntoViewOptions default behavior is
//           ScrollBehavior::Instant per ScrollTypes.h, and Element::SetScrollTop has no
//           animation path at all -- confirmed by reading Element.cpp) -- asserted on the
//           very next get_element_scroll_top() call, no settling needed. B4 below additionally
//           closes a coverage gap found by the GLINTFX-SCROLL-1 review's mutation testing:
//           item-29 (used for the clamped-800 oracle) saturates to the SAME value under both
//           align_with_top=true and =false, so it never discriminates the parameter -- a
//           mutation flipping align_with_top -> !align_with_top in
//           Bootstrap::scroll_element_into_view slipped through undetected. item-10 sits inside
//           the scrollable range for both alignments and yields two DIFFERENT scroll_top values,
//           giving that mutation a tooth (dente).
//
//       (C) Hardening: null id / empty id "" (AUD-TEC-5) / unknown id / no-document-yet on all
//           five new methods return false without crashing; a non-finite (NaN/+inf) scroll_top
//           is REJECTED (verified by proving the scroll_top value stays unchanged after the
//           rejected call, not just by the false return).
//
//       (A0) Wheel over a hover target with NO scrollable ancestor is a safe no-op (does not
//           crash, does not scroll an unrelated ancestor "by accident") -- ProcessMouseWheel
//           resolves the scroll target via hover->GetClosestScrollableContainer(), which itself
//           returns nullptr when no ancestor in the chain is overflow-auto/scroll (confirmed by
//           reading Element::GetClosestScrollableContainer in the pinned RmlUi source).
//
//       (D) The scroll-metrics trio -- get_element_scroll_height()/get_element_client_height(),
//           alongside get_element_scroll_top() -- report the exact scene-authored values
//           (900px / 100px, see the scene oracle below), the numbers a host needs to build a
//           custom scrollbar (see the doc-comments in ui_layer.hpp for the usage formula).
//
//     Scene oracle (scroll_scene.rml/.rcss): #scroller is a 120x100 border-box (no margin/
//     border/padding) containing 30 x 30px-tall items (900px content) -- an exact, deterministic
//     scrollable range of [0, 800] (GetScrollHeight() 900 - GetClientHeight() 100). #item-29's
//     content-local top offset (870px) exceeds that range, so scroll_element_into_view(
//     "item-29", align_with_top=true [default]) is expected to land on the CLAMPED value 800,
//     not 870 -- Element::SetScrollTop clamps internally regardless of ScrollIntoView's exact
//     alignment math, so this is a robust oracle even without replicating RmlUi's alignment
//     formula here.
//
// PT: Verifica a feature GLINTFX-SCROLL-1 (v0.3.2, consumer-driven pelo GusWorld: uma lista de
//     menu de 30 itens overflow-y:auto não conseguia rolar em embed mode) ponta-a-ponta só pela
//     API PÚBLICA do UiLayer (glintfx::UiLayer::process_event/get_element_scroll_top/
//     set_element_scroll_top/scroll_element_into_view -- nunca Bootstrap/Engine/Rml::Context
//     diretamente):
//
//       (A) O encaminhamento de wheel de fato rola, com dente na direção: UiEvent{MouseWheel}
//           encaminhado a Rml::Context::ProcessMouseWheel rola o elemento sob o alvo de hover
//           CORRENTE (um MouseMove é enviado antes, sobre #scroller -- confirmado lendo
//           Context::ProcessMouseWheel no source pinado do RmlUi: lê o membro `hover` definido
//           pelo ProcessMouseMove/ProcessMouseButtonDown mais recente, resolve
//           Element::GetClosestScrollableContainer() a partir dele, e é no-op quando hover é
//           nulo). Um delta de wheel y positivo aumenta o scroll_top; um delta y negativo
//           diminui -- o "dente" que distingue rolagem de verdade de um efeito colateral
//           unidirecional acidental. O comportamento de rolagem default do RmlUi aqui é uma
//           animação SMOOTHSCROLL guiada por tempo real de relógio (ScrollController::
//           UpdateSmoothscroll, confirmado lendo ScrollController.cpp: modelo de velocidade,
//           `smoothscroll_prefer_instant` tem default false e a glintfx nunca sobrepõe) -- NÃO
//           síncrono como ScrollIntoView/SetScrollTop abaixo -- então este teste bombeia
//           update() num loop de poll com sleeps reais (settle_scroll() abaixo) em vez de
//           verificar num único frame.
//
//       (B) scroll_element_into_view()/get_element_scroll_top()/set_element_scroll_top() são
//           síncronos (o behavior default de ScrollIntoViewOptions do RmlUi é
//           ScrollBehavior::Instant conforme ScrollTypes.h, e Element::SetScrollTop não tem
//           caminho de animação nenhum -- confirmado lendo Element.cpp) -- verificado na
//           chamada a get_element_scroll_top() imediatamente seguinte, sem settling necessário.
//           O caso B4 abaixo fecha um gap de cobertura achado pelo mutation testing do review
//           GLINTFX-SCROLL-1: o item-29 (usado no oráculo do valor saturado 800) satura pro
//           MESMO valor sob align_with_top=true e =false, então nunca discrimina o parâmetro --
//           uma mutação trocando align_with_top -> !align_with_top em
//           Bootstrap::scroll_element_into_view passava despercebida. O item-10 fica dentro do
//           intervalo rolável para as duas alinhamentos e produz dois valores de scroll_top
//           DIFERENTES, dando dente a essa mutação.
//
//       (C) Hardening: id nulo / id vazio "" (AUD-TEC-5) / id desconhecido / nenhum-documento-
//           ainda nos cinco métodos novos retornam false sem crashar; um scroll_top não-finito
//           (NaN/+inf) é REJEITADO (verificado provando que o valor de scroll_top permanece
//           inalterado após a chamada
//           rejeitada, não só pelo retorno false).
//
//       (A0) Wheel sobre um alvo de hover SEM ancestral rolável é um no-op seguro (não crasha,
//           não rola um ancestral não-relacionado "por acidente") -- o ProcessMouseWheel
//           resolve o alvo de rolagem via hover->GetClosestScrollableContainer(), que por si
//           retorna nullptr quando nenhum ancestral na cadeia é overflow-auto/scroll
//           (confirmado lendo Element::GetClosestScrollableContainer no source pinado do
//           RmlUi).
//
//       (D) O trio de métricas de rolagem -- get_element_scroll_height()/
//           get_element_client_height(), junto de get_element_scroll_top() -- reporta os
//           valores exatos autorados na cena (900px / 100px, ver o oráculo da cena abaixo), os
//           números que um host precisa para construir uma scrollbar customizada (ver os
//           doc-comments em ui_layer.hpp pra fórmula de uso).
//
//     Oráculo da cena (scroll_scene.rml/.rcss): #scroller é um border-box 120x100 (sem margem/
//     borda/padding) contendo 30 itens de 30px de altura (900px de conteúdo) -- um intervalo
//     rolável exato e determinístico de [0, 800] (GetScrollHeight() 900 - GetClientHeight()
//     100). O offset de topo local-de-conteúdo do #item-29 (870px) excede esse intervalo, então
//     scroll_element_into_view("item-29", align_with_top=true [default]) deve cair no valor
//     SATURADO 800, não 870 -- Element::SetScrollTop satura internamente independente da
//     matemática exata de alinhamento do ScrollIntoView, então este é um oráculo robusto mesmo
//     sem replicar aqui a fórmula de alinhamento do RmlUi.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <thread>

static bool approx(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

// EN: Pump UiLayer::update() with real sleeps until the scroll_top of `id` has been stable
//     across 3 consecutive polls (RmlUi's smoothscroll animation has settled), or a wall-clock
//     budget is exhausted (whichever first) -- see the file header for why this cannot be a
//     single-frame assertion. Returns the last observed scroll_top.
// PT: Bombeia UiLayer::update() com sleeps reais até o scroll_top de `id` ter ficado estável
//     por 3 polls consecutivos (a animação smoothscroll do RmlUi assentou), ou um orçamento de
//     tempo de relógio se esgotar (o que vier primeiro) -- ver o cabeçalho do arquivo pro
//     motivo de isto não poder ser uma asserção de frame único. Retorna o último scroll_top
//     observado.
static float settle_scroll(glintfx::UiLayer& ui, const char* id,
                            int budget_ms = 1500, int poll_ms = 15) {
  float last = 0.f;
  ui.get_element_scroll_top(id, last);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(budget_ms);
  int stable_polls = 0;
  while (std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    ui.update();
    float now = 0.f;
    ui.get_element_scroll_top(id, now);
    if (approx(now, last, 0.05f)) {
      if (++stable_polls >= 3) { last = now; break; }
    } else {
      stable_polls = 0;
    }
    last = now;
  }
  return last;
}

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("scroll_host", 200, 200)) { std::puts("FAIL: host create"); return 1; }

  glintfx::UiLayer ui({ .logical_width = 200, .logical_height = 200, .load_gl = true });
  if (!ui.ok()) { std::puts("FAIL: ui attach"); return 2; }

  // ---------------------------------------------------------------------------
  // (C0) Hardening -- no document loaded yet: all five methods must fail soft.
  // ---------------------------------------------------------------------------
  float probe = 0.f;
  if (ui.scroll_element_into_view("scroller")) { std::puts("FAIL: scroll_into_view before load"); return 3; }
  if (ui.get_element_scroll_top("scroller", probe)) { std::puts("FAIL: get_scroll_top before load"); return 4; }
  if (ui.set_element_scroll_top("scroller", 10.f)) { std::puts("FAIL: set_scroll_top before load"); return 5; }
  if (ui.get_element_scroll_height("scroller", probe)) { std::puts("FAIL: get_scroll_height before load"); return 6; }
  if (ui.get_element_client_height("scroller", probe)) { std::puts("FAIL: get_client_height before load"); return 7; }

  if (!ui.load("scroll_scene.rml")) { std::puts("FAIL: load"); return 8; }
  ui.update(); ui.render();

  // ---------------------------------------------------------------------------
  // (C1) Hardening -- null id / unknown id, document now loaded.
  // ---------------------------------------------------------------------------
  if (ui.scroll_element_into_view(nullptr)) { std::puts("FAIL: scroll_into_view(nullptr)"); return 9; }
  if (ui.scroll_element_into_view("does-not-exist")) { std::puts("FAIL: scroll_into_view(unknown id)"); return 10; }
  if (ui.get_element_scroll_top(nullptr, probe)) { std::puts("FAIL: get_scroll_top(nullptr)"); return 11; }
  if (ui.get_element_scroll_top("does-not-exist", probe)) { std::puts("FAIL: get_scroll_top(unknown id)"); return 12; }
  if (ui.set_element_scroll_top(nullptr, 10.f)) { std::puts("FAIL: set_scroll_top(nullptr)"); return 13; }
  if (ui.set_element_scroll_top("does-not-exist", 10.f)) { std::puts("FAIL: set_scroll_top(unknown id)"); return 14; }
  if (ui.get_element_scroll_height(nullptr, probe)) { std::puts("FAIL: get_scroll_height(nullptr)"); return 15; }
  if (ui.get_element_scroll_height("does-not-exist", probe)) { std::puts("FAIL: get_scroll_height(unknown id)"); return 16; }
  if (ui.get_element_client_height(nullptr, probe)) { std::puts("FAIL: get_client_height(nullptr)"); return 17; }
  if (ui.get_element_client_height("does-not-exist", probe)) { std::puts("FAIL: get_client_height(unknown id)"); return 18; }

  // (C1b, AUD-TEC-5) id="" must fail-high the same way null does -- not silently fall through
  // to a GetElementById("") lookup.
  if (ui.scroll_element_into_view("")) { std::puts("FAIL: scroll_into_view(\"\")"); return 49; }
  if (ui.get_element_scroll_top("", probe)) { std::puts("FAIL: get_scroll_top(\"\")"); return 50; }
  if (ui.set_element_scroll_top("", 10.f)) { std::puts("FAIL: set_scroll_top(\"\")"); return 51; }
  if (ui.get_element_scroll_height("", probe)) { std::puts("FAIL: get_scroll_height(\"\")"); return 52; }
  if (ui.get_element_client_height("", probe)) { std::puts("FAIL: get_client_height(\"\")"); return 53; }

  // ---------------------------------------------------------------------------
  // (B0) Scroll-metrics trio -- scroll_height/client_height match the scene oracle:
  //      900px content (30 items x 30px) / 100px client (see scroll_scene.rcss header).
  // ---------------------------------------------------------------------------
  float scroll_h = -1.f, client_h = -1.f;
  if (!ui.get_element_scroll_height("scroller", scroll_h)) { std::puts("FAIL: get_scroll_height(scroller)"); return 19; }
  if (!approx(scroll_h, 900.f, 1.0f)) {
    std::fprintf(stderr, "FAIL: scroll_height=%.2f expected ~900\n", scroll_h);
    return 20;
  }
  if (!ui.get_element_client_height("scroller", client_h)) { std::puts("FAIL: get_client_height(scroller)"); return 21; }
  if (!approx(client_h, 100.f, 1.0f)) {
    std::fprintf(stderr, "FAIL: client_height=%.2f expected ~100\n", client_h);
    return 22;
  }

  // ---------------------------------------------------------------------------
  // (B1) Baseline -- freshly loaded document, never scrolled: scroll_top == 0.
  // ---------------------------------------------------------------------------
  float top = -1.f;
  if (!ui.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_scroll_top(scroller) baseline"); return 23; }
  if (!approx(top, 0.f, 0.5f)) {
    std::fprintf(stderr, "FAIL: baseline scroll_top=%.2f expected ~0\n", top);
    return 24;
  }

  // ---------------------------------------------------------------------------
  // (B2) set_element_scroll_top / get_element_scroll_top round-trip, synchronous.
  // ---------------------------------------------------------------------------
  if (!ui.set_element_scroll_top("scroller", 50.f)) { std::puts("FAIL: set_scroll_top(scroller, 50)"); return 25; }
  if (!ui.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_scroll_top(scroller) after set"); return 26; }
  if (!approx(top, 50.f, 0.5f)) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f expected ~50 after set_element_scroll_top\n", top);
    return 27;
  }

  // ---------------------------------------------------------------------------
  // (C2) Hardening -- non-finite scroll_top is rejected: return false AND value unchanged.
  // ---------------------------------------------------------------------------
  if (ui.set_element_scroll_top("scroller", std::numeric_limits<float>::quiet_NaN())) {
    std::puts("FAIL: set_scroll_top(NaN) returned true"); return 28;
  }
  if (ui.set_element_scroll_top("scroller", std::numeric_limits<float>::infinity())) {
    std::puts("FAIL: set_scroll_top(+inf) returned true"); return 29;
  }
  if (!ui.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_scroll_top after rejected set"); return 30; }
  if (!approx(top, 50.f, 0.5f)) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f changed after rejected NaN/inf set (expected ~50 unchanged)\n", top);
    return 31;
  }

  // ---------------------------------------------------------------------------
  // (B3) scroll_element_into_view -- item-29 is far past the 800px scrollable max; the
  //      synchronous result must be the CLAMPED value 800 (see file header oracle).
  // ---------------------------------------------------------------------------
  if (!ui.scroll_element_into_view("item-29")) { std::puts("FAIL: scroll_into_view(item-29)"); return 32; }
  if (!ui.get_element_scroll_top("scroller", top)) { std::puts("FAIL: get_scroll_top after scroll_into_view"); return 33; }
  if (!approx(top, 800.f, 1.5f)) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f expected ~800 (clamped) after scroll_into_view(item-29)\n", top);
    return 34;
  }

  // ---------------------------------------------------------------------------
  // (B4) GLINTFX-SCROLL-1 review (mutation testing): item-29 above happens to saturate to the
  //      SAME clamped value (800) under both align_with_top=true and =false, so it never
  //      actually exercises the parameter discriminantly -- a mutation flipping
  //      `align_with_top` -> `!align_with_top` inside Bootstrap::scroll_element_into_view slips
  //      through undetected. item-10 (content-local top offset 300px) sits squarely inside the
  //      [0,800] scrollable range for BOTH alignments, so neither saturates and the two results
  //      diverge. Oracle derived by reading Element::ScrollIntoView/GetScrollOffsetDelta in the
  //      pinned RmlUi source: ScrollAlignment::Start (align_with_top=true) resolves to the
  //      item's content-local top offset verbatim (300); ScrollAlignment::End
  //      (align_with_top=false) resolves to that same offset minus (client_height - item_height)
  //      = 300 - (100 - 30) = 230. Both are independent of the scroll_top in effect BEFORE the
  //      call -- it cancels out algebraically in Element::ScrollIntoView's delta math -- so no
  //      reset is needed between the two calls below.
  // ---------------------------------------------------------------------------
  if (!ui.scroll_element_into_view("item-10", /*align_with_top=*/true)) {
    std::puts("FAIL: scroll_into_view(item-10, align_with_top=true)"); return 35;
  }
  if (!ui.get_element_scroll_top("scroller", top)) {
    std::puts("FAIL: get_scroll_top after scroll_into_view(item-10, align_with_top=true)"); return 36;
  }
  const float top_aligned_top = top;
  if (!approx(top_aligned_top, 300.f, 1.5f)) {
    std::fprintf(stderr,
                 "FAIL: scroll_top=%.2f expected ~300 after scroll_into_view(item-10, align_with_top=true)\n",
                 top_aligned_top);
    return 37;
  }

  if (!ui.scroll_element_into_view("item-10", /*align_with_top=*/false)) {
    std::puts("FAIL: scroll_into_view(item-10, align_with_top=false)"); return 38;
  }
  if (!ui.get_element_scroll_top("scroller", top)) {
    std::puts("FAIL: get_scroll_top after scroll_into_view(item-10, align_with_top=false)"); return 39;
  }
  const float top_aligned_bottom = top;
  if (!approx(top_aligned_bottom, 230.f, 1.5f)) {
    std::fprintf(stderr,
                 "FAIL: scroll_top=%.2f expected ~230 after scroll_into_view(item-10, align_with_top=false)\n",
                 top_aligned_bottom);
    return 40;
  }

  // The dente: the two alignments MUST land on different scroll_top values. This is the
  // assertion mutation testing proved item-29 alone could never provide.
  if (approx(top_aligned_top, top_aligned_bottom, 5.0f)) {
    std::fprintf(stderr,
                 "FAIL: align_with_top=true (%.2f) and align_with_top=false (%.2f) produced the "
                 "SAME scroll_top -- the align_with_top parameter was not exercised discriminantly\n",
                 top_aligned_top, top_aligned_bottom);
    return 41;
  }

  // Reset to a clean baseline before the wheel (part A) assertions below.
  if (!ui.set_element_scroll_top("scroller", 0.f)) { std::puts("FAIL: reset scroll_top to 0"); return 42; }

  // ---------------------------------------------------------------------------
  // (A0) Wheel over an element with NO scrollable ancestor -- (170, 170) is body background,
  //      well outside #scroller's border-box (10,10)-(130,110). Per the confirmed RmlUi
  //      source reading in ui_event.hpp's MouseWheel doc-comment (Context::ProcessMouseWheel,
  //      Context.cpp line 829: `hover->GetClosestScrollableContainer()`), a hover target whose
  //      ancestor chain has no overflow-auto/scroll container resolves to nullptr and the
  //      wheel event is a safe no-op: it must NOT crash, and must NOT scroll #scroller (or any
  //      other unrelated ancestor) "by accident".
  // ---------------------------------------------------------------------------
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 170.f, .y = 170.f });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = 5.f });
  const float top_after_offtarget_wheel = settle_scroll(ui, "scroller", /*budget_ms=*/300);
  if (!approx(top_after_offtarget_wheel, 0.f, 0.5f)) {
    std::fprintf(stderr,
                 "FAIL: scroll_top=%.2f after wheel over a non-scrollable hover target "
                 "(expected ~0, unrelated ancestor must not scroll)\n",
                 top_after_offtarget_wheel);
    return 43;
  }
  if (!ui.ok()) { std::puts("FAIL: ok() false after off-target wheel"); return 44; }

  // ---------------------------------------------------------------------------
  // (A1) Wheel forwarding -- hover over #scroller (border-box window-space (10,10)-(130,110);
  //      (70,60) is well inside), then a positive-y wheel delta must increase scroll_top.
  // ---------------------------------------------------------------------------
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 70.f, .y = 60.f });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = 2.f });
  const float top_after_down = settle_scroll(ui, "scroller");
  if (!(top_after_down > 20.f)) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f after positive wheel delta, expected clearly > 0\n",
                 top_after_down);
    return 45;
  }

  // ---------------------------------------------------------------------------
  // (A2) Dente: a negative-y wheel delta must scroll the OTHER way (scroll_top decreases).
  // ---------------------------------------------------------------------------
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 70.f, .y = 60.f });
  ui.process_event({ .type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = -1.f });
  const float top_after_up = settle_scroll(ui, "scroller");
  if (!(top_after_up < top_after_down - 5.f)) {
    std::fprintf(stderr,
                 "FAIL: scroll_top=%.2f after negative wheel delta did not decrease from %.2f "
                 "(opposite-sign delta must scroll the other way)\n",
                 top_after_up, top_after_down);
    return 46;
  }
  if (top_after_up < -0.5f) {
    std::fprintf(stderr, "FAIL: scroll_top=%.2f went negative\n", top_after_up);
    return 47;
  }

  if (!ui.ok()) { std::puts("FAIL: ok() false after scroll sequence"); return 48; }

  std::puts("scroll_sanity: PASS");
  return 0;
}
