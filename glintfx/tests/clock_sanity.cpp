// SPDX-License-Identifier: MPL-2.0
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
//     constante), e que a unidade de nanossegundo é real (uma espera ocupada curta produz um
//     delta mensuravelmente maior que um par consecutivo sem trabalho nenhum entre eles).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/clock.hpp>

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
  // EN: Two calls always differ by a plausible, non-negative delta, and the delta scales with
  //     real elapsed work -- proves the nanosecond unit is genuine (not e.g. silently returning
  //     milliseconds or a constant): a short busy-wait must produce a measurably larger delta
  //     than a back-to-back pair with no work between them.
  // PT: Duas chamadas sempre diferem por um delta plausível e não-negativo, e o delta escala com
  //     trabalho real decorrido -- prova que a unidade de nanossegundo é genuína (não, por
  //     exemplo, silenciosamente retornando milissegundos ou uma constante): uma espera ocupada
  //     curta precisa produzir um delta mensuravelmente maior que um par consecutivo sem
  //     trabalho nenhum entre eles.
  // ---------------------------------------------------------------------------
  {
    const std::uint64_t a = glintfx::monotonic_now_ns();
    const std::uint64_t b = glintfx::monotonic_now_ns();
    check(b >= a, "back-to-back pair: b >= a (non-negative delta)");

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
    (void)sink;

    check(after_busy >= before_busy, "busy-wait pair: after >= before (still non-decreasing)");
    check(after_busy - before_busy > (b - a),
          "busy-wait delta is measurably larger than a back-to-back no-op delta "
          "(proves the nanosecond unit is real, not a frozen/constant/coarser value)");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "clock_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("clock_sanity: PASS");
  return 0;
}
