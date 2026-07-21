// SPDX-License-Identifier: MPL-2.0
// EN: i18n-1 (framework-2D, ADR-0015 module "i18n") -- translation catalog, key lookup with
//     fallback, and CLDR-lite plural selection. This is the WHOLE public surface of the module:
//     no third-party type (RmlUi/GLFW/GL/stb/SDL) appears here, and the class depends on the
//     C++ standard library ONLY (ADR-0015 (b): "i18n depends on core ONLY" -- no GL, no window,
//     no RmlUi type crossing this header, testable without Xvfb, same independence class as the
//     audio atom). Feeding a translated string INTO the UI is the consumer's job via the
//     data-binding surface that already exists (`create_data_model`/`bind_string`/`set_string`,
//     docs/embed-integration.md section 6) -- this module never includes anything from `ui`.
//
//     CATALOG FORMAT (own, zero-dep -- decision documented in
//     docs/superpowers/plans/2026-07-19-framework2d-i18n-module.md section 7 item 2): a small,
//     human-editable, UTF-8 text format, NOT JSON/YAML/any third-party parser. Grammar:
//
//       # a comment line -- '#' must be the FIRST non-whitespace character of the line.
//       [locale]                 -- opens a section; every key=value line below it belongs to
//                                    this locale, until the next [locale] header or EOF. Locale
//                                    codes are matched case-insensitively (ADR-0015 anti-OE
//                                    principle #3: locale != language -- always write the FULL
//                                    tag, e.g. "pt-BR"/"en-US", never a bare "pt"/"en" section
//                                    UNLESS you deliberately want it to serve as every region's
//                                    base-language fallback -- see RESOLUTION ORDER below).
//       key.with.dots = value    -- a translation entry. Leading/trailing whitespace around
//                                    both key and value is trimmed. Blank lines are ignored.
//       key.one = # singular     -- plural sub-keys (ICU-lite convention, see PLURALS below).
//       key.other = # plural
//
//     A line that is not blank, not a comment, not a `[locale]` header, and not a well-formed
//     `key = value` pair (no '=', or an empty key) is MALFORMED: it is skipped and counted in
//     I18nLoadResult::lines_skipped -- load_catalog_* NEVER aborts or throws on malformed input
//     (fail-high on the LINE, not on the whole load; hostile/truncated/garbled catalog files are
//     a robustness requirement, not an edge case -- same discipline as decorator_polygon.cpp's
//     "reject the one bad decorator, never abort the host"). A key=value line seen before any
//     `[locale]` header has nowhere to go and is ALSO skipped+counted (not silently dropped
//     into some implicit locale).
//
//     ESCAPES (minimal, by design -- ADR-0015 anti-OE #1, do not grow this past real need):
//     inside a value, `\\` -> `\`, `\n` -> newline, `\t` -> tab. Any other backslash sequence is
//     left as-is (both characters literal) -- a stray/unknown escape is never a parse error.
//
//     PLURALS (a subset of ICU MessageFormat's `plural`, NOT the full ICU grammar -- see
//     the vision doc's anti-over-engineering section 3.3, "not the whole CLDR"): a pluralizable
//     concept is stored as TWO catalog entries under a shared base key, `<key>.one` and
//     `<key>.other` (ICU-lite category names; this module supports exactly these two, BY SCOPE
//     CHOICE, not because the shipped locales lack the others -- en genuinely has only one/other,
//     but pt DOES define a `many` category in CLDR v48 (exact multiples of 1,000,000, e.g.
//     n=1000000), which this module deliberately does not implement: a HUD showing "1,000,000
//     lives" is not a real game scenario (ADR-0015 anti-OE principle #1), so `many` collapses
//     into `other` here by design, not by omission. `few`/`zero`/`two` genuinely do not apply to
//     either en or pt in CLDR v48, cited in i18n.cpp). Inside a `.one`/`.other` value, every
//     literal '#' character is the COUNT PLACEHOLDER (ICU convention) -- tr_plural() replaces
//     every '#' with the decimal digits of the count passed in. Because of this, a `.one`/
//     `.other` value cannot contain a literal '#' as ordinary text; use tr() for strings that
//     need one.
//
//     Example catalog (two locales, one plain key, one pluralized key):
//
//       # HUD strings.
//       [en-US]
//       hud.title = My Game
//       hud.lives.one = # life
//       hud.lives.other = # lives
//
//       [pt-BR]
//       hud.title = Meu Jogo
//       hud.lives.one = # vida
//       hud.lives.other = # vidas
//
//     RESOLUTION ORDER (tr()/tr_plural(), never crashes, never returns an empty string for a
//     non-null key): (1) the exact active locale; (2) the active locale's base language (the
//     part before '-', e.g. "pt-BR" -> "pt") IF the active locale itself has a region tag;
//     (3) the exact fallback locale, if one was set via set_fallback_locale(); (4) the fallback
//     locale's base language, under the same condition as (2); (5) the bare key itself, verbatim
//     -- the last-resort signal that a translation is missing, deliberately visible in the
//     rendered UI rather than silently blank. tr_plural() additionally falls back from the
//     `.one` form to the `.other` form within step (1)-(4) before advancing to the next step,
//     because "other" is the CLDR-guaranteed catch-all category every locale defines.
//
// PT: i18n-1 (framework-2D, módulo "i18n" da ADR-0015) -- catálogo de tradução, lookup de chave
//     com fallback, e seleção de plural CLDR-lite. Esta é a superfície pública INTEIRA do
//     módulo: nenhum tipo de terceiro (RmlUi/GLFW/GL/stb/SDL) aparece aqui, e a classe depende
//     SÓ da biblioteca padrão C++ (ADR-0015 (b): "i18n depende SÓ de core" -- zero GL, zero
//     janela, zero tipo do RmlUi cruzando este header, testável sem Xvfb, mesma classe de
//     independência do átomo audio). Alimentar a string traduzida NA UI é trabalho do
//     consumidor via a superfície de data-binding que já existe (`create_data_model`/
//     `bind_string`/`set_string`, docs/embed-integration.md seção 6) -- este módulo nunca
//     inclui nada da `ui`.
//
//     FORMATO DO CATÁLOGO (próprio, zero-dep -- decisão documentada em
//     docs/superpowers/plans/2026-07-19-framework2d-i18n-module.md seção 7 item 2): um formato
//     texto pequeno, editável a mão, UTF-8, NÃO JSON/YAML/nenhum parser de terceiro. Gramática:
//
//       # linha de comentário -- '#' precisa ser o PRIMEIRO caractere não-espaço da linha.
//       [locale]                 -- abre uma seção; toda linha key=value abaixo dela pertence a
//                                    este locale, até o próximo cabeçalho [locale] ou EOF.
//                                    Códigos de locale são comparados sem diferenciar
//                                    maiúsculas/minúsculas (princípio anti-OE #3 da ADR-0015:
//                                    locale != idioma -- sempre escreva a tag COMPLETA, ex.:
//                                    "pt-BR"/"en-US", nunca um "pt"/"en" cru A MENOS que você
//                                    deliberadamente queira que sirva de fallback de idioma-base
//                                    para toda região -- ver ORDEM DE RESOLUÇÃO abaixo).
//       chave.com.pontos = valor -- uma entrada de tradução. Espaço em branco antes/depois de
//                                    chave e valor é removido. Linhas em branco são ignoradas.
//       chave.one = # singular   -- sub-chaves de plural (convenção ICU-lite, ver PLURAIS abaixo).
//       chave.other = # plural
//
//     Uma linha que não é branca, não é comentário, não é cabeçalho `[locale]`, e não é um par
//     `key = value` bem formado (sem '=', ou chave vazia) é MALFORMADA: é pulada e contada em
//     I18nLoadResult::lines_skipped -- load_catalog_* NUNCA aborta nem lança exceção com input
//     malformado (fail-high na LINHA, não no load inteiro; arquivo de catálogo hostil/truncado/
//     corrompido é requisito de robustez, não caso de borda -- mesma disciplina do
//     decorator_polygon.cpp, "rejeita o decorator ruim, nunca aborta o host"). Uma linha
//     key=value vista antes de qualquer cabeçalho `[locale]` não tem onde ir e TAMBÉM é pulada
//     +contada (não cai silenciosamente num locale implícito).
//
//     ESCAPES (mínimos, de propósito -- princípio anti-OE #1 da ADR-0015, não crescer isto além
//     da necessidade real): dentro de um valor, `\\` -> `\`, `\n` -> nova linha, `\t` -> tab.
//     Qualquer outra sequência de barra invertida fica como está (os dois caracteres literais)
//     -- um escape perdido/desconhecido nunca é erro de parse.
//
//     PLURAIS (um subconjunto do `plural` do ICU MessageFormat, NÃO a gramática ICU completa --
//     ver a seção anti-over-engineering 3.3 do doc de visão, "não o CLDR inteiro"): um conceito
//     pluralizável é guardado como DUAS entradas de catálogo sob uma chave-base compartilhada,
//     `<chave>.one` e `<chave>.other` (nomes de categoria ICU-lite; este módulo suporta
//     exatamente estas duas, por ESCOLHA DE ESCOPO, não porque os locales shipados careçam das
//     outras -- o en de fato só tem one/other, mas o pt TEM uma categoria `many` no CLDR v48
//     (múltiplos exatos de 1.000.000, ex.: n=1000000), que este módulo deliberadamente não
//     implementa: um HUD mostrando "1.000.000 vidas" não é um cenário real de jogo (princípio
//     anti-OE #1 da ADR-0015), então `many` colapsa em `other` aqui por design, não por omissão.
//     `few`/`zero`/`two` de fato não se aplicam nem a en nem a pt no CLDR v48, citado em
//     i18n.cpp). Dentro de um valor `.one`/`.other`, todo caractere '#' literal é o PLACEHOLDER
//     DE CONTAGEM (convenção ICU) -- tr_plural() substitui todo '#' pelos dígitos decimais da
//     contagem passada. Por causa disso, um valor `.one`/`.other` não pode conter um '#' literal
//     como texto comum; use tr() para strings que precisem de um.
//
//     Exemplo de catálogo (dois locales, uma chave simples, uma chave pluralizada):
//
//       # Strings do HUD.
//       [en-US]
//       hud.title = My Game
//       hud.lives.one = # life
//       hud.lives.other = # lives
//
//       [pt-BR]
//       hud.title = Meu Jogo
//       hud.lives.one = # vida
//       hud.lives.other = # vidas
//
//     ORDEM DE RESOLUÇÃO (tr()/tr_plural(), nunca crasha, nunca retorna string vazia para uma
//     chave não-nula): (1) o locale ativo exato; (2) o idioma-base do locale ativo (a parte
//     antes do '-', ex.: "pt-BR" -> "pt") SE o locale ativo em si tiver tag de região;
//     (3) o locale de fallback exato, se um foi definido via set_fallback_locale(); (4) o
//     idioma-base do locale de fallback, sob a mesma condição de (2); (5) a própria chave, ao
//     pé da letra -- o sinal de último recurso de que falta uma tradução, deliberadamente
//     visível na UI renderizada em vez de silenciosamente em branco. tr_plural() adicionalmente
//     cai da forma `.one` para a forma `.other` dentro de cada passo (1)-(4) antes de avançar
//     pro próximo passo, porque "other" é a categoria catch-all garantida pelo CLDR que todo
//     locale define.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace glintfx {

