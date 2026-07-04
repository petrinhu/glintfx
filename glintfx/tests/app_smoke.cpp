// SPDX-License-Identifier: MPL-2.0
// EN: Smoke test — open App, load a minimal document, run 1 frame headless.
//     Also checks glintfx::version() against GLINTFX_VERSION (config.hpp, generated
//     from the CMake project(VERSION) at configure time — single source of truth,
//     see CMakeLists.txt and include/glintfx/config.hpp.in). Comparing against the
//     live macro instead of a hardcoded literal means this test tracks every release
//     bump automatically instead of masking drift (a hardcoded literal here previously
//     went stale across a release and silently passed against an un-bumped
//     project(VERSION) — see TODO.md v0.3.0 fix). Same pattern as
//     tests/ui_layer_attach.cpp's version() check (L1.9-VERSEMBED).
// PT: Teste de fumaça — abre App, carrega documento mínimo, roda 1 frame headless.
//     Também checa glintfx::version() contra GLINTFX_VERSION (config.hpp, gerado do
//     project(VERSION) do CMake em tempo de configure — fonte única de verdade, ver
//     CMakeLists.txt e include/glintfx/config.hpp.in). Comparar com a macro viva em
//     vez de um literal hardcoded faz este teste acompanhar todo bump de release
//     automaticamente em vez de mascarar drift (um literal hardcoded aqui ficou
//     defasado numa release anterior e passou silenciosamente contra um
//     project(VERSION) não bumpado — ver TODO.md fix v0.3.0). Mesmo padrão do
//     check de version() em tests/ui_layer_attach.cpp (L1.9-VERSEMBED).
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstring>
int main() {
  if (std::strcmp(glintfx::version(), GLINTFX_VERSION) != 0) {
    std::fprintf(stderr, "app smoke FAIL: version()=\"%s\" esperado \"%s\"\n",
                 glintfx::version(), GLINTFX_VERSION);
    return 1;
  }
  glintfx::AppConfig cfg; cfg.title = "app"; cfg.width = 320; cfg.height = 240;
  glintfx::App app(cfg);
  app.load("tests/min.rml");
  app.poll_events(); app.update(); app.render();   // 1 frame, sem crash
  std::puts("app smoke OK");
  return 0;
}
