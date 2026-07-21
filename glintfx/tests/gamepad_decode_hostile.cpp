// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for GamepadDecoder's hostile-input robustness (A2-GAMEPAD, framework-2D) --
//     no GL, no window, no RmlUi, no Xvfb. Same discipline as audio_hostile_sanity.cpp/
//     i18n_catalog_sanity.cpp's malformed-input coverage (auditoria-dominó: the same robustness
//     class applies to every module that parses attacker-reachable input -- and evdev bytes from
//     `/dev/input/eventN` are about as attacker-adjacent as input gets, any process with access
//     to the node can write plausible-looking garbage). Every case below must never crash/read
//     out of bounds -- if this binary segfaults/aborts, ctest reports it as a failure on its own,
//     no assertion needed to detect that class of bug. Where a specific safe VALUE is guaranteed
//     (e.g. a degenerate axis range normalizing to exactly 0.0f), it is also asserted.
// PT: Teste unit puro pra robustez a input hostil do GamepadDecoder (A2-GAMEPAD, framework-2D)
//     -- sem GL, sem janela, sem RmlUi, sem Xvfb. Mesma disciplina da cobertura de input
//     malformado do audio_hostile_sanity.cpp/i18n_catalog_sanity.cpp (auditoria-dominó: a mesma
//     classe de robustez se aplica a todo módulo que faz parse de input alcançável por atacante
//     -- e bytes evdev de `/dev/input/eventN` são tão próximos de "alcançável por atacante"
//     quanto input físico costuma chegar, qualquer processo com acesso ao node pode escrever
//     lixo plausível). Todo caso abaixo precisa nunca crashar/ler fora dos limites -- se este
//     binário segfaultar/abortar, o ctest já reporta isso como falha por conta própria, nenhuma
//     asserção extra é necessária pra detectar essa classe de bug. Onde um VALOR seguro
//     específico é garantido (ex.: uma faixa de eixo degenerada normalizando pra exatamente
//     0.0f), ele também é verificado.
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_decode.hpp"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <random>
#include <vector>

