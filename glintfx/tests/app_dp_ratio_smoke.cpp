// SPDX-License-Identifier: MPL-2.0
// EN: App dp_ratio / set_asset_base_url smoke test.
//
//     Covers the two new App API paths added in v0.2.2 that the embed-mode tests
//     (dp_ratio_sanity, base_url_sanity) do not exercise:
//
//       1. AppConfig::dp_ratio — applied at construction time via Engine::set_dp_ratio().
//          Tested by constructing App with cfg.dp_ratio = 2.0f and verifying ok().
//
//       2. App::set_dp_ratio(float) — runtime setter.
//          Tested by calling set_dp_ratio(1.5f) and set_dp_ratio(1.0f) on a live App.
//
//       3. App::set_asset_base_url(const char*) — runtime setter.
//          Tested by calling set_asset_base_url("") (no-op clear) on a live App.
//
//     Each setter must not crash and App::ok() must remain true after the calls.
//     One render frame is issued after the setters to confirm the pipeline stays live.
//
//     This is a smoke test (no pixel comparison). The correctness of dp_ratio scaling is
//     proven in dp_ratio_sanity (UiLayer path); this test only proves the App API surface
//     is wired and the cfg path is applied without fault.
//
// PT: Teste de fumaça do dp_ratio / set_asset_base_url do App.
//
//     Cobre os dois novos caminhos de API do App adicionados na v0.2.2 que os testes do
//     embed mode (dp_ratio_sanity, base_url_sanity) não exercitam:
//
//       1. AppConfig::dp_ratio — aplicado na construção via Engine::set_dp_ratio().
//          Testado construindo App com cfg.dp_ratio = 2.0f e verificando ok().
//
//       2. App::set_dp_ratio(float) — setter runtime.
//          Testado chamando set_dp_ratio(1.5f) e set_dp_ratio(1.0f) num App ativo.
//
//       3. App::set_asset_base_url(const char*) — setter runtime.
//          Testado chamando set_asset_base_url("") (no-op clear) num App ativo.
//
//     Cada setter não deve crashar e App::ok() deve permanecer true após as chamadas.
//     Um frame de render é emitido após os setters para confirmar que o pipeline continua ativo.
//
//     Este é um teste de fumaça (sem comparação de pixels). A corretude do escalonamento de
//     dp_ratio é provada em dp_ratio_sanity (caminho UiLayer); este teste só prova que a
//     superfície de API do App está conectada e que o caminho cfg é aplicado sem falha.
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 — construct App with dp_ratio=2.0f via AppConfig (config path).
  //     This exercises the code path in app.cpp that calls engine.set_dp_ratio(cfg.dp_ratio).
  // PT: Passo 1 — constrói App com dp_ratio=2.0f via AppConfig (caminho config).
  //     Exercita o caminho de código em app.cpp que chama engine.set_dp_ratio(cfg.dp_ratio).
  // ---------------------------------------------------------------------------
  glintfx::AppConfig cfg;
  cfg.title    = "app_dp_ratio";
  cfg.width    = 320;
  cfg.height   = 240;
  cfg.dp_ratio = 2.0f;  // EN: non-default — confirms the config field is applied.
                        // PT: não-padrão — confirma que o campo config é aplicado.

  glintfx::App app(cfg);
  if (!app.ok()) {
    std::puts("app_dp_ratio_smoke FAIL: App init failed (dp_ratio=2.0 in AppConfig)");
    return 1;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 — exercise set_dp_ratio() at runtime (runtime setter path).
  //     Two calls: change value then reset. ok() must remain true throughout.
  // PT: Passo 2 — exercita set_dp_ratio() em runtime (caminho setter runtime).
  //     Duas chamadas: altera valor depois reseta. ok() deve permanecer true.
  // ---------------------------------------------------------------------------
  app.set_dp_ratio(1.5f);   // EN: change — exercises the setter.
                             // PT: altera — exercita o setter.
  if (!app.ok()) {
    std::puts("app_dp_ratio_smoke FAIL: ok() false after set_dp_ratio(1.5f)");
    return 2;
  }

  app.set_dp_ratio(1.0f);   // EN: reset to neutral — idempotent path in SetDensityIndependentPixelRatio.
                             // PT: reseta para neutro — caminho idempotente em SetDensityIndependentPixelRatio.
  if (!app.ok()) {
    std::puts("app_dp_ratio_smoke FAIL: ok() false after set_dp_ratio(1.0f)");
    return 3;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 — exercise set_asset_base_url() at runtime.
  //     Pass "" to clear any base URL (no-op when already empty — tests the API surface).
  // PT: Passo 3 — exercita set_asset_base_url() em runtime.
  //     Passa "" para limpar qualquer base URL (no-op quando já vazio — testa a superfície de API).
  // ---------------------------------------------------------------------------
  app.set_asset_base_url("");  // EN: clear. PT: limpa.
  if (!app.ok()) {
    std::puts("app_dp_ratio_smoke FAIL: ok() false after set_asset_base_url(\"\")");
    return 4;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 4 — load a document and render one frame.
  //     Confirms that the pipeline stays functional after the setter calls.
  //     min.rml is the simplest valid document; it is always present at tests/min.rml
  //     relative to the build root (WORKING_DIRECTORY = CMAKE_BINARY_DIR).
  // PT: Passo 4 — carrega documento e renderiza um frame.
  //     Confirma que o pipeline permanece funcional após as chamadas de setter.
  //     min.rml é o documento válido mais simples; sempre presente em tests/min.rml
  //     relativo à raiz do build (WORKING_DIRECTORY = CMAKE_BINARY_DIR).
  // ---------------------------------------------------------------------------
  app.load("tests/min.rml");
  app.poll_events();
  app.update();
  app.render();  // EN: must not crash. PT: não deve crashar.

  std::puts("app_dp_ratio_smoke: PASS");
  return 0;
}
