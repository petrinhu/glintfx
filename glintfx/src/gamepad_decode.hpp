// SPDX-License-Identifier: MPL-2.0
// EN: A2-GAMEPAD (framework-2D, ADR-0015/ADR-0016 module "gamepad") -- the PURE decode core:
//     `GamepadDecoder` reassembles a byte stream (as `read()` on an evdev `/dev/input/eventN`
//     node hands it back, in whatever chunk sizes the kernel/consumer happens to deliver) into
//     `struct input_event` records and tracks per-code button/axis state. No `fd`, no `ioctl`, no
//     directory scan -- this is dependency-injection-by-construction: the device layer (Agent B,
//     src/gamepad.cpp) is the ONLY caller that ever touches a real file descriptor; everything in
//     THIS file is fed synthetic buffers, in tests and in production alike, which is exactly what
//     makes it headless-testable (see tests/gamepad_decode_{sanity,hostile}.cpp).
//
//     This header lives under `src/` (INTERNAL, never installed), so it is free to
//     `#include <linux/input.h>` directly (AGENTS.md encapsulation rule: zero third-party/kernel
//     type may cross the PUBLIC `include/glintfx/` boundary -- `gamepad_types.hpp` is that public
//     surface and stays kernel-type-free; this file is not part of it).
//
//     WIRE FORMAT: `struct input_event` on x86-64 userspace is 24 bytes -- `struct timeval time`
//     (2x `long`, 16 bytes) + `__u16 type` + `__u16 code` + `__s32 value` (8 bytes) = 24. This
//     decoder does NOT reinterpret_cast a raw byte buffer as `struct input_event*` (that would be
//     both a strict-aliasing violation and an alignment assumption `feed_bytes()`'s "any chunk
//     size, including 1 byte at a time" contract explicitly breaks) -- it `memcpy`s `type`/
//     `code`/`value` out of the buffer at their known fixed byte offsets (16/18/20) instead, which
//     is well-defined regardless of the buffer's alignment or the caller's chunking.
//
//     STATE MODEL: `key_state_[code]` is a plain boolean per `EV_KEY` code (sized to
//     `KEY_MAX + 1`, `code >= KEY_MAX` is a hard guard, never indexed). `abs_state_[code]` is an
//     `EvAbsInfo` per `EV_ABS` code (sized to `ABS_MAX + 1`, same hard-guard discipline) --
//     `configure_abs()` sets the *range* (`min`/`max`/`fuzz`/`flat`/`resolution`, normally read
//     once via `EVIOCGABS` by the device layer at scan time); `feed_bytes()`'s `EV_ABS` handling
//     only ever updates the `.value` field of an already-allocated slot, so an axis that was never
//     `configure_abs()`'d (e.g. a hat, which this slice does not `configure_abs` at all -- see
//     `dpad_*()` below) still safely records its raw value, just with `min == max == 0`
//     (`abs_normalized()`'s degenerate-range guard then correctly reports `0.0f` for it, which is
//     the right answer for a code this decoder was never told the range of).
//
//     `abs_normalized()`'s STICK-vs-TRIGGER formula selection is a clean-room ENGINEERING
//     DECISION of this file (not literally spelled out as a selector in the A2-GAMEPAD plan,
//     which only gives the two formulas): the shape of the CONFIGURED RANGE decides, not a
//     hardcoded evdev-code table -- `min < 0` (a range straddling/centred near zero, e.g. a
//     typical dual-stick axis `-32768..32767`) selects the STICK formula
//     (`2*(v-min)/(max-min) - 1`, clamped `[-1, 1]`); `min >= 0` (a range starting at zero, e.g. a
//     typical analog-trigger axis `0..255`) selects the TRIGGER formula (`(v-min)/(max-min)`,
//     clamped `[0, 1]`). This keeps the decoder ignorant of `GamepadAxis`/the mapping module's
//     enum entirely (no upward dependency), and correctly covers BOTH of the kernel gamepad.rst
//     spec's trigger placements (D5 of the plan: `ABS_BRAKE`/`ABS_GAS`, or the `ABS_Z`/`ABS_RZ`
//     fallback) without this file needing to know which evdev code means "trigger" -- every one
//     of those axes is reported by real hardware/drivers with `minimum == 0`.
//
//     `SYN_DROPPED` sets a STICKY flag (`sync_lost()`), cleared only by `clear_sync_lost()` --
//     never auto-cleared by `feed_bytes()` itself. Resyncing (re-reading full key/abs state via
//     `EVIOCGKEY`/`EVIOCGABS`) is the device layer's job (D8 of the plan); this decoder's only
//     responsibility is to raise the flag reliably and never silently drop it. `SYN_REPORT` (and
//     any other `EV_SYN` code) is a documented no-op THIS SLICE -- state is applied event-by-event
//     as `feed_bytes()` parses each `input_event`, not batched until a `SYN_REPORT` boundary (see
//     the plan's own D-column note, section 2.3).
// PT: A2-GAMEPAD (framework-2D, módulo "gamepad" da ADR-0015/ADR-0016) -- o núcleo de decode
//     PURO: `GamepadDecoder` reagrupa um stream de bytes (como o `read()` num node evdev
//     `/dev/input/eventN` devolve, em qualquer tamanho de chunk que o kernel/consumidor entregue)
//     em registros `struct input_event` e rastreia o estado de botão/eixo por código. Sem `fd`,
//     sem `ioctl`, sem varredura de diretório -- isto é injeção-de-dependência-por-construção: a
//     camada de device (Agente B, src/gamepad.cpp) é a ÚNICA chamadora que de fato toca um
//     descritor de arquivo real; tudo NESTE arquivo é alimentado com buffers sintéticos, tanto em
//     teste quanto em produção, o que é exatamente o que o torna testável headless (ver
//     tests/gamepad_decode_{sanity,hostile}.cpp).
//
//     Este header vive sob `src/` (INTERNO, nunca instalado), então é livre para incluir
//     `<linux/input.h>` diretamente (regra de encapsulamento do AGENTS.md: zero tipo de
//     terceiro/kernel pode cruzar a fronteira PÚBLICA de `include/glintfx/` -- `gamepad_types.hpp`
//     é essa superfície pública e continua livre de tipo de kernel; este arquivo não faz parte
//     dela).
//
//     FORMATO DE FIO: `struct input_event` em userspace x86-64 tem 24 bytes -- `struct timeval
//     time` (2x `long`, 16 bytes) + `__u16 type` + `__u16 code` + `__s32 value` (8 bytes) = 24.
//     Este decoder NÃO faz `reinterpret_cast` de um buffer de bytes cru pra `struct
//     input_event*` (isso seria tanto violação de strict-aliasing quanto uma suposição de
//     alinhamento que o contrato "qualquer tamanho de chunk, incluindo 1 byte por vez" do
//     `feed_bytes()` explicitamente quebra) -- em vez disso ele faz `memcpy` de `type`/`code`/
//     `value` do buffer nos offsets de byte fixos conhecidos (16/18/20), o que é bem-definido
//     independente do alinhamento do buffer ou do chunking do chamador.
//
//     MODELO DE ESTADO: `key_state_[code]` é um booleano simples por código `EV_KEY`
//     (dimensionado pra `KEY_MAX + 1`, `code >= KEY_MAX` é guarda dura, nunca indexado).
//     `abs_state_[code]` é um `EvAbsInfo` por código `EV_ABS` (dimensionado pra `ABS_MAX + 1`,
//     mesma disciplina de guarda dura) -- `configure_abs()` define a *faixa* (`min`/`max`/
//     `fuzz`/`flat`/`resolution`, normalmente lida uma vez via `EVIOCGABS` pela camada de device
//     em tempo de varredura); o tratamento de `EV_ABS` do `feed_bytes()` só atualiza o campo
//     `.value` de um slot já alocado, então um eixo nunca `configure_abs()`'ado (ex.: um hat, que
//     esta fatia não `configure_abs` nenhum -- ver `dpad_*()` abaixo) ainda registra o valor cru
//     com segurança, só que com `min == max == 0` (a guarda de faixa degenerada do
//     `abs_normalized()` então reporta corretamente `0.0f` pra ele, que é a resposta certa pra um
//     código do qual este decoder nunca foi informado a faixa).
//
//     A seleção de fórmula STICK-vs-GATILHO do `abs_normalized()` é uma DECISÃO DE ENGENHARIA
//     clean-room deste arquivo (não literalmente escrita como um seletor no plano do A2-GAMEPAD,
//     que só dá as duas fórmulas): a FORMA DA FAIXA CONFIGURADA decide, não uma tabela fixa de
//     código-evdev -- `min < 0` (uma faixa a cavalo/centrada perto de zero, ex.: um eixo típico de
//     analógico duplo `-32768..32767`) seleciona a fórmula STICK (`2*(v-min)/(max-min) - 1`,
//     clampada `[-1, 1]`); `min >= 0` (uma faixa começando em zero, ex.: um eixo típico de gatilho
//     analógico `0..255`) seleciona a fórmula GATILHO (`(v-min)/(max-min)`, clampada `[0, 1]`).
//     Isso mantém o decoder ignorante do `GamepadAxis`/enum do módulo de mapeamento por completo
//     (sem dependência ascendente), e cobre corretamente AMBOS os posicionamentos de gatilho da
//     spec gamepad.rst do kernel (D5 do plano: `ABS_BRAKE`/`ABS_GAS`, ou o fallback
//     `ABS_Z`/`ABS_RZ`) sem que este arquivo precise saber qual código evdev significa "gatilho"
//     -- todo eixo desses é reportado por hardware/drivers reais com `minimum == 0`.
//
//     `SYN_DROPPED` seta uma flag PEGAJOSA (`sync_lost()`), limpa só por `clear_sync_lost()` --
//     nunca auto-limpa pelo próprio `feed_bytes()`. Ressincronizar (reler estado completo de
//     tecla/abs via `EVIOCGKEY`/`EVIOCGABS`) é trabalho da camada de device (D8 do plano); a única
//     responsabilidade deste decoder é levantar a flag de forma confiável e nunca derrubá-la em
//     silêncio. `SYN_REPORT` (e qualquer outro código `EV_SYN`) é no-op documentado NESTA FATIA --
//     o estado é aplicado evento-a-evento conforme `feed_bytes()` faz parse de cada
//     `input_event`, não em lote até uma fronteira de `SYN_REPORT` (ver a própria nota da coluna D
//     do plano, seção 2.3).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <linux/input.h>

