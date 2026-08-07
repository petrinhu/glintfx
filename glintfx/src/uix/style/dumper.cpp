// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-DUMP-B -- implementation. See dumper.hpp's own header comment for the full scope/
//     boundary/design-decision rationale this file holds itself to; the comments here are about
//     HOW, not WHY at the file-scope level (same discipline the DOM sibling's own dumper.cpp
//     follows) -- but the ONE new decision this file itself makes, not delegated to any header
//     above it, is documented in full here: the ALTERNATE-DOMAIN CLASSIFIER.
//
//     THE ALTERNATE-DOMAIN CLASSIFIER (`resolve_effective_domain`, below) -- WHY IT EXISTS AND
//     EXACTLY WHAT IT DOES NOT CLAIM: `property_registry.hpp`'s own header states, in so many
//     words, that deciding which of a `has_alternate_domain` property's two domains a given raw
//     value text actually is "needs the actual value text, parser territory... out of this item's
//     own brief". This dumper is the first (and, per the pipeline's own six named stages, the
//     ONLY) place that both HAS the raw text and NEEDS the answer -- so this classifier is that
//     territory, claimed explicitly, not smuggled in as a side effect of something else. It is
//     deliberately NARROW and SYNTACTIC (shape of the token, never semantic validity) -- the exact
//     same discipline `shorthand.cpp`'s own `looks_like_number_token`/`looks_like_length_token`
//     precedent already established for an structurally IDENTICAL problem (routing a raw token to
//     one of two candidate domains by what it LOOKS like, not by fully validating it) -- this file
//     could not reuse those two functions directly (they are `.cpp`-local to shorthand.cpp, never
//     exposed via shorthand.hpp), so it re-derives the identical narrow-classifier SHAPE rather
//     than reinventing a different one.
//
//     THE FOUR PAIRS THIS CLASSIFIER HANDLES, EXHAUSTIVELY, BY CONSTRUCTION -- verified against
//     `property_registry.cpp`'s own `kTable` (read directly, not guessed): every `two(...)` row in
//     that table uses exactly one of these four `(domain, alternate_domain)` shapes today. A FIFTH
//     shape appearing in a future registry change is NOT silently guessed here -- the fallback
//     branch below defaults to the PRIMARY domain and this file's own `dumper_sanity.cpp` test
//     suite documents that this is the complete, closed enumeration as of this writing (this
//     item's own "enumerate the small space, don't search inside it" discipline, cited by the
//     brief that dispatched this item, applied to a classifier's own domain space):
//       (1) `(Keyword, LengthPercent)` -- the majority: `bottom`/`flex-basis`/`height`/`left`/
//           `margin-*`/`max-height`/`max-width`/`right`/`top`/`vertical-align`/`width`. A raw text
//           that LOOKS numeric (leading digit, `-`, or `.`) is `LengthPercent`; anything else
//           (`auto`, `none`, `baseline`, ...) is `Keyword`.
//       (2) `(Keyword, Length)` -- `letter-spacing` alone. Same numeric-leading test routes to
//           `Length` instead of `LengthPercent` (this property has no `%` reading at all, spec
//           section 6.1's own cell: "keyword(normal) or length").
//       (3) `(Number, LengthPercent)` -- `line-height` alone. BOTH sides of this pair start with a
//           digit (`"1.2"` is `Number`, `"150%"`/`"20px"` are `LengthPercent`) -- the
//           numeric-leading test alone cannot distinguish them, so this branch instead checks for
//           a `%` suffix OR a recognised length-unit suffix (`px`/`dp`) FIRST; only a bare number
//           with neither falls through to `Number`.
//       (4) `(Keyword, String)` -- `text-overflow` alone. Neither side "looks like" a shape a
//           numeric-leading test could route -- this branch instead checks against the CLOSED,
//           2-member keyword enumeration spec section 6.1's own cell actually names
//           ("keyword(clip,ellipsis) or string"): exactly `"clip"`/`"ellipsis"` route to `Keyword`,
//           anything else (a custom truncation string an author wrote) routes to `String`.
//
//     THE ANIMATION GAP: `value_compute.hpp` ships no `compute_animation` function at all (its own
//     header names why -- a genuine spec-vs-upstream field-count mismatch it refuses to guess
//     past). `try_print_for_domain` below therefore has an explicit `"animation"` branch that
//     accepts ONLY the empty-list spellings (`""`/`"none"`, spec section 9's own empty-list rule)
//     and reports `Invalid` for anything else -- which `canonical_print`'s own one-shot fallback
//     then resolves against the property's own registry `initial_value` (always `"none"` for
//     `animation`, spec section 6.1), so a real, well-formed-looking `animation` declaration always
//     dumps as `"none"` today. This is not a hidden simplification: it is one explicit branch, one
//     comment, one test (`dumper_sanity.cpp`'s own
//     `test_domain_routing_alternate_and_fail_high_fallback`) proving it never crashes or prints
//     garbage for a real-shaped input.
//
//     FAIL-HIGH RETRY, ONE SHOT, NOT RECURSIVE: `canonical_print` tries the winning raw text once;
//     on `Invalid` it tries EXACTLY the registry's own `initial_value` once more, never a chain of
//     further fallbacks -- because every registry `initial_value` this repo ships is a literal this
//     item's own author verified parses successfully under its own declared domain (spec section
//     6.1's own table, restated verbatim by `property_registry.cpp`), so a second `Invalid` would
//     mean the REGISTRY itself is broken, not this dumper -- an unreachable-by-construction case
//     this file still handles defensively (prints the initial value's raw text, unprocessed,
//     rather than aborting the whole dump) per this project's own "never let one bad property poison
//     the entire dump" discipline (spec section 11's own fail-high framing, applied one level up).
// PT: UIX-RCSS-DUMP-B -- implementação. Ver o próprio comentário de cabeçalho do dumper.hpp pro
//     escopo/fronteira/racional-de-decisão completos a que este arquivo se prende; os comentários
//     aqui são sobre COMO, não PORQUÊ no nível de arquivo (mesma disciplina que o próprio
//     dumper.cpp do irmão DOM segue) -- mas a ÚNICA decisão nova que este arquivo toma sozinho, não
//     delegada a nenhum header acima dele, está documentada por completo aqui: o CLASSIFICADOR DE
//     DOMÍNIO-ALTERNATIVO.
//
//     O CLASSIFICADOR DE DOMÍNIO-ALTERNATIVO (`resolve_effective_domain`, abaixo) -- POR QUE EXISTE
//     E EXATAMENTE O QUE NÃO AFIRMA: o próprio cabeçalho do property_registry.hpp declara, com
//     todas as letras, que decidir a qual dos dois domínios de uma propriedade
//     `has_alternate_domain` um dado texto de valor cru de fato pertence "precisa do próprio texto
//     de valor, território de parser... fora do próprio briefing deste item". Este dumper é o
//     primeiro (e, pelos próprios seis estágios nomeados do pipeline, o ÚNICO) lugar que TEM o
//     texto cru E PRECISA da resposta -- então este classificador é esse território, reivindicado
//     explicitamente, não contrabandeado como efeito colateral de outra coisa. É deliberadamente
//     ESTREITO e SINTÁTICO (forma do token, nunca validade semântica) -- exatamente a mesma
//     disciplina que o próprio precedente `looks_like_number_token`/`looks_like_length_token` do
//     shorthand.cpp já estabeleceu pra um problema estruturalmente IDÊNTICO (rotear um token cru
//     pra um de dois domínios candidatos pela CARA que ele tem, não validando-o por completo) --
//     este arquivo não conseguiu reusar as duas funções diretamente (são locais-ao-.cpp do
//     shorthand.cpp, nunca expostas via shorthand.hpp), então re-deriva a mesma FORMA de
//     classificador estreito em vez de inventar uma diferente.
//
//     OS QUATRO PARES QUE ESTE CLASSIFICADOR TRATA, EXAUSTIVAMENTE, POR CONSTRUÇÃO -- verificado
//     contra o próprio `kTable` do property_registry.cpp (lido direto, não chutado): toda linha
//     `two(...)` daquela tabela usa exatamente uma destas quatro formas `(domain,
//     alternate_domain)` hoje. Uma QUINTA forma aparecendo numa futura mudança de registro NÃO é
//     chutada em silêncio aqui -- o ramo de fallback abaixo assume o domínio PRIMÁRIO por padrão e
//     a própria suíte de teste dumper_sanity.cpp deste arquivo documenta que esta é a enumeração
//     completa, fechada, no momento desta escrita (a própria disciplina "enumere o espaço pequeno,
//     não busque dentro dele" deste item, citada pelo briefing que o despachou, aplicada ao próprio
//     espaço de domínio de um classificador):
//       (1) `(Keyword, LengthPercent)` -- a maioria: `bottom`/`flex-basis`/`height`/`left`/
//           `margin-*`/`max-height`/`max-width`/`right`/`top`/`vertical-align`/`width`. Um texto
//           cru com CARA numérica (dígito, `-`, ou `.` liderando) é `LengthPercent`; qualquer outra
//           coisa (`auto`, `none`, `baseline`, ...) é `Keyword`.
//       (2) `(Keyword, Length)` -- só `letter-spacing`. O mesmo teste de-liderança-numérica roteia
//           pra `Length` em vez de `LengthPercent` (esta propriedade não tem leitura `%` nenhuma,
//           a própria célula da seção 6.1 da spec: "keyword(normal) ou length").
//       (3) `(Number, LengthPercent)` -- só `line-height`. OS DOIS lados deste par começam com
//           dígito (`"1.2"` é `Number`, `"150%"`/`"20px"` são `LengthPercent`) -- o teste de
//           liderança-numérica sozinho não consegue distingui-los, então este ramo em vez disso
//           checa por sufixo `%` OU sufixo de unidade-de-comprimento reconhecido (`px`/`dp`)
//           PRIMEIRO; só um número cru sem nenhum dos dois cai pra `Number`.
//       (4) `(Keyword, String)` -- só `text-overflow`. Nenhum dos dois lados "tem cara" de forma
//           que um teste de liderança-numérica conseguisse rotear -- este ramo em vez disso checa
//           contra a enumeração FECHADA, de 2 membros, que a própria célula da seção 6.1 da spec de
//           fato nomeia ("keyword(clip,ellipsis) ou string"): exatamente `"clip"`/`"ellipsis"`
//           roteiam pra `Keyword`, qualquer outra coisa (uma string de truncamento custom que um
//           autor escreveu) roteia pra `String`.
//
//     A LACUNA DO ANIMATION: o `value_compute.hpp` não entrega função `compute_animation` nenhuma
//     (o próprio cabeçalho dele nomeia por quê -- um descompasso genuíno spec-vs-upstream de
//     contagem-de-campo que se recusa a chutar). O `try_print_for_domain` abaixo portanto tem um
//     ramo `"animation"` explícito que aceita SÓ as grafias de lista-vazia (`""`/`"none"`, a
//     própria regra de lista-vazia da seção 9 da spec) e reporta `Invalid` pra qualquer outra coisa
//     -- que o próprio fallback de um-tiro do `canonical_print` então resolve contra o próprio
//     `initial_value` de registro da propriedade (sempre `"none"` pro `animation`, seção 6.1 da
//     spec), então uma declaração `animation` real, com cara de bem-formada, sempre dumpa como
//     `"none"` hoje. Isto não é uma simplificação escondida: é um ramo explícito, um comentário, um
//     teste (o próprio `test_domain_routing_alternate_and_fail_high_fallback` do
//     dumper_sanity.cpp) provando que nunca trava nem imprime lixo pra um input com cara real.
//
//     RETRY FAIL-HIGH, UM TIRO SÓ, NÃO RECURSIVO: `canonical_print` tenta o texto cru vencedor uma
//     vez; em `Invalid` tenta EXATAMENTE o próprio `initial_value` de registro mais uma vez, nunca
//     uma cadeia de mais fallbacks -- porque todo `initial_value` de registro que este repo entrega
//     é um literal que o próprio autor deste item verificou parsear com sucesso sob o próprio
//     domínio declarado dele (a própria tabela da seção 6.1 da spec, restatada verbatim pelo
//     property_registry.cpp), então um segundo `Invalid` significaria que o PRÓPRIO REGISTRO está
//     quebrado, não este dumper -- um caso inalcançável-por-construção que este arquivo ainda trata
//     defensivamente (imprime o próprio texto cru do valor inicial, não-processado, em vez de
//     abortar o dump inteiro) pela própria disciplina "nunca deixe uma propriedade ruim envenenar o
//     dump inteiro" deste projeto (a própria formulação fail-high da seção 11 da spec, aplicada um
//     nível acima).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/dumper.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "uix/style/property_registry.hpp"
#include "uix/style/selector_match.hpp"
#include "uix/style/value_compute.hpp"

