// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Gamepads' device-layer scan (A2-GAMEPAD, framework-2D, Agent B)
//     -- no GL, no window, no RmlUi, no Xvfb, no real `/dev/input` device (see
//     GamepadsConfig::dev_dir's own doc-comment): every case below points `dev_dir` at a
//     temporary directory this test builds and tears down itself, so this suite is fully
//     headless and never touches the host's real input devices. Covers section 5.3's device
//     scenarios from the A2-GAMEPAD plan: (a) empty dir, (b) nonexistent dir, (c) a regular
//     garbage file named like an evdev node, (d) a permission-denied node (EVIOCGBIT/open
//     failure paths, D7's "graceful skip" discipline) -- plus a shutdown()+re-init() cycle.
// PT: Teste unit puro pra varredura da camada de device do glintfx::Gamepads (A2-GAMEPAD,
//     framework-2D, Agente B) -- sem GL, sem janela, sem RmlUi, sem Xvfb, sem device real de
//     `/dev/input` (ver o próprio doc-comment de GamepadsConfig::dev_dir): todo caso abaixo
//     aponta `dev_dir` pra um diretório temporário que este teste constrói e desmonta por conta
//     própria, então esta suíte é inteiramente headless e nunca toca nos devices de input reais
//     do host. Cobre os cenários de device da seção 5.3 do plano A2-GAMEPAD: (a) dir vazio, (b)
//     dir inexistente, (c) um arquivo-lixo regular com nome de node evdev, (d) um node com
//     permissão negada (caminhos de falha de EVIOCGBIT/open, disciplina de "pula gracioso" do
//     D7) -- mais um ciclo shutdown()+re-init().
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/gamepad.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <unistd.h>

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
  // EN: A handful of bytes that are emphatically NOT a valid evdev ioctl response -- this file
  //     is opened successfully (it is a regular file), and `EVIOCGBIT` on it must fail with
  //     ENOTTY (a regular file is not a character device), which is exactly the "not an evdev
  //     node" skip path this test exercises.
  // PT: Um punhado de bytes que categoricamente NÃO é uma resposta válida de ioctl evdev -- este
  //     arquivo é aberto com sucesso (é um arquivo regular), e `EVIOCGBIT` nele precisa falhar
  //     com ENOTTY (um arquivo regular não é um character device), que é exatamente o caminho de
  //     skip "não é um node evdev" que este teste exercita.
  f << "not an evdev node, just garbage bytes\n";
}

} // namespace

using namespace glintfx;

