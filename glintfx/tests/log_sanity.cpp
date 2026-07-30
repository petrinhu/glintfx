// SPDX-License-Identifier: MPL-2.0
// EN: Unit test for glintfx/include/glintfx/log.hpp (FW-LOG, W20). No window, no GL context, no
//     RmlUi/GLFW dependency AT ALL -- glintfx::log()/log_info()/log_warn()/log_error() are plain,
//     freestanding functions over a single process-wide sink. Verification is done entirely
//     through a CUSTOM sink capturing (level, message) pairs in-process (glintfx::set_log_sink())
//     -- no stderr capture/freopen/dup2 needed (unlike log_throttle_embed_sanity.cpp, which has
//     to capture real stderr because it is exercising RmlUi's own LogMessage path; this test
//     exercises log.hpp directly and the sink IS the observation point).
//
//     Exercises: level dispatch (log_info/log_warn/log_error tag the sink with the right
//     LogLevel), the printf-style formatting path (log_info("count=%d", 42)), the PLAIN path
//     (log()) round-tripping a literal '%' verbatim (proving it never touches vsnprintf, per
//     log.hpp's "TWO DISTINCT ENTRY POINTS" paragraph), nullptr message/fmt hardening, the %n
//     hostile-format rejection (asserting BOTH that the sink never sees the caller's arguments
//     interpreted against it AND that the target of a would-be %n write is untouched), a dangling
//     '%' at the end of fmt, the exact kLogMaxMessageBytes truncation BOUNDARY (N accepted
//     verbatim vs N+1 trutruncated -- not an oversized value, per this session's own "attack the
//     boundary, not the exaggeration" lesson), sink replacement (2nd registration silences the
//     1st), nullptr-sink reset to the built-in default, and sink reentrancy (a sink that installs
//     a replacement AND logs again from inside its own body).
// PT: Teste unit para glintfx/include/glintfx/log.hpp (FW-LOG, W20). Sem janela, sem contexto GL,
//     sem dependência de RmlUi/GLFW NENHUMA -- glintfx::log()/log_info()/log_warn()/log_error()
//     são funções simples, freestanding, sobre um único sink processo-inteiro. A verificação é
//     feita inteiramente por um sink CUSTOMIZADO capturando pares (level, message) em processo
//     (glintfx::set_log_sink()) -- sem captura de stderr/freopen/dup2 (diferente do
//     log_throttle_embed_sanity.cpp, que precisa capturar o stderr real porque exercita o próprio
//     caminho LogMessage do RmlUi; este teste exercita o log.hpp direto e o sink É o ponto de
//     observação).
//
//     Exercita: despacho de nível (log_info/log_warn/log_error marcam o sink com o LogLevel
//     certo), o caminho de formatação estilo printf (log_info("count=%d", 42)), o caminho PLANO
//     (log()) fazendo round-trip de um '%' literal verbatim (provando que ele nunca toca o
//     vsnprintf, conforme o parágrafo "DOIS PONTOS DE ENTRADA DISTINTOS" do log.hpp), hardening
//     de message/fmt nullptr, a rejeição de format hostil %n (verificando TANTO que o sink nunca
//     vê os argumentos do chamador interpretados contra ele QUANTO que o alvo de uma escrita %n
//     tentada fica intocado), um '%' pendurado no final de fmt, a FRONTEIRA exata de truncamento
//     kLogMaxMessageBytes (N aceito verbatim vs N+1 truncado -- não um valor absurdamente grande,
//     conforme a própria lição desta sessão "ataque a fronteira, não o exagero"), substituição de
//     sink (2º registro silencia o 1º), reset pro default embutido com sink nullptr, e
//     reentrância de sink (um sink que instala um substituto E loga de novo de dentro do próprio
//     corpo).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/log.hpp>

#include <cstdio>
#include <string>
#include <vector>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

