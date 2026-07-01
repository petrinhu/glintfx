// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — open App, load a minimal document, run 1 frame headless.
//     Also checks glintfx::version() against the current release ("0.2.4") —
//     catches a regression to a hardcoded stale literal in app.cpp (was "0.1.0"
//     until v0.2.4) as well as a stale project(VERSION) in CMakeLists.txt.
// PT: Teste de fumaça — abre App, carrega documento mínimo, roda 1 frame headless.
//     Também checa glintfx::version() contra o release atual ("0.2.4") — pega
//     tanto uma regressão para literal hardcoded defasado em app.cpp (era "0.1.0"
//     até a v0.2.4) quanto um project(VERSION) defasado em CMakeLists.txt.
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstring>
int main() {
  if (std::strcmp(glintfx::version(), "0.2.4") != 0) {
    std::fprintf(stderr, "app smoke FAIL: version()=\"%s\" esperado \"0.2.4\"\n",
                 glintfx::version());
    return 1;
  }
  glintfx::AppConfig cfg; cfg.title = "app"; cfg.width = 320; cfg.height = 240;
  glintfx::App app(cfg);
  app.load("tests/min.rml");
  app.poll_events(); app.update(); app.render();   // 1 frame, sem crash
  std::puts("app smoke OK");
  return 0;
}
