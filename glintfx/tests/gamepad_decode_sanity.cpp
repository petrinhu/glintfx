// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for GamepadDecoder's well-formed decode path (A2-GAMEPAD, framework-2D) --
//     no GL, no window, no RmlUi, no Xvfb (this decoder is fed synthetic byte buffers only, see
//     gamepad_decode.hpp's own header comment). Exercises: a button press/release, autorepeat
//     (value=2) NOT changing button state, an analog stick axis normalized via a real
//     `-32768..32767` absinfo range, an analog trigger axis normalized via a real `0..255` range,
//     the D-pad hat with `-1/0/+1` values, `SYN_DROPPED`'s sticky flag, and -- the point of this
//     whole file per the A2-GAMEPAD plan section 5.1 -- that reassembly is INDEPENDENT of how the
//     caller chunks the byte stream: the exact same sequence of events, fed as one call, as 1
//     byte at a time, and as 7+17-byte pieces, must all decode to the identical final state.
// PT: Teste unit puro para o caminho de decode bem-formado do GamepadDecoder (A2-GAMEPAD,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (este decoder só é alimentado com
//     buffers de bytes sintéticos, ver o próprio comentário de cabeçalho do gamepad_decode.hpp).
//     Exercita: press/release de botão, autorepeat (value=2) NÃO mudando o estado do botão, um
//     eixo de stick analógico normalizado via uma faixa absinfo real `-32768..32767`, um eixo de
//     gatilho analógico normalizado via uma faixa real `0..255`, o hat do D-pad com valores
//     `-1/0/+1`, a flag pegajosa do `SYN_DROPPED`, e -- o ponto deste arquivo inteiro conforme a
//     seção 5.1 do plano A2-GAMEPAD -- que o reagrupamento é INDEPENDENTE de como o chamador
//     fatia o stream de bytes: a mesma sequência exata de eventos, alimentada como uma única
//     chamada, como 1 byte por vez, e como pedaços de 7+17 bytes, precisa decodificar pro mesmo
//     estado final idêntico.
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_decode.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
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

void check_near(float actual, float expected, float tolerance, const char* what) {
  if (std::fabs(actual - expected) > tolerance) {
    std::fprintf(stderr, "FAIL: %s (actual=%f expected=%f tolerance=%f)\n", what, static_cast<double>(actual),
                 static_cast<double>(expected), static_cast<double>(tolerance));
    ++g_failures;
  }
}

