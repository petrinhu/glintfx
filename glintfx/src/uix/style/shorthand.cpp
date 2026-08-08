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

// EN: `ESC-2`'s own quote marker set -- see `split_whitespace`'s own doc-comment below for what
//     this buys.
// PT: O próprio conjunto de marcador de aspa da `ESC-2` -- ver o próprio comentário de doc do
//     `split_whitespace` abaixo pro que isto compra.
constexpr bool is_quote(char c) {
  return c == '"' || c == '\'';
}

// EN: Splits `s` on whitespace RUNS (one or more consecutive whitespace bytes collapse to a
//     single separator), views into `s` itself -- zero-copy, same "Token fields reference the
//     original source buffer" precedent lexer.hpp's own header states. **`ESC-2` addendum:**
//     quote-aware for the one target among the 20 shorthands whose accepted grammar needs it
//     (`font`'s own `font-family` item, a `string`-domain value that may contain internal
//     whitespace, e.g. `"Times New Roman"`) -- a double- or single-quoted run collapses to ONE
//     token, its own surrounding quote byte excluded, internal whitespace preserved, mirroring
//     upstream's real `ParsePropertyValues` (`PropertySpecification.cpp:513-682`, Whitespace mode)
//     for every SHAPE/BOUNDARY decision that function makes:
//       - Opening a quote FLUSHES whatever plain run had been accumulating right before it (no
//         whitespace needed between the two, `:577-589`) -- `16px"ABC"` tokenizes as `["16px",
//         "ABC"]`, not one glued token, matching `SubmitValue()`'s own call at quote-open.
//       - A backslash INSIDE a quote protects the next byte from ending the string early --
//         `"Lato\"Sans"` is ONE token, the embedded `\"` does not close it early (`:645-648`'s own
//         `VALUE_QUOTE_ESCAPE_NEXT` transition) -- boundary/count parity with upstream is exact.
//       - An unterminated quote silently discards everything from the opening quote to the end of
//         `s` (no token for that attempt, and nothing after it either, since there is no closing
//         byte left to resume scanning from) -- `:672-679`'s own post-loop `if (state == VALUE)`
//         (never true for a dangling `VALUE_QUOTE`/`VALUE_QUOTE_ESCAPE_NEXT`) plus the
//         whole-value `Error` a fully-empty result would trigger.
//       - An empty quoted run (`""`) IS pushed as its own (empty) token -- `:637`'s own
//         `SubmitExactValue()` submits unconditionally, unlike `SubmitValue()`'s own
//         empty-after-strip filter the plain (non-quoted) run below still uses.
//     **Named, deliberate divergence from upstream's OWN byte CONTENT (not its boundary/count
//     decisions):** this function does NOT interpret the escape -- `\"`/`\'`/`\\` stay LITERAL
//     (backslash and all) in the emitted token, rather than collapsing to the bare escaped
//     character the way upstream's own `value += character;` (`:657-660`) does. Reason, not a
//     shortcut: `LonghandValue::value` (this file's own header, `LonghandValue`'s own doc-comment)
//     is a `std::string_view` that must point EITHER into the caller's own `raw_value` buffer OR a
//     literal of static storage duration -- an interpreted escape (removing the `\` byte) is
//     neither: it is no longer a contiguous slice of `raw_value`, so honoring it would need memory
//     this call synthesizes, and the only safe home for that memory (given this repo's own "no
//     mutable global singleton" discipline, and the real caller lifetime pattern --
//     `parser.cpp`'s own `apply_declaration` copies the *view*, not the bytes, into a long-lived
//     `PropertyDeclaration` that outlives many LATER `expand_shorthand` calls) would either dangle
//     (a cleared/reused arena) or leak unboundedly (a never-cleared one) -- both rejected. This
//     costs nothing for the corpus/spec's own explicitly-cited case (`font: 16px "Times New
//     Roman"` has no escape at all), and this repo's own `lexer.hpp` already sets the precedent of
//     deferring quote-STRIPPING itself to "a future, semantic parser step" (`lexer.hpp:57-63`) --
//     deferring escape INTERPRETATION the same way, while still tracking escape BOUNDARIES
//     faithfully, is the same discipline one layer further out, not a new one. See
//     `shorthand_expansion_sanity.cpp`'s own `test_font_fallthrough_quoted_forms`, sub-case
//     "escape kept literal", for the exact, asserted behaviour.
// PT: Divide `s` por TRECHOS de whitespace (um ou mais bytes de whitespace consecutivos colapsam
//     num único separador), views sobre o próprio `s` -- zero-cópia, mesmo precedente "campos de
//     Token referenciam o próprio buffer-fonte original" que o próprio cabeçalho do lexer.hpp
//     declara. **Adendo da `ESC-2`:** ciente de aspas pro único alvo, dentre os 20 shorthands, cuja
//     própria gramática aceita precisa disso (o próprio item `font-family` do `font`, um valor de
//     domínio `string` que pode conter whitespace interno, ex. `"Times New Roman"`) -- um trecho
//     entre aspas duplas ou simples colapsa num ÚNICO token, o próprio byte de aspa externa
//     excluído, whitespace interno preservado, espelhando o próprio `ParsePropertyValues` real do
//     upstream (`PropertySpecification.cpp:513-682`, modo Whitespace) pra toda decisão de
//     FORMA/FRONTEIRA que aquela função toma:
//       - Abrir uma aspa DÁ FLUSH no que quer que um trecho plano estivesse acumulando bem antes
//         dela (sem precisar de whitespace entre os dois, `:577-589`) -- `16px"ABC"` tokeniza como
//         `["16px", "ABC"]`, não um token colado só, casando com a própria chamada do
//         `SubmitValue()` na abertura de aspa.
//       - Uma barra invertida DENTRO de uma aspa protege o próximo byte de fechar a string cedo
//         demais -- `"Lato\"Sans"` é UM token só, o `\"` embutido não a fecha cedo (a própria
//         transição `VALUE_QUOTE_ESCAPE_NEXT` de `:645-648`) -- paridade de fronteira/contagem com
//         o upstream é exata.
//       - Uma aspa não-terminada descarta em silêncio tudo da aspa de abertura até o fim de `s`
//         (nenhum token pra esta tentativa, e nada depois dela também, já que não sobra byte de
//         fechamento pra retomar o scan) -- o próprio `if (state == VALUE)` pós-laço de `:672-679`
//         (nunca verdadeiro pra um `VALUE_QUOTE`/`VALUE_QUOTE_ESCAPE_NEXT` pendurado) mais o
//         `Error` de valor-inteiro que um resultado totalmente vazio dispararia.
//       - Um trecho entre aspas vazio (`""`) É empurrado como o próprio token (vazio) dele -- o
//         próprio `SubmitExactValue()` de `:637` submete incondicionalmente, diferente do próprio
//         filtro vazio-depois-de-stripar do `SubmitValue()` que o trecho plano (não-aspado) abaixo
//         ainda usa.
//     **Divergência nomeada, deliberada, do próprio BYTE de conteúdo do upstream (não das próprias
//     decisões de fronteira/contagem dele):** esta função NÃO interpreta o escape -- `\"`/`\'`/`\\`
//     ficam LITERAIS (barra invertida e tudo) no token emitido, em vez de colapsar pro próprio
//     caractere escapado nu do jeito que o próprio `value += character;` (`:657-660`) do upstream
//     faz. Motivo, não atalho: `LonghandValue::value` (o próprio comentário de doc de
//     `LonghandValue` deste arquivo) é um `std::string_view` que precisa apontar OU pro próprio
//     buffer `raw_value` do chamador OU pra um literal de duração de armazenamento estática -- um
//     escape interpretado (removendo o byte `\`) não é nenhum dos dois: não é mais uma fatia
//     contígua de `raw_value`, então honrá-lo precisaria de memória que esta chamada sintetiza, e o
//     único lar seguro pra essa memória (dada a própria disciplina "sem singleton mutável global"
//     deste repo, e o padrão real de lifetime do chamador -- o próprio `apply_declaration` do
//     parser.cpp copia a *view*, não os bytes, pra um `PropertyDeclaration` de vida longa que
//     sobrevive a MUITAS chamadas POSTERIORES de `expand_shorthand`) ou penduraria (uma arena
//     limpa/reusada) ou vazaria sem teto (uma nunca-limpa) -- os dois rejeitados. Isto não custa
//     nada pro próprio caso explicitamente citado pelo corpus/spec (`font: 16px "Times New Roman"`
//     não tem escape nenhum), e o próprio `lexer.hpp` deste repo já fixa o precedente de adiar o
//     próprio DESPIR-de-aspas pra "um futuro passo semântico de parser" (`lexer.hpp:57-63`) --
//     adiar a INTERPRETAÇÃO do escape do mesmo jeito, ainda rastreando a FRONTEIRA do escape
//     fielmente, é a mesma disciplina uma camada mais adiante, não uma nova. Ver o próprio
//     `test_font_fallthrough_quoted_forms` do shorthand_expansion_sanity.cpp, subcaso "escape kept
//     literal", pro comportamento exato, testado.
std::vector<std::string_view> split_whitespace(std::string_view s) {
  std::vector<std::string_view> tokens;
  std::size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && is_whitespace(s[i])) {
      ++i;
    }
    if (i >= s.size()) {
      break;
    }
    if (is_quote(s[i])) {
      const char open_quote = s[i];
      const std::size_t content_start = i + 1;
      std::size_t j = content_start;
      bool terminated = false;
      while (j < s.size()) {
        if (s[j] == open_quote) {
          terminated = true;
          break;
        }
        if (s[j] == '\\' && j + 1 < s.size()) {
          j += 2; // escaped pair kept verbatim; an escaped quote does not close the run
          continue;
        }
        ++j;
      }
      if (!terminated) {
        break; // unterminated: discard the dangling tail silently, nothing follows it either
      }
      tokens.push_back(s.substr(content_start, j - content_start));
      i = j + 1; // past the closing quote
      continue;
    }
    const std::size_t start = i;
    while (i < s.size() && !is_whitespace(s[i]) && !is_quote(s[i])) {
      ++i;
    }
    tokens.push_back(s.substr(start, i - start));
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

