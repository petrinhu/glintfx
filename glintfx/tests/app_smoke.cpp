// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — open App, load a minimal document, run 1 frame headless.
// PT: Teste de fumaça — abre App, carrega documento mínimo, roda 1 frame headless.
#include <glintfx/glintfx.hpp>
#include <cstdio>
int main() {
  glintfx::AppConfig cfg; cfg.title = "app"; cfg.width = 320; cfg.height = 240;
  glintfx::App app(cfg);
  app.load("tests/min.rml");
  app.poll_events(); app.update(); app.render();   // 1 frame, sem crash
  std::puts("app smoke OK");
  return 0;
}
