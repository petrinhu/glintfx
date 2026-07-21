// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of GUID construction, SDL-compatible layout derivation, the
//     gamecontrollerdb.txt parser, and the default kernel-spec mapping. See gamepad_mapping.hpp's
//     own header comment for the "why" of each piece; comments here focus on the "how".
// PT: Implementação da construção de GUID, derivação de layout compatível-SDL, o parser do
//     gamecontrollerdb.txt, e o mapeamento default kernel-spec. Ver o próprio comentário de
//     cabeçalho do gamepad_mapping.hpp pro "porquê" de cada peça; os comentários aqui focam no
//     "como".
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_mapping.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>

namespace glintfx {

namespace {

// ---------------------------------------------------------------------------
// EN: build_guid() helpers.
// PT: Auxiliares do build_guid().
// ---------------------------------------------------------------------------

// EN: CRC-16/ARC -- polynomial 0xA001 (the bit-reversed/reflected form of the "CRC-16-IBM"
//     polynomial 0x8005), initial value 0, no final XOR. A well-known, publicly documented
//     bitwise algorithm (reimplemented clean-room from the well-known reflected-CRC
//     construction, not copied from any specific codebase). An EMPTY `data`: the loop body never
//     runs, so this returns exactly `0` -- see gamepad_mapping.hpp's header comment on why that
//     matters for build_guid(id, "").
// PT: CRC-16/ARC -- polinômio 0xA001 (a forma bit-invertida/refletida do polinômio "CRC-16-IBM"
//     0x8005), valor inicial 0, sem XOR final. Um algoritmo bitwise bem conhecido e
//     publicamente documentado (reimplementado clean-room a partir da construção de CRC
//     refletido bem conhecida, não copiado de nenhuma base de código específica). Um `data`
//     VAZIO: o corpo do laço nunca roda, então isto retorna exatamente `0` -- ver o comentário
//     de cabeçalho do gamepad_mapping.hpp sobre por que isso importa pro build_guid(id, "").
std::uint16_t crc16_arc(const std::string& data) {
  std::uint16_t crc = 0;
  for (unsigned char byte : data) {
    crc = static_cast<std::uint16_t>(crc ^ byte);
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x0001) {
        crc = static_cast<std::uint16_t>((crc >> 1) ^ 0xA001);
      } else {
        crc = static_cast<std::uint16_t>(crc >> 1);
      }
    }
  }
  return crc;
}

void append_u16_le(std::string& hex, std::uint16_t v) {
  char buf[5];
  // EN: Low byte first, high byte second -- little-endian byte order, then each byte hex-encoded
  //     (2 chars). std::snprintf's return value is not checked: the fixed 3-char format
  //     ("%02x") into a 5-byte buffer can never fail/truncate for a std::uint8_t input.
  // PT: Byte baixo primeiro, byte alto depois -- ordem little-endian, cada byte então
  //     hex-codificado (2 chars). O valor de retorno do std::snprintf não é checado: o formato
  //     fixo de 3 chars ("%02x") num buffer de 5 bytes nunca pode falhar/truncar pra um input
  //     std::uint8_t.
  std::snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned>(v & 0xFF));
  hex += buf;
  std::snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned>((v >> 8) & 0xFF));
  hex += buf;
}

bool is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

std::string to_lower_ascii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  });
  return s;
}

// ---------------------------------------------------------------------------
// EN: gamecontrollerdb.txt parser helpers.
// PT: Auxiliares do parser do gamecontrollerdb.txt.
// ---------------------------------------------------------------------------

