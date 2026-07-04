// SPDX-License-Identifier: MPL-2.0
// EN: app_input_hardening_smoke -- App-path parity smoke for the v0.3.0 input-hardening audit
//     fixes (GLFW backend only; the pixel-level proof is input_hardening_sanity via the UiLayer/
//     Engine path, which App shares). Proves that on the standalone App facade too:
//       (1) App::load(nullptr) returns false and does NOT abort the process (Bootstrap::load
//           null-guard -- CRITICAL: without it, std::string(nullptr) -> terminate() -> SIGABRT).
//       (2) App::set_dp_ratio() with 0 / negative / NaN / inf does not crash and leaves ok()
//           true (the invalid ratio is ignored at the common Engine point).
//       (3) The happy path still works: a valid load + set_dp_ratio(2.0) + one render frame.
//     No pixel assertion -- this only proves the App surface is wired to the same guards and does
//     not fault. TOOTH for (1): with the load() null-guard removed, this test aborts (SIGABRT)
//     and never prints PASS.
// PT: app_input_hardening_smoke -- smoke de paridade do caminho App para os fixes da auditoria de
//     hardening de entrada v0.3.0 (só backend GLFW; a prova a nível de pixel é
//     input_hardening_sanity via o caminho UiLayer/Engine, que o App compartilha). Prova que na
//     fachada standalone App também:
//       (1) App::load(nullptr) retorna false e NÃO aborta o processo (null-guard de
//           Bootstrap::load -- CRITICAL: sem ele, std::string(nullptr) -> terminate() -> SIGABRT).
//       (2) App::set_dp_ratio() com 0 / negativo / NaN / inf não crasha e mantém ok() true (o
//           ratio inválido é ignorado no ponto comum do Engine).
//       (3) O caminho feliz ainda funciona: um load válido + set_dp_ratio(2.0) + um frame.
//     Sem asseração de pixel -- só prova que a superfície do App está ligada às mesmas guardas e
//     não falha. DENTE para (1): com o null-guard de load() removido, este teste aborta (SIGABRT)
//     e nunca imprime PASS.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
  glintfx::AppConfig cfg;
  cfg.title  = "app_input_hardening";
  cfg.width  = 320;
  cfg.height = 240;

  glintfx::App app(cfg);
  if (!app.ok()) {
    std::puts("app_input_hardening_smoke FAIL: App init failed");
    return 1;
  }

  // (1) load(nullptr) -> false, no abort.  TOOTH: guard removed -> SIGABRT here.
  if (app.load(nullptr)) {
    std::puts("app_input_hardening_smoke FAIL: load(nullptr) returned true (expected false)");
    return 2;
  }

  // (2) invalid dp ratios: must not crash, ok() stays true.
  app.set_dp_ratio(0.0f);
  app.set_dp_ratio(-1.0f);
  app.set_dp_ratio(std::nanf(""));
  app.set_dp_ratio(std::numeric_limits<float>::infinity());
  if (!app.ok()) {
    std::puts("app_input_hardening_smoke FAIL: ok() false after invalid set_dp_ratio calls");
    return 3;
  }

  // (3) happy path: valid load + valid dp ratio + one frame.
  if (!app.load("tests/min.rml")) {
    std::puts("app_input_hardening_smoke FAIL: valid load(tests/min.rml) returned false");
    return 4;
  }
  app.set_dp_ratio(2.0f);
  app.poll_events();
  app.update();
  app.render();  // EN: must not crash. PT: não deve crashar.
  if (!app.ok()) {
    std::puts("app_input_hardening_smoke FAIL: ok() false after happy-path frame");
    return 5;
  }

  std::puts("app_input_hardening_smoke: PASS");
  return 0;
}
