// SPDX-License-Identifier: MPL-2.0
// EN: I18n implementation -- catalog parser, locale-aware lookup with fallback, CLDR-lite
//     plural category selection. See glintfx/i18n.hpp for the catalog format, escapes, plural
//     convention, and resolution order (the module's whole public contract).
//
//     CLDR PLURAL RULE SOURCE (cited per the team-lead brief, "pegue as regras CLDR certas, não
//     chute"): Unicode CLDR Language Plural Rules chart v48,
//     https://www.unicode.org/cldr/charts/48/supplemental/language_plural_rules.html, fetched
//     and read directly (not assumed from memory) on 2026-07-20. Verbatim rule text for the two
//     locales i18n-1 ships:
//       en (English) -- one: "i = 1 and v = 0" (integer part exactly 1, no visible decimals);
//                       other: everything else (0, 2-16, 100, ...).
//       pt (Portuguese, the GENERIC "pt" entry the chart lists -- this is the base language
//           subtag shared by both pt-BR and pt-PT; the chart's pt-PT-specific override was not
//           consulted because ADR-0015's i18n-1 slice ships pt-BR only, and this module always
//           selects a plural category from the ACTIVE LOCALE'S BASE LANGUAGE, "pt", never a
//           region-specific pt-PT rule set) -- one: "i = 0..1" (integer part 0 OR 1); other:
//           everything else (2-17, 100, ...).
//     `i` in CLDR's grammar is the integer part of the operand with any trailing decimals
//     truncated (`v` is the count of visible decimal digits). This module's plural API only
//     ever receives a whole `long long`, so `i` == the (absolute value of the) count directly --
//     v is always 0, matching both rules' "v = 0"/implicit-integer condition exactly; CLDR
//     defines cardinal categories on the ABSOLUTE VALUE of the operand (negative counts are not
//     a separate case), hence the absolute value below -- computed WITHOUT `std::llabs()`/
//     unary `-`, both of which are undefined behaviour on `LLONG_MIN` (there is no positive
//     `long long` that represents its magnitude): see plural_category_for()'s
//     `absolute_value_of()`.
// PT: Implementação do I18n -- parser de catálogo, lookup com consciência de locale e fallback,
//     seleção de categoria de plural CLDR-lite. Ver glintfx/i18n.hpp pro formato do catálogo,
//     escapes, convenção de plural, e ordem de resolução (o contrato público inteiro do módulo).
//
//     FONTE DA REGRA DE PLURAL CLDR (citada conforme pedido no brief do team-lead, "pegue as
//     regras CLDR certas, não chute"): gráfico Unicode CLDR Language Plural Rules v48,
//     https://www.unicode.org/cldr/charts/48/supplemental/language_plural_rules.html, buscado e
//     lido diretamente (não assumido de memória) em 2026-07-20. Texto verbatim da regra pros
//     dois locales que o i18n-1 entrega:
//       en (Inglês) -- one: "i = 1 and v = 0" (parte inteira exatamente 1, sem decimais
//                      visíveis); other: todo o resto (0, 2-16, 100, ...).
//       pt (Português, a entrada GENÉRICA "pt" que o gráfico lista -- essa é a subtag de idioma-
//           base compartilhada por pt-BR e pt-PT; o override específico pt-PT do gráfico NÃO foi
//           consultado porque a fatia i18n-1 da ADR-0015 entrega só pt-BR, e este módulo sempre
//           seleciona uma categoria de plural a partir do IDIOMA-BASE DO LOCALE ATIVO, "pt",
//           nunca um conjunto de regra específico de região pt-PT) -- one: "i = 0..1" (parte
//           inteira 0 OU 1); other: todo o resto (2-17, 100, ...).
//     `i` na gramática do CLDR é a parte inteira do operando com decimais à direita truncados
//     (`v` é a contagem de dígitos decimais visíveis). A API de plural deste módulo só recebe um
//     `long long` inteiro, então `i` == a (valor absoluto da) contagem diretamente -- v é sempre
//     0, batendo exatamente com a condição "v = 0"/inteiro-implícito das duas regras; o CLDR
//     define categorias cardinais sobre o VALOR ABSOLUTO do operando (contagens negativas não
//     são um caso separado), daí o valor absoluto abaixo -- calculado SEM `std::llabs()`/unário
//     `-`, ambos comportamento indefinido em `LLONG_MIN` (não existe `long long` positivo que
//     represente a magnitude dele): ver `absolute_value_of()` em plural_category_for().
// Copyright (c) 2026 Petrus Silva Costa