// EN: A single well-formed-or-not `field:value` control assignment, parsed from the RIGHT-HAND
//     side of a DB entry field (the part after ':', e.g. "b0", "+a5~", "h0.1"). Returns
//     `MappingKind::None` for anything that does not parse cleanly -- the caller then skips the
//     WHOLE FIELD (fail-high per-field, never per-line, per the plan's "campo desconhecido:
//     ignora e segue").
// PT: Uma única atribuição de controle `field:value` bem-ou-mal-formada, parseada do lado
//     DIREITO de um campo de entrada do DB (a parte depois de ':', ex.: "b0", "+a5~", "h0.1").
//     Retorna `MappingKind::None` pra qualquer coisa que não faça parse limpo -- o chamador então
//     pula o CAMPO INTEIRO (fail-high por-campo, nunca por-linha, conforme o "campo desconhecido:
//     ignora e segue" do plano).
MappingSpec parse_value_spec(std::string value) {
  MappingSpec spec;

  if (!value.empty() && value.back() == '~') {
    spec.inverted = true;
    value.pop_back();
  }
  if (!value.empty() && (value.front() == '+' || value.front() == '-')) {
    spec.half = (value.front() == '+') ? 1 : -1;
    value.erase(value.begin());
  }
  if (value.empty()) {
    return MappingSpec{}; // EN: nothing left -- malformed. PT: nada sobrou -- malformado.
  }

  const char kind_char = value.front();
  const std::string rest = value.substr(1);

  if (kind_char == 'b' || kind_char == 'a') {
    if (rest.empty()) return MappingSpec{};
    int index = 0;
    const auto result = std::from_chars(rest.data(), rest.data() + rest.size(), index);
    if (result.ec != std::errc{} || result.ptr != rest.data() + rest.size() || index < 0) {
      return MappingSpec{}; // EN: trailing garbage, empty, or negative index -- malformed.
                             // PT: lixo à direita, vazio, ou índice negativo -- malformado.
    }
    spec.kind        = (kind_char == 'b') ? MappingKind::Button : MappingKind::Axis;
    spec.index       = index;
    spec.direct_code = false;
    return spec;
  }

  if (kind_char == 'h') {
    const std::size_t dot = rest.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 >= rest.size()) {
      return MappingSpec{}; // EN: missing/misplaced '.' -- malformed. PT: '.' ausente/mal-posto -- malformado.
    }
    const std::string hat_str  = rest.substr(0, dot);
    const std::string mask_str = rest.substr(dot + 1);
    int hat_index = 0;
    int hat_mask  = 0;
    const auto hat_result = std::from_chars(hat_str.data(), hat_str.data() + hat_str.size(), hat_index);
    const auto mask_result = std::from_chars(mask_str.data(), mask_str.data() + mask_str.size(), hat_mask);
    if (hat_result.ec != std::errc{} || hat_result.ptr != hat_str.data() + hat_str.size() || hat_index < 0 ||
        mask_result.ec != std::errc{} || mask_result.ptr != mask_str.data() + mask_str.size() || hat_mask < 0) {
      return MappingSpec{}; // EN: either half malformed. PT: qualquer uma das metades malformada.
    }
    spec.kind        = MappingKind::Hat;
    spec.index       = hat_index;
    spec.hat_mask     = hat_mask;
    spec.direct_code  = false;
    return spec;
  }

  return MappingSpec{}; // EN: unrecognised kind char -- malformed. PT: char de tipo não reconhecido -- malformado.
}

// EN: The fixed SDL control-name vocabulary this module recognises, mapped to a slot in
//     `GamepadMapping::buttons`/`::axes`. `nullptr`-equivalent (kind == None, target == nullptr)
//     for "platform" (handled separately by the caller) and for any name outside this table --
//     an unrecognised field key is skipped, not an error (fail-high).
// PT: O vocabulário fixo de nomes de controle do SDL que este módulo reconhece, mapeado pra um
//     slot em `GamepadMapping::buttons`/`::axes`. Equivalente a `nullptr` (kind == None, target ==
//     nullptr) pra "platform" (tratado separadamente pelo chamador) e pra qualquer nome fora
//     desta tabela -- uma chave de campo não reconhecida é pulada, não é um erro (fail-high).
struct ControlTarget {
  bool          is_button = false;
  GamepadButton button    = GamepadButton::Count;
  GamepadAxis   axis      = GamepadAxis::Count;
};

