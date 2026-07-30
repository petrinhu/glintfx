// SPDX-License-Identifier: MPL-2.0
// EN: FW-LOG implementation. See glintfx/include/glintfx/log.hpp for the full public contract
//     (THREADING/REENTRANCY/HARDENING). This file has ZERO RmlUi/GLFW/GL dependency -- same
//     independence class as src/log_dedup.hpp (which THIS file does not need either: dedup
//     already happened upstream, in draw2d.cpp/system_clock.hpp/system_glfw_dedup.hpp, before a
//     message ever reaches log()/log_info()/log_warn()/log_error() here -- see log.hpp's own
//     "DEDUP/THROTTLE HAPPENS BEFORE THE SINK" paragraph).
//
//     Sink storage is a single function-local `static LogSink` (sink_storage() below), not a
//     namespace-scope global -- avoids any static-initialization-order question for a call that
//     could in principle happen from another translation unit's own static initializer (defensive;
//     nothing in this library currently logs from a static initializer, but the function-local
//     static costs nothing and removes the question entirely, same reasoning a Meyers singleton
//     is chosen for over a plain global in this class of "one shared piece of process-wide
//     state" utility).
//
//     `log()` and the 3 printf-style wrappers all take a LOCAL COPY of the currently-installed
//     sink (`LogSink sink_copy = sink_storage();`) BEFORE invoking it -- the REENTRANCY contract
//     log.hpp documents: a sink that calls `set_log_sink()` (even to install itself again) or
//     calls back into `log()`/`log_info()`/... from inside its own body never observes a
//     dangling/moved-from `std::function`, because the in-flight call is running against its own
//     already-taken copy, not against `sink_storage()` live.
// PT: Implementação do FW-LOG. Ver glintfx/include/glintfx/log.hpp pro contrato público completo
//     (THREADING/REENTRÂNCIA/HARDENING). Este arquivo tem ZERO dependência de RmlUi/GLFW/GL --
//     mesma classe de independência do src/log_dedup.hpp (que ESTE arquivo também não precisa: o
//     dedup já aconteceu rio-acima, em draw2d.cpp/system_clock.hpp/system_glfw_dedup.hpp, antes
//     de uma mensagem sequer alcançar log()/log_info()/log_warn()/log_error() aqui -- ver o
//     próprio parágrafo "DEDUP/THROTTLE ACONTECE ANTES DO SINK" do log.hpp).
//
//     O armazenamento do sink é um único `static LogSink` local-de-função (sink_storage()
//     abaixo), não um global em namespace scope -- evita qualquer questão de ordem de
//     inicialização estática para uma chamada que em princípio poderia vir do próprio
//     inicializador estático de outra unidade de tradução (defensivo; nada nesta biblioteca hoje
//     loga a partir de um inicializador estático, mas o static local-de-função não custa nada e
//     remove a questão inteiramente, mesmo raciocínio pelo qual um singleton de Meyers é
//     escolhido em vez de um global puro nesta classe de utilitário "um pedaço de estado
//     compartilhado processo-inteiro").
//
//     `log()` e os 3 wrappers estilo printf tomam todos uma CÓPIA LOCAL do sink correntemente
//     instalado (`LogSink sink_copy = sink_storage();`) ANTES de invocá-lo -- o contrato de
//     REENTRÂNCIA que o log.hpp documenta: um sink que chama `set_log_sink()` (mesmo que pra
//     instalar ele mesmo de novo) ou chama de volta `log()`/`log_info()`/... de dentro do próprio
//     corpo nunca observa um `std::function` pendurado/movido-de, porque a chamada em voo está
//     rodando contra a própria cópia já tirada, não contra o `sink_storage()` ao vivo.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/log.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace glintfx {