namespace glintfx {

// EN: Own mirror of `struct input_absinfo` (EVIOCGABS/EVIOCSABS), field names matched to the
//     A2-GAMEPAD plan's literal `EvAbsInfo{value,min,max,fuzz,flat,resolution}` (section 2.3).
//     Kept as our own struct (not a `using EvAbsInfo = input_absinfo` alias) so this file's
//     internal state array has a stable, self-documented shape independent of the kernel UAPI
//     struct's own field names (`minimum`/`maximum`, not `min`/`max`).
// PT: Espelho próprio de `struct input_absinfo` (EVIOCGABS/EVIOCSABS), nomes de campo casados com
//     o literal `EvAbsInfo{value,min,max,fuzz,flat,resolution}` do plano A2-GAMEPAD (seção 2.3).
//     Mantido como struct própria (não um alias `using EvAbsInfo = input_absinfo`) para que o
//     array de estado interno deste arquivo tenha uma forma estável e autodocumentada,
//     independente dos próprios nomes de campo da struct UAPI do kernel (`minimum`/`maximum`, não
//     `min`/`max`).
struct EvAbsInfo {
  std::int32_t value      = 0;
  std::int32_t min        = 0;
  std::int32_t max        = 0;
  std::int32_t fuzz       = 0;
  std::int32_t flat       = 0;
  std::int32_t resolution = 0;
};

class GamepadDecoder {
public:
  GamepadDecoder();

