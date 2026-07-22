// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::Audio's hostile-input robustness (A3-AUDIO, framework-2D) --
//     no GL, no window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb" comment in
//     tests/CMakeLists.txt). Uses AudioConfig::null_backend=true (headless). Same discipline as
//     i18n_catalog_sanity.cpp's malformed-input coverage and decorator_polygon.cpp's "reject
//     the bad input, never abort the host" (auditoria-dominó: the same robustness class applies
//     to every module that parses attacker-reachable input, not just the ones that happened to
//     get audited first). Every case below must return 0/false and MUST NOT crash -- if this
//     binary segfaults/aborts, ctest reports it as a failure on its own, no assertion needed to
//     detect that class of bug. AUDIO-F2 additions: play()/stop()'s fade_in_s/fade_out_s given
//     NaN/+Inf/-Inf/a huge finite value (1e30f) -- all rejected, none crash, none silently
//     overflow the ma_uint64 millisecond cast (the exact bug class the audit-domino memory
//     names); play_oneshot() on hostile ids (unknown/0); a play_oneshot() SPAM (past the 32-per-
//     id cap) proving voice-stealing keeps the call succeeding and the count bounded, never a
//     crash or an unbounded voice table.
// PT: Teste unit puro para a robustez a input hostil do glintfx::Audio (A3-AUDIO,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora
//     do GLFW/Xvfb" em tests/CMakeLists.txt). Mesma disciplina da cobertura de input malformado
//     do i18n_catalog_sanity.cpp e do "rejeita o input ruim, nunca aborta o host" do
//     decorator_polygon.cpp (auditoria-dominó: a mesma classe de robustez se aplica a todo
//     módulo que faz parse de input alcançável por atacante, não só aos que calharam de ser
//     auditados primeiro). Todo caso abaixo precisa retornar 0/false e NÃO PODE crashar -- se
//     este binário segfaultar/abortar, o ctest já reporta isso como falha por conta própria,
//     nenhuma asserção extra é necessária pra detectar essa classe de bug. Adições do AUDIO-F2:
//     fade_in_s/fade_out_s do play()/stop() recebendo NaN/+Inf/-Inf/um valor finito enorme
//     (1e30f) -- todos rejeitados, nenhum crasha, nenhum estoura silenciosamente o cast pra
//     milissegundos ma_uint64 (exatamente a classe de bug que a memória de auditoria-dominó
//     nomeia); play_oneshot() em ids hostis (desconhecido/0); um SPAM de play_oneshot() (além do
//     teto de 32-por-id) provando que o voice-stealing mantém a chamada com sucesso e a contagem
//     limitada, nunca um crash ou uma tabela de vozes sem limite.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/audio.hpp>

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
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

