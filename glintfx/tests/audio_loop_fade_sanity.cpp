// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Audio's TIME-BASED loop/fade proofs (AUDIO-F2, framework-2D)
//     -- no GL, no window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb" comment in
//     tests/CMakeLists.txt). Uses AudioConfig::null_backend=true (headless). Same
//     self-contained/no-repo-fixture discipline as audio_playback_sanity.cpp/
//     audio_polyphony_sanity.cpp (WAV built in code, duplicated on purpose, not shared). The
//     null backend genuinely advances wall-clock time (its device thread is timer-paced), so
//     every assertion below is either poll-with-timeout (never a bare sleep+assert) or a fixed
//     wait chosen to be safely past/short-of the window it is proving something about. Covers:
//     the loop-twin proof (a non-looping sound ends while its looping twin keeps playing, both
//     right when the twin ends AND after an extra margin -- proves it wrapped instead of
//     stopping); the fade-out observable (is_playing() stays true WHILE stop(id, fade_out_s>0)
//     fades, then becomes false once the window elapses); the play-after-fade-stop regression
//     (§1 of the plan this implements -- a fresh play() must reset a previously-scheduled
//     fade-stop, per the vendored miniaudio.h's own declaration-comment warning, or the new
//     playback would be silently killed once the old stop time arrives). NOT provable headless
//     (no PCM tap on the null device): fade-in AUDIBILITY, loop GAPLESSNESS -- covered by code
//     review of the exact miniaudio calls plus a manual smoke on a real host (see docs/audio.md).
//     The DETERMINISTIC (no clock) polyphony proofs (voice_count/cap/reap) live in
//     audio_polyphony_sanity.cpp instead -- this file stays about TIME.
// PT: Teste unit puro pras provas TEMPORAIS de loop/fade do glintfx::Audio (AUDIO-F2,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora
//     do GLFW/Xvfb" em tests/CMakeLists.txt). Usa AudioConfig::null_backend=true (headless).
//     Mesma disciplina autocontida/sem-fixture-de-repo do audio_playback_sanity.cpp/
//     audio_polyphony_sanity.cpp (WAV construído em código, duplicado de propósito, não
//     compartilhado). O backend null avança tempo de parede de verdade (a thread de device dele
//     é cadenciada por timer), então toda asserção abaixo é ou poll-com-timeout (nunca um
//     sleep+assert seco) ou uma espera fixa escolhida pra ficar seguramente além/aquém da janela
//     que está provando algo. Cobre: a prova do gêmeo de loop (um som sem loop termina enquanto
//     o gêmeo em loop continua tocando, tanto bem quando o gêmeo termina QUANTO depois de uma
//     margem extra -- prova que ele deu a volta em vez de parar); o observável do fade-out
//     (is_playing() continua true ENQUANTO o stop(id, fade_out_s>0) faz o fade, depois vira
//     false quando a janela passa); a regressão play-após-fade-stop (§1 do plano que isto
//     implementa -- um play() novo precisa resetar um fade-stop agendado antes, conforme o
//     próprio aviso do comentário de declaração do miniaudio.h vendorizado, senão a reprodução
//     nova seria morta silenciosamente quando o stop-time antigo chegasse). NÃO provável headless
//     (sem tap de PCM no device null): AUDIBILIDADE do fade-in, GAPLESSNESS do loop -- coberto
//     por review de código das chamadas exatas do miniaudio mais um smoke manual num host real
//     (ver docs/audio.md). As provas DETERMINÍSTICAS (sem relógio) de polifonia (voice_count/
//     teto/reap) vivem no audio_polyphony_sanity.cpp -- este arquivo continua sobre TEMPO.
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

// EN: Polls `pred` up to `timeout` (5s cap, per this suite's plan), sleeping `step` between
//     checks -- see audio_polyphony_sanity.cpp's identical helper for the full rationale
//     (duplicated on purpose, same self-contained-file discipline).
// PT: Faz poll de `pred` até `timeout` (teto de 5s, conforme o plano desta suíte), dormindo
//     `step` entre checagens -- ver o helper idêntico do audio_polyphony_sanity.cpp pro
//     raciocínio completo (duplicado de propósito, mesma disciplina de arquivo autocontido).
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

