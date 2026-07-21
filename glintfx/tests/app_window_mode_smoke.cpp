// SPDX-License-Identifier: MPL-2.0
// EN: App window-mode creation smoke (A4-WINMODES, v0.16.0). Proves each WindowMode is
//     accepted at App construction time without a crash, the resulting viewport is sane, and
//     the LIVE App::window_mode() query matches what was requested -- with ONE documented
//     exception (see below).
//
//     ONE BINARY, PARAMETERIZED BY ENV VAR (App's N3 contract: only ONE App instance per
//     process -- see app.hpp -- so 4+1 modes cannot live in a single process/main(); this file
//     is registered 5x in tests/CMakeLists.txt via set_tests_properties(... ENVIRONMENT
//     GLINTFX_TEST_WINDOW_MODE=...), same env-var-parameterization idiom the plan mandates):
//       windowed             -> AppConfig::window_mode = Windowed
//       maximized            -> AppConfig::window_mode = Maximized (TOLERANT assertion below)
//       fullscreen_desktop   -> AppConfig::window_mode = FullscreenDesktop
//       fullscreen_exclusive -> AppConfig::window_mode = FullscreenExclusive
//       invalid_config       -> AppConfig::window_mode = static_cast<WindowMode>(255) (D5
//                                fallback-to-Windowed path, exercised through the PUBLIC
//                                AppConfig surface -- App::App()'s ctor-level validation, not
//                                WindowGlfw::create()'s own internal twin guard)
//
//     THE ONE TOLERANT CASE (D4, documented not hidden): under Xvfb there is NO window
//     manager, so a Maximized request may not be honoured at all -- glfwWindowHint(
//     GLFW_MAXIMIZED, GLFW_TRUE) is a REQUEST, the WM applies it. window_mode()'s live
//     derivation (glfwGetWindowAttrib(win, GLFW_MAXIMIZED)) can therefore legitimately read
//     back either Maximized (WM honoured it) or Windowed (no WM to honour it) here -- both are
//     accepted. The REAL "maximize respects the taskbar work area" claim (the GusWorld fix
//     this whole slice exists for) is NOT machine-verifiable under Xvfb (no panel to invade in
//     the first place) and is proven instead by the manual smoke on a live KDE/GNOME session --
//     see docs/window-modes.md's "verificação manual" section and the review's smoke-manual
//     step. FullscreenDesktop/FullscreenExclusive do NOT depend on a WM (a monitor + its video
//     mode is enough) so those two assert the EXACT requested mode, no tolerance.
//
//     VIEWPORT-SANITY ORACLE: snapshot()'s own PPM header (P6\n<w> <h>\n255\n) is written from
//     the SAME w/h App::snapshot() just glReadPixels'd with, which is the SAME w/h
//     Impl::sync_viewport() (AUD-PUB-1) just told RmlUi's Context about -- so "PPM header dims
//     == get_window_size()" is a cheap, deterministic proof that the whole render/viewport
//     pipeline is internally consistent for THIS window mode, independent of what the WM did
//     with the window on screen.
//
// PT: Smoke de criação de modo de janela do App (A4-WINMODES, v0.16.0). Prova que cada
//     WindowMode é aceito na construção do App sem crash, o viewport resultante é são, e a
//     query VIVA App::window_mode() bate com o que foi pedido -- com UMA exceção documentada
//     (ver abaixo).
//
//     UM BINÁRIO, PARAMETRIZADO POR ENV VAR (contrato N3 do App: só UMA instância de App por
//     processo -- ver app.hpp -- então 4+1 modos não cabem num único processo/main(); este
//     arquivo é registrado 5x em tests/CMakeLists.txt via set_tests_properties(...
//     ENVIRONMENT GLINTFX_TEST_WINDOW_MODE=...), o mesmo idioma de parametrização-por-env-var
//     que o plano exige):
//       windowed             -> AppConfig::window_mode = Windowed
//       maximized            -> AppConfig::window_mode = Maximized (asserção TOLERANTE abaixo)
//       fullscreen_desktop   -> AppConfig::window_mode = FullscreenDesktop
//       fullscreen_exclusive -> AppConfig::window_mode = FullscreenExclusive
//       invalid_config       -> AppConfig::window_mode = static_cast<WindowMode>(255) (caminho
//                                de fallback-para-Windowed do D5, exercitado pela superfície
//                                PÚBLICA AppConfig -- a validação em nível de ctor do
//                                App::App(), não a guarda gêmea interna do próprio
//                                WindowGlfw::create())
//
//     O ÚNICO CASO TOLERANTE (D4, documentado, não escondido): sob Xvfb não há window manager
//     nenhum, então um pedido Maximized pode simplesmente não ser honrado --
//     glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE) é um PEDIDO, o WM que aplica. A derivação viva
//     de window_mode() (glfwGetWindowAttrib(win, GLFW_MAXIMIZED)) pode portanto legitimamente
//     ler de volta Maximized (o WM honrou) OU Windowed (sem WM pra honrar) aqui -- ambos são
//     aceitos. A alegação REAL "maximizar respeita a work area da barra de tarefas" (o fix do
//     GusWorld que esta fatia inteira existe para resolver) NÃO é verificável por máquina sob
//     Xvfb (não há painel pra invadir, pra começar) e é provada em vez disso pelo smoke manual
//     numa sessão KDE/GNOME real -- ver a seção "verificação manual" de docs/window-modes.md e
//     o passo de smoke manual do review. FullscreenDesktop/FullscreenExclusive NÃO dependem de
//     um WM (um monitor + seu video mode bastam) então esses dois exigem o modo pedido EXATO,
//     sem tolerância.
//
//     ORÁCULO DE SANIDADE DE VIEWPORT: o próprio header PPM do snapshot() (P6\n<w> <h>\n255\n)
//     é escrito com o mesmo w/h que App::snapshot() acabou de ler via glReadPixels, que é o
//     mesmo w/h que Impl::sync_viewport() (AUD-PUB-1) acabou de informar ao Context do RmlUi --
//     então "dims do header PPM == get_window_size()" é uma prova barata e determinística de
//     que o pipeline inteiro de render/viewport está internamente consistente para ESTE modo
//     de janela, independente do que o WM fez com a janela na tela.
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// EN: Minimal P6 PPM header parser -- reads only the "P6\n<w> <h>\n255\n" prologue
//     App::snapshot() writes (app.cpp), does not touch the pixel payload.
// PT: Parser mínimo de cabeçalho P6 PPM -- lê só o prólogo "P6\n<w> <h>\n255\n" que
//     App::snapshot() escreve (app.cpp), não toca no payload de pixels.
bool read_ppm_dims(const char* path, int& w, int& h) {
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  char magic[3] = {};
  const int got = std::fscanf(f, "%2s %d %d", magic, &w, &h);
  std::fclose(f);
  return got == 3 && std::strcmp(magic, "P6") == 0;
}

} // namespace

