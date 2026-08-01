// SPDX-License-Identifier: Apache-2.0
// EN: FW-LOG (framework-2D, W20, pedido D do consumidor GusWorld 2026-07-29) -- the ONLY public
//     surface this library offers for its own diagnostic output. Until this header existed,
//     glintfx wrote every warning straight to `stderr` with zero API: 95 `log_warn()` call sites
//     in src/draw2d.cpp alone, plus every RmlUi-internal message funneled through
//     src/system_clock.hpp (embed mode) / src/system_glfw_dedup.hpp (App mode) -- a consumer
//     embedding this library into its own binary had NO way to redirect, filter, or even see
//     that noise as anything other than raw text on the process's stderr. The actual measured
//     need (GusWorld's own `SDL_Log` call sites, audited 2026-07-29): 30 occurrences, ZERO of
//     which specify a severity/priority -- every single one is the plainest possible "one string,
//     nothing else" call. That is the honest floor this API targets; everything here is already
//     MORE than the measured need, on purpose, per the team lead's explicit license to be
//     strict when in doubt.
//
//     TWO DISTINCT ENTRY POINTS, on purpose (not an oversight, not redundant):
//       - `log(LogLevel, const char*)` -- a PLAIN message, zero formatting. This is what every
//         call site INSIDE glintfx itself uses (draw2d.cpp's `Impl::log_warn`,
//         system_clock.hpp's/system_glfw_dedup.hpp's `LogMessage` overrides), because those
//         messages routinely embed caller-supplied text verbatim -- a filesystem path
//         (`load_texture(): could not open '<path>'`), a GL shader-compile-error string, an
//         RmlUi document warning. Any of those CAN legitimately contain a literal '%' character
//         (a filename is not required to avoid it). Passing that text through a printf-style
//         formatter would be a textbook format-string bug (CWE-134: an attacker- or
//         accident-controlled '%s'/'%n' in what the program treats as ITS OWN format string).
//         `log()` never touches `<cstdio>`'s formatting machinery at all -- structurally immune,
//         not merely careful.
//       - `log_info()`/`log_warn()`/`log_error()` -- printf-style convenience, the direct
//         `SDL_Log(fmt, ...)` replacement a host reaches for. `fmt` MUST be a trusted, effectively
//         literal format string authored by the CALLING CODE -- exactly the same discipline every
//         printf-family function in C/C++ already demands (never pass arbitrary/untrusted runtime
//         text as `fmt`; if you need to log untrusted text verbatim, use `log(level, text)`
//         above, or `log_info("%s", untrusted_text)` -- putting the untrusted text in an ARGUMENT
//         position, never in `fmt` itself, is what makes that safe).
//
//     DEDUP/THROTTLE HAPPENS BEFORE THE SINK, NOT AFTER: the LogDedup policy (src/log_dedup.hpp,
//     LOGTHR-1/D8 -- 1st occurrence prints, repeats suppressed except a summary at counts
//     10/100/1000/..., is_error bypasses dedup unconditionally) still runs INSIDE draw2d.cpp and
//     the two SystemInterface overrides, exactly as before this header existed -- this API sits
//     AFTER that filter, not instead of it. A host's sink therefore receives the ALREADY-CURATED
//     stream (at most one line per distinct message per decade of repeats), never the raw
//     per-frame flood the dedup machinery exists to prevent. This is a deliberate, load-bearing
//     property: without it, a custom sink would defeat the entire point of LOGTHR-1.
//
//     SINK CONTRACT: `set_log_sink()` installs a single, global `LogSink`
//     (`std::function<void(LogLevel, const char*)>`), replacing whatever was registered before
//     (an empty/`nullptr` `std::function`, including the default-constructed `LogSink{}`, resets
//     to the built-in default -- fprintf(stderr, "%s\n", message), the EXACT byte-for-byte output
//     this library already produced before this header existed, so a consumer who never calls
//     `set_log_sink()` observes zero behaviour change). Only ONE sink is ever active at a time --
//     see docs/capabilities.md's Log row for the full, explicit ceiling this module stops at
//     (deliberately: no multi-sink fan-out, no file rotation, no structured/JSON output, no
//     asynchronous delivery, no per-module filtering).
//
//     THREADING: this module has EXACTLY the same contract as every other glintfx utility that
//     is not itself an audio/gamepad hardware owner (see audio.hpp's own THREADING note for the
//     library-wide precedent) -- SINGLE-THREADED, NO INTERNAL LOCK. `set_log_sink()` and every
//     `log*()` call must all come from ONE thread (in practice: the same thread that owns the
//     `App`/`UiLayer` instance, i.e. the thread driving the GL/RmlUi frame loop -- nothing in
//     this library spawns a thread of its own, see CLAUDE.md's project-state note; a host that
//     itself logs from multiple threads must synchronize externally before calling into this
//     API). This is a plain declared limitation, not a bug: adding a mutex here without a real,
//     measured multi-threaded caller would be exactly the kind of unmeasured complexity this
//     module's ceiling exists to refuse.
//
//     REENTRANCY: every `log*()` call invokes the CURRENTLY registered sink through a LOCAL COPY
//     taken at the start of that call (the same discipline `App`/`Gamepads` already document for
//     their own callbacks -- see e.g. gamepad.hpp's own reentrancy note) -- so a sink that itself
//     calls `set_log_sink()` (installing a replacement, including itself again) or calls back
//     into `log()`/`log_info()`/... from inside its own body is well-defined: the in-flight
//     invocation keeps running against the copy it already captured, never a dangling/moved-from
//     `std::function`, and the fresh call from inside the sink reads whatever is CURRENTLY
//     installed at that later point (which may already be the replacement). Never a crash, never
//     UB, by construction.
//
//     HARDENING (fail-high, same discipline as `decorator_polygon.cpp`/`i18n.cpp`/`Audio`'s own
//     malformed-input handling -- see this library's project-wide "input validation em borda"
//     convention): a `nullptr` `message`/`fmt` is treated as an empty string (never dereferenced,
//     never a crash); a message longer than `kMaxMessageBytes` (4096, INCLUDING the terminating
//     NUL the underlying `vsnprintf` counts against that budget) is truncated in place with a
//     visible `"...[truncated]"` marker replacing its final bytes, so a caller can tell the
//     difference between "the message really ended here" and "the message was cut" -- never a
//     buffer overrun (this module never writes past its own fixed-size internal buffer,
//     regardless of how long `fmt`'s EXPANSION would have been); `fmt` is validated against an
//     ALLOWLIST of conversion specifiers (`diouxXeEfFgGaAcsp` -- see `format_is_hostile()` in
//     log.cpp), not a blocklist of specific known-bad ones: a conversion whose trailing character
//     is NOT on that allowlist is rejected WHOLESALE before `vsnprintf` ever runs, in EVERY
//     syntactic form the POSIX/glibc printf grammar allows for it (a bare `%n`, the POSIX/glibc
//     positional-argument form `%1$n`, any length-modified form `%ln`/`%lln`/`%hn`/`%hhn`/`%qn`/
//     `%jn`/`%zn`/`%tn`/`%Ln`, and combinations thereof such as `%1$ln`) -- the sink receives a
//     fixed, safe diagnostic message instead of the caller's `fmt`, never the caller's variadic
//     arguments interpreted against a hostile format string. `%n` -- the one conversion specifier
//     whose ENTIRE purpose is to WRITE to a caller-supplied pointer, the specific,
//     historically-exploited write-primitive half of the format-string bug class CWE-134 that
//     plain input-length hardening cannot neutralise -- is the reason this allowlist exists; it is
//     an allowlist rather than a blocklist specifically because an earlier version of this guard
//     (a blocklist that skipped a fixed set of "known" flag/width/length characters before
//     checking for a literal `n`) did not include the POSIX/glibc positional-argument separator
//     `$` in that skipped set, so `%1$n` reached `vsnprintf` unrejected -- a real, proven
//     arbitrary-memory-write via this library's public API, closed by this allowlist rewrite (see
//     `CHANGELOG.md`, `FW-LOG`). A dangling `%` at the very end of `fmt` (an incomplete conversion
//     -- undefined behaviour per the C standard if handed to the platform's own `vsnprintf`) is
//     rejected the same way, for the same reason. This allowlist does NOT protect against `%s`
//     with a mismatched/invalid argument (a classic printf footgun, but not a write primitive) --
//     that risk is inherent to any printf-style API and remains the CALLING CODE's documented
//     responsibility (see the "TWO DISTINCT ENTRY POINTS" paragraph above: pass untrusted text as
//     an ARGUMENT via `%s`, never as `fmt` itself).
// PT: FW-LOG (framework-2D, W20, pedido D do consumidor GusWorld 2026-07-29) -- a ÚNICA
//     superfície pública que esta biblioteca oferece para a própria saída de diagnóstico. Até
//     este header existir, a glintfx escrevia todo aviso direto no `stderr` com ZERO API: 95
//     pontos de chamada de `log_warn()` só em src/draw2d.cpp, mais toda mensagem interna do
//     RmlUi canalizada por src/system_clock.hpp (modo embed) / src/system_glfw_dedup.hpp (modo
//     App) -- um consumidor embarcando esta biblioteca no próprio binário NÃO tinha como
//     redirecionar, filtrar, ou sequer ver esse ruído como algo além de texto cru no stderr do
//     processo. A necessidade de fato medida (os próprios call sites de `SDL_Log` do GusWorld,
//     auditados em 2026-07-29): 30 ocorrências, ZERO delas especificando severidade/prioridade --
//     toda única chamada é a forma mais crua possível ("uma string, nada mais"). Esse é o piso
//     honesto que esta API mira; tudo aqui já é MAIS do que a necessidade medida, de propósito,
//     conforme a licença explícita do líder de time pra ser estrito na dúvida.
//
//     DOIS PONTOS DE ENTRADA DISTINTOS, de propósito (não é descuido nem redundância):
//       - `log(LogLevel, const char*)` -- mensagem PLANA, zero formatação. É o que TODO ponto de
//         chamada DENTRO da própria glintfx usa (`Impl::log_warn` do draw2d.cpp, os overrides de
//         `LogMessage` de system_clock.hpp/system_glfw_dedup.hpp), porque essas mensagens
//         rotineiramente embutem texto fornecido pelo chamador verbatim -- um caminho de
//         filesystem (`load_texture(): could not open '<path>'`), uma string de erro de compile
//         de shader GL, um aviso de documento do RmlUi. Qualquer um desses PODE legitimamente
//         conter um caractere '%' literal (um nome de arquivo não é obrigado a evitá-lo). Passar
//         esse texto por um formatador estilo printf seria um bug de format-string de livro-texto
//         (CWE-134: um '%s'/'%n' controlado por atacante ou por acidente naquilo que o programa
//         trata como o PRÓPRIO format string). `log()` nunca toca a maquinaria de formatação do
//         `<cstdio>` -- imune por estrutura, não só por cuidado.
//       - `log_info()`/`log_warn()`/`log_error()` -- conveniência estilo printf, o substituto
//         direto de `SDL_Log(fmt, ...)` que um host busca. `fmt` PRECISA ser um format string
//         confiável, efetivamente literal, autorado pelo PRÓPRIO CÓDIGO CHAMADOR -- exatamente a
//         mesma disciplina que toda função da família printf em C/C++ já exige (nunca passe texto
//         de runtime arbitrário/não-confiável como `fmt`; se precisar logar texto não-confiável
//         verbatim, use `log(level, texto)` acima, ou `log_info("%s", texto_nao_confiavel)` --
//         colocar o texto não-confiável numa posição de ARGUMENTO, nunca no próprio `fmt`, é o
//         que torna isso seguro).
//
//     DEDUP/THROTTLE ACONTECE ANTES DO SINK, NÃO DEPOIS: a política LogDedup (src/log_dedup.hpp,
//     LOGTHR-1/D8 -- 1ª ocorrência imprime, repetições suprimidas exceto um sumário nas contagens
//     10/100/1000/..., is_error ignora o dedup incondicionalmente) continua rodando DENTRO do
//     draw2d.cpp e dos dois overrides de SystemInterface, exatamente como antes deste header
//     existir -- esta API fica DEPOIS desse filtro, não no lugar dele. O sink de um host portanto
//     recebe o stream JÁ CURADO (no máximo uma linha por mensagem distinta por década de
//     repetições), nunca o flood cru por-frame que a maquinaria de dedup existe para prevenir.
//     Esta é uma propriedade deliberada e estrutural: sem ela, um sink customizado anularia todo
//     o propósito do LOGTHR-1.
//
//     CONTRATO DO SINK: `set_log_sink()` instala um único `LogSink` global
//     (`std::function<void(LogLevel, const char*)>`), substituindo o que estava registrado antes
//     (um `std::function` vazio/`nullptr`, inclusive o `LogSink{}` default-construído, reseta pro
//     default embutido -- fprintf(stderr, "%s\n", message), a MESMA saída byte-a-byte que esta
//     biblioteca já produzia antes deste header existir, então um consumidor que nunca chama
//     `set_log_sink()` observa zero mudança de comportamento). Só UM sink fica ativo por vez --
//     ver a linha de Log do docs/capabilities.md pro teto completo e explícito onde este módulo
//     para (deliberadamente: sem fan-out multi-sink, sem rotação de arquivo, sem saída
//     estruturada/JSON, sem entrega assíncrona, sem filtro por-módulo).
//
//     THREADING: este módulo tem EXATAMENTE o mesmo contrato de todo outro utilitário da glintfx
//     que não é ele próprio dono de recurso de hardware de áudio/gamepad (ver a própria nota
//     THREADING do audio.hpp pro precedente da biblioteca inteira) -- SINGLE-THREADED, SEM LOCK
//     INTERNO. `set_log_sink()` e toda chamada `log*()` precisam vir de UMA thread (na prática: a
//     mesma thread dona da instância `App`/`UiLayer`, ou seja, a thread que comanda o frame loop
//     GL/RmlUi -- nada nesta biblioteca cria a própria thread, ver a nota de estado-do-projeto do
//     CLAUDE.md; um host que loga de várias threads precisa sincronizar por fora antes de chamar
//     esta API). Isto é uma limitação declarada, não um bug: adicionar um mutex aqui sem um
//     chamador multi-thread real e medido seria exatamente o tipo de complexidade não-medida que
//     o teto deste módulo existe para recusar.
//
//     REENTRÂNCIA: toda chamada `log*()` invoca o sink CORRENTEMENTE registrado através de uma
//     CÓPIA LOCAL tirada no início daquela chamada (mesma disciplina que `App`/`Gamepads` já
//     documentam para os próprios callbacks -- ver ex. a própria nota de reentrância do
//     gamepad.hpp) -- então um sink que ele próprio chama `set_log_sink()` (instalando um
//     substituto, inclusive ele mesmo de novo) ou chama de volta `log()`/`log_info()`/... de
//     dentro do próprio corpo é bem definido: a invocação em voo continua rodando contra a cópia
//     que já capturou, nunca um `std::function` pendurado/movido-de, e a chamada nova de dentro
//     do sink lê o que estiver CORRENTEMENTE instalado naquele ponto mais tarde (que já pode ser
//     o substituto). Nunca um crash, nunca UB, por construção.
//
//     HARDENING (fail-high, mesma disciplina do tratamento de input malformado do
//     `decorator_polygon.cpp`/`i18n.cpp`/`Audio` -- ver a convenção "input validation em borda" a
//     nível de biblioteca deste projeto): uma `message`/`fmt` `nullptr` é tratada como string
//     vazia (nunca desreferenciada, nunca um crash); uma mensagem mais longa que
//     `kMaxMessageBytes` (4096, INCLUINDO o NUL terminador que o `vsnprintf` de baixo conta contra
//     esse orçamento) é truncada no lugar com um marcador visível `"...[truncated]"` substituindo
//     os bytes finais, para que o chamador consiga distinguir "a mensagem realmente terminou
//     aqui" de "a mensagem foi cortada" -- nunca um estouro de buffer (este módulo nunca escreve
//     além do próprio buffer interno de tamanho fixo, independente de quão longa a EXPANSÃO de
//     `fmt` teria sido); `fmt` é validado contra uma ALLOWLIST de especificadores de conversão
//     (`diouxXeEfFgGaAcsp` -- ver `format_is_hostile()` em log.cpp), não uma blocklist de casos
//     específicos conhecidamente ruins: uma conversão cujo caractere final NÃO está nessa
//     allowlist é rejeitada POR INTEIRO antes do `vsnprintf` sequer rodar, em TODA forma sintática
//     que a gramática POSIX/glibc de printf permite pra ela (um `%n` puro, a forma posicional
//     POSIX/glibc `%1$n`, qualquer forma com modificador de comprimento `%ln`/`%lln`/`%hn`/
//     `%hhn`/`%qn`/`%jn`/`%zn`/`%tn`/`%Ln`, e combinações como `%1$ln`) -- o sink recebe uma
//     mensagem de diagnóstico fixa e segura no lugar do `fmt` do chamador, nunca os argumentos
//     variádicos do chamador interpretados contra um format string hostil. `%n` -- o único
//     especificador de conversão cujo PROPÓSITO INTEIRO é ESCREVER num ponteiro fornecido pelo
//     chamador, a metade de primitiva-de-escrita específica e historicamente explorada da classe
//     de bug format-string CWE-134 que hardening de tamanho de input sozinho não neutraliza -- é
//     o motivo desta allowlist existir; ela é uma allowlist em vez de uma blocklist
//     especificamente porque uma versão anterior desta guarda (uma blocklist que pulava um
//     conjunto fixo de caracteres "conhecidos" de flag/largura/comprimento antes de checar por um
//     `n` literal) não incluía o separador de argumento posicional POSIX/glibc `$` nesse conjunto
//     pulado, então `%1$n` chegava ao `vsnprintf` sem rejeição -- uma escrita arbitrária de
//     memória real e comprovada via a API pública desta biblioteca, fechada por esta reescrita em
//     allowlist (ver `CHANGELOG.md`, `FW-LOG`). Um `%` pendurado bem no final de `fmt` (uma
//     conversão incompleta -- comportamento indefinido pelo padrão C se entregue ao `vsnprintf`
//     da própria plataforma) é rejeitado do mesmo jeito, pelo mesmo motivo. Esta allowlist NÃO
//     protege contra `%s` com argumento incompatível/inválido (um footgun clássico de printf, mas
//     não uma primitiva de escrita) -- esse risco é inerente a qualquer API estilo printf e
//     continua sendo responsabilidade documentada do CÓDIGO CHAMADOR (ver o parágrafo "DOIS
//     PONTOS DE ENTRADA DISTINTOS" acima: passe texto não-confiável como um ARGUMENTO via `%s`,
//     nunca como o próprio `fmt`).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <functional>

