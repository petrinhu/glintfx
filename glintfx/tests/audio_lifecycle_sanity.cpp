// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Audio's lifecycle/RAII contract (A3-AUDIO, framework-2D) --
//     no GL, no window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb" comment in
//     tests/CMakeLists.txt). Uses AudioConfig::null_backend=true throughout (headless, no
//     ALSA/PulseAudio dependency). Exercises: init/shutdown/is_initialized state transitions;
//     shutdown() idempotency (2x, and via the destructor without an explicit call); init()
//     called twice without an intervening shutdown() (must fail, must not disturb the already-
//     initialized instance); every operation attempted before the first successful init() OR
//     after shutdown() returning false/0 (never a crash -- the core RAII promise in
//     glintfx/audio.hpp's header comment); re-init after a clean shutdown() working normally.
//     Does NOT touch load_sound()/play()/etc. with a real file -- that is
//     audio_playback_sanity.cpp's job; this file is about STATE, not decode.
// PT: Teste unit puro para o contrato de ciclo-de-vida/RAII do glintfx::Audio (A3-AUDIO,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora
//     do GLFW/Xvfb" em tests/CMakeLists.txt). Usa AudioConfig::null_backend=true o tempo todo
//     (headless, sem dependência de ALSA/PulseAudio). Exercita: transições de estado
//     init/shutdown/is_initialized; idempotência do shutdown() (2x, e via o destrutor sem
//     chamada explícita); init() chamado duas vezes sem shutdown() no meio (precisa falhar, não
//     pode perturbar a instância já inicializada); toda operação tentada antes do primeiro
//     init() bem-sucedido OU depois do shutdown() retornando false/0 (nunca um crash -- a
//     promessa central de RAII no comentário de cabeçalho do glintfx/audio.hpp); re-init depois
//     de um shutdown() limpo funcionando normalmente. NÃO toca em load_sound()/play()/etc. com
//     um arquivo de verdade -- isso é trabalho do audio_playback_sanity.cpp; este arquivo é
//     sobre ESTADO, não decode.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#include <cstdio>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: Ops attempted before ANY init() must return false/0, never crash.
  // PT: Operações tentadas ANTES de qualquer init() precisam retornar false/0, nunca crashar.
  // ---------------------------------------------------------------------------
  {
    Audio audio;
    check(!audio.is_initialized(), "pre-init: is_initialized() == false");
    check(audio.load_sound("/does/not/matter.wav") == 0, "pre-init: load_sound() == 0");
    check(!audio.play(1), "pre-init: play(1) == false");
    check(!audio.stop(1), "pre-init: stop(1) == false");
    check(!audio.set_volume(1, 0.5f), "pre-init: set_volume(1, 0.5f) == false");
    audio.set_master_volume(0.5f); // EN: void, must not crash. PT: void, não pode crashar.
    audio.stop_all();              // EN: void, must not crash. PT: void, não pode crashar.
  }

  // ---------------------------------------------------------------------------
  // EN: init(null_backend=true) succeeds; is_initialized() flips true; shutdown() flips it back
  //     to false; a second shutdown() is a safe no-op (idempotent).
  // PT: init(null_backend=true) tem sucesso; is_initialized() vira true; shutdown() volta a
  //     false; um segundo shutdown() é um no-op seguro (idempotente).
  // ---------------------------------------------------------------------------
  {
    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    check(audio.init(cfg), "init(null_backend): returns true");
    check(audio.is_initialized(), "post-init: is_initialized() == true");

    audio.shutdown();
    check(!audio.is_initialized(), "post-shutdown: is_initialized() == false");

    audio.shutdown(); // EN: idempotent, second call. PT: idempotente, segunda chamada.
    check(!audio.is_initialized(), "post-shutdown x2: is_initialized() still false, no crash");
  }

  // ---------------------------------------------------------------------------
  // EN: The destructor calls shutdown() on its own -- no explicit shutdown() call in this
  //     scope; if this leaks or use-after-frees anything, ASan (the review adversarial's gate)
  //     catches it, not this assertion -- this block just proves the object is USABLE up to
  //     scope exit without an explicit call.
  // PT: O destrutor chama shutdown() sozinho -- nenhuma chamada explícita a shutdown() neste
  //     escopo; se isso vazar ou fizer use-after-free de algo, o ASan (o gate do review
  //     adversarial) pega, não esta asserção -- este bloco só prova que o objeto é USÁVEL até a
  //     saída de escopo sem chamada explícita.
  // ---------------------------------------------------------------------------
  {
    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    check(audio.init(cfg), "dtor test: init() returns true");
    check(audio.is_initialized(), "dtor test: is_initialized() == true before scope exit");
    // EN: no shutdown() call here -- ~Audio() must do it. PT: nenhuma chamada shutdown() aqui -- ~Audio() precisa fazer.
  }

  // ---------------------------------------------------------------------------
  // EN: init() called a second time WITHOUT an intervening shutdown() must return false and
  //     must NOT disturb the already-initialized instance (still usable afterwards).
  // PT: init() chamado uma segunda vez SEM shutdown() no meio precisa retornar false e NÃO pode
  //     perturbar a instância já inicializada (ainda usável depois).
  // ---------------------------------------------------------------------------
  {
    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    check(audio.init(cfg), "double-init: first init() returns true");
    check(!audio.init(cfg), "double-init: second init() (no shutdown between) returns false");
    check(audio.is_initialized(), "double-init: still initialized after the rejected second call");
    audio.shutdown();
  }

  // ---------------------------------------------------------------------------
  // EN: Ops attempted AFTER shutdown() must return false/0, exactly like pre-init -- no UAF,
  //     no crash (the GusWorld pain this module exists to prevent).
  // PT: Operações tentadas DEPOIS do shutdown() precisam retornar false/0, exatamente como
  //     pré-init -- sem UAF, sem crash (a dor do GusWorld que este módulo existe pra evitar).
  // ---------------------------------------------------------------------------
  {
    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    audio.init(cfg);
    audio.shutdown();

    check(audio.load_sound("/does/not/matter.wav") == 0, "post-shutdown: load_sound() == 0");
    check(!audio.play(1), "post-shutdown: play(1) == false");
    check(!audio.stop(1), "post-shutdown: stop(1) == false");
    check(!audio.set_volume(1, 0.5f), "post-shutdown: set_volume(1, 0.5f) == false");
    audio.set_master_volume(0.5f); // EN: void, must not crash. PT: void, não pode crashar.
    audio.stop_all();              // EN: void, must not crash. PT: void, não pode crashar.
  }

  // ---------------------------------------------------------------------------
  // EN: Re-init after a clean shutdown() is fully supported (documented in audio.hpp's init()
  //     comment) -- this is the exact cycle a level-transition in a real game performs.
  // PT: Re-init depois de um shutdown() limpo é totalmente suportado (documentado no comentário
  //     de init() do audio.hpp) -- é exatamente o ciclo que uma transição de nível num jogo de
  //     verdade faz.
  // ---------------------------------------------------------------------------
  {
    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    check(audio.init(cfg), "re-init: first init() returns true");
    audio.shutdown();
    check(!audio.is_initialized(), "re-init: is_initialized() == false after shutdown()");
    check(audio.init(cfg), "re-init: second init() (after shutdown) returns true");
    check(audio.is_initialized(), "re-init: is_initialized() == true after the second init()");
    audio.shutdown();
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_lifecycle_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_lifecycle_sanity: PASS");
  return 0;
}
