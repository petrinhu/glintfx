// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- implementation. See shorthand.hpp's own header comment for the full
//     scope/boundary this file holds itself to; this file is about HOW, not WHY (same convention
//     lexer.cpp's own header states for itself).
//
//     THE TWO ROUTING CLASSIFIERS, AND WHY THEY ARE THE NARROWEST HONEST IMPLEMENTATION OF
//     UPSTREAM'S OWN FallThrough DISPATCH THIS ITEM'S BOUNDARY ALLOWS: real upstream
//     (`PropertySpecification.cpp:429-471`, read directly) decides FallThrough routing by
//     actually calling each candidate longhand's own real `ParseValue` (full unit/color/keyword
//     validation) and advancing only on success. This module cannot do that without a value
//     parser, explicitly out of this item's own scope ("não implemente parse de valor"). Instead:
//       - `looks_like_number_token`: `^-?[0-9]+(\.[0-9]+)?$` -- a BARE number, no unit suffix, no
//         `%`. This is exactly what routes a `flex` token to `flex-grow`/`flex-shrink` (both
//         `Number`-domain, section 6.1) before EITHER ever gets tried against `flex-basis`
//         (`Keyword(auto)`-or-`LengthPercent`-domain) -- a token WITH a unit suffix (`30px`,
//         `1dp`) or the literal `auto` genuinely fails this classifier, matching upstream's own
//         `ParseValue` failure for the NUMBER property type on those same inputs.
//       - `looks_like_length_token`: starts with a digit, `.`, or `-` immediately followed by a
//         digit/`.` -- ANYTHING numeric-shaped, with or without a unit suffix. This routes
//         `border-{side}`'s own 2 tokens: the width candidate is tried with THIS classifier
//         first; if it does not look numeric at all (a hex color, `#7A5A2E`), the SECOND item
//         (`-color`) accepts it via a catch-all (see `kCatchAll` below) -- this exactly
//         reproduces upstream's own real behaviour for the corpus's 100%-measured 2-token,
//         width-then-color shape (docs/uix-rcss.md section 6.2). 🔴 The REVERSED order
//         (color-then-width) is `MalformedValue`, NOT a second success path -- upstream's own real
//         loop (`PropertySpecification.cpp:429-471`) always advances the ITEM cursor, only
//         advances the TOKEN cursor on a match, so a token that only matches the LAST item leaves
//         the item cursor exhausted while a later token is still unclaimed, and upstream's own
//         post-loop guard aborts the whole shorthand -- a correction to docs/uix-rcss.md section
//         6.2's own "order-independent between the two" phrase, traced iteration-by-iteration, not
//         guessed. Proven by shorthand_expansion_sanity.cpp's own
//         `test_border_top_fallthrough_order_is_load_bearing`.
//     Neither classifier ever claims a token is a VALID length/number (a `dp`/`px`/`%` unit's own
//     spelling, a color's own hex-digit count) -- see this file's header, "Scope", for why that
//     line is drawn here and not one token-shape-check further.
//
//     THE `kCatchAll` CLASSIFIER (used as EVERY FallThrough/Flex chain's own LAST candidate):
//     returns `true` unconditionally. This mirrors upstream's own real terminal behaviour for the
//     corpus's shapes measured here (`border-*-color`, `flex-basis`, `background-color`): by the
//     time routing reaches the LAST item in a chain with EXACTLY as many tokens as items (the
//     only shape this file's own `expand_fallthrough` accepts for the non-defaulted case, see
//     that function's own doc-comment below), there is nowhere else for the remaining token to
//     go, so accepting it unconditionally is observationally identical to upstream's own
//     `ParseValue`-based acceptance for every real value this repo's own corpus contains --
//     divergence (a hostile, genuinely-malformed final token) is explicitly this item's own
//     un-owned territory, deferred to a future parser slice, same "narrow routing, not
//     validation" boundary this file's own header states.
// PT: UIX-PROP-REGISTRY -- implementação. Ver o próprio comentário de cabeçalho do shorthand.hpp
//     pro escopo/fronteira completos a que este arquivo se prende; este arquivo é sobre COMO, não
//     PORQUÊ (mesma convenção que o próprio cabeçalho do lexer.cpp declara pra si mesmo).
//
//     OS DOIS CLASSIFICADORES DE ROTEAMENTO, E POR QUE SÃO A IMPLEMENTAÇÃO HONESTA MAIS ESTREITA
//     DO PRÓPRIO DISPATCH DE FallThrough DO UPSTREAM QUE A FRONTEIRA DESTE ITEM PERMITE: o
//     upstream real (`PropertySpecification.cpp:429-471`, lido direto) decide o roteamento de
//     FallThrough chamando de fato o próprio `ParseValue` real de cada longhand candidato
//     (validação completa de unidade/cor/palavra-chave) e só avançando em caso de sucesso. Este
//     módulo não consegue fazer isso sem um parser de valor, explicitamente fora do próprio
//     escopo deste item ("não implemente parse de valor"). Em vez disso:
//       - `looks_like_number_token`: `^-?[0-9]+(\.[0-9]+)?$` -- um número PURO, sem sufixo de
//         unidade, sem `%`. Isto é exatamente o que roteia um token de `flex` pra
//         `flex-grow`/`flex-shrink` (os dois de domínio `Number`, seção 6.1) antes de QUALQUER um
//         dos dois sequer ser tentado contra `flex-basis` (domínio `Keyword(auto)`-ou-
//         `LengthPercent`) -- um token COM sufixo de unidade (`30px`, `1dp`) ou o literal `auto`
//         genuinamente falha este classificador, casando com a própria falha real do `ParseValue`
//         do upstream pro tipo de propriedade NÚMERO nesses mesmos inputs.
//       - `looks_like_length_token`: começa com um dígito, `.`, ou `-` imediatamente seguido de um
//         dígito/`.` -- QUALQUER COISA com forma numérica, com ou sem sufixo de unidade. Isto
//         roteia os próprios 2 tokens de `border-{side}`: o candidato de width é tentado com ESTE
//         classificador primeiro; se ele não parece numérico de jeito nenhum (uma cor hex,
//         `#7A5A2E`), o SEGUNDO item (`-color`) o aceita via um catch-all (ver `kCatchAll`
//         abaixo) -- isto reproduz exatamente o próprio comportamento real do upstream pra forma
//         de 2-tokens, width-depois-color, 100%-medida do corpus (seção 6.2 do docs/uix-rcss.md).
//         🔴 A ordem REVERTIDA (color-depois-width) é `MalformedValue`, NÃO um segundo caminho de
//         sucesso -- o próprio laço real do upstream (`PropertySpecification.cpp:429-471`) sempre
//         avança o cursor de ITEM, só avança o cursor de TOKEN num casamento, então um token que só
//         casa com o ÚLTIMO item deixa o cursor de item esgotado enquanto um token posterior ainda
//         não foi reivindicado, e a própria guarda pós-laço do upstream aborta o shorthand inteiro
//         -- uma correção à própria frase "independente de ordem entre os dois" da seção 6.2 do
//         docs/uix-rcss.md, rastreada iteração-por-iteração, não chutada. Provado pelo próprio
//         `test_border_top_fallthrough_order_is_load_bearing` do shorthand_expansion_sanity.cpp.
//     Nenhum dos dois classificadores nunca alega que um token é um length/número VÁLIDO (a
//     própria grafia de uma unidade `dp`/`px`/`%`, a própria contagem de dígito hex de uma cor) --
//     ver o cabeçalho deste arquivo, "Escopo", pro porquê desta linha ser traçada aqui e não mais
//     um check-de-forma-de-token adiante.
//
//     O CLASSIFICADOR `kCatchAll` (usado como o PRÓPRIO ÚLTIMO candidato de toda cadeia
//     FallThrough/Flex): retorna `true` incondicionalmente. Isto espelha o próprio comportamento
//     terminal real do upstream pras formas do corpus medidas aqui (`border-*-color`,
//     `flex-basis`, `background-color`): no momento em que o roteamento alcança o ÚLTIMO item de
//     uma cadeia com EXATAMENTE tantos tokens quanto itens (a única forma que o próprio
//     `expand_fallthrough` deste arquivo aceita pro caso sem-default, ver o próprio
//     doc-comment daquela função abaixo), não há mais lugar nenhum pro token restante ir, então
//     aceitá-lo incondicionalmente é observacionalmente idêntico à própria aceitação baseada em
//     `ParseValue` do upstream pra todo valor real que o próprio corpus deste repo contém --
//     divergência (um token final hostil, genuinamente malformado) é explicitamente território
//     não-possuído deste item, adiado pra uma futura fatia de parser, mesma fronteira "roteamento
//     estreito, não validação" que o próprio cabeçalho deste arquivo declara.
// Copyright (c) 2026 Petrus Silva Costa
#include "shorthand.hpp"