// EN: "x"/"y" -> North/West, NOT West/North -- the kernel's OWN gamepad.rst spec aliases
//     `BTN_X` to `BTN_NORTH` (0x133) and `BTN_Y` to `BTN_WEST` (0x134) (confirmed by reading
//     <linux/input-event-codes.h> directly, not assumed from an Xbox-pad physical-layout
//     mental model, which would get this backwards -- North/West here are the KERNEL's compass
//     labels for the "X"/"Y" letter buttons, not a description of where they sit on any specific
//     controller's face). This is exactly the class of "conversion formula with no delta test"
//     bug named in glfw_event_translate_sanity.cpp's own header comment -- caught here by
//     tracing the real kernel header before writing the table, not by assumption.
// PT: "x"/"y" -> North/West, NÃO West/North -- a própria spec gamepad.rst do kernel apelida
//     `BTN_X` de `BTN_NORTH` (0x133) e `BTN_Y` de `BTN_WEST` (0x134) (confirmado lendo
//     <linux/input-event-codes.h> diretamente, não assumido por um modelo mental de layout
//     físico de controle Xbox, que erraria isso ao contrário -- North/West aqui são os rótulos
//     de bússola do PRÓPRIO KERNEL pros botões-letra "X"/"Y", não uma descrição de onde eles
//     ficam na face de um controle específico). Esta é exatamente a classe de bug "fórmula de
//     conversão sem teste de delta" nomeada no próprio comentário de cabeçalho do
//     glfw_event_translate_sanity.cpp -- pega aqui rastreando o header real do kernel antes de
//     escrever a tabela, não por suposição.
bool lookup_control_target(const std::string& key, ControlTarget& out) {
  static const std::pair<const char*, ControlTarget> kTable[] = {
      {"a", {true, GamepadButton::South, GamepadAxis::Count}},
      {"b", {true, GamepadButton::East, GamepadAxis::Count}},
      {"x", {true, GamepadButton::North, GamepadAxis::Count}},
      {"y", {true, GamepadButton::West, GamepadAxis::Count}},
      {"back", {true, GamepadButton::Back, GamepadAxis::Count}},
      {"guide", {true, GamepadButton::Guide, GamepadAxis::Count}},
      {"start", {true, GamepadButton::Start, GamepadAxis::Count}},
      {"leftstick", {true, GamepadButton::LeftStick, GamepadAxis::Count}},
      {"rightstick", {true, GamepadButton::RightStick, GamepadAxis::Count}},
      {"leftshoulder", {true, GamepadButton::LeftBumper, GamepadAxis::Count}},
      {"rightshoulder", {true, GamepadButton::RightBumper, GamepadAxis::Count}},
      {"dpup", {true, GamepadButton::DpadUp, GamepadAxis::Count}},
      {"dpdown", {true, GamepadButton::DpadDown, GamepadAxis::Count}},
      {"dpleft", {true, GamepadButton::DpadLeft, GamepadAxis::Count}},
      {"dpright", {true, GamepadButton::DpadRight, GamepadAxis::Count}},
      {"leftx", {false, GamepadButton::Count, GamepadAxis::LeftX}},
      {"lefty", {false, GamepadButton::Count, GamepadAxis::LeftY}},
      {"rightx", {false, GamepadButton::Count, GamepadAxis::RightX}},
      {"righty", {false, GamepadButton::Count, GamepadAxis::RightY}},
      {"lefttrigger", {false, GamepadButton::Count, GamepadAxis::LeftTrigger}},
      {"righttrigger", {false, GamepadButton::Count, GamepadAxis::RightTrigger}},
  };
  for (const auto& entry : kTable) {
    if (key == entry.first) {
      out = entry.second;
      return true;
    }
  }
  return false;
}

