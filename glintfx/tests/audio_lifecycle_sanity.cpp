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
//     Does NOT otherwise touch load_sound()/play()/etc. with a real file -- that is
//     audio_playback_sanity.cpp's job; this file is about STATE, not decode. AUDIO-F2 addition
//     (the one exception to the paragraph above): shutdown() with a live LOOP + a live FADE-OUT
//     in progress + live oneshot COPIES all at once must still tear down clean (the ASan/LSan
//     run in the review-adversarial build is the actual proof of no leak/UAF -- this file only
//     asserts the observable state transitions) and a subsequent re-init must work exactly like
//     the plain re-init case above.
// PT: Teste unit puro para o contrato de ciclo-de-vida/RAII do glintfx::Audio (A3-AUDIO,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora
//     do GLFW/Xvfb" em tests/CMakeLists.txt). Usa AudioConfig::null_backend=true o tempo todo
//     (headless, sem dependência de ALSA/PulseAudio). Exercita: transições de estado
//     init/shutdown/is_initialized; idempotência do shutdown() (2x, e via o destrutor sem
//     chamada explícita); init() chamado duas vezes sem shutdown() no meio (precisa falhar, não
//     pode perturbar a instância já inicializada); toda operação tentada antes do primeiro
//     init() bem-sucedido OU depois do shutdown() retornando false/0 (nunca um crash -- a
//     promessa central de RAII no comentário de cabeçalho do glintfx/audio.hpp); re-init depois
//     de um shutdown() limpo funcionando normalmente. Fora isso, NÃO toca em load_sound()/
//     play()/etc. com um arquivo de verdade -- isso é trabalho do audio_playback_sanity.cpp;
//     este arquivo é sobre ESTADO, não decode. Adição do AUDIO-F2 (a única exceção ao parágrafo
//     acima): shutdown() com um LOOP vivo + um FADE-OUT em andamento + cópias oneshot vivas ao
//     mesmo tempo precisa desmontar limpo mesmo assim (a rodada ASan/LSan no build do review
//     adversarial É a prova de fato de zero leak/UAF -- este arquivo só afirma as transições de
//     estado observáveis) e um re-init subsequente precisa funcionar exatamente como o caso de
//     re-init simples acima.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: AUDIO-F2's shutdown-with-live-state block below is the only place in this file that needs
//     a real, decodable sound -- same minimal-WAV-built-in-code shape as
//     audio_playback_sanity.cpp/audio_hostile_sanity.cpp (deliberately duplicated, not shared:
//     each test file in this suite is self-contained, same discipline as those two files).
// PT: O bloco de shutdown-com-estado-vivo do AUDIO-F2 abaixo é o único lugar deste arquivo que
//     precisa de um som real e decodificável -- mesma forma de WAV mínimo construído em código
//     do audio_playback_sanity.cpp/audio_hostile_sanity.cpp (deliberadamente duplicado, não
//     compartilhado: cada arquivo de teste desta suíte é autocontido, mesma disciplina desses
//     dois arquivos).
void append_u32_le(std::vector<unsigned char>& out, std::uint32_t v) {
  out.push_back(static_cast<unsigned char>(v & 0xFF));
  out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  out.push_back(static_cast<unsigned char>((v >> 16) & 0xFF));
  out.push_back(static_cast<unsigned char>((v >> 24) & 0xFF));
}

void append_u16_le(std::vector<unsigned char>& out, std::uint16_t v) {
  out.push_back(static_cast<unsigned char>(v & 0xFF));
  out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
}

void append_bytes(std::vector<unsigned char>& out, const char* tag) {
  out.insert(out.end(), tag, tag + 4);
}

std::vector<unsigned char> build_minimal_wav(int sample_count = 100) {
  const std::uint32_t sample_rate     = 8000;
  const std::uint16_t num_channels    = 1;
  const std::uint16_t bits_per_sample = 16;
  const std::uint32_t byte_rate       = sample_rate * num_channels * bits_per_sample / 8;
  const std::uint16_t block_align     = static_cast<std::uint16_t>(num_channels * bits_per_sample / 8);
  const std::uint32_t data_size       = static_cast<std::uint32_t>(sample_count) * block_align;

  std::vector<unsigned char> wav;
  append_bytes(wav, "RIFF");
  append_u32_le(wav, 36 + data_size);
  append_bytes(wav, "WAVE");
  append_bytes(wav, "fmt ");
  append_u32_le(wav, 16);
  append_u16_le(wav, 1);
  append_u16_le(wav, num_channels);
  append_u32_le(wav, sample_rate);
  append_u32_le(wav, byte_rate);
  append_u16_le(wav, block_align);
  append_u16_le(wav, bits_per_sample);
  append_bytes(wav, "data");
  append_u32_le(wav, data_size);
  wav.resize(wav.size() + data_size, 0); // EN: silence, content irrelevant. PT: silêncio, conteúdo irrelevante.
  return wav;
}