// EN: Outcome of a load_catalog_*() call. `ok` is false ONLY on a fatal condition (null text,
//     unreadable/nonexistent file) -- a malformed LINE inside an otherwise-readable catalog is
//     never fatal, see lines_skipped. entries_loaded counts every key=value pair accepted
//     across every [locale] section in this one call (a key repeated within the same locale in
//     the same call overwrites and is still counted -- last-write-wins, documented, not an
//     error: it is the mechanism a consumer uses to load a base catalog then an override file).
// PT: Resultado de uma chamada a load_catalog_*(). `ok` é false SÓ numa condição fatal (texto
//     nulo, arquivo ilegível/inexistente) -- uma LINHA malformada dentro de um catálogo por
//     outro lado legível nunca é fatal, ver lines_skipped. entries_loaded conta todo par
//     key=value aceito em toda seção [locale] nesta chamada (uma chave repetida dentro do mesmo
//     locale na mesma chamada sobrescreve e ainda é contada -- último-escreve-vence, documentado,
//     não é erro: é o mecanismo que um consumidor usa pra carregar um catálogo base e depois um
//     arquivo de override).
struct I18nLoadResult {
  bool        ok             = false;
  std::size_t entries_loaded = 0;
  std::size_t lines_skipped  = 0;
};

// EN: Translation catalog + locale-aware lookup + CLDR-lite plural selection. See this file's
//     header comment above for the catalog format, escapes, plural convention, and resolution
//     order. No pImpl here (unlike DataBinder/UiLayer) -- there is no third-party type to hide
//     behind an opaque pointer, every member is standard-library data, so the extra heap
//     indirection would buy nothing; encapsulation is via `private:` alone.
// PT: Catálogo de tradução + lookup com consciência de locale + seleção de plural CLDR-lite.
//     Ver o comentário de cabeçalho deste arquivo acima para o formato do catálogo, escapes,
//     convenção de plural e ordem de resolução. Sem pImpl aqui (diferente de DataBinder/
//     UiLayer) -- não há tipo de terceiro pra esconder atrás de um ponteiro opaco, todo membro é
//     dado de biblioteca padrão, então a indireção extra de heap não compraria nada;
//     encapsulamento é só via `private:`.
class I18n {
public:
  I18n()  = default;
  ~I18n() = default;