#include <glintfx/i18n.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace glintfx {

namespace {

// EN: ASCII-only lower-casing -- locale codes (BCP 47 tags like "pt-BR"/"en-US") and catalog
//     keys are always ASCII in this module's own grammar, so a full Unicode case-fold is out of
//     scope (would be a real dependency, ICU-class, for a problem this format does not have).
// PT: Lower-casing só-ASCII -- códigos de locale (tags BCP 47 tipo "pt-BR"/"en-US") e chaves de
//     catálogo são sempre ASCII na gramática própria deste módulo, então um case-fold Unicode
//     completo está fora de escopo (seria uma dependência real, classe ICU, pra um problema que
//     este formato não tem).
std::string to_lower_ascii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  });
  return s;
}

// EN: The base-language subtag: everything before the first '-', or the whole string when there
//     is no '-' (e.g. "pt-BR" -> "pt", "en-US" -> "en", "pt" -> "pt").
// PT: A subtag de idioma-base: tudo antes do primeiro '-', ou a string inteira quando não há
//     '-' (ex.: "pt-BR" -> "pt", "en-US" -> "en", "pt" -> "pt").
std::string base_language(const std::string& locale_lc) {
  const std::size_t dash = locale_lc.find('-');
  return dash == std::string::npos ? locale_lc : locale_lc.substr(0, dash);
}

// EN: Trims ASCII whitespace (space, tab, CR, LF) from both ends.
// PT: Remove espaço em branco ASCII (espaço, tab, CR, LF) das duas pontas.
std::string trim(const std::string& s) {
  auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  std::size_t begin = 0;
  std::size_t end   = s.size();
  while (begin < end && is_space(s[begin])) ++begin;
  while (end > begin && is_space(s[end - 1])) --end;
  return s.substr(begin, end - begin);
}

// EN: Processes the minimal escape set documented in i18n.hpp (`\\`->`\`, `\n`->newline,
//     `\t`->tab). Any other backslash sequence is copied through literally, both characters --
//     an unrecognised escape is never a parse error (fail-high on robustness, not on syntax
//     purism; a hostile/typo'd catalog must never crash the loader).
// PT: Processa o conjunto mínimo de escapes documentado em i18n.hpp (`\\`->`\`, `\n`->nova
//     linha, `\t`->tab). Qualquer outra sequência de barra invertida é copiada literalmente, os
//     dois caracteres -- um escape não reconhecido nunca é erro de parse (fail-high em
//     robustez, não em purismo de sintaxe; um catálogo hostil/com erro de digitação nunca deve
//     crashar o loader).
std::string unescape_value(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\' && i + 1 < raw.size()) {
      const char next = raw[i + 1];
      if (next == '\\') { out.push_back('\\'); ++i; continue; }
      if (next == 'n')  { out.push_back('\n'); ++i; continue; }
      if (next == 't')  { out.push_back('\t'); ++i; continue; }
      // EN: unknown escape -- fall through, copy the backslash itself as a literal character;
      //     the loop's next iteration then copies `next` normally.
      // PT: escape desconhecido -- cai pro fim, copia a própria barra invertida como caractere
      //     literal; a próxima iteração do laço copia `next` normalmente.
    }
    out.push_back(raw[i]);
  }
  return out;
}

