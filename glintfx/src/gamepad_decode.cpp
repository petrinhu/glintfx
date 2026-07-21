// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of the pure evdev decode core. See gamepad_decode.hpp's own header comment
//     for the wire format, state model, and the stick-vs-trigger normalization decision; comments
//     here focus on the "how" of each function, not the "why" (already spelled out there).
// PT: Implementação do núcleo de decode puro evdev. Ver o próprio comentário de cabeçalho do
//     gamepad_decode.hpp pro formato de fio, modelo de estado, e a decisão de normalização
//     stick-vs-gatilho; os comentários aqui focam no "como" de cada função, não no "porquê" (já
//     detalhado lá).
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_decode.hpp"

#include <algorithm>
#include <cstring>

namespace glintfx {

GamepadDecoder::GamepadDecoder() = default;

void GamepadDecoder::configure_abs(std::uint16_t code, const EvAbsInfo& info) {
  if (code >= kAbsStates) {
    return; // EN: hard guard, never indexes out of bounds. PT: guarda dura, nunca indexa fora dos limites.
  }
  abs_state_[code] = info;
}

void GamepadDecoder::feed_bytes(const void* data, std::size_t len) {
  if (data == nullptr || len == 0) {
    return;
  }
  const unsigned char* bytes = static_cast<const unsigned char*>(data);
  pending_.insert(pending_.end(), bytes, bytes + len);

  // EN: Drain every fully-buffered event, in order, regardless of how many chunks it took to
  //     accumulate 24 bytes -- this loop is what makes 1-byte-at-a-time feeding and 7+17-byte
  //     feeding produce byte-for-byte the same decoded events as a single 24*N-byte feed.
  // PT: Drena todo evento já completamente bufferizado, em ordem, independente de quantos chunks
  //     foram necessários pra acumular 24 bytes -- este laço é o que faz a alimentação 1 byte por
  //     vez e a alimentação de 7+17 bytes produzirem, byte a byte, os mesmos eventos decodificados
  //     que uma única alimentação de 24*N bytes.
  while (pending_.size() >= kEventSize) {
    std::uint16_t type  = 0;
    std::uint16_t code  = 0;
    std::int32_t  value = 0;
    std::memcpy(&type, pending_.data() + kTypeOffset, sizeof(type));
    std::memcpy(&code, pending_.data() + kCodeOffset, sizeof(code));
    std::memcpy(&value, pending_.data() + kValueOffset, sizeof(value));
    process_event(type, code, value);
    pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(kEventSize));
  }
}

void GamepadDecoder::process_event(std::uint16_t type, std::uint16_t code, std::int32_t value) {
  switch (type) {
    case EV_KEY:
      if (code >= kKeyStates) {
        return; // EN: code >= KEY_MAX -- hard guard, never indexed. PT: code >= KEY_MAX -- guarda dura, nunca indexado.
      }
      if (value == 0) {
        key_state_[code] = false;
      } else if (value == 1) {
        key_state_[code] = true;
      }
      // EN: value == 2 (autorepeat) or any other value (hostile INT32_MIN/MAX etc.): no state
      //     change, by design -- an unrecognised value never guesses a press/release.
      // PT: value == 2 (autorepeat) ou qualquer outro valor (hostil INT32_MIN/MAX etc.): nenhuma
      //     mudança de estado, por design -- um valor não reconhecido nunca chuta press/release.
      break;

    case EV_ABS:
      if (code >= kAbsStates) {
        return; // EN: code >= ABS_MAX -- hard guard. PT: code >= ABS_MAX -- guarda dura.
      }
      abs_state_[code].value = value;
      break;

    case EV_SYN:
      if (code == SYN_DROPPED) {
        sync_lost_ = true;
      }
      // EN: SYN_REPORT and any other EV_SYN code: documented no-op this slice.
      // PT: SYN_REPORT e qualquer outro código EV_SYN: no-op documentado nesta fatia.
      break;

    default:
      break; // EN: any other event type is ignored. PT: qualquer outro tipo de evento é ignorado.
  }
}

bool GamepadDecoder::key_state(std::uint16_t code) const {
  if (code >= kKeyStates) {
    return false;
  }
  return key_state_[code];
}

std::int32_t GamepadDecoder::abs_value(std::uint16_t code) const {
  if (code >= kAbsStates) {
    return 0;
  }
  return abs_state_[code].value;
}

float GamepadDecoder::abs_normalized(std::uint16_t code) const {
  if (code >= kAbsStates) {
    return 0.0f;
  }
  const EvAbsInfo& info = abs_state_[code];
  if (info.min >= info.max) {
    return 0.0f; // EN: degenerate-range guard -- never divides by zero. PT: guarda de faixa degenerada -- nunca divide por zero.
  }
  const float value = static_cast<float>(info.value);
  const float min    = static_cast<float>(info.min);
  const float max    = static_cast<float>(info.max);
  if (info.min < 0) {
    // EN: stick formula, symmetric range. PT: fórmula stick, faixa simétrica.
    const float normalized = 2.0f * (value - min) / (max - min) - 1.0f;
    return std::clamp(normalized, -1.0f, 1.0f);
  }
  // EN: trigger formula, zero-based range. PT: fórmula gatilho, faixa baseada em zero.
  const float normalized = (value - min) / (max - min);
  return std::clamp(normalized, 0.0f, 1.0f);
}

bool GamepadDecoder::abs_is_signed(std::uint16_t code) const {
  if (code >= kAbsStates) {
    return false;
  }
  return abs_state_[code].min < 0;
}

std::int32_t GamepadDecoder::clamp_hat(std::int32_t raw) {
  if (raw < 0) return -1;
  if (raw > 0) return 1;
  return 0;
}

bool GamepadDecoder::dpad_up() const    { return clamp_hat(abs_value(ABS_HAT0Y)) < 0; }
bool GamepadDecoder::dpad_down() const  { return clamp_hat(abs_value(ABS_HAT0Y)) > 0; }
bool GamepadDecoder::dpad_left() const  { return clamp_hat(abs_value(ABS_HAT0X)) < 0; }
bool GamepadDecoder::dpad_right() const { return clamp_hat(abs_value(ABS_HAT0X)) > 0; }

bool GamepadDecoder::sync_lost() const { return sync_lost_; }
void GamepadDecoder::clear_sync_lost() { sync_lost_ = false; }

} // namespace glintfx