#include <algorithm>
#include <array>

namespace glintfx::uix::style {

namespace {

// EN: Same 4-character whitespace set lexer.cpp's own `is_whitespace` uses -- see this file's
//     header, "Scope", for why this module's own tokenization needs nothing richer.
// PT: Mesmo conjunto de 4 caracteres de whitespace que o próprio `is_whitespace` do lexer.cpp usa
//     -- ver "Escopo" no cabeçalho deste arquivo pro porquê a própria tokenização deste módulo não
//     precisar de nada mais rico.
constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

// EN: Splits `s` on whitespace RUNS (one or more consecutive whitespace bytes collapse to a
//     single separator), views into `s` itself -- zero-copy, same "Token fields reference the
//     original source buffer" precedent lexer.hpp's own header states.
// PT: Divide `s` por TRECHOS de whitespace (um ou mais bytes de whitespace consecutivos colapsam
//     num único separador), views sobre o próprio `s` -- zero-cópia, mesmo precedente "campos de
//     Token referenciam o próprio buffer-fonte original" que o próprio cabeçalho do lexer.hpp
//     declara.
std::vector<std::string_view> split_whitespace(std::string_view s) {
  std::vector<std::string_view> tokens;
  std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && is_whitespace(s[i])) {
      ++i;
    }
    const std::size_t start = i;
    while (i < s.size() && !is_whitespace(s[i])) {
      ++i;
    }
    if (i > start) {
      tokens.push_back(s.substr(start, i - start));
    }
  }
  return tokens;
}