  // EN: Load a catalog from an in-memory string (e.g. an asset already read by the host, or a
  //     literal embedded at compile time). May be called more than once -- entries merge into
  //     the existing catalog, last-write-wins per (locale, key) across calls, so a consumer can
  //     load a base catalog then a locale-specific override. `text == nullptr` returns
  //     `{ .ok = false }` without touching the catalog.
  // PT: Carrega um catálogo de uma string em memória (ex.: um asset já lido pelo host, ou um
  //     literal embutido em compile time). Pode ser chamada mais de uma vez -- entradas se
  //     fundem no catálogo existente, último-escreve-vence por (locale, chave) entre chamadas,
  //     então um consumidor pode carregar um catálogo base e depois um override específico de
  //     locale. `text == nullptr` retorna `{ .ok = false }` sem tocar no catálogo.
  I18nLoadResult load_catalog_string(const char* text);

  // EN: Load a catalog from a file path (UTF-8 text, read via std::ifstream -- glintfx is
  //     hosted C++23, not the freestanding Layer 0; the standard library is available and
  //     already used throughout this lib, e.g. DataBinder's std::map). Returns
  //     `{ .ok = false }` without touching the catalog when `path` is nullptr or the file
  //     cannot be opened for reading.
  // PT: Carrega um catálogo de um caminho de arquivo (texto UTF-8, lido via std::ifstream --
  //     glintfx é C++23 hosted, não a Camada 0 freestanding; a biblioteca padrão está
  //     disponível e já é usada em toda esta lib, ex.: o std::map do DataBinder). Retorna
  //     `{ .ok = false }` sem tocar no catálogo quando `path` é nulo ou o arquivo não pode ser
  //     aberto para leitura.
  I18nLoadResult load_catalog_file(const char* path);