  // EN: Sets the RANGE (`min`/`max`/`fuzz`/`flat`/`resolution`) for `code` -- normally called
  //     once per axis by the device layer right after `EVIOCGABS`, before any `feed_bytes()` for
  //     that pad. `code >= ABS_MAX`: ignored (hard guard, mirrors feed_bytes()'s own EV_ABS
  //     guard -- defensive here too even though the device layer is the only expected caller,
  //     never trust an index range purely by caller identity).
  // PT: Define a FAIXA (`min`/`max`/`fuzz`/`flat`/`resolution`) de `code` -- normalmente chamado
  //     uma vez por eixo pela camada de device logo após o `EVIOCGABS`, antes de qualquer
  //     `feed_bytes()` daquele pad. `code >= ABS_MAX`: ignorado (guarda dura, espelha a própria
  //     guarda de EV_ABS do feed_bytes() -- defensivo aqui também mesmo a camada de device sendo
  //     a única chamadora esperada, nunca confiar numa faixa de índice só pela identidade do
  //     chamador).
  void configure_abs(std::uint16_t code, const EvAbsInfo& info);

  // EN: Feeds `len` bytes of raw evdev wire data, in ANY chunk size (0, 1, 7, 17, megabytes --
  //     all legal). Bytes are appended to an internal reassembly buffer; every time it holds
  //     `>= 24` bytes, the front 24 are parsed as one `input_event` and consumed, repeatedly,
  //     until fewer than 24 remain (which stay buffered for the NEXT call). `data == nullptr` or
  //     `len == 0`: no-op.
  // PT: Alimenta `len` bytes de dado cru evdev, em QUALQUER tamanho de chunk (0, 1, 7, 17,
  //     megabytes -- todos legais). Os bytes são anexados a um buffer interno de reagrupamento;
  //     toda vez que ele acumula `>= 24` bytes, os 24 da frente são parseados como um
  //     `input_event` e consumidos, repetidamente, até sobrarem menos de 24 (que ficam
  //     armazenados pra PRÓXIMA chamada). `data == nullptr` ou `len == 0`: no-op.
  void feed_bytes(const void* data, std::size_t len);

