// SPDX-License-Identifier: MPL-2.0
// EN: Regression test for the bug reported by GusWorld (2026-07-04, real consumer, SDL3 game):
//     Bootstrap::load() stacked documents into the same Rml::Context without ever closing the
//     PREVIOUS one -- (1) a silent memory leak, one live document per reload; (2) a "ghost
//     document" bug, the previous Show()n tree kept rendering UNDERNEATH the new one (most
//     visible with transparency/glow). Reproduced by GusWorld navigating between menu screens
//     in the SAME UiLayer via repeated load(next_path) calls (reload-on-navigate pattern).
//
//     Proven here via Rml::Context::GetNumDocuments() -- only reachable from an internal test
//     that includes engine.hpp + RmlUi/Core/Context.h directly (same pattern as
//     engine_smoke.cpp, which already links RmlUi::Core explicitly for this reason). Exercises
//     Bootstrap::load() through Engine::load(), the exact call both glintfx::App and
//     glintfx::UiLayer delegate to -- so this single test covers both public consumption paths
//     GusWorld could use.
//
//     Assertions:
//       1. load(A) -> load(B) -> load(A) on the SAME Engine (menu-screen navigation, exact
//          GusWorld pattern), each followed by update() (Close()/UnloadDocument() DEFER actual
//          destruction to the next Rml::Context::Update() -- confirmed by reading
//          ElementDocument.h/Context.h in the pinned RmlUi source; Engine::update() already
//          calls Context::Update() once per frame in production, so this drives the exact same
//          step that happens naturally every frame): GetNumDocuments() stays at 1 after each
//          reload -- it must never accumulate.
//       2. 5x repeated load() of the SAME path (worst case of the reload-on-navigate pattern:
//          re-entering the same menu screen) stays stable at 1 document and never crashes.
//
//     Run under -DGLINTFX_SANITIZE=ON (ASan/LSan, applied automatically to every test in this
//     directory via the DIRECTORY PROPERTY TESTS block at the bottom of CMakeLists.txt) to catch
//     the leak directly at the allocator level, not just through the document-count proxy above.
//
// PT: Teste de regressão para o bug relatado pelo GusWorld (2026-07-04, consumidor real, jogo
//     SDL3): Bootstrap::load() empilhava documentos no mesmo Rml::Context sem nunca fechar o
//     ANTERIOR -- (1) vazamento de memória silencioso, um documento vivo por reload; (2) bug de
//     "documento fantasma", a árvore anterior ainda Show()n continuava renderizando POR BAIXO
//     da nova (mais visível com transparência/glow). Reproduzido pelo GusWorld navegando entre
//     telas de menu no MESMO UiLayer via chamadas repetidas de load(proximo_path) (padrão
//     reload-on-navigate).
//
//     Provado aqui via Rml::Context::GetNumDocuments() -- só alcançável de um teste interno que
//     inclui engine.hpp + RmlUi/Core/Context.h diretamente (mesmo padrão de engine_smoke.cpp,
//     que já linka RmlUi::Core explicitamente por este motivo). Exercita Bootstrap::load()
//     através de Engine::load(), a chamada exata para a qual glintfx::App e glintfx::UiLayer
//     delegam -- então este único teste cobre os dois caminhos públicos de consumo que o
//     GusWorld poderia usar.
//
//     Asserções:
//       1. load(A) -> load(B) -> load(A) no MESMO Engine (navegação entre telas de menu, padrão
//          exato do GusWorld), cada um seguido de update() (Close()/UnloadDocument() DIFEREM a
//          destruição real para o próximo Rml::Context::Update() -- confirmado lendo
//          ElementDocument.h/Context.h no source pinado do RmlUi; Engine::update() já chama
//          Context::Update() uma vez por frame em produção, então isso dispara exatamente o
//          mesmo passo que acontece naturalmente a cada frame): GetNumDocuments() permanece em
//          1 após cada reload -- nunca deve acumular.
//       2. 5x load() repetido do MESMO path (pior caso do padrão reload-on-navigate: reentrar na
//          mesma tela de menu) permanece estável em 1 documento e nunca crasha.
//
//     Rodar sob -DGLINTFX_SANITIZE=ON (ASan/LSan, aplicado automaticamente a todo teste deste
//     diretório via o bloco DIRECTORY PROPERTY TESTS no fim do CMakeLists.txt) para pegar o leak
//     diretamente no nível do alocador, não só via o proxy de contagem de documentos acima.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <cstdio>

