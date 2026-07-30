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
    "[glintfx::log: rejected a format string containing a disallowed "
    "conversion specifier (e.g. %n) or a dangling '%' -- message dropped "
    "for safety]";

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

// EN: Conversion specifier characters this module considers safe to hand to `vsnprintf` --
//     everything a caller needs for a diagnostic message (`%s`/`%d`/`%f`/... and their length- and
//     width-modified forms), and NOTHING that can turn a format string into a write primitive or
//     an unvetted extension. Deliberately excludes:
//       - `n` -- the ONE conversion whose entire purpose is to WRITE to a caller-supplied pointer
//         (CWE-134's write half). This is the specifier this guard exists to stop, in ALL of its
//         forms (see format_is_hostile's own comment below for the enumerated list).
//       - `m` -- glibc's argument-less "print strerror(errno)" extension. Not a write primitive,
//         but not part of this module's measured need either (GusWorld's 30 `SDL_Log` call sites,
//         audited 2026-07-29, use none of glibc's non-portable extensions) -- an allowlist stays
//         an allowlist only if it excludes what it has not deliberately vetted, not just what is
//         known-dangerous.
//     `%s` and `%p` ARE on the allowlist despite `%s` being unsafe if the caller mismatches the
//     argument type (a classic printf footgun) -- that risk is inherent to ANY printf-style API
//     and is the caller's documented responsibility (see log.hpp's "TWO DISTINCT ENTRY POINTS"
//     paragraph: `log()` exists precisely for text the caller does not fully trust), not something
//     this guard can or should neutralise; without `%s` the API would not be a usable `SDL_Log`
//     replacement at all.
// PT: Caracteres de especificador de conversão que este módulo considera seguros para entregar ao
//     `vsnprintf` -- tudo que um chamador precisa para uma mensagem de diagnóstico (`%s`/`%d`/
//     `%f`/... e as formas modificadas por comprimento/largura), e NADA que possa transformar um
//     format string numa primitiva de escrita ou numa extensão não avaliada. Exclui
//     deliberadamente:
//       - `n` -- a ÚNICA conversão cujo propósito inteiro é ESCREVER num ponteiro fornecido pelo
//         chamador (a metade de escrita do CWE-134). É o especificador que esta guarda existe
//         para barrar, em TODAS as suas formas (ver o comentário do próprio format_is_hostile
//         abaixo pra lista enumerada).
//       - `m` -- extensão glibc sem argumento "imprime strerror(errno)". Não é primitiva de
//         escrita, mas também não faz parte da necessidade medida deste módulo (os 30 pontos de
//         chamada `SDL_Log` do GusWorld, auditados em 2026-07-29, não usam nenhuma extensão
//         não-portável do glibc) -- uma allowlist só continua allowlist se exclui o que não foi
//         deliberadamente avaliado, não só o que é conhecidamente perigoso.
//     `%s` e `%p` ESTÃO na allowlist apesar de `%s` ser inseguro se o chamador erra o tipo do
//     argumento (um footgun clássico de printf) -- esse risco é inerente a QUALQUER API estilo
//     printf e é responsabilidade documentada do chamador (ver o parágrafo "DOIS PONTOS DE ENTRADA
//     DISTINTOS" do log.hpp: `log()` existe exatamente para texto em que o chamador não confia
//     plenamente), não algo que esta guarda possa ou deva neutralizar; sem `%s` a API nem seria um
//     substituto utilizável de `SDL_Log`.
constexpr const char* kSafeConversions = "diouxXeEfFgGaAcsp";