// EN: `ESC-2` classifiers -- `font`'s own style/weight items and `flex-flow`'s own
//     direction/wrap items are CLOSED keyword sets (verbatim from
//     `StyleSheetSpecification.cpp:354-355` / `:421,425`), unlike `border-{side}`'s open,
//     shape-only `looks_like_length_token` (`font-family`/`border-*-color` stay `kCatchAll`,
//     their own real domain is genuinely open -- "string"/"color"). An exact-set classifier is
//     NOT this file's own "value validation" the header's "Scope" paragraph defers to a later
//     slice -- it is still a ROUTING decision (which of two/three/four candidate DOMAINS a
//     token's own literal spelling suggests), the same kind `looks_like_number_token`/
//     `looks_like_length_token` already make; the set is simply closed (a keyword domain)
//     instead of open (a shape domain), so membership IS the routing test, there is no narrower
//     shape check possible for a keyword-only property the way there would be for a
//     length/number one.
// PT: Classificadores da `ESC-2` -- os próprios itens style/weight do `font` e direction/wrap do
//     `flex-flow` são conjuntos de palavra-chave FECHADOS (verbatim do
//     `StyleSheetSpecification.cpp:354-355` / `:421,425`), diferente do `looks_like_length_token`
//     aberto, só-de-forma, do `border-{side}` (`font-family`/`border-*-color` seguem `kCatchAll`,
//     o próprio domínio real deles é genuinamente aberto -- "string"/"color"). Um classificador de
//     conjunto exato NÃO é a própria "validação de valor" deste arquivo que o parágrafo "Escopo"
//     do cabeçalho adia pra uma fatia posterior -- ainda é uma decisão de ROTEAMENTO (a qual de
//     dois/três/quatro DOMÍNIOS candidatos a própria grafia literal de um token sugere), o mesmo
//     tipo que `looks_like_number_token`/`looks_like_length_token` já fazem; o conjunto é só
//     fechado (um domínio palavra-chave) em vez de aberto (um domínio de forma), então
//     pertencimento É o próprio teste de roteamento, não há checagem de forma mais estreita
//     possível pra uma propriedade só-palavra-chave do jeito que haveria pra uma de
//     comprimento/número.
bool is_font_style_token(std::string_view t) {
  return t == "normal" || t == "italic";
}

