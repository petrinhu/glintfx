// SPDX-License-Identifier: MPL-2.0
// EN: Neutral, host-agnostic input event — full definition arrives in T4.
//     This stub provides only the forward declaration needed by ui_layer.hpp (T2).
// PT: Evento de entrada neutro, agnóstico de host — definição completa chega na T4.
//     Este stub provê apenas a forward declaration necessária para ui_layer.hpp (T2).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
namespace glintfx {
// EN: Full struct definition (Type, x/y, button, key, modifiers, text, width/height)
//     added in T4 when process_event() is implemented.
// PT: Definição completa da struct (Type, x/y, button, key, modifiers, text, width/height)
//     adicionada na T4 quando process_event() for implementado.
struct UiEvent;
} // namespace glintfx