// EN: See this file's header comment, "The two routing classifiers".
// PT: Ver o parágrafo "Os dois classificadores de roteamento" do comentário de cabeçalho deste
//     arquivo.
bool looks_like_number_token(std::string_view t) {
  if (t.empty()) {
    return false;
  }
  std::size_t i = 0;
  if (t[0] == '-') {
    i = 1;
    if (i >= t.size()) {
      return false;
    }
  }
  bool has_int_digit = false;
  while (i < t.size() && is_digit(t[i])) {
    ++i;
    has_int_digit = true;
  }
  if (!has_int_digit) {
    return false;
  }
  if (i < t.size() && t[i] == '.') {
    ++i;
    bool has_frac_digit = false;
    while (i < t.size() && is_digit(t[i])) {
      ++i;
      has_frac_digit = true;
    }
    if (!has_frac_digit) {
      return false;
    }
  }
  return i == t.size();
}

bool looks_like_length_token(std::string_view t) {
  if (t.empty()) {
    return false;
  }
  std::size_t i = 0;
  if (t[0] == '-') {
    i = 1;
    if (i >= t.size()) {
      return false;
    }
  }
  return is_digit(t[i]) || t[i] == '.';
}

bool kCatchAllFn(std::string_view) {
  return true;
}

// EN: See this file's header comment, "The kCatchAll classifier".
// PT: Ver o parágrafo "O classificador kCatchAll" do comentário de cabeçalho deste arquivo.
constexpr auto kCatchAll = kCatchAllFn;

// EN: One candidate in a FallThrough/Flex chain. `default_value` is only consulted when
//     `has_default` is true AND this item was never claimed by a token -- see `expand_fallthrough`
//     below for the exact semantics (mirrors `PropertySpecification.cpp`'s own
//     `default_omitted_values` mechanism, `Flex`-only in upstream, generalised here so
//     plain-`FallThrough` shorthands can express "no default, every item MUST be claimed" via
//     `has_default = false`).
// PT: Um candidato numa cadeia FallThrough/Flex. `default_value` só é consultado quando
//     `has_default` é verdadeiro E este item nunca foi reivindicado por um token -- ver o próprio
//     `expand_fallthrough` abaixo pra semântica exata (espelha o próprio mecanismo
//     `default_omitted_values` do `PropertySpecification.cpp`, só-de-`Flex` no upstream,
//     generalizado aqui pra shorthands `FallThrough` simples poderem expressar "sem default, todo
//     item TEM de ser reivindicado" via `has_default = false`).
struct FallthroughItem {
  std::string_view name;
  bool (*accepts)(std::string_view) = kCatchAllFn;
  bool has_default = false;
  std::string_view default_value;
};

