// SPDX-License-Identifier: Apache-2.0
// EN: Unit test for glintfx/include/glintfx/clock.hpp (FW-CLOCK, W20). No window, no GL context,
//     no RmlUi/GLFW dependency AT ALL -- glintfx::monotonic_now_ns() is a plain, freestanding,
//     stateless function. Same "no display to isolate here" reasoning as log_sanity.cpp.
//
//     Exercises the three hostile/edge cases this slice's own brief called out explicitly:
//       1. Called before any App/UiLayer/engine construction of any kind -- this test never
//          constructs one, proving the function has no hidden init dependency (unlike e.g.
//          glintfx::gl_proc_address(), which returns nullptr before glx_gl_load() has run --
//          this function has no such precondition at all).
//       2. Two (and many) consecutive calls: the second (and every subsequent) call is NEVER
//          smaller than the one before it -- attacked at the TIGHTEST possible boundary
//          (back-to-back, zero-sleep, tight loop), not with a sparse/large-gap sampling that
//          would hide a regression behind coarse resolution -- this session's own "attack the
//          boundary, not the exaggeration" lesson, applied here as "the boundary is zero elapsed
//          time between calls", not "call it after an absurdly long duration".
//       3. System wall-clock change resistance -- NOT independently re-verified here (mutating
//          the host machine's system clock from a CI test would be invasive/dangerous and is
//          explicitly out of this test's scope, per this file's own project-safety discipline);
//          this is std::chrono::steady_clock's own standard-mandated guarantee, inherited
//          unchanged -- see clock.hpp's own header comment for the citation and the honest
//          value-proposition verdict this test does not, and cannot, extend.
//
//     Also exercises: two calls always differ by a plausible, small, non-negative delta (proves
//     the function is actually advancing, not frozen or returning a constant), and the
//     nanosecond unit is real (a short busy-wait produces a measurably larger delta than a
//     back-to-back pair with no work between them).
// PT: Teste unit para glintfx/include/glintfx/clock.hpp (FW-CLOCK, W20). Sem janela, sem
//     contexto GL, sem dependência de RmlUi/GLFW NENHUMA -- glintfx::monotonic_now_ns() é uma
//     função simples, freestanding, sem estado. Mesma racional "nada relacionado a display pra
//     isolar aqui" do log_sanity.cpp.
//
//     Exercita os três casos hostis/de borda que o próprio briefing desta fatia citou
//     explicitamente:
//       1. Chamada antes de qualquer construção de App/UiLayer/engine -- este teste nunca
//          constrói nenhum, provando que a função não tem dependência de init escondida
//          (diferente, por exemplo, do glintfx::gl_proc_address(), que retorna nullptr antes do
//          glx_gl_load() ter rodado -- esta função não tem pré-condição nenhuma desse tipo).
//       2. Duas (e várias) chamadas consecutivas: a segunda (e toda subsequente) NUNCA é menor
//          que a anterior -- atacado na fronteira MAIS estreita possível (consecutivas, sem
//          sleep, laço apertado), não com uma amostragem esparsa/de intervalo grande que
//          esconderia uma regressão atrás da resolução grosseira -- a própria lição desta sessão
//          "ataque a fronteira, não o exagero", aplicada aqui como "a fronteira é tempo zero
//          decorrido entre chamadas", não "chame depois de uma duração absurdamente longa".
//       3. Resistência a mudança do relógio de parede do sistema -- NÃO re-verificada
//          independentemente aqui (mutar o relógio de sistema da máquina host a partir de um
//          teste de CI seria invasivo/perigoso e está explicitamente fora do escopo deste teste,
//          conforme a própria disciplina de segurança de projeto deste arquivo); esta é a própria
//          garantia mandatada pelo padrão do std::chrono::steady_clock, herdada sem alteração --
//          ver o próprio comentário de cabeçalho do clock.hpp pra citação e o veredito honesto
//          de valor próprio que este teste não estende, nem pode.
//
//     Também exercita: duas chamadas sempre diferem por um delta pequeno, plausível e
//     não-negativo (prova que a função está de fato avançando, não congelada nem retornando uma
//     constante), e que a unidade de nanossegundo é REAL -- não uma unidade mais grosseira
//     silenciosamente coagida (ex.: milissegundo). A ASSERÇÃO ORIGINAL desta última checagem
//     (comparar o delta de uma espera ocupada contra o delta de um par consecutivo sem
//     trabalho) foi provada FALSA sob mutação pelo review adversarial da W20
//     (`TST-CLOCK-UNIT`, TODO.md): trocar `nanoseconds` por `milliseconds` em `clock.cpp` (só a
//     unidade, lógica intacta) deixou a suíte VERDE -- a espera ocupada de 20 milhões de
//     iterações às vezes já dura mais de 1ms de tempo real, então mesmo em unidade de
//     milissegundo o delta da espera ocupada podia ficar > 0 (o delta consecutivo, sem
//     trabalho), sem provar nada sobre GRANULARIDADE. A substituta abaixo (ver o comentário do
//     bloco correspondente) compara o delta relatado por `monotonic_now_ns()` contra um delta
//     de referência lido diretamente de `std::chrono::steady_clock` NO MESMO INTERVALO -- uma
//     checagem de RAZÃO, autocalibrada, e por isso estável sob carga da máquina (não depende de
//     nenhuma duração absoluta).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/clock.hpp>

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}
} // namespace