// EN: Portable printf-format compile-time check: GCC/Clang understand
//     `__attribute__((format(printf, ...)))` (catches a mismatched fmt/argument at COMPILE time,
//     e.g. `log_warn("%s", 42)`); MSVC has no equivalent attribute spelling in this codebase's
//     toolchain (see CLAUDE.md's Windows/MSVC guard precedent in glintfx/CMakeLists.txt -- the
//     `-Wall`/`-Wextra` MSVC guard right there is the SAME class of portability fix), so the
//     macro expands to nothing there -- a silent no-op, never a build break, exactly like every
//     other GCC/Clang-only warning flag already guarded in this codebase.
// PT: Checagem portável de format-string estilo printf em tempo de compilação: GCC/Clang
//     entendem `__attribute__((format(printf, ...)))` (pega um fmt/argumento incompatível em
//     tempo de COMPILAÇÃO, ex.: `log_warn("%s", 42)`); o MSVC não tem grafia de atributo
//     equivalente na toolchain deste repositório (ver o precedente de guarda Windows/MSVC do
//     glintfx/CMakeLists.txt -- a guarda `-Wall`/`-Wextra` do MSVC logo ali é a MESMA classe de
//     fix de portabilidade), então a macro expande pra nada lá -- um no-op silencioso, nunca uma
//     quebra de build, exatamente como toda outra flag de warning exclusiva de GCC/Clang já
//     guardada neste código-base.
#if defined(__GNUC__) || defined(__clang__)
#define GLINTFX_PRINTF_LIKE(fmt_index, first_arg_index) \
  __attribute__((format(printf, fmt_index, first_arg_index)))
