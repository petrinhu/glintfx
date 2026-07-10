// SPDX-License-Identifier: MPL-2.0
// EN: E2 -- see include/printf.h for the public contract and the design rationale
//     (`<stdarg.h>` boundary, testability split, buffer/flush design). This file holds the
//     shared formatting CORE (`mini_format_core`, static/internal) plus two thin "sink"
//     adapters that each `mini_vsnprintf`/`mini_printf` plug into it -- the conversion logic
//     (`%d %u %x %s %c %f %%` + unknown-directive fallback) is written EXACTLY ONCE and reused by
//     both public entry points, so there is no risk of the two functions' formatting drifting
//     apart. (`%f`, SOV-FCONV, appended later -- see its own `case 'f'` block below for the
//     rationale; `%g` is NOT implemented, see that block and TODO.md.)
//
//     THE SINK ABSTRACTION: `mini_sink_fn` is a function pointer `(ctx, data, n) -> void` that
//     receives one RUN of output bytes at a time (a literal-text run between `%` directives, or
//     one conversion's full output) -- never a single character at a time, which is what keeps
//     `mini_printf`'s stdout sink from making one `sys_write` per byte. `mini_format_core` never
//     touches `buf`/`sys_write` directly; it only calls `sink(ctx, ...)` and separately
//     accumulates the C99-`vsnprintf`-style "logical length" (the length that WOULD have been
//     produced, independent of whatever the sink actually kept) -- this is why `mini_vsnprintf`
//     can report a length beyond `cap` on truncation, exactly like the real `vsnprintf`.
//
//     GOTCHA -- default argument promotions (C's variadic-call rule): `char`/`short` arguments
//     are promoted to `int`, `float` to `double`, when passed through `...`. So `%c` MUST read
//     `va_arg(ap, int)` (never `va_arg(ap, char)` -- that would be UNDEFINED BEHAVIOUR, reading
//     the wrong-sized slot from the variadic argument area) and narrow to a `char` only
//     afterwards; `%d` reads `va_arg(ap, int)` (never `short`); `%u`/`%x` read
//     `va_arg(ap, unsigned)`. `%s` is unaffected (pointers are not subject to this promotion).
//
//     `mini_vsnprintf`'s buffer sink (`mini_buf_sink`/`mini_buf_ctx_t`): copies at most
//     `cap - 1` bytes total into `buf` via D1's own `memcpy` (never libc's), silently dropping
//     anything beyond that (truncation, not an error) while `mini_format_core` keeps counting
//     the FULL logical length regardless. `cap == 0` is handled by never touching `buf` at all
//     (checked once in `mini_vsnprintf` itself before the loop even starts, so `buf` may be
//     `NULL` in that case) -- and by the sink's own `room == 0` short-circuit, which also
//     protects a `cap == 1` call from ever writing past the single NUL-terminator slot.
//
//     `mini_printf`'s flush sink (`mini_flush_sink`/`mini_flush_ctx_t`): a 256-byte STATIC-SIZE
//     (stack-local, not `static` storage -- freestanding/no-thread-safety-claimed, but also no
//     hidden global state) scratch buffer. The sink's `while (n > 0)` loop handles a single
//     incoming run LARGER than the whole 256-byte buffer (e.g. a long `%s` argument) by copying
//     as much as fits, flushing, and repeating -- this is the one piece of genuine looping logic
//     in this file, and it is what makes `mini_printf` never truncate regardless of output
//     length (unlike `mini_vsnprintf`, which is deliberately buffer-bounded like real
//     `vsnprintf`). `mini_flush` (the actual `sys_write` call site) adds `sys_write`'s return
//     value to a running total ONLY when positive -- short writes/errors are not retried (same
//     "caller decides" contract `sys_write` itself documents, ADR-0002); this project's `E2`
//     scope does not include a write-retry loop (YAGNI, no real caller has hit a short write on
//     a regular file/terminal yet).
// PT: E2 -- ver include/printf.h pro contrato publico e a justificativa de design (fronteira do
//     `<stdarg.h>`, separacao de testabilidade, design de buffer/flush). Este arquivo guarda o
//     NUCLEO de formatacao compartilhado (`mini_format_core`, estatico/interno) mais dois
//     adaptadores "sink" finos nos quais cada `mini_vsnprintf`/`mini_printf` se pluga -- a
//     logica de conversao (`%d %u %x %s %c %%` + fallback de diretiva desconhecida) e' escrita
//     EXATAMENTE UMA VEZ e reusada pelos dois pontos de entrada publicos, entao nao ha risco das
//     duas funcoes divergirem na formatacao.
//
//     A ABSTRACAO DE SINK: `mini_sink_fn` e' um ponteiro de funcao `(ctx, data, n) -> void` que
//     recebe uma LEVA de bytes de saida por vez (um trecho de texto literal entre diretivas
//     `%`, ou a saida completa de uma conversao) -- nunca um caractere por vez, o que e' o que
//     impede o sink de stdout do `mini_printf` de fazer um `sys_write` por byte.
//     `mini_format_core` nunca toca `buf`/`sys_write` diretamente; so chama `sink(ctx, ...)` e
//     separadamente acumula o "tamanho logico" estilo `vsnprintf` do C99 (o tamanho que TERIA
//     sido produzido, independente do que o sink de fato guardou) -- e' por isso que o
//     `mini_vsnprintf` consegue reportar um tamanho alem de `cap` em caso de truncamento,
//     exatamente como o `vsnprintf` de verdade.
//
//     GOTCHA -- promocoes de argumento padrao (regra de chamada variadica de C):
//     argumentos `char`/`short` sao promovidos a `int`, `float` a `double`, ao passar por
//     `...`. Entao `%c` DEVE ler `va_arg(ap, int)` (nunca `va_arg(ap, char)` -- seria
//     COMPORTAMENTO INDEFINIDO, lendo o slot de tamanho errado da area de argumento variadico)
//     e estreitar pra `char` so' depois; `%d` le `va_arg(ap, int)` (nunca `short`); `%u`/`%x`
//     leem `va_arg(ap, unsigned)`. `%s` nao e' afetado (ponteiros nao sofrem essa promocao).
//
//     O sink de buffer do `mini_vsnprintf` (`mini_buf_sink`/`mini_buf_ctx_t`): copia no maximo
//     `cap - 1` bytes no total pra `buf` via o `memcpy` PROPRIO do D1 (nunca o da libc),
//     descartando silenciosamente qualquer coisa alem disso (truncamento, nao um erro) enquanto
//     `mini_format_core` continua contando o tamanho logico COMPLETO de qualquer jeito.
//     `cap == 0` e' tratado nunca tocando `buf` de jeito nenhum (checado uma vez no proprio
//     `mini_vsnprintf` antes mesmo do laco comecar, entao `buf` pode ser `NULL` nesse caso) -- e
//     pelo proprio curto-circuito `room == 0` do sink, que tambem protege uma chamada `cap == 1`
//     de jamais escrever alem do unico slot do NUL terminador.
//
//     O sink de flush do `mini_printf` (`mini_flush_sink`/`mini_flush_ctx_t`): um buffer de
//     rascunho de TAMANHO FIXO de 256 bytes (local de pilha, nao armazenamento `static` --
//     freestanding/sem reivindicar thread-safety, mas tambem sem estado global escondido). O
//     laco `while (n > 0)` do sink trata uma unica leva de entrada MAIOR que o buffer de 256
//     bytes inteiro (ex.: um argumento `%s` longo) copiando o quanto couber, dando flush, e
//     repetindo -- essa e' a unica peca de logica de laco de verdade neste arquivo, e e' o que
//     faz o `mini_printf` nunca truncar independente do tamanho da saida (diferente do
//     `mini_vsnprintf`, que e' deliberadamente limitado por buffer como o `vsnprintf` de
//     verdade). O `mini_flush` (o ponto real de chamada do `sys_write`) so soma o retorno do
//     `sys_write` a um total corrente QUANDO positivo -- short writes/erros nao sao
//     reexecutados (mesmo contrato "quem chama decide" que o proprio `sys_write` documenta,
//     ADR-0002); o escopo deste `E2` nao inclui um laco de retry de escrita (YAGNI, nenhum
//     chamador real bateu num short write em arquivo regular/terminal ainda).
// Copyright (c) 2026 Petrus Silva Costa
#include "printf.h"
#include "conv.h"
#include "str.h"
#include "mem.h"
#include "sys_write.h"
#include "types.h"