int main() {
  // EN: Host creates a hidden GL context -- stands in for SDL3 in GusWorld (same fixture
  //     pattern as ui_layer_attach/ui_layer_events; works in both GLINTFX_BACKEND_GLFW
  //     configs via the _glintfx_test_ctx helper wired in tests/CMakeLists.txt).
  // PT: Host cria contexto GL oculto -- representa o SDL3 no GusWorld (mesmo padrão de fixture
  //     de ui_layer_attach/ui_layer_events; funciona nas duas configs de GLINTFX_BACKEND_GLFW
  //     via o helper _glintfx_test_ctx conectado em tests/CMakeLists.txt).
  glintfx::WindowGlfw host;
  if (!host.create("reload-leak", 320, 240)) {
    std::puts("FAIL: host window create");
    return 1;
  }

  glintfx::SystemClock clock;
  glintfx::Engine engine;
  if (!engine.attach(&clock, 320, 240)) {
    std::puts("FAIL: engine attach");
    return 2;
  }

  Rml::Context* ctx = engine.context();
  if (!ctx) {
    std::puts("FAIL: null context after successful attach");
    return 3;
  }

  // ---------------------------------------------------------------------------
  // EN: 1) Navigate A -> B -> A on the SAME Engine -- the exact "menu screen swap" pattern
  //     GusWorld hit. min.rml and min_partial.rml are two DIFFERENT documents already present
  //     in build/tests/ (copied unconditionally / by ui_layer_compose earlier in
  //     tests/CMakeLists.txt).
  // PT: 1) Navega A -> B -> A no MESMO Engine -- o padrão exato de "troca de tela de menu" que
  //     o GusWorld encontrou. min.rml e min_partial.rml são dois documentos DIFERENTES já
  //     presentes em build/tests/ (copiados incondicionalmente / pelo ui_layer_compose antes em
  //     tests/CMakeLists.txt).
  // ---------------------------------------------------------------------------
  const char* nav_sequence[] = { "tests/min.rml", "tests/min_partial.rml", "tests/min.rml" };
  for (const char* path : nav_sequence) {
    if (!engine.load(path)) {
      std::printf("FAIL: load(%s) returned false\n", path);
      return 4;
    }
    // EN: Close()/UnloadDocument() DEFER destruction to the next Context::Update() -- drive it
    //     explicitly so GetNumDocuments() below reflects the post-teardown steady state, exactly
    //     what happens naturally every frame in production via Engine::update().
    // PT: Close()/UnloadDocument() DIFEREM a destruição para o próximo Context::Update() --
    //     dispara explicitamente aqui para que GetNumDocuments() abaixo reflita o estado
    //     estável pós-destruição, exatamente o que acontece naturalmente a cada frame em
    //     produção via Engine::update().
    engine.update();
    const int n = ctx->GetNumDocuments();
    if (n != 1) {
      std::printf("FAIL: GetNumDocuments()=%d after load(%s), expected 1 "
                  "(previous document stacked/leaked instead of closed)\n", n, path);
      return 5;
    }
  }

  // ---------------------------------------------------------------------------
  // EN: 2) Repeated reload of the SAME path, 5x -- worst case of the reload-on-navigate
  //     pattern (re-entering the same menu screen). Must stay stable at 1 document, never
  //     crash and never accumulate under ASan/LSan (GLINTFX_SANITIZE=ON).
  // PT: 2) Reload repetido do MESMO path, 5x -- pior caso do padrão reload-on-navigate
  //     (reentrar na mesma tela de menu). Deve permanecer estável em 1 documento, nunca
  //     crashar e nunca acumular sob ASan/LSan (GLINTFX_SANITIZE=ON).
  // ---------------------------------------------------------------------------
  for (int i = 0; i < 5; ++i) {
    if (!engine.load("tests/min.rml")) {
      std::printf("FAIL: repeated load() #%d returned false\n", i);
      return 6;
    }
    engine.update();
    const int n = ctx->GetNumDocuments();
    if (n != 1) {
      std::printf("FAIL: GetNumDocuments()=%d after repeated load() #%d, expected 1\n", n, i);
      return 7;
    }
  }

  std::puts("document_reload_leak OK");
  return 0;
}