bool is_font_weight_token(std::string_view t) {
  return t == "normal" || t == "bold" || looks_like_number_token(t);
}

bool is_flex_direction_token(std::string_view t) {
  return t == "row" || t == "row-reverse" || t == "column" || t == "column-reverse";
}

bool is_flex_wrap_token(std::string_view t) {
  return t == "nowrap" || t == "wrap" || t == "wrap-reverse";
}

// EN: `perspective-origin`/`transform-origin`'s own `-x`/`-y` items: a 2-way domain,
//     `keyword(...)` OR `length_percent` (`StyleSheetSpecification.cpp:390-391,394-395`) --
//     `looks_like_length_token` alone already covers the length_percent half (a `%` suffix is
//     just another numeric-shaped token, same reasoning this file's header already gives for why
//     that classifier accepts "ANYTHING numeric-shaped, with or without a unit suffix").
//     `transform-origin-z`'s own domain is plain `length`, no keyword half at all (`:396`) -- it
//     reuses `looks_like_length_token` directly, no new classifier needed.
// PT: Os próprios itens `-x`/`-y` de `perspective-origin`/`transform-origin`: um domínio de 2
//     vias, `keyword(...)` OU `length_percent` (`StyleSheetSpecification.cpp:390-391,394-395`) --
//     o `looks_like_length_token` sozinho já cobre a metade length_percent (um sufixo `%` é só
//     mais um token de forma numérica, o mesmo raciocínio que o cabeçalho deste arquivo já dá pro
//     porquê aquele classificador aceitar "QUALQUER COISA com forma numérica, com ou sem sufixo de
//     unidade"). O próprio domínio de `transform-origin-z` é `length` puro, sem metade
//     palavra-chave nenhuma (`:396`) -- reusa `looks_like_length_token` direto, nenhum
//     classificador novo precisa.
bool is_origin_x_token(std::string_view t) {
  return t == "left" || t == "center" || t == "right" || looks_like_length_token(t);
}

