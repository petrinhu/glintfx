// SPDX-License-Identifier: Apache-2.0
// EN: Minimal RmlUi SystemInterface for embed mode — provides the clock and (LOGTHR-1, Onda 2,
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decision D8) a dedup/throttle
//     LogMessage override. No GLFW/SDL dependency (the host owns window/input). LogMessage below
//     routes its already-deduped output through glintfx::log() (FW-LOG, W20,
//     glintfx/include/glintfx/log.hpp) instead of a raw std::fprintf(stderr, ...) -- the dedup
//     table still runs FIRST, exactly as before FW-LOG existed (see log.hpp's own "DEDUP/
//     THROTTLE HAPPENS BEFORE THE SINK" paragraph), so a host's glintfx::set_log_sink() sees the
//     curated stream, never the raw per-element-per-frame flood LOGTHR-1 exists to prevent.
// PT: SystemInterface mínimo do RmlUi para embed — provê o relógio e (LOGTHR-1, Onda 2,
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decisão D8) um override de
//     LogMessage com dedup/throttle. Sem dependência de GLFW/SDL (o host é dono de janela/input).
//     O LogMessage abaixo roteia a própria saída já deduplicada pelo glintfx::log() (FW-LOG, W20,
//     glintfx/include/glintfx/log.hpp) em vez de um std::fprintf(stderr, ...) cru -- a tabela de
//     dedup continua rodando PRIMEIRO, exatamente como antes do FW-LOG existir (ver o próprio
//     parágrafo "DEDUP/THROTTLE ACONTECE ANTES DO SINK" do log.hpp), então o
//     glintfx::set_log_sink() de um host vê o stream curado, nunca o flood cru
//     por-elemento-por-frame que o LOGTHR-1 existe para prevenir.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <glintfx/log.hpp>
#include <chrono>
#include <functional>
#include <string>
#include "../log_dedup.hpp"

namespace glintfx {

class SystemClock final : public Rml::SystemInterface {
public:
  SystemClock() : start_(std::chrono::steady_clock::now()) {}

  double GetElapsedTime() override {
    return std::chrono::duration<double>(
               std::chrono::steady_clock::now() - start_)
        .count();
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
      // EN: glintfx::log() -- the PLAIN, non-formatting entry point (FW-LOG, W20) -- `out_line`
      //     is RmlUi's own message text, which may legitimately contain a literal '%' (e.g. an
      //     element attribute value); see log.hpp's "TWO DISTINCT ENTRY POINTS" paragraph.
      // PT: glintfx::log() -- o ponto de entrada PLANO, sem formatação (FW-LOG, W20) --
      //     `out_line` é o próprio texto de mensagem do RmlUi, que pode legitimamente conter um
      //     '%' literal (ex.: um valor de atributo de elemento); ver o parágrafo "DOIS PONTOS DE
      //     ENTRADA DISTINTOS" do log.hpp.
      glintfx::log(is_error ? LogLevel::Error : LogLevel::Warn, out_line.c_str());
    }
    return true;
  }

  // EN: AUD-PUB-6g -- RmlUi calls this ONLY when Context::UpdateHoverChain's cached cursor name
  //     (Context::cursor_name, default "") changes; see UiLayer::set_cursor_callback's
  //     doc-comment (glintfx/include/glintfx/ui_layer.hpp) for the full "fires on change only"
  //     contract this forwards verbatim. Copies the functor locally before invoking it
  //     (AUD-TEC-3, same discipline as ClickEventListener::ProcessEvent in bootstrap.cpp): if
  //     the callback itself calls set_cursor_callback(...) reentrantly, that std::move-assigns
  //     a NEW value into cursor_cb_ while this invocation is still executing, which would
  //     destroy the std::function's current target out from under itself -- a local copy
  //     survives that reentrant reassignment.
  // PT: AUD-PUB-6g -- o RmlUi chama isto SÓ quando o nome de cursor cacheado pelo
  //     Context::UpdateHoverChain (Context::cursor_name, default "") muda; ver o doc-comment de
  //     UiLayer::set_cursor_callback (glintfx/include/glintfx/ui_layer.hpp) para o contrato
  //     completo "dispara só na mudança" que isto repassa ipsis litteris. Copia o functor
  //     localmente antes de invocá-lo (AUD-TEC-3, mesma disciplina de
  //     ClickEventListener::ProcessEvent em bootstrap.cpp): se o próprio callback chamar
  //     set_cursor_callback(...) reentrantemente, isso faz std::move-assign de um valor NOVO em
  //     cursor_cb_ enquanto esta invocação ainda está executando, o que destruiria o alvo atual
  //     do std::function debaixo de si mesmo -- uma cópia local sobrevive a essa reatribuição
  //     reentrante.
  void SetMouseCursor(const Rml::String& cursor_name) override {
    auto cb = cursor_cb_;
    if (cb) cb(cursor_name.c_str());
  }

  void set_cursor_callback(std::function<void(const char*)> cb) {
    cursor_cb_ = std::move(cb);
  }

private:
  std::chrono::steady_clock::time_point start_;
  LogDedup dedup_;
  std::function<void(const char*)> cursor_cb_;
};

} // namespace glintfx
