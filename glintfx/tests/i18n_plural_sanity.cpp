// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::I18n's CLDR-lite plural category selection (i18n-1,
//     framework-2D) -- no GL, no window, no RmlUi, no Xvfb (see tests/CMakeLists.txt). Asserts
//     by COUNT, not just presence, mirroring the "fórmula de conversão sem teste de delta" bug
//     class the glfw_event_translate_sanity.cpp header comment names: en and pt DIVERGE at
//     exactly n=0 (en: "other" / pt: "one"), and that single boundary value is the one most
//     likely to silently regress if the two rules are ever collapsed back into "n==1 ? one :
//     other" for every locale. See i18n.cpp's own header comment for the cited CLDR v48 rule
//     text this table implements.
//     Mutation-testability note for the adversarial reviewer: flipping the "an == 0 ||" clause
//     out of plural_category_for()'s "pt" branch in i18n.cpp, or dropping the '#' substitution
//     loop in tr_plural(), MUST turn one of the assertions below red.
// PT: Teste unit puro para a seleção de categoria de plural CLDR-lite do glintfx::I18n
//     (i18n-1, framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver
//     tests/CMakeLists.txt). Verifica por CONTAGEM, não só presença, espelhando a classe de bug
//     "fórmula de conversão sem teste de delta" que o comentário de cabeçalho de
//     glfw_event_translate_sanity.cpp nomeia: en e pt DIVERGEM exatamente em n=0 (en: "other" /
//     pt: "one"), e esse único valor de fronteira é o mais provável de regredir silenciosamente
//     se as duas regras algum dia forem colapsadas de volta pra "n==1 ? one : other" pra todo
//     locale. Ver o próprio comentário de cabeçalho de i18n.cpp pro texto de regra CLDR v48
//     citado que esta tabela implementa.
//     Nota de mutation-testability pro reviewer adversarial: tirar a cláusula "an == 0 ||" do
//     ramo "pt" de plural_category_for() em i18n.cpp, ou remover o laço de substituição de '#'
//     em tr_plural(), TEM que deixar vermelha alguma das asserções abaixo.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/i18n.hpp>