// EN: Parses one line of gamecontrollerdb.txt-format text into `guid`+`mapping`. Returns `false`
//     (line rejected wholesale) when: fewer than 2 comma-separated fields, the GUID is not
//     exactly 32 hex chars, or no `platform:Linux` field was found. Any OTHER malformed FIELD
//     inside an otherwise-acceptable line is simply skipped (fail-high per-field).
// PT: Faz parse de uma linha em formato gamecontrollerdb.txt pra `guid`+`mapping`. Retorna
//     `false` (linha rejeitada por completo) quando: menos de 2 campos separados por vírgula, o
//     GUID não tem exatamente 32 chars hex, ou nenhum campo `platform:Linux` foi encontrado.
//     Qualquer OUTRO CAMPO malformado dentro de uma linha aceitável no resto é simplesmente
//     pulado (fail-high por-campo).
bool parse_line(const std::string& line, std::string& guid_out, GamepadMapping& mapping_out) {
  if (line.empty() || line.front() == '#') {
    return false;
  }

  std::vector<std::string> fields;
  std::size_t start = 0;
  while (start <= line.size()) {
    const std::size_t comma = line.find(',', start);
    if (comma == std::string::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, comma - start));
    start = comma + 1;
  }
  if (fields.size() < 2) {
    return false; // EN: no comma at all -- not guid,name,... shape. PT: nenhuma vírgula -- não é forma guid,nome,...
  }

  const std::string& guid = fields[0];
  if (guid.size() != 32 || !std::all_of(guid.begin(), guid.end(), is_hex_digit)) {
    return false; // EN: short/odd/non-hex GUID -- malformed line. PT: GUID curto/ímpar/não-hex -- linha malformada.
  }

  GamepadMapping mapping;
  bool           platform_linux = false;

  for (std::size_t i = 2; i < fields.size(); ++i) {
    const std::string& field = fields[i];
    const std::size_t  colon = field.find(':');
    if (colon == std::string::npos) {
      continue; // EN: not a key:value field -- skip it, keep parsing the rest of the line.
                 // PT: não é um campo key:value -- pula, continua parseando o resto da linha.
    }
    const std::string key   = field.substr(0, colon);
    const std::string value = field.substr(colon + 1);

    if (key == "platform") {
      if (value == "Linux") {
        platform_linux = true;
      }
      continue;
    }

    ControlTarget target;
    if (!lookup_control_target(key, target)) {
      continue; // EN: unrecognised control name -- fail-high, skip this field only.
                 // PT: nome de controle não reconhecido -- fail-high, pula só este campo.
    }
    const MappingSpec spec = parse_value_spec(value);
    if (spec.kind == MappingKind::None) {
      continue; // EN: malformed value spec -- skip this field only. PT: value spec malformado -- pula só este campo.
    }
    if (target.is_button) {
      mapping.buttons[static_cast<std::size_t>(target.button)] = spec;
    } else {
      mapping.axes[static_cast<std::size_t>(target.axis)] = spec;
    }
  }

  if (!platform_linux) {
    return false; // EN: no platform:Linux field -- this whole line is for a different platform.
                   // PT: sem campo platform:Linux -- esta linha inteira é de outra plataforma.
  }

  guid_out    = to_lower_ascii(guid);
  mapping_out = mapping;
  return true;
}

} // namespace

// ---------------------------------------------------------------------------
// EN: build_guid()
// PT: build_guid()
// ---------------------------------------------------------------------------

std::string build_guid(const input_id& id, const std::string& name) {
  std::string hex;
  hex.reserve(32);
  append_u16_le(hex, id.bustype);
  append_u16_le(hex, crc16_arc(name));
  append_u16_le(hex, id.vendor);
  append_u16_le(hex, 0);
  append_u16_le(hex, id.product);
  append_u16_le(hex, 0);
  append_u16_le(hex, id.version);
  append_u16_le(hex, 0);
  return hex;
}

// ---------------------------------------------------------------------------
// EN: derive_layout()
// PT: derive_layout()
// ---------------------------------------------------------------------------