bool is_origin_y_token(std::string_view t) {
  return t == "top" || t == "center" || t == "bottom" || looks_like_length_token(t);
}

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

// EN: `ESC-2`'s own 4 new `FallThrough` chains, `StyleSheetSpecification.cpp:359,392,397,429`.
//     `has_default = false` on EVERY item here (none of the 4 shares `flex`'s own
//     `default_omitted_values` mechanism -- `PropertySpecification.cpp:320-334` names that
//     mechanism `Flex`-only, and none of `font`/`perspective-origin`/`transform-origin`/
//     `flex-flow`'s own real `RegisterShorthand(...)` call sites is `ShorthandType::Flex`) -- a
//     never-claimed item here is OMITTED from `expand_shorthand`'s own output, per
//     `expand_fallthrough`'s own doc-comment below (`UIX-RCSS-ERRATA-8`).
// PT: As próprias 4 cadeias `FallThrough` novas da `ESC-2`,
//     `StyleSheetSpecification.cpp:359,392,397,429`. `has_default = false` em TODO item aqui
//     (nenhuma das 4 compartilha o próprio mecanismo `default_omitted_values` do `flex` --
//     `PropertySpecification.cpp:320-334` nomeia aquele mecanismo só-de-`Flex`, e nenhum dos
//     próprios call sites reais `RegisterShorthand(...)` de `font`/`perspective-origin`/
//     `transform-origin`/`flex-flow` é `ShorthandType::Flex`) -- um item nunca-reivindicado aqui é
//     OMITIDO da própria saída do `expand_shorthand`, per o próprio comentário de doc do
//     `expand_fallthrough` abaixo (`UIX-RCSS-ERRATA-8`).
constexpr FallthroughItem kFontItems[4] = {
    {"font-style", is_font_style_token, false, {}},
    {"font-weight", is_font_weight_token, false, {}},
    {"font-size", looks_like_length_token, false, {}},
    {"font-family", kCatchAll, false, {}},
};