// EN: One run of output bytes handed to whichever sink is active. `n == 0` calls never happen
//     from mini_format_core (every call site below only invokes sink with n >= 1), but sinks
//     tolerate it harmlessly regardless (defensive, not load-bearing).
// PT: Uma leva de bytes de saida repassada a qualquer sink que esteja ativo. Chamadas com
//     `n == 0` nunca acontecem a partir do mini_format_core (todo ponto de chamada abaixo so
//     invoca o sink com n >= 1), mas os sinks toleram isso sem dano de qualquer jeito
//     (defensivo, nao estrutural).
typedef void (*mini_sink_fn)(void* ctx, const char* data, size_t n);

// EN: Shared formatting core -- see file header for the sink abstraction and the promotion
//     gotcha. Returns (via `*out_len`) the FULL logical length that would be produced,
//     regardless of how much the sink actually kept.
// PT: Nucleo de formatacao compartilhado -- ver cabecalho do arquivo pra abstracao de sink e o
//     gotcha de promocao. Retorna (via `*out_len`) o tamanho logico COMPLETO que seria
//     produzido, independente de quanto o sink de fato guardou.
static void mini_format_core(mini_sink_fn sink, void* ctx, size_t* out_len, const char* fmt,
                              va_list ap) {
    size_t len = 0;

    while (*fmt != '\0') {
        if (*fmt != '%') {
            // EN: Literal run: batch every contiguous non-'%' byte into ONE sink call (avoids
            //     one call per character).
            // PT: Leva literal: agrupa todo byte contiguo nao-'%' numa UNICA chamada de sink
            //     (evita uma chamada por caractere).
            const char* start = fmt;
            while (*fmt != '\0' && *fmt != '%') {
                fmt++;
            }
            size_t n = (size_t)(fmt - start);
            sink(ctx, start, n);
            len += n;
            continue;
        }

        fmt++; // skip '%'
        switch (*fmt) {
            case 'd': {
                int v = va_arg(ap, int); // no promotion gotcha here: %d already wants int
                char digits[CONV_ITOA_BUF_MIN];
                itoa(v, digits, 10);
                size_t n = strlen(digits);
                sink(ctx, digits, n);
                len += n;
                fmt++;
                break;
            }
            case 'u': {
                unsigned v = va_arg(ap, unsigned);
                char digits[CONV_UTOA_BUF_MIN];
                utoa(v, digits, 10);
                size_t n = strlen(digits);
                sink(ctx, digits, n);
                len += n;
                fmt++;
                break;
            }
            case 'x': {
                unsigned v = va_arg(ap, unsigned);
                char digits[CONV_UTOA_BUF_MIN];
                utoa(v, digits, 16);
                size_t n = strlen(digits);
                sink(ctx, digits, n);
                len += n;
                fmt++;
                break;
            }
            case 'f': {
                // EN: SOV-FCONV addition. `double` is unaffected by default argument promotion
                //     the way `float` WOULD be (a `float` argument passed through `...` is
                //     already promoted to `double` before this function ever sees it -- the
                //     promotion happens at the CALL SITE, per C's variadic rules) -- so
                //     `va_arg(ap, double)` is correct for both a literal `double` argument and a
                //     `float` one. Fixed default precision only (CONV_FTOA_DEFAULT_PRECISION,
                //     6) -- no width/precision/flags modifiers, same YAGNI stance as every other
                //     conversion in this function (see file header). `%g` is NOT implemented:
                //     it would require scientific-notation (`%e`-style) formatting to pick
                //     between fixed/exponential by magnitude, which `ftoa` does not provide (it
                //     is fixed-point ONLY) -- registered as a follow-up in TODO.md rather than
                //     built here, per this task's explicit scope ("so' se sair de graca").
                // PT: Adicao do SOV-FCONV. `double` nao e' afetado pela promocao de argumento
                //     padrao do jeito que um `float` SERIA (um argumento `float` passado por
                //     `...` ja e' promovido a `double` antes desta funcao sequer ve-lo -- a
                //     promocao acontece no PONTO DE CHAMADA, pelas regras variadicas de C) --
                //     entao `va_arg(ap, double)` e' correto tanto pra um argumento `double`
                //     literal quanto um `float`. So precisao padrao fixa
                //     (CONV_FTOA_DEFAULT_PRECISION, 6) -- sem modificadores de
                //     largura/precisao/flags, mesma postura YAGNI de toda outra conversao desta
                //     funcao (ver cabecalho do arquivo). `%g` NAO e' implementado: exigiria
                //     formatacao em notacao cientifica (estilo `%e`) pra escolher entre
                //     fixo/exponencial pela magnitude, o que o `ftoa` nao oferece (e' SO'
                //     ponto-fixo) -- registrado como item de seguimento no TODO.md em vez de
                //     construido aqui, conforme o escopo explicito desta tarefa ("so' se sair de
                //     graca").
                double v = va_arg(ap, double);
                char fbuf[CONV_FTOA_BUF_MIN];
                ftoa(v, fbuf, CONV_FTOA_DEFAULT_PRECISION);
                size_t n = strlen(fbuf);
                sink(ctx, fbuf, n);
                len += n;
                fmt++;
                break;
            }
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (s == NULL) {
                    s = "(null)"; // documented NULL behaviour, see include/printf.h
                }
                size_t n = strlen(s);
                sink(ctx, s, n);
                len += n;
                fmt++;
                break;
            }
            case 'c': {
                // EN: THE promotion gotcha -- must read `int`, never `char` (see file header).
                // PT: O gotcha de promocao -- deve ler `int`, nunca `char` (ver cabecalho do
                //     arquivo).
                int v = va_arg(ap, int);
                char ch = (char)v;
                sink(ctx, &ch, 1);
                len += 1;
                fmt++;
                break;
            }
            case '%': {
                sink(ctx, "%", 1);
                len += 1;
                fmt++;
                break;
            }
            case '\0': {
                // EN: Lone trailing '%' at the very end of fmt -- print it literally, do NOT
                //     advance fmt past the NUL (the outer while condition stops the loop next).
                // PT: '%' sozinho no fim de fmt -- imprime literalmente, NAO avanca fmt alem do
                //     NUL (a condicao do while externo para o laco em seguida).
                sink(ctx, "%", 1);
                len += 1;
                break;
            }
            default: {
                // EN: Unknown directive (e.g. "%z"): print '%' and the character literally,
                //     consume no variadic argument (documented in include/printf.h).
                // PT: Diretiva desconhecida (ex.: "%z"): imprime '%' e o caractere
                //     literalmente, nao consome argumento variadico nenhum (documentado em
                //     include/printf.h).
                sink(ctx, "%", 1);
                sink(ctx, fmt, 1);
                len += 2;
                fmt++;
                break;
            }
        }
    }

    *out_len = len;
}

