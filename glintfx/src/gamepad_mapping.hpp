// SPDX-License-Identifier: MPL-2.0
// EN: A2-GAMEPAD (framework-2D, ADR-0015/ADR-0016 module "gamepad") -- SDL-GUID construction,
//     SDL-compatible joystick-index derivation, the `gamecontrollerdb.txt` text parser
//     (hostile-safe), and the default kernel-spec mapping for a pad the DB does not cover (D5 of
//     the plan). Pure, headless-testable (no `fd`/`ioctl`; capability bitmaps and decoder state
//     are passed in, never read from a real device) -- same dependency-injection-by-construction
//     posture as gamepad_decode.hpp.
//
//     This header lives under `src/` (INTERNAL, never installed) and freely includes
//     `<linux/input.h>` for `struct input_id` and the `BTN_*`/`ABS_*`/`KEY_MAX`/`ABS_MAX`
//     constants (same encapsulation rule as gamepad_decode.hpp: this is not the public
//     `include/glintfx/` surface).
//
//     THREE PIECES, in the order the A2-GAMEPAD plan documents them (section 2.4):
//
//     1. `build_guid(id, name)` -- the 16-byte, 32-hex-char SDL GUID: bytes 0-1 = `bustype` (LE);
//        2-3 = CRC-16/ARC (poly `0xA001`, init `0`, reflected) of `name`, LE; 4-5 = `vendor` (LE);
//        6-7 = 0; 8-9 = `product` (LE); 10-11 = 0; 12-13 = `version` (LE); 14-15 = 0. Note: an
//        EMPTY `name` produces CRC `0` by construction (the reflected-CRC loop simply never
//        runs), so `build_guid(id, "")` is the deterministic way to get the "CRC already zero"
//        shape by construction, without any special-casing in this function -- exactly the shape
//        most `gamecontrollerdb.txt` community entries ship (their GUID's CRC field is
//        conventionally zeroed, SDL >= 2.26 behaviour, cited the same way i18n.cpp cites its own
//        CLDR source). `MappingDb::find()` (below) is what does the "2 attempts" matching (exact,
//        then with bytes 2-3 zeroed) against a REAL device's honestly-computed (non-zero-CRC)
//        GUID -- `build_guid()` itself never fabricates a fake CRC.
//     2. `derive_layout(key_bits, abs_bits)` -- clean-room, from PUBLICLY DOCUMENTED SDL/evdev
//        enumeration behaviour (not from reading SDL's own source): joystick BUTTON index i is
//        assigned by iterating evdev `EV_KEY` codes present in `key_bits` in ascending order,
//        first across `[BTN_JOYSTICK, KEY_MAX)`, THEN across `[0, BTN_JOYSTICK)` -- gamepad/
//        joystick buttons (`BTN_SOUTH` etc., `BTN_JOYSTICK` upward) enumerate before any stray
//        lower-range code a permissive D7 capability check let through. AXIS index i is assigned
//        by iterating `EV_ABS` codes `0..ABS_MAX` ascending, EXCLUDING the 8 hat codes
//        `ABS_HAT0X..ABS_HAT3Y` (`0x10..0x17`) -- those are exposed separately as `hat_pairs`
//        (index i -> the evdev `{X, Y}` code pair for hat i). A `gamecontrollerdb.txt` entry's
//        `bN`/`aN`/`hH.M` fields index into THESE THREE lists, not raw evdev codes directly.
//     3. `MappingDb` -- a `parse_text()`-fed table of GUID -> `GamepadMapping`, keyed by the
//        32-hex-char GUID string; `find()` encapsulates the "2 attempts" GUID match so callers
//        never need to know about the CRC-zeroing fallback themselves.
//
//     `default_kernel_spec_mapping()` (D5) is the RESOLUTION-FREE fallback for a pad no DB entry
//     matches: it maps `GamepadButton`/`GamepadAxis` directly to hardcoded kernel gamepad.rst
//     evdev codes (`MappingSpec::direct_code = true`, `index` IS the raw evdev code, bypassing
//     `JoystickLayout` entirely) -- modern drivers (xpad, hid-playstation, hid-nintendo) already
//     normalize to this spec, so the DB only needs to correct the exceptions.
//
//     `resolve_button()`/`resolve_axis()` are the single shared resolution path both a DB-derived
//     `GamepadMapping` (index into `JoystickLayout`) and the default kernel-spec `GamepadMapping`
//     (`direct_code = true`, index IS the evdev code) go through -- `MappingSpec::direct_code`
//     is the one branch point, everything else (button/axis/hat kind dispatch, `~` inversion,
//     `+`/`-` half-axis) is identical for both origins.
// PT: A2-GAMEPAD (framework-2D, módulo "gamepad" da ADR-0015/ADR-0016) -- construção de GUID SDL,
//     derivação de índice de joystick compatível-SDL, o parser de texto do
//     `gamecontrollerdb.txt` (hostile-safe), e o mapeamento default kernel-spec pra um pad que o
//     DB não cobre (D5 do plano). Puro, testável headless (sem `fd`/`ioctl`; bitmaps de
//     capacidade e estado do decoder são passados por parâmetro, nunca lidos de um device real)
//     -- mesma postura de injeção-de-dependência-por-construção do gamepad_decode.hpp.
//
//     Este header vive sob `src/` (INTERNO, nunca instalado) e inclui livremente
//     `<linux/input.h>` pela `struct input_id` e as constantes `BTN_*`/`ABS_*`/`KEY_MAX`/
//     `ABS_MAX` (mesma regra de encapsulamento do gamepad_decode.hpp: esta não é a superfície
//     pública de `include/glintfx/`).
//
//     TRÊS PEÇAS, na ordem em que o plano A2-GAMEPAD as documenta (seção 2.4):
//
//     1. `build_guid(id, name)` -- o GUID SDL de 16 bytes, 32 chars hex: bytes 0-1 = `bustype`
//        (LE); 2-3 = CRC-16/ARC (polinômio `0xA001`, init `0`, refletido) de `name`, LE; 4-5 =
//        `vendor` (LE); 6-7 = 0; 8-9 = `product` (LE); 10-11 = 0; 12-13 = `version` (LE); 14-15 =
//        0. Nota: um `name` VAZIO produz CRC `0` por construção (o laço de CRC refletido
//        simplesmente nunca roda), então `build_guid(id, "")` é a forma determinística de obter a
//        forma "CRC já zero" por construção, sem nenhum caso especial nesta função -- exatamente
//        a forma que a maioria das entradas da comunidade do `gamecontrollerdb.txt` traz (o campo
//        CRC do GUID delas é convencionalmente zerado, comportamento SDL >= 2.26, citado do mesmo
//        jeito que o i18n.cpp cita a própria fonte CLDR). `MappingDb::find()` (abaixo) é quem faz
//        o casamento de "2 tentativas" (exato, depois com bytes 2-3 zerados) contra o GUID
//        honestamente calculado (CRC não-zero) de um device REAL -- o próprio `build_guid()`
//        nunca fabrica um CRC falso.
//     2. `derive_layout(key_bits, abs_bits)` -- clean-room, a partir de comportamento de
//        enumeração SDL/evdev PUBLICAMENTE DOCUMENTADO (não de leitura do source do próprio SDL):
//        o índice de BOTÃO de joystick i é atribuído iterando códigos evdev `EV_KEY` presentes em
//        `key_bits` em ordem ascendente, primeiro em `[BTN_JOYSTICK, KEY_MAX)`, DEPOIS em
//        `[0, BTN_JOYSTICK)` -- botões de gamepad/joystick (`BTN_SOUTH` etc., `BTN_JOYSTICK` pra
//        cima) enumeram antes de qualquer código de faixa inferior perdido que uma checagem de
//        capacidade D7 permissiva deixou passar. O índice de EIXO i é atribuído iterando códigos
//        `EV_ABS` `0..ABS_MAX` ascendentes, EXCLUINDO os 8 códigos de hat `ABS_HAT0X..ABS_HAT3Y`
//        (`0x10..0x17`) -- esses são expostos separadamente como `hat_pairs` (índice i -> o par
//        de código evdev `{X, Y}` do hat i). Os campos `bN`/`aN`/`hH.M` de uma entrada do
//        `gamecontrollerdb.txt` indexam NESSAS TRÊS LISTAS, não códigos evdev crus diretamente.
//     3. `MappingDb` -- uma tabela GUID -> `GamepadMapping` alimentada por `parse_text()`,
//        indexada pela string GUID de 32 chars hex; `find()` encapsula o casamento de "2
//        tentativas" de GUID pra que os chamadores nunca precisem saber do fallback de
//        zerar-CRC por conta própria.
//
//     `default_kernel_spec_mapping()` (D5) é o fallback SEM RESOLUÇÃO pra um pad que nenhuma
//     entrada de DB casa: mapeia `GamepadButton`/`GamepadAxis` direto pra códigos evdev
//     hardcoded da spec gamepad.rst do kernel (`MappingSpec::direct_code = true`, `index` É o
//     código evdev cru, contornando o `JoystickLayout` por completo) -- drivers modernos (xpad,
//     hid-playstation, hid-nintendo) já normalizam pra essa spec, então o DB só precisa corrigir
//     as exceções.
//
//     `resolve_button()`/`resolve_axis()` são o único caminho de resolução compartilhado por onde
//     tanto um `GamepadMapping` derivado de DB (índice em `JoystickLayout`) quanto o
//     `GamepadMapping` default kernel-spec (`direct_code = true`, índice É o código evdev)
//     passam -- `MappingSpec::direct_code` é o único ponto de ramificação, tudo o resto
//     (despacho de tipo botão/eixo/hat, inversão `~`, meio-eixo `+`/`-`) é idêntico pras duas
//     origens.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <glintfx/gamepad_types.hpp>

