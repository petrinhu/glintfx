// SPDX-License-Identifier: Apache-2.0
// EN: FW-CLOCK implementation. See glintfx/include/glintfx/clock.hpp for the full contract
//     (epoch, resolution, threading, the honest value-proposition verdict). A one-line
//     delegation to std::chrono::steady_clock, converted to nanoseconds -- this is the ENTIRE
//     value this translation unit adds over calling the standard library directly: a fixed unit
//     (nanoseconds, regardless of steady_clock::period on this platform) and an integer
//     return-type ABI shape (see clock.hpp's point 1/3).
// PT: Implementação do FW-CLOCK. Ver glintfx/include/glintfx/clock.hpp pro contrato completo
//     (época, resolução, threading, o veredito honesto de valor próprio). Uma delegação de uma
//     linha pro std::chrono::steady_clock, convertida para nanossegundos -- isto é TUDO que esta
//     unidade de tradução adiciona sobre chamar a biblioteca padrão direto: uma unidade fixa
//     (nanossegundos, independente do steady_clock::period nesta plataforma) e uma forma de ABI
//     em tipo de retorno inteiro (ver o ponto 1/3 do clock.hpp).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/clock.hpp>

#include <chrono>
#include <thread>

namespace glintfx {

std::uint64_t monotonic_now_ns() {
  const auto now = std::chrono::steady_clock::now();
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
  return static_cast<std::uint64_t>(ns.count());
}

// EN: FW-SLEEP implementation. See glintfx/include/glintfx/clock.hpp for the full contract
//     (the frame_callback/vsync warning, the declared ceiling, unit/type precedent, threading).
//     A one-line delegation to std::this_thread::sleep_for -- this function adds nothing to the
//     standard library's own guarantee beyond the fixed std::uint64_t-milliseconds ABI shape
//     (same "thin wrapper, integer ABI" discipline as monotonic_now_ns() above).
// PT: Implementação do FW-SLEEP. Ver glintfx/include/glintfx/clock.hpp pro contrato completo
//     (o aviso frame_callback/vsync, o teto declarado, o precedente de unidade/tipo,
//     threading). Uma delegação de uma linha pro std::this_thread::sleep_for -- esta função não
//     adiciona nada à própria garantia da biblioteca padrão além da forma de ABI fixa em
//     std::uint64_t-milissegundos (mesma disciplina "wrapper fino, ABI inteira" do
//     monotonic_now_ns() acima).
void sleep_ms(std::uint64_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace glintfx
