// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::I18n's RESOLUTION ORDER (i18n.hpp doc-comment, steps 1-5) --
//     no GL, no window, no RmlUi, no Xvfb (see tests/CMakeLists.txt for why this file lives
//     outside the GLINTFX_BACKEND_GLFW/Xvfb block). Each step of the chain gets its own,
//     isolated assertion so a regression that collapses two steps into one (e.g. "region
//     fallback" silently reusing the "explicit fallback_locale" code path) shows up as a
//     specific failing check, not a vague "something broke". Also covers runtime locale
//     switching (T1 in the ADR-0015 i18n plan: "troca de idioma em runtime, sem reiniciar o
//     jogo" -- the whole point of i18n-1's highest-ROI slice) and the null-key hardening.
// PT: Teste unit puro para a ORDEM DE RESOLUÇÃO do glintfx::I18n (comentário de cabeçalho de
//     i18n.hpp, passos 1-5) -- sem GL, sem janela, sem RmlUi, sem Xvfb (ver tests/CMakeLists.txt
//     pro porquê deste arquivo viver fora do bloco GLINTFX_BACKEND_GLFW/Xvfb). Cada passo da
//     cadeia ganha sua própria asserção isolada pra que uma regressão que colapse dois passos
//     num só (ex.: "fallback de região" reusando silenciosamente o caminho de código do
//     "fallback_locale explícito") apareça como uma checagem específica falhando, não um "algo
//     quebrou" vago. Também cobre a troca de locale em runtime (T1 no plano de i18n da
//     ADR-0015: "troca de idioma em runtime, sem reiniciar o jogo" -- o ponto inteiro da fatia
//     de maior ROI do i18n-1) e o hardening de chave nula.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/i18n.hpp>

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
  // ---------------------------------------------------------------------------
  // EN: Fixture catalog for every case below -- three locale sections: the generic base
  //     languages "en"/"pt" (no region), and the region-tagged "pt-BR" that deliberately does
  //     NOT define "only.in.pt" so step 2 (region -> base-language) has something real to prove.
  // PT: Catálogo de fixture pra todo caso abaixo -- três seções de locale: os idiomas-base
  //     genéricos "en"/"pt" (sem região), e o "pt-BR" com tag de região que deliberadamente NÃO
  //     define "only.in.pt" pra que o passo 2 (região -> idioma-base) tenha algo real pra provar.
  // ---------------------------------------------------------------------------
  const char* fixture =
      "[en]\n"
      "greeting = Hello\n"
      "only.in.en = English-only string\n"
      "\n"
      "[pt]\n"
      "greeting = Ola\n"
      "only.in.pt = String so em portugues\n"
      "\n"
      "[pt-BR]\n"
      "greeting = Oi\n";
      // EN: pt-BR intentionally has NO "only.in.pt" key -- must fall back to the "pt" section.
      // PT: pt-BR intencionalmente NÃO tem chave "only.in.pt" -- precisa cair pra seção "pt".

  // ---------------------------------------------------------------------------
  // EN: Step 1 -- exact active-locale hit.
  // PT: Passo 1 -- acerto exato do locale ativo.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("pt-BR");
    check(i18n.tr("greeting") == "Oi", "step 1: exact active-locale (pt-BR) hit");
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 -- active locale "pt-BR" misses "only.in.pt"; falls back to its base language
  //     "pt", where the key IS defined. No fallback_locale is set at all in this case, proving
  //     step 2 is independent of step 3/4.
  // PT: Passo 2 -- locale ativo "pt-BR" não acha "only.in.pt"; cai pro idioma-base dele "pt",
  //     onde a chave ESTÁ definida. Nenhum fallback_locale é definido neste caso, provando que o
  //     passo 2 é independente do passo 3/4.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("pt-BR");
    check(i18n.tr("only.in.pt") == "String so em portugues", "step 2: region (pt-BR) -> base-language (pt) fallback");
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 -- active locale is one with NO catalog data at all ("fr", never loaded); the
  //     explicit fallback_locale ("en") is consulted and resolves the key.
  // PT: Passo 3 -- locale ativo é um sem NENHUM dado de catálogo ("fr", nunca carregado); o
  //     fallback_locale explícito ("en") é consultado e resolve a chave.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("fr");
    check(i18n.set_fallback_locale("en"), "set_fallback_locale(\"en\") returns true");
    check(i18n.tr("only.in.en") == "English-only string", "step 3: explicit fallback_locale (en) resolves when active locale has no data");
  }

  // ---------------------------------------------------------------------------
  // EN: Step 4 -- active locale unloaded ("fr"), fallback_locale is region-tagged ("pt-BR",
  //     which itself misses "only.in.pt"); resolution must walk to the FALLBACK locale's base
  //     language ("pt") to find it -- proves step 4 is reached, not just step 3.
  // PT: Passo 4 -- locale ativo descarregado ("fr"), fallback_locale tem tag de região
  //     ("pt-BR", que por si só não acha "only.in.pt"); a resolução precisa andar até o
  //     idioma-base do locale DE FALLBACK ("pt") pra achar -- prova que o passo 4 é alcançado,
  //     não só o passo 3.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("fr");
    i18n.set_fallback_locale("pt-BR");
    check(i18n.tr("only.in.pt") == "String so em portugues", "step 4: fallback locale's own base-language fallback (pt-BR -> pt)");
  }

  // ---------------------------------------------------------------------------
  // EN: Step 5 -- nothing resolves anywhere (unknown locale, no fallback, unknown key); the key
  //     itself comes back, verbatim, never an empty string or a crash.
  // PT: Passo 5 -- nada resolve em lugar nenhum (locale desconhecido, sem fallback, chave
  //     desconhecida); a própria chave volta, ao pé da letra, nunca uma string vazia ou um crash.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("de"); // EN: never loaded, no fallback set. PT: nunca carregado, sem fallback definido.
    check(i18n.tr("totally.unknown.key") == "totally.unknown.key", "step 5: bare key returned verbatim as last resort");
  }

  // ---------------------------------------------------------------------------
  // EN: Case-insensitive locale matching -- "PT-br" (mixed case) must resolve against the
  //     "pt-BR" section exactly like the canonical casing does.
  // PT: Comparação de locale sem diferenciar maiúsculas/minúsculas -- "PT-br" (caixa mista)
  //     precisa resolver contra a seção "pt-BR" exatamente como a grafia canônica resolve.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("PT-br");
    check(i18n.tr("greeting") == "Oi", "case-insensitive locale match (\"PT-br\" resolves the \"pt-BR\" section)");
  }

  // ---------------------------------------------------------------------------
  // EN: Runtime locale switch (T1) -- the SAME I18n instance, the SAME key, re-resolved
  //     differently after each set_locale() call -- no reload/recreate needed, matching the
  //     UX-level "swap active language mid-game" contract i18n-1 exists to deliver.
  // PT: Troca de locale em runtime (T1) -- a MESMA instância I18n, a MESMA chave, re-resolvida
  //     de forma diferente após cada chamada set_locale() -- sem precisar recarregar/recriar,
  //     batendo com o contrato de nível de UX "trocar idioma ativo no meio do jogo" que o
  //     i18n-1 existe para entregar.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("en");
    check(i18n.tr("greeting") == "Hello", "runtime switch: greeting under en");
    i18n.set_locale("pt");
    check(i18n.tr("greeting") == "Ola", "runtime switch: greeting under pt (same instance, same key, no reload)");
    i18n.set_locale("pt-BR");
    check(i18n.tr("greeting") == "Oi", "runtime switch: greeting under pt-BR");
    check(i18n.locale() == "pt-BR", "locale() reflects the last set_locale() call, verbatim casing");
  }

  // ---------------------------------------------------------------------------
  // EN: locales() lists every section seen across all loads, sorted case-insensitively, each in
  //     its first-seen display casing.
  // PT: locales() lista toda seção vista em todas as cargas, ordenada sem diferenciar
  //     maiúsculas/minúsculas, cada uma na grafia de exibição da 1ª vez que foi vista.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    std::vector<std::string> locales = i18n.locales();
    check(locales.size() == 3, "locales(): 3 sections loaded");
    check(locales[0] == "en" && locales[1] == "pt" && locales[2] == "pt-BR", "locales(): sorted case-insensitively, original casing preserved");
  }

  // ---------------------------------------------------------------------------
  // EN: Hardening -- nullptr key/locale never crash and degrade to the documented safe default
  //     (tr(nullptr) == "", set_locale(nullptr)/set_locale("") == false with no state change).
  // PT: Hardening -- chave/locale nulos nunca crasham e degradam pro default seguro documentado
  //     (tr(nullptr) == "", set_locale(nullptr)/set_locale("") == false sem mudança de estado).
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string(fixture);
    i18n.set_locale("en");
    check(i18n.tr(nullptr).empty(), "tr(nullptr) == \"\"");
    check(i18n.tr_plural(nullptr, 5).empty(), "tr_plural(nullptr, 5) == \"\"");
    check(!i18n.set_locale(nullptr), "set_locale(nullptr) returns false");
    check(!i18n.set_locale(""), "set_locale(\"\") returns false");
    check(i18n.locale() == "en", "locale() unchanged after the two rejected set_locale() calls above");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "i18n_lookup_fallback_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("i18n_lookup_fallback_sanity: PASS");
  return 0;
}