JoystickLayout derive_layout(const std::array<bool, static_cast<std::size_t>(KEY_MAX) + 1>& key_bits,
                              const std::array<bool, static_cast<std::size_t>(ABS_MAX) + 1>& abs_bits) {
  JoystickLayout layout;

  // EN: Buttons: [BTN_JOYSTICK, KEY_MAX) first, then [0, BTN_JOYSTICK) -- see this file's header
  //     comment.
  // PT: Botões: [BTN_JOYSTICK, KEY_MAX) primeiro, depois [0, BTN_JOYSTICK) -- ver o comentário de
  //     cabeçalho deste arquivo.
  for (int code = BTN_JOYSTICK; code < KEY_MAX; ++code) {
    if (key_bits[static_cast<std::size_t>(code)]) {
      layout.button_codes.push_back(static_cast<std::uint16_t>(code));
    }
  }
  for (int code = 0; code < BTN_JOYSTICK; ++code) {
    if (key_bits[static_cast<std::size_t>(code)]) {
      layout.button_codes.push_back(static_cast<std::uint16_t>(code));
    }
  }

  // EN: Axes: 0..ABS_MAX ascending, excluding the 8 hat codes (ABS_HAT0X..ABS_HAT3Y).
  // PT: Eixos: 0..ABS_MAX ascendente, excluindo os 8 códigos de hat (ABS_HAT0X..ABS_HAT3Y).
  for (int code = 0; code <= ABS_MAX; ++code) {
    if (code >= ABS_HAT0X && code <= ABS_HAT3Y) {
      continue; // EN: hats are exposed separately below, never as a plain axis index.
                 // PT: hats são expostos separadamente abaixo, nunca como índice de eixo simples.
    }
    if (abs_bits[static_cast<std::size_t>(code)]) {
      layout.axis_codes.push_back(static_cast<std::uint16_t>(code));
    }
  }

  // EN: Hats: 4 pairs, ABS_HAT0..ABS_HAT3, present if either the X or Y bit of the pair is set.
  // PT: Hats: 4 pares, ABS_HAT0..ABS_HAT3, presentes se o bit X ou Y do par estiver setado.
  for (int hat = 0; hat < 4; ++hat) {
    const int x_code = ABS_HAT0X + hat * 2;
    const int y_code = ABS_HAT0Y + hat * 2;
    if (abs_bits[static_cast<std::size_t>(x_code)] || abs_bits[static_cast<std::size_t>(y_code)]) {
      layout.hat_pairs.emplace_back(static_cast<std::uint16_t>(x_code), static_cast<std::uint16_t>(y_code));
    }
  }

  return layout;
}

// ---------------------------------------------------------------------------
// EN: MappingDb
// PT: MappingDb
// ---------------------------------------------------------------------------

int MappingDb::parse_text(const char* text) {
  if (text == nullptr) {
    return 0;
  }
  const std::string full(text);

  int         added = 0;
  std::size_t start = 0;
  while (start <= full.size()) {
    const std::size_t newline = full.find('\n', start);
    const std::string line    = (newline == std::string::npos) ? full.substr(start) : full.substr(start, newline - start);

    // EN: Strip a trailing '\r' (CRLF input), same convention as I18n::parse_and_merge.
    // PT: Remove um '\r' à direita (input CRLF), mesma convenção do I18n::parse_and_merge.
    std::string trimmed = line;
    if (!trimmed.empty() && trimmed.back() == '\r') {
      trimmed.pop_back();
    }

    std::string    guid;
    GamepadMapping mapping;
    if (parse_line(trimmed, guid, mapping)) {
      by_guid_[guid] = mapping; // EN: last-write-wins per GUID. PT: último-escreve-vence por GUID.
      ++added;
    }

    if (newline == std::string::npos) {
      break; // EN: reached the end -- last line handled whether or not it had a trailing '\n'.
              // PT: chegou ao fim -- última linha tratada com ou sem '\n' final.
    }
    start = newline + 1;
  }
  return added;
}

