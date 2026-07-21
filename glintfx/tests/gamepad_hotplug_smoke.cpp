// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Gamepads' inotify-based hotplug path (A2-GAMEPAD, framework-2D,
//     Agent B, D2/D3 of the plan) -- no GL, no window, no RmlUi, no Xvfb, no real gamepad device.
//     `dev_dir` points at a temporary directory this test creates/deletes `eventN`-named files
//     in, between poll() calls, with hotplug=true. Every file here is garbage (not a real evdev
//     node), so `try_register()`'s own capability check (EVIOCGBIT failing ENOTTY, same path
//     gamepad_device_sanity.cpp's case (c) exercises for the INITIAL scan) always rejects it --
//     what THIS file proves is that the inotify code path itself (IN_CREATE/IN_ATTRIB/IN_DELETE
//     drained and reacted to by poll()) runs cleanly under file churn: no crash, no phantom pad
//     ever appears (connected_count() stays 0 throughout, since nothing here is ever a real
//     gamepad), no leak of the create/delete cycle across repeated poll() calls.
// PT: Teste unit puro pro caminho de hotplug via inotify do glintfx::Gamepads (A2-GAMEPAD,
//     framework-2D, Agente B, D2/D3 do plano) -- sem GL, sem janela, sem RmlUi, sem Xvfb, sem
//     device de gamepad real. `dev_dir` aponta pra um diretório temporário no qual este teste
//     cria/apaga arquivos nomeados `eventN`, entre chamadas a poll(), com hotplug=true. Todo
//     arquivo aqui é lixo (não é um node evdev real), então a própria checagem de capacidade do
//     `try_register()` (EVIOCGBIT falhando ENOTTY, mesmo caminho que o caso (c) do
//     gamepad_device_sanity.cpp exercita pra varredura INICIAL) sempre rejeita -- o que ESTE
//     arquivo prova é que o próprio caminho de código do inotify (IN_CREATE/IN_ATTRIB/IN_DELETE
//     drenados e reagidos pelo poll()) roda limpo sob a agitação de arquivos: sem crash, nenhum
//     pad fantasma jamais aparece (connected_count() fica em 0 o tempo todo, já que nada aqui
//     jamais é um gamepad de verdade), sem vazamento do ciclo criar/apagar através de chamadas
//     repetidas de poll().
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/gamepad.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void write_garbage_file(const std::filesystem::path& path) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f << "hotplug-smoke garbage, not a real evdev node\n";
}

} // namespace

using namespace glintfx;

int main() {
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "glintfx-gamepad-hotplug-smoke";
  std::error_code ec;
  fs::remove_all(dir, ec);
  fs::create_directories(dir, ec);
  check(!ec, "test setup: temp directory created");

  struct DirGuard {
    fs::path path;
    ~DirGuard() {
      std::error_code cleanup_ec;
      fs::remove_all(path, cleanup_ec);
    }
  } dir_guard{dir};

  Gamepads       pads;
  GamepadsConfig cfg;
  const std::string dir_str = dir.string();
  cfg.dev_dir = dir_str.c_str();
  cfg.hotplug = true;

  int connection_events = 0;
  pads.set_connection_callback([&connection_events](int /*pad*/, bool /*connected*/) {
    ++connection_events; // EN: must NEVER fire -- every file here is garbage, never a gamepad.
                          // PT: NUNCA pode disparar -- todo arquivo aqui é lixo, nunca um gamepad.
  });

  check(pads.init(cfg), "init(): returns true with an empty, hotplug-watched dir");
  check(pads.connected_count() == 0, "initial scan: connected_count() == 0 (dir started empty)");

  // EN: poll() once before any file churn -- must be a clean no-op (no inotify events pending
  //     yet).
  // PT: poll() uma vez antes de qualquer agitação de arquivo -- precisa ser um no-op limpo
  //     (nenhum evento de inotify pendente ainda).
  pads.poll();
  check(pads.connected_count() == 0, "poll() before churn: still 0 connected");

  // EN: Create/delete cycles, interleaved with poll() -- exercises IN_CREATE, IN_ATTRIB
  //     (chmod after create), and IN_DELETE in turn.
  // PT: Ciclos de criar/apagar, intercalados com poll() -- exercita IN_CREATE, IN_ATTRIB (chmod
  //     depois de criar), e IN_DELETE em sequência.
  for (int i = 0; i < 5; ++i) {
    const fs::path node = dir / ("event" + std::to_string(i));

    write_garbage_file(node);
    pads.poll(); // EN: reacts to IN_CREATE -- rejected by capability check (ENOTTY), no crash.
                 // PT: reage ao IN_CREATE -- rejeitado pela checagem de capacidade (ENOTTY), sem crash.
    check(pads.connected_count() == 0, "after create: connected_count() == 0 (garbage, rejected)");

    fs::permissions(node, fs::perms::owner_read | fs::perms::owner_write, ec);
    pads.poll(); // EN: reacts to IN_ATTRIB -- same rejection, no crash, no phantom pad.
                 // PT: reage ao IN_ATTRIB -- mesma rejeição, sem crash, sem pad fantasma.
    check(pads.connected_count() == 0, "after chmod (IN_ATTRIB): connected_count() == 0");

    fs::remove(node, ec);
    pads.poll(); // EN: reacts to IN_DELETE -- nothing was connected under this name, so this is
                 //     a clean no-op (no disconnect callback fires, checked below).
                 // PT: reage ao IN_DELETE -- nada estava conectado sob este nome, então isto é
                 //     um no-op limpo (nenhum callback de disconnect dispara, checado abaixo).
    check(pads.connected_count() == 0, "after delete: connected_count() == 0");
  }

  check(connection_events == 0, "connection callback never fired (no file here was ever a real gamepad)");

  pads.shutdown();
  check(!pads.is_initialized(), "shutdown(): is_initialized() false");

  // EN: File churn AFTER shutdown() (inotify fd closed) must not crash a subsequent re-init().
  // PT: Agitação de arquivo DEPOIS do shutdown() (fd de inotify fechado) não pode crashar um
  //     re-init() subsequente.
  write_garbage_file(dir / "event99");
  check(pads.init(cfg), "re-init() after shutdown() and post-shutdown file churn: returns true");
  check(pads.connected_count() == 0, "re-init(): connected_count() == 0 (event99 is garbage too)");
  pads.shutdown();

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_hotplug_smoke: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_hotplug_smoke: PASS");
  return 0;
}
