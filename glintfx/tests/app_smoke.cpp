// SPDX-License-Identifier: Apache-2.0
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

// EN: Shape check for the SemVer build-metadata suffix (spec §10, https://semver.org/):
//     "MAJOR.MINOR.PATCH.TWEAK+sha[.dirty]" -- 4 dot-separated numeric groups, a literal
//     '+', then either "unknown" (no git) or a short hex commit SHA optionally followed by
//     ".dirty". Deliberately format-only (never asserts a literal SHA/version, so it does not
//     go stale on a bump) -- same "track drift instead of masking it" reasoning as the
//     GLINTFX_VERSION comparison below.
// PT: Checagem de formato do sufixo de build-metadata do SemVer (spec §10,
//     https://semver.org/): "MAJOR.MINOR.PATCH.TWEAK+sha[.dirty]" -- 4 grupos numéricos
//     separados por ponto, um '+' literal, depois "unknown" (sem git) ou um SHA hex curto de
//     commit, opcionalmente seguido de ".dirty". Deliberadamente só-formato (nunca afirma um
//     SHA/versão literal, então não fica defasado num bump) -- mesmo raciocínio de "acompanhar
//     drift em vez de mascarar" da comparação com GLINTFX_VERSION abaixo.
static bool is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static bool version_has_build_metadata_shape(const char* v) {
  const char* plus = std::strchr(v, '+');
  if (!plus) return false;
  int dots = 0;
  for (const char* p = v; p < plus; ++p) {
    if (*p == '.') {
      ++dots;
      continue;
    }
    if (*p < '0' || *p > '9') return false;
  }
  if (dots != 3) return false;
  const char* meta = plus + 1;
  if (std::strcmp(meta, "unknown") == 0) return true;
  int hex_count = 0;
  const char* p = meta;
  while (*p && is_hex_digit(*p)) {
    ++hex_count;
    ++p;
  }
  if (hex_count < 4) return false;
  if (*p == '\0') return true;
  return std::strcmp(p, ".dirty") == 0;
}

int main() {
  if (std::strcmp(glintfx::version(), GLINTFX_VERSION) != 0) {
    std::fprintf(stderr, "app smoke FAIL: version()=\"%s\" esperado \"%s\"\n",
                 glintfx::version(), GLINTFX_VERSION);
    return 1;
  }
  if (!version_has_build_metadata_shape(glintfx::version())) {
    std::fprintf(stderr,
                 "app smoke FAIL: version() \"%s\" nao tem o formato "
                 "MAJOR.MINOR.PATCH.TWEAK+sha[.dirty]\n",
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
