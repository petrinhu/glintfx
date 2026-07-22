// SPDX-License-Identifier: MPL-2.0
// EN: Minimal RmlUi SystemInterface for embed mode — provides the clock and (LOGTHR-1, Onda 2,
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decision D8) a dedup/throttle
//     LogMessage override. No GLFW/SDL dependency (the host owns window/input).
// PT: SystemInterface mínimo do RmlUi para embed — provê o relógio e (LOGTHR-1, Onda 2,
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decisão D8) um override de
//     LogMessage com dedup/throttle. Sem dependência de GLFW/SDL (o host é dono de janela/input).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <chrono>
#include <cstdio>
#include <string>
#include "log_dedup.hpp"

namespace glintfx {

class SystemClock final : public Rml::SystemInterface {
public:
  SystemClock() : start_(std::chrono::steady_clock::now()) {}

  double GetElapsedTime() override {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start_).count();
  }

  // EN: LOGTHR-1 (D8) -- overrides RmlUi's default LogMessage (which would otherwise print
  //     every call unconditionally via LogDefault, the source of the per-element-per-frame
  //     flood). Delegates the print-or-suppress decision to the pure LogDedup helper; the
  //     translation from Rml::Log::Type to a plain int + the is_error flag happens HERE (the
  //     one place in this file allowed to know about RmlUi types), keeping log_dedup.hpp itself
  //     free of any RmlUi dependency. Returns true unconditionally (matches LogDefault's own
  //     non-Win32 behaviour: this codebase targets Linux, see CLAUDE.md's stack table -- there
  //     is no Win32 MessageBox "continue execution?" path to preserve here).
  // PT: Sobrescreve o LogMessage padrão do RmlUi (que do contrário imprimiria toda chamada
  //     incondicionalmente via LogDefault, a origem do flood por-elemento-por-frame). Delega a
  //     decisão imprimir-ou-suprimir ao helper puro LogDedup; a tradução de Rml::Log::Type para
  //     um int simples + a flag is_error acontece AQUI (o único lugar deste arquivo autorizado a
  //     conhecer tipos do RmlUi), mantendo log_dedup.hpp livre de qualquer dependência de RmlUi.
  //     Retorna true incondicionalmente (casa com o próprio comportamento não-Win32 do
  //     LogDefault: este código-base mira Linux, ver a tabela de stack do CLAUDE.md -- não há
  //     caminho de MessageBox "continuar execução?" do Win32 a preservar aqui).
  bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
    const bool is_error = (type == Rml::Log::LT_ERROR || type == Rml::Log::LT_ASSERT);
    std::string out_line;
    if (dedup_.should_print(static_cast<int>(type), message, is_error, out_line)) {
      std::fprintf(stderr, "%s\n", out_line.c_str());
    }
    return true;
  }

private:
  std::chrono::steady_clock::time_point start_;
  LogDedup dedup_;
};

} // namespace glintfx