  // EN: Sets the active locale used by tr()/tr_plural() (see RESOLUTION ORDER above). Matched
  //     case-insensitively against loaded [locale] sections at lookup time -- the exact string
  //     passed here is preserved verbatim for locale()/locales(). Returns false (no change) when
  //     `locale` is nullptr or empty; the locale need not already exist in a loaded catalog
  //     (resolution simply falls through to fallback/the bare key until a matching catalog is
  //     loaded, e.g. across a runtime language pack download).
  // PT: Define o locale ativo usado por tr()/tr_plural() (ver ORDEM DE RESOLUÇÃO acima).
  //     Comparado sem diferenciar maiúsculas/minúsculas contra as seções [locale] carregadas em
  //     tempo de lookup -- a string exata passada aqui é preservada ao pé da letra para
  //     locale()/locales(). Retorna false (sem mudança) quando `locale` é nulo ou vazio; o
  //     locale não precisa já existir num catálogo carregado (a resolução simplesmente cai para
  //     o fallback/a chave crua até um catálogo correspondente ser carregado, ex.: por um
  //     download de pacote de idioma em runtime).
  bool set_locale(const char* locale);

  // EN: The active locale string as last set via set_locale(), or "" if never set.
  // PT: A string do locale ativo tal como definida por set_locale(), ou "" se nunca definida.
  const std::string& locale() const;

