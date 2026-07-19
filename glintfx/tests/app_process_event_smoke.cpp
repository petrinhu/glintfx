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
#include <cstdio>
#include <string>

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

  app.render();
  if (!app.ok()) { std::puts("FAIL: ok() false after final render"); return 10; }

  std::puts("app_process_event_smoke: PASS");
  return 0;
}