#include "gamepad_decode.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <linux/input.h>

namespace glintfx {

// EN: SDL-compatible joystick layout derived by `derive_layout()` -- see this file's header
//     comment for the exact enumeration order of each list.
// PT: Layout de joystick compatível-SDL derivado por `derive_layout()` -- ver o comentário de
//     cabeçalho deste arquivo pra ordem de enumeração exata de cada lista.
struct JoystickLayout {
  std::vector<std::uint16_t>                            button_codes;
  std::vector<std::uint16_t>                             axis_codes;
  std::vector<std::pair<std::uint16_t, std::uint16_t>>   hat_pairs; // EN: {X code, Y code} per hat index. PT: {código X, código Y} por índice de hat.
};

JoystickLayout derive_layout(const std::array<bool, static_cast<std::size_t>(KEY_MAX) + 1>& key_bits,
                              const std::array<bool, static_cast<std::size_t>(ABS_MAX) + 1>& abs_bits);

// EN: 32 lower-case hex chars (16 bytes), SDL GUID format -- see this file's header comment.
// PT: 32 chars hex minúsculos (16 bytes), formato de GUID do SDL -- ver o comentário de
//     cabeçalho deste arquivo.
std::string build_guid(const input_id& id, const std::string& name);

// EN: What one control (button or axis) resolves to. `direct_code == true`: `index` IS the raw
//     evdev code (default kernel-spec mapping, no JoystickLayout involved). `direct_code ==
//     false`: `index` is a position into `JoystickLayout::button_codes`/`axis_codes`/`hat_pairs`
//     (DB-derived mapping). `kind == Hat`: `hat_mask` is the SDL hat bitmask (`1`=Up, `2`=Right,
//     `4`=Down, `8`=Left); for `direct_code == true` Hat specs, `index` is the hat's X code and
//     the Y code is implicitly `index + 1` (evdev's own `ABS_HATnX`/`ABS_HATnY` are always
//     adjacent codes). `half`: `0` = full axis, `+1`/`-1` = only the positive/negative half of an
//     axis used as a digital button (`+aN`/`-aN` DB syntax). `inverted`: the `~` DB suffix.
// PT: Pra que um controle (botão ou eixo) resolve. `direct_code == true`: `index` É o código
//     evdev cru (mapeamento default kernel-spec, sem envolver JoystickLayout). `direct_code ==
//     false`: `index` é uma posição em `JoystickLayout::button_codes`/`axis_codes`/`hat_pairs`
//     (mapeamento derivado de DB). `kind == Hat`: `hat_mask` é o bitmask de hat do SDL (`1`=Up,
//     `2`=Right, `4`=Down, `8`=Left); pra specs Hat com `direct_code == true`, `index` é o código
//     X do hat e o código Y é implicitamente `index + 1` (os próprios `ABS_HATnX`/`ABS_HATnY` do
//     evdev são sempre códigos adjacentes). `half`: `0` = eixo completo, `+1`/`-1` = só a metade
//     positiva/negativa de um eixo usada como botão digital (sintaxe de DB `+aN`/`-aN`).
//     `inverted`: o sufixo `~` do DB.
enum class MappingKind : std::uint8_t { None, Button, Axis, Hat };

struct MappingSpec {
  MappingKind kind   = MappingKind::None;
  int         index  = -1;
  int         hat_mask = 0;
  int         half     = 0;
  bool        inverted    = false;
  bool        direct_code = false;
};

// EN: One full pad's worth of control resolution -- fixed-size arrays indexed by `GamepadButton`/
//     `GamepadAxis` (via `static_cast<size_t>`), default-constructed entries are `MappingKind::
//     None` (an unmapped control resolves to `false`/`0.0f`, never garbage).
// PT: A resolução de controles de um pad inteiro -- arrays de tamanho fixo indexados por
//     `GamepadButton`/`GamepadAxis` (via `static_cast<size_t>`), entradas construídas por default
//     são `MappingKind::None` (um controle não mapeado resolve pra `false`/`0.0f`, nunca lixo).
struct GamepadMapping {
  std::array<MappingSpec, static_cast<std::size_t>(GamepadButton::Count)> buttons{};
  std::array<MappingSpec, static_cast<std::size_t>(GamepadAxis::Count)>   axes{};
};

// EN: `gamecontrollerdb.txt`-format text database, GUID -> GamepadMapping. Hostile-safe parser
//     (malformed line/field: skipped, never a crash) -- see this file's header comment and the
//     .cpp's own `parse_line()` comment for the exact grammar accepted.
// PT: Base de dados em formato texto `gamecontrollerdb.txt`, GUID -> GamepadMapping. Parser
//     hostile-safe (linha/campo malformado: pulado, nunca crash) -- ver o comentário de
//     cabeçalho deste arquivo e o próprio comentário de `parse_line()` do .cpp pra gramática
//     exata aceita.
class MappingDb {
public:
  // EN: Parses `text` (any length, any number of lines, with or without a trailing newline) and
  //     MERGES matched entries into this instance (last-write-wins per GUID, same convention as
  //     I18n::parse_and_merge). Returns the number of entries actually added/replaced (0 on
  //     `nullptr`, empty text, or a text with zero valid `platform:Linux` entries). Never throws,
  //     never crashes on malformed/truncated/oversized input.
  // PT: Faz parse de `text` (qualquer tamanho, qualquer número de linhas, com ou sem newline
  //     final) e MESCLA as entradas casadas nesta instância (último-escreve-vence por GUID,
  //     mesma convenção do I18n::parse_and_merge). Retorna o número de entradas de fato
  //     adicionadas/substituídas (0 em `nullptr`, texto vazio, ou texto com zero entradas
  //     `platform:Linux` válidas). Nunca lança, nunca crasha em input malformado/truncado/
  //     gigante.
  int parse_text(const char* text);