  // EN: `code >= KEY_MAX`: returns `false` (fail-high, never indexes out of bounds).
  // PT: `code >= KEY_MAX`: retorna `false` (fail-high, nunca indexa fora dos limites).
  bool key_state(std::uint16_t code) const;

  // EN: Raw, un-normalized last-reported `EV_ABS` value. `code >= ABS_MAX`: returns `0`.
  // PT: Valor cru, não-normalizado, do último `EV_ABS` reportado. `code >= ABS_MAX`: retorna `0`.
  std::int32_t abs_value(std::uint16_t code) const;

  // EN: Normalized axis value -- see this file's header comment for the stick-vs-trigger formula
  //     selection. `code >= ABS_MAX`, or a degenerate configured range (`min >= max`, including
  //     the "never configure_abs()'d" default of `min == max == 0`): returns `0.0f`.
  // PT: Valor de eixo normalizado -- ver o comentário de cabeçalho deste arquivo pra seleção de
  //     fórmula stick-vs-gatilho. `code >= ABS_MAX`, ou uma faixa configurada degenerada
  //     (`min >= max`, incluindo o default "nunca configure_abs()'ado" de `min == max == 0`):
  //     retorna `0.0f`.
  float abs_normalized(std::uint16_t code) const;

  // EN: `true` when `code`'s configured range is signed (`min < 0`, the "stick" shape) --
  //     exposed for the mapping module's axis-inversion logic (src/gamepad_mapping.cpp), which
  //     needs to know which of the two `abs_normalized()` formula branches applied to correctly
  //     invert a `~`-suffixed DB entry. `code >= ABS_MAX`: returns `false`.
  // PT: `true` quando a faixa configurada de `code` tem sinal (`min < 0`, a forma "stick") --
  //     exposto pra lógica de inversão de eixo do módulo de mapeamento (src/gamepad_mapping.cpp),
  //     que precisa saber qual dos dois ramos de fórmula do `abs_normalized()` se aplicou pra
  //     inverter corretamente uma entrada de DB com sufixo `~`. `code >= ABS_MAX`: retorna
  //     `false`.
  bool abs_is_signed(std::uint16_t code) const;

