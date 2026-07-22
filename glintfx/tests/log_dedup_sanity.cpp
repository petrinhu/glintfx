// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for src/log_dedup.hpp (LOGTHR-1, Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decision D8). No RmlUi, no GLFW,
//     no window, no I/O of any kind -- LogDedup::should_print is a plain in-memory decision.
//     Exercises: 1st-occurrence-always-prints, mid-run suppression, the 10/100/1000 summary
//     re-emit boundary (with the built count string), per-message independence (two distinct
//     keys never bleed into each other's count), is_error bypassing dedup unconditionally
//     (never tracked, never suppressed even across many repeats), the LRU cap under a stream of
//     10 000 distinct hostile messages (bounded memory, no crash), and empty/huge message safety.
// PT: Teste unit puro para src/log_dedup.hpp (LOGTHR-1, Onda 2, framework-2D --
//     docs/superpowers/plans/2026-07-22-onda2-input-host.md, decisão D8). Sem RmlUi, sem GLFW,
//     sem janela, sem I/O de espécie nenhuma -- LogDedup::should_print é uma decisão em memória
//     pura. Exercita: 1ª-ocorrência-sempre-imprime, supressão no meio da execução, a fronteira
//     de reemissão de sumário 10/100/1000 (com a string de contagem construída), independência
//     entre mensagens (duas chaves distintas nunca vazam contagem uma pra outra), is_error
//     ignorando o dedup incondicionalmente (nunca rastreado, nunca suprimido mesmo através de
//     muitas repetições), o teto LRU sob um stream de 10 000 mensagens hostis distintas
//     (memória limitada, sem crash), e segurança de mensagem vazia/enorme.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/log_dedup.hpp"
#include <cstdio>
#include <string>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: 1st occurrence always prints, verbatim (out_line == message, unmodified).
  // PT: 1ª ocorrência sempre imprime, verbatim (out_line == mensagem, sem modificação).
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    const bool printed = dedup.should_print(3, "hello", false, out);
    check(printed, "1st occurrence: should_print returns true");
    check(out == "hello", "1st occurrence: out_line is the verbatim message");
  }

  // ---------------------------------------------------------------------------
  // EN: Occurrences 2..9 of the SAME (type, message) pair are suppressed (should_print ==
  //     false); occurrence 10 re-emits a summary line containing the count.
  // PT: Ocorrências 2..9 do MESMO par (type, message) são suprimidas (should_print == false);
  //     a ocorrência 10 reemite uma linha de sumário contendo a contagem.
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    dedup.should_print(1, "flood", false, out); // occurrence 1: prints, established baseline
    for (int i = 2; i <= 9; ++i) {
      out.clear();
      const bool printed = dedup.should_print(1, "flood", false, out);
      check(!printed, "occurrences 2..9: suppressed (should_print == false)");
      check(out.empty(), "occurrences 2..9: out_line left untouched");
    }
    out.clear();
    const bool printed10 = dedup.should_print(1, "flood", false, out);
    check(printed10, "occurrence 10: summary line re-emitted");
    check(out.find("flood") != std::string::npos, "occurrence 10: summary contains original message");
    check(out.find("10") != std::string::npos, "occurrence 10: summary contains the count");
    check(out.find("suppressed") != std::string::npos, "occurrence 10: summary says suppressed");
  }

  // ---------------------------------------------------------------------------
  // EN: 10/100/1000 re-emit boundary -- summary fires ONLY at exact powers of ten, never at a
  //     plain multiple of ten (e.g. 20, 30) that is not itself a power of ten.
  // PT: Fronteira de reemissão 10/100/1000 -- o sumário dispara SÓ em potências de dez exatas,
  //     nunca num múltiplo simples de dez (ex.: 20, 30) que não seja ele mesmo potência de dez.
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    int summary_hits = 0;
    for (int i = 1; i <= 1000; ++i) {
      out.clear();
      if (dedup.should_print(2, "repeat-me", false, out)) {
        ++summary_hits;
        if (i > 1) {
          check(out.find(std::to_string(i)) != std::string::npos,
                "summary at count i contains the exact count i");
        }
      }
    }
    // EN: Hits at i = 1 (first print), 10, 100, 1000 -- exactly 4.
    // PT: Disparos em i = 1 (1ª impressão), 10, 100, 1000 -- exatamente 4.
    check(summary_hits == 4, "exactly 4 print events across 1000 occurrences (1, 10, 100, 1000)");
  }

  // ---------------------------------------------------------------------------
  // EN: Per-message independence -- two distinct (type, message) pairs are tracked in fully
  //     separate counters; repeating one never suppresses or advances the other's count.
  // PT: Independência entre mensagens -- dois pares (type, message) distintos são rastreados em
  //     contadores totalmente separados; repetir um nunca suprime nem avança a contagem do outro.
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    check(dedup.should_print(1, "message A", false, out), "message A: 1st occurrence prints");
    check(dedup.should_print(1, "message B", false, out), "message B (distinct text): 1st occurrence ALSO prints");
    check(!dedup.should_print(1, "message A", false, out), "message A: 2nd occurrence suppressed");
    check(!dedup.should_print(1, "message B", false, out), "message B: 2nd occurrence suppressed independently");

    // EN: Same message text, DIFFERENT type -- also tracked independently (the key is the pair).
    // PT: Mesmo texto de mensagem, type DIFERENTE -- também rastreado independentemente (a
    //     chave é o par).
    check(dedup.should_print(2, "message A", false, out),
          "same text, different type: treated as a distinct key (1st occurrence prints)");
  }

  // ---------------------------------------------------------------------------
  // EN: is_error bypasses dedup unconditionally -- always prints, even across many repeats of
  //     the identical (type, message) pair; never tracked (does not consume LRU capacity, does
  //     not interfere with a same-text non-error message's own count).
  // PT: is_error ignora o dedup incondicionalmente -- sempre imprime, mesmo através de muitas
  //     repetições do mesmo par (type, message) idêntico; nunca rastreado (não consome
  //     capacidade LRU, não interfere na contagem própria de uma mensagem não-erro com o mesmo
  //     texto).
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    for (int i = 0; i < 50; ++i) {
      out.clear();
      const bool printed = dedup.should_print(1, "critical failure", true, out);
      check(printed, "is_error: every occurrence prints (never suppressed)");
      check(out == "critical failure", "is_error: out_line is the verbatim message every time");
    }
    // EN: A non-error occurrence of the SAME text is unaffected -- still its own 1st occurrence.
    // PT: Uma ocorrência não-erro do MESMO texto não é afetada -- ainda é a própria 1ª ocorrência.
    check(dedup.should_print(1, "critical failure", false, out),
          "is_error bypass does not pollute the tracked table for the same text as non-error");
  }

  // ---------------------------------------------------------------------------
  // EN: LRU cap -- a stream of 10 000 distinct hostile messages against a small capacity never
  //     crashes and never grows the table past that capacity.
  // PT: Teto LRU -- um stream de 10 000 mensagens hostis distintas contra um teto pequeno nunca
  //     crasha e nunca cresce a tabela além desse teto.
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup(/*capacity=*/8);
    std::string out;
    for (int i = 0; i < 10000; ++i) {
      const bool printed = dedup.should_print(0, "distinct-" + std::to_string(i), false, out);
      check(printed, "every never-before-seen message prints on its 1st occurrence");
    }
    // EN: The most recent message must still be tracked (it was just inserted, MRU).
    // PT: A mensagem mais recente ainda deve estar rastreada (acabou de ser inserida, MRU).
    check(!dedup.should_print(0, "distinct-9999", false, out),
          "the most recently inserted message is still resident (2nd occurrence suppressed)");
    // EN: An early message (evicted long ago under an 8-entry cap) is treated as new again.
    // PT: Uma mensagem antiga (evicta há muito sob um teto de 8 entradas) é tratada como nova
    //     de novo.
    check(dedup.should_print(0, "distinct-0", false, out),
          "an evicted-long-ago message is treated as a fresh 1st occurrence again");
  }

  // ---------------------------------------------------------------------------
  // EN: Hostile edge cases -- empty message and a very large message are both handled safely
  //     (no crash, same 1st-occurrence/suppression semantics as any other message).
  // PT: Casos-limite hostis -- mensagem vazia e mensagem bem grande são ambas tratadas com
  //     segurança (sem crash, mesma semântica de 1ª-ocorrência/supressão de qualquer outra
  //     mensagem).
  // ---------------------------------------------------------------------------
  {
    LogDedup dedup;
    std::string out;
    check(dedup.should_print(0, "", false, out), "empty message: 1st occurrence prints");
    check(out.empty(), "empty message: out_line is empty (verbatim)");
    check(!dedup.should_print(0, "", false, out), "empty message: 2nd occurrence suppressed");

    const std::string huge(1'000'000, 'x');
    check(dedup.should_print(0, huge, false, out), "huge (1 MB) message: 1st occurrence prints");
    check(out.size() == huge.size(), "huge message: out_line round-trips the full size");
    check(!dedup.should_print(0, huge, false, out), "huge message: 2nd occurrence suppressed");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "log_dedup_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("log_dedup_sanity: PASS");
  return 0;
}
