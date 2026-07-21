// SPDX-License-Identifier: MPL-2.0
// EN: A2-GAMEPAD (framework-2D, ADR-0015/ADR-0016 module "gamepad") -- the normalized button/axis
//     vocabulary the `Gamepads` public API (glintfx/gamepad.hpp, Agent B) exposes on top of the
//     raw evdev surface. This header is deliberately the SMALLEST possible: two plain enums, zero
//     third-party/kernel type (no `<linux/input.h>`, no `input_event`/`input_id`/`input_absinfo`)
//     -- those live only in the internal `src/gamepad_decode.hpp`/`src/gamepad_mapping.hpp`
//     headers, never here (AGENTS.md encapsulation rule: nothing kernel-shaped crosses the public
//     `include/glintfx/` boundary, same discipline as every other module in this library).
//
//     `GamepadButton`/`GamepadAxis` are the NORMALIZED layout (kernel `Documentation/
//     input/gamepad.rst` naming, matched 1:1 by the mapping module's default kernel-spec table,
//     src/gamepad_mapping.cpp) -- what a consumer gets from `Gamepads::button()`/`Gamepads::axis()`
//     after DB/kernel-spec resolution. The library's own leader decision (see the A2-GAMEPAD plan,
//     section "Leader decisions already made") is to ALSO expose the complete RAW surface
//     (`Gamepads::raw_button()`/`raw_axis()`/...) alongside this normalized one -- this header
//     covers only the normalized half of that contract.
//
//     `Count` in both enums is a sentinel (array-sizing / bounds-check use only), never itself a
//     valid button/axis value -- same convention as every other sentinel enumerator in this
//     codebase (e.g. `Key::Count` in ui_event.hpp).
// PT: A2-GAMEPAD (framework-2D, módulo "gamepad" da ADR-0015/ADR-0016) -- o vocabulário
//     normalizado de botão/eixo que a API pública `Gamepads` (glintfx/gamepad.hpp, Agente B)
//     expõe sobre a superfície evdev crua. Este header é deliberadamente o MENOR possível: dois
//     enums simples, zero tipo de terceiro/kernel (nenhum `<linux/input.h>`, nenhum
//     `input_event`/`input_id`/`input_absinfo`) -- esses vivem só nos headers internos
//     `src/gamepad_decode.hpp`/`src/gamepad_mapping.hpp`, nunca aqui (regra de encapsulamento do
//     AGENTS.md: nada com formato de kernel cruza a fronteira pública de `include/glintfx/`,
//     mesma disciplina de todo outro módulo desta biblioteca).
//
//     `GamepadButton`/`GamepadAxis` são o layout NORMALIZADO (nomenclatura da spec do kernel
//     `Documentation/input/gamepad.rst`, casada 1:1 pela tabela default kernel-spec do módulo de
//     mapeamento, src/gamepad_mapping.cpp) -- o que um consumidor recebe de
//     `Gamepads::button()`/`Gamepads::axis()` depois da resolução via DB/kernel-spec. A própria
//     decisão do líder deste projeto (ver o plano A2-GAMEPAD, seção "decisões do líder já
//     tomadas") é expor TAMBÉM a superfície CRUA completa (`Gamepads::raw_button()`/`raw_axis()`/
//     ...) ao lado desta normalizada -- este header cobre só a metade normalizada desse contrato.
//
//     `Count` nos dois enums é um sentinela (uso só de dimensionamento de array/checagem de
//     limite), nunca um valor válido de botão/eixo em si -- mesma convenção de todo outro
//     enumerador sentinela desta base de código (ex.: `Key::Count` em ui_event.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>

namespace glintfx {

// EN: Digital buttons, in kernel gamepad.rst naming (`South`/`East`/`West`/`North` instead of a
//     brand-specific "A/B/X/Y" or "Cross/Circle/Square/Triangle" -- the kernel spec's own choice,
//     which this module mirrors so the enum names do not silently assume one controller brand's
//     face-button labelling). `DpadUp/Down/Left/Right` are digital even though the underlying
//     kernel event is `ABS_HAT0X/Y` (an analog hat axis in evdev terms) -- normalizing the
//     4-direction hat into 4 booleans here is exactly what a "gamepad" (as opposed to a raw
//     joystick) is expected to expose (D-pad, not an analog hat), and is also SDL's own
//     `SDL_GameControllerButton` convention for the same 4 kernel-hat-backed directions.
//     `LeftStick`/`RightStick` are the analog stick CLICK buttons (`BTN_THUMBL`/`BTN_THUMBR`),
//     distinct from the `LeftX/LeftY/RightX/RightY` analog AXES in `GamepadAxis` below.
// PT: Botões digitais, na nomenclatura do gamepad.rst do kernel (`South`/`East`/`West`/`North`
//     em vez de um "A/B/X/Y" específico de marca ou "Cross/Circle/Square/Triangle" -- a própria
//     escolha da spec do kernel, que este módulo espelha para que os nomes do enum não assumam
//     silenciosamente a rotulagem de botão-de-face de uma marca de controle específica).
//     `DpadUp/Down/Left/Right` são digitais mesmo o evento de kernel subjacente sendo
//     `ABS_HAT0X/Y` (um eixo de hat analógico em termos de evdev) -- normalizar o hat de 4
//     direções em 4 booleanos aqui é exatamente o que se espera de um "gamepad" (em oposição a
//     um joystick cru) expor (D-pad, não um hat analógico), e é também a própria convenção
//     `SDL_GameControllerButton` do SDL para essas mesmas 4 direções apoiadas em hat de kernel.
//     `LeftStick`/`RightStick` são os botões de CLIQUE dos analógicos (`BTN_THUMBL`/`BTN_THUMBR`),
//     distintos dos EIXOS analógicos `LeftX/LeftY/RightX/RightY` do `GamepadAxis` abaixo.
enum class GamepadButton : std::uint8_t {
  South,
  East,
  West,
  North,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  LeftBumper,
  RightBumper,
  LeftStick,
  RightStick,
  Start,
  Back,
  Guide,
  Count // EN: sentinel, not a valid button. PT: sentinela, não é um botão válido.
};

// EN: Analog axes. `LeftX/LeftY/RightX/RightY` are the two sticks, normalized to `[-1, 1]`
//     (`GamepadDecoder::abs_normalized()`'s "stick formula" branch, src/gamepad_decode.cpp).
//     `LeftTrigger/RightTrigger` are the analog triggers, normalized to `[0, 1]` (the same
//     function's "trigger formula" branch) -- see `Gamepads::axis()`'s own doc-comment
//     (glintfx/gamepad.hpp, Agent B) for the exact per-axis range contract.
// PT: Eixos analógicos. `LeftX/LeftY/RightX/RightY` são os dois analógicos, normalizados para
//     `[-1, 1]` (o ramo "fórmula de stick" de `GamepadDecoder::abs_normalized()`,
//     src/gamepad_decode.cpp). `LeftTrigger/RightTrigger` são os gatilhos analógicos,
//     normalizados para `[0, 1]` (o ramo "fórmula de gatilho" da mesma função) -- ver o próprio
//     doc-comment de `Gamepads::axis()` (glintfx/gamepad.hpp, Agente B) pro contrato exato de
//     faixa por-eixo.
enum class GamepadAxis : std::uint8_t {
  LeftX,
  LeftY,
  RightX,
  RightY,
  LeftTrigger,
  RightTrigger,
  Count // EN: sentinel, not a valid axis. PT: sentinela, não é um eixo válido.
};

} // namespace glintfx
