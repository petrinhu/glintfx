// SPDX-License-Identifier: MPL-2.0
// EN: A1 (framework-2D) -- verifies App::process_event(const UiEvent&), the synthetic/replay
//     channel that shares the exact Engine::process_event route the GLFW physical-input
//     callbacks now feed (window_glfw.cpp) and UiLayer::process_event has always used.
//     (1) Synthetic click: MouseMove + MouseButtonDown/Up at btn_a's coordinates fires
//     set_click_callback with "btn_a" -- same click_at() pattern click_callback_sanity.cpp
//     uses for UiLayer, proving App shares the identical translation.
//     (2) Focus + Enter: App::set_focus("btn_a") + a synthetic Key::Enter down/up ALSO fires
//     the click callback -- RmlUi's own "Enter emulates Click on a tab-index:auto focused
//     element" behaviour (ElementDocument::ProcessDefaultAction, confirmed against the pinned
//     source -- see app_process_event_scene.rcss's header comment), parity with the embed
//     route already exercised by ui_layer_events.cpp's Key::Enter injection (that test only
//     smokes for no-crash; this one asserts the actual click fires).
//     (3) Resize: a documented no-op here (App owns the window and is its own size
//     authority via render()'s AUD-PUB-1 sync_viewport) -- injecting one must not crash and
//     must leave ok() true.
// PT: A1 (framework-2D) -- verifica App::process_event(const UiEvent&), o canal sintético/
//     replay que compartilha a MESMA rota Engine::process_event que os callbacks físicos GLFW
//     agora alimentam (window_glfw.cpp) e que UiLayer::process_event sempre usou.
//     (1) Clique sintético: MouseMove + MouseButtonDown/Up nas coordenadas do btn_a dispara
//     set_click_callback com "btn_a" -- mesmo padrão click_at() que click_callback_sanity.cpp
//     usa para o UiLayer, provando que o App compartilha a tradução idêntica.
//     (2) Focus + Enter: App::set_focus("btn_a") + um Key::Enter sintético down/up TAMBÉM
//     dispara o callback de clique -- o próprio comportamento do RmlUi "Enter emula Click num
//     elemento focado com tab-index:auto" (ElementDocument::ProcessDefaultAction, confirmado
//     contra o source pinado -- ver o comentário de cabeçalho de app_process_event_scene.rcss),
//     paridade com a rota embed já exercitada pela injeção de Key::Enter de
//     ui_layer_events.cpp (aquele teste só faz fumaça de não-crash; este assere que o clique
//     de fato dispara).
//     (3) Resize: um no-op documentado aqui (o App é dono da janela e é a própria autoridade
//     de tamanho via o sync_viewport AUD-PUB-1 de render()) -- injetar um não pode crashar e
//     precisa manter ok() true.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>

namespace {

// EN: VF-APPWHEEL (v0.18.1) -- same settle_scroll() idiom as scroll_sanity.cpp (see that file's
//     own doc-comment for the full derivation): RmlUi's default wheel-driven scroll is a
//     SMOOTHSCROLL animation advanced by real wall-clock time (ScrollController::
//     UpdateSmoothscroll), not synchronous like set_element_scroll_top -- so a single update()
//     call right after process_event(MouseWheel) would very likely observe an in-flight,
//     partially-animated value, not the settled one. Pumps update() with real sleeps until 3
//     consecutive polls agree (animation has settled) or a wall-clock budget runs out.
// PT: VF-APPWHEEL (v0.18.1) -- mesmo idioma settle_scroll() de scroll_sanity.cpp (ver o próprio
//     doc-comment daquele arquivo pra derivação completa): o scroll guiado por wheel do RmlUi por
//     padrão é uma animação SMOOTHSCROLL avançada por tempo real de relógio (ScrollController::
//     UpdateSmoothscroll), não síncrono como set_element_scroll_top -- então uma única chamada a
//     update() logo após process_event(MouseWheel) muito provavelmente observaria um valor em
//     voo, parcialmente animado, não o assentado. Bombeia update() com sleeps reais até 3 polls
//     consecutivos concordarem (animação assentou) ou um orçamento de tempo de relógio se esgotar.
bool approx(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

float settle_scroll(glintfx::App& app, const char* id, int budget_ms = 1500, int poll_ms = 15) {
  float last = 0.f;
  app.get_element_scroll_top(id, last);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(budget_ms);
  int stable_polls = 0;
  while (std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    app.update();
    float now = 0.f;
    app.get_element_scroll_top(id, now);
    if (approx(now, last, 0.05f)) {
      if (++stable_polls >= 3) {
        last = now;
        break;
      }
    } else {
      stable_polls = 0;
    }
    last = now;
  }
  return last;
}

} // namespace

int main() {
  glintfx::App app({ .title = "app_process_event_smoke", .width = 300, .height = 200 });
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 1; }

