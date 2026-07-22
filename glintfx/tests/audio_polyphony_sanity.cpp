// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Audio's polyphony surface (AUDIO-F2, framework-2D) -- no GL,
//     no window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb" comment in
//     tests/CMakeLists.txt). Uses AudioConfig::null_backend=true (headless). Same
//     self-contained/no-repo-fixture discipline as audio_playback_sanity.cpp/
//     audio_hostile_sanity.cpp (WAV built in code, duplicated on purpose, not shared). Exercises
//     the DETERMINISTIC (no clock) half of AUDIO-F2's polyphony contract (see audio.hpp's
//     POLYPHONY paragraph): voice_count() == N right after N play_oneshot() calls of a LONG wav
//     (still all playing, nothing has ended); the 32-per-id cap with oldest-steal keeping
//     voice_count() exactly at the cap under spam, and play_oneshot() itself never failing
//     because of the cap; the lazy-reap contract (a SHORT wav's copies naturally end, is_playing/
//     voice_count reflect that immediately, and a subsequent play_oneshot() call -- which is the
//     only place the reap actually runs -- does not crash while dropping the ended entries; the
//     ASan/LSan run in the review-adversarial build is the real proof no reaped memory leaks).
//     The TIME-BASED proofs (loop gaplessness twin, fade-out observable, play-after-fade-stop
//     regression) live in audio_loop_fade_sanity.cpp instead -- this file stays about the voice
//     TABLE, not fades/loop.
// PT: Teste unit puro pra superfície de polifonia do glintfx::Audio (AUDIO-F2, framework-2D) --
//     sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora do GLFW/Xvfb" em
//     tests/CMakeLists.txt). Usa AudioConfig::null_backend=true (headless). Mesma disciplina
//     autocontida/sem-fixture-de-repo do audio_playback_sanity.cpp/audio_hostile_sanity.cpp (WAV
//     construído em código, duplicado de propósito, não compartilhado). Exercita a metade
//     DETERMINÍSTICA (sem relógio) do contrato de polifonia do AUDIO-F2 (ver o parágrafo
//     POLIFONIA do audio.hpp): voice_count() == N logo após N chamadas play_oneshot() de um WAV
//     LONGO (todas ainda tocando, nada terminou); o teto de 32-por-id com steal-do-mais-antigo
//     mantendo voice_count() exatamente no teto sob spam, e o play_oneshot() em si nunca falhando
//     por causa do teto; o contrato de reap preguiçoso (as cópias de um WAV CURTO terminam
//     naturalmente, is_playing/voice_count refletem isso imediatamente, e uma chamada
//     play_oneshot() subsequente -- o único lugar onde o reap de fato roda -- não crasha ao
//     descartar as entradas terminadas; a rodada ASan/LSan no build do review adversarial é a
//     prova de fato de que nenhuma memória reapada vaza). As provas TEMPORAIS (gêmeo de
//     gaplessness do loop, observável do fade-out, regressão play-após-fade-stop) vivem no
//     audio_loop_fade_sanity.cpp -- este arquivo continua sobre a TABELA de vozes, não
//     fades/loop.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <thread>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Polls `pred` up to `timeout` (5s cap, per this suite's plan -- never a bare sleep+assert
//     for a time-based condition), sleeping `step` between checks. The null backend genuinely
//     advances wall-clock time (its device thread is timer-paced), so this is the only sound way
//     to observe a sound reaching its natural end. Returns true the first tick `pred()` is true,
//     false if the deadline passes first (one final check is still made at the deadline).
// PT: Faz poll de `pred` até `timeout` (teto de 5s, conforme o plano desta suíte -- nunca um
//     sleep+assert seco pra uma condição temporal), dormindo `step` entre checagens. O backend
//     null avança tempo de parede de verdade (a thread de device dele é cadenciada por timer),
//     então esta é a única forma sã de observar um som chegando ao fim natural. Retorna true no
//     primeiro tick em que `pred()` é true, false se o prazo passar primeiro (uma checagem final
//     ainda é feita no prazo).
bool poll_until(const std::function<bool()>& pred,
                 std::chrono::milliseconds timeout = std::chrono::seconds(5),
                 std::chrono::milliseconds step = std::chrono::milliseconds(5)) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  for (;;) {
    if (pred()) {
      return true;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      return pred();
    }
    std::this_thread::sleep_for(step);
  }
}

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

