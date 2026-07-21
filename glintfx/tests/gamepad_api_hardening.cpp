// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Gamepads' fail-high API hardening (A2-GAMEPAD, framework-2D,
//     Agent B) -- no GL, no window, no RmlUi, no Xvfb, no real gamepad device. Exercises the
//     WHOLE public surface (gamepad.hpp) before init(), after shutdown(), and with hostile
//     pad/index arguments (negative, at-the-boundary, past-the-boundary): every one of those must
//     return false/0/0.0f/"" and MUST NOT crash -- same discipline as audio_hostile_sanity.cpp
//     and decorator_polygon.cpp's "reject the bad input, never abort the host" (auditoria-dominó:
//     the same robustness class applies to every module in this library, ADR-0015's own house
//     rule). Also covers load_mappings_file/text hostile inputs (nullptr, nonexistent path,
//     binary-garbage file), the "second init() without shutdown() fails" contract, and a
//     nullptr/empty connection callback being a safe no-op.
// PT: Teste unit puro pro endurecimento fail-high da API do glintfx::Gamepads (A2-GAMEPAD,
//     framework-2D, Agente B) -- sem GL, sem janela, sem RmlUi, sem Xvfb, sem device de gamepad
//     real. Exercita a superfície pública INTEIRA (gamepad.hpp) antes do init(), depois do
//     shutdown(), e com argumentos hostis de pad/index (negativo, na fronteira, além da
//     fronteira): cada um precisa retornar false/0/0.0f/"" e NÃO PODE crashar -- mesma disciplina
//     do audio_hostile_sanity.cpp e do "rejeita o input ruim, nunca aborta o host" do
//     decorator_polygon.cpp (auditoria-dominó: a mesma classe de robustez se aplica a todo módulo
//     desta biblioteca, regra da casa da própria ADR-0015). Cobre também inputs hostis de
//     load_mappings_file/text (nullptr, caminho inexistente, arquivo lixo binário), o contrato
//     "segundo init() sem shutdown() falha", e um callback de conexão nullptr/vazio sendo um
//     no-op seguro.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/gamepad.hpp>
#include <glintfx/gamepad_types.hpp>

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: The complete read-only surface, checked at a given `pad`/`index` combination -- every
//     accessor must return its documented "invalid" sentinel (false/0/0.0f/""), never crash.
// PT: A superfície completa só-leitura, checada numa combinação `pad`/`index` dada -- todo
//     acessor precisa retornar seu sentinela "inválido" documentado (false/0/0.0f/""), nunca
//     crashar.
void check_all_accessors_invalid(glintfx::Gamepads& pads, int pad, const char* phase) {
  using glintfx::GamepadAxis;
  using glintfx::GamepadButton;

  std::string label = std::string(phase) + " pad=" + std::to_string(pad);

  check(!pads.connected(pad), (label + ": connected() false").c_str());
  check(std::string(pads.name(pad)).empty(), (label + ": name() empty").c_str());
  check(std::string(pads.guid(pad)).empty(), (label + ": guid() empty").c_str());
  check(!pads.button(pad, GamepadButton::South), (label + ": button() false").c_str());
  check(pads.axis(pad, GamepadAxis::LeftX) == 0.0f, (label + ": axis() 0.0f").c_str());
  check(pads.raw_button_count(pad) == 0, (label + ": raw_button_count() 0").c_str());
  check(!pads.raw_button(pad, 0), (label + ": raw_button() false").c_str());
  check(pads.raw_button_code(pad, 0) == 0, (label + ": raw_button_code() 0").c_str());
  check(pads.raw_axis_count(pad) == 0, (label + ": raw_axis_count() 0").c_str());
  check(pads.raw_axis(pad, 0) == 0.0f, (label + ": raw_axis() 0.0f").c_str());
  check(pads.raw_axis_value(pad, 0) == 0, (label + ": raw_axis_value() 0").c_str());
  check(pads.raw_axis_code(pad, 0) == 0, (label + ": raw_axis_code() 0").c_str());
  // EN: Also probe a hostile negative index alongside pad=0/valid-range pad, to prove index
  //     validation is independent of pad validation.
  // PT: Também sonda um índice negativo hostil junto de pad=0/pad em faixa válida, pra provar
  //     que a validação de índice é independente da validação de pad.
  check(!pads.raw_button(pad, -1), (label + ": raw_button(index=-1) false").c_str());
  check(pads.raw_axis(pad, -1) == 0.0f, (label + ": raw_axis(index=-1) 0.0f").c_str());
}

} // namespace

using namespace glintfx;

