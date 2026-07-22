// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Audio's decode/playback/volume surface (A3-AUDIO,
//     framework-2D) -- no GL, no window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb"
//     comment in tests/CMakeLists.txt). Uses AudioConfig::null_backend=true (headless). Writes
//     a minimal, valid PCM16 mono WAV file to a temp path IN CODE (no binary fixture committed
//     to the repo -- same "self-contained, no repo fixture" discipline i18n_catalog_sanity.cpp
//     applies to its directory-as-path test) and exercises: load -> play -> stop -> set_volume
//     on a real, successfully-decoded sound; set_volume() hardening (NaN/Inf/negative rejected,
//     valid values accepted); an unknown SoundId on every per-sound operation returning false;
//     load_sound() of a path that does not exist on disk returning 0. AUDIO-F2 additions:
//     is_playing()/voice_count() basics on the primary; play()/stop()'s extended signature
//     (loop, fade_in_s/fade_out_s) and set_looping() accepting valid values; negative-fade
//     rejection; the new ops (set_looping/play_oneshot/is_playing/voice_count) on an
//     unknown/0 id (loop-twin/fade-out/cap/reap proofs live in audio_loop_fade_sanity.cpp and
//     audio_polyphony_sanity.cpp instead -- this file stays about the non-time-based contract).
// PT: Teste unit puro para a superfície de decode/reprodução/volume do glintfx::Audio
//     (A3-AUDIO, framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário
//     "por que fora do GLFW/Xvfb" em tests/CMakeLists.txt). Usa AudioConfig::null_backend=true
//     (headless). Escreve um arquivo WAV PCM16 mono mínimo e válido num caminho temporário EM
//     CÓDIGO (sem fixture binária commitada no repo -- mesma disciplina "autocontido, sem
//     fixture de repo" que o i18n_catalog_sanity.cpp aplica ao próprio teste de
//     diretório-como-caminho) e exercita: load -> play -> stop -> set_volume num som real,
//     decodificado com sucesso; hardening do set_volume() (NaN/Inf/negativo rejeitados, valores
//     válidos aceitos); um SoundId desconhecido em toda operação por-som retornando false;
//     load_sound() de um caminho que não existe em disco retornando 0. Adições do AUDIO-F2:
//     básico de is_playing()/voice_count() na primária; assinatura estendida do play()/stop()
//     (loop, fade_in_s/fade_out_s) e set_looping() aceitando valores válidos; rejeição de fade
//     negativo; as operações novas (set_looping/play_oneshot/is_playing/voice_count) num id
//     desconhecido/0 (as provas de gêmeo-de-loop/fade-out/teto/reap vivem no
//     audio_loop_fade_sanity.cpp e no audio_polyphony_sanity.cpp -- este arquivo continua sobre
//     o contrato não-temporal).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

// EN: Little-endian byte appenders -- WAV headers are always little-endian regardless of host
//     byte order.
// PT: Appenders little-endian -- headers WAV são sempre little-endian independente da ordem de
//     bytes do host.
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