// EN: CLDR-lite cardinal plural category, the two this module supports (see this file's header
//     comment for the cited CLDR v48 rule text and the "v is always 0" reasoning).
// PT: Categoria de plural cardinal CLDR-lite, as duas que este módulo suporta (ver o comentário
//     de cabeçalho deste arquivo pro texto de regra CLDR v48 citado e o raciocínio "v é sempre 0").
enum class PluralCategory { One, Other };

// EN: Absolute value of `n` as an unsigned long long, safe for the whole `long long` range
//     including `LLONG_MIN` -- unlike `std::llabs(n)` or unary `-n`, which are undefined
//     behaviour exactly there (confirmed live by UBSan: "negation of -9223372036854775808 cannot
//     be represented in type 'long long'"). Converting a negative `long long` to an unsigned type
//     is well-defined (wraps modulo 2^64), and unsigned subtraction from 0 is well-defined too,
//     so `0ULL - static_cast<unsigned long long>(n)` recovers the true magnitude for every `n`,
//     LLONG_MIN included, without ever computing the value in signed arithmetic.
// PT: Valor absoluto de `n` como unsigned long long, seguro para o range inteiro de `long long`
//     inclusive `LLONG_MIN` -- diferente de `std::llabs(n)` ou unário `-n`, que são comportamento
//     indefinido exatamente ali (confirmado ao vivo pelo UBSan: "negation of
//     -9223372036854775808 cannot be represented in type 'long long'"). Converter um `long long`
//     negativo pra um tipo sem sinal é bem-definido (dá a volta módulo 2^64), e subtração sem
//     sinal a partir de 0 também é bem-definida, então `0ULL - static_cast<unsigned long long>(n)`
//     recupera a magnitude real pra todo `n`, LLONG_MIN incluso, sem nunca calcular o valor em
//     aritmética com sinal.
unsigned long long absolute_value_of(long long n) {
  return n < 0 ? (0ULL - static_cast<unsigned long long>(n)) : static_cast<unsigned long long>(n);
}

// EN: Selects the plural category for `base_lang` (already lower-cased, already stripped of any
//     region subtag -- callers pass base_language(active_locale_lc)) and count `n`. Unknown
//     locales fall back to the English rule (n==1 -> One): "other" is CLDR's universal
//     catch-all and every locale defines it, and English's shape (singular only at exactly 1)
//     is the least-surprising default for a locale this table does not yet curate -- consistent
//     with the project's own-font-engine posture of "start small, extend by real demand, never
//     guess a whole table speculatively" (ADR-0015 section 6 anti-OE item 1).
// PT: Seleciona a categoria de plural para `base_lang` (já em minúsculas, já sem qualquer subtag
//     de região -- chamadores passam base_language(active_locale_lc)) e a contagem `n`. Locales
//     desconhecidos caem pra regra do inglês (n==1 -> One): "other" é o catch-all universal do
//     CLDR e todo locale o define, e o formato do inglês (singular só em exatamente 1) é o
//     default menos surpreendente pra um locale que esta tabela ainda não cura -- consistente
//     com a própria postura do motor de fonte próprio de "começar pequeno, estender por demanda
//     real, nunca chutar uma tabela inteira especulativamente" (ADR-0015 seção 6 item anti-OE 1).
PluralCategory plural_category_for(const std::string& base_lang, long long n) {
  const unsigned long long an = absolute_value_of(n); // EN: CLDR cardinal rules key off |n|. PT: regras cardinais do CLDR usam |n|.
  if (base_lang == "en") {
    return an == 1ULL ? PluralCategory::One : PluralCategory::Other;
  }
  if (base_lang == "pt") {
    return (an == 0ULL || an == 1ULL) ? PluralCategory::One : PluralCategory::Other;
  }
  return an == 1ULL ? PluralCategory::One : PluralCategory::Other;
}

} // namespace