constexpr FallthroughItem kBorderSideItems[4][2] = {
    {{"border-top-width", looks_like_length_token, false, {}},
     {"border-top-color", kCatchAll, false, {}}},
    {{"border-right-width", looks_like_length_token, false, {}},
     {"border-right-color", kCatchAll, false, {}}},
    {{"border-bottom-width", looks_like_length_token, false, {}},
     {"border-bottom-color", kCatchAll, false, {}}},
    {{"border-left-width", looks_like_length_token, false, {}},
     {"border-left-color", kCatchAll, false, {}}},
};

constexpr FallthroughItem kBackgroundItems[1] = {
    {"background-color", kCatchAll, false, {}},
};

// EN: `PropertySpecification.cpp:320-334`'s own `default_omitted_values[] = {"1", "1", "0"}` --
//     see this file's header, "The two routing classifiers", for why `flex-grow`/`flex-shrink`
//     share `looks_like_number_token` and `flex-basis` is the chain's own catch-all.
// PT: O próprio `default_omitted_values[] = {"1", "1", "0"}` do
//     `PropertySpecification.cpp:320-334` -- ver "Os dois classificadores de roteamento" no
//     cabeçalho deste arquivo pro porquê `flex-grow`/`flex-shrink` compartilharem
//     `looks_like_number_token` e `flex-basis` ser o próprio catch-all da cadeia.
constexpr FallthroughItem kFlexItems[3] = {
    {"flex-grow", looks_like_number_token, true, "1"},
    {"flex-shrink", looks_like_number_token, true, "1"},
    {"flex-basis", kCatchAll, true, "0"},
};

// EN: `Box`-algorithm target names, in upstream's own registered per-shorthand order
//     (docs/uix-rcss.md section 6.2/6.3 -- `border-radius`'s own CORNER order, top-left/
//     top-right/bottom-right/bottom-left, deliberately differs from the SIDE order the other 3
//     Box shorthands use, see shorthand_expansion_sanity.cpp's own
//     `test_border_radius_box_expansion_corner_order` for why this is pinned, not incidental).
// PT: Nomes-alvo do algoritmo `Box`, na própria ordem registrada do upstream por-shorthand (seção
//     6.2/6.3 do docs/uix-rcss.md -- a própria ordem de CANTO de `border-radius`, topo-esquerda/
//     topo-direita/baixo-direita/baixo-esquerda, diverge deliberadamente da ordem de LADO que os
//     outros 3 shorthands Box usam, ver o próprio
//     `test_border_radius_box_expansion_corner_order` do shorthand_expansion_sanity.cpp pro
//     porquê disto ser pinado, não incidental).
constexpr std::string_view kMarginTargets[4] = {"margin-top", "margin-right", "margin-bottom",
                                                "margin-left"};
constexpr std::string_view kPaddingTargets[4] = {"padding-top", "padding-right", "padding-bottom",
                                                 "padding-left"};
constexpr std::string_view kBorderRadiusTargets[4] = {
    "border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius",
    "border-bottom-left-radius"};
constexpr std::string_view kBorderColorTargets[4] = {"border-top-color", "border-right-color",
                                                     "border-bottom-color", "border-left-color"};

constexpr std::string_view kGapTargets[2] = {"row-gap", "column-gap"};
constexpr std::string_view kOverflowTargets[2] = {"overflow-x", "overflow-y"};

// EN: `RecursiveRepeat` targets are SHORTHAND names, not longhand names -- `border`'s own raw
//     value is fed VERBATIM to each, per docs/uix-rcss.md section 6.2's own citation.
// PT: Alvos de `RecursiveRepeat` são nomes de SHORTHAND, não nomes de longhand -- o próprio valor
//     cru de `border` é alimentado VERBATIM a cada um, per a própria citação da seção 6.2 do
//     docs/uix-rcss.md.
constexpr std::string_view kBorderRecursiveTargets[4] = {"border-top", "border-right",
                                                         "border-bottom", "border-left"};

