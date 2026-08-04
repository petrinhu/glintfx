// SPDX-License-Identifier: Apache-2.0
// EN: log_winglfw_sink_sanity -- regression gate for `LOG-WINGLFW` (W25, seeded by the FW-LOG
//     implementer 2026-07-29, see TODO.md). FW-LOG (76519bb) routed glintfx's 3 biggest
//     diagnostic funnels (draw2d.cpp's `Impl::log_warn`, system_clock.hpp's/
//     system_glfw_dedup.hpp's `LogMessage` overrides) through `glintfx::log()`/`log_warn()`, but
//     left `glintfx/src/window_glfw.cpp` on raw `std::fprintf(stderr, ...)` -- a consumer
//     installing `glintfx::set_log_sink()` had zero visibility into those 9 window/icon
//     diagnostics; this test proves that gap is closed for at least one of them.
//
//     WHY THIS ONE SITE: `WindowGlfw::set_window_icon()`'s pre-create guard
//     (`win_ == nullptr` -- window_glfw.cpp) is the ONLY one of the 9 converted call sites
//     reachable WITHOUT a real GLFW window/display -- no `create()`, no `glfwInit()`, no Xvfb.
//     The other 8 sites either need a successfully created window (the remaining 3
//     `set_window_icon` guards: null pixels, non-positive size, over-cap size) or a headless
//     environment that reports NO primary monitor/video mode (the 4 `create()`/`set_mode()`
//     fallback-to-windowed sites), which Xvfb does not naturally produce -- see
//     app_vsync_sanity.cpp/app_window_icon_sanity.cpp's own "headless honesty" precedent for why
//     this suite does not fake coverage of paths it cannot actually exercise. This test therefore
//     runs as a PLAIN `add_test` (no `run_xvfb.cmake` wrapper), same "no window, no GL context"
//     shape as log_sanity.cpp.
//
//     VERIFICATION: installs a custom `glintfx::LogSink`, constructs a default (never-`create()`d)
//     `WindowGlfw`, calls `set_window_icon()` on it, and asserts (a) the call still returns
//     `false` (the pre-existing D5 fail-high contract, unchanged by this routing), (b) exactly one
//     message reached the sink instead of stderr, (c) it carries `LogLevel::Warn` (matching every
//     other window/icon diagnostic in this cluster -- none of the 9 are fatal-to-the-process, all
//     are graceful-fallback/rejected-input), and (d) the message TEXT is preserved verbatim (the
//     conversion is a channel swap, not a wording change -- see TODO.md's own "preserve o texto"
//     instruction for this slice).
// PT: log_winglfw_sink_sanity -- gate de regressão para `LOG-WINGLFW` (W25, semeado pelo
//     implementer do FW-LOG em 2026-07-29, ver TODO.md). O FW-LOG (76519bb) roteou os 3 maiores
//     funis de diagnóstico da glintfx (`Impl::log_warn` do draw2d.cpp, os overrides `LogMessage`
//     de system_clock.hpp/system_glfw_dedup.hpp) por `glintfx::log()`/`log_warn()`, mas deixou
//     `glintfx/src/window_glfw.cpp` no `std::fprintf(stderr, ...)` cru -- um consumidor que
//     instala `glintfx::set_log_sink()` não tinha visibilidade nenhuma sobre esses 9 diagnósticos
//     de janela/ícone; este teste prova que a lacuna está fechada para pelo menos um deles.
//
//     POR QUE ESTE SÍTIO: a guarda pré-create de `WindowGlfw::set_window_icon()`
//     (`win_ == nullptr` -- window_glfw.cpp) é o ÚNICO dos 9 sítios convertidos alcançável SEM uma
//     janela/display GLFW real -- sem `create()`, sem `glfwInit()`, sem Xvfb. Os outros 8 sítios
//     precisam ou de uma janela criada com sucesso (as 3 guardas restantes de `set_window_icon`:
//     pixels nulos, tamanho não-positivo, tamanho acima do teto) ou de um ambiente headless que
//     reporte NENHUM monitor primário/video mode (os 4 sítios de fallback-pra-windowed de
//     `create()`/`set_mode()`), o que o Xvfb não produz naturalmente -- ver o precedente de
//     "honestidade headless" do próprio app_vsync_sanity.cpp/app_window_icon_sanity.cpp pro
//     motivo desta suíte não forjar cobertura de caminhos que não consegue de fato exercitar. Este
//     teste portanto roda como um `add_test` PLANO (sem o wrapper `run_xvfb.cmake`), mesma forma
//     "sem janela, sem contexto GL" do log_sanity.cpp.
//
//     VERIFICAÇÃO: instala um `glintfx::LogSink` customizado, constrói um `WindowGlfw` default
//     (nunca `create()`ado), chama `set_window_icon()` nele, e verifica (a) a chamada ainda
//     retorna `false` (o contrato fail-high D5 pré-existente, inalterado por este roteamento),
//     (b) exatamente uma mensagem chegou ao sink em vez do stderr, (c) ela carrega
//     `LogLevel::Warn` (batendo com todo outro diagnóstico de janela/ícone deste cluster -- nenhum
//     dos 9 é fatal-pro-processo, todos são fallback-gracioso/input-rejeitado), e (d) o TEXTO da
//     mensagem é preservado verbatim (a conversão é uma troca de canal, não uma mudança de
//     redação -- ver a própria instrução "preserve o texto" desta fatia no TODO.md).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/log.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