  // EN: Sets an explicit fallback locale (step 3/4 of RESOLUTION ORDER above) -- typically the
  //     game's default/base language (e.g. "en-US"), consulted when the active locale's catalog
  //     is missing a key. Optional: if never set, resolution skips straight from step 2 to the
  //     bare-key last resort (step 5). Same validation/matching rules as set_locale().
  // PT: Define um locale de fallback explícito (passo 3/4 da ORDEM DE RESOLUÇÃO acima) --
  //     tipicamente o idioma padrão/base do jogo (ex.: "en-US"), consultado quando o catálogo do
  //     locale ativo não tem uma chave. Opcional: se nunca definido, a resolução pula direto do
  //     passo 2 pro último recurso da chave crua (passo 5). Mesmas regras de validação/
  //     comparação de set_locale().
  bool set_fallback_locale(const char* locale);

  // EN: The fallback locale string as last set via set_fallback_locale(), or "" if never set.
  // PT: A string do locale de fallback tal como definida por set_fallback_locale(), ou "" se
  //     nunca definida.
  const std::string& fallback_locale() const;

  // EN: Every locale for which at least one entry has been loaded (any load_catalog_* call, any
  //     number of times), each in the exact casing of its [locale] header the FIRST time it was
  //     seen, sorted case-insensitively. Empty if nothing has been loaded yet. Intended for a
  //     language picker / settings menu.
  // PT: Todo locale para o qual ao menos uma entrada foi carregada (qualquer chamada a
  //     load_catalog_*, quantas vezes for), cada um na grafia exata do cabeçalho [locale] na
  //     PRIMEIRA vez que foi visto, ordenado sem diferenciar maiúsculas/minúsculas. Vazio se
  //     nada foi carregado ainda. Pensado pra um seletor de idioma / menu de configurações.
  std::vector<std::string> locales() const;

  // EN: Resolves `key` against the active locale (RESOLUTION ORDER above), no plural/count
  //     handling and no placeholder substitution -- a bare string lookup. `key == nullptr`
  //     returns "". Never crashes on a missing key: falls all the way to returning the key
  //     itself, verbatim, as the last resort.
  // PT: Resolve `key` contra o locale ativo (ORDEM DE RESOLUÇÃO acima), sem tratamento de
  //     plural/contagem e sem substituição de placeholder -- um lookup de string crua.
  //     `key == nullptr` retorna "". Nunca crasha numa chave ausente: cai até retornar a
  //     própria chave, ao pé da letra, como último recurso.
  std::string tr(const char* key) const;