enum class Algorithm { Box,
                       FallThrough,
                       RecursiveRepeat,
                       Replicate,
                       Flex };

struct ShorthandInfo {
  std::string_view name;
  Algorithm algorithm = Algorithm::Box;
  std::span<const std::string_view> box_or_replicate_or_repeat_targets;
  std::span<const FallthroughItem> fallthrough_items;
};

// EN: The 13-entry shorthand table, docs/uix-rcss.md section 6.2's own table restated as data.
//     Order here is the table's own row order, not alphabetical (unlike property_registry.cpp's
//     own table -- this one is never dumped per docs/uix-rcss.md section 3's own sort-order
//     contract, which applies to LONGHAND `PROP` lines only, so no ordering requirement applies
//     here).
// PT: A tabela de 13 shorthands, a própria tabela da seção 6.2 do docs/uix-rcss.md restatada como
//     dado. A ordem aqui é a própria ordem de linha da tabela, não alfabética (diferente da
//     própria tabela do property_registry.cpp -- esta nunca é dumpada per o próprio contrato de
//     ordem-de-sort da seção 3 do docs/uix-rcss.md, que se aplica só a linhas `PROP` de LONGHAND,
//     então nenhum requisito de ordenação se aplica aqui).
constexpr ShorthandInfo kShorthands[] = {
    {"margin", Algorithm::Box, kMarginTargets, {}},
    {"padding", Algorithm::Box, kPaddingTargets, {}},
    {"border-radius", Algorithm::Box, kBorderRadiusTargets, {}},
    {"border-color", Algorithm::Box, kBorderColorTargets, {}},
    {"border-top", Algorithm::FallThrough, {}, kBorderSideItems[0]},
    {"border-right", Algorithm::FallThrough, {}, kBorderSideItems[1]},
    {"border-bottom", Algorithm::FallThrough, {}, kBorderSideItems[2]},
    {"border-left", Algorithm::FallThrough, {}, kBorderSideItems[3]},
    {"border", Algorithm::RecursiveRepeat, kBorderRecursiveTargets, {}},
    {"background", Algorithm::FallThrough, {}, kBackgroundItems},
    {"gap", Algorithm::Replicate, kGapTargets, {}},
    {"overflow", Algorithm::Replicate, kOverflowTargets, {}},
    {"flex", Algorithm::Flex, {}, kFlexItems},
};

const ShorthandInfo* find_shorthand_info(std::string_view name) {
  const auto it = std::find_if(std::begin(kShorthands), std::end(kShorthands),
                               [name](const ShorthandInfo& sh) { return sh.name == name; });
  return it == std::end(kShorthands) ? nullptr : &*it;
}

// EN: docs/uix-rcss.md section 6.3's own table, verbatim -- `v0..v3` index into `tokens`, `n` is
//     `tokens.size()` (1..4). See lexer.hpp's own precedent for citing an algorithm once, by
//     table, rather than re-deriving it in prose per call site.
// PT: A própria tabela da seção 6.3 do docs/uix-rcss.md, verbatim -- `v0..v3` indexam `tokens`,
//     `n` é `tokens.size()` (1..4). Ver o próprio precedente do lexer.hpp de citar um algoritmo
//     uma vez, por tabela, em vez de re-derivá-lo em prosa por call site.
constexpr std::array<std::array<int, 4>, 5> kBoxIndexTable = {{
    {0, 0, 0, 0}, // n=0, unused (guarded out before this table is indexed)
    {0, 0, 0, 0}, // n=1
    {0, 1, 0, 1}, // n=2
    {0, 1, 2, 1}, // n=3
    {0, 1, 2, 3}, // n=4
}};