// EN: Appends one 24-byte input_event (16-byte zeroed timeval + type/code/value) to `buf`.
// PT: Anexa um input_event de 24 bytes (timeval zerado de 16 bytes + type/code/value) a `buf`.
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
  // EN: Button press/release + autorepeat no-op.
  // PT: Press/release de botão + autorepeat no-op.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    check(!decoder.key_state(BTN_SOUTH), "fresh decoder: BTN_SOUTH starts released");

    std::vector<unsigned char> press;
    append_event(press, EV_KEY, BTN_SOUTH, 1);
    decoder.feed_bytes(press.data(), press.size());
    check(decoder.key_state(BTN_SOUTH), "EV_KEY value=1: BTN_SOUTH pressed");

    std::vector<unsigned char> repeat;
    append_event(repeat, EV_KEY, BTN_SOUTH, 2); // EN: autorepeat. PT: autorepeat.
    decoder.feed_bytes(repeat.data(), repeat.size());
    check(decoder.key_state(BTN_SOUTH), "EV_KEY value=2 (autorepeat): state unchanged (still pressed)");

    std::vector<unsigned char> release;
    append_event(release, EV_KEY, BTN_SOUTH, 0);
    decoder.feed_bytes(release.data(), release.size());
    check(!decoder.key_state(BTN_SOUTH), "EV_KEY value=0: BTN_SOUTH released");
  }

  // ---------------------------------------------------------------------------
  // EN: Analog stick axis, real -32768..32767 absinfo range.
  // PT: Eixo de stick analógico, faixa absinfo real -32768..32767.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    EvAbsInfo      stick_info;
    stick_info.min = -32768;
    stick_info.max = 32767;
    decoder.configure_abs(ABS_X, stick_info);
    check(decoder.abs_is_signed(ABS_X), "stick range (min<0): abs_is_signed true");

    std::vector<unsigned char> centered;
    append_event(centered, EV_ABS, ABS_X, 0);
    decoder.feed_bytes(centered.data(), centered.size());
    check_near(decoder.abs_normalized(ABS_X), 0.0f, 0.01f, "stick centered (value=0): normalized ~ 0.0");

    std::vector<unsigned char> full_positive;
    append_event(full_positive, EV_ABS, ABS_X, 32767);
    decoder.feed_bytes(full_positive.data(), full_positive.size());
    check_near(decoder.abs_normalized(ABS_X), 1.0f, 0.01f, "stick max (value=32767): normalized ~ 1.0");

    std::vector<unsigned char> full_negative;
    append_event(full_negative, EV_ABS, ABS_X, -32768);
    decoder.feed_bytes(full_negative.data(), full_negative.size());
    check_near(decoder.abs_normalized(ABS_X), -1.0f, 0.01f, "stick min (value=-32768): normalized ~ -1.0");
  }

  // ---------------------------------------------------------------------------
  // EN: Analog trigger axis, real 0..255 absinfo range.
  // PT: Eixo de gatilho analógico, faixa absinfo real 0..255.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    EvAbsInfo      trigger_info;
    trigger_info.min = 0;
    trigger_info.max = 255;
    decoder.configure_abs(ABS_GAS, trigger_info);
    check(!decoder.abs_is_signed(ABS_GAS), "trigger range (min>=0): abs_is_signed false");

    std::vector<unsigned char> released;
    append_event(released, EV_ABS, ABS_GAS, 0);
    decoder.feed_bytes(released.data(), released.size());
    check_near(decoder.abs_normalized(ABS_GAS), 0.0f, 0.01f, "trigger released (value=0): normalized ~ 0.0");

    std::vector<unsigned char> half;
    append_event(half, EV_ABS, ABS_GAS, 128);
    decoder.feed_bytes(half.data(), half.size());
    check_near(decoder.abs_normalized(ABS_GAS), 128.0f / 255.0f, 0.01f, "trigger half (value=128): normalized ~ 0.5");

    std::vector<unsigned char> full;
    append_event(full, EV_ABS, ABS_GAS, 255);
    decoder.feed_bytes(full.data(), full.size());
    check_near(decoder.abs_normalized(ABS_GAS), 1.0f, 0.01f, "trigger full (value=255): normalized ~ 1.0");
  }

  // ---------------------------------------------------------------------------
  // EN: D-pad hat, -1/0/+1 values.
  // PT: Hat do D-pad, valores -1/0/+1.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    check(!decoder.dpad_up() && !decoder.dpad_down() && !decoder.dpad_left() && !decoder.dpad_right(),
          "fresh decoder: no dpad direction pressed");

    std::vector<unsigned char> left;
    append_event(left, EV_ABS, ABS_HAT0X, -1);
    decoder.feed_bytes(left.data(), left.size());
    check(decoder.dpad_left() && !decoder.dpad_right(), "hat X=-1: dpad_left true, dpad_right false");

    std::vector<unsigned char> right;
    append_event(right, EV_ABS, ABS_HAT0X, 1);
    decoder.feed_bytes(right.data(), right.size());
    check(!decoder.dpad_left() && decoder.dpad_right(), "hat X=+1: dpad_right true, dpad_left false");

    std::vector<unsigned char> center_x;
    append_event(center_x, EV_ABS, ABS_HAT0X, 0);
    decoder.feed_bytes(center_x.data(), center_x.size());
    check(!decoder.dpad_left() && !decoder.dpad_right(), "hat X=0: neither left nor right");

    std::vector<unsigned char> up;
    append_event(up, EV_ABS, ABS_HAT0Y, -1);
    decoder.feed_bytes(up.data(), up.size());
    check(decoder.dpad_up() && !decoder.dpad_down(), "hat Y=-1: dpad_up true, dpad_down false");

    std::vector<unsigned char> down;
    append_event(down, EV_ABS, ABS_HAT0Y, 1);
    decoder.feed_bytes(down.data(), down.size());
    check(!decoder.dpad_up() && decoder.dpad_down(), "hat Y=+1: dpad_down true, dpad_up false");
  }

  // ---------------------------------------------------------------------------
  // EN: SYN_DROPPED sticky flag.
  // PT: Flag pegajosa do SYN_DROPPED.
  // ---------------------------------------------------------------------------
  {
    GamepadDecoder decoder;
    check(!decoder.sync_lost(), "fresh decoder: sync_lost starts false");

    std::vector<unsigned char> dropped;
    append_event(dropped, EV_SYN, SYN_DROPPED, 0);
    decoder.feed_bytes(dropped.data(), dropped.size());
    check(decoder.sync_lost(), "EV_SYN/SYN_DROPPED: sync_lost true");

    decoder.clear_sync_lost();
    check(!decoder.sync_lost(), "clear_sync_lost(): sync_lost false again");

    // EN: SYN_REPORT is a documented no-op -- must NOT set the flag.
    // PT: SYN_REPORT é no-op documentado -- NÃO pode setar a flag.
    std::vector<unsigned char> report;
    append_event(report, EV_SYN, SYN_REPORT, 0);
    decoder.feed_bytes(report.data(), report.size());
    check(!decoder.sync_lost(), "EV_SYN/SYN_REPORT: sync_lost stays false");
  }

  // ---------------------------------------------------------------------------
  // EN: Chunking independence -- the same 3-event sequence (press A, release A, press B),
  //     decoded 3 different ways (one call, 1 byte at a time, 7+17-byte pieces), must produce
  //     the identical final state.
  // PT: Independência de fatiamento -- a mesma sequência de 3 eventos (press A, release A,
  //     press B), decodificada de 3 formas diferentes (uma chamada, 1 byte por vez, pedaços de
  //     7+17 bytes), precisa produzir o mesmo estado final idêntico.
  // ---------------------------------------------------------------------------
  {
    std::vector<unsigned char> sequence;
    append_event(sequence, EV_KEY, BTN_SOUTH, 1);
    append_event(sequence, EV_KEY, BTN_SOUTH, 0);
    append_event(sequence, EV_KEY, BTN_EAST, 1);

    GamepadDecoder one_call;
    one_call.feed_bytes(sequence.data(), sequence.size());

    GamepadDecoder byte_at_a_time;
    for (unsigned char b : sequence) {
      byte_at_a_time.feed_bytes(&b, 1);
    }

    GamepadDecoder seven_then_seventeen;
    // EN: A single event is exactly 24 bytes -- feed the first event as 7+17 bytes (crossing the
    //     boundary mid-event), then the rest as a single call.
    // PT: Um único evento tem exatamente 24 bytes -- alimenta o primeiro evento como 7+17 bytes
    //     (cruzando a fronteira no meio do evento), depois o resto como uma única chamada.
    seven_then_seventeen.feed_bytes(sequence.data(), 7);
    seven_then_seventeen.feed_bytes(sequence.data() + 7, 17);
    seven_then_seventeen.feed_bytes(sequence.data() + 24, sequence.size() - 24);

    check(!one_call.key_state(BTN_SOUTH) && one_call.key_state(BTN_EAST), "one_call: final state correct");
    check(!byte_at_a_time.key_state(BTN_SOUTH) && byte_at_a_time.key_state(BTN_EAST),
          "byte_at_a_time: final state matches one_call");
    check(!seven_then_seventeen.key_state(BTN_SOUTH) && seven_then_seventeen.key_state(BTN_EAST),
          "seven_then_seventeen: final state matches one_call");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_decode_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_decode_sanity: PASS");
  return 0;
}