const GamepadMapping* MappingDb::find(const std::string& guid_hex32) const {
  if (guid_hex32.size() != 32 || !std::all_of(guid_hex32.begin(), guid_hex32.end(), is_hex_digit)) {
    return nullptr;
  }
  const std::string guid_lc = to_lower_ascii(guid_hex32);

  auto it = by_guid_.find(guid_lc);
  if (it != by_guid_.end()) {
    return &it->second;
  }

  // EN: Second attempt: zero the CRC field (hex chars [4, 8), i.e. bytes 2-3) -- the shape most
  //     community DB entries actually ship in (see this file's header comment).
  // PT: Segunda tentativa: zera o campo CRC (chars hex [4, 8), ou seja, bytes 2-3) -- a forma
  //     que a maioria das entradas de DB da comunidade de fato traz (ver o comentário de
  //     cabeçalho deste arquivo).
  std::string crc_zeroed = guid_lc;
  crc_zeroed.replace(4, 4, "0000");
  it = by_guid_.find(crc_zeroed);
  if (it != by_guid_.end()) {
    return &it->second;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// EN: default_kernel_spec_mapping() (D5)
// PT: default_kernel_spec_mapping() (D5)
// ---------------------------------------------------------------------------

namespace {

MappingSpec direct_button(int code) {
  MappingSpec spec;
  spec.kind        = MappingKind::Button;
  spec.index       = code;
  spec.direct_code = true;
  return spec;
}

MappingSpec direct_axis(int code) {
  MappingSpec spec;
  spec.kind        = MappingKind::Axis;
  spec.index       = code;
  spec.direct_code = true;
  return spec;
}

MappingSpec direct_hat0(int mask) {
  MappingSpec spec;
  spec.kind        = MappingKind::Hat;
  spec.index       = ABS_HAT0X; // EN: Y code is index+1 for direct_code Hat specs, see hpp comment.
                                  // PT: código Y é index+1 pra specs Hat com direct_code, ver comentário do hpp.
  spec.hat_mask     = mask;
  spec.direct_code  = true;
  return spec;
}

} // namespace

GamepadMapping default_kernel_spec_mapping(bool has_abs_gas, bool has_abs_brake) {
  GamepadMapping mapping;

  mapping.buttons[static_cast<std::size_t>(GamepadButton::South)]       = direct_button(BTN_SOUTH);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::East)]        = direct_button(BTN_EAST);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::West)]        = direct_button(BTN_WEST);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::North)]       = direct_button(BTN_NORTH);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::LeftBumper)]  = direct_button(BTN_TL);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::RightBumper)] = direct_button(BTN_TR);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::LeftStick)]   = direct_button(BTN_THUMBL);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::RightStick)]  = direct_button(BTN_THUMBR);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::Start)]       = direct_button(BTN_START);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::Back)]        = direct_button(BTN_SELECT);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::Guide)]       = direct_button(BTN_MODE);

  // EN: D5: the D-pad is a hat, not 4 discrete BTN_* codes, in the kernel gamepad.rst spec.
  // PT: D5: o D-pad é um hat, não 4 códigos BTN_* discretos, na spec gamepad.rst do kernel.
  mapping.buttons[static_cast<std::size_t>(GamepadButton::DpadUp)]    = direct_hat0(0x1);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::DpadRight)] = direct_hat0(0x2);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::DpadDown)]  = direct_hat0(0x4);
  mapping.buttons[static_cast<std::size_t>(GamepadButton::DpadLeft)]  = direct_hat0(0x8);

  mapping.axes[static_cast<std::size_t>(GamepadAxis::LeftX)]  = direct_axis(ABS_X);
  mapping.axes[static_cast<std::size_t>(GamepadAxis::LeftY)]  = direct_axis(ABS_Y);
  mapping.axes[static_cast<std::size_t>(GamepadAxis::RightX)] = direct_axis(ABS_RX);
  mapping.axes[static_cast<std::size_t>(GamepadAxis::RightY)] = direct_axis(ABS_RY);
  // EN: D5: ABS_BRAKE/ABS_GAS when present, else the ABS_Z/ABS_RZ fallback.
  // PT: D5: ABS_BRAKE/ABS_GAS quando presentes, senão o fallback ABS_Z/ABS_RZ.
  mapping.axes[static_cast<std::size_t>(GamepadAxis::LeftTrigger)]  = direct_axis(has_abs_brake ? ABS_BRAKE : ABS_Z);
  mapping.axes[static_cast<std::size_t>(GamepadAxis::RightTrigger)] = direct_axis(has_abs_gas ? ABS_GAS : ABS_RZ);

  return mapping;
}

// ---------------------------------------------------------------------------
// EN: resolve_button() / resolve_axis()
// PT: resolve_button() / resolve_axis()
// ---------------------------------------------------------------------------

