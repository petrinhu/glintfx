// SPDX-License-Identifier: MPL-2.0
// EN: Minimal RmlUi SystemInterface for embed mode — provides only the clock.
//     No GLFW/SDL dependency (the host owns window/input).
// PT: SystemInterface mínimo do RmlUi para embed — provê só o relógio.
//     Sem dependência de GLFW/SDL (o host é dono de janela/input).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <chrono>

namespace glintfx {

class SystemClock final : public Rml::SystemInterface {
public:
  SystemClock() : start_(std::chrono::steady_clock::now()) {}

  double GetElapsedTime() override {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start_).count();
  }

private:
  std::chrono::steady_clock::time_point start_;
};

} // namespace glintfx