// EN: Builds a minimal, well-formed mono 16-bit PCM WAV: RIFF/WAVE header, a 16-byte "fmt "
//     subchunk, and a "data" subchunk with `sample_count` samples of a simple sine wave (the
//     actual waveform content is irrelevant to this test -- only that it decodes cleanly).
// PT: Constrói um WAV mono PCM 16-bit mínimo e bem-formado: header RIFF/WAVE, subchunk "fmt "
//     de 16 bytes, e subchunk "data" com `sample_count` amostras de uma senoide simples (o
//     conteúdo real da forma de onda é irrelevante pra este teste -- só que decodifica limpo).
std::vector<unsigned char> build_minimal_wav(int sample_count = 100) {
  const std::uint32_t sample_rate     = 8000;
  const std::uint16_t num_channels    = 1;
  const std::uint16_t bits_per_sample = 16;
  const std::uint32_t byte_rate       = sample_rate * num_channels * bits_per_sample / 8;
  const std::uint16_t block_align     = static_cast<std::uint16_t>(num_channels * bits_per_sample / 8);
  const std::uint32_t data_size       = static_cast<std::uint32_t>(sample_count) * block_align;

  std::vector<unsigned char> wav;
  append_bytes(wav, "RIFF");
  append_u32_le(wav, 36 + data_size); // EN: ChunkSize = 36 + Subchunk2Size. PT: ChunkSize = 36 + Subchunk2Size.
  append_bytes(wav, "WAVE");

  append_bytes(wav, "fmt ");
  append_u32_le(wav, 16); // EN: Subchunk1Size (PCM). PT: Subchunk1Size (PCM).
  append_u16_le(wav, 1);  // EN: AudioFormat = 1 (PCM, uncompressed). PT: AudioFormat = 1 (PCM, sem compressão).
  append_u16_le(wav, num_channels);
  append_u32_le(wav, sample_rate);
  append_u32_le(wav, byte_rate);
  append_u16_le(wav, block_align);
  append_u16_le(wav, bits_per_sample);

  append_bytes(wav, "data");
  append_u32_le(wav, data_size);
  for (int i = 0; i < sample_count; ++i) {
    const double  angle  = 2.0 * 3.14159265358979323846 * 440.0 * static_cast<double>(i) / sample_rate;
    const std::int16_t s = static_cast<std::int16_t>(8000.0 * std::sin(angle));
    append_u16_le(wav, static_cast<std::uint16_t>(s));
  }
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
  const fs::path dir = fs::temp_directory_path() / "glintfx-audio-playback-sanity";
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

  const fs::path wav_path = dir / "minimal.wav";
  write_file(wav_path, build_minimal_wav());

  Audio       audio;
  AudioConfig cfg;
  cfg.null_backend = true;
  check(audio.init(cfg), "init(null_backend): returns true");

  // ---------------------------------------------------------------------------
  // EN: load -> play -> stop -> set_volume on a real, well-formed sound.
  // PT: load -> play -> stop -> set_volume num som real e bem-formado.
  // ---------------------------------------------------------------------------
  const Audio::SoundId id = audio.load_sound(wav_path.string().c_str());
  check(id != 0, "load_sound(valid WAV): returns a non-zero SoundId");
  check(audio.play(id), "play(valid id): returns true");
  check(audio.stop(id), "stop(valid id): returns true");

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- is_playing()/voice_count() basics on the primary instance only (no oneshot
  //     copy involved here -- see audio_polyphony_sanity.cpp for copy-aware cases).
  // PT: AUDIO-F2 -- básico de is_playing()/voice_count() só na instância primária (nenhuma
  //     cópia oneshot envolvida aqui -- ver audio_polyphony_sanity.cpp pros casos com cópia).
  // ---------------------------------------------------------------------------
  check(!audio.is_playing(id), "is_playing(id) after the play/stop above: false");
  check(audio.voice_count(id) == 0, "voice_count(id) after the play/stop above: 0");
  check(audio.play(id), "play(id) again: returns true");
  check(audio.is_playing(id), "is_playing(id) right after play(): true");
  check(audio.voice_count(id) == 1, "voice_count(id) right after play(): 1 (primary only)");
  check(audio.stop(id), "stop(id) (default fade_out_s=0): returns true");
  check(!audio.is_playing(id), "is_playing(id) right after stop(id, 0): false");
  check(audio.voice_count(id) == 0, "voice_count(id) right after stop(id, 0): 0");

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- play()/stop()'s extended signature (loop, fade_in_s/fade_out_s) and
  //     set_looping() accept valid values. Audibility/gaplessness are NOT provable headless
  //     (see docs/audio.md) -- the loop-twin/fade-out-observable behaviour proofs live in
  //     audio_loop_fade_sanity.cpp; this is just the "valid values are accepted" contract.
  // PT: AUDIO-F2 -- a assinatura estendida do play()/stop() (loop, fade_in_s/fade_out_s) e o
  //     set_looping() aceitam valores válidos. Audibilidade/gapless NÃO são prováveis headless
  //     (ver docs/audio.md) -- as provas de comportamento do gêmeo de loop/observável do
  //     fade-out vivem em audio_loop_fade_sanity.cpp; isto é só o contrato "valores válidos são
  //     aceitos".
  // ---------------------------------------------------------------------------
  check(audio.play(id, /*loop=*/false, /*fade_in_s=*/0.01f), "play(id, false, 0.01f): returns true");
  check(audio.set_looping(id, true), "set_looping(id, true): returns true");
  check(audio.set_looping(id, false), "set_looping(id, false): returns true");
  check(audio.stop(id, /*fade_out_s=*/0.01f), "stop(id, 0.01f): returns true");
  // EN: is_playing(id) is expected to STILL read true right here -- a fade-out keeps the sound
  //     audible/playing throughout its window (see audio_loop_fade_sanity.cpp's dedicated
  //     "fade-out observable" proof); an IMMEDIATE stop(id, 0) below forces a known, fade-free
  //     state before the hardening block that follows.
  // PT: is_playing(id) é ESPERADO continuar lendo true bem aqui -- um fade-out mantém o som
  //     audível/tocando durante toda a janela dele (ver a prova dedicada "fade-out observável"
  //     do audio_loop_fade_sanity.cpp); um stop(id, 0) IMEDIATO abaixo força um estado conhecido,
  //     sem fade, antes do bloco de hardening que segue.
  check(audio.stop(id), "stop(id) (immediate, fade_out_s=0): returns true -- forces a known fade-free state");
  check(!audio.is_playing(id), "is_playing(id): false after the immediate stop() above");

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- fade hardening: a negative fade_in_s/fade_out_s is rejected (false), the
  //     primary is left untouched (still stopped from the line above -- not resurrected by the
  //     rejected call). NaN/Inf/a huge finite value (1e30f, accepted+clamped, not rejected) are
  //     audio_hostile_sanity.cpp's job (auditoria-dominó: same robustness class, different file
  //     per this slice's plan).
  // PT: AUDIO-F2 -- hardening de fade: um fade_in_s/fade_out_s negativo é rejeitado (false), a
  //     primária fica intocada (ainda parada da linha acima -- não ressuscitada pela chamada
  //     rejeitada). NaN/Inf/um valor finito enorme (1e30f, aceito+clampeado, não rejeitado) são
  //     trabalho do audio_hostile_sanity.cpp (auditoria-dominó: mesma classe de robustez, arquivo
  //     diferente conforme o plano desta fatia).
  // ---------------------------------------------------------------------------
  check(!audio.play(id, false, -0.5f), "play(id, false, -0.5f): returns false (negative fade rejected)");
  check(!audio.is_playing(id), "is_playing(id): still false -- the rejected play() did not start it");
  check(!audio.stop(id, -0.5f), "stop(id, -0.5f): returns false (negative fade rejected)");

  check(audio.set_volume(id, 0.5f), "set_volume(valid id, 0.5f): returns true");
  check(audio.set_volume(id, 1.0f), "set_volume(valid id, 1.0f): returns true (upper bound, no clamp needed)");
  check(audio.set_volume(id, 0.0f), "set_volume(valid id, 0.0f): returns true (lower bound)");
  check(audio.set_volume(id, 2.5f), "set_volume(valid id, 2.5f): accepted, clamped internally to 1.0");

  // ---------------------------------------------------------------------------
  // EN: set_volume() hardening -- NaN/Inf/negative rejected (false), never a crash.
  // PT: Hardening do set_volume() -- NaN/Inf/negativo rejeitados (false), nunca um crash.
  // ---------------------------------------------------------------------------
  check(!audio.set_volume(id, std::numeric_limits<float>::quiet_NaN()), "set_volume(NaN): returns false");
  check(!audio.set_volume(id, std::numeric_limits<float>::infinity()), "set_volume(+Inf): returns false");
  check(!audio.set_volume(id, -std::numeric_limits<float>::infinity()), "set_volume(-Inf): returns false");
  check(!audio.set_volume(id, -1.0f), "set_volume(-1.0f): returns false");

  // ---------------------------------------------------------------------------
  // EN: set_master_volume() -- same hardening contract, void return; must not crash on hostile
  //     input, and a valid call afterwards still works (state not corrupted by the rejects).
  // PT: set_master_volume() -- mesmo contrato de hardening, retorno void; não pode crashar com
  //     input hostil, e uma chamada válida depois ainda funciona (estado não corrompido pelos
  //     rejects).
  // ---------------------------------------------------------------------------
  audio.set_master_volume(std::numeric_limits<float>::quiet_NaN());
  audio.set_master_volume(-1.0f);
  audio.set_master_volume(0.75f);
  audio.set_master_volume(3.0f); // EN: accepted, clamped internally. PT: aceito, clampeado internamente.

  // ---------------------------------------------------------------------------
  // EN: An unknown SoundId (never returned by load_sound()) must fail every per-sound op.
  // PT: Um SoundId desconhecido (nunca retornado por load_sound()) precisa falhar toda
  //     operação por-som.
  // ---------------------------------------------------------------------------
  const Audio::SoundId unknown_id = 999999;
  check(!audio.play(unknown_id), "play(unknown id): returns false");
  check(!audio.stop(unknown_id), "stop(unknown id): returns false");
  check(!audio.set_volume(unknown_id, 0.5f), "set_volume(unknown id): returns false");
  check(!audio.play(0), "play(0): returns false (0 is reserved as invalid)");

  // EN: AUDIO-F2's new ops on an unknown id / id 0 -- same false/0 contract as every op above.
  // PT: As operações novas do AUDIO-F2 num id desconhecido / id 0 -- mesmo contrato false/0 de
  //     toda operação acima.
  check(!audio.set_looping(unknown_id, true), "set_looping(unknown id, true): returns false");
  check(!audio.play_oneshot(unknown_id), "play_oneshot(unknown id): returns false");
  check(!audio.is_playing(unknown_id), "is_playing(unknown id): returns false");
  check(audio.voice_count(unknown_id) == 0, "voice_count(unknown id): returns 0");
  check(!audio.is_playing(0), "is_playing(0): returns false");
  check(audio.voice_count(0) == 0, "voice_count(0): returns 0");
  check(!audio.set_looping(0, true), "set_looping(0, true): returns false");
  check(!audio.play_oneshot(0), "play_oneshot(0): returns false");

  // ---------------------------------------------------------------------------
  // EN: load_sound() of a path that does not exist on disk returns 0.
  // PT: load_sound() de um caminho que não existe em disco retorna 0.
  // ---------------------------------------------------------------------------
  check(audio.load_sound("/definitely/does/not/exist/glintfx-audio-test.wav") == 0,
        "load_sound(missing path): returns 0");

  audio.stop_all(); // EN: must not crash. PT: não pode crashar.
  audio.shutdown();

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_playback_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_playback_sanity: PASS");
  return 0;
}