  // EN: Looks up `guid_hex32` (a 32-lower-case-hex-char SDL GUID, e.g. from `build_guid()`) --
  //     tries an EXACT match first, then a match with the CRC field (hex chars 4..7, i.e. bytes
  //     2-3) zeroed (the shape most community DB entries actually ship in). `nullptr` if neither
  //     matches, or if `guid_hex32` is not exactly 32 hex chars.
  // PT: Busca `guid_hex32` (um GUID SDL de 32 chars hex minúsculos, ex.: vindo de
  //     `build_guid()`) -- tenta um casamento EXATO primeiro, depois um casamento com o campo CRC
  //     (chars hex 4..7, ou seja, bytes 2-3) zerado (a forma que a maioria das entradas de DB da
  //     comunidade de fato traz). `nullptr` se nenhum dos dois casar, ou se `guid_hex32` não tiver
  //     exatamente 32 chars hex.
  const GamepadMapping* find(const std::string& guid_hex32) const;

  std::size_t size() const { return by_guid_.size(); }

private:
  std::unordered_map<std::string, GamepadMapping> by_guid_;
};

// EN: D5 of the plan -- the mapping used when no DB entry matches a pad's GUID. `has_abs_gas`/
//     `has_abs_brake` (from the pad's capability bitmap) select `ABS_GAS`/`ABS_BRAKE` for the
//     right/left trigger when present, falling back to `ABS_RZ`/`ABS_Z` otherwise (both
//     `direct_code = true` -- this mapping never touches a `JoystickLayout`).
// PT: D5 do plano -- o mapeamento usado quando nenhuma entrada de DB casa o GUID de um pad.
//     `has_abs_gas`/`has_abs_brake` (do bitmap de capacidade do pad) selecionam `ABS_GAS`/
//     `ABS_BRAKE` pro gatilho direito/esquerdo quando presentes, caindo pra `ABS_RZ`/`ABS_Z`
//     senão (ambos com `direct_code = true` -- este mapeamento nunca toca um `JoystickLayout`).
GamepadMapping default_kernel_spec_mapping(bool has_abs_gas, bool has_abs_brake);

// EN: Shared resolution path -- see this file's header comment for the `direct_code` branch
//     point shared by both DB-derived and default-kernel-spec mappings.
// PT: Caminho de resolução compartilhado -- ver o comentário de cabeçalho deste arquivo pro
//     ponto de ramificação `direct_code` compartilhado tanto por mapeamentos derivados de DB
//     quanto pelo default kernel-spec.
bool  resolve_button(const GamepadMapping& mapping, const JoystickLayout& layout,
                      const GamepadDecoder& decoder, GamepadButton button);
float resolve_axis(const GamepadMapping& mapping, const JoystickLayout& layout,
                    const GamepadDecoder& decoder, GamepadAxis axis);

} // namespace glintfx