#include <climits>
#include <cstdint>
#include <cstdio>

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
  const char* fixture =
      "[en-US]\n"
      "hud.lives.one = # life\n"
      "hud.lives.other = # lives\n"
      "\n"
      "[pt-BR]\n"
      "hud.lives.one = # vida\n"
      "hud.lives.other = # vidas\n";

  I18n i18n;
  I18nLoadResult r = i18n.load_catalog_string(fixture);
  check(r.ok, "fixture catalog loads");

  // ---------------------------------------------------------------------------
  // EN: en -- CLDR v48 "one: i = 1 and v = 0": ONLY n=1 selects "one"; every other tested value,
  //     INCLUDING n=0, selects "other". This is the exact boundary that diverges from pt below.
  // PT: en -- CLDR v48 "one: i = 1 and v = 0": SÓ n=1 seleciona "one"; todo outro valor testado,
  //     INCLUSIVE n=0, seleciona "other". Esta é a fronteira exata que diverge do pt abaixo.
  // ---------------------------------------------------------------------------
  {
    i18n.set_locale("en-US");
    check(i18n.tr_plural("hud.lives", 0) == "0 lives", "en: n=0 -> other (\"0 lives\")");
    check(i18n.tr_plural("hud.lives", 1) == "1 life", "en: n=1 -> one (\"1 life\")");
    check(i18n.tr_plural("hud.lives", 2) == "2 lives", "en: n=2 -> other (\"2 lives\")");
    check(i18n.tr_plural("hud.lives", 16) == "16 lives", "en: n=16 -> other (\"16 lives\")");
    check(i18n.tr_plural("hud.lives", 100) == "100 lives", "en: n=100 -> other (\"100 lives\")");
  }

  // ---------------------------------------------------------------------------
  // EN: pt -- CLDR v48 "one: i = 0..1": BOTH n=0 and n=1 select "one"; n>=2 selects "other".
  //     Category selected from the active locale's BASE LANGUAGE ("pt-BR" -> "pt"), per
  //     i18n.hpp's documented contract (tr_plural always uses base_language(active_locale)).
  // PT: pt -- CLDR v48 "one: i = 0..1": TANTO n=0 QUANTO n=1 selecionam "one"; n>=2 seleciona
  //     "other". Categoria selecionada a partir do IDIOMA-BASE do locale ativo ("pt-BR" -> "pt"),
  //     conforme o contrato documentado em i18n.hpp (tr_plural sempre usa
  //     base_language(active_locale)).
  // ---------------------------------------------------------------------------
  {
    i18n.set_locale("pt-BR");
    check(i18n.tr_plural("hud.lives", 0) == "0 vida", "pt: n=0 -> one (\"0 vida\", singular text -- the actual divergence from en's \"0 lives\" above)");
    check(i18n.tr_plural("hud.lives", 1) == "1 vida", "pt: n=1 -> one (\"1 vida\")");
    check(i18n.tr_plural("hud.lives", 2) == "2 vidas", "pt: n=2 -> other (\"2 vidas\")");
    check(i18n.tr_plural("hud.lives", 17) == "17 vidas", "pt: n=17 -> other (\"17 vidas\")");
  }

  // ---------------------------------------------------------------------------
  // EN: '#' count-placeholder substitution -- multiple '#' occurrences in one value all get
  //     replaced, and a negative n keeps its sign in the substituted digits.
  // PT: substituição do placeholder de contagem '#' -- múltiplas ocorrências de '#' num mesmo
  //     valor são todas substituídas, e um n negativo mantém o sinal nos dígitos substituídos.
  // ---------------------------------------------------------------------------
  {
    I18n i2;
    i2.load_catalog_string("[en]\nscore.one = # point (#)\nscore.other = # points (#)\n");
    i2.set_locale("en");
    check(i2.tr_plural("score", 1) == "1 point (1)", "multiple '#' occurrences all substituted (n=1, one)");
    check(i2.tr_plural("score", 3) == "3 points (3)", "multiple '#' occurrences all substituted (n=3, other)");
    check(i2.tr_plural("score", -5) == "-5 points (-5)", "negative n keeps its sign in the substituted digits");
  }

  // ---------------------------------------------------------------------------
  // EN: Missing ".one" falls back to ".other" WITHIN the same locale step (RESOLUTION ORDER,
  //     i18n.hpp) rather than skipping straight to the next locale step or the bare-key last
  //     resort -- a catalog author may legitimately omit ".one" for a concept with no natural
  //     singular (e.g. "seconds remaining").
  // PT: ".one" ausente cai pra ".other" DENTRO do mesmo passo de locale (ORDEM DE RESOLUÇÃO,
  //     i18n.hpp) em vez de pular direto pro próximo passo de locale ou pro último recurso da
  //     chave crua -- um autor de catálogo pode legitimamente omitir ".one" pra um conceito sem
  //     singular natural (ex.: "segundos restantes").
  // ---------------------------------------------------------------------------
  {
    I18n i3;
    i3.load_catalog_string("[en]\ntimer.other = # seconds remaining\n"); // EN: no .one at all. PT: sem .one nenhum.
    i3.set_locale("en");
    check(i3.tr_plural("timer", 1) == "1 seconds remaining", "n=1 selects category one, but .one is absent -> falls back to .other WITHIN the en step");
    check(i3.tr_plural("timer", 5) == "5 seconds remaining", "n=5 selects category other directly");
  }

  // ---------------------------------------------------------------------------
  // EN: Neither ".one" nor ".other" resolves anywhere -- last resort is the BARE key (no
  //     suffix, no substitution), per i18n.hpp's documented contract.
  // PT: Nem ".one" nem ".other" resolvem em lugar nenhum -- último recurso é a chave CRUA
  //     (sem sufixo, sem substituição), conforme o contrato documentado em i18n.hpp.
  // ---------------------------------------------------------------------------
  {
    I18n i4;
    i4.set_locale("en"); // EN: nothing loaded at all. PT: nada carregado.
    check(i4.tr_plural("nowhere.defined", 3) == "nowhere.defined", "last resort: bare key, unmodified, no '#' substitution attempted");
  }

  // ---------------------------------------------------------------------------
  // EN: LLONG_MIN/INT64_MIN hardening (review adversarial finding, i18n-1) -- unary `-` on
  //     LLONG_MIN is signed-integer-overflow undefined behaviour (there is no positive `long
  //     long` that represents its magnitude), confirmed live by UBSan before the fix:
  //     "negation of -9223372036854775808 cannot be represented in type 'long long'". This must
  //     NOT crash/UB and must resolve to a real category ("other" for both en and pt, since
  //     |LLONG_MIN| is nowhere near 0 or 1). Mutation-testability note: reverting
  //     absolute_value_of() in i18n.cpp back to a plain `n < 0 ? -n : n` ternary must turn this
  //     UBSan-clean under `-fsanitize=undefined` (see AGENT_REPORT.md/PR for the before/after
  //     UBSan transcript -- this assertion alone cannot observe UB that does not also change the
  //     result, so the sanitizer run is the real proof, this is the behavioural companion).
  // PT: Hardening de LLONG_MIN/INT64_MIN (achado do review adversarial, i18n-1) -- `-` unário em
  //     LLONG_MIN é overflow de inteiro com sinal, comportamento indefinido (não existe `long
  //     long` positivo que represente a magnitude dele), confirmado ao vivo pelo UBSan antes do
  //     fix: "negation of -9223372036854775808 cannot be represented in type 'long long'". Isso
  //     NÃO pode crashar/UB e precisa resolver pra uma categoria real ("other" tanto pra en
  //     quanto pra pt, já que |LLONG_MIN| está longe de 0 ou 1). Nota de mutation-testability:
  //     reverter absolute_value_of() em i18n.cpp de volta pro ternário simples `n < 0 ? -n : n`
  //     precisa deixar isso UBSan-sujo sob `-fsanitize=undefined` (ver o transcript UBSan antes/
  //     depois no relatório -- esta asserção sozinha não observa UB que não muda o resultado, a
  //     execução sob sanitizer é a prova real, isto é a companhia comportamental).
  // ---------------------------------------------------------------------------
  {
    I18n i5;
    i5.load_catalog_string(
        "[en]\nhud.lives.one = # life\nhud.lives.other = # lives\n"
        "[pt]\nhud.lives.one = # vida\nhud.lives.other = # vidas\n");

    i5.set_locale("en");
    check(i5.tr_plural("hud.lives", LLONG_MIN) == "-9223372036854775808 lives",
          "en: n=LLONG_MIN does not UB, resolves to \"other\"");
    check(i5.tr_plural("hud.lives", INT64_MIN) == "-9223372036854775808 lives",
          "en: n=INT64_MIN does not UB, resolves to \"other\"");

    i5.set_locale("pt");
    check(i5.tr_plural("hud.lives", LLONG_MIN) == "-9223372036854775808 vidas",
          "pt: n=LLONG_MIN does not UB, resolves to \"other\"");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "i18n_plural_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("i18n_plural_sanity: PASS");
  return 0;
}
