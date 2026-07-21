// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for the well-formed mapping path (A2-GAMEPAD, framework-2D) -- no GL, no
//     window, no RmlUi, no Xvfb, no real device. Exercises, in the order the A2-GAMEPAD plan's
//     section 5.1 test 3 lists them: (1) `build_guid()` against a known real `input_id` (Xbox 360
//     USB), asserted against the literal 32-hex-char string the plan's section 2.4 gives, plus
//     the GUID "2 attempts" match (exact, then CRC-zeroed) `MappingDb::find()` performs; (2)
//     `derive_layout()` against a synthetic capability bitmap, asserting the exact SDL-compatible
//     enumeration ORDER (not just membership); (3) `MappingDb::parse_text()` fed a
//     gamecontrollerdb.txt-format line for the same pad, resolved end to end (a synthetic evdev
//     button-press event resolves to `GamepadButton::South == true` through the parsed mapping);
//     (4) `default_kernel_spec_mapping()` (D5) for a pad no DB entry matches.
// PT: Teste unit puro pro caminho de mapeamento bem-formado (A2-GAMEPAD, framework-2D) -- sem
//     GL, sem janela, sem RmlUi, sem Xvfb, sem device real. Exercita, na ordem em que o teste 3
//     da seção 5.1 do plano A2-GAMEPAD os lista: (1) `build_guid()` contra um `input_id` real
//     conhecido (Xbox 360 USB), verificado contra a string literal de 32 chars hex que a seção
//     2.4 do plano dá, mais o casamento de "2 tentativas" de GUID (exato, depois CRC-zerado) que
//     `MappingDb::find()` faz; (2) `derive_layout()` contra um bitmap de capacidade sintético,
//     verificando a ORDEM exata de enumeração compatível-SDL (não só a presença); (3)
//     `MappingDb::parse_text()` alimentado com uma linha em formato gamecontrollerdb.txt pro
//     mesmo pad, resolvida ponta a ponta (um evento evdev sintético de press de botão resolve
//     pra `GamepadButton::South == true` através do mapeamento parseado); (4)
//     `default_kernel_spec_mapping()` (D5) pra um pad que nenhuma entrada de DB casa.
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_decode.hpp"
#include "gamepad_mapping.hpp"

#include <cmath>
#include <cstdio>