// ---- mini_vsnprintf's buffer sink -----------------------------------------------------------

typedef struct {
    char*  buf;
    size_t cap;  // total capacity of buf, INCLUDING the NUL slot
    size_t used; // bytes copied so far, always <= (cap == 0 ? 0 : cap - 1)
} mini_buf_ctx_t;

static void mini_buf_sink(void* ctx_, const char* data, size_t n) {
    mini_buf_ctx_t* ctx = (mini_buf_ctx_t*)ctx_;
    if (ctx->cap == 0 || n == 0) {
        return; // nothing to write / cap==0 means buf may be NULL, never touch it
    }
    size_t room = (ctx->cap - 1) - ctx->used; // space left before the reserved NUL slot
    if (room == 0) {
        return;
    }
    size_t to_copy = (n < room) ? n : room;
    memcpy(ctx->buf + ctx->used, data, to_copy);
    ctx->used += to_copy;
}

int mini_vsnprintf(char* buf, size_t cap, const char* fmt, va_list ap) {
    mini_buf_ctx_t ctx = { buf, cap, 0 };
    size_t len = 0;
    mini_format_core(mini_buf_sink, &ctx, &len, fmt, ap);
    if (cap > 0) {
        buf[ctx.used] = '\0';
    }
    return (int)len;
}