I18nLoadResult I18n::parse_and_merge(const std::string& text) {
  I18nLoadResult result;
  result.ok = true;

  std::string current_locale_lc; // EN: "" means "no [locale] section open yet". PT: "" = "nenhuma seção [locale] aberta ainda".

  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    // EN: std::getline splits on '\n' but leaves a trailing '\r' in place for CRLF input --
    //     trim() below strips it along with ordinary whitespace either way.
    // PT: std::getline separa por '\n' mas deixa um '\r' à direita para input CRLF -- o trim()
    //     abaixo remove isso junto do espaço em branco comum de qualquer forma.
    const std::string trimmed = trim(line);

    if (trimmed.empty() || trimmed[0] == '#') continue; // EN: blank line / comment. PT: linha em branco / comentário.

    if (trimmed.front() == '[') {
      if (trimmed.back() != ']' || trimmed.size() < 2) {
        // EN: malformed section header (missing closing ']', or just "["). Skip+count; the
        //     PREVIOUS section (if any) stays open so a single bad header line does not silently
        //     misattribute the rest of a well-formed file.
        // PT: cabeçalho de seção malformado (falta o ']' de fechamento, ou só "["). Pula+conta;
        //     a seção ANTERIOR (se houver) continua aberta pra que uma única linha de cabeçalho
        //     ruim não atribua mal silenciosamente o resto de um arquivo bem-formado.
        ++result.lines_skipped;
        continue;
      }
      const std::string locale_display = trim(trimmed.substr(1, trimmed.size() - 2));
      if (locale_display.empty()) {
        ++result.lines_skipped;
        continue;
      }
      current_locale_lc = to_lower_ascii(locale_display);
      auto [it, inserted] = catalog_by_locale_lc_.try_emplace(current_locale_lc);
      if (inserted) it->second.display_name = locale_display;
      continue;
    }

    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos || current_locale_lc.empty()) {
      // EN: no '=' at all (not a key=value line), OR a key=value line before any [locale]
      //     header opened -- both are malformed in this grammar, skip+count.
      // PT: sem '=' nenhum (não é linha key=value), OU uma linha key=value antes de qualquer
      //     cabeçalho [locale] ter aberto -- ambos malformados nesta gramática, pula+conta.
      ++result.lines_skipped;
      continue;
    }
    const std::string key = trim(trimmed.substr(0, eq));
    if (key.empty()) {
      ++result.lines_skipped;
      continue;
    }
    const std::string value = unescape_value(trim(trimmed.substr(eq + 1)));

    catalog_by_locale_lc_[current_locale_lc].entries[key] = value; // EN: last-write-wins. PT: último-escreve-vence.
    ++result.entries_loaded;
  }

  return result;
}

I18nLoadResult I18n::load_catalog_string(const char* text) {
  if (text == nullptr) return I18nLoadResult{};
  return parse_and_merge(std::string(text));
}