  std::string last_id = "<none>";
  int hit_count = 0;
  app.set_click_callback([&](const char* id) { last_id = id; ++hit_count; });

  // EN: WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block convention -- see
  //     app_click_callback_smoke.cpp); app_process_event_scene.rml is copied to
  //     CMAKE_CURRENT_BINARY_DIR (build/tests/) by tests/CMakeLists.txt, addressed here with
  //     the "tests/" prefix.
  // PT: WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco GLFW em todo o repo -- ver
  //     app_click_callback_smoke.cpp); app_process_event_scene.rml é copiado para
  //     CMAKE_CURRENT_BINARY_DIR (build/tests/) por tests/CMakeLists.txt, endereçado aqui com
  //     o prefixo "tests/".
  if (!app.load("tests/app_process_event_scene.rml")) { std::puts("FAIL: load"); return 2; }
  app.update();
  app.render();

  // ---------------------------------------------------------------------------
  // EN: (1) Synthetic click on btn_a (left = 10px, top = 10px, w = 80px, h = 40px --
  //     app_process_event_scene.rcss -- (50, 30) lands inside).
  // PT: (1) Clique sintético em btn_a (left = 10px, top = 10px, w = 80px, h = 40px --
  //     app_process_event_scene.rcss -- (50, 30) cai dentro).
  // ---------------------------------------------------------------------------
  app.process_event({ .type = glintfx::UiEvent::Type::MouseMove, .x = 50.f, .y = 30.f });
  app.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = true });
  app.process_event({ .type = glintfx::UiEvent::Type::MouseButton, .button = 0, .pressed = false });

  if (hit_count != 1) {
    std::fprintf(stderr, "FAIL: expected 1 click hit after synthetic mouse click, got %d\n", hit_count);
    return 3;
  }
  if (last_id != "btn_a") {
    std::fprintf(stderr, "FAIL: last_id='%s' expected 'btn_a'\n", last_id.c_str());
    return 4;
  }

  // ---------------------------------------------------------------------------
  // EN: (2) Focus + Enter emulates a second click on the same element (App::set_focus is
  //     App's own pre-existing L1.17-FOCUS surface -- not new in A1 -- reused here to arm the
  //     precondition for RmlUi's Enter-emulates-Click default action).
  // PT: (2) Focus + Enter emula um segundo clique no mesmo elemento (App::set_focus é
  //     superfície pré-existente L1.17-FOCUS do próprio App -- não é nova no A1 -- reusada
  //     aqui para armar a pré-condição da ação padrão "Enter emula Click" do RmlUi).
  // ---------------------------------------------------------------------------
  if (!app.set_focus("btn_a")) { std::puts("FAIL: set_focus(\"btn_a\")"); return 5; }

  app.process_event({ .type = glintfx::UiEvent::Type::Key, .pressed = true,  .key = glintfx::Key::Enter });
  app.process_event({ .type = glintfx::UiEvent::Type::Key, .pressed = false, .key = glintfx::Key::Enter });

  if (hit_count != 2) {
    std::fprintf(stderr, "FAIL: expected 2 click hits after focus+Enter, got %d\n", hit_count);
    return 6;
  }
  if (last_id != "btn_a") {
    std::fprintf(stderr, "FAIL: last_id='%s' expected 'btn_a' after focus+Enter\n", last_id.c_str());
    return 7;
  }

  // ---------------------------------------------------------------------------
  // EN: (3) Resize -- documented no-op on App (see app.hpp doc-comment); must not crash and
  //     must not touch the click-hit count (no click semantics attached to Resize).
  // PT: (3) Resize -- no-op documentado no App (ver doc-comment em app.hpp); não pode crashar
  //     e não pode tocar a contagem de hits de clique (nenhuma semântica de clique ligada ao
  //     Resize).
  // ---------------------------------------------------------------------------
  app.process_event({ .type = glintfx::UiEvent::Type::Resize, .width = 640, .height = 480 });
  if (!app.ok()) { std::puts("FAIL: ok() false after Resize injection"); return 8; }
  if (hit_count != 2) {
    std::fprintf(stderr, "FAIL: Resize injection changed hit_count to %d (expected 2 unchanged)\n", hit_count);
    return 9;
  }

  // ---------------------------------------------------------------------------
  // EN: (4) VF-APPWHEEL (v0.18.1) -- proves the INBOX seed "App has no MouseWheel path" is
  //     stale: App::process_event(UiEvent::Type::MouseWheel) shares the exact
  //     Engine::process_event route window_glfw.cpp's physical scroll callback feeds (see
  //     glfw_event_translate.hpp), the same route UiLayer has always used. This is a VALUE
  //     assertion (get_element_scroll_top), not a "did not crash" smoke -- it goes RED if the
  //     wheel delta is not actually applied, exactly what the INBOX item asked to prove before
  //     being struck. #scroller is the same 900px-content/100px-client geometry
  //     scroll_sanity.cpp already established for UiLayer (see app_process_event_scene.rml's own
  //     comment), so the same oracle applies: a positive-y delta increases scroll_top.
  // PT: (4) VF-APPWHEEL (v0.18.1) -- prova que a semente da INBOX "App não tem caminho de
  //     MouseWheel" está desatualizada: App::process_event(UiEvent::Type::MouseWheel)
  //     compartilha a MESMA rota Engine::process_event que o callback físico de scroll de
  //     window_glfw.cpp alimenta (ver glfw_event_translate.hpp), a mesma rota que o UiLayer
  //     sempre usou. Esta é uma asserção de VALOR (get_element_scroll_top), não uma fumaça de
  //     "não crashou" -- fica VERMELHA se o delta de wheel não for de fato aplicado, exatamente o
  //     que o item da INBOX pediu pra provar antes de ser riscado. #scroller é a mesma geometria
  //     900px-de-conteúdo/100px-de-cliente que scroll_sanity.cpp já estabeleceu para o UiLayer
  //     (ver o próprio comentário de app_process_event_scene.rml), então o mesmo oráculo se
  //     aplica: um delta y positivo aumenta o scroll_top.
  // ---------------------------------------------------------------------------
  float scroll_top = -1.f;
  if (!app.get_element_scroll_top("scroller", scroll_top)) {
    std::puts("FAIL: get_scroll_top(scroller) baseline");
    return 10;
  }
  if (!approx(scroll_top, 0.f, 0.5f)) {
    std::fprintf(stderr, "FAIL: baseline scroll_top=%.2f expected ~0\n", scroll_top);
    return 11;
  }

  // Hover over #scroller (border-box window-space (10,60)-(130,160); (70,110) is well inside),
  // then a positive-y wheel delta must increase scroll_top -- same hover-then-wheel idiom
  // scroll_sanity.cpp uses for UiLayer (Context::ProcessMouseWheel scrolls whatever is under the
  // CURRENT hover target, set by the immediately-preceding MouseMove).
  app.process_event({.type = glintfx::UiEvent::Type::MouseMove, .x = 70.f, .y = 110.f});
  app.process_event({.type = glintfx::UiEvent::Type::MouseWheel, .x = 0.f, .y = 2.f});
  const float top_after_wheel = settle_scroll(app, "scroller");
  if (!(top_after_wheel > 20.f)) {
    std::fprintf(stderr,
                 "FAIL: scroll_top=%.2f after positive wheel delta via App::process_event, expected "
                 "clearly > 0 -- the MouseWheel delta was not applied (VF-APPWHEEL regression)\n",
                 top_after_wheel);
    return 12;
  }
  if (!app.ok()) {
    std::puts("FAIL: ok() false after wheel scroll");
    return 13;
  }

  app.render();
  if (!app.ok()) {
    std::puts("FAIL: ok() false after final render");
    return 14;
  }

  std::puts("app_process_event_smoke: PASS");
  return 0;
}