// EN: Minimal, well-formed mono 16-bit PCM WAV builder, parameterized by sample count -- see
//     audio_polyphony_sanity.cpp's identical builder for the full rationale.
// PT: Builder de WAV mono PCM 16-bit mínimo e bem-formado, parametrizado por contagem de
//     amostras -- ver o builder idêntico do audio_polyphony_sanity.cpp pro raciocínio completo.
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
  const fs::path dir = fs::temp_directory_path() / "glintfx-audio-loop-fade-sanity";
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

  const fs::path short_wav_path = dir / "short.wav"; // EN: 12.5ms @ 8kHz. PT: 12,5ms @ 8kHz.
  const fs::path long_wav_path  = dir / "long.wav";  // EN: 2s @ 8kHz -- outlives every fixed wait below. PT: 2s @ 8kHz -- sobrevive a toda espera fixa abaixo.
  write_file(short_wav_path, build_wav(100));
  write_file(long_wav_path, build_wav(16000));

  Audio       audio;
  AudioConfig cfg;
  cfg.null_backend = true;
  check(audio.init(cfg), "init(null_backend): returns true");

  // ---------------------------------------------------------------------------
  // EN: Loop twin proof -- two independent SoundIds decoded from the SAME short (12.5ms) WAV:
  //     `id_once` plays without looping and is expected to reach its natural end quickly;
  //     `id_loop` plays with loop=true and must still be playing both right when `id_once` ends
  //     AND after a further margin (proves it wrapped instead of stopping at the end -- the
  //     GAPLESSNESS of the wrap itself is not provable headless, see this file's header comment).
  // PT: Prova do gêmeo de loop -- dois SoundIds independentes decodificados do MESMO WAV curto
  //     (12,5ms): `id_once` toca sem loop e é esperado atingir o fim natural rápido; `id_loop`
  //     toca com loop=true e precisa continuar tocando tanto bem quando `id_once` termina QUANTO
  //     depois de uma margem extra (prova que deu a volta em vez de parar no fim -- o GAPLESSNESS
  //     da volta em si não é provável headless, ver o comentário de cabeçalho deste arquivo).
  // ---------------------------------------------------------------------------
  {
    const Audio::SoundId id_once = audio.load_sound(short_wav_path.string().c_str());
    const Audio::SoundId id_loop = audio.load_sound(short_wav_path.string().c_str());
    check(id_once != 0 && id_loop != 0, "loop twin: both load_sound() calls return a non-zero id");

    check(audio.play(id_once), "loop twin: play(id_once) (no loop) returns true");
    check(audio.play(id_loop, /*loop=*/true), "loop twin: play(id_loop, loop=true) returns true");

    check(poll_until([&] { return !audio.is_playing(id_once); }),
          "loop twin: is_playing(id_once) becomes false within the timeout (reaches its natural end)");
    check(audio.is_playing(id_loop),
          "loop twin: is_playing(id_loop) is STILL true right when the non-loop twin ended");

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    check(audio.is_playing(id_loop),
          "loop twin: is_playing(id_loop) is STILL true 300ms after the non-loop twin ended (wrapped, did not stop)");

    audio.stop(id_loop);
    check(!audio.is_playing(id_loop), "loop twin: stop(id_loop) ends the looping twin");
  }

  // ---------------------------------------------------------------------------
  // EN: Fade-out observable -- stop(id, fade_out_s > 0) does NOT stop the sound immediately;
  //     is_playing() stays true throughout the fade window (per ma_sound_is_playing()'s own
  //     scheduled-stop-time awareness) and only becomes false once the window elapses. Uses the
  //     LONG wav so the primary's own natural end cannot race the (short) fade window.
  // PT: Observável do fade-out -- stop(id, fade_out_s > 0) NÃO para o som imediatamente;
  //     is_playing() continua true durante toda a janela do fade (conforme a própria ciência de
  //     stop-time-agendado do ma_sound_is_playing()) e só vira false quando a janela passa. Usa o
  //     WAV LONGO pra que o fim natural da primária não corra com a janela (curta) do fade.
  // ---------------------------------------------------------------------------
  {
    const Audio::SoundId fade_id = audio.load_sound(long_wav_path.string().c_str());
    check(fade_id != 0, "fade-out observable: load_sound() returns a non-zero id");

    check(audio.play(fade_id), "fade-out observable: play(fade_id) returns true");
    check(audio.stop(fade_id, /*fade_out_s=*/0.05f), "fade-out observable: stop(fade_id, 0.05f) returns true");
    check(audio.is_playing(fade_id), "fade-out observable: is_playing(fade_id) is true immediately after stop() with a fade");

    check(poll_until([&] { return !audio.is_playing(fade_id); }),
          "fade-out observable: is_playing(fade_id) becomes false within the timeout once the fade window elapses");
  }

  // ---------------------------------------------------------------------------
  // EN: play-after-fade-stop regression (§1 of the plan this implements) -- the vendored
  //     miniaudio.h's own declaration comment for ma_sound_stop_with_fade_in_milliseconds() warns
  //     that a fresh play() MUST reset the previously-scheduled stop time (this module does so
  //     via ma_sound_reset_stop_time_and_fade(), see audio.cpp's play()) or the OLD scheduled
  //     stop time silently kills the NEW playback once it arrives. Schedule a SHORT (0.1s)
  //     fade-stop, play() again immediately, then wait well PAST where the old stop time would
  //     have fired (0.6s -- 6x the old window) and assert it is STILL playing. Uses the LONG wav
  //     so nothing here is confused with the sound's own natural end.
  // PT: Regressão play-após-fade-stop (§1 do plano que isto implementa) -- o próprio comentário
  //     de declaração do miniaudio.h vendorizado pro ma_sound_stop_with_fade_in_milliseconds()
  //     avisa que um play() novo PRECISA resetar o stop-time agendado antes (este módulo faz isso
  //     via ma_sound_reset_stop_time_and_fade(), ver o play() do audio.cpp) senão o stop-time
  //     antigo agendado mata silenciosamente a reprodução NOVA assim que chega. Agenda um
  //     fade-stop CURTO (0,1s), dá play() de novo imediatamente, depois espera bem ALÉM de onde o
  //     stop-time antigo teria disparado (0,6s -- 6x a janela antiga) e afirma que ainda está
  //     tocando. Usa o WAV LONGO pra que nada aqui seja confundido com o fim natural do som.
  // ---------------------------------------------------------------------------
  {
    const Audio::SoundId regress_id = audio.load_sound(long_wav_path.string().c_str());
    check(regress_id != 0, "play-after-fade-stop regression: load_sound() returns a non-zero id");

    check(audio.play(regress_id), "play-after-fade-stop regression: initial play(regress_id) returns true");
    check(audio.stop(regress_id, /*fade_out_s=*/0.1f),
          "play-after-fade-stop regression: stop(regress_id, 0.1f) schedules a short fade-stop");
    check(audio.is_playing(regress_id),
          "play-after-fade-stop regression: is_playing(regress_id) still true immediately (fading, not yet stopped)");

    check(audio.play(regress_id),
          "play-after-fade-stop regression: play(regress_id) again succeeds while the old fade-stop is still scheduled");

    std::this_thread::sleep_for(std::chrono::milliseconds(600)); // EN: 6x past the old 0.1s window. PT: 6x além da janela antiga de 0,1s.
    check(audio.is_playing(regress_id),
          "play-after-fade-stop regression: is_playing(regress_id) is STILL true well past the old fade-stop's window -- play() reset it instead of being silently killed");

    audio.stop(regress_id);
  }

  audio.shutdown();

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_loop_fade_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_loop_fade_sanity: PASS");
  return 0;
}