I18nLoadResult I18n::load_catalog_file(const char* path) {
  if (path == nullptr) return I18nLoadResult{};
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) return I18nLoadResult{};
  // EN: is_open() alone is not enough -- on Linux it is also true for a DIRECTORY path (opening
  //     a directory as an ifstream "succeeds" at the open() syscall level). Without this probe,
  //     `buffer << file.rdbuf()` below reads zero bytes from a directory (the underlying read()
  //     syscall fails with EISDIR, confirmed via strace) and returns {.ok = true,
  //     .entries_loaded = 0} -- indistinguishable from a legitimate empty (0-byte) catalog file.
  //     NOTE: checking file.good()/fail() AFTER `buffer << file.rdbuf()` does NOT work here --
  //     `ostream& operator<<(ostream&, streambuf*)` extracts through the streambuf pointer
  //     directly, bypassing `file`'s own istream sentry, so `file`'s failbit is never set by that
  //     read, empirically confirmed identical (good=1, eof=0, size=0) for a directory and an
  //     empty file. `peek()` goes through the sentry and DOES distinguish them: a directory sets
  //     failbit+badbit (eofbit stays clear); a genuinely empty file sets ONLY eofbit. peek() does
  //     not consume the byte it reads, so the character (if any) is still there for the real read
  //     below via the shared streambuf.
  // PT: is_open() sozinho não basta -- no Linux também é true para um caminho de DIRETÓRIO (abrir
  //     um diretório como ifstream "funciona" no nível da syscall open()). Sem esta sondagem, o
  //     `buffer << file.rdbuf()` abaixo lê zero bytes de um diretório (a syscall read() por baixo
  //     falha com EISDIR, confirmado via strace) e retorna {.ok = true, .entries_loaded = 0} --
  //     indistinguível de um arquivo de catálogo legitimamente vazio (0 bytes). NOTA: checar
  //     file.good()/fail() DEPOIS de `buffer << file.rdbuf()` NÃO funciona aqui -- o
  //     `ostream& operator<<(ostream&, streambuf*)` extrai direto pelo ponteiro do streambuf,
  //     contornando o sentry do próprio istream `file`, então o failbit de `file` nunca é setado
  //     por essa leitura, confirmado empiricamente idêntico (good=1, eof=0, size=0) tanto pra um
  //     diretório quanto pra um arquivo vazio. `peek()` passa pelo sentry e DISTINGUE os dois: um
  //     diretório seta failbit+badbit (eofbit fica limpo); um arquivo genuinamente vazio seta SÓ
  //     eofbit. peek() não consome o byte que lê, então o caractere (se houver) continua
  //     disponível pra leitura real abaixo via o streambuf compartilhado.
  file.peek();
  if (file.fail() && !file.eof()) return I18nLoadResult{};
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return parse_and_merge(buffer.str());
}

bool I18n::set_locale(const char* locale_code) {
  if (locale_code == nullptr || locale_code[0] == '\0') return false;
  active_locale_ = locale_code;
  return true;
}

const std::string& I18n::locale() const { return active_locale_; }

bool I18n::set_fallback_locale(const char* locale_code) {
  if (locale_code == nullptr || locale_code[0] == '\0') return false;
  fallback_locale_ = locale_code;
  return true;
}

const std::string& I18n::fallback_locale() const { return fallback_locale_; }

std::vector<std::string> I18n::locales() const {
  std::vector<std::string> out;
  out.reserve(catalog_by_locale_lc_.size());
  // EN: catalog_by_locale_lc_ is a std::map keyed by the lower-cased locale, so this iteration
  //     is already sorted case-insensitively; each value carries the display_name (original
  //     casing, first-seen) documented on locales() in i18n.hpp.
  // PT: catalog_by_locale_lc_ é um std::map indexado pelo locale em minúsculas, então esta
  //     iteração já vem ordenada sem diferenciar maiúsculas/minúsculas; cada valor carrega o
  //     display_name (grafia original, 1ª vez vista) documentado em locales() no i18n.hpp.
  for (const auto& [locale_lc, section] : catalog_by_locale_lc_) {
    (void)locale_lc;
    out.push_back(section.display_name);
  }
  return out;
}

const std::string* I18n::lookup_raw(const std::string& locale_lc, const std::string& key) const {
  const auto section_it = catalog_by_locale_lc_.find(locale_lc);
  if (section_it == catalog_by_locale_lc_.end()) return nullptr;
  const auto entry_it = section_it->second.entries.find(key);
  if (entry_it == section_it->second.entries.end()) return nullptr;
  return &entry_it->second;
}