namespace {

// EN: See log.hpp's HARDENING paragraph. Fixed, safe fallback text substituted for the caller's
//     `fmt` when it is rejected -- never the caller's variadic arguments interpreted against it.
// PT: Ver o parágrafo HARDENING do log.hpp. Texto de fallback fixo e seguro, substituído pelo
//     `fmt` do chamador quando ele é rejeitado -- nunca os argumentos variádicos do chamador
//     interpretados contra ele.
constexpr char kHostileFormatMessage[] =
    "[glintfx::log: rejected a format string containing %n or a dangling "
    "'%' -- message dropped for safety]";

constexpr char kTruncatedMarker[] = "...[truncated]";

// EN: Copies `text` into `buf` (capacity `cap`, including the NUL), truncating with `...` if
//     needed -- used ONLY for the two fixed diagnostic strings above, both far shorter than
//     kLogMaxMessageBytes in practice, but hardened anyway (defensive, zero-cost).
// PT: Copia `text` para `buf` (capacidade `cap`, incluindo o NUL), truncando com `...` se
//     necessário -- usado SÓ para as duas strings de diagnóstico fixas acima, ambas bem mais
//     curtas que kLogMaxMessageBytes na prática, mas protegido mesmo assim (defensivo, custo
//     zero).
void copy_fixed(char* buf, std::size_t cap, const char* text) {
  if (cap == 0) return;
  const std::size_t len = std::strlen(text);
  const std::size_t n = (len < cap - 1) ? len : (cap - 1);
  std::memcpy(buf, text, n);
  buf[n] = '\0';
}

// EN: Scans `fmt` for the ONE conversion this module refuses to ever hand to `vsnprintf` (`%n`,
//     see log.hpp's HARDENING paragraph) and for a dangling `%` at the very end (an incomplete
//     conversion -- undefined behaviour per the C standard). `%%` (an escaped literal percent) is
//     correctly walked over without tripping either check. Every other conversion (`%s`, `%d`,
//     `%f`, flags/width/precision/length modifiers, ...) passes through untouched -- this is a
//     narrow, targeted reject, not a format-string allowlist.
// PT: Varre `fmt` procurando a ÚNICA conversão que este módulo se recusa a entregar ao
//     `vsnprintf` (`%n`, ver o parágrafo HARDENING do log.hpp) e um `%` pendurado bem no final
//     (uma conversão incompleta -- comportamento indefinido pelo padrão C). `%%` (um percent
//     literal escapado) é corretamente atravessado sem disparar nenhuma das duas checagens. Toda
//     outra conversão (`%s`, `%d`, `%f`, modificadores de flag/largura/precisão/tamanho, ...)
//     passa intocada -- é uma rejeição estreita e mirada, não uma allowlist de format string.
bool format_is_hostile(const char* fmt) {
  for (std::size_t i = 0; fmt[i] != '\0';) {
    if (fmt[i] != '%') {
      ++i;
      continue;
    }
    if (fmt[i + 1] == '%') { // literal "%%" -- not a conversion, skip both chars
      i += 2;
      continue;
    }
    std::size_t j = i + 1;
    // EN: Skip flag/width/precision/length-modifier characters (POSIX printf grammar) until the
    //     conversion specifier character itself.
    // PT: Pula caracteres de flag/largura/precisão/modificador-de-tamanho (gramática POSIX do
    //     printf) até o próprio caractere de especificador de conversão.
    while (fmt[j] != '\0' && std::strchr("-+ #0123456789.*hlLqjzt", fmt[j]) != nullptr) {
      ++j;
    }
    if (fmt[j] == '\0') return true; // dangling '%' -- incomplete conversion, reject
    if (fmt[j] == 'n') return true;  // %n -- writes to a caller pointer, reject
    i = j + 1;
  }
  return false;
}

// EN: Formats `fmt`/`ap` into `buf` (capacity `cap`), applying every HARDENING rule log.hpp
//     documents: `nullptr` fmt -> empty message; hostile fmt (%n / dangling %) ->
//     kHostileFormatMessage, `vsnprintf` never called; a `vsnprintf` encoding error (negative
//     return) -> empty message; a result that would not have fit `cap` -> truncated in place with
//     a visible "...[truncated]" marker overwriting the final bytes (never a buffer overrun --
//     `vsnprintf` itself never writes past `cap`, by its own contract, regardless of `fmt`'s
//     would-be expansion length).
// PT: Formata `fmt`/`ap` em `buf` (capacidade `cap`), aplicando toda regra de HARDENING que o
//     log.hpp documenta: fmt `nullptr` -> mensagem vazia; fmt hostil (%n / % pendurado) ->
//     kHostileFormatMessage, `vsnprintf` nunca é chamado; um erro de codificação do `vsnprintf`
//     (retorno negativo) -> mensagem vazia; um resultado que não teria cabido em `cap` ->
//     truncado no lugar com um marcador visível "...[truncated]" sobrescrevendo os bytes finais
//     (nunca um estouro de buffer -- o próprio `vsnprintf` nunca escreve além de `cap`, pelo
//     próprio contrato dele, independente de quão longa a expansão de `fmt` teria sido).
void format_into_buffer(char* buf, std::size_t cap, const char* fmt, va_list ap) {
  if (cap == 0) return;
  if (fmt == nullptr) {
    buf[0] = '\0';
    return;
  }
  if (format_is_hostile(fmt)) {
    copy_fixed(buf, cap, kHostileFormatMessage);
    return;
  }

  const int written = std::vsnprintf(buf, cap, fmt, ap);
  if (written < 0) {
    // EN: Encoding error (e.g. a multibyte conversion failure deep in the platform's own
    //     vsnprintf) -- fail-high, never propagate garbage.
    // PT: Erro de codificação (ex.: falha de conversão multibyte lá dentro do próprio vsnprintf
    //     da plataforma) -- fail-high, nunca propagar lixo.
    buf[0] = '\0';
    return;
  }
  const std::size_t would_be_len = static_cast<std::size_t>(written);
  if (would_be_len >= cap) {
    // EN: vsnprintf already NUL-terminated buf at cap-1 chars (its own contract) -- overwrite the
    //     final bytes with the truncation marker, never growing past `cap`.
    // PT: O vsnprintf já terminou buf em NUL com cap-1 chars (contrato dele) -- sobrescreve os
    //     bytes finais com o marcador de truncamento, nunca crescendo além de `cap`.
    const std::size_t buf_len = cap - 1;
    const std::size_t marker_len = sizeof(kTruncatedMarker) - 1;
    const std::size_t start = (marker_len < buf_len) ? (buf_len - marker_len) : 0;
    std::memcpy(buf + start, kTruncatedMarker, marker_len + 1); // +1 copies the NUL too
  }
}

LogSink& sink_storage() {
  static LogSink current; // empty by default -- log()/log_info()/... fall back to stderr
  return current;
}

} // namespace

void set_log_sink(LogSink sink) {
  sink_storage() = std::move(sink);
}

void log(LogLevel level, const char* message) {
  const char* msg = (message != nullptr) ? message : "";
  LogSink sink_copy = sink_storage(); // REENTRANCY contract -- see this file's header comment
  if (sink_copy) {
    sink_copy(level, msg);
  } else {
    std::fprintf(stderr, "%s\n", msg);
  }
}

void log_info(const char* fmt, ...) {
  char buf[kLogMaxMessageBytes];
  va_list ap;
  va_start(ap, fmt);
  format_into_buffer(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  log(LogLevel::Info, buf);
}

void log_warn(const char* fmt, ...) {
  char buf[kLogMaxMessageBytes];
  va_list ap;
  va_start(ap, fmt);
  format_into_buffer(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  log(LogLevel::Warn, buf);
}

void log_error(const char* fmt, ...) {
  char buf[kLogMaxMessageBytes];
  va_list ap;
  va_start(ap, fmt);
  format_into_buffer(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  log(LogLevel::Error, buf);
}

} // namespace glintfx