#include <linux/input.h>

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
  // EN: build_guid() -- Xbox 360 USB, name="" (deterministic CRC=0, see gamepad_mapping.hpp's
  //     header comment on why an empty name gives the "CRC already zero" shape by construction).
  // PT: build_guid() -- Xbox 360 USB, name="" (CRC=0 determinístico, ver o comentário de
  //     cabeçalho do gamepad_mapping.hpp sobre por que um nome vazio dá a forma "CRC já zero"
  //     por construção).
  // ---------------------------------------------------------------------------
  input_id xbox360_id{};
  xbox360_id.bustype = 0x03;
  xbox360_id.vendor  = 0x045e;
  xbox360_id.product = 0x028e;
  xbox360_id.version = 0x0114;

  const std::string guid_empty_name = build_guid(xbox360_id, "");
  check(guid_empty_name == "030000005e0400008e02000014010000",
        "build_guid(Xbox 360 USB, name=\"\"): matches the plan's literal expected string");

  // EN: A non-empty name gives a non-zero CRC (bytes 2-3 differ from the DB's zeroed shape) --
  //     MappingDb::find() must still resolve it via the crc-zeroed fallback attempt.
  // PT: Um nome não-vazio dá um CRC não-zero (bytes 2-3 diferentes da forma zerada do DB) --
  //     MappingDb::find() ainda precisa resolvê-lo via a tentativa de fallback crc-zerado.
  const std::string guid_real_name = build_guid(xbox360_id, "Xbox 360 Wireless Receiver");
  check(guid_real_name != guid_empty_name,
        "build_guid() with a non-empty name produces a DIFFERENT guid (non-zero CRC)");

  // ---------------------------------------------------------------------------
  // EN: derive_layout() -- synthetic capability bitmap, exact enumeration ORDER.
  // PT: derive_layout() -- bitmap de capacidade sintético, ORDEM de enumeração exata.
  // ---------------------------------------------------------------------------
  {
    std::array<bool, static_cast<std::size_t>(KEY_MAX) + 1> key_bits{};
    key_bits[BTN_SOUTH] = true; // 0x130
    key_bits[BTN_EAST]  = true; // 0x131
    key_bits[0x05]      = true; // EN: a stray LOW-range code -- must enumerate AFTER the joystick range. PT: um código de faixa BAIXA perdido -- precisa enumerar DEPOIS da faixa de joystick.

    std::array<bool, static_cast<std::size_t>(ABS_MAX) + 1> abs_bits{};
    abs_bits[ABS_X]      = true; // 0x00
    abs_bits[ABS_RX]     = true; // 0x03
    abs_bits[ABS_HAT0X]  = true; // 0x10 -- must be EXCLUDED from axis_codes.
    abs_bits[ABS_HAT0Y]  = true; // 0x11 -- must be EXCLUDED from axis_codes.

    const JoystickLayout layout = derive_layout(key_bits, abs_bits);

    check(layout.button_codes.size() == 3, "derive_layout: 3 buttons enumerated");
    if (layout.button_codes.size() == 3) {
      check(layout.button_codes[0] == BTN_SOUTH, "derive_layout: button index 0 = BTN_SOUTH (high range first)");
      check(layout.button_codes[1] == BTN_EAST, "derive_layout: button index 1 = BTN_EAST (high range, ascending)");
      check(layout.button_codes[2] == 0x05, "derive_layout: button index 2 = the stray low-range code (enumerated LAST)");
    }

    check(layout.axis_codes.size() == 2, "derive_layout: 2 axes enumerated (hats excluded)");
    if (layout.axis_codes.size() == 2) {
      check(layout.axis_codes[0] == ABS_X, "derive_layout: axis index 0 = ABS_X");
      check(layout.axis_codes[1] == ABS_RX, "derive_layout: axis index 1 = ABS_RX");
    }

    check(layout.hat_pairs.size() == 1, "derive_layout: 1 hat pair enumerated");
    if (layout.hat_pairs.size() == 1) {
      check(layout.hat_pairs[0].first == ABS_HAT0X && layout.hat_pairs[0].second == ABS_HAT0Y,
            "derive_layout: hat 0 = {ABS_HAT0X, ABS_HAT0Y}");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: MappingDb::parse_text() + end-to-end resolution -- a representative Xbox-360-shaped
  //     gamecontrollerdb.txt line (this module's own vocabulary, not the vendored DB Agent C
  //     ships separately -- see this file's own header comment), a matching synthetic
  //     capability bitmap, and a synthetic evdev button-press event resolving through the parsed
  //     mapping to GamepadButton::South == true.
  // PT: MappingDb::parse_text() + resolução ponta a ponta -- uma linha gamecontrollerdb.txt no
  //     formato do Xbox 360 (vocabulário próprio deste módulo, não o DB vendorizado que o
  //     Agente C entrega separadamente -- ver o próprio comentário de cabeçalho deste arquivo),
  //     um bitmap de capacidade sintético correspondente, e um evento evdev sintético de press
  //     de botão resolvendo através do mapeamento parseado pra GamepadButton::South == true.
  // ---------------------------------------------------------------------------
  {
    const std::string db_line =
        guid_empty_name +
        ",Xbox 360 Controller,"
        "a:b0,b:b1,x:b2,y:b3,leftshoulder:b4,rightshoulder:b5,back:b6,start:b7,guide:b8,"
        "leftstick:b9,rightstick:b10,"
        "leftx:a0,lefty:a1,lefttrigger:a2,rightx:a3,righty:a4,righttrigger:a5,"
        "dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,"
        "platform:Linux,";

    MappingDb db;
    const int added = db.parse_text(db_line.c_str());
    check(added == 1, "parse_text: 1 entry added for the well-formed Xbox 360 line");

    const GamepadMapping* mapping_exact = db.find(guid_empty_name);
    check(mapping_exact != nullptr, "find(): exact-GUID match hits");

    // EN: The crc-zeroed fallback: a real device's honestly-computed (non-zero-CRC) GUID must
    //     ALSO resolve to this same DB entry.
    // PT: O fallback crc-zerado: o GUID honestamente calculado (CRC não-zero) de um device real
    //     TAMBÉM precisa resolver pra esta mesma entrada de DB.
    const GamepadMapping* mapping_fallback = db.find(guid_real_name);
    check(mapping_fallback != nullptr, "find(): crc-zeroed fallback match hits for a real (non-zero-CRC) device GUID");
    check(mapping_fallback == mapping_exact, "find(): both attempts resolve to the SAME mapping entry");

    check(db.find("not-a-valid-guid") == nullptr, "find(): malformed guid string returns nullptr");

    if (mapping_exact != nullptr) {
      std::array<bool, static_cast<std::size_t>(KEY_MAX) + 1> key_bits{};
      key_bits[BTN_SOUTH]  = true;
      key_bits[BTN_EAST]   = true;
      key_bits[BTN_NORTH]  = true;
      key_bits[BTN_WEST]   = true;
      key_bits[BTN_TL]     = true;
      key_bits[BTN_TR]     = true;
      key_bits[BTN_SELECT] = true;
      key_bits[BTN_START]  = true;
      key_bits[BTN_MODE]   = true;
      key_bits[BTN_THUMBL] = true;
      key_bits[BTN_THUMBR] = true;

      std::array<bool, static_cast<std::size_t>(ABS_MAX) + 1> abs_bits{};
      abs_bits[ABS_X]     = true;
      abs_bits[ABS_Y]     = true;
      abs_bits[ABS_Z]     = true;
      abs_bits[ABS_RX]    = true;
      abs_bits[ABS_RY]    = true;
      abs_bits[ABS_RZ]    = true;
      abs_bits[ABS_HAT0X] = true;
      abs_bits[ABS_HAT0Y] = true;

      const JoystickLayout layout = derive_layout(key_bits, abs_bits);

      GamepadDecoder decoder;
      check(!resolve_button(*mapping_exact, layout, decoder, GamepadButton::South),
            "before any event: South resolves false");

      std::vector<unsigned char> press;
      press.insert(press.end(), 16, 0);
      press.push_back(static_cast<unsigned char>(EV_KEY & 0xFF));
      press.push_back(static_cast<unsigned char>((EV_KEY >> 8) & 0xFF));
      press.push_back(static_cast<unsigned char>(BTN_SOUTH & 0xFF));
      press.push_back(static_cast<unsigned char>((BTN_SOUTH >> 8) & 0xFF));
      press.push_back(1);
      press.push_back(0);
      press.push_back(0);
      press.push_back(0);
      decoder.feed_bytes(press.data(), press.size());

      check(resolve_button(*mapping_exact, layout, decoder, GamepadButton::South),
            "end-to-end: BTN_SOUTH press resolves to GamepadButton::South == true");
      check(!resolve_button(*mapping_exact, layout, decoder, GamepadButton::East),
            "end-to-end: East stays false (was never pressed)");

      // EN: Dpad via hat, through the parsed "dpup:h0.1" entry.
      // PT: Dpad via hat, através da entrada parseada "dpup:h0.1".
      EvAbsInfo hat_info; // EN: hats need no configure_abs() -- see gamepad_decode.hpp. PT: hats não precisam de configure_abs() -- ver gamepad_decode.hpp.
      (void)hat_info;
      std::vector<unsigned char> hat_up;
      hat_up.insert(hat_up.end(), 16, 0);
      hat_up.push_back(static_cast<unsigned char>(EV_ABS & 0xFF));
      hat_up.push_back(static_cast<unsigned char>((EV_ABS >> 8) & 0xFF));
      hat_up.push_back(static_cast<unsigned char>(ABS_HAT0Y & 0xFF));
      hat_up.push_back(static_cast<unsigned char>((ABS_HAT0Y >> 8) & 0xFF));
      hat_up.push_back(0xFF); // EN: -1 LE (int32). PT: -1 LE (int32).
      hat_up.push_back(0xFF);
      hat_up.push_back(0xFF);
      hat_up.push_back(0xFF);
      decoder.feed_bytes(hat_up.data(), hat_up.size());
      check(resolve_button(*mapping_exact, layout, decoder, GamepadButton::DpadUp),
            "end-to-end: hat Y=-1 resolves to GamepadButton::DpadUp == true via parsed \"dpup:h0.1\"");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: default_kernel_spec_mapping() (D5) -- a pad no DB entry matches.
  // PT: default_kernel_spec_mapping() (D5) -- um pad que nenhuma entrada de DB casa.
  // ---------------------------------------------------------------------------
  {
    const GamepadMapping default_mapping   = default_kernel_spec_mapping(/*has_abs_gas=*/false, /*has_abs_brake=*/false);
    const JoystickLayout  unused_layout; // EN: direct_code=true specs never touch this. PT: specs direct_code=true nunca tocam isto.
    GamepadDecoder         decoder;

    std::vector<unsigned char> press;
    press.insert(press.end(), 16, 0);
    press.push_back(static_cast<unsigned char>(EV_KEY & 0xFF));
    press.push_back(static_cast<unsigned char>((EV_KEY >> 8) & 0xFF));
    press.push_back(static_cast<unsigned char>(BTN_SOUTH & 0xFF));
    press.push_back(static_cast<unsigned char>((BTN_SOUTH >> 8) & 0xFF));
    press.push_back(1);
    press.push_back(0);
    press.push_back(0);
    press.push_back(0);
    decoder.feed_bytes(press.data(), press.size());

    check(resolve_button(default_mapping, unused_layout, decoder, GamepadButton::South),
          "default_kernel_spec_mapping: BTN_SOUTH press resolves to GamepadButton::South == true");
    check(!resolve_button(default_mapping, unused_layout, decoder, GamepadButton::East),
          "default_kernel_spec_mapping: East stays false");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_mapping_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_mapping_sanity: PASS");
  return 0;
}