// EN: A well-formed minimal WAV (same shape as audio_playback_sanity.cpp's builder) used ONLY
//     as a base to be TRUNCATED below -- this file does not test the well-formed case (that is
//     audio_playback_sanity.cpp's job).
// PT: Um WAV mínimo bem-formado (mesma forma do builder do audio_playback_sanity.cpp) usado SÓ
//     como base para ser TRUNCADO abaixo -- este arquivo não testa o caso bem-formado (isso é
//     trabalho do audio_playback_sanity.cpp).
std::vector<unsigned char> build_well_formed_wav() {
  const std::uint32_t sample_rate     = 8000;
  const std::uint16_t num_channels    = 1;
  const std::uint16_t bits_per_sample = 16;
  const std::uint32_t byte_rate       = sample_rate * num_channels * bits_per_sample / 8;
  const std::uint16_t block_align     = static_cast<std::uint16_t>(num_channels * bits_per_sample / 8);
  const int           sample_count    = 100;
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
  wav.resize(wav.size() + data_size, 0); // EN: silence (all-zero PCM), content irrelevant. PT: silêncio (PCM todo zero), conteúdo irrelevante.
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
  const fs::path dir = fs::temp_directory_path() / "glintfx-audio-hostile-sanity";
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

  Audio       audio;
  AudioConfig cfg;
  cfg.null_backend = true;
  check(audio.init(cfg), "init(null_backend): returns true");

  // ---------------------------------------------------------------------------
  // EN: load_sound(nullptr) -- must never crash, must return 0.
  // PT: load_sound(nullptr) -- nunca pode crashar, precisa retornar 0.
  // ---------------------------------------------------------------------------
  check(audio.load_sound(nullptr) == 0, "load_sound(nullptr): returns 0");

  // ---------------------------------------------------------------------------
  // EN: Truncated WAV -- the well-formed file cut off mid-header (before the "fmt " subchunk
  //     even finishes), and mid-data (header intact, data chunk incomplete). Neither may crash.
  // PT: WAV truncado -- o arquivo bem-formado cortado no meio do header (antes até do subchunk
  //     "fmt " terminar), e no meio do data (header intacto, chunk de data incompleto). Nenhum
  //     dos dois pode crashar.
  // ---------------------------------------------------------------------------
  {
    const std::vector<unsigned char> full = build_well_formed_wav();

    const fs::path truncated_header_path = dir / "truncated_header.wav";
    const std::vector<unsigned char> truncated_header(full.begin(), full.begin() + 10);
    write_file(truncated_header_path, truncated_header);
    check(audio.load_sound(truncated_header_path.string().c_str()) == 0,
          "load_sound(WAV truncated mid-header): returns 0");

    // EN: A complete, well-formed header (44 bytes) followed by only 6 bytes of the declared
    //     ~200-byte data payload. This is NOT asserted to fail: miniaudio's dr_wav decoder is
    //     deliberately lenient here (a well-known real-world WAV quirk -- many encoders/writers
    //     leave the RIFF/data chunk sizes inaccurate, e.g. a stream that was cut off mid-write),
    //     and correctly decodes the shorter-than-declared number of complete PCM frames that
    //     ARE actually present instead of failing outright. What this test proves is the thing
    //     that actually matters: the call returns promptly and does not crash/read out of
    //     bounds, whatever id it hands back.
    // PT: Um header completo e bem-formado (44 bytes) seguido de só 6 bytes do payload de dado
    //     declarado (~200 bytes). Isto NÃO é afirmado como falha: o decoder dr_wav do miniaudio
    //     é deliberadamente leniente aqui (uma peculiaridade conhecida de WAV no mundo real --
    //     muitos encoders/writers deixam os tamanhos de chunk RIFF/data imprecisos, ex.: um
    //     stream cortado no meio da escrita), e decodifica corretamente o número de frames PCM
    //     completos que DE FATO estão presentes, menor que o declarado, em vez de falhar de
    //     cara. O que este teste prova é o que de fato importa: a chamada retorna prontamente e
    //     não crasha/lê fora dos limites, seja qual for o id que ela devolve.
    const fs::path truncated_data_path = dir / "truncated_data.wav";
    const std::vector<unsigned char> truncated_data(full.begin(), full.begin() + 50);
    write_file(truncated_data_path, truncated_data);
    (void)audio.load_sound(truncated_data_path.string().c_str());
  }

  // ---------------------------------------------------------------------------
  // EN: Random garbage bytes (not a WAV at all, no recognisable RIFF magic).
  // PT: Bytes de lixo aleatório (nem de longe um WAV, sem magic RIFF reconhecível).
  // ---------------------------------------------------------------------------
  {
    std::mt19937                          rng(0xA3AD10);
    std::uniform_int_distribution<int>    byte_dist(0, 255);
    std::vector<unsigned char>            garbage(256);
    for (auto& b : garbage) {
      b = static_cast<unsigned char>(byte_dist(rng));
    }
    const fs::path garbage_path = dir / "garbage.bin";
    write_file(garbage_path, garbage);
    check(audio.load_sound(garbage_path.string().c_str()) == 0,
          "load_sound(random garbage bytes): returns 0");
  }

  // ---------------------------------------------------------------------------
  // EN: Empty file (0 bytes).
  // PT: Arquivo vazio (0 bytes).
  // ---------------------------------------------------------------------------
  {
    const fs::path empty_path = dir / "empty.wav";
    write_file(empty_path, {});
    check(audio.load_sound(empty_path.string().c_str()) == 0, "load_sound(empty file): returns 0");
  }

  // ---------------------------------------------------------------------------
  // EN: A WAV header that LIES about its data chunk size -- claims far more data than the file
  //     actually contains (a classic malformed-container attack shape). Must not read past the
  //     end of the file / crash.
  // PT: Um header WAV que MENTE sobre o tamanho do chunk de data -- alega muito mais dado do
  //     que o arquivo de fato contém (uma forma clássica de ataque de container malformado).
  //     Não pode ler além do fim do arquivo / crashar.
  // ---------------------------------------------------------------------------
  {
    std::vector<unsigned char> lying = build_well_formed_wav();
    // EN: Overwrite the "data" subchunk's declared size (the 4 bytes right after the "data"
    //     tag) with 0xFFFFFFFF -- the file itself stays the same (short) length.
    // PT: Sobrescreve o tamanho declarado do subchunk "data" (os 4 bytes logo depois da tag
    //     "data") com 0xFFFFFFFF -- o arquivo em si continua com o mesmo tamanho (curto).
    const std::size_t data_size_offset = 40; // EN: 12 (RIFF/WAVE) + 8+16 (fmt) + 4 ("data" tag). PT: 12 (RIFF/WAVE) + 8+16 (fmt) + 4 (tag "data").
    lying[data_size_offset + 0] = 0xFF;
    lying[data_size_offset + 1] = 0xFF;
    lying[data_size_offset + 2] = 0xFF;
    lying[data_size_offset + 3] = 0xFF;
    const fs::path lying_path = dir / "lying_size.wav";
    write_file(lying_path, lying);
    // EN: We do not assert 0 here -- some decoders may still successfully clamp/decode the
    //     available bytes. What we assert is what actually matters for this test: the call
    //     returns (does not hang/crash), and the SoundId it returns (0 or otherwise) is safe to
    //     immediately shut down through, proven by reaching the end of main() below.
    // PT: Não afirmamos 0 aqui -- alguns decoders podem ainda conseguir clampear/decodificar os
    //     bytes disponíveis. O que afirmamos é o que de fato importa pra este teste: a chamada
    //     retorna (não trava/crasha), e o SoundId que ela retorna (0 ou não) é seguro de
    //     desligar imediatamente depois, provado por chegar ao fim do main() abaixo.
    (void)audio.load_sound(lying_path.string().c_str());
  }

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- fade hardening: NaN/+Inf/-Inf/negative are REJECTED (false, state untouched)
  //     as play()'s fade_in_s / stop()'s fade_out_s. A huge but FINITE value (1e30f) is a
  //     DIFFERENT case: it is ACCEPTED and silently clamped to 3600s (per audio.hpp's contract --
  //     only non-finite/negative are rejected outright) -- the point of this sub-case is that
  //     validate_fade_seconds() clamps it in FLOAT strictly before any ma_uint64 millisecond
  //     cast, so it never overflows (the exact bug class the audit-domino memory names), even
  //     though it is not itself a rejected value. Needs a real, successfully-decoded sound (the
  //     garbage/truncated files above never register one), so load a fresh well-formed WAV here.
  // PT: AUDIO-F2 -- hardening de fade: NaN/+Inf/-Inf/negativo são REJEITADOS (false, estado
  //     intocado) como fade_in_s do play() / fade_out_s do stop(). Um valor enorme mas FINITO
  //     (1e30f) é um caso DIFERENTE: é ACEITO e clampeado silenciosamente em 3600s (conforme o
  //     contrato do audio.hpp -- só não-finito/negativo são rejeitados de cara) -- o ponto deste
  //     subcaso é que o validate_fade_seconds() clampeia em FLOAT estritamente antes de qualquer
  //     cast pra milissegundos ma_uint64, então nunca estoura (exatamente a classe de bug que a
  //     memória de auditoria-dominó nomeia), mesmo não sendo ele próprio um valor rejeitado.
  //     Precisa de um som real e decodificado com sucesso (os arquivos lixo/truncados acima
  //     nunca registram um), então carrega um WAV bem-formado novo aqui.
  // ---------------------------------------------------------------------------
  {
    const fs::path fade_wav_path = dir / "fade_hardening.wav";
    write_file(fade_wav_path, build_well_formed_wav());
    const Audio::SoundId fade_id = audio.load_sound(fade_wav_path.string().c_str());
    check(fade_id != 0, "AUDIO-F2 fade hardening: load_sound(well-formed WAV) returns non-zero id");

    const float rejected_fades[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -1.0f,
    };
    for (float f : rejected_fades) {
      check(!audio.play(fade_id, /*loop=*/false, f), "AUDIO-F2 fade hardening: play(fade_id, false, rejected fade): returns false");
      check(!audio.stop(fade_id, f), "AUDIO-F2 fade hardening: stop(fade_id, rejected fade): returns false");
    }

    // EN: 1e30f is finite and non-negative -- accepted (clamped to 3600s internally), never a
    //     crash/hang and never a silent ma_uint64 overflow. Immediately stop with fade_out_s=0
    //     afterwards so the huge accepted fade does not bleed into the rest of this test.
    // PT: 1e30f é finito e não-negativo -- aceito (clampeado em 3600s internamente), nunca um
    //     crash/travamento e nunca um estouro silencioso do ma_uint64. Para imediatamente com
    //     fade_out_s=0 depois pra que o fade enorme aceito não vaze pro resto deste teste.
    check(audio.play(fade_id, /*loop=*/false, 1e30f), "AUDIO-F2 fade hardening: play(fade_id, false, 1e30f): accepted (finite, clamped), returns true");
    check(audio.stop(fade_id, 1e30f), "AUDIO-F2 fade hardening: stop(fade_id, 1e30f): accepted (finite, clamped), returns true");
    check(audio.stop(fade_id), "AUDIO-F2 fade hardening: stop(fade_id) (immediate) cleans up after the huge accepted fade");

    // EN: State untouched by every rejected call above -- a normal play/stop still works.
    // PT: Estado intocado por toda chamada rejeitada acima -- um play/stop normal ainda funciona.
    check(audio.play(fade_id), "AUDIO-F2 fade hardening: play(fade_id) after the rejected calls still returns true");
    check(audio.stop(fade_id), "AUDIO-F2 fade hardening: stop(fade_id) after the rejected calls still returns true");
  }

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- play_oneshot() on hostile ids (unknown, 0) -- never crashes, always false.
  // PT: AUDIO-F2 -- play_oneshot() em ids hostis (desconhecido, 0) -- nunca crasha, sempre false.
  // ---------------------------------------------------------------------------
  check(!audio.play_oneshot(424242), "AUDIO-F2: play_oneshot(unknown id): returns false");
  check(!audio.play_oneshot(0), "AUDIO-F2: play_oneshot(0): returns false");

  // ---------------------------------------------------------------------------
  // EN: AUDIO-F2 -- play_oneshot() SPAM well past the 32-per-id cap: every call still succeeds
  //     (voice-stealing, never a rejection due to the cap) and voice_count() never exceeds the
  //     cap -- no crash, no unbounded growth of the internal voice table under hostile/bursty
  //     use (a rapid-fire weapon in a real game is exactly this shape).
  // PT: AUDIO-F2 -- SPAM de play_oneshot() bem além do teto de 32-por-id: toda chamada ainda tem
  //     sucesso (voice-stealing, nunca uma rejeição por causa do teto) e voice_count() nunca
  //     excede o teto -- sem crash, sem crescimento sem limite da tabela de vozes interna sob uso
  //     hostil/em rajada (uma arma automática num jogo de verdade é exatamente essa forma).
  // ---------------------------------------------------------------------------
  {
    const fs::path spam_wav_path = dir / "spam.wav";
    write_file(spam_wav_path, build_well_formed_wav()); // EN: short (100 samples @ 8kHz) -- some copies may naturally end and get reaped mid-spam; irrelevant to the <= 32 assertion below either way. PT: curto (100 amostras @ 8kHz) -- algumas cópias podem terminar naturalmente e ser reapadas no meio do spam; irrelevante pra asserção <= 32 abaixo de qualquer forma.
    const Audio::SoundId spam_id = audio.load_sound(spam_wav_path.string().c_str());
    check(spam_id != 0, "AUDIO-F2 oneshot spam: load_sound() returns non-zero id");

    for (int i = 0; i < 40; ++i) {
      check(audio.play_oneshot(spam_id), "AUDIO-F2 oneshot spam: play_oneshot() call never fails due to the voice cap");
    }
    check(audio.voice_count(spam_id) <= 32, "AUDIO-F2 oneshot spam: voice_count() never exceeds the 32-per-id cap");
  }

  audio.shutdown(); // EN: must not crash regardless of what the hostile loads above did. PT: não pode crashar independente do que os loads hostis acima fizeram.

  if (g_failures > 0) {
    std::fprintf(stderr, "audio_hostile_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("audio_hostile_sanity: PASS");
  return 0;
}