int main() {
  namespace fs = std::filesystem;
  const fs::path base = fs::temp_directory_path() / "glintfx-gamepad-device-sanity";
  std::error_code ec;
  fs::remove_all(base, ec); // EN: clean slate from a possible prior crashed run. PT: estado limpo de uma execução anterior que possa ter crashado.
  fs::create_directories(base, ec);
  check(!ec, "test setup: base temp directory created");

  struct DirGuard {
    fs::path path;
    ~DirGuard() {
      std::error_code cleanup_ec;
      // EN: chmod the garbage/permission-denied fixtures back to something removable, in case
      //     this process is not root and the 000-mode file would otherwise survive the removal.
      // PT: chmod dos fixtures de lixo/permissão-negada de volta pra algo removível, caso este
      //     processo não seja root e o arquivo em modo 000 sobreviveria à remoção de outra forma.
      std::error_code chmod_ec;
      fs::permissions(path, fs::perms::owner_all, chmod_ec);
      fs::remove_all(path, cleanup_ec);
    }
  } dir_guard{base};

  const bool is_root = (::geteuid() == 0);

  // ---------------------------------------------------------------------------
  // EN: (a) Empty directory -- init() must succeed with zero pads.
  // PT: (a) Diretório vazio -- init() precisa ter sucesso com zero pads.
  // ---------------------------------------------------------------------------
  {
    const fs::path dir = base / "empty";
    fs::create_directories(dir, ec);

    Gamepads       pads;
    GamepadsConfig cfg;
    const std::string dir_str = dir.string();
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false; // EN: this sub-case is scan-only. PT: este sub-caso é só varredura.

    check(pads.init(cfg), "(a) empty dir: init() returns true");
    check(pads.is_initialized(), "(a) empty dir: is_initialized() true after init()");
    check(pads.connected_count() == 0, "(a) empty dir: connected_count() == 0");
    pads.poll(); // EN: must not crash even with zero pads and hotplug off. PT: não pode crashar mesmo com zero pads e hotplug desligado.
    pads.shutdown();
    check(!pads.is_initialized(), "(a) empty dir: is_initialized() false after shutdown()");
  }

  // ---------------------------------------------------------------------------
  // EN: (b) Nonexistent directory -- init() must still succeed (opendir() failure is a
  //     "0 pads found" outcome, not an init() failure -- see gamepad.hpp's header comment).
  // PT: (b) Diretório inexistente -- init() ainda precisa ter sucesso (falha do opendir() é um
  //     resultado de "0 pads encontrados", não uma falha de init() -- ver o comentário de
  //     cabeçalho do gamepad.hpp).
  // ---------------------------------------------------------------------------
  {
    const fs::path dir = base / "does-not-exist";

    Gamepads       pads;
    GamepadsConfig cfg;
    const std::string dir_str = dir.string();
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;

    check(pads.init(cfg), "(b) nonexistent dir: init() returns true");
    check(pads.connected_count() == 0, "(b) nonexistent dir: connected_count() == 0");
    pads.poll();
    pads.shutdown();
  }

  // ---------------------------------------------------------------------------
  // EN: (c) A regular file (garbage content) named like an evdev node -- open() succeeds (it IS
  //     a readable regular file), EVIOCGBIT must fail (ENOTTY), and the scan skips it gracefully.
  // PT: (c) Um arquivo regular (conteúdo lixo) com nome de node evdev -- open() tem sucesso (É
  //     um arquivo regular legível), EVIOCGBIT precisa falhar (ENOTTY), e a varredura o pula
  //     graciosamente.
  // ---------------------------------------------------------------------------
  {
    const fs::path dir = base / "garbage-node";
    fs::create_directories(dir, ec);
    write_garbage_file(dir / "event0");

    Gamepads       pads;
    GamepadsConfig cfg;
    const std::string dir_str = dir.string();
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;

    check(pads.init(cfg), "(c) garbage regular file 'event0': init() returns true");
    check(pads.connected_count() == 0, "(c) garbage regular file 'event0': connected_count() == 0 (skipped, ENOTTY)");
    pads.shutdown();
  }

  // ---------------------------------------------------------------------------
  // EN: (d) A node with chmod 000 -- open(O_RDONLY) must fail EACCES, skipped gracefully, no
  //     root required, no crash. Skipped as root (root bypasses DAC permission bits, so this
  //     sub-case cannot exercise the EACCES path there -- documented, not a test gap).
  // PT: (d) Um node com chmod 000 -- open(O_RDONLY) precisa falhar EACCES, pulado graciosamente,
  //     sem exigir root, sem crash. Pulado como root (root contorna os bits de permissão DAC,
  //     então este sub-caso não consegue exercitar o caminho EACCES ali -- documentado, não é
  //     uma lacuna de teste).
  // ---------------------------------------------------------------------------
  if (!is_root) {
    const fs::path dir = base / "denied-node";
    fs::create_directories(dir, ec);
    write_garbage_file(dir / "event1");
    fs::permissions(dir / "event1", fs::perms::none, ec);
    check(!ec, "(d) test setup: chmod 000 on 'event1' applied");

    Gamepads       pads;
    GamepadsConfig cfg;
    const std::string dir_str = dir.string();
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;

    check(pads.init(cfg), "(d) permission-denied node: init() returns true");
    check(pads.connected_count() == 0, "(d) permission-denied node: connected_count() == 0 (skipped, EACCES)");
    pads.shutdown();
  } else {
    std::fprintf(stderr, "gamepad_device_sanity: (d) EACCES sub-case skipped (running as root)\n");
  }

  // ---------------------------------------------------------------------------
  // EN: shutdown() + re-init() cycle -- fully supported, same discipline as Audio.
  // PT: Ciclo shutdown() + re-init() -- totalmente suportado, mesma disciplina do Audio.
  // ---------------------------------------------------------------------------
  {
    const fs::path dir = base / "reinit";
    fs::create_directories(dir, ec);

    Gamepads       pads;
    GamepadsConfig cfg;
    const std::string dir_str = dir.string();
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;

    check(pads.init(cfg), "reinit cycle: first init() returns true");
    check(!pads.init(cfg), "reinit cycle: second init() (without shutdown) returns false");
    pads.shutdown();
    check(pads.init(cfg), "reinit cycle: init() after shutdown() returns true again");
    pads.shutdown();
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_device_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_device_sanity: PASS");
  return 0;
}
