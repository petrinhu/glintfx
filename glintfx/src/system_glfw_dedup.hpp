// SPDX-License-Identifier: Apache-2.0
// EN: LOGTHR-1 (Onda 2, docs/superpowers/plans/2026-07-22-onda2-input-host.md, decision D8) --
//     thin subclass of RmlUi's upstream SystemInterface_GLFW (RmlUi_Platform_GLFW.h, pinned,
//     not patched) that adds ONLY a dedup/throttle LogMessage override. Every other method
//     (GetElapsedTime, SetMouseCursor, Set/GetClipboardText) is inherited unchanged -- this
//     file exists solely to close the App-mode half of the LOGTHR-1 flood fix (see
//     src/system_clock.hpp for the embed-mode half). GLFW=ON only (App is the only consumer;
//     app.cpp swaps its one construction site from SystemInterface_GLFW to this type).
// PT: LOGTHR-1 (Onda 2, docs/superpowers/plans/2026-07-22-onda2-input-host.md, decisão D8) --
//     subclasse fina do SystemInterface_GLFW upstream do RmlUi (RmlUi_Platform_GLFW.h, pinado,
//     não remendado) que só acrescenta um override de LogMessage com dedup/throttle. Todo outro
//     método (GetElapsedTime, SetMouseCursor, Set/GetClipboardText) é herdado sem mudança --
//     este arquivo existe só para fechar a metade App-mode do fix de flood do LOGTHR-1 (ver
//     src/system_clock.hpp para a metade embed-mode). Só GLFW=ON (App é o único consumidor;
//     app.cpp troca seu único ponto de construção de SystemInterface_GLFW para este tipo).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include "RmlUi_Platform_GLFW.h"
#include <glintfx/log.hpp>
#include <functional>
#include <string>
#include "log_dedup.hpp"

namespace glintfx {

// EN: See src/system_clock.hpp's own LogMessage override for the full rationale (D8 policy,
//     why the Rml::Log::Type -> int + is_error translation happens here and not in
//     log_dedup.hpp, and why the return value is unconditionally true) -- identical here, only
//     the base class differs (SystemInterface_GLFW instead of SystemClock). Also identical: this
//     override routes its already-deduped output through glintfx::log() (FW-LOG, W20,
//     glintfx/include/glintfx/log.hpp) instead of a raw std::fprintf(stderr, ...) -- see
//     system_clock.hpp's own header comment for why the dedup table still running FIRST matters.
// PT: Ver o próprio override de LogMessage de src/system_clock.hpp para o racional completo
//     (política D8, por que a tradução Rml::Log::Type -> int + is_error acontece aqui e não em
//     log_dedup.hpp, e por que o valor de retorno é incondicionalmente true) -- idêntico aqui,
//     só a classe-base difere (SystemInterface_GLFW em vez de SystemClock). Também idêntico:
//     este override roteia a própria saída já deduplicada pelo glintfx::log() (FW-LOG, W20,
//     glintfx/include/glintfx/log.hpp) em vez de um std::fprintf(stderr, ...) cru -- ver o
//     próprio comentário de cabeçalho do system_clock.hpp pro motivo da tabela de dedup continuar
//     rodando PRIMEIRO importar.
class SystemInterfaceGlfwDedup final : public SystemInterface_GLFW {
public:
  explicit SystemInterfaceGlfwDedup(GLFWwindow* window) : SystemInterface_GLFW(window) {}

  bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
    const bool is_error = (type == Rml::Log::LT_ERROR || type == Rml::Log::LT_ASSERT);
    std::string out_line;
    if (dedup_.should_print(static_cast<int>(type), message, is_error, out_line)) {
      glintfx::log(is_error ? LogLevel::Error : LogLevel::Warn, out_line.c_str());
    }
    return true;
  }

  // EN: AUD-PUB-6g -- observer, not replacement: calls the INHERITED
  //     SystemInterface_GLFW::SetMouseCursor() FIRST (the real glfwSetCursor the window already
  //     relied on, unchanged), THEN forwards the same name to the registered callback. Same
  //     "fires on change only" contract as SystemClock::SetMouseCursor (embed half, see
  //     src/rml/system_clock.hpp) -- RmlUi's Context::UpdateHoverChain is what decides WHEN this
  //     is called, identically for both SystemInterface implementations. Local-copy-before-
  //     invoke reentrancy guard (AUD-TEC-3): see SystemClock::SetMouseCursor's doc-comment for
  //     the full use-after-free rationale this mirrors.
  // PT: AUD-PUB-6g -- observador, não substituição: chama PRIMEIRO o
  //     SystemInterface_GLFW::SetMouseCursor() HERDADO (o glfwSetCursor real do qual a janela
  //     já dependia, sem mudança), DEPOIS repassa o mesmo nome ao callback registrado. Mesmo
  //     contrato "dispara só na mudança" de SystemClock::SetMouseCursor (metade embed, ver
  //     src/rml/system_clock.hpp) -- o Context::UpdateHoverChain do RmlUi é quem decide QUANDO
  //     isto é chamado, identicamente para as duas implementações de SystemInterface. Guarda de
  //     reentrância cópia-local-antes-de-invocar (AUD-TEC-3): ver o doc-comment de
  //     SystemClock::SetMouseCursor para o racional completo de use-after-free que isto espelha.
  void SetMouseCursor(const Rml::String& cursor_name) override {
    SystemInterface_GLFW::SetMouseCursor(cursor_name);
    auto cb = cursor_cb_;
    if (cb) cb(cursor_name.c_str());
  }

  void set_cursor_callback(std::function<void(const char*)> cb) {
    cursor_cb_ = std::move(cb);
  }

private:
  LogDedup dedup_;
  std::function<void(const char*)> cursor_cb_;
};

} // namespace glintfx