// ---- mini_printf's flush-to-stdout sink -----------------------------------------------------

#define MINI_PRINTF_FLUSH_CAP 256

typedef struct {
    char    scratch[MINI_PRINTF_FLUSH_CAP];
    size_t  used;
    ssize_t total; // sum of sys_write's successful (positive) returns so far
} mini_flush_ctx_t;

static void mini_flush(mini_flush_ctx_t* ctx) {
    if (ctx->used == 0) {
        return;
    }
    ssize_t written = sys_write(1, ctx->scratch, ctx->used);
    if (written > 0) {
        ctx->total += written;
    }
    ctx->used = 0;
}

static void mini_flush_sink(void* ctx_, const char* data, size_t n) {
    mini_flush_ctx_t* ctx = (mini_flush_ctx_t*)ctx_;
    // EN: Handles a single run LARGER than the whole scratch buffer (e.g. a long %s) by
    //     copying as much as fits, flushing, and repeating -- see file header.
    // PT: Trata uma leva UNICA maior que o buffer de rascunho inteiro (ex.: um %s longo)
    //     copiando o quanto couber, dando flush, e repetindo -- ver cabecalho do arquivo.
    while (n > 0) {
        size_t room = sizeof(ctx->scratch) - ctx->used;
        size_t to_copy = (n < room) ? n : room;
        memcpy(ctx->scratch + ctx->used, data, to_copy);
        ctx->used += to_copy;
        data += to_copy;
        n -= to_copy;
        if (ctx->used == sizeof(ctx->scratch)) {
            mini_flush(ctx);
        }
    }
}

int mini_printf(const char* fmt, ...) {
    mini_flush_ctx_t ctx;
    ctx.used = 0;
    ctx.total = 0;

    va_list ap;
    va_start(ap, fmt);
    size_t len = 0; // logical length is not part of mini_printf's return contract, discarded
    mini_format_core(mini_flush_sink, &ctx, &len, fmt, ap);
    va_end(ap);
    (void)len;

    mini_flush(&ctx); // final flush -- whatever is left in scratch after the loop above
    return (int)ctx.total;
}