  // EN: D-pad, normalized from `ABS_HAT0X`/`ABS_HAT0Y` (kernel convention: `X < 0` = Left,
  //     `X > 0` = Right, `Y < 0` = Up, `Y > 0` = Down -- same sign convention `joydev`/`jstest`
  //     use for a digital hat). Values other than exactly `{-1, 0, +1}` (hostile/malformed
  //     `EV_ABS` payloads) are clamped to that set FIRST (`raw < 0` -> `-1`, `raw > 0` -> `+1`),
  //     so e.g. `INT32_MIN`/`INT32_MAX` never produce anything but a clean Up/Down/Left/Right
  //     boolean. `ABS_HAT0X`/`Y` need NOT be `configure_abs()`'d for these to work correctly --
  //     they read the raw last-reported value directly, not the normalized one.
  // PT: D-pad, normalizado a partir de `ABS_HAT0X`/`ABS_HAT0Y` (convenção do kernel: `X < 0` =
  //     Left, `X > 0` = Right, `Y < 0` = Up, `Y > 0` = Down -- mesma convenção de sinal que
  //     `joydev`/`jstest` usam pra um hat digital). Valores diferentes de exatamente `{-1, 0, +1}`
  //     (payloads `EV_ABS` hostis/malformados) são clampados pra esse conjunto PRIMEIRO
  //     (`raw < 0` -> `-1`, `raw > 0` -> `+1`), então ex.: `INT32_MIN`/`INT32_MAX` nunca produzem
  //     nada além de um booleano limpo de Up/Down/Left/Right. `ABS_HAT0X`/`Y` NÃO precisam ser
  //     `configure_abs()`'ados pra isso funcionar corretamente -- eles leem o valor cru do último
  //     reportado diretamente, não o normalizado.
  bool dpad_up() const;
  bool dpad_down() const;
  bool dpad_left() const;
  bool dpad_right() const;

  // EN: `SYN_DROPPED` sticky flag (D8 of the plan) -- see this file's header comment.
  // PT: Flag pegajosa de `SYN_DROPPED` (D8 do plano) -- ver o comentário de cabeçalho deste
  //     arquivo.
  bool sync_lost() const;
  void clear_sync_lost();

private:
  void process_event(std::uint16_t type, std::uint16_t code, std::int32_t value);
  static std::int32_t clamp_hat(std::int32_t raw);

  // EN: 16 (struct timeval, 2x long on x86-64) + 2 (__u16 type) + 2 (__u16 code) +
  //     4 (__s32 value) = 24. See this file's header comment for why this is a fixed byte-offset
  //     memcpy, not a reinterpret_cast of `struct input_event`.
  // PT: 16 (struct timeval, 2x long em x86-64) + 2 (__u16 type) + 2 (__u16 code) +
  //     4 (__s32 value) = 24. Ver o comentário de cabeçalho deste arquivo pro porquê disto ser um
  //     memcpy de offset de byte fixo, não um reinterpret_cast de `struct input_event`.
  static constexpr std::size_t kEventSize   = 24;
  static constexpr std::size_t kTypeOffset  = 16;
  static constexpr std::size_t kCodeOffset  = 18;
  static constexpr std::size_t kValueOffset = 20;

  static constexpr std::size_t kKeyStates = static_cast<std::size_t>(KEY_MAX) + 1;
  static constexpr std::size_t kAbsStates = static_cast<std::size_t>(ABS_MAX) + 1;

  std::vector<unsigned char>       pending_;
  std::array<bool, kKeyStates>     key_state_{};
  std::array<EvAbsInfo, kAbsStates> abs_state_{};
  bool                              sync_lost_ = false;
};

} // namespace glintfx