  // EN: Resolves the pluralized concept `key` for count `n`: selects the CLDR-lite category
  //     (One/Other, see this file's header comment) for the active locale's BASE language, looks
  //     up `<key>.one` or `<key>.other` via the same RESOLUTION ORDER as tr() (falling back to
  //     `.other` within each locale step before advancing, since "other" is CLDR's guaranteed
  //     catch-all), then replaces every '#' in the resolved value with the base-10 digits of
  //     `n` (sign included, e.g. n=-3 -> "-3"). `key == nullptr` returns "". Never crashes: if
  //     neither `.one` nor `.other` resolves anywhere in the chain, returns the bare `key`
  //     itself (no suffix, no substitution) as the last resort.
  // PT: Resolve o conceito pluralizável `key` para a contagem `n`: seleciona a categoria
  //     CLDR-lite (One/Other, ver o comentário de cabeçalho deste arquivo) para o idioma-BASE
  //     do locale ativo, busca `<key>.one` ou `<key>.other` pela mesma ORDEM DE RESOLUÇÃO do
  //     tr() (caindo para `.other` dentro de cada passo de locale antes de avançar, já que
  //     "other" é o catch-all garantido do CLDR), depois substitui todo '#' no valor resolvido
  //     pelos dígitos base-10 de `n` (sinal incluído, ex.: n=-3 -> "-3"). `key == nullptr`
  //     retorna "". Nunca crasha: se nem `.one` nem `.other` resolver em nenhum ponto da cadeia,
  //     retorna a própria `key` crua (sem sufixo, sem substituição) como último recurso.
  std::string tr_plural(const char* key, long long n) const;

private:
  // EN: locale (as-given casing, first-seen) -> key -> value. Looked up by a lowercased copy of
  //     the requested locale (see resolve_locale_section() in i18n.cpp) so matching stays
  //     case-insensitive without mutating the stored display casing.
  // PT: locale (grafia como dada, na 1ª vez vista) -> chave -> valor. Buscado por uma cópia em
  //     minúsculas do locale pedido (ver resolve_locale_section() em i18n.cpp) pra que a
  //     comparação fique sem diferenciar maiúsculas/minúsculas sem mutar a grafia de exibição
  //     guardada.
  struct LocaleSection {
    std::string                       display_name;
    std::map<std::string, std::string> entries;
  };
  std::map<std::string, LocaleSection> catalog_by_locale_lc_;

  std::string active_locale_;
  std::string fallback_locale_;

  // EN: Shared implementation of load_catalog_string()/load_catalog_file() once the text is in
  //     memory -- see i18n.cpp.
  // PT: Implementação compartilhada de load_catalog_string()/load_catalog_file() uma vez que o
  //     texto está em memória -- ver i18n.cpp.
  I18nLoadResult parse_and_merge(const std::string& text);

  // EN: One resolution attempt: looks up `key` in the (lowercased) locale `locale_lc`, if that
  //     locale was ever loaded. Returns nullptr on any miss.
  // PT: Uma tentativa de resolução: busca `key` no locale `locale_lc` (minúsculo), se esse
  //     locale já foi carregado alguma vez. Retorna nullptr em qualquer falta.
  const std::string* lookup_raw(const std::string& locale_lc, const std::string& key) const;

  // EN: Walks the full RESOLUTION ORDER (steps 1-4 of the header comment): for each locale step
  //     in order (active exact, active base language, fallback exact, fallback base language),
  //     tries every string in `candidate_keys` IN ORDER before advancing to the next step --
  //     this is what implements "falls back from .one to .other within each locale step before
  //     advancing" for tr_plural() (candidate_keys = {"<key>.one", "<key>.other"}, or just
  //     {"<key>.other"} when the selected category already IS other) while tr() passes a single-
  //     element {"<key>"}. Returns nullptr if nothing resolves in any step; the caller supplies
  //     its own step-5 (bare key) last resort.
  // PT: Percorre a ORDEM DE RESOLUÇÃO completa (passos 1-4 do comentário de cabeçalho): para
  //     cada passo de locale em ordem (ativo exato, idioma-base do ativo, fallback exato,
  //     idioma-base do fallback), tenta toda string de `candidate_keys` EM ORDEM antes de
  //     avançar pro próximo passo -- é isso que implementa "cai de .one pra .other dentro de
  //     cada passo de locale antes de avançar" pro tr_plural() (candidate_keys =
  //     {"<key>.one", "<key>.other"}, ou só {"<key>.other"} quando a categoria selecionada já É
  //     other) enquanto tr() passa um único elemento {"<key>"}. Retorna nullptr se nada resolver
  //     em nenhum passo; o chamador fornece seu próprio último recurso do passo 5 (chave crua).
  const std::string* resolve_candidates(const std::vector<std::string>& candidate_keys) const;
};

} // namespace glintfx