// EN: `Box` algorithm (docs/uix-rcss.md section 6.3). `targets` must have exactly 4 entries.
//     Accepts 1-4 tokens; anything else is `MalformedValue` (0 tokens: nothing to expand; 5+:
//     over-specified, matches upstream's own `PropertySpecification.cpp` "Abort over-specified
//     shorthand values" check for the shorthand types that share its final generic loop).
// PT: Algoritmo `Box` (seção 6.3 do docs/uix-rcss.md). `targets` precisa ter exatamente 4
//     entradas. Aceita 1-4 tokens; qualquer outra coisa é `MalformedValue` (0 tokens: nada pra
//     expandir; 5+: superespecificado, casa com o próprio check "Abort over-specified shorthand
//     values" do `PropertySpecification.cpp` do upstream pros tipos de shorthand que compartilham
//     o próprio laço genérico final dele).
bool expand_box(std::span<const std::string_view> targets, std::string_view raw_value,
                std::vector<LonghandValue>* out) {
  const std::vector<std::string_view> tokens = split_whitespace(raw_value);
  if (tokens.empty() || tokens.size() > 4) {
    return false;
  }
  // EN: `kBoxIndexTable` used to be a plain C-array of `std::array<int, 4>`
  //     (`std::array<int, 4> kBoxIndexTable[5]`); cppcheck 2.13/2.21 both confused that
  //     C-array's own outer size (5) with the inner element type's template argument (4)
  //     and reported this exact indexing as `containerOutOfBounds`, even though the guard
  //     just above already proves `tokens.size()` is in `[1, 4]`, well within the table's
  //     real 5 rows (CI-CPPCHECK-DIVERGENCIA, 2026-08-07 -- see that item's own TODO.md
  //     entry for the fuller story of why the CI and local gates disagreed on this
  //     finding). Wrapping the whole table in one `std::array<std::array<int, 4>, 5>`
  //     (nested-braces form below) gives cppcheck's own value-flow analysis a single
  //     aggregate type with an unambiguous `size()`, and the false positive is gone under
  //     both cppcheck 2.13.0 (CI) and 2.21.1 (measured locally) -- no suppression needed
  //     here anymore. Equivalent generated code, verified against the module's own green
  //     test suite (`uix_style_shorthand_expansion_sanity` exercises all 4 reachable Box
  //     token counts).
  // PT: `kBoxIndexTable` costumava ser um C-array puro de `std::array<int, 4>`
  //     (`std::array<int, 4> kBoxIndexTable[5]`); o cppcheck 2.13/2.21 confundiam o
  //     próprio tamanho externo desse C-array (5) com o argumento de template do
  //     tipo-elemento interno (4) e acusavam esta indexação exata de
  //     `containerOutOfBounds`, mesmo com a guarda logo acima já provando que
  //     `tokens.size()` está em `[1, 4]`, bem dentro das 5 linhas reais da tabela
  //     (CI-CPPCHECK-DIVERGENCIA, 2026-08-07 -- ver a própria entrada deste item no
  //     `TODO.md` pra história completa de por que o gate do CI e o local discordavam
  //     deste achado). Envolver a tabela inteira num único
  //     `std::array<std::array<int, 4>, 5>` (forma de chaves aninhadas abaixo) dá à
  //     análise de value-flow do cppcheck um único tipo agregado com `size()`
  //     inequívoco, e o falso positivo some tanto sob o cppcheck 2.13.0 (CI) quanto o
  //     2.21.1 (medido localmente) -- nenhuma supressão necessária aqui mais. Código
  //     gerado equivalente, verificado contra a própria suíte de teste verde do módulo
  //     (`uix_style_shorthand_expansion_sanity` exercita as 4 contagens de token Box
  //     alcançáveis).
  const std::array<int, 4>& idx = kBoxIndexTable[tokens.size()];
  for (std::size_t i = 0; i < 4; ++i) {
    out->push_back({targets[i], tokens[static_cast<std::size_t>(idx[i])]});
  }
  return true;
}

// EN: `Replicate` algorithm (docs/uix-rcss.md section 6.2). `targets` must have exactly 2
//     entries. Accepts 1 token (both targets share it) or 2 (each gets its own); anything else is
//     `MalformedValue`.
// PT: Algoritmo `Replicate` (seção 6.2 do docs/uix-rcss.md). `targets` precisa ter exatamente 2
//     entradas. Aceita 1 token (os dois alvos compartilham) ou 2 (cada um recebe o próprio);
//     qualquer outra coisa é `MalformedValue`.
bool expand_replicate(std::span<const std::string_view> targets, std::string_view raw_value,
                      std::vector<LonghandValue>* out) {
  const std::vector<std::string_view> tokens = split_whitespace(raw_value);
  if (tokens.size() == 1) {
    out->push_back({targets[0], tokens[0]});
    out->push_back({targets[1], tokens[0]});
    return true;
  }
  if (tokens.size() == 2) {
    out->push_back({targets[0], tokens[0]});
    out->push_back({targets[1], tokens[1]});
    return true;
  }
  return false;
}

