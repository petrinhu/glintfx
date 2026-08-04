// SPDX-License-Identifier: Apache-2.0
// EN: FW-CLOCK (framework-2D, W20, pedido C do consumidor GusWorld 2026-07-29) -- a single,
//     thin, always-available free function: a monotonic nanosecond clock, the
//     `SDL_GetTicksNS()` equivalent a host migrating its own window-owning shell onto this
//     library needs for a fixed-step accumulator, duration measurement, or animation timing.
//     NOT frame `dt` -- `App::set_frame_callback`'s hook already delivers that; this is
//     ABSOLUTE monotonic time, for code that needs to measure elapsed time ACROSS frames or
//     independent of the frame loop entirely (e.g. before any `App`/`UiLayer` exists).
//
//     ⚠ HONEST VALUE-PROPOSITION VERDICT (read before treating this as more than it is): the
//     consumer that requested this ALSO argued against the request, twice, and the argument is
//     correct as far as it goes -- `std::chrono::steady_clock` is C++ standard library, not a
//     third-party dependency (linking it pins nobody down, the same way using `std::vector`
//     does not), and at the consumer's own most-important call site the clock is ALREADY an
//     injected dependency (`now_fn = [] { return SDL_GetTicksNS(); }`), so swapping the source
//     to `std::chrono::steady_clock` directly is exactly as easy as swapping it to this
//     function. If the ONLY property wanted were "a clock that does not go backwards", this
//     header would add ZERO value over calling `std::chrono::steady_clock::now()` directly --
//     `steady_clock` is DEFINED by the C++ standard to never decrease as physical time moves
//     forward, full stop, no wrapper needed to obtain that guarantee. Shipped anyway per the
//     project lead's explicit, standing decision (`TODO.md`'s `FW-CLOCK` entry, "no negatives
//     to the consumer, migrate the dependency count to nothing-but-the-standard-library") --
//     the actual, measured, non-zero value this thin wrapper adds is narrower and stated
//     plainly:
//       1. FIXED NANOSECOND CONTRACT, not "whatever `steady_clock::period` happens to be" on a
//          given platform/standard-library implementation (the C++ standard leaves that
//          IMPLEMENTATION-DEFINED). `monotonic_now_ns()` always returns nanoseconds, converted
//          internally with `std::chrono::duration_cast` -- a caller never needs to reason about
//          or convert away from the local tick period.
//       2. ZERO-DIFF MIGRATION SHAPE: this function's exact signature -- `uint64_t (*)()`, no
//          arguments -- matches `SDL_GetTicksNS()` byte-for-byte, so the ONE injection point the
//          consumer already has (`now_fn`, `app/src/screen_state.cpp:69`) is a literal one-token
//          swap, with no `std::chrono` type or `.count()` call needed at that call site.
//       3. AN INTEGER ABI SHAPE, not a `std::chrono::time_point`/`duration` template
//          instantiation, crossing this library's public boundary -- consistent with this
//          library's existing precedent for thin, process-wide free functions
//          (`glintfx::version()`, `glintfx::gl_proc_address()`): no complex third-party OR
//          standard-library template type is ever part of the contract a consumer links
//          against.
//       4. This library now TESTS AND VERSIONS the monotonicity property as ITS OWN contract
//          (`tests/clock_sanity.cpp`) rather than silently re-exporting the standard library's
//          guarantee by reference -- a consumer reading glintfx's own CHANGELOG/docs sees this
//          promised explicitly, without having to already know the C++ standard's `steady_clock`
//          wording.
//     None of the four points above is "we invented a stronger monotonicity guarantee than
//     `steady_clock` already has" -- they do not exist. If a future host finds this wrapper adds
//     nothing IT specifically needs, calling `std::chrono::steady_clock` directly remains exactly
//     as valid, exactly as the consumer's own argument says.
//
//     EPOCH: unspecified, fixed for the lifetime of the process (same contract
//     `std::chrono::steady_clock` itself makes -- this function is a thin wrapper over it, see
//     clock.cpp). Do NOT assume it is process start, the Unix epoch, or `0` at any particular
//     point -- only DELTAS between two calls in the SAME process are meaningful. Two different
//     processes' return values are never comparable.
//
//     RETURN TYPE: a plain `std::uint64_t` of nanoseconds (not `std::chrono::duration`) --
//     deliberately, see point 3 above. At this resolution a `uint64_t` does not wrap for
//     roughly 584 years of process uptime (2^64 ns), so wraparound is not a case this API
//     needs to define behaviour for.
//
//     MONOTONICITY UNDER A SYSTEM CLOCK CHANGE: this is exactly the property `steady_clock`
//     (as opposed to `system_clock`) is specified to resist -- an operator changing the wall
//     clock, DST, or an NTP step must NOT move this function's return value backwards or
//     forwards out of step with elapsed physical time. This is the C++ standard's own
//     `steady_clock` guarantee, inherited unchanged, not independently re-verified by this
//     library's test suite (mutating the HOST machine's system clock from a CI test would be
//     invasive/dangerous and is explicitly out of scope -- see clock_sanity.cpp's own
//     doc-comment for what IS verified instead: back-to-back, zero-sleep calls never regress,
//     which is the tightest, most adversarial approximation of "attack the boundary" available
//     without touching the system clock).
//
//     THREADING: unlike most of this library's other utility headers (`log.hpp`, `i18n.hpp`),
//     this function has NO shared/global state of its own -- it is a direct, stateless
//     delegation to `std::chrono::steady_clock::now()`, which is itself required by the C++
//     standard to be safe to call concurrently from multiple threads with no external
//     synchronisation. Safe to call from ANY thread, at ANY time, including before any
//     `glintfx::App`/`glintfx::UiLayer` has been constructed in this process.
// PT: FW-CLOCK (framework-2D, W20, pedido C do consumidor GusWorld 2026-07-29) -- uma única
//     função livre, fina, sempre disponível: um relógio monotônico em nanossegundos, o
//     equivalente ao `SDL_GetTicksNS()` de que um host migrando a própria casca dona-de-janela
//     para esta biblioteca precisa para um acumulador de passo fixo, medição de duração, ou
//     temporização de animação. NÃO é o `dt` de quadro -- o hook de `App::set_frame_callback`
//     já entrega isso; isto é tempo monotônico ABSOLUTO, para código que precisa medir tempo
//     decorrido ATRAVÉS de frames ou independente do frame loop por inteiro (ex.: antes de
//     qualquer `App`/`UiLayer` existir).
//
//     ⚠ VEREDITO HONESTO DE VALOR PRÓPRIO (leia antes de tratar isto como mais do que é): o
//     consumidor que pediu isto TAMBÉM argumentou contra o próprio pedido, duas vezes, e o
//     argumento está correto até onde vai -- `std::chrono::steady_clock` é biblioteca padrão da
//     linguagem C++, não dependência de terceiro (depender dela não prende ninguém, do mesmo
//     jeito que usar `std::vector` não prende), e no lugar mais importante do próprio consumidor
//     o relógio JÁ é uma dependência injetada (`now_fn = [] { return SDL_GetTicksNS(); };`),
//     então trocar a fonte por `std::chrono::steady_clock` direto é exatamente tão fácil quanto
//     trocar por esta função. Se a ÚNICA propriedade desejada fosse "um relógio que não anda pra
//     trás", este header adicionaria valor ZERO sobre chamar `std::chrono::steady_clock::now()`
//     direto -- o `steady_clock` é DEFINIDO pelo padrão C++ para nunca decrescer conforme o
//     tempo físico avança, ponto final, nenhum wrapper é necessário para obter essa garantia.
//     Entregue mesmo assim por decisão explícita e vigente do líder do projeto (a entrada
//     `FW-CLOCK` do `TODO.md`, "sem negativas ao consumidor, migrar a contagem de dependências
//     pra nada-além-da-biblioteca-padrão") -- o valor real, medido, não-zero que este wrapper
//     fino de fato adiciona é mais estreito e dito sem rodeios:
//       1. CONTRATO FIXO DE NANOSSEGUNDOS, não "o que quer que `steady_clock::period` seja" numa
//          dada plataforma/implementação de biblioteca padrão (o padrão C++ deixa isso
//          DEFINIDO-PELA-IMPLEMENTAÇÃO). `monotonic_now_ns()` sempre retorna nanossegundos,
//          convertido internamente com `std::chrono::duration_cast` -- um chamador nunca precisa
//          raciocinar sobre nem converter a partir do período de tick local.
//       2. FORMA DE MIGRAÇÃO SEM DIFF: a assinatura exata desta função -- `uint64_t (*)()`, sem
//          argumentos -- casa byte-a-byte com `SDL_GetTicksNS()`, então o ÚNICO ponto de injeção
//          que o consumidor já tem (`now_fn`, `app/src/screen_state.cpp:69`) é uma troca literal
//          de um token, sem precisar de nenhum tipo `std::chrono` nem chamada `.count()` naquele
//          call site.
//       3. UMA FORMA DE ABI INTEIRA, não uma instanciação de template
//          `std::chrono::time_point`/`duration`, cruzando a fronteira pública desta biblioteca --
//          consistente com o precedente já existente desta biblioteca para funções livres finas
//          e de escopo de processo (`glintfx::version()`, `glintfx::gl_proc_address()`): nenhum
//          tipo template complexo de terceiro OU da biblioteca padrão faz parte do contrato
//          contra o qual um consumidor linka.
//       4. Esta biblioteca agora TESTA E VERSIONA a propriedade de monotonicidade como CONTRATO
//          PRÓPRIO dela (`tests/clock_sanity.cpp`) em vez de re-exportar silenciosamente a
//          garantia da biblioteca padrão por referência -- um consumidor lendo o próprio
//          CHANGELOG/docs da glintfx vê isso prometido explicitamente, sem precisar já conhecer
//          a redação do `steady_clock` do padrão C++.
//     Nenhum dos quatro pontos acima é "inventamos uma garantia de monotonicidade mais forte que
//     a do `steady_clock`" -- elas não existem. Se um host futuro achar que este wrapper não
//     adiciona nada que ELE especificamente precise, chamar `std::chrono::steady_clock` direto
//     continua exatamente tão válido quanto o próprio argumento do consumidor diz.
//
//     ÉPOCA: não especificada, fixa pela vida do processo (mesmo contrato que o próprio
//     `std::chrono::steady_clock` faz -- esta função é um wrapper fino sobre ele, ver clock.cpp).
//     NÃO presuma que é o início do processo, a época Unix, ou `0` em ponto nenhum específico --
//     só DELTAS entre duas chamadas no MESMO processo têm significado. Valores de retorno de
//     dois processos diferentes nunca são comparáveis.
//
//     TIPO DE RETORNO: um `std::uint64_t` simples de nanossegundos (não `std::chrono::duration`)
//     -- deliberadamente, ver o ponto 3 acima. Nesta resolução um `uint64_t` não dá a volta por
//     aproximadamente 584 anos de uptime de processo (2^64 ns), então wraparound não é um caso
//     para o qual esta API precise definir comportamento.
//
//     MONOTONICIDADE SOB MUDANÇA DO RELÓGIO DO SISTEMA: esta é exatamente a propriedade que o
//     `steady_clock` (em contraste com o `system_clock`) é especificado para resistir -- um
//     operador mudando o relógio de parede, DST, ou um step de NTP NÃO pode mover o valor de
//     retorno desta função pra trás nem pra frente fora de passo com o tempo físico decorrido.
//     Esta é a própria garantia do `steady_clock` do padrão C++, herdada sem alteração, não
//     re-verificada independentemente pela suíte de testes desta biblioteca (mutar o relógio do
//     sistema da máquina HOST a partir de um teste de CI seria invasivo/perigoso e está
//     explicitamente fora de escopo -- ver o próprio comentário de cabeçalho do
//     clock_sanity.cpp pro que É verificado no lugar: chamadas consecutivas, sem sleep, nunca
//     regridem, a aproximação mais estreita e adversarial de "ataque a fronteira" disponível sem
//     tocar o relógio do sistema).
//
//     THREADING: diferente da maioria dos outros headers utilitários desta biblioteca
//     (`log.hpp`, `i18n.hpp`), esta função NÃO tem estado compartilhado/global próprio -- é uma
//     delegação direta, sem estado, para `std::chrono::steady_clock::now()`, que por si só é
//     exigida pelo padrão C++ a ser segura para chamar concorrentemente de várias threads sem
//     sincronização externa. Seguro chamar de QUALQUER thread, a QUALQUER momento, inclusive
//     antes de qualquer `glintfx::App`/`glintfx::UiLayer` ter sido construído neste processo.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>

