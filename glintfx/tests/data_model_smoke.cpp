// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test for the data-model API (T1, v0.2.3).
//     Exercises: create_data_model -> bind_string/bind_number -> load -> update + render.
//     Validates:
//       (A) The full lifecycle completes without crash.
//       (B) bind_* called AFTER load() returns false (views compiled on load).
//     Uses glintfx::App (N3: only one RmlUi instance per process; no concurrent UiLayer).
//     WORKING_DIRECTORY is CMAKE_BINARY_DIR so "tests/ui/hud.rml" resolves to
//     build/tests/ui/hud.rml (copied by CMake unconditionally at configure time).
// PT: Teste de fumaça para a API de data-model (T1, v0.2.3).
//     Exercita: create_data_model -> bind_string/bind_number -> load -> update + render.
//     Valida:
//       (A) O ciclo de vida completo termina sem crash.
//       (B) bind_* chamado APÓS load() retorna false (views compiladas no load).
//     Usa glintfx::App (N3: apenas uma instância RmlUi por processo; sem UiLayer concorrente).
//     WORKING_DIRECTORY é CMAKE_BINARY_DIR para que "tests/ui/hud.rml" resolva para
//     build/tests/ui/hud.rml (copiado pelo CMake incondicionalmente no configure).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>

int main() {
  glintfx::AppConfig cfg;
  cfg.title  = "data_model_smoke";
  cfg.width  = 320;
  cfg.height = 240;

  glintfx::App app(cfg);
  if (!app.ok()) {
    std::puts("SKIP: no display / GL context");
    return 0;   // EN: non-fatal under CI without Xvfb. PT: não-fatal em CI sem Xvfb.
  }

  // EN: Step A — lifecycle: create -> bind -> load -> update + render (must not crash).
  // PT: Passo A — ciclo de vida: create -> bind -> load -> update + render (não deve crashar).
  bool ok_create = app.create_data_model("hud");
  bool ok_name   = app.bind_string("name", "Gus");
  bool ok_hp     = app.bind_number("hp", 10.0);
  // EN: Verify load() succeeds — without this check the test would pass even if
  //     hud.rml was not found and no document was ever loaded or rendered.
  // PT: Verifica que load() tem sucesso — sem esta checagem o teste passaria mesmo
  //     se hud.rml não fosse encontrado e nenhum documento fosse carregado ou renderizado.
  bool ok_load   = app.load("tests/ui/hud.rml");

  app.update();
  app.render();

  // EN: Step B — bind_* after load() must return false (too late, views already compiled).
  // PT: Passo B — bind_* após load() deve retornar false (tarde demais, views já compiladas).
  bool late_bind = app.bind_string("should_fail", "x");

  // EN: Report and assert.
  // PT: Reportar e verificar.
  std::printf("create_data_model: %s\n", ok_create ? "OK" : "FAIL");
  std::printf("bind_string(name): %s\n", ok_name   ? "OK" : "FAIL");
  std::printf("bind_number(hp):   %s\n", ok_hp     ? "OK" : "FAIL");
  std::printf("load(hud.rml):     %s\n", ok_load   ? "OK" : "FAIL");
  std::printf("late bind:         %s\n", !late_bind ? "OK (returned false as expected)"
                                                     : "FAIL (should have returned false)");

  bool all_ok = ok_create && ok_name && ok_hp && ok_load && !late_bind;
  if (!all_ok) {
    std::puts("data_model_smoke FAIL");
    return 1;
  }

  std::puts("data_model_smoke OK");
  return 0;
}