struct Captured {
  glintfx::LogLevel level;
  std::string message;
};

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: Level dispatch -- log_info/log_warn/log_error tag the sink with the corresponding
  //     LogLevel, and the printf-style formatting path substitutes correctly.
  // PT: Despacho de nível -- log_info/log_warn/log_error marcam o sink com o LogLevel
  //     correspondente, e o caminho de formatação estilo printf substitui corretamente.
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> got;
    set_log_sink([&](LogLevel level, const char* message) {
      got.push_back({level, message});
    });

    log_info("hello");
    log_warn("count=%d", 42);
    log_error("%s/%s", "a", "b");

    check(got.size() == 3, "level dispatch: 3 calls captured");
    if (got.size() == 3) {
      check(got[0].level == LogLevel::Info && got[0].message == "hello",
            "log_info: level Info, plain message verbatim");
      check(got[1].level == LogLevel::Warn && got[1].message == "count=42",
            "log_warn: level Warn, printf substitution applied");
      check(got[2].level == LogLevel::Error && got[2].message == "a/b",
            "log_error: level Error, multi-argument printf substitution applied");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: The PLAIN log() entry point never touches vsnprintf -- a literal '%' in the message
  //     round-trips verbatim, including a sequence that WOULD be a dangerous conversion
  //     (e.g. "%n") if it were ever handed to a formatter. This is the exact scenario draw2d.cpp/
  //     system_clock.hpp/system_glfw_dedup.hpp rely on (a filesystem path or a GL error string
  //     may legitimately contain '%').
  // PT: O ponto de entrada PLANO log() nunca toca o vsnprintf -- um '%' literal na mensagem faz
  //     round-trip verbatim, inclusive uma sequência que SERIA uma conversão perigosa (ex.:
  //     "%n") se algum dia fosse entregue a um formatador. Este é o cenário exato de que
  //     draw2d.cpp/system_clock.hpp/system_glfw_dedup.hpp dependem (um caminho de filesystem ou
  //     uma string de erro GL pode legitimamente conter '%').
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> got;
    set_log_sink([&](LogLevel level, const char* message) {
      got.push_back({level, message});
    });

    log(LogLevel::Warn, "discount: 50% off, format spec %n stays literal");
    check(got.size() == 1, "log(): exactly 1 call captured");
    if (got.size() == 1) {
      check(got[0].message == "discount: 50% off, format spec %n stays literal",
            "log(): literal '%' (including a '%n'-shaped substring) round-trips unmodified");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: Hostile input -- nullptr message/fmt never crash and are treated as an empty string.
  // PT: Input hostil -- message/fmt nullptr nunca crasham e são tratados como string vazia.
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> got;
    set_log_sink([&](LogLevel level, const char* message) {
      got.push_back({level, message});
    });

    log(LogLevel::Info, nullptr);
    log_info(nullptr);

    check(got.size() == 2, "nullptr message/fmt: both calls still reach the sink");
    if (got.size() == 2) {
      check(got[0].message.empty(), "log(level, nullptr): captured message is empty, not a crash");
      check(got[1].message.empty(), "log_info(nullptr): captured message is empty, not a crash");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: %n hostile-format rejection -- the sink NEVER receives the caller's variadic argument
  //     interpreted against a %n conversion, AND (the load-bearing part of this check) the
  //     pointer that %n would have written through is provably untouched -- a weaker test that
  //     only inspects the captured string could pass even if some other code path let %n reach
  //     vsnprintf, so this test additionally seeds a canary value and asserts it survives.
  // PT: Rejeição de format hostil %n -- o sink NUNCA recebe o argumento variádico do chamador
  //     interpretado contra uma conversão %n, E (a parte que sustenta esta checagem) o ponteiro
  //     pelo qual %n teria escrito fica comprovadamente intocado -- um teste mais fraco que só
  //     inspecionasse a string capturada poderia passar mesmo que algum outro caminho deixasse
  //     %n chegar ao vsnprintf, então este teste também planta um valor-canário e verifica que
  //     ele sobrevive.
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> got;
    set_log_sink([&](LogLevel level, const char* message) {
      got.push_back({level, message});
    });

    volatile int canary = 0x5EED;
    // NOLINTNEXTLINE -- deliberately hostile %n format, this is the exact case under test.
    log_warn("%n", const_cast<int*>(&canary));

    check(got.size() == 1, "%n format: the call still reaches the sink (fail-high, not silently dropped)");
    check(canary == 0x5EED, "%n format: the target pointer is untouched -- %n never reached vsnprintf");
    if (got.size() == 1) {
      check(!got[0].message.empty(), "%n format: sink receives a non-empty fixed diagnostic message");
      check(got[0].message.find("%n") != std::string::npos,
            "%n format: the diagnostic message names the rejected conversion");
    }

    got.clear();
    // EN: A dangling '%' at the very end of fmt -- an incomplete conversion, also rejected.
    //     GLINTFX_PRINTF_LIKE (log.hpp) makes GCC/Clang flag this literal at COMPILE time
    //     (-Wformat "incomplete format specifier") -- exactly the attribute doing its job on a
    //     deliberately malformed literal; silenced locally since triggering it IS this test.
    // PT: Um '%' pendurado bem no final de fmt -- uma conversão incompleta, também rejeitada.
    //     O GLINTFX_PRINTF_LIKE (log.hpp) faz o GCC/Clang sinalizar este literal em tempo de
    //     COMPILAÇÃO (-Wformat "incomplete format specifier") -- exatamente o atributo fazendo o
    //     trabalho dele sobre um literal deliberadamente malformado; silenciado localmente já que
    //     disparar isso É este teste.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#endif
    log_warn("value: %");
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    check(got.size() == 1, "dangling '%': the call still reaches the sink");
    if (got.size() == 1) {
      check(!got[0].message.empty(), "dangling '%': sink receives a non-empty fixed diagnostic message");
    }

    got.clear();
    // EN: "%%" (an escaped literal percent) must NOT trip the %n/dangling-% rejection.
    // PT: "%%" (um percent literal escapado) NÃO deve disparar a rejeição de %n/%-pendurado.
    log_warn("100%% done");
    check(got.size() == 1, "escaped '%%': the call reaches the sink");
    if (got.size() == 1) {
      check(got[0].message == "100% done", "escaped '%%': formats to a literal '%', not rejected");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: kLogMaxMessageBytes truncation BOUNDARY -- exactly cap-1 (accepted verbatim, no marker)
  //     vs exactly cap (truncated, visible marker) -- the fronteira itself, not an absurdly large
  //     value (an oversized input would be rejected by ANY reasonable cap and would not prove the
  //     boundary is where this test claims it is).
  // PT: FRONTEIRA de truncamento kLogMaxMessageBytes -- exatamente cap-1 (aceito verbatim, sem
  //     marcador) vs exatamente cap (truncado, marcador visível) -- a fronteira em si, não um
  //     valor absurdamente grande (um input descomunal seria rejeitado por QUALQUER teto
  //     razoável e não provaria que a fronteira está onde este teste afirma).
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> got;
    set_log_sink([&](LogLevel level, const char* message) {
      got.push_back({level, message});
    });

    // EN: vsnprintf's returned "would-be length" for "%s" with an N-char argument is exactly N;
    //     truncation triggers when that length >= kLogMaxMessageBytes (the buffer capacity,
    //     INCLUDING the NUL -- see log.hpp's kLogMaxMessageBytes doc-comment). So a (cap - 1)-char
    //     argument is the largest that still fits with room for the NUL; a cap-char argument is
    //     the smallest that overflows by exactly one byte.
    // PT: O "tamanho-que-seria" retornado pelo vsnprintf para "%s" com um argumento de N chars é
    //     exatamente N; o truncamento dispara quando esse tamanho >= kLogMaxMessageBytes (a
    //     capacidade do buffer, INCLUINDO o NUL -- ver o doc-comment de kLogMaxMessageBytes do
    //     log.hpp). Então um argumento de (cap - 1) chars é o maior que ainda cabe com espaço pro
    //     NUL; um argumento de cap chars é o menor que estoura por exatamente um byte.
    const std::string fits(kLogMaxMessageBytes - 1, 'a');
    const std::string overflows_by_one(kLogMaxMessageBytes, 'b');

    log_info("%s", fits.c_str());
    check(got.size() == 1, "boundary N: the call reaches the sink");
    if (got.size() == 1) {
      check(got[0].message == fits,
            "boundary N (cap - 1 chars): accepted verbatim, no truncation marker");
    }

    got.clear();
    log_info("%s", overflows_by_one.c_str());
    check(got.size() == 1, "boundary N+1: the call reaches the sink");
    if (got.size() == 1) {
      check(got[0].message.size() == kLogMaxMessageBytes - 1,
            "boundary N+1 (cap chars): truncated to exactly cap - 1 bytes, never overflowing the buffer");
      check(got[0].message != overflows_by_one,
            "boundary N+1: the captured message differs from the original (proves truncation happened)");
      check(got[0].message.find("...[truncated]") != std::string::npos,
            "boundary N+1: the visible truncation marker is present");
    }
  }

  // ---------------------------------------------------------------------------
  // EN: Sink replacement -- a 2nd set_log_sink() call fully replaces the 1st (which is never
  //     invoked again), and set_log_sink(nullptr) resets to the built-in default (observed here
  //     as "our capture vector stops growing", since the default writes to real stderr, not to
  //     this test's vector).
  // PT: Substituição de sink -- uma 2ª chamada set_log_sink() substitui totalmente a 1ª (que
  //     nunca mais é invocada), e set_log_sink(nullptr) reseta pro default embutido (observado
  //     aqui como "nosso vetor de captura para de crescer", já que o default escreve no stderr
  //     real, não neste vetor do teste).
  // ---------------------------------------------------------------------------
  {
    std::vector<int> hits_a, hits_b;
    set_log_sink([&](LogLevel, const char*) { hits_a.push_back(1); });
    log_info("to A");
    set_log_sink([&](LogLevel, const char*) { hits_b.push_back(1); });
    log_info("to B, not A");

    check(hits_a.size() == 1, "sink A: received exactly the 1 call before replacement");
    check(hits_b.size() == 1, "sink B: received exactly the 1 call after replacement");

    set_log_sink(nullptr);
    log_info("to the default, neither A nor B");
    check(hits_a.size() == 1, "after nullptr reset: sink A still at 1 (never called again)");
    check(hits_b.size() == 1, "after nullptr reset: sink B still at 1 (not called for the post-reset log)");
  }

  // ---------------------------------------------------------------------------
  // EN: Reentrancy -- a sink that (1) installs a replacement sink AND (2) logs again from inside
  //     its own body must not crash/deadlock/corrupt state. The in-flight call still completes
  //     against the copy it was invoked with; the NESTED log call made from inside the sink body
  //     reads whatever is CURRENTLY installed at that point (the replacement, since it was
  //     installed just before the nested call).
  // PT: Reentrância -- um sink que (1) instala um sink substituto E (2) loga de novo de dentro do
  //     próprio corpo não pode crashar/travar/corromper estado. A chamada em voo ainda completa
  //     contra a cópia com que foi invocada; a chamada de log ANINHADA feita de dentro do corpo
  //     do sink lê o que estiver CORRENTEMENTE instalado naquele ponto (o substituto, já que foi
  //     instalado logo antes da chamada aninhada).
  // ---------------------------------------------------------------------------
  {
    std::vector<Captured> outer_hits, replacement_hits;
    bool reentered = false;

    set_log_sink([&](LogLevel level, const char* message) {
      outer_hits.push_back({level, message});
      if (!reentered) {
        reentered = true;
        set_log_sink([&](LogLevel lvl2, const char* msg2) {
          replacement_hits.push_back({lvl2, msg2});
        });
        log_info("nested, from inside the sink");
      }
    });

    log_info("outer");

    check(outer_hits.size() == 1, "reentrancy: the outer sink saw exactly its own 1 call");
    if (outer_hits.size() == 1) {
      check(outer_hits[0].message == "outer", "reentrancy: outer sink got the outer message");
    }
    check(replacement_hits.size() == 1,
          "reentrancy: the nested log_info() call was routed to the JUST-INSTALLED replacement sink");
    if (replacement_hits.size() == 1) {
      check(replacement_hits[0].message == "nested, from inside the sink",
            "reentrancy: replacement sink got the nested message");
    }

    // EN: The replacement is still installed after the outer call returns.
    // PT: O substituto continua instalado depois da chamada externa retornar.
    log_info("after, should reach the replacement too");
    check(replacement_hits.size() == 2,
          "reentrancy: the replacement sink remains installed after the outer call returns");
  }

  // ---------------------------------------------------------------------------
  // EN: Restore the built-in default before exiting -- good citizenship for any test process that
  //     might run after this one in the same ctest invocation.
  // PT: Restaura o default embutido antes de sair -- boa cidadania para qualquer processo de
  //     teste que rode depois deste na mesma invocação de ctest.
  // ---------------------------------------------------------------------------
  set_log_sink(nullptr);

  if (g_failures > 0) {
    std::fprintf(stderr, "log_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("log_sanity: PASS");
  return 0;
}