// EN: Scans `fmt` for a conversion specifier whose final character is NOT on `kSafeConversions`
//     (see log.hpp's HARDENING paragraph) and for a dangling `%` at the very end (an incomplete
//     conversion -- undefined behaviour per the C standard). `%%` (an escaped literal percent) is
//     correctly walked over without tripping either check.
//
//     This is an ALLOWLIST, not a blocklist -- the fix for the exact bug class that shipped in
//     76519bb: the previous version walked past a fixed set of "known flag/width/length"
//     characters and only rejected the conversion char if it was literally `n`, so `%1$n` (the
//     POSIX/glibc positional-argument form, `$` not in that fixed set) reached `vsnprintf`
//     unrejected -- a proven arbitrary-memory-write via this library's public API. This scanner
//     instead walks the FULL POSIX/glibc printf grammar step by step (optional positional prefix
//     `m$`, flags INCLUDING `'`/`I`, width INCLUDING `*`/`*m$`, precision INCLUDING `.*`/`.*m$`,
//     length modifier) and only accepts the trailing conversion character if it is in the
//     allowlist -- so ANY unrecognised or not-deliberately-vetted conversion is rejected by
//     construction, not just the ones this comment happens to enumerate. Every `%n` variant this
//     module has identified and tests against (log_sanity.cpp) is caught by the SAME single
//     allowlist check, not by pattern-matching each one individually:
//       `%n`, `%1$n` (positional), `%ln`/`%lln`/`%hn`/`%hhn`/`%qn`/`%jn`/`%zn`/`%tn`/`%Ln`
//       (length-modified), `%1$ln` (positional + length-modified), `%*n`/`%1$*2$n` (width does
//       not change the trailing conversion character).
// PT: Varre `fmt` procurando um especificador de conversão cujo caractere final NÃO esteja em
//     `kSafeConversions` (ver o parágrafo HARDENING do log.hpp) e um `%` pendurado bem no final
//     (uma conversão incompleta -- comportamento indefinido pelo padrão C). `%%` (um percent
//     literal escapado) é corretamente atravessado sem disparar nenhuma das duas checagens.
//
//     Isto é uma ALLOWLIST, não uma blocklist -- o conserto pra exata classe de bug que foi pro
//     ar em 76519bb: a versão anterior pulava um conjunto fixo de caracteres "flag/largura/
//     comprimento conhecidos" e só rejeitava o caractere de conversão se fosse literalmente `n`,
//     então `%1$n` (a forma de argumento posicional POSIX/glibc, `$` fora desse conjunto fixo)
//     chegava ao `vsnprintf` sem rejeição -- uma escrita arbitrária de memória comprovada via a
//     API pública desta biblioteca. Este scanner em vez disso percorre a gramática COMPLETA
//     POSIX/glibc de printf passo a passo (prefixo posicional opcional `m$`, flags INCLUINDO
//     `'`/`I`, largura INCLUINDO `*`/`*m$`, precisão INCLUINDO `.*`/`.*m$`, modificador de
//     comprimento) e só aceita o caractere de conversão final se ele estiver na allowlist -- então
//     QUALQUER conversão não-reconhecida ou não-deliberadamente-avaliada é rejeitada por
//     construção, não só as que este comentário eventualmente enumera. Toda variante de `%n` que
//     este módulo identificou e testa (log_sanity.cpp) é pega pela MESMA checagem única de
//     allowlist, não por casamento de padrão individual de cada uma:
//       `%n`, `%1$n` (posicional), `%ln`/`%lln`/`%hn`/`%hhn`/`%qn`/`%jn`/`%zn`/`%tn`/`%Ln`
//       (com modificador de comprimento), `%1$ln` (posicional + comprimento), `%*n`/`%1$*2$n`
//       (largura não muda o caractere de conversão final).
bool format_is_hostile(const char* fmt) {
  // EN: Consumes an optional POSIX/glibc positional prefix `<digits>$` starting at `fmt[*pos]`,
  //     used for BOTH the leading "%m$..." argument-index prefix and the "*m$" positional-width/
  //     -precision form. Only commits (advances `*pos`) if a `$` is actually found after the
  //     digit run -- a bare digit run with no trailing `$` is ordinary width/precision digits and
  //     must be left for the caller to consume instead.
  // PT: Consome um prefixo posicional opcional POSIX/glibc `<dígitos>$` começando em
  //     `fmt[*pos]`, usado TANTO pro prefixo líder "%m$..." de índice de argumento QUANTO pra
  //     forma "*m$" de largura/precisão posicional. Só efetiva (avança `*pos`) se um `$` for
  //     realmente encontrado depois da sequência de dígitos -- uma sequência de dígitos sem `$`
  //     no final são dígitos comuns de largura/precisão e precisam ficar pro chamador consumir.
  auto skip_positional_dollar = [&fmt](std::size_t* pos) {
    std::size_t k = *pos;
    while (fmt[k] != '\0' && fmt[k] >= '0' && fmt[k] <= '9') ++k;
    if (k > *pos && fmt[k] == '$') *pos = k + 1;
  };

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

    // EN: Optional leading positional argument index, e.g. the "1$" in "%1$d"/"%1$n".
    // PT: Índice de argumento posicional líder opcional, ex.: o "1$" em "%1$d"/"%1$n".
    skip_positional_dollar(&j);

    // EN: Flags -- POSIX set plus glibc's `'` (thousands grouping) and `I` (locale digits).
    // PT: Flags -- conjunto POSIX mais o `'` (agrupamento de milhar) e `I` (dígitos de locale)
    //     do glibc.
    while (fmt[j] != '\0' && std::strchr("-+ #0'I", fmt[j]) != nullptr) {
      ++j;
    }

    // EN: Width -- either a plain digit run, or `*` optionally followed by a positional index
    //     (glibc's "*m$" form, e.g. the "*2$" in "%1$*2$n").
    // PT: Largura -- ou uma sequência de dígitos simples, ou `*` opcionalmente seguido de um
    //     índice posicional (a forma "*m$" do glibc, ex.: o "*2$" em "%1$*2$n").
    if (fmt[j] == '*') {
      ++j;
      skip_positional_dollar(&j);
    } else {
      while (fmt[j] != '\0' && fmt[j] >= '0' && fmt[j] <= '9') ++j;
    }

    // EN: Precision -- `.` then either a (possibly empty) digit run, or `*` with the same
    //     optional positional index as width.
    // PT: Precisão -- `.` seguido ou de uma sequência de dígitos (possivelmente vazia), ou `*`
    //     com o mesmo índice posicional opcional da largura.
    if (fmt[j] == '.') {
      ++j;
      if (fmt[j] == '*') {
        ++j;
        skip_positional_dollar(&j);
      } else {
        while (fmt[j] != '\0' && fmt[j] >= '0' && fmt[j] <= '9') ++j;
      }
    }

    // EN: Length modifier -- longest match first ("hh"/"ll" before "h"/"l") so e.g. "%hhn" is not
    //     misparsed as length "h" + conversion "h" (which would still be safely rejected below,
    //     but for the wrong reason -- this keeps legitimate "%hhd"/"%lld"/... correctly accepted).
    // PT: Modificador de comprimento -- casamento mais longo primeiro ("hh"/"ll" antes de "h"/
    //     "l") pra que ex. "%hhn" não seja mal-interpretado como comprimento "h" + conversão "h"
    //     (que ainda seria rejeitado com segurança abaixo, mas pelo motivo errado -- isto mantém
    //     "%hhd"/"%lld"/... legítimos corretamente aceitos).
    static const char* const kLengthModifiers[] = {"hh", "ll", "h", "l", "L", "q", "j", "z", "t"};
    for (const char* mod : kLengthModifiers) {
      const std::size_t len = std::strlen(mod);
      if (std::strncmp(fmt + j, mod, len) == 0) {
        j += len;
        break;
      }
    }

    if (fmt[j] == '\0') return true; // dangling '%' -- incomplete conversion, reject
    if (std::strchr(kSafeConversions, fmt[j]) == nullptr) {
      return true; // not on the allowlist -- covers 'n' (every form above), 'm', and anything
                   // else this module has not deliberately vetted. Fail-high by construction.
    }
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