constexpr FallthroughItem kPerspectiveOriginItems[2] = {
    {"perspective-origin-x", is_origin_x_token, false, {}},
    {"perspective-origin-y", is_origin_y_token, false, {}},
};

constexpr FallthroughItem kTransformOriginItems[3] = {
    {"transform-origin-x", is_origin_x_token, false, {}},
    {"transform-origin-y", is_origin_y_token, false, {}},
    {"transform-origin-z", looks_like_length_token, false, {}},
};

constexpr FallthroughItem kFlexFlowItems[2] = {
    {"flex-direction", is_flex_direction_token, false, {}},
    {"flex-wrap", is_flex_wrap_token, false, {}},
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

// EN: `ESC-2`'s own 3 new `Box` target arrays, upstream's own registered per-shorthand order
//     (`StyleSheetSpecification.cpp:286,313,382`). `kInsetTargets` names the 4 longhands
//     `top`/`right`/`bottom`/`left` THEMSELVES (not `inset-*`) -- upstream's own shorthand
//     registration literally lists those bare names as the shorthand's own constituents;
//     `kNavTargets` is the SIDE order (up/right/down/left), pinned distinctly from
//     `kBorderRadiusTargets`'s own CORNER order by `test_nav_box_expansion_up_right_down_left_order`
//     (shorthand_expansion_sanity.cpp), same "order is load-bearing, not incidental" discipline
//     the comment above `kBorderRadiusTargets` already states.
// PT: Os próprios 3 arrays de alvo `Box` novos da `ESC-2`, na própria ordem registrada do upstream
//     por-shorthand (`StyleSheetSpecification.cpp:286,313,382`). `kInsetTargets` nomeia os 4
//     longhands `top`/`right`/`bottom`/`left` ELES MESMOS (não `inset-*`) -- o próprio registro de
//     shorthand do upstream lista literalmente esses nomes nus como os próprios constituintes do
//     shorthand; `kNavTargets` é a ordem de LADO (up/right/down/left), pinada distinta da própria
//     ordem de CANTO do `kBorderRadiusTargets` pelo próprio
//     `test_nav_box_expansion_up_right_down_left_order` (shorthand_expansion_sanity.cpp), mesma
//     disciplina "ordem é load-bearing, não incidental" que o próprio comentário acima do
//     `kBorderRadiusTargets` já declara.
constexpr std::string_view kBorderWidthTargets[4] = {"border-top-width", "border-right-width",
                                                     "border-bottom-width", "border-left-width"};
constexpr std::string_view kInsetTargets[4] = {"top", "right", "bottom", "left"};
constexpr std::string_view kNavTargets[4] = {"nav-up", "nav-right", "nav-down", "nav-left"};

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

// EN: The 20-entry shorthand table (13 original + `ESC-2`'s own +7), docs/uix-rcss.md section
//     6.2's own table restated as data. Order here is the table's own row order, not alphabetical
//     (unlike property_registry.cpp's own table -- this one is never dumped per docs/uix-rcss.md
//     section 3's own sort-order contract, which applies to LONGHAND `PROP` lines only, so no
//     ordering requirement applies here) -- the 7 new rows are appended at the end, per that same
//     "no ordering contract" reasoning.
// PT: A tabela de 20 shorthands (13 originais + os +7 da própria `ESC-2`), a própria tabela da
//     seção 6.2 do docs/uix-rcss.md restatada como dado. A ordem aqui é a própria ordem de linha
//     da tabela, não alfabética (diferente da própria tabela do property_registry.cpp -- esta
//     nunca é dumpada per o próprio contrato de ordem-de-sort da seção 3 do docs/uix-rcss.md, que
//     se aplica só a linhas `PROP` de LONGHAND, então nenhum requisito de ordenação se aplica
//     aqui) -- as 7 linhas novas são acrescentadas no fim, pelo mesmo raciocínio "sem contrato de
//     ordem".
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
    {"border-width", Algorithm::Box, kBorderWidthTargets, {}},
    {"inset", Algorithm::Box, kInsetTargets, {}},
    {"nav", Algorithm::Box, kNavTargets, {}},
    {"font", Algorithm::FallThrough, {}, kFontItems},
    {"perspective-origin", Algorithm::FallThrough, {}, kPerspectiveOriginItems},
    {"transform-origin", Algorithm::FallThrough, {}, kTransformOriginItems},
    {"flex-flow", Algorithm::FallThrough, {}, kFlexFlowItems},
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
//     terminal conditions: items exhausted with tokens still unclaimed (`MalformedValue`,
//     upstream's own "no more properties to pass them to" abort -- the `if (value_index <
//     tokens.size()) return false;` guard right after this loop); tokens exhausted (success path
//     -- every CLAIMED item emits its own token; a never-claimed item that HAS a default emits
//     it; a never-claimed item with NO default is simply OMITTED from `*out`).
//     **`UIX-RCSS-ERRATA-8` (`ESC-2`, docs/uix-rcss.md):** that last case used to `return false`
//     (`MalformedValue`) instead of omitting -- an implicit "every item MUST be claimed" doctrine
//     that does NOT match upstream's own real loop, traced directly (`:433-471`, not paraphrased):
//     upstream's own loop has no post-loop "was every item visited" check at all -- an item the
//     token cursor never reaches simply never gets its own `dictionary.SetProperty()` call, the
//     same as any OTHER item this function's own `while` condition above never visits because
//     `item_index` never reaches it while tokens remain unconsumed. Concretely, `border-top: 2px`
//     (only a width token, `-color` never visited) is a SUCCESS upstream, claiming only
//     `border-top-width` -- `border-top-color` is left for the cascade to resolve from its own
//     §6.1 registry initial or an inherited value, exactly the same as any OTHER longhand this
//     declaration simply never mentions. Sub-specification is not an edge case for a real
//     shorthand's grammar, either -- `font: 16px LatoLatin` (style/weight both omitted) is the
//     NORMAL way to author `font`, not a hostile input, which is why the pre-`ESC-2` doctrine
//     would have made all 4 of `ESC-2`'s own new `FallThrough` shorthands unusable for their own
//     most common real forms. Pinned by `shorthand_expansion_sanity.cpp`'s own
//     `test_border_top_sub_specified_values_are_ok_not_malformed` and the sub-specified sub-cases
//     of `test_font_fallthrough_unquoted_forms`. **Unaffected by this correction:** the
//     over-specified / reversed-order guard immediately below this loop
//     (`test_border_top_fallthrough_order_is_load_bearing` still pins `#7A5A2E 1dp` as
//     `MalformedValue` -- a token left unclaimed with no item left to try is still a hard failure,
//     upstream's own `PropertySpecification.cpp:469-471` guard, untouched here); Decision 1 /
//     `UIX-RCSS-ERRATA-5`'s own atomic-discard-on-REJECTION semantics (docs/uix-rcss.md section
//     14.1) -- that ledger entry is about what happens when a shorthand is REJECTED (Side A writes
//     a partial match before failing, Side B discards atomically and keeps that discard,
//     deliberately, by líder decision); this correction is about what happens when a shorthand
//     ACCEPTS a sub-specified value, a different, unrelated code path this ledger entry does not
//     name and this correction does not touch.
// PT: Laço de roteamento compartilhado por `FallThrough`/`Flex` -- ver "Os dois classificadores de
//     roteamento" e "O classificador kCatchAll" no cabeçalho deste arquivo pro desenho que isto
//     implementa. Espelha exatamente o próprio laço `for` externo do
//     `PropertySpecification.cpp:429-471`: tenta `items[item_index]` contra
//     `tokens[value_index]`; em caso de aceitação, reivindica (emite, avança OS DOIS índices); em
//     caso de rejeição, avança só `item_index` (reitera o MESMO token contra o próximo item). Duas
//     condições terminais: itens esgotados com tokens ainda não reivindicados (`MalformedValue`, o
//     próprio abort "no more properties to pass them to" do upstream -- a própria guarda
//     `if (value_index < tokens.size()) return false;` logo depois deste laço); tokens esgotados
//     (caminho de sucesso -- todo item CLAIMED emite o próprio token; um item nunca-reivindicado
//     que TEM um default emite ele; um item nunca-reivindicado SEM default é simplesmente OMITIDO
//     de `*out`).
//     **`UIX-RCSS-ERRATA-8` (`ESC-2`, docs/uix-rcss.md):** aquele último caso costumava dar
//     `return false` (`MalformedValue`) em vez de omitir -- uma doutrina implícita "todo item TEM
//     de ser reivindicado" que NÃO bate com o próprio laço real do upstream, rastreada direto
//     (`:433-471`, não parafraseada): o próprio laço do upstream não tem checagem pós-laço nenhuma
//     de "todo item foi visitado" -- um item que o cursor de token nunca alcança simplesmente nunca
//     recebe a própria chamada `dictionary.SetProperty()` dele, o mesmo que qualquer OUTRO item que
//     a própria condição `while` desta função acima nunca visita porque `item_index` nunca o
//     alcança enquanto sobram tokens não-consumidos. Concretamente, `border-top: 2px` (só um token
//     de width, `-color` nunca visitado) é um SUCESSO no upstream, reivindicando só
//     `border-top-width` -- `border-top-color` fica pra cascata resolver do próprio valor inicial
//     de registro da §6.1 ou um valor herdado, exatamente como qualquer OUTRO longhand que esta
//     declaração simplesmente nunca menciona. Sub-especificação também não é caso de borda pra
//     gramática de um shorthand real -- `font: 16px LatoLatin` (style/weight os dois omitidos) é o
//     jeito NORMAL de autorar `font`, não um input hostil, que é por isso a doutrina pré-`ESC-2`
//     teria deixado os 4 shorthands `FallThrough` novos da própria `ESC-2` inutilizáveis pra própria
//     forma mais comum real deles. Pinado pelo próprio
//     `test_border_top_sub_specified_values_are_ok_not_malformed` e pelos subcasos
//     sub-especificados do `test_font_fallthrough_unquoted_forms`, os dois do
//     shorthand_expansion_sanity.cpp. **Não afetado por esta correção:** a própria guarda de
//     over-specified / ordem-revertida logo abaixo deste laço (o próprio
//     `test_border_top_fallthrough_order_is_load_bearing` continua pinando `#7A5A2E 1dp` como
//     `MalformedValue` -- um token deixado não-reivindicado sem item nenhum sobrando pra tentar
//     continua sendo falha dura, a própria guarda `PropertySpecification.cpp:469-471` do upstream,
//     intocada aqui); as próprias semânticas de descarte-atômico-na-REJEIÇÃO da `UIX-RCSS-ERRATA-5`
//     / Decisão 1 (seção 14.1 do docs/uix-rcss.md) -- aquela entrada do ledger é sobre o que
//     acontece quando um shorthand é REJEITADO (o lado A escreve um casamento parcial antes de
//     falhar, o lado B descarta atomicamente e mantém esse descarte, deliberadamente, por decisão
//     do líder); esta correção é sobre o que acontece quando um shorthand ACEITA um valor
//     sub-especificado, um caminho de código diferente, não-relacionado, que esta entrada do
//     ledger não nomeia e que esta correção não toca.
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
    }
    // EN: else: never-claimed, no default -- OMITTED, not `MalformedValue` (`UIX-RCSS-ERRATA-8`,
    //     `ESC-2`). See this function's own doc-comment above.
    // PT: else: nunca-reivindicado, sem default -- OMITIDO, não `MalformedValue`
    //     (`UIX-RCSS-ERRATA-8`, `ESC-2`). Ver o próprio comentário de doc desta função acima.
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
      {"border-width"},
      {"inset"},
      {"nav"},
      {"font"},
      {"perspective-origin"},
      {"transform-origin"},
      {"flex-flow"},
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
      //     its own registry initial `1`). **`ESC-2` sibling finding:** the check is
      //     `tokens[0] == "none"`, upstream's own real condition inspects ONLY index 0 -- it does
      //     NOT also require `property_values.size() == 1`; any trailing tokens after `"none"`
      //     are silently ignored (upstream never reads `property_values[1..]` once index 0
      //     matches). The pre-`ESC-2` `tokens.size() == 1 && ...` guard here rejected that real,
      //     upstream-accepted form (`flex: none 2`) as `MalformedValue` -- corrected to
      //     `!tokens.empty() && tokens[0] == "none"` (the `!empty()` half stays, replacing
      //     upstream's own implicit "there is always at least one token by the time this line
      //     runs" invariant, which this module's own tokenizer does not get for free the way
      //     upstream's `ParsePropertyValues` does -- that function's own `Error` on
      //     `values_list.empty()`, `:678-679`, runs BEFORE this point in upstream; here,
      //     `split_whitespace` can return an empty vector and `tokens[0]` would be
      //     undefined behaviour without this guard). See
      //     shorthand_expansion_sanity.cpp's own `test_flex_none_ignores_extra_tokens`.
      // PT: O próprio caso especial `property_values[0] == "none"` literal do
      //     `PropertySpecification.cpp:315-318` -- checado ANTES do laço de roteamento genérico,
      //     exatamente como o upstream, e o contorna por completo (o caso "none" não é "3 tokens
      //     que calham de rotear posicionalmente", é uma forma-fonte inteiramente DIFERENTE,
      //     `{"0", "0", "auto"}` verbatim, NÃO os próprios valores iniciais normais do registro --
      //     ver o próprio `test_flex_none_expands_to_0_0_auto` do shorthand_expansion_sanity.cpp
      //     pro porquê `flex-shrink` genuinamente diverge do próprio inicial de registro `1`).
      //     **Achado irmão da `ESC-2`:** a checagem é `tokens[0] == "none"`, a própria condição
      //     real do upstream inspeciona SÓ o índice 0 -- ela NÃO exige também
      //     `property_values.size() == 1`; quaisquer tokens finais depois de `"none"` são
      //     ignorados em silêncio (o upstream nunca lê `property_values[1..]` uma vez que o índice
      //     0 casa). A própria guarda `tokens.size() == 1 && ...` pré-`ESC-2` daqui rejeitava essa
      //     forma real, aceita pelo upstream (`flex: none 2`), como `MalformedValue` --
      //     consertada pra `!tokens.empty() && tokens[0] == "none"` (a metade `!empty()` fica,
      //     substituindo o próprio invariante implícito "sempre sobra pelo menos um token quando
      //     esta linha roda" do upstream, que a própria tokenização deste módulo não ganha de
      //     graça do jeito que o próprio `ParsePropertyValues` do upstream ganha -- o próprio
      //     `Error` daquela função em `values_list.empty()`, `:678-679`, roda ANTES deste ponto no
      //     upstream; aqui, `split_whitespace` pode retornar um vetor vazio e `tokens[0]` seria
      //     comportamento indefinido sem esta guarda). Ver o próprio
      //     `test_flex_none_ignores_extra_tokens` do shorthand_expansion_sanity.cpp.
      const std::vector<std::string_view> tokens = split_whitespace(raw_value);
      if (!tokens.empty() && tokens[0] == "none") {
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