int main() {
  const char* mode_env = std::getenv("GLINTFX_TEST_WINDOW_MODE");
  if (!mode_env) {
    std::puts("FAIL: GLINTFX_TEST_WINDOW_MODE not set (registered via ctest ENVIRONMENT)");
    return 1;
  }

  glintfx::AppConfig cfg;
  cfg.title  = "app_window_mode_smoke";
  cfg.width  = 400;
  cfg.height = 300;

  glintfx::WindowMode expected      = glintfx::WindowMode::Windowed;
  bool                tolerant_max  = false;

  if (std::strcmp(mode_env, "windowed") == 0) {
    cfg.window_mode = glintfx::WindowMode::Windowed;
    expected         = glintfx::WindowMode::Windowed;
  } else if (std::strcmp(mode_env, "maximized") == 0) {
    cfg.window_mode = glintfx::WindowMode::Maximized;
    tolerant_max     = true;  // EN: see file header. PT: ver cabeçalho do arquivo.
  } else if (std::strcmp(mode_env, "fullscreen_desktop") == 0) {
    cfg.window_mode = glintfx::WindowMode::FullscreenDesktop;
    expected         = glintfx::WindowMode::FullscreenDesktop;
  } else if (std::strcmp(mode_env, "fullscreen_exclusive") == 0) {
    cfg.window_mode = glintfx::WindowMode::FullscreenExclusive;
    expected         = glintfx::WindowMode::FullscreenExclusive;
  } else if (std::strcmp(mode_env, "invalid_config") == 0) {
    // EN: D5 -- enum lixo (out-of-range static_cast) via the PUBLIC AppConfig surface. Must
    //     fall back to Windowed, decided ONCE inside App::App() (see app.cpp).
    // PT: D5 -- enum lixo (static_cast fora de faixa) pela superfície PÚBLICA AppConfig.
    //     Precisa cair pra Windowed, decidido UMA VEZ dentro de App::App() (ver app.cpp).
    cfg.window_mode = static_cast<glintfx::WindowMode>(255);
    expected         = glintfx::WindowMode::Windowed;
  } else {
    std::printf("FAIL: unknown GLINTFX_TEST_WINDOW_MODE=\"%s\"\n", mode_env);
    return 2;
  }

  glintfx::App app(cfg);
  if (!app.ok()) { std::puts("FAIL: app ok() false"); return 3; }

  // EN: Shared minimal scene, WORKING_DIRECTORY = CMAKE_BINARY_DIR (repo-wide GLFW-block
  //     convention); min.rml is copied unconditionally at the top of tests/CMakeLists.txt.
  // PT: Cena mínima compartilhada, WORKING_DIRECTORY = CMAKE_BINARY_DIR (convenção do bloco
  //     GLFW em todo o repo); min.rml é copiado incondicionalmente no topo do
  //     tests/CMakeLists.txt.
  if (!app.load("tests/min.rml")) { std::puts("FAIL: load"); return 4; }

  // EN: Two frames -- poll_events() drains any pending window-system events (mode requests are
  //     asynchronous w.r.t. the WM), then update()+render() settles the layout, same two-frame
  //     shape app_resize_smoke.cpp relies on.
  // PT: Dois frames -- poll_events() drena eventuais eventos pendentes do sistema de janelas
  //     (pedidos de modo são assíncronos em relação ao WM), depois update()+render() assenta o
  //     layout, mesma forma de dois frames que app_resize_smoke.cpp usa.
  app.poll_events();
  app.update();
  app.render();
  app.poll_events();
  app.update();
  app.render();

  int w = 0, h = 0;
  app.get_window_size(w, h);
  if (w <= 0 || h <= 0) {
    std::printf("FAIL: get_window_size() non-positive %dx%d\n", w, h);
    return 5;
  }

  const char* kSnapshotPath = "app_window_mode_smoke_snapshot.ppm";
  if (!app.snapshot(kSnapshotPath)) { std::puts("FAIL: snapshot() returned false"); return 6; }

  int ppm_w = 0, ppm_h = 0;
  if (!read_ppm_dims(kSnapshotPath, ppm_w, ppm_h)) {
    std::puts("FAIL: could not parse PPM header");
    return 7;
  }
  if (ppm_w != w || ppm_h != h) {
    std::printf("FAIL: PPM header dims %dx%d != get_window_size() %dx%d (viewport pipeline "
                "inconsistent for this window mode)\n", ppm_w, ppm_h, w, h);
    return 8;
  }

  const glintfx::WindowMode live = app.window_mode();
  if (tolerant_max) {
    if (live != glintfx::WindowMode::Maximized && live != glintfx::WindowMode::Windowed) {
      std::printf("FAIL: maximized live window_mode() is neither Maximized nor Windowed (%d)\n",
                  static_cast<int>(live));
      return 9;
    }
    std::printf("app_window_mode_smoke[maximized]: live mode = %s "
                "(Xvfb has no WM -- see docs/window-modes.md manual smoke for the real claim)\n",
                live == glintfx::WindowMode::Maximized ? "Maximized" : "Windowed");
  } else if (live != expected) {
    std::printf("FAIL: live window_mode() = %d, expected %d\n",
                static_cast<int>(live), static_cast<int>(expected));
    return 9;
  }

  std::printf("app_window_mode_smoke[%s] OK (%dx%d)\n", mode_env, w, h);
  return 0;
}