namespace {

bool resolve_code_for_button_or_axis(const MappingSpec& spec, const std::vector<std::uint16_t>& indexed_codes,
                                      std::uint16_t& code_out) {
  if (spec.direct_code) {
    code_out = static_cast<std::uint16_t>(spec.index);
    return true;
  }
  if (spec.index < 0 || static_cast<std::size_t>(spec.index) >= indexed_codes.size()) {
    return false;
  }
  code_out = indexed_codes[static_cast<std::size_t>(spec.index)];
  return true;
}

int clamp_hat_sign(std::int32_t raw) {
  if (raw < 0) return -1;
  if (raw > 0) return 1;
  return 0;
}

} // namespace

bool resolve_button(const GamepadMapping& mapping, const JoystickLayout& layout,
                    const GamepadDecoder& decoder, GamepadButton button) {
  const auto idx = static_cast<std::size_t>(button);
  if (idx >= mapping.buttons.size()) {
    return false;
  }
  const MappingSpec& spec = mapping.buttons[idx];

  switch (spec.kind) {
    case MappingKind::Button: {
      std::uint16_t code = 0;
      if (!resolve_code_for_button_or_axis(spec, layout.button_codes, code)) return false;
      const bool pressed = decoder.key_state(code);
      return spec.inverted ? !pressed : pressed;
    }
    case MappingKind::Axis: {
      std::uint16_t code = 0;
      if (!resolve_code_for_button_or_axis(spec, layout.axis_codes, code)) return false;
      float value = decoder.abs_normalized(code);
      if (spec.inverted) value = -value;
      if (spec.half > 0) return value > 0.5f;
      if (spec.half < 0) return value < -0.5f;
      return value > 0.5f; // EN: full axis used as a digital button: positive-half convention.
                             // PT: eixo completo usado como botão digital: convenção de metade positiva.
    }
    case MappingKind::Hat: {
      std::uint16_t x_code = 0;
      std::uint16_t y_code = 0;
      if (spec.direct_code) {
        x_code = static_cast<std::uint16_t>(spec.index);
        y_code = static_cast<std::uint16_t>(spec.index + 1);
      } else {
        if (spec.index < 0 || static_cast<std::size_t>(spec.index) >= layout.hat_pairs.size()) return false;
        x_code = layout.hat_pairs[static_cast<std::size_t>(spec.index)].first;
        y_code = layout.hat_pairs[static_cast<std::size_t>(spec.index)].second;
      }
      const int hx = clamp_hat_sign(decoder.abs_value(x_code));
      const int hy = clamp_hat_sign(decoder.abs_value(y_code));
      bool result = false;
      if (spec.hat_mask & 0x1) result |= (hy < 0); // up
      if (spec.hat_mask & 0x2) result |= (hx > 0); // right
      if (spec.hat_mask & 0x4) result |= (hy > 0); // down
      if (spec.hat_mask & 0x8) result |= (hx < 0); // left
      return result;
    }
    default:
      return false;
  }
}

float resolve_axis(const GamepadMapping& mapping, const JoystickLayout& layout,
                   const GamepadDecoder& decoder, GamepadAxis axis) {
  const auto idx = static_cast<std::size_t>(axis);
  if (idx >= mapping.axes.size()) {
    return 0.0f;
  }
  const MappingSpec& spec = mapping.axes[idx];
  if (spec.kind != MappingKind::Axis) {
    return 0.0f;
  }
  std::uint16_t code = 0;
  if (!resolve_code_for_button_or_axis(spec, layout.axis_codes, code)) return 0.0f;

  float value = decoder.abs_normalized(code);
  if (spec.inverted) {
    // EN: Symmetric (stick) range: negate. Zero-based (trigger) range: flip within [0,1].
    // PT: Faixa simétrica (stick): nega. Faixa baseada em zero (gatilho): inverte dentro de [0,1].
    value = decoder.abs_is_signed(code) ? -value : (1.0f - value);
  }
  if (spec.half > 0) {
    value = std::max(value, 0.0f);
  } else if (spec.half < 0) {
    value = std::max(-value, 0.0f);
  }
  return value;
}

} // namespace glintfx