int main() {
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "glintfx-gamepad-api-hardening";
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

  const std::string dir_str = dir.string();

  // ---------------------------------------------------------------------------
  // EN: BEFORE init() -- every accessor is invalid, at pad=0, at a negative pad, and at the
  //     kMaxPads boundary (both exactly kMaxPads and well past it).
  // PT: ANTES do init() -- todo acessor é inválido, no pad=0, num pad negativo, e na fronteira
  //     do kMaxPads (tanto exatamente kMaxPads quanto bem além dele).
  // ---------------------------------------------------------------------------
  {
    Gamepads pads;
    check(!pads.is_initialized(), "before init(): is_initialized() false");
    check(pads.connected_count() == 0, "before init(): connected_count() 0");

    check_all_accessors_invalid(pads, 0, "before-init");
    check_all_accessors_invalid(pads, -1, "before-init");
    check_all_accessors_invalid(pads, Gamepads::kMaxPads, "before-init");
    check_all_accessors_invalid(pads, Gamepads::kMaxPads + 1000, "before-init");
    check_all_accessors_invalid(pads, -1000, "before-init");

    check(pads.load_mappings_file(nullptr) == 0, "before init(): load_mappings_file(nullptr) == 0");
    check(pads.load_mappings_text(nullptr) == 0, "before init(): load_mappings_text(nullptr) == 0");
    check(pads.load_mappings_text("03000000c82d00001930000011010000,Test,a:b0,platform:Linux,") == 0,
          "before init(): load_mappings_text(well-formed) == 0 (not initialized)");

    pads.poll(); // EN: no-op before init(), must not crash. PT: no-op antes do init(), não pode crashar.

    // EN: A nullptr/empty callback must never be invoked and never crash to set. PT: um callback
    //     nullptr/vazio nunca pode ser invocado e nunca pode crashar ao ser setado.
    pads.set_connection_callback(nullptr);
    pads.poll();

    pads.shutdown(); // EN: shutdown() on a never-initialized instance is a safe no-op.
                      // PT: shutdown() numa instância nunca inicializada é um no-op seguro.
  }

  // ---------------------------------------------------------------------------
  // EN: AFTER init()+shutdown() -- same invalid-everywhere contract as before init().
  // PT: DEPOIS de init()+shutdown() -- mesmo contrato de inválido-em-todo-lugar de antes do
  //     init().
  // ---------------------------------------------------------------------------
  {
    Gamepads       pads;
    GamepadsConfig cfg;
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;
    check(pads.init(cfg), "second init() setup: init() returns true");
    pads.shutdown();

    check(!pads.is_initialized(), "after shutdown(): is_initialized() false");
    check_all_accessors_invalid(pads, 0, "after-shutdown");
    check_all_accessors_invalid(pads, -1, "after-shutdown");
    check_all_accessors_invalid(pads, Gamepads::kMaxPads, "after-shutdown");

    check(pads.load_mappings_file(nullptr) == 0, "after shutdown(): load_mappings_file(nullptr) == 0");
    check(pads.load_mappings_text(nullptr) == 0, "after shutdown(): load_mappings_text(nullptr) == 0");
    pads.poll(); // EN: no-op after shutdown(). PT: no-op depois do shutdown().
  }

  // ---------------------------------------------------------------------------
  // EN: Second init() without an intervening shutdown() -- must return false, and must NOT
  //     disturb the already-running instance's state.
  // PT: Segundo init() sem um shutdown() no meio -- precisa retornar false, e NÃO pode
  //     perturbar o estado da instância já rodando.
  // ---------------------------------------------------------------------------
  {
    Gamepads       pads;
    GamepadsConfig cfg;
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;
    check(pads.init(cfg), "double-init: first init() returns true");
    check(!pads.init(cfg), "double-init: second init() (already initialized) returns false");
    check(pads.is_initialized(), "double-init: still initialized after the rejected second init()");
    pads.shutdown();
  }

  // ---------------------------------------------------------------------------
  // EN: load_mappings_file() hostile inputs, on an initialized instance: nullptr, a nonexistent
  //     path, and a file full of random binary garbage -- 0 in every case, never a crash.
  // PT: Inputs hostis do load_mappings_file(), numa instância inicializada: nullptr, um caminho
  //     inexistente, e um arquivo cheio de lixo binário aleatório -- 0 em todo caso, nunca um
  //     crash.
  // ---------------------------------------------------------------------------
  {
    Gamepads       pads;
    GamepadsConfig cfg;
    cfg.dev_dir = dir_str.c_str();
    cfg.hotplug = false;
    check(pads.init(cfg), "load_mappings hostile: init() returns true");

    check(pads.load_mappings_file(nullptr) == 0, "load_mappings_file(nullptr) == 0 (initialized)");
    check(pads.load_mappings_file((dir / "does-not-exist.txt").string().c_str()) == 0,
          "load_mappings_file(nonexistent path) == 0");

    const fs::path garbage_path = dir / "garbage-mappings.bin";
    {
      std::mt19937                       rng(0xA2AD10);
      std::uniform_int_distribution<int> byte_dist(0, 255);
      std::vector<unsigned char>         garbage(4096);
      for (auto& b : garbage) {
        b = static_cast<unsigned char>(byte_dist(rng));
      }
      std::ofstream f(garbage_path, std::ios::binary | std::ios::trunc);
      f.write(reinterpret_cast<const char*>(garbage.data()), static_cast<std::streamsize>(garbage.size()));
    }
    // EN: Random binary garbage may contain embedded NUL bytes -- parse_text()'s own contract
    //     (gamepad_mapping.hpp) is to never crash regardless; this test does not assert a count
    //     of 0 (a random byte soup COULD, astronomically unlikely but not impossible, contain a
    //     32-hex-char run followed by a comma), it asserts the call returns promptly and safely.
    // PT: Lixo binário aleatório pode conter bytes NUL embutidos -- o próprio contrato do
    //     parse_text() (gamepad_mapping.hpp) é nunca crashar de qualquer forma; este teste não
    //     afirma uma contagem de 0 (uma sopa de bytes aleatória PODERIA, astronomicamente
    //     improvável mas não impossível, conter uma sequência de 32 chars hex seguida de
    //     vírgula), ele afirma que a chamada retorna prontamente e com segurança.
    (void)pads.load_mappings_file(garbage_path.string().c_str());

    check(pads.load_mappings_text(nullptr) == 0, "load_mappings_text(nullptr) == 0 (initialized)");
    check(pads.load_mappings_text("") == 0, "load_mappings_text(empty string) == 0");
    check(pads.load_mappings_text("garbage, no valid guid line here at all\n") == 0,
          "load_mappings_text(garbage text) == 0");

    pads.shutdown();
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_api_hardening: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_api_hardening: PASS");
  return 0;
}