#else
#define GLINTFX_PRINTF_LIKE(fmt_index, first_arg_index)
#endif

namespace glintfx {

// EN: Deliberately 3 values, matching this module's ceiling exactly (see this file's header
//     comment) -- not RmlUi's own 6-value Rml::Log::Type, not SDL's 6-priority scheme. `Info` is
//     never emitted by glintfx's OWN internal call sites today (draw2d.cpp/system_clock.hpp/
//     system_glfw_dedup.hpp only ever emit `Warn` or `Error`) -- it exists for a HOST's own
//     messages (the `SDL_Log` replacement use case this module was built for).
// PT: Deliberadamente 3 valores, batendo exatamente com o teto deste módulo (ver o comentário de
//     cabeçalho deste arquivo) -- não os 6 valores do próprio Rml::Log::Type do RmlUi, não o
//     esquema de 6 prioridades do SDL. `Info` nunca é emitido pelos PRÓPRIOS pontos de chamada
//     internos da glintfx hoje (draw2d.cpp/system_clock.hpp/system_glfw_dedup.hpp só emitem
//     `Warn` ou `Error`) -- existe para as próprias mensagens de um HOST (o caso de uso
//     substituto de `SDL_Log` para o qual este módulo foi construído).
enum class LogLevel {
  Info,
  Warn,
  Error,
};

// EN: A message longer than this (INCLUDING the terminating NUL) is truncated with a visible
//     "...[truncated]" marker -- see this file's header comment's HARDENING paragraph. Generous
//     enough for any real single-line diagnostic; small enough that a hostile/degenerate `fmt`
//     expansion costs a bounded, fixed amount of stack (this module never heap-allocates its
//     formatting buffer).
// PT: Uma mensagem mais longa que isto (INCLUINDO o NUL terminador) é truncada com um marcador
//     visível "...[truncated]" -- ver o parágrafo HARDENING do comentário de cabeçalho deste
//     arquivo. Generoso o bastante pra qualquer diagnóstico real de uma linha; pequeno o
//     bastante pra que uma expansão de `fmt` hostil/degenerada custe uma quantidade limitada e
//     fixa de stack (este módulo nunca aloca no heap o próprio buffer de formatação).
inline constexpr std::size_t kLogMaxMessageBytes = 4096;

// EN: Sink signature: `level` is the severity, `message` is the final, already-formatted,
//     NUL-terminated C string (never longer than `kLogMaxMessageBytes - 1` -- see the truncation
//     contract above). `message` is only valid for the DURATION of the sink call -- copy it (e.g.
//     into a `std::string`) if you need it afterwards, same lifetime contract as every other
//     `const char*` callback parameter in this library (e.g. `App::set_click_callback`'s
//     `element_id`, see click_info.hpp).
// PT: Assinatura do sink: `level` é a severidade, `message` é a string C final, já formatada,
//     terminada em NUL (nunca mais longa que `kLogMaxMessageBytes - 1` -- ver o contrato de
//     truncamento acima). `message` só é válida pela DURAÇÃO da chamada do sink -- copie-a (ex.:
//     pra uma `std::string`) se precisar dela depois, mesmo contrato de tempo de vida de todo
//     outro parâmetro `const char*` de callback desta biblioteca (ex.: o `element_id` do
//     `App::set_click_callback`, ver click_info.hpp).
using LogSink = std::function<void(LogLevel level, const char* message)>;

// EN: Installs `sink` as the ONE active destination for every subsequent `log()`/`log_info()`/
//     `log_warn()`/`log_error()` call (from ANY module inside this library, and from your own
//     code if you call these functions directly too) -- replacing whatever was installed before.
//     An empty/`nullptr` `sink` (including a default-constructed `LogSink{}`) resets to the
//     built-in default: `fprintf(stderr, "%s\n", message)`, unconditionally, regardless of
//     `level` -- the exact behaviour this library already had before this header existed. See
//     this file's header comment for the full THREADING/REENTRANCY contract.
// PT: Instala `sink` como o ÚNICO destino ativo pra toda chamada `log()`/`log_info()`/
//     `log_warn()`/`log_error()` subsequente (de QUALQUER módulo dentro desta biblioteca, e do
//     seu próprio código também se você chamar estas funções direto) -- substituindo o que
//     estava instalado antes. Um `sink` vazio/`nullptr` (inclusive um `LogSink{}`
//     default-construído) reseta pro default embutido: `fprintf(stderr, "%s\n", message)`,
//     incondicionalmente, independente de `level` -- exatamente o comportamento que esta
//     biblioteca já tinha antes deste header existir. Ver o comentário de cabeçalho deste
//     arquivo pro contrato completo de THREADING/REENTRÂNCIA.
void set_log_sink(LogSink sink);

// EN: Plain message, zero formatting -- see this file's header comment's "TWO DISTINCT ENTRY
//     POINTS" paragraph for why this exists alongside the printf-style functions below (safe for
//     arbitrary/untrusted text, including text containing a literal '%'). A `nullptr` `message`
//     is treated as an empty string.
// PT: Mensagem plana, zero formatação -- ver o parágrafo "DOIS PONTOS DE ENTRADA DISTINTOS" do
//     comentário de cabeçalho deste arquivo pro motivo de isto existir ao lado das funções estilo
//     printf abaixo (seguro para texto arbitrário/não-confiável, inclusive texto contendo um '%'
//     literal). Uma `message` `nullptr` é tratada como string vazia.
void log(LogLevel level, const char* message);

// EN: printf-style convenience -- the direct `SDL_Log(fmt, ...)` replacement (see this file's
//     header comment). `fmt` MUST be a trusted/literal format string authored by the calling
//     code -- see the "TWO DISTINCT ENTRY POINTS" paragraph above for why, and use `log()` above
//     instead when the text itself is not something you control. `nullptr`/malformed `fmt`
//     (a `%n` conversion, or a dangling `%` at the very end) is rejected -- see this file's
//     header comment's HARDENING paragraph; the sink still receives a fixed, safe diagnostic
//     message in that case, never a crash, never an interpretation of your arguments against a
//     hostile format string.
// PT: Conveniência estilo printf -- o substituto direto de `SDL_Log(fmt, ...)` (ver o comentário
//     de cabeçalho deste arquivo). `fmt` PRECISA ser um format string confiável/literal autorado
//     pelo código chamador -- ver o parágrafo "DOIS PONTOS DE ENTRADA DISTINTOS" acima pro
//     motivo, e use o `log()` acima em vez disto quando o próprio texto não é algo que você
//     controla. Um `fmt` `nullptr`/malformado (uma conversão `%n`, ou um `%` pendurado bem no
//     final) é rejeitado -- ver o parágrafo HARDENING do comentário de cabeçalho deste arquivo; o
//     sink ainda recebe uma mensagem de diagnóstico fixa e segura nesse caso, nunca um crash,
//     nunca uma interpretação dos seus argumentos contra um format string hostil.
void log_info(const char* fmt, ...) GLINTFX_PRINTF_LIKE(1, 2);
void log_warn(const char* fmt, ...) GLINTFX_PRINTF_LIKE(1, 2);
void log_error(const char* fmt, ...) GLINTFX_PRINTF_LIKE(1, 2);

} // namespace glintfx