// EN: `FallThrough`/`Flex`-shared routing loop -- see this file's header, "The two routing
//     classifiers" and "The kCatchAll classifier", for the design this implements. Mirrors
//     `PropertySpecification.cpp:429-471`'s own outer `for` loop exactly: try `items[item_index]`
//     against `tokens[value_index]`; on acceptance, claim it (emit, advance BOTH indices); on
//     rejection, advance only `item_index` (retry the SAME token against the next item). Two
//     terminal conditions: tokens exhausted (success path, defaults fill any never-claimed item
//     that HAS one; `MalformedValue` if any never-claimed item has none -- this is what makes
//     plain `FallThrough` shorthands, `has_default = false` on every item, require EXACTLY
//     `items.size()` tokens); items exhausted with tokens still unclaimed (`MalformedValue`,
//     upstream's own "no more properties to pass them to" abort).
// PT: Laço de roteamento compartilhado por `FallThrough`/`Flex` -- ver "Os dois classificadores de
//     roteamento" e "O classificador kCatchAll" no cabeçalho deste arquivo pro desenho que isto
//     implementa. Espelha exatamente o próprio laço `for` externo do
//     `PropertySpecification.cpp:429-471`: tenta `items[item_index]` contra
//     `tokens[value_index]`; em caso de aceitação, reivindica (emite, avança OS DOIS índices); em
//     caso de rejeição, avança só `item_index` (reitera o MESMO token contra o próximo item). Duas
//     condições terminais: tokens esgotados (caminho de sucesso, defaults preenchem todo item
//     nunca-reivindicado que TEM um; `MalformedValue` se algum item nunca-reivindicado não tem
//     nenhum -- isto é o que faz shorthands `FallThrough` simples, `has_default = false` em todo
//     item, exigirem EXATAMENTE `items.size()` tokens); itens esgotados com tokens ainda não
//     reivindicados (`MalformedValue`, o próprio abort "no more properties to pass them to" do
//     upstream).
bool expand_fallthrough(std::span<const FallthroughItem> items, std::string_view raw_value,
                        std::vector<LonghandValue>* out) {
  const std::vector<std::string_view> tokens = split_whitespace(raw_value);
  if (tokens.empty() || tokens.size() > items.size()) {
    return false;
  }

  std::vector<bool> claimed(items.size(), false);
  std::vector<std::string_view> claimed_value(items.size());
  std::size_t value_index = 0;
  std::size_t item_index = 0;
  while (value_index < tokens.size() && item_index < items.size()) {
    if (items[item_index].accepts(tokens[value_index])) {
      claimed[item_index] = true;
      claimed_value[item_index] = tokens[value_index];
      ++value_index;
    }
    ++item_index;
  }

  if (value_index < tokens.size()) {
    return false; // over-specified: tokens remain, no item left to claim them
  }

  for (std::size_t i = 0; i < items.size(); ++i) {
    if (claimed[i]) {
      out->push_back({items[i].name, claimed_value[i]});
    } else if (items[i].has_default) {
      out->push_back({items[i].name, items[i].default_value});
    } else {
      return false; // under-specified, and this item has no default to fall back on
    }
  }
  return true;
}

} // namespace

bool is_shorthand(std::string_view name) {
  return find_shorthand_info(name) != nullptr;
}