int main() {
  // ---------------------------------------------------------------------------
  // EN: Case 1 -- called with no prior App/UiLayer/engine construction anywhere in this
  //     process. If this test binary links and runs at all, this precondition is already
  //     satisfied (there is nothing else in main() before this call).
  // PT: Caso 1 -- chamada sem nenhuma construção prévia de App/UiLayer/engine neste processo.
  //     Se este binário de teste linka e roda, esta pré-condição já está satisfeita (não há
  //     nada mais no main() antes desta chamada).
  // ---------------------------------------------------------------------------
  const std::uint64_t first = glintfx::monotonic_now_ns();
  // EN: The primary proof here is structural: reaching this comment at all means the call above
  //     RETURNED (a crash/hang would have aborted the test process first, and ctest reports that
  //     as a failed test regardless of any assertion inside it). The check below adds a real,
  //     non-tautological assertion on top: the returned value fits a plausible nanosecond-scale
  //     magnitude (well under ~11.5 days of ns, generous for any process uptime a test harness
  //     would realistically have) rather than e.g. garbage from an uninitialised read.
  // PT: A prova primária aqui é estrutural: chegar a este comentário já significa que a chamada
  //     acima RETORNOU (um crash/travamento teria abortado o processo de teste antes, e o ctest
  //     reporta isso como teste falho independente de qualquer assertion interna). A checagem
  //     abaixo soma uma assertion real, não-tautológica: o valor retornado cabe numa magnitude
  //     plausível de escala-nanossegundo (bem abaixo de ~11,5 dias em ns, generoso pra qualquer
  //     uptime de processo que um harness de teste teria de forma realista) em vez de, por
  //     exemplo, lixo de uma leitura não-inicializada.
  constexpr std::uint64_t kPlausibleCeilingNs = 1'000'000'000'000'000ULL; // ~11.5 days
  check(first < kPlausibleCeilingNs,
        "call before any App/UiLayer/engine construction: returns a plausible ns-scale value");

  // ---------------------------------------------------------------------------
  // EN: Case 2 -- tight-loop, zero-sleep, back-to-back calls: the boundary itself. Attacks the
  //     narrowest possible elapsed-time gap between two calls, the case most likely to expose a
  //     non-monotonic clock source (a coarse/racy implementation could plausibly return an equal
  //     or SMALLER value here where it would never do so across a sleep). Every single
  //     consecutive pair across a large sample is checked, not just the first pair.
  // PT: Caso 2 -- laço apertado, sem sleep, chamadas consecutivas: a própria fronteira. Ataca o
  //     menor intervalo de tempo decorrido possível entre duas chamadas, o caso mais provável de
  //     expor uma fonte de relógio não-monotônica (uma implementação grosseira/com corrida
  //     poderia plausivelmente retornar um valor igual ou MENOR aqui, onde nunca o faria através
  //     de um sleep). Todo par consecutivo ao longo de uma amostra grande é checado, não só o
  //     primeiro par.
  // ---------------------------------------------------------------------------
  {
    constexpr int kSamples = 200000;
    std::vector<std::uint64_t> samples;
    samples.reserve(kSamples);
    for (int i = 0; i < kSamples; ++i) {
      samples.push_back(glintfx::monotonic_now_ns());
    }

    int regressions = 0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
      if (samples[i] < samples[i - 1]) ++regressions;
    }
    check(regressions == 0, "tight-loop back-to-back calls: never regress (0 of 200000 pairs)");

    check(samples.front() != samples.back(),
          "tight-loop sample: the clock actually advances over 200000 back-to-back calls "
          "(not frozen, not a constant)");
  }

  // ---------------------------------------------------------------------------
  // EN: The nanosecond unit is REAL -- not silently coarsened to e.g. milliseconds. Proved by
  //     a RATIO check, not an absolute-duration threshold, so it stays stable under machine
  //     load: bracket a short, real chunk of work with BOTH glintfx::monotonic_now_ns() (the
  //     function under test) and an INDEPENDENT std::chrono::steady_clock reading taken in
  //     this test itself (the ground truth for "how much real time actually passed" over the
  //     SAME interval). If the unit is genuinely nanoseconds, the two deltas describe the same
  //     physical interval in the same unit and stay close (same order of magnitude, regardless
  //     of how long the interval happens to be -- fast machine, slow machine, quiet machine,
  //     loaded machine: the RATIO is what matters, never the absolute duration). If the unit
  //     were silently coarsened to milliseconds, the reported delta would undercount the SAME
  //     real interval by a factor of ~1,000,000 (1 raw "millisecond" unit per 1,000,000 real
  //     nanoseconds) -- a gap the generous kUnitDiscriminationFactor below (1000x, three
  //     orders of magnitude short of the real ~1e6 gap a millisecond mutation produces) still
  //     catches with room to spare.
  //
  //     THIS REPLACES THE PREVIOUS "busy-wait delta > back-to-back delta" ASSERTION, which
  //     `TST-CLOCK-UNIT` (TODO.md, W20 review) proved FALSE UNDER MUTATION: swapping
  //     `nanoseconds` for `milliseconds` in clock.cpp left that assertion (and the whole test)
  //     GREEN, because the 20,000,000-iteration busy-wait below can itself take longer than
  //     1ms of real time -- so even a millisecond-granularity clock could show a busy-wait
  //     delta > 0 while the back-to-back delta stayed 0, which the old assertion mistook for
  //     proof of fine granularity. A ratio against an INDEPENDENT reference clock over the SAME
  //     bracketed interval has no such blind spot: it fails unconditionally under the
  //     millisecond mutation, at any interval length, any machine load.
  // PT: A unidade de nanossegundo é REAL -- não silenciosamente coagida para, por exemplo,
  //     milissegundo. Provado por uma checagem de RAZÃO, não um limiar de duração absoluta,
  //     para se manter estável sob carga da máquina: cerca um trecho curto e real de trabalho
  //     com AMBOS glintfx::monotonic_now_ns() (a função sob teste) E uma leitura INDEPENDENTE
  //     de std::chrono::steady_clock feita neste próprio teste (a referência de "quanto tempo
  //     real de fato passou" no MESMO intervalo). Se a unidade é de fato nanossegundo, os dois
  //     deltas descrevem o mesmo intervalo físico na mesma unidade e ficam próximos (mesma
  //     ordem de grandeza, independente de quão longo o intervalo seja -- máquina rápida,
  //     lenta, ociosa, sob carga: o que importa é a RAZÃO, nunca a duração absoluta). Se a
  //     unidade fosse silenciosamente coagida para milissegundo, o delta relatado subcontaria o
  //     MESMO intervalo real por um fator de ~1.000.000 (1 unidade "milissegundo" crua por
  //     1.000.000 de nanossegundos reais) -- uma diferença que o generoso
  //     kUnitDiscriminationFactor abaixo (1000x, três ordens de grandeza aquém da diferença
  //     real de ~1e6 que a mutação de milissegundo produz) ainda pega com folga.
  //
  //     ISTO SUBSTITUI a asserção anterior "delta da espera ocupada > delta consecutivo", que o
  //     `TST-CLOCK-UNIT` (TODO.md, review da W20) provou FALSA SOB MUTAÇÃO: trocar
  //     `nanoseconds` por `milliseconds` em clock.cpp deixava aquela asserção (e a suíte
  //     inteira) VERDE, porque a espera ocupada de 20.000.000 de iterações abaixo pode, por si
  //     só, durar mais de 1ms de tempo real -- então mesmo um relógio de granularidade
  //     milissegundo podia mostrar um delta de espera ocupada > 0 enquanto o delta consecutivo
  //     ficava 0, o que a asserção antiga confundia com prova de granularidade fina. Uma razão
  //     contra um relógio de referência INDEPENDENTE sobre o MESMO intervalo cercado não tem
  //     esse ponto cego: falha incondicionalmente sob a mutação de milissegundo, em qualquer
  //     duração de intervalo, qualquer carga de máquina.
  // ---------------------------------------------------------------------------
  {
    const std::uint64_t a = glintfx::monotonic_now_ns();
    const std::uint64_t b = glintfx::monotonic_now_ns();
    check(b >= a, "back-to-back pair: b >= a (non-negative delta)");

    // EN: Interleaved reads -- the independent reference clock (`ref_before`/`ref_after`)
    //     brackets the SAME calls to the function under test (`before_busy`/`after_busy`), not
    //     a wider or narrower window, so both deltas describe as close to the identical real
    //     interval as two clock sources can get.
    // PT: Leituras intercaladas -- o relógio de referência independente (`ref_before`/
    //     `ref_after`) cerca as MESMAS chamadas à função sob teste (`before_busy`/`after_busy`),
    //     não uma janela mais larga nem mais estreita, para que os dois deltas descrevam o
    //     intervalo real mais próximo possível um do outro entre duas fontes de relógio.
    const auto ref_before = std::chrono::steady_clock::now();
    const std::uint64_t before_busy = glintfx::monotonic_now_ns();
    // EN: A deliberately cheap, dependency-free busy-wait -- no <thread>/sleep needed, just
    //     enough real work that the compiler cannot fold it away (volatile accumulator) to
    //     produce a delta clearly larger than back-to-back noise.
    // PT: Uma espera ocupada deliberadamente barata, sem dependência -- sem precisar de
    //     <thread>/sleep, só trabalho real o bastante pra que o compilador não consiga eliminar
    //     (acumulador volatile) e produzir um delta claramente maior que o ruído consecutivo.
    volatile std::uint64_t sink = 0;
    for (std::uint64_t i = 0; i < 20000000ULL; ++i) {
      sink += i;
    }
    const std::uint64_t after_busy = glintfx::monotonic_now_ns();
    const auto ref_after = std::chrono::steady_clock::now();
    (void)sink;

    check(after_busy >= before_busy, "busy-wait pair: after >= before (still non-decreasing)");

    const std::uint64_t reported_delta_ns = after_busy - before_busy;
    const std::uint64_t reference_delta_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(ref_after - ref_before).count());

    // EN: Generous, RATIO-based discrimination factor -- geometric middle ground between "1"
    //     (the ratio a correct nanosecond unit produces: both deltas measure the same real
    //     interval) and "~1,000,000" (the ratio a millisecond mutation produces). 1000x is
    //     three orders of magnitude of margin on EITHER side: nowhere near tight enough to be
    //     tripped by ordinary measurement jitter between the two clock reads, but nowhere near
    //     loose enough to let a millisecond-scale mutation slip through.
    // PT: Fator de discriminação generoso, baseado em RAZÃO -- meio-termo geométrico entre "1"
    //     (a razão que uma unidade correta de nanossegundo produz: os dois deltas medem o
    //     mesmo intervalo real) e "~1.000.000" (a razão que uma mutação de milissegundo
    //     produz). 1000x é três ordens de grandeza de margem em QUALQUER lado: nem de longe
    //     apertado o bastante pra ser disparado por ruído comum de medição entre as duas
    //     leituras de relógio, nem de longe frouxo o bastante pra deixar passar uma mutação de
    //     escala-milissegundo.
    constexpr std::uint64_t kUnitDiscriminationFactor = 1000;
    check(reference_delta_ns == 0 ||
              reported_delta_ns * kUnitDiscriminationFactor >= reference_delta_ns,
          "reported delta tracks an INDEPENDENT std::chrono::steady_clock reading of the SAME "
          "bracketed interval within 1000x -- fails by a ~1e6x margin if the unit were "
          "silently coarsened from nanoseconds to milliseconds (TST-CLOCK-UNIT fix: a "
          "ratio-based check that actually distinguishes ns from ms, unlike the "
          "duration-based check it replaces)");
  }

  // ---------------------------------------------------------------------------
  // EN: FW-SLEEP (W26) -- glintfx::sleep_ms() floor-only check. This is deliberately the ONLY
  //     assertion this slice makes about sleep_ms()'s duration: a FLOOR ("slept at LEAST the
  //     requested time"), never a CEILING ("slept at MOST X"). A ceiling assertion is flaky by
  //     construction on a shared, contended CI/local runner -- the scheduler gives no upper-bound
  //     wake guarantee, and this project has already been burned by exactly that class of
  //     flakiness elsewhere (see this slice's own brief). bracketed with the SAME
  //     monotonic_now_ns() this file already exercises above, so this test adds no new
  //     dependency.
  // PT: FW-SLEEP (W26) -- checagem de SÓ PISO do glintfx::sleep_ms(). Esta é deliberadamente a
  //     ÚNICA asserção que esta fatia faz sobre a duração do sleep_ms(): um PISO ("dormiu PELO
  //     MENOS o tempo pedido"), nunca um TETO ("dormiu NO MÁXIMO X"). Uma asserção de teto é
  //     flaky por construção num runner compartilhado e contendido -- o escalonador não dá
  //     garantia de limite superior de acordar, e este projeto já foi queimado exatamente por
  //     essa classe de flakiness em outro lugar (ver o próprio briefing desta fatia). Cercada
  //     com o MESMO monotonic_now_ns() que este arquivo já exercita acima, então este teste não
  //     soma dependência nova nenhuma.
  // ---------------------------------------------------------------------------
  {
    constexpr std::uint64_t kSleepMs = 20;
    const std::uint64_t before = glintfx::monotonic_now_ns();
    glintfx::sleep_ms(kSleepMs);
    const std::uint64_t after = glintfx::monotonic_now_ns();
    const std::uint64_t elapsed_ns = after - before;
    const std::uint64_t floor_ns = kSleepMs * 1'000'000ULL;
    check(elapsed_ns >= floor_ns,
          "sleep_ms(20): elapsed monotonic_now_ns() delta >= 20ms floor (floor-only, NEVER a "
          "ceiling assertion -- see this block's own comment)");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "clock_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("clock_sanity: PASS");
  return 0;
}