#include <linux/input.h>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void append_event(std::vector<unsigned char>& buf, std::uint16_t type, std::uint16_t code, std::int32_t value) {
  buf.insert(buf.end(), 16, 0);
  buf.push_back(static_cast<unsigned char>(type & 0xFF));
  buf.push_back(static_cast<unsigned char>((type >> 8) & 0xFF));
  buf.push_back(static_cast<unsigned char>(code & 0xFF));
  buf.push_back(static_cast<unsigned char>((code >> 8) & 0xFF));
  const std::uint32_t uvalue = static_cast<std::uint32_t>(value);
  buf.push_back(static_cast<unsigned char>(uvalue & 0xFF));
  buf.push_back(static_cast<unsigned char>((uvalue >> 8) & 0xFF));
  buf.push_back(static_cast<unsigned char>((uvalue >> 16) & 0xFF));
  buf.push_back(static_cast<unsigned char>((uvalue >> 24) & 0xFF));
}

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: Random garbage bytes, several sizes, several seeds -- never crashes.
  // PT: Bytes de lixo aleatório, vários tamanhos, várias seeds -- nunca crasha.
  // ---------------------------------------------------------------------------
  {
    for (unsigned seed : {0xA2AD10u, 0xDEADBEEFu, 1u}) {
      std::mt19937                       rng(seed);
      std::uniform_int_distribution<int> byte_dist(0, 255);
      GamepadDecoder                     decoder;
      std::vector<unsigned char>         garbage(4096);
      for (auto& b : garbage) {
        b = static_cast<unsigned char>(byte_dist(rng));
      }
      decoder.feed_bytes(garbage.data(), garbage.size());
      // EN: Reaching this line without a crash IS the assertion for this case.
      // PT: Chegar nesta linha sem crashar JÁ É a asserção deste caso.
    }
    std::puts("gamepad_decode_hostile: random garbage did not crash");
  }

  // ---------------------------------------------------------------------------
  // EN: Event truncated at the very end of the buffer (a well-formed event followed by a partial
  //     one) -- the partial tail must stay buffered, never parsed as garbage, never crash.
  // PT: Evento truncado bem no fim do buffer (um evento bem-formado seguido de um parcial) -- a
  //     cauda parcial precisa ficar bufferizada, nunca parseada como lixo, nunca crashar.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder              decoder;
    std::vector<unsigned char>  buffer;
    append_event(buffer, EV_KEY, BTN_SOUTH, 1);
    buffer.insert(buffer.end(), 10, 0xAB); // EN: 10 trailing bytes -- less than a full event. PT: 10 bytes finais -- menos que um evento completo.
    decoder.feed_bytes(buffer.data(), buffer.size());
    check(decoder.key_state(BTN_SOUTH), "truncated tail: the well-formed event before it still decoded");
  }

  // ---------------------------------------------------------------------------
  // EN: type/code far out of the valid evdev range (0xFFFF) -- must be ignored, never indexed.
  // PT: type/code bem fora da faixa evdev válida (0xFFFF) -- precisa ser ignorado, nunca indexado.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder              decoder;
    std::vector<unsigned char>  bad_type;
    append_event(bad_type, 0xFFFF, 0xFFFF, 1);
    decoder.feed_bytes(bad_type.data(), bad_type.size());

    std::vector<unsigned char> bad_key_code;
    append_event(bad_key_code, EV_KEY, 0xFFFF, 1);
    decoder.feed_bytes(bad_key_code.data(), bad_key_code.size());
    check(!decoder.key_state(0xFFFF), "EV_KEY code=0xFFFF (>= KEY_MAX): never indexed, reads back false");

    std::vector<unsigned char> bad_abs_code;
    append_event(bad_abs_code, EV_ABS, 0xFFFF, 12345);
    decoder.feed_bytes(bad_abs_code.data(), bad_abs_code.size());
    check(decoder.abs_value(0xFFFF) == 0, "EV_ABS code=0xFFFF (>= ABS_MAX): never indexed, reads back 0");
  }

  // ---------------------------------------------------------------------------
  // EN: value = INT32_MIN/INT32_MAX for both EV_KEY (any value other than 0/1/2 is a no-op) and
  //     EV_ABS (extreme raw value, must not overflow/crash the normalization math).
  // PT: value = INT32_MIN/INT32_MAX tanto pra EV_KEY (qualquer valor diferente de 0/1/2 é no-op)
  //     quanto pra EV_ABS (valor cru extremo, não pode estourar/crashar a matemática de
  //     normalização).
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    EvAbsInfo      stick_info;
    stick_info.min = -32768;
    stick_info.max = 32767;
    decoder.configure_abs(ABS_X, stick_info);

    std::vector<unsigned char> key_min;
    append_event(key_min, EV_KEY, BTN_SOUTH, std::numeric_limits<std::int32_t>::min());
    decoder.feed_bytes(key_min.data(), key_min.size());
    check(!decoder.key_state(BTN_SOUTH), "EV_KEY value=INT32_MIN: no-op, still released");

    std::vector<unsigned char> key_max;
    append_event(key_max, EV_KEY, BTN_SOUTH, std::numeric_limits<std::int32_t>::max());
    decoder.feed_bytes(key_max.data(), key_max.size());
    check(!decoder.key_state(BTN_SOUTH), "EV_KEY value=INT32_MAX: no-op, still released");

    std::vector<unsigned char> abs_min;
    append_event(abs_min, EV_ABS, ABS_X, std::numeric_limits<std::int32_t>::min());
    decoder.feed_bytes(abs_min.data(), abs_min.size());
    const float normalized_min = decoder.abs_normalized(ABS_X);
    check(normalized_min >= -1.0f && normalized_min <= 1.0f, "EV_ABS value=INT32_MIN: normalized stays within [-1,1]");

    std::vector<unsigned char> abs_max;
    append_event(abs_max, EV_ABS, ABS_X, std::numeric_limits<std::int32_t>::max());
    decoder.feed_bytes(abs_max.data(), abs_max.size());
    const float normalized_max = decoder.abs_normalized(ABS_X);
    check(normalized_max >= -1.0f && normalized_max <= 1.0f, "EV_ABS value=INT32_MAX: normalized stays within [-1,1]");
  }

  // ---------------------------------------------------------------------------
  // EN: Degenerate absinfo ranges -- min==max and min>max -- must normalize to exactly 0.0f,
  //     never divide by zero.
  // PT: Faixas absinfo degeneradas -- min==max e min>max -- precisam normalizar pra exatamente
  //     0.0f, nunca dividir por zero.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;

    EvAbsInfo equal_info;
    equal_info.min = 100;
    equal_info.max = 100;
    decoder.configure_abs(ABS_RX, equal_info);
    std::vector<unsigned char> equal_event;
    append_event(equal_event, EV_ABS, ABS_RX, 100);
    decoder.feed_bytes(equal_event.data(), equal_event.size());
    check(decoder.abs_normalized(ABS_RX) == 0.0f, "min==max: normalized is exactly 0.0f");

    EvAbsInfo inverted_info;
    inverted_info.min = 500;
    inverted_info.max = -500;
    decoder.configure_abs(ABS_RY, inverted_info);
    std::vector<unsigned char> inverted_event;
    append_event(inverted_event, EV_ABS, ABS_RY, 0);
    decoder.feed_bytes(inverted_event.data(), inverted_event.size());
    check(decoder.abs_normalized(ABS_RY) == 0.0f, "min>max: normalized is exactly 0.0f");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_decode_hostile: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_decode_hostile: PASS");
  return 0;
}