// EN: Minimal, well-formed mono 16-bit PCM WAV builder (same shape as
//     audio_playback_sanity.cpp's), parameterized by sample count so callers control duration:
//     a SHORT wav (default) ends fast for the reap proof; a LONG wav stays alive for the whole
//     duration of the cap/steal and voice_count() proofs.
// PT: Builder de WAV mono PCM 16-bit mínimo e bem-formado (mesma forma do
//     audio_playback_sanity.cpp), parametrizado por contagem de amostras pra quem chama
//     controlar a duração: um WAV CURTO (padrão) termina rápido pra prova de reap; um WAV LONGO
//     fica vivo pela duração inteira das provas de teto/steal e voice_count().
std::vector<unsigned char> build_wav(int sample_count) {
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
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "glintfx-audio-polyphony-sanity";
  std::error_code ec;
  fs::remove_all(dir, ec); // EN: clean slate from a possible prior crashed run. PT: estado limpo de uma execução anterior que possa ter crashado.
  fs::create_directory(dir, ec);
  check(!ec, "test setup: temp directory created");

  struct DirGuard {
    fs::path path;
    ~DirGuard() {
      std::error_code cleanup_ec;
      fs::remove_all(path, cleanup_ec);
    }
  } dir_guard{dir};

  // EN: A LONG wav (1s @ 8kHz) -- stays alive across every deterministic assertion below (no
  //     race with the null backend's real-time-advancing device thread). A SHORT wav (12.5ms,
  //     same shape audio_playback_sanity.cpp uses) -- for the reap proof, deliberately ends fast.
  // PT: Um WAV LONGO (1s @ 8kHz) -- fica vivo por toda asserção determinística abaixo (sem race
  //     com a thread de device do backend null, que avança tempo real). Um WAV CURTO (12,5ms,
  //     mesma forma que o audio_playback_sanity.cpp usa) -- pra prova de reap, termina rápido de
  //     propósito.
  const fs::path long_wav_path  = dir / "long.wav";
  const fs::path short_wav_path = dir / "short.wav";
  write_file(long_wav_path, build_wav(8000));
  write_file(short_wav_path, build_wav(100));

  Audio       audio;
  AudioConfig cfg;
  cfg.null_backend = true;
  check(audio.init(cfg), "init(null_backend): returns true");

  const Audio::SoundId long_id = audio.load_sound(long_wav_path.string().c_str());
  check(long_id != 0, "load_sound(long WAV): returns a non-zero SoundId");

  // ---------------------------------------------------------------------------
  // EN: voice_count(long_id) == N right after N play_oneshot() calls -- all copies of the LONG
  //     wav are still playing, deterministic (no clock needed).
  // PT: voice_count(long_id) == N logo após N chamadas play_oneshot() -- todas as cópias do WAV
  //     LONGO ainda estão tocando, determinístico (sem precisar de relógio).
  // ---------------------------------------------------------------------------
  check(audio.voice_count(long_id) == 0, "voice_count(long_id) before any play_oneshot(): 0");
  for (int i = 0; i < 5; ++i) {
    check(audio.play_oneshot(long_id), "play_oneshot(long_id): returns true");
  }
  check(audio.voice_count(long_id) == 5, "voice_count(long_id) right after 5 play_oneshot() calls: 5");
  check(audio.is_playing(long_id), "is_playing(long_id): true (copies playing, no primary needed)");

  audio.stop_all(); // EN: clears the 5 copies above before the cap test below. PT: limpa as 5 cópias acima antes do teste de teto abaixo.
  check(poll_until([&] { return audio.voice_count(long_id) == 0; }),
        "voice_count(long_id) reaches 0 within the timeout after stop_all()");

  // ---------------------------------------------------------------------------
  // EN: 32-per-id cap with oldest-steal: spamming 40 play_oneshot() calls of the LONG wav (still
  //     all playing -- 1s duration dwarfs the time this loop takes) leaves EXACTLY 32 live
  //     copies (8 stolen), and no call fails because of the cap.
  // PT: Teto de 32-por-id com steal-do-mais-antigo: um spam de 40 chamadas play_oneshot() do WAV
  //     LONGO (todas ainda tocando -- a duração de 1s é muito maior que o tempo deste laço)
  //     deixa EXATAMENTE 32 cópias vivas (8 roubadas), e nenhuma chamada falha por causa do teto.
  // ---------------------------------------------------------------------------
  for (int i = 0; i < 40; ++i) {
    check(audio.play_oneshot(long_id), "play_oneshot(long_id) spam: call never fails due to the voice cap");
  }
  check(audio.voice_count(long_id) == 32, "voice_count(long_id) after spamming 40 oneshots: exactly 32 (cap + oldest-steal)");

  audio.stop_all();
  check(poll_until([&] { return audio.voice_count(long_id) == 0; }),
        "voice_count(long_id) reaches 0 within the timeout after stop_all() (post-cap-test cleanup)");

  // ---------------------------------------------------------------------------
  // EN: Lazy reap: 3 play_oneshot() calls of the SHORT wav (12.5ms) naturally end; voice_count()
  //     tracks that immediately via ma_sound_is_playing() (no reap needed to observe THIS). Then
  //     one more play_oneshot() call -- the only place the actual reap (ma_sound_uninit() +
  //     drop) runs -- must not crash while dropping the 3 now-ended entries; the ASan/LSan run in
  //     the review-adversarial build is the real proof the freed memory does not leak/UAF.
  // PT: Reap preguiçoso: 3 chamadas play_oneshot() do WAV CURTO (12,5ms) terminam naturalmente;
  //     voice_count() reflete isso imediatamente via ma_sound_is_playing() (nenhum reap
  //     necessário pra observar ISSO). Depois, mais uma chamada play_oneshot() -- o único lugar
  //     onde o reap de fato (ma_sound_uninit() + descarte) roda -- não pode crashar ao descartar
  //     as 3 entradas já terminadas; a rodada ASan/LSan no build do review adversarial é a prova
  //     de fato de que a memória liberada não vaza/UAF.
  // ---------------------------------------------------------------------------
  const Audio::SoundId short_id = audio.load_sound(short_wav_path.string().c_str());
  check(short_id != 0, "load_sound(short WAV): returns a non-zero SoundId");

  for (int i = 0; i < 3; ++i) {
    check(audio.play_oneshot(short_id), "play_oneshot(short_id): returns true");
  }
  check(poll_until([&] { return audio.voice_count(short_id) == 0; }),
        "reap proof: voice_count(short_id) reaches 0 within the timeout (the 3 copies naturally end)");
  check(audio.play_oneshot(short_id),
        "reap proof: an extra play_oneshot() after voice_count hits 0 still succeeds (this is the call that actually reaps the 3 ended entries)");

  audio.shutdown(); // EN: with a live copy (the extra one above) still in the pool -- exercises the voices-before-primaries shutdown ordering too. PT: com uma cópia viva (a extra acima) ainda no pool -- exercita a ordem voices-antes-de-primárias do shutdown() também.

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_polyphony_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_polyphony_sanity: PASS");
  return 0;
}