std::span<const ShorthandDescriptor> all_shorthands() {
  static constexpr ShorthandDescriptor kDescriptors[] = {
      {"margin"},
      {"padding"},
      {"border-radius"},
      {"border-color"},
      {"border-top"},
      {"border-right"},
      {"border-bottom"},
      {"border-left"},
      {"border"},
      {"background"},
      {"gap"},
      {"overflow"},
      {"flex"},
  };
  return std::span<const ShorthandDescriptor>(kDescriptors,
                                              sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}

ShorthandExpandStatus expand_shorthand(std::string_view shorthand_name, std::string_view raw_value,
                                       std::vector<LonghandValue>* out) {
  out->clear();

  const ShorthandInfo* sh = find_shorthand_info(shorthand_name);
  if (sh == nullptr) {
    return ShorthandExpandStatus::UnknownShorthand;
  }

  switch (sh->algorithm) {
    case Algorithm::Box:
      return expand_box(sh->box_or_replicate_or_repeat_targets, raw_value, out)
                 ? ShorthandExpandStatus::Ok
                 : ShorthandExpandStatus::MalformedValue;

    case Algorithm::Replicate:
      return expand_replicate(sh->box_or_replicate_or_repeat_targets, raw_value, out)
                 ? ShorthandExpandStatus::Ok
                 : ShorthandExpandStatus::MalformedValue;

    case Algorithm::FallThrough:
      return expand_fallthrough(sh->fallthrough_items, raw_value, out)
                 ? ShorthandExpandStatus::Ok
                 : ShorthandExpandStatus::MalformedValue;

    case Algorithm::Flex: {
      // EN: `PropertySpecification.cpp:315-318`'s own literal `property_values[0] == "none"`
      //     special case -- checked BEFORE the generic routing loop, exactly like upstream, and
      //     bypasses it entirely (the "none" case is not "3 tokens that happen to route
      //     positionally", it is a DIFFERENT source form entirely, verbatim `{"0", "0", "auto"}`,
      //     NOT the registry's own normal initial values -- see shorthand_expansion_sanity.cpp's
      //     own `test_flex_none_expands_to_0_0_auto` for why `flex-shrink` genuinely differs from
      //     its own registry initial `1`).
      // PT: O próprio caso especial `property_values[0] == "none"` literal do
      //     `PropertySpecification.cpp:315-318` -- checado ANTES do laço de roteamento genérico,
      //     exatamente como o upstream, e o contorna por completo (o caso "none" não é "3 tokens
      //     que calham de rotear posicionalmente", é uma forma-fonte inteiramente DIFERENTE,
      //     `{"0", "0", "auto"}` verbatim, NÃO os próprios valores iniciais normais do registro --
      //     ver o próprio `test_flex_none_expands_to_0_0_auto` do shorthand_expansion_sanity.cpp
      //     pro porquê `flex-shrink` genuinamente diverge do próprio inicial de registro `1`).
      const std::vector<std::string_view> tokens = split_whitespace(raw_value);
      if (tokens.size() == 1 && tokens[0] == "none") {
        out->push_back({"flex-grow", "0"});
        out->push_back({"flex-shrink", "0"});
        out->push_back({"flex-basis", "auto"});
        return ShorthandExpandStatus::Ok;
      }
      return expand_fallthrough(sh->fallthrough_items, raw_value, out)
                 ? ShorthandExpandStatus::Ok
                 : ShorthandExpandStatus::MalformedValue;
    }

    case Algorithm::RecursiveRepeat: {
      // EN: `border`'s own citation, docs/uix-rcss.md section 6.2: "the same 2-token value string
      //     is fed to all 4 side-shorthands verbatim". Each sub-expansion is a full,
      //     independent `FallThrough` dispatch (this function recurses) -- if ANY of the 4 fails,
      //     the whole `border` expansion fails (matches upstream's own `result &= ...` across all
      //     4, `PropertySpecification.cpp:369-380`).
      // PT: A própria citação de `border`, seção 6.2 do docs/uix-rcss.md: "o mesmo texto de valor
      //     de 2 tokens é alimentado aos 4 shorthands de lado verbatim". Cada sub-expansão é um
      //     dispatch `FallThrough` completo, independente (esta função recursa) -- se QUALQUER um
      //     dos 4 falhar, a expansão inteira de `border` falha (casa com o próprio
      //     `result &= ...` do upstream nos 4, `PropertySpecification.cpp:369-380`).
      for (std::string_view sub_name : sh->box_or_replicate_or_repeat_targets) {
        std::vector<LonghandValue> sub_out;
        if (expand_shorthand(sub_name, raw_value, &sub_out) != ShorthandExpandStatus::Ok) {
          out->clear();
          return ShorthandExpandStatus::MalformedValue;
        }
        out->insert(out->end(), sub_out.begin(), sub_out.end());
      }
      return ShorthandExpandStatus::Ok;
    }
  }
  return ShorthandExpandStatus::UnknownShorthand; // unreachable, silences -Wreturn-type
}

} // namespace glintfx::uix::style