struct Captured {
  glintfx::LogLevel level;
  std::string message;
};

// EN: A tiny, well-formed 2x2 RGBA8 buffer -- content is irrelevant here (the guard under test
//     rejects BEFORE ever touching pixel data, `win_ == nullptr`), but a real-shaped buffer keeps
//     this test symmetric with app_window_icon_sanity.cpp's own kIcon2x2.
// PT: Um buffer RGBA8 2x2 pequeno e bem-formado -- o conteúdo é irrelevante aqui (a guarda sob
//     teste rejeita ANTES de sequer tocar o dado de pixel, `win_ == nullptr`), mas um buffer com
//     forma real mantém este teste simétrico ao próprio kIcon2x2 de app_window_icon_sanity.cpp.
constexpr unsigned char kIcon2x2[2 * 2 * 4] = {
    255,
    0,
    0,
    255,
    0,
    255,
    0,
    255,
    0,
    0,
    255,
    255,
    255,
    255,
    255,
    128,
};

} // namespace

int main() {
  std::vector<Captured> got;
  glintfx::set_log_sink([&](glintfx::LogLevel level, const char* message) {
    got.push_back({level, message});
  });

  glintfx::WindowGlfw w; // EN: never create()d. PT: nunca create()ado.
  const bool accepted = w.set_window_icon(kIcon2x2, 2, 2);

  check(!accepted,
        "set_window_icon() on a pre-create WindowGlfw still returns false (D5 unchanged)");
  check(got.size() == 1,
        "LOG-WINGLFW: the pre-create diagnostic reached the sink instead of raw stderr");
  if (got.size() == 1) {
    check(got[0].level == glintfx::LogLevel::Warn,
          "LOG-WINGLFW: routed at LogLevel::Warn, matching this cluster's other 8 diagnostics");
    check(got[0].message.find("set_window_icon") != std::string::npos &&
              got[0].message.find("before a successful create()") != std::string::npos,
          "LOG-WINGLFW: message text preserved verbatim (channel swap, not a wording change)");
  }

  // EN: Good citizenship for any test process running after this one in the same ctest
  //     invocation (same convention log_sanity.cpp ends on).
  // PT: Boa cidadania para qualquer processo de teste rodando depois deste na mesma invocação de
  //     ctest (mesma convenção com que log_sanity.cpp termina).
  glintfx::set_log_sink(nullptr);

  if (g_failures > 0) {
    std::fprintf(stderr, "log_winglfw_sink_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("log_winglfw_sink_sanity: PASS");
  return 0;
}