namespace glintfx {

// EN: Monotonic nanosecond timestamp against an unspecified, process-fixed epoch. See this
//     file's own header comment for the full contract (epoch, resolution, threading, the honest
//     value-proposition verdict). Guaranteed non-decreasing across any two calls from the SAME
//     process, in wall-clock/physical-time order, regardless of thread, and immune to system
//     wall-clock adjustments (`std::chrono::steady_clock`'s own defining property).
// PT: Timestamp monotônico em nanossegundos contra uma época não especificada, fixa pelo
//     processo. Ver o próprio comentário de cabeçalho deste arquivo pro contrato completo
//     (época, resolução, threading, o veredito honesto de valor próprio). Garantidamente
//     não-decrescente entre quaisquer duas chamadas do MESMO processo, em ordem de
//     tempo-físico/relógio-de-parede, independente de thread, e imune a ajustes do relógio de
//     parede do sistema (a própria propriedade definidora do `std::chrono::steady_clock`).
std::uint64_t monotonic_now_ns();

// EN: FW-SLEEP (framework-2D, W26, requested by the consumer GusWorld 2026-07-30 -- the
//     consumer ITSELF argued against its own request, and the argument is correct as far as it
//     goes; see this project's `TODO.md` `FW-SLEEP` entry for the full back-and-forth) --
//     cooperative, best-effort CPU cession for the CALLING thread, in whole milliseconds.
//
//     ⚠ DO NOT call this from inside `App::set_frame_callback`'s hook. `App` already has an
//     opinion about frame cadence -- `set_swap_interval()` (see `app.hpp`) makes the driver's
//     vsync block INSIDE the swap call, every frame. Sleeping inside the frame callback ADDS to
//     that block instead of replacing it, so the two delays sum and the effective frame rate
//     drops below what vsync alone would already deliver. This is exactly the objection the
//     requesting consumer raised against their own request, and it is correct: this function is
//     for the HOST's OWN modal loops (a menu loop the host drives itself, outside `App::run()`
//     entirely) and for embed mode (`UiLayer`), where the host -- not this library -- owns frame
//     cadence and decides when to yield. Neither of those two intended call sites has a vsync
//     block already competing for the same time budget.
//
//     DECLARED CEILING (read before treating this as more than it is): this is CPU cession, not
//     a timer. NO precision-of-wake guarantee -- the underlying `std::this_thread::sleep_for`
//     may (and under a loaded scheduler, WILL) wake meaningfully later than requested; it is
//     never specified or tested to wake EARLIER (see `clock_sanity.cpp`'s own test for why only
//     the floor is asserted, never a ceiling -- asserting an upper bound on a shared,
//     contended runner is flaky by construction, not a hardening). NO cancellation -- once
//     called, the calling thread is blocked for at least the requested duration, full stop, no
//     way to interrupt it from another thread. `0` is a valid, well-defined no-op-ish call (some
//     `std::this_thread::sleep_for` implementations still yield the thread's remaining
//     timeslice on `0`; this function makes no promise either way about that edge).
//
//     UNIT AND PARAMETER TYPE: `std::uint64_t` milliseconds, following this same header's
//     `monotonic_now_ns()` precedent -- a plain, fixed-width integer crossing the public
//     boundary (see that function's own point 3 above), not a `std::chrono::duration` template
//     instantiation.
//
//     THREADING: blocks ONLY the calling thread (`std::this_thread::sleep_for`'s own contract);
//     no shared/global state, safe to call from any thread, at any time, including before any
//     `glintfx::App`/`glintfx::UiLayer` exists in this process -- same independence class as
//     `monotonic_now_ns()` above.
// PT: FW-SLEEP (framework-2D, W26, pedido do consumidor GusWorld 2026-07-30 -- o próprio
//     consumidor argumentou CONTRA o próprio pedido, e o argumento está correto até onde vai;
//     ver a entrada `FW-SLEEP` do `TODO.md` deste projeto pro debate completo) -- cessão de CPU
//     cooperativa, best-effort, pela thread CHAMADORA, em milissegundos inteiros.
//
//     ⚠ NÃO chame isto de dentro do hook de `App::set_frame_callback`. O `App` já TEM opinião
//     sobre cadência de quadro -- `set_swap_interval()` (ver `app.hpp`) faz o vsync do driver
//     bloquear DENTRO da própria chamada de swap, a cada quadro. Dormir dentro do frame
//     callback SOMA a esse bloqueio em vez de substituí-lo, então os dois atrasos se somam e a
//     taxa efetiva de quadro cai abaixo do que o vsync sozinho já entregaria. Este é exatamente
//     o argumento que o próprio consumidor que pediu levantou contra o próprio pedido, e ele
//     está certo: esta função é para os LAÇOS MODAIS PRÓPRIOS do host (um laço de menu que o
//     host mesmo dirige, fora do `App::run()` por inteiro) e para o modo embed (`UiLayer`), onde
//     o host -- não esta biblioteca -- é dono da cadência de quadro e decide quando ceder.
//     Nenhum dos dois sítios de chamada pretendidos tem um bloqueio de vsync já competindo pelo
//     mesmo orçamento de tempo.
//
//     TETO DECLARADO (leia antes de tratar isto como mais do que é): isto é cessão de CPU, não
//     um timer. SEM garantia de precisão de acordar -- o `std::this_thread::sleep_for`
//     subjacente pode (e sob um escalonador carregado, VAI) acordar significativamente depois
//     do pedido; nunca é especificado nem testado a acordar ANTES (ver o próprio teste do
//     `clock_sanity.cpp` pro motivo de só o piso ser afirmado, nunca um teto -- afirmar um
//     limite superior num runner compartilhado e contendido é flaky por construção, não um
//     endurecimento). SEM cancelamento -- uma vez chamada, a thread chamadora fica bloqueada
//     por pelo menos a duração pedida, ponto final, sem forma de interromper de outra thread.
//     `0` é uma chamada válida e bem definida, quase-no-op (algumas implementações de
//     `std::this_thread::sleep_for` ainda cedem o resto do timeslice da thread em `0`; esta
//     função não promete nada sobre essa borda em nenhum dos dois sentidos).
//
//     UNIDADE E TIPO DO PARÂMETRO: `std::uint64_t` milissegundos, seguindo o mesmo precedente
//     do `monotonic_now_ns()` deste header -- um inteiro simples, de largura fixa, cruzando a
//     fronteira pública (ver o ponto 3 da própria função acima), não uma instanciação de
//     template `std::chrono::duration`.
//
//     THREADING: bloqueia SÓ a thread chamadora (o próprio contrato do
//     `std::this_thread::sleep_for`); sem estado compartilhado/global, seguro chamar de
//     qualquer thread, a qualquer momento, inclusive antes de qualquer
//     `glintfx::App`/`glintfx::UiLayer` existir neste processo -- mesma classe de independência
//     do `monotonic_now_ns()` acima.
void sleep_ms(std::uint64_t ms);

} // namespace glintfx