const std::string* I18n::resolve_candidates(const std::vector<std::string>& candidate_keys) const {
  const std::string active_lc   = to_lower_ascii(active_locale_);
  const std::string fallback_lc = to_lower_ascii(fallback_locale_);

  // EN: Build the ordered list of locale steps (RESOLUTION ORDER 1-4), skipping a step when it
  //     is empty (never set) or a byte-for-byte duplicate of a step already queued -- e.g.
  //     active_locale="en" has no '-', so its "base language" step would just repeat step 1 and
  //     is dropped rather than re-tried for nothing.
  // PT: Monta a lista ordenada de passos de locale (ORDEM DE RESOLUÇÃO 1-4), pulando um passo
  //     quando está vazio (nunca definido) ou é duplicata byte-a-byte de um passo já enfileirado
  //     -- ex.: active_locale="en" não tem '-', então o passo "idioma-base" dele só repetiria o
  //     passo 1 e é descartado em vez de re-tentado à toa.
  std::vector<std::string> locale_steps;
  auto add_step = [&locale_steps](const std::string& candidate) {
    if (candidate.empty()) return;
    if (std::find(locale_steps.begin(), locale_steps.end(), candidate) != locale_steps.end()) return;
    locale_steps.push_back(candidate);
  };
  add_step(active_lc);
  add_step(base_language(active_lc));
  add_step(fallback_lc);
  add_step(base_language(fallback_lc));

  // EN: The inner search is `std::find_if` over candidate_keys, predicate = "this key exists in
  //     locale_lc" (checked via lookup_raw, discarding the pointer -- only membership matters
  //     here). A second lookup_raw() retrieves the actual pointer once find_if has located the
  //     matching key; both calls hit the same small per-locale std::map (a handful of keys in
  //     realistic catalogs), so the repeat lookup is not a measured hot path, and this keeps the
  //     predicate pure (no captured-by-reference output parameter smuggled through a "side-
  //     effecting predicate", a well-known algorithm-abuse anti-pattern).
  // PT: A busca interna é `std::find_if` sobre candidate_keys, predicado = "esta chave existe em
  //     locale_lc" (checado via lookup_raw, descartando o ponteiro -- só a existência importa
  //     aqui). Um segundo lookup_raw() recupera o ponteiro de fato depois que find_if localiza a
  //     chave correspondente; as duas chamadas batem no mesmo std::map pequeno por-locale (um
  //     punhado de chaves em catálogos realistas), então o lookup repetido não é um hot path
  //     medido, e isso mantém o predicado puro (sem parâmetro de saída capturado por referência
  //     contrabandeado por um "predicado com efeito colateral", um anti-padrão conhecido de uso
  //     de algoritmo).
  for (const auto& locale_lc : locale_steps) {
    const auto match = std::find_if(candidate_keys.begin(), candidate_keys.end(),
                                     [this, &locale_lc](const std::string& candidate_key) {
                                       return lookup_raw(locale_lc, candidate_key) != nullptr;
                                     });
    if (match != candidate_keys.end()) return lookup_raw(locale_lc, *match);
  }
  return nullptr;
}

std::string I18n::tr(const char* key) const {
  if (key == nullptr) return std::string();
  const std::string key_str(key);
  if (const std::string* found = resolve_candidates({key_str})) return *found;
  return key_str; // EN: step 5, last resort. PT: passo 5, último recurso.
}

std::string I18n::tr_plural(const char* key, long long n) const {
  if (key == nullptr) return std::string();
  const std::string key_str(key);

  const PluralCategory category = plural_category_for(base_language(to_lower_ascii(active_locale_)), n);
  const std::string one_key   = key_str + ".one";
  const std::string other_key = key_str + ".other";

  std::vector<std::string> candidates;
  if (category == PluralCategory::One) candidates.push_back(one_key);
  candidates.push_back(other_key);

  const std::string* found = resolve_candidates(candidates);
  std::string result = found ? *found : key_str; // EN: step 5, last resort (bare key, no substitution).
                                                   // PT: passo 5, último recurso (chave crua, sem substituição).
  if (!found) return result;

  // EN: Substitute every '#' with the base-10 digits of n (sign included) -- ICU convention,
  //     documented in i18n.hpp as the plural count placeholder.
  // PT: Substitui todo '#' pelos dígitos base-10 de n (sinal incluído) -- convenção ICU,
  //     documentada em i18n.hpp como o placeholder de contagem do plural.
  const std::string count_str = std::to_string(n);
  std::string substituted;
  substituted.reserve(result.size());
  for (char c : result) {
    if (c == '#') {
      substituted += count_str;
    } else {
      substituted.push_back(c);
    }
  }
  return substituted;
}

} // namespace glintfx