namespace glintfx::uix::style {

namespace {

// ---------------------------------------------------------------------------
// EN: Narrow, syntactic token-shape helpers (this file's own header, "The alternate-domain
//     classifier" -- shape only, never semantic validity; the actual VALIDATING parse happens
//     inside value_compute.hpp's own parse_length/parse_percent, called after classification).
// PT: Helpers de forma-de-token, estreitos, sintáticos (o próprio cabeçalho deste arquivo, "O
//     classificador de domínio-alternativo" -- só forma, nunca validade semântica; o parse que de
//     fato VALIDA acontece dentro do próprio parse_length/parse_percent do value_compute.hpp,
//     chamado depois da classificação).
// ---------------------------------------------------------------------------

bool ends_with(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool looks_percent_shaped(std::string_view raw) {
  return !raw.empty() && raw.back() == '%';
}

bool looks_length_unit_suffixed(std::string_view raw) {
  return ends_with(raw, "px") || ends_with(raw, "dp");
}

bool is_numeric_leading(std::string_view raw) {
  if (raw.empty()) {
    return false;
  }
  const unsigned char c = static_cast<unsigned char>(raw.front());
  return std::isdigit(c) != 0 || raw.front() == '-' || raw.front() == '.';
}

// EN: This file's header, "The four pairs this classifier handles, exhaustively, by
//     construction". `info.has_alternate_domain == false` is the common case and is handled by
//     the caller before this function is even reached (see `try_print_for_domain` below) -- this
//     function is only ever called for the 13 `two(...)` rows `property_registry.cpp` declares
//     today.
// PT: O próprio cabeçalho deste arquivo, "Os quatro pares que este classificador trata,
//     exaustivamente, por construção". `info.has_alternate_domain == false` é o caso comum e é
//     tratado pelo chamador antes mesmo desta função ser alcançada (ver `try_print_for_domain`
//     abaixo) -- esta função só é chamada mesmo pras 13 linhas `two(...)` que o
//     property_registry.cpp declara hoje.
ValueDomain resolve_effective_domain(const PropertyInfo& info, std::string_view raw) {
  const ValueDomain primary = info.domain;
  const ValueDomain alt = info.alternate_domain;

  if (primary == ValueDomain::Keyword && alt == ValueDomain::LengthPercent) {
    return is_numeric_leading(raw) ? ValueDomain::LengthPercent : ValueDomain::Keyword;
  }
  if (primary == ValueDomain::Keyword && alt == ValueDomain::Length) {
    return is_numeric_leading(raw) ? ValueDomain::Length : ValueDomain::Keyword;
  }
  if (primary == ValueDomain::Number && alt == ValueDomain::LengthPercent) {
    if (looks_percent_shaped(raw) || looks_length_unit_suffixed(raw)) {
      return ValueDomain::LengthPercent;
    }
    return ValueDomain::Number;
  }
  if (primary == ValueDomain::Keyword && alt == ValueDomain::String) {
    return (raw == "clip" || raw == "ellipsis") ? ValueDomain::Keyword : ValueDomain::String;
  }

  // EN: Unreachable by construction against today's registry (see this file's own header,
  //     "exhaustively, by construction") -- defaults to the primary domain rather than crashing or
  //     guessing, per this project's own fail-high discipline applied to an internal classifier.
  // PT: Inalcançável por construção contra o registro de hoje (ver o próprio cabeçalho deste
  //     arquivo, "exaustivamente, por construção") -- assume o domínio primário em vez de travar ou
  //     chutar, pela própria disciplina fail-high deste projeto aplicada a um classificador
  //     interno.
  return primary;
}

// EN: This module's own copy of value_compute.cpp's `parse_float_token` algorithm (that function
//     is `.cpp`-local there, never exposed via value_compute.hpp) -- same `std::strtof`,
//     whole-string, non-finite-rejecting shape, needed here so a `Number`-domain raw value (e.g.
//     `flex-grow`'s own `"0"`, `opacity`'s own `"0.5"`) can be widened to `float` before handing it
//     to `print_number()`.
// PT: A própria cópia deste módulo do algoritmo `parse_float_token` do value_compute.cpp (aquela
//     função é local-ao-.cpp lá, nunca exposta via value_compute.hpp) -- mesma forma
//     `std::strtof`, string-inteira, rejeita-não-finito, precisa aqui pra um valor cru de domínio
//     `Number` (ex. o próprio `"0"` de `flex-grow`, o próprio `"0.5"` de `opacity`) poder ser
//     ampliado pra `float` antes de entregar ao `print_number()`.
bool parse_plain_number(std::string_view raw, float* out) {
  if (raw.empty() || raw.size() > 64) {
    return false;
  }
  char buf[65];
  for (std::size_t i = 0; i < raw.size(); ++i) {
    buf[i] = raw[i];
  }
  buf[raw.size()] = '\0';
  char* end = nullptr;
  const float v = std::strtof(buf, &end);
  if (end != buf + raw.size()) {
    return false;
  }
  if (!std::isfinite(v)) {
    return false;
  }
  *out = v;
  return true;
}

bool try_print_length(std::string_view raw, float dp_ratio, std::string* out) {
  float value = 0.0f;
  LengthUnit unit = LengthUnit::Px;
  if (parse_length(raw, &value, &unit) != ValueComputeStatus::Ok) {
    return false;
  }
  *out = print_length_px(resolve_length_px(value, unit, dp_ratio));
  return true;
}

bool try_print_length_percent(std::string_view raw, float dp_ratio, std::string* out) {
  if (looks_percent_shaped(raw)) {
    float pct = 0.0f;
    if (parse_percent(raw, &pct) != ValueComputeStatus::Ok) {
      return false;
    }
    *out = print_percent(pct);
    return true;
  }
  return try_print_length(raw, dp_ratio, out);
}

// EN: This file's header, "The animation gap". Only the two empty-list spellings spec section 9
//     names (`""`, the registry's own composite-empty initial spelling for `decorator`-family
//     properties, and `"none"`, the spelling `animation`/`box-shadow`/`transform` actually use)
//     succeed; anything else is `Invalid`, letting `canonical_print`'s own one-shot fallback take
//     over.
// PT: O próprio cabeçalho deste arquivo, "A lacuna do animation". Só as duas grafias de
//     lista-vazia que a seção 9 da spec nomeia (`""`, a própria grafia de inicial
//     composto-vazio do registro pras propriedades da família `decorator`, e `"none"`, a grafia
//     que `animation`/`box-shadow`/`transform` de fato usam) têm sucesso; qualquer outra coisa é
//     `Invalid`, deixando o próprio fallback de um-tiro do `canonical_print` assumir.
bool try_print_animation(std::string_view raw, std::string* out) {
  if (raw.empty() || raw == "none") {
    *out = "none";
    return true;
  }
  return false;
}

// EN: Dispatches a Composite-domain property by NAME (property_registry.hpp's own
//     `ValueDomain::Composite` is one tag for five different per-kind grammars, spec section 9 --
//     this file, not the registry, knows which of the 7 Composite-tagged property NAMES needs
//     which value_compute.hpp function).
// PT: Despacha uma propriedade de domínio Composite por NOME (o próprio
//     `ValueDomain::Composite` do property_registry.hpp é uma tag só pra cinco gramáticas
//     por-espécie diferentes, seção 9 da spec -- este arquivo, não o registro, sabe qual dos 7
//     NOMES de propriedade tagueados Composite precisa de qual função do value_compute.hpp).
bool try_print_composite(std::string_view name, std::string_view raw, float dp_ratio,
                         std::string* out) {
  if (name == "box-shadow") {
    return compute_box_shadow(raw, dp_ratio, out) == ValueComputeStatus::Ok;
  }
  if (name == "decorator" || name == "mask-image" || name == "filter" ||
      name == "backdrop-filter") {
    return compute_decorator_list(raw, dp_ratio, out) == ValueComputeStatus::Ok;
  }
  if (name == "transform") {
    return compute_transform_list(raw, dp_ratio, out) == ValueComputeStatus::Ok;
  }
  if (name == "animation") {
    return try_print_animation(raw, out);
  }
  // EN: Unreachable by construction -- these 7 names are the only Composite-tagged entries
  //     property_registry.cpp declares today (see this file's own header, same "exhaustively, by
  //     construction" discipline as resolve_effective_domain above).
  // PT: Inalcançável por construção -- estes 7 nomes são as únicas entradas tagueadas Composite
  //     que o property_registry.cpp declara hoje (ver o próprio cabeçalho deste arquivo, mesma
  //     disciplina "exaustivamente, por construção" do resolve_effective_domain acima).
  return false;
}

// EN: One dispatch attempt, given the property's OWN registry info and a CANDIDATE raw text (the
//     winning declared value on the first attempt, or the registry's own `initial_value` on the
//     retry `canonical_print` performs below). Returns false (never touching `*out`) exactly when
//     the corresponding value_compute.hpp function would have signalled `Invalid` -- this
//     function's own caller decides what a failed attempt means (spec section 11's own fail-high
//     policy, applied at THIS print stage rather than left to a future cascade rewrite).
// PT: Uma tentativa de despacho, dado o próprio info de registro da propriedade e um texto cru
//     CANDIDATO (o valor declarado vencedor na primeira tentativa, ou o próprio `initial_value` de
//     registro no retry que o `canonical_print` faz abaixo). Retorna falso (nunca tocando `*out`)
//     exatamente quando a função correspondente do value_compute.hpp teria sinalizado `Invalid` --
//     o próprio chamador desta função decide o que uma tentativa falha significa (a própria
//     política fail-high da seção 11 da spec, aplicada NESTE estágio de impressão em vez de deixada
//     pra uma futura reescrita de cascata).
bool try_print_for_domain(const PropertyInfo& info, std::string_view name, std::string_view raw,
                          float dp_ratio, std::string* out) {
  const ValueDomain effective =
      info.has_alternate_domain ? resolve_effective_domain(info, raw) : info.domain;

  switch (effective) {
    case ValueDomain::Keyword:
      // EN: See dumper.hpp's own header, "keyword-domain values are not validated" -- always
      //     succeeds, printed verbatim.
      // PT: Ver o próprio cabeçalho do dumper.hpp, "valores de domínio-keyword não são validados"
      //     -- sempre tem sucesso, impresso verbatim.
      *out = std::string(raw);
      return true;
    case ValueDomain::Number: {
      float x = 0.0f;
      if (!parse_plain_number(raw, &x)) {
        return false;
      }
      *out = print_number(x);
      return true;
    }
    case ValueDomain::Length:
      return try_print_length(raw, dp_ratio, out);
    case ValueDomain::LengthPercent:
      return try_print_length_percent(raw, dp_ratio, out);
    case ValueDomain::Color: {
      Rgba8 rgba{};
      if (parse_color(raw, &rgba) != ValueComputeStatus::Ok) {
        return false;
      }
      *out = print_color(rgba);
      return true;
    }
    case ValueDomain::String:
      // EN: print_string() itself never signals failure (dequote + escape only) -- see
      //     value_compute.hpp's own header for print_string's own contract.
      // PT: o próprio print_string() nunca sinaliza falha (só tira-aspas + escapa) -- ver o
      //     próprio cabeçalho do value_compute.hpp pro próprio contrato do print_string.
      *out = print_string(raw);
      return true;
    case ValueDomain::Composite:
      return try_print_composite(name, raw, dp_ratio, out);
  }
  // EN: Unreachable -- ValueDomain is a closed 7-member enum, every member handled above.
  // PT: Inalcançável -- ValueDomain é um enum fechado de 7 membros, todos tratados acima.
  return false;
}

// EN: `docs/uix-dom.md` section 3's own path-addressing scheme (spec section 2's own "reused
//     unchanged" clause), mirroring `glintfx/src/uix/dom/dumper.cpp`'s own `dump_element`
//     traversal EXACTLY (same child-index-counts-every-child, same recurse-only-into-Elements
//     shape) -- collects one path string per Element, in the SAME pre-order depth-first sequence
//     `cascade_tree` (cascade.hpp) is documented to walk, so the two vectors zip 1:1 by index
//     below without either side needing to know the other's own internal traversal code.
// PT: O próprio esquema de endereçamento de caminho da seção 3 do docs/uix-dom.md (a própria
//     cláusula "reusado sem mudança" da seção 2 da spec), espelhando EXATAMENTE a própria
//     travessia `dump_element` do glintfx/src/uix/dom/dumper.cpp (mesmo
//     índice-de-filho-conta-todo-filho, mesma forma recursa-só-em-Elements) -- coleta uma string de
//     caminho por Element, na MESMA sequência pré-ordem, profundidade-primeiro que o
//     `cascade_tree` (cascade.hpp) está documentado a percorrer, então os dois vetores se
//     emparelham 1:1 por índice abaixo sem nenhum dos dois lados precisar saber o próprio código
//     de travessia interno do outro.
void collect_element_paths(const glintfx::uix::Element& element, const std::string& path,
                           std::vector<std::string>* out) {
  out->push_back(path);
  std::size_t index = 0;
  for (const auto& child : element.children()) {
    std::string child_path = path;
    child_path += '/';
    child_path += std::to_string(index);
    if (const glintfx::uix::Element* child_element = glintfx::uix::as_element(child.get())) {
      collect_element_paths(*child_element, child_path, out);
    }
    ++index;
  }
}

// EN: `UIX-EM-UNIT`'s own UA-default font-size, in px -- `property_registry.cpp`'s own `font-size`
//     row (`one("font-size", "12px", true, LEN)`), matching this file's own PRE-existing fallback
//     (`canonical_print()`'s own one-shot retry against the registry's `initial_value`, unchanged)
//     for the ONE case this ceiling is reachable at all: the document root's own conceptual parent
//     (a real DOM has no ancestor above `body`, so `body`'s own `em`, if it ever declared one, needs
//     SOME starting point -- not exercised by this repo's own corpus today, docs/uix-rcss-censo.md's
//     own measured `em` count is exactly 1, `fonteng_sup_scene.rml`'s `.sup`, one level below
//     `body`'s own absolute `64px`). A literal, not a `find_property("font-size")->initial_value`
//     re-parse, for the same reason `kMaxQuantizeMagnitude` above is a literal, not a runtime lookup
//     -- this value is exercised by every dump this function ever produces (unlike a rare boundary
//     ceiling), so a `find_property` call here would be a hot-path indirection this file's own
//     sibling `value_compute.hpp` header already avoids for its OWN literals; kept in sync with
//     `property_registry.cpp`'s own row by this comment's own explicit cross-reference, not by
//     shared code (the two files are deliberately independent registries per this project's own
//     ADR-0020 discipline elsewhere in this module).
// PT: O próprio font-size default de UA da `UIX-EM-UNIT`, em px -- a própria linha `font-size` do
//     property_registry.cpp (`one("font-size", "12px", true, LEN)`), casando com o próprio fallback
//     PRÉ-existente deste arquivo (o próprio retry de um-tiro do `canonical_print()` contra o
//     `initial_value` do registro, inalterado) pro ÚNICO caso em que este teto é alcançável de
//     jeito nenhum: o próprio pai conceitual da raiz do documento (um DOM real não tem ancestral
//     acima do `body`, então o próprio `em` do `body`, se algum dia declarasse um, precisa de ALGUM
//     ponto de partida -- não exercitado pelo próprio corpus deste repo hoje, a própria contagem
//     medida de `em` do docs/uix-rcss-censo.md é exatamente 1, o `.sup` do `fonteng_sup_scene.rml`,
//     um nível abaixo do próprio `64px` absoluto do `body`). Um literal, não um re-parse de
//     `find_property("font-size")->initial_value`, pelo MESMO motivo que o `kMaxQuantizeMagnitude`
//     acima é um literal, não uma consulta em tempo de execução -- este valor é exercitado por todo
//     dump que esta função algum dia produz (diferente de um teto raro de fronteira), então uma
//     chamada `find_property` aqui seria uma indireção de hot-path que o próprio cabeçalho irmão
//     value_compute.hpp já evita pros PRÓPRIOS literais dele; mantido em sincronia com a própria
//     linha do property_registry.cpp por esta própria referência-cruzada explícita do comentário,
//     não por código compartilhado (os dois arquivos são deliberadamente registros independentes per
//     a própria disciplina ADR-0020 deste módulo em outro lugar).
constexpr float kUaDefaultFontSizePx = 12.0f;

// EN: `UIX-EM-UNIT` -- resolves each node's own `font-size` property to a concrete pixel value, in
//     the SAME pre-order (parent-before-child) sequence `paths`/`node_styles` already share, so
//     `em`'s own parent-relative multiplier (docs/uix-rcss.md section 1's own "em/rem against the
//     font-size chain" clause) has a real ancestor value to multiply against -- something
//     `canonical_print()`'s own per-property, ancestor-blind signature structurally cannot supply
//     (see dumper.hpp's own header, `dump_style()`'s own doc-comment, for the full rationale).
//     Absolute (`px`/`dp`) and unitless-zero declarations resolve exactly as `canonical_print()`
//     already would (delegates to the SAME `parse_font_size()`, which itself delegates to
//     `parse_length()`/`resolve_length_px()` for those two units, unchanged) -- the ONLY declarations
//     whose print output diverges from plain `canonical_print()` are the `em`-shaped ones.
//     `by_element` is safe to key by raw `Element*` (never dereferenced past the lifetime of the
//     `node_styles` vector this function's own caller owns for the duration of one `dump_style()`
//     call, same "observer, not owner" contract `NodeStyle::element` itself already documents) --
//     every child's own parent is guaranteed already inserted by the time that child is visited
//     (`cascade_tree()`'s own pre-order-depth-first traversal, restated by this file's own
//     `collect_element_paths()` doc-comment above), so a lookup miss is unreachable by construction;
//     handled defensively anyway (falls back to `kUaDefaultFontSizePx`) rather than assuming.
// PT: `UIX-EM-UNIT` -- resolve o próprio "font-size" de cada nó pra um valor de pixel concreto, na
//     MESMA sequência pré-ordem (pai-antes-de-filho) que `paths`/`node_styles` já compartilham, pra
//     que o próprio multiplicador relativo-ao-pai do `em` (a própria cláusula "em/rem contra a
//     cadeia de font-size" da seção 1 do docs/uix-rcss.md) tenha um valor ancestral real pra
//     multiplicar -- algo que a própria assinatura por-propriedade, cega-a-ancestral, do
//     `canonical_print()` estruturalmente não consegue fornecer (ver o próprio cabeçalho do
//     dumper.hpp, o próprio doc-comment do `dump_style()`, pro racional completo). Declarações
//     absolutas (`px`/`dp`) e zero-sem-unidade resolvem EXATAMENTE como o `canonical_print()` já
//     resolveria (delega pro MESMO `parse_font_size()`, que ele mesmo delega pro
//     `parse_length()`/`resolve_length_px()` pras duas unidades, inalterado) -- as ÚNICAS
//     declarações cuja saída de impressão diverge do `canonical_print()` puro são as com forma
//     `em`. `by_element` é seguro de indexar pelo `Element*` cru (nunca desreferenciado além da
//     vida do próprio vetor `node_styles` que o próprio chamador desta função possui durante UMA
//     chamada de `dump_style()`, mesmo contrato "observador, não dono" que o próprio
//     `NodeStyle::element` já documenta) -- o próprio pai de todo filho está garantido já inserido
//     no momento em que aquele filho é visitado (a própria travessia pré-ordem-profundidade-primeiro
//     do `cascade_tree()`, restatada pelo próprio doc-comment do `collect_element_paths()` acima),
//     então um miss de busca é inalcançável por construção; tratado defensivamente mesmo assim (cai
//     pro `kUaDefaultFontSizePx`) em vez de assumir.
std::vector<float> resolve_font_size_chain_px(const std::vector<std::string>& paths,
                                              const std::vector<NodeStyle>& node_styles,
                                              float dp_ratio) {
  const std::size_t n = paths.size() < node_styles.size() ? paths.size() : node_styles.size();
  std::vector<float> resolved(n, kUaDefaultFontSizePx);
  std::unordered_map<const glintfx::uix::Element*, float> by_element;
  by_element.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    const glintfx::uix::Element* element = node_styles[i].element;
    const ComputedStyle& style = node_styles[i].style;

    float parent_px = kUaDefaultFontSizePx;
    if (element != nullptr) {
      const glintfx::uix::Element* parent = element->parent();
      if (parent != nullptr) {
        const auto it = by_element.find(parent);
        if (it != by_element.end()) {
          parent_px = it->second;
        }
      }
    }

    float px = kUaDefaultFontSizePx;
    // EN: `useStlAlgorithm` (cppcheck) -- `std::find_if` instead of a raw hand-rolled loop, same
    //     lookup, same early-exit-on-match semantics.
    // PT: `useStlAlgorithm` (cppcheck) -- `std::find_if` em vez de um laço escrito à mão, a mesma
    //     busca, a mesma semântica de saída antecipada ao achar.
    const auto font_size_it =
        std::find_if(style.begin(), style.end(),
                     [](const ComputedProperty& prop) { return prop.name == "font-size"; });
    if (font_size_it != style.end()) {
      float out_px = 0.0f;
      if (parse_font_size(font_size_it->value, parent_px, dp_ratio, &out_px) ==
          ValueComputeStatus::Ok) {
        px = out_px;
      }
      // EN: `Invalid` (a malformed/unresolvable value) falls back to `kUaDefaultFontSizePx`, the
      //     SAME registry-initial-value fallback `canonical_print()`'s own one-shot retry already
      //     uses for every other property -- this function does not invent a second fail-high
      //     policy.
      // PT: `Invalid` (um valor malformado/não-resolvível) cai pro `kUaDefaultFontSizePx`, o
      //     MESMO fallback de valor-inicial-de-registro que o próprio retry de um-tiro do
      //     `canonical_print()` já usa pra toda outra propriedade -- esta função não inventa uma
      //     segunda política fail-high.
    }

    resolved[i] = px;
    if (element != nullptr) {
      by_element[element] = px;
    }
  }
  return resolved;
}

} // namespace

std::string canonical_print(const ComputedProperty& prop, float dp_ratio) {
  const PropertyInfo* info = find_property(prop.name);
  if (info == nullptr) {
    // EN: Unreachable by construction (see dumper.hpp's own header) -- `prop.name` always comes
    //     from a `ComputedStyle` built by walking `all_properties()`. Defensive, not crashing.
    // PT: Inalcançável por construção (ver o próprio cabeçalho do dumper.hpp) -- `prop.name`
    //     sempre vem de um `ComputedStyle` construído percorrendo `all_properties()`. Defensivo,
    //     sem travar.
    return prop.value;
  }

  std::string result;
  if (try_print_for_domain(*info, prop.name, prop.value, dp_ratio, &result)) {
    return result;
  }
  // EN: Spec section 11's own fail-high fallback, applied at print time (dumper.hpp's own header,
  //     "keyword-domain values are not validated" names the one domain this cannot help).
  // PT: O próprio fallback fail-high da seção 11 da spec, aplicado em tempo de impressão (o
  //     próprio cabeçalho do dumper.hpp, "valores de domínio-keyword não são validados" nomeia o
  //     único domínio em que isto não ajuda).
  if (try_print_for_domain(*info, prop.name, info->initial_value, dp_ratio, &result)) {
    return result;
  }
  // EN: Unreachable by construction against today's registry (this file's own header, "fail-high
  //     retry, one shot, not recursive") -- defensive last resort, never crashes.
  // PT: Inalcançável por construção contra o registro de hoje (o próprio cabeçalho deste arquivo,
  //     "retry fail-high, um tiro só, não recursivo") -- último recurso defensivo, nunca trava.
  return std::string(info->initial_value);
}

std::string dump_style(const StyleSheet& sheet, const glintfx::uix::Element& root, float dp_ratio) {
  std::string out;

  std::vector<std::string> paths;
  collect_element_paths(root, "body", &paths);

  // EN: Spec section 4's own fixed, prose-declared file order (`none` first) -- explicitly NOT the
  //     byte-wise sort rule section 6 uses for property names ("hover-all" sorts before "none"
  //     byte-wise, the opposite order), so this table is hardcoded in spec order, never derived by
  //     sorting the two state names.
  // PT: A própria ordem fixa, prosa-declarada, de arquivo da seção 4 da spec (`none` primeiro) --
  //     explicitamente NÃO a regra de sort byte-a-byte que a seção 6 usa pra nomes de propriedade
  //     ("hover-all" ordena antes de "none" byte-a-byte, a ordem oposta), então esta tabela é
  //     hardcoded na ordem da spec, nunca derivada ordenando os dois nomes de estado.
  struct StateEntry {
    const char* name;
    bool hover_active;
  };
  static constexpr StateEntry kStates[] = {
      {"none", false},
      {"hover-all", true},
  };

  for (const StateEntry& state_entry : kStates) {
    out += "STATE ";
    out += state_entry.name;
    out += '\n';

    const MatchState state{state_entry.hover_active};
    const std::vector<NodeStyle> node_styles = cascade_tree(sheet, root, state);

    // EN: `paths` and `node_styles` are built by two independently-written traversals that both
    //     walk the SAME pre-order-depth-first, elements-only sequence (see collect_element_paths's
    //     own comment above) -- they zip 1:1 by construction. Defensive `std::min` bound rather
    //     than an unchecked index, in case a future change to either traversal ever desyncs them
    //     (this dumper never crashes on its own inputs, per dumper.hpp's own header).
    // PT: `paths` e `node_styles` são construídos por duas travessias escritas de forma
    //     independente que as duas percorrem a MESMA sequência pré-ordem, profundidade-primeiro,
    //     só-elementos (ver o próprio comentário do collect_element_paths acima) -- se emparelham
    //     1:1 por construção. Limite defensivo `std::min` em vez de índice sem checagem, pro caso
    //     de uma futura mudança numa das duas travessias algum dia dessincronizá-las (este dumper
    //     nunca trava sobre os próprios inputs, pelo próprio cabeçalho do dumper.hpp).
    const std::size_t n = paths.size() < node_styles.size() ? paths.size() : node_styles.size();

    // EN: `UIX-EM-UNIT` -- see dumper.hpp's own `dump_style()` doc-comment for the full rationale.
    //     Computed once per STATE block (node_styles/paths themselves are recomputed per state just
    //     above, so this chain is too -- a hover-state font-size declaration, if the corpus ever
    //     grows one, gets its own correctly-scoped chain rather than reusing `none`'s).
    // PT: `UIX-EM-UNIT` -- ver o próprio doc-comment do `dump_style()` no dumper.hpp pro racional
    //     completo. Computado uma vez por bloco STATE (node_styles/paths eles mesmos são
    //     recomputados por state logo acima, então esta cadeia também é -- uma declaração de
    //     font-size em hover-state, se o corpus algum dia crescer uma, ganha a própria cadeia
    //     corretamente escopada em vez de reusar a do `none`).
    const std::vector<float> font_size_px = resolve_font_size_chain_px(paths, node_styles, dp_ratio);

    for (std::size_t i = 0; i < n; ++i) {
      const std::string& path = paths[i];
      const ComputedStyle& style = node_styles[i].style;

      out += path;
      out += " PROPS ";
      out += std::to_string(style.size());
      out += '\n';

      for (const ComputedProperty& prop : style) {
        out += path;
        out += " PROP ";
        out += prop.name; // structural identifier, never escaped -- spec section 7's own rule.
        out += '=';
        // EN: `UIX-EM-UNIT` -- `font-size` alone is printed from the pre-resolved chain above
        //     (handles `em`, delegates identically to `canonical_print()` for every other unit);
        //     every other property is completely unaffected, same `canonical_print()` call as
        //     before.
        // PT: `UIX-EM-UNIT` -- só o `font-size` é impresso a partir da cadeia pré-resolvida acima
        //     (trata `em`, delega identicamente ao `canonical_print()` pra toda outra unidade); toda
        //     outra propriedade fica completamente inafetada, a mesma chamada `canonical_print()` de
        //     antes.
        if (prop.name == "font-size") {
          out += print_length_px(font_size_px[i]);
        } else {
          out += canonical_print(prop, dp_ratio);
        }
        out += '\n';
      }
    }
  }

  return out;
}

} // namespace glintfx::uix::style