void write_file(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
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

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- shutdown() with a live loop + a live fade-out in progress + live oneshot
  //     copies, all at once, must still tear down clean; a subsequent re-init must work. `id_a`
  //     carries a looping primary plus 2 live oneshot copies; `id_b` is a primary mid-way
  //     through a long (5s) fade-out (still fading when shutdown() runs, well before the fade
  //     window elapses). This exercises every one of shutdown()'s new voices-then-sounds
  //     ordering paths at once -- the ASan/LSan run (review-adversarial's build) is the actual
  //     proof of no leak/UAF, not an assertion in this file.
  // PT: AUDIO-F2 -- shutdown() com um loop vivo + um fade-out em andamento + cópias oneshot
  //     vivas, tudo ao mesmo tempo, precisa desmontar limpo mesmo assim; um re-init subsequente
  //     precisa funcionar. `id_a` carrega uma primária em loop mais 2 cópias oneshot vivas;
  //     `id_b` é uma primária no meio de um fade-out longo (5s) (ainda fazendo fade quando o
  //     shutdown() roda, bem antes da janela do fade terminar). Isto exercita todo caminho da
  //     nova ordem vozes-depois-sons do shutdown() de uma vez -- a rodada ASan/LSan (build do
  //     review adversarial) é a prova de fato de zero leak/UAF, não uma asserção neste arquivo.
  // ---------------------------------------------------------------------------
  {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "glintfx-audio-lifecycle-f2-sanity";
    std::error_code ec;
    fs::remove_all(dir, ec); // EN: clean slate from a possible prior crashed run. PT: estado limpo de uma execução anterior que possa ter crashado.
    fs::create_directory(dir, ec);
    check(!ec, "AUDIO-F2 live-state shutdown: temp directory created");

    struct DirGuard {
      fs::path path;
      ~DirGuard() {
        std::error_code cleanup_ec;
        fs::remove_all(path, cleanup_ec);
      }
    } dir_guard{dir};

    const fs::path wav_path = dir / "minimal.wav";
    write_file(wav_path, build_minimal_wav());

    Audio      audio;
    AudioConfig cfg;
    cfg.null_backend = true;
    check(audio.init(cfg), "AUDIO-F2 live-state shutdown: init() returns true");

    const Audio::SoundId id_a = audio.load_sound(wav_path.string().c_str());
    const Audio::SoundId id_b = audio.load_sound(wav_path.string().c_str());
    check(id_a != 0 && id_b != 0, "AUDIO-F2 live-state shutdown: both load_sound() calls return a non-zero id");

    check(audio.play(id_a, /*loop=*/true), "AUDIO-F2 live-state shutdown: play(id_a, loop=true) returns true");
    check(audio.play_oneshot(id_a), "AUDIO-F2 live-state shutdown: play_oneshot(id_a) #1 returns true");
    check(audio.play_oneshot(id_a), "AUDIO-F2 live-state shutdown: play_oneshot(id_a) #2 returns true");

    check(audio.play(id_b), "AUDIO-F2 live-state shutdown: play(id_b) returns true");
    check(audio.stop(id_b, /*fade_out_s=*/5.0f), "AUDIO-F2 live-state shutdown: stop(id_b, 5.0f) returns true");
    check(audio.is_playing(id_b), "AUDIO-F2 live-state shutdown: is_playing(id_b) still true mid-fade-out");

    audio.shutdown(); // EN: the ordering + ASan/LSan are the real proof here. PT: a ordem + ASan/LSan são a prova de verdade aqui.
    check(!audio.is_initialized(), "AUDIO-F2 live-state shutdown: is_initialized() == false after shutdown");

    check(audio.init(cfg), "AUDIO-F2 live-state shutdown: re-init afterwards returns true");
    check(audio.is_initialized(), "AUDIO-F2 live-state shutdown: is_initialized() == true after the re-init");
    audio.shutdown();
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_lifecycle_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_lifecycle_sanity: PASS");
  return 0;
}
